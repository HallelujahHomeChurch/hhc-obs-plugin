# K3 Console targeted production recheck

Time: 2026-10-15 13:21–13:30 Asia/Taipei (UTC+8). Existing no-meeting authorization through before Friday remained valid. Isolated OBS only; no global stream, YouTube or ordinary recording was started or changed.

## Version evidence

- Installed plugin remains `4f332583bbcd5ed9221dbbb03c74c025d5cc6fa9`; normal unsigned ZIP SHA-256 `e4f4fa485ce07769d9e7a03ae720ba26ab64618f970b933538939e8f6c7ca361`. This report commit is not an installed binary update.
- Admin `2bb275fe78060cc6a68f87f6e1d4c7d111755cd0`; release38685008923 successful, actual `/assets/index-DY63hRwd.js` contains that commit and SHA-256 is `0a31ad848153a1b7203e69235c1ee9b0277dd2e1d0e30f58742f1ed7f4f1e8ce`.
- CMS release38675334454 successful at `97c7402d443e25468d2807ed0dcc9462ee3f98a8`. Revision196, readiness/flags/digest are the Mac 13:18 deployment readback, not an independent Windows Azure read.
- Fixed followup at b31c3c43 SHA-256 verified: `785ab5b8bd4ca367284edfcd4f58a61501d1d6aa786dad8a3cba01a2024f6136`.
- K3/B1/C1 and media contract unchanged. No plugin, CLI or platform code changed.
- Browser session displays IT HHC. Authenticated read/write actions succeeded and publish controls are available; this is not a separate re-audit of foh identity/scopes and no publication was attempted.

## Console results

**Passed: deleted receiver recovery.** Fresh retained draft `2294ed31-3c2c-4f66-982a-720465bba109` shows no selected receiver and enabled Prepare. Normal UI Prepare returns HTTP200. Source revision13, target retained draft, inputBusy=false; event revision2, draft, announced=false, binding=null, autoStart=false, autoEnd=true, autoPublish=false. No capture created by preparation. Connection settings correctly names the draft, waiting signal and unannounced state. Key was not reset or retrieved.

**Passed: edit conflict and original-page reconnect.** Console A changed description locally without saving. Console B saved title ending `B edit`. A preserved the exact unsaved description and original input title, showed the content conflict, and disabled Save. A alone was placed offline using tab-scoped CDP network emulation; seven Fetch failures reported ERR_INTERNET_DISCONNECTED. After restoring normal network, the same page resumed source/event/stream-key HTTP200 without reload. Its draft and write conflict remained. No host networking or OBS networking was changed. Network override was restored.

**Passed, limited scope: two-Console CAS race.** Created one future private draft `ab11453c-27a4-4411-b33b-0b26ea8be6a5`, scheduled 18:00 UTC+8, autoStart=false/autoPublish=false. Creation alone retained the immediate draft as receiver. Two tabs of the future event clicked Prepare concurrently through normal UI: one HTTP200, one HTTP412 `broadcast_revision_conflict`; loser shows connection changed/reload prompt, with no silent mutation retry. Source advances to revision14. This is same-target competition; different-target simultaneous competition remains covered by Mac PostgreSQL/CI, not this native browser exercise.

**Passed: occupied-input UI and private preview.** Short OBS transmission bound the future draft even before its announced time (not announced and never public). Capture `65b587600695c0e5030a38d84a2f3153`, epoch1, event revision4, phase preview. Both public boundaries remain null. Immediate event Prepare disabled with explicit occupied-input explanation. Bound private page has no invalid Cancel action and instructs normal HHC stop. No forced API request bypassed the disabled control; server409 busy was not separately induced.

**Passed: release and next preparation.** After accepted Stop/seal, immediate event Prepare becomes enabled and succeeds through normal UI. Final source revision15, inputBusy=false, target `2294ed31-3c2c-4f66-982a-720465bba109`. It stays unannounced and unbound. No second capture was necessary. Do not start another OBS transmission unintentionally: the retained draft is now the prepared target.

## Short private capture timeline

Local session `4ff15c2e-d82f-4f35-94ec-4a03f225e45f`.

| Milestone | UTC+8 evidence |
| --- | --- |
| HHC Start UI | 13:25:48.841 |
| NVENC/AAC initialized | 13:25:50.815–13:25:50.841 |
| Local HHC output stopped | 13:26:34.882 |
| Server Stop accepted | 13:26:34.434131846 |
| Server seal accepted | 13:27:04.150755 |
| Server ready observed | 13:27:57.478245373 |
| Console cancelled observed | by 13:29:04.853 |
| Released source / next target observed | 13:29:31.271 |

Local and server clocks are separate; the server Stop timestamp precedes the local output log by about448ms. Do not infer exact cross-clock action latency. Server seal-to-ready observation bound is53.327490373s.

Three renditions each have2 segments, duration43.810433s, 30000/1001fps. Inventory digest `f25c27b7ce4c68cca5cc3af0c8c7c9a38e65822cbeffcff028a7fa57fb051971`. Local `scripts/verify-media.py ... --seconds 44` passed every object hash, complete decode, codec and aligned frame-grid checks. Source is the existing static synthetic text fixture (its displayed historical timestamp/version is not the current binary identity). AAC track1 is present; no audible-source mix test is claimed.

Journal: normalEnd=true, stopIntent=true, sealAcknowledged=true, confirmedReady=true; active input released=true. Real platform state ready was checked, not inferred from exit0. Final event revision5 cancelled, archive none/reason null, start/end null. Recording manager shows draft, validation passed, member invisible, auto-publication cancelled. This is correct never-public behavior, not a failed publication.

## Publication and remaining coverage

- Existing long reference `a57859d4-364e-4f24-9589-209294de5d18` read-only: Console archived/recording available and recording manager published/validation passed, with no stale blocker message. No media replay or reupload.
- Blocked/retrying/disabled archive reasons were not artificially induced through permission changes or manual publication. Their complete production-state matrix remains unverified. The never-public cancelled case is verified separately above.
- Signal-only revision with a concurrently dirty form was not separately exercised; content-conflict preservation and transport recovery passed.
- No new Console End isolation test: this never-public field used normal HHC Stop. Prior independent local-recording evidence remains historical; YouTube load is still untested.
- No new 16-minute outage, player, long-field or strict recovery benchmark. Historical379/388s lag,32.730s weak seek,122.098s strict threshold failure and actual meeting+YouTube untested remain unchanged.
- Retained original draft, new cancelled diagnostic, all protected originals and local evidence remain. No deletion, key reset, manual publish, merge or deployment.

Evidence: ignored `artifacts/k3-console-20261015/acceptance-progress.json`, native `obs-stopped.png`, and original local queue/journals/media. Browser screenshots remain unavailable due capture timeout; AX and sanitized network evidence retained. Four Console tabs retained for follow-up, including A's unsaved conflict draft.
