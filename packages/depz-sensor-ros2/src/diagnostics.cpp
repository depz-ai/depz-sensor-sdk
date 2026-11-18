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


#include "depz_sensors/diagnostics.hpp"

#include <cstdio>

namespace depz_sensors
{

namespace
{

using diagnostic_msgs::msg::DiagnosticStatus;

std::string fixed(double value, int digits)
{
  char text[64];
  std::snprintf(text, sizeof text, "%.*f", digits, value);
  return text;
}

// "3 CRC errors", "1 CRC error".
std::string counted(std::uint64_t n, const char * one, const char * many)
{
  return std::to_string(n) + " " + (n == 1 ? one : many);
}

void add(DiagnosticStatus & status, const char * key, std::string value)
{
  diagnostic_msgs::msg::KeyValue kv;
  kv.key = key;
  kv.value = std::move(value);
  status.values.push_back(std::move(kv));
}

// "no sample for 3.2 s, restarted 2 times".
std::string silence(const WatchState & watch)
{
  return "no sample for " + fixed(*watch.silent_for_s, 1) + " s, restarted " +
         counted(static_cast<std::uint64_t>(watch.restarts), "time", "times");
}

// The reasons of a warning, each in a few words; none: the board is well.
std::vector<std::string> warnings(const BoardHealth & h)
{
  std::vector<std::string> out;
  // A silence is said in place of the rate: the board publishes nothing.
  const bool silent = h.watch.silent_for_s.has_value();
  if (silent) {out.push_back(silence(h.watch));}
  if (!silent && h.streaming && h.rate && h.configured_hz > 0.0 &&
    h.rate->span_s >= kRateJudgedAfterS &&
    h.rate->span_s * h.configured_hz >= kRateJudgedAfterMessages &&
    h.rate->hz < kRateWarnBelow * h.configured_hz)
  {
    out.push_back(h.rate->hz > 0.0 ?
                            "publishes at " + fixed(h.rate->hz,
          1) + " Hz, " + fixed(h.configured_hz, 1) + " Hz configured" :
                            "publishes nothing, " + fixed(h.configured_hz, 1) + " Hz configured");
  }
  if (h.grown.dropped) {
    out.push_back(counted(h.grown.dropped, "sample dropped", "samples dropped"));
  }
  if (h.grown.parse_errors) {
    out.push_back(counted(h.grown.parse_errors, "parse error", "parse errors"));
  }
  if (h.grown.crc_errors) {out.push_back(counted(h.grown.crc_errors, "CRC error", "CRC errors"));}
  if (h.grown.seq_gaps) {out.push_back(counted(h.grown.seq_gaps, "sequence gap", "sequence gaps"));}
  if (h.grown.device_seq_errors) {
    out.push_back(counted(h.grown.device_seq_errors, "sequence error reported by the board",
                              "sequence errors reported by the board"));
  }
  if (h.sync && h.sync_age_s > kSyncStaleAfterS) {
    out.push_back("time sync is " + fixed(h.sync_age_s, 0) + " s old");
  }
  return out;
}

}  // namespace

void RateMeter::add(double t_s, std::uint64_t count)
{
  samples_.emplace_back(t_s, count);
  // Keep the newest sample that is a whole window old, drop those before it.
  while (samples_.size() > 2 && t_s - samples_[1].first >= window_s_) {samples_.pop_front();}
}

std::optional<MeasuredRate> RateMeter::rate() const
{
  if (samples_.size() < 2) {return std::nullopt;}
  const double span = samples_.back().first - samples_.front().first;
  if (!(span > 0.0)) {return std::nullopt;}
  return MeasuredRate{static_cast<double>(samples_.back().second - samples_.front().second) / span,
    span};
}

DiagnosticStatus to_status(const std::string & node, const BoardHealth & h)
{
  DiagnosticStatus status;
  status.name = node + ": " + h.name;
  status.hardware_id = h.hardware_id;
  if (h.lost) {
    status.level = kLevelError;
    status.message = "disconnected: " + *h.lost;
  } else if (h.watch.silent_for_s && h.watch.failed_in_row >= kErrorAfterFailed) {
    status.level = kLevelError;
    status.message = silence(h.watch) + ": " +
      counted(static_cast<std::uint64_t>(h.watch.failed_in_row), "restart", "restarts") +
      " in a row did not bring it back";
  } else if (const auto reasons = warnings(h); !reasons.empty()) {
    status.level = kLevelWarn;
    for (const auto & reason : reasons) {
      status.message += (status.message.empty() ? "" : "; ") + reason;
    }
  } else {
    status.level = kLevelOk;
    status.message = h.streaming ? "streaming" : "waiting for a subscriber on each topic";
  }

  add(status, "Model", h.model);
  add(status, "Port", h.port);
  std::string topics;
  for (const auto & topic : h.topics) {
    topics += (topics.empty() ? "" : " ") + topic;
  }
  add(status, "Topics", topics);
  add(status, "Publish rate (Hz)", h.rate ? fixed(h.rate->hz, 1) : "not measured yet");
  add(status, "Configured rate (Hz)",
      h.configured_hz > 0.0 ? fixed(h.configured_hz, 1) : "unknown");
  add(status, "Messages published", std::to_string(h.published));
  add(status, "Samples dropped", std::to_string(h.total.dropped));
  add(status, "Parse errors", std::to_string(h.total.parse_errors));
  if (h.link) {
    add(status, "CRC errors", std::to_string(h.link->crc_errors));
    add(status, "Sequence gaps", std::to_string(h.link->seq_gaps));
    add(status, "Sequence errors reported by the board", std::to_string(h.link->device_seq_errors));
    add(status, "Header errors", std::to_string(h.link->header_errors));
    add(status, "Trash bytes", std::to_string(h.link->trash_bytes));
    add(status, "Packets received", std::to_string(h.link->rx_packets));
  }
  if (h.sync) {
    // Device clock - host clock (the SDK's monotonic one).
    add(status, "Time sync offset (s)", fixed(static_cast<double>(h.sync->offset_us) * 1e-6, 6));
    add(status, "Time sync round trip (ms)", fixed(static_cast<double>(h.sync->rtt_us) * 1e-3, 3));
    add(status, "Time sync age (s)", fixed(h.sync_age_s, 1));
  }
  if (h.watch.silent_after_s > 0.0) {
    add(status, "Restarted when silent for (s)", fixed(h.watch.silent_after_s, 1));
    add(status, "Restarts", std::to_string(h.watch.restarts));
    add(status, "Recoveries", std::to_string(h.watch.recoveries));
    add(status, "Last restart", h.watch.last.empty() ? "none" : h.watch.last);
  }
  if (h.mcu_celsius) {add(status, "MCU temperature (C)", fixed(*h.mcu_celsius, 1));}
  if (h.calibration) {
    const auto & c = *h.calibration;
    add(status, "BNO055 calibration (system gyro accel mag, 0-3)",
            std::to_string(c.system) + " " + std::to_string(c.gyro) + " " +
        std::to_string(c.accel) + " " +
                std::to_string(c.mag));
  }
  return status;
}

}  // namespace depz_sensors
