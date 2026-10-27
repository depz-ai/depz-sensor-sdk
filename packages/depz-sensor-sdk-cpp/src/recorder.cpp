#include "depz/recorder.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <depz_sensor_io.h>
#include "depz/identity.hpp"

namespace depz {
namespace dataset {
namespace records {

namespace {

template <class T>
Value ints(const std::vector<T>& a, std::size_t n = std::size_t(-1))
{
    Value v = Value::array();
    n = std::min(n, a.size());
    for (std::size_t i = 0; i < n; ++i) v.push(static_cast<std::int64_t>(a[i]));
    return v;
}

template <class A>
Value doubles(const A& a, std::size_t n = std::size_t(-1))
{
    Value v = Value::array();
    n = std::min(n, static_cast<std::size_t>(a.size()));
    for (std::size_t i = 0; i < n; ++i) v.push(static_cast<double>(a[i]));
    return v;
}

Value three(const std::array<double, 3>& a) { return Value(Value::Array{a[0], a[1], a[2]}); }

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
    Value v = Value::object();
    v.set("echo_us", static_cast<std::int64_t>(m.echo_time_us));
    v.set("source", m.from_loop ? "loop" : "once");
    return v;
}

Value vl53l4(const Vl53l4cdMeasurement& m)
{
    Value v = Value::object();
    v.set("range_status", m.r.range_status);
    v.set("distance_mm", m.r.distance_mm);
    v.set("sigma_mm", m.r.sigma_mm);
    v.set("signal_rate_kcps", m.r.signal_rate_kcps);
    v.set("ambient_rate_kcps", m.r.ambient_rate_kcps);
    v.set("number_of_spad", m.r.number_of_spad);
    v.set("stream_count", m.r.stream_count);
    return v;
}

Value vl53l8(const Vl53l8LiveFrame& f, bool layers)
{
    const auto& fr = f.frame;
    const std::size_t n = fr.resolution < 0 ? 0 : static_cast<std::size_t>(fr.resolution);
    Value v = Value::object();
    v.set("resolution", fr.resolution);
    v.set("silicon_temp_degc", fr.silicon_temp_degc);
    v.set("distance_mm", ints(fr.distance_mm, n));
    v.set("target_status", ints(fr.target_status, n));
    v.set("nb_target_detected", ints(fr.nb_target_detected, n));
    if (layers) {
        v.set("signal_per_spad", doubles(fr.signal_per_spad, n));
        v.set("ambient_per_spad", doubles(fr.ambient_per_spad, n));
        v.set("range_sigma_mm", doubles(fr.range_sigma_mm, n));
        v.set("reflectance", doubles(fr.reflectance, n));
    }
    if (f.motion) {
        const Vl53l8Motion& mo = *f.motion;
        Value m = Value::object();
        m.set("global_indicator_1", mo.global_indicator_1);
        m.set("global_indicator_2", mo.global_indicator_2);
        m.set("status", static_cast<std::int64_t>(mo.status));
        m.set("nb_of_detected_aggregates", static_cast<std::int64_t>(mo.nb_of_detected_aggregates));
        m.set("nb_of_aggregates", static_cast<std::int64_t>(mo.nb_of_aggregates));
        Value arr = Value::array();
        for (std::uint32_t x : mo.motion) arr.push(x);
        m.set("motion", std::move(arr));
        v.set("motion", std::move(m));
    }
    return v;
}

Value vl53l8_cnh(byte_span cnh_raw, const CnhSetup& setup)
{
    Value v = Value::object();
    v.set("nb_aggregates", static_cast<std::int64_t>(setup.nb_of_aggregates));
    v.set("feature_length", static_cast<std::int64_t>(setup.feature_length));
    v.set("sum_span", static_cast<std::int64_t>(setup.sum_span));
    // Python: ref_bin_offset // 2048 (floor division)
    v.set("start_bin", static_cast<std::int64_t>(std::floor(setup.ref_bin_offset / 2048.0)));
    v.set("cnh_raw_b64", base64(cnh_raw));
    return v;
}

Value vl53lx(const Vl53lxMeasurement& m, const std::string& product, const std::string& driver)
{
    Value v = Value::object();
    v.set("product", product);
    v.set("driver", driver);
    v.set("status", m.status);
    v.set("distance_mm", static_cast<std::int64_t>(m.distance_mm));
    v.set("sigma_mm", m.sigma_mm);
    v.set("signal_kcps", m.signal_kcps);
    v.set("ambient_kcps", m.ambient_kcps);
    v.set("spads", m.spads);
    if (!m.targets.empty()) {
        Value targets = Value::array();
        for (const Vl53lxTarget& t : m.targets) {
            Value tv = Value::object();
            tv.set("distance_mm", static_cast<std::int64_t>(t.distance_mm));
            tv.set("status", t.status);
            tv.set("signal_kcps", t.signal_kcps);
            targets.push(std::move(tv));
        }
        v.set("targets", std::move(targets));
    }
    return v;
}

Value bno055(const Bno055Sample& s)
{
    Value v = Value::object();
    v.set("unit_sel", static_cast<std::int64_t>(s.units.pack()));
    if (s.quaternion) v.set("quaternion", doubles(*s.quaternion));
    if (s.euler) v.set("euler", doubles(*s.euler));
    if (s.accel) v.set("accel", doubles(*s.accel));
    if (s.gyro) v.set("gyro", doubles(*s.gyro));
    if (s.mag) v.set("mag", doubles(*s.mag));
    if (s.linear_accel) v.set("linear_accel", doubles(*s.linear_accel));
    if (s.gravity) v.set("gravity", doubles(*s.gravity));
    if (s.temperature) v.set("temperature", *s.temperature);
    if (s.calibration) {
        const auto& c = *s.calibration;
        v.set("calib", Value(Value::Array{static_cast<std::int64_t>(c.system), static_cast<std::int64_t>(c.gyro),
                                          static_cast<std::int64_t>(c.accel), static_cast<std::int64_t>(c.mag)}));
    }
    return v;
}

Value tmf8829(const Tmf8829Measurement& m)
{
    const auto& r = m.results;
    const auto [rows, cols] = m.shape();
    Value v = Value::object();
    v.set("fp_mode", static_cast<std::int64_t>(m.fp_mode));
    v.set("rows", static_cast<std::int64_t>(rows));
    v.set("cols", static_cast<std::int64_t>(cols));
    v.set("peaks", static_cast<std::int64_t>(r.peaks));
    v.set("f_number", m.f_number);
    v.set("flags", static_cast<std::int64_t>(m.flags));
    v.set("temperature_c", m.temperature_c());
    Value dist = Value::array();
    for (double x : m.distance_mm) dist.push(round1(x));
    v.set("distance_mm", std::move(dist));
    v.set("snr", r.snr ? ints(*r.snr) : Value::array());
    if (r.signal) v.set("signal", ints(*r.signal));
    if (r.noise) v.set("noise", ints(*r.noise));
    if (r.xtalk) v.set("xtalk", ints(*r.xtalk));
    return v;
}

Value bno086(const Bno086Report& r)
{
    using RT = depz::bno086::ReportType;
    Value v = Value::object();
    v.set("type", [&] {
        switch (r.type) {
            case RT::Acceleration: return "Acceleration";
            case RT::Gyroscope: return "Gyroscope";
            case RT::Magnetometer: return "Magnetometer";
            case RT::UncalibratedGyroscope: return "UncalibratedGyroscope";
            case RT::UncalibratedMagnetometer: return "UncalibratedMagnetometer";
            case RT::RotationVector: return "RotationVector";
            case RT::ScalarReport: return "ScalarReport";
            case RT::TapDetector: return "TapDetector";
            case RT::StepCounter: return "StepCounter";
            case RT::StepDetector: return "StepDetector";
            case RT::SignificantMotion: return "SignificantMotion";
            case RT::StabilityClassifier: return "StabilityClassifier";
            case RT::ShakeDetector: return "ShakeDetector";
            case RT::GenericEvent: return "GenericEvent";
            case RT::PersonalActivityClassifier: return "PersonalActivityClassifier";
            case RT::RawSensor: return "RawSensor";
            case RT::GyroIntegratedRV: return "GyroIntegratedRV";
            case RT::MotionRequest: return "MotionRequest";
            case RT::FlushCompleted: return "FlushCompleted";
            case RT::UnknownReport: break;
        }
        return "UnknownReport";
    }());
    v.set("sensor_id", r.sensor_id);
    const auto acc = [&] { v.set("accuracy", r.accuracy); };
    switch (r.type) {
        case RT::Acceleration: case RT::Gyroscope: case RT::Magnetometer:
        case RT::UncalibratedGyroscope: case RT::UncalibratedMagnetometer: {
            const auto a = r.xyz();
            v.set("x", a[0]);
            v.set("y", a[1]);
            v.set("z", a[2]);
            acc();
            if (r.type == RT::UncalibratedGyroscope || r.type == RT::UncalibratedMagnetometer) v.set("bias", three(r.bias()));
            break;
        }
        case RT::RotationVector: {
            const auto q = r.quaternion();
            v.set("i", q[0]);
            v.set("j", q[1]);
            v.set("k", q[2]);
            v.set("real", q[3]);
            if (const auto a = r.accuracy_rad()) v.set("accuracy_rad", *a);
            acc();
            break;
        }
        case RT::GyroIntegratedRV: {
            const auto q = r.quaternion();
            v.set("i", q[0]);
            v.set("j", q[1]);
            v.set("k", q[2]);
            v.set("real", q[3]);
            v.set("angular_velocity", three(r.angular_velocity()));
            break;
        }
        case RT::ScalarReport:
            acc();
            v.set("value", r.scalar());
            break;
        case RT::TapDetector:
            acc();
            v.set("flags", r.flags);
            v.set("double_tap", (r.flags & 0x40) != 0);
            break;
        case RT::StepCounter:
            acc();
            v.set("steps", r.steps);
            v.set("latency_us", r.latency_us);
            break;
        case RT::StepDetector:
            acc();
            v.set("latency_us", r.latency_us);
            break;
        case RT::SignificantMotion:
            acc();
            v.set("motion", r.motion);
            break;
        case RT::StabilityClassifier:
            acc();
            v.set("classification", r.classification);
            break;
        case RT::MotionRequest:
            acc();
            v.set("intent", r.intent);
            v.set("request", r.request);
            break;
        case RT::ShakeDetector:
            acc();
            v.set("flags", r.flags);
            break;
        case RT::PersonalActivityClassifier:
            acc();
            v.set("most_likely_state", r.most_likely_state);
            v.set("confidences", ints(r.confidences));
            break;
        case RT::RawSensor:
            acc();
            // the raw ADC streams (0x14..0x16) keep their integer counts
            if (r.sensor_id >= 0x14 && r.sensor_id <= 0x16) {
                v.set("x_raw", r.x_raw);
                v.set("y_raw", r.y_raw);
                v.set("z_raw", r.z_raw);
                v.set("temperature_raw", r.temperature_raw);
                v.set("sensor_timestamp_us", r.sensor_timestamp_us);
            }
            break;
        case RT::GenericEvent:
            acc();
            v.set("value_raw", r.value_raw);
            break;
        case RT::FlushCompleted:
            v.set("flushed_sensor", r.flushed_sensor);
            break;
        case RT::UnknownReport:
            break;
    }
    v.set("timestamp_us", r.timestamp_us);
    return v;
}

Value temperature(double celsius, const std::string& source)
{
    Value v = Value::object();
    v.set("celsius", celsius);
    v.set("source", source);
    return v;
}

}  // namespace records

// ── SessionRecorder ──────────────────────────────────────────────────────────

struct SessionRecorder::Impl : std::enable_shared_from_this<Impl> {
    struct Entry {
        Device* device;
        std::string id;
        std::int64_t offset_us;
        Value meta;
        std::vector<Device::Unsubscribe> hooks;
        std::optional<double> silicon;  // the sensor die's last temperature (VL53L8 frames)
    };

