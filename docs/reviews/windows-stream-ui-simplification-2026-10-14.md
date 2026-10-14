# HHC stream dock simplification — 2026-10-14

Scope: Windows K3 dock, OBS audio-track adapter, tests and operator instructions. No platform code, contract, capture encoding parameters, global stream/recording control, or event selection changed.

- Rename dock HHC 串流; stable dock ID retained for layouts.
- Main controls: status, 開始直播/停止直播, 設定. Read-only platform title remains identification only.
- Settings hold masked key editor, optional website shortcut and legacy recovery. Closing settings clears unsaved key text. Legacy journals are not discarded.
- Fix key editor remaining locked after prior input released. Active capture, preparation, pending sync/control and input-lock ownership still fence edits.
- Transient transport recovery remains automatic; manual retry is visible only for paused intervention.
- Remove audio selection UI. Each new input reads OBS profile stream track (Simple=1, Advanced=TrackIndex); invalid selection prevents capture. Existing journals preserve their original track for recovery.
- Native OBS local recording remains independent. No new 'recording' action that silently uploads or publishes.

Validation: key replacement regression observed failing before fix. Full RelWithDebInfo build, 18/18 CTest, media-grid Python test and PowerShell path/exit-code test passed. Additional UI tests cover hidden settings, unsaved secret clearing, conditional recovery and invalid track rejection.

This is local/unit evidence; existing real OBS 5af0fa4/Web208 passes remain historical. New audio adapter has not yet undergone live audio decoding or end-to-end capture. No new capture or platform changes required for UI inspection. Candidate is unsigned.

Mac coordination: docs/reviews/mac-stream-ui-simplification-2026-10-14.md. Request evidence for existing single-key/event exclusion before proposing any new contract.

2026-10-15 follow-up: Removed the 240px dock minimum. Key settings are now a separate owned non-modal dialog; closing it clears unsaved key text. Main labels wrap without imposing long-title widths. Removed legacy account recovery entry and controller ownership from K3, per user direction; no historic files deleted. Regression test first failed against inline settings, then verifies independent window, resize to 160px and secret clearing. This does not make controls usable at zero width; native button/text minima remain.
