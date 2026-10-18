# S10 upload backlog: Windows correction and platform follow-up

## Observed production problem (Asia/Taipei UTC+8)

Running production plugin: c7e68b738487b50e4bfa6ff3f979557d65e14cfa, OBS32.2.2.
Recording: `c41fab8b-2e42-438a-b32f-8534e54cf443`.
Capture: `fcdd4c04a9db64bdb6d0dff27b4c0f0f`.
Output: `55408f45-74ce-4865-8c5a-f5396a113d89`.
Profile: obs-hls-10s-v1, 30000/1001fps, 300frames/10.01s.

Read-only local journal observations; production output was not stopped, restarted or modified by the agent:

| Observed time | Closed objects | Receipted confirmations | Closed max sequence | Confirmed max sequence (1080/720/480) | Platform verified sequence |
| --- | ---: | ---: | ---: | --- | ---: |
| 08:47:51.927 | 165 | 148 | not sampled | not sampled | 38 |
| 08:48:06.612 | 171 | 151 | not sampled | not sampled | 40 |
| 08:52:27.113 | 249 | 209 | 81 each | 68/68/67 | 63 |
| 08:54:08.914 | 265 | 234 | 85 each | 76/76/76 | 75 |
| 08:55:01.428 | 265 | 247 | 85 each | 81/80/80 | 79 |

At08:52:27, 40 closed objects (89.416MiB) lacked confirm receipts. This is a receipt-based diagnostic, not an exact instantaneous bytes-on-wire count. Persisted lastCapture deliberately omits the objects list; absence of that list MUST NOT be interpreted as all objects pending.

Recent individual confirm acceptedAt intervals mostly3.5–5s, with one approximately18s gap. Continuous three-rendition production requires3objects/10.01s, or an average completion interval no greater than3.337s before overhead/catch-up headroom. Hardware saturation is NOT established. API/TLS latency, PUT duration and retry delay were not separately timed on the old installed binary.

At08:52:27 approximately14segments/140.14s separated fully confirmed media from newest closed media, and another4segments/40.04s separated fully confirmed media from verified media. These sequence deltas are media pipeline gaps, not measured viewer wall-clock latency. `liveState=recovering` was observed; it changed to `ending` after the operator stopped output. The agent did not initiate Stop/End.

Existing Console video at08:51:27.963 decoded854x480, currentTime90.766744, readyState4,2712frames, playing1x, MediaError=null, buffered[64.064,100.099999], seekable[]. This is staff preview evidence only. The affected member device was not available; member seekability/follower recovery remain unverified. No reload/seek/playback changes were made by the agent.

## Windows correction

CaptureSync previously declared, signed, PUT and confirmed ONE object per complete synchronization round. The key path also polls adoption/control/status before each round; each httpRequest currently constructs a separate QNetworkAccessManager. S10 triples object frequency compared with900frame fragments, amplifying this fixed serial round-trip overhead without necessarily tripling media bitrate.

Use existing C1 batch contract (declare/sign/confirm allow up to100 entries), with a small maximum of THREE closed objects per round. PUTs remain sequential and byte/hash validated; no new concurrency, connection pool, credential storage, wire version, media preset or public-boundary change. Small bounded batches avoid placing the whole backlog ahead of control recovery. Confirm only after all selected PUTs succeed; an uncertain confirm retains and replays its original operation key/body. Complete retained inventory still seals normally. Subtract newly accepted batch bytes from the displayed pending snapshot so completed work is not displayed as pending until the next round.

Scope reaches CaptureSync callers (key transfer, OAuth controller and background helper), finalization/recovery mock fixtures and CTest registration. Both DLL and helper must be updated together after the operator's active session safely finishes. The change does not rewrite any existing journal or touch the OBS executable/settings.

## Verification

- RED: new batch/pending assertions fail against the old one-object code.
- GREEN: complete local CTest25/25 pass,15.16s.
- Ten-object mock inventory uses4declare/4confirm calls instead of10each; exact ten uploaded objects and full seal inventory retained. This is call-count evidence, NOT a production throughput benchmark.
- Partial PUT failure after one successful object: reconstructed CaptureSync resumes original capture and finishes exact inventory.
- Confirm accepted but503 response returned: restart replays identical original batch body/key, no second capture.
- Signed capability403 renewal, Stop before upload/seal, B1 seal fence, local ownership lock, missing package/terminal pagination and durable ready retention regressions pass.

NOT YET PROVEN: new binary production sustained throughput, normal meeting/YouTube combined load, member seek/follower recovery or end-to-end latency. Earlier379/388s latency,32.730s weak-network seek and other historical failures remain failures. Local tests do not erase them. No automated new production capture is part of this fix.

## Mac handoff (no blocking wire change requested)

Please correlate the recording/capture/output above over2026-10-18 08:38–08:56 UTC+8:

1. Ingest declare/sign/confirm/status latency, throttling/retry and the18s confirm gap around08:51:46–08:52:04. Separate API round trips and object upload confirmation from verification/playlist publication.
2. Verify S10 worker capacity at3renditions per10.01s. At08:52:27 confirmed common sequence67 vs verified63, while producer reached81. Determine whether verification is bounded after upstream catches up; do not assign all backlog to GPU or bandwidth without evidence.
3. User reports member playback stall and inability to drag to newest available media. Read actual published playlist edge/media sequence, safe playback boundary, member seekable/buffered/currentTime and follower recovery. Respect intentional DVR/paused/non1x semantics. An empty staff-preview seekable range alone does not prove a member-player bug. Do not conceal missing media by seeking beyond verified/public limits or relaxing verification.

Windows batch correction can proceed independently. Return actual revisions, findings and only necessary minimal recheck; no new protocol required for existing batch APIs. Do not stop/restart the operator's OBS, manually publish, re-upload, delete this recording, reset keys or send Stop/seal again. No secrets or signed URLs in reports.
