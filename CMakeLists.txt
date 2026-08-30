cmake_minimum_required(VERSION 3.22)
project(CustomVehicleGame LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 6.0)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# -----------------------------------------------------------------------------
# VEK language dependency
# -----------------------------------------------------------------------------
# Resolution order:
#   1. Newest verified compatible runtime under C:\\vek\\versions.
#   2. Legacy/current managed runtime under C:\\vek\\repo.
#   3. Latest stable GitHub release if local versions are missing or behind.
# C:\\vek\\updates is staging-only and is never compiled against.
set(VEK_GIT_REPOSITORY "https://github.com/defnot67kid-beep/vek.git" CACHE STRING "Official VEK Git repository")
set(VEK_GIT_TAG "latest" CACHE STRING "VEK selector: latest, main, or explicit vX.Y.Z")
set(VEK_MIN_VERSION "2.7.1" CACHE STRING "Minimum VEK runtime version required by this game")
set(VEK_INSTALL_ROOT "C:/vek" CACHE PATH "Installed VEK root checked before cloning GitHub")
option(VEK_PREFER_INSTALLED "Use a verified current C:/vek runtime when available" ON)

# VEK_GITHUB_REPO.txt remains an explicit project-level override for forks.
if(EXISTS "${CMAKE_SOURCE_DIR}/VEK_GITHUB_REPO.txt")
  file(STRINGS "${CMAKE_SOURCE_DIR}/VEK_GITHUB_REPO.txt" _vek_repo LIMIT_COUNT 1)
  string(STRIP "${_vek_repo}" _vek_repo)
  if(_vek_repo AND NOT _vek_repo STREQUAL "LOCAL")
    set(VEK_GIT_REPOSITORY "${_vek_repo}" CACHE STRING "Official VEK Git repository" FORCE)
  endif()
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/VEK_GITHUB_TAG.txt")
  file(STRINGS "${CMAKE_SOURCE_DIR}/VEK_GITHUB_TAG.txt" _vek_tag LIMIT_COUNT 1)
  string(STRIP "${_vek_tag}" _vek_tag)
  if(_vek_tag)
    set(VEK_GIT_TAG "${_vek_tag}" CACHE STRING "VEK branch used for freshness checks and fallback clone" FORCE)
  endif()
endif()

include("${CMAKE_SOURCE_DIR}/cmake/ResolveVEK.cmake")
resolve_vek_dependency()

add_executable(CustomVehicleGame
  src/main.cpp
  src/CrashLog.cpp
  src/Game.cpp
  src/Character.cpp
  src/HumanoidSystem.cpp
  src/VekCharacterRules.cpp
  src/VekVehicleEditorRules.cpp
  src/VekPartRegistrySystem.cpp
  src/VekHangarRules.cpp
  src/VekHangarInteractionSystem.cpp
  src/VekLifeCycleSystem.cpp
  src/VekCameraWorldRules.cpp
  src/VekSkyRenderer.cpp
  src/VekAuthorityRules.cpp
  src/VekVehicleEditorGuiSystem.cpp
  src/VekGuiTextRenderer.cpp
  src/VekJobRules.cpp
  src/EditorCameraSystem.cpp
  src/MainMenuSystem.cpp
  src/CharacterRotationSystem.cpp
  src/CameraRotationSystem.cpp
  src/MapSystem.cpp
  src/SecurityHash.cpp
  src/VekSecuritySystem.cpp
  src/SecureSaveStore.cpp
  src/CollisionSystem.cpp
  src/EscapeMenuSystem.cpp
  src/WindowInputSystem.cpp
  src/WindowsPlatformHooks.cpp
  src/AvatarSaveSystem.cpp
  src/ClothingSystem.cpp
  src/CharacterCustomizationSystem.cpp
  src/CharacterInteractionSystem.cpp
  src/Vehicle.cpp
  src/PartPresentationRenderer.cpp
  src/VehicleBuilder.cpp
  src/World.cpp
  src/Jobs.cpp
  src/Save.cpp
)
target_include_directories(CustomVehicleGame PRIVATE src)
target_link_libraries(CustomVehicleGame PRIVATE raylib VEK::Runtime)

