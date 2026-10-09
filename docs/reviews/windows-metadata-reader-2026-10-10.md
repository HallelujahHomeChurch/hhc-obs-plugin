# Windows metadata reader replacement race

The Windows shared JSON reader could fail while the encoder replaced a
mutable metadata file with `ReplaceFileW`. A real one-writer/four-reader
characterization performed 2,000 replacements: 691,342 successful reads,
96,944 failed opens, first Win32 error 2, zero partial snapshots and zero
writer failures. This reproduces a reader race; it does not identify the
masked exception in historical capture `ae629481f356c4af926485049b09fcce`.

`openSharedJsonRead` now retries only `ERROR_FILE_NOT_FOUND` (2) and
`ERROR_SHARING_VIOLATION` (32), at most five opens separated by 10 ms.
Successful handles retain read/write/delete sharing and QFile ownership.
Other errors fail immediately; persistent missing files and sharing denial
remain bounded failures. Credential storage, wire contracts, object hashes,
inventory duration, encoder selection and operation keys are unchanged.

The permanent short/extended-path concurrent replacement check failed
before this fix. Retrying only error 2 still failed with error 32. Both
observed errors are now covered. The same four-reader characterization then
passed on short and extended paths: 118,737 and 120,858 reads respectively,
2,000 writes each, no failed opens, partial snapshots or failed writes.

A fresh full local build and all 13 CTests passed at
2026-10-10 05:40:03.0345737 +08:00 after restoring the permanent test source from the
temporary characterization. A read-only review found no critical or
important findings. The reviewer noted scheduling limits in the concurrent
test and optional-file existence checks outside this shared open helper;
those attribute lookups have not been characterized. This is not evidence
for concurrent writers, historical root cause, hosted CI, actual OBS or
end-to-end qualification of the new source revision.

Ignored local evidence is retained under `artifacts/metadata-reader-*`:
the original stress and regression failures, error-32 diagnostic, final
stress JSON/log, and fresh candidate build/CTest logs. Only allowlisted
counts and Win32 codes are recorded; no credentials, server bodies or
signed URLs are retained.
