# Windows targeted platform follow-up — 2026-10-13

Status: this short/long targeted run reached ready and automatic publication; final VOD decode and selected-cover hash are verified. Platform display failures and the package retry remain unresolved. Times are Asia/Taipei (UTC+8) unless suffixed Z. This is a real isolated OBS synthetic-source test, not actual congregation/YouTube load, CI, or deployment qualification.

## Fixed inputs

- Installed plugin: `9c658aeb76e6fe9dc0b3b3ce37c7088a09e77e82`.
- Unsigned candidate ZIP SHA-256: `b598ac6fe45348de493da2ff69b80d82d19ed7108e9829ef017bba370df0a0e8`. Three runtime files matched the candidate manifest at 17:26.
- Mac handoff: asset-api commit `baabce03b53c518ad6c5e5b3e923335407baea26`, `docs/reviews/live-followup-2026-10-13.md`.
- Read-only preflight observed CMS `hhc-web-api--0000188`, Asset `asset-api--0000091`, enabled writer/HLS/source flags and worker image `sha256:10ebdab641b4e65c8ea264510ca485be709ae27791e36ef3922ba32f86483adc`. Admin `6abde585c4af5072b4c256be54e0d5950fb476eb` is the supplied release identifier; the loaded bundle commit was not independently fingerprinted.
- B1/C1/inventory 1 unchanged; 30000/1001 fps, 900-frame/30.03-second interior segments. No source changes or platform mutation to validation rules.

## Interrupted private preview

Recording `3f85c736-72b2-4f0a-b5f4-48697a817f7a`, capture `2bf17ce6693f40fc2fb43c6ffa0f5544`, local session `d54033c6-cf83-4f10-a613-ab51a54e0995`.

The user requested pause before public Start. HHC was normally stopped at 17:40:43.122; 37 objects reached ready at 17:42:41.566. Console reports canceled and recording remains draft. This never-public case does not qualify normal live automatic publication. Its original Console tab did not survive the paused turn; no same-page-thumbnail claim is made for this field.

## Resumed short field

Recording `ba4b1f22-12e8-4d7f-b2c6-5608992d15a2`, capture `5f0cdc71d88e695a6e44398b24482df3`, local session `b5312733-db2a-432d-9dd3-d7c2d5cc11a3`, epoch 1. Manual Start/End, automatic publication enabled. Original Console tab 77 is retained without reload; original member tab 78 is retained.

| Evidence | Result |
| --- | --- |
| Source begins | Native NVENC initialization at 17:46:31.504; synthetic Program, track 1; global streaming/recording were not started |
| Staff preview | Initial attempts returned 409 `broadcast_state_conflict` before media availability; after validation advanced, actual 854×480 decode, 630 frames, currentTime 22.085501, error null |
| Start | Durable ACK accepted at common sequence 7, original operation key retained |
| Original Console Auto image | By 17:52:50.042 cover state ready; at 17:52:57.776 `目前縮圖` is visible/complete with natural size 1280×720, without reload |
| Live image | HTTP 200 at 17:54:02.255; 64245 JPEG bytes; SHA-256 `bbc8c161f8155784b89e40ba5c72770a2ad0071f2fca7abb2ee9e96066248b4a` |
| Member live | Actual Auto 480p to 1080p progression; at 17:53:27.157: 1920×1080, time 88.212156, 2648 frames, rate 1, playing, no media error |
| End | Console click confirmation observed 17:54:21.549; durable End ACK accepted sequence 16 by 17:54:42.5032845 |
| Public range | `[7,16)`, 9 segments, 270.27 seconds; source continues after End |
| Stop / seal | Stop accepted 17:55:20.081630835; seal accepted 17:55:59.714839; no manual publication |
| Local media | `verify-media.py` exit 0: all 61 hashes, aligned grids and full three-rendition decodes; each 18 segments / 525.892033 seconds |
| Live replay EOS | Original member page at 17:57:39.280: time=duration 270.301999, paused, 8104 frames, error null, seekable [0,270.301999]; this is replay, not yet final VOD |
| Publication UI while validating | Recording management displays `直播已結束 · 完成後自動發布`; no false canceled label |
| Ready | Native durable confirmedReady=true / sealAcknowledged=true; readyAt 17:58:29.326; first package attempt observed. Seal→ready about 149.611 seconds |
| Selected VOD cover | 18:00:33.959 HTTP 200, 64245 bytes, SHA-256 identical to live image. At 18:01:22 the UI settles to three candidates with automatic cover 1 selected; intermediate six-candidate/cover-4 display is retained as a transition observation |
| Automatic publication | By 18:02:24.075, Console phase archived / archive published, revision 13, unchanged [7,16); recording management displays published. No manual publish action |
| VOD regression | Original member page explicitly continues to VOD. First attempt shows retry message; second succeeds without reload. 480p tail from 240 seconds naturally ends at 270.301999 (912 frames, ended=true); Home to 1080p retains paused zero and full 270.27; Auto resumes 1080p at time 60.972328 / 270.27, playing at 1x, error null |
| Public list | API durationSeconds=270.27 and card 4:30. Initial card had no image; independent list reload at approximately 18:07 produces this recording's cover HTTP 200 and loaded 1280×720 image |

