#pragma once
#include "capture-output.hpp"
#include "key-transfer.hpp"
#include "platform-controller.hpp"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTimer>
#include <QFutureWatcher>
#include <QLockFile>
namespace hhc {
class KeyController : public QObject {
public:
	explicit KeyController(QString root, QObject *parent = nullptr);
	~KeyController() override;
	QWidget *view() const { return view_; }
	void shutdown();

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
	QComboBox *track_;
	QPushButton *action_, *save_, *legacyButton_;
	QLabel *status_, *titleLabel_;
	QTimer timer_;
	QFutureWatcher<KeyJob> sync_, control_;
	std::unique_ptr<CaptureOutput> capture_;
	std::unique_ptr<PlatformController> legacy_;
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
