#pragma once
#include "http.hpp"
#include "auth.hpp"
#include <QDateTime>
namespace hhc {
struct IngestCredentials {
	std::function<std::optional<QByteArray>(const QString &)> load;
	std::function<void(const QString &, const QByteArray &)> save;
};
QString ingestRoute(const QByteArray &method, const QString &path, const QJsonObject &session);
class KeyIngest {
public:
	KeyIngest(QString root, QString localId, IngestCredentials vault = {},
		  ApiClient::Transport transport = httpRequest);
	void saveKey(const QByteArray &);
	static void validateKey(const QByteArray &);
	bool hasKey() const;
	QByteArray key() const;
	void prepareOpen(const QString &mediaProfile = {});
	QJsonObject bind();
	QByteArray bearer(bool force = false);
	ApiClient api();
	QJsonObject metadata() const { return journal_; }

private:
	void save();
	void acceptSession(const QJsonObject &);
	QJsonObject open(bool revalidate = false);
	void checkOpened(const QJsonObject &) const;
	QJsonObject bootstrap(const QString &, QJsonObject, const QString &);
	QString root_, id_;
	QJsonObject journal_, session_;
	IngestCredentials vault_;
	ApiClient::Transport transport_;
};
} // namespace hhc
