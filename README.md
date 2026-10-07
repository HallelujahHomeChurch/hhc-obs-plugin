# HHC OBS plugin — Windows x64 development

Independent Program capture using three NVIDIA NVENC H.264 encoders and one explicitly selected AAC audio mixer. Never mutates OBS global settings, streaming or original recording configuration. No CPU video fallback. Windows only; no CLI is required for the planned operator dock.

**Prototype, not a release candidate.** W0/W1 real OBS short capture is available; long capture is running. HTTP/OAuth/server UI are gated on the frozen Mac C1, V1 and S1 handoffs. There is no plugin remote, PR, hosted CI or production deployment yet.

## Build

Install Visual Studio 2022 Build Tools with C++ x64 and Windows SDK 10.0.26100, CMake >=3.28. The verified local combination is MSVC 19.44.35228, CMake 3.31.6, OBS 32.2.2, Qt 6.11.1 and FFmpeg 8.1 (official obs-deps 2026-07-15). Only OBS 32.2.2 x64 is currently tested. Do not overwrite OBS's Qt/FFmpeg runtime with random DLLs.

```powershell
./scripts/bootstrap-windows.ps1
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
```

Bootstrap follows the necessary Development-component build route of the [official plugin template](https://github.com/obsproject/obs-plugintemplate), pinned to [OBS 32.2.2](https://github.com/obsproject/obs-studio/tree/32.2.2) and that tag's dependency hashes. VS2022 is the toolchain verified here; OBS's full-application preset uses VS2026. We build only its plugin SDK with VS2022, not the OBS application. No macOS target is supplied.

## Evidence levels

- `capture-test`: pure native safety policy unit tests (no GPU).
- `obs-capture-test`: development executable using the real installed libobs, D3D11 and NVENC. This is not the OBS frontend.
- `hhc-obs-fixture.dll`: separate developer-only plugin loaded into a dedicated portable OBS 32.2.2 copy. Generates a synthetic Program with visible timecode; runs 61 or 9000 seconds. Never distribute this target as the operator plugin. It refuses non-portable or already streaming/recording instances.
- `scripts/verify-media.py`: local per-object SHA256, stream format, aligned segment timeline and complete decoding. Not Mac Asset V1 or E2E.
- `docs/progress.md`: actual results, rulings, missing inputs and known issues.

The operator's correction fixes the media rate at **30000/1001 (29.97)**. 60-frame GOP produces regular 30.03-second boundaries; exact frame-based EXTINF values must be preserved. Three renditions remain 1080p 3Mbps, 720p 1.5Mbps, 854x480 800kbps; AAC 128kbps/48kHz/stereo. Global mismatches fail visibly; no automatic OBS setting change.

## Safety and current limitations

Packet callback only retains packets into a 64 MiB bounded queue. A worker does mux/file/hash work. Closed segments are atomically copied into the local queue; changing playlists are kept in staging until finalization. The local manifest is explicitly not a wire schema. Only user stop with all successful tails can mark normalEnd. No network upload exists yet; normalEnd never means published.

Runtime disk/package quotas and header/stop watchdogs are implemented; injected boundary testing remains. Account-bound journal recovery is integrated into captures that provide queue identity and has controlled process-termination evidence. Credential Manager and PKCE remain independent of the controller. Still required before P1: production controller/auth integration, production Qt dock, network OAuth after C1, consumer tests, fault/concurrency qualification, package install/remove and final review. F1-L currently tests HHC output only; concurrent original recording/YouTube and five-hour two-mode resource tests remain separate unverified requirements. No code signing certificate supplied.
