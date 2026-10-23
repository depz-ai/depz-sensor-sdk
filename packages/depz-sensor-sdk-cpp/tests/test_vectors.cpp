// Vector-driven parity test runner: loads contracts/vectors/*.json (including
// vl53l4.json, vl53l7.json, vl53lx.json, bno055.json, tmf8829.json,
// tmf8829_frame_time.json) and asserts
// the C++ SDK is byte-exact with the golden vectors.
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "depz/bno055.hpp"
#include "depz/bno086.hpp"
#include "depz/common.hpp"
#include "depz/crc.hpp"
#include "depz/dataset.hpp"
#include "depz/framing.hpp"
#include "depz/fwdepz.hpp"
#include "depz/identity.hpp"
#include "depz/sr04.hpp"
#include "depz/tmf8829.hpp"
#include "depz/usb_ids.hpp"
#include "depz/vl53l4.hpp"
#include "depz/vl53l7.hpp"
#include "depz/vl53lx.hpp"
#include "depz/vl53l8.hpp"
#ifdef DEPZ_TEST_HAS_IO
#include "depz/device.hpp"
#endif
#include "json.hpp"
#include "vector_helpers.hpp"

#ifndef DEPZ_VECTORS_DIR
#error "DEPZ_VECTORS_DIR must be defined"
#endif
#ifndef DEPZ_RECORDINGS_DIR
#error "DEPZ_RECORDINGS_DIR must be defined"
#endif

using testjson::Value;

// ---- test harness ----------------------------------------------------------
static int g_checks = 0;
static int g_fails = 0;

static void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_fails;
        std::cerr << "  FAIL: " << what << "\n";
    }
}

template <class A, class B>
static void check_eq(const A& a, const B& b, const std::string& what) {
    ++g_checks;
    if (!(a == b)) {
        ++g_fails;
        std::cerr << "  FAIL: " << what << " (got '" << a << "' want '" << b << "')\n";
    }
}

// ---- coverage gate (scripts/check_vector_coverage.py) ----------------------
#ifdef DEPZ_TEST_VECTOR_SEEN
extern "C" void depz_test_vector_seen(const char* id);  // tests/vector_seen.c
#endif

// Report a golden-vector ID as asserted ($DEPZ_VECTOR_SEEN), only when no
// check failed since `fails_before` was taken: a failing case is never reported.
static void seen_id(const std::string& id, int fails_before) {
#ifdef DEPZ_TEST_VECTOR_SEEN
    if (g_fails == fails_before) depz_test_vector_seen(id.c_str());
#else
    (void)id;
    (void)fails_before;
#endif
}

// A case object: its "id" (every case of a case-list group carries one).
static void seen(const Value& c, int fails_before) {
    check(c.has("id"), "vector case without an \"id\"");
    if (c.has("id")) seen_id(c.at("id").as_string(), fails_before);
}

// ---- hex helpers -----------------------------------------------------------
static depz::bytes from_hex(const std::string& h) {
    depz::bytes out;
    out.reserve(h.size() / 2);
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i + 1 < h.size(); i += 2) {
        int hi = nib(h[i]), lo = nib(h[i + 1]);
        out.push_back(std::byte{static_cast<std::uint8_t>((hi << 4) | lo)});
    }
    return out;
}

static std::string to_hex(const depz::bytes& b) {
    static const char* d = "0123456789abcdef";
    std::string out;
    out.reserve(b.size() * 2);
    for (std::byte x : b) {
        std::uint8_t v = std::to_integer<std::uint8_t>(x);
        out.push_back(d[v >> 4]);
        out.push_back(d[v & 0xF]);
    }
    return out;
}

static std::string to_hex(depz::byte_span b) {
    depz::bytes v(b.begin(), b.end());
    return to_hex(v);
}

static std::string load_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "cannot open " << path << "\n";
        std::exit(2);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static Value load_vector(const std::string& name) {
    return testjson::parse(load_file(std::string(DEPZ_VECTORS_DIR) + "/" + name));
}

static std::string load_recording(const std::string& name) {
    return load_file(std::string(DEPZ_RECORDINGS_DIR) + "/" + name);
}

static std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t nl = s.find('\n', i);
        std::string line = s.substr(i, nl == std::string::npos ? std::string::npos : nl - i);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") != std::string::npos) out.push_back(line);
        if (nl == std::string::npos) break;
        i = nl + 1;
    }
    return out;
}

// ---- crc -------------------------------------------------------------------
static void test_crc() {
    Value v = load_vector("crc.json");
    for (const Value& c : v.at("cases").as_array()) {
        const int f0 = g_fails;
        depz::bytes in = from_hex(c.at("input").as_string());
        std::string nm = c.at("name").as_string();
        check_eq(static_cast<std::uint64_t>(depz::crc8_maxim(in)),
                 c.at("crc8_maxim").as_u64(), "crc8_maxim " + nm);
        check_eq(static_cast<std::uint64_t>(depz::crc16_modbus(in)),
                 c.at("crc16_modbus").as_u64(), "crc16_modbus " + nm);
        check_eq(static_cast<std::uint64_t>(depz::crc32_iso_hdlc(in)),
                 c.at("crc32_iso_hdlc").as_u64(), "crc32_iso_hdlc " + nm);
        check_eq(static_cast<std::uint64_t>(depz::crc16_ccitt_false(in)),
                 c.at("crc16_ccitt_false").as_u64(), "crc16_ccitt_false " + nm);
        seen(c, f0);
    }
}

// ---- framing encode --------------------------------------------------------
static void test_framing_encode() {
    Value v = load_vector("framing_encode.json");
    for (const Value& c : v.at("cases").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes payload = from_hex(c.at("payload").as_string());
        auto cmd = static_cast<std::uint8_t>(c.at("cmd").as_int());
        auto seq = static_cast<std::uint32_t>(c.at("seq").as_int());
        auto crc_type = static_cast<depz::CrcType>(c.at("crc_type").as_int());
        depz::bytes frame = depz::build_packet(cmd, depz::as_bytes(payload), seq, crc_type);
        check_eq(to_hex(frame), c.at("frame").as_string(), "framing_encode " + nm);
        seen(c, f0);
    }
}

// ---- framing decode --------------------------------------------------------
struct DecodeResult {
    std::vector<std::string> events;  // "P cmd seq payload" / "C cmd seq"
    std::string trash;                // concatenated hex
    std::string residue;              // hex
    std::size_t header_errors = 0;
};

static void absorb(depz::PacketParser& p, const std::vector<depz::ParserEvent>& evs,
                   DecodeResult& r) {
    for (const auto& ev : evs) {
        if (auto* pkt = std::get_if<depz::Packet>(&ev)) {
            r.events.push_back("P " + std::to_string(pkt->cmd) + " " + std::to_string(pkt->seq) +
                               " " + to_hex(pkt->payload));
        } else if (auto* ce = std::get_if<depz::CrcError>(&ev)) {
            r.events.push_back("C " + std::to_string(ce->cmd) + " " + std::to_string(ce->seq));
        } else if (auto* tr = std::get_if<depz::Trash>(&ev)) {
            r.trash += to_hex(tr->data);
        }
    }
    (void)p;
}

static DecodeResult decode_whole(const depz::bytes& stream) {
    depz::PacketParser p;
    DecodeResult r;
    absorb(p, p.feed(depz::as_bytes(stream)), r);
    r.residue = to_hex(p.residue());
    r.header_errors = p.header_errors;
    return r;
}

static DecodeResult decode_byte_by_byte(const depz::bytes& stream) {
    depz::PacketParser p;
    DecodeResult r;
    for (std::byte b : stream) {
        absorb(p, p.feed(depz::byte_span(&b, 1)), r);
    }
    r.residue = to_hex(p.residue());
    r.header_errors = p.header_errors;
    return r;
}

static DecodeResult decode_random(const depz::bytes& stream, std::uint32_t seed) {
    depz::PacketParser p;
    DecodeResult r;
    std::uint32_t st = seed ? seed : 1;
    std::size_t i = 0;
    while (i < stream.size()) {
        st = st * 1664525u + 1013904223u;  // LCG
        std::size_t chunk = 1 + (st >> 8) % 7;  // 1..7 bytes
        chunk = std::min(chunk, stream.size() - i);
        absorb(p, p.feed(depz::byte_span(stream.data() + i, chunk)), r);
        i += chunk;
    }
    r.residue = to_hex(p.residue());
    r.header_errors = p.header_errors;
    return r;
}

static bool same(const DecodeResult& a, const DecodeResult& b) {
    return a.events == b.events && a.trash == b.trash && a.residue == b.residue &&
           a.header_errors == b.header_errors;
}

static void test_framing_decode() {
    Value v = load_vector("framing_decode.json");
    for (const Value& c : v.at("cases").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes stream = from_hex(c.at("stream").as_string());

        // Expected normalized result from JSON.
        DecodeResult exp;
        const Value& e = c.at("expect");
        for (const Value& ev : e.at("events").as_array()) {
            std::string type = ev.at("type").as_string();
            if (type == "packet") {
                exp.events.push_back("P " + std::to_string(ev.at("cmd").as_int()) + " " +
                                     std::to_string(ev.at("seq").as_int()) + " " +
                                     ev.at("payload").as_string());
            } else if (type == "crc_error") {
                exp.events.push_back("C " + std::to_string(ev.at("cmd").as_int()) + " " +
                                     std::to_string(ev.at("seq").as_int()));
            }
        }
        exp.trash = e.at("trash").as_string();
        exp.residue = e.at("residue").as_string();
        exp.header_errors = static_cast<std::size_t>(e.at("header_errors").as_int());

        DecodeResult whole = decode_whole(stream);
        DecodeResult bbb = decode_byte_by_byte(stream);

        check(same(whole, exp), "framing_decode whole==expect " + nm);
        check(same(bbb, exp), "framing_decode byte_by_byte==expect " + nm);
        for (std::uint32_t seed : {1u, 7u, 12345u, 99991u}) {
            DecodeResult rnd = decode_random(stream, seed);
            check(same(rnd, exp), "framing_decode random==expect " + nm +
                                      " seed=" + std::to_string(seed));
        }
        seen(c, f0);
    }
}

// ---- usb ids ---------------------------------------------------------------
static void test_usb_ids() {
    Value v = load_vector("usb_ids.json");
    for (const Value& c : v.at("identity").as_array()) {
        const int f0 = g_fails;
        auto vid = static_cast<std::uint16_t>(c.at("vid").as_int());
        auto pid = static_cast<std::uint16_t>(c.at("pid").as_int());
        std::string tag = "vid=" + std::to_string(vid) + " pid=" + std::to_string(pid);
        check_eq(depz::is_known_depz_usb(vid, pid), c.at("known").as_bool(),
                 "is_known_depz_usb " + tag);
        auto hint = depz::usb_model_hint(vid, pid);
        const Value& model = c.at("model");
        if (model.is_null()) {
            check(!hint.has_value(), "usb_model_hint null " + tag);
        } else {
            check(hint.has_value() && *hint == model.as_string(), "usb_model_hint " + tag);
        }
        seen(c, f0);
    }
    // None inputs.
    check(!depz::is_known_depz_usb(std::nullopt, 0xEC78), "is_known null vid");
    check(!depz::is_known_depz_usb(0x1BCF, std::nullopt), "is_known null pid");

    // The id constants themselves.
    {
        int f0 = g_fails;
        check_eq(v.at("vid").as_int(), static_cast<std::int64_t>(depz::DEPZ_USB_VID), "usb_ids vid");
        seen_id("usb_ids/vid", f0);
        f0 = g_fails;
        check_eq(v.at("dev_vid").as_int(), static_cast<std::int64_t>(depz::DEV_USB_VID), "usb_ids dev_vid");
        seen_id("usb_ids/dev_vid", f0);
        f0 = g_fails;
        check_eq(v.at("dev_pid").as_int(), static_cast<std::int64_t>(depz::DEV_USB_PID), "usb_ids dev_pid");
        seen_id("usb_ids/dev_pid", f0);
        f0 = g_fails;
        const auto& pr = v.at("pid_range").as_array();
        check(pr.size() == 2 && pr[0].as_int() == depz::DEPZ_PID_RANGE_LO &&
                  pr[1].as_int() == depz::DEPZ_PID_RANGE_HI,
              "usb_ids pid_range");
        seen_id("usb_ids/pid_range", f0);
    }

    for (const Value& s : v.at("serial_ordering").as_array()) {
        const int f0 = g_fails;
        std::string nm = s.at("name").as_string();
        std::vector<depz::PortInfo> ports;
        for (const Value& p : s.at("ports").as_array()) {
            depz::PortInfo pi;
            pi.port = p.at("port").as_string();
            const Value& ser = p.at("serial");
            if (!ser.is_null()) pi.usb_serial = ser.as_string();
            ports.push_back(pi);
        }
        auto ordered = depz::order_by_serial(ports);
        std::vector<std::string> got;
        for (const auto& p : ordered) got.push_back(p.port);
        std::vector<std::string> want;
        for (const Value& o : s.at("order").as_array()) want.push_back(o.as_string());
        check(got == want, "serial_ordering " + nm);
        seen(s, f0);
    }
}

// ---- common commands -------------------------------------------------------
static void test_common_commands() {
    Value v = load_vector("common_commands.json");
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        depz::bytes got;
        if (kind == "sync_time_request") {
            got = depz::pack_sync_time(c.at("pc_timestamp_us").as_u64());
        } else if (kind == "set_payload_crc_type") {
            got = depz::pack_set_payload_crc_type(
                static_cast<std::uint8_t>(c.at("crc_type").as_int()));
        } else if (kind == "sync_pin_config") {
            depz::SyncPinConfig cfg{static_cast<std::uint8_t>(c.at("pin").as_int()),
                                    static_cast<depz::SyncPinMode>(c.at("mode").as_int()),
                                    static_cast<depz::SyncPinPolarity>(c.at("polarity").as_int())};
            got = cfg.pack();
        } else {
            check(false, "unknown encode kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "common encode " + nm);
        seen(c, f0);
    }

    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        int report = static_cast<int>(c.at("report").as_int());
        depz::bytes payload = from_hex(c.at("payload").as_string());
        const Value& ex = c.at("expect");
        if (report == 0x80) {
            auto r = depz::StatusReport::unpack(payload);
            check_eq(static_cast<std::int64_t>(r.cmd), ex.at("cmd").as_int(), "status.cmd " + nm);
            check_eq(static_cast<std::int64_t>(r.status), ex.at("status").as_int(),
                     "status.status " + nm);
        } else if (report == 0x81) {
            auto r = depz::TextReport::unpack(payload);
            check_eq(static_cast<std::int64_t>(r.cmd), ex.at("cmd").as_int(), "text.cmd " + nm);
            check_eq(r.text, ex.at("text").as_string(), "text.text " + nm);
        } else if (report == 0x83) {
            auto r = depz::TemperatureReport::unpack(payload);
            check_eq(static_cast<std::int64_t>(r.timestamp_us), ex.at("timestamp_us").as_int(),
                     "temp.timestamp " + nm);
            check_eq(static_cast<std::int64_t>(r.raw_decidegrees),
                     ex.at("raw_decidegrees").as_int(), "temp.raw " + nm);
        } else if (report == 0x84) {
            auto r = depz::SequenceErrorReport::unpack(payload);
            check_eq(static_cast<std::int64_t>(r.expected_seq), ex.at("expected_seq").as_int(),
                     "seqerr.expected " + nm);
            check_eq(static_cast<std::int64_t>(r.received_seq), ex.at("received_seq").as_int(),
                     "seqerr.received " + nm);
        } else {
            check(false, "unknown decode report " + std::to_string(report));
        }
        seen(c, f0);
    }

    for (const Value& c : v.at("sync_time_math").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        auto res = depz::sync_time_offset_rtt(c.at("t1").as_int(), c.at("t2").as_int(),
                                              c.at("t3").as_int(), c.at("t4").as_int());
        check_eq(res.first, c.at("offset_us").as_int(), "sync_time offset " + nm);
        check_eq(res.second, c.at("rtt_us").as_int(), "sync_time rtt " + nm);
        seen(c, f0);
    }
}

// ---- identity --------------------------------------------------------------
static void test_identity() {
    Value v = load_vector("identity.json");
    for (const Value& c : v.at("cases").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        std::string name = depz::strip_device_string(raw);
        depz::Identity id = depz::parse_software_name(name);
        const Value& ex = c.at("expect");
        check_eq(depz::to_string(id.mode), ex.at("mode").as_string(), "identity.mode " + nm);
        check_eq(id.software_name, ex.at("software_name").as_string(),
                 "identity.software_name " + nm);
        check_eq(id.version, ex.at("version").as_string(), "identity.version " + nm);
        const Value& st = ex.at("sensor_type");
        if (st.is_null()) {
            check(!id.sensor_type.has_value(), "identity.sensor_type null " + nm);
        } else {
            check(id.sensor_type.has_value() &&
                      depz::to_string(*id.sensor_type) == st.as_string(),
                  "identity.sensor_type " + nm);
        }
        seen(c, f0);
    }
}

// ---- probe: class resolution (contract 02 §4.4) ----------------------------
static void test_probe() {
    Value v = load_vector("probe.json");
    {
        const int f0 = g_fails;
        const auto& want = v.at("classes").as_array();
        const auto& got = depz::sensor_class_names();
        check_eq(got.size(), want.size(), "probe.classes count");
        for (std::size_t i = 0; i < want.size() && i < got.size(); i++)
            check_eq(got[i], want[i].as_string(), "probe.classes[" + std::to_string(i) + "]");
        seen_id("probe/classes", f0);
    }
    auto id16 = [](const Value& x) -> std::optional<std::uint16_t> {
        if (x.is_null()) return std::nullopt;
        return static_cast<std::uint16_t>(x.as_int());
    };
    for (const Value& c : v.at("resolve").as_array()) {
        const int f0 = g_fails;
        check_eq(depz::resolve_sensor_class(c.at("software_name").as_string(), c.at("device_name").as_string(),
                                            id16(c.at("usb_vid")), id16(c.at("usb_pid"))),
                 c.at("class").as_string(), "probe.resolve " + c.at("name").as_string());
        seen(c, f0);
    }
}

// ---- sr04 ------------------------------------------------------------------
static void test_sr04() {
    Value v = load_vector("sr04.json");
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        depz::bytes got;
        if (kind == "set_sample_period") {
            got = depz::pack_sample_period(static_cast<std::uint32_t>(c.at("period_us").as_u64()));
        } else if (kind == "set_echo_decay") {
            got = depz::pack_echo_decay(static_cast<std::uint16_t>(c.at("decay_us").as_int()));
        } else {
            check(false, "unknown sr04 encode kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "sr04 encode " + nm);
        seen(c, f0);
    }

    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        int report = static_cast<int>(c.at("report").as_int());
        depz::bytes payload = from_hex(c.at("payload").as_string());
        const Value& ex = c.at("expect");
        if (report == 0x91) {
            auto d = depz::Sr04Data::unpack(payload);
            check_eq(static_cast<std::int64_t>(d.source_cmd), ex.at("source_cmd").as_int(),
                     "sr04.source_cmd " + nm);
            check_eq(static_cast<std::int64_t>(d.timestamp_us), ex.at("timestamp_us").as_int(),
                     "sr04.timestamp " + nm);
            check_eq(static_cast<std::int64_t>(d.echo_time_us), ex.at("echo_time_us").as_int(),
                     "sr04.echo " + nm);
        } else if (report == 0x92) {
            check_eq(static_cast<std::int64_t>(depz::unpack_sample_period(payload)),
                     ex.at("period_us").as_int(), "sr04.period " + nm);
        } else if (report == 0x93) {
            check_eq(static_cast<std::int64_t>(depz::unpack_echo_decay(payload)),
                     ex.at("decay_us").as_int(), "sr04.decay " + nm);
        } else {
            check(false, "unknown sr04 decode report " + std::to_string(report));
        }
        seen(c, f0);
    }
}

