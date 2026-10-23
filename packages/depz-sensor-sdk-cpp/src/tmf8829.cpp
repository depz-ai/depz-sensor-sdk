// TMF8829 SPI register bridge (contract 14): a port of the Python SDK's
// protocol/tmf8829.py and tmf8829/{frames,clock,builder,config}.py.
//
// The clock math is integer and bit-exact with the reference: u32
// differences modulo 2^32, signed 32-bit tick deltas, 64-bit µs, floor
// division and floor shifts also for negative operands (C++17's `/`
// truncates toward zero and `>>` of a negative value is
// implementation-defined, so both go through helpers here).
#include "depz/tmf8829.hpp"

#include <cstdio>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace tmf8829 {

using detail::rd_u16;
using detail::rd_u32;
using detail::rd_u64;
using detail::u8;
using detail::wr_le;

namespace {

// floor(a / b) for b > 0.
std::int64_t floor_div(std::int64_t a, std::int64_t b)
{
    std::int64_t q = a / b;
    if (a % b != 0 && a < 0) q--;
    return q;
}

// floor(x / 2^16), i.e. Python's arithmetic x >> 16.
std::int64_t floor_shr16(std::int64_t x)
{
    if (x >= 0) return x >> 16;
    return -static_cast<std::int64_t>(static_cast<std::uint64_t>(-(x + 1)) >> 16) - 1;
}

// u32 -> s32 without the implementation-defined conversion.
std::int64_t s32(std::uint32_t v)
{
    return v >= 0x80000000u ? static_cast<std::int64_t>(v) - 0x100000000LL : static_cast<std::int64_t>(v);
}

std::string hex2(unsigned v)
{
    char b[8];
    std::snprintf(b, sizeof b, "0x%02X", v & 0xFFu);
    return b;
}

std::string hex4(unsigned v)
{
    char b[8];
    std::snprintf(b, sizeof b, "0x%04X", v & 0xFFFFu);
    return b;
}

}  // namespace

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(int addr, int len)
{
    if (addr < 0 || addr > 0xFF) throw std::invalid_argument("addr must be 0..255");
    if (len < 1 || len > static_cast<int>(READ_MAX)) throw std::invalid_argument("length must be 1..1024");
    bytes out;
    wr_le(out, static_cast<std::uint64_t>(addr), 1);
    wr_le(out, static_cast<std::uint64_t>(len), 2);
    return out;
}

