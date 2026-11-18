#!/usr/bin/env python3
# Copyright 2026 DEPZ
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


"""
RViz 2 on every topic the sensors node publishes.

The program behind

    ros2 launch depz_sensors view.launch.py

The topics are named after the boards' serial numbers, so no ready RViz file
can list them. This program waits until the sensors node has started its
boards, asks ROS for the node's topics and writes an RViz file with one
display per topic, each a copy of the display of that kind in the package's
rviz/depz_sensors.rviz (the `config` parameter):

    sensor_msgs/Range        a cone                 (the Range display)
    sensor_msgs/Imu          a box that turns       (the Imu display, rviz_imu_plugin)
    sensor_msgs/Image        the zone grid, a panel (the Image display)
    sensor_msgs/PointCloud2  the zones in space     (the PointCloud2 display)

RViz draws a cone, a box or a cloud only where it knows the board to be. With
`place_boards` (the default) the program says so itself: a static transform
from `fixed_frame` (base_link) to each board's frame — the frame_id of its
messages — the boards side by side, `spacing_m` apart along y, each looking
along x. On a robot that publishes these transforms itself, set
`place_boards` to false.

Then it starts RViz (`rviz`: the command, rviz2) with that file and ends when
RViz is closed. Boards plugged in later are not in the view: start it again.
"""

import signal
import sys

# An interrupt while the modules below load (the launch ends at once: its sensors node cannot run
# with a parameter) is the end of the program, not a traceback. main() gives Ctrl+C back to Python.
signal.signal(signal.SIGINT, lambda *_: sys.exit(0))

import copy  # noqa: E402, I100
import os  # noqa: E402
import subprocess  # noqa: E402
import tempfile  # noqa: E402
import time  # noqa: E402

from ament_index_python.packages import (  # noqa: E402
    get_package_share_directory, PackageNotFoundError)
from geometry_msgs.msg import TransformStamped  # noqa: E402
from rcl_interfaces.srv import GetParameters  # noqa: E402
import rclpy  # noqa: E402
from rclpy.executors import ExternalShutdownException  # noqa: E402
from rclpy.node import Node  # noqa: E402
from rclpy.qos import qos_profile_sensor_data  # noqa: E402
from sensor_msgs.msg import Image, Imu, PointCloud2, Range  # noqa: E402
from tf2_ros import StaticTransformBroadcaster  # noqa: E402
import yaml  # noqa: E402

SENSORS_NODE = 'depz_sensors'
# message type -> (its Python class, the class of the RViz display that shows it)
DISPLAYS = {
    'sensor_msgs/msg/Range': (Range, 'rviz_default_plugins/Range'),
    'sensor_msgs/msg/Imu': (Imu, 'rviz_imu_plugin/Imu'),
    'sensor_msgs/msg/Image': (Image, 'rviz_default_plugins/Image'),
    'sensor_msgs/msg/PointCloud2': (PointCloud2, 'rviz_default_plugins/PointCloud2'),
}
IMU_DISPLAY = 'rviz_imu_plugin/Imu'
SETTLED_S = 1.0  # the node's topics are taken once the list has not changed for this long
FRAME_WAIT_S = 3.0  # how long a topic's first message (its frame_id) is waited for


def view_config(template, topics, fixed_frame):
    """
    Make the RViz configuration of the view.

    That is `template` (the parsed rviz/depz_sensors.rviz) with its topic
    displays replaced by one per topic of `topics` — [(topic, message type)],
    in that order — each a copy of the template's display of that message
    type, and `fixed_frame` as the Fixed Frame. A message type the template has
    no display for raises KeyError.
    """
    config = copy.deepcopy(template)
    manager = config['Visualization Manager']
    by_class = {d['Class']: d for d in manager['Displays']}
    classes = {display for _, display in DISPLAYS.values()}
    # the grid, the frames
    displays = [d for d in manager['Displays'] if d['Class'] not in classes]
    for topic, type_name in topics:
        display = copy.deepcopy(by_class[DISPLAYS[type_name][1]])
        display['Name'] = topic.strip('/')
        display['Topic']['Value'] = topic
        displays.append(display)
    manager['Displays'] = displays
    manager['Global Options']['Fixed Frame'] = fixed_frame
    for panel in config['Panels']:
        # which displays are unfolded: those of the template
        panel.pop('Property Tree Widget', None)
    # The template's image panel is gone; RViz docks the new ones itself.
    config['Window Geometry'] = {k: v for k, v in config['Window Geometry'].items()
                                 if k in ('Height', 'Width')}
    return config


