# K4 Windows local implementation — 2026-10-15

Scope: Windows x64 only, OBS 32.2.2. Candidate implementation, not production or real OBS acceptance.

## Immutable acknowledgement

Platform source: `HallelujahHomeChurch/hhc-web-api@5e8c33ec1b3ca5628605fab0a578ca8312d67d1d`, `docs/obs-capture/studio-v2/windows-handoff.md`.

- K4 `k4-2026-10-15.rc1`; manifest SHA-256 `e984a5a54b80c817ba701f6923d73c02b57bf9c5b7d238f15442543c5ae18464`.
- All six K4 manifest artifacts and eight frozen K3/B1/C1 entries checked against raw immutable bytes.
- All eleven artifacts referenced by the C1 manifest independently fetched and hash checked, including CMS, Asset, Account and Gateway OpenAPI.
- Additive K4 schemas/fixtures frozen by `scripts/freeze-k4.py`; existing generated K3/B1/C1 definitions unchanged.
- K3/B1/C1, inventory 1, 30000/1001 fps, 900-frame / 30.03-second media boundaries unchanged.

## Implementation

An explicit Start persists a K4 opening intent with the durable localId as outputSessionId before any network request. Only POST `/api/ingest/streams/open` bootstraps new outputs. HTTP 200 must contain a completed, correlated binding/receipt/session. No K3 resolve or B1 bind fallback occurs for K4.

Identity is pinned before credentials are saved. Journals contain no stream key, session token or signed URL. Tokens retain the existing Windows Credential Manager namespace and existing scoped renewal/routes. Legacy K3 intents keep their original protocol, key and binding behavior.

Cancellation writes a separate durable stopping marker, avoiding concurrent replacement of the worker's opening journal. An uncertain opening cannot be retired as unbound. Restart/background cleanup resolves the same output and uses the existing capture cleanup. A late response after public End cannot restart an encoder; existing media and capture journals continue original Stop/seal. A server terminal response persists a tombstone and never triggers another automatic open. Only another explicit Start creates a new output ID.

Transient opening errors use 1/2/4/8/16/30-second exponential delays with ±20% jitter and honor larger Retry-After. Non-retryable failures preserve intent and ownership. Cleanup helpers retain their 24-hour bound; the deadline never proves the remote capture was released.

## Local verification

- 22 CTest suites passed in the implementation run, including new K4 wire/identity/opening/transfer tests and existing K3, B1, C1 recovery suites.
- K4 mocks cover lost response / same-ID retry, cancellation + restart + original abort, End before initial response, completed binding adoption without another bind, identity mismatch, 202 rejection, error retention, terminal restart fencing and Retry-After/backoff.
- Mock dock covers stopping persistence, restart, prevention of a second output during uncertain cleanup, terminal without automatic restart, and missing K4 intent without legacy fallback.
- Existing legacy journals/renewal remain covered by `k3-ingest` and B1/C1 suites.
- Python regular-frame/short-tail QA and Windows watcher exit-code/path checks passed.
- Local mocks use only synthetic fixtures and injected transport; none of these results is real OBS/GPU or production K4 evidence.

Final source commit, Windows CI runs and candidate ZIP hashes are supplied in the PR and delivery report. Packages retain legacy and modern layouts; neither is installed as part of this K4 work.

## Pending / preserved limits

Wait for Mac's immutable deployment versions and enabled-feature evidence, then reconfirm the meeting window and existing authorization before the smallest real OBS short test. Verify direct/scheduled selection, same-output recovery and End→Stop→seal using that actual deployment. No K4 production calls, key reset, recording deletion, manual publication, OBS upgrade or other output changes are authorized in this phase.

Lost initial response followed by lost delegation can require an authorized Console operator; a new output ID is not a workaround. The candidate remains unsigned. Existing player, 16-minute recovery and 2.5-hour evidence is retained without rerunning those tests; historical latency/weak-network failures and untested meeting + YouTube load remain unchanged.
