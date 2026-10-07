param([Parameter(Mandatory)][int]$ObsPid,[Parameter(Mandatory)][string]$Output,[Parameter(Mandatory)][string]$ProducerCommit)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$report=Join-Path $root 'artifacts/F1-L-resource-samples.jsonl'
$started=Get-Date
while((Get-Date)-$started -lt [TimeSpan]::FromHours(3)) {
 $process=Get-Process -Id $ObsPid -ErrorAction SilentlyContinue
 if(-not $process){break}
 $gpu=& nvidia-smi --query-gpu=utilization.gpu,utilization.encoder,memory.used,temperature.gpu --format=csv,noheader,nounits
 $sample=[ordered]@{at=(Get-Date).ToUniversalTime().ToString('o');pid=$ObsPid;cpuSeconds=$process.TotalProcessorTime.TotalSeconds;workingSetBytes=$process.WorkingSet64;privateBytes=$process.PrivateMemorySize64;gpu=$gpu;diskFree=(Get-PSDrive C).Free;closedSegments=@(Get-ChildItem -LiteralPath $Output -Recurse -Filter '*.m4s' -ErrorAction SilentlyContinue | Where-Object FullName -NotMatch '[\\/]staging[\\/]').Count}
 $sample|ConvertTo-Json -Compress|Add-Content -LiteralPath $report
 Start-Sleep -Seconds 30
}
if(-not(Test-Path -LiteralPath (Join-Path $Output 'inventory.json'))){'FAILED: no final inventory; preserve files'|Set-Content (Join-Path $root 'artifacts/F1-L-status.txt');exit 1}
& python (Join-Path $root 'scripts/verify-media.py') $Output --seconds 9000 *> (Join-Path $root 'artifacts/F1-L-validation.log')
if($LASTEXITCODE -ne 0){'FAILED local validation; preserve files'|Set-Content (Join-Path $root 'artifacts/F1-L-status.txt');exit 1}
$dest=Join-Path $root 'artifacts/F1-L-handoff-01'
& python (Join-Path $root 'scripts/prepare-handoff.py') $Output $dest --seconds 9000 --commit $ProducerCommit *> (Join-Path $root 'artifacts/F1-L-package.log')
if($LASTEXITCODE -ne 0){'FAILED packaging; preserve files'|Set-Content (Join-Path $root 'artifacts/F1-L-status.txt');exit 1}
'LOCAL_QA_AND_PACKAGE_COMPLETE; shared access and Mac receipt pending'|Set-Content (Join-Path $root 'artifacts/F1-L-status.txt')
