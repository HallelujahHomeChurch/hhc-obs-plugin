# Mac / browser follow-up: Service Worker API path

This is a bounded diagnostic handoff, not a platform root-cause claim or permission to loosen media/authentication rules. Windows continues independent source/queue/long-duration checks.

Please reproduce in ordinary Chrome/Edge with the **actual deployed Website version and active Service Worker**, then distinguish a Website regression from Codex in-app-browser compatibility. Do not merge or deploy on Windows' behalf.

## Fixed context

- Mac blockers handoff `6a607af33b5de3f0a9258e3c52e1ce1d180c538f`, document SHA-256 `1f7ef3b8a3fe64c738a225565040eeb7a134bece1f2b95589f32be03a9940a3d`.
- This round's verified versions: CMS `hhc-web-api--0000182`, Asset `asset-api--0000090`, Website **`hhc-web--0000199`**, source `46ea3802c8e83e5725eea163b408a7fe9476af05`, release `38107954344`. Re-read actual versions before reproducing; 0000197 is an older snapshot.
- B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, schema1, digest/idempotency/member authorization, 30000/1001 fps and 900-frame/30.03-second segments remain unchanged.
- Exercised product `b259fa747f5454d0c1d9c4bab686484d36646a54`, unsigned local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`.
- Recording `9983fe7b-3f70-4829-98cd-57e0e1c4fb80`, capture `6d5466c1240fc6bfe5ef97ec3be01583`, epoch1; [pressure report](windows-b1-pressure-2026-10-11.md).

## Measured observations (Asia/Taipei UTC+8)

OBS-only Qt transport block **14:05:05.310–14:10:05.348**, 300.038 seconds; browser/OS network not intentionally disconnected. Source and concurrent portable global recording continue. Original member page is never reloaded or manually returned to live before the ten-minute gate.

At **14:11:36.887** the original member player resumes automatically, then emits emptied at **14:12:12.705**. By 14:13:01 the actual visible page says “暫時無法播放，請重試” and has no video. Same-origin member watch/account/session/CSRF/push Fetch requests subsequently produce `net::ERR_FAILED`, without a recorded HTTP response or explicit CORS reason. Navigator is online; the page is controlled by `/sw.js`.

Independent unauthenticated HTTP reads to account CSRF/session and push/config endpoints return 200. This establishes endpoint connectivity only, not member authorization.

Separate diagnostic page A/B/A:

1. Service Worker enabled: recording/identity API loading fails.
2. Temporarily bypass Service Worker **on that page only**, then reload: identity and recording load, live player exists, watch API returns 200.
3. Restore bypass=false at **14:18:51.777**, reload: recording API loading failure returns.

Original follower/DVR page remains unmodified. Screenshot capture failed; DOM and passive media/CDP event evidence are retained. No screenshot claim. Served SW at 14:18:43.933: **6,651 UTF-8 bytes**, SHA-256 **962aba2dcb581a0963e605eea9cf4b430730f2b8a4505e124c40e4ceeaa99b64**. The served source intercepts API/Authorization requests with `event.respondWith(fetch(request))`; the source alone does not prove a defect.

Upload catches up at **14:19:36.5841534**, 571.2361534 seconds after restoration, all186 accepted confirm receipts. At the ten-minute gate **14:20:05.348**, platform-nearest sample has **183 verified / 6 queued / 189 declared**, but the original member page has no video. The full player-recovery gate is **not passed**. Actual gate decoding/currentTime/latency cannot be measured. Do not equate server liveState with player recovery or claim a worker throughput regression from the stale original watch watermark.

## Requested next evidence

- Compare ordinary browser API behavior with its active SW and deployed version, including installation/update/clients.claim during an already-open member page. Windows requested the user's ordinary-browser check; its result is still pending when this document was written.
- Inspect matching Website/Gateway/account/member requests and frontend error telemetry around 14:12–14:20; distinguish HTTP status, network failure and worker interception. Do not export credentials, cookies or signed URLs.
- If ordinary browsers reproduce a Website defect, provide a fixed commit/report, tests and actual deployment confirmation before formal Windows retest. Preserve SW no-cache/authentication intent and all B1/C1/media validation rules.
- If ordinary browsers pass and this is specific to the Codex in-app browser, state that limitation and supply an ordinary-browser verification route; do not ship a permanent SW bypass as an OBS fix.
- Keep short recovery, five-minute pressure, normal meeting/YouTube load, long source encoding and 2.5-hour continuous member viewing as separate results. Previous 379/388-second failures remain failures.

When finished, return a copyable Windows continuation prompt with the fixed report URL/hash, actual deployment versions or a confirmed unchanged deployment, verified browser conditions, and remaining acceptance steps. Windows needs this concrete evidence; local/CI success alone is not formal playback acceptance.
