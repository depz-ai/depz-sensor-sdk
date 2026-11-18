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
Print roll / pitch / yaw from a DEPZ IMU (BNO055, BNO085, BNO086).

    ros2 run depz_sensors imu_listener.py --ros-args -p topic:=/depz/bno086_xxxx/imu
"""

import math
import signal

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Imu


def euler_deg(q):
    """Quaternion -> roll, pitch, yaw in degrees."""
    roll = math.atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y))
    pitch = math.asin(max(-1.0, min(1.0, 2 * (q.w * q.y - q.z * q.x))))
    yaw = math.atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
    return [math.degrees(a) for a in (roll, pitch, yaw)]


class ImuListener(Node):

    def __init__(self):
        super().__init__('depz_imu_listener')
        topic = self.declare_parameter('topic', '/depz/imu').value
        self.count = 0
        self.create_subscription(Imu, topic, self.on_imu, qos_profile_sensor_data)
        self.get_logger().info(f'listening on {topic}')

    def on_imu(self, msg: Imu):
        self.count += 1
        if self.count % 20 != 1:  # the first sample, then every 20th: readable at 100 Hz
            return
        roll, pitch, yaw = euler_deg(msg.orientation)
        a = msg.linear_acceleration
        self.get_logger().info(
            f'roll {roll:7.1f}°  pitch {pitch:7.1f}°  yaw {yaw:7.1f}°   '
            f'accel {a.x:6.2f} {a.y:6.2f} {a.z:6.2f} m/s²')


def main():
    rclpy.init()
    node = ImuListener()
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
