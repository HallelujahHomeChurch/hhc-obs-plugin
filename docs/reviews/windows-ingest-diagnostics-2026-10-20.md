# Windows ingest diagnostics — 2026-10-20

Independent of blocked terminal-recovery PR #14. Diagnostics only: no wire/schema/profile,
upload concurrency, public boundary, recording settings or retry-policy change.

## Why

The existing local recording identified an audio-buffer discontinuity and later a
confirm-receipt gap, but did not retain enough request timing to identify the gap's
cause. The deletion-related invalid-session 401 occurred later and remains separate.
New instrumentation cannot retrospectively prove why the earlier signal was lost.

## Recorded fields

- HTTP: UTC, monotonic start reference, fixed operation name instead of URL/path,
  attempt number, elapsed milliseconds, status, existing allowlisted error code,
  canonical UUID/hex request or correlation ID, response/exception/cancel outcome.
- Raw media PUT and transport failure: same safe operation metadata; distinguish
  absolute/inactivity timeout, cancellation and network error when observable.
- Renewal: hashed output identity, UTC/monotonic time, old credential TTL seconds,
  force flag, elapsed time, renewed/rejected result and numeric status.
- Transfer ownership contention: hashed output, time and fixed local-session-busy code.
- Queue scan: hashed output, pending object count/bytes, contiguous common confirmed
  sequence across all three renditions, and separately the verified sequence. These
  are server-poll/queue-scan observations, not member playback latency or cutoff.
- Capture state transitions: bounded enum values, hashed output and verified progress.

No token, Authorization header, stream key, full URL, query, request/response body,
signed media capability, raw output/recording/capture ID, title or personal data is
included in these added messages. Unknown error codes become `request_rejected`;
unrecognized correlation values become `none`.

## Persistence and bounds

Use Qt's existing logger, which OBS captures in its normal log. Detached helper
messages with the trusted `[HHC ` prefix are also saved under its settings root in
`diagnostics/helper.log` and `helper.previous.log`. Only single lines up to 2 KiB
are eligible. Cross-process rotation is locked; two files have a combined maximum
of 16 MiB, including across helper launches. No media/journal/queue is rotated.
Contention or logging I/O failure skips diagnostic persistence and does not fail
the transfer. This is bounded best-effort evidence, not a lossless event journal.

## Verification and limits

- HTTP log assertions failed before adding diagnostics, then passed: per-attempt
  timing/correlation and redaction of synthetic bearer, key, body, origin and query;
  malicious unknown code/correlation and cancellation classification are covered.
- Common-confirmed progress assertion failed before instrumentation, then passed.
  Existing partial PUT and lost-confirm recovery cases still pass.
- Helper retention assertions failed with an empty sink, then passed after bounded
  two-file rotation; tests include repeated rotation, newline rejection and writers
  yielding under the same cross-process lock.
- Focused local CTests cover C1 HTTP, K3/K4 ingest, capture HTTP recovery, finalization,
  partial PUT and lost confirm. Full build/CTest/package checks belong to PR CI.
- Local helper target compiles; no helper was launched against a real session.

No installed OBS change or new live test is authorized by this PR. Production
recordings, keys and original journals remain untouched. OBS stays 32.2.2. Request
timings/logging overhead and real signal recovery still require separately authorized
field acceptance after recording ends. Historical latency and weak-network failures
remain unchanged.
