# Windows K5 replay-only consumer — draft, not installed

## Fixed producer acknowledgement

- Contract: `k5-2026-10-20.rc1`, producer `33569a3bd292a600a143608ce1a62b3b579d7c53`, reported CMS `hhc-web-api--0000242`.
- The immutable files are retained byte-for-byte under `tests/fixtures/k5/` and the exact JSON Schema is embedded in `src/k5-schema.inc` without changing its references/constraints.
- Manifest SHA-256: `17d02cb529d45b6f5b2820b2e2d89fce553017978fa5e08a4be029e499aa249d`.
- Schema SHA-256: `38ab8c99e1204c29153b35fbbb2d05264c4db80d22f60d97b3cbb2efae4a0b67`.
- Wire fixture SHA-256: `77a9647492406bcc538e7a2b2815ca2b652d2000280d3d03b8744ea0c50ec27f`.
- Contract SHA-256: `5554aa915bef3481e0648b4d9a948e1693c49b7b99c7215e089171a0268d386b`.
- All 23 frozen K4/S10 files listed by this manifest were independently read at the fixed producer commit and matched. K3/B1/C1/D1/S10 schemas and media behavior are unchanged.
- The contract file's source-stage deployment label is historical. The coordinator supplied CMS deployment evidence; public Gateway acceptance is a separate pending gate, not inferred from CMS deployment.

## Consumer behavior

New K4 opening intents are durable before HTTP. Their original key is pinned to an output-specific Windows Credential Manager reference before the first request; only the reference is journaled. Initial/lost-response open uses that original key. Bound output recovery never calls open/resolve/bind.

After exact invalid-session renewal, or loss of a bound cached session, the consumer sends authenticated `POST /api/ingest/streams/replay`. It validates the fixed schema and exact output/source/recording/capture/epoch/profile/completed receipt. An active result can renew only the original session. Only exact HTTP 200 terminal saves a credential-free terminal proof and then signals input release. Generic 401/403/404/409/422/429/5xx, unsupported routes, cancellation, malformed identity and missing evidence do not release or create anything. There is no K4 fallback. Replay-unknown errors retain their provenance through control, transfer and helper paths: they pause without ordinary conflict reconciliation, revoke/abort handling or transient upload retries.

Terminal persistence is atomic and protected against stale renewals. Failure to persist proof restores the same instance's previous state. Controller release commits the active pointer before enabling Start and is monotonic against late control/upload completions. It does not call global streaming or recording controls. A new output ID is created only by a later explicit Start; ordinary unfinished Stop/seal remains on the original workflow. The helper retires only the original network output and leaves original journals/receipts/media intact. Legacy K3 keeps its existing path.

## Pre-upgrade incident: distinguish K5 credentials from normal release receipts

Read-only inspection of output `b679eadf-02b4-44e4-9022-0d6aa7fadef8` found its original K4 intent, source `2c58a390-1906-4d62-b962-777f7f40bbb5`, recording `1bb0b772-783c-4346-a2eb-f81e0ab3c47b`, capture `da11e1ed84c37bc5e27914606ea76ee8`, epoch 1, S10 profile and completed original bind receipt. It has **no per-output credential reference** or K5 terminal proof. No credential was read or copied for this inspection; a mutable global key cannot fill this K5 gap.

The latest original sync journal also contains matching normal-release evidence: Stop accepted `2026-10-20 06:42:20.49950198 UTC+8` and seal accepted `06:45:33.305642 UTC+8`, each with this exact capture and original output operation key. The older `06:44` snapshot with sealAcknowledged=false preceded that seal and is not the current receipt evidence. The current journal's last capture state is validating; this is **not** a ready/publication claim.

Startup and explicit Start now verify a released pointer rather than trusting `released=true` alone. A validated saved K5 terminal proof, or the existing original CaptureSync matching Stop receipt **and** seal receipt (or accepted abort receipt) evidence, is required before clearing the original K4 output. CaptureSync reuses its existing strict mutation/body/receipt/path/identity parser without HTTP. Thus this incident may use its independently validated legitimate normal-release path without guessing an original key or replaying HTTP. The read-only test probe loaded the actual original sync journal through the production CaptureSync parser and returned `matchingReleaseReceipt=1 httpCalls=0`; it did not access credentials or write the original journal. The actual active pointer is `released=true`; `started` and `stopping` are absent (not explicit false), as is `protocolVersion`. Therefore the startup released-pointer branch runs the proof check before clearing the old ID; the same check fences a later explicit Start. No controller was run against actual OBS data. Actual installation/OBS unlock is not tested or authorized in this change.

Pre-upgrade outputs missing both K5 credential/proof and legitimate normal-release receipts remain paused with the same ID. Separately accepted original-owner migration evidence/contract is still required for those cases. Missing rows, terminal labels, a bare released pointer, generic errors, local detach and a new capture do not substitute for release evidence.

## Verification and remaining gates

- Watched focused K5/schema/key-pinning/no-fallback tests fail before implementation, then pass.
- Watched terminal-proof lock/same-instance regression fail, then pass after rollback fix.
- Watched actual transfer revoked-error and control replay409 propagation regressions fail before fixing provenance guards.
- Local headless K3/K4 tests pass; controller mock compiles. Widget mocks run on hosted Windows CI, without touching the user's desktop or credential store.
- Coverage includes canonical fixture parity/hash, legacy900/S10, original key versus changed global key, missing old credential reference, released-pointer startup/explicit-Start fencing, validated normal receipt release, incomplete receipt, exact identity/profile, all generic errors, lost cache, stale renewal, restart, helper/transfer journal retention, release pointer write failure and late callback ordering.
- Hosted PR CI/package evidence is reported on PR #14 after completion. Existing PR15 diagnostics tests are not separately rerun locally.
- Keep PR #14 Draft. No merge, package installation, OBS launch/restart/controls/focus/configuration, live request, capture or original queue/media mutation was performed for this change.
- Public Gateway acceptance and live consumer acceptance remain separate. The initial signal/audio interruption remains unresolved and is not repaired by K5 recovery.
