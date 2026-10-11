# Windows B1 fixed-platform followup — 2026-10-11

Status: partial browser acceptance; new OBS field blocked. All times below are Asia/Taipei UTC+8. Previous failures remain historical failures.

## Fixed source and scope

- Platform handoff: [efa914b production followup](https://github.com/HallelujahHomeChurch/hhc-web-api/blob/efa914b5397a877403a3eec5d6f8ef19416e98a6/docs/obs-capture/live-control-v1/production-followup-2026-10-11.md), byte SHA-256 `b5a628c966ae8fb2871e268a1443492c5b4532cca9d81a4f10efa660683641f0`.
- Companion evidence SHA-256 `f5f2fd48f61f762b894de498cac6314883aabfb113a3074d1fc477e54e800d69`.
- Installed product commit `c2166da44da0d2fccbd4f84d82aa54a2fa03ad55`; reporting HEAD before this report `c9c9abc368fb236572edefc413ef751ef617f362`.
- CI candidate ZIP SHA-256 `daa2a94841fb9506e02ce761266bf00ddb4eb5820eb9460a6e63b95888021fe6`; installed DLL `2108c80d9eb58961ace5bc0a46dabac39532ccfbe0ee67460a51dcb6c4f55337`. Both native CI runs 38135586667 / 38135589088 remain successful. Unsigned candidate.
- B1 `b1-2026-10-10.rc1`, document `b1-mac-2026-10-10.rc2`, C1 `c1-2026-10-08.2`, inventory schema 1: all fixed rc2 and C1 file hashes rechecked and matched. Keep 30000/1001 fps, 900-frame / 30.03-second segments.
- Native installed helper `--auth-check` succeeded: OAuth refresh and recordings read/write/publish. This is separate from Admin browser refresh.
- Fresh Azure readback at approximately 22:40–22:41: CMS 0000186 / writer=true / Running / Succeeded / latest traffic 100%; Website 0000201 / Running / Succeeded; Asset 0000090 / HLS=true / source=true / Running / Succeeded. Images match the pinned handoff. Admin route 200 and new bundle reference present.
- User authorizes OBS testing until Tuesday 2026-10-13 06:00 UTC+8 without further meeting checks. Completed, unused test recordings should be removed directly; protected originals and evidence still needed for acceptance remain retained.

Raw evidence is retained under ignored `artifacts/b1-platform-followup-20261011/`. No credentials or signed URLs are saved in the evidence files.

## Console blocker: actual refresh response incompatible with browser parser

The Console tab loaded the new release before attempting a new draft. It was not reloaded or signed in again between attempts. No new recording/capture was created successfully.

| Event | Time UTC+8 | Result |
| --- | --- | --- |
| First `POST /api/admin/broadcasts` | 23:08:15.941 | HTTP 401; bearer expires at 23:06:06.000 |
| First `POST /api/account/v1/refresh` request | 23:08:16.204 | HTTP 200; returned token expires at 23:23:13.000 |
| Same Create action retried | 23:09:14.658 | HTTP 401; still uses bearer expiring at 23:06:06.000 |
| Second refresh request | 23:09:14.724 | HTTP 200; returned token expires at 23:24:12.000 |

The two Create bodies are byte-identical and reuse operation key `ba8e8aaf-3539-4fea-a612-e2c8a57ead95`; body SHA-256 `eacdebcd1cec13d85e784f74403a3b23b484051407dc93e6df9d3cd7ce1953c2`. All three recordings permissions are present in the examined bearer/refresh claims. Only expiry and scope facts were retained, never bearer values.

Both actual refresh JSON responses have exactly one field, `access_token`; `expires_in` is absent. The loaded public `locale-context-B6FtOkvp.js` contains the Account browser client parser:

```js
if (!be(e) || typeof e.access_token != `string` || typeof e.expires_in != `number`)
  throw new M(200, `INVALID_RESPONSE`);
return { accessToken: e.access_token, expiresIn: e.expires_in };
```

Its `refreshAccessToken()` invokes that parser on `/refresh`; the real AuthProvider instantiates this client with `le({baseUrl:t.accountApiBaseUrl})`. Therefore the captured valid token response is rejected before it can replace the expired token. This identifies an additional browser response/parser compatibility blocker, rather than assuming the earlier tokenRef patch failed. The UI reports “操作未完成，請重試同一操作。” The response body/error code for the server's 401 was unavailable; do not label an inferred server error code as observed. `INVALID_RESPONSE` is the observed loaded client's deterministic rejection of the captured response, not an HTTP server error code.

Loaded Admin main bundle `index--WRVQnAy.js` SHA-256 `ed193a2ae0192d032404b38b91786fe654a51a065ab74758c54ca241238e5f97` equals the separately fetched public bundle. Loaded auth-context SHA-256 `214602ebf730b88cb35b06440996dc1ea25fae4d6808fc59dfb85ab70cd055d5`; its public release marker is `b1971334933db8400c4988a43f2d4919a268f245`. Admin has no controlling Service Worker. The handoff's `06cd79…` hash describes the HTML index, not this JS bundle; these are different artifacts.

Two expired-token Create attempts are not two completed token-expiry cycles with preview/End. The required new-field refresh/preview/End gate remains untested and blocked.

## Browser VOD results on Website 0000201

Protected recording `e496a04e-4e2e-4dad-9269-63a610dd7c06`, capture `bcccb147fbd332d73a9bae51097fa633` was read only. Its media was generated by product `9cbeec04d79a09eee89c37f0a92460b80708a805`; these browser checks do not qualify a new capture encoded by c2166da. Server readback remains ready / auto-published / 991 verified objects / recording version 3. Its old replay scope is expired. This was a fresh VOD page, not a test of old-scope live-to-VOD continuation.

Windows Chromium viewport was 1280×720. Actions used actual player controls and keyboard seek; CDP collected media facts and non-mutating frame observations. There was no programmatic play or forced quality assignment. The normal screenshot command timed out; a decoded video frame was retained separately, not represented as a full UI screenshot.

Passed scoped checks:

- 480p final five seconds played naturally to `ended` at **22:58:42.997**, with the post-end sample at **22:59:03.891**: 854×480, 799 decoded frames, zero dropped, `MediaError=null`. The natural EOS duration is 9519.539999 seconds; restored whole-asset duration is 9519.510000000006–7. The roughly 0.03-second EOS rounding does not truncate the asset.
- Home after EOS → manual 1080p: 22:59:42.096 sample, 1920×1080, position 0, paused, full duration. Auto at 23:00:15.534 also decodes 1920×1080 and retains position/pause/duration.
- Paused 100-second / 1.5× rapid switches: clicks at 23:02:17.120 (480p), :17.988 (720p), :18.861 (1080p), :19.735 (Auto). Final decoded frame at :21.487: **4.367 seconds** from first click, **1.752 seconds** from last. Position 100, paused=true, rate=1.5, full duration, no MediaError. All 27 sampled zero-width video states during this burst have a visible non-black retained canvas. This is one scoped measurement, not a rewrite of the earlier 18.491-second result or a universal performance guarantee.
- Individual manual 480p / 720p / 1080p and Auto recover actual decoded resolutions during 1.5× playback; active playback position and rate are preserved after each rebuild. Samples taken while loading (width=0 / temporary rate=1) are kept separately and are not called failures or decoded passes.
- Paused matrix at position 294.397454, rate=1.5: 23:05:45.534 854×480; :47.032 1280×720; :49.047 1920×1080; :51.055 Auto 1920×1080. Identical position/pause/rate and full duration after each decoded recovery.

Controlled VOD ABR, separate from live reconnect:

- Tab-only network throttling: 135416 bytes/s down/up (about 1.083 Mbps), latency 80 ms. No OS network outage, Service Worker bypass, cache disabling, concurrent OBS encoding or YouTube load.
- Starting from decoded 1080p/Auto and seeking to uncached position 4765.5 at 23:06:44.063, actual 480p resumes at 23:07:16.793. **Initial wait 32.730 seconds** is a limitation; this does not pass smooth weak-network seek/startup. Initial rate was 1.5×, which exceeded the available rate; subsequent waits are retained in the event log.
- Rate deliberately restored to 1× at 23:08:00.664; playback resumes at 23:08:02.382. By 23:10:12.278, actual 480p / 4934.633972 seconds / 5066 frames, no further waiting event in this interval, error=null.
- Throttling removed at 23:10:36.553. At 23:12:32.172 still 480p; first later sample at 23:14:21.477 confirms 1080p / position 5183.832651 / 12533 frames, error=null. Actual transition time was not sampled; **224.924 seconds is an upper-bound observation time after restore, not an exact switch timestamp**.
- Final 23:18:55.151: 1920×1080 / position 5457.506604 / duration 9519.510000000007 / 20737 frames / zero drops / rate=1 / error=null; buffered [5169.164,5585.579999], seekable [0,9519.510000000007]. From 23:08:02.382 to this sample, media advances about 652.711 seconds over 652.769 wall seconds with no new waiting event. This is about 10.9 minutes of VOD, not 2.5-hour uninterrupted viewing.
- No native media error event was captured during this browser session. The later 100-ms UI error monitor reported no matching error text. Earlier rapid-switch UI observations also showed none; do not claim this samples every possible error presentation.

Network override and probe timers were removed, video left paused. Decoded-frame JPEG SHA-256 `9ec1de9fb2d3192603f449d27952e8f9a618547c17faddd37485b1e1edd34ca0` (a 640×360 evidence image drawn from the actual 1920×1080 decoder; not a downloaded remote video).

## Native control limitation and untested gates

The Windows automation launcher initially resolved the explicit portable executable to the original Program Files OBS. The path mismatch was detected before recording/streaming; that owned idle process was terminated. The portable executable was then started by exact path. Hidden launch could not expose a targetable window, and the owned idle process was terminated before interactive launch. These induced unclean-start warnings are not reported as spontaneous plugin crashes.

Interactive portable OBS PID 39932 stopped at its normal/safe-mode startup prompt. Window capture works, but activation/input fails (`failed to activate captured window`; one AX input returned `coordinate input geometry is unavailable`). Rebinding and resetting the native tool did not restore input. User assistance to choose normal mode / foreground OBS was requested. No HHC capture, original recording or YouTube output was started. At 23:18:02, all nine original OBS profile/scene files still match their pre-test hashes.

### Windows closeout — 2026-10-12 UTC+8

After the user selected normal mode, the exact portable OBS process entered normal startup at **2026-10-11 23:45:56.119**, logged compiled product c2166da at **23:45:57.820**, completed startup at **23:45:59.398**, and made the native dock ready at **23:45:59.854**. Fresh UI inspection on October 12 confirms HHC-Fixture portable profile/scene, 29.97 / 29.97 FPS, audio track 1, HHC `準備收錄` / `開始收錄`, and both global controls showing Start. The synthetic clock had been stopped at 23:18:02; its displayed timestamp is stale and is not new capture timing evidence. No encoder or new platform capture was started.

Native input works after the user's assistance. The isolated OBS was closed normally; its log records `Freeing OBS context data` at **02:09:45.142** and memory-leak count **0** at **02:09:45.242**. Process readback finds no remaining OBS or upload helper. This closes the local input limitation; it does not resolve the independent Admin refresh blocker.

Fresh closeout verification: all **14/14 local CTests** pass (exit 0); both pinned product CI runs still report success at c2166da; CI ZIP and installed DLL hashes match the values above; all nine original profile/scene hashes remain unchanged. Installed helper build-info matches c2166da, and the account-bound `--auth-check --root <existing portable settings root>` exits 0 with refresh/read/write/publish success. These helper checks require the existing Qt runtime on the developer command's PATH; invoking the helper without `--root` exits 2 before authentication and is not an OAuth failure. No runtime or credential files were changed for this check.

This round created no new recording/capture and therefore no additional unused test recording to delete. Protected e496 remains retained for the current acceptance evidence. Raw observations and closeout receipt remain in the ignored evidence directory. Reporting changes do not change the installed product or qualify its GPU/long-run/end-to-end acceptance.

Still untested this release: two real expiry cycles with new-field staff preview and same-page End; live auto/custom/default cover bytes and propagation; simultaneous follower/active DVR/paused DVR/non-1× End→Stop→ready transitions; fresh live public boundaries and original-scope continuation; real OBS/original-recording/YouTube reconnect; physical outage/crash; fixed-product 2.5-hour capture and continuous watch. Preserve the historical 379/388-second and other failures.

## Copyable Mac followup

Please fix the actual formal Admin browser refresh response/parser mismatch before another fixed B1 release. Platform remains Admin b197133 / Website 0000201 / CMS 0000186 / Asset 0000090. The long-open Console's first Create at 2026-10-11 23:08:15.941 UTC+8 returns 401 using an access token expired at 23:06:06.000. Refresh at 23:08:16.204 returns 200 and a valid new token expiring 23:23:13.000, but actual JSON keys are only `["access_token"]`. The shipped `locale-context-B6FtOkvp.js` Account client requires numeric `expires_in` and throws `INVALID_RESPONSE` when missing. Retrying the same Create key/body at 23:09:14.658 still uses the old expired token and returns 401; second refresh returns 200/new valid token but still lacks `expires_in`.

Use the actual formal `/api/account/v1/refresh` response in an AuthProvider + browser Account client + CMS mutation integration test. Verify both browser session issuance and refresh formats, fresh token adoption, original key/body reuse, preview/Start/End and two real expiry cycles without reload. Fix the appropriate Account response or compatible browser parser; do not infer that HTTP 200 proves successful refresh, fabricate a fixed expiry, change B1/C1/schema/digest/fps/media rules, or alter native plugin OAuth. Native plugin refresh/read/write/publish succeeds separately.

Return a new immutable handoff with Account/Admin/common-client revisions and hashes, CI/formal response-shape evidence, actual enable flags and a Windows continuation prompt. VOD switch/EOS/retained-frame checks on Website 0000201 have scoped positive results above; weak initial seek waits 32.730 seconds and must remain a separate limitation. No new capture/recording ID exists yet. Old e496 field remains protected/read only.
