# HHC OBS plugin — Windows x64 development

Independent Program capture using three NVIDIA NVENC H.264 encoders and one explicitly selected AAC audio mixer. Never mutates OBS global settings, streaming or original recording configuration. No CPU video fallback. Windows only; operators use the native Qt dock.

**Unsigned platform preview; end-to-end acceptance incomplete.** The fixed platform handoff is now available and HTTP/OAuth is implemented. Actual production login, refresh, signed uploads, stop and seal have been observed. The first completed package failed server validation; the producer's CODECS omission is fixed and new short material passes the fixed validator locally. Further production recording tests are blocked by the two-draft quota until the failed synthetic drafts are removed with authorization. Earlier 2.5-hour local captures are retained; the current media-core long run is separately in progress. See the integration ledger for exact evidence. Remote: https://github.com/HallelujahHomeChurch/hhc-obs-plugin. Development stays on feat/windows-capture; no merge or production deployment. Local, CI and production results remain distinct.

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

Packet callback only retains packets into a 64 MiB bounded queue. A worker does mux/file/hash work. Closed segments are atomically copied into the local queue; changing playlists are kept in staging until finalization. The local manifest is explicitly not a wire schema; finalization constructs the pinned canonical wire inventory separately. Only user stop with all successful tails can mark normalEnd. Uploaded or queued objects and local normalEnd never mean server ready or published.

Runtime disk/package quotas and header/stop watchdogs are implemented and have developer fault-harness evidence. Account-bound journal recovery has actual controlled OBS termination evidence. Credential Manager, PKCE, rotating refresh and the production controller are integrated. Remaining acceptance includes server-ready media, automatic publication, live/full DVR, member playback and actual network/crash recovery on production, followed by current-version long-run qualification. Concurrent original recording/YouTube and five-hour two-mode resource tests remain separate unverified requirements. No code signing certificate supplied.

The production dock exposes login, audio-track selection, independent live/publication intent, stop and session recovery. The developer local-validation dock and its offline queue stay separate from platform accounts. See [Windows preview instructions](docs/windows-local-preview.md) for GUI installation, use and removal. Package with scripts/package-windows.ps1 after committing tested source; fixture/fault harnesses are excluded.

The platform preview implements the pinned c1-2026-10-07.1 HTTP/OAuth contract: native PKCE and Windows Credential Manager, independent NVENC capture at 30000/1001, signed immutable uploads, durable stop/seal and separate live/publication controls, account-bound recovery and hidden background upload helper. See [integration evidence](docs/windows-platform-integration.md) and [operator preview instructions](docs/windows-local-preview.md). Local/CI tests do not certify production ready, publication, member playback or long-run qualification. The unsigned package validates its compiled source revision and includes the pinned Qt Schannel TLS backend; original OBS/YouTube settings are preserved.
