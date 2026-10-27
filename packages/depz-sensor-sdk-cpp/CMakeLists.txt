cmake_minimum_required(VERSION 3.16)
project(depz_sensor_sdk_cpp VERSION 0.8.0 LANGUAGES CXX)

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
  src/tmf8829.cpp
  src/tmf8829_firmware.cpp
  src/dataset.cpp
)
target_include_directories(depz_sensor_sdk PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(depz_sensor_sdk PUBLIC cxx_std_17)
# Warnings: the full set whenever we own the build (never forced on consumers
# who embed us); as errors in our own builds — tests on, i.e. development and
# CI — but not for packagers (vcpkg / Conan build with tests off), whose newer
# compiler may know warnings this code has never met.
option(DEPZ_SENSOR_SDK_CPP_WERROR "Treat compiler warnings as errors"
       ${DEPZ_SENSOR_SDK_CPP_BUILD_TESTS})
function(depz_cpp_warnings target)
  if(NOT _depz_is_top_level)
    return()
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 $<$<BOOL:${DEPZ_SENSOR_SDK_CPP_WERROR}>:/WX>)
    # getenv / strcat are standard C++; MSVC's _s variants are not portable (C4996)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    # gcc's maybe-uninitialized stays a warning: at -O2 newer gcc (15 on CI) reports it inside
    # libstdc++ for an std::optional<std::vector> member (Vl53l8Frame::cnh_raw) that is always
    # constructed — a known false positive of the optimiser, nothing this code can initialise.
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic
                           $<$<BOOL:${DEPZ_SENSOR_SDK_CPP_WERROR}>:-Werror>
                           $<$<CXX_COMPILER_ID:GNU>:-Wno-error=maybe-uninitialized>)
  endif()
endfunction()
depz_cpp_warnings(depz_sensor_sdk)

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
  target_sources(depz_sensor_sdk PRIVATE src/device.cpp src/recorder.cpp)
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

