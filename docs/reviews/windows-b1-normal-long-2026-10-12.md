# Windows B1 normal long-field evidence

Times are 2026-10-12 Asia/Taipei UTC+8. This report separates the current field, completed browser checks, native CI and historical failures. **Source recording has stopped normally and full local media QA passes; as of22:37 platform ready/publication remain pending.**

## Fixed identities and authorization

- Actual running source: `9c658aeb76e6fe9dc0b3b3ce37c7088a09e77e82`.
- Installed PUSH ZIP SHA-256: `b598ac6fe45348de493da2ff69b80d82d19ed7108e9829ef017bba370df0a0e8`. Source9c658ae native PUSH38238123926 and PR38238128526 pass; report-head a93017f PUSH38246611556 and PR38246618726 also pass. These are distinct from OBS/platform qualification.
- Recording/broadcast `b0cd1c3b-74db-4847-b62a-86ab8ae4bffa`; capture `03470f36a8686b42834682bee9a234f8`; local `a2da00e1-2ed2-4884-b8a4-4e86fc0c93d0`; binding epoch1.
- Actual platform: Account0162, CMS0187/writer enabled, Asset0090/HLS+source enabled, Website0204/b4cf65093fa3cc039e8595a1796b7b1973fe1721, loaded Admin b8155194972bcd8c16a21e42e9f6d912234e58ec. Earlier deployment handoffs are not substituted for these actual versions.
- B1 b1-2026-10-10.rc1 / document b1-mac-2026-10-10.rc2; C1 c1-2026-10-08.2; inventory1. Existing pinned contract acknowledgement/hash evidence remains unchanged. No fake fixture hashes are uploaded.
- The user authorized testing until2026-10-13 06:00 UTC+8. Existing original OBS profiles/YouTube settings are preserved. No merge, main push or deployment is performed.

## Source and current progress

Three NVENC H.264 outputs retain30000/1001 fps and900 frames/30.03seconds; audio track1/AAC128kbps,48kHz/stereo. No CPU fallback. Synthetic Program timecode source starts at19:38:08.56–.686; own isolated OBS HEVC recording starts at19:39:03.972. This is synthetic Program plus an actual concurrent local recording, not a qualified actual congregation/YouTube load.

Same original Console Start request19:40:58.160 / HTTP20019:40:58.297, original operationKey f26c4f25-d10c-41d2-989b-ab0cee7a7901, bodySHA2565bfa8cfaaa75aa59d95323e47f21ab19caaf7e3981330388a24e053d89acd39d. Durable Start command d81b168f-4f8f-4672-b454-fb5899f10817 acknowledges next common boundary6. Planned normal End is approximately22:11 after at least150public minutes, followed by native HHC Stop, seal, complete validation and natural Console publication. No injected upload outage is used in this field.

At21:09:55.3518496, native C1 readback has550declared objects/3644416082bytes:546verified,4queued, no immutable hash/size mismatches against retained local journal. Capture remains uploading/live, no Stop or seal accepted; expiration is2026-10-13 19:38:03.604730532 UTC+8. Local/control timestamps continue to advance. A brief recovering metadata state around21:08 is retained; it does not establish a player outage.

The B1-bound C1 bridge reports autoPublish=cancelled while Console owns publication policy. This is not substituted for the broadcast's policy or final publication outcome. At21:24 the same original Console still shows the checked, disabled "直播結束後發布錄影" option; final publication must be read back independently. At21:24:15.6154627,636objects/4212037401bytes are declared,630verified and6queued, immutableMismatches empty. Original follower still decodes1080p at21:22:27.303, currentTime5942.156420/duration5976.001333,176246frames/13dropped, playing1×/error null, anchor delay136.571614seconds; paused DVR remains4764.705135/1.5×.

At21:37:15.6530982, all714closed local objects have accepted confirmation receipts; the native status advances beyond the earlier field's stalled sequence235. At21:39:00.6900693, platform725objects/4807879193bytes are declared,723verified/2queued, immutableMismatches empty. Native synchronization reaches238 and control timestamps remain fresh. This field has crossed two source hours without reproducing the old stall; it does not identify or prove a fix for the old cause. The original follower at21:38:36.186 decodes1080p, currentTime6911.019060/duration6936.961333,205283frames/13dropped, playing1×/error null, anchor delay136.591974seconds.

