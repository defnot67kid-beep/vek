@echo off
setlocal EnableExtensions
cd /d "%~dp0"

for /f "usebackq delims=" %%V in ("VERSION") do set "VEK_VERSION=%%V"
if not defined VEK_VERSION (
  echo [VEK] VERSION could not be read.
  exit /b 1
)

python tools\version_manager.py sync
if errorlevel 1 exit /b 1
python tools\version_manager.py check
if errorlevel 1 exit /b 1

set "BUILD_DIR=build-portable"
set "STAGE_DIR=stage-portable"
set "PKG_DIR=VEK-v%VEK_VERSION%-windows-x64"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
if exist "%PKG_DIR%" rmdir /s /q "%PKG_DIR%"
if exist "%PKG_DIR%.zip" del /q "%PKG_DIR%.zip"

cmake -S . -B "%BUILD_DIR%" -A x64 -DVEK_BUILD_TESTS=ON -DVEK_BUILD_C_ABI=ON -DVEK_PORTABLE_STATIC_MSVC_RUNTIME=ON
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 exit /b 1
ctest --test-dir "%BUILD_DIR%" -C Release --output-on-failure
if errorlevel 1 exit /b 1
cmake --install "%BUILD_DIR%" --config Release --prefix "%STAGE_DIR%"
if errorlevel 1 exit /b 1

mkdir "%PKG_DIR%"
copy /y "%STAGE_DIR%\bin\vek.exe" "%PKG_DIR%\vek.exe" >nul
if exist "%STAGE_DIR%\bin\VekInstaller.exe" copy /y "%STAGE_DIR%\bin\VekInstaller.exe" "%PKG_DIR%\VekInstaller.exe" >nul
if exist "%STAGE_DIR%\bin\vek.dll" copy /y "%STAGE_DIR%\bin\vek.dll" "%PKG_DIR%\vek.dll" >nul
if exist "%STAGE_DIR%\lib" xcopy /e /i /y "%STAGE_DIR%\lib" "%PKG_DIR%\lib" >nul
xcopy /e /i /y "%STAGE_DIR%\include" "%PKG_DIR%\include" >nul
xcopy /e /i /y "examples" "%PKG_DIR%\examples" >nul
xcopy /e /i /y "docs" "%PKG_DIR%\docs" >nul
for %%F in (VERSION LICENSE README.md INSTALL_PATH.cmd UNINSTALL_PATH.cmd) do copy /y "%%F" "%PKG_DIR%\%%F" >nul

where go >nul 2>nul
if errorlevel 1 (
  echo [VEK] Go 1.23+ is required to build the release installer.
  exit /b 1
)
pushd tools\github-installer
go build -trimpath -ldflags "-s -w -H=windowsgui" -o "..\..\%PKG_DIR%\VekInstaller.exe" .
if errorlevel 1 (popd & exit /b 1)
popd
if not exist build-installer mkdir build-installer
pushd tools\portable-launcher
go build -trimpath -ldflags "-s -w" -o "..\..\build-installer\vek.exe" .
if errorlevel 1 (popd & exit /b 1)
popd
>"%PKG_DIR%\REPOSITORY.txt" echo https://github.com/defnot67kid-beep/vek.git
copy /y "docs\PORTABLE_RELEASE.md" "%PKG_DIR%\PORTABLE_RELEASE.md" >nul
copy /y "docs\RELEASE_NOTES_V3.0.2.md" "%PKG_DIR%\RELEASE_NOTES_V3.0.2.md" >nul

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root=(Resolve-Path '%PKG_DIR%').Path; $files=Get-ChildItem $root -File -Recurse ^| Where-Object { $_.Name -ne 'VekInstaller.exe' } ^| Sort-Object FullName; $lines=foreach($f in $files){$h=(Get-FileHash -Algorithm SHA256 $f.FullName).Hash.ToLowerInvariant(); $r=[IO.Path]::GetRelativePath($root,$f.FullName).Replace('\','/'); $h+'  '+$r}; $lines ^| Set-Content -Encoding ascii (Join-Path $root 'manifest.sha256'); Compress-Archive -Path $root -DestinationPath '%PKG_DIR%.zip' -CompressionLevel Optimal"
if errorlevel 1 exit /b 1

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$h=(Get-FileHash -Algorithm SHA256 '%PKG_DIR%.zip').Hash.ToLowerInvariant(); ($h+'  %PKG_DIR%.zip') ^| Set-Content -Encoding ascii '%PKG_DIR%.zip.sha256'; New-Item -ItemType Directory -Force -Path 'installer-stage' ^| Out-Null; Copy-Item '%PKG_DIR%\VekInstaller.exe','build-installer\vek.exe','%PKG_DIR%\VERSION','%PKG_DIR%\REPOSITORY.txt','UPDATE_POLICY','release-version.json','README.md' 'installer-stage' -Force; if(Test-Path 'VekInstaller.zip'){Remove-Item 'VekInstaller.zip' -Force}; Compress-Archive -Path 'installer-stage\*' -DestinationPath 'VekInstaller.zip' -CompressionLevel Optimal; $ih=(Get-FileHash -Algorithm SHA256 'VekInstaller.zip').Hash.ToLowerInvariant(); ($ih+'  VekInstaller.zip') ^| Set-Content -Encoding ascii 'VekInstaller.zip.sha256'; Remove-Item 'installer-stage' -Recurse -Force"
if errorlevel 1 exit /b 1

"%PKG_DIR%\vek.exe" doctor
if errorlevel 1 exit /b 1
"%PKG_DIR%\vek.exe" verify
if errorlevel 1 exit /b 1

echo.
echo [VEK] Portable package created:
echo       %CD%\%PKG_DIR%.zip
echo       %CD%\%PKG_DIR%.zip.sha256
echo       %CD%\VekInstaller.zip
echo       %CD%\VekInstaller.zip.sha256
exit /b 0
