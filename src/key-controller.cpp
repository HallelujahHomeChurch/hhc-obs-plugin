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
#include <QCryptographicHash>
#include <limits>
#include <algorithm>
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
		failures_ = controlFailures_ = 0;
		tick();
	});
	connect(&sync_, &QFutureWatcher<KeyJob>::finished, this, [this] { syncDone(); });
	connect(&control_, &QFutureWatcher<KeyJob>::finished, this, [this] {
		controlAwaiting_ = false;
		if (closing_ || terminalRetired_ || controlGeneration_ != generation_.load()) {
			if (!closing_) display();
			return;
		}
		try {
			auto r = workerResult(control_.future());
			if (!r.id.isEmpty() && r.id != id_) return;
			recoveryRequested_ = r.recoveryRequested;
			recoveryControl_ = r.recoveryControl;
			if (!recoveryControl_.isEmpty()) { end_.reset(); serverEnd_.reset(); }
			controlFailures_ = 0;
			end_ = r.end;
			if(r.serverEnd) serverEnd_=r.serverEnd;
			nextControl_ = QDateTime::currentMSecsSinceEpoch() + 2000;
		} catch (const RequestError &e) {
			if (preserveRecoveryLocal(e)) { recordFailure("control", &e); display(); return; }
			if (e.outputReleased && e.code == "output_replay_terminal") {
				recordFailure("control", &e);
				retireOutput();
				display();
				return;
			}
			if (e.outputReplayUnknown) {
				paused_ = true;
				issue_ = e.code;
				nextControl_ = std::numeric_limits<qint64>::max();
				recordFailure("control", &e);
				display();
				return;
			}
			if (e.code == "broadcast_ingest_revoked") {
				revoked_ = true;
				markIngestRevoked(root_, id_);
				if (capture_) {
					recordStopCause("ingest_revoked");
					capture_->stop(StopReason::EncoderFailure);
				}
			}
			issue_ = e.code;
			const auto delay = e.retryDelay(controlFailures_);
			nextControl_ = QDateTime::currentMSecsSinceEpoch() + qint64((std::max)(5, delay.value_or(5))) * 1000;
			recordFailure("control", &e, !paused_ && !revoked_);
		} catch (const std::exception &e) {
			issue_ = "control_recovery_required";
			paused_ = true;
			recordFailure("control", &e);
		} catch (...) {
			issue_ = "control_recovery_required";
			paused_ = true;
			recordFailure("control");
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
				recoveryRequested_ = KeyIngest(root_ + "/intents", id_).requestsRecovery();
				networkBlocked_ = recoveryRequested_ && QFileInfo::exists(root_ + "/intents/" + id_ + ".network-blocked");
				stopping_ = a["stopping"].toBool() ||
					    QFileInfo::exists(root_ + "/intents/" + id_ + ".stopping");
				starting_ = !a["started"].toBool() && !stopping_;
				audioTrack_ = unsigned(qBound(1, a["audioTrack"].toInt(1), 6));
			} else {
				// The input can be released before ready/local cleanup; resume off the UI thread.
				id_ = a["localId"].toString();
				if (hasReleaseProof()) {
					launchHelper();
					id_.clear();
				} else {
					paused_ = stopping_ = true;
					issue_ = QString::fromUtf8("尚無原輸出的釋放證據；保留收錄與素材，請重試。");
				}
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
	const auto recovery = recoveryControl_["recovery"].toObject();
	const bool localOnly = running && (networkBlocked_ || recoveryControl_["publicEnd"].isObject());
	const auto state = localOnly ? QString::fromUtf8("本機保存中") : paused_ ? QString::fromUtf8("需要處理") :
		running && recovery["state"] == "interrupted" ? QString::fromUtf8("等待恢復") :
		failures_ ? QString::fromUtf8("重連中") : starting_ ? QString::fromUtf8("連線中") : running ? QString::fromUtf8("傳送中") : finalizing ? (pendingBytes_ ? QString::fromUtf8("補傳中") : QString::fromUtf8("收尾中")) : QString::fromUtf8("未連線");
	QString text = QString::fromUtf8("HHC：%1").arg(state);
	// Routine segment uploads fluctuate; show sustained pending work without calling it a network failure.
	if (pendingBytes_ && (paused_ || failures_ || finalizing || now - pendingSince_ >= 60000))
		text += QString::fromUtf8(" · 待傳 %1 MB").arg(double(pendingBytes_) / 1048576, 0, 'f', 1);
	status_->setText(text + QString::fromUtf8("  |  GPU 編碼：%1").arg(gpuPercent_ >= 0 ? QString::number(gpuPercent_) + "%" : QString::fromUtf8("—")));
	status_->setToolTip(title_ + "\n" + issue_ + QString::fromUtf8("\n上傳：%1 Mbps（最近 30 秒成功傳送的媒體平均值）\n待傳：%2 MB（尚未確認的媒體，正常分段傳送也會短暫增加）\n待傳持續 60 秒未清空、重連或補傳時顯示於狀態列；不代表會員播放狀態。\nGPU：整張 NVIDIA 顯示卡的 Video Encode 使用率，包含本機錄影與其他程式；多卡或讀取失敗顯示 —。")
		.arg(mbps, 0, 'f', 1).arg(double(pendingBytes_) / 1048576, 0, 'f', 1));
	if (!recoveryControl_.isEmpty() && recovery["version"].isNull())
		status_->setToolTip(status_->toolTip() + QString::fromUtf8("\n本場未啟用五分鐘恢復能力；不依本機時間推算期限。"));
	if (localOnly) status_->setToolTip(status_->toolTip() + QString::fromUtf8("\n公開播放已結束或網路授權需處理；HHC 本機擷取仍在運作，OBS 原生錄影狀態請看錄影按鈕。"));
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
	if (QFileInfo::exists(root_ + "/active.json")) {
		const auto active = read(root_ + "/active.json");
		if (active["protocolVersion"] == "k4-2026-10-15.rc1" || active["released"].toBool())
			return false;
	}
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
		++generation_;
		stopping_ = true;
		recordStopCause("user_stop");
		capture_->stop(StopReason::User);
		recordWithStream(false);
		display();
		return;
	}
	if (starting_) {
		++generation_;
		recordStopCause("user_cancel_opening");
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
	if (!id_.isEmpty()) {
		try {
			if (!hasReleaseProof()) throw RequestError(409, "ingest_release_unproven");
		} catch (...) {
			paused_ = true;
			released_ = false;
			issue_ = QString::fromUtf8("尚無原輸出的釋放證據；保留收錄與素材，請重試。");
			display();
			return;
		}
		launchHelper();
	}
	capture_.reset();
	++generation_;
	id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
	try {
		KeyIngest(root_ + "/intents", id_).prepareOpen("obs-hls-10s-v1", "common-watermark-v1");
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
	recoveryRequested_ = true;
	networkBlocked_ = false;
	cancelled_ = false;
	recoveryControl_ = {};
	title_.clear();
	recoveryValidated_ = false;
	starting_ = true;
	controlFailures_ = 0;
	terminalRetired_ = false;
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
	if (networkBlocked_) {
		// No protected request is needed to honor an already persisted release proof after local stop.
		if ((!capture_ || capture_->finished()) && !released_) {
			try {
				if (hasReleaseProof()) {
					atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
					released_ = true; stopping_ = starting_ = paused_ = false;
				}
			} catch (...) { paused_ = true; }
		}
		return;
	}
	if (closing_ || paused_ || terminalRetired_ || !lease_->isLocked() || id_.isEmpty())
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
		if (stopping_) recordStopCause("server_end", serverEnd_);
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
			recordStopCause("console_end", end_);
			capture_->stop(StopReason::User);
		}
	}
	pollControl();
	if (syncAwaiting_ || QDateTime::currentMSecsSinceEpoch() < next_)
		return;
	const auto id = id_;
	const bool active = capture_ && !capture_->finished(), revoked = revoked_, preparing = starting_;
	syncAwaiting_ = true;
	syncGeneration_ = generation_.load();
	const auto generation = syncGeneration_;
	sync_.setFuture(QtConcurrent::run([this, id, active, revoked, preparing, generation] {
		HttpCancellationScope cancellation(cancelled_);
		return keyTransferStep(root_, id, active, revoked, preparing, recoveryValidated_, &cancelled_, {}, httpRequest,
			[this](quint64 n) { uploadedBytes_.fetch_add(n); }, [this, generation] { return generation_.load() == generation; });
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
	controlGeneration_ = generation_.load();
	const auto generation = controlGeneration_;
	control_.setFuture(QtConcurrent::run([this, id, actor, boundary, generation] {
		HttpCancellationScope cancellation(cancelled_);
		KeyIngest ingest(root_ + "/intents", id);
		auto api = ingest.api();
		BroadcastControl control(SessionStore(root_ + "/queue").mediaDirectory(actor, id), actor, id, api,
					 true);
		try { control.poll(boundary, true, [this, generation] { return generation_.load() == generation; }); }
		catch (const RequestError &e) { ingest.blockNetwork(e); throw; }
		KeyJob out;
		out.id = id;
		out.recoveryRequested = ingest.requestsRecovery();
		out.recoveryControl = control.recoveryControl();
		out.end = control.acceptedEnd();
		out.serverEnd = control.serverEnd();
		return out;
	}));
}
bool KeyController::hasReleaseProof() const
{
	KeyIngest ingest(root_ + "/intents", id_);
	const auto metadata = ingest.metadata();
	if (metadata["version"] == "k3-2026-10-14.rc1") return true;
	if (metadata["version"] != "k4-2026-10-15.rc1") return false;
	// KeyIngest validates a saved terminal proof against all original pins on load.
	if (metadata.contains("terminalProof")) return true;
	const auto binding = metadata["sessionBinding"].toObject();
	const auto actor = metadata["broadcast"].toObject()["binding"].toObject()["actorId"].toString();
	if (actor.isEmpty() || binding.isEmpty()) return false;
	auto api = ingest.api();
	CaptureSync saved(root_ + "/queue", actor, id_, api);
	return saved.hasReleaseReceipt(binding["recordingId"].toString(), binding["captureId"].toString());
}
void KeyController::retireOutput()
{
	next_ = std::numeric_limits<qint64>::max();
	try {
		atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
	} catch (...) {
		paused_ = true;
		released_ = false;
		issue_ = QString::fromUtf8("無法保存輸出釋放紀錄；原輸出與素材保留，請重試。");
		return;
	}
	if (capture_ && !capture_->finished()) {
		recordStopCause("output_released");
		capture_->stop(StopReason::EncoderFailure);
	}
	starting_ = stopping_ = paused_ = false;
	released_ = terminalRetired_ = true;
	issue_ = QString::fromUtf8("本次輸出已結束；再次按開始直播才會建立下一次輸出。");
}
void KeyController::syncDone()
{
	syncAwaiting_ = false;
	// Control can persist release before an already-running upload reports its old result.
	if (closing_ || terminalRetired_ || syncGeneration_ != generation_.load()) {
		if (!closing_) display();
		return;
	}
	try {
		const auto r = workerResult(sync_.future());
		if (!r.id.isEmpty() && r.id != id_) return;
		recoveryRequested_ = r.recoveryRequested;
		recoveryControl_ = r.recoveryControl;
		if (!recoveryControl_.isEmpty()) { end_.reset(); serverEnd_.reset(); }
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
			if (preserveRecoveryLocal(RequestError(409, "capture_terminal"))) { display(); return; }
			starting_ = false;
			if (capture_ && !capture_->finished()) {
				recordStopCause("capture_terminal");
				capture_->stop(StopReason::EncoderFailure);
			}
			released_ = true;
			atomicJson(root_ + "/active.json", {{"localId", id_}, {"released", true}});
			paused_ = false;
			next_ = std::numeric_limits<qint64>::max();
		} else
			next_ = QDateTime::currentMSecsSinceEpoch() + (r.sync.pendingBytes ? 0 : 3000);
		if (r.sync.state == "ready" && !r.sync.localCleanupComplete)
			issue_ = QString::fromUtf8("收錄完成；本機暫存清理將自動重試。");
		if (r.sync.state == "ready" && r.sync.autoPublish != "pending" && r.sync.localCleanupComplete)
			next_ = std::numeric_limits<qint64>::max();
	} catch (const RequestError &e) {
		issue_ = e.code;
		if (preserveRecoveryLocal(e)) { recordFailure("sync", &e); display(); return; }
		if (e.outputOpening && recoveryRequested_ && e.status == 400) {
			paused_ = true;
			issue_ = QString::fromUtf8("平台拒絕 D2 開始請求；請確認伺服器支援或輸入，原開始紀錄保留。");
			recordFailure("sync", &e); display(); return;
		}
		if (e.outputReplayUnknown) {
			paused_ = true;
			next_ = std::numeric_limits<qint64>::max();
		} else if (e.code == "broadcast_ingest_revoked") {
			revoked_ = true;
			if (capture_) {
				recordStopCause("ingest_revoked");
				capture_->stop(StopReason::EncoderFailure);
			}
			next_ = 0;
		} else if (e.outputReleased && e.code == "output_replay_terminal") {
			retireOutput();
		} else if (!e.outputOpening && e.code == "broadcast_source_not_prepared") {
			issue_ = QString::fromUtf8("平台尚未準備接收，正在自動重試。");
			next_ = QDateTime::currentMSecsSinceEpoch() + 5000 + QRandomGenerator::global()->bounded(1000);
		} else if (e.code == "local_session_busy") {
			// Upload adoption and control polling share a journal lock. Yield without
			// treating local ownership contention as a failed network connection.
			next_ = QDateTime::currentMSecsSinceEpoch() + 250;
		} else {
			if (!failures_) blog(LOG_WARNING, "[HHC] Transfer retry: status=%d code=%s", e.status, qPrintable(safeDiagnosticCode(e.code)));
			auto delay = e.retryDelayMs(failures_);
			paused_ = !delay;
			next_ = QDateTime::currentMSecsSinceEpoch() + delay.value_or(0);
		}
		recordFailure("sync", &e, !paused_ && !released_ && !revoked_);
	} catch (const std::exception &e) {
		paused_ = true;
		issue_ = QString::fromUtf8("恢復資料檢查未完成；原 intent 與素材保留。");
		recordFailure("sync", &e);
	} catch (...) {
		paused_ = true;
		issue_ = QString::fromUtf8("恢復資料檢查未完成；原 intent 與素材保留。");
		recordFailure("sync");
	}
	if (stopping_ || (paused_ && issue_ == "broadcast_stream_key_invalid"))
		retireUnbound();
	display();
}
bool KeyController::preserveRecoveryLocal(const RequestError &error) noexcept
{
	if (!recoveryRequested_ || !(error.status == 401 || error.status == 403 || error.status == 404 ||
	    error.code == "broadcast_epoch_mismatch" || error.code == "malformed_success" ||
	    error.code == "capture_terminal" || error.outputReleased || error.outputReplayUnknown)) return false;
	networkBlocked_ = paused_ = true;
	cancelled_ = true;
	++generation_;
	next_ = nextControl_ = std::numeric_limits<qint64>::max();
	issue_ = QString::fromUtf8("網路授權／原場狀態需確認；已停止網路操作，本機素材保留。");
	try { KeyIngest(root_ + "/intents", id_).blockNetwork(error); } catch (...) {}
	return true;
}
void KeyController::recordStopCause(const char *cause, std::optional<int> cutoff) noexcept
{
	try {
		const QString value = QString::fromLatin1(cause);
		const QString safe = QStringList{"user_stop", "user_cancel_opening", "console_end", "server_end",
			"ingest_revoked", "output_released", "capture_terminal", "obs_shutdown"}.contains(value) ? value : "unknown";
		const auto observedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
		const auto output = QString::fromLatin1(QCryptographicHash::hash(id_.toUtf8(), QCryptographicHash::Sha256).toHex().left(16));
		QDir().mkpath(root_ + "/diagnostics/native");
		tryObserverJson(root_ + "/diagnostics/native/stop-cause.json",
			{{"observedAt", observedAt}, {"output", output}, {"cause", safe}, {"event", "stop_requested"},
			 {"publicEndSequenceExclusive", cutoff ? QJsonValue(*cutoff) : QJsonValue(QJsonValue::Null)}});
		diagnostic(QString("[HHC stop] utc=%1 output=%2 cause=%3 event=stop_requested cutoff=%4")
			.arg(observedAt, output, safe).arg(cutoff.value_or(-1)));
	} catch (...) {
		// Optional evidence must never change the requested stop or authority fence.
	}
}
void KeyController::recordFailure(const char *stage, const std::exception *error, bool retryable) noexcept
{
	try {
		const auto *request = dynamic_cast<const RequestError *>(error);
		// Only exact internal literals may select a code; never persist arbitrary what() text.
		const auto reason = error && !request ? QByteArray(error->what()) : QByteArray{};
		const QString code = request ? safeDiagnosticCode(request->code)
			: reason == "Remote journal commit failed" ? "local_remote_journal_write_failed"
			: reason == "Queue document unavailable" ? "local_queue_unavailable"
			: reason == "Queue document corrupt; data retained" ? "local_queue_corrupt"
			: reason == "Remote journal limit" ? "local_remote_journal_limit"
			: reason == "Closed upload bytes changed" ? "local_media_changed"
			: reason == "Remote immutable object differs" ? "local_remote_media_conflict"
			: reason == "journal commit failed" ? "local_journal_write_failed"
			: "local_validation_failed";
		const auto correlation = safeDiagnosticRequestId(request ? request->requestId : QString{});
		const auto localId = QUuid(id_).isNull() ? QString("unavailable") : id_;
		const auto observedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
		const QString category = request ? "request" : error ? "std_exception" : "unknown_exception";
		QDir().mkpath(root_ + "/diagnostics/native");
		tryObserverJson(root_ + "/diagnostics/native/" + stage + "-error.json",
			{{"stage", stage}, {"localId", localId}, {"observedAt", observedAt}, {"code", code},
			 {"category", category}, {"status", request ? QJsonValue(request->status) : QJsonValue(QJsonValue::Null)},
			 {"requestId", correlation}, {"paused", paused_}, {"retryable", retryable},
			 {"pauseReason", paused_ ? QJsonValue(request && request->outputReplayUnknown ? "output_authority_unknown" : code) : QJsonValue(QJsonValue::Null)},
			 {"outputReplayUnknown", request && request->outputReplayUnknown},
			 {"outputReleasedProof", request && request->outputReleased},
			 {"stopping", stopping_}, {"released", released_}});
		diagnostic(QString("[HHC controller] utc=%1 stage=%2 category=%3 code=%4 status=%5 requestId=%6 paused=%7 retryable=%8 localId=%9")
			.arg(observedAt, stage, category, code).arg(request ? request->status : 0)
			.arg(correlation).arg(paused_).arg(retryable).arg(localId));
	} catch (...) {
		// Optional diagnostics must not alter the original pause or authority fence.
	}
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
	++generation_;
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
		if (!capture_->finished()) recordStopCause("obs_shutdown");
		capture_->stop(StopReason::User);
		capture_->wait(6000);
		capture_.reset();
	}
	// Each future can throw; one failure must not skip joining the other workers.
	try { gpu_.waitForFinished(); } catch (...) {}
	try { sync_.waitForFinished(); } catch (...) {}
	try { control_.waitForFinished(); } catch (...) {}
	if (!networkBlocked_) launchHelper();
}
} // namespace hhc