bytes pack_write_reg(int addr, byte_span data)
{
    if (addr < 0 || addr > 0xFF) throw std::invalid_argument("addr must be 0..255");
    if (data.size() < 1 || data.size() > WRITE_MAX) throw std::invalid_argument("data must be 1..1023 bytes");
    bytes out;
    out.reserve(1 + data.size());
    wr_le(out, static_cast<std::uint64_t>(addr), 1);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

bytes pack_start_stream(int int_mask, int flags)
{
    if (int_mask < 1 || int_mask > INT_MASK_ALL)
        throw std::invalid_argument("int_mask must be a non-zero subset of 0x0F");
    if (flags < 0 || (flags & ~static_cast<int>(SF_REF_FRAME)))
        throw std::invalid_argument("flags: only SF_REF_FRAME (0x01) is defined");
    bytes out;
    wr_le(out, static_cast<std::uint64_t>(int_mask), 1);
    wr_le(out, static_cast<std::uint64_t>(flags), 1);
    return out;
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<RegData> RegData::unpack(byte_span payload)
{
    // `<BQ` + data
    if (payload.size() < REG_DATA_HEADER_SIZE) return std::nullopt;
    if (u8(payload[0]) != static_cast<std::uint8_t>(Tmf8829Cmd::ReadReg)) return std::nullopt;
    RegData r;
    r.timestamp_us = rd_u64(payload, 1);
    r.data.assign(payload.begin() + REG_DATA_HEADER_SIZE, payload.end());
    return r;
}

std::optional<Tmf8829Info> Tmf8829Info::unpack(byte_span payload)
{
    // `<IIIIIBBBBB`
    if (payload.size() < INFO_SIZE) return std::nullopt;
    Tmf8829Info r;
    r.int_edges = rd_u32(payload, 0);
    r.frames = rd_u32(payload, 4);
    r.frames_aborted = rd_u32(payload, 8);
    r.frame_errors = rd_u32(payload, 12);
    r.spi_timeouts = rd_u32(payload, 16);
    r.int_level = u8(payload[20]);
    r.streaming = u8(payload[21]) != 0;
    r.int_mask = u8(payload[22]);
    r.last_int_status = u8(payload[23]);
    r.last_fifo_status = u8(payload[24]);
    return r;
}

std::optional<FrameChunk> FrameChunk::unpack(byte_span payload)
{
    // `<QHHBQ` + data
    if (payload.size() < FRAME_CHUNK_HEADER_SIZE) return std::nullopt;
    FrameChunk r;
    r.timestamp_us = rd_u64(payload, 0);
    r.full_size = rd_u16(payload, 8);
    r.offset = rd_u16(payload, 10);
    r.frame_seq = u8(payload[12]);
    r.edge_us = rd_u64(payload, 13);
    r.data.assign(payload.begin() + FRAME_CHUNK_HEADER_SIZE, payload.end());
    return r;
}

std::optional<Tmf8829Event> Tmf8829Event::unpack(byte_span payload)
{
    // `<QBBBB`
    if (payload.size() < EVENT_SIZE) return std::nullopt;
    Tmf8829Event r;
    r.timestamp_us = rd_u64(payload, 0);
    r.int_status = u8(payload[8]);
    r.code = u8(payload[9]);
    r.frame_id = u8(payload[10]);
    r.fifo_status = u8(payload[11]);
    return r;
}

// ── chunk reassembly ────────────────────────────────────────────────────────

void FrameReassembler::reset()
{
    completed = discarded = seq_gaps = 0;
    has_head_ = false;
    next_seq_.reset();
    buf_.clear();
}

std::optional<RawFrame> FrameReassembler::feed(const FrameChunk& c)
{
    if (c.offset == 0) {
        if (has_head_) discarded++;  // the previous frame never completed
        if (next_seq_ && c.frame_seq != *next_seq_)
            seq_gaps += static_cast<std::uint8_t>(c.frame_seq - *next_seq_);
        next_seq_ = static_cast<std::uint8_t>(c.frame_seq + 1);
        has_head_ = true;
        head_timestamp_us_ = c.timestamp_us;
        head_edge_us_ = c.edge_us;
        head_full_size_ = c.full_size;
        head_seq_ = c.frame_seq;
        buf_.assign(c.data.begin(), c.data.end());
    } else if (has_head_ && c.frame_seq == head_seq_ && c.full_size == head_full_size_ &&
               c.offset == buf_.size()) {
        buf_.insert(buf_.end(), c.data.begin(), c.data.end());
    } else {
        if (has_head_) discarded++;
        has_head_ = false;
        buf_.clear();
        return std::nullopt;
    }
    if (buf_.size() == head_full_size_) {
        RawFrame f;
        f.timestamp_us = head_timestamp_us_;
        f.edge_us = head_edge_us_;
        f.frame_seq = head_seq_;
        f.data = std::move(buf_);
        buf_.clear();
        has_head_ = false;
        completed++;
        return f;
    }
    if (buf_.size() > head_full_size_) {
        discarded++;
        has_head_ = false;
        buf_.clear();
    }
    return std::nullopt;
}

// ── frames ──────────────────────────────────────────────────────────────────

namespace {

struct Grid {
    unsigned rows, cols;
    const char* name;
};

constexpr std::array<Grid, 6> FP_GRID = {{
    {8, 8, "8x8A"}, {8, 8, "8x8B"}, {16, 16, "16x16"}, {32, 32, "32x32"}, {32, 32, "32x32s"}, {32, 48, "48x32"},
}};

}  // namespace

std::optional<std::pair<unsigned, unsigned>> fp_grid(unsigned fp_mode)
{
    if (fp_mode >= FP_GRID.size()) return std::nullopt;
    return std::make_pair(FP_GRID[fp_mode].rows, FP_GRID[fp_mode].cols);
}

std::optional<std::pair<unsigned, unsigned>> frame_rows(unsigned fp_mode)
{
    auto g = fp_grid(fp_mode);
    if (!g) return std::nullopt;
    return std::make_pair(g->first == 32 ? g->first / 2 : g->first, g->second);
}

std::string fp_name(unsigned fp_mode) { return fp_mode < FP_GRID.size() ? FP_GRID[fp_mode].name : ""; }

std::vector<std::string> FrameInfo::problems() const
{
    std::vector<std::string> out;
    if (eof != FRAME_EOF) out.push_back("EOF " + hex4(eof));
    if (!(frame_status & FRAME_VALID)) out.push_back("status " + hex2(frame_status) + " not valid");
    if (frame_status & FRAME_ABORTED) out.push_back("aborted");
    if (frame_status & FRAME_HV_CP_OVERLOAD) out.push_back("HV_CP_OVERLOAD");
    if (frame_status & FRAME_VCDRV_OVERLOAD) out.push_back("VCDRV_OVERLOAD");
    if (frame_status & FRAME_VCDRV_BURST_EXCEEDED) out.push_back("VCDRV_BURST_EXCEEDED");
    if (fifo_status & FIFO_TX_UNDERRUN) out.push_back("txfifo_underrun");
    if (fifo_status & FIFO_RX_OVERRUN) out.push_back("rxfifo_overrun");
    return out;
}

std::size_t frame_size(byte_span head)
{
    if (head.size() < 9) throw std::invalid_argument("frame_size: need the first 21 frame bytes");
    return PRE_HEADER_SIZE + 4 + rd_u16(head, 7);
}

FrameInfo parse_frame(byte_span raw)
{
    if (raw.size() < FRAME_HEAD + FOOTER_SIZE)
        throw FrameError("frame of " + std::to_string(raw.size()) + " B is shorter than header + footer");
    FrameInfo f;
    f.fifo_status = u8(raw[0]);
    f.systick = rd_u32(raw, 1);
    const std::size_t h = PRE_HEADER_SIZE;
    f.id = u8(raw[h]);
    f.layout = u8(raw[h + 1]);
    f.payload = rd_u16(raw, h + 2);
    f.f_number = rd_u32(raw, h + 4);
    for (std::size_t i = 0; i < 3; ++i) {
        const int v = u8(raw[h + 8 + i]);
        f.temperature[i] = static_cast<std::int8_t>(v >= 0x80 ? v - 0x100 : v);
    }
    f.bdv = u8(raw[h + 11]);
    f.ref_pos[0] = rd_u16(raw, h + 12);
    f.ref_pos[1] = rd_u16(raw, h + 14);
    const std::size_t t = raw.size() - FOOTER_SIZE;
    f.t0_integration = rd_u32(raw, t);
    f.t1_integration = rd_u32(raw, t + 4);
    f.frame_status = u8(raw[t + 8]);
    f.reserved = u8(raw[t + 9]);
    f.eof = rd_u16(raw, t + 10);
    f.size = raw.size();
    return f;
}

// ── results pixels ──────────────────────────────────────────────────────────

std::pair<unsigned, unsigned> PixelResults::shape() const
{
    if (distance || noise || xtalk) return {rows, cols};
    return {0, 0};
}

PixelResults parse_results(byte_span raw, const std::optional<FrameInfo>& info)
{
    const FrameInfo f = info ? *info : parse_frame(raw);
    if (f.kind() != FID_RESULTS) throw FrameError("frame id " + hex2(f.id) + " is not a results frame");
    const auto rc = frame_rows(f.fp_mode());
    if (!rc) throw FrameError("unknown fp_mode " + std::to_string(f.fp_mode()));
    const unsigned rows = rc->first, cols = rc->second, n = rows * cols;
    const unsigned peaks = f.layout & LAYOUT_PEAKS;
    const bool sig = (f.layout & LAYOUT_SIGNAL) != 0;
    const bool noise = (f.layout & LAYOUT_NOISE) != 0;
    const bool xtalk = (f.layout & LAYOUT_XTALK) != 0;
    // [noise u16] [xtalk u16] then per peak: distance u16, snr u8 [, signal u16]
    const std::size_t item = (noise ? 2u : 0u) + (xtalk ? 2u : 0u) + peaks * (3u + (sig ? 2u : 0u));
    const std::size_t data_size = f.size - FRAME_HEAD - FOOTER_SIZE;
    if (item * n != data_size || raw.size() < FRAME_HEAD + data_size) {
        throw FrameError("result data " + std::to_string(data_size) + " B, expected " + std::to_string(n) +
                         " px \xC3\x97 " + std::to_string(item) + " B (layout " + hex2(f.layout) + ")");
    }
    PixelResults r;
    r.peaks = peaks;
    r.rows = rows;
    r.cols = cols;
    if (peaks) {
        r.distance.emplace(static_cast<std::size_t>(peaks) * n);
        r.snr.emplace(static_cast<std::size_t>(peaks) * n);
        if (sig) r.signal.emplace(static_cast<std::size_t>(peaks) * n);
    }
    if (noise) r.noise.emplace(n);
    if (xtalk) r.xtalk.emplace(n);
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t at = FRAME_HEAD + i * item;
        if (noise) { (*r.noise)[i] = rd_u16(raw, at); at += 2; }
        if (xtalk) { (*r.xtalk)[i] = rd_u16(raw, at); at += 2; }
        for (std::size_t p = 0; p < peaks; ++p) {
            (*r.distance)[p * n + i] = rd_u16(raw, at);
            (*r.snr)[p * n + i] = u8(raw[at + 2]);
            at += 3;
            if (sig) { (*r.signal)[p * n + i] = rd_u16(raw, at); at += 2; }
        }
    }
    return r;
}

namespace {

// Interleave rows of [planes][rows][cols] arrays: a -> even rows, b -> odd.
template <class T>
std::optional<std::vector<T>> merge_rows(const std::optional<std::vector<T>>& a,
                                         const std::optional<std::vector<T>>& b, unsigned rows, unsigned cols)
{
    if (!a || !b) return std::nullopt;
    const std::size_t half = static_cast<std::size_t>(rows) * cols;
    const std::size_t planes = half ? a->size() / half : 0;
    std::vector<T> m(2 * half * planes);
    for (std::size_t p = 0; p < planes; ++p) {
        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                m[p * 2 * half + (2 * r) * cols + c] = (*a)[p * half + r * cols + c];
                m[p * 2 * half + (2 * r + 1) * cols + c] = (*b)[p * half + r * cols + c];
            }
        }
    }
    return m;
}

}  // namespace

