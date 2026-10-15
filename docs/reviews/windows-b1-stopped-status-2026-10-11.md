# B1 stopped-capture status text

## Observed problem and bounded change

During the real OBS run at plugin `9cbeec04d79a09eee89c37f0a92460b80708a805`, the source had stopped and seal was accepted, but the dock still showed `控制室：等待結束邊界`. At **2026-10-11 19:15:25.772 Asia/Taipei UTC+8**, its main state correctly showed server validation. Read-only B1 GET at 19:04:49.909 had already shown `processing`, with the accepted public interval **[10,327)**.

Active control polling ends with the encoder. Its last cached `end_pending` phase is therefore not a current post-capture status. The C1 uploader intentionally does not depend on a fresh B1 view in this phase. Adding control polling or ACK attempts after shutdown would change recovery behavior unnecessarily.

The dock now directs the operator to the Console for post-capture broadcast state during uploading, validation, draft-ready and published states. The main upload/validation status remains available. It does not invent an archived/published outcome from a cached control phase. This change only affects presentation in `Dock::apply`; encoder, HTTP, journal, command ACK, public bounds and publication policy are unchanged.

## Verification and limits

- Regression test first failed in all four stopped phases with the old presentation; it then passed after the bounded dock change.
- Full native build exits 0; all **14/14 CTest cases** pass after rebuilding all consumers. A fresh complete CTest run at approximately **19:26 UTC+8** also exits 0.
- Media-grid and watcher-path checks pass. These are local/mock evidence, not formal-platform or GPU qualification of a new package.
- The installed OBS candidate remains `9cbeec0` throughout its long-run acceptance. Its media and end-to-end results must not be attributed to this later presentation change or package.
- Package source revision and SHA-256 will be reported separately after committing, reconfiguring and rebuilding. The candidate remains unsigned; no production deployment or merge is authorized.

See [the real OBS long-run evidence](windows-b1-recheck-long-2026-10-11.md) and [the separate Mac platform follow-up](windows-b1-admin-refresh-followup-2026-10-11.md). Fixing this dock label does not repair expired preview authorization, member-list thumbnails or the observed DVR jump at Console End.

## Fixed candidate and hosted evidence

Product commit: **c2166da44da0d2fccbd4f84d82aa54a2fa03ad55**. Both hosted Windows native runs **38135586667** and **38135589088** complete successfully. [Downloadable CI artifact](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/actions/runs/38135586667): `hhc-obs-windows-x64-unverified`.

Contained product ZIP `hhc-obs-plugin-windows-x64-platform-preview-c2166da.zip` has SHA-256 **daa2a94841fb9506e02ce761266bf00ddb4eb5820eb9460a6e63b95888021fe6**. It was downloaded and independently rehashed at **19:33:45.9037476 UTC+8**, matching its adjacent hash file; all manifest file hashes match and sourceCommit is exactlyc2166da. GitHub's outer artifact-container digest is a different object and must not be confused with this product ZIP.

Separately built local product ZIP hash is **ff1f9d7686f52e6b28d79e7ed41601df8d95b6fb0ee39889c9915e5865df7762**. Packaging verifies a clean committed source, the compiled DLL source revision and the helper's `--build-info`; its timestamp-dependent ZIP bytes are not claimed identical to CI.

A first optional native warm-load attempt at19:35 encountered a still-running9cbe background helper's executable/TLS locks. Only the new DLL was copied at that point; this incomplete installation was explicitly not qualified and its receipt is retained separately. OBS was closed and the original9cbe DLL restored. The owned post-test helper cache scan was stopped after the capture was ready/published with zero pending; all journals/media remain intact.

At **19:40:06.3390592 UTC+8**, the complete CI DLL/helper/Schannel runtime set is installed in the dedicated portable OBS only. All three installed file hashes match the CI manifest; helper build-info exits0 and matchesc2166da. No new capture is created. Native module-load evidence and shutdown are recorded separately from the9cbe endurance run; neither installation nor CI constitutes a newc2166da GPU/end-to-end long-run pass. The operator's original OBS profiles and YouTube configuration remain untouched.

OBS32.2.2 logs compiled sourcec2166da at **19:40:07.110**, startup complete19:40:07.607 and Native platform dock ready19:40:07.814. Its actual HHC dock opens through the existing Docks menu; global Stream/Record controls both show Start, and no capture/encoder is started. Native UIA and the dock screenshot are retained. It normally shuts down19:41:51.552 with this warm-load log's memory-leak counter0; this does not replace the separate9cbe full-run log's counter1.

The installed CI helper's developer `--auth-check` at **19:42:17.7774373** exits0: `OAuth refresh OK; read=1 write=1 publish=1`. It uses the account-bound existing Windows Credential Manager credentials, performs no capture/media write and exposes no credential. This qualifies native warm-load/package consistency and an OAuth refresh check, while the post-stop label regression remains directly exercised by the mock dock test. No OBS/helper process remains after this bounded check.
