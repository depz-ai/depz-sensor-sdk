#include "depz/tmf8829.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace detail {

tmf8829::PixelResults tmf8829_results_from_c(const depz_tmf8829_results& c) {
    tmf8829::PixelResults r;
    const std::size_t zones = static_cast<std::size_t>(c.rows) * c.cols, n = c.peaks * zones;
    r.peaks = c.peaks;
    r.rows = c.rows;
    r.cols = c.cols;
    if (c.peaks) {
        r.distance = std::vector<std::uint16_t>(c.distance, c.distance + n);
        r.snr = std::vector<std::uint8_t>(c.snr, c.snr + n);
    }
    if (c.has_signal) r.signal = std::vector<std::uint16_t>(c.signal, c.signal + n);
    if (c.has_noise) r.noise = std::vector<std::uint16_t>(c.noise, c.noise + zones);
    if (c.has_xtalk) r.xtalk = std::vector<std::uint16_t>(c.xtalk, c.xtalk + zones);
    return r;
}

tmf8829::Measurement tmf8829_measurement_from_c(const depz_tmf8829_measurement& c) {
    tmf8829::Measurement m;
    m.timestamp_us = c.timestamp_us;
    m.flags = c.flags;
    m.fp_mode = c.fp_mode;
    m.f_number = c.f_number;
    m.ratio_q16 = c.ratio_q16;
    m.results = tmf8829_results_from_c(c.results);
    if (c.results.peaks) {
        const std::size_t n = static_cast<std::size_t>(c.results.peaks) * c.results.rows * c.results.cols;
        m.distance_mm.assign(c.distance_mm, c.distance_mm + n);
    }
    for (std::size_t i = 0; i < c.n_frames && i < 2; ++i) {
        tmf8829::FrameInfo f;
        static_cast<depz_tmf8829_frame_info&>(f) = c.frames[i];
        m.frames.push_back(f);
        m.times.push_back(c.times[i]);
    }
    m.n_histograms = c.n_histograms;
    m.n_ref_frames = c.n_ref_frames;
    m.histograms.reserve(c.n_histograms);
    for (std::size_t i = 0; c.histograms && i < c.n_histograms; ++i) {
        tmf8829::HistogramFrame h;
        static_cast<depz_tmf8829_frame_info&>(h.info) = c.histograms[i].info;
        const std::byte* raw = reinterpret_cast<const std::byte*>(c.histograms[i].raw);
        h.raw.assign(raw, raw + c.histograms[i].info.size);
        m.histograms.push_back(std::move(h));
    }
    m.ref_frames.reserve(c.n_ref_frames);
    for (std::size_t i = 0; c.ref_frames && i < c.n_ref_frames; ++i) {
        const std::byte* raw = reinterpret_cast<const std::byte*>(c.ref_frames[i].raw);
        m.ref_frames.emplace_back(raw, raw + c.ref_frames[i].len);
    }
    m.n_problems = c.n_problems;
    const std::size_t kept = std::min<std::size_t>(c.n_problems, DEPZ_TMF8829_PROBLEMS_KEPT);
    for (std::size_t i = 0; i < kept; ++i) m.problems.emplace_back(c.problems[i]);
    return m;
}

// The C view of decoded results; the C decoder's arrays are sized by the
// sensor's limits, so a PixelResults past them is a bad argument.
std::unique_ptr<depz_tmf8829_results> tmf8829_results_to_c(const tmf8829::PixelResults& r) {
    const std::size_t zones = static_cast<std::size_t>(r.rows) * r.cols, n = r.peaks * zones;
    if (r.peaks > DEPZ_TMF8829_MAX_PEAKS || zones > DEPZ_TMF8829_MAX_ZONES)
        throw std::invalid_argument("results exceed the sensor's peaks x zones");
    auto size_is = [](const auto& v, std::size_t want) { return !v || v->size() == want; };
    if (!size_is(r.distance, n) || !size_is(r.snr, n) || !size_is(r.signal, n) || !size_is(r.noise, zones) ||
        !size_is(r.xtalk, zones))
        throw std::invalid_argument("results arrays do not match peaks x rows x cols");
    auto c = std::make_unique<depz_tmf8829_results>();
    std::memset(c.get(), 0, sizeof *c);
    c->peaks = static_cast<std::uint8_t>(r.peaks);
    c->rows = static_cast<std::uint16_t>(r.rows);
    c->cols = static_cast<std::uint16_t>(r.cols);
    if (r.distance) std::copy(r.distance->begin(), r.distance->end(), c->distance);
    if (r.snr) std::copy(r.snr->begin(), r.snr->end(), c->snr);
    if ((c->has_signal = r.signal.has_value())) std::copy(r.signal->begin(), r.signal->end(), c->signal);
    if ((c->has_noise = r.noise.has_value())) std::copy(r.noise->begin(), r.noise->end(), c->noise);
    if ((c->has_xtalk = r.xtalk.has_value())) std::copy(r.xtalk->begin(), r.xtalk->end(), c->xtalk);
    return c;
}

}  // namespace detail

