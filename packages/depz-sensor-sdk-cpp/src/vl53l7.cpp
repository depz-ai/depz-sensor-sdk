#include "depz/vl53l7.hpp"

#include <regex>

#include "depz/detail/byteio.hpp"

namespace depz {
namespace vl53l7 {

using detail::rd_u16;
using detail::rd_u32;
using detail::u8;
using detail::wr_le;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint16_t addr, std::uint16_t len) {
    bytes out;
    wr_le(out, addr, 2);
    wr_le(out, len, 2);
    return out;
}

bytes pack_write_reg(std::uint16_t addr, byte_span data) {
    bytes out;
    out.reserve(2 + data.size());
    wr_le(out, addr, 2);
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

bytes pack_pin_ctrl(std::uint8_t action) {
    bytes out;
    wr_le(out, action, 1);
    return out;
}

bytes pack_set_i2c_speed(std::uint16_t khz) {
    bytes out;
    wr_le(out, khz, 2);
    return out;
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<Vl53l7Info> Vl53l7Info::unpack(byte_span payload) {
    if (payload.size() < INFO_SIZE) return std::nullopt;
    Vl53l7Info r;
    r.int_edges = rd_u32(payload, 0);
    r.frames_dropped = rd_u32(payload, 4);
    r.i2c_errors = rd_u32(payload, 8);
    r.last_i2c_error = u8(payload[12]);
    r.lpn_level = u8(payload[13]);
    r.int_level = u8(payload[14]);
    r.i2c_khz = rd_u16(payload, 15);
    r.frame_size = rd_u16(payload, 17);
    r.streaming = u8(payload[19]) != 0;
    return r;
}

// ── sensor-class resolution ─────────────────────────────────────────────────

std::string to_string(Model m) {
    switch (m) {
        case Model::Vl53l5cx: return "vl53l5cx";
        case Model::Vl53l7cx: return "vl53l7cx";
        case Model::Vl53l7ch: return "vl53l7ch";
    }
    return "vl53l7cx";
}

namespace {
std::optional<Model> model_from_name(const std::string& s) {
    if (s == "vl53l5cx") return Model::Vl53l5cx;
    if (s == "vl53l7cx") return Model::Vl53l7cx;
    if (s == "vl53l7ch") return Model::Vl53l7ch;
    return std::nullopt;
}
}  // namespace

Model resolve_model(const std::optional<std::string>& usb_model,
                    const std::string& device_name) {
    // 1. Production USB PID model.
    if (usb_model) {
        if (auto m = model_from_name(*usb_model)) return *m;
    }
    // 2. First VL53L<5|7><CX|CH> part in the device name (case-sensitive).
    static const std::regex part_re("VL53L([57])(CX|CH)");
    std::smatch m;
    if (std::regex_search(device_name, m, part_re)) {
        std::string part = "vl53l" + m[1].str() + (m[2].str() == "CH" ? "ch" : "cx");
        if (auto r = model_from_name(part)) return *r;
        return Model::Vl53l7cx;  // e.g. VL53L5CH: no such class
    }
    // 3. Safe default.
    return Model::Vl53l7cx;
}

// ── frame decode ────────────────────────────────────────────────────────────

namespace {
constexpr std::uint32_t AMBIENT_RATE_IDX = 0x54D0;
}  // namespace

std::optional<int> infer_resolution(byte_span raw) {
    const std::size_t drs = raw.size();
    if (drs < 16) return std::nullopt;
    bytes swapped = vl53l8::swap_buffer(raw);
    byte_span buf = as_bytes(swapped);
    // Same block walk as vl53l8::decode_frame.
    std::size_t i = 16;
    while (i + 4 <= drs) {
        std::uint32_t bh = rd_u32(buf, i);
        std::uint32_t type = bh & 0xF;
        std::uint32_t size = (bh >> 4) & 0xFFF;
        std::uint32_t idx = (bh >> 16) & 0xFFFF;
        std::size_t msize = (type > 0x1 && type < 0xD) ? type * size : size;
        if (i + 4 + msize > drs) break;
        if (idx == AMBIENT_RATE_IDX) return static_cast<int>(msize / 4);
        i += msize + 4;
    }
    return std::nullopt;
}

std::optional<vl53l8::Vl53l8Frame> decode_frame(byte_span raw, std::optional<int> resolution) {
    auto f = vl53l8::decode_frame(raw, FOOTER_ID_OFF);
    if (!f) return std::nullopt;
    if (!resolution) resolution = infer_resolution(raw);
    if (resolution && *resolution > 0) {
        // Trim every per-zone array to the zones that exist (one target per
        // zone). The shared decoder's status-255 fill ran over the padding too;
        // the entries kept are identical to trim-then-fill.
        const std::size_t n = static_cast<std::size_t>(*resolution);
        auto trim = [n](auto& v) {
            if (v.size() > n) v.resize(n);
        };
        trim(f->distance_mm);
        trim(f->target_status);
        trim(f->nb_target_detected);
        trim(f->signal_per_spad);
        trim(f->ambient_per_spad);
        trim(f->nb_spads_enabled);
        trim(f->range_sigma_mm_raw);
        trim(f->reflectance);
        f->resolution = static_cast<int>(f->nb_target_detected.size());
    }
    return f;
}

}  // namespace vl53l7
}  // namespace depz
