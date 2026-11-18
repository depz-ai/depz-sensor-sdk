#include "depz/vl53l8.hpp"

#include <cmath>
#include <memory>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace detail {

vl53l8::Vl53l8Frame vl53l8_frame_from_c(const depz_vl53l8_frame& c, const std::uint8_t* cnh, std::size_t cnh_len) {
    vl53l8::Vl53l8Frame f;
    const int n = c.resolution;
    f.timestamp_us = c.timestamp_us;
    f.resolution = n;
    f.silicon_temp_degc = c.silicon_temp_degc;
    f.distance_mm.assign(c.distance_mm, c.distance_mm + n);
    f.target_status.assign(c.target_status, c.target_status + n);
    f.nb_target_detected.assign(c.nb_target_detected, c.nb_target_detected + n);
    f.signal_per_spad.assign(c.signal_per_spad, c.signal_per_spad + n);
    f.ambient_per_spad.assign(c.ambient_per_spad, c.ambient_per_spad + n);
    f.nb_spads_enabled.assign(c.nb_spads_enabled, c.nb_spads_enabled + n);
    f.range_sigma_mm.assign(c.range_sigma_mm, c.range_sigma_mm + n);
    f.reflectance.assign(c.reflectance, c.reflectance + n);
    if (cnh_len) f.cnh_raw = to_bytes(cnh, cnh_len);
    return f;
}

}  // namespace detail

namespace vl53l8 {

using detail::to_bytes;
using detail::u8p;

// ── FrameChunk / reassembler ────────────────────────────────────────────────

bytes pack_start_stream(std::uint16_t frame_size) {
    std::uint8_t out[2];
    // contract 04 §4 rule 4: the firmware refuses 0 and > 8192; so does the host
    return detail::packed(out, depz_vl53l8_pack_start_stream(frame_size, out),
                          "frame_size outside 1..8192");
}

FrameChunk FrameChunk::unpack(byte_span payload) {
    depz_vl53l8_chunk c{};
    if (depz_vl53l8_unpack_chunk(u8p(payload), payload.size(), &c) != 0)
        throw std::invalid_argument("FrameChunk: short payload");
    return FrameChunk{c.timestamp_us, c.full_size, c.offset, to_bytes(c.data, c.data_len)};
}

std::optional<std::pair<std::uint64_t, bytes>> FrameReassembler::feed(const FrameChunk& chunk) {
    depz_vl53l8_chunk c{};
    c.timestamp_us = chunk.timestamp_us;
    c.full_size = chunk.full_size;
    c.offset = chunk.offset;
    c.data = u8p(chunk.data);
    c.data_len = chunk.data.size();
    const std::uint8_t* frame = nullptr;
    std::size_t frame_len = 0;
    std::uint64_t ts = 0;
    if (depz_vl53l8_reasm_feed(&r_, &c, &frame, &frame_len, &ts) != 1) return std::nullopt;
    return std::make_pair(ts, to_bytes(frame, frame_len));
}

// ── frame decode ────────────────────────────────────────────────────────────

bytes swap_buffer(byte_span data) {
    bytes out(data.begin(), data.end());
    depz_vl53l8_swap_buffer(reinterpret_cast<std::uint8_t*>(out.data()), out.size());
    return out;
}

std::optional<Vl53l8Frame> decode_frame(byte_span raw, int footer_id_off) {
    auto c = std::make_unique<depz_vl53l8_frame>();
    if (footer_id_off == FOOTER_ID_OFF_CX) {
        if (depz_vl53l8_decode_frame(u8p(raw), raw.size(), 0, c.get()) != 0) return std::nullopt;
        return detail::vl53l8_frame_from_c(*c, nullptr, 0);
    }
    if (footer_id_off != FOOTER_ID_OFF_CH)
        throw std::invalid_argument("decode_frame: footer_id_off is FOOTER_ID_OFF_CX (12) or FOOTER_ID_OFF_CH (4)");
    bytes cnh(raw.size());
    std::size_t cnh_len = 0;
    if (depz_vl53l8ch_decode_frame(u8p(raw), raw.size(), 0, c.get(), reinterpret_cast<std::uint8_t*>(cnh.data()),
                                   cnh.size(), &cnh_len) != 0)
        return std::nullopt;
    return detail::vl53l8_frame_from_c(*c, reinterpret_cast<const std::uint8_t*>(cnh.data()), cnh_len);
}

// ── advanced-feature DCI codecs ─────────────────────────────────────────────

std::uint32_t xtalk_margin_raw(std::uint32_t margin_kcps) {
    std::uint32_t raw = 0;
    if (depz_vl53l8_xtalk_margin_to_raw(margin_kcps, &raw) != 0) throw std::out_of_range("xtalk margin above 10000 kcps");
    return raw;
}

std::uint32_t xtalk_margin_kcps(std::uint32_t raw) { return depz_vl53l8_xtalk_margin_from_raw(raw); }

bytes pack_detection_thresholds(const std::vector<DetectionThreshold>& thresholds) {
    if (thresholds.size() > NB_THRESHOLDS) throw std::invalid_argument("at most 64 detection thresholds");
    std::uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
    if (depz_vl53l8_pack_thresholds_checked(thresholds.data(), thresholds.size(), start, valid) != 0)
        throw std::out_of_range("a scaled detection threshold overflows i32");
    return to_bytes(start, sizeof start);
}

bytes detection_thresholds_valid_status() {
    std::uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
    depz_vl53l8_pack_thresholds(nullptr, 0, start, valid);
    return to_bytes(valid, sizeof valid);
}

bytes MotionConfig::pack() const {
    std::uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE];
    depz_vl53l8_motion_cfg_pack(this, out);
    return to_bytes(out, sizeof out);
}

