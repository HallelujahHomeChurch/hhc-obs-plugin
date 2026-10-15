# Native local preview review — 2026-10-08

Independent read-only review: 7d2b1e9..c1c50c5. No confirmed Critical finding; four Important findings entered one fix pass. No second review was used in place of regression evidence.

## Fixed Important findings

1. OBS may reject a Close event after HHC's confirmation. Defer permanent controller shutdown until frontend EXIT; do not stop capture or disable the controller while OBS can still veto. Real libobs + Qt regression accepts the HHC prompt, invokes a host window that rejects Close, then verifies capture continues and the stop action still completes. RED exit 7 -> GREEN exit 0.
2. QFutureWatcher::waitForFinished can rethrow an already displayed recovery exception. Teardown now catches it and retains a fixed local error. Actual Windows symbolic-link queue root reproduces scanPending failure. RED exit 31 -> GREEN exit 0; shutdown/destruction no longer propagate that recovery exception.
3. Track advancing DTS and monotonic progress time independently for all three video encoders plus audio. A post-start stream stall fails incomplete after 10 seconds. Separate fault-only libobs harness drops packets after one second. RED exit 21 -> GREEN exit 0. Missing-header, stop-timeout and low-disk injection also pass again.
4. CaptureOutput::wait now requires finalized normalEnd, not merely a stopped worker without a prior exception. A real external obs_output_force_stop remains incomplete and returns failure. RED exit 21 -> GREEN exit 0; normal dock capture still completes.

All four CTest suites passed after the fixes. Reviewed native OBS dock capture: 61.027633 seconds, three matching renditions, three segments each; complete decoding and object hashes passed; OBS exit allocation count 0. These checks do not prove E2E, physical power-loss safety or current-build long-duration qualification.

## Minor deferred

The packaging script checks clean Git state and hashes files but does not automatically bind an existing build DLL to a source stamp. This run explicitly built the current source before packaging; artifact contents are hash-verified. A future user who packages a stale ignored DLL could mislabel its source revision. Add automated provenance stamping in a later packaging change.

## Rulings on review limits

- Missing HTTP/OAuth wire mapping and old F1/V1/file-share gates are not defects in this local-only deliverable: user explicitly waits for Mac's fixed contract/endpoints/login and removed advance file delivery. Cost: platform use remains unavailable until integration.
- Mac compatibility, server ready/publication and E2E are unverified. Cost: no production readiness claim from local tests.
- Older producer 3701e79 long media cannot qualify the current candidate. Cost: current-build long/concurrent output validation remains required.
- Hosted CI is independently checked via GitHub Actions, but physical disk-full/power loss and GPU device hangs are not reproduced by injected faults. Cost: those hardware-level behaviors remain unqualified.
- Review findings were initially source-path analysis; the fix pass added actual regression runs for all four. Cost: tests cover those triggers, not every possible OBS shutdown sequence.
