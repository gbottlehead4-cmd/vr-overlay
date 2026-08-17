<#
.SYNOPSIS
  Authenticode-sign VisorVR's binaries.

.DESCRIPTION
  Run this against a build tree BEFORE make-portable.ps1, so the packaged copy
  is signed.

  Signing is what removes the SmartScreen "Windows protected your PC" prompt.
  It is not affected by whether you ship a zip or an installer - an unsigned
  MSI is warned about exactly the same way. Two kinds of certificate:

    EV (extended validation) - SmartScreen trusts it immediately. The private
      key lives on a hardware token or cloud HSM, so this script's -Thumbprint
      mode is the one to use.
    OV/standard - cheaper, but reputation has to accumulate over downloads, so
      early users still see the warning.

  Expect Defender to occasionally flag VisorVR regardless: injecting a layer
  into a running game resembles what malware does. Signing reduces it; if it
  happens, submit the binary to Microsoft at
  https://www.microsoft.com/en-us/wdsi/filesubmission

.PARAMETER Thumbprint
  Certificate thumbprint in the local certificate store. Use this with an EV
  token - the key never leaves the hardware.

.PARAMETER PfxPath
  Alternative: path to a .pfx file. -PfxPassword is then required.

.EXAMPLE
  .\sign-build.ps1 -Thumbprint A1B2C3...
  .\sign-build.ps1 -PfxPath cert.pfx -PfxPassword (Read-Host -AsSecureString)
#>
[CmdletBinding(DefaultParameterSetName = 'Thumbprint')]
param(
  [Parameter(Mandatory, ParameterSetName = 'Thumbprint')]
  [string] $Thumbprint,

  [Parameter(Mandatory, ParameterSetName = 'Pfx')]
  [string] $PfxPath,
  [Parameter(Mandatory, ParameterSetName = 'Pfx')]
  [System.Security.SecureString] $PfxPassword,

  [string] $BuildDir,
  [string] $Config = 'RelWithDebInfo',
  [string] $TimestampUrl = 'http://timestamp.digicert.com'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $repoRoot 'build' }
$outRoot = Join-Path (Resolve-Path $BuildDir) "out\$Config"
if (-not (Test-Path $outRoot)) { throw "No build output at $outRoot" }

$signtool =
  Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter 'signtool.exe' -ErrorAction SilentlyContinue |
  Where-Object { $_.FullName -match '\\x64\\' } |
  Sort-Object FullName -Descending |
  Select-Object -First 1
if (-not $signtool) {
  throw 'signtool.exe not found - install the Windows SDK.'
}

# Only our own binaries: third-party ones (CEF, the Windows App SDK) arrive
# already signed by their vendors, and re-signing them would be wrong.
$targets =
  Get-ChildItem $outRoot -Recurse -Include '*.exe', '*.dll' -File |
  Where-Object { $_.Name -like 'VisorVR*' }

if (-not $targets) { throw "No VisorVR binaries found under $outRoot" }
Write-Host "Signing $($targets.Count) binaries with $($signtool.Name)"

$common = @('sign', '/fd', 'SHA256', '/td', 'SHA256', '/tr', $TimestampUrl, '/v')
if ($PSCmdlet.ParameterSetName -eq 'Thumbprint') {
  $auth = @('/sha1', $Thumbprint)
} else {
  $plain = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
    [Runtime.InteropServices.Marshal]::SecureStringToBSTR($PfxPassword))
  $auth = @('/f', $PfxPath, '/p', $plain)
}

& $signtool.FullName @common @auth @($targets.FullName)
if ($LASTEXITCODE -ne 0) { throw "signtool failed (exit $LASTEXITCODE)" }

Write-Host 'Verifying...'
& $signtool.FullName verify /pa /all @($targets.FullName) | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Signature verification failed (exit $LASTEXITCODE)" }
Write-Host "Signed and verified $($targets.Count) binaries."
