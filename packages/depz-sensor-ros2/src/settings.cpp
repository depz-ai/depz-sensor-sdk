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


#include "depz_sensors/settings.hpp"

// DEPZ_VL53L8_RANGING_MODE_* / _TARGET_ORDER_*: the values the C++ SDK's setters take
#include <depz_sensor_io.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>

namespace depz_sensors
{

namespace
{

constexpr const char * kBoards = "boards.";
constexpr const char * kFrameId = "frame_id";

// name -> value pairs of an enumeration, in the order a message lists them.
template<class T>
using Names = std::vector<std::pair<const char *, T>>;

template<class T>
std::string listed(const Names<T> & names)
{
  std::string out;
  for (const auto & n : names) {
    out += (out.empty() ? "\"" : ", \"") + std::string(n.first) + "\"";
  }
  return out;
}

template<class T>
std::optional<T> named(const Names<T> & names, const std::string & text)
{
  for (const auto & n : names) {
    if (text == n.first) {return n.second;}
  }
  return std::nullopt;
}

const Names<std::uint8_t> & ranging_modes()
{
  static const Names<std::uint8_t> names{{"continuous",
    static_cast<std::uint8_t>(DEPZ_VL53L8_RANGING_MODE_CONTINUOUS)},
    {"autonomous", static_cast<std::uint8_t>(DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS)}};
  return names;
}

const Names<std::uint8_t> & target_orders()
{
  static const Names<std::uint8_t> names{{"closest",
    static_cast<std::uint8_t>(DEPZ_VL53L8_TARGET_ORDER_CLOSEST)},
    {"strongest", static_cast<std::uint8_t>(DEPZ_VL53L8_TARGET_ORDER_STRONGEST)}};
  return names;
}

// The fusion modes: those that compute the orientation an Imu message carries.
const Names<depz::bno055::OprMode> & bno055_modes()
{
  using M = depz::bno055::OprMode;
  static const Names<M> names{{"imu", M::Imu},
    {"compass", M::Compass},
    {"m4g", M::M4g},
    {"ndof_fmc_off", M::NdofFmcOff},
    {"ndof", M::Ndof}};
  return names;
}

const Names<depz::bno086::SensorId> & bno086_orientations()
{
  using S = depz::bno086::SensorId;
  static const Names<S> names{{"rotation_vector", S::RotationVector},
    {"game_rotation_vector", S::GameRotationVector}};
  return names;
}

std::string quoted_list(const std::vector<std::string> & items)
{
  std::string out;
  for (const auto & i : items) {
    out += (out.empty() ? "\"" : ", \"") + i + "\"";
  }
  return out;
}

}  // namespace

const std::vector<ParamSpec> & sensor_params()
{
  using S = SensorSettings;
  static const std::vector<ParamSpec> specs{
    {"sr04_period_ms", &S::sr04_period_ms, kSr04, "HC-SR04",
      "milliseconds between measurements, 1 or more"},
    {"tof_resolution", &S::tof_resolution, kMultizone, "VL53L5CX / L7 / L8",
      "16 (4x4) or 64 (8x8) zones"},
    {"tof_frequency_hz", &S::tof_frequency_hz, kMultizone, "VL53L5CX / L7 / L8",
      "frames per second: up to 60 at 4x4 and 15 at 8x8, "
      "from 2 on a VL53L8 and from 1 on a VL53L5CX / L7"},
    {"tof_ranging_mode", &S::tof_ranging_mode, kMultizone, "VL53L5CX / L7 / L8",
      "\"continuous\" or \"autonomous\"; \"\" leaves the sensor's own"},
    {"tof_integration_time_ms", &S::tof_integration_time_ms, kMultizone, "VL53L5CX / L7 / L8",
      "2..1000 ms, in the autonomous ranging mode; 0 leaves the sensor's own"},
    {"tof_target_order", &S::tof_target_order, kMultizone, "VL53L5CX / L7 / L8",
      "\"closest\" or \"strongest\"; \"\" leaves the sensor's own"},
    {"tof_sharpener_percent", &S::tof_sharpener_percent, kMultizone, "VL53L5CX / L7 / L8",
      "0..99 %, 0 = off; -1 leaves the sensor's own"},
    {"range_budget_ms", &S::range_budget_ms, kVl53l4cd | kVl53lx,
      "VL53L4CD and VL53L0X / L1 / L3CX / L4CX",
      "milliseconds per measurement, within the sensor's own limits"},
    {"range_mode", &S::range_mode, kVl53lx, "VL53L0X / L1 / L3CX / L4CX",
      "a distance mode of the sensor's driver; \"\" leaves the driver's own"},
    {"imu_rate_hz", &S::imu_rate_hz, kBno055 | kBno086, "BNO055 and BNO085 / BNO086",
      "samples per second, above 0"},
    {"bno055_mode", &S::bno055_mode, kBno055, "BNO055",
      "a fusion mode: \"imu\", \"compass\", \"m4g\", "
      "\"ndof_fmc_off\" or \"ndof\""},
    {"bno086_orientation", &S::bno086_orientation, kBno086, "BNO085 / BNO086",
      "\"rotation_vector\" or \"game_rotation_vector\""},
    {"bno086_mag_rate_hz", &S::bno086_mag_rate_hz, kBno086, "BNO085 / BNO086",
      "magnetometer samples per second; 0 = no magnetometer"},
    {"tmf8829_preset", &S::tmf8829_preset, kTmf8829, "TMF8829",
      "a preset of the SDK: \"8x8\", \"16x16\", ..."},
    {"tmf8829_period_ms", &S::tmf8829_period_ms, kTmf8829, "TMF8829",
      "milliseconds between measurements"},
    {"silent_restart_after_s", &S::silent_restart_after_s, kEveryBoard, "every board",
      "seconds without a sample after which the board is restarted, 0 or more; 0 = never"},
    // The extra topics.
    {"mcu_temperature", &S::mcu_temperature, kEveryBoard, "every board",
      "true publishes the board's MCU temperature once a second"},
    {"sensor_temperature", &S::sensor_temperature, kMultizone | kTmf8829 | kBno055,
      "VL53L5CX / L7 / L8, TMF8829 and BNO055",
      "true publishes the sensor's own temperature with each sample"},
    {"tof_signal", &S::tof_signal, kMultizone, "VL53L5CX / L7 / L8",
      "true publishes each frame's signal per zone as an image"},
    {"tof_status", &S::tof_status, kMultizone, "VL53L5CX / L7 / L8",
      "true publishes each frame's target status per zone as an image"},
    {"tmf8829_peaks", &S::tmf8829_peaks, kTmf8829, "TMF8829",
      "1..4 peaks per zone; each peak after the first is an image of its own"},
    {"point_cloud", &S::point_cloud, kMultizone | kTmf8829, "VL53L5CX / L7 / L8 and TMF8829",
      "true publishes each frame's zones as a point cloud"},
    {"imu_linear_acceleration", &S::imu_linear_acceleration, kBno055 | kBno086,
      "BNO055 and BNO085 / BNO086",
      "true publishes the acceleration without gravity"},
    {"imu_gravity", &S::imu_gravity, kBno055 | kBno086, "BNO055 and BNO085 / BNO086",
      "true publishes the gravity vector"},
  };
  return specs;
}

const ParamSpec * find_param(const std::string & name)
{
  for (const auto & spec : sensor_params()) {
    if (name == spec.name) {return &spec;}
  }
  return nullptr;
}

ParamValue get_param(const SensorSettings & settings, const ParamSpec & spec)
{
  return std::visit(
    [&](auto member) -> ParamValue {
      const auto & v = settings.*member;
      if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
        return static_cast<std::int64_t>(v);
      } else {
        return v;
      }
        },
        spec.field);
}

