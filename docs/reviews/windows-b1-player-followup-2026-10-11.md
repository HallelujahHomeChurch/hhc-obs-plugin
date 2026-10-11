# Mac follow-up: transient quality-switch error on published B1 VOD

Please investigate the reproducible member-player transition below. Keep the B1/C1/schema/digest/fps/media-validation contracts unchanged. No Windows plugin change is justified by this symptom yet.

## Fixed execution context

- Windows plugin package source `b259fa747f5454d0c1d9c4bab686484d36646a54`, local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`.
- Actual Website `hhc-web--0000199`, source `46ea3802c8e83e5725eea163b408a7fe9476af05` (includes bf1b8b5 playback fixes); CMS 0000182, Asset 0000090. Readbacks distinct from earlier handoff Website 0000197.
- Recording `e38353c4-ce2f-4851-a21c-89b3f0afcd97`, capture `9bb8071a479d63385f95623c4b6a4718`, epoch 1.
- Public range [7,18), nominal 330.33s; full capture 580.246333s, three renditions/20 segments each, 30000/1001fps/900 frames. All 67 objects verified, ready, automatically published/version 3, B1 archived/revision 13.
- Original member URL: `https://www.alive.org.tw/zh-Hant/member-videos/e38353c4-ce2f-4851-a21c-89b3f0afcd97`.
- Original member page stayed open without reload from pre-Start through Stop/seal/ready/publication and the following live-scope quality regression; explicit UI continuation later changed it to VOD on the same URL. No manual Publish or cover selection.

## Reproduction (Asia/Taipei UTC+8)

1. 12:29:27.769: actual 480p reaches natural end, 854x480, time=duration=330.355333, ended=true, paused=true, 9,900 frames/0 dropped, native media error=null.
2. UI playback slider Home; select 1080p.
3. 12:29:51.679: visible "無法播放媒體。", videoWidth/Height=0, currentTime=0, duration=null, zero frames, paused=true, media error=null.
4. 12:30:13.259: recovers without reload to 1920x1080/time0/duration330.33, four decoded frames, paused=true/error=null. Explicit Play then decodes normally.
5. Restore Auto: playback resumes with the intended position, but transient rebuilding still needs last-frame/error-state QA.
6. After explicit UI continuation to published VOD, pause at 49.812211s, select 1.5x. Manual Auto/1080p -> 720p at 12:31:33.350 repeats "無法播放媒體。" with zero dimensions/time/duration and rate1. By 12:31:46.695 it restores actual 1280x720, **49.812211s**, duration330.33, paused=true, **rate1.5**, four frames/error=null.
7. Paused 1080p -> Auto at 12:33:10.366 repeats the prompt and temporary reset; by 12:33:33.988 it restores actual 1080p and the same position/pause/1.5x.

Repeated prompts are failures of clean quality transitions even though final state preservation works. A null native media error does not identify the cause. Browser screenshot capture timed out; DOM and structured decode measurements are available, not pixel proof of last-frame retention. Do not infer transport/decode failure or prescribe a Windows workaround from the prompt alone. Please inspect the existing player source replacement/error lifecycle, actual watch requests and media events to identify the root cause, then add the smallest regression covering natural end/backward seek and paused 1.5x switching. Read-only measurements did not capture a failing watch response; no HTTP error code is asserted for the transition.

## Separate passes and remaining gates

New staff preview truly decoded; a member credential against staff preview was rejected HTTP401. Original scope continued within replayUntil13:01:09.084525. Start/End ACKs used encoder boundaries 7/18; Stop preserved [7,18). Active global OBS recording survived Console End and HHC Stop. VOD selected auto-cover1 JPEG hash exactly equals live JPEG: `9e8cbc96fb16db5d88c3e4c02a4e6081de1dea69b33d37b7e5a2b71a8e8317f0`, 62,713 bytes. These earlier blockers are passed for this new capture.

Controlled browser Auto at 1.1Mbps/120ms plays actual480p; after restoration to100Mbps/20ms it eventually reaches1080p. This does not prove controlled downshift, physical weak-network behavior or producer-outage recovery. Approximately379/388-second historical recovery failures remain failures. Current normal-load recovery, five-minute/ten-minute pressure indicator and fixed2.5-hour acceptance remain independent Windows work; do not mark them passed from this VOD result.

After the fix is tested and deployed, return a fixed handoff with commit/revisions, immutable report hash, CI/deployment evidence and exact Windows regression steps. Preserve this recording for reproduction; do not manually republish it to conceal a state transition.

Complete local evidence ledger: [Windows B1 retest](windows-b1-retest-2026-10-11.md). Credentials, cookies and signed URLs are excluded.
