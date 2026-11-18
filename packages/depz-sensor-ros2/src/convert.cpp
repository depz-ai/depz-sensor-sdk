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


#include "depz_sensors/convert.hpp"

#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <vector>

namespace depz_sensors
{

namespace
{

constexpr double kDeg = M_PI / 180.0;
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();

// An image of one channel of `bytes` bytes per zone, zeroed.
sensor_msgs::msg::Image zone_image(
  const std_msgs::msg::Header & header, unsigned rows, unsigned cols,
  const char * encoding, unsigned bytes)
{
  sensor_msgs::msg::Image img;
  img.header = header;
  img.height = rows;
  img.width = cols;
  img.encoding = encoding;
  img.is_bigendian = 0;
  img.step = cols * bytes;
  img.data.resize(static_cast<std::size_t>(rows) * img.step);
  return img;
}

sensor_msgs::msg::Image depth_image(
  const std_msgs::msg::Header & header, unsigned rows,
  unsigned cols)
{
  return zone_image(header, rows, cols, "32FC1", sizeof(float));
}

void put(sensor_msgs::msg::Image & img, std::size_t i, float v)
{
  std::memcpy(img.data.data() + i * sizeof(float), &v, sizeof(float));
}

// Imu with everything marked "not present": orientation_covariance[0] = -1
// until an orientation is set; rates and acceleration likewise.
sensor_msgs::msg::Imu blank_imu(const std_msgs::msg::Header & header)
{
  sensor_msgs::msg::Imu imu;
  imu.header = header;
  imu.orientation.w = 1.0;
  imu.orientation_covariance[0] = -1.0;
  imu.angular_velocity_covariance[0] = -1.0;
  imu.linear_acceleration_covariance[0] = -1.0;
  return imu;
}

void set_vec(geometry_msgs::msg::Vector3 & v, const std::array<double, 3> & a)
{
  v.x = a[0];
  v.y = a[1];
  v.z = a[2];
}

geometry_msgs::msg::Vector3Stamped stamped(
  const std_msgs::msg::Header & header,
  const std::array<double, 3> & a)
{
  geometry_msgs::msg::Vector3Stamped v;
  v.header = header;
  set_vec(v.vector, a);
  return v;
}

// A cloud of rows x cols points x, y, z (float32), every point NaN.
sensor_msgs::msg::PointCloud2 zone_cloud(
  const std_msgs::msg::Header & header, unsigned rows,
  unsigned cols)
{
  using sensor_msgs::msg::PointField;
  sensor_msgs::msg::PointCloud2 cloud;
  cloud.header = header;
  cloud.height = rows;
  cloud.width = cols;
  const char * const names[] = {"x", "y", "z"};
  for (unsigned i = 0; i < 3; ++i) {
    PointField field;
    field.name = names[i];
    field.offset = i * sizeof(float);
    field.datatype = PointField::FLOAT32;
    field.count = 1;
    cloud.fields.push_back(field);
  }
  cloud.is_bigendian = false;
  cloud.point_step = 3 * sizeof(float);
  cloud.row_step = cloud.point_step * cols;
  cloud.is_dense = false;    // a zone without a target is a NaN point
  const std::vector<float> blank(static_cast<std::size_t>(rows) * cols * 3, kNaN);
  cloud.data.resize(blank.size() * sizeof(float));
  std::memcpy(cloud.data.data(), blank.data(), cloud.data.size());
  return cloud;
}

void put_point(sensor_msgs::msg::PointCloud2 & cloud, std::size_t i, double x, double y, double z)
{
  const float xyz[3] = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
  std::memcpy(cloud.data.data() + i * cloud.point_step, xyz, sizeof(xyz));
}

// The side of a multizone frame's grid.
unsigned side_of(const depz::vl53l8::Vl53l8Frame & f) {return f.resolution == 16 ? 4 : 8;}

// Where a zone looks, on a DEPZ board: the images and clouds are laid out as a person sees the
// field from behind the sensor, the board's USB connector down — row 0 the top of the field,
// column 0 its left. No datasheet can say this: it is how the chip sits on the board. Measured on
// the bench on 2026-11-16 (every board connector down, a hand 5-10 cm in front of one half of the
// field, the zones it covered read off the frame as the SDK gives it):
//   - VL53L5CX, VL53L7CH, VL53L8CH alike: the top half of the field is the frame's first columns,
//     its left half the frame's last rows — the frame is the field turned a quarter turn, and a
//     clockwise quarter turn puts it back (depz-sensor-examples' example 4 measured the same
//     turn on its own, with a doorway; its Android example uses it).
//   - TMF8829: the top half is the frame's first columns, the left half its first rows — the
//     frame is the field mirrored about its diagonal, rows and columns swapped.

// The zone of a VL53L5 / L7 / L8 frame (an index into its arrays) seen at row `row`, column
// `col` of the side x side image.
std::size_t st_zone(unsigned side, unsigned row, unsigned col)
{
  return static_cast<std::size_t>(side - 1 - col) * side + row;
}

// The zone of a TMF8829 measurement of `rows` x `cols` (its arrays' grid) seen at row `row`,
// column `col` of the image — which is cols high and rows wide.
std::size_t tmf_zone(unsigned cols, unsigned row, unsigned col)
{
  return static_cast<std::size_t>(col) * cols + row;
}

}  // namespace

std::optional<RangeSpec> range_spec(const std::string & model)
{
  using sensor_msgs::msg::Range;
  // Full cone angle, min / max range: the boards' datasheets.
  static const std::map<std::string, RangeSpec> specs = {
    {"hcsr04", {Range::ULTRASOUND, static_cast<float>(15 * kDeg), 0.02F, 4.0F}},
    {"vl53l4cd", {Range::INFRARED, static_cast<float>(18 * kDeg), 0.001F, 1.3F}},
    {"vl53l0x", {Range::INFRARED, static_cast<float>(25 * kDeg), 0.03F, 2.0F}},
    {"vl53l1cx", {Range::INFRARED, static_cast<float>(27 * kDeg), 0.04F, 4.0F}},
    {"vl53l1cb", {Range::INFRARED, static_cast<float>(27 * kDeg), 0.04F, 4.0F}},
    {"vl53l3cx", {Range::INFRARED, static_cast<float>(25 * kDeg), 0.025F, 3.0F}},
    {"vl53l4cx", {Range::INFRARED, static_cast<float>(18 * kDeg), 0.001F, 6.0F}},
  };
  const auto it = specs.find(model);
  if (it == specs.end()) {return std::nullopt;}
  return it->second;
}

sensor_msgs::msg::Range make_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  std::optional<double> distance_m, bool no_target)
{
  sensor_msgs::msg::Range r;
  r.header = header;
  r.radiation_type = spec.radiation_type;
  r.field_of_view = spec.field_of_view;
  r.min_range = spec.min_range;
  r.max_range = spec.max_range;
  if (distance_m) {
    // REP 117: a distance beyond max_range is nothing in range (an HC-SR04 on the bench of
    // 2026-11-12: echoes of up to 9.9 m, with max_range 4 m).
    r.range = static_cast<float>(*distance_m);
    if (r.range > spec.max_range) {r.range = std::numeric_limits<float>::infinity();}
  } else {
    r.range = no_target ? std::numeric_limits<float>::infinity() : kNaN;
  }
  return r;
}

sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Sr04Measurement & m)
{
  const auto mm = m.distance_mm();
  // No echo within the window: nothing in range.
  return make_range(header, spec, mm ? std::optional<double>(*mm / 1000.0) : std::nullopt,
      !m.valid());
}

sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Vl53l4cdMeasurement & m)
{
  return make_range(header, spec,
                      m.valid() ? std::optional<double>(m.r.distance_mm / 1000.0) : std::nullopt,
      false);
}

sensor_msgs::msg::Range to_range(
  const std_msgs::msg::Header & header, const RangeSpec & spec,
  const depz::Vl53lxMeasurement & m)
{
  // A valid status with a distance below 0 is no distance (a VL53L4CX on the bench of 2026-11-12:
  // -59 mm, one measurement in 200 of a wall 1 m away): NaN, as a measurement the sensor rejected.
  const auto mm = m.primary_distance_mm();
  return make_range(
    header, spec, mm && *mm >= 0 ? std::optional<double>(*mm / 1000.0) : std::nullopt, false);
}

bool zone_valid(std::uint8_t target_status, std::uint8_t nb_target_detected)
{
  return nb_target_detected > 0 && (target_status == 5 || target_status == 9);
}

sensor_msgs::msg::Image to_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f)
{
  const unsigned side = side_of(f);
  auto img = depth_image(header, side, side);
  for (unsigned row = 0; row < side; ++row) {
    for (unsigned col = 0; col < side; ++col) {
      const std::size_t z = st_zone(side, row, col);
      const bool have = z < f.distance_mm.size() && z < f.target_status.size() &&
        z < f.nb_target_detected.size();
      const bool ok = have && zone_valid(f.target_status[z], f.nb_target_detected[z]);
      put(img, row * side + col, ok ? static_cast<float>(f.distance_mm[z] / 1000.0) : kNaN);
    }
  }
  return img;
}

