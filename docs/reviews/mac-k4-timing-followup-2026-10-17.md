# Mac: K4 public End / attempt timing correlation

請依同目錄 windows-k4-timing-2026-10-17.md 核對本次既定短場的伺服器紀錄。外掛未修改，短場已正常停止、seal並ready；不要重傳、重送Stop/seal、手動發布、刪場或重設金鑰。

部署採你提供的CMSd04f7b7fdd2d3f6d0de8a61a52a7fd5434d86c56/0000207、Asset37cba2f0aa7b1c8420e53cdece47a8033c4a4875/0000099。Windows沒有再次讀Azurerevision。

- Recording:4e3a9939-0e60-4cfb-a1c3-71993324abb7
- Capture:33890e9b63b1b860da1ae5fbfee78ede
- Output:e16c1645-2b17-4a60-96d6-317bd585ffb5
- Installed plugin:667ee287e75ed9382624938eb2360bf3fe70eb48

日期2026-10-17，全部Asia/Taipei UTC+8：
- 編碼開始01:52:30.848；公開開始API01:55:11.851272。
- 正常Stop接受01:55:45.481045392；seal接受01:56:09.954308。
- attempt1開始01:56:40.867709，phase=package_validation。
- API readyAt01:57:16.564643；seal-to-ready66.610335秒。
- 01:56:43.700原Console仍live/end=null；01:57:38.408已processing/public[0,7)。請提供精確公開End時間；Windows不以這兩次觀察推算精確End。
- attempt開始到ready差35.696934秒，並非已證明worker完整attempt耗時；請核對claim/stage/result/finish及實際attempt數。
- 截至02:02:16.749仍draft/version3、selectedCoverId已有值、capture.autoPublish=pending；ready後300.184秒。原會員頁保持ended replay。請核對自然發布是否後續成功、精確時間及等待原因；未宣告永久失敗，不以人工發布完成驗收。

三畫質各7段，194.661133秒，30000/1001fps；本機完整decode/hash/grid通過。會員直播實際1920x1080、frames641、currentTime111.297241，MediaError=null。既有首次轉VOD錯誤仍保留，本輪尚未到可執行該步驟。

請回傳固定伺服器證據／時間與結論；若需修正，再提供commit/revision和最小重測prompt。已通過的長場與播放器整套不重跑。不得回傳憑證或signed URL。
