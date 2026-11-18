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
Start the DEPZ sensors node as a component, loaded into a component container.

The container is one process shared with other nodes, with intra-process
communication on, so a subscriber loaded into the same container gets each
message without the middleware in between.

    ros2 launch depz_sensors sensors_composed.launch.py
    ros2 launch depz_sensors sensors_composed.launch.py config:=/path/to/my.yaml namespace:=robot1

Into a container that already runs (yours, with the nodes that read the
sensors), by its full name — add these two arguments to the command:

    container:=/my_container start_container:=false

The node, its parameters and its topics are those of sensors.launch.py.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, LoadComposableNodes
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    share = get_package_share_directory('depz_sensors')
    default_config = os.path.join(share, 'config', 'sensors.yaml')
    return LaunchDescription([
        DeclareLaunchArgument('config', default_value=default_config,
                              description="YAML file with the node's parameters"),
        DeclareLaunchArgument('namespace', default_value='',
                              description='ROS namespace in front of every topic'),
        DeclareLaunchArgument('container', default_value='depz_container',
                              description="The container's name; with start_container:=false, "
                                          'the full name of the running container to load the '
                                          'node into'),
        DeclareLaunchArgument('start_container', default_value='true',
                              description='Start the container (false: it already runs)'),
        ComposableNodeContainer(
            condition=IfCondition(LaunchConfiguration('start_container')),
            package='rclcpp_components',
            executable='component_container',
            name=LaunchConfiguration('container'),
            namespace='',
            output='screen',
            emulate_tty=True,
        ),
        LoadComposableNodes(
            target_container=LaunchConfiguration('container'),
            composable_node_descriptions=[
                ComposableNode(
                    package='depz_sensors',
                    plugin='depz_sensors::SensorsNode',
                    name='depz_sensors',
                    namespace=LaunchConfiguration('namespace'),
                    parameters=[LaunchConfiguration('config')],
                    extra_arguments=[{'use_intra_process_comms': True}],
                ),
            ],
        ),
    ])