// ---- fwdepz ----------------------------------------------------------------
static void test_fwdepz() {
    Value v = load_vector("fwdepz.json");
    for (const Value& c : v.at("cases").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes blob = from_hex(c.at("file").as_string());
        if (c.has("error")) {
            std::string want = c.at("error").as_string();
            std::string got_kind = "none";
            try {
                depz::FwDepzImage::parse(depz::as_bytes(blob));
            } catch (const depz::FwDepzError& e) {
                switch (e.kind) {
                    case depz::FwDepzErrorKind::TooShort: got_kind = "too_short"; break;
                    case depz::FwDepzErrorKind::Magic: got_kind = "magic"; break;
                    case depz::FwDepzErrorKind::HeaderCrc: got_kind = "header_crc"; break;
                    case depz::FwDepzErrorKind::Size: got_kind = "size"; break;
                }
            }
            check_eq(got_kind, want, "fwdepz error " + nm);
        } else {
            const Value& ex = c.at("expect");
            try {
                auto img = depz::FwDepzImage::parse(depz::as_bytes(blob));
                check_eq(static_cast<std::int64_t>(img.load_addr), ex.at("load_addr").as_int(),
                         "fwdepz.load_addr " + nm);
                check_eq(static_cast<std::int64_t>(img.fw_size), ex.at("fw_size").as_int(),
                         "fwdepz.fw_size " + nm);
                check_eq(static_cast<std::int64_t>(img.fw_crc32), ex.at("fw_crc32").as_int(),
                         "fwdepz.fw_crc32 " + nm);
                check_eq(static_cast<std::int64_t>(img.cur_sec), ex.at("cur_sec").as_int(),
                         "fwdepz.cur_sec " + nm);
                check_eq(static_cast<std::int64_t>(img.tot_sec), ex.at("tot_sec").as_int(),
                         "fwdepz.tot_sec " + nm);
                check_eq(img.payload_crc_ok(), ex.at("payload_crc_ok").as_bool(),
                         "fwdepz.payload_crc_ok " + nm);
            } catch (const depz::FwDepzError& e) {
                check(false, std::string("fwdepz unexpected error ") + nm + ": " + e.what());
            }
        }
        seen(c, f0);
    }
}

// ---- vl53l8 frame reassembly + decode (recording replay) -------------------
// The replay capture is from a VL53L8CX (dev-default) device (software_name
// APP_VL53L8_v0.9). The results-frame layout is shared with the VL53L8CH, so the same decode path
// covers both; only the footer-id offset is variant-specific (see the decode
// call below for which offset this firmware's frames carry).
// A hand-built 4x4 frame with known wire values: the scaling to real units
// (contract 04 §5), as the Python test_get_ranging_data_*_and_scales. Blocks
// are laid out in the swapped domain, then byte-reversed per 32-bit word like
// the sensor sends them.
static void test_vl53l8_real_units() {
    constexpr int N = 16;
    const std::array<std::int16_t, N> dist_raw{400 * 4 + 3, -5, -1, 0, 3, 4, 7, 8};
    const std::array<std::uint32_t, N> rate_raw{1, 2048, 3072, 0, 4096};
    const std::array<std::uint16_t, N> sigma_raw{256, 1, 129};
    const std::array<std::uint8_t, N> refl_raw{3, 200, 255, 0};
    std::vector<std::uint8_t> buf(400, 0);
    buf[8] = 0x12;
    buf[9] = 0x34;
    std::size_t i = 16;
    auto block = [&](std::uint32_t type, std::uint32_t idx, auto&& put) {
        std::uint32_t bh = type | (static_cast<std::uint32_t>(N) << 4) | (idx << 16);
        for (int b = 0; b < 4; ++b) buf[i + b] = static_cast<std::uint8_t>(bh >> (8 * b));
        for (int z = 0; z < N; ++z) put(&buf[i + 4 + static_cast<std::size_t>(z) * (type > 1 ? type : 1)], z);
        i += 4 + static_cast<std::size_t>(N) * (type > 1 ? type : 1);
    };
    auto le = [](std::uint8_t* p, std::uint32_t v, int n) {
        for (int b = 0; b < n; ++b) p[b] = static_cast<std::uint8_t>(v >> (8 * b));
    };
    block(4, 0x54d0, [&](std::uint8_t* p, int z) { le(p, rate_raw[z], 4); });        // ambient
    block(1, 0xdb84, [&](std::uint8_t* p, int) { *p = 1; });                           // nb target
    block(4, 0xdbc4, [&](std::uint8_t* p, int z) { le(p, rate_raw[z], 4); });        // signal
    block(2, 0xdec4, [&](std::uint8_t* p, int z) { le(p, sigma_raw[z], 2); });       // sigma
    block(2, 0xdf44, [&](std::uint8_t* p, int z) {
        le(p, static_cast<std::uint16_t>(dist_raw[z]), 2);                              // distance
    });
    block(1, 0xe044, [&](std::uint8_t* p, int z) { *p = refl_raw[z]; });             // reflectance
    block(1, 0xe084, [&](std::uint8_t* p, int) { *p = 5; });                           // status
    buf[i] = 0xf0;  // a header whose block runs past the end stops the walk
    buf[i + 1] = 0xff;
    const std::size_t foff = buf.size() - depz::vl53l8::FOOTER_ID_OFF_CX;
    buf[foff] = 0x12;
    buf[foff + 1] = 0x34;
    depz::bytes raw(buf.size());
    for (std::size_t w = 0; w < buf.size(); w += 4)
        for (int b = 0; b < 4; ++b) raw[w + b] = static_cast<std::byte>(buf[w + 3 - b]);

    auto f = depz::vl53l8::decode_frame(depz::as_bytes(raw), depz::vl53l8::FOOTER_ID_OFF_CX);
    check(f.has_value(), "vl53l8 synthetic frame decodes");
    if (!f) return;
    check_eq(static_cast<std::int64_t>(f->resolution), 16, "vl53l8 synthetic resolution");
    const std::array<std::int32_t, 8> dist_mm{400, 0, 0, 0, 0, 1, 1, 2};  // /4 floored, < 0 -> 0
    for (std::size_t k = 0; k < dist_mm.size(); ++k)
        check_eq(static_cast<std::int64_t>(f->distance_mm[k]), static_cast<std::int64_t>(dist_mm[k]),
                 "vl53l8 synthetic distance_mm[" + std::to_string(k) + "]");
    const std::array<double, 5> rate{1.0 / 2048, 1.0, 1.5, 0.0, 2.0};  // kcps/SPAD
    for (std::size_t k = 0; k < rate.size(); ++k) {
        check(f->signal_per_spad[k] == rate[k], "vl53l8 synthetic signal_per_spad[" + std::to_string(k) + "]");
        check(f->ambient_per_spad[k] == rate[k], "vl53l8 synthetic ambient_per_spad[" + std::to_string(k) + "]");
    }
    const std::array<double, 3> sigma{2.0, 1.0 / 128, 129.0 / 128};  // mm
    for (std::size_t k = 0; k < sigma.size(); ++k)
        check(f->range_sigma_mm[k] == sigma[k], "vl53l8 synthetic range_sigma_mm[" + std::to_string(k) + "]");
    const std::array<double, 4> refl{1.5, 100.0, 127.5, 0.0};  // %
    for (std::size_t k = 0; k < refl.size(); ++k)
        check(f->reflectance[k] == refl[k], "vl53l8 synthetic reflectance[" + std::to_string(k) + "]");
}

static void test_vl53l8_frame() {
    test_vl53l8_real_units();
    const int f0 = g_fails;
    Value exp = testjson::parse(load_recording("vl53l8_8x8_15hz_3s.expected.json"));
    const auto& want_frames = exp.at("frames").as_array();

    // Extract VL53_FRAME (cmd 0x93) chunks from the recorded rx stream, then
    // reassemble + decode — the verifiable decode layer, with the live ULD init
    // driver deliberately bypassed.
    std::string rec = load_recording("vl53l8_8x8_15hz_3s.depzrec");
    depz::PacketParser parser;
    depz::vl53l8::FrameReassembler reasm;
    struct Got {
        std::uint64_t ts;
        depz::vl53l8::Vl53l8Frame f;
    };
    std::vector<Got> frames;
    std::size_t parse_errors = 0;

    for (const std::string& line : split_lines(rec)) {
        Value o = testjson::parse(line);
        if (!o.has("dir") || o.at("dir").as_string() != "rx") continue;
        depz::bytes data = from_hex(o.at("data").as_string());
        for (const auto& ev : parser.feed(depz::as_bytes(data))) {
            auto* pkt = std::get_if<depz::Packet>(&ev);
            if (!pkt || pkt->cmd != 0x93 || pkt->payload.size() < 12) continue;
            auto done = reasm.feed(depz::vl53l8::FrameChunk::unpack(depz::as_bytes(pkt->payload)));
            if (!done) continue;
            // The DEPZ firmware streams this CX capture with the offset-12
            // footer layout (matching how the golden sidecar was generated), so
            // the shared decoder runs with FOOTER_ID_OFF_CX here.
            auto decoded = depz::vl53l8::decode_frame(
                depz::as_bytes(done->second), depz::vl53l8::FOOTER_ID_OFF_CX);
            if (!decoded) {
                ++parse_errors;
                continue;
            }
            decoded->timestamp_us = done->first;
            frames.push_back(Got{done->first, std::move(*decoded)});
        }
    }

    // The recording streams one extra frame past the sidecar (the live capture
    // stopped after islice); require the sidecar frames to match exactly.
    check(frames.size() >= want_frames.size(),
          "vl53l8_frame decoded>=expected (" + std::to_string(frames.size()) + ">=" +
              std::to_string(want_frames.size()) + ")");
    check_eq(parse_errors, static_cast<std::size_t>(0), "vl53l8_frame no parse errors");

    std::size_t n = std::min(frames.size(), want_frames.size());
    for (std::size_t k = 0; k < n; ++k) {
        const auto& g = frames[k].f;
        const Value& w = want_frames[k];
        std::string nm = "frame#" + std::to_string(k);
        check_eq(static_cast<std::int64_t>(g.timestamp_us), w.at("timestamp_us").as_int(),
                 "vl53l8 " + nm + " ts");
        check_eq(static_cast<std::int64_t>(g.resolution), w.at("resolution").as_int(),
                 "vl53l8 " + nm + " resolution");
        check_eq(static_cast<std::int64_t>(g.silicon_temp_degc),
                 w.at("silicon_temp_degc").as_int(), "vl53l8 " + nm + " temp");
        const auto& wd = w.at("distance_mm").as_array();
        const auto& ws = w.at("target_status").as_array();
        const auto& wn = w.at("nb_target_detected").as_array();
        bool ok = g.distance_mm.size() == wd.size() && g.target_status.size() == ws.size() &&
                  g.nb_target_detected.size() == wn.size();
        for (std::size_t z = 0; ok && z < wd.size(); ++z) {
            if (static_cast<std::int64_t>(g.distance_mm[z]) != wd[z].as_int()) ok = false;
            if (static_cast<std::int64_t>(g.target_status[z]) != ws[z].as_int()) ok = false;
            if (static_cast<std::int64_t>(g.nb_target_detected[z]) != wn[z].as_int()) ok = false;
        }
        check(ok, "vl53l8 " + nm + " per-zone arrays");
    }
    seen_id("recordings/vl53l8_8x8_15hz_3s.depzrec", f0);
}

// ---- vl53l8 advanced DCI codecs (shared by CX and CH) ----------------------
static void test_vl53l8_advanced() {
    Value v = load_vector("vl53l8_advanced.json");
    for (const Value& m : v.at("motion").as_array()) {
        const int f0 = g_fails;
        std::string nm = m.at("name").as_string();
        auto cfg = depz::vl53l8::motion_config_init(static_cast<int>(m.at("resolution").as_int()));
        check_eq(to_hex(cfg.pack()), m.at("pack").as_string(), "vl53l8 motion " + nm);
        seen(m, f0);
    }
    for (const Value& t : v.at("thresholds").as_array()) {
        const int f0 = g_fails;
        std::string nm = t.at("name").as_string();
        std::vector<depz::vl53l8::DetectionThreshold> thr;
        for (const Value& e : t.at("thresholds").as_array()) {
            depz::vl53l8::DetectionThreshold d;
            d.low_thresh = static_cast<std::int32_t>(e.at("low_thresh").as_int());
            d.high_thresh = static_cast<std::int32_t>(e.at("high_thresh").as_int());
            d.measurement = static_cast<std::uint8_t>(e.at("measurement").as_int());
            d.type = static_cast<std::uint8_t>(e.at("type").as_int());
            d.zone_num = static_cast<std::uint8_t>(e.at("zone_num").as_int());
            d.operation = static_cast<std::uint8_t>(e.at("operation").as_int());
            thr.push_back(d);
        }
        check_eq(to_hex(depz::vl53l8::pack_detection_thresholds(thr)),
                 t.at("start_block").as_string(), "vl53l8 thresholds " + nm);
        check_eq(to_hex(depz::vl53l8::detection_thresholds_valid_status()),
                 t.at("valid_status").as_string(), "vl53l8 thresholds valid " + nm);
        seen(t, f0);
    }
    for (const Value& x : v.at("xtalk_margin").as_array()) {
        const int f0 = g_fails;
        std::string nm = x.at("name").as_string();
        check_eq(static_cast<std::int64_t>(depz::vl53l8::xtalk_margin_raw(x.at("kcps").as_double())),
                 x.at("raw").as_int(), "vl53l8 xtalk " + nm);
        seen(x, f0);
    }
}

// ---- vl53l8 CNH (Compact Network Histogram) decode — VL53L8CH only ----------
static void test_vl53l8_cnh() {
    const int f0 = g_fails;
    Value v = load_vector("vl53l8_cnh.json");
    const Value& cfg = v.at("config");
    int nb_agg = static_cast<int>(cfg.at("nb_of_aggregates").as_int());
    int feat = static_cast<int>(cfg.at("feature_length").as_int());
    depz::bytes raw = from_hex(v.at("cnh_raw").as_string());

    auto decoded = depz::vl53l8::decode_cnh(nb_agg, feat, depz::as_bytes(raw));

    // A block shorter than the config implies must be refused, not read past.
    {
        depz::bytes cut(raw.begin(), raw.end() - 1);
        bool threw = false;
        try {
            (void)depz::vl53l8::decode_cnh(nb_agg, feat, depz::as_bytes(cut));
        } catch (const std::length_error&) {
            threw = true;
        }
        check(threw, "vl53l8 cnh truncated block throws length_error");
        threw = false;
        try {
            (void)depz::vl53l8::decode_cnh(0, feat, depz::as_bytes(raw));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "vl53l8 cnh zero aggregates throws invalid_argument");
    }

    const Value& exp = v.at("expected");
    check_eq(static_cast<std::uint64_t>(decoded.ref_residual_word),
             exp.at("ref_residual_word").as_u64(), "vl53l8 cnh ref_residual_word");

    const auto& want_aggs = exp.at("aggregates").as_array();
    check_eq(decoded.aggregates.size(), want_aggs.size(), "vl53l8 cnh aggregate-count");

    std::size_t n = std::min(decoded.aggregates.size(), want_aggs.size());
    int agg_pass = 0;
    for (std::size_t g = 0; g < n; ++g) {
        const auto& a = decoded.aggregates[g];
        const Value& w = want_aggs[g];
        const auto& wr = w.at("hist_raw").as_array();
        const auto& ws = w.at("hist_scaler").as_array();
        std::string nm = "agg#" + std::to_string(g);
        bool ok = a.hist_raw.size() == wr.size() && a.hist_scaler.size() == ws.size();
        for (std::size_t i = 0; ok && i < wr.size(); ++i) {
            if (static_cast<std::int64_t>(a.hist_raw[i]) != wr[i].as_int()) ok = false;
            if (static_cast<std::int64_t>(a.hist_scaler[i]) != ws[i].as_int()) ok = false;
        }
        check(ok, "vl53l8 cnh " + nm + " hist_raw+hist_scaler exact");
        if (ok) ++agg_pass;
    }
    check_eq(agg_pass, static_cast<int>(want_aggs.size()), "vl53l8 cnh all aggregates exact");
    seen_id("vl53l8_cnh", f0);
}

// ---- vl53l7 bridge codecs + class resolution (vl53l7.json, contract 11) ----
static void test_vl53l7_codecs() {
    Value v = load_vector("vl53l7.json");
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        depz::bytes got;
        if (kind == "read_reg") {
            got = depz::vl53l7::pack_read_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                              static_cast<std::uint16_t>(c.at("len").as_int()));
        } else if (kind == "write_reg") {
            depz::bytes data = from_hex(c.at("data").as_string());
            got = depz::vl53l7::pack_write_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                               depz::as_bytes(data));
        } else if (kind == "pin_ctrl") {
            got = depz::vl53l7::pack_pin_ctrl(static_cast<std::uint8_t>(c.at("action").as_int()));
        } else if (kind == "set_i2c_speed") {
            got = depz::vl53l7::pack_set_i2c_speed(
                static_cast<std::uint16_t>(c.at("khz").as_int()));
        } else {
            check(false, "unknown vl53l7 encode kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "vl53l7 encode " + nm);
        seen(c, f0);
    }
    // The read-ceiling vector is the transfer limit itself.
    check_eq(static_cast<int>(depz::vl53l7::READ_MAX_LEN), 1536, "vl53l7 READ_MAX_LEN");
    check_eq(static_cast<int>(depz::vl53l7::WRITE_MAX_LEN), 2048, "vl53l7 WRITE_MAX_LEN");
    check_eq(static_cast<int>(depz::vl53l7::STREAM_CHUNK_MAX), 1536, "vl53l7 STREAM_CHUNK_MAX");

    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        check_eq(c.at("report").as_int(), static_cast<std::int64_t>(0x92), "vl53l7 report " + nm);
        depz::bytes payload = from_hex(c.at("payload").as_string());
        auto r = depz::vl53l7::Vl53l7Info::unpack(depz::as_bytes(payload));
        if (!r) {
            check(false, "vl53l7 info unpack " + nm);
            continue;
        }
        const Value& ex = c.at("expect");
        check_eq(static_cast<std::int64_t>(r->int_edges), ex.at("int_edges").as_int(),
                 "vl53l7 info.int_edges " + nm);
        check_eq(static_cast<std::int64_t>(r->frames_dropped), ex.at("frames_dropped").as_int(),
                 "vl53l7 info.frames_dropped " + nm);
        check_eq(static_cast<std::int64_t>(r->i2c_errors), ex.at("i2c_errors").as_int(),
                 "vl53l7 info.i2c_errors " + nm);
        check_eq(static_cast<std::int64_t>(r->last_i2c_error), ex.at("last_i2c_error").as_int(),
                 "vl53l7 info.last_i2c_error " + nm);
        check_eq(static_cast<std::int64_t>(r->lpn_level), ex.at("lpn_level").as_int(),
                 "vl53l7 info.lpn_level " + nm);
        check_eq(static_cast<std::int64_t>(r->int_level), ex.at("int_level").as_int(),
                 "vl53l7 info.int_level " + nm);
        check_eq(static_cast<std::int64_t>(r->i2c_khz), ex.at("i2c_khz").as_int(),
                 "vl53l7 info.i2c_khz " + nm);
        check_eq(static_cast<std::int64_t>(r->frame_size), ex.at("frame_size").as_int(),
                 "vl53l7 info.frame_size " + nm);
        check_eq(r->streaming, ex.at("streaming").as_bool(), "vl53l7 info.streaming " + nm);
        seen(c, f0);
    }
    depz::bytes short_info(depz::vl53l7::INFO_SIZE - 1, std::byte{0});
    check(!depz::vl53l7::Vl53l7Info::unpack(depz::as_bytes(short_info)).has_value(),
          "vl53l7 info rejects a short payload");

    for (const Value& c : v.at("model").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        std::optional<std::string> usb_model;
        if (!c.at("usb_model").is_null()) usb_model = c.at("usb_model").as_string();
        auto m = depz::vl53l7::resolve_model(usb_model, c.at("device_name").as_string());
        check_eq(depz::vl53l7::to_string(m), c.at("expect").as_string(), "vl53l7 model " + nm);
        seen(c, f0);
    }
}

