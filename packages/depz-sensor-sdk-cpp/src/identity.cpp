#include "depz/identity.hpp"

#include <regex>

namespace depz {

std::string to_string(SensorType t) {
    switch (t) {
        case SensorType::Sr04: return "sr04";
        case SensorType::Vl53l8: return "vl53l8";
        case SensorType::Bno086: return "bno086";
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
        if (contains("SR04")) return Identity{DeviceMode::App, SensorType::Sr04, name, version};
        if (contains("VL53L8")) return Identity{DeviceMode::App, SensorType::Vl53l8, name, version};
        if (contains("BNO086")) return Identity{DeviceMode::App, SensorType::Bno086, name, version};
        return Identity{DeviceMode::App, SensorType::Unknown, name, version};
    }
    return Identity{DeviceMode::Unknown, std::nullopt, name, version};
}

}  // namespace depz
