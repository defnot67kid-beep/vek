cmake_minimum_required(VERSION 3.22)

# VERSION is the single source of truth for the VEK release number.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VEK_VERSION LIMIT_COUNT 1)
string(STRIP "${VEK_VERSION}" VEK_VERSION)
if(NOT VEK_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a semantic version such as 3.0.1; got '${VEK_VERSION}'")
endif()
project(VEK VERSION ${VEK_VERSION} LANGUAGES C CXX)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vek/VekVersion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/vek/VekVersion.h"
    @ONLY)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

option(VEK_BUILD_CLI "Build the vek command-line executable" ON)
option(VEK_BUILD_INSTALLER "Build the native Windows VEK installer" ON)
option(VEK_BUILD_TESTS "Build VEK tests" ON)
option(VEK_BUILD_C_ABI "Build the stable C ABI shared library" ON)
option(VEK_PORTABLE_STATIC_MSVC_RUNTIME "Use the static MSVC runtime for portable Windows builds" OFF)

if(MSVC AND VEK_PORTABLE_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(vek_runtime STATIC
    src/VekScriptEngine.cpp
    src/VekGameSystems.cpp
    src/VekEditorSystems.cpp
    src/VekSdkSystems.cpp
    src/VekAuthoritySystems.cpp
    src/VekPhysicsSystems.cpp
    src/VekInteractiveUiSystems.cpp
    src/VekDiagnosticsSystems.cpp
    src/VekRuntimeSystems.cpp
    src/VekGuiFramework.cpp
    src/VekUiReactive.cpp
    src/VekUiStyle.cpp
    src/VekUiCommands.cpp
    src/VekUiVirtualization.cpp
)
target_include_directories(vek_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(vek_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME Runtime)
add_library(VEK::Runtime ALIAS vek_runtime)

if(VEK_BUILD_C_ABI)
    add_library(vek_c SHARED src/vek_c.cpp)
    target_link_libraries(vek_c PRIVATE VEK::Runtime)
    target_include_directories(vek_c PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_compile_definitions(vek_c PRIVATE VEK_C_EXPORTS=1)
    set_target_properties(vek_c PROPERTIES OUTPUT_NAME "vek")
    add_library(VEK::CABI ALIAS vek_c)
endif()

if(VEK_BUILD_CLI)
    add_executable(vek_cli cli/main.cpp)
    target_link_libraries(vek_cli PRIVATE VEK::Runtime)
    set_target_properties(vek_cli PROPERTIES OUTPUT_NAME "vek")
endif()

if(WIN32 AND VEK_BUILD_CLI AND VEK_BUILD_INSTALLER)
    add_executable(vek_installer WIN32 tools/installer/main.cpp)
    target_link_libraries(vek_installer PRIVATE comctl32 ole32 shell32 advapi32)
    set_target_properties(vek_installer PROPERTIES OUTPUT_NAME "VekInstaller")
endif()

if(VEK_BUILD_TESTS)
    enable_testing()
    add_executable(vek_tests tests/vek_tests.cpp)
    target_link_libraries(vek_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_language COMMAND vek_tests)

    add_executable(vek_game_systems_tests tests/game_systems_tests.cpp)
    target_link_libraries(vek_game_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_game_systems COMMAND vek_game_systems_tests)

    add_executable(vek_editor_systems_tests tests/editor_systems_tests.cpp)
    target_link_libraries(vek_editor_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_editor_systems COMMAND vek_editor_systems_tests)

    add_executable(vek_v13_tests tests/v13_language_tests.cpp)
    target_link_libraries(vek_v13_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_v13_features COMMAND vek_v13_tests)

    add_executable(vek_interaction_systems_tests tests/interaction_systems_tests.cpp)
    target_link_libraries(vek_interaction_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_interaction_gui COMMAND vek_interaction_systems_tests)

    add_executable(vek_authority_security_tests tests/authority_security_tests.cpp)
    target_link_libraries(vek_authority_security_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_authority_security COMMAND vek_authority_security_tests)

    add_executable(vek_physics_systems_tests tests/physics_systems_tests.cpp)
    target_link_libraries(vek_physics_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_secondary_motion_physics COMMAND vek_physics_systems_tests)

    add_executable(vek_interactive_ui_systems_tests tests/interactive_ui_systems_tests.cpp)
    target_link_libraries(vek_interactive_ui_systems_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_interactive_ui COMMAND vek_interactive_ui_systems_tests)


    add_executable(vek_diagnostics_runtime_tests tests/diagnostics_runtime_tests.cpp)
    target_link_libraries(vek_diagnostics_runtime_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_diagnostics_runtime COMMAND vek_diagnostics_runtime_tests)

    add_executable(vek_language_v27_tests tests/language_v27_tests.cpp)
    target_link_libraries(vek_language_v27_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_language_v27 COMMAND vek_language_v27_tests)

    add_executable(vek_runtime_platform_tests tests/runtime_platform_tests.cpp)
    target_link_libraries(vek_runtime_platform_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_runtime_platform COMMAND vek_runtime_platform_tests)

    add_executable(vek_gui_framework_tests tests/gui_framework_tests.cpp)
    target_link_libraries(vek_gui_framework_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_gui_framework_v31 COMMAND vek_gui_framework_tests)

    add_executable(vek_ui_next_tests tests/ui_next_tests.cpp)
    target_link_libraries(vek_ui_next_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_ui_next COMMAND vek_ui_next_tests)

    add_executable(vek_modern_gui_visual_tests tests/modern_gui_visual_tests.cpp)
    target_link_libraries(vek_modern_gui_visual_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_modern_gui_visual COMMAND vek_modern_gui_visual_tests)

    add_executable(vek_camera_physics_v32_tests tests/camera_physics_v32_tests.cpp)
    target_link_libraries(vek_camera_physics_v32_tests PRIVATE VEK::Runtime)
    add_test(NAME vek_camera_physics_v32 COMMAND vek_camera_physics_v32_tests)

    add_executable(vek_version_system_tests tests/version_system_tests.cpp)
    target_link_libraries(vek_version_system_tests PRIVATE VEK::Runtime)
    target_compile_definitions(vek_version_system_tests PRIVATE VEK_EXPECTED_VERSION=\"${VEK_VERSION}\")
    add_test(NAME vek_version_system COMMAND vek_version_system_tests)

    if(VEK_BUILD_C_ABI)
        add_executable(vek_c_api_tests tests/c_api_tests.c)
        target_link_libraries(vek_c_api_tests PRIVATE vek_c)
        add_test(NAME vek_c_api COMMAND vek_c_api_tests)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS vek_runtime EXPORT VEKTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(VEK_BUILD_C_ABI)
  install(TARGETS vek_c
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(VEK_BUILD_CLI)
  install(TARGETS vek_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND VEK_BUILD_CLI AND VEK_BUILD_INSTALLER)
  install(TARGETS vek_installer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} PATTERN "VekVersion.h.in" EXCLUDE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/vek/VekVersion.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vek)
install(EXPORT VEKTargets FILE VEKTargets.cmake NAMESPACE VEK:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VEK)

# Files installed for relocatable/portable distributions.
install(FILES VERSION LICENSE README.md INSTALL_PATH.cmd UNINSTALL_PATH.cmd BUILD_PORTABLE_WINDOWS.bat DESTINATION .)
install(DIRECTORY examples/ DESTINATION examples)
install(DIRECTORY docs/ DESTINATION docs)
