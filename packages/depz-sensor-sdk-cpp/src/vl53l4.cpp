#include "depz/vl53l4.hpp"

#include <algorithm>
#include <cmath>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace vl53l4 {

using detail::rd_u16;
using detail::rd_u32;
using detail::rd_u64;
using detail::u8;
using detail::wr_le;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint16_t addr, std::uint16_t len) {
    bytes out;
    wr_le(out, addr, 2);
    wr_le(out, len, 2);
    return out;
}

bytes pack_write_reg(std::uint16_t addr, byte_span data) {
    bytes out;
    out.reserve(2 + data.size());
    wr_le(out, addr, 2);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

bytes pack_xshut(std::uint8_t action) {
    bytes out;
    wr_le(out, action, 1);
    return out;
}

bytes pack_start_stream(std::uint16_t addr, std::uint16_t len, std::uint8_t flags) {
    bytes out;
    wr_le(out, addr, 2);
    wr_le(out, len, 2);
    wr_le(out, flags, 1);
    return out;
}

bytes pack_set_i2c_speed(std::uint16_t khz) {
    bytes out;
    wr_le(out, khz, 2);
    return out;
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<RegData> RegData::unpack(byte_span payload) {
    if (payload.size() < 9) return std::nullopt;
    RegData r;
    r.cmd = u8(payload[0]);
    r.timestamp_us = rd_u64(payload, 1);
    r.data.assign(payload.begin() + 9, payload.end());
    return r;
}

std::optional<Vl53l4Info> Vl53l4Info::unpack(byte_span payload) {
    if (payload.size() < 21) return std::nullopt;
    Vl53l4Info r;
    r.int_edges = rd_u32(payload, 0);
    r.slots_skipped = rd_u32(payload, 4);
    r.i2c_errors = rd_u32(payload, 8);
    r.last_i2c_error = u8(payload[12]);
    r.model_id = rd_u16(payload, 13);
    r.fw_status = u8(payload[15]);
    r.initialized = u8(payload[16]);
    r.xshut_level = u8(payload[17]);
    r.int_level = u8(payload[18]);
    r.i2c_khz = rd_u16(payload, 19);
    return r;
}

std::optional<StreamData> StreamData::unpack(byte_span payload) {
    if (payload.size() < 12) return std::nullopt;
    StreamData r;
    r.timestamp_us = rd_u64(payload, 0);
    r.addr = rd_u16(payload, 8);
    r.len = rd_u16(payload, 10);
    const std::size_t n = std::min<std::size_t>(r.len, payload.size() - 12);
    r.data.assign(payload.begin() + 12, payload.begin() + 12 + n);
    return r;
}

// ── result-block decode ─────────────────────────────────────────────────────

std::optional<Vl53l4Result> parse_result_block(byte_span raw) {
    if (raw.size() < 15) return std::nullopt;

    // GetResult() raw status -> ULD status (status_rtn[24] in VL53L4CD_api.c).
    static constexpr int kStatusRtn[24] = {255, 255, 255, 5,   2,   4,   1,   7,
                                           3,   0,   255, 255, 9,   13,  255, 255,
                                           255, 255, 10,  6,   255, 255, 11,  12};

    int status = u8(raw[0]) & 0x1F;
    if (status < 24) status = kStatusRtn[status];

    auto be16 = [&raw](std::size_t off) {
        return (u8(raw[off]) << 8) | u8(raw[off + 1]);
    };
    const int raw_spads = be16(3);           // 0x008C
    const int signal_kcps = be16(5) * 8;     // 0x008E
    const int ambient_kcps = be16(7) * 8;    // 0x0090

    Vl53l4Result r;
    r.range_status = status;
    r.stream_count = u8(raw[2]);
    r.number_of_spad = raw_spads / 256;
    r.signal_rate_kcps = signal_kcps;
    r.ambient_rate_kcps = ambient_kcps;
    r.sigma_mm = be16(9) / 4;   // 0x0092
    r.distance_mm = be16(13);   // 0x0096
    r.signal_per_spad_kcps = raw_spads ? signal_kcps * 256 / raw_spads : 0;
    r.ambient_per_spad_kcps = raw_spads ? ambient_kcps * 256 / raw_spads : 0;
    return r;
}

// ── timing math ─────────────────────────────────────────────────────────────

// Macro period from the 0x0006 oscillator word — the ULD's 32-bit expression
// (unsigned wrap-around included).
static std::uint32_t macro_period_us_from_osc(std::uint16_t osc_frequency) {
    return (2304u * (0x40000000u / osc_frequency)) >> 6;
}

std::optional<RangeTimingRegs> range_timing_registers(std::uint32_t budget_ms,
                                                      std::uint32_t inter_ms,
                                                      std::uint16_t osc_frequency,
                                                      std::uint16_t clock_pll) {
    if (osc_frequency == 0) return std::nullopt;
    if (budget_ms < 10 || budget_ms > 200) return std::nullopt;

    std::uint32_t timing_budget_us = budget_ms * 1000u;
    const std::uint32_t macro_period_us = macro_period_us_from_osc(osc_frequency);

    std::uint32_t intermeasurement_raw = 0;
    if (inter_ms == 0) {  // continuous
        timing_budget_us -= 2500u;
    } else if (inter_ms > budget_ms) {  // autonomous low power
        const double factor =
            1.055 * static_cast<double>(inter_ms) * static_cast<double>(clock_pll & 0x3FF);
        intermeasurement_raw = static_cast<std::uint32_t>(factor);
        timing_budget_us = (timing_budget_us - 4300u) / 2u;
    } else {
        return std::nullopt;
    }

    timing_budget_us <<= 12;  // 32-bit wrap, as in the ULD

    std::uint16_t words[2] = {0, 0};
    const std::uint32_t mults[2] = {16u, 12u};  // RANGE_CONFIG_A, RANGE_CONFIG_B
    for (int k = 0; k < 2; ++k) {
        const std::uint32_t tmp = (macro_period_us * mults[k]) >> 6;
        if (tmp == 0) return std::nullopt;  // degenerate osc word (division guard)
        std::uint64_t ls_byte =
            (static_cast<std::uint64_t>(timing_budget_us) + (tmp >> 1)) / tmp - 1u;
        std::uint32_t ms_byte = 0;
        while (ls_byte & 0xFFFFFF00ull) {
            ls_byte >>= 1;
            ++ms_byte;
        }
        words[k] = static_cast<std::uint16_t>((ms_byte << 8) + (ls_byte & 0xFFu));
    }
    return RangeTimingRegs{words[0], words[1], intermeasurement_raw};
}

std::optional<RangeTiming> decode_range_timing(std::uint32_t intermeasurement_raw,
                                               std::uint16_t clock_pll,
                                               std::uint16_t osc_frequency,
                                               std::uint16_t range_config_a) {
    if (osc_frequency == 0) return std::nullopt;

    const std::uint32_t pll =
        static_cast<std::uint32_t>(1.065 * static_cast<double>(clock_pll & 0x3FF)) & 0xFFFFu;
    const std::uint32_t inter_measurement_ms =
        pll ? (intermeasurement_raw / pll) & 0xFFFFu : 0u;

    std::uint32_t macro_period_us = macro_period_us_from_osc(osc_frequency);
    const std::uint32_t ls_byte = static_cast<std::uint32_t>(range_config_a & 0x00FFu) << 4;
    std::uint32_t ms_byte = static_cast<std::uint32_t>(range_config_a & 0xFF00u) >> 8;
    ms_byte = 0x04u - (ms_byte - 1u) - 1u;  // 32-bit wrap, as in the ULD
    macro_period_us *= 16u;                 // 32-bit wrap

    const std::uint32_t mp6 = macro_period_us >> 6;
    std::uint32_t budget = ((ls_byte + 1u) * mp6 - (mp6 >> 1)) >> 12;  // 32-bit wrap
    if (ms_byte < 12u) budget >>= ms_byte;
    budget = (intermeasurement_raw == 0) ? budget + 2500u : budget * 2u + 4300u;
    return RangeTiming{budget / 1000u, inter_measurement_ms};
}

// ── tuning-register codecs ──────────────────────────────────────────────────

std::uint16_t offset_raw(int offset_mm) {
    return static_cast<std::uint16_t>(offset_mm * 4);
}

int decode_offset(std::uint16_t raw_word) {
    const int temp = static_cast<std::uint16_t>(raw_word << 3) >> 5;
    return temp > 1024 ? temp - 2048 : temp;
}

std::uint16_t xtalk_raw(int xtalk_kcps) {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(xtalk_kcps) << 9);
}

int decode_xtalk(std::uint16_t raw_word) {
    return static_cast<int>(std::lround(raw_word / 512.0));
}

std::uint16_t signal_threshold_raw(int signal_kcps) {
    return static_cast<std::uint16_t>(signal_kcps >> 3);
}

int decode_signal_threshold(std::uint16_t raw_word) {
    return static_cast<std::uint16_t>(raw_word << 3);
}

std::optional<std::uint16_t> sigma_threshold_raw(int sigma_mm) {
    if (sigma_mm > (0xFFFF >> 2)) return std::nullopt;
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(sigma_mm) << 2);
}

