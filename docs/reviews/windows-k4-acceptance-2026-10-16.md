# K4 Windows acceptance — 2026-10-16

Status: scheduled targeted flow passed through natural publication and same-URL VOD; direct normal Stop lifecycle blocked, evidence handed off to Mac.

Authorization: user permits isolated OBS/GPU testing until 2026-10-16 18:30 Asia/Taipei. No new permission request is needed within this window. Preserve diagnostic recordings; no key reset, manual publish, deletion, formal OBS update or merge.

## Candidate and fixed contract

- Plugin: 667ee287e75ed9382624938eb2360bf3fe70eb48; PR #3 remains open.
- CI push 38713805309 and PR 38713812808: success (read back this run).
- Installed isolated legacy CI ZIP SHA-256: 86f5f216a1d74bd445c55c7e3a8c10198feb68fec1f44c71657e6132c93beab6.
- Fixed platform contract commit: 5e8c33ec1b3ca5628605fab0a578ca8312d67d1d.
- Handoff hash d63a5d3b5d6e2e7835e0e1f162fed3a2050f510a94d44759bfd4fdc30d064c2e; manifest hash e984a5a54b80c817ba701f6923d73c02b57bf9c5b7d238f15442543c5ae18464 rechecked.
- Prior local 22/22 CTest and fixture/schema/legacy hash evidence: windows-k4-local-2026-10-15.md. These are not formal acceptance.

## Initial overnight preflight (superseded by resumed run)

- Used existing .deps/portable-recovery runtime; existing active journal is released=true. No journal migration, credential changes or new output intent.
- Backed up and installed only candidate DLL, upload helper and TLS backend. Installed hashes equal extracted CI files; artifacts/k4-acceptance-20261016/isolated-install.json.
- Isolated OBS PID 69628 launched 2026-10-16T00:55:08.1384996+08:00.
- Startup log 00:55:12.077 confirms exact plugin source above; 00:55:11.823 confirms 30000/1001 fps; 00:55:13.664 confirms key-only dock initialized.
- Windows native tool repeatedly returned no targetable windows, including after startup. OS session is active; no lock assertion made. User asked to show the isolated OBS window. No HHC Start, global streaming, recording or YouTube action performed.
- Formal Admin original tab 28 navigated to /content/studio/direct. Account menu confirms foh@alive.org.tw. Existing selected key label HHC; waiting-for-signal UI and existing watch URL recording 89cf022d-e80e-479f-9bbe-d8590771cc59. UI displays no assigned receiving broadcast. This does not alone prove server input idle or delegation scope.
- Opened details dialog read-only; title is 直播, empty description, automatic thumbnail. No save performed.

## Platform versions supplied by Mac, not independently runtime-read in this run

CMS 4a426ebf0b7fcf236e218d0108073a8c7e821544 / hhc-web-api--0000200; Gateway 26c3b240155e8648478a5fd5972aba346acd5273 / api-gateway--0000208; Admin 8168c1a83c5db7834d2e971048d2ba5abad43b8b; Website 323c6d8c2b7c042354636f1bf08dc12a0d3ed7c1 / hhc-web--0000209; Asset 88a02b938c4d4be96b40a7a107544e235fb1b2ac / asset-api--0000094.

## Outstanding at overnight preflight (historical)

Verify source/delegation and actual entry version; complete direct open, normal cleanup and same-URL playback, Console-closed direct Start, scheduled preview/control/release and conflicts. No K4 capture/recording created by OBS, ACK boundary, decoding, seal/ready/publish evidence yet. Lost-response/restart tests have local/mock evidence only. Historical latency/weak-network findings remain unchanged. No full long-run repetition required.

## Native UI recovery, 2026-10-16 01:21–01:24 UTC+8

Resolved the earlier native UI blocker. Initial development launch used Hidden; the native inventory could not target its main window. Computer Use explicit path without process: selected the installed OBS instead; no output buttons were pressed. Exact process: launch selected the portable executable but lacked OBS working directory, causing locale errors, which were dismissed through Computer Use. Checked released=true and restarted only idle isolated PID 69628 using the correct bin/64bit working directory and visible window. Chose normal mode via Computer Use; isolated PID 42364 exposes HHC-Fixture main window. Opened HHC 串流 dock through Docks menu; shows 未連線 and 開始直播. No capture started. The scene still contains old static test text and must be updated before media acceptance. Formal OBS was opened by the launcher but its output/configuration were not changed.

