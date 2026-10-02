// Live hardware: links, the device core, discovery and the sensor classes —
// a C++17 wrapper over the C SDK's depz_sensor_io.h (contract 07). Errors are
// exceptions; handles are move-only and close themselves; callbacks are
// std::function and run on the device's reader thread (keep them short, never
// block, never destroy their device from one).
//
// Sensor classes so far: Sr04, Vl53l4cd, Vl53l8 (VL53L8CX/CH, VL53L5CX, VL53L7CX/CH),
// Bno055, Bno086 (BNO085 / BNO086), Vl53lx (the VL53L 1D family).
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "depz/bno055.hpp"
#include "depz/bno086.hpp"
#include "depz/common.hpp"
#include "depz/framing.hpp"
#include "depz/identity.hpp"
#include "depz/span.hpp"
#include "depz/sr04.hpp"
#include "depz/vl53l4.hpp"
#include "depz/vl53l7.hpp"
#include "depz/vl53l8.hpp"
#include "depz/vl53lx.hpp"

extern "C" {
struct depz_link;
struct depz_device;
struct depz_stream;
}

namespace depz {

// ── errors ──────────────────────────────────────────────────────────────────

// Base of every live-layer error; `code()` is the C SDK's depz_err value.
class Error : public std::runtime_error {
public:
    Error(int code, const std::string& what) : std::runtime_error(what), code_(code) {}
    int code() const noexcept { return code_; }

private:
    int code_;
};

class ArgumentError : public Error { using Error::Error; };
class IoError : public Error { using Error::Error; };             // the OS refused
class DeviceLostError : public Error { using Error::Error; };    // closed / unplugged
class TimeoutError : public Error { using Error::Error; };
class ProtocolError : public Error { using Error::Error; };      // a reply that does not parse
class NoDeviceError : public Error { using Error::Error; };
class WrongTypeError : public Error { using Error::Error; };
class ReplayMismatchError : public Error { using Error::Error; };
class BootloaderModeError : public Error { using Error::Error; };

// The device answered a non-OK RPT_STATUS.
class StatusError : public Error {
public:
    StatusError(int code, const std::string& what, std::uint8_t cmd, Status status)
        : Error(code, what), cmd_(cmd), status_(status) {}
    std::uint8_t cmd() const noexcept { return cmd_; }
    Status status() const noexcept { return status_; }

private:
    std::uint8_t cmd_;
    Status status_;
};

// ERR_BUSY, or the same opcode already in flight.
class BusyError : public StatusError { using StatusError::StatusError; };

// ── links ───────────────────────────────────────────────────────────────────

// A byte pipe to a device: serial port, loopback, .depzrec record/replay.
class Link {
public:
    Link() = default;
    ~Link();
    Link(Link&& o) noexcept : l_(std::exchange(o.l_, nullptr)) {}
    Link& operator=(Link&& o) noexcept;
    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;

    // The CDC-ACM port, opened exclusively ("/dev/ttyACM0", "COM7").
    static Link serial(const std::string& port);
    // Causal replay of a capture; `strict_tx` throws ReplayMismatchError on
    // any write that differs from the recorded requests.
    static Link replay(const std::string& path, bool strict_tx = false, bool realtime = false);
    // Tee `inner` into a new .depzrec file. `header_extra_json`: the inside of
    // a JSON object merged into the header (`"port": "live"`), or "".
    static Link recording(Link inner, const std::string& path,
                          const std::string& header_extra_json = "");
    // Two in-memory ends: what one writes, the other reads.
    static std::pair<Link, Link> loopback_pair();
    // Take ownership of a C link (a custom depz_link_new() one, say).
    static Link adopt(depz_link* l) noexcept { return Link(l); }

    // Next chunk (empty on timeout); DeviceLostError once closed.
    bytes read(std::chrono::milliseconds timeout);
    void write(byte_span data);
    void close();
    bool closed() const;
    std::string name() const;
    // Replay links: every recorded rx chunk has been served.
    bool replay_exhausted() const;

    explicit operator bool() const noexcept { return l_ != nullptr; }
    depz_link* c_handle() const noexcept { return l_; }
    depz_link* release() noexcept { return std::exchange(l_, nullptr); }

private:
    explicit Link(depz_link* l) : l_(l) {}
    depz_link* l_ = nullptr;
};

// ── streams ─────────────────────────────────────────────────────────────────

namespace detail {
// A callback's storage: it stays alive until its device is destroyed (the
// reader may still be running it right after an unsubscribe).
struct CallbackHolder {
    virtual ~CallbackHolder() = default;
};

struct StreamHandle {
    depz_stream* s = nullptr;
    ~StreamHandle();
    bool next(void* item, std::chrono::milliseconds timeout, bool& closed);
    std::uint64_t dropped() const;
};
}  // namespace detail

// A bounded drop-oldest pull subscription (contract 07 §3). Registered at
// creation: nothing produced after it is missed.
template <class T>
class Stream {
public:
    Stream() = default;
    explicit Stream(depz_stream* s) : h_(std::make_unique<detail::StreamHandle>()) { h_->s = s; }

