# K3 fixed candidate long-run acceptance

Capture, full local media QA, platform ready, automatic publication, same-URL
VOD seek/decode and inherited-cover hash passed. Uninterrupted long live viewing
did not pass: retain the observed stalls and26.194-second backward recovery.
The requested new short replay regression follows separately.

- Normal fault-disabled candidate `5af0fa4a82c473df7bac780fa97779f5ebd57f49`.
- ZIP SHA-256 `9102cd7541cc4991cbb4e637763bac1bb1438d9ed36abc31943dbac5e2c22dfa`.
- Native startup independently logs source 5af0fa4, no fault-build warning.
- Recording `a57859d4-364e-4f24-9589-209294de5d18`.
- Capture `77044fc2351d448bbf2622be9b9ded2c`.
- Local `fd2cf719-0f7b-4249-91b5-f235957d29d6`.
- Source starts **2026-10-14 14:31:37.849 Asia/Taipei UTC+8**.
- Separate portable local recording starts **14:32:13.034 UTC+8**.
- End no earlier than **17:06 UTC+8**, subject to at least 2.5 hours of actual
  continuous member playback as well as source capture. Console tab 20;
  original follower tab 21, no reload.
- Same synthetic timestamp scene, three NVENC renditions, audio track 1,
  30000/1001 fps, 900-frame segments. No YouTube or original profile changes.
- Evidence `artifacts/k3-long-20261014/`; safe journal sampler PID 13540
  every ten seconds, stops on `monitor-stop` or four-hour deadline. Clock
  stops on `clock-stop`. These are local test helpers, not a cron/automation.
- Some external journal reads collide with atomic replacement and are
  recorded as sampleUnavailable; this is not media loss. Native reader
  sharing tests remain distinct from the Python observer.

Short fault-field publication was observed 14:29:36.864, with boundaries
unchanged and no manual publish. Its rapid-switch failure remains retained
on tab18 and recording94219ab8, separate from this normal-binary field.

## Incoming Mac handoff during the run

At14:51–14:52 UTC+8 received and downloaded immutable Admin document at
`350825d5967c7ae57e2ff6c6eb6f116313a17dac`,
`docs/reviews/windows-k3-finalization-followup-2026-10-14.md`.
Verified SHA-256:
`965497d834e3a2a16552503f1416a44aa0a371ce486a1c09ac86aa507b949849`.
Retained exact bytes in the long-run artifact directory.

Handoff reports Asset93 / CMS194 / Website207 and Admin350825d deployed;
these are Mac deployment evidence, not independent Azure readback by Windows.
Contracts and media requirements unchanged. Existing field and original pages
continue; no second concurrent capture. After this field completes, perform
the requested new short ended-live replay test with fresh frontend pages,
preserving both original comparison recordings.

Read-only first-field Console confirmed archived and recording Admin published
at14:52 UTC+8. Original live cover at14:52:47.587:65539 bytes,1280x720,
SHA-256 `01816b45b363ac1701cd6cd24ef2b947e0d75351d4d5f77d90b8e69108629785`,
matching the fixed Mac handoff. No Stop/seal replay, upload, manual publication
or changes to either retained comparison recording.
At14:54:54.335 original-URL VOD decoded854x480,609 frames, currentTime20.041082 /
duration420.42, paused deliberately after playback, rate1, MediaError null.
Selected Admin VOD cover at14:54:54.349 has the same65539 bytes and SHA-256.
This passes the targeted retained-first-field readback, not a new complete watch.
Old long-open recording Admin still displays a cover-settings load warning;
the selected cover itself loads and hashes correctly. Preserve that distinction.

## Early long-run observations

Original member Play at14:35:13.315. At14:42:04.799 source-clock latency
was212.726 seconds; at14:44:46.820 it recovered automatically to125.507,
and at14:49:40.426 to65.477. All samples decoded1920x1080, playing1x,
MediaError null. No reload or manual return-to-live. Intermittent observations
do not prove absence of brief stalls; full long-run status remains pending.
Live cover1280x720/64231 bytes SHA-256
`047b4a60662a2392a8ed81c97dec7f33e5f1306104ba48a6be7f24a138eab55d`.

