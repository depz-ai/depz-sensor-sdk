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


// A board that stays on USB and stops delivering is restarted by the node:
// this is the decision — when a board is silent, when the next restart is
// due, what its status says. Pure: the times are given (seconds of a
// monotonic clock), no node, no device, so the unit tests run it by hand.
//
//   silent     no item of the board's stream for silent_after_s();
//   restart    at once when the silence is found, then after waits that
//              double — 1, 2, 4, 8, 16, 30, 30 ... s — for as long as the
//              board is silent; never given up;
//   steady     the waits start at 1 s again once the board has delivered for
//              a minute without a silence;
//   failed     a restart that threw, or went through and was followed by
//              silence; a delivered item ends the row of failed ones;
//   said       one log line per restart while the waits grow, then — a board
//              silent for hours — one in five minutes.
#pragma once

#include <optional>
#include <string>

namespace depz_sensors
{

// A stream is silent after this many of its expected periods ...
inline constexpr double kSilentPeriods = 5.0;
// The wait after the first restart of a row; it doubles up to the longest.
inline constexpr double kFirstWaitS = 1.0;
inline constexpr double kLongestWaitS = 30.0;
// Delivered for this long without a silence: the waits start anew.
inline constexpr double kSteadyAfterS = 60.0;
// Every restart is said in the log while the waits grow; at the longest wait,
// one in this many seconds.
inline constexpr double kRestartSaidEveryS = 300.0;
// This many failed restarts in a row: the board's status is an error.
inline constexpr int kErrorAfterFailed = 3;

// ... and never sooner than the `silent_restart_after_s` parameter:
// max(parameter_s, kSilentPeriods * period_s). `period_s` is the time between
// two items the board's parameters ask for; 0: unknown, the parameter alone.
// A parameter of 0 switches the restarts off: 0.
double silent_after_s(double parameter_s, double period_s);

// The wait after the restart number `index` (from 0) of a row, seconds.
double restart_wait_s(int index);

// What the watchdog knows at one moment, for the board's status.
struct WatchState
{
  double silent_after_s = 0.0;           // 0: the board is not watched
  std::optional<double> silent_for_s;    // set: the board is silent, for this long
  int restarts = 0;                      // restarts made since the node started
  int recoveries = 0;                    // silences that ended after a restart
  int failed_in_row = 0;                 // failed restarts since the last item
  std::string last;                      // the last restart in words; "": none yet
};

// A silence that ended: for the log.
struct Recovery
{
  double silent_s = 0.0;    // from the last item before it to the item that ended it
  int restarts = 0;         // made during it
};

class Watchdog {
public:
  // `silent_after_s` 0: nothing is ever due.
  explicit Watchdog(double silent_after_s = 0.0)
  : after_(silent_after_s) {}

  // The board's stream was started (the node's first start of it).
  void started(double now_s);
  // Look at the board: `last_item_s` is when its stream last delivered
  // (nullopt: never). True: a restart is due now — make it, then say
  // restarted().
  bool look(double now_s, std::optional<double> last_item_s);
  // The restart that was due is made, at `now_s`. `reason`: why, in words;
  // `error` empty: the board was reopened and its stream started again;
  // else what failed — the board is closed until the next restart. True:
  // this restart is one to say in the log.
  bool restarted(double now_s, const std::string & reason, const std::string & error);
  // The silence that the last look() found ended, once.
  std::optional<Recovery> take_recovery();
  WatchState state(double now_s) const;
  // When the next restart may be made at the earliest; nullopt: at once.
  std::optional<double> next_restart_s() const {return next_at_;}

private:
  double after_;
  std::optional<double> stream_since_;    // the running stream's start; none: the board is closed
  double last_data_ = 0.0;                // the last item, or the first start
  bool quiet_ = false;                    // the last look found the board silent
  bool pending_ = false;                  // a restart went through; no item since
  int in_silence_ = 0;                    // restarts of the silence that lasts
  int restarts_ = 0, recoveries_ = 0, failed_ = 0;
  int wait_index_ = 0;
  std::optional<double> next_at_;
  std::optional<double> steady_since_;    // delivering since, after a restart
  std::optional<double> said_at_;         // the last restart said in the log ...
  bool said_longest_ = false;             // ... was followed by the longest wait
  std::string last_;
  std::optional<Recovery> recovery_;
};

}  // namespace depz_sensors
