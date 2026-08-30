# ResolveVEK.cmake
# CustomVehicleGame v26.4
#
# Stable runtime resolution model:
#   1. Discover the newest compatible verified runtime under C:/vek/versions.
#   2. Also support the managed checkout at C:/vek/repo.
#   3. Check the official GitHub repository for the newest stable vX.Y.Z tag.
#   4. If a newer stable release exists, use that release for this build.
#   5. If GitHub is unavailable, fall back to the newest valid local version.
#
# C:/vek/updates is intentionally NEVER used as a runtime source. It is only
# a staging/download area and may contain incomplete files.

function(_vek_normalize_repo_url input_url output_var)
  set(_url "${input_url}")
  string(STRIP "${_url}" _url)
  string(REGEX REPLACE "^git@github\\.com:" "https://github.com/" _url "${_url}")
  string(REGEX REPLACE "\\.git/?$" "" _url "${_url}")
  string(REGEX REPLACE "/$" "" _url "${_url}")
  string(TOLOWER "${_url}" _url)
  set(${output_var} "${_url}" PARENT_SCOPE)
endfunction()

function(_vek_read_first_line file_path output_var)
  set(_value "")
  if(EXISTS "${file_path}")
    file(STRINGS "${file_path}" _lines LIMIT_COUNT 1)
    if(_lines)
      list(GET _lines 0 _value)
      string(STRIP "${_value}" _value)
    endif()
  endif()
  set(${output_var} "${_value}" PARENT_SCOPE)
endfunction()

function(_vek_normalize_version input_version output_var)
  set(_v "${input_version}")
  string(STRIP "${_v}" _v)
  string(REGEX REPLACE "^[vV]" "" _v "${_v}")
  if(NOT _v MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    set(_v "")
  endif()
  set(${output_var} "${_v}" PARENT_SCOPE)
endfunction()

function(_vek_has_runtime_layout root_dir output_var)
  set(_required
    "${root_dir}/CMakeLists.txt"
    "${root_dir}/VERSION"
    "${root_dir}/include/vek/VekScriptEngine.h"
    "${root_dir}/include/vek/VekAuthoritySystems.h"
    "${root_dir}/include/vek/VekPhysicsSystems.h"
    "${root_dir}/src/VekScriptEngine.cpp"
    "${root_dir}/src/VekGameSystems.cpp"
    "${root_dir}/src/VekEditorSystems.cpp"
    "${root_dir}/src/VekSdkSystems.cpp"
    "${root_dir}/src/VekAuthoritySystems.cpp"
    "${root_dir}/src/VekPhysicsSystems.cpp"
  )
  set(_ok TRUE)
  foreach(_path IN LISTS _required)
    if(NOT EXISTS "${_path}")
      set(_ok FALSE)
    endif()
  endforeach()
  set(${output_var} ${_ok} PARENT_SCOPE)
endfunction()

function(_vek_latest_remote_stable output_version output_tag)
  set(_latest "")
  if(GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" ls-remote --tags --refs "${VEK_GIT_REPOSITORY}" "refs/tags/v*"
      OUTPUT_VARIABLE _remote_tags
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _tag_result
      ERROR_QUIET
      TIMEOUT 20
    )
    if(_tag_result EQUAL 0 AND _remote_tags)
      string(REPLACE "\r\n" "\n" _remote_tags "${_remote_tags}")
      string(REPLACE "\n" ";" _tag_lines "${_remote_tags}")
      foreach(_line IN LISTS _tag_lines)
        if(_line MATCHES "refs/tags/v([0-9]+\\.[0-9]+\\.[0-9]+)$")
          set(_candidate "${CMAKE_MATCH_1}")
          if(NOT _latest OR _candidate VERSION_GREATER _latest)
            set(_latest "${_candidate}")
          endif()
        endif()
      endforeach()
    endif()
  endif()
  set(${output_version} "${_latest}" PARENT_SCOPE)
  if(_latest)
    set(${output_tag} "v${_latest}" PARENT_SCOPE)
  else()
    set(${output_tag} "" PARENT_SCOPE)
  endif()
endfunction()

function(_vek_validate_candidate root_dir output_ok output_reason)
  set(_ok FALSE)
  set(_reason "")

  if(EXISTS "${root_dir}/.git")
    if(NOT GIT_FOUND)
      set(_reason "Git unavailable for managed-runtime verification")
    else()
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${root_dir}" remote get-url origin
        OUTPUT_VARIABLE _origin
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _origin_result
        ERROR_QUIET
      )
      _vek_normalize_repo_url("${_origin}" _origin_norm)
      _vek_normalize_repo_url("${VEK_GIT_REPOSITORY}" _required_origin_norm)

      if(NOT _origin_result EQUAL 0)
        set(_reason "no readable origin remote")
      elseif(NOT _origin_norm STREQUAL _required_origin_norm)
        set(_reason "wrong origin ${_origin}")
      else()
        execute_process(
          COMMAND "${GIT_EXECUTABLE}" -C "${root_dir}" status --porcelain --untracked-files=no
          OUTPUT_VARIABLE _changes
          OUTPUT_STRIP_TRAILING_WHITESPACE
          RESULT_VARIABLE _status_result
          ERROR_QUIET
        )
        if(NOT _status_result EQUAL 0)
          set(_reason "Git status verification failed")
        elseif(_changes)
          set(_reason "tracked runtime files are modified")
        else()
          set(_ok TRUE)
        endif()
      endif()
    endif()
  elseif(EXISTS "${root_dir}/.vek-verified")
    # Version snapshots installed from a checksum-verified release can use this
    # marker. The installer should write it only after verification completes.
    set(_ok TRUE)
  else()
    set(_reason "runtime has neither .git metadata nor .vek-verified marker")
  endif()

  set(${output_ok} ${_ok} PARENT_SCOPE)
  set(${output_reason} "${_reason}" PARENT_SCOPE)
