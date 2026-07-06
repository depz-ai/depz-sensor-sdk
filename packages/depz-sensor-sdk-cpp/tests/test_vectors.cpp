// Vector-driven parity test runner: loads contracts/vectors/*.json and asserts
// the C++ SDK is byte-exact with the golden vectors.
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "depz/bno086.hpp"
#include "depz/common.hpp"
#include "depz/crc.hpp"
#include "depz/dataset.hpp"
#include "depz/framing.hpp"
#include "depz/fwdepz.hpp"
#include "depz/identity.hpp"
#include "depz/sr04.hpp"
#include "depz/usb_ids.hpp"
#include "depz/vl53l8.hpp"
#include "json.hpp"

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
    }
}

// ---- framing encode --------------------------------------------------------
static void test_framing_encode() {
    Value v = load_vector("framing_encode.json");
    for (const Value& c : v.at("cases").as_array()) {
        std::string nm = c.at("name").as_string();
        depz::bytes payload = from_hex(c.at("payload").as_string());
        auto cmd = static_cast<std::uint8_t>(c.at("cmd").as_int());
        auto seq = static_cast<std::uint8_t>(c.at("seq").as_int());
        auto crc_type = static_cast<depz::CrcType>(c.at("crc_type").as_int());
        depz::bytes frame = depz::build_packet(cmd, depz::as_bytes(payload), seq, crc_type);
        check_eq(to_hex(frame), c.at("frame").as_string(), "framing_encode " + nm);
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
    }
}

// ---- usb ids ---------------------------------------------------------------
static void test_usb_ids() {
    Value v = load_vector("usb_ids.json");
    for (const Value& c : v.at("identity").as_array()) {
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
    }
    // None inputs.
    check(!depz::is_known_depz_usb(std::nullopt, 0xEC78), "is_known null vid");
    check(!depz::is_known_depz_usb(0x1BCF, std::nullopt), "is_known null pid");

    for (const Value& s : v.at("serial_ordering").as_array()) {
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
    }
}

// ---- common commands -------------------------------------------------------
static void test_common_commands() {
    Value v = load_vector("common_commands.json");
    for (const Value& c : v.at("encode").as_array()) {
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
    }

    for (const Value& c : v.at("decode").as_array()) {
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
    }

    for (const Value& c : v.at("sync_time_math").as_array()) {
        std::string nm = c.at("name").as_string();
        auto res = depz::sync_time_offset_rtt(c.at("t1").as_int(), c.at("t2").as_int(),
                                              c.at("t3").as_int(), c.at("t4").as_int());
        check_eq(res.first, c.at("offset_us").as_int(), "sync_time offset " + nm);
        check_eq(res.second, c.at("rtt_us").as_int(), "sync_time rtt " + nm);
    }
}

// ---- identity --------------------------------------------------------------
static void test_identity() {
    Value v = load_vector("identity.json");
    for (const Value& c : v.at("cases").as_array()) {
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
    }
}

// ---- sr04 ------------------------------------------------------------------
static void test_sr04() {
    Value v = load_vector("sr04.json");
    for (const Value& c : v.at("encode").as_array()) {
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
    }

    for (const Value& c : v.at("decode").as_array()) {
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
    }
}

// ---- fwdepz ----------------------------------------------------------------
static void test_fwdepz() {
    Value v = load_vector("fwdepz.json");
    for (const Value& c : v.at("cases").as_array()) {
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
    }
}

// ---- vl53l8 frame reassembly + decode (recording replay) -------------------
// The replay capture is from a VL53L8CX (dev-default) device (software_name
// APP_VL53L8_v0.9). The results-frame layout is shared with the VL53L8CH, so the same decode path
// covers both; only the footer-id offset is variant-specific (see the decode
// call below for which offset this firmware's frames carry).
static void test_vl53l8_frame() {
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
}

// ---- vl53l8 advanced DCI codecs (shared by CX and CH) ----------------------
static void test_vl53l8_advanced() {
    Value v = load_vector("vl53l8_advanced.json");
    for (const Value& m : v.at("motion").as_array()) {
        std::string nm = m.at("name").as_string();
        auto cfg = depz::vl53l8::motion_config_init(static_cast<int>(m.at("resolution").as_int()));
        check_eq(to_hex(cfg.pack()), m.at("pack").as_string(), "vl53l8 motion " + nm);
    }
    for (const Value& t : v.at("thresholds").as_array()) {
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
    }
    for (const Value& x : v.at("xtalk_margin").as_array()) {
        std::string nm = x.at("name").as_string();
        check_eq(static_cast<std::int64_t>(depz::vl53l8::xtalk_margin_raw(x.at("kcps").as_double())),
                 x.at("raw").as_int(), "vl53l8 xtalk " + nm);
    }
}

