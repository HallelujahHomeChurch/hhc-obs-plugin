# Copyable Mac follow-up — Windows B1 production evidence

請接續 B1 正式平台驗收阻擋項，先讀 Windows 固定證據報告：
`hhc-obs-plugin/docs/reviews/windows-b1-production-2026-10-10.md`。
Windows 沒有修改平台 repositories、merge、部署或 YouTube。

## 已確認版本與 Windows 結果

- 固定平台交接：`hhc-web-api` commit `bcea55a56aae3eaeefe64bc066218011706ea7e1` 的 `docs/obs-capture/live-control-v1/production-acceptance-2026-10-10.md`；交接及五份固定契約 bytes/hash 已重核。
- C1 `c1-2026-10-08.2`、B1 `b1-2026-10-10.rc1`、inventory 1、30000/1001 fps、900 幀／30.03 秒不變。
- 等待週報 release `38060911650` 成功後，Windows 實測 CMS `ae3b6fb5d36ca978c00e6bc269074ba7773d3365`／`hhc-web-api--0000181`，Healthy、writer=true、100% 流量；不是先前 `0000180` smoke。
- Asset `asset-api--0000089`、Website `hhc-web--0000193`、Gateway `api-gateway--0000202`、Account `account-api--0000160`。Validation job 每分鐘排程；image digest `sha256:4252d07f048f4e350fc99623ae8b2674e6c3d1c92feb38db29d01263dc4344ef`。
- Windows 修正真 OBS 揭露的 prepared-directory 與 optional commandId 消費錯誤後，`fe61bb12eae6376578911f17742ef329a4cc8b4b` 真 OBS 三畫質、Start/End ACK、Stop、seal、40 物件 ready 通過。本機完整 hash/grid／逐畫質全解碼通過。local ZIP SHA-256 `b481c5d711f09997eabdfc2f4a143a3fef2c90aac0b40bac7b95b9289d21b08a`。
- 最新 `b259fa747f5454d0c1d9c4bab686484d36646a54` 僅修正 B1 停止／ready 發布文案；14/14 本機測試、push `38066593938`／PR `38066599817` CI 通過，真 OBS never-public 程序中斷後 GUI 恢復原 capture／binding 並安全 abort 通過，12/12 原媒體 hash 不變。local ZIP SHA-256 `18a5ace1a39b81d3c55797c609effa342c6965a63475ef3e81fd0f3102f2694d`。
- b259fa7 已下載的 CI 產品 ZIP SHA-256 `351cd9de0b0cf8b257003da609a01ce448f70f0f2541980b001a020e5cc9ed88`，run `38066593938`、artifact `hhc-obs-windows-x64-unverified`；七個 manifest hash、DLL/helper source、fps 已核對，未簽章。真實測試使用 local 包，不能寫成 CI ZIP 實機通過。

## 請查證並修復的平台路徑

以下均為 **2026-10-11 Asia/Taipei（UTC+8）**。保留實際原始時鐘；local UI 與 server 接受時間不可強行排序。

主要 recording/broadcast：`e9c5ebb4-74d7-46e0-a7b4-e3730579fc8a`。
capture：`e818d9764d7997b1e04deccf07961b49`，epoch 1。
local session：`c0ac9aa7-8fa4-4b2d-895e-45a41841890a`。

