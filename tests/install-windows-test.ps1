param([string]$Installer = (Join-Path $PSScriptRoot '../scripts/install-windows.ps1'))
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Installer -PathType Leaf)) { throw 'Installer entry point is missing' }
$root = Join-Path ([IO.Path]::GetTempPath()) ("hhc-install-test-" + [guid]::NewGuid())
$package = Join-Path $root 'extracted ZIP'
$obs = Join-Path $root 'Portable OBS'
$exe = Join-Path $obs 'bin/64bit/obs64.exe'
$paths = @(
    'obs-plugins/64bit/hhc-obs-plugin.dll'
    'obs-plugins/64bit/hhc-upload-helper.exe'
    'data/obs-plugins/hhc-obs-plugin/qt-plugins/tls/qschannelbackend.dll'
)
New-Item -ItemType Directory -Path (Split-Path $exe) -Force | Out-Null
try {
    Add-Type -OutputAssembly $exe -OutputType ConsoleApplication -TypeDefinition @'
using System.Threading;
using System.Reflection;
[assembly: AssemblyFileVersion("32.2.2.0")]
public class FakeObs {
    public static void Main() { Thread.Sleep(30000); }
}
'@
    New-Item -ItemType Directory -Path $package | Out-Null
    Copy-Item -LiteralPath $Installer -Destination (Join-Path $package 'install-windows.ps1')
    $entries = foreach ($path in $paths) {
        $file = Join-Path $package $path
        New-Item -ItemType Directory -Path (Split-Path $file) -Force | Out-Null
        Set-Content -LiteralPath $file -Value "new $path" -Encoding Ascii
        @{path = $path; bytes = (Get-Item -LiteralPath $file).Length; sha256 = (Get-FileHash -LiteralPath $file).Hash}
    }
    @{layout = 'legacy'; architecture = 'x64'; obs = '32.2.2'; files = @($entries)} |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $package 'manifest.json')
    $installedDll = Join-Path $obs $paths[0]
    New-Item -ItemType Directory -Path (Split-Path $installedDll) -Force | Out-Null
    Set-Content -LiteralPath $installedDll -Value 'previous DLL'
    $oldHash = (Get-FileHash -LiteralPath $installedDll).Hash
    $profile = Join-Path $obs 'profile-sentinel.ini'
    Set-Content -LiteralPath $profile -Value 'settings'
    $profileHash = (Get-FileHash -LiteralPath $profile).Hash
    $script = Join-Path $package 'install-windows.ps1'

    $running = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
    try {
        $refused = $false
        try { & $script -ObsRoot $obs | Out-Null } catch { $refused = $_.Exception.Message -match 'Close OBS' }
        if (-not $refused -or (Get-FileHash -LiteralPath $installedDll).Hash -ne $oldHash) { throw 'Running target OBS must prevent writes' }
    } finally { Stop-Process -Id $running.Id; $running.WaitForExit() }

    $tls = Join-Path $package $paths[2]
    $valid = [IO.File]::ReadAllBytes($tls)
    Set-Content -LiteralPath $tls -Value 'broken'
    $refused = $false
    try { & $script -ObsRoot $obs | Out-Null } catch { $refused = $_.Exception.Message -match 'integrity check' }
    if (-not $refused -or (Get-FileHash -LiteralPath $installedDll).Hash -ne $oldHash) { throw 'Altered package must prevent writes' }
    [IO.File]::WriteAllBytes($tls, $valid)

    $blockedParent = Join-Path $obs 'data'
    Set-Content -LiteralPath $blockedParent -Value 'blocked destination directory'
    $restored = $false
    try { & $script -ObsRoot $obs | Out-Null } catch { $restored = $_.Exception.Message -match 'Previous plugin files were restored' }
    if (-not $restored -or (Get-FileHash -LiteralPath $installedDll).Hash -ne $oldHash -or
        (Test-Path -LiteralPath (Join-Path $obs $paths[1]))) { throw 'Partial copy must roll back installed files' }
    Remove-Item -LiteralPath $blockedParent

    $result = & $script -ObsRoot $obs
    foreach ($entry in $entries) {
        if ((Get-FileHash -LiteralPath (Join-Path $obs $entry.path)).Hash -ne $entry.sha256) { throw "Bad install: $($entry.path)" }
    }
    if ((Get-FileHash -LiteralPath (Join-Path $result.Backup $paths[0])).Hash -ne $oldHash) { throw 'Previous plugin backup differs' }
    if ((Get-FileHash -LiteralPath $profile).Hash -ne $profileHash) { throw 'Profile changed' }
    Write-Output 'PASS: running OBS and tampered packages refused, partial install rolled back, install hashes and backup verified, profile untouched'

    $launcher = Join-Path $package 'Install-HHC.cmd'
    Copy-Item -LiteralPath (Join-Path (Split-Path $Installer) 'Install-HHC.cmd') -Destination $launcher
    $previousModulePath = $env:PSModulePath
    try {
        $env:PSModulePath = Join-Path $root 'incompatible parent shell modules'
        @($obs, '') | & $launcher | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'Double-click launcher failed with inherited module paths' }
    } finally { $env:PSModulePath = $previousModulePath }
    Write-Output 'PASS: launcher accepts an OBS path with spaces and uses Windows PowerShell system modules'
} finally {
    $resolved = (Resolve-Path -LiteralPath $root).ProviderPath
    $tmp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if ($resolved -eq $root -and $resolved.StartsWith($tmp, [StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
