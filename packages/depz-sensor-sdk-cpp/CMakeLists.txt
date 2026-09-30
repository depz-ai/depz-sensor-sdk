cmake_minimum_required(VERSION 3.16)
project(depz_sensor_sdk_cpp VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect standalone vs. subordinate (add_subdirectory / FetchContent) build.
# PROJECT_IS_TOP_LEVEL is CMake >= 3.21; fall back to a source-dir compare.
if(DEFINED PROJECT_IS_TOP_LEVEL)
  set(_depz_is_top_level ${PROJECT_IS_TOP_LEVEL})
elseif(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(_depz_is_top_level ON)
else()
  set(_depz_is_top_level OFF)
endif()

# Tests build standalone by default, off when consumed as a subproject.
option(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS
       "Build the depz-sensor-sdk-cpp test suite" ${_depz_is_top_level})

if(_depz_is_top_level AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(GNUInstallDirs)

# ---- library ---------------------------------------------------------------
add_library(depz_sensor_sdk
  src/crc.cpp
  src/framing.cpp
  src/usb_ids.cpp
  src/common.cpp
  src/identity.cpp
  src/sr04.cpp
  src/fwdepz.cpp
  src/vl53l8.cpp
  src/vl53l7.cpp
  src/vl53l4.cpp
  src/vl53lx.cpp
  src/bno055.cpp
  src/bno086.cpp
  src/dataset.cpp
)
target_include_directories(depz_sensor_sdk PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(depz_sensor_sdk PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(depz_sensor_sdk PRIVATE /W3)
else()
  target_compile_options(depz_sensor_sdk PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- live hardware (depz/device.hpp) ---------------------------------------
# A C++ wrapper over the C SDK's depz_sensor_io.h: the C library comes from an
# installed package, else from the sibling directory of this repository.
option(DEPZ_SENSOR_SDK_CPP_BUILD_IO "Build the live-hardware layer (depz/device.hpp)" ON)
if(DEPZ_SENSOR_SDK_CPP_BUILD_IO)
  if(NOT TARGET depz::sensor_sdk_c)
    find_package(depz-sensor-sdk-c CONFIG QUIET)
  endif()
  if(NOT TARGET depz::sensor_sdk_c)
    set(_depz_c_dir "${CMAKE_CURRENT_SOURCE_DIR}/../depz-sensor-sdk-c")
    if(NOT EXISTS "${_depz_c_dir}/CMakeLists.txt")
      message(FATAL_ERROR "depz-sensor-sdk-cpp: the live-hardware layer needs depz-sensor-sdk-c "
                          "(install it, or pass -DDEPZ_SENSOR_SDK_CPP_BUILD_IO=OFF)")
    endif()
    enable_language(C)
    set(DEPZ_SENSOR_SDK_C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(DEPZ_SENSOR_SDK_C_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${_depz_c_dir}" "${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-c")
  endif()
  target_sources(depz_sensor_sdk PRIVATE src/device.cpp)
  target_link_libraries(depz_sensor_sdk PUBLIC depz::sensor_sdk_c)
endif()
set_target_properties(depz_sensor_sdk PROPERTIES
  OUTPUT_NAME depz_sensor_sdk
  EXPORT_NAME sensor_sdk_cpp)

# Namespaced aliases for in-tree consumers (FetchContent / add_subdirectory).
# depz::sensor_sdk_cpp is the canonical, package-matching name; depz::sensor_sdk
# is kept for backwards compatibility.
add_library(depz::sensor_sdk_cpp ALIAS depz_sensor_sdk)
add_library(depz::sensor_sdk ALIAS depz_sensor_sdk)

# ---- install / export ------------------------------------------------------
set(DEPZ_SENSOR_SDK_CPP_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/depz-sensor-sdk-cpp"
    CACHE STRING "Install location for depz-sensor-sdk-cpp CMake package files")

install(TARGETS depz_sensor_sdk
  EXPORT depz-sensor-sdk-cpp-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT depz-sensor-sdk-cpp-targets
  FILE depz-sensor-sdk-cpp-targets.cmake
  NAMESPACE depz::
  DESTINATION ${DEPZ_SENSOR_SDK_CPP_INSTALL_CMAKEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/depz-sensor-sdk-cpp-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-cpp-config.cmake
  INSTALL_DESTINATION ${DEPZ_SENSOR_SDK_CPP_INSTALL_CMAKEDIR})
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-cpp-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-cpp-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-cpp-config-version.cmake
  DESTINATION ${DEPZ_SENSOR_SDK_CPP_INSTALL_CMAKEDIR})

# ---- docs ------------------------------------------------------------------
# Regenerate docs/api.md + docs/<sensor>/api.md from the header doc-comments:
#   cmake --build build --target docs
# Stdlib Python only; no compiler or doc tool required.
find_program(PYTHON3 NAMES python3 python)
if(PYTHON3)
  add_custom_target(docs
    COMMAND ${PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_api_md.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Regenerating C++ API reference (docs/api.md + per-sensor api.md)"
    VERBATIM)
endif()

# ---- tests -----------------------------------------------------------------
# Golden vectors live in the development tree's contracts/ directory and are
# not shipped with the packaged sources — skip the tests gracefully without them.
if(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS)
  set(DEPZ_VECTORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/vectors"
      CACHE PATH "Path to the golden-vector tree")
  if(NOT EXISTS "${DEPZ_VECTORS_DIR}")
    message(STATUS "depz-sensor-sdk-cpp: golden vectors not found at ${DEPZ_VECTORS_DIR}; tests skipped (set DEPZ_VECTORS_DIR to enable)")
    set(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS OFF)
  endif()
endif()

if(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS)
  enable_testing()

  add_executable(depz_test_vectors tests/test_vectors.cpp)
  target_link_libraries(depz_test_vectors PRIVATE depz_sensor_sdk)
  target_include_directories(depz_test_vectors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_compile_definitions(depz_test_vectors PRIVATE
    DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}"
    DEPZ_RECORDINGS_DIR="${DEPZ_VECTORS_DIR}/recordings")
  if(NOT MSVC)
    target_compile_options(depz_test_vectors PRIVATE -Wall -Wextra)
  endif()

  # The foundation parity suite (crc/framing/usb/common/identity/sr04/fwdepz).
  add_test(NAME vectors COMMAND depz_test_vectors)

  # The live-hardware wrapper against the C SDK's fake SR04 over loopback.
  set(_depz_c_tests "${CMAKE_CURRENT_SOURCE_DIR}/../depz-sensor-sdk-c/tests")
  if(DEPZ_SENSOR_SDK_CPP_BUILD_IO AND EXISTS "${_depz_c_tests}/fake_sr04.c")
    enable_language(C)
    add_executable(depz_test_device tests/test_device.cpp "${_depz_c_tests}/fake_sr04.c"
                   "${_depz_c_tests}/fake_vl53l4.c" "${_depz_c_tests}/fake_bno086.c")
    target_link_libraries(depz_test_device PRIVATE depz_sensor_sdk)
    target_include_directories(depz_test_device PRIVATE "${_depz_c_tests}" tests)
    target_compile_definitions(depz_test_device PRIVATE DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}")
    foreach(t identity_and_types errors_are_typed timeout_when_silent
              measure_stream_and_callbacks disconnect_ends_streams promote_and_record_replay
              sr04_session_replay l4_fake_roundtrips_and_guards l4_session_replay l8ch_cnh_replay
              bno_ndof_replay bno086_session_replay bno086_fake_errors_and_reports
              vlx_l4cd_uld_replay vlx_l1cx_uld_replay vlx_l3cx_ulp_replay vlx_l0x_uld_replay
              vlx_l4cx_hist_short_replay)
      add_test(NAME device_${t} COMMAND depz_test_device ${t})
    endforeach()
  endif()

  # Sensor-decode layer: each area is its own ctest target (same runner, a suite
  # selector argument picks which checks run).
  add_test(NAME vl53l8_frame    COMMAND depz_test_vectors vl53l8_frame)
  add_test(NAME vl53l8_advanced COMMAND depz_test_vectors vl53l8_advanced)
  add_test(NAME vl53l8_cnh       COMMAND depz_test_vectors vl53l8_cnh)
  add_test(NAME vl53l7_codecs   COMMAND depz_test_vectors vl53l7_codecs)
  add_test(NAME vl53l7_frame    COMMAND depz_test_vectors vl53l7_frame)
  add_test(NAME vl53l4          COMMAND depz_test_vectors vl53l4)
  add_test(NAME vl53lx          COMMAND depz_test_vectors vl53lx)
  add_test(NAME bno055          COMMAND depz_test_vectors bno055)
  add_test(NAME bno086_shtp     COMMAND depz_test_vectors bno086_shtp)
  add_test(NAME bno086_reports  COMMAND depz_test_vectors bno086_reports)
  add_test(NAME dataset         COMMAND depz_test_vectors dataset)
endif()
