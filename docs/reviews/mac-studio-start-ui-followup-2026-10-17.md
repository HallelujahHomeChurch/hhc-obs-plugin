# Mac handoff: Studio direct-start and preview UI — 2026-10-17

請合併處理以下平台問題；不要重跑已通過的播放器、16 分鐘續傳或 2.5 小時長場。Windows 正在獨立修 OBS 按鈕主色與 GPU Video Encode 顯示，不需要改 wire。

## 1. 優先查直接直播為何停在私密 preview

正式外掛：2d3d1303bcdfb66f3168324aa13dfc124be44ab1（PR #5 已合併，main ad647195c0c8339cb5691e95148af14df0a49c1d）。
本機 outputSessionId：7cb5e762-2eba-406a-a6fd-b4c833665e8b
recording：2aa794f4-60ad-44cc-95bf-6227c746be7a
capture：6dec18ef53495c53272eb9e47f97dcbf

使用者在正式 OBS 按「開始 HHC 串流」，Console 為 /content/studio/direct。
2026-10-17 14:13:25.919–14:13:26.001 Asia/Taipei（UTC+8）三路 HHC NVENC 啟動；14:13:26.047 原生錄影啟動。
K4 持久化回應 broadcast.phase=preview；broadcast journal commands={}，未有公開 Start 命令／ACK。
素材確實上傳：最後 capture state=ready、liveState=ending、progress.lastSequence=3、autoPublish=cancelled。
本機 normalEnd=true、sealAcknowledged=true、confirmedReady=true；readyAt=2026-10-17T06:17:01.088Z（14:17:01.088 UTC+8），這是保存的服務 ready 時間，不是精確前台發布時間。
讀取時 active.json released=true。Console 文案為「停止 HHC 串流輸出後，這場私密預覽會自動取消。OBS 的其他輸出可繼續。」

上述證明編碼／上傳／驗證有完成，沒有證明公開直播。請依 ID 對照 open decision、委派／source target／mode、auto-start policy、ready prerequisites、Start 命令建立與 reconciler 日誌，找出直接直播為何落入／停留 preview。不要先假定根因就是 auto-start=false 或使用者設錯。若是保留 target 或設定問題，需呈現可理解的原因，避免直接直播頁看似已可直接開播卻靜默成私密預覽；不要把場次管理或公開按鈕加到 OBS。
本輪診斷未重新 Start/Stop/seal、未手動發布、未改模式／金鑰；請保留這場診斷資料。

## 2. 預覽載入失敗與白框

同一 Console 顯示「暫時無法顯示預覽，請重新載入」。請查 preview ticket／授權、媒體就緒、playlist 與播放器錯誤；不能把白框對比修正當成預覽已恢復。
白框已定位為文字「載入預覽」的 button：computed color rgb(255,255,255)，background rgb(255,253,249)，尺寸98×40 CSS px。修復黑色預覽區內的重試按鈕對比、焦點與disabled狀態，與周圍暗色工具列一致。

## 3. 左上直播狀態對齊使用者提供的 YouTube 參考

參考圖為左上單層緊湊暗色標籤、左側狀態圓點與文字；目前 HHC 為外層深灰容器內又包淺色膠囊。請對齊單層層級、內距、高度、圓角、距預覽頂／左邊距及右側金鑰工具列的視覺基線。文字較長時與窄版仍需不重疊。
沿用真實 HHC 狀態語意（未連線／等待／私密預覽／公開直播／中斷／結束），不要照抄參考「沒有資料」或只憑badge宣告直播成功。分開顯示輸入訊號與公開狀態。

## 4. 分享圖示

使用者提供的是向右上方的外框分享箭頭參考。請核對實際分享按鈕圖示、線寬、視覺尺寸、對齊與留白，使用一致的線框風格，保留清楚可點範圍、hover/focus及可存取名稱。不要把「複製」雙矩形或金鑰重設圖示當作分享；不改分享目的地／權限。此項是視覺要求，Windows 尚未宣稱已定位特定CSS根因。

## 交付與最小驗收

先回報目前實際 runtime commits/revisions 與上述原因；依使用者授權流程完成修正、測試、CI及部署，不把候選當正式。
補測 direct open→真訊號→自動公開Start→正常Stop→seal/ready/自然發布／同URL回放（若原意圖允許）。另確認明確私密預覽與預排行為不被改壞。
UI涵蓋無訊號／載入失敗／preview／公開／中斷／窄版，分享和重試按鈕對比／焦點。不得以重整掩蓋原頁問題。
完成後提供固定 commit 交接文件、SHA-256、實際部署版本、根因、已測與未測及 Windows 最小重測 prompt。K4/K3/B1/C1、29.97fps、900幀/30.03秒、digest、冪等及完整媒體驗證不變；不要為本問題放寬驗證。不得提供任何憑證或 signed URL。
