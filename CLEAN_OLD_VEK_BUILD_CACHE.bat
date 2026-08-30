@echo off
setlocal EnableExtensions DisableDelayedExpansion

if defined LOCALAPPDATA (
  set "BUILD_BASE=%LOCALAPPDATA%\VEK\builds"
  set "SMART_CACHE=%LOCALAPPDATA%\VEK\incremental\CustomVehicleGame"
) else (
  set "BUILD_BASE=%TEMP%\VEK-builds"
  set "SMART_CACHE=%TEMP%\VEK-incremental\CustomVehicleGame"
)

echo ============================================
echo  VEK LEGACY BUILD CACHE CLEANER
echo ============================================
echo.
echo This removes OLD v26.5/v26.6 random build folders only:
echo   %BUILD_BASE%\CVG-v26-*
echo.
echo It intentionally keeps the new v26.7 smart incremental cache:
echo   %SMART_CACHE%
echo.
echo Keeping the smart cache is what makes future builds fast.
echo Use RESET_INCREMENTAL_BUILD_CACHE.bat only if you truly need a full reset.
echo.
choice /C YN /N /M "Delete legacy random build caches? [Y/N] "
if errorlevel 2 exit /b 0

if exist "%BUILD_BASE%" (
  for /d %%D in ("%BUILD_BASE%\CVG-v26-*") do (
    echo Removing %%~fD
    rmdir /s /q "%%~fD" 2>nul
  )
)

echo.
echo Legacy cleanup finished. Smart incremental cache was preserved.
pause
