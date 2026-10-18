#include "runtime-paths.hpp"
#include "gpu-encode.hpp"
#include "key-controller.hpp"
#include "ingest.hpp"
#include "session-store.hpp"
#include "broadcast-control.hpp"
#include "async.hpp"
#include "hls-muxer.hpp"
#include "windows-path.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QMessageBox>
#include <QHBoxLayout>
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
#include <QApplication>
#include <QRegularExpression>
#include <QUuid>
#include <windows.h>
namespace hhc {
namespace {
const char *optionKeys[] = {"WarnBeforeStartingStream", "WarnBeforeStoppingStream", "RecordWhenStreaming", "KeepRecordingWhenStreamStops"};
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
	  view_(new QWidget),
	  audioTrackProvider_(std::move(audioTrack))
{
	auto *layout = new QVBoxLayout(view_);
	layout->setAlignment(Qt::AlignTop);
	settings_ = new QWidget(view_);
	auto *settingsLayout = new QVBoxLayout(settings_);
	key_ = new QLineEdit;
	key_->setEchoMode(QLineEdit::Password);
	key_->setPlaceholderText(QString::fromUtf8("串流金鑰"));
	auto *keyLabel = new QLabel(QString::fromUtf8("串流金鑰"));
	keyLabel->setBuddy(key_);
	auto *form = new QFormLayout;
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
	auto *keyRow = new QHBoxLayout;
	showKey_ = new QPushButton(QString::fromUtf8("顯示"));
	auto *getKey = new QPushButton(QString::fromUtf8("取得串流金鑰"));
	getKey->setToolTip(QString::fromUtf8("開啟 HHC 後台"));
	keyRow->addWidget(key_, 1);
	keyRow->addWidget(showKey_);
	keyRow->addWidget(getKey);
	form->addRow(keyLabel, keyRow);
	settingsLayout->addLayout(form);
	connect(showKey_, &QPushButton::clicked, this, [this] {
		const bool show = key_->echoMode() == QLineEdit::Password;
		key_->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
		showKey_->setText(QString::fromUtf8(show ? "隱藏" : "顯示"));
	});
	connect(getKey, &QPushButton::clicked, this, [] {
		QDesktopServices::openUrl(QUrl("https://admin.alive.org.tw/content/studio/direct"));
	});
	settingsMessage_ = new QLabel(QString::fromUtf8("金鑰儲存於 Windows Credential Manager。"));
	settingsMessage_->setWordWrap(true);
	settingsLayout->addWidget(settingsMessage_);
	auto *output = new QGroupBox(QString::fromUtf8("輸出"));
	auto *outputLayout = new QFormLayout(output);
	outputLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	outputLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
	auto margins = outputLayout->contentsMargins();
	outputLayout->setContentsMargins(margins.left(), 2, margins.right(), margins.bottom());
	outputLayout->setItem(0, QFormLayout::LabelRole, new QSpacerItem(170, 5, QSizePolicy::Fixed, QSizePolicy::Fixed));
	const char *labels[] = {"啟動 HHC 串流時顯示確認對話方塊", "停止 HHC 串流時顯示確認對話方塊", "HHC 串流時自動錄製", "HHC 串流停止時繼續錄製"};
	QSettings preferences(root_ + "/output.ini", QSettings::IniFormat);
	for (size_t i = 0; i < outputOptions_.size(); ++i) {
		savedOptions_[i] = preferences.value(optionKeys[i], false).toBool();
		outputOptions_[i] = new QCheckBox(QString::fromUtf8(labels[i]));
		outputOptions_[i]->setObjectName(QString::fromLatin1(optionKeys[i]));
		outputOptions_[i]->setChecked(savedOptions_[i]);
		outputLayout->setWidget(int(i), QFormLayout::FieldRole, outputOptions_[i]);
		connect(outputOptions_[i], &QCheckBox::toggled, this, [this] {
			outputOptions_[3]->setEnabled(outputOptions_[2]->isChecked());
			if (settingsEdited) settingsEdited();
		});
	}
	outputOptions_[3]->setEnabled(savedOptions_[2]);
	settingsLayout->addWidget(output);
	settingsLayout->addStretch();
	status_ = new QLabel;
	status_->setWordWrap(true);
	status_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	status_->setTextFormat(Qt::PlainText);
	layout->addWidget(status_);
	action_ = new QPushButton;
	action_->setObjectName("hhcStreamButton");
	layout->addWidget(action_);
	retry_ = new QPushButton(QString::fromUtf8("重試"));
	layout->addWidget(retry_);
	settings_->hide();
	status_->setWordWrap(false);
	status_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	rateClock_.start();
	connect(action_, &QPushButton::clicked, this, [this] { startStop(); });
	connect(retry_, &QPushButton::clicked, this, [this] {
		paused_ = false;
		next_ = nextControl_ = 0;
		failures_ = 0;
		tick();
	});
	connect(&sync_, &QFutureWatcher<KeyJob>::finished, this, [this] { syncDone(); });
	connect(&control_, &QFutureWatcher<KeyJob>::finished, this, [this] {
		controlAwaiting_ = false;
		if (closing_)
			return;
		try {
			auto r = workerResult(control_.future());
			end_ = r.end;
			if(r.serverEnd) serverEnd_=r.serverEnd;
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
				stopping_ = a["stopping"].toBool() ||
					    QFileInfo::exists(root_ + "/intents/" + id_ + ".stopping");
				starting_ = !a["started"].toBool() && !stopping_;
				audioTrack_ = unsigned(qBound(1, a["audioTrack"].toInt(1), 6));
			}
		} catch (...) {
			paused_ = true;
			issue_ = QString::fromUtf8("收錄 journal 無法讀取；資料保留，請檢查。");
		}
	}
	connect(&gpu_, &QFutureWatcher<int>::finished, this, [this] {
		try { gpuPercent_ = gpu_.result(); } catch (...) { gpuPercent_ = -1; }
		if (!closing_) display();
	});
	connect(&timer_, &QTimer::timeout, this, [this] {
		if (!gpu_.isRunning() && rateClock_.elapsed() >= nextGpu_) {
			nextGpu_ = rateClock_.elapsed() + 2000;
			gpu_.setFuture(QtConcurrent::run([] { return sampleEncoderGpu(); }));
		}
		tick(); display();
	});
	timer_.start(500);
	display();
}
KeyController::~KeyController()
{
	shutdown();
	delete view_;
}
void KeyController::setStreamActive(bool active)
{
	// OBS themes scope active colors to #streamButton, not to every QPushButton.
	// Copy only those rules onto our own widget; never duplicate the native object name.
	const auto theme = qApp->styleSheet();
	if (theme != theme_) {
		theme_ = theme;
		QString rules;
		static const QRegularExpression selector("([^{}]*#streamButton[^{}]*)\\{([^{}]*)\\}");
		auto matches = selector.globalMatch(theme);
		while (matches.hasNext()) {
			auto rule = matches.next().captured(0);
			rules += rule.replace("#streamButton", "#hhcStreamButton");
		}
		action_->setStyleSheet(rules);
	}
	const QString activeClass = active ? "state-active" : "";
	if (action_->property("class").toString() != activeClass) {
		action_->setProperty("class", activeClass);
		const auto style = action_->styleSheet();
		action_->setStyleSheet("/* */");
		action_->setStyleSheet(style);
	}
}
void KeyController::display()
{
	const bool running = capture_ && !capture_->finished();
	setStreamActive(running);
	action_->setText(running || starting_ ? QString::fromUtf8("停止 HHC 串流") : QString::fromUtf8("開始 HHC 串流"));
	action_->setEnabled(lease_->isLocked() && (running || starting_ || !paused_) &&
			    (id_.isEmpty() || released_ || running || starting_));
	key_->setEnabled(lease_->isLocked() && (id_.isEmpty() || released_) && !running && !starting_ &&
			  !syncAwaiting_ && !controlAwaiting_);
	key_->setPlaceholderText(keyAvailable_ ? QString::fromUtf8("已設定；可更換金鑰")
					       : QString::fromUtf8("串流金鑰"));
	retry_->setVisible(paused_ && lease_->isLocked());
	const auto now = rateClock_.elapsed();
	rateSamples_.emplace_back(now, uploadedBytes_.load());
	while (rateSamples_.size() > 1 && rateSamples_[1].first <= now - 30000) rateSamples_.pop_front();
	const auto dt = now - rateSamples_.front().first;
	const double mbps = dt > 0 ? double(uploadedBytes_.load() - rateSamples_.front().second) * 0.008 / dt : 0;
	if (!pendingBytes_) pendingSince_ = -1;
	else if (pendingSince_ == -1) pendingSince_ = now;
	const bool finalizing = !running && !starting_ && !id_.isEmpty() && !released_;
	const auto state = paused_ ? QString::fromUtf8("需要處理") : failures_ ? QString::fromUtf8("重連中") : starting_ ? QString::fromUtf8("連線中") : running ? QString::fromUtf8("傳送中") : finalizing ? (pendingBytes_ ? QString::fromUtf8("補傳中") : QString::fromUtf8("收尾中")) : QString::fromUtf8("未連線");
	QString text = QString::fromUtf8("HHC：%1").arg(state);
	// Routine segment uploads fluctuate; show sustained pending work without calling it a network failure.
	if (pendingBytes_ && (paused_ || failures_ || finalizing || now - pendingSince_ >= 60000))
		text += QString::fromUtf8(" · 待傳 %1 MB").arg(double(pendingBytes_) / 1048576, 0, 'f', 1);
	status_->setText(text + QString::fromUtf8("  |  GPU 編碼：%1").arg(gpuPercent_ >= 0 ? QString::number(gpuPercent_) + "%" : QString::fromUtf8("—")));
	status_->setToolTip(title_ + "\n" + issue_ + QString::fromUtf8("\n上傳：%1 Mbps（最近 30 秒成功傳送的媒體平均值）\n待傳：%2 MB（尚未確認的媒體，正常分段傳送也會短暫增加）\n待傳持續 60 秒未清空、重連或補傳時顯示於狀態列；不代表會員播放狀態。\nGPU：整張 NVIDIA 顯示卡的 Video Encode 使用率，包含本機錄影與其他程式；多卡或讀取失敗顯示 —。")
		.arg(mbps, 0, 'f', 1).arg(double(pendingBytes_) / 1048576, 0, 'f', 1));
	status_->setStyleSheet(paused_ ? "color: #e76f51" : "");
}
bool KeyController::validateSettings()
{
	if (!key_->isModified() || key_->text().isEmpty()) return true;
	if (!key_->isEnabled()) {
		settingsMessage_->setText(QString::fromUtf8("收錄或補傳尚未完成，不能更換金鑰。"));
		return false;
	}
	try {
		KeyIngest::validateKey(key_->text().toUtf8());
		return true;
	} catch (...) {
		settingsMessage_->setText(QString::fromUtf8("串流金鑰格式不正確。"));
		return false;
	}
}
bool KeyController::settingsModified() const
{
	if (key_->isModified()) return true;
	for (size_t i = 0; i < outputOptions_.size(); ++i)
		if (outputOptions_[i]->isChecked() != savedOptions_[i]) return true;
	return false;
}
void KeyController::recordWithStream(bool start)
{
	// Match OBS StartStreaming/StopStreaming: the native recorder is shared, not owned by a stream.
	if (recording && savedOptions_[2] && (start || !savedOptions_[3])) recording(start);
}
bool KeyController::applySettings()
{
	if (!validateSettings()) return false;
	QSettings preferences(root_ + "/output.ini", QSettings::IniFormat);
	for (size_t i = 0; i < outputOptions_.size(); ++i)
		preferences.setValue(optionKeys[i], outputOptions_[i]->isChecked());
	preferences.sync();
	if (preferences.status() != QSettings::NoError) {
		settingsMessage_->setText(QString::fromUtf8("無法儲存 HHC 輸出設定。"));
		return false;
	}
	for (size_t i = 0; i < outputOptions_.size(); ++i) savedOptions_[i] = outputOptions_[i]->isChecked();
	if (!key_->isModified() || key_->text().isEmpty()) { key_->setModified(false); return true; }
	try {
		KeyIngest(root_ + "/intents", "settings").saveKey(key_->text().toUtf8());
		key_->setModified(false); keyAvailable_ = true;
		key_->setEchoMode(QLineEdit::Password);
		showKey_->setText(QString::fromUtf8("顯示"));
		settingsMessage_->setText(QString::fromUtf8("金鑰已儲存。"));
		display();
		return true;
	} catch (...) {
		settingsMessage_->setText(QString::fromUtf8("無法儲存金鑰；請確認格式及 Credential Manager。"));
		return false;
	}
}
void KeyController::beginSettings()
{
	discardSettings();
	try {
		KeyIngest ingest(root_ + "/intents", "settings");
		if (ingest.hasKey()) key_->setText(QString::fromLatin1(ingest.key()));
		key_->setModified(false);
	} catch (...) {
		settingsMessage_->setText(QString::fromUtf8("無法讀取已儲存金鑰；請確認 Windows Credential Manager。"));
	}
}
void KeyController::discardSettings()
{
	for (size_t i = 0; i < outputOptions_.size(); ++i) outputOptions_[i]->setChecked(savedOptions_[i]);
	key_->clear();
	key_->setModified(false);
	key_->setEchoMode(QLineEdit::Password);
	showKey_->setText(QString::fromUtf8("顯示"));
	settingsMessage_->setText(keyAvailable_ ? QString::fromUtf8("已儲存串流金鑰；留空則保留原金鑰。")
					      : QString::fromUtf8("尚未設定串流金鑰。金鑰儲存於 Windows Credential Manager。"));
}
bool KeyController::retireUnbound()
{
	if (syncAwaiting_ || id_.isEmpty() || QFileInfo::exists(root_ + "/intents/" + id_ + ".json"))
		return false;
	// Missing K4 metadata is a recovery fault, not evidence the server released an output.
	if (QFileInfo::exists(root_ + "/active.json") &&
	    read(root_ + "/active.json")["protocolVersion"] == "k4-2026-10-15.rc1")
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
	const bool stopping = starting_ || (capture_ && !capture_->finished());
	if (savedOptions_[stopping ? 1 : 0] && view_->isVisible()) {
		if (QMessageBox::question(view_->window(), QString::fromUtf8(stopping ? "停止 HHC 串流" : "開始 HHC 串流"),
		    QString::fromUtf8(stopping ? "確定要停止 HHC 串流嗎？" : "確定要開始 HHC 串流嗎？"),
		    QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
		// Control polling can finish this output while the modal confirmation is open.
		if (stopping != (starting_ || (capture_ && !capture_->finished()))) return;
	}
	if (capture_ && !capture_->finished()) {
		stopping_ = true;
		capture_->stop(StopReason::User);
		recordWithStream(false);
		display();
		return;
	}
	if (starting_) {
		if (QFileInfo::exists(root_ + "/intents/" + id_ + ".json"))
			markIngestStopping(root_, id_);
		starting_ = false;
		stopping_ = true;
		auto state = QFileInfo::exists(root_ + "/active.json") ? read(root_ + "/active.json") : QJsonObject{};
		state["localId"] = id_;
		state["released"] = false;
		state["started"] = false;
		state["stopping"] = true;
		atomicJson(root_ + "/active.json", state);
		retireUnbound();
		recordWithStream(false);
		display();
		return;
	}
	if (syncAwaiting_ || controlAwaiting_ || (!id_.isEmpty() && !released_))
		return;
	if (!keyAvailable_) {
		issue_ = QString::fromUtf8("請先儲存串流金鑰。");
		if (openSettings) openSettings();
		display();
		return;
	}
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
	try {
		KeyIngest(root_ + "/intents", id_).prepareOpen("obs-hls-10s-v1");
		atomicJson(root_ + "/active.json", {{"localId", id_},
						    {"protocolVersion", "k4-2026-10-15.rc1"},
						    {"released", false},
						    {"started", false},
						    {"audioTrack", int(audioTrack_)}});
	} catch (...) {
		id_.clear();
		paused_ = true;
		issue_ = QString::fromUtf8("無法保存開始紀錄；尚未送出連線請求。");
		display();
		return;
	}
	actor_.clear();
	title_.clear();
	recoveryValidated_ = false;
	starting_ = true;
	stopping_ = released_ = revoked_ = paused_ = false;
	end_.reset();
	serverEnd_.reset();
	next_ = 0;
	issue_ = QString::fromUtf8("正在連線…");
	tick();
	display();
}
void KeyController::tick()
{
	if (closing_ || paused_ || !lease_->isLocked() || id_.isEmpty())
		return;
	if (!QFileInfo::exists(root_ + "/intents/" + id_ + ".json") && QFileInfo::exists(root_ + "/active.json") &&
	    read(root_ + "/active.json")["protocolVersion"] == "k4-2026-10-15.rc1") {
		paused_ = true;
		issue_ = QString::fromUtf8("開始紀錄遺失；保留原輸出識別，請檢查本機資料。");
		display();
		return;
	}
	if (stopping_ && retireUnbound()) {
		display();
		return;
	}
	if(capture_ && serverEnd_ && !stopping_) {
		// Public cutoff is independent of retained media. Stop at the next actual encoder grid.
		stopping_=capture_->stopAtCommonBoundary();
	}
	if (capture_ && end_ && !serverEnd_ && !stopping_) {
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
		return keyTransferStep(root_, id, active, revoked, preparing, recoveryValidated_, &cancelled_, {}, httpRequest, [this](quint64 n) { uploadedBytes_.fetch_add(n); });
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
		control.poll(boundary, true);
		KeyJob out;
		out.end = control.acceptedEnd();
		out.serverEnd = control.serverEnd();
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
		pendingBytes_ = r.sync.pendingBytes;
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
			auto state = read(root_ + "/active.json");
			state["localId"] = id_;
			state["released"] = false;
			state["started"] = true;
			state["audioTrack"] = int(audioTrack_);
			atomicJson(root_ + "/active.json", state);
			capture_ = std::make_unique<CaptureOutput>();
			if (!capture_->start({dir, audioTrack_, root_ + "/queue", actor_, id_, r.mediaProfile})) {
				capture_.reset();
				issue_ = QString::fromUtf8("NVENC 啟動失敗，保留資料並中止原 capture。");
			} else recordWithStream(true);
		}
		if (r.end)
			end_ = r.end;
		if(r.serverEnd) serverEnd_=r.serverEnd;
		released_ = r.sync.stopAccepted && r.sync.sealAccepted;
		if (released_) {
			starting_ = false;
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
		}
		// Bound publication belongs to Console; C1's cancellation is not the broadcast policy.
		issue_ = capture_ && !capture_->finished() ? QString::fromUtf8("傳送中")
			 : r.sync.state == "ready"         ? QString::fromUtf8("收錄完成")
							   : QString::fromUtf8("收尾中（%1）").arg(r.sync.state);
		if (r.sync.pendingBytes)
			issue_ +=
				QString::fromUtf8("；待傳 %1 MB").arg(double(r.sync.pendingBytes) / 1048576, 0, 'f', 1);
		if (r.sync.state == "aborted" || r.sync.state == "expired" || r.sync.state == "failed") {
			starting_ = false;
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
		} else if (e.outputOpening && e.code == "broadcast_output_terminal") {
			starting_ = stopping_ = paused_ = false;
			released_ = true;
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
			next_ = std::numeric_limits<qint64>::max();
			issue_ = QString::fromUtf8("本次輸出已結束；再次按開始直播才會建立下一次輸出。");
		} else if (!e.outputOpening && e.code == "broadcast_source_not_prepared") {
			issue_ = QString::fromUtf8("平台尚未準備接收，正在自動重試。");
			next_ = QDateTime::currentMSecsSinceEpoch() + 5000 + QRandomGenerator::global()->bounded(1000);
		} else if (e.code == "local_session_busy") {
			// Upload adoption and control polling share a journal lock. Yield without
			// treating local ownership contention as a failed network connection.
			next_ = QDateTime::currentMSecsSinceEpoch() + 250;
		} else {
			if (!failures_) blog(LOG_WARNING, "[HHC] Transfer retry: status=%d code=%s", e.status, qPrintable(e.code));
			auto delay = e.retryDelayMs(failures_);
			paused_ = !delay;
			next_ = QDateTime::currentMSecsSinceEpoch() + delay.value_or(0);
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
	const auto program = uploadHelperPath(currentModuleFile());
	if (program.isEmpty()) {
		blog(LOG_ERROR, "[HHC] Background helper missing; journal retained for next OBS start");
		return;
	}
	QProcess p;
	p.setProgram(program);
	p.setArguments({"--root", root_, "--k3", id_});
	auto env = QProcessEnvironment::systemEnvironment();
	env.insert("PATH", QCoreApplication::applicationDirPath() + ";" + env.value("PATH"));
	p.setProcessEnvironment(env);
	p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) {
		a->flags |= CREATE_NO_WINDOW;
		a->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
		a->startupInfo->wShowWindow = SW_HIDE;
	});
	if (!p.startDetached())
		blog(LOG_ERROR, "[HHC] Background helper could not start; journal retained for next OBS start");
}
void KeyController::shutdown()
{
	if (closing_)
		return;
	closing_ = true;
	timer_.stop();
	if (starting_ && QFileInfo::exists(root_ + "/active.json")) {
		try {
			if (QFileInfo::exists(root_ + "/intents/" + id_ + ".json"))
				markIngestStopping(root_, id_);
			auto state = read(root_ + "/active.json");
			state["stopping"] = true;
			atomicJson(root_ + "/active.json", state);
		} catch (...) {
			blog(LOG_ERROR, "[HHC] Cannot save stopping intent; original output retained");
		}
	}
	cancelled_ = true;
	if (capture_) {
		capture_->stop(StopReason::User);
		capture_->wait(6000);
		capture_.reset();
	}
	try {
		gpu_.waitForFinished();
		sync_.waitForFinished();
		control_.waitForFinished();
	} catch (...) {
	}
	launchHelper();
}
} // namespace hhc
