<#
.SYNOPSIS
    Packages an already-built texturelab.exe into a redistributable folder and
    zip. Used by .github/workflows/release.yml; runnable locally for the same
    result.

.DESCRIPTION
    Two stages, because code signing has to happen after the files are gathered
    but before they are zipped:

      scripts/package-windows.ps1 -Tag v0.4.0-beta -Stage Deploy   # dist/texturelab-win-<tag>/
      <sign dist/texturelab-win-<tag>/*.exe here>
      scripts/package-windows.ps1 -Tag v0.4.0-beta -Stage Zip      # dist/texturelab-win-<tag>-x64.zip

    -Stage Both (the default) runs both, which is what you want locally.
    windeployqt is found on PATH, or via $env:QTDIR, or -WindeployQt.
#>
param(
    [Parameter(Mandatory = $true)][string]$Tag,
    [ValidateSet("Deploy", "Zip", "Both")][string]$Stage = "Both",
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$WindeployQt
)

$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$dist = "dist\texturelab-win-$Tag"
$zip = "dist\texturelab-win-$Tag-x64.zip"

if ($Stage -eq "Deploy" -or $Stage -eq "Both") {
    $exe = "$BuildDir\src\texturelab\$Config\texturelab.exe"
    if (-not (Test-Path $exe)) { throw "$exe not found - build first" }

    if (-not $WindeployQt) {
        $WindeployQt = if ($env:QTDIR) { "$env:QTDIR\bin\windeployqt.exe" }
                       else { (Get-Command windeployqt.exe -ErrorAction SilentlyContinue).Source }
    }
    if (-not $WindeployQt -or -not (Test-Path $WindeployQt)) {
        throw "windeployqt.exe not found - set `$env:QTDIR or pass -WindeployQt"
    }

    if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
    New-Item -ItemType Directory -Force $dist | Out-Null
    Copy-Item $exe $dist

    & $WindeployQt "$dist\texturelab.exe" --release --no-translations
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed ($LASTEXITCODE)" }

    # Ship only the SQLite driver, matching the Linux and macOS packages.
    Get-ChildItem "$dist\sqldrivers" -Filter *.dll -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne "qsqlite.dll" } | Remove-Item

    # crashpad_handler.exe must sit next to the exe or crashes go unreported.
    $handler = @(
        "$BuildDir\src\texturelab\$Config\crashpad_handler.exe",
        "$BuildDir\src\texturelab\crashpad_handler.exe",
        "$BuildDir\_deps\sentry-build\crashpad_build\handler\$Config\crashpad_handler.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $handler) { throw "crashpad_handler.exe not found in $BuildDir" }
    Copy-Item $handler $dist

    Copy-Item LICENSE "$dist\LICENSE.txt"
    Write-Host "deployed: $dist"
}

if ($Stage -eq "Zip" -or $Stage -eq "Both") {
    if (-not (Test-Path $dist)) { throw "$dist not found - run -Stage Deploy first" }
    if (Test-Path $zip) { Remove-Item $zip -Force }
    # Zip the folder itself so it extracts into texturelab-win-<tag>\.
    Compress-Archive -Path $dist -DestinationPath $zip
    Write-Host "packaged: $zip"
}
