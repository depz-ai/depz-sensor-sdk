#include <stdexcept>
#include <string>
#include "depz/vl53l8.hpp"

#include <cmath>
#include <cstring>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace vl53l8 {

using detail::rd_u16;
using detail::rd_u32;
using detail::rd_u64;
using detail::u8;

namespace {

// union Block_header: type[3:0], size[15:4], idx[31:16].
struct BhFields {
    std::uint32_t type;
    std::uint32_t size;
    std::uint32_t idx;
};
BhFields bh_fields(std::uint32_t bh) {
    return BhFields{bh & 0xF, (bh >> 4) & 0xFFF, (bh >> 16) & 0xFFFF};
}

// Block indices (uld.py).
constexpr std::uint32_t METADATA_IDX = 0x54B4;
constexpr std::uint32_t SPAD_COUNT_IDX = 0x55D0;
constexpr std::uint32_t AMBIENT_RATE_IDX = 0x54D0;
constexpr std::uint32_t NB_TARGET_DETECTED_IDX = 0xDB84;
constexpr std::uint32_t SIGNAL_RATE_IDX = 0xDBC4;
constexpr std::uint32_t RANGE_SIGMA_MM_IDX = 0xDEC4;
constexpr std::uint32_t DISTANCE_IDX = 0xDF44;
constexpr std::uint32_t REFLECTANCE_EST_PC_IDX = 0xE044;
constexpr std::uint32_t TARGET_STATUS_IDX = 0xE084;

constexpr int NB_TARGET_PER_ZONE = 1;

// ST GetRangingData distance: wire value / 4 floored, a negative result
// clamped to 0 (contract 04 §5; Python `max(0, d // 4)`). A negative wire
// value floors to a negative result, so all of them give 0.
std::int32_t distance_mm_from_raw(std::int32_t a) {
    return a <= 0 ? 0 : a / 4;
}

// Wire integers -> real units: value / divisor as a double (ST divides
// integers; the fraction is kept).
template <typename T>
std::vector<double> scaled(const std::vector<T>& raw, double divisor) {
    std::vector<double> out(raw.size());
    for (std::size_t k = 0; k < raw.size(); ++k) out[k] = static_cast<double>(raw[k]) / divisor;
    return out;
}

std::int16_t s16(std::uint16_t v) { return static_cast<std::int16_t>(v); }

}  // namespace

// ── FrameChunk / reassembler ────────────────────────────────────────────────

bytes pack_start_stream(std::uint16_t frame_size) {
    // contract 04 §4 rule 4: the firmware refuses 0 and > 8192; so does the host
    if (frame_size < 1 || frame_size > STREAM_TOTAL_MAX)
        throw std::invalid_argument("frame_size " + std::to_string(frame_size) + " outside 1..8192");
    bytes out;
    detail::wr_le(out, frame_size, 2);
    return out;
}

FrameChunk FrameChunk::unpack(byte_span payload) {
    detail::require(payload, 12, "FrameChunk: short payload");
    FrameChunk c;
    c.timestamp_us = rd_u64(payload, 0);
    c.full_size = rd_u16(payload, 8);
    c.offset = rd_u16(payload, 10);
    c.data.assign(payload.begin() + 12, payload.end());
    return c;
}

std::optional<std::pair<std::uint64_t, bytes>> FrameReassembler::feed(const FrameChunk& chunk) {
    if (chunk.data.size() > chunk_max_ || chunk.full_size > STREAM_TOTAL_MAX ||
        std::size_t{chunk.offset} + chunk.data.size() > chunk.full_size) {
        // rule 4: damaged — the frame in progress is lost (counted, rules 1-2)
        // and so is the frame an offset-0 chunk starts (counted); a damaged
        // orphan with nothing in progress is dropped like any orphan
        if (!buf_.empty()) ++discarded;
        if (chunk.offset == 0) ++discarded;
        buf_.clear();
        full_size_ = 0;
        return std::nullopt;
    }
    if (chunk.offset == 0) {
        if (!buf_.empty() && buf_.size() != full_size_) ++discarded;
        buf_ = chunk.data;
        full_size_ = chunk.full_size;
        timestamp_us_ = chunk.timestamp_us;
    } else if (chunk.offset == buf_.size() && full_size_ == chunk.full_size && !buf_.empty()) {
        buf_.insert(buf_.end(), chunk.data.begin(), chunk.data.end());
    } else {
        if (!buf_.empty()) ++discarded;
        buf_.clear();
        full_size_ = 0;
        return std::nullopt;
    }
    if (buf_.size() == full_size_ && full_size_ > 0) {
        bytes frame = std::move(buf_);
        buf_.clear();
        full_size_ = 0;
        ++completed;
        return std::make_pair(timestamp_us_, std::move(frame));
    }
    return std::nullopt;
}

