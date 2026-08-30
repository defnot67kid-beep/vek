@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build -A x64 -DVEK_BUILD_CLI=ON -DVEK_BUILD_TESTS=ON
if errorlevel 1 goto :fail
cmake --build build --config Release
if errorlevel 1 goto :fail
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto :fail
echo.
echo VEK build complete: build\Release\vek.exe
pause
exit /b 0
:fail
echo VEK build failed.
pause
exit /b 1