    // The next item, or nullopt on timeout — and once the device is closed
    // and the stream drained (then closed() is true).
    std::optional<T> next(std::chrono::milliseconds timeout = std::chrono::milliseconds(200)) {
        if (!h_) return std::nullopt;
        return T::pull(*h_, timeout, closed_);
    }
    bool closed() const noexcept { return closed_; }
    std::uint64_t dropped_count() const { return h_ ? h_->dropped() : 0; }

private:
    std::unique_ptr<detail::StreamHandle> h_;
    bool closed_ = false;
};

// ── device core ─────────────────────────────────────────────────────────────

struct LinkStats {
    std::uint64_t tx_packets, rx_packets, tx_bytes, rx_bytes;
    std::uint64_t crc_errors, header_errors, trash_bytes;
    std::uint64_t seq_gaps, device_seq_errors;
};

struct TimeSync {
    std::int64_t offset_us;  // device clock - host clock
    std::int64_t rtt_us;
    std::uint64_t synced_at_host_us;
};

struct DeviceEvent {
    enum class Type { SequenceError, CrcError, Trash, UnsolicitedStatus, Text, Temperature, Disconnected };
    Type type;
    std::uint8_t cmd = 0, seq = 0;
    std::uint8_t expected_seq = 0, received_seq = 0;
    bool by_device = false;               // SequenceError reported by the device
    std::uint8_t status = 0;              // UnsolicitedStatus (ERR_HARDWARE_FAULT = a fault)
    std::uint64_t timestamp_us = 0;       // Temperature
    double celsius = 0.0;
    std::size_t trash_len = 0;            // Trash: total length; `data` = the first bytes
    bytes data;
    std::string text;                     // Text; Disconnected: the reason

    bool is_hardware_fault() const {
        return type == Type::UnsolicitedStatus && status == static_cast<std::uint8_t>(Status::ErrHardwareFault);
    }

    // Stream<DeviceEvent> plumbing.
    static std::optional<DeviceEvent> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                           bool& closed);
};

// Host monotonic clock, µs — the host side of all time-sync math.
std::uint64_t host_now_us();

// A connection to one DEPZ device in application mode (contract 02 / 07).
class Device {
public:
    // A plain device on a port / a link, no identity probe.
    static std::unique_ptr<Device> open(const std::string& port);
    static std::unique_ptr<Device> open(Link link);

    virtual ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Stop the reader and close the link (the destructor does it too).
    void close();
    bool closed() const;
    std::string port() const;
    // nullopt until identified (open_device / promote).
    std::optional<SensorType> sensor_type() const;
    void set_timeout(std::chrono::milliseconds t);
    LinkStats stats() const;

    std::string device_name();
    std::string software_name();
    std::string serial_number();
    double read_mcu_temperature();  // °C, cached by the device ~2 Hz
    CrcType payload_crc_type();
    void set_payload_crc_type(CrcType t);  // device->host payload CRC
    SyncPinConfig sync_pin(std::uint8_t pin);
    void set_sync_pin(const SyncPinConfig& c);
    void reset();             // the device ACKs, then reboots; the link drops
    void enter_bootloader();  // reboot into the bootloader; closes this device

    TimeSync sync_time(int samples = 5);
    std::optional<TimeSync> time_sync() const;
    std::int64_t to_host_time_us(std::uint64_t device_us) const;

    // Identify (GET_NAME_ACTIVE_SOFTWARE) and return the matching class,
    // consuming this plain device — what open_device() does after its probe.
    static std::unique_ptr<Device> promote(std::unique_ptr<Device> dev);

    // Escape hatch: send `cmd` and wait for RPT_STATUS(cmd, OK) ...
    void request_ok(std::uint8_t cmd, byte_span payload = {},
                    std::optional<std::chrono::milliseconds> timeout = std::nullopt);
    // ... or for the first packet `match` accepts (reader thread, device lock
    // held: copy what you need, nothing else). Returns that packet's payload.
    bytes request(std::uint8_t cmd, byte_span payload,
                  std::function<bool(std::uint8_t rpt, byte_span payload)> match,
                  std::optional<std::chrono::milliseconds> timeout = std::nullopt);
    void send(std::uint8_t cmd, byte_span payload = {});

    // Events: callbacks stay registered until unsubscribed or the device dies.
    using Unsubscribe = std::function<void()>;
    Unsubscribe on_event(std::function<void(const DeviceEvent&)> cb);
    Stream<DeviceEvent> events(std::size_t maxsize = 256);

    depz_device* c_handle() const noexcept { return d_; }

protected:
    explicit Device(depz_device* d) : d_(d) {}
    // The C handle, or DeviceLostError after close().
    depz_device* handle() const;
    void keep(std::unique_ptr<detail::CallbackHolder> h);
    depz_device* d_;

private:
    std::mutex held_lock_;
    std::vector<std::unique_ptr<detail::CallbackHolder>> held_;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── SR04 ultrasonic ranger (contract 03) ────────────────────────────────────

struct Sr04Measurement {
    std::uint64_t timestamp_us;  // device µs
    std::uint16_t echo_time_us;  // ECHO_TIMEOUT = no echo
    bool from_loop;              // false: MEASURE_ONCE or a SYNC_IN edge

    bool valid() const { return echo_time_us != ECHO_TIMEOUT; }
    // Distance at 343 m/s; nullopt without an echo.
    std::optional<double> distance_mm() const;
    // Temperature-compensated speed of sound (331.3 + 0.606·T m/s).
    std::optional<double> distance_mm_at(double air_temp_c) const;

    // Stream<Sr04Measurement> plumbing.
    static std::optional<Sr04Measurement> pull(detail::StreamHandle& h,
                                               std::chrono::milliseconds timeout, bool& closed);
};

class Sr04 : public Device {
public:
    // An SR04 on a link without an identity probe (tests, replay).
    static std::unique_ptr<Sr04> open(Link link);

    // Stored minimum interval between measurement starts (default 50000 µs);
    // the effective rate is also limited by the echo window (contract 03 §3).
    std::uint32_t sample_period_us();
    void set_sample_period_us(std::uint32_t period_us);
    std::uint16_t echo_decay_us();
    // The device clamps to 4000..65000 µs; returns the value in effect.
    // ArgumentError above 65535 (the u16 wire field).
    std::uint16_t set_echo_decay_us(std::uint32_t decay_us);

    // Single shot; BusyError while the loop runs.
    Sr04Measurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    void start();  // measurement loop; idempotent
    void stop();

