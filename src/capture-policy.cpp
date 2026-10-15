#include "capture-policy.hpp"
#include <algorithm>
#include <limits>
namespace hhc {
std::optional<int> nextCommonBoundary(const std::array<std::int64_t, 3> &frames)
{
	if (*std::min_element(frames.begin(), frames.end()) < 0)
		return std::nullopt;
	const auto sequence = *std::max_element(frames.begin(), frames.end()) / 900 + 1;
	return sequence <= std::numeric_limits<int>::max() ? std::optional<int>(int(sequence)) : std::nullopt;
}
StopReason failureReason(StopReason current, StopReason failure)
{
	return current == StopReason::User || current == StopReason::Shutdown ? failure : current;
}
std::string validateSource(const SourceFormat &s)
{
	if (s.width != 1920 || s.height != 1080)
		return "Program must be 1920x1080";
	if (!s.fpsDen || std::uint64_t(s.fpsNum) * 1001ULL != 30000ULL * s.fpsDen)
		return "Program must be 29.97 fps (30000/1001); OBS settings were not changed";
	if (s.sampleRate != 48000 || s.channels != 2)
		return "Audio must be 48 kHz stereo";
	if (s.track < 1 || s.track > 6)
		return "Choose an explicit audio track (1-6)";
	if (!s.supportedColor)
		return "Program must be NV12, limited-range BT.709; OBS settings were not changed";
	return {};
}
bool normalEnd(StopReason reason, const std::array<bool, 3> &tails, bool failed)
{
	return reason == StopReason::User && !failed && tails[0] && tails[1] && tails[2];
}
std::optional<StopReason> runtimeLimit(std::uint64_t free, std::uint64_t bytes, unsigned objects, double seconds)
{
	if (free < 2147483648ULL || bytes >= 10000000000ULL - 134217728ULL || objects >= 9996 || seconds >= 43200)
		return StopReason::DiskLimit;
	return {};
}
bool canStartOnDisk(std::uint64_t free, std::uint64_t queued)
{
	constexpr std::uint64_t reserve = 10000000000ULL + 2147483648ULL;
	return free >= reserve && queued <= free - reserve;
}
} // namespace hhc
