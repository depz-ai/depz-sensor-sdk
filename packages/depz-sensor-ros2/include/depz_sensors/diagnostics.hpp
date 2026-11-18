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


// A board's health -> diagnostic_msgs/DiagnosticStatus (the node publishes one
// per board on /diagnostics). Pure functions of a snapshot the board takes of
// itself: no node, no device, so the unit tests build snapshots by hand.
//
// Levels:
//   ERROR  the board is lost (unplugged, the link dropped, its capture ended) —
//          the message is the cause; reported until the node lets the board go;
//          or it is silent and kErrorAfterFailed restarts in a row did not
//          bring it back (watchdog.hpp);
//   WARN   it is silent (the node restarts it); or
//          it streams, and publishes less than half of the configured rate; or
//          since the previous status samples were dropped, reports did not
//          parse, the link saw CRC errors or sequence gaps; or the time sync is
//          stale;
//   OK     otherwise.
#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <depz/device.hpp>
#include <diagnostic_msgs/msg/diagnostic_status.hpp>

#include "depz_sensors/watchdog.hpp"

namespace depz_sensors
{

// DiagnosticStatus::level, by value: the message's own constant for 2 is named
// ERROR, a macro of the Windows headers.
inline constexpr std::uint8_t kLevelOk = 0, kLevelWarn = 1, kLevelError = 2;

// The counters whose growth is a warning.
struct HealthCounters
{
  // items the board's subscription let go (the publisher was too slow)
  std::uint64_t dropped = 0;
  std::uint64_t parse_errors = 0;         // reports of the stream that did not decode
  std::uint64_t crc_errors = 0;           // the link's: packets with a bad CRC,
  std::uint64_t seq_gaps = 0;             // packets missed (host-observed),
  std::uint64_t device_seq_errors = 0;    // sequence errors the device reported
};

// A publish rate measured over the last seconds.
struct MeasuredRate
{
  double hz = 0.0;
  double span_s = 0.0;    // the time it was measured over
};

// Measures a rate from a message count sampled now and then (once per status):
// the growth of the count over the samples of the last `window_s` seconds.
class RateMeter {
public:
  explicit RateMeter(double window_s = 5.0)
  : window_s_(window_s) {}
  // The count of messages so far, at `t_s` (seconds of a monotonic clock).
  void add(double t_s, std::uint64_t count);
  // nullopt until two samples are in.
  std::optional<MeasuredRate> rate() const;

private:
  double window_s_;
  std::deque<std::pair<double, std::uint64_t>> samples_;
};

// What a board knows about itself at one moment.
struct BoardHealth
{
  std::string name, model, port;
  std::string hardware_id;    // the USB serial number, else the board's own, else empty
  std::vector<std::string> topics;
  // The cause of the loss; set: the board is gone.
  std::optional<std::string> lost;
  // The sensor was started. Not so: a replayed board that waits for its subscribers.
  bool streaming = false;
  std::uint64_t published = 0;         // messages on the board's main topic (range / depth / imu)
  std::optional<MeasuredRate> rate;    // of those messages, since the stream started
  double configured_hz = 0.0;          // what the parameters ask for; 0: unknown
  HealthCounters total;                // since the board was opened ...
  HealthCounters grown;                // ... and since the previous status
  std::optional<depz::LinkStats> link;
  // Device clock -> host clock: the last sync and how old it is. None: time_sync
  // is off, or the board is a capture (its clock answers nothing).
  std::optional<depz::TimeSync> sync;
  double sync_age_s = 0.0;
  std::optional<double> mcu_celsius;                       // the last reading; none on a capture
  std::optional<depz::bno055::CalibStatus> calibration;    // BNO055: the last sample's CALIB_STAT
  WatchState watch;    // the restarts of a silent board
};

// A rate is judged once it was measured over this long and the configured one
// would have given this many messages in that time ...
inline constexpr double kRateJudgedAfterS = 2.0;
inline constexpr double kRateJudgedAfterMessages = 5.0;
// ... and is a warning below this share of the configured one.
inline constexpr double kRateWarnBelow = 0.5;
// The node syncs every 2 s: a sync older than this missed over a dozen in a row.
inline constexpr double kSyncStaleAfterS = 35.0;

// The status of one board: `name` is "<node>: <board name>", the values are
// listed in the package's config/sensors.yaml.
diagnostic_msgs::msg::DiagnosticStatus to_status(
  const std::string & node,
  const BoardHealth & health);

}  // namespace depz_sensors