# The shipped Windows game is a GUI application: launching the EXE does not
# create a separate black console window. Test executables remain console apps.
if(WIN32)
  target_link_libraries(CustomVehicleGame PRIVATE user32 bcrypt crypt32)
  set_target_properties(CustomVehicleGame PROPERTIES WIN32_EXECUTABLE TRUE)
  # MSVC's Windows subsystem normally looks for WinMain. Keep the portable
  # C++ main() entry point and tell the CRT to start there without allocating
  # a console window.
  if(MSVC)
    target_link_options(CustomVehicleGame PRIVATE "/ENTRY:mainCRTStartup")
  endif()
endif()

# Keep VEK scripts/assets beside the executable even when no C++ translation
# unit needs rebuilding. This is intentionally an ALL target instead of only a
# POST_BUILD command: editing a .vek script should refresh runtime data without
# forcing the game executable itself to relink.
add_custom_target(CustomVehicleGameRuntimeFiles ALL
  # IMPORTANT: these runtime folders are authoritative mirrors, not merge-copy
  # destinations. A deleted/renamed .vek file must disappear from the runnable
  # game or directory-scanning systems (for example scripts/parts) can execute
  # stale code from an older ZIP. Removing only these generated runtime folders
  # keeps compiled object/library caches intact.
  COMMAND ${CMAKE_COMMAND} -E rm -rf
          "$<TARGET_FILE_DIR:CustomVehicleGame>/scripts"
  COMMAND ${CMAKE_COMMAND} -E rm -rf
          "$<TARGET_FILE_DIR:CustomVehicleGame>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/scripts"
          "$<TARGET_FILE_DIR:CustomVehicleGame>/scripts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:CustomVehicleGame>/assets"
  COMMENT "Authoritatively mirroring VEK scripts and runtime assets"
  VERBATIM
)
add_dependencies(CustomVehicleGameRuntimeFiles CustomVehicleGame)

option(VEK_DEVELOPMENT_MODE "Allow unsigned VEK source and F11 hot reload (DO NOT SHIP)" OFF)
if(VEK_DEVELOPMENT_MODE)
  target_compile_definitions(CustomVehicleGame PRIVATE VEK_DEVELOPMENT_MODE=1)
else()
  target_compile_definitions(CustomVehicleGame PRIVATE VEK_DEVELOPMENT_MODE=0)
endif()

option(BUILD_ROTATION_TESTS "Build CharacterRotationSystem verification tests" OFF)
if(BUILD_ROTATION_TESTS)
  add_executable(CharacterRotationTests
    tests/CharacterRotationTests.cpp
    src/CharacterRotationSystem.cpp
  src/CameraRotationSystem.cpp
  )
  target_include_directories(CharacterRotationTests PRIVATE src)
  target_link_libraries(CharacterRotationTests PRIVATE raylib)
endif()

option(BUILD_HUMANOID_TESTS "Build HumanoidSystem verification tests" OFF)
if(BUILD_HUMANOID_TESTS)
  add_executable(HumanoidSystemTests
    tests/HumanoidSystemTests.cpp
    src/HumanoidSystem.cpp
  )
  target_include_directories(HumanoidSystemTests PRIVATE src)
endif()

option(BUILD_MAP_TESTS "Build MapSystem verification tests" OFF)
if(BUILD_MAP_TESTS)
  add_executable(MapSystemTests
    tests/MapSystemTests.cpp
    src/MapSystem.cpp
    src/World.cpp
  )
  target_include_directories(MapSystemTests PRIVATE src)
  target_link_libraries(MapSystemTests PRIVATE raylib)
endif()


option(BUILD_VEK_TESTS "Build VEK language verification tests" OFF)
if(BUILD_VEK_TESTS)
  add_executable(VekLanguageTests
    tests/VekLanguageTests.cpp
    )
  target_include_directories(VekLanguageTests PRIVATE src)
  target_link_libraries(VekLanguageTests PRIVATE VEK::Runtime)
endif()

