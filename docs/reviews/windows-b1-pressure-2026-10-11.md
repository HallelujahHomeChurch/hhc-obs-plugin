# Windows B1 five-minute transport recovery — 2026-10-11

Acceptance is in progress. This report does not convert previous 379/388-second recovery failures into passes. All event times below are Asia/Taipei (UTC+8).

## Fixed candidate and scope

- Exercised product commit: `b259fa747f5454d0c1d9c4bab686484d36646a54`; unsigned local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`.
- Installed product DLL SHA-256 `885a692bda4a32d64bbe74fe5f273b3374b81de4c0df2505562a7166611d1a6b`; helper SHA-256 `91490295a4a77ca7375ce3be32631c0c4d389c162999eb4bdd2afcff8f8afd7c`. Later repository commits concern evidence and a developer-only fixture, not replacement product binaries.
- Developer-only fault fixture SHA-256 `9016eeeba6f8122a3ce7dc1b7cf0e9bb21ff7b117a41abcf4bf6f92d9d20e9be`. It changes the OBS process's Qt application proxy temporarily. It does not change OS networking, browser connectivity, firewall rules, or the shipped package.
- Fixed Mac blockers handoff `6a607af33b5de3f0a9258e3c52e1ce1d180c538f`; document SHA-256 `1f7ef3b8a3fe64c738a225565040eeb7a134bece1f2b95589f32be03a9940a3d`. Immutable C1 artifacts were verified this round: B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory schema 1 unchanged.
- Actual production versions read this round: CMS `hhc-web-api--0000182`, Asset `asset-api--0000090`, Website `hhc-web--0000199` / source `46ea3802c8e83e5725eea163b408a7fe9476af05`. Do not attribute these observations to the earlier Website revision 0000197.
- Portable OBS 32.2.2, synthetic Program clock, three NVENC renditions, track 1, 30000/1001 fps, 900-frame/30.03-second segments. Concurrent portable global HEVC recording is enabled. No actual YouTube broadcast; physical outage and normal meeting/YouTube load remain unqualified.

## Capture and milestones

Recording/broadcast `9983fe7b-3f70-4829-98cd-57e0e1c4fb80`; capture `6d5466c1240fc6bfe5ef97ec3be01583`, epoch 1; local session `0d85c189-867b-4f15-9faa-cd8ab7e85166`.

| Time | Observation |
| --- | --- |
| 13:48:40.637 | OBS bind click; a fresh B1 field test, no parallel C1 capture. |
| 13:48:42.613/.658/.705; .734 | 1080p/720p/480p NVENC and AAC initialization. |
| 13:48:48.129 | Concurrent portable global recording starts. |
| 13:49:25.865 | Console Start requested. Command `c1901e9f-a1e9-4152-96aa-ec8e4fe13631` uses durable next-common boundary **2** and accepted original-key ACK. Member media start is 60.081 seconds. |
| 13:51:38.506 | Initial Play gesture on the original member page; it is retained without reload or manual Back to Live. |
| 13:52:18.584 | Actual 1920x1080, currentTime 39.282423, 1,187 frames, zero dropped, no native media error; estimated delay 116.486577 seconds. |
| 13:53–14:02 | User temporarily took computer control. Passive probe cadence and decoded frame count show background throttling; this interval does not qualify as continuous foreground decoded viewing. No fault was injected during handoff. |
| 14:02 | On handback, delay was already about 209 seconds, before fault injection. It subsequently recovered automatically without UI input. |
| 14:04:26.744 | Follower: actual 1920x1080, currentTime 794.139477, duration 810.851999, 6,724 frames/10 dropped, no media error; delay about 89.79 seconds. Separate DVR page remains intentionally paused at 0 seconds/1.5x. |
| 14:05:05.310 | Qt transport block begins after manual test trigger. |
| 14:09:39.472 | Follower is waiting at currentTime 900.856107, duration 900.921333, 9,913 frames/17 dropped, no native media error. Platform view is interrupted; published watermark sequence 31/mediaEnd 960.96. |
| 14:10:05.348 | Original Qt proxy restored, receipt confirms restoration. Actual fault duration **300.038 seconds**. |

Ten-minute gate: **14:20:05.348 UTC+8**. Gate evidence must distinguish durable upload receipts, platform object states/verification watermark, playlist/media edge, and actual decoded member position. Automatic recovery pass requires return to 60–120 seconds within this gate. A future final publication does not establish this pass.

Delay is calculated as wall time minus `(encoder AAC start + member mediaStartSeconds + currentTime)`. This is an approximate capture-clock delay; the 21 ms AAC offset is retained. Safe raw observations live in ignored `artifacts/b1-pressure-long-20261011/`. Local object `confirmed` flags are not upload ACKs; pending counts derive from accepted remote-journal confirm receipts. A zero-byte integration status frozen during transport loss is stale, not proof of an empty queue.

## Ten-minute result

- First valid sampled pending-receipt zero: **14:19:36.5841534**, 186 objects / 186 accepted confirm receipts, **571.2361534 seconds after restoration**. Continued encoding subsequently creates ordinary new pending bundles; a single zero does not imply no more uploads.
- Gate-nearest platform sample: observed 14:20:05.6228641, serverNow 14:20:04.204785734, 189 declared objects, **183 verified / 6 queued**, liveEnabled=true, state=uploading. The local integration sample at 14:20:02.699 is a separate observation: lastSequence=59 and 3,025,474 pending bytes. That sequence is not substituted for a verified media watermark.
- Journal bracket: 14:20:01.7077015 has 189 objects / 188 accepted receipts / one 3,025,474-byte pending object; 14:20:06.7233764 has all 189 accepted / zero pending.
- Original member probe brackets the actual gate with **no video** at 14:20:01.918 and 14:20:06.920. At 14:20:17.197, the rendered page has the real visible playback failure, not just inaccessible video fallback text. It was never reloaded or manually returned to live. Actual decoded resolution/currentTime/frame count/latency are unavailable at the gate; do not fabricate them or infer success from liveState.

**Overall automatic player recovery gate did not pass.** Upload receipt catch-up occurred before ten minutes, but playback is confounded by a new browser/Service Worker API transport failure. The last original app watch watermark retained before this failure is revision32/sequence31/mediaEnd960.96, and is stale at the gate. No fresh original-page verified-watermark or playlist-edge claim is made. This is not sufficient evidence to blame the new worker revision for a 379/388-second lag regression.

## Independent browser diagnostic

At 14:11:36.887 the original player automatically resumed; it advanced to the next segment and emitted emptied at 14:12:12.705, then rendered “暫時無法播放，請重試”. Its video element is removed. A separate paused DVR page also lost its video; pause/1.5x preservation cannot be qualified across this failure.

The original page is online and controlled by `https://www.alive.org.tw/sw.js`. Same-origin member watch, account CSRF/session and push/config Fetch requests fail with `net::ERR_FAILED`, no recorded HTTP response or explicit CORS reason. Independent unauthenticated HTTP checks to the three account/push endpoints return 200; these checks establish connectivity, not member authorization or playback.

