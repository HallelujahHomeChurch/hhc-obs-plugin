# Stop confirmation acceptance

The common accepted plan Task 6 requires stop confirmation. Audit after the earlier whole-branch review found that the actual dock still sent its stop callback immediately; the prior review did not establish this requirement as complete. This change completes that remaining plan item; it is outside the earlier review range and is not represented as a new reviewer approval.

The native dock now asks before invoking its stop callback, defaults to Cancel, and explains backfill/validation and the current automatic-publication intent. The local developer dock explains that its data stays local. Both controllers use the same dock handler; no wire mapping or publication state is added. If encoding ends while the dialog is open, accepting its stale question cannot invoke another event action. Callback/controller guards and journals remain authoritative.

Verification on the fix working tree (parent source 347d827, not that unchanged commit):

- RED: mock-dock CTest exit 8. Cancel invoked the stop callback, the intended confirmed callback count failed, and the later replacement action count exposed the extra call. Full output: `artifacts/stop-confirmation-red.log`.
- GREEN: cancel leaves the callback untouched, Yes invokes it once, and an asynchronously terminal phase cannot reuse the old confirmation. All 10 local CTest cases pass in 2.48 seconds; `artifacts/stop-confirmation-final-ctest.log`.
- Actual isolated OBS process 10176: the developer fixture cancelled the real modal question, observed encoding still active and journal stopIntent still false, then confirmed the real stop. Both booleans in `artifacts/stop-confirmation-obs-green/stop-cancel.json` are true. OBS exits 0; normalEnd/stopIntent are true, no ready/seal receipt is fabricated. This is automated native UI/capture behavior, not physical operator keyboard/DPI acceptance.
- Three renditions total 61.027633 seconds each: 30.03 + 30.03 + 0.967633. Inventory hashes, formats, aligned timelines and complete decodes pass. Nine canonical media fragments, no retained staging media copies. `artifacts/stop-confirmation-obs-green/{storage-result.json,media-verification.log}`.

Developer regression: `tests/capture-storage-test.ps1 -RunName UNIQUE_NAME -CancelStopOnce`. The separate fixture supplies modal answers only in the developer module; it is excluded from the operator package. Existing ordinary-crash and local-controller developer checks retain their separate meanings.

Frozen source-stamped candidate and CI results are recorded separately after committing this change. Current-version local 2.5-hour original-recording/HHC validation is independent of the outstanding test12 cleanup permission. Fresh positive production capture, seal/ready/publication, live/DVR/recovery and production long-run acceptance still require the quota and platform gates in [continuation status](../windows-integration-next-gates.md). No platform edits, merge, release or YouTube operation occurred.
