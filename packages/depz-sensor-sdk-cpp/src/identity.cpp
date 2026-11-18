#include "depz/identity.hpp"

#include <cstring>

#include "detail/c_bridge.hpp"

namespace depz {
namespace {

std::optional<SensorType> sensor_type_from_c(depz_sensor_type t) {
    switch (t) {
        case DEPZ_SENSOR_NONE: return std::nullopt;
        case DEPZ_SENSOR_SR04: return SensorType::Sr04;
        case DEPZ_SENSOR_VL53L8: return SensorType::Vl53l8;
        case DEPZ_SENSOR_BNO086: return SensorType::Bno086;
        case DEPZ_SENSOR_VL53L4: return SensorType::Vl53l4;
        case DEPZ_SENSOR_VL53L7: return SensorType::Vl53l7;
        case DEPZ_SENSOR_VL53LX: return SensorType::Vl53lx;
        case DEPZ_SENSOR_BNO055: return SensorType::Bno055;
        case DEPZ_SENSOR_TMF8829: return SensorType::Tmf8829;
        case DEPZ_SENSOR_UNKNOWN: break;
    }
    return SensorType::Unknown;
}

depz_sensor_type sensor_type_to_c(SensorType t) {
    switch (t) {
        case SensorType::Sr04: return DEPZ_SENSOR_SR04;
        case SensorType::Vl53l8: return DEPZ_SENSOR_VL53L8;
        case SensorType::Vl53l7: return DEPZ_SENSOR_VL53L7;
        case SensorType::Vl53l4: return DEPZ_SENSOR_VL53L4;
        case SensorType::Vl53lx: return DEPZ_SENSOR_VL53LX;
        case SensorType::Bno086: return DEPZ_SENSOR_BNO086;
        case SensorType::Bno055: return DEPZ_SENSOR_BNO055;
        case SensorType::Tmf8829: return DEPZ_SENSOR_TMF8829;
        case SensorType::Unknown: break;
    }
    return DEPZ_SENSOR_UNKNOWN;
}

DeviceMode mode_from_c(const char* mode) {
    if (std::strcmp(mode, "app") == 0) return DeviceMode::App;
    if (std::strcmp(mode, "bootloader") == 0) return DeviceMode::Bootloader;
    return DeviceMode::Unknown;
}

}  // namespace

std::string to_string(SensorType t) { return depz_sensor_type_str(sensor_type_to_c(t)); }

std::string to_string(DeviceMode m) {
    switch (m) {
        case DeviceMode::App: return "app";
        case DeviceMode::Bootloader: return "bootloader";
        case DeviceMode::Unknown: break;
    }
    return "unknown";
}

Identity parse_software_name(const std::string& name) {
    depz_identity c{};
    depz_parse_software_name_n(name.data(), name.size(), &c);
    // the C copies are truncated to their fields; the C++ Identity keeps the
    // caller's whole string
    return Identity{mode_from_c(c.mode), sensor_type_from_c(c.sensor_type), name,
                    std::string(c.version, depz_device_string_len(c.version, sizeof c.version))};
}

const std::vector<std::string>& sensor_class_names() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> v;
        for (std::size_t i = 0; i < DEPZ_SENSOR_CLASS_COUNT; ++i) v.emplace_back(depz_sensor_class_name(i));
        return v;
    }();
    return names;
}

std::string resolve_sensor_class(const std::string& software_name, const std::string& device_name,
                                 std::optional<std::uint16_t> usb_vid, std::optional<std::uint16_t> usb_pid) {
    const int vid = usb_vid.value_or(0), pid = usb_pid.value_or(0);
    return depz_resolve_sensor_class_n(software_name.data(), software_name.size(), device_name.data(),
                                       device_name.size(), usb_vid ? &vid : nullptr, usb_pid ? &pid : nullptr);
}

}  // namespace depz
