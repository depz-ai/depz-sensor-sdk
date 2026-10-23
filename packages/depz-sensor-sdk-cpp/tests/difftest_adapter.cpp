// Differential-testing adapter for the DEPZ C++ SDK (contracts/16_DIFFTEST.md).
//
// A long-lived process speaking JSON Lines on stdin/stdout:
//   {"id": n, "op": "<op>", "in": {...}}  ->  {"id": n, "out": ...}
//                                         or  {"id": n, "error": "invalid" | "unsupported"}
// `invalid` = the SDK rejected the input (an exception, an empty optional);
// `unsupported` = this SDK has no function for the op. Built as the CMake
// target `depz_difftest_adapter` of the test build; the driver
// (scripts/difftest.py) builds and starts it. Everything goes through the C++
// API (depz/*.hpp), never the C SDK underneath. Output shapes are the
// golden-vector `expect` shapes the vector runner (test_vectors.cpp) asserts.
//
// Arguments: the API's parameter types are the argument range a C++ caller can
// pass at all. An input outside the parameter's type (300 for a uint8_t, a
// negative value for an unsigned, 2^64 anywhere, null for a std::string) is
// answered `invalid` — the call cannot be written — rather than silently
// wrapped into a different argument. Inside the type, the SDK decides.
//
// Declared gaps (`unsupported`):
//   bno086_record     — no contract 09 dataset recorder: depz::dataset only
//                       reads datasets, nothing builds a record value.
//   vl53l8_cnh_decode — only when built without the live-hardware layer
//                       (depz::CnhSetup lives in depz/device.hpp).
//
// The vl53l8_* advanced ops: the test tree has no fake VL53L8 DCI platform, so
// they call the codecs the Vl53l8 methods write with — motion_config_init
// (motion_indicator_init; the resolution check is in it),
// pack_detection_thresholds + detection_thresholds_valid_status (what
// set_detection_thresholds writes) and xtalk_margin_raw (its 0..10000 check is
// the refusal Vl53l8::set_xtalk_margin applies before the write).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "depz/bno055.hpp"
#include "depz/bno086.hpp"
#include "depz/common.hpp"
#include "depz/crc.hpp"
#include "depz/framing.hpp"
#include "depz/fwdepz.hpp"
#include "depz/identity.hpp"
#include "depz/sr04.hpp"
#include "depz/tmf8829.hpp"
#include "depz/usb_ids.hpp"
#include "depz/vl53l4.hpp"
#include "depz/vl53l7.hpp"
#include "depz/vl53l8.hpp"
#include "depz/vl53lx.hpp"
#ifdef DEPZ_DIFFTEST_HAS_IO
#include "depz/device.hpp"
#endif
#include "json.hpp"
#include "vector_helpers.hpp"

namespace {

using testjson::Type;
using testjson::Value;

struct Invalid : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// ── JSON output ────────────────────────────────────────────────────────────

class Out {
public:
    std::string s;

    void obj() { sep(); s += '{'; first_.push_back(true); }
    void end_obj() { first_.pop_back(); s += '}'; }
    void arr() { sep(); s += '['; first_.push_back(true); }
    void end_arr() { first_.pop_back(); s += ']'; }
    Out& key(const char* k) { sep(); str_raw(k); s += ':'; after_key_ = true; return *this; }
    void null() { sep(); s += "null"; }
    void boolean(bool b) { sep(); s += b ? "true" : "false"; }
    void i64(std::int64_t v) { sep(); s += std::to_string(v); }
    void u64(std::uint64_t v) { sep(); s += std::to_string(v); }
    void str(const std::string& v) { sep(); str_raw(v); }
    void hex(depz::byte_span b) {
        static const char* H = "0123456789abcdef";
        sep();
        s += '"';
        for (std::byte x : b) {
            const auto c = std::to_integer<unsigned>(x);
            s += H[c >> 4];
            s += H[c & 15];
        }
        s += '"';
    }
    void raw(const std::string& t) { sep(); s += t; }

private:
    std::vector<bool> first_;
    bool after_key_ = false;

    void sep() {
        if (after_key_) { after_key_ = false; return; }
        if (!first_.empty()) {
            if (!first_.back()) s += ',';
            first_.back() = false;
        }
    }
    void esc(std::uint32_t u) {
        char b[8];
        std::snprintf(b, sizeof b, "\\u%04x", static_cast<unsigned>(u));
        s += b;
    }
    // A std::string's bytes read as UTF-8 the way a WHATWG TextDecoder reads
    // them (each maximal invalid subsequence -> U+FFFD), non-ASCII as \u escapes.
    void str_raw(const std::string& t) {
        const std::size_t n = t.size();
        std::size_t i = 0;
        s += '"';
        while (i < n) {
            const unsigned c = static_cast<unsigned char>(t[i]);
            if (c < 0x80) {
                if (c == '"' || c == '\\') { s += '\\'; s += static_cast<char>(c); }
                else if (c < 0x20 || c == 0x7F) esc(c);
                else s += static_cast<char>(c);
                ++i;
                continue;
            }
            std::uint32_t cp;
            int need, k;
            unsigned lo = 0x80, hi = 0xBF;
            if (c >= 0xC2 && c <= 0xDF) { need = 1; cp = c & 0x1F; }
            else if (c >= 0xE0 && c <= 0xEF) { need = 2; cp = c & 0x0F; if (c == 0xE0) lo = 0xA0; if (c == 0xED) hi = 0x9F; }
            else if (c >= 0xF0 && c <= 0xF4) { need = 3; cp = c & 0x07; if (c == 0xF0) lo = 0x90; if (c == 0xF4) hi = 0x8F; }
            else { esc(0xFFFD); ++i; continue; }
            std::size_t j = i + 1;
            for (k = 0; k < need && j < n; ++k, ++j) {
                const unsigned b = static_cast<unsigned char>(t[j]);
                if (b < lo || b > hi) break;
                lo = 0x80; hi = 0xBF;
                cp = (cp << 6) | (b & 0x3F);
            }
            i = j;
            if (k < need) { esc(0xFFFD); continue; }
            if (cp < 0x10000) esc(cp);
            else { cp -= 0x10000; esc(0xD800 | (cp >> 10)); esc(0xDC00 | (cp & 0x3FF)); }
        }
        s += '"';
    }
};

// ── argument readers ───────────────────────────────────────────────────────

const Value& field(const Value& in, const char* k) {
    if (in.type != Type::Object || !in.has(k)) throw Invalid(std::string("missing field ") + k);
    return in.at(k);
}

std::int64_t ival(const Value& v, const char* k, std::int64_t lo, std::int64_t hi) {
    if (v.type != Type::Int) throw Invalid(std::string(k) + ": not an integer");
    if (v.range != 0 || v.i < lo || v.i > hi) throw Invalid(std::string(k) + ": outside the C++ parameter type");
    return v.i;
}
std::int64_t iarg(const Value& in, const char* k, std::int64_t lo, std::int64_t hi) {
    return ival(field(in, k), k, lo, hi);
}
std::uint8_t U8(const Value& in, const char* k) { return static_cast<std::uint8_t>(iarg(in, k, 0, 0xFF)); }
std::uint16_t U16(const Value& in, const char* k) { return static_cast<std::uint16_t>(iarg(in, k, 0, 0xFFFF)); }
std::uint32_t U32(const Value& in, const char* k) { return static_cast<std::uint32_t>(iarg(in, k, 0, 0xFFFFFFFFLL)); }
int I32(const Value& in, const char* k) { return static_cast<int>(iarg(in, k, INT32_MIN, INT32_MAX)); }
std::int64_t I64(const Value& in, const char* k) { return iarg(in, k, INT64_MIN, INT64_MAX); }

std::uint64_t uval(const Value& v, const char* k) {
    if (v.type != Type::Int) throw Invalid(std::string(k) + ": not an integer");
    if (v.range == 1) return v.u;
    if (v.range != 0 || v.i < 0) throw Invalid(std::string(k) + ": outside uint64");
    return static_cast<std::uint64_t>(v.i);
}
std::uint64_t U64(const Value& in, const char* k) { return uval(field(in, k), k); }

bool barg(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type != Type::Bool) throw Invalid(std::string(k) + ": not a boolean");
    return v.b;
}
double darg(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type == Type::Double) return v.d;
    if (v.type != Type::Int) throw Invalid(std::string(k) + ": not a number");
    return v.range == 1 ? static_cast<double>(v.u) : static_cast<double>(v.i);
}
const std::string& sarg(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type != Type::String) throw Invalid(std::string(k) + ": not a string");
    return v.s;
}
std::optional<std::string> snarg(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type == Type::Null) return std::nullopt;
    if (v.type != Type::String) throw Invalid(std::string(k) + ": not a string");
    return v.s;
}
const testjson::Array& larg(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type != Type::Array) throw Invalid(std::string(k) + ": not a list");
    return v.arr;
}

