#include "session-store.hpp"
#include "windows-path.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <stdexcept>
namespace hhc {
namespace {
void require(bool ok, const char *why)
{
	if (!ok)
		throw std::runtime_error(why);
}
QString hash(const QByteArray &b)
{
	return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
}
void checkCancellation(const std::atomic<bool> *cancelled)
{
	if (cancelled && cancelled->load())
		throw std::runtime_error("Local recovery cancelled; files retained");
}
void identifiers(const QString &account, const QString &id)
{
	require(!account.isEmpty() && account.size() <= 256, "invalid account identity");
	require(QRegularExpression("\\A[a-zA-Z0-9_-]{1,80}\\z").match(id).hasMatch(), "invalid local identity");
}
QJsonObject json(const CaptureJournal &j)
{
	QJsonArray objects;
	for (const auto &o : j.objects)
		objects.append(QJsonObject{{"path", o.path},
					   {"size", o.size},
					   {"sha256", o.sha256},
					   {"confirmed", o.confirmed}});
	return {{"localVersion", 1},
		{"account", j.account},
		{"localId", j.localId},
		{"remoteCapture", j.remoteCapture},
		{"packageId", j.packageId},
		{"readyPackage", j.readyPackage},
		{"objects", objects},
		{"stopIntent", j.stopIntent},
		{"normalEnd", j.normalEnd},
		{"sealAcknowledged", j.sealAcknowledged},
		{"confirmedReady", j.confirmedReady},
		{"readyAt", j.readyAt.toUTC().toString(Qt::ISODateWithMs)}};
}
CaptureJournal parse(const QJsonObject &o)
{
	const QSet<QString> allowed{"localVersion", "account",          "localId",        "remoteCapture",
				    "packageId",    "readyPackage",     "objects",        "stopIntent",
				    "normalEnd",    "sealAcknowledged", "confirmedReady", "readyAt"};
	for (auto it = o.begin(); it != o.end(); ++it)
		require(allowed.contains(it.key()), "unknown journal field");
	require(o["localVersion"].toInt() == 1 && o["objects"].isArray(), "invalid journal version");
	CaptureJournal j;
	j.account = o["account"].toString();
	j.localId = o["localId"].toString();
	j.remoteCapture = o["remoteCapture"].toString();
	j.packageId = o["packageId"].toString();
	j.readyPackage = o["readyPackage"].toString();
	for (const auto *key : {"stopIntent", "normalEnd", "sealAcknowledged", "confirmedReady"})
		require(o[key].isBool(), "invalid journal boolean");
	j.stopIntent = o["stopIntent"].toBool();
	j.normalEnd = o["normalEnd"].toBool();
	j.sealAcknowledged = o["sealAcknowledged"].toBool();
	j.confirmedReady = o["confirmedReady"].toBool();
	j.readyAt = QDateTime::fromString(o["readyAt"].toString(), Qt::ISODateWithMs);
	for (const auto &v : o["objects"].toArray()) {
		require(v.isObject(), "invalid journal object");
		const auto item = v.toObject();
		require(item.size() == 4 && item["confirmed"].isBool() && item["size"].isDouble(),
			"invalid journal object fields");
		j.objects.append({item["path"].toString(), item["size"].toInteger(), item["sha256"].toString(),
				  item["confirmed"].toBool()});
	}
	return j;
}
void validateObjects(const QString &root, const CaptureJournal &j, const std::atomic<bool> *cancelled = nullptr)
{
	require(j.objects.size() <= 10000, "object count limit");
	QSet<QString> paths;
	qint64 total = 0;
	const auto canonicalRoot = QFileInfo(root).canonicalFilePath() + "/";
	for (const auto &o : j.objects) {
		checkCancellation(cancelled);
		require(QRegularExpression(
				"\\A(?:master\\.m3u8|(?:1080p|720p|480p)/(?:init\\.mp4|index\\.m3u8|segment-[0-9]{5}\\.m4s|seg-[0-9]{6}\\.m4s))\\z")
				.match(o.path)
				.hasMatch(),
			"invalid closed object path");
		require(!paths.contains(o.path), "duplicate immutable path");
		paths.insert(o.path);
		require(o.size > 0 && o.size <= 134217728 && total <= 10000000000LL - o.size,
			"object/package size limit");
		total += o.size;
		require(QRegularExpression("\\A[a-f0-9]{64}\\z").match(o.sha256).hasMatch(), "invalid SHA256");
		const QFileInfo info(root + "/" + o.path);
		require(!info.isSymLink() && info.canonicalFilePath().startsWith(canonicalRoot, Qt::CaseInsensitive),
			"media escaped session root");
		require(info.size() == o.size, "closed object size changed");
		QFile f(info.filePath());
		require(f.open(QIODevice::ReadOnly), "closed object missing");
		QCryptographicHash digest(QCryptographicHash::Sha256);
		while (!f.atEnd()) {
			checkCancellation(cancelled);
			const auto bytes = f.read(1024 * 1024);
			require(!bytes.isEmpty() && f.error() == QFileDevice::NoError, "closed object unreadable");
			digest.addData(bytes);
		}
		checkCancellation(cancelled);
		require(f.error() == QFileDevice::NoError && f.pos() == o.size, "closed object unreadable");
		require(QString::fromLatin1(digest.result().toHex()) == o.sha256, "closed object hash changed");
	}
}
} // namespace
SessionStore::SessionStore(QString root) : root_(QFileInfo(root).absoluteFilePath()) {}
QString SessionStore::mediaDirectory(const QString &account, const QString &id) const
{
	identifiers(account, id);
	return root_ + "/" + hash(account.toUtf8()) + "/" + id;
}
void SessionStore::save(const CaptureJournal &j)
{
	const auto dir = mediaDirectory(j.account, j.localId);
	require(!QFileInfo(root_).isSymLink() && !QFileInfo(QFileInfo(dir).absolutePath()).isSymLink() &&
			!QFileInfo(dir).isSymLink(),
		"session ancestor is a link");
	require(QDir().mkpath(dir), "create session directory failed");
	require(!QFileInfo(dir).isSymLink(), "session directory is a link");
	validateObjects(dir, j);
	QFile previous(dir + "/journal.json");
	if (previous.exists()) {
		require(previous.size() <= 8 * 1024 * 1024 && openSharedJsonRead(previous),
			"prior journal unreadable");
		QJsonParseError error;
		auto doc = QJsonDocument::fromJson(previous.readAll(), &error);
		require(error.error == QJsonParseError::NoError && doc.isObject(),
			"prior journal corrupt; refuse overwrite");
		const auto old = parse(doc.object());
		require(old.account == j.account && old.localId == j.localId, "prior journal identity mismatch");
		for (const auto &o : old.objects) {
			auto found = std::find_if(j.objects.begin(), j.objects.end(),
						  [&](const auto &n) { return n.path == o.path; });
			require(found != j.objects.end() && found->size == o.size && found->sha256 == o.sha256,
				"immutable object identity changed");
		}
		previous.close();
	}
	// Serialize an allowlist of typed local fields, never arbitrary server JSON.
	const auto data = QJsonDocument(json(j)).toJson(QJsonDocument::Compact);
	require(writeAtomicMetadata(dir + "/journal.json", data), "atomic journal commit failed");
}
QVector<CaptureJournal> SessionStore::loadPending(const QString &account) const
{
	auto report = scanPending(account);
	require(report.issues.empty(), "session recovery issues require attention; media retained");
	return report.sessions;
}
RecoveryReport SessionStore::scanPending(const QString &account, const std::atomic<bool> *cancelled) const
{
	identifiers(account, "probe");
	checkCancellation(cancelled);
	const auto accountDir = root_ + "/" + hash(account.toUtf8());
	require(!QFileInfo(root_).isSymLink() && !QFileInfo(accountDir).isSymLink(), "account directory is a link");
	RecoveryReport result;
	for (const auto &id : QDir(accountDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		checkCancellation(cancelled);
		try {
			const auto dir = mediaDirectory(account, id);
			require(!QFileInfo(dir).isSymLink(), "session directory is a link");
			QFile f(dir + "/journal.json");
			if (!f.exists())
				continue;
			require(f.size() <= 8 * 1024 * 1024 && openSharedJsonRead(f),
				"journal unreadable or too large");
			QJsonParseError error;
			const auto doc = QJsonDocument::fromJson(f.readAll(), &error);
			require(error.error == QJsonParseError::NoError && doc.isObject(),
				"corrupt journal retained for manual recovery");
			auto j = parse(doc.object());
			require(j.account == account && j.localId == id, "journal account mismatch");
			// A crash may occur after an atomic per-rendition close receipt but before
			// the next session checkpoint. Recover only receipted bytes, never .tmp files
			// or a normal-end inference. This read does not rewrite the saved journal.
			if (!j.normalEnd) {
				for (const auto *rendition : {"1080p", "720p", "480p"}) {
					QFile receipt(dir + "/" + rendition + "/closed.json");
					if (!receipt.exists())
						continue;
					require(!QFileInfo(receipt).isSymLink() && receipt.size() <= 8 * 1024 * 1024 &&
							openSharedJsonRead(receipt),
						"close receipt unreadable");
					QJsonParseError receiptError;
					const auto closed = QJsonDocument::fromJson(receipt.readAll(), &receiptError);
					require(receiptError.error == QJsonParseError::NoError && closed.isObject() &&
							closed.object()["objects"].isArray(),
						"corrupt close receipt retained");
					for (const auto &value : closed.object()["objects"].toArray()) {
						const auto o = value.toObject();
						require(o.size() == 3 && o["size"].isDouble() &&
								o["path"].toString().startsWith(QString(rendition) +
												"/"),
							"invalid close receipt object");
						ClosedObject recovered{o["path"].toString(), o["size"].toInteger(),
								       o["sha256"].toString(), false};
						auto found = std::find_if(j.objects.begin(), j.objects.end(),
									  [&](const auto &old) {
										  return old.path == recovered.path;
									  });
						if (found == j.objects.end())
							j.objects.append(recovered);
						else
							require(found->size == recovered.size &&
									found->sha256 == recovered.sha256,
								"conflicting close receipt retained");
					}
				}
			}
			validateObjects(dir, j, cancelled);
			result.sessions.append(j);
		} catch (const std::exception &) {
			checkCancellation(cancelled);
			// Never include raw JSON, credentials or server responses in recovery UI.
			const bool safeId = QRegularExpression("\\A[a-zA-Z0-9_-]{1,80}\\z").match(id).hasMatch();
			result.issues.append({safeId ? id : QString("invalid-local-id"),
					      "Local recovery validation failed; files retained for inspection"});
		}
	}
	checkCancellation(cancelled);
	return result;
}
void SessionStore::checkpointLocal(const QString &account, const QString &localId, const QVector<ClosedObject> &closed,
				   bool stopIntent, bool normalEnd)
{
	const auto dir = mediaDirectory(account, localId);
	require(!QFileInfo(root_).isSymLink() && !QFileInfo(QFileInfo(dir).absolutePath()).isSymLink() &&
			!QFileInfo(dir).isSymLink(),
		"session ancestor is a link");
	QFile file(dir + "/journal.json");
	require(!QFileInfo(file).isSymLink() && file.size() <= 8 * 1024 * 1024 && openSharedJsonRead(file),
		"checkpoint requires existing journal");
	QJsonParseError error;
	const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
	require(error.error == QJsonParseError::NoError && doc.isObject(), "corrupt journal retained");
	file.close();
	auto j = parse(doc.object());
	require(j.account == account && j.localId == localId, "checkpoint identity mismatch");
	CaptureJournal added;
	for (const auto &o : closed) {
		auto found = std::find_if(j.objects.begin(), j.objects.end(),
					  [&](const auto &old) { return old.path == o.path; });
		if (found != j.objects.end()) {
			require(found->size == o.size && found->sha256 == o.sha256,
				"immutable checkpoint identity changed");
		} else {
			require(!j.normalEnd && !o.confirmed, "cannot append after finalization or invent receipt");
			added.objects.append(o);
			j.objects.append(o);
		}
	}
	validateObjects(dir, added);
	qint64 total = 0;
	require(j.objects.size() <= 10000, "object count limit");
	for (const auto &o : j.objects) {
		require(o.size > 0 && o.size <= 134217728 && total <= 10000000000LL - o.size, "package size limit");
		total += o.size;
	}
	j.stopIntent = j.stopIntent || stopIntent;
	if (normalEnd) {
		require(j.stopIntent, "normal end requires stop intent");
		for (const auto *path : {"master.m3u8", "1080p/index.m3u8", "720p/index.m3u8", "480p/index.m3u8",
					 "1080p/init.mp4", "720p/init.mp4", "480p/init.mp4"})
			require(std::any_of(j.objects.begin(), j.objects.end(),
					    [&](const auto &o) { return o.path == path; }),
				"normal end requires all final playlists and init objects");
		validateObjects(dir, j);
		j.normalEnd = true;
	}
	const auto bytes = QJsonDocument(json(j)).toJson(QJsonDocument::Compact);
	require(writeAtomicMetadata(dir + "/journal.json", bytes),
		"atomic checkpoint failed");
}
void SessionStore::confirmReady(const QString &account, const QString &id, const QString &captureId,
				const QString &packageId, QDateTime observedAt)
{
	const auto dir = mediaDirectory(account, id);
	require(!QFileInfo(root_).isSymLink() && !QFileInfo(QFileInfo(dir).absolutePath()).isSymLink() &&
			!QFileInfo(dir).isSymLink(),
		"Ready session ancestor is a link");
	QFile file(dir + "/journal.json");
	require(!QFileInfo(file).isSymLink() && file.size() <= 8 * 1024 * 1024 && openSharedJsonRead(file),
		"Ready journal unavailable; media retained");
	QJsonParseError error;
	const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
	require(error.error == QJsonParseError::NoError && doc.isObject(), "Ready journal corrupt; media retained");
	file.close();
	auto j = parse(doc.object());
	require(j.account == account && j.localId == id && j.normalEnd && j.stopIntent,
		"Ready requires matching complete stopped session");
	const QRegularExpression remoteId("\\A[a-f0-9]{32}\\z");
	require(remoteId.match(captureId).hasMatch() && remoteId.match(packageId).hasMatch() && observedAt.isValid(),
		"Ready package evidence invalid");
	require((j.remoteCapture.isEmpty() || j.remoteCapture == captureId) &&
			(j.packageId.isEmpty() || j.packageId == packageId) &&
			(j.readyPackage.isEmpty() || j.readyPackage == packageId),
		"Ready package identity changed; media retained");
	if (j.confirmedReady) {
		require(j.sealAcknowledged && j.remoteCapture == captureId && j.packageId == packageId &&
				j.readyPackage == packageId && j.readyAt.isValid(),
			"Prior ready evidence incomplete; media retained");
		return;
	}
	j.remoteCapture = captureId;
	j.packageId = j.readyPackage = packageId;
	j.sealAcknowledged = j.confirmedReady = true;
	j.readyAt = observedAt.toUTC();
	// Metadata checkpoint only. Recovery verifies every media hash before offering cleanup.
	const auto bytes = QJsonDocument(json(j)).toJson(QJsonDocument::Compact);
	require(writeAtomicMetadata(dir + "/journal.json", bytes),
		"Ready evidence commit failed; media retained");
}
bool SessionStore::isPrepared(const QString &account, const QString &id) const
{
	try {
		auto dir = mediaDirectory(account, id);
		require(!QFileInfo(root_).isSymLink() && !QFileInfo(QFileInfo(dir).absolutePath()).isSymLink() &&
				!QFileInfo(dir).isSymLink(),
			"Prepared directory is a link");
		const QSet<QString> allowed{"journal.json", "remote-journal.json", "local-session.json",
					    "broadcast-journal.json"};
		for (const auto &name :
		     QDir(dir).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
			require(allowed.contains(name) && QFileInfo(dir + "/" + name).isFile() &&
					!QFileInfo(dir + "/" + name).isSymLink(),
				"Prepared directory contains media or unknown files");
		QFile f(dir + "/journal.json");
		require(f.size() <= 8 * 1024 * 1024 && openSharedJsonRead(f), "Prepared journal unavailable");
		QJsonParseError e;
		auto doc = QJsonDocument::fromJson(f.readAll(), &e);
		require(e.error == QJsonParseError::NoError && doc.isObject(), "Prepared journal corrupt");
		auto j = parse(doc.object());
		require(j.account == account && j.localId == id && j.objects.empty() && !j.stopIntent && !j.normalEnd &&
				!j.sealAcknowledged && !j.confirmedReady,
			"Prepared journal is not pristine");
		return true;
	} catch (...) {
		return false;
	}
}
bool SessionStore::mayCleanup(const CaptureJournal &j, QDateTime now) const
{
	return j.normalEnd && j.sealAcknowledged && j.confirmedReady && !j.packageId.isEmpty() &&
	       j.packageId == j.readyPackage && j.readyAt.isValid() && now.isValid() && j.readyAt.addDays(7) <= now;
}
} // namespace hhc
