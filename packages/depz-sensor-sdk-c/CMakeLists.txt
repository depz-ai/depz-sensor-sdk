cmake_minimum_required(VERSION 3.13)
project(depz_sensor_sdk_c VERSION 0.4.0 LANGUAGES C)

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
add_library(depz_sensor_sdk_c
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
  src/vl53l4.c
  src/vl53l7.c
  src/vl53lx.c
  src/bno055.c
  src/bno086_shtp.c
  src/bno086_reports.c
  src/bno086_sh2.c
  src/dataset.c
)

# --- Live-hardware layer (depz_sensor_io.h) -------------------------------
# Serial ports, record/replay, the device core, discovery and the sensor
# classes. Needs threads and, per OS, the port-enumeration API.
option(DEPZ_SENSOR_SDK_C_BUILD_IO "Build the live-hardware layer (depz_sensor_io.h)" ON)
if(DEPZ_SENSOR_SDK_C_BUILD_IO)
  target_sources(depz_sensor_sdk_c PRIVATE
    src/io/platform.c
    src/io/util.c
    src/io/link.c
    src/io/device.c
    src/io/discovery.c
    src/io/sr04_device.c
    src/io/regbridge.c
    src/io/vl53l4cd_device.c
    src/io/vl53l8_device.c
    src/io/vl53l8_blobs.c
    src/io/bno055_device.c
    src/io/bno086_device.c
    src/io/vl53lx_device.c
    src/io/vl53lx_die.c
    src/io/vl53lx_l0x.c
    src/io/vl53lx_bare.c
    src/io/vl53lx_bare_hist.c
    src/io/vl53lx_bare_tables.c)
  if(WIN32)
    target_sources(depz_sensor_sdk_c PRIVATE src/io/serial_win32.c src/io/ports_win32.c)
    target_link_libraries(depz_sensor_sdk_c PUBLIC setupapi cfgmgr32)
  else()
    target_sources(depz_sensor_sdk_c PRIVATE src/io/serial_posix.c)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(depz_sensor_sdk_c PUBLIC Threads::Threads)
    if(APPLE)
      target_sources(depz_sensor_sdk_c PRIVATE src/io/ports_macos.c)
      target_link_libraries(depz_sensor_sdk_c PUBLIC "-framework IOKit" "-framework CoreFoundation")
    else()
      target_sources(depz_sensor_sdk_c PRIVATE src/io/ports_linux.c)
    endif()
  endif()
endif()

# Namespaced alias so in-tree consumers (add_subdirectory/FetchContent) link the
# exact same target name as installed/find_package consumers.
add_library(depz::sensor_sdk_c ALIAS depz_sensor_sdk_c)

