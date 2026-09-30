// Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).
#pragma once

#include <optional>
#include <string>

namespace depz {

enum class SensorType {
    Sr04,
    Vl53l8,
    Vl53l7,  // VL53L5CX / VL53L7CX / VL53L7CH (one APP_VL53L7 firmware)
    Vl53l4,
    Vl53lx,  // VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX (one APP_VL53L0_4 firmware)
    Bno086,
    Bno055,  // 9-axis IMU on the APP_BNO055 register bridge (contract 13)
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