MotionConfig motion_config_init(int resolution) {
    MotionConfig cfg{};
    if (depz_vl53l8_motion_cfg_default(resolution, &cfg) != 0)
        throw std::invalid_argument("motion config: resolution is 16 (4x4) or 64 (8x8)");
    return cfg;
}

// ── CNH decode ──────────────────────────────────────────────────────────────

CnhFrame decode_cnh(int nb_of_aggregates, int feature_length, byte_span raw) {
    const depz_vl53l8ch_cnh_config cfg{nb_of_aggregates, feature_length};
    auto c = std::make_unique<depz_vl53l8ch_cnh_frame>();
    switch (depz_vl53l8ch_decode_cnh(&cfg, u8p(raw), raw.size(), c.get())) {
        case 0: break;
        case -1: throw std::invalid_argument("decode_cnh: aggregates / feature length out of range");
        case -2: throw std::length_error("decode_cnh: block shorter than the CNH header");
        default: throw std::length_error("decode_cnh: block shorter than the config implies");
    }
    CnhFrame out;
    out.ref_residual_word = c->ref_residual_word;
    out.ref_residual = c->ref_residual_word / 2048.0;
    out.aggregates.reserve(static_cast<std::size_t>(c->nb_aggregates));
    for (int agg = 0; agg < c->nb_aggregates; ++agg) {
        CnhAggregate a;
        const auto n = static_cast<std::size_t>(c->feature_length);
        a.hist_raw.assign(c->hist_raw[agg], c->hist_raw[agg] + n);
        a.hist_scaler.assign(c->hist_scaler[agg], c->hist_scaler[agg] + n);
        a.hist.resize(n);
        for (std::size_t i = 0; i < n; ++i) a.hist[i] = a.hist_raw[i] / std::pow(2.0, a.hist_scaler[i]);
        a.ambient_raw = c->ambient_raw[agg];
        a.ambient_scaler = c->ambient_scaler[agg];
        a.ambient = a.ambient_raw / std::pow(2.0, a.ambient_scaler);
        out.aggregates.push_back(std::move(a));
    }
    return out;
}

}  // namespace vl53l8
}  // namespace depz
