# Capture pagination throughput correction — 2026-10-22

## Trigger and change

The normal 25d1e6d source traversed every capture status page during both
active adoption and the subsequent upload step. At 2,001 objects this means
six GETs for each three-object upload round. The retained real OBS trace
showed approximately 11–12 second confirmation cycles while S10 produces
a group every 10.01 seconds; pending groups consequently grew.

Established active outputs now adopt fresh authority using the existing
capture GET with `limit=1`. The upload step still traverses all pages before
mutation. A capture with more than 1,000 remote objects transfers at most
six closed objects per round; smaller captures retain the three-object cap.
No wire, encoder, timeout, control lock, or across-round object cache changes.

The mandatory full status refresh after replaying uncertain mutations stays
in place. Summary reads cannot perform ready-file cleanup. Identity checks,
D1 cutoff persistence, terminal handling, operation keys, and complete seal
inventory remain in the existing paths.

## Deterministic verification

Test-only RED commit: `0b9d38a` (base `25d1e6d`). Actual `keyTransferStep`
with 2,001 / 4,500 objects observed 6 / 10 GETs and failed the expected
one-summary-plus-one-traversal assertion. Fixture setup failures preceding
that committed regression are not regression evidence.

GREEN: the same scales observe 4 / 6 GETs. Additional mock checks cover:

- Identity mismatch and generic 401 reject without release or mutation.
- A failed object on the last status page is detected before upload.
- Ten pending objects drain with a six-object maximum batch.
- An uncommitted confirm response is replayed unchanged after restart;
  a fresh full snapshot prevents signing already confirmed objects again.
- An authoritative aborted capture prevents later mutation.
- Existing D1 zero cutoff, restart, stale snapshot, and changed cutoff checks.

Focused CTest: 5/5 passed, 18.22 seconds, exit 0:
`k4-ingest`, `capture-http-finalization`, `capture-batch-partial-put`,
`capture-batch-lost-confirm`, `capture-batch-uncommitted-confirm`.
Local build completed; `git diff --check` passed. Required Windows CI and
its packaged artifacts are reported separately in the PR.

## Real OBS evidence and remaining work

The original 25d1e6d field case was stopped once at
2026-10-22 12:24:49.579 Asia/Taipei (UTC+8), following its safety guard.
Stop accepted at 12:24:52.422536501; seal accepted at 12:30:56.150684.
Recording: `d261d9d3-8229-4dc6-a8aa-e812a87ad5cc`.
Capture: `d39b7fb35f0dcc0c5bbdc57641b74bf7`.
Output: `e4b8d3c4-74ea-4ae3-b759-f9c721a6b571`.

All 2,530 retained media objects were copied and SHA-256 verified.
Media bytes 5,653,132,496 plus canonical inventory bytes 322,775 equal
the platform total 5,653,455,271; this difference is not missing media.
Inventory SHA-256:
`7d3485c8dc8ebc70da9de28dada1b4d37ba09f82fdf2de9b793b5bfe3b0839bf`.

This source correction has not been installed or measured in real OBS.
The earlier safety stop means the near-9.5 GB field target was not reached.
No new capture, load cohort, manual publication, production install, or
restart is part of this change. Existing playback/stress failures remain.
