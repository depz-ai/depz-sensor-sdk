#include "depz/bno086.hpp"

#include <new>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace detail {

// The C and C++ report enums list their members in different orders.
bno086::ReportType bno086_report_type_from_c(depz_bno_report_type t) {
    using T = bno086::ReportType;
    switch (t) {
        case DEPZ_BNO_ACCELERATION: return T::Acceleration;
        case DEPZ_BNO_GYROSCOPE: return T::Gyroscope;
        case DEPZ_BNO_MAGNETOMETER: return T::Magnetometer;
        case DEPZ_BNO_UNCAL_GYROSCOPE: return T::UncalibratedGyroscope;
        case DEPZ_BNO_UNCAL_MAGNETOMETER: return T::UncalibratedMagnetometer;
        case DEPZ_BNO_ROTATION_VECTOR: return T::RotationVector;
        case DEPZ_BNO_SCALAR_REPORT: return T::ScalarReport;
        case DEPZ_BNO_TAP_DETECTOR: return T::TapDetector;
        case DEPZ_BNO_STEP_COUNTER: return T::StepCounter;
        case DEPZ_BNO_STEP_DETECTOR: return T::StepDetector;
        case DEPZ_BNO_SIGNIFICANT_MOTION: return T::SignificantMotion;
        case DEPZ_BNO_STABILITY_CLASSIFIER: return T::StabilityClassifier;
        case DEPZ_BNO_SHAKE_DETECTOR: return T::ShakeDetector;
        case DEPZ_BNO_ACTIVITY_CLASSIFIER: return T::PersonalActivityClassifier;
        case DEPZ_BNO_RAW_SENSOR: return T::RawSensor;
        case DEPZ_BNO_GENERIC_EVENT: return T::GenericEvent;
        case DEPZ_BNO_GYRO_INTEGRATED_RV: return T::GyroIntegratedRV;
        case DEPZ_BNO_MOTION_REQUEST: return T::MotionRequest;
        case DEPZ_BNO_FLUSH_COMPLETED_REPORT: return T::FlushCompleted;
        case DEPZ_BNO_UNKNOWN_REPORT: break;
    }
    return T::UnknownReport;
}

bno086::Report bno086_report_from_c(const depz_bno_report& c) {
    bno086::Report r;
    r.type = bno086_report_type_from_c(c.type);
    r.sensor_id = c.sensor_id;
    r.timestamp_us = c.timestamp_us;
    r.seq = c.seq;
    r.accuracy = c.accuracy;
    r.delay_us = c.delay_us;
    r.x_raw = c.x_raw; r.y_raw = c.y_raw; r.z_raw = c.z_raw;
    r.bias_x_raw = c.bias_x_raw; r.bias_y_raw = c.bias_y_raw; r.bias_z_raw = c.bias_z_raw;
    r.i_raw = c.i_raw; r.j_raw = c.j_raw; r.k_raw = c.k_raw; r.real_raw = c.real_raw;
    r.has_accuracy_raw = c.has_accuracy_raw;
    r.accuracy_raw = c.has_accuracy_raw ? c.accuracy_raw : 0;
    r.value_raw = c.value_raw;
    r.flags = c.flags;
    r.latency_us = c.latency_us;
    r.steps = static_cast<int>(c.steps);
    r.motion = c.motion;
    r.classification = c.classification;
    r.page_number = c.page_number;
    r.end_of_sequence = c.end_of_sequence;
    r.most_likely_state = c.most_likely_state;
    r.intent = c.intent;
    r.request = c.request;
    r.flushed_sensor = c.flushed_sensor;
    if (c.type == DEPZ_BNO_ACTIVITY_CLASSIFIER) r.confidences.assign(c.confidences, c.confidences + 10);
    r.sensor_timestamp_us = c.sensor_timestamp_us;
    r.temperature_raw = c.temperature_raw;
    r.vx_raw = c.vx_raw; r.vy_raw = c.vy_raw; r.vz_raw = c.vz_raw;
    if (c.data && c.data_len) r.data = to_bytes(c.data, c.data_len);
    return r;
}