// ── frame decode ────────────────────────────────────────────────────────────

bytes swap_buffer(byte_span data) {
    bytes out(data.begin(), data.end());
    std::size_t n = out.size() / 4;
    for (std::size_t w = 0; w < n; ++w) {
        std::size_t o = w * 4;
        std::swap(out[o + 0], out[o + 3]);
        std::swap(out[o + 1], out[o + 2]);
    }
    return out;
}

std::optional<Vl53l8Frame> decode_frame(byte_span raw, int footer_id_off) {
    const std::size_t drs = raw.size();
    if (drs < 16) return std::nullopt;
    bytes swapped = swap_buffer(raw);
    byte_span buf = as_bytes(swapped);

    Vl53l8Frame f;
    // Default to full 8x8; blocks overwrite/resize as present.
    std::vector<std::int32_t> distance(RESOLUTION_8X8 * NB_TARGET_PER_ZONE, 0);
    std::vector<std::uint8_t> target_status(RESOLUTION_8X8 * NB_TARGET_PER_ZONE, 0);
    std::vector<std::uint8_t> nb_target(RESOLUTION_8X8, 0);
    std::vector<std::uint32_t> signal(RESOLUTION_8X8 * NB_TARGET_PER_ZONE, 0);
    std::vector<std::uint32_t> ambient(RESOLUTION_8X8, 0);
    std::vector<std::uint32_t> spad(RESOLUTION_8X8, 0);
    std::vector<std::uint16_t> sigma(RESOLUTION_8X8 * NB_TARGET_PER_ZONE, 0);
    std::vector<std::uint8_t> reflect(RESOLUTION_8X8 * NB_TARGET_PER_ZONE, 0);
    int silicon_temp = 0;

    std::size_t i = 16;
    while (i + 4 <= drs) {
        std::uint32_t bh = rd_u32(buf, i);
        BhFields bf = bh_fields(bh);
        std::size_t msize = (bf.type > 0x1 && bf.type < 0xD) ? bf.type * bf.size : bf.size;
        // Exact-sized buffer: stop once a block would run past the end (footer).
        if (i + 4 + msize > drs) break;

        if (bf.idx == METADATA_IDX) {
            silicon_temp = static_cast<std::int8_t>(u8(buf[i + 12]));
        } else if (bf.idx == DISTANCE_IDX) {
            std::size_t nz = msize / 2;
            distance.assign(nz, 0);
            for (std::size_t z = 0; z < nz; ++z) distance[z] = s16(rd_u16(buf, i + 4 + 2 * z));
        } else if (bf.idx == TARGET_STATUS_IDX) {
            target_status.assign(msize, 0);
            for (std::size_t z = 0; z < msize; ++z) target_status[z] = u8(buf[i + 4 + z]);
        } else if (bf.idx == NB_TARGET_DETECTED_IDX) {
            nb_target.assign(msize, 0);
            for (std::size_t z = 0; z < msize; ++z) nb_target[z] = u8(buf[i + 4 + z]);
        } else if (bf.idx == SIGNAL_RATE_IDX) {
            std::size_t nz = msize / 4;
            signal.assign(nz, 0);
            for (std::size_t z = 0; z < nz; ++z) signal[z] = rd_u32(buf, i + 4 + 4 * z);
        } else if (bf.idx == AMBIENT_RATE_IDX) {
            std::size_t nz = msize / 4;
            ambient.assign(nz, 0);
            for (std::size_t z = 0; z < nz; ++z) ambient[z] = rd_u32(buf, i + 4 + 4 * z);
        } else if (bf.idx == SPAD_COUNT_IDX) {
            std::size_t nz = msize / 4;
            spad.assign(nz, 0);
            for (std::size_t z = 0; z < nz; ++z) spad[z] = rd_u32(buf, i + 4 + 4 * z);
        } else if (bf.idx == RANGE_SIGMA_MM_IDX) {
            std::size_t nz = msize / 2;
            sigma.assign(nz, 0);
            for (std::size_t z = 0; z < nz; ++z) sigma[z] = rd_u16(buf, i + 4 + 2 * z);
        } else if (bf.idx == REFLECTANCE_EST_PC_IDX) {
            reflect.assign(msize, 0);
            for (std::size_t z = 0; z < msize; ++z) reflect[z] = u8(buf[i + 4 + z]);
        } else if (bf.idx == CNH_DATA_IDX) {
            f.cnh_raw = bytes(buf.begin() + i + 4, buf.begin() + i + 4 + msize);
        }
        i += msize + 4;
    }

    // Real units, as ST GetRangingData (contract 04 §5): distance /4 floored
    // with a negative distance clamped to 0; signal, ambient, sigma and
    // reflectance are scaled where they are moved into the frame below.
    for (auto& d : distance) d = distance_mm_from_raw(d);

    // No target detected -> status 255, over the zones actually present.
    std::size_t nzones = nb_target.size();
    for (std::size_t z = 0; z < nzones; ++z) {
        if (nb_target[z] == 0) {
            for (int t = 0; t < NB_TARGET_PER_ZONE; ++t) {
                std::size_t idx = NB_TARGET_PER_ZONE * z + t;
                if (idx < target_status.size()) target_status[idx] = 255;
            }
        }
    }

    // Header/footer id-match check (footer id offset is variant-specific:
    // FOOTER_ID_OFF_CX=12 for VL53L8CX, FOOTER_ID_OFF_CH=4 for VL53L8CH).
    if (footer_id_off > 0 && drs >= static_cast<std::size_t>(footer_id_off) &&
        drs - footer_id_off + 2 <= drs) {
        std::size_t fo = drs - footer_id_off;
        if (!(u8(buf[8]) == u8(buf[fo]) && u8(buf[9]) == u8(buf[fo + 1]))) {
            return std::nullopt;  // corrupted frame
        }
    }

    f.timestamp_us = 0;  // caller sets from reassembler
    f.resolution = static_cast<int>(nb_target.size());
    f.silicon_temp_degc = silicon_temp;
    f.distance_mm = std::move(distance);
    f.target_status = std::move(target_status);
    f.nb_target_detected = std::move(nb_target);
    f.signal_per_spad = scaled(signal, 2048.0);    // kcps/SPAD
    f.ambient_per_spad = scaled(ambient, 2048.0);  // kcps/SPAD
    f.nb_spads_enabled = std::move(spad);
    f.range_sigma_mm = scaled(sigma, 128.0);       // mm
    f.reflectance = scaled(reflect, 2.0);          // %
    return f;
}

