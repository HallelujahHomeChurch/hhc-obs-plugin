#include "broadcast-control.hpp"
#include "capture-sync.hpp"
#include "session-store.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int failures = 0;
	auto check = [&](bool ok, const char *why) {
		if (!ok) {
			++failures;
			std::cerr << "FAIL " << why << '\n';
		}
	};
	QFile f("tests/fixtures/b1.json");
	f.open(QIODevice::ReadOnly);
	QJsonObject fixtures;
	for (const auto &v : QJsonDocument::fromJson(f.readAll()).array())
		fixtures[v.toObject()["name"].toString()] = v.toObject()["value"];
	auto broadcast = fixtures["view_draft"].toObject()["data"].toObject();
	const QString recording = broadcast["recordingId"].toString(),
		      actor = "018f0c1f-18d0-7e81-9f6f-69c456db7009";
	QJsonObject binding{{"captureId", QString(32, 'a')}, {"epoch", 1}, {"actorId", actor}};
	auto command = fixtures["control_pending"].toObject()["data"].toObject()["pendingCommand"].toObject();
	QTemporaryDir dir;
	QTcpServer server;
	server.listen(QHostAddress::LocalHost, 0);
	int bindPosts = 0, ackPosts = 0, viewGets = 0, calls = 0;
	bool bound = false, loseAck = true, conflict = false, badProtocol = false, rejected = false,
	     b1Unavailable = false, rejectSuperseded = false;
	int stops = 0, aborts = 0;
	QFile c1File("tests/fixtures/c1.json");
	check(c1File.open(QIODevice::ReadOnly), "C1 fixture");
	QJsonObject c1Poll;
	QJsonObject c1Stop, c1Abort;
	for (const auto &v : QJsonDocument::fromJson(c1File.readAll()).array()) {
		if (v.toObject()["name"] == "poll")
			c1Poll = v.toObject()["value"].toObject()["data"].toObject();
		if (v.toObject()["name"] == "stop_receipt")
			c1Stop = v.toObject()["value"].toObject()["data"].toObject();
		if (v.toObject()["name"] == "abort_receipt")
			c1Abort = v.toObject()["value"].toObject()["data"].toObject();
	}
	QVector<QJsonObject> ackBodies;
	QObject::connect(&server, &QTcpServer::newConnection, [&] {
		auto *s = server.nextPendingConnection();
		auto bytes = std::make_shared<QByteArray>();
		QObject::connect(s, &QTcpSocket::readyRead, [&, s, bytes] {
			*bytes += s->readAll();
			auto headerEnd = bytes->indexOf("\r\n\r\n");
			if (headerEnd < 0)
				return;
			int length = 0;
			for (const auto &line : bytes->left(headerEnd).split('\n'))
				if (line.toLower().startsWith("content-length:"))
					length = line.mid(15).trimmed().toInt();
			if (bytes->size() < headerEnd + 4 + length)
				return;
			++calls;
			const auto first = bytes->left(bytes->indexOf('\r'));
			auto body = QJsonDocument::fromJson(bytes->mid(headerEnd + 4, length)).object();
			QJsonValue data;
			QString errorCode;
			int status = 200;
			if (first.contains("/broadcast-capabilities"))
				data = QJsonObject{
				    {"versions", QJsonArray{badProtocol ? "unsupported" : hhc::b1Version}},
				    {"maxActiveCaptures", 1},
				    {"markerMode", "next-common-segment"}};
			else if (first.startsWith("GET /api/admin/broadcasts?")) {
				check(first.contains("filter=selectable"), "selectable query");
				data = QJsonObject{{"items", QJsonArray{broadcast}},
						   {"nextCursor", QJsonValue::Null}};
			} else if (first.contains("/bindings")) {
				++bindPosts;
				check(body["operationKey"] == "session.bind" &&
					  body["protocolVersion"] == hhc::b1Version,
				      "fixed bind key/wire");
				check(QFile::exists(dir.path() + "/broadcast-journal.json"),
				      "bind committed before request");
				data = QJsonObject{
				    {"broadcast", broadcast},
				    {"receipt", QJsonObject{{"operationKey", body["operationKey"]},
							    {"operation", "bind"},
							    {"state", "accepted"}}}};
				status = 202;
			} else if (first.contains("/commands/")) {
				++ackPosts;
				ackBodies.append(body);
				QFile journal(dir.path() + "/broadcast-journal.json");
				journal.open(QIODevice::ReadOnly);
				check(journal.readAll().contains(
					  QJsonDocument(body).toJson(QJsonDocument::Compact)),
				      "marker committed before ACK");
				check(!body.contains("expectedRevision"), "ACK has no invented revision");
				auto boundaries = broadcast["boundaries"].toObject();
				boundaries[command["kind"] == "start" ? "startSequence"
								      : "endSequenceExclusive"] =
				    body["boundarySequence"];
				broadcast["boundaries"] = boundaries;
				data = QJsonObject{
				    {"broadcast", broadcast},
				    {"receipt", QJsonObject{{"operationKey", body["operationKey"]},
							    {"operation", "ack"},
							    {"state", "completed"},
							    {"commandId", command["commandId"]}}}};
				status = rejected ? 412 : loseAck ? 503 : 202;
				loseAck = false;
				if (rejectSuperseded &&
				    !first.contains(
					("/commands/" + command["commandId"].toString() + "/ack").toUtf8())) {
					status = 409;
					errorCode = "broadcast_state_conflict";
				}
			} else if (first.contains("/control?")) {
				check(first.contains("captureId=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa&epoch=1"),
				      "control binding fence");
				data = QJsonObject{
				    {"revision", 4},
				    {"phase", command["kind"] == "end" ? "end_pending" : "start_pending"},
				    {"pendingCommand", command}};
				if (conflict)
					status = 409;
			} else if (first.contains("/captures/")) {
				if (first.contains("/stop ") || first.contains("/abort ")) {
					const bool stop = first.contains("/stop ");
					stops += stop;
					aborts += !stop;
					auto result = stop ? c1Stop : c1Abort;
					auto receipt = result["receipt"].toObject();
					receipt["operationKey"] = body["operationKey"];
					result["receipt"] = receipt;
					data = result;
				} else {
					check(first.startsWith("GET "), "adoption never POSTs a C1 capture");
					data = c1Poll;
				}
			} else {
				++viewGets;
				if (bound)
					broadcast["binding"] = binding;
				data = broadcast;
			}
			if (b1Unavailable && first.contains("/broadcasts/"))
				status = 503;
			if (rejectSuperseded && first.startsWith("GET /api/admin/broadcasts/") &&
			    !first.contains("/control?")) {
				auto v = broadcast;
				v["pendingCommand"] = command;
				data = v;
			}
			QJsonObject envelope{
			    {"data", data}, {"meta", QJsonObject{}}, {"error", QJsonValue::Null}};
			if (status >= 400)
				envelope = QJsonObject{
				    {"error",
				     QJsonObject{{"code", !errorCode.isEmpty() ? errorCode
							  : status == 409      ? "broadcast_epoch_mismatch"
									       : "unavailable"}}}};
			auto response = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
			s->write("HTTP/1.1 " + QByteArray::number(status) +
				 " Result\r\nContent-Length: " + QByteArray::number(response.size()) +
				 "\r\nConnection: close\r\n\r\n" + response);
			s->disconnectFromHost();
		});
		QObject::connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
	});
	hhc::ApiClient api(QUrl(QString("http://127.0.0.1:%1/api").arg(server.serverPort())),
			   [](bool) { return QByteArray("synthetic-token"); });
	check(hhc::BroadcastControl::selectable(api).size() == 1, "capabilities and selectable broadcasts");
	badProtocol = true;
	bool unsupported = false;
	try {
		hhc::BroadcastControl::selectable(api);
	} catch (...) {
		unsupported = true;
	}
	check(unsupported, "unsupported wire rejected");
	badProtocol = false;
	const char *stage = "bind";
	try {
		hhc::BroadcastControl control(dir.path(), actor, "session", api);
		check(control.bind(recording)["binding"].isNull(), "202/null binding remains pending");
		check(control.bind(recording)["binding"].isNull() && bindPosts == 1,
		      "pending bind queries without duplicate creation");
		stage = "bound";
		bound = true;
		check(control.bind(recording)["binding"] == binding, "original capture adopted");
		stage = "lost-response";
		try {
			control.poll([] { return std::optional<int>{8}; });
		} catch (const hhc::RequestError &e) {
			check(e.status == 503, "lost ACK response retained");
		}
		stage = "restart";
		hhc::BroadcastControl recovered(dir.path(), actor, "session", api);
		recovered.poll([] { return std::optional<int>{99}; });
		check(ackBodies.size() == 2 && ackBodies[0] == ackBodies[1] &&
			  ackBodies[0]["boundarySequence"] == 8,
		      "restart replays original key and marker");
		stage = "end";
		command["kind"] = "end";
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7010";
		recovered.poll([] { return std::optional<int>{20}; });
		check(ackBodies.last()["boundarySequence"] == 20, "end is exclusive encoder marker");
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7011";
		command["status"] = "cancelled";
		stage = "cancel-new";
		auto before = ackPosts;
		recovered.poll([] { return std::optional<int>{21}; });
		check(ackPosts == before, "cancelled command not ACKed");
		stage = "epoch";
		command["status"] = "pending";
		command["epoch"] = 2;
		bool fenced = false;
		try {
			recovered.poll([] { return std::optional<int>{22}; });
		} catch (...) {
			fenced = true;
		}
		check(fenced && ackPosts == before, "old epoch refused");
		command["epoch"] = 1;
		conflict = true;
		before = viewGets;
		try {
			recovered.poll();
		} catch (...) {
		}
		check(viewGets > before, "409 GET reconciliation");
		conflict = false;
		stage = "412";
		rejected = true;
		before = viewGets;
		try {
			recovered.poll([] { return std::optional<int>{22}; });
		} catch (...) {
		}
		check(viewGets > before, "412 ACK GET reconciliation");
		rejected = false;
		stage = "cancel-uncertain";
		command["status"] = "cancelled";
		before = ackPosts;
		recovered.poll();
		check(ackPosts == before, "cancelled uncertain ACK is fenced");
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7012";
		command["status"] = "pending";
		stage = "later-command";
		recovered.poll([] { return std::optional<int>{23}; });
		check(ackPosts == before + 1, "later command does not replay cancelled intent");
		stage = "superseded-between-polls";
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7013";
		loseAck = true;
		try {
			recovered.poll([] { return std::optional<int>{24}; });
		} catch (...) {
		}
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7014";
		rejectSuperseded = true;
		bool advanced = true;
		try {
			recovered.poll([] { return std::optional<int>{25}; });
		} catch (...) {
			advanced = false;
		}
		check(advanced && ackBodies.last()["operationKey"].toString().endsWith(
				      command["commandId"].toString()),
		      "cancel plus replacement between polls does not poison later command");
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7015";
		loseAck = true;
		try {
			recovered.poll([] { return std::optional<int>{26}; });
		} catch (...) {
		}
		command["commandId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7016";
		advanced = true;
		try {
			recovered.replay();
		} catch (...) {
			advanced = false;
		}
		check(advanced, "inactive replay reconciles superseded uncertain command");
		try {
			recovered.poll([] { return std::optional<int>{27}; });
		} catch (...) {
		}
		rejectSuperseded = false;
		stage = "c1-adopt";
		QTemporaryDir queueRoot;
		const QString localId = "session";
		hhc::SessionStore store(queueRoot.path());
		hhc::CaptureJournal local;
		local.account = actor;
		local.localId = localId;
		store.save(local);
		hhc::CaptureSync sync(queueRoot.path(), actor, localId, api);
		const auto c1 = sync.adopt(broadcast);
		check(c1.recordingId == recording && c1.captureId == binding["captureId"].toString(),
		      "bound capture adopted into original C1 uploader");
		hhc::CaptureSync syncRestart(queueRoot.path(), actor, localId, api);
		syncRestart.step(true);
		check(QFile::copy(dir.path() + "/broadcast-journal.json",
				  sync.directory() + "/broadcast-journal.json"),
		      "seed original B1 identity beside C1 uploader");
		b1Unavailable = true;
		const auto beforeViews = viewGets;
		bool independent = true;
		try {
			syncRestart.broadcastStep(recording, true, false);
		} catch (...) {
			independent = false;
		}
		check(independent && viewGets == beforeViews,
		      "established C1 upload/status proceeds during B1-only outage");
		local.stopIntent = true;
		store.save(local);
		try {
			syncRestart.broadcastStep(recording, true, false);
			syncRestart.broadcastStep(recording, false, false);
		} catch (...) {
			independent = false;
		}
		check(independent && stops > 0 && aborts > 0,
		      "B1-only outage cannot block C1 stop or abnormal abort");
		b1Unavailable = false;
		check(bindPosts == 1, "C1 recovery never creates another binding/capture");
		auto replayCalls = calls;
		recovered.replay();
		check(calls == replayCalls, "inactive complete journal does not poll control/view");
		QFile journal(dir.path() + "/broadcast-journal.json");
		check(journal.open(QIODevice::ReadOnly), "durable B1 journal");
		const auto bytes = journal.readAll();
		check(!bytes.contains("synthetic-token") && !bytes.contains("https://") &&
			  !bytes.contains("signed"),
		      "B1 journal excludes credentials and URLs");
		auto network = calls;
		bool wrongAccount = false;
		try {
			hhc::BroadcastControl wrong(dir.path(), recording, "session", api);
			wrong.bind(recording);
		} catch (...) {
			wrongAccount = true;
		}
		check(wrongAccount && calls == network, "journal account fence before network");
	} catch (const std::exception &e) {
		std::cerr << "stage " << stage << "\n";
		check(false, e.what());
	}
	return failures ? 1 : 0;
}
