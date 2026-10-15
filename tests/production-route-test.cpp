#include "http.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDir>
#include <QSslSocket>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	if (argc != 2)
		return 2;
	QCoreApplication::addLibraryPath(QString::fromLocal8Bit(argv[1]));
	try {
		auto d = hhc::httpRequest("GET", QUrl("https://account.alive.org.tw/.well-known/openid-configuration"));
		auto o = QJsonDocument::fromJson(d.body).object();
		if (d.status != 200 || o["issuer"] != "https://account.alive.org.tw" ||
		    o["token_endpoint"] != "https://account.alive.org.tw/api/account/v1/oauth/token")
			return 3;
		auto r = hhc::httpRequest("GET", QUrl("https://admin.alive.org.tw/api/admin/recordings"));
		std::cout << "Schannel backend=" << QSslSocket::activeBackend().toStdString()
			  << " discovery=" << d.status << " anonymous recordings=" << r.status
			  << "; READ ONLY, NOT AUTHENTICATED E2E\n";
		return r.status == 401 ? 0 : 4;
	} catch (const hhc::RequestError &e) {
		std::cerr << e.what() << '\n';
		return 5;
	}
}