// ── advanced-feature DCI codecs ─────────────────────────────────────────────

std::uint32_t xtalk_margin_raw(std::uint32_t margin_kcps) {
    // as ST: a whole number of kcps, at most 10000; above is refused, never wrapped (contract 07 §3)
    if (margin_kcps > XTALK_MARGIN_MAX_KCPS)
        throw std::out_of_range("xtalk margin above 10000 kcps");
    return margin_kcps * 2048u;
}

std::uint32_t xtalk_margin_kcps(std::uint32_t raw) { return raw / 2048u; }

namespace {
std::int32_t thresh_scale(std::uint8_t meas) {
    switch (meas) {
        case THRESH_DIST_MM: return 4;
        case THRESH_SIGNAL_PER_SPAD_KCPS: return 2048;
        case THRESH_RANGE_SIGMA_MM: return 128;
        case THRESH_AMBIENT_PER_SPAD_KCPS: return 2048;
        case THRESH_NB_SPADS_ENABLED: return 256;
        case THRESH_MOTION_INDICATOR: return 65535;
        default: return 1;
    }
}
void wr_i32(bytes& out, std::int32_t v) {
    detail::wr_le(out, static_cast<std::uint32_t>(v), 4);
}
}  // namespace

bytes pack_detection_thresholds(const std::vector<DetectionThreshold>& thresholds) {
    // never silently dropped, never wrapped (contract 07 §3)
    if (thresholds.size() > static_cast<std::size_t>(NB_THRESHOLDS))
        throw std::invalid_argument("at most 64 detection thresholds");
    for (const DetectionThreshold& t : thresholds) {
        const std::int64_t scale = thresh_scale(t.measurement);
        const std::int64_t lo = t.low_thresh * scale, hi = t.high_thresh * scale;
        if (lo < INT32_MIN || lo > INT32_MAX || hi < INT32_MIN || hi > INT32_MAX)
            throw std::out_of_range("a scaled detection threshold overflows i32");
    }
    bytes out;
    out.reserve(NB_THRESHOLDS * 12);
    for (int k = 0; k < NB_THRESHOLDS; ++k) {
        DetectionThreshold t{};
        if (k < static_cast<int>(thresholds.size())) t = thresholds[k];
        const std::int64_t scale = thresh_scale(t.measurement);
        wr_i32(out, static_cast<std::int32_t>(t.low_thresh * scale));
        wr_i32(out, static_cast<std::int32_t>(t.high_thresh * scale));
        out.push_back(std::byte{t.measurement});
        out.push_back(std::byte{t.type});
        out.push_back(std::byte{t.zone_num});
        out.push_back(std::byte{t.operation});
    }
    return out;
}

