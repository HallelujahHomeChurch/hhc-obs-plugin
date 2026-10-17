# OBS native UI candidate — 2026-10-17

Scope: isolated OBS 32.2.2 only; production OBS installation and profiles unchanged.

- HHC start/stop is inserted below OBS streaming in Controls. Original streaming and recording handlers are not connected or invoked by this integration.
- HHC status, successful media upload average (30-second window), and queued size are shown in the existing status bar. No additional HHC timer or dropped-frame counter.
- Original OBS Settings contains an HHC 設定 page. Key entry stays there; Apply/OK validates and saves through Credential Manager, Cancel discards. Blank entry preserves the existing key. Active capture/finalization locks replacement.
- No login, event management, encoding changes, wire changes or independent settings dialog.

Local evidence: all 23 CTests pass, including native page insertion, duplicate prevention, invalid Apply, Cancel, original handler forwarding and page lifetime across reopen. Media-grid check passes. Evidence: `artifacts/native-ui-20261017/ctest.txt`.

Real isolated OBS: 2026-10-17 13:10–13:15 Asia/Taipei, Controls and footer rendered; original Settings showed HHC 設定 and detected existing saved key; Cancel and reopening worked without a second key window. The existing completed output remained released. No new capture or credentials were created.

Limitations: settings integration uses OBS 32.2.2 internal Qt widget names and its original button slot. Revalidate before supporting another OBS release. Local/UI evidence is not CI, live upload throughput, simultaneous YouTube/recording, or end-to-end acceptance. Historical platform delay and weak-network limitations are unchanged. Production installation is not part of this preview.

Pre-merge independent review found native validation and titlebar/Escape save gaps. Fixed: HHC syntax validation precedes native Apply, credentials commit only after native ClearChanged acknowledges success. Dirty HHC close requests use Save/Discard/Cancel and the same apply path. Tests cover native rejection without credential write, cancelled close, rejected save-close and Escape discard. Full 23 CTests, media-grid and watcher-path checks pass after these changes.
