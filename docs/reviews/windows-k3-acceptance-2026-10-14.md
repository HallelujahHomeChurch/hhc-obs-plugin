# Windows K3 acceptance — 2026-10-14

Status: in progress; not a full end-to-end pass. All event times below use Asia/Taipei (UTC+8).

## Fixed candidate and contract

- Installed plugin: `686dcfecba1a4e5cfc40995d40f219e32ff83964`.
- Unsigned ZIP SHA-256: `3f969109f0b0c712ab2b8deeca5e5b77c94d7bd327062a0d004ef32de267a242`.
- Native CI runs `38503008028` and `38503003598`: success. Local Windows build and 16/16 CTest tests passed.
- K3 `k3-2026-10-14.rc1`, B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory 1. Supplied fixed handoff/contract/OpenAPI/fixtures and A–D report hashes verified before implementation.
- Deployment identifiers are the Mac handoff snapshot, not an independent Azure readback: CMS 0000193, Gateway 0000206, Asset 0000092, Website 0000206; Admin `386099c507642ecf4a552e2ae2664d6dc04f92c1`.
- User confirmed creation/storage of the persistent stream key. Console created it; OBS saved it through its password field to Windows Credential Manager. Clipboard cleared. No key/session values belong in this report or journals.

## Bootstrap and first native short field

The no-prepared-event wait was stopped before preparing a target. Its active pointer released and no bind intent appeared later. A subsequent explicit Start created a new intent.

- Recording: `b7a8d3f4-54cc-4a69-88b6-ccda0d8b7136`.
- Capture: `63163a8265f4f2cc31f97bfddec93665`, epoch 1.
- Local intent: `146c9d85-7f35-46f7-bff4-169bd74c1aa8`.
- Initial candidate rejected a valid B1 `accepted` bind receipt. Fixed in `686dcfe` with regression coverage; retried the original intent and capture, not a replacement.
- Source start: 10:49:42.5 (OBS log).
- Original Console preview decoded at 10:56:02.751: 854×480, currentTime 15.020139, playing, readyState 4. Initial preview attempts before availability showed a retry message; not counted as uninterrupted preview success.
- Auto thumbnail appeared on the original non-reloaded Console; natural size 1280×720.
- Console End ACK: boundary 14, accepted; public scope `[0,14)`.
- Stop accepted: 10:56:44.095812277.
- Seal accepted: 10:57:51.836961; local input released only after receipts.
- Capture ready observed at serverNow 10:59:46.401706689. This is an observation upper bound, approximately 114.565 seconds after seal, not an exact worker completion timestamp.
- Local full decode/hash/codec/timeline check passed for all three renditions: 15 segments, 420.787033 seconds, 30000/1001 fps. The last source segment lies outside the public end boundary; do not use full source duration as public duration.
- Auto publication remains unconfirmed: recording was still a draft at 11:03; original Console response at 11:09:26.615 has phase `processing`, archive `ready`, reason null, policy autoPublish true. No manual publish or cancel was issued. C1 `autoPublish=cancelled` is the existing B1 bridge behavior, not proof of cancellation. Corrected the K3 dock display to defer publication status to Console, matching existing legacy B1 handling; actual installed field binary remains `686dcfe`.

Qualification: the isolated OBS still loaded an old `hhc-obs-fixture.dll`, producing a duplicate `hhc_hls_output` registration message. This short field proves K3 transport/control behavior but does not establish exact-candidate encoder acceptance. After seal and graceful OBS exit, moved that DLL to the evidence backup directory. Restart log at 10:58:26 identifies `686dcfe`, loads only `hhc-obs-plugin.dll`, and contains no duplicate registration. Original installed OBS output settings and YouTube were not changed.

## Second field: same saved key, clean candidate

- Recording: `21edf722-0533-4081-a416-580ab69517f6`.
- Capture: `fd6df71873a5d2a4ac0384a155611ea5`, epoch 1.
- Local intent: `8ef054b3-d23c-4391-a893-30b1aafff4eb`.
- Scheduled metadata: 11:30; autoStart false, autoEnd and autoPublish true. Announced and prepared in Console.
- Native source started 11:03:07.818–11:03:07.960. Existing stored key reused after OBS restart; no OAuth, event picker or metadata entry in OBS.
- Three NVENC encoders and audio track 1 initialized. Original Console retained for preview/Start/End and A–C checks. Acceptance still running.
- Private preview initially returned HTTP 409 `broadcast_state_conflict`; after verified media became available, retried on the same page and decoded 854×480, playing, currentTime 14.43689. This is availability-gated retry evidence, not zero-error startup.
- Start acknowledged at encoder boundary 9 before the scheduled 11:30 time. Member original page initially showed the countdown, then automatically became a player without reload. At 11:09:26 the UI reported 109 seconds estimated latency; this badge alone does not establish decode or recovery success.
- Isolated local OBS recording started around 11:08:19 for independent-output End testing. YouTube remains untouched.

## Evidence and remaining scope

Ignored local evidence: `artifacts/k3-20261014/`, including fixed documents, builds/CTest, original profile hashes, candidate ZIP, first-status.json and first-local-validation.json. Durable intents and queue remain in the isolated OBS plugin configuration. Do not expose credentials or signed media URLs when sampling.

Not yet passed: clean candidate Start/End/VOD auto-publication, A–C complete lifecycle, expired-session real disconnection/reconnect, revocation cleanup, member authorization/ABR/DVR, normal workload recovery, stress test and current-version 2.5-hour long run. No new automation was created. Previous 379/388-second lag, 32.730-second weak-network seek and prior long-worker retry evidence remain unchanged.
