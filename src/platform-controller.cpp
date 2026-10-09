#include "async.hpp"
#include "platform-controller.hpp"
#include "hls-muxer.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QMessageBox>
#include <QEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDesktopServices>
#include <QUuid>
#include <windows.h>
#include <QRandomGenerator>
#include <QScopeGuard>
namespace hhc {
PlatformController::PlatformController(QString root, QObject *parent)
	: QObject(parent),
	  root_(std::move(root)),
	  auth_(root_, this),
	  view_(new Dock)
{
	state_.autoPublish = false;
	state_.phase = Phase::Unavailable;
	view_->apply(state_);
	view_->setWindowTitle(QString::fromUtf8("HHC 影音"));
	view_->onAction = [this] {
		action();
	};
	view_->onRefresh = [this] {
		refresh();
	};
	view_->onRecover = [this](QString id) {
		recover(id);
	};
	view_->onLogin = [this] {
		if (closing_ || authenticating_ || busy() || jobBusy())
			return;
		const bool previouslyPaused = paused_;
		paused_ = true;
		authenticating_ = true;
		state_.checkingLocal = true;
		try {
			auth_.login();
			state_.issue = QString::fromUtf8("請在系統瀏覽器完成登入。 ");
		} catch (...) {
			authenticating_ = false;
			state_.checkingLocal = false;
			paused_ = previouslyPaused;
			state_.issue = QString::fromUtf8("無法啟動登入，請重試。 ");
		}
		view_->apply(state_);
	};
	view_->onLogout = [this] {
		if (closing_ || authenticating_ || busy() || jobBusy())
			return;
		try {
			auth_.logout();
		} catch (...) {
			state_.issue = QString::fromUtf8("登出尚未完成，請稍後重試。本機收錄資料保留。");
			view_->apply(state_);
		}
	};
	view_->onOpenFolder = [this] {
		QDir().mkpath(root_ + "/queue");
		QDesktopServices::openUrl(QUrl::fromLocalFile(root_ + "/queue"));
	};
	view_->onOpenCleanup = [this](QString id) {
		if (closing_ || authenticating_ || busy() || jobBusy() || auth_.account().isEmpty())
			return;
		SessionStore store(root_ + "/queue");
		for (const auto &j : recoveryCache_.sessions) {
			if (j.localId != id || j.account != auth_.account() ||
			    !store.mayCleanup(j, QDateTime::currentDateTimeUtc()))
				continue;
			const QFileInfo dir(store.mediaDirectory(j.account, j.localId));
			if (!QFileInfo(root_ + "/queue").isSymLink() && !dir.isSymLink() && dir.isDir() &&
			    !QFileInfo(dir.absolutePath()).isSymLink() &&
			    dir.canonicalFilePath().startsWith(QFileInfo(root_ + "/queue").canonicalFilePath() + "/",
							       Qt::CaseInsensitive))
				QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absoluteFilePath()));
			return;
		}
	};
	view_->onCloseLive = [this] {
		control(true);
	};
	view_->onCancelPublish = [this] {
		control(false);
	};
	auth_.onChanged = [this](QString error) {
		if (closing_ || !view_)
			return;
		authenticating_ = false;
		state_.checkingLocal = false;
		if (state_.account != auth_.account()) {
			capture_.reset();
			id_.clear();
			owner_.clear();
			title_.clear();
			recoveryCache_ = {};
			publish_ = live_ = creating_ = stopping_ = false;
			paused_ = true;
			failures_ = 0;
			nextAttempt_ = {};
			state_ = {};
			state_.autoPublish = false;
		}
		state_.account = auth_.account();
		state_.connected = !state_.account.isEmpty();
		state_.canPublish = auth_.permitted("cms:recordings:publish");
		state_.issue = error.isEmpty() ? QString{} : QString::fromUtf8("登入／refresh 未完成：") + error;
		if (!busy() && id_.isEmpty())
			state_.phase = state_.connected && auth_.permitted("cms:recordings:write") ? Phase::Ready
												   : Phase::Unavailable;
		try {
			if (state_.connected)
				atomicJson(
					root_ + "/oauth-evidence.json",
					{{"humanPrincipal", state_.account},
					 {"read", auth_.permitted("cms:recordings:read")},
					 {"write", auth_.permitted("cms:recordings:write")},
					 {"publish", state_.canPublish},
					 {"observedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
		} catch (...) {
			state_.issue = QString::fromUtf8("登入狀態已取得，但本機證據檔保存失敗。");
		}
		view_->apply(state_);
		if (state_.connected)
			refresh();
	};
	if (parent)
		parent->installEventFilter(this);
	timer_.setInterval(200);
	connect(&timer_, &QTimer::timeout, this, [this] { poll(); });
	connect(&job_, &QFutureWatcher<PlatformJob>::finished, this, [this] { completed(); });
	timer_.start();
	auth_.resume();
}
PlatformController::~PlatformController()
{
	shutdown();
	if (view_)
		delete view_.data();
}
bool PlatformController::eventFilter(QObject *, QEvent *event)
{
	if (event->type() == QEvent::Close && busy() && !closing_) {
		auto answer = QMessageBox::question(
			view_, QString::fromUtf8("HHC 正在收錄"),
			QString::fromUtf8("退出 OBS 會正常停止 HHC 收錄，並由背景工作繼續補傳。確定退出？"),
			QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
		if (answer != QMessageBox::Yes) {
			event->ignore();
			return true;
		}
	}
	return false;
}
void PlatformController::shutdown()
{
	if (closing_)
		return;
	closing_ = true;
	cancelled_ = true;
	auth_.cancelPending();
	timer_.stop();
	if (capture_) {
		capture_->stop(StopReason::User);
		capture_->wait(6000);
		capture_.reset();
	}
	try {
		job_.waitForFinished();
	} catch (...) {
	}
	if (!id_.isEmpty())
		helper();
}
void PlatformController::helper()
{
	auto program = QDir(QCoreApplication::applicationDirPath())
			       .absoluteFilePath("../../obs-plugins/64bit/hhc-upload-helper.exe");
	if (!QFileInfo::exists(program))
		return;
	QProcess p;
	p.setProgram(program);
	p.setArguments({"--root", root_, "--account", owner_});
	auto env = QProcessEnvironment::systemEnvironment();
	env.insert("PATH", QCoreApplication::applicationDirPath() + ";" + env.value("PATH"));
	p.setProcessEnvironment(env);
	p.setWorkingDirectory(QCoreApplication::applicationDirPath());
	p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
		args->flags |= CREATE_NO_WINDOW;
		args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
		args->startupInfo->wShowWindow = SW_HIDE;
	});
	p.startDetached();
}
void PlatformController::refresh()
{
	if (closing_ || authenticating_ || jobBusy() || busy() || auth_.account().isEmpty())
		return;
	state_.checkingLocal = true;
	view_->apply(state_);
	auto account = auth_.account();
	launch([this, root = root_, account] {
		PlatformJob result;
		result.scan = true;
		result.recovery = SessionStore(root + "/queue").scanPending(account, &cancelled_);
		return result;
	});
}
void PlatformController::action()
{
	if (closing_ || authenticating_ || !view_)
		return;
	if (capture_ && capture_->finished()) {
		poll();
		return;
	}
	if (busy()) {
		if (state_.phase != Phase::Capturing)
			return;
		stopping_ = true;
		capture_->stop(StopReason::User);
		state_.phase = Phase::StopPending;
		view_->apply(state_);
		nextAttempt_ = {};
		return;
	}
	if (jobBusy() || auth_.account().isEmpty() || !auth_.permitted("cms:recordings:write"))
		return;
	if (state_.phase == Phase::Published || state_.phase == Phase::DraftReady ||
	    (state_.phase == Phase::Failed && state_.terminalFailure)) {
		capture_.reset();
		state_.phase = Phase::Ready;
		state_.terminalFailure = false;
		state_.issue.clear();
		state_.pendingBytes = 0;
		state_.liveEnabled = false;
		state_.liveState.clear();
		state_.autoPublish = false;
		view_->apply(state_);
		return;
	}
	if (state_.phase != Phase::Ready)
		return;
	if (!id_.isEmpty())
		helper();
	capture_.reset();
	id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
	owner_ = auth_.account();
	title_ = view_->title();
	track_ = view_->audioTrack();
	live_ = view_->selectedLive();
	publish_ = view_->selectedPublish();
	if ((live_ || publish_) && !auth_.permitted("cms:recordings:publish")) {
		state_.issue = QString::fromUtf8("目前帳號沒有直播／發布權限。");
		view_->apply(state_);
		return;
	}
	creating_ = true;
	state_.terminalFailure = false;
	stopping_ = false;
	paused_ = false;
	failures_ = 0;
	state_.phase = Phase::Creating;
	state_.issue.clear();
	state_.liveEnabled = live_;
	state_.liveState.clear();
	state_.autoPublish = publish_;
	view_->apply(state_);
	submit();
}
void PlatformController::recover(QString id)
{
	if (closing_ || authenticating_ || busy() || jobBusy() || auth_.account().isEmpty())
		return;
	const auto &report = recoveryCache_;
	auto found = std::find_if(report.sessions.begin(), report.sessions.end(),
				  [&](const auto &j) { return j.localId == id; });
	if (found == report.sessions.end())
		return;
	if (!found->normalEnd &&
	    QMessageBox::question(view_, QString::fromUtf8("中斷的收錄"),
				  QString::fromUtf8("這場未正常完成。同步會回報 abort 並保留本機素材；不會發布。繼續？"),
				  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
		return;
	id_ = id;
	owner_ = auth_.account();
	capture_.reset();
	creating_ = false;
	state_.terminalFailure = false;
	paused_ = false;
	failures_ = 0;
	state_.phase = Phase::Uploading;
	state_.liveState.clear();
	state_.issue.clear();
	nextAttempt_ = {};
	view_->apply(state_);
	submit();
}
void PlatformController::submit()
{
	if (closing_ || authenticating_ || paused_ || jobBusy() || id_.isEmpty())
		return;
	bool active = busy(), create = creating_;
	auto account = owner_, id = id_, title = title_;
	bool publish = publish_, live = live_;
	launch([this, account, id, title, publish, live, active, create] {
		HttpCancellationScope cancellation(cancelled_);
		if (auth_.account() != account)
			throw RequestError(403, "account_mismatch");
		ApiClient api(QUrl("https://admin.alive.org.tw/api"), [this, account](bool force) {
			auto token = auth_.bearer(force);
			if (auth_.account() != account)
				throw RequestError(403, "account_mismatch");
			return token;
		});
		CaptureSync sync(root_ + "/queue", account, id, api);
		PlatformJob result;
		if (create) {
			CaptureJournal j;
			j.account = account;
			j.localId = id;
			SessionStore store(root_ + "/queue");
			if (!QFileInfo::exists(store.mediaDirectory(account, id) + "/journal.json"))
				store.save(j);
			result.sync = sync.begin(title, publish, live);
		} else
			result.sync = sync.step(active);
		return result;
	});
}
void PlatformController::launch(std::function<PlatformJob()> work)
{
	if (jobBusy())
		return;
	awaitingResult_ = true;
	try {
		job_.setFuture(QtConcurrent::run(std::move(work)));
	} catch (...) {
		awaitingResult_ = false;
		throw;
	}
}
void PlatformController::control(bool closeLive)
{
	if (closing_ || authenticating_ || id_.isEmpty())
		return;
	try {
		CaptureSync::persistControl(root_ + "/queue", owner_, id_, closeLive);
		if (!closeLive)
			state_.cancelPending = true;
		else
			state_.issue = QString::fromUtf8("關閉直播意向已保存，待伺服器確認。");
		paused_ = false;
		failures_ = 0;
		nextAttempt_ = {};
		view_->apply(state_);
	} catch (...) {
		state_.issue = QString::fromUtf8("控制意向無法保存，請重試。");
		view_->apply(state_);
	}
}
void PlatformController::poll()
{
	if (closing_ || !view_)
		return;
	if (capture_ && capture_->finished()) {
		bool normal = capture_->wait(1);
		if (!normal && state_.issue.isEmpty())
			state_.issue = QString::fromUtf8("編碼未正常完成；本機素材保留，將同步 abort。") +
				       " " + capture_->error();
		capture_.reset();
		state_.phase = paused_ ? Phase::Failed : Phase::Uploading;
		nextAttempt_ = {};
		view_->apply(state_);
	}
	if (!authenticating_ && !id_.isEmpty() && !paused_ && !jobBusy() &&
	    std::chrono::steady_clock::now() >= nextAttempt_)
		submit();
}
void PlatformController::completed()
{
	const auto consumed = qScopeGuard([this] { awaitingResult_ = false; });
	if (closing_ || !view_)
		return;
	state_.checkingLocal = false;
	try {
		auto result = workerResult(job_.future());
		failures_ = 0;
		state_.issue.clear();
		if (result.scan) {
			recoveryCache_ = result.recovery;
			QStringList rows, ids, cleanupIds;
			for (const auto &j : result.recovery.sessions) {
				if (QFileInfo::exists(
					    SessionStore(root_ + "/queue").mediaDirectory(j.account, j.localId) +
					    "/remote-journal.json")) {
					ids << j.localId;
					const bool cleanup = SessionStore(root_ + "/queue")
								     .mayCleanup(j, QDateTime::currentDateTimeUtc());
					if (cleanup)
						cleanupIds << j.localId;
					rows << j.localId + QString::fromUtf8(
								    cleanup ? " · 已確認就緒滿七日，可查看暫存"
								    : j.confirmedReady ? " · 已確認就緒，七日內保留"
								    : j.normalEnd      ? " · 本機完整，可繼續同步"
										       : " · 未正常完成，資料保留");
				}
			}
			for (const auto &issue : result.recovery.issues)
				rows << issue.localId + QString::fromUtf8(" · 本機驗證失敗，資料保留");
			view_->setRecoveryText(rows.empty() ? QString::fromUtf8("尚無此帳號的本機收錄。")
							    : rows.join('\n'));
			view_->setRecoverySessions(ids, cleanupIds);
			view_->apply(state_);
			return;
		}
		const auto &remote = result.sync;
		state_.pendingBytes = remote.pendingBytes;
		state_.liveState = remote.liveState;
		state_.liveEnabled = remote.liveEnabled;
		if (remote.autoPublish == "cancelled") {
			state_.autoPublish = false;
			state_.cancelPending = false;
		}
		if (creating_) {
			creating_ = false;
			SessionStore store(root_ + "/queue");
			auto path = store.mediaDirectory(owner_, id_);
			atomicJson(path + "/local-session.json", {{"title", title_},
								  {"wireRevision", "c1-2026-10-08.2"},
								  {"autoPublish", publish_},
								  {"liveEnabled", live_},
								  {"audioTrack", int(track_)},
								  {"fpsNum", 30000},
								  {"fpsDen", 1001}});
			capture_ = std::make_unique<CaptureOutput>();
			if (!capture_->start({path, track_, root_ + "/queue", owner_, id_})) {
				state_.issue = QString::fromUtf8("無法開始 NVENC 收錄，將同步 abort。");
				capture_.reset();
				state_.phase = Phase::Uploading;
			} else
				state_.phase = Phase::Capturing;
		} else if (remote.state == "ready") {
			state_.phase = remote.autoPublish == "published" ? Phase::Published
				       : remote.autoPublish == "pending" ? Phase::Validating
									 : Phase::DraftReady;
			if (remote.autoPublish == "blocked")
				state_.issue =
					QString::fromUtf8("影片已就緒，自動發布受阻；請在管理平台檢查權限／封面。 ");
			if (remote.autoPublish != "pending")
				paused_ = true;
		} else if (remote.state == "failed" || remote.state == "expired" || remote.state == "aborted") {
			state_.terminalFailure = true;
			if (capture_)
				capture_->stop(StopReason::EncoderFailure);
			state_.phase = busy() ? Phase::StopPending : Phase::Failed;
			state_.issue = QString::fromUtf8("伺服器狀態：") + remote.state +
				       QString::fromUtf8("。本機資料保留。");
			paused_ = true;
		} else if (busy())
			state_.phase = stopping_ ? (remote.stopAccepted ? Phase::Uploading : Phase::StopPending)
						 : Phase::Capturing;
		else
			state_.phase = remote.sealAccepted ? Phase::Validating : Phase::Uploading;
		if (remote.stopAccepted && state_.phase == Phase::StopPending)
			state_.phase = Phase::Uploading;
		tryObserverJson(root_ + "/integration-status.json",
				{{"localId", id_},
				 {"recordingId", remote.recordingId},
				 {"captureId", remote.captureId},
				 {"state", remote.state},
				 {"liveState", remote.liveState},
				 {"liveEnabled", remote.liveEnabled},
				 {"pendingBytes", qint64(remote.pendingBytes)},
				 {"autoPublish", remote.autoPublish},
				 {"stopAccepted", remote.stopAccepted},
				 {"sealAccepted", remote.sealAccepted},
				 {"lastSequence", remote.lastSequence},
				 {"observedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
		nextAttempt_ = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	} catch (const RequestError &e) {
		state_.issue = QString::fromUtf8(e.what());
		if (e.status == 410 && e.code == "capture_expired") {
			state_.terminalFailure = true;
			if (capture_)
				capture_->stop(StopReason::EncoderFailure);
			state_.phase = busy() ? Phase::StopPending : Phase::Failed;
		}
		auto delay = e.retryDelay(failures_);
		if (!delay)
			paused_ = true;
		nextAttempt_ = std::chrono::steady_clock::now() + std::chrono::seconds(delay.value_or(0)) +
			       std::chrono::milliseconds(QRandomGenerator::global()->bounded(1000));
		if (!busy())
			state_.phase = Phase::Failed;
	} catch (...) {
		paused_ = true;
		state_.issue = QString::fromUtf8("同步檢查失敗；保留原始意向及本機資料，請重新檢查。");
		if (!busy())
			state_.phase = Phase::Failed;
	}
	view_->apply(state_);
}
} // namespace hhc
