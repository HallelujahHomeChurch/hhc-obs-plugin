# Mac follow-up: ended-live rapid quality switching

Windows completed independent long-run acceptance. Do not reset the stream
key, create another capture, or treat this report as a deployment request.

## Retained production reproduction

- Recording `94219ab8-014c-4a94-84c2-2713c13beb98`.
- Capture `abdb493b142bb01499ca92036a4bc1ad`.
- Windows source `4bdb86f70904e2db40964e7063483783b2cdb096`, developer
  HTTP fault build; pacing code equals normal candidate `5af0fa4`.
- B1/C1/K3 contract and 30000/1001 fps / 900-frame grid unchanged.
- All times below 2026-10-14 Asia/Taipei UTC+8.

Original ended-live DVR page was paused at `currentTime=213.246493`,
`playbackRate=1.5`. Ordinary individual 720p/480p/1080p switches restored
the position, pause and rate; the old decoded frame remained visible while
rebuilding. Rapid `480p -> 1080p -> Auto` at **14:25:07.876–14:25:10.006**
instead lost the state and did not recover through **14:26:54.684**:

```
videoWidth=0 videoHeight=0 readyState=0 totalVideoFrames=0
currentTime=0 playbackRate=1 paused=true buffered=[] error=null
```

Auto remained selected. No reload or manual recovery was attempted on that
page; Windows retains it and the recording for diagnosis. This reproduction
is ended-live replay, not a fresh published-VOD page. Local screenshots and
sanitized samples are under `artifacts/k3-pacing-20261014/`, especially
`rapid-switch-stuck.png` and `browser-samples.json`.

Please investigate overlapping source rebuilds and preservation of playback
intent across rapid switches and live-to-VOD transitions. These are hypotheses,
not a proven root cause. Add regression coverage for multiple rapid changes
while paused/non-1x and preserve the last valid position, pause, rate and
decoded frame. Report exact fix commit, CI, deployed versions and a fixed
handoff for targeted Windows retest. Do not infer a platform fix from the
Windows upload pacing change.

## Separate results, not blockers to conflate

- Short 70.289-second HTTP outage recovered automatically without reload;
  synthetic OBS plus local-recording load, not real meeting/YouTube load.
- Pressure outage was 321.150 seconds. Pending 239,547,117 bytes reached
  observed zero at14:13:22.044, <=474.591 seconds after restoration.
- Fixed ten-minute checkpoint was14:15:27.453; sampled14:15:31.993 latency
  was122.098 seconds. Do not round this into a60–120-second strict pass.
  Platform queued count was null/unavailable, not zero.
- Target-only1.2Mbps/80ms Auto test decoded480p, but later readyState1;
  uninterrupted weak-network playback is not passed. Network/cache defaults
  restored. Earlier32.730-second uncached seek remains separate.
- End boundary57, public `[0,57)`1711.71 seconds. Stop14:20:02.085546542,
  seal14:20:47.797728, ready observed14:26:07.330968801, automatically
  archived/published observed14:29:36.864. Local full-media QA passed.
- Published VOD decoded1080p at646.3886/1711.71 after a seek into the outage
  interval. This does not erase the original ended-live rapid-switch failure.
- Prior379/388-second latency failures are retained unchanged.

No credentials or signed URLs are needed in the reply. Windows normal
candidate 2.5-hour capture and finalization passed; uninterrupted viewing did not.

## Latest platform handoff acknowledged during testing

Downloaded Admin350825d fixed finalization handoff; SHA-256 matches
`965497d834e3a2a16552503f1416a44aa0a371ce486a1c09ac86aa507b949849`.
At15:50:33.212 UTC+8 compared all24 document script pathnames on the retained
rapid-switch failure page, the ongoing long-run page, and a fresh original-first
VOD page. All three lists match exactly. This rules out different document
script paths as a simple explanation; it is not independent Azure revision
readback. Requested fresh short ended-live case follows the active long-run.

