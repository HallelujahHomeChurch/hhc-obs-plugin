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
