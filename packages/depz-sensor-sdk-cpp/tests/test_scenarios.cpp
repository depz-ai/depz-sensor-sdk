// test_scenarios.cpp — contract 15 device scenarios: the C++ runner.
//
// Each contracts/scenarios/<id>.json names a committed real-hardware capture
// and the API calls that produced it. The runner opens the capture as its
// `probe` field says — "canonical": depz::open_on_link() (the contract 02
// §4.4 probe, then the class in place); "legacy": the requests the capture
// holds, resolve_sensor_class() and attach_sensor_class() — over a causal
// strict-tx replay, so every byte the SDK writes must equal the recorded tx
// stream, executes the steps (§2) through the C++ binding (depz/device.hpp),
// canonicalises each result (§3) and matches it against `expect`. Python
// (the reference), TS and C run the same files.
//
// Usage: depz_test_scenarios [id] (no id = every scenario).
// $DEPZ_SCENARIOS_DIR overrides the scenario directory.
#include "depz/device.hpp"
#include "depz/usb_ids.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "json.hpp"

#ifndef DEPZ_SCENARIOS_DIR
#define DEPZ_SCENARIOS_DIR "."
#endif
#ifndef DEPZ_RECORDINGS_DIR
#define DEPZ_RECORDINGS_DIR "."
#endif

#ifdef DEPZ_TEST_VECTOR_SEEN
extern "C" void depz_test_vector_seen(const char* id);  // tests/vector_seen.c
#endif

using namespace std::chrono_literals;
using testjson::Array;
using testjson::Object;
using testjson::Type;
using testjson::Value;

namespace {

constexpr auto TIMEOUT = 2000ms;       // request timeout, as the replay tests always used
constexpr auto TAKE_TIMEOUT = 5000ms;  // per streamed item; replay serves them instantly

struct StepFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};
[[noreturn]] void fail(const std::string& m) { throw StepFailure(m); }

void seen(const std::string& id)
{
#ifdef DEPZ_TEST_VECTOR_SEEN
    depz_test_vector_seen(id.c_str());
#else
    (void)id;
#endif
}

// ── canonical JSON values (§3) ──────────────────────────────────────────────

Value vnull() { return Value{}; }
Value vbool(bool b)
{
    Value v;
    v.type = Type::Bool;
    v.b = b;
    return v;
}
Value vint(std::int64_t i)
{
    Value v;
    v.type = Type::Int;
    v.i = i;
    return v;
}
Value vdbl(double d)
{
    Value v;
    v.type = Type::Double;
    v.d = d;
    return v;
}
Value vstr(const std::string& s)
{
    Value v;
    v.type = Type::String;
    v.s = s;
    return v;
}
Value varr(Array a)
{
    Value v;
    v.type = Type::Array;
    v.arr = std::move(a);
    return v;
}
Value vobj(std::initializer_list<std::pair<const std::string, Value>> kv)
{
    Value v;
    v.type = Type::Object;
    v.obj = Object(kv);
    return v;
}
template <class C>
Value ints(const C& c)
{
    Array a;
    for (const auto& x : c) a.push_back(vint(static_cast<std::int64_t>(x)));
    return varr(std::move(a));
}
template <class C>
Value dbls(const C& c)
{
    Array a;
    for (const auto& x : c) a.push_back(vdbl(static_cast<double>(x)));
    return varr(std::move(a));
}
template <class T, class F>
Value opt(const std::optional<T>& o, F f)
{
    return o ? f(*o) : vnull();
}
Value hexv(const depz::bytes& b)
{
    static const char* d = "0123456789abcdef";
    std::string out;
    for (std::byte x : b) {
        const auto v = std::to_integer<unsigned>(x);
        out.push_back(d[v >> 4]);
        out.push_back(d[v & 0xF]);
    }
    return vstr(out);
}

std::string dump(const Value& v)
{
    std::ostringstream o;
    switch (v.type) {
    case Type::Null: o << "null"; break;
    case Type::Bool: o << (v.b ? "true" : "false"); break;
    case Type::Int: o << v.i; break;
    case Type::Double: o.precision(17); o << v.d; break;
    case Type::String: o << '"' << v.s << '"'; break;
    case Type::Array: {
        o << '[';
        for (std::size_t i = 0; i < v.arr.size(); i++) o << (i ? ", " : "") << dump(v.arr[i]);
        o << ']';
        break;
    }
    case Type::Object: {
        o << '{';
        bool first = true;
        for (const auto& kv : v.obj) {
            o << (first ? "" : ", ") << '"' << kv.first << "\": " << dump(kv.second);
            first = false;
        }
        o << '}';
        break;
    }
    }
    std::string s = o.str();
    return s.size() > 300 ? s.substr(0, 300) + "..." : s;
}

// §3 matching; nullopt when `got` matches `want`, else where and how not.
std::optional<std::string> mismatch(const Value& got, const Value& want, const std::string& path = "")
{
    const std::string at = path.empty() ? "result" : path;
    if (want.type == Type::Object) {
        if (got.type != Type::Object) return at + ": expected an object, got " + dump(got);
        for (const auto& kv : want.obj) {
            if (!got.has(kv.first)) {
                std::string fields;
                for (const auto& g : got.obj) fields += (fields.empty() ? "" : ", ") + g.first;
                return at + "." + kv.first + ": missing (fields: " + fields + ")";
            }
            if (auto m = mismatch(got.at(kv.first), kv.second, path + "." + kv.first)) return m;
        }
        return std::nullopt;
    }
    if (want.type == Type::Array) {
        if (got.type != Type::Array) return at + ": expected an array, got " + dump(got);
        if (got.arr.size() != want.arr.size())
            return at + ": expected " + std::to_string(want.arr.size()) + " elements, got " +
                   std::to_string(got.arr.size());
        for (std::size_t i = 0; i < want.arr.size(); i++)
            if (auto m = mismatch(got.arr[i], want.arr[i], path + "[" + std::to_string(i) + "]")) return m;
        return std::nullopt;
    }
    if (want.type == Type::Double) {
        if (got.type == Type::Int || got.type == Type::Double) {
            const double g = got.as_double();
            if (std::fabs(g - want.d) <= std::max(1e-9 * std::fabs(want.d), 1e-12)) return std::nullopt;
        }
        return at + ": expected " + dump(want) + ", got " + dump(got);
    }
    // integers, strings, booleans, null: exact, a bool never equal to an int
    bool same = got.type == want.type;
    if (same) {
        switch (want.type) {
        case Type::Bool: same = got.b == want.b; break;
        case Type::Int: same = got.i == want.i; break;
        case Type::String: same = got.s == want.s; break;
        default: break;
        }
    }
    if (!same) return at + ": expected " + dump(want) + ", got " + dump(got);
    return std::nullopt;
}

// ── C++ results as canonical JSON, keyed by the Python field names ──────────

Value canon(const depz::Sr04Measurement& m)
{
    return vobj({{"timestamp_us", vint(static_cast<std::int64_t>(m.timestamp_us))},
                 {"echo_time_us", vint(m.echo_time_us)},
                 {"source", vstr(m.from_loop ? "loop" : "once")}});
}

