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


// DEPZ SDK samples -> standard ROS 2 messages. Pure functions of the sample and
// a header (stamp + frame id, made by the caller): no node, no device, so the
// unit tests build samples by hand.
//
// Conventions (REP 103 / REP 117 / sensor_msgs docs):
//   - metres, radians, m/s², rad/s, tesla;
//   - Range: +inf = nothing in range (no echo, or a distance beyond
//     max_range), NaN = the sensor rejected the measurement (a status other
//     than valid) or gave a distance below 0;
//   - Image (32FC1, metres): NaN in a zone with no valid target;
//   - PointCloud2 (x, y, z float32, metres): one point per zone, laid out as
//     the depth image; x forward out of the sensor, y left, z up (REP 103);
//     NaN, NaN, NaN in a zone with no valid target (is_dense false);
//   - Imu: covariance all zero = unknown; orientation_covariance[0] = -1 when
//     the message carries no orientation;
//   - Temperature: °C, variance 0 = unknown.
#pragma once

#include <optional>
#include <string>

#include <depz/device.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/range.hpp>
#include <sensor_msgs/msg/temperature.hpp>
#include <std_msgs/msg/header.hpp>

namespace depz_sensors
{

// The fixed fields of a single-zone sensor's Range messages.
struct RangeSpec
{
  std::uint8_t radiation_type;    // sensor_msgs::msg::Range::ULTRASOUND / INFRARED
  float field_of_view;            // rad, full cone angle
  float min_range, max_range;     // m
};

// Datasheet figures for a model ("hcsr04", "vl53l4cd", "vl53l0x", "vl53l1cx",
// "vl53l1cb", "vl53l3cx", "vl53l4cx"); nullopt for a model that is not a
// single-zone ranger.
std::optional<RangeSpec> range_spec(const std::string & model);

// `distance_m` up to the spec's max_range is the range, beyond it +inf.
// Without a distance: +inf for `no_target`, else NaN.
sensor_msgs::msg::Range make_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  std::optional<double> distance_m, bool no_target);

sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Sr04Measurement & m);
sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Vl53l4cdMeasurement & m);
sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Vl53lxMeasurement & m);

// VL53L5/L7/L8 zone status: valid as ST counts it (5, or 9 = valid with a
// large pulse) — the viewer's rule.
bool zone_valid(std::uint8_t target_status, std::uint8_t nb_target_detected);

// A multizone frame as a 4x4 / 8x8 32FC1 depth image: the field as seen from
// behind the sensor, the board USB connector down — row 0 the top, column 0 the
// left. On a DEPZ board that is the frame's zone grid turned a quarter turn
// clockwise (measured on the boards; convert.cpp's st_zone), as the viewer draws it.
sensor_msgs::msg::Image to_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f);
// The frame's signal per zone as a 32FC1 image, kcps/SPAD, laid out as the
// depth image. Every zone as the sensor reports it — also one the depth image
// marks NaN (the status image says why); NaN only where the frame holds no value.
sensor_msgs::msg::Image to_signal_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f);
// The frame's target status per zone as a mono8 image, laid out as the depth
// image: ST's code as it is (5 and 9 are valid, 255 is no target); 0 where the
// frame holds no value.
sensor_msgs::msg::Image to_status_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f);
// A TMF8829 measurement as a 32FC1 depth image of one of its peaks — `peak` 0
// is the first (closest); a zone without that peak (0 distance, or a
// measurement with fewer peaks) is NaN. The field as seen from behind the
// sensor, the board USB connector down: on a DEPZ board that is the
// measurement's rows x cols grid transposed, cols high and rows wide (measured
// on the boards; convert.cpp's tmf_zone), as the viewer draws it.
sensor_msgs::msg::Image to_image(
  const std_msgs::msg::Header & header, const depz::Tmf8829Measurement & m,
  unsigned peak = 0);

// The field of view of a VL53L5 / L7 / L8 across its zone grid: the full
// angles between the grid's edges, radians.
struct ZoneFov
{
  double horizontal, vertical;
};

// ST's datasheet figures for a model ("vl53l5cx", "vl53l7cx", "vl53l7ch",
// "vl53l8cx", "vl53l8ch"); nullopt for another model.
std::optional<ZoneFov> zone_fov(const std::string & model);

// A multizone frame as a point cloud: one point per zone, the cloud's
// height x width the zone grid and its points in the depth image's order, so
// point i is pixel i. The frame is the sensor: x out of it along its optical
// axis, y to its left, z up, for the board held USB connector down — pixel 0
// looks up and to the left. A zone the depth image marks NaN is the point NaN,
// NaN, NaN.
//
// The sensor reports each zone's distance to the plane through itself, at
// right angles to its optical axis — not along the zone's ray — so that
// distance is x, and y and z are x times the tangents of the zone centre's
// angles; the zones split the field of view evenly on a plane square to the axis.
sensor_msgs::msg::PointCloud2 to_point_cloud(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f, const ZoneFov & fov);
// A TMF8829 measurement's first peak as a point cloud, laid out as its depth
// image; a zone without a peak is NaN, NaN, NaN. The frame is the sensor: x
// out of it, y to its left, z up, for the board held USB connector down — on a
// DEPZ board the datasheet's pixel x (the column index) runs down the field and
// its pixel y (the row index) to the right. The sensor reports the distance
// along each zone's ray; ams OSRAM's correction factor per zone turns it into x.
sensor_msgs::msg::PointCloud2 to_point_cloud(
  const std_msgs::msg::Header & header,
  const depz::Tmf8829Measurement & m);

// A temperature in °C (the MCU's, a VL53L5/L7/L8's silicon, a TMF8829's die).
sensor_msgs::msg::Temperature to_temperature(const std_msgs::msg::Header & header, double celsius);

// BNO055 sample (units: m/s², rad/s, µT — what the node configures) -> Imu,
// and its magnetometer -> MagneticField (nullopt when the block lacks it).
// nullopt Imu when the block carries neither orientation nor rates.
std::optional<sensor_msgs::msg::Imu> to_imu(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s);
std::optional<sensor_msgs::msg::MagneticField> to_magnetic_field(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s);

// The chip's temperature (°C — what the node configures), its acceleration
// without gravity and its gravity vector (m/s², the fusion's): nullopt when
// the block lacks the channel.
std::optional<sensor_msgs::msg::Temperature> to_temperature(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s);
std::optional<geometry_msgs::msg::Vector3Stamped> to_linear_acceleration(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s);
std::optional<geometry_msgs::msg::Vector3Stamped> to_gravity(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s);

// The BNO085/086 sends each quantity as its own report. This keeps the last
// acceleration and angular rate and emits an Imu on each rotation-vector
// report (the orientation; the hub's fused output), a MagneticField on each
// magnetometer report, and a Vector3Stamped (m/s²) on each linear-acceleration
// and each gravity report.
class Bno086Assembler {
public:
  struct Out
  {
    std::optional<sensor_msgs::msg::Imu> imu;
    std::optional<sensor_msgs::msg::MagneticField> mag;
    std::optional<geometry_msgs::msg::Vector3Stamped> linear_acceleration, gravity;
  };
  Out feed(const std_msgs::msg::Header & header, const depz::Bno086Report & r);

private:
  std::optional<std::array<double, 3>> accel_, gyro_;
};

}  // namespace depz_sensors
