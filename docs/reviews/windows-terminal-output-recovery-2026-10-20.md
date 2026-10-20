# Windows terminal output recovery — 2026-10-20

## Scope

An already bound K4 output can lose its ingest session after its recording is deleted.
Previously Retry resumed the same invalid session, then paused again and disabled Start,
even when the local active pointer already said released. This change does not diagnose
the earlier live-signal interruption. In particular, the later deletion-related 401 is
not evidence that the initial interruption was caused by authentication.

## Recovery rule

- Keep normal renewal first. Only renewal HTTP 401 with the exact known
  `broadcast_ingest_session_invalid` code on a bound K4 journal replays authenticated
  `POST /api/ingest/streams/open` with its original durable outputSessionId and profile.
- The existing server output ledger must recognize that original identity. No new ID,
  resolve/bind fallback, Admin credentials or anonymous status endpoint is used.
- Successful replay must match original source, recording, capture, epoch, binding,
  profile and receipt before accepting credentials. Active output resumes that identity.
- Only HTTP 409 `broadcast_output_terminal` permits retiring the old output. Persist
  the terminal marker and released pointer; retain media, queue and receipts. The next
  output is created only by an explicit user Start. Server input/capacity guards remain.
- Generic 401/403, timeout, busy, malformed responses and identity changes do not release.
  Legacy K3 keeps its existing behavior.
- A short per-output metadata lock and reread make terminal persistence monotonic:
  a delayed control renewal cannot overwrite another worker's terminal decision.
  No lock is held over HTTP requests. Credential writes are outside that lock: a
  same-binding renewal already past the disk check can still write a credential,
  but cannot erase the tombstone or reopen the capture.
- If a terminal response arrives during HHC encoding, stop only that HHC capture;
  do not invoke global streaming or original-recording stop callbacks.

## Verification

Local isolated source worktree, no live requests, capture, upload or notification:

- K4 regression first failed on the unmodified source: active revalidation, authoritative
  terminal classification, same-ID retry and terminal restart assertions.
- A second regression first reproduced a delayed renewal erasing the terminal marker.
- After the fixes, CTest `c1-http`, `k3-ingest`, `k4-ingest`: **3/3 passed**.
- Cases cover valid renewal, active replay, terminal, occupied input, generic 401/403,
  timeout, source/recording/capture/epoch/profile/receipt mismatch, retained media,
  retained credentials and receipts, restart, and two stale ingest instances.
- The controller test adds non-409 terminal-code rejection, paused-to-terminal Start
  enablement, and verifies tick does not automatically create the next identity.
  Local offscreen Qt execution produced no result and was stopped; this is not a pass.
  Full Windows UI tests and candidate package are delegated to normal PR CI.

No changes to production OBS, installed DLLs, settings, stream key, live journals,
original recordings or protected media. OBS remains 32.2.2. No real OBS acceptance or
production update is claimed. HTTP/renewal diagnostics are a separate focused follow-up.

## Rollback

Nothing is installed by this PR. Preserve the currently installed package and its
existing backup. Any eventual installation or rollback requires recording to have
ended and explicit authorization; replace only the plugin using the established
backup/install workflow, never OBS itself or its Qt/FFmpeg DLLs.
