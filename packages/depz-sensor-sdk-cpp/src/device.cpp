// device.cpp — the C++ live-hardware layer over the C SDK (depz_sensor_io.h).
#include "depz/device.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <typeinfo>

#include <depz_sensor_io.h>

#include "detail/c_bridge.hpp"
#include "detail/io_bridge.hpp"

namespace depz {

void detail::callback_threw(std::atomic<std::uint64_t>& errors)
{
    errors.fetch_add(1, std::memory_order_relaxed);
    std::cerr << "depz-sensor-sdk: a callback threw; dropped\n";
}

// ── errors ──────────────────────────────────────────────────────────────────

static_assert(kErrArg == DEPZ_E_ARG, "depz/errors.hpp kErrArg must be the C SDK's DEPZ_E_ARG");

[[noreturn]] void detail::throw_rc(int rc)
{
    const std::string msg = depz_last_error();
    switch (rc) {
    case DEPZ_E_ARG:
    case DEPZ_E_NOMEM: throw ArgumentError(rc, msg);
    case DEPZ_E_IO: throw IoError(rc, msg);
    case DEPZ_E_CLOSED: throw LinkClosedError(rc, msg);
    case DEPZ_E_LOST: throw DeviceLostError(rc, msg);
    case DEPZ_E_TIMEOUT: throw TimeoutError(rc, msg);
    case DEPZ_E_INVALID_REPLY: throw InvalidReplyError(rc, msg);
    case DEPZ_E_NO_DEVICE: throw NoDeviceError(rc, msg);
    case DEPZ_E_WRONG_TYPE: throw WrongTypeError(rc, msg);
    case DEPZ_E_REPLAY_MISMATCH: throw ReplayMismatchError(rc, msg);
    case DEPZ_E_BOOTLOADER: throw BootloaderModeError(rc, msg);
    case DEPZ_E_STATUS:
    case DEPZ_E_BUSY: {
        const int st = depz_last_status(), cmd = depz_last_status_cmd();
        const auto c = static_cast<std::uint8_t>(cmd < 0 ? 0 : cmd);
        const auto s = static_cast<Status>(st < 0 ? 0 : st);
        if (rc == DEPZ_E_BUSY) throw BusyError(rc, msg, c, s);
        throw StatusError(rc, msg, c, s);
    }
    default: throw Error(rc, msg.empty() ? "depz error " + std::to_string(rc) : msg);
    }
}

using detail::check;
using detail::cnh_to_c;

namespace {

int ms(std::chrono::milliseconds t)
{
    const auto n = t.count();
    return n < 0 ? 0 : n > 0x7FFFFFFF ? 0x7FFFFFFF : static_cast<int>(n);
}

int ms(const std::optional<std::chrono::milliseconds>& t) { return t ? ms(*t) : -1; }

const std::uint8_t* raw(byte_span s) { return reinterpret_cast<const std::uint8_t*>(s.data()); }

std::optional<SensorType> to_cpp(depz_sensor_type t)
{
    switch (t) {
    case DEPZ_SENSOR_SR04: return SensorType::Sr04;
    case DEPZ_SENSOR_VL53L8: return SensorType::Vl53l8;
    case DEPZ_SENSOR_BNO086: return SensorType::Bno086;
    case DEPZ_SENSOR_VL53L4: return SensorType::Vl53l4;
    case DEPZ_SENSOR_VL53L7: return SensorType::Vl53l7;
    case DEPZ_SENSOR_VL53LX: return SensorType::Vl53lx;
    case DEPZ_SENSOR_BNO055: return SensorType::Bno055;
    case DEPZ_SENSOR_TMF8829: return SensorType::Tmf8829;
    case DEPZ_SENSOR_UNKNOWN: return SensorType::Unknown;
    default: return std::nullopt;
    }
}

DeviceMode mode_of(const char* m)
{
    if (!std::strcmp(m, "app")) return DeviceMode::App;
    if (!std::strcmp(m, "bootloader")) return DeviceMode::Bootloader;
    return DeviceMode::Unknown;
}

std::optional<std::uint16_t> usb_id(int v)
{
    if (v < 0) return std::nullopt;
    return static_cast<std::uint16_t>(v);
}

}  // namespace

// ── links ───────────────────────────────────────────────────────────────────

Link::~Link() { depz_link_free(l_); }

Link& Link::operator=(Link&& o) noexcept
{
    if (this != &o) {
        depz_link_free(l_);
        l_ = std::exchange(o.l_, nullptr);
    }
    return *this;
}

Link Link::serial(const std::string& port)
{
    depz_link* l = nullptr;
    check(depz_link_open_serial(port.c_str(), &l));
    return Link(l);
}

Link Link::replay(const std::string& path, bool strict_tx, bool realtime)
{
    depz_link* l = nullptr;
    check(depz_link_open_replay(path.c_str(), strict_tx, realtime, &l));
    return Link(l);
}

Link Link::recording(Link inner, const std::string& path, const std::string& header_extra_json)
{
    depz_link* l = nullptr;
    check(depz_link_open_recording(inner.release(), path.c_str(),
                                   header_extra_json.empty() ? nullptr : header_extra_json.c_str(), &l));
    return Link(l);
}

std::pair<Link, Link> Link::loopback_pair()
{
    depz_link *a = nullptr, *b = nullptr;
    check(depz_link_loopback_pair(&a, &b));
    return {Link(a), Link(b)};
}

bytes Link::read(std::chrono::milliseconds timeout)
{
    std::uint8_t buf[4096];
    std::size_t got = 0;
    if (!l_) throw LinkClosedError(DEPZ_E_CLOSED, "link moved from");
    check(depz_link_read(l_, buf, sizeof buf, ms(timeout), &got));
    const auto* p = reinterpret_cast<const std::byte*>(buf);
    return bytes(p, p + got);
}

void Link::write(byte_span data)
{
    if (!l_) throw LinkClosedError(DEPZ_E_CLOSED, "link moved from");
    check(depz_link_write(l_, raw(data), data.size()));
}

void Link::close() { depz_link_close(l_); }
bool Link::closed() const { return !l_ || depz_link_closed(l_); }
std::string Link::name() const { return l_ ? depz_link_name(l_) : ""; }
bool Link::replay_exhausted() const { return depz_link_replay_exhausted(l_); }

// ── streams ─────────────────────────────────────────────────────────────────

detail::SubscriptionHandle::~SubscriptionHandle() { depz_subscription_close(s); }

bool detail::SubscriptionHandle::next(void* item, std::chrono::milliseconds timeout, bool& closed,
                                       std::optional<std::string>& lost)
{
    const int rc = depz_subscription_next(s, item, ms(timeout));
    if (rc == DEPZ_OK) return true;
    if (rc == DEPZ_E_CLOSED) closed = true;
    else if (rc == DEPZ_E_LOST) {
        closed = true;
        lost = depz_last_error();
    } else if (rc != DEPZ_E_TIMEOUT) throw_rc(rc);
    return false;
}

std::uint64_t detail::SubscriptionHandle::dropped() const { return depz_subscription_dropped_count(s); }

// ── events ──────────────────────────────────────────────────────────────────

namespace {

DeviceEvent event_from_c(const depz_device_event& c)
{
    DeviceEvent e;
    switch (c.type) {
    case DEPZ_DEV_EV_SEQUENCE_ERROR: e.type = DeviceEvent::Type::SequenceError; break;
    case DEPZ_DEV_EV_SEQ_GAP: e.type = DeviceEvent::Type::SeqGap; break;
    case DEPZ_DEV_EV_CRC_ERROR: e.type = DeviceEvent::Type::CrcError; break;
    case DEPZ_DEV_EV_TRASH: e.type = DeviceEvent::Type::Trash; break;
    case DEPZ_DEV_EV_UNSOLICITED_STATUS: e.type = DeviceEvent::Type::UnsolicitedStatus; break;
    case DEPZ_DEV_EV_TEXT: e.type = DeviceEvent::Type::Text; break;
    case DEPZ_DEV_EV_TEMPERATURE: e.type = DeviceEvent::Type::Temperature; break;
    case DEPZ_DEV_EV_DISCONNECTED: e.type = DeviceEvent::Type::Disconnected; break;
    case DEPZ_DEV_EV_TMF8829_INTERRUPT: e.type = DeviceEvent::Type::Tmf8829Interrupt; break;
    }
    e.cmd = c.cmd;
    e.seq = c.seq;
    e.expected_seq = c.expected_seq;
    e.received_seq = c.received_seq;
    e.missed = c.missed;
    e.by_device = c.by_device;
    e.status = c.status;
    e.timestamp_us = c.timestamp_us;
    e.celsius = c.celsius;
    e.trash_len = c.trash_len;
    const auto* d = reinterpret_cast<const std::byte*>(c.data);
    e.data.assign(d, d + c.data_len);
    e.text = c.text;
    e.tmf8829 = detail::wrap<tmf8829::Tmf8829Event>(c.tmf8829);
    return e;
}

Sr04Measurement measurement_from_c(const depz_sr04_measurement& c)
{
    return Sr04Measurement{c.timestamp_us, c.echo_time_us, c.from_loop};
}

template <class T>
struct FnHolder : detail::CallbackHolder {
    std::function<void(const T&)> fn;
    std::function<bool(const T&)> where;  // Stream<T>::on: none = every item
};

// Contract 07 §3: a callback that threw is counted, reported and dropped — an
// exception must not unwind through the C reader thread, and the reader goes on.
void callback_threw(detail::CallbackHolder* h)
{
    if (h && h->errors) detail::callback_threw(*h->errors);
}

Vl53l4cdMeasurement l4_from_c(const depz_vl53l4cd_measurement& c)
{
    vl53l4::Vl53l4Result r;
    r.range_status = c.r.range_status;
    r.distance_mm = c.r.distance_mm;
    r.ambient_rate_kcps = c.r.ambient_rate_kcps;
    r.ambient_per_spad_kcps = c.r.ambient_per_spad_kcps;
    r.signal_rate_kcps = c.r.signal_rate_kcps;
    r.signal_per_spad_kcps = c.r.signal_per_spad_kcps;
    r.number_of_spad = c.r.number_of_spad;
    r.sigma_mm = c.r.sigma_mm;
    r.stream_count = c.r.stream_count;
    return Vl53l4cdMeasurement{c.timestamp_us, r};
}

Vl53l8LiveFrame l8_from_c(const depz_vl53l8_live_frame& c)
{
    Vl53l8LiveFrame out;
    out.frame = detail::vl53l8_frame_from_c(c.f, c.cnh, c.cnh_len);
    if (c.has_motion) {
        Vl53l8Motion m{};
        m.global_indicator_1 = c.motion.global_indicator_1;
        m.global_indicator_2 = c.motion.global_indicator_2;
        m.status = c.motion.status;
        m.nb_of_detected_aggregates = c.motion.nb_of_detected_aggregates;
        m.nb_of_aggregates = c.motion.nb_of_aggregates;
        std::copy(c.motion.motion, c.motion.motion + 32, m.motion.begin());
        out.motion = m;
    }
    return out;
}

// Progress callbacks run on the caller's thread and are not guarded (contract
// 07 §3): the first exception is kept, the callback is not called again and
// the exception is rethrown once the C call returns.
template <class Fn>
struct ProgressCtx {
    Fn& fn;
    std::exception_ptr error;
};

void progress_tramp(const char* phase, std::size_t done, std::size_t total, void* user)
{
    auto* ctx = static_cast<ProgressCtx<Vl53l8Progress>*>(user);
    if (ctx->error) return;
    try {
        ctx->fn(phase, done, total);
    } catch (...) {
        ctx->error = std::current_exception();
    }
}

}  // namespace

depz_vl53l8_cnh_setup detail::cnh_to_c(const CnhSetup& s)
{
    depz_vl53l8_cnh_setup c{};
    c.ref_bin_offset = s.ref_bin_offset;
    c.detection_threshold = s.detection_threshold;
    c.extra_noise_sigma = s.extra_noise_sigma;
    c.null_den_clip_value = s.null_den_clip_value;
    c.mem_update_mode = s.mem_update_mode;
    c.mem_update_choice = s.mem_update_choice;
    c.sum_span = s.sum_span;
    c.feature_length = s.feature_length;
    c.nb_of_aggregates = s.nb_of_aggregates;
    c.nb_of_temporal_accumulations = s.nb_of_temporal_accumulations;
    c.min_nb_for_global_detection = s.min_nb_for_global_detection;
    c.global_indicator_format_1 = s.global_indicator_format_1;
    c.global_indicator_format_2 = s.global_indicator_format_2;
    c.cnh_cfg = s.cnh_cfg;
    c.cnh_flex_shift = s.cnh_flex_shift;
    c.spare_3 = s.spare_3;
    std::copy(s.map_id.begin(), s.map_id.end(), c.map_id);
    std::copy(s.indicator_format_1.begin(), s.indicator_format_1.end(), c.indicator_format_1);
    std::copy(s.indicator_format_2.begin(), s.indicator_format_2.end(), c.indicator_format_2);
    return c;
}

namespace {

void cnh_from_c(const depz_vl53l8_cnh_setup& c, CnhSetup& s)
{
    s.ref_bin_offset = c.ref_bin_offset;
    s.detection_threshold = c.detection_threshold;
    s.extra_noise_sigma = c.extra_noise_sigma;
    s.null_den_clip_value = c.null_den_clip_value;
    s.mem_update_mode = c.mem_update_mode;
    s.mem_update_choice = c.mem_update_choice;
    s.sum_span = c.sum_span;
    s.feature_length = c.feature_length;
    s.nb_of_aggregates = c.nb_of_aggregates;
    s.nb_of_temporal_accumulations = c.nb_of_temporal_accumulations;
    s.min_nb_for_global_detection = c.min_nb_for_global_detection;
    s.global_indicator_format_1 = c.global_indicator_format_1;
    s.global_indicator_format_2 = c.global_indicator_format_2;
    s.cnh_cfg = c.cnh_cfg;
    s.cnh_flex_shift = c.cnh_flex_shift;
    s.spare_3 = c.spare_3;
    std::copy(c.map_id, c.map_id + 64, s.map_id.begin());
    std::copy(c.indicator_format_1, c.indicator_format_1 + 32, s.indicator_format_1.begin());
    std::copy(c.indicator_format_2, c.indicator_format_2 + 32, s.indicator_format_2.begin());
}


Bno055Sample bno_from_c(const depz_bno055_sample& c)
{
    Bno055Sample s;
    s.timestamp_us = c.timestamp_us;
    s.addr = c.addr;
    const auto* r = reinterpret_cast<const std::byte*>(c.raw);
    s.raw.assign(r, r + c.len);
    s.units = bno055::Units(c.units);
    auto v3 = [](const double* p) { return std::array<double, 3>{p[0], p[1], p[2]}; };
    if (c.present & DEPZ_BNO055_HAS_ACCEL) s.accel = v3(c.accel);
    if (c.present & DEPZ_BNO055_HAS_MAG) s.mag = v3(c.mag);
    if (c.present & DEPZ_BNO055_HAS_GYRO) s.gyro = v3(c.gyro);
    if (c.present & DEPZ_BNO055_HAS_EULER) s.euler = v3(c.euler);
    if (c.present & DEPZ_BNO055_HAS_LINEAR_ACCEL) s.linear_accel = v3(c.linear_accel);
    if (c.present & DEPZ_BNO055_HAS_GRAVITY) s.gravity = v3(c.gravity);
    if (c.present & DEPZ_BNO055_HAS_QUATERNION)
        s.quaternion = std::array<double, 4>{c.quaternion[0], c.quaternion[1], c.quaternion[2], c.quaternion[3]};
    if (c.present & DEPZ_BNO055_HAS_TEMPERATURE) s.temperature = c.temperature;
    if (c.present & DEPZ_BNO055_HAS_CALIBRATION)
        s.calibration = bno055::CalibStatus(c.calibration);
    return s;
}

Bno086Report bno086_from_c(const depz_bno_report& c) { return Bno086Report{detail::bno086_report_from_c(c)}; }

bno086::Feature feature_from_c(const depz_bno_feature& c) { return detail::wrap<bno086::Feature>(c); }

Vl53lxMeasurement vlx_from_c(const depz_vl53lx_measurement& c)
{
    Vl53lxMeasurement m;
    m.timestamp_us = c.timestamp_us;
    m.distance_mm = c.distance_mm;
    m.status = c.status;
    m.status_text = c.status_text ? c.status_text : "";
    m.signal_kcps = c.signal_kcps;
    m.ambient_kcps = c.ambient_kcps;
    m.sigma_mm = c.sigma_mm;
    m.spads = c.spads;
    for (std::size_t i = 0; i < c.n_targets && i < DEPZ_VL53LX_MAX_TARGETS; i++) {
        const auto& t = c.targets[i];
        m.targets.push_back({t.distance_mm, t.status, t.status_text ? t.status_text : "", t.signal_kcps,
                             t.ambient_kcps, t.sigma_mm, t.min_range_mm, t.max_range_mm});
    }
    if (c.stream_count >= 0) m.stream_count = c.stream_count;
    if (c.dmax_mm >= 0) m.dmax_mm = c.dmax_mm;
    if (c.device_range_status >= 0) m.device_range_status = c.device_range_status;
    if (c.min_range_mm >= 0) m.min_range_mm = c.min_range_mm;
    if (c.max_range_mm >= 0) m.max_range_mm = c.max_range_mm;
    if (c.peak_bin >= 0) m.peak_bin = c.peak_bin;
    if (!c.has_bins) {
        m.signal_per_spad_kcps = c.signal_per_spad_kcps;
        m.ambient_per_spad_kcps = c.ambient_per_spad_kcps;
    } else {
        Vl53lxBins b;
        const std::size_t n = c.bins.number_of_bins < 24 ? c.bins.number_of_bins : 24;
        b.bin_data.assign(c.bins.bin_data, c.bins.bin_data + n);
        b.vcsel_period = c.bins.vcsel_period;
        b.stream_count = c.bins.stream_count;
        b.range_status = c.bins.range_status;
        b.dss_actual_effective_spads = c.bins.dss_actual_effective_spads;
        b.ambient_per_bin = c.bins.ambient_per_bin;
        b.zero_distance_phase = c.bins.zero_distance_phase;
        m.bins = std::move(b);
    }
    return m;
}

depz_vl53lx_driver vlx_kind_to_c(vl53lx::DriverKind k)
{
    switch (k) {
    case vl53lx::DriverKind::Uld: return DEPZ_VL53LX_DRIVER_ULD;
    case vl53lx::DriverKind::Ulp: return DEPZ_VL53LX_DRIVER_ULP;
    default: return DEPZ_VL53LX_DRIVER_HISTOGRAM;
    }
}

std::optional<vl53lx::DriverKind> vlx_kind_from_c(depz_vl53lx_driver k)
{
    switch (k) {
    case DEPZ_VL53LX_DRIVER_ULD: return vl53lx::DriverKind::Uld;
    case DEPZ_VL53LX_DRIVER_ULP: return vl53lx::DriverKind::Ulp;
    case DEPZ_VL53LX_DRIVER_HISTOGRAM: return vl53lx::DriverKind::Histogram;
    default: return std::nullopt;
    }
}

depz_vl53lx_product vlx_product_to_c(const std::string& name)
{
    const depz_vl53lx_product p = depz_vl53lx_product_from_str(name.c_str());
    if (p == DEPZ_VL53LX_PRODUCT_NONE) throw ArgumentError(DEPZ_E_ARG, "vl53lx: no such product: " + name);
    return p;
}

std::optional<std::string> vlx_product_name(depz_vl53lx_product p)
{
    const auto* row = depz_vl53lx_product_get(p);
    if (!row) return std::nullopt;
    return std::string(row->name);
}

}  // namespace

// ── device ──────────────────────────────────────────────────────────────────

std::uint64_t host_now_us() { return depz_host_now_us(); }

std::unique_ptr<Device> wrap_device(depz_device* d)
{
    if (depz_is_sr04(d)) return std::unique_ptr<Device>(new Sr04(d));
    if (depz_is_vl53l4cd(d)) return std::unique_ptr<Device>(new Vl53l4cd(d));
    if (depz_is_vl53l8(d)) return std::unique_ptr<Device>(new Vl53l8(d));
    if (depz_is_bno055(d)) return std::unique_ptr<Device>(new Bno055(d));
    if (depz_is_bno086(d)) return std::unique_ptr<Device>(new Bno086(d));
    if (depz_is_vl53lx(d)) return std::unique_ptr<Device>(new Vl53lx(d));
    if (depz_is_tmf8829(d)) return std::unique_ptr<Device>(new Tmf8829(d));
    return std::unique_ptr<Device>(new Device(d));
}

std::unique_ptr<Device> Device::open(const std::string& port)
{
    depz_device* d = nullptr;
    check(depz_device_open(port.c_str(), &d));
    return wrap_device(d);
}

std::unique_ptr<Device> Device::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_device_open_link(link.release(), &d));
    return wrap_device(d);
}

