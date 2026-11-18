# depz_sensors — DEPZ USB sensors in ROS 2

`depz_sensors` is a ROS 2 package with one program (a *node*) that finds every
DEPZ sensor board plugged into USB and publishes its measurements as standard
ROS 2 messages. You do not write any C++ — or any code at all — to get data:

```bash
ros2 launch depz_sensors sensors.launch.py
```

Each board gets its own *topics* (named streams of messages) that any ROS 2
program, in Python or C++, can subscribe to:

<!-- doc-check: skip — topic names, not code -->
```text
/depz/hcsr04_uexample01/range      distance, metres
/depz/vl53l8ch_texample01/depth    8x8 grid of distances, metres
/depz/bno086_iexample01/imu        orientation, angular velocity, acceleration
/depz/bno086_iexample01/mag        magnetic field
```

The names come from the board's model and its USB serial number, so a board
keeps its topics when you restart, replug it or add other boards.

It works on **ROS 2 Jazzy** (Ubuntu 24.04) and **ROS 2 Lyrical** (Ubuntu
26.04), is built on the [DEPZ C++ SDK](../depz-sensor-sdk-cpp/README.md) —
which does all the USB and sensor work — and uses only standard `sensor_msgs`
messages.

Contents: [What each board publishes](#what-each-board-publishes) ·
[Install](#install) · [USB permissions](#usb-permissions) ·
[Run](#run) · [Look at the data](#look-at-the-data) ·
[Examples](#examples) ·
[Several boards and aliases](#several-boards-and-aliases) ·
[Parameters](#parameters) · [Coordinate frames](#coordinate-frames) ·
[Time stamps](#time-stamps) · [Troubleshooting](#troubleshooting) ·
[Limitations](#limitations)

## What each board publishes

A topic is `/<topic_prefix>/<board name>/<leaf>`: `topic_prefix` is `depz`
unless you change it, the board name is `<model>_<USB serial>` in lower case
(or [an alias you give it](#several-boards-and-aliases)), and the leaf says
what the data is.

| Board | Leaf | Message type | Default rate | Rate / mode parameters |
|---|---|---|---|---|
| HC-SR04 (ultrasonic) | `range` | `sensor_msgs/msg/Range` | 20 Hz | `sr04_period_ms` |
| VL53L4CD (laser, one zone) | `range` | `sensor_msgs/msg/Range` | about 20 Hz | `range_budget_ms` |
| VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX (laser, one zone) | `range` | `sensor_msgs/msg/Range` | about 20 Hz | `range_budget_ms`, `range_mode` |
| VL53L5CX, VL53L7CX, VL53L7CH, VL53L8CX, VL53L8CH (laser, 4x4 or 8x8 zones) | `depth` | `sensor_msgs/msg/Image` | 15 Hz, 8x8 | `tof_resolution`, `tof_frequency_hz`, `tof_ranging_mode` and more |
| TMF8829 (laser, 8x8 to 48x32 zones) | `depth` | `sensor_msgs/msg/Image` | 30 Hz, 8x8 | `tmf8829_preset`, `tmf8829_period_ms` |
| BNO055 (IMU) | `imu`, `mag` | `sensor_msgs/msg/Imu`, `sensor_msgs/msg/MagneticField` | 100 Hz | `imu_rate_hz`, `bno055_mode` |
| BNO085, BNO086 (IMU) | `imu`, `mag` | `sensor_msgs/msg/Imu`, `sensor_msgs/msg/MagneticField` | 100 Hz | `imu_rate_hz`, `bno086_orientation`, `bno086_mag_rate_hz` |

More topics — temperatures, the signal and status of each zone, a TMF8829's
further peaks, the zones as a point cloud, an IMU's gravity — are there when
you switch them on: see
[Extra topics](#extra-topics). Every board's health is on `/diagnostics`
([Board health](#board-health-diagnostics)).

Units are the ROS standard ones ([REP 103](https://www.ros.org/reps/rep-0103.html)):
metres, radians, rad/s, m/s², tesla. A measurement the sensor could not make
is never a made-up number ([REP 117](https://www.ros.org/reps/rep-0117.html)):

**`range` — `sensor_msgs/msg/Range`**

- `range`: the distance in metres.
  - `+inf` — nothing in range: the HC-SR04 heard no echo, or the sensor gave
    a distance beyond `max_range` (an HC-SR04 gives echoes of up to 10 m).
  - `NaN` — the sensor rejected the measurement (a laser sensor's status other
    than "valid": too little light back, a wrapped-around target, …), or its
    distance came out below zero. Laser sensors side by side that look at one
    surface take each other's light, and reject more: a VL53L3CX 1 m from a
    wall rejected nothing alone and a quarter to a third of its measurements
    beside four or five other laser sensors.
- `radiation_type`: `ULTRASOUND` (0) for the HC-SR04, `INFRARED` (1) for the
  laser sensors.
- `field_of_view` (radians, the full cone angle), `min_range`, `max_range`
  (metres): the sensor's datasheet figures —

  | Model | Cone | `min_range` | `max_range` |
  |---|---|---|---|
  | HC-SR04 | 15° | 0.02 m | 4.0 m |
  | VL53L4CD | 18° | 0.001 m | 1.3 m |
  | VL53L0X | 25° | 0.03 m | 2.0 m |
  | VL53L1CX, VL53L1CB | 27° | 0.04 m | 4.0 m |
  | VL53L3CX | 25° | 0.025 m | 3.0 m |
  | VL53L4CX | 18° | 0.001 m | 6.0 m |

  (for the VL53L0X … VL53L4CX family `max_range` is the reach of the ranging
  mode the board runs, when its driver reports one).

**`depth` — `sensor_msgs/msg/Image`**

- One pixel per zone: `width` x `height` is the zone grid (8x8 by default),
  `encoding` is `32FC1` — each pixel is one 32-bit float, the distance in
  metres to the closest target of that zone.
- `NaN` — the zone has no valid target. The first image after the start of a
  VL53L5CX / L7 / L8 has no valid zone at all: it is the sensor's first
  measurement, which the sensor itself marks as not yet trusted.
- The image is the field as seen from behind the sensor, the board held
  with its USB connector down: row 0 is the top of the field, column 0 its
  left — the way a camera image is laid out. This is not the order of the
  sensor's own zone numbers (the chip sits turned on the board); see
  [which way a zone looks](#which-way-a-zone-looks).

**`imu` — `sensor_msgs/msg/Imu`**

- `orientation` (a quaternion): the chip's own fused orientation.
- `angular_velocity` (rad/s) and `linear_acceleration` (m/s², gravity
  included).
- Covariances: all zeros mean "unknown". The BNO085 / BNO086 report how
  accurate their heading is; that estimate (radians, squared) is on the
  diagonal of `orientation_covariance`. A part the message does not carry yet
  (the first messages of a BNO085 / BNO086, before its gyroscope and
  accelerometer have reported) has `-1` in the first element of its
  covariance, as the message definition prescribes.

**`mag` — `sensor_msgs/msg/MagneticField`**

- `magnetic_field` in tesla (the Earth's field is about 0.00005 T).

## Install

The package is built from source; there is no `apt` package yet. You need
Linux with ROS 2 — or just Docker, [see below](#without-ros-on-your-machine-docker).

### 1. ROS 2

- **Ubuntu 24.04 → ROS 2 Jazzy.** Follow the official
  [Jazzy installation page](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
  (it adds the ROS package source to `apt`), then install ROS and the build
  tools:

  <!-- doc-check: skip — apt packages of ROS itself, not a package of this repository -->
  ```bash
  sudo apt install ros-jazzy-desktop ros-dev-tools
  ```

  `ros-jazzy-desktop` includes the viewing tools this guide uses (RViz 2, rqt).
  On a robot without a screen `ros-jazzy-ros-base` is enough.
- **Ubuntu 26.04 → ROS 2 Lyrical.** The same with the
  [Lyrical installation page](https://docs.ros.org/en/lyrical/Installation.html)
  and `ros-lyrical-desktop`; in every command below write `lyrical` where it
  says `jazzy`.

Every new terminal needs ROS switched on before any `ros2` or `colcon`
command (put the line into `~/.bashrc` to have it always):

<!-- doc-check: skip — sets up the shell, nothing of this repository to check -->
```bash
source /opt/ros/jazzy/setup.bash
```

### 2. Get the sources

<!-- doc-check: skip — git clone, not an install command -->
```bash
git clone https://github.com/depz-ai/depz-sensor-sdk.git ~/depz-sensor-sdk
```

The repository holds the DEPZ SDKs for seven languages; the ROS 2 package is
its folder `packages/depz-sensor-ros2`, and it compiles the C++ SDK next to it
into the node — there is nothing else to install.

### 3. Build with colcon

`colcon` is the ROS 2 build tool. It builds into a *workspace*: a folder of
your choice (here `~/depz_ws`) that receives `build/`, `install/` and `log/`.

<!-- doc-check: skip — rosdep and colcon on the user's machine; this colcon build is run in ROS 2 containers before every release -->
```bash
mkdir -p ~/depz_ws && cd ~/depz_ws
# once per machine: let rosdep install what the package depends on
sudo rosdep init
rosdep update
rosdep install --from-paths ~/depz-sensor-sdk/packages/depz-sensor-ros2 --ignore-src -y
# build (a few minutes the first time)
colcon build --base-paths ~/depz-sensor-sdk/packages/depz-sensor-ros2
```

`sudo rosdep init` says "already exists" if it was run before — that is fine.
`--base-paths` points colcon at the ROS 2 package alone; without it colcon
would also try to build the other SDKs of the repository. On a small computer
(a Raspberry Pi) put `MAKEFLAGS=-j2` in front of `colcon build`, so the
compiler does not run out of memory.

### 4. Switch the workspace on

In every terminal that uses the package, after ROS itself:

<!-- doc-check: skip — sets up the shell, nothing of this repository to check -->
```bash
source ~/depz_ws/install/setup.bash
```

To update later: `git pull` in `~/depz-sensor-sdk`, then the `colcon build`
line again.

### Without ROS on your machine (Docker)

`scripts/ros.sh` builds and runs the package inside a ROS 2 container; the
machine needs only Docker (and your user in the `docker` group). From
`~/depz-sensor-sdk/packages/depz-sensor-ros2`:

<!-- doc-check: skip — the package's own container script; its `test` is run before every release -->
```bash
scripts/ros.sh jazzy build                 # build the package in a Jazzy container
scripts/ros.sh jazzy run                   # ros2 launch depz_sensors sensors.launch.py, with your USB boards
scripts/ros.sh jazzy run config:=/src/packages/depz-sensor-ros2/config/example_aliases.yaml
scripts/ros.sh jazzy exec 'ros2 topic list'   # any command, in a second terminal
scripts/ros.sh jazzy shell                 # a shell inside, for several commands
scripts/ros.sh jazzy test                  # the package's tests
```

`test` runs the tests the package carries here: the standard ROS lint set
(`ament_lint_auto` with `ament_lint_common`). The full suite — unit tests and
the node on recorded captures — is kept in DEPZ's development repository and
runs there before every release.

`lyrical` in place of `jazzy` uses a Lyrical container. Inside the container
the repository is `/src` (read-only) and the workspace is `/ws`, kept on your
machine in `~/.cache/depz-ros/<distro>`. The container sees the machine's
`/dev`: a board plugged in while it runs is there too (the node takes it with
`hot_plug: true`), and [the USB permissions](#usb-permissions) of your
machine apply.

## USB permissions

A board is a USB serial port (`/dev/ttyACM0`, `/dev/ttyACM1`, …), and Linux
does not let every program open one. Do **one** of the two — the first is
recommended.

**A. Install the udev rule shipped with the package.** A udev rule tells Linux
what to do with a device when it is plugged in. This one gives DEPZ boards to
the user sitting at the machine and to the group `dialout`, and keeps
ModemManager — a system service that probes every new serial port as if it
were a modem — away from them (its probing makes the first requests to a board
time out).

```bash
sudo cp $(ros2 pkg prefix --share depz_sensors)/udev/60-depz-sensors.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

(In the sources the file is `packages/depz-sensor-ros2/udev/60-depz-sensors.rules`.)
Then unplug the boards and plug them back.

**B. Join the `dialout` group**, which may open every serial port:

<!-- doc-check: skip — a system command, nothing of this repository to check -->
```bash
sudo usermod -aG dialout $USER
```

This takes effect at the next login: **log out and log in again** (logging
out closes your open windows — save your work first). `groups` must then list
`dialout`.

A robot's service account, or a user logged in over ssh, is not "the user
sitting at the machine": there use B, or A and B together.

## Run

```bash
ros2 launch depz_sensors sensors.launch.py
```

The node prints one line per board it opened — its name, model, port, USB
serial number, topics and how they are delivered — and how many it started:

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[INFO] hcsr04_uexample01: hcsr04 on /dev/ttyACM3 (USB serial UEXAMPLE01) -> depz/hcsr04_uexample01/range (best effort, depth 5)
[INFO] bno086_iexample01: bno086 on /dev/ttyACM0 (USB serial IEXAMPLE01) -> depz/bno086_iexample01/imu depz/bno086_iexample01/mag (best effort, depth 5)
[INFO] started 2 of 2 boards found
```

(On the screen each line has more in front — the program, the time and the
node's name: `[sensors_node-1] [INFO] [1794554921.670947040] [depz_sensors]: …`.
This page quotes the node's lines from the level on.)

Stop it with Ctrl+C: it stops each sensor and releases the ports.

The launch file takes two arguments:

```bash
ros2 launch depz_sensors sensors.launch.py config:=/path/to/my.yaml   # your parameters
ros2 launch depz_sensors sensors.launch.py namespace:=robot1          # topics become /robot1/depz/...
```

`config` is a YAML file of [parameters](#parameters); the default is the
package's `config/sensors.yaml`, which lists every parameter with its default
value — copy it and change what you need:

```bash
cp $(ros2 pkg prefix --share depz_sensors)/config/sensors.yaml ~/my_sensors.yaml
```

Without a launch file, the node runs directly, parameters on the command
line:

```bash
ros2 run depz_sensors sensors_node --ros-args -p hot_plug:=true -p imu_rate_hz:=50.0
```

The node is also an rclcpp *component*: a node that can run inside a shared
process (a component container) beside other nodes, so their messages pass
inside that process instead of over the network layer:

```bash
ros2 launch depz_sensors sensors_composed.launch.py   # takes config:= and namespace:= too
ros2 launch depz_sensors sensors_composed.launch.py container:=/my_container start_container:=false
```

The first starts a container named `depz_container` and loads the node into
it; the second loads it into a container of yours that already runs. A
subscriber loaded into the same container, with intra-process communication
on, gets each message without a copy over the middleware; every other
subscriber gets it as before. Parameters, topics and messages are the same
as with `sensors.launch.py`. The container answers the load once the boards
stream, and refuses it with the node's message on a parameter the node
cannot run with. With `hot_plug` the node looks for new boards on the
container's thread: if other nodes there must not wait for it, use
`component_container_mt` or `component_container_isolated`.

To start it from a launch file of your own project, include the package's:

<!-- doc-check: skip — a ROS launch file: launch and launch_ros are not on the host that runs this check -->
```python
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    sensors = os.path.join(get_package_share_directory("depz_sensors"), "launch", "sensors.launch.py")
    return LaunchDescription([
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(sensors),
            launch_arguments={"config": "/path/to/my.yaml", "namespace": "robot1"}.items(),
        ),
    ])
```

## Look at the data

In a second terminal (ROS and the workspace switched on there too), while the
node runs:

<!-- doc-check: skip — ros2 topic commands on the user's own topic names, nothing of this repository to check -->
```bash
ros2 topic list                                   # every topic; yours start with /depz/
ros2 topic echo /depz/hcsr04_uexample01/range     # print the messages (Ctrl+C to stop)
ros2 topic echo --once /depz/hcsr04_uexample01/range    # just one
ros2 topic hz /depz/bno086_iexample01/imu         # how many messages per second
ros2 topic info -v /depz/bno086_iexample01/imu    # message type, who publishes, who listens
ros2 topic echo --no-arr /depz/vl53l8ch_texample01/depth   # an image without its 256 data bytes
```

Use your own topic names from `ros2 topic list`. One `range` message looks
like this:

<!-- doc-check: skip — the output of ros2 topic echo -->
```text
header:
  stamp:
    sec: 1794342975
    nanosec: 689433911
  frame_id: hcsr04_uexample01
radiation_type: 0
field_of_view: 0.2617993950843811
min_range: 0.019999999552965164
max_range: 4.0
range: 0.8130000233650208
variance: 0.0
```

### The depth grid: rqt_image_view

<!-- doc-check: skip — a ROS tool of another package on the user's own topic name -->
```bash
ros2 run rqt_image_view rqt_image_view /depz/vl53l8ch_texample01/depth
```

The picture is as small as the zone grid (8x8 pixels) — enlarge the window.
Distances are drawn from black (0 m) to white (the value in the box that says
`10.00m`): lower that value to the depth of your scene, 1 to 4 m, to see
contrast. Zones without a target are black. (`sudo apt install
ros-jazzy-rqt-image-view` if the command is not found.)

### Everything at once: RViz 2

RViz 2 is the 3D viewer of ROS. One command starts the sensors node and RViz
with every board in it — a cone for each distance sensor, a box that turns
with each IMU, the zone grid of each multizone sensor as an image (and its
[point cloud](#point-cloud) when `point_cloud` is on):

```bash
ros2 launch depz_sensors view.launch.py
```

Use it **instead of** `sensors.launch.py`, not beside it (a board has one
owner). It takes the same `config:=` and `namespace:=`. Closing the RViz
window, or Ctrl+C, stops the node too; and when the node ends — it cannot run
with a parameter of your file and says so in a `[FATAL]` line — the whole
launch ends, RViz with it.

What it does, so that nothing in it is magic:

- **Topics.** Your topics are named after your boards' serial numbers, so no
  ready-made RViz file can list them. The launch file waits until the node has
  started its boards, asks ROS for the node's topics and writes a view with one
  display per topic (the node's own topics only: `range`, `imu`, `depth` and
  the other images, `points`). A board plugged in later is not in the view:
  start it again.
- **Frames.** RViz draws a cone or a box only where it knows the board to be
  relative to its *Fixed Frame* (`base_link`). The launch file tells it: it
  places the boards side by side, 30 cm apart, each looking along x — a
  [static transform](#coordinate-frames) per board. That is a bench, not your
  robot. When your robot publishes these transforms itself
  ([the example launch file](#your-own-launch-file) shows how), switch the
  placing off, and name your fixed frame if it is not `base_link`:

  ```bash
  ros2 launch depz_sensors view.launch.py place_boards:=false fixed_frame:=base_link
  ```

- **What you need.** RViz 2 itself (`rviz2`, part of `ros-jazzy-desktop`; on
  its own: `sudo apt install ros-jazzy-rviz2`), and for the IMUs the Imu
  display, a plug-in of its own: `sudo apt install ros-jazzy-rviz-imu-plugin`.
  `rviz:=/path/to/rviz2` names another RViz program.
- **Not shown:** `mag`, the temperatures and the vectors of the
  [extra topics](#extra-topics) — look at those with `ros2 topic echo`.

The displays are copies of those in the package's
`rviz/depz_sensors.rviz` — the cone's colour, the box's size, the cloud's
points, each subscribing best effort as the node publishes. That file is also
a view of its own, of [the aliases example](#several-boards-and-aliases)
(`/depz/front/range`, `/depz/imu/imu`, `/depz/depth/depth`,
`/depz/depth/points`), for a robot whose launch file already runs the node:

```bash
rviz2 -d $(ros2 pkg prefix --share depz_sensors)/rviz/depz_sensors.rviz
```

With other names, open the display in the *Displays* panel on the left and
pick your topic in its *Topic* field. When you add a display yourself (*Add* →
*By topic*), set its *Topic* → *Reliability Policy* to *Best Effort* if
nothing shows — see [QoS](#qos-ros2-topic-echo-or-my-subscriber-prints-nothing).
If RViz stays empty, see [RViz shows nothing](#rviz-shows-nothing).

### Board health: `/diagnostics`

Once a second the node publishes how each board is doing on `/diagnostics`
(`diagnostic_msgs/msg/DiagnosticArray`, one status per board, named
`depz_sensors: <board name>`):

<!-- doc-check: skip — a ROS tool, nothing of this repository to check -->
```bash
ros2 topic echo /diagnostics
```

`rqt_runtime_monitor` shows the same as a list. The level says it in one
word:

- **OK** — the board streams.
- **WARN** — it has gone silent and the node is restarting it
  ([A silent board is restarted](#a-silent-board-is-restarted)); or it
  publishes less than half of its configured rate, or since the previous
  status samples were dropped, reports did not parse, the USB link saw damaged
  packets or gaps, or the time sync is over 35 s old.
- **ERROR** — the board is lost (unplugged); the message is the cause, and the
  status then goes away with the board. Or it is silent and three restarts in
  a row did not bring it back; the node goes on restarting it.

The values under each status: model, port, topics, the measured and the
configured rate, messages published, samples dropped, the link's counters,
the time sync, the restarts of a silent board (`Restarted when silent for (s)`,
`Restarts`, `Recoveries`, `Last restart` — why, and how it went), the board's
MCU temperature (read every 10 s) and a BNO055's calibration status.
`diagnostics: false` turns it all off.

## Examples

Small programs that subscribe to the node's topics — start from them for your
own. They are installed with the package (sources in [`examples/`](examples)),
and each takes the topic to listen to as its `topic` parameter. All but one are
written with `rclpy`, the ROS 2 Python library.

Distance from a single-zone sensor ([`range_listener.py`](examples/range_listener.py))
— prints `83.1 cm`, `nothing in range` or `no valid measurement`:

```bash
ros2 run depz_sensors range_listener.py --ros-args -p topic:=/depz/hcsr04_uexample01/range
```

Every distance sensor at once ([`range_table.py`](examples/range_table.py)) —
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

The zone grid of a multizone sensor ([`depth_listener.py`](examples/depth_listener.py))
— prints the grid in centimetres, `--` for a zone without a target:

```bash
ros2 run depz_sensors depth_listener.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

The closest target a multizone sensor sees, and where in the image
([`depth_min_distance.py`](examples/depth_min_distance.py)) — prints
`closest 0.412 m at row 3, column 3` — row 0 is the top of the field,
column 0 its left:

```bash
ros2 run depz_sensors depth_min_distance.py --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

**The same in C++** ([`depth_min_distance.cpp`](examples/depth_min_distance.cpp)):
it prints exactly the same lines, which shows that a C++ program reads the same
topics with nothing of DEPZ — only `rclcpp` and `sensor_msgs`. Its name has no
`.py`:

```bash
ros2 run depz_sensors depth_min_distance --ros-args -p topic:=/depz/vl53l8ch_texample01/depth
```

It is built with the package; `--cmake-args -DDEPZ_SENSORS_BUILD_EXAMPLES=OFF`
on `colcon build` leaves it out.

Roll, pitch and yaw from an IMU ([`imu_listener.py`](examples/imu_listener.py))
— prints the angles in degrees and the acceleration, five times a second:

```bash
ros2 run depz_sensors imu_listener.py --ros-args -p topic:=/depz/bno086_iexample01/imu
```

What they show: subscribing with `qos_profile_sensor_data` in Python and
`rclcpp::SensorDataQoS()` in C++ (the QoS the node publishes with), telling
`+inf` and `NaN` from a distance, reading the floats out of a `32FC1` image,
turning a quaternion into angles, finding topics while the program runs.

### Your own launch file

[`launch/example_robot.launch.py`](launch/example_robot.launch.py) starts the
sensors node and the programs that use it with one command — the shape of a
launch file for your robot. It runs the node with
[the aliases example](#several-boards-and-aliases) (boards named `front`,
`depth` and `imu`), `depth_min_distance.py` on `depth`, `imu_listener.py` on
`imu` and `range_table.py` — and says where each board is mounted on the
robot, with a [static transform](#coordinate-frames) from `base_link` to the
board's frame:

```bash
ros2 launch depz_sensors example_robot.launch.py config:=$HOME/my_sensors.yaml namespace:=robot1
```

Inside it the topics are written without a leading `/` (`depz/depth/depth`),
so that `namespace:=robot1` moves the node's topics and the programs'
subscriptions together. Copy the file into your own package, put your
programs in place of the examples and your boards' real places into the
`mounted(...)` lines.

## Several boards and aliases

Plug in as many boards as you like, also several of the same model: each gets
its own topics, named `<model>_<USB serial>`. The serial number is printed on
the node's startup line, so the name of a board never depends on the port it
landed on or the order the boards were plugged in.

A name like `vl53l8ch_texample01` is stable but says nothing about where the
board sits on your robot. Give it an **alias** — `SERIAL=name` in the
`aliases` parameter:

```yaml
/**:
  ros__parameters:
    aliases:
      - "UEXAMPLE01=front"
      - "TEXAMPLE04=depth"
      - "IEXAMPLE01=imu"
    hot_plug: true
```

With this file the topics are `/depz/front/range`, `/depz/depth/depth`,
`/depz/imu/imu` and `/depz/imu/mag`, and the frames are `front`, `depth` and
`imu`. Boards without an alias keep their automatic names. The package has
this example ready as `config/example_aliases.yaml` — copy it, put in your
serial numbers, and launch:

```bash
cp $(ros2 pkg prefix --share depz_sensors)/config/example_aliases.yaml ~/my_sensors.yaml
ros2 launch depz_sensors sensors.launch.py config:=$HOME/my_sensors.yaml
```

Good to know:

- The first line of a parameter file, `/**:`, means "for any node"; keep it
  and `ros__parameters:` as they are.
- The serial number is compared without regard to upper and lower case. A name
  is turned into lower case, and anything but letters and digits into `_`.
- Two boards given the same alias, or two boards that would get the same name
  for any other reason, become `name` and `name_2`.
- **One board only:** set `auto_discover: false` and either `serial_number`
  or `device_path`; the node then opens that board and nothing else.

  ```yaml
  /**:
    ros__parameters:
      auto_discover: false
      serial_number: "TEXAMPLE01"
  ```

- **Boards plugged in later:** `hot_plug: true` makes the node look for new
  boards every second. A board that was unplugged and comes back gets the
  same name and topics again.
- **Two robots on one network:** `namespace:=robot1` on the launch command
  puts everything under `/robot1/`.

## Parameters

Set them in the YAML file passed as `config:=…`, or with `-p name:=value`
after `ros2 run … --ros-args`. They are read once, when the node starts: to
change one, restart the node (`ros2 param set` is refused while it runs). A
parameter the node does not have — a mistyped name in your file — is named in
a warning when the node starts.

Which boards are opened:

| Parameter | Type | Default | Meaning |
|---|---|---|---|
| `auto_discover` | bool | `true` | Open every DEPZ board found on USB. `false`: open only the board chosen by `serial_number` or `device_path` (one of them is then required). |
| `serial_number` | string | `""` | Open only the board with this USB serial number (upper or lower case). |
| `device_path` | string | `""` | Open only the board on this port: `/dev/ttyACM0`, or a stable link such as `/dev/serial/by-path/…`. |
| `hot_plug` | bool | `false` | Keep looking, once a second, for boards plugged in while the node runs. |
| `probe_timeout_ms` | int | `300` | How long a port may take to answer the "who are you" request during discovery. |
| `replay` | string list | `[]` | For tests and demos without hardware: `.depzrec` capture files to play instead of USB boards, each as one board. See [Limitations](#limitations). |

Names and delivery:

| Parameter | Type | Default | Meaning |
|---|---|---|---|
| `topic_prefix` | string | `"depz"` | First part of every topic: `/<topic_prefix>/<board name>/<leaf>`. Empty: `/<board name>/<leaf>`. |
| `aliases` | string list | `[]` | Names of your own for boards: `"SERIAL=name"` entries. |
| `frame_prefix` | string | `""` | Put in front of the board name to make the `frame_id` of its messages. |
| `qos` | string | `"sensor_data"` | How messages are delivered: `sensor_data` (best effort — a message that does not get through is dropped, not resent; the ROS default for sensors) or `reliable`. |
| `qos_depth` | int | `0` | How many messages are kept for a slow subscriber. `0`: the profile's own — 5 for `sensor_data`, 10 for `reliable`. |
| `time_sync` | bool | `true` | Stamp each message with the moment the board measured (`true`) or the moment it arrived at the computer (`false`). See [Time stamps](#time-stamps). |
| `diagnostics` | bool | `true` | Publish each board's health on `/diagnostics` once a second. See [Board health](#board-health-diagnostics). |

The sensors — one value for every board of the family (for one board only,
see [One board's own settings](#one-boards-own-settings)). A value a board
cannot take — out of the sensor's range, a mode its driver does not have —
stops that board only, with an error naming the board and the parameter and
saying what it accepts; the other boards go on. Nothing is quietly lowered to
a limit. A number may be written either way — `imu_rate_hz: 50` is 50.0,
`sr04_period_ms: 20.0` is 20 — where ROS alone would refuse the node for an
integer in place of a real number; a fraction for a whole-number parameter or
a text for a number stops the node at the start, with a message naming the
parameter.

| Parameter | Type | Default | Meaning |
|---|---|---|---|
| `sr04_period_ms` | int | `50` | HC-SR04: milliseconds between measurements (50 → 20 per second). |
| `range_budget_ms` | int | `50` | VL53L4CD, VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX: milliseconds spent on one measurement — longer is steadier and slower (100 → 10 per second). Must be within what the sensor accepts: 10 to 200 on a VL53L4CD and a VL53L3CX, 20 to 200 on a VL53L0X, 2 to 200 on a VL53L4CX; a VL53L1CX / L1CB takes only 20, 33, 50, 100, 200 and 500 (and 15 in its `short` mode). |
| `range_mode` | string | `""` | VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX: a distance mode of the sensor's driver — `short` / `long` on a VL53L1, `medium` / `long` on a VL53L4CX, … (the error for a wrong one lists them). `""`: the driver's own. A VL53L3CX has none: leave it `""` for that board. A VL53L4CD has none and ignores it. |
| `tof_resolution` | int | `64` | VL53L5CX / L7CX / L7CH / L8CX / L8CH: number of zones, `16` (4x4) or `64` (8x8). |
| `tof_frequency_hz` | int | `15` | The same boards: frames per second — up to 60 at 4x4 and 15 at 8x8, from 2 on a VL53L8 and from 1 on the others. |
| `tof_ranging_mode` | string | `""` | The same boards: `continuous` or `autonomous`. `""`: as the sensor starts. |
| `tof_integration_time_ms` | int | `0` | The same boards, in the `autonomous` mode: how long the sensor collects light at a time, 2 to 1000 ms — longer is steadier. `0`: as the sensor starts. A frame takes four of these at 8x8 (one at 4x4) plus about 10 ms; when that is longer than the frame period the sensor publishes fewer frames than `tof_frequency_hz` — at 8x8 and 15 Hz, up to 13 ms keeps 15 frames per second, 20 ms gives 11, 50 ms gives 4.7. |
| `tof_target_order` | string | `""` | The same boards: which target of a zone comes first, `closest` or `strongest`. `""`: as the sensor starts. |
| `tof_sharpener_percent` | int | `-1` | The same boards: how much light from neighbouring zones is removed, 0 to 99 % (0 = off). `-1`: as the sensor starts. A high value takes valid zones away on a flat surface: at 50 % a wall 1 m away lost a third of its zones on a VL53L7CH, at 99 % nearly all of them on every sensor. |
| `imu_rate_hz` | double | `100.0` | BNO055, BNO085, BNO086: messages per second (a BNO055 gives at most 100). |
| `bno055_mode` | string | `"ndof"` | BNO055: the fusion mode — `ndof`, `ndof_fmc_off`, `m4g`, `compass`, or `imu` (without the magnetometer: no `mag` topic). `compass` and `m4g` run without the gyroscope (`angular_velocity` is zero in `compass`; in `m4g` the chip works it out from the magnetometer) and the chip is slower: at `imu_rate_hz` 100 it gives about 61 messages per second in `compass` and 77 to 92 in `m4g` (it varies); 50 holds in both. |
| `bno086_orientation` | string | `"rotation_vector"` | BNO085, BNO086: where the `imu` orientation comes from — `rotation_vector` (with the magnetometer: a true compass heading) or `game_rotation_vector` (without it: no jumps near magnets, but the heading slowly drifts). |
| `bno086_mag_rate_hz` | double | `100.0` | BNO085, BNO086: magnetometer messages per second; `0`: no `mag` topic. The sensor hub runs rates of its own grid and takes the next one up: 20 asked gives 25. |
| `tmf8829_preset` | string | `"8x8"` | TMF8829 zone layout: `8x8`, `16x16`, `32x32`, `48x32`, the high-accuracy `8x8-ha`, `16x16-ha`, `32x32-ha`, `48x32-ha`, and the long-range `8x8-lr`, `8x8-xr`. |
| `tmf8829_period_ms` | int | `33` | TMF8829: milliseconds between measurements (33 → 30 per second). |
| `silent_restart_after_s` | double | `1.0` | Every board: seconds without a message after which a board that is still plugged in is restarted — see [A silent board is restarted](#a-silent-board-is-restarted). `0`: never. |

### A silent board is restarted

A board can stay on USB and stop delivering: its sensor has hung, or lost
power, while the board's own processor still answers. The node notices and
restarts that board by itself — closes it, opens it and starts its sensor
again with the same parameters, as it starts a board that is plugged in. The
board's topics stay (a subscriber sees a pause, nothing more), and the other
boards go on publishing meanwhile.

- **Silent** means: no message on the board's main topic (`range`, `depth` or
  `imu`) for `silent_restart_after_s` — 1 s by default — and for five of the
  board's periods when those are longer: a board set to 2 messages per second
  is silent after 2.5 s. For a VL53L5CX / L7 / L8 with a
  `tof_integration_time_ms` the period is the time a frame then takes.
- **Restarts** are made at once when the silence is found, then 1, 2, 4, 8 and
  16 s after the restart before, then every 30 s — for as long as the board is
  plugged in and silent; the node never gives up. A restart that went through
  is given the board's silence time before the next one, when that is longer
  than the wait. Once the board has delivered
  for a minute, the waits start at 1 s again.
- **What you see:** once per silence, before the first restart, what came
  from the board in the seconds before it — samples, packets and bytes a
  second, whether it still sent anything after its last sample, and its last
  events; then one line in the log per restart
  ([One board stops](#one-board-stops) shows them), and on
  [`/diagnostics`](#board-health-diagnostics) the board at WARN while silent,
  at ERROR once three restarts in a row brought no message, at OK again when
  it delivers — with the restarts counted in its values.

`silent_restart_after_s: 0.0` switches the restarts off; like every sensor
parameter it can be given for one board
([One board's own settings](#one-boards-own-settings)). A board replayed from
a capture is never restarted. One board is restarted at a time, on the node's
main thread: while a restart is under way — a fraction of a second, the second
and a half of its start for a VL53L5CX / L7 / L8 or a BNO085 / BNO086, longer
when the board's processor does not answer and each request waits out its
time — `/diagnostics` and the search for newly plugged boards (`hot_plug`)
wait. The other boards' topics do not.

### Extra topics

Off by default. Each is a sensor parameter, like those above (for a family,
or for one board), and adds a topic under the board's name, with the frame
and the time stamp of the sample it comes from:

| Parameter | Type | Default | Boards | Topic | Message |
|---|---|---|---|---|---|
| `mcu_temperature` | bool | `false` | every board | `mcu_temperature` | `sensor_msgs/msg/Temperature`, °C, once a second (stamped on arrival; not for a replayed board) |
| `sensor_temperature` | bool | `false` | VL53L5CX / L7 / L8, TMF8829, BNO055 | `temperature` | `sensor_msgs/msg/Temperature`, °C, the sensor chip's own, with each sample |
| `tof_signal` | bool | `false` | VL53L5CX / L7 / L8 | `depth/signal` | `sensor_msgs/msg/Image` `32FC1`: the signal strength of each zone, kcps/SPAD — also of a zone the depth image marks `NaN` |
| `tof_status` | bool | `false` | VL53L5CX / L7 / L8 | `depth/status` | `sensor_msgs/msg/Image` `mono8`: ST's target status of each zone (5 and 9 are valid, 255 is no target) |
| `tmf8829_peaks` | int | `1` | TMF8829 | `depth/peak_2` … `depth/peak_4` | how many targets per zone, 1 to 4; the depth image is the first, each further one an image of its own (metres, `NaN` where a zone has no such target) |
| `point_cloud` | bool | `false` | VL53L5CX / L7 / L8, TMF8829 | `points` | `sensor_msgs/msg/PointCloud2`: each zone's target as a point in space, with each depth image — [below](#point-cloud) |
| `imu_linear_acceleration` | bool | `false` | BNO055, BNO085 / BNO086 | `linear_acceleration` | `geometry_msgs/msg/Vector3Stamped`, m/s²: the acceleration with gravity taken out |
| `imu_gravity` | bool | `false` | BNO055, BNO085 / BNO086 | `gravity` | `geometry_msgs/msg/Vector3Stamped`, m/s²: the direction of gravity |

The images are laid out as the depth image: row 0 the top of the field, column 0 its left.

### Point cloud

`point_cloud: true` gives every multizone board a topic `points`
(`sensor_msgs/msg/PointCloud2`): with each depth image, the same zones as
points in space, in metres, under the image's stamp and frame.

- **Layout.** Three `float32` fields, `x`, `y`, `z`. One point per zone:
  `height` x `width` is the zone grid and point *i* is pixel *i* of the depth
  image. A zone the image marks `NaN` is the point `NaN, NaN, NaN`
  (`is_dense` is false) — RViz skips such points; in your own code, skip a
  point whose `x` is `NaN`.
- **Axes** ([REP 103](https://www.ros.org/reps/rep-0103.html)): `x` forward,
  out of the sensor along its optical axis; `y` to the sensor's left; `z` up.
  The origin is the sensor. "Left" and "up" are the board's, held with its
  USB connector down, as seen from behind it: how the board sits on your
  robot is the static transform you give ([Coordinate frames](#coordinate-frames)).
  The image's column runs toward `-y` (to the right), its row toward `-z`
  (down): pixel 0 looks up and to the left — the depth image is the field
  as the sensor sees it.
- **The first peak only** for a TMF8829 (`depth`, not `depth/peak_2` …).

How far off the axis a zone looks is computed from the manufacturer's
figures for the model; which way it looks was measured on the board
([below](#which-way-a-zone-looks)).

| Model | Field of view across the zone grid | Source |
|---|---|---|
| VL53L5CX | 45° x 45° | ST datasheet DS13754 Rev 13, Table 2 "FoV angles" (detection volume), p. 4 |
| VL53L7CX | 60° x 60° | ST DS13865 Rev 8, Table 2, p. 4 |
| VL53L7CH | 60° x 60° | ST DS14309 Rev 3, Table 2, p. 4 |
| VL53L8CX | 45° x 45° | ST DS14161 Rev 7, Table 2, p. 4 |
| VL53L8CH | 45° x 45° | ST DS14310 Rev 3, Table 2, p. 4 |
| TMF8829 | 67.4° x 53.1° (the zones at ± 2/3 and ± 1/2 of the distance); on a DEPZ board the 67.4° runs up and down | ams OSRAM AN001096 v3-00, section 4.8 "3D point cloud calculation", pp. 43–44; the datasheet DS001140 v2-00, p. 15, gives 67.9° x 52.8° |

- A VL53L5CX / L7 / L8 reports each zone's distance **to the plane through
  the sensor**, not along the zone's line of sight: in front of a wall every
  zone reports the same distance. That distance is the point's `x`; `y` and
  `z` are `x` times the tangents of the angles of the zone's centre. The zones
  divide the field of view evenly on a wall square to the sensor (ST AN5894
  Rev 2, section 2.3). No ST
  datasheet says which of the two distances the sensor reports: ST's support
  forum does, and the bench confirms it (below).
- A TMF8829 reports the distance **along the zone's line of sight**. ams
  OSRAM's note gives each zone's place and the factor that turns its distance
  into `x`. The node takes the middle of the grid as the centre. The note
  writes it as `pixel_x - ((X-1)/2) - 0.5`, which is the middle with the
  integer division its Python expects (3 at `X = 8`, so 3.5) and half a zone
  off with a true division; the bench agrees with the middle: only so is a
  wall the same plane at 8x8, 16x16 and 48x32.

On our bench (2026-11-13, four boards about 1 m from a wall, each mounted a
few degrees askew) the points of each board lie on one plane:

| Board | Grid | Plane from the sensor | Points off the plane (RMS) | … with the other reading of the distance |
|---|---|---|---|---|
| VL53L8CH | 8x8 | 1.005 m | 4 mm | 27 mm |
| VL53L5CX | 8x8 | 1.021 m | 10 mm | 24 mm |
| VL53L7CH | 8x8 | 1.015 m | 15 mm | 44 mm |
| TMF8829 | 8x8 | 1.019 m | 13 mm | 74 mm |
| TMF8829 | 16x16 | 1.020 m | 14 mm | 78 mm |
| TMF8829 | 48x32 | 1.015 m | 16 mm | 81 mm |

(10 of the 64 zones of the VL53L5CX and 13 of the VL53L7CH, at one edge of
their fields, had no steady target and are not in the figures.) Which way
round the cloud is was measured apart ([below](#which-way-a-zone-looks)): a
bare wall cannot tell, as a cloud flipped left to right is the same plane.
The VL53L7CX and the VL53L8CX are not on the
bench; they use the same code with their datasheets' figures.


### Which way a zone looks

No datasheet can say this for a DEPZ board: the manufacturers draw the bare
chip, and the chip sits on the board turned. It was measured on the bench on
2026-11-16: every board held with its USB connector down, a hand 5 to 10 cm
in front of the top half, then the left half, of its field, and the zones
that read the hand found in the sensor's own data (the order the SDKs give):

| Model | Hand over the top half | Hand over the left half | So the node |
|---|---|---|---|
| VL53L5CX, VL53L7CH, VL53L8CH | the zone grid's first columns | its last rows | turns the grid a quarter turn clockwise |
| TMF8829 | the grid's first columns | its first rows | swaps rows and columns |

The VL53L7CX and the VL53L8CX are not on the bench: they are taken to sit as
their CH siblings do.

What the cloud does not have: the zone's size (a point is the zone's centre —
a zone of a VL53L8 at 1 m covers about 10 cm x 10 cm), per-board calibration
of the optics, intensity. The field of view is the manufacturer's typical
figure; the real edge of what a sensor detects moves with the target and the
light.

To see it in RViz 2: *Add* → *By topic* → `points` → *PointCloud2*, with
*Reliability Policy* *Best Effort*, *Style* *Spheres* and a *Size* of a few
centimetres, and the board's frame as the *Fixed Frame* (or a static
transform to it).

### One board's own settings

Any sensor parameter — and `frame_id` — can be given for one board, under
its name (its alias when it has one). It wins over the family's value:

```yaml
/**:
  ros__parameters:
    aliases: ["TEXAMPLE01=front_tof"]
    tof_frequency_hz: 10          # every VL53L5CX / L7 / L8 ...
    boards:
      front_tof:                  # ... but this one
        tof_resolution: 16
        tof_frequency_hz: 30
        frame_id: "front_tof_link"
```

`frame_id` here replaces `frame_prefix` + name for that board. A parameter of
another family under a board's name stops that board with an error.

## Coordinate frames

Every message carries a `frame_id` in its header: the name of the coordinate
frame the measurement is in. For this package it is simply **the board's
name** — `hcsr04_uexample01`, or `front` with an alias — with `frame_prefix`
in front if you set one. One board, one frame; its `imu` and `mag` share it.

| Board | What the frame is |
|---|---|
| Distance sensors (`range`) | The sensor. The beam leaves along the frame's **x axis** — that is how RViz draws the cone. |
| Multizone sensors (`depth`, `points`) | The sensor. The image is the zone grid as the sensor numbers it, not a camera image: there is no `camera_info`. The [point cloud](#point-cloud) has `x` forward out of the sensor, `y` to its left, `z` up. |
| IMUs (`imu`, `mag`) | The sensor chip: angular velocity, acceleration and magnetic field are along the chip's own x, y, z axes. |

The node does not know where a board is mounted on your robot, so it
publishes no transforms. You say it — once per board, with a *static
transform* from a frame of your robot (usually `base_link`) to the board's
frame:

<!-- doc-check: skip — tf2_ros, a ROS tool of another package -->
```bash
ros2 run tf2_ros static_transform_publisher --x 0.10 --y 0 --z 0.05 --yaw 0 --pitch 0 --roll 0 --frame-id base_link --child-frame-id front
```

This says: the frame `front` is 10 cm ahead of `base_link` and 5 cm above it,
not rotated (so the sensor looks forward, along x). Distances are metres,
angles radians; `--yaw 1.5708` would turn it to look left. Leave the command
running — one terminal per board. With it, RViz and other programs can place
the measurement in space.

On a robot the transforms belong in its launch file (or its URDF), started
with the node. [`launch/example_robot.launch.py`](launch/example_robot.launch.py)
does it for three boards — the same program, one line per board:

<!-- doc-check: skip — a part of a ROS launch file: launch_ros is not on the host that runs this check; test_example_robot runs the file -->
```python
def mounted(frame, x, y, z, yaw=0.0):
    return Node(package="tf2_ros", executable="static_transform_publisher", name=f"{frame}_mount",
                namespace=namespace,
                arguments=["--x", str(x), "--y", str(y), "--z", str(z), "--yaw", str(yaw),
                           "--frame-id", "base_link", "--child-frame-id", frame])

mounted("front", 0.10, 0.0, 0.05),                  # 10 cm ahead of the robot's centre, 5 cm up, looking forward
mounted("depth", 0.0, 0.0, 0.20, yaw=math.pi / 2),  # 20 cm up, looking to the left
mounted("imu", 0.0, 0.0, 0.02),                     # at the centre, 2 cm up
```

`ros2 launch depz_sensors view.launch.py` needs none of this: it
[places the boards itself](#everything-at-once-rviz-2), side by side.

## Time stamps

`header.stamp` says when the board had the measurement, on the computer's
clock: the moment the sensor reported its data ready (the end of a
measurement, not its middle), not the moment the message reached the computer.
Every message made from one sample carries the same stamp (a depth image and
its `depth/signal`, a BNO055's `imu` and `mag`).

- **`time_sync: true`** (the default). Each board has its own clock and stamps
  every sample with it. The node compares the board's clock with the
  computer's when it opens the board and again every 2 seconds, and converts
  the board's stamp into computer time. The boards' clocks are not exact — on
  our bench they run up to 0.04 % fast, and unevenly — so between two
  comparisons the node goes on at the rate of the last ones.
- **`time_sync: false`**. The stamp is the moment the sample arrived at the
  node: simple, later than the measurement by the times in the table below,
  and as uneven as USB delivers.

What we measured: 13 boards at once on one computer (Ubuntu 24.04, ROS 2
Jazzy), default parameters, 10 minutes, 370 000 messages. "Age" is how old the
stamp is when the node publishes the message — the time the sensor's data
takes to reach the computer; a subscriber on the same computer got the
message 0.6 ms later (median; under 2 ms for 19 in 20).

| Sensor | Topic | Age at publication, ms: median (5 % .. 95 % of messages) |
|---|---|---|
| HC-SR04 | `range` | 0.25 (0.05 .. 0.45) |
| VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD | `range` | 0.5 to 0.7 (0.1 .. 1.0) |
| VL53L4CX | `range` | 1.3 (1.0 .. 1.6) |
| VL53L8CH | `depth` | 7.6 (7.3 .. 8.0) |
| VL53L5CX, VL53L7CH | `depth` | 17.4 to 17.7 (17.1 .. 18.0) |
| TMF8829 | `depth` | 18.2 (18.0 .. 18.5) |
| BNO055 | `imu`, `mag` | 2.3 (1.6 .. 5.6) |
| BNO086 | `imu` | 3.3 (2.9 .. 3.9) |
| BNO086 | `mag` | 5.0 (4.4 .. 5.6) |

A depth image is old because the board first reads the whole frame from the
sensor; an IMU sample waits for its USB packet. A BNO085 is the BNO086's
sensor hub under another name; ours was not in this run.

What you can rely on:

- **Order.** The stamps of a topic never step back, and no stamp is later than
  the moment the node publishes the message. In the 10 minutes above: none
  back, none repeated, none in the future, with 300 clock comparisons per
  board. When a new comparison would put a message before the previous one,
  it gets the previous one's stamp instead, so two messages in a row can
  carry the same stamp (seen once: one Imu message in 73 starts of the node).
- **Evenness.** The stamps follow the board's clock, not the USB delivery: the
  time between two stamps varied by 0.02 to 0.3 ms (standard deviation), the
  time between two publications by 0.1 to 1.4 ms.
- **Accuracy.** Take a stamp as good to about half a millisecond. A
  comparison of the two clocks is good to half of its round trip over USB,
  which took 0.35 ms (median; 1 ms at worst). Each new comparison moved the
  stamps by 0.1 to 0.2 ms (median per board; under 0.7 ms for 19 in 20,
  0.9 ms the largest). Between two comparisons the stamps did not drift:
  0.1 ms or less from just after one to just before the next.
- **What the stamp is not.** It is the end of the measurement. A sensor that
  collects light for 50 ms saw the scene, on average, 25 ms before its stamp;
  the node does not correct for that.
- **Fallbacks.** If a converted stamp would lie after the arrival or more than
  5 seconds back (a board that restarted), the node uses the arrival time for
  that message. The `mcu_temperature` topic and a replayed capture are always
  stamped on arrival.
- The stamps are on the computer's clock: if that clock is set back, the
  stamps go back with it.

## Troubleshooting

The node says what is wrong in its log: the terminal where `ros2 launch`
runs. Each case below starts with the line to look for — the level (`WARN`,
`ERROR`, `FATAL`) and the message, as the node prints them; ports, names and
serial numbers are examples. On the screen the line has the program, the time
and the node's name in front (`robot1.depz_sensors` under
`namespace:=robot1`):

<!-- doc-check: skip — the node's output, a whole line -->
```text
[sensors_node-1] [WARN] [1794555208.522840115] [depz_sensors]: no DEPZ board found
```

Which case is yours:

| What you see | Go to |
|---|---|
| `no DEPZ board found` | [No board found](#no-board-found) |
| `no read/write access` | [Permissions](#permissions-no-readwrite-access) |
| `no answer to the probe` or `trying once more` just after plugging in | [The udev rule](#the-udev-rule-a-board-does-not-answer-just-after-plugging-in) |
| `no answer to the probe … the board is left out` every time | [The board is open in another program](#the-board-is-open-in-another-program) |
| `disconnected: … its topics stop publishing`, `no sample for … — restarted`, or one topic goes quiet | [One board stops](#one-board-stops) |
| Fewer messages per second than expected | [Low rate under load](#low-rate-under-load) |
| `incompatible QoS`, or `ros2 topic echo` prints nothing | [QoS](#qos-ros2-topic-echo-or-my-subscriber-prints-nothing) |
| RViz is empty or a display shows an error | [RViz shows nothing](#rviz-shows-nothing) |
| `could not start` | [`could not start`](#could-not-start) |
| `is in bootloader mode` | [`is in bootloader mode`](#is-in-bootloader-mode) |

### No board found

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] no DEPZ board found
[WARN] no DEPZ board found — waiting for one (hot_plug)
```

The second is the same with `hot_plug: true`: the node stays and takes a board
as soon as one appears. The node goes on running either way, with no topics.

- Read the lines above this one first: a port the node saw and could not use
  has its own line there — [permissions](#permissions-no-readwrite-access),
  [another program](#the-board-is-open-in-another-program),
  [the bootloader](#is-in-bootloader-mode). No line above: Linux shows the
  node no DEPZ board at all.
- Is the board plugged in and seen by Linux? `ls /dev/ttyACM*` must list a
  port, and `lsusb` a device with the id `1bcf:…` (a development board:
  `0483:56dc`). Nothing there: try another cable (some USB cables only
  charge) or port.
- With `auto_discover: false`: do `serial_number` / `device_path` name the
  board exactly as the startup line of a normal run shows it?
- Plugged in after the node started: restart the node, or run it with
  `hot_plug: true`.

### Permissions: `no read/write access`

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[ERROR] /dev/ttyACM0: no read/write access — add your user to the 'dialout' group (sudo usermod -aG dialout $USER, then log in again) or install the DEPZ udev rule (see the depz_sensors README)
```

Linux did not let the node open the port; `no DEPZ board found` follows when
it was the only board. Do [USB permissions](#usb-permissions), then replug the
board (after the udev rule) or log in again (after `dialout`). Check with
`ls -l /dev/ttyACM0` — the line must show your user or a group that `groups`
lists.

- A `device_path` that does not exist gets the same line: check the path.
- In a Docker container of your own the line means the port is not in the
  container: `--device /dev/ttyACM0` gives it the ports of the moment it
  starts, and a board that is plugged in later — or comes back under another
  number (`hot_plug`) — is not among them. Give it the machine's `/dev` and
  every `ttyACM` port instead, as `scripts/ros.sh` does:
  `-v /dev:/dev --device-cgroup-rule 'c 166:* rmw'`.

### The udev rule: a board does not answer just after plugging in

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] /dev/ttyACM0: no answer to the probe within 300 ms — answered at the second try
[WARN] hcsr04_uexample01 (hcsr04, /dev/ttyACM0): no reply to cmd 0x33 within 200 ms — trying once more
[WARN] /dev/ttyACM0 (USB serial UEXAMPLE01): no answer to the probe within 300 ms, twice — the board is left out
```

The node asks each port "who are you" (the probe) and then sets the sensor
up. A request that gets no answer is asked once more — the first two lines:
the board then runs, nothing is lost. After two unanswered probes the board
is left out (the third line; with `hot_plug` it ends
`— asked again every second (hot_plug)` and the board joins as soon as it
answers).

When this happens in the first seconds after a board is plugged in — or after
the computer starts — and not later, it is ModemManager: a system service that
probes every new serial port as if it were a modem; its probing makes the
first requests to a board time out. Install [the udev rule](#usb-permissions),
which tells ModemManager to leave DEPZ boards alone, and replug the boards. To
check:

<!-- doc-check: skip — system commands, nothing of this repository to check -->
```bash
ls /etc/udev/rules.d/60-depz-sensors.rules    # the rule is installed
systemctl is-active ModemManager              # "active": the service runs on this machine
```

If the third line comes every time, also long after plugging in, see the next
case.

### The board is open in another program

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] /dev/ttyACM0 (USB serial TEXAMPLE01): no answer to the probe within 300 ms, twice — the board is left out
```

A port has one owner at a time. A program built on a DEPZ SDK — the DEPZ
viewer, a script of yours, a second copy of this node — locks the port it
opens, and the node then cannot open it. It does not say "busy": it reports
the port as one that did not answer, the line above (and `no DEPZ board
found` when it was the only board).

- Find the owner: `lsof /dev/ttyACM0` lists the programs that have the port
  open. Close that program; then restart the node — with `hot_plug: true` it
  takes the board by itself within a second.
- A program that opens the port without locking it — a serial terminal, a
  browser tab with the web viewer — does not keep the node out; the two then
  take each other's bytes, and requests go unanswered at random (`no reply to
  cmd … — trying once more`, [`could not start`](#could-not-start)). Close it.
- `ros2 launch depz_sensors view.launch.py` starts the node itself: do not
  run it beside `sensors.launch.py`.

### One board stops

The others go on; the node never stops because of one board.

**It was unplugged, or USB lost it** (a loose cable, a hub without enough
power):

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[ERROR] front (/dev/ttyACM3) disconnected: /dev/ttyACM3: device lost — its topics stop publishing
[INFO] front: closed
```

The first line names the board and the cause; its topics disappear, and
`/diagnostics` shows it at the level ERROR with the message
`disconnected: /dev/ttyACM3: device lost` before its status goes away. Plug it back and restart the
node — or run with `hot_plug: true`: it comes back by itself, under the same
name and topics, with its startup line. If it keeps happening with the cable
untouched, try another cable, another port, a powered hub.

**It is still plugged in and its topic went quiet.** The board is there, its
sensor has stopped delivering. The node first says what came from the board
before — once per silence, ahead of its first restart:

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] front (/dev/ttyACM3): silent for 1.0 s; what came before:
  what it sent, a line a second (samples published / packets / bytes from the board):
    -2.2 s: 15 samples, 15 packets, 3900 bytes
    -1.2 s: 14 samples, 14 packets, 3640 bytes
    -0.2 s: 3 samples, 3 packets, 780 bytes
  the last sample 1.0 s ago; from -0.8 s to 0.0 s 0 packets, 0 bytes — the board sent nothing
  no event from the board or the link in that time
```

Ten such lines a second back at most. The verdict at the end tells two
faults apart: **the board sent nothing** (its sensor or its processor
stopped, or USB stopped carrying it) and **the board sent, but no sample came
of it** (what arrived did not decode — the link's errors are named). Below
come the board's last events, if any: text its firmware sent, a hardware
fault it reported, bad packets. Keep these lines when you report a silent
board. Then the node [restarts the board](#a-silent-board-is-restarted) and
says so, one line per restart:

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] front (/dev/ttyACM3): no sample for 1.2 s — restarted (restart 1); if it stays silent, again in 1 s
[INFO] front (/dev/ttyACM3): publishes again after 1.6 s of silence and 1 restart
```

The second line is the end of it: the board delivers again, nothing to do.
A sensor that cannot be started yet makes the restart fail, and the node tries
again after the wait it names — a line each time, an `[ERROR]` line from the
third failure in a row on:

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] front (/dev/ttyACM3): no sample for 1.2 s — restart 1 failed: cmd 0x32 failed: ERR_HARDWARE_FAULT (0x09) — the next in 1 s
[WARN] front (/dev/ttyACM3): no sample for 2.4 s — restart 2 failed: cmd 0x32 failed: ERR_HARDWARE_FAULT (0x09) — the next in 2 s
[ERROR] front (/dev/ttyACM3): no sample for 4.6 s — restart 3 failed: cmd 0x32 failed: ERR_HARDWARE_FAULT (0x09) — the next in 4 s
```

Meanwhile [`/diagnostics`](#board-health-diagnostics) shows the board at WARN,
then at ERROR, with messages such as

<!-- doc-check: skip — /diagnostics status messages, not log lines -->
```text
no sample for 2.5 s, restarted 2 times
no sample for 5.4 s, restarted 3 times: 3 restarts in a row did not bring it back
```

The restarts go on — after 8 and 16 s, then every 30 s; from then on the log
has one of these lines in five minutes, the status counts them all — and the
board comes back by itself when its sensor can deliver again. If it does not:
the reason is in the line (`ERR_HARDWARE_FAULT` — the sensor does not answer
the board: its cable or its power; `no reply` — the board's processor itself
hangs); unplug the board and plug it in again (with `hot_plug: true` the node
takes it back), and check the sensor's wiring. A board that falls silent again
and again within a minute keeps the longer waits, and its `Restarts` on
`/diagnostics` grow: look at its power and its cable.

With `silent_restart_after_s: 0.0` the node leaves a silent board alone: it
prints nothing, and `/diagnostics` shows the board at WARN with

<!-- doc-check: skip — a /diagnostics status message, not a log line -->
```text
publishes nothing, 20.0 Hz configured
```

(A capture given by `replay` that has played to its end shows as
`disconnected: the replay ended`; that is not a fault.)

**It never started.** See [`could not start`](#could-not-start).

### Low rate under load

`ros2 topic hz` shows fewer messages per second than
[the table](#what-each-board-publishes) says. The node prints no line for
this; it says where the messages go missing on
[`/diagnostics`](#board-health-diagnostics), once a second per board
(`ros2 topic echo /diagnostics`): the values `Publish rate (Hz)` and
`Configured rate (Hz)`, and at the level WARN a message such as

<!-- doc-check: skip — /diagnostics status messages, not log lines -->
```text
publishes at 4.8 Hz, 15.0 Hz configured
3 samples dropped
```

- **`publishes at … Hz, … Hz configured`** (the board gives less than half of
  its rate) with nothing dropped: the sensor itself is slower than asked. That
  is a setting, not load — a long `tof_integration_time_ms`, a BNO055 mode
  without the gyroscope, a rate the sensor does not reach; the
  [parameter table](#parameters) gives the rates each setting can hold.
- **`… samples dropped`**: the board sent the samples and the node did not
  take them in time — the computer is too busy. Give the node more of it:
  stop other heavy programs, lower the boards' rates, run fewer boards on a
  small computer.
- **`/diagnostics` is at OK with the full rate, and your program still gets
  fewer.** The messages are lost between the node and your subscriber. The
  node publishes best effort: when the subscriber is slower than the topic, or
  the network between two computers drops packets (Wi-Fi), a message is
  dropped, not resent. Make the subscriber's callback lighter, or raise
  `qos_depth`; with `qos: "reliable"` nothing is dropped while the subscriber
  keeps up within `qos_depth` messages.
- `CRC errors`, `sequence gaps` in the same message: USB itself loses data —
  the cable or the hub.

For a measure of what one computer carries: 14 boards at once ran an hour at
their default rates without a pause ([Limitations](#limitations)).

### QoS: `ros2 topic echo` or my subscriber prints nothing

QoS, *quality of service*, is the delivery contract of a topic. The node
publishes **best effort** — the ROS convention for sensors: a lost message is
not resent. A subscriber that insists on **reliable** delivery is not
compatible with that and gets nothing. Both sides say so, once — the
subscriber (here `ros2 topic echo --qos-reliability reliable`):

<!-- doc-check: skip — a ROS warning, as ros2 topic echo printed it on Jazzy and on Lyrical -->
```text
[WARN] [1794555345.450783552] [_ros2cli_74]: New publisher discovered on topic '/depz/front/range', offering incompatible QoS. No messages will be received from it. Last incompatible policy: RELIABILITY
```

and the node, in its own terminal:

<!-- doc-check: skip — a ROS warning under the node's name, as Jazzy printed it -->
```text
[WARN] [1794554923.918369305] [depz_sensors]: New subscription discovered on topic '/depz/front/range', requesting incompatible QoS. No messages will be sent to it. Last incompatible policy: RELIABILITY_QOS_POLICY
```

- `ros2 topic echo` and `ros2 topic hz` without options adapt to the
  publisher by themselves (on Jazzy and on Lyrical). If yours does not, or you
  gave it a QoS option, ask for best effort:

  <!-- doc-check: skip — a ros2 topic command on the user's own topic name -->
  ```bash
  ros2 topic echo --qos-reliability best_effort /depz/front/range
  ```

- In your own code subscribe with `qos_profile_sensor_data` in Python and
  `rclcpp::SensorDataQoS()` in C++, as the [examples](#examples) do.
- In RViz: the display's *Topic* → *Reliability Policy* → *Best Effort*.
- Or make the node publish reliably: `qos: "reliable"`. Every kind of
  subscriber then works.

`ros2 topic info -v <topic>` shows the QoS of the publisher and of every
subscriber.

### RViz shows nothing

`ros2 launch depz_sensors view.launch.py` says why it does not start RViz, or
what RViz will lack:

<!-- doc-check: skip — the output of view.launch.py's program; each line is held against its source -->
```text
[ERROR] depz_sensors publishes no topic: no board was started — see its lines above and "Troubleshooting" in the depz_sensors README
[WARN] depz_sensors publishes no topic yet — RViz starts when a board is plugged in
[ERROR] rviz2: no such program — RViz 2 is not installed: sudo apt install ros-jazzy-rviz2
[WARN] the Imu display of RViz is not installed — the imu topics are not drawn: sudo apt install ros-jazzy-rviz-imu-plugin
```

- The first: the node found no board — [No board found](#no-board-found); the
  launch ends. The second is the same with `hot_plug: true`: it waits.
- The third and fourth name the package to install. Without the Imu plug-in
  RViz starts and lists the IMU's display as failed.
- RViz is open and a display has a red *Transform* error: RViz does not know
  where that board is. With `place_boards:=false`, or with
  `rviz/depz_sensors.rviz` opened by hand, the transforms are yours to
  publish — [Coordinate frames](#coordinate-frames) — and *Fixed Frame* (top
  of the *Displays* panel) must be a frame they start from.
- A display you added yourself stays empty: its *Reliability Policy* —
  [QoS](#qos-ros2-topic-echo-or-my-subscriber-prints-nothing).
- A board plugged in after RViz started is not in the view: close RViz and
  launch again.
- The launch ends at once, or RViz closes by itself: the node ended, and the
  launch ends with it. Its last lines say why — a `[FATAL]` line names the
  parameter it cannot run with.

### `could not start`

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[ERROR] hcsr04_uexample01 (hcsr04, /dev/ttyACM0): could not start: no reply to cmd 0x33 within 200 ms
[INFO] started 13 of 14 boards found
```

The board answered the discovery but a later step failed — after a second
try, when the step got no answer; the message ends with the reason, and the
other boards run. Replug the board and start again. A reason that names a
parameter is a value this board cannot take: the message says what it accepts
([Parameters](#parameters)). If it says `no ROS mapping for this board`, the
board's firmware is one this package has no topics for.

### `is in bootloader mode`

<!-- doc-check: skip — the node's output; each line is held against the node's source -->
```text
[WARN] /dev/ttyACM0 is in bootloader mode (BOOTDEPZ_v1.20): flash or reset it to use it
```

The board is running its bootloader — the small program that installs
firmware — not the sensor firmware, so it has no measurements to give. This
happens during a firmware update, or when an update did not finish. Finish or
repeat the update with the DEPZ viewer
([sensor-viewer.depz.ai](https://sensor-viewer.depz.ai), in Chrome or Edge),
then start the node again. The node skips such a board and goes on with the
others.

### A board without a serial number

Every DEPZ production board has a unique USB serial number. A development
unit may have none; the node then names it after its port —
`hcsr04_ttyacm0` — and that name is only as stable as the port: it can change
when boards are plugged in another order. Two ways to pin it down:

- Open it by the physical USB socket it is in. `ls -l /dev/serial/by-path/`
  shows a link per port that stays the same while the board stays in that
  socket; give it as `device_path` (with `auto_discover: false`).
- If the board has a serial number in its own memory (the startup line shows
  the name built from it), give that number an alias.

## Limitations

- **Linux only**, built from source. Run with real boards, 14 at once, on
  ROS 2 Jazzy (Ubuntu 24.04; an hour without a pause) and on ROS 2 Lyrical
  (15 minutes); the tests of both run on recorded sessions.
- **No transforms, no `camera_info`.** The node publishes the measurements;
  where a board sits on the robot is yours to say
  ([Coordinate frames](#coordinate-frames)); `view.launch.py` lays the boards
  out side by side for a look, no more. The [point cloud](#point-cloud)
  is off by default and is computed from the manufacturer's field of view of
  the model, not calibrated per board.
- **One distance per zone.** `range` and `depth` carry the closest target.
  What the sensors measure beyond that — signal strength, ambient light,
  further targets, histograms — is available through the DEPZ SDKs, not on
  these topics.
- **IMU orientation is the chip's own**, published as the chip reports it; the
  covariances of angular velocity and acceleration are unknown (zeros).
- **Parameters are read at startup.** `ros2 param set` on the running node
  changes nothing; restart it.
- **No automatic reconnection** unless `hot_plug` is on. A board that stays
  plugged in and goes silent is restarted without it
  ([A silent board is restarted](#a-silent-board-is-restarted)).
- **A board without a USB serial number** has a name that depends on its port
  ([above](#a-board-without-a-serial-number)).
- **A port has one owner:** while the node runs, the DEPZ viewer and other
  SDK programs cannot open the same board, and the other way round.
- **`replay`** is for tests and demos. A capture is played once, starts only
  when each of its topics has a subscriber, and must have been recorded with
  the same settings the node runs with. It starts at the *first* subscriber
  of each topic: a second one — even a program started before the node — may
  be matched after the start and miss part or all of a short capture, so give
  a replayed topic one subscriber, or play the same file from a second node
  for the second program.

## More

- The boards and the SDKs: [depz.ai/developers/sensors/sdk](https://depz.ai/developers/sensors/sdk).
- The C++ SDK under this package: [`../depz-sensor-sdk-cpp`](../depz-sensor-sdk-cpp/README.md).
- Source and license (MIT): [github.com/depz-ai/depz-sensor-sdk](https://github.com/depz-ai/depz-sensor-sdk).