PixelResults merge_halves(const PixelResults& first, const PixelResults& second)
{
    if (first.peaks != second.peaks || first.rows != second.rows || first.cols != second.cols)
        throw std::invalid_argument("merge_halves: the halves differ in peaks or shape");
    const unsigned rows = first.rows, cols = first.cols;
    PixelResults m;
    m.peaks = first.peaks;
    m.rows = 2 * rows;
    m.cols = cols;
    m.distance = merge_rows(first.distance, second.distance, rows, cols);
    m.snr = merge_rows(first.snr, second.snr, rows, cols);
    m.signal = merge_rows(first.signal, second.signal, rows, cols);
    m.noise = merge_rows(first.noise, second.noise, rows, cols);
    m.xtalk = merge_rows(first.xtalk, second.xtalk, rows, cols);
    return m;
}

// ── clock model ─────────────────────────────────────────────────────────────

std::vector<std::string> flag_names(std::uint8_t flags)
{
    static const std::pair<std::uint8_t, const char*> NAMES[] = {
        {FT_NOMINAL, "nominal"}, {FT_CLOCK_RESET, "clock_reset"}, {FT_EDGE_LATE, "edge_late"},
        {FT_STALLED, "stalled"}, {FT_T1_UNSURE, "t1_unsure"},
    };
    std::vector<std::string> out;
    for (const auto& n : NAMES)
        if (flags & n.first) out.emplace_back(n.second);
    return out;
}

