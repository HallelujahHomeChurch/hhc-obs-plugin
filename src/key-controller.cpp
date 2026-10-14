#include "key-controller.hpp"
#include "ingest.hpp"
#include "async.hpp"
#include "hls-muxer.hpp"
#include "windows-path.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <limits>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QUuid>
#include <windows.h>
namespace hhc {
namespace {
QJsonObject read(const QString &path)
{
	QFile f(path);
	QJsonParseError e;
	if (!openSharedJsonRead(f) || f.size() > 8 * 1024 * 1024)
		throw RequestError(409, "ingest_local_conflict");
	auto d = QJsonDocument::fromJson(f.readAll(), &e);
	if (e.error != QJsonParseError::NoError || !d.isObject())
		throw RequestError(409, "ingest_local_conflict");
	return d.object();
}
} // namespace
KeyController::KeyController(QString root, QObject *parent, std::function<unsigned()> audioTrack)
	: QObject(parent),
	  root_(std::move(root) + "/k3"),
	  view_(new QWidget), audioTrackProvider_(std::move(audioTrack))
{
	auto *layout = new QVBoxLayout(view_);
	settings_ = new QWidget;
	auto *settingsLayout = new QVBoxLayout(settings_);
	key_ = new QLineEdit;
	key_->setEchoMode(QLineEdit::Password);
	key_->setPlaceholderText(QString::fromUtf8("串流金鑰"));
	settingsLayout->addWidget(key_);
	save_ = new QPushButton(QString::fromUtf8("儲存串流金鑰"));
	settingsLayout->addWidget(save_);
	titleLabel_ = new QLabel;
	titleLabel_->setWordWrap(true);
	titleLabel_->setTextFormat(Qt::PlainText);
	layout->addWidget(titleLabel_);
	status_ = new QLabel;
	status_->setWordWrap(true);
	layout->addWidget(status_);
	action_ = new QPushButton;
	layout->addWidget(action_);
	retry_ = new QPushButton(QString::fromUtf8("重試"));
	layout->addWidget(retry_);
	settingsButton_ = new QPushButton(QString::fromUtf8("設定"));
	layout->addWidget(settingsButton_);
	layout->addWidget(settings_);
	settings_->hide();
	connect(settingsButton_, &QPushButton::clicked, this, [this] {
		settings_->setVisible(settings_->isHidden());
		if (settings_->isHidden()) key_->clear();
	});
	auto *console = new QPushButton(QString::fromUtf8("管理直播"));
	settingsLayout->addWidget(console);
	legacyButton_ = new QPushButton(QString::fromUtf8("舊版資料恢復（進階）"));
	settingsLayout->addWidget(legacyButton_);
	connect(console, &QPushButton::clicked, this,
		[] { QDesktopServices::openUrl(QUrl("https://admin.alive.org.tw/content/broadcasts")); });
	connect(save_, &QPushButton::clicked, this, [this] {
		try {
			KeyIngest(root_ + "/intents", "settings").saveKey(key_->text().toUtf8());
			key_->clear();
			keyAvailable_ = true;
			paused_ = false;
			issue_ = QString::fromUtf8("串流金鑰已儲存。");
			settings_->hide();
		} catch (...) {
			issue_ = QString::fromUtf8("無法儲存金鑰；請確認格式及 Credential Manager。");
		}
		display();
	});
	connect(action_, &QPushButton::clicked, this, [this] { startStop(); });
	connect(retry_, &QPushButton::clicked, this, [this] {
		paused_ = false;
		next_ = nextControl_ = 0;
		failures_ = 0;
		tick();
	});
	connect(legacyButton_, &QPushButton::clicked, this, [this] {
		if (!id_.isEmpty() || syncAwaiting_ || controlAwaiting_)
			return;
		if (!legacy_)
			legacy_ = std::make_unique<PlatformController>(QDir(root_).absoluteFilePath(".."), this, true);
		legacy_->view()->setWindowTitle(QString::fromUtf8("舊版 journal 恢復（不轉換為 K3）"));
		legacy_->view()->show();
	});
	connect(&sync_, &QFutureWatcher<KeyJob>::finished, this, [this] { syncDone(); });
	connect(&control_, &QFutureWatcher<KeyJob>::finished, this, [this] {
		controlAwaiting_ = false;
		if (closing_)
			return;
		try {
			auto r = workerResult(control_.future());
			end_ = r.end;
			nextControl_ = QDateTime::currentMSecsSinceEpoch() + 2000;
		} catch (const RequestError &e) {
			if (e.code == "broadcast_ingest_revoked") {
				revoked_ = true;
				markIngestRevoked(root_, id_);
				if (capture_)
					capture_->stop(StopReason::EncoderFailure);
			}
			issue_ = e.code;
			nextControl_ = QDateTime::currentMSecsSinceEpoch() + 5000;
		} catch (...) {
			issue_ = "control_recovery_required";
			paused_ = true;
		}
		display();
	});
	QDir().mkpath(root_);
	try {
		keyAvailable_ = KeyIngest(root_ + "/intents", "settings").hasKey();
	} catch (...) {
		issue_ = "credential_store_unavailable";
	}
	lease_ = std::make_unique<QLockFile>(lockFilePath(root_ + "/input.lock"));
	lease_->setStaleLockTime(0);
	if (!lease_->tryLock(0)) {
		paused_ = true;
		issue_ = QString::fromUtf8("另一個 HHC 外掛正在使用此輸入。");
	} else if (QFileInfo::exists(root_ + "/active.json")) {
		try {
			const auto a = read(root_ + "/active.json");
			if (!a["released"].toBool()) {
				id_ = a["localId"].toString();
				stopping_ = a["stopping"].toBool();
				starting_ = !a["started"].toBool() && !stopping_;
				audioTrack_ = unsigned(qBound(1, a["audioTrack"].toInt(1), 6));
			}
		} catch (...) {
			paused_ = true;
			issue_ = QString::fromUtf8("收錄 journal 無法讀取；資料保留，請檢查。");
		}
	}
	connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
	timer_.start(500);
	display();
}
KeyController::~KeyController()
{
	shutdown();
	delete view_;
}
void KeyController::display()
{
	const bool running = capture_ && !capture_->finished();
	action_->setText(running || starting_ ? QString::fromUtf8("停止直播")
					      : QString::fromUtf8("開始直播"));
	action_->setEnabled(lease_->isLocked() && (running || starting_ || !paused_) &&
			    (id_.isEmpty() || released_ || running || starting_));
	save_->setEnabled(lease_->isLocked() && (id_.isEmpty() || released_) && !running && !starting_ && !syncAwaiting_ && !controlAwaiting_);
	key_->setEnabled(save_->isEnabled());
	key_->setPlaceholderText(keyAvailable_ ? QString::fromUtf8("已設定；輸入新金鑰以更換") : QString::fromUtf8("串流金鑰"));
	retry_->setVisible(paused_ && lease_->isLocked());
	legacyButton_->setEnabled(id_.isEmpty());
	titleLabel_->setText(title_);
	titleLabel_->setVisible(!title_.isEmpty() && !released_);
	status_->setText(issue_.isEmpty() ? QString::fromUtf8("未連線") : issue_);
}
bool KeyController::retireUnbound()
{
	if (syncAwaiting_ || id_.isEmpty() || QFileInfo::exists(root_ + "/intents/" + id_ + ".json"))
		return false;
	atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
	id_.clear();
	starting_ = stopping_ = paused_ = false;
	issue_ = QString::fromUtf8("尚未綁定，已停止等待；可修正金鑰後重新開始。");
	return true;
}
void KeyController::startStop()
{
	if (closing_ || !lease_->isLocked())
		return;
	if (capture_ && !capture_->finished()) {
		stopping_ = true;
		capture_->stop(StopReason::User);
		display();
		return;
	}
	if (starting_) {
		starting_ = false;
		stopping_ = true;
		atomicJson(root_ + "/active.json",
			   {{"localId", id_}, {"released", false}, {"started", false}, {"stopping", true}});
		retireUnbound();
		display();
		return;
	}
	if (syncAwaiting_ || controlAwaiting_ || (!id_.isEmpty() && !released_))
		return;
	if (legacy_ && legacy_->busy()) {
		issue_ = QString::fromUtf8("請先完成舊版收錄恢復。");
		display();
		return;
	}
	if (!keyAvailable_) {
		issue_ = QString::fromUtf8("請先儲存串流金鑰。");
		settings_->show();
		display();
		return;
	}
	legacy_.reset();
	const auto selectedTrack = audioTrackProvider_ ? audioTrackProvider_() : 1u;
	if (selectedTrack < 1 || selectedTrack > 6) {
		issue_ = QString::fromUtf8("無法取得 OBS 串流音軌，請檢查 OBS 輸出設定。");
		display();
		return;
	}
	audioTrack_ = selectedTrack;
	if (!id_.isEmpty())
		launchHelper();
	capture_.reset();
	id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
	actor_.clear();
	title_.clear();
	recoveryValidated_ = false;
	starting_ = true;
	stopping_ = released_ = revoked_ = paused_ = false;
	end_.reset();
	next_ = 0;
	atomicJson(root_ + "/active.json", {{"localId", id_},
					    {"released", false},
					    {"started", false},
					    {"audioTrack", int(audioTrack_)}});
	issue_ = QString::fromUtf8("正在連線…");
	tick();
	display();
}
void KeyController::tick()
{
	if (closing_ || paused_ || !lease_->isLocked() || id_.isEmpty())
		return;
	if (stopping_ && retireUnbound()) {
		display();
		return;
	}
	if (capture_ && end_ && !stopping_) {
		// Wait until all public segments are closed; the chosen marker is ahead of the encoder.
		const auto dir = SessionStore(root_ + "/queue").mediaDirectory(actor_, id_);
		bool complete = true;
		for (const auto &rendition : QStringList{"480p", "720p", "1080p"}) {
			const auto files = QDir(dir + "/" + rendition).entryList({"*.m4s"}, QDir::Files);
			complete &= files.size() >= *end_;
		}
		if (complete) {
			stopping_ = true;
			capture_->stop(StopReason::User);
		}
	}
	pollControl();
	if (syncAwaiting_ || QDateTime::currentMSecsSinceEpoch() < next_)
		return;
	const auto id = id_;
	const bool active = capture_ && !capture_->finished(), revoked = revoked_, preparing = starting_;
	syncAwaiting_ = true;
	sync_.setFuture(QtConcurrent::run([this, id, active, revoked, preparing] {
		HttpCancellationScope cancellation(cancelled_);
		return keyTransferStep(root_, id, active, revoked, preparing, recoveryValidated_, &cancelled_);
	}));
}
void KeyController::pollControl()
{
	if (controlAwaiting_ || starting_ || actor_.isEmpty() || !capture_ || capture_->finished() || stopping_ ||
	    revoked_ || QDateTime::currentMSecsSinceEpoch() < nextControl_)
		return;
	const auto id = id_, actor = actor_;
	const auto boundary = capture_->boundaryClock();
	controlAwaiting_ = true;
	control_.setFuture(QtConcurrent::run([this, id, actor, boundary] {
		HttpCancellationScope cancellation(cancelled_);
		KeyIngest ingest(root_ + "/intents", id);
		auto api = ingest.api();
		BroadcastControl control(SessionStore(root_ + "/queue").mediaDirectory(actor, id), actor, id, api,
					 true);
		control.poll(boundary);
		KeyJob out;
		out.end = control.acceptedEnd();
		return out;
	}));
}
void KeyController::syncDone()
{
	syncAwaiting_ = false;
	if (closing_)
		return;
	try {
		const auto r = workerResult(sync_.future());
		actor_ = r.actor;
		title_ = r.title;
		failures_ = 0;
		const auto dir = SessionStore(root_ + "/queue").mediaDirectory(actor_, id_);
		if (starting_ && r.bound) {
			starting_ = false;
			atomicJson(dir + "/local-session.json", {{"title", title_},
								 {"wireRevision", "c1-2026-10-08.2"},
								 {"audioTrack", int(audioTrack_)},
								 {"fpsNum", 30000},
								 {"fpsDen", 1001}});
			atomicJson(root_ + "/active.json", {{"localId", id_},
							    {"released", false},
							    {"started", true},
							    {"audioTrack", int(audioTrack_)}});
			capture_ = std::make_unique<CaptureOutput>();
			if (!capture_->start(
				    {dir, audioTrack_, root_ + "/queue", actor_, id_})) {
				capture_.reset();
				issue_ = QString::fromUtf8("NVENC 啟動失敗，保留資料並中止原 capture。");
			}
		}
		if (r.end)
			end_ = r.end;
		released_ = r.sync.stopAccepted && r.sync.sealAccepted;
		if (released_)
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
		// Bound publication belongs to Console; C1's cancellation is not the broadcast policy.
		issue_ = capture_ && !capture_->finished() ? QString::fromUtf8("傳送中")
			: r.sync.state == "ready" ? QString::fromUtf8("收錄完成")
			: QString::fromUtf8("收尾中（%1）").arg(r.sync.state);
		if (r.sync.pendingBytes)
			issue_ += QString::fromUtf8("；待傳 %1 MB").arg(double(r.sync.pendingBytes) / 1048576, 0, 'f', 1);
		if (r.sync.state == "aborted" || r.sync.state == "expired" || r.sync.state == "failed") {
			if (capture_ && !capture_->finished())
				capture_->stop(StopReason::EncoderFailure);
			released_ = true;
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
			paused_ = false;
			next_ = std::numeric_limits<qint64>::max();
		} else
			next_ = QDateTime::currentMSecsSinceEpoch() + (r.sync.pendingBytes ? 0 : 3000);
		if (r.sync.state == "ready" && r.sync.autoPublish != "pending")
			next_ = std::numeric_limits<qint64>::max();
	} catch (const RequestError &e) {
		issue_ = e.code;
		if (e.code == "broadcast_ingest_revoked") {
			revoked_ = true;
			if (capture_)
				capture_->stop(StopReason::EncoderFailure);
			next_ = 0;
		} else if (e.code == "broadcast_source_not_prepared") {
			issue_ = QString::fromUtf8("平台尚未準備接收，正在自動重試。");
			next_ = QDateTime::currentMSecsSinceEpoch() + 5000 + QRandomGenerator::global()->bounded(1000);
		} else {
			auto delay = e.retryDelay(failures_);
			paused_ = !delay;
			next_ = QDateTime::currentMSecsSinceEpoch() + 1000 * delay.value_or(0);
		}
	} catch (...) {
		paused_ = true;
		issue_ = QString::fromUtf8("恢復資料檢查未完成；原 intent 與素材保留。");
	}
	if (stopping_ || (paused_ && issue_ == "broadcast_stream_key_invalid"))
		retireUnbound();
	display();
}
void KeyController::launchHelper()
{
	if (id_.isEmpty() || !QFileInfo::exists(root_ + "/intents/" + id_ + ".json"))
		return;
	QProcess p;
	p.setProgram(QDir(QCoreApplication::applicationDirPath())
			     .absoluteFilePath("../../obs-plugins/64bit/hhc-upload-helper.exe"));
	p.setArguments({"--root", root_, "--k3", id_});
	auto env = QProcessEnvironment::systemEnvironment();
	env.insert("PATH", QCoreApplication::applicationDirPath() + ";" + env.value("PATH"));
	p.setProcessEnvironment(env);
	p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) {
		a->flags |= CREATE_NO_WINDOW;
		a->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
		a->startupInfo->wShowWindow = SW_HIDE;
	});
	p.startDetached();
}
void KeyController::shutdown()
{
	if (closing_)
		return;
	closing_ = true;
	timer_.stop();
	cancelled_ = true;
	if (capture_) {
		capture_->stop(StopReason::User);
		capture_->wait(6000);
		capture_.reset();
	}
	try {
		sync_.waitForFinished();
		control_.waitForFinished();
	} catch (...) {
	}
	launchHelper();
	legacy_.reset();
}
} // namespace hhc
