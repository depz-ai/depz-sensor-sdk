// VL53L8CX/CH ToF decode layer (contracts/04_SENSOR_VL53L8.md).
//
// Serves BOTH DEPZ ToF sensors: the VL53L8CX (base ToF die, ST ULD 2.1.0,
// dev-default part) and the VL53L8CH (same ToF plus CNH histograms, production
// USB PID 0xED40). They stream the *same* results-frame layout, so one decoder
// covers both — the only variant-specific difference here is the footer-id
// offset (see Variant / decode_frame below).
//
// This is the *verifiable* decode layer only: frame-chunk reassembly, the
// shared results-frame decoder (raw per-zone arrays), and the advanced-feature
// DCI codecs (xtalk margin, detection thresholds, motion indicator), and the
// CH-only CNH histogram decode (decode_cnh, at the bottom of this header). It is
// pure codecs, no I/O. The live ULD driver (firmware download, DCI read/write
// handshakes, power-mode register sequences) is the live class depz::Vl53l8 in
// depz/device.hpp, which runs the C SDK's port of the reference Python
// `vl53l8/uld.py` and decodes its frames with these structs.
//
// Semantics are byte-exact with the Python/TS reference implementations; frame
// values come out in ST GetRangingData's real units (contract 04 §5, see
// Vl53l8Frame).
#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "depz/span.hpp"

namespace depz {
namespace vl53l8 {

inline constexpr int RESOLUTION_4X4 = 16;
inline constexpr int RESOLUTION_8X8 = 64;

// Max frame-data bytes carried in one RPT_VL53_FRAME chunk (contract 04).
inline constexpr std::size_t STREAM_CHUNK_MAX = 1528;
// Largest frame_size VL53_START_STREAM accepts (contract 04 §2).
inline constexpr std::size_t STREAM_TOTAL_MAX = 8192;

// VL53_START_STREAM (0x35) payload: frame_size u16 LE — the host ULD's
// data_read_size (contract 04 §2). Byte-exact with the reference
// pack_start_stream; frame_size outside 1..8192 throws std::invalid_argument
// (contract 04 §4 rule 4 — the firmware refuses it too).
bytes pack_start_stream(std::uint16_t frame_size);

// The two DEPZ ToF die variants. Both stream the same results-frame layout
// decoded by decode_frame(); they differ in the footer-id offset here, and the
// CH additionally emits CNH histograms (decode_cnh, at the bottom).
enum class Variant {
    CX,  // base ToF (ULD 2.1.0); dev-default, no dedicated production USB PID
    CH,  // CX + CNH histograms; production USB PID 0xED40 (VL53LMZ 2.0.16)
};

// Footer-id offset used by decode_frame's header/footer match check, per
// variant: 12 for VL53L8CX (ULD 2.1.0), 4 for VL53L8CH (VL53LMZ 2.0.16).
inline constexpr int FOOTER_ID_OFF_CX = 12;
inline constexpr int FOOTER_ID_OFF_CH = 4;

// Results-block index of the CNH data output block (VL53LMZ plugin_cnh).
inline constexpr std::uint32_t CNH_DATA_IDX = 0xC048;

// Footer-id offset for a variant (convenience for decode_frame's argument).
inline constexpr int footer_id_off(Variant v) {
    return v == Variant::CH ? FOOTER_ID_OFF_CH : FOOTER_ID_OFF_CX;
}

// ── frame reassembly ────────────────────────────────────────────────────────

// One RPT_VL53_FRAME payload: capture timestamp + this chunk's slice of the
// full frame. Header is ts(u64) full_size(u16) offset(u16), then data.
struct FrameChunk {
    std::uint64_t timestamp_us = 0;
    std::uint16_t full_size = 0;
    std::uint16_t offset = 0;
    bytes data;

    static FrameChunk unpack(byte_span payload);
};

// Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports. Rules
// (contract 04 §4): reset on offset==0; chunks must be contiguous — a gap
// discards the frame in progress; a frame completes when the accumulated bytes
// equal full_size. A chunk out of the rule-4 bounds (more than chunk_max bytes,
// full_size over STREAM_TOTAL_MAX, or running past full_size) is damaged:
// dropped and counted as a lost frame like the other damaged input, never
// thrown. chunk_max is the device's VL53_STREAM_FRAME_MAX
// (vl53l7::STREAM_CHUNK_MAX = 1536 on the I2C board, contract 11 §3).
class FrameReassembler {
public:
    explicit FrameReassembler(std::size_t chunk_max = STREAM_CHUNK_MAX) : chunk_max_(chunk_max) {}

