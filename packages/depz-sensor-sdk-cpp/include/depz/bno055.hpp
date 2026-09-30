// BNO055 9-axis IMU on the APP_BNO055 register bridge
// (contracts/13_SENSOR_BNO055.md, protocol v0.10).
//
// The bridge is thin: the MCU owns the I2C bus (7-bit 0x28, 400 kHz), the
// reset pin and one streaming loop; Bosch's sensor fusion runs on the chip,
// so there is no host driver to port. Operating mode, units, axis remap and
// calibration are host logic expressed as register access.
//
// This header is the *base* layer every SDK implements (contract 13 §6): the
// wire codecs, the pure register codecs of §4 (units, CALIB_STAT, calibration
// profile, axis remap + placements, page-1 sensor configs) and the decoding
// of any register window into raw integers. Scaling is value = raw / LSB
// (§4.2). The live driver (mode switching, boot / fusion-start polling, page
// discipline, the scaled sample stream) is depz::Bno055 in depz/device.hpp.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "depz/span.hpp"

namespace depz {
namespace bno055 {

// Bridge commands (0x32..0x37; 0x30/0x31 are the common sync pins).
enum class Bno055Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u8, len u8 -> RPT_BNO_REG_DATA
    WriteReg = 0x33,     // addr u8, data[1..128]
    Reset = 0x34,        // no payload; answered after the chip-ID handshake (~0.5 s)
    StartStream = 0x35,  // trigger u8, addr u8, len u8, period_ms u16
    StopStream = 0x36,   // no payload
    GetInfo = 0x37,      // no payload -> RPT_BNO_INFO
};

enum class Bno055Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_BNO_REG_DATA
    Info = 0x92,     // RPT_BNO_INFO (38 bytes)
    Stream = 0x93,   // RPT_BNO_REG_STREAM
};

// Max bytes per READ_REG / WRITE_REG / streamed block; addr + len <= 0x100.
inline constexpr std::size_t XFER_MAX = 128;
// RPT_BNO_INFO payload size.
inline constexpr std::size_t INFO_SIZE = 38;

// BNO_START_STREAM trigger. Data-ready interrupts do not exist on sensor SW
// 03.11: TIMER is the only data trigger. INT reads on each INT rising edge
// (motion interrupts); period_ms is then a missed-edge watchdog, 0 = off.
inline constexpr std::uint8_t TRIGGER_TIMER = 0;
inline constexpr std::uint8_t TRIGGER_INT = 1;

// ── command encoders ────────────────────────────────────────────────────────
// BNO_RESET, BNO_STOP_STREAM and BNO_GET_INFO carry an empty payload.

// BNO_READ_REG payload: addr u8, len u8.
bytes pack_read_reg(std::uint8_t addr, std::uint8_t len);

// BNO_WRITE_REG payload: addr u8 then the raw register data (1..XFER_MAX).
bytes pack_write_reg(std::uint8_t addr, byte_span data);

// BNO_START_STREAM payload: trigger u8, addr u8, len u8, period_ms u16 LE.
bytes pack_start_stream(std::uint8_t trigger, std::uint8_t addr, std::uint8_t len,
                        std::uint16_t period_ms);

// ── report decoders ─────────────────────────────────────────────────────────

// RPT_BNO_REG_DATA — one register read. `cmd` echoes 0x32; `timestamp_us` is
// MCU uptime at I2C-read completion.
struct RegData {
    std::uint8_t cmd = 0;
    std::uint64_t timestamp_us = 0;
    bytes data;

    // Decode a RPT_BNO_REG_DATA payload (9+N bytes); nullopt when too short.
    static std::optional<RegData> unpack(byte_span payload);
};

// RPT_BNO_INFO (38 bytes, `<BBBBBHBBBIHHHIIIHBBH`) — sensor identity
// (registers 0x00..0x06) plus bridge diagnostics. Counters are free-running
// and wrap; read_*_us, slots_skipped and loop_max_us reset at START_STREAM. A
// rising sensor_resets means the bridge pulsed nRESET: the sensor is back in
// CONFIG and the host must re-apply its configuration.
struct Bno055Info {
    std::uint8_t i2c_addr = 0;        // 0x28
    std::uint8_t chip_id = 0;         // healthy 0xA0
    std::uint8_t acc_id = 0;          // 0xFB
    std::uint8_t mag_id = 0;          // 0x32
    std::uint8_t gyr_id = 0;          // 0x0F
    std::uint16_t sw_rev = 0;         // BCD: 0x0311 = 03.11
    std::uint8_t bl_rev = 0;
    std::uint8_t initialized = 0;     // 1 = chip-ID handshake passed
    std::uint8_t int_level = 0;
    std::uint32_t int_edges = 0;
    std::uint16_t read_min_us = 0;
    std::uint16_t read_max_us = 0;
    std::uint16_t read_avg_us = 0;
    std::uint32_t tx_dropped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint32_t slots_skipped = 0;
    std::uint16_t bus_recoveries = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint8_t sensor_resets = 0;
    std::uint16_t loop_max_us = 0;

