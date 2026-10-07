#include "http.hpp"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <iostream>
#include <QJsonDocument>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTcpServer server;
	server.listen(QHostAddress::LocalHost, 0);
	int calls = 0, refreshes = 0, mode = 0;
	QByteArray seen;
	QObject::connect(&server, &QTcpServer::newConnection, [&] {
		auto *s = server.nextPendingConnection();
		auto bytes = std::make_shared<QByteArray>();
		QObject::connect(s, &QTcpSocket::readyRead, [&, s, bytes] {
			*bytes += s->readAll();
			if (!bytes->contains("\r\n\r\n"))
				return;
			seen = *bytes;
			++calls;
			int status = mode == 1 ? 403 : mode == 2 ? 401 : mode == 3 ? 413 : calls == 1 ? 401 : 200;
			QByteArray body = status == 200 ? "{\"data\":{},\"meta\":{},\"error\":null}"
							: "gateway error with signed URL and secret";
			if (mode == 5) {
				status = 200;
				body = QJsonDocument(
					       QJsonObject{{"data",
							    QJsonObject{{"id", "018f0c1f-18d0-7e81-9f6f-69c456db7003"},
									{"title", "TEST"},
									{"uploadedAt", QJsonValue::Null},
									{"expiresAt", QJsonValue::Null},
									{"featured", false},
									{"hidden", false},
									{"status", "draft"},
									{"version", 1}}},
							   {"meta", QJsonObject{}}})
					       .toJson(QJsonDocument::Compact);
			}
			s->write("HTTP/1.1 " + QByteArray::number(status) + " Result\r\nContent-Length: " +
				 QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
			s->disconnectFromHost();
		});
		QObject::connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
	});
	int failed = 0;
	auto check = [&](bool ok, const char *n) {
		if (!ok) {
			std::cerr << "FAIL " << n << '\n';
			++failed;
		}
	};
	hhc::ApiClient api(QUrl(QString("http://127.0.0.1:%1/api").arg(server.serverPort())), [&](bool refresh) {
		refreshes += refresh;
		return QByteArray("synthetic-token");
	});
	auto result = api.request("POST", "/admin/recordings", {{"title", "TEST"}}, {}, "fixed-key");
	check(calls == 2 && refreshes == 1 && result.contains("data"), "401 refresh once then retry original");
	check(seen.contains("Idempotency-Key: fixed-key"), "durable title key sent");
	for (mode = 1; mode <= 3; ++mode) {
		calls = 0;
		refreshes = 0;
		bool error = false;
		try {
			api.request("GET", "/test");
		} catch (const hhc::RequestError &e) {
			error = e.status == (mode == 1   ? 403
					     : mode == 2 ? 401
							 : 413) &&
				QString::fromUtf8(e.what()).size() < 100 &&
				!QString::fromUtf8(e.what()).contains("secret");
		}
		check(error, "HTTP status before JSON and no raw error logging");
		check(calls == (mode == 2 ? 2 : 1) && refreshes == (mode == 2 ? 1 : 0), "no permission refresh loop");
	}
	mode = 5;
	bool optionalErrorAccepted = false;
	try {
		optionalErrorAccepted =
			api.request("POST", "/admin/recordings", {{"title", "TEST"}}, "RecordingEnvelope", "same-key")
				.contains("data");
	} catch (...) {
	}
	check(optionalErrorAccepted, "existing title envelope permits omitted optional error field");
	return failed ? 1 : 0;
}
