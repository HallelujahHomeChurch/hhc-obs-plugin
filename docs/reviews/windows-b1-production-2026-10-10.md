# Windows B1 production acceptance

User confirmed no meeting in progress and authorized actual OBS testing. Contract acknowledgement remains B1 `b1-2026-10-10.rc1`, C1 `c1-2026-10-08.2`, inventory 1, 30000/1001 fps and 900-frame/30.03-second segments.

## First qualified candidate: failed encoder start

- Plugin: `29dc37050f9d1dc317ec62a1763f09839b54d665`.
- CI ZIP SHA-256: `6304902f3903dbb446625fdf8ddda58df725db4afea866c8fa996457c768f5ab`.
- Current CMS release run `38060911650` completed successfully before testing: commit `ae3b6fb5d36ca978c00e6bc269074ba7773d3365`, revision `hhc-web-api--0000181`, healthy, writer enabled and 100% traffic. The earlier `0000180` smoke is not evidence for this revision.
- Other read-back revisions: Asset `0000089`, Website `0000193`, Gateway `0000202`, Account `0000160`.
- OBS 32.2.2 portable HHC-Fixture, Program text clock, audio track 1. Original global stream and recording inactive; no YouTube changes.
- Broadcast/recording: `d48eeb42-bced-4e5b-9444-691a986fbdf1`; manual start/end, auto-publish intended. Announcement accepted; binding reached private preview.
- Bind UI invoked at **2026-10-10 23:31:18.668 Asia/Taipei (UTC+8)**. Capture `70c1b3ef130e88565f3ba1e8830e0615`, local session `6402289e-dde1-482b-a217-0e80a43787a6`.
- OBS error at **23:31:20.835 UTC+8**: `Capture directory must be new`. No media produced; no public start/end ACK. C1 abort observed at **23:31:24.801 UTC+8**, pending bytes 0, auto-publish cancelled. Console reports `capture_incomplete`.
- Root cause: `SessionStore::isPrepared()` allowed C1 metadata but rejected the durable `broadcast-journal.json` created by B1 before initial encoder start. Mock binding tests did not previously exercise this preparation gate.
- Regression test failed specifically on the real B1 journal plus adopted C1 journal preparation assertion. The minimal fix adds that exact metadata filename; unknown files, existing media directories, links and non-pristine local journals remain rejected. Fresh build and all **14/14 local CTests passed** after the fix. This is local evidence, not production acceptance of the replacement package.

Retained metadata SHA-256:

| File | SHA-256 |
| --- | --- |
| broadcast-journal.json | 54c6f17c3a79b515386b755b3784101e106ffb3056dfabd05d5faf8ec86b4b41 |
| journal.json | ae4c87e1220fa213be5de0158bdd6958fee031436d646d32b32eb503a76aae77 |
| local-session.json | 2ae8eed2e166b6635c5baa9ee3672f3a605c2d2d20e1b70538e868136c06fa78 |
| remote-journal.json | f5f65dc03eb9835362274d6144254dd82058fc9dd8e056e4850b83e6f48c4852 |

Runtime preparation also found an old developer fixture DLL registering a duplicate HHC output. It was not executing a fixture capture, and the first-start failure is explained by the preparation gate above. Remove the unused fixture from the owned portable runtime before retesting the normal package; preserve it outside the plugin load directory.

Still unqualified: actual preview decode, start/end ACK and public range, crash recovery, stop/seal/ready/publication, member-player quality/DVR/ABR, normal-load automatic recovery, outage stress and current-version 2.5-hour continuous playback. Historical approximately 379/388-second delay failures remain failures. Unsigned package; Safari native HLS/fullscreen untested.