# Only apply our warning flags when we own the build; don't force them on
# consumers who embed us.
if(DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL)
  if(MSVC)
    target_compile_options(depz_sensor_sdk_c PRIVATE /W3)
    target_compile_definitions(depz_sensor_sdk_c PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(depz_sensor_sdk_c PRIVATE -Wall -Wextra)
  endif()
endif()

target_include_directories(depz_sensor_sdk_c PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(depz_sensor_sdk_c PROPERTIES
  OUTPUT_NAME depz_sensor_sdk_c
  VERSION ${PROJECT_VERSION}
  # Exported target is depz::sensor_sdk_c (namespace depz:: + this EXPORT_NAME).
  EXPORT_NAME sensor_sdk_c)

# dataset.c / vl53l8_advanced.c pull in libm (round()).
find_library(DEPZ_LIBM m)
if(DEPZ_LIBM)
  target_link_libraries(depz_sensor_sdk_c PUBLIC ${DEPZ_LIBM})
endif()

# --- Examples (live hardware) ---------------------------------------------
option(DEPZ_SENSOR_SDK_C_BUILD_EXAMPLES "Build the live-hardware examples"
       ${DEPZ_SENSOR_SDK_C_IS_TOP_LEVEL})
if(DEPZ_SENSOR_SDK_C_BUILD_IO AND DEPZ_SENSOR_SDK_C_BUILD_EXAMPLES)
  foreach(ex depz_list sr04_minimal bno086_minimal vl53lx_minimal)
    add_executable(${ex} examples/${ex}.c)
    target_link_libraries(${ex} PRIVATE depz_sensor_sdk_c)
  endforeach()
endif()

# --- Install / export -----------------------------------------------------
include(CMakePackageConfigHelpers)

set(DEPZ_SENSOR_SDK_C_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/depz-sensor-sdk-c"
    CACHE STRING "Install location for depz-sensor-sdk-c CMake package files")

install(TARGETS depz_sensor_sdk_c
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
# Golden vectors live in the development tree's contracts/ directory and are
# not shipped with the packaged sources — skip the tests gracefully without them.
if(DEPZ_SENSOR_SDK_C_BUILD_TESTS)
  set(DEPZ_VECTORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/vectors"
      CACHE PATH "Path to the golden-vector tree")
  if(NOT EXISTS "${DEPZ_VECTORS_DIR}")
    message(STATUS "depz-sensor-sdk-c: golden vectors not found at ${DEPZ_VECTORS_DIR}; tests skipped (set DEPZ_VECTORS_DIR to enable)")
    set(DEPZ_SENSOR_SDK_C_BUILD_TESTS OFF)
  endif()
endif()

if(DEPZ_SENSOR_SDK_C_BUILD_TESTS)
  enable_testing()

  add_executable(test_vectors
    tests/test_vectors.c
    tests/json.c
  )
  target_link_libraries(test_vectors PRIVATE depz_sensor_sdk_c)
  target_include_directories(test_vectors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_compile_definitions(test_vectors PRIVATE
    DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}")

  # One CTest per consumed vector / replay target: the transport+protocol
  # foundation (8) plus the sensor-decode layer (7) across the five sensors
  # (SR04, VL53L8CX, VL53L8CH, VL53L4CD, BNO086). The vl53l8_* targets cover
  # the CX/CH-shared decode plus the CH-only CNH decode: vl53l8_advanced =
  # shared DCI codecs, vl53l8_replay = a CH device capture through the shared
  # frame decoder, vl53l8_cnh = CH compact-network-histogram decode
  # (live-capture golden vector). vl53l4 = the VL53L4CD wire codecs + host-ULD
  # math (contract 10). vl53l7 = the VL53L5CX/L7CX/L7CH I2C-bridge codecs +
  # class resolution (contract 11); vec_vl53l7_replay_<capture> = one live
  # L5/L7 capture each through framing -> reassembler -> L5/L7 frame decoder.
  # vl53lx = the VL53L 1D family on the APP_VL53L0_4 bridge (contract 12):
  # v2.00 codecs, product table + class resolution, die/L0X/histogram blocks.
  # bno055 = the BNO055 register bridge (contract 13): codecs, units /
  # calibration / axis remap / page-1 configs, register-window decode.
  foreach(stem
      crc usb_ids identity common_commands sr04 framing_encode framing_decode fwdepz
      vl53l8_advanced vl53l4 bno086_shtp bno086_reports vl53l8_replay vl53l8_cnh
      dataset vl53l7 vl53lx bno055)
    add_test(NAME vec_${stem} COMMAND test_vectors ${stem})
  endforeach()
  # The live-hardware layer against a fake SR04 over loopback (no device).
  if(DEPZ_SENSOR_SDK_C_BUILD_IO)
    add_executable(test_io tests/test_io.c tests/fake_sr04.c tests/fake_vl53l4.c tests/fake_bno086.c tests/json.c)
    target_link_libraries(test_io PRIVATE depz_sensor_sdk_c)
    target_include_directories(test_io PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_compile_definitions(test_io PRIVATE DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}")
    foreach(t
        identity_roundtrip temperature sync_time_produces_offset
        unknown_cmd_raises_status_error sync_pin_validation timeout_when_device_silent
        wrong_type_is_refused sample_period_readback_is_stored echo_decay_clamps_both_ends
        measure_once_is_source_once loop_samples_are_source_loop start_stop_idempotent
        measure_once_busy_during_loop no_echo_timeout_sentinel temperature_compensated_distance
        stream_drop_oldest_counter two_independent_streams sync_in_single_shot_reaches_stream
        events_and_disconnect same_opcode_in_flight_is_busy record_then_replay
        sr04_session_replay
        l4_init_boot_speed_then_retimes l4_init_at_400_never_retimes l4_init_config_block_and_vhv
        l4_range_timing_roundtrip_and_bounds l4_tuning_roundtrips l4_is_alive_checks_model_id
        l4_config_refused_while_ranging l4_stream_decodes_and_counts_short_blocks
        l4_measure_once_polls_and_stops l4_session_replay
        l8_guards_and_cnh_math l8cx_replay l8ch_replay l8ch_cnh_replay
        l5cx_replay l5cx_4x4_replay l7ch_replay l7ch_cnh_replay
        bno_imu_replay bno_ndof_replay
        bno086_session_replay bno086_features bno086_busy_retry bno086_reports bno086_commands
        bno086_frs bno086_codecs
        vlx_l4cd_uld_replay vlx_l1cx_uld_replay vlx_l1cb_uld_replay vlx_l3cx_ulp_replay
        vlx_l0x_uld_replay vlx_l4cx_hist_medium_replay vlx_l4cx_hist_short_replay
        vlx_l1cx_hist_replay vlx_l1cb_hist_replay vlx_l3cx_hist_replay)
      add_test(NAME io_${t} COMMAND test_io ${t})
    endforeach()
  endif()
  foreach(rec
      vl53l5cx_8x8_15hz_3s vl53l5cx_4x4_15hz vl53l7ch_8x8_15hz_3s vl53l7ch_cnh_8x8_15hz
      vl53l8ch_8x8_15hz_3s vl53l8ch_cnh_8x8_15hz)
    add_test(NAME vec_vl53l7_replay_${rec} COMMAND test_vectors vl53l7_replay ${rec})
  endforeach()
endif()
