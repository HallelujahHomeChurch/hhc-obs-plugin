param(
 [Parameter(Mandatory)][int]$ObsPid,
 [Parameter(Mandatory)][string]$Output,
 [Parameter(Mandatory)][string]$ProducerCommit,
 [ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$RunName='F1-L',
 [int]$Seconds=9000,
 [switch]$QueueCapture,
 [switch]$OriginalRecording,
 [switch]$SkipPackage
)
$ErrorActionPreference='Stop'
function Invoke-PythonCheck([string[]]$ScriptArguments,[string]$Log) {
 $ErrorActionPreference='Continue'
 $global:LASTEXITCODE=1
 & python @ScriptArguments *> $Log
 return $global:LASTEXITCODE
}
$root=Split-Path $PSScriptRoot -Parent
$Output=[IO.Path]::GetFullPath($Output)
if(-not $Output.StartsWith('\\?\')) {
 if($Output.StartsWith('\\')) {$Output='\\?\UNC\'+$Output.Substring(2)}
 else {$Output='\\?\'+$Output}
}
$prefix=Join-Path $root "artifacts/$RunName"
$report="$prefix-resource-samples.jsonl"
if(Test-Path -LiteralPath $report){throw 'Run already exists; preserve evidence'}
$started=Get-Date
while((Get-Date)-$started -lt [TimeSpan]::FromSeconds($Seconds+1800)) {
 $process=Get-Process -Id $ObsPid -ErrorAction SilentlyContinue
 if(-not $process){break}
 $gpu=& nvidia-smi --query-gpu=utilization.gpu,utilization.encoder,memory.used,temperature.gpu --format=csv,noheader,nounits
 $sample=[ordered]@{at=(Get-Date).ToUniversalTime().ToString('o');pid=$ObsPid;cpuSeconds=$process.TotalProcessorTime.TotalSeconds;workingSetBytes=$process.WorkingSet64;privateBytes=$process.PrivateMemorySize64;gpu=$gpu;diskFree=(Get-PSDrive C).Free;closedSegments=@(Get-ChildItem -LiteralPath $Output -Recurse -Filter '*.m4s' -ErrorAction SilentlyContinue | Where-Object FullName -NotMatch '[\\/]staging[\\/]').Count}
 $sample|ConvertTo-Json -Compress|Add-Content -LiteralPath $report
 Start-Sleep -Seconds 30
}
$media=$Output
if($QueueCapture){
 $inventories=@(Get-ChildItem -LiteralPath $Output -Recurse -Filter inventory.json)
 if($inventories.Count -ne 1){'FAILED: expected one finalized capture'|Set-Content "$prefix-status.txt";exit 1}
 $media=$inventories[0].DirectoryName
 $journal=Get-Content -LiteralPath ([IO.Path]::Combine($media,'journal.json')) -Raw|ConvertFrom-Json
 if(-not $journal.normalEnd -or -not $journal.stopIntent -or $journal.confirmedReady -or $journal.sealAcknowledged){'FAILED: incorrect local journal state'|Set-Content "$prefix-status.txt";exit 1}
}
if(-not(Test-Path -LiteralPath ([IO.Path]::Combine($media,'inventory.json')))){'FAILED: no final inventory; preserve files'|Set-Content "$prefix-status.txt";exit 1}
if((Invoke-PythonCheck @((Join-Path $root 'scripts/verify-media.py'),$media,'--seconds',"$Seconds") "$prefix-validation.log") -ne 0){'FAILED local validation; preserve files'|Set-Content "$prefix-status.txt";exit 1}
if($OriginalRecording -and (Invoke-PythonCheck @((Join-Path $root 'scripts/verify-original-recording.py'),$Output,'--seconds',"$Seconds") "$prefix-original-recording.log") -ne 0){'FAILED original recording validation; preserve files'|Set-Content "$prefix-status.txt";exit 1}
if(-not $SkipPackage){
 $dest=Join-Path $root "artifacts/$RunName-handoff-01"
 if((Invoke-PythonCheck @((Join-Path $root 'scripts/prepare-handoff.py'),$media,$dest,'--seconds',"$Seconds",'--commit',$ProducerCommit) "$prefix-package.log") -ne 0){'FAILED packaging; preserve files'|Set-Content "$prefix-status.txt";exit 1}
}
[ordered]@{producerCommit=$ProducerCommit;media=$media;seconds=$Seconds;evidence='local actual OBS only; not E2E';completed=(Get-Date).ToString('o')}|ConvertTo-Json|Set-Content "$prefix-result.json"
'LOCAL_QA_COMPLETE; not E2E acceptance'|Set-Content "$prefix-status.txt"