std::unique_ptr<Device> Device::rewrap(std::unique_ptr<Device> dev)
{
    depz_device* d = dev->handle();
    auto out = wrap_device(d);
    {
        std::lock_guard<std::mutex> g(dev->held_lock_);
        out->held_ = std::move(dev->held_);
        out->callback_errors_ = dev->callback_errors_;
    }
    out->life_ = dev->life_;  // Unsubscribes taken before stay live
    dev->d_ = nullptr;  // the new wrapper owns the handle now
    return out;
}

std::unique_ptr<Device> attach_sensor_class(std::unique_ptr<Device>&& dev, const std::string& class_name)
{
    if (!dev) throw ArgumentError(DEPZ_E_ARG, "attach_sensor_class: no device");
    const Device& plain = *dev;
    if (typeid(plain) != typeid(Device))
        throw ArgumentError(DEPZ_E_ARG, "attach_sensor_class needs a plain Device (it already has a class)");
    check(depz_attach_sensor_class(dev->handle(), class_name.c_str()));
    if (class_name == "device") return std::move(dev);
    return Device::rewrap(std::move(dev));
}

std::unique_ptr<Device> open_on_link(Link link, std::optional<std::uint16_t> usb_vid,
                                     std::optional<std::uint16_t> usb_pid, std::chrono::milliseconds timeout)
{
    depz_device* d = nullptr;
    check(depz_open_on_link(link.release(), usb_vid ? *usb_vid : -1, usb_pid ? *usb_pid : -1,
                            ms(timeout), &d));
    return wrap_device(d);
}

Device::Device(depz_device* d) : d_(d), events_(stream_init(depz_device_events)) {}

Device::HandleLife::~HandleLife() { depz_device_free(shut_down); }

Device::~Device() { close(); }

namespace {
// > 0 on the reader thread while it runs a user callback (Stream<T>::on,
// Tmf8829::on_frame): a close() from there must not wait for a close() on
// another thread that is joining this very thread.
thread_local int t_in_callback = 0;

struct InCallback {
    InCallback() { ++t_in_callback; }
    ~InCallback() { --t_in_callback; }
};
}  // namespace

