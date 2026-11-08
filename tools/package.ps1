# Build the plugin and assemble a release folder + zip.
# ASCII-only on purpose: Windows PowerShell 5.1 reads .ps1 files as ANSI.
param(
    [string]$Prefix = '',
    [string]$Version = '1.0.0',
    [string]$Out = ''
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot

Write-Host '== building =='
& (Join-Path $root 'build.ps1') -Prefix $Prefix

$dll = Join-Path $root 'build-vs\Release\ObituarySKSE.dll'
if (-not (Test-Path $dll)) {
    throw "build output missing: $dll"
}

$modDir = Join-Path $root 'mod'
if (-not (Test-Path $modDir)) {
    throw "runtime files missing: $modDir"
}

if (-not $Out) { $Out = Join-Path $root 'dist\ObituarySKSE' }
if (Test-Path $Out) { Remove-Item $Out -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $Out 'SKSE\Plugins') | Out-Null

Write-Host '== assembling =='
Copy-Item (Join-Path $modDir '*') -Destination $Out -Recurse -Force
Copy-Item $dll (Join-Path $Out 'SKSE\Plugins\ObituarySKSE.dll') -Force

foreach ($doc in @('README.txt', 'README.md', 'LOCALIZATION.md')) {
    $p = Join-Path $root $doc
    if (Test-Path $p) {
        $name = if ($doc -eq 'README.md') { 'README.txt' } else { $doc }
        Copy-Item $p (Join-Path $Out $name) -Force
    }
}
$tool = Join-Path $root 'tools\localization.py'
if (Test-Path $tool) {
    New-Item -ItemType Directory -Force -Path (Join-Path $Out 'tools') | Out-Null
    Copy-Item $tool (Join-Path $Out 'tools\localization.py') -Force
}

$zip = Join-Path $root ("dist\ObituarySKSE-$Version.zip")
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $Out '*') -DestinationPath $zip -Force

Write-Host ''
Write-Host "FOLDER: $Out"
Write-Host "ZIP   : $zip"