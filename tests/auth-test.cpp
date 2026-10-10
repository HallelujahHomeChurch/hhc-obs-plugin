#include "auth.hpp"
#include <QCoreApplication>
#include <QUrlQuery>
#include <QUuid>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int failures = 0;
	auto check = [&](bool ok, const char *s) {
		if (!ok) {
			std::cerr << "FAIL " << s << '\n';
			++failures;
		}
	};
	check(hhc::pkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk") ==
		      "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM",
	      "RFC7636 S256 vector");
	hhc::OAuthAttempt a(45123), b(45123);
	check(a.verifier().size() >= 43 && a.verifier() != b.verifier(), "fresh random PKCE verifier");
	check(a.state().size() >= 32 && a.state() != b.state(), "fresh random OAuth state");
	auto url = a.redirect();
	QUrlQuery q;
	q.addQueryItem("code", "synthetic-code");
	q.addQueryItem("state", QString::fromLatin1(a.state()));
	url.setQuery(q);
	auto wrong = url;
	wrong.setHost("evil.invalid");
	check(!a.consumeCallback(wrong), "reject nonloopback origin");
	wrong = url;
	wrong.setPort(45124);
	check(!a.consumeCallback(wrong), "reject different callback port");
	wrong = url;
	wrong.setPath("/other");
	check(!a.consumeCallback(wrong), "reject different callback path");
	wrong = url;
	QUrlQuery duplicate(q);
	duplicate.addQueryItem("state", "extra");
	wrong.setQuery(duplicate);
	check(!a.consumeCallback(wrong), "reject duplicate state");
	auto code = a.consumeCallback(url);
	check(code && *code == "synthetic-code", "valid callback consumes code");
	check(!a.consumeCallback(url), "callback state is one use");
	const QString issuer = "https://hhc-obs-test.invalid", account = QUuid::createUuid().toString();
	hhc::CredentialVault::save(issuer, account, "synthetic-test-token");
	auto secret = hhc::CredentialVault::load(issuer, account);
	check(secret && *secret == "synthetic-test-token", "Credential Manager roundtrip");
	check(!hhc::CredentialVault::load(issuer, account + "-other"), "Credential Manager account isolation");
	hhc::CredentialVault::erase(issuer, account);
	check(!hhc::CredentialVault::load(issuer, account), "delete only owned synthetic credential");
	return failures ? 1 : 0;
}
