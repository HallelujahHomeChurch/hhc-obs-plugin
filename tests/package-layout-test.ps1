param([string]$PackageRoot='artifacts/packages', [string]$RuntimeBin='.deps/qt/bin')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
$revision=git -C $taskRoot rev-parse HEAD
$base="hhc-obs-plugin-windows-x64-platform-preview-$($revision.Substring(0,7))"
foreach ($layout in @('legacy','modern')) {
    $suffix=if($layout -eq 'modern'){'-modern'}else{''}
    $package=Join-Path $taskRoot "$PackageRoot/$base$suffix"
    $manifest=Get-Content -LiteralPath "$package/manifest.json" -Raw | ConvertFrom-Json
    if($manifest.sourceCommit -ne $revision -or $manifest.layout -ne $layout){throw 'Incorrect package manifest'}
    foreach($entry in $manifest.files){
        if((Get-FileHash -LiteralPath (Join-Path $package $entry.path) -Algorithm SHA256).Hash.ToLower() -ne $entry.sha256){throw "Hash mismatch: $($entry.path)"}
    }
    $module=if($layout -eq 'modern'){"$package/hhc-obs-plugin"}else{"$package/obs-plugins/64bit"}
    $data=if($layout -eq 'modern'){"$module/data"}else{"$package/data/obs-plugins/hhc-obs-plugin"}
    foreach($file in @("$module/hhc-obs-plugin.dll","$module/hhc-upload-helper.exe","$data/qt-plugins/tls/qschannelbackend.dll")){
        if(-not(Test-Path -LiteralPath $file -PathType Leaf)){throw "Missing packaged file: $file"}
    }
    $originalPath=$env:PATH
    try {
        $env:PATH="$(Join-Path $taskRoot $RuntimeBin);$originalPath"
        $actual=& "$module/hhc-upload-helper.exe" --build-info
        if($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $revision){throw "Packaged helper failed: $layout"}
    } finally {$env:PATH=$originalPath}
    Write-Output "PASS: $layout manifest, hashes and packaged helper launch"
}