void Device::close()
{
    // One close() at a time: a second one on another thread waits for the
    // first to finish (it returned at once and cleared the callback storage
    // while the first was still joining the reader inside a callback).
    std::unique_lock<std::mutex> one_at_a_time(close_lock_, std::defer_lock);
    if (!t_in_callback) one_at_a_time.lock();
    // Unsubscribes first (they must not reach a closed handle), then the
    // reader, then the callback storage it may still be running. The handle's
    // memory goes with the last owner of life_, not here: a call on another
    // thread that read d_ just before the exchange gets the closed error.
    if (depz_device* live = d_.load()) {  // kept: lost() and handle() still answer after the handle has gone
        if (const char* why = depz_device_lost(live)) {
            std::lock_guard<std::mutex> g(held_lock_);
            lost_ = why;
        }
    }
    depz_device* d = d_.exchange(nullptr);
    if (!d) return;  // closed already
    {
        life_->alive.store(false);
        life_->shut_down = d;
        for (auto& [f, get] : finals_) {
            const depz_stream* s = get(d);
            std::vector<std::uint64_t> dropped(depz_stream_dropped_counts(s, nullptr, 0));
            dropped.resize(depz_stream_dropped_counts(s, dropped.data(), dropped.size()));
            std::lock_guard<std::mutex> g(f->lock);
            f->parse_errors = depz_stream_parse_errors(s);
            f->dropped_counts = std::move(dropped);
        }
    }
    depz_device_shutdown(d);
    // From a callback the reader runs on (it was not joined): the callback
    // storage holds the very closure that is running — it stays until the
    // Device goes, after depz_device_free() has joined the reader.
    if (t_in_callback) return;
    std::lock_guard<std::mutex> g(held_lock_);
    held_.clear();
}

depz_device* Device::handle() const
{
    depz_device* d = d_.load();
    if (!d) throw_ended();
    return d;
}

void Device::throw_ended() const
{
    if (const std::optional<std::string> why = lost()) throw DeviceLostError(DEPZ_E_LOST, *why);  // the cause as the C SDK built it
    throw LinkClosedError(DEPZ_E_CLOSED, "device closed");
}

Unsubscribe Device::unsubscriber(void (*off)(depz_device*, int), int token) const
{
    std::weak_ptr<HandleLife> life = life_;
    depz_device* d = d_.load();
    return [life, d, off, token] {
        const auto l = life.lock();  // holds the handle's memory for the call
        if (l && l->alive.load()) off(d, token);
    };
}

Unsubscribe Device::unsubscriber(depz_stream* s, int token) const
{
    std::weak_ptr<HandleLife> life = life_;
    return [life, s, token] {
        const auto l = life.lock();  // holds the handle's memory for the call
        if (l && l->alive.load()) depz_stream_off(s, token);
    };
}

void Device::keep(std::unique_ptr<detail::CallbackHolder> h)
{
    std::lock_guard<std::mutex> g(held_lock_);
    held_.push_back(std::move(h));
}

bool Device::closed() const
{
    depz_device* d = d_.load();
    return !d || depz_device_closed(d);
}

std::optional<std::string> Device::lost() const
{
    if (depz_device* d = d_.load()) {
        if (const char* why = depz_device_lost(d)) return std::string(why);
        return std::nullopt;
    }
    std::lock_guard<std::mutex> g(held_lock_);
    return lost_;
}

std::string Device::port() const
{
    depz_device* d = d_.load();
    return d ? depz_device_port(d) : "";
}

std::optional<SensorType> Device::sensor_type() const { return to_cpp(depz_device_sensor_type(d_)); }
void Device::set_timeout(std::chrono::milliseconds t) { depz_device_set_timeout_ms(handle(), ms(t)); }

LinkStats Device::stats() const
{
    depz_link_stats s{};
    depz_device_stats(handle(), &s);
    return LinkStats{s.tx_packets,  s.rx_packets,  s.tx_bytes, s.rx_bytes,
                     s.crc_errors,  s.header_errors, s.trash_bytes, s.seq_gaps,
                     s.device_seq_errors, callback_errors_->load(std::memory_order_relaxed)};
}

std::string Device::device_name()
{
    char s[256];
    check(depz_device_get_device_name(handle(), s, sizeof s));
    return s;
}

std::string Device::software_name()
{
    char s[256];
    check(depz_device_get_software_name(handle(), s, sizeof s));
    return s;
}

std::string Device::serial_number()
{
    char s[256];
    check(depz_device_get_serial_number(handle(), s, sizeof s));
    return s;
}

Identity Device::identify()
{
    char s[256];
    check(depz_device_get_software_name(handle(), s, sizeof s));
    return parse_software_name(std::string(s, depz_device_string_len(s, sizeof s)));
}

double Device::read_mcu_temperature()
{
    double c = 0;
    check(depz_device_read_mcu_temperature(handle(), &c));
    return c;
}

CrcType Device::payload_crc_type()
{
    depz_crc_type t = DEPZ_CRC_NONE;
    check(depz_device_get_payload_crc_type(handle(), &t));
    return static_cast<CrcType>(t);
}

void Device::set_payload_crc_type(CrcType t)
{
    check(depz_device_set_payload_crc_type(handle(), static_cast<depz_crc_type>(t)));
}

SyncPinConfig Device::sync_pin(std::uint8_t pin)
{
    depz_sync_pin_config c{};
    check(depz_device_get_sync_pin(handle(), pin, &c));
    return SyncPinConfig{c.pin, static_cast<SyncPinMode>(c.mode), static_cast<SyncPinPolarity>(c.polarity)};
}

void Device::set_sync_pin(const SyncPinConfig& c)
{
    depz_sync_pin_config cc{c.pin, static_cast<std::uint8_t>(c.mode), static_cast<std::uint8_t>(c.polarity)};
    check(depz_device_set_sync_pin(handle(), &cc));
}

void Device::reset() { check(depz_device_reset(handle())); }
void Device::enter_bootloader() { check(depz_device_enter_bootloader(handle())); }

TimeSync Device::sync_time(int samples)
{
    depz_time_sync s{};
    check(depz_device_sync_time(handle(), samples, &s));
    return TimeSync{s.offset_us, s.rtt_us, s.synced_at_host_us};
}

std::optional<TimeSync> Device::time_sync() const
{
    depz_time_sync s{};
    if (!depz_device_time_sync(handle(), &s)) return std::nullopt;
    return TimeSync{s.offset_us, s.rtt_us, s.synced_at_host_us};
}

std::int64_t Device::to_host_time_us(std::uint64_t device_us) const
{
    std::int64_t out = 0;
    check(depz_device_to_host_time_us(handle(), device_us, &out));
    return out;
}

void Device::request_ok(std::uint8_t cmd, byte_span payload,
                        std::optional<std::chrono::milliseconds> timeout)
{
    check(depz_device_request(handle(), cmd, raw(payload), payload.size(), nullptr, nullptr, true,
                              ms(timeout)));
}

namespace {
struct MatchCtx {
    std::function<bool(std::uint8_t, byte_span)>* fn;
    bytes out;
    std::exception_ptr error;  // the matcher threw on the packet it was shown
};

// Contract 07 §3 (ERRATA A4): a matcher that throws fails its own request —
// the packet is claimed so the C request completes now, and request() turns
// the exception into InvalidReplyError. Left unclaimed, the request would
// wait its whole timeout out for a reply that had already come.
bool match_tramp(std::uint8_t cmd, const std::uint8_t* p, std::size_t len, void* ctx)
{
    auto* m = static_cast<MatchCtx*>(ctx);
    const auto* b = reinterpret_cast<const std::byte*>(p);
    try {
        if (!(*m->fn)(cmd, byte_span(b, len))) return false;
    } catch (...) {
        m->error = std::current_exception();
        return true;
    }
    m->out.assign(b, b + len);
    return true;
}

std::string what_of(const std::exception_ptr& e)
{
    try {
        std::rethrow_exception(e);
    } catch (const std::exception& x) {
        return x.what();
    } catch (...) {
        return "a non-standard exception";
    }
}
}  // namespace

bytes Device::request(std::uint8_t cmd, byte_span payload,
                      std::function<bool(std::uint8_t rpt, byte_span payload)> match,
                      std::optional<std::chrono::milliseconds> timeout)
{
    // contract 07 §2: a request names how its reply completes
    if (!match) throw ArgumentError(DEPZ_E_ARG, "request: configure a matcher (request_ok() waits for RPT_STATUS OK)");
    MatchCtx ctx{&match, {}, nullptr};
    const int rc = depz_device_request(handle(), cmd, raw(payload), payload.size(), match_tramp, &ctx, false,
                                       ms(timeout));
    if (ctx.error) {
        char head[48];
        std::snprintf(head, sizeof head, "reply to cmd 0x%02X does not decode: ", static_cast<unsigned>(cmd));
        throw InvalidReplyError(DEPZ_E_INVALID_REPLY, head + what_of(ctx.error));
    }
    check(rc);
    return ctx.out;
}

void Device::send(std::uint8_t cmd, byte_span payload)
{
    check(depz_device_send(handle(), cmd, raw(payload), payload.size()));
}

// ── SR04 ────────────────────────────────────────────────────────────────────

std::optional<double> Sr04Measurement::distance_mm() const
{
    depz_sr04_measurement c{timestamp_us, echo_time_us, from_loop};
    double mm = 0;
    if (!depz_sr04_measurement_distance_mm(&c, &mm)) return std::nullopt;
    return mm;
}

std::optional<double> Sr04Measurement::distance_mm_at(double air_temp_c) const
{
    depz_sr04_measurement c{timestamp_us, echo_time_us, from_loop};
    double mm = 0;
    if (!depz_sr04_measurement_distance_mm_at(&c, air_temp_c, &mm)) return std::nullopt;
    return mm;
}

std::unique_ptr<Sr04> Sr04::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_sr04_open_link(link.release(), &d));
    return std::unique_ptr<Sr04>(new Sr04(d));
}

Sr04::Sr04(depz_device* d) : Device(d), measurements_(stream_init(depz_sr04_measurements)) {}

std::uint32_t Sr04::sample_period_us()
{
    std::uint32_t v = 0;
    check(depz_sr04_get_sample_period_us(handle(), &v));
    return v;
}

void Sr04::set_sample_period_us(std::uint32_t period_us)
{
    check(depz_sr04_set_sample_period_us(handle(), period_us));
}

std::uint16_t Sr04::echo_decay_us()
{
    std::uint16_t v = 0;
    check(depz_sr04_get_echo_decay_us(handle(), &v));
    return v;
}

std::uint16_t Sr04::set_echo_decay_us(std::uint32_t decay_us)
{
    std::uint16_t v = 0;
    check(depz_sr04_set_echo_decay_us(handle(), decay_us, &v));
    return v;
}

Sr04Measurement Sr04::measure_once(std::chrono::milliseconds timeout)
{
    depz_sr04_measurement m{};
    check(depz_sr04_measure_once(handle(), ms(timeout), &m));
    return measurement_from_c(m);
}

void Sr04::start() { check(depz_sr04_start(handle())); }
void Sr04::stop() { check(depz_sr04_stop(handle())); }

// ── VL53L4CD ────────────────────────────────────────────────────────────────

std::string Vl53l4cdMeasurement::status_text() const { return depz_vl53l4cd_status_text(r.range_status); }

std::unique_ptr<Vl53l4cd> Vl53l4cd::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_vl53l4cd_open_link(link.release(), &d));
    return std::unique_ptr<Vl53l4cd>(new Vl53l4cd(d));
}

Vl53l4cd::Vl53l4cd(depz_device* d) : Device(d), measurements_(stream_init(depz_vl53l4cd_measurements)) {}