sensor_msgs::msg::Image to_signal_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f)
{
  const unsigned side = side_of(f);
  auto img = depth_image(header, side, side);
  for (unsigned row = 0; row < side; ++row) {
    for (unsigned col = 0; col < side; ++col) {
      const std::size_t z = st_zone(side, row, col);
      put(img, row * side + col,
        z < f.signal_per_spad.size() ? static_cast<float>(f.signal_per_spad[z]) : kNaN);
    }
  }
  return img;
}

sensor_msgs::msg::Image to_status_image(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f)
{
  const unsigned side = side_of(f);
  auto img = zone_image(header, side, side, "mono8", 1);
  for (unsigned row = 0; row < side; ++row) {
    for (unsigned col = 0; col < side; ++col) {
      const std::size_t z = st_zone(side, row, col);
      if (z < f.target_status.size()) {img.data[row * side + col] = f.target_status[z];}
    }
  }
  return img;
}

sensor_msgs::msg::Image to_image(
  const std_msgs::msg::Header & header, const depz::Tmf8829Measurement & m,
  unsigned peak)
{
  const auto [rows, cols] = m.results.shape();    // the arrays' grid (merged halves)
  auto img = depth_image(header, cols, rows);      // rows and columns swapped (st_zone's note)
  const std::size_t n = static_cast<std::size_t>(rows) * cols;
  // distance_mm is [peaks][rows][cols]: n values per peak, the first peak first.
  const std::size_t first = static_cast<std::size_t>(peak) * n;
  for (unsigned row = 0; row < cols; ++row) {
    for (unsigned col = 0; col < rows; ++col) {
      const std::size_t z = first + tmf_zone(cols, row, col);
      const double mm = z < m.distance_mm.size() ? m.distance_mm[z] : 0.0;
      put(img, row * rows + col, mm > 0.0 ? static_cast<float>(mm / 1000.0) : kNaN);
    }
  }
  return img;
}

std::optional<ZoneFov> zone_fov(const std::string & model)
{
    // "Detection volume", horizontal x vertical, of each product's datasheet (st.com), section 2.2
    // "Field of view", Table 2 "FoV angles", page 4:
  static const std::map<std::string, ZoneFov> fovs = {
    {"vl53l5cx", {45 * kDeg, 45 * kDeg}},      // DS13754 Rev 13
    {"vl53l7cx", {60 * kDeg, 60 * kDeg}},      // DS13865 Rev 8
    {"vl53l7ch", {60 * kDeg, 60 * kDeg}},      // DS14309 Rev 3
    {"vl53l8cx", {45 * kDeg, 45 * kDeg}},      // DS14161 Rev 7
    {"vl53l8ch", {45 * kDeg, 45 * kDeg}},      // DS14310 Rev 3
  };
  const auto it = fovs.find(model);
  if (it == fovs.end()) {return std::nullopt;}
  return it->second;
}

sensor_msgs::msg::PointCloud2 to_point_cloud(
  const std_msgs::msg::Header & header,
  const depz::vl53l8::Vl53l8Frame & f, const ZoneFov & fov)
{
  // The geometry, from ST's documents:
  //   - The zones: a square grid, numbered row by row, "the physical view is from the device top
  //     into the lens" (DS14310 Rev 3 section 8.1 "Zone mapping", figures 24 and 25, page 23; the
  //     other products' datasheets alike). In front of a target square to the sensor "each zone is
  //     a square", and the edge of the field of view is tan(HFoV / 2) of the distance away from the
  //     axis (AN5894 Rev 2 section 2.3, equation 1, page 5): so on that plane the zones are side x
  //     side equal squares, and a zone's centre is its place in the grid times tan(FoV / 2).
  //   - Which way a zone looks: on a DEPZ board, as measured (st_zone's note) — not as the
  //     datasheet's figures of the bare module have it, which know nothing of how the chip sits on
  //     the board. The cloud is laid out as the depth image, point i its pixel i.
  //   - What the distance is: no datasheet says. ST's answer is that the sensor turns each zone's
  //     distance along its ray into the distance to the plane through the sensor ("if you point the
  //     sensor at a wall exactly perpendicular and at 1 meter, all the zones will report that 1
  //     meter", ST employee, community.st.com, 2022-02-17, "For VL53L5CX: what does
  //     ResultsData->distance_mm[ZoneNum] exactly mean?"), and so it is on the bench of 2026-11-13:
  //     the zones of a VL53L8CH 1 m from a wall lie on a plane within 4 mm RMS taken that way,
  //     27 mm taken along the rays.
  const unsigned side = side_of(f);
  auto cloud = zone_cloud(header, side, side);
  const double half = side / 2.0, centre = (side - 1) / 2.0;
  const double tan_h = std::tan(fov.horizontal / 2), tan_v = std::tan(fov.vertical / 2);
  for (unsigned row = 0; row < side; ++row) {      // the image's: row 0 the top, column 0 the left
    for (unsigned col = 0; col < side; ++col) {
      const std::size_t z = st_zone(side, row, col);
      const bool have =
        z < f.distance_mm.size() && z < f.target_status.size() && z < f.nb_target_detected.size();
      if (!have || !zone_valid(f.target_status[z], f.nb_target_detected[z])) {continue;}
      const double x = f.distance_mm[z] / 1000.0;
      put_point(cloud, static_cast<std::size_t>(row) * side + col,
        x, x * (centre - col) / half * tan_h, x * (centre - row) / half * tan_v);
    }
  }
  return cloud;
}

