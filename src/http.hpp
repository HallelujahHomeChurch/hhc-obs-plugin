#pragma once
#include <QByteArray>
#include <QUrl>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <optional>
namespace hhc {
class HttpCancellationScope {
public:
	explicit HttpCancellationScope(const std::atomic<bool> &);
	~HttpCancellationScope();

private:
	const std::atomic<bool> *previous_;
};
void cancellableWait(int milliseconds);
struct HttpResponse {
	int status = 0;
	QByteArray body;
	int retryAfter = 0;
	QString requestId;
};
class RequestError : public std::runtime_error {
public:
	RequestError(int s, QString c, int retry = 0)
		: std::runtime_error((QString("HTTP %1: ").arg(s) + c).toStdString()),
		  status(s),
		  code(std::move(c)),
		  retryAfter(retry)
	{
	}
	int status;
	QString code;
	int retryAfter;
	bool outputOpening = false;
	std::optional<qint64> retryDelayMs(unsigned &failures) const;
	std::optional<int> retryDelay(unsigned &failures) const
	{
		if (!(status == 0 && code == "transport_unavailable" || status == 429 || status >= 500 ||
		      status == 409 && (code == "local_session_busy" || code == "local_credentials_busy" ||
					code == "capture_missing_objects" || code == "oauth_temporarily_unavailable")))
			return std::nullopt;
		failures = std::min(failures, 5u) + 1;
		return std::max(retryAfter, int(std::min(60u, 1u << failures)));
	}
};
HttpResponse httpRequest(const QByteArray &method, const QUrl &, const QByteArray &body = {},
			 const QMap<QByteArray, QByteArray> &headers = {});
class ApiClient {
public:
	using Transport = std::function<HttpResponse(const QByteArray &, const QUrl &, const QByteArray &,
						     const QMap<QByteArray, QByteArray> &)>;
	using Route = std::function<QString(const QByteArray &, const QString &)>;
	ApiClient(QUrl origin, std::function<QByteArray(bool)> token, Transport transport = httpRequest,
		  Route route = {})
		: origin_(std::move(origin)),
		  token_(std::move(token)),
		  transport_(std::move(transport)),
		  route_(std::move(route))
	{
	}
	QJsonObject request(const QByteArray &method, const QString &path, const QJsonObject &body = {},
			    const QString &schema = {}, const QByteArray &key = {});

private:
	QUrl origin_;
	std::function<QByteArray(bool)> token_;
	Transport transport_;
	Route route_;
};
} // namespace hhc
