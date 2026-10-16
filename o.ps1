# Windows launcher (PowerShell): runs o.exe (built by CMake into this folder) on a game, e.g.  .\o.ps1 test
Set-Location $PSScriptRoot
if (-not (Test-Path "$PSScriptRoot\o.exe")) {
  Write-Error 'o.exe is not built yet. See "Building on Windows" in README.md.'
  exit 1
}
& "$PSScriptRoot\o.exe" @args
exit $LASTEXITCODE
