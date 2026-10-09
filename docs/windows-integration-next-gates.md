# Current continuation status — 2026-10-10

Actual downloaded CI product `d59f1eb8035168cdc205eebcd1b42f82ecd76c0e`
passes short native stop/seal/ready/auto-publication, original hashes/full
three-rendition decode, member VOD, isolated-load recovery and completed
Auto/fixed thumbnail cases. Shared-load actual player recovery fails the
ten-minute60–120-second gate. Auto lowest-resolution live startup fails the
first-presented-frame check, despite successful measured ABR downgrade/upgrade.
Long34 completed local9000.224567-second full media validation, native
exit0, platform ready/automatic publication and native GUI recovery. Its
full published480p watch continues. Scene-switch case40 subsequently
published with all covers ready and the unchanged fixed JPEG selected;
its member VOD played from zero to natural end without a media error.
Reader replacement race fix `3462fe5` passes
all13 local tests and both hosted CI runs. Its downloaded candidate passed
manifest/source checks and native OAuth refresh; new9000-second actual-OBS
capture41 is in progress, not complete.
See [current results](reviews/windows-e2e-2026-10-10.md),
[fixed Mac recovery handoff](reviews/windows-shared-recovery-2026-10-10.md)
and [thumbnail results](reviews/windows-thumbnails-2026-10-10.md).
See [latest platform blockers](reviews/windows-platform-blockers-2026-10-10.md)
for exact capture IDs, times and the requested owner investigation.
No merge, signed release, deployment or YouTube changes. The sections below
retain historical context and do not override these current results.

## Historical continuation status — 2026-10-09

[Mutable-journal fix and long18 failure](reviews/windows-atomic-journal-2026-10-09.md): local 13-test suite and actual NVENC inventory-fault check pass; new CI/package and 9000-second native production qualification remain pending. Long18 is failed, not running or ready. Live20 short stop/seal/ready/publication and the separately requested 2375.91-second player flow passed; the ten-minute live catch-up gate failed and awaits the platform/backend/player owner. Existing evidence below is historical, not qualification of the new candidate. No merge, release, deployment or YouTube changes.

# Windows integration continuation gates — 2026-10-08

The [whole-branch review fix pass](reviews/windows-final-branch-review-2026-10-08.md) resolves two Important source findings: terminal failure can explicitly prepare another event after encoding stops, and closed segments move into the queue without a retained media copy. New local 61-second media, sharing-denial and ACL-denial checks pass; original terminal capture preservation and fixed C1 byte checks pass. These are distinct from fresh positive production acceptance. Earlier frozen candidates and their evidence remain unchanged.

Current contract: C1 c1-2026-10-08.2 from fixed handoff e5a2dc0f370ddc4a90944d09015dca9ef1ef8540. All11 immutable artifacts match byte hashes; see [acknowledgement](reviews/c1-2026-10-08.2-acknowledgement.json). The owner reports Asset asset-api--0000084 / commit52efad00367d0101e9882519bdbbd0fa6aa8acd6 and CMS hhc-web-api--0000149 deployed. This resolves the old final-seal count contract dependency; Windows still needs actual current-platform acceptance.

Original recording28a3c057-4885-4036-ac1c-0437c1495206 was previously deleted with explicit approval. Its capture5d68171bc2e63cd5f9cae36eef7032b7 remains aborted/recording_deleted, unexpired and stopped, with31 declared objects/27 verified. Original8 fragments/240.206633s/30000/1001fps, inventory, digest and seal operation key remain unchanged. No terminal operation may be reopened or forced through seal. The current native recovery observes the terminal state without replaying pending mutations.

Interrupted test12 recording5edca3a5-c1e0-48f1-9465-e50a345cc952 was deleted after exact user approval on October9 (204, then404). The subsequent native b6e79be retest created recordingb18818dd-dae0-400e-9088-fd1bb740952f / capture6ca719fbb9bbc4c6ab73c0b2fdd795f6 and failed at the first segment because the Windows queue move lacked an extended path. The [long-path fix](reviews/windows-queue-long-path-2026-10-09.md) passes RED→GREEN, all12 local CTests, and actual local OBS three-rendition hash/full-decode QA. The new capture remains aborted/capture_incomplete with0 remote objects; no stop/seal/ready/publication succeeded. Recordingb18818dd-dae0-400e-9088-fd1bb740952f was subsequently deleted after exact user approval (204, then404). Old long11 recording405ba990-994a-4553-88d7-28be32b141d5 remains retained without deletion authority; its capture now reports failed/recording_media_unavailable. Current production capturec17a21af1047858e4ce9bbe8a0a5b797 ended normally at2375.906867s/80fragments per rendition after operator confirmation; all247 local hashes and three full decodes pass. Its obsolete developer driver prevented the planned240s stop. The old failed capture repeats a stale object page and prevented its background upload/stop/seal. The [terminal-pagination fix and regression](reviews/windows-terminal-pagination-2026-10-09.md) pass all12 local tests; actual corrected recovery/ready/publication remain pending. Preserve all local media, journals and failure evidence; never change operation keys or accounts to bypass quota.

