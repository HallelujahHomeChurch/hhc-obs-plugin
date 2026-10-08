# Execution ledger — Windows plan 2026-10-07

2026-10-08 23:50 Taipei: follow-up Task5 lifecycle audit reproduced completed-worker/pending-Qt-callback logout race. Shared NativeAuth launch/login/logout now guard through callback consumption. RED logout-callback native-oauth; GREEN full10 CTest in2.47s, no HTTP/real credentials. See logout isolation report. Original long-run fixture/processes remain unchanged; source d609993 remains separately frozen before this fix.

2026-10-08 23:47 Taipei: complete remaining Task5 native logout entry and account-view isolation. [Logout evidence](reviews/windows-logout-isolation-2026-10-08.md): RED native-oauth/mock-dock, GREEN real synthetic Credential Manager/lock and Qt checks; full10 CTest cases pass in2.51s. Ruling: preserve an explicit empty account selection at local logout so restart cannot migrate a legacy credential; do not invent remote revocation — cost: this does not revoke website sessions or already-issued server grants. Current ca29821 long-run handles remain live and loaded fixture unchanged; later authentication changes have separate source/acceptance scope.

2026-10-08 23:20 Taipei: completion audit found the common Task6 stop-confirmation requirement was not connected to the actual dock. [Stop confirmation](reviews/windows-stop-confirmation-2026-10-08.md) is now verified RED→GREEN, all10 CTest cases pass, and actual OBS Cancel leaves encoding/stopIntent unchanged before confirmed normal completion. Three61.027633s renditions pass hashes/full decoding with no staging segment copies. This remaining plan implementation is outside the earlier reviewer range. Ruling: implement confirmation in the shared dock so both local and platform controllers receive only confirmed stops; stale confirmations cannot invoke a newer phase — cost: physical keyboard/DPI and fresh positive platform UI still require their separate acceptance.

Latest whole-branch review and focused fix pass: [report](reviews/windows-final-branch-review-2026-10-08.md). Two Important findings fixed, all 10 local tests pass, actual 61-second OBS media and Windows sharing/ACL denial checks pass. Original 8-segment terminal capture is retained, not resealed; quota cleanup approval and fresh positive production/long-run acceptance remain open. Evidence distinguishes precommit working-tree checks from frozen package and hosted CI.

Authority: hhc-web-api e77d63aef2a02f2ddc590e8f5278cf15765d793f, reachable on origin/docs/obs-capture-plans. All four requested plans read. Platform working tree remains read-only. Local plugin branch: feat/windows-capture in .worktrees/windows-capture; no remote guessed.

## Active integration direction — user update 2026-10-07

This decision supersedes earlier requirements to deliver F1/F1-L across machines before integration. No separate file share is required or to be configured. Keep existing media, hashes and reports immutable; supply necessary excerpts only if compatibility diagnosis requires them.

Continue local plugin/controller/recovery development now. Mac owns platform deployment and will provide a fixed API contract revision, service addresses, issuer/client/login settings and readiness notification. Once those arrive, test directly through the plugin: recording, upload, member live playback, offline recovery, transition from live to recording, and automatic publication. Do not require a separate V1 fixture receipt as an advance gate under this updated direction. Direct integration still must verify media compatibility and actual server states; local tests are not E2E acceptance.

Retain Windows media at 30000/1001 (29.97), including exact EXTINF values and 60-frame GOP. Mac owns resolution of the live validator's currently fixed 30 fps contract mismatch. Do not alter OBS FPS or falsify media metadata to pass it. No inferred wire schema, production deployment, merge, or change to existing YouTube/recording is authorized.

## Current stage

Latest2026-10-08 gates and evidence are in [windows-integration-next-gates.md](windows-integration-next-gates.md), superseding historical stage statements below. C1c1-2026-10-08.2 is acknowledged with immutable byte hashes. Current operator candidate864bf3b preserves terminal captures without retrying seal, cancels recovery scans on exit and persists matching-package ready evidence for explicit successful-data inspection. Corrected29.97-grid local9000s, selected audio tracks1–6/mute and actual Studio Mode Program/Preview/transition checks passed on their recorded producers. Original recording plus HHC9000s has completed and passed full media checks. Actual NVENC resource exhaustion and local NTFS ACL/queue-sharing denial checks passed with no CPU fallback or false completion. New candidate native ready/published readback of historical media passed; fresh positive production E2E still awaits exact draft cleanup approval/quota, and the current member-player quality/backward-seek failure remains open. No production-ready acceptance is claimed.

