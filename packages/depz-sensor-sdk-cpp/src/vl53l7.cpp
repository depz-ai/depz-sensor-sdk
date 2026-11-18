#include "depz/vl53l7.hpp"

#include <memory>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {
namespace vl53l7 {

using detail::packed;
using detail::to_bytes;
using detail::u8p;

// ── command encoders ────────────────────────────────────────────────────────

bytes pack_read_reg(std::uint16_t addr, std::uint16_t len) {
    std::uint8_t out[4];
    return packed(out, depz_vl53l7_pack_read_reg(addr, len, out),
                  "VL53_READ_REG: len must be 1..1536 and addr+len <= 0x10000");
}

bytes pack_write_reg(std::uint16_t addr, byte_span data) {
    bytes out(2 + data.size());
    const std::size_t n = depz_vl53l7_pack_write_reg(addr, u8p(data), data.size(),
                                                     reinterpret_cast<std::uint8_t*>(out.data()));
    if (n == 0) throw std::invalid_argument("VL53_WRITE_REG: data must be 1..2048 bytes and addr+N <= 0x10000");
    out.resize(n);
    return out;
}

bytes pack_pin_ctrl(std::uint8_t action) {
    std::uint8_t out[1];
    return to_bytes(out, depz_vl53l7_pack_pin_ctrl(action, out));
}

bytes pack_set_i2c_speed(std::uint16_t khz) {
    std::uint8_t out[2];
    return to_bytes(out, depz_vl53l7_pack_set_i2c_speed(khz, out));
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<Vl53l7Info> Vl53l7Info::unpack(byte_span payload) {
    Vl53l7Info i{};
    if (depz_vl53l7_unpack_info(u8p(payload), payload.size(), &i) != 0) return std::nullopt;
    return i;
}

// ── sensor-class resolution ─────────────────────────────────────────────────

namespace {

depz_vl53l7_model to_c(Model m) {
    switch (m) {
        case Model::Vl53l5cx: return DEPZ_VL53L7_MODEL_L5CX;
        case Model::Vl53l7ch: return DEPZ_VL53L7_MODEL_L7CH;
        case Model::Vl53l7cx: break;
    }
    return DEPZ_VL53L7_MODEL_L7CX;
}

Model from_c(depz_vl53l7_model m) {
    switch (m) {
        case DEPZ_VL53L7_MODEL_L5CX: return Model::Vl53l5cx;
        case DEPZ_VL53L7_MODEL_L7CH: return Model::Vl53l7ch;
        case DEPZ_VL53L7_MODEL_L7CX: break;
    }
    return Model::Vl53l7cx;
}

}  // namespace

std::string to_string(Model m) { return depz_vl53l7_model_str(to_c(m)); }

Model resolve_model(const std::optional<std::string>& usb_model, const std::string& device_name) {
    return from_c(depz_vl53l7_resolve_model_n(usb_model ? usb_model->data() : nullptr,
                                              usb_model ? usb_model->size() : 0, device_name.data(),
                                              device_name.size()));
}

// ── frame decode ────────────────────────────────────────────────────────────

std::optional<int> infer_resolution(byte_span raw) {
    auto c = std::make_unique<depz_vl53l8_frame>();
    if (depz_vl53l7_decode_frame(u8p(raw), raw.size(), 0, c.get(), nullptr, 0, nullptr) != 0 || c->resolution == 0)
        return std::nullopt;
    return c->resolution;
}

std::optional<vl53l8::Vl53l8Frame> decode_frame(byte_span raw, std::optional<int> resolution) {
    auto c = std::make_unique<depz_vl53l8_frame>();
    bytes cnh(raw.size());
    std::size_t cnh_len = 0;
    if (depz_vl53l7_decode_frame(u8p(raw), raw.size(), 0, c.get(), reinterpret_cast<std::uint8_t*>(cnh.data()),
                                 cnh.size(), &cnh_len) != 0)
        return std::nullopt;
    auto f = detail::vl53l8_frame_from_c(*c, reinterpret_cast<const std::uint8_t*>(cnh.data()), cnh_len);
    if (resolution && *resolution > 0 && *resolution < f.resolution) {
        // The caller's resolution is below the frame's zone count: keep the
        // zones that exist for it (the C decoder already trimmed to the frame's).
        const auto n = static_cast<std::size_t>(*resolution);
        auto trim = [n](auto& v) {
            if (v.size() > n) v.resize(n);
        };
        trim(f.distance_mm);
        trim(f.target_status);
        trim(f.nb_target_detected);
        trim(f.signal_per_spad);
        trim(f.ambient_per_spad);
        trim(f.nb_spads_enabled);
        trim(f.range_sigma_mm);
        trim(f.reflectance);
        f.resolution = static_cast<int>(f.nb_target_detected.size());
    }
    return f;
}

}  // namespace vl53l7
}  // namespace depz
