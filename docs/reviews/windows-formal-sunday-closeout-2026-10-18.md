# Formal Sunday stream closeout — 2026-10-18

This is a protected real meeting recording. The operator started/stopped HHC; the agent only observed journals and browser responses. No replayed Stop/seal, manual publication, upload, key change, cleanup, YouTube or native recording operation occurred.

## Identity and versions

- Recording: `2796ca13-51b1-4817-ac49-795848d68437`.
- Capture: `dce3101b54b15a24624316eeb7e3e5b4`.
- Local/output: `c349b952-fb5d-445f-b68d-a52d331162fb`.
- Formal installed plugin: `32d5d66ccdd24520c8a9aa5eae092a9e7f60dbcc`; OBS 32.2.2.
- Formal DLL hash remained `e5c5b39c856ba227cfc5b7b0b867e47a6271467fb10e3dfcab9e7cf3566304a9`.
- Platform delivery reported Asset `asset-api--0000103` / `f3d98dedf8bdcd4f618d23c98301b5adeac9b20f`, CMS `hhc-web-api--0000226` / `4e8cd4343d2580373102e63ae8a0bcd32ac3ab3a`. These are handoff versions, not independent deployment inspection in this run.

## Result: normal closeout, natural publication and VOD playback passed

All times Asia/Taipei UTC+8 on 2026-10-18. Service events and client observations are distinguished.

| Milestone | Time / evidence |
| --- | --- |
| Stop accepted, service receipt | 12:05:49.202945612 |
| Last confirm accepted, service receipt | 12:06:47.120202 |
| Seal accepted, service receipt | 12:06:55.335311 |
| Package validation attempt 1 started, service field | 12:07:28.476523 |
| Media ready, service `readyAt` | 12:30:33.658388 |
| Selected cover observed ready | 12:32:08.008; other pending candidates also present |
| Last observed draft response | 12:33:04.204, version 3 |
| First observed published response | 12:33:15.723, version 4, `autoPublish=published` |

Seal to media ready: 23m38.323s. Media ready to first published observation: 2m42.065s; this is not the exact publication event or an all-prerequisites-ready interval. Mac should inspect thumbnail eligibility and reconciler timing. Attempt 1 progressed through 1080p, 720p, 480p; no second attempt was observed.

1357 objects confirmed, zero pending; 450 segments per rendition. Preset `obs-hls-10s-v1`; frame rate 30000/1001; source duration 4502.164333s. Inventory digest `3a5b87785431934cc9d7897884dfb037a6355e0614c29ad30e7ae3880357fbba`.

The same member URL opened while processing transitioned to a playable VOD without reload. Muted playback started through the UI. At 12:33:28.523, actual 854x480, 183 decoded frames, currentTime 5.927569s, readyState 4. After seeking through the UI to 120 seconds, at 12:33:55.069 actual 1920x1080, 978 frames, currentTime 125.42153s, duration 4502.164334s, paused=false, rate=1, MediaError=null; buffered [0,250.249999], seekable [0,4502.164334]. Playback was paused after verification. This is a short playback/seek check, not a complete-player or continuous-viewing retest.

## Live monitoring and optimization scope

Five-second sanitized journal samples are in `artifacts/formal-live-monitor-20261018/samples.jsonl`, summarized in `five-minute-summary.json`. Early pending counts were 0–3, later mostly 3–6, with a peak of 9 in the 12:00–12:05 window. At 12:04:43.275, producer442 / common confirmed440 / verified436. Both upload and verification continued; do not describe pending as always zero or infer viewer latency directly from sequence differences.

Investigate stage timings before changing concurrency: declare/sign/PUT/confirm, verification queue and worker handoff, decode and playlist publication, then full package validation/cover/publication separately. Preserve complete validation and public cutoff. Whole-run player latency and controlled backlog recovery were not measured here; historical failures remain unchanged.

The compact status-bar candidate is documented separately in `windows-compact-status-2026-10-18.md`. It was installed only into isolated OBS; formal OBS and its executable hash were unchanged. No new stream was started for UI verification.

The recurring monitor was paused after successful closeout and `monitor-stop` was written. All recording media and evidence are retained.