After lawful quota release, use a fresh native candidate event to verify recording/upload/stop/seal → freezing → ready, existing explicit auto-publication intent → published, member playback and all three media renditions. Then qualify30min sustained live with5min controlled outage/10min catch-up and measured steady-state latency, full-event normal-stop DVR/grant renewal/expiry, live-to-recording, and current2.5h production tests. Preserve29.97fps and900-frame/30.03s segments; never upload fake fixture hashes.

Current member-player acceptance also has a reproducible [quality-change/backward-seek failure](reviews/windows-member-quality-seek-2026-10-08.md). On the previously published617s synthetic recording, 720p→480p followed by a backward seek shrank the actual duration/slider maximum to480.511999s and, after reload and reproduction,337.855999s. Seeking610s was rejected. A reload control had a complete21-fragment/617.183233s ENDLIST playlist and actual480p tail playback through ended=true. Preserve the evidence and have the platform owner investigate; a successful reload does not close this failure, and old media does not qualify the current plugin candidate.

Independent local evidence now includes corrected9000s hash/grid/full-decode QA, selected audio tracks1–6/mute and actual Studio Mode Program/Preview isolation/transition. The [original-recording/HHC9000s run](reviews/windows-original-concurrent-long-2026-10-08.md) completed with OBS exit0, 9000026ms overlap, 907 verified HHC objects, three full decoded renditions and a verified/full-decoded9032.924s original MKV. The original watcher exit1 from its unsupported extra30s duration bound is retained; the corrected original verifier returns0. It is not production E2E. One reported allocation remains unattributed.

The [actual NVENC exhaustion check](reviews/windows-nvenc-exhaustion-2026-10-08.md) now qualifies module-startup denial and per-rendition partial-initialization failure without CPU fallback or false completion. Both journals remain unchanged, partial NVENC resources are released, and an independent healthy three-rendition capture passes full media checks. Physical GPU removal and encoding-time device failure remain unqualified.

The [actual NTFS ACL queue-write denial](reviews/windows-acl-queue-denial-2026-10-08.md) preserves six closed hashes, incomplete stop state and fully decoded retained fragments; exact original ACL is restored. Explicit stopped-session and ordinary-crash recovery modes each accept only their correct stopIntent state, without journal changes. This is not physical disk exhaustion or real platform permission revocation.

The [ordinary-mode interruption test](reviews/windows-ordinary-crash-2026-10-08.md) retained six verified closed objects, recovered the exact journal in a separate process, and actually detected the unclean stop on ordinary restart. The hidden crash prompt still awaits the requested explicit window-display authorization; normal/safe-mode selection and native dock availability are unqualified. Separate portable roots have separate instance mutexes, so another portable long run does not block this test. Original YouTube plus recording plus HHC still requires a deliberately authorized synthetic YouTube test. Physical keyboard/DPI, physical failures and iPhone Safari remain separate. Mac owns shared platform load/cost/permission acceptance.

See [current evidence](reviews/windows-e2e-2026-10-08.md) for exact sources, candidate hashes and CI. Candidate is unsigned, PR#1 remains draft, and no merge/deployment/release/YouTube modification is performed. Operator installation, login and recovery use the [GUI instructions](windows-local-preview.md); developer fixtures are excluded from packages.

The [ready retention/GUI inspection update](reviews/windows-ready-retention-2026-10-08.md) now connects accepted seal/authoritative ready to stable local package evidence and lists successful owned data after seven days. It opens only the selected qualified folder; no deletion occurs. All10 local tests pass and the fixed C1's11 immutable artifacts were reverified. Original capture GET at22:10Taipei remains terminal; inventory unchanged, no forced retry. Candidate864bf3b archive hash f01e246b15608579af3743f44049cf848f395e016abb2cd6bdc7eeb367aa6f97 and installed files were verified. Actual native recovery of historical published capture1f13f5a6ff0af956cc1d03cce61c65cf saved matching ready evidence, kept all70 objects/inventory/keys unchanged, and exited0. This ready readback does not close new positive production E2E; hosted source CI results are recorded separately in the evidence JSON.
