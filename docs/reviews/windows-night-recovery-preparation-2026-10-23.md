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

Exact422/broadcast_protocol_unsupported persists a D1 fallback. As specified, that explicit fallback resumes legacy native boundary-stop semantics and provides no five-minute or public/local-separation guarantee; it still cannot clear a committed public cutoff. This differs from a valid D2 unsupported capability response, which remains accepted D2 metadata without requesting native stop. Other errors never permit this downgrade.

Final401/403/404, identity/epoch uncertainty, revoke or terminal authority stop protected network operations and preserve local media. A per-output durable deny marker fences restart/helper/PUT requests. Bounded transient retries remain existing policy. Only explicit local Stop, shutdown or native safety can end retained local capture; native OBS recording remains a separate lifecycle. A failed worker future cannot skip joining the other worker during shutdown.

Runtime regression fixtures cover request persistence, unchanged same-ID/body retry after generic400 and a lost response, authority deny across restart with zero further HTTP, no generic control downgrade, stale generation rejection, durable public End without local stop, capability rollback, and independent worker joins. Local suite27/27 passed in36.76seconds; affected ingest/D2 tests passed again, as did two hidden-window controller/safety regressions. Current isolated real HTTP outage recovery passed90.33seconds; it proves transport recovery, not continued native encoding or the D2 producer policy. New hosted CI must pass before selecting an installation package. Earlier CI39953482983 and39957098236 passed for their earlier commits only and are not evidence for this runtime revision.

At21:28 the final CMS PR312 OpenAPI was fetched at immutable `d4eaf63e0a998fdeca83eea4486d0ebd20ff397e`; exact file SHA-256 `b8eb743cc16a27029c3bb15cde16265456a165d428e31f5ff09c7bd5cdc3a553`. Its open schema explicitly permits an empty legacy preference, so the decoder retains that compatibility; new Start still emits only `common-watermark-v1`. This is a candidate-source verification, not a deployment claim. The platform owner confirms producer is not deployed before22:00.

Not claimed: deployed D2 service, server transaction fencing, actual SourceManaged automatic five-minute recovery, real ongoing encoder recovery, or the normal30-minute private staff-preview chain. A manual unannounced preview validates normal media only, not automatic public End/recovery. No announcement or paid resource is authorized for this test.

## Hosted CI and compatibility supplement21:42

CI39961520055 succeeded for head67813e7bc3f9d78bc56b4cea290673b620618aa4: full native build and32/32 CTest,156.64seconds, media-grid/path checks and both package layouts passed. Its synthetic merge2a4c82b672949dc5bd81b524babdbde1184e90e5 has parents266313c and67813e7. Every package manifest file was independently hashed. Modern ZIP SHA-25696253a03b17722b70d00e15540621b09e4e2008d9f43f8daad18472efe28daa4; legacy9a26a96b53195b19f697323eabcf64e2e12b1476bff4a8d596c9e25f7410a1f1. Neither was installed.

An additional native-library regression covers a manual/scheduled-style D2 unsupported preview without requesting local stop, plus opted-in session renewal/K5 replay under success,429,503 and transport timeout. It retains the original output/capture/epoch/receipt/profile/preference and does not reopen or downgrade; replay authority uncertainty remains fail closed. The expanded ingest test passed locally15.91seconds. This supplement changes tests/docs only and requires its own final CI; it is not real OBS encoder or deployed-producer evidence.

The90second harness is `http-outage-test`, linked to native `hhc-http-faults` and Qt Network, using a process-local fault marker and loopback HTTP. It is not libobs/NVENC, KeyController ongoing capture, D2 server recovery or production outage proof. Controller policy and D2 metadata are separate deterministic native-library/mock tests. No claim combines these into an end-to-end recovery pass.

Formal installation remains blocked pending producer-first deployment/readback, current CI/package selection, after22:00 safe output/helper idle checks, and real native/private-chain acceptance. Earliest UAC is after22:00 plus these gates; if all are immediately available, a30minute private chain alone makes approximately22:45 the earliest plausible window, not a promise. Backup/rollback uses the verified installed266313c normal package73043580112f05c8b449aa44c0489eb1b12a9a4b49fb9f833a66472f515400cf and preserves OBS32.2.2/settings/credentials/journal/outbox/media.

After22:00, recheck OBS streaming/recording and helper completion before any installation. Preserve settings, key, journal/outbox/media, and backup the installed DLL/helper/TLS set. Prefer one verified normal candidate and one user-operated UAC; never install a fault build or force-stop active output/helper. Retain the verified266313c CI package for rollback; this diagnostic-only change adds no journal migration.
