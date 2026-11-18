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

#include "depz_sensors/access.hpp"

#include <iterator>

namespace depz_sensors
{

bool DeniedPorts::report(const std::string & port, bool startup)
{
  const auto [entry, fresh] = reported_.try_emplace(port, false);
  if (entry->second) {return false;}       // said once
  if (fresh && !startup) {return false;}   // just plugged in: udev may not be through with it
  entry->second = true;
  return true;
}

void DeniedPorts::clear(const std::string & port) {reported_.erase(port);}

void DeniedPorts::keep(const std::set<std::string> & present)
{
  for (auto it = reported_.begin(); it != reported_.end(); ) {
    it = present.count(it->first) ? std::next(it) : reported_.erase(it);
  }
}

}  // namespace depz_sensors
