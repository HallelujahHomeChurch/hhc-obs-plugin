# Mac follow-up correction: quality-source rebuilding is not a visible error

The earlier request to fix a visible "無法播放媒體。" prompt is withdrawn. Chromium exposed that text as the video accessibility fallback during source rebuilding; it was not demonstrated on the rendered player. Please do not implement a platform or Windows workaround based on that report.

## Fixed execution context

- Exercised Windows product source `b259fa747f5454d0c1d9c4bab686484d36646a54`, local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`.
- Website actual `hhc-web--0000199`, source `46ea3802c8e83e5725eea163b408a7fe9476af05`; CMS 0000182, Asset 0000090. These are distinct from the handoff's old Website 0000197.
- Recording `e38353c4-ce2f-4851-a21c-89b3f0afcd97`, capture `9bb8071a479d63385f95623c4b6a4718`, epoch 1.
- Public [7,18), nominal 330.33s; private capture 580.246333s, 20 segments per rendition, 30000/1001fps and 900-frame segmentation. All 67 objects verified, ready, auto-published/version 3, B1 archived/revision 13.

## Corrected evidence (Asia/Taipei UTC+8)

The original sparse measurements at 12:29:51.679, 12:31:33.350 and 12:33:10.366 remain in the local evidence: temporary zero video dimensions/time, null duration and native error null, followed by successful restoration. They do not prove visible error text, a decode failure or the duration of source replacement.

Passive events and existing held-frame canvas measurements now show:

| Time | Observation |
| --- | --- |
| 13:06:07.485 | Paused 1.5x, 720p -> 480p emits `emptied`; error null; no visible error text or alert. Held canvas 1280x720 remains displayed with 68,793 nonblack pixels. |
| 13:06:08.113 | `loadedmetadata` at actual 854x480; the same held image remains displayed. |
| 13:06:08.180 | `loadeddata` restores position 49.812211, paused=true/rate=1.5; then the canvas is cleared. |

The held RGBA pixels have SHA-256 `e9401157664309d376dec5a2026ffda1ceadaa6486fee80b71dec5babfc21ae7` at both pre-decode events. No media error event occurred. An earlier 1080p -> 720p probe likewise restored within about 0.9 seconds. Evidence is `artifacts/b1-retest-20261011/quality-frame-probe.json`; passive listeners were removed afterwards. Screenshot capture timed out; no screenshot proof is claimed. This supports frame retention for these paused transitions, not all rapid/playing-state combinations.

## Separate passes and remaining gates

Staff preview truly decoded; member credentials against staff preview were rejected HTTP401. The original member page continued without reload inside replayUntil13:01:09.084525. Start/End ACKs use common encoder boundaries 7/18; later Stop preserves [7,18). Global OBS recording survives Console End and HHC Stop. Selected VOD cover1 JPEG equals the live image: SHA-256 `9e8cbc96fb16db5d88c3e4c02a4e6081de1dea69b33d37b7e5a2b71a8e8317f0`, 62,713 bytes. Natural 480p end -> backward seek -> 1080p -> Auto recovers; manual qualities preserve position/pause/1.5x/full duration. Controlled 1.1Mbps/120ms Auto plays 480p, then eventually reaches 1080p after 100Mbps/20ms restoration; controlled downshift is now separately observed in the recovery report.

The qualified 65-second Qt transport recovery and controlled Auto downshift are recorded in [the separate recovery report](windows-b1-recovery-2026-10-11.md). Actual physical producer-network outage, normal meeting load with YouTube, five-minute/ten-minute pressure, current fixed 2.5-hour acceptance and remaining authorization checks are independent gates. Historical 379/388-second recovery failures remain failures. Normally stopped never-public rehearsal now separately passes; no Start/End, all19objects ready but CMS remains draft/autoPublish cancelled/member no player. The false visible-error classification neither creates a new Mac blocker nor converts those old failures into passes.

Full ledger: [Windows B1 retest](windows-b1-retest-2026-10-11.md). Preserve original raw measurements and distinguish rendered text, accessibility labels, media errors and actual decoded frames. No tokens, cookies or signed URLs are included.