// ---- vl53l4 bridge codecs + host-ULD math (vl53l4.json, contract 10) --------
static void check_l4_result(const std::optional<depz::vl53l4::Vl53l4Result>& r, const Value& ex,
                            const std::string& what) {
    if (!r) {
        check(false, what + " decode");
        return;
    }
    check_eq(static_cast<std::int64_t>(r->range_status), ex.at("range_status").as_int(),
             what + " range_status");
    check_eq(static_cast<std::int64_t>(r->distance_mm), ex.at("distance_mm").as_int(),
             what + " distance_mm");
    check_eq(static_cast<std::int64_t>(r->sigma_mm), ex.at("sigma_mm").as_int(),
             what + " sigma_mm");
    check_eq(static_cast<std::int64_t>(r->signal_rate_kcps), ex.at("signal_rate_kcps").as_int(),
             what + " signal_rate_kcps");
    check_eq(static_cast<std::int64_t>(r->ambient_rate_kcps), ex.at("ambient_rate_kcps").as_int(),
             what + " ambient_rate_kcps");
    check_eq(static_cast<std::int64_t>(r->signal_per_spad_kcps),
             ex.at("signal_per_spad_kcps").as_int(), what + " signal_per_spad_kcps");
    check_eq(static_cast<std::int64_t>(r->ambient_per_spad_kcps),
             ex.at("ambient_per_spad_kcps").as_int(), what + " ambient_per_spad_kcps");
    check_eq(static_cast<std::int64_t>(r->number_of_spad), ex.at("number_of_spad").as_int(),
             what + " number_of_spad");
    check_eq(static_cast<std::int64_t>(r->stream_count), ex.at("stream_count").as_int(),
             what + " stream_count");
}

static void test_vl53l4() {
    namespace L4 = depz::vl53l4;
    Value v = load_vector("vl53l4.json");
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        depz::bytes got;
        if (kind == "read_reg") {
            got = L4::pack_read_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                    static_cast<std::uint16_t>(c.at("len").as_int()));
        } else if (kind == "write_reg") {
            depz::bytes data = from_hex(c.at("data").as_string());
            got = L4::pack_write_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                     depz::as_bytes(data));
        } else if (kind == "xshut") {
            got = L4::pack_xshut(static_cast<std::uint8_t>(c.at("action").as_int()));
        } else if (kind == "start_stream") {
            got = L4::pack_start_stream(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                        static_cast<std::uint16_t>(c.at("len").as_int()),
                                        static_cast<std::uint8_t>(c.at("flags").as_int()));
        } else if (kind == "set_i2c_speed") {
            got = L4::pack_set_i2c_speed(static_cast<std::uint16_t>(c.at("khz").as_int()));
        } else {
            check(false, "unknown vl53l4 encode kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "vl53l4 encode " + nm);
        seen(c, f0);
    }

    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes payload = from_hex(c.at("payload").as_string());
        const Value& ex = c.at("expect");
        const std::int64_t report = c.at("report").as_int();
        if (report == 0x91) {
            auto r = L4::RegData::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "vl53l4 reg_data unpack " + nm);
                continue;
            }
            check_eq(static_cast<std::int64_t>(r->cmd), ex.at("cmd").as_int(), "vl53l4 cmd " + nm);
            check_eq(r->timestamp_us, ex.at("timestamp_us").as_u64(), "vl53l4 ts " + nm);
            check_eq(to_hex(r->data), ex.at("data").as_string(), "vl53l4 data " + nm);
        } else if (report == 0x92) {
            auto r = L4::Vl53l4Info::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "vl53l4 info unpack " + nm);
                continue;
            }
            auto f = [&](const char* k, std::int64_t got) {
                check_eq(got, ex.at(k).as_int(), std::string("vl53l4 info.") + k + " " + nm);
            };
            f("int_edges", r->int_edges);
            f("slots_skipped", r->slots_skipped);
            f("i2c_errors", r->i2c_errors);
            f("last_i2c_error", r->last_i2c_error);
            f("model_id", r->model_id);
            f("fw_status", r->fw_status);
            f("initialized", r->initialized);
            f("xshut_level", r->xshut_level);
            f("int_level", r->int_level);
            f("i2c_khz", r->i2c_khz);
        } else if (report == 0x93) {
            auto r = L4::StreamData::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "vl53l4 stream unpack " + nm);
                continue;
            }
            check_eq(r->timestamp_us, ex.at("timestamp_us").as_u64(), "vl53l4 stream ts " + nm);
            check_eq(static_cast<std::int64_t>(r->addr), ex.at("addr").as_int(),
                     "vl53l4 stream addr " + nm);
            check_eq(static_cast<std::int64_t>(r->len), ex.at("len").as_int(),
                     "vl53l4 stream len " + nm);
            check_eq(to_hex(r->data), ex.at("data").as_string(), "vl53l4 stream data " + nm);
        } else {
            check(false, "unknown vl53l4 report " + nm);
        }
        seen(c, f0);
    }

    for (const Value& c : v.at("result_block").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        check_l4_result(L4::parse_result_block(depz::as_bytes(raw)), c.at("expect"),
                        "vl53l4 result " + nm);
        seen(c, f0);
    }

    const Value& timing = v.at("timing");
    const int timing_f0 = g_fails;
    check(!timing.at("encode").as_array().empty() && !timing.at("decode").as_array().empty(),
          "vl53l4 timing encode/decode cases present");
    for (const Value& c : timing.at("encode").as_array()) {
        std::string nm = c.at("name").as_string();
        auto r = L4::range_timing_registers(
            static_cast<std::uint32_t>(c.at("timing_budget_ms").as_int()),
            static_cast<std::uint32_t>(c.at("inter_measurement_ms").as_int()),
            static_cast<std::uint16_t>(c.at("osc_frequency").as_int()),
            static_cast<std::uint16_t>(c.at("clock_pll").as_int()));
        if (!r) {
            check(false, "vl53l4 timing encode " + nm);
            continue;
        }
        check_eq(static_cast<std::int64_t>(r->range_config_a), c.at("range_config_a").as_int(),
                 "vl53l4 timing range_config_a " + nm);
        check_eq(static_cast<std::int64_t>(r->range_config_b), c.at("range_config_b").as_int(),
                 "vl53l4 timing range_config_b " + nm);
        check_eq(static_cast<std::int64_t>(r->intermeasurement_raw),
                 c.at("intermeasurement_raw").as_int(), "vl53l4 timing intermeasurement " + nm);
    }
    for (const Value& c : timing.at("decode").as_array()) {
        std::string nm = c.at("name").as_string();
        auto r = L4::decode_range_timing(
            static_cast<std::uint32_t>(c.at("intermeasurement_raw").as_int()),
            static_cast<std::uint16_t>(c.at("clock_pll").as_int()),
            static_cast<std::uint16_t>(c.at("osc_frequency").as_int()),
            static_cast<std::uint16_t>(c.at("range_config_a").as_int()));
        if (!r) {
            check(false, "vl53l4 timing decode " + nm);
            continue;
        }
        check_eq(static_cast<std::int64_t>(r->timing_budget_ms), c.at("timing_budget_ms").as_int(),
                 "vl53l4 timing budget " + nm);
        check_eq(static_cast<std::int64_t>(r->inter_measurement_ms),
                 c.at("inter_measurement_ms").as_int(), "vl53l4 timing inter " + nm);
    }
    seen_id("vl53l4/timing", timing_f0);

    for (const Value& c : v.at("tuning").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        const int value = static_cast<int>(c.at("value").as_int());
        const auto raw = static_cast<std::uint16_t>(c.at("raw").as_int());
        std::int64_t enc = -1, dec = 0;
        if (kind == "offset") {
            enc = L4::offset_raw(value);
            dec = L4::decode_offset(raw);
        } else if (kind == "xtalk") {
            enc = L4::xtalk_raw(value);
            dec = L4::decode_xtalk(raw);
        } else if (kind == "signal_threshold") {
            enc = L4::signal_threshold_raw(value);
            dec = L4::decode_signal_threshold(raw);
        } else if (kind == "sigma_threshold") {
            auto e = L4::sigma_threshold_raw(value);
            enc = e ? static_cast<std::int64_t>(*e) : -1;
            dec = L4::decode_sigma_threshold(raw);
        } else {
            check(false, "unknown vl53l4 tuning kind " + kind);
            continue;
        }
        check_eq(enc, c.at("raw").as_int(), "vl53l4 tuning encode " + nm);
        check_eq(dec, static_cast<std::int64_t>(value), "vl53l4 tuning decode " + nm);
        seen(c, f0);
    }

    // C round(): an exact .5 goes away from zero (256 -> 1), as ST's GetXtalk
    for (const Value& c : v.at("xtalk_decode").as_array()) {
        const int f0 = g_fails;
        const auto raw = static_cast<std::uint16_t>(c.at("raw").as_int());
        check_eq(static_cast<std::int64_t>(L4::decode_xtalk(raw)), c.at("kcps").as_int(),
                 "vl53l4 xtalk_decode " + std::to_string(raw));
        seen(c, f0);
    }

    const Value& cfg = v.at("config_block");
    const int cb_f0 = g_fails;
    check_eq(static_cast<std::int64_t>(L4::CONFIG_ADDR), cfg.at("addr").as_int(),
             "vl53l4 config_block addr");
    check_eq(to_hex(L4::config_block()), cfg.at("data").as_string(), "vl53l4 config_block data");
    seen_id("vl53l4/config_block", cb_f0);
}

// ---- vl53lx 1D family base (vl53lx.json, contract 12) -----------------------
static std::vector<depz::vl53lx::ClearStep> clear_steps_of(const Value& arr) {
    std::vector<depz::vl53lx::ClearStep> out;
    for (const Value& s : arr.as_array()) {
        const auto& p = s.as_array();
        out.push_back({static_cast<std::uint16_t>(p.at(0).as_int()),
                       static_cast<std::uint8_t>(p.at(1).as_int())});
    }
    return out;
}

static void test_vl53lx() {
    namespace LX = depz::vl53lx;
    Value v = load_vector("vl53lx.json");

    // encode: SET_ADDR_WIDTH, START_STREAM with its clear list, contract-10 codecs.
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        std::optional<depz::bytes> got;
        if (kind == "set_addr_width") {
            got = LX::pack_set_addr_width(static_cast<std::uint8_t>(c.at("width").as_int()));
        } else if (kind == "read_reg") {
            got = LX::pack_read_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                    static_cast<std::uint16_t>(c.at("len").as_int()));
        } else if (kind == "write_reg") {
            depz::bytes data = from_hex(c.at("data").as_string());
            got = LX::pack_write_reg(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                     depz::as_bytes(data));
        } else if (kind == "xshut") {
            got = LX::pack_xshut(static_cast<std::uint8_t>(c.at("action").as_int()));
        } else if (kind == "set_i2c_speed") {
            got = LX::pack_set_i2c_speed(static_cast<std::uint16_t>(c.at("khz").as_int()));
        } else if (kind == "start_stream") {
            got = LX::pack_start_stream(static_cast<std::uint16_t>(c.at("addr").as_int()),
                                        static_cast<std::uint16_t>(c.at("len").as_int()),
                                        clear_steps_of(c.at("clear")),
                                        static_cast<std::uint8_t>(c.at("flags").as_int()));
        } else {
            check(false, "unknown vl53lx encode kind " + kind);
            continue;
        }
        if (!got) {
            check(false, "vl53lx encode refused " + nm);
            continue;
        }
        check_eq(to_hex(*got), c.at("payload").as_string(), "vl53lx encode " + nm);
        seen(c, f0);
    }
    check(!LX::pack_start_stream(0x0089, 17, std::vector<LX::ClearStep>(5, {0x86, 1})),
          "vl53lx start_stream refuses five clear steps");
    check(!LX::pack_set_addr_width(0) && !LX::pack_set_addr_width(3),
          "vl53lx set_addr_width refuses widths other than 1/2");

    // decode: RPT_VL53_INFO (23 B).
    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        check_eq(c.at("report").as_int(), static_cast<std::int64_t>(0x92), "vl53lx report " + nm);
        depz::bytes payload = from_hex(c.at("payload").as_string());
        auto r = LX::Vl53lxInfo::unpack(depz::as_bytes(payload));
        if (!r) {
            check(false, "vl53lx info unpack " + nm);
            continue;
        }
        const Value& ex = c.at("expect");
        check_eq(ex.as_object().size(), static_cast<std::size_t>(10), "vl53lx info field count " + nm);
        auto f = [&](const char* k, std::int64_t got) {
            check_eq(got, ex.at(k).as_int(), std::string("vl53lx info.") + k + " " + nm);
        };
        f("int_edges", r->int_edges);
        f("slots_skipped", r->slots_skipped);
        f("i2c_errors", r->i2c_errors);
        f("last_i2c_error", r->last_i2c_error);
        f("xshut_level", r->xshut_level);
        f("int_level", r->int_level);
        f("i2c_khz", r->i2c_khz);
        f("addr_width", r->addr_width);
        f("n_clear", r->n_clear);
        f("frames_dropped", r->frames_dropped);
        seen(c, f0);
    }
    depz::bytes short_info(LX::INFO_SIZE - 1, std::byte{0});
    check(!LX::Vl53lxInfo::unpack(depz::as_bytes(short_info)).has_value(),
          "vl53lx info rejects a short payload");

    // products: the table, in order.
    const auto& rows = v.at("products").as_array();
    check_eq(LX::products().size(), rows.size(), "vl53lx product count");
    for (std::size_t i = 0; i < rows.size() && i < LX::products().size(); ++i) {
        const int f0 = g_fails;
        const Value& w = rows[i];
        const LX::Product& p = LX::products()[i];
        std::string nm = w.at("product").as_string();
        check_eq(p.name, nm, "vl53lx product order #" + std::to_string(i));
        check(LX::find_product(nm) == &p, "vl53lx find_product " + nm);
        check_eq(static_cast<std::int64_t>(p.model_id), w.at("model_id").as_int(),
                 "vl53lx model_id " + nm);
        check_eq(static_cast<std::int64_t>(p.reach_mm), w.at("reach_mm").as_int(),
                 "vl53lx reach_mm " + nm);
        std::string kinds, want_kinds;
        for (auto k : p.driver_kinds) kinds += LX::to_string(k) + ",";
        for (const Value& k : w.at("driver_kinds").as_array()) want_kinds += k.as_string() + ",";
        check_eq(kinds, want_kinds, "vl53lx driver_kinds " + nm);
        check_eq(LX::to_string(p.default_driver), w.at("default_driver").as_string(),
                 "vl53lx default_driver " + nm);
        check_eq(static_cast<std::int64_t>(p.addr_width), w.at("addr_width").as_int(),
                 "vl53lx addr_width " + nm);
        auto want_clear = clear_steps_of(w.at("clear_steps"));
        bool same = want_clear.size() == p.clear_steps.size();
        for (std::size_t j = 0; same && j < want_clear.size(); ++j) {
            same = want_clear[j].addr == p.clear_steps[j].addr &&
                   want_clear[j].value == p.clear_steps[j].value;
        }
        check(same, "vl53lx clear_steps " + nm);
        check_eq(static_cast<std::int64_t>(p.max_khz), w.at("max_khz").as_int(),
                 "vl53lx max_khz " + nm);
        seen(rows[i], f0);
    }
    check(LX::find_product("VL53L5CX") == nullptr, "vl53lx find_product unknown");

    // model: class + product resolution.
    for (const Value& c : v.at("model").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        std::optional<std::string> usb_model;
        if (!c.at("usb_model").is_null()) usb_model = c.at("usb_model").as_string();
        const std::string& dev = c.at("device_name").as_string();
        check_eq(LX::to_string(LX::resolve_class(usb_model, dev)), c.at("expect_class").as_string(),
                 "vl53lx class " + nm);
        auto prod = LX::product_from_board_name(dev);
        const Value& ep = c.at("expect_product");
        if (ep.is_null()) {
            check(!prod.has_value(), "vl53lx product null " + nm);
        } else {
            check(prod.has_value() && *prod == ep.as_string(), "vl53lx product " + nm);
        }
        seen(c, f0);
    }

    // die_block: both variants.
    for (const Value& c : v.at("die_block").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        std::string var = c.at("variant").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        LX::DieVariant dv = var == "l1" ? LX::DieVariant::L1 : LX::DieVariant::L4;
        check(var == "l1" || var == "l4", "vl53lx die variant known " + nm);
        check_l4_result(LX::decode_die_block(depz::as_bytes(raw), dv), c.at("expect"),
                        "vl53lx die " + nm);
        if (dv == LX::DieVariant::L4) {
            auto a = LX::decode_die_block(depz::as_bytes(raw), dv);
            auto b = depz::vl53l4::parse_result_block(depz::as_bytes(raw));
            check(a && b && a->signal_rate_kcps == b->signal_rate_kcps &&
                      a->signal_per_spad_kcps == b->signal_per_spad_kcps,
                  "vl53lx die l4 == contract-10 decode " + nm);
        }
        depz::bytes cut(raw.begin(), raw.end() - 1);
        check(!LX::decode_die_block(depz::as_bytes(cut), dv).has_value(),
              "vl53lx die rejects 16 bytes " + nm);
        seen(c, f0);
    }

    // l0x_raw.
    for (const Value& c : v.at("l0x_raw").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        auto r = LX::decode_l0x_raw(depz::as_bytes(raw));
        if (!r) {
            check(false, "vl53lx l0x decode " + nm);
            continue;
        }
        const Value& ex = c.at("expect");
        auto f = [&](const char* k, std::int64_t got) {
            check_eq(got, ex.at(k).as_int(), std::string("vl53lx l0x.") + k + " " + nm);
        };
        f("distance_raw", r->distance_raw);
        f("device_range_status", r->device_range_status);
        f("signal_rate_mcps_1616", r->signal_rate_mcps_1616);
        f("ambient_rate_mcps_1616", r->ambient_rate_mcps_1616);
        f("effective_spad_count_88", r->effective_spad_count_88);
        depz::bytes cut(raw.begin(), raw.end() - 1);
        check(!LX::decode_l0x_raw(depz::as_bytes(cut)).has_value(),
              "vl53lx l0x rejects a short block " + nm);
        seen(c, f0);
    }

    // histogram_raw.
    for (const Value& c : v.at("histogram_raw").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        auto r = LX::decode_histogram_raw(depz::as_bytes(raw));
        if (!r) {
            check(false, "vl53lx histogram decode " + nm);
            continue;
        }
        const Value& ex = c.at("expect");
        auto f = [&](const char* k, std::int64_t got) {
            check_eq(got, ex.at(k).as_int(), std::string("vl53lx hist.") + k + " " + nm);
        };
        f("interrupt_status", r->interrupt_status);
        f("range_status", r->range_status);
        f("report_status", r->report_status);
        f("stream_count", r->stream_count);
        f("dss_actual_effective_spads", r->dss_actual_effective_spads);
        f("reference_phase", r->reference_phase);
        f("vcsel_start", r->vcsel_start);
        const auto& wb = ex.at("bins").as_array();
        check_eq(wb.size(), LX::HISTOGRAM_BINS, "vl53lx hist bin count " + nm);
        for (std::size_t i = 0; i < wb.size() && i < LX::HISTOGRAM_BINS; ++i) {
            check_eq(static_cast<std::int64_t>(r->bins[i]), wb[i].as_int(),
                     "vl53lx hist bin " + std::to_string(i) + " " + nm);
        }
        depz::bytes cut(raw.begin(), raw.end() - 1);
        check(!LX::decode_histogram_raw(depz::as_bytes(cut)).has_value(),
              "vl53lx histogram rejects a short block " + nm);
        seen(c, f0);
    }
}

