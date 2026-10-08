# Developer-only real NVENC initialization denial; no HTTP, CPU fallback or operator UI claim.
param([ValidatePattern('^[a-zA-Z0-9_-]{1,64}$')][string]$RunName=('nvenc-exhaustion-'+(Get-Date -Format yyyyMMdd-HHmmss)))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $root
$out=Join-Path $root "artifacts/$RunName"
$exe=Join-Path $root '.deps/portable-concurrent/bin/64bit/obs64.exe'
$ffmpeg='C:/ProgramData/chocolatey/lib/ffmpeg/tools/ffmpeg/bin/ffmpeg.exe'
if(Test-Path -LiteralPath $out){throw 'Preserve existing evidence'}
if(@(Get-CimInstance Win32_Process -Filter "Name='obs64.exe'"|Where-Object ExecutablePath -EQ $exe).Count){throw 'Owned fixture OBS active'}
if(Test-Path -LiteralPath (Join-Path $root '.deps/portable-concurrent/obs-plugins/64bit/hhc-obs-plugin.dll')){throw 'Use only the local developer fixture'}
if([int](& nvidia-smi --query-gpu=encoder.stats.sessionCount --format=csv,noheader,nounits) -ne 0){throw 'An existing NVENC workload must not be disturbed'}
$null=New-Item -ItemType Directory -Path $out
$fixture=Join-Path $root '.deps/portable-concurrent/obs-plugins/64bit/hhc-obs-fixture.dll'
Copy-Item -LiteralPath $fixture -Destination "$out/previous-fixture.dll"
Copy-Item -LiteralPath (Join-Path $root 'build/RelWithDebInfo/hhc-obs-fixture.dll') -Destination $fixture
Get-ChildItem Env:HHC_FIXTURE_* | Remove-Item
Remove-Item Env:QT_PLUGIN_PATH,Env:QT_SCALE_FACTOR -ErrorAction SilentlyContinue
$env:HHC_FIXTURE_DOCK='1'
$env:HHC_FIXTURE_AUDIO_TRACK='1'
$env:HHC_FIXTURE_SOURCE_TRACK='1'
$env:HHC_FIXTURE_TONE_HZ='0'
function Invoke-Fixture([string]$Name) {
 $env:HHC_FIXTURE_OUTPUT=Join-Path $out $Name
 $env:HHC_FIXTURE_SECONDS='5'
 $started=Get-Date
 $obs=Start-Process -FilePath $exe -ArgumentList '--portable','--multi','--disable-updater','--disable-missing-files-check' -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
 $null=$obs.Handle
 if(-not $obs.WaitForExit(95000)){throw 'OBS observation timed out; do not terminate or restart it'}
 $obs.WaitForExit()
 $log=Get-ChildItem -LiteralPath (Join-Path $root '.deps/portable-concurrent/config/obs-studio/logs') -Filter '*.txt'|Where-Object LastWriteTime -GE $started|Sort-Object LastWriteTime -Descending|Select-Object -First 1
 if(-not $log){throw 'Owned OBS log missing'}
 Copy-Item -LiteralPath $log.FullName -Destination "$out/$Name-obs.log"
 return @{pid=$obs.Id;exitCode=$obs.ExitCode;started=$started.ToString('o');completed=(Get-Date).ToString('o');log="$out/$Name-obs.log"}
}
$holders=@()
try {
 $denied=$null
 for($i=0;$i -lt 16;$i++) {
  $p=Start-Process -FilePath $ffmpeg -ArgumentList '-hide_banner','-v','error','-re','-f','lavfi','-i','color=size=640x360:rate=1','-c:v','h264_nvenc','-preset','p1','-pix_fmt','nv12','-t','180','-f','null','-' -WindowStyle Hidden -RedirectStandardError "$out/holder-$i.log" -RedirectStandardOutput "$out/holder-$i.stdout" -PassThru
  $null=$p.Handle
  $holders+=$p
  Start-Sleep -Seconds 1
  if($p.HasExited){$p.WaitForExit();$denied=@{pid=$p.Id;exitCode=$p.ExitCode;log="$out/holder-$i.log"};break}
 }
 if(-not $denied -or $denied.exitCode -eq 0){throw 'NVENC resource denial was not observed'}
 $sessions=[int](& nvidia-smi --query-gpu=encoder.stats.sessionCount --format=csv,noheader,nounits)
 if($sessions -lt 1){throw 'No real NVENC holder was established'}
 $failure=Invoke-Fixture 'denied'
 $log=Get-Content -LiteralPath $failure.log -Raw
 if($failure.exitCode -ne 0 -or -not $log.Contains('Dock failed to start') -or -not $log.Contains('[NVENC] Test process failed: session_limit') -or -not $log.Contains("Encoder ID 'obs_nvenc_h264_tex' not found") -or $log.Contains('[x264 encoder:')){throw 'Expected native NVENC module denial without CPU initialization was not observed'}
 $journalFiles=@(Get-ChildItem -LiteralPath "$out/denied" -Recurse -Filter journal.json)
 if($journalFiles.Count -ne 1){throw 'Expected one retained failed-start journal'}
 $journal=Get-Content -LiteralPath $journalFiles[0].FullName -Raw|ConvertFrom-Json
 if(@($journal.objects).Count -ne 0 -or $journal.normalEnd -or $journal.stopIntent -or $journal.sealAcknowledged -or $journal.confirmedReady){throw 'Failed initialization invented capture completion'}
 if(@(Get-ChildItem -LiteralPath "$out/denied" -Recurse -Filter '*.m4s').Count){throw 'Failed initialization emitted media'}
 $beforeHash=(Get-FileHash -LiteralPath $journalFiles[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
 # Leave two available sessions: module probing can succeed, but three HHC encoders cannot.
 foreach($p in @($holders|Where-Object {-not $_.HasExited}|Select-Object -Last 2)){$p.Kill();$p.WaitForExit()}
 $partialSessions=[int](& nvidia-smi --query-gpu=encoder.stats.sessionCount --format=csv,noheader,nounits)
 if($partialSessions -ne $sessions-2){throw 'Two NVENC holder releases were not observed'}
 $partial=Invoke-Fixture 'partial'
 $partialLog=Get-Content -LiteralPath $partial.log -Raw
 if($partial.exitCode -ne 0 -or -not $partialLog.Contains('Dock failed to start') -or -not $partialLog.Contains('NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY') -or $partialLog.Contains('Test process failed: session_limit') -or $partialLog.Contains('[x264 encoder:')){throw 'Expected per-rendition NVENC initialization denial was not observed'}
 $partialFiles=@(Get-ChildItem -LiteralPath "$out/partial" -Recurse -Filter journal.json)
 if($partialFiles.Count -ne 1){throw 'Expected one retained partial-initialization journal'}
 $partialJournal=Get-Content -LiteralPath $partialFiles[0].FullName -Raw|ConvertFrom-Json
 if(@($partialJournal.objects).Count -ne 0 -or $partialJournal.normalEnd -or $partialJournal.stopIntent -or $partialJournal.sealAcknowledged -or $partialJournal.confirmedReady -or @(Get-ChildItem -LiteralPath "$out/partial" -Recurse -Filter '*.m4s').Count){throw 'Partial initialization invented media or completion'}
 $partialHash=(Get-FileHash -LiteralPath $partialFiles[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
 if([int](& nvidia-smi --query-gpu=encoder.stats.sessionCount --format=csv,noheader,nounits) -ne $partialSessions){throw 'Failed OBS initialization leaked NVENC sessions'}
} finally {
 foreach($p in $holders){if(-not $p.HasExited){$p.Kill();$p.WaitForExit()}}
}
if([int](& nvidia-smi --query-gpu=encoder.stats.sessionCount --format=csv,noheader,nounits) -ne 0){throw 'NVENC holders were not released'}
$healthy=Invoke-Fixture 'healthy'
if($healthy.exitCode -ne 0 -or -not (Get-Content -LiteralPath $healthy.log -Raw).Contains('Dock complete (success=true)')){throw 'Independent healthy local capture failed after resources released'}
if((Get-FileHash -LiteralPath $journalFiles[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $beforeHash){throw 'Failed-start journal changed during independent healthy test'}
if((Get-FileHash -LiteralPath $partialFiles[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $partialHash){throw 'Partial-initialization journal changed during independent healthy test'}
@{scope='Actual local OBS NVENC module-startup and per-rendition initialization denial plus independent healthy capture; not GPU removal, same-dock retry, CPU benchmark or E2E';sourceCommit=(& git rev-parse HEAD);fixtureSha256=(Get-FileHash -LiteralPath $fixture -Algorithm SHA256).Hash.ToLowerInvariant();activeSessionsAtDenial=$sessions;obsNvencCheckError='session_limit';probe=$denied;failedObs=$failure;failedJournalLocalId=$journal.localId;failedJournalSha256=$beforeHash;failedJournalUnchanged=$true;failedObjects=0;normalEnd=$false;confirmedReady=$false;cpuEncoderInitialized=$false;partialInitialization=@{obs=$partial;error='NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY';holderSessions=$partialSessions;localId=$partialJournal.localId;journalSha256=$partialHash;objects=0;journalUnchanged=$true;nvencSessionsReleased=$true};healthyObs=$healthy}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath "$out/result.json"
Write-Output 'PASS: native NVENC denial, no CPU initialization or false completion; resources released and independent local capture completed'
