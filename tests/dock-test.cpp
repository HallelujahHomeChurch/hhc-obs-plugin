#include "dock.hpp"
#include <QApplication>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDir>
#include <QComboBox>
#include <QKeyEvent>
#include <QTimer>
#include <QMessageBox>
#include <QJsonObject>
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
	auto *cleanup = dock.findChild<QPushButton *>("openCleanupFolder");
	check(cleanup && !cleanup->isEnabled(), "cleanup entry cannot open unqualified media by default");
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
	state.localOnly = true;
	state.phase = hhc::Phase::Ready;
	state.autoPublish.reset();
	dock.apply(state);
	check(action->isEnabled() && action->text().contains("本機"),
	      "local validation can start without platform login");
	check(!live->isEnabled() && !publish->isEnabled(), "local validation cannot expose live or publication");
	auto *track = dock.findChild<QComboBox *>("audioTrack");
	check(track && track->count() == 6, "explicit audio mixer selection");
	if (track) {
		track->setCurrentIndex(2);
		check(dock.audioTrack() == 3, "selected mixer is passed to capture");
	}
	int clicks = 0;
	state.localOnly = false;
	state.connected = true;
	state.account = "synthetic";
	state.phase = hhc::Phase::Ready;
	state.canPublish = true;
	state.autoPublish = false;
	dock.apply(state);
	dock.setBroadcasts(QJsonArray{QJsonObject{{"title", "SYNTHETIC Console broadcast"},
						  {"recordingId", "018f0c1f-18d0-7e81-9f6f-69c456db7003"}}});
	auto *broadcasts = dock.findChild<QComboBox *>("broadcasts");
	broadcasts->setCurrentIndex(1);
	check(!dock.selectedBroadcast().isEmpty() && !title->isEnabled() && !live->isEnabled() &&
		  !publish->isEnabled(),
	      "Console binding uses server details and policy");
	broadcasts->setCurrentIndex(0);
	check(title->isEnabled() && live->isEnabled() && publish->isEnabled(),
	      "standalone controls restored after deselection");
	state.broadcastBound = true;
	state.broadcastPhase = "end_pending";
	state.phase = hhc::Phase::Capturing;
	dock.apply(state);
	check(dock.findChild<QPushButton *>("closeLive")->isHidden() && action->isEnabled(),
	      "Console End keeps HHC stop available and hides legacy live control");
	check(dock.findChild<QPushButton *>("broadcastRefresh")->isEnabled(),
	      "active B1 can retry original control without rebind");
	state.broadcastBound = false;
	state.localOnly = true;
	state.phase = hhc::Phase::Ready;
	dock.apply(state);
	dock.onAction = [&] {
		++clicks;
	};
	action->click();
	check(clicks == 1, "dock action reaches controller");
	state.phase = hhc::Phase::Capturing;
	dock.apply(state);
	check(track && !track->isEnabled(), "audio mixer locked during capture");
	auto answerStop = [&](QMessageBox::StandardButton answer) {
		bool prompted = false;
		QTimer::singleShot(0, &dock, [&] {
			if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
				prompted = true;
				box->button(answer)->click();
			}
		});
		action->click();
		app.processEvents();
		return prompted;
	};
	check(answerStop(QMessageBox::Cancel) && clicks == 1, "cancel stop must leave capture controller untouched");
	check(answerStop(QMessageBox::Yes) && clicks == 2, "confirmed stop reaches capture controller once");
	QTimer::singleShot(0, &dock, [&] {
		state.phase = hhc::Phase::Failed;
		state.terminalFailure = true;
		dock.apply(state);
		if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
			box->button(QMessageBox::Yes)->click();
	});
	action->click();
	app.processEvents();
	check(clicks == 2, "obsolete stop confirmation cannot prepare or start another event");
	state.terminalFailure = false;
	state.phase = hhc::Phase::LocalComplete;
	dock.apply(state);
	check(status->text().contains("本機") && !status->text().contains("已發布"),
	      "local completion is never platform publication");
	state.localOnly = false;
	state.connected = true;
	state.phase = hhc::Phase::Failed;
	dock.apply(state);
	check(!action->isEnabled(), "unresolved synchronization failure cannot abandon its session");
	state.terminalFailure = true;
	dock.apply(state);
	check(action->isEnabled(), "settled terminal failure permits preparing a replacement event");
	action->click();
	check(clicks == 3, "prepare replacement action reaches the controller");
	state.connected = false;
	dock.apply(state);
	check(!action->isEnabled(), "replacement requires a logged-in account");
	state.connected = true;
	state.phase = hhc::Phase::StopPending;
	dock.apply(state);
	check(!action->isEnabled(), "terminal response cannot replace an encoder still stopping");
	state.terminalFailure = false;
	state.account = "synthetic-user";
	state.phase = hhc::Phase::Capturing;
	state.liveEnabled = true;
	state.autoPublish = true;
	dock.apply(state);
	auto *login = dock.findChild<QPushButton *>("login");
	auto *logout = dock.findChild<QPushButton *>("logout");
	auto *closeLive = dock.findChild<QPushButton *>("closeLive");
	auto *cancelPublish = dock.findChild<QPushButton *>("cancelPublish");
	check(login && !login->isEnabled(), "cannot change account during active capture");
	check(logout && !logout->isEnabled(), "cannot sign out during active capture");
	check(closeLive && closeLive->isEnabled() && cancelPublish && cancelPublish->isEnabled(),
	      "independent live and publication controls");
	for (const auto &[serverState, message] :
	     {std::pair{"starting", "準備中"}, std::pair{"live", "直播中"}, std::pair{"recovering", "補傳中"},
	      std::pair{"interrupted", "連線中斷"}, std::pair{"ending", "收尾中"}, std::pair{"ended", "已結束"},
	      std::pair{"failed", "失敗"}, std::pair{"expired", "已到期"}, std::pair{"aborted", "已中止"}}) {
		state.liveState = serverState;
		dock.apply(state);
		auto *liveStatus = dock.findChild<QLabel *>("liveStatus");
		check(liveStatus && liveStatus->text().contains(QString::fromUtf8(message)), serverState);
		check(action->isEnabled() && action->text().contains("停止"),
		      "live status does not stop local recording");
		check(state.autoPublish == true && cancelPublish->isEnabled(),
		      "live status leaves publication independent");
		state.liveEnabled = false;
		dock.apply(state);
		check(liveStatus && liveStatus->text().contains(QString::fromUtf8(message)) &&
			      liveStatus->text().contains("未開放"),
		      "server progress and viewer admission are independent");
		check(!closeLive->isEnabled(), "disabled admission cannot be closed again");
		state.liveEnabled = true;
	}
	dock.apply(state);
	int controls = 0;
	dock.onCloseLive = [&] {
		controls += 1;
	};
	dock.onCancelPublish = [&] {
		controls += 10;
	};
	if (closeLive)
		closeLive->click();
	check(controls == 1, "close live leaves publication intent untouched");
	if (cancelPublish)
		cancelPublish->click();
	check(controls == 11, "cancel publication is distinct callback");
	state.phase = hhc::Phase::Ready;
	state.canPublish = true;
	state.liveEnabled = false;
	state.autoPublish = false;
	dock.apply(state);
	if (cleanup) {
		auto *sessions = dock.findChild<QComboBox *>("recoverSession");
		QString opened;
		dock.onOpenCleanup = [&](QString id) {
			opened = id;
		};
		dock.setRecoverySessions({"failed", "ready-aged"}, {"ready-aged"});
		check(!cleanup->isEnabled(), "failed session cannot be offered for cleanup");
		sessions->setCurrentIndex(1);
		check(cleanup->isEnabled(), "eligible selected success can be inspected");
		cleanup->click();
		check(opened == "ready-aged", "cleanup targets only the qualified selected session");
		for (const auto phase : {hhc::Phase::Capturing, hhc::Phase::Uploading, hhc::Phase::Validating}) {
			state.phase = phase;
			dock.apply(state);
			check(!cleanup->isEnabled(), "active work cannot offer cleanup");
		}
		state.phase = hhc::Phase::Ready;
		state.checkingLocal = true;
		dock.apply(state);
		check(!cleanup->isEnabled(), "verification in progress cannot offer cleanup");
		state.checkingLocal = false;
		state.connected = false;
		dock.apply(state);
		check(!cleanup->isEnabled(), "disconnected account cannot offer cleanup");
		state.connected = true;
		dock.setRecoverySessions({"another-account"});
		dock.apply(state);
		check(!cleanup->isEnabled(), "refresh removes prior account cleanup selection");
	}
	check(logout && logout->isEnabled(), "signed-in idle operator has a logout entry");
	if (logout) {
		int signOuts = 0;
		dock.onLogout = [&] {
			++signOuts;
		};
		logout->click();
		check(signOuts == 1, "idle sign out reaches controller once");
		state.checkingLocal = true;
		dock.apply(state);
		check(!logout->isEnabled(), "local scan must finish before sign out");
		logout->click();
		check(signOuts == 1, "pending scan cannot invoke sign out");
		state.checkingLocal = false;
		dock.apply(state);
	}
	title->setText("Previous account title");
	track->setCurrentIndex(4);
	dock.setRecoverySessions({"previous-account-session"});
	state.account = "another-synthetic-account";
	dock.apply(state);
	check(title->text() != "Previous account title" && dock.audioTrack() == 1 &&
		      dock.findChild<QComboBox *>("recoverSession")->count() == 0,
	      "account change clears previous title audio preference and recovery selection");
	dock.show();
	QApplication::setActiveWindow(&dock);
	title->setFocus();
	app.processEvents();
	for (auto *next : {static_cast<QWidget *>(track), static_cast<QWidget *>(live), static_cast<QWidget *>(publish),
			   static_cast<QWidget *>(action)}) {
		QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
		QApplication::sendEvent(QApplication::focusWidget(), &tab);
		check(QApplication::focusWidget() == next, "keyboard follows title/track/live/publication/action");
	}
	title->setFocus();
	const int beforeReturn = clicks;
	QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
	QApplication::sendEvent(title, &enter);
	check(clicks == beforeReturn, "enter in title does not start recording");
	return fails ? 1 : 0;
}