// ---- vl53l7 frame reassembly + decode (recording replay) -------------------
// Full-stack captures from live L5CX / L7CH boards (APP_VL53L7_v0.53). Replay
// the rx side through framing -> reassembly -> the L5/L7 frame decoder (footer
// id at size-4, per-zone arrays trimmed to the resolution — the 4x4 capture
// pins the trim: per-target blocks arrive with 64 entries).
static void replay_vl53l7(const std::string& stem, int resolution, bool with_cnh) {
    const int f0 = g_fails;
    Value exp = testjson::parse(load_recording(stem + ".expected.json"));
    const auto& want_frames = exp.at("frames").as_array();
    check_eq(exp.at("software_name").as_string(), std::string("APP_VL53L7_v0.53"),
             stem + " software_name");

    std::string rec = load_recording(stem + ".depzrec");
    depz::PacketParser parser;
    depz::vl53l8::FrameReassembler reasm;
    std::vector<depz::vl53l8::Vl53l8Frame> frames;
    std::size_t parse_errors = 0;
    std::size_t max_chunk = 0;

    for (const std::string& line : split_lines(rec)) {
        Value o = testjson::parse(line);
        if (!o.has("dir") || o.at("dir").as_string() != "rx") continue;
        depz::bytes data = from_hex(o.at("data").as_string());
        for (const auto& ev : parser.feed(depz::as_bytes(data))) {
            auto* pkt = std::get_if<depz::Packet>(&ev);
            if (!pkt || pkt->cmd != static_cast<std::uint8_t>(depz::vl53l7::Vl53l7Rpt::Frame) ||
                pkt->payload.size() < 12)
                continue;
            auto chunk = depz::vl53l8::FrameChunk::unpack(depz::as_bytes(pkt->payload));
            max_chunk = std::max(max_chunk, chunk.data.size());
            auto done = reasm.feed(chunk);
            if (!done) continue;
            auto decoded = depz::vl53l7::decode_frame(depz::as_bytes(done->second));
            if (!decoded) {
                ++parse_errors;
                continue;
            }
            // The explicit-resolution path must agree with the inferred one.
            auto explicit_res = depz::vl53l7::decode_frame(depz::as_bytes(done->second), resolution);
            check(explicit_res && explicit_res->distance_mm == decoded->distance_mm &&
                      explicit_res->resolution == decoded->resolution,
                  stem + " explicit==inferred resolution");
            decoded->timestamp_us = done->first;
            frames.push_back(std::move(*decoded));
        }
    }

    check_eq(parse_errors, static_cast<std::size_t>(0), stem + " no parse errors");
    check(max_chunk <= depz::vl53l7::STREAM_CHUNK_MAX, stem + " chunks <= STREAM_CHUNK_MAX");
    check(frames.size() >= want_frames.size(),
          stem + " decoded>=expected (" + std::to_string(frames.size()) + ">=" +
              std::to_string(want_frames.size()) + ")");

    std::size_t n = std::min(frames.size(), want_frames.size());
    for (std::size_t k = 0; k < n; ++k) {
        const auto& g = frames[k];
        const Value& w = want_frames[k];
        std::string nm = stem + " frame#" + std::to_string(k);
        check_eq(static_cast<std::int64_t>(g.timestamp_us), w.at("timestamp_us").as_int(),
                 nm + " ts");
        check_eq(static_cast<std::int64_t>(g.resolution), w.at("resolution").as_int(),
                 nm + " resolution");
        check_eq(static_cast<std::int64_t>(g.resolution), static_cast<std::int64_t>(resolution),
                 nm + " resolution==capture");
        check_eq(static_cast<std::int64_t>(g.silicon_temp_degc),
                 w.at("silicon_temp_degc").as_int(), nm + " temp");
        const auto& wd = w.at("distance_mm").as_array();
        const auto& ws = w.at("target_status").as_array();
        const auto& wn = w.at("nb_target_detected").as_array();
        bool ok = g.distance_mm.size() == wd.size() && g.target_status.size() == ws.size() &&
                  g.nb_target_detected.size() == wn.size() &&
                  wd.size() == static_cast<std::size_t>(resolution);
        for (std::size_t z = 0; ok && z < wd.size(); ++z) {
            if (static_cast<std::int64_t>(g.distance_mm[z]) != wd[z].as_int()) ok = false;
            if (static_cast<std::int64_t>(g.target_status[z]) != ws[z].as_int()) ok = false;
            if (static_cast<std::int64_t>(g.nb_target_detected[z]) != wn[z].as_int()) ok = false;
        }
        check(ok, nm + " per-zone arrays");
        bool trimmed = g.signal_per_spad.size() == static_cast<std::size_t>(resolution) &&
                       g.ambient_per_spad.size() == static_cast<std::size_t>(resolution) &&
                       g.nb_spads_enabled.size() == static_cast<std::size_t>(resolution) &&
                       g.range_sigma_mm.size() == static_cast<std::size_t>(resolution) &&
                       g.reflectance.size() == static_cast<std::size_t>(resolution);
        check(trimmed, nm + " all per-zone arrays trimmed to resolution");
        if (with_cnh) {
            check(g.cnh_raw.has_value() && w.has("cnh_raw") &&
                      to_hex(*g.cnh_raw) == w.at("cnh_raw").as_string(),
                  nm + " cnh_raw exact");
        } else {
            check(!g.cnh_raw.has_value() && !w.has("cnh_raw"), nm + " no cnh_raw");
        }
    }
    seen_id("recordings/" + stem + ".depzrec", f0);
}

static void test_vl53l7_frame() {
    replay_vl53l7("vl53l5cx_8x8_15hz_3s", depz::vl53l8::RESOLUTION_8X8, false);
    replay_vl53l7("vl53l5cx_4x4_15hz", depz::vl53l8::RESOLUTION_4X4, false);
    replay_vl53l7("vl53l7ch_8x8_15hz_3s", depz::vl53l8::RESOLUTION_8X8, false);
    replay_vl53l7("vl53l7ch_cnh_8x8_15hz", depz::vl53l8::RESOLUTION_8X8, true);
}

