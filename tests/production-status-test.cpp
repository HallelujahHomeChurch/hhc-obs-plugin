#include "native-auth.hpp"
#include "http.hpp"
#include "session-store.hpp"
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSaveFile>
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
		if (QString::fromLocal8Bit(argv[2]) == "--recording-meta") {
			const auto path = "/admin/recordings/" + QString::fromLocal8Bit(argv[3]);
			auto recording = api.request("GET", path)["data"].toObject();
			QJsonObject safe;
			for (const auto *key :
			     {"id", "title", "status", "version", "uploadedAt", "readyAt", "selectedCoverId"})
				safe[key] = recording[key];
			std::cout << QJsonDocument(safe).toJson().toStdString();
			return 0;
		}
		if (QString::fromLocal8Bit(argv[2]) == "--approved-delete-test" ||
		    QString::fromLocal8Bit(argv[2]) == "--cleanup-check-test") {
			const QString id = QString::fromLocal8Bit(argv[3]);
			const QMap<QString, QString> authorized{
				{"4b410b08-892f-46a0-bfcf-6ff68844f756", "f0d763fbce234ea290e774d9c008811d"},
				{"2a002691-2cd7-4dca-b8be-0b8b7e330375", "e5e5d5b869f638e10abbab83b2c02def"},
				{"28a3c057-4885-4036-ac1c-0437c1495206", "5d68171bc2e63cd5f9cae36eef7032b7"},
				{"def050ae-22a5-40c6-98a7-ea4d87d06dea", "fca9d0aa7491f6e6615b93ccc922f350"}};
			if (!authorized.contains(id))
				throw std::runtime_error("Recording outside explicit cleanup authorization");
			auth.bearer();
			if (auth.account() != "019fd685-994e-798a-b5bc-62c535337fee")
				throw std::runtime_error("Cleanup account differs from test authorizer");
			const auto path = "/admin/recordings/" + id;
			auto r = api.request("GET", path)["data"].toObject();
			auto c = api.request("GET", path + "/captures/" + authorized[id] + "?limit=1000", {},
					     "RecordingCaptureEnvelope")["data"]
					 .toObject();
			const auto eligible = [&](QJsonObject record, QJsonObject capture) {
				const bool rejectedSeal =
					id == "28a3c057-4885-4036-ac1c-0437c1495206" &&
					capture["state"] == "uploading" && capture["liveEnabled"].isBool() &&
					!capture["liveEnabled"].toBool() && capture["stopAcceptedAt"].isString();
				return record["id"] == id && record["status"] == "draft" &&
				       record["title"].toString().startsWith("[HHC OBS SYNTHETIC TEST] ") &&
				       record.contains("uploadedAt") && record["uploadedAt"].isNull() &&
				       record["version"].isDouble() && record["version"].toInteger() > 0 &&
				       record["version"].toDouble() == record["version"].toInteger() &&
				       capture["recordingId"] == id && capture["captureId"] == authorized[id] &&
				       (capture["state"] == "failed" || capture["state"] == "aborted" || rejectedSeal);
			};
			if (!eligible(r, c))
				throw std::runtime_error("Cleanup preconditions changed; preserved recording");
			// Runnable guard checks before the consequential request; no package/operator CLI.
			for (const auto *field : {"id", "status", "uploadedAt", "version"}) {
				auto wrong = r;
				wrong.remove(field);
				if (eligible(wrong, c))
					throw std::runtime_error("Cleanup guard self-check failed");
			}
			auto published = r;
			published["status"] = "published";
			if (eligible(published, c))
				throw std::runtime_error("Published cleanup guard failed");
			if (c["state"] == "uploading") {
				for (const auto *field : {"liveEnabled", "stopAcceptedAt"}) {
					auto wrong = c;
					wrong.remove(field);
					if (eligible(r, wrong))
						throw std::runtime_error("Stopped cleanup guard failed");
				}
			}
			const auto version = QByteArray::number(r["version"].toInteger());
			QJsonObject receipt{{"recordingId", id},          {"captureId", authorized[id]},
					    {"title", r["title"]},        {"version", r["version"]},
					    {"captureState", c["state"]}, {"deleteAccepted", false}};
			if (QString::fromLocal8Bit(argv[2]) == "--cleanup-check-test") {
				std::cout << QJsonDocument(receipt).toJson().toStdString();
				return 0;
			}
			const auto save = [&] {
				QSaveFile file("artifacts/approved-cleanup-" + id + ".json");
				auto bytes = QJsonDocument(receipt).toJson();
				if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() ||
				    !file.commit())
					throw std::runtime_error("Cannot persist cleanup receipt");
			};
			save();
			auto response = hhc::httpRequest("DELETE", QUrl("https://admin.alive.org.tw/api" + path), {},
							 {{"Authorization", "Bearer " + auth.bearer()},
							  {"If-Match", "\"" + version + "\""}});
			receipt["httpStatus"] = response.status;
			receipt["deleteAccepted"] = response.status == 204;
			save();
			if (response.status != 204)
				throw hhc::RequestError(response.status, "cleanup_not_accepted");
			auto absent = hhc::httpRequest("GET", QUrl("https://admin.alive.org.tw/api" + path), {},
						       {{"Authorization", "Bearer " + auth.bearer()}});
			receipt["verifyHttpStatus"] = absent.status;
			save();
			std::cout << QJsonDocument(receipt).toJson().toStdString();
			return absent.status == 404 ? 0 : 5;
		}
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
