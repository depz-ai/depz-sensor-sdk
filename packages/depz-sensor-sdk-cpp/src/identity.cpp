#include "depz/identity.hpp"

#include "depz/usb_ids.hpp"
#include "depz/vl53l7.hpp"
#include "depz/vl53lx.hpp"

#include <algorithm>
#include <cctype>

#include <regex>

namespace depz {

std::string to_string(SensorType t) {
    switch (t) {
        case SensorType::Sr04: return "sr04";
        case SensorType::Vl53l8: return "vl53l8";
        case SensorType::Vl53l7: return "vl53l7";
        case SensorType::Vl53l4: return "vl53l4";
        case SensorType::Vl53lx: return "vl53lx";
        case SensorType::Bno086: return "bno086";
        case SensorType::Bno055: return "bno055";
        case SensorType::Tmf8829: return "tmf8829";
        case SensorType::Unknown: return "unknown";
    }
    return "unknown";
}

std::string to_string(DeviceMode m) {
    switch (m) {
        case DeviceMode::App: return "app";
        case DeviceMode::Bootloader: return "bootloader";
        case DeviceMode::Unknown: return "unknown";
    }
    return "unknown";
}

Identity parse_software_name(const std::string& name) {
    static const std::regex version_re(R"(_v(\d+(?:\.\d+)*)$)");

    std::string version;
    std::smatch m;
    if (std::regex_search(name, m, version_re)) {
        version = m[1].str();
    }

    auto starts_with = [&](const char* prefix) {
        std::string p(prefix);
        return name.size() >= p.size() && name.compare(0, p.size(), p) == 0;
    };
    auto contains = [&](const char* tok) { return name.find(tok) != std::string::npos; };

    if (starts_with("BOOTDEPZ")) {
        return Identity{DeviceMode::Bootloader, std::nullopt, name, version};
    }
    if (starts_with("APP_")) {
        // Product tokens in contract order: the first one found wins.
        if (contains("SR04")) return Identity{DeviceMode::App, SensorType::Sr04, name, version};
        if (contains("VL53L4")) return Identity{DeviceMode::App, SensorType::Vl53l4, name, version};
        if (contains("VL53L8")) return Identity{DeviceMode::App, SensorType::Vl53l8, name, version};
        if (contains("VL53L7")) return Identity{DeviceMode::App, SensorType::Vl53l7, name, version};
        // The 1D-family bridge: boards answer APP_VL53L0_4_v*, the protocol
        // spec calls it APP_VL53LX_v* (contract 12).
        if (contains("VL53L0_4")) return Identity{DeviceMode::App, SensorType::Vl53lx, name, version};
        if (contains("VL53LX")) return Identity{DeviceMode::App, SensorType::Vl53lx, name, version};
        if (contains("BNO086")) return Identity{DeviceMode::App, SensorType::Bno086, name, version};
        if (contains("BNO055")) return Identity{DeviceMode::App, SensorType::Bno055, name, version};
        if (contains("TMF8829")) return Identity{DeviceMode::App, SensorType::Tmf8829, name, version};
        return Identity{DeviceMode::App, SensorType::Unknown, name, version};
    }
    return Identity{DeviceMode::Unknown, std::nullopt, name, version};
}

// ── class resolution (contract 02 §4.4) ─────────────────────────────────────

const std::vector<std::string>& sensor_class_names() {
    static const std::vector<std::string> names = {
        "sr04",     "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx",
        "vl53l7ch", "vl53l0x",  "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx",
        "vl53lx",   "bno086",   "bno055",   "tmf8829",  "device",   "bootloader",
    };
    return names;
}

std::string resolve_sensor_class(const std::string& software_name, const std::string& device_name,
                                 std::optional<std::uint16_t> usb_vid,
                                 std::optional<std::uint16_t> usb_pid) {
    const Identity ident = parse_software_name(software_name);
    if (ident.mode == DeviceMode::Bootloader) return "bootloader";
    if (!ident.sensor_type) return "device";
    const std::optional<std::string> model = usb_model_hint(usb_vid, usb_pid);
    switch (*ident.sensor_type) {
        case SensorType::Sr04: return "sr04";
        case SensorType::Vl53l8:
            // CX and CH share firmware and silicon: only the production PID
            // tells them apart; anything else falls to the CX base.
            return model && *model == "vl53l8ch" ? "vl53l8ch" : "vl53l8cx";
        case SensorType::Vl53l7: return vl53l7::to_string(vl53l7::resolve_model(model, device_name));
        case SensorType::Vl53lx: {
            std::string name = vl53lx::to_string(vl53lx::resolve_class(model, device_name));  // "Vl53l0x"
            std::transform(name.begin(), name.end(), name.begin(),
                           [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });
            return name;
        }
        case SensorType::Vl53l4: return "vl53l4cd";
        case SensorType::Bno086: return "bno086";
        case SensorType::Bno055: return "bno055";
        case SensorType::Tmf8829: return "tmf8829";
        case SensorType::Unknown: break;
    }
    return "device";
}

}  // namespace depz