    // Loop samples and SYNC_IN single shots.
    Unsubscribe on_measurement(std::function<void(const Sr04Measurement&)> cb);
    Stream<Sr04Measurement> stream(std::size_t maxsize = 256);

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── VL53L4CD single-zone ToF (contract 10) ─────────────────────────────────
// The ST ULD 2.2.3 runs on the host over the register bridge. init() first;
// configuration throws ArgumentError while ranging (the stream owns the
// register bank).

struct Vl53l4cdMeasurement {
    std::uint64_t timestamp_us;  // MCU µs at the INT edge (stream) / the read (poll)
    vl53l4::Vl53l4Result r;      // r.range_status 0 = valid, r.distance_mm, ...

    bool valid() const { return r.range_status == 0; }
    // UM2931 name of the range status ("valid", "sigma above threshold", ...).
    std::string status_text() const;

    // Stream<Vl53l4cdMeasurement> plumbing.
    static std::optional<Vl53l4cdMeasurement> pull(detail::StreamHandle& h,
                                                   std::chrono::milliseconds timeout, bool& closed);
};

// When INT fires relative to the window [low_mm, high_mm] (SYSTEM__INTERRUPT).
// Once programmed, only init() restores the "no window" default.
enum class DetectionWindow : std::uint8_t { Below = 0, Above = 1, Out = 2, In = 3 };

// The VL53L4CD distance window: INT only fires when `window` holds.
struct DetectionThresholds {
    std::uint16_t low_mm, high_mm;
    DetectionWindow window;
};

// VL53L4CD single-zone ToF: the ST ULD 2.2.3 on the host over the bridge.
class Vl53l4cd : public Device {
public:
    // A VL53L4CD on a link without an identity probe (tests, replay).
    static std::unique_ptr<Vl53l4cd> open(Link link);

    bool is_alive();  // the sensor answers with its model id
    // Configuration block + VHV calibration, then the bus at `bus_khz`.
    void init(std::uint16_t bus_khz = 1000);
    bool initialized() const;
    bool ranging() const;
    void xshut(std::uint8_t action);  // vl53l4::XSHUT_OFF / _ON / _RESET
    void reset_sensor();
    vl53l4::Vl53l4Info bridge_info();
    void set_i2c_speed_khz(std::uint16_t khz);

    // Budget 10..200 ms; inter 0 = continuous, > budget = autonomous.
    void set_range_timing(std::uint32_t budget_ms, std::uint32_t inter_ms = 0);
    vl53l4::RangeTiming range_timing();
    void set_offset_mm(std::int32_t mm);
    std::int32_t offset_mm();
    void set_xtalk_kcps(std::uint16_t kcps);  // 0 = off
    std::uint16_t xtalk_kcps();
    void set_detection_thresholds(const DetectionThresholds& t);
    DetectionThresholds detection_thresholds();
    void set_signal_threshold_kcps(std::uint16_t kcps);
    std::uint16_t signal_threshold_kcps();
    void set_sigma_threshold_mm(std::uint16_t mm);  // <= 16383
    std::uint16_t sigma_threshold_mm();
    void start_temperature_update();  // after a > 8 °C ambient change
    // Against a target at `target_mm`; returns the programmed value.
    std::int32_t calibrate_offset(std::uint16_t target_mm, std::uint8_t nb_samples = 20);
    std::uint16_t calibrate_xtalk(std::uint16_t target_mm, std::uint8_t nb_samples = 20);

    void start_ranging();
    void stop_ranging();  // idempotent
    // Poll mode: start, wait data-ready, read, stop. Refused while ranging.
    Vl53l4cdMeasurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    Unsubscribe on_measurement(std::function<void(const Vl53l4cdMeasurement&)> cb);
    Stream<Vl53l4cdMeasurement> measurements(std::size_t maxsize = 64);
    // The next streamed measurement; TimeoutError / DeviceLostError.
    Vl53l4cdMeasurement get_measurement(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    std::uint64_t stream_parse_errors() const;

    // Raw register access (16-bit address, contents as the sensor has them).
    bytes read_reg(std::uint16_t addr, std::size_t len);
    void write_reg(std::uint16_t addr, byte_span data);

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── multizone ToF: VL53L8CX / CH (contract 04), VL53L5CX / L7CX / L7CH (11) ──
// The ST ULD runs on the host over the SPI bridge (VL53L8) or the I2C bridge
// (L5/L7); init() downloads the ~84 KB sensor firmware. Configuration needs
// init() and throws ArgumentError while ranging.

// The multizone boards this class serves: VL53L8CX/CH on the SPI bridge
// (APP_VL53L8), VL53L5CX/L7CX/L7CH on the I2C bridge (APP_VL53L7). The model
// fixes the sensor firmware init() downloads (L8CX: ST ULD 2.1.0, L8CH:
// VL53LMZ 2.0.16 with CNH); open_device() picks L8CH for the VL53L8 production
// USB PID 0xED40, else L8CX; on APP_VL53L7 the PID (0xED48 / 0xED49 / 0xED4A),
// else the device name, else L7CX.
enum class Vl53l8Model { L8CX = 0, L8CH = 1, L5CX = 2, L7CX = 3, L7CH = 4 };

// The motion-indicator output of a frame (configure_motion_indicator).
struct Vl53l8Motion {
    std::uint32_t global_indicator_1, global_indicator_2;
    std::uint8_t status, nb_of_detected_aggregates, nb_of_aggregates;
    std::array<std::uint32_t, 32> motion;
};

// One streamed frame: the zone arrays in `frame` (distance_mm in mm; CH with
// CNH armed: frame.cnh_raw, decode with vl53l8::decode_cnh()), plus motion.
struct Vl53l8LiveFrame {
    vl53l8::Vl53l8Frame frame;
    std::optional<Vl53l8Motion> motion;

    // Stream<Vl53l8LiveFrame> plumbing.
    static std::optional<Vl53l8LiveFrame> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                               bool& closed);
};

// VL53LMZ_Motion_Configuration for CNH (compact network histograms, CH only),
// filled by the plugin helpers.
struct CnhSetup {
    std::int32_t ref_bin_offset = 0;
    std::uint32_t detection_threshold = 0, extra_noise_sigma = 0, null_den_clip_value = 0;
    std::uint8_t mem_update_mode = 0, mem_update_choice = 0, sum_span = 0, feature_length = 0;
    std::uint8_t nb_of_aggregates = 0, nb_of_temporal_accumulations = 1, min_nb_for_global_detection = 0;
    std::uint8_t global_indicator_format_1 = 0, global_indicator_format_2 = 0;
    std::uint8_t cnh_cfg = 0, cnh_flex_shift = 0, spare_3 = 0;
    std::array<std::int8_t, 64> map_id{};
    std::array<std::uint8_t, 32> indicator_format_1{}, indicator_format_2{};