### Independent failure

Recording-management live-thumbnail panel initially displays an error. Component-level retry (no unsaved edits) still returns HTTP 404 `capture_not_found` at 17:58:23.506 for `/api/admin/recordings/{recordingId}/captures/{captureId}/cover-settings`. The Console's live image works. On ready, the recording page replaces this panel with VOD cover candidates. This is a separate platform/UI issue; it does not invalidate the observed original-Console Auto image, and it must not be described as fixed.

The first explicit same-page VOD continuation showed `暫時無法播放，請重試`; a second click on the same continuation action succeeded. No network trace was enabled for the first failure, so its cause is unqualified. During the separate member-list reload, several other cover endpoints returned 503 `authorization_unavailable`; this field's cover returned 200. Do not infer that those 503s caused the earlier continuation failure. The initial missing list image recovered only after a list reload; no automatic-image-recovery claim is made.

### Evidence limitations and remaining checks

Browser screenshot capture times out; DOM image completeness/natural dimensions/visibility, actual media counters and image-byte hash are retained, but no screenshot proof is claimed. CDP event buffers reported truncation during preview playback: snapshots and local durable ACKs are evidence, not a complete mutation-network trace. The first local QA invocation rejected a fractional `--seconds` CLI argument; the corrected supported integer 526 invocation exited 0 (actual media duration is 525.892033).

Recommendation-card duration remains pending. Custom-image late-Auto protection and explicit cancellation are separate untested cases for this release. No new representative long-field latency or retry result is claimed. Historical 379/388-second delay, pressure cutoff and weak uncached seek failures remain unchanged. At approximately 18:06 the user extended the meeting-free test window until before Friday; no further permission repetition is necessary within that window.

Sanitized local evidence: `artifacts/b1-platform-followup-20261013/resumed/`. Journals/media and protected originals remain retained. No merge/deployment or original YouTube/profile changes.

## Representative long field (source stopped; ready and automatically published)

Recording `8a43d04c-76d7-4504-843d-81f7689ca995`, capture `688d785ae02be5980b693701856af1d4`, local session `13bd1701-95de-482a-ad49-74f7d5261025`, epoch 1. Original Console tab 76 and member tab 81 are retained. Same installed candidate and fixed platform as the short field.

