# Local 2.5-hour original recording and HHC concurrency

This is actual Windows OBS 32.2.2 local concurrency evidence from developer producer `7e17c7446ffcc844556a0b1a2e1e97cffb3c71bb`. It is not a current-candidate production platform test. The operator candidate remains `f009725b49a7ea8cb24745f9df660d5f6351da4a`; the developer fixture is excluded from its package. No YouTube stream, HTTP capture or publication intent was started by this run.

The isolated OBS process13768 started the cloned original HEVC NVENC/MKV/FLAC recording before the HHC three-rendition synthetic Program capture. HHC started at18:25:36.301 Taipei, stopped encoding at20:55:36.549, and completed its queue before the fixture stopped the original recording at20:56:08.106. The retained process handle reports OBS exit0. The receipt measures9000026ms of overlap, with zero encoding skipped frames and zero render lagged frames. The original recording preserves1920x1080/30000/1001fps and48kHz stereo FLAC. Original installed profile and encoder configuration hashes remain unchanged.

HHC finalized907 objects totaling5982736690 bytes. Each of the1080p/720p/480p renditions has300 fragments and9000.024367 seconds:299 regular fragments of30.03 seconds followed by21.054367 seconds. Hashes, sizes, playlists, aligned timelines, H.264/AAC formats and full three-rendition decoding pass. Inventory SHA-256 is `9663d2560c93b77ab2712db7c3eab5515a984e9f8965163ad76aa1f0c9a9e0f2`. normalEnd and stopIntent are true; sealAcknowledged and confirmedReady are false, as required for this local test.

The original MKV is28232443094 bytes and9032.924 seconds. SHA-256 is `f72de7af0e704990b6496a7b2cfc1a6e3a575fc9c378e560e5b590401fa0bc4c`. A separate whole-file FFmpeg decode returned0 with an empty error log. The original file is retained at `artifacts/original-concurrent-recordings-9000/2026-10-08 18-25-34.mkv`; HHC inventory and media are under `artifacts/original-concurrent-long-9000`. These are Windows local paths, not cross-machine delivery claims.

## QA duration ruling and preserved failure

The original watcher returned1 after its HHC checks passed: the original-recording verifier required duration to be at most requestedSeconds+30. Both the direct reproduction and unchanged watcher failed with `AssertionError: unexpected recording duration`. The fixture deliberately stops the original recording only after HHC queue finalization; the actual log and receipt show that completion, rather than a lost stop or extra capture intent. There is no fixed30-second upper bound for hashing/finalizing a long queue.

Ruling: validate finite duration of at least the requested interval, actual full overlap, requested stop, stopped receipt, completed HHC output, unchanged media formats, SHA-256 and whole-file decoding. Remove the unsupported extra-duration upper bound. Cost if wrong: this media verifier no longer detects excessive queue-finalization delay from file duration alone; shutdown latency remains a separate performance gate. No media bytes, duration, timestamps, stop receipt or failed watcher result are edited to pass.

The original exit receipt retains obsExitCode0/watcherExitCode1 and the failed status/log. The corrected original verifier returned0 against exactly the same MKV and receipt, with complete hash/full-decode verification; its result is recorded separately in the adjacent JSON. Do not reinterpret the original watcher result as0.

The OBS exit log reports one allocation. Earlier short developer fixtures, including plugin-absent controls, also report one; this does not establish the cause of the long-run allocation. Leak-free operation is not qualified by this result.

Production seal/freezing/ready/publication, member live-to-VOD behavior, current-candidate2.5-hour E2E, network recovery, YouTube concurrency and physical-device acceptance remain separate gates. The ordinary crash prompt and operator keyboard tests also remain separate from this long run.