int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
depz::bytes hex_of(const Value& v, const char* k) {
    if (v.type != Type::String) throw Invalid(std::string(k) + ": not a hex string");
    const std::string& h = v.s;
    if (h.size() % 2) throw Invalid(std::string(k) + ": odd hex length");
    depz::bytes out(h.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const int a = hexval(h[2 * i]), b = hexval(h[2 * i + 1]);
        if (a < 0 || b < 0) throw Invalid(std::string(k) + ": not a hex string");
        out[i] = static_cast<std::byte>(a << 4 | b);
    }
    return out;
}
depz::bytes HEX(const Value& in, const char* k) { return hex_of(field(in, k), k); }

// The value of an optional result; empty = the SDK refused the input. By value:
// callers bind it to `const auto&` and the temporary optional is gone by then.
template <class T>
T need(std::optional<T> v, const char* what) {
    if (!v) throw Invalid(std::string(what) + " refused");
    return std::move(*v);
}

// ── ops ────────────────────────────────────────────────────────────────────

using OpFn = std::function<void(const Value&, Out&)>;

void payload(Out& o, const depz::bytes& p) {
    o.obj();
    o.key("payload").hex(p);
    o.end_obj();
}

void op_crc(const Value& in, Out& o) {
    const depz::bytes b = HEX(in, "input");
    o.obj();
    o.key("crc8_maxim").u64(depz::crc8_maxim(b));
    o.key("crc16_modbus").u64(depz::crc16_modbus(b));
    o.key("crc32_iso_hdlc").u64(depz::crc32_iso_hdlc(b));
    o.key("crc16_ccitt_false").u64(depz::crc16_ccitt_false(b));
    o.end_obj();
}

void op_framing_encode(const Value& in, Out& o) {
    const std::uint8_t cmd = U8(in, "cmd");
    // `seq` is the API's u32 counter; the SDK takes it modulo 256 on the wire
    // (framing_encode seq_wraps_modulo_256)
    const std::uint32_t seq = U32(in, "seq");
    const auto ct = static_cast<depz::CrcType>(U8(in, "crc_type"));
    const depz::bytes p = HEX(in, "payload");
    const depz::bytes f = depz::build_packet(cmd, p, seq, ct);
    o.obj();
    o.key("frame").hex(f);
    o.end_obj();
}

void op_framing_decode(const Value& in, Out& o) {
    std::vector<depz::bytes> chunks;
    if (in.has("chunks")) {
        for (const Value& c : larg(in, "chunks")) chunks.push_back(hex_of(c, "chunks"));
    } else {
        chunks.push_back(HEX(in, "stream"));
    }
    depz::PacketParser p;
    depz::bytes trash;
    o.obj();
    o.key("events");
    o.arr();
    for (const auto& c : chunks) {
        for (const auto& ev : p.feed(c)) {
            if (const auto* pk = std::get_if<depz::Packet>(&ev)) {
                o.obj();
                o.key("type").str("packet");
                o.key("cmd").u64(pk->cmd);
                o.key("seq").u64(pk->seq);
                o.key("payload").hex(pk->payload);
                o.end_obj();
            } else if (const auto* ce = std::get_if<depz::CrcError>(&ev)) {
                o.obj();
                o.key("type").str("crc_error");
                o.key("cmd").u64(ce->cmd);
                o.key("seq").u64(ce->seq);
                o.end_obj();
            } else if (const auto* t = std::get_if<depz::Trash>(&ev)) {
                trash.insert(trash.end(), t->data.begin(), t->data.end());
            }
        }
    }
    o.end_arr();
    o.key("trash").hex(trash);
    o.key("residue").hex(p.residue());
    o.key("header_errors").u64(p.header_errors);
    o.end_obj();
}

void op_identity(const Value& in, Out& o) {
    const depz::bytes raw = HEX(in, "raw");
    const depz::Identity id = depz::parse_software_name(depz::strip_device_string(raw));
    o.obj();
    o.key("mode").str(depz::to_string(id.mode));
    o.key("sensor_type");
    if (id.sensor_type) o.str(depz::to_string(*id.sensor_type)); else o.null();
    o.key("software_name").str(id.software_name);
    o.key("version").str(id.version);
    o.end_obj();
}

std::optional<std::uint16_t> opt16(const Value& in, const char* k) {
    const Value& v = field(in, k);
    if (v.type == Type::Null) return std::nullopt;
    return static_cast<std::uint16_t>(ival(v, k, 0, 0xFFFF));
}

void op_usb_identity(const Value& in, Out& o) {
    const auto vid = opt16(in, "vid"), pid = opt16(in, "pid");
    const auto model = depz::usb_model_hint(vid, pid);
    o.obj();
    o.key("known").boolean(depz::is_known_depz_usb(vid, pid));
    o.key("model");
    if (model) o.str(*model); else o.null();
    o.end_obj();
}

void op_usb_serial_ordering(const Value& in, Out& o) {
    std::vector<depz::PortInfo> ports;
    for (const Value& p : larg(in, "ports")) ports.push_back({sarg(p, "port"), snarg(p, "serial")});
    o.obj();
    o.key("order");
    o.arr();
    for (const auto& p : depz::order_by_serial(ports)) o.str(p.port);
    o.end_arr();
    o.end_obj();
}

void op_probe_resolve(const Value& in, Out& o) {
    const std::string cls = depz::resolve_sensor_class(sarg(in, "software_name"), sarg(in, "device_name"),
                                                       opt16(in, "usb_vid"), opt16(in, "usb_pid"));
    o.obj();
    o.key("class").str(cls);
    o.end_obj();
}

void op_common_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "sync_time_request") return payload(o, depz::pack_sync_time(U64(in, "pc_timestamp_us")));
    if (kind == "set_payload_crc_type") {
        // no codec check of its own: CrcType is the argument's type
        const std::uint8_t ct = U8(in, "crc_type");
        if (ct > static_cast<std::uint8_t>(depz::CrcType::Crc32)) throw Invalid("not a CrcType");
        return payload(o, depz::pack_set_payload_crc_type(ct));
    }
    if (kind == "sync_pin_config") {
        depz::SyncPinConfig c{U8(in, "pin"), static_cast<depz::SyncPinMode>(U8(in, "mode")),
                              static_cast<depz::SyncPinPolarity>(U8(in, "polarity"))};
        return payload(o, c.pack());
    }
    throw Invalid("kind " + kind);
}

