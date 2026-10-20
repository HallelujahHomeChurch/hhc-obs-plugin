#include "ingest.hpp"
#include "wire.hpp"
#include "capture-sync.hpp"
#include "session-store.hpp"
#include "broadcast-control.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <iostream>
namespace { QStringList diagnosticMessages; }
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const auto oldHandler = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
		diagnosticMessages.append(message);
	});
	int failed = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) {
			++failed;
			std::cerr << "FAIL " << name << '\n';
		}
	};
	const QString recording = "018f0000-0000-7000-8000-000000000002", capture(32, 'a');
	QJsonObject session{{"token", "hhc_ingest_example.signature"},
			    {"expiresAt", "2026-10-14T00:15:00Z"},
			    {"recordingId", recording},
			    {"captureId", capture},
			    {"epoch", 1}};
	const auto base = "/admin/recordings/" + recording + "/captures/" + capture;
	check(hhc::ingestRoute("POST", base + "/seal", session) ==
		      "/ingest/broadcasts/" + recording + "/captures/" + capture + "/seal",
	      "C1 seal uses scoped ingest");
	check(hhc::ingestRoute("GET", base + "?limit=1000&cursor=abc", session).startsWith("/ingest/"), "paged status");
	for (const auto &path :
	     QStringList{"/admin/recordings", base + "/live", base + "/../publish", base + "?unexpected=1",
			 "/admin/broadcasts/" + recording + "/control?captureId=" + capture + "&epoch=2"}) {
		bool rejected = false;
		try {
			hhc::ingestRoute("GET", path, session);
		} catch (const hhc::RequestError &) {
			rejected = true;
		}
		check(rejected, "event session cannot cross endpoint or epoch");
	}
	QTemporaryDir temp;
	QMap<QString, QByteArray> secrets;
	hhc::IngestCredentials vault{[&](const QString &id) -> std::optional<QByteArray> {
					     if (!secrets.contains(id))
						     return {};
					     return secrets[id];
				     },
				     [&](const QString &id, const QByteArray &value) {
					     secrets[id] = value;
				     }};
	int resolves = 0, binds = 0, renewals = 0;
	QJsonObject firstIntent;
	// Use the canonical B1 fixture rather than inventing a reduced broadcast body.
	QFile fixtures("tests/fixtures/b1.json");
	check(fixtures.open(QIODevice::ReadOnly), "read B1 fixtures");
	QJsonObject broadcast;
	for (const auto &v : QJsonDocument::fromJson(fixtures.readAll()).array()) {
		auto o = v.toObject();
		if (o["name"] == "view_draft")
			broadcast = o["value"].toObject()["data"].toObject();
	}
	check(!broadcast.isEmpty(), "bound B1 fixture available");
	broadcast["recordingId"] = recording;
	auto binding = broadcast["binding"].toObject();
	binding["actorId"] = "018f0c1f-18d0-7e81-9f6f-69c456db7009";
	binding["captureId"] = capture;
	binding["epoch"] = 1;
	broadcast["binding"] = binding;
	auto transport = [&](const QByteArray &, const QUrl &url, const QByteArray &bytes,
			     const QMap<QByteArray, QByteArray> &headers) {
		check(!headers.contains("Authorization"), "bootstrap has no OAuth or bearer");
		const auto body = QJsonDocument::fromJson(bytes).object();
		QJsonObject data;
		if (url.path().endsWith("/resolve")) {
			++resolves;
			data = {{"recordingId", recording}, {"sourceRevision", 3}, {"broadcastRevision", 2}};
		} else if (url.path().endsWith("/bindings")) {
			++binds;
			if (binds == 1) {
				firstIntent = body;
				throw hhc::RequestError(0, "transport_unavailable");
			}
			check(body == firstIntent, "lost bind response reuses exact intent");
			QJsonObject receipt{{"operationKey", body["operationKey"]},
					    {"operation", "bind"},
					    {"state", "accepted"}};
			data = {{"broadcast", broadcast}, {"receipt", receipt}, {"session", session}};
		} else {
			++renewals;
			check(body["sessionToken"] == session["token"], "expired original token retained for renewal");
			data = session;
			data["expiresAt"] = QDateTime::currentDateTimeUtc().addSecs(900).toString(Qt::ISODateWithMs);
		}
		return hhc::HttpResponse{
			200,
			QJsonDocument(QJsonObject{{"data", data}, {"meta", QJsonObject{}}, {"error", QJsonValue::Null}})
				.toJson(),
			0};
	};
	hhc::KeyIngest ingest(temp.path(), "local-test", vault, transport);
	ingest.saveKey("hhc_stream_" + QByteArray(43, 'A'));
	try {
		ingest.bind();
	} catch (const hhc::RequestError &) {
	}
	hhc::KeyIngest restarted(temp.path(), "local-test", vault, transport);
	try {
		auto result = restarted.bind();
		check(result["broadcast"].toObject()["recordingId"] == recording, "restarted binding same event");
		check(resolves == 1 && binds == 2, "restart does not resolve a new target");
		check(!restarted.bearer().isEmpty() && renewals == 1, "expired session renews before use");
		auto api = restarted.api();
		hhc::BroadcastControl control(temp.path() + "/control", binding["actorId"].toString(), "local-test",
					      api, true);
		control.adoptIngest(result, restarted.metadata()["intent"].toObject());
		check(!control.acceptedEnd(), "adopting K3 does not invent an End");
		control.adoptIngest(result, restarted.metadata()["intent"].toObject());
		check(resolves == 1 && binds == 2, "B1 adoption never makes another binding request");
	} catch (const std::exception &e) {
		std::cerr << "FAIL bootstrap " << e.what() << '\n';
		++failed;
	}
	QFile journal(temp.path() + "/local-test.json");
	check(journal.open(QIODevice::ReadOnly), "read safe journal");
	auto bytes = journal.readAll();
	check(!bytes.contains("hhc_stream_") && !bytes.contains("hhc_ingest_") && !bytes.contains("sessionToken"),
	      "journal has no credentials");
	int sessionCalls = 0;
	hhc::KeyIngest scoped(
		temp.path(), "local-test", vault,
		[&](const QByteArray &, const QUrl &url, const QByteArray &,
		    const QMap<QByteArray, QByteArray> &headers) {
			++sessionCalls;
			check(url.path() == "/api/ingest/broadcasts/" + recording + "/captures/" + capture,
			      "only scoped status request leaves transport");
			check(headers.value("Authorization") == "Bearer hhc_ingest_example.signature",
			      "session bearer, not stream key");
			return hhc::HttpResponse{
				409, "{\"data\":null,\"meta\":{},\"error\":{\"code\":\"broadcast_ingest_revoked\"}}",
				0};
		});
	auto scopedApi = scoped.api();
	bool revoked = false, denied = false;
	try {
		scopedApi.request("GET", base);
	} catch (const hhc::RequestError &e) {
		revoked = e.code == "broadcast_ingest_revoked";
	}
	try {
		scopedApi.request("POST", "/admin/recordings");
	} catch (const hhc::RequestError &) {
		denied = true;
	}
	check(revoked && denied && sessionCalls == 1,
	      "revocation preserved and Admin rejected before sending credential");
	hhc::KeyIngest badRenewal(
		temp.path(), "local-test", vault,
		[&](const QByteArray &, const QUrl &, const QByteArray &, const QMap<QByteArray, QByteArray> &) {
			auto bad = session;
			bad["captureId"] = QString(32, 'b');
			return hhc::HttpResponse{200,
						 QJsonDocument(QJsonObject{{"data", bad},
									   {"meta", QJsonObject{}},
									   {"error", QJsonValue::Null}})
							 .toJson(),
						 0};
		});
	bool switched = false;
	try {
		badRenewal.bearer(true);
	} catch (const hhc::RequestError &) {
		switched = true;
	}
	check(switched && QJsonDocument::fromJson(secrets["session-local-test"]).object()["captureId"] == capture,
	      "renewal cannot switch capture or overwrite original vault session");

	QFile c1("tests/fixtures/c1.json");
	check(c1.open(QIODevice::ReadOnly), "read capture fixtures");
	QJsonObject state;
	for (const auto &v : QJsonDocument::fromJson(c1.readAll()).array())
		if (v.toObject()["name"] == "poll")
			state = v.toObject()["value"].toObject()["data"].toObject();
	state["recordingId"] = recording;
	state["captureId"] = capture;
	state["expiresAt"] = "2099-01-01T00:00:00Z";
	int aborts = 0;
	QJsonObject abortBody;
	hhc::ApiClient cleanup(
		QUrl("https://admin.alive.org.tw/api"), [](bool) { return QByteArray("test"); },
		[&](const QByteArray &method, const QUrl &url, const QByteArray &bytes,
		    const QMap<QByteArray, QByteArray> &) {
			QJsonObject data = state;
			if (method == "POST") {
				check(url.path().endsWith("/abort"), "revoked cleanup never sends Stop or upload");
				const auto body = QJsonDocument::fromJson(bytes).object();
				++aborts;
				state["state"] = "aborted";
				if (aborts == 1) {
					abortBody = body;
					throw hhc::RequestError(0, "transport_unavailable");
				}
				check(body == abortBody, "abort retry has identical key and body");
				data = {{"capture", state},
					{"receipt", QJsonObject{{"operationKey", body["operationKey"]},
								{"operation", "abort"},
								{"captureId", capture},
								{"acceptedAt", "2026-10-14T00:00:00Z"}}}};
			}
			return hhc::HttpResponse{200,
						 QJsonDocument(QJsonObject{{"data", data},
									   {"meta", QJsonObject{}},
									   {"error", QJsonValue::Null}})
							 .toJson(),
						 0};
		});
	try {
		const auto actor = binding["actorId"].toString();
		hhc::CaptureSync first(temp.path() + "/abort", actor, "abort-test", cleanup);
		first.adopt(broadcast);
		try {
			first.abort();
		} catch (const hhc::RequestError &) {
		}
		hhc::CaptureSync recovered(temp.path() + "/abort", actor, "abort-test", cleanup);
		check(recovered.abort().state == "aborted" && aborts == 2,
		      "lost abort receipt replayed after restart despite terminal status");
	} catch (const std::exception &e) {
		std::cerr << "FAIL abort recovery " << e.what() << '\n';
		++failed;
	}
	qInstallMessageHandler(oldHandler);
	const auto logs = diagnosticMessages.join('\n');
	check(logs.contains("[HHC renew]") && logs.contains("ttlSec=") && logs.contains("result=renewed") &&
	      logs.contains("result=rejected"), "renewal diagnostics distinguish success and rejection");
	check(!logs.contains("local-test") && !logs.contains("hhc_stream_") && !logs.contains("hhc_ingest_") &&
	      !logs.contains("Authorization") && !logs.contains("sessionToken"), "renewal diagnostics contain only hashed identity");
	return failed ? 1 : 0;
}
