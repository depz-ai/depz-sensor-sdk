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


// What came from a board before it went silent, kept so that the log can say
// it when the silence is found: the board's counters at each look of the node
// (samples published, packets and bytes the board sent, the link's errors) for
// the last ten seconds, told a line a second, and its last device events (firmware text, a hardware
// fault, CRC errors, gaps). The node prints it once per silence, before its
// first restart: "the board sent nothing" and "the board sent, but nothing
// decoded" are different faults. Pure: the times are given, no node, no
// device, so the unit tests run it by hand. Thread-safe: events come from the
// device's reader thread.
#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include <depz/device.hpp>

namespace depz_sensors
{

// A board's counters, each monotonic while its device stays open.
struct TrailCounters
{
  std::uint64_t published = 0;            // messages on the board's main topic
  std::uint64_t rx_packets = 0, rx_bytes = 0;   // what the board sent
  std::uint64_t crc_errors = 0, header_errors = 0, trash_bytes = 0;
  std::uint64_t seq_gaps = 0, device_seq_errors = 0;
  std::uint64_t parse_errors = 0;         // reports of the stream that did not decode
};

class Trail {
public:
  static constexpr int kSeconds = 10;     // the lines told: the last ten seconds
  static constexpr std::size_t kEvents = 8;

  // The board's counters at `now_s`, as often as the node looks (5 times a
  // second): every snapshot of the last ten seconds is kept, so that what
  // came after the last sample is counted to the look that finds the silence.
  void tick(double now_s, const TrailCounters & counters);
  // A device event, in words (describe()).
  void event(double now_s, std::string what);
  // A new device: what was kept is of the one before.
  void reset();
  // What came before `now_s`, the board's last sample at `last_item_s`
  // (nullopt: none since it was opened), as lines for the log.
  std::string story(double now_s, std::optional<double> last_item_s) const;

private:
  mutable std::mutex lock_;
  std::deque<std::pair<double, TrailCounters>> ticks_;
  std::deque<std::pair<double, std::string>> events_;
};

// A device event in words, for the trail; the MCU temperature is not one.
std::optional<std::string> describe(const depz::DeviceEvent & e);

}  // namespace depz_sensors