    // Returns (timestamp_us, frame_bytes) when a frame completes, else nullopt.
    std::optional<std::pair<std::uint64_t, bytes>> feed(const FrameChunk& chunk);

    std::size_t completed = 0;
    std::size_t discarded = 0;

private:
    std::size_t chunk_max_;
    bytes buf_;
    std::uint16_t full_size_ = 0;
    std::uint64_t timestamp_us_ = 0;
};

// ── frame decode ────────────────────────────────────────────────────────────

// One decoded ranging frame. Arrays are sized to the active resolution (16 or
// 64 zones), row-major, in the real units ST's GetRangingData gives (contract
// 04 §5): `distance_mm` = wire value / 4 floored, a negative result clamped to
// 0; signal / ambient = wire / 2048 (kcps/SPAD); `range_sigma_mm` = wire / 128
// (mm); `reflectance` = wire / 2 (%). ST divides integers; the double fields
// keep the fraction.
struct Vl53l8Frame {
    std::uint64_t timestamp_us = 0;
    int resolution = 0;
    int silicon_temp_degc = 0;
    std::vector<std::int32_t> distance_mm;       // mm, >= 0
    std::vector<std::uint8_t> target_status;
    std::vector<std::uint8_t> nb_target_detected;
    std::vector<double> signal_per_spad;         // kcps/SPAD
    std::vector<double> ambient_per_spad;        // kcps/SPAD
    std::vector<std::uint32_t> nb_spads_enabled;
    std::vector<double> range_sigma_mm;          // mm
    std::vector<double> reflectance;             // %
    // Raw CNH data block (index CNH_DATA_IDX, byte-swapped like every results
    // block) when the frame carries one — VL53L8CH / VL53L7CH with CNH
    // configured; nullopt otherwise. Feed it to decode_cnh().
    std::optional<bytes> cnh_raw;
};

// VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail bytes unchanged).
bytes swap_buffer(byte_span data);

// Decode one raw results frame (the full reassembled frame bytes). Shared by
// both CX and CH — the layout is identical. Resolution is inferred from the
// frame's block layout. Returns nullopt for a corrupted frame (header/footer id
// mismatch). `footer_id_off` is variant-specific (FOOTER_ID_OFF_CX /
// FOOTER_ID_OFF_CH, or footer_id_off(Variant)); it defaults to the CX offset.
std::optional<Vl53l8Frame> decode_frame(byte_span raw,
                                        int footer_id_off = FOOTER_ID_OFF_CX);

// ── advanced-feature DCI codecs (ST ULD, BSD-3) ─────────────────────────────

// Largest xtalk margin set_xtalk_margin accepts, kcps/SPAD (as ST).
constexpr std::uint32_t XTALK_MARGIN_MAX_KCPS = 10000;

// Xtalk margin, a whole number of kcps/SPAD (as ST) <-> DCI_XTALK_CFG raw word
// (raw = kcps*2048). xtalk_margin_raw: std::out_of_range above
// XTALK_MARGIN_MAX_KCPS. xtalk_margin_kcps: raw / 2048 (integer division).
std::uint32_t xtalk_margin_raw(std::uint32_t margin_kcps);
std::uint32_t xtalk_margin_kcps(std::uint32_t raw);

// Detection-threshold measurement selectors (plugin source).
enum ThreshMeasurement : std::uint8_t {
    THRESH_DIST_MM = 1,
    THRESH_SIGNAL_PER_SPAD_KCPS = 2,
    THRESH_RANGE_SIGMA_MM = 4,
    THRESH_AMBIENT_PER_SPAD_KCPS = 8,
    THRESH_NB_TARGET_DETECTED = 9,
    THRESH_TAR_STATUS = 12,
    THRESH_NB_SPADS_ENABLED = 13,
    THRESH_MOTION_INDICATOR = 19,
};

struct DetectionThreshold {
    std::int32_t low_thresh = 0;   // real units; scaled on pack
    std::int32_t high_thresh = 0;  // real units; scaled on pack
    std::uint8_t measurement = 0;
    std::uint8_t type = 0;       // window selector
    std::uint8_t zone_num = 0;
    std::uint8_t operation = 0;  // combine op
};

inline constexpr int NB_THRESHOLDS = 64;

// Pack the 64-entry DCI_DET_THRESH_START block (768 bytes). Missing entries
// default to zeros; low/high are multiplied by the measurement's scale factor.
// std::invalid_argument for more than 64 entries, std::out_of_range when a
// scaled low/high does not fit the i32 field (contract 07 §3).
bytes pack_detection_thresholds(const std::vector<DetectionThreshold>& thresholds);

// The 8-byte DCI_DET_THRESH_VALID_STATUS write that accompanies the block.
bytes detection_thresholds_valid_status();

// Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).
struct MotionConfig {
    std::int32_t ref_bin_offset = 0;
    std::uint32_t detection_threshold = 0;
    std::uint32_t extra_noise_sigma = 0;
    std::uint32_t null_den_clip_value = 0;
    std::uint8_t mem_update_mode = 0;
    std::uint8_t mem_update_choice = 0;
    std::uint8_t sum_span = 0;
    std::uint8_t feature_length = 0;
    std::uint8_t nb_of_aggregates = 0;
    std::uint8_t nb_of_temporal_accumulations = 0;
    std::uint8_t min_nb_for_global_detection = 0;
    std::uint8_t global_indicator_format_1 = 0;
    std::uint8_t global_indicator_format_2 = 0;
    std::uint8_t spare_1 = 0;
    std::uint8_t spare_2 = 0;
    std::uint8_t spare_3 = 0;
    std::int8_t map_id[64] = {0};
    std::uint8_t indicator_format_1[32] = {0};
    std::uint8_t indicator_format_2[32] = {0};

