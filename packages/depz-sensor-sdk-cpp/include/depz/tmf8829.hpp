// TMF8829 multizone dToF on the APP_TMF8829 SPI register bridge
// (contracts/14_SENSOR_TMF8829.md, protocol v1.00).
//
// The MCU is a thin bridge: it reads self-sized frames off the sensor FIFO on
// INT and pushes them in chunks; the host runs the sensor (soft reset,
// firmware download, config page, presets, start / stop, frame parsing and
// clock correction). 8x8 ... 48x32 zones, up to 4 peaks per zone.
//
// This header is the *base* layer every SDK implements (contract 14 §7): the
// wire codecs, the chunk reassembler, frame header / footer parse, the
// results-pixel decode and half-frame merge, the clock model and capture time
// (bit-exact with vectors/tmf8829_frame_time.json), measurement assembly,
// presets and config-page fields, and the embedded ams sensor firmware. The
// live driver is depz::Tmf8829 in depz/device.hpp.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "depz/span.hpp"

namespace depz {
namespace tmf8829 {

// Bridge commands (0x32..0x37; 0x34 reserved, 0x30/0x31 not supported).
enum class Tmf8829Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u8, len u16 -> RPT_TMF_REG_DATA
    WriteReg = 0x33,     // addr u8, data[1..1023]
    StartStream = 0x35,  // int_mask u8, flags u8
    StopStream = 0x36,   // no payload
    GetInfo = 0x37,      // no payload -> RPT_TMF_INFO
};

enum class Tmf8829Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_TMF_REG_DATA
    Info = 0x92,     // RPT_TMF_INFO (25 B, no echoed cmd byte)
    Frame = 0x93,    // RPT_TMF_FRAME (unsolicited, chunked)
    Event = 0x94,    // RPT_TMF_EVENT (unsolicited)
};

// RPT_TMF_EVENT.code.
inline constexpr std::uint8_t EV_INT = 0;         // MOTION/PROXIMITY interrupt, no frame
inline constexpr std::uint8_t EV_BAD_HEADER = 1;  // header refused, frame not read (once on STOP mid histogram dump)

inline constexpr std::size_t READ_MAX = 1024;   // TMF_READ_REG len 1..1024
inline constexpr std::size_t WRITE_MAX = 1023;  // TMF_WRITE_REG data 1..1023
inline constexpr std::size_t CHUNK_MAX = 1024;  // frame bytes per RPT_TMF_FRAME

// INT_STATUS bits the bridge serves (TMF_START_STREAM int_mask).
inline constexpr std::uint8_t INT_RESULTS = 0x01;
inline constexpr std::uint8_t INT_MOTION = 0x02;
inline constexpr std::uint8_t INT_PROXIMITY = 0x04;
inline constexpr std::uint8_t INT_HISTOGRAMS = 0x08;
inline constexpr std::uint8_t INT_MASK_ALL = 0x0F;
// TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.
inline constexpr std::uint8_t SF_REF_FRAME = 0x01;

inline constexpr std::size_t REG_DATA_HEADER_SIZE = 9;
inline constexpr std::size_t FRAME_CHUNK_HEADER_SIZE = 21;
inline constexpr std::size_t EVENT_SIZE = 12;
inline constexpr std::size_t INFO_SIZE = 25;

// ── command encoders ────────────────────────────────────────────────────────
// TMF_STOP_STREAM and TMF_GET_INFO carry an empty payload. Each encoder
// throws std::invalid_argument before anything is sent when an argument is
// out of range.

// TMF_READ_REG payload: addr u8 (0..255), len u16 LE (1..READ_MAX).
bytes pack_read_reg(int addr, int len);

// TMF_WRITE_REG payload: addr u8 (0..255) then 1..WRITE_MAX data bytes.
bytes pack_write_reg(int addr, byte_span data);

// TMF_START_STREAM payload: int_mask (a non-zero subset of INT_MASK_ALL),
// flags (only SF_REF_FRAME).
bytes pack_start_stream(int int_mask, int flags = 0);

// ── report decoders ─────────────────────────────────────────────────────────

// RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.
struct RegData {
    std::uint64_t timestamp_us = 0;
    bytes data;

