#include "broadcast-control.hpp"
#include "wire.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <iostream>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int failed = 0;
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
	open.remove("recoveryVersion");
	check(hhc::validWire("BroadcastOutputOpenD2Input", open) && hhc::validWire("BroadcastOutputOpenInput", open), "omitted offer remains legacy");
	return failed ? 1 : 0;
}
