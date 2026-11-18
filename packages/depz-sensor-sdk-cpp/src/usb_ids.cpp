#include "depz/usb_ids.hpp"

#include <algorithm>

#include "detail/c_bridge.hpp"

namespace depz {
namespace {

int c_id(std::optional<std::uint16_t> v) { return v ? static_cast<int>(*v) : -1; }

}  // namespace

bool is_known_depz_usb(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid) noexcept {
    return vid && pid && depz_is_known_depz_usb(c_id(vid), c_id(pid));
}

std::optional<std::string> usb_model_hint(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid) {
    if (!vid || !pid) return std::nullopt;
    const char* model = depz_usb_model_hint(c_id(vid), c_id(pid));
    if (!model) return std::nullopt;
    return std::string(model);
}

std::vector<PortInfo> order_by_serial(std::vector<PortInfo> ports) {
    auto key = [](const PortInfo& p) {
        depz_port_key k{p.port.c_str(), p.port.size(), nullptr, 0};
        if (p.usb_serial) {
            k.serial = p.usb_serial->c_str();
            k.serial_len = p.usb_serial->size();
        }
        return k;
    };
    std::sort(ports.begin(), ports.end(), [&](const PortInfo& a, const PortInfo& b) {
        const depz_port_key ka = key(a), kb = key(b);
        return depz_port_order_cmp(&ka, &kb) < 0;
    });
    return ports;
}

}  // namespace depz