- W0: actual Windows/OBS/GPU/CPU/RAM/profile baseline and pinned SDK are recorded. Original encoding/profile hashes remain unchanged.
- W1: independent Program/selected mixer1–6/three NVENC/fMP4 is implemented; corrected local9000s and Studio Mode/audio routing pass. Original-recording/HHC9000s passed local full hash/grid/format/decode checks. YouTube concurrency is unqualified.
- W2: account-bound atomic journals, bounded immutable queue, recovery and fault handling are implemented with local, loopback and controlled actual OBS evidence. Physical failures remain separate.
- W3: fixed C1c1-2026-10-08.2 bytes are acknowledged. Native PKCE/Credential Manager/refresh/HTTP/stop/seal/abort/live/publication mapping exists; earlier short positive production tests and current terminal recovery are recorded. New current-platform positive E2E needs lawful quota release.
- W4: native GUI/Explorer installation/removal, keyboard focus simulation and actual Studio Mode/audio routing are tested. Ordinary-mode interruption retained verified media and restart detected the unclean stop; crash-prompt normal/safe-mode selection awaits explicit window-display authorization. Physical keyboard/display-DPI/iPhone Safari remain open.
- W5/P1: incomplete. Unsigned candidate, draft PR#1 and hosted CI exist. Current long production tests, sustained outage/DVR/latency and operational/shared-platform acceptance still require evidence; no merge or release.

2026-10-08 20:21 Taipei: two actual local OBS/NVENC queue-commit sharing-denial runs passed the expected-failure checks. User stop persisted, normalEnd/ready/seal remained false, six closed hashes were retained and the denied placeholder was excluded. Retained three-rendition media passed full decode/format checks. Developer runnable check: tests/capture-file-sharing-test.ps1; [evidence](reviews/windows-file-sharing-2026-10-08.md). Ruling: use the complete portable OBS runtime after the standalone setup failed before encoding — only the actual portable runs qualify this fault. At that observation the original-recording/HHC9000-second process was active; later completion is recorded below.

2026-10-08 21:03 Taipei: [original-recording/HHC long QA](reviews/windows-original-concurrent-long-2026-10-08.md) completed. HHC907 objects/300 fragments per rung/9000.024367s and the original9032.924s HEVC/FLAC MKV pass full decoding and hashes, with9000026ms overlap, zero encoding skipped/render lagged frames and actual OBS exit0. Original watcher exit1 from its unsupported requestedSeconds+30 upper duration bound is preserved; corrected original verification exits0. Ruling: original recording waits for HHC queue finalization, so keep finite duration/full-overlap/stop/format/hash/decode checks and remove that unsupported upper bound — cost: excessive finalization latency requires a separate performance check. One allocation remains unattributed.

2026-10-08 21:17 Taipei: [NVENC exhaustion](reviews/windows-nvenc-exhaustion-2026-10-08.md) qualifies actual module-startup session_limit and partial per-rendition initialization failure with12 and10 held sessions respectively. No CPU encoder initialized, no completion/media invented, partial GPU sessions released; independent healthy three-rendition capture fully decoded. Physical GPU removal/encoding-time device failure remain unqualified.

2026-10-08 21:40 Taipei: [NTFS ACL queue denial](reviews/windows-acl-queue-denial-2026-10-08.md) and sharing regression both return0 with expected EncoderFailure, retained six closed hashes and no seal/ready. Exact original ACL restored. Actual stopped-session recovery returns0; both mismatch modes reject17 and ordinary crash recovery remains0. All10 CTest cases pass. Ruling: add an explicit developer --recover-stopped check rather than alter journal stopIntent or loosen the existing crash guard — cost: ordinary GUI and HTTP recovery still need separate acceptance. Platform permission revocation and physical disk/power failure remain unqualified.

## Decisions

2026-10-08 22:19 Taipei: [ready retention/GUI inspection](reviews/windows-ready-retention-2026-10-08.md) fixes the uncalled cleanup predicate and missing production ready metadata. Accepted seal plus authoritative matching-package ready now persist typed local evidence and a stable first-observation clock; recovery hashes remain mandatory before offering eligible owned-session inspection. All10 local unit/mock/loopback tests pass after two observed RED checks. Ruling: explicit Explorer inspection only, no automatic deletion — design treats seven-day deletion as a proposal — cost: operators confirm remote availability before deciding any removal. Fresh C1 all11 byte hashes match; actual original capture readback remains aborted/recording_deleted, so no seal is forced. Current candidate rebuilding and new CI remain separate from these local results.