    // Decode a RPT_TMF_REG_DATA payload (9+N bytes); nullopt when too short
    // or when the echoed cmd is not 0x32.
    static std::optional<RegData> unpack(byte_span payload);
};

// RPT_TMF_INFO: bridge state only, the sensor is never touched. Carries no
// echoed command byte. `int_edges` runs from MCU power-up; the frame counters
// restart at TMF_START_STREAM.
struct Tmf8829Info {
    std::uint32_t int_edges = 0;
    std::uint32_t frames = 0;
    std::uint32_t frames_aborted = 0;
    std::uint32_t frame_errors = 0;
    std::uint32_t spi_timeouts = 0;
    std::uint8_t int_level = 0;  // INT pin now, 0 = asserted
    bool streaming = false;
    std::uint8_t int_mask = 0;
    std::uint8_t last_int_status = 0;
    std::uint8_t last_fifo_status = 0;

    // Decode a RPT_TMF_INFO payload; nullopt when shorter than INFO_SIZE.
    static std::optional<Tmf8829Info> unpack(byte_span payload);
};

// One RPT_TMF_FRAME chunk. `timestamp_us` + the frame's pre-header SYSTICK
// are a clock pair (not the capture time); `edge_us` is the INT falling edge
// credited to the frame, 0 = none.
struct FrameChunk {
    std::uint64_t timestamp_us = 0;
    std::uint16_t full_size = 0;
    std::uint16_t offset = 0;
    std::uint8_t frame_seq = 0;
    std::uint64_t edge_us = 0;
    bytes data;

    // Decode a RPT_TMF_FRAME payload (21+N bytes); nullopt when too short.
    static std::optional<FrameChunk> unpack(byte_span payload);
};

// RPT_TMF_EVENT. `int_status` is INT_STATUS as read (unmasked).
struct Tmf8829Event {
    std::uint64_t timestamp_us = 0;
    std::uint8_t int_status = 0;
    std::uint8_t code = 0;         // EV_*
    std::uint8_t frame_id = 0;     // header id byte (EV_BAD_HEADER), else 0
    std::uint8_t fifo_status = 0;  // pre-header byte (EV_BAD_HEADER), else 0

    // Decode a RPT_TMF_EVENT payload; nullopt when shorter than EVENT_SIZE.
    static std::optional<Tmf8829Event> unpack(byte_span payload);
};

// ── chunk reassembly (§2, normative) ────────────────────────────────────────

// A reassembled frame: the sensor bytes from register 0xFA on.
struct RawFrame {
    std::uint64_t timestamp_us = 0;
    std::uint64_t edge_us = 0;
    std::uint8_t frame_seq = 0;
    bytes data;
};

// Rebuilds frames from RPT_TMF_FRAME chunks: reset on offset 0 (an unfinished
// frame then counts as discarded); append only a chunk of the same frame_seq
// and full_size at the expected offset, anything else discards the frame in
// progress; complete at full_size, discarded past it. Never stitches chunks
// of different frame_seq. `seq_gaps` counts frames the bridge dropped
// (frame_seq jumps, u8 wrap).
class FrameReassembler {
public:
    // Forget everything — call at every stream start (frame_seq restarts at 0).
    void reset();
    // The frame `chunk` completes, or nullopt.
    std::optional<RawFrame> feed(const FrameChunk& chunk);

    std::uint64_t completed = 0;
    std::uint64_t discarded = 0;
    std::uint64_t seq_gaps = 0;

private:
    bool has_head_ = false;
    std::optional<std::uint8_t> next_seq_;
    std::uint64_t head_timestamp_us_ = 0, head_edge_us_ = 0;
    std::uint16_t head_full_size_ = 0;
    std::uint8_t head_seq_ = 0;
    bytes buf_;
};

// ── frames (§3) ─────────────────────────────────────────────────────────────

inline constexpr std::size_t PRE_HEADER_SIZE = 5;
inline constexpr std::size_t HEADER_SIZE = 16;
inline constexpr std::size_t FOOTER_SIZE = 12;
inline constexpr std::size_t FRAME_HEAD = PRE_HEADER_SIZE + HEADER_SIZE;  // 21 B the bridge reads @0xFA
inline constexpr std::uint16_t FRAME_EOF = 0xE0F7;