// ---- vl53l8 CNH (Compact Network Histogram) decode — VL53L8CH only ----------
static void test_vl53l8_cnh() {
    Value v = load_vector("vl53l8_cnh.json");
    const Value& cfg = v.at("config");
    int nb_agg = static_cast<int>(cfg.at("nb_of_aggregates").as_int());
    int feat = static_cast<int>(cfg.at("feature_length").as_int());
    depz::bytes raw = from_hex(v.at("cnh_raw").as_string());

    auto decoded = depz::vl53l8::decode_cnh(nb_agg, feat, depz::as_bytes(raw));

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
}

// ---- bno086 SHTP framing ---------------------------------------------------
static void test_bno086_shtp() {
    Value v = load_vector("bno086_shtp.json");

    for (const Value& h : v.at("header").as_array()) {
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
    }

    // Per-channel TX sequence counters.
    depz::bno086::ShtpLayer tx;
    int idx = 0;
    for (const Value& t : v.at("tx_seq").as_array()) {
        depz::bytes payload = from_hex(t.at("payload").as_string());
        int channel = static_cast<int>(t.at("channel").as_int());
        check_eq(to_hex(tx.next_frame(channel, depz::as_bytes(payload))),
                 t.at("frame").as_string(), "shtp tx_seq #" + std::to_string(idx++));
    }

    // RX reassembly.
    for (const Value& c : v.at("reassembly").as_array()) {
        std::string nm = c.at("name").as_string();
        depz::bno086::ShtpLayer rx;
        std::vector<depz::bno086::ShtpCargo> got;
        for (const Value& fr : c.at("frames").as_array()) {
            depz::bytes frame = from_hex(fr.as_string());
            auto cargo = rx.feed(depz::as_bytes(frame));
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
        }
        check_eq(static_cast<std::int64_t>(rx.discarded), e.at("discarded").as_int(),
                 "shtp reassembly discarded " + nm);
    }

    // SH-2 control-channel encoders.
    for (const Value& c : v.at("control_encode").as_array()) {
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
    }
}

// ---- bno086 report parsers -------------------------------------------------
static std::string report_type_name(depz::bno086::ReportType t) {
    using RT = depz::bno086::ReportType;
    switch (t) {
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
        case RT::UnknownReport: return "UnknownReport";
    }
    return "?";
}

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

static void test_bno086_reports() {
    Value v = load_vector("bno086_reports.json");
    for (const Value& c : v.at("input_cargos").as_array()) {
        std::string nm = c.at("name").as_string();
        depz::bytes cargo = from_hex(c.at("cargo").as_string());
        auto reports = depz::bno086::parse_input_cargo(
            depz::as_bytes(cargo), c.at("capture_timestamp_us").as_int());
        const auto& want = c.at("expect").as_array();
        check_eq(reports.size(), want.size(), "bno086 reports count " + nm);
        std::size_t n = std::min(reports.size(), want.size());
        for (std::size_t k = 0; k < n; ++k)
            check_report(reports[k], want[k], nm + " #" + std::to_string(k));
    }
    for (const Value& c : v.at("gyro_rv").as_array()) {
        std::string nm = c.at("name").as_string();
        depz::bytes cargo = from_hex(c.at("cargo").as_string());
        auto r = depz::bno086::parse_gyro_rv_cargo(depz::as_bytes(cargo),
                                                   c.at("capture_timestamp_us").as_int());
        if (!r) {
            check(false, "bno086 gyro_rv parsed " + nm);
            continue;
        }
        check_report(*r, c.at("expect"), "bno086 gyro_rv " + nm);
    }
}

// ---- dataset read (.depzdata, contract 09) ---------------------------------
static void test_dataset() {
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
        check_eq(d0->second.time_sync.offset_us, static_cast<std::int64_t>(-8524738852),
                 "dataset d0 offset_us");
        check_eq(d0->second.time_sync.rtt_us, static_cast<std::int64_t>(13), "dataset d0 rtt_us");
    }
    if (d1 != ds.devices.end())
        check_eq(d1->second.time_sync.offset_us, static_cast<std::int64_t>(-3525739108),
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
}

static int run_foundation();

int main(int argc, char** argv) {
    std::string suite = argc > 1 ? argv[1] : "foundation";
    if (suite == "vl53l8_frame") {
        test_vl53l8_frame();
    } else if (suite == "vl53l8_advanced") {
        test_vl53l8_advanced();
    } else if (suite == "vl53l8_cnh") {
        test_vl53l8_cnh();
    } else if (suite == "bno086_shtp") {
        test_bno086_shtp();
    } else if (suite == "bno086_reports") {
        test_bno086_reports();
    } else if (suite == "dataset") {
        test_dataset();
    } else {
        return run_foundation();
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