    // vl53lmz_cnh_init_config: histogram start bin, CNH bins, device bins per CNH bin.
    void init_config(int start_bin, int num_bins, int sub_sample);
    // vl53lmz_cnh_create_agg_map: zones (resolution 16 | 64) -> aggregates.
    void create_agg_map(int resolution, int start_x, int start_y, int merge_x, int merge_y, int cols,
                        int rows);
    // On-device CNH buffer bytes; ArgumentError when blank or above the cap.
    std::size_t required_memory() const;
};

// Progress of init(): phase text, and done/total bytes during the big writes.
using Vl53l8Progress = std::function<void(const std::string& phase, std::size_t done, std::size_t total)>;

// Multizone ToF: the ST ULD on the host over the board's register bridge,
// one class for every Vl53l8Model.
class Vl53l8 : public Device {
public:
    // A multizone ToF on a link without an identity probe, the model given by
    // hand (tests, replay).
    static std::unique_ptr<Vl53l8> open(Link link, Vl53l8Model model);

    Vl53l8Model model() const;  // fixed at open: the USB PID, or open(link, model)
    bool is_alive();  // id 0xF0 / rev 0x0C over SPI; rev 0x02 (or 0xF0 / 0x01) on L5/L7
    // Boot the sensor MCU, download its firmware, upload NVM / xtalk / config
    // (~0.8 s over SPI, ~1.3 s over I2C). Needed after every power-up, deep
    // sleep and L5/L7 LpnOff / SoftCycle; it disarms CNH.
    void init(Vl53l8Progress progress = {});
    bool initialized() const;
    bool ranging() const;  // between start_ranging() and stop_ranging()

    int resolution();                          // 16 | 64 zones
    void set_resolution(int zones);            // vl53l8::RESOLUTION_4X4 | _8X8
    std::uint8_t ranging_frequency_hz();       // Hz
    void set_ranging_frequency_hz(std::uint8_t hz);  // >= 2 VL53L8, >= 1 L5/L7; max 60 (4x4) / 15 (8x8)
    std::uint8_t ranging_mode();               // DEPZ_VL53L8_RANGING_MODE_*: 1 continuous, 3 autonomous
    void set_ranging_mode(std::uint8_t mode);  // 1 continuous, 3 autonomous
    std::uint32_t integration_time_ms();       // ms per frame in autonomous mode
    void set_integration_time_ms(std::uint32_t ms);  // 2..1000, autonomous only
    std::uint8_t sharpener_percent();          // 0 = off; read back rounded
    void set_sharpener_percent(std::uint8_t pct);    // 0..99
    std::uint8_t target_order();               // 1 closest, 2 strongest
    void set_target_order(std::uint8_t order);  // 1 closest, 2 strongest
    std::uint8_t power_mode();                 // 0 sleep, 1 wakeup, 2 deep sleep
    void set_power_mode(std::uint8_t mode);    // no deep sleep on L5CX/L7CX; waking from it re-runs init()
    double xtalk_margin();                     // kcps/SPAD
    void set_xtalk_margin(double kcps_per_spad);  // <= 10000
    // Crosstalk calibration (reflectance 1..99 %, samples 1..16, 600..3000 mm);
    // returns false when the firmware found nothing to calibrate (no cover glass).
    bool calibrate_xtalk(std::uint8_t reflectance_percent, std::uint8_t nb_samples, std::uint16_t distance_mm);
    bytes caldata_xtalk();                     // the 776-byte blob, to save
    void set_caldata_xtalk(byte_span blob);    // restore a saved blob (776 bytes)
    // Detection thresholds: with them enabled, INT (and so the stream) only
    // fires for frames that meet them.
    bool detection_thresholds_enabled();
    void set_detection_thresholds_enabled(bool enabled);
    std::vector<vl53l8::DetectionThreshold> detection_thresholds();  // all 64
    // Up to 64, low/high in real units; bit 7 of zone_num marks the last one.
    void set_detection_thresholds(const std::vector<vl53l8::DetectionThreshold>& thresholds);
    void set_detection_thresholds_auto_stop(bool auto_stop);  // stop ranging on a hit; not on L5CX/L7CX
    // Motion indicator over [min_mm, max_mm] (400..4000, span <= 1500) for the
    // current resolution; frames then carry `motion`.
    void configure_motion_indicator(std::uint16_t min_mm = 400, std::uint16_t max_mm = 1500);
    // L8CH / L7CH only (WrongTypeError otherwise): arm CNH for the next
    // start_ranging(); frames then carry frame.cnh_raw. init() disarms it.
    void configure_cnh(const CnhSetup& setup);

