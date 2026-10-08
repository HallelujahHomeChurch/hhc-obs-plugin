#include "capture-output.hpp"
#include "session-store.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <chrono>
#include <thread>
#include <iostream>
#include <util/platform.h>

int main(int argc, char **argv)
{
	QGuiApplication app(argc, argv);
	if (argc < 3) {
		std::cerr << "usage: obs-capture-test OUTPUT SECONDS\n";
		return 2;
	}
	const int seconds = QString(argv[2]).toInt();
	const bool missingNvenc = argc > 3 && QString(argv[3]) == "--missing-nvenc";
	const bool muxFailure = argc > 3 && QString(argv[3]) == "--mux-failure";
	if (seconds < 1 || seconds > 9000)
		return 2;
	const bool inventoryFailure = argc > 3 && QString(argv[3]) == "--journal-inventory-failure";
	const QString fault = argc > 3 ? QString(argv[3]) : QString();
	const bool externalStop = fault == "--external-stop";
	const bool runtimeFault = fault == "--low-disk" || fault == "--stop-timeout" || fault == "--header-timeout" ||
				  fault == "--media-stall" || externalStop;
	if (runtimeFault)
		qputenv("HHC_TEST_CAPTURE_FAULT", fault.toUtf8());
	const bool journal = runtimeFault || inventoryFailure || fault == "--journal" || fault == "--recover" ||
			     fault == "--recover-stopped";
	const QString requested = QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
	hhc::SessionStore store(requested);
	const QString out = journal ? store.mediaDirectory("synthetic-test-account", "capture-test") : requested;
	if (fault == "--recover" || fault == "--recover-stopped") {
		const auto account = argc > 4 ? QString::fromLocal8Bit(argv[4]) : QString("synthetic-test-account");
		const auto pending = store.loadPending(account);
		if (pending.size() != 1 || pending[0].objects.size() < 6 || pending[0].normalEnd ||
		    pending[0].stopIntent != (fault == "--recover-stopped") || pending[0].sealAcknowledged ||
		    pending[0].confirmedReady || (argc > 5 && pending[0].localId != QString::fromLocal8Bit(argv[5])))
			return 17;
		std::cout << "Recovered interrupted capture: " << pending[0].objects.size()
			  << " verified closed objects; not normal or published\n";
		return 0;
	}
	if (QFile::exists(out)) {
		std::cerr << "output must be new\n";
		return 2;
	}
	if (!obs_startup("en-US", nullptr, nullptr))
		return 3;
	obs_add_data_path("C:/Program Files/obs-studio/data/libobs/");
	obs_video_info vi{};
	vi.graphics_module = "C:/Program Files/obs-studio/bin/64bit/libobs-d3d11.dll";
	vi.adapter = 0;
	vi.base_width = vi.output_width = 1920;
	vi.base_height = vi.output_height = 1080;
	vi.fps_num = 30000;
	vi.fps_den = 1001;
	vi.output_format = VIDEO_FORMAT_NV12;
	vi.colorspace = VIDEO_CS_709;
	vi.range = VIDEO_RANGE_PARTIAL;
	vi.gpu_conversion = true;
	vi.scale_type = OBS_SCALE_BICUBIC;
	if (obs_reset_video(&vi) != OBS_VIDEO_SUCCESS)
		return 4;
	obs_audio_info ai{48000, SPEAKERS_STEREO};
	if (!obs_reset_audio(&ai))
		return 5;
	for (const char *name : {"obs-nvenc", "obs-ffmpeg", "obs-filters"}) {
		if (missingNvenc && QString(name) == "obs-nvenc")
			continue;
		obs_module_t *mod = nullptr;
		const auto dll = QString("C:/Program Files/obs-studio/obs-plugins/64bit/%1.dll").arg(name).toUtf8();
		const auto data = QString("C:/Program Files/obs-studio/data/obs-plugins/%1").arg(name).toUtf8();
		if (obs_open_module(&mod, dll.constData(), data.constData()) == MODULE_SUCCESS)
			obs_init_module(mod);
	}
	obs_post_load_modules();
	obs_source_info si{};
	si.id = "hhc_synthetic_test";
	si.type = OBS_SOURCE_TYPE_INPUT;
	si.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;
	si.get_name = [](void *) {
		return "HHC synthetic test";
	};
	si.create = [](obs_data_t *, obs_source_t *s) -> void * {
		return s;
	};
	si.destroy = [](void *) {
	};
	obs_register_source(&si);
	auto *source = obs_source_create_private(si.id, "Synthetic timecode", nullptr);
	auto *scene = obs_scene_create_private("Synthetic Program");
	obs_scene_add(scene, source);
	obs_set_output_source(0, obs_scene_get_source(scene));
	obs_source_set_audio_mixers(source, 1);
	hhc::CaptureOutput::registerOutput();
	int result = 0;
	{
		hhc::CaptureOutput capture;
		hhc::CaptureConfig config{out, 1};
		if (journal) {
			config.queueRoot = requested;
			config.account = "synthetic-test-account";
			config.localId = "capture-test";
		}
		if (!capture.start(config)) {
			if (missingNvenc) {
				int encoders = 0, outputs = 0;
				obs_enum_encoders(
					[](void *n, obs_encoder_t *) {
						++*static_cast<int *>(n);
						return true;
					},
					&encoders);
				obs_enum_outputs(
					[](void *n, obs_output_t *) {
						++*static_cast<int *>(n);
						return true;
					},
					&outputs);
				if (encoders || outputs || capture.active() || !capture.finished()) {
					std::cerr << "FAIL leaked failed-start resources\n";
					result = 11;
				}
			} else {
				std::cerr << "FAIL native output start: " << capture.error().toStdString() << '\n';
				result = 6;
			}
		} else if (runtimeFault) {
			std::this_thread::sleep_for(
				std::chrono::seconds(fault == "--header-timeout" || fault == "--media-stall" ? 13 : 3));
			if (externalStop)
				obs_enum_outputs(
					[](void *, obs_output_t *output) {
						obs_output_force_stop(output);
						return true;
					},
					nullptr);
			else
				capture.stop(hhc::StopReason::User);
			const bool ok = capture.wait(15000);
			const auto recovered = store.loadPending(config.account);
			QFile inv(out + "/inventory.json");
			if (ok || !capture.finished() || recovered.size() != 1 || recovered[0].normalEnd ||
			    !inv.open(QIODevice::ReadOnly)) {
				std::cerr << "FAIL runtime fault did not stop as incomplete\n";
				result = 21;
			} else {
				const auto evidence = QJsonDocument::fromJson(inv.readAll()).object();
				const auto expected = externalStop            ? hhc::StopReason::Shutdown
						      : fault == "--low-disk" ? hhc::StopReason::DiskLimit
									      : hhc::StopReason::EncoderFailure;
				if (evidence["normalEnd"].toBool() || evidence["stopReason"].toInt() != int(expected))
					result = 22;
			}
		} else if (inventoryFailure) {
			if (!QDir().mkpath(out + "/inventory.json"))
				return 19;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			capture.stop(hhc::StopReason::User);
			const bool ok = capture.wait(15000);
			const auto recovered = store.loadPending(config.account);
			if (ok || !capture.finished() || recovered.size() != 1 || recovered[0].normalEnd) {
				std::cerr << "FAIL final inventory failure declared successful journal\n";
				result = 20;
			}
		} else if (muxFailure) {
			for (unsigned i = 0; i < 100 && !QFile::exists(out + "/staging/1080p/init.mp4"); ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (!QDir().mkpath(out + "/staging/1080p/seg-000000.m4s.tmp"))
				return 14;
			capture.stop(hhc::StopReason::User);
			if (capture.wait(15000)) {
				std::cerr << "FAIL blocked segment became successful\n";
				result = 12;
			}
			if (!capture.finished()) {
				std::cerr << "FAIL failed capture never reports completion\n";
				result = 13;
			}
			QFile journal(out + "/inventory.json");
			if (!journal.open(QIODevice::ReadOnly)) {
				std::cerr << "FAIL missing failure evidence\n";
				result = 14;
			} else {
				const auto j = QJsonDocument::fromJson(journal.readAll()).object();
				if (j["normalEnd"].toBool() ||
				    j["stopReason"].toInt() != static_cast<int>(hhc::StopReason::EncoderFailure)) {
					std::cerr << "FAIL incorrect failure reason\n";
					result = 15;
				}
			}
		} else {
			QImage image(1920, 1080, QImage::Format_RGBA8888);
			std::array<float, 1602> audio{};
			uint64_t audioFrame = 0;
			const auto start = std::chrono::steady_clock::now();
			const uint64_t base = os_gettime_ns();
			for (int frame = 0; uint64_t(frame) * 1001 < uint64_t(seconds) * 30000 && capture.active();
			     ++frame) {
				image.fill(QColor::fromHsv((frame / 30) % 360, 180, 90));
				QPainter painter(&image);
				painter.setPen(Qt::white);
				painter.setFont(QFont("Consolas", 72));
				painter.drawText(100, 200, QString("HHC SYNTHETIC / PROGRAM"));
				const auto ms = uint64_t(frame) * 1001 / 30;
				painter.drawText(100, 400,
						 QString("%1:%2:%3.%4  frame %5")
							 .arg(ms / 3600000, 2, 10, QChar('0'))
							 .arg((ms / 60000) % 60, 2, 10, QChar('0'))
							 .arg((ms / 1000) % 60, 2, 10, QChar('0'))
							 .arg(ms % 1000, 3, 10, QChar('0'))
							 .arg(frame));
				painter.drawRect((frame * 7) % 1600, 600, 200, 200);
				painter.end();
				obs_source_frame vf{};
				vf.data[0] = image.bits();
				vf.linesize[0] = image.bytesPerLine();
				vf.width = 1920;
				vf.height = 1080;
				vf.format = VIDEO_FORMAT_RGBA;
				vf.timestamp = base + uint64_t(frame) * 1001000000ULL / 30;
				vf.full_range = true;
				obs_source_output_video(source, &vf);
				const uint64_t nextAudio = uint64_t(frame + 1) * 48000 * 1001 / 30000;
				obs_source_audio af{};
				af.data[0] = reinterpret_cast<const uint8_t *>(audio.data());
				af.data[1] = af.data[0];
				af.frames = static_cast<uint32_t>(nextAudio - audioFrame);
				af.speakers = SPEAKERS_STEREO;
				af.format = AUDIO_FORMAT_FLOAT_PLANAR;
				af.samples_per_sec = 48000;
				af.timestamp = base + audioFrame * 1000000000ULL / 48000;
				audioFrame = nextAudio;
				obs_source_output_audio(source, &af);
				std::this_thread::sleep_until(
					start + std::chrono::nanoseconds(uint64_t(frame + 1) * 1001000000ULL / 30));
			}
			capture.stop(hhc::StopReason::User);
			if (!capture.wait(30000)) {
				std::cerr << "FAIL output finalization " << capture.error().toStdString() << '\n';
				result = 7;
			}
			for (const char *rendition : {"1080p", "720p", "480p"}) {
				if (!QDir(out + "/staging/" + rendition).entryList({"seg-*.m4s"}, QDir::Files).empty()) {
					std::cerr << "FAIL closed media retains an unbudgeted staging copy\n";
					result = 23;
				}
				QFile playlist(out + "/" + rendition + "/index.m3u8");
				double total = 0;
				int count = 0;
				if (playlist.open(QIODevice::ReadOnly)) {
					const auto text = QString::fromUtf8(playlist.readAll());
					auto matches = QRegularExpression("#EXTINF:([0-9.]+)").globalMatch(text);
					while (matches.hasNext()) {
						total += matches.next().captured(1).toDouble();
						++count;
					}
				}
				if (total < seconds - 0.5 || total > seconds + 0.1 || (seconds == 61 && count != 3)) {
					std::cerr << "FAIL timeline " << rendition << " duration=" << total
						  << " segments=" << count << "\\n";
					result = 10;
				}
			}
			QFile inventory(out + "/inventory.json");
			if (journal) {
				const auto pending = store.loadPending(config.account);
				if (pending.size() != 1 || !pending[0].stopIntent || !pending[0].normalEnd ||
				    pending[0].objects.size() < 10 || pending[0].sealAcknowledged ||
				    pending[0].confirmedReady) {
					std::cerr << "FAIL capture not checkpointed into account journal\n";
					result = 18;
				}
			}
			if (!QFile::exists(out + "/master.m3u8")) {
				std::cerr << "FAIL missing master playlist\n";
				result = 16;
			}
			if (!inventory.open(QIODevice::ReadOnly)) {
				std::cerr << "FAIL missing inventory\n";
				result = 8;
			} else {
				auto j = QJsonDocument::fromJson(inventory.readAll()).object();
				if (!j["normalEnd"].toBool()) {
					std::cerr << "FAIL abnormal end\n";
					result = 9;
				}
			}
		}
	}
	obs_set_output_source(0, nullptr);
	obs_scene_release(scene);
	obs_source_release(source);
	obs_shutdown();
	return result;
}