- Source initialization 18:17:40.352. Concurrent isolated OBS local recording starts 18:18:23.088, writing `D:/Users/IT/Videos/2026-10-13 18-18-22.mkv`. No global streaming or YouTube was started. This is synthetic Program plus local recording, not an actual congregation/YouTube load qualification.
- Initial preview attempts return 409 `broadcast_state_conflict`; at 18:20:50.586 actual preview is 854×480, currentTime 16.941388, 497 decoded frames, zero dropped frames, no media error.
- Start is durably ACKed at common sequence 7. Target public duration is at least 9000 seconds; planned End around 20:52, followed by source Stop and independent processing/readiness observation.
- Custom synthetic cover fixture is uploaded through the UI after Start and saved at approximately 18:23:12. Revision 8 reports `custom/pending`, then `custom/ready` by 18:23:44.342. Original Console nevertheless has no `目前縮圖` image at 18:24:43; a separate fresh Console tab 82 displays the image without changing the original page. The original cover-content request at 18:23:12.149 returned 404 `broadcast_not_found`. The new page's image returns HTTP 200, 66526 bytes, SHA-256 `f40a7a65edeade25c8fe2a6aa352dc08f4a927cfbe8ca0018b701badc11cc901`. This is an unresolved same-page custom-image display failure, independent of the short field's passing Auto case. Late-Auto overwrite protection is not yet qualified.
- Original member tab automatically enters live, then the operator performs the initial Play only. At 18:23:25.488 it decodes 1920×1080, time 43.202665, duration 60.083333, 398 frames, zero dropped frames, playing 1x, error null. This is a startup snapshot, not yet long continuous-viewing acceptance.
- Evidence: `artifacts/b1-platform-followup-20261013/long/`. Browser CDP event buffers can truncate while preview plays; snapshots and durable journals do not imply a complete network trace.
- At 18:28:48.791 the original member page is still playing 1920×1080 at 1x, currentTime 322.018682, 8750 total frames / 7 dropped, media error null. UI estimates 137 seconds of latency. A prior 18:28:29.504 journal sample reports liveState `recovering`, lastSequence 17 and 66 confirm receipts; this is not classified as a disconnect test or failure solely from the label.
- Local read-only journal sampler PID 67248 records every 30 seconds (initial long-path read errors were corrected). App heartbeat `windows-obs-b1` resumes this same task every 10 minutes to observe playback and complete End/Stop/readiness after the target time. It must stop after this field is finished or clearly blocked. No long-field completion is claimed at setup time.

The interrupted never-public draft `3f85c736-72b2-4f0a-b5f4-48697a817f7a` was deleted after readiness under the standing disposable-test authorization. DELETE returned 204; Admin and member listing absence was verified at 18:10:15.019. Its local journal/media remain retained. The short published field remains available for the current recommendation/cover checks and platform diagnostics.

### Long-field ending and validation

- The ten-minute heartbeat resumed at 20:58, after the minimum-duration target; actual End is later than the provisional 20:52 plan. The original Console issues End without reload or login. Durable ACK accepts sequence 323, public range `[7,323)`, 316 segments / 9489.48 seconds. This overrun is scheduling, not a relaxed pressure-test cutoff.
- HHC source Stop: native 21:00:55.055; server accepted 21:00:54.985396908. Console End left both source and concurrent local recording running until separate Stop actions. OBS local recording stops 21:01:03.638; streaming was never started.
- Seal accepted 21:02:30.022578. All 988 objects have confirm receipts. Source contains 327 segments per rendition, 9794.317867 seconds, including private preview and post-End tail.
- Local hash, grid, three-rendition ffprobe and full decode validation exits 0, `passed=true`. `long/local-media-qa.json` retains exact 30000/1001 video and aligned 900-frame interiors. The verifier was called with exact playlist duration through its Python function because its CLI only accepts integer seconds; no tolerance or validator logic was changed.
- Concurrent local file is HEVC 1920×1080 at 30000/1001, AAC, duration 9759.950 seconds, size 12208156721 bytes. Full decode exits 0 at 21:06:22.9944196; stderr is empty.
- Original member tab samples throughout the long field remain 1920×1080 / 1x, media error null; no reload, seek or manual return-to-live was performed. At 21:02:18.798 it naturally ends at 9489.501999 seconds, 275127 total frames / 40 dropped. These periodic samples do not prove absence of every transient stall between observations. This is live replay EOS, not final VOD readiness.
- Platform package attempt 1 starts 21:03:30.615725, phase `package_validation`. Original Console is processing revision 11 with unchanged bounds; recording management correctly says completion will auto-publish. `ready` and publication remain pending.
- Recording management again returns 404 for capture cover-settings and initially for its package lookup; processing progress is available through the recording response. No change is made to the platform, selection, or publication intent.
- Program clock stopped after source Stop. Local journal sampler is temporarily retained through validation to preserve readiness timing. Stop `long/monitor-stop` after readiness or when ending this observation; do not restart capture.
- At approximately 21:09 the recording UI reports attempt 2, elapsed 2 minutes, 61/988 objects, last progress and heartbeat 21:08. The first attempt therefore did not establish readiness. This new retry must be correlated with the deployed worker's claim/stage/result/finish diagnostics; local decode success does not explain or excuse the platform retry. Final readiness and elapsed time remain pending.
- Exact attempt 2 start is 21:06:23.004398. At 21:09:26.962 observation, progress is 1080p, 73/988 objects, 68/981 segments, 767332121/6510746532 bytes; last progress 21:09:19.9087286 and heartbeat 21:09:23.053478. Neither attempt's root cause nor retry reason has been established from Windows evidence.

