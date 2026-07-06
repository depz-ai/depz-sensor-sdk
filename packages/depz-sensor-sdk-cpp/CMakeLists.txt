cmake_minimum_required(VERSION 3.16)
project(depz_sensor_sdk_cpp VERSION 0.1.2 LANGUAGES CXX)

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
  src/bno086.cpp
  src/dataset.cpp
)
target_include_directories(depz_sensor_sdk PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(depz_sensor_sdk PUBLIC cxx_std_17)
target_compile_options(depz_sensor_sdk PRIVATE -Wall -Wextra -Wpedantic)
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
if(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS)
  enable_testing()

  # Resolve the shared golden-vectors directory (packages/../../contracts/vectors).
  set(DEPZ_VECTORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/vectors"
      CACHE PATH "Path to contracts/vectors golden vectors")

  add_executable(depz_test_vectors tests/test_vectors.cpp)
  target_link_libraries(depz_test_vectors PRIVATE depz_sensor_sdk)
  target_include_directories(depz_test_vectors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_compile_definitions(depz_test_vectors PRIVATE
    DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}"
    DEPZ_RECORDINGS_DIR="${DEPZ_VECTORS_DIR}/recordings")
  target_compile_options(depz_test_vectors PRIVATE -Wall -Wextra)

  # The foundation parity suite (crc/framing/usb/common/identity/sr04/fwdepz).
  add_test(NAME vectors COMMAND depz_test_vectors)

  # Sensor-decode layer: each area is its own ctest target (same runner, a suite
  # selector argument picks which checks run).
  add_test(NAME vl53l8_frame    COMMAND depz_test_vectors vl53l8_frame)
  add_test(NAME vl53l8_advanced COMMAND depz_test_vectors vl53l8_advanced)
  add_test(NAME vl53l8_cnh       COMMAND depz_test_vectors vl53l8_cnh)
  add_test(NAME bno086_shtp     COMMAND depz_test_vectors bno086_shtp)
  add_test(NAME bno086_reports  COMMAND depz_test_vectors bno086_reports)
  add_test(NAME dataset         COMMAND depz_test_vectors dataset)
endif()
