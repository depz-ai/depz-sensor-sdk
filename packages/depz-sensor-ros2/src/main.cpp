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


// The `sensors_node` executable: the depz_sensors::SensorsNode component in a
// process of its own. A main of our own rather than the one rclcpp_components
// generates: that one lets an exception of the node's constructor end the
// process by std::terminate, and a bad parameter must be one FATAL line and
// exit code 1.
#include <exception>

#include <rclcpp/rclcpp.hpp>

#include "sensors_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  int rc = 0;
  try {
    auto node = depz_sensors::make_sensors_node(rclcpp::NodeOptions());
    if (rclcpp::ok()) {rclcpp::spin(node);}    // Ctrl+C during start-up: straight to the clean-up
  } catch (const std::exception & e) {
    RCLCPP_FATAL(rclcpp::get_logger("depz_sensors"), "%s", e.what());
    rc = 1;
  }
  rclcpp::shutdown();
  return rc;
}