    const std::string path;
    const RecorderOptions options;
    std::mutex lock;  // the writer, the entries' silicon readings
    std::unique_ptr<Writer> writer;
    std::vector<std::unique_ptr<Entry>> entries;
    std::thread thread;
    std::mutex tlock;
    std::condition_variable tcond;
    bool stop_flag = false;

    Impl(std::string p, RecorderOptions o) : path(std::move(p)), options(std::move(o)) {}

    bool recording()
    {
        std::lock_guard<std::mutex> g(lock);
        return writer != nullptr;
    }

    // One record at device time `device_us`, mapped onto the host timeline. A
    // result racing stop() (the file already closed) is dropped.
    void emit(Entry& e, std::int64_t device_us, const char* kind, const Value& value)
    {
        std::lock_guard<std::mutex> g(lock);
        if (!writer) return;
        try {
            writer->write(e.id, device_us - e.offset_us, kind, value);
        } catch (const std::runtime_error&) {
            // a write after the file closed, or a failing disk: the record is lost
        }
    }

    void hook(Entry& e)
    {
        Device& d = *e.device;
        Entry* ep = &e;
        if (auto* s = dynamic_cast<Sr04*>(&d)) {
            e.hooks.push_back(s->on_measurement([this, ep](const Sr04Measurement& m) {
                emit(*ep, static_cast<std::int64_t>(m.timestamp_us), "sr04", records::sr04(m));
            }));
        } else if (auto* l4 = dynamic_cast<Vl53l4cd*>(&d)) {
            e.hooks.push_back(l4->on_measurement([this, ep](const Vl53l4cdMeasurement& m) {
                emit(*ep, static_cast<std::int64_t>(m.timestamp_us), "vl53l4", records::vl53l4(m));
            }));
        } else if (auto* l8 = dynamic_cast<Vl53l8*>(&d)) {
            e.hooks.push_back(l8->on_frame([this, ep, l8](const Vl53l8LiveFrame& f) {
                {
                    std::lock_guard<std::mutex> g(lock);
                    ep->silicon = static_cast<double>(f.frame.silicon_temp_degc);
                }
                const auto t = static_cast<std::int64_t>(f.frame.timestamp_us);
                emit(*ep, t, "vl53l8", records::vl53l8(f, options.vl53l8_layers));
                // CH: the raw CNH block riding this frame, with the armed setup's
                // decode parameters (contract 09 `vl53l8_cnh`).
                if (f.frame.cnh_raw && !f.frame.cnh_raw->empty()) {
                    if (const auto setup = l8->cnh_setup()) emit(*ep, t, "vl53l8_cnh", records::vl53l8_cnh(*f.frame.cnh_raw, *setup));
                }
            }));
        } else if (auto* lx = dynamic_cast<Vl53lx*>(&d)) {
            e.hooks.push_back(lx->on_measurement([this, ep, lx](const Vl53lxMeasurement& m) {
                const auto product = lx->product();
                const auto kind = lx->driver_kind();
                emit(*ep, static_cast<std::int64_t>(m.timestamp_us), "vl53lx",
                     records::vl53lx(m, product ? *product : "", kind ? vl53lx::to_string(*kind) : ""));
            }));
        } else if (auto* b55 = dynamic_cast<Bno055*>(&d)) {
            e.hooks.push_back(b55->on_sample([this, ep](const Bno055Sample& s) {
                emit(*ep, static_cast<std::int64_t>(s.timestamp_us), "bno055", records::bno055(s));
            }));
        } else if (auto* tmf = dynamic_cast<Tmf8829*>(&d)) {
            e.hooks.push_back(tmf->on_measurement([this, ep](const Tmf8829Measurement& m) {
                emit(*ep, m.timestamp_us, "tmf8829", records::tmf8829(m));
            }));
        } else if (auto* b86 = dynamic_cast<Bno086*>(&d)) {
            // Every enabled sensor's report at its measurement time; reports of
            // one cargo carry out-of-order device times, so t may step back
            // within a device — readers sort (contract 09).
            e.hooks.push_back(b86->on_report([this, ep](const Bno086Report& r) {
                emit(*ep, r.timestamp_us, "bno086", records::bno086(r));
            }));
        }
        // a plain Device: nothing to hook
    }