def placements(frames, fixed_frame, spacing_m, stamp):
    """
    Make a static transform fixed_frame -> frame for each of `frames`.

    Side by side along y, `spacing_m` apart, centred, not rotated.
    """
    out = []
    for i, frame in enumerate(frames):
        t = TransformStamped()
        t.header.stamp = stamp
        t.header.frame_id = fixed_frame
        t.child_frame_id = frame
        t.transform.translation.y = (i - (len(frames) - 1) / 2) * spacing_m
        t.transform.rotation.w = 1.0
        out.append(t)
    return out


class View(Node):

    def __init__(self):
        super().__init__('depz_view')
        default = os.path.join(get_package_share_directory('depz_sensors'), 'rviz',
                               'depz_sensors.rviz')
        self.config = self.declare_parameter('config', default).value
        self.fixed_frame = self.declare_parameter('fixed_frame', 'base_link').value
        self.place_boards = self.declare_parameter('place_boards', True).value
        self.spacing_m = self.declare_parameter('spacing_m', 0.3).value
        self.rviz = self.declare_parameter('rviz', 'rviz2').value
        self.broadcaster = StaticTransformBroadcaster(self)

    def spin_until(self, done, timeout_s=None):
        """Spin until `done()` holds; False when `timeout_s` passed first."""
        deadline = None if timeout_s is None else time.monotonic() + timeout_s
        while not done():
            if deadline is not None and time.monotonic() >= deadline:
                return False
            rclpy.spin_once(self, timeout_sec=0.1)
        return True

    def sensors_hot_plug(self):
        """
        Wait for the sensors node and return its `hot_plug`.

        The node answers once its start-up is through: every board that was
        plugged in has its topics by then.
        """
        client = self.create_client(GetParameters, f'{SENSORS_NODE}/get_parameters')
        self.get_logger().info(f'waiting for the node {SENSORS_NODE} to start its boards')
        while True:
            self.spin_until(client.service_is_ready)
            future = client.call_async(GetParameters.Request(names=['hot_plug']))
            # A node that went before it answered (a parameter it cannot run with): wait for the
            # next one.
            self.spin_until(lambda: future.done() or not client.service_is_ready())
            if future.done():
                break
        self.destroy_client(client)
        return future.result().values[0].bool_value

    def sensor_topics(self):
        """Return [(topic, message type)] the node publishes and RViz can show."""
        found = []
        for name, types in self.get_topic_names_and_types():
            if any(part.startswith('_') for part in name.split('/')):
                continue  # ROS's hidden topics
            shown = [t for t in types if t in DISPLAYS]
            # ... published by the sensors node of this namespace
            if shown and any(p.node_name == SENSORS_NODE
                             and p.node_namespace == self.get_namespace()
                             for p in self.get_publishers_info_by_topic(name)):
                found.append((name, shown[0]))
        return sorted(found)

    def settled_topics(self, hot_plug):
        """
        Return the node's topics, once their list holds still.

        None: there is none and none will come (no hot_plug).
        """
        seen = {'topics': None, 'since': time.monotonic(), 'said': False}

        def settled():
            topics, now = self.sensor_topics(), time.monotonic()
            if topics != seen['topics']:
                seen['topics'], seen['since'] = topics, now
            if now - seen['since'] < SETTLED_S:
                return False
            if topics or not hot_plug:
                return True
            if not seen['said']:
                seen['said'] = True
                self.get_logger().warning(f'{SENSORS_NODE} publishes no topic yet — RViz starts '
                                          'when a board is plugged in')
            return False

        self.spin_until(settled)
        return seen['topics'] or None

    def frames_of(self, topics):
        """
        Return {topic: the frame_id of its messages}.

        A topic without a message in FRAME_WAIT_S is left out.
        """
        frames = {}
        subscriptions = [
            self.create_subscription(
                DISPLAYS[type_name][0], topic,
                lambda msg, topic=topic: frames.setdefault(topic, msg.header.frame_id),
                qos_profile_sensor_data)
            for topic, type_name in topics]
        self.spin_until(lambda: len(frames) == len(topics), FRAME_WAIT_S)
        for subscription in subscriptions:
            self.destroy_subscription(subscription)
        return frames

    def warn_of_missing_imu_plugin(self, topics):
        """Warn of a missing Imu display: without its package RViz lists it as failed."""
        if not any(DISPLAYS[type_name][1] == IMU_DISPLAY for _, type_name in topics):
            return
        try:
            get_package_share_directory('rviz_imu_plugin')
        except PackageNotFoundError:
            distro = os.environ.get('ROS_DISTRO', '<distro>')
            self.get_logger().warning('the Imu display of RViz is not installed — the imu topics '
                                      'are not drawn: sudo apt install '
                                      f'ros-{distro}-rviz-imu-plugin')

    def run(self):
        """Run the view; return the program's exit code."""
        topics = self.settled_topics(self.sensors_hot_plug())
        if topics is None:
            self.get_logger().error(f'{SENSORS_NODE} publishes no topic: no board was started — '
                                    'see its lines above and "Troubleshooting" in the '
                                    'depz_sensors README')
            return 1
        frames = self.frames_of(topics)
        for topic, _ in topics:
            if topic not in frames:
                self.get_logger().warning(f'{topic}: no message within {FRAME_WAIT_S:.0f} s — '
                                          'its frame is not known')
        if self.place_boards:
            names = sorted(set(frames.values()))
            self.broadcaster.sendTransform(
                placements(names, self.fixed_frame, self.spacing_m,
                           self.get_clock().now().to_msg()))
            self.get_logger().info(f'placed {len(names)} frames {self.spacing_m:g} m apart in '
                                   f'{self.fixed_frame}: ' + ' '.join(names))

        with open(self.config, encoding='utf-8') as f:
            template = yaml.safe_load(f)
        self.warn_of_missing_imu_plugin(topics)
        handle, path = tempfile.mkstemp(prefix='depz_view_', suffix='.rviz')
        with os.fdopen(handle, 'w', encoding='utf-8') as f:
            yaml.safe_dump(view_config(template, topics, self.fixed_frame), f, sort_keys=False)
        self.get_logger().info(f'{self.rviz} -d {path}: {len(topics)} topics — '
                               + ' '.join(t for t, _ in topics))
        try:
            rviz = subprocess.Popen([self.rviz, '-d', path])
        except FileNotFoundError:
            os.unlink(path)
            distro = os.environ.get('ROS_DISTRO', '<distro>')
            self.get_logger().error(f'{self.rviz}: no such program — RViz 2 is not installed: '
                                    f'sudo apt install ros-{distro}-rviz2')
            return 1
        try:
            # the static transforms stay while RViz runs
            self.spin_until(lambda: rviz.poll() is not None)
        finally:
            # Ctrl+C: RViz got it from the terminal too, or gets it here.
            signal.signal(signal.SIGINT, signal.SIG_IGN)
            if rviz.poll() is None:
                rviz.send_signal(signal.SIGINT)
                try:
                    rviz.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    rviz.kill()
                    rviz.wait()
            os.unlink(path)
        return 0


def main():
    code = 0
    node = None
    try:
        signal.signal(signal.SIGINT, signal.default_int_handler)
        rclpy.init()
        node = View()
        code = node.run()
    except (KeyboardInterrupt, ExternalShutdownException):
        pass  # Ctrl+C: which of the two comes depends on how the program was started
    finally:
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        if node is not None:
            node.destroy_node()
        rclpy.try_shutdown()
    sys.exit(code)


if __name__ == '__main__':
    main()
