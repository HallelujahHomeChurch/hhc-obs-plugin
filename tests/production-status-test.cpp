#include "native-auth.hpp"
#include "http.hpp"
#include "session-store.hpp"
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	if (argc != 4)
		return 2;
	try {
		QCoreApplication::addLibraryPath(QDir(".deps/qt/plugins").absolutePath());
		hhc::NativeAuth auth(QString::fromLocal8Bit(argv[1]));
		hhc::ApiClient api(QUrl("https://admin.alive.org.tw/api"), [&](bool f) { return auth.bearer(f); });
		if (QString::fromLocal8Bit(argv[2]) == "--replay-title") {
			auto dir = hhc::SessionStore(QString::fromLocal8Bit(argv[1]) + "/queue")
					   .mediaDirectory(auth.account().isEmpty() ? (auth.bearer(), auth.account())
										    : auth.account(),
							   QString::fromLocal8Bit(argv[3]));
			QFile f(dir + "/remote-journal.json");
			if (!f.open(QIODevice::ReadOnly))
				return 3;
			auto j = QJsonDocument::fromJson(f.readAll()).object();
			auto response = hhc::httpRequest(
				"POST", QUrl("https://admin.alive.org.tw/api/admin/recordings"),
				QJsonDocument(QJsonObject{{"title", j["title"]}}).toJson(QJsonDocument::Compact),
				{{"Authorization", "Bearer " + auth.bearer()},
				 {"Content-Type", "application/json"},
				 {"Idempotency-Key", (j["localId"].toString() + ".title").toUtf8()}});
			auto code =
				QJsonDocument::fromJson(response.body).object()["error"].toObject()["code"].toString();
			QJsonObject safe{{"httpStatus", response.status},
					 {"code", QRegularExpression("^[a-z0-9_:-]{0,128}$").match(code).hasMatch()
							  ? code
							  : QString("redacted")}};
			std::cout << QJsonDocument(safe).toJson().toStdString();
			return 0;
		}
		auto c = api.request("GET",
				     QString("/admin/recordings/%1/captures/%2?limit=1000").arg(argv[2], argv[3]), {},
				     "RecordingCaptureEnvelope")["data"]
				 .toObject();
		QJsonObject safe;
		for (const auto *key : {"captureId", "recordingId", "state", "liveState", "autoPublish",
					"stopAcceptedAt", "packageId", "serverNow", "declaredObjects", "declaredBytes"})
			safe[key] = c[key];
		auto reason = c["terminalReason"].toString();
		safe["terminalReason"] = reason.isEmpty() ? QJsonValue::Null
					 : QRegularExpression("^[a-zA-Z0-9_ .:-]{1,128}$").match(reason).hasMatch()
						 ? QJsonValue(reason)
						 : QJsonValue("redacted_non_code_reason");
		QJsonObject counts;
		for (const auto &v : c["objects"].toArray()) {
			auto s = v.toObject()["state"].toString();
			counts[s] = counts[s].toInt() + 1;
		}
		safe["objectStates"] = counts;
		std::cout << QJsonDocument(safe).toJson().toStdString();
		return c["state"] == "failed" ? 4 : 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 9;
	}
}
