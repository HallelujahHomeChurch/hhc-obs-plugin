#include "session-store.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
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
	return failures ? 1 : 0;
}
