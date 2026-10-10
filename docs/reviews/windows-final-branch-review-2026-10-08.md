# Windows candidate whole-branch review and fix pass

One fresh, read-only reviewer examined base `7d2b1e9a7fe993ceb4b97f33f2df5b2c71c75435` through `7fdd47581ceb1bcab20d022d27db81110339905a`, including production source, relevant tests, packaging, CI and the accepted plans. It found no Critical issues and two Important issues. No merge or operational release was recommended. This report records the single subsequent focused fix pass; it is not a second reviewer approval.

## Findings and disposition

1. **P2 — Terminal failure prevents another event.** After source/NVENC failure or server abort, `Phase::Failed` disabled the action. Reauthentication, refresh and terminal recovery did not restore editable Ready. Fixed by exposing **prepare another event** only after authoritative failed/expired/aborted state and encoder completion. The first click returns to Ready, releases the completed encoder, clears the old display warning, and turns live/publication off; it does not create an event or change the failed session. Unresolved synchronization errors still require recovery. Original IDs, media, intent and operation keys remain available.
2. **P2 — Staging copies underbudget disk preflight.** Closed fMP4 segments were copied into the queue while their staging copies remained; the accepted 10 GB + 2 GiB reserve could stop near 5 GB of media. Fixed with a Windows same-volume `MoveFileExW` rename after hashing and closing the source, without overwrite or copy fallback. Final validation checks canonical queue files after committing the final fragment. Tiny init/playlist staging files remain for the muxer's lifecycle; historical staging media is preserved.

## Verification

The checks below exercised the fix working tree before its source commit; JSON records retain the earlier HEAD plus `dirty=true` or explicit working-tree scope. Frozen candidate/source-stamp and hosted CI evidence follow separately. Never attribute these runs to the unchanged earlier HEAD.

| Scope | Result and evidence |
| --- | --- |
| RED, actual local OBS | A fresh 61-second event retained 9 staging segments; regression exit 23. `artifacts/review-storage-obs-red/storage-result.json` |
| RED, actual native production readback | Original aborted capture recovered, but the replacement preparation check returned false, exit 1. No new capture started. `artifacts/terminal-prepare-red-2/native/prepare-another.json` |
| GREEN, local build and unit/mock/loopback | Native build exit 0; all 10 CTest suites passed in 2.44 seconds. Dock checks cover unresolved failure, settled terminal failure, disconnected account and encoder stopping. `artifacts/review-fixes-final-{build,ctest}.log` |
| GREEN, actual local OBS media | OBS exit 0, 9 canonical segments and 0 staging segment copies. Three identical 60.9609-second timelines, 30.03/30.03/0.9009 segments, 30000/1001 fps; all object hashes, stream formats and full decodes passed. `artifacts/review-storage-obs-green/{storage-result.json,media-verification.log}` |
| GREEN, native platform terminal handling | Original capture returned aborted with stop accepted. The real dock returned to editable Ready with live/publication off; started=false, no replacement event. `artifacts/terminal-prepare-green/native/prepare-another.json` |
| GREEN, actual Windows sharing denial | Denied next segment is not receipted; all 6 prior object hashes retained, three retained renditions fully decoded; EncoderFailure, normalEnd/ready/seal false, OBS exit 0. `artifacts/review-storage-sharing-green/{result,retained-media-verified}.json` |
| GREEN, actual NTFS ACL denial | Actual UnauthorizedAccess confirmed; the same retained-media checks passed, original ACL restored exactly, OBS exit 0. `artifacts/review-storage-acl-green/{result,retained-media-verified}.json` |
| Fixed contract | Fresh GitHub C1 manifest and all 11 immutable artifacts match at 2026-10-08T14:54:48.845333Z. `artifacts/review-c1-recheck.json` |
| Original preservation | All 31 local objects match their hashes; original inventory and stored seal body/key match exactly. `artifacts/review-original-preserved.json` |

The standalone libobs harness could not load installed NVENC/AAC modules in three preliminary launches, each exit 6. These are retained in `artifacts/review-storage-red*.log`, not counted as a regression pass or physical GPU failure. A preliminary native fixture launch used an already-existing destination and therefore did not start its test; its idle owned process was terminated. The corrected native launch produced the RED result above. Actual OBS frontend runs provide the stated storage and GUI evidence.

## End-to-end status

C1 acknowledgement remains `c1-2026-10-08.2`, inventory schema 1, OpenAPI 3.1.0, CMS 0.2.0 / Asset 1.0.0. Wire fields, digest and idempotency rules were not changed. Production revision details remain the platform owner's reported deployment; Windows did not deploy it.

Original capture `5d68171bc2e63cd5f9cae36eef7032b7`, recording `28a3c057-4885-4036-ac1c-0437c1495206`, is aborted because the recording was previously explicitly deleted (`recording_deleted`). It remains unexpired with stop accepted, 31 declared objects / 27 verified / 4 queued. Its 8 segments per rendition, 240.206633 seconds, 30000/1001 fps, digest `f50005b13e8f25e585424f8d93c8428a7da50f39dcd7ff57058e120c75b3dcef` and operation key `439db0a4-560d-4156-ac00-aeafad6742a7.seal` are preserved. No seal retry was issued against a terminal capture; freezing/ready/publication are not passed outcomes for it.

Fresh positive recording/live/upload/stop/seal/ready/publication, member-player quality/seek, controlled outage recovery and current 2.5-hour production acceptance remain open. Exact cleanup permission for test12 recording `5edca3a5-c1e0-48f1-9465-e50a345cc952` is pending; earlier cleanup permissions do not cover it. Ordinary OBS crash-dialog display/keyboard acceptance and a synthetic YouTube concurrency run require their identified permissions. Physical disk/power/GPU failure and Safari evidence remain separate. See [continuation gates](../windows-integration-next-gates.md).

Historical ready readback, hosted CI and local media checks cannot substitute for those end-to-end results. PR #1 stays draft; no merge, deployment, release, production profile change or YouTube operation occurred.
