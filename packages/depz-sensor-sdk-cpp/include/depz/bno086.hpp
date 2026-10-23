// BNO086 SHTP framing + SH-2 report decode (contracts/05_SENSOR_BNO086.md).
//
// Pure codecs — no I/O. Raw wire integers are authoritative; scaled floats are
// left to a higher layer (Q points documented per report). Byte-exact with the
// Python/TS reference implementations (shtp.py / reports.py / sh2.py).
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "depz/span.hpp"

namespace depz {
namespace bno086 {

// ── SHTP framing (§3) ───────────────────────────────────────────────────────

inline constexpr std::size_t SHTP_HEADER_SIZE = 4;
inline constexpr std::uint16_t LENGTH_MASK = 0x7FFF;
inline constexpr std::uint16_t CONTINUATION_BIT = 0x8000;
inline constexpr int NUM_CHANNELS = 6;
inline constexpr std::size_t MAX_TX_FRAME = 64;  // ERRATA E2 MCU slot

enum class ShtpChannel : std::uint8_t {
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
};

// One-byte host writes on the executable channel (contract 05 §6a); the hub
// answers none of them. Reset restarts the hub (the 0x01 reset-complete
// follows on the same channel); Sleep stops every report except those enabled
// with FEATURE_WAKEUP | FEATURE_ALWAYS_ON, keeping the configuration; On ends
// Sleep and every report resumes at its rate. Bno086::sleep() / power_on().
enum class ExecutableCommand : std::uint8_t {
    Reset = 1,
    On = 2,
    Sleep = 3,
};

struct ShtpHeader {
    std::uint16_t length = 0;  // bits 14:0 — cargo length incl. this 4-byte header
    std::uint8_t channel = 0;
    std::uint8_t seq = 0;
    bool continuation = false;

    // std::invalid_argument when length > 0x7FFF (never masked, contract 07 §3).
    bytes pack() const;
    static ShtpHeader unpack(byte_span data);
};

// One reassembled cargo: `payload` excludes all SHTP headers.
struct ShtpCargo {
    int channel = 0;
    int seq = 0;  // seq of the first fragment
    bytes payload;
    std::uint64_t capture_us = 0;  // capture time of the first fragment (contract 05 §3)
};

// Single-fragment frame: length = header + payload.
bytes shtp_build_frame(int channel, byte_span payload, std::uint8_t seq);

// Split a cargo into wire frames of at most `max_frame` bytes. First fragment
// advertises the TOTAL cargo length; continuations carry the remaining length
// with the continuation bit set; seq increments per frame.
std::vector<bytes> shtp_fragment_cargo(int channel, byte_span payload, std::uint8_t seq_start,
                                        std::size_t max_frame = MAX_TX_FRAME);

// Per-channel TX sequence counters + RX cargo reassembly (§3).
class ShtpLayer {
public:
    // Build a single-fragment frame, consuming the channel's TX seq.
    // std::out_of_range for a channel outside 0..5, std::invalid_argument for
    // a payload past the 64-byte MCU slot.
    bytes next_frame(int channel, byte_span payload);
    // std::out_of_range for a channel outside 0..5.
    int tx_seq(int channel) const {
        if (channel < 0 || channel >= NUM_CHANNELS) throw std::out_of_range("SHTP channel out of range 0..5");
        return tx_seq_[channel];
    }

    // Consume one inbound frame; return the cargo when complete, else nullopt.
    // `capture_us` is the frame's RPT_DATA capture time; the cargo carries the
    // FIRST fragment's — the base its 0xFB timestamps refer to (§3, §5).
    std::optional<ShtpCargo> feed(byte_span frame, std::uint64_t capture_us = 0);

    // Forget all TX seq counters and partial cargos (sensor reset).
    void reset();

