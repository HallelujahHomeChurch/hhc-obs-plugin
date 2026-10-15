#include "key-transfer.hpp"
#include "ingest.hpp"
#include "broadcast-control.hpp"
#include "session-store.hpp"
#include "windows-path.hpp"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
namespace hhc {
QString revokePath(const QString &root, const QString &id)
{
	return root + "/intents/" + id + ".revoked";
}
void markIngestRevoked(const QString &root, const QString &id)
{
	if (!writeAtomicMetadata(revokePath(root, id), QJsonDocument(QJsonObject{{"revoked", true}}).toJson()))
		throw RequestError(409, "ingest_local_conflict");
}
KeyJob keyTransferStep(const QString &root, const QString &id, bool active, bool revoked, bool preparing,
		       bool &validated, const std::atomic<bool> *cancelled)
{
	QDir().mkpath(root + "/intents");
	QLockFile lock(lockFilePath(root + "/intents/" + id + ".work.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	KeyIngest ingest(root + "/intents", id);
	if (!preparing && ingest.metadata().isEmpty())
		throw RequestError(409, "ingest_no_bind_intent");
	const auto result = ingest.bind(), b = result["broadcast"].toObject();
	KeyJob out;
	out.id = id;
	out.actor = b["binding"].toObject()["actorId"].toString();
	out.title = b["title"].toString();
	auto api = ingest.api();
	SessionStore store(root + "/queue");
	const auto directory = store.mediaDirectory(out.actor, id);
	const bool prepared = !QFileInfo::exists(directory + "/journal.json");
	if (prepared) {
		CaptureJournal j;
		j.account = out.actor;
		j.localId = id;
		store.save(j);
	}
	BroadcastControl control(directory, out.actor, id, api, true);
	control.adoptIngest(result, ingest.metadata()["intent"].toObject());
	CaptureSync sync(root + "/queue", out.actor, id, api);
	try {
		out.sync = sync.adopt(b);
		out.end = control.acceptedEnd();
		if (revoked || QFileInfo::exists(revokePath(root, id)))
			out.sync = sync.abort();
		else if (!(preparing && store.isPrepared(out.actor, id))) {
			if (!active && !validated && !out.sync.sealAccepted) {
				const auto recovery = store.scanPending(out.actor, cancelled, id);
				if (!recovery.issues.empty())
					throw RequestError(409, "local_media_validation_failed");
				validated = true;
			}
			out.sync = sync.step(active, [&] { control.replay(); });
		}
	} catch (const RequestError &e) {
		if (e.code == "broadcast_ingest_revoked")
			markIngestRevoked(root, id);
		throw;
	}
	out.bound = preparing && store.isPrepared(out.actor, id);
	return out;
}
} // namespace hhc
