# Mac follow-up: K4 first Continue VOD transient failure

請只調查本次新觀察到的會員播放器首次轉 VOD 問題；正常停止、自動 End、ready、自然發布已通過，不需重做或修改外掛。

完整 Windows 報告：同目錄 windows-k4-normal-stop-retest-2026-10-16.md。

- Installed plugin667ee287e75ed9382624938eb2360bf3fe70eb48; OBS32.2.2。
- Admin2a000ce515b11958a8f98fa3f90cadf064499270；CMS0000203、Asset0000098，依固定交接版本。
- Recording3e93b740-1915-4cad-b5b6-c379107c2b06
- Capture57764974a1111951f32c81b64f8a9aa5
- Outputabeb1a4f-a053-4c18-bb09-64c30fe95f3d
- 時區Asia/Taipei UTC+8，2026-10-16。

Stop17:51:40.998220190接受；seal17:52:09.598219接受；ready首次serverNow17:53:18.579992952；原Console17:56:19.160觀察archived/published，公開[0,8)。上述是實測成功，不能因播放器問題倒推後端停止失敗。

原會員頁從開播到End/ready/published未reload。直播回看到片尾後，首次按「接續觀看會後影片」（約17:56:14–19）出現「暫時無法播放，請重試」。17:56:30.681實際video仍1920x1080，currentTime=duration=218.453333、paused=true、rate1、readyState4、frames4750、MediaError=null。約17:56:40第二次按相同按鈕成功，沒有reload或改網址。17:57:09.992以正常UI回跳後VOD實際1920x1080、currentTime16.536297、duration218.453333、frames356、readyState4、paused=false、rate1、MediaError=null。

請對照17:56:10–17:56:55的會員回放/播放session/發布可見性，判斷首次失敗原因，針對來源切換與錯誤重試修正或補證。Windows未取得首次HTTP status/code，不要猜為特定錯誤碼或已證明根因。無需重傳素材、手動發布或更動契約。

修正後請提供固定commit、部署revision、證據及最小重測prompt。若既有VOD足以驗證就沿用；只有ended-live窗口必要時才另提新短場，不重跑已通過長場與播放器整套。保留本場及原診斷場，不重設金鑰、不刪除、不重送Stop/seal。不要回傳憑證或signed URL。
