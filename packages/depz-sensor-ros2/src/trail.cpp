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


#include "depz_sensors/trail.hpp"

#include <cstdio>
#include <iterator>
#include <sstream>
#include <vector>

namespace depz_sensors
{

namespace
{

std::uint64_t grown(std::uint64_t now, std::uint64_t before)
{
  return now > before ? now - before : 0;
}

// The link's errors between two snapshots, in words; empty: none.
std::string errors(const TrailCounters & a, const TrailCounters & b)
{
  std::vector<std::string> parts;
  const auto add = [&parts](std::uint64_t n, const char * what) {
      if (n) {parts.push_back(std::to_string(n) + " " + what);}
    };
  add(grown(b.crc_errors, a.crc_errors), "CRC errors");
  add(grown(b.header_errors, a.header_errors), "bad headers");
  add(grown(b.trash_bytes, a.trash_bytes), "stray bytes");
  add(grown(b.seq_gaps, a.seq_gaps), "packets missed");
  add(grown(b.device_seq_errors, a.device_seq_errors), "sequence errors the board reported");
  add(grown(b.parse_errors, a.parse_errors), "reports that did not decode");
  std::string out;
  for (const auto & p : parts) {
    out += (out.empty() ? "" : ", ") + p;
  }
  return out;
}

std::string seconds(double s)
{
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.1f s", s);
  return buf;
}

}  // namespace

void Trail::tick(double now_s, const TrailCounters & counters)
{
  std::lock_guard<std::mutex> guard(lock_);
  if (!ticks_.empty() && now_s <= ticks_.back().first) {return;}
  ticks_.emplace_back(now_s, counters);
  // Ten seconds of lines need the snapshot a little more than ten seconds back.
  while (ticks_.front().first < now_s - kSeconds - 0.5) {ticks_.pop_front();}
}

void Trail::event(double now_s, std::string what)
{
  std::lock_guard<std::mutex> guard(lock_);
  events_.emplace_back(now_s, std::move(what));
  while (events_.size() > kEvents) {events_.pop_front();}
}

void Trail::reset()
{
  std::lock_guard<std::mutex> guard(lock_);
  ticks_.clear();
  events_.clear();
}

std::string Trail::story(double now_s, std::optional<double> last_item_s) const
{
  std::lock_guard<std::mutex> guard(lock_);
  std::ostringstream out;
  if (ticks_.size() < 2) {
    out << "  no count of what it sent: it was opened less than a second before";
  } else {
    out << "  what it sent, a line a second (samples published / packets / bytes from the board):";
    // A line a second: from the oldest snapshot, each next one at least a second later.
    std::size_t prev = 0;
    for (std::size_t i = 1; i < ticks_.size(); ++i) {
      if (ticks_[i].first - ticks_[prev].first < 1.0 - 1e-9) {continue;}
      const TrailCounters & a = ticks_[prev].second;
      const auto & [t1, b] = ticks_[i];
      out << "\n    " << seconds(t1 - now_s) << ": " << grown(b.published, a.published) <<
        " samples, " << grown(b.rx_packets, a.rx_packets) << " packets, " <<
        grown(b.rx_bytes, a.rx_bytes) << " bytes";
      const std::string e = errors(a, b);
      if (!e.empty()) {out << "; " << e;}
      prev = i;
    }
    // After the last sample: from the first snapshot past it (the oldest without one) to the
    // newest.
    auto from = ticks_.begin();
    if (last_item_s) {
      while (from != ticks_.end() && from->first <= *last_item_s) {++from;}
    }
    const std::string last = last_item_s ?
      "the last sample " + seconds(now_s - *last_item_s) + " ago" :
      std::string("no sample since it was opened");
    out << "\n  " << last;
    if (from == ticks_.end() || std::next(from) == ticks_.end()) {
      out << "; nothing counted after it yet";
    } else {
      const auto & [t_from, before] = *from;
      const auto & [t_to, after] = ticks_.back();
      const std::uint64_t packets = grown(after.rx_packets, before.rx_packets);
      out << "; from " << seconds(t_from - now_s) << " to " << seconds(t_to - now_s) << " " <<
        packets << " packets, " << grown(after.rx_bytes, before.rx_bytes) << " bytes — ";
      if (packets == 0) {
        out << "the board sent nothing";
      } else if (grown(after.published, before.published) == 0) {
        out << "the board sent, but no sample came of it";
        const std::string e = errors(before, after);
        if (!e.empty()) {out << " (" << e << ")";}
      } else {
        out << "it still delivered then";
      }
    }
  }
  if (events_.empty()) {
    out << "\n  no event from the board or the link in that time";
  } else {
    out << "\n  its last events:";
    for (const auto & [t, what] : events_) {
      out << "\n    " << seconds(t - now_s) << ": " << what;
    }
  }
  return out.str();
}

std::optional<std::string> describe(const depz::DeviceEvent & e)
{
  using T = depz::DeviceEvent::Type;
  char buf[160];
  switch (e.type) {
    case T::Temperature:
      return std::nullopt;
    case T::SequenceError:
      std::snprintf(buf, sizeof buf, "the board reported a sequence error (expected %u, got %u)",
        e.expected_seq, e.received_seq);
      return buf;
    case T::SeqGap:
      std::snprintf(buf, sizeof buf, "%u packets missed", e.missed);
      return buf;
    case T::CrcError:
      return std::string("a packet with a bad CRC");
    case T::Trash:
      std::snprintf(buf, sizeof buf, "%zu stray bytes", e.trash_len);
      return buf;
    case T::UnsolicitedStatus:
      if (e.is_hardware_fault()) {return std::string("the board reported a hardware fault");}
      std::snprintf(buf, sizeof buf, "the board sent status 0x%02x unasked", e.status);
      return buf;
    case T::Text:
      return "the board said: \"" + e.text.substr(0, 120) + "\"";
    case T::Disconnected:
      return "disconnected: " + e.text;
    case T::Tmf8829Interrupt:
      return std::string("a TMF8829 event");
  }
  return std::string("an event of another kind");
}

}  // namespace depz_sensors
