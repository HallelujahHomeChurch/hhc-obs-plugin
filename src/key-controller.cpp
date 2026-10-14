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
KeyController::KeyController(QString root, QObject *parent)
	: QObject(parent),
	  root_(std::move(root) + "/k3"),
	  view_(new QWidget)
{
	auto *layout = new QVBoxLayout(view_);
	auto *help = new QLabel(QString::fromUtf8("在 Console 準備直播；此處只需儲存一次串流金鑰。"));
	help->setWordWrap(true);
	layout->addWidget(help);
	key_ = new QLineEdit;
	key_->setEchoMode(QLineEdit::Password);
	key_->setPlaceholderText(QString::fromUtf8("串流金鑰"));
	layout->addWidget(key_);
	save_ = new QPushButton(QString::fromUtf8("儲存串流金鑰"));
	layout->addWidget(save_);
	track_ = new QComboBox;
	for (int i = 1; i <= 6; ++i)
		track_->addItem(QString::fromUtf8("音軌 %1").arg(i), i);
	layout->addWidget(track_);
	titleLabel_ = new QLabel;
	titleLabel_->setWordWrap(true);
	titleLabel_->setTextFormat(Qt::PlainText);
	layout->addWidget(titleLabel_);
	status_ = new QLabel;
	status_->setWordWrap(true);
	layout->addWidget(status_);
	action_ = new QPushButton;
	layout->addWidget(action_);
	auto *retry = new QPushButton(QString::fromUtf8("重新檢查／恢復補傳"));
	layout->addWidget(retry);
	auto *console = new QPushButton(QString::fromUtf8("開啟 Console"));
	layout->addWidget(console);
	legacyButton_ = new QPushButton(QString::fromUtf8("處理舊版未完成收錄"));
	layout->addWidget(legacyButton_);
	connect(console, &QPushButton::clicked, this,
		[] { QDesktopServices::openUrl(QUrl("https://admin.alive.org.tw/content/broadcasts")); });
	connect(save_, &QPushButton::clicked, this, [this] {
		try {
			KeyIngest(root_ + "/intents", "settings").saveKey(key_->text().toUtf8());
			key_->clear();
			keyAvailable_ = true;
			paused_ = false;
			issue_ = QString::fromUtf8("串流金鑰已儲存至 Windows Credential Manager。");
		} catch (...) {
			issue_ = QString::fromUtf8("無法儲存金鑰；請確認格式及 Credential Manager。");
		}
		display();
	});
	connect(action_, &QPushButton::clicked, this, [this] { startStop(); });
	connect(retry, &QPushButton::clicked, this, [this] {
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
				track_->setCurrentIndex(qBound(0, a["audioTrack"].toInt(1) - 1, 5));
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
	action_->setText(running || starting_ ? QString::fromUtf8("停止 HHC 傳送")
					      : QString::fromUtf8("開始 HHC 傳送"));
	action_->setEnabled(lease_->isLocked() && (running || starting_ || !paused_) &&
			    (id_.isEmpty() || released_ || running || starting_));
	save_->setEnabled(id_.isEmpty() && !syncAwaiting_);
	key_->setEnabled(save_->isEnabled());
	track_->setEnabled(id_.isEmpty() || released_);
	legacyButton_->setEnabled(id_.isEmpty());
	titleLabel_->setText(title_.isEmpty() ? QString::fromUtf8("等待 Console 已準備的直播") : title_);
	status_->setText(issue_);
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
		display();
		return;
	}
	legacy_.reset();
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
					    {"audioTrack", track_->currentData().toInt()}});
	issue_ = QString::fromUtf8("等待 Console 準備直播…");
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
								 {"audioTrack", track_->currentData().toInt()},
								 {"fpsNum", 30000},
								 {"fpsDen", 1001}});
			atomicJson(root_ + "/active.json", {{"localId", id_},
							    {"released", false},
							    {"started", true},
							    {"audioTrack", track_->currentData().toInt()}});
			capture_ = std::make_unique<CaptureOutput>();
			if (!capture_->start(
				    {dir, unsigned(track_->currentData().toInt()), root_ + "/queue", actor_, id_})) {
				capture_.reset();
				issue_ = QString::fromUtf8("NVENC 啟動失敗，保留資料並中止原 capture。");
			}
		}
		if (r.end)
			end_ = r.end;
		released_ = r.sync.stopAccepted && r.sync.sealAccepted;
		if (released_)
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
		issue_ = QString::fromUtf8("HHC：%1；待傳 %2 bytes；自動發布：%3")
				 .arg(r.sync.state)
				 .arg(r.sync.pendingBytes)
				 .arg(r.sync.autoPublish);
		if (r.sync.state == "aborted" || r.sync.state == "expired" || r.sync.state == "failed") {
			if (capture_ && !capture_->finished())
				capture_->stop(StopReason::EncoderFailure);
			released_ = true;
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
			paused_ = false;
			next_ = std::numeric_limits<qint64>::max();
		} else
			next_ = QDateTime::currentMSecsSinceEpoch() + 3000;
		if (r.sync.state == "ready" && r.sync.autoPublish != "pending")
			next_ = std::numeric_limits<qint64>::max();
	} catch (const RequestError &e) {
		issue_ = e.code;
		if (e.code == "broadcast_ingest_revoked") {
			revoked_ = true;
			if (capture_)
				capture_->stop(StopReason::EncoderFailure);
			next_ = 0;
		} else if (e.code == "broadcast_source_not_prepared")
			next_ = QDateTime::currentMSecsSinceEpoch() + 5000 + QRandomGenerator::global()->bounded(1000);
		else {
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
