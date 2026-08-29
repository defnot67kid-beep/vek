#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "VEK::vek_runtime" for configuration ""
set_property(TARGET VEK::vek_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(VEK::vek_runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libvek_runtime.a"
  )

list(APPEND _cmake_import_check_targets VEK::vek_runtime )
list(APPEND _cmake_import_check_files_for_VEK::vek_runtime "${_IMPORT_PREFIX}/lib/libvek_runtime.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
