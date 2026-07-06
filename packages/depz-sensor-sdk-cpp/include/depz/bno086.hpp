// BNO086 SHTP framing + SH-2 report decode (contracts/05_SENSOR_BNO086.md).
//
// Pure codecs — no I/O. Raw wire integers are authoritative; scaled floats are
// left to a higher layer (Q points documented per report). Byte-exact with the
// Python/TS reference implementations (shtp.py / reports.py / sh2.py).
#pragma once

#include <cstdint>
#include <optional>
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

struct ShtpHeader {
    std::uint16_t length = 0;  // bits 14:0 — cargo length incl. this 4-byte header
    std::uint8_t channel = 0;
    std::uint8_t seq = 0;
    bool continuation = false;

    bytes pack() const;
    static ShtpHeader unpack(byte_span data);
};

// One reassembled cargo: `payload` excludes all SHTP headers.
struct ShtpCargo {
    int channel = 0;
    int seq = 0;  // seq of the first fragment
    bytes payload;
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
    bytes next_frame(int channel, byte_span payload);
    int tx_seq(int channel) const { return tx_seq_[channel]; }

    // Consume one inbound frame; return the cargo when complete, else nullopt.
    std::optional<ShtpCargo> feed(byte_span frame);

    // Forget all TX seq counters and partial cargos (sensor reset).
    void reset();

    std::size_t discarded = 0;

private:
    struct ChannelRx {
        bytes buf;
        std::size_t expected = 0;  // total cargo payload bytes (headers excluded)
        int seq = 0;
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
// base = capture − base_delta·100 µs.
std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us);

// Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes: 7×i16
// bare, or prefixed with 0xFB + i32 base delta + u16 delay (100 µs ticks).
std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us);

}  // namespace bno086
}  // namespace depz
