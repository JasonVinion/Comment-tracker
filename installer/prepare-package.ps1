param(
    [string]$BuildDir = "..\out\build\x64-Release",
    [string]$OutputDir = ".\package",
    [string]$RedistUrl = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$resolvedBuildDir = Resolve-Path -Path (Join-Path $scriptDir $BuildDir) -ErrorAction Stop
$resolvedOutputDir = Join-Path $scriptDir $OutputDir
$exePath = Join-Path $resolvedBuildDir "CommentExplorer.exe"

if (-not (Test-Path $exePath)) {
    Write-Error "CommentExplorer.exe not found in $resolvedBuildDir. Build the project first, then re-run this script."
    exit 1
}

$requiredFiles = @(
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Concurrent.dll",
    "platforms\\qwindows.dll"
)

foreach ($file in $requiredFiles) {
    $path = Join-Path $resolvedBuildDir $file
    if (-not (Test-Path $path)) {
        Write-Error "Missing required runtime dependency: $path"
        exit 1
    }
}

if (Test-Path $resolvedOutputDir) {
    Remove-Item -Path $resolvedOutputDir -Recurse -Force
}

New-Item -Path $resolvedOutputDir -ItemType Directory -Force | Out-Null
Copy-Item -Path (Join-Path $resolvedBuildDir "*") -Destination $resolvedOutputDir -Recurse -Force

$redistDir = Join-Path $resolvedOutputDir "redist"
New-Item -Path $redistDir -ItemType Directory -Force | Out-Null
$redistPath = Join-Path $redistDir "vc_redist.x64.exe"

Write-Host "Downloading VC++ runtime..."
Invoke-WebRequest -Uri $RedistUrl -OutFile $redistPath -UseBasicParsing

Write-Host "Installer package prepared at $resolvedOutputDir"
