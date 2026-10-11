# Mac follow-up: Console renewal, live thumbnail and End DVR preservation

Observed on 2026-10-11, Asia/Taipei UTC+8. Windows product is `9cbeec04d79a09eee89c37f0a92460b80708a805`; CMS `hhc-web-api--0000183`, Asset `asset-api--0000090`, Website `hhc-web--0000200`. This is a browser Console result, separate from native OAuth refresh and OBS encoding/upload.

Recording/broadcast `e496a04e-4e2e-4dad-9269-63a610dd7c06`, capture `bcccb147fbd332d73a9bae51097fa633`. Windows has normally ended/stopped/sealed it for long acceptance. Keep it read-only from Mac; do not delete, abort, resend Stop/seal or manually publish. CMS changes from0183 to0184 during the field; Asset0090/Website0200 remain unchanged.

Copyable handoff:

> Please investigate Admin Console staff-preview access after its browser access token expires. Keep B1/C1, digest, idempotency, member/media authorization and the Windows plugin unchanged.
>
> Console was opened before the current broadcast started. Staff preview actually decoded 854×480 before Start at 16:18. At approximately 17:02 and again 17:04, clicking 「載入預覽」 returns HTTP 401 from the Admin-origin preview POST. Its attached bearer has `exp` corresponding to **16:22:21.000 UTC+8**, already expired. A following Admin-origin POST whose path contains `refresh` returns 200; another operator preview attempt still fails. No raw credential or response token was retained. Error body was unavailable, so no invented error code is reported.
>
> At approximately 17:06, reload only the Console. It returns to the same live broadcast/binding. A subsequent preview load truly decodes 854×480 at **17:07:29.517**, currentTime13.144345,365frames,readyState4,playing,errornull. The member follower is never reloaded and native OBS keeps recording/uploading the original capture.
>
> At approximately17:35, a fresh preview attempt reproduces the problem on the reloaded Console: the attached bearer expired **17:21:45.000 UTC+8**, preview POST returns401, refresh POST returns200 and the error alert remains. This is a second token-expiry cycle, not only a stale pre-reload observation.
>
> These observations establish an expired bearer on the failed request and successful reload control; they do not identify the exact client implementation defect. Trace token renewal, current-token propagation and replay of the original preview operation. Check other long-lived Console write/control actions for the same behavior. Preserve operation keys and all existing Start/End semantics; no parallel capture or permission expansion.
>
> Regression should leave Console open beyond token expiry, then load/reload staff preview without a page reload or login. Verify actual media decode after the successful refresh, not just refresh200. Separately exercise End with the same long-lived page while original OBS recording/YouTube are preserved. Report exact deployed Admin/client versions, CI, request statuses and UTC+8 times; never include tokens, cookies or signed URLs. Return a fixed-commit Windows continuation prompt after release.

Sanitized evidence: ignored `artifacts/b1-recheck-long-20261011/preview-expired-bearer.json`, `preview-retry-401.json`, `preview-after-console-reload.json`. Reload is a recovery control, not a fix or a passing long-lived-auth result. Windows source endurance, original-player recovery, final End/Stop/seal/ready/publication and media QA continue independently.

## Member list does not display the current live JPEG

> On the same fixed platform/candidate and active recording, Console displays a real1280×720 live JPEG. At17:42:54.279 it is63,288bytes, SHA-256 `bb8549345c73aebfd04cee987ba68a3646437321535d70e2185ab5065a9393dc`.
>
> A newly opened member-list page at17:55–17:57 shows the current recording as live, but its card uses the HHC brand fallback. At17:57:09.479 the card contains only a loaded512×512 brand asset (`size-14 object-contain`), no thumbnail image and no CSS background-image node. The member watch200 response separately contains a nonempty `cover.revision` and `cover.contentPath`; their private values/URLs are not retained. This establishes the list display mismatch, not its implementation root cause. Search/recommendation/poster are not extrapolated from the list.
>
> Please trace B1 live-cover metadata into the member-list DTO/client image loading and fallback behavior. Preserve permission checks, C1/B1 schemas and live/public-range semantics. Regression should use a newly announced/bound/started broadcast: actual live JPEG in Console and member list, then unchanged selected cover after automatic VOD publication. Check search/recommendation/poster separately. Report deployed versions, actual image bytes/hash and failures; provide a fixed-commit Windows continuation prompt after release. Do not mutate the active Windows endurance field from Mac.

