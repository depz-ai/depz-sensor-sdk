#include "depz/usb_ids.hpp"

#include <algorithm>
#include <tuple>
#include <unordered_map>

namespace depz {
namespace {

// PID -> sensor-model hint. Informational; the protocol probe is authoritative.
const std::unordered_map<std::uint16_t, std::string>& pid_model_map() {
    static const std::unordered_map<std::uint16_t, std::string> m = {
        {PID_SR04, "sr04"},         // 0xEC78
        {PID_VL53L8, "vl53l8ch"},   // 0xED40
        {0xED41, "vl53l0x"},   {0xED42, "vl53l1cb"}, {0xED43, "vl53l1cx"},
        {0xED44, "vl53l3cx"},  {0xED45, "vl53l4cd"}, {0xED46, "vl53l4cx"},
        {0xED47, "vl53l4ed"},  {0xED48, "vl53l5cx"}, {0xED49, "vl53l7cx"},
        {0xED4A, "vl53l7ch"},  {0xED4B, "vl53l8cx"},  // 0xED4B hw-verified
        {PID_BNO086, "bno086"},     // 0xEE08
        {0xEE09, "bno085"},    {0xEE0A, "bno055"},
    };
    return m;
}

}  // namespace

bool is_known_depz_usb(std::optional<std::uint16_t> vid,
                       std::optional<std::uint16_t> pid) noexcept {
    if (!vid || !pid) return false;
    if (*vid == DEV_USB_VID && *pid == DEV_USB_PID) return true;
    if (*vid != DEPZ_USB_VID) return false;
    if (pid_model_map().count(*pid)) return true;
    return *pid >= DEPZ_PID_RANGE_LO && *pid <= DEPZ_PID_RANGE_HI;
}

std::optional<std::string> usb_model_hint(std::optional<std::uint16_t> vid,
                                          std::optional<std::uint16_t> pid) {
    if (!vid || !pid) return std::nullopt;
    if (*vid == DEV_USB_VID && *pid == DEV_USB_PID) return std::string("dev");
    if (*vid == DEPZ_USB_VID) {
        auto it = pid_model_map().find(*pid);
        if (it != pid_model_map().end()) return it->second;
    }
    return std::nullopt;
}

std::vector<PortInfo> order_by_serial(std::vector<PortInfo> ports) {
    auto key = [](const PortInfo& p) {
        if (p.usb_serial && !p.usb_serial->empty()) {
            return std::make_tuple(0, *p.usb_serial, p.port);
        }
        return std::make_tuple(1, std::string(), p.port);
    };
    std::sort(ports.begin(), ports.end(),
              [&](const PortInfo& a, const PortInfo& b) { return key(a) < key(b); });
    return ports;
}

}  // namespace depz
