cmake_minimum_required(VERSION 3.13)
project(depz_sensor_sdk_c VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Are we the top-level project (standalone) or added via
# add_subdirectory()/FetchContent by a consumer?
if(NOT DEFINED DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL ON)
  else()
    set(DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL OFF)
  endif()
endif()

# Build our own test suite by default only when standalone; consumers who pull
# us in via add_subdirectory/FetchContent should not build (or need the vectors
# for) our tests.
option(DEPZ_SENSOR_SDK_C_BUILD_TESTS
       "Build the depz-sensor-sdk-c test suite"
       ${DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(GNUInstallDirs)

# --- SDK library ----------------------------------------------------------
add_library(depz_sensor_sdk
  src/crc.c
  src/framing.c
  src/usb_ids.c
  src/identity.c
  src/common.c
  src/sr04.c
  src/fwdepz.c
  # sensor-decode layer (contract-first, verifiable):
  src/vl53l8_decode.c
  src/vl53l8_advanced.c
  src/bno086_shtp.c
  src/bno086_reports.c
  src/dataset.c
)

# Namespaced alias so in-tree consumers (add_subdirectory/FetchContent) link the
# exact same target name as installed/find_package consumers.
add_library(depz::sensor_sdk_c ALIAS depz_sensor_sdk)

# Only apply our warning flags when we own the build; don't force them on
# consumers who embed us.
if(DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL)
  target_compile_options(depz_sensor_sdk PRIVATE -Wall -Wextra)
endif()

target_include_directories(depz_sensor_sdk PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(depz_sensor_sdk PROPERTIES
  OUTPUT_NAME depz_sensor_sdk
  VERSION ${PROJECT_VERSION}
  # Exported target is depz::sensor_sdk_c (namespace depz:: + this EXPORT_NAME).
  EXPORT_NAME sensor_sdk_c)

# dataset.c / vl53l8_advanced.c pull in libm (round()).
find_library(DEPZ_LIBM m)
if(DEPZ_LIBM)
  target_link_libraries(depz_sensor_sdk PUBLIC ${DEPZ_LIBM})
endif()

# --- Install / export -----------------------------------------------------
include(CMakePackageConfigHelpers)

set(DEPZ_SENSOR_SDK_C_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/depz-sensor-sdk-c"
    CACHE STRING "Install location for depz-sensor-sdk-c CMake package files")

install(TARGETS depz_sensor_sdk
  EXPORT depz-sensor-sdk-c-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")

install(EXPORT depz-sensor-sdk-c-targets
  FILE depz-sensor-sdk-c-targets.cmake
  NAMESPACE depz::
  DESTINATION ${DEPZ_SENSOR_SDK_C_INSTALL_CMAKEDIR})

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/depz-sensor-sdk-c-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-c-config.cmake
  INSTALL_DESTINATION ${DEPZ_SENSOR_SDK_C_INSTALL_CMAKEDIR})

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-c-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-c-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-c-config-version.cmake
  DESTINATION ${DEPZ_SENSOR_SDK_C_INSTALL_CMAKEDIR})

# --- Tests ----------------------------------------------------------------
if(DEPZ_SENSOR_SDK_C_BUILD_TESTS)
  enable_testing()

  # Golden vectors live in the monorepo's contracts/ directory.
  set(DEPZ_VECTORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/vectors"
      CACHE PATH "Path to contracts/vectors")

  add_executable(test_vectors
    tests/test_vectors.c
    tests/json.c
  )
  target_link_libraries(test_vectors PRIVATE depz_sensor_sdk)
  target_include_directories(test_vectors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_compile_definitions(test_vectors PRIVATE
    DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}")

  # One CTest per consumed vector / replay target: the transport+protocol
  # foundation (8) plus the sensor-decode layer (6) across the four sensors
  # (SR04, VL53L8CX, VL53L8CH, BNO086). The vl53l8_* targets cover the CX/CH-
  # shared decode plus the CH-only CNH decode: vl53l8_advanced = shared DCI
  # codecs, vl53l8_replay = a CH device capture through the shared frame decoder,
  # vl53l8_cnh = CH compact-network-histogram decode (live-capture golden vector).
  foreach(stem
      crc usb_ids identity common_commands sr04 framing_encode framing_decode fwdepz
      vl53l8_advanced bno086_shtp bno086_reports vl53l8_replay vl53l8_cnh dataset)
    add_test(NAME vec_${stem} COMMAND test_vectors ${stem})
  endforeach()
endif()
