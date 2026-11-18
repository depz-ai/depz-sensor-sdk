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


// Stable names for discovered DEPZ boards: topic and frame names come from the
// board's model and its USB serial number, never from the enumeration order,
// so a board keeps its topics across restarts, replugs and other boards.
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace depz_sensors
{

// What naming needs to know about one board (filled from depz::DeviceInfo).
struct BoardId
{
  std::string model;         // "vl53l8ch", "bno086", "hcsr04", ... (model_name())
  std::string usb_serial;    // USB iSerial, "" when the OS gave none
  std::string serial;        // protocol serial (GET_SERIAL), "" when none
  std::string port;          // "/dev/ttyACM3"
};

// The model part of a name: the SDK's sensor class (resolve_sensor_class),
// refined where the class covers several models (a BNO085 runs the BNO086
// firmware; "vl53lx" is the whole 1D family): the model the board's device
// name carries ("DEPZ IMU BNO085 USB v2.1 ..."), else the USB model hint.
// "sr04" is spelled "hcsr04" as in the task. Lower case, [a-z0-9].
std::string model_name(
  const std::string & sensor_class, const std::string & device_name,
  const std::optional<std::string> & usb_model_hint);

// Lower case, every character outside [a-z0-9] turned into '_', runs of '_'
// collapsed, none leading or trailing: a valid ROS name token. A token that
// would start with a digit gets a leading 'n' (ROS names may not).
std::string sanitize(const std::string & text);

// "SERIAL=alias" entries (the `aliases` parameter) -> map from the serial
// (as given, compared case-insensitively) to the alias. An entry without
// '=' or with an empty side, or whose name has no letter or digit (nothing
// is left of it as a ROS name), is an error: std::invalid_argument naming it.
// An empty entry is skipped (YAML's [""] for "no aliases").
std::map<std::string, std::string> parse_aliases(const std::vector<std::string> & entries);

// The name of a board: its alias when one matches its USB serial or its
// protocol serial; else "<model>_<usb serial>" (else the protocol serial,
// else the port's basename — a board without a serial is only as stable as
// its port; the README says so). Sanitized.
std::string board_name(const BoardId & id, const std::map<std::string, std::string> & aliases);

// `name` when none of `taken` is it, else the first of `name`_2, `name`_3 ...
// that is free.
std::string free_name(const std::string & name, const std::vector<std::string> & taken);

// Names for every board of one discovery, collision-free: two boards that
// would share a name (no serial, or the same alias twice) get "_2", "_3" ...
// in the order of `ids`, which the caller sorts by port; the first keeps the
// plain name. Every plain name is given to its first board before any suffix
// is, so a suffixed name never lands on another board's own, wherever that
// board stands in `ids` (alias "tof" twice and an alias "tof_2": tof, tof_3,
// tof_2). `taken` are names already in use (the boards that run): none of
// them is given, a board whose name is among them gets a suffix. Same order
// as `ids`.
std::vector<std::string> board_names(
  const std::vector<BoardId> & ids,
  const std::map<std::string, std::string> & aliases,
  const std::vector<std::string> & taken = {});

}  // namespace depz_sensors
