# K3 pacing, recovery and long-run follow-up

User authorized sequential Windows-only acceptance without waiting for Mac.
Existing no-meeting authorization remains valid before Friday. No cron is used.

1. Measure process-scoped HTTP outage backlog drain with the pacing fix.
2. Measure original-page recovery and separately controlled ABR/DVR semantics.
3. Run the normal fault-disabled fixed candidate for 2.5 hours, then verify
   End/Stop/seal/ready/publication and full local/VOD media.

## Current field

- Source `4bdb86f70904e2db40964e7063483783b2cdb096`, fault-enabled developer
  build. The only differences from pacing candidate `5af0fa4` are reports.
- Fault variant 17/17 CTest pass; normal candidate ZIP SHA-256
  `9102cd7541cc4991cbb4e637763bac1bb1438d9ed36abc31943dbac5e2c22dfa`.
- Native CI for both 5af0fa4 and report HEAD 4bdb86f passed.
- Recording `94219ab8-014c-4a94-84c2-2713c13beb98`.
- Capture `abdb493b142bb01499ca92036a4bc1ad`.
- Local session `a8dd8c41-720a-47d6-81b1-4f9f2960bf82`.
- Source started 2026-10-14 13:51:25.702 Asia/Taipei UTC+8;
  separate portable local recording started 13:52:02.047 UTC+8.
- Synthetic timestamp scene, 30000/1001 fps, 900-frame grid, audio track 1.
  Three native NVENC renditions plus separate local recording. No YouTube.
- Console tab 16, follower tab 17; retain both without reload.
- Safe evidence under `artifacts/k3-pacing-20261014/`.

This is synthetic load and HTTP fault injection, not a physical NIC outage or
proof of actual meeting/YouTube load. No backlog, latency, ABR or long-run pass
is claimed before measurement. Prior 379/388-second failures remain unchanged.

## Short outage checkpoint

Follower baseline 13:54:39.108 UTC+8: 1920x1080, currentTime 68.493672 /
duration 120.149333, playing 1x, 262 frames / 0 dropped, error null. Source
wall-clock anchor is 13:51:25.702; latency estimate = sample wall clock minus
that anchor minus currentTime (start sequence zero). This is a source-clock
estimate, not the UI badge or a frame-OCR calibration.

HHC transport cut 13:54:46.159871, restored 13:55:56.4490502 (70.2891792 s).
The PowerShell duration guard logged a DateTime/DateTimeOffset comparison
error; actual restoration was after the intended one minute, verified from
saved timestamps. Pressure guard uses matching DateTimeOffset types.

At restoration 59,886,786 closed bytes awaited confirmation. The ordinary
retry delay remains; by 13:57:03 confirmations resumed. At 13:58:25.959,
all 42 closed objects confirmed, pending zero, source still encoding.
Original follower automatically resumed, no reload/return-to-live:
13:58:17.913 1080p at 279.600965 / 300.330666, playing 1x, 4745 frames /
6 dropped, error null, estimated latency 132.610 s. At 13:59:58.554 it was
437.221900 / 450.474666, 7631 frames / 6 dropped, estimated latency 75.630 s.
This passes the short synthetic-load recovery case only.

Five-minute pressure cut started 14:00:06 UTC+8. Follower remains unchanged;
separate DVR tab 18 is explicitly at zero, paused, rate 1.5. Pressure and
DVR results are pending. Admin health exposes queuedObjects as null, so it
must not be reported as zero.

## Pressure recovery checkpoint

Actual cut 14:00:06.3034812, restore 14:05:27.4534395 UTC+8:
321.1499583 seconds (5m21s), not exactly 300 seconds. At restore, 239,547,117
closed bytes awaited confirmation. Source continued throughout.

14:06:57 original follower resumed 1080p decode without intervention. Pending
fell to 119,773,563 at 14:08:20, 62,912,258 at 14:09:25, and was observed zero
at **14:13:22.044**, an upper bound of 474.591 seconds after restore. Full
confirmation can occur earlier between samples. Newly closing segments can
temporarily add bytes; this is not a persistent backlog after catch-up.

14:13:14.600 decoded time 1235.973946, source-clock latency 72.924 s. Actual
screen timestamp 14:12:00.767 independently gives about 73.8 s. 14:14:22.275
decoded time 1274.476169 gives latency 102.097 s.