The long-run also captured a follower stall/backward recovery at15:35:18–33;
see `windows-k3-long-2026-10-14.md`. That issue is separate from quality switching.

## Consolidated final handoff to Mac

Please fix only the remaining reproducible player problems below. Preserve
passed ingest, recovery, media validation and publication evidence; do not
request another complete upload or 2.5-hour source run for a player-only fix.

1. **Rapid ended-live source changes lose playback intent and stall.** Fresh
   Website207 short recording `45fd5c77-be41-4fe4-a2f2-03ad57b9fe9d`, capture
   `61da0f63c09600506282f2cd55bd50f7`, normal installed plugin `5af0fa4`.
   At 17:54:23.710 it decoded 720p at 120 seconds, paused, rate 1.5.
   A slower switch group recovered correctly. A second rapid group
   480p -> 1080p -> Auto at **17:55:00.470–17:55:03.315 UTC+8** instead
   remained width/height 0, frames 0, readyState 0, currentTime 0, rate 1,
   paused, buffered empty, MediaError null through **17:56:47.207**,
   103.892 seconds after the final selection without recovery action.
   Preserve the last valid intent through overlapping changes; cancel stale
   rebuild completion safely. This is a diagnosis direction, not proven cause.
   Single changes, EOS -> start -> 1080p -> Auto and retained last frame passed.
   Source is already stopped/sealed/ready; auto publish is deliberately off.
   Retain this record for diagnosis; replay UI advertises expiry at 18:30.

2. **Follower buffering/recovery can move backward without user action.**
   Normal candidate long recording `a57859d4-364e-4f24-9589-209294de5d18`,
   capture `77044fc2351d448bbf2622be9b9ded2c`. Original follower had no reload,
   seek, pause or return-to-live action. Samples at 15:35:18.561, 23.572,
   28.595: position 3753.759063, frames 98659, readyState 2, buffer end
   3753.749999. At 15:35:33.612 resumed at 3727.565097 (back 26.194 s),
   frames 98810, readyState 4. MediaError null throughout. Compare playlist
   advancement, available edge and player recovery policy; preserve deliberate
   DVR/pause/non-1x semantics. Do not fix by forcing all viewers to the edge.

3. **Weak-network playback remains a separate limitation.** Target-browser
   1.2 Mbps / 80 ms Auto test on recording
   `94219ab8-014c-4a94-84c2-2713c13beb98` decoded 480p
   but later stalled at readyState 1. ABR downshift passed, continuous weak-net
   playback did not. Keep earlier 32.730-second uncached seek evidence.
   Prioritize normal-network stability first; do not demand fixed 480p startup.

Passed and not to redo: fixed-key ingest; 16-minute process-scoped outage
recovery; pacing backlog drain; three-rendition full local validation; normal
2.5-hour source and local recording; one-attempt platform ready; natural
publication; inherited-cover bytes; same-URL VOD seeks; first retained field
natural publication; current Console no longer asking to stop OBS; ordinary
ended-live switches, position/pause/rate restoration and natural EOS regression.

Long seal 17:09:13.309649 -> ready observed 17:36:01.366, then natural
published/archived 17:40:46.310. Only attempt 1 observed. This does not explain
historical 57-minute waits or erase historical 379/388-second latency failures.
Pressure checkpoint was 122.098 seconds, not a strict 60–120-second pass.
Normal meeting plus actual YouTube load remains untested, not a confirmed bug.

Please deliver one combined fix/report, tests for overlapping source changes
and follower recovery, exact commits/CI/deployed revisions, and a fixed Windows
targeted-retest prompt. Report any remaining limitations explicitly. Do not
change K3/B1/C1, digest, idempotency, 29.97 fps/grid, public boundaries or relax
media validation. Do not reset keys, publish/delete protected recordings, or
include credentials/signed URLs. Deployment requires the user's normal process.

Windows artifacts remain local under k3-replay-20261014, k3-long-20261014 and
k3-pacing-20261014; versioned reports provide the sanitized measurements.
