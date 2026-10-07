#include "capture-output.hpp"
#include "local-controller.hpp"
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QElapsedTimer>
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QCoreApplication>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QComboBox>
#include <QMessageBox>
#include <QFile>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>
#include <atomic>
#include <thread>
#include <chrono>
#include <util/platform.h>
#include "hls-muxer.hpp"

// Developer fixture target only. Never compiled into the candidate plugin.
namespace {
std::unique_ptr<hhc::CaptureOutput> capture;
std::unique_ptr<hhc::LocalController> localController;
std::thread producer;
std::atomic<bool> running{false};
obs_source_t *source = nullptr;
obs_scene_t *scene = nullptr;
QString destination;
int duration = 0;
void finish()
{
	running = false;
	if (producer.joinable())
		producer.join();
	capture.reset();
	localController.reset();
	obs_frontend_set_current_scene(nullptr);
	if (scene) {
		obs_scene_release(scene);
		scene = nullptr;
	}
	if (source) {
		obs_source_release(source);
		source = nullptr;
	}
}
void begin()
{
	const QString base = QCoreApplication::applicationDirPath() + "/../../";
	if (!QFileInfo::exists(base + "portable_mode.txt") || obs_frontend_streaming_active() ||
	    obs_frontend_recording_active()) {
		blog(LOG_ERROR, "[HHC fixture] Refusing non-portable or already active OBS");
		return;
	}
	if (qEnvironmentVariableIsSet("HHC_FIXTURE_NATIVE_SMOKE")) {
		auto *window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
		auto *dock = window->findChild<QWidget *>("hhcCaptureDock");
		const bool absent = qEnvironmentVariableIsSet("HHC_FIXTURE_EXPECT_ABSENT");
		blog(LOG_INFO, "[HHC fixture] Native install smoke success=%s, dock=%s",
		     bool(dock) != absent ? "true" : "false", dock ? "present" : "absent");
		if (dock)
			dock->grab().save(qEnvironmentVariable("HHC_FIXTURE_OUTPUT") + ".png");
		QCoreApplication::quit();
		return;
	}
	destination = qEnvironmentVariable("HHC_FIXTURE_OUTPUT");
	duration = qEnvironmentVariableIntValue("HHC_FIXTURE_SECONDS");
	if (destination.isEmpty() || QFileInfo::exists(destination) || duration < 1 || duration > 9000)
		return;
	obs_source_info si{};
	si.id = "hhc_fixture_synthetic";
	si.type = OBS_SOURCE_TYPE_INPUT;
	si.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;
	si.get_name = [](void *) {
		return "HHC synthetic timecode";
	};
	si.create = [](obs_data_t *, obs_source_t *s) -> void * {
		return s;
	};
	si.destroy = [](void *) {
	};
	obs_register_source(&si);
	source = obs_source_create_private(si.id, "HHC synthetic Program", nullptr);
	scene = obs_scene_create_private("HHC synthetic scene");
	obs_scene_add(scene, source);
	obs_frontend_set_current_scene(obs_scene_get_source(scene));
	obs_source_set_audio_mixers(source, 1);
	running = true;
	producer = std::thread([] {
		QImage image(1920, 1080, QImage::Format_RGBA8888);
		std::array<float, 1602> samples{};
		const auto start = std::chrono::steady_clock::now();
		const auto ns = os_gettime_ns();
		uint64_t audioFrame = 0;
		for (uint64_t frame = 0; running; ++frame) {
			image.fill(QColor::fromHsv(static_cast<int>((frame / 30) % 360), 170, 100));
			QPainter p(&image);
			p.setPen(Qt::white);
			p.setFont(QFont("Consolas", 64));
			p.drawText(90, 180, "HHC SYNTHETIC / OBS PROGRAM");
			const auto ms = frame * 1001 / 30;
			p.drawText(90, 360,
				   QString("%1:%2:%3.%4")
					   .arg(ms / 3600000, 2, 10, QChar('0'))
					   .arg((ms / 60000) % 60, 2, 10, QChar('0'))
					   .arg((ms / 1000) % 60, 2, 10, QChar('0'))
					   .arg(ms % 1000, 3, 10, QChar('0')));
			p.drawText(90, 500, QString("30000/1001 fps | frame %1").arg(frame));
			p.drawRect(static_cast<int>((frame * 7) % 1600), 650, 200, 200);
			p.end();
			obs_source_frame vf{};
			vf.data[0] = image.bits();
			vf.linesize[0] = image.bytesPerLine();
			vf.width = 1920;
			vf.height = 1080;
			vf.format = VIDEO_FORMAT_RGBA;
			vf.full_range = true;
			vf.timestamp = ns + frame * 1001000000ULL / 30;
			obs_source_output_video(source, &vf);
			const uint64_t nextAudio = (frame + 1) * 48000 * 1001 / 30000;
			obs_source_audio af{};
			af.data[0] = reinterpret_cast<const uint8_t *>(samples.data());
			af.data[1] = af.data[0];
			af.frames = static_cast<uint32_t>(nextAudio - audioFrame);
			af.speakers = SPEAKERS_STEREO;
			af.format = AUDIO_FORMAT_FLOAT_PLANAR;
			af.samples_per_sec = 48000;
			af.timestamp = ns + audioFrame * 1000000000ULL / 48000;
			obs_source_output_audio(source, &af);
			audioFrame = nextAudio;
			std::this_thread::sleep_until(start +
						      std::chrono::nanoseconds((frame + 1) * 1001000000ULL / 30));
		}
	});
	if (qEnvironmentVariableIsSet("HHC_FIXTURE_NATIVEDRIVE")) {
		QDir().mkpath(destination);
		auto *watch = new QTimer(QCoreApplication::instance());
		watch->setInterval(250);
		struct Run {
			QElapsedTimer overall, recording;
			bool requested = false, started = false, stopped = false, offline = false, restored = false;
			int lastEvidence = -1;
			bool closeRequested = false, cancelRequested = false;
			int firstLiveMs = -1;
			QJsonArray trace;
			QTcpServer blackhole;
			QNetworkProxy previous;
		};
		;
		auto run = std::make_shared<Run>();
		run->overall.start();
		QObject::connect(watch, &QTimer::timeout, [watch, run] {
			auto *window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
			auto *dock = window->findChild<QWidget *>("hhcCaptureDock");
			if (!dock)
				return;
			auto *status = dock->findChild<QLabel *>("status");
			auto *warning = dock->findChild<QLabel *>("warning");
			auto *action = dock->findChild<QPushButton *>("action");
			if (!status || !action)
				return;
			auto recoverId = qEnvironmentVariable("HHC_FIXTURE_RECOVER_ID");
			if (!recoverId.isEmpty() && !run->requested &&
			    status->text() == QString::fromUtf8("準備收錄")) {
				auto *sessions = dock->findChild<QComboBox *>("recoverSession");
				auto index = sessions ? sessions->findData(recoverId) : -1;
				if (index >= 0) {
					sessions->setCurrentIndex(index);
					run->requested = true;
					run->stopped = true;
					QTimer::singleShot(50, QCoreApplication::instance(), [] {
						if (auto *box = qobject_cast<QMessageBox *>(
							    QApplication::activeModalWidget())) {
							if (box->text().contains(QString::fromUtf8("這場未正常完成")))
								box->button(QMessageBox::Yes)->click();
						}
					});
					dock->findChild<QPushButton *>("resumeSession")->click();
				}
			}
			if (recoverId.isEmpty() && !run->requested && status->text() == QString::fromUtf8("準備收錄") &&
			    action->isEnabled()) {
				dock->findChild<QLineEdit *>("title")->setText(
					QString("[HHC OBS SYNTHETIC TEST] %1 %2")
						.arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
						     qEnvironmentVariable("HHC_FIXTURE_CASE", "recording")));
				dock->findChild<QCheckBox *>("live")->setChecked(
					qEnvironmentVariableIsSet("HHC_FIXTURE_LIVE"));
				dock->findChild<QCheckBox *>("publish")->setChecked(
					qEnvironmentVariableIsSet("HHC_FIXTURE_PUBLISH"));
				action->click();
				run->requested = true;
				blog(LOG_INFO,
				     "[HHC fixture] Native platform recording requested; explicit exposure flags applied");
			}
			if (run->requested && !run->started && status->text() == QString::fromUtf8("收錄中")) {
				run->started = true;
				run->recording.start();
				dock->grab().save(destination + "/native-running.png");
				blog(LOG_INFO, "[HHC fixture] Native platform recording started");
			}
			if (run->started && !run->stopped && run->recording.elapsed() >= duration * 1000) {
				action->click();
				run->stopped = true;
				blog(LOG_INFO, "[HHC fixture] Native stop requested");
			}
			auto elapsed = run->started ? run->recording.elapsed() : 0;
			auto closeAt = qEnvironmentVariableIntValue("HHC_FIXTURE_CLOSE_LIVE_AFTER_SECONDS"),
			     cancelAt = qEnvironmentVariableIntValue("HHC_FIXTURE_CANCEL_PUBLISH_AFTER_SECONDS");
			if (run->started && closeAt > 0 && !run->closeRequested && elapsed >= closeAt * 1000) {
				auto *b = dock->findChild<QPushButton *>("closeLive");
				if (b && b->isEnabled()) {
					b->click();
					run->closeRequested = true;
				}
			}
			if (run->started && cancelAt > 0 && !run->cancelRequested && elapsed >= cancelAt * 1000) {
				auto *b = dock->findChild<QPushButton *>("cancelPublish");
				if (b && b->isEnabled()) {
					b->click();
					run->cancelRequested = true;
				}
			}

			auto offlineAt = qEnvironmentVariableIntValue("HHC_FIXTURE_OFFLINE_AFTER_SECONDS"),
			     offlineSeconds = qEnvironmentVariableIntValue("HHC_FIXTURE_OFFLINE_SECONDS");
			if (run->started && offlineSeconds > 0 && !run->offline && elapsed >= offlineAt * 1000) {
				run->previous = QNetworkProxy::applicationProxy();
				run->blackhole.listen(QHostAddress::LocalHost, 0);
				QObject::connect(&run->blackhole, &QTcpServer::newConnection, [run] {
					while (run->blackhole.hasPendingConnections()) {
						auto *s = run->blackhole.nextPendingConnection();
						s->abort();
						s->deleteLater();
					}
				});
				QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1",
										 run->blackhole.serverPort()));
				run->offline = true;
				blog(LOG_INFO, "[HHC fixture] Developer Qt transport offline fault active");
			}
			if (run->offline && !run->restored && elapsed >= (offlineAt + offlineSeconds) * 1000) {
				QNetworkProxy::setApplicationProxy(run->previous);
				run->blackhole.close();
				run->restored = true;
				blog(LOG_INFO, "[HHC fixture] Developer transport restored");
			}
			auto seconds = int(run->overall.elapsed() / 1000);
			if (seconds != run->lastEvidence) {
				run->lastEvidence = seconds;
				auto platformPath =
					QCoreApplication::applicationDirPath() +
					"/../../config/obs-studio/plugin_config/hhc-obs-plugin/platform/integration-status.json";
				QFile platform(platformPath);
				QJsonObject snapshot;
				if (platform.open(QIODevice::ReadOnly))
					snapshot = QJsonDocument::fromJson(platform.readAll()).object();
				if (run->started) {
					snapshot["recordingMs"] = elapsed;
					snapshot["issue"] = warning ? warning->text() : QString{};
					run->trace.append(snapshot);
					if (snapshot["liveState"] == "live" && run->firstLiveMs < 0)
						run->firstLiveMs = int(elapsed);
				}

				hhc::atomicJson(destination + "/progress.json",
						{{"status", status->text()},
						 {"issue", warning ? warning->text() : QString{}},
						 {"recordingMs", elapsed},
						 {"overallMs", run->overall.elapsed()},
						 {"offlineInjected", run->offline},
						 {"transportRestored", run->restored}});
			}
			const bool expectedAbort = !recoverId.isEmpty() &&
						   qEnvironmentVariableIsSet("HHC_FIXTURE_EXPECT_ABORT") && warning &&
						   warning->text().contains(QString::fromUtf8("伺服器狀態：aborted"));
			const bool complete = expectedAbort ||
					      run->stopped &&
						      (status->text().contains(QString::fromUtf8("草稿已就緒")) ||
						       status->text().contains(QString::fromUtf8("會後影片已發布")));
			const bool rejected = warning &&
					      ((warning->text().contains(QString::fromUtf8("伺服器狀態：aborted")) ||
						warning->text().contains(QString::fromUtf8("伺服器狀態：failed")) ||
						warning->text().contains(QString::fromUtf8("伺服器狀態：expired"))) ||
					       warning->text().contains("HTTP 400") ||
					       warning->text().contains("HTTP 403") ||
					       warning->text().contains("malformed_success") ||
					       warning->text().contains("account_mismatch"));
			if (complete || rejected || run->overall.elapsed() > (duration + 900) * 1000LL) {
				hhc::atomicJson(destination + "/native-dock-evidence.json",
						{{"complete", complete},
						 {"requested", run->requested},
						 {"started", run->started},
						 {"stopped", run->stopped},
						 {"status", status->text()},
						 {"issue", warning ? warning->text() : QString{}},
						 {"elapsedMs", run->overall.elapsed()},
						 {"trace", run->trace},
						 {"firstLiveMs", run->firstLiveMs},
						 {"closeLiveRequested", run->closeRequested},
						 {"cancelPublishRequested", run->cancelRequested},
						 {"recoveredLocalId", recoverId},
						 {"expectedAbort", expectedAbort},
						 {"liveIntent", qEnvironmentVariableIsSet("HHC_FIXTURE_LIVE")},
						 {"publishIntent", qEnvironmentVariableIsSet("HHC_FIXTURE_PUBLISH")}});
				dock->grab().save(destination + "/native-final.png");
				blog(LOG_INFO, "[HHC fixture] Native platform test complete=%s",
				     complete ? "true" : "false");
				if (run->offline && !run->restored)
					QNetworkProxy::setApplicationProxy(run->previous);
				watch->stop();
				watch->deleteLater();
				finish();
				QCoreApplication::quit();
			}
		});
		watch->start();
		return;
	}
	if (qEnvironmentVariableIsSet("HHC_FIXTURE_DOCK")) {
		localController = std::make_unique<hhc::LocalController>(destination);
		obs_frontend_add_dock_by_id("hhc.fixture.dock", "HHC 本機驗證", localController->view());
	}
	QTimer::singleShot(1500, QCoreApplication::instance(), [] {
		if (localController) {
			localController->view()->findChild<QPushButton *>("action")->click();
			if (!localController->busy()) {
				blog(LOG_ERROR, "[HHC fixture] Dock failed to start");
				finish();
				QCoreApplication::quit();
				return;
			}
			blog(LOG_INFO, "[HHC fixture] Real dock capture started for %d seconds", duration);
			localController->view()->grab().save(destination + "/dock-running.png");
			QTimer::singleShot(duration * 1000, QCoreApplication::instance(), [] {
				localController->view()->findChild<QPushButton *>("action")->click();
				auto *timer = new QTimer(QCoreApplication::instance());
				timer->setInterval(200);
				QObject::connect(timer, &QTimer::timeout, [timer] {
					if (localController->busy() ||
					    localController->phase() == hhc::Phase::StopPending)
						return;
					blog(LOG_INFO, "[HHC fixture] Dock complete (success=%s)",
					     localController->phase() == hhc::Phase::LocalComplete ? "true" : "false");
					localController->view()->grab().save(destination + "/dock-complete.png");
					timer->stop();
					timer->deleteLater();
					finish();
					QCoreApplication::quit();
				});
				timer->start();
			});
			return;
		}
		capture = std::make_unique<hhc::CaptureOutput>();
		if (!capture->start({destination, 1})) {
			blog(LOG_ERROR, "[HHC fixture] %s", capture->error().toUtf8().constData());
			finish();
			return;
		}
		blog(LOG_INFO, "[HHC fixture] Started real OBS frontend synthetic capture for %d seconds", duration);
		QTimer::singleShot(duration * 1000, QCoreApplication::instance(), [] {
			capture->stop(hhc::StopReason::User);
			auto *timer = new QTimer(QCoreApplication::instance());
			timer->setInterval(100);
			QObject::connect(timer, &QTimer::timeout, [timer] {
				if (!capture->finished())
					return;
				const bool ok = capture->wait(1);
				if (!ok)
					blog(LOG_ERROR, "[HHC fixture] Failed: %s",
					     capture->error().toUtf8().constData());
				timer->stop();
				timer->deleteLater();
				blog(LOG_INFO, "[HHC fixture] Capture finished (success=%s)", ok ? "true" : "false");
				finish();
				QTimer::singleShot(500, QCoreApplication::instance(), [] { QCoreApplication::quit(); });
			});
			timer->start();
		});
	});
}
void event(obs_frontend_event e, void *)
{
	if (e == OBS_FRONTEND_EVENT_FINISHED_LOADING && qEnvironmentVariableIsSet("HHC_FIXTURE_OUTPUT"))
		QTimer::singleShot(1500, QCoreApplication::instance(), begin);
	if (e == OBS_FRONTEND_EVENT_EXIT) {
		if (capture)
			capture->stop(hhc::StopReason::Shutdown);
		finish();
	}
}
} // namespace
void registerFixtureController()
{
	obs_frontend_add_event_callback(event, nullptr);
}
