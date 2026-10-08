# HHC OBS plugin — Windows x64 development

Independent Program capture using three NVIDIA NVENC H.264 encoders and one explicitly selected AAC audio mixer. Never mutates OBS global settings, streaming or original recording configuration. No CPU video fallback. Windows only; operators use the native Qt dock.

**Unsigned platform preview; end-to-end acceptance incomplete.** Fixed C1 c1-2026-10-08.2 HTTP/OAuth is implemented and acknowledged. Earlier production short tests observed login, refresh, upload, stop/seal, ready, automatic publication and member live/VOD playback. Mac has released the 29.97 fps final-seal repair; a fresh positive test is still required because the original eight-segment recording was already deleted with authorization and its capture is terminal. The corrected local 2.5-hour run passes all907 object hashes,300 fragments per rendition,9000.024367 seconds, aligned timelines and full decoding. Selected audio tracks1–6/mute and Studio Mode Program/Preview isolation pass in actual local OBS. Original recording plus HHC long-run and current production E2E remain separate gates. See [current evidence](docs/reviews/windows-e2e-2026-10-08.md) and [next gates](docs/windows-integration-next-gates.md). Development stays on feat/windows-capture and [draft PR#1](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/pull/1); no merge or production deployment.

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

Runtime disk/package quotas and header/stop watchdogs have developer fault-harness evidence. Credential Manager, PKCE, rotating refresh, durable queues and native recovery are integrated. Actual production tests observed65s transport-outage backfill, accepted OBS exit/background completion and interrupted encoding followed by original-session abort. These earlier results do not qualify the current candidate's full positive E2E path. Remaining gates include30min live with5min outage/10min catch-up, measured latency, complete normal-stop DVR/expiry, current2.5h production tests, concurrent original recording/YouTube, ordinary single-instance crash startup and physical keyboard/DPI/iPhone Safari. Short developer fixtures report one allocation whose cause remains unproven; the corrected local9000s exit reported zero. No code signing certificate supplied.

The production dock exposes login, audio-track selection, independent live/publication intent, stop and session recovery. The developer local-validation dock and its offline queue stay separate from platform accounts. See [Windows preview instructions](docs/windows-local-preview.md) for GUI installation, use and removal. Package with scripts/package-windows.ps1 after committing tested source; fixture/fault harnesses are excluded.

The platform preview consumes pinned C1 c1-2026-10-08.2: [immutable contract acknowledgement](docs/reviews/c1-2026-10-08.2-acknowledgement.json). See [integration history](docs/windows-platform-integration.md) and [繁體中文操作說明](docs/windows-local-preview.md). Local/CI tests do not certify production readiness, publication or member playback. The unsigned package validates its compiled source revision and includes the pinned Qt Schannel TLS backend; original OBS/YouTube settings are preserved.
