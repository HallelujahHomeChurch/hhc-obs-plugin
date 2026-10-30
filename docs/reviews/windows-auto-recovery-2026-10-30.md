# Windows automatic recovery without a Retry button

Base: installed production source `b95edc0dcbbe853f105cf4729f14d14ba6ae1ca4`.
Scope: isolated Windows x64 worktree; no installation into the operator's OBS, OBS operations or live broadcasts during this change.

## Problem and resulting behavior

An unclassified worker exception set a shared permanent pause. This prevented both upload and control polling; the Retry button only cleared that flag. Its diagnostic fallback `local_validation_failed` claimed a validation failure without identifying the actual exception.

- Remove the native Retry button and all its handlers.
- Retain existing HTTP backoff, Retry-After, original output identity and platform/queue limits. There is no new three-minute deadline or retry-attempt termination limit.
- Schedule automatic backoff for unclassified exceptions and transient Windows sharing/locking failures. Keep control polling independent of upload pause, including finalization; retain authority/ownership fences.
- Type required metadata I/O and internal validation failures. Diagnostics report Win32 error codes or source check locations without arbitrary exception text, paths, credentials or signed URLs.
- Stop HHC capture for permanent faults and retain durable stopping/abort markers and media. The independent OBS recorder is untouched by this fault path.
- Replay an abort only against the authenticated original binding. Damaged media/control journals do not have to be validated before abort, but required remote identity/receipts are never bypassed or repaired by guessing.
- Retry release-pointer persistence without permitting a replacement output. Recovered and released events clear the active failure snapshot; a proven terminal response is not logged as a fresh failure.

## Validation

The original implementation failed `key-dock-test --automatic-recovery` with all three expected symptoms: permanent pause, misleading local-validation diagnostic and remaining Retry button. The corrected controller passes.

The 35-test CTest suite passed in an intermediate full run, including the existing real 90-second localhost outage test. After final diagnostic/release-pointer changes, all 34 affected/fast CTests passed; the HTTP outage implementation itself was unchanged. Release-tag CI reruns the complete suite.

Additional regression coverage:

- Deterministic controller clock advances through more than fifteen minutes of transport/429/503 retry schedules and then succeeds, without stopping the output, changing identity or invoking the native recorder.
- Upload pauses and local Stop do not prevent control-worker dispatch; all transports/credentials are isolated fixtures.
- Real Windows exclusive file handles exercise required-metadata read/write sharing failures, followed by successful replacement after unlocking.
- Disk-full, immutable-media mismatch and invalid-key failures retain original identity and stopping/abort intent without claiming server release.
- Actual transfer/adoption tests abort with damaged local media/control journals, preserve their bytes, and replay a lost abort receipt with the original operation key, without reopening or duplicating the operation.
- Existing cancellation, stale-generation, authority, stop/seal, cleanup, HTTP, TLS and UI compatibility tests remain in the suite. Diagnostic fixtures assert that synthetic secrets and URLs do not leak.

Evidence is in the isolated worktree's ignored `artifacts/automatic-recovery-20261030/` directory. Packaged artifacts include only the operator DLL, helper, pinned TLS backend, documentation and licenses.

## Limits

This is unit/mock/local HTTP and package evidence, not a real OBS/NVENC run, two-hour media verification, production server outage or member-playback acceptance. No streaming, upload, resume or production installation was performed by this coding task.

The initial 2026-10-30 exception remains unknown because the old log discarded it. A revoked credential or damaged authoritative remote journal can prevent safe remote termination; preserve pending state and report required intervention rather than claiming the platform ended. Do not replace the original output or delete its evidence to bypass that protection.
