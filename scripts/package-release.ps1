param(
    [string]$Version = "2.1.0"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$distRoot = Join-Path $root "dist"
$dirName = "ClipWheel_v$Version"
$pkgDir = Join-Path $distRoot $dirName
$zipPath = Join-Path $distRoot "$dirName.zip"

if (Test-Path $pkgDir) { Remove-Item -Recurse -Force $pkgDir }
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }

New-Item -ItemType Directory -Force -Path $pkgDir | Out-Null

Copy-Item (Join-Path $root "clipwheel.exe") $pkgDir
Copy-Item (Join-Path $root "clipwheel.ini") $pkgDir
Copy-Item (Join-Path $root "README.md") $pkgDir
Copy-Item (Join-Path $root "LICENSE") $pkgDir
Copy-Item (Join-Path $root "clipwheel.ico") $pkgDir

Compress-Archive -Path (Join-Path $pkgDir "*") -DestinationPath $zipPath -CompressionLevel Optimal
Write-Host "Release package created: $zipPath"
