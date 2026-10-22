#include "ingest.hpp"
#include "wire.hpp"
#include "key-transfer.hpp"
#include "broadcast-control.hpp"
#include "windows-path.hpp"
#include "session-store.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <iostream>
#include <memory>
#include <QLockFile>
#include <QCryptographicHash>
#include <QUrlQuery>

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
	// Read-only probe of existing release receipts; no OBS, credential store, transport or journal writes.
	if (app.arguments().size()==7 && app.arguments()[1]=="--check-release") {
		int requests=0;
		ApiClient offline(QUrl("https://fixture.invalid/api"),[](bool){return QByteArray{};},
			[&](const auto &,const auto &,const auto &,const auto &)->HttpResponse{++requests;throw RequestError(0,"unexpected_network");});
		try {
			CaptureSync saved(app.arguments()[2],app.arguments()[3],app.arguments()[4],offline);
			const bool released=saved.hasReleaseReceipt(app.arguments()[5],app.arguments()[6]);
			std::cout<<"matchingReleaseReceipt="<<released<<" httpCalls="<<requests<<"\n";
			return released && requests==0 ? 0 : 1;
		} catch (const std::exception &e) {
			std::cerr<<"release proof rejected: "<<e.what()<<"\n";
			return 1;
		}
	}
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
	bool lost = true, replaying = false;
	auto transport = [&](const QByteArray &method, const QUrl &url, const QByteArray &body, const auto &headers) {
		++calls;
		if (replaying) {
			check(method == "POST" && url.path() == "/api/ingest/streams/replay", "lost credential uses K5, never K4 reopen");
			const auto data = response["data"].toObject(), session = data["session"].toObject();
			QJsonObject replay{{"protocolVersion","k5-2026-10-20.rc1"},{"outputSessionId",data["outputSessionId"]},
				{"sourceId",data["sourceId"]},{"recordingId",session["recordingId"]},
				{"captureId",session["captureId"]},{"epoch",session["epoch"]},{"receipt",data["receipt"]},
				{"state","active"},{"session",session}};
			return HttpResponse{200,QJsonDocument(QJsonObject{{"data",replay},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
		}
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
	// A lost cached session validates original identity through replay, never repeated open.
	replaying = true;
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
	replaying = false;
	response = fixtures[0].toObject()["value"].toObject();
	// All K4 errors remain unknown; an error label is not authenticated K5 release proof.
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
		check(attempts == 2 && !read(errors.path()+"/"+id+".json").contains("terminalProof"), "generic errors never manufacture a terminal tombstone");
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
	// Large active captures keep fresh authority without walking every status page twice.
	for (int count : {2001, 4500}) {
		QTemporaryDir paged;
		secrets.remove("session-" + id);
		auto opened = fixtures[0].toObject()["value"].toObject()["data"].toObject();
		opened["mediaProfile"] = "obs-hls-10s-v1";
		auto session = opened["session"].toObject();
		session["expiresAt"] = "2099-01-01T00:00:00Z";
		opened["session"] = session;
		const auto actor = opened["broadcast"].toObject()["binding"].toObject()["actorId"].toString();
		QJsonObject status;
		QFile fixed("tests/fixtures/c1.json");
		check(fixed.open(QIODevice::ReadOnly), "pagination C1 fixture readable");
		for (const auto &v : QJsonDocument::fromJson(fixed.readAll()).array())
			if (v.toObject()["name"] == "poll") status = v.toObject()["value"].toObject()["data"].toObject();
		status["recordingId"] = session["recordingId"];
		status["captureId"] = session["captureId"];
		status["expiresAt"] = "2099-01-01T00:00:00Z";
		status["declaredObjects"] = count;
		status["declaredBytes"] = count;
		QJsonArray objects;
		CaptureJournal local;
		local.account = actor;
		local.localId = id;
		SessionStore pagedStore(paged.path() + "/queue");
		const auto mediaDir = pagedStore.mediaDirectory(actor, id);
		const QStringList qualities{"1080p", "720p", "480p"};
		for (const auto &quality : qualities) QDir().mkpath(mediaDir + "/" + quality);
		const auto hash = QString::fromLatin1(QCryptographicHash::hash("x", QCryptographicHash::Sha256).toHex());
		for (int i = 0; i < count; ++i) {
			const auto path = qualities[i % 3] + QString("/seg-%1.m4s").arg(i / 3, 6, 10, QChar('0'));
			QFile mediaFile(mediaDir + "/" + path);
			check(mediaFile.open(QIODevice::WriteOnly) && mediaFile.write("x") == 1, "create owned pagination fixture bytes");
			mediaFile.close();
			objects.append(QJsonObject{{"path", path}, {"sizeBytes", 1}, {"sha256", hash}, {"state", "queued"}});
			local.objects.append({path, 1, hash, false});
		}
		int reads = 0, unexpected = 0;
		auto wire = [&](const QByteArray &method, const QUrl &url, const QByteArray &, const auto &) {
			QJsonObject value;
			if (url.path() == "/api/ingest/streams/open") value = opened;
			else if (url.path().endsWith("/control"))
				value = {{"revision", 3}, {"phase", "live"}, {"pendingCommand", QJsonValue::Null},
					{"controlVersion", "d1-2026-10-17.rc2"}, {"recordingId", session["recordingId"]},
					{"captureId", session["captureId"]}, {"epoch", session["epoch"]},
					{"boundaries", QJsonObject{{"startSequence", 0}, {"endSequenceExclusive", QJsonValue::Null}, {"revision", 3}}}};
			else if (method == "GET" && url.path().endsWith("/captures/" + session["captureId"].toString())) {
				++reads;
				const QUrlQuery query(url);
				const auto cursor = query.queryItemValue("cursor");
				const int offset = cursor.isEmpty() ? 0 : cursor.mid(1).toInt();
				const int limit = query.queryItemValue("limit").toInt();
				check(limit >= 1 && limit <= 1000, "pagination uses supported limits");
				QJsonArray page;
				for (int i = offset; i < qMin(offset + limit, count); ++i) page.append(objects[i]);
				value = status;
				value["objects"] = page;
				value["nextCursor"] = offset + limit < count ? "p" + QString::number(offset + limit) : QString{};
			} else ++unexpected;
			return HttpResponse{200, QJsonDocument(QJsonObject{{"data", value}, {"meta", QJsonObject{}}, {"error", QJsonValue::Null}}).toJson()};
		};
		bool validated = false;
		try {
			KeyIngest ingest(paged.path() + "/intents", id, vault, wire);
			ingest.prepareOpen("obs-hls-10s-v1");
			ingest.bind();
			pagedStore.save(local);
			const auto result = keyTransferStep(paged.path(), id, true, false, false, validated, nullptr, vault, wire);
			std::cout << "pagination objects=" << count << " GETs=" << reads << '\n';
			check(reads == 1 + (count + 999) / 1000, "active round has one authority summary and one full status traversal");
			check(result.sync.pendingObjects == 0 && unexpected == 0, "paged active round retains all queued objects without writes");
		} catch (const std::exception &e) {
			std::cerr << "FAIL pagination " << e.what() << '\n';
			++failures;
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
	// Preserve canonical bytes independently of checkout newlines and runtime schema embedding.
	for (const auto &[name, hash] : QMap<QString, QByteArray>{
		{"manifest.json","17d02cb529d45b6f5b2820b2e2d89fce553017978fa5e08a4be029e499aa249d"},
		{"schema.json","38ab8c99e1204c29153b35fbbb2d05264c4db80d22f60d97b3cbb2efae4a0b67"},
		{"wire-fixtures.json","77a9647492406bcc538e7a2b2815ca2b652d2000280d3d03b8744ea0c50ec27f"},
		{"semantic-fixtures.json","6602649e794885c58c126d52becc4a031a651b91a96e972139bcd1e4a0211b31"},
		{"server-rules.json","e7a57f6ec6658d231b4967f642aad814fe58da758a932fa84aca29f4451749fd"},
		{"contract.md","5554aa915bef3481e0648b4d9a948e1693c49b7b99c7215e089171a0268d386b"},
		{"openapi.snapshot.yaml","40dfe452df397621d50ba84a5b03b92c48bdf4f788216d881982d99b32dbc439"}}.asKeyValueRange()) {
		QFile fixed("tests/fixtures/k5/"+name);
		check(fixed.open(QIODevice::ReadOnly) && QCryptographicHash::hash(fixed.readAll(),QCryptographicHash::Sha256).toHex()==hash,qPrintable("K5 fixed bytes "+name));
	}
	QFile embedded("src/k5-schema.inc"), canonical("tests/fixtures/k5/schema.json");
	check(embedded.open(QIODevice::ReadOnly) && canonical.open(QIODevice::ReadOnly) && embedded.readAll()=="R\"K5("+canonical.readAll()+")K5\"\n", "embedded K5 schema is exact canonical bytes");
	// Frozen K5 schema parity. Semantic identity/release checks are exercised below.
	for (const auto &v : read("tests/fixtures/k5/wire-fixtures.json")["fixtures"].toArray()) {
		const auto f = v.toObject();
		check(validWire(f["schema"].toString(), f["value"]) == f["valid"].toBool(), qPrintable(f["name"].toString()));
	}
	// Renewal recovery is read/renew-only: no second open, resolve, bind or new output.
	for (const QString mode : {"renewed", "active", "terminal", "401", "403", "404", "409", "422", "429", "503", "timeout", "cancelled", "malformed", "201", "version", "output", "source", "capture", "epoch", "recording", "profile", "receipt", "terminal-session", "terminal-lock", "active-session", "no-pin", "missing-key", "missing-receipt", "accepted-receipt", "legacy900"}) {
		QTemporaryDir recovery;
		auto opened = fixtures[0].toObject()["value"].toObject();
		auto data = opened["data"].toObject();
		const bool shortProfile = mode != "legacy900";
		if (shortProfile) data["mediaProfile"] = "obs-hls-10s-v1";
		auto session = data["session"].toObject();
		session["expiresAt"] = "2099-01-01T00:00:00Z";
		data["session"] = session;
		opened["data"] = data;
		secrets.remove("session-" + id);
		secrets["stream-key"] = "hhc_stream_" + QByteArray(43, 'A');
		int opens = 0, renews = 0, replays = 0;
		std::unique_ptr<QLockFile> terminalLock;
		auto failure = [](int status, const QString &code) {
			return HttpResponse{status, QJsonDocument(QJsonObject{{"error", QJsonObject{{"code", code}}}}).toJson()};
		};
		auto recover = [&](const auto &, const QUrl &url, const QByteArray &bytes, const auto &) -> HttpResponse {
			const auto request = QJsonDocument::fromJson(bytes).object();
			if (url.path() == "/api/ingest/streams/open") {
				++opens;
				check(opens == 1, "K5 recovery must never invoke K4 open");
				return opens == 1 ? HttpResponse{200, QJsonDocument(opened).toJson()} : failure(409, "broadcast_output_terminal");
			}
			if (url.path().endsWith("/session")) {
				++renews;
				check(request["streamKey"] == "hhc_stream_" + QString(43, 'A'), "renew uses original output key, not changed global key");
				if (mode == "renewed")
					return HttpResponse{200, QJsonDocument(QJsonObject{{"data", session}, {"meta", QJsonObject{}}, {"error", QJsonValue::Null}}).toJson()};
				return failure(401, "broadcast_ingest_session_invalid");
			}
			++replays;
			check(url.path() == "/api/ingest/streams/replay", "only authenticated replay after invalid renewal");
			check(request["protocolVersion"] == "k5-2026-10-20.rc1" && request["outputSessionId"] == id &&
			      request["sourceId"] == data["sourceId"] && request["receipt"] == data["receipt"] &&
			      request["recordingId"] == session["recordingId"] && request["captureId"] == session["captureId"] &&
			      request["epoch"] == session["epoch"] && request.value("mediaProfile") == data.value("mediaProfile") &&
			      request["streamKey"] == "hhc_stream_" + QString(43, 'A'), "replay pins original full identity, receipt, profile and key");
			if (mode == "timeout") throw RequestError(0, "transport_unavailable");
			if (mode == "cancelled") throw RequestError(0, "transport_cancelled");
			bool numeric = false;
			const auto status = mode.toInt(&numeric);
			if (numeric && status != 201) return failure(status, "broadcast_output_terminal");
			if (mode == "malformed") return {200, "{}"};
			if (mode == "terminal-lock") {
				terminalLock = std::make_unique<QLockFile>(lockFilePath(recovery.path()+"/"+id+".metadata.lock"));
				terminalLock->setStaleLockTime(0);
				check(terminalLock->tryLock(0),"inject terminal proof lock contention");
			}
			auto replay = request;
			replay.remove("streamKey");
			replay["state"] = mode == "terminal" || mode == "terminal-session" || mode == "terminal-lock" ? "terminal" : "active";
			if (mode != "terminal" && mode != "terminal-lock") replay["session"] = session;
			if (mode == "version") replay["protocolVersion"] = "k4-2026-10-15.rc1";
			if (mode == "output") replay["outputSessionId"] = "018f0000-0000-4000-8000-000000000099";
			if (mode == "source") replay["sourceId"] = "018f0000-0000-4000-8000-000000000099";
			if (mode == "capture") replay["captureId"] = QString(32, 'b');
			if (mode == "epoch") replay["epoch"] = 2;
			if (mode == "recording") replay["recordingId"] = "018f0000-0000-4000-8000-000000000099";
			if (mode == "profile") replay.remove("mediaProfile");
			if (mode == "receipt") { auto r = replay["receipt"].toObject(); r["operationKey"] = "other.bind"; replay["receipt"] = r; }
			if (mode == "active-session") { auto changed = session; changed["captureId"] = QString(32, 'b'); replay["session"] = changed; }
			return {mode == "201" ? 201 : 200, QJsonDocument(QJsonObject{{"data", replay}, {"meta", QJsonObject{}}, {"error", QJsonValue::Null}}).toJson()};
		};
		KeyIngest initial(recovery.path(), id, vault, recover);
		initial.prepareOpen(shortProfile ? "obs-hls-10s-v1" : "");
		initial.bind();
		secrets["stream-key"] = "hhc_stream_" + QByteArray(42, 'B') + 'A';
		if (mode == "no-pin" || mode == "missing-receipt" || mode == "accepted-receipt") {
			auto saved = initial.metadata();
			if (mode == "no-pin") saved.remove("credentialRef");
			else if (mode == "missing-receipt") saved.remove("receipt");
			else { auto r = saved["receipt"].toObject(); r["state"] = "accepted"; saved["receipt"] = r; }
			check(writeAtomicMetadata(recovery.path()+"/"+id+".json",QJsonDocument(saved).toJson()),"write legacy/incomplete fixture");
		}
		if (mode == "missing-key") secrets.remove("output-key-"+id);
		QFile retained(recovery.path() + "/retained.m4s");
		check(retained.open(QIODevice::WriteOnly) && retained.write("original-media") == 14, "create retained media");
		retained.close();
		const auto credentialBefore = secrets["session-" + id];
		auto lateRenewal = [&](const auto &, const auto &, const auto &, const auto &) {
			auto late = session;
			late["token"] = "hhc_ingest_late.signature";
			return HttpResponse{200,QJsonDocument(QJsonObject{{"data",late},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
		};
		// Construct stale worker before terminal proof is saved by another worker.
		std::unique_ptr<KeyIngest> stale;
		if (mode == "terminal") stale = std::make_unique<KeyIngest>(recovery.path(), id, vault, lateRenewal);
		bool accepted = false, terminal = false;
		std::unique_ptr<KeyIngest> sameInstance;
		try {
			sameInstance=std::make_unique<KeyIngest>(recovery.path(), id, vault, recover);
			sameInstance->bearer(true); accepted = true;
		}
		catch (const RequestError &e) {
			terminal = e.outputReleased && e.code == "output_replay_terminal";
			unsigned attempts=0;
			if (!terminal && mode!="terminal-lock") check(!e.retryDelayMs(attempts),"K5 unknown stays paused, not normal upload error handling");
		}
		if (mode == "terminal-lock") {
			terminalLock.reset();
			check(!sameInstance->metadata().contains("terminalProof") && !read(recovery.path()+"/"+id+".json").contains("terminalProof"), "failed persistence leaves no in-memory or disk release proof");
			try { sameInstance->bearer(); } catch (const RequestError &e) { check(!e.outputReleased,"same instance never releases unsaved proof"); }
		}
		const bool lacksProof = mode == "no-pin" || mode == "missing-key" || mode == "missing-receipt" || mode == "accepted-receipt";
		check(accepted == (mode == "renewed" || mode == "active" || mode == "legacy900"), qPrintable("K5 renew/replay " + mode));
		check(terminal == (mode == "terminal"), qPrintable("only authenticated 200 terminal releases: " + mode));
		check(opens == 1 && renews <= 1 && replays == (mode == "renewed" || lacksProof ? 0 : 1), "no implicit new output or fallback");
		check(secrets["session-" + id] == credentialBefore, "rejected identity never overwrites credential");
		check(retained.open(QIODevice::ReadOnly) && retained.readAll() == "original-media", "all results retain original media");
		if (mode == "terminal") {
			try { stale->bearer(true); check(false,"stale renewal cannot erase terminal proof"); }
			catch (const RequestError &e) { check(e.outputReleased && e.code == "output_replay_terminal", "stale metadata writer observes durable release"); }
			check(secrets["session-"+id] == credentialBefore, "stale renewal never overwrites released session credential");
			try { KeyIngest(recovery.path(), id, vault, recover).bind(); check(false, "terminal survives restart"); }
			catch (const RequestError &e) { check(e.code == "output_replay_terminal" && opens == 1 && replays == 1, "restart reads durable proof without HTTP"); }
		}
	}
	// Exercise the real helper/transfer entry with a stopped original journal: no cleanup or rebind.
	for (const int replayStatus : {0,403,409}) {
		const bool denied = replayStatus != 0;
		QTemporaryDir worker;
		secrets["stream-key"] = "hhc_stream_"+QByteArray(43,'A');
		secrets.remove("session-"+id);
		auto data = fixtures[0].toObject()["value"].toObject()["data"].toObject();
		auto session = data["session"].toObject();
		session["expiresAt"] = "2099-01-01T00:00:00Z";
		data["session"] = session;
		int opens=0, replays=0;
		auto wire = [&](const auto &,const QUrl &url,const QByteArray &bytes,const auto &)->HttpResponse {
			if (url.path()=="/api/ingest/streams/open") {
				++opens;
				return {200,QJsonDocument(QJsonObject{{"data",data},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
			}
			if (denied && url.path()!="/api/ingest/streams/replay")
				return {401,QJsonDocument(QJsonObject{{"error",QJsonObject{{"code","broadcast_ingest_session_invalid"}}}}).toJson()};
			++replays;
			check(url.path()=="/api/ingest/streams/replay","helper only replays original output");
			if (denied) return {replayStatus,QJsonDocument(QJsonObject{{"error",QJsonObject{{"code",replayStatus==403 ? "broadcast_ingest_revoked" : "broadcast_output_terminal"}}}}).toJson()};
			auto reply = QJsonDocument::fromJson(bytes).object();
			reply.remove("streamKey"); reply["state"]="terminal";
			return {200,QJsonDocument(QJsonObject{{"data",reply},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
		};
		KeyIngest original(worker.path()+"/intents",id,vault,wire);
		original.prepareOpen(); original.bind();
		if (!denied) secrets.remove("session-"+id);
		SessionStore store(worker.path()+"/queue");
		CaptureJournal journal;
		journal.account=data["broadcast"].toObject()["binding"].toObject()["actorId"].toString();
		journal.localId=id; journal.remoteCapture=session["captureId"].toString();
		journal.normalEnd=true; journal.stopIntent=true;
		store.save(journal);
		const auto directory=store.mediaDirectory(journal.account,id);
		const auto before=read(directory+"/journal.json");
		check(writeAtomicMetadata(directory+"/stop-seal-receipts.json","{\"operationKey\":\"original.seal\"}"),"write original receipt evidence");
		markIngestStopping(worker.path(),id);
		bool validated=false;
		KeyIngest controlIngest(worker.path()+"/intents",id,vault,wire);
		auto controlApi = controlIngest.api();
		BroadcastControl control(directory,journal.account,id,controlApi,true);
		if (replayStatus==409) control.adoptIngest(data,original.metadata()["intent"].toObject());
		for (int attempt=0;attempt<2;++attempt) {
			const int beforeReplay=replays;
			try {
				if (replayStatus==409) control.poll({},true);
				else keyTransferStep(worker.path(),id,false,false,false,validated,nullptr,vault,wire);
				check(false,"terminal/unknown returns explicit disposition");
			}
			catch(const RequestError &e) {
				check(denied ? !e.outputReleased && e.outputReplayUnknown && e.status==replayStatus && e.code==(replayStatus==403 ? "broadcast_ingest_revoked" : "broadcast_output_terminal") : e.outputReleased && e.code=="output_replay_terminal","control/helper preserves original unknown provenance or terminal proof");
			}
			check(replays-beforeReplay==(denied || attempt==0 ? 1 : 0),"one replay per explicit operation, no control reconciliation after unknown");
			check(!QFileInfo::exists(worker.path()+"/intents/"+id+".revoked"),"K5 error label must never mark original revoked/abort it");
		}
		check(opens==1 && replays==(denied?2:1) && !validated && read(directory+"/journal.json")==before,"helper restart retains original journal without new output");
		QFile receipt(directory+"/stop-seal-receipts.json");
		check(receipt.open(QIODevice::ReadOnly) && receipt.readAll()=="{\"operationKey\":\"original.seal\"}","helper retains original Stop/seal receipts");
	}
	// A legacy released pointer is not proof; normal matching Stop/seal receipts are sufficient.
	{
		QTemporaryDir legacy;
		const auto opened = fixtures[0].toObject()["value"].toObject()["data"].toObject();
		const auto binding = opened["session"].toObject();
		const auto actor = opened["broadcast"].toObject()["binding"].toObject()["actorId"].toString();
		QJsonObject capture, seal;
		QFile fixed("tests/fixtures/c1.json"); check(fixed.open(QIODevice::ReadOnly),"read normal release fixtures");
		for (const auto &v : QJsonDocument::fromJson(fixed.readAll()).array()) {
			const auto f=v.toObject();
			if (f["name"]=="poll") capture=f["value"].toObject()["data"].toObject();
			if (f["name"]=="seal") seal=f["value"].toObject();
		}
		capture["recordingId"]=binding["recordingId"];capture["captureId"]=binding["captureId"];
		capture["state"]="validating";capture["stopAcceptedAt"]="2026-10-20T00:00:00Z";
		seal["operationKey"]=id+".seal";
		const auto cap=binding["captureId"].toString(), rec=binding["recordingId"].toString();
		const auto base="/admin/recordings/"+rec+"/captures/"+cap;
		QJsonObject remote{{"version",1},{"account",actor},{"localId",id},{"title","legacy"},{"autoPublish",false},
			{"liveEnabled",true},{"recordingId",rec},{"recordingVersion",1},{"captureId",cap},
			{"mutations",QJsonObject{}},{"lastCapture",capture}};
		SessionStore store(legacy.path());
		const auto dir=store.mediaDirectory(actor,id);QDir().mkpath(dir);
		int requests=0;
		ApiClient none(QUrl("https://fixture.invalid/api"),[](bool){return QByteArray{};},[&](const auto &,const auto &,const auto &,const auto &)->HttpResponse{++requests;throw RequestError(0,"unexpected_network");});
		auto inspect=[&]{check(writeAtomicMetadata(dir+"/remote-journal.json",QJsonDocument(remote).toJson()),"write legacy release evidence");return CaptureSync(legacy.path(),actor,id,none).hasReleaseReceipt(rec,cap);};
		check(!inspect(),"terminal label/stop time without receipts cannot release a legacy pointer");
		QJsonObject mutations;
		for (const auto &op : {QString("stop"),QString("seal")}) {
			QJsonObject body=op=="seal" ? seal : QJsonObject{{"operationKey",id+".stop"}};
			QJsonObject receipt{{"operationKey",id+"."+op},{"operation",op},{"captureId",cap},{"acceptedAt","2026-10-20T00:00:00Z"}};
			mutations[op]=QJsonObject{{"body",body},{"method","POST"},{"path",base+"/"+op},{"receipt",receipt}};
			remote["mutations"]=mutations;
			check(inspect()==(op=="seal"),"legacy normal Stop/seal release requires accepted seal receipt");
		}
		check(!CaptureSync(legacy.path(),actor,id,none).hasReleaseReceipt(rec,QString(32,'f')),"release receipt must match exact original capture");
		mutations.remove("stop"); remote["mutations"]=mutations;
		check(!inspect(),"seal-only plus stop timestamp cannot substitute for original Stop receipt");
		check(requests==0,"checking durable release evidence performs no HTTP");
	}
	return failures ? 1 : 0;
}
