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
An example of your own launch file: the DEPZ sensors and the programs that use them.

All are started by one command:

    ros2 launch depz_sensors example_robot.launch.py config:=$HOME/my_sensors.yaml

It starts
- the sensors node with config/example_aliases.yaml (or `config`): three
  boards named by alias — `front` (a distance sensor), `depth` (a multizone
  sensor) and `imu`;
- depth_min_distance.py on depz/depth/depth: the closest target of `depth`;
- imu_listener.py on depz/imu/imu: the angles of `imu`;
- range_table.py: every distance sensor in a table (it finds them itself);
- where each board is mounted on the robot: a static transform from the
  robot's frame `base_link` to the board's frame (its name: `front`, `depth`,
  `imu`), so that RViz and every other program can place its measurements —
  `front` 10 cm ahead of the robot's centre and 5 cm up, looking forward;
  `depth` 20 cm up, looking to the left; `imu` at the centre, 2 cm up.
  Put in where your boards really are.

The topic names have no leading `/`: they are inside the launch's
`namespace`, as the node's topics are. Copy this file into your own package
and put your programs in place of the examples.
"""

import math
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share = get_package_share_directory('depz_sensors')
    namespace = LaunchConfiguration('namespace')

    def example(executable, **parameters):
        return Node(package='depz_sensors', executable=executable, namespace=namespace,
                    parameters=[parameters], output='screen', emulate_tty=True)

    def mounted(frame, x, y, z, yaw=0.0):
        """
        Mount the board's frame `frame` on base_link.

        x / y / z metres ahead of / to the left of / above base_link, turned by
        `yaw` radians to the left (0: it looks forward, along the robot's x).
        """
        return Node(package='tf2_ros', executable='static_transform_publisher',
                    name=f'{frame}_mount', namespace=namespace,
                    arguments=['--x', str(x), '--y', str(y), '--z', str(z), '--yaw', str(yaw),
                               '--frame-id', 'base_link', '--child-frame-id', frame])

    return LaunchDescription([
        DeclareLaunchArgument('config',
                              default_value=os.path.join(share, 'config', 'example_aliases.yaml'),
                              description="YAML file with the sensors node's parameters"),
        DeclareLaunchArgument('namespace', default_value='',
                              description='ROS namespace in front of every topic'),
        # The sensors node, exactly as `ros2 launch depz_sensors sensors.launch.py` starts it.
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(share, 'launch', 'sensors.launch.py')),
            launch_arguments={'config': LaunchConfiguration('config'),
                              'namespace': namespace}.items()),
        example('depth_min_distance.py', topic='depz/depth/depth'),
        example('imu_listener.py', topic='depz/imu/imu'),
        example('range_table.py'),
        mounted('front', 0.10, 0.0, 0.05),
        mounted('depth', 0.0, 0.0, 0.20, yaw=math.pi / 2),
        mounted('imu', 0.0, 0.0, 0.02),
    ])