void op_common_decode(const Value& in, Out& o) {
    const std::int64_t rpt = I64(in, "report");
    const depz::bytes p = HEX(in, "payload");
    o.obj();
    if (rpt == 0x80) {
        const auto r = depz::StatusReport::unpack(p);
        o.key("cmd").u64(r.cmd);
        o.key("status").u64(r.status);
    } else if (rpt == 0x81) {
        const auto r = depz::TextReport::unpack(p);
        o.key("cmd").u64(r.cmd);
        o.key("text").str(r.text);
    } else if (rpt == 0x83) {
        const auto r = depz::TemperatureReport::unpack(p);
        o.key("timestamp_us").u64(r.timestamp_us);
        o.key("raw_decidegrees").i64(r.raw_decidegrees);
    } else if (rpt == 0x84) {
        const auto r = depz::SequenceErrorReport::unpack(p);
        o.key("expected_seq").u64(r.expected_seq);
        o.key("received_seq").u64(r.received_seq);
    } else {
        throw Invalid("report");
    }
    o.end_obj();
}

void op_sync_time_math(const Value& in, Out& o) {
    const auto r = depz::sync_time_offset_rtt(I64(in, "t1"), I64(in, "t2"), I64(in, "t3"), I64(in, "t4"));
    o.obj();
    o.key("offset_us").i64(r.first);
    o.key("rtt_us").i64(r.second);
    o.end_obj();
}

void op_sr04_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "set_sample_period") return payload(o, depz::pack_sample_period(U32(in, "period_us")));
    if (kind == "set_echo_decay") return payload(o, depz::pack_echo_decay(U16(in, "decay_us")));
    throw Invalid("kind " + kind);
}

void op_sr04_decode(const Value& in, Out& o) {
    const std::int64_t rpt = I64(in, "report");
    const depz::bytes p = HEX(in, "payload");
    o.obj();
    if (rpt == 0x91) {
        const auto d = depz::Sr04Data::unpack(p);
        o.key("source_cmd").u64(d.source_cmd);
        o.key("timestamp_us").u64(d.timestamp_us);
        o.key("echo_time_us").u64(d.echo_time_us);
    } else if (rpt == 0x92) {
        o.key("period_us").u64(depz::unpack_sample_period(p));
    } else if (rpt == 0x93) {
        o.key("decay_us").u64(depz::unpack_echo_decay(p));
    } else {
        throw Invalid("report");
    }
    o.end_obj();
}

void op_fwdepz_parse(const Value& in, Out& o) {
    const auto img = depz::FwDepzImage::parse(HEX(in, "file"));
    o.obj();
    o.key("load_addr").u64(img.load_addr);
    o.key("fw_size").u64(img.fw_size);
    o.key("fw_crc32").u64(img.fw_crc32);
    o.key("cur_sec").u64(img.cur_sec);
    o.key("tot_sec").u64(img.tot_sec);
    o.key("payload_crc_ok").boolean(img.payload_crc_ok());
    o.end_obj();
}

// ── BNO055 ──

namespace B = depz::bno055;

void op_bno055_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "reset" || kind == "stop_stream" || kind == "get_info") return payload(o, {});
    if (kind == "read_reg") return payload(o, B::pack_read_reg(U8(in, "addr"), U8(in, "len")));
    if (kind == "write_reg") {
        const std::uint8_t addr = U8(in, "addr");
        return payload(o, B::pack_write_reg(addr, HEX(in, "data")));
    }
    if (kind == "start_stream")
        return payload(o, B::pack_start_stream(U8(in, "trigger"), U8(in, "addr"), U8(in, "len"), U16(in, "period_ms")));
    throw Invalid("kind " + kind);
}

void op_bno055_decode(const Value& in, Out& o) {
    const std::int64_t rpt = I64(in, "report");
    const depz::bytes p = HEX(in, "payload");
    o.obj();
    if (rpt == 0x92) {
        const auto& i = need(B::Bno055Info::unpack(p), "Bno055Info::unpack");
        o.key("i2c_addr").u64(i.i2c_addr); o.key("chip_id").u64(i.chip_id); o.key("acc_id").u64(i.acc_id);
        o.key("mag_id").u64(i.mag_id); o.key("gyr_id").u64(i.gyr_id); o.key("sw_rev").u64(i.sw_rev);
        o.key("bl_rev").u64(i.bl_rev); o.key("initialized").u64(i.initialized); o.key("int_level").u64(i.int_level);
        o.key("int_edges").u64(i.int_edges); o.key("read_min_us").u64(i.read_min_us);
        o.key("read_max_us").u64(i.read_max_us); o.key("read_avg_us").u64(i.read_avg_us);
        o.key("tx_dropped").u64(i.tx_dropped); o.key("i2c_errors").u64(i.i2c_errors);
        o.key("slots_skipped").u64(i.slots_skipped); o.key("bus_recoveries").u64(i.bus_recoveries);
        o.key("last_i2c_error").u64(i.last_i2c_error); o.key("sensor_resets").u64(i.sensor_resets);
        o.key("loop_max_us").u64(i.loop_max_us);
    } else if (rpt == 0x91) {
        const auto& r = need(B::RegData::unpack(p), "RegData::unpack");
        o.key("cmd").u64(r.cmd);
        o.key("timestamp_us").u64(r.timestamp_us);
        o.key("data").hex(r.data);
    } else if (rpt == 0x93) {
        const auto& s = need(B::StreamData::unpack(p), "StreamData::unpack");
        o.key("timestamp_us").u64(s.timestamp_us);
        o.key("addr").u64(s.addr);
        o.key("len").u64(s.len);
        o.key("data").hex(s.data);
    } else {
        throw Invalid("report");
    }
    o.end_obj();
}

void op_bno055_units(const Value& in, Out& o) {
    const auto u = B::Units::unpack(U8(in, "unit_sel"));
    o.obj();
    o.key("accel_mg").boolean(u.accel_mg);
    o.key("gyro_rps").boolean(u.gyro_rps);
    o.key("euler_rad").boolean(u.euler_rad);
    o.key("temp_f").boolean(u.temp_f);
    o.key("android").boolean(u.android);
    o.key("repack").u64(u.pack());
    o.end_obj();
}

void op_bno055_calib_stat(const Value& in, Out& o) {
    const auto s = B::CalibStatus::unpack(U8(in, "value"));
    o.obj();
    o.key("system").u64(s.system);
    o.key("gyro").u64(s.gyro);
    o.key("accel").u64(s.accel);
    o.key("mag").u64(s.mag);
    o.key("fully_calibrated").boolean(s.fully_calibrated());
    o.key("repack").u64(s.pack());
    o.end_obj();
}

template <class A>
void ints(Out& o, const char* k, const A& a) {
    o.key(k);
    o.arr();
    for (auto x : a) o.i64(static_cast<std::int64_t>(x));
    o.end_arr();
}

void op_bno055_calibration_profile(const Value& in, Out& o) {
    const auto& p = need(B::CalibrationProfile::unpack(HEX(in, "bytes")), "CalibrationProfile::unpack");
    o.obj();
    ints(o, "accel_offset", p.accel_offset);
    ints(o, "mag_offset", p.mag_offset);
    ints(o, "gyro_offset", p.gyro_offset);
    o.key("accel_radius").i64(p.accel_radius);
    o.key("mag_radius").i64(p.mag_radius);
    o.key("repack").hex(p.pack());
    o.end_obj();
}

void repack_pair(Out& o, const B::AxisRemap& a) {
    const auto& pr = need(a.pack(), "AxisRemap::pack");
    o.key("repack");
    o.arr();
    o.u64(pr.first);
    o.u64(pr.second);
    o.end_arr();
}

void op_bno055_axis_remap(const Value& in, Out& o) {
    const auto a = B::AxisRemap::unpack(U8(in, "config"), U8(in, "sign"));
    o.obj();
    o.key("x").u64(a.x);
    o.key("y").u64(a.y);
    o.key("z").u64(a.z);
    o.key("x_negative").boolean(a.x_negative);
    o.key("y_negative").boolean(a.y_negative);
    o.key("z_negative").boolean(a.z_negative);
    repack_pair(o, a);
    o.end_obj();
}

