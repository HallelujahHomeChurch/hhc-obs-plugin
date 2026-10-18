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
ready evidence. The helper retries eligible local cleanup every ten seconds without needing
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

## Isolated real OBS acceptance (Asia/Taipei UTC+8)

- OBS 32.2.2 portable, loaded `ec527ea5af46e0326f26059fe2494dde6b350a49`;
  candidate ZIP SHA-256 `5427f093c72b3bb6a12587445d2f9815cbdf7eb57e6d68be2b430c2acc58cf42`.
- Recording `2f0d4a8d-27e5-4dc1-92d9-e0fbf73cc836`, capture
  `fd20cd243b64757a63e0da14221cc04c`, local/output
  `29234ce9-0df5-414a-9f91-18c5b784d32b`.
- Synthetic HHC source: 29.97 fps / 10.01-second S10, three renditions;
  Console preview decoded 854x480 at currentTime 38.836 before normal stop.
- Native recording started 13:18:24.850. HHC output stopped 13:22:44.668;
  Stop accepted 13:22:44.821554549, seal accepted 13:23:05.975546 (server receipts).
- First observed ready and cleanup complete: 13:24:31.101; sampled serverNow
  13:24:28.324660538 (not an exact ready event timestamp).
- 82 journal objects remain documented; all 88 HLS files including staging copies,
  165,229,565 bytes, automatically became zero. No manual deletion was used.
- Native recording was still running after cleanup: 453,922,816 bytes at the first
  cleanup sample, then 469,532,672 bytes at 13:24:41.107. Stopped independently
  at 13:25:04.787. Result: 399.365 seconds, 1920x1080 HEVC 30000/1001 plus
  AAC 48kHz stereo, 499,544,033 bytes. Full ffmpeg decode exit 0, empty stderr.
  SHA-256 `cf3929c19822b13a09f1b36028a1223c69f98c4b8cf3446f90829b3d0a7ed5b7`.
- OBS UI showed the HHC start button available and native recording continuing;
  compact status showed HHC state and GPU Encode with native CPU/FPS/timers intact.
- Post-acceptance review added bounded retry of other ready sessions even after
  the selected transfer exits exceptionally. Native QProcess regression holds a
  prior sync lock, triggers no-bind exit, releases the lock, then verifies cleanup
  and the original exit code without networking. Full CTest 26/26 passes.
- Evidence: `artifacts/auto-hls-cleanup-20261018/real/` safe 5-second samples,
  native decode result and preserved recording. No YouTube/native global stream
  was started. Formal OBS briefly opened idle during launch selection, then closed;
  the actual capture and recording ran only in the portable profile.

Cleanup readiness is independent of publication; this acceptance does not claim
new member playback, outage recovery, or long-run coverage. Formal installation
and remote CI are recorded separately after this acceptance. OBS remains 32.2.2.
Historical failed/unfinished captures remain retained and can still consume disk.
