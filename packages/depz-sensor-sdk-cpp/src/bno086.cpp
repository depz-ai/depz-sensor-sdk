#include "depz/bno086.hpp"

#include <stdexcept>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace bno086 {

using detail::rd_u16;
using detail::rd_u32;
using detail::u8;

// ── SHTP framing ────────────────────────────────────────────────────────────

bytes ShtpHeader::pack() const {
    std::uint16_t word = (length & LENGTH_MASK) | (continuation ? CONTINUATION_BIT : 0);
    bytes out;
    detail::wr_le(out, word, 2);
    out.push_back(std::byte{channel});
    out.push_back(std::byte{seq});
    return out;
}

ShtpHeader ShtpHeader::unpack(byte_span data) {
    detail::require(data, SHTP_HEADER_SIZE, "ShtpHeader: short");
    std::uint16_t word = rd_u16(data, 0);
    ShtpHeader h;
    h.length = word & LENGTH_MASK;
    h.channel = u8(data[2]);
    h.seq = u8(data[3]);
    h.continuation = (word & CONTINUATION_BIT) != 0;
    return h;
}

bytes shtp_build_frame(int channel, byte_span payload, std::uint8_t seq) {
    ShtpHeader h;
    h.length = static_cast<std::uint16_t>(SHTP_HEADER_SIZE + payload.size());
    h.channel = static_cast<std::uint8_t>(channel);
    h.seq = seq;
    bytes out = h.pack();
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<bytes> shtp_fragment_cargo(int channel, byte_span payload, std::uint8_t seq_start,
                                       std::size_t max_frame) {
    if (max_frame <= SHTP_HEADER_SIZE)
        throw std::invalid_argument("max_frame must exceed the 4-byte SHTP header");
    std::size_t room = max_frame - SHTP_HEADER_SIZE;
    std::vector<bytes> frames;
    std::size_t off = 0;
    std::uint8_t seq = seq_start;
    std::size_t total = SHTP_HEADER_SIZE + payload.size();
    while (true) {
        std::size_t chunk_len = std::min(room, payload.size() - off);
        std::size_t remaining = total - off;  // includes one header
        ShtpHeader h;
        h.length = static_cast<std::uint16_t>(remaining & LENGTH_MASK);
        h.channel = static_cast<std::uint8_t>(channel);
        h.seq = seq;
        h.continuation = off > 0;
        bytes frame = h.pack();
        frame.insert(frame.end(), payload.begin() + off, payload.begin() + off + chunk_len);
        frames.push_back(std::move(frame));
        off += chunk_len;
        seq = static_cast<std::uint8_t>(seq + 1);
        if (off >= payload.size()) return frames;
    }
}

bytes ShtpLayer::next_frame(int channel, byte_span payload) {
    if (SHTP_HEADER_SIZE + payload.size() > MAX_TX_FRAME)
        throw std::invalid_argument("TX cargo exceeds the MCU slot");
    int seq = tx_seq_[channel];
    tx_seq_[channel] = (seq + 1) & 0xFF;
    return shtp_build_frame(channel, payload, static_cast<std::uint8_t>(seq));
}

std::optional<ShtpCargo> ShtpLayer::feed(byte_span frame) {
    if (frame.size() < SHTP_HEADER_SIZE) return std::nullopt;
    ShtpHeader hdr = ShtpHeader::unpack(frame);
    if (hdr.channel >= NUM_CHANNELS || hdr.length < SHTP_HEADER_SIZE) return std::nullopt;
    byte_span chunk = frame.subspan(SHTP_HEADER_SIZE);
    ChannelRx& rx = rx_[hdr.channel];
    if (!hdr.continuation) {
        if (rx.expected && !rx.buf.empty()) ++discarded;
        rx.buf.assign(chunk.begin(), chunk.end());
        rx.expected = hdr.length - SHTP_HEADER_SIZE;
        rx.seq = hdr.seq;
    } else {
        if (!rx.expected) {
            ++discarded;
            return std::nullopt;
        }
        rx.buf.insert(rx.buf.end(), chunk.begin(), chunk.end());
    }
    if (rx.buf.size() < rx.expected) return std::nullopt;
    if (rx.buf.size() > rx.expected) {  // overrun — junk framing
        ++discarded;
        rx.buf.clear();
        rx.expected = 0;
        return std::nullopt;
    }
    ShtpCargo cargo;
    cargo.channel = hdr.channel;
    cargo.seq = rx.seq;
    cargo.payload = std::move(rx.buf);
    rx.buf.clear();
    rx.expected = 0;
    return cargo;
}

void ShtpLayer::reset() {
    for (int c = 0; c < NUM_CHANNELS; ++c) {
        tx_seq_[c] = 0;
        rx_[c] = ChannelRx{};
    }
}

// ── SH-2 control encoders ───────────────────────────────────────────────────

bytes sh2_build_set_feature(std::uint8_t sensor_id, std::uint32_t interval_us,
                            std::uint32_t batch_us, std::uint16_t sensitivity,
                            std::uint8_t flags, std::uint32_t cfg_word) {
    bytes out;
    out.push_back(std::byte{0xFD});
    out.push_back(std::byte{sensor_id});
    out.push_back(std::byte{flags});
    detail::wr_le(out, sensitivity, 2);
    detail::wr_le(out, interval_us, 4);
    detail::wr_le(out, batch_us, 4);
    detail::wr_le(out, cfg_word, 4);
    return out;
}

bytes sh2_build_get_feature_request(std::uint8_t sensor_id) {
    return bytes{std::byte{0xFE}, std::byte{sensor_id}};
}

bytes sh2_build_product_id_request() { return bytes{std::byte{0xF9}, std::byte{0x00}}; }

bytes sh2_build_command_request(std::uint8_t seq, std::uint8_t command, byte_span params) {
    if (params.size() > 9)
        throw std::invalid_argument("command request carries at most 9 parameter bytes");
    bytes out;
    out.push_back(std::byte{0xF2});
    out.push_back(std::byte{seq});
    out.push_back(std::byte{command});
    for (std::byte b : params) out.push_back(b);
    while (out.size() < 12) out.push_back(std::byte{0x00});
    return out;
}

bytes sh2_build_frs_read_request(std::uint16_t frs_type, std::uint16_t offset_words,
                                 std::uint16_t block_words) {
    bytes out;
    out.push_back(std::byte{0xF4});
    out.push_back(std::byte{0x00});
    detail::wr_le(out, offset_words, 2);
    detail::wr_le(out, frs_type, 2);
    detail::wr_le(out, block_words, 2);
    return out;
}

bytes sh2_build_frs_write_request(std::uint16_t frs_type, std::uint16_t length_words) {
    bytes out;
    out.push_back(std::byte{0xF7});
    out.push_back(std::byte{0x00});
    detail::wr_le(out, length_words, 2);
    detail::wr_le(out, frs_type, 2);
    return out;
}

bytes sh2_build_frs_write_data(std::uint16_t offset_words,
                               const std::vector<std::uint32_t>& words) {
    if (words.empty() || words.size() > 2)
        throw std::invalid_argument("FRS write data carries 1 or 2 words");
    std::uint32_t w0 = words[0];
    std::uint32_t w1 = words.size() > 1 ? words[1] : 0;
    bytes out;
    out.push_back(std::byte{0xF6});
    out.push_back(std::byte{0x00});
    detail::wr_le(out, offset_words, 2);
    detail::wr_le(out, w0, 4);
    detail::wr_le(out, w1, 4);
    return out;
}

// ── SH-2 input reports ──────────────────────────────────────────────────────

namespace {

// Sensor report IDs (subset used by the parser).
enum Sid : std::uint8_t {
    ACCELEROMETER = 0x01,
    GYROSCOPE = 0x02,
    MAGNETOMETER = 0x03,
    LINEAR_ACCELERATION = 0x04,
    ROTATION_VECTOR = 0x05,
    GRAVITY = 0x06,
    UNCALIBRATED_GYROSCOPE = 0x07,
    GAME_ROTATION_VECTOR = 0x08,
    GEOMAGNETIC_ROTATION_VECTOR = 0x09,
    PRESSURE = 0x0A,
    AMBIENT_LIGHT = 0x0B,
    HUMIDITY = 0x0C,
    PROXIMITY = 0x0D,
    TEMPERATURE = 0x0E,
    UNCALIBRATED_MAGNETOMETER = 0x0F,
    TAP_DETECTOR = 0x10,
    STEP_COUNTER = 0x11,
    SIGNIFICANT_MOTION = 0x12,
    STABILITY_CLASSIFIER = 0x13,
    RAW_ACCELEROMETER = 0x14,
    RAW_GYROSCOPE = 0x15,
    RAW_MAGNETOMETER = 0x16,
    STEP_DETECTOR = 0x18,
    SHAKE_DETECTOR = 0x19,
    PERSONAL_ACTIVITY_CLASSIFIER = 0x1E,
    ARVR_STABILIZED_RV = 0x28,
    ARVR_STABILIZED_GAME_RV = 0x29,
    GYRO_INTEGRATED_RV = 0x2A,
};

// Total report length on the wire, 4-byte SH-2 header included.
int report_length(std::uint8_t rid) {
    switch (rid) {
        case ACCELEROMETER: return 10;
        case GYROSCOPE: return 10;
        case MAGNETOMETER: return 10;
        case LINEAR_ACCELERATION: return 10;
        case ROTATION_VECTOR: return 14;
        case GRAVITY: return 10;
        case UNCALIBRATED_GYROSCOPE: return 16;
        case GAME_ROTATION_VECTOR: return 12;
        case GEOMAGNETIC_ROTATION_VECTOR: return 14;
        case PRESSURE: return 8;
        case AMBIENT_LIGHT: return 8;
        case HUMIDITY: return 6;
        case PROXIMITY: return 6;
        case TEMPERATURE: return 6;
        case UNCALIBRATED_MAGNETOMETER: return 16;
        case TAP_DETECTOR: return 5;
        case STEP_COUNTER: return 12;
        case SIGNIFICANT_MOTION: return 6;
        case STABILITY_CLASSIFIER: return 6;
        case RAW_ACCELEROMETER: return 16;
        case RAW_GYROSCOPE: return 16;
        case RAW_MAGNETOMETER: return 16;
        case STEP_DETECTOR: return 8;
        case SHAKE_DETECTOR: return 6;
        case 0x1A: return 6;  // flip
        case 0x1B: return 8;  // pickup
        case 0x1C: return 6;  // stability detector
        case PERSONAL_ACTIVITY_CLASSIFIER: return 16;
        case 0x1F: return 6;  // sleep
        case 0x20: return 6;  // tilt
        case 0x21: return 6;  // pocket
        case 0x22: return 6;  // circle
        case 0x23: return 6;  // heart rate
        case ARVR_STABILIZED_RV: return 14;
        case ARVR_STABILIZED_GAME_RV: return 12;
        case GYRO_INTEGRATED_RV: return 14;
        default: return -1;
    }
}

std::int16_t rd_i16(byte_span s, std::size_t off) {
    return static_cast<std::int16_t>(rd_u16(s, off));
}

Report decode_report(std::uint8_t rid, byte_span rep, std::int64_t ts, int seq, int accuracy,
                     std::int64_t delay_us) {
    Report r;
    r.sensor_id = rid;
    r.timestamp_us = ts;
    r.seq = seq;
    r.accuracy = accuracy;
    r.delay_us = delay_us;

    switch (rid) {
        case ACCELEROMETER:
        case LINEAR_ACCELERATION:
        case GRAVITY:
            r.type = ReportType::Acceleration;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            return r;
        case GYROSCOPE:
            r.type = ReportType::Gyroscope;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            return r;
        case MAGNETOMETER:
            r.type = ReportType::Magnetometer;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            return r;
        case UNCALIBRATED_GYROSCOPE:
            r.type = ReportType::UncalibratedGyroscope;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            r.bias_x_raw = rd_i16(rep, 10);
            r.bias_y_raw = rd_i16(rep, 12);
            r.bias_z_raw = rd_i16(rep, 14);
            return r;
        case UNCALIBRATED_MAGNETOMETER:
            r.type = ReportType::UncalibratedMagnetometer;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            r.bias_x_raw = rd_i16(rep, 10);
            r.bias_y_raw = rd_i16(rep, 12);
            r.bias_z_raw = rd_i16(rep, 14);
            return r;
        case ROTATION_VECTOR:
        case GEOMAGNETIC_ROTATION_VECTOR:
        case ARVR_STABILIZED_RV:
            r.type = ReportType::RotationVector;
            r.i_raw = rd_i16(rep, 4);
            r.j_raw = rd_i16(rep, 6);
            r.k_raw = rd_i16(rep, 8);
            r.real_raw = rd_i16(rep, 10);
            r.has_accuracy_raw = true;
            r.accuracy_raw = rd_i16(rep, 12);
            return r;
        case GAME_ROTATION_VECTOR:
        case ARVR_STABILIZED_GAME_RV:
            r.type = ReportType::RotationVector;
            r.i_raw = rd_i16(rep, 4);
            r.j_raw = rd_i16(rep, 6);
            r.k_raw = rd_i16(rep, 8);
            r.real_raw = rd_i16(rep, 10);
            r.has_accuracy_raw = false;
            return r;
        case PRESSURE:
        case AMBIENT_LIGHT:
            r.type = ReportType::ScalarReport;
            r.value_raw = rd_u32(rep, 4);
            return r;
        case HUMIDITY:
        case PROXIMITY:
            r.type = ReportType::ScalarReport;
            r.value_raw = rd_u16(rep, 4);
            return r;
        case TEMPERATURE:
            r.type = ReportType::ScalarReport;
            r.value_raw = rd_i16(rep, 4);
            return r;
        case TAP_DETECTOR:
            r.type = ReportType::TapDetector;
            r.flags = u8(rep[4]);
            return r;
        case STEP_COUNTER:
            r.type = ReportType::StepCounter;
            r.latency_us = rd_u32(rep, 4);
            r.steps = rd_u16(rep, 8);
            return r;
        case STEP_DETECTOR:
            r.type = ReportType::StepDetector;
            r.latency_us = rd_u32(rep, 4);
            return r;
        case SIGNIFICANT_MOTION:
            r.type = ReportType::SignificantMotion;
            r.motion = rd_u16(rep, 4);
            return r;
        case STABILITY_CLASSIFIER:
            r.type = ReportType::StabilityClassifier;
            r.classification = u8(rep[4]);
            return r;
        case SHAKE_DETECTOR:
            r.type = ReportType::ShakeDetector;
            r.flags = rd_u16(rep, 4);
            return r;
        case PERSONAL_ACTIVITY_CLASSIFIER:
            r.type = ReportType::PersonalActivityClassifier;
            r.page_number = u8(rep[4]) & 0x7F;
            r.end_of_sequence = (u8(rep[4]) & 0x80) != 0;
            r.most_likely_state = u8(rep[5]);
            for (std::size_t z = 6; z < 16; ++z) r.confidences.push_back(u8(rep[z]));
            return r;
        case RAW_ACCELEROMETER:
        case RAW_MAGNETOMETER:
            r.type = ReportType::RawSensor;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            r.sensor_timestamp_us = rd_u32(rep, 12);
            return r;
        case RAW_GYROSCOPE:
            r.type = ReportType::RawSensor;
            r.x_raw = rd_i16(rep, 4);
            r.y_raw = rd_i16(rep, 6);
            r.z_raw = rd_i16(rep, 8);
            r.temperature_raw = rd_i16(rep, 10);
            r.sensor_timestamp_us = rd_u32(rep, 12);
            return r;
        default:
            r.type = ReportType::GenericEvent;
            r.value_raw = rd_u16(rep, 4);
            return r;
    }
}

}  // namespace

std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us) {
    std::vector<Report> out;
    std::int64_t base_us = capture_timestamp_us;
    std::size_t pos = 0;
    std::size_t n = payload.size();
    while (pos < n) {
        std::uint8_t rid = u8(payload[pos]);
        if (rid == BASE_TIMESTAMP_REF && pos + 5 <= n) {
            std::int32_t delta = static_cast<std::int32_t>(rd_u32(payload, pos + 1));
            base_us = capture_timestamp_us - static_cast<std::int64_t>(delta) * 100;
            pos += 5;
            continue;
        }
        if (rid == TIMESTAMP_REBASE && pos + 5 <= n) {
            std::int32_t delta = static_cast<std::int32_t>(rd_u32(payload, pos + 1));
            base_us += static_cast<std::int64_t>(delta) * 100;
            pos += 5;
            continue;
        }
        int length = report_length(rid);
        if (length < 0 || pos + static_cast<std::size_t>(length) > n) {
            Report r;
            r.type = ReportType::UnknownReport;
            r.sensor_id = rid;
            r.timestamp_us = base_us;
            r.data.assign(payload.begin() + pos, payload.end());
            out.push_back(std::move(r));
            break;
        }
        byte_span rep = payload.subspan(pos, static_cast<std::size_t>(length));
        int seq = u8(rep[1]);
        int status = u8(rep[2]);
        int delay_lsb = u8(rep[3]);
        int accuracy = status & 0x03;
        std::int64_t delay_us = (((status >> 2) << 8) | delay_lsb) * 100;
        std::int64_t ts = base_us + delay_us;
        out.push_back(decode_report(rid, rep, ts, seq, accuracy, delay_us));
        pos += static_cast<std::size_t>(length);
    }
    return out;
}

std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us) {
    std::int64_t ts = capture_timestamp_us;
    byte_span body = payload;
    if (!payload.empty() && u8(payload[0]) == BASE_TIMESTAMP_REF) {
        if (payload.size() < 5 + 2 + 14) return std::nullopt;
        std::int32_t delta = static_cast<std::int32_t>(rd_u32(payload, 1));
        std::uint16_t delay = rd_u16(payload, 5);
        ts = capture_timestamp_us - static_cast<std::int64_t>(delta) * 100 +
             static_cast<std::int64_t>(delay) * 100;
        body = payload.subspan(7);
    }
    if (body.size() < 14) return std::nullopt;
    Report r;
    r.type = ReportType::GyroIntegratedRV;
    r.sensor_id = GYRO_INTEGRATED_RV;
    r.timestamp_us = ts;
    r.i_raw = rd_i16(body, 0);
    r.j_raw = rd_i16(body, 2);
    r.k_raw = rd_i16(body, 4);
    r.real_raw = rd_i16(body, 6);
    r.vx_raw = rd_i16(body, 8);
    r.vy_raw = rd_i16(body, 10);
    r.vz_raw = rd_i16(body, 12);
    return r;
}

}  // namespace bno086
}  // namespace depz