    void start_ranging();  // start the sensor and the MCU's frame stream
    void stop_ranging();  // idempotent
    // Every streamed frame, on the reader thread.
    Unsubscribe on_frame(std::function<void(const Vl53l8LiveFrame&)> cb);
    // A pull stream of frames; a frame is several KB, so keep maxsize small.
    Stream<Vl53l8LiveFrame> frames(std::size_t maxsize = 8);
    // The next streamed frame; TimeoutError / DeviceLostError.
    Vl53l8LiveFrame get_frame(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    std::uint64_t frame_parse_errors() const;    // frames that did not decode
    std::uint64_t reassembler_discards() const;  // chunked frames lost on a gap

    // Escape hatches: raw registers and DCI indices.
    bytes read_reg(std::uint16_t addr, std::size_t len);
    void write_reg(std::uint16_t addr, byte_span data);
    bytes dci_read(std::uint16_t index, std::size_t len);
    void dci_write(std::uint16_t index, byte_span data);

    // L5/L7: the module type read at init(): 0 MZ = VL53L5CX, 1 MZEVO =
    // VL53L7CX/CH (never CX vs CH); nullopt before init and on a VL53L8.
    std::optional<int> module_type() const;
    // L5/L7 only (WrongTypeError on a VL53L8), like the next two: the bridge
    // counters; read before / after a run, not during one.
    vl53l7::Vl53l7Info bridge_info();
    // L5/L7 only. SCL snaps to 100, 200, 400, 500 ... 1000 kHz; returns the
    // value in effect.
    std::uint16_t set_i2c_speed_khz(std::uint16_t khz);
    // L5/L7 only. LpnOff / SoftCycle drop the sensor's state: init() again.
    void pin_ctrl(vl53l7::PinAction action);

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── BNO055 9-axis IMU (contract 13) ─────────────────────────────────────────
// Fuses on chip; mode / units / remap / calibration are register access here.
// configure(), then start_stream(10 ms) and read samples(), or poll
// read_sample(). Page-1 access throws ArgumentError while streaming.

// One decoded register block scaled by the units in effect; a channel is
// nullopt when the block did not cover it.
struct Bno055Sample {
    std::uint64_t timestamp_us;  // MCU µs: trigger (stream) / read completion
    std::uint8_t addr;
    bytes raw;
    bno055::Units units;
    std::optional<std::array<double, 3>> accel, mag, gyro, euler, linear_accel, gravity;
    std::optional<std::array<double, 4>> quaternion;  // w, x, y, z
    std::optional<double> temperature;
    std::optional<bno055::CalibStatus> calibration;

    // Stream<Bno055Sample> plumbing.
    static std::optional<Bno055Sample> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                            bool& closed);
};

// ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.
struct Bno055Status {
    std::uint8_t self_test, clk_status, status, error;
};

class Bno055 : public Device {
public:
    // A BNO055 on a link without an identity probe (tests, replay).
    static std::unique_ptr<Bno055> open(Link link);

    bno055::Bno055Info bridge_info();  // safe while streaming
    bool is_alive();
    // nRESET: stops any stream; CONFIG mode, power-on units — configure again.
    void reset_sensor();

    bytes read_registers(std::uint8_t addr, std::size_t len, int page = 0);
    void write_registers(std::uint8_t addr, byte_span data, int page = 0);

    bno055::OprMode operation_mode();
    // Through CONFIG; into a fusion mode it waits for the fusion to run.
    void set_operation_mode(bno055::OprMode mode);
    std::uint8_t power_mode();              // 0 normal, 1 low power, 2 suspend
    void set_power_mode(std::uint8_t mode);
    bno055::Units units();
    void set_units(const bno055::Units& units);
    bno055::AxisRemap axis_remap();
    void set_axis_remap(const bno055::AxisRemap& remap);
    void set_axis_placement(const std::string& placement);  // "P0".."P7"
    std::uint8_t temperature_source();      // 0 accel, 1 gyro
    void set_temperature_source(std::uint8_t source);
    // CONFIG -> units -> [remap] -> [calibration] -> mode; remembered.
    void configure(bno055::OprMode mode = bno055::OprMode::Ndof, const bno055::Units& units = {},
                   const std::optional<bno055::AxisRemap>& remap = std::nullopt,
                   const std::optional<bno055::CalibrationProfile>& calibration = std::nullopt);
    void restore_configuration();

    Bno055Status system_status();
    Bno055Status self_test();               // ~0.45 s, not while streaming
    bno055::CalibStatus calibration_status();
    bno055::CalibrationProfile calibration_profile();          // via CONFIG
    void write_calibration_profile(const bno055::CalibrationProfile& p);
    std::array<std::int16_t, 9> sic_matrix();
    void set_sic_matrix(const std::array<std::int16_t, 9>& m);

    bno055::AccelConfig accel_config();
    void set_accel_config(const bno055::AccelConfig& c);
    bno055::GyroConfig gyro_config();
    void set_gyro_config(const bno055::GyroConfig& c);
    bno055::MagConfig mag_config();
    void set_mag_config(const bno055::MagConfig& c);
    bytes unique_id();                      // 16 bytes
    std::uint8_t interrupt_enable();
    void set_interrupt_enable(std::uint8_t mask);
    std::uint8_t interrupt_mask();
    void set_interrupt_mask(std::uint8_t mask);
    void set_interrupt_setting(std::uint8_t reg, std::uint8_t value);  // 0x11..0x1F
    std::uint8_t read_interrupt_status();   // clears on read
    void clear_interrupt();

