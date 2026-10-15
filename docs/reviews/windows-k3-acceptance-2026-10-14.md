# Windows K3 acceptance — 2026-10-14

Status: in progress; not a full end-to-end pass. All event times below use Asia/Taipei (UTC+8).

## Fixed candidate and contract

- Installed plugin: `686dcfecba1a4e5cfc40995d40f219e32ff83964`.
- Unsigned ZIP SHA-256: `3f969109f0b0c712ab2b8deeca5e5b77c94d7bd327062a0d004ef32de267a242`.
- Native CI runs `38503008028` and `38503003598`: success. Local Windows build and 16/16 CTest tests passed.
- K3 `k3-2026-10-14.rc1`, B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory 1. Supplied fixed handoff/contract/OpenAPI/fixtures and A–D report hashes verified before implementation.
- Deployment identifiers are the Mac handoff snapshot, not an independent Azure readback: CMS 0000193, Gateway 0000206, Asset 0000092, Website 0000206; Admin `386099c507642ecf4a552e2ae2664d6dc04f92c1`.
- User confirmed creation/storage of the persistent stream key. Console created it; OBS saved it through its password field to Windows Credential Manager. Clipboard cleared. No key/session values belong in this report or journals.

## Bootstrap and first native short field

The no-prepared-event wait was stopped before preparing a target. Its active pointer released and no bind intent appeared later. A subsequent explicit Start created a new intent.

- Recording: `b7a8d3f4-54cc-4a69-88b6-ccda0d8b7136`.
- Capture: `63163a8265f4f2cc31f97bfddec93665`, epoch 1.
- Local intent: `146c9d85-7f35-46f7-bff4-169bd74c1aa8`.
- Initial candidate rejected a valid B1 `accepted` bind receipt. Fixed in `686dcfe` with regression coverage; retried the original intent and capture, not a replacement.
- Source start: 10:49:42.5 (OBS log).
- Original Console preview decoded at 10:56:02.751: 854×480, currentTime 15.020139, playing, readyState 4. Initial preview attempts before availability showed a retry message; not counted as uninterrupted preview success.
- Auto thumbnail appeared on the original non-reloaded Console; natural size 1280×720.
- Console End ACK: boundary 14, accepted; public scope `[0,14)`.
- Stop accepted: 10:56:44.095812277.
- Seal accepted: 10:57:51.836961; local input released only after receipts.
- Capture ready observed at serverNow 10:59:46.401706689. This is an observation upper bound, approximately 114.565 seconds after seal, not an exact worker completion timestamp.
- Local full decode/hash/codec/timeline check passed for all three renditions: 15 segments, 420.787033 seconds, 30000/1001 fps. The last source segment lies outside the public end boundary; do not use full source duration as public duration.
- Auto publication remains unconfirmed: recording was still a draft at 11:03; original Console response at 11:09:26.615 has phase `processing`, archive `ready`, reason null, policy autoPublish true. No manual publish or cancel was issued. C1 `autoPublish=cancelled` is the existing B1 bridge behavior, not proof of cancellation. Corrected the K3 dock display to defer publication status to Console, matching existing legacy B1 handling; actual installed field binary remains `686dcfe`.

Qualification: the isolated OBS still loaded an old `hhc-obs-fixture.dll`, producing a duplicate `hhc_hls_output` registration message. This short field proves K3 transport/control behavior but does not establish exact-candidate encoder acceptance. After seal and graceful OBS exit, moved that DLL to the evidence backup directory. Restart log at 10:58:26 identifies `686dcfe`, loads only `hhc-obs-plugin.dll`, and contains no duplicate registration. Original installed OBS output settings and YouTube were not changed.

## Second field: same saved key, clean candidate

