# Mac handoff: keep event management out of OBS

使用者已確認產品方向：OBS 只負責固定串流金鑰及開始／停止影音輸出；直接直播或預排場次由 Console 決定，外掛不選場、不設定時間、不判斷是否該公開。

Windows 本輪簡化：面板更名 HHC 串流；主畫面狀態、開始／停止直播、設定。金鑰移入設定，輸入釋放後可更換；自動重連／補傳保持原流程，僅需人工介入時顯示重試。移除音軌選單，新收錄沿用 OBS 串流音軌，恢復使用原 journal 音軌。管理直播及 legacy 恢復僅留設定內。沒有新增 Admin 控制權限或 wire。

請先核對既有實作；已符合者提供證據，不重做：

1. 同一把金鑰同時只能指定一個有效接收場次。直接直播與預排的 prepare／切換必須原子互斥，包含兩個 Console 同時操作；衝突在 Console 清楚呈現，不能讓 OBS 選場或接到兩場。允許未來預排資料存在，不等於同時占用輸入。
2. Console 明確呈現金鑰目前接收哪一場、訊號與公開狀態。預定時間仍是預告，autoStart／autoEnd 由場次策略決定；OBS 的開始直播僅開始傳送，不強制公開、不建立直接直播替代預排場。
3. 保留 K3 未準備的 409 broadcast_source_not_prepared 等待語意；已綁定／斷線／續期／補傳仍固定原 capture、epoch、operation key，不重新 resolve 下一場。前場 End＋Stop＋seal 釋放後才可接用下一場。
4. 正常停止與網路中斷必須區分，Console End 不影響 YouTube 或本機錄影；公開 [start,endExclusive)、never-public、會後發布與只錄影語意不變。
5. 核對新舊金鑰／撤銷的錯誤與操作說明，不要求在 OBS 登入或選場。若以上已由 K3 完整提供，不新增契約；若確有缺口，提供固定版本差異及錯誤碼後再串接。

回傳一份 Windows 最小驗收 prompt：固定 commit／部署版本／契約 hash／已通過證據／剩餘案例。不重測既有已通過的播放器與長場；不宣稱本輪 UI 變更修復歷史弱網或負載問題。不得傳回金鑰、token、cookie 或 signed URL。沒有新的清理、merge 或部署授權。