unsigned subints(unsigned fp_mode)
{
    static constexpr unsigned N[6] = {2, 2, 2, 4, 4, 6};
    return fp_mode < 6 ? N[fp_mode] : 2;
}

void SensorClock::reset()
{
    valid_ = false;
    last_us_ = anchor_us_ = mid_us_ = 0;
    last_tick_ = anchor_tick_ = mid_tick_ = 0;
    has_mid_ = false;
    ratio_q16_ = NOMINAL_Q16;
    nominal_ = true;
}

std::uint8_t SensorClock::add(std::int64_t us, std::uint32_t tick)
{
    if (us < 0) throw std::out_of_range("SensorClock: an MCU µs time is a u64");
    std::uint8_t flags = 0;
    if (valid_) {
        const std::int64_t d_us = us - last_us_;
        const std::int64_t expect = static_cast<std::int64_t>(static_cast<std::uint32_t>(tick - last_tick_)) * 8;
        const std::int64_t tol = floor_div(d_us, 10) + 50;
        if (!(expect <= d_us + tol && expect + tol >= d_us)) {
            valid_ = false;
            flags = FT_CLOCK_RESET;
        }
    }
    last_us_ = us;
    last_tick_ = tick;
    if (!valid_) {
        valid_ = true;
        anchor_us_ = us;
        anchor_tick_ = tick;
        has_mid_ = false;
        ratio_q16_ = NOMINAL_Q16;
        nominal_ = true;
        return flags;
    }
    const std::int64_t span = us - anchor_us_;
    if (span >= CLOCK_WINDOW_MIN_US) {
        const std::uint32_t dt = static_cast<std::uint32_t>(tick - anchor_tick_);
        // dt == 0 over >= 200 ms would have failed the step check above.
        if (dt) {
            ratio_q16_ = static_cast<std::uint32_t>((static_cast<std::uint64_t>(span) << 16) / dt);
            nominal_ = false;
        }
    }
    if (!has_mid_ && span >= CLOCK_WINDOW_SLIDE_US) {
        has_mid_ = true;
        mid_us_ = us;
        mid_tick_ = tick;
    } else if (has_mid_ && span >= 2 * CLOCK_WINDOW_SLIDE_US) {
        anchor_us_ = mid_us_;
        anchor_tick_ = mid_tick_;
        has_mid_ = false;
    }
    return flags;
}

