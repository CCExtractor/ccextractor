$ErrorActionPreference = 'Stop'

$packageName = 'ccextractor'
$toolsDir = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"

# Package parameters
$packageArgs = @{
  packageName    = $packageName
  fileType       = 'MSI'
  url64bit       = 'https://github.com/CCExtractor/ccextractor/releases/download/v0.96.4/CCExtractor.0.96.4.msi'
  checksum64     = 'FFCAB0D766180AFC2832277397CDEC885D15270DECE33A9A51947B790F1F095B'
  checksumType64 = 'sha256'
  silentArgs     = '/quiet /norestart'
  validExitCodes = @(0, 3010, 1641)
}

Install-ChocolateyPackage @packageArgs

# Add to PATH if not already there
$installPath = Join-Path $env:ProgramFiles 'CCExtractor'
if (Test-Path $installPath) {
  Install-ChocolateyPath -PathToInstall $installPath -PathType 'Machine'
  Write-Host "CCExtractor installed to: $installPath"
}
