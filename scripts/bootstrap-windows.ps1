param([string]$CMake = 'cmake')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
Set-Location $taskRoot
if(-not (Get-Command $CMake -ErrorAction SilentlyContinue)) {
 $CMake='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
function Invoke-Checked([scriptblock]$Action) { & $Action; if($LASTEXITCODE -ne 0){throw "Command failed ($LASTEXITCODE)"} }
New-Item -ItemType Directory -Force .deps | Out-Null
$obsCommit='ba2f32bdf791005443988a4955e963663e16b1ed'
if(-not(Test-Path .deps/obs-studio/.git)) { Invoke-Checked {git clone --depth 1 --branch 32.2.2 https://github.com/obsproject/obs-studio.git .deps/obs-studio} }
if((git -C .deps/obs-studio rev-parse HEAD) -ne $obsCommit){throw 'Unexpected OBS source revision'}
$packages=@(
 @{Name='deps';Dir='prebuilt';File='windows-deps-2026-07-15-x64.zip';Hash='6f90e9598fa10cff5ad23cdcfae49b87868c07bf896b02cd464582b4ce2f2ba9'},
 @{Name='qt';Dir='qt';File='windows-deps-qt6-2026-07-15-x64.zip';Hash='7c7f985711d80467bdc1795b6592275a27d5b0e5a2c7a61db1f2c1d08d6a5579'}
)
foreach($p in $packages) {
 $archive=Join-Path '.deps' ($p.Name+'.zip')
 if(-not(Test-Path $archive)){Invoke-WebRequest ('https://github.com/obsproject/obs-deps/releases/download/2026-07-15/'+$p.File) -OutFile $archive}
 if((Get-FileHash $archive -Algorithm SHA256).Hash -ne $p.Hash){throw ('Dependency hash mismatch: '+$p.Name)}
 $destination=Join-Path '.deps' $p.Dir
 if(-not(Test-Path $destination)){Expand-Archive $archive $destination}
}
# Same libobs/frontend Development component route as official obs-plugintemplate.
$prefix="$taskRoot/.deps/prebuilt;$taskRoot/.deps/qt"
Invoke-Checked {& $CMake -S .deps/obs-studio -B .deps/obs-build -G 'Visual Studio 17 2022' -A x64 '-DENABLE_PLUGINS=OFF' '-DENABLE_FRONTEND=OFF' '-DENABLE_SCRIPTING=OFF' '-DOBS_VERSION_OVERRIDE=32.2.2' "-DCMAKE_PREFIX_PATH=$prefix"}
Invoke-Checked {& $CMake --build .deps/obs-build --target obs-frontend-api --config RelWithDebInfo --parallel 4}
Invoke-Checked {& $CMake --install .deps/obs-build --component Development --config RelWithDebInfo --prefix .deps/sdk}
Invoke-Checked {& $CMake -S . -B build -G 'Visual Studio 17 2022' -A x64}
