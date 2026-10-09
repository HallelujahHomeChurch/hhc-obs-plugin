#include "windows-path.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTemporaryDir temporary;
	int failures = 0;
	auto check = [&](bool ok, const char *name) {
		if (!ok) {
			std::cerr << "FAIL " << name << " (Win32 " << GetLastError() << ")\n";
			++failures;
		}
	};
	const QByteArray bytes("closed immutable segment bytes");
	const auto read = [](const QString &path) {
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
	};
	for (bool longPath : {false, true}) {
		QString root = temporary.path() + (longPath ? "/long" : "/short");
		while (longPath && root.size() < 280)
			root += QString::fromUtf8("/影音暫存-1234567890");
		check(QDir().mkpath(root), "create owned test directory");
		const auto metadata = root + "/journal.json";
		check(hhc::writeAtomicMetadata(metadata, "first"), "create atomic metadata");
		QFile snapshot(metadata);
		check(hhc::openSharedJsonRead(snapshot), "open shared metadata snapshot");
		check(hhc::writeAtomicMetadata(metadata, "second") && snapshot.readAll() == "first" &&
			      read(metadata) == "second", "replacement preserves old snapshot on short and long paths");
		snapshot.close();
		const auto source = root + "/closed.m4s", destination = root + "/queued.m4s";
		QFile file(source);
		check(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "write closed bytes");
		file.close();
		check(hhc::moveQueueSegment(source, destination),
		      longPath ? "move beyond MAX_PATH" : "move short path");
		check(!QFile::exists(source) && read(destination) == bytes, "move keeps bytes without staging copy");
		check(file.open(QIODevice::WriteOnly) && file.write("replacement") == 11, "write conflicting segment");
		file.close();
		check(!hhc::moveQueueSegment(source, destination) && read(source) == "replacement" &&
			      read(destination) == bytes,
		      "existing queue object and staging bytes both retained");
		check(QFile::remove(destination), "remove only owned temporary destination");
		const auto nativeSource = hhc::lockFilePath(source);
		HANDLE held = CreateFileW(reinterpret_cast<LPCWSTR>(nativeSource.utf16()), GENERIC_READ,
					  FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		check(held != INVALID_HANDLE_VALUE, "hold source without delete sharing");
		check(!hhc::moveQueueSegment(source, destination) && read(source) == "replacement" &&
			      !QFile::exists(destination),
		      "sharing denial retains staging and invents no queued file");
		if (held != INVALID_HANDLE_VALUE)
			CloseHandle(held);
		check(hhc::moveQueueSegment(source, destination) && !QFile::exists(source) &&
			      read(destination) == "replacement",
		      "move succeeds once sharing denial is released");
	}
	return failures ? 1 : 0;
}