bool Vl53l4cd::is_alive()
{
    bool alive = false;
    check(depz_vl53l4cd_is_alive(handle(), &alive));
    return alive;
}

void Vl53l4cd::init(std::uint16_t bus_khz) { check(depz_vl53l4cd_init(handle(), bus_khz)); }
bool Vl53l4cd::initialized() const { return depz_vl53l4cd_initialized(d_); }
bool Vl53l4cd::ranging() const { return depz_vl53l4cd_ranging(d_); }
void Vl53l4cd::xshut(std::uint8_t action) { check(depz_vl53l4cd_xshut(handle(), action)); }
void Vl53l4cd::reset_sensor() { check(depz_vl53l4cd_reset_sensor(handle())); }

vl53l4::Vl53l4Info Vl53l4cd::bridge_info()
{
    vl53l4::Vl53l4Info i{};
    check(depz_vl53l4cd_bridge_info(handle(), &i));
    return i;
}

void Vl53l4cd::set_i2c_speed_khz(std::uint16_t khz) { check(depz_vl53l4cd_set_i2c_speed_khz(handle(), khz)); }

void Vl53l4cd::set_range_timing(std::uint32_t budget_ms, std::uint32_t inter_ms)
{
    check(depz_vl53l4cd_set_range_timing(handle(), budget_ms, inter_ms));
}

vl53l4::RangeTiming Vl53l4cd::range_timing()
{
    vl53l4::RangeTiming t;
    check(depz_vl53l4cd_get_range_timing(handle(), &t.timing_budget_ms, &t.inter_measurement_ms));
    return t;
}

void Vl53l4cd::set_offset_mm(std::int32_t mm) { check(depz_vl53l4cd_set_offset_mm(handle(), mm)); }

std::int32_t Vl53l4cd::offset_mm()
{
    std::int32_t v = 0;
    check(depz_vl53l4cd_get_offset_mm(handle(), &v));
    return v;
}

void Vl53l4cd::set_xtalk_kcps(std::uint16_t kcps) { check(depz_vl53l4cd_set_xtalk_kcps(handle(), kcps)); }

std::uint16_t Vl53l4cd::xtalk_kcps()
{
    std::uint16_t v = 0;
    check(depz_vl53l4cd_get_xtalk_kcps(handle(), &v));
    return v;
}

void Vl53l4cd::set_detection_thresholds(const DetectionThresholds& t)
{
    check(depz_vl53l4cd_set_detection_thresholds(handle(), t.low_mm, t.high_mm,
                                                  static_cast<std::uint8_t>(t.window)));
}

DetectionThresholds Vl53l4cd::detection_thresholds()
{
    std::uint16_t lo = 0, hi = 0;
    std::uint8_t w = 0;
    check(depz_vl53l4cd_get_detection_thresholds(handle(), &lo, &hi, &w));
    return DetectionThresholds{lo, hi, static_cast<DetectionWindow>(w)};
}

void Vl53l4cd::set_signal_threshold_kcps(std::uint16_t kcps)
{
    check(depz_vl53l4cd_set_signal_threshold_kcps(handle(), kcps));
}

std::uint16_t Vl53l4cd::signal_threshold_kcps()
{
    std::uint16_t v = 0;
    check(depz_vl53l4cd_get_signal_threshold_kcps(handle(), &v));
    return v;
}

void Vl53l4cd::set_sigma_threshold_mm(std::uint16_t mm) { check(depz_vl53l4cd_set_sigma_threshold_mm(handle(), mm)); }

std::uint16_t Vl53l4cd::sigma_threshold_mm()
{
    std::uint16_t v = 0;
    check(depz_vl53l4cd_get_sigma_threshold_mm(handle(), &v));
    return v;
}

void Vl53l4cd::start_temperature_update() { check(depz_vl53l4cd_start_temperature_update(handle())); }

std::int32_t Vl53l4cd::calibrate_offset(std::uint16_t target_mm, std::uint8_t nb_samples)
{
    std::int32_t v = 0;
    check(depz_vl53l4cd_calibrate_offset(handle(), target_mm, nb_samples, &v));
    return v;
}

std::int32_t Vl53l4cd::calibrate_xtalk(std::uint16_t target_mm, std::uint8_t nb_samples)
{
    std::int32_t v = 0;
    check(depz_vl53l4cd_calibrate_xtalk(handle(), target_mm, nb_samples, &v));
    return v;
}

void Vl53l4cd::start_ranging() { check(depz_vl53l4cd_start_ranging(handle())); }
void Vl53l4cd::stop_ranging() { check(depz_vl53l4cd_stop_ranging(handle())); }

Vl53l4cdMeasurement Vl53l4cd::measure_once(std::chrono::milliseconds timeout)
{
    depz_vl53l4cd_measurement m{};
    check(depz_vl53l4cd_measure_once(handle(), ms(timeout), &m));
    return l4_from_c(m);
}

bytes Vl53l4cd::read_reg(std::uint16_t addr, std::size_t len)
{
    bytes out(len);
    check(depz_vl53l4cd_read_reg(handle(), addr, reinterpret_cast<std::uint8_t*>(out.data()), len));
    return out;
}

void Vl53l4cd::write_reg(std::uint16_t addr, byte_span data)
{
    check(depz_vl53l4cd_write_reg(handle(), addr, raw(data), data.size()));
}

// ── VL53L8 ──────────────────────────────────────────────────────────────────

void CnhSetup::init_config(int start_bin, int num_bins, int sub_sample)
{
    depz_vl53l8_cnh_setup c{};
    check(depz_vl53l8_cnh_init_config_checked(&c, start_bin, num_bins, sub_sample));
    cnh_from_c(c, *this);
}

void CnhSetup::create_agg_map(int resolution, int start_x, int start_y, int merge_x, int merge_y, int cols, int rows)
{
    depz_vl53l8_cnh_setup c = cnh_to_c(*this);
    check(depz_vl53l8_cnh_create_agg_map(&c, resolution, start_x, start_y, merge_x, merge_y, cols, rows));
    cnh_from_c(c, *this);
}

std::size_t CnhSetup::required_memory() const
{
    depz_vl53l8_cnh_setup c = cnh_to_c(*this);
    std::size_t n = 0;
    check(depz_vl53l8_cnh_required_memory(&c, &n));
    return n;
}

std::unique_ptr<Vl53l8> Vl53l8::open(Link link, Vl53l8Model model)
{
    depz_device* d = nullptr;
    check(depz_vl53l8_open_link(link.release(), static_cast<depz_vl53l8_model>(model), &d));
    return std::unique_ptr<Vl53l8>(new Vl53l8(d));
}

Vl53l8::Vl53l8(depz_device* d) : Device(d), frames_(stream_init(depz_vl53l8_frames)) {}

Vl53l8Model Vl53l8::model() const { return static_cast<Vl53l8Model>(depz_vl53l8_get_model(d_)); }

bool Vl53l8::is_alive()
{
    bool alive = false;
    check(depz_vl53l8_is_alive(handle(), &alive, nullptr, nullptr));
    return alive;
}

void Vl53l8::init(Vl53l8Progress progress)
{
    ProgressCtx<Vl53l8Progress> ctx{progress, nullptr};
    const int rc = depz_vl53l8_init(handle(), progress ? progress_tramp : nullptr, progress ? &ctx : nullptr);
    if (ctx.error) std::rethrow_exception(ctx.error);
    check(rc);
}

bool Vl53l8::initialized() const { return depz_vl53l8_initialized(d_); }
bool Vl53l8::ranging() const { return depz_vl53l8_ranging(d_); }

int Vl53l8::resolution()
{
    int z = 0;
    check(depz_vl53l8_get_resolution(handle(), &z));
    return z;
}

void Vl53l8::set_resolution(int zones) { check(depz_vl53l8_set_resolution(handle(), zones)); }

std::uint8_t Vl53l8::ranging_frequency_hz()
{
    std::uint8_t v = 0;
    check(depz_vl53l8_get_ranging_frequency_hz(handle(), &v));
    return v;
}

void Vl53l8::set_ranging_frequency_hz(std::uint8_t hz) { check(depz_vl53l8_set_ranging_frequency_hz(handle(), hz)); }

std::uint8_t Vl53l8::ranging_mode()
{
    std::uint8_t v = 0;
    check(depz_vl53l8_get_ranging_mode(handle(), &v));
    return v;
}

void Vl53l8::set_ranging_mode(std::uint8_t mode) { check(depz_vl53l8_set_ranging_mode(handle(), mode)); }

std::uint32_t Vl53l8::integration_time_ms()
{
    std::uint32_t v = 0;
    check(depz_vl53l8_get_integration_time_ms(handle(), &v));
    return v;
}

void Vl53l8::set_integration_time_ms(std::uint32_t ms) { check(depz_vl53l8_set_integration_time_ms(handle(), ms)); }

std::uint8_t Vl53l8::sharpener_percent()
{
    std::uint8_t v = 0;
    check(depz_vl53l8_get_sharpener_percent(handle(), &v));
    return v;
}

void Vl53l8::set_sharpener_percent(std::uint8_t pct) { check(depz_vl53l8_set_sharpener_percent(handle(), pct)); }

std::uint8_t Vl53l8::target_order()
{
    std::uint8_t v = 0;
    check(depz_vl53l8_get_target_order(handle(), &v));
    return v;
}

void Vl53l8::set_target_order(std::uint8_t order) { check(depz_vl53l8_set_target_order(handle(), order)); }

std::uint8_t Vl53l8::power_mode()
{
    std::uint8_t v = 0;
    check(depz_vl53l8_get_power_mode(handle(), &v));
    return v;
}

void Vl53l8::set_power_mode(std::uint8_t mode) { check(depz_vl53l8_set_power_mode(handle(), mode)); }

std::uint32_t Vl53l8::xtalk_margin()
{
    std::uint32_t v = 0;
    check(depz_vl53l8_get_xtalk_margin(handle(), &v));
    return v;
}

void Vl53l8::set_xtalk_margin(std::uint32_t kcps) { check(depz_vl53l8_set_xtalk_margin(handle(), kcps)); }

bool Vl53l8::calibrate_xtalk(std::uint8_t reflectance_percent, std::uint8_t nb_samples, std::uint16_t distance_mm)
{
    bool failed = false;
    check(depz_vl53l8_calibrate_xtalk(handle(), reflectance_percent, nb_samples, distance_mm, &failed));
    return !failed;
}

bytes Vl53l8::caldata_xtalk()
{
    bytes out(DEPZ_VL53L8_XTALK_BUFFER_SIZE);
    check(depz_vl53l8_get_caldata_xtalk(handle(), reinterpret_cast<std::uint8_t*>(out.data())));
    return out;
}

void Vl53l8::set_caldata_xtalk(byte_span blob)
{
    if (blob.size() != DEPZ_VL53L8_XTALK_BUFFER_SIZE)
        throw ArgumentError(DEPZ_E_ARG, "xtalk blob must be 776 bytes");
    check(depz_vl53l8_set_caldata_xtalk(handle(), raw(blob)));
}

bool Vl53l8::detection_thresholds_enabled()
{
    bool v = false;
    check(depz_vl53l8_get_detection_thresholds_enable(handle(), &v));
    return v;
}

void Vl53l8::set_detection_thresholds_enabled(bool enabled)
{
    check(depz_vl53l8_set_detection_thresholds_enable(handle(), enabled));
}

std::vector<vl53l8::DetectionThreshold> Vl53l8::detection_thresholds()
{
    std::vector<vl53l8::DetectionThreshold> out(DEPZ_VL53L8_NB_THRESHOLDS);
    check(depz_vl53l8_get_detection_thresholds(handle(), out.data()));
    return out;
}

