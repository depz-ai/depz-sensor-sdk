// VL53L 1D ToF family on the APP_VL53L0_4 bridge — VL53L0X / L1CX / L1CB /
// L3CX / L4CD / L4CX (contracts/12_SENSOR_VL53LX.md, protocol v2.00).
//
// One bridge firmware serves six products and knows no sensor: every ULD runs
// on the host. v2.00 is a delta against the VL53L4CD bridge of contract 10
// (depz/vl53l4.hpp): the register-address width (VL53_SET_ADDR_WIDTH, new),
// the interrupt-release writes (now carried by VL53_START_STREAM) and the boot
// handshake (no longer inside VL53_XSHUT) moved to the host. READ_REG,
// WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA / STREAM
// reports are the contract-10 codecs and are re-exported from depz::vl53l4.
//
// This header is the *base* layer every SDK implements (contract 12 §4): the
// wire codecs, the product table, the class resolution rule and the three
// stateless block decoders. The live drivers (ULD / ULP / Bare histogram
// driver) are intentionally NOT ported — see the reference Python
// `vl53lx/uld/`.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "depz/span.hpp"
#include "depz/vl53l4.hpp"

namespace depz {
namespace vl53lx {

// Bridge commands (0x32..0x3A; 0x30/0x31 are the common sync pins).
enum class Vl53lxCmd : std::uint8_t {
    ReadReg = 0x32,       // addr u16, len u16 -> RPT_VL53_REG_DATA
    WriteReg = 0x33,      // addr u16, data[1..253]
    Xshut = 0x34,         // action u8; RESET has no boot handshake (host polls)
    StartStream = 0x35,   // addr u16, len u16, flags u8, n_clear u8, clear[n]
    StopStream = 0x36,
    GetInfo = 0x37,       // -> RPT_VL53_INFO (23 bytes)
    SetI2cSpeed = 0x38,   // khz u16
    SetAddrWidth = 0x39,  // width u8 (1 or 2); sticky, 2 after a reset
    ClearI2cErrors = 0x3A,  // v2.01 (fw v0.24), no payload; after every sensor init
};

// Bridge reports (0x91 / 0x93 are contract 10 unchanged).
enum class Vl53lxRpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA (decode with RegData)
    Info = 0x92,     // RPT_VL53_INFO (v2.00 layout, Vl53lxInfo)
    Stream = 0x93,   // RPT_VL53_STREAM (decode with StreamData)
};

// Identical on the wire to contract 10.
using vl53l4::SF_INT_ACT_HIGH;
using vl53l4::XFER_MAX;
using vl53l4::XSHUT_OFF;
using vl53l4::XSHUT_ON;
using vl53l4::XSHUT_RESET;
using RegData = vl53l4::RegData;
using StreamData = vl53l4::StreamData;

// Interrupt-release steps one START_STREAM may carry (VL53_CLEAR_STEPS_WIRE_MAX).
inline constexpr std::size_t CLEAR_STEPS_MAX = 4;
// RPT_VL53_INFO payload size (v2.00).
inline constexpr std::size_t INFO_SIZE = 23;

// The streamed blocks (contract 12 §3).
inline constexpr std::uint16_t DIE_BLOCK_ADDR = 0x0089;  // die ULD/ULP result block
inline constexpr std::size_t DIE_BLOCK_LEN = 17;
inline constexpr std::uint16_t L0X_BLOCK_ADDR = 0x14;    // VL53L0X, address width 1
inline constexpr std::size_t L0X_BLOCK_LEN = 12;
inline constexpr std::uint16_t HISTOGRAM_BLOCK_ADDR = 0x0088;  // Bare Driver bins
inline constexpr std::size_t HISTOGRAM_BLOCK_LEN = 83;
inline constexpr std::size_t HISTOGRAM_BINS = 24;

// One interrupt-release write the bridge plays after every block read.
struct ClearStep {
    std::uint16_t addr = 0;
    std::uint8_t value = 0;
};

// ── command encoders ────────────────────────────────────────────────────────

// Contract-10 codecs, identical on the wire: VL53_READ_REG, VL53_WRITE_REG,
// VL53_XSHUT, VL53_SET_I2C_SPEED. At address width 1 only the low byte of
// `addr` goes on the bus and addr+len must stay <= 0x100.
using vl53l4::pack_read_reg;
using vl53l4::pack_set_i2c_speed;
using vl53l4::pack_write_reg;
using vl53l4::pack_xshut;

// VL53_START_STREAM payload (6 + 3n bytes): addr u16, len u16, flags u8,
// n_clear u8, then n x {addr u16, value u8} — the interrupt-release list the
// bridge plays after every block read. nullopt for more than CLEAR_STEPS_MAX
// steps (the bridge would answer ERR_PAYLOAD_FORMAT).
std::optional<bytes> pack_start_stream(std::uint16_t addr, std::uint16_t len,
                                       const std::vector<ClearStep>& clear,
                                       std::uint8_t flags = 0);

// VL53_SET_ADDR_WIDTH payload: width u8. nullopt unless width is 1 or 2.
std::optional<bytes> pack_set_addr_width(std::uint8_t width);

// ── report decoders ─────────────────────────────────────────────────────────

// RPT_VL53_INFO (v2.00, 23 bytes, `<IIIBBBHBBI`) — bridge state only, the
// bridge reads no sensor register. Counters are free-running (wrap silently);
// slots_skipped, frames_dropped and the fault latch reset at START_STREAM.
struct Vl53lxInfo {
    std::uint32_t int_edges = 0;
    std::uint32_t slots_skipped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint8_t xshut_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;
    std::uint8_t addr_width = 0;
    std::uint8_t n_clear = 0;
    std::uint32_t frames_dropped = 0;

