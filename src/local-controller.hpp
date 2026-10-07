#pragma once
#include "capture-output.hpp"
#include "dock.hpp"
#include "session-store.hpp"
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QFutureWatcher>
namespace hhc {
// Local validation only. Never binds this offline queue to a future login.
class LocalController : public QObject {
public:
	explicit LocalController(QString queueRoot, QObject *parent = nullptr);
	~LocalController() override;
	Dock *view() const { return view_; }
	bool busy() const { return capture_ && !capture_->finished(); }
	Phase phase() const { return state_.phase; }
	void shutdown();
	void refresh();

protected:
	bool eventFilter(QObject *, QEvent *) override;

private:
	void action();
	void poll();
	QPointer<Dock> view_;
	QString root_;
	DockState state_;
	std::unique_ptr<CaptureOutput> capture_;
	QTimer timer_;
	QFutureWatcher<RecoveryReport> recovery_;
	bool closing_ = false;
};
} // namespace hhc