void Vl53l8::set_detection_thresholds(const std::vector<vl53l8::DetectionThreshold>& thresholds)
{
    check(depz_vl53l8_set_detection_thresholds(handle(), thresholds.data(), thresholds.size()));
}

void Vl53l8::set_detection_thresholds_auto_stop(bool auto_stop)
{
    check(depz_vl53l8_set_detection_thresholds_auto_stop(handle(), auto_stop));
}

void Vl53l8::configure_motion_indicator(std::uint16_t min_mm, std::uint16_t max_mm)
{
    check(depz_vl53l8_configure_motion_indicator(handle(), min_mm, max_mm));
}

void Vl53l8::configure_cnh(const CnhSetup& setup)
{
    depz_vl53l8_cnh_setup c = cnh_to_c(setup);
    check(depz_vl53l8_configure_cnh(handle(), &c));
}

std::optional<CnhSetup> Vl53l8::cnh_setup() const
{
    depz_vl53l8_cnh_setup c{};
    if (!depz_vl53l8_cnh_armed(handle(), &c)) return std::nullopt;
    CnhSetup s;
    cnh_from_c(c, s);
    return s;
}

void Vl53l8::start_ranging() { check(depz_vl53l8_start_ranging(handle())); }
void Vl53l8::stop_ranging() { check(depz_vl53l8_stop_ranging(handle())); }

std::uint64_t Vl53l8::reassembler_discards() const { return depz_vl53l8_reassembler_discards(d_); }

bytes Vl53l8::read_reg(std::uint16_t addr, std::size_t len)
{
    bytes out(len);
    check(depz_vl53l8_read_reg(handle(), addr, reinterpret_cast<std::uint8_t*>(out.data()), len));
    return out;
}

void Vl53l8::write_reg(std::uint16_t addr, byte_span data) { check(depz_vl53l8_write_reg(handle(), addr, raw(data), data.size())); }

bytes Vl53l8::dci_read(std::uint16_t index, std::size_t len)
{
    bytes out(len);
    check(depz_vl53l8_dci_read(handle(), index, reinterpret_cast<std::uint8_t*>(out.data()), len));
    return out;
}

void Vl53l8::dci_write(std::uint16_t index, byte_span data) { check(depz_vl53l8_dci_write(handle(), index, raw(data), data.size())); }

std::optional<int> Vl53l8::module_type() const
{
    const int t = depz_vl53l8_module_type(d_);
    if (t < 0) return std::nullopt;
    return t;
}

vl53l7::Vl53l7Info Vl53l8::bridge_info()
{
    vl53l7::Vl53l7Info i{};
    check(depz_vl53l7_bridge_info(handle(), &i));
    return i;
}

std::uint16_t Vl53l8::set_i2c_speed_khz(std::uint16_t khz)
{
    std::uint16_t eff = 0;
    check(depz_vl53l7_set_i2c_speed_khz(handle(), khz, &eff));
    return eff;
}

void Vl53l8::pin_ctrl(vl53l7::PinAction action)
{
    check(depz_vl53l7_pin_ctrl(handle(), static_cast<std::uint8_t>(action)));
}

// ── BNO055 ──────────────────────────────────────────────────────────────────

std::unique_ptr<Bno055> Bno055::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_bno055_open_link(link.release(), &d));
    return std::unique_ptr<Bno055>(new Bno055(d));
}

Bno055::Bno055(depz_device* d) : Device(d), samples_(stream_init(depz_bno055_samples)) {}

bno055::Bno055Info Bno055::bridge_info()
{
    bno055::Bno055Info i{};
    check(depz_bno055_bridge_info(handle(), &i));
    return i;
}

bool Bno055::is_alive()
{
    bool alive = false;
    check(depz_bno055_is_alive(handle(), &alive));
    return alive;
}

void Bno055::reset_sensor() { check(depz_bno055_reset_sensor(handle())); }

bytes Bno055::read_registers(std::uint8_t addr, std::size_t len, int page)
{
    bytes out(len);
    check(depz_bno055_read_registers(handle(), addr, reinterpret_cast<std::uint8_t*>(out.data()), len, page));
    return out;
}

void Bno055::write_registers(std::uint8_t addr, byte_span data, int page)
{
    check(depz_bno055_write_registers(handle(), addr, raw(data), data.size(), page));
}

bno055::OprMode Bno055::operation_mode()
{
    std::uint8_t v = 0;
    check(depz_bno055_get_operation_mode(handle(), &v));
    return static_cast<bno055::OprMode>(v);
}

void Bno055::set_operation_mode(bno055::OprMode mode)
{
    check(depz_bno055_set_operation_mode(handle(), static_cast<std::uint8_t>(mode)));
}

std::uint8_t Bno055::power_mode()
{
    std::uint8_t v = 0;
    check(depz_bno055_get_power_mode(handle(), &v));
    return v;
}

void Bno055::set_power_mode(std::uint8_t mode) { check(depz_bno055_set_power_mode(handle(), mode)); }

bno055::Units Bno055::units()
{
    bno055::Units u;
    check(depz_bno055_get_units(handle(), &u));
    return u;
}

void Bno055::set_units(const bno055::Units& units)
{
    check(depz_bno055_set_units(handle(), &units));
}

bno055::AxisRemap Bno055::axis_remap()
{
    bno055::AxisRemap r;
    check(depz_bno055_get_axis_remap(handle(), &r));
    return r;
}

namespace {
// The sensor would silently keep its old mapping for a non-permutation.
void require_permutation(const bno055::AxisRemap& r)
{
    if (!r.pack()) throw ArgumentError(DEPZ_E_ARG, "bno055: axes must be a permutation of x, y, z");
}
}  // namespace

void Bno055::set_axis_remap(const bno055::AxisRemap& remap)
{
    require_permutation(remap);
    check(depz_bno055_set_axis_remap(handle(), &remap));
}

void Bno055::set_axis_placement(const std::string& placement)
{
    check(depz_bno055_set_axis_placement(handle(), placement.c_str()));
}

std::uint8_t Bno055::temperature_source()
{
    std::uint8_t v = 0;
    check(depz_bno055_get_temperature_source(handle(), &v));
    return v;
}

void Bno055::set_temperature_source(std::uint8_t source) { check(depz_bno055_set_temperature_source(handle(), source)); }

void Bno055::configure(bno055::OprMode mode, const bno055::Units& units, const std::optional<bno055::AxisRemap>& remap,
                       const std::optional<bno055::CalibrationProfile>& calibration)
{
    if (remap) require_permutation(*remap);
    check(depz_bno055_configure(handle(), static_cast<std::uint8_t>(mode), &units, remap ? &*remap : nullptr,
                                calibration ? &*calibration : nullptr));
}

void Bno055::restore_configuration() { check(depz_bno055_restore_configuration(handle())); }

Bno055Status Bno055::system_status()
{
    depz_bno055_status_regs c{};
    check(depz_bno055_system_status(handle(), &c));
    return Bno055Status{c.self_test, c.clk_status, c.status, c.error};
}

Bno055Status Bno055::self_test()
{
    depz_bno055_status_regs c{};
    check(depz_bno055_self_test(handle(), &c));
    return Bno055Status{c.self_test, c.clk_status, c.status, c.error};
}

bno055::CalibStatus Bno055::calibration_status()
{
    bno055::CalibStatus c;
    check(depz_bno055_calibration_status(handle(), &c));
    return c;
}

bno055::CalibrationProfile Bno055::calibration_profile()
{
    bno055::CalibrationProfile p;
    check(depz_bno055_read_calibration_profile(handle(), &p));
    return p;
}

void Bno055::write_calibration_profile(const bno055::CalibrationProfile& p)
{
    check(depz_bno055_write_calibration_profile(handle(), &p));
}

std::array<std::int16_t, 9> Bno055::sic_matrix()
{
    std::array<std::int16_t, 9> m{};
    check(depz_bno055_get_sic_matrix(handle(), m.data()));
    return m;
}

void Bno055::set_sic_matrix(const std::array<std::int16_t, 9>& m) { check(depz_bno055_set_sic_matrix(handle(), m.data())); }

bno055::AccelConfig Bno055::accel_config()
{
    bno055::AccelConfig c;
    check(depz_bno055_get_accel_config(handle(), &c));
    return c;
}

void Bno055::set_accel_config(const bno055::AccelConfig& cfg)
{
    check(depz_bno055_set_accel_config(handle(), &cfg));
}

bno055::GyroConfig Bno055::gyro_config()
{
    bno055::GyroConfig c;
    check(depz_bno055_get_gyro_config(handle(), &c));
    return c;
}

void Bno055::set_gyro_config(const bno055::GyroConfig& cfg)
{
    check(depz_bno055_set_gyro_config(handle(), &cfg));
}

bno055::MagConfig Bno055::mag_config()
{
    bno055::MagConfig c;
    check(depz_bno055_get_mag_config(handle(), &c));
    return c;
}

void Bno055::set_mag_config(const bno055::MagConfig& cfg)
{
    check(depz_bno055_set_mag_config(handle(), &cfg));
}

bytes Bno055::unique_id()
{
    bytes out(16);
    check(depz_bno055_unique_id(handle(), reinterpret_cast<std::uint8_t*>(out.data())));
    return out;
}

std::uint8_t Bno055::interrupt_enable()
{
    std::uint8_t v = 0;
    check(depz_bno055_get_interrupt_enable(handle(), &v));
    return v;
}

void Bno055::set_interrupt_enable(std::uint8_t mask) { check(depz_bno055_set_interrupt_enable(handle(), mask)); }

std::uint8_t Bno055::interrupt_mask()
{
    std::uint8_t v = 0;
    check(depz_bno055_get_interrupt_mask(handle(), &v));
    return v;
}

void Bno055::set_interrupt_mask(std::uint8_t mask) { check(depz_bno055_set_interrupt_mask(handle(), mask)); }
void Bno055::set_interrupt_setting(std::uint8_t reg, std::uint8_t value) { check(depz_bno055_set_interrupt_setting(handle(), reg, value)); }

std::uint8_t Bno055::read_interrupt_status()
{
    std::uint8_t v = 0;
    check(depz_bno055_read_interrupt_status(handle(), &v));
    return v;
}

void Bno055::clear_interrupt() { check(depz_bno055_clear_interrupt(handle())); }

Bno055Sample Bno055::read_sample(std::uint8_t addr, std::uint8_t len)
{
    depz_bno055_sample c{};
    check(depz_bno055_read_sample(handle(), addr, len, &c));
    return bno_from_c(c);
}

std::array<double, 4> Bno055::read_quaternion()
{
    std::array<double, 4> q{};
    check(depz_bno055_read_quaternion(handle(), q.data()));
    return q;
}

void Bno055::start_stream(std::uint16_t period_ms, std::uint8_t addr, std::uint8_t len, std::uint8_t trigger)
{
    check(depz_bno055_start_stream(handle(), period_ms, addr, len, trigger));
}

void Bno055::stop_stream() { check(depz_bno055_stop_stream(handle())); }
bool Bno055::streaming() const { return depz_bno055_streaming(d_); }

// ── BNO085 / BNO086 ─────────────────────────────────────────────────────────

std::optional<int> bno086::q_point(std::uint8_t sensor_id)
{
    const int q = depz_bno_q_point(sensor_id);
    if (q < 0) return std::nullopt;
    return q;
}

std::array<double, 3> Bno086Report::xyz() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_xyz(&c, v.data());
    return v;
}

std::array<double, 3> Bno086Report::bias() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_bias(&c, v.data());
    return v;
}

std::array<double, 4> Bno086Report::quaternion() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    std::array<double, 4> v{};
    depz_bno_report_quaternion(&c, v.data());
    return v;
}

