#include "http.hpp"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <iostream>
#include <QJsonDocument>
#include <QStringList>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "helper-diagnostics.hpp"
namespace { QStringList diagnosticMessages; }
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const auto oldHandler = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
		diagnosticMessages.append(message);
	});
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
				 QByteArray::number(body.size()) + "\r\nX-Request-ID: 0123456789abcdef0123456789abcdef\r\nConnection: close\r\n\r\n" + body);
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
	for (const auto &error :
	     {hhc::RequestError(0, "transport_unavailable"), hhc::RequestError(429, "capture_rate_limited"),
	      hhc::RequestError(503, "capture_unavailable"), hhc::RequestError(409, "local_credentials_busy"),
	      hhc::RequestError(409, "local_session_busy"), hhc::RequestError(409, "capture_missing_objects"),
	      hhc::RequestError(409, "oauth_temporarily_unavailable")}) {
		unsigned failures = 0;
		for (int attempt = 0; attempt < 20; ++attempt) {
			auto delay = error.retryDelay(failures);
			check(delay && *delay >= 2 && *delay <= 60 && failures <= 6,
			      "transient outage keeps retrying after six failures with capped backoff");
		}
	}
	for (const auto &error :
	     {hhc::RequestError(401, "sign_in_required"), hhc::RequestError(403, "forbidden"),
	      hhc::RequestError(410, "capture_expired"), hhc::RequestError(409, "capture_conflict"),
	      hhc::RequestError(409, "oauth_unavailable"), hhc::RequestError(0, "transport_cancelled"),
	      hhc::RequestError(0, "invalid_transport_origin")}) {
		unsigned failures = 0;
		check(!error.retryDelay(failures) && failures == 0, "terminal/cancelled requests pause safely");
	}
	unsigned failures = 6;
	check(hhc::RequestError(429, "capture_rate_limited", 300).retryDelay(failures) == 300,
	      "Retry-After survives capped exponential backoff");
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
	const auto put = hhc::httpRequest("PUT", QUrl(QString("http://127.0.0.1:%1/private-object?private-signature=yes").arg(server.serverPort())),
		"private-media", {{"Authorization", "private-put-header"}});
	check(put.status == 200, "loopback PUT diagnostics preserve transfer result");
	hhc::ApiClient privateRequest(QUrl("https://private-origin.invalid/api"), [](bool) { return QByteArray("private-bearer"); },
		[](const auto &, const auto &, const auto &, const auto &) {
			return hhc::HttpResponse{403, "{\"error\":{\"code\":\"private-error-body\"}}", 0, "0123456789abcdef\n"};
		});
	try { privateRequest.request("POST", "/ingest/streams/open?private-query=yes", {{"streamKey", "private-stream-key"}}, {}, "private-operation-key"); }
	catch (const hhc::RequestError &) {}
	hhc::ApiClient cancelled(QUrl("https://private-origin.invalid/api"), [](bool) { return QByteArray{}; },
		[](const auto &, const auto &, const auto &, const auto &) -> hhc::HttpResponse {
			throw hhc::RequestError(0, "transport_cancelled");
		});
	try { cancelled.request("GET", "/private-path"); } catch (const hhc::RequestError &) {}
	const auto logs = diagnosticMessages.join('\n');
	check(logs.contains("op=recording.create") && logs.contains("status=401") && logs.contains("attempt=2") &&
	      logs.contains("requestId=0123456789abcdef0123456789abcdef") && logs.contains("elapsedMs=") &&
	      logs.contains("monoMs=") && logs.contains("utc="), "safe per-attempt request diagnostics");
	check(!logs.contains("synthetic-token") && !logs.contains("fixed-key") && !logs.contains("gateway error") &&
	      !logs.contains("127.0.0.1") && !logs.contains("signed URL"), "diagnostics exclude credentials, origins and raw bodies");
	check(!logs.contains("private-") && logs.contains("op=media.put") && logs.contains("requestId=none") && logs.contains("code=request_rejected") &&
	      logs.contains("code=transport_cancelled") && logs.contains("outcome=cancelled"),
	      "redact query/body/bearer/idempotency key, reject injected correlation and classify cancellation");
	QTemporaryDir helperLogs;
	const auto directory = helperLogs.path() + "/diagnostics";
	check(hhc::appendHelperDiagnostic(directory, "[HHC helper] result=ready"), "helper diagnostic is persisted");
	check(!hhc::appendHelperDiagnostic(directory, "untrusted Qt text") &&
	      !hhc::appendHelperDiagnostic(directory, "[HHC helper] injected\nline"), "helper persists only bounded single-line HHC diagnostics");
	for (int rotation = 0; rotation < 2; ++rotation) {
		QFile current(directory + "/helper.log");
		check(current.open(QIODevice::WriteOnly) && current.resize(8 * 1024 * 1024), "fill diagnostic rotation fixture");
		current.close();
		check(hhc::appendHelperDiagnostic(directory, "[HHC helper] result=rotated"), "helper log rotates without a new per-process file");
	}
	check(QDir(directory).entryList(QDir::Files).size() == 2 &&
	      QFileInfo(directory + "/helper.log").size() < 2048 &&
	      QFileInfo(directory + "/helper.previous.log").size() == 8 * 1024 * 1024,
	      "helper diagnostics retain two files and at most 16 MiB total across restarts");
	QLockFile logOwner(hhc::lockFilePath(directory + "/helper.lock"));
	check(logOwner.tryLock(0) && !hhc::appendHelperDiagnostic(directory, "[HHC helper] result=busy"),
	      "concurrent helper writers yield instead of racing rotation");
	qInstallMessageHandler(oldHandler);
	return failed ? 1 : 0;
}
