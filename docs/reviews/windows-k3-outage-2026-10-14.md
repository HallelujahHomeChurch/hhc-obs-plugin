# K3 isolated HTTP outage field

This is a real OBS / production-platform field with developer-only transport
fault injection, not a physical network test or an unmodified candidate pass.

- Source: `6966356eee72188897b0b780c2451547bb5875d5`, fault variant installed
  only in portable `HHC-Fixture`. Native startup logs identify the source and
  the non-distributable network fault warning.
- Recording: `f6d3c5a8-d79b-41a6-add1-268eaccaa6e0`.
- Capture: `44e833a888b7bb57c3ee6d40612192ca`.
- Local session: `d4f4df61-38a7-4295-a861-ac913c79431d`.
- K3/B1/C1 unchanged; 30000/1001 fps, 900-frame / 30.03-second segments.
- Original Console tab 12 and member tab 13 retained without reload.
- Source started approximately 12:25:31 UTC+8; separate isolated local OBS
  recording started approximately 12:28:30. Original installed OBS and
  YouTube were not touched.

At 12:29:22.365 UTC+8, member playback decoded 1920x1080 at 1x, playing,
time 70.977001 / duration 120.149333, buffered `[30.03,120.119999]`, seekable
`[0,120.149333]`, 336 frames / 0 dropped, no media error. This is baseline
playability, not a normal-latency acceptance result.

HHC HTTP was blocked at **12:29:31.3631165 UTC+8**, with recovery scheduled
no earlier than 12:45:31.3631165 (16 minutes). Closed media then contained
seven segments per rendition; all 24 objects had confirmation receipts and
no closed bytes awaited confirmation. Last locally observed platform
verification sequence was 3. Browser/host networking remained enabled.

At 12:31:50.525, every rendition had 12 closed segments and 99,811,165 bytes
awaited confirmation. No normal end, Stop receipt, seal receipt or new
capture existed. Journal serverNow remained the pre-cut value and must not
be treated as a current platform status while offline.

At 12:31:57.045, original member page showed interrupted/loading and retained
1080p at time 210.186673 / duration 210.239999, paused false, rate 1,
readyState 2, buffered `[62.062,210.209999]`, seekable `[0,210.239999]`,
4500 frames / 0 dropped, media error null. No reload or return-to-live action.

Field still in progress. No reconnect, session-renewal, catch-up, finalization
or post-outage VOD pass is claimed yet. Safe samples/timing/binary hashes and
native screenshot are retained under `artifacts/k3-20261014/`.

Default (fault-disabled) build: 18/18 CTest pass; CI 38511147122 and
38511141399 pass. Normal unsigned ZIP SHA-256:
`728e8120d79b061e0f48977856e2a191e04fd9bc705ff6ab841d53cc63f6ffde`.
Fault build: 17/17 CTest pass, independently hashed, never packaged. The
normal DLL does not contain the outage-marker variable; fault DLL does.

## Recovery and controlled end

Transport restored at **12:45:47.3809476 UTC+8**, after **976.017831 seconds**. Every rendition had 40 closed segments, 123 closed objects / 24 confirmed, with 658,754,080 bytes pending. No user retry, login, reload or return-to-live action was used.

By 12:46:57.229, original-capture server status was fresh and confirmed objects increased to 26. This is real post-expiry renewal/reconnect/upload continuity, with no replacement capture. At 12:48:16.361 the original member page resumed actual 1920x1080 playback: currentTime 241.251033, duration 300.330666, playing at 1x, readyState 4, buffered `[122.122,300.299999]`, seekable `[0,300.330666]`, 5441 frames / 6 dropped, error null. Its UI estimated 1121 seconds behind; that is a badge, not an independently calibrated wall-clock measurement. Catch-up to normal latency did not pass before this field was ended; this 16-minute outage is not the separate five-minute/ten-minute pressure benchmark.

