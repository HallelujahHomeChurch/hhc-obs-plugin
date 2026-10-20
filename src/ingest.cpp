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
#include <QLockFile>
#include <QDebug>
#include <QCryptographicHash>
#include <QElapsedTimer>
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
		const bool d1 = query.queryItems().size()==3 && query.queryItemValue("controlVersion")=="d1-2026-10-17.rc2";
		require((query.queryItems().size() == 2 || d1) && query.queryItemValue("captureId") == capture &&
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
		const QSet<QString> allowed{"version", "localId",        "intent",   "broadcast",
					    "receipt", "sessionBinding", "sourceId", "terminal", "credentialRef", "terminalProof"};
		for (auto i = journal_.begin(); i != journal_.end(); ++i)
			require(allowed.contains(i.key()));
		const bool k4 = journal_["version"] == "k4-2026-10-15.rc1";
		require((k4 || journal_["version"] == "k3-2026-10-14.rc1") && journal_["localId"] == id_);
		auto intent = journal_["intent"].toObject();
		intent["streamKey"] = "hhc_stream_" + QString(43, 'A');
		if (k4) {
			require(validWire("BroadcastOutputOpenInput", intent) && intent["outputSessionId"] == id_);
			require(!journal_.contains("terminal") || journal_["terminal"].isBool());
			require(!journal_.contains("credentialRef") || journal_["credentialRef"] == "output-key-" + id_);
		} else {
			require(!journal_.contains("terminal") && !journal_.contains("sourceId") &&
				!journal_.contains("credentialRef") && !journal_.contains("terminalProof"));
			require(validWire("BroadcastSourceBindInput", intent) &&
				intent["operationKey"] == id_ + ".bind");
		}
		if (journal_.contains("broadcast")) {
			const auto b = journal_["broadcast"].toObject(), r = journal_["receipt"].toObject();
			require(validWire("BroadcastView", b) && validWire("BroadcastReceipt", r));
			require((k4 || b["recordingId"] == intent["recordingId"]) &&
				r["operationKey"] == id_ + ".bind" && r["operation"] == "bind" &&
				r["state"] != "rejected");
			const auto binding = b["binding"].toObject(), pinned = journal_["sessionBinding"].toObject();
			require(pinned["recordingId"] == b["recordingId"] &&
				pinned["captureId"] == binding["captureId"] && pinned["epoch"] == binding["epoch"]);
			if (k4) {
				auto fakeSession = pinned;
				fakeSession["token"] = "hhc_ingest_placeholder.placeholder";
				fakeSession["expiresAt"] = "2026-10-15T00:00:00Z";
				QJsonObject saved{{"protocolVersion", journal_["version"]},
					     {"outputSessionId", id_},
					     {"sourceId", journal_["sourceId"]},
					     {"broadcast", b},
					     {"receipt", r},
					     {"session", fakeSession}};
				if(intent.contains("mediaProfile")) saved["mediaProfile"]=intent["mediaProfile"];
				checkOpened(saved);
			}
		}
		if (journal_.contains("terminalProof")) {
			checkReplay(journal_["terminalProof"].toObject());
			require(journal_["terminalProof"].toObject()["state"] == "terminal");
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
		require((journal_["version"] == "k4-2026-10-15.rc1"
				 ? journal_["broadcast"].toObject()["recordingId"]
				 : journal_["intent"].toObject()["recordingId"]) == s["recordingId"]);
		session_ = s;
	}
}
void KeyIngest::validateKey(const QByteArray &value)
{
	require(QRegularExpression("^hhc_stream_[A-Za-z0-9_-]{43}$").match(QString::fromLatin1(value)).hasMatch());
}
void KeyIngest::saveKey(const QByteArray &value)
{
	validateKey(value);
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
	QLockFile lock(lockFilePath(root_ + "/" + id_ + ".metadata.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	QFile current(root_ + "/" + id_ + ".json");
	if (current.exists()) {
		require(!QFileInfo(current).isSymLink() && current.size() < 65536 && openSharedJsonRead(current));
		QJsonParseError error;
		const auto saved = QJsonDocument::fromJson(current.readAll(), &error);
		require(error.error == QJsonParseError::NoError && saved.isObject());
		// A late session renewal must not erase another worker's authenticated release proof.
		if (saved.object().contains("terminalProof")) {
			const auto proof = saved.object()["terminalProof"].toObject();
			checkReplay(proof);
			require(proof["state"] == "terminal");
			if (!journal_.contains("terminalProof")) {
				journal_["terminalProof"] = proof;
				checkTerminal();
			}
			require(journal_["terminalProof"] == proof);
		}
		if (saved.object()["terminal"].toBool() && !journal_["terminal"].toBool())
			throw RequestError(409, "ingest_local_conflict");
	}
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
		s["recordingId"] == (journal_["version"] == "k4-2026-10-15.rc1"
					     ? journal_["broadcast"].toObject()["recordingId"]
					     : journal_["intent"].toObject()["recordingId"]));
	auto binding = s;
	binding.remove("token");
	binding.remove("expiresAt");
	require(!journal_.contains("sessionBinding") || journal_["sessionBinding"] == binding);
	require(QDateTime::fromString(s["expiresAt"].toString(), Qt::ISODateWithMs).isValid());
	journal_["sessionBinding"] = binding;
	save();
	vault_.save("session-" + id_, QJsonDocument(s).toJson(QJsonDocument::Compact));
	session_ = s;
}
QJsonObject KeyIngest::bind()
{
	if (journal_.value("version") == "k4-2026-10-15.rc1")
		return open();
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
		data["receipt"].toObject()["operation"] == "bind" && data["receipt"].toObject()["state"] != "rejected");
	acceptSession(s);
	journal_["broadcast"] = b;
	journal_["receipt"] = data["receipt"];
	save();
	data.remove("session");
	return data;
}
void KeyIngest::prepareOpen(const QString &mediaProfile)
{
	require(journal_.isEmpty() && session_.isEmpty());
	QJsonObject intent{{"protocolVersion", "k4-2026-10-15.rc1"}, {"outputSessionId", id_}};
	if (!mediaProfile.isEmpty()) intent["mediaProfile"] = mediaProfile;
	auto check = intent;
	check["streamKey"] = "hhc_stream_" + QString(43, 'A');
	require(validWire("BroadcastOutputOpenInput", check));
	journal_ = {{"version", "k4-2026-10-15.rc1"}, {"localId", id_}, {"intent", intent}};
	save();
	const auto original = key();
	validateKey(original);
	const auto ref = "output-key-" + id_;
	vault_.save(ref, original);
	require(vault_.load(ref) == std::optional<QByteArray>(original));
	journal_["credentialRef"] = ref;
	save();
}
void KeyIngest::checkOpened(const QJsonObject &data) const
{
	if (data.value("mediaProfile") != journal_.value("intent").toObject().value("mediaProfile"))
		throw RequestError(422, "broadcast_protocol_unsupported");
	require(validWire("BroadcastOutputOpened", data) && data["outputSessionId"] == id_);
	const auto b = data["broadcast"].toObject(), binding = b["binding"].toObject(), s = data["session"].toObject();
	require(data["receipt"].toObject()["operationKey"] == id_ + ".bind" && b["recordingId"] == s["recordingId"] &&
		binding["captureId"] == s["captureId"] && binding["epoch"] == s["epoch"]);
	if (journal_.contains("sourceId"))
		require(data["sourceId"] == journal_["sourceId"]);
	if (journal_.contains("broadcast"))
		require(b["recordingId"] == journal_["broadcast"].toObject()["recordingId"] &&
			binding == journal_["broadcast"].toObject()["binding"]);
	if (journal_.contains("sessionBinding")) {
		auto pin = s;
		pin.remove("token");
		pin.remove("expiresAt");
		require(pin == journal_["sessionBinding"]);
	}
}
QJsonObject KeyIngest::open()
{
	try {
		checkTerminal();
		if (journal_.value("terminal").toBool())
			throw RequestError(409, "ingest_local_conflict");
		if (journal_.value("broadcast").isObject()) {
			if (session_.isEmpty()) replay();
			return {{"broadcast", journal_["broadcast"]}, {"receipt", journal_["receipt"]}};
		}
		auto body = journal_["intent"].toObject();
		body["streamKey"] = QString::fromLatin1(outputKey());
		require(validWire("BroadcastOutputOpenInput", body));
		ApiClient client(
			QUrl("https://admin.alive.org.tw/api"), [](bool) { return QByteArray{}; },
			[this](const auto &method, const auto &url, const auto &bytes, const auto &headers) {
				auto response = transport_(method, url, bytes, headers);
				if (response.status >= 200 && response.status < 300 && response.status != 200)
					throw RequestError(response.status, "malformed_success");
				return response;
			});
		auto data =
			client.request("POST", "/ingest/streams/open", body, "BroadcastOutputOpenedEnvelope")["data"]
				.toObject();
		checkOpened(data);
		journal_["broadcast"] = data["broadcast"];
		journal_["receipt"] = data["receipt"];
		journal_["sourceId"] = data["sourceId"];
		auto pin = data["session"].toObject();
		pin.remove("token");
		pin.remove("expiresAt");
		journal_["sessionBinding"] = pin;
		// Pin identity before saving credentials so a failed vault write cannot permit rebinding.
		save();
		acceptSession(data["session"].toObject());
		data.remove("session");
		return data;
	} catch (RequestError &e) {
		e.outputOpening = true;
		throw;
	}
}
QByteArray KeyIngest::outputKey() const
{
	if (journal_["version"] != "k4-2026-10-15.rc1") return key();
	const auto ref = "output-key-" + id_;
	// shortcut: legacy outputs without an original credential reference need separately proven owner migration.
	if (journal_["credentialRef"] != ref)
		throw RequestError(409, "ingest_original_credential_missing");
	const auto original = vault_.load(ref);
	if (!original)
		throw RequestError(409, "ingest_original_credential_missing");
	validateKey(*original);
	return *original;
}
void KeyIngest::checkReplay(const QJsonObject &data) const
{
	const auto pin = journal_["sessionBinding"].toObject();
	require(journal_["version"] == "k4-2026-10-15.rc1" && journal_["credentialRef"] == "output-key-" + id_ &&
		validWire("BroadcastOutputReplayReceipt", journal_["receipt"]) &&
		journal_["receipt"].toObject()["operationKey"] == id_ + ".bind" &&
		validWire(data["state"] == "terminal" ? "BroadcastOutputReplayTerminal" : "BroadcastOutputReplayActive", data));
	require(data["outputSessionId"] == id_ && data["sourceId"] == journal_["sourceId"] &&
		data["recordingId"] == pin["recordingId"] && data["captureId"] == pin["captureId"] &&
		data["epoch"] == pin["epoch"] && data["receipt"] == journal_["receipt"] &&
		data.value("mediaProfile") == journal_["intent"].toObject().value("mediaProfile"));
	if (data["state"] == "active") {
		auto binding = data["session"].toObject();
		binding.remove("token");
		binding.remove("expiresAt");
		require(binding == pin);
	}
}
void KeyIngest::checkTerminal() const
{
	if (!journal_.contains("terminalProof")) return;
	const auto proof = journal_["terminalProof"].toObject();
	checkReplay(proof);
	require(proof["state"] == "terminal");
	RequestError e(0, "output_replay_terminal");
	e.outputReleased = true;
	throw e;
}
void KeyIngest::replay()
{
	checkTerminal();
	const auto pin = journal_["sessionBinding"].toObject();
	QJsonObject body{{"protocolVersion", "k5-2026-10-20.rc1"}, {"outputSessionId", id_},
		{"sourceId", journal_["sourceId"]}, {"recordingId", pin["recordingId"]},
		{"captureId", pin["captureId"]}, {"epoch", pin["epoch"]}, {"receipt", journal_["receipt"]},
		{"streamKey", QString::fromLatin1(outputKey())}};
	if (journal_["intent"].toObject().contains("mediaProfile"))
		body["mediaProfile"] = journal_["intent"].toObject()["mediaProfile"];
	require(validWire("BroadcastOutputReplayInput", body));
	ApiClient client(QUrl("https://admin.alive.org.tw/api"), [](bool) { return QByteArray{}; },
		[this](const auto &method, const auto &url, const auto &bytes, const auto &headers) {
			auto response = transport_(method, url, bytes, headers);
			if (response.status >= 200 && response.status < 300 && response.status != 200)
				throw RequestError(response.status, "malformed_success");
			return response;
		});
	const auto data = client.request("POST", "/ingest/streams/replay", body, "BroadcastOutputReplayEnvelope")["data"].toObject();
	checkReplay(data);
	if (data["state"] == "terminal") {
		const auto before = journal_;
		journal_["terminalProof"] = data;
		try { save(); }
		catch (...) { journal_ = before; throw; }
		checkTerminal();
	}
	acceptSession(data["session"].toObject());
}
QByteArray KeyIngest::bearer(bool force)
{
	checkTerminal();
	if (session_.isEmpty() && journal_["version"] == "k4-2026-10-15.rc1") replay();
	require(!session_.isEmpty());
	const auto expiry = QDateTime::fromString(session_["expiresAt"].toString(), Qt::ISODateWithMs);
	if (force || QDateTime::currentDateTimeUtc().secsTo(expiry) <= 120) {
		const auto output = QString::fromLatin1(QCryptographicHash::hash(id_.toUtf8(), QCryptographicHash::Sha256).toHex().left(16));
		QElapsedTimer elapsed;
		elapsed.start();
		auto logRenew = [&](const char *result, int status = 0) {
			qInfo().noquote() << "[HHC renew] utc=" + QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
				<< "monoMs=" + QString::number(elapsed.msecsSinceReference()) << "elapsedMs=" + QString::number(elapsed.elapsed())
				<< "output=" + output << "forced=" + QString::number(force)
				<< "ttlSec=" + QString::number(QDateTime::currentDateTimeUtc().secsTo(expiry))
				<< "result=" + QString::fromLatin1(result) << "status=" + QString::number(status);
		};
		logRenew("begin");
		try {
			acceptSession(bootstrap(
				"session", {{"streamKey", QString::fromLatin1(outputKey())}, {"sessionToken", session_["token"]}},
				"BroadcastIngestSessionEnvelope"));
			logRenew("renewed", 200);
		} catch (const RequestError &e) {
			logRenew("rejected", e.status);
			if (e.status != 401 || e.code != "broadcast_ingest_session_invalid" ||
			    journal_["version"] != "k4-2026-10-15.rc1")
				throw;
			// Invalid session is not release proof; K5 can only renew or retire this exact original output.
			replay();
		}
	}
	return session_["token"].toString().toUtf8();
}
ApiClient KeyIngest::api()
{
	return ApiClient(
		QUrl("https://admin.alive.org.tw/api"), [this](bool f) { return bearer(f); }, transport_,
		[this](const QByteArray &method, const QString &path) { return ingestRoute(method, path, session_); });
}
} // namespace hhc