On a separate diagnostic page only:

1. Normal Service Worker path: account/recording cannot load; API requests fail.
2. Temporary CDP `Network.setBypassServiceWorker(true)` and diagnostic-page reload: identity and member recording load, live player exists and watch API returns 200.
3. Restore bypass=false at **14:18:51.777**, reload diagnostic page: “錄影暫時無法載入” returns.

The original follower/DVR page were not bypassed or reloaded. This A/B/A isolates an interaction with the Service Worker fetch path in the Codex in-app Chromium environment, **not yet the root cause or regular Chrome/Edge behavior**. A user check in their ordinary browser is requested asynchronously. A screenshot attempt timed out; DOM, passive media events and sanitized CDP network records are retained instead. No screenshot proof is asserted.

Current served Service Worker UTF-8 bytes: 6,651, SHA-256 **962aba2dcb581a0963e605eea9cf4b430730f2b8a4505e124c40e4ceeaa99b64** (14:18:43.9331980). Its API/Authorization branch uses `event.respondWith(fetch(request))`; this source observation alone does not establish that the branch is defective. No Website edits, unregister/cache deletion, authentication changes, or permanent bypass were applied.

## Subsequent Windows sync pause and early normal Stop

The product's last successful integration observation is 14:28:44.625 (sequence77, pending0); control observation is 14:28:48.522. Encoding continued but accepted confirm receipts remained at240. The native warning became “同步檢查失敗；保留原始意向及本機資料，請重新檢查。” The installed product does not persist the original non-HTTP exception code, so its first cause cannot be determined retrospectively.

