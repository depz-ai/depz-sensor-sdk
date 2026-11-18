#include "depz/bno055.hpp"

#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace bno055 {

using detail::packed;
using detail::to_bytes;
using detail::u8p;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint8_t addr, std::uint8_t len) {
    std::uint8_t out[2];
    return packed(out, depz_bno055_pack_read_reg(addr, len, out),
                  "BNO_READ_REG: len must be 1..128 and addr+len <= 0x100");
}

bytes pack_write_reg(std::uint8_t addr, byte_span data) {
    bytes out(1 + data.size());
    const std::size_t n = depz_bno055_pack_write_reg(addr, u8p(data), data.size(),
                                                     reinterpret_cast<std::uint8_t*>(out.data()));
    if (n == 0) throw std::invalid_argument("BNO_WRITE_REG: data must be 1..128 bytes and addr+N <= 0x100");
    out.resize(n);
    return out;
}

bytes pack_start_stream(std::uint8_t trigger, std::uint8_t addr, std::uint8_t len, std::uint16_t period_ms) {
    std::uint8_t out[5];
    return packed(out, depz_bno055_pack_start_stream(trigger, addr, len, period_ms, out),
                  "BNO_START_STREAM: TIMER period_ms must be 1..60000");
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<RegData> RegData::unpack(byte_span payload) {
    depz_bno055_reg_data c{};
    if (depz_bno055_unpack_reg_data(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return RegData{c.cmd, c.timestamp_us, to_bytes(c.data, c.data_len)};
}

std::optional<Bno055Info> Bno055Info::unpack(byte_span payload) {
    Bno055Info i{};
    if (depz_bno055_unpack_info(u8p(payload), payload.size(), &i) != 0) return std::nullopt;
    return i;
}

std::optional<StreamData> StreamData::unpack(byte_span payload) {
    depz_bno055_stream c{};
    if (depz_bno055_unpack_stream(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return StreamData{c.timestamp_us, c.addr, c.len, to_bytes(c.data, c.len)};
}

// ── units ───────────────────────────────────────────────────────────────────

Units Units::unpack(std::uint8_t unit_sel) {
    Units u;
    depz_bno055_unpack_units(unit_sel, &u);
    return u;
}

// ── calibration ─────────────────────────────────────────────────────────────

CalibStatus CalibStatus::unpack(std::uint8_t value) {
    CalibStatus s;
    depz_bno055_unpack_calib_status(value, &s);
    return s;
}

bytes CalibrationProfile::pack() const {
    std::uint8_t out[DEPZ_BNO055_CALIB_PROFILE_LEN];
    return to_bytes(out, depz_bno055_pack_calib_profile(this, out));
}

std::optional<CalibrationProfile> CalibrationProfile::unpack(byte_span data) {
    CalibrationProfile p;
    if (depz_bno055_unpack_calib_profile(u8p(data), data.size(), &p) != 0) return std::nullopt;
    return p;
}

// ── axis remap ──────────────────────────────────────────────────────────────

std::optional<std::pair<std::uint8_t, std::uint8_t>> AxisRemap::pack() const {
    std::uint8_t config = 0, sign = 0;
    if (depz_bno055_pack_axis_remap(this, &config, &sign) != 0) return std::nullopt;
    return std::make_pair(config, sign);
}

AxisRemap AxisRemap::unpack(std::uint8_t config, std::uint8_t sign) {
    AxisRemap r;
    depz_bno055_unpack_axis_remap(config, sign, &r);
    return r;
}

std::optional<AxisRemap> placement(const std::string& name) {
    AxisRemap r;
    if (!detail::whole_cstr(name) || depz_bno055_placement(name.c_str(), &r) != 0) return std::nullopt;
    return r;
}

// ── page-1 sensor configuration ─────────────────────────────────────────────

AccelConfig AccelConfig::unpack(std::uint8_t value) {
    AccelConfig c;
    depz_bno055_unpack_accel_config(value, &c);
    return c;
}

bytes GyroConfig::pack() const {
    std::uint8_t out[2];
    return to_bytes(out, depz_bno055_pack_gyro_config(this, out));
}

std::optional<GyroConfig> GyroConfig::unpack(byte_span data) {
    GyroConfig c;
    if (depz_bno055_unpack_gyro_config_n(u8p(data), data.size(), &c) != 0) return std::nullopt;
    return c;
}

MagConfig MagConfig::unpack(std::uint8_t value) {
    MagConfig c;
    depz_bno055_unpack_mag_config(value, &c);
    return c;
}

// ── register-window decode ──────────────────────────────────────────────────

RawBlock decode_block(std::uint8_t addr, byte_span data) {
    depz_bno055_block c{};
    depz_bno055_decode_block(addr, u8p(data), data.size(), &c);
    auto v3 = [](const std::int16_t* p) { return Vec3{p[0], p[1], p[2]}; };
    RawBlock b;
    if (c.has_accel) b.accel = v3(c.accel);
    if (c.has_mag) b.mag = v3(c.mag);
    if (c.has_gyro) b.gyro = v3(c.gyro);
    if (c.has_euler) b.euler = v3(c.euler);
    if (c.has_quaternion) b.quaternion = Quat{c.quaternion[0], c.quaternion[1], c.quaternion[2], c.quaternion[3]};
    if (c.has_linear_accel) b.linear_accel = v3(c.linear_accel);
    if (c.has_gravity) b.gravity = v3(c.gravity);
    if (c.has_temperature) b.temperature = c.temperature;
    if (c.has_calib_stat) b.calib_stat = c.calib_stat;
    return b;
}

}  // namespace bno055
}  // namespace depz
