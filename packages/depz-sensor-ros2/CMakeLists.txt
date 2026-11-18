cmake_minimum_required(VERSION 3.16)
project(depz_sensors VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# ---- the DEPZ C++ SDK ------------------------------------------------------
# An installed SDK (find_package), else the SDK sources next to this package
# in the repository (DEPZ_SDK_CPP_DIR to point elsewhere). Built static into
# the node's library: nothing to install besides this package. That library is
# shared (a component), so what goes into it is position-independent code — an
# installed SDK must be built so too.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(DEPZ_SDK_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../depz-sensor-sdk-cpp"
    CACHE PATH "depz-sensor-sdk-cpp sources, used when no installed SDK is found")
find_package(depz-sensor-sdk-cpp CONFIG QUIET)
if(NOT TARGET depz::sensor_sdk)
  if(NOT EXISTS "${DEPZ_SDK_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "depz_sensors needs the DEPZ C++ SDK: install depz-sensor-sdk-cpp, "
                        "or set DEPZ_SDK_CPP_DIR to its sources")
  endif()
  set(BUILD_SHARED_LIBS OFF)
  set(DEPZ_SENSOR_SDK_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${DEPZ_SDK_CPP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/depz-sensor-sdk-cpp" EXCLUDE_FROM_ALL)
endif()

# Warnings are errors in this package's own targets (not in the SDK's) when asked:
# scripts/ros.sh asks; a user's compiler may warn about more.
option(DEPZ_SENSORS_WERROR "Treat warnings in depz_sensors' own code as errors" OFF)
set(_depz_warn -Wall -Wextra -Wpedantic)
if(DEPZ_SENSORS_WERROR)
  list(APPEND _depz_warn -Werror)
endif()

# ---- conversion + naming: no node, unit-tested -----------------------------
add_library(depz_sensors_core STATIC
  src/access.cpp
  src/convert.cpp
  src/diagnostics.cpp
  src/naming.cpp
  src/settings.cpp
  src/stamps.cpp
  src/trail.cpp
  src/watchdog.cpp
)
target_include_directories(depz_sensors_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(depz_sensors_core PUBLIC
  depz::sensor_sdk
  ${diagnostic_msgs_TARGETS}
  ${geometry_msgs_TARGETS}
  ${sensor_msgs_TARGETS}
  ${std_msgs_TARGETS})
target_compile_options(depz_sensors_core PRIVATE ${_depz_warn})

# ---- the node --------------------------------------------------------------
# A component: the shared library a component container loads the node from
# (plugin depz_sensors::SensorsNode, sensors_composed.launch.py) ...
add_library(depz_sensors_component SHARED
  src/board.cpp
  src/sensors_node.cpp
)
target_link_libraries(depz_sensors_component PRIVATE depz_sensors_core rclcpp_components::component)
target_link_libraries(depz_sensors_component PUBLIC rclcpp::rclcpp)
target_link_options(depz_sensors_component PRIVATE -Wl,--no-undefined)  # a missing symbol fails here, not at load
target_compile_options(depz_sensors_component PRIVATE ${_depz_warn})
rclcpp_components_register_nodes(depz_sensors_component "depz_sensors::SensorsNode")

# ... and the `sensors_node` executable: the same node in a process of its own
# (src/main.cpp says why it is not the main rclcpp_components generates).
add_executable(sensors_node src/main.cpp)
target_link_libraries(sensors_node depz_sensors_component)
target_compile_options(sensors_node PRIVATE ${_depz_warn})

install(TARGETS depz_sensors_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS sensors_node DESTINATION lib/${PROJECT_NAME})
# config/: the default parameters and an example with aliases; rviz/: an RViz 2
# view of them; udev/: the USB permission rule (the README says how to use each).
install(DIRECTORY launch config rviz udev DESTINATION share/${PROJECT_NAME})
# The licences of what the node is built from: the package's (MIT) and ST's (BSD-3-Clause) for the
# VL53L5 / L7 / L8 firmware and driver code of the SDK compiled into it.
install(FILES LICENSE DESTINATION share/${PROJECT_NAME})
install(DIRECTORY licenses DESTINATION share/${PROJECT_NAME})
# view.launch.py's program: RViz 2 with a display per topic of the node (it reads rviz/).
install(PROGRAMS scripts/rviz_view.py DESTINATION lib/${PROJECT_NAME})
install(PROGRAMS
  examples/range_listener.py
  examples/depth_listener.py
  examples/imu_listener.py
  examples/depth_min_distance.py
  examples/range_table.py
  DESTINATION lib/${PROJECT_NAME})

# The C++ example: a subscriber of the node's topics, as a C++ program of yours
# would be — rclcpp and sensor_msgs only, nothing of the node or the SDK.
option(DEPZ_SENSORS_BUILD_EXAMPLES "Build the C++ example (examples/depth_min_distance.cpp)" ON)
if(DEPZ_SENSORS_BUILD_EXAMPLES)
  add_executable(depth_min_distance examples/depth_min_distance.cpp)
  target_link_libraries(depth_min_distance rclcpp::rclcpp ${sensor_msgs_TARGETS})
  target_compile_options(depth_min_distance PRIVATE ${_depz_warn})
  install(TARGETS depth_min_distance DESTINATION lib/${PROJECT_NAME})
endif()

if(BUILD_TESTING)
  # The standard ROS lint tests (ament_lint_common: copyright, cppcheck, cpplint,
  # flake8, lint_cmake, pep257, uncrustify, xmllint) on what the public
  # repository carries: src/, include/, examples/, launch/, this file and
  # package.xml. They are the only tests there — the public tree has no test/
  # directory. The full suite (unit tests, the node on captures) is kept in the
  # development repository, in test/: it is built when its sources are here,
  # and it is not held to the ROS style, so the linters leave it out.
  find_package(ament_lint_auto REQUIRED)
  set(_depz_suite "${CMAKE_CURRENT_SOURCE_DIR}/test")
  if(EXISTS "${_depz_suite}")
    # CONFIGURE_DEPENDS: a file added to test/ is left out by the next build
    # too, not only by the next configure.
    file(GLOB_RECURSE AMENT_LINT_AUTO_FILE_EXCLUDE CONFIGURE_DEPENDS "${_depz_suite}/*")
    # pep257's hook is the one that does not read AMENT_LINT_AUTO_FILE_EXCLUDE
    # (Jazzy, Lyrical): the same test, called here with the list.
    file(GLOB_RECURSE _depz_suite_py CONFIGURE_DEPENDS "${_depz_suite}/*.py")
    list(APPEND AMENT_LINT_AUTO_EXCLUDE ament_cmake_pep257)
    find_package(ament_cmake_pep257 REQUIRED)
    ament_pep257(--exclude ${_depz_suite_py})
  endif()
  ament_lint_auto_find_test_dependencies()
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/test_convert.cpp")
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_access test/test_access.cpp)
  target_link_libraries(test_access depz_sensors_core)
  target_compile_options(test_access PRIVATE ${_depz_warn})
  ament_add_gtest(test_convert test/test_convert.cpp)
  target_link_libraries(test_convert depz_sensors_core)
  target_compile_options(test_convert PRIVATE ${_depz_warn})
  ament_add_gtest(test_naming test/test_naming.cpp)
  target_link_libraries(test_naming depz_sensors_core)
  target_compile_options(test_naming PRIVATE ${_depz_warn})
  ament_add_gtest(test_diagnostics test/test_diagnostics.cpp)
  target_link_libraries(test_diagnostics depz_sensors_core)
  target_compile_options(test_diagnostics PRIVATE ${_depz_warn})
  ament_add_gtest(test_settings test/test_settings.cpp)
  target_link_libraries(test_settings depz_sensors_core)
  target_compile_options(test_settings PRIVATE ${_depz_warn})
  ament_add_gtest(test_stamps test/test_stamps.cpp)
  target_link_libraries(test_stamps depz_sensors_core)
  target_compile_options(test_stamps PRIVATE ${_depz_warn})
  ament_add_gtest(test_watchdog test/test_watchdog.cpp)
  target_link_libraries(test_watchdog depz_sensors_core)
  target_compile_options(test_watchdog PRIVATE ${_depz_warn})
  ament_add_gtest(test_trail test/test_trail.cpp)
  target_link_libraries(test_trail depz_sensors_core)
  target_compile_options(test_trail PRIVATE ${_depz_warn})

  # The node made in this process beside a subscriber, intra-process
  # communication on: one capture's messages arrive without the middleware.
  ament_add_gtest(test_component test/test_component.cpp TIMEOUT 120)
  target_link_libraries(test_component depz_sensors_component ${sensor_msgs_TARGETS})
  target_include_directories(test_component PRIVATE src)
  target_compile_definitions(test_component PRIVATE
    DEPZ_SENSORS_TEST_REPLAYS="${CMAKE_CURRENT_SOURCE_DIR}/test/replays")
  target_compile_options(test_component PRIVATE ${_depz_warn})

  # The node itself, on captures instead of boards (the `replay` parameter;
  # start_retry, silent_restart, unplug: on pseudo-terminals, test/fake_board.py):
  # test/replays/, written by scripts/gen_ros2_replays.py. One at a time: each
  # launches its own nodes. launch_files starts it as a user does: the
  # installed launch file with the installed parameter files.
  # The captures are cut from the golden vectors of the development tree
  # (contracts/) and live only there: without them these tests are skipped, as
  # the SDK's vector tests are — but never in the development tree itself.
  set(_depz_replays "${CMAKE_CURRENT_SOURCE_DIR}/test/replays")
  if(EXISTS "${_depz_replays}")
    find_package(launch_testing_ament_cmake REQUIRED)
    foreach(_test replay_streams replay_naming bad_parameters board_parameters shutdown diagnostics extra_topics
        composed start_retry silent_restart unplug parameters launch_files view)
      add_launch_test(test/test_${_test}.launch.py TIMEOUT 180)
    endforeach()
    if(DEPZ_SENSORS_BUILD_EXAMPLES)  # every example on the captures, the C++ one among them
      add_launch_test(test/test_examples.launch.py TIMEOUT 180)
      add_launch_test(test/test_example_robot.launch.py TIMEOUT 180)
      add_launch_test(test/test_examples_ctrl_c.launch.py TIMEOUT 120)
    endif()
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/vectors")
    message(FATAL_ERROR "depz_sensors: no captures at ${_depz_replays} — write them with scripts/gen_ros2_replays.py")
  else()
    message(STATUS "depz_sensors: no captures at ${_depz_replays}; the launch tests on captures are skipped")
  endif()
endif()

ament_package()
