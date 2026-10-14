#include "http.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <iostream>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const bool production = app.arguments().contains("--production");
	QTemporaryDir temp;
	if (!temp.isValid()) return 2;
	const auto marker = temp.filePath("offline");
	qputenv("HHC_TEST_NETWORK_OFFLINE_FILE", marker.toUtf8());
	auto offline = [&] { QFile f(marker); return f.open(QIODevice::WriteOnly); };
	QTcpServer server;
	if (!server.listen(QHostAddress::LocalHost, 0)) return 2;
	int calls = 0, failed = 0;
	bool stall = false;
	QObject::connect(&server, &QTcpServer::newConnection, [&] {
		auto *socket = server.nextPendingConnection();
		QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
			socket->readAll();
			++calls;
			auto respond = [socket] {
				socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK");
				socket->disconnectFromHost();
			};
			if (stall) {
				QTimer::singleShot(50, &app, offline);
				QTimer::singleShot(2000, socket, respond);
			} else respond();
		});
	});
	const QUrl url(QString("http://127.0.0.1:%1/test").arg(server.serverPort()));
	auto request = [&](const QByteArray &method) {
		try { return hhc::httpRequest(method, url, {}, {}).status == 200; }
		catch (const hhc::RequestError &e) {
			if (e.code != "transport_unavailable") ++failed;
			return false;
		}
	};
	auto check = [&](bool ok, const char *what) {
		if (!ok) { std::cerr << "FAIL " << what << '\n'; ++failed; }
	};
	check(offline(), "create marker");
	check(request("GET") == production, "only test build blocks offline request");
	check(calls == (production ? 1 : 0), "offline test request never reaches server");
	QFile::remove(marker);
	check(request("POST"), "removing marker restores real HTTP");
	if (!production) {
		stall = true;
		QElapsedTimer elapsed; elapsed.start();
		check(!request("PUT"), "new outage aborts an in-flight upload");
		check(elapsed.elapsed() < 1000, "in-flight abort is prompt");
		QFile::remove(marker);
		stall = false;
		check(request("POST"), "HTTP resumes after in-flight outage");
	}
	qunsetenv("HHC_TEST_NETWORK_OFFLINE_FILE");
	return failed ? 1 : 0;
}
