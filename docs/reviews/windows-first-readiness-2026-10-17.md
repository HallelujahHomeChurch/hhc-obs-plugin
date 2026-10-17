# 正式 OBS 首次就緒／早停最小重測 — 2026-10-17

使用者明確指定正式 OBS，未使用隔離環境。OBS 32.2.2 未升級；沿用「聚會」場景與使用者當時準備好的導播機輸入。YouTube 串流全程未啟動，未修改任何設定或金鑰。原生錄影依既有 RecordWhenStreaming=true / KeepRecordingWhenStreamStops=false 自動啟停。

## 固定版本

- 安裝 plugin source: `1f01309529689db2850cf6dd989837524284e499`，PR #6 main merge `d5f4fe554a7dae2ca6cc96c96337ad618c7bf1b0` 同 tree。
- ZIP SHA-256: `aeadf5f838c13624cbaf548ddbf5be0b7258b73d19bba2c5c984c4a76327c7c6`。
- 正式 DLL hash 實讀: `dc4e1a3b6e173fff88fdf6c4c70cdaaab671d8311331f2db1434503e3e9bde0b`。
- 平台交付版本: Admin `2d3d91e97a753568efdd32f443297ff81e865d73`；CMS `f269498de639259d58c0ab80a2a61b05f0cf4a30` / 0000215；Asset `cc88daf912a421d44337fe914192d97fcb537f9f` / 0000100。這些為 Mac 部署讀回，不冒充本輪 Azure 控制面重核。
- 固定診斷原始檔 SHA-256 已實算符合 `3d950a448999d04faffcb772662a48f12a9d804cf4662475ab8030cce0c880db`。
- K4/B1/C1、30000/1001 fps、900 幀／30.03 秒不變。此次沒有程式變更、沒有重跑 CI；沿用已成功 CI 候選。

## A 一般直接開播

recording `394502a0-e51e-410f-9cdd-70218f3c3d59`
capture `f4333ffcbc6ccb7399b662efdab41745`
outputSessionId `85c5f978-f657-4c10-b6dc-15b0e5cba3b4`

所有下列時間為 2026-10-17 Asia/Taipei UTC+8；服務事件和觀察時間分開標示。

| 事件 | 時間 | 性質 |
|---|---|---|
| 開始按鈕 |16:30:37.445|客戶端操作，之後有既有確認對話框|
| 第一個 HHC NVENC 設定 |16:30:53.722|OBS log|
| 首組三畫質 confirm 完成 |16:31:52.298|最後一個 seq0 receipt，非 UI badge|
| 首次 verified seq0 |16:32:12.504|1 秒 journal 採樣首次觀察；serverNow 16:32:08.239|
| 公開開始 |16:32:32.736|recording.broadcastStartedAt 服務欄位|
| 兩端首個正向解碼採樣 |16:33:18.427|Console 854x480/447 frames/16.204s；會員 1920x1080/434 frames/74.389s。不是精確首幀時刻|
| 正常 Stop 確認 |16:33:34.326|客戶端操作|
| Stop 接受 |16:33:36.316|receipt.acceptedAt|
| seal 接受 |16:34:04.704|receipt.acceptedAt|
| 自動 End 公開範圍 [0,6) |16:35:09.475|首次輪詢觀察 processing/revision7；不是精確 End 時刻|
| ready |16:35:21.341|local journal.readyAt；16:35:26.156 首次採樣確認 ready|
| 自動封面選取可見 |16:38:04.402|客戶端觀察；不能推算精確封面處理完成|
| published/version4 |16:39:31.149|首次本輪採樣讀到；前次16:39:03.885仍 draft/version3|
| 同 URL 正式 VOD 解碼 |16:40:26.880|1920x1080，862 frames，currentTime28.583，duration160.618666，paused=false，rate1，readyState4，MediaError=null|

通過：未按手動 preview／公開開始；原 Console、會員頁未 reload；三組完整分段上傳確認且兩端解碼後停止。6 段（5完整+短尾）、完整公開長度約160.62秒。Stop/seal一次；自然 End/ready/publish；Console 自動導向錄影設定。會員原頁以產品「接續觀看會後影片」操作轉同 URL VOD，再從起點播放，非 reload。封面選取觀察至 published 觀察為約86.75秒，但採樣稀疏、選圖中間有版本變更，不作精確 reconciler cadence 合格宣稱。

## B 首次就緒附近停止

recording `670d447b-adb5-4ed9-8bd1-42ed24f3a697`
capture `3d64990400be95f3141265d9caeff99a`
outputSessionId `401b8ae4-d140-4262-aa1a-ffbd474c6bc0`

開播前重進仍為同一 draft / binding=null / autoStart+autoEnd+autoPublish=true。重進僅發生在開始前；開始後原頁保持不 reload。

| 事件 | UTC+8時間 | 性質 |
|---|---|---|
| 開始確認 |16:37:08.631|客戶端操作|
| NVENC設定 |16:37:10.119|OBS log|
| 首組三畫質 confirm 完成 |16:38:23.736|最後 seq0 receipt|
| 首次 verified seq0 |16:38:43.376|journal首次觀察；serverNow16:38:39.129|
| Stop 確認 |16:38:57.290|客戶端；會員無video，Console仍start_pending|
| Stop接受 |16:38:57.498|receipt|
| seal接受 |16:39:19.663|receipt|
| ready |16:39:59.466|journal.readyAt；16:40:03.653首次採樣|
| 歸檔設定頁 |16:40:26.879|draft/version1/autoPublish pending；直播開始時間未記錄|

公開範圍 [0,4)。早停時只有seq0/1水位，尚未實際公開，沒有捏造 actualStartedAt。16:42:13.828 首次採樣確認自然 published/version3（前次16:41:11.929仍draft/version1）；直播開始時間仍未記錄。早停後沒有退回 preview/cancelled，未手動發布。

## UI 與獨立問題

- Console 無白色載入預覽框，單層暗色狀態圓點，監控區854x480約16:9，header線框分享箭頭。金鑰selector再次點擊從expanded變collapsed（B開播前）。
- 正式 OBS 按鈕仍呈灰色，即使實際編碼傳送中；footer多次顯示連線重試，隨後自行恢復傳送中。GPU Video Encode顯示約36–46%，包含本機錄影。這是另列的 Windows UI/瞬時重試現象，不以平台通過宣稱已修好。
- 保護的舊 recording/capture 完全未操作；本輪兩場保留。未重設金鑰、手動發布、補送Stop/seal、重傳或刪除。
- 歷史379/388秒延遲、弱網等維持原判定；未重跑長場、完整播放器套件、YouTube並行負載。

## 證據

本機 `artifacts/studio-first-readiness-20261017/`：server-receipts.json、a/b-local.jsonl（約1秒只讀journal採樣）、a/b-browser.json（不等間隔CDP媒體/正常UI網路回應只讀觀察）、PNG。原始憑證或signed URL未寫入報告。


B 同 URL VOD：16:42:38.674 UTC+8 實際854x480、389 frames、currentTime12.788145、duration107.178666、readyState4、paused=false、rate1、MediaError=null，buffered/seekable均[0,107.178666]。原會員頁自然取得VOD後只點播放，未reload。完整素材共4段（3完整+短尾）。

最終：A、B 本輪核心流程通過；兩個新場保留供Mac核對。兩個只讀採樣器已以stop marker結束；active.json released=true。本輪未修正 OBS 灰色按鈕及瞬時連線重試現象，需 Windows 另行定位，不能說所有 UI 問題均通過。