Value canon(const depz::vl53l4::Vl53l4Info& i)
{
    return vobj({{"int_edges", vint(i.int_edges)}, {"slots_skipped", vint(i.slots_skipped)},
                 {"i2c_errors", vint(i.i2c_errors)}, {"last_i2c_error", vint(i.last_i2c_error)},
                 {"model_id", vint(i.model_id)}, {"fw_status", vint(i.fw_status)},
                 {"initialized", vint(i.initialized)}, {"xshut_level", vint(i.xshut_level)},
                 {"int_level", vint(i.int_level)}, {"i2c_khz", vint(i.i2c_khz)}});
}

// Python's Vl53l4cd measurement is flat: the timestamp beside the result fields.
Value canon(const depz::Vl53l4cdMeasurement& m)
{
    const auto& r = m.r;
    return vobj({{"timestamp_us", vint(static_cast<std::int64_t>(m.timestamp_us))},
                 {"range_status", vint(r.range_status)}, {"distance_mm", vint(r.distance_mm)},
                 {"sigma_mm", vint(r.sigma_mm)}, {"signal_rate_kcps", vint(r.signal_rate_kcps)},
                 {"ambient_rate_kcps", vint(r.ambient_rate_kcps)},
                 {"signal_per_spad_kcps", vint(r.signal_per_spad_kcps)},
                 {"ambient_per_spad_kcps", vint(r.ambient_per_spad_kcps)},
                 {"number_of_spad", vint(r.number_of_spad)}, {"stream_count", vint(r.stream_count)}});
}

Value canon(const depz::vl53l4::RangeTiming& t)  // Python: a (budget, inter) tuple
{
    return varr({vint(t.timing_budget_ms), vint(t.inter_measurement_ms)});
}

Value canon(const depz::Vl53l8Motion& m)
{
    return vobj({{"global_indicator_1", vint(m.global_indicator_1)},
                 {"global_indicator_2", vint(m.global_indicator_2)}, {"status", vint(m.status)},
                 {"nb_of_detected_aggregates", vint(m.nb_of_detected_aggregates)},
                 {"nb_of_aggregates", vint(m.nb_of_aggregates)}, {"motion", ints(m.motion)}});
}

// Real units as Python's Vl53l8Frame (contract 04 §5): the scaled fields are
// floats.
Value canon(const depz::Vl53l8LiveFrame& lf)
{
    const auto& f = lf.frame;
    return vobj({{"timestamp_us", vint(static_cast<std::int64_t>(f.timestamp_us))},
                 {"resolution", vint(f.resolution)}, {"silicon_temp_degc", vint(f.silicon_temp_degc)},
                 {"distance_mm", ints(f.distance_mm)}, {"target_status", ints(f.target_status)},
                 {"nb_target_detected", ints(f.nb_target_detected)},
                 {"signal_per_spad", dbls(f.signal_per_spad)}, {"ambient_per_spad", dbls(f.ambient_per_spad)},
                 {"nb_spads_enabled", ints(f.nb_spads_enabled)},
                 {"range_sigma_mm", dbls(f.range_sigma_mm)}, {"reflectance", dbls(f.reflectance)},
                 {"cnh_raw", opt(f.cnh_raw, hexv)},
                 {"motion", opt(lf.motion, [](const depz::Vl53l8Motion& m) { return canon(m); })}});
}

Value canon(const depz::bno055::Bno055Info& i)
{
    return vobj({{"i2c_addr", vint(i.i2c_addr)}, {"chip_id", vint(i.chip_id)}, {"acc_id", vint(i.acc_id)},
                 {"mag_id", vint(i.mag_id)}, {"gyr_id", vint(i.gyr_id)}, {"sw_rev", vint(i.sw_rev)},
                 {"bl_rev", vint(i.bl_rev)}, {"initialized", vint(i.initialized)},
                 {"int_level", vint(i.int_level)}, {"int_edges", vint(i.int_edges)},
                 {"read_min_us", vint(i.read_min_us)}, {"read_max_us", vint(i.read_max_us)},
                 {"read_avg_us", vint(i.read_avg_us)}, {"tx_dropped", vint(i.tx_dropped)},
                 {"i2c_errors", vint(i.i2c_errors)}, {"slots_skipped", vint(i.slots_skipped)},
                 {"bus_recoveries", vint(i.bus_recoveries)}, {"last_i2c_error", vint(i.last_i2c_error)},
                 {"sensor_resets", vint(i.sensor_resets)}, {"loop_max_us", vint(i.loop_max_us)}});
}

Value canon(const depz::bno055::Units& u)
{
    return vobj({{"accel_mg", vbool(u.accel_mg)}, {"gyro_rps", vbool(u.gyro_rps)},
                 {"euler_rad", vbool(u.euler_rad)}, {"temp_f", vbool(u.temp_f)},
                 {"android", vbool(u.android)}});
}

Value canon(const depz::bno055::CalibStatus& c)
{
    return vobj({{"system", vint(c.system)}, {"gyro", vint(c.gyro)}, {"accel", vint(c.accel)},
                 {"mag", vint(c.mag)}});
}

Value canon(const depz::bno055::CalibrationProfile& p)
{
    return vobj({{"accel_offset", ints(p.accel_offset)}, {"mag_offset", ints(p.mag_offset)},
                 {"gyro_offset", ints(p.gyro_offset)}, {"accel_radius", vint(p.accel_radius)},
                 {"mag_radius", vint(p.mag_radius)}});
}

Value canon(const depz::Bno055Status& s)
{
    return vobj({{"self_test", vint(s.self_test)}, {"clk_status", vint(s.clk_status)},
                 {"status", vint(s.status)}, {"error", vint(s.error)}});
}

Value canon(const depz::Bno055Sample& s)
{
    auto v3 = [](const std::array<double, 3>& a) { return dbls(a); };
    return vobj({{"timestamp_us", vint(static_cast<std::int64_t>(s.timestamp_us))},
                 {"addr", vint(s.addr)}, {"raw", hexv(s.raw)}, {"units", canon(s.units)},
                 {"accel", opt(s.accel, v3)}, {"mag", opt(s.mag, v3)}, {"gyro", opt(s.gyro, v3)},
                 {"euler", opt(s.euler, v3)},
                 {"quaternion", opt(s.quaternion, [](const std::array<double, 4>& a) { return dbls(a); })},
                 {"linear_accel", opt(s.linear_accel, v3)}, {"gravity", opt(s.gravity, v3)},
                 {"temperature", opt(s.temperature, vdbl)},
                 {"calibration", opt(s.calibration, [](const depz::bno055::CalibStatus& c) { return canon(c); })}});
}

Value canon(const depz::bno086::ProductId& p)
{
    return vobj({{"reset_cause", vint(p.reset_cause)}, {"sw_version_major", vint(p.sw_version_major)},
                 {"sw_version_minor", vint(p.sw_version_minor)}, {"sw_part_number", vint(p.sw_part_number)},
                 {"sw_build_number", vint(p.sw_build_number)}, {"sw_version_patch", vint(p.sw_version_patch)}});
}