// ---- bno086 SHTP framing ---------------------------------------------------
static void test_bno086_shtp() {
    Value v = load_vector("bno086_shtp.json");

    for (const Value& h : v.at("header").as_array()) {
        const int f0 = g_fails;
        std::string nm = h.at("name").as_string();
        depz::bno086::ShtpHeader hdr;
        hdr.length = static_cast<std::uint16_t>(h.at("length").as_int());
        hdr.channel = static_cast<std::uint8_t>(h.at("channel").as_int());
        hdr.seq = static_cast<std::uint8_t>(h.at("seq").as_int());
        hdr.continuation = h.at("continuation").as_bool();
        check_eq(to_hex(hdr.pack()), h.at("bytes").as_string(), "shtp header pack " + nm);
        auto ud = depz::bno086::ShtpHeader::unpack(depz::as_bytes(from_hex(h.at("bytes").as_string())));
        check(ud.length == hdr.length && ud.channel == hdr.channel && ud.seq == hdr.seq &&
                  ud.continuation == hdr.continuation,
              "shtp header unpack " + nm);
        seen(h, f0);
    }

    // Per-channel TX sequence counters.
    depz::bno086::ShtpLayer tx;
    int idx = 0;
    for (const Value& t : v.at("tx_seq").as_array()) {
        const int f0 = g_fails;
        depz::bytes payload = from_hex(t.at("payload").as_string());
        int channel = static_cast<int>(t.at("channel").as_int());
        check_eq(to_hex(tx.next_frame(channel, depz::as_bytes(payload))),
                 t.at("frame").as_string(), "shtp tx_seq #" + std::to_string(idx++));
        seen(t, f0);
    }

    // Regression (difftest crash): a channel outside 0..5 indexed the
    // per-channel seq table out of bounds; it is refused, the table untouched.
    {
        depz::bno086::ShtpLayer l;
        const depz::bytes one{std::byte{0xF9}};
        for (int ch : {6, 255, -1, 1 << 20}) {
            bool threw = false;
            try {
                (void)l.next_frame(ch, depz::as_bytes(one));
            } catch (const std::out_of_range&) {
                threw = true;
            }
            check(threw, "shtp next_frame refuses channel " + std::to_string(ch));
        }
        check(l.tx_seq(0) == 0 && l.tx_seq(5) == 0, "shtp seq table untouched by refused channels");
        depz::bno086::ShtpHeader big;
        big.length = 0x8000;
        bool threw = false;
        try {
            (void)big.pack();
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "shtp header length over 15 bits refused");
    }

    // RX reassembly.
    for (const Value& c : v.at("reassembly").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bno086::ShtpLayer rx;
        std::vector<depz::bno086::ShtpCargo> got;
        const auto& frames = c.at("frames").as_array();
        const auto& captures = c.at("captures_us").as_array();
        check_eq(captures.size(), frames.size(), "shtp reassembly captures_us " + nm);
        for (std::size_t j = 0; j < frames.size() && j < captures.size(); ++j) {
            depz::bytes frame = from_hex(frames[j].as_string());
            auto cargo = rx.feed(depz::as_bytes(frame), static_cast<std::uint64_t>(captures[j].as_int()));
            if (cargo) got.push_back(std::move(*cargo));
        }
        const Value& e = c.at("expect");
        const auto& want = e.at("cargos").as_array();
        check_eq(got.size(), want.size(), "shtp reassembly cargo-count " + nm);
        std::size_t n = std::min(got.size(), want.size());
        for (std::size_t k = 0; k < n; ++k) {
            check_eq(static_cast<std::int64_t>(got[k].channel), want[k].at("channel").as_int(),
                     "shtp reassembly channel " + nm + " #" + std::to_string(k));
            check_eq(static_cast<std::int64_t>(got[k].seq), want[k].at("seq").as_int(),
                     "shtp reassembly seq " + nm + " #" + std::to_string(k));
            check_eq(to_hex(got[k].payload), want[k].at("payload").as_string(),
                     "shtp reassembly payload " + nm + " #" + std::to_string(k));
            check_eq(static_cast<std::int64_t>(got[k].capture_us), want[k].at("capture_us").as_int(),
                     "shtp reassembly capture_us " + nm + " #" + std::to_string(k));
        }
        check_eq(static_cast<std::int64_t>(rx.discarded), e.at("discarded").as_int(),
                 "shtp reassembly discarded " + nm);
        seen(c, f0);
    }

    // SH-2 control-channel encoders.
    for (const Value& c : v.at("control_encode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        std::string kind = c.at("kind").as_string();
        depz::bytes got;
        if (kind == "set_feature") {
            got = depz::bno086::sh2_build_set_feature(
                static_cast<std::uint8_t>(c.at("sensor_id").as_int()),
                static_cast<std::uint32_t>(c.at("interval_us").as_u64()),
                static_cast<std::uint32_t>(c.at("batch_us").as_u64()),
                static_cast<std::uint16_t>(c.at("sensitivity").as_int()),
                static_cast<std::uint8_t>(c.at("flags").as_int()),
                static_cast<std::uint32_t>(c.at("cfg_word").as_u64()));
        } else if (kind == "get_feature_request") {
            got = depz::bno086::sh2_build_get_feature_request(
                static_cast<std::uint8_t>(c.at("sensor_id").as_int()));
        } else if (kind == "product_id_request") {
            got = depz::bno086::sh2_build_product_id_request();
        } else if (kind == "command_request") {
            depz::bytes params = from_hex(c.at("params").as_string());
            got = depz::bno086::sh2_build_command_request(
                static_cast<std::uint8_t>(c.at("seq").as_int()),
                static_cast<std::uint8_t>(c.at("command").as_int()), depz::as_bytes(params));
        } else if (kind == "frs_read_request") {
            got = depz::bno086::sh2_build_frs_read_request(
                static_cast<std::uint16_t>(c.at("frs_type").as_int()),
                static_cast<std::uint16_t>(c.at("offset_words").as_int()),
                static_cast<std::uint16_t>(c.at("block_words").as_int()));
        } else if (kind == "frs_write_request") {
            got = depz::bno086::sh2_build_frs_write_request(
                static_cast<std::uint16_t>(c.at("frs_type").as_int()),
                static_cast<std::uint16_t>(c.at("length_words").as_int()));
        } else if (kind == "frs_write_data") {
            std::vector<std::uint32_t> words;
            for (const Value& w : c.at("words").as_array())
                words.push_back(static_cast<std::uint32_t>(w.as_u64()));
            got = depz::bno086::sh2_build_frs_write_data(
                static_cast<std::uint16_t>(c.at("offset_words").as_int()), words);
        } else {
            check(false, "shtp control unknown kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "shtp control " + nm);
        seen(c, f0);
    }

    // Executable channel: sleep / power_on (contract 05 §6a), fresh layer → seq 0, 1.
    depz::bno086::ShtpLayer exec;
    std::size_t n_exec = 0;
    for (const Value& c : v.at("executable").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bno086::ExecutableCommand command;
        if (nm == "sleep") {
            command = depz::bno086::ExecutableCommand::Sleep;
        } else if (nm == "power_on") {
            command = depz::bno086::ExecutableCommand::On;
        } else {
            check(false, "shtp executable unknown command " + nm);
            continue;
        }
        check_eq(static_cast<std::int64_t>(command), c.at("command").as_int(), "shtp executable command " + nm);
        check_eq(c.at("channel").as_int(),
                 static_cast<std::int64_t>(depz::bno086::ShtpChannel::Executable),
                 "shtp executable channel " + nm);
        depz::bytes payload{static_cast<std::byte>(command)};
        check_eq(to_hex(payload), c.at("payload").as_string(), "shtp executable payload " + nm);
        depz::bytes frame = exec.next_frame(static_cast<int>(depz::bno086::ShtpChannel::Executable),
                                            depz::as_bytes(payload));
        check_eq(to_hex(frame), c.at("shtp_frame").as_string(), "shtp executable frame " + nm);
        check_eq(c.at("bridge_cmd").as_int(), std::int64_t{0x34}, "shtp executable bridge cmd " + nm);
        depz::bytes bridge = depz::build_packet(
            static_cast<std::uint8_t>(c.at("bridge_cmd").as_int()), depz::as_bytes(frame),
            static_cast<std::uint8_t>(c.at("bridge_seq").as_int()),
            static_cast<depz::CrcType>(c.at("crc_type").as_int()));
        check_eq(to_hex(bridge), c.at("bridge_frame").as_string(), "shtp executable bridge frame " + nm);
        n_exec++;
        seen(c, f0);
    }
    check_eq(n_exec, std::size_t{2}, "shtp executable case count");

    // Set Feature flag bits (depz/device.hpp).
#ifdef DEPZ_TEST_HAS_IO
    {
        const int f0 = g_fails;
        const Value& ff = v.at("feature_flags");
        check_eq(ff.as_object().size(), std::size_t{2}, "shtp feature_flags: wakeup + always_on");
        check_eq(ff.at("wakeup").as_int(), static_cast<std::int64_t>(depz::bno086::FEATURE_WAKEUP),
                 "shtp feature flag wakeup");
        check_eq(ff.at("always_on").as_int(), static_cast<std::int64_t>(depz::bno086::FEATURE_ALWAYS_ON),
                 "shtp feature flag always_on");
        seen_id("bno086_shtp/feature_flags", f0);
    }
#endif
}

// ---- bno086 report parsers -------------------------------------------------
static void check_report(const depz::bno086::Report& r, const Value& e, const std::string& nm) {
    check_eq(report_type_name(r.type), e.at("type").as_string(), nm + " type");
    const Value& f = e.at("fields");
    auto ck = [&](const char* key, std::int64_t got) {
        if (f.has(key)) check_eq(got, f.at(key).as_int(), nm + " " + key);
    };
    ck("sensor_id", r.sensor_id);
    ck("timestamp_us", r.timestamp_us);
    ck("seq", r.seq);
    ck("accuracy", r.accuracy);
    ck("delay_us", r.delay_us);
    ck("x_raw", r.x_raw);
    ck("y_raw", r.y_raw);
    ck("z_raw", r.z_raw);
    ck("bias_x_raw", r.bias_x_raw);
    ck("bias_y_raw", r.bias_y_raw);
    ck("bias_z_raw", r.bias_z_raw);
    ck("i_raw", r.i_raw);
    ck("j_raw", r.j_raw);
    ck("k_raw", r.k_raw);
    ck("real_raw", r.real_raw);
    ck("accuracy_raw", r.accuracy_raw);
    ck("value_raw", r.value_raw);
    ck("flags", r.flags);
    ck("latency_us", r.latency_us);
    ck("steps", r.steps);
    ck("motion", r.motion);
    ck("classification", r.classification);
    ck("page_number", r.page_number);
    ck("end_of_sequence", r.end_of_sequence ? 1 : 0);
    ck("most_likely_state", r.most_likely_state);
    ck("intent", r.intent);
    ck("request", r.request);
    ck("flushed_sensor", r.flushed_sensor);
    ck("sensor_timestamp_us", r.sensor_timestamp_us);
    ck("temperature_raw", r.temperature_raw);
    ck("vx_raw", r.vx_raw);
    ck("vy_raw", r.vy_raw);
    ck("vz_raw", r.vz_raw);
    if (f.has("confidences")) {
        const auto& wc = f.at("confidences").as_array();
        bool ok = r.confidences.size() == wc.size();
        for (std::size_t z = 0; ok && z < wc.size(); ++z)
            if (static_cast<std::int64_t>(r.confidences[z]) != wc[z].as_int()) ok = false;
        check(ok, nm + " confidences");
    }
    if (f.has("data")) check_eq(to_hex(r.data), f.at("data").as_string(), nm + " data");
}

#ifdef DEPZ_TEST_HAS_IO
// `record`: the contract 09 dataset value, i.e. the report in SI units (raw /
// 2^Q, exact in binary). Its integer keys repeat `fields` (check_report); here
// the scaled ones go through the SDK's scalers (depz::Bno086Report).
static void check_report_record(const depz::bno086::Report& raw, const Value& rec, const std::string& nm) {
    const depz::Bno086Report r{raw};
    check_eq(report_type_name(r.type), rec.at("type").as_string(), nm + " record type");
    auto three = [&](const char* key, const std::array<double, 3>& got) {
        const auto& want = rec.at(key).as_array();
        check_eq(want.size(), std::size_t{3}, nm + " record " + key + " size");
        for (std::size_t k = 0; k < 3 && k < want.size(); ++k)
            check_eq(got[k], want[k].as_double(), nm + " record " + key + "[" + std::to_string(k) + "]");
    };
    if (rec.has("x")) {
        const auto v = r.xyz();
        check_eq(v[0], rec.at("x").as_double(), nm + " record x");
        check_eq(v[1], rec.at("y").as_double(), nm + " record y");
        check_eq(v[2], rec.at("z").as_double(), nm + " record z");
    }
    if (rec.has("bias")) three("bias", r.bias());
    if (rec.has("i")) {
        const auto q = r.quaternion();
        check_eq(q[0], rec.at("i").as_double(), nm + " record i");
        check_eq(q[1], rec.at("j").as_double(), nm + " record j");
        check_eq(q[2], rec.at("k").as_double(), nm + " record k");
        check_eq(q[3], rec.at("real").as_double(), nm + " record real");
        if (r.type != depz::bno086::ReportType::GyroIntegratedRV) {
            // the game variants carry no heading accuracy: the key is absent
            const auto acc = r.accuracy_rad();
            check_eq(acc.has_value(), rec.has("accuracy_rad"), nm + " record accuracy_rad presence");
            if (acc && rec.has("accuracy_rad"))
                check_eq(*acc, rec.at("accuracy_rad").as_double(), nm + " record accuracy_rad");
        }
    }
    if (rec.has("value")) check_eq(r.scalar(), rec.at("value").as_double(), nm + " record value");
    if (rec.has("angular_velocity")) three("angular_velocity", r.angular_velocity());
}
#endif

static void test_bno086_reports() {
    Value v = load_vector("bno086_reports.json");
    // A case is reported only with its `record` checked too: that needs the
    // SI scalers of depz/device.hpp. interactive_and_flush: MotionRequest
    // (0x2B) and the bare 0xEF Flush Completed, in the same shape.
    std::vector<Value> cargos = v.at("input_cargos").as_array();
    for (const Value& c : v.at("interactive_and_flush").as_array()) cargos.push_back(c);
    for (const Value& c : cargos) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes cargo = from_hex(c.at("cargo").as_string());
        auto reports = depz::bno086::parse_input_cargo(
            depz::as_bytes(cargo), c.at("capture_timestamp_us").as_int());
        const auto& want = c.at("expect").as_array();
        check_eq(reports.size(), want.size(), "bno086 reports count " + nm);
        std::size_t n = std::min(reports.size(), want.size());
        for (std::size_t k = 0; k < n; ++k) {
            check_report(reports[k], want[k], nm + " #" + std::to_string(k));
#ifdef DEPZ_TEST_HAS_IO
            check_report_record(reports[k], want[k].at("record"), nm + " #" + std::to_string(k));
#endif
        }
#ifdef DEPZ_TEST_HAS_IO
        seen(c, f0);
#else
        (void)f0;
#endif
    }
    for (const Value& c : v.at("gyro_rv").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes cargo = from_hex(c.at("cargo").as_string());
        auto r = depz::bno086::parse_gyro_rv_cargo(depz::as_bytes(cargo),
                                                   c.at("capture_timestamp_us").as_int());
        if (!r) {
            check(false, "bno086 gyro_rv parsed " + nm);
            continue;
        }
        check_report(*r, c.at("expect"), "bno086 gyro_rv " + nm);
#ifdef DEPZ_TEST_HAS_IO
        check_report_record(*r, c.at("expect").at("record"), "bno086 gyro_rv " + nm);
        seen(c, f0);
#else
        (void)f0;
#endif
    }

    // Q points: the per-sensor table, and nothing beyond it (every other id is
    // an event report); then the two fixed ones.
#ifdef DEPZ_TEST_HAS_IO
    {
        const int f0 = g_fails;
        const auto& qp = v.at("q_points").as_object();
        check(!qp.empty(), "bno086 q_points present");
        std::size_t in_table = 0;
        for (unsigned sid = 0; sid < 256; ++sid) {
            char key[8];
            std::snprintf(key, sizeof key, "0x%02X", sid);
            const auto q = depz::bno086::q_point(static_cast<std::uint8_t>(sid));
            const auto it = qp.find(key);
            if (it == qp.end()) {
                check(!q.has_value(), std::string("bno086 q_point(") + key + ") not in the table");
            } else {
                in_table++;
                check(q && *q == it->second.as_int(), std::string("bno086 q_point(") + key + ")");
            }
        }
        check_eq(in_table, qp.size(), "bno086 q_points keys are 0x%02X sensor ids");
        seen_id("bno086_reports/q_points", f0);
    }
#endif
    {
        int f0 = g_fails;
        check_eq(v.at("rv_accuracy_q").as_int(), static_cast<std::int64_t>(depz::bno086::RV_ACCURACY_Q),
                 "bno086 rv_accuracy_q");
        seen_id("bno086_reports/rv_accuracy_q", f0);
        f0 = g_fails;
        check_eq(v.at("gyro_rv_angvel_q").as_int(), static_cast<std::int64_t>(depz::bno086::GYRO_RV_ANGVEL_Q),
                 "bno086 gyro_rv_angvel_q");
        seen_id("bno086_reports/gyro_rv_angvel_q", f0);
    }
}

// ---- dataset read (.depzdata, contract 09) ---------------------------------
static void test_dataset() {
    const int f0 = g_fails;
    std::string content = load_recording("dataset_dual_sr04.depzdata");
    depz::dataset::Reader ds(content);

    check_eq(ds.schema, std::string("depz.dataset/1"), "dataset schema");
    check_eq(ds.note, std::string("dual fake SR04 fixture"), "dataset note");
    check_eq(ds.devices.size(), static_cast<std::size_t>(2), "dataset device-count");

    // Device metadata.
    auto d0 = ds.devices.find("d0");
    auto d1 = ds.devices.find("d1");
    check(d0 != ds.devices.end() && d1 != ds.devices.end(), "dataset devices d0/d1 present");
    if (d0 != ds.devices.end()) {
        check_eq(d0->second.serial, std::string("SN0042"), "dataset d0 serial");
        check_eq(d0->second.sensor_type, std::string("sr04"), "dataset d0 sensor_type");
        check_eq(d0->second.software_name, std::string("APP_usonic_SR04_v0.95"),
                 "dataset d0 software_name");
        check_eq(d0->second.time_sync.offset_us, INT64_C(-8524738852),
                 "dataset d0 offset_us");
        check_eq(d0->second.time_sync.rtt_us, static_cast<std::int64_t>(13), "dataset d0 rtt_us");
    }
    if (d1 != ds.devices.end())
        check_eq(d1->second.time_sync.offset_us, INT64_C(-3525739108),
                 "dataset d1 offset_us");

    // Records: merged by host time (stable), 10 SR04 entries.
    check_eq(ds.records.size(), static_cast<std::size_t>(10), "dataset record-count");
    bool sorted = true;
    for (std::size_t k = 1; k < ds.records.size(); ++k)
        if (ds.records[k].t_host_us < ds.records[k - 1].t_host_us) sorted = false;
    check(sorted, "dataset records sorted by host time");

    if (!ds.records.empty()) {
        const auto& r0 = ds.records[0];
        check_eq(r0.device_id, std::string("d0"), "dataset r0 device");
        check_eq(r0.kind, std::string("sr04"), "dataset r0 kind");
        check_eq(r0.t_host_us, static_cast<std::int64_t>(8525788852), "dataset r0 t");
        auto echo = r0.ints.find("echo_us");
        check(echo != r0.ints.end() && echo->second == 5831, "dataset r0 echo_us");
        auto src = r0.strings.find("source");
        check(src != r0.strings.end() && src->second == "loop", "dataset r0 source");
    }
    // A record with source=="once" exists (d1 second sample).
    bool has_once = false;
    for (const auto& r : ds.records) {
        auto s = r.strings.find("source");
        if (s != r.strings.end() && s->second == "once") has_once = true;
    }
    check(has_once, "dataset has a 'once' source record");

    // Record for record against the file itself: the header's devices, then
    // every line (already host-time sorted) as one record, values included.
    const auto lines = split_lines(content);
    check(!lines.empty(), "dataset has a header line");
    if (!lines.empty()) {
        const Value hdr = testjson::parse(lines[0]);
        const auto& devs = hdr.at("devices").as_object();
        check_eq(devs.size(), ds.devices.size(), "dataset header devices");
        for (const auto& kv : devs) {
            auto it = ds.devices.find(kv.first);
            check(it != ds.devices.end(), "dataset device " + kv.first);
            if (it == ds.devices.end()) continue;
            const Value& w = kv.second;
            check(it->second.serial == w.at("serial").as_string() &&
                      it->second.sensor_type == w.at("sensor_type").as_string() &&
                      it->second.software_name == w.at("software_name").as_string() &&
                      it->second.time_sync.offset_us == w.at("time_sync").at("offset_us").as_int() &&
                      it->second.time_sync.rtt_us == w.at("time_sync").at("rtt_us").as_int(),
                  "dataset device " + kv.first + " metadata");
        }
        check_eq(lines.size() - 1, ds.records.size(), "dataset: one record per line");
        for (std::size_t k = 1; k < lines.size() && k - 1 < ds.records.size(); ++k) {
            const Value w = testjson::parse(lines[k]);
            const auto& r = ds.records[k - 1];
            const std::string tag = "dataset record " + std::to_string(k - 1);
            check(r.device_id == w.at("d").as_string() && r.t_host_us == w.at("t").as_int() &&
                      r.kind == w.at("k").as_string(),
                  tag);
            const Value& val = w.at("v");
            auto echo = r.ints.find("echo_us");
            auto src = r.strings.find("source");
            check(val.as_object().size() == 2 && r.ints.size() == 1 && r.strings.size() == 1 &&
                      r.arrays.empty() && echo != r.ints.end() &&
                      echo->second == val.at("echo_us").as_int() && src != r.strings.end() &&
                      src->second == val.at("source").as_string(),
                  tag + " value");
        }
    }
    seen_id("recordings/dataset_dual_sr04.depzdata", f0);
}

// ---- BNO055 register bridge (bno055.json, contract 13) ----------------------
// Consumes every section: encode / decode / units / calib_stat /
// calibration_profile / axis_remap / axis_remap_invalid / sensor_config / blocks.
template <std::size_t N>
static void check_bno_words(const std::optional<std::array<std::int16_t, N>>& got,
                            const Value& want, const std::string& what) {
    if (want.is_null()) {
        check(!got.has_value(), what + " absent");
        return;
    }
    if (!got) {
        check(false, what + " present");
        return;
    }
    check_eq(want.as_array().size(), N, what + " size");
    for (std::size_t k = 0; k < N && k < want.as_array().size(); ++k)
        check_eq(static_cast<std::int64_t>((*got)[k]), want.as_array()[k].as_int(),
                 what + "[" + std::to_string(k) + "]");
}

template <class T>
static void check_bno_scalar(const std::optional<T>& got, const Value& want,
                             const std::string& what) {
    if (want.is_null()) {
        check(!got.has_value(), what + " absent");
        return;
    }
    if (!got) {
        check(false, what + " present");
        return;
    }
    check_eq(static_cast<std::int64_t>(*got), want.as_int(), what);
}

static void test_bno055() {
    namespace B = depz::bno055;
    Value v = load_vector("bno055.json");

    // encode: 0x32..0x37 payloads.
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        std::string kind = c.at("kind").as_string();
        std::string nm = c.at("name").as_string();
        depz::bytes got;
        if (kind == "read_reg") {
            got = B::pack_read_reg(static_cast<std::uint8_t>(c.at("addr").as_int()),
                                   static_cast<std::uint8_t>(c.at("len").as_int()));
        } else if (kind == "write_reg") {
            depz::bytes data = from_hex(c.at("data").as_string());
            got = B::pack_write_reg(static_cast<std::uint8_t>(c.at("addr").as_int()),
                                    depz::as_bytes(data));
        } else if (kind == "start_stream") {
            got = B::pack_start_stream(static_cast<std::uint8_t>(c.at("trigger").as_int()),
                                       static_cast<std::uint8_t>(c.at("addr").as_int()),
                                       static_cast<std::uint8_t>(c.at("len").as_int()),
                                       static_cast<std::uint16_t>(c.at("period_ms").as_int()));
        } else if (kind == "reset" || kind == "stop_stream" || kind == "get_info") {
            got = {};  // empty payload
        } else {
            check(false, "unknown bno055 encode kind " + kind);
            continue;
        }
        check_eq(to_hex(got), c.at("payload").as_string(), "bno055 encode " + nm);
        seen(c, f0);
    }

    // decode: RPT_BNO_INFO 0x92, RPT_BNO_REG_DATA 0x91, RPT_BNO_REG_STREAM 0x93.
    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        std::string nm = c.at("name").as_string();
        depz::bytes payload = from_hex(c.at("payload").as_string());
        const Value& ex = c.at("expect");
        const std::int64_t report = c.at("report").as_int();
        if (report == static_cast<std::int64_t>(B::Bno055Rpt::Info)) {
            auto r = B::Bno055Info::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "bno055 info unpack " + nm);
                continue;
            }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(20),
                     "bno055 info field count " + nm);
            auto f = [&](const char* k, std::int64_t got) {
                check_eq(got, ex.at(k).as_int(), std::string("bno055 info.") + k + " " + nm);
            };
            f("i2c_addr", r->i2c_addr);
            f("chip_id", r->chip_id);
            f("acc_id", r->acc_id);
            f("mag_id", r->mag_id);
            f("gyr_id", r->gyr_id);
            f("sw_rev", r->sw_rev);
            f("bl_rev", r->bl_rev);
            f("initialized", r->initialized);
            f("int_level", r->int_level);
            f("int_edges", r->int_edges);
            f("read_min_us", r->read_min_us);
            f("read_max_us", r->read_max_us);
            f("read_avg_us", r->read_avg_us);
            f("tx_dropped", r->tx_dropped);
            f("i2c_errors", r->i2c_errors);
            f("slots_skipped", r->slots_skipped);
            f("bus_recoveries", r->bus_recoveries);
            f("last_i2c_error", r->last_i2c_error);
            f("sensor_resets", r->sensor_resets);
            f("loop_max_us", r->loop_max_us);
        } else if (report == static_cast<std::int64_t>(B::Bno055Rpt::RegData)) {
            auto r = B::RegData::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "bno055 reg_data unpack " + nm);
                continue;
            }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(3),
                     "bno055 reg_data field count " + nm);
            check_eq(static_cast<std::int64_t>(r->cmd), ex.at("cmd").as_int(), "bno055 cmd " + nm);
            check_eq(r->timestamp_us, ex.at("timestamp_us").as_u64(), "bno055 ts " + nm);
            check_eq(to_hex(r->data), ex.at("data").as_string(), "bno055 data " + nm);
        } else if (report == static_cast<std::int64_t>(B::Bno055Rpt::Stream)) {
            auto r = B::StreamData::unpack(depz::as_bytes(payload));
            if (!r) {
                check(false, "bno055 stream unpack " + nm);
                continue;
            }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(4),
                     "bno055 stream field count " + nm);
            check_eq(r->timestamp_us, ex.at("timestamp_us").as_u64(), "bno055 stream ts " + nm);
            check_eq(static_cast<std::int64_t>(r->addr), ex.at("addr").as_int(),
                     "bno055 stream addr " + nm);
            check_eq(static_cast<std::int64_t>(r->len), ex.at("len").as_int(),
                     "bno055 stream len " + nm);
            check_eq(to_hex(r->data), ex.at("data").as_string(), "bno055 stream data " + nm);
        } else {
            check(false, "unknown bno055 report " + nm);
        }
        seen(c, f0);
    }
    depz::bytes short_info(B::INFO_SIZE - 1, std::byte{0});
    check(!B::Bno055Info::unpack(depz::as_bytes(short_info)).has_value(),
          "bno055 info rejects a short payload");
    depz::bytes short_hdr(8, std::byte{0});
    check(!B::RegData::unpack(depz::as_bytes(short_hdr)).has_value(),
          "bno055 reg_data rejects 8 bytes");
    check(!B::StreamData::unpack(depz::as_bytes(short_hdr)).has_value(),
          "bno055 stream rejects 8 bytes");

    // units: UNIT_SEL flags; the repack drops undefined bits.
    for (const Value& c : v.at("units").as_array()) {
        const int f0 = g_fails;
        const auto sel = static_cast<std::uint8_t>(c.at("unit_sel").as_int());
        const std::string nm = "bno055 units " + std::to_string(sel);
        const Value& ex = c.at("expect");
        B::Units u = B::Units::unpack(sel);
        check_eq(ex.as_object().size(), static_cast<std::size_t>(5), nm + " field count");
        check_eq(u.accel_mg, ex.at("accel_mg").as_bool(), nm + " accel_mg");
        check_eq(u.gyro_rps, ex.at("gyro_rps").as_bool(), nm + " gyro_rps");
        check_eq(u.euler_rad, ex.at("euler_rad").as_bool(), nm + " euler_rad");
        check_eq(u.temp_f, ex.at("temp_f").as_bool(), nm + " temp_f");
        check_eq(u.android, ex.at("android").as_bool(), nm + " android");
        check_eq(static_cast<std::int64_t>(u.pack()), c.at("repack").as_int(), nm + " repack");
        // LSB constants follow the flags (§4.2).
        check_eq(u.accel_lsb(), u.accel_mg ? 1.0 : 100.0, nm + " accel_lsb");
        check_eq(u.gyro_lsb(), u.gyro_rps ? 900.0 : 16.0, nm + " gyro_lsb");
        check_eq(u.euler_lsb(), u.euler_rad ? 900.0 : 16.0, nm + " euler_lsb");
        check_eq(u.temp_lsb(), u.temp_f ? 0.5 : 1.0, nm + " temp_lsb");
        seen(c, f0);
    }
    check(B::MAG_LSB == 16.0 && B::QUAT_LSB == 16384.0 && B::FUSION_ACCEL_LSB == 100.0,
          "bno055 fixed LSBs");

    // calib_stat: 2-bit fields, fully_calibrated, exact repack.
    for (const Value& c : v.at("calib_stat").as_array()) {
        const int f0 = g_fails;
        const auto val = static_cast<std::uint8_t>(c.at("value").as_int());
        const std::string nm = "bno055 calib_stat " + std::to_string(val);
        const Value& ex = c.at("expect");
        B::CalibStatus s = B::CalibStatus::unpack(val);
        check_eq(ex.as_object().size(), static_cast<std::size_t>(4), nm + " field count");
        check_eq(static_cast<std::int64_t>(s.system), ex.at("system").as_int(), nm + " system");
        check_eq(static_cast<std::int64_t>(s.gyro), ex.at("gyro").as_int(), nm + " gyro");
        check_eq(static_cast<std::int64_t>(s.accel), ex.at("accel").as_int(), nm + " accel");
        check_eq(static_cast<std::int64_t>(s.mag), ex.at("mag").as_int(), nm + " mag");
        check_eq(s.fully_calibrated(), c.at("fully_calibrated").as_bool(), nm + " fully_calibrated");
        check_eq(static_cast<std::int64_t>(s.pack()), static_cast<std::int64_t>(val), nm + " repack");
        seen(c, f0);
    }

    // calibration_profile: 11 x i16 LE, byte-exact repack.
    for (const Value& c : v.at("calibration_profile").as_array()) {
        const int f0 = g_fails;
        const std::string nm = "bno055 calibration_profile " + c.at("name").as_string();
        const std::string hex = c.at("bytes").as_string();
        depz::bytes raw = from_hex(hex);
        const Value& ex = c.at("expect");
        auto p = B::CalibrationProfile::unpack(depz::as_bytes(raw));
        if (!p) {
            check(false, nm + " unpack");
            continue;
        }
        check_eq(ex.as_object().size(), static_cast<std::size_t>(5), nm + " field count");
        check_bno_words<3>(p->accel_offset, ex.at("accel_offset"), nm + " accel_offset");
        check_bno_words<3>(p->mag_offset, ex.at("mag_offset"), nm + " mag_offset");
        check_bno_words<3>(p->gyro_offset, ex.at("gyro_offset"), nm + " gyro_offset");
        check_eq(static_cast<std::int64_t>(p->accel_radius), ex.at("accel_radius").as_int(),
                 nm + " accel_radius");
        check_eq(static_cast<std::int64_t>(p->mag_radius), ex.at("mag_radius").as_int(),
                 nm + " mag_radius");
        check_eq(to_hex(p->pack()), hex, nm + " repack");
        check(!B::CalibrationProfile::unpack(depz::byte_span(raw.data(), raw.size() - 1)),
              nm + " rejects 21 bytes");
        seen(c, f0);
    }

    // axis_remap: placements P0..P7 — unpack, repack, placement() and the table.
    const auto& remaps = v.at("axis_remap").as_array();
    check_eq(remaps.size(), B::PLACEMENTS.size(), "bno055 axis_remap placement count");
    for (std::size_t i = 0; i < remaps.size(); ++i) {
        const int f0 = g_fails;
        const Value& c = remaps[i];
        const std::string name = c.at("name").as_string();
        const std::string nm = "bno055 axis_remap " + name;
        const auto cfg = static_cast<std::uint8_t>(c.at("config").as_int());
        const auto sgn = static_cast<std::uint8_t>(c.at("sign").as_int());
        const Value& ex = c.at("expect");
        B::AxisRemap a = B::AxisRemap::unpack(cfg, sgn);
        check_eq(ex.as_object().size(), static_cast<std::size_t>(6), nm + " field count");
        check_eq(static_cast<std::int64_t>(a.x), ex.at("x").as_int(), nm + " x");
        check_eq(static_cast<std::int64_t>(a.y), ex.at("y").as_int(), nm + " y");
        check_eq(static_cast<std::int64_t>(a.z), ex.at("z").as_int(), nm + " z");
        check_eq(a.x_negative, ex.at("x_negative").as_bool(), nm + " x_negative");
        check_eq(a.y_negative, ex.at("y_negative").as_bool(), nm + " y_negative");
        check_eq(a.z_negative, ex.at("z_negative").as_bool(), nm + " z_negative");
        auto packed = a.pack();
        const auto& rp = c.at("repack").as_array();
        check(packed && rp.size() == 2 &&
                  static_cast<std::int64_t>(packed->first) == rp[0].as_int() &&
                  static_cast<std::int64_t>(packed->second) == rp[1].as_int(),
              nm + " repack");
        auto pl = B::placement(name);
        check(pl && *pl == a, nm + " placement()");
        if (i < B::PLACEMENTS.size())
            check(B::PLACEMENTS[i].first == cfg && B::PLACEMENTS[i].second == sgn,
                  nm + " PLACEMENTS row");
        seen(c, f0);
    }
    check(B::placement("p1") && *B::placement("p1") == B::AxisRemap{},
          "bno055 placement is case-insensitive; P1 is the default");
    check(!B::placement("P8") && !B::placement("P") && !B::placement("P10") && !B::placement(""),
          "bno055 placement refuses unknown names");

    // axis_remap_invalid: a non-permutation must be refused.
    for (const Value& c : v.at("axis_remap_invalid").as_array()) {
        const int f0 = g_fails;
        B::AxisRemap a;
        a.x = static_cast<std::uint8_t>(c.at("x").as_int());
        a.y = static_cast<std::uint8_t>(c.at("y").as_int());
        a.z = static_cast<std::uint8_t>(c.at("z").as_int());
        check(!a.pack().has_value(), "bno055 axis_remap_invalid " + c.at("name").as_string());
        seen(c, f0);
    }
    {
        B::AxisRemap a;
        a.x = 3;
        check(!a.pack().has_value(), "bno055 axis_remap refuses axis code 3");
    }

    // sensor_config: page-1 ACC / GYR / MAG config codes.
    const Value& sc = v.at("sensor_config");
    const int sc_f0 = g_fails;
    for (const Value& c : sc.at("accel").as_array()) {
        const auto val = static_cast<std::uint8_t>(c.at("value").as_int());
        const std::string nm = "bno055 accel_config " + std::to_string(val);
        const Value& ex = c.at("expect");
        B::AccelConfig a = B::AccelConfig::unpack(val);
        check_eq(ex.as_object().size(), static_cast<std::size_t>(3), nm + " field count");
        check_eq(static_cast<std::int64_t>(a.range), ex.at("range").as_int(), nm + " range");
        check_eq(static_cast<std::int64_t>(a.bandwidth), ex.at("bandwidth").as_int(),
                 nm + " bandwidth");
        check_eq(static_cast<std::int64_t>(a.power), ex.at("power").as_int(), nm + " power");
        check_eq(static_cast<std::int64_t>(a.pack()), static_cast<std::int64_t>(val), nm + " repack");
    }
    for (const Value& c : sc.at("gyro").as_array()) {
        const std::string hex = c.at("bytes").as_string();
        const std::string nm = "bno055 gyro_config " + hex;
        const Value& ex = c.at("expect");
        depz::bytes raw = from_hex(hex);
        auto g = B::GyroConfig::unpack(depz::as_bytes(raw));
        if (!g) {
            check(false, nm + " unpack");
            continue;
        }
        check_eq(ex.as_object().size(), static_cast<std::size_t>(3), nm + " field count");
        check_eq(static_cast<std::int64_t>(g->range), ex.at("range").as_int(), nm + " range");
        check_eq(static_cast<std::int64_t>(g->bandwidth), ex.at("bandwidth").as_int(),
                 nm + " bandwidth");
        check_eq(static_cast<std::int64_t>(g->power), ex.at("power").as_int(), nm + " power");
        check_eq(to_hex(g->pack()), hex, nm + " repack");
    }
    {
        depz::bytes one(1, std::byte{0x38});
        check(!B::GyroConfig::unpack(depz::as_bytes(one)), "bno055 gyro_config needs 2 bytes");
    }
    for (const Value& c : sc.at("mag").as_array()) {
        const std::int64_t val = c.at("value").as_int();
        const std::string nm = "bno055 mag_config " + std::to_string(val);
        const Value& ex = c.at("expect");
        B::MagConfig m = B::MagConfig::unpack(static_cast<std::uint8_t>(val));
        check_eq(ex.as_object().size(), static_cast<std::size_t>(3), nm + " field count");
        check_eq(static_cast<std::int64_t>(m.rate), ex.at("rate").as_int(), nm + " rate");
        check_eq(static_cast<std::int64_t>(m.mode), ex.at("mode").as_int(), nm + " mode");
        check_eq(static_cast<std::int64_t>(m.power), ex.at("power").as_int(), nm + " power");
        // Bit 7 is not a field: the repack is value & 0x7F.
        check_eq(static_cast<std::int64_t>(m.pack()), val & 0x7F, nm + " repack");
    }
    check_eq(static_cast<int>(B::MagConfig::unpack(0x8B).pack()), 0x0B,
             "bno055 mag_config drops bit 7");
    seen_id("bno055/sensor_config", sc_f0);

    // blocks: channel extraction from any register window.
    for (const Value& c : v.at("blocks").as_array()) {
        const int f0 = g_fails;
        const std::string nm = "bno055 block " + c.at("name").as_string();
        depz::bytes data = from_hex(c.at("data").as_string());
        const Value& ex = c.at("expect");
        B::RawBlock b =
            B::decode_block(static_cast<std::uint8_t>(c.at("addr").as_int()), depz::as_bytes(data));
        check_eq(ex.as_object().size(), static_cast<std::size_t>(9), nm + " field count");
        check_bno_words(b.accel, ex.at("accel"), nm + " accel");
        check_bno_words(b.mag, ex.at("mag"), nm + " mag");
        check_bno_words(b.gyro, ex.at("gyro"), nm + " gyro");
        check_bno_words(b.euler, ex.at("euler"), nm + " euler");
        check_bno_words(b.quaternion, ex.at("quaternion"), nm + " quaternion");
        check_bno_words(b.linear_accel, ex.at("linear_accel"), nm + " linear_accel");
        check_bno_words(b.gravity, ex.at("gravity"), nm + " gravity");
        check_bno_scalar(b.temperature, ex.at("temperature"), nm + " temperature");
        check_bno_scalar(b.calib_stat, ex.at("calib_stat"), nm + " calib_stat");
        seen(c, f0);
    }
    {
        // Window edges beyond the vectors: the full block, and one byte short.
        depz::bytes full(B::FULL_BLOCK_LEN, std::byte{0});
        full[B::REG_TEMP - B::FULL_BLOCK_ADDR] = std::byte{0xE5};  // -27
        full[B::REG_CALIB_STAT - B::FULL_BLOCK_ADDR] = std::byte{0xFF};
        B::RawBlock b = B::decode_block(B::FULL_BLOCK_ADDR, depz::as_bytes(full));
        check(b.accel && b.mag && b.gyro && b.euler && b.quaternion && b.linear_accel &&
                  b.gravity && b.temperature == std::int8_t{-27} && b.calib_stat == 0xFF,
              "bno055 full block: every channel present");
        b = B::decode_block(B::FULL_BLOCK_ADDR, depz::byte_span(full.data(), full.size() - 1));
        check(b.temperature && !b.calib_stat, "bno055 45-byte block: no calib_stat");
        b = B::decode_block(B::REG_GRV_DATA, depz::byte_span(full.data(), 5));
        check(!b.gravity, "bno055 5-byte gravity window: gravity absent");
        b = B::decode_block(0xF0, depz::byte_span(full.data(), 16));
        check(!b.accel && !b.calib_stat, "bno055 window past the data registers");
    }
}

