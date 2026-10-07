#include "capture-output.hpp"
#include "hls-muxer.hpp"
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QStorageInfo>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <chrono>

namespace hhc {
struct CaptureOutput::Impl {
	obs_output_t *output = nullptr;
	std::array<obs_encoder_t *, 3> video{};
	obs_encoder_t *audio = nullptr;
	std::array<std::unique_ptr<HlsMuxer>, 3> mux;
	CaptureConfig config;
	std::thread worker;
	mutable std::mutex mutex;
	std::condition_variable cv;
	std::deque<encoder_packet> packets;
	size_t bytes = 0;
	std::atomic<unsigned> seenVideo{0};
	std::atomic<bool> failed{false}, done{true}, deactivated{false}, stopRequested{false};
	std::atomic<StopReason> reason{StopReason::Shutdown};
	std::atomic<int64_t> firstVideoSys{0}, cutoffPts{0};
	std::atomic<int64_t> stopDeadline{0};
	std::atomic<unsigned> endedVideo{0};
	QString error;
	void fail(const char *message)
	{
		auto previous = reason.load();
		while (!reason.compare_exchange_weak(previous, failureReason(previous, StopReason::EncoderFailure))) {
		}
		failed = true;
		{
			std::lock_guard lock(mutex);
			error = QString::fromUtf8(message);
		}
		cv.notify_all();
	}
	void release()
	{
		if (output) {
			obs_output_release(output);
			output = nullptr;
		}
		for (auto *&e : video) {
			if (e)
				obs_encoder_release(e);
			e = nullptr;
		}
		if (audio)
			obs_encoder_release(audio);
		audio = nullptr;
	}
	~Impl() { release(); }
	static void *create(obs_data_t *settings, obs_output_t *output)
	{
		auto *s = reinterpret_cast<Impl *>(static_cast<uintptr_t>(obs_data_get_int(settings, "owner")));
		s->output = output;
		return s;
	}
	static bool begin(void *data)
	{
		auto &s = *static_cast<Impl *>(data);
		if (!obs_output_can_begin_data_capture(s.output, 0) || !obs_output_initialize_encoders(s.output, 0)) {
			s.fail("NVENC/AAC initialization failed; no CPU fallback");
			return false;
		}

		s.done = false;
		s.worker = std::thread([&s] { s.run(); });
		if (!obs_output_begin_data_capture(s.output, 0)) {
			s.fail("OBS begin capture failed");
			s.deactivated = true;
			s.cv.notify_all();
			return false;
		}
		return true;
	}
	static void end(void *data, uint64_t ts)
	{
		auto &s = *static_cast<Impl *>(data);
		s.stopRequested = true;
		s.stopDeadline = std::chrono::steady_clock::now().time_since_epoch().count();
		if (ts && s.firstVideoSys > 0)
			s.cutoffPts =
				((static_cast<int64_t>(ts / 1000) - s.firstVideoSys) * 30000 / (1000000LL * 1001)) *
				1001;
		else
			obs_output_end_data_capture(s.output);
	}
	static void deactivate(void *data, calldata_t *)
	{
		auto &s = *static_cast<Impl *>(data);
		s.deactivated = true;
		s.cv.notify_all();
	}
	static void packet(void *data, encoder_packet *p)
	{
		auto &s = *static_cast<Impl *>(data);
		if (!p) {
			s.fail("OBS encoder failed");
			return;
		}
		if (s.failed)
			return;
		if (p->type == OBS_ENCODER_VIDEO && p->track_idx == 0 && s.firstVideoSys == 0)
			s.firstVideoSys = p->sys_dts_usec;
		if (s.cutoffPts > 0 && p->pts * 30000 >= s.cutoffPts * p->timebase_den) {
			if (p->type == OBS_ENCODER_VIDEO && p->track_idx < 3 &&
			    (s.endedVideo.fetch_or(1U << p->track_idx) | (1U << p->track_idx)) == 7)
				obs_output_end_data_capture(s.output);
			return;
		}
		if (p->type == OBS_ENCODER_VIDEO && p->track_idx < 3)
			s.seenVideo.fetch_or(1U << p->track_idx);
		{
			std::lock_guard lock(s.mutex);
			if (p->size > 64ULL * 1024 * 1024 - s.bytes) {
				s.reason = failureReason(s.reason, StopReason::QueueLimit);
				s.failed = true;
				s.error = "Encoded packet queue exceeded 64 MiB";
			} else {
				encoder_packet copy{};
				obs_encoder_packet_ref(&copy, p);
				s.packets.push_back(copy);
				s.bytes += p->size;
			}
		}
		s.cv.notify_all();
	}
	void run()
	{
		bool stopping = false;
		const auto began = std::chrono::steady_clock::now();
		auto lastDiskCheck = began;
		uint64_t encodedBytes = 0;
		try {
			while (!failed && seenVideo != 7 && !deactivated) {
				if (std::chrono::steady_clock::now() - began > std::chrono::seconds(10))
					throw std::runtime_error(
						"NVENC produced no complete header set within 10 seconds");
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			if (seenVideo != 7)
				throw std::runtime_error("All three video headers required");
			for (unsigned i = 0; i < 3; ++i)
				mux[i] = std::make_unique<HlsMuxer>(config.directory, i, video[i], audio);
			while (true) {
				const auto now = std::chrono::steady_clock::now();
				if (stopDeadline > 0 &&
				    now - std::chrono::steady_clock::time_point(
						  std::chrono::steady_clock::duration(stopDeadline.load())) >
					    std::chrono::seconds(5) &&
				    !deactivated)
					throw std::runtime_error("Encoder stop timed out; capture incomplete");
				if (!failed && now - lastDiskCheck >= std::chrono::seconds(1)) {
					lastDiskCheck = now;
					QStorageInfo disk(config.directory);
					unsigned count = 0;
					for (auto &m : mux)
						if (m)
							count += static_cast<unsigned>(m->objects().size());
					const auto limit = runtimeLimit(
						static_cast<uint64_t>(std::max<qint64>(0, disk.bytesAvailable())),
						encodedBytes, count,
						std::chrono::duration<double>(now - began).count());
					if (limit) {
						reason = *limit;
						fail("Local capture quota or disk reserve reached");
					}
				}
				if (failed && !stopping) {
					stopping = true;
					obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
				}
				encoder_packet p{};
				{
					std::unique_lock lock(mutex);
					cv.wait_for(lock, std::chrono::milliseconds(100), [&] {
						return !packets.empty() || deactivated || (failed && !stopping);
					});
					if (packets.empty()) {
						if (deactivated)
							break;
						continue;
					}
					p = packets.front();
					packets.pop_front();
					bytes -= p.size;
				}
				encodedBytes += p.size * (p.type == OBS_ENCODER_AUDIO ? 3 : 1);
				try {
					if (!failed) {
						if (p.type == OBS_ENCODER_VIDEO) {
							if (p.track_idx >= 3)
								throw std::runtime_error("invalid video track");
							mux[p.track_idx]->write(p);
						} else
							for (auto &m : mux)
								m->write(p);
					}
				} catch (...) {
					obs_encoder_packet_release(&p);
					throw;
				}
				obs_encoder_packet_release(&p);
			}
		} catch (const std::exception &e) {
			fail(e.what());
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
		}
		while (!deactivated)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::array<bool, 3> closed{};
		for (unsigned i = 0; i < 3; ++i) {
			try {
				if (!failed) {
					mux[i]->close();
					closed[i] = true;
				}
			} catch (const std::exception &e) {
				fail(e.what());
			}
		}
		QJsonArray inventory;
		for (auto &m : mux)
			if (m)
				for (const auto &o : m->objects())
					inventory.append(o);
		try {
			if (hhc::normalEnd(reason.load(), closed, failed))
				inventory.append(finalizeMaster(config.directory));
		} catch (const std::exception &e) {
			fail(e.what());
		}
		try {
			atomicJson(config.directory + "/inventory.json",
				   {{"localFormat", "hhc-local-media-v1"},
				    {"wireContract", QJsonValue::Null},
				    {"normalEnd", hhc::normalEnd(reason.load(), closed, failed)},
				    {"stopReason", static_cast<int>(reason.load())},
				    {"objects", inventory},
				    {"fpsNum", 30000},
				    {"fpsDen", 1001},
				    {"audioTrack", static_cast<int>(config.audioTrack)},
				    {"sampleRate", 48000},
				    {"audioKbps", 128},
				    {"encoder", "obs_nvenc_h264_tex"},
				    {"preset", "p5"},
				    {"rateControl", "CBR"},
				    {"keyintFrames", 60},
				    {"bframes", 0},
				    {"obsVersion", QString::fromUtf8(obs_get_version_string())}});
		} catch (const std::exception &e) {
			fail(e.what());
		}
		for (auto &m : mux)
			m.reset();
		{
			std::lock_guard lock(mutex);
			for (auto &p : packets)
				obs_encoder_packet_release(&p);
			packets.clear();
			bytes = 0;
		}
		done = true;
		cv.notify_all();
	}
};
CaptureOutput::CaptureOutput() : d(std::make_unique<Impl>()) {}
CaptureOutput::~CaptureOutput()
{
	if (d->output && obs_output_active(d->output))
		stop(StopReason::Shutdown);
	if (d->worker.joinable())
		d->worker.join();
}
void CaptureOutput::registerOutput()
{
	obs_output_info i{};
	i.id = "hhc_hls_output";
	i.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_VIDEO;
	i.get_name = [](void *) {
		return "HHC independent HLS output";
	};
	i.create = Impl::create;
	i.destroy = [](void *) {
	};
	i.start = Impl::begin;
	i.stop = Impl::end;
	i.encoded_packet = Impl::packet;
	i.encoded_video_codecs = "h264";
	i.encoded_audio_codecs = "aac";
	obs_register_output(&i);
}
bool CaptureOutput::start(const CaptureConfig &config)
{
	if (d->output) {
		d->fail("CaptureOutput is single-use");
		return false;
	}
	obs_video_info vi{};
	obs_audio_info ai{};
	if (!obs_get_video_info(&vi) || !obs_get_audio_info(&ai)) {
		d->fail("OBS audio/video not ready");
		return false;
	}
	const auto invalid =
		validateSource({vi.output_width, vi.output_height, vi.fps_num, vi.fps_den, ai.samples_per_sec,
				static_cast<unsigned>(get_audio_channels(ai.speakers)), config.audioTrack,
				vi.output_format == VIDEO_FORMAT_NV12 && vi.colorspace == VIDEO_CS_709 &&
					vi.range == VIDEO_RANGE_PARTIAL});
	if (!invalid.empty()) {
		d->fail(invalid.c_str());
		return false;
	}
	if (QFileInfo::exists(config.directory)) {
		d->fail("Capture directory must be new");
		return false;
	}
	QDir().mkpath(QFileInfo(config.directory).absolutePath());
	QStorageInfo disk(QFileInfo(config.directory).absolutePath());
	if (!disk.isValid() || !canStartOnDisk(static_cast<uint64_t>(disk.bytesAvailable()), 0)) {
		d->fail("Insufficient disk reserve");
		return false;
	}
	d->config = config;
	d->config.directory = QFileInfo(config.directory).absoluteFilePath();
	auto *settings = obs_data_create();
	obs_data_set_int(settings, "owner", static_cast<long long>(reinterpret_cast<uintptr_t>(d.get())));
	d->output = obs_output_create("hhc_hls_output", "HHC capture", settings, nullptr);
	obs_data_release(settings);
	if (!d->output) {
		d->fail("Output creation failed");
		return false;
	}
	signal_handler_connect(obs_output_get_signal_handler(d->output), "deactivate", Impl::deactivate, d.get());
	for (unsigned i = 0; i < 3; ++i) {
		auto *s = obs_data_create();
		obs_data_set_string(s, "rate_control", "CBR");
		obs_data_set_int(s, "bitrate", profiles[i].kbps);
		obs_data_set_string(s, "preset", "p5");
		obs_data_set_string(s, "profile", "high");
		obs_data_set_string(s, "tune", "hq");
		obs_data_set_string(s, "multipass", "disabled");
		obs_data_set_int(s, "keyint_sec", 2);
		obs_data_set_int(s, "bf", 0);
		obs_data_set_string(s, "opts", "keyint=60");
		obs_data_set_bool(s, "lookahead", false);
		d->video[i] = obs_video_encoder_create(
			"obs_nvenc_h264_tex", ("HHC " + std::to_string(profiles[i].height)).c_str(), s, nullptr);
		obs_data_release(s);
		if (!d->video[i]) {
			d->fail("NVENC unavailable; no CPU fallback");
			d->release();
			return false;
		}
		obs_encoder_set_video(d->video[i], obs_get_video());
		obs_encoder_set_scaled_size(d->video[i], profiles[i].width, profiles[i].height);
		obs_encoder_set_gpu_scale_type(d->video[i], OBS_SCALE_BICUBIC);
		obs_output_set_video_encoder2(d->output, d->video[i], i);
	}
	auto *as = obs_data_create();
	obs_data_set_int(as, "bitrate", 128);
	d->audio = obs_audio_encoder_create("ffmpeg_aac", "HHC AAC", as, config.audioTrack - 1, nullptr);
	obs_data_release(as);
	if (!d->audio) {
		d->fail("AAC encoder unavailable");
		d->release();
		return false;
	}
	obs_encoder_set_audio(d->audio, obs_get_audio());
	obs_output_set_audio_encoder(d->output, d->audio, 0);
	if (!obs_output_start(d->output)) {
		if (d->worker.joinable())
			d->worker.join();
		d->release();
		return false;
	}
	return true;
}
void CaptureOutput::stop(StopReason reason)
{
	if (!d->output || d->stopRequested.exchange(true))
		return;
	auto previous = d->reason.load();
	while ((previous == StopReason::Shutdown || previous == StopReason::User) &&
	       !d->reason.compare_exchange_weak(previous, reason)) {
	}
	try {
		atomicJson(d->config.directory + "/stop-intent.json",
			   {{"reason", static_cast<int>(reason)}, {"stopAcknowledged", false}});
	} catch (const std::exception &e) {
		d->fail(e.what());
	}
	obs_output_stop(d->output);
}
bool CaptureOutput::active() const
{
	return !d->done && !d->failed;
}
bool CaptureOutput::finished() const
{
	return d->done;
}
bool CaptureOutput::wait(unsigned timeoutMs)
{
	std::unique_lock lock(d->mutex);
	return d->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return d->done.load(); }) && !d->failed;
}
QString CaptureOutput::error() const
{
	std::lock_guard lock(d->mutex);
	return d->error;
}
} // namespace hhc
