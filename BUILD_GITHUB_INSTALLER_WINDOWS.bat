@echo off
setlocal EnableExtensions
cd /d "%~dp0"

python tools\version_manager.py sync
if errorlevel 1 exit /b 1
python tools\version_manager.py check
if errorlevel 1 exit /b 1

for %%F in (UPDATE_POLICY release-version.json) do (
  if not exist "%%F" (echo [VEK] Required release file is missing: %%F& exit /b 1)
)

where go >nul 2>nul
if errorlevel 1 (
  echo [VEK] Go 1.23+ is required to build the GitHub installer.
  exit /b 1
)

if not exist build-installer mkdir build-installer
pushd tools\github-installer
go build -trimpath -ldflags "-s -w -H=windowsgui" -o ..\..\build-installer\VekInstaller.exe .
if errorlevel 1 (popd & exit /b 1)
popd
copy /y VERSION build-installer\VERSION >nul
copy /y UPDATE_POLICY build-installer\UPDATE_POLICY >nul
copy /y release-version.json build-installer\release-version.json >nul
>build-installer\REPOSITORY.txt echo https://github.com/defnot67kid-beep/vek.git
pushd tools\portable-launcher
go build -trimpath -ldflags "-s -w" -o ..\..\build-installer\vek.exe .
if errorlevel 1 (popd & exit /b 1)
popd

echo [VEK] Built: %CD%\build-installer\VekInstaller.exe
exit /b 0
