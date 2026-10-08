#include "local-controller.hpp"
#include "platform-controller.hpp"
#include <QApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QUuid>
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
			app.processEvents();
			check(c.state_.phase == Phase::Uploading && c.state_.pendingBytes == 321 && c.id_ == id,
			      "prior sync result is consumed for its original account and session");
			c.paused_ = true;
			c.state_.phase = Phase::Failed;
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