- User corrected the approved media rate to **30000/1001 (29.97)**. Preserve this actual OBS rate. A 60-frame GOP yields 30.03-second normal segments; retain actual EXTINF rather than rewriting to 30. Global mismatches (including non-NV12/non-limited BT709) reject without changing OBS.
- Native worktree creation returned Not a git repository for parent Projects. Created a new local repo and isolated ignored git worktree manually. Existing repositories and files preserved.
- W1 -> W2: only closed immutable objects enter queue. normalEnd requires explicit user stop, all three successful tails, and matching timelines; it never means server ready/published.
- W2 -> W3/W4: live, auto-publish and record-only remain distinct. Stop intent is not acknowledgment, seal or publication. Credentials/signed URLs must never enter journals.
- Platform ea24e71774a6d965629eccb86a0c8f713ac5a837 handoff explicitly says C1 preparation, not freeze. Reused documented canonical media filenames only, not provisional HTTP schemas.

## Machine and original settings

Windows 11 Pro 10.0.26200 x64; Ryzen 7 7700X 8C/16T; RAM 33485795328 bytes. RTX 4060 8188 MiB, NVIDIA 617.14; integrated AMD also present. OBS 32.2.2 was stopped at initial inspection; Qt 6.11.1.

Original Advanced output: YouTube RTMPS, obs_nvenc_h264_tex CBR 8000 kbps, track 1; local MKV obs_nvenc_hevc_tex CBR 25000 kbps, track mask 1. Program 1920x1080 29.97, 48 kHz stereo. No credentials copied into reports.

VS2022 Build Tools 17.14.37, MSVC 19.44.35228; SDK 10.0.26100; CMake 3.31.6-msvc6. Official OBS 32.2.2 source ba2f32bdf791005443988a4955e963663e16b1ed. Official deps/Qt 2026-07-15 ZIP hashes checked against its CMakePresets.json. Bootstrap rerun passed with existing pinned dependencies; clean-host CI has not run.

## Verification and review

- 2026-10-07 20:44 Taipei: native build passed; CTest 4/4 (capture policy, local journal, local auth, mock dock). One known deprecated obs_add_data_path warning is confined to the developer harness.
- Real libobs fault tests passed: missing NVENC returns failure and releases encoders/output; a blocked segment path cannot become normalEnd and failed capture reports completion.
- Native master generation smoke passed with aligned three-rendition timelines. Finalization checks every referenced segment: FFmpeg trailer return value alone did not detect an injected segment-open failure.
- Journal tests cover account separation, immutable object identity/hash, quotas and corrupt input; power-cut/process-kill matrix has not run. Strict loadPending still fails closed on any issue; scanPending now reports damaged sessions separately while returning only other verified sessions.
- Auth tests cover RFC7636 S256 vector, fresh state, exact loopback origin/path/port, replay/duplicate-state rejection, and a unique synthetic Credential Manager save/read/delete. No real credential was accessed.
- Qt mock screenshots are in artifacts/ui. These demonstrate layout/state rendering, not production action wiring.
- Read-only reviewer covered 7d2b1e9..ec866ea; no critical issue, three important issues fixed: failed fixture completion, source color validation, preserved failure reason. Standalone fixture cadence corrected to 30000/1001. Auth, dock and later master changes were outside that review range.

## F1 and F1-L

F1 producer 3701e79639ef119eb93b90d0c5e76219174f95bb used a separate portable actual OBS frontend, moving synthetic Program with visible timecode and silent stereo. Three renditions: 30.030000 + 30.030000 + 0.934267 = 60.994267 seconds. Full decoding, ffprobe format and object hashes passed locally. This is not Mac V1.

Canonical package artifacts/F1-obs-02.zip: 40678410 bytes; SHA256 1fbb8d260fdf44e2315fb62087c6009393c1c9a02f127387f615d6c0b8323188. Packaging renamed references and added master; media bytes unchanged. The archived machine-progress attachment predates the user's FPS correction; HANDOFF.md and actual media carry the correct 29.97. Keep this archive immutable; issue a new revision for corrected metadata.

F1-L producer is the same 3701e79, separate portable OBS PID 3128, capture started approximately 20:12:32 Taipei. Requested 9000 seconds, expected finish approximately 22:42:32. Preserve loaded DLL while running. Hidden developer helper PID 14064 samples every 30 seconds, then validates and packages if capture completes. Status/logs under artifacts/F1-L-*. Resource figures include short overlapping harness tests (approximately 20:26–20:46); do not present these as isolated production performance qualification. No simultaneous original recording or YouTube test was performed.

## Historical dependencies at initial local-only stage

