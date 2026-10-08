#include "native-auth.hpp"
#include "capture-sync.hpp"
#include "session-store.hpp"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QLockFile>
#include "windows-path.hpp"
#include <QThread>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QCoreApplication::addLibraryPath(QDir(QCoreApplication::applicationDirPath())
						 .absoluteFilePath("../../data/obs-plugins/hhc-obs-plugin/qt-plugins"));
	QCommandLineParser options;
	options.addOption(QCommandLineOption(QStringList{"root"}, "Native settings root", "path"));
	options.addOption(QCommandLineOption(QStringList{"account"}, "Expected human account", "UUID"));
	options.addOption(QCommandLineOption(QStringList{"auth-check"}, "Developer auth evidence without secrets"));
	options.addOption(QCommandLineOption(QStringList{"build-info"}, "Compiled source revision"));
	options.process(app);
	if (options.isSet("build-info")) {
		std::cout << HHC_SOURCE_COMMIT << '\n';
		return 0;
	}
	auto root = options.value("root"), expected = options.value("account");
	if (root.isEmpty())
		return 2;
	try {
		hhc::NativeAuth auth(root, nullptr, expected);
		auto cutoff = std::chrono::steady_clock::now() + std::chrono::hours(24);
		unsigned authFailures = 0;
		for (;;) {
			try {
				auth.bearer();
				break;
			} catch (const hhc::RequestError &e) {
				auto delay = e.retryDelay(authFailures);
				if (!delay || std::chrono::steady_clock::now() >= cutoff)
					throw;
				QThread::msleep(1000 * *delay + QRandomGenerator::global()->bounded(1000));
			}
		}
		auto account = auth.account();
		if (options.isSet("auth-check")) {
			std::cout << "OAuth refresh OK; read=" << auth.permitted("cms:recordings:read")
				  << " write=" << auth.permitted("cms:recordings:write")
				  << " publish=" << auth.permitted("cms:recordings:publish") << '\n';
			return 0;
		}
		if (expected != account)
			return 3;
		auto identity = QString::fromLatin1(
			QCryptographicHash::hash(account.toUtf8(), QCryptographicHash::Sha256).toHex());
		QLockFile lock(hhc::lockFilePath(root + "/queue/" + identity + "/background.lock"));
		lock.setStaleLockTime(0);
		if (!lock.tryLock(0))
			return 0;
		auto accountDirectory = root + "/queue/" + identity;
		auto completedIds = [&] {
			QStringList ids;
			for (const auto &id : QDir(accountDirectory).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
				QFile j(accountDirectory + "/" + id + "/journal.json");
				if (j.size() > 8 * 1024 * 1024 || !j.open(QIODevice::ReadOnly))
					continue;
				auto meta = QJsonDocument::fromJson(j.readAll()).object();
				if (meta["normalEnd"].toBool() && meta["account"] == account)
					ids << id;
			}
			return ids;
		};
		auto known = completedIds();
		unsigned attempts = 0;
		auto report = hhc::SessionStore(root + "/queue").scanPending(account);
		while (std::chrono::steady_clock::now() < cutoff) {
			try {
				auto current = completedIds();
				if (current != known) {
					report = hhc::SessionStore(root + "/queue").scanPending(account);
					known = current;
				}
				bool pending = false, terminalFailed = false;
				for (const auto &j : report.sessions) {
					auto dir =
						hhc::SessionStore(root + "/queue").mediaDirectory(account, j.localId);
					if (!j.normalEnd || !QFileInfo::exists(dir + "/remote-journal.json"))
						continue;
					hhc::ApiClient api(QUrl("https://admin.alive.org.tw/api"), [&](bool force) {
						auto bearer = auth.bearer(force);
						if (auth.account() != expected)
							throw hhc::RequestError(403, "account_mismatch");
						return bearer;
					});
					hhc::CaptureSync sync(root + "/queue", account, j.localId, api);
					auto state = sync.step(false);
					terminalFailed |= state.state == "failed" || state.state == "expired";
					pending |= state.state != "ready" && state.state != "aborted" &&
							   state.state != "expired" && state.state != "failed" ||
						   state.state == "ready" && state.autoPublish == "pending";
					QJsonObject status{{"localId", j.localId},
							   {"recordingId", state.recordingId},
							   {"captureId", state.captureId},
							   {"state", state.state},
							   {"autoPublish", state.autoPublish},
							   {"observedAt", QDateTime::currentDateTimeUtc().toString(
										  Qt::ISODateWithMs)}};
					auto bytes = QJsonDocument(status).toJson(QJsonDocument::Compact);
					QSaveFile f(root + "/background-status.json");
					if (f.open(QIODevice::WriteOnly)) {
						f.write(bytes);
						f.commit();
					}
				}
				if (!pending)
					return !terminalFailed && report.issues.empty() ? 0 : 4;
				attempts = 0;
				QThread::sleep(5);
			} catch (const hhc::RequestError &e) {
				auto delay = e.retryDelay(attempts);
				if (!delay)
					throw;
				QThread::msleep(1000 * *delay + QRandomGenerator::global()->bounded(1000));
			}
		}
		return 5;
	} catch (const hhc::RequestError &e) {
		std::cerr << e.what() << '\n';
		return 6;
	} catch (const std::exception &e) {
		std::cerr << e.what() << "; account-bound media retained\n";
		return 7;
	} catch (...) {
		std::cerr << "Background synchronization failed; account-bound media retained\n";
		return 7;
	}
}
