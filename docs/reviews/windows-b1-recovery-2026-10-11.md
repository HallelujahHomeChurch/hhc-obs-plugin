# B1 recovery retest — 2026-10-11

All times below are Asia/Taipei UTC+8. This is a qualified synthetic Program / concurrent portable OBS recording test, not normal meeting load with actual YouTube. The fault affected the OBS-process Qt HTTP transport only; Windows/browser networking stayed available. No firewall rule was installed. Historical 379/388-second failures remain failures.

## Fixed execution context

- Product source `b259fa747f5454d0c1d9c4bab686484d36646a54`; unsigned local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`.
- Product DLL SHA-256 `885a692bda4a32d64bbe74fe5f273b3374b81de4c0df2505562a7166611d1a6b`; helper `91490295a4a77ca7375ce3be32631c0c4d389c162999eb4bdd2afcff8f8afd7c`.
- Separate developer-only fault fixture SHA-256 `f3994aa2ef75fed6ea4abcd1c5c29f3f7e2423632a6936d23af0a6fb87efc485`; not included in the product ZIP. Product DLL/helper stayed unchanged.
- CMS `hhc-web-api--0000182`, Asset `asset-api--0000090`, Website actual `hhc-web--0000199` / source `46ea3802c8e83e5725eea163b408a7fe9476af05`. B1/C1 fixed artifacts and hashes verified in the [main ledger](windows-b1-retest-2026-10-11.md).
- Recording/broadcast `23276597-1b23-43b1-b4fb-5d23d3bcdd35`; capture `5bb9dcbdfb0d498a0259afad95e0f89a`, epoch 1; local session `e4720893-2e88-4285-bc10-a0577081426c`.
- Synthetic clock, track 1, three NVENC renditions, 30000/1001 fps, 900-frame / 30.03-second segments. Manual Console Start/End, automatic post-event publication, automatic cover.

## Milestones and actual recovery

| UTC+8 time | Observation |
| --- | --- |
| 13:09:49.030 | Bind UI action. NVENC initialized 13:09:51.246/.297/.337; AAC initialized .362. |
| 13:10:04.933 | Global HEVC recording started in the owned portable test profile. No YouTube stream started. |
| 13:10:52.953 | Console Start. Command `331b21a5-ecfc-453b-9f51-85d14ec35663` durably selects encoder common boundary **3**, then original-key ACK accepted. |
| 13:13:41.485 | Original member tab: 1920x1080, time 33.165880, duration 60.096666, 1,004 frames / 0 dropped, playing 1x, error null; estimated delay 106.867s. |
| 13:14:36.030 | Qt HTTP fault starts. Recording continues; original member browser remains online and is not reloaded. |
| 13:15:41.047 | Original Qt proxy automatically restored; actual fault duration 65.017s. |
| 13:16:38.014 | Durable local receipts: 42 objects, 29 confirmed, 13 awaiting confirmation, 82,874,581 bytes pending. Local observer flags are not substituted for upload ACKs. |
| 13:17:11.240 | Original follower stalls at time 150.122334, paused=false, 1x, error null, estimated delay 199.666s. |
| 13:17:41.083 | Follower automatically resumes at 1920x1080 / time 154.052599; 4,628 frames / 7 dropped, error null. No reload or manual return-to-live. |
| 13:19:12.414 | Automatic following catches forward to time 330.301056; 7,142 frames / 13 dropped, estimated delay 140.661s. |
| 13:20:10.137761 | First valid sampled all-upload-confirm-receipts pending-zero: 63 local objects. This is a sample upper bound, 269.091s after transport restoration, not an exact event timestamp. |
| 13:20:11.873526 | Platform: 63 objects, 60 verified, 3 queued, live. New segments continue to create ordinary pending work. |
| 13:21:24.134 | Follower: 1920x1080, time 495.704424 / duration 510.550, 10,268 frames / 25 dropped, error null, 1x; delay 106.978s. |
| 13:22:41.712 | Time 572.157640, 12,559 frames / 32 dropped, delay 108.102s. |
| 13:24:52.095 | Time 702.204562, 16,456 frames / 45 dropped, delay 108.438s. |
| 13:26:40.766 | Time 809.706091 / duration 870.870, 19,678 frames / 53 dropped, delay 109.608s. About five minutes of stable 107–110s delay after recovery; no continuously increasing lag in these samples. |
| 13:25:49.810 | Independent active-DVR tab stays paused at time 0 / rate 1.5 / actual 1080p, 8 frames, error null. No forced return to live during the fault/recovery. |
| 13:26:39.714 | Console End confirmed. Command `0745bd73-162d-4358-b2e4-c9988e21f95b` uses next encoder common boundary **34**, not lagging verified sequence; durable original-key ACK accepted. Public **[3,34)**, nominal 930.93s. |
| 13:27:52.838979 server / 13:27:54.240 native | Normal HHC Stop accepted / output stopped. Clock differences retained. Global recording remains active, then separately stops 13:28:09.861. |
| 13:28:41.475 | Original scope reaches natural end, actual 720p, currentTime=duration=930.965999, 900 frames / 2 dropped, media error null. Scope replayUntil 14:17:28.516471; continuation is within the real deadline. Source replacement resets frame counters. |
| 13:28:52.118667 | Seal/uploadedAt accepted. Original inventory, digest, keys, 29.97 fps and segment count retained. |
| 13:32:21.053461 | Platform ready, all 118 objects verified. |
| 13:35:18.385732 server read | Automatic publication confirmed; CMS version 3. Fresh metadata 13:38 confirms status published / selectedCoverId `0803dc608fccf68b0eb9a2e3d1cd274c-1`. No manual publish. |
| 13:39–13:40 | Original URL explicitly continues to post-event VOD, Console reads archived. Seek into fault-period media truly decodes: 13:40:13.702 actual 1920x1080, currentTime 241.468651 / duration 930.965999, 654 frames / 4 dropped, playing 1x, error null, seekable [0,930.965999]. |

Delay estimate = observation wall clock minus (approximate AAC initialization 13:09:51.362 + start boundary 3×30.03 + media currentTime). The watch mediaStartSeconds differs by about 21ms; these are approximate wall-clock estimates, not millisecond-synchronized clocks. Buffered/seekable and safe watch/control/progress samples are retained in ignored local JSON evidence.

## Complete media and independent recording

Local HLS hash/grid/timeline/full CPU decode verification exited 0 for all three renditions: 37 segments each, 36×30.03s plus 1.568233s tail, **1082.648233s** private capture total. 118 objects / 719,693,206 bytes; inventory SHA-256 `3951fe50cf446967d3fab7c75c3ce2efb5452584b9d264b672fae7f37f027fa5`. All renditions H.264/AAC / 30000/1001 fps, aligned. Public range remains distinct from complete staff capture.

Concurrent global recording independently passes full CPU decode, exit 0: HEVC/AAC, 1920x1080, 30000/1001 fps, 48kHz stereo, **1084.350s**, 1,356,309,269 bytes; SHA-256 `2477e8547dfb374a7741992ed83e2378103a948ff0bed369f5968725a5094ef2`. HHC Stop isolation screenshot is retained. Original nine profile/scene hashes all still match after teardown.

## Separate limitations and observations

- Qualified pass: 65-second OBS Qt transport interruption, synthetic Program plus global recording, pending receipt catch-up, original follower automatic recovery and stable sampled latency, active DVR/pause/rate preservation, normal Stop/seal/ready/automatic publication and fault-period VOD decoding.
- **End/Stop transition waiting remains an observation:** `waiting` at 13:27:18.925 (time 870.838773) to `playing` at 13:27:35.210, approximately **16.285s**. Other rebuilds/waits were shorter. No media `error` event occurred. Final successful publication does not conceal this interruption.
- Console auto image at 13:34:23.328 is 63,262 JPEG bytes, SHA-256 `3462441b1b433c056d7464fee6ea063c449312a4caea7196e5eceeaba326f3c1`. Original live video poster at 13:31:46.650 was a 297-byte placeholder SVG (`2157ca0a22793524f44a1fe055bbdbc04a754ec231535d3d2db642c3f844ab89`). This alone does not establish its cause or all surfaces failing. New VOD selected-image comparison remains separate; the earlier capture's live/VOD JPEG equality stays passed.
- Controlled VOD downshift separately observed on recording `e38353c4-ce2f-4851-a21c-89b3f0afcd97`: strong-network Auto actual 1080p at 13:21:47.747, then CDP 1.1Mbps / 120ms from 13:21:52.425, actual 480p at 13:24:52.085, time 257.068856 / duration 330.355333, 6,218 frames / 0 dropped, playing 1x, error null. Exact switch instant not measured. Temporary network/cache overrides restored 13:25:49.802.
- Not qualified by this result: physical network outage, actual YouTube/normal church load, five-minute/ten-minute pressure indicator, current fixed 2.5-hour continuous viewing, all-object authorization, normally stopped never-public rehearsal, rapid/all playing-state quality combinations. No threshold was lengthened to rewrite older failures.
- Raw incomplete journal reads and local ready-package flags are excluded from upload-backlog calculations. In particular the false 399MB incomplete-read sample is not used.

Safe local evidence: `artifacts/b1-recovery-20261011/` and controlled ABR series in `artifacts/b1-retest-20261011/`. No credential, cookie or signed URL included. This report is evidence, not a complete A–E acceptance declaration.
