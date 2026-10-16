# Mac K4 targeted follow-up — 2026-10-16

請只處理下列直接直播正常停止的收尾問題；已通過的預排短場、播放器、16分鐘續傳及2.5小時長場不要重做。請保留兩筆本輪診斷場，不手動發布、不刪除、不重送Stop/seal、不重設金鑰。

## 固定版本

Windows隔離OBS32.2.2，plugin667ee287e75ed9382624938eb2360bf3fe70eb48，PR#3；兩個native CI成功。
候選ZIP SHA256:86f5f216a1d74bd445c55c7e3a8c10198feb68fec1f44c71657e6132c93beab6。
平台依Mac交接：CMS0000200/4a426ebf、Gateway0000208、Admin8168c1a、Website0000209、Asset0000094；實際Admin entry index-B-iXU8Lu.js。Windows本輪未重新讀取Azurerevision，因此平台版本以你們交付證據為準。
K4/B1/C1、inventory1、30000/1001fps、900幀30.03秒、digest與公開邊界不變。

## 需排查：直接直播正常Stop後未自動End／歸檔

- recording89cf022d-e80e-479f-9bbe-d8590771cc59
- capture32264350a74315406f3d119a72023cce，epoch1
- outputSessionId9580c378-5f1e-4175-9fc9-66b9a351d277
- source2c58a390-1906-4d62-b962-777f7f40bbb5
- 下列時間均2026-10-16 Asia/Taipei UTC+8：
  - 編碼開始09:44:44.508；autoStart成功，startSequence0。
  - HHC正常Stop09:49:18.777；Stop accepted09:49:20.307682039。
  - seal accepted09:50:02.0298，normalEnd=true。
  - 錄影API readyAt09:51:17.232848；capture ready/liveState ended。
  - 10:08:44.196錄影API仍draft/version2、autoPublish pending、未選封面。
  - 10:09:45.349原Studio API仍live/revision6；start0/endExclusive=null/boundaryRevision3，archive none；policy autoStart/autoEnd/autoPublish皆true。
- 會員原頁成功解碼1080p，之後顯示ended replay；尚未自然轉VOD。Console仍顯示直播中，錄影頁卻顯示直播已結束／完成後自動發布。
- 本機全三畫質完整解碼、SHA256、fps/audio、對齊分段通過：各10段274.040433秒。
- 固定contract第5節要求normal sender stop在共同邊界關閉direct；目前未觀察到。請對照stop/seal回呼、autoEnd/reconciler、公開範圍與縮圖／發布鏈路，確定根因。不要僅修改badge或由Windows補人工End掩蓋問題。

## 已通過對照：預排Start/End及自然發布

- recordingd1618202-89ec-44c8-9082-0d7fcb242040
- capture6c69d79ff2981de8d40380b2b926bd8f，epoch1
- outputSessionIdcb125385-2253-458e-84dc-8d01a16d819d
- 使用相同已儲存金鑰、Console準備預排，OBS無選場；17:00預定時間沒有阻擋10:00前後公開。
- 編碼09:58:08.190；Start ACK sequence4；End ACK sequence10；公開[4,10)，180.18秒。
- Console End約10:02:57；Stop accepted10:03:08.493830221；僅HHC output自動停10:03:09.466；seal10:03:42.277122。
- 錄影readyAt10:04:34.9723，published/version3；10:08:55.833原Studio已archived/revision12/archive published。processing曾延遲更新，最終成功，不列為持續故障。
- Staff preview10:02:50.962實解碼854x480/592frames；會員先480p後1080p，無MediaError。早期preview409在媒體就緒後恢復，不列為持續故障。
- 原會員頁無reload；正常按「接續觀看會後影片」後同URL VOD。10:09:45.329：1920x1080、418frames、currentTime73.78498、duration180.210666、readyState4、pausedfalse/rate1/errornull；實際公開長度比180.18秒多30.666ms，不包含120.12秒預覽或End後尾段。
- 全來源11段301.034067秒，三畫質本機QA皆通過。
- VOD自動封面1已選取，Studio auto cover ready；image bytes取回為空，沒有可靠hash一致性結論，不以空hash充數。

## 回覆要求

請完成根因與最小修正，提供PR/CI、固定交接文件與hash。依既有授權流程處理合併部署；未部署不要稱正式可驗收。部署完成請附實際CMS/Asset/Admin等受影響revision、commit、image digest、旗標與UTC+8確認時間，並提供可直接貼回Windows的最小重測prompt。

請先唯讀對照原直接場是否自然恢復；不要替它人工發布。必要時只新增一筆direct短場驗證正常Stop→End→ready→自然發布、同URLVOD與縮圖。Console關閉時新direct、production lost-response/restart、雙Console競爭的K4新增實機證據仍未完成；既有local/mock結果不冒充正式通過。
歷史379/388秒落後、弱網seek及聚會＋YouTube負載未驗證保持原判定。不得輸出憑證或signed URL。