At15:30:44.207 sampled readyState2 at3423.408717 with buffered end3423.419999;
the page subsequently resumed itself. More materially, samples15:35:18.561,
15:35:23.572 and15:35:28.595 stayed at3753.759063, frames98659, readyState2,
buffered end3753.749999, duration3753.791999, error null. At15:35:33.612 the
page resumed at3727.565097, readyState4, frames98810: a26.194-second backward
movement without tester seek/reload/return-to-live. Platform health was healthy,
verified end3753.75, last publication15:34:42.627241, queued null. Preserve this
as a follower-stall/backward-recovery failure, even though latency remained
within the broad2–3-minute target. Do not claim uninterrupted long viewing.

## End, Stop, seal and local QA

- Original Console End yielded durable ACK boundary311, epoch1, with original
  key `fd2cf719-0f7b-4249-91b5-f235957d29d6.ack.5db9a018-ef88-40f0-8256-fe2fd90d2f73`.
- Public range remains `[0,311)` =9339.33 seconds; no tail expansion.
- Native source stopped **17:07:18.310 UTC+8**.
- Stop accepted **17:07:21.616002641 UTC+8**, original `.stop` key.
- Separate local recording demonstrably continued after End; explicit tester
  stop **17:07:58.040 UTC+8**. HEVC1080p/AAC,30000/1001 fps,9344.435 seconds.
  Independent full FFmpeg decode exited0 with empty stderr; probe and decode
  are distinct from the three HHC rendition checks.
- Seal accepted **17:09:13.309649 UTC+8**, original `.seal` key.
-943 inventory objects confirmed including master; zero pending.
- All three source renditions have312 segments,9340.2309 seconds, final tail
  0.9009 seconds. The last short source tail is outside the public range.
- Existing verifier full hash/codec/CFR/grid/cross-rendition/full decode passed
  all three. Called its unchanged Python function with expected9340.2 seconds
  because its CLI accepts only integer duration. No media modification.
- Original Console end confirmation said other OBS outputs remain unaffected.
  End-pending and processing displayed no old stop-OBS instruction. Phase
  processing observed17:10:16.175; ready/published not yet claimed.
- Original member naturally reached EOS9339.370666, paused1x,1080p,
  266989 decoded frames,212 dropped, MediaError null at17:10:16.132.

Live viewing sample summary:1260 observations from14:35:13.315 through
17:06:36.703, with denser five-second sampling after14:55. Sampled source-clock
latency min65.477 / median123.252 / p95133.850 / max212.726 seconds. Eight
readyState<3 samples, zero non-null MediaErrors, one observed backward transition.
These are sampled statistics, not an exact stall-duration or uninterrupted-watch
measurement. Original page had no reload, manual live catch-up, seek or pause
until natural EOS. Dense samples and all exceptions remain in local evidence.

Platform attempt1 started17:09:27.026979 UTC+8,14 seconds after seal acceptance.
Admin processingProgress provides objects/segments/bytes and heartbeat; its
sanitized values are saved in `processing-samples.json`. At17:22:17.828 it
reported305/943 objects,1080p, still attempt1. No additional capture is started
while measuring this field's validation timing. Ready is not inferred from
worker activity, progress or local QA success.

## Final platform and VOD readback

First durable local ready observation17:36:01.366, serverNow17:35:58.708787610
UTC+8. Seal-to-ready upper bound1605.399 seconds (26m45.399s). Only attempt1
observed, through1080p/720p/480p; no retry. This is a successful current long-field
validation, not proof of the root cause of the historical57-minute wait.

Automatic published/archived observed **17:40:46.310 UTC+8**, version3 in the
recording response; Console boundary revision5 still `[0,311)`. Seal-to-published
upper bound1893.000661 seconds (31m33.001s), ready-to-published upper bound287.601s.
No manual publish or Stop/seal replay. Original Console shows the correct phases
without the removed stop-OBS instruction.

Selected VOD cover at17:37:24.935 has64231 bytes,1280x720 and SHA-256
`047b4a60662a2392a8ed81c97dec7f33e5f1306104ba48a6be7f24a138eab55d`, identical
to the live cover. Original member URL transitioned using its explicit continue
button, not reload. VOD duration9339.370666 (audio/container tail ~0.041s beyond
the9339.33 video boundary). Actual1080p decode at3617.762444 at17:41:57.022,
then9024.168251 at17:42:43.367 after seeks to3600 and9000. ReadyState4, playing1x,
MediaError null; paused deliberately after saving evidence. Full source decode
and public-duration checks remain distinct from these sampled VOD seeks.

Clock and safe monitor stopped; OBS HHC and independent recording stopped.
Retain this recording for the follower-stall/backward-recovery diagnosis.
