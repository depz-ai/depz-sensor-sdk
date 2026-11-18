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

// When to say that a port cannot be opened for lack of access rights.
// No node, no device: the unit tests cover it without ROS.
//
// A port the user may not open is reported once, with the remedy. But a port
// that has just appeared is not the user's yet: the kernel makes the device
// file, and udev gives it its group and mode a moment later. A board plugged
// in while the node looks for new ones (hot_plug) can be seen in between (the
// bench of 2026-11-12: once in 109 returns of a board) — its port opens at
// the next look. So a look while the node runs reports a port only when the
// look before found it without access too; the first look of the node, at
// ports that were there before it, reports at once.
#pragma once

#include <map>
#include <set>
#include <string>

namespace depz_sensors
{

class DeniedPorts {
public:
    // `port` has no read/write access at this look; true: say so now.
    // `startup`: the node's first look.
  bool report(const std::string & port, bool startup);
    // `port` could be accessed: a later denial is a first one again.
  void clear(const std::string & port);
    // Forget every port that is not in `present` (unplugged).
  void keep(const std::set<std::string> & present);

private:
  std::map<std::string, bool> reported_;    // every port found without access: was it said?
};

}  // namespace depz_sensors
