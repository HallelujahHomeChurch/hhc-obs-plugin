#pragma once
#include "capture-output.hpp"
#include "capture-sync.hpp"
#include "native-auth.hpp"
#include "dock.hpp"
#include "session-store.hpp"
#include <QPointer>
#include <QTimer>
#include <QFutureWatcher>
namespace hhc {
struct PlatformJob {
	SyncState sync;
	RecoveryReport recovery;
	bool scan = false;
};
class PlatformController : public QObject {
public:
	explicit PlatformController(QString root, QObject *parent = nullptr);
	~PlatformController() override;
	Dock *view() const { return view_; }
	void shutdown();
	bool busy() const { return capture_ && !capture_->finished(); }

protected:
	bool eventFilter(QObject *, QEvent *) override;

private:
	friend struct ControllerLifecycleTest;
	void action();
	void poll();
	void refresh();
	void submit();
	void launch(std::function<PlatformJob()>);
	bool jobBusy() const { return awaitingResult_ || job_.isRunning(); }
	void recover(QString);
	void control(bool);
	void completed();
	void helper();
	QString root_, id_, title_, owner_;
	unsigned track_ = 1;
	RecoveryReport recoveryCache_;
	bool stopping_ = false;
	bool publish_ = false, live_ = false, creating_ = false, closing_ = false, paused_ = false;
	bool authenticating_ = false;
	bool awaitingResult_ = false;
	unsigned failures_ = 0;
	std::chrono::steady_clock::time_point nextAttempt_{};
	std::atomic<bool> cancelled_{false};
	NativeAuth auth_;
	QPointer<Dock> view_;
	DockState state_;
	std::unique_ptr<CaptureOutput> capture_;
	QTimer timer_;
	QFutureWatcher<PlatformJob> job_;
};
} // namespace hhc
