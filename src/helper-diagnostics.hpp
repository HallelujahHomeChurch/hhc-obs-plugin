#pragma once
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLockFile>
#include "windows-path.hpp"
namespace hhc {
inline bool appendHelperDiagnostic(const QString &directory, const QString &message)
{
	const auto bytes = message.toUtf8() + '\n';
	if (!message.startsWith("[HHC ") || message.contains('\n') || message.contains('\r') || bytes.size() > 2048 ||
	    QFileInfo(directory).isSymLink() || !QDir().mkpath(directory)) return false;
	QLockFile owner(lockFilePath(directory + "/helper.lock"));
	owner.setStaleLockTime(0);
	if (!owner.tryLock(5)) return false;
	const auto path = directory + "/helper.log", previous = directory + "/helper.previous.log";
	if (QFileInfo(path).isSymLink() || QFileInfo(previous).isSymLink()) return false;
	if (QFileInfo(path).size() + bytes.size() > 8 * 1024 * 1024) {
		if (QFile::exists(previous) && !QFile::remove(previous)) return false;
		if (!QFile::rename(path, previous)) return false;
	}
	QFile file(path);
	return file.open(QIODevice::WriteOnly | QIODevice::Append) && file.write(bytes) == bytes.size() && file.flush();
}
}