- Fresh durable readiness confirmation: readyAt 2026-10-13 21:34:14.157 Asia/Taipei (UTC+8), confirmedReady=true, normalEnd=true, sealAcknowledged=true, all 988 confirm receipts. Seal-to-ready is 31m44.134s. Attempt 2 start-to-ready is 27m51.153s; retry reason remains unknown. Automatic publication and final VOD checks are separate and not yet confirmed at this checkpoint. The journal sampler stop marker was created after readiness.

### Final publication and VOD checkpoint

- At 21:39:47.609, the unchanged original Console response reports archived / published, revision 15, custom cover ready, epoch 1, public boundaries still `[7,323)`. No manual publication was performed. Publication is observed no later than 37m17.586s after seal and 5m33.452s after ready; these are observation bounds, not exact server publication latency.
- Original recording-management page had stopped refreshing with a load error, stale draft status and 799/988 progress at 21:30. Its explicit component reload at 21:40 restored published/version 3 by 21:40:22.911. Original Console and original member page were not reloaded. Do not claim the recording-management page updated automatically without intervention.
- Original member page explicitly continues to VOD on the same URL. At 21:40:10.355 it retains the paused end position (9489.48 seconds, 1x), with media duration 9489.501999 and no media error. Home then Play succeeds. At 21:40:33.894 actual decode is 1920×1080, currentTime 11.177668, duration 9489.501999, 337 frames / zero dropped, playing at 1x, error null. Buffered ranges `[0,150.149999]` and `[9459.45,9489.479999]`; seekable `[0,9489.501999]`. The approximately 22 ms container tail is distinguished from the exact 9489.48-second public segment range; preview/post-End segments are not included.
- At 21:40:58.408, selected custom VOD cover returns HTTP 200, 66526 bytes, 1280×720. SHA-256 `f40a7a65edeade25c8fe2a6aa352dc08f4a927cfbe8ca0018b701badc11cc901` equals the live custom cover. Title and recording URL remain unchanged. Original Console custom image still does not render; correct stored cover bytes do not resolve that UI failure.
- All nine original OBS profile/scene/encoder/service file hashes match the preflight baseline. HHC source and isolated local recording are stopped. Program-clock and journal-monitor stop markers exist. No original recording or YouTube setting was modified.

## Handoff and remaining limits

This targeted run verifies real OBS short/long encoding, durable public boundaries, normal stop/seal, full local media decode, platform ready, automatic publication, final VOD decode and inherited selected thumbnail. It is not an overall B1 acceptance pass.

Mac follow-up: correlate capture `688d785ae02be5980b693701856af1d4` / recording `8a43d04c-76d7-4504-843d-81f7689ca995` with worker attempt 1 at 21:03:30.615725, attempt 2 at 21:06:23.004398 and ready at 21:34:14.157 (2026-10-13 UTC+8). Obtain claim/stage/result/finish evidence for the retry and cost of the 31m44s seal-to-ready interval. Separately investigate the original Console custom-ready missing image, capture cover-settings 404, and stale recording-management page requiring component reload. No root cause is inferred solely from a worker exit status or browser display.

Normal congregation/YouTube-load disconnect recovery, controlled weak-network ABR, five-minute outage/ten-minute catch-up pressure, physical disconnection/crash, and every DVR/rate transition remain independently unqualified in this run. Prior 379/388-second delay and 32.730-second uncached seek evidence remain failures/limits. Periodic playback samples do not establish uninterrupted playback at every instant. Late-Auto overwrite protection and thumbnail-default isolation were not newly qualified.

The interrupted unused preview was already deleted and absence verified. The two newly published short/long fields remain temporarily needed for the unresolved platform diagnosis above; retain them with local journals and artifacts. Protected read-only originals `e496a04e-4e2e-4dad-9269-63a610dd7c06` and `e9c5ebb4-74d7-46e0-a7b4-e3730579fc8a`, and all real meeting recordings, are untouched. Delete these disposable current fields once their diagnostic use ends under the standing cleanup authorization.

This report changes documentation only. CI build/unit/media-grid/watcher checks are separate from these real OBS results and must be reported by actual run status. No merge or deployment is authorized by this report.