inline constexpr std::uint8_t FID_MASK = 0xF0;
inline constexpr std::uint8_t FID_RESULTS = 0x10;
inline constexpr std::uint8_t FID_HISTOGRAMS = 0x20;
inline constexpr std::uint8_t FID_REF_SPAD = 0x30;

inline constexpr std::uint8_t LAYOUT_PEAKS = 0x07;
inline constexpr std::uint8_t LAYOUT_SIGNAL = 0x08;
inline constexpr std::uint8_t LAYOUT_NOISE = 0x10;
inline constexpr std::uint8_t LAYOUT_XTALK = 0x20;
inline constexpr std::uint8_t LAYOUT_SUBFRAME = 0x40;  // 32x32/48x32: the second (odd-row) half-frame

inline constexpr std::uint8_t FRAME_VALID = 0x01;
inline constexpr std::uint8_t FRAME_ABORTED = 0xC0;
inline constexpr std::uint8_t FRAME_HV_CP_OVERLOAD = 0x08;
inline constexpr std::uint8_t FRAME_VCDRV_OVERLOAD = 0x10;
inline constexpr std::uint8_t FRAME_VCDRV_BURST_EXCEEDED = 0x20;
inline constexpr std::uint8_t FOOTER_DIST_IN_MM = 0x01;  // footer.reserved b0: mm, else 0.25 mm

inline constexpr std::uint8_t FIFO_TX_UNDERRUN = 0x10;
inline constexpr std::uint8_t FIFO_RX_OVERRUN = 0x20;

inline constexpr std::size_t MAX_PEAKS = 4;  // nr_peaks the sensor supports

// A frame whose bytes do not match its own header (or are too short).
class FrameError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

// fp_mode -> (rows, cols) of the full focal plane: 8x8, 8x8, 16x16, 32x32,
// 32x32, 32x48; nullopt for an unknown fp_mode.
std::optional<std::pair<unsigned, unsigned>> fp_grid(unsigned fp_mode);
// (rows, cols) one results frame carries: half the rows for 32x32/48x32.
std::optional<std::pair<unsigned, unsigned>> frame_rows(unsigned fp_mode);
// "8x8A", "8x8B", "16x16", "32x32", "32x32s", "48x32"; "" if unknown.
std::string fp_name(unsigned fp_mode);

// Pre-header, header and footer of one frame (any kind).
struct FrameInfo {
    std::uint8_t fifo_status = 0;
    std::uint32_t systick = 0;  // sensor SYSTICK, 125 kHz
    std::uint8_t id = 0;
    std::uint8_t layout = 0;
    std::uint16_t payload = 0;
    std::uint32_t f_number = 0;
    std::array<std::int8_t, 3> temperature{};
    std::uint8_t bdv = 0;
    std::array<std::uint16_t, 2> ref_pos{};
    std::uint32_t t0_integration = 0;  // SYSTICK at the first sub-integration
    std::uint32_t t1_integration = 0;  // ... and at the last one
    std::uint8_t frame_status = 0;
    std::uint8_t reserved = 0;
    std::uint16_t eof = 0;
    std::size_t size = 0;  // bytes of the whole frame

    // FID_RESULTS, FID_HISTOGRAMS or FID_REF_SPAD.
    std::uint8_t kind() const { return static_cast<std::uint8_t>(id & FID_MASK); }
    std::uint8_t fp_mode() const { return static_cast<std::uint8_t>(id & 0x0F); }
    bool is_second_half() const { return (layout & LAYOUT_SUBFRAME) != 0; }
    bool distance_in_mm() const { return (reserved & FOOTER_DIST_IN_MM) != 0; }
    // What is wrong with the frame, in the reference wording ("EOF 0x1234",
    // "status 0x00 not valid", "aborted", "HV_CP_OVERLOAD", ...,
    // "txfifo_underrun", "rxfifo_overrun"); empty when fine.
    std::vector<std::string> problems() const;
};

