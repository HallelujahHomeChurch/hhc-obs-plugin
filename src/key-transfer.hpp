#pragma once
#include "capture-sync.hpp"
#include <atomic>
namespace hhc {
struct KeyJob {
	QString id, actor, title;
	SyncState sync;
	bool bound = false;
	std::optional<int> end;
};
KeyJob keyTransferStep(const QString &root, const QString &id, bool active, bool revoked, bool preparing,
		       bool &validated, const std::atomic<bool> *cancelled = nullptr);
void markIngestRevoked(const QString &root, const QString &id);
} // namespace hhc
