// DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).
//
// The PID->model map is an informational hint only; the protocol probe
// (GET_NAME_ACTIVE_SOFTWARE) remains the source of truth for what a device is.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace depz {

inline constexpr std::uint16_t DEPZ_USB_VID = 0x1BCF;  // 7119
inline constexpr std::uint16_t PID_SR04 = 0xEC78;      // 60536
inline constexpr std::uint16_t PID_VL53L8 = 0xED40;    // 60736
inline constexpr std::uint16_t PID_BNO086 = 0xEE08;    // 60936

// Dev / unprogrammed default (STMicroelectronics) that dev units carry.
inline constexpr std::uint16_t DEV_USB_VID = 0x0483;  // 1155
inline constexpr std::uint16_t DEV_USB_PID = 0x56DC;  // 22236

// Inclusive reserved sensor PID block under DEPZ_USB_VID.
inline constexpr std::uint16_t DEPZ_PID_RANGE_LO = 60536;
inline constexpr std::uint16_t DEPZ_PID_RANGE_HI = 65535;

// True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.
bool is_known_depz_usb(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid) noexcept;

// Best-guess model name for a (vid, pid), or std::nullopt. Informational only.
std::optional<std::string> usb_model_hint(std::optional<std::uint16_t> vid,
                                          std::optional<std::uint16_t> pid);

// One enumerated serial port with its USB iSerial (used for ordering).
struct PortInfo {
    std::string port;
    std::optional<std::string> usb_serial;
};

// Order ports by USB serial ascending; None/empty serial sorts last, tie-break
// by port path (contract 02 §4).
std::vector<PortInfo> order_by_serial(std::vector<PortInfo> ports);

}  // namespace depz
