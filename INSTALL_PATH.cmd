@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "VEK_DIR=%~dp0"
if "%VEK_DIR:~-1%"=="\" set "VEK_DIR=%VEK_DIR:~0,-1%"

if not exist "%VEK_DIR%\vek.exe" (
  echo [VEK] vek.exe was not found in:
  echo       %VEK_DIR%
  echo.
  echo Use this file from the official Windows portable package root.
  exit /b 2
)

for /f "tokens=2,*" %%A in ('reg query HKCU\Environment /v Path 2^>nul ^| find /i "Path"') do set "USER_PATH=%%B"
if not defined USER_PATH set "USER_PATH="

set "FOUND=0"
for %%P in ("!USER_PATH:;=" "!") do (
  if /I "%%~P"=="%VEK_DIR%" set "FOUND=1"
)

if "!FOUND!"=="1" (
  echo [VEK] Already on your User PATH:
  echo       %VEK_DIR%
) else (
  if defined USER_PATH (
    set "NEW_PATH=!USER_PATH!;%VEK_DIR%"
  ) else (
    set "NEW_PATH=%VEK_DIR%"
  )
  reg add HKCU\Environment /v Path /t REG_EXPAND_SZ /d "!NEW_PATH!" /f >nul
  if errorlevel 1 (
    echo [VEK] Could not update your User PATH.
    exit /b 1
  )
  echo [VEK] Added to your User PATH:
  echo       %VEK_DIR%
)

echo.
echo VEK_HOME is intentionally NOT required or set.
echo Close this Command Prompt, open a NEW one, then run:
echo.
echo   vek --version
echo   vek doctor
echo.
exit /b 0