- Recording: `21edf722-0533-4081-a416-580ab69517f6`.
- Capture: `fd6df71873a5d2a4ac0384a155611ea5`, epoch 1.
- Local intent: `8ef054b3-d23c-4391-a893-30b1aafff4eb`.
- Scheduled metadata: 11:30; autoStart false, autoEnd and autoPublish true. Announced and prepared in Console.
- Native source started 11:03:07.818–11:03:07.960. Existing stored key reused after OBS restart; no OAuth, event picker or metadata entry in OBS.
- Three NVENC encoders and audio track 1 initialized. Original Console retained for preview/Start/End and A–C checks. Acceptance still running.
- Private preview initially returned HTTP 409 `broadcast_state_conflict`; after verified media became available, retried on the same page and decoded 854×480, playing, currentTime 14.43689. This is availability-gated retry evidence, not zero-error startup.
- Start acknowledged at encoder boundary 9 before the scheduled 11:30 time. Member original page initially showed the countdown, then automatically became a player without reload. At 11:09:26 the UI reported 109 seconds estimated latency; this badge alone does not establish decode or recovery success.
- Isolated local OBS recording started around 11:08:19 for independent-output End testing. YouTube remains untouched.

### Short-field completion checkpoint

- Member true decode at 11:10:10.119: 1920×1080, currentTime 36.175312, duration 90.113999, playing at 1x, readyState 4, buffered `[0,90.089999]`, seekable `[0,90.113999]`. These are live-window coordinates, not source-time offsets or a recovery pass.
- End accepted at boundary 16; public scope `[9,16)` = 210.21 seconds. HHC output stopped at 11:11:09.264. Stop accepted 11:11:17.152477929; seal accepted 11:12:21.308683; ready observed at serverNow 11:14:02.457813092 (101.149 seconds after seal, observation bound).
- Separate OBS recording started 11:08:20.412, remained active after HHC stopped (screenshot at 11:11:50.705), and was stopped explicitly by the tester at 11:12:01.562. This proves local recording independence; YouTube streaming was not exercised.
- Local three-rendition full hash/decode/codec/timeline QA passed: 17 source segments, 481.047233 seconds each, 30000/1001 fps. Source material outside `[9,16)` is not public duration.
- Member original-page EOS at 11:15:14.108: 854×480, currentTime=duration=210.241999, paused=true, rate=1, 6304 decoded / 13 dropped frames, no media error; buffered `[32.032,210.241999]`, seekable `[0,210.241999]`. The approximately 32 ms container tail is separate from exact segment range.
- EOS→Home→manual 1080p showed a transient **無法播放媒體** at 11:16:01.199 (0×0, duration unavailable). By 11:16:12.975 it recovered without reload: 1920×1080, time 0, duration 210.21, paused, 1x, readyState 4, error null. Selecting Auto repeated the transient error; by 11:17:48.524 it again decoded 1920×1080 at time 0, paused, 1x, error null. This is a failed no-false-error requirement, not an unrecoverable playback failure. It was ended-live replay, not published VOD; precise switch latency and retained-last-frame visual evidence were not obtained.
- First field at 11:14:51.653: original Console still processing / archive ready / reason null, autoPublish=true, auto cover ready (`677a2722adf24547d32ebbae1e4c0552`). No publish action was substituted. Both fields still appeared as drafts in the Admin list at 11:17.
- Later success: second field's original Console response at **11:20:16.797** reports archived / published, reason null, unchanged `[9,16)`, auto cover ready (`f680987b9103e3ffa4d5554e69130f92`). No manual publication occurred. Observed seal-to-published upper bound 7m55.488s, ready-observation-to-published 6m14.339s. Earlier draft samples are superseded for the second field only. Original member page offers continuation to VOD at the same URL.
- Selected continuation on the original member page without reload. At 11:21:02.351 published VOD retains time 0 / paused / 1x and duration 210.21, decoded 854×480, error null. After Play, 11:21:31.858: currentTime 18.275733, 854×480, duration 210.21, 1x, 557 frames / 5 dropped, no media error. Title and URL unchanged.
- Published recording page selects Auto cover 1. Actual displayed live cover and selected VOD image are both 1280×720, 64114 bytes, SHA-256 `9766b863f7bd6f54cf6659890befbbc7f62ed6ebbb0d07dde21afe655d6c21de`. Hash computed from the already-rendered same-origin blob bytes; no signed URL retained.