std::string to_text(const ParamValue & value)
{
  struct Text
  {
    std::string operator()(std::monostate) const {return "a list";}
    std::string operator()(bool b) const {return b ? "true" : "false";}
    std::string operator()(std::int64_t i) const {return std::to_string(i);}
    std::string operator()(double d) const
    {
      std::ostringstream s;
      s << d;
      return s.str();
    }
    std::string operator()(const std::string & t) const {return "\"" + t + "\"";}
  };
  return std::visit(Text{}, value);
}

void set_param(
  SensorSettings & settings, const ParamSpec & spec, const ParamValue & value,
  const std::string & written)
{
  const auto wrong = [&](const char * wanted) {
      return ParameterError(
        "parameter " + written + " = " + to_text(value) + ": must be " + wanted + " (" +
        spec.accepts + ")");
    };
  if (const auto * member = std::get_if<int SensorSettings::*>(&spec.field)) {
    constexpr double lo = std::numeric_limits<int>::min(), hi = std::numeric_limits<int>::max();
    const auto * i = std::get_if<std::int64_t>(&value);
    const auto * d = std::get_if<double>(&value);
    const auto ilo = static_cast<std::int64_t>(lo), ihi = static_cast<std::int64_t>(hi);
    if (i && *i >= ilo && *i <= ihi) {
      settings.*(*member) = static_cast<int>(*i);
    } else if (d && *d >= lo && *d <= hi && std::floor(*d) == *d) {
      settings.*(*member) = static_cast<int>(*d);
    } else {
      throw wrong("a whole number");
    }
  } else if (const auto * real = std::get_if<double SensorSettings::*>(&spec.field)) {
    if (const auto * d = std::get_if<double>(&value)) {
      settings.*(*real) = *d;
    } else if (const auto * i = std::get_if<std::int64_t>(&value)) {
      settings.*(*real) = static_cast<double>(*i);
    } else {
      throw wrong("a number");
    }
  } else if (const auto * flag = std::get_if<bool SensorSettings::*>(&spec.field)) {
    const auto * b = std::get_if<bool>(&value);
    if (!b) {throw wrong("true or false");}
    settings.*(*flag) = *b;
  } else {
    const auto * text = std::get_if<std::string>(&value);
    if (!text) {throw wrong("a text");}
    settings.*std::get<std::string SensorSettings::*>(spec.field) = *text;
  }
}