// ---- tmf8829 ---------------------------------------------------------------

// SHA-256 (FIPS 180-4), test-runner only: the TMF8829 vectors pin frame and
// image bytes by digest.
// A JSON int array (null = absent) against an optional vector.
template <class T>
static void check_tmf_ints(const std::optional<std::vector<T>>& got, const Value& want, const std::string& what)
{
    if (want.is_null()) {
        check(!got.has_value(), what + " absent");
        return;
    }
    if (!got) {
        check(false, what + " present");
        return;
    }
    const auto& arr = want.as_array();
    bool same = got->size() == arr.size();
    for (std::size_t i = 0; same && i < arr.size(); ++i)
        same = static_cast<std::int64_t>((*got)[i]) == arr[i].as_int();
    check(same, what);
}

// The vectors' `_arrays(res)`: peaks, shape, every raw per-zone array.
static void check_tmf_results(const depz::tmf8829::PixelResults& r, const Value& ex, const std::string& nm)
{
    const auto shape = r.shape();
    check_eq(static_cast<std::int64_t>(r.peaks), ex.at("peaks").as_int(), "tmf8829 peaks " + nm);
    check_eq(static_cast<std::int64_t>(shape.first), ex.at("rows").as_int(), "tmf8829 rows " + nm);
    check_eq(static_cast<std::int64_t>(shape.second), ex.at("cols").as_int(), "tmf8829 cols " + nm);
    check_tmf_ints(r.distance, ex.at("distance"), "tmf8829 distance " + nm);
    check_tmf_ints(r.snr, ex.at("snr"), "tmf8829 snr " + nm);
    check_tmf_ints(r.signal, ex.at("signal"), "tmf8829 signal " + nm);
    check_tmf_ints(r.noise, ex.at("noise"), "tmf8829 noise " + nm);
    check_tmf_ints(r.xtalk, ex.at("xtalk"), "tmf8829 xtalk " + nm);
}

