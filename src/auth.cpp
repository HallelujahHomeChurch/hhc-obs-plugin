#include "auth.hpp"
#include <QCryptographicHash>
#include <QUrlQuery>
#include <windows.h>
#include <wincred.h>
#include <bcrypt.h>
#include <stdexcept>
namespace hhc {
namespace {
QByteArray randomSecret()
{
	QByteArray bytes(32, Qt::Uninitialized);
	if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(bytes.data()), 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
		throw std::runtime_error("Secure random generation failed");
	return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}
QString target(const QString &issuer, const QString &account)
{
	if (issuer.isEmpty() || account.isEmpty() || issuer.size() > 2048 || account.size() > 256)
		throw std::runtime_error("Invalid credential identity");
	return "HHC-OBS/" +
	       QString::fromLatin1(QCryptographicHash::hash(issuer.toUtf8() + QByteArray(1, '\0') + account.toUtf8(),
							    QCryptographicHash::Sha256)
					   .toHex());
}
} // namespace
QByteArray pkceChallenge(const QByteArray &v)
{
	return QCryptographicHash::hash(v, QCryptographicHash::Sha256)
		.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}
OAuthAttempt::OAuthAttempt(quint16 port)
	: state_(randomSecret()),
	  verifier_(randomSecret()),
	  expires_(std::chrono::steady_clock::now() + std::chrono::minutes(5))
{
	if (port == 0)
		throw std::runtime_error("Loopback listener port must be bound first");
	redirect_ = QUrl(QString("http://127.0.0.1:%1/oauth/callback").arg(port));
}
std::optional<QString> OAuthAttempt::consumeCallback(const QUrl &url)
{
	if (used_ || std::chrono::steady_clock::now() >= expires_ || !url.isValid() ||
	    url.scheme() != redirect_.scheme() || url.host() != redirect_.host() || url.port() != redirect_.port() ||
	    url.path() != redirect_.path() || !url.userInfo().isEmpty() || url.hasFragment())
		return {};
	const QUrlQuery query(url);
	int stateCount = 0, codeCount = 0;
	QString returned, code;
	for (const auto &item : query.queryItems(QUrl::FullyDecoded)) {
		if (item.first == "state") {
			++stateCount;
			returned = item.second;
		} else if (item.first == "code") {
			++codeCount;
			code = item.second;
		} else if (item.first == "error")
			return {};
	}
	if (stateCount != 1 || codeCount != 1 || code.isEmpty() || code.size() > 4096)
		return {};
	const auto bytes = returned.toLatin1();
	if (bytes.size() != state_.size())
		return {};
	unsigned mismatch = 0;
	for (qsizetype i = 0; i < bytes.size(); ++i)
		mismatch |= static_cast<unsigned char>(bytes[i]) ^ static_cast<unsigned char>(state_[i]);
	if (mismatch)
		return {};
	used_ = true;
	return code;
}
void CredentialVault::save(const QString &issuer, const QString &account, const QByteArray &secret)
{
	if (secret.isEmpty() || secret.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE)
		throw std::runtime_error("Credential size invalid");
	auto key = target(issuer, account).toStdWString();
	CREDENTIALW credential{};
	credential.Type = CRED_TYPE_GENERIC;
	credential.TargetName = key.data();
	credential.CredentialBlobSize = static_cast<DWORD>(secret.size());
	credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(secret.constData()));
	credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
	if (!CredWriteW(&credential, 0))
		throw std::runtime_error("Credential Manager write failed");
}
std::optional<QByteArray> CredentialVault::load(const QString &issuer, const QString &account)
{
	auto key = target(issuer, account).toStdWString();
	PCREDENTIALW credential = nullptr;
	if (!CredReadW(key.c_str(), CRED_TYPE_GENERIC, 0, &credential)) {
		if (GetLastError() == ERROR_NOT_FOUND)
			return {};
		throw std::runtime_error("Credential Manager read failed");
	}
	QByteArray secret(reinterpret_cast<const char *>(credential->CredentialBlob), credential->CredentialBlobSize);
	CredFree(credential);
	return secret;
}
void CredentialVault::erase(const QString &issuer, const QString &account)
{
	auto key = target(issuer, account).toStdWString();
	if (!CredDeleteW(key.c_str(), CRED_TYPE_GENERIC, 0) && GetLastError() != ERROR_NOT_FOUND)
		throw std::runtime_error("Credential Manager delete failed");
}
} // namespace hhc