// Full frame size (pre-header included) from its first 21 bytes;
// std::invalid_argument when `head` is shorter.
std::size_t frame_size(byte_span head);
// Header and footer of a frame; FrameError when shorter than 33 bytes.
FrameInfo parse_frame(byte_span raw);

// Decoded results of one frame or of a merged measurement, raw sensor
// integers (distance in 0.25 mm, or mm with the footer flag; not yet
// clock-corrected). Per-peak arrays are [peaks][rows][cols] flattened
// (index p * rows * cols + r * cols + c); noise / xtalk are [rows][cols].
// Absent fields are nullopt: distance / snr without peaks, signal without the
// layout bit (or peaks).
struct PixelResults {
    unsigned peaks = 0;
    unsigned rows = 0, cols = 0;
    std::optional<std::vector<std::uint16_t>> distance;
    std::optional<std::vector<std::uint8_t>> snr;
    std::optional<std::vector<std::uint16_t>> signal;
    std::optional<std::vector<std::uint16_t>> noise;
    std::optional<std::vector<std::uint16_t>> xtalk;

    // (rows, cols) of the arrays present; (0, 0) when none is (the Python
    // reference's `shape`).
    std::pair<unsigned, unsigned> shape() const;
};

// Pixel results of one results frame, row-major. `info` = parse_frame(raw)
// when already parsed. FrameError: not a results frame, unknown fp_mode, or
// data size != pixels x pixel size (a corrupt frame). Like the Python
// reference it decodes any layout peak count (0..7); the sensor gives <= 4.
PixelResults parse_results(byte_span raw, const std::optional<FrameInfo>& info = std::nullopt);

// 32x32/48x32: the first half-frame holds the even rows, the second the odd
// ones; full[2i] = first[i], full[2i+1] = second[i]. A field is present when
// both halves have it. std::invalid_argument when the halves differ in peaks
// or shape.
PixelResults merge_halves(const PixelResults& first, const PixelResults& second);

// A raw histogram frame (25 377 B); decoding the bins is left to the caller
// (ams histogram layout).
struct HistogramFrame {
    FrameInfo info;
    bytes raw;

    // The bytes between header and footer.
    byte_span data() const {
        return byte_span(raw.data() + FRAME_HEAD, raw.size() - FRAME_HEAD - FOOTER_SIZE);
    }
};

// ── clock model and capture time (§5, bit-exact) ────────────────────────────

inline constexpr std::uint32_t NOMINAL_Q16 = 8u << 16;  // SYSTICK 125 kHz = 8 MCU µs per tick, Q16
inline constexpr std::int64_t CLOCK_WINDOW_MIN_US = 200000;    // first ratio estimate
inline constexpr std::int64_t CLOCK_WINDOW_SLIDE_US = 1000000;  // then a 1..2 s window
inline constexpr std::int64_t EDGE_LATE_US = 2000;

// Frame-time flags.
inline constexpr std::uint8_t FT_NOMINAL = 0x01;      // clock ratio not measured yet
inline constexpr std::uint8_t FT_CLOCK_RESET = 0x02;  // clock model restarted at this frame
inline constexpr std::uint8_t FT_EDGE_LATE = 0x04;    // INT edge > 2 ms later after t1 than usual
inline constexpr std::uint8_t FT_STALLED = 0x08;      // sub-integrations not back to back
inline constexpr std::uint8_t FT_T1_UNSURE = 0x10;    // histograms + 32x32/48x32: centre may be late

// "nominal", "clock_reset", "edge_late", "stalled", "t1_unsure" for the set bits.
std::vector<std::string> flag_names(std::uint8_t flags);

// Sub-integrations per results frame: fp_mode 0..2 -> 2, 3..4 -> 4, 5 -> 6,
// any other -> 2.
unsigned subints(unsigned fp_mode);

