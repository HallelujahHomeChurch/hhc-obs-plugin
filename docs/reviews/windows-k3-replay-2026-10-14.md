# K3 targeted ended-live replay, 2026-10-14

All event times below are Asia/Taipei UTC+8. Installed normal candidate is
`5af0fa4a82c473df7bac780fa97779f5ebd57f49`, ZIP SHA-256
`9102cd7541cc4991cbb4e637763bac1bb1438d9ed36abc31943dbac5e2c22dfa`.
No plugin or platform code was changed for this retest.

Recording `45fd5c77-be41-4fe4-a2f2-03ad57b9fe9d`, capture
`61da0f63c09600506282f2cd55bd50f7`, local session
`2944c1d4-0621-4136-aadd-fc794d08c830`. Synthetic timestamp scene,
three NVENC renditions, 30000/1001 fps, 900-frame / 30.03-second grid,
audio track 1 and separate local recording. No YouTube or physical outage.

## Lifecycle and evidence limits

- Source initialized 17:43:47.365–17:43:47.504; local recording started
  17:44:18.632. Auto start/end enabled, auto publish disabled only for this field.
- Initial Console/member tabs were lost to tool turn cleanup before this turn.
  Reopened the same recording at approximately 17:50 without another capture.
  Therefore original-page continuity from before Start is **not verified**.
  Replacement pages were retained without reload across End and replay tests.
- Live decode at 17:51:31.020: 1920x1080, currentTime 384.950922,
  duration 390.421333, 743 frames, zero dropped, 1x, MediaError null.
- End requested immediately after that sample. Durable End ACK boundary 16,
  epoch 1, original local-session ACK key. Public range `[0,16)` = 480.48 s.
- HHC stopped 17:51:48.966. Independent local recording remained active;
  explicitly stopped separately at 17:52:20.203. Global streaming untouched.
- Stop accepted 17:51:53.570139414; seal accepted 17:52:27.212541.
  All 58 inventory objects confirmed, zero pending. Source has 17 segments
  per rendition, 481.247433 s; short source tail is outside public range.
- Full local hash/codec/CFR/grid/cross-rendition/full-decode QA passed all three.
- Ready observed 17:54:18.324, receipt serverNow 17:54:07.678797076.
  Console shows recording complete, waiting for publication, consistent with
  disabled auto publish. No manual publish. No old instruction to stop OBS.

## Replay regression

- 480p naturally reached EOS at 17:53:16.151: 854x480, 2602 frames,
  zero dropped, currentTime = duration = 480.511999, paused, 1x, error null.
- Seek to 0, select 1080p: decoded 1920x1080 at 17:53:37.207,
  currentTime 0, duration 480.48, paused, 1x, readyState 4, error null.
- Auto then seek 120, set 1.5x: 17:53:56.794 decoded 1080p at 120,
  paused, rate 1.5, duration 480.48, readyState 4, error null.
- Single 720p change: 17:54:23.710 decoded 1280x720 at 120, paused,
  rate 1.5, same public duration, readyState 4, error null. Screenshot during
  rebuild retains the decoded timestamp frame, not poster or media error.
- First rapid 480p -> 1080p -> Auto group 17:54:30.417–17:54:39.440:
  by 17:54:53.278 decoded 1080p, currentTime 120, paused, rate 1.5,
  readyState 4, error null. This sampled interval is not exact switch latency.
- Faster group 17:55:00.470–17:55:03.315 reproduced the older failure:
  at 17:55:42.378 and **17:56:47.207**, dimensions/frames 0, currentTime 0,
  rate 1, paused, readyState 0, buffered empty, MediaError null. No reload
  or recovery action. Thus the final observation is 103.892 seconds after
  the final selection; this is an observation interval, not switch latency.
  Retain the recording and original failed replacement page for diagnosis.

HHC output, local recording and timestamp helper are stopped. No additional
capture will be created. This failed field remains needed and is not disposable.

Evidence: `artifacts/k3-replay-20261014/`, browser-samples.json,
outage-samples.jsonl, paused-switch-frame.png, end-keeps-recording.png;
queue local-validation.json. No credentials or signed URLs in report.
