#include "capture-sync.hpp"
#include "broadcast-control.hpp"
#include "session-store.hpp"
#include "windows-path.hpp"
#include "wire.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSet>
#include <QLockFile>
#include <QUuid>
#include <algorithm>
namespace hhc {
namespace {
void require(bool ok, const char *why)
{
	if (!ok)
		throw std::runtime_error(why);
}
QJsonObject readJson(const QString &path)
{
	QFile f(path);
	require(!QFileInfo(f).isSymLink() && f.size() <= 8 * 1024 * 1024 && openSharedJsonRead(f),
		"Queue document unavailable");
	QJsonParseError e;
	auto d = QJsonDocument::fromJson(f.readAll(), &e);
	require(e.error == QJsonParseError::NoError && d.isObject(), "Queue document corrupt; data retained");
	return d.object();
}
QString digest(const QByteArray &b)
{
	return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
}
QString schemaFor(const QString &op)
{
	if (op == "create")
		return "RecordingCaptureCreateInput";
	if (op == "declare")
		return "RecordingCaptureDeclareInput";
	if (op == "confirm")
		return "RecordingCaptureConfirmInput";
	if (op == "seal")
		return "RecordingCaptureSealInput";
	if (op == "abort")
		return "RecordingCaptureAbortInput";
	return "RecordingCaptureOperationInput";
}
} // namespace
CaptureSync::CaptureSync(QString root, QString account, QString localId, ApiClient &api, PutTransport put)
	: root_(std::move(root)),
	  account_(std::move(account)),
	  id_(std::move(localId)),
	  api_(api),
	  put_(std::move(put))
{
	load();
}
void CaptureSync::load()
{
	journal_ = {};
	state_ = {};
	auto path = directory() + "/remote-journal.json";
	if (QFileInfo::exists(path)) {
		journal_ = readJson(path);
		const QSet<QString> allowed{"version",     "account",     "localId",     "title",
					    "autoPublish", "liveEnabled", "recordingId", "recordingVersion",
					    "captureId",   "mutations",   "lastCapture"};
		for (auto it = journal_.begin(); it != journal_.end(); ++it)
			require(allowed.contains(it.key()), "Unknown remote journal field");
		require(journal_["version"].toInt() == 1 && journal_["account"] == account_ &&
				journal_["localId"] == id_,
			"Remote journal identity mismatch");
		require(journal_["mutations"].isObject(), "Remote mutations unavailable");
		state_.recordingId = journal_["recordingId"].toString();
		state_.captureId = journal_["captureId"].toString();
		require(journal_["title"].isString() && !journal_["title"].toString().trimmed().isEmpty() &&
				journal_["title"].toString().size() <= 180 && journal_["autoPublish"].isBool() &&
				journal_["liveEnabled"].isBool(),
			"Remote intent malformed");
		require(state_.recordingId.isEmpty() || !QUuid(state_.recordingId).isNull(),
			"Saved recording ID invalid");
		require(state_.captureId.isEmpty() ||
				QRegularExpression("^[a-f0-9]{32}$").match(state_.captureId).hasMatch(),
			"Saved capture ID invalid");
		const auto mutations = journal_["mutations"].toObject();
		for (auto it = mutations.begin(); it != mutations.end(); ++it) {
			auto saved = it.value().toObject();
			auto tag = it.key(), op = tag.startsWith("declare-")   ? QString("declare")
						  : tag.startsWith("confirm-") ? QString("confirm")
									       : tag;
			require(QStringList{"create", "declare", "confirm", "stop", "seal", "abort", "close_live",
					    "cancel_auto_publish"}
						.contains(op) &&
					saved.size() == 4 && saved["body"].isObject() &&
					validWire(schemaFor(op), saved["body"]),
				"Saved mutation invalid");
			auto body = saved["body"].toObject();
			require(body["operationKey"] == id_ + "." + tag, "Saved operation key invalid");
			auto base = "/admin/recordings/" + state_.recordingId + "/captures";
			const auto suffix = op == "declare"               ? QString("/objects")
					    : op == "close_live"          ? QString("/live")
					    : op == "cancel_auto_publish" ? QString("/auto-publish")
									  : "/" + op;
			require(saved["path"] == (op == "create" ? base : base + "/" + state_.captureId + suffix) &&
					saved["method"] ==
						(op == "close_live" || op == "cancel_auto_publish" ? "DELETE" : "POST"),
				"Saved mutation binding invalid");
			if (!saved["receipt"].isNull()) {
				auto receipt = saved["receipt"].toObject();
				require(validWire("RecordingCaptureReceipt", receipt) &&
						receipt["operationKey"] == body["operationKey"] &&
						receipt["operation"] == op && receipt["captureId"] == state_.captureId,
					"Saved receipt invalid");
			}
		}

		if (!journal_["lastCapture"].toObject().empty())
			apply(journal_["lastCapture"].toObject());
	}
}
std::unique_ptr<QLockFile> CaptureSync::lock()
{
	require(QDir().mkpath(directory()) && !QFileInfo(directory()).isSymLink(),
		"Session ownership directory unavailable");
	auto owner = std::make_unique<QLockFile>(lockFilePath(directory() + "/sync.lock"));
	owner->setStaleLockTime(0);
	if (!owner->tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	return owner;
}
SyncState CaptureSync::begin(const QString &t, bool p, bool l)
{
	auto owner = lock();
	return beginImpl(t, p, l);
}
SyncState CaptureSync::step(bool active, std::function<void()> beforeSeal)
{
	auto owner = lock();
	return stepImpl(active, beforeSeal);
}
BroadcastSyncResult CaptureSync::broadcastStep(const QString &recordingId, bool active, bool creating)
{
	SessionStore store(root_);
	if (!QFileInfo::exists(directory() + "/journal.json")) {
		CaptureJournal j;
		j.account = account_;
		j.localId = id_;
		store.save(j);
	}
	BroadcastControl control(directory(), account_, id_, api_);
	BroadcastSyncResult result;
	if (!creating && !state_.captureId.isEmpty()) {
		require(state_.recordingId == recordingId, "Bound recording changed");
		result.sync = step(active, [&] { control.replay(); });
		return result;
	}
	result.broadcast = control.bind(recordingId);
	if (!result.broadcast["binding"].isObject())
		return result;
	result.sync = adopt(result.broadcast);
	if (!creating)
		result.sync = step(active, [&] { control.replay(); });
	return result;
}
SyncState CaptureSync::adopt(const QJsonObject &broadcast)
{
	require(validWire("BroadcastView", broadcast), "Invalid bound broadcast");
	const auto binding = broadcast["binding"].toObject();
	require(binding["actorId"] == account_, "Binding account mismatch");
	auto owner = lock();
	if (journal_.empty()) {
		journal_ = {{"version", 1},
			    {"account", account_},
			    {"localId", id_},
			    {"title", broadcast["title"]},
			    {"autoPublish", broadcast["policy"].toObject()["autoPublish"]},
			    {"liveEnabled", true},
			    {"recordingId", broadcast["recordingId"]},
			    {"recordingVersion", 0},
			    {"captureId", binding["captureId"]},
			    {"mutations", QJsonObject{}},
			    {"lastCapture", QJsonObject{}}};
		state_.recordingId = broadcast["recordingId"].toString();
		state_.captureId = binding["captureId"].toString();
		save();
	}
	require(journal_["recordingId"] == broadcast["recordingId"] &&
		    journal_["captureId"] == binding["captureId"],
		"Existing capture differs from binding");
	poll(); // C1 GET only, never create a parallel capture.
	return state_;
}
SyncState CaptureSync::control(bool live)
{
	persistControl(root_, account_, id_, live);
	auto owner = lock();
	return controlImpl(live);
}
void CaptureSync::persistControl(const QString &root, const QString &account, const QString &id, bool live)
{
	const auto dir = SessionStore(root).mediaDirectory(account, id);
	require(QFileInfo(dir).isDir() && !QFileInfo(dir).isSymLink(), "Control session unavailable");
	const auto path = dir + "/control-intents.json";
	auto intent = QFileInfo::exists(path) ? readJson(path) : QJsonObject{};
	for (auto it = intent.begin(); it != intent.end(); ++it)
		require((it.key() == "close_live" || it.key() == "cancel_auto_publish") &&
				it.value() == id + "." + it.key(),
			"Invalid control intent");
	auto tag = live ? QString("close_live") : QString("cancel_auto_publish");
	intent[tag] = id + "." + tag;
	auto bytes = QJsonDocument(intent).toJson(QJsonDocument::Compact);
	require(writeAtomicMetadata(path, bytes), "Control intent commit failed");
}
QString CaptureSync::directory() const
{
	return SessionStore(root_).mediaDirectory(account_, id_);
}
void CaptureSync::save()
{
	require(QDir().mkpath(directory()) && !QFileInfo(directory()).isSymLink(),
		"Remote queue directory unavailable");
	auto bytes = QJsonDocument(journal_).toJson(QJsonDocument::Compact);
	require(bytes.size() <= 8 * 1024 * 1024, "Remote journal limit");
	require(writeAtomicMetadata(directory() + "/remote-journal.json", bytes),
		"Remote journal commit failed");
}
void CaptureSync::apply(const QJsonObject &capture)
{
	const auto package = journal_["lastCapture"].toObject()["packageId"].toString();
	if (capture["state"] == "ready" && !package.isEmpty())
		require(capture["packageId"] == package, "Ready package differs from sealed package");
	require(state_.recordingId.isEmpty() || capture["recordingId"] == state_.recordingId,
		"Remote recording identity mismatch");
	require(state_.captureId.isEmpty() || capture["captureId"] == state_.captureId,
		"Remote capture identity mismatch");
	state_.recordingId = capture["recordingId"].toString();
	state_.captureId = capture["captureId"].toString();
	state_.state = capture["state"].toString();
	state_.liveState = capture["liveState"].toString();
	state_.liveEnabled = capture["liveEnabled"].toBool();
	state_.autoPublish = capture["autoPublish"].toString();
	state_.stopAccepted = capture["stopAcceptedAt"].isString();
	state_.lastSequence = capture["progress"].toObject()["lastSequence"].toInt(-1);
	auto expires = QDateTime::fromString(capture["expiresAt"].toString(), Qt::ISODateWithMs),
	     now = QDateTime::fromString(capture["serverNow"].toString(), Qt::ISODateWithMs);
	require(expires.isValid() && now.isValid(), "Remote deadline malformed");
	deadline_ = std::min(deadline_, std::chrono::steady_clock::now() +
						std::chrono::milliseconds(std::max<qint64>(0, now.msecsTo(expires))));
	QJsonObject safe;
	for (const auto *key : {"recordingId", "captureId", "state", "liveState", "autoPublish", "liveEnabled",
				"stopAcceptedAt", "packageId", "serverNow", "expiresAt"})
		safe[key] = capture[key];
	safe["progress"] = QJsonObject{{"lastSequence", state_.lastSequence}};
	journal_["lastCapture"] = safe;
	journal_["captureId"] = state_.captureId;
	state_.sealAccepted = journal_["mutations"].toObject()["seal"].toObject()["receipt"].isObject();
}
QJsonObject CaptureSync::mutate(QString tag, QByteArray method, QString path, QJsonObject body, QString operation)
{
	auto mutations = journal_["mutations"].toObject();
	auto saved = mutations[tag].toObject();
	body["operationKey"] = id_ + "." + tag;
	require(validWire(schemaFor(operation), body), "Mutation body violates frozen C1");
	if (!saved.empty()) {
		require(saved.size() == 4 && saved["path"] == path && saved["method"] == QString::fromLatin1(method) &&
				saved["body"] == body,
			"Persisted intent changed; reconcile required");
		if (saved["receipt"].isObject()) {
			auto receipt = saved["receipt"].toObject();
			require(validWire("RecordingCaptureReceipt", receipt) &&
					receipt["operationKey"] == body["operationKey"] &&
					receipt["operation"] == operation && receipt["captureId"] == state_.captureId,
				"Persisted receipt invalid");
			return {{"capture", journal_["lastCapture"]}, {"receipt", saved["receipt"]}};
		}
	} else {
		saved = {{"path", path},
			 {"method", QString::fromLatin1(method)},
			 {"body", body},
			 {"receipt", QJsonValue::Null}};
		mutations[tag] = saved;
		journal_["mutations"] = mutations;
		save();
	}
	auto data = api_.request(method, path, body, "RecordingCaptureResultEnvelope")["data"].toObject(),
	     receipt = data["receipt"].toObject(), capture = data["capture"].toObject();
	require(receipt["operationKey"] == body["operationKey"] && receipt["operation"] == operation &&
			receipt["captureId"] == capture["captureId"],
		"Receipt does not match original intent");
	apply(capture);
	saved["receipt"] = receipt;
	mutations[tag] = saved;
	journal_["mutations"] = mutations;
	save();
	state_.sealAccepted = mutations["seal"].toObject()["receipt"].isObject();
	return data;
}
SyncState CaptureSync::beginImpl(const QString &title, bool publish, bool live)
{
	if (journal_.empty()) {
		require(!title.trimmed().isEmpty() && title.size() <= 180, "Title invalid");
		journal_ = {{"version", 1},
			    {"account", account_},
			    {"localId", id_},
			    {"title", title},
			    {"autoPublish", publish},
			    {"liveEnabled", live},
			    {"recordingId", ""},
			    {"recordingVersion", 0},
			    {"captureId", ""},
			    {"mutations", QJsonObject{}},
			    {"lastCapture", QJsonObject{}}};
		save();
	}
	require(journal_["title"] == title && journal_["autoPublish"] == publish && journal_["liveEnabled"] == live,
		"Original capture intent must be replayed unchanged");
	if (journal_["recordingId"].toString().isEmpty()) {
		auto data = api_.request("POST", "/admin/recordings", {{"title", title}}, "RecordingEnvelope",
					 (id_ + ".title").toUtf8())["data"]
				    .toObject();
		require(!data["id"].toString().isEmpty() && data["version"].toInteger() > 0,
			"Recording success malformed");
		journal_["recordingId"] = data["id"];
		journal_["recordingVersion"] = data["version"];
		state_.recordingId = data["id"].toString();
		save();
	}
	if (journal_["captureId"].toString().isEmpty())
		mutate("create", "POST", "/admin/recordings/" + state_.recordingId + "/captures",
		       {{"expectedVersion", journal_["recordingVersion"]},
			{"autoPublish", publish},
			{"liveEnabled", live}},
		       "create");
	return state_;
}
QJsonObject CaptureSync::poll()
{
	QString cursor;
	QJsonArray objects;
	QSet<QString> cursors, paths;
	QJsonObject capture;
	do {
		QString path =
			"/admin/recordings/" + state_.recordingId + "/captures/" + state_.captureId + "?limit=1000";
		if (!cursor.isEmpty())
			path += "&cursor=" + QString::fromLatin1(QUrl::toPercentEncoding(cursor));
		auto page = api_.request("GET", path, {}, "RecordingCaptureEnvelope")["data"].toObject();
		apply(page);
		capture = page;
		if (QStringList{"aborted", "expired", "failed"}.contains(state_.state))
			break;
		for (const auto &v : page["objects"].toArray()) {
			require(!paths.contains(v.toObject()["path"].toString()), "Repeated object in status pages");
			paths.insert(v.toObject()["path"].toString());
			objects.append(v);
			require(objects.size() <= 10000, "Status object limit");
		}
		cursor = page["nextCursor"].toString();
		if (!cursor.isEmpty()) {
			require(!cursors.contains(cursor), "Status cursor cycle");
			cursors.insert(cursor);
		}
	} while (!cursor.isEmpty());
	capture["objects"] = objects;
	save();
	if (state_.state == "ready" && state_.sealAccepted)
		SessionStore(root_).confirmReady(account_, id_, state_.captureId, capture["packageId"].toString(),
						 QDateTime::fromString(capture["serverNow"].toString(),
								       Qt::ISODateWithMs));
	return capture;
}
SyncState CaptureSync::controlImpl(bool closeLive)
{
	require(!state_.captureId.isEmpty(), "Capture unavailable");
	poll();
	auto base = "/admin/recordings/" + state_.recordingId + "/captures/" + state_.captureId;
	mutate(closeLive ? "close_live" : "cancel_auto_publish", "DELETE",
	       base + (closeLive ? "/live" : "/auto-publish"), {}, closeLive ? "close_live" : "cancel_auto_publish");
	return state_;
}
SyncState CaptureSync::stepImpl(bool active, std::function<void()> beforeSeal)
{
	if (state_.captureId.isEmpty())
		beginImpl(journal_["title"].toString(), journal_["autoPublish"].toBool(),
			  journal_["liveEnabled"].toBool());
	require(!state_.captureId.isEmpty(), "Capture unavailable");
	auto capture = poll();
	auto local = readJson(directory() + "/journal.json");
	require(local["account"] == account_ && local["localId"] == id_, "Local session identity mismatch");
	if (QStringList{"aborted", "expired", "failed"}.contains(state_.state))
		return state_;
	const QString notifyPath = "/admin/recordings/" + state_.recordingId + "/captures/" + state_.captureId;
	if (local["stopIntent"].toBool() && QStringList{"creating", "uploading"}.contains(state_.state))
		mutate("stop", "POST", notifyPath + "/stop", {}, "stop");
	const auto intentPath = directory() + "/control-intents.json";
	if (QFileInfo::exists(intentPath)) {
		auto intent = readJson(intentPath);
		for (auto it = intent.begin(); it != intent.end(); ++it) {
			require((it.key() == "close_live" || it.key() == "cancel_auto_publish") &&
					it.value() == id_ + "." + it.key(),
				"Invalid control intent");
			mutate(it.key(), "DELETE", notifyPath + (it.key() == "close_live" ? "/live" : "/auto-publish"),
			       {}, it.key());
		}
	}
	auto pending = journal_["mutations"].toObject();
	const QString basePath = "/admin/recordings/" + state_.recordingId + "/captures/" + state_.captureId;
	for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
		const auto saved = it.value().toObject();
		if (saved["receipt"].isObject())
			continue;
		const auto tag = it.key();
		const auto op = tag.startsWith("declare-")   ? QString("declare")
				: tag.startsWith("confirm-") ? QString("confirm")
							     : tag;
		if (op == "seal" && state_.state == "uploading")
			continue;
		QString suffix = op == "declare"               ? "/objects"
				 : op == "cancel_auto_publish" ? "/auto-publish"
				 : op == "close_live"          ? "/live"
							       : "/" + op;
		require(QStringList{"declare", "confirm", "stop", "seal", "abort", "cancel_auto_publish", "close_live"}
				.contains(op),
			"Unknown pending intent");
		auto method = op == "close_live" || op == "cancel_auto_publish" ? QByteArray("DELETE")
										: QByteArray("POST");
		if (op == "seal" && beforeSeal)
			beforeSeal();
		mutate(tag, method, basePath + suffix, saved["body"].toObject(), op);
	}
	if (state_.state == "ready" || state_.state == "aborted" || state_.state == "expired" ||
	    state_.state == "failed")
		return state_;
	if (state_.sealAccepted && (state_.state == "freezing" || state_.state == "validating"))
		return state_;
	if (std::chrono::steady_clock::now() >= deadline_)
		throw RequestError(410, "capture_expired");
	auto base = "/admin/recordings/" + state_.recordingId + "/captures/" + state_.captureId;
	if (local["stopIntent"].toBool())
		mutate("stop", "POST", base + "/stop", {}, "stop");
	if (!active && !local["normalEnd"].toBool()) {
		mutate("abort", "POST", base + "/abort", {{"reasonCode", "capture_incomplete"}}, "abort");
		return state_;
	}
	QMap<QString, QJsonObject> remote;
	for (const auto &v : capture["objects"].toArray())
		remote[v.toObject()["path"].toString()] = v.toObject();
	QJsonArray objects, paths;
	state_.pendingBytes = 0;
	bool complete = true;
	for (const auto &v : local["objects"].toArray()) {
		const auto o = v.toObject();
		auto r = remote.value(o["path"].toString());
		if (!r.empty())
			require(r["sha256"] == o["sha256"] && r["sizeBytes"] == o["size"],
				"Remote immutable object differs");
		if (r["state"] == "failed")
			throw RequestError(409, "object_verification_failed");
		if (r["state"] == "queued" || r["state"] == "verified")
			continue;
		complete = false;
		state_.pendingBytes += quint64(o["size"].toInteger());
		if (objects.size() < 1) {
			objects.append(
				QJsonObject{{"path", o["path"]}, {"sizeBytes", o["size"]}, {"sha256", o["sha256"]}});
			paths.append(o["path"]);
		}
	}
	if (!objects.empty()) {
		const auto tag = digest(QJsonDocument(objects).toJson(QJsonDocument::Compact)).left(40);
		mutate("declare-" + tag, "POST", base + "/objects", {{"objects", objects}}, "declare");
		auto signedObjects = api_.request("POST", base + "/sign", {{"paths", paths}},
						  "RecordingCaptureSignedEnvelope")["data"]
					     .toArray();
		require(signedObjects.size() == objects.size(), "Signed object count differs");
		QSet<QString> signedPaths;
		for (const auto &v : signedObjects) {
			auto signedObject = v.toObject();
			for (int capabilityAttempt = 0; capabilityAttempt < 2; ++capabilityAttempt) {
				auto path = signedObject["path"].toString();
				require(paths.contains(path) && !signedPaths.contains(path),
					"Signed object path differs");
				signedPaths.insert(path);
				QUrl url(signedObject["url"].toString());
				require(url.scheme() == "https" && !url.userInfo().size() && !url.hasFragment(),
					"Invalid upload capability");
				const auto info = QFileInfo(directory() + "/" + path);
				require(!info.isSymLink() && info.canonicalFilePath().startsWith(
								     QFileInfo(directory()).canonicalFilePath() + "/",
								     Qt::CaseInsensitive),
					"Upload escaped local queue");
				QFile file(info.filePath());
				require(file.open(QIODevice::ReadOnly) && file.size() <= 134217728,
					"Closed upload object unavailable");
				auto bytes = file.readAll();
				auto o =
					std::find_if(objects.constBegin(), objects.constEnd(), [&](const QJsonValue &x) {
						return x.toObject()["path"] == path;
					})->toObject();
				require(bytes.size() == o["sizeBytes"].toInteger() && digest(bytes) == o["sha256"],
					"Closed upload bytes changed");
				QMap<QByteArray, QByteArray> headers;
				auto issued = signedObject["headers"].toObject();
				for (auto it = issued.begin(); it != issued.end(); ++it) {
					auto key = it.key().toUtf8();
					require(key.toLower() != "authorization", "Unsafe issued Authorization header");
					require(key.toLower() != "cookie", "Unsafe issued Cookie header");
					require(QRegularExpression("^[!#$%&\'*+.^_`|~0-9A-Za-z-]+$")
							.match(it.key())
							.hasMatch(),
						"Invalid upload header name");
					require(!key.toLower().startsWith("x-hhc-"), "Unsafe issued trusted header");
					QByteArray value;
					for (const auto &h : it.value().toArray()) {
						auto part = h.toString().toUtf8();
						require(!part.contains('\r') && !part.contains('\n'),
							"Unsafe upload header value");
						if (!value.isEmpty())
							value += ',';
						value += part;
					}
					if (key.toLower() == "host")
						require(value.toLower() ==
								url.authority(QUrl::FullyEncoded).toUtf8().toLower(),
							"Issued Host differs from upload capability");
					headers[key] = value;
				}
				auto result = put_("PUT", url, bytes, headers);
				if (result.status == 403 && capabilityAttempt == 0) {
					auto renewed = api_.request("POST", base + "/sign",
								    {{"paths", QJsonArray{path}}},
								    "RecordingCaptureSignedEnvelope")["data"]
							       .toArray();
					require(renewed.size() == 1 && renewed[0].toObject()["path"] == path,
						"Renewed capability differs");
					signedObject = renewed[0].toObject();
					signedPaths.remove(path);
					continue;
				}
				if (result.status < 200 || result.status >= 300)
					throw RequestError(result.status, "upload_rejected", result.retryAfter);
				break;
			}
		}
		mutate("confirm-" + tag, "POST", base + "/confirm", {{"paths", paths}}, "confirm");
	}
	if (!active && local["normalEnd"].toBool() && complete) {
		require(local["stopIntent"].toBool() && state_.stopAccepted, "Seal requires durable stop acceptance");
		if (beforeSeal)
			beforeSeal();
		mutate("seal", "POST", base + "/seal",
		       {{"normalEnd", true}, {"inventory", buildInventory(directory())}}, "seal");
	}
	return state_;
}
} // namespace hhc
