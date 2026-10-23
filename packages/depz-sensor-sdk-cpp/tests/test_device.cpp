// test_device.cpp — the C++ live-hardware wrapper against the C SDK's fakes
// (SR04, VL53L4CD, BNO086, TMF8829) over loopback: RAII, exceptions,
// callbacks, streams, record/replay. The lab captures replay as contract 15
// scenarios (test_scenarios.cpp).
// Usage: depz_test_device [name] (no name = all).
#include "depz/device.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string>
#include <thread>

extern "C" {
#include "fake_sr04.h"
#include "fake_vl53l4.h"
#include "fake_bno086.h"
#include "fake_tmf8829.h"
}

#include "json.hpp"

#ifndef DEPZ_VECTORS_DIR
#define DEPZ_VECTORS_DIR "."
#endif

using namespace std::chrono_literals;

namespace {

int g_failed = 0;

#define CHECK(c)                                                                   \
    do {                                                                           \
        if (!(c)) {                                                                \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << ": " #c "\n"; \
            g_failed++;                                                            \
            return;                                                                \
        }                                                                          \
    } while (0)

testjson::Value load_sidecar(const std::string& stem)
{
    std::ifstream f(std::string(DEPZ_VECTORS_DIR) + "/recordings/" + stem + ".expected.json", std::ios::binary);
    if (!f) throw std::runtime_error("no sidecar for " + stem);
    std::stringstream ss;
    ss << f.rdbuf();
    return testjson::parse(ss.str());
}

template <class E, class F>
bool throws(F&& f)
{
    try {
        f();
    } catch (const E&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

// A fake SR04 and an Sr04 on its loopback end.
struct Rig {
    fake_sr04* fake = nullptr;
    std::unique_ptr<depz::Sr04> dev;
    Rig()
    {
        depz_link* host = nullptr;
        fake = fake_sr04_start(&host);
        dev = depz::Sr04::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
    }
    ~Rig()
    {
        dev.reset();
        fake_sr04_stop(fake);
    }
};

void identity_and_types()
{
    Rig r;
    CHECK(r.dev->software_name() == FAKE_SR04_SOFTWARE);
    CHECK(r.dev->device_name() == FAKE_SR04_NAME);
    CHECK(r.dev->serial_number() == FAKE_SR04_SERIAL);
    CHECK(r.dev->sensor_type() == depz::SensorType::Sr04);
    CHECK(std::fabs(r.dev->read_mcu_temperature() - 27.3) < 1e-9);
    auto s = r.dev->sync_time(3);
    CHECK(s.rtt_us >= 0);
    CHECK(r.dev->to_host_time_us(2000000) == 2000000 - s.offset_us);
}

void errors_are_typed()
{
    Rig r;
    try {
        r.dev->request_ok(0x5A);
        CHECK(false);
    } catch (const depz::BusyError&) {
        CHECK(false);
    } catch (const depz::StatusError& e) {
        CHECK(e.status() == depz::Status::ErrInvalidCmd);
        CHECK(e.cmd() == 0x5A);
    }
    r.dev->start();
    try {
        r.dev->measure_once();
        CHECK(false);
    } catch (const depz::BusyError& e) {
        CHECK(e.status() == depz::Status::ErrBusy);
    }
    r.dev->stop();
    CHECK(throws<depz::ArgumentError>([&] { r.dev->set_echo_decay_us(65536); }));
    CHECK(r.dev->set_echo_decay_us(100) == depz::ECHO_DECAY_MIN_US);
}

void timeout_when_silent()
{
    depz_link* host = nullptr;
    fake_sr04* f = fake_silent_start(&host);
    {
        auto dev = depz::Device::open(depz::Link::adopt(host));
        dev->set_timeout(80ms);
        CHECK(throws<depz::TimeoutError>([&] { dev->software_name(); }));
        CHECK(!dev->sensor_type());
    }
    fake_sr04_stop(f);
}

void measure_stream_and_callbacks()
{
    Rig r;
    auto m = r.dev->measure_once();
    CHECK(!m.from_loop && m.valid());
    CHECK(std::fabs(*m.distance_mm() - 5831 * 0.343 / 2) < 1e-6);

    std::atomic<int> seen{0};
    auto unsub = r.dev->on_measurement([&](const depz::Sr04Measurement& x) {
        if (x.from_loop) seen++;
    });
    auto stream = r.dev->stream(16);
    r.dev->start();
    for (int i = 0; i < 3; i++) fake_sr04_send_measurement(r.fake, 0x37);
    for (int i = 0; i < 3; i++) {
        auto x = stream.next(1000ms);
        CHECK(x && x->from_loop);
    }
    CHECK(seen == 3);
    unsub();
    fake_sr04_send_measurement(r.fake, 0x37);
    CHECK(stream.next(1000ms));
    CHECK(seen == 3);
    r.dev->stop();

    fake_sr04_set_echo(r.fake, depz::ECHO_TIMEOUT);
    auto none = r.dev->measure_once();
    CHECK(!none.valid() && !none.distance_mm());
}

void disconnect_ends_streams()
{
    depz_link* host = nullptr;
    fake_sr04* f = fake_sr04_start(&host);
    auto dev = depz::Sr04::open(depz::Link::adopt(host));
    auto ms = dev->stream(8);
    auto evs = dev->events(8);
    std::atomic<bool> lost{false};
    dev->on_event([&](const depz::DeviceEvent& e) {
        if (e.type == depz::DeviceEvent::Type::Disconnected) lost = true;
    });
    fake_sr04_send_measurement(f, 0x37);
    fake_sr04_stop(f);
    CHECK(ms.next(1000ms));        // drains first
    CHECK(!ms.next(1000ms));
    CHECK(ms.closed());
    auto e = evs.next(1000ms);
    CHECK(e && e->type == depz::DeviceEvent::Type::Disconnected);
    CHECK(lost);
    CHECK(dev->closed());
    CHECK(throws<depz::DeviceLostError>([&] { dev->sample_period_us(); }));
    dev->close();
    CHECK(throws<depz::DeviceLostError>([&] { dev->sample_period_us(); }));
}

void promote_and_record_replay()
{
    const std::string path = "test_device_roundtrip.depzrec";
    depz_link* host = nullptr;
    fake_sr04* f = fake_sr04_start(&host);
    std::uint32_t p1 = 0;
    depz::Sr04Measurement m1{};
    {
        auto rec = depz::Link::recording(depz::Link::adopt(host), path, "\"port\": \"loopback\"");
        auto plain = depz::Device::open(std::move(rec));
        CHECK(dynamic_cast<depz::Sr04*>(plain.get()) == nullptr);
        auto dev = depz::Device::promote(std::move(plain));
        auto* s = dynamic_cast<depz::Sr04*>(dev.get());
        CHECK(s != nullptr);
        s->set_sample_period_us(20000);
        p1 = s->sample_period_us();
        m1 = s->measure_once();
    }
    fake_sr04_stop(f);
    CHECK(p1 == 20000);
    {
        auto dev = depz::Device::promote(depz::Device::open(depz::Link::replay(path, true)));
        auto* s = dynamic_cast<depz::Sr04*>(dev.get());
        CHECK(s != nullptr);
        s->set_sample_period_us(20000);
        CHECK(s->sample_period_us() == 20000);
        auto m2 = s->measure_once();
        CHECK(m2.timestamp_us == m1.timestamp_us && m2.echo_time_us == m1.echo_time_us);
    }
    {
        auto dev = depz::Device::open(depz::Link::replay(path, true));
        CHECK(throws<depz::ReplayMismatchError>([&] { dev->device_name(); }));
    }
    std::remove(path.c_str());
}

void l4_fake_roundtrips_and_guards()
{
    depz_link* host = nullptr;
    fake_vl53l4* f = fake_vl53l4_start(&host);
    {
        auto dev = depz::Vl53l4cd::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
        CHECK(dev->is_alive());
        dev->init();
        CHECK(dev->initialized());
        dev->set_range_timing(50, 0);
        auto t = dev->range_timing();
        CHECK(t.timing_budget_ms >= 48 && t.timing_budget_ms <= 50 && t.inter_measurement_ms == 0);
        dev->set_offset_mm(-12);
        CHECK(dev->offset_mm() == -12);
        dev->set_detection_thresholds({100, 300, depz::DetectionWindow::In});
        auto th = dev->detection_thresholds();
        CHECK(th.low_mm == 100 && th.high_mm == 300 && th.window == depz::DetectionWindow::In);
        CHECK(throws<depz::ArgumentError>([&] { dev->set_sigma_threshold_mm(16384); }));
        auto m = dev->measure_once();
        CHECK(m.timestamp_us > 0);
        dev->start_ranging();
        CHECK(dev->ranging());
        CHECK(throws<depz::ArgumentError>([&] { dev->set_offset_mm(1); }));
        dev->stop_ranging();
        CHECK(!dev->ranging());
        CHECK(dev->bridge_info().model_id == depz::vl53l4::MODEL_ID);
    }
    fake_vl53l4_stop(f);
}

// The fake hub: typed errors, the report enum mapping, FRS round trip.
void bno086_fake_errors_and_reports()
{
    using depz::bno086::SensorId;
    depz_link* link = nullptr;
    fake_bno086* fk = fake_bno086_start(&link);
    CHECK(fk);
    auto b = depz::Bno086::open(depz::Link::adopt(link));
    b->hardware_reset();
    CHECK(b->product_id().sw_part_number == FAKE_BNO086_PART);
    {
        const auto ids = b->product_ids();
        CHECK(ids.size() == 4 && ids[0].sw_part_number == FAKE_BNO086_PART && ids[0].reset_cause == 1);
        CHECK(ids.size() == 4 && ids[1].sw_part_number == 10003606u && ids[3].sw_version_patch == 10);
    }
    fake_bno086_busy_next(fk, DEPZ_BNO086_BUSY_RETRIES);
    CHECK(throws<depz::BusyError>([&] { b->disable(SensorId::Gravity); }));
    CHECK(throws<depz::StatusError>([&] { b->frs_read(0x1234); }));  // unrecognised record
    CHECK(throws<depz::ArgumentError>([&] { b->metadata(static_cast<SensorId>(0x0A)); }));
    fake_bno086_calibration_status(fk, 2);
    CHECK(throws<depz::StatusError>([&] { b->set_calibration(); }));
    std::vector<std::uint32_t> words(40);
    for (std::size_t i = 0; i < words.size(); i++) words[i] = static_cast<std::uint32_t>(i * 3);
    fake_bno086_set_record(fk, 0x7979, words.data(), 32);  // the fake holds 32 at most
    words.resize(32);
    CHECK(b->frs_read(0x7979) == words);
    b->frs_write(0x2D3E, {1, 2, 3});
    CHECK(b->frs_read(0x2D3E) == (std::vector<std::uint32_t>{1, 2, 3}));
    {   // hub power (contract 05 §6a): one byte on channel 1, no answer
        b->sleep();
        b->power_on();
        std::uint8_t fr[8];
        CHECK(fake_bno086_executable_frame(fk, 0, fr) == 5 && fr[2] == 1 && fr[3] == 0 && fr[4] == 0x03);
        CHECK(fake_bno086_executable_frame(fk, 1, fr) == 5 && fr[2] == 1 && fr[3] == 1 && fr[4] == 0x02);
        depz::bno086::FeatureRequest req;
        req.interval_us = 10000;
        req.flags = depz::bno086::FEATURE_WAKEUP | depz::bno086::FEATURE_ALWAYS_ON;
        CHECK(!b->enable(SensorId::Accelerometer, req, false));
    }
    const auto g = b->enable(SensorId::GameRotationVector, 200);
    CHECK(g.interval_us == 4000 && depz::Bno086::rate_ok(5000, g));
    CHECK(!b->enable(SensorId::Gyroscope, depz::bno086::FeatureRequest{2000}, false));
    // A personal-activity-classifier and a raw gyroscope: the enum members the
    // C and C++ layers order differently.
    const std::uint8_t cargo[] = {0x1E, 1, 0, 0, 0x85, 6, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100,
                                  0x15, 2, 0, 0, 1, 0, 2, 0, 3, 0, 0x10, 0, 0x40, 0x42, 0x0F, 0};
    std::atomic<int> seen{0};
    auto off = b->on_report([&](const depz::Bno086Report&) { seen++; });
    auto stream = b->reports(8);
    fake_bno086_send_input(fk, 3, 7000000, cargo, sizeof cargo);
    auto pac = stream.next(2000ms);
    CHECK(pac && pac->type == depz::bno086::ReportType::PersonalActivityClassifier);
    CHECK(pac->page_number == 5 && pac->end_of_sequence && pac->most_likely_state == 6);
    CHECK(pac->confidences.size() == 10 && pac->confidences[9] == 100);
    auto raw = stream.next(2000ms);
    CHECK(raw && raw->type == depz::bno086::ReportType::RawSensor && raw->temperature_raw == 16);
    CHECK(raw->sensor_timestamp_us == 1000000 && raw->xyz()[2] == 3.0);
    CHECK(seen == 2);
    off();
    CHECK(depz::bno086::q_point(0x01) == 8 && !depz::bno086::q_point(0x10));
    b.reset();
    fake_bno086_stop(fk);
}

// Replays of the lab captures (sessions, init transcripts, streams) are the
// contract 15 scenarios (tests/test_scenarios.cpp). What a scenario step
// cannot say stays here.

// CNH setup for the lab CH captures (start bin 10, 20 bins, sub-sample 2,
// 4x4 aggregates of 2x2 zones): the on-device buffer it needs.
void cnh_required_memory()
{
    depz::CnhSetup cnh;
    cnh.init_config(10, 20, 2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
    CHECK(cnh.required_memory() == 1708);
}

// A configuration group the bound VL53Lx driver lacks (the signal threshold)
// is refused before the re-init, with nothing written: the capture's own
// configure still replays strictly right after it. No scenario step can say
// "and nothing was written", so this replays each 1D capture up to there.
void vlx_refusal_before_traffic()
{
    const char* stems[] = {"vl53l4cx_uld_as_l4cd_50ms", "vl53l1cx_uld_long_33ms", "vl53l1cb_uld_short_50ms",
                           "vl53l3cx_ulp_33ms", "vl53l0x_uld_long-range_33ms", "vl53l4cx_histogram_medium_50ms",
                           "vl53l1cx_histogram_long_33ms", "vl53l1cb_histogram_medium_33ms",
                           "vl53l3cx_histogram_medium_33ms"};
    const std::string dir = std::string(DEPZ_VECTORS_DIR) + "/recordings/";
    int refused_total = 0;
    for (const char* stem : stems) {
        const testjson::Value exp = load_sidecar(stem);
        auto plain = depz::Device::open(depz::Link::replay(dir + stem + ".depzrec", true));
        plain->set_timeout(2000ms);
        auto dev = depz::Device::promote(std::move(plain));
        auto* v = dynamic_cast<depz::Vl53lx*>(dev.get());
        CHECK(v != nullptr);
        CHECK(v->software_name() == exp.at("software_name").as_string());  // as the capture did
        const std::string kind = exp.at("driver").as_string();
        const auto dk = kind == "uld" ? depz::vl53lx::DriverKind::Uld
                      : kind == "ulp" ? depz::vl53lx::DriverKind::Ulp : depz::vl53lx::DriverKind::Histogram;
        const auto& parg = exp.at("product_arg");
        v->init(dk, parg.is_null() ? std::nullopt : std::optional<std::string>(parg.as_string()));
        if (v->supports(depz::Vl53lxCap::SignalThresh)) continue;
        const auto& mode = exp.at("mode");
        const auto mode_arg = mode.is_null() ? std::nullopt : std::optional<std::string>(mode.as_string());
        const int budget = static_cast<int>(exp.at("budget_ms").as_int());
        CHECK(throws<depz::ArgumentError>([&] { v->configure(budget, 0, mode_arg, std::nullopt, std::nullopt, 512); }));
        v->configure(budget, 0, mode_arg);  // strict replay: the refusal wrote nothing
        refused_total++;
    }
    CHECK(refused_total > 0);
}

// The TMF8829 wrapper against the C SDK's fake bridge (tests/fake_tmf8829.c)
// with the frames of contracts/vectors/tmf8829.json: promote, init (firmware
// download + progress), configure with read-back and its refusals, the
// measurement stream / callbacks / raw frames / events, raw registers, bridge
// info, stop swallowing the BAD_HEADER, a dropped frame_seq, re-init.
depz::bytes tmf_unhex(const std::string& h)
{
    depz::bytes out;
    for (std::size_t i = 0; i + 1 < h.size(); i += 2)
        out.push_back(std::byte{static_cast<std::uint8_t>(std::stoi(h.substr(i, 2), nullptr, 16))});
    return out;
}

const testjson::Value* tmf_named(const testjson::Value& arr, const std::string& name)
{
    for (const auto& v : arr.as_array())
        if (v.at("name").as_string() == name) return &v;
    return nullptr;
}

// Queue the frames of one tmf8829.json measurement scenario on the fake.
bool tmf_script(fake_tmf8829* f, const testjson::Value& root, const std::string& scenario)
{
    const testjson::Value* s = tmf_named(root.at("measurements"), scenario);
    if (!s) return false;
    fake_tmf8829_clear_frames(f);
    for (const auto& e : s->at("frames").as_array()) {
        const testjson::Value* src = tmf_named(root.at("frames"), e.at("raw_case").as_string());
        if (!src) return false;
        const depz::bytes raw = tmf_unhex(src->at("raw").as_string());
        fake_tmf8829_add_frame(f, reinterpret_cast<const std::uint8_t*>(raw.data()), raw.size(),
                               e.at("timestamp_us").as_u64(), e.at("edge_us").as_u64());
    }
    return true;
}

bool tmf_same(const depz::Tmf8829Measurement& m, const testjson::Value& e)
{
    return m.timestamp_us == e.at("timestamp_us").as_int() && m.flags == e.at("flags").as_int() &&
           m.f_number == e.at("f_number").as_u64() && m.ratio_q16 == e.at("ratio_q16").as_u64() &&
           m.n_ref_frames == e.at("n_ref_frames").as_u64() && m.n_histograms == e.at("n_histograms").as_u64() &&
           m.n_problems == e.at("n_problems").as_u64() && m.results.peaks == e.at("peaks").as_u64() &&
           m.results.rows == e.at("rows").as_u64() && m.results.cols == e.at("cols").as_u64() &&
           m.results.distance && m.results.distance->size() == e.at("distance").as_array().size() &&
           (*m.results.distance)[0] == e.at("distance").as_array()[0].as_int() &&
           m.histograms.empty() && m.ref_frames.empty();
}

// The fake and the device on its link; the device goes first.
struct TmfRig {
    fake_tmf8829* fake = nullptr;
    std::unique_ptr<depz::Device> dev;
    ~TmfRig()
    {
        dev.reset();
        if (fake) fake_tmf8829_stop(fake);
    }
};

template <class Pred>
bool wait_for(Pred p)
{
    for (int i = 0; i < 400 && !p(); i++) std::this_thread::sleep_for(5ms);
    return p();
}

void tmf8829_fake_session()
{
    std::ifstream in(std::string(DEPZ_VECTORS_DIR) + "/tmf8829.json", std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    const testjson::Value root = testjson::parse(ss.str());
    const testjson::Value& ex32 = tmf_named(root.at("measurements"), "32x32_ref")->at("expect");
    const testjson::Value& ex8 = tmf_named(root.at("measurements"), "8x8")->at("expect");

    TmfRig rig;
    depz_link* link = nullptr;
    rig.fake = fake_tmf8829_start(&link);
    CHECK(rig.fake);
    const depz::bytes base = tmf_unhex(root.at("config").at("page").as_string());
    fake_tmf8829_set_base_page(rig.fake, reinterpret_cast<const std::uint8_t*>(base.data()));

    // discovery wiring: APP_TMF8829_* promotes to depz::Tmf8829
    rig.dev = depz::Device::promote(depz::Device::open(depz::Link::adopt(link)));
    auto* t = dynamic_cast<depz::Tmf8829*>(rig.dev.get());
    CHECK(t != nullptr);
    CHECK(t->sensor_type() == depz::SensorType::Tmf8829);
    CHECK(throws<depz::ArgumentError>([&] { t->sensor_id(); }));  // before init()
    CHECK(!t->config());

    // init: soft reset, bootloader, the image in <= 1023 B FIFO writes
    std::size_t calls = 0, last_done = 0, last_total = 0;
    const auto id = t->init([&](std::size_t done, std::size_t total) {
        calls++;
        last_done = done;
        last_total = total;
    });
    CHECK(id.app_version == "2.200.0" && id.serial == FAKE_TMF8829_SERIAL);
    CHECK(t->sensor_id().serial == FAKE_TMF8829_SERIAL);
    CHECK(calls == 15 && last_done == 14444 && last_total == 14444);
    {
        const std::uint8_t* img = nullptr;
        const std::size_t n = fake_tmf8829_image(rig.fake, &img);
        const depz::byte_span want = depz::tmf8829::firmware_image();
        CHECK(n == want.size() && std::memcmp(img, want.data(), n) == 0);  // the C++ copy is the same image
        std::uint32_t addr = 0;
        std::uint16_t words = 0;
        std::size_t writes = 0, maxw = 0;
        fake_tmf8829_fifo_params(rig.fake, &addr, &words, &writes, &maxw);
        CHECK(addr == depz::tmf8829::FIRMWARE_LOAD_ADDR && words == 3611 && maxw == 1023);
    }

    // configure: every survivor field written over the stale captured page,
    // verified by read-back
    depz::Tmf8829Options opt;
    opt.period_ms = 50;
    opt.ref_frame = true;
    auto cfg = t->configure("32x32", opt);
    CHECK(cfg.preset == "32x32" && cfg.rows == 32 && cfg.cols == 32 && cfg.ref_frame && !cfg.histograms);
    CHECK(cfg.page.size() == depz::tmf8829::CFG_SIZE && cfg.fields.size() == depz::tmf8829::CFG_FIELDS.size());
    CHECK(cfg.field("period_ms") == 50 && cfg.field("kilo_iterations") == 2344 && cfg.field("nr_peaks") == 1);
    CHECK(cfg.field("signal_strength") == 0 && cfg.field("ref_spad_frame") == 1 && cfg.field("fp_mode") == 3);
    CHECK(cfg.fields[2].first == "fp_mode" && cfg.fields[2].second == 3);
    CHECK(t->config_page() == cfg.page);
    CHECK(throws<depz::ArgumentError>([&] { t->configure("64x64"); }));
    {
        depz::Tmf8829Options bad;
        bad.nr_peaks = 8;
        CHECK(throws<depz::ArgumentError>([&] { t->configure("8x8", bad); }));
        bad.nr_peaks = 1;
        bad.fields = {{"no_such_field", 1}};
        CHECK(throws<depz::ArgumentError>([&] { t->configure("8x8", bad); }));
        bad.fields = {{"fov_correction", 3}};
        fake_tmf8829_pin_fov_correction(rig.fake, true);
        CHECK(throws<depz::ProtocolError>([&] { t->configure("8x8", bad); }));  // did not stick
        fake_tmf8829_pin_fov_correction(rig.fake, false);
    }
    CHECK(t->config() && t->config()->preset == "32x32");  // a refused configure keeps the last good one
    t->configure("32x32", opt);

    // stream: two 32x32 half-frames, each behind its ref-SPAD frame
    CHECK(tmf_script(rig.fake, root, "32x32_ref"));
    std::mutex lock;
    int frames = 0, measured = 0, events = 0;
    std::size_t ref_bytes = 0;
    auto off_frame = t->on_frame([&](const depz::tmf8829::RawFrame& f) {
        std::lock_guard<std::mutex> g(lock);
        frames++;
        if ((std::to_integer<unsigned>(f.data.size() > 5 ? f.data[5] : std::byte{0}) & 0xF0) == 0x30)
            ref_bytes += f.data.size();
    });
    auto off_meas = t->on_measurement([&](const depz::Tmf8829Measurement&) {
        std::lock_guard<std::mutex> g(lock);
        measured++;
    });
    auto off_event = t->on_event([&](const depz::tmf8829::Tmf8829Event& e) {
        std::lock_guard<std::mutex> g(lock);
        if (e.code == depz::tmf8829::EV_INT) events++;
    });
    auto stream = t->measurements(4);
    t->start();
    CHECK(t->streaming());
    std::uint8_t mask = 0, flags = 0;
    CHECK(fake_tmf8829_armed(rig.fake, &mask, &flags) && mask == depz::tmf8829::INT_RESULTS &&
          flags == depz::tmf8829::SF_REF_FRAME);
    auto m = stream.next(2000ms);
    CHECK(m && tmf_same(*m, ex32.as_array()[0]) && m->frames.size() == 2 && m->times.size() == 2);
    CHECK(m->shape() == std::make_pair(32u, 32u) && m->distance_mm.size() == m->results.distance->size());
    CHECK(wait_for([&] {
        std::lock_guard<std::mutex> g(lock);
        return frames == 4 && measured == 1;
    }));
    CHECK(ref_bytes == 2 * 65);  // the ref-SPAD bytes reach on_frame
    CHECK(throws<depz::ArgumentError>([&] { t->configure("8x8"); }));  // not while streaming

    // an INT event reaches the callback
    fake_tmf8829_send_event(rig.fake, depz::tmf8829::INT_MOTION, depz::tmf8829::EV_INT);
    CHECK(wait_for([&] {
        std::lock_guard<std::mutex> g(lock);
        return events == 1;
    }));

    // bridge info + raw reads that run into the FIFO port
    const auto info = t->bridge_info();
    CHECK(info.streaming && info.int_mask == 1 && info.frames == 4);
    for (std::uint8_t i = 0; i < 15; i++) fake_tmf8829_set_reg(rig.fake, static_cast<std::uint8_t>(0xF0 + i), static_cast<std::uint8_t>(0x60 + i));
    fake_tmf8829_set_reg(rig.fake, 0xFF, 0xAB);
    const depz::bytes regs = t->read_registers(0xF0, 1100);
    CHECK(regs.size() == 1100 && regs[0] == std::byte{0x60} && regs[14] == std::byte{0x6E} &&
          regs[15] == std::byte{0xAB} && regs[1099] == std::byte{0xAB});
    const depz::bytes one = {std::byte{0x5A}};
    t->write_registers(0xF0, depz::as_bytes(one));
    CHECK(fake_tmf8829_reg(rig.fake, 0xF0) == 0x5A);

    // stop swallows the one BAD_HEADER a STOP raises mid histogram dump
    fake_tmf8829_bad_header_on_stop(rig.fake, true);
    t->stop();
    CHECK(!t->streaming() && !fake_tmf8829_armed(rig.fake, nullptr, nullptr));
    auto st = t->stream_stats();
    CHECK(st.events_swallowed == 1 && st.frames_completed == 4 && st.measurements == 1 && st.frames_dropped == 0 &&
          st.frames_discarded == 0 && st.frame_parse_errors == 0);
    fake_tmf8829_bad_header_on_stop(rig.fake, false);
    off_frame();
    off_meas();
    off_event();

    // a second stream (8x8): a dropped frame_seq is counted, not fatal
    cfg = t->configure("8x8");
    CHECK(cfg.rows == 8 && !cfg.ref_frame);
    CHECK(tmf_script(rig.fake, root, "8x8"));
    fake_tmf8829_skip_seq_before(rig.fake, 1);
    t->start();
    for (std::size_t i = 0; i < 2; i++) {
        auto m8 = stream.next(2000ms);
        CHECK(m8 && tmf_same(*m8, ex8.as_array()[i]) && m8->frames.size() == 1);
    }
    st = t->stream_stats();
    CHECK(st.frames_dropped == 1 && st.frames_completed == 2 && st.measurements == 2);
    t->stop();
    CHECK(stream.dropped_count() == 0);
    CHECK(throws<depz::TimeoutError>([&] { t->get_measurement(50ms); }));

    // re-init forgets the config; start() then configures "8x8" itself
    t->init();
    CHECK(!t->config());
    fake_tmf8829_clear_frames(rig.fake);
    t->start();
    CHECK(t->config() && t->config()->preset == "8x8");
    t->stop();
}

struct Case {
    const char* name;
    void (*fn)();
};

const Case k_cases[] = {
    {"identity_and_types", identity_and_types},
    {"errors_are_typed", errors_are_typed},
    {"timeout_when_silent", timeout_when_silent},
    {"measure_stream_and_callbacks", measure_stream_and_callbacks},
    {"disconnect_ends_streams", disconnect_ends_streams},
    {"promote_and_record_replay", promote_and_record_replay},
    {"l4_fake_roundtrips_and_guards", l4_fake_roundtrips_and_guards},
    {"bno086_fake_errors_and_reports", bno086_fake_errors_and_reports},
    {"cnh_required_memory", cnh_required_memory},
    {"vlx_refusal_before_traffic", vlx_refusal_before_traffic},
    {"tmf8829_fake_session", tmf8829_fake_session},
};

}  // namespace

int main(int argc, char** argv)
{
    int ran = 0;
    for (const auto& c : k_cases) {
        if (argc > 1 && std::strcmp(argv[1], c.name) != 0) continue;
        ran++;
        const int before = g_failed;
        try {
            c.fn();
        } catch (const std::exception& e) {
            std::cerr << "  FAIL uncaught: " << e.what() << "\n";
            g_failed++;
        }
        std::cout << (g_failed == before ? "ok   " : "FAIL ") << c.name << "\n";
    }
    if (!ran) {
        std::cerr << "no test named " << (argc > 1 ? argv[1] : "?") << "\n";
        return 2;
    }
    return g_failed ? 1 : 0;
}
