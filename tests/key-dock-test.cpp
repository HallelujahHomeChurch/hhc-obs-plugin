#include "key-controller.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QPushButton>
#include <iostream>
#include <limits>
#include <QDir>
#include <QFile>
namespace hhc {
struct KeyControllerTest {
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
	failed += hhc::KeyControllerTest::lifecycle(controller);
	if (failed)
		std::cerr << "FAIL key-only dock has unexpected credential or metadata controls\n";
	return failed ? 1 : 0;
}
