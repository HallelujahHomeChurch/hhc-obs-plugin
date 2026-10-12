# Windows B1 pressure retest — 2026-10-12

All times below are Asia/Taipei (UTC+8). This is real isolated OBS on production APIs, with a **synthetic** Program scene and concurrent owned OBS HEVC recording. It does not qualify physical disconnection or the congregation's actual YouTube workload.

## Fixed runtime and platform

- Runtime source: `9c658aeb76e6fe9dc0b3b3ce37c7088a09e77e82`.
- Installed PUSH artifact ZIP SHA-256: `b598ac6fe45348de493da2ff69b80d82d19ed7108e9829ef017bba370df0a0e8`; its manifest and installed product files match. The PR merge artifact was retained separately and **not installed**.
- Native CI PUSH run `38238123926` and PR run `38238128526`: success. CI success is distinct from this test.
- Actual readback: Account `account-api--0000162`; CMS `hhc-web-api--0000187`; Asset `asset-api--0000090`; Website `hhc-web--0000204` / `b4cf65093fa3cc039e8595a1796b7b1973fe1721`; loaded Admin `b8155194972bcd8c16a21e42e9f6d912234e58ec`.
- B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory schema 1 remain unchanged. Capabilities return `next-common-segment`, maxActiveCaptures 1. Encoding remains 30000/1001 fps, 900 frames / 30.03 seconds; no CPU fallback.
- Recording/broadcast `00c23cc8-2df6-4225-8099-5112ace21623`; capture `187f755f92690ec8a61d5a79a96b9fca`; local session `2fe7ed81-ced9-4daf-b45c-7ac474e687d6`; binding epoch 1.

## Five-minute outage and ten-minute cutoff

Only the OBS-process developer Qt transport proxy was faulted. The OS network and original member browser stayed online. No reload, manual return-to-live or manual follower seek was performed.

| Milestone | Time / result |
| --- | --- |
| Qt transport blocked | 19:06:06.406 |
| Transport restored | 19:11:06.414; 300.008 seconds blocked |
| Follower resumes automatically | 19:12:34.768 |
| First decoded 60–120-second delay | 19:19:55.013; 114.669 seconds, subsequently interrupted by waiting |
| Closed local backlog first reaches zero | 19:20:53.850645; 147 closed / 147 confirmation receipts; 586.586 seconds after restoration |
| Last stable sample before cutoff | 19:21:05.014; 1920×1080, currentTime 1107.877731, duration 1111.141333, playing 1×, readyState 4, 21105 frames, 26 dropped, error null; delay **128.128959 seconds** |
| Exact ten-minute cutoff | **19:21:06.414** |
| Automatic next seek | Starts 19:21:05.459; playing resumes **19:21:08.289**, 1.875 seconds after cutoff; delay about 68.15 seconds |

**The strict 60–120-second recovery gate at ten minutes is not passed.** Do not extend the cutoff or use the earlier transient 114.669-second sample to claim stable recovery. Functional automatic recovery and local backlog drainage are observed separately. The last normal local closure before cutoff adds three new pending objects, after the old backlog had drained.

The last retained native C1 snapshot at 19:17:25.738 has 116 declared objects: 111 verified, 4 queued, 1 declared. There is no atomic C1 queued/verified snapshot at the exact cutoff. CMS queuedObjects=null is unknown, not zero.

Latency uses wall-clock minus the burned-in Program clock. At 19:03:44.952, the decoded frame shows 19:01:47.675 at currentTime 78.667690. The clock file updates every 250 ms; later samples use this media-time anchor. A live badge is not a latency measurement.

Outage retries record transport_unavailable/status 0 with backoff up to 60 seconds and paused=false. A separate real HTTP 502 at **19:17:08.699**, code request_rejected, retries after 8 seconds with paused=false. The optional diagnostic identifies an API request rather than a PUT upload but does not identify its exact API stage; no root cause is inferred.

## Normal End, Stop, seal and publication