- Cross-host F1/F1-L delivery and shared location: removed as prerequisites by the user; retain archives for optional compatibility diagnosis.
- Plugin remote and PR target: not provided; no PR or hosted CI can be claimed.
- Fixed contract revision, deployed connection addresses and login/test access: await Mac readiness notification before direct integration.
- Production UI/controller, network retry/resume/cleanup orchestration, remaining fault matrix, concurrent-output runs, candidate install/remove and signing remain unfinished.
- Historical producers retained staging media copies. The whole-branch review fix now moves closed segments into the immutable queue on the same volume without overwrite/copy fallback; existing historical files are preserved. Physical disk exhaustion remains a separate gate.
- F1-L uses the older producer without the later watchdog/finalization fixes. Its eventual media result is evidence only for that producer, not the current build.

No merge, deployment, production plugin install, YouTube action or macOS work has occurred.

## Latest short fixture checkpoint (20:48 Taipei)

Current producer 7a677c74366a3aa585b03c81ad35b53da616c1e9 also passed a new 61-second actual OBS frontend run: normalEnd=true, 60.994267 seconds, aligned 30.03/30.03/0.934267 tails, all three full decodes and hashes passed. Visually checked elapsed timecode at 15 and 59 seconds; it advances correctly. OBS reported one remaining allocation at exit; origin is not isolated, so leak-free teardown is not claimed.

Preferred F1 package: artifacts/F1-obs-03.zip, 40768717 bytes, SHA256 fcfe1ec4d209c9ec78e9e1d4d5a32f4bef82f3b650bdd7ac72c07b03f630011d. Native capture now emits master and canonical filenames. Packaging/QA tools 84baaa2. This supersedes F1-obs-02 for Mac validation; no cross-host receipt yet. The short test overlapped the long run at 20:44:54–20:45:55; preserve this qualification caveat.

## Capture journal integration checkpoint (21:09 Taipei)

CaptureConfig can now bind queueRoot/account/localId to its exact account-isolated media directory. A journal exists before encoder start. The worker checkpoints newly closed objects; only new bytes are hashed per checkpoint, while restart and successful finalization verify all hashes. Stop intent is atomically saved before requesting OBS stop. Session success is committed only after the final inventory file is durable. Standalone F1 fixture mode remains available without a queue binding; the running F1-L producer was not replaced.

Recovery reads atomic rendition close receipts to bridge a crash between object closure and the next session checkpoint. It returns only hash-verified closed objects and never infers normalEnd or remote receipts. Receipt-less orphan files and partial staging files stay on disk for investigation; no automatic deletion or upload is attempted. Recovery does not rewrite journals. Single writer per session is required; CaptureOutput serializes journal access with its own mutex.

Evidence: native build and 4/4 CTest passed. Real libobs normal capture-to-journal completed. At 21:05:36 the separately owned synthetic test process (PID 22260) was deliberately terminated after 6 closed objects were checkpointed; a new process recovered and hashed all 6 with normalEnd=false, stopIntent=false and no server receipts. This is controlled process termination, not Windows power-loss testing and not the OBS frontend. The original F1-L OBS PID 3128 and its monitor remained running.

Additional RED -> GREEN checks cover a close receipt newer than its session journal, immutable checkpoint identity, incomplete finalization rejection, and Windows file-sharing denial of journal replacement (old journal remains valid). A real libobs final-inventory write fault first reproduced an incorrectly successful journal, then passed after correcting commit order. Unit tests for a locked file do not establish physical disk-full/power-cut durability. Network recovery still requires fixed C1 receipt semantics.

Short harness GPU loads at approximately 21:02–21:08 overlapped F1-L; retain this caveat for resource interpretation. Existing F1/media/review ZIPs remain immutable and predate this source change.

## F1-L completed and checked (2026-10-07 23:27 Taipei)

Actual portable OBS capture ran 20:12:32–22:42:32 and exited. The helper completed local QA and packaging at 22:50. All three renditions have 300 segments and identical total duration 8999.991 seconds at 30000/1001. Local full decoding, stream-format checks and per-object hashes passed. Canonical handoff includes 907 media objects (900 segments, 3 init, 3 rendition playlists, master).

Artifact: artifacts/F1-L-handoff-01.zip; 5982993822 bytes; SHA256 6ace26df4b0a3bb71e772d620889e481fb412ba544378559e49fbf4a4398acdb. Archive hash independently recomputed and matched at 23:27. Producer remains 3701e79639ef119eb93b90d0c5e76219174f95bb, not the newer journal implementation. This is a local real OBS HHC-only long-media pass, not concurrent YouTube/recording qualification, Mac V1, E2E, or P1 acceptance. OBS exit log again reports one remaining allocation; root cause is still open. No capture/monitor process remains running.

Mac-accessible shared location and consumer receipt are still missing. No upload was made. Code implementation remains 2666739; background work only recorded, validated and packaged the older producer. The immutable archive was not rewritten when adding this ledger result.

