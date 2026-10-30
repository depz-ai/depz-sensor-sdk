// test_device.cpp — the C++ live-hardware wrapper against the C SDK's fakes
// (SR04, VL53L4CD, BNO086, TMF8829, VL53L8, VL53Lx, BNO055) over loopback: RAII, exceptions,
// callbacks, streams, record/replay. The lab captures replay as contract 15
// scenarios (test_scenarios.cpp).
// Usage: depz_test_device [name] (no name = all).
#include "depz/dataset.hpp"
#include "depz/device.hpp"
#include "depz/framing.hpp"
#include "depz/recorder.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

extern "C" {
#include "fake_sr04.h"
#include "fake_vl53l4.h"
#include "fake_bno086.h"
#include "fake_tmf8829.h"
#include "fake_vl53l8.h"
#include "fake_vl53lx.h"
#include "fake_bno055.h"
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
    // contract 02 §2 + 07 §3 (ERRATA A2): a CRC type outside 0..3 is refused on the host
    CHECK(throws<depz::ArgumentError>([&] { r.dev->set_payload_crc_type(static_cast<depz::CrcType>(7)); }));
    CHECK(r.dev->set_echo_decay_us(100) == depz::ECHO_DECAY_MIN_US);
}

// Contract 07 §3 (ERRATA A4): a reply shorter than its fields fails that one
// request with ProtocolError; the reader goes on and the next request is
// answered — the device is not dead.
void short_reply_fails_its_request_and_the_reader_goes_on()
{
    Rig r;
    fake_sr04_set_short_temperature(r.fake, true);
    CHECK(throws<depz::ProtocolError>([&] { r.dev->read_mcu_temperature(); }));
    CHECK(throws<depz::ProtocolError>([&] { r.dev->read_mcu_temperature(); }));
    fake_sr04_set_short_temperature(r.fake, false);
    CHECK(r.dev->device_name() == FAKE_SR04_NAME);
    CHECK(std::fabs(r.dev->read_mcu_temperature() - 27.3) < 1e-9);
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

// Contract 07 §3: a port that does not open — as one another program holds —
// is still NoDeviceError, and its message names the open failure.
void a_port_that_does_not_open_names_the_open_failure()
{
#ifdef _WIN32
    const std::string no_port = "\\\\.\\DEPZ_NO_SUCH_PORT";
#else
    const std::string no_port = "/nonexistent/depz-no-such-port";
#endif
    depz::OpenOptions o;
    o.port = no_port;
    o.timeout = 100ms;
    std::string what;
    try {
        depz::open_device(o);
    } catch (const depz::NoDeviceError& e) {
        what = e.what();
    } catch (...) {
    }
    const std::string want = "no DEPZ device answered on " + no_port + ": could not open the port (";
    CHECK(what.compare(0, want.size(), want) == 0);
    CHECK(what.find("cannot open", want.size()) != std::string::npos);
    CHECK(!depz::probe_port(no_port, 100ms));
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

// Contract 07 §3: a callback that throws is counted and dropped — the other
// callback and the stream still get the item, the reader goes on.
void a_throwing_callback_never_stops_the_reader()
{
    Rig r;
    std::atomic<int> seen{0};
    auto boom = r.dev->on_event([](const depz::DeviceEvent&) { throw std::runtime_error("consumer bug"); });
    auto keep = r.dev->on_event([&](const depz::DeviceEvent& e) {
        if (e.type == depz::DeviceEvent::Type::Text) seen++;
    });
    auto evs = r.dev->events(8);
    const uint8_t ab[] = {0xAB};
    fake_sr04_send_raw(r.fake, 0xC5, ab, sizeof ab);
    auto e = evs.next(1000ms);
    CHECK(e && e->type == depz::DeviceEvent::Type::Text);
    CHECK(seen == 1);
    CHECK(r.dev->stats().callback_errors == 1);
    CHECK(!r.dev->software_name().empty());  // the reader goes on

    auto boom_m = r.dev->on_measurement([](const depz::Sr04Measurement&) { throw 42; });
    auto ms = r.dev->stream(4);
    r.dev->start();
    fake_sr04_send_measurement(r.fake, 0x37);
    auto m = ms.next(1000ms);
    CHECK(m && m->from_loop);
    r.dev->stop();
    CHECK(r.dev->stats().callback_errors == 2);
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

// Contract 01 §5 step 5 (ERRATA A1): a gap the host sees in the device->host
// seq is a SeqGap event of its own, ahead of the packet it was seen on, and
// counts in stats().seq_gaps; the first packet after open is no gap, and the
// device-reported RPT 0x84 stays a SequenceError.
void host_seq_gap_is_an_event()
{
    using T = depz::DeviceEvent::Type;
    auto [host, side] = depz::Link::loopback_pair();
    auto dev = depz::Device::open(std::move(host));
    auto events = dev->events(16);
    const depz::bytes payload{std::byte{0x00}};
    for (const std::uint32_t s : {5u, 9u, 10u}) side.write(depz::build_packet(0x7F, payload, s));
    auto ev = events.next(std::chrono::seconds(1));  // seq 5: the first packet, no gap
    CHECK(ev && ev->type == T::Text);
    ev = events.next(std::chrono::seconds(1));
    CHECK(ev && ev->type == T::SeqGap && ev->expected_seq == 6 && ev->received_seq == 9 && ev->missed == 3);
    ev = events.next(std::chrono::seconds(1));  // the packet is still delivered
    CHECK(ev && ev->type == T::Text);
    ev = events.next(std::chrono::seconds(1));  // seq 10 follows 9: no gap
    CHECK(ev && ev->type == T::Text);
    const depz::LinkStats st = dev->stats();
    CHECK(st.seq_gaps == 1 && st.device_seq_errors == 0 && st.rx_packets == 3);
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

// ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
// container is not inflated but refused like any damaged file. The gzipped
// bytes (mtime 0) are the same 70 in every SDK's test; `plain` is what they
// inflate to, a valid capture.
void replay_refuses_a_gzipped_capture()
{
    static const char plain[] = "{\"schema\": \"depz.rec/1\"}\n{\"t\": 0, \"dir\": \"rx\", \"data\": \"aa\"}\n";
    static const unsigned char gzipped[70] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xab, 0x56, 0x2a, 0x4e,
        0xce, 0x48, 0xcd, 0x4d, 0x54, 0xb2, 0x52, 0x50, 0x4a, 0x49, 0x2d, 0xa8, 0xd2, 0x2b,
        0x4a, 0x4d, 0xd6, 0x37, 0x54, 0xaa, 0xe5, 0xaa, 0x56, 0x2a, 0x01, 0x8a, 0x19, 0xe8,
        0x00, 0x85, 0x33, 0x8b, 0x40, 0xb2, 0x45, 0x15, 0x4a, 0x20, 0x4e, 0x62, 0x09, 0x58,
        0x6d, 0x62, 0x22, 0x50, 0x0d, 0x00, 0x7e, 0x3b, 0xc0, 0x0b, 0x3d, 0x00, 0x00, 0x00,
    };
    const std::string path = "test_device_gzipped.depzrec";
    {
        std::ofstream f(path, std::ios::binary);
        f.write(plain, sizeof plain - 1);
    }
    (void)depz::Link::replay(path, false);  // opens: an exception fails the test
    {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(gzipped), sizeof gzipped);
    }
    CHECK(throws<depz::ArgumentError>([&] { depz::Link::replay(path, false); }));
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
    {   // interactive calibration (0x0E, no answer); MotionRequest enabled without read-back
        b->set_motion_intent(depz::bno086::MotionIntent::StationaryNoVibration);
        std::uint8_t params[9];
        CHECK(fake_bno086_last_command(fk, depz::bno086::CMD_INTERACTIVE_CALIBRATION, params) && params[0] == 1);
        b->enable_motion_request();
        CHECK(b->feature(SensorId::MotionRequest).interval_us == 512000);  // the fake's grid
        CHECK(throws<depz::ArgumentError>([&] { b->enable_motion_request(0); }));
    }
    {   // force flush (0xF0): returns once the fake's `ef <sensor>` input report arrived
        auto flushed = b->reports(8);
        b->flush(SensorId::Accelerometer);
        auto fc = flushed.next(1000ms);
        CHECK(fc && fc->type == depz::bno086::ReportType::FlushCompleted &&
              fc->flushed_sensor == static_cast<int>(SensorId::Accelerometer));
    }
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

// VL53L8CH over the C SDK's fake bridge: CNH and the motion indicator share
// DCI 0xBFAC (contract 04 §7, ERRATA B4) — configuring one disarms the other,
// init() disarms both.
void vl53l8ch_cnh_and_motion_exclusive()
{
    depz_link* link = nullptr;
    fake_vl53l8* fake = fake_vl53l8_start(0x0C0B6C9Eu, &link);  // the CH blob's checksum
    CHECK(fake);
    if (!fake) return;
    {
        auto dev = depz::Vl53l8::open(depz::Link::adopt(link), depz::Vl53l8Model::L8CH);
        auto take = [&]() -> std::optional<depz::Vl53l8LiveFrame> {
            auto frames = dev->frames(2);
            if (!fake_vl53l8_send_frame(fake)) return std::nullopt;
            return frames.next(2000ms);
        };
        depz::CnhSetup cnh;
        cnh.init_config(10, 20, 2);
        cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
        dev->init();
        dev->set_resolution(depz::vl53l8::RESOLUTION_8X8);

        dev->configure_motion_indicator(400, 1500);
        dev->configure_cnh(cnh);  // disarms the motion indicator
        CHECK(dev->cnh_setup().has_value());
        dev->start_ranging();
        auto f = take();
        CHECK(f && !f->motion && f->frame.cnh_raw && f->frame.cnh_raw->size() == 1708);
        dev->stop_ranging();

        dev->configure_motion_indicator(400, 1500);  // disarms CNH
        CHECK(!dev->cnh_setup());
        dev->start_ranging();
        CHECK(fake_vl53l8_frame_size(fake) == 1444);
        f = take();
        CHECK(f && f->motion && !f->frame.cnh_raw);
        dev->stop_ranging();

        dev->configure_cnh(cnh);
        dev->init();  // disarms both
        CHECK(!dev->cnh_setup());
        dev->set_resolution(depz::vl53l8::RESOLUTION_8X8);
        dev->start_ranging();
        f = take();
        CHECK(f && !f->motion && !f->frame.cnh_raw);
        dev->stop_ranging();
    }
    fake_vl53l8_stop(fake);
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

// Vl53lx::set_mode over the C fake register file (ERRATA A9): the L1X ULD
// hands back the budget in effect (15 ms is short-only, long takes 20 ms);
// the histogram driver never moves it, so nullopt.
void vlx_set_mode_hands_back_the_budget()
{
    depz_link* host = nullptr;
    fake_vl53l4* f = fake_vl53l4_start(&host);
    {
        auto dev = depz::Vl53lx::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
        dev->init(depz::vl53lx::DriverKind::Uld, std::string("VL53L1CX"));
        dev->configure(15, 0, std::string("short"));
        CHECK(dev->set_mode("long") == std::optional<int>(20));
        CHECK(dev->range_timing().first == 20);
        CHECK(dev->mode() == std::optional<std::string>("long"));
        CHECK(throws<depz::ArgumentError>([&] { dev->set_mode("medium"); }));
        dev->init(depz::vl53lx::DriverKind::Histogram, std::string("VL53L1CX"));
        CHECK(dev->set_mode("long") == std::nullopt);
    }
    fake_vl53l4_stop(f);
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

// Contract 09 `tmf8829` record of a measurement against its tmf8829.json
// expectation, as the Python test_record_value_is_the_contract_09_tmf8829_record:
// distances clock-corrected (x ratio, / 4 unless in mm) and rounded to 0.1 mm
// (ties to even — every expected value here is exact in binary).
bool tmf_record_ok(const depz::Tmf8829Measurement& m, const testjson::Value& e)
{
    const depz::dataset::Value v = depz::dataset::records::tmf8829(m);
    if (v.at("fp_mode").as_int() != e.at("fp_mode").as_int() || v.at("rows").as_int() != e.at("rows").as_int() ||
        v.at("cols").as_int() != e.at("cols").as_int() || v.at("peaks").as_int() != e.at("peaks").as_int() ||
        v.at("f_number").as_int() != e.at("f_number").as_int() || v.at("flags").as_int() != e.at("flags").as_int() ||
        v.at("temperature_c").as_int() != m.temperature_c())
        return false;
    if (v.has("signal") || v.has("noise") || v.has("xtalk")) return false;  // null in both expectations
    const double ratio = static_cast<double>(e.at("ratio_q16").as_u64()) / (8 << 16);
    const double div = e.at("distance_in_mm").as_bool() ? 1.0 : 4.0;
    const auto& want_d = e.at("distance").as_array();
    const auto& want_snr = e.at("snr").as_array();
    const depz::dataset::Value& d = v.at("distance_mm");
    const depz::dataset::Value& snr = v.at("snr");
    if (d.size() != want_d.size() || snr.size() != want_snr.size()) return false;
    for (std::size_t i = 0; i < want_d.size(); i++) {
        const double want = std::nearbyint(static_cast<double>(want_d[i].as_int()) * ratio / div * 10.0) / 10.0;
        if (!d[i].is_double() || d[i].as_double() != want) return false;
        if (!snr[i].is_int() || snr[i].as_int() != want_snr[i].as_int()) return false;
    }
    return true;
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
    CHECK(tmf_record_ok(*m, ex32.as_array()[0]));  // contract 09 `tmf8829` record
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
        CHECK(tmf_record_ok(*m8, ex8.as_array()[i]));
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


// ── datasets: the session recorder (contract 09) ─────────────────────────────

namespace ds = depz::dataset;

std::string tmp_dataset_path(const std::string& name)
{
    static int n = 0;
    const char* dir = std::getenv("TMPDIR");
#ifdef _WIN32
    if (!dir) dir = std::getenv("TEMP");
#endif
    if (!dir) dir = "/tmp";
    return std::string(dir) + "/depz_cpp_device_" + std::to_string(depz::host_now_us()) + "_" + std::to_string(n++) + "_" + name;
}

bool until(const std::function<bool()>& cond, int ms)
{
    for (int i = 0; i < ms / 10; i++) {
        if (cond()) return true;
        std::this_thread::sleep_for(10ms);
    }
    return cond();
}

// Two fake SR04 boards land on one host timeline, ordered by capture time:
// t = device time − offset.
void dataset_recorder_two_sr04()
{
    Rig a, b;
    const std::string path = tmp_dataset_path("two.depzdata");
    {
        ds::RecorderOptions o;
        o.note = "unit test";
        o.temperature_hz = std::nullopt;
        ds::SessionRecorder rec(path, o);
        CHECK(rec.add(*a.dev) == "d0");
        CHECK(rec.add(*b.dev) == "d1");
        CHECK(!rec.recording() && rec.records_written() == 0);
        rec.start();
        rec.start();  // idempotent
        CHECK(rec.recording());
        bool threw = false;
        try {
            rec.add(*a.dev);
        } catch (const depz::ArgumentError&) {
            threw = true;
        }
        CHECK(threw);
        // 50 ms device steps per sample: the step exceeds the gap between the
        // two syncs, so the merged order does not depend on how fast add() ran
        fake_sr04_send_measurement(a.fake, 0x37);
        std::this_thread::sleep_for(20ms);
        fake_sr04_send_measurement(b.fake, 0x37);
        std::this_thread::sleep_for(20ms);
        fake_sr04_send_measurement(a.fake, 0x37);
        std::this_thread::sleep_for(20ms);
        fake_sr04_send_measurement(b.fake, 0x36);
        CHECK(until([&] { return rec.records_written() >= 4; }, 2000));
        rec.stop();
        CHECK(!rec.recording() && rec.records_written() == 4);  // the finished file's count
        fake_sr04_send_measurement(a.fake, 0x37);  // unhooked: dropped
        std::this_thread::sleep_for(20ms);
    }
    const ds::Reader r = ds::Reader::open(path);
    std::remove(path.c_str());
    CHECK(r.note == "unit test");
    CHECK(r.devices.size() == 2);
    for (const auto& [id, meta] : r.devices) {
        CHECK(id == "d0" || id == "d1");
        CHECK(meta.sensor_type == "sr04" && meta.serial == FAKE_SR04_SERIAL && meta.software_name == FAKE_SR04_SOFTWARE);
    }
    CHECK(r.records.size() == 4);
    const std::vector<std::string> order = {"d0", "d1", "d0", "d1"};
    for (std::size_t i = 0; i < r.records.size(); i++) {
        const auto& rec = r.records[i];
        CHECK(rec.device_id == order[i] && rec.kind == "sr04");
        CHECK(i == 0 || rec.t_host_us >= r.records[i - 1].t_host_us);
        // t on the host clock: the device clocks are gone
        CHECK(std::llabs(rec.t_host_us - static_cast<std::int64_t>(depz::host_now_us())) < 10000000LL);
        CHECK(rec.value.at("echo_us").as_int() == 5831);
        CHECK(rec.value.at("source").as_string() == (i == 3 ? "once" : "loop"));
    }
}

// The temperature sampler writes `temperature` records (the MCU thermometer)
// to a gzip file the reader opens.
void dataset_recorder_temperature_gz()
{
    Rig a;
    const std::string path = tmp_dataset_path("temp.depzdata.gz");
    {
        ds::RecorderOptions o;
        o.temperature_hz = 50.0;
        ds::SessionRecorder rec(path, o);
        CHECK(rec.add(*a.dev, std::string("imu"), 3) == "imu");
        rec.start();
        std::this_thread::sleep_for(150ms);
    }  // the destructor stops: the file gets its gzip trailer
    std::ifstream f(path, std::ios::binary);
    char magic[2] = {0, 0};
    f.read(magic, 2);
    CHECK(static_cast<unsigned char>(magic[0]) == 0x1F && static_cast<unsigned char>(magic[1]) == 0x8B);
    f.close();
    const ds::Reader r = ds::Reader::open(path);
    std::remove(path.c_str());
    std::size_t temps = 0;
    for (const auto& rec : r.records) {
        if (rec.kind != "temperature") continue;
        temps++;
        CHECK(rec.device_id == "imu");
        CHECK(rec.value.at("celsius").as_double() == 27.3 && rec.value.at("source").as_string() == "mcu");
    }
    CHECK(temps >= 2);
}

// Contract 09 `bno086`: every report at its measurement time on the host
// timeline (device timestamp − offset), not its arrival time; one cargo's
// reports come back sorted by t.
void dataset_recorder_bno086()
{
    // 0xFB base delta 20 ticks, a rotation vector (delay 3 ticks) and an accelerometer
    static const std::uint8_t cargo[] = {
        0xFB, 20, 0, 0, 0,
        0x05, 9, 0x03, 3, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
        0x01, 10, 0x02, 0, 0x00, 0x01, 0x00, 0xFF, 0x80, 0x09};
    depz_link* link = nullptr;
    fake_bno086* fk = fake_bno086_start(&link);
    CHECK(fk);
    const std::string path = tmp_dataset_path("imu.depzdata");
    {
        auto b = depz::Bno086::open(depz::Link::adopt(link));
        b->hardware_reset();
        ds::SessionRecorder rec(path);
        CHECK(rec.add(*b, std::nullopt, 3) == "d0");
        rec.start();
        fake_bno086_send_input(fk, DEPZ_SHTP_CH_INPUT_NORMAL, 5000000, cargo, sizeof cargo);
        CHECK(until([&] { return rec.records_written() >= 2; }, 2000));
        CHECK(rec.records_written() == 2);
    }
    fake_bno086_stop(fk);
    const ds::Reader r = ds::Reader::open(path);
    std::remove(path.c_str());
    const auto d0 = r.devices.find("d0");
    CHECK(d0 != r.devices.end() && d0->second.sensor_type == "bno086" && d0->second.serial == "SN0086");
    CHECK(r.records.size() == 2);
    const std::int64_t off = d0->second.time_sync.offset_us;
    const auto& r0 = r.records[0];
    const auto& r1 = r.records[1];
    // the accelerometer (delay 0) comes before the rotation vector (delay 3 ticks)
    CHECK(r0.kind == "bno086" && r1.kind == "bno086");
    CHECK(r0.value.at("type").as_string() == "Acceleration" && r1.value.at("type").as_string() == "RotationVector");
    CHECK(r0.t_host_us == 5000000 - 2000 - off && r1.t_host_us == 5000000 - 2000 + 300 - off);
    CHECK(r0.value.at("timestamp_us").as_int() == 5000000 - 2000);
    CHECK(r1.value.at("i").as_double() == 0.5 && r1.value.at("real").as_double() == 0.0 && r1.value.at("accuracy_rad").as_double() == 0.5);
    CHECK(!r0.value.has("i") && !r1.value.has("x"));
}

// ── the device core beyond the sensor classes ─────────────────────────────

// port(), stats(), time_sync() before and after a sync, request() with a
// matcher, send(), the host-side refusals of sync_pin(), and the value
// arithmetic of the measurement structs (contract 03 §4; the 1D family's
// plottable statuses 0 / 6 / 11 and the first plottable target).
void device_core_extras()
{
    Rig r;
    CHECK(!r.dev->port().empty());
    CHECK(!r.dev->closed());
    CHECK(!r.dev->time_sync().has_value());
    CHECK(r.dev->device_name() == FAKE_SR04_NAME);
    const depz::LinkStats st = r.dev->stats();
    CHECK(st.tx_packets >= 1 && st.rx_packets >= 1 && st.tx_bytes > 0 && st.rx_bytes > 0);
    CHECK(st.crc_errors == 0 && st.header_errors == 0 && st.trash_bytes == 0 && st.seq_gaps == 0);
    const depz::TimeSync ts = r.dev->sync_time(2);
    const auto back = r.dev->time_sync();
    CHECK(back && back->offset_us == ts.offset_us && back->rtt_us == ts.rtt_us &&
          back->synced_at_host_us == ts.synced_at_host_us);

    // request(): the first packet the matcher accepts — GET_DEVICE_NAME's
    // RPT_TEXT (0x81) carries the echoed cmd and the name
    const depz::bytes name = r.dev->request(0x03, {}, [](std::uint8_t rpt, depz::byte_span p) {
        return rpt == 0x81 && !p.empty();
    });
    CHECK(name.size() > 1 && depz::strip_device_string(depz::byte_span(name).subspan(1)) == FAKE_SR04_NAME);
    // send(): fire and forget — START_LOOP reaches the fake, no reply is awaited
    r.dev->send(0x37);
    CHECK(until([&] { return fake_sr04_loop_running(r.fake); }, 2000));
    r.dev->stop();
    CHECK(!fake_sr04_loop_running(r.fake));

    // the payload-CRC type round-trips; a value past 0..3 that does reach the
    // device is a typed StatusError (ERR_INVALID_PARAM) and changes nothing.
    // The sync-pin pair answers mode 0, polarity 0; a pin outside 1..5 never
    // reaches the fake
    CHECK(r.dev->payload_crc_type() == depz::CrcType::None);
    r.dev->set_payload_crc_type(depz::CrcType::Crc16);
    CHECK(fake_sr04_payload_crc_type(r.fake) == 2 && r.dev->payload_crc_type() == depz::CrcType::Crc16);
    bool invalid_param = false;
    try {
        const depz::bytes seven{std::byte{7}};
        r.dev->request_ok(0x09, seven);
    } catch (const depz::StatusError& e) {
        invalid_param = e.cmd() == 0x09 && e.status() == depz::Status::ErrInvalidParam;
    }
    CHECK(invalid_param);
    CHECK(r.dev->payload_crc_type() == depz::CrcType::Crc16);
    const depz::SyncPinConfig sp = r.dev->sync_pin(1);
    CHECK(sp.pin == 1 && static_cast<int>(sp.mode) == 0 && static_cast<int>(sp.polarity) == 0);
    CHECK(throws<depz::ArgumentError>([&] { (void)r.dev->sync_pin(0); }));
    CHECK(throws<depz::ArgumentError>([&] { (void)r.dev->sync_pin(6); }));
    CHECK(throws<depz::ArgumentError>([&] {
        r.dev->set_sync_pin(depz::SyncPinConfig{9, depz::SyncPinMode::In, depz::SyncPinPolarity::IdleLow});
    }));
    r.dev->set_sync_pin(depz::SyncPinConfig{1, depz::SyncPinMode::In, depz::SyncPinPolarity::IdleLow});

    depz::Sr04Measurement m{0, 5831, false};
    CHECK(m.valid() && m.distance_mm() && std::fabs(*m.distance_mm() - 1000.0165) < 1e-9);
    CHECK(m.distance_mm_at(20.0) && std::fabs(*m.distance_mm_at(20.0) - 1001.24101) < 1e-9);
    m.echo_time_us = depz::ECHO_TIMEOUT;
    CHECK(!m.valid() && !m.distance_mm() && !m.distance_mm_at(20.0));

    depz::Vl53l4cdMeasurement l4{0, {}};
    CHECK(l4.valid() && l4.status_text() == depz_vl53l4cd_status_text(0));
    l4.r.range_status = 1;
    CHECK(!l4.valid() && l4.status_text() == depz_vl53l4cd_status_text(1) && l4.status_text() != depz_vl53l4cd_status_text(0));

    depz::Vl53lxMeasurement lx;
    lx.distance_mm = 500;
    lx.status = 4;
    CHECK(!lx.plottable() && !lx.primary_distance_mm());
    lx.status = 6;
    CHECK(lx.plottable() && lx.primary_distance_mm() == 500);
    lx.status = 11;
    CHECK(lx.plottable());
    depz::Vl53lxTarget t0, t1;
    t0.distance_mm = 300;
    t0.status = 4;
    t1.distance_mm = 900;
    t1.status = 11;
    lx.targets = {t0, t1};
    CHECK(lx.primary_distance_mm() == 900);
    t1.status = 7;
    lx.targets = {t0, t1};
    CHECK(!lx.primary_distance_mm());  // targets present, none plottable: no distance
}

// ── contract 09 record values of the live layer ───────────────────────────

bool record_is(const ds::Value& got, const std::string& want)
{
    const ds::Value w = ds::Value::parse(want);
    if (got == w) return true;
    std::cerr << "  record: got  " << got.to_json() << "\n          want " << want << "\n";
    return false;
}

// The record value (`v`) of every result type of this layer, key for key the
// Python SessionRecorder (contract 09): sr04, vl53l4, vl53l8 — the zone
// arrays cut to the resolution, the four layers on request, the motion
// block when present — vl53l8_cnh (base64, start_bin = floor(offset / 2048)),
// vl53lx with and without targets, bno055 with every channel and with one,
// temperature.
void dataset_live_records()
{
    namespace rec = depz::dataset::records;
    CHECK(record_is(rec::sr04(depz::Sr04Measurement{1000, 5831, true}), R"({"echo_us": 5831, "source": "loop"})"));
    CHECK(record_is(rec::sr04(depz::Sr04Measurement{1000, depz::ECHO_TIMEOUT, false}),
                    R"({"echo_us": 65535, "source": "once"})"));

    depz::Vl53l4cdMeasurement l4{0, {}};
    l4.r.distance_mm = 1234;
    l4.r.sigma_mm = 3;
    l4.r.signal_rate_kcps = 120;
    l4.r.ambient_rate_kcps = 7;
    l4.r.number_of_spad = 180;
    l4.r.stream_count = 9;
    CHECK(record_is(rec::vl53l4(l4), R"({"range_status": 0, "distance_mm": 1234, "sigma_mm": 3, "signal_rate_kcps": 120, )"
                                     R"("ambient_rate_kcps": 7, "number_of_spad": 180, "stream_count": 9})"));

    depz::Vl53l8LiveFrame f{};
    f.frame.resolution = 16;
    f.frame.silicon_temp_degc = 31;
    for (int i = 0; i < 64; i++) {  // a 64-zone buffer: the record keeps the resolution's zones
        f.frame.distance_mm.push_back(100 + 10 * i);
        f.frame.target_status.push_back(5);
        f.frame.nb_target_detected.push_back(i < 8 ? 1 : 0);
        f.frame.signal_per_spad.push_back(i + 0.5);
        f.frame.ambient_per_spad.push_back(i + 0.25);
        f.frame.range_sigma_mm.push_back(i + 0.75);
        f.frame.reflectance.push_back(2.0 * i);
    }
    CHECK(record_is(rec::vl53l8(f, false), "{\"resolution\": 16, \"silicon_temp_degc\": 31, \"distance_mm\": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250], \"target_status\": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5], \"nb_target_detected\": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0]}"));
    CHECK(record_is(rec::vl53l8(f, true), "{\"resolution\": 16, \"silicon_temp_degc\": 31, \"distance_mm\": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250], \"target_status\": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5], \"nb_target_detected\": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0], \"signal_per_spad\": [0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5], \"ambient_per_spad\": [0.25, 1.25, 2.25, 3.25, 4.25, 5.25, 6.25, 7.25, 8.25, 9.25, 10.25, 11.25, 12.25, 13.25, 14.25, 15.25], \"range_sigma_mm\": [0.75, 1.75, 2.75, 3.75, 4.75, 5.75, 6.75, 7.75, 8.75, 9.75, 10.75, 11.75, 12.75, 13.75, 14.75, 15.75], \"reflectance\": [0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0, 26.0, 28.0, 30.0]}"));
    depz::Vl53l8Motion mo{};
    mo.global_indicator_1 = 7;
    mo.global_indicator_2 = 9;
    mo.status = 0;
    mo.nb_of_detected_aggregates = 2;
    mo.nb_of_aggregates = 4;
    for (std::size_t i = 0; i < 32; i++) mo.motion[i] = static_cast<std::uint32_t>(3 * i);
    f.motion = mo;
    CHECK(record_is(rec::vl53l8(f, false), "{\"resolution\": 16, \"silicon_temp_degc\": 31, \"distance_mm\": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250], \"target_status\": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5], \"nb_target_detected\": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0], \"motion\": {\"global_indicator_1\": 7, \"global_indicator_2\": 9, \"status\": 0, \"nb_of_detected_aggregates\": 2, \"nb_of_aggregates\": 4, \"motion\": [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93]}}"));

    depz::CnhSetup setup;
    setup.nb_of_aggregates = 8;
    setup.feature_length = 6;
    setup.sum_span = 2;
    setup.ref_bin_offset = -6143;  // -6143 // 2048 == -3 (floor, not truncation)
    const depz::bytes raw = {std::byte{1}, std::byte{2}, std::byte{3}};
    CHECK(record_is(rec::vl53l8_cnh(depz::as_bytes(raw), setup),
                    R"({"nb_aggregates": 8, "feature_length": 6, "sum_span": 2, "start_bin": -3, "cnh_raw_b64": "AQID"})"));
    setup.ref_bin_offset = 4096;
    CHECK(record_is(rec::vl53l8_cnh(depz::byte_span(raw).first(2), setup),
                    R"({"nb_aggregates": 8, "feature_length": 6, "sum_span": 2, "start_bin": 2, "cnh_raw_b64": "AQI="})"));
    CHECK(record_is(rec::vl53l8_cnh(depz::byte_span(raw).first(1), setup),
                    R"({"nb_aggregates": 8, "feature_length": 6, "sum_span": 2, "start_bin": 2, "cnh_raw_b64": "AQ=="})"));
    CHECK(record_is(rec::vl53l8_cnh({}, setup),
                    R"({"nb_aggregates": 8, "feature_length": 6, "sum_span": 2, "start_bin": 2, "cnh_raw_b64": ""})"));

    depz::Vl53lxMeasurement lx;
    lx.distance_mm = 1234;
    lx.sigma_mm = 2.5;
    lx.signal_kcps = 120.25;
    lx.ambient_kcps = 3.5;
    lx.spads = 180.0;
    CHECK(record_is(rec::vl53lx(lx, "vl53l1cb", "uld"),
                    R"({"product": "vl53l1cb", "driver": "uld", "status": 0, "distance_mm": 1234, )"
                    R"("sigma_mm": 2.5, "signal_kcps": 120.25, "ambient_kcps": 3.5, "spads": 180.0})"));
    depz::Vl53lxTarget t0, t1;
    t0.distance_mm = 1234;
    t0.status = 0;
    t0.signal_kcps = 120.25;
    t1.distance_mm = 2100;
    t1.status = 6;
    t1.signal_kcps = 8.5;
    lx.targets = {t0, t1};
    CHECK(record_is(rec::vl53lx(lx, "", ""),
                    R"({"product": "", "driver": "", "status": 0, "distance_mm": 1234, )"
                    R"("sigma_mm": 2.5, "signal_kcps": 120.25, "ambient_kcps": 3.5, "spads": 180.0, )"
                    R"("targets": [{"distance_mm": 1234, "status": 0, "signal_kcps": 120.25}, )"
                    R"({"distance_mm": 2100, "status": 6, "signal_kcps": 8.5}]})"));

    depz::Bno055Sample s{};
    s.units.accel_mg = true;   // 0x01
    s.units.euler_rad = true;  // 0x04
    s.units.android = true;    // 0x80
    s.quaternion = std::array<double, 4>{1.0, 0.0, 0.0, 0.0};
    s.euler = std::array<double, 3>{0.5, -0.25, 1.75};
    s.accel = std::array<double, 3>{0.0, 0.0, 9.81};
    s.gyro = std::array<double, 3>{0.1, 0.2, 0.3};
    s.mag = std::array<double, 3>{10.5, -20.25, 30.0};
    s.linear_accel = std::array<double, 3>{0.01, 0.02, 0.03};
    s.gravity = std::array<double, 3>{0.0, 0.0, 9.8};
    s.temperature = 24.0;
    depz::bno055::CalibStatus cal;
    cal.system = 3;
    cal.gyro = 3;
    cal.accel = 2;
    cal.mag = 1;
    s.calibration = cal;
    CHECK(record_is(rec::bno055(s),
                    R"({"unit_sel": 133, "quaternion": [1.0, 0.0, 0.0, 0.0], "euler": [0.5, -0.25, 1.75], )"
                    R"("accel": [0.0, 0.0, 9.81], "gyro": [0.1, 0.2, 0.3], "mag": [10.5, -20.25, 30.0], )"
                    R"("linear_accel": [0.01, 0.02, 0.03], "gravity": [0.0, 0.0, 9.8], "temperature": 24.0, )"
                    R"("calib": [3, 3, 2, 1]})"));
    depz::Bno055Sample one{};
    one.accel = std::array<double, 3>{0.0, 0.0, 9.81};
    CHECK(record_is(rec::bno055(one), R"({"unit_sel": 0, "accel": [0.0, 0.0, 9.81]})"));

    CHECK(record_is(rec::temperature(27.3, "mcu"), R"({"celsius": 27.3, "source": "mcu"})"));
}

// ── the sensor classes over the C SDK's register-level fakes ──────────────
// The C++ side of the C SDK's test_io.c cases over the same fakes
// (fake_vl53l8, fake_vl53lx, fake_bno055, fake_bno086, fake_vl53l4): the
// same calls, the same numbers, the C error codes as typed exceptions.

std::uint32_t le32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 |
           static_cast<std::uint32_t>(p[2]) << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}

depz::bytes bytes_of(std::initializer_list<int> v)
{
    depz::bytes out;
    for (int x : v) out.push_back(std::byte{static_cast<std::uint8_t>(x)});
    return out;
}

constexpr std::uint32_t L8_CRC_CX = 0xCADF7CAFu;  // the checksum the VL53L8CX 2.1.0 blob boots with
constexpr std::uint32_t L8_CRC_CH = 0x0C0B6C9Eu;  // the LMZ blob (VL53L8CH)

// A fake VL53L8 and a Vl53l8 on its link; the device goes first.
struct L8Rig {
    fake_vl53l8* fake = nullptr;
    std::unique_ptr<depz::Vl53l8> dev;
    L8Rig(depz::Vl53l8Model model, std::uint32_t crc)
    {
        depz_link* link = nullptr;
        fake = fake_vl53l8_start(crc, &link);
        if (!fake) throw std::runtime_error("fake_vl53l8_start");
        dev = depz::Vl53l8::open(depz::Link::adopt(link), model);
        dev->set_timeout(1000ms);
    }
    ~L8Rig()
    {
        dev.reset();
        fake_vl53l8_stop(fake);
    }
};

// test_io.c l8_init_downloads_and_reads_the_defaults / l8_configuration_round_trips:
// init with progress, the ULD defaults, every setting's round trip and its
// host-side refusal, raw DCI access.
void vl53l8_fake_init_and_configuration()
{
    using depz::ArgumentError;
    L8Rig r(depz::Vl53l8Model::L8CX, L8_CRC_CX);
    auto& l8 = *r.dev;
    std::uint8_t b[64];
    CHECK(l8.model() == depz::Vl53l8Model::L8CX);
    CHECK(l8.is_alive() && !l8.initialized());
    CHECK(throws<ArgumentError>([&] { l8.dci_read(0x5450, 8); }));  // init() first
    int notes = 0;
    bool done = false;
    std::size_t last_done = 0, last_total = 0;
    l8.init([&](const std::string& phase, std::size_t d, std::size_t t) {
        if (phase == "download") {
            last_done = d;
            last_total = t;
            return;
        }
        notes++;
        if (phase == "Sensor init done") done = true;
    });
    CHECK(l8.initialized() && !l8.ranging() && fake_vl53l8_boots(r.fake) == 1);
    CHECK(done && notes > 3 && last_done == 0x5000 && last_total == 0x5000);  // bank 3
    CHECK(!l8.module_type());  // read on the L5/L7 only
    CHECK(l8.resolution() == depz::vl53l8::RESOLUTION_4X4 && l8.ranging_frequency_hz() == 1);
    CHECK(l8.ranging_mode() == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS && l8.integration_time_ms() == 5);
    CHECK(l8.sharpener_percent() == 5 && l8.target_order() == DEPZ_VL53L8_TARGET_ORDER_CLOSEST);
    CHECK(l8.power_mode() == DEPZ_VL53L8_POWER_MODE_WAKEUP && l8.xtalk_margin() == 0);

    l8.set_resolution(depz::vl53l8::RESOLUTION_8X8);
    CHECK(l8.resolution() == 64 && fake_vl53l8_dci(r.fake, 0x5450, b, sizeof b) == 8 && b[0] == 8 && b[1] == 8);
    CHECK(throws<ArgumentError>([&] { l8.set_ranging_frequency_hz(16); }));  // <= 15 Hz at 8x8
    l8.set_ranging_frequency_hz(15);
    CHECK(l8.ranging_frequency_hz() == 15);
    l8.set_resolution(depz::vl53l8::RESOLUTION_4X4);
    CHECK(throws<ArgumentError>([&] { l8.set_resolution(32); }));
    const std::size_t writes = fake_vl53l8_write_count(r.fake);
    CHECK(throws<ArgumentError>([&] { l8.set_ranging_frequency_hz(1); }));  // VL53L8 floor 2 Hz
    CHECK(throws<ArgumentError>([&] { l8.set_ranging_frequency_hz(61); }));
    CHECK(fake_vl53l8_write_count(r.fake) == writes);  // refused before the bus
    l8.set_ranging_mode(DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS);
    CHECK(l8.ranging_mode() == DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS);
    l8.set_integration_time_ms(20);
    CHECK(l8.integration_time_ms() == 20 && fake_vl53l8_dci(r.fake, 0x545C, b, sizeof b) == 20 && le32(b) == 20000);
    CHECK(throws<ArgumentError>([&] { l8.set_integration_time_ms(1001); }));
    l8.set_ranging_mode(DEPZ_VL53L8_RANGING_MODE_CONTINUOUS);
    CHECK(l8.ranging_mode() == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS);
    CHECK(throws<ArgumentError>([&] { l8.set_ranging_mode(2); }));
    l8.set_sharpener_percent(99);
    CHECK(l8.sharpener_percent() == 99 && fake_vl53l8_dci(r.fake, 0xAED8, b, sizeof b) == 16 && b[0xD] == 99 * 255 / 100);
    CHECK(throws<ArgumentError>([&] { l8.set_sharpener_percent(100); }));
    l8.set_target_order(DEPZ_VL53L8_TARGET_ORDER_STRONGEST);
    CHECK(l8.target_order() == DEPZ_VL53L8_TARGET_ORDER_STRONGEST);
    CHECK(throws<ArgumentError>([&] { l8.set_target_order(3); }));
    l8.set_xtalk_margin(3);  // 3 kcps/SPAD -> 3 x 2048 on the wire
    CHECK(l8.xtalk_margin() == 3 && fake_vl53l8_dci(r.fake, 0xAD94, b, sizeof b) == 16 && le32(b) == 3 * 2048);
    CHECK(throws<ArgumentError>([&] { l8.set_xtalk_margin(10001); }));

    depz::bytes f = l8.dci_read(0x5458, 4);  // the frequency entry
    CHECK(f.size() == 4 && f[1] == std::byte{15});
    f[1] = std::byte{20};
    l8.dci_write(0x5458, f);
    CHECK(l8.ranging_frequency_hz() == 20);
}

// test_io.c l8_thresholds_motion_and_power / l8_xtalk_calibration_round_trip /
// l8_start_stop_and_refusals: thresholds scaled on the wire, the motion
// indicator, the xtalk calibration and its blob, start / stop and what the
// stream refuses, the board commands an L8 does not have, the power modes.
void vl53l8_fake_thresholds_calibration_and_ranging()
{
    using depz::ArgumentError;
    L8Rig r(depz::Vl53l8Model::L8CX, L8_CRC_CX);
    auto& l8 = *r.dev;
    l8.init();
    std::uint8_t b[512];

    CHECK(!l8.detection_thresholds_enabled());
    std::vector<depz::vl53l8::DetectionThreshold> th(2);
    th[0] = {200, 600, DEPZ_VL53L8_DIST_MM, DEPZ_VL53L8_THRESH_IN_WINDOW, 5, DEPZ_VL53L8_THRESH_OP_OR};
    th[1] = {1, 3, DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS, DEPZ_VL53L8_THRESH_GREATER_THAN_MAX, 6, DEPZ_VL53L8_THRESH_OP_AND};
    l8.set_detection_thresholds(th);
    const auto got = l8.detection_thresholds();
    CHECK(got.size() == 64 && got[0].low_thresh == 200 && got[0].high_thresh == 600 && got[0].zone_num == 5 &&
          got[0].measurement == DEPZ_VL53L8_DIST_MM && got[0].type == DEPZ_VL53L8_THRESH_IN_WINDOW);
    CHECK(got[1].low_thresh == 1 && got[1].high_thresh == 3 && got[1].zone_num == 6 &&
          got[1].measurement == DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS && got[1].operation == DEPZ_VL53L8_THRESH_OP_AND);
    CHECK(got[2].low_thresh == 0 && got[2].high_thresh == 0 && got[2].zone_num == 0);
    // on the wire the distance is x4 and the signal x2048 (plugin scaling)
    CHECK(fake_vl53l8_dci(r.fake, 0xB6E8, b, sizeof b) >= 20);
    CHECK(static_cast<std::int32_t>(le32(b)) == 800 && static_cast<std::int32_t>(le32(b + 4)) == 2400);
    CHECK(static_cast<std::int32_t>(le32(b + 12)) == 2048 && static_cast<std::int32_t>(le32(b + 16)) == 3 * 2048);
    l8.set_detection_thresholds_enabled(true);
    CHECK(l8.detection_thresholds_enabled());
    l8.set_detection_thresholds_enabled(false);
    CHECK(!l8.detection_thresholds_enabled());
    l8.set_detection_thresholds_auto_stop(true);
    CHECK(fake_vl53l8_dci(r.fake, 0xDB80, b, sizeof b) == 4 && b[3] == 1);
    l8.set_detection_thresholds_auto_stop(false);
    CHECK(fake_vl53l8_dci(r.fake, 0xDB80, b, sizeof b) == 4 && b[3] == 0);
    CHECK(throws<ArgumentError>([&] { l8.set_detection_thresholds(std::vector<depz::vl53l8::DetectionThreshold>(65)); }));

    // xtalk calibration: argument checks off the bus, the run, the blob
    std::size_t writes = fake_vl53l8_write_count(r.fake);
    CHECK(throws<ArgumentError>([&] { l8.calibrate_xtalk(0, 4, 600); }));
    CHECK(throws<ArgumentError>([&] { l8.calibrate_xtalk(50, 17, 600); }));
    CHECK(throws<ArgumentError>([&] { l8.calibrate_xtalk(50, 4, 3001); }));
    CHECK(fake_vl53l8_write_count(r.fake) == writes);
    CHECK(l8.calibrate_xtalk(50, 4, 600));
    std::size_t table_len = 0;
    std::uint16_t d4 = 0, r16 = 0;
    std::uint8_t nb = 0;
    CHECK(fake_vl53l8_calibrations(r.fake, &table_len, &d4, &r16, &nb) == 1 && d4 == 600 * 4 && r16 == 50 * 16 && nb == 4);
    const std::size_t cx_table_len = table_len;
    CHECK(l8.resolution() == 16);  // the settings around the run restored
    const depz::bytes blob = l8.caldata_xtalk();
    const depz::bytes footer = bytes_of({0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04});
    CHECK(blob.size() == DEPZ_VL53L8_XTALK_BUFFER_SIZE && std::equal(footer.begin(), footer.end(), blob.end() - 8));
    {   // the signal grid the fake's calibration computed, zone by zone (the blob is big-endian words)
        depz::bytes w = blob;
        for (std::size_t i = 0; i + 4 <= w.size(); i += 4) std::reverse(w.begin() + static_cast<std::ptrdiff_t>(i),
                                                                       w.begin() + static_cast<std::ptrdiff_t>(i) + 4);
        const auto* g = reinterpret_cast<const std::uint8_t*>(w.data()) + 0x34;
        for (unsigned z = 0; z < 64; z++) CHECK(le32(g + 4 * z) == fake_vl53l8_xtalk_grid_value(600, 50, 4, z));
    }
    CHECK(throws<ArgumentError>([&] { l8.set_caldata_xtalk(depz::byte_span(blob).first(775)); }));
    l8.set_caldata_xtalk(blob);
    l8.set_resolution(depz::vl53l8::RESOLUTION_8X8);  // the blob reaches the FW verbatim on the 8x8 upload
    std::uint8_t again[DEPZ_VL53L8_XTALK_BUFFER_SIZE];
    CHECK(fake_vl53l8_last_xtalk(r.fake, again) && std::memcmp(again, blob.data(), sizeof again) == 0);
    l8.set_resolution(depz::vl53l8::RESOLUTION_4X4);
    fake_vl53l8_xtalk_cal_fails(r.fake, true);  // "coverglass too good"
    CHECK(!l8.calibrate_xtalk(20, 1, 3000));
    CHECK(fake_vl53l8_calibrations(r.fake, &table_len, &d4, &r16, &nb) == 2 && table_len == cx_table_len &&
          d4 == 3000 * 4 && r16 == 20 * 16 && nb == 1);
    fake_vl53l8_xtalk_cal_fails(r.fake, false);

    // the motion indicator: the plugin config at 8x8 for 400..1500 mm
    l8.set_resolution(depz::vl53l8::RESOLUTION_8X8);
    l8.configure_motion_indicator(400, 1500);
    CHECK(fake_vl53l8_dci(r.fake, 0xBFAC, b, sizeof b) == DEPZ_VL53L8_MOTION_CFG_SIZE && le32(b) == 13636 && b[19] == 9);
    CHECK(throws<ArgumentError>([&] { l8.configure_motion_indicator(400, 2000); }));  // span > 1500 mm
    l8.set_resolution(depz::vl53l8::RESOLUTION_4X4);

    // ranging: the stream owns the bank; the fake renders no frames
    std::atomic<int> seen{0};
    auto off = l8.on_frame([&](const depz::Vl53l8LiveFrame&) { seen++; });
    auto frames = l8.frames(2);
    l8.set_ranging_frequency_hz(15);
    l8.start_ranging();
    CHECK(l8.ranging() && fake_vl53l8_ranging(r.fake));
    CHECK(fake_vl53l8_dci(r.fake, 0xD968, b, sizeof b) == 8 && le32(b) == 540);  // the CX frame at 4x4
    writes = fake_vl53l8_write_count(r.fake);
    CHECK(throws<ArgumentError>([&] { l8.set_resolution(64); }));
    CHECK(throws<ArgumentError>([&] { l8.set_power_mode(DEPZ_VL53L8_POWER_MODE_SLEEP); }));
    CHECK(throws<ArgumentError>([&] { l8.caldata_xtalk(); }));
    CHECK(throws<ArgumentError>([&] { l8.configure_motion_indicator(); }));
    CHECK(throws<ArgumentError>([&] { l8.start_ranging(); }));
    CHECK(throws<ArgumentError>([&] { l8.init(); }));
    CHECK(fake_vl53l8_write_count(r.fake) == writes);
    CHECK(!frames.next(30ms));
    CHECK(throws<depz::TimeoutError>([&] { l8.get_frame(30ms); }));
    CHECK(l8.frame_parse_errors() == 0 && l8.reassembler_discards() == 0);
    // stop: STOP_STREAM refused, the ULD stop still runs, the flag clears
    fake_vl53l8_fail_next(r.fake, 0x36, 0x01);
    try {
        l8.stop_ranging();
        CHECK(false);
    } catch (const depz::StatusError& e) {
        CHECK(e.cmd() == 0x36 && e.status() == depz::Status::Error);  // the fake's 0x01
    }
    CHECK(!l8.ranging() && !fake_vl53l8_ranging(r.fake));
    writes = fake_vl53l8_write_count(r.fake);
    l8.stop_ranging();  // idempotent, nothing on the bus
    CHECK(fake_vl53l8_write_count(r.fake) == writes);
    off();
    CHECK(seen == 0);

    // power: sleep, wake (the firmware kept), deep sleep, wake (init again)
    l8.set_power_mode(DEPZ_VL53L8_POWER_MODE_SLEEP);
    CHECK(l8.power_mode() == DEPZ_VL53L8_POWER_MODE_SLEEP && fake_vl53l8_reg(r.fake, 0, 0x09) == 0x02);
    l8.set_power_mode(DEPZ_VL53L8_POWER_MODE_WAKEUP);
    CHECK(l8.power_mode() == DEPZ_VL53L8_POWER_MODE_WAKEUP && fake_vl53l8_boots(r.fake) == 1);
    l8.set_power_mode(DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP);
    CHECK(l8.power_mode() == DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP && fake_vl53l8_reg(r.fake, 0, 0x0F) == 0x43);
    l8.set_power_mode(DEPZ_VL53L8_POWER_MODE_WAKEUP);
    CHECK(l8.power_mode() == DEPZ_VL53L8_POWER_MODE_WAKEUP && fake_vl53l8_boots(r.fake) == 2);
    CHECK(throws<ArgumentError>([&] { l8.set_power_mode(7); }));
    // the CX firmware has no CNH; the L5/L7 board commands are not an L8's
    depz::CnhSetup cnh;
    cnh.init_config(10, 20, 2);
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
    CHECK(throws<depz::WrongTypeError>([&] { l8.configure_cnh(cnh); }));
    CHECK(!l8.cnh_setup());
    CHECK(throws<depz::WrongTypeError>([&] { l8.pin_ctrl(depz::vl53l7::PinAction::LpnOn); }));
    CHECK(throws<depz::WrongTypeError>([&] { l8.bridge_info(); }));
    CHECK(throws<depz::WrongTypeError>([&] { l8.set_i2c_speed_khz(400); }));
    fake_vl53l8_laser_safety_fault(r.fake, true);
    CHECK(throws<depz::ProtocolError>([&] { l8.start_ranging(); }));
    CHECK(!l8.ranging());
    fake_vl53l8_laser_safety_fault(r.fake, false);

    fake_vl53l8_set_reg(r.fake, 0, 0x09, 0x07);  // a power register value the ULD does not know
    CHECK(throws<depz::ProtocolError>([&] { l8.power_mode(); }));
}

// test_io.c l8ch_cnh_and_lmz_calibration_table: CNH armed on the CH firmware,
// read back by cnh_setup(), disarmed by init().
void vl53l8ch_fake_cnh()
{
    L8Rig r(depz::Vl53l8Model::L8CH, L8_CRC_CH);
    auto& l8 = *r.dev;
    l8.init();
    CHECK(l8.model() == depz::Vl53l8Model::L8CH);
    depz::CnhSetup cnh;
    cnh.init_config(10, 20, 2);
    CHECK(throws<depz::ArgumentError>([&] { l8.configure_cnh(cnh); }));  // no aggregate map yet
    cnh.create_agg_map(depz::vl53l8::RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
    l8.configure_cnh(cnh);
    const auto armed = l8.cnh_setup();
    CHECK(armed && armed->nb_of_aggregates == 16 && armed->feature_length == cnh.feature_length &&
          armed->map_id == cnh.map_id && armed->ref_bin_offset == cnh.ref_bin_offset);
    std::uint8_t b[160];
    CHECK(fake_vl53l8_dci(r.fake, 0xBFAC, b, sizeof b) == 156);
    CHECK(l8.calibrate_xtalk(50, 4, 600));
    CHECK(fake_vl53l8_calibrations(r.fake, nullptr, nullptr, nullptr, nullptr) == 1);
    l8.init();  // a fresh sensor: CNH disarmed
    CHECK(!l8.cnh_setup() && fake_vl53l8_boots(r.fake) == 2);
}

// vectors/vl53lx.json die_block vl53l1cx_uld_long_33ms_f2: 576 mm, 16072 kcps, 67 SPADs
const std::uint8_t VLX_L1_BLOCK[17] = {0x09, 0x09, 0x01, 0x43, 0x90, 0x07, 0xd9, 0x01, 0x3f,
                                       0x00, 0x0d, 0x22, 0x34, 0x02, 0x40, 0x07, 0xd9};
// die_block vl53l4cx_uld_as_l4cd_50ms_f2: 624 mm, 5160 kcps, 211 SPADs
const std::uint8_t VLX_L4_BLOCK[17] = {0x09, 0x09, 0x01, 0xd3, 0x20, 0x02, 0x85, 0x00, 0x89,
                                       0x00, 0x08, 0x2b, 0xa7, 0x02, 0x70, 0x02, 0x85};

// A fake APP_VL53L0_4 bridge and a Vl53lx on its link, init()-ed with the
// defaults (the product from the board name); the device goes first.
struct VlxRig {
    fake_vl53lx* fake = nullptr;
    std::unique_ptr<depz::Vl53lx> dev;
    explicit VlxRig(const char* board, bool init = true)
    {
        depz_link* link = nullptr;
        fake = fake_vl53lx_start(board, &link);
        if (!fake) throw std::runtime_error("fake_vl53lx_start");
        dev = depz::Vl53lx::open(depz::Link::adopt(link));
        dev->set_timeout(500ms);
        if (init) dev->init();
        fake_vl53lx_clear(fake);
    }
    ~VlxRig()
    {
        dev.reset();
        fake_vl53lx_stop(fake);
    }
};

// test_io.c vlx_l1_*: the L1X ULD's modes and budgets, thresholds, the
// offset / xtalk calibrations, ROI, the stream (decode + bad blocks) and its
// refusals, the bridge counters, raw registers.
void vl53lx_fake_l1cx_uld()
{
    using depz::ArgumentError;
    using depz::vl53lx::DriverKind;
    VlxRig r("DEPZ ToF Sensor VL53L1CX USB v2.1 SER");
    auto& v = *r.dev;
    CHECK(v.initialized() && v.product() == std::optional<std::string>("VL53L1CX"));
    CHECK(v.detected_product() == std::optional<std::string>("VL53L1CX"));
    CHECK(v.driver_kind() == std::optional<DriverKind>(DriverKind::Uld) && v.caveat().empty());
    CHECK(v.model_id() == 0xEBAA);
    CHECK(v.modes() == (std::vector<std::string>{"long", "short"}) && v.mode() == std::optional<std::string>("long"));
    CHECK(v.budget_range() == std::make_pair(15, 500) && v.driver_reach_mm() == 4563);
    v.configure(15, 0, std::string("short"));
    CHECK(v.range_timing() == std::make_pair(15, 15));
    const auto short_budgets = v.budget_choices();
    CHECK(short_budgets.size() == 7 && short_budgets[0] == 15 && v.driver_reach_mm() == 1388);
    v.set_mode("long");  // 15 ms exists in short mode only: the nearest long budget, 20 ms
    CHECK(v.mode() == std::optional<std::string>("long") && v.range_timing() == std::make_pair(20, 15));
    CHECK(v.budget_choices() == (std::vector<int>{20, 33, 50, 100, 200, 500}));
    CHECK(throws<ArgumentError>([&] { v.set_mode("medium"); }));
    CHECK(throws<ArgumentError>([&] { v.configure(40, 0); }));  // not a budget the long mode has

    fake_vl53lx_set_reg(r.fake, 0x0046, 0x90);
    v.set_detection_thresholds(100, 300, 3);  // the window bits rewritten, the others kept
    CHECK(fake_vl53lx_reg(r.fake, 0x0046) == 0x93 && v.detection_thresholds() == (std::array<int, 3>{100, 300, 3}));

    fake_vl53lx_set_block(r.fake, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
    CHECK(v.calibrate_offset(600, 5) == 24 && fake_vl53lx_word(r.fake, 0x001E) == 96 && v.offset_mm() == 24);
    CHECK(v.calibrate_xtalk(650, 5) == 27 && fake_vl53lx_word(r.fake, 0x0016) == 13982 && v.xtalk_kcps() == 27);
    fake_vl53lx_clear(r.fake);
    CHECK(throws<ArgumentError>([&] { v.calibrate_offset(600, 4); }));  // at least 5 samples
    CHECK(fake_vl53lx_rx_count(r.fake) == 0);

    v.set_roi(8, 6);
    CHECK(fake_vl53lx_reg(r.fake, 0x0080) == 0x57 && v.roi() == std::make_pair(8, 6));
    v.set_roi(16, 16);  // over 10: the centre moves to 199
    CHECK(v.roi_center() == 199);
    v.set_roi_center(0x88);
    CHECK(v.roi_center() == 0x88);

    v.start_ranging();
    CHECK(v.ranging());
    CHECK(throws<ArgumentError>([&] { v.set_offset_mm(5); }));  // the stream owns the bus
    CHECK(throws<ArgumentError>([&] { v.write_reg(0x0087, bytes_of({0})); }));
    std::atomic<int> seen{0};
    auto off = v.on_measurement([&](const depz::Vl53lxMeasurement& m) {
        if (m.distance_mm == 576) seen++;
    });
    auto s = v.measurements(4);
    fake_vl53lx_stream(r.fake, 1, 17, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
    auto m = s.next(1000ms);
    CHECK(m && m->timestamp_us == 1 && m->distance_mm == 576 && m->status == 0 && m->status_text == "valid");
    CHECK(m->sigma_mm == 3 && m->signal_kcps == 16072 && m->ambient_kcps == 2552 && m->spads == 67);
    CHECK(m->signal_per_spad_kcps == std::optional<double>(23) && m->stream_count == std::optional<std::int32_t>(1));
    CHECK(m->plottable() && m->primary_distance_mm() == std::optional<std::int32_t>(576) && m->targets.empty());
    fake_vl53lx_stream(r.fake, 2, 17, VLX_L1_BLOCK, 4);  // shorter than its length field
    fake_vl53lx_stream(r.fake, 3, 4, VLX_L1_BLOCK, 4);   // 4 B: no die block
    const auto info = v.bridge_info();                   // behind the reports
    CHECK(info.frames_dropped == 0 && v.stream_parse_errors() == 2 && seen == 1);
    off();
    CHECK(throws<depz::TimeoutError>([&] { v.get_measurement(30ms); }));
    const depz::bytes reg = v.read_reg(0x0087, 1);  // reads are fine
    CHECK(reg.size() == 1);
    v.stop_ranging();
    CHECK(!v.ranging() && fake_vl53lx_reg(r.fake, 0x0087) == 0x00);  // the L1X stop code
    v.write_reg(0x0087, bytes_of({0x00}));
    v.xshut(depz::vl53l4::XSHUT_OFF);  // drops the driver
    CHECK(!v.initialized() && throws<ArgumentError>([&] { v.model_id(); }));
}

// test_io.c vlx_l4_* / vlx_l3_refusals_name_the_caveat: configure() with its
// tuning, the tuning round trips, temperature update, measure_once, the
// groups a driver lacks refused by name with no traffic, the binding table.
void vl53lx_fake_l4cd_and_l3cx()
{
    using depz::ArgumentError;
    using depz::vl53lx::DriverKind;
    CHECK(depz::Vl53lx::driver_kinds("VL53L3CX") == (std::vector<DriverKind>{DriverKind::Ulp, DriverKind::Histogram}));
    CHECK(depz::Vl53lx::driver_kinds("VL53L0X") == (std::vector<DriverKind>{DriverKind::Uld}));
    CHECK(throws<ArgumentError>([&] { depz::Vl53lx::driver_kinds("VL53L9"); }));
    {
        VlxRig r("DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL");
        auto& v = *r.dev;
        CHECK(v.budget_range() == std::make_pair(10, 200) && v.driver_reach_mm() == 1388);
        v.configure(33, 0, std::nullopt, -10, 20, 1024);
        CHECK(v.offset_mm() == -10 && v.xtalk_kcps() == 20 && v.signal_threshold_kcps() == 1024);
        const auto t = v.range_timing();
        CHECK(t.second == 0 && std::abs(t.first - 33) <= 1 && !v.mode());
        CHECK(throws<ArgumentError>([&] { v.configure(300, 0); }));  // 10..200
        fake_vl53lx_clear(r.fake);
        CHECK(throws<ArgumentError>([&] { v.set_mode("long"); }));  // no modes on the L4CD
        CHECK(fake_vl53lx_rx_count(r.fake) == 0);

        v.set_offset_mm(10);
        CHECK(fake_vl53lx_word(r.fake, 0x001E) == 40 && v.offset_mm() == 10);
        v.set_xtalk_kcps(3);
        CHECK(fake_vl53lx_word(r.fake, 0x0016) == 1536 && v.xtalk_kcps() == 3);
        v.set_signal_threshold_kcps(2048);
        CHECK(fake_vl53lx_word(r.fake, 0x0066) == 256 && v.signal_threshold_kcps() == 2048);
        v.set_sigma_threshold_mm(15);
        CHECK(fake_vl53lx_word(r.fake, 0x0064) == 60 && v.sigma_threshold_mm() == 15);
        CHECK(throws<ArgumentError>([&] { v.set_sigma_threshold_mm(16384); }));
        fake_vl53lx_clear(r.fake);
        CHECK(!v.supports(depz::Vl53lxCap::Roi) && v.supports(depz::Vl53lxCap::SigmaThresh));
        CHECK(throws<ArgumentError>([&] { v.roi(); }));
        CHECK(throws<ArgumentError>([&] { v.set_roi(4, 4); }));
        CHECK(throws<ArgumentError>([&] { v.perform_ref_spad_management(); }));
        CHECK(fake_vl53lx_rx_count(r.fake) == 0);

        v.start_temperature_update();  // VHV re-run: 7 register writes
        CHECK(fake_vl53lx_count(r.fake, 0x33) == 7 && fake_vl53lx_reg(r.fake, 0x0087) == 0x80);

        fake_vl53lx_set_block(r.fake, VLX_L4_BLOCK, sizeof VLX_L4_BLOCK);
        const auto m = v.measure_once();
        CHECK(m.distance_mm == 624 && m.status == 0 && m.signal_kcps == 5160 && m.spads == 211);
        CHECK(m.timestamp_us >= 5000000 && m.primary_distance_mm() == std::optional<std::int32_t>(624));
        CHECK(v.calibrate_offset(600, 5) == -24 && v.offset_mm() == -24);
        CHECK(v.calibrate_xtalk(650, 5) == 1 && fake_vl53lx_word(r.fake, 0x0016) == 500);
        fake_vl53lx_set_reg(r.fake, 0x0031, 0x01);  // INT active low, never asserted
        CHECK(throws<depz::TimeoutError>([&] { v.measure_once(5ms); }));
    }
    {
        VlxRig r("DEPZ ToF Sensor VL53L3CX USB v2.1 SER");
        auto& v = *r.dev;
        CHECK(v.driver_kind() == std::optional<DriverKind>(DriverKind::Ulp));
        CHECK(v.caveat().find("single-target ranging only") != std::string::npos);
        CHECK(!v.supports(depz::Vl53lxCap::Offset));
        CHECK(throws<ArgumentError>([&] { v.configure(33, 0, std::nullopt, 5); }));
        CHECK(throws<ArgumentError>([&] { v.calibrate_xtalk(600); }));
        CHECK(throws<ArgumentError>([&] { v.start_temperature_update(); }));
        CHECK(fake_vl53lx_rx_count(r.fake) == 0);
        v.set_roi(4, 4);
        CHECK(fake_vl53lx_reg(r.fake, 0x0080) == 0x33 && v.roi() == std::make_pair(4, 4));
    }
    {
        // unstamped: no product until one is named; a pair without a row is refused
        VlxRig r("DEPZ ToF Sensor USB v2.1", false);
        auto& v = *r.dev;
        CHECK(!v.detected_product() && v.sensor_class() == "vl53lx" && !v.initialized());
        CHECK(throws<ArgumentError>([&] { v.init(); }));
        CHECK(throws<ArgumentError>([&] { v.init(DriverKind::Uld, std::string("VL53L4CX")); }));
        v.init(std::nullopt, std::string("VL53L4CD"));
        CHECK(v.product() == std::optional<std::string>("VL53L4CD") && v.initialized());
    }
}

// A fake BNO055 bridge + sensor model and a Bno055 on its link.
struct B055Rig {
    fake_bno055* fake = nullptr;
    std::unique_ptr<depz::Bno055> dev;
    B055Rig()
    {
        depz_link* link = nullptr;
        fake = fake_bno055_start(&link);
        if (!fake) throw std::runtime_error("fake_bno055_start");
        dev = depz::Bno055::open(depz::Link::adopt(link));
        dev->set_timeout(300ms);
    }
    ~B055Rig()
    {
        dev.reset();
        fake_bno055_stop(fake);
    }
};

// test_io.c b055_identity_and_raw_registers / b055_operation_and_power_modes /
// b055_units_axes_temperature_source / b055_status_self_test_calibration /
// b055_page1_configs_and_interrupts / b055_configure_restore_and_reset.
void bno055_fake_registers_modes_and_config()
{
    using depz::ArgumentError;
    using depz::bno055::OprMode;
    B055Rig r;
    auto& b = *r.dev;
    const auto opr = [&] { return fake_bno055_opr_mode(r.fake); };
    CHECK(throws<ArgumentError>([&] { b.restore_configuration(); }));  // configure() first
    const auto info = b.bridge_info();
    CHECK(info.chip_id == 0xA0 && info.sw_rev == 0x0311 && info.initialized == 1 && b.is_alive());

    // raw registers: page 0, page 1 wrapped in PAGE_ID writes, the refusals
    const depz::bytes id = b.read_registers(0x00, 4);
    CHECK(id == bytes_of({0xA0, 0xFB, 0x32, 0x0F}));
    b.write_registers(0x55, bytes_of({1, 2}));
    CHECK(fake_bno055_reg(r.fake, 0, 0x55) == 1 && fake_bno055_reg(r.fake, 0, 0x56) == 2);
    fake_bno055_set_reg(r.fake, 1, 0x20, 0x5A);
    CHECK(b.read_registers(0x20, 1, 1) == bytes_of({0x5A}) && fake_bno055_page(r.fake) == 0);
    b.write_registers(0x21, bytes_of({0x6B}), 1);
    CHECK(fake_bno055_reg(r.fake, 1, 0x21) == 0x6B && fake_bno055_page(r.fake) == 0);
    CHECK(throws<ArgumentError>([&] { b.read_registers(0x00, 1, 2); }));  // pages 0 and 1
    fake_bno055_short_read(r.fake, 1);
    CHECK(throws<depz::ProtocolError>([&] { b.read_registers(0x00, 4); }));
    fake_bno055_fail_next(r.fake, 0x32, 0x09);
    CHECK(throws<depz::StatusError>([&] { b.read_registers(0x00, 1); }));
    fake_bno055_silent(r.fake, 0x32, true);
    CHECK(throws<depz::TimeoutError>([&] { b.read_registers(0x00, 1); }));
    fake_bno055_silent(r.fake, 0x32, false);
    CHECK(b.read_registers(0x00, 1) == bytes_of({0xA0}));  // not wedged

    // operating and power modes
    fake_bno055_set_reg(r.fake, 0, 0x3D, 0x1C);
    CHECK(b.operation_mode() == OprMode::Ndof);  // bits 7:4 masked
    fake_bno055_set_reg(r.fake, 0, 0x3D, 0x00);
    b.set_operation_mode(OprMode::Ndof);
    CHECK(opr() == DEPZ_BNO055_MODE_NDOF);
    b.set_operation_mode(OprMode::Amg);  // through CONFIG
    CHECK(opr() == DEPZ_BNO055_MODE_AMG && b.operation_mode() == OprMode::Amg);
    CHECK(throws<ArgumentError>([&] { b.set_operation_mode(static_cast<OprMode>(0x0D)); }));
    b.set_power_mode(DEPZ_BNO055_POWER_SUSPEND);
    CHECK(b.power_mode() == DEPZ_BNO055_POWER_SUSPEND && opr() == DEPZ_BNO055_MODE_CONFIG);
    b.set_power_mode(DEPZ_BNO055_POWER_LOW);  // wakes into the mode it left
    CHECK(b.power_mode() == DEPZ_BNO055_POWER_LOW && opr() == DEPZ_BNO055_MODE_AMG);

    // units, axes, temperature source (each through CONFIG and back)
    depz::bno055::Units android;
    android.android = true;
    CHECK(b.units() == android);  // power-on 0x80
    const depz::bno055::Units all{true, true, true, true, true};
    fake_bno055_set_mode(r.fake, DEPZ_BNO055_MODE_NDOF);
    b.set_units(all);
    CHECK(b.units() == all && fake_bno055_reg(r.fake, 0, 0x3B) == 0x97 && opr() == DEPZ_BNO055_MODE_NDOF);
    b.set_axis_placement("p0");
    CHECK(fake_bno055_reg(r.fake, 0, 0x41) == 0x21 && fake_bno055_reg(r.fake, 0, 0x42) == 0x04);
    CHECK(b.axis_remap() == *depz::bno055::placement("P0"));
    CHECK(throws<ArgumentError>([&] { b.set_axis_placement("P9"); }));
    depz::bno055::AxisRemap bad;
    bad.x = bad.y = bad.z = depz::bno055::AXIS_Y;  // not a permutation: refused on the host
    CHECK(throws<ArgumentError>([&] { b.set_axis_remap(bad); }));
    b.set_axis_remap(*depz::bno055::placement("P2"));
    CHECK(b.axis_remap() == *depz::bno055::placement("P2") && fake_bno055_reg(r.fake, 0, 0x42) == 0x06);
    b.set_temperature_source(1);  // gyro
    CHECK(b.temperature_source() == 1 && fake_bno055_reg(r.fake, 0, 0x40) == 0x01);

    // status, self-test, calibration
    fake_bno055_set_reg(r.fake, 0, 0x38, 1);
    auto st = b.system_status();
    CHECK(st.self_test == 0x0F && st.clk_status == 1 && st.status == 5 && st.error == 0);
    fake_bno055_set_reg(r.fake, 0, 0x3F, 0x80);
    st = b.self_test();  // from NDOF: keeps CLK_SEL, back to NDOF
    CHECK(st.self_test == 0x0F && st.error == 0 && opr() == DEPZ_BNO055_MODE_NDOF &&
          fake_bno055_reg(r.fake, 0, 0x3F) == 0x80);
    fake_bno055_set_reg(r.fake, 0, 0x35, 0xC4);
    const auto cs = b.calibration_status();
    CHECK(cs.system == 3 && cs.gyro == 0 && cs.accel == 1 && cs.mag == 0 && !cs.fully_calibrated());
    depz::bno055::CalibrationProfile prof;
    prof.accel_offset = {-22, 13, -31};
    prof.mag_offset = {-187, 402, -96};
    prof.gyro_offset = {-1, -2, 0};
    prof.accel_radius = 1000;
    prof.mag_radius = 623;
    b.write_calibration_profile(prof);
    CHECK(b.calibration_profile().pack() == prof.pack() && opr() == DEPZ_BNO055_MODE_NDOF);
    for (std::size_t k = 0; k < depz::bno055::CALIB_PROFILE_LEN; k++)
        CHECK(std::byte{fake_bno055_reg(r.fake, 0, static_cast<std::uint8_t>(0x55 + k))} == prof.pack()[k]);
    const std::array<std::int16_t, 9> identity = {16384, 0, 0, 0, 16384, 0, 0, 0, 16384};
    b.set_sic_matrix(identity);
    CHECK(b.sic_matrix() == identity && fake_bno055_reg(r.fake, 0, 0x44) == 0x40);

    // page-1 configs, unique id, interrupts
    auto ac = b.accel_config();
    CHECK(ac.range == 1 && ac.bandwidth == 3 && ac.power == 0);
    ac.range = 3;
    ac.bandwidth = 7;
    b.set_accel_config(ac);
    CHECK(fake_bno055_reg(r.fake, 1, 0x08) == 0x1F && fake_bno055_page(r.fake) == 0 && opr() == DEPZ_BNO055_MODE_NDOF);
    auto gc = b.gyro_config();
    auto mc = b.mag_config();
    CHECK(gc.range == 0 && gc.bandwidth == 7 && gc.power == 0 && mc.rate == 5 && mc.mode == 1 && mc.power == 3);
    gc.range = 2;
    gc.bandwidth = 3;
    gc.power = 1;
    mc.rate = 7;
    mc.mode = 3;
    mc.power = 0;
    b.set_gyro_config(gc);
    b.set_mag_config(mc);
    CHECK(fake_bno055_reg(r.fake, 1, 0x0A) == 0x1A && fake_bno055_reg(r.fake, 1, 0x0B) == 0x01 &&
          fake_bno055_reg(r.fake, 1, 0x09) == 0x1F);
    gc = b.gyro_config();
    mc = b.mag_config();
    CHECK(gc.range == 2 && gc.bandwidth == 3 && gc.power == 1 && mc.rate == 7 && mc.mode == 3 && mc.power == 0);
    const depz::bytes uid = b.unique_id();
    CHECK(uid.size() == 16 && uid[0] == std::byte{0xC0} && uid[15] == std::byte{0xCF});
    b.set_interrupt_enable(0x40);
    b.set_interrupt_mask(0xC0);
    CHECK(b.interrupt_enable() == 0x40 && b.interrupt_mask() == 0xC0);
    CHECK(throws<ArgumentError>([&] { b.set_interrupt_setting(0x10, 1); }));  // page 1 0x11..0x1F only
    b.set_interrupt_setting(0x11, 0x0A);
    CHECK(fake_bno055_reg(r.fake, 1, 0x11) == 0x0A);
    fake_bno055_set_reg(r.fake, 0, 0x37, 0x40);
    CHECK(b.read_interrupt_status() == 0x40 && b.read_interrupt_status() == 0);  // clears on read
    b.clear_interrupt();
    CHECK(fake_bno055_reg(r.fake, 0, 0x3F) == 0x80);  // CLK_SEL kept

    // configure, the stream refusing page 1, reset, restore
    fake_bno055_sys_status(r.fake, 3, 2);  // still booting for two polls
    b.configure(OprMode::Ndof, all, depz::bno055::placement("P2"), prof);
    CHECK(opr() == DEPZ_BNO055_MODE_NDOF && fake_bno055_reg(r.fake, 0, 0x3B) == 0x97 &&
          fake_bno055_reg(r.fake, 0, 0x41) == 0x24 && fake_bno055_reg(r.fake, 0, 0x42) == 0x06);
    CHECK(fake_bno055_sys_status_left(r.fake) == 0);
    b.start_stream();
    CHECK(b.streaming() && fake_bno055_streaming(r.fake));
    CHECK(throws<ArgumentError>([&] { b.self_test(); }));  // stop the stream first
    CHECK(throws<ArgumentError>([&] { b.unique_id(); }));
    fake_bno055_sys_status(r.fake, 2, 2);
    b.reset_sensor();
    CHECK(!b.streaming() && !fake_bno055_streaming(r.fake) && fake_bno055_reset_count(r.fake) == 1);
    CHECK(fake_bno055_reg(r.fake, 0, 0x3B) == 0x80);  // the power-on registers
    b.restore_configuration();
    CHECK(fake_bno055_reg(r.fake, 0, 0x3B) == 0x97 && opr() == DEPZ_BNO055_MODE_NDOF);
    fake_bno055_fail_next(r.fake, 0x34, 0x09);
    CHECK(throws<depz::StatusError>([&] { b.reset_sensor(); }));
    CHECK(throws<ArgumentError>([&] { b.configure(static_cast<OprMode>(0x0D)); }));
}

// vectors/bno055.json blocks full_block_live.
const char* const B055_FULL_LIVE =
    "a2037fff73ff17ff4e01d100fffffcff00009f05e50464f7ba2952f9efcf0000e2ff03000400c0037bff6eff1a00";

bool near3(const std::optional<std::array<double, 3>>& v, double a, double b, double c)
{
    return v && std::fabs((*v)[0] - a) < 1e-9 && std::fabs((*v)[1] - b) < 1e-9 && std::fabs((*v)[2] - c) < 1e-9;
}

// test_io.c b055_polling_scales_by_the_units / b055_stream_wire_units_and_bad_reports.
void bno055_fake_samples_and_stream()
{
    using depz::ArgumentError;
    {
        B055Rig r;
        auto& b = *r.dev;
        const depz::bytes block = tmf_unhex(B055_FULL_LIVE);
        for (std::size_t k = 0; k < block.size(); k++)
            fake_bno055_set_reg(r.fake, 0, static_cast<std::uint8_t>(0x08 + k), std::to_integer<std::uint8_t>(block[k]));
        auto s = b.read_sample();
        CHECK(s.units.android && s.addr == 0x08 && s.raw.size() == 46 && s.timestamp_us > 5000000);
        CHECK(near3(s.accel, 9.30, -1.29, -1.41) && near3(s.mag, -233 / 16.0, 334 / 16.0, 209 / 16.0));
        CHECK(near3(s.gyro, -1 / 16.0, -4 / 16.0, 0) && near3(s.euler, 1439 / 16.0, 1253 / 16.0, -2204 / 16.0));
        CHECK(s.quaternion && std::fabs((*s.quaternion)[0] - 10682 / 16384.0) < 1e-12 &&
              std::fabs((*s.quaternion)[1] + 1710 / 16384.0) < 1e-12);
        CHECK(near3(s.linear_accel, -0.30, 0.03, 0.04) && near3(s.gravity, 9.60, -1.33, -1.46));
        CHECK(s.temperature == std::optional<double>(26) && s.calibration && s.calibration->system == 0);
        fake_bno055_set_reg(r.fake, 0, 0x34, 0xE7);  // -25 C
        fake_bno055_set_reg(r.fake, 0, 0x35, 0xFF);
        s = b.read_sample(0x34, 2);  // a window leaves the channels it does not cover out
        CHECK(s.temperature == std::optional<double>(-25) && s.calibration && s.calibration->fully_calibrated());
        CHECK(!s.accel && !s.mag && !s.gyro && !s.euler && !s.quaternion && !s.linear_accel && !s.gravity);
        CHECK(throws<ArgumentError>([&] { b.read_sample(0xF9, 8); }));
        b.set_units(depz::bno055::Units{true, true, true, true, true});
        for (std::size_t k = 0; k < block.size(); k++)
            fake_bno055_set_reg(r.fake, 0, static_cast<std::uint8_t>(0x08 + k), std::to_integer<std::uint8_t>(block[k]));
        s = b.read_sample();
        CHECK(s.units.accel_mg && near3(s.accel, 930, -129, -141) && near3(s.gyro, -1 / 900.0, -4 / 900.0, 0));
        CHECK(s.temperature == std::optional<double>(52) && near3(s.linear_accel, -0.30, 0.03, 0.04));
        fake_bno055_set_mode(r.fake, DEPZ_BNO055_MODE_NDOF);
        const auto q = b.read_quaternion();  // the 8-byte block only
        CHECK(std::fabs(q[0] - 10685 / 16384.0) < 1e-12 && std::fabs(q[2] + 12305 / 16384.0) < 1e-12 && q[3] == 0);
    }
    {
        B055Rig r;
        auto& b = *r.dev;
        const depz::bytes accel = bytes_of({0xa2, 0x03, 0x7f, 0xff, 0x73, 0xff});
        const auto* a = reinterpret_cast<const std::uint8_t*>(accel.data());
        CHECK(!b.streaming());
        CHECK(throws<ArgumentError>([&] { b.start_stream(0); }));  // TIMER period 0
        CHECK(throws<ArgumentError>([&] { b.start_stream(10, 0x08, 129); }));
        CHECK(fake_bno055_sent(r.fake, 0x35) == 0 && !b.streaming());
        fake_bno055_set_reg(r.fake, 0, 0x3B, 0x97);
        b.start_stream(10, 0x08, 46, depz::bno055::TRIGGER_TIMER);
        CHECK(b.streaming() && fake_bno055_streaming(r.fake));
        std::atomic<int> got{0};
        auto off = b.on_sample([&](const depz::Bno055Sample&) { got++; });
        auto it = b.samples(4);
        fake_bno055_push_stream(r.fake, 1, 0x08, 6, a, 6);
        auto s = it.next(1000ms);  // scaled by the units latched before START_STREAM
        CHECK(s && s->timestamp_us == 1 && s->units.accel_mg && near3(s->accel, 930, -129, -141) && !s->gyro);
        fake_bno055_push_stream(r.fake, 2, 0x08, 46, a, 6);  // shorter than its length field
        fake_bno055_push_stream(r.fake, 3, 0x08, 6, a, 0);
        (void)b.bridge_info();  // behind the reports
        CHECK(b.stream_parse_errors() == 2 && got == 1);
        CHECK(!it.next(20ms));
        off();
        fake_bno055_push_stream(r.fake, 4, 0x08, 6, a, 6);
        s = it.next(1000ms);
        CHECK(s && s->timestamp_us == 4 && got == 1);  // unsubscribed
        CHECK(throws<depz::TimeoutError>([&] { b.get_sample(20ms); }));
        b.stop_stream();
        CHECK(!b.streaming() && !fake_bno055_streaming(r.fake));
        b.start_stream();
        fake_bno055_fail_next(r.fake, 0x36, 0x01);
        CHECK(throws<depz::StatusError>([&] { b.stop_stream(); }));
        CHECK(!b.streaming());  // a failure still clears the flag
    }
}

// test_io.c bno086_commands: the SH-2 commands' parameters and answers on the fake hub.
void bno086_fake_commands()
{
    using depz::bno086::SensorId;
    depz_link* link = nullptr;
    fake_bno086* fk = fake_bno086_start(&link);
    CHECK(fk);
    {
        auto b = depz::Bno086::open(depz::Link::adopt(link));
        b->hardware_reset();
        auto c = b->calibration();
        CHECK(c.accel && !c.gyro && c.mag && !c.planar);
        b->set_calibration(true, true, false, true);
        c = b->calibration();
        CHECK(c.accel && c.gyro && !c.mag && c.planar);
        b->tare_now(depz::bno086::TARE_Z, depz::bno086::TareBasis::GameRotationVector);
        CHECK(b->oscillator_type() == DEPZ_BNO_OSC_EXT_CRYSTAL);  // the tare went first
        std::uint8_t p[9];
        CHECK(fake_bno086_last_command(fk, DEPZ_SH2_CMD_TARE, p) && p[0] == 0 && p[1] == 4 && p[2] == 1);
        b->set_reorientation(0, 0, -0.5, 1.0);
        b->save_dcd();
        CHECK(fake_bno086_last_command(fk, DEPZ_SH2_CMD_TARE, p) && p[0] == 2 && p[1] == 0 && p[3] == 0 &&
              p[5] == 0x00 && p[6] == 0xE0 && p[7] == 0x00 && p[8] == 0x40);
        CHECK(throws<depz::ArgumentError>([&] { b->set_reorientation(2.0, 0, 0, 0); }));
        b->persist_tare();
        b->configure_periodic_dcd(false);
        const auto errs = b->errors(0);
        CHECK(errs.size() == 1 && errs[0].severity == 2 && errs[0].source == 3 && errs[0].code == 0x33);
        CHECK(fake_bno086_last_command(fk, DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG, p) && p[0] == 1);
        CHECK(fake_bno086_last_command(fk, DEPZ_SH2_CMD_TARE, p) && p[0] == 1);
        const auto n = b->counts(SensorId::RotationVector);
        CHECK(n.sensor_id == 5 && n.offered == 105 && n.accepted == 95 && n.on == 85 && n.attempted == 75);
        b->clear_counts(SensorId::RotationVector);
        const auto rsp = b->command(DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE, {}, true);
        CHECK(rsp && rsp->command == DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE && rsp->r[0] == 1);
        const depz::bytes ten(10);
        CHECK(throws<depz::ArgumentError>([&] { b->command(0x55, ten, false); }));  // 9 parameters at most
        b->wake();
        b->clear_dcd_and_reset();
        CHECK(b->oscillator_type() == DEPZ_BNO_OSC_EXT_CRYSTAL);
        CHECK(throws<depz::TimeoutError>([&] { b->get_report(50ms); }));
        CHECK(b->shtp_discarded() == 0);
    }
    fake_bno086_stop(fk);
}

// test_io.c l4_tuning_roundtrips / l4_stream_decodes_and_counts_short_blocks /
// l4_config_refused_while_ranging / l4_is_alive_checks_model_id: the
// thresholds, the stream with a short block counted, raw registers, the
// refusals while ranging, xshut.
void l4_fake_tuning_stream_and_registers()
{
    using depz::ArgumentError;
    depz_link* host = nullptr;
    fake_vl53l4* f = fake_vl53l4_start(&host);
    CHECK(f);
    {
        auto dev = depz::Vl53l4cd::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
        dev->set_xtalk_kcps(20);
        CHECK(dev->xtalk_kcps() == 20);
        dev->set_signal_threshold_kcps(1024);
        CHECK(dev->signal_threshold_kcps() == 1024);
        dev->set_sigma_threshold_mm(15);
        CHECK(dev->sigma_threshold_mm() == 15);

        // distance 0x0123 = 291 mm, stream count 7, raw status 9 -> 0 (valid)
        static const std::uint8_t block[17] = {0x09, 0x00, 0x07, 0x0f, 0x00, 0x01, 0xf4, 0x00, 0x64,
                                               0x00, 0x28, 0x00, 0x00, 0x01, 0x23, 0x00, 0xff};
        std::atomic<int> seen{0};
        auto off = dev->on_measurement([&](const depz::Vl53l4cdMeasurement& m) {
            if (m.r.distance_mm == 291) seen++;
        });
        auto s = dev->measurements(8);
        fake_vl53l4_send_stream(f, 1234, block, sizeof block);
        auto m = s.next(1000ms);
        CHECK(m && m->timestamp_us == 1234 && m->r.distance_mm == 291 && m->r.range_status == 0 &&
              m->r.stream_count == 7 && m->valid() && m->status_text() == "valid");
        fake_vl53l4_send_stream(f, 1, block, 2);
        (void)dev->range_timing();  // a round trip behind the stream report
        CHECK(dev->stream_parse_errors() == 1 && seen == 1);
        off();
        CHECK(throws<depz::TimeoutError>([&] { dev->get_measurement(30ms); }));

        const depz::bytes id = dev->read_reg(0x010F, 2);
        CHECK(id == bytes_of({0xEB, 0xAA}));  // the model id the fake's die answers
        dev->write_reg(0x0024, bytes_of({0x07}));
        CHECK(fake_vl53l4_reg(f, 0x0024) == 0x07);

        dev->init();
        dev->start_ranging();
        CHECK(dev->ranging() && fake_vl53l4_streaming(f));
        CHECK(throws<ArgumentError>([&] { dev->set_xtalk_kcps(10); }));
        CHECK(throws<ArgumentError>([&] { dev->set_signal_threshold_kcps(8); }));
        CHECK(throws<ArgumentError>([&] { dev->set_sigma_threshold_mm(8); }));
        dev->stop_ranging();
        CHECK(!dev->ranging() && !fake_vl53l4_streaming(f) && fake_vl53l4_reg(f, 0x0087) == 0x80);
        dev->xshut(depz::vl53l4::XSHUT_RESET);
        CHECK(!dev->initialized());
        fake_vl53l4_set_reg(f, 0x010F, 0x12);
        CHECK(!dev->is_alive());
    }
    fake_vl53l4_stop(f);
}

// VL53L4CD_calibration.c through the wrapper (test_vl53l4cd_device.py): the
// result block seeded in the fake's registers (raw status 9 = valid, 2048 raw
// SPADs = 8), offset = target - mean, xtalk = (1 - mean/target) * signal/SPADs;
// a bad argument is an ArgumentError, no valid sample a ProtocolError; a
// callback that throws is counted and the stream goes on.
void l4_fake_calibrations_and_a_throwing_callback()
{
    depz_link* host = nullptr;
    fake_vl53l4* f = fake_vl53l4_start(&host);
    CHECK(f);
    const auto seed = [f](std::uint16_t mm, std::uint8_t status) {
        std::uint8_t raw[17] = {};
        raw[0] = status;
        raw[2] = 1;
        raw[3] = 0x08;  // spads 2048
        raw[6] = 150;   // signal 1200 kcps / 8
        raw[10] = 12;   // sigma 3 mm x 4
        raw[13] = static_cast<std::uint8_t>(mm >> 8);
        raw[14] = static_cast<std::uint8_t>(mm & 0xFF);
        for (std::uint16_t i = 0; i < 17; i++) fake_vl53l4_set_reg(f, static_cast<std::uint16_t>(0x0089 + i), raw[i]);
    };
    {
        auto dev = depz::Vl53l4cd::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
        dev->init();
        seed(120, 9);
        CHECK(dev->calibrate_offset(150, 5) == 30);
        CHECK(dev->offset_mm() == 30);
        CHECK(dev->calibrate_xtalk(240, 5) == 75);
        CHECK(dev->xtalk_kcps() == 75);
        CHECK(throws<depz::ArgumentError>([&] { dev->calibrate_offset(9, 20); }));
        CHECK(throws<depz::ArgumentError>([&] { dev->calibrate_xtalk(5001, 20); }));
        seed(100, 4);  // "signal below threshold" in every frame
        CHECK(throws<depz::ProtocolError>([&] { dev->calibrate_xtalk(200, 5); }));
        dev->start_temperature_update();
        CHECK(fake_vl53l4_reg(f, 0x0008) == 0x09 && fake_vl53l4_reg(f, 0x000B) == 0x00);
        CHECK(fake_vl53l4_reg(f, 0x0087) == 0x80 && !dev->ranging());

        static const std::uint8_t block[17] = {0x09, 0x00, 0x07, 0x0f, 0x00, 0x01, 0xf4, 0x00, 0x64,
                                               0x00, 0x28, 0x00, 0x00, 0x01, 0x23, 0x00, 0xff};
        auto boom = dev->on_measurement([](const depz::Vl53l4cdMeasurement&) { throw std::runtime_error("bug"); });
        auto s = dev->measurements(4);
        fake_vl53l4_send_stream(f, 1234, block, sizeof block);
        auto m = s.next(1000ms);
        CHECK(m && m->r.distance_mm == 291);
        CHECK(dev->stats().callback_errors == 1);
        dev->reset_sensor();
        CHECK(!dev->initialized());
    }
    fake_vl53l4_stop(f);
}

// The TMF8829 reader over a bare loopback: a one-chunk frame completes and
// reaches the frame callback (its 4 bytes are no results frame: a parse
// error, no measurement), an event reaches the event callback; both
// callbacks throw, are counted, and the reader goes on.
void tmf8829_throwing_callbacks_are_counted()
{
    auto [host, side] = depz::Link::loopback_pair();
    auto dev = depz::Tmf8829::open(std::move(host));
    std::atomic<int> frames{0}, events{0};
    auto f_off = dev->on_frame([&](const depz::tmf8829::RawFrame& r) {
        if (r.timestamp_us == 77 && r.edge_us == 78 && r.frame_seq == 3 && r.data == bytes_of({1, 2, 3, 4})) frames++;
        throw std::runtime_error("frame consumer bug");
    });
    auto e_off = dev->on_event([&](const depz::tmf8829::Tmf8829Event& e) {
        if (e.timestamp_us == 0x0102 && e.int_status == 2 && e.code == 0 && e.frame_id == 5 && e.fifo_status == 6) events++;
        throw 7;
    });
    // ts 77, full_size 4, offset 0, frame_seq 3, edge 78, then the 4 bytes
    side.write(depz::build_packet(0x93, bytes_of({77, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 3, 78, 0, 0, 0, 0, 0, 0, 0,
                                                  1, 2, 3, 4}), 0));
    side.write(depz::build_packet(0x94, bytes_of({0x02, 0x01, 0, 0, 0, 0, 0, 0, 2, 0, 5, 6}), 1));
    CHECK(wait_for([&] { return dev->stats().callback_errors == 2; }));
    CHECK(frames == 1 && events == 1);
    const depz::Tmf8829StreamStats st = dev->stream_stats();
    CHECK(st.frames_completed == 1 && st.measurements == 0 && st.frame_parse_errors == 1 && st.chunk_errors == 0);
}

// Contract 07 §3: progress callbacks run on the caller's thread unguarded —
// an exception thrown there leaves init() (TMF8829 and VL53L8 alike), is not
// counted as a reader callback error, and the callback is not called again.
void a_throwing_progress_callback_propagates_out_of_init()
{
    {
        TmfRig rig;
        depz_link* link = nullptr;
        rig.fake = fake_tmf8829_start(&link);
        CHECK(rig.fake);
        auto t = depz::Tmf8829::open(depz::Link::adopt(link));
        t->set_timeout(1000ms);
        int calls = 0;
        bool thrown = false;
        try {
            t->init([&](std::size_t, std::size_t) {
                if (++calls == 3) throw std::runtime_error("user cancels the download");
            });
        } catch (const std::runtime_error& e) {
            thrown = std::string(e.what()) == "user cancels the download";
        }
        CHECK(thrown && calls == 3);
        CHECK(t->stats().callback_errors == 0);
        CHECK(t->init().serial == FAKE_TMF8829_SERIAL);  // the device stays usable
        rig.dev = std::move(t);
    }
    {
        L8Rig r(depz::Vl53l8Model::L8CX, L8_CRC_CX);
        int calls = 0;
        CHECK(throws<int>([&] { r.dev->init([&](const std::string&, std::size_t, std::size_t) { calls++; throw 7; }); }));
        CHECK(calls == 1 && r.dev->stats().callback_errors == 0);
        r.dev->init();
        CHECK(r.dev->initialized());
    }
}

// attach_sensor_class() refusing a class leaves the caller's device open and
// usable (the C function and Python do not close it either).
void attach_sensor_class_refusal_keeps_the_device()
{
    depz_link* host = nullptr;
    fake_sr04* f = fake_sr04_start(&host);
    {
        auto dev = depz::Device::open(depz::Link::adopt(host));
        dev->set_timeout(1000ms);
        CHECK(throws<depz::ArgumentError>([&] { depz::attach_sensor_class(std::move(dev), "no_such_class"); }));
        CHECK(dev && dev->software_name() == FAKE_SR04_SOFTWARE);
        CHECK(throws<depz::BootloaderModeError>([&] { depz::attach_sensor_class(std::move(dev), "bootloader"); }));
        CHECK(dev && dev->software_name() == FAKE_SR04_SOFTWARE);
        auto sr = depz::Device::promote(std::move(dev));
        CHECK(throws<depz::ArgumentError>([&] { depz::attach_sensor_class(std::move(sr), "sr04"); }));  // has a class
        CHECK(sr && sr->device_name() == FAKE_SR04_NAME);
    }
    fake_sr04_stop(f);
}

// The dataset layer's own JSON and gzip readers past what the vectors use:
// every string escape, \u in either case, a surrogate pair, a too-big integer
// read as a real, NaN / infinity written as null (as Python's dataset writer
// does), refused syntax; a gzip member with FEXTRA and FNAME, and the damaged
// headers / an invalid deflate block refused.
void dataset_json_and_gzip_edges()
{
    using depz::dataset::Value;
    const Value s = Value::parse(R"("q\"b\\s\/b\bf\fn\nr\rt\t é€😀")");
    CHECK(s.is_string() && s.as_string() == "q\"b\\s/b\bf\fn\nr\rt\t \xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    const Value big = Value::parse("123456789012345678901");
    CHECK(big.is_double() && big.as_double() == 123456789012345678901.0);
    CHECK(Value(std::uint64_t{7}).as_int() == 7 && Value(std::uint64_t{7}).is_int());
    CHECK(Value(2.75).as_int() == 2 && Value("x").as_int() == 0 && Value("x").as_double() == 0.0);
    CHECK(Value(std::numeric_limits<double>::quiet_NaN()).to_json() == "null");
    CHECK(Value(-std::numeric_limits<double>::infinity()).to_json() == "null");
    CHECK(throws<std::runtime_error>([] { Value::parse(R"("\x")"); }));
    CHECK(throws<std::runtime_error>([] { Value::parse(R"("\u12G4")"); }));
    CHECK(throws<std::runtime_error>([] { Value::parse("[1 2]"); }));
    CHECK(throws<std::runtime_error>([] { Value::parse(R"({"a": 1 "b": 2})"); }));

    // gzip of "hi there" with FEXTRA "XY" and FNAME "n.txt" (Python's gzip reads it)
    const depz::bytes member = bytes_of({0x1f, 0x8b, 0x08, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x58,
                                         0x59, 0x6e, 0x2e, 0x74, 0x78, 0x74, 0x00, 0xcb, 0xc8, 0x54, 0x28, 0xc9, 0x48,
                                         0x2d, 0x4a, 0x05, 0x00, 0xec, 0x76, 0xa3, 0xe3, 0x08, 0x00, 0x00, 0x00});
    CHECK(depz::dataset::is_gzip(member) && !depz::dataset::is_gzip(bytes_of({0x1f})));
    const depz::bytes text = depz::dataset::gunzip(member);
    CHECK(std::string(reinterpret_cast<const char*>(text.data()), text.size()) == "hi there");
    const auto refused = [](const depz::bytes& b, const char* what) {
        try {
            depz::dataset::gunzip(b);
        } catch (const std::runtime_error& e) {
            return std::strstr(e.what(), what) != nullptr;
        }
        return false;
    };
    CHECK(refused(bytes_of({0x1f, 0x8b, 0x07, 0, 0, 0, 0, 0, 0, 3}), "not a deflate member"));
    CHECK(refused(bytes_of({0x1f, 0x8b, 0x08, 0x04, 0, 0, 0, 0, 0, 3, 0x02}), "truncated extra field"));
    CHECK(refused(bytes_of({0x1f, 0x8b, 0x08, 0x08, 0, 0, 0, 0, 0, 3, 'n', '.'}), "truncated name"));
    CHECK(refused(bytes_of({0x1f, 0x8b, 0x08, 0x00, 0, 0, 0, 0, 0, 3, 0x07}), "invalid block type"));

    CHECK(std::strcmp(depz::dataset::to_string(depz::dataset::PlayerState::Idle), "idle") == 0);
    CHECK(std::strcmp(depz::dataset::to_string(depz::dataset::PlayerState::Playing), "playing") == 0);
    CHECK(std::strcmp(depz::dataset::to_string(depz::dataset::PlayerState::Paused), "paused") == 0);
    CHECK(std::strcmp(depz::dataset::to_string(depz::dataset::PlayerState::Done), "done") == 0);

    // framing: a payload past MAX_PAYLOAD does not fit a packet
    const depz::bytes huge(depz::MAX_PAYLOAD + 1u);
    CHECK(throws<std::length_error>([&] { (void)depz::build_packet(0x40, huge); }));
}

// Raw register access through the wrapper: the page register selects the
// bank, a write lands in the fake's register file and reads back; a bridge
// status on READ_REG is a typed error.
void vl53l8_fake_raw_registers()
{
    L8Rig r(depz::Vl53l8Model::L8CX, L8_CRC_CX);
    auto& l8 = *r.dev;
    l8.write_reg(0x7FFF, bytes_of({0x01}));
    CHECK(fake_vl53l8_page(r.fake) == 1);
    l8.write_reg(0x0123, bytes_of({0x5A, 0xA5}));
    CHECK(fake_vl53l8_reg(r.fake, 1, 0x0123) == 0x5A && fake_vl53l8_reg(r.fake, 1, 0x0124) == 0xA5);
    CHECK(l8.read_reg(0x0123, 2) == bytes_of({0x5A, 0xA5}));
    CHECK(l8.read_reg(0x7FFF, 1) == bytes_of({0x01}));
    fake_vl53l8_fail_next(r.fake, 0x32, 0x06);  // ERR_BUSY
    CHECK(throws<depz::BusyError>([&] { (void)l8.read_reg(0x0123, 2); }));
    CHECK(l8.read_reg(0x0123, 1) == bytes_of({0x5A}));  // the next one is answered
}

// A thermometer that stops answering is skipped: the session goes on and
// the file gets no temperature record (the sr04 records still land).
void dataset_recorder_skips_a_failing_thermometer()
{
    Rig a;
    const std::string path = tmp_dataset_path("thermo.depzdata");
    const auto temperature_requests = [&] {
        std::uint8_t cmds[256];
        const std::size_t n = fake_sr04_commands(a.fake, cmds, sizeof cmds);
        return static_cast<int>(std::count(cmds, cmds + std::min(n, sizeof cmds), std::uint8_t{0x07}));
    };
    {
        ds::RecorderOptions o;
        o.temperature_hz = 50.0;
        ds::SessionRecorder rec(path, o);
        CHECK(rec.add(*a.dev) == "d0");
        fake_sr04_set_short_temperature(a.fake, true);
        const int before = temperature_requests();
        rec.start();
        fake_sr04_send_measurement(a.fake, 0x37);
        CHECK(until([&] { return rec.records_written() >= 1 && temperature_requests() >= before + 3; }, 3000));
    }
    const ds::Reader r = ds::Reader::open(path);
    std::remove(path.c_str());
    std::size_t sr04 = 0, temps = 0;
    for (const auto& rec : r.records) {
        if (rec.kind == "sr04") sr04++;
        if (rec.kind == "temperature") temps++;
    }
    CHECK(sr04 == 1 && temps == 0);
}

// CPP-1: an Unsubscribe outlives its device's handle. Once the device is
// closed or destroyed it does nothing (the C handle is freed) — the
// SessionRecorder's own stop() unhooks after a close; an Unsubscribe taken
// before promote() still removes its callback from the promoted device.
void an_unsubscribe_after_close_does_nothing()
{
    {
        Rig r;
        auto unsub = r.dev->on_measurement([](const depz::Sr04Measurement&) {});
        r.dev->close();
        unsub();
        unsub();
    }
    {
        Rig r;
        auto unsub = r.dev->on_event([](const depz::DeviceEvent&) {});
        r.dev.reset();
        unsub();
    }
    {
        Rig r;
        const std::string path = tmp_dataset_path("closed.depzdata");
        {
            ds::RecorderOptions o;
            o.temperature_hz = std::nullopt;
            ds::SessionRecorder rec(path, o);
            rec.add(*r.dev);
            rec.start();
            r.dev->close();
            rec.stop();
        }
        std::remove(path.c_str());
    }
    {
        auto [host, side] = depz::Link::loopback_pair();
        auto plain = depz::Device::open(std::move(host));
        std::atomic<int> texts{0};
        auto unsub = plain->on_event([&](const depz::DeviceEvent& e) {
            if (e.type == depz::DeviceEvent::Type::Text) texts++;
        });
        auto dev = depz::attach_sensor_class(std::move(plain), "sr04");
        std::atomic<int> after{0};  // registered last: runs after `texts`
        dev->on_event([&](const depz::DeviceEvent& e) {
            if (e.type == depz::DeviceEvent::Type::Text) after++;
        });
        const depz::bytes payload{std::byte{0x00}};
        side.write(depz::build_packet(0x7F, payload, 1));
        CHECK(until([&] { return after == 1; }, 2000));
        CHECK(texts == 1);
        unsub();  // the handle lives on in `dev`: this really unsubscribes
        side.write(depz::build_packet(0x7F, payload, 2));
        CHECK(until([&] { return after == 2; }, 2000));
        CHECK(texts == 1);
    }
}

// CPP-2: the reader may run a recorder hook it snapshotted before stop()
// unhooked it — here behind a slow user callback, with the recorder destroyed
// meanwhile. The hook must find the recorder gone, not touch freed memory.
void a_recorder_destroyed_under_a_running_hook_is_not_touched()
{
    Rig r;
    std::atomic<bool> inside{false}, release{false}, tail{false};
    auto slow = r.dev->on_measurement([&](const depz::Sr04Measurement&) {
        inside = true;
        until([&] { return release.load(); }, 3000);
    });
    const std::string path = tmp_dataset_path("dtor.depzdata");
    depz::Device::Unsubscribe last;
    {
        ds::RecorderOptions o;
        o.temperature_hz = std::nullopt;
        ds::SessionRecorder rec(path, o);
        rec.add(*r.dev);
        rec.start();
        // registered after the recorder's hook: it runs after it, same packet
        last = r.dev->on_measurement([&](const depz::Sr04Measurement&) { tail = true; });
        fake_sr04_send_measurement(r.fake, 0x37);
        CHECK(until([&] { return inside.load(); }, 2000));
    }
    release = true;
    CHECK(until([&] { return tail.load(); }, 2000));
    slow();
    last();
    std::remove(path.c_str());
}

// CPP-3: close() on one thread while another waits inside a request: the
// waiter gets DeviceLostError, and no thread races on the handle (ThreadSanitizer).
void close_while_a_request_waits()
{
    depz_link* host = nullptr;
    fake_sr04* f = fake_silent_start(&host);
    std::unique_ptr<depz::Device> dev = depz::Sr04::open(depz::Link::adopt(host));
    dev->set_timeout(5000ms);
    std::atomic<int> outcome{0};  // 1 DeviceLostError, 2 anything else
    std::thread waiter([&] {
        try {
            dev->device_name();
            outcome = 2;
        } catch (const depz::DeviceLostError&) {
            outcome = 1;
        } catch (...) {
            outcome = 2;
        }
    });
    const bool sent = until([&] { return dev->stats().tx_packets >= 1; }, 2000);
    // a reader of the handle with no lock in common with close()
    std::thread watcher([&] {
        while (dev->c_handle()) std::this_thread::yield();
    });
    dev->close();
    watcher.join();
    waiter.join();
    dev.reset();
    fake_sr04_stop(f);
    CHECK(sent);
    CHECK(outcome == 1);
}

const Case k_cases[] = {
    {"identity_and_types", identity_and_types},
    {"errors_are_typed", errors_are_typed},
    {"short_reply_fails_its_request_and_the_reader_goes_on", short_reply_fails_its_request_and_the_reader_goes_on},
    {"timeout_when_silent", timeout_when_silent},
    {"a_port_that_does_not_open_names_the_open_failure", a_port_that_does_not_open_names_the_open_failure},
    {"measure_stream_and_callbacks", measure_stream_and_callbacks},
    {"a_throwing_callback_never_stops_the_reader", a_throwing_callback_never_stops_the_reader},
    {"disconnect_ends_streams", disconnect_ends_streams},
    {"host_seq_gap_is_an_event", host_seq_gap_is_an_event},
    {"promote_and_record_replay", promote_and_record_replay},
    {"replay_refuses_a_gzipped_capture", replay_refuses_a_gzipped_capture},
    {"l4_fake_roundtrips_and_guards", l4_fake_roundtrips_and_guards},
    {"bno086_fake_errors_and_reports", bno086_fake_errors_and_reports},
    {"cnh_required_memory", cnh_required_memory},
    {"vl53l8ch_cnh_and_motion_exclusive", vl53l8ch_cnh_and_motion_exclusive},
    {"vlx_refusal_before_traffic", vlx_refusal_before_traffic},
    {"vlx_set_mode_hands_back_the_budget", vlx_set_mode_hands_back_the_budget},
    {"tmf8829_fake_session", tmf8829_fake_session},
    {"dataset_recorder_two_sr04", dataset_recorder_two_sr04},
    {"dataset_recorder_temperature_gz", dataset_recorder_temperature_gz},
    {"dataset_recorder_bno086", dataset_recorder_bno086},
    {"device_core_extras", device_core_extras},
    {"dataset_live_records", dataset_live_records},
    {"vl53l8_fake_init_and_configuration", vl53l8_fake_init_and_configuration},
    {"vl53l8_fake_thresholds_calibration_and_ranging", vl53l8_fake_thresholds_calibration_and_ranging},
    {"vl53l8ch_fake_cnh", vl53l8ch_fake_cnh},
    {"vl53lx_fake_l1cx_uld", vl53lx_fake_l1cx_uld},
    {"vl53lx_fake_l4cd_and_l3cx", vl53lx_fake_l4cd_and_l3cx},
    {"bno055_fake_registers_modes_and_config", bno055_fake_registers_modes_and_config},
    {"bno055_fake_samples_and_stream", bno055_fake_samples_and_stream},
    {"bno086_fake_commands", bno086_fake_commands},
    {"l4_fake_tuning_stream_and_registers", l4_fake_tuning_stream_and_registers},
    {"dataset_recorder_skips_a_failing_thermometer", dataset_recorder_skips_a_failing_thermometer},
    {"l4_fake_calibrations_and_a_throwing_callback", l4_fake_calibrations_and_a_throwing_callback},
    {"tmf8829_throwing_callbacks_are_counted", tmf8829_throwing_callbacks_are_counted},
    {"dataset_json_and_gzip_edges", dataset_json_and_gzip_edges},
    {"vl53l8_fake_raw_registers", vl53l8_fake_raw_registers},
    {"a_throwing_progress_callback_propagates_out_of_init", a_throwing_progress_callback_propagates_out_of_init},
    {"attach_sensor_class_refusal_keeps_the_device", attach_sensor_class_refusal_keeps_the_device},
    {"an_unsubscribe_after_close_does_nothing", an_unsubscribe_after_close_does_nothing},
    {"a_recorder_destroyed_under_a_running_hook_is_not_touched", a_recorder_destroyed_under_a_running_hook_is_not_touched},
    {"close_while_a_request_waits", close_while_a_request_waits},
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