int decode_sigma_threshold(std::uint16_t raw_word) {
    return raw_word >> 2;
}

// ── init configuration block ────────────────────────────────────────────────

const std::array<std::uint8_t, 91>& default_configuration() {
    // VL53L4CD_DEFAULT_CONFIGURATION[] — registers 0x2D..0x87 (ULD 2.2.3).
    static const std::array<std::uint8_t, 91> cfg = {{
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,  // 0x2D..0x34
        0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,  // 0x35..0x3C
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,  // 0x3D..0x44
        0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,  // 0x45..0x4C
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,  // 0x4D..0x54
        0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,  // 0x55..0x5C
        0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,  // 0x5D..0x64
        0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,  // 0x65..0x6C
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x6D..0x74
        0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,  // 0x75..0x7C
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,  // 0x7D..0x84
        0x01, 0x00, 0x00,                                // 0x85..0x87
    }};
    return cfg;
}

bytes config_block() {
    const auto& cfg = default_configuration();
    bytes out;
    out.reserve(cfg.size());
    out.push_back(std::byte{CONFIG_FMP_BYTE});
    for (std::size_t i = 1; i < cfg.size(); ++i) out.push_back(std::byte{cfg[i]});
    return out;
}

}  // namespace vl53l4
}  // namespace depz
