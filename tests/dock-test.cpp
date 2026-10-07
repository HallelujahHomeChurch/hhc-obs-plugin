#include "dock.hpp"
#include <QApplication>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDir>
#include <iostream>
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	hhc::Dock dock;
	dock.setAttribute(Qt::WA_DontShowOnScreen);
	int fails = 0;
	auto check = [&](bool ok, const char *s) {
		if (!ok) {
			std::cerr << "FAIL " << s << '\n';
			++fails;
		}
	};
	auto *live = dock.findChild<QCheckBox *>("live");
	auto *publish = dock.findChild<QCheckBox *>("publish");
	auto *status = dock.findChild<QLabel *>("status");
	auto *title = dock.findChild<QLineEdit *>("title");
	auto *action = dock.findChild<QPushButton *>("action");
	check(live && publish && status && title && action, "required native dock controls");
	if (!live || !publish || !status || !title || !action)
		return 1;
	hhc::DockState state;
	state.phase = hhc::Phase::Ready;
	state.canPublish = true;
	dock.apply(state);
	check(!live->isChecked(), "live defaults off");
	check(publish->checkState() == Qt::PartiallyChecked && !action->isEnabled(),
	      "first use requires explicit publication choice");
	state.autoPublish = false;
	state.liveEnabled = true;
	dock.apply(state);
	check(live->isChecked() && !publish->isChecked(), "live and publication independent");
	state.canPublish = false;
	dock.apply(state);
	check(!live->isEnabled() && !publish->isEnabled(), "publish permission gates both exposure controls");
	state.phase = hhc::Phase::DraftReady;
	dock.apply(state);
	check(!status->text().contains("已發布"), "ready is not published");
	state.phase = hhc::Phase::StopPending;
	dock.apply(state);
	check(status->text().contains("待同步"), "offline stop does not imply server acknowledgement");
	title->setText(QString(200, QChar(0x805A)));
	check(title->text().size() == 180, "Chinese title limit");
	state.phase = hhc::Phase::Capturing;
	state.pendingBytes = 345000000;
	state.issue = QString::fromUtf8("網路中斷，收錄持續，片段暫存本機。");
	dock.apply(state);
	dock.resize(360, 620);
	dock.show();
	app.processEvents();
	check(dock.minimumWidth() == 320, "minimum dock DIP width");
	QDir().mkpath("artifacts/ui");
	dock.grab().save("artifacts/ui/dock-mock-" + qEnvironmentVariable("QT_SCALE_FACTOR", "1") + ".png");
	dock.close();
	check(!dock.isVisible(), "close hides mock dock");
	return fails ? 1 : 0;
}
