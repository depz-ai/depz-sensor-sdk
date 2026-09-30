// device.cpp — the C++ live-hardware layer over the C SDK (depz_sensor_io.h).
#include "depz/device.hpp"

#include <algorithm>
#include <cstring>

#include <depz_sensor_io.h>

namespace depz {

// ── errors ──────────────────────────────────────────────────────────────────

namespace {

[[noreturn]] void throw_rc(int rc)
{
    const std::string msg = depz_last_error();
    switch (rc) {
    case DEPZ_E_ARG:
    case DEPZ_E_NOMEM: throw ArgumentError(rc, msg);
    case DEPZ_E_IO: throw IoError(rc, msg);
    case DEPZ_E_CLOSED: throw DeviceLostError(rc, msg);
    case DEPZ_E_TIMEOUT: throw TimeoutError(rc, msg);
    case DEPZ_E_PROTOCOL: throw ProtocolError(rc, msg);
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

inline void check(int rc)
{
    if (rc != DEPZ_OK) throw_rc(rc);
}

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
    if (!l_) throw DeviceLostError(DEPZ_E_CLOSED, "link moved from");
    check(depz_link_read(l_, buf, sizeof buf, ms(timeout), &got));
    const auto* p = reinterpret_cast<const std::byte*>(buf);
    return bytes(p, p + got);
}

void Link::write(byte_span data)
{
    if (!l_) throw DeviceLostError(DEPZ_E_CLOSED, "link moved from");
    check(depz_link_write(l_, raw(data), data.size()));
}

void Link::close() { depz_link_close(l_); }
bool Link::closed() const { return !l_ || depz_link_closed(l_); }
std::string Link::name() const { return l_ ? depz_link_name(l_) : ""; }
bool Link::replay_exhausted() const { return depz_link_replay_exhausted(l_); }

// ── streams ─────────────────────────────────────────────────────────────────

detail::StreamHandle::~StreamHandle() { depz_stream_close(s); }

bool detail::StreamHandle::next(void* item, std::chrono::milliseconds timeout, bool& closed)
{
    const int rc = depz_stream_next(s, item, ms(timeout));
    if (rc == DEPZ_OK) return true;
    if (rc == DEPZ_E_CLOSED) closed = true;
    else if (rc != DEPZ_E_TIMEOUT) throw_rc(rc);
    return false;
}

std::uint64_t detail::StreamHandle::dropped() const { return depz_stream_dropped_count(s); }

// ── events ──────────────────────────────────────────────────────────────────

namespace {

DeviceEvent event_from_c(const depz_device_event& c)
{
    DeviceEvent e;
    switch (c.type) {
    case DEPZ_DEV_EV_SEQUENCE_ERROR: e.type = DeviceEvent::Type::SequenceError; break;
    case DEPZ_DEV_EV_CRC_ERROR: e.type = DeviceEvent::Type::CrcError; break;
    case DEPZ_DEV_EV_TRASH: e.type = DeviceEvent::Type::Trash; break;
    case DEPZ_DEV_EV_UNSOLICITED_STATUS: e.type = DeviceEvent::Type::UnsolicitedStatus; break;
    case DEPZ_DEV_EV_TEXT: e.type = DeviceEvent::Type::Text; break;
    case DEPZ_DEV_EV_TEMPERATURE: e.type = DeviceEvent::Type::Temperature; break;
    case DEPZ_DEV_EV_DISCONNECTED: e.type = DeviceEvent::Type::Disconnected; break;
    }
    e.cmd = c.cmd;
    e.seq = c.seq;
    e.expected_seq = c.expected_seq;
    e.received_seq = c.received_seq;
    e.by_device = c.by_device;
    e.status = c.status;
    e.timestamp_us = c.timestamp_us;
    e.celsius = c.celsius;
    e.trash_len = c.trash_len;
    const auto* d = reinterpret_cast<const std::byte*>(c.data);
    e.data.assign(d, d + c.data_len);
    e.text = c.text;
    return e;
}

Sr04Measurement measurement_from_c(const depz_sr04_measurement& c)
{
    return Sr04Measurement{c.timestamp_us, c.echo_time_us, c.from_loop};
}

template <class T>
struct FnHolder : detail::CallbackHolder {
    std::function<void(const T&)> fn;
};

void event_tramp(const depz_device_event* ev, void* user)
{
    try {
        static_cast<FnHolder<DeviceEvent>*>(user)->fn(event_from_c(*ev));
    } catch (...) {
        // An exception must not unwind through the C reader thread.
    }
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

void l4_tramp(const depz_vl53l4cd_measurement* m, void* user)
{
    try {
        static_cast<FnHolder<Vl53l4cdMeasurement>*>(user)->fn(l4_from_c(*m));
    } catch (...) {
    }
}

Vl53l8LiveFrame l8_from_c(const depz_vl53l8_live_frame& c)
{
    Vl53l8LiveFrame out;
    auto& f = out.frame;
    const int n = c.f.resolution;
    f.timestamp_us = c.f.timestamp_us;
    f.resolution = n;
    f.silicon_temp_degc = c.f.silicon_temp_degc;
    f.distance_mm.assign(c.f.distance_mm, c.f.distance_mm + n);
    f.target_status.assign(c.f.target_status, c.f.target_status + n);
    f.nb_target_detected.assign(c.f.nb_target_detected, c.f.nb_target_detected + n);
    f.signal_per_spad.assign(c.f.signal_per_spad, c.f.signal_per_spad + n);
    f.ambient_per_spad.assign(c.f.ambient_per_spad, c.f.ambient_per_spad + n);
    f.nb_spads_enabled.assign(c.f.nb_spads_enabled, c.f.nb_spads_enabled + n);
    f.range_sigma_mm_raw.assign(c.f.range_sigma_mm_raw, c.f.range_sigma_mm_raw + n);
    f.reflectance.assign(c.f.reflectance, c.f.reflectance + n);
    if (c.cnh_len) {
        const auto* b = reinterpret_cast<const std::byte*>(c.cnh);
        f.cnh_raw = bytes(b, b + c.cnh_len);
    }
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

void l8_tramp(const depz_vl53l8_live_frame* f, void* user)
{
    try {
        static_cast<FnHolder<Vl53l8LiveFrame>*>(user)->fn(l8_from_c(*f));
    } catch (...) {
    }
}

void progress_tramp(const char* phase, std::size_t done, std::size_t total, void* user)
{
    try {
        (*static_cast<Vl53l8Progress*>(user))(phase, done, total);
    } catch (...) {
    }
}

depz_vl53l8_cnh_setup cnh_to_c(const CnhSetup& s)
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

depz_bno055_units units_to_c(const bno055::Units& u)
{
    depz_bno055_units c{};
    depz_bno055_unpack_units(u.pack(), &c);
    return c;
}

Bno055Sample bno_from_c(const depz_bno055_sample& c)
{
    Bno055Sample s;
    s.timestamp_us = c.timestamp_us;
    s.addr = c.addr;
    const auto* r = reinterpret_cast<const std::byte*>(c.raw);
    s.raw.assign(r, r + c.len);
    s.units = bno055::Units::unpack(depz_bno055_pack_units(&c.units));
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
        s.calibration = bno055::CalibStatus::unpack(depz_bno055_pack_calib_status(&c.calibration));
    return s;
}

void bno_tramp(const depz_bno055_sample* m, void* user)
{
    try {
        static_cast<FnHolder<Bno055Sample>*>(user)->fn(bno_from_c(*m));
    } catch (...) {
    }
}

// The C and C++ report enums list their members in different orders.
bno086::ReportType report_type_from_c(depz_bno_report_type t)
{
    using T = bno086::ReportType;
    switch (t) {
    case DEPZ_BNO_ACCELERATION: return T::Acceleration;
    case DEPZ_BNO_GYROSCOPE: return T::Gyroscope;
    case DEPZ_BNO_MAGNETOMETER: return T::Magnetometer;
    case DEPZ_BNO_UNCAL_GYROSCOPE: return T::UncalibratedGyroscope;
    case DEPZ_BNO_UNCAL_MAGNETOMETER: return T::UncalibratedMagnetometer;
    case DEPZ_BNO_ROTATION_VECTOR: return T::RotationVector;
    case DEPZ_BNO_SCALAR_REPORT: return T::ScalarReport;
    case DEPZ_BNO_TAP_DETECTOR: return T::TapDetector;
    case DEPZ_BNO_STEP_COUNTER: return T::StepCounter;
    case DEPZ_BNO_STEP_DETECTOR: return T::StepDetector;
    case DEPZ_BNO_SIGNIFICANT_MOTION: return T::SignificantMotion;
    case DEPZ_BNO_STABILITY_CLASSIFIER: return T::StabilityClassifier;
    case DEPZ_BNO_SHAKE_DETECTOR: return T::ShakeDetector;
    case DEPZ_BNO_ACTIVITY_CLASSIFIER: return T::PersonalActivityClassifier;
    case DEPZ_BNO_RAW_SENSOR: return T::RawSensor;
    case DEPZ_BNO_GENERIC_EVENT: return T::GenericEvent;
    case DEPZ_BNO_GYRO_INTEGRATED_RV: return T::GyroIntegratedRV;
    default: return T::UnknownReport;
    }
}

Bno086Report bno086_from_c(const depz_bno_report& c)
{
    Bno086Report r;
    r.type = report_type_from_c(c.type);
    r.sensor_id = c.sensor_id;
    r.timestamp_us = c.timestamp_us;
    r.seq = c.seq;
    r.accuracy = c.accuracy;
    r.delay_us = c.delay_us;
    r.x_raw = c.x_raw; r.y_raw = c.y_raw; r.z_raw = c.z_raw;
    r.bias_x_raw = c.bias_x_raw; r.bias_y_raw = c.bias_y_raw; r.bias_z_raw = c.bias_z_raw;
    r.i_raw = c.i_raw; r.j_raw = c.j_raw; r.k_raw = c.k_raw; r.real_raw = c.real_raw;
    r.has_accuracy_raw = c.has_accuracy_raw;
    r.accuracy_raw = c.has_accuracy_raw ? c.accuracy_raw : 0;
    r.value_raw = c.value_raw;
    r.flags = c.flags;
    r.latency_us = c.latency_us;
    r.steps = static_cast<int>(c.steps);
    r.motion = c.motion;
    r.classification = c.classification;
    r.page_number = c.page_number;
    r.end_of_sequence = c.end_of_sequence;
    r.most_likely_state = c.most_likely_state;
    if (c.type == DEPZ_BNO_ACTIVITY_CLASSIFIER) r.confidences.assign(c.confidences, c.confidences + 10);
    r.sensor_timestamp_us = c.sensor_timestamp_us;
    r.temperature_raw = c.temperature_raw;
    r.vx_raw = c.vx_raw; r.vy_raw = c.vy_raw; r.vz_raw = c.vz_raw;
    if (c.data && c.data_len) {
        const auto* b = reinterpret_cast<const std::byte*>(c.data);
        r.data.assign(b, b + c.data_len);
    }
    return r;
}

// The C view of a C++ report, for the C scaling helpers.
depz_bno_report bno086_to_c(const bno086::Report& r)
{
    depz_bno_report c{};
    c.sensor_id = static_cast<std::uint8_t>(r.sensor_id);
    c.x_raw = static_cast<std::int32_t>(r.x_raw); c.y_raw = static_cast<std::int32_t>(r.y_raw);
    c.z_raw = static_cast<std::int32_t>(r.z_raw);
    c.bias_x_raw = static_cast<std::int32_t>(r.bias_x_raw); c.bias_y_raw = static_cast<std::int32_t>(r.bias_y_raw);
    c.bias_z_raw = static_cast<std::int32_t>(r.bias_z_raw);
    c.i_raw = static_cast<std::int32_t>(r.i_raw); c.j_raw = static_cast<std::int32_t>(r.j_raw);
    c.k_raw = static_cast<std::int32_t>(r.k_raw); c.real_raw = static_cast<std::int32_t>(r.real_raw);
    c.has_accuracy_raw = r.has_accuracy_raw;
    c.accuracy_raw = static_cast<std::int32_t>(r.accuracy_raw);
    c.value_raw = r.value_raw;
    c.vx_raw = static_cast<std::int32_t>(r.vx_raw); c.vy_raw = static_cast<std::int32_t>(r.vy_raw);
    c.vz_raw = static_cast<std::int32_t>(r.vz_raw);
    return c;
}

bno086::Feature feature_from_c(const depz_bno_feature& c)
{
    bno086::Feature f;
    f.sensor_id = c.sensor_id;
    f.flags = c.flags;
    f.sensitivity = c.sensitivity;
    f.interval_us = c.interval_us;
    f.batch_us = c.batch_us;
    f.cfg_word = c.cfg_word;
    return f;
}

void bno086_tramp(const depz_bno_report* m, void* user)
{
    try {
        static_cast<FnHolder<Bno086Report>*>(user)->fn(bno086_from_c(*m));
    } catch (...) {
    }
}

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

void vlx_tramp(const depz_vl53lx_measurement* m, void* user)
{
    try {
        static_cast<FnHolder<Vl53lxMeasurement>*>(user)->fn(vlx_from_c(*m));
    } catch (...) {
    }
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

void sr04_tramp(const depz_sr04_measurement* m, void* user)
{
    try {
        static_cast<FnHolder<Sr04Measurement>*>(user)->fn(measurement_from_c(*m));
    } catch (...) {
    }
}

}  // namespace

std::optional<DeviceEvent> DeviceEvent::pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                             bool& closed)
{
    depz_device_event c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return event_from_c(c);
}

std::optional<Sr04Measurement> Sr04Measurement::pull(detail::StreamHandle& h,
                                                     std::chrono::milliseconds timeout, bool& closed)
{
    depz_sr04_measurement c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return measurement_from_c(c);
}

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

std::unique_ptr<Device> Device::promote(std::unique_ptr<Device> dev)
{
    depz_device* d = dev->handle();
    check(depz_device_promote(d));
    auto out = wrap_device(d);
    {
        std::lock_guard<std::mutex> g(dev->held_lock_);
        out->held_ = std::move(dev->held_);
    }
    dev->d_ = nullptr;  // the new wrapper owns the handle now
    return out;
}

Device::~Device() { close(); }

void Device::close()
{
    // Reader first, then the callback storage it may still be running.
    depz_device_close(std::exchange(d_, nullptr));
    std::lock_guard<std::mutex> g(held_lock_);
    held_.clear();
}

depz_device* Device::handle() const
{
    if (!d_) throw DeviceLostError(DEPZ_E_CLOSED, "device closed");
    return d_;
}

void Device::keep(std::unique_ptr<detail::CallbackHolder> h)
{
    std::lock_guard<std::mutex> g(held_lock_);
    held_.push_back(std::move(h));
}

bool Device::closed() const { return !d_ || depz_device_closed(d_); }
std::string Device::port() const { return d_ ? depz_device_port(d_) : ""; }
std::optional<SensorType> Device::sensor_type() const { return to_cpp(depz_device_sensor_type(d_)); }
void Device::set_timeout(std::chrono::milliseconds t) { depz_device_set_timeout_ms(handle(), ms(t)); }

LinkStats Device::stats() const
{
    depz_link_stats s{};
    depz_device_stats(handle(), &s);
    return LinkStats{s.tx_packets, s.rx_packets,    s.tx_bytes, s.rx_bytes,         s.crc_errors,
                     s.header_errors, s.trash_bytes, s.seq_gaps, s.device_seq_errors};
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
};

bool match_tramp(std::uint8_t cmd, const std::uint8_t* p, std::size_t len, void* ctx)
{
    auto* m = static_cast<MatchCtx*>(ctx);
    const auto* b = reinterpret_cast<const std::byte*>(p);
    try {
        if (!(*m->fn)(cmd, byte_span(b, len))) return false;
    } catch (...) {
        return false;
    }
    m->out.assign(b, b + len);
    return true;
}
}  // namespace

bytes Device::request(std::uint8_t cmd, byte_span payload,
                      std::function<bool(std::uint8_t rpt, byte_span payload)> match,
                      std::optional<std::chrono::milliseconds> timeout)
{
    MatchCtx ctx{&match, {}};
    check(depz_device_request(handle(), cmd, raw(payload), payload.size(), match_tramp, &ctx, false,
                              ms(timeout)));
    return ctx.out;
}

void Device::send(std::uint8_t cmd, byte_span payload)
{
    check(depz_device_send(handle(), cmd, raw(payload), payload.size()));
}

Device::Unsubscribe Device::on_event(std::function<void(const DeviceEvent&)> cb)
{
    auto h = std::make_unique<FnHolder<DeviceEvent>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_device_on_event(handle(), event_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_device_off_event(d, token); };
}

Stream<DeviceEvent> Device::events(std::size_t maxsize)
{
    depz_stream* s = depz_device_events(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "events: cannot subscribe");
    return Stream<DeviceEvent>(s);
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

Device::Unsubscribe Sr04::on_measurement(std::function<void(const Sr04Measurement&)> cb)
{
    auto h = std::make_unique<FnHolder<Sr04Measurement>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_sr04_on_measurement(handle(), sr04_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_sr04_off_measurement(d, token); };
}

Stream<Sr04Measurement> Sr04::stream(std::size_t maxsize)
{
    depz_stream* s = depz_sr04_stream(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "sr04 stream: cannot subscribe");
    return Stream<Sr04Measurement>(s);
}

// ── VL53L4CD ────────────────────────────────────────────────────────────────

std::string Vl53l4cdMeasurement::status_text() const { return depz_vl53l4cd_status_text(r.range_status); }

std::optional<Vl53l4cdMeasurement> Vl53l4cdMeasurement::pull(detail::StreamHandle& h,
                                                             std::chrono::milliseconds timeout, bool& closed)
{
    depz_vl53l4cd_measurement c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return l4_from_c(c);
}

std::unique_ptr<Vl53l4cd> Vl53l4cd::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_vl53l4cd_open_link(link.release(), &d));
    return std::unique_ptr<Vl53l4cd>(new Vl53l4cd(d));
}

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
    depz_vl53l4_info c{};
    check(depz_vl53l4cd_bridge_info(handle(), &c));
    vl53l4::Vl53l4Info i;
    i.int_edges = c.int_edges;
    i.slots_skipped = c.slots_skipped;
    i.i2c_errors = c.i2c_errors;
    i.last_i2c_error = c.last_i2c_error;
    i.model_id = c.model_id;
    i.fw_status = c.fw_status;
    i.initialized = c.initialized;
    i.xshut_level = c.xshut_level;
    i.int_level = c.int_level;
    i.i2c_khz = c.i2c_khz;
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

std::uint16_t Vl53l4cd::calibrate_xtalk(std::uint16_t target_mm, std::uint8_t nb_samples)
{
    std::uint16_t v = 0;
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

Device::Unsubscribe Vl53l4cd::on_measurement(std::function<void(const Vl53l4cdMeasurement&)> cb)
{
    auto h = std::make_unique<FnHolder<Vl53l4cdMeasurement>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_vl53l4cd_on_measurement(handle(), l4_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_vl53l4cd_off_measurement(d, token); };
}

Stream<Vl53l4cdMeasurement> Vl53l4cd::measurements(std::size_t maxsize)
{
    depz_stream* s = depz_vl53l4cd_stream(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "vl53l4cd stream: cannot subscribe");
    return Stream<Vl53l4cdMeasurement>(s);
}

Vl53l4cdMeasurement Vl53l4cd::get_measurement(std::chrono::milliseconds timeout)
{
    depz_vl53l4cd_measurement m{};
    check(depz_vl53l4cd_get_measurement(handle(), ms(timeout), &m));
    return l4_from_c(m);
}

std::uint64_t Vl53l4cd::stream_parse_errors() const { return depz_vl53l4cd_stream_parse_errors(d_); }

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

std::optional<Vl53l8LiveFrame> Vl53l8LiveFrame::pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                                     bool& closed)
{
    auto c = std::make_unique<depz_vl53l8_live_frame>();
    if (!h.next(c.get(), timeout, closed)) return std::nullopt;
    return l8_from_c(*c);
}

void CnhSetup::init_config(int start_bin, int num_bins, int sub_sample)
{
    depz_vl53l8_cnh_setup c{};
    depz_vl53l8_cnh_init_config(&c, start_bin, num_bins, sub_sample);
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

Vl53l8Model Vl53l8::model() const { return static_cast<Vl53l8Model>(depz_vl53l8_get_model(d_)); }

bool Vl53l8::is_alive()
{
    bool alive = false;
    check(depz_vl53l8_is_alive(handle(), &alive, nullptr, nullptr));
    return alive;
}

void Vl53l8::init(Vl53l8Progress progress)
{
    check(depz_vl53l8_init(handle(), progress ? progress_tramp : nullptr, progress ? &progress : nullptr));
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

double Vl53l8::xtalk_margin()
{
    double v = 0;
    check(depz_vl53l8_get_xtalk_margin(handle(), &v));
    return v;
}

void Vl53l8::set_xtalk_margin(double kcps) { check(depz_vl53l8_set_xtalk_margin(handle(), kcps)); }

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
    depz_vl53l8_threshold c[64];
    check(depz_vl53l8_get_detection_thresholds(handle(), c));
    std::vector<vl53l8::DetectionThreshold> out(64);
    for (int k = 0; k < 64; k++)
        out[k] = vl53l8::DetectionThreshold{c[k].low_thresh, c[k].high_thresh, c[k].measurement,
                                            c[k].type,       c[k].zone_num,    c[k].operation};
    return out;
}

void Vl53l8::set_detection_thresholds(const std::vector<vl53l8::DetectionThreshold>& thresholds)
{
    std::vector<depz_vl53l8_threshold> c;
    for (const auto& t : thresholds)
        c.push_back(depz_vl53l8_threshold{t.low_thresh, t.high_thresh, t.measurement, t.type, t.zone_num, t.operation});
    check(depz_vl53l8_set_detection_thresholds(handle(), c.data(), c.size()));
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

void Vl53l8::start_ranging() { check(depz_vl53l8_start_ranging(handle())); }
void Vl53l8::stop_ranging() { check(depz_vl53l8_stop_ranging(handle())); }

Device::Unsubscribe Vl53l8::on_frame(std::function<void(const Vl53l8LiveFrame&)> cb)
{
    auto h = std::make_unique<FnHolder<Vl53l8LiveFrame>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_vl53l8_on_frame(handle(), l8_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_vl53l8_off_frame(d, token); };
}

Stream<Vl53l8LiveFrame> Vl53l8::frames(std::size_t maxsize)
{
    depz_stream* s = depz_vl53l8_frames(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "vl53l8 frames: cannot subscribe");
    return Stream<Vl53l8LiveFrame>(s);
}

Vl53l8LiveFrame Vl53l8::get_frame(std::chrono::milliseconds timeout)
{
    auto c = std::make_unique<depz_vl53l8_live_frame>();
    check(depz_vl53l8_get_frame(handle(), ms(timeout), c.get()));
    return l8_from_c(*c);
}

std::uint64_t Vl53l8::frame_parse_errors() const { return depz_vl53l8_frame_parse_errors(d_); }
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
    depz_vl53l7_info c{};
    check(depz_vl53l7_bridge_info(handle(), &c));
    vl53l7::Vl53l7Info i;
    i.int_edges = c.int_edges;
    i.frames_dropped = c.frames_dropped;
    i.i2c_errors = c.i2c_errors;
    i.last_i2c_error = c.last_i2c_error;
    i.lpn_level = c.lpn_level;
    i.int_level = c.int_level;
    i.i2c_khz = c.i2c_khz;
    i.frame_size = c.frame_size;
    i.streaming = c.streaming;
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

std::optional<Bno055Sample> Bno055Sample::pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                               bool& closed)
{
    depz_bno055_sample c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return bno_from_c(c);
}

std::unique_ptr<Bno055> Bno055::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_bno055_open_link(link.release(), &d));
    return std::unique_ptr<Bno055>(new Bno055(d));
}

bno055::Bno055Info Bno055::bridge_info()
{
    depz_bno055_info c{};
    check(depz_bno055_bridge_info(handle(), &c));
    bno055::Bno055Info i;
    i.i2c_addr = c.i2c_addr;
    i.chip_id = c.chip_id;
    i.acc_id = c.acc_id;
    i.mag_id = c.mag_id;
    i.gyr_id = c.gyr_id;
    i.sw_rev = c.sw_rev;
    i.bl_rev = c.bl_rev;
    i.initialized = c.initialized;
    i.int_level = c.int_level;
    i.int_edges = c.int_edges;
    i.read_min_us = c.read_min_us;
    i.read_max_us = c.read_max_us;
    i.read_avg_us = c.read_avg_us;
    i.tx_dropped = c.tx_dropped;
    i.i2c_errors = c.i2c_errors;
    i.slots_skipped = c.slots_skipped;
    i.bus_recoveries = c.bus_recoveries;
    i.last_i2c_error = c.last_i2c_error;
    i.sensor_resets = c.sensor_resets;
    i.loop_max_us = c.loop_max_us;
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
    depz_bno055_units c{};
    check(depz_bno055_get_units(handle(), &c));
    return bno055::Units::unpack(depz_bno055_pack_units(&c));
}

void Bno055::set_units(const bno055::Units& units)
{
    depz_bno055_units c = units_to_c(units);
    check(depz_bno055_set_units(handle(), &c));
}

bno055::AxisRemap Bno055::axis_remap()
{
    depz_bno055_axis_remap c{};
    check(depz_bno055_get_axis_remap(handle(), &c));
    std::uint8_t cfg = 0, sign = 0;
    depz_bno055_pack_axis_remap(&c, &cfg, &sign);
    return bno055::AxisRemap::unpack(cfg, sign);
}

namespace {
depz_bno055_axis_remap remap_to_c(const bno055::AxisRemap& r)
{
    auto p = r.pack();
    if (!p) throw ArgumentError(DEPZ_E_ARG, "bno055: axes must be a permutation of x, y, z");
    depz_bno055_axis_remap c{};
    depz_bno055_unpack_axis_remap(p->first, p->second, &c);
    return c;
}

depz_bno055_calib_profile profile_to_c(const bno055::CalibrationProfile& p)
{
    depz_bno055_calib_profile c{};
    const bytes b = p.pack();
    depz_bno055_unpack_calib_profile(reinterpret_cast<const std::uint8_t*>(b.data()), b.size(), &c);
    return c;
}
}  // namespace

void Bno055::set_axis_remap(const bno055::AxisRemap& remap)
{
    depz_bno055_axis_remap c = remap_to_c(remap);
    check(depz_bno055_set_axis_remap(handle(), &c));
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
    depz_bno055_units u = units_to_c(units);
    depz_bno055_axis_remap r{};
    depz_bno055_calib_profile p{};
    if (remap) r = remap_to_c(*remap);
    if (calibration) p = profile_to_c(*calibration);
    check(depz_bno055_configure(handle(), static_cast<std::uint8_t>(mode), &u, remap ? &r : nullptr,
                                calibration ? &p : nullptr));
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
    depz_bno055_calib_status c{};
    check(depz_bno055_calibration_status(handle(), &c));
    return bno055::CalibStatus::unpack(depz_bno055_pack_calib_status(&c));
}

bno055::CalibrationProfile Bno055::calibration_profile()
{
    depz_bno055_calib_profile c{};
    check(depz_bno055_read_calibration_profile(handle(), &c));
    std::uint8_t b[DEPZ_BNO055_CALIB_PROFILE_LEN];
    depz_bno055_pack_calib_profile(&c, b);
    return *bno055::CalibrationProfile::unpack(byte_span(reinterpret_cast<const std::byte*>(b), sizeof b));
}

void Bno055::write_calibration_profile(const bno055::CalibrationProfile& p)
{
    depz_bno055_calib_profile c = profile_to_c(p);
    check(depz_bno055_write_calibration_profile(handle(), &c));
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
    depz_bno055_accel_config c{};
    check(depz_bno055_get_accel_config(handle(), &c));
    return bno055::AccelConfig::unpack(depz_bno055_pack_accel_config(&c));
}

void Bno055::set_accel_config(const bno055::AccelConfig& cfg)
{
    depz_bno055_accel_config c{};
    depz_bno055_unpack_accel_config(cfg.pack(), &c);
    check(depz_bno055_set_accel_config(handle(), &c));
}

bno055::GyroConfig Bno055::gyro_config()
{
    depz_bno055_gyro_config c{};
    check(depz_bno055_get_gyro_config(handle(), &c));
    std::uint8_t b[2];
    depz_bno055_pack_gyro_config(&c, b);
    return *bno055::GyroConfig::unpack(byte_span(reinterpret_cast<const std::byte*>(b), 2));
}

void Bno055::set_gyro_config(const bno055::GyroConfig& cfg)
{
    depz_bno055_gyro_config c{};
    const bytes b = cfg.pack();
    depz_bno055_unpack_gyro_config(reinterpret_cast<const std::uint8_t*>(b.data()), &c);
    check(depz_bno055_set_gyro_config(handle(), &c));
}

bno055::MagConfig Bno055::mag_config()
{
    depz_bno055_mag_config c{};
    check(depz_bno055_get_mag_config(handle(), &c));
    return bno055::MagConfig::unpack(depz_bno055_pack_mag_config(&c));
}

void Bno055::set_mag_config(const bno055::MagConfig& cfg)
{
    depz_bno055_mag_config c{};
    depz_bno055_unpack_mag_config(cfg.pack(), &c);
    check(depz_bno055_set_mag_config(handle(), &c));
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

Device::Unsubscribe Bno055::on_sample(std::function<void(const Bno055Sample&)> cb)
{
    auto h = std::make_unique<FnHolder<Bno055Sample>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_bno055_on_sample(handle(), bno_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_bno055_off_sample(d, token); };
}

Stream<Bno055Sample> Bno055::samples(std::size_t maxsize)
{
    depz_stream* s = depz_bno055_samples(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "bno055 samples: cannot subscribe");
    return Stream<Bno055Sample>(s);
}

Bno055Sample Bno055::get_sample(std::chrono::milliseconds timeout)
{
    depz_bno055_sample c{};
    check(depz_bno055_get_sample(handle(), ms(timeout), &c));
    return bno_from_c(c);
}

std::uint64_t Bno055::stream_parse_errors() const { return depz_bno055_stream_parse_errors(d_); }

// ── BNO085 / BNO086 ─────────────────────────────────────────────────────────

std::optional<int> bno086::q_point(std::uint8_t sensor_id)
{
    const int q = depz_bno_q_point(sensor_id);
    if (q < 0) return std::nullopt;
    return q;
}

std::array<double, 3> Bno086Report::xyz() const
{
    const depz_bno_report c = bno086_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_xyz(&c, v.data());
    return v;
}

std::array<double, 3> Bno086Report::bias() const
{
    const depz_bno_report c = bno086_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_bias(&c, v.data());
    return v;
}

std::array<double, 4> Bno086Report::quaternion() const
{
    const depz_bno_report c = bno086_to_c(*this);
    std::array<double, 4> v{};
    depz_bno_report_quaternion(&c, v.data());
    return v;
}

std::optional<double> Bno086Report::accuracy_rad() const
{
    const depz_bno_report c = bno086_to_c(*this);
    double v = 0;
    if (!depz_bno_report_accuracy_rad(&c, &v)) return std::nullopt;
    return v;
}

std::array<double, 3> Bno086Report::angular_velocity() const
{
    const depz_bno_report c = bno086_to_c(*this);
    std::array<double, 3> v{};
    depz_bno_report_angular_velocity(&c, v.data());
    return v;
}

double Bno086Report::scalar() const
{
    const depz_bno_report c = bno086_to_c(*this);
    return depz_bno_report_scalar(&c);
}

std::optional<Bno086Report> Bno086Report::pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                               bool& closed)
{
    depz_bno_report c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return bno086_from_c(c);
}

std::unique_ptr<Bno086> Bno086::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_bno086_open_link(link.release(), &d));
    return std::unique_ptr<Bno086>(new Bno086(d));
}

