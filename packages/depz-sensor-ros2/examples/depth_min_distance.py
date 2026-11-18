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
The closest target a DEPZ multizone sensor (VL53L5/L7/L8, TMF8829) sees.

    ros2 run depz_sensors depth_min_distance.py --ros-args -p topic:=/depz/vl53l8ch_xxxx/depth

For each depth image: the zone with the smallest distance and where it is
in the image, e.g. `vl53l8ch_xxxx: closest 0.412 m at row 3, column 3`. The
image is the field as seen from behind the sensor (a DEPZ board held USB
connector down): row 0 is the top, column 0 the left. A zone without a valid
target is NaN and is skipped; with none at all: `no target in any zone`.

examples/depth_min_distance.cpp is the same program in C++: both print the
same lines for the same images.
"""

import math
import signal
import struct

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Image


def closest_zone(msg: Image):
    """(index, metres) of the closest valid zone of a 32FC1 image, or None."""
    values = struct.unpack(f'<{msg.width * msg.height}f', bytes(msg.data))
    best = None
    for i, v in enumerate(values):
        if not math.isnan(v) and (best is None or v < best[1]):
            best = (i, v)
    return best


class DepthMinDistance(Node):

    def __init__(self):
        super().__init__('depz_depth_min_distance')
        topic = self.declare_parameter('topic', '/depz/depth').value
        self.create_subscription(Image, topic, self.on_image, qos_profile_sensor_data)
        self.get_logger().info(f'listening on {topic}')

    def on_image(self, msg: Image):
        best = closest_zone(msg)
        if best is None:
            self.get_logger().info(f'{msg.header.frame_id}: no target in any zone')
            return
        pixel, metres = best
        row, column = divmod(pixel, msg.width)
        self.get_logger().info(
            f'{msg.header.frame_id}: closest {metres:.3f} m at row {row}, column {column}')


def main():
    rclpy.init()
    node = DepthMinDistance()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass  # Ctrl+C: which of the two comes depends on how the program was started
    finally:
        # A second Ctrl+C must not break the clean-up: in a terminal `ros2 launch`
        # passes on the one the program got from the keyboard already.
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()
