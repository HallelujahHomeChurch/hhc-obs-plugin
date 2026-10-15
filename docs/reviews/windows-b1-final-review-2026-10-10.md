# B1 final source review and fix evidence

Fresh independent reviewer inspected `1b736b5e599241ccc04ecc4c0dab782eb7899f56..f86a0bfe34b29dd957e415eaaf3a39ab9d559f23`, the complete B1 plan and immutable public contract. No critical or minor findings; two Important findings. No production/OBS/GPU actions.

## Original findings

1. **B1 control availability gates established C1 upload/stop/abort.** Each established session calls `control.bind()` and `sync.adopt()` before `sync.step()`. A B1-only 503/403/409 prevents C1 work while encoding continues. Repeated lock collisions are retryable, but the dependency remains. Bind/adopt once at creation or missing-journal recovery; uncertain ACK recovery must not gate basic upload/stop/abort work.
2. **Missing a cancellation observation blocks subsequent commands.** Persist ACK A, lose request/response, cancel A and issue B before the next poll. A is replayed first, rejected with 409/412, GET information is discarded and control pauses before B. Retry repeats it; inactive replay may prevent finalization. Durably classify superseded/rejected saved commands without changing their keys or markers.

Reviewer also confirmed pinned hashes/fixture bytes, encoder PTS and next 900-frame markers, 202/null gating, and absence of introduced CPU fallback, global OBS/YouTube stop, credential persistence or signed-URL journal fields. The original 14/14 tests did not cover these two sequences.

## One fix pass

- Added shared `CaptureSync::broadcastStep`, used by the actual controller and loopback test. Established captures use C1 directly; only initial/missing-journal binding queries B1. Before-seal reconciliation runs after normal stop and all uploads. Background helper uses the same seal fence; abnormal abort remains independent.
- Rejected stale ACK first GETs the same binding. Only `broadcast_state_conflict` with that command no longer pending retires its saved intent. Keys, markers and bodies remain unchanged. Operation/boundary/epoch conflicts stay fenced. Explicit cancellation remains durably fenced. New commands then proceed.
- **Observed RED:** replacement-between-polls; inactive superseded replay; established C1 during B1-only outage; B1-only outage stop/abort; seal fence after successful upload/stop.
- **Observed GREEN:** both HTTP regression executables; fresh complete build exit 0; all **14/14 CTest** in 4.54 seconds; media-grid test and Windows watcher exit-code test exit 0. An additional Qt test include was corrected before the final full build.

No deferred findings, design rulings or second review. These are local/mock source checks; actual timing, public range, deployed platform and real encoder acceptance remain pending.
