# Actual NTFS queue write denial and retained recovery

This developer test uses actual isolated OBS 32.2.2/NVENC and the existing capture code from candidate f009725. The test base is62fecd4; no production plugin code, wire schema, operator package or real account permissions change.

Run `powershell -File tests/capture-file-sharing-test.ps1 -Failure acl -Portable portable-concurrent -RunName UNIQUE_NAME` only with the existing developer portable runtime. The check refuses an active owned OBS or a production plugin in that runtime, preserves existing evidence, and changes only the newly created test queue's1080p directory. After the six first closed objects pass hashes, a temporary deny-CreateFiles rule for the current user makes an actual creation probe return UnauthorizedAccessException. Existing objects remain readable. Original SDDL is saved before the change and restored in finally; a separate readback confirms exact restoration. No existing user profile or another process's queue is targeted.

Actual OBS process33312 ran21:27:38–21:28:43 Taipei. The failed tail was recorded as EncoderFailure/stopReason2, user stopIntent=true, normalEnd=false, sealAcknowledged=false and confirmedReady=false. Six closed objects remained in the journal with unchanged hashes; the failed next fragment was not declared. OBS exited0. All retained1080p/720p/480p fragments passed SHA-256, size, H.264/AAC format,30000/1001fps,48kHz stereo and full decoding. The final local dock screenshot was inspected and reports that the capture needs attention and local data are retained.

A separate actual sharing-denial regression on the same runtime also returned0 with the same incomplete/retained-object invariants. Its aclRestored=false and accessDeniedProbeConfirmed=false indicate that this branch never changed an ACL; they do not indicate a failed ACL restore. The ACL branch records both true. Both original profile/encoder configuration hashes remain unchanged.

## Explicit failed-stop recovery check

The original developer `--recover` check intentionally requires stopIntent=false for an interrupted capture. Applying it to this valid failed-tail journal returned17 because stopIntent is correctly true. Do not change the journal to make that check pass. The newly requested `--recover-stopped` mode was initially unsupported and returned2; after adding the explicit mode it returned0 against the same retained session and verified six closed objects.

The original crash mode still rejects the stopped journal with17. Against the actual ordinary-mode crash journal, crash recovery returns0 and stopped recovery rejects with17. Thus all four state checks pass without weakening the existing guard. Both journals remain byte-identical. This executes actual SessionStore loadPending and media hashing in a separate process; it does not qualify the ordinary OBS startup dialog, same-dock recovery UI or production HTTP reconciliation.

Build and clang-format checks pass; all10 CTest cases pass separately from these actual OBS/recovery checks. The runs report one allocation at exit, whose cause remains unqualified. Results and safe hashes are in the adjacent JSON; source logs, ACL backup, failed/red recovery checks, decoded-fragment proof and original receipts are retained in `artifacts/obs-acl-denial-62fecd4` and `artifacts/obs-sharing-regression-62fecd4`.

This closes local NTFS permission denial for queue-file creation. Physical disk exhaustion/power failure and real platform permission revocation remain separate gates. Candidate f009725 is unchanged; current positive seal/freezing/ready/publication/member playback and long production tests remain incomplete.
