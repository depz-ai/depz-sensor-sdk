#!/usr/bin/env bash
# Build and test depz_sensors in a ROS 2 container (no ROS on the host needed).
#
#   scripts/ros.sh [jazzy|lyrical] build      colcon build
#   scripts/ros.sh [jazzy|lyrical] test       colcon build + test; fails on a failed test, and on no test at all
#   scripts/ros.sh [jazzy|lyrical] shell      a shell in the workspace
#   scripts/ros.sh [jazzy|lyrical] run ARGS   ros2 launch depz_sensors sensors.launch.py ARGS (with the USB boards)
#   scripts/ros.sh [jazzy|lyrical] exec 'CMD' CMD in the built workspace (with the USB boards)
#
# The workspace (build/, install/, log/) lives in ~/.cache/depz-ros/<distro>,
# out of the repository (DEPZ_ROS_WS to put it elsewhere). Files are written as
# the calling user.
# Our own code is built with warnings as errors (DEPZ_SENSORS_WERROR).
# DEPZ_ROS_MEMORY caps the container's memory (docker --memory, e.g. 3g) and
# DEPZ_ROS_JOBS sets the compile jobs (3 by default). DEPZ_ROS_IMAGE names another
# image to run in — one made from depz-ros:<distro> (with RViz 2 added, say),
# which is built first either way.
set -euo pipefail
distro=${1:-jazzy}; shift || true
cmd=${1:-test}; shift || true
here=$(cd "$(dirname "$0")/.." && pwd)
repo=$(cd "$here/../.." && pwd)
ws=${DEPZ_ROS_WS:-$HOME/.cache/depz-ros/$distro}
image=depz-ros:$distro

# No docker is a failure with its reason: ROS is only in the containers.
if ! command -v docker >/dev/null 2>&1; then
  echo "ros.sh: docker is not installed — depz_sensors is built and tested only in a ROS 2 container" >&2
  exit 1
fi
if ! docker info >/dev/null 2>&1; then
  echo "ros.sh: docker does not answer (\`docker info\` failed): is the daemon running, is $(id -un) in the docker group?" >&2
  exit 1
fi
mkdir -p "$ws"

docker build -q -t "$image" --build-arg ROS_DISTRO="$distro" "$here/docker" >/dev/null

devices=()
if [[ $cmd == run || $cmd == shell || $cmd == exec ]]; then
  # Every USB serial port (ttyACM, major 166) of the machine, also one that appears while the container
  # runs: a board plugged in later, or one that comes back under another number (hot_plug). `--device`
  # per port would give only the ports of this moment — the container's /dev is a copy made at its start.
  # The shared memory stays the container's own, as without the machine's /dev.
  devices+=(-v /dev:/dev --device-cgroup-rule 'c 166:* rmw' --tmpfs /dev/shm:rw,nosuid,nodev,mode=1777,size=64m)
  devices+=(--group-add "$(stat -c %g /dev/ttyACM0 2>/dev/null || getent group dialout | cut -d: -f3)")
  devices+=(-v /run/udev:/run/udev:ro -v /sys:/sys:ro)
fi
tty=(); [[ -t 0 ]] && tty=(-it)

# Only the two SDK packages and this one are visible to colcon.
inner='set -e; source /opt/ros/$ROS_DISTRO/setup.bash; cd /ws
build() { colcon build --base-paths /src/packages/depz-sensor-ros2 --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDEPZ_SENSORS_WERROR=ON \
  --parallel-workers 1 --event-handlers console_cohesion+ "$@"; }'
case $cmd in
  build) inner+='; build';;
  # `colcon test` is green whatever the tests say: the verdict is test-result's (rc 1 on a
  # failure or an error). Results of an earlier run go first, so a test that is gone cannot
  # speak; no result at all (the tests were not built, or did not start) is a failure too.
  test)  inner+='; build; colcon test-result --delete-yes >/dev/null 2>&1 || true
    colcon test --base-paths /src/packages/depz-sensor-ros2 --event-handlers console_cohesion+
    colcon test-result --all --verbose
    colcon test-result --all | grep -Eq "^Summary: [1-9][0-9]* tests" ||
      { echo "ros.sh: no test ran — a green result needs at least one" >&2; exit 1; }';;
  shell) inner+='; [ -f install/setup.bash ] && source install/setup.bash; exec bash';;
  run)   inner+='; source install/setup.bash; exec ros2 launch depz_sensors sensors.launch.py "$@"';;
  exec)  inner+='; source install/setup.bash; eval "$1"';;
  *) echo "unknown command: $cmd" >&2; exit 2;;
esac

exec docker run --rm --init "${tty[@]}" --net=host -u "$(id -u):$(id -g)" -e HOME=/tmp -e MAKEFLAGS="-j${DEPZ_ROS_JOBS:-3}" \
  ${DEPZ_ROS_MEMORY:+--memory "$DEPZ_ROS_MEMORY"} \
  -v "$repo:/src:ro" -v "$ws:/ws" "${devices[@]}" "${DEPZ_ROS_IMAGE:-$image}" bash -c "$inner" bash "$@"
