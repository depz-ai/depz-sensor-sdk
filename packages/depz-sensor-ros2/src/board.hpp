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


// One DEPZ board inside the node: opened, configured, its stream pumped into
// its publishers on a thread of its own, closed. A board that is unplugged
// ends its own pump (the SDK's stream end, contract 07 §3); the node reaps it.
//
// A board may also be a capture (`replay` parameter): the SDK's replay link in
// place of the port. It runs the same calls; its stream starts once each of
// its topics has a subscriber, is published at the pace of the capture's
// device timestamps, and ends — like an unplugged board — with the capture.
//
// Besides its family's topics a board publishes the extra ones its parameters
// switch on (settings.hpp: temperatures, signal and status images, further
// peaks, a point cloud, linear acceleration and gravity); none by default.
//
// A live board that stays on USB and delivers nothing is restarted by watch()
// (watchdog.hpp says when): closed, opened and started again as start() does
// it, into the publishers it has. A restart that fails leaves the board closed
// — down() — until the next one.
//
// health() is the board's snapshot of itself for /diagnostics (diagnostics.hpp).
#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <depz/device.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/range.hpp>
#include <sensor_msgs/msg/temperature.hpp>

#include "depz_sensors/diagnostics.hpp"
#include "depz_sensors/settings.hpp"
#include "depz_sensors/stamps.hpp"
#include "depz_sensors/trail.hpp"
#include "depz_sensors/watchdog.hpp"

namespace depz_sensors
{

// The node's parameters every board reads; a board's sensor parameters are its
// BoardConfig (settings.hpp).
struct BoardSettings
{
  std::string topic_prefix = "depz";
  std::string frame_prefix;
  rclcpp::QoS qos = rclcpp::SensorDataQoS();
  bool time_sync = true;    // device clock -> host clock; false: arrival time
  // the node publishes /diagnostics: a live board reads its MCU temperature for it
  bool diagnostics = true;
};

// A capture opened for replay (open_replay): the device, probed and with its
// class attached, on the SDK's replay link. `link` is that link, owned by
// `dev` — valid while `dev` is open.
struct Replay
{
  std::unique_ptr<depz::Device> dev;
  depz_link * link = nullptr;
};

// Open the .depzrec capture `path` for replay, as depz::open_on_link() opens a
// link: the contract 02 §4.4 probe on it (GET_NAME_ACTIVE_SOFTWARE,
// GET_DEVICE_NAME, GET_SERIAL — `timeout` each; the last two may stay
// unanswered), then the resolved class on the same connection. The probe's
// answers go to `info` (open_on_link keeps them to itself, and asking again
// would leave the capture's sequence); `info.port` is `path`. Throws
// depz::Error: an unreadable or damaged file, no answer, a bootloader.
Replay open_replay(
  const std::string & path, std::chrono::milliseconds timeout,
  depz::DeviceInfo & info);

class Board {
public:
  // `info` from the probe; `name` the board's stable name (naming.hpp);
  // `model` its model token; `config` its sensor parameters; `replay` the
  // opened capture of a replayed board (none: a live board, opened by
  // start() on info.port).
  Board(
    rclcpp::Node & node, depz::DeviceInfo info, std::string name, std::string model,
    const BoardSettings & settings, BoardConfig config, Replay replay = {});
  ~Board();
  Board(const Board &) = delete;
  Board & operator=(const Board &) = delete;

  // Open the board, configure its sensor, make its publishers and start the
  // pump. Throws when any step fails — ParameterError (naming the parameter)
  // for a value the board cannot take, depz::Error, std::exception; the
  // board is then closed again.
  void start();
  // Stop the pump, stop the sensor and close the board; the log says which
  // of the two happened to a board that was streaming. Idempotent.
  void stop();
  // True once the board's stream ended by itself (unplugged, lost, the
  // capture played to its end).
  bool ended() const {return ended_.load();}
  // Restart the board when its stream has been silent for too long and a
  // restart is due (watchdog.hpp); one log line per restart. The node's
  // thread, now and then; never throws.
  void watch();
  // A restart failed: the board is closed, its port free, until the next one.
  bool down() const {return !ended() && !dev_;}
  // A board that is down is gone from USB: it ended, `cause` says why.
  void lose(const std::string & cause);
  // The board now, for its diagnostic status: a lost board (ended()) says the
  // cause. The counters' growth and the rate are measured from call to call —
  // one caller, the node's thread (which also calls stop()).
  BoardHealth health();

