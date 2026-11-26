& {
    $ErrorActionPreference = 'Stop'
    $env:PSModulePath = Join-Path $PSHOME 'Modules'
    $installer = Join-Path ([IO.Path]::GetTempPath()) ("hhc-install-$([guid]::NewGuid()).ps1")
    try {
        $registered = Get-Item -LiteralPath 'HKLM:\SOFTWARE\OBS Studio' -ErrorAction SilentlyContinue
        $candidates = @(Join-Path $env:ProgramFiles 'obs-studio')
        if ($registered) { $candidates += $registered.GetValue('') }
        $roots = @($candidates | Where-Object {
            $_ -and (Test-Path -LiteralPath (Join-Path $_ 'bin/64bit/obs64.exe') -PathType Leaf)
        } | ForEach-Object {
            (Resolve-Path -LiteralPath $_).ProviderPath.TrimEnd('\')
        } | Select-Object -Unique)
        if ($roots.Count -ne 1) {
            throw 'Expected one installed OBS Studio. For a portable or ambiguous install, use Install-HHC.cmd from the ZIP.'
        }

        $source = 'https://raw.githubusercontent.com/HallelujahHomeChurch/hhc-obs-plugin/eb57e5042fd86bf99b2a72418eb456361217b0f2/scripts/install-windows.ps1'
        Invoke-WebRequest -UseBasicParsing -Uri $source -OutFile $installer
        if ((Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash -ne '20cceb8eaeede841041457d5ffe04f8044c9f64cc2c2e38411b1dd76a7f50255') {
            throw 'Installer SHA-256 mismatch.'
        }
        $windowsPowerShell = Join-Path $PSHOME 'powershell.exe'
        & $windowsPowerShell -NoProfile -ExecutionPolicy RemoteSigned -File $installer -ReleaseTag v0.2.1 -ObsRoot $roots[0]
        if ($LASTEXITCODE -ne 0) { throw 'HHC installation did not complete.' }
    } catch {
        [Console]::Error.WriteLine($_.Exception.Message)
        exit 1
    } finally {
        Remove-Item -LiteralPath $installer -Force -ErrorAction SilentlyContinue
    }
    exit 0
}
