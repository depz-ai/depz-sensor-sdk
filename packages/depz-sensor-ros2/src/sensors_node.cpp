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


// depz_sensors node: finds the DEPZ boards on USB, opens each with the DEPZ
// C++ SDK and publishes its measurements on standard sensor_msgs topics named
// after the board's model and USB serial number. Once a second it publishes
// each board's health on /diagnostics (parameter `diagnostics`).
//
// The node is an rclcpp component, depz_sensors::SensorsNode: a component
// container loads it from this library (sensors_composed.launch.py), and the
// `sensors_node` executable (src/main.cpp) makes the same node in a process of
// its own.
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <depz/device.hpp>
#include <depz/usb_ids.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "board.hpp"
#include "depz_sensors/access.hpp"
#include "depz_sensors/diagnostics.hpp"
#include "depz_sensors/naming.hpp"
#include "depz_sensors/settings.hpp"
#include "sensors_node.hpp"

namespace depz_sensors
{

namespace
{

std::string lower(std::string s)
{
  std::transform(
    s.begin(), s.end(), s.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  return s;
}

// A sensor parameter's value <-> the ROS parameter value.
rclcpp::ParameterValue to_ros(const ParamValue & value)
{
  struct Ros
  {
    rclcpp::ParameterValue operator()(std::monostate) const {return {};}
    rclcpp::ParameterValue operator()(bool b) const {return rclcpp::ParameterValue(b);}
    rclcpp::ParameterValue operator()(std::int64_t i) const {return rclcpp::ParameterValue(i);}
    rclcpp::ParameterValue operator()(double d) const {return rclcpp::ParameterValue(d);}
    rclcpp::ParameterValue operator()(const std::string & s) const
    {
      return rclcpp::ParameterValue(s);
    }
  };
  return std::visit(Ros{}, value);
}

ParamValue from_ros(const rclcpp::ParameterValue & value)
{
  switch (value.get_type()) {
    case rclcpp::ParameterType::PARAMETER_BOOL:
      return value.get<bool>();
    case rclcpp::ParameterType::PARAMETER_INTEGER:
      return value.get<std::int64_t>();
    case rclcpp::ParameterType::PARAMETER_DOUBLE:
      return value.get<double>();
    case rclcpp::ParameterType::PARAMETER_STRING:
      return value.get<std::string>();
    default:
      return std::monostate{};    // a list: no sensor parameter is one
  }
}

const char * kPermissionHint =
  "no read/write access — add your user to the 'dialout' group (sudo usermod -aG dialout $USER, "
  "then log in again) or install the DEPZ udev rule (see the depz_sensors README)";

// A board that did not answer — the probe, or a request of its start — is
// asked once more after this pause: seen on the bench as a rare miss of one
// board among fourteen that answered the next time.
constexpr auto kRetryPause = std::chrono::milliseconds(500);

// The longest probe_timeout_ms: a minute per port is beyond any board's answer.
constexpr std::int64_t kLongestProbeMs = 60'000;

// How often each board is looked at for a silence (watchdog.hpp).
constexpr auto kWatchEvery = std::chrono::milliseconds(200);

// Does a board of this SDK class take over a second to start? Measured on the
// bench of 2026-11-11 (14 boards, 64 start-ups): a multizone VL53L5 / L7 / L8
// loads its sensor's firmware and a BNO085 / BNO086 restarts its sensor hub —
// medians 1.2 .. 1.6 s each; every other board 0.03 .. 0.27 s (nine of them
// 1.4 s together, of the 8.5 s the fourteen take one after another).
bool slow_to_start(const std::string & sensor_class)
{
  return sensor_class == "bno086" || sensor_class.rfind("vl53l5",
        0) == 0 || sensor_class.rfind("vl53l7", 0) == 0 ||
         sensor_class.rfind("vl53l8", 0) == 0;
}

// Is `e` the SDK's "the board did not answer in time"?
bool no_answer(const std::exception & e)
{
  return dynamic_cast<const depz::TimeoutError *>(&e) != nullptr ||
         dynamic_cast<const depz::NoDeviceError *>(&e) != nullptr;
}

}  // namespace

class SensorsNode : public rclcpp::Node {
public:
  // The name is depz_sensors unless the options rename it (a launch file's
  // `name`, a container's load request).
  explicit SensorsNode(const rclcpp::NodeOptions & options)
  : Node("depz_sensors", options)
  {
    // Every parameter is read when the node starts and is read-only from
    // then on: a `ros2 param set` is refused, not taken and left without effect.
    rcl_interfaces::msg::ParameterDescriptor read_only;
    read_only.read_only = true;
    auto_discover_ = declare_parameter<bool>("auto_discover", true, read_only);
    serial_number_ = declare_parameter<std::string>("serial_number", "", read_only);
    device_path_ = declare_parameter<std::string>("device_path", "", read_only);
    const auto aliases =
      declare_parameter<std::vector<std::string>>("aliases", std::vector<std::string>{}, read_only);
    aliases_ = parse_aliases(aliases);
    hot_plug_ = declare_parameter<bool>("hot_plug", false, read_only);
    probe_timeout_ms_ = declare_parameter<int>("probe_timeout_ms", 300, read_only);
    if (probe_timeout_ms_ < 1 || probe_timeout_ms_ > kLongestProbeMs) {
      throw std::invalid_argument("probe_timeout_ms must be 1.." + std::to_string(kLongestProbeMs) +
                                        " (milliseconds a port is given to answer), not " +
                                        std::to_string(probe_timeout_ms_));
    }
    for (const auto & path :
      declare_parameter<std::vector<std::string>>("replay", std::vector<std::string>{}, read_only))
    {
      if (!path.empty()) {replay_.push_back(path);}      // YAML's [""] for "no replay"
    }
    std::sort(replay_.begin(), replay_.end());      // names must not depend on the order given

    settings_.topic_prefix = declare_parameter<std::string>("topic_prefix", "depz", read_only);
    check_topic_prefix();
    settings_.frame_prefix = declare_parameter<std::string>("frame_prefix", "", read_only);
    const std::string profile = declare_parameter<std::string>("qos", "sensor_data", read_only);
    const QosChoice qos = qos_choice(profile, declare_parameter<int>("qos_depth", 0, read_only));
    settings_.qos =
      qos.reliable ? rclcpp::QoS(rclcpp::KeepLast(static_cast<std::size_t>(qos.depth))).reliable() :
      rclcpp::SensorDataQoS().keep_last(static_cast<std::size_t>(qos.depth));
    settings_.time_sync = declare_parameter<bool>("time_sync", true, read_only);
    settings_.diagnostics = declare_parameter<bool>("diagnostics", true, read_only);
    // The topic every diagnostics tool reads, whatever the node's namespace.
    if (settings_.diagnostics) {
      diagnostics_pub_ = create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics",
          10);
    }
    // The sensor parameters: one value for every board of a family. What the
    // node was given is taken as a board's own is (set_param): either kind of
    // number that is the parameter's value — ROS itself refuses a node given
    // `silent_restart_after_s:=0`, an integer for a real number — and a wrong
    // type in the node's words. Declared in the parameter's own type ...
    const auto & passed = get_node_parameters_interface()->get_parameter_overrides();
    for (const auto & spec : sensor_params()) {
      const auto own = passed.find(spec.name);
      if (own != passed.end()) {
        set_param(family_, spec, from_ros(own->second), spec.name);
      }
      declare_parameter(spec.name, to_ros(get_param(family_, spec)), read_only, true);
    }
    // ... and a board's own, boards.<board name>.<parameter>: taken from what
    // the node was given (a board's name is only known once it is found),
    // and declared so that `ros2 param` shows them.
    std::map<std::string, ParamValue> given;
    for (const auto & [name, value] : passed) {
      if (name.rfind("boards.", 0) == 0) {
        given[name] = from_ros(declare_parameter(name, value, read_only));
      } else if (!has_parameter(name) && name.rfind("qos_overrides.", 0) != 0) {
        // ROS takes a parameter nobody declares in silence, and the node would run on the default.
        RCLCPP_WARN(get_logger(),
            "parameter %s: the node has no such parameter — it is not used (mistyped?)",
                            name.c_str());
      }
    }
    overrides_ = board_overrides(given);

