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
	for (const auto &v : read("tests/fixtures/k4.json")["fixtures"].toArray()) {
		auto f = v.toObject();
		check(validWire(f["schema"].toString(), f["value"]) == f["valid"].toBool(),
		      qPrintable(f["name"].toString()));
	}
	const auto fixtures = read("tests/fixtures/k4-identity.json")["fixtures"].toArray();
	auto response = fixtures[0].toObject()["value"].toObject();
	const auto id = response["data"].toObject()["outputSessionId"].toString();
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
	for (bool endedBeforeResponse : {false, true}) {
		QTemporaryDir transfer;
		secrets.remove("session-" + id);
		auto data = response["data"].toObject();
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
		KeyIngest(transfer.path() + "/intents", id, vault, pipeline).prepareOpen();
		bool validated = false;
		try {
			keyTransferStep(transfer.path(), id, false, false, true, validated, nullptr, vault, pipeline);
		} catch (const RequestError &) {
		}
		if (!endedBeforeResponse)
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
	return failures ? 1 : 0;
}
