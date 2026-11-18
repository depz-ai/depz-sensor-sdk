// recorder.cpp — the C++ face of the C SDK's live record values and session
// recorder (depz_sensor_io.h, contract 09): each `records::*` builder hands
// the C++ result to the C builder of its kind and wraps the value; the
// SessionRecorder is a depz_session_recorder (the time sync, the hooks, the
// writer thread-safety and the temperature sampler live in C).
#include "depz/recorder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>

#include <depz_sensor_io.h>

#include "detail/c_bridge.hpp"
#include "detail/io_bridge.hpp"

namespace depz {
namespace dataset {
namespace records {

namespace {

// A record value built by C, as a Value.
Value take(depz_json* c)
{
    const detail::CJson v(c);
    if (!v) throw std::bad_alloc();
    return detail::json_from_c(v.get());
}

// A zone array into the C frame's fixed one: what fits (the record cuts to the resolution).
template <class T, std::size_t N, class U>
void copy_zones(T (&out)[N], const std::vector<U>& in)
{
    const std::size_t n = std::min(N, in.size());
    for (std::size_t i = 0; i < n; ++i) out[i] = static_cast<T>(in[i]);
}

void frame_to_c(const vl53l8::Vl53l8Frame& f, depz_vl53l8_frame& c)
{
    c.timestamp_us = f.timestamp_us;
    c.resolution = f.resolution;
    c.silicon_temp_degc = static_cast<std::int8_t>(f.silicon_temp_degc);
    copy_zones(c.distance_mm, f.distance_mm);
    copy_zones(c.target_status, f.target_status);
    copy_zones(c.nb_target_detected, f.nb_target_detected);
    copy_zones(c.signal_per_spad, f.signal_per_spad);
    copy_zones(c.ambient_per_spad, f.ambient_per_spad);
    copy_zones(c.nb_spads_enabled, f.nb_spads_enabled);
    copy_zones(c.range_sigma_mm, f.range_sigma_mm);
    copy_zones(c.reflectance, f.reflectance);
}

}  // namespace

double round1(double x)
{
    if (x == 0.0 || std::isnan(x) || std::isinf(x)) return x;
    // printf's correctly rounded "%.1f": the exact binary value, ties to even, as Python's round()
    char b[48];
    std::snprintf(b, sizeof b, "%.1f", x);
    return std::strtod(b, nullptr);
}

std::string base64(byte_span data)
{
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    const std::size_t n = data.size();
    for (std::size_t i = 0; i < n; i += 3) {
        std::uint32_t w = std::to_integer<std::uint32_t>(data[i]) << 16;
        if (i + 1 < n) w |= std::to_integer<std::uint32_t>(data[i + 1]) << 8;
        if (i + 2 < n) w |= std::to_integer<std::uint32_t>(data[i + 2]);
        out.push_back(A[(w >> 18) & 63]);
        out.push_back(A[(w >> 12) & 63]);
        out.push_back(i + 1 < n ? A[(w >> 6) & 63] : '=');
        out.push_back(i + 2 < n ? A[w & 63] : '=');
    }
    return out;
}

Value sr04(const Sr04Measurement& m)
{
    const depz_sr04_measurement c{m.timestamp_us, m.echo_time_us, m.from_loop};
    return take(depz_dataset_sr04_record(&c));
}

Value vl53l4(const Vl53l4cdMeasurement& m) { return take(depz_dataset_vl53l4_record(&m.r)); }

Value vl53l8(const Vl53l8LiveFrame& f, bool layers)
{
    auto c = std::make_unique<depz_vl53l8_live_frame>();  // zeroed; too big for the stack
    frame_to_c(f.frame, c->f);
    if (f.motion) {
        const Vl53l8Motion& mo = *f.motion;
        c->has_motion = true;
        c->motion.global_indicator_1 = mo.global_indicator_1;
        c->motion.global_indicator_2 = mo.global_indicator_2;
        c->motion.status = mo.status;
        c->motion.nb_of_detected_aggregates = mo.nb_of_detected_aggregates;
        c->motion.nb_of_aggregates = mo.nb_of_aggregates;
        std::copy(mo.motion.begin(), mo.motion.end(), c->motion.motion);
    }
    return take(depz_dataset_vl53l8_live_record(c.get(), layers));
}

Value vl53l8_cnh(byte_span cnh_raw, const CnhSetup& setup)
{
    const depz_vl53l8_cnh_setup c = detail::cnh_to_c(setup);
    return take(depz_dataset_vl53l8_cnh_record(detail::u8p(cnh_raw), cnh_raw.size(), &c));
}

Value vl53lx(const Vl53lxMeasurement& m, const std::string& product, const std::string& driver)
{
    depz_vl53lx_measurement c{};
    c.timestamp_us = m.timestamp_us;
    c.distance_mm = m.distance_mm;
    c.status = m.status;
    c.signal_kcps = m.signal_kcps;
    c.ambient_kcps = m.ambient_kcps;
    c.sigma_mm = m.sigma_mm;
    c.spads = m.spads;
    c.n_targets = std::min<std::size_t>(m.targets.size(), DEPZ_VL53LX_MAX_TARGETS);
    for (std::size_t i = 0; i < c.n_targets; ++i) {
        const Vl53lxTarget& t = m.targets[i];
        depz_vl53lx_target& ct = c.targets[i];
        ct.distance_mm = t.distance_mm;
        ct.status = t.status;
        ct.signal_kcps = t.signal_kcps;
        ct.ambient_kcps = t.ambient_kcps;
        ct.sigma_mm = t.sigma_mm;
        ct.min_range_mm = t.min_range_mm;
        ct.max_range_mm = t.max_range_mm;
    }
    return take(depz_dataset_vl53lx_record(&c, product.c_str(), driver.c_str()));
}

Value bno055(const Bno055Sample& s)
{
    depz_bno055_sample c{};
    c.timestamp_us = s.timestamp_us;
    c.addr = s.addr;
    c.units = s.units;
    const auto put = [&](const std::optional<std::array<double, 3>>& v, double (&out)[3], std::uint32_t flag) {
        if (!v) return;
        std::copy(v->begin(), v->end(), out);
        c.present |= flag;
    };
    put(s.accel, c.accel, DEPZ_BNO055_HAS_ACCEL);
    put(s.mag, c.mag, DEPZ_BNO055_HAS_MAG);
    put(s.gyro, c.gyro, DEPZ_BNO055_HAS_GYRO);
    put(s.euler, c.euler, DEPZ_BNO055_HAS_EULER);
    put(s.linear_accel, c.linear_accel, DEPZ_BNO055_HAS_LINEAR_ACCEL);
    put(s.gravity, c.gravity, DEPZ_BNO055_HAS_GRAVITY);
    if (s.quaternion) {
        std::copy(s.quaternion->begin(), s.quaternion->end(), c.quaternion);
        c.present |= DEPZ_BNO055_HAS_QUATERNION;
    }
    if (s.temperature) {
        c.temperature = *s.temperature;
        c.present |= DEPZ_BNO055_HAS_TEMPERATURE;
    }
    if (s.calibration) {
        c.calibration = *s.calibration;
        c.present |= DEPZ_BNO055_HAS_CALIBRATION;
    }
    return take(depz_dataset_bno055_record(&c));
}

Value tmf8829(const Tmf8829Measurement& m)
{
    auto c = std::make_unique<depz_tmf8829_measurement>();  // zeroed; too big for the stack
    c->timestamp_us = m.timestamp_us;
    c->flags = m.flags;
    c->fp_mode = m.fp_mode;
    c->f_number = m.f_number;
    const auto [rows, cols] = m.shape();
    c->rows = static_cast<std::uint16_t>(rows);
    c->cols = static_cast<std::uint16_t>(cols);
    c->temperature_c = static_cast<std::int8_t>(m.temperature_c());
    c->results = *detail::tmf8829_results_to_c(m.results);
    const std::size_t n = std::min(m.distance_mm.size(), std::size(c->distance_mm));
    std::copy_n(m.distance_mm.begin(), n, c->distance_mm);
    return take(depz_dataset_tmf8829_record(c.get()));
}

Value bno086(const Bno086Report& r)
{
    const depz_bno_report c = detail::bno086_report_to_c(r);
    return take(depz_dataset_bno086_record(&c));
}

Value temperature(double celsius, const std::string& source)
{
    return take(depz_dataset_temperature_record(celsius, source.c_str()));
}

}  // namespace records

// ── SessionRecorder ──────────────────────────────────────────────────────────

struct SessionRecorder::Impl {
    depz_session_recorder* rec;

