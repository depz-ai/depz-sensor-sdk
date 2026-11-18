#include "depz/vl53l4.hpp"

#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace vl53l4 {

using detail::packed;
using detail::to_bytes;
using detail::u8p;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint16_t addr, std::uint16_t len) {
    std::uint8_t out[4];
    return packed(out, depz_vl53l4_pack_read_reg(addr, len, out),
                  "VL53_READ_REG: len must be 1..253 and addr+len <= 0x10000");
}

bytes pack_write_reg(std::uint16_t addr, byte_span data) {
    bytes out(2 + data.size());
    const std::size_t n = depz_vl53l4_pack_write_reg(addr, u8p(data), data.size(),
                                                     reinterpret_cast<std::uint8_t*>(out.data()));
    if (n == 0) throw std::invalid_argument("VL53_WRITE_REG: data must be 1..253 bytes and addr+N <= 0x10000");
    out.resize(n);
    return out;
}

bytes pack_xshut(std::uint8_t action) {
    std::uint8_t out[1];
    return to_bytes(out, depz_vl53l4_pack_xshut(action, out));
}

bytes pack_start_stream(std::uint16_t addr, std::uint16_t len, std::uint8_t flags) {
    std::uint8_t out[5];
    return to_bytes(out, depz_vl53l4_pack_start_stream(addr, len, flags, out));
}

bytes pack_set_i2c_speed(std::uint16_t khz) {
    std::uint8_t out[2];
    return to_bytes(out, depz_vl53l4_pack_set_i2c_speed(khz, out));
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<RegData> RegData::unpack(byte_span payload) {
    depz_vl53l4_reg_data c{};
    if (depz_vl53l4_unpack_reg_data(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return RegData{c.cmd, c.timestamp_us, to_bytes(c.data, c.data_len)};
}

std::optional<Vl53l4Info> Vl53l4Info::unpack(byte_span payload) {
    Vl53l4Info i{};
    if (depz_vl53l4_unpack_info(u8p(payload), payload.size(), &i) != 0) return std::nullopt;
    return i;
}

std::optional<StreamData> StreamData::unpack(byte_span payload) {
    depz_vl53l4_stream c{};
    if (depz_vl53l4_unpack_stream(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return StreamData{c.timestamp_us, c.addr, c.len, to_bytes(c.data, c.len)};
}

// ── result-block decode ─────────────────────────────────────────────────────

std::optional<Vl53l4Result> parse_result_block(byte_span raw) {
    Vl53l4Result r{};
    if (depz_vl53l4_parse_result_block(u8p(raw), raw.size(), &r) != 0) return std::nullopt;
    return r;
}

// ── timing math ─────────────────────────────────────────────────────────────

std::optional<RangeTimingRegs> range_timing_registers(std::uint32_t budget_ms, std::uint32_t inter_ms,
                                                      std::uint16_t osc_frequency, std::uint16_t clock_pll) {
    RangeTimingRegs r;
    if (depz_vl53l4_range_timing_registers(budget_ms, inter_ms, osc_frequency, clock_pll, &r.range_config_a,
                                           &r.range_config_b, &r.intermeasurement_raw) != 0)
        return std::nullopt;
    return r;
}

std::optional<RangeTiming> decode_range_timing(std::uint32_t intermeasurement_raw, std::uint16_t clock_pll,
                                               std::uint16_t osc_frequency, std::uint16_t range_config_a) {
    RangeTiming t;
    if (depz_vl53l4_decode_range_timing(intermeasurement_raw, clock_pll, osc_frequency, range_config_a,
                                        &t.timing_budget_ms, &t.inter_measurement_ms) != 0)
        return std::nullopt;
    return t;
}

// ── tuning-register codecs ──────────────────────────────────────────────────

std::uint16_t offset_raw(int offset_mm) {
    std::uint16_t raw = 0;
    if (depz_vl53l4_offset_raw_checked(offset_mm, &raw) != 0) throw std::out_of_range("offset_mm outside -1024..1023");
    return raw;
}

int decode_offset(std::uint16_t raw_word) { return depz_vl53l4_decode_offset(raw_word); }

std::uint16_t xtalk_raw(int xtalk_kcps) {
    std::uint16_t raw = 0;
    if (xtalk_kcps < 0 || xtalk_kcps > 0xFFFF ||
        depz_vl53l4_xtalk_raw_checked(static_cast<std::uint16_t>(xtalk_kcps), &raw) != 0)
        throw std::out_of_range("xtalk_kcps outside 0..127");
    return raw;
}

int decode_xtalk(std::uint16_t raw_word) { return depz_vl53l4_decode_xtalk(raw_word); }

std::uint16_t signal_threshold_raw(int signal_kcps) {
    if (signal_kcps < 0 || signal_kcps > 0xFFFF) throw std::out_of_range("signal_kcps outside 0..65535");
    return depz_vl53l4_signal_threshold_raw(static_cast<std::uint16_t>(signal_kcps));
}

int decode_signal_threshold(std::uint16_t raw_word) { return depz_vl53l4_decode_signal_threshold(raw_word); }

std::optional<std::uint16_t> sigma_threshold_raw(int sigma_mm) {
    std::uint16_t raw = 0;
    if (sigma_mm < 0 || sigma_mm > 0xFFFF ||
        depz_vl53l4_sigma_threshold_raw(static_cast<std::uint16_t>(sigma_mm), &raw) != 0)
        return std::nullopt;
    return raw;
}

int decode_sigma_threshold(std::uint16_t raw_word) { return depz_vl53l4_decode_sigma_threshold(raw_word); }

// ── init configuration block ────────────────────────────────────────────────

span<const std::uint8_t> default_configuration() noexcept {
    return span<const std::uint8_t>(DEPZ_VL53L4_DEFAULT_CONFIGURATION, 91);
}

bytes config_block() {
    std::uint8_t out[91];
    return to_bytes(out, depz_vl53l4_config_block(out));
}

}  // namespace vl53l4
}  // namespace depz