    Bno055Sample read_sample(std::uint8_t addr = bno055::FULL_BLOCK_ADDR,
                             std::uint8_t len = bno055::FULL_BLOCK_LEN);
    std::array<double, 4> read_quaternion();
    void start_stream(std::uint16_t period_ms = 10, std::uint8_t addr = bno055::FULL_BLOCK_ADDR,
                      std::uint8_t len = bno055::FULL_BLOCK_LEN,
                      std::uint8_t trigger = bno055::TRIGGER_TIMER);
    void stop_stream();
    bool streaming() const;
    Unsubscribe on_sample(std::function<void(const Bno055Sample&)> cb);
    Stream<Bno055Sample> samples(std::size_t maxsize = 256);
    Bno055Sample get_sample(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    std::uint64_t stream_parse_errors() const;

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── VL53L 1D ToF family (contract 12) ───────────────────────────────────────
// VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX (and VL53L4CD) on the
// APP_VL53L0_4 register bridge; the ST drivers run in the C SDK. init()
// binds (product, driver kind) — the product defaults to the one the board's
// name carries, a sibling's name borrows its driver — then configure() before
// every run, start_ranging() and read measurements(). Configuration while
// ranging, or a capability the pair lacks (ask supports()), throws
// ArgumentError.

enum class Vl53lxCap : unsigned {
    Mode = 0x0001, Timing = 0x0002, Offset = 0x0004, Xtalk = 0x0008, CalibOffset = 0x0010,
    CalibXtalk = 0x0020, Thresholds = 0x0040, SignalThresh = 0x0080, SigmaThresh = 0x0100,
    Roi = 0x0200, TempUpdate = 0x0400, RefSpad = 0x0800,
};

// One return of a histogram frame; min / max_range_mm bound its own pulse.
struct Vl53lxTarget {
    std::int32_t distance_mm = 0;
    int status = 0;
    std::string status_text;
    double signal_kcps = 0, ambient_kcps = 0, sigma_mm = 0;
    std::int32_t min_range_mm = 0, max_range_mm = 0;
};

// The histogram frame the targets came from (after the driver read it).
struct Vl53lxBins {
    std::vector<std::int32_t> bin_data;  // number_of_bins photon counts
    std::uint8_t vcsel_period = 0;       // the register value
    std::uint8_t stream_count = 0;
    std::uint8_t range_status = 0;
    std::uint32_t dss_actual_effective_spads = 0;
    std::int32_t ambient_per_bin = 0;
    std::int32_t zero_distance_phase = 0;
};

// One ranging result, the same shape for every product. Histogram driver:
// `targets` holds every return (the top level repeats targets[0]) and `bins`
// the frame; light drivers leave both empty. Extras a driver lacks are nullopt.
struct Vl53lxMeasurement {
    std::uint64_t timestamp_us = 0;  // MCU µs at the INT edge / the read
    std::int32_t distance_mm = 0;
    int status = 0;
    std::string status_text;
    double signal_kcps = 0, ambient_kcps = 0, sigma_mm = 0, spads = 0;
    std::vector<Vl53lxTarget> targets;
    std::optional<std::int32_t> stream_count;
    std::optional<double> signal_per_spad_kcps, ambient_per_spad_kcps;  // die ULDs
    std::optional<std::int32_t> dmax_mm, device_range_status;            // VL53L0X
    std::optional<std::int32_t> min_range_mm, max_range_mm, peak_bin;   // histogram
    std::optional<Vl53lxBins> bins;

    // Status 0, 6 (no wrap check yet) or 11 (merged target): a real distance.
    bool plottable() const;
    // The first plottable target, or the single distance of a light driver.
    std::optional<std::int32_t> primary_distance_mm() const;

    // Stream<Vl53lxMeasurement> plumbing.
    static std::optional<Vl53lxMeasurement> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                                 bool& closed);
};

class Vl53lx : public Device {
public:
    // A 1D-family board on a link without an identity probe (tests, replay).
    static std::unique_ptr<Vl53lx> open(Link link);

    // The product the board's device name carries ("VL53L4CX"), nullopt on
    // an unstamped board (then name it at init).
    std::optional<std::string> detected_product();
    // The driver kinds a product has (UI order).
    static std::vector<vl53lx::DriverKind> driver_kinds(const std::string& product);

    // Bind (product, driver) and initialise; defaults: the detected product,
    // its first kind (uld, ulp, histogram).
    void init(std::optional<vl53lx::DriverKind> driver = std::nullopt,
              const std::optional<std::string>& product = std::nullopt);
    bool initialized() const;
    std::optional<std::string> product() const;
    std::optional<vl53lx::DriverKind> driver_kind() const;
    std::string caveat() const;                 // "" when none
    std::uint32_t driver_reach_mm() const;      // 0 = not characterised
    bool supports(Vl53lxCap cap) const;
    std::uint16_t model_id();                   // cross-check only
    std::vector<std::string> modes() const;     // UI order; mode() = the one in use
    std::pair<int, int> budget_range() const;   // inclusive, ms
    std::vector<int> budget_choices();          // empty: any integer in range

    void xshut(std::uint8_t action);            // XSHUT_OFF / _ON / _RESET: init again
    vl53lx::Vl53lxInfo bridge_info();           // safe while streaming

    // Re-initialise and apply budget / mode (and a stored calibration).
    // signal_kcps replaces the blob's signal threshold, last — the re-init
    // puts it back to the default (depz_vl53lx_configure_ex).
    void configure(int budget_ms = 50, int inter_ms = 0, const std::optional<std::string>& mode = std::nullopt,
                   std::optional<std::int32_t> offset_mm = std::nullopt,
                   std::optional<std::int32_t> xtalk_kcps = std::nullopt,
                   std::optional<int> signal_kcps = std::nullopt);
    std::pair<int, int> range_timing();         // (budget ms, inter-measurement ms)
    void set_mode(const std::string& mode);
    std::optional<std::string> mode();          // nullopt on a product without modes

