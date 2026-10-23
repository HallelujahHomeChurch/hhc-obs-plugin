#pragma once
#include "capture-sync.hpp"
#include "ingest.hpp"
#include <atomic>
namespace hhc {
struct KeyJob {
	QString id, actor, title, mediaProfile;
	SyncState sync;
	bool bound = false;
	bool recoveryRequested = false;
	QJsonObject recoveryControl;
	std::optional<int> end;
	std::optional<int> serverEnd;
};
KeyJob keyTransferStep(const QString &root, const QString &id, bool active, bool revoked, bool preparing,
		       bool &validated, const std::atomic<bool> *cancelled = nullptr, IngestCredentials vault = {},
		       ApiClient::Transport transport = httpRequest, std::function<void(quint64)> uploaded = {},
		       std::function<bool()> current = {});
void markIngestStopping(const QString &root, const QString &id);
void markIngestRevoked(const QString &root, const QString &id);
} // namespace hhc
