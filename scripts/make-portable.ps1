<#
.SYNOPSIS
  Package a copy-and-run VisorVR folder from a build tree.

.DESCRIPTION
  Produces a folder (and by default a .zip) that runs on another PC with no
  installer and no administrator rights: the app registers its own OpenXR layer
  under HKCU on launch.

  The directory layout is load-bearing and must not be flattened:
    <root>\bin\VisorVRApp.exe   finds CEF at   <root>\libexec\cef
                                and data at    <root>\share
  Those paths are derived from the executable's location, so `bin` on its own
  is not a working copy - every web panel would fail to render.

  Debug symbols (`pdb`, ~400MB) and build-only outputs (`include`,
  `installer`) are left out.

.EXAMPLE
  .\make-portable.ps1
  .\make-portable.ps1 -Config RelWithDebInfo -OutputDir D:\releases -NoZip
#>
[CmdletBinding()]
param(
  [string] $BuildDir,
  [string] $Config = 'RelWithDebInfo',
  [string] $OutputDir,
  [switch] $NoZip
)

$ErrorActionPreference = 'Stop'

# Not defaulted in param(): $PSScriptRoot is not populated there under 5.1.
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $repoRoot 'build' }
if (-not $OutputDir) { $OutputDir = Join-Path $BuildDir 'portable' }

$outRoot = Join-Path (Resolve-Path $BuildDir) "out\$Config"
if (-not (Test-Path $outRoot)) {
  throw "No build output at $outRoot - build the $Config configuration first."
}

# Everything a user needs at runtime, and nothing else.
$include = @(
  'bin', # app, OpenXR layer, WinUI + CRT runtime
  'libexec', # CEF/Chromium - web panels are dead without this
  'share', # data files and third-party licence texts
  'lib', # DCS Lua hook and API
  'utilities', # RemoteControl executables, viewer
  'scripts'    # tracing profiles for troubleshooting
)

# Files that must exist afterwards, or the package is broken. Cheaper to fail
# here than to have someone unzip it and find no overlay.
$required = @(
  'bin\VisorVRApp.exe',
  'bin\VisorVR-OpenXR64.dll',
  'bin\VisorVR-OpenXR.json',
  'bin\Microsoft.WindowsAppRuntime.dll',
  'libexec\cef\libcef.dll',
  'share\doc\LICENSE.txt'
)

$version = '0.0.0'
$versionJson = Join-Path $outRoot 'installer\version.json'
if (Test-Path $versionJson) {
  $v = (Get-Content $versionJson -Raw | ConvertFrom-Json).components
  $version = "$($v.a).$($v.b).$($v.c).$($v.d)"
}

$stageName = "VisorVR-$version-portable"
$stage = Join-Path $OutputDir $stageName
Write-Host "Packaging $stageName from $outRoot"

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

foreach ($dir in $include) {
  $src = Join-Path $outRoot $dir
  if (-not (Test-Path $src)) {
    Write-Warning "skipping $dir (not present in this build)"
    continue
  }
  robocopy $src (Join-Path $stage $dir) /E /COPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
  # robocopy uses exit codes < 8 for success-with-info
  if ($LASTEXITCODE -ge 8) { throw "robocopy failed for $dir (exit $LASTEXITCODE)" }
  $size = (Get-ChildItem (Join-Path $stage $dir) -Recurse -File | Measure-Object -Property Length -Sum).Sum
  Write-Host ("  {0,-10} {1,8:N1} MB" -f $dir, ($size / 1MB))
}

# The start button. The app itself cannot live here - it locates libexec and
# share relative to its own folder - so this launcher sits at the top level
# and starts bin\VisorVRApp.exe.
$launcher = Join-Path $outRoot 'launcher\VisorVR.exe'
if (Test-Path $launcher) {
  Copy-Item $launcher (Join-Path $stage 'VisorVR.exe')
  Write-Host "  VisorVR.exe (launcher)"
} else {
  Write-Warning "No launcher at $launcher - build the VisorVR-Portable-Launcher target"
}

$required += 'VisorVR.exe'

$missing = $required | Where-Object { -not (Test-Path (Join-Path $stage $_)) }
if ($missing) {
  throw "Package is incomplete, missing:`n  $($missing -join "`n  ")"
}

Set-Content -Path (Join-Path $stage 'README.txt') -Encoding utf8 -Value @"
VisorVR $version - portable

To run:
  Double-click VisorVR.exe in this folder.

The first time, Windows will say "Windows protected your PC" and that the app is
unrecognised. Click "More info", then "Run anyway".

That warning appears for any program not signed with a paid certificate.
VisorVR is free and unfunded, so it is not signed. The full source is public at
https://github.com/gbottlehead4-cmd/vr-overlay if you would rather build it
yourself. Some antivirus tools may also object: VisorVR draws overlays by
loading a layer into the running game, which resembles what malware does. It is
a false positive.

No installation, and no administrator rights. On launch VisorVR registers its
OpenXR layer for your user account only, pointing at this folder, so you can
put it anywhere - move it later and it re-registers itself on the next start.

Keep this folder together. bin\VisorVRApp.exe locates Chromium in libexec and
its data files in share by looking beside its own folder, so bin on its own
will not work.

Settings are stored separately, in:
  %LOCALAPPDATA%\VisorVR

To remove VisorVR: turn off Settings > Virtual Reality > "OpenXR support for
64-bit games", then delete this folder. The toggle removes the registry entry;
without it one is left behind.

VisorVR is a fork of OpenKneeboard by Fred Emmott, used under the OpenKneeboard
Public License v1. Licence texts are in share\doc.
"@

Write-Host "Staged: $stage"

if (-not $NoZip) {
  $zip = "$stage.zip"
  if (Test-Path $zip) { Remove-Item $zip -Force }
  Write-Host "Compressing (this takes a minute; CEF is large)..."
  Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
  Write-Host ("Zip: {0} ({1:N1} MB)" -f $zip, ((Get-Item $zip).Length / 1MB))
}
