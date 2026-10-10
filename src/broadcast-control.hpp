#pragma once
#include "http.hpp"
#include <QJsonArray>
#include <optional>
namespace hhc
{
inline constexpr auto b1Version = "b1-2026-10-10.rc1";
class BroadcastControl
{
      public:
	BroadcastControl(QString directory, QString account, QString localId, ApiClient &api);
	static QJsonArray selectable(ApiClient &);
	QJsonObject bind(const QString &recordingId);
	QJsonObject poll(std::function<std::optional<int>()> nextBoundary = {});
	void replay();

      private:
	void load();
	void save();
	void checkBinding(const QJsonObject &);
	QJsonObject view();
	QJsonObject ack(const QString &, QJsonObject);
	QJsonObject journal_;
	QString directory_, account_, id_;
	ApiClient &api_;
};
} // namespace hhc
