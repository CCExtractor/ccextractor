$ErrorActionPreference = 'Stop'

$packageName = 'ccextractor'

# Get the uninstall registry key
$regKey = Get-UninstallRegistryKey -SoftwareName 'CCExtractor*'

if ($regKey) {
  $silentArgs = '/quiet /norestart'
  $file = $regKey.UninstallString -replace 'msiexec.exe','msiexec.exe ' -replace '/I','/X'

  $packageArgs = @{
    packageName    = $packageName
    fileType       = 'MSI'
    silentArgs     = "$($regKey.PSChildName) $silentArgs"
    file           = ''
    validExitCodes = @(0, 3010, 1605, 1614, 1641)
  }

  Uninstall-ChocolateyPackage @packageArgs
} else {
  Write-Warning "CCExtractor was not found in the registry. It may have been uninstalled already."
}
