#include "broadcast-control.hpp"
#include "wire.hpp"
#include "windows-path.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QSet>
#include <QUuid>
namespace hhc {
namespace {
void require(bool ok)
{
	if (!ok)
		throw RequestError(409, "broadcast_local_conflict");
}
void fields(const QJsonObject &o, const QSet<QString> &allowed)
{
	for (auto it = o.begin(); it != o.end(); ++it)
		require(allowed.contains(it.key()));
}
} // namespace
bool d2FallbackAllowed(const RequestError &error)
{
	return !error.outputReplayUnknown && !error.outputReleased && error.status == 422 &&
		error.code == "broadcast_protocol_unsupported";
}
QJsonObject acceptD2Observation(const QJsonObject &previous, const QJsonObject &incoming,
			      const QJsonObject &identity, quint64 requestGeneration, quint64 currentGeneration)
{
	if (requestGeneration != currentGeneration) return {};
	auto check = [&](const QJsonObject &c) {
		require(validWire("BroadcastControlD2", c));
		for (const auto *key : {"recordingId", "captureId", "epoch"}) require(c[key] == identity[key]);
		const auto bounds = c["boundaries"].toObject(), end = c["publicEnd"].toObject();
		require(bounds["startSequence"].isNull() || bounds["endSequenceExclusive"].isNull() ||
			bounds["startSequence"].toInteger() <= bounds["endSequenceExclusive"].toInteger());
		if (!end.isEmpty() && !end["endSequenceExclusive"].isNull())
			require(end["endSequenceExclusive"] == bounds["endSequenceExclusive"]);
	};
	check(incoming);
	if (previous.isEmpty()) return incoming;
	check(previous);
	const auto bounds = incoming["boundaries"].toObject(), oldBounds = previous["boundaries"].toObject();
	if (incoming["revision"].toInteger() < previous["revision"].toInteger() ||
	    bounds["revision"].toInteger() < oldBounds["revision"].toInteger()) return {};
	for (const auto *key : {"startSequence", "endSequenceExclusive"})
		require(oldBounds[key].isNull() || bounds[key] == oldBounds[key]);
	if (bounds["revision"] == oldBounds["revision"]) require(bounds == oldBounds);
	auto base = [](QJsonObject c) { c.remove("recovery"); c.remove("publicEnd"); return c; };
	if (incoming["revision"] == previous["revision"]) require(base(incoming) == base(previous));
	auto accepted = incoming;
	if (previous["publicEnd"].isObject()) {
		if (incoming["publicEnd"].isObject())
			require(incoming["publicEnd"].toObject()["endSequenceExclusive"] == previous["publicEnd"].toObject()["endSequenceExclusive"]);
		accepted["publicEnd"] = previous["publicEnd"];
	}
	return accepted;
}
BroadcastControl::BroadcastControl(QString d, QString a, QString id, ApiClient &api, bool ingest)
	: directory_(std::move(d)),
	  account_(std::move(a)),
	  id_(std::move(id)),
	  api_(api),
	  ingest_(ingest)
{
	require(!QUuid(account_).isNull() && !id_.isEmpty() && id_.size() <= 64);
}
void BroadcastControl::load()
{
	journal_ = {};
	const auto path = directory_ + "/broadcast-journal.json";
	if (!QFileInfo::exists(path))
		return;
	QFile f(path);
	require(!QFileInfo(f).isSymLink() && f.size() <= 8 * 1024 * 1024 && openSharedJsonRead(f));
	QJsonParseError error;
	const auto doc = QJsonDocument::fromJson(f.readAll(), &error);
	require(error.error == QJsonParseError::NoError && doc.isObject());
	journal_ = doc.object();
	fields(journal_,
	       {"version", "protocolVersion", "account", "localId", "recordingId", "bind", "binding", "commands", "controlD1"});
	require(journal_["version"] == 1 && journal_["protocolVersion"] == b1Version &&
		journal_["account"] == account_ && journal_["localId"] == id_ &&
		!QUuid(journal_["recordingId"].toString()).isNull());
	auto bind = journal_["bind"].toObject();
	fields(bind, {"body", "receipt"});
	const bool opened = bind["body"].toObject()["protocolVersion"] == "k4-2026-10-15.rc1";
	if (opened) {
		auto input = bind["body"].toObject();
		input["streamKey"] = "hhc_stream_" + QString(43, 'A');
		require(ingest_ && validWire("BroadcastOutputOpenInput", input) && input["outputSessionId"] == id_ &&
			bind["receipt"].toObject()["state"] == "completed" && journal_["binding"].isObject());
	} else
		require(validWire("BroadcastBindInput", bind["body"]) &&
			bind["body"].toObject()["operationKey"] == id_ + ".bind" &&
			bind["body"].toObject()["protocolVersion"] == b1Version);
	if (!bind["receipt"].isNull())
		require(validWire("BroadcastReceipt", bind["receipt"]) &&
			bind["receipt"].toObject()["operationKey"] == id_ + ".bind" &&
			bind["receipt"].toObject()["operation"] == "bind");
	if (!journal_["binding"].isNull())
		require(validWire("BroadcastBinding", journal_["binding"]) &&
			journal_["binding"].toObject()["actorId"] == account_);
	require(journal_["commands"].isObject());
	if (journal_.contains("controlD1")) {
		const auto c=journal_["controlD1"].toObject(), b=journal_["binding"].toObject();
		require(validWire("BroadcastControlD1",c) && c["recordingId"]==journal_["recordingId"] &&
			c["captureId"]==b["captureId"] && c["epoch"]==b["epoch"]);
	}
	const auto commands = journal_["commands"].toObject();
	for (auto it = commands.begin(); it != commands.end(); ++it) {
		auto saved = it.value().toObject();
		fields(saved, {"kind", "body", "receipt", "cancelled"});
		require(!saved.contains("cancelled") || saved["cancelled"].isBool());
		require(!QUuid(it.key()).isNull() && (saved["kind"] == "start" || saved["kind"] == "end") &&
			validWire("BroadcastMarkerAckInput", saved["body"]));
		auto body = saved["body"].toObject(), binding = journal_["binding"].toObject();
		require(body["operationKey"] == id_ + ".ack." + it.key() && body["captureId"] == binding["captureId"] &&
			body["epoch"] == binding["epoch"]);
		if (!saved["receipt"].isNull())
			require(validWire("BroadcastReceipt", saved["receipt"]) &&
				saved["receipt"].toObject()["operationKey"] == body["operationKey"] &&
				(!saved["receipt"].toObject().contains("commandId") ||
				 saved["receipt"].toObject()["commandId"] == it.key()) &&
				saved["receipt"].toObject()["operation"] == "ack");
	}
}
void BroadcastControl::save()
{
	require(writeAtomicMetadata(directory_ + "/broadcast-journal.json",
				    QJsonDocument(journal_).toJson(QJsonDocument::Compact)));
}
QJsonArray BroadcastControl::selectable(ApiClient &api)
{
	auto capabilities =
		api.request("GET", "/admin/broadcast-capabilities", {}, "BroadcastCapabilitiesEnvelope")["data"]
			.toObject();
	if (!capabilities["versions"].toArray().contains(b1Version))
		throw RequestError(409, "broadcast_protocol_unsupported");
	QJsonArray items;
	QString cursor;
	QSet<QString> cursors, ids;
	do {
		auto path = QString("/admin/broadcasts?filter=selectable&limit=100");
		if (!cursor.isEmpty())
			path += "&cursor=" + QString::fromLatin1(QUrl::toPercentEncoding(cursor));
		auto page = api.request("GET", path, {}, "BroadcastListEnvelope")["data"].toObject();
		for (const auto &v : page["items"].toArray()) {
			auto id = v.toObject()["recordingId"].toString();
			require(!ids.contains(id) && items.size() < 1000);
			ids.insert(id);
			items.append(v);
		}
		cursor = page["nextCursor"].toString();
		require(cursor.isEmpty() || !cursors.contains(cursor));
		cursors.insert(cursor);
	} while (!cursor.isEmpty());
	return items;
}
void BroadcastControl::checkBinding(const QJsonObject &broadcast)
{
	require(broadcast["recordingId"] == journal_["recordingId"]);
	auto binding = broadcast["binding"];
	if (!journal_["binding"].isNull())
		require(binding == journal_["binding"]);
	if (binding.isObject()) {
		require(binding.toObject()["actorId"] == account_);
		journal_["binding"] = binding;
		save();
	}
}
QJsonObject BroadcastControl::view()
{
	if (ingest_) {
		const auto b = journal_["binding"].toObject();
		return api_
			.request("GET",
				 "/admin/broadcasts/" + journal_["recordingId"].toString() +
					 "/control?captureId=" + b["captureId"].toString() +
					 "&epoch=" + QString::number(b["epoch"].toInteger()),
				 {}, "BroadcastControlEnvelope")["data"]
			.toObject();
	}
	auto v = api_.request("GET", "/admin/broadcasts/" + journal_["recordingId"].toString(), {},
			      "BroadcastViewEnvelope")["data"]
			 .toObject();
	checkBinding(v);
	return v;
}
void BroadcastControl::adoptIngest(const QJsonObject &result, const QJsonObject &intent)
{
	require(ingest_ && QDir().mkpath(directory_));
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	const auto b = result["broadcast"].toObject();
	const bool opened = intent["protocolVersion"] == "k4-2026-10-15.rc1";
	if (opened) {
		auto input = intent;
		input["streamKey"] = "hhc_stream_" + QString(43, 'A');
		require(validWire("BroadcastOutputOpenInput", input) && input["outputSessionId"] == id_ &&
			result["receipt"].toObject()["operation"] == "bind" &&
			result["receipt"].toObject()["state"] == "completed");
	}
	require(validWire("BroadcastView", b) && b["binding"].isObject() &&
		(opened || b["recordingId"] == intent["recordingId"]) &&
		b["binding"].toObject()["actorId"] == account_ && validWire("BroadcastReceipt", result["receipt"]) &&
		result["receipt"].toObject()["operationKey"] == id_ + ".bind");
	if (!journal_.isEmpty()) {
		checkBinding(b);
		return;
	}
	journal_ = {{"version", 1},
		    {"protocolVersion", b1Version},
		    {"account", account_},
		    {"localId", id_},
		    {"recordingId", b["recordingId"]},
		    {"binding", b["binding"]},
		    {"commands", QJsonObject{}},
		    {"bind", QJsonObject{{"body", opened ? intent
							 : QJsonObject{{"operationKey", intent["operationKey"]},
								       {"expectedRevision", intent["expectedRevision"]},
								       {"protocolVersion", b1Version}}},
					 {"receipt", result["receipt"]}}}};
	save();
}
std::optional<int> BroadcastControl::acceptedEnd()
{
	load();
	for (const auto &v : journal_["commands"].toObject()) {
		const auto s = v.toObject();
		const auto state = s["receipt"].toObject()["state"].toString();
		if (s["kind"] == "end" && !s["cancelled"].toBool() && (state == "accepted" || state == "completed"))
			return s["body"].toObject()["boundarySequence"].toInt();
	}
	return {};
}
QJsonObject BroadcastControl::acceptControl(const QJsonObject &c)
{
	const auto binding=journal_["binding"].toObject(), bounds=c["boundaries"].toObject();
	require(validWire("BroadcastControlD1",c) && c["recordingId"]==journal_["recordingId"] &&
		c["captureId"]==binding["captureId"] && c["epoch"]==binding["epoch"]);
	require(bounds["startSequence"].isNull() || bounds["endSequenceExclusive"].isNull() ||
		bounds["startSequence"].toInteger() <= bounds["endSequenceExclusive"].toInteger());
	const auto old=journal_["controlD1"].toObject(), previous=old["boundaries"].toObject();
	if (!old.isEmpty()) {
		if(c["revision"].toInteger()<old["revision"].toInteger() || bounds["revision"].toInteger()<previous["revision"].toInteger()) return old;
		if(c["revision"]==old["revision"]) require(c==old);
		for(const auto *field : {"startSequence","endSequenceExclusive"})
			require(previous[field].isNull() || bounds[field]==previous[field]);
		if(bounds["revision"]==previous["revision"]) require(bounds==previous);
	}
	journal_["controlD1"]=c;
	save();
	return c;
}
std::optional<int> BroadcastControl::serverEnd()
{
	load();
	const auto c=journal_["controlD1"].toObject(), bounds=c["boundaries"].toObject();
	if (!c.isEmpty() && !bounds["endSequenceExclusive"].isNull() &&
	    QStringList{"end_pending","processing","archived"}.contains(c["phase"].toString()))
		return bounds["endSequenceExclusive"].toInt();
	return {};
}
QJsonObject BroadcastControl::bind(const QString &recordingId)
{
	require(!QUuid(recordingId).isNull() && QDir().mkpath(directory_) && !QFileInfo(directory_).isSymLink());
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	if (journal_.isEmpty()) {
		// Console owns the recording/capture; never call the C1 create endpoints.
		auto v = api_.request("GET", "/admin/broadcasts/" + recordingId, {}, "BroadcastViewEnvelope")["data"]
				 .toObject();
		require(v["recordingId"] == recordingId && v["binding"].isNull());
		journal_ = {{"version", 1},
			    {"protocolVersion", b1Version},
			    {"account", account_},
			    {"localId", id_},
			    {"recordingId", recordingId},
			    {"binding", QJsonValue::Null},
			    {"commands", QJsonObject{}},
			    {"bind", QJsonObject{{"body", QJsonObject{{"operationKey", id_ + ".bind"},
								      {"expectedRevision", v["revision"]},
								      {"protocolVersion", b1Version}}},
						 {"receipt", QJsonValue::Null}}}};
		save();
	}
	require(journal_["recordingId"] == recordingId);
	auto intent = journal_["bind"].toObject();
	if (intent["receipt"].isNull()) {
		try {
			auto data = api_.request("POST", "/admin/broadcasts/" + recordingId + "/bindings",
						 intent["body"].toObject(), "BroadcastMutationResultEnvelope")["data"]
					    .toObject();
			auto receipt = data["receipt"].toObject();
			require(receipt["operationKey"] == id_ + ".bind" && receipt["operation"] == "bind" &&
				receipt["state"] != "rejected");
			checkBinding(data["broadcast"].toObject());
			intent["receipt"] = receipt;
			journal_["bind"] = intent;
			save();
		} catch (const RequestError &e) {
			if (!e.outputReplayUnknown && !e.outputReleased && (e.status == 409 || e.status == 412))
				view();
			throw;
		}
	}
	return view();
}
QJsonObject BroadcastControl::ack(const QString &commandId, QJsonObject saved)
{
	try {
		auto data = api_.request("POST",
					 "/admin/broadcasts/" + journal_["recordingId"].toString() + "/commands/" +
						 commandId + "/ack",
					 saved["body"].toObject(), "BroadcastMutationResultEnvelope")["data"]
				    .toObject();
		auto receipt = data["receipt"].toObject();
		require(receipt["operationKey"] == saved["body"].toObject()["operationKey"] &&
			receipt["operation"] == "ack" &&
			(!receipt.contains("commandId") || receipt["commandId"] == commandId) &&
			receipt["state"] != "rejected");
		checkBinding(data["broadcast"].toObject());
		saved["receipt"] = receipt;
		auto commands = journal_["commands"].toObject();
		commands[commandId] = saved;
		journal_["commands"] = commands;
		save();
		return data["broadcast"].toObject();
	} catch (const RequestError &e) {
		if (!e.outputReplayUnknown && !e.outputReleased && (e.status == 409 || e.status == 412)) {
			const auto current = view();
			// A rejected stale command must not poison the next command. Other conflicts stay
			// fenced.
			if (e.code == "broadcast_state_conflict" &&
			    current["pendingCommand"].toObject()["commandId"] != commandId) {
				saved["cancelled"] = true;
				auto commands = journal_["commands"].toObject();
				commands[commandId] = saved;
				journal_["commands"] = commands;
				save();
				return current;
			}
		}
		throw;
	}
}
void BroadcastControl::replay()
{
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	require(journal_["binding"].isObject());
	const auto savedCommands = journal_["commands"].toObject();
	bool pending = false;
	for (const auto &v : savedCommands) {
		const auto s = v.toObject();
		pending |= !s.value("cancelled").toBool() && s["receipt"].toObject()["state"] != "completed";
	}
	if (!pending)
		return;
	const auto broadcast = view();
	const auto command = broadcast["pendingCommand"].toObject();
	auto commands = journal_["commands"].toObject();
	for (auto it = commands.begin(); it != commands.end(); ++it) {
		auto saved = it.value().toObject();
		if (saved.value("cancelled").toBool() || saved["receipt"].toObject()["state"] == "completed")
			continue;
		if (command["commandId"] == it.key() &&
		    (command["status"] == "cancelled" || command["status"] == "rejected")) {
			saved["cancelled"] = true;
			auto updated = journal_["commands"].toObject();
			updated[it.key()] = saved;
			journal_["commands"] = updated;
			save();
			continue;
		}
		ack(it.key(), saved); // Reconcile only saved markers; never choose a new inactive marker.
	}
}
QJsonObject BroadcastControl::poll(std::function<std::optional<int>()> nextBoundary, bool d1)
{
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	// Let the independent control reader finish without restarting an adopted upload round.
	if (!lock.tryLock(2000))
		throw RequestError(409, "local_session_busy");
	load();
	require(journal_["binding"].isObject());
	auto binding = journal_["binding"].toObject();
	QJsonObject control;
	try {
		control = api_.request("GET",
				       "/admin/broadcasts/" + journal_["recordingId"].toString() +
					       "/control?captureId=" + binding["captureId"].toString() +
					       "&epoch=" + QString::number(binding["epoch"].toInteger()) +
					       (d1 ? "&controlVersion=d1-2026-10-17.rc2" : ""),
				       {}, d1 ? "BroadcastControlD1Envelope" : "BroadcastControlEnvelope")["data"]
				  .toObject();
	} catch (const RequestError &e) {
		if (!e.outputReplayUnknown && !e.outputReleased && (e.status == 409 || e.status == 412))
			view();
		throw;
	}
	if(d1) {
		control=acceptControl(control);
		if(!control["boundaries"].toObject()["endSequenceExclusive"].isNull()) return control;
	}
	auto command = control["pendingCommand"].toObject(), commands = journal_["commands"].toObject();
	const auto currentId = command["commandId"].toString();
	if (commands.contains(currentId) && (command["status"] == "cancelled" || command["status"] == "rejected")) {
		auto saved = commands[currentId].toObject();
		saved["cancelled"] = true;
		commands[currentId] = saved;
		journal_["commands"] = commands;
		save();
	}
	// Replay a committed ACK even if the successful server response was lost.
	for (auto it = commands.begin(); it != commands.end(); ++it) {
		auto saved = it.value().toObject();
		if (saved.value("cancelled").toBool())
			continue;
		if (saved["receipt"].isNull() || saved["receipt"].toObject()["state"] == "accepted") {
			if (command["commandId"] == it.key() &&
			    (command["status"] == "cancelled" || command["status"] == "rejected"))
				continue;
			if (command["commandId"] == it.key())
				require(command["captureId"] == binding["captureId"] &&
					command["epoch"] == binding["epoch"] && command["kind"] == saved["kind"]);
			ack(it.key(), saved);
		}
	}
	if (command.empty() || command["status"] != "pending")
		return control;
	require(command["captureId"] == binding["captureId"] && command["epoch"] == binding["epoch"]);
	const auto commandId = command["commandId"].toString();
	commands = journal_["commands"].toObject();
	if (commands.contains(commandId))
		return control;
	auto boundary = nextBoundary ? nextBoundary() : std::nullopt;
	if (!boundary)
		return control;
	QJsonObject body{{"operationKey", id_ + ".ack." + commandId},
			 {"captureId", binding["captureId"]},
			 {"epoch", binding["epoch"]},
			 {"boundarySequence", *boundary}};
	require(validWire("BroadcastMarkerAckInput", body));
	QJsonObject saved{{"kind", command["kind"]}, {"body", body}, {"receipt", QJsonValue::Null}};
	commands[commandId] = saved;
	journal_["commands"] = commands;
	save();
	ack(commandId, saved);
	return control;
}
} // namespace hhc
