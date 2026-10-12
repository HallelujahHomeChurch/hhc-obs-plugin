# Windows B1 browser refresh retest — 2026-10-12

In progress; no new capture yet. Times are Asia/Taipei UTC+8 unless explicitly UTC. Historical failures and protected recording e496a04e-4e2e-4dad-9269-63a610dd7c06 remain unchanged.

## Fixed inputs and actual deployment

- Product remains c2166da44da0d2fccbd4f84d82aa54a2fa03ad55; product ZIP SHA-256 daa2a94841fb9506e02ce761266bf00ddb4eb5820eb9460a6e63b95888021fe6. Both product native CI runs pass. Reporting head 671140dc178f4d797e3444eafbbd0f2a4e6aaa6c also now passes both native runs 38162785272 / 38162781423. These are separate from new formal OBS acceptance.
- Existing user authorization remains valid until 2026-10-13 06:00 UTC+8; current clock at preflight is 2026-10-12 10:10:06 UTC+8. No renewed meeting permission is required in that window.
- Account source 9071a8100b00969cd96a108029213420cd6d483e; actual account-api--0000161 / ready / Running / Succeeded; image 3db86e6f2ee8e5a52b323de30a5f3ec9a38f0b53f8ce04a360aa7aca67cb3216.
- Actual CMS hhc-web-api--0000187 / ready / Running / Succeeded / latest traffic 100%; BROADCAST_WRITERS_ENABLED=true. Asset remains asset-api--0000090 with HLS/source enabled.
- Admin's actual loaded index-BxpptPsz.js SHA-256 14019c5db6242cb4aa159fd49c8b389e34cb0ff6bf3004825459644d8d5e63a0; actual loaded locale-context-CM9Oh0Jd.js SHA-256 f24b51f127464bd6789cb0e63bbf475e6bdfe6b236c7260bf19b109f546d093b. Both public release markers identify 34b5073f6d821fde400a4a1a1de583861be58f0f.
- Website readback is hhc-web--0000203, not the handoff's 0000201. Actual image 24bda02e3e5fb4c35ce9f7d6d035105b2a1d88ea9053ef48be3989d43a68016b maps to completed successful Production Release run 38167505385, source beeba54c6462639844e1fa17b6b32cc184f89349. The intervening changed files concern the weekly reader and shared translations; media-player files are unchanged. This does not turn the 0000201 browser evidence into 0000203 acceptance.
- B1 b1-2026-10-10.rc1 / document b1-mac-2026-10-10.rc2; C1 c1-2026-10-08.2 / inventory 1; 30000/1001 fps and 900-frame / 30.03-second segments remain unchanged.

The immutable Account contract document, OpenAPI and sanitized fixture bytes were fetched and independently matched SHA-256 92e1fbaa6b2215073fee966f46d012d621923f44b744bd22d9faf00d8c635884, 5838a02ae93a514d0b8020d622d8419024107e23793941369bab9043e7bc2aa2 and 2131de854ba3452fe4e1ebaa669338455a390325db99da7550f00e9ee6494752 respectively. The fixture's 137-second expiry is synthetic and is never used as the production expiry or uploaded media.

## Console expiry and native preparation

A fresh Admin page is opened once, then retained without reload or login. Its initial observed bearer was issued at 10:10:47.000 and expires at 10:25:47.000. The real GET live-cover-settings at 10:12:26.172 succeeds with all three recording scopes. Auto cover is currently the shared default; it is inspected without mutation. One Create form is prepared for `[HHC OBS SYNTHETIC TEST] B1 refresh-c2166da 2026-10-12`, manual Start / manual End / automatic VOD publication. Create has not yet been submitted. Actual expiry, refresh response adoption, original-key/body retries and preview/Start/End remain pending.

Isolated portable OBS starts idle, with the fixed product and 29.97 fps. Native window activation is again rejected by the automation tool while another application is foreground; the user was asked only to foreground the isolated OBS. No HHC/global output has been started. Existing independent HHC upload processes are not stopped or altered; any concurrent uplink load must be reported separately from an isolated normal-load pass.

### First actual expiry cycle: background renewal succeeds

