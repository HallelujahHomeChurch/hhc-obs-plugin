#include "local-controller.hpp"
#include "hls-muxer.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QJsonObject>
#include <QMessageBox>
#include <QUrl>
#include <QUuid>
namespace hhc {
static const QString localAccount = "offline-validation-only";
LocalController::LocalController(QString root, QObject *parent)
	: QObject(parent),
	  view_(new Dock),
	  root_(std::move(root))
{
	state_.localOnly = true;
	state_.phase = Phase::Ready;
	state_.autoPublish = false;
	view_->apply(state_);
	view_->onAction = [this] {
		action();
	};
	view_->onRefresh = [this] {
		refresh();
	};
	view_->onOpenFolder = [this] {
		QDir().mkpath(root_);
		QDesktopServices::openUrl(QUrl::fromLocalFile(root_));
	};
	if (parent)
		parent->installEventFilter(this);
	timer_.setInterval(200);
	connect(&timer_, &QTimer::timeout, this, [this] { poll(); });
	connect(&recovery_, &QFutureWatcher<RecoveryReport>::finished, this, [this] {
		if (!view_ || closing_)
			return;
		state_.checkingLocal = false;
		view_->apply(state_);
		try {
			const auto report = recovery_.result();
			QStringList rows;
			for (const auto &j : report.sessions) {
				quint64 bytes = 0;
				for (const auto &o : j.objects)
					bytes += o.size;
				rows << QString::fromUtf8("%1\n%2 · %3 MB · 僅存本機")
						.arg(j.localId, j.normalEnd ? QString::fromUtf8("本機完成")
									    : QString::fromUtf8("未完成，資料保留"))
						.arg(bytes / 1000000.0, 0, 'f', 1);
			}
			for (const auto &issue : report.issues)
				rows << QString::fromUtf8("%1：驗證失敗，資料保留待處理").arg(issue.localId);
			view_->setRecoveryText(rows.empty() ? QString::fromUtf8("尚無本機驗證收錄。")
							    : rows.join("\n\n"));
		} catch (...) {
			view_->setRecoveryText(QString::fromUtf8("無法讀取本機收錄；資料保留，請檢查資料夾權限。"));
		}
	});
	refresh();
}
LocalController::~LocalController()
{
	shutdown();
	if (view_)
		delete view_.data();
}
void LocalController::shutdown()
{
	closing_ = true;
	timer_.stop();
	if (capture_)
		capture_->stop(StopReason::Shutdown);
	capture_.reset();
	recovery_.waitForFinished();
}
bool LocalController::eventFilter(QObject *, QEvent *event)
{
	if (event->type() == QEvent::Close && busy() && !closing_) {
		const auto answer =
			QMessageBox::question(view_, QString::fromUtf8("本機收錄尚未完成"),
					      QString::fromUtf8("退出 OBS 會停止本機收錄並保留未完成資料。確定退出？"),
					      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
		if (answer != QMessageBox::Yes) {
			event->ignore();
			return true;
		}
		shutdown();
	}
	return false;
}
void LocalController::refresh()
{
	if (!view_ || closing_ || recovery_.isRunning())
		return;
	if (busy()) {
		view_->setRecoveryText(QString::fromUtf8("目前正在收錄；結束後自動檢查本機資料。"));
		return;
	}
	view_->setRecoveryText(QString::fromUtf8("正在檢查本機收錄與 SHA-256…"));
	state_.checkingLocal = true;
	view_->apply(state_);
	recovery_.setFuture(QtConcurrent::run([root = root_] { return SessionStore(root).scanPending(localAccount); }));
}
void LocalController::action()
{
	if (!view_ || closing_)
		return;
	if (busy()) {
		if (state_.phase != Phase::Capturing)
			return;
		state_.phase = Phase::StopPending;
		view_->apply(state_);
		capture_->stop(StopReason::User);
		return;
	}
	// Avoid a recovery read racing startup or claiming a partially written object.
	if (recovery_.isRunning())
		return;
	capture_ = std::make_unique<CaptureOutput>();
	const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	SessionStore store(root_);
	const auto path = store.mediaDirectory(localAccount, id);
	state_.issue.clear();
	state_.pendingBytes = 0;
	if (!capture_->start({path, view_->audioTrack(), root_, localAccount, id})) {
		state_.phase = Phase::Failed;
		state_.issue = QString::fromUtf8("無法開始本機收錄：") + capture_->error();
		capture_.reset();
		view_->apply(state_);
		refresh();
		return;
	}
	state_.phase = Phase::Capturing;
	try {
		atomicJson(path + "/local-session.json", {{"title", view_->title()},
							  {"mode", "offline-validation"},
							  {"fpsNum", 30000},
							  {"fpsDen", 1001},
							  {"audioTrack", int(view_->audioTrack())}});
	} catch (...) {
		state_.issue = QString::fromUtf8("本機收錄資訊無法保存，正在停止。資料保留。 ");
		state_.phase = Phase::StopPending;
		capture_->stop(StopReason::EncoderFailure);
	}
	view_->apply(state_);
	timer_.start();
}
void LocalController::poll()
{
	if (capture_) {
		state_.pendingBytes = capture_->encodedBytes();
		if (view_)
			view_->apply(state_);
	}
	if (!capture_ || !capture_->finished())
		return;
	timer_.stop();
	if (capture_->wait(1))
		state_.phase = Phase::LocalComplete;
	else {
		state_.phase = Phase::Failed;
		state_.issue = capture_->error();
	}
	capture_.reset();
	if (view_)
		view_->apply(state_);
	refresh();
}
} // namespace hhc
