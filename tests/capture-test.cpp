#include "capture-policy.hpp"
#include <iostream>
#include <limits>
int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) {
			std::cerr << "FAIL " << name << '\n';
			++failures;
		}
	};
	hhc::SourceFormat s{1920, 1080, 30000, 1001, 48000, 2, 1};
	check(hhc::validateSource(s).empty(), "accept actual Program 1080p29.97 stereo track 1");
	s.fpsNum = 30;
	s.fpsDen = 1;
	check(!hhc::validateSource(s).empty(), "reject mismatched 30fps without changing OBS");
	s.fpsNum = 30000;
	s.fpsDen = 1001;
	s.track = 0;
	check(!hhc::validateSource(s).empty(), "reject unspecified track");
	s.track = 7;
	check(!hhc::validateSource(s).empty(), "reject track beyond OBS mixers");
	s.track = 6;
	s.sampleRate = 44100;
	check(!hhc::validateSource(s).empty(), "reject 44.1kHz");
	s.sampleRate = 48000;
	s.channels = 1;
	check(!hhc::validateSource(s).empty(), "reject mono");
	s.channels = 2;
	s.width = 1280;
	check(!hhc::validateSource(s).empty(), "reject incompatible Program dimensions");
	check(hhc::normalEnd(hhc::StopReason::User, {true, true, true}, false), "all tails normal user stop");
	check(!hhc::normalEnd(hhc::StopReason::User, {true, false, true}, false), "missing rendition is not normal");
	check(!hhc::normalEnd(hhc::StopReason::User, {true, true, true}, true),
	      "prior encoder error cannot become normal");
	for (auto reason : {hhc::StopReason::DiskLimit, hhc::StopReason::EncoderFailure, hhc::StopReason::QueueLimit,
			    hhc::StopReason::Shutdown})
		check(!hhc::normalEnd(reason, {true, true, true}, false), "abnormal stop never normal");
	constexpr std::uint64_t needed = 10000000000ULL + 2147483648ULL;
	check(hhc::canStartOnDisk(needed, 0), "exact disk reserve");
	check(!hhc::canStartOnDisk(needed - 1, 0), "reject low disk");
	check(!hhc::canStartOnDisk(needed, 1), "account for previous queue");
	check(!hhc::canStartOnDisk(needed, std::numeric_limits<std::uint64_t>::max()), "disk overflow fails closed");
	check(!hhc::runtimeLimit(3ULL * 1024 * 1024 * 1024, 1000000, 10, 61), "ordinary capture continues");
	check(hhc::runtimeLimit(2147483647, 0, 0, 1).has_value(), "runtime disk reserve stops capture");
	check(hhc::runtimeLimit(3000000000, 9900000000ULL, 0, 1).has_value(), "package headroom stops capture");
	check(hhc::runtimeLimit(3000000000, 0, 10000, 1).has_value(), "object count stops capture");
	check(hhc::runtimeLimit(3000000000, 0, 0, 43200).has_value(), "12 hour ceiling stops capture");
	s = {1920, 1080, 30000, 1001, 48000, 2, 1, false};
	check(!hhc::validateSource(s).empty(), "reject color metadata mismatch");
	check(hhc::failureReason(hhc::StopReason::User, hhc::StopReason::EncoderFailure) ==
		      hhc::StopReason::EncoderFailure,
	      "tail encoder failure overrides user reason");
	check(hhc::failureReason(hhc::StopReason::Shutdown, hhc::StopReason::QueueLimit) == hhc::StopReason::QueueLimit,
	      "queue limit persisted explicitly");
	check(hhc::failureReason(hhc::StopReason::DiskLimit, hhc::StopReason::EncoderFailure) ==
		      hhc::StopReason::DiskLimit,
	      "keep first abnormal cause");
	return failures ? 1 : 0;
}
