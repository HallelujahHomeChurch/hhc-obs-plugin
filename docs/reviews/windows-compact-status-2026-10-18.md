# Compact HHC status bar — 2026-10-18

Scope: Windows UI only; no transport, wire, encoder, credentials, native stream or recording changes.

The operator approved keeping HHC state and whole-GPU Video Encode utilization visible. Upload throughput (30-second successful-media average) and pending media remain available in the tooltip. Pending bytes appear immediately during retries, operator-required recovery and final upload, or after 60 seconds continuously nonzero. This threshold controls visibility only: it is not a network-failure or playback-health verdict. Zero pending resets the threshold. Final upload is labelled 補傳中; completion without pending bytes remains 收尾中 until the existing release logic completes.

OBS's CPU, FPS, native stream/record timers remain untouched. HHC throughput is not combined with YouTube/native stream throughput. GPU sampling still reports whole-device Video Encode usage and — when unavailable.

## Evidence

- Focused test failed against the original verbose display, then passed after implementation.
- Local CTest: 25/25 passed, including transient/sustained pending, clear/reset, retry, final upload, idle, tooltip and native UI integration cases.
- Candidate commit: `b1b2dd6059c586684fa87d6171da8e6eefb669d0`.
- ZIP SHA-256: `7c0043aa37e7355dc7177dbb377e6d588d9bca981043f4d12e4cd6d0612e750e`.
- Updated only `.deps/portable-recovery` OBS 32.2.2, after backing up its DLL/helper. Startup log confirms native integration. Real idle UI shows `HHC：未連線 | GPU 編碼：0%`, with native CPU/FPS and inactive recording/stream timers present.
- Screenshot: `artifacts/compact-status-20261018/isolated-obs.jpg`.
- No new capture, recording or native stream was started. Formal OBS installation was not changed. Active-stream variants are covered by local state tests, not a new real livestream.
- Branch `feat/compact-hhc-status`; no PR, remote CI, merge or formal installation in this request.

The existing formal Sunday capture is monitored independently under `artifacts/formal-live-monitor-20261018`; do not classify it as disposable test material.