    std::int32_t offset_mm();
    void set_offset_mm(std::int32_t mm);
    std::int32_t xtalk_kcps();
    void set_xtalk_kcps(std::int32_t kcps);
    // Against a flat target; nb_samples 0 = the driver's default. Store the result.
    std::int32_t calibrate_offset(int target_mm, int nb_samples = 0);
    std::int32_t calibrate_xtalk(int target_mm, int nb_samples = 0);
    std::array<int, 3> detection_thresholds();  // low mm, high mm, window
    void set_detection_thresholds(int low_mm, int high_mm, int window);
    int signal_threshold_kcps();
    void set_signal_threshold_kcps(int kcps);
    int sigma_threshold_mm();
    void set_sigma_threshold_mm(int mm);
    std::pair<int, int> roi();
    void set_roi(int x, int y);
    int roi_center();
    void set_roi_center(int spad);
    void start_temperature_update();
    std::pair<std::uint32_t, bool> perform_ref_spad_management();  // VL53L0X

    void start_ranging();
    void stop_ranging();
    bool ranging() const;
    Vl53lxMeasurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    Unsubscribe on_measurement(std::function<void(const Vl53lxMeasurement&)> cb);
    Stream<Vl53lxMeasurement> measurements(std::size_t maxsize = 64);
    Vl53lxMeasurement get_measurement(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    std::uint64_t stream_parse_errors() const;

    bytes read_reg(std::uint16_t addr, std::size_t len);
    void write_reg(std::uint16_t addr, byte_span data);

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── BNO085 / BNO086 9-axis IMU (contract 05) ────────────────────────────────
// The bridge is an SHTP pass-through; the SH-2 hub protocol runs in the C SDK.
// hardware_reset(), enable(RotationVector, 100 Hz), then read reports();
// product_id() tells BNO085 (part 10004148) from BNO086 (10004563). SH-2
// refusals (non-zero status in a command / FRS answer) throw StatusError.

namespace bno086 {

// SH-2 sensor ids — also a report's `sensor_id`.
enum class SensorId : std::uint8_t {
    Accelerometer = 0x01, Gyroscope = 0x02, Magnetometer = 0x03, LinearAcceleration = 0x04,
    RotationVector = 0x05, Gravity = 0x06, UncalibratedGyroscope = 0x07, GameRotationVector = 0x08,
    GeomagneticRotationVector = 0x09, UncalibratedMagnetometer = 0x0F, TapDetector = 0x10,
    StepCounter = 0x11, SignificantMotion = 0x12, StabilityClassifier = 0x13, RawAccelerometer = 0x14,
    RawGyroscope = 0x15, RawMagnetometer = 0x16, StepDetector = 0x18, ShakeDetector = 0x19,
    FlipDetector = 0x1A, PickupDetector = 0x1B, StabilityDetector = 0x1C,
    PersonalActivityClassifier = 0x1E, SleepDetector = 0x1F, TiltDetector = 0x20, PocketDetector = 0x21,
    CircleDetector = 0x22, HeartRateMonitor = 0x23, ArvrStabilizedRv = 0x28,
    ArvrStabilizedGameRv = 0x29, GyroIntegratedRv = 0x2A,
};

enum class TareBasis : std::uint8_t {
    RotationVector = 0, GameRotationVector = 1, GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3, ArvrStabilizedRv = 4, ArvrStabilizedGameRv = 5,
};
inline constexpr std::uint8_t TARE_X = 1, TARE_Y = 2, TARE_Z = 4, TARE_ALL = 7;

// Get Feature Response: the rates in effect.
struct Feature {
    std::uint8_t sensor_id = 0, flags = 0;
    std::uint16_t sensitivity = 0;
    std::uint32_t interval_us = 0;  // granted interval; 0 = disabled
    std::uint32_t batch_us = 0;
    std::uint32_t cfg_word = 0;
};

// Everything Set Feature carries.
struct FeatureRequest {
    std::uint32_t interval_us = 0;
    std::uint32_t batch_us = 0;
    std::uint16_t sensitivity = 0;
    std::uint8_t flags = 0;
    std::uint32_t cfg_word = 0;
};

struct ProductId {
    std::uint8_t reset_cause = 0, sw_version_major = 0, sw_version_minor = 0;
    std::uint32_t sw_part_number = 0;  // 10004148 = BNO085, 10004563 = BNO086
    std::uint32_t sw_build_number = 0;
    std::uint16_t sw_version_patch = 0;
};

struct CalibrationConfig { bool accel = false, gyro = false, mag = false, planar = false; };
struct ErrorRecord { std::uint8_t severity, seq, source, error, module, code; };
struct Counts { std::uint8_t sensor_id; std::uint32_t offered, accepted, on, attempted; };

struct CommandResponse {
    std::uint8_t seq = 0, command = 0, command_seq = 0, response_seq = 0;
    std::array<std::uint8_t, 11> r{};  // r[0]: status for most commands
};

// A sensor's metadata FRS record; revision-gated fields are 0 when older.
struct SensorMetadata {
    std::uint8_t me_version = 0, mh_version = 0, sh_version = 0;
    std::uint32_t range_raw = 0, resolution_raw = 0;
    std::uint16_t revision = 0;
    std::uint16_t power_ma_q10 = 0;  // mA, Q10
    std::uint32_t min_period_us = 0, max_period_us = 0;
    std::uint16_t fifo_max = 0, fifo_reserved = 0, batch_buffer_bytes = 0;
    std::uint16_t q_point_1 = 0, q_point_2 = 0, q_point_3 = 0;
    double power_ma() const { return power_ma_q10 / 1024.0; }
};

// Q point of a sensor's primary fields; nullopt for event reports.
std::optional<int> q_point(std::uint8_t sensor_id);

}  // namespace bno086

// A decoded report (codec fields, raw integers authoritative) plus scaling.
struct Bno086Report : bno086::Report {
    // x, y, z in m/s², rad/s or µT (raw counts for the raw sensors).
    std::array<double, 3> xyz() const;
    std::array<double, 3> bias() const;           // uncalibrated gyro / mag
    std::array<double, 4> quaternion() const;     // i, j, k, real
    std::optional<double> accuracy_rad() const;   // nullopt for game variants
    std::array<double, 3> angular_velocity() const;  // gyro-integrated RV, rad/s
    double scalar() const;                        // environment reports

    // Stream<Bno086Report> plumbing.
    static std::optional<Bno086Report> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                            bool& closed);
};

class Bno086 : public Device {
public:
    // A BNO085 / BNO086 on a link without an identity probe (tests, replay).
    static std::unique_ptr<Bno086> open(Link link);

