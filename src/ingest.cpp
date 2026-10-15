#include "ingest.hpp"
#include "wire.hpp"
#include "windows-path.hpp"
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QSet>
namespace hhc {
namespace {
void require(bool ok)
{
	if (!ok)
		throw RequestError(409, "ingest_local_conflict");
}
} // namespace
QString ingestRoute(const QByteArray &method, const QString &path, const QJsonObject &s)
{
	require(validWire("BroadcastIngestSession", s));
	const auto recording = s["recordingId"].toString(), capture = s["captureId"].toString();
	const auto base = "/admin/recordings/" + recording + "/captures/" + capture;
	const QUrl url("https://local.invalid" + path);
	const QUrlQuery query(url);
	require(!path.contains('%') || path.startsWith(base + "?"));
	require(!path.contains("..") && !path.contains('#') && !path.contains('\\'));
	if (url.path() == base && method == "GET") {
		QSet<QString> seen;
		for (const auto &q : query.queryItems()) {
			require((q.first == "limit" || q.first == "cursor") && !seen.contains(q.first));
			seen.insert(q.first);
		}
		return "/ingest/broadcasts/" + recording + "/captures/" + capture +
		       (url.hasQuery() ? "?" + url.query() : QString{});
	}
	if (path.startsWith(base + "/") && method == "POST" && !url.hasQuery()) {
		require(QStringList{"objects", "sign", "confirm", "stop", "seal", "abort"}.contains(
			path.mid(base.size() + 1)));
		return "/ingest/broadcasts/" + recording + "/captures/" + capture + path.mid(base.size());
	}
	const auto broadcast = "/admin/broadcasts/" + recording;
	if (url.path() == broadcast + "/control" && method == "GET") {
		require(query.queryItems().size() == 2 && query.queryItemValue("captureId") == capture &&
			query.queryItemValue("epoch") == QString::number(s["epoch"].toInteger()));
		return "/ingest/broadcasts/" + recording + "/control?" + url.query();
	}
	if (method == "POST" && !url.hasQuery() && path.startsWith(broadcast + "/commands/") && path.endsWith("/ack")) {
		const auto command = path.mid(broadcast.size() + 10); // /commands/
		require(QRegularExpression("^[0-9a-fA-F-]{36}/ack$").match(command).hasMatch());
		return "/ingest/broadcasts/" + recording + path.mid(broadcast.size());
	}
	throw RequestError(403, "ingest_route_forbidden");
}
KeyIngest::KeyIngest(QString root, QString id, IngestCredentials vault, ApiClient::Transport transport)
	: root_(std::move(root)),
	  id_(std::move(id)),
	  vault_(std::move(vault)),
	  transport_(std::move(transport))
{
	require(QRegularExpression("^[A-Za-z0-9-]{1,64}$").match(id_).hasMatch());
	if (!vault_.load)
		vault_.load = [](const QString &id) {
			return CredentialVault::load("https://admin.alive.org.tw/k3", id);
		};
	if (!vault_.save)
		vault_.save = [](const QString &id, const QByteArray &b) {
			CredentialVault::save("https://admin.alive.org.tw/k3", id, b);
		};
	QFile file(root_ + "/" + id_ + ".json");
	if (file.exists()) {
		require(!QFileInfo(file).isSymLink() && file.size() < 65536 && openSharedJsonRead(file));
		QJsonParseError e;
		auto doc = QJsonDocument::fromJson(file.readAll(), &e);
		require(e.error == QJsonParseError::NoError && doc.isObject());
		journal_ = doc.object();
		const QSet<QString> allowed{"version", "localId", "intent", "broadcast", "receipt", "sessionBinding"};
		for (auto i = journal_.begin(); i != journal_.end(); ++i)
			require(allowed.contains(i.key()));
		require(journal_["version"] == "k3-2026-10-14.rc1" && journal_["localId"] == id_);
		auto intent = journal_["intent"].toObject();
		intent["streamKey"] = "hhc_stream_" + QString(43, 'A');
		require(validWire("BroadcastSourceBindInput", intent) && intent["operationKey"] == id_ + ".bind");
		if (journal_.contains("broadcast")) {
			const auto b = journal_["broadcast"].toObject(), r = journal_["receipt"].toObject();
			require(validWire("BroadcastView", b) && validWire("BroadcastReceipt", r));
			require(b["recordingId"] == intent["recordingId"] &&
				r["operationKey"] == intent["operationKey"] && r["operation"] == "bind" &&
				r["state"] != "rejected");
			const auto binding = b["binding"].toObject(), pinned = journal_["sessionBinding"].toObject();
			require(pinned["recordingId"] == b["recordingId"] &&
				pinned["captureId"] == binding["captureId"] && pinned["epoch"] == binding["epoch"]);
		}
	}
	auto stored = vault_.load("session-" + id_);
	if (stored) {
		const auto s = QJsonDocument::fromJson(*stored).object();
		require(validWire("BroadcastIngestSession", s));
		auto binding = s;
		binding.remove("token");
		binding.remove("expiresAt");
		require(!journal_.contains("sessionBinding") || journal_["sessionBinding"] == binding);
		require(journal_["intent"].toObject()["recordingId"] == s["recordingId"]);
		session_ = s;
	}
}
void KeyIngest::saveKey(const QByteArray &value)
{
	require(QRegularExpression("^hhc_stream_[A-Za-z0-9_-]{43}$").match(QString::fromLatin1(value)).hasMatch());
	vault_.save("stream-key", value);
}
bool KeyIngest::hasKey() const
{
	return vault_.load("stream-key").has_value();
}
QByteArray KeyIngest::key() const
{
	auto value = vault_.load("stream-key");
	if (!value)
		throw RequestError(403, "broadcast_stream_key_invalid");
	return *value;
}
void KeyIngest::save()
{
	require(QDir().mkpath(root_) && !QFileInfo(root_).isSymLink());
	require(writeAtomicMetadata(root_ + "/" + id_ + ".json",
				    QJsonDocument(journal_).toJson(QJsonDocument::Compact)));
}
QJsonObject KeyIngest::bootstrap(const QString &path, QJsonObject body, const QString &schema)
{
	ApiClient client(QUrl("https://admin.alive.org.tw/api"), [](bool) { return QByteArray{}; }, transport_);
	return client.request("POST", "/ingest/broadcast-source/" + path, body, schema)["data"].toObject();
}
void KeyIngest::acceptSession(const QJsonObject &s)
{
	require(validWire("BroadcastIngestSession", s) &&
		s["recordingId"] == journal_["intent"].toObject()["recordingId"]);
	auto binding = s;
	binding.remove("token");
	binding.remove("expiresAt");
	require(!journal_.contains("sessionBinding") || journal_["sessionBinding"] == binding);
	require(QDateTime::fromString(s["expiresAt"].toString(), Qt::ISODateWithMs).isValid());
	vault_.save("session-" + id_, QJsonDocument(s).toJson(QJsonDocument::Compact));
	session_ = s;
	journal_["sessionBinding"] = binding;
	save();
}
QJsonObject KeyIngest::bind()
{
	if (journal_.isEmpty()) {
		const auto resolved = bootstrap("resolve", {{"streamKey", QString::fromLatin1(key())}},
						"BroadcastSourceResolvedEnvelope");
		journal_ = {{"version", "k3-2026-10-14.rc1"},
			    {"localId", id_},
			    {"intent", QJsonObject{{"operationKey", id_ + ".bind"},
						   {"recordingId", resolved["recordingId"]},
						   {"expectedSourceRevision", resolved["sourceRevision"]},
						   {"expectedRevision", resolved["broadcastRevision"]},
						   {"protocolVersion", "b1-2026-10-10.rc1"}}}};
		save();
	}
	if (journal_["broadcast"].isObject() && !session_.isEmpty())
		return {{"broadcast", journal_["broadcast"]}, {"receipt", journal_["receipt"]}};
	auto body = journal_["intent"].toObject();
	body["streamKey"] = QString::fromLatin1(key());
	require(validWire("BroadcastSourceBindInput", body));
	auto data = bootstrap("bindings", body, "BroadcastIngestBindingEnvelope");
	const auto b = data["broadcast"].toObject(), binding = b["binding"].toObject(), s = data["session"].toObject();
	require(b["recordingId"] == body["recordingId"] && binding["captureId"] == s["captureId"] &&
		binding["epoch"] == s["epoch"]);
	require(data["receipt"].toObject()["operationKey"] == body["operationKey"] &&
		data["receipt"].toObject()["operation"] == "bind" &&
		data["receipt"].toObject()["state"] != "rejected");
	acceptSession(s);
	journal_["broadcast"] = b;
	journal_["receipt"] = data["receipt"];
	save();
	data.remove("session");
	return data;
}
QByteArray KeyIngest::bearer(bool force)
{
	require(!session_.isEmpty());
	const auto expiry = QDateTime::fromString(session_["expiresAt"].toString(), Qt::ISODateWithMs);
	if (force || QDateTime::currentDateTimeUtc().secsTo(expiry) <= 120)
		acceptSession(bootstrap(
			"session", {{"streamKey", QString::fromLatin1(key())}, {"sessionToken", session_["token"]}},
			"BroadcastIngestSessionEnvelope"));
	return session_["token"].toString().toUtf8();
}
ApiClient KeyIngest::api()
{
	return ApiClient(
		QUrl("https://admin.alive.org.tw/api"), [this](bool f) { return bearer(f); }, transport_,
		[this](const QByteArray &method, const QString &path) { return ingestRoute(method, path, session_); });
}
} // namespace hhc
