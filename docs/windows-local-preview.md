# HHC 串流操作指南

適用 Windows x64、OBS Studio 32.2.2 和支援 NVENC 的 NVIDIA 顯卡。Windows 發布包未簽章。

## 安裝或更新

1. 從 [GitHub Releases](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/releases) 的 **Assets** 下載檔名不含 `-modern` 的標準 ZIP。
2. 等串流、錄影與 HHC 補傳結束，正常關閉 OBS，再完整解壓 ZIP。
3. 雙擊 **`Install-HHC.cmd`**。按 Enter 使用顯示的安裝目錄，或輸入自訂目錄。Windows 可能要求管理員權限。若 PowerShell 因下載檔案的安全標記拒絕執行，請先核對發布頁的 SHA-256，再從 ZIP「內容」解除封鎖並重新解壓，或使用下方的手動方式。
4. 安裝成功後開 OBS，從 **設定 → HHC** 和 **控制項 → 開始 HHC 串流** 確認外掛已載入。

更新時，舊外掛檔案會備份到 OBS 安裝根目錄內的 `.hhc-plugin-backups`。需要回退時先關閉 OBS，再從所選備份目錄把同路徑的檔案複製回安裝根目錄。

### 手動安裝

| ZIP | 安裝方法 |
| --- | --- |
| 標準 ZIP（含 `obs-plugins` 和 `data`） | 關閉 OBS，把兩個資料夾合併到 OBS 安裝根目錄。預設為 `C:\Program Files\obs-studio`。Portable OBS 使用自己的根目錄。 |
| `-modern` ZIP（含 `hhc-obs-plugin`） | 關閉 OBS，把整個 `hhc-obs-plugin` 資料夾放入 `C:\ProgramData\obs-studio\plugins`，新版 portable OBS 則放在根目錄的 `plugins` 下。OBS 33／34 的外掛相容性仍需另行驗證。 |

保留 ZIP 的資料夾結構，外掛 DLL、補傳程式和 TLS 元件要一起安裝。同一套 OBS 只使用一種安裝方式。升級 OBS 前請先以隔離環境驗證。 [OBS 官方目錄說明](https://obsproject.com/kb/legacy-plugin-locations)

## 準備、開始與停止

- 在 [HHC 後台](https://admin.alive.org.tw/content/studio/direct) 準備直接直播或預排場次。標題、縮圖、公開及發布設定由後台管理。
- 到 **OBS → 設定 → HHC** 貼入串流金鑰後按「套用」或「確定」。金鑰儲存在 Windows Credential Manager，日常開播沿用已儲存的金鑰。
- **開始 HHC 串流** 送出 OBS 的節目畫面。簡易輸出用音軌 1，進階輸出用 OBS 設定的串流音軌。音量與靜音以 OBS 混音器為準。驗證環境採 1920×1080、29.97 fps 及 48 kHz 雙聲道。
- 在後台確認訊號與公開狀態。按 **停止 HHC 串流** 後，外掛會上傳剩餘分段並提交收尾。從後台確認是否已可回放。HHC 設定也可選擇開始／停止確認、自動錄製，以及 HHC 停止後是否繼續本機錄影。

## 補傳與磁碟空間

連線暫時中斷時，外掛會在平台允許的期限內自動重試與補傳。正常關閉 OBS 後，背景程序可繼續收尾，需讓電腦保持開機。平台確認媒體就緒後，外掛會清理符合條件的本機 HLS 暫存。未完成或失敗的素材留在原處以便恢復。OBS 本機錄影另存於原生錄影目錄。

## 排錯與移除

- **看不到 HHC 設定或按鈕**：確認 OBS 是 32.2.2、ZIP 已完整解壓，且檔案已裝到目前使用的 OBS。
- **後台一直等待訊號**：確認使用正確的串流金鑰和場次，並檢查 OBS 底部的 HHC 狀態。若仍顯示待傳或收尾，保留該場暫存。
- **移除外掛**：待補傳完成並關閉 OBS。標準安裝只移除 `obs-plugins\64bit` 內的 `hhc-obs-plugin.dll`、`hhc-upload-helper.exe`，以及 `data\obs-plugins\hhc-obs-plugin`。`-modern` 安裝移除對應的整個 `hhc-obs-plugin` 資料夾。保留 OBS 設定、原生錄影和待恢復資料。
- [回報問題](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/issues)時，附上 OBS 版本、外掛發布版本與畫面上的錯誤狀態，並遮蔽串流金鑰及播放授權網址。
