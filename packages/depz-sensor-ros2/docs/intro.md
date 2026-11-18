# ROS 2

`depz_sensors` is the **ROS 2 package** for the DEPZ USB sensor line: one
node that finds every DEPZ board on USB and publishes its measurements as
standard `sensor_msgs` messages. It runs on **ROS 2 Jazzy** (Ubuntu 24.04) and
**ROS 2 Lyrical** (Ubuntu 26.04) and is built on the C++ SDK, which does the
USB and sensor work — you write no C++ to use it.

```bash
ros2 launch depz_sensors sensors.launch.py
```

Every board gets topics of its own, named after its model and USB serial
number, so they stay the same across restarts and replugs:

<!-- doc-check: skip — topic names, not code -->
```text
/depz/hcsr04_uexample01/range      sensor_msgs/msg/Range
/depz/vl53l8ch_texample01/depth    sensor_msgs/msg/Image
/depz/bno086_iexample01/imu        sensor_msgs/msg/Imu
/depz/bno086_iexample01/mag        sensor_msgs/msg/MagneticField
```

## What each board publishes

| Board | Topic | Message | Default rate |
|---|---|---|---|
| HC-SR04 | `range` | `sensor_msgs/msg/Range`, metres | 20 Hz |
| VL53L4CD · VL53L0X · VL53L1CX · VL53L1CB · VL53L3CX · VL53L4CX | `range` | `sensor_msgs/msg/Range`, metres | about 20 Hz |
| VL53L5CX · VL53L7CX · VL53L7CH · VL53L8CX · VL53L8CH | `depth` | `sensor_msgs/msg/Image`, `32FC1`, metres per zone | 15 Hz, 8x8 |
| TMF8829 | `depth` | `sensor_msgs/msg/Image`, `32FC1`, metres per zone | 30 Hz, 8x8 |
| BNO055 · BNO085 · BNO086 | `imu`, `mag` | `sensor_msgs/msg/Imu`, `sensor_msgs/msg/MagneticField` | 100 Hz |

Units follow REP 103 (metres, radians, rad/s, m/s², tesla). A measurement the
sensor could not make is `+inf` (nothing in range) or `NaN` (rejected by the
sensor, or a zone without a target) — REP 117 — never a made-up distance.
Messages are published with the sensor-data QoS (best effort) and stamped
with the moment of the measurement: the board's clock, mapped to the
computer's.

## Quick start

1. Install ROS 2 ([Jazzy](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
   on Ubuntu 24.04) and build the package from the SDK repository with
   `colcon` ([Install](install.md) has every step, ROS 2 Lyrical and Docker):

   <!-- doc-check: skip — git and colcon on the user's machine; this colcon build is run in ROS 2 containers before every release -->
   ```bash
   source /opt/ros/jazzy/setup.bash
   git clone https://github.com/depz-ai/depz-sensor-sdk.git ~/depz-sensor-sdk
   mkdir -p ~/depz_ws && cd ~/depz_ws
   colcon build --base-paths ~/depz-sensor-sdk/packages/depz-sensor-ros2
   source install/setup.bash
   ```

2. Let your user open the boards — install the udev rule the package ships
   (it also keeps ModemManager off the ports), then replug the boards:

   ```bash
   sudo cp $(ros2 pkg prefix --share depz_sensors)/udev/60-depz-sensors.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

3. Start the node and look at the data from a second terminal:

   ```bash
   ros2 launch depz_sensors sensors.launch.py
   ```

   <!-- doc-check: skip — ros2 topic commands on the user's own topic names -->
   ```bash
   ros2 topic list
   ros2 topic echo /depz/hcsr04_uexample01/range
   ```

   Or see every board at once in RViz 2, the 3D viewer of ROS — a cone per
   distance sensor, a box that turns with each IMU, each multizone sensor's
   zone grid — with one command that starts the node and RViz together:

   ```bash
   ros2 launch depz_sensors view.launch.py
   ```

No ROS on the machine? The package's `scripts/ros.sh jazzy run` does the same
in a Docker container ([Install](install.md#without-ros-on-your-machine-docker)).

## Names of your own

Give boards aliases by serial number in a parameter file, and pass it with
`config:=`:

```yaml
/**:
  ros__parameters:
    aliases:
      - "UEXAMPLE01=front"
      - "IEXAMPLE01=imu"
    hot_plug: true
```

```bash
ros2 launch depz_sensors sensors.launch.py config:=$HOME/my_sensors.yaml
```

The topics are then `/depz/front/range` and `/depz/imu/imu`, the frames
`front` and `imu`; `hot_plug` picks up boards plugged in later.

## From Python and C++

The package installs example programs to start from — distance, every
distance sensor in one table, depth grid, the closest zone (in Python and the
same in C++), IMU angles:

```bash
ros2 run depz_sensors range_listener.py --ros-args -p topic:=/depz/front/range
ros2 run depz_sensors range_table.py
ros2 run depz_sensors depth_listener.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
ros2 run depz_sensors depth_min_distance.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
ros2 run depz_sensors depth_min_distance --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
ros2 run depz_sensors imu_listener.py --ros-args -p topic:=/depz/imu/imu
```

`example_robot.launch.py` starts the node and examples together and says
where each board is mounted on the robot (a static transform from `base_link`
to the board's frame) — the shape of a launch file for your robot. [Examples](examples.md) has each program and
what it prints.

Subscribe with `qos_profile_sensor_data` (C++: `rclcpp::SensorDataQoS()`): a
subscriber that asks for reliable delivery gets nothing from a best-effort
publisher.

## Where to next

- **[Install](install.md)** — step by step: ROS 2 Jazzy or Lyrical, the build
  with `colcon`, Docker instead of ROS on the machine, USB permissions.
- **[Guide](guide.md)** — running the node, viewing depth in rqt_image_view
  and every board in RViz 2 (`view.launch.py`), what each message carries,
  aliases, every parameter, coordinate frames and static transforms, time
  stamps, troubleshooting by the line the node prints, limitations.
- **[Examples](examples.md)** — the programs above one by one, with what each
  prints, and a launch file that starts them with the node.
- **[C++ SDK](https://depz.ai/developers/sensors/docs/cpp/guide)** — the
  library under the node, for programs that talk to a board directly.
- **Source & license** — open source (MIT):
  [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk);
  [the package's README](https://github.com/depz-ai/depz-sensor-sdk/blob/main/packages/depz-sensor-ros2/README.md)
  is these pages in one.
