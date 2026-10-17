# K4 normal Stop targeted production retest — 2026-10-16

All event times are Asia/Taipei (UTC+8). This is one new real OBS short direct output, not a rerun of the full acceptance suite.

## Fixed inputs

- Mac handoff commit 7a999ac282ac81416aa236659a31351bbd75efd2; SHA-256 262d6d3cc24c6dd9a17a66036722fac23764e4052577c50eab600b052797079b verified against downloaded bytes.
- Installed plugin 667ee287e75ed9382624938eb2360bf3fe70eb48, isolated OBS 32.2.2. DLL SHA-256 6bc031c67ca14e7e64428a5deB467A80feef55b379a9a1598f46cf4d984d7587 (case-insensitive hex).
- Candidate ZIP SHA-256 86f5f216a1d74bd445c55c7e3a8c10198feb68fec1f44c71657e6132c93beab6. Existing two native CI checks on PR #3 re-read passing; no runtime code changes in this retest.
- Admin 2a000ce515b11958a8f98fa3f90cadf064499270, actual loaded entry /assets/index-B-2irZYE.js. Mac supplied CMS 48120b32db29d97d8023cd5e10def3214c56de3a / hhc-web-api--0000203 and Asset 7875b25f7d0ca9a135092a86278752020421033a / asset-api--0000098. Azure revisions were not independently reread on Windows.
- Existing explicit authorization until 18:30 verified against current time 17:45:34. Existing fixed key unchanged; preflight source revision32, directEnabled=true, inputBusy=false.
- Recording: 3e93b740-1915-4cad-b5b6-c379107c2b06
- Capture: 57764974a1111951f32c81b64f8a9aa5
- Output/local ID: abeb1a4f-a053-4c18-bb09-64c30fe95f3d, epoch1.
- Evidence: artifacts/k4-retest-20261016/ (local, ignored; no credentials or signed URLs).

## Executed flow

One metadata Save changed the waiting draft title to HHC OBS K4 TEST 2026-10-16 normal-stop retest. Exactly one broadcast mutation HTTP200 was observed. No custom thumbnail upload/selection was added.

All Console tabs were then closed. OBS Start around17:47:58 resolved the existing prepared direct draft above. Encoder initialization completed17:48:01.334; AAC17:48:01.362. Reopened Console tab42 selected the same recording. This proves Console-closed startup with an existing waiting draft; it does not prove creation with no existing draft. Console42/member43 subsequently remained open without reload.

Automatic Start observed live revision6, public startSequence0. No manual Start/End/publication. Durable K4 resolved intent has matching output/recording/capture/epoch/receipt. No production lost-response injection this turn.

At17:51:25.870 member video decoded1920x1080, frames632, currentTime80.822882, duration120.149333, readyState4, paused=false, rate1, MediaError=null. At17:51:25.878 staff preview decoded854x480, frames532, currentTime19.577226, readyState4, paused=false, rate1, MediaError=null. Preview initially showed unavailable before media readiness; Load Preview succeeded once live. Member playback required the normal user-gesture Play click.

Actual signal monitor973x547.3125 =16:9. Key dropdown first click expanded, second click collapsed. No key reveal/reset. Metadata single Save passed; combined metadata+thumbnail atomicity remains covered by platform tests, not a new Windows test.

Normal HHC Stop logged17:51:39.988; API accepted17:51:40.998220190. Seal accepted17:52:09.598219. No manual End, publication, Stop/seal resend or recording cleanup. OBS global stream, YouTube and ordinary recording were idle; their concurrent-load isolation is not newly proven here.

Local full decode/hash QA passed all three qualities: eight segments, seven30.03s plus8.2082s tail, total218.4182s; 30000/1001fps; aligned timelines, AAC48kHz stereo. This is separate from platform readiness. Synthetic clock stopped after output closure.

## Completion observation

Capture ready first recorded serverNow17:53:18.579992952, <=68.982s after seal (observation bound, not exact ready transition). At17:53:44.402 original Console automatically changed to processing/revision9, archive ready, public[0,8), revision4. No B1 End ACK was sent for this normal direct Stop. Original member page became ended replay without reload. Natural publication observed by17:56:19.160: archived/revision10, archive published, public[0,8)/revision5. This is <=249.562s after seal, an observation upper bound, not an exact publication timestamp. The independent recording-management page also showed Published. No manual publication occurred. Local active released=true; final native UI shows capture complete/Start, all outputs idle.

Production screenshot API timed out; no screenshot proof is claimed. Safe DOM, layout, network response and media decode evidence are retained.

## Retained limits

No rerun of scheduled, full player suite,16-minute continuation or2.5-hour capture. Historical379/388s lag, weak-network/32.730s seek and unverified meeting+YouTube load retain their previous judgments. Production lost-open-response/restart and key reveal/backfill not tested. Formal OBS unchanged; existing diagnostic recordings retained.


## Final player/UI result

Same-URL VOD passed **after one retry**, not a clean first transition. First Continue VOD click at approximately17:56:14–19 produced visible "暫時無法播放，請重試", observed17:56:30.681. MediaError remained null; video at old replay end218.453333, frames4750. Did not reload the member page. Second normal Continue VOD click around17:56:40 succeeded; underlying HTTP status/code unavailable (no captured responses from that tab), so no root cause is claimed.

At17:57:09.992 same member URL VOD decoded1920x1080, currentTime16.536297 after UI seek, duration218.453333, frames356, readyState4, paused=false, rate1, MediaError=null. Buffered[0,150.149999] and[210.21,218.418199]; seekable[0,218.453333]. Playback duration exceeds nominal source218.4182 by35.133ms, reported explicitly.

Archived original Console remained973x547.3125 (16:9); both rendered thumbnail images loaded1280x720. Live library selected tab shows this exact recording as 回放. Same watch URL retained throughout. Thumbnail bytes equality was not measured or claimed.

| Item | Result |
|---|---|
| Console-closed start using existing direct waiting draft; reopen same event | Pass |
| Auto Start, normal HHC Stop, automatic End/public[0,8), seal, ready, natural publish | Pass |
| Actual staff/member live decode | Pass |
| Real-signal and archived monitor16:9; menu second-click collapse; one metadata Save | Pass |
| Live replay classification, same URL VOD decoding | Pass after one player retry |
| First ended-replay to VOD transition | Failed once, recovered with second Continue VOD click; platform follow-up needed |
| No-existing-draft startup, production lost response, concurrent real meeting/YouTube load | Not tested |

Original Console/member retained for diagnostics. No new runtime fix, merge, installation or platform deployment. Report-only changes do not create new binary CI evidence.
