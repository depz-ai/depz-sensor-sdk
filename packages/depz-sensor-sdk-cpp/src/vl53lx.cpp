#include "depz/vl53lx.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace vl53lx {

using detail::rd_u16;
using detail::rd_u32;
using detail::u8;
using detail::wr_le;

// ── command encoders ────────────────────────────────────────────────────────

std::optional<bytes> pack_start_stream(std::uint16_t addr, std::uint16_t len,
                                       const std::vector<ClearStep>& clear,
                                       std::uint8_t flags) {
    if (clear.size() > CLEAR_STEPS_MAX) return std::nullopt;
    bytes out;
    out.reserve(6 + 3 * clear.size());
    wr_le(out, addr, 2);
    wr_le(out, len, 2);
    wr_le(out, flags, 1);
    wr_le(out, clear.size(), 1);
    for (const ClearStep& s : clear) {
        wr_le(out, s.addr, 2);
        wr_le(out, s.value, 1);
    }
    return out;
}

std::optional<bytes> pack_set_addr_width(std::uint8_t width) {
    if (width != 1 && width != 2) return std::nullopt;
    bytes out;
    wr_le(out, width, 1);
    return out;
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<Vl53lxInfo> Vl53lxInfo::unpack(byte_span payload) {
    if (payload.size() < INFO_SIZE) return std::nullopt;
    Vl53lxInfo r;
    r.int_edges = rd_u32(payload, 0);
    r.slots_skipped = rd_u32(payload, 4);
    r.i2c_errors = rd_u32(payload, 8);
    r.last_i2c_error = u8(payload[12]);
    r.xshut_level = u8(payload[13]);
    r.int_level = u8(payload[14]);
    r.i2c_khz = rd_u16(payload, 15);
    r.addr_width = u8(payload[17]);
    r.n_clear = u8(payload[18]);
    r.frames_dropped = rd_u32(payload, 19);
    return r;
}

// ── product table ───────────────────────────────────────────────────────────

std::string to_string(DriverKind k) {
    switch (k) {
        case DriverKind::Uld: return "uld";
        case DriverKind::Ulp: return "ulp";
        case DriverKind::Histogram: return "histogram";
    }
    return "uld";
}

const std::vector<Product>& products() {
    // Die interrupt release: SYSTEM__INTERRUPT_CLEAR (0x0086) <- 1. The
    // VL53L0X takes two writes: 0x0B <- 1, then 0x0B <- 0.
    static const std::vector<ClearStep> kDieClear = {{0x0086, 0x01}};
    static const std::vector<ClearStep> kL0xClear = {{0x0B, 0x01}, {0x0B, 0x00}};
    using K = DriverKind;
    static const std::vector<Product> table = {
        {"VL53L0X", 0xED41, 0x00EE, 2000, {K::Uld}, K::Uld, 1, kL0xClear, 400},
        {"VL53L1CX", 0xED43, 0xEACC, 4000, {K::Uld, K::Histogram}, K::Uld, 2, kDieClear, 1000},
        {"VL53L1CB", 0xED42, 0xEACC, 8000, {K::Uld, K::Histogram}, K::Uld, 2, kDieClear, 1000},
        {"VL53L3CX", 0xED44, 0xEAAA, 3000, {K::Ulp, K::Histogram}, K::Ulp, 2, kDieClear, 1000},
        {"VL53L4CD", 0xED45, 0xEBAA, 1200, {K::Uld, K::Histogram}, K::Uld, 2, kDieClear, 1000},
        {"VL53L4CX", 0xED46, 0xEBAA, 6000, {K::Histogram}, K::Histogram, 2, kDieClear, 1000},
    };
    return table;
}

const Product* find_product(const std::string& name) {
    for (const Product& p : products()) {
        if (p.name == name) return &p;
    }
    return nullptr;
}

namespace {
std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}
}  // namespace

std::optional<std::string> product_from_board_name(const std::string& name) {
    if (name.empty()) return std::nullopt;
    static const std::regex part_re("VL53L(\\d[A-Z0-9]*)");
    const std::string up = upper(name);
    std::smatch m;
    if (!std::regex_search(up, m, part_re)) return std::nullopt;
    std::string product = "VL53L" + m[1].str();
    if (!find_product(product)) return std::nullopt;
    return product;
}

// ── class resolution ────────────────────────────────────────────────────────

std::string to_string(SensorClass c) {
    switch (c) {
        case SensorClass::Vl53lx: return "Vl53lx";
        case SensorClass::Vl53l0x: return "Vl53l0x";
        case SensorClass::Vl53l1cx: return "Vl53l1cx";
        case SensorClass::Vl53l1cb: return "Vl53l1cb";
        case SensorClass::Vl53l3cx: return "Vl53l3cx";
        case SensorClass::Vl53l4cx: return "Vl53l4cx";
    }
    return "Vl53lx";
}

