#pragma once
#include "auth.hpp"
#include <QObject>
#include <QTcpServer>
#include <QMutex>
#include <QFutureWatcher>
#include <QJsonObject>
#include <QSet>
#include <functional>
#include <atomic>
namespace hhc {
struct TokenSet {
	QByteArray access, refresh;
	QString account;
	QSet<QString> scopes;
	int expiresIn = 0;
	QJsonObject principal;
};
TokenSet parseNativeToken(const QJsonObject &);
class NativeAuth : public QObject {
public:
	explicit NativeAuth(QString settingsRoot, QObject *parent = nullptr, QString expectedAccount = {});
	~NativeAuth() override;
	void login();
	void resume();
	void logout();
	void cancelPending()
	{
		cancelled_ = true;
		listener_.close();
	}
	QByteArray bearer(bool forceRefresh = false);
	QString account() const;
	bool permitted(const QString &) const;
	std::function<void(QString)> onChanged;

private:
	TokenSet exchange(QJsonObject form, const QString &expectedAccount = {});
	void accept(TokenSet);
	void launch(std::function<TokenSet()>);
	QString root_, deviceId_, expectedAccount_;
	std::atomic<bool> cancelled_{false};
	mutable QMutex mutex_;
	QMutex refreshMutex_;
	TokenSet token_;
	std::chrono::steady_clock::time_point expiry_{};
	QTcpServer listener_;
	std::unique_ptr<OAuthAttempt> attempt_;
	QFutureWatcher<TokenSet> future_;
	bool awaitingResult_ = false;
};
} // namespace hhc
