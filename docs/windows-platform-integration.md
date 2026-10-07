# Windows platform integration ledger — 2026-10-08

Authority: user-approved direct production tests; immutable handoff ea906e45640bd36985684534f36ba4c489f5e6e1, C1 c1-2026-10-07.1, S1 s1-2026-10-08.1. Existing isolated worktree feat/windows-capture continues PR #1; platform repositories untouched.

Verified bytes: CMS OpenAPI ebd1fde66d1f810510eb59484f5cdbc5f78446c1b20b2b85f39217b809589116; fixtures a877807cb3559fb507520596118a6f1c0ddae4c1a87f086018acea3fde7efd9a; Account OpenAPI 8d18660fda96ebcb8380df2e3daa2ccaf8cea3c4074264a9b737f3528b69948c. Consumer acknowledges these fixed schemas; issuer-confirmed principal is required beyond generic OAuth response schema.

Sequence:
1. OAuth code/refresh, draft recording + capture, declare/sign/PUT/confirm/stop/seal and explicit auto-publication.
2. Live controls, whole-event DVR and normal VOD; member website owns playback exchange.
3. Offline retries and account-bound recovery, current-build 2.5-hour qualification.

Pre-flight: capture worker owns journal.json, remote worker owns remote-journal.json; no concurrent whole-journal rewrites. HTTP must consume frozen C1 schemas. Existing offline-validation-only queue remains isolated from OAuth users. Native callback changes /callback to /oauth/callback. Final inventory uses Go struct field order and numeric serialization; master bandwidth must use measured segment bytes rather than configured NVENC bitrate.

Tests so far: wire-test RED (positive schema examples, callback path, canonical digest) -> GREEN; http-test RED (no transport) -> GREEN (real loopback HTTP, one refresh on 401, no refresh on 403, non-JSON 413 and redacted errors); native-auth-test RED (unparsed identity) -> GREEN (human/client identity and exact granted scopes).

Ruling: user explicitly authorizes synthetic tests on production including auto-publication/live — defaults remain false and tests deliberately enable each — cost: labeled synthetic test records may be exposed to entitled members during authorized tests.

Evidence boundaries: unit/mock is not real OAuth, actual NVENC is not platform ready, server ready is not publication, desktop playback is not physical iPhone playback. End-to-end and current-build long-run are pending until observed.

Observed production evidence (not complete acceptance): native system-browser PKCE login and actual refresh succeeded with read/write/publish scopes; Schannel discovery200/anonymousCMS401. Recording4b410b08-892f-46a0-bfcf-6ff68844f756 / capturef0d763fbce234ea290e774d9c008811d aborted before encoding due to precreated journal directory; prepared-directory rule fixed. Recording2a002691-2cd7-4dca-b8be-0b8b7e330375 / capturee5e5d5b869f638e10abbab83b2c02def completed real61s 29.97 NVENC,16objects40,661,469bytes. IssuedHost initially rejected; matching-authority mapping fixed, same queue recovered. Actual PUT/confirm/stop/seal accepted, then serverfailed terminalReasonpackage_failed (9verified/7queued on finalpoll). Fixed Asset Go inventory validation passes and playlist validation fails; master omittedmandatoryCODECS. Actual init AVCC/AAC-derived CODECS added; do not reuse this failed capture or claim ready.

Fresh whole-branch review: eight Important findings (dropped controls, session journal race, rotating credential race, helper scan freshness, lost Stop on terminal state, unbounded shutdown, expired capabilities, uneditable next-event intent) plus nested-journal validation Minor. One grouped fix pass: GUI saves independent control-intents before HTTP, shared per-session QLockFile reloads under ownership, per-account vault entries and cross-process refresh locking reload latest rotation, helper detects newly completed sessions, terminal server failure explicitly stops encoder, cooperative HTTP cancellation + one-object work units, storage403 re-sign once, next event returns editable Ready with live/publication false, validate persisted IDs/mutations before API. Existing legacy vault migrates once without credential logging. Local10CTest pass; expiredPUT, durable controls, ownership exclusion, cancellation<2s and worker error propagation covered by mocks/loopback. No claim that these cover actual long-run/player behavior.

Ruling: original F1/F1-L remain retained; direct native production tests replace cross-machine material delivery per user. Member web session is separately awaiting human login in Codex browser; no credentials requested in chat.
