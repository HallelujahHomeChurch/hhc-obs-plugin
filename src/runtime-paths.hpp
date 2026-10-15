#pragma once
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <windows.h>
namespace hhc {
inline QString uploadHelperPath(const QString &moduleFile)
{
    if (moduleFile.isEmpty()) return {};
    const auto helper = QFileInfo(moduleFile).dir().absoluteFilePath("hhc-upload-helper.exe");
    return QFileInfo(helper).isFile() ? helper : QString{};
}
inline QString currentModuleFile()
{
    static const char anchor = 0;
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&anchor), &module)) return {};
    wchar_t path[32768];
    const auto size = GetModuleFileNameW(module, path, 32768);
    if (!size || size >= 32768) return {};
    return QString::fromWCharArray(path, int(size));
}
inline QString helperTlsPath(const QString &helperDirectory)
{
    const QDir dir(helperDirectory);
    const auto modern = dir.absoluteFilePath("data/qt-plugins");
    if (QFileInfo(modern + "/tls/qschannelbackend.dll").isFile()) return modern;
    const auto legacy = QDir::cleanPath(dir.absoluteFilePath("../../data/obs-plugins/hhc-obs-plugin/qt-plugins"));
    return QFileInfo(legacy + "/tls/qschannelbackend.dll").isFile() ? legacy : QString{};
}
}
