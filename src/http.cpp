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
namespace hhc {
namespace {
thread_local const std::atomic<bool> *cancellation = nullptr;
void checkCancellation()
{
	if (cancellation && cancellation->load())
		throw RequestError(0, "transport_cancelled");
}
} // namespace
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
	QNetworkAccessManager manager;
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
	checkCancellation();
	response.body += reply->readAll();
	response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	auto retry = reply->rawHeader("Retry-After");
	bool numeric = false;
	auto seconds = retry.toInt(&numeric);
	if (!numeric) {
		auto when = QDateTime::fromString(QString::fromLatin1(retry), Qt::RFC2822Date);
		seconds = when.isValid() ? int(QDateTime::currentDateTimeUtc().secsTo(when)) : 0;
	}
	response.retryAfter = qBound(0, seconds, 300);
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
	QUrl url(origin_.toString() + path);
	HttpResponse result;
	for (int attempt = 0; attempt < 2; ++attempt) {
		QMap<QByteArray, QByteArray> headers{{"Authorization", "Bearer " + token_(attempt == 1)},
						     {"Content-Type", "application/json"},
						     {"Accept", "application/json"}};
		if (!key.isEmpty())
			headers["Idempotency-Key"] = key;
		result = httpRequest(
			method, url,
			method == "GET" ? QByteArray{} : QJsonDocument(body).toJson(QJsonDocument::Compact), headers);
		if (result.status != 401 || attempt == 1)
			break;
	}
	if (result.status < 200 || result.status >= 300) {
		const auto code = QJsonDocument::fromJson(result.body).object()["error"].toObject()["code"].toString();
		static const QSet<QString> known{
			"capture_invalid",        "capture_unauthorized", "capture_forbidden",
			"capture_not_found",      "capture_conflict",     "capture_missing_objects",
			"capture_expired",        "capture_too_large",    "capture_rate_limited",
			"capture_unavailable",    "recording_conflict",   "invalid_recording",
			"recordings_unavailable", "unauthorized",         "forbidden"};
		throw RequestError(result.status, known.contains(code) ? code : QString("request_rejected"),
				   result.retryAfter);
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