    if (!auto_discover_ && serial_number_.empty() && device_path_.empty()) {
      throw std::invalid_argument(
                "auto_discover is false: "
                "set serial_number or device_path to choose the board to open");
    }

    if (!replay_.empty()) {
      open_replays();
    } else {
      discover(true);
    }
    // interrupted while starting: the destructor closes what is open
    if (!rclcpp::ok()) {return;}
    if (boards_.empty()) {
      if (!replay_.empty()) {
        RCLCPP_WARN(get_logger(), "no capture of the `replay` parameter could be opened");
      } else {
        RCLCPP_WARN(get_logger(), "no DEPZ board found%s",
            hot_plug_ ? " — waiting for one (hot_plug)" : "");
      }
    }
    // A board's parameters that no board took: a mistyped name, or a board that is not there.
    for (const auto & [board, parameters] : overrides_) {
      if (named_.count(board)) {continue;}
      RCLCPP_WARN(get_logger(), "parameter boards.%s.%s: no board is called %s%s", board.c_str(),
                        parameters.begin()->first.c_str(), board.c_str(),
                        hot_plug_ ? " yet (hot_plug)" : " — its parameters are not used");
    }
    timer_ = create_wall_timer(std::chrono::seconds(1), [this] {tick();});
    watch_timer_ = create_wall_timer(kWatchEvery, [this] {watch();});
  }

