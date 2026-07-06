#include "depz/crc.hpp"

#include <array>

namespace depz {
namespace {

template <class T>
std::array<T, 256> make_reflected_table(T poly_reflected) {
    std::array<T, 256> table{};
    for (int i = 0; i < 256; ++i) {
        T crc = static_cast<T>(i);
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 1) ? static_cast<T>((crc >> 1) ^ poly_reflected)
                            : static_cast<T>(crc >> 1);
        }
        table[static_cast<std::size_t>(i)] = crc;
    }
    return table;
}

const std::array<std::uint8_t, 256>& crc8_table() {
    static const auto t = make_reflected_table<std::uint8_t>(0x8C);
    return t;
}
const std::array<std::uint16_t, 256>& crc16_table() {
    static const auto t = make_reflected_table<std::uint16_t>(0xA001);
    return t;
}
const std::array<std::uint32_t, 256>& crc32_table() {
    static const auto t = make_reflected_table<std::uint32_t>(0xEDB88320u);
    return t;
}

inline std::uint8_t u8(std::byte b) noexcept { return std::to_integer<std::uint8_t>(b); }

}  // namespace

std::uint8_t crc8_maxim(byte_span data) noexcept {
    const auto& table = crc8_table();
    std::uint8_t crc = 0x00;
    for (std::byte b : data) {
        crc = table[static_cast<std::uint8_t>(crc ^ u8(b))];
    }
    return crc;
}

std::uint16_t crc16_modbus(byte_span data) noexcept {
    const auto& table = crc16_table();
    std::uint16_t crc = 0xFFFF;
    for (std::byte b : data) {
        crc = static_cast<std::uint16_t>((crc >> 8) ^
                                         table[static_cast<std::uint8_t>((crc ^ u8(b)) & 0xFF)]);
    }
    return crc;
}

std::uint32_t crc32_iso_hdlc(byte_span data) noexcept {
    const auto& table = crc32_table();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::byte b : data) {
        crc = (crc >> 8) ^ table[static_cast<std::uint8_t>((crc ^ u8(b)) & 0xFF)];
    }
    return crc ^ 0xFFFFFFFFu;
}

std::uint16_t crc16_ccitt_false(byte_span data) noexcept {
    std::uint16_t crc = 0xFFFF;
    for (std::byte b : data) {
        crc ^= static_cast<std::uint16_t>(u8(b) << 8);
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 0x8000) ? static_cast<std::uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<std::uint16_t>(crc << 1);
        }
    }
    return crc;
}

}  // namespace depz