# Third-party licences of what the library embeds: the ams TMF8829 sensor
# firmware (MIT) behind depz::tmf8829::firmware_image().
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/licenses/tmf8829/LICENSES-MIT.TXT
  DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/depz-sensor-sdk-cpp/tmf8829)

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

  # Coverage gate (scripts/check_vector_coverage.py): the C SDK's header-only
  # vector_seen.h, compiled once as C (tests/vector_seen.c) so <windows.h>
  # stays out of the C++ test sources. Without the sibling C tree it is a no-op.
  set(_depz_c_tests "${CMAKE_CURRENT_SOURCE_DIR}/../depz-sensor-sdk-c/tests")
  set(_depz_seen_src "")
  if(EXISTS "${_depz_c_tests}/vector_seen.h")
    enable_language(C)
    set(_depz_seen_src tests/vector_seen.c)
  endif()

  add_executable(depz_test_vectors tests/test_vectors.cpp ${_depz_seen_src})
  target_link_libraries(depz_test_vectors PRIVATE depz_sensor_sdk)
  target_include_directories(depz_test_vectors PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_compile_definitions(depz_test_vectors PRIVATE
    DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}"
    DEPZ_RECORDINGS_DIR="${DEPZ_VECTORS_DIR}/recordings")
  if(_depz_seen_src)
    target_include_directories(depz_test_vectors PRIVATE "${_depz_c_tests}")
    target_compile_definitions(depz_test_vectors PRIVATE DEPZ_TEST_VECTOR_SEEN=1)
  endif()
  # The BNO086 SI scaling (bno086_reports records, q_points) is in depz/device.hpp.
  if(DEPZ_SENSOR_SDK_CPP_BUILD_IO)
    target_compile_definitions(depz_test_vectors PRIVATE DEPZ_TEST_HAS_IO=1)
  endif()
  depz_cpp_warnings(depz_test_vectors)

  # The foundation parity suite (crc/framing/usb/common/identity/sr04/fwdepz).
  add_test(NAME vectors COMMAND depz_test_vectors)

  # Contract 16 differential-testing adapter: a JSON-lines process the driver
  # (scripts/difftest.py) builds and starts. Not part of `all`.
  add_executable(depz_difftest_adapter EXCLUDE_FROM_ALL tests/difftest_adapter.cpp)
  target_link_libraries(depz_difftest_adapter PRIVATE depz_sensor_sdk)
  target_include_directories(depz_difftest_adapter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  if(DEPZ_SENSOR_SDK_CPP_BUILD_IO)
    target_compile_definitions(depz_difftest_adapter PRIVATE DEPZ_DIFFTEST_HAS_IO=1)
  endif()
  depz_cpp_warnings(depz_difftest_adapter)

  # The live-hardware wrapper against the C SDK's fakes over loopback.
  if(DEPZ_SENSOR_SDK_CPP_BUILD_IO AND EXISTS "${_depz_c_tests}/fake_sr04.c")
    enable_language(C)
    add_executable(depz_test_device tests/test_device.cpp "${_depz_c_tests}/fake_sr04.c"
                   "${_depz_c_tests}/fake_vl53l4.c" "${_depz_c_tests}/fake_bno086.c"
                   "${_depz_c_tests}/fake_tmf8829.c" "${_depz_c_tests}/fake_vl53l8.c"
                   "${_depz_c_tests}/fake_vl53lx.c" "${_depz_c_tests}/fake_bno055.c")
    target_link_libraries(depz_test_device PRIVATE depz_sensor_sdk)
    depz_cpp_warnings(depz_test_device)
    target_include_directories(depz_test_device PRIVATE "${_depz_c_tests}" tests)
    target_compile_definitions(depz_test_device PRIVATE DEPZ_VECTORS_DIR="${DEPZ_VECTORS_DIR}")
    foreach(t identity_and_types errors_are_typed short_reply_fails_its_request_and_the_reader_goes_on
              timeout_when_silent measure_stream_and_callbacks a_throwing_callback_never_stops_the_reader disconnect_ends_streams host_seq_gap_is_an_event
              promote_and_record_replay replay_refuses_a_gzipped_capture
              l4_fake_roundtrips_and_guards bno086_fake_errors_and_reports
              cnh_required_memory vl53l8ch_cnh_and_motion_exclusive vlx_refusal_before_traffic tmf8829_fake_session
              vlx_set_mode_hands_back_the_budget
              dataset_recorder_two_sr04 dataset_recorder_temperature_gz dataset_recorder_bno086
              device_core_extras dataset_live_records
              vl53l8_fake_init_and_configuration vl53l8_fake_thresholds_calibration_and_ranging
              vl53l8ch_fake_cnh vl53lx_fake_l1cx_uld vl53lx_fake_l4cd_and_l3cx
              bno055_fake_registers_modes_and_config bno055_fake_samples_and_stream
              bno086_fake_commands l4_fake_tuning_stream_and_registers
              dataset_recorder_skips_a_failing_thermometer)
      add_test(NAME device_${t} COMMAND depz_test_device ${t})
    endforeach()
  endif()

  # Contract 15 device scenarios (contracts/scenarios/*.json): one ctest
  # target per scenario, all through the live layer over strict-tx replay.
  set(DEPZ_SCENARIOS_DIR "${DEPZ_VECTORS_DIR}/../scenarios" CACHE PATH "Path to the device-scenario tree")
  if(DEPZ_SENSOR_SDK_CPP_BUILD_IO AND EXISTS "${DEPZ_SCENARIOS_DIR}")
    add_executable(depz_test_scenarios tests/test_scenarios.cpp ${_depz_seen_src})
    target_link_libraries(depz_test_scenarios PRIVATE depz_sensor_sdk)
    target_include_directories(depz_test_scenarios PRIVATE tests)
    target_compile_definitions(depz_test_scenarios PRIVATE
      DEPZ_SCENARIOS_DIR="${DEPZ_SCENARIOS_DIR}"
      DEPZ_RECORDINGS_DIR="${DEPZ_VECTORS_DIR}/recordings")
    if(_depz_seen_src)
      target_include_directories(depz_test_scenarios PRIVATE "${_depz_c_tests}")
      target_compile_definitions(depz_test_scenarios PRIVATE DEPZ_TEST_VECTOR_SEEN=1)
    endif()
    depz_cpp_warnings(depz_test_scenarios)
    file(GLOB _depz_scenarios CONFIGURE_DEPENDS "${DEPZ_SCENARIOS_DIR}/*.json")
    foreach(_sc ${_depz_scenarios})
      get_filename_component(_id "${_sc}" NAME_WE)
      add_test(NAME scenario_${_id} COMMAND depz_test_scenarios ${_id})
    endforeach()
  endif()

  # Sensor-decode layer: each area is its own ctest target (same runner, a suite
  # selector argument picks which checks run).
  add_test(NAME vl53l8_frame    COMMAND depz_test_vectors vl53l8_frame)
  add_test(NAME vl53l8_advanced COMMAND depz_test_vectors vl53l8_advanced)
  add_test(NAME vl53l8_cnh       COMMAND depz_test_vectors vl53l8_cnh)
  add_test(NAME vl53l8_wire     COMMAND depz_test_vectors vl53l8_wire)
  add_test(NAME vl53l7_codecs   COMMAND depz_test_vectors vl53l7_codecs)
  add_test(NAME vl53l7_frame    COMMAND depz_test_vectors vl53l7_frame)
  add_test(NAME vl53l4          COMMAND depz_test_vectors vl53l4)
  add_test(NAME vl53lx          COMMAND depz_test_vectors vl53lx)
  add_test(NAME bno055          COMMAND depz_test_vectors bno055)
  add_test(NAME tmf8829         COMMAND depz_test_vectors tmf8829)
  add_test(NAME tmf8829_frame_time COMMAND depz_test_vectors tmf8829_frame_time)
  add_test(NAME bno086_shtp     COMMAND depz_test_vectors bno086_shtp)
  add_test(NAME bno086_reports  COMMAND depz_test_vectors bno086_reports)
  add_test(NAME dataset         COMMAND depz_test_vectors dataset)
  add_test(NAME dataset_player  COMMAND depz_test_vectors dataset_player)
  add_test(NAME input_rules     COMMAND depz_test_vectors input_rules)
endif()
