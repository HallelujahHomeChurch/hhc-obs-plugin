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

The fixed D2 schema/fixtures/version negotiation and rollback rules are not frozen at this checkpoint. Do not infer a five-minute recovery authority from a missing server field or ignore a D1 cutoff/revocation. The platform owner must define freshness independent of retained backlog.

An existing unannounced manual staff-preview source can validate normal encoding/upload/preview without audience publication. It cannot prove public auto-idle or public End behavior. D2 failure recovery must be tested separately against a safe isolated protocol harness after the fixed contract arrives. No direct auto-announced live source may be called a private test endpoint.

## D2 target parsing checkpoint20:56

The platform owner's20:45 wire target was delivered in full through the trusted coordination message. It is not a deployed-server acceptance result. A separate D2 schema and16 synthetic wire fixtures now cover required/null fields, 32-lowercase-hex capture identity, UTC timestamps, safe integers, unsupported capability and unknown states/reasons. Frozen D1/K4/S10 schemas are unchanged; D2 open-input validation extends the complete S10 input rather than the older K4-only definition.

The pure observation guard checks exact recording/capture/epoch, drops an older request generation or revision, processes recovery changes at equal D1 revision, preserves committed public end while accepting capability loss, and rejects cutoff expansion. Only422/broadcast_protocol_unsupported allows control fallback; generic400,404,500 and unknown-authority errors do not. Local D2 observation tests passed. These guards are not yet connected to native polling, opening, or capture lifecycle: production behavior remains legacy until the integrated implementation is completed and reviewed.

Opening eligibility is unresolved for a single fixed key: before streams/open the client cannot know whether the source targets direct SourceManaged or manual/scheduled. Always offering recovery currently rejects manual targets; omitting it permanently creates a legacy capture. Do not remove the offer after409, mint another output identity, or invent OBS scene selection. Platform owner must resolve this contract gate before a complete D2 installation candidate can be claimed. Manual/private-preview opens must omit the offer under the current target.

After22:00, recheck OBS streaming/recording and helper completion before any installation. Preserve settings, key, journal/outbox/media, and backup the installed DLL/helper/TLS set. Prefer one verified normal candidate and one user-operated UAC; never install a fault build or force-stop active output/helper. Retain the verified266313c CI package for rollback; this diagnostic-only change adds no journal migration.
