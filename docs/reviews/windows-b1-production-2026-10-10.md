# Windows B1 production acceptance

User confirmed no meeting in progress and authorized actual OBS testing. Contract acknowledgement remains B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory 1, 30000/1001 fps and 900-frame/30.03-second segments.

## First qualified candidate: failed encoder start

- Plugin: `29dc37050f9d1dc317ec62a1763f09839b54d665`.
- CI ZIP SHA-256: `6304902f3903dbb446625fdf8ddda58df725db4afea866c8fa996457c768f5ab`.
- Current CMS release run `38060911650` completed successfully before testing: commit `ae3b6fb5d36ca978c00e6bc269074ba7773d3365`, revision `hhc-web-api--0000181`, healthy, writer enabled and 100% traffic. The earlier `0000180` smoke is not evidence for this revision.
- Other read-back revisions: Asset `0000089`, Website `0000193`, Gateway `0000202`, Account `0000160`.
- OBS 32.2.2 portable HHC-Fixture, Program text clock, audio track 1. Original global stream and recording inactive; no YouTube changes.
- Broadcast/recording: `d48eeb42-bced-4e5b-9444-691a986fbdf1`; manual start/end, auto-publish intended. Announcement accepted; binding reached private preview.
- Bind UI invoked at **2026-10-10 23:31:18.668 Asia/Taipei (UTC+8)**. Capture `70c1b3ef130e88565f3ba1e8830e0615`, local session `6402289e-dde1-482b-a217-0e80a43787a6`.
- OBS error at **23:31:20.835 UTC+8**: `Capture directory must be new`. No media produced; no public start/end ACK. C1 abort observed at **23:31:24.801 UTC+8**, pending bytes 0, auto-publish cancelled. Console reports `capture_incomplete`.
- Root cause: `SessionStore::isPrepared()` allowed C1 metadata but rejected the durable `broadcast-journal.json` created by B1 before initial encoder start. Mock binding tests did not previously exercise this preparation gate.
- Regression test failed specifically on the real B1 journal plus adopted C1 journal preparation assertion. The minimal fix adds that exact metadata filename; unknown files, existing media directories, links and non-pristine local journals remain rejected. Fresh build and all **14/14 local CTests passed** after the fix. This is local evidence, not production acceptance of the replacement package.

Retained metadata SHA-256:

| File | SHA-256 |
| --- | --- |
| broadcast-journal.json | 54c6f17c3a79b515386b755b3784101e106ffb3056dfabd05d5faf8ec86b4b41 |
| journal.json | ae4c87e1220fa213be5de0158bdd6958fee031436d646d32b32eb503a76aae77 |
| local-session.json | 2ae8eed2e166b6635c5baa9ee3672f3a605c2d2d20e1b70538e868136c06fa78 |
| remote-journal.json | f5f65dc03eb9835362274d6144254dd82058fc9dd8e056e4850b83e6f48c4852 |

Runtime preparation also found an old developer fixture DLL registering a duplicate HHC output. It was not executing a fixture capture, and the first-start failure is explained by the preparation gate above. Remove the unused fixture from the owned portable runtime before retesting the normal package; preserve it outside the plugin load directory.

Still unqualified: actual preview decode, start/end ACK and public range, crash recovery, stop/seal/ready/publication, member-player quality/DVR/ABR, normal-load automatic recovery, outage stress and current-version 2.5-hour continuous playback. Historical approximately 379/388-second delay failures remain failures. Unsigned package; Safari native HLS/fullscreen untested.

## Second candidate: encoding works; ACK receipt incompatibility found

