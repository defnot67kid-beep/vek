@echo off
setlocal
cd /d "%~dp0"
echo ============================================
echo   ROTATE + SIGN ALL VEK 2.0 RELEASE SCRIPTS
echo ============================================
echo This creates a LOCAL private key under developer_keys\.
echo NEVER distribute that folder.
echo.
py tools\generate_vek_signing_key.py
if errorlevel 1 (
  echo.
  echo Signing/manifest generation failed.
  pause
  exit /b 1
)
echo.
echo All VEK scripts are signed and the compiled SHA-256 manifest is current.
echo Rebuild the SECURE RELEASE game now.
pause
