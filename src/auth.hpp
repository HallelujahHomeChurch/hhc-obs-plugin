#pragma once
#include <QByteArray>
#include <QUrl>
#include <QString>
#include <optional>
#include <chrono>
namespace hhc {
QByteArray pkceChallenge(const QByteArray &verifier);
class OAuthAttempt {
public:
	explicit OAuthAttempt(quint16 port);
	QByteArray state() const { return state_; }
	QByteArray verifier() const { return verifier_; }
	QUrl redirect() const { return redirect_; }
	std::optional<QString> consumeCallback(const QUrl &);

private:
	QByteArray state_, verifier_;
	QUrl redirect_;
	bool used_ = false;
	std::chrono::steady_clock::time_point expires_;
};
// Tokens are opaque here. Frozen C1 supplies token parsing and issuer/client identity.
class CredentialVault {
public:
	static void save(const QString &issuer, const QString &account, const QByteArray &secret);
	static std::optional<QByteArray> load(const QString &issuer, const QString &account);
	static void erase(const QString &issuer, const QString &account);
};
} // namespace hhc