std::int64_t SensorClock::to_us(std::int64_t pair_us, std::uint32_t pair_tick, std::uint32_t tick) const
{
    return pair_us + floor_shr16(s32(static_cast<std::uint32_t>(tick - pair_tick)) *
                                 static_cast<std::int64_t>(ratio_q16_));
}

// ── capture time ────────────────────────────────────────────────────────────

void CaptureTimer::reset()
{
    lag_min_.reset();
    hist_seen_ = false;
    sub_min_ = 0;
}

FrameTime CaptureTimer::results(const SensorClock& clock, unsigned fp_mode, std::uint32_t systick,
                                std::uint32_t t0_tick, std::uint32_t t1_tick, std::int64_t pair_us,
                                std::int64_t edge_us, std::uint8_t flags)
{
    // fp_mode is a byte, MCU µs times are u64 (contract 07 §3)
    if (fp_mode > 0xFFu) throw std::out_of_range("CaptureTimer: fp_mode is a byte");
    if (pair_us < 0 || edge_us < 0) throw std::out_of_range("CaptureTimer: an MCU µs time is a u64");
    if (clock.nominal()) flags |= FT_NOMINAL;
    const std::int64_t t0 = clock.to_us(pair_us, systick, t0_tick);
    const std::int64_t t1 = clock.to_us(pair_us, systick, t1_tick);

    if (edge_us != 0 && !hist_seen_) {
        const std::int64_t lag = edge_us - t1;
        if (lag <= 0 || (lag_min_ && lag > *lag_min_ + EDGE_LATE_US)) {
            flags |= FT_EDGE_LATE;
        } else if (!lag_min_ || lag < *lag_min_) {
            lag_min_ = lag;
        }
    }

    const std::uint32_t n1 = subints(fp_mode) - 1;
    const std::uint32_t sub = static_cast<std::uint32_t>(t1_tick - t0_tick) / n1;
    if (!sub_min_ || sub < sub_min_) sub_min_ = sub;
    if (static_cast<std::uint64_t>(sub) > static_cast<std::uint64_t>(sub_min_) + 1) flags |= FT_STALLED;
    if (hist_seen_ && n1 > 1) flags |= FT_T1_UNSURE;

    const auto t1_len =
        static_cast<std::uint32_t>((static_cast<std::uint64_t>(sub_min_) * clock.ratio_q16()) >> 16);
    FrameTime t;
    t.timestamp_us = floor_div(t0 + t1 + static_cast<std::int64_t>(t1_len), 2);
    t.flags = flags;
    t.t1_len_us = t1_len;
    t.t0_us = t0;
    t.t1_us = t1;
    t.edge_us = edge_us;
    t.pair_us = pair_us;
    t.ratio_q16 = clock.ratio_q16();
    return t;
}

FrameTime CaptureTimer::results_frame(const SensorClock& clock, const FrameInfo& f, std::int64_t pair_us,
                                      std::int64_t edge_us, std::uint8_t flags)
{
    return results(clock, f.fp_mode(), f.systick, f.t0_integration, f.t1_integration, pair_us, edge_us, flags);
}

// ── measurement assembly ────────────────────────────────────────────────────