At 14:39:18.539 all241 declared remote object's sizes/hashes match the corresponding retained objects. An initial constructor-only diagnostic validates the remote synchronization journal, not the complete local checkpoint/media. A later, corrected strict targeted inspection at **15:00:57.0575280** exits0 and validates the typed checkpoint, receipted closed media and remote journal, without HTTP or mutation. Neither observation proves that no earlier transient write failure occurred.

The developer journal observer originally used PowerShell Get-Content, which can omit delete sharing while holding an authoritative metadata file. The lifecycle test demonstrates that a reader without delete sharing blocks atomic replacement, while a read/write/delete-sharing snapshot permits it. This is a plausible test-instrumentation contributor, **not a proven historical exception**. Old monitoring was stopped; the observer script now uses FileStream with FileShare.ReadWrite|Delete. Authoritative writes/validation remain fail-closed.

HHC normal Stop confirmed **14:49:17.228**, retaining stopIntent=true and normalEnd=true. The independent portable global recording remained active after HHC Stop, then was separately stopped at **14:49:45.972**. No Console End command/ACK is claimed for this pressure round. The native operator selected the original local ID and resumed it at **14:54:36.971**; Stop acceptance is read back as server **14:54:36.153223714**. Server/client milestones are separate observations; synchronized clocks are not assumed. At server **14:58:05.948778381** the actual state is uploading/ending,272 declared,267 verified/5queued, with628,234,073 bytes pending in the separate local observation14:58:07.056. Same recording/capture IDs are retained. This round's observed autoPublish intent is cancelled; it must not be silently enabled or counted as an automatic-publish failure.

Local full media QA passes all hashes, exact frame grid and complete three-rendition decoding: **122 segments/rendition**,121×30.03+tail1.134467, **3,634.764467 seconds**,373 objects,2,416,205,324 bytes,30000/1001 fps. Inventory SHA-256: **5f18cab8720f9fed6b2954546f85de27491d00b038f22b4af3f83b57579f33e1**. This is about61 minutes, **not a 2.5-hour pass**. Final seal/ready and fault-period member VOD remain pending.

## Windows operator recovery correction

The current-capture recheck previously returned while encoding was active; the broadcast recheck did not clear the paused C1 sync. The correction revalidates the same account/local ID with the existing typed journal/closed-media validation, then resumes the original sync. It fences in-flight sync/control results and terminal/account-mismatched sessions, without changing wire schema, capture IDs, operation keys or encoder behavior. Targeted validation avoids hashing all historical sessions on this retry.

Fixed diagnostic codes cover local queue unavailability, remote-journal commit failure, changed upload bytes and immutable remote conflicts; unknown exceptions have one fixed fallback. Raw exception text is not displayed or persisted. sync-error.json is optional observer evidence, not an authoritative journal. Tests cover missing/non-boolean normalEnd before HTTP/abort, same-length changed-media rejection, same-session remote validation, account/terminal/result fences, metadata reader sharing, and arbitrary secret-URL redaction.

The new retry and diagnostic regression tests were observed failing before their corresponding product changes; independent review found and then confirmed correction of the local-validation gap and overly broad initial diagnostic claim. Full rebuild and **14/14 CTests pass** after correction. The installed OBS product remains b259fa7 at this observation; these results are local unit/mock/source-review evidence, not a new candidate's real OBS acceptance. A fresh fixed candidate and fresh long test are required.
