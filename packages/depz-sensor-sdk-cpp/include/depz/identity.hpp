// Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).
#pragma once

#include <optional>
#include <string>

namespace depz {

enum class SensorType {
    Sr04,
    Vl53l8,
    Bno086,
    Unknown,
};

// String name for a SensorType (matches the vector encoding).
std::string to_string(SensorType t);

enum class DeviceMode {
    App,
    Bootloader,
    Unknown,
};

std::string to_string(DeviceMode m);

struct Identity {
    DeviceMode mode;
    std::optional<SensorType> sensor_type;  // nullopt in bootloader/unknown mode
    std::string software_name;
    std::string version;  // "" when not parseable
};

// Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
// stripped of trailing NUL/0xFF (strip_device_string).
Identity parse_software_name(const std::string& name);

}  // namespace depz