std::pair<unsigned, unsigned> Measurement::shape() const
{
    auto g = fp_grid(fp_mode);
    return g ? *g : std::make_pair(0u, 0u);
}

std::vector<double> distance_mm(const PixelResults& results, double ratio, bool in_mm)
{
    std::vector<double> out;
    if (!results.distance) return out;
    out.reserve(results.distance->size());
    for (std::uint16_t raw : *results.distance) {
        const double d = static_cast<double>(raw) * ratio;
        out.push_back(in_mm ? d : d / 4.0);
    }
    return out;
}

void MeasurementBuilder::reset()
{
    clock.reset();
    timer.reset();
    pending_.clear();
    hist_.clear();
    refs_.clear();
    half_.reset();
    problems_.clear();
    parse_errors = 0;
}

std::optional<Measurement> MeasurementBuilder::add(byte_span raw, std::int64_t timestamp_us, std::int64_t edge_us)
{
    // MCU µs times are u64 (contract 07 §3): checked before the frame is parsed
    if (timestamp_us < 0 || edge_us < 0) throw std::out_of_range("MeasurementBuilder: an MCU µs time is a u64");
    FrameInfo f;
    try {
        f = parse_frame(raw);
    } catch (const FrameError& e) {
        parse_errors++;
        problems_.emplace_back(e.what());
        return std::nullopt;
    }
    if (f.kind() == FID_REF_SPAD) {
        pending_.emplace_back(f, bytes(raw.begin(), raw.end()));  // zero pre-header: no clock pair
        return std::nullopt;
    }
    if (f.kind() != FID_RESULTS && f.kind() != FID_HISTOGRAMS) {
        parse_errors++;
        problems_.push_back("frame id " + hex2(f.id) + ": unknown kind");
        return std::nullopt;
    }
    const std::uint8_t flags = clock.add(timestamp_us, f.systick);
    const auto bad = f.problems();
    if (!bad.empty()) {
        std::string joined;
        for (std::size_t k = 0; k < bad.size(); ++k) joined += (k ? ", " : "") + bad[k];
        problems_.push_back("frame id=" + hex2(f.id) + " fN=" + std::to_string(f.f_number) + " " +
                            std::to_string(f.size) + " B: " + joined);
    }
    if (f.kind() == FID_HISTOGRAMS) {
        timer.histograms();
        pending_.emplace_back(f, bytes(raw.begin(), raw.end()));
        return std::nullopt;
    }

    const FrameTime t = timer.results_frame(clock, f, timestamp_us, edge_us, flags);
    for (auto& p : pending_) {
        if (p.first.kind() == FID_HISTOGRAMS) hist_.push_back(HistogramFrame{p.first, std::move(p.second)});
        else refs_.push_back(std::move(p.second));
    }
    pending_.clear();
    PixelResults res;
    try {
        res = parse_results(raw, f);
    } catch (const FrameError& e) {
        parse_errors++;
        problems_.push_back("fN=" + std::to_string(f.f_number) + ": " + e.what());
        half_.reset();
        return std::nullopt;
    }

    Measurement m;
    if (fp_grid(f.fp_mode())->first == 32) {
        if (!half_) {
            if (f.is_second_half()) {
                problems_.push_back("second half fN=" + std::to_string(f.f_number) + " without a first one - dropped");
                hist_.clear();
                refs_.clear();
                return std::nullopt;
            }
            half_ = Half{f, std::move(res), t};
            return std::nullopt;
        }
        if (!f.is_second_half()) {
            problems_.push_back("two first halves in a row (fN " + std::to_string(half_->info.f_number) + ", " +
                                std::to_string(f.f_number) + ")");
            half_ = Half{f, std::move(res), t};
            return std::nullopt;
        }
        Half h = std::move(*half_);
        half_.reset();
        if (h.info.fp_mode() != f.fp_mode() ||
            (h.info.layout & ~LAYOUT_SUBFRAME) != (f.layout & ~LAYOUT_SUBFRAME)) {
            // Impossible within one stream (configure() needs a stop); never crash the reader.
            problems_.push_back("half-frames of different formats (fN " + std::to_string(h.info.f_number) + ", " +
                                std::to_string(f.f_number) + ") - dropped");
            hist_.clear();
            refs_.clear();
            return std::nullopt;
        }
        m.results = merge_halves(h.res, res);
        m.frames = {h.info, f};
        m.times = {h.time, t};
    } else {
        m.results = std::move(res);
        m.frames = {f};
        m.times = {t};
    }

    std::int64_t sum = 0;
    for (const auto& x : m.times) {
        sum += x.timestamp_us;
        m.flags |= x.flags;
    }
    m.timestamp_us = floor_div(sum, static_cast<std::int64_t>(m.times.size()));
    m.fp_mode = f.fp_mode();
    m.f_number = f.f_number;
    m.ratio_q16 = clock.ratio_q16();
    m.distance_mm = distance_mm(m.results, clock.ratio(), f.distance_in_mm());
    m.histograms = std::move(hist_);
    m.ref_frames = std::move(refs_);
    m.n_histograms = m.histograms.size();
    m.n_ref_frames = m.ref_frames.size();
    m.problems = std::move(problems_);
    m.n_problems = m.problems.size();
    hist_.clear();
    refs_.clear();
    problems_.clear();
    return m;
}