void op_bno055_axis_remap_pack(const Value& in, Out& o) {
    B::AxisRemap a;
    a.x = U8(in, "x");
    a.y = U8(in, "y");
    a.z = U8(in, "z");
    a.x_negative = barg(in, "x_negative");
    a.y_negative = barg(in, "y_negative");
    a.z_negative = barg(in, "z_negative");
    o.obj();
    repack_pair(o, a);
    o.end_obj();
}

void op_bno055_sensor_config(const Value& in, Out& o) {
    const std::string& sensor = sarg(in, "sensor");
    o.obj();
    if (sensor == "accel") {
        const auto a = B::AccelConfig::unpack(U8(in, "value"));
        o.key("range").u64(a.range); o.key("bandwidth").u64(a.bandwidth); o.key("power").u64(a.power);
        o.key("repack").u64(a.pack());
    } else if (sensor == "gyro") {
        const auto& g = need(B::GyroConfig::unpack(HEX(in, "bytes")), "GyroConfig::unpack");
        o.key("range").u64(g.range); o.key("bandwidth").u64(g.bandwidth); o.key("power").u64(g.power);
        o.key("repack").hex(g.pack());
    } else if (sensor == "mag") {
        const auto m = B::MagConfig::unpack(U8(in, "value"));
        o.key("rate").u64(m.rate); o.key("mode").u64(m.mode); o.key("power").u64(m.power);
        o.key("repack").u64(m.pack());
    } else {
        throw Invalid("sensor " + sensor);
    }
    o.end_obj();
}

template <class A>
void opt_ints(Out& o, const char* k, const std::optional<A>& a) {
    if (a) ints(o, k, *a);
    else o.key(k).null();
}

void op_bno055_block(const Value& in, Out& o) {
    const std::uint8_t addr = U8(in, "addr");
    const auto b = B::decode_block(addr, HEX(in, "data"));
    o.obj();
    opt_ints(o, "accel", b.accel);
    opt_ints(o, "mag", b.mag);
    opt_ints(o, "gyro", b.gyro);
    opt_ints(o, "euler", b.euler);
    opt_ints(o, "quaternion", b.quaternion);
    opt_ints(o, "linear_accel", b.linear_accel);
    opt_ints(o, "gravity", b.gravity);
    o.key("temperature");
    if (b.temperature) o.i64(*b.temperature); else o.null();
    o.key("calib_stat");
    if (b.calib_stat) o.u64(*b.calib_stat); else o.null();
    o.end_obj();
}

// ── BNO086 ──

namespace S = depz::bno086;

void op_bno086_shtp_header_pack(const Value& in, Out& o) {
    S::ShtpHeader h;
    h.length = U16(in, "length");
    h.channel = U8(in, "channel");
    h.seq = U8(in, "seq");
    h.continuation = barg(in, "continuation");
    o.obj();
    o.key("bytes").hex(h.pack());
    o.end_obj();
}

void op_bno086_shtp_header_unpack(const Value& in, Out& o) {
    const auto h = S::ShtpHeader::unpack(HEX(in, "bytes"));
    o.obj();
    o.key("length").u64(h.length);
    o.key("channel").u64(h.channel);
    o.key("seq").u64(h.seq);
    o.key("continuation").boolean(h.continuation);
    o.end_obj();
}

void op_bno086_shtp_tx_seq(const Value& in, Out& o) {
    S::ShtpLayer layer;
    o.obj();
    o.key("frames");
    o.arr();
    for (const Value& p : larg(in, "packets")) {
        const int ch = I32(p, "channel");
        o.hex(layer.next_frame(ch, HEX(p, "payload")));
    }
    o.end_arr();
    o.end_obj();
}

void op_bno086_shtp_reassembly(const Value& in, Out& o) {
    const auto& fr = larg(in, "frames");
    const auto& cap = larg(in, "captures_us");
    if (fr.size() != cap.size()) throw Invalid("frames / captures_us length");
    S::ShtpLayer layer;
    o.obj();
    o.key("cargos");
    o.arr();
    for (std::size_t i = 0; i < fr.size(); ++i) {
        const depz::bytes f = hex_of(fr[i], "frames");
        const auto c = layer.feed(f, uval(cap[i], "captures_us"));
        if (c) {
            o.obj();
            o.key("channel").i64(c->channel);
            o.key("seq").i64(c->seq);
            o.key("payload").hex(c->payload);
            o.key("capture_us").u64(c->capture_us);
            o.end_obj();
        }
    }
    o.end_arr();
    o.key("discarded").u64(layer.discarded);
    o.end_obj();
}

void op_bno086_control_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "set_feature")
        return payload(o, S::sh2_build_set_feature(U8(in, "sensor_id"), U32(in, "interval_us"), U32(in, "batch_us"),
                                                   U16(in, "sensitivity"), U8(in, "flags"), U32(in, "cfg_word")));
    if (kind == "get_feature_request") return payload(o, S::sh2_build_get_feature_request(U8(in, "sensor_id")));
    if (kind == "product_id_request") return payload(o, S::sh2_build_product_id_request());
    if (kind == "command_request") {
        const std::uint8_t seq = U8(in, "seq"), cmd = U8(in, "command");
        return payload(o, S::sh2_build_command_request(seq, cmd, HEX(in, "params")));
    }
    if (kind == "frs_read_request")
        return payload(o, S::sh2_build_frs_read_request(U16(in, "frs_type"), U16(in, "offset_words"),
                                                        U16(in, "block_words")));
    if (kind == "frs_write_request")
        return payload(o, S::sh2_build_frs_write_request(U16(in, "frs_type"), U16(in, "length_words")));
    if (kind == "frs_write_data") {
        const std::uint16_t off = U16(in, "offset_words");
        std::vector<std::uint32_t> words;
        for (const Value& w : larg(in, "words")) words.push_back(static_cast<std::uint32_t>(ival(w, "words", 0, 0xFFFFFFFFLL)));
        return payload(o, S::sh2_build_frs_write_data(off, words));
    }
    throw Invalid("kind " + kind);
}

// The raw `fields` of one report: the keys of the TS / Python report record
// for its type (bools as 0/1, bytes hex).
void bno_fields(Out& o, const S::Report& r) {
    using RT = S::ReportType;
    o.obj();
    o.key("sensor_id").i64(r.sensor_id);
    o.key("timestamp_us").i64(r.timestamp_us);
    if (r.type == RT::UnknownReport) {
        o.key("data").hex(r.data);
        o.end_obj();
        return;
    }
    if (r.type == RT::FlushCompleted) {
        o.key("flushed_sensor").i64(r.flushed_sensor);
        o.end_obj();
        return;
    }
    if (r.type == RT::GyroIntegratedRV) {
        o.key("i_raw").i64(r.i_raw); o.key("j_raw").i64(r.j_raw); o.key("k_raw").i64(r.k_raw);
        o.key("real_raw").i64(r.real_raw);
        o.key("vx_raw").i64(r.vx_raw); o.key("vy_raw").i64(r.vy_raw); o.key("vz_raw").i64(r.vz_raw);
        o.end_obj();
        return;
    }
    o.key("seq").i64(r.seq);
    o.key("accuracy").i64(r.accuracy);
    o.key("delay_us").i64(r.delay_us);
    switch (r.type) {
        case RT::Acceleration: case RT::Gyroscope: case RT::Magnetometer:
            o.key("x_raw").i64(r.x_raw); o.key("y_raw").i64(r.y_raw); o.key("z_raw").i64(r.z_raw);
            break;
        case RT::UncalibratedGyroscope: case RT::UncalibratedMagnetometer:
            o.key("x_raw").i64(r.x_raw); o.key("y_raw").i64(r.y_raw); o.key("z_raw").i64(r.z_raw);
            o.key("bias_x_raw").i64(r.bias_x_raw); o.key("bias_y_raw").i64(r.bias_y_raw);
            o.key("bias_z_raw").i64(r.bias_z_raw);
            break;
        case RT::RotationVector:
            o.key("i_raw").i64(r.i_raw); o.key("j_raw").i64(r.j_raw); o.key("k_raw").i64(r.k_raw);
            o.key("real_raw").i64(r.real_raw);
            o.key("accuracy_raw");
            if (r.has_accuracy_raw) o.i64(r.accuracy_raw); else o.null();
            break;
        case RT::ScalarReport: case RT::GenericEvent:
            o.key("value_raw").i64(r.value_raw);
            break;
        case RT::TapDetector: case RT::ShakeDetector:
            o.key("flags").i64(r.flags);
            break;
        case RT::StepCounter:
            o.key("latency_us").i64(r.latency_us); o.key("steps").i64(r.steps);
            break;
        case RT::StepDetector:
            o.key("latency_us").i64(r.latency_us);
            break;
        case RT::SignificantMotion:
            o.key("motion").i64(r.motion);
            break;
        case RT::StabilityClassifier:
            o.key("classification").i64(r.classification);
            break;
        case RT::MotionRequest:
            o.key("intent").i64(r.intent);
            o.key("request").i64(r.request);
            break;
        case RT::PersonalActivityClassifier:
            o.key("page_number").i64(r.page_number);
            o.key("end_of_sequence").i64(r.end_of_sequence ? 1 : 0);
            o.key("most_likely_state").i64(r.most_likely_state);
            ints(o, "confidences", r.confidences);
            break;
        case RT::RawSensor:
            o.key("x_raw").i64(r.x_raw); o.key("y_raw").i64(r.y_raw); o.key("z_raw").i64(r.z_raw);
            o.key("sensor_timestamp_us").i64(r.sensor_timestamp_us);
            o.key("temperature_raw").i64(r.temperature_raw);
            break;
        default:
            break;
    }
    o.end_obj();
}

