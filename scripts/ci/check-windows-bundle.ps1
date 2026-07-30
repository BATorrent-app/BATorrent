#!/usr/bin/env pwsh
# Fail the Windows package if it would not open on a clean machine:
# missing Qt platform plugin, wrong libtorrent DLL, unresolved imports.
param(
    [Parameter(Mandatory = $true)]
    [string]$Dir
)

$ErrorActionPreference = "Stop"
if (!(Test-Path $Dir)) { throw "bundle dir missing: $Dir" }

$fail = 0
function Err($msg) {
    Write-Host "::error::$msg"
    $script:fail = 1
}

# --- Critical files (crash-before-main / silent no-open class) ---------------
$required = @(
    "BATorrent.exe",
    "torrent-rasterbar.dll",
    "platforms\qwindows.dll",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "Qt6Svg.dll",
    "Qt6Multimedia.dll",
    "Qt6Quick.dll",
    "Qt6Qml.dll",
    "Qt6OpenGL.dll",
    "Qt6QuickControls2.dll",
    "Qt6QuickTemplates2.dll",
    "Qt6QuickEffects.dll",
    "Qt6QuickShapes.dll",
    "qml\QtQuick\Controls\Basic\qmldir",
    "qml\QtQuick\Effects\qmldir",
    "qml\QtQuick\Shapes\qmldir",
    "qml\QtQuick\Dialogs\qmldir",
    "qml\Qt\labs\platform\qmldir"
)
foreach ($rel in $required) {
    $p = Join-Path $Dir $rel
    if (!(Test-Path $p)) { Err "critical file missing from bundle: $rel — app would not open" }
    else { Write-Host "OK $rel" }
}

$ff = Get-ChildItem $Dir -Filter "avcodec-*.dll" -ErrorAction SilentlyContinue
if ($ff.Count -eq 0) {
    Err "no avcodec-*.dll in bundle — Qt Multimedia falls back and MKV/HEVC playback breaks"
} else {
    Write-Host "OK ffmpeg runtime: $($ff.Name -join ', ')"
}

$svgPlugin = Get-ChildItem (Join-Path $Dir "imageformats") -Filter "qsvg*.dll" -ErrorAction SilentlyContinue
if (-not $svgPlugin -or $svgPlugin.Count -eq 0) {
    Err "imageformats/qsvg*.dll missing — icons/SVG UI assets will not load"
} else {
    Write-Host "OK $($svgPlugin.Name -join ', ')"
}

# --- Fork libtorrent must be the one that ships (issue #20 / #32 class) ------
$dumpbinCmd = Get-Command dumpbin -ErrorAction SilentlyContinue
$dumpbin = $null
if ($dumpbinCmd) { $dumpbin = $dumpbinCmd.Source }
if (-not $dumpbin) {
    $cand = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Recurse -Filter dumpbin.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($cand) { $dumpbin = $cand.FullName }
}
if (-not $dumpbin) {
    Err "dumpbin not found — cannot verify DLL imports/exports (refusing to ship blind)"
} else {
    $dll = Join-Path $Dir "torrent-rasterbar.dll"
    if (Test-Path $dll) {
        $exports = & $dumpbin /exports $dll 2>&1 | Out-String
        if ($exports -notmatch "set_geo_local_fn") {
            Err "bundled torrent-rasterbar.dll lacks set_geo_local_fn — stock/vcpkg DLL shipped (would crash on launch)"
        } else {
            Write-Host "OK fork symbol set_geo_local_fn exported"
        }
    }

    $systemish = [regex]'^(?i)(api-ms-win-|ext-ms-|KERNEL32|USER32|GDI32|ADVAPI32|SHELL32|OLE32|OLEAUT32|WS2_32|WINMM|IMM32|VERSION|SETUPAPI|CFGMGR32|CRYPT32|BCRYPT|NCRYPT|SECHOST|RPCRT4|COMBASE|SHLWAPI|UXTHEME|DWMAPI|IPHLPAPI|DNSAPI|NTDLL|MSVCRT|MSVCP_WIN|ucrtbase|WINTRUST|bcryptprimitives|nsi|WTSAPI32|USERENV|PROPSYS|CLBCatQ|COMDLG32|WINSPOOL|SHCORE|dxgi|d3d11|d3d12|DWrite|dxcore|HID|powrprof|dbghelp|PSAPI|MPR|NETAPI32|SAMLIB|wldp|CRYPTBASE|FLTLIB|gpapi|mswsock|WSOCK32|NORMALIZ|NSI|dhcpcsvc|WINHTTP|webio|URLMON|iertutil|sxs|profapi)\b'

    $targets = @((Join-Path $Dir "BATorrent.exe"))
    $targets += Get-ChildItem $Dir -Filter "Qt6*.dll" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
    $targets += Get-ChildItem (Join-Path $Dir "platforms") -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
    $targets += Get-ChildItem (Join-Path $Dir "torrent-rasterbar.dll") -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }

    $seenImports = 0
    foreach ($target in ($targets | Select-Object -Unique)) {
        if (!(Test-Path $target)) { continue }
        $deps = & $dumpbin /dependents $target 2>&1 | Out-String
        $dlls = [regex]::Matches($deps, '(?im)^\s+([A-Za-z0-9._\-]+\.dll)\s*$') |
            ForEach-Object { $_.Groups[1].Value } |
            Select-Object -Unique
        $seenImports += $dlls.Count
        foreach ($name in $dlls) {
            if ($name -match $systemish) { continue }
            $here = Join-Path $Dir $name
            if (!(Test-Path $here)) {
                Err "$(Split-Path $target -Leaf) imports $name but it is not in the bundle — classic missing-DLL no-open"
            }
        }
    }
    if ($seenImports -eq 0) {
        Err "dumpbin returned no dependents — VS toolchain broken or wrong dumpbin; refusing vacuous pass"
    } else {
        Write-Host "OK dumpbin scanned $seenImports import entries"
    }
}

if ($fail -ne 0) { exit 1 }
Write-Host "Windows bundle integrity OK"
exit 0