template <class F>
static bool throws_any(F&& f)
{
    try {
        f();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

static void test_tmf8829()
{
    namespace T = depz::tmf8829;
    Value v = load_vector("tmf8829.json");

    // encode: 0x32..0x37 payloads.
    for (const Value& c : v.at("encode").as_array()) {
        const int f0 = g_fails;
        const std::string nm = c.at("name").as_string();
        const std::int64_t cmd = c.at("cmd").as_int();
        depz::bytes got;
        if (cmd == static_cast<std::int64_t>(T::Tmf8829Cmd::ReadReg)) {
            got = T::pack_read_reg(static_cast<int>(c.at("addr").as_int()), static_cast<int>(c.at("len").as_int()));
        } else if (cmd == static_cast<std::int64_t>(T::Tmf8829Cmd::WriteReg)) {
            depz::bytes data = from_hex(c.at("data").as_string());
            got = T::pack_write_reg(static_cast<int>(c.at("addr").as_int()), depz::as_bytes(data));
        } else if (cmd == static_cast<std::int64_t>(T::Tmf8829Cmd::StartStream)) {
            got = T::pack_start_stream(static_cast<int>(c.at("int_mask").as_int()),
                                       static_cast<int>(c.at("flags").as_int()));
        }  // STOP_STREAM / GET_INFO: empty payload
        check_eq(to_hex(got), c.at("payload").as_string(), "tmf8829 encode " + nm);
        seen(c, f0);
    }

    // encode_errors: refused before anything is sent.
    for (const Value& c : v.at("encode_errors").as_array()) {
        const int f0 = g_fails;
        const std::string nm = c.at("name").as_string(), kind = c.at("kind").as_string();
        bool threw = false;
        try {
            if (kind == "read_reg") {
                T::pack_read_reg(static_cast<int>(c.at("addr").as_int()), static_cast<int>(c.at("len").as_int()));
            } else if (kind == "write_reg") {
                depz::bytes data = from_hex(c.at("data").as_string());
                T::pack_write_reg(static_cast<int>(c.at("addr").as_int()), depz::as_bytes(data));
            } else {
                T::pack_start_stream(static_cast<int>(c.at("int_mask").as_int()),
                                     static_cast<int>(c.at("flags").as_int()));
            }
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw, "tmf8829 encode_error " + nm);
        seen(c, f0);
    }

    // decode: REG_DATA 0x91, INFO 0x92 (no echoed cmd), FRAME chunk 0x93, EVENT 0x94.
    for (const Value& c : v.at("decode").as_array()) {
        const int f0 = g_fails;
        const std::string nm = c.at("name").as_string();
        depz::bytes payload = from_hex(c.at("payload").as_string());
        const Value& ex = c.at("expect");
        const std::int64_t report = c.at("report").as_int();
        auto f = [&](const char* k, std::int64_t got) {
            check_eq(got, ex.at(k).as_int(), std::string("tmf8829 decode.") + k + " " + nm);
        };
        if (report == static_cast<std::int64_t>(T::Tmf8829Rpt::RegData)) {
            auto r = T::RegData::unpack(depz::as_bytes(payload));
            if (!r) { check(false, "tmf8829 reg_data unpack " + nm); continue; }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(2), "tmf8829 reg_data field count " + nm);
            f("timestamp_us", static_cast<std::int64_t>(r->timestamp_us));
            check_eq(to_hex(r->data), ex.at("data").as_string(), "tmf8829 reg_data.data " + nm);
        } else if (report == static_cast<std::int64_t>(T::Tmf8829Rpt::Info)) {
            auto r = T::Tmf8829Info::unpack(depz::as_bytes(payload));
            if (!r) { check(false, "tmf8829 info unpack " + nm); continue; }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(10), "tmf8829 info field count " + nm);
            f("int_edges", r->int_edges);
            f("frames", r->frames);
            f("frames_aborted", r->frames_aborted);
            f("frame_errors", r->frame_errors);
            f("spi_timeouts", r->spi_timeouts);
            f("int_level", r->int_level);
            check_eq(r->streaming, ex.at("streaming").as_bool(), "tmf8829 info.streaming " + nm);
            f("int_mask", r->int_mask);
            f("last_int_status", r->last_int_status);
            f("last_fifo_status", r->last_fifo_status);
        } else if (report == static_cast<std::int64_t>(T::Tmf8829Rpt::Frame)) {
            auto r = T::FrameChunk::unpack(depz::as_bytes(payload));
            if (!r) { check(false, "tmf8829 frame chunk unpack " + nm); continue; }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(6), "tmf8829 chunk field count " + nm);
            f("timestamp_us", static_cast<std::int64_t>(r->timestamp_us));
            f("full_size", r->full_size);
            f("offset", r->offset);
            f("frame_seq", r->frame_seq);
            f("edge_us", static_cast<std::int64_t>(r->edge_us));
            check_eq(to_hex(r->data), ex.at("data").as_string(), "tmf8829 chunk.data " + nm);
        } else if (report == static_cast<std::int64_t>(T::Tmf8829Rpt::Event)) {
            auto r = T::Tmf8829Event::unpack(depz::as_bytes(payload));
            if (!r) { check(false, "tmf8829 event unpack " + nm); continue; }
            check_eq(ex.as_object().size(), static_cast<std::size_t>(5), "tmf8829 event field count " + nm);
            f("timestamp_us", static_cast<std::int64_t>(r->timestamp_us));
            f("int_status", r->int_status);
            f("code", r->code);
            f("frame_id", r->frame_id);
            f("fifo_status", r->fifo_status);
        } else {
            check(false, "unknown tmf8829 report " + nm);
        }
        seen(c, f0);
    }
    // Short payloads and a wrong echoed cmd do not decode.
    {
        depz::bytes p = from_hex("33000000000000000000");
        check(!T::RegData::unpack(depz::as_bytes(p)), "tmf8829 reg_data wrong cmd");
        p.resize(8);
        check(!T::Tmf8829Event::unpack(depz::as_bytes(p)), "tmf8829 event short");
        check(!T::FrameChunk::unpack(depz::as_bytes(p)), "tmf8829 chunk short");
        check(!T::Tmf8829Info::unpack(depz::as_bytes(p)), "tmf8829 info short");
    }

    // reassembly: in-order, seq change mid-frame, offset gap, ...
    for (const Value& c : v.at("reassembly").as_array()) {
        const int f0 = g_fails;
        const std::string nm = c.at("name").as_string();
        const Value& ex = c.at("expect");
        T::FrameReassembler r;
        std::vector<T::RawFrame> done;
        for (const Value& ch : c.at("chunks").as_array()) {
            depz::bytes p = from_hex(ch.as_string());
            auto chunk = T::FrameChunk::unpack(depz::as_bytes(p));
            if (!chunk) { check(false, "tmf8829 reassembly chunk " + nm); continue; }
            if (auto f = r.feed(*chunk)) done.push_back(std::move(*f));
        }
        const auto& want = ex.at("frames").as_array();
        check_eq(done.size(), want.size(), "tmf8829 reassembly frames " + nm);
        for (std::size_t i = 0; i < done.size() && i < want.size(); ++i) {
            const Value& w = want[i];
            check_eq(static_cast<std::int64_t>(done[i].frame_seq), w.at("frame_seq").as_int(), "tmf8829 reassembly seq " + nm);
            check_eq(static_cast<std::int64_t>(done[i].timestamp_us), w.at("timestamp_us").as_int(), "tmf8829 reassembly ts " + nm);
            check_eq(static_cast<std::int64_t>(done[i].edge_us), w.at("edge_us").as_int(), "tmf8829 reassembly edge " + nm);
            check_eq(static_cast<std::int64_t>(done[i].data.size()), w.at("size").as_int(), "tmf8829 reassembly size " + nm);
            check_eq(sha256_hex(depz::as_bytes(done[i].data)), w.at("sha256").as_string(), "tmf8829 reassembly sha256 " + nm);
        }
        check_eq(r.completed, ex.at("completed").as_u64(), "tmf8829 reassembly completed " + nm);
        check_eq(r.discarded, ex.at("discarded").as_u64(), "tmf8829 reassembly discarded " + nm);
        check_eq(r.seq_gaps, ex.at("seq_gaps").as_u64(), "tmf8829 reassembly seq_gaps " + nm);
        seen(c, f0);
    }

    // frames: header / footer, problems, raw per-zone integers (or data digest).
    for (const Value& c : v.at("frames").as_array()) {
        const int f0 = g_fails;
        const std::string nm = c.at("name").as_string();
        depz::bytes raw = from_hex(c.at("raw").as_string());
        const T::FrameInfo fi = T::parse_frame(depz::as_bytes(raw));
        const Value& h = c.at("header");
        auto f = [&](const char* k, std::int64_t got) {
            check_eq(got, h.at(k).as_int(), std::string("tmf8829 header.") + k + " " + nm);
        };
        f("fifo_status", fi.fifo_status);
        f("systick", fi.systick);
        f("id", fi.id);
        f("kind", fi.kind());
        f("fp_mode", fi.fp_mode());
        f("layout", fi.layout);
        f("payload", fi.payload);
        f("f_number", fi.f_number);
        const auto& temp = h.at("temperature").as_array();
        check(temp.size() == 3 && temp[0].as_int() == fi.temperature[0] && temp[1].as_int() == fi.temperature[1] &&
                  temp[2].as_int() == fi.temperature[2],
              "tmf8829 header.temperature " + nm);
        f("bdv", fi.bdv);
        const auto& rp = h.at("ref_pos").as_array();
        check(rp.size() == 2 && rp[0].as_int() == fi.ref_pos[0] && rp[1].as_int() == fi.ref_pos[1],
              "tmf8829 header.ref_pos " + nm);
        f("t0_integration", fi.t0_integration);
        f("t1_integration", fi.t1_integration);
        f("frame_status", fi.frame_status);
        f("reserved", fi.reserved);
        f("eof", fi.eof);
        f("size", static_cast<std::int64_t>(fi.size));
        check_eq(h.as_object().size(), static_cast<std::size_t>(17), "tmf8829 header field count " + nm);
        std::vector<std::string> want_problems;
        for (const Value& p : c.at("problems").as_array()) want_problems.push_back(p.as_string());
        check(fi.problems() == want_problems, "tmf8829 problems " + nm);
        check_eq(depz::tmf8829::frame_size(depz::as_bytes(raw)), raw.size(), "tmf8829 frame_size " + nm);
        if (fi.kind() == T::FID_RESULTS) {
            check_tmf_results(T::parse_results(depz::as_bytes(raw), fi), c.at("results"), nm);
        } else {
            depz::byte_span data = depz::as_bytes(raw).subspan(21, raw.size() - 33);
            check_eq(sha256_hex(data), c.at("data_sha256").as_string(), "tmf8829 data_sha256 " + nm);
        }
        seen(c, f0);
    }
    // A frame shorter than header + footer, and a corrupt results frame.
    {
        depz::bytes shorty(32);
        check(throws_any([&] { T::parse_frame(depz::as_bytes(shorty)); }), "tmf8829 short frame refused");
        for (const Value& c : v.at("frames").as_array()) {
            if (c.at("name").as_string() != "8x8#0") continue;
            depz::bytes raw = from_hex(c.at("raw").as_string());
            raw.erase(raw.begin() + 21);  // one data byte short
            bool frame_error = false;
            try {
                T::parse_results(depz::as_bytes(raw));
            } catch (const T::FrameError&) {
                frame_error = true;
            }
            check(frame_error, "tmf8829 corrupt results frame refused");
        }
    }

    // measurements: each scenario through a fresh builder.
    std::vector<std::pair<std::string, depz::bytes>> raw_by_case;
    for (const Value& c : v.at("frames").as_array())
        raw_by_case.emplace_back(c.at("name").as_string(), from_hex(c.at("raw").as_string()));
    auto raw_of = [&](const std::string& name) -> const depz::bytes* {
        for (const auto& p : raw_by_case)
            if (p.first == name) return &p.second;
        return nullptr;
    };
    for (const Value& s : v.at("measurements").as_array()) {
        const int f0 = g_fails;
        const std::string nm = s.at("name").as_string();
        T::MeasurementBuilder b;
        std::vector<std::pair<T::Measurement, std::uint32_t>> got;
        for (const Value& f : s.at("frames").as_array()) {
            const depz::bytes* raw = raw_of(f.at("raw_case").as_string());
            if (!raw) { check(false, "tmf8829 measurement raw_case " + nm); continue; }
            auto m = b.add(depz::as_bytes(*raw), f.at("timestamp_us").as_int(), f.at("edge_us").as_int());
            if (m) got.emplace_back(std::move(*m), b.clock.ratio_q16());
        }
        const auto& want = s.at("expect").as_array();
        check_eq(got.size(), want.size(), "tmf8829 measurements count " + nm);
        for (std::size_t i = 0; i < got.size() && i < want.size(); ++i) {
            const T::Measurement& m = got[i].first;
            const Value& e = want[i];
            const std::string tag = nm + "[" + std::to_string(i) + "]";
            check_eq(e.as_object().size(), static_cast<std::size_t>(19), "tmf8829 measurement field count " + tag);
            check_eq(m.timestamp_us, e.at("timestamp_us").as_int(), "tmf8829 m.timestamp_us " + tag);
            check_eq(static_cast<std::int64_t>(m.flags), e.at("flags").as_int(), "tmf8829 m.flags " + tag);
            check_eq(static_cast<std::int64_t>(m.fp_mode), e.at("fp_mode").as_int(), "tmf8829 m.fp_mode " + tag);
            check_eq(static_cast<std::int64_t>(m.f_number), e.at("f_number").as_int(), "tmf8829 m.f_number " + tag);
            std::vector<std::int64_t> ft, tl, wft, wtl;
            for (const auto& t : m.times) {
                ft.push_back(t.timestamp_us);
                tl.push_back(t.t1_len_us);
            }
            for (const Value& x : e.at("frame_times_us").as_array()) wft.push_back(x.as_int());
            for (const Value& x : e.at("t1_len_us").as_array()) wtl.push_back(x.as_int());
            check(ft == wft, "tmf8829 m.frame_times_us " + tag);
            check(tl == wtl, "tmf8829 m.t1_len_us " + tag);
            check_eq(static_cast<std::int64_t>(got[i].second), e.at("ratio_q16").as_int(), "tmf8829 ratio_q16 " + tag);
            check_eq(static_cast<std::int64_t>(m.ratio_q16), e.at("ratio_q16").as_int(), "tmf8829 m.ratio_q16 " + tag);
            check_eq(static_cast<std::int64_t>(m.histograms.size()), e.at("n_histograms").as_int(), "tmf8829 m.n_histograms " + tag);
            check_eq(static_cast<std::int64_t>(m.n_histograms), e.at("n_histograms").as_int(), "tmf8829 m.n_histograms count " + tag);
            check_eq(static_cast<std::int64_t>(m.ref_frames.size()), e.at("n_ref_frames").as_int(), "tmf8829 m.n_ref_frames " + tag);
            check_eq(static_cast<std::int64_t>(m.problems.size()), e.at("n_problems").as_int(), "tmf8829 m.n_problems " + tag);
            check_eq(m.distance_in_mm(), e.at("distance_in_mm").as_bool(), "tmf8829 m.distance_in_mm " + tag);
            check_tmf_results(m.results, e, tag);
            // distance_mm = raw x ratio (/ 4 in 0.25 mm units)
            if (m.results.distance) {
                const double ratio = static_cast<double>(m.ratio_q16) / depz::tmf8829::NOMINAL_Q16;
                const double raw0 = (*m.results.distance)[0] * ratio;
                check(m.distance_mm.size() == m.results.distance->size() &&
                          m.distance_mm[0] == (m.distance_in_mm() ? raw0 : raw0 / 4.0),
                      "tmf8829 m.distance_mm " + tag);
            }
        }
        seen(s, f0);
    }
    // Half-frames of different formats are dropped and logged, never merged
    // (contract 14 §3): a 32x32 first half, then a second half of another
    // fp_mode.
    {
        const depz::bytes* first = raw_of("32x32#0");
        const depz::bytes* second = raw_of("32x32#1");
        if (first && second) {
            depz::bytes odd = *second;
            // fp_mode 3 (32x32) -> 4 (32x32s): the same frame size, another format
            odd[5] = std::byte{static_cast<std::uint8_t>(std::to_integer<std::uint8_t>(odd[5]) ^ 0x07)};
            T::MeasurementBuilder b;
            check(!b.add(depz::as_bytes(*first), 1000000, 0), "tmf8829 mixed pair: first half pending");
            check(!b.add(depz::as_bytes(odd), 1033000, 0), "tmf8829 mixed pair: dropped");
            check(!b.add(depz::as_bytes(*first), 1066000, 0), "tmf8829 mixed pair: next first half");
            auto m = b.add(depz::as_bytes(*second), 1099000, 0);
            bool logged = false;
            if (m)
                for (const auto& p : m->problems) logged |= p.find("half-frames of different formats") != std::string::npos;
            check(m && logged, "tmf8829 mixed pair logged in the next measurement");
        } else {
            check(false, "tmf8829 32x32 frames present");
        }
    }

    // config: get / set / set_errors on a captured page.
    const Value& cfg = v.at("config");
    const int cfg_f0 = g_fails;
    const depz::bytes page = from_hex(cfg.at("page").as_string());
    {
        const auto& fields = cfg.at("fields").as_object();
        check_eq(fields.size(), T::CFG_FIELDS.size(), "tmf8829 config field count");
        for (const auto& fd : T::CFG_FIELDS) {
            const Value& w = cfg.at("fields").at(fd.name);
            check(w.as_array().size() == 3 && w.as_array()[0].as_int() == fd.reg &&
                      w.as_array()[1].as_int() == fd.shift && w.as_array()[2].as_int() == fd.width,
                  std::string("tmf8829 config field ") + fd.name);
        }
    }
    for (const auto& kv : cfg.at("get").as_object())
        check_eq(static_cast<std::int64_t>(T::cfg_get(depz::as_bytes(page), kv.first)), kv.second.as_int(),
                 "tmf8829 cfg_get " + kv.first);
    {
        const auto all = T::config_fields(depz::as_bytes(page));
        check_eq(all.size(), cfg.at("get").as_object().size(), "tmf8829 config_fields count");
        for (const auto& kv : all)
            check_eq(static_cast<std::int64_t>(kv.second), cfg.at("get").at(kv.first).as_int(), "tmf8829 config_fields " + kv.first);
    }
    for (const Value& c : cfg.at("set").as_array()) {
        depz::bytes pg = page;
        T::cfg_set(pg, c.at("field").as_string(), c.at("value").as_int());
        check_eq(to_hex(pg), c.at("page").as_string(), "tmf8829 cfg_set " + c.at("field").as_string());
    }
    for (const Value& c : cfg.at("set_errors").as_array()) {
        depz::bytes pg = page;
        bool threw = false;
        try {
            T::cfg_set(pg, c.at("field").as_string(), c.at("value").as_int());
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        check(threw && pg == page, "tmf8829 cfg_set error " + c.at("field").as_string());
    }
    {
        depz::bytes pg = page;
        check(throws_any([&] { T::cfg_set(pg, "no_such_field", 1); }), "tmf8829 cfg_set unknown field");
        check(throws_any([&] { T::cfg_get(depz::as_bytes(page), "no_such_field"); }), "tmf8829 cfg_get unknown field");
    }
    seen_id("tmf8829/config", cfg_f0);
    // Presets and their kilo_iterations (contract 14 §4).
    {
        auto p = T::find_preset("32x32");
        check(p && p->cmd == 0x45 && p->kilo_iterations == 2344, "tmf8829 preset 32x32");
        p = T::find_preset("8x8-xr");
        check(p && p->cmd == 0x49 && p->kilo_iterations == 586, "tmf8829 preset 8x8-xr");
        p = T::find_preset("48x32-ha");
        check(p && p->cmd == 0x48 && p->kilo_iterations == 3516, "tmf8829 preset 48x32-ha");
        check(!T::find_preset("64x64"), "tmf8829 unknown preset");
        check(T::status_name(4) == "ERR_CONFIG_RESULT_SIZE" && T::status_name(0x0E) == "ERR_OSC_TUNE" &&
                  T::status_name(0x0F).empty(),
              "tmf8829 status names");
    }

    // firmware: the embedded image.
    const Value& fw = v.at("firmware");
    const int fw_f0 = g_fails;
    const depz::byte_span img = T::firmware_image();
    check_eq(static_cast<std::int64_t>(T::FIRMWARE_LOAD_ADDR), fw.at("load_addr").as_int(), "tmf8829 firmware load_addr");
    check_eq(static_cast<std::int64_t>(img.size()), fw.at("size").as_int(), "tmf8829 firmware size");
    check_eq(static_cast<std::int64_t>(img.size() / 4), fw.at("words").as_int(), "tmf8829 firmware words");
    check_eq(sha256_hex(img), fw.at("sha256").as_string(), "tmf8829 firmware sha256");
    seen_id("tmf8829/firmware", fw_f0);
}

// tmf8829_frame_time.json: 27 recorded streams through SensorClock +
// CaptureTimer, every results frame bit for bit.
static void test_tmf8829_frame_time()
{
    namespace T = depz::tmf8829;
    Value v = load_vector("tmf8829_frame_time.json");
    std::vector<std::string> cols;
    for (const Value& c : v.at("columns").as_array()) cols.push_back(c.as_string());
    auto col = [&](const char* name) -> std::size_t {
        for (std::size_t i = 0; i < cols.size(); ++i)
            if (cols[i] == name) return i;
        check(false, std::string("tmf8829_frame_time column ") + name);
        return 0;
    };
    const std::size_t c_id = col("id"), c_ts = col("ts"), c_tick = col("systick"), c_t0 = col("t0"),
                      c_t1 = col("t1"), c_edge = col("edge_us"), c_ets = col("exp_timestamp_us"),
                      c_elen = col("exp_t1_len_us"), c_eflags = col("exp_flags"), c_eratio = col("exp_ratio_q16");
    int checked = 0;
    for (const Value& s : v.at("streams").as_array()) {
        const int f0 = g_fails;
        const std::string scen = s.at("scenario").as_string();
        T::SensorClock clock;
        T::CaptureTimer timer;
        int row_no = 0;
        for (const Value& rv : s.at("frames").as_array()) {
            const auto& row = rv.as_array();
            const auto fid = static_cast<unsigned>(row[c_id].as_int());
            const std::int64_t ts = row[c_ts].as_int();
            const auto tick = static_cast<std::uint32_t>(row[c_tick].as_int());
            const unsigned kind = fid & 0xF0;
            row_no++;
            if (kind == T::FID_REF_SPAD) continue;
            const std::uint8_t flags = clock.add(ts, tick);
            if (kind == T::FID_HISTOGRAMS) {
                timer.histograms();
                continue;
            }
            const T::FrameTime t = timer.results(clock, fid & 0x0F, tick, static_cast<std::uint32_t>(row[c_t0].as_int()),
                                                 static_cast<std::uint32_t>(row[c_t1].as_int()), ts,
                                                 row[c_edge].as_int(), flags);
            const bool same = t.timestamp_us == row[c_ets].as_int() && t.t1_len_us == row[c_elen].as_int() &&
                              t.flags == row[c_eflags].as_int() && clock.ratio_q16() == row[c_eratio].as_int();
            check(same, "tmf8829_frame_time " + scen + " frame " + std::to_string(row_no));
            checked++;
        }
        seen(s, f0);
    }
    check_eq(checked, 970, "tmf8829_frame_time results frames");
    // Floor division / floor shift on negative operands (MCU time going back).
    {
        T::SensorClock c;
        check_eq(c.to_us(1000, 100, 99), static_cast<std::int64_t>(992), "tmf8829 to_us one tick back");
        check_eq(c.to_us(1000, 0x00000005u, 0xFFFFFFFFu), static_cast<std::int64_t>(952), "tmf8829 to_us u32 wrap back");
        check_eq(static_cast<int>(c.add(1000000, 0)), 0, "tmf8829 clock first pair");
        // MCU time going back is never a valid step (d_us < 0, floor-divided tolerance).
        check_eq(static_cast<int>(c.add(1000000 - 1005, 125)), static_cast<int>(T::FT_CLOCK_RESET),
                 "tmf8829 clock reset when time goes back");
    }
    check(T::flag_names(T::FT_NOMINAL | T::FT_STALLED) == std::vector<std::string>{"nominal", "stalled"},
          "tmf8829 flag_names");

    // recordings/tmf8829_frame_time.jsonl: the board's own capture the vector
    // was generated from, replayed raw — each `scenario` line starts a stream.
    {
        const int f0 = g_fails;
        T::SensorClock clock;
        T::CaptureTimer timer;
        int rec_checked = 0, line_no = 0;
        for (const std::string& line : split_lines(load_recording("tmf8829_frame_time.jsonl"))) {
            line_no++;
            const Value o = testjson::parse(line);
            if (o.has("scenario")) {
                clock.reset();
                timer.reset();
                continue;
            }
            const auto fid = static_cast<unsigned>(o.at("id").as_int());
            const std::int64_t ts = o.at("ts").as_int();
            const auto tick = static_cast<std::uint32_t>(o.at("systick").as_int());
            const unsigned kind = fid & 0xF0;
            if (kind == T::FID_REF_SPAD) continue;
            const std::uint8_t flags = clock.add(ts, tick);
            if (kind == T::FID_HISTOGRAMS) {
                timer.histograms();
                continue;
            }
            const T::FrameTime t = timer.results(clock, fid & 0x0F, tick, static_cast<std::uint32_t>(o.at("t0").as_int()),
                                                 static_cast<std::uint32_t>(o.at("t1").as_int()), ts,
                                                 o.at("edge_us").as_int(), flags);
            const Value& e = o.at("exp");
            check(t.timestamp_us == e.at("timestamp_us").as_int() && t.t1_len_us == e.at("t1_len_us").as_int() &&
                      t.flags == e.at("flags").as_int() && clock.ratio_q16() == e.at("ratio_q16").as_int(),
                  "tmf8829_frame_time.jsonl line " + std::to_string(line_no));
            rec_checked++;
        }
        check_eq(rec_checked, 970, "tmf8829_frame_time.jsonl results frames");
        seen_id("recordings/tmf8829_frame_time.jsonl", f0);
    }
}

