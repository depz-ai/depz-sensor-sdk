// Copyright 2026 DEPZ
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


// The closest target a DEPZ multizone sensor (VL53L5/L7/L8, TMF8829) sees —
// examples/depth_min_distance.py in C++: both print the same lines for the
// same images.
//
//     ros2 run depz_sensors depth_min_distance --ros-args -p topic:=/depz/vl53l8ch_xxxx/depth
//
// The messages are standard sensor_msgs: nothing of DEPZ is needed to read
// them. The image is 32FC1 — one float per zone, metres, NaN for a zone
// without a valid target — and is the field as seen from behind the sensor
// (a DEPZ board held USB connector down): row 0 the top, column 0 the left.

#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

class DepthMinDistance : public rclcpp::Node {
public:
  DepthMinDistance()
  : Node("depz_depth_min_distance")
  {
    const auto topic = declare_parameter<std::string>("topic", "/depz/depth");
    // SensorDataQoS: the QoS the node publishes with (best effort, a short queue).
    sub_ = create_subscription<sensor_msgs::msg::Image>(
            topic, rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::Image & msg) {on_image(msg);});
    RCLCPP_INFO(get_logger(), "listening on %s", topic.c_str());
  }

private:
  void on_image(const sensor_msgs::msg::Image & msg)
  {
    const std::size_t zones = std::size_t{msg.width} *msg.height;
    if (msg.encoding != "32FC1" || msg.data.size() < zones * sizeof(float)) {
      RCLCPP_WARN(get_logger(), "%s: not a 32FC1 depth image", msg.header.frame_id.c_str());
      return;
    }
    std::size_t best = zones;      // none yet
    float best_m = 0.0f;
    for (std::size_t i = 0; i < zones; ++i) {
      float v;        // the bytes may be unaligned for a float: copy them out
      std::memcpy(&v, msg.data.data() + i * sizeof(float), sizeof v);
      if (!std::isnan(v) && (best == zones || v < best_m)) {
        best = i;
        best_m = v;
      }
    }
    if (best == zones) {
      RCLCPP_INFO(get_logger(), "%s: no target in any zone", msg.header.frame_id.c_str());
      return;
    }
    RCLCPP_INFO(get_logger(), "%s: closest %.3f m at row %zu, column %zu",
                    msg.header.frame_id.c_str(), static_cast<double>(best_m),
      best / msg.width, best % msg.width);
  }

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DepthMinDistance>());
  rclcpp::shutdown();
  return 0;
}
