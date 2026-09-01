// VL53L4CD ToF register-bridge codecs + host-ULD math
// (contracts/10_SENSOR_VL53L4.md).
//
// The VL53L4CD is a single-zone Time-of-Flight ranger behind a thin I2C
// register bridge: the MCU owns nothing but the I2C bus, the XSHUT/INT pins
// and one streaming FSM. The ST Ultra-Lite Driver (ULD 2.2.3, STSW-IMG026)
// runs on the host as plain register access; this header carries the wire
// codecs for the bridge commands/reports plus the pure ULD math (result-block
// decode, SetRangeTiming/GetRangeTiming register math, tuning-word codecs and
// the default configuration block) held to byte-exact parity with the golden
// vectors. Register *contents* are big-endian (the bridge passes sensor bytes
// through untouched); all wire fields (addr, len, ...) are little-endian.
#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "depz/span.hpp"

namespace depz {
namespace vl53l4 {

// Bridge commands (0x32..0x38; 0x30/0x31 are the common sync pins).
enum class Vl53l4Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u16, len u16 -> RPT_VL53_REG_DATA
    WriteReg = 0x33,     // addr u16, data[1..253]
    Xshut = 0x34,        // action u8 (XSHUT_OFF / XSHUT_ON / XSHUT_RESET)
    StartStream = 0x35,  // addr u16, len u16, flags u8
    StopStream = 0x36,
    GetInfo = 0x37,      // -> RPT_VL53_INFO
    SetI2cSpeed = 0x38,  // khz u16 (clamped to the nearest nominal step)
};

// Bridge reports.
enum class Vl53l4Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA
    Info = 0x92,     // RPT_VL53_INFO
    Stream = 0x93,   // RPT_VL53_STREAM
};

// Max read len / write data length per transfer (STM32 I2C NBYTES is 8-bit; a
// write spends two bytes on the register address; the firmware applies the
// same limit to both directions).
inline constexpr std::uint16_t XFER_MAX = 253;

// VL53_XSHUT actions.
inline constexpr std::uint8_t XSHUT_OFF = 0;
inline constexpr std::uint8_t XSHUT_ON = 1;
inline constexpr std::uint8_t XSHUT_RESET = 2;  // answered after the boot handshake

// VL53_START_STREAM flags bit 1: INT active high, mirroring bit 4 of
// GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.
inline constexpr std::uint8_t SF_INT_ACT_HIGH = 0x02;

// The block the MCU streams: RESULT__RANGE_STATUS (0x0089) .. 0x0099 — every
// field of the ULD results struct in one read.
inline constexpr std::uint16_t RESULT_BLOCK_ADDR = 0x0089;
inline constexpr std::uint16_t RESULT_BLOCK_LEN = 17;

// IDENTIFICATION__MODEL_ID (0x010F..0x0110) expected value.
inline constexpr std::uint16_t MODEL_ID = 0xEBAA;

// First register of the 91-byte init configuration block (0x2D..0x87).
inline constexpr std::uint16_t CONFIG_ADDR = 0x2D;

// config_block() forces byte 0 (register 0x2D) to this value: I2C Fast Mode
// Plus pad, set unconditionally and never cleared.
inline constexpr std::uint8_t CONFIG_FMP_BYTE = 0x12;

// ── command encoders ────────────────────────────────────────────────────────

// VL53_READ_REG payload: addr u16, len u16 (both little-endian).
bytes pack_read_reg(std::uint16_t addr, std::uint16_t len);

// VL53_WRITE_REG payload: addr u16 then the raw register data (1..XFER_MAX).
bytes pack_write_reg(std::uint16_t addr, byte_span data);

// VL53_XSHUT payload: action u8 (XSHUT_OFF / XSHUT_ON / XSHUT_RESET).
bytes pack_xshut(std::uint8_t action);

// VL53_START_STREAM payload: addr u16, len u16, flags u8 (SF_INT_ACT_HIGH).
bytes pack_start_stream(std::uint16_t addr, std::uint16_t len, std::uint8_t flags);

// VL53_SET_I2C_SPEED payload: khz u16 (firmware clamps to the nearest step).
bytes pack_set_i2c_speed(std::uint16_t khz);

// ── report decoders ─────────────────────────────────────────────────────────

// RPT_VL53_REG_DATA — one I2C read result. `cmd` echoes 0x32; `timestamp_us`
// is MCU uptime at I2C-read completion.
struct RegData {
    std::uint8_t cmd = 0;
    std::uint64_t timestamp_us = 0;
    bytes data;

    // Decode a RPT_VL53_REG_DATA payload (9+N bytes); nullopt when too short.
    static std::optional<RegData> unpack(byte_span payload);
};

// RPT_VL53_INFO — bridge diagnostics (21-byte little-endian payload).
// Counters are free-running and wrap silently; watch increments, not absolute
// values. Not a data path: safe to request while streaming.
struct Vl53l4Info {
    std::uint32_t int_edges = 0;
    std::uint32_t slots_skipped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint16_t model_id = 0;       // expected MODEL_ID (0xEBAA)
    std::uint8_t fw_status = 0;       // expected 0x03 (booted)
    std::uint8_t initialized = 0;     // 1 = MODEL_ID matched on this read
    std::uint8_t xshut_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;

    // Decode a RPT_VL53_INFO payload; nullopt when shorter than 21 bytes.
    static std::optional<Vl53l4Info> unpack(byte_span payload);
};

