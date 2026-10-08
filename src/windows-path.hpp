#pragma once
#include <QDir>
#include <QFileInfo>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace hhc {
// Native Win32 file operations need the extended prefix beyond MAX_PATH.
inline QString lockFilePath(const QString &path)
{
	auto absolute = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
#ifdef Q_OS_WIN
	if (!absolute.startsWith(QStringLiteral("\\\\?\\")))
		return absolute.startsWith(QStringLiteral("\\\\")) ? QStringLiteral("\\\\?\\UNC\\") + absolute.mid(2)
								   : QStringLiteral("\\\\?\\") + absolute;
#endif
	return absolute;
}
inline bool moveQueueSegment(const QString &source, const QString &destination)
{
	const auto from = lockFilePath(source), to = lockFilePath(destination);
	return MoveFileExW(reinterpret_cast<LPCWSTR>(from.utf16()), reinterpret_cast<LPCWSTR>(to.utf16()),
			   MOVEFILE_WRITE_THROUGH);
}
} // namespace hhc