Original follower stays on the same page without reload, manual seek or return-to-live. Actual1080p at21:05:31.181: currentTime4926.056263, duration4985.003999,145794frames/13dropped, playing1×, readyState4, error null, anchor delay136.549771seconds. Earlier13–15-second waiting and another8.303-second waiting are preserved; uninterrupted/zero-buffering playback is not claimed.

At20:46:15.759, a retained actual decoded1080p frame shows burned-in20:43:58.660: actual clock delay137.099seconds. Initial media-time anchor estimates136.525227seconds, a0.573773-second difference. Clock-source sampling/drift uncertainty is explicit and no cause is assigned. Live badge text is not the delay measurement.

A second retained1080p frame at21:52:53.837 shows burned-in21:50:36.748, actual delay137.089seconds. Its anchor estimate136.610706seconds differs by0.478294seconds. Actual source-clock observations remain consistent with roughly137seconds; no drift cause is inferred. Frame file hour-2-clock.jpg and sanitized metadata are retained.

## Normal End, Stop and seal

The same original Console sends End at **22:11:30.547**, HTTP200 **22:11:30.684**, operationKey **5f13fd13-67cf-4acf-bd40-99974f47dd10**, bodySHA256 **a7a145f9aa436bcaae5c36a0e23601f11e2dc7a05a2179d39c0f2a54e5541763**, with a naturally refreshed bearer. The actual next-common boundary is307; durable End command **42f8458d-a69b-46d6-8272-4685e1c1eefc** retains key a2da00e1-2ed2-4884-b8a4-4e86fc0c93d0.ack.42f8458d-a69b-46d6-8272-4685e1c1eefc. Public range **[6,307)** is301segments/**9039.03seconds**; the planned approximate22:11 End is executed in that minute, not at an invented verified boundary.

Own HEVC recording remains active and its file continues growing after End. Native HHC Stop completes encoder shutdown at **22:14:02.672**; server Stop accepts original a2da00e1-2ed2-4884-b8a4-4e86fc0c93d0.stop at **22:14:06.949439369**. The journal initially records stopIntent=true while finalization is unfinished; normalEnd=true is subsequently observed, without rewriting inventory or pretending the transient state was ready. Own global HEVC output is separately stopped normally at **22:15:59.821**, after HHC Stop; it reports282176output frames/2render-lag frames. No YouTube output is started or changed.

Final native source inventory has **943objects /6217876837bytes**, three matching **312segments /9353.744400seconds**,30000/1001 fps. Original seal key a2da00e1-2ed2-4884-b8a4-4e86fc0c93d0.seal is accepted **22:15:31.532366**, inventoryDigest **759ad61aa6359ea05404493004470c61e5a9dcf5cbbb05d6ec9b4785af0caf07**, schema1. All943closed objects have confirmation receipts by22:16:28; pending zero. At22:18:13.4512774, platform is validating:939verified/4queued, immutableMismatches empty, replayUntil **2026-10-13 01:22:22.528096 UTC+8**. These do not yet establish ready or publication.

Original follower naturally reaches EOS **22:14:04.296**, actual1080p/currentTime=duration **9039.062666**,269053frames/13dropped,error null, paused1×. The End→processing active DVR keeps playing1.5× at22:17:35.088, currentTime4068.459781/122431frames/error null, without a forced jump. The paused player stays **4764.705135**,paused1.5×/15107frames/error null. Original pages are not reloaded; ready/publication transition and explicit VOD continuation remain pending.

Full local inventory hashes, aligned timelines and complete three-rendition/global HEVC decoding are launched only after their normal finalization. Hardware readback22:02:52: Windows11Pro10.0.26200, Ryzen7 7700X8cores/16threads, visible RAM33485795328bytes, RTX4060/driver617.14/8188MiB. Actual portable OBS PID31780 uses the verified isolated executable. The installed candidate remains9c658ae; report edits do not replace its runtime identity.

The validator exits **0** by22:20:46. All943local inventory objects match actual SHA-256 and sizes. Every rendition fully decodes without error, has312aligned segments/9353.744400seconds, correct H.264/AAC format and30000/1001 fps; all interior segments remain900frames/30.03s. Concurrent global HEVC1920×1080/30000/1001/48kHz-stereo fully decodes282175frames, duration **9415.239seconds**, without decoder error. File D:/Users/IT/Videos/2026-10-12 19-39-03.mkv SHA-256 **c480c7997937e419123cc98ec2d8f5bd779b21315a84076735f6807019f5b25d**. This is finalized local QA, not a substitute for platform ready/publication or actual YouTube load. Source-full-validation-safe.json and global-recording-validation-safe.json remain retained.

Original profile preservation is checked at22:24:14.7519212 against the same-day10:14:28.3559928 baseline: all9tracked original settings/profile/scene hashes match. No restoration from an older baseline is performed.

At22:37:24.4676077, the authenticated native readback still reports validating,939verified/4queued, immutableMismatches empty, terminalReason null. The original Console at22:36:54.974 remains processing/revision13, Console autoPublish=true, fixed boundaries[6,307)/revision4. CMS queuedObjects=null is unknown, not zero. The active DVR still decodes1080p at5808.266707,174573frames, playing1.5x/error null; the paused DVR remains4764.705135/paused1.5x. Follower EOS is preserved; its wall-clock-minus-currentTime after EOS is not interpreted as growing live latency.

Read-only Azure diagnostics identify execution asset-recording-validation-29863576, started22:16:00 and still Running at22:37. Its capture-specific logs match03470f36a8686b42834682bee9a234f8: validation stage22:16:22.7527894/4735ms, stable_publication22:16:27.4506427/4697ms, playlists22:16:28.4189098/967ms, pointer22:16:28.7339546/315ms. These stage names establish partial progress, not final ready/publication or the cause of subsequent delay. Actual job image is alive.azurecr.io/alive/asset-recording@sha256:b4aa0750b6d6547a67196c625aab23905b6088668cb207ad575f92acf9a48345,4CPU/8Gi, scheduled execution/parallelism1/21600s replica timeout. Execution parallelism does not establish the number of in-process validation slots. No platform settings/jobs are changed. Only whitelisted times/stages/scalar metrics are retained, without raw logs or signed URLs.

## Two actual browser refresh cycles

The original Console is not reloaded or relogged in. It records natural expired-bearer GET401 at **20:40:00.718**, refresh POST20:40:00.719 / HTTP20020:40:00.843; and GET401 at **20:56:04.982**, refresh POST20:56:04.983 / HTTP20020:56:05.124. Both actual response bodies have numeric expires_in900; access tokens are inspected only in memory and are never retained. Subsequent GETs and ordinary preview requests use renewed bearer claims with all three recording permissions.

Renewed preview at20:58:41.178 actually decodes854×480, currentTime100.120511,2955frames, playing1×, readyState4/error null. These are two real **GET refresh/retry chains plus renewed preview use**. They are not production evidence that each Create/Start/End mutation individually receives401 and retries; the platform's AuthProvider mutation retry tests remain mock/CI evidence. CDP event eviction is recorded, so complete uninterrupted HTTP history is not claimed.

## Controlled Auto and quality checks

An independent second member player is used from approximately20:29. Its additional browser decoder/GPU load is separate from the original follower; the OBS/OS/uplink/follower network is not altered. Auto is actually1080p on strong network before throttling. Forced480p first frame is not a blocking criterion.

Only the observed media origin on this second player is limited to220000bytes/s and120ms latency at20:30:47.000; other requests remain100Mbps/20ms. Auto first sampled actual480p at20:31:55.575. Waiting20:31:56.894→20:32:08.582 lasts11.688seconds; subsequent480p playback continues without new waiting until restoration20:36:24.776. Thus downgrade/recovery is observed, but this transition is not zero-buffering. Resource timings show roughly14-second480p media requests; cross-origin transferSize/bodySize are unavailable, so no calibrated exact throughput is claimed. The per-tab rules and cache setting are restored.

After restoration, Auto actually decodes720p at20:38:09.570 and1080p at20:38:39.570,134.794seconds after restoration, with error null. No manual quality selection is used to obtain this upgrade.

Paused at1.5×, the original currentTime **3505.403310** is retained across480p/720p/1080p/Auto source changes. First fully restored actual frames appear within sampled upper bounds **2.978 /3.554 /2.949 /2.615seconds**, respectively. Every rebuild has a visible nonempty last-frame canvas and no observed media error. Completion excludes old frames before the observed source rebuild.

Playing DVR at1.5×, the same four choices restore actual decoded media within sampled upper bounds **2.584 /2.339 /2.912 /2.728seconds**, preserve rate/playing intent and position consistent with elapsed playback, and retain nonempty last frames. They do not jump to the live edge.

Continuous UI quality selections at20:59:43.450/46.472/49.486/52.484 choose480p/720p/1080p/Auto. Final actual1080p resumes20:59:55.563,3.079seconds after the final choice and12.113seconds after the first; currentTime4261.383066, playing1.5×/22frames/error null. No false media-error text is observed. This is strong-network **live DVR**, not a replacement for the historical18.491-second VOD switch or32.730-second uncached weak seek limits.

The second player is subsequently paused at21:05:31.171, currentTime4764.705135,1080p/15107frames/rate1.5/error null. It remains paused at that position while the normal field continues; End→VOD state preservation is still pending for this field.

A separate active DVR player opens around21:31 and uses ordinary UI Home plus1.5× playback, not programmatic seeking or forced follow-live. At21:32:39.596 it actually decodes1080p at25.312496, playing1.5×/1258frames/error null; at21:37:06.162 it advances to425.149597/13241frames without jumping to live. Its extra decoder load is disclosed. Same-page End/Stop/processing/ready/publication behavior for this active DVR remains pending, separately from the untouched follower and paused second player.

## Auto cover and tool-limited checks

Current Auto-only live cover actually loads1280×720 in a separate member list. Authenticated observed cover readback20:26:42.722 returns64038JPEG bytes, SHA-256 **52a42c15e8bf0d29ab24cf2e006778ff2929fbd71518d93ddbf1dce2d729ad75**. Empty CDP binary bodies are discarded as hash evidence. Temporary service-worker bypass on the independent list was restored; no original Console/follower reload is used.

The original Console still lacks the ready Auto cover image. See [Mac same-page follow-up](mac-auto-cover-original-page-followup-2026-10-12.md); this is independent from encoding. VOD byte inheritance is pending until this field is published.

Direct same-capture member credential isolation probes return browser TypeError, including the public baseline; a separate media-origin diagnostic tab is blocked by the browser client and closed without weakening protections. Thus this field's staff-preview/beforeStart **HTTP rejection is unmeasured**, not a platform denial/pass or media failure. Prior scoped results remain attributed to their original field/deployment. Browser screenshot capture also times out; no successful cleanup screenshot is claimed. Actual decoded frame images are retained separately.

## Remaining finalization and retained failures

Normal End/Stop/seal, fixed [6,307), all local hashes and full three-rendition/global-recording decodes pass their stated scopes; platform ready/publication remain pending. The validator refuses normalEnd=false or duration<9000; no inventory, duration, fps or operation keys are changed to manufacture readiness.

The earlier c2166da field stalls around15:06, then reaches its10GB guard at17:14 after the intended End was missed. Its original capture is safely aborted with journals/media intact; no root cause is inferred from stale status timestamps. Source9c658ae adds safe HTTP pause/retry evidence, not a claimed stall fix. Historical379/388-second lag and the current pressure field's strict128-second ten-minute result remain failures; see [pressure report](windows-b1-pressure-2026-10-12.md).

Physical disconnection, actual simultaneous YouTube/congregation load, native Safari/fullscreen and signing remain unqualified. Ignored raw-safe evidence is under artifacts/b1-normal-long-9c658ae-20261012; no token/cookie/password, signedURL or raw credential-bearing response is saved. Finished disposable pressure00c23cc8 is removed from Admin/member lists. The aborted old1a336a82draft is also deleted after evidence retention; at22:39:06.020 its title/ID are absent from Admin and the member recommendation list. Browser screenshot capture remains unavailable, so no successful cleanup screenshot is claimed. Current long, both explicitly read-only protected originals and all local failure evidence remain.
