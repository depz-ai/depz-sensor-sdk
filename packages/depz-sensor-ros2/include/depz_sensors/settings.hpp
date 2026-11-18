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


// The node's sensor parameters: one value for every board of a family
// (`tof_resolution: 64`), and the same parameter for one board under its name
// (`boards.front_tof.tof_resolution: 16`, plus `boards.<name>.frame_id`). No
// node, no device: plain values in, a board's configuration out, so the unit
// tests cover it without ROS.
//
// What a value may be is the SDK's to say: this file checks what has to be
// known before the SDK is called — the type, a name of an enumeration, a
// number that fits the SDK's argument — and the board (board.cpp) passes the
// rest on, naming the parameter when the SDK refuses it.
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <depz/device.hpp>

namespace depz_sensors
{

// A parameter value as given to the node; monostate is a value of a type no
// sensor parameter has (a list).
using ParamValue = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

// A parameter that cannot be used: what() names it as the user wrote it
// ("boards.front_tof.tof_resolution" or "tof_resolution"), gives its value and
// says what it accepts.
class ParameterError : public std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

// The kinds of board the node drives (one per SDK class), as bits.
enum BoardKind : unsigned
{
  kSr04 = 1U << 0,
  kVl53l4cd = 1U << 1,
  kVl53lx = 1U << 2,       // VL53L0X / L1CX / L1CB / L3CX / L4CX
  kMultizone = 1U << 3,    // VL53L5CX / L7CX / L7CH / L8CX / L8CH
  kTmf8829 = 1U << 4,
  kBno055 = 1U << 5,
  kBno086 = 1U << 6,       // BNO085 / BNO086
  kEveryBoard = (1U << 7) - 1,
};

// The sensor parameters, with their defaults. A text that is empty, an
// integration time of 0 and a sharpener of -1 leave the setting as the sensor
// starts with it: nothing is sent. The extra topics are off: the node
// publishes the family's own topics only.
struct SensorSettings
{
  int sr04_period_ms = 50;
  int tof_resolution = 64;
  int tof_frequency_hz = 15;
  std::string tof_ranging_mode;
  int tof_integration_time_ms = 0;
  std::string tof_target_order;
  int tof_sharpener_percent = -1;
  int range_budget_ms = 50;
  std::string range_mode;
  double imu_rate_hz = 100.0;
  std::string bno055_mode = "ndof";
  std::string bno086_orientation = "rotation_vector";
  double bno086_mag_rate_hz = 100.0;
  std::string tmf8829_preset = "8x8";
  int tmf8829_period_ms = 33;
  // A board silent for this long (and for 5 of its periods) is restarted; 0: never.
  double silent_restart_after_s = 1.0;
  // ---- the extra topics ----
  bool mcu_temperature = false;            // <board>/mcu_temperature
  bool sensor_temperature = false;         // <board>/temperature
  bool tof_signal = false;                 // <board>/depth/signal
  bool tof_status = false;                 // <board>/depth/status
  int tmf8829_peaks = 1;                   // <board>/depth/peak_2 ... peak_<n>
  bool point_cloud = false;                // <board>/points
  bool imu_linear_acceleration = false;    // <board>/linear_acceleration
  bool imu_gravity = false;                // <board>/gravity
};

// One sensor parameter: its name, where it lives in SensorSettings (which
// fixes its type), the kinds of board it configures and what it accepts, in
// words for an error message.
struct ParamSpec
{
  using Field = std::variant<int SensorSettings::*, double SensorSettings::*,
      std::string SensorSettings::*,
      bool SensorSettings::*>;
  const char * name;
  Field field;
  unsigned kinds;
  const char * boards;    // the kinds in words: "VL53L5CX / L7 / L8"
  const char * accepts;
};

// Every sensor parameter, in the order of the documentation.
const std::vector<ParamSpec> & sensor_params();
// The parameter called `name`, or nullptr.
const ParamSpec * find_param(const std::string & name);
// Its value in `settings`.
ParamValue get_param(const SensorSettings & settings, const ParamSpec & spec);
// Put `value` there. ParameterError (naming it `written`) when the type is
// not the parameter's; an integer is taken for a real-number parameter, a
// whole real number that fits for an integer one, true / false alone for a switch.
void set_param(
  SensorSettings & settings, const ParamSpec & spec, const ParamValue & value,
  const std::string & written);

// `value` as it reads in a message: 16, 12.5, "long", true.
std::string to_text(const ParamValue & value);

// The parameters given under `boards.`: board name -> parameter -> value.
using BoardOverrides = std::map<std::string, std::map<std::string, ParamValue>>;

// Pick the `boards.<name>.<parameter>` entries out of every parameter given
// to the node; other names are left alone. What <parameter> is, is checked
// when the board is configured (board_config), so a mistake there stops that
// board only. ParameterError for a name that is not of that form
// ("boards.front_tof", "boards..x").
BoardOverrides board_overrides(const std::map<std::string, ParamValue> & given);

// The configuration of one board: the family-wide settings with the board's
// own on top.
class BoardConfig {
public:
  SensorSettings sensor;
  std::string frame_id;    // boards.<name>.frame_id; "" = the node's frame_prefix + name

