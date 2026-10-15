# K3 Console acceptance: deleted receiver blocks preparation

Observed 2026-10-15 07:17–07:19 Asia/Taipei (UTC+8).

## Fixed versions

- Admin PR236 merged as `efa766b45adfefca977fb06ef11d820e72503839`; Production Release `38651992792` completed successfully.
- Production `/assets/index-Ca4D_zh9.js` contains that exact release commit.
- Windows installed-source log: `4f332583bbcd5ed9221dbbb03c74c025d5cc6fa9`.
- Normal ZIP SHA-256: `e4f4fa485ce07769d9e7a03ae720ba26ab64618f970b933538939e8f6c7ca361`.
- Mac fixed handoff SHA-256 verified: `9bc08faaea6949d6b89c7da61b7352cbd3f94c22d634280925c2b9e30ccdca66`.
- CMS/Asset runtime versions were not independently reread this round. No wire change.

## Actual result

Fresh authenticated production Console, existing saved key unchanged:

1. OBS connection settings shows `無法載入 OBS 連線，請重試。`.
2. Retrying performs GET `/api/admin/broadcast-source`: HTTP200, revision11, configured=true, canRotate=true, inputBusy=false, targetRecordingId=`ca0b1b12-39c2-4657-ba1a-45ffc25624e1`.
3. GET `/api/admin/broadcasts/ca0b1b12-39c2-4657-ba1a-45ffc25624e1`: HTTP404, `broadcast_not_found`.
4. That completed Web208 short field was permanently deleted with explicit user confirmation in the preceding cleanup. Admin removal and member original URL unavailability were verified. Local evidence was retained.
5. Created exactly one new immediate draft through the UI, with auto-start and auto-publish disabled: `2294ed31-3c2c-4f66-982a-720465bba109`, title `[HHC OBS SYNTHETIC TEST] K3 Console immediate 4f33258 2026-10-15`.
6. Its control room still links to the deleted receiver, reports `無法確認接收場次，訊號與公開狀態暫時不明。`, and disables `準備直播`. The normal UI cannot replace the missing target.

This is a production Console failure, not an OBS failure. No OBS output, capture, announcement, manual publication, key rotation, or direct-API bypass was performed. The new draft is retained for the fix verification. No second draft was needed once preparation was blocked.

## Requested platform correction

Handle an authoritative deleted/missing receiver without stranding the reusable source. Investigate both source cleanup on recording deletion and Admin missing-target handling. Preserve transaction/revision checks and occupied-input protection. Do not interpret generic 401/403/5xx/network failures as idle or silently retry mutations with new intent. Do not require resetting the key or recreating media.

Add regression coverage for released target deletion, settings/control-room recovery and preparation of the next event. Retain tests for active input and stale source/event revisions. After authorized release, return actual deployed commits/assets and fixed handoff. Reuse the above draft and stored key where appropriate; then resume only missing two-Console conflict/receiver-summary/release-to-next cases.

Existing player, 16-minute recovery and long-field passes are unchanged. Historical live-lag/weak-network failures and real meeting+YouTube untested status remain unchanged. This round does not establish fresh native-media acceptance for the UI candidate.