void op_bno086_input_cargo(const Value& in, Out& o) {
    const depz::bytes c = HEX(in, "cargo");
    const auto reps = S::parse_input_cargo(c, I64(in, "capture_timestamp_us"));
    o.obj();
    o.key("reports");
    o.arr();
    for (const auto& r : reps) {
        o.obj();
        o.key("type").str(report_type_name(r.type));
        o.key("fields");
        bno_fields(o, r);
        o.end_obj();
    }
    o.end_arr();
    o.end_obj();
}

void op_bno086_gyro_rv(const Value& in, Out& o) {
    const depz::bytes c = HEX(in, "cargo");
    const auto r = S::parse_gyro_rv_cargo(c, I64(in, "capture_timestamp_us"));
    if (!r) return o.null();  // short cargo -> no report
    o.obj();
    o.key("type").str(report_type_name(r->type));
    o.key("fields");
    bno_fields(o, *r);
    o.end_obj();
}

// ── TMF8829 ──

namespace T = depz::tmf8829;

void op_tmf8829_encode(const Value& in, Out& o) {
    const std::int64_t cmd = I64(in, "cmd");
    if (cmd == 0x32) return payload(o, T::pack_read_reg(I32(in, "addr"), I32(in, "len")));
    if (cmd == 0x33) {
        const int addr = I32(in, "addr");
        return payload(o, T::pack_write_reg(addr, HEX(in, "data")));
    }
    if (cmd == 0x35) return payload(o, T::pack_start_stream(I32(in, "int_mask"), I32(in, "flags")));
    payload(o, {});
}

void op_tmf8829_decode(const Value& in, Out& o) {
    const std::int64_t rpt = I64(in, "report");
    const depz::bytes p = HEX(in, "payload");
    o.obj();
    if (rpt == 0x91) {
        const auto& r = need(T::RegData::unpack(p), "RegData::unpack");
        o.key("timestamp_us").u64(r.timestamp_us);
        o.key("data").hex(r.data);
    } else if (rpt == 0x92) {
        const auto& r = need(T::Tmf8829Info::unpack(p), "Tmf8829Info::unpack");
        o.key("int_edges").u64(r.int_edges); o.key("frames").u64(r.frames);
        o.key("frames_aborted").u64(r.frames_aborted); o.key("frame_errors").u64(r.frame_errors);
        o.key("spi_timeouts").u64(r.spi_timeouts); o.key("int_level").u64(r.int_level);
        o.key("streaming").boolean(r.streaming); o.key("int_mask").u64(r.int_mask);
        o.key("last_int_status").u64(r.last_int_status); o.key("last_fifo_status").u64(r.last_fifo_status);
    } else if (rpt == 0x93) {
        const auto& r = need(T::FrameChunk::unpack(p), "FrameChunk::unpack");
        o.key("timestamp_us").u64(r.timestamp_us); o.key("full_size").u64(r.full_size);
        o.key("offset").u64(r.offset); o.key("frame_seq").u64(r.frame_seq); o.key("edge_us").u64(r.edge_us);
        o.key("data").hex(r.data);
    } else if (rpt == 0x94) {
        const auto& r = need(T::Tmf8829Event::unpack(p), "Tmf8829Event::unpack");
        o.key("timestamp_us").u64(r.timestamp_us); o.key("int_status").u64(r.int_status);
        o.key("code").u64(r.code); o.key("frame_id").u64(r.frame_id); o.key("fifo_status").u64(r.fifo_status);
    } else {
        throw Invalid("report");
    }
    o.end_obj();
}

void op_tmf8829_reassembly(const Value& in, Out& o) {
    T::FrameReassembler r;
    o.obj();
    o.key("frames");
    o.arr();
    for (const Value& c : larg(in, "chunks")) {
        const auto& ch = need(T::FrameChunk::unpack(hex_of(c, "chunks")), "FrameChunk::unpack");
        const auto f = r.feed(ch);
        if (f) {
            o.obj();
            o.key("frame_seq").u64(f->frame_seq);
            o.key("timestamp_us").u64(f->timestamp_us);
            o.key("edge_us").u64(f->edge_us);
            o.key("size").u64(f->data.size());
            o.key("sha256").str(sha256_hex(f->data));
            o.end_obj();
        }
    }
    o.end_arr();
    o.key("completed").u64(r.completed);
    o.key("discarded").u64(r.discarded);
    o.key("seq_gaps").u64(r.seq_gaps);
    o.end_obj();
}

template <class V>
void opt_arr(Out& o, const char* k, const std::optional<V>& v) {
    if (v) ints(o, k, *v);
    else o.key(k).null();
}

// tmf8829 `results`: what check_tmf_results() in test_vectors.cpp asserts.
void tmf_arrays(Out& o, const T::PixelResults& r) {
    const auto shape = r.shape();
    o.key("peaks").u64(r.peaks);
    o.key("rows").u64(shape.first);
    o.key("cols").u64(shape.second);
    opt_arr(o, "distance", r.distance);
    opt_arr(o, "snr", r.snr);
    opt_arr(o, "signal", r.signal);
    opt_arr(o, "noise", r.noise);
    opt_arr(o, "xtalk", r.xtalk);
}

void op_tmf8829_frame(const Value& in, Out& o) {
    const depz::bytes raw = HEX(in, "raw");
    const T::FrameInfo f = T::parse_frame(raw);
    o.obj();
    o.key("header");
    o.obj();
    o.key("fifo_status").u64(f.fifo_status); o.key("systick").u64(f.systick); o.key("id").u64(f.id);
    o.key("kind").u64(f.kind()); o.key("fp_mode").u64(f.fp_mode()); o.key("layout").u64(f.layout);
    o.key("payload").u64(f.payload); o.key("f_number").u64(f.f_number);
    ints(o, "temperature", f.temperature);
    o.key("bdv").u64(f.bdv);
    ints(o, "ref_pos", f.ref_pos);
    o.key("t0_integration").u64(f.t0_integration); o.key("t1_integration").u64(f.t1_integration);
    o.key("frame_status").u64(f.frame_status); o.key("reserved").u64(f.reserved); o.key("eof").u64(f.eof);
    o.key("size").u64(f.size);
    o.end_obj();
    o.key("problems");
    o.arr();
    for (const auto& p : f.problems()) o.str(p);
    o.end_arr();
    if (f.kind() == T::FID_RESULTS) {
        const T::PixelResults r = T::parse_results(raw, f);
        o.key("results");
        o.obj();
        tmf_arrays(o, r);
        o.end_obj();
    } else {
        if (raw.size() < 33) throw Invalid("frame shorter than head + footer");
        o.key("data_sha256").str(sha256_hex(depz::byte_span(raw.data() + 21, raw.size() - 33)));
    }
    o.end_obj();
}

