@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install Visual Studio 2022 Community with Desktop development with C++ and CMake tools.
  pause
  exit /b 1
)

if not exist build-tests mkdir build-tests
cmake -S . -B build-tests -A x64 -DBUILD_ROTATION_TESTS=ON -DBUILD_HUMANOID_TESTS=ON -DBUILD_MAP_TESTS=ON -DBUILD_VEK_TESTS=ON -DBUILD_COLLISION_TESTS=ON -DBUILD_ESCAPE_MENU_TESTS=ON -DBUILD_VEK_SECURITY_TESTS=ON -DBUILD_GUARDED_VALUE_TESTS=ON -DBUILD_SECURE_SAVE_TESTS=ON -DBUILD_VEK_CHARACTER_RULES_TESTS=ON -DBUILD_VEK_EDITOR_RULES_TESTS=ON -DBUILD_VEK_MAIN_MENU_TESTS=ON -DBUILD_VEK_CAMERA_WORLD_TESTS=ON
if errorlevel 1 pause & exit /b 1
cmake --build build-tests --config Release
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Character + Camera Rotation =====
"build-tests\Release\CharacterRotationTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Humanoid =====
"build-tests\Release\HumanoidSystemTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Map =====
"build-tests\Release\MapSystemTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== VEK Language =====
"build-tests\Release\VekLanguageTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== VEK Collision =====
"build-tests\Release\CollisionSystemTests.exe"
if errorlevel 1 pause & exit /b 1


echo.
echo ===== VEK Character Gameplay Rules =====
"build-tests\Release\VekCharacterRulesTests.exe"
if errorlevel 1 pause & exit /b 1


echo.
echo ===== VEK Vehicle Editor Rules =====
"build-tests\Release\VekVehicleEditorRulesTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== VEK Main Menu GUI =====
"build-tests\Release\VekMainMenuScriptTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== VEK Camera / Sky / Rig =====
"build-tests\Release\VekCameraWorldTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Escape Menu =====
"build-tests\Release\EscapeMenuTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo.
echo ===== VEK Guard Security =====
"build-tests\Release\VekSecurityTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Guarded Runtime Values =====
"build-tests\Release\GuardedValueTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ===== Secure Save Integrity =====
"build-tests\Release\SecureSaveTests.exe"
if errorlevel 1 pause & exit /b 1

echo.
echo ALL SYSTEM TESTS PASSED.
pause
