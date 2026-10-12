# Current candidate long-field acceptance, 2026-10-12

This run is ongoing. No two-and-a-half-hour, whole-source decode, final readiness or automatic publication pass is claimed yet. All event times are Asia/Taipei UTC+8; raw observers use UTC ISO timestamps with explicit offsets.

## Fixed inputs and scope

Installed product commit c2166da44da0d2fccbd4f84d82aa54a2fa03ad55, unsigned ZIP SHA-256 daa2a94841fb9506e02ce761266bf00ddb4eb5820eb9460a6e63b95888021fe6, DLL SHA-2562108c80d9eb58961ace5bc0a46dabac39532ccfbe0ee67460a51dcb6c4f55337. Native candidate CI success is separate from this formal test. C1 c1-2026-10-08.2, B1 b1-2026-10-10.rc1 / document b1-mac-2026-10-10.rc2, inventory1. Eleven immutable C1 sources were freshly rehashed during the preceding short field; fixtures are not uploaded as media.

Actual platform during this continuation: Account0162 / source93bba252fce803c5f694743357885be472bceb23, CMS0187 with writer enabled, Asset0090 with HLS/source enabled, Website0203 / sourcebeeba54c6462639844e1fa17b6b32cc184f89349, loaded Admin34b5073f6d821fde400a4a1a1de583861be58f0f. These actual revisions, rather than older handoff versions, identify the evidence. No platform modifications or deployment are performed.

Recording/broadcast **1a336a82-3540-4037-8a49-6e1ace08c314**, capture **0b4a109f78c45cdc0f06ab1b2c4d2541**, local **b85118d1-8dd8-4752-8193-1d8bd82e6b7a**, epoch1. Manual Start/End, automatic VOD publication, initial auto cover. The preceding short recording remains available only while its scoped weak-network diagnostics are needed; protected e496a04e-4e2e-4dad-9269-63a610dd7c06 remains read-only.

OBS32.2.2 portable HHC-Fixture encodes synthetic Program clock/track1 at30000/1001fps, three NVENC/AAC renditions with900-frame/30.03s segments. Source begins **13:06:39.554–.580**. The independently owned synthetic global NVENC HEVC recording starts **13:07:17.086**, file D:/Users/IT/Videos/2026-10-12 13-07-16.mkv. Earliest qualifying source Stop is15:36:39.580, global recording Stop15:37:17.086. Both must finish normally before a long-run result is asserted.

Global YouTube/streaming is not started. This is synthetic source plus concurrent local recording and browser decoders, not actual congregation/YouTube or physical-network qualification. Original profiles are preserved, and CPU encoder fallback is not enabled. The current OBS launch has no armed manual Qt transport trigger receipt; do not fabricate an injected outage. Additional controlled transport/crash tests are separate work after this continuous recording.

## Start and baseline

Start command **fb9282c8-d188-462f-8302-db8b07d50d85** is durably acknowledged at encoder common boundary **5**, original key retained. Public end is unset. The original waiting member page becomes a player without reload; a normal Play click begins actual1080p/1x playback. No “Back to live” action is used.

Actual burned-in clock sample: wall **13:12:51.889**, decoded frame **13:10:45.169**, media time96.2354,1920×1080,1095 frames/error=null; delay **126.720s**. JPEG SHA-256 **71fb5efeecea6cc78da25b1942fb083e75c8b7d953e066fe4c4cdd30384be9bf**. Subsequent latency uses this media-time/clock anchor, subject to later drift checks; it is not computed from a live badge.

A passive browser probe samples every5s: actual dimensions, currentTime/duration, frames/drops, buffered/seekable, readyState, paused/rate, error and anchored wall-clock latency. A separate passive local observer samples minimal queue/control/process data every5s with shared read/write/delete file access. Neither changes media state, network or journal contents. Raw evidence is in ignored artifacts/b1-current-long-20261012; no credentials or signed URLs are included.

## Uninjected stall and automatic recovery observation

At13:20:55.699 actual1080p media time580.045823,15595 frames, dropped7, readyState4/playing1x/error=null, delay126.720s. Local pending bytes are zero at13:20:44.446 and13:22:34.826. Segment closure naturally produces temporary queue bytes; do not treat one nonzero sample as an unresolved upload outage.

No transport fault is injected. At **13:24:16.426**, original playback waits at780.772563. At13:24:51.457 actual Console health is healthy, verifiedEndSeconds930.93, public start5, no end. `queuedObjects` and `lastConfirmedAt` are null/unavailable, not zero. Player delay grows while waiting, reaching215.991s at13:25:45.697. It resumes at **13:25:50.077**, then waits again at **13:26:20.076**, resumes **13:26:51.147**. No reload, login, manual seek or “Back to live” is used.

A real1080p playlist HTTP200 at13:26:15 contains sequences5–31,27 segments totaling810.81s, MEDIA-SEQUENCE5 / EVENT, no ENDLIST, Cache-Control private/no-store, fromDiskCache=false/fromServiceWorker=false. The same-page watch response reports view=live, liveState=recovering, progress lastSequence31/mediaEndSeconds960.96/lastAdvancedAt13:25:39.921408, mediaStartSeconds150.171/durationSeconds810.81. Response bodies are decoded according to the CDP base64 flag before parsing; an initial zero-count observation before decoding is superseded, not a platform empty-playlist failure.

