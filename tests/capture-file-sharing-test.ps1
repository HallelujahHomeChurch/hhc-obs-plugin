# Developer-only Windows queue write denial; no fault-enabled library or HTTP.
param(
 [ValidatePattern('^[a-zA-Z0-9_-]{1,64}$')][string]$RunName=('obs-write-denial-'+(Get-Date -Format yyyyMMdd-HHmmss)),
 [ValidateSet('sharing','acl')][string]$Failure='sharing',
 [ValidateSet('portable','portable-concurrent')][string]$Portable='portable'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $root
$out=Join-Path $root "artifacts/$RunName"
if(Test-Path -LiteralPath $out){throw 'Preserve existing test evidence'}
$null=New-Item -ItemType Directory -Path $out
$portableRoot=Join-Path $root ".deps/$Portable"
$exe=Join-Path $portableRoot 'bin/64bit/obs64.exe'
if(@(Get-CimInstance Win32_Process -Filter "Name='obs64.exe'"|Where-Object ExecutablePath -EQ $exe).Count){throw 'Owned fixture OBS active'}
if(Test-Path -LiteralPath (Join-Path $portableRoot 'obs-plugins/64bit/hhc-obs-plugin.dll')){throw 'Use only the local developer fixture'}
Copy-Item -LiteralPath (Join-Path $root 'build/RelWithDebInfo/hhc-obs-fixture.dll') -Destination (Join-Path $portableRoot 'obs-plugins/64bit/hhc-obs-fixture.dll')
Remove-Item Env:QT_PLUGIN_PATH,Env:QT_SCALE_FACTOR,Env:HHC_FIXTURE_ORIGINAL_RECORDING,Env:HHC_FIXTURE_NATIVEDRIVE,Env:HHC_FIXTURE_RECOVER_ID,Env:HHC_FIXTURE_EXPECT_ABORT,Env:HHC_FIXTURE_NATIVE_SMOKE,Env:HHC_FIXTURE_STUDIO,Env:HHC_FIXTURE_SOURCE_MUTED -ErrorAction SilentlyContinue
$queue=Join-Path $out 'queue'
$env:HHC_FIXTURE_OUTPUT=$queue
$env:HHC_FIXTURE_SECONDS='60'
$env:HHC_FIXTURE_DOCK='1'
$env:HHC_FIXTURE_AUDIO_TRACK='1'
$env:HHC_FIXTURE_SOURCE_TRACK='1'
$env:HHC_FIXTURE_TONE_HZ='0'
$started=Get-Date
$process=Start-Process -FilePath $exe -ArgumentList '--portable','--multi','--disable-updater','--disable-missing-files-check' -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
$null=$process.Handle
@{pid=$process.Id;exe=$exe;started=(Get-Date).ToString('o')}|ConvertTo-Json|Set-Content -LiteralPath "$out/process.json"
$lock=$null
$originalAcl=$null
$aclTarget=$null
$accessDenied=$false
try {
 $deadline=(Get-Date).AddSeconds(50)
 $journal=$null
 while((Get-Date) -lt $deadline -and -not $process.HasExited){
  $files=@(Get-ChildItem -LiteralPath $queue -Recurse -Filter journal.json -ErrorAction SilentlyContinue)
  if($files.Count -eq 1){
   $j=Get-Content -LiteralPath $files[0].FullName -Raw|ConvertFrom-Json
   if(@($j.objects).Count -eq 6){$journal=$files[0];break}
  }
  Start-Sleep -Milliseconds 100
 }
 if(-not $journal){throw 'First three closed fragments were not checkpointed; preserve evidence'}
 $before=Get-Content -LiteralPath $journal.FullName -Raw|ConvertFrom-Json
 foreach($o in $before.objects){
  if((Get-FileHash -LiteralPath (Join-Path $journal.DirectoryName $o.path) -Algorithm SHA256).Hash.ToLower() -ne $o.sha256){throw 'Initial closed hash mismatch'}
 }
 $blocked=Join-Path $journal.DirectoryName '1080p/seg-000001.m4s'
 if(Test-Path -LiteralPath $blocked){throw 'Next fragment already exists; preserve it'}
 if($Failure -eq 'sharing'){
  $lock=[IO.File]::Open($blocked,[IO.FileMode]::CreateNew,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
 }else{
  $aclTarget=[IO.Path]::GetFullPath((Join-Path $journal.DirectoryName '1080p'))
  if(-not $aclTarget.StartsWith([IO.Path]::GetFullPath($out)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'ACL target outside test output'}
  $originalAcl=Get-Acl -LiteralPath $aclTarget
  $originalAcl.Sddl|Set-Content -LiteralPath "$out/acl-before.txt"
  $deniedAcl=Get-Acl -LiteralPath $aclTarget
  $rule=[Security.AccessControl.FileSystemAccessRule]::new([Security.Principal.WindowsIdentity]::GetCurrent().User,[Security.AccessControl.FileSystemRights]::CreateFiles,[Security.AccessControl.AccessControlType]::Deny)
  $deniedAcl.AddAccessRule($rule)
  Set-Acl -LiteralPath $aclTarget -AclObject $deniedAcl
  try{[IO.File]::WriteAllText((Join-Path $aclTarget 'denial-probe.tmp'),'test')}catch{
   if($_.Exception.InnerException -is [UnauthorizedAccessException]){$accessDenied=$true}else{throw}
  }
  if(-not $accessDenied){throw 'NTFS access denial was not observed'}
 }
 $blockedAt=(Get-Date).ToString('o')
 if(-not $process.WaitForExit(95000)){throw 'Observation timed out; do not restart or terminate the capture'}
 $process.WaitForExit()
 $after=Get-Content -LiteralPath $journal.FullName -Raw|ConvertFrom-Json
 $inventory=Get-Content -LiteralPath (Join-Path $journal.DirectoryName 'inventory.json') -Raw|ConvertFrom-Json
 if($process.ExitCode -ne 0 -or $after.normalEnd -or $after.confirmedReady -or $after.sealAcknowledged -or $inventory.normalEnd -or $inventory.stopReason -ne 2){throw 'Queue write denial did not finish as retained EncoderFailure'}
 foreach($o in $before.objects){
  if((Get-FileHash -LiteralPath (Join-Path $journal.DirectoryName $o.path) -Algorithm SHA256).Hash.ToLower() -ne $o.sha256){throw 'Previously closed media changed'}
  if(-not @($after.objects|Where-Object {$_.path -eq $o.path -and $_.sha256 -eq $o.sha256}).Count){throw 'Previously closed object absent from final journal'}
 }
 if(@($after.objects|Where-Object path -EQ '1080p/seg-000001.m4s').Count){throw 'Denied placeholder must never enter immutable journal'}
 $logs=Join-Path $portableRoot 'config/obs-studio/logs'
 $log=Get-ChildItem -LiteralPath $logs -Filter '*.txt'|Where-Object LastWriteTime -GE $started|Sort-Object LastWriteTime -Descending|Select-Object -First 1
 if(-not $log -or -not (Get-Content -LiteralPath $log.FullName -Raw).Contains('Dock complete (success=false)')){throw 'Expected actual dock failure absent'}
 $result=@{scope='Actual local OBS/NVENC queue write denial; not physical disk failure, platform permission revocation or E2E';failure=$Failure;accessDeniedProbeConfirmed=$accessDenied;sourceCommit=(& git rev-parse HEAD);fixtureSha256=(Get-FileHash -LiteralPath (Join-Path $portableRoot 'obs-plugins/64bit/hhc-obs-fixture.dll') -Algorithm SHA256).Hash.ToLower();pid=$process.Id;exitCode=$process.ExitCode;expectedCaptureFailure=$true;blockedAt=$blockedAt;completed=(Get-Date).ToString('o');normalEnd=$after.normalEnd;stopIntent=$after.stopIntent;confirmedReady=$after.confirmedReady;sealAcknowledged=$after.sealAcknowledged;stopReason=$inventory.stopReason;previousObjects=@($before.objects).Count;retainedObjects=@($after.objects).Count;previousHashesRetained=$true;blockedObjectDeclared=$false;log=$log.FullName}
} finally {
 if($originalAcl){Set-Acl -LiteralPath $aclTarget -AclObject $originalAcl}
 if($lock){$lock.Dispose()}
}
if($originalAcl -and (Get-Acl -LiteralPath $aclTarget).Sddl -ne $originalAcl.Sddl){throw 'Original NTFS ACL was not restored'}
$result.aclRestored=[bool]$originalAcl
$result|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$out/result.json"
Write-Output 'PASS: actual queue write denied; capture incomplete and previous closed hashes retained; original ACL restored where changed'
