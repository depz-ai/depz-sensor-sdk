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
Start the DEPZ sensors node and RViz 2 on everything it publishes.

A cone per distance sensor, a turning box per IMU, a zone grid per multizone
sensor (and its point cloud with `point_cloud: true`), whatever the boards are
called.

    ros2 launch depz_sensors view.launch.py
    ros2 launch depz_sensors view.launch.py config:=/path/to/my.yaml namespace:=robot1

RViz starts when the node has started its boards; closing its window (or
Ctrl+C) ends the node too, and a node that ends — a parameter it cannot run
with, at once — ends the launch, RViz with it. The boards are placed side by
side in the frame `fixed_frame` (base_link) by static transforms — on a robot
that publishes the boards' transforms itself:

    ros2 launch depz_sensors view.launch.py place_boards:=false

The view is written by scripts/rviz_view.py from rviz/depz_sensors.rviz; it
needs RViz 2 (ros-<distro>-rviz2, part of ros-<distro>-desktop) and, for the
IMUs, ros-<distro>-rviz-imu-plugin.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.actions import RegisterEventHandler, Shutdown
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def is_sensors_node(action):
    return isinstance(action, Node) and action.node_executable == 'sensors_node'


def end_with_the_node(event, context):
    # During a shutdown the node ends because it was told to: nothing to add.
    return None if context.is_shutdown else [Shutdown(reason='the sensors node ended')]


def generate_launch_description():
    share = get_package_share_directory('depz_sensors')
    namespace = LaunchConfiguration('namespace')
    return LaunchDescription([
        DeclareLaunchArgument('config',
                              default_value=os.path.join(share, 'config', 'sensors.yaml'),
                              description="YAML file with the sensors node's parameters"),
        DeclareLaunchArgument('namespace', default_value='',
                              description='ROS namespace in front of every topic'),
        DeclareLaunchArgument('fixed_frame', default_value='base_link',
                              description='The frame RViz draws in (its Fixed Frame)'),
        DeclareLaunchArgument('place_boards', default_value='true',
                              description='Publish a static transform fixed_frame -> each '
                                          "board's frame, the boards side by side (false: your "
                                          'robot publishes them)'),
        DeclareLaunchArgument('rviz', default_value='rviz2',
                              description='The command that starts RViz 2'),
        # The sensors node, exactly as `ros2 launch depz_sensors sensors.launch.py` starts it.
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(share, 'launch', 'sensors.launch.py')),
            launch_arguments={'config': LaunchConfiguration('config'),
                              'namespace': namespace}.items()),
        # Without its node the view waits for nothing (a bad parameter: the node's FATAL line,
        # exit code 1).
        RegisterEventHandler(OnProcessExit(target_action=is_sensors_node,
                                           on_exit=end_with_the_node)),
        Node(
            package='depz_sensors',
            executable='rviz_view.py',
            namespace=namespace,
            parameters=[{
                'fixed_frame': LaunchConfiguration('fixed_frame'),
                'place_boards': ParameterValue(LaunchConfiguration('place_boards'),
                                               value_type=bool),
                'rviz': LaunchConfiguration('rviz'),
            }],
            output='screen',
            emulate_tty=True,
            on_exit=Shutdown(reason='RViz was closed'),
        ),
    ])