Create at **10:26:10.197** carries the initial bearer nominally expired at 10:25:47 and returns **201**. This records the server accepting this particular request 23.197 seconds after the claim's expiry, not a measured rejection or a proven configured grace period. It must not be counted as Create 401/retry evidence. Exactly one draft exists: recording/broadcast **cb129c48-28ad-41bb-86e8-57250709e128**. Create operation key **1b91d3a3-de99-46a1-8c8f-ef84ca4b9e0a**, exact body SHA-256 **0cb62743cdc32befdb20c96d5f65566154b0685333c3cb5ea8ddfc8839c75331**; no retry is observed for that mutation.

The same page's background detail GET at **10:26:51.616** carries the initial expired bearer and returns **401**. Refresh POST at **10:26:51.688** returns **200** with exactly `[access_token, expires_in]`, numeric **expires_in=900**, issuer claims issued10:26:48 / expiry10:41:48 / exp-minus-iat900. A parallel detail GET at10:26:51.755 also carries the old bearer and returns401. Subsequent detail GETs at **10:26:56.909** and .925 carry the newly issued bearer and return **200**, without reload/login. Safe response observation timestamps are sampling times; request wall times establish the event order above.

This passes the first scoped real-expiry background renewal/adoption check and no longer reproduces the old token-only parser blocker. Two actual expiry cycles, mutation original-key retry, true preview decode and same-page Start/End remain incomplete. No new capture or automatic publication is claimed.

### Second actual expiry cycle: announcement retry succeeds

The same Console page remains open without reload or login. At **10:42:54.160**, its actual UI announcement POST carries the bearer expired at10:41:48 and returns **401**. Refresh at **10:42:54.229** returns **200** with both response fields, numeric expires_in900 and issuer exp-minus-iat900. New issuer claims are issued10:42:50 / expiry10:57:50. The original announcement automatically retries at **10:42:54.350**, now with the new bearer, and returns **200**.

Both actual announcement bodies reuse operation key **a155bb2f-4c63-42c0-9f45-3fb745972a32**, exact byte SHA-256 **66fb4a29387ba6527d8ccd836e9cc13d2eaeca51c00e010ae86ad904d7c5f95b** and keys `[operationKey, expectedRevision]`. This is a real expired-token mutation/retry, not an injected401, fabricated expiry or altered browser parser. No offline/throttle/cookie override or token replacement was applied. A short wall-clock wait only times the ordinary UI click; it does not modify credentials or the server's result.

The actual returned broadcast is revision **2**, phase **scheduled**, announced=true, binding=null, public start/end=null, policy manual Start / manual End / autoPublish=true. A fresh member page50 at the same recording URL displays the correct test title and `等待直播開始`; that original page is retained for the future Start→VOD test. Announcement is not a live decode, staff preview, capture or publication pass. Admin screenshot acquisition returns a Page.captureScreenshot timeout; actual DOM snapshots are retained, with no fabricated screenshot.

The first background renewal and second announcement renewal together prove **two real nominal token-expiry cycles with actual401→refresh200→new-bearer200 on the same Console page**. The second additionally proves original mutation key/body reuse. The old browser response/parser blocker is resolved for these measured paths. Create401 retry, preview decode/renewal, true Start/End ACK, media and source lifecycle acceptance remain separate pending gates.

## Remaining Windows dependency

Native window activation and its observed `Raise` action both fail with `failed to activate captured window`; an earlier AX-only click returned unavailable coordinate geometry. The isolated OBS still runs idle with no HHC/global encoder. A foreground-switch assistance request is pending; the meeting authorization itself is still valid. Existing HHC upload/verification processes are left untouched. Until native control is usable, no bind/capture/preview/Start/End/Stop/seal/ready/autopublish or current-candidate long-run can be claimed. Keep this single scheduled test field and its original member waiting page for continuation; it is not completed/disposable yet. Protected e496 and all historical failures remain unchanged.

Raw sanitized evidence is retained under ignored artifacts/b1-browser-refresh-retest-20261012. Network observers retain response field names/expiry/scopes, request paths/status/operation keys/body SHA-256; never credential values, cookies or signed media URLs.
