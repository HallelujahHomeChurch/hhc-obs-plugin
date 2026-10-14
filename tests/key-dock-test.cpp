#include "key-controller.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QPushButton>
#include <iostream>
#include <limits>
#include <QDir>
#include <QFile>
#include <QPromise>
#include <QDateTime>
namespace hhc {
struct KeyControllerTest {
	static int settingsAfterRelease(KeyController &c)
	{
		c.timer_.stop();
		c.id_ = "completed-session";
		c.released_ = false;
		c.display();
		int failed = c.save_->isEnabled();
		c.released_ = true;
		c.display();
		failed += !c.save_->isEnabled();
		c.syncAwaiting_ = true;
		c.display();
		failed += c.save_->isEnabled();
		c.syncAwaiting_ = false;
		c.released_ = false;
		c.id_.clear();
		if (failed) std::cerr << "FAIL released input must allow key replacement only while idle\n";
		return failed;
	}
	static int simpleControls(KeyController &c)
	{
		c.keyAvailable_ = true;
		c.paused_ = false;
		c.display();
		int failed = !c.settings_->isHidden() || !c.retry_->isHidden();
		c.settingsButton_->click();
		failed += c.settings_->isHidden();
		c.key_->setText("unsaved-secret");
		c.settingsButton_->click();
		failed += !c.settings_->isHidden() || !c.key_->text().isEmpty();
		c.paused_ = true;
		c.display();
		failed += c.retry_->isHidden();
		c.retry_->click(); // No active input: this must not create a capture.
		failed += c.paused_ || !c.id_.isEmpty();
		c.audioTrackProvider_ = [] { return 0u; };
		c.startStop();
		failed += !c.id_.isEmpty(); // Invalid OBS mixer must not silently fall back.
		c.audioTrackProvider_ = {};
		if (failed) std::cerr << "FAIL settings, recovery or audio selection behavior\n";
		return failed;
	}
	static int uploadPacing(KeyController &c)
	{
		c.timer_.stop();
		c.id_ = "pacing-test";
		int failed = 0;
		for (auto pending : {quint64(1024), quint64(0)}) {
			KeyJob result;
			result.actor = "pacing-account";
			result.sync.state = "uploading";
			result.sync.pendingBytes = pending;
			QPromise<KeyJob> promise;
			promise.start();
			promise.addResult(result);
			promise.finish();
			c.sync_.setFuture(promise.future());
			c.syncDone();
			const auto delay = c.next_ - QDateTime::currentMSecsSinceEpoch();
			const bool ok = pending ? delay <= 100 : delay >= 2000;
			if (!ok) {
				std::cerr << "FAIL upload backlog must drain without idle polling delay\n";
				++failed;
			}
		}
		c.id_.clear();
		c.actor_.clear();
		return failed;
	}
	static int lifecycle(KeyController &c)
	{
		c.timer_.stop();
		c.next_ = std::numeric_limits<qint64>::max();
		c.keyAvailable_ = false;
		c.cancelled_ = true;
		c.startStop();
		try {
			c.sync_.waitForFinished();
		} catch (...) {
		}
		c.syncAwaiting_ = false;
		int failed = !c.id_.isEmpty() || !c.save_->isEnabled();
		c.id_ = "unbound-test";
		c.starting_ = true;
		c.stopping_ = false;
		c.startStop();
		failed += !c.id_.isEmpty() || c.starting_;
		c.id_ = "uncertain-test";
		c.starting_ = true;
		c.stopping_ = false;
		QDir().mkpath(c.root_ + "/intents");
		QFile intent(c.root_ + "/intents/uncertain-test.json");
		if (!intent.open(QIODevice::WriteOnly))
			return failed + 1;
		intent.write("{}");
		intent.close();
		c.startStop();
		failed += c.id_ != "uncertain-test" || c.starting_ || !c.stopping_;
		c.id_.clear();
		c.closing_ = true;
		return failed;
	}
};
} // namespace hhc
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	QTemporaryDir root;
	hhc::KeyController controller(root.path());
	auto *view = controller.view();
	int failed = 0;
	const auto fields = view->findChildren<QLineEdit *>();
	if (fields.size() != 1 || fields[0]->echoMode() != QLineEdit::Password)
		++failed;
	for (auto *button : view->findChildren<QPushButton *>())
		if (button->text().contains(QString::fromUtf8("登入")) ||
		    button->text().contains(QString::fromUtf8("登出")))
			++failed;
	failed += hhc::KeyControllerTest::settingsAfterRelease(controller);
	failed += hhc::KeyControllerTest::simpleControls(controller);
	failed += hhc::KeyControllerTest::uploadPacing(controller);
	failed += hhc::KeyControllerTest::lifecycle(controller);
	if (failed)
		std::cerr << "FAIL key-only dock has unexpected credential or metadata controls\n";
	return failed ? 1 : 0;
}
