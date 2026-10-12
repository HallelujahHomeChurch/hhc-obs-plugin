#include "local-controller.hpp"
#include "platform-controller.hpp"
#include "hls-muxer.hpp"
#include "windows-path.hpp"
#include "async.hpp"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QCryptographicHash>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>

namespace hhc {
struct ControllerLifecycleTest {
	static int run(QApplication &app)
	{
		int failures = 0;
		auto check = [&](bool ok, const char *name) {
			if (!ok) {
				std::cerr << "FAIL " << name << '\n';
				++failures;
			}
		};
		auto drain = [&] {
			check(QThreadPool::globalInstance()->waitForDone(5000), "worker completed without GUI events");
			app.processEvents();
		};
		QTemporaryDir evidenceRoot;
		const auto evidencePath = evidenceRoot.path() + "/progress.json";
		atomicJson(evidencePath, {{"recordingMs", 1000}});
		const auto nativePath = lockFilePath(evidencePath);
		HANDLE held = CreateFileW(reinterpret_cast<LPCWSTR>(nativePath.utf16()), GENERIC_READ, FILE_SHARE_READ,
					  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		check(held != INVALID_HANDLE_VALUE, "observer holds fixture progress without delete sharing");
		bool escaped = false, written = true, criticalFailed = false;
		try {
			written = tryObserverJson(evidencePath, {{"recordingMs", 2000}});
		} catch (const std::exception &) {
			escaped = true;
		}
		check(!escaped && !written, "observer sharing denial cannot escape the fixture timer");
		try {
			atomicJson(evidencePath, {{"recordingMs", 3000}});
		} catch (const std::exception &) {
			criticalFailed = true;
		}
		check(criticalFailed, "required journal writes still fail closed on sharing denial");
		QFile previous(evidencePath);
		check(previous.open(QIODevice::ReadOnly) &&
			      QJsonDocument::fromJson(previous.readAll()).object()["recordingMs"].toInt() == 1000,
		      "failed telemetry/journal replacement preserves the prior file");
		previous.close();
		if (held != INVALID_HANDLE_VALUE)
			CloseHandle(held);
		check(tryObserverJson(evidencePath, {{"recordingMs", 4000}}),
		      "fixture telemetry recovers when the observer releases its file");
		held = CreateFileW(reinterpret_cast<LPCWSTR>(nativePath.utf16()), GENERIC_READ,
				   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
				   FILE_ATTRIBUTE_NORMAL, nullptr);
		check(held != INVALID_HANDLE_VALUE, "observer opens a shared metadata snapshot");
		check(writeAtomicMetadata(evidencePath, QJsonDocument(QJsonObject{{"recordingMs", 5000}}).toJson()),
		      "delete-sharing observer does not block authoritative metadata replacement");
		if (held != INVALID_HANDLE_VALUE)
			CloseHandle(held);
		QTemporaryDir localRoot;
		{
			LocalController c(localRoot.path());
			drain();
			c.timer_.stop();
			// An unstarted output is already finished and unsuccessful. No GPU/encoder runs.
			c.capture_ = std::make_unique<CaptureOutput>();
			c.state_.phase = Phase::Capturing;
			c.view_->apply(c.state_);
			c.view_->onAction();
			check(c.state_.phase == Phase::Failed && c.state_.issue.isEmpty(),
			      "stale local stop reconciles finished output without another start");
		}
		QTemporaryDir platformRoot;
		QFile selection(platformRoot.path() + "/active-account");
		check(selection.open(QIODevice::WriteOnly), "explicit signed-out isolated selection");
		selection.close();
		{
			PlatformController c(platformRoot.path());
			drain(); // Empty selection refuses resume before any vault read or HTTP.
			c.timer_.stop();
			const auto account = QUuid::createUuid().toString(QUuid::WithoutBraces);
			c.auth_.token_.account = account;
			c.auth_.token_.access = "synthetic-controller-never-sent";
			c.auth_.expiry_ = std::chrono::steady_clock::now() + std::chrono::hours(1);
			c.auth_.token_.scopes.insert("cms:recordings:write");
			c.state_.account = account;
			c.state_.connected = true;
			c.state_.phase = Phase::Capturing;
			c.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
			c.owner_ = account;
			c.paused_ = true;    // This terminal/paused capture must never initiate HTTP.
			c.cancelled_ = true; // Also fence an accidental start on the unfixed source before HTTP.
			c.capture_ = std::make_unique<CaptureOutput>();
			const auto id = c.id_;
			c.view_->apply(c.state_);
			c.view_->onAction();
			check(c.id_ == id && !c.creating_ && c.state_.phase == Phase::Failed,
			      "stale platform stop retains original capture and reconciles its result");
			try {
				c.job_.waitForFinished(); // An accidental create cancels before HTTP on unfixed source.
			} catch (...) {
			}
			app.processEvents();
			c.id_.clear();
			c.capture_.reset();
			c.creating_ = false;
			c.paused_ = true;
			c.auth_.token_.account = account;
			c.state_.account = account;
			c.state_.phase = Phase::Failed;
			c.cancelled_ = false;
			c.refresh(); // Real scan worker; its result remains queued in Qt.
			check(QThreadPool::globalInstance()->waitForDone(5000) && !c.job_.isRunning(),
			      "platform scan finished before result consumption");
			c.view_->onLogout();
			check(c.auth_.account() == account, "pending platform result blocks account logout");
			app.processEvents();
			c.id_ = id;
			c.owner_ = account;
			c.paused_ = false;
			c.cancelled_ = true;
			c.launch([] {
				PlatformJob result;
				result.sync.state = "capturing";
				result.sync.pendingBytes = 321;
				return result;
			});
			check(QThreadPool::globalInstance()->waitForDone(5000) && !c.job_.isRunning(),
			      "sync result completed while its GUI handler is withheld");
			c.refresh();
			c.submit();
			c.poll();
			c.recover("absent");
			c.view_->onLogout();
			check(c.job_.future().isFinished() && c.job_.future().result().sync.pendingBytes == 321 &&
				      c.auth_.account() == account && c.id_ == id,
			      "pending sync result blocks account change and future replacement on all routes");
			const auto statusPath = platformRoot.path() + "/integration-status.json";
			atomicJson(statusPath, {{"previous", true}});
			const auto nativeStatusPath = lockFilePath(statusPath);
			HANDLE statusReader = CreateFileW(reinterpret_cast<LPCWSTR>(nativeStatusPath.utf16()),
							  GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL, nullptr);
			check(statusReader != INVALID_HANDLE_VALUE, "observer holds optional platform status");
			app.processEvents();
			check(c.state_.phase == Phase::Uploading && c.state_.pendingBytes == 321 && c.id_ == id,
			      "prior sync result is consumed for its original account and session");
			check(!c.paused_ && c.state_.issue.isEmpty(), "observer cannot pause authoritative sync");
			if (statusReader != INVALID_HANDLE_VALUE)
				CloseHandle(statusReader);
			QFile b1Fixture("tests/fixtures/b1.json");
			check(b1Fixture.open(QIODevice::ReadOnly), "complete B1 publication fixture");
			QJsonObject boundBroadcast;
			for (const auto &entry : QJsonDocument::fromJson(b1Fixture.readAll()).array())
				if (entry.toObject()["name"] == "view_draft")
					boundBroadcast = entry.toObject()["value"].toObject()["data"].toObject();
			check(!boundBroadcast.isEmpty(), "B1 draft fixture was found");
			boundBroadcast["binding"] = QJsonObject{{"captureId", QString(32, 'a')}, {"epoch", 1}, {"actorId", account}};
			c.broadcastId_ = boundBroadcast["recordingId"].toString();
			c.state_.broadcastBound = true;
			for (bool publish : {true, false}) {
				auto policy = boundBroadcast["policy"].toObject();
				policy["autoPublish"] = publish;
				boundBroadcast["policy"] = policy;
				c.launch([boundBroadcast] {
					PlatformJob result;
					result.broadcast = boundBroadcast;
					result.sync.state = "uploading";
					result.sync.autoPublish = "cancelled";
					return result;
				});
				drain();
				check(c.view_->selectedPublish() == publish,
				      "bound OBS publication display follows Console policy despite C1 bridge cancellation");
				c.launch([] {
					PlatformJob result;
					result.sync.state = "uploading";
					result.sync.autoPublish = "cancelled";
					return result;
				});
				drain();
				check(c.view_->selectedPublish() == publish,
				      "later C1-only synchronization retains the bound Console publication choice");
			}
			c.broadcastId_.clear();
			c.state_.broadcastBound = false;
			c.state_.autoPublish = true;
			c.state_.cancelPending = true;
			c.launch([] {
				PlatformJob result;
				result.sync.state = "uploading";
				result.sync.autoPublish = "cancelled";
				return result;
			});
			drain();
			check(!c.view_->selectedPublish() && !c.state_.cancelPending,
			      "standalone C1 cancellation still clears its own publication intent");
			for (const auto &error : {RequestError(410, "capture_expired"), RequestError(403, "capture_forbidden"),
						  RequestError(503, "capture_unavailable")}) {
				c.state_.terminalFailure = false;
				c.state_.phase = Phase::Capturing;
				c.paused_ = false;
				c.launch([error]() -> PlatformJob { throw error; });
				drain();
				check(c.state_.terminalFailure == (error.code == "capture_expired") && c.id_ == id,
				      "terminal capture HTTP error is distinguished from permission and temporary failures");
				QFile requestEvidence(platformRoot.path() + "/sync-http-error.json");
				check(requestEvidence.open(QIODevice::ReadOnly), "HTTP pause has diagnostic evidence");
				const auto requestDiagnostic = QJsonDocument::fromJson(requestEvidence.readAll()).object();
				check(requestDiagnostic["localId"] == id && requestDiagnostic["status"] == error.status &&
					      requestDiagnostic["code"] == error.code &&
					      requestDiagnostic["paused"].toBool() == c.paused_,
				      "HTTP evidence identifies original session, error and retry decision");
			}
			// Reject a corrupt retained journal before any HTTP; a retry must use this same session.
			const auto retryDir = SessionStore(platformRoot.path() + "/queue").mediaDirectory(account, id);
			QDir().mkpath(retryDir);
			atomicJson(retryDir + "/remote-journal.json", {{"invalid", true}});
			c.paused_ = true;
			c.state_.terminalFailure = false;
			c.state_.phase = Phase::Capturing;
			c.state_.issue = "retained failure";
			c.failures_ = 5;
			c.cancelled_ = false; // Both corrupt journals reject before any HTTP.
			c.refresh();
			check(!c.paused_ && c.failures_ == 0 && c.id_ == id && !c.creating_ && c.owner_ == account,
			      "operator recheck resumes the paused original capture without creating another");
			check(QThreadPool::globalInstance()->waitForDone(5000), "retained journal retry completes");
			bool localRejected = false;
			try {
				workerResult(c.job_.future());
			} catch (const std::exception &e) {
				localRejected = QByteArray(e.what()) == "Local recovery validation failed; files retained for inspection";
			}
			check(localRejected, "paused retry rejects missing local journal before remote sync or HTTP");
			drain();
			check(c.paused_ && c.id_ == id && !c.state_.issue.isEmpty(),
			      "retry revalidates retained media and fails closed on a corrupt journal");
			CaptureJournal saved;
			saved.account = account;
			saved.localId = id;
			SessionStore retryStore(platformRoot.path() + "/queue");
			retryStore.save(saved);
			QFile localRetry(retryDir + "/journal.json");
			check(localRetry.open(QIODevice::ReadOnly), "read typed retry checkpoint");
			const auto typedLocal = QJsonDocument::fromJson(localRetry.readAll()).object();
			localRetry.close();
			for (const auto &invalidEnd : {QJsonValue(), QJsonValue("true")}) {
				auto damaged = typedLocal;
				damaged["stopIntent"] = true;
				if (invalidEnd.isNull())
					damaged.remove("normalEnd");
				else
					damaged["normalEnd"] = invalidEnd;
				atomicJson(retryDir + "/journal.json", damaged);
				c.paused_ = true;
				c.refresh();
				check(QThreadPool::globalInstance()->waitForDone(5000), "damaged Stop retry completes");
				bool refusedBeforeSync = false;
				try { workerResult(c.job_.future()); } catch (const std::exception &e) {
					refusedBeforeSync = QByteArray(e.what()) == "Local recovery validation failed; files retained for inspection";
				}
				check(refusedBeforeSync, "damaged normalEnd never turns a retry into an abort request");
				drain();
			}
			atomicJson(retryDir + "/journal.json", typedLocal);
			c.paused_ = true;
			c.refresh();
			check(QThreadPool::globalInstance()->waitForDone(5000), "typed checkpoint retry completes");
			bool reachedOriginalRemote = false;
			try { workerResult(c.job_.future()); } catch (const std::exception &e) {
				reachedOriginalRemote = QByteArray(e.what()) == "Unknown remote journal field";
			}
			check(reachedOriginalRemote && c.id_ == id,
			      "valid local checkpoint proceeds to validation of the same remote journal");
			drain();
			QDir().mkpath(retryDir + "/480p");
			QFile retainedMedia(retryDir + "/480p/init.mp4");
			check(retainedMedia.open(QIODevice::WriteOnly) && retainedMedia.write("aaa") == 3,
			      "create immutable retry media fixture");
			retainedMedia.close();
			saved.objects.append({"480p/init.mp4", 3,
				QString::fromLatin1(QCryptographicHash::hash("aaa", QCryptographicHash::Sha256).toHex()), false});
			retryStore.save(saved);
			check(retainedMedia.open(QIODevice::WriteOnly) && retainedMedia.write("bbb") == 3,
			      "change retained media without changing its size");
			retainedMedia.close();
			c.paused_ = true;
			c.refresh();
			check(QThreadPool::globalInstance()->waitForDone(5000), "changed media retry completes");
			bool changedMediaRejected = false;
			try { workerResult(c.job_.future()); } catch (const std::exception &e) {
				changedMediaRejected = QByteArray(e.what()) == "Local recovery validation failed; files retained for inspection";
			}
			check(changedMediaRejected, "retry hashes retained media before HTTP, even for equal-length corruption");
			drain();
			c.owner_ = "another-account";
			c.refresh();
			check(c.paused_ && c.id_ == id, "recheck never resumes another account's retained capture");
			drain();
			c.owner_ = account;
			c.state_.terminalFailure = true;
			c.refresh();
			check(c.paused_ && c.id_ == id, "recheck never resumes a terminal capture");
			drain();
			c.state_.terminalFailure = false;
			QFile::remove(retryDir + "/remote-journal.json");
			for (const auto &failure : {
				 std::pair<const char *, const char *>{"Remote journal commit failed", "local_remote_journal_write_failed"},
				 {"Queue document unavailable", "local_queue_unavailable"},
				 {"Closed upload bytes changed", "local_media_changed"},
				 {"Remote immutable object differs", "local_remote_media_conflict"},
				 {"https://example.invalid/upload?signature=synthetic-secret", "local_sync_validation_failed"}}) {
				c.paused_ = false;
				c.launch([message = failure.first]() -> PlatformJob { throw std::runtime_error(message); });
				drain();
				QFile diagnostics(platformRoot.path() + "/sync-error.json");
				check(diagnostics.open(QIODevice::ReadOnly), "safe sync error evidence saved");
				const auto bytes = diagnostics.readAll();
				const auto diagnostic = QJsonDocument::fromJson(bytes).object();
				check(c.paused_ && c.state_.issue.contains(failure.second) &&
					      diagnostic["code"] == failure.second && diagnostic["localId"] == id &&
					      !bytes.contains("synthetic-secret") && !c.state_.issue.contains("synthetic-secret"),
				      "local failures expose only fixed diagnostic codes and retain the original session");
			}
			c.paused_ = true;
			c.state_.phase = Phase::Failed;
			c.broadcastId_ = "018f0c1f-18d0-7e81-9f6f-69c456db7003";
			c.controlAwaiting_ = true;
			c.controlJob_.setFuture(
			    QtConcurrent::run([] { return QJsonObject{{"phase", "end_pending"}}; }));
			check(QThreadPool::globalInstance()->waitForDone(5000),
			      "control worker completes before Qt consumption");
			c.view_->onLogout();
			check(c.auth_.account() == account && c.controlBusy(),
			      "pending control result fences logout");
			app.processEvents();
			check(!c.controlBusy() && c.state_.broadcastPhase == "end_pending" &&
				  c.state_.phase == Phase::Failed,
			      "Console End does not change capture phase or stop outputs");
			const auto priorControl = c.controlJob_.future();
			c.pollControl();
			check(!c.controlBusy() && c.controlJob_.future().isFinished() && c.controlJob_.future().result() == priorControl.result(),
			      "inactive capture does not launch control polls");
			CaptureJournal recovery;
			recovery.account = account;
			recovery.localId = "corrupt-binding";
			recovery.normalEnd = true;
			c.recoveryCache_.sessions.append(recovery);
			const auto corruptDir = SessionStore(platformRoot.path() + "/queue")
						    .mediaDirectory(account, recovery.localId);
			QDir().mkpath(corruptDir);
			atomicJson(corruptDir + "/broadcast-journal.json", {{"recordingId", ""}});
			const auto previousId = c.id_;
			c.recover(recovery.localId);
			check(c.id_ == previousId && !c.jobBusy(),
			      "corrupt B1 recovery never falls back to another C1 capture");
			recovery.localId = "bound-recovery";
			c.recoveryCache_.sessions.append(recovery);
			const auto boundDir = SessionStore(platformRoot.path() + "/queue")
						 .mediaDirectory(account, recovery.localId);
			QDir().mkpath(boundDir);
			atomicJson(boundDir + "/broadcast-journal.json",
				   {{"account", account}, {"localId", recovery.localId}, {"protocolVersion", b1Version},
				    {"recordingId", boundBroadcast["recordingId"]}});
			c.broadcastId_.clear();
			c.state_.broadcastBound = false;
			c.state_.cancelPending = true;
			c.cancelled_ = true; // Fence the synthetic recovery before any HTTP.
			c.recover(recovery.localId);
			check(c.id_ == recovery.localId && c.state_.broadcastBound && !c.state_.cancelPending,
			      "adopting B1 recovery clears another session's cancellation warning");
			drain();
			std::function<std::optional<int>()> clock;
			{
				CaptureOutput output;
				clock = output.boundaryClock();
				check(!clock(), "no marker before encoder progress");
			}
			check(!clock(), "expired encoder clock cannot select a recovery marker");
			c.view_->onLogout();
			check(c.auth_.account().isEmpty() && c.state_.phase == Phase::Unavailable,
			      "consumed platform result permits stable signed-out state");
		}
		return failures;
	}
};
} // namespace hhc

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	// Initialize only the core so an unintended start safely reports video unavailable.
	if (!obs_startup("en-US", nullptr, nullptr))
		return 2;
	const auto failures = hhc::ControllerLifecycleTest::run(app);
	obs_shutdown();
	return failures ? 1 : 0;
}
