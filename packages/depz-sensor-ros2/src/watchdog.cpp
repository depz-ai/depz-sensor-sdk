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


#include "depz_sensors/watchdog.hpp"

#include <algorithm>
#include <cmath>

namespace depz_sensors
{

double silent_after_s(double parameter_s, double period_s)
{
  if (!(parameter_s > 0.0)) {return 0.0;}
  return period_s > 0.0 ? std::max(parameter_s, kSilentPeriods * period_s) : parameter_s;
}

double restart_wait_s(int index)
{
  // 2^index, without leaving the range of the exponent.
  return index >= 5 ? kLongestWaitS : std::min(kLongestWaitS, std::ldexp(kFirstWaitS, index));
}

void Watchdog::started(double now_s)
{
  stream_since_ = now_s;
  last_data_ = now_s;
}

bool Watchdog::look(double now_s, std::optional<double> last_item_s)
{
  if (!(after_ > 0.0)) {return false;}
  // An item of the stream that runs now (not one from before a restart).
  const bool fresh = stream_since_ && last_item_s && *last_item_s >= *stream_since_;
  const double before = last_data_;
  if (fresh) {last_data_ = std::max(last_data_, *last_item_s);}
  const bool silent = !stream_since_ || now_s - (fresh ? *last_item_s : *stream_since_) >= after_;
  if (silent) {
    if (!quiet_) {
      quiet_ = true;
      in_silence_ = 0;
    }
    steady_since_.reset();
    if (pending_) {    // the restart went through and brought nothing
      pending_ = false;
      ++failed_;
    }
    return !next_at_ || now_s >= *next_at_;
  }
  if (!fresh) {return false;}    // just started: its first item is not late yet
  if (quiet_) {
    quiet_ = false;
    if (in_silence_ > 0) {++recoveries_;}
    steady_since_ = now_s;
    recovery_ = Recovery{last_data_ - before, in_silence_};
  }
  pending_ = false;
  failed_ = 0;
  if (steady_since_ && now_s - *steady_since_ >= kSteadyAfterS) {
    steady_since_.reset();
    wait_index_ = 0;
    next_at_.reset();
  }
  return false;
}

bool Watchdog::restarted(double now_s, const std::string & reason, const std::string & error)
{
  ++restarts_;
  ++in_silence_;
  if (error.empty()) {
    stream_since_ = now_s;
    pending_ = true;
    last_ = reason + ": restarted";
  } else {
    stream_since_.reset();
    pending_ = false;
    ++failed_;
    last_ = reason + ": the restart failed: " + error;
  }
  const double wait = restart_wait_s(wait_index_);
  next_at_ = now_s + wait;
  ++wait_index_;
  const bool longest = wait >= kLongestWaitS;
  if (longest && said_longest_ && now_s - *said_at_ < kRestartSaidEveryS) {return false;}
  said_at_ = now_s;
  said_longest_ = longest;
  return true;
}

std::optional<Recovery> Watchdog::take_recovery()
{
  auto out = recovery_;
  recovery_.reset();
  return out;
}

WatchState Watchdog::state(double now_s) const
{
  WatchState s;
  s.silent_after_s = after_;
  if (quiet_) {s.silent_for_s = now_s - last_data_;}
  s.restarts = restarts_;
  s.recoveries = recoveries_;
  s.failed_in_row = failed_;
  s.last = last_;
  return s;
}

}  // namespace depz_sensors