    // Decode a RPT_BNO_INFO payload; nullopt when shorter than INFO_SIZE.
    static std::optional<Bno055Info> unpack(byte_span payload);
};

// RPT_BNO_REG_STREAM — one streamed register block. `addr`/`len` echo the
// stream configuration; `timestamp_us` is the trigger time (timer expiry or
// INT edge), not the I2C completion.
struct StreamData {
    std::uint64_t timestamp_us = 0;
    std::uint8_t addr = 0;
    std::uint8_t len = 0;
    bytes data;  // payload[10..10+len]

    // Decode a RPT_BNO_REG_STREAM payload (10+len bytes); nullopt when shorter
    // than the 10-byte header.
    static std::optional<StreamData> unpack(byte_span payload);
};

// ── register map essentials (§4.1; page 0 unless REG1_) ─────────────────────

inline constexpr std::uint8_t REG_CHIP_ID = 0x00;
inline constexpr std::uint8_t REG_PAGE_ID = 0x07;          // host-owned; always back to 0
inline constexpr std::uint8_t REG_ACC_DATA = 0x08;         // 3 x i16 LE (x, y, z)
inline constexpr std::uint8_t REG_MAG_DATA = 0x0E;
inline constexpr std::uint8_t REG_GYR_DATA = 0x14;
inline constexpr std::uint8_t REG_EUL_DATA = 0x1A;         // heading, roll, pitch
inline constexpr std::uint8_t REG_QUA_DATA = 0x20;         // 4 x i16: w, x, y, z
inline constexpr std::uint8_t REG_LIA_DATA = 0x28;         // linear accel (gravity removed)
inline constexpr std::uint8_t REG_GRV_DATA = 0x2E;         // gravity vector
inline constexpr std::uint8_t REG_TEMP = 0x34;             // i8
inline constexpr std::uint8_t REG_CALIB_STAT = 0x35;
inline constexpr std::uint8_t REG_ST_RESULT = 0x36;
inline constexpr std::uint8_t REG_INT_STA = 0x37;          // clears on read
inline constexpr std::uint8_t REG_SYS_CLK_STATUS = 0x38;
inline constexpr std::uint8_t REG_SYS_STATUS = 0x39;
inline constexpr std::uint8_t REG_SYS_ERR = 0x3A;
inline constexpr std::uint8_t REG_UNIT_SEL = 0x3B;
inline constexpr std::uint8_t REG_OPR_MODE = 0x3D;         // bits 3:0
inline constexpr std::uint8_t REG_PWR_MODE = 0x3E;
inline constexpr std::uint8_t REG_SYS_TRIGGER = 0x3F;
inline constexpr std::uint8_t REG_TEMP_SOURCE = 0x40;
inline constexpr std::uint8_t REG_AXIS_MAP_CONFIG = 0x41;
inline constexpr std::uint8_t REG_AXIS_MAP_SIGN = 0x42;
inline constexpr std::uint8_t REG_SIC_MATRIX = 0x43;       // 9 x i16, 1.0 = 16384
inline constexpr std::uint8_t REG_CALIB_PROFILE = 0x55;    // 22 bytes, CONFIG mode only
inline constexpr std::uint8_t REG1_ACC_CONFIG = 0x08;
inline constexpr std::uint8_t REG1_MAG_CONFIG = 0x09;
inline constexpr std::uint8_t REG1_GYR_CONFIG_0 = 0x0A;
inline constexpr std::uint8_t REG1_GYR_CONFIG_1 = 0x0B;
inline constexpr std::uint8_t REG1_INT_MSK = 0x0F;
inline constexpr std::uint8_t REG1_INT_EN = 0x10;
inline constexpr std::uint8_t REG1_UNIQUE_ID = 0x50;       // 16 bytes

// Every output channel in one read (ACC_DATA_X_LSB .. CALIB_STAT), and the
// quaternion alone (the cheapest orientation read).
inline constexpr std::uint8_t FULL_BLOCK_ADDR = REG_ACC_DATA;
inline constexpr std::uint8_t FULL_BLOCK_LEN = 46;
inline constexpr std::uint8_t QUAT_BLOCK_ADDR = REG_QUA_DATA;
inline constexpr std::uint8_t QUAT_BLOCK_LEN = 8;