## Candidate update and cleanup

The display-only follow-up candidate is `33be386aed287098f23d8421acd0697887cf2c05`, ZIP SHA-256 `231cf1f1099d39105398f6283d9716f2b4e8386b607572464dbc0c37824c2637`. Local 16/16 tests and CI `38504843184` / `38504836041` passed. Installed its DLL/helper after both HHC fields stopped, drained and became ready; it has not been rerun through native capture. Field measurements above remain attributed to `686dcfe`.

Both isolated HHC/local recordings are stopped, input released, isolated OBS gracefully closed and program clock stop marker created. Five original OBS profile/scene/encoder/service hashes match this run's preflight baseline. The unrelated original OBS instance remains untouched. Browser screenshot capture timed out; retained AX/network/media evidence and a native OBS screenshot instead.

Deleted only the two explicitly disposable prior fields `ba4b1f22-12e8-4d7f-b2c6-5608992d15a2` and `8a43d04c-76d7-4504-843d-81f7689ca995`. Verified absence from Admin list, fresh member list and original member recommendations. Their local reports/journals/hash evidence remains. Protected originals, current K3 fields and real meeting recordings remain untouched.

## Evidence and remaining scope

Ignored local evidence: `artifacts/k3-20261014/`, including fixed documents, builds/CTest, original profile hashes, candidate ZIP, first-status.json and first-local-validation.json. Durable intents and queue remain in the isolated OBS plugin configuration. Do not expose credentials or signed media URLs when sampling.

Passed short-field scope: stored-key reuse into separate events, clean candidate preview/Start/End/Stop/seal/ready, local recording independence, second-field automatic publication and same-URL VOD decode, exact public duration, automatic cover inheritance bytes/selection. These do not establish all A–C modes or continuous playback.

Not yet passed: first-field automatic publication, no-false-error ended-replay quality switching, A–C complete custom/default/dirty-edit lifecycle, expired-session real disconnection/reconnect, revocation cleanup, member staff-preview authorization denial, controlled ABR/DVR, normal workload recovery, stress test and current-version 2.5-hour long run. No new automation was created. Previous 379/388-second lag, 32.730-second weak-network seek and prior long-worker retry evidence remain unchanged.

## Independent Windows recovery follow-up (12:00 UTC+8 onward)

Candidate `33be386aed287098f23d8421acd0697887cf2c05` was actually loaded by isolated OBS at 11:57:21.343. The candidate ZIP hash was rechecked unchanged; all 16 local CTest cases passed again. No fixture DLL or duplicate output registration appeared in this startup log. The first field's original Console still displayed processing / waiting for publication at 11:57; this does not supersede the second field's successful publication.

Crash field: recording `124d106b-9326-471c-9005-5f2c949ef234`, capture `99a83cc8ee5983fcb5210f66ad21247f`, local session `7aa83aba-5a99-4d3d-a38c-d812cbd02a79`. Prepared without announcement, autoStart false and autoPublish false; Console confirmed private preview. At **12:01:13.5151612 UTC+8**, terminated only the verified isolated OBS PID 7868, then relaunched the same portable profile/candidate. The unrelated installed OBS PID 17072 was not targeted.

The original capture accepted abort at **12:01:20.424473519 UTC+8**, operation key `7aa83aba-5a99-4d3d-a38c-d812cbd02a79.abort`, reason `capture_incomplete`. By 12:01:32.429 the durable journal retained `normalEnd=false`, `stopIntent=false`, no Stop or seal mutation, original recording/capture IDs, and `active.released=true`. Console subsequently reported failure; the native dock reported aborted and offered a new Start. This passes safe incomplete-capture recovery, not seamless encoding continuation or successful VOD finalization.