- Plugin `9825d91b3ea88cdd885fd1624fe12689ff9a03fd`; local ZIP `bb41513c6b5b6585c68a3d977cd6f0db8cff3c4be6385ca6f54c907597a6f6e4`. Both hosted native runs succeeded: push `38064255788`, PR `38064258758`. Local and hosted ZIP bytes are separate evidence.
- New recording `9a39862a-1034-49c7-ba9b-ef9ca075e834`, capture `d4f89e3c675a53459b8255214f8d2c3a`, epoch 1, local session `56d6be00-3779-4459-85fd-624a3012dbf7`.
- Bind invoked **23:38:18.872 UTC+8**. Three NVENC encoders started at **23:38:21.055/.104/.146**: 1920x1080/3000 kbps, 1280x720/1500 kbps, 854x480/800 kbps; AAC stereo track 1, 128 kbps. No CPU fallback.
- Before start, member page observed **23:39:43.639** with waiting text and zero video elements.
- Staff preview-access returned 200, but `media.alive.org.tw` cookie endpoint **OPTIONS preflight returned 403 with no Access-Control-* response headers**, preventing preview decode. This is a separate platform/browser-path failure, not an OAuth API 401. No exchange credential or media URL retained.
- Original start command `df91a718-bd5c-44da-8c55-3f8aa836c8f9` durably selected boundary **8**, while the C1 verified sequence was 5. Platform read-back showed startSequence 8, same capture/epoch, but native journal receipt remained null and control paused with `broadcast_local_conflict`.
- Diagnostic replay sent the exact original body/key, preserving marker 8. Production receipt has operation `ack`, original operationKey, state `accepted`, **no commandId**. The pinned OpenAPI requires operationKey/operation/state only; commandId is optional. Native ACK and journal-load checks incorrectly required it. Minimal fix permits absence while continuing to reject a mismatched value when present; request path/key and capture/epoch/account fences remain unchanged. RED reproduced optional receipt/restart failure; GREEN full local suite 14/14. Independent focused review found no actionable issues.
- Member actual Chromium decode at **23:45:10.393 UTC+8**: 1920x1080, currentTime 42.100251, duration 60.090666, buffered [0,60.059999], seekable [0,60.090666], 371 decoded frames, 0 dropped, playing, 1x, no media error. This establishes short live decode only; it is not recovery/ABR/full-range acceptance.
- Normal HHC Stop dialog opened **23:47:32.382**, then confirmed. Platform accepted stop **23:47:45.630 UTC+8**; at **23:47:46.091**, 57 declared objects, 54 verified/3 queued, liveState ending. Global OBS streaming/recording remained inactive. Manual Console End was not qualified on this candidate because control had paused.

The second failure is a Windows contract-consumption bug. Staff preview CORS needs Mac/Worker investigation; no platform files were modified. Subsequent acceptance must identify the replacement commit and package; do not attribute it to the original pinned candidate.

## Replacement native candidate and original-session recovery — October 11

All times below are Asia/Taipei (UTC+8), unless explicitly marked otherwise. Local UI observations and server acceptance times are separate clocks; retain their measured values rather than forcing them into one ordering.

- Encoding/control candidate: `fe61bb12eae6376578911f17742ef329a4cc8b4b`; local unsigned ZIP SHA-256 `b481c5d711f09997eabdfc2f4a143a3fef2c90aac0b40bac7b95b9289d21b08a`. Hosted push `38065116633` and PR `38065119718` both succeeded. The original `29dc370` ZIP is not this replacement package.
- The already normally stopped `56d6be00-3779-4459-85fd-624a3012dbf7` session was resumed through the native selector/button at **2026-10-10 23:55:51.808**. The original start ACK intent/key/boundary 8 was replayed and its optional-commandId receipt persisted. No new C1 capture or inactive encoder marker was invented.
- Its seal was accepted **23:55:51.877182**; local pending bytes reached zero at **23:56:37.499**. Original 64 objects were verified and capture ready observed **23:59:28.261**. All three renditions retain 19 segments / 565.097867 seconds / 30000/1001 fps; 18 full 30.03-second segments and a 24.557867-second tail. Original hashes match.
- Because the earlier candidate never ACKed a Console End, C1 ready alone did not close the B1 live range or prove auto-publication. Emergency-close was invoked only for that synthetic failed test at **2026-10-11 00:00:27.326**. This is cleanup, not successful normal End acceptance.

## New native Start / End / Stop / seal / ready

Recording/broadcast `e9c5ebb4-74d7-46e0-a7b4-e3730579fc8a`; capture `e818d9764d7997b1e04deccf07961b49`; epoch 1; local session `c0ac9aa7-8fa4-4b2d-895e-45a41841890a`. Console manual Start/End and auto-publication were selected; no platform edits or manual publication were made.

