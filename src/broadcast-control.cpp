#include "broadcast-control.hpp"
#include "wire.hpp"
#include "windows-path.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLockFile>
#include <QSet>
#include <QUuid>
namespace hhc
{
namespace
{
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
BroadcastControl::BroadcastControl(QString d, QString a, QString id, ApiClient &api)
    : directory_(std::move(d)), account_(std::move(a)), id_(std::move(id)), api_(api)
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
	fields(journal_, {"version", "protocolVersion", "account", "localId", "recordingId", "bind",
			  "binding", "commands"});
	require(journal_["version"] == 1 && journal_["protocolVersion"] == b1Version &&
		journal_["account"] == account_ && journal_["localId"] == id_ &&
		!QUuid(journal_["recordingId"].toString()).isNull());
	auto bind = journal_["bind"].toObject();
	fields(bind, {"body", "receipt"});
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
	const auto commands = journal_["commands"].toObject();
	for (auto it = commands.begin(); it != commands.end(); ++it) {
		auto saved = it.value().toObject();
		fields(saved, {"kind", "body", "receipt", "cancelled"});
		require(!saved.contains("cancelled") || saved["cancelled"].isBool());
		require(!QUuid(it.key()).isNull() && (saved["kind"] == "start" || saved["kind"] == "end") &&
			validWire("BroadcastMarkerAckInput", saved["body"]));
		auto body = saved["body"].toObject(), binding = journal_["binding"].toObject();
		require(body["operationKey"] == id_ + ".ack." + it.key() &&
			body["captureId"] == binding["captureId"] && body["epoch"] == binding["epoch"]);
		if (!saved["receipt"].isNull())
			require(validWire("BroadcastReceipt", saved["receipt"]) &&
				saved["receipt"].toObject()["operationKey"] == body["operationKey"] &&
				saved["receipt"].toObject()["commandId"] == it.key() &&
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
	auto v = api_.request("GET", "/admin/broadcasts/" + journal_["recordingId"].toString(), {},
			      "BroadcastViewEnvelope")["data"]
		     .toObject();
	checkBinding(v);
	return v;
}
QJsonObject BroadcastControl::bind(const QString &recordingId)
{
	require(!QUuid(recordingId).isNull() && QDir().mkpath(directory_) &&
		!QFileInfo(directory_).isSymLink());
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	if (journal_.isEmpty()) {
		// Console owns the recording/capture; never call the C1 create endpoints.
		auto v = api_.request("GET", "/admin/broadcasts/" + recordingId, {},
				      "BroadcastViewEnvelope")["data"]
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
			auto data =
			    api_.request("POST", "/admin/broadcasts/" + recordingId + "/bindings",
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
			if (e.status == 409 || e.status == 412)
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
					 "/admin/broadcasts/" + journal_["recordingId"].toString() +
					     "/commands/" + commandId + "/ack",
					 saved["body"].toObject(), "BroadcastMutationResultEnvelope")["data"]
				.toObject();
		auto receipt = data["receipt"].toObject();
		require(receipt["operationKey"] == saved["body"].toObject()["operationKey"] &&
			receipt["operation"] == "ack" && receipt["commandId"] == commandId &&
			receipt["state"] != "rejected");
		checkBinding(data["broadcast"].toObject());
		saved["receipt"] = receipt;
		auto commands = journal_["commands"].toObject();
		commands[commandId] = saved;
		journal_["commands"] = commands;
		save();
		return data["broadcast"].toObject();
	} catch (const RequestError &e) {
		if (e.status == 409 || e.status == 412) {
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
QJsonObject BroadcastControl::poll(std::function<std::optional<int>()> nextBoundary)
{
	QLockFile lock(lockFilePath(directory_ + "/broadcast.lock"));
	lock.setStaleLockTime(0);
	if (!lock.tryLock(0))
		throw RequestError(409, "local_session_busy");
	load();
	require(journal_["binding"].isObject());
	auto binding = journal_["binding"].toObject();
	QJsonObject control;
	try {
		control = api_.request("GET",
				       "/admin/broadcasts/" + journal_["recordingId"].toString() +
					   "/control?captureId=" + binding["captureId"].toString() +
					   "&epoch=" + QString::number(binding["epoch"].toInteger()),
				       {}, "BroadcastControlEnvelope")["data"]
			      .toObject();
	} catch (const RequestError &e) {
		if (e.status == 409 || e.status == 412)
			view();
		throw;
	}
	auto command = control["pendingCommand"].toObject(), commands = journal_["commands"].toObject();
	const auto currentId = command["commandId"].toString();
	if (commands.contains(currentId) &&
	    (command["status"] == "cancelled" || command["status"] == "rejected")) {
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
					command["epoch"] == binding["epoch"] &&
					command["kind"] == saved["kind"]);
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