std::optional<double> Bno086Report::accuracy_rad() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    double v = 0;
    if (!depz_bno_report_accuracy_rad(&c, &v)) return std::nullopt;
    return v;
}

std::array<double, 3> Bno086Report::angular_velocity() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_angular_velocity(&c, v.data());
    return v;
}

double Bno086Report::scalar() const
{
    const depz_bno_report c = detail::bno086_report_to_c(*this);
    return depz_bno_report_scalar(&c);
}

std::unique_ptr<Bno086> Bno086::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_bno086_open_link(link.release(), &d));
    return std::unique_ptr<Bno086>(new Bno086(d));
}

Bno086::Bno086(depz_device* d) : Device(d), reports_(stream_init(depz_bno086_reports)) {}

void Bno086::hardware_reset(std::chrono::milliseconds timeout)
{
    check(depz_bno086_hardware_reset(handle(), ms(timeout)));
}

void Bno086::wake() { check(depz_bno086_wake(handle())); }

void Bno086::sleep() { check(depz_bno086_sleep(handle())); }

void Bno086::power_on() { check(depz_bno086_power_on(handle())); }

bytes Bno086::advertisement()
{
    std::size_t n = 0;
    check(depz_bno086_advertisement(handle(), nullptr, 0, &n));
    bytes out(n);
    check(depz_bno086_advertisement(handle(), reinterpret_cast<std::uint8_t*>(out.data()), n, &n));
    out.resize(std::min(n, out.size()));
    return out;
}

static bno086::ProductId product_id_from_c(const depz_bno_product_id& c) { return detail::wrap<bno086::ProductId>(c); }

bno086::ProductId Bno086::product_id()
{
    depz_bno_product_id c{};
    check(depz_bno086_product_id(handle(), &c));
    return product_id_from_c(c);
}

std::vector<bno086::ProductId> Bno086::product_ids()
{
    depz_bno_product_id c[8]{};
    std::size_t n = 0;
    check(depz_bno086_product_ids(handle(), c, 8, &n));
    std::vector<bno086::ProductId> out;
    for (std::size_t i = 0; i < n; i++) out.push_back(product_id_from_c(c[i]));
    return out;
}

bno086::Feature Bno086::enable(bno086::SensorId sensor, double hz)
{
    depz_bno_feature g{};
    check(depz_bno086_enable(handle(), static_cast<std::uint8_t>(sensor), hz, &g));
    return feature_from_c(g);
}

std::optional<bno086::Feature> Bno086::enable(bno086::SensorId sensor, const bno086::FeatureRequest& req,
                                              bool verify)
{
    depz_bno086_feature_request c{};
    c.interval_us = req.interval_us;
    c.batch_us = req.batch_us;
    c.sensitivity = req.sensitivity;
    c.flags = req.flags;
    c.cfg_word = req.cfg_word;
    depz_bno_feature g{};
    check(depz_bno086_enable_ex(handle(), static_cast<std::uint8_t>(sensor), &c, verify ? &g : nullptr));
    if (!verify) return std::nullopt;
    return feature_from_c(g);
}

bool Bno086::rate_ok(std::uint32_t requested_interval_us, const bno086::Feature& granted)
{
    depz_bno_feature g{};
    g.interval_us = granted.interval_us;
    return depz_bno086_rate_ok(requested_interval_us, &g);
}

void Bno086::disable(bno086::SensorId sensor)
{
    check(depz_bno086_disable(handle(), static_cast<std::uint8_t>(sensor)));
}

bno086::Feature Bno086::feature(bno086::SensorId sensor)
{
    depz_bno_feature g{};
    check(depz_bno086_get_feature(handle(), static_cast<std::uint8_t>(sensor), &g));
    return feature_from_c(g);
}

std::uint64_t Bno086::shtp_discarded() const { return depz_bno086_shtp_discarded(d_); }

void Bno086::tare_now(std::uint8_t axes, bno086::TareBasis basis)
{
    check(depz_bno086_tare_now(handle(), axes, static_cast<std::uint8_t>(basis)));
}

void Bno086::persist_tare() { check(depz_bno086_persist_tare(handle())); }

void Bno086::set_reorientation(double x, double y, double z, double w)
{
    check(depz_bno086_set_reorientation(handle(), x, y, z, w));
}

void Bno086::set_calibration(bool accel, bool gyro, bool mag, bool planar)
{
    check(depz_bno086_set_calibration(handle(), accel, gyro, mag, planar));
}

bno086::CalibrationConfig Bno086::calibration()
{
    depz_bno086_calibration c{};
    check(depz_bno086_get_calibration(handle(), &c));
    return {c.accel, c.gyro, c.mag, c.planar};
}

void Bno086::save_dcd() { check(depz_bno086_save_dcd(handle())); }

void Bno086::configure_periodic_dcd(bool enable) { check(depz_bno086_configure_periodic_dcd(handle(), enable)); }

void Bno086::set_motion_intent(bno086::MotionIntent intent)
{
    check(depz_bno086_set_motion_intent(handle(), static_cast<std::uint8_t>(intent)));
}

void Bno086::enable_motion_request(double hz) { check(depz_bno086_enable_motion_request(handle(), hz)); }

void Bno086::flush(bno086::SensorId sensor)
{
    check(depz_bno086_flush(handle(), static_cast<std::uint8_t>(sensor)));
}

std::vector<std::uint32_t> Bno086::frs_read(std::uint16_t record)
{
    std::vector<std::uint32_t> words(64);
    std::size_t n = 0;
    int rc = depz_bno086_frs_read(handle(), record, words.data(), words.size(), &n);
    if (rc == DEPZ_E_ARG && n > words.size()) { /* a longer record: read it again, whole */
        words.resize(n);
        rc = depz_bno086_frs_read(handle(), record, words.data(), words.size(), &n);
    }
    check(rc);
    words.resize(n);
    return words;
}

void Bno086::frs_write(std::uint16_t record, const std::vector<std::uint32_t>& words)
{
    check(depz_bno086_frs_write(handle(), record, words.data(), words.size()));
}

bno086::SensorMetadata Bno086::metadata(bno086::SensorId sensor)
{
    // depz_bno086_get_metadata() is this FRS read + parse; done here to keep the words.
    const std::uint16_t record = depz_bno_metadata_record(static_cast<std::uint8_t>(sensor));
    depz_bno_metadata c{};
    if (!record) check(depz_bno086_get_metadata(handle(), static_cast<std::uint8_t>(sensor), &c));  // DEPZ_E_ARG
    std::uint32_t words[64];
    std::size_t n = 0;
    check(depz_bno086_frs_read(handle(), record, words, sizeof words / sizeof words[0], &n));
    depz_bno_metadata_from_words(words, n, &c);
    bno086::SensorMetadata m;
    m.raw_words.assign(words, words + n);
    m.me_version = c.me_version;
    m.mh_version = c.mh_version;
    m.sh_version = c.sh_version;
    m.range_raw = c.range_raw;
    m.resolution_raw = c.resolution_raw;
    m.revision = c.revision;
    m.power_ma_q10 = c.power_ma_q10;
    m.min_period_us = c.min_period_us;
    m.max_period_us = c.max_period_us;
    m.fifo_max = c.fifo_max;
    m.fifo_reserved = c.fifo_reserved;
    m.batch_buffer_bytes = c.batch_buffer_bytes;
    m.q_point_1 = c.q_point_1;
    m.q_point_2 = c.q_point_2;
    m.q_point_3 = c.q_point_3;
    return m;
}

std::uint8_t Bno086::oscillator_type()
{
    std::uint8_t t = 0;
    check(depz_bno086_get_oscillator_type(handle(), &t));
    return t;
}

void Bno086::clear_dcd_and_reset(std::chrono::milliseconds timeout)
{
    check(depz_bno086_clear_dcd_and_reset(handle(), ms(timeout)));
}

std::vector<bno086::ErrorRecord> Bno086::errors(std::uint8_t severity)
{
    depz_bno086_error_record c[64];
    std::size_t n = 0;
    check(depz_bno086_get_errors(handle(), severity, c, 64, &n));
    std::vector<bno086::ErrorRecord> out;
    for (std::size_t i = 0; i < n; i++)
        out.push_back({c[i].severity, c[i].seq, c[i].source, c[i].error, c[i].module, c[i].code});
    return out;
}

bno086::Counts Bno086::counts(bno086::SensorId sensor)
{
    depz_bno086_counts c{};
    check(depz_bno086_get_counts(handle(), static_cast<std::uint8_t>(sensor), &c));
    return {c.sensor_id, c.offered, c.accepted, c.on, c.attempted};
}

void Bno086::clear_counts(bno086::SensorId sensor)
{
    check(depz_bno086_clear_counts(handle(), static_cast<std::uint8_t>(sensor)));
}

std::optional<bno086::CommandResponse> Bno086::command(std::uint8_t cmd, byte_span params, bool wait_response)
{
    depz_bno_command_response c{};
    check(depz_bno086_command(handle(), cmd, raw(params), params.size(), wait_response ? &c : nullptr));
    if (!wait_response) return std::nullopt;
    return detail::wrap<bno086::CommandResponse>(c);
}

void Bno086::send_shtp(std::uint8_t channel, byte_span payload)
{
    check(depz_bno086_send_shtp(handle(), channel, raw(payload), payload.size()));
}

// ── VL53L 1D family ─────────────────────────────────────────────────────────

bool Vl53lxMeasurement::plottable() const { return depz_vl53lx_status_plottable(status); }

std::optional<std::int32_t> Vl53lxMeasurement::primary_distance_mm() const
{
    if (!targets.empty()) {
        for (const auto& t : targets)
            if (depz_vl53lx_status_plottable(t.status)) return t.distance_mm;
        return std::nullopt;
    }
    if (!plottable()) return std::nullopt;
    return distance_mm;
}

std::unique_ptr<Vl53lx> Vl53lx::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_vl53lx_open_link(link.release(), &d));
    return std::unique_ptr<Vl53lx>(new Vl53lx(d));
}

Vl53lx::Vl53lx(depz_device* d) : Device(d), measurements_(stream_init(depz_vl53lx_measurements)) {}

std::optional<std::string> Vl53lx::detected_product()
{
    return vlx_product_name(depz_vl53lx_detected_product(handle()));
}

std::string Vl53lx::sensor_class() const { return depz_vl53lx_class_str(depz_vl53lx_get_class(handle())); }

std::vector<vl53lx::DriverKind> Vl53lx::driver_kinds(const std::string& product)
{
    const unsigned k = depz_vl53lx_driver_kinds(vlx_product_to_c(product));
    std::vector<vl53lx::DriverKind> out;
    if (k & DEPZ_VL53LX_DRIVER_ULD) out.push_back(vl53lx::DriverKind::Uld);
    if (k & DEPZ_VL53LX_DRIVER_ULP) out.push_back(vl53lx::DriverKind::Ulp);
    if (k & DEPZ_VL53LX_DRIVER_HISTOGRAM) out.push_back(vl53lx::DriverKind::Histogram);
    return out;
}

void Vl53lx::init(std::optional<vl53lx::DriverKind> driver, const std::optional<std::string>& product)
{
    check(depz_vl53lx_init(handle(), driver ? vlx_kind_to_c(*driver) : static_cast<depz_vl53lx_driver>(0),
                           product ? vlx_product_to_c(*product) : DEPZ_VL53LX_PRODUCT_NONE));
}