BoardOverrides board_overrides(const std::map<std::string, ParamValue> & given)
{
  BoardOverrides out;
  const std::string prefix = kBoards;
  for (const auto & [name, value] : given) {
    if (name.compare(0, prefix.size(), prefix) != 0) {continue;}
    const auto dot = name.find('.', prefix.size());
    const std::string board = name.substr(prefix.size(),
        dot == std::string::npos ? dot : dot - prefix.size());
    const std::string parameter = dot == std::string::npos ? "" : name.substr(dot + 1);
    if (board.empty() || parameter.empty()) {
      throw ParameterError("parameter " + name +
          ": a board's parameter is boards.<board name>.<parameter>");
    }
    out[board][parameter] = value;
  }
  return out;
}

BoardConfig board_config(
  const SensorSettings & family, const std::string & name,
  const BoardOverrides & overrides)
{
  BoardConfig config;
  config.sensor = family;
  config.name_ = name;
  const auto own = overrides.find(name);
  if (own == overrides.end()) {return config;}
  for (const auto & [parameter, value] : own->second) {
    const std::string written = kBoards + name + "." + parameter;
    if (parameter == kFrameId) {
      const auto * text = std::get_if<std::string>(&value);
      if (!text || text->empty()) {
        throw ParameterError("parameter " + written + " = " + to_text(value) +
            ": must be a frame name");
      }
      config.frame_id = *text;
      continue;
    }
    const ParamSpec * spec = find_param(parameter);
    if (!spec) {
      std::string known = kFrameId;
      for (const auto & s : sensor_params()) {
        known += std::string(", ") + s.name;
      }
      throw ParameterError("parameter " + written + ": no such board parameter (there are: " +
          known + ")");
    }
    set_param(config.sensor, *spec, value, written);
    config.own_.push_back(parameter);
  }
  return config;
}

std::string BoardConfig::written(const std::string & parameter) const
{
  const bool own = std::find(own_.begin(), own_.end(), parameter) != own_.end();
  return own ? kBoards + name_ + "." + parameter : parameter;
}

void BoardConfig::refuse(const std::string & parameter, const std::string & why) const
{
  const ParamSpec * spec = find_param(parameter);
  std::string text = "parameter " + written(parameter);
  if (spec) {text += " = " + to_text(get_param(sensor, *spec)) + " (" + spec->accepts + ")";}
  throw ParameterError(text + ": " + why);
}

