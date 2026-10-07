#include "async.hpp"
#include "native-auth.hpp"
#include "http.hpp"
#include "wire.hpp"
#include <QJsonDocument>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QSaveFile>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QTimer>
#include <QThread>
#include <QLockFile>
#include <QtConcurrent/QtConcurrentRun>
namespace hhc {
namespace {
const QString issuer = "https://account.alive.org.tw";
const QString tokenEndpoint = issuer + "/api/account/v1/oauth/token";
void require(bool ok, const char *text)
{
	if (!ok)
		throw std::runtime_error(text);
}
} // namespace
TokenSet parseNativeToken(const QJsonObject &j)
{
	require(validWire("OAuthTokenResponse", j), "Malformed OAuth success");
	auto p = j["principal"].toObject();
	require(p["type"] == "human" && p["client_id"] == "hhc-obs", "Native human principal required");
	require(!j["access_token"].toString().isEmpty() && !j["refresh_token"].toString().isEmpty(),
		"Native credentials missing");
	require(j["access_token"].toString().size() < 16384 && j["refresh_token"].toString().size() < 1024,
		"Native credential limit");
	TokenSet t;
	t.access = j["access_token"].toString().toUtf8();
	t.refresh = j["refresh_token"].toString().toUtf8();
	t.principal = p;
	t.account = p["id"].toString();
	t.expiresIn = j["expires_in"].toInt();
	for (const auto &scope : j["scope"].toString().split(' ', Qt::SkipEmptyParts))
		t.scopes.insert(scope);
	return t;
}
NativeAuth::NativeAuth(QString root, QObject *parent, QString expectedAccount)
	: QObject(parent),
	  root_(std::move(root)),
	  expectedAccount_(std::move(expectedAccount))
{
	require(QDir().mkpath(root_), "Cannot create native settings");
	QFile f(root_ + "/device-id");
	if (f.open(QIODevice::ReadOnly)) {
		require(f.size() < 128, "Invalid device identifier");
		deviceId_ = QString::fromUtf8(f.readAll()).trimmed();
	}
	if (deviceId_.isEmpty()) {
		deviceId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		QSaveFile file(root_ + "/device-id");
		auto b = deviceId_.toUtf8();
		require(file.open(QIODevice::WriteOnly) && file.write(b) == b.size() && file.commit(),
			"Cannot persist device identifier");
	}
	require(!QUuid(deviceId_).isNull(), "Invalid device identifier");
	connect(&future_, &QFutureWatcher<TokenSet>::finished, this, [this] {
		try {
			accept(workerResult(future_.future()));
			if (onChanged)
				onChanged({});
		} catch (const std::exception &e) {
			if (onChanged)
				onChanged(QString::fromUtf8(e.what()));
		} catch (...) {
			if (onChanged)
				onChanged("OAuth operation failed");
		}
	});
	connect(&listener_, &QTcpServer::newConnection, this, [this] {
		while (listener_.hasPendingConnections()) {
			auto *socket = listener_.nextPendingConnection();
			auto bytes = std::make_shared<QByteArray>();
			auto consumed = std::make_shared<bool>(false);
			QTimer::singleShot(5000, socket, &QTcpSocket::abort);
			connect(socket, &QTcpSocket::readyRead, this, [this, socket, bytes, consumed] {
				if (*consumed)
					return;
				*bytes += socket->read(8193 - bytes->size());
				if (bytes->size() > 8192) {
					*consumed = true;
					socket->abort();
					return;
				}
				if (!bytes->contains("\r\n\r\n"))
					return;
				*consumed = true;
				const auto line = bytes->left(bytes->indexOf("\r\n")).split(' ');
				std::optional<QString> code;
				if (attempt_ && line.size() == 3 && line[0] == "GET" &&
				    line[1].startsWith("/oauth/callback?") && !line[1].contains('#') &&
				    socket->peerAddress() == QHostAddress::LocalHost) {
					QUrl url = attempt_->redirect();
					url.setQuery(QUrl::fromEncoded(line[1]).query(QUrl::FullyEncoded));
					code = attempt_->consumeCallback(url);
				}
				const QByteArray content = code ? "HHC OBS sign-in received. You may close this tab."
								: "Invalid or expired HHC OBS callback.";
				socket->write(
					"HTTP/1.1 " + QByteArray(code ? "200 OK" : "400 Bad Request") +
					"\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: " +
					QByteArray::number(content.size()) + "\r\nConnection: close\r\n\r\n" + content);
				socket->disconnectFromHost();
				if (code) {
					QJsonObject form{{"grant_type", "authorization_code"},
							 {"code", *code},
							 {"code_verifier", QString::fromLatin1(attempt_->verifier())},
							 {"redirect_uri",
							  attempt_->redirect().toString(QUrl::FullyEncoded)},
							 {"device_name", "HHC OBS"}};
					listener_.close();
					attempt_.reset();
					launch([this, form] {
						QMutexLocker own(&refreshMutex_);
						QLockFile vaultLock(root_ + "/credentials.lock");
						vaultLock.setStaleLockTime(0);
						for (int i = 0; !vaultLock.tryLock(0); ++i) {
							if (i == 50)
								throw RequestError(409, "local_credentials_busy");
							cancellableWait(100);
						}
						return exchange(form);
					});
				}
			});
			connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		}
	});
}
NativeAuth::~NativeAuth()
{
	cancelled_ = true;
	listener_.close();
	try {
		future_.waitForFinished();
	} catch (...) {
	}
}
void NativeAuth::launch(std::function<TokenSet()> function)
{
	if (future_.isRunning())
		return;
	future_.setFuture(QtConcurrent::run([this, function = std::move(function)] {
		HttpCancellationScope cancellation(cancelled_);
		return function();
	}));
}
void NativeAuth::login()
{
	if (future_.isRunning())
		return;
	listener_.close();
	listener_.setMaxPendingConnections(8);
	require(listener_.listen(QHostAddress::LocalHost, 0), "Loopback listener unavailable");
	attempt_ = std::make_unique<OAuthAttempt>(listener_.serverPort());
	QUrl url(issuer + "/api/account/v1/oauth/authorize");
	QUrlQuery query;
	query.addQueryItem("response_type", "code");
	query.addQueryItem("client_id", "hhc-obs");
	query.addQueryItem("redirect_uri", attempt_->redirect().toString());
	query.addQueryItem(
		"scope",
		"openid profile offline_access cms:recordings:read cms:recordings:write cms:recordings:publish");
	query.addQueryItem("state", QString::fromLatin1(attempt_->state()));
	query.addQueryItem("code_challenge", QString::fromLatin1(pkceChallenge(attempt_->verifier())));
	query.addQueryItem("code_challenge_method", "S256");
	url.setQuery(query);
	if (!QDesktopServices::openUrl(url)) {
		listener_.close();
		attempt_.reset();
		throw std::runtime_error("System browser unavailable");
	}
	QTimer::singleShot(300000, this, [this, state = attempt_->state()] {
		if (attempt_ && attempt_->state() == state) {
			listener_.close();
			attempt_.reset();
			if (onChanged)
				onChanged("Sign-in callback expired; sign in again");
		}
	});
}
TokenSet NativeAuth::exchange(QJsonObject form, const QString &expectedAccount)
{
	form["client_id"] = "hhc-obs";
	form["device_id"] = deviceId_;
	HttpResponse response;
	for (int attempt = 0; attempt < 3; ++attempt) {
		response = httpRequest("POST", QUrl(tokenEndpoint), QJsonDocument(form).toJson(QJsonDocument::Compact),
				       {{"Content-Type", "application/json"}, {"Accept", "application/json"}});
		auto code = QJsonDocument::fromJson(response.body).object()["error"].toString();
		if (attempt == 2 || !(response.status == 409 && code == "temporarily_unavailable" ||
				      response.status == 503 && code == "server_error"))
			break;
		cancellableWait(1000 * std::max(response.retryAfter, 1 << attempt));
	}
	if (response.status != 200) {
		auto code = QJsonDocument::fromJson(response.body).object()["error"].toString();
		if (code == "invalid_grant")
			throw RequestError(response.status, "sign_in_required");
		throw RequestError(response.status, "oauth_unavailable", response.retryAfter);
	}
	auto token = parseNativeToken(QJsonDocument::fromJson(response.body).object());
	require(expectedAccount.isEmpty() || token.account == expectedAccount,
		"Refresh principal changed; queue remains account-bound");
	QJsonObject stored{{"refresh_token", QString::fromUtf8(token.refresh)},
			   {"principal", token.principal},
			   {"device_id", deviceId_}};
	CredentialVault::save(issuer, "native:" + deviceId_ + ":" + token.account,
			      QJsonDocument(stored).toJson(QJsonDocument::Compact));
	if (expectedAccount.isEmpty()) {
		QSaveFile selected(root_ + "/active-account");
		auto bytes = token.account.toUtf8();
		require(selected.open(QIODevice::WriteOnly) && selected.write(bytes) == bytes.size() &&
				selected.commit(),
			"Cannot save active account");
	}
	return token;
}
void NativeAuth::accept(TokenSet t)
{
	QMutexLocker lock(&mutex_);
	token_ = std::move(t);
	expiry_ = std::chrono::steady_clock::now() + std::chrono::seconds(token_.expiresIn);
}
void NativeAuth::resume()
{
	launch([this] {
		bearer(true);
		QMutexLocker lock(&mutex_);
		return token_;
	});
}
QByteArray NativeAuth::bearer(bool force)
{
	QMutexLocker refreshLock(&refreshMutex_);
	QByteArray refresh;
	QString account;
	{
		QMutexLocker lock(&mutex_);
		if (!force && !token_.access.isEmpty() &&
		    std::chrono::steady_clock::now() + std::chrono::seconds(30) < expiry_)
			return token_.access;
		refresh = token_.refresh;
		account = token_.account;
	}
	QLockFile vaultLock(root_ + "/credentials.lock");
	vaultLock.setStaleLockTime(0);
	for (int i = 0; !vaultLock.tryLock(0); ++i) {
		if (i == 50)
			throw RequestError(409, "local_credentials_busy");
		cancellableWait(100);
	}
	if (account.isEmpty())
		account = expectedAccount_;
	if (account.isEmpty()) {
		QFile selected(root_ + "/active-account");
		if (selected.open(QIODevice::ReadOnly) && selected.size() < 128)
			account = QString::fromUtf8(selected.readAll()).trimmed();
	}
	bool missingSelection = false;
	if (account.isEmpty()) {
		auto legacy = CredentialVault::load(issuer, "native-active");
		if (legacy) {
			auto j = QJsonDocument::fromJson(*legacy).object();
			account = j["principal"].toObject()["id"].toString();
			missingSelection = true;
		}
	}
	if (account.isEmpty())
		throw RequestError(401, "sign_in_required");
	require(!QUuid(account).isNull(), "Stored account invalid");
	auto stored = CredentialVault::load(issuer, "native:" + deviceId_ + ":" + account);
	QString migratedTarget;
	if (!stored) {
		migratedTarget = "native:" + account;
		stored = CredentialVault::load(issuer, migratedTarget);
		if (!stored) {
			migratedTarget = "native-active";
			stored = CredentialVault::load(issuer, migratedTarget);
		}
	}
	if (!stored)
		throw RequestError(401, "sign_in_required");
	auto j = QJsonDocument::fromJson(*stored).object();
	require(j["principal"].toObject()["id"] == account, "Stored principal differs");
	require(!j.contains("device_id") || j["device_id"] == deviceId_, "Stored device differs");
	refresh = j["refresh_token"].toString().toUtf8();
	require(!refresh.isEmpty(), "Stored native credential invalid");
	auto fresh =
		exchange({{"grant_type", "refresh_token"}, {"refresh_token", QString::fromUtf8(refresh)}}, account);
	require(fresh.account == account, "Refresh principal changed; queue remains account-bound");
	// Delete a legacy entry only after the issuer proves this device owns its refresh chain.
	if (!migratedTarget.isEmpty())
		CredentialVault::erase(issuer, migratedTarget);
	if (missingSelection && expectedAccount_.isEmpty()) {
		QSaveFile selected(root_ + "/active-account");
		auto bytes = account.toUtf8();
		require(selected.open(QIODevice::WriteOnly) && selected.write(bytes) == bytes.size() &&
				selected.commit(),
			"Cannot migrate account selection");
	}

	QMutexLocker lock(&mutex_);
	token_ = std::move(fresh);
	expiry_ = std::chrono::steady_clock::now() + std::chrono::seconds(token_.expiresIn);
	return token_.access;
}
void NativeAuth::logout()
{
	if (future_.isRunning())
		return;
	listener_.close();
	attempt_.reset();
	{
		QMutexLocker lock(&mutex_);
		CredentialVault::erase(issuer, "native:" + deviceId_ + ":" + token_.account);
		token_ = {};
	}
	if (onChanged)
		onChanged({});
}
QString NativeAuth::account() const
{
	QMutexLocker lock(&mutex_);
	return token_.account;
}
bool NativeAuth::permitted(const QString &scope) const
{
	QMutexLocker lock(&mutex_);
	return token_.scopes.contains(scope);
}
} // namespace hhc
