#include "session-store.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <windows.h>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTemporaryDir root;
	int failures = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) {
			std::cerr << "FAIL " << name << '\n';
			++failures;
		}
	};
	auto rejects = [&](auto action, const char *name) {
		bool thrown = false;
		try {
			action();
		} catch (...) {
			thrown = true;
		}
		check(thrown, name);
	};
	hhc::SessionStore store(root.path());
	hhc::CaptureJournal j;
	j.account = "account-a";
	j.localId = "capture-001";
	auto path = store.mediaDirectory(j.account, j.localId);
	QDir().mkpath(path + "/1080p");
	QFile media(path + "/1080p/init.mp4");
	check(media.open(QIODevice::WriteOnly), "test media open");
	media.write("closed bytes");
	media.close();
	j.objects.append(
		{"1080p/init.mp4", 12,
		 QString::fromLatin1(QCryptographicHash::hash("closed bytes", QCryptographicHash::Sha256).toHex()),
		 false});
	j.stopIntent = true;
	store.save(j);
	auto loaded = store.loadPending(j.account);
	check(loaded.size() == 1, "recover durable journal after restart");
	if (!loaded.empty()) {
		check(loaded[0].stopIntent, "stop intent persists before network");
		check(!loaded[0].normalEnd, "crash does not invent normal end");
		check(loaded[0].objects[0].sha256 == j.objects[0].sha256, "immutable object identity survives restart");
	}
	check(store.loadPending("account-b").empty(), "different account cannot resume queue");
	// A capture checkpoint appends closed media without inventing remote receipts.
	hhc::CaptureJournal active;
	active.account = "account-a";
	active.localId = "active-002";
	store.save(active);
	check(store.isPrepared(active.account, active.localId),
	      "account-bound empty journal can prepare capture before HTTP creation");
	auto activePath = store.mediaDirectory(active.account, active.localId);
	QDir().mkpath(activePath + "/1080p");
	check(QFile::copy(path + "/1080p/init.mp4", activePath + "/1080p/init.mp4"), "copy closed test object");
	store.checkpointLocal(active.account, active.localId, j.objects, false, false);
	store.checkpointLocal(active.account, active.localId, j.objects, true, false);
	auto recovered = store.loadPending(active.account);
	auto found = std::find_if(recovered.begin(), recovered.end(),
				  [&](const auto &item) { return item.localId == active.localId; });
	check(found != recovered.end() && found->stopIntent && !found->normalEnd && found->objects.size() == 1 &&
		      !found->sealAcknowledged,
	      "idempotent checkpoints recover partial capture and durable stop");
	rejects([&] { store.checkpointLocal(active.account, active.localId, {}, true, true); },
		"incomplete capture cannot finalize journal");
	auto changed = j.objects;
	changed[0].sha256 = QString(64, '0');
	rejects([&] { store.checkpointLocal(active.account, active.localId, changed, false, false); },
		"checkpoint cannot replace immutable object");
	check(QFile::copy(path + "/1080p/init.mp4", activePath + "/1080p/seg-000000.m4s"), "copy orphan closed object");
	QFile receipt(activePath + "/1080p/closed.json");
	check(receipt.open(QIODevice::WriteOnly), "open atomic close receipt fixture");
	receipt.write(QJsonDocument(QJsonObject{{"objects", QJsonArray{QJsonObject{{"path", "1080p/seg-000000.m4s"},
										   {"size", 12},
										   {"sha256", j.objects[0].sha256}}}}})
			      .toJson());
	receipt.close();
	recovered = store.loadPending(active.account);
	found = std::find_if(recovered.begin(), recovered.end(),
			     [&](const auto &item) { return item.localId == active.localId; });
	check(found != recovered.end() && found->objects.size() == 2 && !found->normalEnd,
	      "restart recovers closed receipt committed before session checkpoint");
	hhc::CaptureJournal blocked;
	blocked.account = "account-a";
	blocked.localId = "blocked-003";
	store.save(blocked);
	const auto blockedPath = store.mediaDirectory(blocked.account, blocked.localId) + "/journal.json";
	HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(blockedPath.utf16()), GENERIC_READ, FILE_SHARE_READ,
				    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	check(handle != INVALID_HANDLE_VALUE, "lock existing journal against replacement");
	rejects([&] { store.checkpointLocal(blocked.account, blocked.localId, {}, true, false); },
		"failed atomic replacement reports failure");
	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);
	recovered = store.loadPending(blocked.account);
	found = std::find_if(recovered.begin(), recovered.end(),
			     [&](const auto &item) { return item.localId == blocked.localId; });
	check(found != recovered.end() && !found->stopIntent, "failed replacement retains previous valid journal");
	check(store.mediaDirectory("account-b", j.localId) != path, "account paths isolated");
	rejects([&] { store.mediaDirectory("account-a", "../outside"); }, "reject path traversal");
	auto bad = j;
	bad.objects[0].path = "../secret";
	rejects([&] { store.save(bad); }, "reject non-media path");
	bad = j;
	bad.objects[0].sha256 = QString(64, '0');
	rejects([&] { store.save(bad); }, "reject changed object hash");
	bad = j;
	bad.objects[0].path = "1080p/unfinished.tmp";
	rejects([&] { store.save(bad); }, "never upload partial file");
	check(!store.mayCleanup(j, QDateTime::currentDateTimeUtc()), "failure media is retained");
	j.normalEnd = true;
	j.sealAcknowledged = true;
	j.confirmedReady = true;
	j.packageId = "package-a";
	j.readyPackage = "package-a";
	j.readyAt = QDateTime::currentDateTimeUtc().addDays(-8);
	check(store.mayCleanup(j, QDateTime::currentDateTimeUtc()), "ready matching package older than seven days");
	j.readyPackage = "package-b";
	check(!store.mayCleanup(j, QDateTime::currentDateTimeUtc()), "mismatched remote package never cleaned");
	j.readyPackage = "package-a";
	j.readyAt = QDateTime::currentDateTimeUtc().addDays(-6);
	check(!store.mayCleanup(j, QDateTime::currentDateTimeUtc()), "seven day retention enforced");
	j.normalEnd = false;
	check(!store.mayCleanup(j, QDateTime::currentDateTimeUtc().addDays(30)), "age never makes crash successful");
	auto altered = j;
	altered.normalEnd = false;
	altered.objects[0].sha256 =
		QString::fromLatin1(QCryptographicHash::hash("new content!", QCryptographicHash::Sha256).toHex());
	check(media.open(QIODevice::WriteOnly), "test replacement open");
	media.write("new content!");
	media.close();
	rejects([&] { store.save(altered); }, "saved object identity cannot be replaced even with matching new bytes");
	// A corrupt session must stay visible without blocking unrelated valid captures.
	QFile corrupt(activePath + "/journal.json");
	check(corrupt.open(QIODevice::WriteOnly), "open corrupt journal fixture");
	corrupt.write("{interrupted-json");
	corrupt.close();
	auto report = store.scanPending("account-a");
	check(report.sessions.size() == 1 && report.sessions[0].localId == blocked.localId,
	      "recover healthy session alongside corrupt journal and changed media");
	check(report.issues.size() == 2, "report each damaged session without treating it as recoverable");
	check(QFile::exists(activePath + "/1080p/seg-000000.m4s") && QFile::exists(path + "/1080p/init.mp4"),
	      "damaged sessions retain media");
	check(corrupt.open(QIODevice::ReadOnly) && corrupt.readAll() == "{interrupted-json",
	      "scan never rewrites damaged journal");
	check(store.scanPending("account-b").sessions.empty() && store.scanPending("account-b").issues.empty(),
	      "recovery reports remain account isolated");
	rejects([&] { store.loadPending("account-a"); }, "strict recovery cannot silently discard issues");
	return failures ? 1 : 0;
}