bytes detection_thresholds_valid_status() { return bytes(8, std::byte{0x05}); }

bytes MotionConfig::pack() const {
    bytes out;
    out.reserve(156);
    detail::wr_le(out, static_cast<std::uint32_t>(ref_bin_offset), 4);
    detail::wr_le(out, detection_threshold, 4);
    detail::wr_le(out, extra_noise_sigma, 4);
    detail::wr_le(out, null_den_clip_value, 4);
    for (std::uint8_t b : {mem_update_mode, mem_update_choice, sum_span, feature_length,
                           nb_of_aggregates, nb_of_temporal_accumulations,
                           min_nb_for_global_detection, global_indicator_format_1,
                           global_indicator_format_2, spare_1, spare_2, spare_3}) {
        out.push_back(std::byte{b});
    }
    for (std::int8_t v : map_id) out.push_back(std::byte{static_cast<std::uint8_t>(v)});
    for (std::uint8_t v : indicator_format_1) out.push_back(std::byte{v});
    for (std::uint8_t v : indicator_format_2) out.push_back(std::byte{v});
    return out;
}

// ── CNH (Compact Network Histogram) decode ──────────────────────────────────

namespace {

// Signed int32 read at word index `w` (byte offset w*4), matching the Python
// `words = struct.unpack_from('<Ni', raw, 0)` signed-int32 view.
std::int32_t rd_i32(byte_span s, std::size_t off) {
    return static_cast<std::int32_t>(rd_u32(s, off));
}
std::int8_t rd_i8(byte_span s, std::size_t off) {
    return static_cast<std::int8_t>(u8(s[off]));
}

// 1:1 port of cnh._decode_aggregate for the fixed cnh_cfg (ping-pong + variance
// disabled). `words` are the signed-int32 view of the header; byte reads reuse
// `raw` directly for arbitrary offsets.
CnhAggregate decode_aggregate(byte_span raw, int nb_agg, int feat, int agg_id) {
    const std::size_t agg_x_feat = static_cast<std::size_t>(nb_agg) * feat;
    const std::size_t agg_off = static_cast<std::size_t>(agg_id) * feat;

    const std::int32_t state = rd_i32(raw, 0);
    const std::uint32_t info = rd_u32(raw, CNH_PER_HEADER_BUFFER_INFO_IDX * 4);
    const std::uint32_t pp_size = info & CNH_BUFFER_INFO_WORDS_MASK;

    // Select ping or pong buffer exactly as the C code does. With ping-pong
    // disabled the device reports a single buffer and this resolves to ping.
    int local_pp = 1;
    if ((rd_i32(raw, CNH_PER_HEADER_FLAGS_IDX * 4) & 0x10) == 0x10) local_pp = 1;
    if (state == CNH_MI_STATE_PING) local_pp = 1 - local_pp;

    std::size_t base = CNH_PER_HEADER_WORDS;
    if (local_pp == 1) base += pp_size;
    // Buffer header is 2 words (state, nb_accumulated); data starts after it.
    std::size_t blk = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4;  // byte offset of p[2]

    CnhAggregate a;
    // FEAT_INT: int32 per (agg, feat).
    a.hist_raw.resize(feat);
    for (int i = 0; i < feat; ++i)
        a.hist_raw[i] = rd_i32(raw, blk + (agg_off + i) * 4);
    blk += agg_x_feat * 4;
    // FEAT_FRAC: int8 scaler per (agg, feat).
    a.hist_scaler.resize(feat);
    for (int i = 0; i < feat; ++i) a.hist_scaler[i] = rd_i8(raw, blk + agg_off + i);
    blk += ((3 + agg_x_feat) / 4) * 4;
    // AMBIENT_INT: int32 per aggregate.
    a.ambient_raw = rd_i32(raw, blk + static_cast<std::size_t>(agg_id) * 4);
    blk += static_cast<std::size_t>(nb_agg) * 4;
    // AMBIENT_FRAC: int8 scaler per aggregate.
    a.ambient_scaler = rd_i8(raw, blk + static_cast<std::size_t>(agg_id));

    a.hist.resize(feat);
    for (int i = 0; i < feat; ++i)
        a.hist[i] = a.hist_raw[i] / std::pow(2.0, a.hist_scaler[i]);
    a.ambient = a.ambient_raw / std::pow(2.0, a.ambient_scaler);
    return a;
}

}  // namespace

