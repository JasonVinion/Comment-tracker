param(
    [string]$OutputPath = "..\resources\app.ico",
    [string]$SourcePngUrl = "https://raw.githubusercontent.com/google/material-design-icons/master/png/action/find_in_page/materialicons/48dp/2x/baseline_find_in_page_black_48dp.png"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outputFile = Join-Path $scriptDir $OutputPath
$outputDir = Split-Path -Parent $outputFile

if (-not (Test-Path $outputDir)) {
    New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
}

$tempPng = Join-Path $env:TEMP "comment-explorer-icon.png"

Write-Host "Downloading icon source..."
Invoke-WebRequest -Uri $SourcePngUrl -OutFile $tempPng -UseBasicParsing -ErrorAction Stop

Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class IconHelper
{
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool DestroyIcon(IntPtr hIcon);
}
"@

$bitmap = [System.Drawing.Bitmap]::FromFile($tempPng)
$icon = [System.Drawing.Icon]::FromHandle($bitmap.GetHicon())

try {
    $stream = New-Object System.IO.FileStream($outputFile, [System.IO.FileMode]::Create)
    $icon.Save($stream)
    $stream.Close()
} finally {
    [IconHelper]::DestroyIcon($icon.Handle) | Out-Null
    $icon.Dispose()
    $bitmap.Dispose()
}

Remove-Item -Path $tempPng -Force
Write-Host "Icon written to $outputFile"
