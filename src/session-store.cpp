#include "session-store.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
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
void validateObjects(const QString &root, const CaptureJournal &j)
{
	require(j.objects.size() <= 10000, "object count limit");
	QSet<QString> paths;
	qint64 total = 0;
	const auto canonicalRoot = QFileInfo(root).canonicalFilePath() + "/";
	for (const auto &o : j.objects) {
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
		require(digest.addData(&f), "closed object unreadable");
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
		require(previous.size() <= 8 * 1024 * 1024 && previous.open(QIODevice::ReadOnly),
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
	}
	// Serialize an allowlist of typed local fields, never arbitrary server JSON.
	const auto data = QJsonDocument(json(j)).toJson(QJsonDocument::Compact);
	QSaveFile f(dir + "/journal.json");
	f.setDirectWriteFallback(false);
	require(f.open(QIODevice::WriteOnly), "journal open failed");
	require(f.write(data) == data.size() && f.commit(), "atomic journal commit failed");
}
QVector<CaptureJournal> SessionStore::loadPending(const QString &account) const
{
	identifiers(account, "probe");
	const auto accountDir = root_ + "/" + hash(account.toUtf8());
	require(!QFileInfo(root_).isSymLink() && !QFileInfo(accountDir).isSymLink(), "account directory is a link");
	QVector<CaptureJournal> result;
	for (const auto &id : QDir(accountDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		const auto dir = mediaDirectory(account, id);
		require(!QFileInfo(dir).isSymLink(), "session directory is a link");
		QFile f(dir + "/journal.json");
		if (!f.exists())
			continue;
		require(f.size() <= 8 * 1024 * 1024 && f.open(QIODevice::ReadOnly), "journal unreadable or too large");
		QJsonParseError error;
		const auto doc = QJsonDocument::fromJson(f.readAll(), &error);
		require(error.error == QJsonParseError::NoError && doc.isObject(),
			"corrupt journal retained for manual recovery");
		auto j = parse(doc.object());
		require(j.account == account && j.localId == id, "journal account mismatch");
		validateObjects(dir, j);
		result.append(j);
	}
	return result;
}
bool SessionStore::mayCleanup(const CaptureJournal &j, QDateTime now) const
{
	return j.normalEnd && j.sealAcknowledged && j.confirmedReady && !j.packageId.isEmpty() &&
	       j.packageId == j.readyPackage && j.readyAt.isValid() && now.isValid() && j.readyAt.addDays(7) <= now;
}
} // namespace hhc
