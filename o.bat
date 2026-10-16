@echo off
rem Windows launcher: runs o.exe (built by CMake into this folder) on a game, e.g.  o test
rem Run it from the repository root, since games and scripts are found relative to the current folder.
cd /d "%~dp0"
if not exist "%~dp0o.exe" (
  echo o.exe isn't built yet. See "Building on Windows" in README.md.
  exit /b 1
)
"%~dp0o.exe" %*