## Local recovery isolation after integration-direction update

Added RecoveryReport/scanPending for per-session recovery outcomes. A corrupt journal or changed media hash is reported as an issue without blocking other verified sessions in that same account; damaged files are neither rewritten nor deleted. Issues use fixed local text instead of copying journal/server data. The existing strict loadPending entry point still refuses partial results, so callers cannot silently ignore failures. Controller/UI use of the report remains to be wired.

RED -> GREEN regression: one valid session alongside one corrupt journal and one modified media object yields exactly one recoverable session and two issues. Other accounts see neither those captures nor their issues. Corrupt bytes and failed media remain intact. Native build and all four CTest suites passed. This is local recovery evidence only; no HTTP, platform login, upload or publication occurred.

## 2026-10-08 native dock, fault checks and repository setup

User supplied git@github.com:HallelujahHomeChurch/hhc-obs-plugin.git. Verified empty remote via SSH, then pushed the existing initial main commit as explicitly requested; did not reinitialize or rewrite history. Feature branch remains separate. GitHub CLI token is invalid (401), while SSH push works. PR creation needs refreshed GitHub login; no secret was requested or printed. Repository is public as created by the user.

LocalController now attaches the native dock, starts/stops an independent CaptureOutput with selected audio mixer, creates an explicit offline-validation account queue, persists local title/FPS metadata, polls completion and displays background RecoveryReport. Live/publication controls are disabled in this local-only mode; it is not claimed as the platform record-only workflow. Closing the dock hides it; closing OBS while capture is active requires confirmation and retains incomplete data. Recovery hashing runs off the UI thread; tests never change the user's production OBS profile.

Actual OBS frontend dock test: 61-second synthetic scene, button start/stop, all three timelines 60.9609 seconds with three segments, full decode/object hashes passed. Journal normalEnd/stopIntent true and remote ready/seal false. OBS exit allocation count was 0 for this run; this does not establish the root cause of the older one-allocation reports. Native production DLL installation smoke confirmed dock present; removal smoke confirmed dock absent and synthetic queue marker unchanged. Both ran only in .deps/portable-short, exit 0 and allocation count 0. Operator ZIP installation/removal uses Explorer, not CLI. No production plugin installation occurred.

Separate fault-only harness injects a low runtime disk reading, missing headers and missing stop packets. All three stopped incomplete with expected DiskLimit/EncoderFailure reasons and retained journals. The stop-timeout regression first failed before injection was wired, then passed. Fault code is compiled only into hhc-capture-faults, never the normal plugin library. These are real libobs with simulated faults, not physical disk-full or power-cut evidence. All four CTest suites pass. Platform integration, concurrency qualification and current-build long-run acceptance remain distinct.

## Review fix pass and hosted CI (2026-10-08)

Fresh review c1c50c5 found four Important lifecycle/outcome issues. Final: fixed close-veto teardown, recovery-future exception on exit, post-start media-stall watchdog and external-stop false success — each reproduced RED and verified GREEN in developer regressions; CTest 4/4. Details in docs/reviews/local-preview-2026-10-08.md. Current native OBS dock short capture passed complete decode/hash/timeline verification, 61.027633 seconds across all three renditions, exit allocation count 0.

Final: minor (deferred): package script does not enforce a build-source stamp; this run builds immediately before packaging and verifies hashes, but a future stale local DLL could be mislabeled.

Final: Ruling: defer wire integration until fixed Mac contract/endpoints/login — local validation remains available; cost is no platform features yet.
Final: Ruling: no Mac compatibility/server publication/E2E inference from local success — cost is production acceptance still pending.
Final: Ruling: older F1-L cannot qualify current build — cost is a new long/concurrent-output run is still required.
Final: Ruling: physical disk-full/power loss/GPU hang are not proven by test-only faults — cost is hardware-failure qualification remains open.
Final: Ruling: review was source analysis, followed by actual fix regressions — cost is remaining untested lifecycle combinations are not certified.

GitHub Actions run 37651614864 on c1c50c5 completed success: clean pinned SDK build, native compilation, four CTest suites, package and artifact upload. New fix commit needs its own CI result. Initial main contains only the repo bootstrap; all implementation is on feat/windows-capture, without merge.

Attempt to start a new 9000-second OBS run plus monitoring helper was rejected by automatic command approval review with only 'blocked by policy'. Nothing in that rejected command executed; no new long capture is running. A bounded 61-second regression ran successfully afterward. Do not report the new long run as launched or passed. GitHub CLI token remains invalid; PR creation is waiting for the user's refreshed login, while SSH push and public CI reads work.
