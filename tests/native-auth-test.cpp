#include "native-auth.hpp"
#include <QCoreApplication>
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
	return failures ? 1 : 0;
}
