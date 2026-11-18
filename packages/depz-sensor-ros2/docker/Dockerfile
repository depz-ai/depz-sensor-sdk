# Build / test image for depz_sensors: the ROS 2 base image plus what the
# package and its tests need. scripts/ros.sh builds and uses it.
ARG ROS_DISTRO=jazzy
FROM ros:${ROS_DISTRO}-ros-base
ARG ROS_DISTRO
RUN apt-get update && apt-get install -y --no-install-recommends \
      ros-${ROS_DISTRO}-sensor-msgs \
      ros-${ROS_DISTRO}-launch-ros \
      ros-${ROS_DISTRO}-launch-testing-ament-cmake \
      ros-${ROS_DISTRO}-ament-cmake-gtest \
      ros-${ROS_DISTRO}-ament-cmake-pytest \
      python3-pytest \
    && rm -rf /var/lib/apt/lists/*