void BoardConfig::require_kind(BoardKind kind, const std::string & model) const
{
  for (const auto & parameter : own_) {
    const ParamSpec * spec = find_param(parameter);
    if (spec && !(spec->kinds & kind)) {
      refuse(parameter,
          "this board is a " + model + ", and the parameter configures " + spec->boards);
    }
  }
}

int BoardConfig::whole(const std::string & parameter, std::int64_t lo, std::int64_t hi) const
{
  const ParamSpec * spec = find_param(parameter);
  const int value = sensor.*std::get<int SensorSettings::*>(spec->field);
  if (value < lo || value > hi) {
    refuse(parameter, "must be " + std::to_string(lo) + ".." + std::to_string(hi));
  }
  return value;
}

double BoardConfig::rate(const std::string & parameter, bool zero) const
{
  const ParamSpec * spec = find_param(parameter);
  const double value = sensor.*std::get<double SensorSettings::*>(spec->field);
  if (!std::isfinite(value) || value < 0.0 || (value == 0.0 && !zero)) {
    refuse(parameter, zero ? "must be 0 or more" : "must be above 0");
  }
  return value;
}

std::optional<std::uint8_t> BoardConfig::tof_ranging_mode() const
{
  std::optional<std::uint8_t> mode;
  if (!sensor.tof_ranging_mode.empty()) {
    mode = named(ranging_modes(), sensor.tof_ranging_mode);
    if (!mode) {refuse("tof_ranging_mode", "must be " + listed(ranging_modes()));}
  }
  if (sensor.tof_integration_time_ms != 0 && mode == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS) {
    refuse("tof_integration_time_ms",
        "the integration time is a setting of the autonomous ranging mode, and " +
                                              written("tof_ranging_mode") + " is \"continuous\"");
  }
  return mode;
}

std::optional<std::uint8_t> BoardConfig::tof_target_order() const
{
  if (sensor.tof_target_order.empty()) {return std::nullopt;}
  const auto order = named(target_orders(), sensor.tof_target_order);
  if (!order) {refuse("tof_target_order", "must be " + listed(target_orders()));}
  return order;
}

depz::bno055::OprMode BoardConfig::bno055_mode() const
{
  const auto mode = named(bno055_modes(), sensor.bno055_mode);
  if (!mode) {refuse("bno055_mode", "must be " + listed(bno055_modes()));}
  return *mode;
}

depz::bno086::SensorId BoardConfig::bno086_orientation() const
{
  const auto report = named(bno086_orientations(), sensor.bno086_orientation);
  if (!report) {refuse("bno086_orientation", "must be " + listed(bno086_orientations()));}
  return *report;
}

std::optional<std::string> BoardConfig::range_mode(const std::vector<std::string> & modes) const
{
  if (sensor.range_mode.empty()) {return std::nullopt;}
  if (modes.empty()) {refuse("range_mode", "this sensor's driver has no distance modes");}
  if (std::find(modes.begin(), modes.end(), sensor.range_mode) == modes.end()) {
    refuse("range_mode", "this sensor's driver has the modes " + quoted_list(modes));
  }
  return sensor.range_mode;
}

std::string BoardConfig::tmf8829_preset() const
{
  if (!depz::tmf8829::find_preset(sensor.tmf8829_preset)) {
    std::vector<std::string> presets;
    for (const auto & p : depz::tmf8829::PRESETS) {
      presets.emplace_back(p.name);
    }
    refuse("tmf8829_preset", "the SDK's presets are " + quoted_list(presets));
  }
  return sensor.tmf8829_preset;
}

QosChoice qos_choice(const std::string & profile, std::int64_t depth)
{
  QosChoice choice;
  if (profile == "reliable") {
    choice.reliable = true;
  } else if (profile != "sensor_data") {
    throw ParameterError("qos must be \"sensor_data\" or \"reliable\", not \"" + profile + "\"");
  }
  if (depth < 0 || depth > std::numeric_limits<int>::max()) {
    throw ParameterError(
        "qos_depth must be 0 (the profile's own depth) or the number of messages to keep, not " +
                             std::to_string(depth));
  }
  // The profiles' own: rmw_qos_profile_sensor_data keeps 5, the default (reliable) profile 10.
  choice.depth = depth == 0 ? (choice.reliable ? 10 : 5) : static_cast<int>(depth);
  return choice;
}

}  // namespace depz_sensors
