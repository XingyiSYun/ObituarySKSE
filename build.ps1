# Rebuild the ObituarySKSE plugin (Release x64).
# ASCII-only on purpose: Windows PowerShell 5.1 reads .ps1 files as ANSI.
# Native tools (cmake) write warnings to stderr, so do NOT set ErrorActionPreference=Stop.
param(
    [string]$Prefix = ''
)

$ErrorActionPreference = 'Continue'

$src    = $PSScriptRoot
if (-not $Prefix) { $Prefix = $env:OBITUARY_COMMONLIBSSE }
if (-not $Prefix) { $Prefix = Join-Path (Split-Path -Parent $src) 'vcpkg-install\x64-windows-static-md' }
$prefix = $Prefix
$build  = Join-Path $src 'build-vs'
$config = 'Release'

$configFile = Join-Path $prefix 'share\CommonLibSSE\CommonLibSSEConfig.cmake'
if (-not (Test-Path $configFile)) {
    Write-Error "CommonLibSSE-NG config not found: $configFile"
    exit 1
}

$vsRoot = 'C:\Program Files\Microsoft Visual Studio\18\Community'
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    Write-Error "vcvars64.bat not found: $vcvars"
    exit 1
}

# Prefer the 14.51 toolset when installed; otherwise keep the default toolset.
$toolset   = '14.51.36231'
$vcvarsArg = ''
if (Test-Path (Join-Path $vsRoot "VC\Tools\MSVC\$toolset")) {
    $vcvarsArg = "-vcvars_ver=$toolset"
}

Write-Host "loading MSVC environment..."
$envLines = & cmd /c "`"$vcvars`" $vcvarsArg >nul && set"
foreach ($line in $envLines) {
    if ($line -match '^([^=]+)=(.*)$') {
        try { Set-Item -Path "env:$($matches[1])" -Value $matches[2] -ErrorAction Stop } catch {}
    }
}
if (-not $env:VCToolsInstallDir) {
    Write-Warning "MSVC environment may not be loaded; the Visual Studio generator resolves its own toolset."
}

$cmakeBin = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$cmakeExe = Join-Path $cmakeBin 'cmake.exe'
if (-not (Test-Path $cmakeExe)) {
    Write-Error "cmake.exe not found: $cmakeExe"
    exit 1
}
$env:PATH = "$cmakeBin;$env:PATH"

Write-Host "cmake: $(& $cmakeExe --version | Select-Object -First 1)"

Write-Host "configure (generator: Visual Studio 18 2026, x64)..."
& $cmakeExe -S $src -B $build -G "Visual Studio 18 2026" -A x64 `
    "-DCMAKE_PREFIX_PATH=$prefix" "-DCOMMONLIBSSE_PREFIX=$prefix" 2>&1 | Out-Host
if ($LASTEXITCODE -ne 0) { Write-Error "configure failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

Write-Host "build ($config)..."
& $cmakeExe --build $build --config $config --parallel 2>&1 | Out-Host
if ($LASTEXITCODE -ne 0) { Write-Error "build failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

$dll = Join-Path $build "$config\ObituarySKSE.dll"
if (-not (Test-Path $dll)) { Write-Error "expected output missing: $dll"; exit 1 }
$item = Get-Item $dll
Write-Host ""
Write-Host ("OK: {0} ({1:N0} bytes, {2})" -f $item.FullName, $item.Length, $item.LastWriteTime)