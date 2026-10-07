# Windows x64 本機驗證版

這是平台尚未就緒前的本機驗證版，不是完成端到端驗收的正式版。只支援已測試的 OBS 32.2.2 Windows x64。需要 NVIDIA NVENC；不會切換 CPU。不要安裝開發用的 hhc-obs-fixture.dll。

## 安裝（不需 CLI）

1. 結束 OBS。首次請使用獨立 portable OBS 測試副本。
2. 解壓 ZIP，用檔案總管把 `obs-plugins` 資料夾合併到 OBS 安裝根目錄。根目錄內應已有 `bin`、`data`、`obs-plugins`。
3. 若已有 `obs-plugins/64bit/hhc-obs-plugin.dll`，先備份該檔再替換；勿覆蓋 OBS 的 Qt、FFmpeg 或其他外掛 DLL。
4. 開啟 OBS，在「停駐視窗 / Docks」中開啟「HHC 影音」。介面必須顯示「本機驗證模式」。

## 操作

Program 必須為 1920×1080、30000/1001（29.97 fps）、NV12／limited BT.709；音訊為 48 kHz stereo。設定不符會拒絕開始，不會自行改動 OBS。選擇音軌 1–6，按「開始本機驗證收錄」；停止時等待「本機收錄完成」。

直播與自動發布在此版本停用。本機驗證資料不會自動綁定未來登入的帳號，也不代表平台的「只錄影」工作流程已完成。此版不執行任何網路上傳。既有 YouTube 及 OBS 原錄影使用自己的控制。

「重新檢查本機收錄」會在背景驗證 journal、封閉物件與 SHA-256，列出正常或待處理的收錄。「開啟本機資料夾」使用 Windows 檔案總管。異常結束的收錄保留為未完成，不会自動變成可發布影片。關閉 dock 只隱藏；退出 OBS 時若正在收錄會要求確認並保留未完成資料。

## 移除（保留資料）

結束 OBS，用檔案總管移走或刪除 **只有** `obs-plugins/64bit/hhc-obs-plugin.dll`，再啟動 OBS。不要刪除 OBS 設定資料夾：journal 和本機媒體位於 OBS 的 `plugin_config/hhc-obs-plugin/local-test-queue`，portable 模式則在該副本自己的 config 下。移除 DLL 不會刪除收錄，也不會回復或改動 YouTube 設定。

## 待平台整合

Mac 提供固定版 API 契約、連線位址、登入設定及就緒通知後，才驗證真正的錄製／上傳／會員直播／斷線恢復／直播轉錄影／自動發布。保留 29.97 fps；Mac 處理目前直播驗證器固定 30 fps 的落差。本機成功不表示端到端通過。
