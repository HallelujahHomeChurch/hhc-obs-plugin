@echo off
setlocal DisableDelayedExpansion
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy RemoteSigned -File "%~dp0install-windows.ps1"
set "result=%errorlevel%"
echo.
if not "%result%"=="0" echo Installation failed. See README.md for details.
pause
exit /b %result%