  ~SensorsNode() override
  {
    timer_.reset();
    watch_timer_.reset();
    boards_.clear();      // each Board stops its sensor and closes the port
  }

private:
  // topic_prefix goes in front of every topic: one that is no ROS name would
  // fail each board's publishers one by one.
  void check_topic_prefix() const
  {
    const std::string & prefix = settings_.topic_prefix;
    if (prefix.empty()) {return;}    // no prefix: <board name>/<topic>
    try {
      rclcpp::expand_topic_or_service_name(prefix + "/board/range", get_name(), get_namespace());
    } catch (const std::exception &) {
      throw std::invalid_argument(
                "topic_prefix \"" + prefix +
                "\" is not a ROS name: letters, digits and '_', '/' between names, "
                "no name starting with a digit");
    }
  }

  // A board found: what its probe answered and, for a capture, the opened replay.
  struct Found
  {
    depz::DeviceInfo info;
    Replay replay;
  };

  // Is this board one the parameters ask for?
  bool wanted(const depz::DeviceInfo & info) const
  {
    if (!device_path_.empty() && info.port != device_path_) {return false;}
    if (!serial_number_.empty()) {
      const auto s = lower(serial_number_);
      if (lower(info.usb_serial) != s && lower(info.serial_number) != s) {return false;}
    }
    return true;
  }

  // The candidate ports not already held: known DEPZ USB ids — or, when a
  // device_path is asked for, that port alone, whatever its id (no other
  // board is opened to be asked who it is) —, with their USB identity. A
  // device_path that is a link (/dev/serial/by-id/..., a udev symlink) is the
  // port it points at, under the path as given: the board keeps the USB
  // serial and the USB id its model name may come from.
  std::vector<depz::SerialPortInfo> candidate_ports()
  {
    std::vector<depz::SerialPortInfo> ports;
    std::string target = device_path_;
    if (!device_path_.empty()) {
      std::error_code ec;
      const auto resolved = std::filesystem::canonical(device_path_, ec);
      if (!ec) {target = resolved.string();}
    }
    bool path_listed = false;
    for (const auto & p : depz::list_serial_ports()) {
      const bool asked = !device_path_.empty() && (p.port == device_path_ || p.port == target);
      path_listed = path_listed || asked;
      if (device_path_.empty() ? !depz::is_known_depz_usb(p.vid, p.pid) : !asked) {continue;}
      depz::SerialPortInfo port = p;
      if (asked) {port.port = device_path_;}
      if (held_.count(port.port) || failed_.count(port.port)) {continue;}
      ports.push_back(port);
    }
    if (!device_path_.empty() && !path_listed && !held_.count(device_path_) &&
      !failed_.count(device_path_))
    {
      // a path the listing lacks (a pseudo-terminal)
      ports.push_back({device_path_, std::nullopt, std::nullopt, ""});
    }
    std::sort(
      ports.begin(), ports.end(), [](const auto & a, const auto & b) {
        return a.port < b.port;
      });
    return ports;
  }

