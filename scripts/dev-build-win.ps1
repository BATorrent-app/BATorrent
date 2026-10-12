# Local dev build on Windows: the fork (BAT_LIBTORRENT_SOURCE=ON), Ninja on
# every core, API keys from .env. The Windows side of dev-build-fork.sh.
#
# Needs: VS 2022 Build Tools (C++), Qt 6.7 msvc2019_64 with Multimedia,
# vcpkg with libtorrent:x64-windows and zlib:x64-windows (for Boost/OpenSSL).
#
# Usage: .\scripts\dev-build-win.ps1 [-Tests] [-Clean] [-Jobs N]
#   -Tests  also build and run the Catch2 suite (BAT_BUILD_TESTS=ON)
#   -Clean  wipe the build dir first
#   -Jobs   compilers at once. Default: one per 4 GB of RAM free right now (a
#           cl.exe on the Boost-heavy libtorrent sources can take 1.5-2 GB), at
#           most half the cores, so the box stays usable. Ninja's own default
#           (cores + 2) ran 26 on a 32 GB box and paged it into a bugcheck.
#   -Target build just this target (e.g. test_unit) and stop: no deploy, no ctest
param([switch]$Tests, [switch]$Clean, [int]$Jobs = 0, [string]$Target = '')
if ($Jobs -le 0) {
    $freeGb = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
    $Jobs = [math]::Max(2, [math]::Min([Environment]::ProcessorCount / 2, [math]::Floor($freeGb / 4)))
}
# Not 'Stop': PowerShell 5.1 turns any native stderr line (vcvars, cmake
# warnings) into a terminating error. Native steps check $LASTEXITCODE instead.
$ErrorActionPreference = 'Continue'

$Root     = Split-Path $PSScriptRoot -Parent
$BuildDir = if ($env:BAT_BUILD_DIR) { $env:BAT_BUILD_DIR } else { 'C:\bt-build' }
$QtDir    = if ($env:BAT_QT_DIR)    { $env:BAT_QT_DIR }    else { 'C:\Qt\6.7.3\msvc2019_64' }
$Vcpkg    = if ($env:VCPKG_ROOT)    { $env:VCPKG_ROOT }    else { 'C:\vcpkg' }
$VsDir    = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * `
              -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

# MSVC environment (cl, link, rc, ninja) imported into this process.
cmd /c "`"$VsDir\VC\Auxiliary\Build\vcvars64.bat`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { Set-Item "env:$($Matches[1])" $Matches[2] }
}

# CMake bakes the keys in at configure time, so .env is loaded before it.
$envFile = Join-Path $Root '.env'
if (Test-Path $envFile) {
    # Same file the macOS script sources, so `export KEY=value` lines count too.
    Get-Content $envFile | Where-Object { $_ -match '^\s*(?:export\s+)?([A-Z_][A-Z0-9_]*)\s*=\s*(.*)$' } | ForEach-Object {
        Set-Item "env:$($Matches[1])" $Matches[2].Trim().Trim('"').Trim("'")
    }
} else {
    Write-Warning 'no .env: building without metadata API keys'
}
foreach ($k in 'BAT_TMDB_KEY', 'BAT_IGDB_ID', 'BAT_STEAM_KEY', 'BAT_SUBDL_KEY') {
    $state = if ((Get-Item "env:$k" -ErrorAction SilentlyContinue).Value) { 'present' } else { 'empty' }
    Write-Host ("{0,-14} {1}" -f $k, $state)
}

# app.rc wants the .ico; CI makes it with ImageMagick, Pillow does the same here.
$ico = Join-Path $Root 'src\images\logo1.ico'
if (-not (Test-Path $ico)) {
    python -c "from PIL import Image; Image.open(r'$Root\src\images\logo1.png').convert('RGBA').save(r'$ico', sizes=[(256,256),(64,64),(48,48),(32,32),(16,16)])"
}

if ($Clean -and (Test-Path $BuildDir)) { Remove-Item $BuildDir -Recurse -Force -Confirm:$false }

# vcpkg's libtorrent is only there to pull in Boost/OpenSSL; its applocal step
# would copy ITS torrent-rasterbar.dll next to every exe, and the app and tests
# are built against the fork's headers. So applocal is off, and everything that
# runs during the build (Catch2 lists each test by running it) finds DLLs on a
# PATH with the fork first.
$testsFlag = if ($Tests) { 'ON' } else { 'OFF' }
$forkDir   = "$BuildDir\third_party\libtorrent"
$vcpkgBin  = "$Vcpkg\installed\x64-windows\bin"
$catchDir  = "$BuildDir\_deps\catch2-build\src"   # Catch2 builds as a DLL here
$env:PATH  = "$forkDir;$catchDir;$QtDir\bin;$vcpkgBin;$env:PATH"
cmake -S $Root -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DBAT_LIBTORRENT_SOURCE=ON -DBAT_SENTRY=OFF -DBAT_BUILD_TESTS=$testsFlag `
    -DCMAKE_TOOLCHAIN_FILE="$Vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_APPLOCAL_DEPS=OFF -DCMAKE_PREFIX_PATH="$QtDir"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
# A stale vcpkg copy next to a test from an applocal build would still win over PATH.
Remove-Item "$BuildDir\tests\torrent-rasterbar.dll" -ErrorAction SilentlyContinue
Write-Host "jobs: $Jobs"
if ($Target) {
    cmake --build $BuildDir --parallel $Jobs --target $Target
    exit $LASTEXITCODE
}
cmake --build $BuildDir --parallel $Jobs
if ($LASTEXITCODE) { exit $LASTEXITCODE }

# Next to the app: the fork's engine, plus vcpkg's runtime DLLs (OpenSSL, zlib, ...) but never its libtorrent.
Copy-Item "$forkDir\torrent-rasterbar.dll" $BuildDir -Force
Get-ChildItem $vcpkgBin -Filter *.dll | Where-Object { $_.Name -ne 'torrent-rasterbar.dll' } |
    Copy-Item -Destination $BuildDir -Force

if (-not (Test-Path "$BuildDir\Qt6Core.dll")) {
    & "$QtDir\bin\windeployqt.exe" --qmldir "$Root\src\qml" --release "$BuildDir\BATorrent.exe" | Out-Null
}
# windeployqt brings the ffmpeg plugin but not the ffmpeg DLLs it needs.
Get-ChildItem "$QtDir\bin" -Filter *.dll |
    Where-Object { $_.Name -match '^(avcodec|avformat|avutil|swscale|swresample)-' } |
    Where-Object { -not (Test-Path (Join-Path $BuildDir $_.Name)) } |
    Copy-Item -Destination $BuildDir

if ($Tests) {
    # The suite registers under tests/, not the build root.
    # --timeout: a hung case fails after 5 min instead of holding the run.
    ctest --test-dir "$BuildDir\tests" --output-on-failure --parallel $Jobs --timeout 300
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
Write-Host "built: $BuildDir\BATorrent.exe"
