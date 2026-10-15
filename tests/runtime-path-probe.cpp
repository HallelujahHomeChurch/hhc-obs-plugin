#include "runtime-paths.hpp"
#include <QByteArray>
extern "C" __declspec(dllexport) const char *loadedModuleFile()
{
    static const auto path = hhc::currentModuleFile().toUtf8();
    return path.constData();
}