Fixed ten-minute threshold: **14:15:27.4534395**. Local sample at 14:15:24.255
had pending zero, 144/144 closed objects confirmed, verified sequence 44.
Browser sample **14:15:31.993** (4.54 s after threshold) decoded 1920x1080,
time 1344.193396 / 1351.381333, playing 1x, readyState 4, 22692 frames / 39
dropped, media error null; buffered `[1203.202,1351.349999]`, seekable
`[0,1351.381333]`. Source-clock latency **122.098 s**. Do not round down to
claim strict 60-120-second pressure acceptance: it narrowly misses that
limit, while supporting the ordinary 2-3-minute direction on synthetic load.
Platform last published 14:14:22.621591, verified end 1351.35; queued null.

Paused DVR remained exactly at zero, rate 1.5, 1080p, error null across cut,
recovery and threshold. No reload/manual return-to-live on the follower.
Native recording kept running. Browser samples and frame screenshots saved.
Active non-1x DVR, controlled ABR and long-run remain separate next checks.

## ABR, finalization and a new rapid-switch failure

ABR tab 19 explicitly selected Auto (1080p). At 14:17:06.163 applied CDP
target-only 1.2 Mbps download, 80 ms latency, cache disabled. At 14:18:24.297
it decoded 854x480, playing 1x, readyState 4, error null. At 14:19:34.907 it
was still 480p but readyState 1 with currentTime beyond its buffered ranges;
do not claim uninterrupted weak-network playback. Restored unlimited network
at 14:19:34.919. Auto later rebuilt at 1080p after the EOS regression.

Separate DVR played from zero at 1.5x: 89.381115 at 14:17:12.942 and
196.428734 at 14:18:24.307, without a forced jump. Explicitly paused at
213.246493. That position, pause and 1.5x survived End and platform processing.

End ACK boundary 57, source stopped 14:19:58.521. Stop accepted
14:20:02.085546542, seal accepted 14:20:47.797728, original operation keys.
Local recording continued until explicit tester stop 14:20:25.726.
Public `[0,57)` = 1711.71 seconds. All 181 inventory objects confirmed;
58 source segments per rendition, source duration 1712.577533. Full local
hash/codec/CFR/grid/timeline/decode passed. The CLI's integer `--seconds`
could not represent this arbitrary End tail; called the unchanged verifier
function with expected 1712.6 seconds after the integer-only invocation
failed its duration tolerance. No media files or timestamps were changed.

Platform ready observed at serverNow **14:26:07.330968801**, seal-to-ready
upper bound **319.533241 s**. Automatic publication/archived observed from
the original Console at **14:29:36.864**, seal-to-published upper bound
**529.066 s**. No manual publication; the upload helper remained alive after
OBS closed until publication completed.

Ended-live Auto480 naturally reached EOS at 14:21:52.053 (1711.743999 s,
paused 1x, error null). Home -> manual1080 at 14:22:28.124 -> decoded1080
at 14:22:42.216, an observation upper bound of 14.092 s. Auto subsequently
decoded1080 with position zero, pause and full public duration retained.
This is ended-live replay, not yet published VOD.

Paused DVR switch720 restored exact 213.246493 / 1.5x / paused with real
1280x720 decode. Screenshot during rebuild retained the prior timestamped
frame. Manual480 and1080 also restored. However rapid 480 ->1080 ->Auto
selections **14:25:07.876–14:25:10.006** (2.13 s) failed: at 14:25:56.662
and 14:26:54.684 the video remained 0x0, readyState0, frames0, buffered empty,
currentTime0, rate1, paused true, error null. Previous state was
213.246493, paused, rate1.5. Auto remained selected. No reload was attempted.
Retain this recording/tab for Mac diagnosis; it is not disposable yet.

Browser samples and screenshots are in `artifacts/k3-pacing-20261014/`.
Published VOD on the same URL decoded 1920x1080 at **14:33:58.884**,
currentTime646.3886 / duration1711.71, playing1x, readyState4, 1399 frames /
0 dropped, error null. Seek to600 placed playback inside the outage interval;
buffered `[0,120.119999]` and `[570.57,780.779999]`, seekable `[0,1711.71]`.
Then paused this separate VOD tab and restored browser cache/network defaults.

Normal fixed-candidate 2.5-hour run started separately at14:31:37.849;
see `windows-k3-long-2026-10-14.md`. It cannot inherit a pass from this short
developer fault build.
