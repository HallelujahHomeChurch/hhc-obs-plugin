#include "capture-sync.hpp"
#include "session-store.hpp"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QLockFile>
#include <QCryptographicHash>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	try {
		QFile f("tests/fixtures/c1.json");
		f.open(QIODevice::ReadOnly);
		QJsonObject create, poll, abort, stop;
		for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
			auto o = v.toObject();
			if (o["name"] == "create_receipt")
				create = o["value"].toObject();
			if (o["name"] == "poll")
				poll = o["value"].toObject();
			if (o["name"] == "stop_receipt")
				stop = o["value"].toObject();
			if (o["name"] == "abort_receipt")
				abort = o["value"].toObject();
		}
		QTcpServer server;
		server.listen(QHostAddress::LocalHost, 0);
		int titleCreates = 0, captureCreates = 0, aborts = 0, stops = 0;
		bool keysPresent = true, declarationsUnavailable = false;
		QVector<QJsonObject> declarations;
		QByteArray seen;
		QObject::connect(&server, &QTcpServer::newConnection, [&] {
			auto *s = server.nextPendingConnection();
			auto b = std::make_shared<QByteArray>();
			auto done = std::make_shared<bool>(false);
			QObject::connect(s, &QTcpSocket::readyRead, [&, s, b, done] {
				if (*done)
					return;
				*b += s->readAll();
				int h = b->indexOf("\r\n\r\n");
				if (h < 0)
					return;
				int length = 0;
				for (const auto &line : b->left(h).split('\n'))
					if (line.trimmed().toLower().startsWith("content-length:"))
						length = line.trimmed().mid(15).trimmed().toInt();
				if (b->size() < h + 4 + length)
					return;
				*done = true;
				seen = *b;
				auto request = QJsonDocument::fromJson(b->mid(h + 4)).object();
				const auto line = b->left(b->indexOf("\r\n"));
				QJsonObject response;
				if (line.startsWith("POST /api/admin/recordings HTTP")) {
					++titleCreates;
					keysPresent &= b->contains("Idempotency-Key:");
					response = {{"data", QJsonObject{{"id", "018f0c1f-18d0-7e81-9f6f-69c456db7003"},
									 {"title", "SYNTHETIC"},
									 {"uploadedAt", QJsonValue::Null},
									 {"expiresAt", QJsonValue::Null},
									 {"featured", false},
									 {"hidden", false},
									 {"status", "draft"},
									 {"version", 1}}},
						    {"meta", QJsonObject{}},
						    {"error", QJsonValue::Null}};
				} else if (line.contains("/objects ") && declarationsUnavailable) {
					declarations.append(request);
					QByteArray body =
						"{\"data\":null,\"meta\":{},\"error\":{\"code\":\"capture_unavailable\",\"message\":\"test uncertainty\"}}";
					s->write("HTTP/1.1 503 Unavailable\r\nContent-Length: " +
						 QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" +
						 body);
					s->disconnectFromHost();
					return;
				} else if (line.startsWith("DELETE ")) {
					response = create;
					auto data = response["data"].toObject(), r = data["receipt"].toObject(),
					     c = data["capture"].toObject();
					r["operationKey"] = request["operationKey"];
					r["operation"] = "cancel_auto_publish";
					c["autoPublish"] = "cancelled";
					data["capture"] = c;
					data["receipt"] = r;
					response["data"] = data;
				} else if (line.contains("/stop ")) {
					++stops;
					response = stop;
					auto data = response["data"].toObject(), r = data["receipt"].toObject();
					r["operationKey"] = request["operationKey"];
					data["receipt"] = r;
					response["data"] = data;
				} else if (line.contains("/abort ")) {
					++aborts;
					response = abort;
					auto data = response["data"].toObject(), r = data["receipt"].toObject();
					r["operationKey"] = request["operationKey"];
					data["receipt"] = r;
					response["data"] = data;
				} else if (line.startsWith("POST ")) {
					++captureCreates;
					keysPresent &= request["operationKey"].isString();
					response = create;
					auto data = response["data"].toObject(), r = data["receipt"].toObject();
					r["operationKey"] = request["operationKey"];
					data["receipt"] = r;
					response["data"] = data;
				} else
					response = poll;
				auto data = response["data"].toObject();
				if (data.contains("capture")) {
					auto c = data["capture"].toObject();
					c["expiresAt"] = "2099-10-08T00:00:00Z";
					c["serverNow"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
					data["capture"] = c;
				} else if (data.contains("captureId")) {
					data["expiresAt"] = "2099-10-08T00:00:00Z";
					data["serverNow"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
				}
				response["data"] = data;
				QByteArray body = QJsonDocument(response).toJson(QJsonDocument::Compact);
				s->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) +
					 "\r\nConnection: close\r\n\r\n" + body);
				s->disconnectFromHost();
			});
			QObject::connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
		});
		QTemporaryDir root;
		QString account = "018f0c1f-18d0-7e81-9f6f-69c456db7003", id = "local-fixed";
		hhc::SessionStore store(root.path());
		hhc::CaptureJournal j;
		j.account = account;
		j.localId = id;
		store.save(j);
		hhc::ApiClient api(QUrl(QString("http://127.0.0.1:%1/api").arg(server.serverPort())),
				   [](bool) { return QByteArray("synthetic-token"); });
		int errors = 0;
		auto check = [&](bool ok, const char *n) {
			if (!ok) {
				std::cerr << "FAIL " << n << '\n';
				++errors;
			}
		};
		hhc::CaptureSync sync(root.path(), account, id, api);
		auto state = sync.begin("SYNTHETIC", false, false);
		check(!state.recordingId.isEmpty() && state.captureId == QString(32, 'a'),
		      "recording and capture acceptance parsed");
		hhc::CaptureSync restart(root.path(), account, id, api);
		restart.begin("SYNTHETIC", false, false);
		check(titleCreates == 1 && captureCreates == 1 && keysPresent,
		      "restart uses durable ids and intent keys");
		auto end = restart.step(false);
		check(aborts == 1 && end.state == "aborted", "interrupted encoder uses abort and never seal");
		QFile journal(sync.directory() + "/remote-journal.json");
		check(journal.open(QIODevice::ReadOnly), "remote state is durable");
		auto bytes = journal.readAll();
		journal.close();
		check(!bytes.contains("synthetic-token") && !bytes.contains("https://") &&
			      !bytes.contains("refresh_token"),
		      "remote journal excludes credentials and URLs");
		declarationsUnavailable = true;
		auto dir = store.mediaDirectory(account, id);
		QDir().mkpath(dir + "/720p");
		QFile one(dir + "/720p/init.mp4");
		one.open(QIODevice::WriteOnly);
		one.write("a");
		one.close();
		j.objects.append(
			{"720p/init.mp4", 1,
			 QString::fromLatin1(QCryptographicHash::hash("a", QCryptographicHash::Sha256).toHex()),
			 false});
		store.save(j);
		try {
			restart.step(true);
		} catch (const hhc::RequestError &) {
		}
		QDir().mkpath(dir + "/480p");
		QFile two(dir + "/480p/init.mp4");
		two.open(QIODevice::WriteOnly);
		two.write("b");
		two.close();
		j.stopIntent = true;
		j.objects.append(
			{"480p/init.mp4", 1,
			 QString::fromLatin1(QCryptographicHash::hash("b", QCryptographicHash::Sha256).toHex()),
			 false});
		store.save(j);
		hhc::CaptureSync::persistControl(root.path(), account, id, false);
		QFile controlFile(dir + "/control-intents.json");
		check(controlFile.open(QIODevice::ReadOnly) && controlFile.readAll().contains("cancel_auto_publish"),
		      "operator cancellation is durable before offline reconciliation");
		controlFile.close();
		hhc::CaptureSync retry(root.path(), account, id, api);
		try {
			retry.step(true);
		} catch (const hhc::RequestError &) {
		}
		check(declarations.size() == 2 && declarations[0] == declarations[1] &&
			      declarations[1]["objects"].toArray().size() == 1,
		      "uncertain declare replays original body before new closed bytes");
		if (stops != 1)
			std::cerr << "observed stops=" << stops << "\n";
		check(stops == 1, "durable stop is synchronized before an unavailable earlier declaration");
		QLockFile ownership(dir + "/sync.lock");
		ownership.setStaleLockTime(0);
		check(ownership.tryLock(0), "test obtains session ownership");
		bool excluded = false;
		try {
			retry.begin("SYNTHETIC", false, false);
		} catch (const hhc::RequestError &e) {
			excluded = e.code == "local_session_busy";
		}
		check(excluded, "shared session lock excludes foreground/helper mutation");
		ownership.unlock();
		const auto longRoot = root.path() + "/" + QString(180, 'x');
		hhc::SessionStore longStore(longRoot);
		hhc::CaptureJournal longJournal;
		longJournal.account = account;
		longJournal.localId = id;
		longStore.save(longJournal);
		hhc::CaptureSync longSync(longRoot, account, id, api);
		longSync.begin("SYNTHETIC", false, false);
		check(!QFile::exists(longStore.mediaDirectory(account, id) + "/sync.lock"),
		      "long Windows session lock is removed after a mutation");
		longSync.begin("SYNTHETIC", false, false);
		return errors ? 1 : 0;
	} catch (const std::exception &e) {
		std::cerr << "TEST EXCEPTION " << e.what() << "\n";
		return 9;
	}
}