// Sensor SYSTICK (125 kHz) -> MCU µs, fed one clock pair per non-ref frame
// (the MCU time of the header read and the pre-header SYSTICK sampled by
// it). The ratio is taken over a sliding 1..2 s window, first after 200 ms;
// it is also the distance correction (ams tmf8829ClockCorrection). Integer
// math as the contract: u32 tick differences, floor division and floor
// shifts also for negative operands.
class SensorClock {
public:
    SensorClock() { reset(); }
    void reset();
    // Add a pair -> FT_CLOCK_RESET when it does not fit the model (which then
    // restarts from it), else 0. std::out_of_range for a negative `us` (a u64
    // MCU time, contract 07 §3).
    std::uint8_t add(std::int64_t us, std::uint32_t tick);
    // SYSTICK `tick` -> MCU µs through the pair (pair_us, pair_tick).
    std::int64_t to_us(std::int64_t pair_us, std::uint32_t pair_tick, std::uint32_t tick) const;

    std::uint32_t ratio_q16() const { return ratio_q16_; }  // MCU µs per tick, Q16
    bool nominal() const { return nominal_; }               // ratio not measured yet
    // MCU/sensor clock ratio (~1.015): distances are scaled by it.
    double ratio() const { return static_cast<double>(ratio_q16_) / NOMINAL_Q16; }

private:
    bool valid_ = false;
    std::int64_t last_us_ = 0, anchor_us_ = 0, mid_us_ = 0;
    std::uint32_t last_tick_ = 0, anchor_tick_ = 0, mid_tick_ = 0;
    bool has_mid_ = false;
    std::uint32_t ratio_q16_ = NOMINAL_Q16;
    bool nominal_ = true;
};

// Capture time of one results frame, MCU µs.
struct FrameTime {
    std::int64_t timestamp_us = 0;  // centre of the capture window
    std::uint8_t flags = 0;         // FT_*
    std::uint32_t t1_len_us = 0;    // sub-integration length used
    std::int64_t t0_us = 0;         // start of the first sub-integration
    std::int64_t t1_us = 0;         // start of the last one
    std::int64_t edge_us = 0;       // INT edge credited to the frame, 0 = none
    std::int64_t pair_us = 0;       // MCU time of the header read (the clock pair)
    std::uint32_t ratio_q16 = 0;    // clock ratio after this frame
};

// When a results frame was captured. A results frame is N equal
// sub-integrations back to back; the footer has the SYSTICK of the start of
// the first (t0) and of the last (t1). The window is [t0, t1 + T1], T1 the
// stream's shortest (t1 - t0)/(N - 1); its centre is the frame time. Reset at
// every stream start, with its clock.
class CaptureTimer {
public:
    void reset();
    // A histogram frame was seen in the stream.
    void histograms() { hist_seen_ = true; }
    // `flags` = what clock.add() returned for this frame. std::out_of_range for
    // fp_mode > 255 or a negative pair_us / edge_us.
    FrameTime results(const SensorClock& clock, unsigned fp_mode, std::uint32_t systick,
                      std::uint32_t t0_tick, std::uint32_t t1_tick, std::int64_t pair_us,
                      std::int64_t edge_us, std::uint8_t flags);
    // results() with the frame's own fp_mode, systick, t0 and t1.
    FrameTime results_frame(const SensorClock& clock, const FrameInfo& f, std::int64_t pair_us,
                            std::int64_t edge_us, std::uint8_t flags);

private:
    std::optional<std::int64_t> lag_min_;
    bool hist_seen_ = false;
    std::uint32_t sub_min_ = 0;
};

// ── measurements (§6) ───────────────────────────────────────────────────────

// One measurement: its results frame (8x8, 16x16) or both half-frames
// (32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.
// `results` keeps the raw sensor integers; `distance_mm` is clock-corrected
// mm, [peaks][rows][cols] like results.distance (empty without peaks).
// `problems` lists what went wrong since the previous measurement.
struct Measurement {
    std::int64_t timestamp_us = 0;  // capture time, MCU µs (floor mean of the half-frame captures)
    std::uint8_t flags = 0;         // FT_* of all its results frames, OR-ed
    std::uint8_t fp_mode = 0;
    std::uint32_t f_number = 0;
    std::uint32_t ratio_q16 = 0;    // clock ratio used for distance_mm
    PixelResults results;
    std::vector<double> distance_mm;
    std::vector<FrameInfo> frames;  // 1 or 2
    std::vector<FrameTime> times;   // one per frame
    // The builder keeps the frames themselves; the live layer
    // (depz::Tmf8829) delivers their bytes through on_frame() and only counts
    // them here.
    std::vector<HistogramFrame> histograms;
    std::vector<bytes> ref_frames;  // 65 B each, zero pre-header
    std::size_t n_histograms = 0;
    std::size_t n_ref_frames = 0;
    // n_problems in all; `problems` holds them (the live layer keeps the first 16).
    std::vector<std::string> problems;
    std::size_t n_problems = 0;