depz_bno_report_type bno086_report_type_to_c(bno086::ReportType t) {
    using T = bno086::ReportType;
    switch (t) {
        case T::Acceleration: return DEPZ_BNO_ACCELERATION;
        case T::Gyroscope: return DEPZ_BNO_GYROSCOPE;
        case T::Magnetometer: return DEPZ_BNO_MAGNETOMETER;
        case T::UncalibratedGyroscope: return DEPZ_BNO_UNCAL_GYROSCOPE;
        case T::UncalibratedMagnetometer: return DEPZ_BNO_UNCAL_MAGNETOMETER;
        case T::RotationVector: return DEPZ_BNO_ROTATION_VECTOR;
        case T::ScalarReport: return DEPZ_BNO_SCALAR_REPORT;
        case T::TapDetector: return DEPZ_BNO_TAP_DETECTOR;
        case T::StepCounter: return DEPZ_BNO_STEP_COUNTER;
        case T::StepDetector: return DEPZ_BNO_STEP_DETECTOR;
        case T::SignificantMotion: return DEPZ_BNO_SIGNIFICANT_MOTION;
        case T::StabilityClassifier: return DEPZ_BNO_STABILITY_CLASSIFIER;
        case T::ShakeDetector: return DEPZ_BNO_SHAKE_DETECTOR;
        case T::PersonalActivityClassifier: return DEPZ_BNO_ACTIVITY_CLASSIFIER;
        case T::RawSensor: return DEPZ_BNO_RAW_SENSOR;
        case T::GenericEvent: return DEPZ_BNO_GENERIC_EVENT;
        case T::GyroIntegratedRV: return DEPZ_BNO_GYRO_INTEGRATED_RV;
        case T::MotionRequest: return DEPZ_BNO_MOTION_REQUEST;
        case T::FlushCompleted: return DEPZ_BNO_FLUSH_COMPLETED_REPORT;
        case T::UnknownReport: break;
    }
    return DEPZ_BNO_UNKNOWN_REPORT;
}

depz_bno_report bno086_report_to_c(const bno086::Report& r) {
    depz_bno_report c{};
    c.type = bno086_report_type_to_c(r.type);
    c.sensor_id = static_cast<std::uint8_t>(r.sensor_id);
    c.timestamp_us = r.timestamp_us;
    c.seq = static_cast<std::uint8_t>(r.seq);
    c.accuracy = static_cast<std::uint8_t>(r.accuracy);
    c.delay_us = static_cast<std::int32_t>(r.delay_us);
    c.x_raw = static_cast<std::int32_t>(r.x_raw); c.y_raw = static_cast<std::int32_t>(r.y_raw);
    c.z_raw = static_cast<std::int32_t>(r.z_raw);
    c.bias_x_raw = static_cast<std::int32_t>(r.bias_x_raw); c.bias_y_raw = static_cast<std::int32_t>(r.bias_y_raw);
    c.bias_z_raw = static_cast<std::int32_t>(r.bias_z_raw);
    c.i_raw = static_cast<std::int32_t>(r.i_raw); c.j_raw = static_cast<std::int32_t>(r.j_raw);
    c.k_raw = static_cast<std::int32_t>(r.k_raw); c.real_raw = static_cast<std::int32_t>(r.real_raw);
    c.has_accuracy_raw = r.has_accuracy_raw;
    c.accuracy_raw = static_cast<std::int32_t>(r.accuracy_raw);
    c.value_raw = r.value_raw;
    c.flags = static_cast<std::int32_t>(r.flags);
    c.latency_us = static_cast<std::uint32_t>(r.latency_us);
    c.steps = static_cast<std::uint32_t>(r.steps);
    c.motion = static_cast<std::int32_t>(r.motion);
    c.classification = static_cast<std::int32_t>(r.classification);
    c.page_number = static_cast<std::int32_t>(r.page_number);
    c.end_of_sequence = r.end_of_sequence;
    c.most_likely_state = static_cast<std::int32_t>(r.most_likely_state);
    for (std::size_t i = 0; i < 10 && i < r.confidences.size(); ++i)
        c.confidences[i] = static_cast<std::uint8_t>(r.confidences[i]);
    c.intent = static_cast<std::int32_t>(r.intent);
    c.request = static_cast<std::int32_t>(r.request);
    c.flushed_sensor = static_cast<std::int32_t>(r.flushed_sensor);
    c.sensor_timestamp_us = static_cast<std::uint32_t>(r.sensor_timestamp_us);
    c.temperature_raw = static_cast<std::int32_t>(r.temperature_raw);
    c.vx_raw = static_cast<std::int32_t>(r.vx_raw); c.vy_raw = static_cast<std::int32_t>(r.vy_raw);
    c.vz_raw = static_cast<std::int32_t>(r.vz_raw);
    c.data = r.data.empty() ? nullptr : reinterpret_cast<const std::uint8_t*>(r.data.data());
    c.data_len = r.data.size();
    return c;
}

}  // namespace detail

