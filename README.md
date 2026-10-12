# HHC OBS plugin — Windows x64 development

Independent Program capture using three NVIDIA NVENC H.264 encoders and one explicitly selected AAC audio mixer. Never mutates OBS global settings, streaming or original recording configuration. No CPU video fallback. Windows only; operators use the native Qt dock.

**Unsigned platform preview; whole-product acceptance incomplete.** Fixed B1 b1-2026-10-10.rc1 and C1 c1-2026-10-08.2 HTTP/OAuth, Console binding/control, durable command ACKs and recovery are implemented. The installed candidate is source `9c658aeb76e6fe9dc0b3b3ce37c7088a09e77e82`, ZIP SHA-256 `b598ac6fe45348de493da2ff69b80d82d19ed7108e9829ef017bba370df0a0e8`. A normal synthetic150-minute public field now passes End/Stop/seal, complete local hashes/three-rendition/global-recording decodes, **943/943 formal verification → ready → automatic publication**, exact inherited JPEG bytes and stated same-page End→VOD/quality checks. Two natural browser refresh cycles and renewed preview decoding pass separately. Current strong-network rapid VOD switching preserves last frame/position/pause/1.5x with measured restoration upper bounds1.854–2.505s. The separate five-minute upload interruption drains backlog within ten minutes, but its128.129-second delay fails the strict60–120-second pressure threshold. Historical failures remain. See [normal long field](docs/reviews/windows-b1-normal-long-2026-10-12.md), [pressure evidence](docs/reviews/windows-b1-pressure-2026-10-12.md) and [independent platform UI findings](docs/reviews/mac-auto-cover-original-page-followup-2026-10-12.md).

Earlier C1 media, thumbnails, selected audio tracks1–6/mute, Studio Mode Program/Preview and recovery results remain scoped to their tested candidates: [integration evidence](docs/reviews/windows-e2e-2026-10-10.md), [earlier long field](docs/reviews/windows-long-current-ci-2026-10-10.md), [thumbnail evidence](docs/reviews/windows-thumbnails-2026-10-10.md) and [next gates](docs/windows-integration-next-gates.md). Local tests, hosted CI, platform deployment and a final published VOD do not establish every live playback/recovery gate. Development stays on feat/windows-capture and [draft PR#1](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/pull/1); no merge or production deployment.

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

Packet callback only retains packets into a 64 MiB bounded queue. A worker does mux/file/hash work. Closed segments move atomically into the local queue without a retained staging media copy; changing playlists are kept in staging until finalization. The local manifest is explicitly not a wire schema; finalization constructs the pinned canonical wire inventory separately. Only confirmed user stop with all successful tails can mark normalEnd. Uploaded or queued objects and local normalEnd never mean server ready or published.

Runtime disk/package quotas and header/stop watchdogs have developer fault-harness evidence. Credential Manager, PKCE, rotating refresh, durable queues and native recovery are integrated. Earlier transport-outage backfill, OBS exit/background completion and original-session abort results retain their original scope. The earlier c2166da long field stalled around two hours and later reached its10GB guard; its source/journals are preserved, and current long-run success is not a proven fix for that cause. Remaining qualification includes sustained recovery under actual congregation/YouTube load, physical disconnection/current active crash, complete current authorization/thumbnail matrix and native Safari/fullscreen. Historical379/388-second lag,18.491-second switching and32.730-second weak uncached seeking remain separately scoped failures/limits. No code signing certificate supplied. Completed disposable platform test recordings are cleaned after evidence retention; protected originals and local media/journals remain.

The production dock exposes login, audio-track selection, independent live/publication intent, stop and session recovery. The developer local-validation dock and its offline queue stay separate from platform accounts. See [Windows preview instructions](docs/windows-local-preview.md) for GUI installation, use and removal. Package with scripts/package-windows.ps1 after committing tested source; fixture/fault harnesses are excluded.

The platform preview consumes pinned C1 c1-2026-10-08.2: [immutable contract acknowledgement](docs/reviews/c1-2026-10-08.2-acknowledgement.json). See [integration history](docs/windows-platform-integration.md) and [繁體中文操作說明](docs/windows-local-preview.md). Local/CI tests do not certify production readiness, publication or member playback. The unsigned package validates its compiled source revision and includes the pinned Qt Schannel TLS backend; original OBS/YouTube settings are preserved.