Value canon(const depz::bno086::Feature& f)
{
    return vobj({{"sensor_id", vint(f.sensor_id)}, {"flags", vint(f.flags)},
                 {"sensitivity", vint(f.sensitivity)}, {"interval_us", vint(f.interval_us)},
                 {"batch_us", vint(f.batch_us)}, {"cfg_word", vint(f.cfg_word)}});
}

Value canon(const depz::bno086::CalibrationConfig& c)
{
    return vobj({{"accel", vbool(c.accel)}, {"gyro", vbool(c.gyro)}, {"mag", vbool(c.mag)},
                 {"planar", vbool(c.planar)}});
}

Value canon(const depz::bno086::Counts& c)
{
    return vobj({{"sensor_id", vint(c.sensor_id)}, {"offered", vint(c.offered)}, {"accepted", vint(c.accepted)},
                 {"on", vint(c.on)}, {"attempted", vint(c.attempted)}});
}

Value canon(const depz::bno086::ErrorRecord& e)
{
    return vobj({{"severity", vint(e.severity)}, {"seq", vint(e.seq)}, {"source", vint(e.source)},
                 {"error", vint(e.error)}, {"module", vint(e.module)}, {"code", vint(e.code)}});
}

// The parsed fields must be the ones Python's SensorMetadata.from_words gives
// for raw_words (the authoritative part a scenario pins).
Value canon(const depz::bno086::SensorMetadata& m)
{
    std::vector<std::uint32_t> w = m.raw_words;
    w.resize(std::max<std::size_t>(w.size(), 10), 0);
    const unsigned rev = (w[3] >> 16) & 0xFFFF;
    const bool same = m.me_version == (w[0] & 0xFF) && m.mh_version == ((w[0] >> 8) & 0xFF) &&
                      m.sh_version == ((w[0] >> 16) & 0xFF) && m.range_raw == w[1] && m.resolution_raw == w[2] &&
                      m.revision == rev && m.power_ma_q10 == (w[3] & 0xFFFF) && m.min_period_us == w[4] &&
                      m.fifo_max == (w[5] & 0xFFFF) && m.fifo_reserved == ((w[5] >> 16) & 0xFFFF) &&
                      m.batch_buffer_bytes == (w[6] & 0xFFFF) && m.q_point_1 == (w[7] & 0xFFFF) &&
                      m.q_point_2 == ((w[7] >> 16) & 0xFFFF) &&
                      m.q_point_3 == (rev >= 3 ? (w[8] >> 16) & 0xFFFF : 0u) &&
                      m.max_period_us == (rev >= 4 ? w[9] : 0u);
    if (!same) fail("SensorMetadata fields disagree with its raw_words (SensorMetadata.from_words)");
    return vobj({{"me_version", vint(m.me_version)}, {"mh_version", vint(m.mh_version)},
                 {"sh_version", vint(m.sh_version)}, {"range_raw", vint(m.range_raw)},
                 {"resolution_raw", vint(m.resolution_raw)}, {"revision", vint(m.revision)},
                 {"power_ma_q10", vint(m.power_ma_q10)}, {"min_period_us", vint(m.min_period_us)},
                 {"max_period_us", vint(m.max_period_us)}, {"fifo_max", vint(m.fifo_max)},
                 {"fifo_reserved", vint(m.fifo_reserved)}, {"batch_buffer_bytes", vint(m.batch_buffer_bytes)},
                 {"q_point_1", vint(m.q_point_1)}, {"q_point_2", vint(m.q_point_2)},
                 {"q_point_3", vint(m.q_point_3)}, {"raw_words", ints(m.raw_words)}});
}

// Python has one record class per report type; C++ one struct for all. Every
// field is emitted (a scenario compares only what it pins), accuracy_raw only
// where the report has one.
Value canon(const depz::Bno086Report& r)
{
    Value v = vobj({{"sensor_id", vint(r.sensor_id)}, {"timestamp_us", vint(r.timestamp_us)},
                    {"seq", vint(r.seq)}, {"accuracy", vint(r.accuracy)}, {"delay_us", vint(r.delay_us)},
                    {"x_raw", vint(r.x_raw)}, {"y_raw", vint(r.y_raw)}, {"z_raw", vint(r.z_raw)},
                    {"bias_x_raw", vint(r.bias_x_raw)}, {"bias_y_raw", vint(r.bias_y_raw)},
                    {"bias_z_raw", vint(r.bias_z_raw)}, {"i_raw", vint(r.i_raw)}, {"j_raw", vint(r.j_raw)},
                    {"k_raw", vint(r.k_raw)}, {"real_raw", vint(r.real_raw)}, {"value_raw", vint(r.value_raw)},
                    {"flags", vint(r.flags)}, {"latency_us", vint(r.latency_us)}, {"steps", vint(r.steps)},
                    {"motion", vint(r.motion)}, {"classification", vint(r.classification)},
                    {"page_number", vint(r.page_number)}, {"end_of_sequence", vbool(r.end_of_sequence)},
                    {"most_likely_state", vint(r.most_likely_state)}, {"confidences", ints(r.confidences)},
                    {"intent", vint(r.intent)}, {"request", vint(r.request)},
                    {"flushed_sensor", vint(r.flushed_sensor)},
                    {"sensor_timestamp_us", vint(r.sensor_timestamp_us)},
                    {"temperature_raw", vint(r.temperature_raw)}, {"vx_raw", vint(r.vx_raw)},
                    {"vy_raw", vint(r.vy_raw)}, {"vz_raw", vint(r.vz_raw)}, {"data", hexv(r.data)}});
    if (r.has_accuracy_raw) v.obj["accuracy_raw"] = vint(r.accuracy_raw);
    return v;
}

Value canon(const depz::Vl53lxTarget& t)  // Python: the Target namedtuple
{
    return vobj({{"distance_mm", vint(t.distance_mm)}, {"status", vint(t.status)},
                 {"status_text", vstr(t.status_text)}, {"signal_kcps", vdbl(t.signal_kcps)},
                 {"ambient_kcps", vdbl(t.ambient_kcps)}, {"sigma_mm", vdbl(t.sigma_mm)},
                 {"min_range_mm", vint(t.min_range_mm)}, {"max_range_mm", vint(t.max_range_mm)}});
}

// Python's bins are the ST HistogramBinData: register-named fields.
Value canon(const depz::Vl53lxBins& b)
{
    return vobj({{"bin_data", ints(b.bin_data)}, {"vcsel_period", vint(b.vcsel_period)},
                 {"result__stream_count", vint(b.stream_count)},
                 {"result__dss_actual_effective_spads", vint(b.dss_actual_effective_spads)},
                 {"ambient_per_bin", vint(b.ambient_per_bin)},
                 {"zero_distance_phase", vint(b.zero_distance_phase)}});
}