namespace bno086 {

using detail::packed;
using detail::to_bytes;
using detail::u8p;

// ── SHTP framing ────────────────────────────────────────────────────────────

bytes ShtpHeader::pack() const {
    const depz_shtp_header h{length, channel, seq, continuation};
    std::uint8_t out[4];
    // bits 14:0: a longer length is refused, never masked (contract 07 §3)
    if (depz_shtp_pack_header_checked(&h, out) != 0) throw std::invalid_argument("ShtpHeader: length over 15 bits");
    return to_bytes(out, sizeof out);
}

ShtpHeader ShtpHeader::unpack(byte_span data) {
    if (data.size() < SHTP_HEADER_SIZE) throw std::invalid_argument("ShtpHeader: short");
    depz_shtp_header c{};
    depz_shtp_unpack_header(u8p(data), &c);
    return ShtpHeader{c.length, c.channel, c.seq, c.continuation};
}

bytes shtp_build_frame(int channel, byte_span payload, std::uint8_t seq) {
    if (channel < 0 || channel > 0xFF) throw std::out_of_range("SHTP channel is a byte");
    if (SHTP_HEADER_SIZE + payload.size() > LENGTH_MASK) throw std::invalid_argument("SHTP frame length over 15 bits");
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
    if (max_frame <= SHTP_HEADER_SIZE) throw std::invalid_argument("max_frame must exceed the 4-byte SHTP header");
    if (channel < 0 || channel > 0xFF) throw std::out_of_range("SHTP channel is a byte");
    const std::size_t total = SHTP_HEADER_SIZE + payload.size();  // the first header's length field
    if (total > LENGTH_MASK) throw std::invalid_argument("SHTP cargo length over 15 bits");
    const std::size_t room = max_frame - SHTP_HEADER_SIZE;
    std::vector<bytes> frames;
    std::size_t off = 0;
    std::uint8_t seq = seq_start;
    while (true) {
        const std::size_t chunk = std::min(room, payload.size() - off);
        ShtpHeader h;
        h.length = static_cast<std::uint16_t>(total - off);  // remaining, one header included
        h.channel = static_cast<std::uint8_t>(channel);
        h.seq = seq;
        h.continuation = off > 0;
        // header + chunk into a frame of the final size (an insert into the
        // packed header trips gcc's -O2 bounds analysis of the 4-byte buffer)
        const bytes hdr = h.pack();
        bytes frame(SHTP_HEADER_SIZE + chunk);
        std::copy(hdr.begin(), hdr.end(), frame.begin());
        std::copy(payload.begin() + static_cast<std::ptrdiff_t>(off),
                  payload.begin() + static_cast<std::ptrdiff_t>(off + chunk),
                  frame.begin() + static_cast<std::ptrdiff_t>(SHTP_HEADER_SIZE));
        frames.push_back(std::move(frame));
        off += chunk;
        seq = static_cast<std::uint8_t>(seq + 1);
        if (off >= payload.size()) return frames;
    }
}

bytes ShtpLayer::next_frame(int channel, byte_span payload) {
    if (SHTP_HEADER_SIZE + payload.size() > MAX_TX_FRAME) throw std::invalid_argument("TX cargo exceeds the MCU slot");
    // the per-channel seq table has NUM_CHANNELS entries: refuse, never index past it
    if (channel < 0 || channel >= NUM_CHANNELS) throw std::out_of_range("SHTP channel out of range 0..5");
    std::uint8_t out[MAX_TX_FRAME];
    return packed(out, depz_shtp_next_frame(&l_, static_cast<std::uint8_t>(channel), u8p(payload), payload.size(), out,
                                            sizeof out),
                  "SHTP frame refused");
}

std::optional<ShtpCargo> ShtpLayer::feed(byte_span frame, std::uint64_t capture_us) {
    depz_shtp_cargo c{};
    const int rc = depz_shtp_feed_ex(&l_, u8p(frame), frame.size(), capture_us, &c);
    if (rc < 0) throw std::bad_alloc();
    if (rc == 0) return std::nullopt;
    return ShtpCargo{c.channel, c.seq, to_bytes(c.payload, c.payload_len), c.capture_us};
}

void ShtpLayer::reset() {
    depz_shtp_free(&l_);
    depz_shtp_init(&l_);
}

// ── SH-2 control-channel encoders ───────────────────────────────────────────

bytes sh2_build_set_feature(std::uint8_t sensor_id, std::uint32_t interval_us, std::uint32_t batch_us,
                            std::uint16_t sensitivity, std::uint8_t flags, std::uint32_t cfg_word) {
    std::uint8_t out[17];
    return to_bytes(out, depz_bno_pack_set_feature(sensor_id, flags, sensitivity, interval_us, batch_us, cfg_word, out));
}

bytes sh2_build_get_feature_request(std::uint8_t sensor_id) {
    std::uint8_t out[2];
    return to_bytes(out, depz_bno_pack_get_feature_request(sensor_id, out));
}

bytes sh2_build_product_id_request() {
    std::uint8_t out[2];
    return to_bytes(out, depz_bno_pack_product_id_request(out));
}

bytes sh2_build_command_request(std::uint8_t seq, std::uint8_t command, byte_span params) {
    std::uint8_t out[12];
    return packed(out, depz_bno_pack_command_request(seq, command, u8p(params), params.size(), out),
                  "command request carries at most 9 parameter bytes");
}

bytes sh2_build_frs_read_request(std::uint16_t frs_type, std::uint16_t offset_words, std::uint16_t block_words) {
    std::uint8_t out[8];
    return to_bytes(out, depz_bno_pack_frs_read_request(frs_type, offset_words, block_words, out));
}

bytes sh2_build_frs_write_request(std::uint16_t frs_type, std::uint16_t length_words) {
    std::uint8_t out[6];
    return to_bytes(out, depz_bno_pack_frs_write_request(frs_type, length_words, out));
}

bytes sh2_build_frs_write_data(std::uint16_t offset_words, const std::vector<std::uint32_t>& words) {
    std::uint8_t out[12];
    return packed(out, depz_bno_pack_frs_write_data(offset_words, words.data(), words.size(), out, sizeof out),
                  "FRS write data carries 1 or 2 words");
}

bytes sh2_build_interactive_calibration(std::uint8_t seq, MotionIntent intent) {
    std::uint8_t out[12];
    return to_bytes(out, depz_bno_pack_interactive_calibration(seq, static_cast<std::uint8_t>(intent), out));
}

bytes sh2_build_force_flush(std::uint8_t sensor_id) {
    std::uint8_t out[2];
    return to_bytes(out, depz_bno_pack_force_flush(sensor_id, out));
}

// ── SH-2 control-channel responses ──────────────────────────────────────────

std::optional<Feature> Feature::unpack(byte_span p) {
    Feature f{};
    if (depz_bno_unpack_feature_response(u8p(p), p.size(), &f) != 0) return std::nullopt;
    return f;
}

std::optional<ProductId> ProductId::unpack(byte_span p) {
    ProductId r{};
    if (depz_bno_unpack_product_id(u8p(p), p.size(), &r) != 0) return std::nullopt;
    return r;
}

std::optional<CommandResponse> CommandResponse::unpack(byte_span p) {
    CommandResponse r{};
    if (depz_bno_unpack_command_response(u8p(p), p.size(), &r) != 0) return std::nullopt;
    return r;
}

std::optional<FrsReadResponse> FrsReadResponse::unpack(byte_span p) {
    FrsReadResponse r{};
    if (depz_bno_unpack_frs_read_response(u8p(p), p.size(), &r) != 0) return std::nullopt;
    return r;
}

std::optional<FrsWriteResponse> FrsWriteResponse::unpack(byte_span p) {
    FrsWriteResponse r{};
    if (depz_bno_unpack_frs_write_response(u8p(p), p.size(), &r) != 0) return std::nullopt;
    return r;
}

// ── SH-2 input reports ──────────────────────────────────────────────────────

std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us) {
    if (capture_timestamp_us < 0) throw std::invalid_argument("capture_timestamp_us must not be negative");
    // every report takes at least two cargo bytes (the bare 0xEF Flush Completed)
    std::vector<depz_bno_report> c(payload.size() / 2 + 1);
    const std::size_t n = depz_bno_parse_input_cargo(u8p(payload), payload.size(),
                                                     static_cast<std::uint64_t>(capture_timestamp_us), c.data(),
                                                     c.size());
    std::vector<Report> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) out.push_back(detail::bno086_report_from_c(c[i]));
    return out;
}

std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us) {
    if (capture_timestamp_us < 0) throw std::invalid_argument("capture_timestamp_us must not be negative");
    depz_bno_report c{};
    if (depz_bno_parse_gyro_rv(u8p(payload), payload.size(), static_cast<std::uint64_t>(capture_timestamp_us), &c) != 0)
        return std::nullopt;
    return detail::bno086_report_from_c(c);
}

}  // namespace bno086
}  // namespace depz
