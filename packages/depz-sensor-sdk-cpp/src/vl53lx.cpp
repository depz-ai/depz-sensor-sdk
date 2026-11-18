#include "depz/vl53lx.hpp"

#include <cctype>

#include "detail/c_bridge.hpp"

namespace depz {
namespace vl53lx {

using detail::to_bytes;
using detail::u8p;

// ── command encoders ────────────────────────────────────────────────────────

std::optional<bytes> pack_start_stream(std::uint16_t addr, std::uint16_t len, const std::vector<ClearStep>& clear,
                                       std::uint8_t flags) {
    std::uint8_t out[DEPZ_VL53LX_START_STREAM_MAX];
    const std::size_t n = depz_vl53lx_pack_start_stream(addr, len, flags, clear.data(), clear.size(), out);
    if (n == 0) return std::nullopt;
    return to_bytes(out, n);
}

std::optional<bytes> pack_set_addr_width(std::uint8_t width) {
    std::uint8_t out[1];
    const std::size_t n = depz_vl53lx_pack_set_addr_width(width, out);
    if (n == 0) return std::nullopt;
    return to_bytes(out, n);
}

// ── report decoders ─────────────────────────────────────────────────────────

std::optional<Vl53lxInfo> Vl53lxInfo::unpack(byte_span payload) {
    Vl53lxInfo i{};
    if (depz_vl53lx_unpack_info(u8p(payload), payload.size(), &i) != 0) return std::nullopt;
    return i;
}

// ── product table ───────────────────────────────────────────────────────────

namespace {

depz_vl53lx_driver to_c(DriverKind k) {
    switch (k) {
        case DriverKind::Uld: return DEPZ_VL53LX_DRIVER_ULD;
        case DriverKind::Ulp: return DEPZ_VL53LX_DRIVER_ULP;
        case DriverKind::Histogram: break;
    }
    return DEPZ_VL53LX_DRIVER_HISTOGRAM;
}

// The kinds of a bitmask, UI order (ULD, ULP, histogram).
std::vector<DriverKind> kinds_from_c(unsigned mask) {
    std::vector<DriverKind> out;
    for (DriverKind k : {DriverKind::Uld, DriverKind::Ulp, DriverKind::Histogram})
        if (mask & static_cast<unsigned>(to_c(k))) out.push_back(k);
    return out;
}

Product product_from_c(const depz_vl53lx_product_info& c) {
    Product p;
    p.name = c.name;
    p.usb_pid = c.usb_pid;
    p.model_id = c.model_id;
    p.reach_mm = c.reach_mm;
    p.driver_kinds = kinds_from_c(c.driver_kinds);
    p.default_driver = kinds_from_c(static_cast<unsigned>(c.default_driver)).front();
    p.addr_width = c.addr_width;
    p.clear_steps.assign(c.clear, c.clear + c.n_clear);
    p.max_khz = c.max_khz;
    return p;
}

}  // namespace

std::string to_string(DriverKind k) { return depz_vl53lx_driver_str(to_c(k)); }

const std::vector<Product>& products() {
    static const std::vector<Product> table = [] {
        std::vector<Product> v;
        for (int i = 0; i < DEPZ_VL53LX_PRODUCT_COUNT; ++i)
            v.push_back(product_from_c(*depz_vl53lx_product_get(static_cast<depz_vl53lx_product>(i))));
        return v;
    }();
    return table;
}

const Product* find_product(const std::string& name) {
    const depz_vl53lx_product p = depz_vl53lx_product_from_str(name.c_str());
    if (p == DEPZ_VL53LX_PRODUCT_NONE) return nullptr;
    const Product& row = products()[static_cast<std::size_t>(p)];
    return row.name == name ? &row : nullptr;  // the exact name, not the C codec's case-insensitive match
}

std::optional<std::string> product_from_board_name(const std::string& name) {
    const depz_vl53lx_product p = depz_vl53lx_product_from_board_name_n(name.data(), name.size());
    if (p == DEPZ_VL53LX_PRODUCT_NONE) return std::nullopt;
    return std::string(depz_vl53lx_product_get(p)->name);
}

// ── class resolution ────────────────────────────────────────────────────────

namespace {

SensorClass class_from_c(depz_vl53lx_class c) {
    switch (c) {
        case DEPZ_VL53LX_CLASS_L0X: return SensorClass::Vl53l0x;
        case DEPZ_VL53LX_CLASS_L1CX: return SensorClass::Vl53l1cx;
        case DEPZ_VL53LX_CLASS_L1CB: return SensorClass::Vl53l1cb;
        case DEPZ_VL53LX_CLASS_L3CX: return SensorClass::Vl53l3cx;
        case DEPZ_VL53LX_CLASS_L4CX: return SensorClass::Vl53l4cx;
        case DEPZ_VL53LX_CLASS_GENERIC: break;
    }
    return SensorClass::Vl53lx;
}

depz_vl53lx_class class_to_c(SensorClass c) {
    switch (c) {
        case SensorClass::Vl53l0x: return DEPZ_VL53LX_CLASS_L0X;
        case SensorClass::Vl53l1cx: return DEPZ_VL53LX_CLASS_L1CX;
        case SensorClass::Vl53l1cb: return DEPZ_VL53LX_CLASS_L1CB;
        case SensorClass::Vl53l3cx: return DEPZ_VL53LX_CLASS_L3CX;
        case SensorClass::Vl53l4cx: return DEPZ_VL53LX_CLASS_L4CX;
        case SensorClass::Vl53lx: break;
    }
    return DEPZ_VL53LX_CLASS_GENERIC;
}

}  // namespace

std::string to_string(SensorClass c) {
    // the reference class name capitalises the C SDK's "vl53l0x": "Vl53l0x"
    std::string s = depz_vl53lx_class_str(class_to_c(c));
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

SensorClass resolve_class(const std::optional<std::string>& usb_model, const std::string& device_name) {
    return class_from_c(depz_vl53lx_resolve_class_n(usb_model ? usb_model->data() : nullptr,
                                                    usb_model ? usb_model->size() : 0, device_name.data(),
                                                    device_name.size()));
}

// ── stateless block decoders ────────────────────────────────────────────────

std::optional<DieResult> decode_die_block(byte_span raw, DieVariant variant) {
    DieResult r{};
    if (depz_vl53lx_decode_die_block(u8p(raw), raw.size(),
                                     variant == DieVariant::L1 ? DEPZ_VL53LX_DIE_L1 : DEPZ_VL53LX_DIE_L4, &r) != 0)
        return std::nullopt;
    return r;
}

std::optional<L0xRaw> decode_l0x_raw(byte_span raw) {
    L0xRaw r{};
    if (depz_vl53lx_decode_l0x_raw(u8p(raw), raw.size(), &r) != 0) return std::nullopt;
    return r;
}

std::optional<HistogramRaw> decode_histogram_raw(byte_span raw) {
    HistogramRaw r{};
    if (depz_vl53lx_decode_histogram_raw(u8p(raw), raw.size(), &r) != 0) return std::nullopt;
    return r;
}

}  // namespace vl53lx
}  // namespace depz
