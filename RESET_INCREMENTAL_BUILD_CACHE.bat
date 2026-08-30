@echo off
setlocal EnableExtensions DisableDelayedExpansion

if defined LOCALAPPDATA (
  set "CACHE_ROOT=%LOCALAPPDATA%\VEK\incremental\CustomVehicleGame"
) else (
  set "CACHE_ROOT=%TEMP%\VEK-incremental\CustomVehicleGame"
)

echo ============================================
echo  RESET SMART INCREMENTAL BUILD CACHE
echo ============================================
echo.
echo Use this ONLY when the shared cache is corrupted, your compiler/toolchain
echo changed badly, or you deliberately want a completely fresh build.
echo.
echo This deletes generated/shared build data only:
echo   %CACHE_ROOT%
echo.
echo Your downloaded source folder and C:\vek installation are NOT deleted.
echo Close Visual Studio, CMake, MSBuild and the game first.
echo.
choice /C YN /N /M "Reset incremental cache? [Y/N] "
if errorlevel 2 exit /b 0

if exist "%CACHE_ROOT%" rmdir /s /q "%CACHE_ROOT%"
if exist "%CACHE_ROOT%" (
  echo.
  echo Some cache files could not be deleted because another process is using them.
  pause
  exit /b 1
)

echo.
echo Cache reset complete. Your next build will be a full build once,
echo then smart incremental builds resume.
pause