void op_tmf8829_measurements(const Value& in, Out& o) {
    T::MeasurementBuilder b;
    o.obj();
    o.key("measurements");
    o.arr();
    for (const Value& fj : larg(in, "frames")) {
        const depz::bytes raw = HEX(fj, "raw");
        const auto m = b.add(raw, I64(fj, "timestamp_us"), I64(fj, "edge_us"));
        if (!m) continue;
        o.obj();
        o.key("timestamp_us").i64(m->timestamp_us);
        o.key("flags").u64(m->flags);
        o.key("fp_mode").u64(m->fp_mode);
        o.key("f_number").u64(m->f_number);
        o.key("frame_times_us");
        o.arr();
        for (const auto& t : m->times) o.i64(t.timestamp_us);
        o.end_arr();
        o.key("t1_len_us");
        o.arr();
        for (const auto& t : m->times) o.u64(t.t1_len_us);
        o.end_arr();
        o.key("ratio_q16").u64(b.clock.ratio_q16());
        o.key("n_histograms").u64(m->n_histograms);
        o.key("n_ref_frames").u64(m->n_ref_frames);
        o.key("n_problems").u64(m->n_problems);
        o.key("distance_in_mm").boolean(m->distance_in_mm());
        tmf_arrays(o, m->results);
        o.end_obj();
    }
    o.end_arr();
    o.end_obj();
}

void op_tmf8829_config_get(const Value& in, Out& o) {
    const depz::bytes page = HEX(in, "page");
    const std::uint32_t v = T::cfg_get(page, sarg(in, "field"));
    o.obj();
    o.key("value").u64(v);
    o.end_obj();
}

void op_tmf8829_config_set(const Value& in, Out& o) {
    depz::bytes page = HEX(in, "page");
    T::cfg_set(page, sarg(in, "field"), I64(in, "value"));
    o.obj();
    o.key("page").hex(page);
    o.end_obj();
}

void op_tmf8829_frame_time(const Value& in, Out& o) {
    T::SensorClock clock;
    T::CaptureTimer timer;
    o.obj();
    o.key("times");
    o.arr();
    for (const Value& r : larg(in, "rows")) {
        // the frame-id byte, split here as the SDK's frame parser does
        const std::uint8_t fid = U8(r, "id");
        const unsigned kind = static_cast<unsigned>(fid) & 0xF0u;
        if (kind == 0x30) continue;
        const std::int64_t ts = I64(r, "ts");
        const std::uint32_t tick = U32(r, "systick");
        const std::uint8_t flags = clock.add(ts, tick);
        if (kind == 0x20) {
            timer.histograms();
            continue;
        }
        const T::FrameTime t = timer.results(clock, static_cast<unsigned>(fid) & 0x0Fu, tick, U32(r, "t0"),
                                             U32(r, "t1"), ts, I64(r, "edge_us"), flags);
        o.obj();
        o.key("timestamp_us").i64(t.timestamp_us);
        o.key("t1_len_us").u64(t.t1_len_us);
        o.key("flags").u64(t.flags);
        o.key("ratio_q16").u64(clock.ratio_q16());
        o.end_obj();
    }
    o.end_arr();
    o.end_obj();
}

// ── VL53L4CD ──

namespace L4 = depz::vl53l4;

void op_vl53l4_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "read_reg") return payload(o, L4::pack_read_reg(U16(in, "addr"), U16(in, "len")));
    if (kind == "write_reg") {
        const std::uint16_t addr = U16(in, "addr");
        return payload(o, L4::pack_write_reg(addr, HEX(in, "data")));
    }
    if (kind == "xshut") return payload(o, L4::pack_xshut(U8(in, "action")));
    if (kind == "start_stream") return payload(o, L4::pack_start_stream(U16(in, "addr"), U16(in, "len"), U8(in, "flags")));
    if (kind == "set_i2c_speed") return payload(o, L4::pack_set_i2c_speed(U16(in, "khz")));
    throw Invalid("kind " + kind);
}

void op_vl53l4_decode(const Value& in, Out& o) {
    const std::int64_t rpt = I64(in, "report");
    const depz::bytes p = HEX(in, "payload");
    o.obj();
    if (rpt == 0x91) {
        const auto& r = need(L4::RegData::unpack(p), "RegData::unpack");
        o.key("cmd").u64(r.cmd);
        o.key("timestamp_us").u64(r.timestamp_us);
        o.key("data").hex(r.data);
    } else if (rpt == 0x92) {
        const auto& r = need(L4::Vl53l4Info::unpack(p), "Vl53l4Info::unpack");
        o.key("int_edges").u64(r.int_edges); o.key("slots_skipped").u64(r.slots_skipped);
        o.key("i2c_errors").u64(r.i2c_errors); o.key("last_i2c_error").u64(r.last_i2c_error);
        o.key("model_id").u64(r.model_id); o.key("fw_status").u64(r.fw_status);
        o.key("initialized").u64(r.initialized); o.key("xshut_level").u64(r.xshut_level);
        o.key("int_level").u64(r.int_level); o.key("i2c_khz").u64(r.i2c_khz);
    } else if (rpt == 0x93) {
        const auto& r = need(L4::StreamData::unpack(p), "StreamData::unpack");
        o.key("timestamp_us").u64(r.timestamp_us);
        o.key("addr").u64(r.addr);
        o.key("len").u64(r.len);
        o.key("data").hex(r.data);
    } else {
        throw Invalid("report");
    }
    o.end_obj();
}

void l4_result(Out& o, const L4::Vl53l4Result& r) {
    o.obj();
    o.key("range_status").i64(r.range_status); o.key("distance_mm").i64(r.distance_mm);
    o.key("ambient_rate_kcps").i64(r.ambient_rate_kcps); o.key("ambient_per_spad_kcps").i64(r.ambient_per_spad_kcps);
    o.key("signal_rate_kcps").i64(r.signal_rate_kcps); o.key("signal_per_spad_kcps").i64(r.signal_per_spad_kcps);
    o.key("number_of_spad").i64(r.number_of_spad); o.key("sigma_mm").i64(r.sigma_mm);
    o.key("stream_count").i64(r.stream_count);
    o.end_obj();
}

void op_vl53l4_result_block(const Value& in, Out& o) {
    l4_result(o, need(L4::parse_result_block(HEX(in, "raw")), "parse_result_block"));
}

void op_vl53l4_timing_encode(const Value& in, Out& o) {
    const auto& r = need(L4::range_timing_registers(U32(in, "timing_budget_ms"), U32(in, "inter_measurement_ms"),
                                                    U16(in, "osc_frequency"), U16(in, "clock_pll")),
                         "range_timing_registers");
    o.obj();
    o.key("range_config_a").u64(r.range_config_a);
    o.key("range_config_b").u64(r.range_config_b);
    o.key("intermeasurement_raw").u64(r.intermeasurement_raw);
    o.end_obj();
}

void op_vl53l4_timing_decode(const Value& in, Out& o) {
    const auto& r = need(L4::decode_range_timing(U32(in, "intermeasurement_raw"), U16(in, "clock_pll"),
                                                 U16(in, "osc_frequency"), U16(in, "range_config_a")),
                         "decode_range_timing");
    o.obj();
    o.key("timing_budget_ms").u64(r.timing_budget_ms);
    o.key("inter_measurement_ms").u64(r.inter_measurement_ms);
    o.end_obj();
}

