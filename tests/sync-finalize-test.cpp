#include "capture-sync.hpp"
#include "session-store.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	try {
		QFile fixtures("tests/fixtures/c1.json");
		if (!fixtures.open(QIODevice::ReadOnly))
			return 2;
		QJsonObject capture;
		for (const auto &v : QJsonDocument::fromJson(fixtures.readAll()).array())
			if (v.toObject()["name"] == "poll")
				capture = v.toObject()["value"].toObject()["data"].toObject();
		capture["expiresAt"] = "2099-10-08T00:00:00Z";
		capture["liveEnabled"] = false;
		QJsonArray remoteObjects;
		QStringList operations;
		QMap<QString, QByteArray> media, uploaded;
		QJsonObject sealBody;
		bool publish = false;
		QString readyPackage = capture["captureId"].toString();
		QString terminalOverride;
		bool stalePages = false;
		int statusQueries = 0;
		QTemporaryDir root;
		const QString account = "018f0c1f-18d0-7e81-9f6f-69c456db7003", id = "complete-test";
		hhc::SessionStore store(root.path());
		auto directory = store.mediaDirectory(account, id);
		hhc::CaptureJournal local;
		local.account = account;
		local.localId = id;
		local.normalEnd = true;
		local.stopIntent = true;
		auto write = [&](QString path, QByteArray bytes) {
			QDir().mkpath(QFileInfo(directory + "/" + path).absolutePath());
			QFile f(directory + "/" + path);
			if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size())
				throw std::runtime_error("test file");
			f.close();
			media[path] = bytes;
			local.objects.append(
				{path, bytes.size(),
				 QString::fromLatin1(
					 QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()),
				 false});
		};
		write("master.m3u8", "#EXTM3U\n");
		for (const auto &name : QStringList{"1080p", "720p", "480p"}) {
			write(name + "/init.mp4", "SYNTHETIC INIT");
			write(name + "/seg-000000.m4s", "SYNTHETIC CLOSED SEGMENT");
			write(name + "/index.m3u8",
			      "#EXTM3U\n#EXT-X-TARGETDURATION:31\n#EXTINF:30.03,\nseg-000000.m4s\n#EXT-X-ENDLIST\n");
		}
		store.save(local);
		QJsonArray inventoryObjects;
		for (const auto &o : local.objects)
			inventoryObjects.append(QJsonObject{{"path", o.path}, {"size", o.size}, {"sha256", o.sha256}});
		QFile inventory(directory + "/inventory.json");
		if (!inventory.open(QIODevice::WriteOnly))
			return 3;
		inventory.write(
			QJsonDocument(QJsonObject{{"normalEnd", true}, {"objects", inventoryObjects}}).toJson());
		inventory.close();
		QTcpServer server;
		server.listen(QHostAddress::LocalHost, 0);
		QObject::connect(&server, &QTcpServer::newConnection, [&] {
			auto *socket = server.nextPendingConnection();
			auto received = std::make_shared<QByteArray>();
			auto done = std::make_shared<bool>(false);
			QObject::connect(socket, &QTcpSocket::readyRead, [&, socket, received, done] {
				if (*done)
					return;
				*received += socket->readAll();
				auto end = received->indexOf("\r\n\r\n");
				if (end < 0)
					return;
				int length = 0;
				for (auto line : received->left(end).split('\n'))
					if (line.trimmed().toLower().startsWith("content-length:"))
						length = line.trimmed().mid(15).trimmed().toInt();
				if (received->size() < end + 4 + length)
					return;
				*done = true;
				auto first = received->left(received->indexOf("\r\n"));
				auto request = QJsonDocument::fromJson(received->mid(end + 4)).object();
				QJsonObject response;
				capture["serverNow"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
				int status = 200;
				if (first.startsWith("POST /api/admin/recordings HTTP"))
					response = {{"data", QJsonObject{{"id", capture["recordingId"]},
									 {"title", "TEST"},
									 {"uploadedAt", QJsonValue::Null},
									 {"expiresAt", QJsonValue::Null},
									 {"featured", false},
									 {"hidden", false},
									 {"status", "draft"},
									 {"version", 1}}},
						    {"meta", QJsonObject{}}};
				else if (first.contains("/sign ")) {
					QJsonArray signedObjects;
					for (const auto &p : request["paths"].toArray())
						signedObjects.append(QJsonObject{
							{"path", p},
							{"url", "https://upload.invalid/" + p.toString()},
							{"method", "PUT"},
							{"headers",
							 QJsonObject{{"Content-Type",
								      QJsonArray{"application/octet-stream"}},
								     {"Host", QJsonArray{"upload.invalid"}}}}});
					response = {{"data", signedObjects},
						    {"meta", QJsonObject{}},
						    {"error", QJsonValue::Null}};
				} else if (first.startsWith("GET ")) {
					++statusQueries;
					capture["nextCursor"] = stalePages ? "1080p/init.mp4" : "";
					if (!terminalOverride.isEmpty()) {
						capture["state"] = terminalOverride;
						capture["liveState"] = terminalOverride;
						capture["autoPublish"] = "cancelled";
						capture["terminalReason"] = "recording_deleted";
					} else if (!sealBody.empty()) {
						capture["state"] = "ready";
						capture["autoPublish"] = publish ? "published" : "pending";
						capture["packageId"] = readyPackage.isEmpty()
									       ? QJsonValue(QJsonValue::Null)
									       : QJsonValue(readyPackage);
					}
					capture["objects"] = remoteObjects;
					response = {{"data", capture},
						    {"meta", QJsonObject{}},
						    {"error", QJsonValue::Null}};
				} else {
					QString operation = first.contains("/objects ")   ? "declare"
							    : first.contains("/confirm ") ? "confirm"
							    : first.contains("/stop ")    ? "stop"
							    : first.contains("/seal ")    ? "seal"
											  : "create";
					operations << operation;
					if (operation == "declare")
						for (const auto &v : request["objects"].toArray()) {
							auto o = v.toObject();
							o["state"] = "declared";
							remoteObjects.append(o);
						}
					if (operation == "confirm") {
						status = 202;
						for (int i = 0; i < remoteObjects.size(); ++i) {
							auto o = remoteObjects[i].toObject();
							if (uploaded.contains(o["path"].toString()))
								o["state"] = "queued";
							remoteObjects[i] = o;
						}
					}
					if (operation == "stop")
						capture["stopAcceptedAt"] = "2026-10-08T00:00:00Z";
					if (operation == "seal") {
						sealBody = request;
						capture["state"] = "freezing";
						capture["packageId"] = capture["captureId"];
					}
					capture["objects"] = remoteObjects;
					QJsonObject receipt{{"operationKey", request["operationKey"]},
							    {"operation", operation},
							    {"acceptedAt", "2026-10-08T00:00:00Z"},
							    {"captureId", capture["captureId"]}};
					response = {{"data", QJsonObject{{"capture", capture}, {"receipt", receipt}}},
						    {"meta", QJsonObject{}},
						    {"error", QJsonValue::Null}};
				}
				auto body = QJsonDocument(response).toJson(QJsonDocument::Compact);
				socket->write("HTTP/1.1 " + QByteArray::number(status) + " Result\r\nContent-Length: " +
					      QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
				socket->disconnectFromHost();
			});
			QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		});
		hhc::ApiClient api(QUrl(QString("http://127.0.0.1:%1/api").arg(server.serverPort())),
				   [](bool) { return QByteArray("CMS-SECRET"); });
		bool uploadSafe = true, expireOnce = true;
		hhc::CaptureSync sync(root.path(), account, id, api,
				      [&](const QByteArray &method, const QUrl &url, const QByteArray &bytes,
					  const QMap<QByteArray, QByteArray> &headers) {
					      auto path = url.path().mid(1);
					      uploadSafe &= method == "PUT" && media[path] == bytes &&
							    !headers.contains("Authorization") &&
							    !headers.contains("Cookie");
					      if (expireOnce) {
						      expireOnce = false;
						      return hhc::HttpResponse{403, {}};
					      }
					      uploaded[path] = bytes;
					      return hhc::HttpResponse{204, {}};
				      });
		sync.begin("TEST", true, false);
		auto queued = sync.step(false);
		auto sealed = queued;
		bool fencedSeal = false;
		for (int n = 0; n < 20 && !sealed.sealAccepted && !fencedSeal; ++n) {
			try {
				sealed =
				    sync.step(false, [] { throw hhc::RequestError(503, "unavailable"); });
			} catch (const hhc::RequestError &e) {
				fencedSeal = e.status == 503;
			}
		}
		const bool uploadedBeforeFence =
		    uploaded.size() == 10 && operations.contains("stop") && !operations.contains("seal");
		for (int n = 0; n < 20 && !sealed.sealAccepted; ++n)
			sealed = sync.step(false);
		const auto beforeReady = store.loadPending(account).first();
		auto ready = sync.step(false);
		publish = true;
		auto published = sync.step(false);
		int failed = 0;
		auto check = [&](bool ok, const char *name) {
			if (!ok) {
				std::cerr << "FAIL " << name << '\n';
				++failed;
			}
		};
		check(uploadSafe && uploaded.size() == 10, "PUT exact closed bytes with no CMS bearer or cookies");
		check(fencedSeal && uploadedBeforeFence,
		      "B1 reconciliation fence only gates seal after C1 stop and all uploads");
		check(operations.first() == "create" && operations[1] == "stop" && operations.last() == "seal" &&
			      operations.count("declare") == 10 && operations.count("confirm") == 10,
		      "stop is accepted before tail confirmation and seal");
		check(queued.state != "ready" && !queued.sealAccepted, "202 verification queue is not ready");
		check(!beforeReady.confirmedReady && !beforeReady.readyAt.isValid() &&
			      !store.mayCleanup(beforeReady, QDateTime::currentDateTimeUtc().addYears(1)),
		      "freezing and accepted seal never qualify media for cleanup");
		check(sealed.sealAccepted && sealBody["normalEnd"].toBool(), "only normal complete inventory seals");
		check(sealBody["inventory"].toObject()["objects"].toArray().size() == 10 &&
			      sealBody["inventory"]
					      .toObject()["renditions"]
					      .toArray()[0]
					      .toObject()["frameRate"]
					      .toDouble() == 30000.0 / 1001,
		      "canonical inventory actual 29.97 fps");
		check(ready.state == "ready" && ready.autoPublish == "pending" && published.autoPublish == "published",
		      "server ready and automatic publication remain independent");
		const auto confirmed = store.loadPending(account).first();
		check(confirmed.confirmedReady && confirmed.sealAcknowledged &&
			      confirmed.remoteCapture == capture["captureId"] && confirmed.packageId == readyPackage &&
			      confirmed.readyPackage == readyPackage && confirmed.readyAt.isValid(),
		      "accepted seal and server ready persist matching package evidence for retention");
		check(!store.mayCleanup(confirmed, confirmed.readyAt.addDays(7).addMSecs(-1)) &&
			      store.mayCleanup(confirmed, confirmed.readyAt.addDays(7)),
		      "successful capture keeps the complete seven day recovery window");
		const auto firstReadyAt = confirmed.readyAt;
		sync.step(false);
		check(store.loadPending(account).first().readyAt == firstReadyAt,
		      "repeated ready polls do not reset retention clock");
		for (const auto &wrong : QStringList{QString(32, 'f'), QString{}}) {
			readyPackage = wrong;
			bool rejected = false;
			const auto before = operations.size();
			try {
				sync.step(false);
			} catch (const std::exception &) {
				rejected = true;
			}
			const auto retained = store.loadPending(account).first();
			check(rejected && retained.packageId == confirmed.packageId &&
				      retained.readyAt == firstReadyAt && retained.objects.size() == 10 &&
				      operations.size() == before,
			      "changed or absent ready package preserves local evidence and media without mutations");
		}
		readyPackage = confirmed.packageId;
		QFile savedFile(directory + "/remote-journal.json");
		if (!savedFile.open(QIODevice::ReadOnly))
			return 3;
		auto pending = QJsonDocument::fromJson(savedFile.readAll()).object();
		savedFile.close();
		auto mutations = pending["mutations"].toObject();
		auto savedSeal = mutations["seal"].toObject();
		savedSeal["receipt"] = QJsonValue::Null;
		mutations["seal"] = savedSeal;
		pending["mutations"] = mutations;
		for (const auto &terminal : QStringList{"aborted", "failed", "expired"}) {
			QSaveFile file(directory + "/remote-journal.json");
			auto bytes = QJsonDocument(pending).toJson();
			if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
				return 3;
			terminalOverride = terminal;
			stalePages = true;
			const auto queriesBefore = statusQueries;
			hhc::CaptureSync recovery(root.path(), account, id, api);
			const auto before = operations.size();
			auto stopped = recovery.step(false);
			check(statusQueries == queriesBefore + 1,
			      "terminal status ignores stale object pagination after one authoritative read");
			check(stopped.state == terminal && operations.size() == before,
			      "authoritative terminal capture prevents replay of rejected seal intent");
			QFile afterFile(directory + "/remote-journal.json");
			if (!afterFile.open(QIODevice::ReadOnly))
				return 3;
			auto after = QJsonDocument::fromJson(afterFile.readAll()).object();
			check(after["mutations"].toObject()["seal"] == savedSeal,
			      "terminal recovery preserves exact inventory and operation key without a receipt");
		}
		terminalOverride.clear();
		capture["state"] = "uploading";
		capture["liveState"] = "starting";
		capture["autoPublish"] = "pending";
		capture["terminalReason"] = QJsonValue::Null;
		const auto beforeActive = operations.size();
		bool repeatedPageRejected = false;
		try {
			hhc::CaptureSync active(root.path(), account, id, api);
			active.step(false);
		} catch (const std::exception &e) {
			repeatedPageRejected = QByteArray(e.what()) == "Repeated object in status pages";
		}
		check(repeatedPageRejected && operations.size() == beforeActive,
		      "active capture still rejects repeated object pages before stop or seal");
		return failed ? 1 : 0;
	} catch (const std::exception &e) {
		std::cerr << "TEST EXCEPTION " << e.what() << '\n';
		return 9;
	}
}
