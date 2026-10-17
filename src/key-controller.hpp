#pragma once
#include "capture-output.hpp"
#include "key-transfer.hpp"
#include <QElapsedTimer>
#include <deque>
#include <QPointer>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QFutureWatcher>
#include <QLockFile>
#include <functional>
namespace hhc {
class KeyController : public QObject {
public:
	explicit KeyController(QString root, QObject *parent = nullptr, std::function<unsigned()> audioTrack = {});
	~KeyController() override;
	QWidget *view() const { return view_; }
	void shutdown();
	QWidget *settingsPage() const { return settings_; }
	QLabel *statusLabel() const { return status_; }
	QLineEdit *keyField() const { return key_; }
	bool applySettings();
	bool validateSettings();
	void beginSettings();
	void discardSettings();
	std::function<void()> openSettings;

private:
	friend struct KeyControllerTest;
	void startStop();
	bool retireUnbound();
	void tick();
	void syncDone();
	void pollControl();
	void launchHelper();
	void display();
	QString root_, id_, actor_, title_, issue_;
	QPointer<QWidget> view_;
	QLineEdit *key_;
	unsigned audioTrack_ = 1;
	std::function<unsigned()> audioTrackProvider_;
	QWidget *settings_;
	QLabel *settingsMessage_;
	std::atomic<quint64> uploadedBytes_{0};
	QElapsedTimer rateClock_;
	std::deque<std::pair<qint64, quint64>> rateSamples_;
	quint64 pendingBytes_ = 0;
	QPushButton *action_, *retry_, *showKey_;
	QLabel *status_;
	QTimer timer_;
	QFutureWatcher<KeyJob> sync_, control_;
	std::unique_ptr<CaptureOutput> capture_;
	std::unique_ptr<QLockFile> lease_;
	std::atomic<bool> cancelled_{false};
	bool closing_ = false, starting_ = false, stopping_ = false, paused_ = false, syncAwaiting_ = false,
	     controlAwaiting_ = false;
	bool released_ = false, revoked_ = false;
	bool keyAvailable_ = false, recoveryValidated_ = false;
	unsigned failures_ = 0;
	qint64 next_ = 0, nextControl_ = 0;
	std::optional<int> end_;
};
} // namespace hhc
