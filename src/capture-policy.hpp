#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <optional>
namespace hhc {
enum class StopReason { User, DiskLimit, EncoderFailure, QueueLimit, Shutdown };
struct SourceFormat { unsigned width, height, fpsNum, fpsDen, sampleRate, channels, track; };
struct Profile { unsigned width, height, kbps; };
inline constexpr std::array<Profile,3> profiles{{{1920,1080,3000},{1280,720,1500},{854,480,800}}};
std::string validateSource(const SourceFormat&);
bool normalEnd(StopReason, const std::array<bool,3>& tailsClosed, bool failed);
std::optional<StopReason> runtimeLimit(std::uint64_t free, std::uint64_t encodedBytes, unsigned objects, double seconds);
bool canStartOnDisk(std::uint64_t free, std::uint64_t queued);
}

