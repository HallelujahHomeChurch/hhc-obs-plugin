# Mac follow-up: Console renewal and member-list live thumbnail

Observed on 2026-10-11, Asia/Taipei UTC+8. Windows product is `9cbeec04d79a09eee89c37f0a92460b80708a805`; CMS `hhc-web-api--0000183`, Asset `asset-api--0000090`, Website `hhc-web--0000200`. This is a browser Console result, separate from native OAuth refresh and OBS encoding/upload.

Recording/broadcast `e496a04e-4e2e-4dad-9269-63a610dd7c06`, capture `bcccb147fbd332d73a9bae51097fa633`. It remains active for long acceptance; do not delete, abort, stop, seal or publish it from Mac.

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
