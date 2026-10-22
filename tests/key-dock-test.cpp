#include "key-controller.hpp"
#include "gpu-encode.hpp"
#include <QApplication>
#include <QMessageBox>
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
	static int pausedDiagnostics(QApplication &app)
	{
		QTemporaryDir root;
		KeyController c(root.path());
		c.timer_.stop();
		c.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		const auto original = c.id_;
		int failed = 0;
		auto failWorker = [&](bool control, std::exception_ptr error) {
			auto &watcher = control ? c.control_ : c.sync_;
			c.paused_ = false;
			watcher.setFuture(QtConcurrent::run([error]() -> KeyJob { std::rethrow_exception(error); }));
			try { watcher.waitForFinished(); } catch (...) {}
			app.processEvents();
			QFile file(c.root_ + "/diagnostics/native/" + (control ? "control" : "sync") + "-error.json");
			if (!file.open(QIODevice::ReadOnly)) { ++failed; return QJsonObject{}; }
			const auto bytes = file.readAll();
			failed += bytes.contains("synthetic-secret") || bytes.contains("https://") || bytes.contains("signed");
			const auto record = QJsonDocument::fromJson(bytes).object();
			failed += record["localId"] != original || record["stage"] != (control ? "control" : "sync");
			return record;
		};
		auto record = failWorker(false, std::make_exception_ptr(std::runtime_error("Remote journal commit failed")));
		failed += !c.paused_ || record["code"] != "local_remote_journal_write_failed" ||
			  record["paused"] != true || record["retryable"] != false;
		c.tick(); c.startStop();
		failed += c.id_ != original || c.released_ || c.syncAwaiting_;
		record = failWorker(true, std::make_exception_ptr(std::runtime_error("https://synthetic-secret/signed")));
		failed += !c.paused_ || record["code"] != "local_validation_failed" || record["stage"] != "control";
		record = failWorker(false, std::make_exception_ptr(RequestError(503, "capture_unavailable", 2,
			"0123456789abcdef0123456789abcdef")));
		failed += c.paused_ || record["status"] != 503 || record["retryable"] != true ||
			  record["requestId"] != "0123456789abcdef0123456789abcdef";
		record = failWorker(false, std::make_exception_ptr(RequestError(401, "unauthorized")));
		failed += !c.paused_ || c.released_ || c.id_ != original || record["status"] != 401;
		RequestError unknownAuthority(503, "capture_unavailable");
		unknownAuthority.outputReplayUnknown = true;
		for (bool control : {false, true}) {
			record = failWorker(control, std::make_exception_ptr(unknownAuthority));
			failed += !c.paused_ || c.released_ || c.id_ != original ||
				  record["pauseReason"] != "output_authority_unknown" || record["retryable"] != false;
		}
		record = failWorker(false, std::make_exception_ptr(RequestError(403, "https://synthetic-secret/signed", 0,
			"https://synthetic-secret/request")));
		failed += record["code"] != "request_failed" || record["requestId"] != "none" || !c.paused_;
		record = failWorker(true, std::make_exception_ptr(123));
		failed += !c.paused_ || record["category"] != "unknown_exception";
		const auto diagnosticPath = c.root_ + "/diagnostics/native/sync-error.json";
		QFile::remove(diagnosticPath); QDir().mkpath(diagnosticPath);
		c.sync_.setFuture(QtConcurrent::run([]() -> KeyJob { throw std::runtime_error("Queue document unavailable"); }));
		try { c.sync_.waitForFinished(); } catch (...) {}
		app.processEvents();
		failed += !c.paused_ || c.id_ != original || c.released_;
		c.closing_ = true; // Mock workers only; never launch a background recovery process.
		if (failed) std::cerr << "FAIL durable, privacy-safe worker diagnostics preserve pause and identity\n";
		return failed;
	}
	static int outputOptions()
	{
		QTemporaryDir root;
		KeyController c(root.path());
		c.timer_.stop();
		int failed = c.settingsModified() || c.outputOptions_[3]->isEnabled();
		int starts = 0, stops = 0;
		c.recording = [&](bool start) { start ? ++starts : ++stops; };
		c.recordWithStream(true); c.recordWithStream(false);
		failed += starts != 0 || stops != 0;
		c.outputOptions_[2]->setChecked(true);
		failed += !c.settingsModified() || !c.outputOptions_[3]->isEnabled();
		c.discardSettings();
		failed += c.settingsModified() || c.outputOptions_[3]->isEnabled();
		c.outputOptions_[2]->setChecked(true);
		failed += !c.applySettings() || c.settingsModified();
		c.recordWithStream(true); c.recordWithStream(false);
		failed += starts != 1 || stops != 1;
		c.outputOptions_[3]->setChecked(true);
		failed += !c.applySettings();
		c.recordWithStream(true); c.recordWithStream(false);
		failed += starts != 2 || stops != 1;
		c.outputOptions_[2]->setChecked(false);
		failed += !c.applySettings() || c.outputOptions_[3]->isEnabled();
		c.recordWithStream(true); c.recordWithStream(false);
		failed += starts != 2 || stops != 1;
		c.outputOptions_[0]->setChecked(true);
		c.outputOptions_[1]->setChecked(true);
		failed += !c.applySettings();
		c.view_->show();
		auto refuse = [] { QTimer::singleShot(0, [] {
			for (auto *w : QApplication::topLevelWidgets())
				if (auto *box = qobject_cast<QMessageBox *>(w)) box->button(QMessageBox::No)->click();
		}); };
		refuse(); c.startStop();
		failed += !c.id_.isEmpty() || c.starting_;
		c.starting_ = true;
		refuse(); c.startStop();
		failed += !c.starting_ || c.stopping_;
		c.starting_ = false;
		c.view_->hide();
		c.lease_->unlock();
		KeyController restored(root.path());
		restored.timer_.stop();
		failed += restored.savedOptions_[2] || !restored.savedOptions_[3] || restored.settingsModified();
		return failed;
	}
	static int unprovenReleasedPointer()
	{
		QTemporaryDir root;
		const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);
		QDir().mkpath(root.path()+"/k3/intents");
		// A pointer alone, even explicitly released, cannot erase missing original K4 evidence.
		writeAtomicMetadata(root.path()+"/k3/active.json",QJsonDocument(QJsonObject{
			{"localId",id},{"released",true}}).toJson());
		KeyController restored(root.path());restored.timer_.stop();
		int failed=restored.id_!=id || !restored.paused_ || restored.released_;
		failed += restored.retireUnbound(); // Old released pointers can omit protocolVersion, too.
		restored.keyAvailable_=true;restored.released_=true;restored.paused_=false;
		restored.startStop();
		failed+=restored.id_!=id || !restored.paused_ || restored.released_;
		restored.closing_=true;
		if (failed) std::cerr<<"FAIL released pointer without durable evidence must remain original and paused\n";
		return failed;
	}
	static int k4Recovery(KeyController &c)
	{
		c.timer_.stop();
		c.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		const auto original = c.id_;
		QMap<QString,QByteArray> keys{{"stream-key","hhc_stream_"+QByteArray(43,'A')}};
		IngestCredentials fixtureVault{[&](const QString &name)->std::optional<QByteArray> {
			return keys.contains(name) ? std::optional<QByteArray>(keys[name]) : std::nullopt;
		},[&](const QString &name,const QByteArray &value){keys[name]=value;}};
		KeyIngest(c.root_ + "/intents", original,fixtureVault).prepareOpen();
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
		// An error string alone is not authoritative terminal evidence.
		for (const int status : {403,409}) {
		c.sync_.setFuture(QtConcurrent::run([status]() -> KeyJob {
			RequestError e(status, "broadcast_output_terminal");
			e.outputOpening = true;
			throw e;
		}));
		try { c.sync_.waitForFinished(); } catch (...) {}
		c.syncDone();
		failed += c.released_ || !c.paused_ || c.action_->isEnabled();
		}
		// Replay error labels never reuse ordinary revocation/abort handling.
		c.sync_.setFuture(QtConcurrent::run([]()->KeyJob {
			RequestError e(403,"broadcast_ingest_revoked"); e.outputReplayUnknown=true; throw e;
		}));
		try { c.sync_.waitForFinished(); } catch (...) {}
		c.syncDone();
		failed += c.revoked_ || !c.paused_ || c.released_ || c.action_->isEnabled();
		failed += QFileInfo::exists(c.root_+"/intents/"+original+".revoked");
		int recorderActions = 0;
		c.recording = [&](bool) { ++recorderActions; };
		const auto sentinel = c.root_+"/original-stop-seal.json";
		writeAtomicMetadata(sentinel,"{\"stopIntent\":true,\"sealKey\":\"original.seal\"}");
		QFile::remove(c.root_+"/active.json");
		QDir().mkpath(c.root_+"/active.json"); // Simulate failure to commit the release pointer.
		c.sync_.setFuture(QtConcurrent::run([]()->KeyJob {
			RequestError e(0,"output_replay_terminal"); e.outputReleased=true; throw e;
		}));
		try { c.sync_.waitForFinished(); } catch (...) {}
		try { c.syncDone(); } catch (...) { ++failed; }
		failed += !c.paused_ || c.released_ || c.id_ != original || c.action_->isEnabled();
		QDir().rmdir(c.root_+"/active.json");
		c.paused_ = true; // Invalid-session retry had disabled Start before authoritative terminal.
		c.sync_.setFuture(QtConcurrent::run([]() -> KeyJob {
			RequestError e(0, "output_replay_terminal");
			e.outputReleased = true;
			throw e;
		}));
		try {
			c.sync_.waitForFinished();
		} catch (...) {
		}
		c.syncDone();
		failed += !c.released_ || c.starting_ || c.stopping_ || c.id_ != original;
		failed += c.paused_ || !c.action_->isEnabled() || !c.retry_->isHidden();
		failed += recorderActions != 0;
		QFile preserved(sentinel);
		failed += !preserved.open(QIODevice::ReadOnly) || preserved.readAll()!="{\"stopIntent\":true,\"sealKey\":\"original.seal\"}";
		c.recording = {};
		const auto future = c.next_;
		c.tick();
		failed += c.syncAwaiting_ || c.id_ != original || c.next_ != future;
		// A control-confirmed retirement is monotonic against a late upload success or error.
		for (bool error : {false,true}) {
			c.syncAwaiting_ = true;
			c.sync_.setFuture(QtConcurrent::run([error]()->KeyJob {
				if (error) throw RequestError(403,"forbidden");
				KeyJob r; r.sync.state="uploading"; return r;
			}));
			try { c.sync_.waitForFinished(); } catch (...) {}
			c.syncDone();
			failed += !c.released_ || c.paused_ || c.id_!=original || !c.action_->isEnabled();
		}
		c.terminalRetired_ = false; // The following synthetic state represents a different output.
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
		failed += !c.action_->property("class").toString().isEmpty();
		failed += c.action_->text() != QString::fromUtf8("停止 HHC 串流");
		c.starting_ = false;
		c.display();
		const auto originalStyle = qApp->styleSheet();
		qApp->setStyleSheet("QPushButton { background-color: #303030; } #streamButton.state-active { background-color: #2040c0; }");
		c.setStreamActive(true);
		c.action_->ensurePolished();
		failed += c.action_->palette().color(QPalette::Button) != QColor("#2040c0");
		c.setStreamActive(false);
		failed += c.action_->palette().color(QPalette::Button) != QColor("#303030");
		qApp->setStyleSheet(originalStyle);
		c.setStreamActive(true);
		failed += c.action_->property("class").toString() != "state-active";
		for (const auto code : {"local_session_busy", "transport_unavailable"}) {
			c.sync_.setFuture(QtConcurrent::run([code]() -> KeyJob { throw RequestError(0, code); }));
			try { c.sync_.waitForFinished(); } catch (...) {}
			c.syncDone();
			failed += c.paused_;
			failed += c.failures_ != (QString(code) == "local_session_busy" ? 0u : 1u);
		}
		c.failures_ = 0;
		c.gpuPercent_ = 42;
		c.display();
		failed += !c.action_->property("class").toString().isEmpty() || !c.status_->text().contains(QString::fromUtf8("GPU 編碼：42%"));
		c.gpuPercent_ = -1;
		c.display();
		failed += c.status_->text().contains("GPU: 0%");
		c.pendingBytes_ = 1048576;
		c.rateSamples_.clear();
		c.rateSamples_.emplace_back(c.rateClock_.elapsed() - 1000, 0);
		c.uploadedBytes_ = 5000000;
		c.display();
		failed += c.status_->text().contains("Mbps") || c.status_->text().contains("1.0 MB");
		failed += !c.status_->toolTip().contains("Mbps") || !c.status_->toolTip().contains("1.0 MB");
		// Simulate sustained pending work without sleeping or touching a real output.
		c.pendingSince_ = c.rateClock_.elapsed() - 60001;
		c.display();
		failed += !c.status_->text().contains("1.0 MB");
		c.pendingBytes_ = 0;
		c.display();
		failed += c.pendingSince_ != -1 || c.status_->text().contains(" MB");
		c.pendingBytes_ = 1048576;
		c.display();
		failed += c.status_->text().contains("1.0 MB");
		c.failures_ = 1;
		c.display();
		failed += !c.status_->text().contains("1.0 MB");
		c.failures_ = 0;
		c.id_ = "closing-test";
		c.display();
		failed += !c.status_->text().contains(QString::fromUtf8("補傳中"));
		c.pendingBytes_ = 0;
		c.display();
		failed += !c.status_->text().contains(QString::fromUtf8("收尾中")) || c.status_->text().contains(" MB");
		c.id_.clear();
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
	int failed = hhc::KeyControllerTest::outputOptions();
	failed += hhc::KeyControllerTest::pausedDiagnostics(app);
	failed += hhc::KeyControllerTest::unprovenReleasedPointer();
	const int gpu = hhc::sampleEncoderGpu();
	failed += gpu < -1 || gpu > 100;
	std::cout << "Video Encode sampler: " << gpu << " (-1 means unavailable)\n";
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