Console End was sent around 12:48:45; durable ACK chose **47**, while verified media was far behind. HHC source stopped at **12:49:06.769**. Original Stop receipt accepted at **12:49:11.967361508**, original operation key retained. Public `[0,47)` = **1411.41 seconds**, not all source material. The separate local OBS recording continued through 12:50:07 screenshot and was stopped explicitly at **12:50:17.560**.

Local full SHA-256/codec/decode/frame-grid/cross-rendition QA passed for all three renditions: 48 segments each, **1411.9105 seconds** source duration, final tail 0.5005 seconds, 30000/1001 fps. The extra tail beyond End is not public. Five original installed OBS settings hashes remain unchanged.

At 12:51:01.309, 567,959,932 bytes still awaited confirmation. Stop was accepted; seal/ready/publication remain pending. Do not call the field finalized from local decode success.

Windows-side throughput observation: `CaptureSync::step` selects one object per successful step; the K3 dock waits three seconds between steps and the detached helper waits ten. These are concrete pacing costs worth a separate measured optimization, not proof that all historical lag came from Windows or the platform. This fixed field is not modified during drain.

The completed revoked draft `a067dd3a-0d0e-4e50-9f03-7f1d1dbad685` was deleted under standing test-cleanup authorization, verified absent from Admin and member lists. Its local journals/media/diagnostics remain. Current acceptance and protected originals are retained.

First post-restoration upload confirmation was accepted at **12:46:45.022850 UTC+8** (about 57.642 seconds after restoring transport), for `1080p/seg-000007.m4s`. At 12:55:53.858 (about ten minutes after restoration), 279,810,012 bytes remained. This is a 16-minute outage with End at 12:49; it must not be compared as if it were the unchanged five-minute-outage / ten-minute-live-catch-up benchmark. No 2–3-minute steady-live-latency pass is claimed.

## Final local/transport checkpoint

Last object (`master.m3u8`) confirmation accepted **13:01:36.085640 UTC+8**; seal accepted **13:01:42.077690**, using the original seal operation key. All **151** inventory objects are confirmed (150 rendition closed-receipt entries plus master). Restoration-to-final-confirmation: **948.704692 seconds**; this includes continued source encoding until End. At 13:01:53.100 pending closed bytes were zero and state freezing. At 13:02:57.302 the platform reported validating. **Ready was observed at serverNow 13:06:16.239402994**, a seal-to-ready observation bound of **274.161713 seconds**. This is actual platform ready, not worker exit status.

Original member playback at 13:04:58.472 decoded 1920x1080, time 1243.355940 / duration **1411.41**, playing at 1x, buffered `[1113.112,1381.379999]`, seekable `[0,1411.41]`, 35474 frames / 6 dropped, error null. No reload or manual return-to-live occurred. Full public duration is correct, including the outage interval; this remains ended-live playback until the recording is published.

Displayed live auto-thumbnail: 1280x720, 64,259 bytes, SHA-256 `b0a8ff01b5a9741091e6b0a197cc7d9dbe4d3456565013cceee378c94bda989f`. At the post-ready Console check, autoPublish remains checked and the UI says recording complete / waiting for publication. Publication and selected VOD thumbnail-byte comparison are not yet passed. The Console also still instructs the operator to stop OBS despite accepted Stop/seal; no duplicate Stop was sent to satisfy that text.

Isolated OBS and its helper exited normally after ready. Restored the pre-test `33be386` DLL/helper, both hashes match the saved candidate backup. No marker or test clock remains running, and only the unrelated original OBS process remains. No cron or network/firewall setting was created. The normal `6966356` candidate archive remains available separately; its fault variant is not installed or distributed.

Passed here: process-scoped >15-minute HTTP outage without host disconnection, continued three-rendition encoding, same-capture post-expiry automatic upload recovery, original-page automatic decoded playback recovery, durable End/Stop/seal, full local QA and platform ready, separate local recording independence. Not passed here: normal live latency catch-up, physical network failure, all ABR/DVR modes, current-version 2.5-hour workload or automatic publication/VOD completion. Prior failures remain unchanged.
