# Developer-only actual OBS storage regression; no platform credentials or HTTP.
param([ValidatePattern('^[a-zA-Z0-9_-]{1,64}$')][string]$RunName=('storage-'+(Get-Date -Format yyyyMMdd-HHmmss)),[switch]$CancelStopOnce)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $root
$out=Join-Path $root "artifacts/$RunName"
if(Test-Path -LiteralPath $out){throw 'Preserve existing evidence'}
$portable=Join-Path $root '.deps/portable-concurrent'
$exe=[IO.Path]::GetFullPath((Join-Path $portable 'bin/64bit/obs64.exe')).Replace('/','\')
if(@(Get-CimInstance Win32_Process -Filter "Name='obs64.exe'"|Where-Object ExecutablePath -EQ $exe).Count){throw 'Owned OBS active'}
if(Test-Path -LiteralPath (Join-Path $portable 'obs-plugins/64bit/hhc-obs-plugin.dll')){throw 'Use only the local developer fixture'}
Copy-Item -LiteralPath (Join-Path $root 'build/RelWithDebInfo/hhc-obs-fixture.dll') -Destination (Join-Path $portable 'obs-plugins/64bit/hhc-obs-fixture.dll')
Get-ChildItem Env:HHC_FIXTURE_* | Remove-Item
Remove-Item Env:QT_PLUGIN_PATH,Env:QT_SCALE_FACTOR -ErrorAction SilentlyContinue
$env:HHC_FIXTURE_OUTPUT=$out
$env:HHC_FIXTURE_SECONDS='61'
$env:HHC_FIXTURE_DOCK='1'
if($CancelStopOnce){$env:HHC_FIXTURE_CANCEL_STOP_ONCE='1'}
$started=Get-Date
$p=Start-Process -FilePath $exe -ArgumentList '--portable','--multi','--disable-updater','--disable-missing-files-check' -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
$null=$p.Handle
if(-not $p.WaitForExit(100000)){throw 'Observation timed out; preserve live process'}
$p.WaitForExit()
$inventories=@(Get-ChildItem -LiteralPath $out -Recurse -Filter inventory.json)
if($p.ExitCode -ne 0 -or $inventories.Count -ne 1){throw 'Capture prerequisite failed'}
$media=$inventories[0].DirectoryName
$inventory=Get-Content -LiteralPath $inventories[0].FullName -Raw | ConvertFrom-Json
$staging=@(Get-ChildItem -LiteralPath "$media/staging" -Recurse -Filter 'seg-*.m4s')
$segments=@($inventory.objects | Where-Object path -Like '*.m4s')
$result=@{scope='actual local OBS storage regression; not physical disk failure or E2E';sourceCommit=(& git rev-parse HEAD);dirty=[bool](& git status --porcelain);fixtureSha256=(Get-FileHash -LiteralPath (Join-Path $portable 'obs-plugins/64bit/hhc-obs-fixture.dll') -Algorithm SHA256).Hash.ToLower();started=$started.ToString('o');completed=(Get-Date).ToString('o');pid=$p.Id;exitCode=$p.ExitCode;media=$media;normalEnd=$inventory.normalEnd;canonicalSegments=$segments.Count;stagingSegments=$staging.Count}
$result | ConvertTo-Json | Set-Content -LiteralPath "$out/storage-result.json"
if(-not $inventory.normalEnd -or $segments.Count -ne 9 -or $staging.Count){throw 'Expected nine canonical segments and no staging duplicates'}
if($CancelStopOnce){
 $cancel=Get-Content -LiteralPath "$out/stop-cancel.json" -Raw | ConvertFrom-Json
 if(-not $cancel.cancelKeptEncoding -or -not $cancel.stopIntentUnchanged){throw 'Cancel changed capture or its durable stop intent'}
}
Write-Output 'PASS: three completed renditions; nine canonical segments; no staging media copies'
