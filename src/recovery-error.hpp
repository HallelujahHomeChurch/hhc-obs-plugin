#pragma once
#include <QString>
#include <QByteArray>
#include <algorithm>
#include <source_location>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace hhc {
// Only failed internal invariants use this type. Arbitrary exceptions are not validation failures.
class LocalStateError : public std::runtime_error {
public:
	explicit LocalStateError(const char *why, std::source_location site = std::source_location::current())
		: std::runtime_error(why), site(site) {}
	std::source_location site;
};
class LocalIoError : public std::runtime_error {
public:
	explicit LocalIoError(DWORD error, std::source_location site = std::source_location::current())
		: std::runtime_error("Required local metadata I/O failed"), systemError(error), site(site) {}
	DWORD systemError;
	std::source_location site;
};
struct LocalFailure {
	QString code = "internal_unclassified_exception", check;
	bool retryable = true;
	DWORD systemError = 0;
};
inline LocalFailure classifyLocalFailure(const std::exception *error)
{
	LocalFailure result;
	std::source_location site;
	if (const auto *io = dynamic_cast<const LocalIoError *>(error)) {
		result.systemError = io->systemError;
		result.retryable = io->systemError == ERROR_SHARING_VIOLATION || io->systemError == ERROR_LOCK_VIOLATION ||
			io->systemError == ERROR_BUSY || io->systemError == ERROR_RETRY;
		result.code = io->systemError == ERROR_DISK_FULL || io->systemError == ERROR_HANDLE_DISK_FULL
			? "local_disk_full" : result.retryable ? "local_io_busy" : "local_io_failed";
		site = io->site;
	} else if (const auto *local = dynamic_cast<const LocalStateError *>(error)) {
		const QByteArray reason(local->what());
		result.retryable = false;
		result.code = reason == "Queue document corrupt; data retained" ? "local_queue_corrupt" :
			reason == "Closed upload bytes changed" || reason == "closed object hash changed" ||
			reason == "closed object size changed" ? "local_media_changed" :
			reason == "Remote immutable object differs" ? "local_remote_media_conflict" : "local_state_invalid";
		site = local->site;
	} else return result;
	// Source coordinates identify the failed check without persisting paths, URLs or exception text.
	result.check = QString::fromUtf8(site.file_name()).replace('\\', '/').section('/', -1) + ':' + QString::number(site.line());
	return result;
}
inline qint64 localRetryDelayMs(unsigned &failures)
{
	failures = std::min(failures, 5u) + 1;
	return qint64(std::min(60u, 1u << failures)) * 1000;
}
} // namespace hhc