namespace tmf8829 {

using detail::packed;
using detail::to_bytes;
using detail::u8p;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(int addr, int len) {
    std::uint8_t out[3];
    return packed(out, depz_tmf8829_pack_read_reg(addr, len, out), "TMF_READ_REG: addr must be 0..255, len 1..1024");
}

bytes pack_write_reg(int addr, byte_span data) {
    bytes out(1 + data.size());
    const std::size_t n = depz_tmf8829_pack_write_reg(addr, u8p(data), data.size(),
                                                      reinterpret_cast<std::uint8_t*>(out.data()));
    if (n == 0) throw std::invalid_argument("TMF_WRITE_REG: addr must be 0..255, data 1..1023 bytes");
    out.resize(n);
    return out;
}

bytes pack_start_stream(int int_mask, int flags) {
    std::uint8_t out[2];
    return packed(out, depz_tmf8829_pack_start_stream(int_mask, flags, out),
                  "TMF_START_STREAM: int_mask must be a non-zero subset of 0x0F, flags only SF_REF_FRAME (0x01)");
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<RegData> RegData::unpack(byte_span payload) {
    depz_tmf8829_reg_data c{};
    if (depz_tmf8829_unpack_reg_data(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return RegData{c.timestamp_us, to_bytes(c.data, c.data_len)};
}

std::optional<Tmf8829Info> Tmf8829Info::unpack(byte_span payload) {
    Tmf8829Info i{};
    if (depz_tmf8829_unpack_info(u8p(payload), payload.size(), &i) != 0) return std::nullopt;
    return i;
}

std::optional<FrameChunk> FrameChunk::unpack(byte_span payload) {
    depz_tmf8829_frame_chunk c{};
    if (depz_tmf8829_unpack_frame_chunk(u8p(payload), payload.size(), &c) != 0) return std::nullopt;
    return FrameChunk{c.timestamp_us, c.full_size, c.offset, c.frame_seq, c.edge_us, to_bytes(c.data, c.data_len)};
}

std::optional<Tmf8829Event> Tmf8829Event::unpack(byte_span payload) {
    Tmf8829Event e{};
    if (depz_tmf8829_unpack_event(u8p(payload), payload.size(), &e) != 0) return std::nullopt;
    return e;
}

// ── chunk reassembly ────────────────────────────────────────────────────────

FrameReassembler::FrameReassembler() : r_(std::make_unique<depz_tmf8829_reassembler>()) { reset(); }

void FrameReassembler::reset() { depz_tmf8829_reassembler_reset(r_.get()); }

std::optional<RawFrame> FrameReassembler::feed(const FrameChunk& chunk) {
    depz_tmf8829_frame_chunk c{};
    c.timestamp_us = chunk.timestamp_us;
    c.full_size = chunk.full_size;
    c.offset = chunk.offset;
    c.frame_seq = chunk.frame_seq;
    c.edge_us = chunk.edge_us;
    c.data = u8p(chunk.data);
    c.data_len = chunk.data.size();
    depz_tmf8829_raw_frame f{};
    if (depz_tmf8829_reassembler_feed(r_.get(), &c, &f) != 1) return std::nullopt;
    return RawFrame{f.timestamp_us, f.edge_us, f.frame_seq, to_bytes(f.data, f.len)};
}

// ── frames ──────────────────────────────────────────────────────────────────

std::optional<std::pair<unsigned, unsigned>> fp_grid(unsigned fp_mode) {
    unsigned rows = 0, cols = 0;
    if (depz_tmf8829_fp_grid(fp_mode, &rows, &cols) != 0) return std::nullopt;
    return std::make_pair(rows, cols);
}

std::optional<std::pair<unsigned, unsigned>> frame_rows(unsigned fp_mode) {
    unsigned rows = 0, cols = 0;
    if (depz_tmf8829_frame_rows(fp_mode, &rows, &cols) != 0) return std::nullopt;
    return std::make_pair(rows, cols);
}

std::string fp_name(unsigned fp_mode) {
    const char* n = depz_tmf8829_fp_name(fp_mode);
    return n ? n : "";
}

std::vector<std::string> FrameInfo::problems() const {
    depz_tmf8829_frame_problems p{};
    depz_tmf8829_frame_problems_of(this, &p);
    std::vector<std::string> out;
    for (int i = 0; i < p.count; ++i) out.emplace_back(p.text[i]);
    return out;
}

std::size_t frame_size(byte_span head) {
    if (head.size() < 9) throw std::invalid_argument("frame_size: need the first 21 frame bytes");
    std::uint8_t h[FRAME_HEAD] = {0};
    std::memcpy(h, head.data(), std::min<std::size_t>(head.size(), FRAME_HEAD));
    return depz_tmf8829_frame_size(h);
}

FrameInfo parse_frame(byte_span raw) {
    FrameInfo f{};
    if (depz_tmf8829_parse_frame(u8p(raw), raw.size(), &f) != 0)
        throw FrameError("frame of " + std::to_string(raw.size()) + " B is shorter than header + footer");
    return f;
}

// ── results pixels and the half-frame merge ─────────────────────────────────

std::pair<unsigned, unsigned> PixelResults::shape() const {
    if (distance || noise || xtalk) return {rows, cols};
    return {0, 0};
}

namespace {

}  // namespace

PixelResults parse_results(byte_span raw, const std::optional<FrameInfo>& info) {
    auto c = std::make_unique<depz_tmf8829_results>();
    char err[128];
    if (depz_tmf8829_parse_results(u8p(raw), raw.size(), info ? &*info : nullptr, c.get(), err, sizeof err) != 0)
        throw FrameError(err);
    return detail::tmf8829_results_from_c(*c);
}

PixelResults merge_halves(const PixelResults& first, const PixelResults& second) {
    const auto a = detail::tmf8829_results_to_c(first), b = detail::tmf8829_results_to_c(second);
    auto out = std::make_unique<depz_tmf8829_results>();
    if (depz_tmf8829_merge_halves(a.get(), b.get(), out.get()) != 0)
        throw std::invalid_argument("merge_halves: the halves differ in peaks or shape");
    return detail::tmf8829_results_from_c(*out);
}

// ── clock model and capture time ────────────────────────────────────────────

std::vector<std::string> flag_names(std::uint8_t flags) {
    static const std::pair<std::uint8_t, const char*> NAMES[] = {
        {FT_NOMINAL, "nominal"}, {FT_CLOCK_RESET, "clock_reset"}, {FT_EDGE_LATE, "edge_late"},
        {FT_STALLED, "stalled"}, {FT_T1_UNSURE, "t1_unsure"},
    };
    std::vector<std::string> out;
    for (const auto& n : NAMES)
        if (flags & n.first) out.emplace_back(n.second);
    return out;
}

unsigned subints(unsigned fp_mode) { return depz_tmf8829_subints(fp_mode); }

std::uint8_t SensorClock::add(std::int64_t us, std::uint32_t tick) {
    std::uint8_t flags = 0;
    if (depz_tmf8829_clock_add_checked(c_, us, tick, &flags) != 0)
        throw std::out_of_range("SensorClock: an MCU µs time is a u64");
    return flags;
}

FrameTime CaptureTimer::results(const SensorClock& clock, unsigned fp_mode, std::uint32_t systick,
                                std::uint32_t t0_tick, std::uint32_t t1_tick, std::int64_t pair_us,
                                std::int64_t edge_us, std::uint8_t flags) {
    FrameTime t{};
    if (depz_tmf8829_timer_results_checked(t_, clock.c_, fp_mode, systick, t0_tick, t1_tick, pair_us, edge_us,
                                           flags, &t) != 0)
        throw std::out_of_range("CaptureTimer: fp_mode is a byte and an MCU µs time is a u64");
    return t;
}

// ── measurements ────────────────────────────────────────────────────────────

std::pair<unsigned, unsigned> Measurement::shape() const {
    auto g = fp_grid(fp_mode);
    return g ? *g : std::make_pair(0u, 0u);
}

std::vector<double> distance_mm(const PixelResults& results, double ratio, bool in_mm) {
    std::vector<double> out;
    if (!results.distance) return out;
    out.reserve(results.distance->size());
    for (std::uint16_t raw : *results.distance) {
        const double d = static_cast<double>(raw) * ratio;
        out.push_back(in_mm ? d : d / 4.0);
    }
    return out;
}

MeasurementBuilder::MeasurementBuilder()
    : b_(std::make_unique<depz_tmf8829_builder>()), clock(&b_->clock), timer(&b_->timer) {
    depz_tmf8829_builder_init(b_.get());
}

MeasurementBuilder::~MeasurementBuilder() { depz_tmf8829_builder_free(b_.get()); }

void MeasurementBuilder::reset() { depz_tmf8829_builder_reset(b_.get()); }

std::optional<Measurement> MeasurementBuilder::add(byte_span raw, std::int64_t timestamp_us, std::int64_t edge_us) {
    auto out = std::make_unique<depz_tmf8829_measurement>();
    const int rc = depz_tmf8829_builder_add(b_.get(), u8p(raw), raw.size(), timestamp_us, edge_us, out.get());
    // MCU µs times are u64 (contract 07 §3): refused before the frame is parsed
    if (rc < 0) throw std::out_of_range("MeasurementBuilder: an MCU µs time is a u64");
    if (rc == 0) return std::nullopt;
    // The histogram / ref-SPAD frames point into the C builder (valid until
    // its next add): copied out here with the rest.
    return detail::tmf8829_measurement_from_c(*out);
}

// ── sensor registers, presets, config page ──────────────────────────────────

std::string status_name(std::uint8_t status) {
    const char* n = depz_tmf8829_status_name(status);
    return n ? n : "";
}

std::optional<Preset> find_preset(const std::string& name) {
    const Preset* p = detail::whole_cstr(name) ? depz_tmf8829_find_preset(name.c_str()) : nullptr;
    if (!p) return std::nullopt;
    return *p;
}

std::optional<CfgField> find_cfg_field(const std::string& name) {
    const CfgField* f = detail::whole_cstr(name) ? depz_tmf8829_find_cfg_field(name.c_str()) : nullptr;
    if (!f) return std::nullopt;
    return *f;
}

std::uint32_t cfg_get(byte_span page, const std::string& name) {
    std::uint32_t v = 0;
    switch (detail::whole_cstr(name) ? depz_tmf8829_cfg_get_n(u8p(page), page.size(), name.c_str(), &v) : -1) {
        case 0: return v;
        case -1: throw std::out_of_range("unknown config field " + name);
        default: throw std::invalid_argument(name + ": config page too short for the field");
    }
}

void cfg_set(bytes& page, const std::string& name, std::int64_t value) {
    switch (detail::whole_cstr(name)
                ? depz_tmf8829_cfg_set_n(reinterpret_cast<std::uint8_t*>(page.data()), page.size(), name.c_str(), value)
                : -1) {
        case 0: return;
        case -1: throw std::out_of_range("unknown config field " + name);
        case -2: throw std::invalid_argument(name + ": " + std::to_string(value) + " out of range for the field");
        default: throw std::invalid_argument(name + ": config page too short for the field");
    }
}

std::vector<std::pair<std::string, std::uint32_t>> config_fields(byte_span page) {
    std::vector<std::pair<std::string, std::uint32_t>> out;
    out.reserve(CFG_FIELDS.size());
    for (const CfgField& f : CFG_FIELDS) out.emplace_back(f.name, cfg_get(page, f.name));
    return out;
}

// ── sensor firmware ─────────────────────────────────────────────────────────

byte_span firmware_image() {
    return byte_span(reinterpret_cast<const std::byte*>(depz_tmf8829_firmware), depz_tmf8829_firmware_len);
}

}  // namespace tmf8829
}  // namespace depz