void op_vl53l4_tuning_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    std::int64_t raw;
    if (kind == "offset") raw = L4::offset_raw(I32(in, "value"));
    else if (kind == "xtalk") raw = L4::xtalk_raw(I32(in, "value"));
    else if (kind == "signal_threshold") raw = L4::signal_threshold_raw(I32(in, "value"));
    else if (kind == "sigma_threshold") raw = need(L4::sigma_threshold_raw(I32(in, "value")), "sigma_threshold_raw");
    else throw Invalid("kind " + kind);
    o.obj();
    o.key("raw").i64(raw);
    o.end_obj();
}

void op_vl53l4_tuning_decode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    const std::uint16_t raw = U16(in, "raw");
    std::int64_t v;
    if (kind == "offset") v = L4::decode_offset(raw);
    else if (kind == "xtalk") v = L4::decode_xtalk(raw);
    else if (kind == "signal_threshold") v = L4::decode_signal_threshold(raw);
    else if (kind == "sigma_threshold") v = L4::decode_sigma_threshold(raw);
    else throw Invalid("kind " + kind);
    o.obj();
    o.key("value").i64(v);
    o.end_obj();
}

// ── VL53L5CX / L7 ──

namespace L7 = depz::vl53l7;

void op_vl53l7_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "read_reg") return payload(o, L7::pack_read_reg(U16(in, "addr"), U16(in, "len")));
    if (kind == "write_reg") {
        const std::uint16_t addr = U16(in, "addr");
        return payload(o, L7::pack_write_reg(addr, HEX(in, "data")));
    }
    if (kind == "pin_ctrl") return payload(o, L7::pack_pin_ctrl(U8(in, "action")));
    if (kind == "set_i2c_speed") return payload(o, L7::pack_set_i2c_speed(U16(in, "khz")));
    throw Invalid("kind " + kind);
}

void op_vl53l7_decode(const Value& in, Out& o) {
    if (I64(in, "report") != 0x92) throw Invalid("report");
    const auto& i = need(L7::Vl53l7Info::unpack(HEX(in, "payload")), "Vl53l7Info::unpack");
    o.obj();
    o.key("int_edges").u64(i.int_edges); o.key("frames_dropped").u64(i.frames_dropped);
    o.key("i2c_errors").u64(i.i2c_errors); o.key("last_i2c_error").u64(i.last_i2c_error);
    o.key("lpn_level").u64(i.lpn_level); o.key("int_level").u64(i.int_level); o.key("i2c_khz").u64(i.i2c_khz);
    o.key("frame_size").u64(i.frame_size); o.key("streaming").boolean(i.streaming);
    o.end_obj();
}

void op_vl53l7_model(const Value& in, Out& o) {
    const auto um = snarg(in, "usb_model");
    o.obj();
    o.key("class").str(L7::to_string(L7::resolve_model(um, sarg(in, "device_name"))));
    o.end_obj();
}

// ── VL53L 1D family ──

namespace LX = depz::vl53lx;

void op_vl53lx_encode(const Value& in, Out& o) {
    const std::string& kind = sarg(in, "kind");
    if (kind == "set_addr_width") return payload(o, need(LX::pack_set_addr_width(U8(in, "width")), "pack_set_addr_width"));
    if (kind == "read_reg") return payload(o, LX::pack_read_reg(U16(in, "addr"), U16(in, "len")));
    if (kind == "write_reg") {
        const std::uint16_t addr = U16(in, "addr");
        return payload(o, LX::pack_write_reg(addr, HEX(in, "data")));
    }
    if (kind == "xshut") return payload(o, LX::pack_xshut(U8(in, "action")));
    if (kind == "set_i2c_speed") return payload(o, LX::pack_set_i2c_speed(U16(in, "khz")));
    if (kind == "start_stream") {
        const std::uint16_t addr = U16(in, "addr"), len = U16(in, "len");
        const std::uint8_t flags = U8(in, "flags");
        std::vector<LX::ClearStep> steps;
        for (const Value& st : larg(in, "clear")) {
            if (st.type != Type::Array || st.arr.size() != 2) throw Invalid("clear step: not an [addr, value] pair");
            steps.push_back({static_cast<std::uint16_t>(ival(st.arr[0], "clear.addr", 0, 0xFFFF)),
                             static_cast<std::uint8_t>(ival(st.arr[1], "clear.value", 0, 0xFF))});
        }
        return payload(o, need(LX::pack_start_stream(addr, len, steps, flags), "pack_start_stream"));
    }
    throw Invalid("kind " + kind);
}

void op_vl53lx_decode(const Value& in, Out& o) {
    if (I64(in, "report") != 0x92) throw Invalid("report");
    const auto& i = need(LX::Vl53lxInfo::unpack(HEX(in, "payload")), "Vl53lxInfo::unpack");
    o.obj();
    o.key("int_edges").u64(i.int_edges); o.key("slots_skipped").u64(i.slots_skipped);
    o.key("i2c_errors").u64(i.i2c_errors); o.key("last_i2c_error").u64(i.last_i2c_error);
    o.key("xshut_level").u64(i.xshut_level); o.key("int_level").u64(i.int_level); o.key("i2c_khz").u64(i.i2c_khz);
    o.key("addr_width").u64(i.addr_width); o.key("n_clear").u64(i.n_clear);
    o.key("frames_dropped").u64(i.frames_dropped);
    o.end_obj();
}

void op_vl53lx_model(const Value& in, Out& o) {
    const auto um = snarg(in, "usb_model");
    const std::string& dev = sarg(in, "device_name");
    const auto prod = LX::product_from_board_name(dev);
    o.obj();
    o.key("class").str(LX::to_string(LX::resolve_class(um, dev)));
    o.key("product");
    if (prod) o.str(*prod); else o.null();
    o.end_obj();
}

void op_vl53lx_die_block(const Value& in, Out& o) {
    const std::string& var = sarg(in, "variant");
    LX::DieVariant v;
    if (var == "l4") v = LX::DieVariant::L4;
    else if (var == "l1") v = LX::DieVariant::L1;
    else throw Invalid("variant " + var);
    l4_result(o, need(LX::decode_die_block(HEX(in, "raw"), v), "decode_die_block"));
}

void op_vl53lx_l0x_raw(const Value& in, Out& o) {
    const auto& r = need(LX::decode_l0x_raw(HEX(in, "raw")), "decode_l0x_raw");
    o.obj();
    o.key("distance_raw").u64(r.distance_raw);
    o.key("device_range_status").u64(r.device_range_status);
    o.key("signal_rate_mcps_1616").u64(r.signal_rate_mcps_1616);
    o.key("ambient_rate_mcps_1616").u64(r.ambient_rate_mcps_1616);
    o.key("effective_spad_count_88").u64(r.effective_spad_count_88);
    o.end_obj();
}

void op_vl53lx_histogram_raw(const Value& in, Out& o) {
    const auto& r = need(LX::decode_histogram_raw(HEX(in, "raw")), "decode_histogram_raw");
    o.obj();
    o.key("interrupt_status").u64(r.interrupt_status); o.key("range_status").u64(r.range_status);
    o.key("report_status").u64(r.report_status); o.key("stream_count").u64(r.stream_count);
    o.key("dss_actual_effective_spads").u64(r.dss_actual_effective_spads);
    o.key("reference_phase").u64(r.reference_phase); o.key("vcsel_start").u64(r.vcsel_start);
    ints(o, "bins", r.bins);
    o.end_obj();
}

// ── VL53L8 advanced + CNH ──

namespace L8 = depz::vl53l8;

void op_vl53l8_motion_config(const Value& in, Out& o) {
    const auto cfg = L8::motion_config_init(I32(in, "resolution"));
    o.obj();
    o.key("pack").hex(cfg.pack());
    o.end_obj();
}

