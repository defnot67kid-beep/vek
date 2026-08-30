@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "VEK_DIR=%~dp0"
if "%VEK_DIR:~-1%"=="\" set "VEK_DIR=%VEK_DIR:~0,-1%"

for /f "tokens=2,*" %%A in ('reg query HKCU\Environment /v Path 2^>nul ^| find /i "Path"') do set "USER_PATH=%%B"
if not defined USER_PATH (
  echo [VEK] Your User PATH is empty. Nothing to remove.
  exit /b 0
)

set "NEW_PATH="
for %%P in ("!USER_PATH:;=" "!") do (
  if /I not "%%~P"=="%VEK_DIR%" (
    if defined NEW_PATH (set "NEW_PATH=!NEW_PATH!;%%~P") else set "NEW_PATH=%%~P"
  )
)

reg add HKCU\Environment /v Path /t REG_EXPAND_SZ /d "!NEW_PATH!" /f >nul
if errorlevel 1 (
  echo [VEK] Could not update your User PATH.
  exit /b 1
)
echo [VEK] Removed from your User PATH:
echo       %VEK_DIR%
echo Open a new Command Prompt for the change to take effect.
exit /b 0