  const std::string & name() const {return name_;}
  const std::string & model() const {return model_;}
  const depz::DeviceInfo & info() const {return info_;}
  // The topics this board publishes (for the startup log).
  const std::vector<std::string> & topics() const {return topics_;}
  // The QoS its publishers actually have, in words: "best effort, depth 5".
  std::string qos_text() const;

private:
  template<class T>
  void pump(depz::Subscription<T> sub, std::function<void(const T &)> publish);
  template<class T>
  void run(depz::Stream<T> & stream, std::function<void(const T &)> publish);
  // The header of a sample the board took at `device_us` of its clock;
  // `stream`: which of the board's independently stamped kinds of sample
  // (stamps.hpp).
  std_msgs::msg::Header header(std::uint64_t device_us, int stream = 0);
  // Replay: wait until each topic has a subscriber (false: stopped first).
  bool await_subscribers();
  // Replay: hold the sample stamped `device_us` until its turn.
  void pace(std::uint64_t device_us);
  void resync_if_due();
  // A live board's MCU temperature, read now and then for health() and,
  // when it has the topic, published.
  void read_temperature_if_due();
  // The stream ended by itself: keep the cause for health(), then say ended().
  void end(const std::string & cause);
  // What start() and a restart share: open the port, configure the sensor,
  // start the pump and the stream. Throws as start() does; the board is then
  // closed again.
  void open();
  // Stop the pump, stop the sensor, close the port; `quiet`: without the log
  // lines of stop() (a restart says its own).
  void close(bool quiet);
  std::string topic(const std::string & leaf);
  // The publisher `pub` of the board's topic <board>/<leaf>: made the first
  // time, kept over a restart.
  template<class Msg>
  void advertise(typename rclcpp::Publisher<Msg>::SharedPtr & pub, const std::string & leaf);
  // The same for an extra topic.
  template<class Msg>
  void extra(typename rclcpp::Publisher<Msg>::SharedPtr & pub, const std::string & leaf);
  // The sensor's own temperature, when the board publishes it.
  void publish_temperature(const std_msgs::msg::Header & header, double celsius);
  // Run an SDK call that sets `parameter`: a value the SDK refuses
  // (depz::ArgumentError) is a ParameterError naming the parameter.
  template<class Call>
  void apply(const char * parameter, Call && call);
  void start_sr04(depz::Sr04 & d);
  void start_vl53l4cd(depz::Vl53l4cd & d);
  void start_vl53lx(depz::Vl53lx & d);
  void start_multizone(depz::Vl53l8 & d);
  void start_tmf8829(depz::Tmf8829 & d);
  void start_bno055(depz::Bno055 & d);
  void start_bno086(depz::Bno086 & d);

  rclcpp::Node & node_;
  depz::DeviceInfo info_;
  std::string name_, model_, frame_id_;
  BoardSettings settings_;
  BoardConfig config_;
  std::unique_ptr<depz::Device> dev_;
  std::thread thread_;
  std::atomic<bool> stop_{false}, ended_{false};
  std::function<void()> begin_, stop_sensor_;    // start / stop the sensor's stream
  depz_link * replay_ = nullptr;                 // the replay link of a replayed board
  // start() is through: the pump thread may call begin_
  std::atomic<bool> armed_{false};
  std::atomic<bool> streaming_{false};           // begin_ went through: the sensor streams
  ReplayPace pace_;    // the pump thread's
  std::vector<std::string> topics_;
  std::chrono::steady_clock::time_point next_sync_{};
  // the board's clock against the host's; start()'s, then the pump thread's
  BoardClock clock_;
  SampleStamps stamps_;    // the pump thread's

  // For health(). Written by the pump thread, read by the node's:
  std::atomic<std::uint64_t> published_{0};    // messages on the main topic
  std::atomic<double> mcu_celsius_;            // NaN: not read
  std::atomic<int> calibration_{-1};           // BNO055 CALIB_STAT of the last sample; -1: none
  // when the main topic last got a message, ns of the steady clock; 0: never
  std::atomic<std::int64_t> last_message_ns_{0};
  std::mutex cause_lock_;
  std::string cause_;    // why the stream ended
  std::chrono::steady_clock::time_point next_temperature_{};
  // The node's thread only:
  double configured_hz_ = 0.0;    // the rate the parameters ask of the main topic
  // the time between two items of the stream the parameters ask for, s; 0: unknown
  double period_s_ = 0.0;
  Watchdog watchdog_;
  // what came from the board, for the log line of a silence (trail.hpp); events from the reader
  Trail trail_;
  bool silence_told_ = false;    // the silence that lasts was described in the log
  // dropped + parse errors of the board's stream
  std::function<HealthCounters()> stream_counters_;
  RateMeter meter_;
  std::optional<HealthCounters> reported_;    // the counters of the previous health()

  rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr range_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr depth_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<sensor_msgs::msg::MagneticField>::SharedPtr mag_pub_;
  // The extra topics: each null unless its parameter is on.
  rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr mcu_temperature_pub_,
    temperature_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr signal_pub_, status_pub_;
  // peak 2, 3, ...
  std::vector<rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr> peak_pubs_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr points_pub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3Stamped>::SharedPtr linear_acceleration_pub_,
    gravity_pub_;
  std::vector<rclcpp::PublisherBase::SharedPtr> extras_;    // all of them
};

}  // namespace depz_sensors
