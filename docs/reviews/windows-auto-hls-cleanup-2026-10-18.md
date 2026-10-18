# Windows automatic HLS cleanup

Scope: new local capture journals, unchanged K4/K3/B1/C1 wire and S10 encoding.

The HHC start button becomes available after Stop and seal are accepted. This
does not prove that platform media validation is ready. Keep that release behavior;
continue cleanup in the existing background finalization path after matching
capture/package ready evidence is durably recorded. Publication is independent.

New journals opt in to automatic cleanup. Pre-upgrade journals without the opt-in
retain their diagnostic media; no historical sweep or formal recording deletion
was performed. Only journal-listed HLS files and the six known muxer staging
init/playlist copies are deleted. Source files, native OBS recordings, inventory,
hashes, receipts and journals remain. No recursive directory deletion is used.

Cleanup first validates paths and immutable media, persists a pending intent,
then deletes owned files and records completion. Locked files leave pending state
and are retried without converting a successful upload/release into a failure.
Recovery accepts intentionally missing media only with durable cleanup and matching
ready evidence. The helper retries eligible local cleanup on launch without needing
an unexpired remote session; it respects each session sync lock. An OBS restart
also hands the last released input to the existing helper, since released is not
the same as ready. Existing helper lifetime is 24 hours; retained cleanup resumes
on a subsequent helper/OBS launch rather than running an unbounded service.

## Evidence

- Regression test first failed on the original implementation: ready did not
  remove HLS. It passes with this change.
- Windows RelWithDebInfo build passed.
- CTest: 26/26 passed, including the existing partial PUT/lost confirmation cases.
- Native filesystem test covers >MAX_PATH media, stop without ready, partial
  deletion caused by an open file, durable journal write failure, restart retry,
  synchronization lock ownership, old journal preservation, package mismatch,
  traversal rejection before deletion, and an actual Windows directory junction.
- Mock HTTP finalization verifies blocked local cleanup preserves accepted Stop/
  seal, then completes on the next ready poll; automatic publication remains separate.

This is local/native filesystem and mock HTTP evidence, not a new real-platform
capture or end-to-end cleanup acceptance. No public stream, credential change,
formal OBS installation, merge or deployment was performed. OBS remains 32.2.2.
Historical failed/unfinished captures remain retained and can still consume disk.
