# Native paused-worker diagnostics — 2026-10-22

## Observed case and limits

- Installed main: `137aec1366306e25e15ced6efba8cb17f22d41ee`, OBS 32.2.2.
- Recording `05cdab25-5e12-46f2-84b5-5d21fd7dffcb`, capture `b80f5410f62c4860823fb61177f7135d`, output `2628118a-2198-46ec-b4e9-59e6dce53112`, epoch 1.
- Source began 20:30:31.971 Asia/Taipei UTC+8. Last persisted confirm was 20:56:24; last recorded successful control poll was 20:56:27.848. Confirmation then remained at sequence 153, verification at 152, while encoding continued.
- At 21:00:20.709 the wall-clock verification lag estimate reached 257.209 seconds, beyond the existing 240-second guard. One normal HHC Stop was clicked at 21:00:32.814. The final journal has `normalEnd=true`, `stopIntent=true`.
- All 547 media objects, 1,207,488,177 bytes, were copied and individually SHA-256 verified. Six pre-recovery queue documents were also preserved. No original media, journal, key or settings was changed by this diagnostic work.
- The UI remained paused/needs-attention. Stop/seal receipts, input release and ready were not yet obtained. No Retry, replacement capture, installation or restart was attempted.

`KeyController::tick()` returns while `paused_` is true; normal Stop does not clear that fence. This explains why durable Stop intent alone cannot resume synchronization. It does **not** establish the original exception. In the installed revision both unclassified worker exceptions and the K5 `outputReplayUnknown` request-error branches can pause; some branches returned without recording the reason. Existing HTTP logs and syntactically valid JSON cannot distinguish those causes. The separate platform pool defect is not proof of this native pause's cause.

## Candidate change

The existing control and sync result handlers now record their failure stage, classified code, UTC observation time, output ID, HTTP status when available, safe request correlation, pause/retry state, stopping/released flags and K5 authority flags. The control handler's early authority-unknown and proven-terminal returns are covered too.

- Two bounded latest snapshots: `k3/diagnostics/native/control-error.json` and `sync-error.json`.
- A fixed-format line also uses the existing scoped HHC diagnostic sink and its bounded rotating log.
- Only exact internal exception literals select specific local error codes. Other exception text remains unrecorded. Unknown standard/nonstandard exceptions remain explicitly classified and paused.
- Remote codes use the existing response-code allowlist; locally defined fixed codes have an explicit diagnostic allowlist. Unsafe correlation becomes `none`.
- Request exceptions retain validated response correlation through the existing API boundary. No body, token, credential, signed URL, arbitrary message, header or user title is added to diagnostics.
- Optional diagnostic writes cannot escape into GUI callbacks or change authority, retry/backoff, output identity, capture lifecycle or existing pause behavior.

There is no root-cause repair, generic retry/unpause, 401/403 terminal shortcut, protocol, quality or resource change in this PR. Unknown message content is intentionally unavailable; a future unmatched cause can still require a focused reproduction.

## Validation

- RED commit `3e11c90`: actual Qt worker completion through the existing control/sync watchers reproduced missing durable diagnostics (`k3-dock` failed).
- Focused `k3-dock` and `c1-http`: 2/2 passed after the implementation.
- Full local Windows build and CTest: 27/27 passed, 38.04 seconds.
- Cases cover known/unknown standard exceptions, a nonstandard exception, request correlation, malicious message/code/correlation exclusion, 503 retry state, 401 identity fence, both K5 authority-unknown callback paths, and an unwritable diagnostic destination.
- These are local/mock/controller-boundary results. They do not diagnose the historical live exception, prove same-capture recovery or count as a new OBS/live/100-viewer pass.

## Acceptance gate

PR/CI review first. No merge, deployment, DLL replacement or OBS restart is included. An eventual same-identity Retry requires fresh server authority, preserved original binding, no ambiguous pending mutation and an unreleased input; it must not be used as a blind workaround. Existing originals and the failed 100-viewer/zero-hold evidence remain protected.