  // The `replay` parameter: each capture is one board, in place of the USB
  // boards — probed, selected and named as they are.
  void open_replays()
  {
    std::vector<Found> found;
    for (const auto & path : replay_) {
      if (!rclcpp::ok()) {return;}
      Found f;
      try {
        f.replay = open_replay(path, std::chrono::milliseconds(probe_timeout_ms_), f.info);
      } catch (const std::exception & e) {
        RCLCPP_ERROR(get_logger(), "replay %s: could not open: %s", path.c_str(), e.what());
        continue;
      }
      if (wanted(f.info)) {found.push_back(std::move(f));}
    }
    open_boards(std::move(found), true);
  }

  void discover(bool first)
  {
    std::vector<Found> found;
    for (const auto & candidate : candidate_ports()) {
      if (!rclcpp::ok()) {return;}      // Ctrl+C during start-up: open nothing more
      const std::string & port = candidate.port;
      if (::access(port.c_str(), R_OK | W_OK) != 0) {
        // Not said of a port that has just appeared, unless the next look finds it so too
        // (access.hpp).
        if (denied_.report(port, first)) {
          RCLCPP_ERROR(get_logger(), "%s: %s", port.c_str(), kPermissionHint);
        }
        continue;
      }
      denied_.clear(port);
      std::optional<depz::DeviceInfo> info;
      try {
        info = depz::probe_port(port, std::chrono::milliseconds(probe_timeout_ms_));
        // No answer at start-up: once more after a pause (with hot_plug the next tick asks again
        // anyway).
        if (!info && first) {
          std::this_thread::sleep_for(kRetryPause);
          if (!rclcpp::ok()) {return;}
          info = depz::probe_port(port, std::chrono::milliseconds(probe_timeout_ms_));
          if (info) {
            RCLCPP_WARN(get_logger(),
                "%s: no answer to the probe within %d ms — answered at the second try",
                                    port.c_str(), static_cast<int>(probe_timeout_ms_));
          }
        }
      } catch (const std::exception & e) {
        if (warned_.insert(port).second) {
          RCLCPP_WARN(get_logger(), "%s: probe failed: %s", port.c_str(), e.what());
        }
        continue;
      }
      if (!info) {
        // The port is a DEPZ board's by its USB id (or the device_path asked for): say that it is
        // left out.
        if (warned_.insert(port).second) {
          RCLCPP_WARN(get_logger(),
              "%s (USB serial %s): no answer to the probe within %d ms%s — %s",
                                port.c_str(),
              candidate.usb_serial.empty() ? "none" : candidate.usb_serial.c_str(),
                                static_cast<int>(probe_timeout_ms_), first ? ", twice" : "",
                                hot_plug_ ? "asked again every second (hot_plug)" :
              "the board is left out");
        }
        continue;
      }
      // The probe asks the board; the USB identity comes from the port listing.
      if (info->usb_serial.empty()) {info->usb_serial = candidate.usb_serial;}
      if (!info->usb_vid) {info->usb_vid = candidate.vid;}
      if (!info->usb_pid) {info->usb_pid = candidate.pid;}
      if (info->mode == depz::DeviceMode::Bootloader) {
        if (warned_.insert(port).second) {
          RCLCPP_WARN(get_logger(), "%s is in bootloader mode (%s): flash or reset it to use it",
              port.c_str(),
                                info->software_name.c_str());
        }
        continue;
      }
      if (!wanted(*info)) {continue;}
      found.push_back({*info, {}});
    }
    open_boards(std::move(found), first);
  }