option(BUILD_COLLISION_TESTS "Build VEK collision system verification tests" OFF)
if(BUILD_COLLISION_TESTS)
  add_executable(CollisionSystemTests
    tests/CollisionSystemTests.cpp
    src/CollisionSystem.cpp
      src/SecurityHash.cpp
    src/VekSecuritySystem.cpp
    src/World.cpp
    src/Vehicle.cpp
    src/HumanoidSystem.cpp
  )
  target_include_directories(CollisionSystemTests PRIVATE src)
  target_link_libraries(CollisionSystemTests PRIVATE raylib VEK::Runtime)
  if(WIN32)
    target_link_libraries(CollisionSystemTests PRIVATE bcrypt)
  endif()
  target_compile_definitions(CollisionSystemTests PRIVATE VEK_TEST_SCRIPT_PATH="${CMAKE_SOURCE_DIR}/scripts/collision.vek" VEK_DEVELOPMENT_MODE=0)
endif()

option(BUILD_ESCAPE_MENU_TESTS "Build EscapeMenuSystem verification tests" OFF)
if(BUILD_ESCAPE_MENU_TESTS)
  add_executable(EscapeMenuTests
    tests/EscapeMenuTests.cpp
    src/EscapeMenuSystem.cpp
  )
  target_include_directories(EscapeMenuTests PRIVATE src)
  target_link_libraries(EscapeMenuTests PRIVATE raylib)
endif()

option(BUILD_VEK_SECURITY_TESTS "Build VEK Guard verification tests" OFF)
if(BUILD_VEK_SECURITY_TESTS)
  add_executable(VekSecurityTests
    tests/VekSecurityTests.cpp
      src/SecurityHash.cpp
    src/VekSecuritySystem.cpp
  )
  target_include_directories(VekSecurityTests PRIVATE src)
  target_link_libraries(VekSecurityTests PRIVATE VEK::Runtime)
  target_compile_definitions(VekSecurityTests PRIVATE VEK_DEVELOPMENT_MODE=0 VEK_TEST_SCRIPT_PATH="${CMAKE_SOURCE_DIR}/scripts/collision.vek")
  if(WIN32)
    target_link_libraries(VekSecurityTests PRIVATE bcrypt)
  endif()
endif()

option(BUILD_GUARDED_VALUE_TESTS "Build guarded runtime value tests" OFF)
if(BUILD_GUARDED_VALUE_TESTS)
  add_executable(GuardedValueTests tests/GuardedValueTests.cpp)
  target_include_directories(GuardedValueTests PRIVATE src)
endif()

option(BUILD_SECURE_SAVE_TESTS "Build authenticated save tests" OFF)
if(BUILD_SECURE_SAVE_TESTS)
  add_executable(SecureSaveTests tests/SecureSaveTests.cpp src/SecureSaveStore.cpp src/SecurityHash.cpp)
  target_include_directories(SecureSaveTests PRIVATE src)
  if(WIN32)
    target_link_libraries(SecureSaveTests PRIVATE bcrypt crypt32)
  endif()
endif()

option(BUILD_VEK_CHARACTER_RULES_TESTS "Build VEK character gameplay-rule tests" OFF)
if(BUILD_VEK_CHARACTER_RULES_TESTS)
  add_executable(VekCharacterRulesTests
    tests/VekCharacterRulesTests.cpp
    src/VekCharacterRules.cpp
  src/VekVehicleEditorRules.cpp
  src/VekPartRegistrySystem.cpp
  src/VekHangarRules.cpp
  src/VekHangarInteractionSystem.cpp
  src/VekLifeCycleSystem.cpp
  src/VekCameraWorldRules.cpp
  src/VekSkyRenderer.cpp
  src/VekAuthorityRules.cpp
  src/VekVehicleEditorGuiSystem.cpp
  src/VekGuiTextRenderer.cpp
  src/VekJobRules.cpp
  src/EditorCameraSystem.cpp
  src/MainMenuSystem.cpp
    src/HumanoidSystem.cpp
    src/SecurityHash.cpp
    src/VekSecuritySystem.cpp
  )
  target_include_directories(VekCharacterRulesTests PRIVATE src)
  target_link_libraries(VekCharacterRulesTests PRIVATE VEK::Runtime)
  if(WIN32)
    target_link_libraries(VekCharacterRulesTests PRIVATE bcrypt)
  endif()
  target_compile_definitions(VekCharacterRulesTests PRIVATE VEK_DEVELOPMENT_MODE=1)
