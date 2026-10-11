# Windows B1 production retest — 2026-10-11

Status: in progress; no complete A–E acceptance claim. All event times use Asia/Taipei (UTC+8), with server/browser UTC values retained in local JSON evidence.

User explicitly confirmed outside meeting and authorized real OBS/GPU/new capture testing this round.

- Fixed Mac handoff: `6a607af33b5de3f0a9258e3c52e1ce1d180c538f`, `production-blockers-2026-10-11.md`; actual bytes match SHA-256 `1f7ef3b8a3fe64c738a225565040eeb7a134bece1f2b95589f32be03a9940a3d`.
- All five immutable `rc2/` artifacts match the published hashes. B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory schema 1 unchanged.
- Actual local package source `b259fa747f5454d0c1d9c4bab686484d36646a54`, ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`. Installed DLL/helper hashes match its manifest. Hosted ZIP remains separate evidence, not the package exercised here. Unsigned.
- Fresh local CTest: 14/14 passed; evidence `artifacts/b1-retest-20261011/local-ctest.log`.
- Production reads: CMS `hhc-web-api--0000182`, Asset `asset-api--0000090`, both Running/Succeeded/latest traffic 100%; writer/HLS/source flags enabled. Website actual `hhc-web--0000199`, Running/Succeeded, source `46ea3802c8e83e5725eea163b408a7fe9476af05`, successful release `38107954344`. Compare against the handoff's `bf1b8b5` changes only bulletin/archive/weekly-reader files; do not attribute this test to old revision 0000197.
- Original recording `e9c5ebb4-74d7-46e0-a7b4-e3730579fc8a` read-only at 12:06:15: capture ready/40 verified/autoPublish=published; authoritative CMS is now version 4/status=draft/selectedCoverId set. This differs from Mac's earlier version 3/published snapshot. No old Stop/seal/manual publish sent. Old replayUntil has actually elapsed and cannot qualify deadline-preserving continuation.

## New capture and first milestones

- Recording/broadcast `e38353c4-ce2f-4851-a21c-89b3f0afcd97`.
- Capture `9bb8071a479d63385f95623c4b6a4718`, epoch 1; local session `b71b8832-ad4d-4e1c-85a5-b9248e8b8066`.
- Manual Start/End; automatic post-event publication enabled in Console; automatic cover. Fresh draft/announcement; no parallel C1 capture.
- Dedicated portable OBS 32.2.2 HHC-Fixture, synthetic Program clock, track 1, 30000/1001 fps; 900 frames/30.03 seconds. Installed original OBS profiles/scenes hashed before work. No original profile/YouTube modifications.
- Bind click 12:10:46.934; native NVENC 1080/720/480 initialized 12:10:49.243/.294/.338, AAC 48 kHz stereo/track 1 at .365. Capture-specific durable journal retains bind key and accepted receipt.
- Premature preview attempt returned 409 while media was not ready. After platform verified objects, UI retry succeeds: staff preview true decode 12:13:43.385, 854x480/currentTime 33.936918/1,004 decoded frames/0 dropped/error=null/1x playing; buffered [0,60.059999]. Infinite live duration serializes to JSON null, not a missing/final VOD duration. An observed preview Media request returned 206. Cookie OPTIONS result is not yet separately preserved.
- Global OBS recording intentionally started in this dedicated test profile 12:12:46.862 (HEVC NVENC 1080p/10 Mbps, `D:/Users/IT/Videos/2026-10-11 12-12-46.mkv`). This will test End/Stop independence. No YouTube broadcast started; concurrent real YouTube remains unqualified.
- Original new member tab 36 opened before Start and kept without reload.
- Console Start click 12:13:57.287; native command `b711ce1b-5173-4059-8552-b341b5785ee8` persisted next common boundary **7**, original operation key ACK accepted. At 12:14:18 server phase start_pending awaits media availability. Do not substitute verified sequence for boundary 7.

## New formal results

| Event (UTC+8) | Evidence and result |
| --- | --- |
| Member preview negative, before Start | Existing member `hhc_media` credential temporarily scoped to the exact staff preview path: actual HTTP 401, CORS MissingAllowOriginHeader / net::ERR_FAILED; original member credential preserved, temporary cookie removed. No credential value retained. |
| 12:16:54.473 | Original member page transitions without reload and decodes 854x480, currentTime 13.337859, duration 60.083333, 409 frames/0 dropped/error=null, playing 1x. First Play is an explicit autoplay gesture, not outage recovery. |
| 12:18:57.215 | Auto decodes 1920x1080, time 136.080029/duration 180.211333, 4,088 frames/0 dropped/error=null. UI lag about 143s; capture origin + startSequence*30.03 + currentTime estimates about 141.5s. Short online sample only. |
| 12:19:19.317 | Console End confirmed. Native command `53f5b8df-9acb-4721-a714-17c29089f089` durably selects next encoder common boundary 18; original-key ACK accepted. HHC remains encoding after End. |
| 12:20:29.389 local / 12:20:28.411924094 server | HHC normal Stop; concurrent global recording stays active. Different clocks retained, not silently reordered. |
| 12:20:54.932 | Original member page continues after Stop, 1080p/time 281.870026/duration 300.339333, playing 1x, 2,155 frames/14 dropped/error=null; processing text allows continuation. Source rebuild resets frame counters. |
| 12:21:18.991719 server | Seal/uploadedAt accepted. Original inventory, key and fps remain unchanged; native pending zero/seal-ack observed 12:22:07.635. |
| 12:22:50.222 | Manually stop the concurrent test global recording after isolation evidence. HEVC/AAC, 1920x1080, 30000/1001, 602.768s; full CPU decode exit 0; SHA-256 `03aafe70019eaa45edfa308aba3aa50993c63466e0ce31334a1c03ca4d140a01`. No actual YouTube concurrency qualified. |
| 12:22:57.289430 server | Ready; 67/67 objects verified. Three renditions each 20 segments, 580.246333s; 19 full 30.03s plus 9.676333s tail. Local full-media hashes/grid/timelines/three full CPU decodes pass independently. |
| 12:27:53.898472097 server readback | Automatically published, recording version 3/status published, selectedCoverId `156b36d15e5e497e9ab53725ce799b04-1`. No manual publish or cover selection. Ready is not used as publication evidence. |
| 12:28:32.774 | Selected VOD auto-cover 1 is 62,713 JPEG bytes, SHA-256 `9e8cbc96fb16db5d88c3e4c02a4e6081de1dea69b33d37b7e5a2b71a8e8317f0`, exactly matching live image at 12:19:44.142. |
| 12:29:04.620 readback | B1 revision 13/archived, archive published/reason null, public boundaries still **[7,18)**, revision 5. Later Stop does not expand End. Public nominal length 330.33s; full private capture length is 580.246333s. |
| 12:29:27.769 | Original scope, no reload, actual 480p reaches natural end: currentTime=duration=330.355333, ended=true/paused=true, 9,900 frames/0 dropped/error=null. Earlier zero-frame endpoint sample is excluded from this pass. Scope replayUntil 13:01:09.084525; continuation is within that deadline. |
| 12:30:27.035 / 12:30:50.539 | Natural end -> Home -> 1080p -> Auto: actual 1080p playback restored; positions 12.677451 then 35.370778, no reload. Auto retained live-scope position. Intermediate error is recorded below, not counted as a clean transition. |
| 12:31:02.190 | Explicit UI "continue post-event video" on the original URL transitions to published VOD, same title/URL, 1080p/time 43.497872, duration 330.33, playing 1x/error=null. VOD respects public boundaries. |

Admin control-room access token expired during polling (401); ordinary Console reload restored archived state. The original member page was not reloaded. This is separate from native Credential Manager refresh and the preserved member scope.

## Player failure and separate partial passes

**Reproducible transient failure:** switching natural-end/backward-seek 480p to 1080p at 12:29:51.679 displayed "無法播放媒體。", with videoWidth/Height=0, time=0, duration=null, zero frames and native media error=null. It recovered by 12:30:13.259 with actual 1080p, four decoded frames and the original paused position. Paused 1080p/Auto -> 720p at 12:31:33.350 and paused 1080p -> Auto at 12:33:10.366 repeat that prompt. This does not pass the clean switching/last-frame acceptance. Browser screenshot capture timed out; no nonexistent screenshot claimed. DOM and structured decode evidence are retained.

Recovered paused manual 480p/720p/1080p/Auto all preserve **49.812211s**, paused=true, rate=1.5 and approximately 330.33s total, with actual decoded dimensions 854x480/1280x720/1920x1080. Transient rebuild state resets to 0/1x before restoration; final preservation and the visible error are separate results. Rapid switching and last-frame pixels remain unqualified.

Independent tab 37 uses CDP 1.1 Mbps download/120ms latency/cache disabled: Auto plays actual 480p at 12:32:52.752, and continuously progresses to 41.196043s/1,242 frames/4 dropped/error=null at 12:33:33.980. Change to 100 Mbps/20ms then eventually observes actual 1080p at 12:38:11.033, time 318.248686, 9,546 frames/4 dropped/error=null. No claim about exact promotion time or downward adaptation. All temporary network/cache overrides restored. This is controlled browser throttling, not physical weak-network certification.

## Remaining this round

Prepared new recovery recording `23276597-1b23-43b1-b4fb-5d23d3bcdd35`, announcement only, no capture/encoder yet. User authorization for real OBS remains valid. Non-elevated Windows process cannot install the narrow temporary outbound block needed for unchanged-candidate producer-outage testing. Prepared 65s test script targets only the owned portable OBS and HHC helper, removes rules in finally, and waits for an explicit trigger. User asked whether UAC approval is available; do not start that dependent step until answer.

Still unqualified: current B1 interruption retest, all-object public/private media authorization, normally stopped never-public rehearsal, controlled downward ABR, rapid switching/last-frame pixels, normal meeting load including actual YouTube, automatic follower recovery with pending/queued/verified/playlist/position evidence, five-minute outage/ten-minute pressure indicator, current fixed 2.5-hour continuous playback. Historical approximately 379/388-second delay failures remain failures. Source/package unchanged; CI/local/short live/VOD results do not qualify these remaining gates. E remains after A–D; a final published VOD cannot conceal failed live recovery.

Original nine OBS profile/scene hashes still match at 12:40:38.582; test global recording and HHC output are inactive. Only the owned portable test profile was used. No original YouTube settings changed. Narrow firewall script parses without errors and neither UAC elevation nor any firewall mutation has been started while confirmation is pending.

Local evidence lives under ignored `artifacts/b1-retest-20261011/`. Credential Manager holds authentication; evidence excludes token/password/cookie/signed URL (targeted JWT/signed-query scan has no matches). Preserve original inventories, operation keys and failed measurements. The [Mac player follow-up](windows-b1-player-followup-2026-10-11.md) is copyable and keeps prior platform failures separate from this new capture's passes.
