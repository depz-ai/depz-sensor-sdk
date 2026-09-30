#include "depz/bno055.hpp"

#include <algorithm>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace bno055 {

using detail::rd_i16;
using detail::rd_u16;
using detail::rd_u32;
using detail::rd_u64;
using detail::u8;
using detail::wr_le;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint8_t addr, std::uint8_t len) {
    bytes out;
    wr_le(out, addr, 1);
    wr_le(out, len, 1);
    return out;
}

bytes pack_write_reg(std::uint8_t addr, byte_span data) {
    bytes out;
    out.reserve(1 + data.size());
    wr_le(out, addr, 1);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

bytes pack_start_stream(std::uint8_t trigger, std::uint8_t addr, std::uint8_t len,
                        std::uint16_t period_ms) {
    bytes out;
    out.reserve(5);
    wr_le(out, trigger, 1);
    wr_le(out, addr, 1);
    wr_le(out, len, 1);
    wr_le(out, period_ms, 2);
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

std::optional<Bno055Info> Bno055Info::unpack(byte_span payload) {
    // `<BBBBBHBBBIHHHIIIHBBH`
    if (payload.size() < INFO_SIZE) return std::nullopt;
    Bno055Info r;
    r.i2c_addr = u8(payload[0]);
    r.chip_id = u8(payload[1]);
    r.acc_id = u8(payload[2]);
    r.mag_id = u8(payload[3]);
    r.gyr_id = u8(payload[4]);
    r.sw_rev = rd_u16(payload, 5);
    r.bl_rev = u8(payload[7]);
    r.initialized = u8(payload[8]);
    r.int_level = u8(payload[9]);
    r.int_edges = rd_u32(payload, 10);
    r.read_min_us = rd_u16(payload, 14);
    r.read_max_us = rd_u16(payload, 16);
    r.read_avg_us = rd_u16(payload, 18);
    r.tx_dropped = rd_u32(payload, 20);
    r.i2c_errors = rd_u32(payload, 24);
    r.slots_skipped = rd_u32(payload, 28);
    r.bus_recoveries = rd_u16(payload, 32);
    r.last_i2c_error = u8(payload[34]);
    r.sensor_resets = u8(payload[35]);
    r.loop_max_us = rd_u16(payload, 36);
    return r;
}

std::optional<StreamData> StreamData::unpack(byte_span payload) {
    // `<QBB` + data[len]
    if (payload.size() < 10) return std::nullopt;
    StreamData r;
    r.timestamp_us = rd_u64(payload, 0);
    r.addr = u8(payload[8]);
    r.len = u8(payload[9]);
    const std::size_t n = std::min<std::size_t>(r.len, payload.size() - 10);
    r.data.assign(payload.begin() + 10, payload.begin() + 10 + n);
    return r;
}

// ── units ───────────────────────────────────────────────────────────────────

std::uint8_t Units::pack() const {
    return static_cast<std::uint8_t>((accel_mg ? UNIT_ACC_MG : 0) | (gyro_rps ? UNIT_GYR_RPS : 0) |
                                     (euler_rad ? UNIT_EUL_RAD : 0) | (temp_f ? UNIT_TEMP_F : 0) |
                                     (android ? UNIT_ORI_ANDROID : 0));
}

Units Units::unpack(std::uint8_t v) {
    Units u;
    u.accel_mg = (v & UNIT_ACC_MG) != 0;
    u.gyro_rps = (v & UNIT_GYR_RPS) != 0;
    u.euler_rad = (v & UNIT_EUL_RAD) != 0;
    u.temp_f = (v & UNIT_TEMP_F) != 0;
    u.android = (v & UNIT_ORI_ANDROID) != 0;
    return u;
}

// ── calibration ─────────────────────────────────────────────────────────────

std::uint8_t CalibStatus::pack() const {
    return static_cast<std::uint8_t>((system & 3) << 6 | (gyro & 3) << 4 | (accel & 3) << 2 |
                                     (mag & 3));
}

CalibStatus CalibStatus::unpack(std::uint8_t v) {
    CalibStatus s;
    s.system = static_cast<std::uint8_t>(v >> 6 & 3);
    s.gyro = static_cast<std::uint8_t>(v >> 4 & 3);
    s.accel = static_cast<std::uint8_t>(v >> 2 & 3);
    s.mag = static_cast<std::uint8_t>(v & 3);
    return s;
}

bytes CalibrationProfile::pack() const {
    // `<11h`: acc offset xyz, mag offset xyz, gyr offset xyz, acc/mag radius
    bytes out;
    out.reserve(CALIB_PROFILE_LEN);
    auto put = [&](std::int16_t v) { wr_le(out, static_cast<std::uint16_t>(v), 2); };
    for (std::int16_t v : accel_offset) put(v);
    for (std::int16_t v : mag_offset) put(v);
    for (std::int16_t v : gyro_offset) put(v);
    put(accel_radius);
    put(mag_radius);
    return out;
}

std::optional<CalibrationProfile> CalibrationProfile::unpack(byte_span data) {
    if (data.size() != CALIB_PROFILE_LEN) return std::nullopt;
    CalibrationProfile p;
    for (std::size_t i = 0; i < 3; ++i) {
        p.accel_offset[i] = rd_i16(data, 2 * i);
        p.mag_offset[i] = rd_i16(data, 6 + 2 * i);
        p.gyro_offset[i] = rd_i16(data, 12 + 2 * i);
    }
    p.accel_radius = rd_i16(data, 18);
    p.mag_radius = rd_i16(data, 20);
    return p;
}

// ── axis remap ──────────────────────────────────────────────────────────────

std::optional<std::pair<std::uint8_t, std::uint8_t>> AxisRemap::pack() const {
    // A permutation of 0/1/2: each chip axis used exactly once.
    unsigned seen = 0;
    for (std::uint8_t a : {x, y, z}) {
        if (a > AXIS_Z) return std::nullopt;
        seen |= 1u << a;
    }
    if (seen != 7u) return std::nullopt;
    const auto config = static_cast<std::uint8_t>(z << 4 | y << 2 | x);
    const auto sign = static_cast<std::uint8_t>((x_negative ? 4 : 0) | (y_negative ? 2 : 0) |
                                                (z_negative ? 1 : 0));
    return std::make_pair(config, sign);
}

AxisRemap AxisRemap::unpack(std::uint8_t config, std::uint8_t sign) {
    AxisRemap a;
    a.x = static_cast<std::uint8_t>(config & 3);
    a.y = static_cast<std::uint8_t>(config >> 2 & 3);
    a.z = static_cast<std::uint8_t>(config >> 4 & 3);
    a.x_negative = (sign & 4) != 0;
    a.y_negative = (sign & 2) != 0;
    a.z_negative = (sign & 1) != 0;
    return a;
}

std::optional<AxisRemap> placement(const std::string& name) {
    if (name.size() != 2 || (name[0] != 'P' && name[0] != 'p') || name[1] < '0' || name[1] > '7')
        return std::nullopt;
    const auto& p = PLACEMENTS[static_cast<std::size_t>(name[1] - '0')];
    return AxisRemap::unpack(p.first, p.second);
}

// ── page-1 sensor configuration ─────────────────────────────────────────────

std::uint8_t AccelConfig::pack() const {
    return static_cast<std::uint8_t>((power & 7) << 5 | (bandwidth & 7) << 2 | (range & 3));
}

AccelConfig AccelConfig::unpack(std::uint8_t v) {
    AccelConfig c;
    c.range = static_cast<std::uint8_t>(v & 3);
    c.bandwidth = static_cast<std::uint8_t>(v >> 2 & 7);
    c.power = static_cast<std::uint8_t>(v >> 5 & 7);
    return c;
}

bytes GyroConfig::pack() const {
    bytes out;
    wr_le(out, static_cast<std::uint8_t>((bandwidth & 7) << 3 | (range & 7)), 1);
    wr_le(out, static_cast<std::uint8_t>(power & 7), 1);
    return out;
}

std::optional<GyroConfig> GyroConfig::unpack(byte_span data) {
    if (data.size() < 2) return std::nullopt;
    GyroConfig c;
    c.range = static_cast<std::uint8_t>(u8(data[0]) & 7);
    c.bandwidth = static_cast<std::uint8_t>(u8(data[0]) >> 3 & 7);
    c.power = static_cast<std::uint8_t>(u8(data[1]) & 7);
    return c;
}

std::uint8_t MagConfig::pack() const {
    return static_cast<std::uint8_t>((power & 3) << 5 | (mode & 3) << 3 | (rate & 7));
}

MagConfig MagConfig::unpack(std::uint8_t v) {
    MagConfig c;
    c.rate = static_cast<std::uint8_t>(v & 7);
    c.mode = static_cast<std::uint8_t>(v >> 3 & 3);
    c.power = static_cast<std::uint8_t>(v >> 5 & 3);
    return c;
}

// ── register-window decode ──────────────────────────────────────────────────

namespace {

// Offset of `reg` in the window when the window covers reg..reg+n-1.
std::optional<std::size_t> covered(std::uint8_t addr, std::size_t len, std::uint8_t reg,
                                   std::size_t n) {
    if (addr > reg || static_cast<std::size_t>(reg) + n > addr + len) return std::nullopt;
    return static_cast<std::size_t>(reg - addr);
}

template <std::size_t N>
std::optional<std::array<std::int16_t, N>> words(std::uint8_t addr, byte_span data,
                                                 std::uint8_t reg) {
    auto off = covered(addr, data.size(), reg, 2 * N);
    if (!off) return std::nullopt;
    std::array<std::int16_t, N> v{};
    for (std::size_t i = 0; i < N; ++i) v[i] = rd_i16(data, *off + 2 * i);
    return v;
}

}  // namespace

RawBlock decode_block(std::uint8_t addr, byte_span data) {
    RawBlock b;
    b.accel = words<3>(addr, data, REG_ACC_DATA);
    b.mag = words<3>(addr, data, REG_MAG_DATA);
    b.gyro = words<3>(addr, data, REG_GYR_DATA);
    b.euler = words<3>(addr, data, REG_EUL_DATA);
    b.quaternion = words<4>(addr, data, REG_QUA_DATA);
    b.linear_accel = words<3>(addr, data, REG_LIA_DATA);
    b.gravity = words<3>(addr, data, REG_GRV_DATA);
    if (auto off = covered(addr, data.size(), REG_TEMP, 1))
        b.temperature = static_cast<std::int8_t>(u8(data[*off]));
    if (auto off = covered(addr, data.size(), REG_CALIB_STAT, 1)) b.calib_stat = u8(data[*off]);
    return b;
}

}  // namespace bno055
}  // namespace depz