    // nRST: SHTP state, the advertisement and every enabled sensor restart.
    void hardware_reset(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    void wake();
    bytes advertisement();
    bno086::ProductId product_id();

    // Set Feature, then the granted rate read back (Get Feature).
    bno086::Feature enable(bno086::SensorId sensor, double hz);
    // No read-back when `verify` is false (then the result is nullopt).
    std::optional<bno086::Feature> enable(bno086::SensorId sensor, const bno086::FeatureRequest& req,
                                          bool verify = true);
    // Within [0.9, 2.1] x the requested rate (contract 05 §7)?
    static bool rate_ok(std::uint32_t requested_interval_us, const bno086::Feature& granted);
    void disable(bno086::SensorId sensor);
    bno086::Feature feature(bno086::SensorId sensor);

    Unsubscribe on_report(std::function<void(const Bno086Report&)> cb);
    Stream<Bno086Report> reports(std::size_t maxsize = 1024);
    Bno086Report get_report(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    std::uint64_t shtp_discarded() const;

    void tare_now(std::uint8_t axes = bno086::TARE_ALL,
                  bno086::TareBasis basis = bno086::TareBasis::RotationVector);
    void persist_tare();
    void set_reorientation(double x, double y, double z, double w);  // zeros clear
    void set_calibration(bool accel = true, bool gyro = true, bool mag = true, bool planar = false);
    bno086::CalibrationConfig calibration();
    void save_dcd();
    void configure_periodic_dcd(bool enable);

    std::vector<std::uint32_t> frs_read(std::uint16_t record);
    void frs_write(std::uint16_t record, const std::vector<std::uint32_t>& words);
    bno086::SensorMetadata metadata(bno086::SensorId sensor);

    std::uint8_t oscillator_type();  // 0 internal, 1 crystal, 2 external clock
    void clear_dcd_and_reset(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
    std::vector<bno086::ErrorRecord> errors(std::uint8_t severity = 0);
    bno086::Counts counts(bno086::SensorId sensor);
    void clear_counts(bno086::SensorId sensor);

    // Escape hatches: a Command Request (nullopt: no answer awaited), a raw cargo.
    std::optional<bno086::CommandResponse> command(std::uint8_t command, byte_span params = {},
                                                   bool wait_response = true);
    void send_shtp(std::uint8_t channel, byte_span payload);

private:
    using Device::Device;
    friend std::unique_ptr<Device> wrap_device(depz_device* d);
};

// ── discovery (contract 02 §4) ──────────────────────────────────────────────

struct SerialPortInfo {
    std::string port;
    std::optional<std::uint16_t> vid, pid;  // nullopt: not a USB port
    std::string usb_serial;                  // "" when unknown
};

struct DeviceInfo {
    std::string port;
    DeviceMode mode;
    std::optional<SensorType> sensor_type;  // nullopt in bootloader mode
    std::string software_name, fw_version, device_name;
    std::string serial_number;  // protocol serial (GET_SERIAL)
    std::optional<std::uint16_t> usb_vid, usb_pid;
    std::string usb_serial;
};

// Every serial port the OS lists, with its USB identity where known.
std::vector<SerialPortInfo> list_serial_ports();
// Probe one port; nullopt when nothing DEPZ-shaped answers.
std::optional<DeviceInfo> probe_port(const std::string& port,
                                     std::chrono::milliseconds timeout = std::chrono::milliseconds(200));
// Probe the candidates (with `match_usb`, only known DEPZ USB ids), by USB serial.
std::vector<DeviceInfo> list_depz_devices(bool match_usb = true,
                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(200));

struct OpenOptions {
    std::optional<std::string> port;    // exactly this port (even an unknown USB id)
    std::optional<std::string> serial;  // else the candidate with this USB serial
    std::optional<int> index;           // else the Nth candidate by USB serial
    std::chrono::milliseconds timeout{0};  // request timeout, 0 = default 200 ms
};

// Find, probe and open with the right class: dynamic_cast<Sr04*> (or
// <Vl53l4cd*>, <Vl53l8*>, <Bno055*>, <Bno086*>, <Vl53lx*>) tells.
std::unique_ptr<Device> open_device(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is an SR04.
std::unique_ptr<Sr04> open_sr04(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is a VL53L4CD.
std::unique_ptr<Vl53l4cd> open_vl53l4cd(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is a multizone ToF. The model
// comes from the USB PID (0xED40 -> L8CH, else L8CX on APP_VL53L8; 0xED48 /
// 0xED49 / 0xED4A -> L5CX / L7CX / L7CH, else the device name, else L7CX on
// APP_VL53L7).
std::unique_ptr<Vl53l8> open_vl53l8(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is a BNO055.
std::unique_ptr<Bno055> open_bno055(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is a BNO085 / BNO086.
std::unique_ptr<Bno086> open_bno086(const OpenOptions& opt = {});
// open_device, and WrongTypeError unless it is a VL53L 1D-family board.
std::unique_ptr<Vl53lx> open_vl53lx(const OpenOptions& opt = {});

}  // namespace depz
