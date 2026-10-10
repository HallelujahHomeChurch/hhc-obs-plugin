# Current CI native Windows 2.5-hour capture

Actual installed product commit `3462fe5c4adb6bcdfd30719ede136aa046e25d07`.
OBS32.2.2, RTX4060/driver617.14, Windows11 build26200. Dedicated portable
OBS and synthetic Program; no YouTube or original operator profile changes.
This run qualifies new-source encoding, separately from the recovery of
old-source Long34. Times below use Asia/Taipei (+08:00).

Recording `8a2a3ce3-5dff-4da6-9350-32932e926642`; capture
`632bfda5df0932c7698cd4713cb5fff3`; local session
`405cabc2-d6e6-49d2-b5f8-6e58aaa3d5dd`.
Fixed handoff `hhc-web-api@40bffe8fc222eaa95d192a3d28f939f101e3cff2`,
owner-reported Asset88/CMS169/Gateway199/Web185/client1.0.53.
C1 `c1-2026-10-08.2`, schema1, all eleven immutable references verified.
Manifest SHA-256
`f702477236ba184853c119ffa9c4da0cc9f6d44cf7911785f85f3aa6d94e8be9`.

## Completed Windows and original live replay evidence

Native encoding05:48:55.867, stop request08:18:56.063.
Original media:907 objects, three renditions each300 segments and
9000.1912 seconds,30000/1001 fps and30.03-second interior playlist durations.
Nominal9000-second tolerance check, all object hashes, aligned timeline,
stream formats and full three-rendition decode passed, exit0 at08:21:40.709336.
The local verifier does not independently inspect every packet's CFR/IDR;
the platform's full media validation remains a separate gate.
No shortened duration, additional segment or changed fps.
Inventory-file SHA-256
`14c7083f77ca2998dc74056114449f9b6304524b8e23f575202964d455480c2e`.
The accepted C1 wire inventoryDigest is a different hash:
`4c11d443d7366114d054a2934c3853834703702c9f91c4b0f74b12f516e7428f`.
Stop accepted08:19:02.475243037, seal accepted08:20:26.998840 with the
original local-session `.stop`/`.seal` operation keys. All907 confirm
receipts were present and local pending bytes zero at08:20:43.070631.

Platform live head ended08:21:30.138609,sequence299/end9000.1912.
The original member page naturally ended08:22:00.874 at9000.213333,
1080p/1x,264267 frames/67 dropped, no sampled fatal HTMLMedia error.
No reload, manual return-to-live or quality change occurred during this run.
This live replay is separate from fresh published-VOD playback.

## Live player gates remain unqualified

8828 playing, decoded1x samples before the actual native stop:
estimated lag p50=116.276907, p95=128.849099, p99=133.113367,
maximum134.879626 seconds;3380 samples exceeded120 seconds.
All8828 samples decoded1080p; no sampled fatal HTMLMedia error.
The estimate uses encoder wall origin minus actual currentTime, not visible
timecode OCR. Post-stop samples are excluded. No outage was injected.
This does not pass the normal120-second p95 gate or qualify outage recovery.
The earlier partial115.277484-second p95 is not a whole-run pass.
Background GUI recovery/render/hash scanning and browser decoding occurred;
whole-GPU resource samples include those loads. It is not strict isolated-GPU
qualification. HLS/network retry errors were not fully instrumented.
The first presented live Auto frame was independently1080p, not480p.
Correlate platform waterlines, media availability and follower decisions;
these observations alone do not establish which component caused the delay.

## Remaining lifecycle checks

At08:24:09.840410 the platform was validating,907 declared,903 verified,
queued4, automatic publication pending, terminalReason null. Stop/seal were
accepted and the local queue was empty. Native OBS remained open awaiting
the platform. Ready, automatic publication, selected-first-Auto JPEG
inheritance, controlled native exit and fresh published-VOD playback are
not counted as passing at this snapshot.

The initial fixture subsequently reached its bounded wait and closed OBS:
08:33:24.2360181, exit0, all three operator-profile hashes unchanged,
`complete=false`,status "伺服器檢查影片中",issue empty. Preserve this
initial result. The profiler reports one unattributed allocation; this is
not leak-free qualification. Numeric encoder-skipped-frame counters were
not independently captured in this fixture. At08:34:26.218330 the platform
still had903 verified/queued4,validating/publication pending, no terminal
error code. Original-session GUI recovery is prepared, preserving identity,
inventory and accepted receipts. The general operator OBS opened08:32:52;
guards refused both recovery and the new concurrency test. No new capture
was created by those attempts. These guard failures are not the planned
original-recording feature RED test. The operator's OBS was not touched.

See [safe machine-readable evidence](windows-long-current-ci-2026-10-10.json).
Raw local journals, media, receipts and player/resource traces are retained.
Current-source original-recording/native-HTTP concurrency is a separate next
test. No merge, signed release or platform deployment occurred.
