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
Print the zone grid of a DEPZ multizone sensor (VL53L5/L7/L8, TMF8829).

    ros2 run depz_sensors depth_listener.py --ros-args -p topic:=/depz/vl53l8ch_xxxx/depth

The image is 32FC1: one float per zone, metres; NaN = no valid target.
"""

import math
import signal
import struct

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Image


class DepthListener(Node):

    def __init__(self):
        super().__init__('depz_depth_listener')
        topic = self.declare_parameter('topic', '/depz/depth').value
        self.create_subscription(Image, topic, self.on_image, qos_profile_sensor_data)
        self.get_logger().info(f'listening on {topic}')

    def on_image(self, msg: Image):
        values = struct.unpack(f'<{msg.width * msg.height}f', bytes(msg.data))
        rows = []
        for r in range(msg.height):
            row = values[r * msg.width:(r + 1) * msg.width]
            rows.append(' '.join('  -- ' if math.isnan(v) else f'{v * 100:5.0f}' for v in row))
        self.get_logger().info(
            f'{msg.header.frame_id} ({msg.width}x{msg.height}, cm):\n' + '\n'.join(rows))


def main():
    rclpy.init()
    node = DepthListener()
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