    std::size_t discarded = 0;

private:
    struct ChannelRx {
        bytes buf;
        std::size_t expected = 0;  // total cargo payload bytes (headers excluded)
        int seq = 0;
        std::uint64_t capture_us = 0;
    };
    int tx_seq_[NUM_CHANNELS] = {0};
    ChannelRx rx_[NUM_CHANNELS];
};

// ── SH-2 control-channel encoders (§6) ──────────────────────────────────────

// Set Feature Command (0xFD), 17 bytes. interval_us = 0 disables the sensor.
bytes sh2_build_set_feature(std::uint8_t sensor_id, std::uint32_t interval_us,
                            std::uint32_t batch_us = 0, std::uint16_t sensitivity = 0,
                            std::uint8_t flags = 0, std::uint32_t cfg_word = 0);
// Get Feature Request (0xFE), 2 bytes.
bytes sh2_build_get_feature_request(std::uint8_t sensor_id);
// Product ID Request (0xF9), 2 bytes.
bytes sh2_build_product_id_request();
// Command Request (0xF2), 12 bytes: id, seq, command, P0..P8 (params, <=9 bytes).
bytes sh2_build_command_request(std::uint8_t seq, std::uint8_t command, byte_span params = {});
// FRS Read Request (0xF4), 8 bytes. block_words = 0 reads the record.
bytes sh2_build_frs_read_request(std::uint16_t frs_type, std::uint16_t offset_words = 0,
                                 std::uint16_t block_words = 0);
// FRS Write Request (0xF7), 6 bytes. length_words = 0 erases the record.
bytes sh2_build_frs_write_request(std::uint16_t frs_type, std::uint16_t length_words);
// FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.
bytes sh2_build_frs_write_data(std::uint16_t offset_words, const std::vector<std::uint32_t>& words);

// ── SH-2 input reports (§5) ─────────────────────────────────────────────────

// In-cargo control IDs on the input channels.
inline constexpr std::uint8_t BASE_TIMESTAMP_REF = 0xFB;
inline constexpr std::uint8_t TIMESTAMP_REBASE = 0xFA;
// + u8 sensor id, no SH-2 header: the answer to Force Sensor Flush (0xF0).
inline constexpr std::uint8_t FLUSH_COMPLETED = 0xEF;

inline constexpr int RV_ACCURACY_Q = 12;
inline constexpr int GYRO_RV_ANGVEL_Q = 10;

enum class ReportType {
    Acceleration,
    Gyroscope,
    Magnetometer,
    UncalibratedGyroscope,
    UncalibratedMagnetometer,
    RotationVector,
    ScalarReport,
    TapDetector,
    StepCounter,
    StepDetector,
    SignificantMotion,
    StabilityClassifier,
    ShakeDetector,
    GenericEvent,
    PersonalActivityClassifier,
    RawSensor,
    GyroIntegratedRV,
    UnknownReport,
    MotionRequest,   // 0x2B, BNO086 interactive calibration: intent, request
    FlushCompleted,  // 0xEF: flushed_sensor; no seq / accuracy / delay
};

// Flat tagged report. Only the fields relevant to `type` are populated; the
// rest keep their zero defaults. Raw integers are authoritative.
struct Report {
    ReportType type = ReportType::UnknownReport;
    int sensor_id = 0;
    std::int64_t timestamp_us = 0;

    // common input-report header (channels 3/4)
    int seq = 0;
    int accuracy = 0;
    std::int64_t delay_us = 0;

    // vector / quaternion components
    std::int64_t x_raw = 0, y_raw = 0, z_raw = 0;
    std::int64_t bias_x_raw = 0, bias_y_raw = 0, bias_z_raw = 0;
    std::int64_t i_raw = 0, j_raw = 0, k_raw = 0, real_raw = 0;
    bool has_accuracy_raw = false;
    std::int64_t accuracy_raw = 0;

    // scalar / detector fields
    std::int64_t value_raw = 0;
    int flags = 0;
    std::int64_t latency_us = 0;
    int steps = 0;
    int motion = 0;
    int classification = 0;

    // personal activity classifier
    int page_number = 0;
    bool end_of_sequence = false;
    int most_likely_state = 0;
    std::vector<int> confidences;

    // motion request (0x2B): the motion intent the hub was last told and what
    // it asks of the device (SH-2 §6.5.45.2; 1, 3, 4 = stay still)
    int intent = 0;
    int request = 0;

    // flush completed (0xEF): the sensor whose batched reports are all sent
    int flushed_sensor = 0;

    // raw sensor
    std::int64_t sensor_timestamp_us = 0;
    std::int64_t temperature_raw = 0;

    // gyro-integrated RV angular velocity
    std::int64_t vx_raw = 0, vy_raw = 0, vz_raw = 0;

    // unknown report tail bytes (from the ID to end of cargo)
    bytes data;
};

// Parse a channel-3/4 cargo into typed reports. Handles 0xFB base timestamp
// references and 0xFA rebases; every report's timestamp is base + delay where
// base = capture − base_delta·100 µs. A bare 0xEF Flush Completed (2 bytes, no
// SH-2 header) is stamped with the current base. A negative capture_timestamp_us (the MCU
// uptime) is std::invalid_argument.
std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us);

// Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes: 7×i16
// bare, or prefixed with 0xFB + i32 base delta + u16 delay (100 µs ticks). A
// negative capture_timestamp_us is std::invalid_argument.
std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us);

}  // namespace bno086
}  // namespace depz