    // (rows, cols) of the full focal plane.
    std::pair<unsigned, unsigned> shape() const;
    unsigned peaks() const { return results.peaks; }
    // Raw distance unit is mm (8x8-xr), else 0.25 mm.
    bool distance_in_mm() const { return !frames.empty() && frames.back().distance_in_mm(); }
    // Sensor die temperature (last frame, header byte 0), °C.
    int temperature_c() const { return frames.empty() ? 0 : frames.back().temperature[0]; }
};

// Raw distances -> mm, clock-corrected (x ratio; / 4 for 0.25 mm units).
std::vector<double> distance_mm(const PixelResults& results, double ratio, bool in_mm);

// Frames in stream order -> measurements. Histogram frames between two
// results frames belong to the second; a ref-SPAD frame comes right before
// its results frame (each half-frame has its own). 32x32/48x32: a second half
// without a first is dropped with the frames collected for it; two first
// halves in a row keep the newer; halves of different fp_mode / layout (bit
// 0x40 aside) are dropped with their frames. Every case is logged in the next
// measurement's problems. Call reset() at the start of every stream: the
// clock model and T1 are per stream.
class MeasurementBuilder {
public:
    MeasurementBuilder() { reset(); }
    void reset();
    // One frame with the MCU time of its header read and its INT edge (0 =
    // none) -> the measurement it completes, or nullopt. std::out_of_range for
    // a negative timestamp_us / edge_us (u64 MCU times, contract 07 §3).
    std::optional<Measurement> add(byte_span raw, std::int64_t timestamp_us, std::int64_t edge_us = 0);

    SensorClock clock;
    CaptureTimer timer;
    std::uint64_t parse_errors = 0;

private:
    struct Half {
        FrameInfo info;
        PixelResults res;
        FrameTime time;
    };
    std::vector<std::pair<FrameInfo, bytes>> pending_;
    std::vector<HistogramFrame> hist_;
    std::vector<bytes> refs_;
    std::optional<Half> half_;
    std::vector<std::string> problems_;
};

// ── sensor registers, commands, presets, config page (§4) ───────────────────

inline constexpr std::uint8_t REG_APP_ID = 0x00;      // bootloader 0x80, application 0x01
inline constexpr std::uint8_t REG_CMD_STAT = 0x08;
inline constexpr std::uint8_t REG_SERIAL = 0x1C;      // u32
inline constexpr std::uint8_t REG_CID_RID = 0x20;
inline constexpr std::uint8_t REG_CFG = 0x22;         // config page 0x22..0xDF
inline constexpr std::uint8_t REG_INT_STATUS = 0xE1;
inline constexpr std::uint8_t REG_INT_ENAB = 0xE2;
inline constexpr std::uint8_t REG_ID = 0xE3;          // reads DEVICE_ID
inline constexpr std::uint8_t REG_REVID = 0xE4;
inline constexpr std::uint8_t REG_RESET = 0xF7;
inline constexpr std::uint8_t REG_ENABLE = 0xF8;
inline constexpr std::uint8_t REG_FIFOSTATUS = 0xFA;
inline constexpr std::uint8_t REG_FIFO = 0xFF;        // the address stays here
inline constexpr std::uint8_t DEVICE_ID = 0x9E;
inline constexpr std::size_t CFG_SIZE = 190;          // 0xDF - 0x22 + 1

// Application commands (written to CMD_STAT) and statuses.
inline constexpr std::uint8_t CMD_MEASURE = 0x10;
inline constexpr std::uint8_t CMD_WRITE_PAGE = 0x15;
inline constexpr std::uint8_t CMD_LOAD_CONFIG_PAGE = 0x16;
inline constexpr std::uint8_t CMD_STOP = 0xFF;
inline constexpr std::uint8_t STAT_OK = 0x00;
inline constexpr std::uint8_t STAT_ACCEPTED = 0x01;
// "OK", "ACCEPTED", "ERR_CONFIG", ..., "ERR_OSC_TUNE"; "" for an unknown status.
std::string status_name(std::uint8_t status);

