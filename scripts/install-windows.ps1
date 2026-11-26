[CmdletBinding(SupportsShouldProcess)]
param([string]$ObsRoot, [string]$ReleaseTag)

$ErrorActionPreference = 'Stop'
$paths = @(
    'obs-plugins/64bit/hhc-obs-plugin.dll'
    'obs-plugins/64bit/hhc-upload-helper.exe'
    'data/obs-plugins/hhc-obs-plugin/qt-plugins/tls/qschannelbackend.dll'
)

function Assert-OrdinaryPath([string]$Path) {
    while ($Path) {
        $item = Get-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
        if ($item -and ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Symbolic links and junctions are not supported: $Path"
        }
        $Path = Split-Path -LiteralPath $Path
    }
}

function Assert-ObsClosed {
    $targetExe = Join-Path $ObsRoot 'bin/64bit/obs64.exe'
    $targetHelper = Join-Path $ObsRoot $paths[1]
    foreach ($process in Get-Process -Name obs64,hhc-upload-helper -ErrorAction SilentlyContinue) {
        if (-not $process.Path -or $process.Path -in @($targetExe, $targetHelper)) {
            throw 'Close OBS after its recording and HHC upload have finished, then run the installer again.'
        }
    }
}

if (-not [Environment]::Is64BitProcess -or $env:PROCESSOR_ARCHITECTURE -ne 'AMD64') {
    throw 'Run this installer with 64-bit Windows PowerShell on Windows x64.'
}

