#include "ingest.hpp"
#include "wire.hpp"
#include "key-transfer.hpp"
#include "broadcast-control.hpp"
#include "windows-path.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <iostream>

using namespace hhc;
namespace {
int failures = 0;
void check(bool ok, const char *name)
{
	if (!ok) {
		++failures;
		std::cerr << "FAIL " << name << '\n';
	}
}
QJsonObject read(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly))
		throw std::runtime_error("fixture unavailable");
	return QJsonDocument::fromJson(f.readAll()).object();
}
} // namespace
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QFile s10("tests/fixtures/s10.json");
	check(s10.open(QIODevice::ReadOnly), "S10 rc2 fixtures readable");
	for (const auto &v : QJsonDocument::fromJson(s10.readAll()).array()) {
		const auto f=v.toObject();
		check(validWire(f["schema"].toString(),f["value"])==f["valid"].toBool(), qPrintable(f["name"].toString()));
	}
	s10.seek(0);
	QJsonObject rendition;
	for (const auto &v : QJsonDocument::fromJson(s10.readAll()).array())
		if (v.toObject()["schema"] == "RecordingRendition") { rendition=v.toObject()["value"].toObject(); break; }
	for (int count : {1440,1441,4320,4321}) {
		rendition["segmentCount"]=count;
		check(validWire("RecordingRendition",rendition)==(count<=4320), "S10 rendition envelope boundary");
	}
	rendition["segmentCount"]=4320;
	QFile c1("tests/fixtures/c1.json");
	check(c1.open(QIODevice::ReadOnly),"C1 status fixture readable");
	QJsonObject status;
	for (const auto &v : QJsonDocument::fromJson(c1.readAll()).array())
		if(v.toObject()["schema"]=="RecordingCaptureStatus" && v.toObject()["valid"].toBool()) {status=v.toObject()["value"].toObject(); break;}
	for (int count : {10000,13000,13001}) {
		status["declaredObjects"]=count;
		check(validWire("RecordingCaptureStatus",status)==(count<=13000), "S10 declared total boundary");
		QJsonArray objects;
		for(int i=0;i<count;++i) objects.append(QJsonObject{{"path",QString("720p/seg-%1.m4s").arg(i,6,10,QChar('0'))},{"sizeBytes",1},{"sha256",QString(64,'a')}});
		QJsonObject inventory{{"schemaVersion",1},{"presetVersion","obs-hls-10s-v1"},{"inventoryDigest",QString(64,'b')},{"renditions",QJsonArray{rendition}},{"objects",objects}};
		check(validWire("RecordingPackageInventory",inventory)==(count<=13000),"S10 inventory array boundary (schema only)");
	}
	QFile d1("tests/fixtures/d1.json");
	check(d1.open(QIODevice::ReadOnly), "D1 fixtures readable");
	for (const auto &v : QJsonDocument::fromJson(d1.readAll()).array()) {
		const auto f=v.toObject();
		check(validWire(f["schema"].toString(),f["value"])==f["valid"].toBool(), qPrintable(f["name"].toString()));
	}
	for (const auto &v : read("tests/fixtures/k4.json")["fixtures"].toArray()) {
		auto f = v.toObject();
		check(validWire(f["schema"].toString(), f["value"]) == f["valid"].toBool(),
		      qPrintable(f["name"].toString()));
	}
	const auto fixtures = read("tests/fixtures/k4-identity.json")["fixtures"].toArray();
	auto response = fixtures[0].toObject()["value"].toObject();
	const auto id = response["data"].toObject()["outputSessionId"].toString();
	check(validWire("BroadcastOutputOpenInput", QJsonObject{{"protocolVersion", "k4-2026-10-15.rc1"},
		{"outputSessionId", id}, {"streamKey", "hhc_stream_" + QString(43, 'A')},
		{"mediaProfile", "obs-hls-10s-v1"}}), "S10 optional profile is accepted by frozen schema");
	QTemporaryDir dir;
	QMap<QString, QByteArray> secrets{{"stream-key", "hhc_stream_" + QByteArray(43, 'A')}};
	IngestCredentials vault{[&](const QString &key) -> std::optional<QByteArray> {
					return secrets.contains(key) ? std::optional<QByteArray>(secrets[key])
								     : std::nullopt;
				},
				[&](const QString &key, const QByteArray &value) {
					secrets[key] = value;
				}};
	int calls = 0;
	bool lost = true;
	auto transport = [&](const QByteArray &method, const QUrl &url, const QByteArray &body, const auto &headers) {
		++calls;
		check(method == "POST" && url.path() == "/api/ingest/streams/open", "only K4 open");
		check(!headers.contains("Authorization"), "bootstrap has no bearer");
		auto request = QJsonDocument::fromJson(body).object();
		check(request.size() == 3 && request["outputSessionId"] == id, "same ID exact request");
		auto persisted = read(dir.path() + "/" + id + ".json");
		check(persisted["intent"].toObject()["outputSessionId"] == id, "intent durable before request");
		check(!QJsonDocument(persisted).toJson().contains("hhc_stream_"), "no key in journal");
		if (lost)
			throw RequestError(0, "transport_unavailable");
		return HttpResponse{200, QJsonDocument(response).toJson()};
	};
	{
		KeyIngest ingest(dir.path(), id, vault, transport);
		ingest.prepareOpen();
		try {
			ingest.bind();
			check(false, "lost response must fail");
		} catch (const RequestError &) {
		}
	}
	lost = false;
	{
		KeyIngest restarted(dir.path(), id, vault, transport);
		auto result = restarted.bind();
		check(result["broadcast"].isObject() && calls == 2, "restart resumes same output");
		restarted.bind();
		check(calls == 2, "bound upload never reopens");
	}
	// Remove only the fake session to exercise identity validation of a repeated open.
	for (const auto &v : fixtures) {
		auto f = v.toObject();
		response = f["value"].toObject();
		secrets.remove("session-" + id);
		bool accepted = false;
		try {
			KeyIngest restarted(dir.path(), id, vault, transport);
			restarted.bind();
			accepted = true;
		} catch (const RequestError &) {
		}
		check(accepted == f["validIdentity"].toBool(), qPrintable(f["name"].toString()));
	}
	response = fixtures[0].toObject()["value"].toObject();
	// Every rejection keeps the same opening intent; terminal is a durable tombstone.
	for (int status : {202, 400, 403, 404, 405, 413, 422, 429, 503, 409}) {
		QTemporaryDir errors;
		secrets.remove("session-" + id);
		QString code = status == 409 ? "broadcast_output_terminal" : "unavailable";
		int attempts = 0;
		auto fail = [&](const auto &, const auto &, const auto &, const auto &) {
			++attempts;
			auto envelope =
				status == 202
					? response
					: QJsonObject{{"data", QJsonValue::Null},
						      {"meta", QJsonObject{}},
						      {"error", QJsonObject{{"code", code}, {"message", "fixture"}}}};
			return HttpResponse{status, QJsonDocument(envelope).toJson(), 601};
		};
		KeyIngest opening(errors.path(), id, vault, fail);
		opening.prepareOpen();
		for (int retry = 0; retry < 2; ++retry) {
			try {
				KeyIngest(errors.path(), id, vault, fail).bind();
				check(false, "reject non-200/error");
			} catch (const RequestError &e) {
				unsigned retries = 0;
				const auto delay = e.retryDelayMs(retries);
				check(e.outputOpening, "preserve opening error classification");
				check(bool(delay) == (status == 429 || status == 503), "only transient errors retry");
				if (delay)
					check(*delay >= 601000, "honor Retry-After beyond old cap");
			}
		}
		check(attempts == (status == 409 ? 1 : 2), "terminal cannot reopen after restart");
		check(read(errors.path() + "/" + id + ".json")["intent"].toObject()["outputSessionId"] == id,
		      "all errors retain ID");
	}
	for (const auto &code : {"broadcast_source_busy", "broadcast_source_not_prepared",
				 "broadcast_output_capacity_exhausted", "broadcast_active_limit"}) {
		RequestError e(409, code);
		e.outputOpening = true;
		unsigned count = 0;
		for (int base : {1000, 2000, 4000, 8000, 16000, 30000, 30000}) {
			auto delay = e.retryDelayMs(count);
			check(delay && *delay >= base * 4 / 5 && *delay <= base * 6 / 5, "bounded exponential jitter");
		}
	}
	// Actual transfer/adoption code: lost response, stopping, restart, original abort. No encoder or network.
	for (int cleanupCase : {0,1,2}) {
		const bool endedBeforeResponse=cleanupCase==1, endedOnControl=cleanupCase==2;
		QTemporaryDir transfer;
		secrets.remove("session-" + id);
		auto data = response["data"].toObject();
		if(endedOnControl) data["mediaProfile"]="obs-hls-10s-v1";
		auto session = data["session"].toObject();
		session["expiresAt"] = "2099-01-01T00:00:00Z";
		data["session"] = session;
		if (endedBeforeResponse) {
			auto b = data["broadcast"].toObject();
			b["phase"] = "processing";
			auto bounds = b["boundaries"].toObject();
			bounds["startSequence"] = 0;
			bounds["endSequenceExclusive"] = 2;
			b["boundaries"] = bounds;
			data["broadcast"] = b;
		}
		QJsonObject state;
		QFile c1("tests/fixtures/c1.json");
		check(c1.open(QIODevice::ReadOnly), "read C1 fixtures");
		for (const auto &v : QJsonDocument::fromJson(c1.readAll()).array())
			if (v.toObject()["name"] == "poll")
				state = v.toObject()["value"].toObject()["data"].toObject();
		state["recordingId"] = session["recordingId"];
		state["captureId"] = session["captureId"];
		state["expiresAt"] = "2099-01-01T00:00:00Z";
		int opens = 0, aborts = 0;
		auto pipeline = [&](const QByteArray &method, const QUrl &url, const QByteArray &bytes, const auto &) {
			QJsonObject result = state;
			if (url.path() == "/api/ingest/streams/open") {
				++opens;
				check(QJsonDocument::fromJson(bytes).object()["outputSessionId"] == id,
				      "cleanup reuses output ID");
				if (opens == 1)
					throw RequestError(0, "transport_unavailable");
				result = data;
			} else if(url.path().endsWith("/control")) {
				check(endedOnControl,"only opted-in recovery negotiates control here");
				result={{"revision",10},{"phase","processing"},{"pendingCommand",QJsonValue::Null},
					{"controlVersion","d1-2026-10-17.rc2"},{"recordingId",session["recordingId"]},
					{"captureId",session["captureId"]},{"epoch",session["epoch"]},
					{"boundaries",QJsonObject{{"startSequence",0},{"endSequenceExclusive",0},{"revision",4}}}};
			} else if (method == "POST") {
				check(url.path().endsWith("/abort"), "unstarted cleanup only aborts original");
				++aborts;
				state["state"] = "aborted";
				result = {{"capture", state},
					  {"receipt",
					   QJsonObject{{"operationKey",
							QJsonDocument::fromJson(bytes).object()["operationKey"]},
						       {"operation", "abort"},
						       {"captureId", session["captureId"]},
						       {"acceptedAt", "2026-10-15T00:00:00Z"}}}};
			} else
				check(url.path().contains("/captures/" + session["captureId"].toString()),
				      "only original capture read");
			return HttpResponse{200, QJsonDocument(QJsonObject{{"data", result},
									   {"meta", QJsonObject{}},
									   {"error", QJsonValue::Null}})
							 .toJson()};
		};
		KeyIngest(transfer.path() + "/intents", id, vault, pipeline).prepareOpen(endedOnControl ? "obs-hls-10s-v1" : "");
		bool validated = false;
		try {
			keyTransferStep(transfer.path(), id, false, false, true, validated, nullptr, vault, pipeline);
		} catch (const RequestError &) {
		}
		if (cleanupCase==0)
			markIngestStopping(transfer.path(), id);
		try {
			auto result = keyTransferStep(transfer.path(), id, false, false, true, validated, nullptr,
						      vault, pipeline);
			check(!result.bound && result.sync.state == "aborted" && opens == 2 && aborts == 1,
			      "cancel/restart or End before response cleans original without starting encoder");
			keyTransferStep(transfer.path(), id, false, false, false, validated, nullptr, vault, pipeline);
			check(opens == 2 && aborts == 1, "settled recovery does not reopen or repeat abort");
		} catch (const std::exception &e) {
			std::cerr << "FAIL cleanup " << e.what() << '\n';
			++failures;
		}
	}
	// S10 opening survives a lost response; missing/wrong echoes never install a binding.
	{
		QTemporaryDir shortDir;
		secrets.remove("session-" + id);
		int requests = 0;
		int mode = 0;
		auto shortTransport = [&](const auto &, const QUrl &url, const QByteArray &body, const auto &) -> HttpResponse {
			++requests;
			const auto sent = QJsonDocument::fromJson(body).object();
			check(url.path() == "/api/ingest/streams/open" && sent["mediaProfile"] == "obs-hls-10s-v1" && sent["outputSessionId"] == id, "retry pins S10 and original ID");
			check(read(shortDir.path()+"/"+id+".json")["intent"].toObject()["mediaProfile"] == "obs-hls-10s-v1", "profile durable before open");
			if (mode == 0) throw RequestError(0, "transport_unavailable");
			auto envelope = fixtures[0].toObject()["value"].toObject();
			auto data = envelope["data"].toObject();
			if (mode == 2) data["mediaProfile"] = "obs-hls-10s-v1";
			envelope["data"] = data;
			return {200, QJsonDocument(envelope).toJson()};
		};
		KeyIngest(shortDir.path(), id, vault, shortTransport).prepareOpen("obs-hls-10s-v1");
		for (mode = 0; mode < 2; ++mode) {
			bool rejected = false;
			try { KeyIngest(shortDir.path(), id, vault, shortTransport).bind(); }
			catch (const RequestError &) { rejected = true; }
			check(rejected && !secrets.contains("session-"+id), "lost response or missing echo leaves no accepted credentials");
		}
		try {
			KeyIngest restarted(shortDir.path(), id, vault, shortTransport);
			restarted.bind();
			check(requests == 3 && restarted.metadata()["intent"].toObject()["mediaProfile"] == "obs-hls-10s-v1", "S10 restart accepts exact echo once");
			KeyIngest(shortDir.path(),id,vault,shortTransport).bind();
			check(requests==3,"bound S10 restart uses original binding without reopen");
		} catch(const RequestError &e) { check(false,qPrintable("S10 echo: "+e.code)); }
	}
	{
		QTemporaryDir ended;
		const auto original=fixtures[0].toObject()["value"].toObject()["data"].toObject();
		const auto broadcast=original["broadcast"].toObject(), binding=broadcast["binding"].toObject();
		QJsonObject control{{"revision",10},{"phase","end_pending"},{"pendingCommand",QJsonValue::Null},
			{"controlVersion","d1-2026-10-17.rc2"},{"recordingId",broadcast["recordingId"]},
			{"captureId",binding["captureId"]},{"epoch",binding["epoch"]},
			{"boundaries",QJsonObject{{"startSequence",0},{"endSequenceExclusive",0},{"revision",4}}}};
		int posts=0;
		ApiClient client(QUrl("https://fixture.invalid/api"),[](bool){return QByteArray("fake");},
			[&](const QByteArray &method,const QUrl &url,const auto &,const auto &){
				posts+=method=="POST";
				check(url.query().contains("controlVersion=d1-2026-10-17.rc2"),"negotiate D1 control explicitly");
				return HttpResponse{200,QJsonDocument(QJsonObject{{"data",control},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
			});
		const auto actor=binding["actorId"].toString();
		BroadcastControl state(ended.path(),actor,id,client,true);
		state.adoptIngest(original,{{"protocolVersion","k4-2026-10-15.rc1"},{"outputSessionId",id}});
		state.poll({},true);
		check(state.serverEnd()==0 && posts==0,"zero cutoff durable without fake ACK");
		BroadcastControl resumed(ended.path(),actor,id,client,true);
		check(resumed.serverEnd()==0,"cutoff survives restart");
		control["revision"]=9;
		control["boundaries"]=QJsonObject{{"startSequence",QJsonValue::Null},{"endSequenceExclusive",QJsonValue::Null},{"revision",3}};
		resumed.poll({},true);
		check(resumed.serverEnd()==0,"older null snapshot cannot roll cutoff back");
		control["revision"]=11;
		control["boundaries"]=QJsonObject{{"startSequence",0},{"endSequenceExclusive",1},{"revision",5}};
		bool conflict=false;
		try { resumed.poll({},true); } catch(const RequestError &){conflict=true;}
		check(conflict && resumed.serverEnd()==0,"reject changed cutoff, preserve durable original");
		control["captureId"]=QString(32,'f');
		conflict=false;
		try { resumed.poll({},true); } catch(const RequestError &){conflict=true;}
		check(conflict && posts==0,"reject wrong capture without mutation");
	}
	return failures ? 1 : 0;
}