At13:27:05.689 playback resumes but delay is251.434s. Automatic seeking/playback recovery at13:27:23.297/.298 moves to900.908999;13:27:45.694 delay193.515s. Another automatic update yields **13:28:45.687**, currentTime1044.758409/duration1111.140666,1080p/1x/playing/error=null,25783 frames/dropped26, delay **131.995s**. This proves scoped automatic recovery after a naturally observed stall; stability, ordinary physical-disconnection recovery and the five-minute/ten-minute pressure gate remain separate and unpassed.

## Thumbnail checks in progress

Initial auto cover is ready, revision1, selection.mode=auto, coverId342408181862cc686c230be3e32e5ae5, inheritedRevision6. At13:28 a synthetic decoded-clock JPEG is uploaded through the ordinary crop dialog as a per-field fixed cover. Save PATCH returns200 and Console confirms saved/processing. Final JPEG bytes, list/poster propagation, resistance to late auto work and switching back to cached auto remain to be checked. Shared default is not changed by this per-field operation.

## Outstanding gates

### Follow-up observations through14:09 UTC+8

The original member page remains open without reload or manual catch-up. At14:07:20.699 it decodes1920×1080, currentTime3380.318175/duration3393.423333, playing1x, readyState4,93752 frames/dropped61, error=null, anchored delay111.447s. Subsequent short waits and automatic seeks are retained in the passive event log; this is not a claim of uninterrupted playback. A second burned-clock sample at13:35:13.721 shows13:33:02.387, actual delay131.334s, anchor drift0.661s. At13:58 GPU RTX4060/driver617.14 reports40% GPU/44% encoder,1770MiB,51°C. These are point samples, not capacity guarantees.

Current authorization tests have positive controls. Using the actual member media session, confirmed private sequence4 returns HTTP404 between two successful public sequence5 HTTP206 responses at13:44:33.604/.737–13:44:34.024. Sequence4 exists locally in all three renditions and each remote confirmation has acceptedAt; denial is not inferred from a missing upload. With an independently valid member grant temporarily scoped to the observed staff-preview path, actual preview returns HTTP401 between staff HTTP200 controls at13:48:55.912/.974–.56.174. Original staff cookie is immediately restored and member cookie unchanged. CORS fetch TypeError alone is not treated as the HTTP result; responseReceivedExtraInfo supplies the denial status. No grant values are saved.

Per-field custom cover7606c37969518902d6d2ae4356b58da1 is ready, JPEG66573 bytes/SHA-2568100f4fb4678624252c6ab990de0b671616a88fbdf61dfff1c49dedfbd1ce5c4. Console/member-list decoded1280×720 pixels match. Switching back to auto retains original cover342408181862cc686c230be3e32e5ae5, revision3; JPEG64331 bytes/SHA-2562baa5ad611f92c84a92486926667c9e1058d84466fcc079e2f32769a6e33f358. Fresh Console/member list show matching decoded pixels. The original long-open Console does not show its current image after this switch, despite API ready and a successful fresh-page image; this is a separate Admin UI failure, not a plugin or media-validation failure. No intentionally late auto job was injected.

Shared default is temporarily changed to custom revision7/covercedd4611569996865b679672b84ee6ab, then **restored to auto revision8/HTTP200**. The next pressure draft00c23cc8-2df6-4225-8099-5112ace21623 inherits revision7 and has no binding or announcement. A second shared-cover drafta39594ee-890b-4780-ab9d-7827ea9d69f4 is deleted HTTP204 after checking the same inherited asset. The retained pressure draft still returns ready, inheritedRevision7 and the identical66573-byte JPEG/hash. Shared-asset cleanup therefore preserves this other reference; no second active capture is created.

Finished disposable recordingscb129c48-28ad-41bb-86e8-57250709e128,744f5b7e-f68a-478b-9670-b340260417c8,9983fe7b-3f70-4829-98cd-57e0e1c4fb80,9a39862a-1034-49c7-ba9b-ef9ca075e834,d48eeb42-bced-4e5b-9444-691a986fbdf1,22da5c33-0e08-4ac4-b973-891c06766a9a and the shared-cover draft are absent from loaded Admin/member lists. Local journals/failure evidence, current long/next pressure field and both protected e496a04e/e9c5ebb4 originals remain. Real meeting recordings are preserved.

Both native hosted CI runs38208470865/38208467112 pass on report commitac14905696f35d0a6d726b7979091f83bbfad213. Installed product remainsc2166da and its original ZIP/DLL hashes; documentation CI does not requalify the running binary.

- Continuous150-minute source/global recording, all-file inventory hashes, whole-source decode, Stop/seal/ready/automatic publication and public end boundary.
- Public endExclusive denial after Console End; current staff-preview and before-start authorization checks above pass.
- VOD cover inheritance and search/recommendation/poster matrix; per-field reuse, shared-default inheritance and shared-reference cleanup above are separately evidenced.
- Normal actual OBS/local recording/YouTube plus physical disconnection recovery. Synthetic and browser throttling cannot substitute for it.
- Five-minute transport interruption/ten-minute catch-up, crash recovery and continuous-viewing results.

Historical379/388s lag, rapid-switch18.491s and weak uncached seek32.730s remain failures/limits. The separate short field has successful ready/automatic-publication, auto-cover byte inheritance, EOS/quality regression and scoped weak-media downgrade evidence; those do not certify the remaining gates here. No completion, merge or production deployment is claimed.
