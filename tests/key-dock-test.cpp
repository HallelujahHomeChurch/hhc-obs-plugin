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
#include "ingest.hpp"
#include "windows-path.hpp"
#include <QJsonDocument>
#include <QtConcurrent/QtConcurrentRun>
namespace hhc {
struct KeyControllerTest {
	static int k4Recovery(KeyController &c)
	{
		c.timer_.stop();
		c.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		const auto original = c.id_;
		KeyIngest(c.root_ + "/intents", original).prepareOpen();
		writeAtomicMetadata(c.root_ + "/active.json",
				    QJsonDocument(QJsonObject{{"localId", original},
							      {"protocolVersion", "k4-2026-10-15.rc1"},
							      {"started", false},
							      {"released", false}})
					    .toJson());
		c.starting_ = true;
		c.syncAwaiting_ = true; // Cancellation while a possibly committed open is in flight.
		c.startStop();
		c.syncAwaiting_ = false;
		int failed = c.id_ != original || !c.stopping_ || c.released_ || c.retireUnbound();
		failed += !QFileInfo::exists(c.root_ + "/intents/" + original + ".stopping");
		c.startStop();
		failed += c.id_ != original; // Cannot create a second output while cleanup is unresolved.
		c.lease_->unlock();
		{
			KeyController restored(QFileInfo(c.root_).absolutePath());
			restored.timer_.stop();
			failed += restored.id_ != original || !restored.stopping_ || restored.starting_;
			restored.closing_ = true; // No background process or real transport in this mock.
		}
		c.lease_->tryLock(0);
		c.sync_.setFuture(QtConcurrent::run([]() -> KeyJob {
			RequestError e(409, "broadcast_output_terminal");
			e.outputOpening = true;
			throw e;
		}));
		try {
			c.sync_.waitForFinished();
		} catch (...) {
		}
		c.syncDone();
		failed += !c.released_ || c.starting_ || c.stopping_ || c.id_ != original;
		const auto future = c.next_;
		c.tick();
		failed += c.syncAwaiting_ || c.id_ != original || c.next_ != future;
		// A missing K4 intent must not silently enter the legacy resolve path.
		c.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		writeAtomicMetadata(c.root_ + "/active.json",
				    QJsonDocument(QJsonObject{{"localId", c.id_},
							      {"protocolVersion", "k4-2026-10-15.rc1"},
							      {"released", false}})
					    .toJson());
		c.released_ = false;
		c.stopping_ = true;
		failed += c.retireUnbound();
		c.tick();
		failed += !c.paused_ || c.syncAwaiting_ || c.released_;
		c.id_.clear();
		c.paused_ = c.stopping_ = false;
		QFile::remove(c.root_ + "/active.json");
		QDir().mkpath(c.root_ + "/active.json"); // Force the active pointer commit to fail.
		c.keyAvailable_ = true;
		c.startStop();
		failed += !c.id_.isEmpty() || c.starting_ || !c.paused_ || c.syncAwaiting_;
		QDir().rmdir(c.root_ + "/active.json");
		c.paused_ = false;
		if (failed)
			std::cerr << "FAIL K4 cancellation, terminal or missing-intent recovery\n";
		return failed;
	}
	static int settingsAfterRelease(KeyController &c)
	{
		c.timer_.stop();
		c.id_ = "completed-session";
		c.released_ = false;
		c.display();
		int failed = c.key_->isEnabled();
		c.released_ = true;
		c.display();
		failed += !c.key_->isEnabled();
		c.syncAwaiting_ = true;
		c.display();
		failed += c.key_->isEnabled();
		c.syncAwaiting_ = false;
		c.released_ = false;
		c.id_.clear();
		if (failed)
			std::cerr << "FAIL released input must allow key replacement only while idle\n";
		return failed;
	}
	static int simpleControls(KeyController &c)
	{
		c.keyAvailable_ = true;
		c.paused_ = false;
		c.display();
		int failed = c.settings_->isWindow() || !c.retry_->isHidden();
		failed += c.action_->text() != QString::fromUtf8("開始 HHC 串流");
		c.starting_ = true;
		c.display();
		failed += c.action_->text() != QString::fromUtf8("停止 HHC 串流");
		c.starting_ = false;
		c.display();
		c.pendingBytes_ = 1048576;
		c.rateSamples_.clear();
		c.rateSamples_.emplace_back(c.rateClock_.elapsed() - 1000, 0);
		c.uploadedBytes_ = 5000000;
		c.display();
		failed += !c.status_->text().contains("Mbps") || !c.status_->text().contains("1.0 MB");
		bool requested = false;
		c.openSettings = [&] { requested = true; };
		c.keyAvailable_ = false;
		c.startStop();
		c.keyAvailable_ = true;
		failed += !requested || !c.settings_->isHidden();
		c.key_->setText("unsaved-secret");
		c.showKey_->click();
		failed += c.key_->echoMode() != QLineEdit::Normal;
		c.discardSettings();
		failed += !c.key_->text().isEmpty() || c.key_->echoMode() != QLineEdit::Password;
		c.key_->setText("invalid");
		c.key_->setModified(true);
		failed += c.applySettings();
		c.discardSettings();
		c.openSettings = {};
		c.paused_ = true;
		c.display();
		failed += c.retry_->isHidden();
		c.retry_->click(); // No active input: this must not create a capture.
		failed += c.paused_ || !c.id_.isEmpty();
		c.audioTrackProvider_ = [] {
			return 0u;
		};
		c.startStop();
		failed += !c.id_.isEmpty(); // Invalid OBS mixer must not silently fall back.
		c.audioTrackProvider_ = {};
		if (failed)
			std::cerr << "FAIL settings, recovery or audio selection behavior\n";
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
		int failed = !c.id_.isEmpty() || !c.key_->isEnabled();
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
	failed += hhc::KeyControllerTest::k4Recovery(controller);
	failed += hhc::KeyControllerTest::lifecycle(controller);
	if (failed)
		std::cerr << "FAIL key-only dock has unexpected credential or metadata controls\n";
	return failed ? 1 : 0;
}
