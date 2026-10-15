# Member playback acceptance failure: quality change followed by backward seek

Observed on the production member page on 2026-10-08, approximately 19:50–20:10 Taipei. This is an actual authenticated Chromium playback check, not a mock. No platform repository was changed.

Recording: `7d4ca4d0-3989-40f5-89d5-b2bb71336f2d`.

Member page: https://www.alive.org.tw/zh-Hant/member-videos/7d4ca4d0-3989-40f5-89d5-b2bb71336f2d?page=1

This previously published synthetic live/backfill recording was produced by `e8fc296a72220a5c35ac960fdf4e0cf412d1f109`. Its actual inventory contains 21 fragments per rendition, 617.183233 seconds at 30000/1001 fps. The current plugin candidate is `f009725b49a7ea8cb24745f9df660d5f6351da4a`; this browser check does not establish positive media E2E acceptance for that candidate.

## Reproduction for the platform owner

1. Open the member page and start the recording.
2. Select 720p in playback settings, seek to 300 seconds and resume. Confirm actual 1280×720 decoding.
3. Select 480p and immediately seek backward to 60 seconds.
4. After playback resumes at actual 854×480, inspect the displayed total duration and playback slider maximum.
5. Pause and try seeking to 610 seconds.

The first observed failure kept duration and slider maximum at 480.511999 seconds; the second reproduction after a reload kept both at 337.855999 seconds. Both rejected a 610-second seek. Actual 480p playback had readyState=4, ended=false and no HTML media error. Thus playback resuming does not mean the full event is accessible.

Expected: quality changes and backward seeks retain the complete approximately 617-second event and permit seeking into its final recorded interval.

## Control and limits

Reloading the same page, selecting 480p and seeking directly to 610 seconds played the tail through ended=true at 617.215999 seconds, actual 854×480, readyState=4 and no HTML media error. The observed successful 480p media playlist returned HTTP200, 21 EXTINF entries summing to 617.183233 seconds, MEDIA-SEQUENCE=0 and ENDLIST. The small final video-element duration difference includes the AAC tail and is separate from the large reproducible truncation.

1080p playback of the post-live-close interval and 720p middle playback also decoded successfully. The complete playlist and successful tail control show that the affected recording has full media available. The root cause of the duration mutation is not established; investigate the player quality-switch/backward-seek state before attributing it to encoding or changing the immutable inventory.

Safe DOM measurements and the exact sequence are retained in the adjacent JSON evidence. Browser screenshot capture timed out through both supported attempts; there is no screenshot evidence. Network observation was truncated, so it is not a complete request trace. No credentials, signed URLs, raw response headers or playlist URIs are retained in this report.

This is a failed member-player acceptance item. Local OBS, hosted CI, prior publication and successful playback after reload do not close it. The separate local original-recording/HHC 9000-second run remains in progress; fresh current-candidate production testing still requires lawful release of the occupied draft quota.