// ── presets, config page ────────────────────────────────────────────────────

std::string status_name(std::uint8_t st)
{
    static const char* const NAMES[] = {
        "OK", "ACCEPTED", "ERR_CONFIG", "ERR_APPLICATION", "ERR_CONFIG_RESULT_SIZE", "ERR_CONFIG_VCSEL",
        "ERR_WAKEUP_TIMED", "ERR_RESET_UNEXPECTED", "ERR_UNKNOWN_CMD", "ERR_UNKNOWN_CID", "ERR_STOP_0",
        "ERR_STOP_1", "ERR_STOP_2", "ERR_STOP_3", "ERR_OSC_TUNE",
    };
    return st < sizeof NAMES / sizeof NAMES[0] ? NAMES[st] : "";
}

std::optional<Preset> find_preset(const std::string& name)
{
    for (const auto& p : PRESETS)
        if (name == p.name) return p;
    return std::nullopt;
}

std::optional<CfgField> find_cfg_field(const std::string& name)
{
    for (const auto& f : CFG_FIELDS)
        if (name == f.name) return f;
    return std::nullopt;
}

namespace {

CfgField field_or_throw(const std::string& name)
{
    auto f = find_cfg_field(name);
    if (!f) throw std::out_of_range("unknown config field " + name);
    return *f;
}

// Index of the field's first byte; std::invalid_argument when the page (a
// whole page or a partial read from its start) is too short to hold it.
std::size_t field_index(std::size_t page_size, const CfgField& f)
{
    const std::size_t i = static_cast<std::size_t>(f.reg - REG_CFG);
    if (page_size < i + (f.width == 16 ? 2u : 1u))
        throw std::invalid_argument(std::string(f.name) + ": config page too short for the field");
    return i;
}

std::uint32_t field_get(byte_span page, const CfgField& f)
{
    const std::size_t i = field_index(page.size(), f);
    if (f.width == 16) return rd_u16(page, i);
    return (static_cast<std::uint32_t>(u8(page[i])) >> f.shift) & ((1u << f.width) - 1u);
}

}  // namespace

std::uint32_t cfg_get(byte_span page, const std::string& name)
{
    const CfgField f = field_or_throw(name);
    return field_get(page, f);
}

void cfg_set(bytes& page, const std::string& name, std::int64_t value)
{
    const CfgField f = field_or_throw(name);
    const std::size_t i = field_index(page.size(), f);
    if (value < 0 || value >= (std::int64_t{1} << f.width))
        throw std::invalid_argument(name + ": " + std::to_string(value) + " out of range for " +
                                    std::to_string(f.width) + " bit");
    if (f.width == 16) {
        page[i] = std::byte{static_cast<std::uint8_t>(value & 0xFF)};
        page[i + 1] = std::byte{static_cast<std::uint8_t>(value >> 8)};
    } else {
        const unsigned mask = ((1u << f.width) - 1u) << f.shift;
        const unsigned v = (u8(page[i]) & ~mask) | (static_cast<unsigned>(value) << f.shift);
        page[i] = std::byte{static_cast<std::uint8_t>(v)};
    }
}

std::vector<std::pair<std::string, std::uint32_t>> config_fields(byte_span page)
{
    std::vector<std::pair<std::string, std::uint32_t>> out;
    out.reserve(CFG_FIELDS.size());
    for (const auto& f : CFG_FIELDS) out.emplace_back(f.name, field_get(page, f));
    return out;
}

}  // namespace tmf8829
}  // namespace depz