bool Vl53lx::initialized() const { return depz_vl53lx_initialized(d_); }
std::optional<std::string> Vl53lx::product() const { return vlx_product_name(depz_vl53lx_product_bound(d_)); }
std::optional<vl53lx::DriverKind> Vl53lx::driver_kind() const { return vlx_kind_from_c(depz_vl53lx_driver_bound(d_)); }
std::string Vl53lx::caveat() const { return depz_vl53lx_caveat(d_); }
std::uint32_t Vl53lx::driver_reach_mm() const { return depz_vl53lx_driver_reach_mm(d_); }
bool Vl53lx::supports(Vl53lxCap cap) const { return depz_vl53lx_supports(d_, static_cast<unsigned>(cap)); }

std::uint16_t Vl53lx::model_id()
{
    std::uint16_t v = 0;
    check(depz_vl53lx_model_id(handle(), &v));
    return v;
}

std::vector<std::string> Vl53lx::modes() const
{
    const char* names[DEPZ_VL53LX_MAX_MODES];
    const std::size_t n = depz_vl53lx_modes(d_, names, DEPZ_VL53LX_MAX_MODES);
    std::vector<std::string> out;
    for (std::size_t i = 0; i < n && i < DEPZ_VL53LX_MAX_MODES; i++) out.emplace_back(names[i]);
    return out;
}

std::pair<int, int> Vl53lx::budget_range() const
{
    int lo = 0, hi = 0;
    check(depz_vl53lx_budget_range(d_, &lo, &hi));
    return {lo, hi};
}

std::vector<int> Vl53lx::budget_choices()
{
    int c[32];
    std::size_t n = 0;
    check(depz_vl53lx_budget_choices(handle(), c, 32, &n));
    return std::vector<int>(c, c + (n < 32 ? n : 32));
}

void Vl53lx::xshut(std::uint8_t action) { check(depz_vl53lx_xshut(handle(), action)); }

vl53lx::Vl53lxInfo Vl53lx::bridge_info()
{
    vl53lx::Vl53lxInfo i{};
    check(depz_vl53lx_bridge_info(handle(), &i));
    return i;
}

void Vl53lx::configure(int budget_ms, int inter_ms, const std::optional<std::string>& mode,
                       std::optional<std::int32_t> offset_mm, std::optional<std::int32_t> xtalk_kcps,
                       std::optional<int> signal_kcps)
{
    check(depz_vl53lx_configure_ex(handle(), budget_ms, inter_ms, mode ? mode->c_str() : nullptr,
                                   offset_mm ? &*offset_mm : nullptr, xtalk_kcps ? &*xtalk_kcps : nullptr,
                                   signal_kcps ? &*signal_kcps : nullptr));
}

std::pair<int, int> Vl53lx::range_timing()
{
    int b = 0, i = 0;
    check(depz_vl53lx_get_range_timing(handle(), &b, &i));
    return {b, i};
}

std::optional<int> Vl53lx::set_mode(const std::string& mode)
{
    int budget_ms = 0;
    check(depz_vl53lx_set_mode(handle(), mode.c_str(), &budget_ms));
    return budget_ms ? std::optional<int>(budget_ms) : std::nullopt;
}

std::optional<std::string> Vl53lx::mode()
{
    const char* m = nullptr;
    check(depz_vl53lx_get_mode(handle(), &m));
    if (!m) return std::nullopt;
    return std::string(m);
}

std::int32_t Vl53lx::offset_mm()
{
    std::int32_t v = 0;
    check(depz_vl53lx_get_offset_mm(handle(), &v));
    return v;
}

void Vl53lx::set_offset_mm(std::int32_t mm) { check(depz_vl53lx_set_offset_mm(handle(), mm)); }

std::int32_t Vl53lx::xtalk_kcps()
{
    std::int32_t v = 0;
    check(depz_vl53lx_get_xtalk_kcps(handle(), &v));
    return v;
}

void Vl53lx::set_xtalk_kcps(std::int32_t kcps) { check(depz_vl53lx_set_xtalk_kcps(handle(), kcps)); }

std::int32_t Vl53lx::calibrate_offset(int target_mm, int nb_samples)
{
    std::int32_t v = 0;
    check(depz_vl53lx_calibrate_offset(handle(), target_mm, nb_samples, &v));
    return v;
}

std::int32_t Vl53lx::calibrate_xtalk(int target_mm, int nb_samples)
{
    std::int32_t v = 0;
    check(depz_vl53lx_calibrate_xtalk(handle(), target_mm, nb_samples, &v));
    return v;
}

std::array<int, 3> Vl53lx::detection_thresholds()
{
    std::array<int, 3> v{};
    check(depz_vl53lx_get_detection_thresholds(handle(), &v[0], &v[1], &v[2]));
    return v;
}

void Vl53lx::set_detection_thresholds(int low_mm, int high_mm, int window)
{
    check(depz_vl53lx_set_detection_thresholds(handle(), low_mm, high_mm, window));
}

int Vl53lx::signal_threshold_kcps()
{
    int v = 0;
    check(depz_vl53lx_get_signal_threshold_kcps(handle(), &v));
    return v;
}

void Vl53lx::set_signal_threshold_kcps(int kcps) { check(depz_vl53lx_set_signal_threshold_kcps(handle(), kcps)); }

int Vl53lx::sigma_threshold_mm()
{
    int v = 0;
    check(depz_vl53lx_get_sigma_threshold_mm(handle(), &v));
    return v;
}

void Vl53lx::set_sigma_threshold_mm(int mm) { check(depz_vl53lx_set_sigma_threshold_mm(handle(), mm)); }

std::pair<int, int> Vl53lx::roi()
{
    int x = 0, y = 0;
    check(depz_vl53lx_get_roi(handle(), &x, &y));
    return {x, y};
}

void Vl53lx::set_roi(int x, int y) { check(depz_vl53lx_set_roi(handle(), x, y)); }

int Vl53lx::roi_center()
{
    int v = 0;
    check(depz_vl53lx_get_roi_center(handle(), &v));
    return v;
}

void Vl53lx::set_roi_center(int spad) { check(depz_vl53lx_set_roi_center(handle(), spad)); }
void Vl53lx::start_temperature_update() { check(depz_vl53lx_start_temperature_update(handle())); }

std::pair<std::uint32_t, bool> Vl53lx::perform_ref_spad_management()
{
    std::uint32_t count = 0;
    bool aperture = false;
    check(depz_vl53lx_perform_ref_spad_management(handle(), &count, &aperture));
    return {count, aperture};
}

void Vl53lx::start_ranging() { check(depz_vl53lx_start_ranging(handle())); }
void Vl53lx::stop_ranging() { check(depz_vl53lx_stop_ranging(handle())); }
bool Vl53lx::ranging() const { return depz_vl53lx_ranging(d_); }

Vl53lxMeasurement Vl53lx::measure_once(std::chrono::milliseconds timeout)
{
    depz_vl53lx_measurement c{};
    check(depz_vl53lx_measure_once(handle(), ms(timeout), &c));
    return vlx_from_c(c);
}

bytes Vl53lx::read_reg(std::uint16_t addr, std::size_t len)
{
    bytes out(len);
    check(depz_vl53lx_read_reg(handle(), addr, reinterpret_cast<std::uint8_t*>(out.data()), len));
    return out;
}

void Vl53lx::write_reg(std::uint16_t addr, byte_span data)
{
    check(depz_vl53lx_write_reg(handle(), addr, raw(data), data.size()));
}

// ── TMF8829 ─────────────────────────────────────────────────────────────────

namespace {

Tmf8829Measurement tmf_measurement_from_c(const depz_tmf8829_measurement& c)
{
    return Tmf8829Measurement{detail::tmf8829_measurement_from_c(c)};
}


Tmf8829Config tmf_config_from_c(const depz_tmf8829_config& c)
{
    Tmf8829Config o;
    o.preset = c.preset;
    const auto* p = reinterpret_cast<const std::byte*>(c.page);
    o.page.assign(p, p + DEPZ_TMF8829_CFG_SIZE);
    o.histograms = c.histograms;
    o.ref_frame = c.ref_frame;
    for (std::size_t i = 0; i < DEPZ_TMF8829_CFG_FIELD_COUNT; ++i)
        o.fields.emplace_back(DEPZ_TMF8829_CFG_FIELDS[i].name, c.fields[i]);
    o.rows = c.rows;
    o.cols = c.cols;
    return o;
}

void tmf_progress_tramp(std::size_t done, std::size_t total, void* user)
{
    auto* ctx = static_cast<ProgressCtx<Tmf8829Progress>*>(user);
    if (ctx->error) return;
    try {
        ctx->fn(done, total);
    } catch (...) {
        ctx->error = std::current_exception();
    }
}

void tmf_frame_tramp(const depz_tmf8829_raw_frame* f, void* user)
{
    const InCallback marker;
    try {
        tmf8829::RawFrame r;
        r.timestamp_us = f->timestamp_us;
        r.edge_us = f->edge_us;
        r.frame_seq = f->frame_seq;
        const auto* d = reinterpret_cast<const std::byte*>(f->data);
        r.data.assign(d, d + f->len);
        static_cast<FnHolder<tmf8829::RawFrame>*>(user)->fn(r);
    } catch (...) {
        callback_threw(static_cast<detail::CallbackHolder*>(user));
    }
}

}  // namespace

std::unique_ptr<Tmf8829> Tmf8829::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_tmf8829_open_link(link.release(), &d));
    return std::unique_ptr<Tmf8829>(new Tmf8829(d));
}

Tmf8829::Tmf8829(depz_device* d) : Device(d), measurements_(stream_init(depz_tmf8829_measurements)) {}

Tmf8829SensorId Tmf8829::init(Tmf8829Progress progress)
{
    char ver[16] = {0};
    std::uint32_t serial = 0;
    ProgressCtx<Tmf8829Progress> ctx{progress, nullptr};
    const int rc = depz_tmf8829_init(handle(), progress ? tmf_progress_tramp : nullptr, progress ? &ctx : nullptr, ver,
                                     sizeof ver, &serial);
    if (ctx.error) std::rethrow_exception(ctx.error);
    check(rc);
    return Tmf8829SensorId{ver, serial};
}

Tmf8829SensorId Tmf8829::sensor_id()
{
    char ver[16] = {0};
    std::uint32_t serial = 0;
    check(depz_tmf8829_sensor_id(handle(), ver, sizeof ver, &serial));
    return Tmf8829SensorId{ver, serial};
}

Tmf8829Config Tmf8829::configure(const std::string& preset, const Tmf8829Options& options)
{
    depz_tmf8829_options o = DEPZ_TMF8829_OPTIONS_INIT;
    o.period_ms = options.period_ms;
    o.kilo_iterations = options.kilo_iterations ? *options.kilo_iterations : -1;
    o.nr_peaks = options.nr_peaks;
    o.signal_strength = options.signal_strength;
    o.noise_strength = options.noise_strength;
    o.xtalk = options.xtalk;
    o.histograms = options.histograms;
    o.ref_frame = options.ref_frame;
    std::vector<depz_tmf8829_field_value> fv;
    fv.reserve(options.fields.size());
    for (const auto& f : options.fields) fv.push_back(depz_tmf8829_field_value{f.first.c_str(), f.second});
    o.fields = fv.empty() ? nullptr : fv.data();
    o.n_fields = fv.size();
    depz_tmf8829_config c{};
    check(depz_tmf8829_configure(handle(), preset.c_str(), &o, &c));
    return tmf_config_from_c(c);
}

std::optional<Tmf8829Config> Tmf8829::config()
{
    depz_tmf8829_config c{};
    const int rc = depz_tmf8829_get_config(handle(), &c);
    if (rc == DEPZ_E_ARG) return std::nullopt;  // no configure() since init()
    check(rc);
    return tmf_config_from_c(c);
}

bytes Tmf8829::config_page()
{
    bytes out(DEPZ_TMF8829_CFG_SIZE);
    check(depz_tmf8829_config_page(handle(), reinterpret_cast<std::uint8_t*>(out.data())));
    return out;
}

