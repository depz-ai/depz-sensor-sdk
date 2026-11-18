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
Start the DEPZ sensors node: every DEPZ board on USB gets its own topics.

    ros2 launch depz_sensors sensors.launch.py
    ros2 launch depz_sensors sensors.launch.py config:=/path/to/my.yaml
    ros2 launch depz_sensors sensors.launch.py namespace:=robot1
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share = get_package_share_directory('depz_sensors')
    default_config = os.path.join(share, 'config', 'sensors.yaml')
    return LaunchDescription([
        DeclareLaunchArgument('config', default_value=default_config,
                              description="YAML file with the node's parameters"),
        DeclareLaunchArgument('namespace', default_value='',
                              description='ROS namespace in front of every topic'),
        Node(
            package='depz_sensors',
            executable='sensors_node',
            name='depz_sensors',
            namespace=LaunchConfiguration('namespace'),
            parameters=[LaunchConfiguration('config')],
            output='screen',
            emulate_tty=True,
        ),
    ])
