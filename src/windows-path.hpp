#pragma once
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QUuid>
#include <io.h>
#include <fcntl.h>
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
// A mutable JSON reader must not deny the producer's atomic replacement on Windows.
// Keep the opened snapshot readable; QFile owns the descriptor after successful adoption.
inline bool openSharedJsonRead(QFile &file)
{
	if (file.isOpen())
		return false;
	const auto path = lockFilePath(file.fileName());
	HANDLE handle = INVALID_HANDLE_VALUE;
	// Concurrent replacement briefly removes or locks the name. Keep the wait bounded.
	for (int attempt = 0; attempt < 5; ++attempt) {
		handle = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ,
				     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
				     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		const auto error = GetLastError();
		if (handle != INVALID_HANDLE_VALUE ||
		    (error != ERROR_FILE_NOT_FOUND && error != ERROR_SHARING_VIOLATION) || attempt == 4)
			break;
		Sleep(10);
	}
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_RDONLY | _O_BINARY);
	if (fd == -1) {
		CloseHandle(handle);
		return false;
	}
	if (file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle))
		return true;
	_close(fd);
	return false;
}
// Qt uses MoveFileEx for overwrite, which fails while a shared-delete reader is open.
// Flush a same-directory temporary file and use ReplaceFile to preserve reader snapshots.
inline bool writeAtomicMetadata(const QString &path, const QByteArray &bytes)
{
	if (bytes.size() > 8 * 1024 * 1024)
		return false;
	const auto from = lockFilePath(path + "." + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".tmp");
	const auto to = lockFilePath(path);
	HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(from.utf16()), GENERIC_WRITE, 0, nullptr,
				   CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	DWORD written = 0;
	const bool flushed = WriteFile(handle, bytes.constData(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
		written == bytes.size() && FlushFileBuffers(handle);
	const bool closed = CloseHandle(handle);
	if (!flushed || !closed) {
		DeleteFileW(reinterpret_cast<LPCWSTR>(from.utf16()));
		return false;
	}
	if (ReplaceFileW(reinterpret_cast<LPCWSTR>(to.utf16()), reinterpret_cast<LPCWSTR>(from.utf16()),
			 nullptr, 0, nullptr, nullptr))
		return true;
	if (GetLastError() == ERROR_FILE_NOT_FOUND &&
	    MoveFileExW(reinterpret_cast<LPCWSTR>(from.utf16()), reinterpret_cast<LPCWSTR>(to.utf16()),
			MOVEFILE_WRITE_THROUGH))
		return true;
	// Keep flushed bytes if replacement failed after changing the destination's name.
	if (QFileInfo::exists(path))
		DeleteFileW(reinterpret_cast<LPCWSTR>(from.utf16()));
	return false;
}
} // namespace hhc