bytes Tmf8829::read_registers(std::uint8_t addr, std::size_t len)
{
    bytes out(len);
    check(depz_tmf8829_read_registers(handle(), addr, reinterpret_cast<std::uint8_t*>(out.data()), len));
    return out;
}

void Tmf8829::write_registers(std::uint8_t addr, byte_span data)
{
    check(depz_tmf8829_write_registers(handle(), addr, raw(data), data.size()));
}

tmf8829::Tmf8829Info Tmf8829::bridge_info()
{
    tmf8829::Tmf8829Info i{};
    check(depz_tmf8829_bridge_info(handle(), &i));
    return i;
}

void Tmf8829::start() { check(depz_tmf8829_start(handle())); }
void Tmf8829::stop() { check(depz_tmf8829_stop(handle())); }
bool Tmf8829::streaming() const { return depz_tmf8829_streaming(d_); }

Unsubscribe Tmf8829::on_frame(std::function<void(const tmf8829::RawFrame&)> cb)
{
    auto h = std::make_unique<FnHolder<tmf8829::RawFrame>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_tmf8829_on_frame(handle(), tmf_frame_tramp, h.get(), &token));
    hold(std::move(h));
    return unsubscriber(depz_tmf8829_off_frame, token);
}

Tmf8829StreamStats Tmf8829::stream_stats() const
{
    depz_tmf8829_stream_stats c{};
    depz_tmf8829_get_stream_stats(d_, &c);
    return Tmf8829StreamStats{c.frames_completed, c.frames_dropped, c.frames_discarded, c.frame_parse_errors,
                              c.chunk_errors,     c.measurements,   c.events_swallowed};
}

// ── the stream object (contract 07 §3) ──────────────────────────────────────
// Stream<T> and Subscription<T> are written once, here, over the C item type
// of each stream and its copy into the C++ item; the eight item types are
// instantiated at the end of this block.

namespace {

template <class T>
struct CItem;  // the C item of a Stream<T>, and the copy into T
template <>
struct CItem<DeviceEvent> {
    using C = depz_device_event;
    static DeviceEvent from(const C& c) { return event_from_c(c); }
};
template <>
struct CItem<Sr04Measurement> {
    using C = depz_sr04_measurement;
    static Sr04Measurement from(const C& c) { return measurement_from_c(c); }
};
template <>
struct CItem<Vl53l4cdMeasurement> {
    using C = depz_vl53l4cd_measurement;
    static Vl53l4cdMeasurement from(const C& c) { return l4_from_c(c); }
};
template <>
struct CItem<Vl53l8LiveFrame> {
    using C = depz_vl53l8_live_frame;
    static Vl53l8LiveFrame from(const C& c) { return l8_from_c(c); }
};
template <>
struct CItem<Bno055Sample> {
    using C = depz_bno055_sample;
    static Bno055Sample from(const C& c) { return bno_from_c(c); }
};
template <>
struct CItem<Bno086Report> {
    using C = depz_bno_report;
    static Bno086Report from(const C& c) { return bno086_from_c(c); }
};
template <>
struct CItem<Vl53lxMeasurement> {
    using C = depz_vl53lx_measurement;
    static Vl53lxMeasurement from(const C& c) { return vlx_from_c(c); }
};
template <>
struct CItem<Tmf8829Measurement> {
    using C = depz_tmf8829_measurement;
    static Tmf8829Measurement from(const C& c) { return tmf_measurement_from_c(c); }
};

// A C item a queue handed out is the consumer's to release: a TMF8829
// measurement owns a heap copy of its histogram / ref-SPAD frames
// (depz_tmf8829_measurement_free, a no-op on a zeroed one); nothing else does.
template <class C>
void release_c_item(C&) {}
void release_c_item(depz_tmf8829_measurement& c) { depz_tmf8829_measurement_free(&c); }

// One zeroed C item to receive a queue's next(): on the stack when small, on
// the heap for the multi-KB frames (a TMF8829 measurement is ~150 KB).
// Released when it goes out of scope.
template <class C, bool Small = (sizeof(C) <= 512)>
struct ItemBuffer {
    C c{};
    ~ItemBuffer() { release_c_item(c); }
    C* get() { return &c; }
};
template <class C>
struct ItemBuffer<C, false> {
    std::unique_ptr<C> c = std::make_unique<C>();
    ~ItemBuffer() { release_c_item(*c); }
    C* get() { return c.get(); }
};

}  // namespace

template <class T>
std::optional<T> Subscription<T>::next(std::chrono::milliseconds timeout)
{
    if (!h_) return std::nullopt;
    ItemBuffer<typename CItem<T>::C> c;
    if (!h_->next(c.get(), timeout, closed_, lost_)) return std::nullopt;
    return CItem<T>::from(*c.get());
}

// The C callback of every Stream<T>::on(): the C item becomes a T once, the
// filter sees it, then the callback; one that throws is counted and dropped.
template <class T>
void Stream<T>::trampoline(const void* item, void* user)
{
    auto* h = static_cast<FnHolder<T>*>(user);
    const InCallback marker;
    try {
        const T v = CItem<T>::from(*static_cast<const typename CItem<T>::C*>(item));
        if (!h->where || h->where(v)) h->fn(v);
    } catch (...) {
        callback_threw(h);
    }
}

template <class T>
Stream<T>::Stream(detail::StreamInit init) : dev_(init.dev), get_(init.get)
{
    dev_->finals_.emplace_back(final_, get_);
}

template <class T>
Unsubscribe Stream<T>::on(std::function<void(const T&)> cb, std::function<bool(const T&)> where)
{
    if (!cb) throw ArgumentError(DEPZ_E_ARG, "on(): no callback");
    auto h = std::make_unique<FnHolder<T>>();
    h->fn = std::move(cb);
    h->where = std::move(where);
    depz_stream* s = get_(dev_->handle());
    const int token = depz_stream_on(s, &Stream<T>::trampoline, h.get(), nullptr);
    if (token < 0) check(token);
    dev_->hold(std::move(h));
    return dev_->unsubscriber(s, token);
}

template <class T>
Subscription<T> Stream<T>::subscribe(std::size_t maxsize)
{
    depz_subscription* s = depz_stream_subscribe(get_(dev_->handle()), maxsize);
    if (!s) {
        if (dev_->closed()) dev_->throw_ended();
        throw ArgumentError(DEPZ_E_NOMEM, depz_last_error());
    }
    return Subscription<T>(s);
}

template <class T>
T Stream<T>::next(std::chrono::milliseconds timeout)
{
    ItemBuffer<typename CItem<T>::C> c;
    check(depz_stream_next(get_(dev_->handle()), c.get(), sizeof(typename CItem<T>::C), ms(timeout)));
    return CItem<T>::from(*c.get());
}

template <class T>
std::vector<std::uint64_t> Stream<T>::dropped_counts() const
{
    depz_device* d = dev_->d_.load();
    if (!d) {
        std::lock_guard<std::mutex> g(final_->lock);
        return final_->dropped_counts;
    }
    const depz_stream* s = get_(d);
    std::vector<std::uint64_t> out(depz_stream_dropped_counts(s, nullptr, 0));
    // a subscription may have come or gone in between: report what fits
    const std::size_t n = depz_stream_dropped_counts(s, out.data(), out.size());
    if (n < out.size()) out.resize(n);
    return out;
}

template <class T>
std::uint64_t Stream<T>::parse_errors() const
{
    depz_device* d = dev_->d_.load();
    if (!d) {
        std::lock_guard<std::mutex> g(final_->lock);
        return final_->parse_errors;
    }
    return depz_stream_parse_errors(get_(d));
}

template class Stream<DeviceEvent>;
template class Stream<Sr04Measurement>;
template class Stream<Vl53l4cdMeasurement>;
template class Stream<Vl53l8LiveFrame>;
template class Stream<Bno055Sample>;
template class Stream<Bno086Report>;
template class Stream<Vl53lxMeasurement>;
template class Stream<Tmf8829Measurement>;
template class Subscription<DeviceEvent>;
template class Subscription<Sr04Measurement>;
template class Subscription<Vl53l4cdMeasurement>;
template class Subscription<Vl53l8LiveFrame>;
template class Subscription<Bno055Sample>;
template class Subscription<Bno086Report>;
template class Subscription<Vl53lxMeasurement>;
template class Subscription<Tmf8829Measurement>;

// ── discovery ───────────────────────────────────────────────────────────────

std::vector<SerialPortInfo> list_serial_ports()
{
    depz_port_info* list = nullptr;
    std::size_t n = 0;
    check(depz_list_serial_ports(&list, &n));
    std::vector<SerialPortInfo> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; i++)
        out.push_back(SerialPortInfo{list[i].port, usb_id(list[i].vid), usb_id(list[i].pid), list[i].usb_serial});
    depz_free_port_list(list);
    return out;
}

namespace {
DeviceInfo info_from_c(const depz_device_info& c)
{
    return DeviceInfo{c.port,          mode_of(c.mode),   to_cpp(c.sensor_type), c.software_name,
                      c.fw_version,    c.device_name,     c.serial_number,       usb_id(c.usb_vid),
                      usb_id(c.usb_pid), c.usb_serial};
}
}  // namespace

std::optional<DeviceInfo> probe_port(const std::string& port, std::chrono::milliseconds timeout)
{
    depz_device_info info{};
    const int rc = depz_probe_port(port.c_str(), ms(timeout), &info);
    if (rc == DEPZ_E_NO_DEVICE) return std::nullopt;
    check(rc);
    return info_from_c(info);
}

std::vector<DeviceInfo> list_depz_devices(bool match_usb, std::chrono::milliseconds timeout)
{
    depz_device_info* list = nullptr;
    std::size_t n = 0;
    check(depz_list_depz_devices(match_usb, ms(timeout), &list, &n));
    std::vector<DeviceInfo> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; i++) out.push_back(info_from_c(list[i]));
    depz_free_device_list(list);
    return out;
}

std::unique_ptr<Device> open_device(const OpenOptions& opt)
{
    depz_open_options o = DEPZ_OPEN_OPTIONS_INIT;
    o.port = opt.port ? opt.port->c_str() : nullptr;
    o.serial = opt.serial ? opt.serial->c_str() : nullptr;
    o.index = opt.index ? *opt.index : -1;
    o.timeout_ms = ms(opt.timeout);
    depz_device* d = nullptr;
    check(depz_open_device(&o, &d));
    return wrap_device(d);
}

std::unique_ptr<Sr04> open_sr04(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Sr04*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Sr04>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not an SR04");
}

std::unique_ptr<Vl53l4cd> open_vl53l4cd(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Vl53l4cd*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Vl53l4cd>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a VL53L4CD");
}

std::unique_ptr<Bno055> open_bno055(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Bno055*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Bno055>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a BNO055");
}

std::unique_ptr<Vl53l8> open_vl53l8(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Vl53l8*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Vl53l8>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a multizone ToF");
}

std::unique_ptr<Bno086> open_bno086(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Bno086*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Bno086>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a BNO085 / BNO086");
}

std::unique_ptr<Vl53lx> open_vl53lx(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Vl53lx*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Vl53lx>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a VL53L 1D-family board");
}

std::unique_ptr<Tmf8829> open_tmf8829(const OpenOptions& opt)
{
    auto dev = open_device(opt);
    if (auto* s = dynamic_cast<Tmf8829*>(dev.get())) {
        dev.release();
        return std::unique_ptr<Tmf8829>(s);
    }
    throw WrongTypeError(DEPZ_E_WRONG_TYPE, dev->port() + " is not a TMF8829");
}

}  // namespace depz
