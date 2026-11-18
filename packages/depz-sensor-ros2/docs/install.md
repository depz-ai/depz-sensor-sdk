<!-- generated from ../README.md — edit that page -->
# ROS 2 — install

How to get `depz_sensors` onto a machine: ROS 2 itself, the sources, the build
with `colcon` — or a Docker container instead of all three — and the USB
permissions of the boards. The [introduction](intro.md) has the short version.

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
