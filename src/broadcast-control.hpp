#pragma once
#include "http.hpp"
#include <QJsonArray>
#include <optional>
namespace hhc {
inline constexpr auto b1Version = "b1-2026-10-10.rc1";
QJsonObject acceptD2Observation(const QJsonObject &previous, const QJsonObject &incoming,
			      const QJsonObject &identity, quint64 requestGeneration, quint64 currentGeneration);
bool d2FallbackAllowed(const RequestError &error);
class BroadcastControl {
public:
	BroadcastControl(QString directory, QString account, QString localId, ApiClient &api, bool ingest = false);
	void adoptIngest(const QJsonObject &result, const QJsonObject &intent);
	std::optional<int> acceptedEnd();
	std::optional<int> serverEnd();
	static QJsonArray selectable(ApiClient &);
	QJsonObject bind(const QString &recordingId);
	QJsonObject poll(std::function<std::optional<int>()> nextBoundary = {}, bool d1 = false);
	void replay();

private:
	void load();
	void save();
	void checkBinding(const QJsonObject &);
	QJsonObject acceptControl(const QJsonObject &);
	QJsonObject view();
	QJsonObject ack(const QString &, QJsonObject);
	QJsonObject journal_;
	QString directory_, account_, id_;
	ApiClient &api_;
	bool ingest_ = false;
};
} // namespace hhc