// OPR_MODE (0x3D) bits 3:0; IMU and above are fusion modes.
enum class OprMode : std::uint8_t {
    Config = 0x00,
    AccOnly = 0x01,
    MagOnly = 0x02,
    GyroOnly = 0x03,
    AccMag = 0x04,
    AccGyro = 0x05,
    MagGyro = 0x06,
    Amg = 0x07,
    Imu = 0x08,
    Compass = 0x09,
    M4g = 0x0A,
    NdofFmcOff = 0x0B,
    Ndof = 0x0C,
};

// ── units (UNIT_SEL 0x3B, §4.2 — as the silicon implements them) ────────────

inline constexpr std::uint8_t UNIT_ACC_MG = 0x01;       // ACC_DATA in mg, else m/s^2
inline constexpr std::uint8_t UNIT_GYR_RPS = 0x02;      // rad/s, else dps
inline constexpr std::uint8_t UNIT_EUL_RAD = 0x04;      // radians, else degrees
inline constexpr std::uint8_t UNIT_TEMP_F = 0x10;       // deg F (1 LSB = 2 F), else C
inline constexpr std::uint8_t UNIT_ORI_ANDROID = 0x80;  // the power-on value is 0x80

// Output units. The default is UNIT_SEL = 0x00 (m/s^2, dps, degrees, deg C,
// Windows orientation); a fresh sensor powers up in Android orientation, so
// hosts write UNIT_SEL explicitly.
struct Units {
    bool accel_mg = false;   // ACC_DATA only: linear accel / gravity stay m/s^2
    bool gyro_rps = false;
    bool euler_rad = false;
    bool temp_f = false;
    bool android = false;

    std::uint8_t pack() const;
    // Undefined UNIT_SEL bits are ignored (they do nothing on the sensor).
    static Units unpack(std::uint8_t unit_sel);

    // LSB per unit: value = raw / lsb.
    double accel_lsb() const { return accel_mg ? 1.0 : 100.0; }
    double gyro_lsb() const { return gyro_rps ? 900.0 : 16.0; }
    double euler_lsb() const { return euler_rad ? 900.0 : 16.0; }
    double temp_lsb() const { return temp_f ? 0.5 : 1.0; }  // deg F: 1 LSB = 2 F

    bool operator==(const Units& o) const {
        return accel_mg == o.accel_mg && gyro_rps == o.gyro_rps && euler_rad == o.euler_rad &&
               temp_f == o.temp_f && android == o.android;
    }
};

inline constexpr double MAG_LSB = 16.0;       // uT, not selectable
inline constexpr double QUAT_LSB = 16384.0;   // 2^14, unit-less
// Linear acceleration and gravity ignore the ACC_Unit bit: always m/s^2 at
// 100 LSB (measured on SW 03.11; the datasheet's Tables 3-33/3-35 say mg).
inline constexpr double FUSION_ACCEL_LSB = 100.0;

// ── calibration (§4.3) ──────────────────────────────────────────────────────

// CALIB_STAT (0x35): sys<7:6> gyr<5:4> acc<3:2> mag<1:0>, 0 = not
// calibrated .. 3 = fully calibrated.
struct CalibStatus {
    std::uint8_t system = 0;
    std::uint8_t gyro = 0;
    std::uint8_t accel = 0;
    std::uint8_t mag = 0;

    std::uint8_t pack() const;
    static CalibStatus unpack(std::uint8_t value);
    // 3/3/3/3.
    bool fully_calibrated() const { return system == 3 && gyro == 3 && accel == 3 && mag == 3; }
};

inline constexpr std::size_t CALIB_PROFILE_LEN = 22;

// Offsets and radii at 0x55..0x6A (11 x i16 LE, sensor LSB). Readable and
// writable only in CONFIG; write all 22 bytes in one transfer (the sensor
// latches each group on its MSB). A written profile is a starting point: the
// sensor keeps calibrating once back in a fusion mode.
struct CalibrationProfile {
    std::array<std::int16_t, 3> accel_offset{};
    std::array<std::int16_t, 3> mag_offset{};
    std::array<std::int16_t, 3> gyro_offset{};
    std::int16_t accel_radius = 0;
    std::int16_t mag_radius = 0;

    bytes pack() const;  // 22 bytes
    // nullopt unless data is exactly CALIB_PROFILE_LEN bytes.
    static std::optional<CalibrationProfile> unpack(byte_span data);
};

// ── axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42, §4.4) ────────────

inline constexpr std::uint8_t AXIS_X = 0;
inline constexpr std::uint8_t AXIS_Y = 1;
inline constexpr std::uint8_t AXIS_Z = 2;

// Which chip axis feeds each output axis, and its sign: `x = AXIS_Y` means
// "output X is the chip's Y axis".
struct AxisRemap {
    std::uint8_t x = AXIS_X;
    std::uint8_t y = AXIS_Y;
    std::uint8_t z = AXIS_Z;
    bool x_negative = false;
    bool y_negative = false;
    bool z_negative = false;

    // (AXIS_MAP_CONFIG = z<5:4> y<3:2> x<1:0>, AXIS_MAP_SIGN = x 2, y 1, z 0).
    // nullopt when x/y/z is not a permutation of 0/1/2: the sensor would
    // silently keep its old mapping.
    std::optional<std::pair<std::uint8_t, std::uint8_t>> pack() const;
    static AxisRemap unpack(std::uint8_t config, std::uint8_t sign);

    bool operator==(const AxisRemap& o) const {
        return x == o.x && y == o.y && z == o.z && x_negative == o.x_negative &&
               y_negative == o.y_negative && z_negative == o.z_negative;
    }
};

// Datasheet §3.4 mounting placements P0..P7 as (AXIS_MAP_CONFIG,
// AXIS_MAP_SIGN); P1 (0x24/0x00) is the power-on default.
inline constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 8> PLACEMENTS = {{
    {0x21, 0x04},  // P0
    {0x24, 0x00},  // P1 (default)
    {0x24, 0x06},  // P2
    {0x21, 0x02},  // P3
    {0x24, 0x03},  // P4
    {0x21, 0x01},  // P5
    {0x21, 0x07},  // P6
    {0x24, 0x05},  // P7
}};

// "P0".."P7" (case-insensitive); nullopt for any other name.
std::optional<AxisRemap> placement(const std::string& name);

// ── page-1 sensor configuration (effective in non-fusion modes only) ────────

// ACC_CONFIG (page 1, 0x08): range<1:0> (2/4/8/16 g), bandwidth<4:2>
// (7.81..1000 Hz), power<7:5>. Power-on 0x0D = 4 g, 62.5 Hz, normal.
struct AccelConfig {
    std::uint8_t range = 1;
    std::uint8_t bandwidth = 3;
    std::uint8_t power = 0;

    std::uint8_t pack() const;
    static AccelConfig unpack(std::uint8_t value);
};

// GYR_CONFIG_0/1 (page 1, 0x0A/0x0B): byte 0 range<2:0> (2000..125 dps),
// bandwidth<5:3>; byte 1 power<2:0>. Power-on 0x38/0x00 = 2000 dps, 32 Hz.
struct GyroConfig {
    std::uint8_t range = 0;
    std::uint8_t bandwidth = 7;
    std::uint8_t power = 0;

    bytes pack() const;  // 2 bytes
    // nullopt when data is shorter than 2 bytes.
    static std::optional<GyroConfig> unpack(byte_span data);
};

// MAG_CONFIG (page 1, 0x09): rate<2:0> (2..30 Hz), mode<4:3>, power<6:5>;
// bit 7 is not a field, so a repack drops it. Power-on 0x0B = 10 Hz, regular.
struct MagConfig {
    std::uint8_t rate = 3;
    std::uint8_t mode = 1;
    std::uint8_t power = 0;

    std::uint8_t pack() const;
    static MagConfig unpack(std::uint8_t value);
};

// ── register-window decode (§4.1) ───────────────────────────────────────────

using Vec3 = std::array<std::int16_t, 3>;
using Quat = std::array<std::int16_t, 4>;

// Raw register values found in one block read. A channel is nullopt when the
// window addr..addr+len does not cover all of its bytes.
struct RawBlock {
    std::optional<Vec3> accel;
    std::optional<Vec3> mag;
    std::optional<Vec3> gyro;
    std::optional<Vec3> euler;         // heading, roll, pitch
    std::optional<Quat> quaternion;    // w, x, y, z
    std::optional<Vec3> linear_accel;
    std::optional<Vec3> gravity;
    std::optional<std::int8_t> temperature;
    std::optional<std::uint8_t> calib_stat;  // CALIB_STAT byte
};

// Unpack whatever channels the register window starting at `addr` holds.
RawBlock decode_block(std::uint8_t addr, byte_span data);

}  // namespace bno055
}  // namespace depz
