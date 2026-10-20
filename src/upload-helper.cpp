#include "runtime-paths.hpp"
#include "native-auth.hpp"
#include "key-transfer.hpp"
#include "capture-sync.hpp"
#include "broadcast-control.hpp"
#include "session-store.hpp"
#include "helper-diagnostics.hpp"
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
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <iostream>
namespace {
QString diagnosticDirectory;
QMutex diagnosticMutex;
QtMessageHandler previousHandler = nullptr;
void helperMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
	if (message.startsWith("[HHC ") && !message.contains('\n') && !message.contains('\r')) {
		QMutexLocker guard(&diagnosticMutex);
		hhc::appendHelperDiagnostic(diagnosticDirectory, message);
	}
	if (previousHandler) previousHandler(type, context, message);
	else std::cerr << message.toUtf8().constData() << '\n';
}
}
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const auto tlsPath = hhc::helperTlsPath(QCoreApplication::applicationDirPath());
	if (!tlsPath.isEmpty())
		QCoreApplication::addLibraryPath(tlsPath);
	QCommandLineParser options;
	options.addOption(QCommandLineOption(QStringList{"root"}, "Native settings root", "path"));
	options.addOption(QCommandLineOption(QStringList{"account"}, "Expected human account", "UUID"));
	options.addOption(QCommandLineOption(QStringList{"auth-check"}, "Developer auth evidence without secrets"));
	options.addOption(QCommandLineOption(QStringList{"build-info"}, "Compiled source revision"));
	options.addOption(QCommandLineOption(QStringList{"k3"}, "Resume original K3 session", "local-id"));
	options.process(app);
	if (options.isSet("build-info")) {
#ifdef HHC_NETWORK_TEST_FAULTS
		std::cout << HHC_SOURCE_COMMIT << "-network-faults\n";
#else
		std::cout << HHC_SOURCE_COMMIT << '\n';
#endif
		return 0;
	}
	auto root = options.value("root"), expected = options.value("account");
	if (root.isEmpty())
		return 2;
	diagnosticDirectory = root + "/diagnostics";
	previousHandler = qInstallMessageHandler(helperMessage);
	if (options.isSet("k3")) {
		auto nextCleanup = std::chrono::steady_clock::now();
		unsigned failures = 0;
		bool validated = false;
		const auto cutoff = std::chrono::steady_clock::now() + std::chrono::hours(24);
		auto finish = [&](int code) {
			while (!hhc::SessionStore(root + "/queue").retryCleanup()) {
				if (std::chrono::steady_clock::now() >= cutoff) return 7;
				QThread::sleep(10);
			}
			return code;
		};
		while (std::chrono::steady_clock::now() < cutoff) {
			if (std::chrono::steady_clock::now() >= nextCleanup) {
				hhc::SessionStore(root + "/queue").retryCleanup();
				nextCleanup = std::chrono::steady_clock::now() + std::chrono::seconds(10);
			}
			try {
				const auto r =
					hhc::keyTransferStep(root, options.value("k3"), false, false, false, validated);
				failures = 0;
				if (r.sync.state == "ready" && r.sync.autoPublish != "pending" && r.sync.localCleanupComplete)
					return finish(0);
				if (QStringList{"aborted", "failed", "expired"}.contains(r.sync.state))
					return finish(4);
				if (!r.sync.pendingBytes)
					QThread::sleep(10);
			} catch (const hhc::RequestError &e) {
				qInfo().noquote() << "[HHC helper] utc=" + QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
					<< "status=" + QString::number(e.status) << "result=request_failed";
				if (e.code == "broadcast_ingest_revoked") {
					QThread::sleep(5);
					continue;
				}
				if (e.code == "ingest_no_bind_intent")
					return finish(0);
				if (e.outputOpening && e.code == "broadcast_output_terminal")
					return finish(0);
				const auto delay = e.retryDelayMs(failures);
				if (!delay)
					return finish(5);
				const auto retryAt =
					std::chrono::steady_clock::now() + std::chrono::milliseconds(*delay);
				while (std::chrono::steady_clock::now() < retryAt) {
					if (std::chrono::steady_clock::now() >= cutoff)
						return 7;
					const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
								       retryAt - std::chrono::steady_clock::now())
								       .count();
					QThread::msleep(static_cast<unsigned long>(
						std::max<qint64>(0, std::min<qint64>(remaining, 1000))));
				}
			} catch (...) {
				return finish(6);
			}
		}
		return 7;
	}
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
				if (j.size() > 8 * 1024 * 1024 || !hhc::openSharedJsonRead(j))
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
					auto state = sync.step(false, [&] {
						if (QFileInfo::exists(dir + "/broadcast-journal.json"))
							hhc::BroadcastControl(dir, account, j.localId, api).replay();
					});
					terminalFailed |= state.state == "failed" || state.state == "expired";
					pending |= state.state != "ready" && state.state != "aborted" &&
							   state.state != "expired" && state.state != "failed" ||
						   state.state == "ready" && (state.autoPublish == "pending" || !state.localCleanupComplete);
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
