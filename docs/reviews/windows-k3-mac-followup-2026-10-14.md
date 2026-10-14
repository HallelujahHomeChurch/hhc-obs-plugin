# K3 targeted platform follow-up

Please investigate these two production observations before Windows calls K3 end-to-end complete. Do not manually publish, recreate, cancel or delete these fields to mask the evidence. Contract/media rules remain unchanged.

Native field plugin: `686dcfecba1a4e5cfc40995d40f219e32ff83964`. The follow-up `33be386aed287098f23d8421acd0697887cf2c05` only corrects misleading publication text; local tests and both CI runs passed. Full evidence: `windows-k3-acceptance-2026-10-14.md` in this directory.

## Automatic publication remains unconfirmed

1. Recording `b7a8d3f4-54cc-4a69-88b6-ccda0d8b7136`, capture `63163a8265f4f2cc31f97bfddec93665`.
   Stop 2026-10-14 10:56:44.095812277 UTC+8; seal 10:57:51.836961; ready observed 10:59:46.401706689. Public `[0,14)`.
   At 11:14:51.653 the original Console GET reports phase processing, archive state ready, reason null, policy autoPublish true, auto cover ready, cover ID `677a2722adf24547d32ebbae1e4c0552`. Admin still draft at 11:17. No manual cancel/publish was sent.
2. Recording `21edf722-0533-4081-a416-580ab69517f6`, capture `fd6df71873a5d2a4ac0384a155611ea5`.
   Stop 11:11:17.152477929; seal 11:12:21.308683; ready observed 11:14:02.457813092. Public `[9,16)` = 210.21 seconds, policy autoPublish true; draft at 11:17.

Please correlate publication/thumbnail/archive reconciler logs and permissions, report exact ready/cover/published times and any blocked reason. C1 `autoPublish=cancelled` is the known B1 bridge value and is **not** itself a new platform failure. Both fields uploaded and sealed successfully; do not ask Windows to re-upload unchanged material.

## Ended-live replay transient false media errors

On the second field, retain original member page without reload. Natural EOS decoded 480p at 11:15:14.108 UTC+8, currentTime=duration=210.241999, 6304 frames, 13 dropped, MediaError null.

Home→manual 1080p temporarily displays **無法播放媒體** (11:16:01.199, 0×0 / no duration), then recovers without reload by 11:16:12.975 to 1920×1080, time 0, duration 210.21, paused, 1x, MediaError null. Selecting Auto repeats the false error; later it recovers at the same position/pause/rate. This is within the displayed replay deadline 11:50, not published-VOD evidence. Investigate source-rebuild/loading error display on ended-live replay. Do not classify it as an unrecoverable decode failure or claim exact switch latency from these sparse samples.

Return fixed commit/deployment/contract evidence and a targeted Windows retest prompt after diagnosis or deployment. Do not broaden wire schema, loosen validation, or claim the older recovery/seek failures resolved. Windows still owes actual expired-session disconnect, revocation, workload/ABR/DVR, pressure and 2.5-hour tests separately; these short fields do not replace them.
