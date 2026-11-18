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


// The time stamp of a sample: when the board took it, on the node's clock.
// No node, no device: plain numbers in, the stamp out, so the unit tests cover
// it without ROS.
//
// A board's clock is mapped to the host's by the time sync, which the node
// takes again every 2 s. Between two syncs the board's clock runs on at its
// own rate — 18 to 371 ppm fast on the bench of 2026-11-13, and not evenly:
// the rate moves by hundreds of ppm within seconds — so the mapping follows
// the rate of the last syncs (BoardClock). Each sync still moves it a
// little, and a sample may map to a moment before the previous one's, or
// after "now". What a subscriber gets, per stream (the messages that share a
// sample):
//   - never a stamp in the future: at most the arrival time;
//   - never a stamp before the clock's zero (simulated time that has not
//     started yet, or has just started): ROS has no negative time;
//   - never a stamp before the previous one's — the previous stamp is
//     repeated instead — unless the previous stamp is itself after the
//     arrival: then the node's clock was set back (a time server's step,
//     simulated time restarted), and the stamps follow it.
#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>

namespace depz_sensors
{

// A sample said to be older than this was not stamped by the clock the sync
// knows (a board that restarted): it gets its arrival time.
inline constexpr std::int64_t kOldestSampleUs = 5'000'000;

// The rate of a board's clock is taken over this many time syncs (12 s of
// the node's): one sync is good to a fraction of its round trip, 0.35 ms on
// the bench, which over a single interval of 2 s would be near 100 ppm; and
// the rate of a minute ago is no longer the clock's.
inline constexpr std::size_t kDriftSyncs = 7;
// The rate is carried on this long past the last sync; the offset stays there
// when no further sync comes.
inline constexpr std::int64_t kDriftHoldUs = 60'000'000;

// A board's clock against the host's, from the time syncs (the SDK's
// depz::TimeSync): the offset of the last one, carried on at the rate the
// offsets of the last kDriftSyncs moved at.
class BoardClock {
public:
    // A time sync: at `host_us` of the host's clock the board's was
    // `offset_us` ahead. A sync that is not after the one before, or whose
    // offset moved by over 1 % of the time since it (no clock is that far
    // off: the board's clock restarted), starts the rate anew.
  void synced(std::int64_t offset_us, std::int64_t host_us);
    // How long before `host_now_us` the board's clock read `device_us`, us;
    // nullopt before the first sync.
  std::optional<std::int64_t> age_us(std::int64_t host_now_us, std::uint64_t device_us) const;

private:
  struct Sync
  {
    std::int64_t offset_us, host_us;
  };
  std::deque<Sync> syncs_;    // the oldest first
};

class SampleStamps {
public:
  // The stamp, in ns of the node's clock, of a sample that arrived at
  // `now_ns` and that the time sync puts `age_us` before the arrival
  // (nullopt: no time sync — the arrival time). `stream` tells apart the
  // samples of one board that are stamped independently (a BNO085 / BNO086
  // report kind); one caller at a time.
  std::int64_t stamp_ns(std::int64_t now_ns, std::optional<std::int64_t> age_us, int stream = 0);

private:
  std::map<int, std::int64_t> last_ns_;
};

// A step of the device clock longer than this is not waited out by a replay
// (a capture with a pause, a clock that restarted): the pace starts afresh there.
inline constexpr std::uint64_t kLongestPaceGapUs = 2'000'000;

// The pace of a replayed capture: its samples go out as far apart as the
// board's clock stamped them. The first is due at once, each later one as
// long after it as the board's clock says — also when one before it went out
// late. A sample stamped a little before the one before it (a BNO085 / BNO086
// stamps each kind of report in its own order) is due at once and leaves the
// pace as it is.
class ReplayPace {
public:
  // When the sample the board stamped `device_us` is due, in µs of the
  // host's monotonic clock, which reads `now_us`.
  std::int64_t due_us(std::uint64_t device_us, std::int64_t now_us);

private:
  bool started_ = false;
  std::uint64_t device_us_ = 0;    // the latest sample of the pace ...
  std::int64_t host_us_ = 0;       // ... and when it was due
};

}  // namespace depz_sensors
