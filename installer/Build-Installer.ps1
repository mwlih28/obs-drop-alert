<#
.SYNOPSIS
    obs-drop-alert kurulum programini (.exe) uretir.

.DESCRIPTION
    Surumu buildspec.json'dan okur, release/<Configuration>/obs-drop-alert
    klasorunun hazir oldugunu dogrular ve Inno Setup derleyicisini calistirir.

    Onkosul: once eklentiyi derleyip install et:
        cmake --preset windows-x64
        cmake --build --preset windows-x64 --config RelWithDebInfo --parallel
        cmake --install build_x64 --prefix release/RelWithDebInfo --config RelWithDebInfo

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File installer\Build-Installer.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$SourceDir = Join-Path $ProjectRoot "release\$Configuration\obs-drop-alert"

# --- Surum numarasi tek kaynaktan: buildspec.json ---
$buildspec = Get-Content (Join-Path $ProjectRoot 'buildspec.json') -Raw | ConvertFrom-Json
$version = $buildspec.version
if (-not $version) { throw 'buildspec.json icinde version alani yok' }

# --- Derlenmis eklenti yerinde mi ---
$dll = Join-Path $SourceDir 'bin\64bit\obs-drop-alert.dll'
if (-not (Test-Path $dll)) {
    throw "Derlenmis eklenti bulunamadi: $dll`nOnce 'cmake --install build_x64 --prefix release/$Configuration --config $Configuration' calistir."
}

# --- Inno Setup derleyicisini bul ---
$isccCandidates = @(
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
)
$iscc = $isccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
    $cmd = Get-Command iscc -ErrorAction SilentlyContinue
    if ($cmd) { $iscc = $cmd.Source }
}
if (-not $iscc) {
    throw "Inno Setup bulunamadi. Kurmak icin:`n    winget install --id JRSoftware.InnoSetup --exact"
}

Write-Host "Inno Setup : $iscc"
Write-Host "Surum      : $version"
Write-Host "Kaynak     : $SourceDir"

$iss = Join-Path $PSScriptRoot 'obs-drop-alert.iss'
& $iscc "/DMyAppVersion=$version" "/DSourceDir=$SourceDir" $iss
if ($LASTEXITCODE -ne 0) { throw "ISCC $LASTEXITCODE koduyla basarisiz oldu" }

$output = Join-Path $ProjectRoot "release\obs-drop-alert-$version-windows-x64-installer.exe"
if (Test-Path $output) {
    $size = [Math]::Round((Get-Item $output).Length / 1KB)
    Write-Host ''
    Write-Host "Kurulum programi hazir: $output ($size KB)"
} else {
    throw "ISCC basarili gorundu ama cikti bulunamadi: $output"
}
