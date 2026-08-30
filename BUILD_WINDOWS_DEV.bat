@echo off
setlocal EnableExtensions DisableDelayedExpansion

cd /d "%~dp0"
for %%I in ("%~dp0.") do set "SRC_DIR=%%~fI"

echo ============================================
echo  VEK DEV BUILD - SMART INCREMENTAL CACHE
echo ============================================
echo NOT FOR SHIPPING. Accepts unsigned .vek edits and enables F11 hot reload.
echo Reuses previous compatible CMake/MSBuild work across newly unzipped versions.
echo Only changed C++ files/dependencies rebuild; VEK scripts/assets are exact-mirrored.
echo.
echo Source:       %SRC_DIR%

where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake was not found on PATH.
  pause
  exit /b 1
)

where git >nul 2>nul
if errorlevel 1 (
  echo ERROR: Git for Windows was not found on PATH.
  pause
  exit /b 1
)

where powershell >nul 2>nul
if errorlevel 1 (
  echo ERROR: Windows PowerShell was not found.
  pause
  exit /b 1
)

if defined LOCALAPPDATA (
  set "CACHE_ROOT=%LOCALAPPDATA%\VEK\incremental\CustomVehicleGame"
) else (
  set "CACHE_ROOT=%TEMP%\VEK-incremental\CustomVehicleGame"
)
set "CACHE_SOURCE=%CACHE_ROOT%\source"
set "SMART_BUILD=%CACHE_ROOT%\dev"
set "LOCAL_OUTPUT=%SRC_DIR%\build-dev\Release"

echo Shared source: %CACHE_SOURCE%
echo Build cache:   %SMART_BUILD%
echo Local output:  %LOCAL_OUTPUT%
echo.
echo [1/4] Detecting changed source files...
powershell -NoProfile -ExecutionPolicy Bypass -File "%SRC_DIR%\build_support\Sync-IncrementalSource.ps1" -SourceRoot "%SRC_DIR%" -MirrorRoot "%CACHE_SOURCE%"
if errorlevel 1 goto :syncfail

echo.
echo [2/4] Configuring/reusing CMake cache...
if not exist "%SMART_BUILD%" mkdir "%SMART_BUILD%" >nul 2>nul
cmake ^
  -S "%CACHE_SOURCE%" ^
  -B "%SMART_BUILD%" ^
  -A x64 ^
  "-DVEK_DEVELOPMENT_MODE=ON" ^
  "-DVEK_PREFER_INSTALLED=ON" ^
  "-DVEK_INSTALL_ROOT=C:/vek"
if errorlevel 1 goto :fail

echo.
echo [3/4] Building only what changed...
cmake --build "%SMART_BUILD%" --config Release --parallel
if errorlevel 1 goto :fail

echo.
echo [4/4] Publishing an authoritative runtime copy...
powershell -NoProfile -ExecutionPolicy Bypass -File "%SRC_DIR%\build_support\Publish-IncrementalOutput.ps1" -BuildOutput "%SMART_BUILD%\Release" -LocalOutput "%LOCAL_OUTPUT%"
if errorlevel 1 goto :copyfail

echo.
echo SMART DEV BUILD COMPLETE:
echo   %LOCAL_OUTPUT%\CustomVehicleGame.exe
echo.
echo Persistent incremental cache:
echo   %SMART_BUILD%
echo.
echo First build is full. After that MSBuild reuses unchanged objects/libraries.
echo Editing only a .vek script does not force the C++ game to recompile.
pause
exit /b 0

:syncfail
echo.
echo Smart source synchronization failed. Your original source was not modified.
pause
exit /b 1

:copyfail
echo.
echo Build succeeded, but copying the output back failed.
echo Close CustomVehicleGame.exe if it is currently running.
echo The newly built game is still available under:
echo   %SMART_BUILD%\Release
pause
exit /b 1

:fail
echo.
echo Build failed. The incremental cache has been kept for diagnosis/retry:
echo   %SMART_BUILD%
echo If the cache itself is damaged, run RESET_INCREMENTAL_BUILD_CACHE.bat once.
pause
exit /b 1
