#include "runtime-paths.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <iostream>
#include <QLibrary>
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir root;
    int failed = 0;
    const auto relocated = root.path() + "/relocated-probe.dll";
    if (!QFile::copy(app.applicationDirPath() + "/runtime-path-probe.dll", relocated)) return 2;
    QLibrary probe(relocated);
    auto getModule = reinterpret_cast<const char *(*)()>(probe.resolve("loadedModuleFile"));
    if (!getModule || QFileInfo(QString::fromUtf8(getModule())).canonicalFilePath() != QFileInfo(relocated).canonicalFilePath()) {
        std::cerr << "Module lookup must follow relocated DLL, not host executable\n"; ++failed;
    }
    probe.unload();
    for (const auto &layout : {"ProgramData/plugins/hhc-obs-plugin", "portable/plugins/hhc-obs-plugin", "obs/obs-plugins/64bit"}) {
        const auto dir = root.path() + "/" + layout;
        QDir().mkpath(dir);
        QFile helper(dir + "/hhc-upload-helper.exe");
        if (!helper.open(QIODevice::WriteOnly)) return 2; helper.close();
        const auto actual = hhc::uploadHelperPath(dir + "/hhc-obs-plugin.dll");
        if (actual != helper.fileName()) { std::cerr << "helper must be adjacent to loaded module: " << layout << '\n'; ++failed; }
    }
    const auto modern = root.path() + "/ProgramData/plugins/hhc-obs-plugin";
    QDir().mkpath(modern + "/data/qt-plugins/tls");
    QFile modernTls(modern + "/data/qt-plugins/tls/qschannelbackend.dll");
    if (!modernTls.open(QIODevice::WriteOnly)) return 2; modernTls.close();
    if (hhc::helperTlsPath(modern) != modern + "/data/qt-plugins") { std::cerr << "modern TLS path\n"; ++failed; }
    const auto legacy = root.path() + "/obs/obs-plugins/64bit";
    QDir().mkpath(root.path() + "/obs/data/obs-plugins/hhc-obs-plugin/qt-plugins/tls");
    QFile legacyTls(root.path() + "/obs/data/obs-plugins/hhc-obs-plugin/qt-plugins/tls/qschannelbackend.dll");
    if (!legacyTls.open(QIODevice::WriteOnly)) return 2; legacyTls.close();
    if (QDir::cleanPath(hhc::helperTlsPath(legacy)) != root.path() + "/obs/data/obs-plugins/hhc-obs-plugin/qt-plugins") { ++failed; }
    if (!hhc::uploadHelperPath(root.path() + "/missing/plugin.dll").isEmpty()) ++failed;
    if (!hhc::uploadHelperPath({}).isEmpty()) ++failed;
    if (!hhc::helperTlsPath(root.path() + "/missing").isEmpty()) ++failed;
    if (QFileInfo(hhc::currentModuleFile()).canonicalFilePath() != QFileInfo(app.applicationFilePath()).canonicalFilePath()) ++failed;
    return failed ? 1 : 0;
}