// A preset: its load command (then LOAD_CONFIG_PAGE) and its own
// kilo_iterations (a preset rescales a stale value, it does not reset it).
struct Preset {
    const char* name;  // "8x8", "8x8-lr", ..., "8x8-xr"
    std::uint8_t cmd;  // 0x40..0x49
    std::uint16_t kilo_iterations;
};

// 8x8 0x40, 8x8-lr 0x41 (long range), 8x8-ha 0x42 (high accuracy), 16x16
// 0x43, 16x16-ha 0x44, 32x32 0x45, 32x32-ha 0x46, 48x32 0x47, 48x32-ha 0x48,
// 8x8-xr 0x49 (extended range, distances in mm).
inline constexpr std::array<Preset, 10> PRESETS = {{
    {"8x8", 0x40, 586},
    {"8x8-lr", 0x41, 586},
    {"8x8-ha", 0x42, 586},
    {"16x16", 0x43, 586},
    {"16x16-ha", 0x44, 586},
    {"32x32", 0x45, 2344},
    {"32x32-ha", 0x46, 2344},
    {"48x32", 0x47, 3516},
    {"48x32-ha", 0x48, 3516},
    {"8x8-xr", 0x49, 586},
}};

// The preset called `name`, or nullopt.
std::optional<Preset> find_preset(const std::string& name);

inline constexpr int DEFAULT_PERIOD_MS = 33;  // the sensor's own value after init()

// A config-page field: register, bit shift, width (16 = little-endian pair).
struct CfgField {
    const char* name;
    std::uint8_t reg;
    std::uint8_t shift;
    std::uint8_t width;
};

inline constexpr std::array<CfgField, 14> CFG_FIELDS = {{
    {"period_ms", 0x22, 0, 16},
    {"kilo_iterations", 0x24, 0, 16},
    {"fp_mode", 0x26, 0, 3},
    {"nr_peaks", 0x2A, 0, 3},
    {"signal_strength", 0x2A, 3, 1},
    {"noise_strength", 0x2A, 4, 1},
    {"xtalk", 0x2A, 5, 1},
    {"dump_histograms", 0x2B, 0, 1},
    {"ref_spad_frame", 0x2C, 0, 1},
    {"power_modes", 0x2E, 0, 8},
    {"ha_kilo_iterations", 0x4A, 0, 16},
    {"dual_mode", 0x4C, 0, 2},
    {"fov_correction", 0x78, 0, 4},
    {"distance_in_mm", 0x8F, 0, 1},
}};

// The field called `name`, or nullopt.
std::optional<CfgField> find_cfg_field(const std::string& name);
// Read a field of a config page (CFG_SIZE bytes, or a partial read from its
// start that holds the field). std::out_of_range for an unknown name,
// std::invalid_argument for a page too short to hold the field.
std::uint32_t cfg_get(byte_span page, const std::string& name);
// Set a field in place. std::out_of_range for an unknown name,
// std::invalid_argument for a value outside 0 .. 2^width - 1 or a page too
// short to hold the field (page untouched either way).
void cfg_set(bytes& page, const std::string& name, std::int64_t value);
// Every named field of a config page, in CFG_FIELDS order (std::invalid_argument
// when the page is too short for one of them).
std::vector<std::pair<std::string, std::uint32_t>> config_fields(byte_span page);

// ── sensor firmware ─────────────────────────────────────────────────────────

// Where the image goes in the sensor (BL W_FIFO_BOTH address).
inline constexpr std::uint32_t FIRMWARE_LOAD_ADDR = 0x00010000;
// ams tmf8829_application.hex as bytes (application 2.200.0, MIT — see
// licenses/tmf8829/LICENSES-MIT.TXT shipped with this SDK): 14 444 B, a
// multiple of 4, sha256 973c89b7...338a20f8.
byte_span firmware_image();

}  // namespace tmf8829
}  // namespace depz