- Start ACK: accepted at common boundary **8**, command `23417f74-d1c3-479c-9a2b-589811242f42`, original durable operation key retained. Exact Start HTTP time was evicted; it is not reconstructed.
- Same long-open Console End: request **19:24:10.656**, response **19:24:10.765**, HTTP 200 using the renewed bearer with all three recording scopes. Operation key `97948d96-f2fb-4ced-9145-d9f54cb88fc1`; body SHA-256 `a279ac5dab31e5be6e4b8f2d1658b567cfbc0a20d608c247952c84afb8861d75`.
- End ACK accepted at common boundary **56**. Command `c1bf496b-3446-4771-917a-abdd4e361b84`; journal retains its original ACK key.
- Native normal Stop accepted **19:26:12.341040285**; HHC output stops **19:26:15.097**. Seal accepted **19:26:58.830858**, with the original `.seal` key and inventory digest `873adee388a8dd7f1b117234f658ff64260aa5225be938860819f44de99cdd20`.
- Ready at **19:32:09.964201**. Readback at approximately 19:37 confirms natural published recording version 4; no manual Publish was used. Exact publication transaction time has not been established.
- Published B1 readback at **19:42:35.158** is archived with unchanged public range **[8,56)**: 48 public segments / **1441.44 seconds**. Stop did not expand it.
- All **187 objects / 1,186,833,567 bytes** match actual local SHA-256 and size. Three full local rendition decodes pass: 60 source segments each / **1785.3836 seconds**, 30000/1001 fps, aligned 30.03-second interiors and short final tail. This full private source duration is different from the public range.
- Own OBS HEVC recording remained active across End and HHC Stop, then was stopped normally at **19:35:34.900**. YouTube remained idle; no actual YouTube load is qualified.
- Console autoPublish=true remains checked in the native dock, even though the bound capture's standalone C1 bridge reports autoPublish=cancelled. The latter is not the Console publication policy.

## Member replay and VOD

- Original follower/DVR/rate pages remained open through End and readiness. The follower and active DVR/rate pages eventually reached natural EOS; they are not counted as forced pauses or seeks. The 1.5× page retains that rate after EOS.
- Paused DVR remains at **8.612379**, paused=true, rate=1, through the outage, End, Stop and ready. At **19:46:44.234**, ordinary Play from the original replay page successfully advances to 22.001619 at 1080p, error null, within replayUntil **20:27:11.614178**; no reload/login was performed.
- After ordinary Pause and the original page's **接續觀看會後影片** action, VOD restores **33.644428 → 33.644427**, paused=true, 1×, 1920×1080, full public duration 1441.44, error null. This is the explicit transition action, distinct from passive readiness.
- EOS DVR and rate pages keep position within 0.027 seconds and preserve pause/1.5× when choosing VOD; newly selected source has zero decoded frames at the exact EOS position, so this is state preservation rather than a new decode claim.
- Fresh VOD actual decode at **19:46:29.712**: 1920×1080, currentTime 81.819355, 2461 frames, playing 1×, duration 1441.44, error null.
- 480p natural EOS at **19:51:58.236**: 854×480, ended=true, 2026 frames, zero dropped, error null. Browser tail rounding is 1441.466666, not source truncation. GUI return to zero then manual1080p restores actual 1920×1080, four decoded frames, paused=true, 1×, full duration at **19:53:33.503**. Exact switch completion was not sampled; the retained interval is an upper bound, not a precise latency claim.
- VOD seek into the outage interval at currentTime400 subsequently decodes 1080p at **19:56:10.628**, currentTime465.469003, 1972 frames, playing, error null. Actual frame image is retained.
- Fixed selected VOD cover has **66573 JPEG bytes**, SHA-256 **8100f4fb4678624252c6ab990de0b671616a88fbdf61dfff1c49dedfbd1ce5c4**, exactly matching the measured live/shared fixed cover. The selected custom cover and original title/URL persist.

## Remaining evidence and independent long run

Historical 379/388-second lag, 18.491-second rapid quality switch and 32.730-second weak uncached seek remain unchanged. Controlled weak-network ABR and normal congregation + actual YouTube / physical disconnection are not qualified by this pressure run. Prior c2166da scoped weak-network/EOS passes remain attributed to their original runtime and deployment.

The new same-runtime normal long field is recording `b0cd1c3b-74db-4847-b62a-86ab8ae4bffa`, capture `03470f36a8686b42834682bee9a234f8`, local `a2da00e1-2ed2-4884-b8a4-4e86fc0c93d0`. Start HTTP200 at 19:40:58.297 acknowledges common boundary6. It is still running; no 2.5-hour result is claimed. Planned normal End is approximately22:11, after at least150 public minutes. At19:45:59.497, actual1080p shows19:43:52.251 at currentTime163.676034, a **127.246-second** delay.

A fresh Auto-cover Console reproduction at **19:56:43.611** has API coverState=ready / coverId=d868e3f6c33d68dd0382267b848dde70 / mode=auto, but no actual thumbnail image in its cover region. Admin is the actual b815519 version; no reload or mode switching was performed. This is a Mac frontend follow-up, independent from long-run synchronization.

Ignored evidence roots: `artifacts/b1-pressure-9c658ae-20261012/` and `artifacts/b1-normal-long-9c658ae-20261012/`. Runtime observations include bounded read-only page timers/event listeners; finished pressure timers were cleared and their temporary pages closed after export. No credentials, cookie values, signed URLs or raw response bodies are saved in the report. Current/protected recordings and local journals remain preserved; disposable completed test recordings are removed after the remaining needed evidence is secured.
