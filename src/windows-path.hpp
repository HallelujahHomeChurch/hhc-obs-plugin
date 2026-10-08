#pragma once
#include <QDir>
#include <QFileInfo>
namespace hhc {
// QLockFile's Windows removal path needs the extended prefix beyond MAX_PATH.
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
} // namespace hhc
