#include "native-auth.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QUuid>
#include <QLockFile>
#include "windows-path.hpp"
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int failures = 0;
	auto check = [&](bool ok, const char *n) {
		if (!ok) {
			std::cerr << "FAIL " << n << '\n';
			++failures;
		}
	};
	QJsonObject j{{"access_token", "synthetic"},
		      {"refresh_token", "synthetic-refresh"},
		      {"token_type", "Bearer"},
		      {"expires_in", 300},
		      {"scope", ""},
		      {"principal", QJsonObject{{"type", "human"},
						{"id", "018f0c1f-18d0-7e81-9f6f-69c456db7003"},
						{"client_id", "hhc-obs"},
						{"credential_expires_at", "2099-10-09T00:00:00Z"}}}};
	auto token = hhc::parseNativeToken(j);
	check(token.account == "018f0c1f-18d0-7e81-9f6f-69c456db7003" && token.scopes.empty() &&
		      token.refresh == "synthetic-refresh",
	      "issuer principal binds queue and empty scope stays empty");
	auto rejects = [&](QJsonObject input) {
		try {
			hhc::parseNativeToken(input);
			return false;
		} catch (...) {
			return true;
		}
	};
	auto p = j["principal"].toObject();
	p["type"] = "service";
	j["principal"] = p;
	check(rejects(j), "initial capture rejects service principal");
	p["type"] = "human";
	p["client_id"] = "wrong";
	j["principal"] = p;
	check(rejects(j), "wrong client rejected");
	p["client_id"] = "hhc-obs";
	j["principal"] = p;
	j.remove("scope");
	check(rejects(j), "missing granted scope rejected");
	QTemporaryDir root;
	const auto write = [&](QString name, QByteArray bytes) {
		QFile f(root.path() + "/" + name);
		return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
	};
	const QString device = QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString selected = QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString other = QUuid::createUuid().toString(QUuid::WithoutBraces);
	const QString issuer = "https://account.alive.org.tw";
	const QString target = "native:" + device + ":" + selected;
	const QString otherTarget = "native:" + device + ":" + other;
	check(root.isValid() && write("device-id", device.toUtf8()) && write("active-account", selected.toUtf8()) &&
		      write("retained-media", "synthetic-media"),
	      "isolated logout input");
	hhc::CredentialVault::save(issuer, target, "synthetic-selected");
	hhc::CredentialVault::save(issuer, otherTarget, "synthetic-other");
	{
		hhc::NativeAuth auth(root.path()); // No resume, browser or network request.
		{
			QLockFile held(hhc::lockFilePath(root.path() + "/credentials.lock"));
			check(held.tryLock(0), "isolated credential writer lock");
			bool rejected = false;
			try {
				auth.logout();
			} catch (...) {
				rejected = true;
			}
			check(rejected &&
				      hhc::CredentialVault::load(issuer, target) == QByteArray("synthetic-selected") &&
				      QFile::exists(root.path() + "/active-account"),
			      "credential lock refuses logout without losing selection or credential");
		}
		auth.logout();
		check(!hhc::CredentialVault::load(issuer, target),
		      "logout removes selected device credential before resume");
		QFile selection(root.path() + "/active-account");
		check(selection.open(QIODevice::ReadOnly) && selection.readAll().isEmpty(),
		      "explicit signed-out selection prevents legacy account migration after restart");
		check(hhc::CredentialVault::load(issuer, otherTarget) == QByteArray("synthetic-other"),
		      "logout preserves other account credentials");
		QFile media(root.path() + "/retained-media");
		check(media.open(QIODevice::ReadOnly) && media.readAll() == "synthetic-media",
		      "logout preserves media");
	}
	hhc::CredentialVault::erase(issuer, target);
	hhc::CredentialVault::erase(issuer, otherTarget);
	return failures ? 1 : 0;
}