  // Name the boards of one discovery and start them.
  void open_boards(std::vector<Found> found, bool first)
  {
    // also a hot-plug discovery: it runs while no board is open
    if (!auto_discover_ && found.size() > 1) {
      RCLCPP_WARN(get_logger(), "%zu boards match the selection; opening the first (%s)",
          found.size(),
                        found.front().info.port.c_str());
      found.resize(1);
    }

    std::vector<BoardId> ids;
    std::vector<std::string> models;
    // The order the boards are started in: those that start fast first, in
    // the order found (by port) — a board that takes over a second holds
    // up only the other slow ones. Names do not depend on it.
    std::vector<std::size_t> fast, slow;
    for (const auto & f : found) {
      const auto & info = f.info;
      const auto cls = depz::resolve_sensor_class(info.software_name, info.device_name,
          info.usb_vid, info.usb_pid);
      (slow_to_start(cls) ? slow : fast).push_back(models.size());
      models.push_back(model_name(cls, info.device_name,
          depz::usb_model_hint(info.usb_vid, info.usb_pid)));
      // A capture's "port" is its file: without a serial it is named after the file's stem.
      const std::string port =
        f.replay.dev ? std::filesystem::path(info.port).stem().string() : info.port;
      ids.push_back({models.back(), info.usb_serial, info.serial_number, port});
    }
    // A board joining later must not take a running board's name.
    std::vector<std::string> taken;
    for (const auto & b : boards_) {taken.push_back(b->name());}
    const auto names = board_names(ids, aliases_, taken);
    std::vector<std::size_t> order = fast;
    order.insert(order.end(), slow.begin(), slow.end());
    std::size_t started = 0;
    for (const std::size_t i : order) {
      const depz::DeviceInfo info = found[i].info;
      const std::string & name = names[i];
      if (!rclcpp::ok()) {return;}
      named_.insert(name);
      // A live board that does not answer a request of its start is
      // started once more; a capture answers the same every time.
      const bool live = !found[i].replay.dev;
      std::unique_ptr<Board> board;
      for (int attempt = 1; !board; ++attempt) {
        try {
          // The family's parameters with the board's own on top; one that
          // does not fit stops this board only.
          board = std::make_unique<Board>(*this, info, name, models[i], settings_,
                                                    board_config(family_, name, overrides_),
              std::move(found[i].replay));
          board->start();
        } catch (const std::exception & e) {
          board.reset();
          // Ctrl+C under the start (its publishers cannot be made on a context shut down): not a
          // failure.
          if (!rclcpp::ok()) {return;}
          if (live && attempt == 1 && no_answer(e)) {
            RCLCPP_WARN(get_logger(), "%s (%s, %s): %s — trying once more", name.c_str(),
                models[i].c_str(),
                                    info.port.c_str(), e.what());
            std::this_thread::sleep_for(kRetryPause);
            if (!rclcpp::ok()) {return;}
            continue;
          }
          RCLCPP_ERROR(get_logger(), "%s (%s, %s): could not start: %s", name.c_str(),
              models[i].c_str(),
                                 info.port.c_str(), e.what());
          failed_.insert(info.port);
          break;
        }
      }
      if (!board) {continue;}
      std::string topics;
      for (const auto & t : board->topics()) {topics += " " + t;}
      RCLCPP_INFO(get_logger(), "%s: %s on %s (USB serial %s) ->%s (%s)", name.c_str(),
          models[i].c_str(),
                        info.port.c_str(),
          info.usb_serial.empty() ? "none" : info.usb_serial.c_str(),
                        topics.c_str(), board->qos_text().c_str());
      held_.insert(info.port);
      boards_.push_back(std::move(board));
      ++started;
    }
    // The start-up is through (not said when Ctrl+C cut it short).
    if (first && !found.empty()) {
      RCLCPP_INFO(get_logger(), "started %zu of %zu boards found", started, found.size());
    }
  }

  // A board that stays on USB and went silent is restarted, one board at a
  // time, on this thread: the other boards publish on their own.
  void watch()
  {
    for (const auto & board : boards_) {
      if (!rclcpp::ok()) {return;}
      board->watch();
    }
  }

