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