CnhFrame decode_cnh(int nb_of_aggregates, int feature_length, byte_span raw) {
    // Bounds first, as the C decoder does: the reads below index `raw`
    // directly, so a block shorter than this config implies would be read
    // past its end.
    if (nb_of_aggregates <= 0 || nb_of_aggregates > CNH_MAX_AGGREGATES ||
        feature_length < 0 || feature_length > CNH_MAX_FEATURE_LENGTH)
        throw std::invalid_argument("decode_cnh: aggregates / feature length out of range");
    if (raw.size() < static_cast<std::size_t>(CNH_PER_HEADER_WORDS) * 4)
        throw std::length_error("decode_cnh: block shorter than the CNH header");
    {
        const std::size_t agg_x_feat = static_cast<std::size_t>(nb_of_aggregates) * feature_length;
        const std::uint32_t pp_size =
            rd_u32(raw, CNH_PER_HEADER_BUFFER_INFO_IDX * 4) & CNH_BUFFER_INFO_WORDS_MASK;
        int local_pp = 1;
        if (rd_i32(raw, 0) == CNH_MI_STATE_PING) local_pp = 0;
        std::size_t base = CNH_PER_HEADER_WORDS + (local_pp == 1 ? pp_size : 0);
        std::size_t end = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4 + agg_x_feat * 4 +
                          ((3 + agg_x_feat) / 4) * 4 +
                          static_cast<std::size_t>(nb_of_aggregates) * 5;
        if (end > raw.size())
            throw std::length_error("decode_cnh: block shorter than the config implies");
    }
    CnhFrame out;
    out.ref_residual_word = rd_u32(raw, 8);  // words[2]; vl53lmz_cnh_get_ref_residual
    out.ref_residual = out.ref_residual_word / 2048.0;
    out.aggregates.reserve(nb_of_aggregates);
    for (int agg = 0; agg < nb_of_aggregates; ++agg)
        out.aggregates.push_back(
            decode_aggregate(raw, nb_of_aggregates, feature_length, agg));
    return out;
}

MotionConfig motion_config_init(int resolution) {
    if (resolution != RESOLUTION_4X4 && resolution != RESOLUTION_8X8)
        throw std::invalid_argument("motion config: resolution is 16 (4x4) or 64 (8x8)");
    MotionConfig cfg;
    cfg.ref_bin_offset = 13633;
    cfg.detection_threshold = 2883584;
    cfg.extra_noise_sigma = 0;
    cfg.null_den_clip_value = 0;
    cfg.mem_update_mode = 6;
    cfg.mem_update_choice = 2;
    cfg.sum_span = 4;
    cfg.feature_length = 9;
    cfg.nb_of_aggregates = 16;
    cfg.nb_of_temporal_accumulations = 16;
    cfg.min_nb_for_global_detection = 1;
    cfg.global_indicator_format_1 = 8;
    cfg.global_indicator_format_2 = 0;
    if (resolution == RESOLUTION_4X4) {
        for (int i = 0; i < 16; ++i) cfg.map_id[i] = static_cast<std::int8_t>(i);
        for (int i = 16; i < 64; ++i) cfg.map_id[i] = -1;
    } else {
        for (int i = 0; i < 64; ++i)
            cfg.map_id[i] = static_cast<std::int8_t>(((i % 8) / 2) + (4 * (i / 16)));
    }
    return cfg;
}

}  // namespace vl53l8
}  // namespace depz