sensor_msgs::msg::PointCloud2 to_point_cloud(
  const std_msgs::msg::Header & header,
  const depz::Tmf8829Measurement & m)
{
  // The geometry, from ams OSRAM's documents:
  //   - AN001096 v3-00 "TMF8829 Host driver communication", section 4.8 "3D point cloud
  //     calculation", pages 43-44, zCorrectionFactorSimple(): a pixel of the X x Y grid (8x8,
  //     16x16, 32x32, 48x32) is at x = (pixel_x - centre) / (X * 3 / 4), y = (pixel_y - centre) / Y
  //     on the plane at distance 1, and the factor sqrt(1 + x² + y²) is its ray's length there
  //     — the reported distance over the factor is the distance along the optical axis. (The grid's
  //     edges are then 2 * atan(2 / 3) = 67.4° and 2 * atan(1 / 2) = 53.1° apart; the datasheet
  //     DS001140 v2-00, "Optical parameters", page 15, has FOVX 67.9° and FOVY 52.8°.)
  //   - The centre is the middle of the grid, (X - 1) / 2.0. The note writes
  //     `pixel_x - ((X-1)/2) - 0.5`: with the integer division its Python expects (it takes care to
  //     write `X * 3.0 / 4.0`), (X - 1) / 2 is 3 at X = 8 and the centre 3.5 — the middle. Read
  //     with a true division the centre would be 4.0, half a pixel off. The bench of 2026-11-13
  //     agrees with the middle: a wall is the same plane at 8x8, 16x16 and 48x32 — tilted +4.3°,
  //     +4.4°, +4.4° and -13.8°, -13.9°, -14.0° about two axes (bench_points.py's figures) — where
  //     the half pixel gives tilts of 7.9°, 6.1°, 5.1° and 16.8°, 15.4°, 14.8° (magnitudes), moving
  //     with the pixel size.
  //   - Which way a pixel looks: on a DEPZ board, as measured (st_zone's note) — the grid's pixel x
  //     (its column, the 67.9° axis) runs down the field from its top, its pixel y (its row) across
  //     it from its left. The datasheet's figure 16 ("pixel (0,0) is upper left corner", DS001140
  //     v2-00 page 35) is of the bare device. The cloud is laid out as the depth image, cols high
  //     and rows wide, point i its pixel i.
  const auto [rows, cols] = m.results.shape();
  auto cloud = zone_cloud(header, cols, rows);
  const double span_x = cols * 3.0 / 4.0, span_y = rows;
  for (unsigned row = 0; row < rows; ++row) {      // the arrays' grid
    for (unsigned col = 0; col < cols; ++col) {
      const std::size_t i = static_cast<std::size_t>(row) * cols + col;
      const double mm = i < m.distance_mm.size() ? m.distance_mm[i] : 0.0;        // the first peak
      if (!(mm > 0.0)) {continue;}
      const double px = (col - (cols - 1) / 2.0) / span_x, py = (row - (rows - 1) / 2.0) / span_y;
      const double x = mm / 1000.0 / std::sqrt(1 + px * px + py * py);
      // Pixel x grows down the field (-z), pixel y to its right (-y); the image is transposed.
      put_point(cloud, static_cast<std::size_t>(col) * rows + row, x, -x * py, -x * px);
    }
  }
  return cloud;
}