## Resumed 09:42 UTC+8; authorization extended to 18:30

User confirmed meeting finished and permits testing until 2026-10-16 18:30 Asia/Taipei. Isolated OBS PID 71124; exact source 667ee287e75ed9382624938eb2360bf3fe70eb48 confirmed in startup log. Existing clock source now displays current K4 source/time; no formal OBS changes.

Direct input API 200: source 2c58a390-1906-4d62-b962-777f7f40bbb5 revision 22, directEnabled=true, inputBusy=false, targetRecordingId=null. Existing FOH delegation already enabled; no delegation/key mutation. Saved explicit synthetic-test title/description in existing waiting direct event; watch URL unchanged. Actual Admin entry /assets/index-B-iXU8Lu.js.

First direct output: Start UI 09:44:42; encoders started 09:44:44.508. localId/outputSessionId 9580c378-5f1e-4175-9fc9-66b9a351d277; recording 89cf022d-e80e-479f-9bbe-d8590771cc59; capture 32264350a74315406f3d119a72023cce; epoch 1. Durable K4 resolved intent and completed receipt match; no K3 resolve/bind body. Opening-before-request sequencing and lost-response scenarios still rely on existing local/mock evidence, not a new production fault injection.

Automatic start observed: source startSequence=0, phase live revision 6, verifiedEndSeconds=120.12 at 09:47:55. Member original tab changed from waiting to player without reload. User-gesture playback at 09:48:00; at 09:48:33.363 actual 1920x1080, currentTime=94.439555, duration=150.186666, readyState=4, frames=1040, paused=false, rate=1, MediaError=null. Wall-clock minus source-start minus playback-position ~=134.4s (coarse source-start basis; not a recovery stress pass).

Normal HHC Stop at 09:49:18.777; API Stop accepted 09:49:20.307682039, seal accepted 09:50:02.0298. Local active intent released=true by 09:51:09; capture validating, liveState ended. No manual Console End or publication. No B1 Start/End ACK command in journal for this automatic direct case. Public final boundary and natural publication still pending.

Local media QA: all SHA-256, full ffmpeg decode, audio/video parameters, aligned three-quality segment grid passed; 10 segments each, 274.040433s, 30000/1001 fps. Local evidence is separate from platform ready. Evidence under artifacts/k4-acceptance-20261016/direct/.

Admin preview initially returned 409 broadcast_state_conflict before media ready. After live, retry still displayed error initially, later embedded preview controls appeared. Actual preview decode has NOT yet been measured; keep this as unverified, not a failure proven after readiness. Member decoding is independently proven above.

Created one future scheduled draft d1618202-89ec-44c8-9082-0d7fcb242040, 2026-10-16 17:00 UTC+8; reused this synthetic direct event's title/description then renamed to scheduled. Did not prepare/announce it yet. Existing unrelated scheduled test 0f1991e0-683b-4b15-bfaf-9631af34c1f5 was not modified.


## Direct post-seal discrepancy and scheduled continuation

First observed capture ready serverNow 2026-10-16T09:51:19.885013414+08:00, liveState ended; ~77.855s after seal acceptance is an observation upper bound, not exact ready transition time. At 09:56:29.434 Console studio still phase live/revision 6, endSequenceExclusive null, archive none, health interrupted; policy all true. Member original page reports ended replay until 10:25 but is stalled at currentTime 274.023611/duration 274.069333, readyState 2, MediaError null. No manual End/publish used. Contract section 5 explicitly expects normal sender Stop to close direct mode at a common boundary. Retain for platform investigation; do not attribute root cause from these observations alone.

Scheduled Prepare via normal UI succeeded at approximately 09:56–09:57 despite original direct page still showing input in use/live. Receiving link changed to scheduled d1618202; no HTTP code captured for this operation (Network capture had not been enabled on that tab). This is not a proven occupied-input rejection. K4 explicit new Start 09:58:07, encoders 09:58:08.190; local/output ID cb125385-2253-458e-84dc-8d01a16d819d, capture 6c69d79ff2981de8d40380b2b926bd8f, epoch 1. Bound correct prepared scheduled recording, distinct from first output. Policy autoStart=false, autoEnd=true, autoPublish=true, phase preview. Announced through UI; member page only countdown, not preview media. Scheduled time 17:00 is informational.

