# Windows night recovery preparation — 2026-10-23

Base: `266313caca2e36b2601ae6d708a763c6ef9d2eab`, OBS32.2.2. Production OBS is in use; no OBS interaction, elevation, installation, live fault injection, or capture creation before22:00 Asia/Taipei.

## Independent change

Record controller stop requests separately as user stop/cancel, Console end, fixed server end, revoke, proven output release, remote terminal capture, or OBS shutdown. The bounded native diagnostic sink and a single last-event JSON retain only allowlisted causes, a hashed output identity, UTC observation time, and nullable public cutoff. Zero is a real cutoff. These are request-origin observations, not proof that the native output has already stopped. Existing native output statistics remain the completion evidence.

No change to stop reasons, capture behavior, recording options, credentials, authority fences, D1 wire, public range, renewal, media, journal format, or upload/seal behavior. A fixed server cutoff remains binding. This change is not a recovery-policy fix.

The developer HTTP test accepts a bounded90–120second outage duration. It uses only a process-local fault marker and a loopback server; it verifies that requests cannot escape the outage and that real HTTP resumes afterward. Production builds still ignore the marker. It does not establish D2/controller recovery, ongoing encoding, or platform auto-idle behavior.

## Local evidence before22:00

- Windows x64 MSVC19.44/Qt/OBS SDK native build succeeded in the isolated worktree, with production network faults disabled.
- Baseline non-OBS-operation CTest:20/20 passed,37.64seconds.
- New and boundary tests:4/4 passed,93.17seconds; bounded HTTP outage test90.19seconds, plus stop-cause privacy/zero/null/persistence-failure checks.
- Full UI mock suite, new hosted CI, isolated real OBS and actual D2/private-chain validation are not claimed yet.

## Remaining dependencies and coverage

The fixed D2 target and its20:58 eligibility revision were delivered by the platform owner. Server deployment and safe-chain acceptance remain separate gates. Do not infer a five-minute recovery authority from a request preference, missing server field, confirmation receipt time, or legacy capability.

An existing unannounced manual staff-preview source can validate normal encoding/upload/preview without audience publication. It cannot prove public auto-idle or public End behavior. D2 failure recovery must be tested separately against a safe isolated protocol harness after the fixed contract arrives. No direct auto-announced live source may be called a private test endpoint.

## D2 target parsing checkpoint20:56

The platform owner's20:45 wire target was delivered in full through the trusted coordination message. It is not a deployed-server acceptance result. A separate D2 schema and16 synthetic wire fixtures now cover required/null fields, 32-lowercase-hex capture identity, UTC timestamps, safe integers, unsupported capability and unknown states/reasons. Frozen D1/K4/S10 schemas are unchanged; D2 open-input validation extends the complete S10 input rather than the older K4-only definition.

The observation guard checks exact recording/capture/epoch, drops an older request generation or revision, processes recovery changes at equal D1 revision, preserves committed public end while accepting capability loss, and rejects cutoff expansion. Only422/broadcast_protocol_unsupported allows control fallback; generic400,404,500 and unknown-authority errors do not.

## Integrated runtime checkpoint21:17

New explicit Start durably saves `common-watermark-v1` as a client preference before streams/open. The producer alone determines effective capability; manual/scheduled must still succeed with unsupported recovery. A generic400 preserves the original request and pauses visibly; it never silently removes the preference. Old K3/K4 journals retain their original request and D1 behavior.

Both native control polling and upload coordination negotiate D2 for a saved preference. The existing broadcast journal lock serializes HTTP control requests. A per-controller generation fences obsolete responses after Start, Stop, shutdown or authority loss. Required recovery/publicEnd metadata is validated and persisted separately from the unchanged D1 journal fields. Public End, including manual End, never requests native local capture or recording stop in D2. Capability rollback preserves an already committed public End while displaying unsupported capability.

Final401/403/404, identity/epoch uncertainty, revoke or terminal authority stop protected network operations and preserve local media. A per-output durable deny marker fences restart/helper/PUT requests. Bounded transient retries remain existing policy. Only explicit local Stop, shutdown or native safety can end retained local capture; native OBS recording remains a separate lifecycle. A failed worker future cannot skip joining the other worker during shutdown.

Runtime regression fixtures cover request persistence, unchanged same-ID/body retry after generic400 and a lost response, authority deny across restart with zero further HTTP, no generic control downgrade, stale generation rejection, durable public End without local stop, capability rollback, and independent worker joins. Local suite27/27 passed in36.76seconds; this excludes native-obs-ui, k3-dock, controller-lifecycle and the already separately tested90second outage. A final semantic identity check and empty-preference rejection were subsequently added and their affected ingest/D2 tests rerun. New hosted CI must pass before selecting an installation package. Earlier CI39953482983 and39957098236 passed for their earlier commits only and are not evidence for this runtime revision.

Not claimed: deployed D2 service, server transaction fencing, actual SourceManaged automatic five-minute recovery, real ongoing encoder recovery, or the normal30-minute private staff-preview chain. A manual unannounced preview validates normal media only, not automatic public End/recovery. No announcement or paid resource is authorized for this test.

After22:00, recheck OBS streaming/recording and helper completion before any installation. Preserve settings, key, journal/outbox/media, and backup the installed DLL/helper/TLS set. Prefer one verified normal candidate and one user-operated UAC; never install a fault build or force-stop active output/helper. Retain the verified266313c CI package for rollback; this diagnostic-only change adds no journal migration.