void op_vl53l8_detection_thresholds(const Value& in, Out& o) {
    std::vector<L8::DetectionThreshold> th;
    for (const Value& e : larg(in, "thresholds")) {
        L8::DetectionThreshold d;
        d.low_thresh = I32(e, "low_thresh");
        d.high_thresh = I32(e, "high_thresh");
        d.measurement = U8(e, "measurement");
        d.type = U8(e, "type");
        d.zone_num = U8(e, "zone_num");
        d.operation = U8(e, "operation");
        th.push_back(d);
    }
    o.obj();
    o.key("start_block").hex(L8::pack_detection_thresholds(th));
    o.key("valid_status").hex(L8::detection_thresholds_valid_status());
    o.end_obj();
}

void op_vl53l8_xtalk_margin(const Value& in, Out& o) {
    const double kcps = darg(in, "kcps");
    o.obj();
    o.key("raw").u64(L8::xtalk_margin_raw(kcps));
    o.end_obj();
}

#ifdef DEPZ_DIFFTEST_HAS_IO
void op_vl53l8_cnh_decode(const Value& in, Out& o) {
    const Value& c = field(in, "config");
    const auto& am = larg(c, "agg_map");
    if (am.size() != 6) throw Invalid("agg_map: 6 integers");
    int m[6];
    for (int k = 0; k < 6; ++k) m[k] = static_cast<int>(ival(am[k], "agg_map", INT32_MIN, INT32_MAX));
    depz::CnhSetup setup;
    setup.init_config(I32(c, "start_bin"), I32(c, "num_bins"), I32(c, "sub_sample"));
    setup.create_agg_map(I32(c, "resolution"), m[0], m[1], m[2], m[3], m[4], m[5]);
    const auto frame = L8::decode_cnh(setup.nb_of_aggregates, setup.feature_length, HEX(in, "cnh_raw"));
    o.obj();
    o.key("aggregates");
    o.arr();
    for (const auto& a : frame.aggregates) {
        o.obj();
        ints(o, "hist_raw", a.hist_raw);
        ints(o, "hist_scaler", a.hist_scaler);
        o.end_obj();
    }
    o.end_arr();
    o.end_obj();
}
#endif

const std::map<std::string, OpFn>& ops() {
    static const std::map<std::string, OpFn> m = {
        {"crc", op_crc}, {"framing_encode", op_framing_encode}, {"framing_decode", op_framing_decode},
        {"identity", op_identity}, {"usb_identity", op_usb_identity},
        {"usb_serial_ordering", op_usb_serial_ordering}, {"probe_resolve", op_probe_resolve},
        {"common_encode", op_common_encode}, {"common_decode", op_common_decode},
        {"sync_time_math", op_sync_time_math}, {"sr04_encode", op_sr04_encode}, {"sr04_decode", op_sr04_decode},
        {"fwdepz_parse", op_fwdepz_parse},
        {"bno055_encode", op_bno055_encode}, {"bno055_decode", op_bno055_decode},
        {"bno055_units", op_bno055_units}, {"bno055_calib_stat", op_bno055_calib_stat},
        {"bno055_calibration_profile", op_bno055_calibration_profile},
        {"bno055_axis_remap", op_bno055_axis_remap}, {"bno055_axis_remap_pack", op_bno055_axis_remap_pack},
        {"bno055_sensor_config", op_bno055_sensor_config}, {"bno055_block", op_bno055_block},
        {"bno086_shtp_header_pack", op_bno086_shtp_header_pack},
        {"bno086_shtp_header_unpack", op_bno086_shtp_header_unpack},
        {"bno086_shtp_tx_seq", op_bno086_shtp_tx_seq}, {"bno086_shtp_reassembly", op_bno086_shtp_reassembly},
        {"bno086_control_encode", op_bno086_control_encode}, {"bno086_input_cargo", op_bno086_input_cargo},
        {"bno086_gyro_rv", op_bno086_gyro_rv},
        {"tmf8829_encode", op_tmf8829_encode}, {"tmf8829_decode", op_tmf8829_decode},
        {"tmf8829_reassembly", op_tmf8829_reassembly}, {"tmf8829_frame", op_tmf8829_frame},
        {"tmf8829_measurements", op_tmf8829_measurements}, {"tmf8829_config_get", op_tmf8829_config_get},
        {"tmf8829_config_set", op_tmf8829_config_set}, {"tmf8829_frame_time", op_tmf8829_frame_time},
        {"vl53l4_encode", op_vl53l4_encode}, {"vl53l4_decode", op_vl53l4_decode},
        {"vl53l4_result_block", op_vl53l4_result_block}, {"vl53l4_timing_encode", op_vl53l4_timing_encode},
        {"vl53l4_timing_decode", op_vl53l4_timing_decode}, {"vl53l4_tuning_encode", op_vl53l4_tuning_encode},
        {"vl53l4_tuning_decode", op_vl53l4_tuning_decode},
        {"vl53l7_encode", op_vl53l7_encode}, {"vl53l7_decode", op_vl53l7_decode},
        {"vl53l7_model", op_vl53l7_model},
        {"vl53lx_encode", op_vl53lx_encode}, {"vl53lx_decode", op_vl53lx_decode}, {"vl53lx_model", op_vl53lx_model},
        {"vl53lx_die_block", op_vl53lx_die_block}, {"vl53lx_l0x_raw", op_vl53lx_l0x_raw},
        {"vl53lx_histogram_raw", op_vl53lx_histogram_raw},
        {"vl53l8_motion_config", op_vl53l8_motion_config},
        {"vl53l8_detection_thresholds", op_vl53l8_detection_thresholds},
        {"vl53l8_xtalk_margin", op_vl53l8_xtalk_margin},
#ifdef DEPZ_DIFFTEST_HAS_IO
        {"vl53l8_cnh_decode", op_vl53l8_cnh_decode},
#endif
    };
    return m;
}

// Declared gaps: op -> reason (see the header comment).
const std::map<std::string, std::string>& unsupported() {
    static const std::map<std::string, std::string> m = {
        {"bno086_record", "no contract 09 dataset recorder in the C++ SDK (depz::dataset reads only)"},
#ifndef DEPZ_DIFFTEST_HAS_IO
        {"vl53l8_cnh_decode", "built without the live-hardware layer (depz::CnhSetup is in depz/device.hpp)"},
#endif
    };
    return m;
}

std::string id_text(const Value& req) {
    if (req.has("id") && req.at("id").type == Type::Int && req.at("id").range == 0)
        return std::to_string(req.at("id").i);
    return "null";
}

std::string error_line(const std::string& id, const char* kind, const std::string& detail) {
    Out o;
    o.obj();
    o.key("id").raw(id);
    o.key("error").str(kind);
    o.key("detail").str(detail.substr(0, 300));
    o.end_obj();
    return o.s;
}

std::string handle(const Value& req) {
    const std::string id = id_text(req);
    const std::string op = req.has("op") && req.at("op").type == Type::String ? req.at("op").s : "";
    const auto u = unsupported().find(op);
    if (u != unsupported().end()) return error_line(id, "unsupported", u->second);
    const auto f = ops().find(op);
    if (f == ops().end()) return error_line(id, "unsupported", "unknown op");
    Out o;
    try {
        if (!req.has("in") || req.at("in").type != Type::Object) throw Invalid("`in` is not an object");
        f->second(req.at("in"), o);
    } catch (const std::exception& e) {
        return error_line(id, "invalid", e.what());
    } catch (...) {
        return error_line(id, "invalid", "non-std exception");
    }
    return "{\"id\":" + id + ",\"out\":" + o.s + "}";
}

}  // namespace

int main() {
    std::ios::sync_with_stdio(false);
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find_first_not_of(" \t") == std::string::npos) continue;
        const Value req = testjson::parse(line);
        std::cout << handle(req) << '\n' << std::flush;
    }
    return 0;
}
