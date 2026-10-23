// Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace depz {

enum class SensorType {
    Sr04,
    Vl53l8,
    Vl53l7,  // VL53L5CX / VL53L7CX / VL53L7CH (one APP_VL53L7 firmware)
    Vl53l4,
    Vl53lx,  // VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX (one APP_VL53L0_4 firmware)
    Bno086,
    Bno055,  // 9-axis IMU on the APP_BNO055 register bridge (contract 13)
    Tmf8829,  // multizone dToF on the APP_TMF8829 SPI register bridge (contract 14)
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

// ── class resolution (contract 02 §4.4) ─────────────────────────────────────

// Every name resolve_sensor_class() returns, in contract order: the sensor
// classes of contract 07 §1, "vl53lx" (the generic 1D-family class),
// "tmf8829", "device" (the base Device: an unknown application, or a name
// that is neither APP_ nor BOOTDEPZ) and "bootloader" (no sensor class: use
// the contract 06 client). BNO085 boards run APP_BNO086 and resolve to
// "bno086".
const std::vector<std::string>& sensor_class_names();

// The canonical class name for a device's probe answers and its port's USB
// id — contract 02 §4.4, pure. `software_name` is the GET_NAME_ACTIVE_SOFTWARE
// answer, `device_name` the GET_DEVICE_NAME answer ("" when there was none);
// the USB id is nullopt when unknown (a replay, a bridge).
//   - BOOTDEPZ_*   -> "bootloader";
//   - APP_VL53L8   -> "vl53l8ch" if the USB model is VL53L8CH, else "vl53l8cx";
//   - APP_VL53L7   -> vl53l7::resolve_model() (USB model, then the name);
//   - APP_VL53L0_4 -> vl53lx::resolve_class() (USB model, then the name, else
//     the generic "vl53lx"; VL53L4CD is generic on this firmware);
//   - any other known token -> its class; unknown APP_* (or neither APP_ nor
//     BOOTDEPZ) -> "device".
// Vectors: contracts/vectors/probe.json.
std::string resolve_sensor_class(const std::string& software_name, const std::string& device_name,
                                 std::optional<std::uint16_t> usb_vid,
                                 std::optional<std::uint16_t> usb_pid);

}  // namespace depz
