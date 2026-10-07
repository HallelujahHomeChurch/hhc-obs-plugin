#pragma once
#include "capture-policy.hpp"
#include <QString>
#include <memory>
#include <obs.h>
namespace hhc {
struct CaptureConfig {
	QString directory;
	unsigned audioTrack = 1;
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

private:
	struct Impl;
	std::unique_ptr<Impl> d;
};
} // namespace hhc
