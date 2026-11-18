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


#include "depz_sensors/naming.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace depz_sensors
{

namespace
{

std::string lower(std::string s)
{
  std::transform(
    s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  return s;
}

std::string basename(const std::string & path)
{
  const auto slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

}  // namespace

std::string model_name(
  const std::string & sensor_class, const std::string & device_name,
  const std::optional<std::string> & usb_model_hint)
{
  std::string model = sensor_class;
  // One firmware, several models: the device name, else the USB id, tells.
  if (model == "bno086" || model == "vl53lx" || model == "device") {
    static const char * const kModels[] = {"bno085", "bno086", "vl53l0x", "vl53l1cx", "vl53l1cb",
      "vl53l3cx", "vl53l4cd", "vl53l4cx"};
    const std::string name = lower(device_name);
    const char * in_name = nullptr;
    for (const char * m : kModels) {
      if (name.find(m) != std::string::npos) {in_name = m;}
    }
    if (in_name) {
      model = in_name;
    } else if (usb_model_hint) {
      model = *usb_model_hint;
    }
  }
  model = sanitize(model);
  if (model == "sr04") {model = "hcsr04";}
  return model.empty() ? "device" : model;
}

std::string sanitize(const std::string & text)
{
  std::string out;
  for (unsigned char c : lower(text)) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (ok) {
      out += static_cast<char>(c);
    } else if (!out.empty() && out.back() != '_') {
      out += '_';
    }
  }
  while (!out.empty() && out.back() == '_') {out.pop_back();}
  if (!out.empty() && out.front() >= '0' && out.front() <= '9') {out.insert(out.begin(), 'n');}
  return out;
}

std::map<std::string, std::string> parse_aliases(const std::vector<std::string> & entries)
{
  std::map<std::string, std::string> out;
  for (const auto & e : entries) {
    if (e.empty()) {continue;}    // YAML cannot type an empty list: [""] stands for none
    const auto eq = e.find('=');
    if (eq == std::string::npos || eq == 0 || eq + 1 == e.size()) {
      throw std::invalid_argument("aliases: \"" + e + "\" is not SERIAL=name");
    }
    // The name is what sanitize() leaves of it: nothing left is no name (a topic "depz//range").
    if (sanitize(e.substr(eq + 1)).empty()) {
      throw std::invalid_argument("aliases: \"" + e + "\": the name needs a letter or a digit");
    }
    out[lower(e.substr(0, eq))] = e.substr(eq + 1);
  }
  return out;
}

std::string board_name(const BoardId & id, const std::map<std::string, std::string> & aliases)
{
  for (const auto * key : {&id.usb_serial, &id.serial}) {
    if (key->empty()) {continue;}
    const auto it = aliases.find(lower(*key));
    if (it != aliases.end()) {return sanitize(it->second);}
  }
  std::string tail =
    !id.usb_serial.empty() ? id.usb_serial : !id.serial.empty() ? id.serial : basename(id.port);
  return sanitize(id.model + "_" + tail);
}

std::string free_name(const std::string & name, const std::vector<std::string> & taken)
{
  const auto is_taken = [&taken](const std::string & n) {
      return std::find(taken.begin(), taken.end(), n) != taken.end();
    };
  std::string free = name;
  for (int n = 2; is_taken(free); ++n) {
    free = name + "_" + std::to_string(n);
  }
  return free;
}

std::vector<std::string> board_names(
  const std::vector<BoardId> & ids,
  const std::map<std::string, std::string> & aliases,
  const std::vector<std::string> & taken)
{
  std::vector<std::string> names;
  for (const auto & id : ids) {
    names.push_back(board_name(id, aliases));
  }
  // The plain names first, each to the first board that has it ...
  std::vector<std::string> used = taken;
  std::vector<bool> suffixed(ids.size(), false);
  for (std::size_t i = 0; i < names.size(); ++i) {
    suffixed[i] = std::find(used.begin(), used.end(), names[i]) != used.end();
    if (!suffixed[i]) {used.push_back(names[i]);}
  }
  // ... then the suffixes, around all of them.
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (!suffixed[i]) {continue;}
    names[i] = free_name(names[i], used);
    used.push_back(names[i]);
  }
  return names;
}

}  // namespace depz_sensors
