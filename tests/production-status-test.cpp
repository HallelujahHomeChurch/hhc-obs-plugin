#include "native-auth.hpp"
#include "http.hpp"
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
