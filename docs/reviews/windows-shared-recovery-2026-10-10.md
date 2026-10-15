# Fixed-version Windows shared-load recovery handoff

Actual downloaded CI plugin `d59f1eb8035168cdc205eebcd1b42f82ecd76c0e`,
C1 `c1-2026-10-08.2`, schema 1; immutable manifest SHA-256
`f702477236ba184853c119ffa9c4da0cc9f6d44cf7911785f85f3aa6d94e8be9`.
Fixed thumbnail handoff `hhc-web-api@40bffe8fc222eaa95d192a3d28f939f101e3cff2`:
owner-reported Asset 88 / CMS 169 / Gateway 199 / Website 185. Actual OBS
32.2.2, RTX 4060, three NVENC renditions, 30000/1001 fps and 900-frame
30.03-second segments. No CPU fallback or YouTube/original-profile changes.

Recording `cd07076b-082d-4361-bf29-0e5185de3409`, capture
`85a97e39455d279eb9cc23deb4b44fc3`; shared GPU/upstream with long recording
`08c9f38a-397e-4037-9661-26bab09a2277`, capture
`8057a3c4c2a27d699057f5e5434f9909`. Both encode three native NVENC
renditions. Member playback and read-only observers also run. The synthetic
upload fault affected only the first capture's Qt transport; browsers and
the second capture remained online. This is not a whole-PC/network outage.

| Milestone | Asia/Taipei (+08:00) |
| --- | --- |
| Native encoding started | 2026-10-10 03:09:09.884 |
| Upload fault activated | 03:19:09.931 |
| Transport restored | 03:24:10.077 |
| Durable upload/confirm backlog first zero | 03:33:03.692728 |
| Ten-minute gate | 03:34:10.077 |

First zero was 8 minutes 53.615728 seconds after restoration. At the gate,
independent server reads bracketed 03:34:08.880367-03:34:11.390387:
150 declared objects, queued 9, verified 141, contiguous sequence 45,
media end 1381.38 seconds, `liveState=recovering`, native pending bytes 0.
The original member tab was opened and playing before the fault, with
initial approximately 100-second lag. It was never reloaded or manually
returned to live before or at the gate.

At 03:34:09.247 / 03:34:10.250 / 03:34:11.259 the original player reported
currentTime 1111.482405 / 1112.485148 / 1113.494603 and frames
26114 / 26138 / 26169, actual 1920x1080, unpaused 1x, readyState 4,
dropped frames 0, no media error. Approximate actual lag at the middle
sample was **387.880852 seconds**, with approximately one-second clock
alignment uncertainty. No automatic forward jump was observed through
03:34:33.797. **The shared-load ten-minute 60-120-second gate failed.**
Local upload zero, thumbnail readiness and a live badge do not change it.

The first later automatic forward jump occurred at 03:35:13.247,
11 minutes 3.170 seconds after restoration: currentTime jumped from
1174.4847 to 1411.439999. At 03:37:13.258/03:37:14.251 actual decoded
1080p/1x playback advanced 1530.529418/1531.522425 seconds and
31580/31611 frames, readyState 4, no error or dropped frame. Its lag was
still approximately 152.844575 seconds. Eventual forward movement does not
qualify either the missed ten-minute gate or the required 60-120-second lag.

An intentional pause established before the fault retained exactly
188.507065 seconds / frame 3850 / paused / 1x at the gate. A separate
intentional DVR viewer was established during the outage, at position zero
and 1.5x: gate positions 1238.51853 / 1240.03196 / 1241.520284 with advancing
decoded 1080p frames, no error and no forced forward jump. The latter does
not prove pre-fault DVR retention. These control passes are independent of
the original following viewer's failure.

For comparison, isolated load on the same platform handoff passed: recording
`b384ab52-76fc-4dd8-bd41-7637a6f70ded`, capture
`b95611e8baf1cbce48c7e38a8f4458c8`, downloaded CI product `7bc114f` (same
product source as `d59f1eb`; later commit changes the developer close-path
fixture). Restoration 02:36:53.780, first zero 02:45:14.502827, ten-minute
gate 02:46:53.780, queued 3 / verified 147 / waterline 1441.44, actual
1080p/1x lag approximately 116.5324 seconds. It then sealed, became ready,
automatically published and exited normally. Both tests had Auto thumbnail
work; an image hash is not recovery evidence.

Please correlate capture admission/stage logs for the two current captures
with the timestamps above. Separate upload/confirm time, validation queue,
per-rendition decode and live publication/availability. Also inspect the
member follower's safe playback watermark and recovery predicate. This
evidence does not attribute all delay to one platform component or justify
changing OBS fps/segment count, weakening media validation, or forcing
paused/DVR users to the edge. Wire schema/digest/idempotency remain unchanged.

Windows continues Stop/seal/ready/publication, outage VOD, fixed/per-session
thumbnail acceptance and the independent current-candidate 9000-second run.
The original following viewer remains unmodified for eventual recovery timing.
No token, password or signed URL is included in this handoff.
