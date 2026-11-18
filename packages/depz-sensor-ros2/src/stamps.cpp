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


#include "depz_sensors/stamps.hpp"

#include <algorithm>

namespace depz_sensors
{

void BoardClock::synced(std::int64_t offset_us, std::int64_t host_us)
{
  if (!syncs_.empty()) {
    const Sync & last = syncs_.back();
    const std::int64_t elapsed = host_us - last.host_us;
    const std::int64_t moved = offset_us > last.offset_us ?
      offset_us - last.offset_us : last.offset_us - offset_us;
    if (elapsed <= 0 || moved > elapsed / 100) {syncs_.clear();}
  }
  syncs_.push_back({offset_us, host_us});
  if (syncs_.size() > kDriftSyncs) {syncs_.pop_front();}
}

std::optional<std::int64_t> BoardClock::age_us(
  std::int64_t host_now_us,
  std::uint64_t device_us) const
{
  if (syncs_.empty()) {return std::nullopt;}
  const Sync & first = syncs_.front();
  const Sync & last = syncs_.back();
  std::int64_t offset = last.offset_us;
  if (last.host_us > first.host_us) {
    // Whole microseconds, toward zero: the same on every platform.
    const std::int64_t since =
      std::clamp<std::int64_t>(host_now_us - last.host_us, 0, kDriftHoldUs);
    offset += (last.offset_us - first.offset_us) * since / (last.host_us - first.host_us);
  }
  return host_now_us - (static_cast<std::int64_t>(device_us) - offset);
}

std::int64_t SampleStamps::stamp_ns(
  std::int64_t now_ns, std::optional<std::int64_t> age_us,
  int stream)
{
  std::int64_t stamp = now_ns;
  if (age_us && *age_us > 0 && *age_us < kOldestSampleUs) {
    // Not before the clock's zero (a clock that reads less than the age: simulated time
    // at its start).
    stamp = std::max<std::int64_t>(stamp - *age_us * 1000, std::min<std::int64_t>(now_ns, 0));
  }
  const auto [last, first] = last_ns_.try_emplace(stream, stamp);
  if (!first) {
    // The previous stamp is held only while it is not in the future: one
    // after `now_ns` means the node's clock was set back, by any amount.
    if (stamp < last->second && last->second <= now_ns) {stamp = last->second;}
    last->second = stamp;
  }
  return stamp;
}

std::int64_t ReplayPace::due_us(std::uint64_t device_us, std::int64_t now_us)
{
  if (!started_ || device_us > device_us_ + kLongestPaceGapUs ||
    device_us + kLongestPaceGapUs < device_us_)
  {
    started_ = true;
    device_us_ = device_us;
    host_us_ = now_us;
    return now_us;
  }
  if (device_us <= device_us_) {return now_us;}    // a step back: at once, the pace stays
  host_us_ += static_cast<std::int64_t>(device_us - device_us_);
  device_us_ = device_us;
  return host_us_;
}

}  // namespace depz_sensors
