# K4 short timing evidence — 2026-10-17

All times below are Asia/Taipei UTC+8. No runtime code, wire, key, encoder or validation-rule change. One new synthetic short direct output only; no long-suite rerun.

## Versions and IDs

- Installed isolated OBS32.2.2 plugin667ee287e75ed9382624938eb2360bf3fe70eb48; DLL SHA2566bc031c67ca14e7e64428a5deb467a80feef55b379a9a1598f46cf4d984d7587 rechecked. Candidate ZIP86f5f216a1d74bd445c55c7e3a8c10198feb68fec1f44c71657e6132c93beab6 unchanged.
- User supplied CMSd04f7b7fdd2d3f6d0de8a61a52a7fd5434d86c56 / hhc-web-api--0000207; Asset37cba2f0aa7b1c8420e53cdece47a8033c4a4875 / asset-api--0000099. No independent Azure revision query; authenticated API observations below are direct Windows evidence.
- Recording4e3a9939-0e60-4cfb-a1c3-71993324abb7
- Capture33890e9b63b1b860da1ae5fbfee78ede
- Output/localIde16c1645-2b17-4a60-96d6-317bd585ffb5, epoch1.
- Preflight source revision44, inputBusy=false, directEnabled=true. User confirmed no meeting and explicitly requested direct isolated OBS launch. Initial native helper returned stale window; reset JS connection and launch exact portable executable resolved it. Formal OBS also open/idle, untouched by test controls.
- Local evidence: artifacts/k4-timing-20261017/. Initial blocked preflight file remains historical; later successful launch supersedes it.

## Timeline

| Event | Time | Evidence / meaning |
|---|---|---|
| Encoder initialization |01:52:30.848; AAC01:52:30.873|OBS log; source actually encoded|
| Public start |01:55:11.851272|recording API broadcastStartedAt|
| Member live decode |01:55:53.265|1920x1080,frames641,currentTime111.297241,duration120.149333,paused=false,rate1,readyState4,MediaError=null|
| Normal HHC Stop accepted |01:55:45.481045392|durable Stop receipt; no manual Console End|
| Seal accepted |01:56:09.954308|durable seal receipt; no manual resend|
| Still live/public end null |01:56:43.700|original Console API observation,revision6|
| Package attempt1 began |01:56:40.867709|processingProgress attempt1,phase package_validation,attemptStartedAt/phaseStartedAt; observed01:57:22.409|
| Ready |01:57:16.564643|recording API readyAt; capture journal ready at serverNow01:57:18.322817368|
| Auto End/public range observed |01:57:38.408|processing/revision8,public[0,7)/boundary revision4; original Console never reloaded after start|

Seal-to-ready66.610335s. Attempt-start-to-ready35.696934s is a timestamp difference, **not claimed as worker attempt execution duration**: finish/result timestamp and exact public End timestamp were not exposed in the observed frozen DTO. Ask Mac to correlate server logs/database with these IDs and01:52–02:05 UTC+8 window. Do not infer attempt count beyond observed attempt1 or equate worker exit success with ready.

Source media: seven segments per quality, six30.03s plus14.481133s tail,total194.661133s,30000/1001fps. Local full decode,hashes,three-quality timeline and audio/video parameter QA passed. Separate from platform ready evidence.

## Final state

At02:02:16.749 recording remains draft/version3, capture autoPublish pending. This was an intermediate observation; publication subsequently succeeded below. Original Console route transitioned to recording management after auto End (observed without an agent navigation action); a newly opened event route also redirects to recording management. Member original page remains ended replay. Capture ready is independently proven above; it does not establish publication.

## Scope and retained limits

No manual End/publication, original Stop/seal resends, recording deletion or key reset. HHC uses isolated fixture; no meeting/YouTube/ordinary recording load test. Previous first Continue VOD failure is retained regardless of this timing-only platform update. Historical379/388s recovery lag, weak-network/32.730s seek and other untested load remain unchanged. Existing two report-head native checks from previous turn re-read passing; this report does not replace installed-binary CI evidence.


Final local state: active released=true, synthetic clock stopped. No new capture or reupload scheduled. Natural publication was pending at that observation and subsequently succeeded below. No publication SLA was invented. At last observation it was300.184s after ready /366.795s after seal. Platform must correlate exact End,attempt finish and publish-reconciler results. Browser screenshot API timed out; DOM/API/media evidence retained.


## Completed publication / VOD observation

Authenticated recording GET remained draft/version3 at02:03:13.555 and first returned published/version4 at02:03:18.975 (HTTP response timestamps; publication lies within this5.420s observation window, not claimed as exact server publishAt). Capture autoPublish also published. Thus seal-to-first-published-observation429.020692s and ready-to-first-published-observation362.410357s. No manual publish or retry mutation. Mac should correlate publisher/cover/reconciler timing, not classify the intermediate pending state as permanent failure.

Original member page offered Continue VOD. First click succeeded in this run, no visible error and no recorded HTTP>=400/loadingFailed during transition. No reload. At02:04:29.255 same URL VOD decoded1920x1080,currentTime14.422108,duration194.687999,frames289,readyState4,paused=false,rate1,MediaError=null; buffered[0,150.149999],[180.18,194.661132],seekable[0,194.687999]. The26.866ms difference from nominal194.661133s is recorded. This single successful transition does not rewrite the previous failed first transition or prove a player fix (this release was timing-only).

Result: normal direct output/Stop/seal/automatic public End/ready/natural publish/same-URL VOD passed for this one short fixture. Exact public End and worker attempt finish/duration remain Mac server-log correlation items. Local output released, clock stopped; formal OBS not modified.