if ($ReleaseTag) {
    if ($ReleaseTag -notmatch '^v[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.-]+)?$') {
        throw 'Specify a version tag such as v0.2.1.'
    }
    if (-not $ObsRoot) { $ObsRoot = Join-Path $env:ProgramFiles 'obs-studio' }
    if (-not $PSCmdlet.ShouldProcess($ObsRoot, "Download and install HHC $ReleaseTag")) { return }

    $repository = 'HallelujahHomeChurch/hhc-obs-plugin'
    $release = Invoke-RestMethod -UseBasicParsing -Uri "https://api.github.com/repos/$repository/releases/tags/$ReleaseTag"
    $assets = @($release.assets | Where-Object {
        $_.state -eq 'uploaded' -and $_.name -cmatch '^hhc-obs-plugin-windows-x64(?:-platform-preview-[a-f0-9]{7})?\.zip$'
    })
    if ($release.draft -or $release.tag_name -cne $ReleaseTag -or $assets.Count -ne 1) {
        throw "Expected one standard Windows ZIP in HHC release $ReleaseTag."
    }
    $asset = $assets[0]
    $downloadUri = "https://github.com/$repository/releases/download/$ReleaseTag/$($asset.name)"
    if ($asset.browser_download_url -cne $downloadUri -or $asset.digest -notmatch '^sha256:([a-f0-9]{64})$') {
        throw 'Release ZIP SHA-256 or download URL is missing or invalid.'
    }
    $expectedHash = $Matches[1]
    $temp = (Resolve-Path -LiteralPath ([IO.Path]::GetTempPath())).ProviderPath
    Assert-OrdinaryPath $temp
    $stage = Join-Path $temp ("hhc-online-install-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $stage | Out-Null
    try {
        $archive = Join-Path $stage 'package.zip'
        Write-Host "Downloading HHC $ReleaseTag ..."
        Invoke-WebRequest -UseBasicParsing -Uri $downloadUri -OutFile $archive
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedHash) {
            throw 'Downloaded ZIP SHA-256 does not match the GitHub release.'
        }
        $package = Join-Path $stage 'package'
        Expand-Archive -LiteralPath $archive -DestinationPath $package
        $installer = Join-Path $package 'install-windows.ps1'
        Assert-OrdinaryPath $installer
        # Early preview ZIPs have the same manifest but predate the bundled installer.
        if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
            Copy-Item -LiteralPath $PSCommandPath -Destination $installer
        }
        & $installer -ObsRoot $ObsRoot
    } finally {
        $resolvedStage = (Resolve-Path -LiteralPath $stage).ProviderPath
        if ($resolvedStage -eq $stage -and $resolvedStage.StartsWith($temp.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
            Assert-OrdinaryPath $resolvedStage
            Remove-Item -LiteralPath $resolvedStage -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    return
}
$manifestPath = Join-Path $PSScriptRoot 'manifest.json'
Assert-OrdinaryPath $manifestPath
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.layout -ne 'legacy' -or $manifest.architecture -ne 'x64' -or $manifest.obs -ne '32.2.2') {
    throw 'This installer requires the standard Windows x64 HHC package for OBS 32.2.2.'
}
foreach ($path in $paths) {
    $source = Join-Path $PSScriptRoot $path
    Assert-OrdinaryPath $source
    $entries = @($manifest.files | Where-Object { $_.path -ceq $path })
    if ($entries.Count -ne 1 -or -not (Test-Path -LiteralPath $source -PathType Leaf) -or
        (Get-Item -LiteralPath $source).Length -ne $entries[0].bytes -or
        (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $entries[0].sha256) {
        throw "Package integrity check failed for $path. Extract a fresh copy of the entire ZIP."
    }
}

if (-not $ObsRoot) {
    $defaultRoot = Join-Path $env:ProgramFiles 'obs-studio'
    if (Test-Path -LiteralPath (Join-Path $defaultRoot 'bin/64bit/obs64.exe') -PathType Leaf) {
        $ObsRoot = (Read-Host "Press Enter to install to $defaultRoot, or enter your OBS folder").Trim().Trim('"')
        if (-not $ObsRoot) { $ObsRoot = $defaultRoot }
    } else {
        $ObsRoot = (Read-Host 'Enter the full path of your OBS Studio folder').Trim().Trim('"')
        if (-not $ObsRoot) { throw 'No OBS folder was selected.' }
    }
}
$ObsRoot = (Resolve-Path -LiteralPath $ObsRoot).ProviderPath
$obsExe = Join-Path $ObsRoot 'bin/64bit/obs64.exe'
Assert-OrdinaryPath $obsExe
if (-not (Test-Path -LiteralPath $obsExe -PathType Leaf)) { throw "OBS executable not found: $obsExe" }
$version = (Get-Item -LiteralPath $obsExe).VersionInfo
if ($version.FileMajorPart -ne 32 -or $version.FileMinorPart -ne 2 -or $version.FileBuildPart -ne 2) {
    throw 'This HHC package is verified for OBS Studio 32.2.2 only.'
}
foreach ($alternate in @(
    (Join-Path $env:ProgramData 'obs-studio/plugins/hhc-obs-plugin')
    (Join-Path $ObsRoot 'plugins/hhc-obs-plugin')
    (Join-Path $ObsRoot 'data/plugins/hhc-obs-plugin')
)) {
    if (Test-Path -LiteralPath $alternate) { throw "Another HHC install layout exists at $alternate. See the package README." }
}
foreach ($path in $paths) {
    $target = Join-Path $ObsRoot $path
    Assert-OrdinaryPath $target
    if (Test-Path -LiteralPath $target -PathType Container) { throw "A directory is in the way: $target" }
}
Assert-ObsClosed
if (-not $PSCmdlet.ShouldProcess($ObsRoot, 'Install the three HHC plugin files')) { return }

$backupRoot = Join-Path $ObsRoot '.hhc-plugin-backups'
Assert-OrdinaryPath $backupRoot
$protectedRoot = [IO.Path]::GetFullPath($env:ProgramFiles).TrimEnd('\') + '\'
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($ObsRoot.StartsWith($protectedRoot, [StringComparison]::OrdinalIgnoreCase) -and -not $isAdmin) {
    Write-Host "Windows may ask for administrator permission to install into $ObsRoot."
    $arguments = '-NoProfile -ExecutionPolicy RemoteSigned -File "{0}" -ObsRoot "{1}"' -f $PSCommandPath, $ObsRoot
    $windowsPowerShell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
    $child = Start-Process -FilePath $windowsPowerShell -ArgumentList $arguments -Verb RunAs -WindowStyle Normal -Wait -PassThru
    if ($child.ExitCode -ne 0) { throw "Elevated installation failed (exit $($child.ExitCode))." }
    Write-Host "HHC plugin installed. Previous files, if any, are under $backupRoot."
    return
}

$backup = Join-Path $backupRoot ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N'))
$previous = @()
foreach ($path in $paths) {
    $target = Join-Path $ObsRoot $path
    if (Test-Path -LiteralPath $target) {
        $saved = Join-Path $backup $path
        New-Item -ItemType Directory -Path (Split-Path $saved) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $saved
        $previous += $path
    }
}

Assert-ObsClosed
$attempted = @()
try {
    foreach ($path in $paths) {
        $target = Join-Path $ObsRoot $path
        New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
        $attempted += $path
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $path) -Destination $target -Force
        $expected = ($manifest.files | Where-Object { $_.path -ceq $path }).sha256
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected) {
            throw "Installed file verification failed: $path"
        }
    }
} catch {
    $failure = $_.Exception.Message
    $restoreFailed = @()
    foreach ($path in $attempted) {
        try {
            $target = Join-Path $ObsRoot $path
            if ($path -in $previous) {
                Copy-Item -LiteralPath (Join-Path $backup $path) -Destination $target -Force
            } elseif (Test-Path -LiteralPath $target -PathType Leaf) {
                Remove-Item -LiteralPath $target -Force
            }
        } catch { $restoreFailed += $path }
    }
    if ($restoreFailed.Count) { throw "$failure Rollback failed for $($restoreFailed -join ', '). Backups: $backup" }
    throw "$failure Previous plugin files were restored."
}
[pscustomobject]@{Installed = $ObsRoot; Backup = $(if ($previous.Count) { $backup })}