    // Decode a RPT_VL53_INFO payload; nullopt when shorter than INFO_SIZE.
    static std::optional<Vl53lxInfo> unpack(byte_span payload);
};

// ── product table (contract 12 §1) ──────────────────────────────────────────

// The kinds of driver ST ships for this family: `uld` (the die computes the
// distance), `ulp` (Ultra Low Power, L3CX only), `histogram` (Bare Driver:
// 24 photon bins, the host finds up to four targets).
enum class DriverKind {
    Uld,
    Ulp,
    Histogram,
};

// "uld" / "ulp" / "histogram".
std::string to_string(DriverKind k);

// One row of the product table. The bridge parameters (addr_width,
// clear_steps, max_khz) are those of the product's default driver.
struct Product {
    std::string name;             // "VL53L0X", ...
    std::uint16_t usb_pid = 0;    // production USB PID
    std::uint16_t model_id = 0;   // cross-check only (L1CX/L1CB, L4CD/L4CX share)
    std::uint32_t reach_mm = 0;   // datasheet rating of the module
    std::vector<DriverKind> driver_kinds;  // pairs that exist, UI order
    DriverKind default_driver = DriverKind::Uld;
    std::uint8_t addr_width = 2;  // register-address width, bytes
    std::vector<ClearStep> clear_steps;
    std::uint16_t max_khz = 0;    // bus ceiling after init (init runs at 400)
};

// The six products, in UI order: VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX,
// VL53L4CD, VL53L4CX.
const std::vector<Product>& products();

// The table row for an exact product name ("VL53L4CX"); nullptr if unknown.
const Product* find_product(const std::string& name);

// `ToF Sensor VL53L4CD USB v2.1` -> "VL53L4CD": the first `VL53L<digit><part>`
// match in the upper-cased name, if it is a family product; else nullopt (an
// unstamped board, or a name we do not recognise).
std::optional<std::string> product_from_board_name(const std::string& name);

// ── class resolution (contract 12 §1) ───────────────────────────────────────

// The classes a vl53lx board opens as. Vl53lx is the generic class that takes
// the product at init; VL53L4CD boards on this firmware use it too (the
// dedicated vl53l4cd class belongs to APP_VL53L4).
enum class SensorClass {
    Vl53lx,
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
};

// The reference class name: "Vl53lx", "Vl53l0x", "Vl53l1cx", ...
std::string to_string(SensorClass c);

// Resolve the class: the production USB PID model (usb_model_hint(), e.g.
// "vl53l4cx", case-insensitive) if it is a family product, else the product
// the device name carries, else the generic class.
SensorClass resolve_class(const std::optional<std::string>& usb_model,
                          const std::string& device_name);

// ── stateless block decoders (contract 12 §4) ───────────────────────────────

// Which ULD reads the die block: L4 = VL53L4CD ULD (also the L3CX ULP and
// L4CX-as-L4CD), signal at byte 5, per-SPAD K = 256; L1 = VL53L1X ULD
// (crosstalk-corrected peak signal at 0x0098 = byte 15, K = 25).
enum class DieVariant {
    L4,
    L1,
};

// Same fields as the contract-10 result (range_status via STATUS_RTN, rates
// kcps, sigma/distance mm, the sensor's frame counter).
using DieResult = vl53l4::Vl53l4Result;

// The 17-byte die block at DIE_BLOCK_ADDR as the named ULD reads it. The L4
// variant is exactly vl53l4::parse_result_block. nullopt when raw is shorter
// than DIE_BLOCK_LEN.
std::optional<DieResult> decode_die_block(byte_span raw, DieVariant variant = DieVariant::L4);

// Raw fields of the VL53L0X 12-byte block at L0X_BLOCK_ADDR. The PAL range
// status, sigma and dmax need device data cached by init (full driver).
struct L0xRaw {
    std::uint16_t distance_raw = 0;            // mm (quarter-mm if RangeFractionalEnable)
    std::uint8_t device_range_status = 0;      // raw byte 0
    std::uint32_t signal_rate_mcps_1616 = 0;   // FixPoint16.16 Mcps (wire 9.7 << 9)
    std::uint32_t ambient_rate_mcps_1616 = 0;
    std::uint16_t effective_spad_count_88 = 0; // 8.8
};

// Decode the VL53L0X block; nullopt when raw is shorter than L0X_BLOCK_LEN.
std::optional<L0xRaw> decode_l0x_raw(byte_span raw);

// Status bytes and the 24 photon bins of the 83-byte histogram block. Turning
// bins into targets (preset, VCSEL period, A/B frame pairs) is the full driver.
struct HistogramRaw {
    std::uint8_t interrupt_status = 0;
    std::uint8_t range_status = 0;
    std::uint8_t report_status = 0;
    std::uint8_t stream_count = 0;
    std::uint16_t dss_actual_effective_spads = 0;
    std::uint16_t reference_phase = 0;
    std::uint8_t vcsel_start = 0;
    std::array<std::uint32_t, HISTOGRAM_BINS> bins{};  // big-endian 24-bit counts
};

// Decode the histogram block at HISTOGRAM_BLOCK_ADDR: bin 23's low byte is
// carried in a separate MSB/LSB pair ((MSB << 2) + LSB, 8-bit) and patched in
// before the bins are read. nullopt when raw is shorter than
// HISTOGRAM_BLOCK_LEN.
std::optional<HistogramRaw> decode_histogram_raw(byte_span raw);

}  // namespace vl53lx
}  // namespace depz