Sanitized list evidence: ignored `artifacts/b1-recheck-long-20261011/member-list-live-thumbnail.json` and `live-thumbnail.json`. The actual member follower continues decoding; a missing card thumbnail is not a playback failure or proof that upload/validation is broken.

## Console End moves an active DVR viewer to the live tail

> On Website `hhc-web--0000200`, the independent member DVR page46 is playing1x/Auto at its deliberately chosen earlier position. No reload, seek, quality change or Back to Live action is made around End. At **18:56:45.460 UTC+8**, it truly decodes1080p/currentTime7497.465265/errornull. At **18:56:50.462**, currentTime9429.45: a1931.985-second jump over5.002seconds. The browser seeking event is18:56:50.309. Console End was requested18:56:28.859; B1 accepts next common boundary327 and preserves public[10,327). HHC Stop occurs later18:57:28.636, so the observed DVR jump precedes SourceStop. The original follower is a separate unmodified page.
>
> Please trace player state preservation across End/end-pending/processing media reconstruction. Retain active DVR position, pause and playback speed; only a viewer who was following live may automatically follow the new edge. Keep B1/C1/public-range/media authorization unchanged. Do not use verified sequence as a substitute boundary or relax the public interval.
>
> Regression needs one original follower and independent active DVR, paused DVR and non1x viewer, without reload/manual live jump. Exercise Console End, then SourceStop, validating, ready and automatic publication. Verify actual decoded currentTime/duration,buffered/seekable,pause/rate and same URL at each stage; a final successful publication cannot overwrite this transition failure. Return a fixed-commit release prompt with actual deployed versions and UTC+8 evidence. Never include credentials or signed URLs.

Sanitized five-second measurements and browser events are in ignored `dvr-end-transition.json`; current-time deltas are observed, not an implementation-root-cause diagnosis. Platform final validation/publication continues independently.

## Final context for the next Mac action

Windows normally completed the same field. CMS readyAt19:26:07.193480UTC+8;991/991 objects verified. By19:29:06.225453848,C1 autoPublishpublished and CMS statuspublished/version3. No manual publication or contract/media relaxation. Public[10,327),9519.51seconds remains fixed; first selected auto-cover bytes/hash exactly matchbb854934…393dc. The original member URL receives VOD and plays/seeks retained outage material; an independent paused0/1.5x viewer preserves its state across ready/publication and later resumes within replayUntil22:13:33.400868 without reload. These successes do not erase the active DVR End failure.

Actual final revisions remain CMS0184/Asset0090/Website0200. Keep this published test field and original replay evidence read-only until Windows/Mac review is complete. The later Windowsc2166da presentation-only candidate does not change platform APIs/media or substitute for a Mac fix.

Additional observation for triage: after publication, original page45 briefly exposes **無法播放媒體** during Home→1080p at19:29:41.896 and Auto at19:30:21.901. Actual video error isnull, successful decode/seek continues and the message is cleared by19:30:41.899 without reload. Network capture began too late to identify a first-request HTTP cause; do not invent an error code or classify this as a persistent decode outage. Trace the error/loading presentation while rebuilding media sources and preserve the last frame and chosen playback state. See [the final Windows report](windows-b1-recheck-long-2026-10-11.md) for precise scoped outcomes.

Please finish fixes and appropriate CI, then provide a fixed-commit continuation prompt containing deployed revisions, enabled capabilities/health, changed behavior and targeted Windows acceptance cases. Preserve B1/C1, wire/digest/idempotency/permissions,29.97fps and900-frame segments. Windows will separately retest long-lived Console, active/paused/non1x DVR at End, live-cover surfaces and any changed quality-rebuild path; actual church/YouTube-load recovery remains an independent gate. Do not describe successful publication or platform CI as all these tests passing.