| Milestone | Time | Actual result |
| --- | --- | --- |
| Create / announce | 00:01:28.478 / 00:02:00.559 | Announcement is separate from public start |
| Native bind UI | 00:02:42.616 | Original B1 binding adopted, no parallel C1 capture |
| Three NVENC encoders | 00:02:44.927 / .976 / 00:02:45.017 | 1080p / 720p / 480p, audio track 1 |
| Before Start | 00:03:32.585 | Member waiting page, zero video elements |
| Start UI | 00:03:32.586 | Native command `3f734393-07e6-4a22-9e60-dd94b65e2ff7`, durable next encoder boundary 2, ACK accepted |
| Member Play / actual decode | 00:05:59.847 / 00:06:17.861 | 854x480, currentTime 17.051157, duration 90.126666, 520 frames / 0 dropped, playing 1x, no error |
| Console End confirmation | 00:06:25.995 | Native command `1e640777-2802-4eee-8461-950545e8db5c`, durable endExclusive 8, ACK accepted |
| HHC still encoding after End | 00:07:21.233 | HHC active; global stream/recording remained inactive |
| HHC Stop confirmation | 00:07:45.514 local UI | Server accepted **00:07:44.848755863** |
| Seal accepted | 00:08:44.579015 server | Original inventory/key, digest below |
| Ready | 00:10:48.406 local; 00:12:33.929 server readback | 40/40 objects verified, capture ready |

The original member page progressed from waiting to a player without reload. Initial Play was an explicit browser gesture; this is not an outage recovery test. Decoded buffered range was [0,90.089999], seekable [0,90.126666].

- Public boundaries remain **[2,8)**, 180.18 seconds. Later Stop did not expand the accepted End. This proves native ACK/readback boundaries, not authorization/decode of every public and private object.
- Full capture media remains **11 segments per rendition / 300.566933 seconds / 30000/1001 fps**, ten full 30.03-second segments plus a 0.266933-second tail. Inventory digest `3e99e25aa80aded1959bc221571a04d32b44ee4a0b79bb34a712774260c32274`, 40 declared objects, 199807226 bytes. Original media hashes all match. Existing local verifier passes hash/grid/cross-rendition checks and full CPU decode of all three renditions. This local check is distinct from the production validator's 40 verified objects / ready result.
- B1 phase/archive remained processing after ready. C1 autoPublish remained pending at server **00:22:23.133711798**. CMS recording metadata remained draft with `selectedCoverId=null`; no manual Publish or cover selection was used to conceal the pending state.
- Live JPEG bytes SHA-256 `7149154592890b5a8c3bab813c1e879c96cece57fc97961f26f155382e9ac67a`. Admin VOD image bytes observed **00:29:26.470** have selected-preview SHA-256 `7d10d879b03af64959ba15728632017fc8ad4065dcd65d38851f2d16acca86d7`, different from the live image. The UI displays auto-cover 1 selected, while authoritative CMS metadata has no selectedCoverId. The other two visible JPEGs hash `60da7d53044c0cf2089c6e164fd43b6704e30e4068186e7fcda2eefaf5231623` and `0ca9b1cc56d50dcb2f7f5ebb11c215aff9acc44a356e6115998c80b2e68779a9`. Empty Fetch bodies are excluded from image-byte evidence; hashes are from actual Image response bodies.
- Staff preview again fails independently: at **00:05:33.673**, preview-access 200 then media cookie OPTIONS **403**, no Access-Control-* response headers, Cloudflare ray `a486ddb35afb2566-TPE`. Actual preview decode is unqualified.
- Original member page changed to **「直播回看期限已到」** at **00:10:03.364** and still shows that text without reload, despite C1 replayUntil **00:44:42.973070**. Preserve this mismatch for the platform owner; C1 expiry alone does not identify the website's authorization/root cause.

## Current candidate UI and real process interruption

`b259fa747f5454d0c1d9c4bab686484d36646a54` changes only the B1 Stop / ready text to defer publication to Console. Standalone C1 and local-only behavior are unchanged. The existing dock regression reproduces the misleading B1 text before the fix; fresh build and all 14/14 local CTests pass. Focused review found no actionable findings. Hosted push `38066593938` and PR `38066599817` both succeeded.

Local unsigned ZIP SHA-256 **`18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`**. Both installed DLL and helper match this package and embed b259fa7. This is a local candidate hash, not a hash of the hosted artifact.

