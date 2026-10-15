$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$taskId=[guid]::NewGuid().ToString('N')
$taskLong=[IO.Path]::Combine($taskRoot,'artifacts',"watcher-path-$taskId")+'\'+('a'*100)+'\'+('b'*100)
$taskExtended='\\?\'+$taskLong
[IO.Directory]::CreateDirectory($taskExtended)|Out-Null
$taskAst=[Management.Automation.Language.Parser]::ParseFile("$taskRoot/scripts/watch-fixture.ps1",[ref]$null,[ref]$null)
$taskFunction=$taskAst.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-PythonCheck'},$true)
. ([ScriptBlock]::Create($taskFunction.Extent.Text))
foreach($taskExit in @(0,7)) {
 $taskActual=Invoke-PythonCheck @('-c',"import sys; sys.exit($taskExit)") "$taskRoot/artifacts/watcher-exit-$taskId-$taskExit.log"
 if($taskActual -ne $taskExit){throw "Python exit status changed: expected$taskExit actual$taskActual"}
}
[IO.File]::WriteAllText([IO.Path]::Combine($taskExtended,'inventory.json'),'{"normalEnd":true,"fpsNum":30000,"fpsDen":1001,"objects":[]}')
[IO.File]::WriteAllText([IO.Path]::Combine($taskExtended,'journal.json'),'{"normalEnd":true,"stopIntent":true,"confirmedReady":false,"sealAcknowledged":false}')
if(Get-Process -Id 2147483647 -ErrorAction SilentlyContinue){throw 'Test PID unexpectedly exists'}
$taskIndex=0
foreach($taskOutput in @($taskLong,$taskExtended)) {
 $taskName="watcher-path-$taskId-$taskIndex"
 $taskPrefix=[IO.Path]::Combine($taskRoot,"artifacts/$taskName")
 & powershell.exe -NoProfile -File "$taskRoot/scripts/watch-fixture.ps1" -ObsPid 2147483647 -Output $taskOutput -ProducerCommit developer-test -RunName $taskName -Seconds 1 -QueueCapture -SkipPackage
 if($LASTEXITCODE -ne 1 -or (Get-Content -LiteralPath "$taskPrefix-status.txt" -Raw).Trim() -ne 'FAILED local validation; preserve files') {throw 'Watcher failed before media QA or ignored validation failure'}
 $taskLog=Get-Content -LiteralPath "$taskPrefix-validation.log" -Raw
 if($taskLog -notmatch 'FileNotFoundError' -or $taskLog -notmatch '1080p[\\/]+index\.m3u8') {throw 'Expected retained empty fixture to reach playlist check'}
 $taskIndex++
}
'PASS: Windows PowerShell preserves Python exit codes and long-path media QA failure status'