static int run_foundation();

// ---- input_rules (contract 07 §3) ------------------------------------------
// Every vector file's `input_rules` group: short payloads rejected, trailing
// bytes ignored, every encoder argument range-checked. An argument outside the
// C++ parameter's own type (300 for a uint8_t, -1 for an unsigned, a fraction)
// cannot be passed at all: that case is the type's rejection (contract 16 §1),
// asserted as such. Everything inside the type goes through the SDK, which
// must throw / return nullopt, or decode.

namespace {
struct Refused {};  // an argument the C++ parameter type cannot hold

std::int64_t ir_int(const Value& in, const char* k, std::int64_t lo, std::int64_t hi) {
    if (!in.has(k)) throw Refused{};
    const Value& v = in.at(k);
    if (v.type != testjson::Type::Int || v.range != 0 || v.i < lo || v.i > hi) throw Refused{};
    return v.i;
}
std::uint8_t ir_u8(const Value& in, const char* k) { return static_cast<std::uint8_t>(ir_int(in, k, 0, 0xFF)); }
std::uint16_t ir_u16(const Value& in, const char* k) { return static_cast<std::uint16_t>(ir_int(in, k, 0, 0xFFFF)); }
std::uint32_t ir_u32(const Value& in, const char* k) { return static_cast<std::uint32_t>(ir_int(in, k, 0, 0xFFFFFFFFLL)); }
int ir_i32(const Value& in, const char* k) { return static_cast<int>(ir_int(in, k, INT32_MIN, INT32_MAX)); }
template <class T>
T ir_need(const std::optional<T>& v) {
    if (!v) throw std::invalid_argument("nullopt");
    return *v;
}

// Run one case: true when refused (type or SDK); else checks the value.
bool ir_run(const std::string& label, const std::string& op, const Value& in, const Value& e) {
    namespace B = depz::bno055;
    namespace S = depz::bno086;
    namespace T = depz::tmf8829;
    namespace L4 = depz::vl53l4;
    namespace L7 = depz::vl53l7;
    namespace LX = depz::vl53lx;
    namespace L8 = depz::vl53l8;
    const std::string kind = in.has("kind") ? in.at("kind").as_string() : "";
    auto want_payload = [&](const depz::bytes& p) { check_eq(to_hex(p), e.at("payload").as_string(), label); };
    try {
        if (op == "common_decode") {
            const depz::bytes p = from_hex(in.at("payload").as_string());
            const std::int64_t rpt = in.at("report").as_int();
            if (rpt == 0x80) {
                const auto r = depz::StatusReport::unpack(p);
                check(r.cmd == e.at("cmd").as_int() && r.status == e.at("status").as_int(), label + ": fields");
            } else if (rpt == 0x81) {
                const auto r = depz::TextReport::unpack(p);
                check(r.cmd == e.at("cmd").as_int() && r.text == e.at("text").as_string(), label + ": fields");
            } else if (rpt == 0x83) {
                const auto r = depz::TemperatureReport::unpack(p);
                check(static_cast<std::int64_t>(r.timestamp_us) == e.at("timestamp_us").as_int() &&
                          r.raw_decidegrees == e.at("raw_decidegrees").as_int(), label + ": fields");
            } else if (rpt == 0x84) {
                const auto r = depz::SequenceErrorReport::unpack(p);
                check(r.expected_seq == e.at("expected_seq").as_int() && r.received_seq == e.at("received_seq").as_int(),
                      label + ": fields");
            } else {
                check(false, label + ": unknown report");
            }
        } else if (op == "common_encode") {
            if (kind == "sync_pin_config") {
                depz::SyncPinConfig c{ir_u8(in, "pin"), static_cast<depz::SyncPinMode>(ir_u8(in, "mode")),
                                      static_cast<depz::SyncPinPolarity>(ir_u8(in, "polarity"))};
                want_payload(c.pack());
            } else if (kind == "sync_time_request") {
                const Value& v = in.at("pc_timestamp_us");
                if (v.type != testjson::Type::Int || v.range == 2 || (v.range == 0 && v.i < 0)) throw Refused{};
                want_payload(depz::pack_sync_time(v.range == 1 ? v.u : static_cast<std::uint64_t>(v.i)));
            } else {
                check(false, label + ": no handler");
            }
        } else if (op == "sr04_encode") {
            if (kind == "set_sample_period") want_payload(depz::pack_sample_period(ir_u32(in, "period_us")));
            else want_payload(depz::pack_echo_decay(ir_u16(in, "decay_us")));
        } else if (op == "sr04_decode") {
            const depz::bytes p = from_hex(in.at("payload").as_string());
            const std::int64_t rpt = in.at("report").as_int();
            if (rpt == 0x91) {
                const auto d = depz::Sr04Data::unpack(p);
                check(d.source_cmd == e.at("source_cmd").as_int() &&
                          static_cast<std::int64_t>(d.timestamp_us) == e.at("timestamp_us").as_int() &&
                          d.echo_time_us == e.at("echo_time_us").as_int(), label + ": fields");
            } else if (rpt == 0x92) {
                check(depz::unpack_sample_period(p) == e.at("period_us").as_int(), label + ": period");
            } else {
                check(depz::unpack_echo_decay(p) == e.at("decay_us").as_int(), label + ": decay");
            }
        } else if (op == "framing_encode") {
            const depz::bytes f = depz::build_packet(ir_u8(in, "cmd"), from_hex(in.at("payload").as_string()),
                                                     ir_u32(in, "seq"), static_cast<depz::CrcType>(ir_u8(in, "crc_type")));
            check_eq(to_hex(f), e.at("frame").as_string(), label);
        } else if (op == "usb_identity") {
            const std::uint16_t vid = ir_u16(in, "vid"), pid = ir_u16(in, "pid");
            check(depz::is_known_depz_usb(vid, pid) == e.at("known").as_bool(), label + ": known");
        } else if (op == "bno055_encode") {
            if (kind == "read_reg") want_payload(B::pack_read_reg(ir_u8(in, "addr"), ir_u8(in, "len")));
            else want_payload(B::pack_start_stream(ir_u8(in, "trigger"), ir_u8(in, "addr"), ir_u8(in, "len"),
                                                   ir_u16(in, "period_ms")));
        } else if (op == "bno055_sensor_config") {
            const auto g = ir_need(B::GyroConfig::unpack(from_hex(in.at("bytes").as_string())));
            check(g.range == e.at("range").as_int() && g.bandwidth == e.at("bandwidth").as_int() &&
                      g.power == e.at("power").as_int(), label + ": fields");
        } else if (op == "bno055_units") {
            (void)B::Units::unpack(ir_u8(in, "unit_sel"));
            check(false, label + ": a byte unpacks; the case expects a refusal");
        } else if (op == "bno055_decode" || op == "vl53l4_decode") {
            const depz::bytes p = from_hex(in.at("payload").as_string());
            std::int64_t ts, addr, len;
            depz::bytes data;
            if (op == "bno055_decode") {
                const auto s = ir_need(B::StreamData::unpack(p));
                ts = static_cast<std::int64_t>(s.timestamp_us); addr = s.addr; len = s.len; data = s.data;
            } else {
                const auto s = ir_need(L4::StreamData::unpack(p));
                ts = static_cast<std::int64_t>(s.timestamp_us); addr = s.addr; len = s.len; data = s.data;
            }
            check(ts == e.at("timestamp_us").as_int() && addr == e.at("addr").as_int() && len == e.at("len").as_int(),
                  label + ": fields");
            check_eq(to_hex(data), e.at("data").as_string(), label + ": data");
        } else if (op == "bno086_shtp_header_pack") {
            S::ShtpHeader h;
            h.length = ir_u16(in, "length");
            h.channel = ir_u8(in, "channel");
            h.seq = ir_u8(in, "seq");
            h.continuation = in.at("continuation").as_bool();
            check_eq(to_hex(h.pack()), e.at("bytes").as_string(), label);
        } else if (op == "bno086_shtp_tx_seq") {
            S::ShtpLayer layer;
            for (const Value& p : in.at("packets").as_array())
                (void)layer.next_frame(ir_i32(p, "channel"), from_hex(p.at("payload").as_string()));
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "bno086_control_encode") {
            if (kind == "set_feature") {
                want_payload(S::sh2_build_set_feature(ir_u8(in, "sensor_id"), ir_u32(in, "interval_us"),
                                                      ir_u32(in, "batch_us"), ir_u16(in, "sensitivity"),
                                                      ir_u8(in, "flags"), ir_u32(in, "cfg_word")));
            } else if (kind == "command_request") {
                want_payload(S::sh2_build_command_request(ir_u8(in, "seq"), ir_u8(in, "command"),
                                                          from_hex(in.at("params").as_string())));
            } else if (kind == "frs_write_data") {
                std::vector<std::uint32_t> words;
                for (const Value& w : in.at("words").as_array()) {
                    if (w.type != testjson::Type::Int || w.range != 0 || w.i < 0 || w.i > 0xFFFFFFFFLL) throw Refused{};
                    words.push_back(static_cast<std::uint32_t>(w.i));
                }
                want_payload(S::sh2_build_frs_write_data(ir_u16(in, "offset_words"), words));
            } else {
                check(false, label + ": no handler");
            }
        } else if (op == "tmf8829_encode") {
            const depz::bytes p = T::pack_start_stream(ir_i32(in, "int_mask"), ir_i32(in, "flags"));
            if (p.empty()) throw std::invalid_argument("refused");
            want_payload(p);
        } else if (op == "tmf8829_config_get") {
            (void)T::cfg_get(from_hex(in.at("page").as_string()), in.at("field").as_string());
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "tmf8829_config_set") {
            depz::bytes page = from_hex(in.at("page").as_string());
            T::cfg_set(page, in.at("field").as_string(), in.at("value").as_int());
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "vl53l4_encode") {
            if (kind == "read_reg") want_payload(L4::pack_read_reg(ir_u16(in, "addr"), ir_u16(in, "len")));
            else want_payload(L4::pack_xshut(ir_u8(in, "action")));
        } else if (op == "vl53l4_timing_encode") {
            (void)ir_need(L4::range_timing_registers(ir_u32(in, "timing_budget_ms"), ir_u32(in, "inter_measurement_ms"),
                                                     ir_u16(in, "osc_frequency"), ir_u16(in, "clock_pll")));
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "vl53l4_timing_decode") {
            const auto r = ir_need(L4::decode_range_timing(ir_u32(in, "intermeasurement_raw"), ir_u16(in, "clock_pll"),
                                                            ir_u16(in, "osc_frequency"), ir_u16(in, "range_config_a")));
            check(r.timing_budget_ms == e.at("timing_budget_ms").as_int() &&
                      r.inter_measurement_ms == e.at("inter_measurement_ms").as_int(), label + ": fields");
        } else if (op == "vl53l4_tuning_encode") {
            std::int64_t raw;
            if (kind == "offset") raw = L4::offset_raw(ir_i32(in, "value"));
            else if (kind == "xtalk") raw = L4::xtalk_raw(ir_i32(in, "value"));
            else if (kind == "signal_threshold") raw = L4::signal_threshold_raw(ir_i32(in, "value"));
            else raw = ir_need(L4::sigma_threshold_raw(ir_i32(in, "value")));
            check(raw == e.at("raw").as_int(), label + ": raw");
        } else if (op == "vl53l4_tuning_decode") {
            (void)ir_u16(in, "raw");
            check(false, label + ": a u16 raw word decodes; the case expects a refusal");
        } else if (op == "vl53l7_encode") {
            if (kind == "pin_ctrl") want_payload(L7::pack_pin_ctrl(ir_u8(in, "action")));
            else if (kind == "read_reg") want_payload(L7::pack_read_reg(ir_u16(in, "addr"), ir_u16(in, "len")));
            else want_payload(L7::pack_write_reg(ir_u16(in, "addr"), from_hex(in.at("data").as_string())));
        } else if (op == "bno086_shtp_reassembly") {
            S::ShtpLayer layer;
            const auto& fr = in.at("frames").as_array();
            const auto& cap = in.at("captures_us").as_array();
            if (fr.size() != cap.size()) throw Refused{};
            for (std::size_t k = 0; k < fr.size(); ++k) {
                const Value& t = cap[k];
                if (t.type != testjson::Type::Int || t.range == 2 || (t.range == 0 && t.i < 0)) throw Refused{};
                (void)layer.feed(from_hex(fr[k].as_string()), t.range == 1 ? t.u : static_cast<std::uint64_t>(t.i));
            }
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "bno086_input_cargo" || op == "bno086_gyro_rv") {
            // capture_timestamp_us is the API's uint64_t: a negative one is unrepresentable
            const Value& t = in.at("capture_timestamp_us");
            if (t.type != testjson::Type::Int || t.range == 2 || (t.range == 0 && t.i < 0)) throw Refused{};
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "vl53lx_encode") {
            std::vector<LX::ClearStep> steps;
            for (const Value& st : in.at("clear").as_array()) {
                Value pair;
                pair.type = testjson::Type::Object;
                if (st.as_array().size() != 2) throw Refused{};
                pair.obj["a"] = st.as_array()[0];
                pair.obj["v"] = st.as_array()[1];
                steps.push_back({ir_u16(pair, "a"), ir_u8(pair, "v")});
            }
            want_payload(ir_need(LX::pack_start_stream(ir_u16(in, "addr"), ir_u16(in, "len"), steps, ir_u8(in, "flags"))));
        } else if (op == "vl53l8_detection_thresholds") {
            std::vector<L8::DetectionThreshold> th;
            for (const Value& t : in.at("thresholds").as_array()) {
                L8::DetectionThreshold d;
                d.low_thresh = ir_i32(t, "low_thresh");
                d.high_thresh = ir_i32(t, "high_thresh");
                d.measurement = ir_u8(t, "measurement");
                d.type = ir_u8(t, "type");
                d.zone_num = ir_u8(t, "zone_num");
                d.operation = ir_u8(t, "operation");
                th.push_back(d);
            }
            (void)L8::pack_detection_thresholds(th);
            check(false, label + ": accepted, the case expects a refusal");
        } else if (op == "vl53l8_xtalk_margin") {
            const Value& v = in.at("kcps");
            if (v.type != testjson::Type::Int && v.type != testjson::Type::Double) throw Refused{};
            check(L8::xtalk_margin_raw(v.as_double()) == e.at("raw").as_u64(), label + ": raw");
        } else if (op == "vl53l8_cnh_decode") {
#ifdef DEPZ_TEST_HAS_IO
            const Value& c = in.at("config");
            const auto& am = c.at("agg_map").as_array();
            depz::CnhSetup setup;
            setup.init_config(ir_i32(c, "start_bin"), ir_i32(c, "num_bins"), ir_i32(c, "sub_sample"));
            if (am.size() != 6) throw Refused{};
            int m[6];
            for (int k = 0; k < 6; ++k) {
                if (am[k].type != testjson::Type::Int || am[k].range != 0 || am[k].i < INT32_MIN || am[k].i > INT32_MAX)
                    throw Refused{};
                m[k] = static_cast<int>(am[k].i);
            }
            setup.create_agg_map(ir_i32(c, "resolution"), m[0], m[1], m[2], m[3], m[4], m[5]);
            check(false, label + ": accepted, the case expects a refusal");
#else
            check(false, label + ": built without the live-hardware layer (depz::CnhSetup)");
#endif
        } else {
            check(false, label + ": no input_rules handler for op " + op);
        }
    } catch (const Refused&) {
        return true;
    } catch (const std::exception&) {
        return true;
    }
    return false;
}
}  // namespace

static void test_input_rules() {
    for (const char* file : {"common_commands.json", "sr04.json", "framing_encode.json", "usb_ids.json", "bno055.json",
                             "bno086_shtp.json", "tmf8829.json", "vl53l4.json", "vl53l7.json", "vl53lx.json",
                             "vl53l8_advanced.json"}) {
        Value v = load_vector(file);
        if (!v.has("input_rules")) continue;
        for (const Value& c : v.at("input_rules").as_array()) {
            const int f0 = g_fails;
            const std::string id = c.at("id").as_string();
            const Value& e = c.at("expect");
            const bool want_invalid = e.has("error");
            const bool rejected = ir_run(id, c.at("op").as_string(), c.at("in"), e);
            check(rejected == want_invalid, id + (rejected ? ": rejected" : ": accepted") +
                                                (want_invalid ? ", expected a rejection" : ", expected a decode"));
            seen(c, f0);
        }
    }
}

int main(int argc, char** argv) {
    std::string suite = argc > 1 ? argv[1] : "foundation";
    if (suite == "vl53l8_frame") {
        test_vl53l8_frame();
    } else if (suite == "vl53l8_advanced") {
        test_vl53l8_advanced();
    } else if (suite == "vl53l8_cnh") {
        test_vl53l8_cnh();
    } else if (suite == "vl53l7_codecs") {
        test_vl53l7_codecs();
    } else if (suite == "vl53l7_frame") {
        test_vl53l7_frame();
    } else if (suite == "vl53l4") {
        test_vl53l4();
    } else if (suite == "vl53lx") {
        test_vl53lx();
    } else if (suite == "bno055") {
        test_bno055();
    } else if (suite == "tmf8829") {
        test_tmf8829();
    } else if (suite == "tmf8829_frame_time") {
        test_tmf8829_frame_time();
    } else if (suite == "bno086_shtp") {
        test_bno086_shtp();
    } else if (suite == "bno086_reports") {
        test_bno086_reports();
    } else if (suite == "dataset") {
        test_dataset();
    } else if (suite == "input_rules") {
        test_input_rules();
    } else if (suite == "foundation") {
        return run_foundation();
    } else {
        // An unknown selector used to fall through to the foundation suite, so
        // a ctest target naming a missing suite passed without checking it.
        std::cerr << "unknown test suite: " << suite << "\n";
        return 2;
    }
    std::cout << "checks: " << g_checks << ", failures: " << g_fails << "\n";
    if (g_fails) {
        std::cerr << "VECTOR PARITY FAILED (" << suite << ")\n";
        return 1;
    }
    std::cout << "ALL VECTORS PASS (" << suite << ")\n";
    return 0;
}

static int run_foundation() {
    test_crc();
    test_framing_encode();
    test_framing_decode();
    test_usb_ids();
    test_common_commands();
    test_identity();
    test_probe();
    test_sr04();
    test_fwdepz();

    std::cout << "checks: " << g_checks << ", failures: " << g_fails << "\n";
    if (g_fails) {
        std::cerr << "VECTOR PARITY FAILED\n";
        return 1;
    }
    std::cout << "ALL VECTORS PASS\n";
    return 0;
}
