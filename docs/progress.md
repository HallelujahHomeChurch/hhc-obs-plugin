# Execution ledger — Windows plan 2026-10-07

Authority: hhc-web-api e77d63aef2a02f2ddc590e8f5278cf15765d793f, reachable on origin/docs/obs-capture-plans. All four requested plans read. Platform working tree remains read-only. Local plugin branch: feat/windows-capture in .worktrees/windows-capture; no remote guessed.

## Current stage

- W0: actual machine/profile baseline and pinned native SDK build completed locally. Original OBS profiles, YouTube and recording settings are preserved.
- W1: independent Program output, selected mixer 1–6, three NVENC H.264 renditions and HLS fMP4 implemented as a native prototype. Real OBS frontend F1 passed local QA. F1-L is running; concurrency qualification is pending. This is not W1 acceptance or P1 yet.
- W2: account-bound atomic local journal and recovery primitives tested independently. Capture now checkpoints into the account-bound journal when queue identity is supplied; no uploader is implemented. No HTTP, guessed wire schema, stop/seal/abort receipt mapping, or server publication exists.
- W3: blocked on frozen C1, then V1 and S1. OAuth PKCE/callback validation and Windows Credential Manager are local primitives only; no network login or loopback listener.
- W4: Qt mock states rendered at 100/150/200 percent and tested. Mock is not wired to the operator plugin. CI definition exists, but no remote or hosted run.
- W5/P1: not reached. No signing certificate, tested installer/uninstaller, production candidate, end-to-end or deployment evidence.

## Decisions

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
- Journal tests cover account separation, immutable object identity/hash, quotas and corrupt input; power-cut/process-kill matrix has not run. Corrupt session currently fails account recovery closed.
- Auth tests cover RFC7636 S256 vector, fresh state, exact loopback origin/path/port, replay/duplicate-state rejection, and a unique synthetic Credential Manager save/read/delete. No real credential was accessed.
- Qt mock screenshots are in artifacts/ui. These demonstrate layout/state rendering, not production action wiring.
- Read-only reviewer covered 7d2b1e9..ec866ea; no critical issue, three important issues fixed: failed fixture completion, source color validation, preserved failure reason. Standalone fixture cadence corrected to 30000/1001. Auth, dock and later master changes were outside that review range.

## F1 and F1-L

F1 producer 3701e79639ef119eb93b90d0c5e76219174f95bb used a separate portable actual OBS frontend, moving synthetic Program with visible timecode and silent stereo. Three renditions: 30.030000 + 30.030000 + 0.934267 = 60.994267 seconds. Full decoding, ffprobe format and object hashes passed locally. This is not Mac V1.

Canonical package artifacts/F1-obs-02.zip: 40678410 bytes; SHA256 1fbb8d260fdf44e2315fb62087c6009393c1c9a02f127387f615d6c0b8323188. Packaging renamed references and added master; media bytes unchanged. The archived machine-progress attachment predates the user's FPS correction; HANDOFF.md and actual media carry the correct 29.97. Keep this archive immutable; issue a new revision for corrected metadata.

F1-L producer is the same 3701e79, separate portable OBS PID 3128, capture started approximately 20:12:32 Taipei. Requested 9000 seconds, expected finish approximately 22:42:32. Preserve loaded DLL while running. Hidden developer helper PID 14064 samples every 30 seconds, then validates and packages if capture completes. Status/logs under artifacts/F1-L-*. Resource figures include short overlapping harness tests (approximately 20:26–20:46); do not present these as isolated production performance qualification. No simultaneous original recording or YouTube test was performed.

## Open dependencies and limitations

- Authorized Mac-accessible shared location and download/hash receipt: not provided. Local archives do not count as cross-host delivery.
- Plugin remote and PR target: not provided; no PR or hosted CI can be claimed.
- Frozen versioned C1, V1 for exact fixture, S1 test environment/accounts: missing. No real integration until received.
- Production UI/controller, network retry/resume/cleanup orchestration, remaining fault matrix, concurrent-output runs, candidate install/remove and signing remain unfinished.
- Staging and immutable queue currently duplicate media on disk. Runtime reserve/package checks exist; disk exhaustion and watchdog fault injection still need testing.
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
