<!-- generated from ../README.md — edit that page -->
# ROS 2 — examples

Programs that read the node's topics, in Python and C++, and a launch file that
starts them with the node. They need the package built and the node running:
[Install](install.md), [Guide](guide.md#run).

## Examples

Small programs that subscribe to the node's topics — start from them for your
own. They are installed with the package (sources in [`examples/`](https://github.com/depz-ai/depz-sensor-sdk/tree/main/packages/depz-sensor-ros2/examples)),
and each takes the topic to listen to as its `topic` parameter. All but one are
written with `rclpy`, the ROS 2 Python library.

Distance from a single-zone sensor ([`range_listener.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/range_listener.py))
— prints `83.1 cm`, `nothing in range` or `no valid measurement`:

```bash
ros2 run depz_sensors range_listener.py --ros-args -p topic:=/depz/hcsr04_uexample01/range
```

Every distance sensor at once ([`range_table.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/range_table.py)) —
no parameter needed: once a second it asks ROS which topics carry
`sensor_msgs/Range`, whatever their names, subscribes to the new ones, drops
those of unplugged boards and prints a table:

```bash
ros2 run depz_sensors range_table.py
```

<!-- doc-check: skip — the program's output -->
```text
topic                                        distance  rate
/depz/vl53l1cb_te4xkbkbea/range               95.7 cm  20 /s
/depz/vl53l3cx_tjuyj4nxsv/range  no valid measurement  20 /s
/depz/vl53l4cx_tovjaln523/range              101.2 cm  20 /s
```

The zone grid of a multizone sensor ([`depth_listener.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/depth_listener.py))
— prints the grid in centimetres, `--` for a zone without a target:

```bash
ros2 run depz_sensors depth_listener.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

The closest target a multizone sensor sees, and where in the image
([`depth_min_distance.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/depth_min_distance.py)) — prints
`closest 0.412 m at row 3, column 3` — row 0 is the top of the field,
column 0 its left:

```bash
ros2 run depz_sensors depth_min_distance.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

**The same in C++** ([`depth_min_distance.cpp`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/depth_min_distance.cpp)):
it prints exactly the same lines, which shows that a C++ program reads the same
topics with nothing of DEPZ — only `rclcpp` and `sensor_msgs`. Its name has no
`.py`:

```bash
ros2 run depz_sensors depth_min_distance --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

It is built with the package; `--cmake-args -DDEPZ_SENSORS_BUILD_EXAMPLES=OFF`
on `colcon build` leaves it out.

Roll, pitch and yaw from an IMU ([`imu_listener.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/examples/imu_listener.py))
— prints the angles in degrees and the acceleration, five times a second:

```bash
ros2 run depz_sensors imu_listener.py --ros-args -p topic:=/depz/bno086_iexample01/imu
```

What they show: subscribing with `qos_profile_sensor_data` in Python and
`rclcpp::SensorDataQoS()` in C++ (the QoS the node publishes with), telling
`+inf` and `NaN` from a distance, reading the floats out of a `32FC1` image,
turning a quaternion into angles, finding topics while the program runs.

### Your own launch file

[`launch/example_robot.launch.py`](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/launch/example_robot.launch.py) starts the
sensors node and the programs that use it with one command — the shape of a
launch file for your robot. It runs the node with
[the aliases example](guide.md#several-boards-and-aliases) (boards named `front`,
`depth` and `imu`), `depth_min_distance.py` on `depth`, `imu_listener.py` on
`imu` and `range_table.py` — and says where each board is mounted on the
robot, with a [static transform](guide.md#coordinate-frames) from `base_link` to the
board's frame:

```bash
ros2 launch depz_sensors example_robot.launch.py config:=$HOME/my_sensors.yaml namespace:=robot1
```

Inside it the topics are written without a leading `/` (`depz/depth/depth`),
so that `namespace:=robot1` moves the node's topics and the programs'
subscriptions together. Copy the file into your own package, put your
programs in place of the examples and your boards' real places into the
`mounted(...)` lines.