Downloaded hosted candidate from push run [38066593938](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/actions/runs/38066593938), artifact `hhc-obs-windows-x64-unverified`, ID `11674729815`: actual inner product ZIP SHA-256 **`351cd9de0b0cf8b257003da609a01ce448f70f0f2541980b001a020e5cc9ed88`**. All seven manifest file hashes match; DLL/helper both embed b259fa7, fps 30000/1001, unsigned. This downloaded CI product was inspected, not installed for the real crash test above. GitHub's outer artifact archive digest is a different object and is not substituted for the product ZIP hash.

- New never-public test: recording `744f5b7e-f68a-478b-9670-b340260417c8`, capture `91f5fbea6a20c38a6be60d18d7b21cdc`, local session `6a531eeb-bbdf-430c-aabe-b85852ebd9de`. Manual Start/End; no Start was issued; Console auto-publication option was left true to check that incomplete never-public media cannot be published.
- Native bind invoked **00:23:38.012**; NVENC settings logged **00:23:39.924 / .976 / 00:23:40.017** for the three renditions. Before interruption, nine journal objects and twelve actual media/playlist files were hashed; `normalEnd=false`, `stopIntent=false`, no control commands.
- Only the identity-checked dedicated portable OBS PID 45460 was forcibly terminated **00:24:59.887**. It was restarted in ordinary mode **00:25:00.007**, PID 26828. Original installed OBS was not running or modified. This launch did not display an OBS safe/normal-mode chooser; do not claim that separate UI branch passed.
- Native recovery scan retained the original session; GUI Resume and the explicit incomplete-capture abort confirmation were accepted **00:28:09.232**. The scan checks existing retained long media too, so the initially empty list while scanning was not treated as lost data.
- Server **00:28:32.119457391**: original capture **aborted / capture_incomplete**, autoPublish cancelled, stopAcceptedAt null, six verified / two queued / one declared object. Native recovery retained the same recording, capture, binding and original bind intent/key. All **12/12 original file hashes** match at **00:28:46.060**; normalEnd/stopIntent/confirmedReady remain false, zero Start/End commands and no seal. This is successful safe abort/preservation after a process crash, not continuous encoder resume or a normally stopped never-public rehearsal.

## Separate player observations and remaining gates

On the earlier 9a39862a test, actual 480p decoding resumed after a UI backward seek; a paused manual 480p → 1080p switch retained currentTime **77.561462**, duration **324.857867**, paused=true and rate=1.5. Before/after measurements at **2026-10-10 23:57:37.151 / 23:58:07.692** decode 854x480 / 1920x1080. This is partial player evidence; last-frame retention, rapid switches, natural-end regression, ready VOD and controlled weak-network ABR remain unqualified. Browser screenshot capture repeatedly timed out; actual native PNGs and structured browser/network/decode measurements were retained, and nonexistent browser screenshots are not represented as evidence.

No normal meeting-load follower outage/recovery, five-minute outage/ten-minute catch-up, or new b259fa7 2.5-hour run is qualified. These await A–D platform-path acceptance; the historical approximately 379/388-second failures remain failures. Previous-version long-run results do not qualify this source. Original nine OBS profile/scene hashes remain identical; no YouTube changes, merge, signed release or deployment.

Later read-back **00:33:31.621** confirms B1 archive has reached **ready**, but phase remains **processing**, with the same capture/epoch, boundaries [2,8), revision 5 and no pending command. Console says recording complete / waiting publication. The original ready session was resumed through b259fa7 GUI at **00:33:22.302** without restarting an encoder; it keeps pending bytes zero and waits for platform publication. This does not qualify publication or VOD playback. A transient Console 401 after the prior browser session expired was resolved by ordinary reload before the crash-test draft was created; it is not an outstanding Windows OAuth blocker.

Ignored local evidence: `artifacts/b1-production-20261010/` contains pinned contract/runtime hashes, original profiles, candidate ZIPs, native journals/receipts, media checks, decode measurements, CORS failure, cover hashes, crash before/after/status and native PNGs. It contains no token, password, cookie or signed URL. See the [Mac follow-up](windows-b1-mac-blockers-2026-10-11.md) for the remaining production dependencies.
