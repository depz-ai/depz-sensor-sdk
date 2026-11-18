^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package depz_sensors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.0.0 (2026-11-17)
------------------
* First release of the package: a ROS 2 driver for the DEPZ USB sensor boards
  (HC-SR04, VL53L0X / L1 / L3 / L4, VL53L5CX / L7 / L8, TMF8829, BNO055,
  BNO085 / BNO086) over the DEPZ C++ SDK, for ROS 2 Jazzy and Lyrical.
* One node, ``sensors_node``, for every board on the machine: automatic
  discovery, a topic set per board named after the board's model and serial
  number, aliases, boards chosen by serial number or device path, and
  ``hot_plug`` for boards that are plugged in or come back while the node runs.
* Standard messages: ``sensor_msgs/Range``, ``Image`` (the depth grid),
  ``Imu`` and ``MagneticField``; more topics, off by default: a
  ``PointCloud2`` per multizone board, ``Temperature``, and what the standard
  messages do not carry.
* Header stamps from the board's clock, synchronised with the host's.
* Sensor parameters per model and per board (``boards.<name>.``), read-only
  once the node runs; a value a board cannot take stops that board, a wrong
  type or a value the node cannot run with ends the node, each with the
  reason.
* ``/diagnostics`` once a second per board: the measured rate, the
  temperature, losses, restarts.
* A board that misses a request at start-up is asked again; a board that goes
  silent on USB is restarted, the other boards untouched.
* The node is an rclcpp component (``depz_sensors::SensorsNode``):
  ``sensors_composed.launch.py`` loads it into a component container.
* ``view.launch.py``: the node and RViz 2 with a display per topic, the boards
  placed side by side.
* The ``replay`` parameter: the node on recorded captures instead of boards.
* Examples in Python and C++, ``example_robot.launch.py`` with the mounting
  transforms, a udev rule for the USB permissions, ``scripts/ros.sh`` to build
  and run the package in a container.
* The release notes of the DEPZ sensor SDKs, which this package is released
  with, have every change in full:
  https://github.com/depz-ai/depz-sensor-sdk/releases
