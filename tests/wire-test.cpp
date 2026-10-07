#include "wire.hpp"
#include "auth.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int failures = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) {
			std::cerr << "FAIL " << name << '\n';
			++failures;
		}
	};
	QFile f("tests/fixtures/c1.json");
	check(f.open(QIODevice::ReadOnly), "read pinned fixtures");
	for (const auto &v : QJsonDocument::fromJson(f.readAll()).array()) {
		const auto o = v.toObject();
		if (o["schema"].toString().contains("Capture") && !o["schema"].toString().contains("Error"))
			check(hhc::validWire(o["schema"].toString(), o["value"]) == o["valid"].toBool(),
			      qPrintable(o["name"].toString()));
	}
	auto fixtures = QJsonDocument::fromJson(QByteArray("[]"));
	QJsonObject token{{"access_token", "synthetic"},
			  {"refresh_token", "synthetic-refresh"},
			  {"token_type", "Bearer"},
			  {"expires_in", 300},
			  {"scope", ""},
			  {"principal", QJsonObject{{"type", "human"},
						    {"id", "018f0c1f-18d0-7e81-9f6f-69c456db7003"},
						    {"client_id", "hhc-obs"},
						    {"credential_expires_at", "2026-10-09T00:00:00Z"}}}};
	check(hhc::validWire("OAuthTokenResponse", token), "empty granted scope is valid and must not be inferred");
	token.remove("scope");
	check(!hhc::validWire("OAuthTokenResponse", token), "missing scope rejected");
	hhc::OAuthAttempt attempt(32123);
	check(attempt.redirect().path() == "/oauth/callback", "fixed C1 loopback path");
	QJsonObject inventory{
		{"schemaVersion", 1},
		{"presetVersion", "hhc-obs-v1"},
		{"objects",
		 QJsonArray{QJsonObject{{"path", "720p/init.mp4"}, {"sizeBytes", 123}, {"sha256", QString(64, 'a')}}}},
		{"renditions", QJsonArray{QJsonObject{{"name", "720p"},
						      {"width", 1280},
						      {"height", 720},
						      {"frameRate", 30000.0 / 1001},
						      {"videoBitrate", 1500000},
						      {"audioBitrate", 128000},
						      {"durationSeconds", 61.027633},
						      {"segmentCount", 3}}}},
		{"inventoryDigest", "omitted"}};
	const QByteArray expected =
		"{\"schemaVersion\":1,\"presetVersion\":\"hhc-obs-v1\",\"objects\":[{\"path\":\"720p/init.mp4\",\"sizeBytes\":123,\"sha256\":\"" +
		QByteArray(64, 'a') +
		"\"}],\"renditions\":[{\"name\":\"720p\",\"width\":1280,\"height\":720,\"frameRate\":29.97002997002997,\"videoBitrate\":1500000,\"audioBitrate\":128000,\"durationSeconds\":61.027633,\"segmentCount\":3}]}";
	if (hhc::canonicalInventory(inventory) != expected)
		std::cerr << hhc::canonicalInventory(inventory).constData() << "\n";
	check(hhc::canonicalInventory(inventory) == expected,
	      "Go-compatible order and shortest double digest serialization");
	check(hhc::measuredBandwidth({30, 30, 1}, {3000000, 6000000, 900000}, 30) == 1780646,
	      "RFC8216 eligible contiguous peak excludes tail-only window");
	check(hhc::measuredBandwidth({1}, {100000}, 1) == 800000, "short-only recording has measured bandwidth");
	check(hhc::initCodecTag(QByteArray::fromHex("0164002aff"), QByteArray::fromHex("1190")) ==
		      "avc1.64002a,mp4a.40.2",
	      "master codecs derive from actual init AVCC and AAC LC");
	bool badCodec = false;
	try {
		hhc::initCodecTag("bad", QByteArray::fromHex("1190"));
	} catch (...) {
		badCodec = true;
	}
	check(badCodec, "malformed init codec declaration is rejected");
	return failures ? 1 : 0;
}
