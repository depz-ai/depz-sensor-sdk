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


#include "board.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>

#include "depz_sensors/convert.hpp"
#include "depz_sensors/naming.hpp"

namespace depz_sensors
{

namespace
{

constexpr auto kPoll = std::chrono::milliseconds(200);
// A board's clock wanders: on the bench of 2026-11-13 its rate against the
// host's moved by hundreds of ppm within seconds. Synced every 10 s the
// mapping was up to 1.3 ms off, every 2 s 0.3 ms in 95 % of the time
// (stamps.hpp: BoardClock carries the rate on between two syncs).
constexpr auto kResyncEvery = std::chrono::seconds(2);
// The board refreshes its MCU temperature about twice a second; a status needs no more than this.
constexpr auto kTemperatureEvery = std::chrono::seconds(10);
// ... and the topic gets one a second.
constexpr auto kTemperatureTopicEvery = std::chrono::seconds(1);
constexpr auto kRequestTimeout = std::chrono::milliseconds(200);  // the SDK's default
// A replayed board's queue: the capture is decoded at once and published at
// its own pace, so the queue holds the whole stream (the default drops the oldest).
constexpr std::size_t kReplayQueue = 65536;
constexpr auto kReplayPoll = std::chrono::milliseconds(20);
std::int64_t steady_ns()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

double steady_s() {return static_cast<double>(steady_ns()) * 1e-9;}

// Has the replay link `raw` (owned by its device) served its last chunk? The
// C++ SDK asks a Link: a view of the handle, given back untouched.
bool replay_played(depz_link * raw)
{
  struct View
  {
    depz::Link link;
    ~View() {link.release();}
  } view{depz::Link::adopt(raw)};
  return view.link.replay_exhausted();
}

}  // namespace

Replay open_replay(
  const std::string & path, std::chrono::milliseconds timeout,
  depz::DeviceInfo & info)
{
  depz::Link link = depz::Link::replay(path);
  Replay replay;
  replay.link = link.c_handle();
  auto base = depz::Device::open(std::move(link));
  base->set_timeout(timeout);
  const depz::Identity identity = base->identify();
  info = depz::DeviceInfo{};
  info.port = path;
  info.mode = identity.mode;
  info.sensor_type = identity.sensor_type;
  info.software_name = identity.software_name;
  info.fw_version = identity.version;
  try {
    info.device_name = base->device_name();
  } catch (const depz::Error &) {
    info.device_name.clear();
  }
  try {
    info.serial_number = base->serial_number();
  } catch (const depz::Error &) {
    info.serial_number.clear();
  }
  // A capture has no USB id: the class comes from the answers alone.
  replay.dev = depz::attach_sensor_class(
        std::move(base),
      depz::resolve_sensor_class(info.software_name, info.device_name, std::nullopt, std::nullopt));
  replay.dev->set_timeout(kRequestTimeout);
  return replay;
}

Board::Board(
  rclcpp::Node & node, depz::DeviceInfo info, std::string name, std::string model,
  const BoardSettings & settings, BoardConfig config, Replay replay)
: node_(node),
  info_(std::move(info)),
  name_(std::move(name)),
  model_(std::move(model)),
  frame_id_(config.frame_id.empty() ? settings.frame_prefix + name_ : config.frame_id),
  settings_(settings),
  config_(std::move(config)),
  dev_(std::move(replay.dev)),
  replay_(replay.link),
  mcu_celsius_(std::numeric_limits<double>::quiet_NaN())
{
}

Board::~Board() {stop();}

std::string Board::topic(const std::string & leaf)
{
  std::string t = settings_.topic_prefix.empty() ? name_ : settings_.topic_prefix + "/" + name_;
  t += "/" + leaf;
  topics_.push_back(t);
  return t;
}

template<class Msg>
void Board::advertise(typename rclcpp::Publisher<Msg>::SharedPtr & pub, const std::string & leaf)
{
  if (!pub) {pub = node_.create_publisher<Msg>(topic(leaf), settings_.qos);}
}

template<class Msg>
void Board::extra(typename rclcpp::Publisher<Msg>::SharedPtr & pub, const std::string & leaf)
{
  if (pub) {return;}
  advertise<Msg>(pub, leaf);
  extras_.push_back(pub);
}

void Board::publish_temperature(const std_msgs::msg::Header & header, double celsius)
{
  if (temperature_pub_) {temperature_pub_->publish(to_temperature(header, celsius));}
}

std::string Board::qos_text() const
{
  const auto text = [](const auto & pub) {
      const rclcpp::QoS qos = pub->get_actual_qos();
      return std::string(qos.reliability() ==
        rclcpp::ReliabilityPolicy::Reliable ? "reliable" : "best effort") +
             ", depth " + std::to_string(qos.depth());
    };
  if (range_pub_) {return text(range_pub_);}
  if (depth_pub_) {return text(depth_pub_);}
  if (imu_pub_) {return text(imu_pub_);}
  return "";
}

std_msgs::msg::Header Board::header(std::uint64_t device_us, int stream)
{
  std_msgs::msg::Header h;
  h.frame_id = frame_id_;
  if (replay_) {pace(device_us);}
  // How long ago, on the host's clock, the board took the sample (nullopt: no time sync).
  const auto age_us = clock_.age_us(static_cast<std::int64_t>(depz::host_now_us()), device_us);
  const rclcpp::Time now = node_.now();
  // Not in the future, not before the stream's previous stamp (stamps.hpp).
  h.stamp = rclcpp::Time(stamps_.stamp_ns(now.nanoseconds(), age_us, stream), now.get_clock_type());
  return h;
}

bool Board::await_subscribers()
{
  RCLCPP_INFO(node_.get_logger(), "%s: the replay starts when each of its topics has a subscriber",
      name_.c_str());
  const auto subscribed = [](const auto & pub) {return !pub || pub->get_subscription_count() > 0;};
  while (!stop_.load()) {
    if (armed_.load() && subscribed(range_pub_) && subscribed(depth_pub_) && subscribed(imu_pub_) &&
      subscribed(mag_pub_) && std::all_of(extras_.begin(), extras_.end(), subscribed))
    {
      return true;
    }
    std::this_thread::sleep_for(kReplayPoll);
  }
  return false;
}

void Board::pace(std::uint64_t device_us)
{
  const auto now_us = [] {return steady_ns() / 1000;};
  const std::int64_t due_us = pace_.due_us(device_us, now_us());
  while (!stop_.load()) {
    const std::int64_t left_us = due_us - now_us();
    if (left_us <= 0) {break;}
    std::this_thread::sleep_for(
      std::min<std::chrono::microseconds>(std::chrono::microseconds(left_us), kReplayPoll));
  }
}

void Board::resync_if_due()
{
  if (!settings_.time_sync || replay_) {return;}  // a capture's clock answers nothing
  const auto now = std::chrono::steady_clock::now();
  if (now < next_sync_) {return;}
  next_sync_ = now + kResyncEvery;
  try {
    const depz::TimeSync sync = dev_->sync_time(3);
    clock_.synced(sync.offset_us, static_cast<std::int64_t>(sync.synced_at_host_us));
  } catch (const std::exception & e) {
    // Keep the last offset (or arrival time if none): a missed sync only
    // costs accuracy.
    RCLCPP_DEBUG(node_.get_logger(), "%s: time sync failed: %s", name_.c_str(), e.what());
  }
}

void Board::read_temperature_if_due()
{
  if (replay_) {return;}         // a capture holds no answer
  if (!armed_.load()) {return;}  // start() still talks to the board on its own thread
  if (!settings_.diagnostics && !mcu_temperature_pub_) {return;}
  const auto now = std::chrono::steady_clock::now();
  if (now < next_temperature_) {return;}
  next_temperature_ = now + (mcu_temperature_pub_ ? kTemperatureTopicEvery : kTemperatureEvery);
  try {
    const double celsius = dev_->read_mcu_temperature();
    mcu_celsius_.store(celsius);
    if (mcu_temperature_pub_) {
      // Stamped on arrival: the board refreshes the value about twice a second.
      std_msgs::msg::Header h;
      h.frame_id = frame_id_;
      h.stamp = node_.now();
      mcu_temperature_pub_->publish(to_temperature(h, celsius));
    }
  } catch (const std::exception & e) {
    // Keep the last reading: the status goes on without a fresh one.
    RCLCPP_DEBUG(node_.get_logger(), "%s: MCU temperature not read: %s", name_.c_str(), e.what());
  }
}

void Board::end(const std::string & cause)
{
  {
    std::lock_guard<std::mutex> guard(cause_lock_);
    cause_ = cause;
  }
  ended_.store(true);
}

BoardHealth Board::health()
{
  BoardHealth h;
  h.name = name_;
  h.model = model_;
  h.port = info_.port;
  h.hardware_id = info_.usb_serial.empty() ? info_.serial_number : info_.usb_serial;
  for (const char * name : {range_pub_ ? range_pub_->get_topic_name() : nullptr,
      depth_pub_ ? depth_pub_->get_topic_name() : nullptr,
      imu_pub_ ? imu_pub_->get_topic_name() : nullptr,
      mag_pub_ ? mag_pub_->get_topic_name() : nullptr})
  {
    if (name) {h.topics.emplace_back(name);}    // as resolved: with the node's namespace
  }
  for (const auto & pub : extras_) {
    h.topics.emplace_back(pub->get_topic_name());
  }
  if (ended_.load()) {
    std::lock_guard<std::mutex> guard(cause_lock_);
    h.lost = cause_;
  }
  h.streaming = streaming_.load();
  h.published = published_.load();
  h.configured_hz = configured_hz_;
  // The rate is measured while the board streams — from its start, and not past its loss — and
  // while it is closed between two restarts: it publishes nothing then, and the rate says so.
  if ((h.streaming || down()) && !h.lost) {
    meter_.add(std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count(),
                   h.published);
  }
  h.watch = watchdog_.state(steady_s());
  h.rate = meter_.rate();
  if (dev_) {
    try {
      if (stream_counters_) {h.total = stream_counters_();}
      const depz::LinkStats link = dev_->stats();
      h.link = link;
      h.total.crc_errors = link.crc_errors;
      h.total.seq_gaps = link.seq_gaps;
      h.total.device_seq_errors = link.device_seq_errors;
      if (settings_.time_sync && !replay_) {
        h.sync = dev_->time_sync();
        if (h.sync) {
          h.sync_age_s = static_cast<double>(depz::host_now_us() - h.sync->synced_at_host_us) *
            1e-6;
        }
      }
    } catch (const depz::Error &) {
      // The device is ending under this call: what was read stays.
    }
  }
  if (reported_) {
    // A counter that restarted (a TMF8829's parse errors, with its stream) has not grown.
    const auto grown = [](std::uint64_t now, std::uint64_t before) {
        return now > before ? now - before : 0;
      };
    h.grown.dropped = grown(h.total.dropped, reported_->dropped);
    h.grown.parse_errors = grown(h.total.parse_errors, reported_->parse_errors);
    h.grown.crc_errors = grown(h.total.crc_errors, reported_->crc_errors);
    h.grown.seq_gaps = grown(h.total.seq_gaps, reported_->seq_gaps);
    h.grown.device_seq_errors = grown(h.total.device_seq_errors, reported_->device_seq_errors);
  }
  reported_ = h.total;
  if (const double celsius = mcu_celsius_.load(); !std::isnan(celsius)) {h.mcu_celsius = celsius;}
  if (const int calibration = calibration_.load(); calibration >= 0) {
    h.calibration = depz::bno055::CalibStatus::unpack(static_cast<std::uint8_t>(calibration));
  }
  return h;
}

template<class T>
void Board::pump(depz::Subscription<T> sub, std::function<void(const T &)> publish)
{
  if (replay_) {
    if (!await_subscribers()) {return;}
    try {
      begin_();
      streaming_.store(true);
    } catch (const std::exception & e) {
      RCLCPP_ERROR(node_.get_logger(), "%s (%s): the replay could not start its stream: %s",
          name_.c_str(),
                         info_.port.c_str(), e.what());
      end(std::string("the replay could not start its stream: ") + e.what());
      return;
    }
  }
  std::string cause = "the stream ended";
  while (!stop_.load()) {
    resync_if_due();
    read_temperature_if_due();
    // Asked before the wait: every chunk served, then a quiet poll — the
    // stream is over.
    const bool played = replay_ && replay_played(replay_);
    std::optional<T> item;
    try {
      item = sub.next(kPoll);
    } catch (const std::exception & e) {
      RCLCPP_ERROR(node_.get_logger(), "%s: %s", name_.c_str(), e.what());
      cause = e.what();
      break;
    }
    if (item) {
      const std::uint64_t before = published_.load();
      try {
        publish(*item);
      } catch (const std::exception & e) {
        RCLCPP_WARN_THROTTLE(node_.get_logger(), *node_.get_clock(), 5000,
            "%s: a sample was dropped: %s",
                                     name_.c_str(), e.what());
      }
      if (published_.load() != before) {last_message_ns_.store(steady_ns());}
      continue;
    }
    if (played && !stop_.load()) {
      RCLCPP_ERROR(node_.get_logger(),
          "%s (%s) disconnected: the replay ended — its topics stop publishing",
                         name_.c_str(), info_.port.c_str());
      cause = "the replay ended";
      break;
    }
    if (sub.closed()) {
      if (!stop_.load()) {
        cause = sub.lost().value_or(cause);
        RCLCPP_ERROR(node_.get_logger(), "%s (%s) disconnected: %s — its topics stop publishing",
                             name_.c_str(), info_.port.c_str(), cause.c_str());
      }
      break;
    }
  }
  end(cause);
}

template<class T>
void Board::run(depz::Stream<T> & stream, std::function<void(const T &)> publish)
{
  // Subscribe before the sensor starts, so the first sample is not missed.
  auto sub = stream.subscribe(replay_ ? kReplayQueue : 0);
  // The stream lives as long as its device: stop() drops this before the device.
  stream_counters_ = [&stream] {
      HealthCounters counters;
      for (const auto dropped : stream.dropped_counts()) {
        counters.dropped += dropped;
      }
      counters.parse_errors = stream.parse_errors();
      return counters;
    };
  thread_ = std::thread([this, s = std::move(sub), publish = std::move(publish)]() mutable {
        pump<T>(std::move(s), std::move(publish));
    });
}

template<class Call>
void Board::apply(const char * parameter, Call && call)
{
  try {
    call();
  } catch (const depz::ArgumentError & e) {
    config_.refuse(parameter, e.what());
  }
}

void Board::start_sr04(depz::Sr04 & d)
{
  config_.require_kind(kSr04, model_);
  // The SDK takes the period in microseconds, in 32 bits.
  const auto period_us = static_cast<std::uint32_t>(config_.whole("sr04_period_ms", 1,
      4'294'967)) * 1000U;
  const auto spec = *range_spec("hcsr04");
  advertise<sensor_msgs::msg::Range>(range_pub_, "range");
  apply("sr04_period_ms", [&] {d.set_sample_period_us(period_us);});
  configured_hz_ = 1e6 / period_us;
  run<depz::Sr04Measurement>(d.measurements(), [this, spec](const depz::Sr04Measurement & m) {
      range_pub_->publish(to_range(header(m.timestamp_us), spec, m));
      ++published_;
    });
  begin_ = [&d] {d.start();};
  stop_sensor_ = [&d] {d.stop();};
}

void Board::start_vl53l4cd(depz::Vl53l4cd & d)
{
  config_.require_kind(kVl53l4cd, model_);
  // range_mode is not a VL53L4CD's (it has no distance modes): the family's value passes it by.
  const auto budget_ms = static_cast<std::uint32_t>(
    config_.whole("range_budget_ms", 0, std::numeric_limits<std::uint32_t>::max()));
  const auto spec = *range_spec("vl53l4cd");
  advertise<sensor_msgs::msg::Range>(range_pub_, "range");
  d.init();
  apply("range_budget_ms", [&] {d.set_range_timing(budget_ms, 0);});
  configured_hz_ = 1000.0 / budget_ms;    // continuous: one measurement per budget
  run<depz::Vl53l4cdMeasurement>(d.measurements(),
    [this, spec](const depz::Vl53l4cdMeasurement & m) {
      range_pub_->publish(to_range(header(m.timestamp_us), spec, m));
      ++published_;
    });
  begin_ = [&d] {d.start_ranging();};
  stop_sensor_ = [&d] {d.stop_ranging();};
}

void Board::start_vl53lx(depz::Vl53lx & d)
{
  config_.require_kind(kVl53lx, model_);
  d.init();
  // The driver's own modes and limits, known once it is bound to the product.
  const auto mode = config_.range_mode(d.modes());
  const auto [lo, hi] = d.budget_range();
  const int budget_ms = config_.whole("range_budget_ms", lo, hi);
  advertise<sensor_msgs::msg::Range>(range_pub_, "range");
  // Only some budgets may exist within the limits (VL53L1), and they move with the mode: the SDK
  // says.
  apply("range_budget_ms", [&] {d.configure(budget_ms, 0, mode);});
  configured_hz_ = 1000.0 / budget_ms;    // continuous: one measurement per budget
  const std::string product = sanitize(d.product().value_or(model_));
  auto spec = range_spec(product).value_or(*range_spec("vl53l1cx"));
  if (const auto reach = d.driver_reach_mm(); reach > 0) {spec.max_range = reach / 1000.0F;}
  run<depz::Vl53lxMeasurement>(d.measurements(), [this, spec](const depz::Vl53lxMeasurement & m) {
      range_pub_->publish(to_range(header(m.timestamp_us), spec, m));
      ++published_;
    });
  begin_ = [&d] {d.start_ranging();};
  stop_sensor_ = [&d] {d.stop_ranging();};
}

void Board::start_multizone(depz::Vl53l8 & d)
{
  config_.require_kind(kMultizone, model_);
  // What must be known before the SDK is called; the limits are the SDK's.
  const int zones = config_.sensor.tof_resolution;
  const auto hz = static_cast<std::uint8_t>(config_.whole("tof_frequency_hz", 0, 255));
  const auto mode = config_.tof_ranging_mode();
  const auto integration_ms = static_cast<std::uint32_t>(
    config_.whole("tof_integration_time_ms", 0, std::numeric_limits<std::uint32_t>::max()));
  const auto order = config_.tof_target_order();
  const int sharpener = config_.whole("tof_sharpener_percent", -1, 255);
  advertise<sensor_msgs::msg::Image>(depth_pub_, "depth");
  if (config_.sensor.tof_signal) {extra<sensor_msgs::msg::Image>(signal_pub_, "depth/signal");}
  if (config_.sensor.tof_status) {extra<sensor_msgs::msg::Image>(status_pub_, "depth/status");}
  // The field of view the zones spread over is the model's.
  const auto fov = zone_fov(model_);
  if (config_.sensor.point_cloud) {
    if (!fov) {config_.refuse("point_cloud", "the field of view of a " + model_ + " is not known");}
    extra<sensor_msgs::msg::PointCloud2>(points_pub_, "points");
  }
  if (config_.sensor.sensor_temperature) {
    extra<sensor_msgs::msg::Temperature>(temperature_pub_, "temperature");
  }
  d.init();
  apply("tof_resolution", [&] {d.set_resolution(zones);});
  apply("tof_frequency_hz", [&] {d.set_ranging_frequency_hz(hz);});
  if (mode) {apply("tof_ranging_mode", [&] {d.set_ranging_mode(*mode);});}
  if (integration_ms != 0) {
    apply(
      "tof_integration_time_ms", [&] {
        d.set_integration_time_ms(integration_ms);
      });
  }
  if (order) {apply("tof_target_order", [&] {d.set_target_order(*order);});}
  if (sharpener >= 0) {
    apply(
      "tof_sharpener_percent", [&] {
        d.set_sharpener_percent(static_cast<std::uint8_t>(sharpener));
      });
  }
  configured_hz_ = hz;
  // A frame takes four integration times at 8x8, one at 4x4, and about 10 ms
  // (measured, the README's `tof_integration_time_ms`): the sensor gives
  // fewer frames than `hz` when that is longer.
  if (integration_ms != 0) {period_s_ = ((zones == 64 ? 4 : 1) * integration_ms + 10) * 1e-3;}
  run<depz::Vl53l8LiveFrame>(d.frames(), [this, fov](const depz::Vl53l8LiveFrame & f) {
      const auto h = header(f.frame.timestamp_us);
      depth_pub_->publish(to_image(h, f.frame));
      ++published_;
      if (signal_pub_) {signal_pub_->publish(to_signal_image(h, f.frame));}
      if (status_pub_) {status_pub_->publish(to_status_image(h, f.frame));}
      if (points_pub_) {points_pub_->publish(to_point_cloud(h, f.frame, *fov));}
      publish_temperature(h, f.frame.silicon_temp_degc);
    });
  begin_ = [&d] {d.start_ranging();};
  stop_sensor_ = [&d] {d.stop_ranging();};
}

void Board::start_tmf8829(depz::Tmf8829 & d)
{
  config_.require_kind(kTmf8829, model_);
  const std::string preset = config_.tmf8829_preset();
  depz::Tmf8829Options o;
  o.period_ms = config_.sensor.tmf8829_period_ms;
  // How many peaks the sensor reports per zone, so how many images: known before the publishers are
  // made.
  o.nr_peaks = config_.whole("tmf8829_peaks", 1, 4);
  advertise<sensor_msgs::msg::Image>(depth_pub_, "depth");
  peak_pubs_.resize(static_cast<std::size_t>(o.nr_peaks - 1));
  for (int peak = 2; peak <= o.nr_peaks; ++peak) {
    extra<sensor_msgs::msg::Image>(peak_pubs_[static_cast<std::size_t>(peak - 2)],
      "depth/peak_" + std::to_string(peak));
  }
  if (config_.sensor.point_cloud) {extra<sensor_msgs::msg::PointCloud2>(points_pub_, "points");}
  if (config_.sensor.sensor_temperature) {
    extra<sensor_msgs::msg::Temperature>(temperature_pub_, "temperature");
  }
  d.init();
  apply("tmf8829_period_ms", [&] {d.configure(preset, o);});
  if (o.period_ms > 0) {configured_hz_ = 1000.0 / o.period_ms;}
  run<depz::Tmf8829Measurement>(d.measurements(), [this](const depz::Tmf8829Measurement & m) {
      const auto h = header(static_cast<std::uint64_t>(m.timestamp_us));
      depth_pub_->publish(to_image(h, m));
      ++published_;
      for (std::size_t i = 0; i < peak_pubs_.size(); ++i) {
        peak_pubs_[i]->publish(to_image(h, m, static_cast<unsigned>(i + 1)));
      }
      if (points_pub_) {points_pub_->publish(to_point_cloud(h, m));}
      publish_temperature(h, m.temperature_c());
    });
  begin_ = [&d] {d.start();};
  stop_sensor_ = [&d] {d.stop();};
}

void Board::start_bno055(depz::Bno055 & d)
{
  config_.require_kind(kBno055, model_);
  const auto mode = config_.bno055_mode();
  // The chip fuses at 100 Hz: a faster stream would repeat samples.
  const double period_ms = std::max(10.0, std::floor(1000.0 / config_.rate("imu_rate_hz")));
  if (period_ms > std::numeric_limits<std::uint16_t>::max()) {
    config_.refuse("imu_rate_hz",
        "too slow for the BNO055's stream period, which is whole milliseconds in 16 bits");
  }
  advertise<sensor_msgs::msg::Imu>(imu_pub_, "imu");
  // The "imu" mode fuses without the magnetometer and leaves it off: no field to publish.
  const bool magnetometer = mode != depz::bno055::OprMode::Imu;
  if (magnetometer) {
    advertise<sensor_msgs::msg::MagneticField>(mag_pub_, "mag");
  }
  using Vector = geometry_msgs::msg::Vector3Stamped;
  if (config_.sensor.imu_linear_acceleration) {
    extra<Vector>(linear_acceleration_pub_, "linear_acceleration");
  }
  if (config_.sensor.imu_gravity) {extra<Vector>(gravity_pub_, "gravity");}
  if (config_.sensor.sensor_temperature) {
    extra<sensor_msgs::msg::Temperature>(temperature_pub_, "temperature");
  }
  // ROS units: m/s², rad/s, radians; Windows orientation (the datasheet's).
  d.configure(mode, depz::bno055::Units(false, true, true, false, false));
  configured_hz_ = 1000.0 / period_ms;
  run<depz::Bno055Sample>(d.samples(), [this](const depz::Bno055Sample & s) {
      const auto h = header(s.timestamp_us);
      if (s.calibration) {calibration_.store(s.calibration->pack());}
      if (auto imu = to_imu(h, s)) {
        imu_pub_->publish(*imu);
        ++published_;
      }
      if (mag_pub_) {
        if (auto mag = to_magnetic_field(h, s)) {mag_pub_->publish(*mag);}
      }
      // The full block the stream reads carries the fusion's outputs and the chip's temperature.
      if (linear_acceleration_pub_) {
        if (auto v = to_linear_acceleration(h, s)) {linear_acceleration_pub_->publish(*v);}
      }
      if (gravity_pub_) {
        if (auto v = to_gravity(h, s)) {gravity_pub_->publish(*v);}
      }
      if (temperature_pub_) {
        if (auto t = to_temperature(h, s)) {temperature_pub_->publish(*t);}
      }
    });
  begin_ = [this, &d, period = static_cast<std::uint16_t>(period_ms)] {
      apply("imu_rate_hz", [&] {d.start_stream(period);});
    };
  stop_sensor_ = [&d] {d.stop_stream();};
}

void Board::start_bno086(depz::Bno086 & d)
{
  namespace b = depz::bno086;
  config_.require_kind(kBno086, model_);
  const auto orientation = config_.bno086_orientation();
  const double hz = config_.rate("imu_rate_hz");
  const double mag_hz = config_.rate("bno086_mag_rate_hz", true);
  advertise<sensor_msgs::msg::Imu>(imu_pub_, "imu");
  if (mag_hz > 0.0) {
    advertise<sensor_msgs::msg::MagneticField>(mag_pub_, "mag");
  }
  // The hub's own: acceleration without gravity, and gravity — a report each, at the Imu's rate.
  using Vector = geometry_msgs::msg::Vector3Stamped;
  if (config_.sensor.imu_linear_acceleration) {
    extra<Vector>(linear_acceleration_pub_, "linear_acceleration");
  }
  if (config_.sensor.imu_gravity) {extra<Vector>(gravity_pub_, "gravity");}
  d.hardware_reset();
  auto assembler = std::make_shared<Bno086Assembler>();
  configured_hz_ = hz;
  run<depz::Bno086Report>(d.reports(), [this, assembler](const depz::Bno086Report & r) {
      // Each kind of report has its own clock of samples: stamped in its own order.
      auto out = assembler->feed(header(static_cast<std::uint64_t>(r.timestamp_us), r.sensor_id),
        r);
      if (out.imu) {
        imu_pub_->publish(*out.imu);
        ++published_;
      }
      if (out.mag && mag_pub_) {mag_pub_->publish(*out.mag);}
      if (out.linear_acceleration && linear_acceleration_pub_) {
        linear_acceleration_pub_->publish(*out.linear_acceleration);
      }
      if (out.gravity && gravity_pub_) {gravity_pub_->publish(*out.gravity);}
    });
  // Enable a report at the rate of `parameter`; the hub grants the rate it
  // can run, and one outside the SDK's band (rate_ok) is said in the log.
  const auto enable = [this, &d](b::SensorId sensor, const char * what, const char * parameter,
    double rate) {
      b::Feature granted{};
      apply(parameter, [&] {granted = d.enable(sensor, rate);});
      if (!depz::Bno086::rate_ok(static_cast<std::uint32_t>(std::llround(1e6 / rate)), granted)) {
        RCLCPP_WARN(node_.get_logger(),
          "%s: parameter %s = %g: the sensor hub runs its %s at %g Hz", name_.c_str(),
                        config_.written(parameter).c_str(), rate, what,
                        granted.interval_us ? 1e6 / granted.interval_us : 0.0);
      }
    };
  // A report beside the Imu's three, for the topic of `pub`.
  const auto enable_for = [this, enable](const rclcpp::PublisherBase::SharedPtr & pub,
    b::SensorId sensor,
    const char * what, const char * parameter, double rate) {
      if (!pub) {return;}
      try {
        enable(sensor, what, parameter, rate);
      } catch (const depz::TimeoutError & e) {
        // A capture need not hold the report (the Imu needs none of
        // them): go on without it. A live board has it.
        if (!replay_) {throw;}
        RCLCPP_WARN(node_.get_logger(), "%s: the %s was not enabled (%s): no %s messages",
          name_.c_str(), what,
                        e.what(), pub->get_topic_name());
      }
    };
  begin_ = [this, enable, enable_for, orientation, hz, mag_hz] {
      enable(b::SensorId::Accelerometer, "accelerometer", "imu_rate_hz", hz);
      enable(b::SensorId::Gyroscope, "gyroscope", "imu_rate_hz", hz);
      enable(orientation, "orientation", "imu_rate_hz", hz);
      enable_for(mag_pub_, b::SensorId::Magnetometer, "magnetometer", "bno086_mag_rate_hz", mag_hz);
      enable_for(linear_acceleration_pub_, b::SensorId::LinearAcceleration, "linear acceleration",
        "imu_rate_hz", hz);
      enable_for(gravity_pub_, b::SensorId::Gravity, "gravity vector", "imu_rate_hz", hz);
    };
  stop_sensor_ = [this, &d, orientation] {
      // The first disable that fails ends the stop, the other reports stay on: a hub that does not
      // answer one would make the shutdown wait out a timeout for each of the rest, and closing
      // matters more.
      d.disable(orientation);
      d.disable(b::SensorId::Accelerometer);
      d.disable(b::SensorId::Gyroscope);
      if (mag_pub_) {d.disable(b::SensorId::Magnetometer);}
      if (linear_acceleration_pub_) {d.disable(b::SensorId::LinearAcceleration);}
      if (gravity_pub_) {d.disable(b::SensorId::Gravity);}
    };
}

void Board::start()
{
  // A capture is not watched: it cannot be opened again where it stopped.
  const double silent_s = config_.rate("silent_restart_after_s", true);
  const bool live = !replay_;
  try {
    open();
  } catch (...) {
    close(false);
    throw;
  }
  if (configured_hz_ > 0.0) {period_s_ = std::max(period_s_, 1.0 / configured_hz_);}
  watchdog_ = Watchdog(live ? silent_after_s(silent_s, period_s_) : 0.0);
  watchdog_.started(steady_s());
}

void Board::watch()
{
  if (ended()) {return;}
  const double now = steady_s();
  const std::int64_t last = last_message_ns_.load();
  const bool due = watchdog_.look(
    now, last ? std::optional<double>(static_cast<double>(last) * 1e-9) : std::nullopt);
  if (const auto recovery = watchdog_.take_recovery()) {
    RCLCPP_INFO(node_.get_logger(), "%s (%s): publishes again after %.1f s of silence and %d %s",
      name_.c_str(), info_.port.c_str(), recovery->silent_s, recovery->restarts,
      recovery->restarts == 1 ? "restart" : "restarts");
    silence_told_ = false;
  }
  if (dev_ && !replay_) {
    TrailCounters c;
    c.published = published_.load();
    try {
      const depz::LinkStats link = dev_->stats();
      c.rx_packets = link.rx_packets;
      c.rx_bytes = link.rx_bytes;
      c.crc_errors = link.crc_errors;
      c.header_errors = link.header_errors;
      c.trash_bytes = link.trash_bytes;
      c.seq_gaps = link.seq_gaps;
      c.device_seq_errors = link.device_seq_errors;
      if (stream_counters_) {c.parse_errors = stream_counters_().parse_errors;}
      trail_.tick(now, c);
    } catch (const depz::Error &) {
      // The device is ending under this call.
    }
  }
  // A board whose stream ended is the node's to let go, not to restart.
  if (!due || ended() || !rclcpp::ok()) {return;}
  const double silent_s = watchdog_.state(now).silent_for_s.value_or(0.0);
  if (!silence_told_) {
    // Once per silence, before its first restart (which starts the trail again): what came before.
    RCLCPP_WARN(node_.get_logger(), "%s (%s): silent for %.1f s; what came before:\n%s",
      name_.c_str(), info_.port.c_str(), silent_s,
      trail_.story(now, last ? std::optional<double>(static_cast<double>(last) * 1e-9) :
      std::nullopt).c_str());
    silence_told_ = true;
  }
  char reason[64];
  std::snprintf(reason, sizeof reason, "no sample for %.1f s", silent_s);
  std::string error;
  try {
    close(true);
    stop_.store(false);
    ended_.store(false);
    clock_ = BoardClock{};    // the board's clock may have restarted with it
    open();
    meter_ = RateMeter{};     // the rate is the new stream's, not one over the silence
  } catch (const std::exception & e) {
    error = e.what();
    close(true);
    stop_.store(false);
    ended_.store(false);
  }
  const double done = steady_s();
  // Every restart while the waits grow; then one now and then.
  if (!watchdog_.restarted(done, reason, error)) {return;}
  const WatchState state = watchdog_.state(done);
  const double wait_s = watchdog_.next_restart_s().value_or(done) - done;
  if (error.empty()) {
    RCLCPP_WARN(node_.get_logger(),
      "%s (%s): no sample for %.1f s — restarted (restart %d); if it stays silent, again in %.0f s",
      name_.c_str(), info_.port.c_str(), silent_s, state.restarts,
      std::max(wait_s, state.silent_after_s));
  } else if (state.failed_in_row >= kErrorAfterFailed) {
    RCLCPP_ERROR(node_.get_logger(),
      "%s (%s): no sample for %.1f s — restart %d failed: %s — the next in %.0f s",
      name_.c_str(), info_.port.c_str(), silent_s, state.restarts, error.c_str(), wait_s);
  } else {
    RCLCPP_WARN(node_.get_logger(),
      "%s (%s): no sample for %.1f s — restart %d failed: %s — the next in %.0f s",
      name_.c_str(), info_.port.c_str(), silent_s, state.restarts, error.c_str(), wait_s);
  }
}

void Board::lose(const std::string & cause)
{
  RCLCPP_ERROR(node_.get_logger(), "%s (%s) disconnected: %s — its topics stop publishing",
    name_.c_str(), info_.port.c_str(), cause.c_str());
  end(cause);
}

void Board::open()
{
  if (!replay_) {
    depz::OpenOptions opt;
    opt.port = info_.port;
    dev_ = depz::open_device(opt);
    // A new device: its counters start again. Its events go to the trail (on its reader thread);
    // the callback lives as long as the device, which this board outlives.
    trail_.reset();
    dev_->events().on([this](const depz::DeviceEvent & e) {
        if (auto what = describe(e)) {trail_.event(steady_s(), std::move(*what));}
      });
  }
  {
    if (settings_.time_sync && !replay_) {
      const depz::TimeSync sync = dev_->sync_time(5);
      clock_.synced(sync.offset_us, static_cast<std::int64_t>(sync.synced_at_host_us));
      next_sync_ = std::chrono::steady_clock::now() + kResyncEvery;
    }
    auto * raw = dev_.get();
    if (auto * sr04 = dynamic_cast<depz::Sr04 *>(raw)) {
      start_sr04(*sr04);
    } else if (auto * l4cd = dynamic_cast<depz::Vl53l4cd *>(raw)) {
      start_vl53l4cd(*l4cd);
    } else if (auto * lx = dynamic_cast<depz::Vl53lx *>(raw)) {
      start_vl53lx(*lx);
    } else if (auto * multizone = dynamic_cast<depz::Vl53l8 *>(raw)) {
      start_multizone(*multizone);
    } else if (auto * tmf = dynamic_cast<depz::Tmf8829 *>(raw)) {
      start_tmf8829(*tmf);
    } else if (auto * bno055 = dynamic_cast<depz::Bno055 *>(raw)) {
      start_bno055(*bno055);
    } else if (auto * bno086 = dynamic_cast<depz::Bno086 *>(raw)) {
      start_bno086(*bno086);
    } else {
      throw std::runtime_error("no ROS mapping for this board (" + info_.software_name + ")");
    }
    if (config_.sensor.mcu_temperature) {
      if (replay_) {
        RCLCPP_INFO(node_.get_logger(),
            "%s: no MCU temperature topic: a capture does not answer the request",
                            name_.c_str());
      } else {
        extra<sensor_msgs::msg::Temperature>(mcu_temperature_pub_, "mcu_temperature");
      }
    }
    // A live board streams from now on; a replayed one once it has its
    // subscribers (the pump thread starts it).
    if (!replay_) {
      begin_();
      streaming_.store(true);
    }
    armed_.store(true);
  }
}

void Board::stop() {close(false);}

void Board::close(bool quiet)
{
  stop_.store(true);
  if (thread_.joinable()) {thread_.join();}
  if (dev_) {
    // A board that streams is told to stop — unless it is gone (unplugged),
    // or is a capture played to its end, which holds no answer.
    const bool gone = dev_->closed() || dev_->lost().has_value() ||
      (replay_ && replay_played(replay_));
    bool stopped = false;
    std::string unstopped;
    if (streaming_.load() && stop_sensor_ && !gone) {
      try {
        stop_sensor_();
        stopped = true;
      } catch (const std::exception & e) {
        unstopped = e.what();          // closing is what matters
      }
    }
    try {
      dev_->close();
    } catch (const std::exception &) {
    }
    stream_counters_ = nullptr;      // it reads the device's stream
    dev_.reset();
    if (quiet) {
      // a restart says its own line
    } else if (stopped) {
      RCLCPP_INFO(node_.get_logger(), "%s: stopped and closed", name_.c_str());
    } else if (!unstopped.empty()) {
      RCLCPP_WARN(node_.get_logger(), "%s: closed; its sensor did not confirm the stop: %s",
          name_.c_str(),
                        unstopped.c_str());
    } else if (streaming_.load()) {
      RCLCPP_INFO(node_.get_logger(), "%s: closed", name_.c_str());
    }
  }
  replay_ = nullptr;    // the link went with the device
  begin_ = nullptr;
  stop_sensor_ = nullptr;
  armed_.store(false);
  streaming_.store(false);
}

}  // namespace depz_sensors