Value canon(const depz::Vl53lxMeasurement& m)
{
    Array targets;
    for (const auto& t : m.targets) targets.push_back(canon(t));
    return vobj({{"timestamp_us", vint(static_cast<std::int64_t>(m.timestamp_us))},
                 {"distance_mm", vint(m.distance_mm)}, {"status", vint(m.status)},
                 {"status_text", vstr(m.status_text)}, {"signal_kcps", vdbl(m.signal_kcps)},
                 {"ambient_kcps", vdbl(m.ambient_kcps)}, {"sigma_mm", vdbl(m.sigma_mm)},
                 {"spads", vdbl(m.spads)}, {"targets", varr(std::move(targets))},
                 {"bins", opt(m.bins, [](const depz::Vl53lxBins& b) { return canon(b); })}});
}

template <class A, class B>
Value canon_pair(const std::pair<A, B>& p)  // Python: a 2-tuple
{
    return varr({vint(p.first), vint(p.second)});
}

Value canon_identity(const depz::Identity& id)
{
    return vobj({{"mode", vstr(depz::to_string(id.mode))},
                 {"sensor_type", id.sensor_type ? vstr(depz::to_string(*id.sensor_type)) : vnull()},
                 {"software_name", vstr(id.software_name)}, {"version", vstr(id.version)}});
}

// ── arguments ───────────────────────────────────────────────────────────────

void nargs(const Array& a, std::size_t lo, std::size_t hi)
{
    if (a.size() < lo || a.size() > hi)
        fail("takes " + std::to_string(lo) + ".." + std::to_string(hi) + " arguments, got " +
             std::to_string(a.size()));
}
std::int64_t ai(const Array& a, std::size_t i)
{
    if (i >= a.size() || a[i].type != Type::Int) fail("argument " + std::to_string(i) + ": expected an integer");
    return a[i].i;
}
double ad(const Array& a, std::size_t i)
{
    if (i >= a.size() || (a[i].type != Type::Int && a[i].type != Type::Double))
        fail("argument " + std::to_string(i) + ": expected a number");
    return a[i].as_double();
}
std::optional<std::string> aos(const Array& a, std::size_t i)  // missing / null = None
{
    if (i >= a.size() || a[i].type == Type::Null) return std::nullopt;
    if (a[i].type != Type::String) fail("argument " + std::to_string(i) + ": expected a string or null");
    return a[i].s;
}

// Record arguments (§2): (C++ class, call, position) -> what the builder makes.
// run_step refuses an object argument without an entry here.
const std::set<std::tuple<std::string, std::string, std::size_t>> RECORD_ARGS = {
    {"Bno055", "configure", 1},     // bno055::Units
    {"Vl53l8", "configure_cnh", 0},  // CnhSetup: init_config + create_agg_map
};

void record_keys(const Value& rec, std::initializer_list<const char*> keys)
{
    if (rec.type != Type::Object) fail("record argument: expected an object");
    for (const auto& kv : rec.obj)
        if (std::none_of(keys.begin(), keys.end(), [&](const char* k) { return kv.first == k; }))
            fail("record argument: unknown field '" + kv.first + "'");
    for (const char* k : keys)
        if (!rec.has(k)) fail(std::string("record argument: missing field '") + k + "'");
}

depz::bno055::Units units_record(const Value& rec)
{
    record_keys(rec, {"accel_mg", "gyro_rps", "euler_rad", "temp_f", "android"});
    depz::bno055::Units u;
    u.accel_mg = rec.at("accel_mg").as_bool();
    u.gyro_rps = rec.at("gyro_rps").as_bool();
    u.euler_rad = rec.at("euler_rad").as_bool();
    u.temp_f = rec.at("temp_f").as_bool();
    u.android = rec.at("android").as_bool();
    return u;
}

depz::CnhSetup cnh_record(const Value& rec)
{
    record_keys(rec, {"start_bin", "num_bins", "sub_sample", "resolution", "agg_map"});
    const auto& m = rec.at("agg_map").as_array();
    if (m.size() != 6) fail("record argument: agg_map needs 6 integers");
    auto i = [](const Value& v) { return static_cast<int>(v.as_int()); };
    depz::CnhSetup s;
    s.init_config(i(rec.at("start_bin")), i(rec.at("num_bins")), i(rec.at("sub_sample")));
    s.create_agg_map(i(rec.at("resolution")), i(m[0]), i(m[1]), i(m[2]), i(m[3]), i(m[4]), i(m[5]));
    return s;
}

std::optional<depz::vl53lx::DriverKind> driver_kind(const std::optional<std::string>& s)
{
    if (!s) return std::nullopt;
    if (*s == "uld") return depz::vl53lx::DriverKind::Uld;
    if (*s == "ulp") return depz::vl53lx::DriverKind::Ulp;
    if (*s == "histogram") return depz::vl53lx::DriverKind::Histogram;
    fail("unknown driver kind '" + *s + "'");
}

// ── the alias table (§2) ────────────────────────────────────────────────────
// C++ has no reflection, so every canonical name a scenario may use is bound
// here by hand: canonical (Python) name -> the C++ spelling (getters drop
// `get_`) and an adapter for its arguments and result. A name missing from a
// class's table fails the scenario; it is never skipped.

using Fn = std::function<Value(const Array&)>;
struct Method {
    std::string cpp;  // the C++ spelling, for failure messages
    Fn fn;
};

struct Sub {
    std::function<std::optional<Value>(std::chrono::milliseconds)> next;
    std::function<std::uint64_t()> dropped;
};
struct StreamBinding {
    std::string cpp;
    std::function<Sub(std::size_t maxsize)> open;
};

struct Binding {
    std::string family;  // the C++ class
    std::map<std::string, Method> calls;
    std::map<std::string, StreamBinding> streams;
    // Checks carried over from the hand-written replay tests that a scenario
    // has no step for; run after the last step (scenario id passed in).
    std::vector<std::function<void(const std::string&)>> post;
};

template <class T>
StreamBinding stream_of(const std::string& cpp, std::function<depz::Stream<T>(std::size_t)> make,
                        std::function<void(const T&)> keep = {})
{
    return {cpp, [make, keep](std::size_t maxsize) {
                auto s = std::make_shared<depz::Stream<T>>(make(maxsize));
                Sub sub;
                sub.next = [s, keep](std::chrono::milliseconds t) -> std::optional<Value> {
                    auto item = s->next(t);
                    if (!item) return std::nullopt;
                    if (keep) keep(*item);
                    return canon(*item);
                };
                sub.dropped = [s] { return s->dropped_count(); };
                return sub;
            }};
}

Value none() { return vnull(); }

void bind_device(Binding& b, depz::Device* d)
{
    b.calls["get_software_name"] = {"software_name()", [d](const Array& a) { nargs(a, 0, 0); return vstr(d->software_name()); }};
    b.calls["get_device_name"] = {"device_name()", [d](const Array& a) { nargs(a, 0, 0); return vstr(d->device_name()); }};
    b.calls["get_serial_number"] = {"serial_number()", [d](const Array& a) { nargs(a, 0, 0); return vstr(d->serial_number()); }};
}