sensor_msgs::msg::Temperature to_temperature(const std_msgs::msg::Header & header, double celsius)
{
  sensor_msgs::msg::Temperature t;
  t.header = header;
  t.temperature = celsius;
  t.variance = 0.0;    // unknown
  return t;
}

std::optional<sensor_msgs::msg::Imu> to_imu(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s)
{
  if (!s.quaternion && !s.gyro && !s.accel) {return std::nullopt;}
  auto imu = blank_imu(header);
  if (s.quaternion) {
    const auto & q = *s.quaternion;     // w, x, y, z
    imu.orientation.w = q[0];
    imu.orientation.x = q[1];
    imu.orientation.y = q[2];
    imu.orientation.z = q[3];
    imu.orientation_covariance[0] = 0.0;
  }
  if (s.gyro) {
    set_vec(imu.angular_velocity, *s.gyro);
    imu.angular_velocity_covariance[0] = 0.0;
  }
  if (s.accel) {
    set_vec(imu.linear_acceleration, *s.accel);
    imu.linear_acceleration_covariance[0] = 0.0;
  }
  return imu;
}

std::optional<sensor_msgs::msg::MagneticField> to_magnetic_field(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s)
{
  if (!s.mag) {return std::nullopt;}
  sensor_msgs::msg::MagneticField m;
  m.header = header;
  m.magnetic_field.x = (*s.mag)[0] * 1e-6;    // µT -> T
  m.magnetic_field.y = (*s.mag)[1] * 1e-6;
  m.magnetic_field.z = (*s.mag)[2] * 1e-6;
  return m;
}

std::optional<sensor_msgs::msg::Temperature> to_temperature(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s)
{
  if (!s.temperature) {return std::nullopt;}
  return to_temperature(header, *s.temperature);
}

std::optional<geometry_msgs::msg::Vector3Stamped> to_linear_acceleration(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s)
{
  if (!s.linear_accel) {return std::nullopt;}
  return stamped(header, *s.linear_accel);
}

std::optional<geometry_msgs::msg::Vector3Stamped> to_gravity(
  const std_msgs::msg::Header & header,
  const depz::Bno055Sample & s)
{
  if (!s.gravity) {return std::nullopt;}
  return stamped(header, *s.gravity);
}

Bno086Assembler::Out Bno086Assembler::feed(
  const std_msgs::msg::Header & header,
  const depz::Bno086Report & r)
{
  namespace b = depz::bno086;
  Out out;
  switch (static_cast<b::SensorId>(r.sensor_id)) {
    case b::SensorId::Accelerometer:
      accel_ = r.xyz();
      break;
    case b::SensorId::Gyroscope:
      gyro_ = r.xyz();
      break;
    case b::SensorId::Magnetometer: {
        sensor_msgs::msg::MagneticField m;
        m.header = header;
        const auto ut = r.xyz();
        m.magnetic_field.x = ut[0] * 1e-6;  // µT -> T
        m.magnetic_field.y = ut[1] * 1e-6;
        m.magnetic_field.z = ut[2] * 1e-6;
        out.mag = m;
        break;
      }
    case b::SensorId::LinearAcceleration:
      out.linear_acceleration = stamped(header, r.xyz());
      break;
    case b::SensorId::Gravity:
      out.gravity = stamped(header, r.xyz());
      break;
    case b::SensorId::RotationVector:
    case b::SensorId::GameRotationVector: {
        auto imu = blank_imu(header);
        const auto q = r.quaternion();  // i, j, k, real
        imu.orientation.x = q[0];
        imu.orientation.y = q[1];
        imu.orientation.z = q[2];
        imu.orientation.w = q[3];
        imu.orientation_covariance[0] = 0.0;
        if (const auto acc = r.accuracy_rad(); acc && *acc > 0.0) {
          // The hub's heading accuracy estimate (rad), as a variance on each axis.
          const double var = *acc * *acc;
          imu.orientation_covariance[0] = var;
          imu.orientation_covariance[4] = var;
          imu.orientation_covariance[8] = var;
        }
        if (gyro_) {
          set_vec(imu.angular_velocity, *gyro_);
          imu.angular_velocity_covariance[0] = 0.0;
        }
        if (accel_) {
          set_vec(imu.linear_acceleration, *accel_);
          imu.linear_acceleration_covariance[0] = 0.0;
        }
        out.imu = imu;
        break;
      }
    default:
      break;
  }
  return out;
}

}  // namespace depz_sensors
