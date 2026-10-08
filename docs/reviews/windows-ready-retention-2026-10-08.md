# Ready evidence and successful temporary-data inspection

Base: e02126ac6bf3351fa8ff4659f2b50f44aa8390df. Scope: Windows shared sync/store and native dock; no platform edits, deletion, automatic purge, merge or deployment.

The seven-day retention predicate had no production caller, and server ready never populated the local journal's remote confirmation fields. The shared sync path now records the validated seal/ready evidence, capture/package binding and first observed server timestamp. Both the native controller and background helper use this path. A ready package differing from the previously sealed package is rejected before updating remote status. Freezing, queued verification and a seal receipt alone cannot qualify media for inspection.

The metadata checkpoint preserves object identities and does not rehash the entire long event on each poll. Recovery still verifies every retained object's SHA-256 before offering the GUI inspection entry. Repeated ready observations retain the first timestamp. Missing/changed package evidence refuses an update and retains media. The inspection button opens only the qualified selected session owned by the current account; it is unavailable during capture/upload/validation, local scanning, disconnected/local-only operation, or for an unqualified selection. The controller also checks ownership, eligibility and directory links. It never deletes files.

| Evidence | Result | Scope |
| --- | --- | --- |
| `artifacts/ready-retention-red.log` | exit1, four expected failed assertions | Missing persisted readiness/retention behavior reproduced against the previous code |
| `artifacts/cleanup-dock-red.log` | exit1, missing safe inspection entry | Qt mock, not physical keyboard input |
| `artifacts/ready-cleanup-final-build.log` | exit0 | Native Windows build; existing SDK/developer warnings remain |
| `artifacts/ready-cleanup-final-ctest.log` | all10 pass, 3.11s | Unit/mock/loopback including ready, package mismatch/missing, retention boundary, active/account gating and existing recovery checks |
| `artifacts/c1-ready-retention-recheck.json` | all11 immutable artifacts match, manifest SHA-256 f702477236ba184853c119ffa9c4da0cc9f6d44cf7911785f85f3aa6d94e8be9 | Fresh authenticated GitHub byte checks; C1c1-2026-10-08.2; fake fixture media is never uploaded |
| `artifacts/original-capture-current-readback.json` | 2026-10-08T14:10:31Z, aborted/recording_deleted | Actual production GET; stop accepted, unexpired,31 declared/27 verified/4 queued, no package and no seal retry |

Original capture5d68171bc2e63cd5f9cae36eef7032b7 remains terminal. The original inventory SHA-256 remains d4d82a30fc701631c823729fc539a6d4fc81e4144c6d4f0670828fdf5911c0db: eight fragments per rendition,240.206633s,30000/1001fps, inventory digest f50005b13e8f25e585424f8d93c8428a7da50f39dcd7ff57058e120c75b3dcef and operationKey439db0a4-560d-4156-ac00-aeafad6742a7.seal are preserved. Current seal/ready/publication retry results: not attempted because the original recording was deleted, not a successful acceptance.

Ruling: use the existing seven-day predicate and Explorer for explicit inspection, without an automatic deletion service — the design calls for listed successful-data options and treats automatic seven-day removal as a proposal — cost: any actual deletion remains an operator decision after checking the original video is still available remotely. No existing local data was removed or aged artificially.

Fresh current-candidate capture/upload/seal/ready/publication and long production E2E remain pending exact draft cleanup authorization/quota. Actual ordinary crash-prompt/physical keyboard tests still await window-display authorization. The member-player quality/backward-seek failure remains open. Existing e02126a push/PR CI both completed success; subsequent source changes require their own candidate and CI evidence.
