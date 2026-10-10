# Stop and logout lifecycle review

Fresh independent read-only review covered `347d82701770f67143654cfb22b8483d1c19a1d5..ca9e560bc58097346033984dbb06dae91ad06f7d`. It found no Critical issues and two Important lifecycle defects. This is a source review, not production end-to-end acceptance.

## Findings and fixes

1. **Important: a stale stop confirmation can start another capture.** At reviewed `src/dock.cpp:186`, the dock's displayed Capturing phase can outlive `CaptureOutput::finished()` until the controllers' next 200 ms poll. Reviewed `src/local-controller.cpp:117` and `src/platform-controller.cpp:219` then interpret that callback as a start. The platform path can create another event with the previous exposure choices. Both controllers now reconcile any finished output and return from this action. Start paths require explicitly eligible phases; active platform stop additionally requires Capturing. Local scanning remains busy until its queued result is consumed.

2. **Important: an unconsumed platform worker result races an account change.** Reviewed `src/platform-controller.cpp:38,56` guard only `job_.isRunning()`, which becomes false before `completed()` processes the queued result. A logout in Failed/retry can clear account/session state before an old synchronization result overwrites it, or a login can overlap an old create completion. Platform jobs now retain their busy flag from launch through the entire result handler, including scan/error/early-return paths. Login/logout, refresh, cleanup, recover, submission and polling use that shared guard. Submission and intent control also refuse during authentication.

The reviewer assessed the reviewed diff as **With fixes**. The findings were reproduced and fixed here; this document does not claim another independent review of the fixes.

## Verification

The new `controller-lifecycle` test runs real local/platform controllers and Qt worker/result delivery in temporary storage. It initializes only libobs core, without graphics, modules or an encoder. An already-finished unsuccessful `CaptureOutput` represents the state before the next GUI poll. Synthetic account/device IDs are isolated; the intentionally fenced accidental start on unfixed source uses a synthetic in-memory bearer and transport cancellation before HTTP. No platform request, browser login or real-account logout is performed.

- Complete RED: `artifacts/controller-lifecycle-red-ctest-3.log`, exit 8, reports exactly the stale local stop, stale platform stop and pending-platform-result logout failures.
- Earlier test setup failures are preserved separately: missing DLL search path (`red-ctest.log`), uncaught cancelled-future test cleanup (`red-ctest-2.log`), and unsupported QFuture equality in the expanded check (`final-build.log`). These are not used as product regression evidence.
- GREEN: native plugin/helper and full source build exit 0 (`artifacts/controller-lifecycle-final-build-2.log`); all 11 CTest cases pass in 2.54 s (`artifacts/controller-lifecycle-final-ctest-2.log`). The expanded check withholds GUI processing after a synthetic synchronization result, invokes refresh/submit/poll/recover/logout, and verifies the original result, account and session survive until consumption. Logout then leaves a stable signed-out state.

## Review exclusions and remaining gates

The reviewer set aside automatic resumption after same-account reauthentication, remote token revocation/helper-grant semantics, actual-account and positive platform/media acceptance, and physical/device/long-run verification. Existing manual recovery behavior and local credential-removal semantics remain; no remote revocation contract is invented. These exclusions are not production acceptance evidence. Real-account logout/relogin and required production capture/live/DVR/recovery/player/long-run gates remain open.

The live OBS 34836 / watcher 33832 retain frozen producer `ca298214c1cbb8f35fc67d300d5c2b80bf04a483` and the original 9000-second local recording/HHC test. No loaded DLL, profile, fixture input, duration or process was replaced. CPU-only builds/tests overlap its resource samples and must remain qualified accordingly. No platform repository change, deletion, merge, deployment or YouTube operation occurred.
