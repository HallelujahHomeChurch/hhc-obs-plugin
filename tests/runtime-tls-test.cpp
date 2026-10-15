#include "runtime-paths.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSslSocket>
#include <iostream>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 3) return 2;
    QTemporaryDir staging;
    const bool modern = QString::fromUtf8(argv[1]) == "modern";
    const auto bin = staging.path() + (modern ? "/hhc-obs-plugin" : "/obs-plugins/64bit");
    const auto tls = staging.path() + (modern ? "/hhc-obs-plugin/data/qt-plugins" : "/data/obs-plugins/hhc-obs-plugin/qt-plugins");
    if (!QDir().mkpath(bin) || !QDir().mkpath(tls + "/tls")) return 2;
    const auto backend = tls + "/tls/qschannelbackend.dll";
    if (!QFile::copy(QString::fromUtf8(argv[2]), backend)) return 2;
    QCoreApplication::setLibraryPaths({hhc::helperTlsPath(bin)});
    if (!QSslSocket::supportsSsl() || QSslSocket::activeBackend() != "schannel") {
        std::cerr << "Staged helper TLS backend did not initialize\n"; return 1;
    }
    wchar_t path[32768];
    const auto module = GetModuleHandleW(L"qschannelbackend.dll");
    if (!module || !GetModuleFileNameW(module, path, 32768) ||
        QFileInfo(QString::fromWCharArray(path)).canonicalFilePath() != QFileInfo(backend).canonicalFilePath()) {
        std::cerr << "TLS backend loaded outside tested package\n"; return 1;
    }
    return 0;
}
