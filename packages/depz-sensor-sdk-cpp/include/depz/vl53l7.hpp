// VL53L5CX / VL53L7CX / VL53L7CH ToF decode layer
// (contracts/11_SENSOR_VL53L7.md, a delta against contracts/04_SENSOR_VL53L8.md).
//
// One firmware app (APP_VL53L7_*) serves three boards that differ only in the
// soldered sensor: VL53L5CX, VL53L7CX (base) and VL53L7CH (CX + CNH). Like the
// VL53L8 the MCU is a thin register bridge — here over I2C — and the ST ULD
// runs on the host. This header is the same *verifiable* layer the C++ SDK
// ships for VL53L8 (depz/vl53l8.hpp): the bridge wire codecs, the sensor-class
// resolution rule, and the results-frame decode for L5/L7 frames. The live ULD
// driver (firmware download, L5/L7 boot branch) is intentionally NOT ported —
// see the reference Python `vl53l8/uld.py` (`i2c` branches).
//
// Frame reassembly (FrameChunk / FrameReassembler), the shared results-frame
// layout and CNH decode (decode_cnh) are the VL53L8 ones and are reused as-is.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "depz/span.hpp"
#include "depz/vl53l8.hpp"

namespace depz {
namespace vl53l7 {

// Bridge commands. 0x32/0x33/0x35/0x36 are the VL53L8 bridge (contract 04);
// 0x34/0x37/0x38 are new on the I2C board.
enum class Vl53l7Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u16, len u16 (1..READ_MAX_LEN) -> RPT_VL53_REG_DATA
    WriteReg = 0x33,     // addr u16, data[1..WRITE_MAX_LEN]
    PinCtrl = 0x34,      // action u8 (PinAction)
    StartStream = 0x35,  // frame_size u16
    StopStream = 0x36,
    GetInfo = 0x37,      // -> RPT_VL53_INFO
    SetI2cSpeed = 0x38,  // khz u16 (snaps to the nearest I2C_SPEED_STEPS_KHZ)
};

// Bridge reports. 0x91 / 0x93 are the VL53L8 ones.
enum class Vl53l7Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA
    Info = 0x92,     // RPT_VL53_INFO (no echoed command byte)
    Frame = 0x93,    // RPT_VL53_FRAME (decode with vl53l8::FrameChunk)
};

// VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
// GPIO): after LPN_OFF or SOFT_CYCLE the host must re-run init().
enum class PinAction : std::uint8_t {
    LpnOff = 0,     // stop streaming, drive LPn low: sensor I2C interface off
    LpnOn = 1,      // drive LPn high: interface on (power-up default)
    I2cRst = 2,     // pulse I2C_RST
    SoftCycle = 3,  // stop streaming, LPn low 1 ms, high, I2C_RST; clears I2C counters
};

// RPT_VL53_INFO.last_i2c_error.
enum class I2cError : std::uint8_t {
    Ok = 0,
    Nack = 1,
    Timeout = 2,  // 250 ms deadline
    BusError = 3,
};

// Transfer ceilings. Hosts MUST split register reads at READ_MAX_LEN (the
// VL53L8 host splits at 2048 — reusing that here fails with ERR_INVALID_PARAM).
inline constexpr std::uint16_t READ_MAX_LEN = 1536;   // VL53LMZ_READ_MAX
inline constexpr std::uint16_t WRITE_MAX_LEN = 2048;  // VL53LMZ_XFER_MAX
// Max frame-data bytes carried in one RPT_VL53_FRAME chunk (VL53L8: 1528).
inline constexpr std::size_t STREAM_CHUNK_MAX = 1536;
// RPT_VL53_INFO payload size.
inline constexpr std::size_t INFO_SIZE = 20;

// L5/L7 range and stream at 1 Hz (VL53L8: >= 2 Hz).
inline constexpr int MIN_RANGING_FREQUENCY_HZ = 1;

// Nominal SCL steps the firmware carries a timing for; others snap to nearest.
inline constexpr std::uint16_t I2C_SPEED_STEPS_KHZ[] = {100, 200, 400, 500, 600,
                                                        700, 800, 900, 1000};

// Footer-id offset of L5/L7 frames: `size − 4` for both blob sets (l7cx and
// l7ch), like the VL53L8CH.
inline constexpr int FOOTER_ID_OFF = vl53l8::FOOTER_ID_OFF_CH;

// ── command encoders ────────────────────────────────────────────────────────

// VL53_READ_REG payload: addr u16, len u16 (both little-endian; VL53L8 format).
bytes pack_read_reg(std::uint16_t addr, std::uint16_t len);

// VL53_WRITE_REG payload: addr u16 then the raw register data (VL53L8 format).
bytes pack_write_reg(std::uint16_t addr, byte_span data);

// VL53_PIN_CTRL payload: action u8 (PinAction value).
bytes pack_pin_ctrl(std::uint8_t action);

// VL53_SET_I2C_SPEED payload: khz u16.
bytes pack_set_i2c_speed(std::uint16_t khz);

// ── report decoders ─────────────────────────────────────────────────────────

// RPT_VL53_INFO — bridge state only (the sensor is never probed), 20-byte
// little-endian payload `<IIIBBBHHB` with NO echoed command byte. Counters run
// from power-up / DEVICE_RESET; SOFT_CYCLE clears the I2C ones.
struct Vl53l7Info {
    std::uint32_t int_edges = 0;
    std::uint32_t frames_dropped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // I2cError
    std::uint8_t lpn_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;
    std::uint16_t frame_size = 0;
    bool streaming = false;

    // Decode a RPT_VL53_INFO payload; nullopt when shorter than INFO_SIZE.
    static std::optional<Vl53l7Info> unpack(byte_span payload);
};

// ── sensor-class resolution (contract 11 §1) ────────────────────────────────

// The three sensor classes an APP_VL53L7 board opens as.
enum class Model {
    Vl53l5cx,
    Vl53l7cx,  // base; safe default (its blob runs on every L5/L7 part)
    Vl53l7ch,  // Vl53l7cx + CNH
};

// "vl53l5cx" / "vl53l7cx" / "vl53l7ch".
std::string to_string(Model m);

// Resolve the class: the production USB PID model (usb_model_hint(): e.g.
// "vl53l7ch") first; else the first match of `VL53L([57])(CX|CH)` in the
// GET_DEVICE_NAME string (a VL53L5CH match falls back to Vl53l7cx); else
// Vl53l7cx.
Model resolve_model(const std::optional<std::string>& usb_model,
                    const std::string& device_name);

// ── frame decode ────────────────────────────────────────────────────────────

// Zone count inferred from an L5/L7 frame: the size of the zone-scaled ambient
// block (index 0x54D0, sized to the resolution by the output-list rule). nullopt
// when the frame carries no such block.
std::optional<int> infer_resolution(byte_span raw);

// Decode one raw L5/L7 results frame (the full reassembled frame bytes). Same
// layout as VL53L8 (vl53l8::decode_frame), with the L5/L7 differences applied:
// footer id at `size − 4`, and every per-zone array trimmed to `resolution`
// entries (per-target blocks carry 64 entries even in 4x4; the sensor fills the
// first `resolution` and zero-pads the rest). `resolution` is the value
// start_ranging used (RESOLUTION_4X4 / RESOLUTION_8X8); when omitted it is
// inferred with infer_resolution(). Returns nullopt for a corrupted frame
// (header/footer id mismatch).
std::optional<vl53l8::Vl53l8Frame> decode_frame(byte_span raw,
                                                std::optional<int> resolution = std::nullopt);

}  // namespace vl53l7
}  // namespace depz
