#include "session-store.hpp"
#include "windows-path.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QLockFile>
#include <QProcess>
#include <iostream>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTemporaryDir temp;
	// Exercise the same extended-length paths as installed OBS.
	hhc::SessionStore store(temp.path() + "/" + QString(100, 'a') + "/" + QString(100, 'b'));
	int failures = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) { ++failures; std::cerr << "FAIL " << name << '\n'; }
	};
	auto make = [&](const QString &id) {
		hhc::CaptureJournal j;
		j.account = "cleanup-test"; j.localId = id; j.stopIntent = j.normalEnd = true;
		const auto dir = store.mediaDirectory(j.account, id);
		QDir().mkpath(dir + "/1080p");
		for (const auto *name : {"init.mp4", "seg-000000.m4s"}) {
			QFile f(dir + "/1080p/" + name);
			check(f.open(QIODevice::WriteOnly) && f.write("generated") == 9, "create test HLS");
			f.close();
			j.objects.append({QString("1080p/") + name, 9,
				QString::fromLatin1(QCryptographicHash::hash("generated", QCryptographicHash::Sha256).toHex()), false});
		}
		store.save(j);
		return j;
	};
	auto ready = [&](const hhc::CaptureJournal &j) {
		store.confirmReady(j.account, j.localId, QString(32, 'a'), QString(32, 'b'), QDateTime::currentDateTimeUtc());
	};
	auto edit = [&](const hhc::CaptureJournal &j, auto mutate) {
		const auto path = store.mediaDirectory(j.account, j.localId) + "/journal.json";
		QFile f(path); check(f.open(QIODevice::ReadOnly), "read journal fixture");
		auto json = QJsonDocument::fromJson(f.readAll()).object(); f.close();
		mutate(json);
		check(hhc::writeAtomicMetadata(path, QJsonDocument(json).toJson()), "write journal fixture");
	};
	const auto j = make("partial");
	const auto dir = store.mediaDirectory(j.account, j.localId);
	check(!store.cleanupReady(j.account, j.localId) && QFile::exists(dir + "/1080p/init.mp4"),
		"normal stop without ready retains media");
	ready(j);
	QDir().mkpath(dir + "/staging/1080p");
	QFile staging(dir + "/staging/1080p/init.mp4");
	check(staging.open(QIODevice::WriteOnly) && staging.write("generated") == 9, "stage generated copy"); staging.close();
	QFile original(dir + "/original-recording.mp4");
	check(original.open(QIODevice::WriteOnly) && original.write("original") == 8, "original sentinel"); original.close();
	const auto blockedPath = hhc::lockFilePath(dir + "/1080p/seg-000000.m4s");
	HANDLE blocker = CreateFileW(reinterpret_cast<LPCWSTR>(blockedPath.utf16()), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	check(blocker != INVALID_HANDLE_VALUE, "hold segment against deletion");
	check(!store.cleanupReady(j.account, j.localId), "locked media retries without claiming completion");
	check(!QFile::exists(dir + "/1080p/init.mp4") && QFile::exists(dir + "/1080p/seg-000000.m4s"),
		"partial cleanup retains only locked owned bytes");
	check(store.scanPending(j.account).issues.empty(), "partial deletion is recoverable after restart");
	if (blocker != INVALID_HANDLE_VALUE) CloseHandle(blocker);
	QLockFile lock(hhc::lockFilePath(dir + "/sync.lock")); lock.setStaleLockTime(0);
	check(lock.tryLock(0), "hold synchronization owner");
	store.retryCleanup();
	check(QFile::exists(dir + "/1080p/seg-000000.m4s"), "startup cleanup respects sync owner");
	lock.unlock();
	store.retryCleanup();
	check(!QFile::exists(dir + "/1080p/seg-000000.m4s") && !QFile::exists(staging.fileName()) &&
		QFile::exists(original.fileName()), "local restart cleanup removes HLS and staging but preserves original");
	const auto recovered = store.loadPending(j.account).first();
	check(recovered.cleanupState == "complete" && recovered.objects.size() == 2 && recovered.confirmedReady &&
		store.cleanupReady(j.account, j.localId), "idempotent cleanup preserves hashes and ready evidence");

	const auto old = make("legacy"); ready(old);
	edit(old, [](QJsonObject &o) { o.remove("autoCleanup"); o.remove("cleanupState"); });
	check(store.cleanupReady(old.account, old.localId) &&
		QFile::exists(store.mediaDirectory(old.account, old.localId) + "/1080p/init.mp4"),
		"pre-upgrade diagnostics are not swept");
	const auto mismatch = make("mismatch"); ready(mismatch);
	edit(mismatch, [](QJsonObject &o) { o["readyPackage"] = QString(32, 'f'); });
	check(!store.cleanupReady(mismatch.account, mismatch.localId), "mismatched package never deleted");
	const auto unsafe = make("unsafe"); ready(unsafe);
	edit(unsafe, [](QJsonObject &o) {
		auto objects = o["objects"].toArray(); auto object = objects[1].toObject();
		object["path"] = "../original.mp4"; objects[1] = object; o["objects"] = objects;
	});
	check(!store.cleanupReady(unsafe.account, unsafe.localId) &&
		QFile::exists(store.mediaDirectory(unsafe.account, unsafe.localId) + "/1080p/init.mp4"),
		"preflight refuses traversal before any deletion");
	const auto blockedJournal = make("journal-locked"); ready(blockedJournal);
	const auto journalPath = hhc::lockFilePath(store.mediaDirectory(blockedJournal.account, blockedJournal.localId) + "/journal.json");
	blocker = CreateFileW(reinterpret_cast<LPCWSTR>(journalPath.utf16()), GENERIC_READ, FILE_SHARE_READ,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	check(blocker != INVALID_HANDLE_VALUE && !store.cleanupReady(blockedJournal.account, blockedJournal.localId) &&
		QFile::exists(store.mediaDirectory(blockedJournal.account, blockedJournal.localId) + "/1080p/init.mp4"),
		"failed durable cleanup intent deletes nothing");
	if (blocker != INVALID_HANDLE_VALUE) CloseHandle(blocker);
	const auto linked = make("junction"); ready(linked);
	const auto linkDir = store.mediaDirectory(linked.account, linked.localId) + "/1080p";
	const auto outside = temp.path() + "/outside";
	check(QDir().rename(linkDir, outside), "move fixture directory outside queue");
	QProcess junction;
	junction.start("cmd.exe", {"/c", "mklink", "/J", hhc::lockFilePath(linkDir), hhc::lockFilePath(outside)});
	check(junction.waitForFinished() && junction.exitCode() == 0, "create junction fixture");
	check(!store.cleanupReady(linked.account, linked.localId) && QFile::exists(outside + "/init.mp4"),
		"junction never deletes outside session");
	const auto nativeLink = hhc::lockFilePath(linkDir);
	check(RemoveDirectoryW(reinterpret_cast<LPCWSTR>(nativeLink.utf16())) != 0, "remove only test junction");
	return failures ? 1 : 0;
}
