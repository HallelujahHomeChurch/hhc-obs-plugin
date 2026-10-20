#include "http.hpp"
#include "wire.hpp"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QSet>
#include <QThread>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QElapsedTimer>
#include <QRegularExpression>
#ifdef HHC_NETWORK_TEST_FAULTS
#include <QFileInfo>
#endif
namespace hhc {
namespace {
QString operation(const QUrl &url, const QByteArray &method)
{
	const auto path = url.path();
	if (method == "PUT") return "media.put";
	if (path == "/api/ingest/streams/open") return "output.open";
	if (path == "/api/ingest/broadcast-source/session") return "session.renew";
	if (path.endsWith("/control")) return "control.poll";
	if (path.endsWith("/ack")) return "control.ack";
	if (path.endsWith("/captures")) return "capture.create";
	if (path == "/api/admin/recordings" && method == "POST") return "recording.create";
	if (path.contains("/captures/")) {
		const auto suffix = path.section('/', -1);
		if (QStringList{"objects", "sign", "confirm", "stop", "seal", "abort"}.contains(suffix))
			return "capture." + suffix;
		if (method == "GET") return "capture.poll";
	}
	return "other";
}
void logRequest(const QByteArray &method, const QUrl &url, int status, const QString &code,
		const QString &requestId, int attempt, const QElapsedTimer &elapsed, const char *outcome)
{
	static const QRegularExpression correlation("^(?:[a-fA-F0-9]{16,64}|[a-fA-F0-9]{8}-(?:[a-fA-F0-9]{4}-){3}[a-fA-F0-9]{12})$");
	const auto match = correlation.match(requestId);
	const auto safeId = match.hasMatch() && match.capturedLength() == requestId.size() ? requestId : QString("none");
	qInfo().noquote() << QString("[HHC HTTP] utc=%1 monoMs=%2 op=%3 status=%4 code=%5 requestId=%6 attempt=%7 elapsedMs=%8 outcome=%9")
		.arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)).arg(elapsed.msecsSinceReference())
		.arg(operation(url, method)).arg(status).arg(code, safeId).arg(attempt).arg(elapsed.elapsed()).arg(outcome);
}
QString responseCode(const QByteArray &body)
{
	static const QSet<QString> known{"broadcast_source_not_prepared",
			 "broadcast_source_busy",
			 "broadcast_output_capacity_exhausted",
			 "broadcast_output_terminal",
			 "broadcast_stream_key_invalid",
			 "broadcast_ingest_session_invalid",
			 "broadcast_ingest_revoked",
			 "capture_invalid",
			 "capture_unauthorized",
			 "capture_forbidden",
			 "capture_not_found",
			 "capture_conflict",
			 "capture_missing_objects",
			 "capture_expired",
			 "capture_too_large",
			 "capture_rate_limited",
			 "capture_unavailable",
			 "recording_conflict",
			 "invalid_recording",
			 "recordings_unavailable",
			 "unauthorized",
			 "forbidden",
			 "broadcast_invalid",
			 "broadcast_not_found",
			 "broadcast_state_conflict",
			 "broadcast_already_bound",
			 "broadcast_active_limit",
			 "broadcast_epoch_mismatch",
			 "broadcast_boundary_conflict",
			 "broadcast_operation_conflict",
			 "broadcast_revision_conflict",
			 "broadcast_protocol_unsupported",
			 "rate_limited",
			 "unavailable"};
	const auto code = QJsonDocument::fromJson(body).object()["error"].toObject()["code"].toString();
	return known.contains(code) ? code : QString("request_rejected");
}
thread_local const std::atomic<bool> *cancellation = nullptr;
void checkCancellation()
{
	if (cancellation && cancellation->load())
		throw RequestError(0, "transport_cancelled");
}
} // namespace
std::optional<qint64> RequestError::retryDelayMs(unsigned &failures) const
{
	if (!outputOpening) {
		auto delay = retryDelay(failures);
		return delay ? std::optional<qint64>(qint64(*delay) * 1000) : std::nullopt;
	}
	const bool wait = status == 409 && QStringList{"broadcast_source_not_prepared", "broadcast_source_busy",
						       "broadcast_output_capacity_exhausted", "broadcast_active_limit"}
						   .contains(code);
	if (!(wait || status == 429 || status >= 500 || (status == 0 && code == "transport_unavailable")))
		return std::nullopt;
	const int base = int(std::min(30u, 1u << std::min(failures, 5u))) * 1000;
	failures = std::min(failures, 5u) + 1;
	return std::max(qint64(base * 4 / 5 + QRandomGenerator::global()->bounded(base * 2 / 5 + 1)),
			qint64(retryAfter) * 1000);
}
HttpCancellationScope::HttpCancellationScope(const std::atomic<bool> &flag) : previous_(cancellation)
{
	cancellation = &flag;
}
HttpCancellationScope::~HttpCancellationScope()
{
	cancellation = previous_;
}
void cancellableWait(int milliseconds)
{
	while (milliseconds > 0) {
		checkCancellation();
		auto chunk = std::min(milliseconds, 100);
		QThread::msleep(chunk);
		milliseconds -= chunk;
	}
	checkCancellation();
}

