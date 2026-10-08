# Actual Windows queue commit denial

Two isolated OBS32.2.2 developer-fixture runs completed on 2026-10-08 at approximately20:17 and20:21 Taipei. The normal capture implementation was used; no fault-enabled capture library, platform API or production account was involved. The candidate remains `f009725`; the intervening `c1c81b8` commit changes documentation only.

The test waited until the first900-frame/30.03-second fragment and init were checkpointed for all three NVENC H.264/AAC renditions. It then created an unlisted empty placeholder for the next1080p fragment and held a real Windows FileShare.None handle across its atomic queue commit. The scheduled60-second user stop finished as EncoderFailure. Both OBS processes exited0, and the dock reported failure with local data retained.

Both final journals and inventories kept normalEnd=false. User stopIntent=true persisted; sealAcknowledged=false and confirmedReady=false. All six previously closed objects retained their SHA-256, size and journal membership. The denied placeholder never entered the immutable journal. The first run's retained fragments also passed actual full decoding and H.264/AAC, resolution,48kHz stereo and30000/1001 checks for1920×1080,1280×720 and854×480.

The final runnable check is `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/capture-file-sharing-test.ps1 -RunName UNIQUE_NAME`. It uses the existing developer portable installation and refuses an active owned OBS, installed normal HHC plugin, existing output or changed media. It retains its exact process handle and releases the owned sharing handle in finally. An observation timeout does not terminate or restart the capture. The fixture is excluded from operator packages.

Evidence is in the adjacent JSON and local `artifacts/obs-file-sharing-c1c81b8` / `artifacts/obs-file-sharing-final-c1c81b8` directories, including the dock failure PNGs, exact journals, inventory and actual OBS logs. Original OBS basic.ini, recordEncoder.json and streamEncoder.json hashes remain unchanged. No service.json or credentials were copied.

The initial standalone libobs launcher attempt could not locate the NVENC test subprocess and FFmpeg runtime, failed before encoding and did not fall back to CPU. Its evidence is retained under `artifacts/capture-file-sharing-c1c81b8`; it does not qualify the sharing failure. The subsequent full portable OBS runs are the applicable evidence.

This proves real Windows sharing denial at a queue commit and preservation of earlier closed media. It does not prove ACL revocation, physical disk exhaustion, drive failure, power loss, GPU failure, ordinary single-instance crash-dialog restart or end-to-end recovery. Both short fixture exits report one OBS allocation, as earlier fixture/plugin-absent controls did; the cause remains unproven. The separate original-recording/HHC9000-second run is still active.
