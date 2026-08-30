@echo off
cd /d "%~dp0"
if exist "build\Release\CustomVehicleGame.exe" (
  start "" "build\Release\CustomVehicleGame.exe"
  exit /b 0
) else (
  echo Build the game first by double-clicking BUILD_WINDOWS.bat
  pause
)
