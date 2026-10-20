#include "key-transfer.hpp"
#include "ingest.hpp"
#include "broadcast-control.hpp"
#include "session-store.hpp"
#include "windows-path.hpp"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <QDebug>
#include <QCryptographicHash>
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
void markIngestStopping(const QString &root, const QString &id)
{
	if (!writeAtomicMetadata(root + "/intents/" + id + ".stopping", "{\"stopping\":true}"))
		throw RequestError(409, "ingest_local_conflict");
}
KeyJob keyTransferStep(const QString &root, const QString &id, bool active, bool revoked, bool preparing,
		       bool &validated, const std::atomic<bool> *cancelled, IngestCredentials vault,
		       ApiClient::Transport transport, std::function<void(quint64)> uploaded)
{
	QDir().mkpath(root + "/intents");
	QLockFile lock(lockFilePath(root + "/intents/" + id + ".work.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0)) {
		qInfo().noquote() << "[HHC lease] utc=" + QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
			<< "output=" + QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Sha256).toHex().left(16))
			<< "operation=transfer result=busy code=local_session_busy";
		throw RequestError(409, "local_session_busy");
	}
	KeyIngest ingest(root + "/intents", id, std::move(vault), std::move(transport));
	if (!preparing && ingest.metadata().isEmpty())
		throw RequestError(409, "ingest_no_bind_intent");
	if (ingest.metadata()["version"] == "k4-2026-10-15.rc1" && !active && !preparing)
		markIngestStopping(root, id);
	preparing = preparing && !QFileInfo::exists(root + "/intents/" + id + ".stopping");
	const auto result = ingest.bind(), b = result["broadcast"].toObject();
	// A late open response can describe an already ended event, but its transport still needs cleanup.
	const bool canStart =
		preparing && b["boundaries"].toObject()["endSequenceExclusive"].isNull() &&
		!QStringList{"processing", "archived", "cancelled", "failed"}.contains(b["phase"].toString());
	KeyJob out;
	out.id = id;
	out.actor = b["binding"].toObject()["actorId"].toString();
	out.title = b["title"].toString();
	out.mediaProfile = ingest.metadata()["intent"].toObject()["mediaProfile"].toString();
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
	CaptureSync sync(root + "/queue", out.actor, id, api,
		[uploaded](const QByteArray &method, const QUrl &url, const QByteArray &body, const QMap<QByteArray, QByteArray> &headers) {
			auto response = httpRequest(method, url, body, headers);
			if (uploaded && response.status >= 200 && response.status < 300) uploaded(quint64(body.size()));
			return response;
		});
	try {
		out.sync = sync.adopt(b);
		// Read authoritative range before uploading retained S10 media after an outage.
		if(!out.mediaProfile.isEmpty() && !out.sync.sealAccepted &&
		   !QStringList{"aborted","failed","expired","ready"}.contains(out.sync.state) &&
		   !revoked && !QFileInfo::exists(revokePath(root,id)))
			control.poll({},true);
		out.end = control.acceptedEnd();
		out.serverEnd = control.serverEnd();
		if (revoked || QFileInfo::exists(revokePath(root, id)))
			out.sync = sync.abort();
		else if (!(canStart && !out.serverEnd && store.isPrepared(out.actor, id))) {
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
	out.bound = canStart && !out.serverEnd && store.isPrepared(out.actor, id) &&
		    !QStringList{"aborted", "failed", "expired", "ready"}.contains(out.sync.state) &&
		    !out.sync.stopAccepted;
	return out;
}
} // namespace hhc
