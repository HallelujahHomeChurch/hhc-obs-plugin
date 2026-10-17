#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <QLibrary>
#include <QString>
namespace hhc {
// NVIDIA driver API; no bundled DLL or subprocess. Never substitute general GPU/3D utilization.
inline int sampleEncoderGpu()
{
 struct Nvml {
  QLibrary library;
  using Init = int (*)();
  using Count = int (*)(unsigned *);
  using Handle = int (*)(unsigned, void **);
  using Utilization = int (*)(void *, unsigned *, unsigned *);
  Init shutdown = nullptr;
  Count count = nullptr;
  Handle handle = nullptr;
  Utilization utilization = nullptr;
  bool ready = false;
  Nvml() {
   wchar_t directory[MAX_PATH];
   const auto size = GetSystemDirectoryW(directory, MAX_PATH);
   if (!size || size >= MAX_PATH) return;
   library.setFileName(QString::fromWCharArray(directory) + "/nvml.dll");
   auto init = reinterpret_cast<Init>(library.resolve("nvmlInit_v2"));
   shutdown = reinterpret_cast<Init>(library.resolve("nvmlShutdown"));
   count = reinterpret_cast<Count>(library.resolve("nvmlDeviceGetCount_v2"));
   handle = reinterpret_cast<Handle>(library.resolve("nvmlDeviceGetHandleByIndex_v2"));
   utilization = reinterpret_cast<Utilization>(library.resolve("nvmlDeviceGetEncoderUtilization"));
   ready = init && shutdown && count && handle && utilization && init() == 0;
  }
  ~Nvml() { if (ready) shutdown(); }
 };
 static Nvml api;
 if (!api.ready) return -1;
 unsigned count = 0;
 if (api.count(&count) != 0 || count != 1) return -1;
 // shortcut: single NVIDIA GPU only; map OBS adapter identity before supporting multi-GPU telemetry.
 void *device = nullptr;
 unsigned value = 0, samplingUs = 0;
 if (api.handle(0, &device) != 0 || api.utilization(device, &value, &samplingUs) != 0 || value > 100) return -1;
 return int(value);
}
}