At 12:02:00.024, all nine receipted files (three init files and two segments per rendition) remained on disk with matching sizes and SHA-256. No `.tmp` files were present. Evidence: `recovery-crash.json`, `recovery-preserved-hashes.json`, `recovery-before-crash.png`, and original safe queue journals under the ignored evidence/config directories. Pending-byte zero in the terminal aborted UI is not proof that all media uploaded.

Full FFmpeg decode of each retained init + two closed segments passed for all three renditions. The 1080p check used the session parent as its working directory because Windows process creation rejected the longer rendition working-directory path; the same media decoded with exit 0 and no FFmpeg error output.

Revocation field: recording `a067dd3a-0d0e-4e50-9f03-7f1d1dbad685`, capture `cee419d7fdd58add1c26ffa3ed73778c`, local session `756be96b-da0c-4c4f-b41c-73c3d41e6705`. Reused the unchanged stored key after the aborted field released. Auto-start true / auto-publish false; original Console reported live before the operator invoked its emergency-close action with a synthetic-test reason. Preview loading initially returned an operation error; no preview-decoding pass is claimed for this field.

Original-capture abort receipt accepted **12:07:29.320614296 UTC+8**, original operation key `756be96b-da0c-4c4f-b41c-73c3d41e6705.abort`. At 12:07:42.791, durable revoked marker existed, remote state was aborted, input released, normalEnd false, and no remote Stop or seal mutation existed. Local stopIntent true records stopping the failed encoder; it did not turn into a normal Stop request. The separate isolated OBS recording, started at 12:04:15.937, remained running in the screenshot at 12:07:42.545 (3m27s) and was stopped explicitly by the tester at 12:07:55.033. Evidence: `revocation-status.json`, `revoked-keeps-local-recording.png`, original log/journals. This passes real revocation cleanup and local-recording isolation; expired cleanup-only session renewal is not yet proved by this short field.

Five original installed OBS profile/scene/encoder/service hashes still match the preflight baseline after both fault tests. No YouTube stream was started or modified, no automation was created, and no source or wire change was needed. Real disconnection exceeding the session lifetime, normal meeting-equivalent load/ABR/DVR, pressure and the current-version 2.5-hour test remain separate outstanding checks.

All 24 receipted revocation-field files retained their size/hash. Three-rendition full decode passed using the established verifier's `ffmpeg -v error -copyts ... -f null -` invocation. An exploratory invocation without `-copyts` emitted audio DTS warnings from timestamp rewriting despite exit 0; it is not counted as a clean pass. A targeted scan of 39 K3 metadata JSON files found no stream-key/session-token prefixes, token fields or signed-URL signature pattern; this is a scoped check, not a general secret-scanner guarantee.

The completed crash test draft `124d106b-9326-471c-9005-5f2c949ef234` was deleted under standing cleanup authorization and confirmed absent from the Admin and member lists, retaining all local evidence. The revocation field remains for expired-session cleanup testing. Mac's two diagnostic fields and protected originals remain untouched.

At 12:21:47.658306126 UTC+8, the original installed `33be386` helper read fresh server status for the revoked capture after more than 17 minutes from binding. It returned the expected terminal-aborted exit 4. Recording/capture IDs, original abort operation key/receipt, 37 mutation entries and the durable intent SHA-256 remained unchanged; no Stop/seal or new binding was introduced. Evidence: `revocation-before-expired-resume.json` and `revocation-after-expired-resume.json`. This establishes post-lifetime cleanup/status continuity; bearer contents were not inspected or logged.

Developer HTTP outage harness: 18/18 default-build CTest checks pass, including real loopback abort/recovery and production ignoring the marker. A synthetic packaging-root check rejects the fault-enabled cache before packaging. Native sustained-outage acceptance remains pending; see `windows-network-fault-harness.md` for precise scope.