HttpResponse httpRequest(const QByteArray &method, const QUrl &url, const QByteArray &body,
			 const QMap<QByteArray, QByteArray> &headers)
{
	checkCancellation();
	if (!url.isValid() || !url.userInfo().isEmpty() || url.hasFragment() ||
	    (url.scheme() != "https" && !(url.scheme() == "http" && url.host() == "127.0.0.1")))
		throw RequestError(0, "invalid_transport_origin");
#ifdef HHC_NETWORK_TEST_FAULTS
	const auto offlinePath = qEnvironmentVariable("HHC_TEST_NETWORK_OFFLINE_FILE");
	auto offline = [&] {
		return !offlinePath.isEmpty() && QFileInfo::exists(offlinePath);
	};
	if (offline())
		throw RequestError(0, "transport_unavailable");
	bool networkCut = false;
#endif
	QNetworkAccessManager manager;
	QElapsedTimer elapsed;
	elapsed.start();
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
	request.setTransferTimeout(30000);
	for (auto i = headers.begin(); i != headers.end(); ++i)
		request.setRawHeader(i.key(), i.value());
	auto *reply = manager.sendCustomRequest(request, method, body);
	QEventLoop loop;
	QTimer deadline;
	deadline.setSingleShot(true);
	deadline.setInterval(35000);
	HttpResponse response;
	bool exceeded = false;
	QTimer cancelled;
	cancelled.setInterval(100);
	QObject::connect(&cancelled, &QTimer::timeout, reply, [&] {
#ifdef HHC_NETWORK_TEST_FAULTS
		if (offline()) {
			networkCut = true;
			reply->abort();
		}
#endif
		if (cancellation && cancellation->load())
			reply->abort();
	});
	cancelled.start();
	QObject::connect(reply, &QNetworkReply::readyRead, [&] {
		response.body += reply->readAll();
		if (response.body.size() > 16 * 1024 * 1024) {
			exceeded = true;
			reply->abort();
		}
	});
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	QObject::connect(&deadline, &QTimer::timeout, reply, &QNetworkReply::abort);
	deadline.start();
	loop.exec();
	const auto outcome = cancellation && cancellation->load() ? "cancelled" :
		(!deadline.isActive() || reply->error() == QNetworkReply::TimeoutError) ? "timeout" :
		reply->error() != QNetworkReply::NoError ? "network_error" : "response";
	if (method == "PUT" || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 0)
		logRequest(method, url, reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
			"transport", {}, 1, elapsed, outcome);
	checkCancellation();
#ifdef HHC_NETWORK_TEST_FAULTS
	if (networkCut)
		throw RequestError(0, "transport_unavailable");
#endif
	response.body += reply->readAll();
	response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	response.requestId = QString::fromLatin1(reply->rawHeader("X-Request-ID").left(128));
	if (response.requestId.isEmpty())
		response.requestId = QString::fromLatin1(reply->rawHeader("X-Correlation-ID").left(128));
	auto retry = reply->rawHeader("Retry-After");
	bool numeric = false;
	auto seconds = retry.toInt(&numeric);
	if (!numeric) {
		auto when = QDateTime::fromString(QString::fromLatin1(retry), Qt::RFC2822Date);
		seconds = when.isValid() ? int(QDateTime::currentDateTimeUtc().secsTo(when)) : 0;
	}
	response.retryAfter = std::max(0, seconds);
	if (exceeded)
		throw RequestError(413, "response_limit");
	if (response.status == 0)
		throw RequestError(0, "transport_unavailable");
	return response;
}
QJsonObject ApiClient::request(const QByteArray &method, const QString &path, const QJsonObject &body,
			       const QString &schema, const QByteArray &key)
{
	if (!path.startsWith('/') || path.startsWith("//") || path.contains('#'))
		throw RequestError(0, "invalid_api_path");
	QUrl url(origin_.toString() + (route_ ? route_(method, path) : path));
	HttpResponse result;
	for (int attempt = 0; attempt < 2; ++attempt) {
		QElapsedTimer elapsed;
		elapsed.start();
		try {
			QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"},
							     {"Accept", "application/json"}};
			const auto bearer = token_(attempt == 1);
			if (!bearer.isEmpty())
				headers["Authorization"] = "Bearer " + bearer;
			if (!key.isEmpty())
				headers["Idempotency-Key"] = key;
			result = transport_(method, url,
					    method == "GET" ? QByteArray{} : QJsonDocument(body).toJson(QJsonDocument::Compact),
					    headers);
			logRequest(method, url, result.status, result.status >= 200 && result.status < 300 ? "none" : responseCode(result.body),
				result.requestId, attempt + 1, elapsed, "response");
			if (result.status != 401 || attempt == 1 || bearer.isEmpty())
				break;
		} catch (const RequestError &e) {
			const auto code = QStringList{"transport_unavailable", "transport_cancelled", "local_session_busy",
				"local_credentials_busy", "ingest_local_conflict", "malformed_success"}.contains(e.code) ? e.code : QString("request_failed");
			logRequest(method, url, e.status, code, {}, attempt + 1, elapsed,
				e.code == "transport_cancelled" ? "cancelled" : "exception");
			throw;
		} catch (...) {
			logRequest(method, url, 0, "local_error", {}, attempt + 1, elapsed, "exception");
			throw;
		}
	}
	if (result.status < 200 || result.status >= 300) {
		const auto code = responseCode(result.body);
		throw RequestError(result.status, code, result.retryAfter);
	}
	QJsonParseError error;
	auto document = QJsonDocument::fromJson(result.body, &error);
	if (error.error != QJsonParseError::NoError || !document.isObject() ||
	    (!schema.isEmpty() && !validWire(schema, document.object())) || !document.object().contains("data") ||
	    (document.object().contains("error") && !document.object().value("error").isNull()))
		throw RequestError(result.status, "malformed_success");
	return document.object();
}
} // namespace hhc