    explicit Impl(depz_session_recorder* r) : rec(r) {}
    ~Impl() { depz_session_recorder_free(rec); }  // stops first
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
};

SessionRecorder::SessionRecorder(const std::string& path, RecorderOptions options)
{
    depz_session_recorder_options o = DEPZ_SESSION_RECORDER_OPTIONS_INIT;
    o.note = options.note.c_str();  // copied by the C recorder
    o.vl53l8_layers = options.vl53l8_layers;
    o.temperature_hz = options.temperature_hz && *options.temperature_hz > 0 ? *options.temperature_hz : 0.0;
    depz_session_recorder* rec = depz_session_recorder_new(path.c_str(), &o);
    if (!rec) throw std::bad_alloc();
    impl_ = std::make_unique<Impl>(rec);
}

SessionRecorder::~SessionRecorder()
{
    try {
        stop();
    } catch (...) {
    }
}

std::string SessionRecorder::add(Device& device, std::optional<std::string> device_id, int sync_samples)
{
    char id[32];  // the default ids (d0, d1, …); a given id is returned as given
    detail::check(depz_session_recorder_add(impl_->rec, device.c_handle(), device_id ? device_id->c_str() : nullptr,
                                            sync_samples, id, sizeof id));
    // "" is no id: the recorder registered d<n> (as Python's `device_id or f"d{n}"`)
    return device_id && !device_id->empty() ? *device_id : std::string(id);
}

void SessionRecorder::start() { detail::check(depz_session_recorder_start(impl_->rec)); }

void SessionRecorder::stop() { detail::check(depz_session_recorder_stop(impl_->rec)); }

bool SessionRecorder::recording() const { return depz_session_recorder_recording(impl_->rec); }

std::uint64_t SessionRecorder::records_written() const { return depz_session_recorder_records_written(impl_->rec); }

}  // namespace dataset
}  // namespace depz
