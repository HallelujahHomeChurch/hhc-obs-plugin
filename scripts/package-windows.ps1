param([string]$Configuration='RelWithDebInfo', [string]$Destination='artifacts/packages')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
Push-Location $taskRoot
try {
    $revision=git rev-parse HEAD
    if ($LASTEXITCODE -ne 0) {throw 'Cannot identify source revision'}
    if (git status --porcelain --untracked-files=normal) {throw 'Commit source before packaging'}
    $dll=Join-Path $taskRoot "build/$Configuration/hhc-obs-plugin.dll"
    if (-not (Test-Path -LiteralPath $dll)) {throw 'Build plugin before packaging'}
    $base="hhc-obs-plugin-windows-x64-local-preview-$($revision.Substring(0,7))"
    $destinationRoot=[IO.Path]::GetFullPath((Join-Path $taskRoot $Destination))
    $staging=Join-Path $destinationRoot $base
    if ((Test-Path -LiteralPath $staging) -or (Test-Path -LiteralPath "$staging.zip")) {throw 'Package revision already exists; preserve it'}
    New-Item -ItemType Directory "$staging/obs-plugins/64bit" -Force | Out-Null
    Copy-Item -LiteralPath $dll -Destination "$staging/obs-plugins/64bit/hhc-obs-plugin.dll"
    Copy-Item -LiteralPath docs/windows-local-preview.md -Destination "$staging/README.md"
    Copy-Item -LiteralPath LICENSE -Destination "$staging/LICENSE"
    $files=@(Get-ChildItem -LiteralPath $staging -Recurse -File | ForEach-Object {
        [ordered]@{path=[IO.Path]::GetRelativePath($staging,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower()}
    })
    [ordered]@{kind='local-preview-not-e2e';sourceCommit=$revision;obs='32.2.2';architecture='x64';fpsNum=30000;fpsDen=1001;signed=$false;files=$files} |
        ConvertTo-Json -Depth 6 | Set-Content "$staging/manifest.json" -Encoding utf8
    Compress-Archive -Path "$staging/*" -DestinationPath "$staging.zip"
    $hash=(Get-FileHash -LiteralPath "$staging.zip" -Algorithm SHA256).Hash.ToLower()
    "$hash  $base.zip" | Set-Content "$staging.zip.sha256" -Encoding ascii
    [ordered]@{archive="$staging.zip";sha256=$hash;commit=$revision}|ConvertTo-Json
} finally {Pop-Location}
