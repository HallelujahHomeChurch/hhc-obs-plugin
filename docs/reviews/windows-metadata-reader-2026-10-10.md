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
those attribute lookups have not been characterized. The local checks alone are not evidence
for concurrent writers, historical root cause, hosted CI, actual OBS or
end-to-end qualification of the new source revision.

Commit `3462fe5c4adb6bcdfd30719ede136aa046e25d07` push CI37994926661 and
PR CI37994931498 both succeeded. Downloaded Windows x64 unsigned preview:
630099 bytes, SHA-256
`f422bfffd81a724961700ac773e2cac97e4d81e934fc5634a478d515127dbe53`.
All seven manifest members and both embedded source stamps matched; the
developer fixture is excluded. The inactive test runtime was updated at
05:47:16.6444722. Its actual CI helper refreshed the existing OAuth session
successfully using Credential Manager and resumed the original pending
thumbnail40 capture; original inventory hash was unchanged.

Actual OBS loaded the same compiled source and began a new9000-second
native capture at05:48:55.867: recording
`8a2a3ce3-5dff-4da6-9350-32932e926642`, capture
`632bfda5df0932c7698cd4713cb5fff3`, local identity
`405cabc2-d6e6-49d2-b5f8-6e58aaa3d5dd`. C1 acknowledgement retains all11
verified immutable references and30000/1001 fps. This run is in progress;
it has not yet passed final media validation, stop/seal/ready/publication,
member VOD or native exit. Other OBS encoders are inactive; prior captures'
background synchronization and member browser observations remain separate
loads. Installation and actual startup do not qualify the long-run gate.

The same downloaded CI3462fe5 subsequently passed actual OBS GUI recovery
of the original long34 session in the other inactive portable runtime.
It selected `9c89a897-7043-4a8e-8769-38ad0077960a`, displayed published,
returned complete=true without starting an encoder, and exited0 at
06:52:30.7177366 with all original operator profile hashes unchanged.
The907-object inventory hash remained unchanged; original stop and seal
receipts retained acceptance times05:19:58.754341759 and05:21:28.223757.
This tests current-source recovery of oldd59f1eb media, independently of
the new9000-second encoding run. Its short synthetic renderer overlapped
long41 and is included in whole-GPU resource observations.

Ignored local evidence is retained under `artifacts/metadata-reader-*`:
the original stress and regression failures, error-32 diagnostic, final
stress JSON/log, and fresh candidate build/CTest logs. Only allowlisted
counts and Win32 codes are recorded; no credentials, server bodies or
signed URLs are retained.
