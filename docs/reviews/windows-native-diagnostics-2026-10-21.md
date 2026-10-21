# Native HTTP diagnostic persistence — 2026-10-21

## Problem and scope

The native OBS field using source `139b740ce7db4d87d6fb52aad037873fb2c2a228` produced no persisted HTTP/renew/queue/sync diagnostic lines. Only the standalone helper installed its Qt handler. Consequently the later upload backlog cannot be attributed to any individual HTTP phase from that field: missing timings, statuses and retry history are unknown, not zero.

Keep the existing safe event formatting. Route those HHC calls through a process-local sink and the existing bounded helper writer; native OBS registers this sink before starting its controller and restores it after controller shutdown. Native writes use a mutex and a separate `diagnostics/native` directory. Other plugins' Qt messages are not intercepted. The helper retains its existing handler and directory. No protocol, retry, encoding, queue, credential or publication behavior changes.

## Verification

- Loopback HTTP tests cover persistence without a helper, operation/status/attempt/elapsed/request ID fields, secret/URL exclusion, unrelated Qt messages and sink restoration.
- Ingest tests cover persisted renewal begin/success/rejection and secret exclusion.
- Existing rotation/locking checks remain in the HTTP suite.
- Full local CTest: 26/26 passed before final documentation/build; final verification is recorded in the PR.
- Independent code review: no blocking findings. Review was source-only, not an OBS execution.

Tests do not directly exercise the OBS frontend callback registration/shutdown or concurrent native controller threads. Hosted CI cannot establish NVENC or true OBS acceptance.

## Existing public field and authority

The original field `29d96b8b-06dd-46b0-bf42-07a05a1a497a` / capture `713baa91fed4695e6822a8f8d307b193` was normally stopped once at 2026-10-21 17:16:55.936 Asia/Taipei because upload backlog regressed. Stop and seal receipts were accepted; full inventory contains 1744 objects / 3,893,109,339 bytes, 579 segments per rendition and 5795.790 seconds. This is a partial 96-minute field, not a passed two-hour/near-limit/load test. Natural validation/publication remains a separate server observation.

The Asset capture's `autoPublish` field must not be substituted for the CMS-bound broadcast policy. Do not manually publish or change this record to resolve that discrepancy; authoritative CMS state and server cause require read-only correlation.

## Candidate gate

PR/required CI and unsigned candidate packaging are authorized. Merge, runtime install, OBS restart and another real capture are not authorized by this source change. A fresh device gate must verify the installed revision, startup marker and actual safe HTTP/renew persistence before calling native OBS validation passed. The existing field and its queue/media/journals remain untouched.