  // A board that is closed between two restarts has no stream to end when it
  // is unplugged: it is lost once its port is gone, or is another board's (a
  // port the listing lacks — a link given as device_path — is its own).
  void lose_unplugged()
  {
    if (std::none_of(boards_.begin(), boards_.end(), [](const auto & b) {return b->down();})) {
      return;
    }
    std::map<std::string, std::string> present;    // port -> USB serial
    try {
      for (const auto & p : depz::list_serial_ports()) {present[p.port] = p.usb_serial;}
    } catch (const std::exception &) {
      return;    // not known now: the next tick asks again
    }
    for (const auto & board : boards_) {
      if (!board->down()) {continue;}
      const std::string & path = board->info().port;
      const auto port = present.find(path);
      if (::access(path.c_str(), F_OK) != 0 ||
        (port != present.end() && port->second != board->info().usb_serial))
      {
        board->lose("unplugged while the node was restarting it");
      }
    }
  }

  void tick()
  {
    lose_unplugged();
    // The health of every board, then reap those whose stream ended
    // (unplugged): close them, drop their publishers. A board found ended
    // here is in this array as an error with the cause — at least once
    // before it goes. The port may come back as a new board (hot_plug).
    diagnostic_msgs::msg::DiagnosticArray diagnostics;
    for (auto it = boards_.begin(); it != boards_.end(); ) {
      const bool ended = (*it)->ended();        // before health(): what is reaped was reported lost
      if (diagnostics_pub_) {diagnostics.status.push_back(to_status(get_name(), (*it)->health()));}
      if (ended) {
        held_.erase((*it)->info().port);
        failed_.erase((*it)->info().port);
        warned_.erase((*it)->info().port);
        it = boards_.erase(it);
      } else {
        ++it;
      }
    }
    if (diagnostics_pub_) {
      diagnostics.header.stamp = now();
      diagnostics_pub_->publish(diagnostics);
    }
    if (!hot_plug_ || !replay_.empty()) {return;}    // a capture that ended does not come back
    if (!auto_discover_ && !boards_.empty()) {return;}
    try {
      // Ports that failed to start are retried only after they disappear.
      std::set<std::string> present;
      for (const auto & p : depz::list_serial_ports()) {present.insert(p.port);}
      // A device_path the listing lacks — a link to a port, a pseudo-terminal — is there
      // while its path is.
      if (!device_path_.empty() && ::access(device_path_.c_str(), F_OK) == 0) {
        present.insert(device_path_);
      }
      for (auto it = failed_.begin(); it != failed_.end(); ) {
        it = present.count(*it) ? std::next(it) : failed_.erase(it);
      }
      for (auto it = warned_.begin(); it != warned_.end(); ) {
        it = present.count(*it) ? std::next(it) : warned_.erase(it);
      }
      denied_.keep(present);
      discover(false);
    } catch (const std::exception & e) {
      // Out of a timer an exception would end the process — a container's other nodes with it. The
      // boards that run go on; the next tick looks again.
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 60000, "looking for new boards failed: %s",
          e.what());
    }
  }

  bool auto_discover_ = true, hot_plug_ = false;
  std::string serial_number_, device_path_;
  std::vector<std::string> replay_;
  std::map<std::string, std::string> aliases_;
  std::int64_t probe_timeout_ms_ = 300;
  BoardSettings settings_;
  SensorSettings family_;      // the sensor parameters of every board ...
  BoardOverrides overrides_;    // ... and those given for one board, by its name
  std::set<std::string> named_;    // every name a board was given, started or not
  std::vector<std::unique_ptr<Board>> boards_;
  std::set<std::string> held_, failed_, warned_;
  DeniedPorts denied_;  // ports found without read/write access
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnostics_pub_;
  rclcpp::TimerBase::SharedPtr timer_, watch_timer_;
};

std::shared_ptr<rclcpp::Node> make_sensors_node(const rclcpp::NodeOptions & options)
{
  return std::make_shared<SensorsNode>(options);
}

}  // namespace depz_sensors

RCLCPP_COMPONENTS_REGISTER_NODE(depz_sensors::SensorsNode)