SensorClass resolve_class(const std::optional<std::string>& usb_model,
                          const std::string& device_name) {
    std::optional<std::string> product;
    if (usb_model && find_product(upper(*usb_model))) product = upper(*usb_model);
    if (!product) product = product_from_board_name(device_name);
    if (!product) return SensorClass::Vl53lx;
    // VL53L4CD has no dedicated class on this firmware: the generic one.
    if (*product == "VL53L0X") return SensorClass::Vl53l0x;
    if (*product == "VL53L1CX") return SensorClass::Vl53l1cx;
    if (*product == "VL53L1CB") return SensorClass::Vl53l1cb;
    if (*product == "VL53L3CX") return SensorClass::Vl53l3cx;
    if (*product == "VL53L4CX") return SensorClass::Vl53l4cx;
    return SensorClass::Vl53lx;
}

// ── stateless block decoders ────────────────────────────────────────────────

std::optional<DieResult> decode_die_block(byte_span raw, DieVariant variant) {
    if (raw.size() < DIE_BLOCK_LEN) return std::nullopt;
    // The l4 variant is the contract-10 decode verbatim.
    auto r = vl53l4::parse_result_block(raw);
    if (!r || variant == DieVariant::L4) return r;

    // l1: VL53L1X ULD — crosstalk-corrected peak signal at 0x0098 (byte 15)
    // and per-SPAD scale K = 25 instead of 256.
    constexpr int K = 25;
    auto be16 = [&raw](std::size_t off) { return (u8(raw[off]) << 8) | u8(raw[off + 1]); };
    const int raw_spads = be16(3);
    const int signal = be16(15) * 8;
    r->signal_rate_kcps = signal;
    r->signal_per_spad_kcps = raw_spads ? signal * K / raw_spads : 0;
    r->ambient_per_spad_kcps = raw_spads ? r->ambient_rate_kcps * K / raw_spads : 0;
    return r;
}

std::optional<L0xRaw> decode_l0x_raw(byte_span raw) {
    if (raw.size() < L0X_BLOCK_LEN) return std::nullopt;
    auto be16 = [&raw](std::size_t off) {
        return static_cast<std::uint16_t>((u8(raw[off]) << 8) | u8(raw[off + 1]));
    };
    L0xRaw r;
    r.distance_raw = be16(10);
    r.device_range_status = u8(raw[0]);
    r.signal_rate_mcps_1616 = static_cast<std::uint32_t>(be16(6)) << 9;
    r.ambient_rate_mcps_1616 = static_cast<std::uint32_t>(be16(8)) << 9;
    r.effective_spad_count_88 = be16(2);
    return r;
}

std::optional<HistogramRaw> decode_histogram_raw(byte_span raw) {
    if (raw.size() < HISTOGRAM_BLOCK_LEN) return std::nullopt;
    // Register offsets inside the block (block starts at 0x0088).
    constexpr std::size_t kBin0 = 0x008E - HISTOGRAM_BLOCK_ADDR;       // RESULT__HISTOGRAM_BIN_0_2
    constexpr std::size_t kBin23Lo = 0x00D5 - HISTOGRAM_BLOCK_ADDR;    // RESULT__HISTOGRAM_BIN_23_0
    constexpr std::size_t kRefPhase = 0x00D6 - HISTOGRAM_BLOCK_ADDR;   // PHASECAL_RESULT__REFERENCE_PHASE
    constexpr std::size_t kVcselStart = 0x00D8 - HISTOGRAM_BLOCK_ADDR; // PHASECAL_RESULT__VCSEL_START
    constexpr std::size_t kBin23Msb = 0x00D9 - HISTOGRAM_BLOCK_ADDR;   // RESULT__HISTOGRAM_BIN_23_0_MSB
    constexpr std::size_t kBin23Lsb = 0x00DA - HISTOGRAM_BLOCK_ADDR;   // RESULT__HISTOGRAM_BIN_23_0_LSB

    std::array<std::uint8_t, HISTOGRAM_BLOCK_LEN> buf{};
    for (std::size_t i = 0; i < HISTOGRAM_BLOCK_LEN; ++i) buf[i] = u8(raw[i]);
    buf[kBin23Lo] = static_cast<std::uint8_t>(((buf[kBin23Msb] << 2) + buf[kBin23Lsb]) & 0xFF);

    HistogramRaw r;
    r.interrupt_status = buf[0];
    r.range_status = buf[1];
    r.report_status = buf[2];
    r.stream_count = buf[3];
    r.dss_actual_effective_spads = static_cast<std::uint16_t>((buf[4] << 8) | buf[5]);
    r.reference_phase = static_cast<std::uint16_t>((buf[kRefPhase] << 8) | buf[kRefPhase + 1]);
    r.vcsel_start = buf[kVcselStart];
    for (std::size_t i = 0; i < HISTOGRAM_BINS; ++i) {
        const std::size_t o = kBin0 + 3 * i;
        r.bins[i] = (static_cast<std::uint32_t>(buf[o]) << 16) |
                    (static_cast<std::uint32_t>(buf[o + 1]) << 8) | buf[o + 2];
    }
    return r;
}

}  // namespace vl53lx
}  // namespace depz