    void temperature_loop()
    {
        const auto period = std::chrono::duration<double>(1.0 / *options.temperature_hz);
        for (;;) {
            {
                std::unique_lock<std::mutex> g(tlock);
                if (tcond.wait_for(g, period, [&] { return stop_flag; })) return;
            }
            for (auto& e : entries) {
                std::optional<double> silicon;
                {
                    std::lock_guard<std::mutex> g(lock);
                    if (!writer) return;
                    silicon = e->silicon;
                    if (silicon) {
                        try {
                            writer->write(e->id, static_cast<std::int64_t>(host_now_us()), "temperature",
                                          records::temperature(*silicon, "sensor"));
                        } catch (const std::runtime_error&) {
                        }
                    }
                }
                double celsius;
                try {
                    celsius = e->device->read_mcu_temperature();
                } catch (const std::exception&) {
                    continue;
                }
                std::lock_guard<std::mutex> g(lock);
                if (!writer) return;
                try {
                    writer->write(e->id, static_cast<std::int64_t>(host_now_us()), "temperature", records::temperature(celsius, "mcu"));
                } catch (const std::runtime_error&) {
                }
            }
        }
    }
};

SessionRecorder::SessionRecorder(const std::string& path, RecorderOptions options)
    : impl_(std::make_shared<Impl>(path, std::move(options))) {}

SessionRecorder::~SessionRecorder()
{
    try {
        stop();
    } catch (...) {
    }
}

std::string SessionRecorder::add(Device& device, std::optional<std::string> device_id, int sync_samples)
{
    if (impl_->recording()) throw ArgumentError(DEPZ_E_ARG, "add() every device before recording starts");
    auto e = std::make_unique<Impl::Entry>();
    e->device = &device;
    e->id = device_id ? *device_id : "d" + std::to_string(impl_->entries.size());
    const depz::TimeSync sync = device.sync_time(sync_samples > 0 ? sync_samples : 5);
    e->offset_us = sync.offset_us;
    e->meta = Value::object();
    Value ts = Value::object();
    ts.set("offset_us", sync.offset_us);
    ts.set("rtt_us", sync.rtt_us);
    e->meta.set("time_sync", std::move(ts));
    // a board that does not answer these is still recorded
    std::string software;
    try {
        const std::string serial = device.serial_number();
        software = device.software_name();
        e->meta.set("serial", serial);
        e->meta.set("software_name", software);
    } catch (const Error&) {
        software.clear();
    }
    const Identity ident = parse_software_name(software);
    e->meta.set("sensor_type", ident.sensor_type ? to_string(*ident.sensor_type) : "unknown");
    const std::string id = e->id;
    impl_->entries.push_back(std::move(e));
    return id;
}

void SessionRecorder::start()
{
    if (impl_->recording()) return;
    Value::Object devices;
    for (const auto& e : impl_->entries) devices.emplace_back(e->id, e->meta);
    auto writer = std::make_unique<Writer>(impl_->path, devices, impl_->options.note);
    {
        std::lock_guard<std::mutex> g(impl_->lock);
        impl_->writer = std::move(writer);
    }
    for (auto& e : impl_->entries) impl_->hook(*e);
    if (impl_->options.temperature_hz && *impl_->options.temperature_hz > 0) {
        impl_->stop_flag = false;
        std::shared_ptr<Impl> self = impl_;
        impl_->thread = std::thread([self] { self->temperature_loop(); });
    }
}

void SessionRecorder::stop()
{
    if (impl_->thread.joinable()) {
        {
            std::lock_guard<std::mutex> g(impl_->tlock);
            impl_->stop_flag = true;
        }
        impl_->tcond.notify_all();
        impl_->thread.join();
    }
    for (auto& e : impl_->entries) {
        for (auto& unsub : e->hooks) unsub();
        e->hooks.clear();
    }
    std::unique_ptr<Writer> writer;
    {
        std::lock_guard<std::mutex> g(impl_->lock);
        writer = std::move(impl_->writer);
    }
    if (writer) writer->close();
}

bool SessionRecorder::recording() const { return impl_->recording(); }

std::uint64_t SessionRecorder::records_written() const
{
    std::lock_guard<std::mutex> g(impl_->lock);
    return impl_->writer ? impl_->writer->records_written() : 0;
}

}  // namespace dataset
}  // namespace depz