Scheduled: Start UI 09:59:43, HTTP200 accepted receipt operationKey 47818541-02f6-49e1-a13f-c51415636e16, command c85fbaeb-21a6-4328-a745-09a1908e137a. Durable ACK startSequence4 (120.12s). At10:01:36.883 still start_pending, verifiedEndSeconds120.12; by10:02:20 live/member original page automatically offers playback. The 17:00 schedule did not gate publication.

Staff preview initially409 broadcast_state_conflict at09:59:19 and shortly after Start, but succeeds after verified public media. Actual staff video10:02:50.962:854x480,currentTime20.872007,readyState4,frames592,errornull. Do not report the early transient409 as persistent preview failure. Member10:02:38.832:854x480,frames328,currentTime10.621254,duration60.082666,readyState4;10:03:01.660:1920x1080,frames1012,currentTime33.449085,duration90.119999,pausedfalse/rate1,errornull. No reload.

End UI confirmed approximately10:02:57; command21e4915d-7bb1-4eb9-b7ee-d95f23e6b6ff ACK boundary10, original operation key cb125385-2253-458e-84dc-8d01a16d819d.ack.21e4915d-7bb1-4eb9-b7ee-d95f23e6b6ff. HHC output stopped automatically10:03:09.466. Expected public range[4,10),180.18s, excludes120.12s preview and any tail afterEnd. Global OBS/YouTube/localrecording were idle throughout this targeted test, so concurrent live-output isolation is NOT newly verified under real load.

Scheduled stop accepted10:03:08.493830221; seal accepted10:03:42.277122 (normalEnd=true, digest6bb58af025d71bfb4e43b5f723bf26db33f906c8a6de288fde559003ff05f74b). At10:04:03.088 studio end_pending/revision9, boundaries[4,10)/revision4; last verified edge270.27. At10:04:07.312 capture validating. Local full decode/hash/grid QA passed:11segments,301.034067s perquality; expected public VOD180.18s differs intentionally from fullsource301.034067s.


Scheduled ready first observed serverNow10:04:35.656023514 (53.379s after seal acceptance, observation bound). Capture liveState ended; active intent released=true. At10:05:19.283 CMS processing/revision11, archive ready, boundary[4,10), verifiedEndSeconds301.034067. Original member page automatically changed to ended-processing replay. Natural publication still being observed; Asset autoPublish cancelled is not sufficient alone to diagnose CMS publication because this is a broadcast-managed archive.

Browser screenshot attempts failed twice with Page.captureScreenshot timeout. Safe journal/API/media samples retained instead; do not claim screenshots saved. No page reload or artificial recovery used.

## Final result, 10:09:45 UTC+8

**Scheduled pass:** recording API published/version3, readyAt10:04:34.9723 (52.695s after seal; published first independently observed10:08, not an exact publication timestamp). Original Studio eventually archived/revision12/archive published at10:08:55.833, boundaries[4,10)/revision5. Do not label its temporary processing lag a sustained failure. Member original page offers standard Continue VOD action; clicked without reload. Same URL VOD then seek/play:10:09:45.329,1920x1080,418frames,currentTime73.78498,duration180.210666,pausedfalse/rate1,readyState4,errornull,buffered/seekable[0,180.210666]. The30.666ms difference from nominal180.18 is reported, not rounded away. Fullsource301.034067 includes preview/tail; public playback does not.

**Direct failed to complete:** readyAt09:51:17.232848 (75.203s after seal); recording API10:08:44.196 draft/version2/autoPublish pending/no selectedCoverId. Studio10:09:45.349 remains live/revision6,start0,endnull,archive none. No artificial End/publication performed. Requires Mac lifecycle investigation; root cause not proven on Windows.

Thumbnail: scheduled auto selection ready/inheritedRevision8 in Studio, selected auto cover1 in recording management; byte hash consistency NOT verified. CDP returned empty image body; empty-body hash discarded and file removed. No false thumbnail hash evidence retained.

**Untested/blocked:** Console-closed new direct, production lost-open-response/restart, K4 simultaneous Console conflict and final thumbnail hash equality. Local/mock recovery tests and CI remain separate evidence. No simultaneous YouTube/recording load, no rerun of passed long suites. Original member/Console tabs retained for follow-up; outputs stopped, active released=true, synthetic clock stop marker written. Formal OBS unchanged.

Full consolidated Mac handoff: mac-k4-followup-2026-10-16.md. No runtime code changed in this acceptance turn. Two native checks on actual plugin667ee28 were re-read successful; a report-only commit is not a new installed binary.
