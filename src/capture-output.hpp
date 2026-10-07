#pragma once
#include "capture-policy.hpp"
#include <QString>
#include <memory>
#include <obs.h>
namespace hhc {
struct CaptureConfig {
	QString directory;
	unsigned audioTrack = 1;
	// Empty only for standalone media fixtures. Real queue sessions bind all three.
	QString queueRoot, account, localId;
};
class CaptureOutput {
public:
	CaptureOutput();
	~CaptureOutput();
	CaptureOutput(const CaptureOutput &) = delete;
	CaptureOutput &operator=(const CaptureOutput &) = delete;
	static void registerOutput();
	bool start(const CaptureConfig &);
	void stop(StopReason);
	bool active() const;
	bool finished() const;
	bool wait(unsigned timeoutMs);
	QString error() const;
	std::uint64_t encodedBytes() const;

private:
	struct Impl;
	std::unique_ptr<Impl> d;
};
} // namespace hhc