// RPT_VL53_STREAM — one streamed register block. `timestamp_us` is MCU uptime
// at the INT edge (the sensor event); `addr`/`len` echo the stream
// configuration so each report is self-describing.
struct StreamData {
    std::uint64_t timestamp_us = 0;
    std::uint16_t addr = 0;
    std::uint16_t len = 0;
    bytes data;  // payload[12..12+len]

    // Decode a RPT_VL53_STREAM payload (12+len bytes); nullopt when too short.
    static std::optional<StreamData> unpack(byte_span payload);
};

// ── result-block decode (VL53L4CD_GetResult) ────────────────────────────────

// VL53L4CD_ResultsData_t plus the sensor's own frame counter. Rates are kcps,
// distances/sigma are millimetres; everything is integer math per the vectors.
struct Vl53l4Result {
    int range_status = 0;  // 0 = valid; raw >= 24 passes through unmapped
    int distance_mm = 0;
    int ambient_rate_kcps = 0;
    int ambient_per_spad_kcps = 0;
    int signal_rate_kcps = 0;
    int signal_per_spad_kcps = 0;
    int number_of_spad = 0;
    int sigma_mm = 0;
    int stream_count = 0;  // sensor frame counter, wraps at 255
};

// Decode the streamed RESULT_BLOCK_ADDR block exactly as VL53L4CD_GetResult()
// decodes the same registers read one by one (big-endian words, x8 rates,
// /4 sigma, per-SPAD rate x256/raw_spads with zero SPADs -> 0). Returns
// nullopt when raw is shorter than 15 bytes.
std::optional<Vl53l4Result> parse_result_block(byte_span raw);

// ── timing math (SetRangeTiming / GetRangeTiming) ───────────────────────────

// The register words SetRangeTiming programs: RANGE_CONFIG_A (0x005E),
// RANGE_CONFIG_B (0x0061) and the INTERMEASUREMENT_MS (0x006C) raw dword.
struct RangeTimingRegs {
    std::uint16_t range_config_a = 0;
    std::uint16_t range_config_b = 0;
    std::uint32_t intermeasurement_raw = 0;
};

// SetRangeTiming register math. `osc_frequency` is the word read from 0x0006;
// `clock_pll` the word from RESULT__OSC_CALIBRATE_VAL (used only in autonomous
// mode). budget 10..200 ms; inter_ms == 0 selects continuous mode, a value
// greater than the budget selects autonomous low power. Returns nullopt when
// osc_frequency is 0, the budget is out of range, or 0 < inter_ms <= budget.
// Bit-exact with the ULD (32-bit truncations, 1.055 PLL factor).
std::optional<RangeTimingRegs> range_timing_registers(std::uint32_t budget_ms,
                                                      std::uint32_t inter_ms,
                                                      std::uint16_t osc_frequency,
                                                      std::uint16_t clock_pll);

// GetRangeTiming result: the user-facing milliseconds.
struct RangeTiming {
    std::uint32_t timing_budget_ms = 0;
    std::uint32_t inter_measurement_ms = 0;
};

// GetRangeTiming register math from the raw register reads (INTERMEASUREMENT_MS
// dword, RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word, RANGE_CONFIG_A word).
// Returns nullopt when osc_frequency reads 0. Bit-exact with the ULD (32-bit
// truncations, 1.065 PLL factor).
std::optional<RangeTiming> decode_range_timing(std::uint32_t intermeasurement_raw,
                                               std::uint16_t clock_pll,
                                               std::uint16_t osc_frequency,
                                               std::uint16_t range_config_a);

// ── tuning-register codecs (register word <-> user units) ───────────────────

// RANGE_OFFSET_MM (0x001E) word for SetOffset (offset x4; INNER/OUTER zeroed
// alongside).
std::uint16_t offset_raw(int offset_mm);

// GetOffset: RANGE_OFFSET_MM word -> signed millimetres.
int decode_offset(std::uint16_t raw_word);

// XTALK_PLANE_OFFSET_KCPS (0x0016) word for SetXtalk (kcps x512).
std::uint16_t xtalk_raw(int xtalk_kcps);

// GetXtalk: XTALK_PLANE_OFFSET_KCPS word -> kcps (std::lround(raw / 512.0)).
int decode_xtalk(std::uint16_t raw_word);

// MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066) word for SetSignalThreshold (kcps /8).
std::uint16_t signal_threshold_raw(int signal_kcps);

// GetSignalThreshold: register word -> kcps.
int decode_signal_threshold(std::uint16_t raw_word);

// RANGE_CONFIG__SIGMA_THRESH (0x0064) word for SetSigmaThreshold (mm x4);
// nullopt when sigma_mm > 16383 (the word would overflow).
std::optional<std::uint16_t> sigma_threshold_raw(int sigma_mm);

// GetSigmaThreshold: register word -> millimetres.
int decode_sigma_threshold(std::uint16_t raw_word);

// ── init configuration block ────────────────────────────────────────────────

// The stock ST VL53L4CD_DEFAULT_CONFIGURATION[] table — 91 bytes for
// registers 0x2D..0x87 (ULD 2.2.3), untouched.
const std::array<std::uint8_t, 91>& default_configuration();

// The 91-byte block sensor_init writes at CONFIG_ADDR: the ST default
// configuration with byte 0 forced to CONFIG_FMP_BYTE (I2C Fast Mode Plus).
bytes config_block();

}  // namespace vl53l4
}  // namespace depz