    bytes pack() const;
};

// Build the default motion-indicator configuration for `resolution`
// (RESOLUTION_4X4 or RESOLUTION_8X8), matching uld.motion_indicator_init.
// std::invalid_argument for any other resolution.
MotionConfig motion_config_init(int resolution);

// ── CNH (Compact Network Histogram) decode — VL53L8CH ───────────────────────
//
// The VL53L8CH adds a compact-network-histogram (CNH) stream that the base CX
// does not produce: a per-aggregate distance histogram captured in poll-mode
// (chunked READ_REG) on top of the normal ranging frame. This is a 1:1 port of
// the decode half of the reference Python `vl53l8/cnh.py` (itself a port of
// vl53lmz_plugin_cnh.c / vl53lmz_plugin_motion_indicator.c, VL53LMZ ULD 2.0.16).
//
// The decode assumes the fixed cnh_cfg used by Example_12_cnh_data.c — ping-pong
// and variance disabled (so the device reports a single buffer). The offset math
// below reproduces _cnh_get_mem_block_addresses exactly; do not "simplify" it.

// Persistent-data header / buffer layout constants (plugin_cnh.c).
// decode_cnh() bounds (same limits as the C SDK's DEPZ_VL53L8_CNH_MAX_*).
inline constexpr int CNH_MAX_AGGREGATES = 64;
inline constexpr int CNH_MAX_FEATURE_LENGTH = 255;
inline constexpr int CNH_PER_HEADER_WORDS = 5;          // CNH_PER_HEADER_BYTES / 4
inline constexpr int CNH_PER_BUFFER_HEADER_WORDS = 2;   // CNH_PER_BUFFER_HEADER_BYTES / 4
inline constexpr int CNH_PER_HEADER_BUFFER_INFO_IDX = 1;
inline constexpr int CNH_PER_HEADER_FLAGS_IDX = 3;
inline constexpr std::uint32_t CNH_BUFFER_INFO_WORDS_MASK = 0xFFFF;
inline constexpr int CNH_MI_STATE_PING = 0;

// One decoded CNH aggregate. `hist_raw[i] / 2**hist_scaler[i]` is the float bin
// value; `ambient = ambient_raw / 2**ambient_scaler`. hist_raw / hist_scaler are
// length == feature_length.
struct CnhAggregate {
    std::vector<std::int32_t> hist_raw;
    std::vector<std::int8_t> hist_scaler;
    std::vector<double> hist;
    std::int32_t ambient_raw = 0;
    std::int8_t ambient_scaler = 0;
    double ambient = 0.0;
};

// A decoded CNH data block. `ref_residual_word` is the raw u32 at byte offset 8;
// `ref_residual = ref_residual_word / 2048.0`.
struct CnhFrame {
    std::uint32_t ref_residual_word = 0;
    double ref_residual = 0.0;
    std::vector<CnhAggregate> aggregates;  // len == nb_of_aggregates
};

// Decode a captured CNH data block (`raw`, byte-swapped exactly like the standard
// ranging blocks) into per-aggregate histograms. `nb_of_aggregates` and
// `feature_length` come from the CNH config used on the device (see CnhConfig /
// MotionConfig). Faithful port of cnh.decode / _decode_aggregate for the fixed
// cnh_cfg (ping-pong + variance disabled).
// Throws std::invalid_argument for an out-of-range config and
// std::length_error for a block shorter than the config implies.
CnhFrame decode_cnh(int nb_of_aggregates, int feature_length, byte_span raw);

}  // namespace vl53l8
}  // namespace depz
