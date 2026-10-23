#include "broadcast-control.hpp"
#include "wire.hpp"
#include "session-store.hpp"
#include "windows-path.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <QProcess>
#include <iostream>

static int preparedAfterPoll()
{
	QFile identityFile("tests/fixtures/k4-identity.json"), controlFile("tests/fixtures/d2-control.json");
	if (!identityFile.open(QIODevice::ReadOnly) || !controlFile.open(QIODevice::ReadOnly)) return 1;
	const auto opened = QJsonDocument::fromJson(identityFile.readAll()).object()["fixtures"].toArray()[0].toObject()["value"].toObject()["data"].toObject();
	const auto broadcast = opened["broadcast"].toObject(), binding = broadcast["binding"].toObject();
	const auto fixedControl = QJsonDocument::fromJson(controlFile.readAll()).array()[0].toObject()["value"].toObject();
	const auto actor = binding["actorId"].toString(), id = opened["outputSessionId"].toString();
	int failed = 0;
	auto check = [&](bool ok, const char *what) { if (!ok) { ++failed; std::cerr << "FAIL " << what << '\n'; } };
	for (bool fallback : {false, true}) {
		QTemporaryDir root;
		hhc::SessionStore store(root.path());
		hhc::CaptureJournal journal; journal.account = actor; journal.localId = id; store.save(journal);
		const auto directory = store.mediaDirectory(actor, id);
		hhc::ApiClient client(QUrl("https://fixture.invalid"), [](bool) { return QByteArray{}; },
			[&](const QByteArray &method, const QUrl &url, const QByteArray &, const QMap<QByteArray,QByteArray> &) {
				if (method != "GET" || !url.path().endsWith("/control")) throw std::runtime_error("Unexpected preparation request");
				const bool d2 = url.query().contains("d2-2026-10-23.rc1");
				if (fallback && d2) return hhc::HttpResponse{422, "{\"error\":{\"code\":\"broadcast_protocol_unsupported\"}}"};
				auto control = fixedControl;
				control["recordingId"] = broadcast["recordingId"]; control["captureId"] = binding["captureId"]; control["epoch"] = binding["epoch"];
				if (!d2) { control.remove("recovery"); control.remove("publicEnd"); control["controlVersion"] = "d1-2026-10-17.rc2"; }
				return hhc::HttpResponse{200, QJsonDocument(QJsonObject{{"data",control},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson()};
			});
		hhc::BroadcastControl control(directory, actor, id, client, true);
		control.adoptIngest(opened, {{"protocolVersion","k4-2026-10-15.rc1"},{"outputSessionId",id},{"recoveryVersion","common-watermark-v1"}});
		check(store.isPrepared(actor,id), "adopted binding remains pristine before first D2 poll");
		control.poll({},true);
		const auto metadata = directory + (fallback ? "/control-d1-fallback.json" : "/recovery-control.json");
		check(QFile::exists(metadata), "actual control poll persists preparation metadata");
		check(store.isPrepared(actor,id), fallback ? "D1 fallback metadata permits initial encoder start" : "D2 observation metadata permits initial encoder start");
		QFile unexpected(directory+"/unexpected.json");
		check(unexpected.open(QIODevice::WriteOnly), "create unknown preparation metadata"); unexpected.write("{}"); unexpected.close();
		check(!store.isPrepared(actor,id), "D2 preparation rejects unknown metadata");
		check(unexpected.remove(), "remove only owned unknown fixture");
		check(QDir().mkpath(directory+"/1080p"), "create owned media directory");
		check(!store.isPrepared(actor,id), "D2 preparation rejects existing media directories");
		check(QDir(directory).rmdir("1080p"), "remove only empty owned media directory");
		for (bool hhc::CaptureJournal::*flag : {&hhc::CaptureJournal::stopIntent,&hhc::CaptureJournal::normalEnd,&hhc::CaptureJournal::sealAcknowledged,&hhc::CaptureJournal::confirmedReady}) {
			journal.*flag=true; store.save(journal);
			check(!store.isPrepared(actor,id), "D2 metadata cannot make stopped/finalized journal pristine");
			journal.*flag=false; store.save(journal);
		}
#ifdef Q_OS_WIN
		QFile original(metadata); check(original.open(QIODevice::ReadOnly), "read owned control metadata"); const auto bytes=original.readAll(); original.close();
		check(original.remove(), "replace only owned metadata for redirect guard");
		QTemporaryDir outside;
		QProcess junction; junction.start("cmd.exe", {"/c","mklink","/J",hhc::lockFilePath(metadata),hhc::lockFilePath(outside.path())});
		const bool linked=junction.waitForFinished() && junction.exitCode()==0;
		check(linked, "create owned metadata junction");
		if (linked) {
			check(!store.isPrepared(actor,id), "known metadata name cannot redirect outside session");
			const auto link=hhc::lockFilePath(metadata);
			check(RemoveDirectoryW(reinterpret_cast<LPCWSTR>(link.utf16()))!=0, "remove only owned junction, preserve destination");
		}
		check(original.open(QIODevice::WriteOnly), "restore owned metadata"); original.write(bytes); original.close();
#endif
		check(store.isPrepared(actor,id), "guard checks leave valid preparation usable");
	}
	return failed;
}

static int httpRecovery(int durationMs)
{
	QFile identityFile("tests/fixtures/k4-identity.json"), controlFile("tests/fixtures/d2-control.json");
	if (!identityFile.open(QIODevice::ReadOnly) || !controlFile.open(QIODevice::ReadOnly)) return 2;
	const auto opened = QJsonDocument::fromJson(identityFile.readAll()).object()["fixtures"].toArray()[0].toObject()["value"].toObject()["data"].toObject();
	const auto broadcast = opened["broadcast"].toObject(), binding = broadcast["binding"].toObject();
	auto control = QJsonDocument::fromJson(controlFile.readAll()).array()[0].toObject()["value"].toObject();
	for (const auto *key : {"recordingId", "captureId", "epoch"}) control[key] = key == QString("recordingId") ? broadcast[key] : binding[key];
	QTcpServer server;
	if (!server.listen(QHostAddress::LocalHost, 0)) return 2;
	bool outage = false;
	int failures = 0, requests = 0, failedRequests = 0;
	QObject::connect(&server, &QTcpServer::newConnection, [&] {
		auto *socket = server.nextPendingConnection();
		QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
			const auto request = socket->readAll(); ++requests;
			failures += !request.startsWith("GET /admin/broadcasts/") || !request.contains("controlVersion=d2-2026-10-23.rc1") ||
				!request.contains(binding["captureId"].toString().toUtf8());
			const auto body = outage ? QByteArray("{\"error\":{\"code\":\"temporarily_unavailable\"}}") :
				QJsonDocument(QJsonObject{{"data",control},{"meta",QJsonObject{}},{"error",QJsonValue::Null}}).toJson(QJsonDocument::Compact);
			socket->write(QByteArray("HTTP/1.1 ") + (outage ? "503 Unavailable" : "200 OK") + "\r\nContent-Type: application/json\r\nContent-Length: " +
				QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
			socket->disconnectFromHost();
		});
	});
	QTemporaryDir root;
	hhc::ApiClient client(QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort())), [](bool){ return QByteArray{}; });
	const auto id = opened["outputSessionId"].toString();
	hhc::BroadcastControl state(root.path(), binding["actorId"].toString(), id, client, true);
	state.adoptIngest(opened, {{"protocolVersion","k4-2026-10-15.rc1"},{"outputSessionId",id},{"recoveryVersion","common-watermark-v1"}});
	state.poll({},true);
	const auto before = state.recoveryControl();
	outage = true;
	QElapsedTimer elapsed; elapsed.start();
	while (elapsed.elapsed() < durationMs) {
		try { state.poll({},true); ++failures; }
		catch (const hhc::RequestError &e) { unsigned retries = 0; failures += e.status != 503 || !e.retryDelayMs(retries); ++failedRequests; }
		QThread::msleep(250);
	}
	failures += state.recoveryControl() != before || state.serverEnd().has_value();
	outage = false;
	auto recovery = control["recovery"].toObject(); recovery["state"] = "healthy"; control["recovery"] = recovery;
	state.poll({},true);
	hhc::BroadcastControl restarted(root.path(), binding["actorId"].toString(), id, client, true);
	failures += restarted.recoveryControl()["recovery"].toObject()["state"] != "healthy" || restarted.serverEnd().has_value();
	std::cout << "D2 native control+HTTP elapsedMs=" << elapsed.elapsed() << " failedRequests=" << failedRequests
		<< " requests=" << requests << " sameOutput=true encoder=false controllerRetryLoop=false\n";
	return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	if (app.arguments().contains("--prepared-only")) return preparedAfterPoll();
	if (app.arguments().contains("--http-outage-ms")) {
		bool ok = false;
		const int duration = app.arguments().value(app.arguments().indexOf("--http-outage-ms")+1).toInt(&ok);
		if (!ok || (duration != 1000 && (duration < 90000 || duration > 120000))) return 2;
		try { return httpRecovery(duration); } catch (const hhc::RequestError &e) { std::cerr << "FAIL D2 HTTP " << qPrintable(e.code) << '\n'; return 1; }
	}
	int failed = 0;
	failed += preparedAfterPoll();
	auto check = [&](bool ok, const char *what) { if (!ok) { ++failed; std::cerr << "FAIL " << what << '\n'; } };
	QFile f("tests/fixtures/d2-control.json");
	if (!f.open(QIODevice::ReadOnly)) return 2;
	QJsonObject fixtures;
	for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
		const auto c = v.toObject();
		check(hhc::validWire(c["schema"].toString(), c["value"]) == c["valid"].toBool(), qPrintable(c["name"].toString()));
		fixtures[c["name"].toString()] = c["value"];
	}
	const auto interrupted = fixtures["interrupted"].toObject();
	const QJsonObject identity{{"recordingId", interrupted["recordingId"]}, {"captureId", interrupted["captureId"]}, {"epoch", 1}};
	auto observe = [&](const QJsonObject &before, const QJsonObject &next) { return hhc::acceptD2Observation(before, next, identity, 4, 4); };
	check(observe({}, interrupted) == interrupted, "initial identity-checked observation");
	check(hhc::acceptD2Observation({}, interrupted, identity, 3, 4).isEmpty(), "old generation discarded before accepting response");
	auto healthy = interrupted;
	auto recovery = healthy["recovery"].toObject(); recovery["state"] = "healthy";
	healthy["recovery"] = recovery;
	check(observe(interrupted, healthy)["recovery"].toObject()["state"] == "healthy", "equal revision processes recovery state");
	auto reject = [&](QJsonObject c) {
		try { observe(interrupted, c); return false; } catch (const hhc::RequestError &) { return true; }
	};
	auto wrong = interrupted; wrong["epoch"] = 2;
	check(reject(wrong), "epoch mismatch never rebinds");
	wrong = interrupted; wrong["captureId"] = QString(32, 'a');
	check(reject(wrong), "capture mismatch rejected");
	wrong = interrupted; wrong["recordingId"] = "44444444-4444-4444-8444-444444444444";
	check(reject(wrong), "recording mismatch rejected");
	try { observe(interrupted, wrong); check(false, "wrong identity must fail closed"); }
	catch (const hhc::RequestError &e) { check(e.code == "malformed_success", "semantic identity mismatch reaches authority-deny fence"); }
	wrong = interrupted; wrong["phase"] = "preview";
	check(reject(wrong), "equal revision cannot rewrite D1 fields");
	wrong = interrupted; wrong["revision"] = 41;
	check(observe(interrupted, wrong).isEmpty(), "older revision discarded");
	const auto ended = fixtures["idle_end"].toObject();
	check(!observe({}, ended)["publicEnd"].isNull(), "public end observed as metadata without issuing native stop");
	auto rollback = ended;
	rollback["revision"] = 43;
	rollback["recovery"] = fixtures["legacy_unsupported"].toObject()["recovery"];
	rollback["publicEnd"] = QJsonValue::Null;
	const auto accepted = observe(ended, rollback);
	check(accepted["publicEnd"] == ended["publicEnd"] && accepted["recovery"].toObject()["version"].isNull(),
	      "capability loss accepted without clearing committed public end");
	wrong = ended;
	auto bounds = wrong["boundaries"].toObject(); bounds["endSequenceExclusive"] = 22; bounds["revision"] = 3;
	wrong["revision"] = 43; wrong["boundaries"] = bounds;
	auto publicEnd = wrong["publicEnd"].toObject(); publicEnd["endSequenceExclusive"] = 22; wrong["publicEnd"] = publicEnd;
	bool changedCutoffRejected = false;
	try { observe(ended, wrong); } catch (const hhc::RequestError &) { changedCutoffRejected = true; }
	check(changedCutoffRejected, "committed cutoff cannot expand");
	check(fixtures["safe_integer_maximum"].toObject()["recovery"].toObject()["commonConfirmedEndSequenceExclusive"].toInteger() == 9007199254740991LL,
	      "safe integer remains qint64, not native int");
	check(!hhc::validWire("BroadcastControlD1", interrupted), "D2 fields cannot enter frozen D1 decoder");
	for (const auto &e : {hhc::RequestError(400, "request_rejected"), hhc::RequestError(404, "capture_not_found"),
		hhc::RequestError(500, "unavailable"), hhc::RequestError(422, "invalid_input")})
		check(!hhc::d2FallbackAllowed(e), "only exact unsupported control response allows fallback");
	check(hhc::d2FallbackAllowed(hhc::RequestError(422, "broadcast_protocol_unsupported")), "explicit unsupported control response permits D1");
	auto unknown = hhc::RequestError(422, "broadcast_protocol_unsupported"); unknown.outputReplayUnknown = true;
	check(!hhc::d2FallbackAllowed(unknown), "unknown authority cannot grant fallback");
	QJsonObject open{{"protocolVersion", "k4-2026-10-15.rc1"}, {"outputSessionId", "33333333-3333-4333-8333-333333333333"},
		{"streamKey", "hhc_stream_" + QString(43, 'A')}, {"mediaProfile", "obs-hls-10s-v1"}, {"recoveryVersion", "common-watermark-v1"}};
	check(hhc::validWire("BroadcastOutputOpenD2Input", open), "D2 open offer has no invented operation key");
	check(!hhc::validWire("BroadcastOutputOpenInput", open), "old K4 schema unchanged");
	open["recoveryVersion"] = "future";
	check(!hhc::validWire("BroadcastOutputOpenD2Input", open), "unknown open recovery version rejected");
	open["recoveryVersion"] = "";
	check(hhc::validWire("BroadcastOutputOpenD2Input", open), "fixed producer schema accepts explicit empty legacy preference");
	open.remove("recoveryVersion");
	check(hhc::validWire("BroadcastOutputOpenD2Input", open) && hhc::validWire("BroadcastOutputOpenInput", open), "omitted offer remains legacy");
	return failed ? 1 : 0;
}