  // `parameter` as the user wrote the value in effect: "boards.<name>.<p>"
  // when the board has its own, else "<p>".
  std::string written(const std::string & parameter) const;
  // Throw ParameterError: "parameter <written> = <value> (<accepts>): <why>".
  [[noreturn]] void refuse(const std::string & parameter, const std::string & why) const;
  // ParameterError when the board has a parameter of its own that
  // configures other kinds of board (imu_rate_hz on a ToF board).
  void require_kind(BoardKind kind, const std::string & model) const;
  // An integer parameter that must be lo..hi to fit the SDK's argument.
  int whole(const std::string & parameter, std::int64_t lo, std::int64_t hi) const;
  // A real-number parameter that must be finite and positive (or zero when `zero`).
  double rate(const std::string & parameter, bool zero = false) const;

  // ---- names of an enumeration -> the SDK's value ----
  // tof_ranging_mode: 1 continuous / 3 autonomous (the SDK's
  // set_ranging_mode); nullopt when left alone. Also refuses an integration
  // time given together with the continuous mode.
  std::optional<std::uint8_t> tof_ranging_mode() const;
  // tof_target_order: 1 closest / 2 strongest; nullopt when left alone.
  std::optional<std::uint8_t> tof_target_order() const;
  // bno055_mode: one of the chip's fusion modes (they give the orientation).
  depz::bno055::OprMode bno055_mode() const;
  // bno086_orientation: the report the Imu's orientation comes from.
  depz::bno086::SensorId bno086_orientation() const;
  // range_mode against the modes the board's driver has (Vl53lx::modes());
  // nullopt when left alone.
  std::optional<std::string> range_mode(const std::vector<std::string> & modes) const;
  // tmf8829_preset: one of the SDK's presets.
  std::string tmf8829_preset() const;

private:
  friend BoardConfig board_config(
    const SensorSettings &, const std::string &,
    const BoardOverrides &);
  std::string name_;
  std::vector<std::string> own_;    // the parameters given under boards.<name>
};

// The configuration of the board called `name`. ParameterError for a
// parameter under boards.<name> that does not exist or has the wrong type.
BoardConfig board_config(
  const SensorSettings & family, const std::string & name,
  const BoardOverrides & overrides);

// The `qos` and `qos_depth` parameters.
struct QosChoice
{
  bool reliable = false;    // false: the sensor-data profile (best effort)
  int depth = 5;            // messages kept for a slow subscriber
};
// `profile` "sensor_data" | "reliable"; `depth` 0 = the profile's own (5 for
// sensor data, 10 for reliable), else 1 or more. ParameterError otherwise.
QosChoice qos_choice(const std::string & profile, std::int64_t depth);

}  // namespace depz_sensors