void Bno086::hardware_reset(std::chrono::milliseconds timeout)
{
    check(depz_bno086_hardware_reset(handle(), ms(timeout)));
}

void Bno086::wake() { check(depz_bno086_wake(handle())); }

bytes Bno086::advertisement()
{
    std::size_t n = 0;
    check(depz_bno086_advertisement(handle(), nullptr, 0, &n));
    bytes out(n);
    check(depz_bno086_advertisement(handle(), reinterpret_cast<std::uint8_t*>(out.data()), n, &n));
    out.resize(std::min(n, out.size()));
    return out;
}

bno086::ProductId Bno086::product_id()
{
    depz_bno_product_id c{};
    check(depz_bno086_product_id(handle(), &c));
    bno086::ProductId p;
    p.reset_cause = c.reset_cause;
    p.sw_version_major = c.sw_version_major;
    p.sw_version_minor = c.sw_version_minor;
    p.sw_part_number = c.sw_part_number;
    p.sw_build_number = c.sw_build_number;
    p.sw_version_patch = c.sw_version_patch;
    return p;
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

Device::Unsubscribe Bno086::on_report(std::function<void(const Bno086Report&)> cb)
{
    auto h = std::make_unique<FnHolder<Bno086Report>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_bno086_on_report(handle(), bno086_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_bno086_off_report(d, token); };
}

Stream<Bno086Report> Bno086::reports(std::size_t maxsize)
{
    depz_stream* s = depz_bno086_reports(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "bno086 reports: cannot subscribe");
    return Stream<Bno086Report>(s);
}

Bno086Report Bno086::get_report(std::chrono::milliseconds timeout)
{
    depz_bno_report c{};
    check(depz_bno086_get_report(handle(), ms(timeout), &c));
    return bno086_from_c(c);
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
    depz_bno_metadata c{};
    check(depz_bno086_get_metadata(handle(), static_cast<std::uint8_t>(sensor), &c));
    bno086::SensorMetadata m;
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
    bno086::CommandResponse r;
    r.seq = c.seq;
    r.command = c.command;
    r.command_seq = c.command_seq;
    r.response_seq = c.response_seq;
    std::copy(c.r, c.r + 11, r.r.begin());
    return r;
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

std::optional<Vl53lxMeasurement> Vl53lxMeasurement::pull(detail::StreamHandle& h, std::chrono::milliseconds timeout,
                                                         bool& closed)
{
    depz_vl53lx_measurement c{};
    if (!h.next(&c, timeout, closed)) return std::nullopt;
    return vlx_from_c(c);
}

std::unique_ptr<Vl53lx> Vl53lx::open(Link link)
{
    depz_device* d = nullptr;
    check(depz_vl53lx_open_link(link.release(), &d));
    return std::unique_ptr<Vl53lx>(new Vl53lx(d));
}

std::optional<std::string> Vl53lx::detected_product()
{
    return vlx_product_name(depz_vl53lx_detected_product(handle()));
}

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
    depz_vl53lx_info c{};
    check(depz_vl53lx_bridge_info(handle(), &c));
    vl53lx::Vl53lxInfo i;
    i.int_edges = c.int_edges;
    i.slots_skipped = c.slots_skipped;
    i.i2c_errors = c.i2c_errors;
    i.last_i2c_error = c.last_i2c_error;
    i.xshut_level = c.xshut_level;
    i.int_level = c.int_level;
    i.i2c_khz = c.i2c_khz;
    i.addr_width = c.addr_width;
    i.n_clear = c.n_clear;
    i.frames_dropped = c.frames_dropped;
    return i;
}

void Vl53lx::configure(int budget_ms, int inter_ms, const std::optional<std::string>& mode,
                       std::optional<std::int32_t> offset_mm, std::optional<std::int32_t> xtalk_kcps)
{
    check(depz_vl53lx_configure(handle(), budget_ms, inter_ms, mode ? mode->c_str() : nullptr,
                                offset_mm ? &*offset_mm : nullptr, xtalk_kcps ? &*xtalk_kcps : nullptr));
}

std::pair<int, int> Vl53lx::range_timing()
{
    int b = 0, i = 0;
    check(depz_vl53lx_get_range_timing(handle(), &b, &i));
    return {b, i};
}

void Vl53lx::set_mode(const std::string& mode) { check(depz_vl53lx_set_mode(handle(), mode.c_str())); }

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

Device::Unsubscribe Vl53lx::on_measurement(std::function<void(const Vl53lxMeasurement&)> cb)
{
    auto h = std::make_unique<FnHolder<Vl53lxMeasurement>>();
    h->fn = std::move(cb);
    int token = 0;
    check(depz_vl53lx_on_measurement(handle(), vlx_tramp, h.get(), &token));
    keep(std::move(h));
    depz_device* d = d_;
    return [d, token] { depz_vl53lx_off_measurement(d, token); };
}

Stream<Vl53lxMeasurement> Vl53lx::measurements(std::size_t maxsize)
{
    depz_stream* s = depz_vl53lx_measurements(handle(), maxsize);
    if (!s) throw ArgumentError(DEPZ_E_NOMEM, "vl53lx measurements: cannot subscribe");
    return Stream<Vl53lxMeasurement>(s);
}

Vl53lxMeasurement Vl53lx::get_measurement(std::chrono::milliseconds timeout)
{
    depz_vl53lx_measurement c{};
    check(depz_vl53lx_get_measurement(handle(), ms(timeout), &c));
    return vlx_from_c(c);
}

std::uint64_t Vl53lx::stream_parse_errors() const { return depz_vl53lx_stream_parse_errors(d_); }

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

}  // namespace depz
