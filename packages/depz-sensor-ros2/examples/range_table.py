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
Every distance sensor at once, as a table: all Range topics, found while running.

    ros2 run depz_sensors range_table.py

Nothing to configure: every `period_s` seconds (default 1) the program asks
ROS which topics carry sensor_msgs/Range — whatever their names, aliases and
namespaces — subscribes to the new ones, forgets those nobody publishes any
more (a board unplugged), and prints a table:

    topic                                        distance  rate
    /depz/vl53l1cb_te4xkbkbea/range               95.7 cm  20 /s
    /depz/vl53l3cx_tjuyj4nxsv/range  no valid measurement  20 /s
    /depz/vl53l4cx_tovjaln523/range              101.2 cm  20 /s

`rate` is the messages that came in since the previous table, per second.
"""

import math
import signal

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Range

RANGE_TYPE = 'sensor_msgs/msg/Range'


def distance_text(r: float) -> str:
    if math.isinf(r):
        return 'nothing in range'  # +inf: no echo, no target
    if math.isnan(r):
        return 'no valid measurement'  # NaN: the sensor rejected this one
    return f'{r * 100:.1f} cm'


class RangeTable(Node):

    def __init__(self):
        super().__init__('depz_range_table')
        self.period_s = self.declare_parameter('period_s', 1.0).value
        # topic -> {"subscription", "last" (Range or None), "count" since the last table}
        self.topics = {}
        self.create_timer(self.period_s, self.tick)
        self.get_logger().info('looking for sensor_msgs/Range topics')

    def tick(self):
        self.follow_the_graph()
        self.print_table()

    def follow_the_graph(self):
        """Subscribe to new Range topics, drop the ones without a publisher."""
        # Our own subscription keeps a topic in the graph after its board went:
        # a topic counts while somebody publishes it.
        found = {name for name, types in self.get_topic_names_and_types()
                 if RANGE_TYPE in types and self.count_publishers(name) > 0}
        for name in sorted(found - self.topics.keys()):
            entry = {'last': None, 'count': 0}
            entry['subscription'] = self.create_subscription(
                Range, name, lambda msg, e=entry: self.on_range(e, msg), qos_profile_sensor_data)
            self.topics[name] = entry
            self.get_logger().info(f'new topic {name}')
        for name in sorted(self.topics.keys() - found):
            self.destroy_subscription(self.topics.pop(name)['subscription'])
            self.get_logger().info(f'topic gone {name}')

    @staticmethod
    def on_range(entry, msg: Range):
        entry['last'] = msg
        entry['count'] += 1

    def print_table(self):
        if not self.topics:
            self.get_logger().info('no Range topic yet')
            return
        width = max(len(name) for name in self.topics)
        lines = [f"{'topic':<{width}}  {'distance':>20}  rate"]
        for name, entry in sorted(self.topics.items()):
            text = 'waiting' if entry['last'] is None else distance_text(entry['last'].range)
            lines.append(f"{name:<{width}}  {text:>20}  {entry['count'] / self.period_s:.0f} /s")
            entry['count'] = 0
        self.get_logger().info('\n' + '\n'.join(lines))


def main():
    rclpy.init()
    node = RangeTable()
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