void bind_sr04(Binding& b, depz::Sr04* d)
{
    b.family = "Sr04";
    b.calls["get_sample_period_us"] = {"sample_period_us()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->sample_period_us()); }};
    b.calls["set_sample_period_us"] = {"set_sample_period_us()", [d](const Array& a) {
        nargs(a, 1, 1); d->set_sample_period_us(static_cast<std::uint32_t>(ai(a, 0))); return none(); }};
    b.calls["get_echo_decay_us"] = {"echo_decay_us()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->echo_decay_us()); }};
    b.calls["set_echo_decay_us"] = {"set_echo_decay_us()", [d](const Array& a) {
        nargs(a, 1, 1); return vint(d->set_echo_decay_us(static_cast<std::uint32_t>(ai(a, 0)))); }};
    b.calls["measure_once"] = {"measure_once()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->measure_once()); }};
    b.calls["start"] = {"start()", [d](const Array& a) { nargs(a, 0, 0); d->start(); return none(); }};
    b.calls["stop"] = {"stop()", [d](const Array& a) { nargs(a, 0, 0); d->stop(); return none(); }};
    // Python SR04 `stream()`, TS `measurements()`: contract 07 says measurements.
    b.streams["measurements"] = stream_of<depz::Sr04Measurement>("stream()", [d](std::size_t n) { return d->stream(n); });
}

void bind_vl53l4cd(Binding& b, depz::Vl53l4cd* d)
{
    b.family = "Vl53l4cd";
    b.calls["bridge_info"] = {"bridge_info()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->bridge_info()); }};
    b.calls["is_alive"] = {"is_alive()", [d](const Array& a) { nargs(a, 0, 0); return vbool(d->is_alive()); }};
    b.calls["init"] = {"init()", [d](const Array& a) {
        nargs(a, 0, 1);
        if (a.empty()) d->init(); else d->init(static_cast<std::uint16_t>(ai(a, 0)));
        return none(); }};
    b.calls["set_range_timing"] = {"set_range_timing()", [d](const Array& a) {
        nargs(a, 1, 2);
        d->set_range_timing(static_cast<std::uint32_t>(ai(a, 0)), a.size() > 1 ? static_cast<std::uint32_t>(ai(a, 1)) : 0);
        return none(); }};
    b.calls["get_range_timing"] = {"range_timing()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->range_timing()); }};
    b.calls["get_offset_mm"] = {"offset_mm()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->offset_mm()); }};
    b.calls["set_offset_mm"] = {"set_offset_mm()", [d](const Array& a) {
        nargs(a, 1, 1); d->set_offset_mm(static_cast<std::int32_t>(ai(a, 0))); return none(); }};
    b.calls["get_xtalk_kcps"] = {"xtalk_kcps()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->xtalk_kcps()); }};
    b.calls["set_xtalk_kcps"] = {"set_xtalk_kcps()", [d](const Array& a) {
        nargs(a, 1, 1); d->set_xtalk_kcps(static_cast<std::uint16_t>(ai(a, 0))); return none(); }};
    b.calls["measure_once"] = {"measure_once()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->measure_once()); }};
    b.calls["start_ranging"] = {"start_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->start_ranging(); return none(); }};
    b.calls["stop_ranging"] = {"stop_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->stop_ranging(); return none(); }};
    b.calls["stream_parse_errors"] = {"stream_parse_errors()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(static_cast<std::int64_t>(d->stream_parse_errors())); }};
    b.streams["measurements"] = stream_of<depz::Vl53l4cdMeasurement>("measurements()", [d](std::size_t n) { return d->measurements(n); });
}

void bind_vl53l8(Binding& b, depz::Vl53l8* d)
{
    b.family = "Vl53l8";
    auto phases = std::make_shared<int>(-1);  // -1: init() not called
    b.calls["init"] = {"init()", [d, phases](const Array& a) {
        nargs(a, 0, 0);
        *phases = 0;
        d->init([phases](const std::string&, std::size_t, std::size_t) { ++*phases; });
        return none(); }};
    b.calls["is_alive"] = {"is_alive()", [d](const Array& a) { nargs(a, 0, 0); return vbool(d->is_alive()); }};
    b.calls["get_resolution"] = {"resolution()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->resolution()); }};
    b.calls["set_resolution"] = {"set_resolution()", [d](const Array& a) {
        nargs(a, 1, 1); d->set_resolution(static_cast<int>(ai(a, 0))); return none(); }};
    b.calls["get_ranging_frequency_hz"] = {"ranging_frequency_hz()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(d->ranging_frequency_hz()); }};
    b.calls["set_ranging_frequency_hz"] = {"set_ranging_frequency_hz()", [d](const Array& a) {
        nargs(a, 1, 1); d->set_ranging_frequency_hz(static_cast<std::uint8_t>(ai(a, 0))); return none(); }};
    b.calls["configure_cnh"] = {"configure_cnh()", [d](const Array& a) {
        nargs(a, 1, 1); d->configure_cnh(cnh_record(a[0])); return none(); }};
    b.calls["start_ranging"] = {"start_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->start_ranging(); return none(); }};
    b.calls["stop_ranging"] = {"stop_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->stop_ranging(); return none(); }};
    b.calls["frame_parse_errors"] = {"frame_parse_errors()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(static_cast<std::int64_t>(d->frame_parse_errors())); }};
    b.calls["module_type"] = {"module_type()", [d](const Array& a) {
        nargs(a, 0, 0); return opt(d->module_type(), [](int m) { return vint(m); }); }};
    b.streams["frames"] = stream_of<depz::Vl53l8LiveFrame>("frames()", [d](std::size_t n) { return d->frames(n); });
    // Carried over from l8_replay / l8_init_transcript: init reports its phases.
    b.post.push_back([phases](const std::string&) {
        if (*phases >= 0 && *phases <= 5)
            fail("init() reported " + std::to_string(*phases) + " progress phases, expected more than 5");
    });
}

void bind_bno055(Binding& b, depz::Bno055* d)
{
    b.family = "Bno055";
    b.calls["bridge_info"] = {"bridge_info()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->bridge_info()); }};
    b.calls["is_alive"] = {"is_alive()", [d](const Array& a) { nargs(a, 0, 0); return vbool(d->is_alive()); }};
    b.calls["reset_sensor"] = {"reset_sensor()", [d](const Array& a) { nargs(a, 0, 0); d->reset_sensor(); return none(); }};
    b.calls["configure"] = {"configure()", [d](const Array& a) {
        nargs(a, 0, 2);
        const auto mode = a.empty() ? depz::bno055::OprMode::Ndof : static_cast<depz::bno055::OprMode>(ai(a, 0));
        d->configure(mode, a.size() > 1 ? units_record(a[1]) : depz::bno055::Units{});
        return none(); }};
    b.calls["read_calibration_profile"] = {"calibration_profile()", [d](const Array& a) {
        nargs(a, 0, 0); return canon(d->calibration_profile()); }};
    b.calls["calibration_status"] = {"calibration_status()", [d](const Array& a) {
        nargs(a, 0, 0); return canon(d->calibration_status()); }};
    b.calls["system_status"] = {"system_status()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->system_status()); }};
    // Python start_stream(period_ms, block=(addr, len)); C++ takes addr, len flat.
    b.calls["start_stream"] = {"start_stream()", [d](const Array& a) {
        nargs(a, 0, 2);
        const auto period = a.empty() ? 10 : static_cast<std::uint16_t>(ai(a, 0));
        if (a.size() > 1) {
            if (a[1].type != Type::Array || a[1].arr.size() != 2) fail("argument 1: expected [addr, len]");
            d->start_stream(period, static_cast<std::uint8_t>(ai(a[1].arr, 0)), static_cast<std::uint8_t>(ai(a[1].arr, 1)));
        } else {
            d->start_stream(period);
        }
        return none(); }};
    b.calls["stop_stream"] = {"stop_stream()", [d](const Array& a) { nargs(a, 0, 0); d->stop_stream(); return none(); }};
    b.calls["stream_parse_errors"] = {"stream_parse_errors()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(static_cast<std::int64_t>(d->stream_parse_errors())); }};
    b.streams["samples"] = stream_of<depz::Bno055Sample>("samples()", [d](std::size_t n) { return d->samples(n); });
}

void bind_bno086(Binding& b, depz::Bno086* d)
{
    using depz::bno086::SensorId;
    b.family = "Bno086";
    auto sid = [](const Array& a, std::size_t i) { return static_cast<SensorId>(ai(a, i)); };
    b.calls["hardware_reset"] = {"hardware_reset()", [d](const Array& a) { nargs(a, 0, 0); d->hardware_reset(); return none(); }};
    b.calls["product_id"] = {"product_id()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->product_id()); }};
    b.calls["enable"] = {"enable(SensorId, double)", [d, sid](const Array& a) {
        nargs(a, 2, 2); return canon(d->enable(sid(a, 0), ad(a, 1))); }};
    b.calls["disable"] = {"disable()", [d, sid](const Array& a) { nargs(a, 1, 1); d->disable(sid(a, 0)); return none(); }};
    b.calls["get_calibration"] = {"calibration()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->calibration()); }};
    b.calls["get_oscillator_type"] = {"oscillator_type()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->oscillator_type()); }};
    b.calls["get_metadata"] = {"metadata()", [d, sid](const Array& a) { nargs(a, 1, 1); return canon(d->metadata(sid(a, 0))); }};
    b.calls["get_counts"] = {"counts()", [d, sid](const Array& a) { nargs(a, 1, 1); return canon(d->counts(sid(a, 0))); }};
    b.calls["get_errors"] = {"errors()", [d](const Array& a) {
        nargs(a, 0, 1);
        Array out;
        for (const auto& e : d->errors(a.empty() ? 0 : static_cast<std::uint8_t>(ai(a, 0)))) out.push_back(canon(e));
        return varr(std::move(out)); }};
    b.calls["shtp_discarded"] = {"shtp_discarded()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(static_cast<std::int64_t>(d->shtp_discarded())); }};
    auto taken = std::make_shared<std::vector<depz::Bno086Report>>();
    b.streams["reports"] = stream_of<depz::Bno086Report>(
        "reports()", [d](std::size_t n) { return d->reports(n); },
        [taken](const depz::Bno086Report& r) { taken->push_back(r); });
    // Carried over from bno086_session_replay: no SHTP bytes thrown away; each
    // report decoded to the type its sensor id names; on the lab session (board
    // lying still) unit quaternions with an accuracy, gravity-sized accel.
    b.post.push_back([d, taken](const std::string& id) {
        if (d->shtp_discarded() != 0) fail("shtp_discarded() = " + std::to_string(d->shtp_discarded()));
        using RT = depz::bno086::ReportType;
        const std::map<int, RT> type_of = {{0x01, RT::Acceleration}, {0x05, RT::RotationVector},
                                           {0x2A, RT::GyroIntegratedRV}};
        for (std::size_t i = 0; i < taken->size(); i++) {
            const auto& r = (*taken)[i];
            const std::string at = "report " + std::to_string(i) + " (sensor " + std::to_string(r.sensor_id) + ")";
            auto t = type_of.find(r.sensor_id);
            if (t != type_of.end() && r.type != t->second) fail(at + ": decoded to the wrong report type");
            if (id != "bno086_session") continue;
            if (r.type == RT::RotationVector) {
                const auto q = r.quaternion();
                const double norm = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
                if (!r.accuracy_rad() || std::fabs(norm - 1.0) >= 0.01) fail(at + ": not a unit quaternion with accuracy");
            } else if (r.type == RT::Acceleration) {
                const auto a = r.xyz();
                const double g = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
                if (!(g > 9.0 && g < 10.6)) fail(at + ": |accel| " + std::to_string(g) + " is not gravity");
            }
        }
    });
}

void bind_vl53lx(Binding& b, depz::Vl53lx* d)
{
    b.family = "Vl53lx";
    // Python init(driver=None, *, product=None); positional here.
    b.calls["init"] = {"init(DriverKind, product)", [d](const Array& a) {
        nargs(a, 0, 2); d->init(driver_kind(aos(a, 0)), aos(a, 1)); return none(); }};
    b.calls["product"] = {"product()", [d](const Array& a) {
        nargs(a, 0, 0); return opt(d->product(), [](const std::string& s) { return vstr(s); }); }};
    b.calls["detected_product"] = {"detected_product()", [d](const Array& a) {
        nargs(a, 0, 0); return opt(d->detected_product(), [](const std::string& s) { return vstr(s); }); }};
    b.calls["configure"] = {"configure()", [d](const Array& a) {
        nargs(a, 0, 3);
        d->configure(a.size() > 0 ? static_cast<int>(ai(a, 0)) : 50, a.size() > 1 ? static_cast<int>(ai(a, 1)) : 0, aos(a, 2));
        return none(); }};
    b.calls["get_range_timing"] = {"range_timing()", [d](const Array& a) { nargs(a, 0, 0); return canon_pair(d->range_timing()); }};
    b.calls["modes"] = {"modes()", [d](const Array& a) {
        nargs(a, 0, 0);
        Array out;
        for (const auto& m : d->modes()) out.push_back(vstr(m));
        return varr(std::move(out)); }};
    // Python reads these off the bound driver (BUDGET_MS, reach_mm()).
    b.calls["budget_range"] = {"budget_range()", [d](const Array& a) { nargs(a, 0, 0); return canon_pair(d->budget_range()); }};
    b.calls["driver_reach_mm"] = {"driver_reach_mm()", [d](const Array& a) { nargs(a, 0, 0); return vint(d->driver_reach_mm()); }};
    b.calls["measure_once"] = {"measure_once()", [d](const Array& a) { nargs(a, 0, 0); return canon(d->measure_once()); }};
    b.calls["start_ranging"] = {"start_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->start_ranging(); return none(); }};
    b.calls["stop_ranging"] = {"stop_ranging()", [d](const Array& a) { nargs(a, 0, 0); d->stop_ranging(); return none(); }};
    b.calls["stream_parse_errors"] = {"stream_parse_errors()", [d](const Array& a) {
        nargs(a, 0, 0); return vint(static_cast<std::int64_t>(d->stream_parse_errors())); }};
    b.streams["measurements"] = stream_of<depz::Vl53lxMeasurement>("measurements()", [d](std::size_t n) { return d->measurements(n); });
}

// Contract 07 §1 layer 3: the canonical class of an opened device.
std::string canonical_class(depz::Device* dev)
{
    if (dynamic_cast<depz::Sr04*>(dev)) return "sr04";
    if (dynamic_cast<depz::Vl53l4cd*>(dev)) return "vl53l4cd";
    if (dynamic_cast<depz::Bno055*>(dev)) return "bno055";
    if (dynamic_cast<depz::Bno086*>(dev)) return "bno086";
    if (dynamic_cast<depz::Tmf8829*>(dev)) return "tmf8829";
    if (auto* v = dynamic_cast<depz::Vl53l8*>(dev)) {
        switch (v->model()) {
        case depz::Vl53l8Model::L8CX: return "vl53l8cx";
        case depz::Vl53l8Model::L8CH: return "vl53l8ch";
        case depz::Vl53l8Model::L5CX: return "vl53l5cx";
        case depz::Vl53l8Model::L7CX: return "vl53l7cx";
        case depz::Vl53l8Model::L7CH: return "vl53l7ch";
        }
    }
    if (auto* v = dynamic_cast<depz::Vl53lx*>(dev)) return v->sensor_class();  // one C++ class for the family
    return "device (no class)";
}

Binding bind(depz::Device* dev)
{
    Binding b;
    bind_device(b, dev);
    if (auto* x = dynamic_cast<depz::Sr04*>(dev)) bind_sr04(b, x);
    else if (auto* x = dynamic_cast<depz::Vl53l4cd*>(dev)) bind_vl53l4cd(b, x);
    else if (auto* x = dynamic_cast<depz::Vl53l8*>(dev)) bind_vl53l8(b, x);
    else if (auto* x = dynamic_cast<depz::Bno055*>(dev)) bind_bno055(b, x);
    else if (auto* x = dynamic_cast<depz::Bno086*>(dev)) bind_bno086(b, x);
    else if (auto* x = dynamic_cast<depz::Vl53lx*>(dev)) bind_vl53lx(b, x);
    else fail("no binding for " + canonical_class(dev) + ": add it to the alias table");
    return b;
}

// The production (vid, pid) of a USB-table model: what enumeration would give.
std::pair<std::optional<std::uint16_t>, std::optional<std::uint16_t>> usb_id(const std::string& model)
{
    for (std::uint32_t p = depz::DEPZ_PID_RANGE_LO; p <= depz::DEPZ_PID_RANGE_HI; p++) {
        const auto pid = static_cast<std::uint16_t>(p);
        const auto hint = depz::usb_model_hint(depz::DEPZ_USB_VID, pid);
        if (hint && *hint == model) return {depz::DEPZ_USB_VID, pid};
    }
    fail("usb_model " + model + " is not in the USB-id table");
}

// A legacy capture's probe (contract 15 §1): exactly the requests it holds —
// GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME for the L5/L7 and 1D
// families — then the resolved class attached in place.
std::unique_ptr<depz::Device> open_legacy(depz::Link link, std::optional<std::uint16_t> vid,
                                          std::optional<std::uint16_t> pid, std::string& software_name)
{
    auto base = depz::Device::open(std::move(link));
    base->set_timeout(TIMEOUT);
    software_name = base->software_name();
    std::string device_name;
    const auto type = depz::parse_software_name(software_name).sensor_type;
    if (type == depz::SensorType::Vl53l7 || type == depz::SensorType::Vl53lx) {
        try {
            device_name = base->device_name();
        } catch (const depz::Error&) {
            device_name.clear();
        }
    }
    return depz::attach_sensor_class(std::move(base),
                                     depz::resolve_sensor_class(software_name, device_name, vid, pid));
}

// §4: the canonical kind of the exception in flight, nullopt for anything else.
std::optional<std::string> error_kind(std::string& what)
{
    try {
        throw;
    } catch (const depz::TimeoutError& e) {
        what = e.what();
        return "timeout";
    } catch (const depz::StatusError& e) {
        what = e.what();
        return e.status() == depz::Status::ErrCmdNotSupported ? "not_supported" : "device_error";
    } catch (const depz::ArgumentError& e) {
        what = e.what();
        return "invalid_argument";
    } catch (const depz::WrongTypeError& e) {  // a CH-only call on a CX, say
        what = e.what();
        return "not_supported";
    } catch (const std::exception& e) {
        what = e.what();
    } catch (...) {
        what = "unknown exception";
    }
    return std::nullopt;
}

// ── the runner ──────────────────────────────────────────────────────────────

std::string describe(const Value& step)
{
    if (step.has("call")) {
        std::string s = step.at("call").as_string() + "(";
        if (step.has("args"))
            for (std::size_t i = 0; i < step.at("args").arr.size(); i++)
                s += (i ? ", " : "") + dump(step.at("args").arr[i]);
        return s + ")";
    }
    if (step.has("subscribe")) return "subscribe " + step.at("subscribe").as_string() + " as " + step.at("as").as_string();
    if (step.has("take")) return "take " + std::to_string(step.at("count").as_int()) + " from " + step.at("take").as_string();
    if (step.has("get")) return "get " + step.at("get").as_string();
    return dump(step);
}

const Method& resolve(const Binding& b, const std::string& name)
{
    auto it = b.calls.find(name);
    if (it == b.calls.end()) fail(b.family + " has no '" + name + "': add it to the alias table");
    return it->second;
}

void run_step(const Binding& b, const Value& step, std::map<std::string, Sub>& subs,
              const std::map<std::string, std::size_t>& sizes)
{
    if (step.has("call")) {
        const std::string name = step.at("call").as_string();
        const Method& m = resolve(b, name);
        const Array args = step.has("args") ? step.at("args").as_array() : Array{};
        for (std::size_t i = 0; i < args.size(); i++)
            if (args[i].type == Type::Object && !RECORD_ARGS.count({b.family, name, i}))
                fail(name + " argument " + std::to_string(i) + " is a record with no builder in RECORD_ARGS");
        if (step.has("error")) {
            const std::string want = step.at("error").as_string();
            Value result;
            try {
                result = m.fn(args);
            } catch (const StepFailure&) {
                throw;
            } catch (...) {
                std::string what;
                const auto kind = error_kind(what);
                if (kind != want)
                    fail("expected error " + want + ", got " + (kind ? *kind : "unmapped") + " (" + what + ")");
                return;
            }
            fail("expected error " + want + ", succeeded with " + dump(result));
        }
        const Value result = m.fn(args);
        if (step.has("expect"))
            if (auto mm = mismatch(result, step.at("expect"))) fail(*mm);
    } else if (step.has("subscribe")) {
        const std::string name = step.at("subscribe").as_string();
        auto it = b.streams.find(name);
        if (it == b.streams.end()) fail(b.family + " has no stream '" + name + "': add it to the alias table");
        const std::string as = step.at("as").as_string();
        auto sz = sizes.find(as);
        subs[as] = it->second.open(sz == sizes.end() ? 4096 : sz->second);
    } else if (step.has("take")) {
        auto it = subs.find(step.at("take").as_string());
        if (it == subs.end()) fail("no subscription '" + step.at("take").as_string() + "'");
        const auto& want = step.at("expect").as_array();
        const auto count = static_cast<std::size_t>(step.at("count").as_int());
        if (want.size() != count) fail("count and expect disagree");
        for (std::size_t i = 0; i < count; i++) {
            auto item = it->second.next(TAKE_TIMEOUT);
            if (!item)
                fail("got " + std::to_string(i) + " of " + std::to_string(count) + " items, then nothing for " +
                     std::to_string(TAKE_TIMEOUT.count() / 1000) + " s");
            if (auto mm = mismatch(*item, want[i], "item[" + std::to_string(i) + "]")) fail(*mm);
        }
    } else if (step.has("get")) {
        const std::string name = step.at("get").as_string();
        const Value v = resolve(b, name).fn(Array{});
        if (auto mm = mismatch(v, step.at("expect"), name)) fail(*mm);
    } else {
        fail("unknown step " + dump(step));
    }
}

// A borrowed view of the device's link, released before it could close it.
struct LinkView {
    depz::Link view;
    explicit LinkView(depz_link* raw) : view(depz::Link::adopt(raw)) {}
    ~LinkView() { view.release(); }
};

// `end: exhausted`. The C SDK's replay link reports the rx side only; a
// causal replay releases an rx chunk only after the tx before it was written,
// so on a capture whose last record is rx an exhausted rx side means every tx
// byte was written too. A capture ending on tx cannot be checked: fail loudly.
std::optional<std::string> wait_exhausted(depz_link* raw, const std::string& recording)
{
    std::ifstream f(recording, std::ios::binary);
    std::string line, last;
    while (std::getline(f, line))
        if (line.find_first_not_of(" \t\r") != std::string::npos) last = line;
    const Value rec = testjson::parse(last);
    if (!rec.has("dir") || rec.at("dir").as_string() != "rx")
        return std::string("capture ends on a tx record: the C++ replay link cannot tell whether it was written");
    LinkView lv(raw);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!lv.view.replay_exhausted() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(10ms);
    if (!lv.view.replay_exhausted()) return std::string("capture not exhausted: rx chunks never delivered");
    return std::nullopt;
}

std::string read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Runs one scenario file; "" when it passed, else the failure.
std::string run_scenario(const std::filesystem::path& file)
{
    const Value sc = testjson::parse(read_file(file.string()));
    const std::string id = sc.at("id").as_string();
    if (sc.at("schema").as_string() != "depz.scenario/1") return id + ": unknown schema";
    if (file.filename().string() != id + ".json") return id + ": file name is not <id>.json";
    const std::string probe = sc.has("probe") ? sc.at("probe").as_string() : "canonical";
    if (probe != "legacy" && probe != "canonical") return id + ": unknown probe " + probe;
    if (sc.has("identity") && probe != "legacy") return id + ": `identity` is for legacy captures only";
    // No drops: each queue holds everything its scenario takes, with room for
    // what streams on past the last take (drop-oldest would eat items).
    std::map<std::string, std::size_t> sizes;
    for (const auto& step : sc.at("steps").as_array())
        if (step.has("take")) sizes[step.at("take").as_string()] += static_cast<std::size_t>(step.at("count").as_int());
    for (auto& kv : sizes) kv.second += 4096;

    const std::string recording = std::string(DEPZ_RECORDINGS_DIR) + "/" + sc.at("recording").as_string();
    std::unique_ptr<depz::Device> dev;
    depz_link* raw = nullptr;
    try {
        depz::Link link = depz::Link::replay(recording, true);
        raw = link.c_handle();  // the device owns it from here on
        // `usb_model` stands in for the USB id a replay has no way to see.
        std::optional<std::uint16_t> vid, pid;
        if (sc.has("usb_model")) std::tie(vid, pid) = usb_id(sc.at("usb_model").as_string());
        if (probe == "legacy") {
            std::string software_name;
            dev = open_legacy(std::move(link), vid, pid, software_name);
            if (sc.has("identity"))
                if (auto m = mismatch(canon_identity(depz::parse_software_name(software_name)), sc.at("identity"),
                                      "identity"))
                    return id + ": " + *m;
        } else {
            dev = depz::open_on_link(std::move(link), vid, pid, TIMEOUT);
        }
    } catch (const std::exception& e) {
        return id + ": open: " + e.what();
    }

    const std::string cls = canonical_class(dev.get());
    if (cls != sc.at("class").as_string()) return id + ": resolved " + cls + ", not " + sc.at("class").as_string();
    std::map<std::string, Sub> subs;
    Binding b;
    const auto& steps = sc.at("steps").as_array();
    for (std::size_t i = 0; i < steps.size(); i++) {
        try {
            if (i == 0) b = bind(dev.get());
            run_step(b, steps[i], subs, sizes);
        } catch (const StepFailure& e) {
            return id + " step " + std::to_string(i) + " " + describe(steps[i]) + ": " + e.what();
        } catch (const std::exception& e) {
            return id + " step " + std::to_string(i) + " " + describe(steps[i]) + ": raised " + e.what();
        }
    }
    try {
        for (const auto& kv : subs)
            if (const auto n = kv.second.dropped()) fail(kv.first + ": " + std::to_string(n) + " items dropped");
        for (const auto& p : b.post) p(id);
    } catch (const std::exception& e) {
        return id + ": " + e.what();
    }
    if (sc.has("end") && sc.at("end").as_string() == "exhausted")
        if (auto m = wait_exhausted(raw, recording)) return id + ": " + *m;
    dev.reset();
    seen("scenarios/" + id);
    seen("recordings/" + sc.at("recording").as_string());
    return "";
}

}  // namespace

int main(int argc, char** argv)
{
    const char* env = std::getenv("DEPZ_SCENARIOS_DIR");
    const std::filesystem::path dir = env && *env ? env : DEPZ_SCENARIOS_DIR;
    std::vector<std::filesystem::path> files;
    if (argc > 1) {
        files.push_back(dir / (std::string(argv[1]) + ".json"));
        if (!std::filesystem::exists(files[0])) {
            std::cerr << "no scenario " << files[0].string() << "\n";
            return 2;
        }
    } else {
        for (const auto& e : std::filesystem::directory_iterator(dir))
            if (e.path().extension() == ".json") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        if (files.empty()) {
            std::cerr << "no scenarios in " << dir.string() << "\n";
            return 2;
        }
    }
    int failed = 0;
    for (const auto& f : files) {
        std::string err;
        try {
            err = run_scenario(f);
        } catch (const std::exception& e) {
            err = f.filename().string() + ": " + e.what();
        }
        if (err.empty()) {
            std::cout << "ok   " << f.stem().string() << "\n";
        } else {
            std::cout << "FAIL " << err << "\n";
            failed++;
        }
    }
    return failed ? 1 : 0;
}