endfunction()

function(_vek_consider_candidate candidate_root)
  if(NOT candidate_root)
    return()
  endif()

  _vek_has_runtime_layout("${candidate_root}" _layout_ok)
  if(NOT _layout_ok)
    return()
  endif()

  _vek_read_first_line("${candidate_root}/VERSION" _raw_version)
  _vek_normalize_version("${_raw_version}" _candidate_version)
  if(NOT _candidate_version OR _candidate_version VERSION_LESS VEK_MIN_VERSION)
    return()
  endif()

  _vek_validate_candidate("${candidate_root}" _candidate_ok _candidate_reason)
  if(NOT _candidate_ok)
    message(STATUS "VEK: ignoring ${candidate_root}: ${_candidate_reason}")
    return()
  endif()

  if(NOT _VEK_BEST_LOCAL_VERSION OR _candidate_version VERSION_GREATER _VEK_BEST_LOCAL_VERSION)
    set(_VEK_BEST_LOCAL_ROOT "${candidate_root}" PARENT_SCOPE)
    set(_VEK_BEST_LOCAL_VERSION "${_candidate_version}" PARENT_SCOPE)
  endif()
endfunction()

function(resolve_vek_dependency)
  if(NOT DEFINED VEK_GIT_REPOSITORY OR NOT VEK_GIT_REPOSITORY)
    message(FATAL_ERROR "resolve_vek_dependency requires VEK_GIT_REPOSITORY")
  endif()
  if(NOT DEFINED VEK_GIT_TAG OR NOT VEK_GIT_TAG)
    set(VEK_GIT_TAG "latest")
  endif()
  if(NOT DEFINED VEK_MIN_VERSION OR NOT VEK_MIN_VERSION)
    set(VEK_MIN_VERSION "2.7.1")
  endif()
  if(NOT DEFINED VEK_INSTALL_ROOT OR NOT VEK_INSTALL_ROOT)
    set(VEK_INSTALL_ROOT "C:/vek")
  endif()

  find_package(Git QUIET)
  _vek_latest_remote_stable(_remote_stable_version _remote_stable_tag)

  if(_remote_stable_version)
    message(STATUS "VEK: latest stable release is v${_remote_stable_version}")
  else()
    message(STATUS "VEK: stable-release lookup unavailable; local/offline resolution enabled")
  endif()

  set(_VEK_BEST_LOCAL_ROOT "")
  set(_VEK_BEST_LOCAL_VERSION "")

  if(VEK_PREFER_INSTALLED)
    message(STATUS "VEK: scanning ${VEK_INSTALL_ROOT}/versions")

    _vek_read_first_line("${VEK_INSTALL_ROOT}/CURRENT_VERSION" _current_raw)
    _vek_normalize_version("${_current_raw}" _current_version)
    if(_current_version)
      foreach(_candidate
        "${VEK_INSTALL_ROOT}/versions/v${_current_version}/repo"
        "${VEK_INSTALL_ROOT}/versions/v${_current_version}/runtime"
        "${VEK_INSTALL_ROOT}/versions/v${_current_version}"
      )
        _vek_consider_candidate("${_candidate}")
      endforeach()
    endif()

    if(EXISTS "${VEK_INSTALL_ROOT}/versions")
      file(GLOB _version_dirs LIST_DIRECTORIES TRUE "${VEK_INSTALL_ROOT}/versions/*")
      list(SORT _version_dirs COMPARE NATURAL ORDER DESCENDING)
      foreach(_dir IN LISTS _version_dirs)
        if(IS_DIRECTORY "${_dir}")
          foreach(_candidate "${_dir}/repo" "${_dir}/runtime" "${_dir}")
            _vek_consider_candidate("${_candidate}")
          endforeach()
        endif()
      endforeach()
    endif()

    # Legacy/current layout remains supported.
    _vek_consider_candidate("${VEK_INSTALL_ROOT}/repo")
    _vek_consider_candidate("${VEK_INSTALL_ROOT}")
  endif()

  if(_VEK_BEST_LOCAL_ROOT)
    message(STATUS "VEK: newest verified local runtime is v${_VEK_BEST_LOCAL_VERSION} at ${_VEK_BEST_LOCAL_ROOT}")
  else()
    message(STATUS "VEK: no compatible verified local runtime found")
  endif()

  set(_desired_tag "${VEK_GIT_TAG}")
  if(VEK_GIT_TAG STREQUAL "latest")
    if(_remote_stable_tag)
      set(_desired_tag "${_remote_stable_tag}")
    else()
      set(_desired_tag "main")
    endif()
  endif()

  set(_use_local FALSE)
  if(_VEK_BEST_LOCAL_ROOT)
    if(VEK_GIT_TAG STREQUAL "latest" AND _remote_stable_version)
      if(_VEK_BEST_LOCAL_VERSION VERSION_LESS _remote_stable_version)
        message(STATUS "VEK: local v${_VEK_BEST_LOCAL_VERSION} is behind v${_remote_stable_version}; using newer stable release")
      else()
        set(_use_local TRUE)
      endif()
    else()
      set(_use_local TRUE)
    endif()
  endif()

  set(VEK_BUILD_CLI OFF CACHE BOOL "" FORCE)
  set(VEK_BUILD_INSTALLER OFF CACHE BOOL "" FORCE)
  set(VEK_BUILD_TESTS OFF CACHE BOOL "" FORCE)

  # A source release also carries a known-compatible VEK snapshot. This is an
  # offline-safe fallback, not an update staging directory. It prevents a game
  # source ZIP from becoming unbuildable just because GitHub is unavailable or
  # the user's installed runtime is older than this game's required schema.
  set(_bundled_vek_root "${CMAKE_SOURCE_DIR}/external/vek")
  _vek_has_runtime_layout("${_bundled_vek_root}" _bundled_layout_ok)
  _vek_read_first_line("${_bundled_vek_root}/VERSION" _bundled_raw)
  _vek_normalize_version("${_bundled_raw}" _bundled_version)
  set(_bundled_compatible FALSE)
  if(_bundled_layout_ok AND _bundled_version AND NOT _bundled_version VERSION_LESS VEK_MIN_VERSION)
    set(_bundled_compatible TRUE)
  endif()

  if(_use_local)
    add_subdirectory("${_VEK_BEST_LOCAL_ROOT}" "${CMAKE_BINARY_DIR}/_deps/vek-installed-build" EXCLUDE_FROM_ALL)
    set(_vek_source_dir "${_VEK_BEST_LOCAL_ROOT}")
    set(_vek_resolution_mode "installed-version")
  elseif(_bundled_compatible AND (NOT _remote_stable_version OR _remote_stable_version VERSION_LESS VEK_MIN_VERSION))
    message(STATUS "VEK: using bundled compatible runtime v${_bundled_version}")
    add_subdirectory("${_bundled_vek_root}" "${CMAKE_BINARY_DIR}/_deps/vek-bundled-build" EXCLUDE_FROM_ALL)
    set(_vek_source_dir "${_bundled_vek_root}")
    set(_vek_resolution_mode "bundled-source")
  else()
    message(STATUS "VEK: fetching ${VEK_GIT_REPOSITORY} @ ${_desired_tag}")
    FetchContent_Declare(vek_external
      GIT_REPOSITORY "${VEK_GIT_REPOSITORY}"
      GIT_TAG "${_desired_tag}"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(vek_external)
    set(_vek_source_dir "${vek_external_SOURCE_DIR}")
    set(_vek_resolution_mode "github-release")
  endif()

  if(NOT TARGET VEK::Runtime AND TARGET vek_runtime)
    add_library(VEK::Runtime ALIAS vek_runtime)
  endif()

  if(NOT TARGET VEK::Runtime)
    _vek_has_runtime_layout("${_vek_source_dir}" _resolved_layout_ok)
    if(_resolved_layout_ok)
      add_library(vek_runtime_compat STATIC
        "${_vek_source_dir}/src/VekScriptEngine.cpp"
        "${_vek_source_dir}/src/VekGameSystems.cpp"
        "${_vek_source_dir}/src/VekEditorSystems.cpp"
        "${_vek_source_dir}/src/VekSdkSystems.cpp"
        "${_vek_source_dir}/src/VekAuthoritySystems.cpp"
        "${_vek_source_dir}/src/VekPhysicsSystems.cpp"
      )
      target_include_directories(vek_runtime_compat PUBLIC "${_vek_source_dir}/include")
      set_target_properties(vek_runtime_compat PROPERTIES POSITION_INDEPENDENT_CODE ON)
      add_library(VEK::Runtime ALIAS vek_runtime_compat)
    endif()
  endif()

  if(NOT TARGET VEK::Runtime)
    message(FATAL_ERROR "VEK resolved from ${_vek_resolution_mode}, but no compatible runtime target exists")
  endif()

  _vek_read_first_line("${_vek_source_dir}/VERSION" _resolved_raw)
  _vek_normalize_version("${_resolved_raw}" _resolved_version)
  if(NOT _resolved_version)
    message(FATAL_ERROR "Resolved VEK runtime has no valid VERSION file: ${_vek_source_dir}")
  endif()
  if(_resolved_version VERSION_LESS VEK_MIN_VERSION)
    message(FATAL_ERROR "CustomVehicleGame requires VEK ${VEK_MIN_VERSION}+; resolved ${_resolved_version}")
  endif()

  message(STATUS "VEK: resolved v${_resolved_version} via ${_vek_resolution_mode}")
  set(VEK_RESOLVED_SOURCE_DIR "${_vek_source_dir}" PARENT_SCOPE)
  set(VEK_RESOLVED_VERSION "${_resolved_version}" PARENT_SCOPE)
  set(VEK_RESOLUTION_MODE "${_vek_resolution_mode}" PARENT_SCOPE)
endfunction()