endif()

option(BUILD_VEK_EDITOR_RULES_TESTS "Build VEK vehicle-editor rule tests" OFF)
if(BUILD_VEK_EDITOR_RULES_TESTS)
  add_executable(VekVehicleEditorRulesTests tests/VekVehicleEditorRulesTests.cpp src/VekVehicleEditorRules.cpp src/SecurityHash.cpp src/VekSecuritySystem.cpp)
  target_include_directories(VekVehicleEditorRulesTests PRIVATE src)
  target_link_libraries(VekVehicleEditorRulesTests PRIVATE VEK::Runtime)
  if(WIN32)
    target_link_libraries(VekVehicleEditorRulesTests PRIVATE bcrypt)
  endif()
  target_compile_definitions(VekVehicleEditorRulesTests PRIVATE VEK_DEVELOPMENT_MODE=1 VEK_TEST_EDITOR_SCRIPT_PATH="${CMAKE_SOURCE_DIR}/scripts/vehicle_editor.vek")
endif()

option(BUILD_VEK_MAIN_MENU_TESTS "Build VEK main-menu GUI script tests" OFF)
if(BUILD_VEK_MAIN_MENU_TESTS)
  add_executable(VekMainMenuScriptTests tests/VekMainMenuScriptTests.cpp)
  target_link_libraries(VekMainMenuScriptTests PRIVATE VEK::Runtime)
  target_compile_definitions(VekMainMenuScriptTests PRIVATE VEK_TEST_MENU_SCRIPT_PATH="${CMAKE_SOURCE_DIR}/scripts/main_menu.vek")
endif()

option(BUILD_VEK_CAMERA_WORLD_TESTS "Build VEK camera/world script tests" OFF)
if(BUILD_VEK_CAMERA_WORLD_TESTS)
  add_executable(VekCameraWorldTests tests/VekCameraWorldTests.cpp)
  target_link_libraries(VekCameraWorldTests PRIVATE VEK::Runtime)
  target_compile_definitions(VekCameraWorldTests PRIVATE VEK_CAMERA_WORLD_SCRIPT_PATH="${CMAKE_SOURCE_DIR}/scripts/camera_world.vek")
endif()

option(BUILD_VEK_AUTHORITY_TESTS "Build VEK 1.9 authority/shell policy tests" OFF)
if(BUILD_VEK_AUTHORITY_TESTS)
  add_executable(VekAuthorityRulesTests
    tests/VekAuthorityRulesTests.cpp
    src/VekAuthorityRules.cpp
    src/VekSecuritySystem.cpp
    src/SecurityHash.cpp
  )
  target_include_directories(VekAuthorityRulesTests PRIVATE src)
  target_link_libraries(VekAuthorityRulesTests PRIVATE VEK::Runtime)
  target_compile_definitions(VekAuthorityRulesTests PRIVATE VEK_DEVELOPMENT_MODE=1 VEK_TEST_AUTHORITY_SCRIPT="${CMAKE_SOURCE_DIR}/scripts/security_authority.vek")
  if(WIN32)
    target_link_libraries(VekAuthorityRulesTests PRIVATE bcrypt)
  endif()
endif()

option(BUILD_PART_PRESENTATION_TESTS "Build VEK part icon/view-model schema tests" OFF)
if(BUILD_PART_PRESENTATION_TESTS)
  add_executable(PartPresentationRegistryTests tests/PartPresentationRegistryTests.cpp)
  target_link_libraries(PartPresentationRegistryTests PRIVATE VEK::Runtime)
  target_compile_definitions(PartPresentationRegistryTests PRIVATE VEK_PART_SCRIPT_DIR="${CMAKE_SOURCE_DIR}/scripts/parts")
endif()