1. **Staff preview Cookie/CORS**：00:05:33.673，Console preview-access 200；`media.alive.org.tw` cookie OPTIONS preflight 403，沒有 Access-Control-* headers，CF ray `a486ddb35afb2566-TPE`。另一場亦重現。請查 Worker／Gateway 的正式 cookie、origin／credentials、CORS 路徑；用 Admin 正常 preview 解碼與會員不得存取 staff preview 的正反驗證收尾。不要以 preview-access 200 取代實際播放。
2. **Ready 後自動發布**：native Start command `3f734393-07e6-4a22-9e60-dd94b65e2ff7` ACK boundary 2；End command `1e640777-2802-4eee-8461-950545e8db5c` ACK endExclusive 8；公開 [2,8) = 180.18 秒，後續 Stop 未扩大。全素材仍是 11 段／300.566933 秒／三畫質，40 objects、199807226 bytes。server stop 00:07:44.848755863、seal 00:08:44.579015；readyAt 00:10:47.442034。seal inventoryDigest `3e99e25aa80aded1959bc221571a04d32b44ee4a0b79bb34a712774260c32274`。00:33:31.621 B1 archive=ready、phase=processing、無 pending command；CMS 仍 draft、selectedCoverId=null、capture autoPublish=pending。Console autoPublish=true，沒有手動 Publish／取消意圖。請查 CMS/B1 archive／C1 auto-publication reconciliation，保留 [start,endExclusive)；勿要求 Windows 再上傳或更改 duration／key。
3. **結束後原會員頁提前顯示到期**：00:10:03.364 原跟隨播放器顯示「直播回看期限已到」，至 00:29 後仍維持，未 reload 或按回到直播；C1 replayUntil=00:44:42.973070 尚未到期。此前 00:06:17.861 真解碼 854x480、currentTime 17.051157、duration 90.126666、520 frames／0 dropped、playing 1x、error=null。請查會員 broadcast 判定、public replay grant／scope／過渡到 VOD 的實際 API 與播放路徑。這是 UI/期限落差證據，Windows 未假定根因或自行繞過授權。
4. **直播縮圖轉 VOD bytes／選取**：直播 JPEG SHA-256 `7149154592890b5a8c3bab813c1e879c96cece57fc97961f26f155382e9ac67a`；00:29:26.470 Admin VOD 已選預覽 JPEG SHA-256 `7d10d879b03af64959ba15728632017fc8ad4065dcd65d38851f2d16acca86d7`。UI 顯示自動封面1，但權威 metadata selectedCoverId=null。另兩張圖 hash `60da7d53044c0cf2089c6e164fd43b6704e30e4068186e7fcda2eefaf5231623`、`0ca9b1cc56d50dcb2f7f5ebb11c215aff9acc44a356e6115998c80b2e68779a9`。請查 B1 archive 是否保留直播 JPEG bytes、candidate1 與選取，同時區分 UI 預設 radio 與伺服器已保存選取。

先保留上述測試場供查證，勿用手動發布掩蓋自動發布驗收；不要碰正式聚會錄影。另一個已安全 abort 的 never-public 測試 recording `744f5b7e-f68a-478b-9670-b340260417c8`／capture `91f5fbea6a20c38a6be60d18d7b21cdc` 可按測試清理流程移除，Windows 原 journal／素材/hash 保留。

## 收尾要求與 Windows 下一輪

依實際根因最小修正、CI／正式 release、read-back revision／image／開關／health，提供 immutable commit 的交接報告與 SHA-256。不要放寬媒體驗證、wire、digest、冪等、會員授權或 29.97 fps 規則。不要把最終发布、UI 修正或本機/CI 成功改寫成直播自動復線已通過。

**完成後請附一份可直接貼回 Windows 的 continuation prompt**，列出實際部署、上述各阻擋項的處置與需重測路徑、可保留重試的原 capture／operation keys、是否需新場及已清理的 ID。先詢問目前是否聚會／能否接手真 OBS；未確認只做本機與唯讀。

Windows 下一輪依 A–D 收尾後，再測正常聚會實際負載（原錄影／實際需要時 YouTube）的無 reload 自動復線與穩定約 2–3 分鐘延遲、主動 DVR／暫停／倍速保持；五分鐘斷線／十分鐘追趕另列壓力，最後做固定版本 2.5 小時。保留舊約379／388秒落後失敗。弱網 ABR、自然片尾→回跳→1080p→Auto、快速與暫停切畫質均需實際解碼／位置證據；Safari native HLS/fullscreen 仍未測。

回報必須分開本機、CI、實機、端到端与部署，含 recording/capture、Asia/Taipei 毫秒時間、真解碼 width/height、currentTime、duration、buffered/seekable、幀數、暫停／倍速／延遲。不得貼 token、密碼、cookie 或 signed URL。
