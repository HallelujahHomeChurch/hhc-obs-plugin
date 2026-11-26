# Windows 開發與測試

使用 Visual Studio 2022 Build Tools 的 C++ x64 工具組、Windows SDK 10.0.26100 與 CMake 3.28 以上。Bootstrap 使用 [OBS 32.2.2](https://github.com/obsproject/obs-studio/tree/32.2.2) 的開發元件及固定依賴版本，僅建置外掛 SDK 所需的部分。

```powershell
./scripts/bootstrap-windows.ps1
cmake --build build --config RelWithDebInfo
ctest --test-dir build -C RelWithDebInfo --output-on-failure
python tests/media-grid-test.py
powershell.exe -NoProfile -File tests/watcher-path-test.ps1
powershell.exe -NoProfile -File tests/install-windows-test.ps1
```

完成測試後，從乾淨且已提交的工作樹封裝：

```powershell
./scripts/package-windows.ps1
./scripts/package-windows.ps1 -Layout modern
./tests/package-layout-test.ps1
```

Release 附上 ZIP 與 SHA-256。標準包附 `Install-HHC.cmd` 與其 PowerShell 安裝程式，`-modern` 包使用手動安裝。兩者均附 `manifest.json` 與操作說明。

- `capture-test` 測試本機安全策略。 `obs-capture-test` 使用 libobs、D3D11 與 NVENC。
- `hhc-obs-fixture.dll` 是開發專用外掛，在專用 portable OBS 產生合成畫面，不隨操作員套件發布。 `scripts/verify-media.py` 驗證本機媒體的 SHA-256、格式、時間線與解碼。
- 安裝測試在暫存資料夾驗證路徑與檔案。GitHub Actions 的建置與封裝和真 OBS、平台直播、長場或會員播放器驗收分開紀錄。
- 逐版本的驗收、故障證據及已知限制見 [reviews](reviews/)、[進度](progress.md)、[平台整合歷史](windows-platform-integration.md)與[驗收項目](windows-integration-next-gates.md)。舊報告中的候選版、契約與分段時長以該次測試為準。
