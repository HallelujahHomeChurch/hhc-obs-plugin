# HHC 串流

HHC 串流是 Windows 版 OBS Studio 外掛，將 OBS 的節目畫面與串流音訊直播到 HHC 官網。可在 HHC 後台準備直接直播、預排場次及管理回放。YouTube 串流與 OBS 本機錄影各用原本的控制項。

**適用環境：** Windows x64、OBS Studio 32.2.2、支援 NVENC 的 NVIDIA 顯卡，以及 HHC 串流金鑰。Windows 套件未簽章。

**[Windows 下載與版本說明](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/releases)** · **[開啟 HHC 直播後台](https://admin.alive.org.tw/content/studio/direct)**

## 安裝或更新

先結束串流和錄影，等 HHC 補傳完成，再關閉 OBS。在**系統管理員 PowerShell** 貼上這一條指令，即可下載並安裝 `v0.2.1`：

```powershell
& { $ErrorActionPreference='Stop'; $p=Join-Path $env:TEMP ('hhc-install-'+[guid]::NewGuid()+'.ps1'); try { Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/HallelujahHomeChurch/hhc-obs-plugin/eb57e5042fd86bf99b2a72418eb456361217b0f2/scripts/install-windows.ps1' -OutFile $p; if ((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne '20cceb8eaeede841041457d5ffe04f8044c9f64cc2c2e38411b1dd76a7f50255') { throw 'Installer SHA-256 mismatch' }; powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File $p -ReleaseTag v0.2.1; if ($LASTEXITCODE -ne 0) { throw 'HHC installation failed' } } finally { Remove-Item -LiteralPath $p -Force -ErrorAction SilentlyContinue } }
```

指令會核對安裝腳本和發布 ZIP 的 SHA-256，備份舊外掛，再安裝到預設的 `C:\Program Files\obs-studio`。安裝後重開 OBS，從 **設定 → HHC** 確認載入。自訂安裝目錄或 portable OBS 可使用下方的 ZIP。

### 從 ZIP 安裝

1. 在發布頁的 **Assets** 下載檔名不含 `-modern` 的 Windows x64 ZIP，完整解壓。
2. 結束串流與錄影，等 HHC 補傳完成，再關閉 OBS。
3. 雙擊 **`Install-HHC.cmd`**。按 Enter 使用畫面列出的 OBS 安裝目錄，或輸入自訂目錄。Windows 可能要求管理員權限。更新時會先備份舊外掛。
4. 重新開啟 OBS，確認看得到 **設定 → HHC** 和 **控制項 → 開始 HHC 串流**。

若下載的舊版 ZIP 沒有 `Install-HHC.cmd`，或你下載的是 `-modern` 包，請依[手動安裝說明](docs/windows-local-preview.md#手動安裝)操作。

## 開一場直播

1. 在 HHC 直播後台準備直接直播或預排場次，並取得串流金鑰。
2. 第一次使用時，到 **OBS → 設定 → HHC** 輸入金鑰，按 **套用** 或 **確定**。
3. 按 **開始 HHC 串流**，到後台確認訊號與公開狀態。OBS 的「傳送中」表示來源正在傳送，公開時間由後台設定決定。
4. 結束時按 **停止 HHC 串流**，等外掛收尾，再到後台確認回放狀態。

[完整操作、更新與排錯](docs/windows-local-preview.md) · [回報問題](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/issues) · [開發與測試](docs/windows-development.md) · [授權](LICENSE)
