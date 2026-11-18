#include "depz/crc.hpp"

#include "detail/c_bridge.hpp"

namespace depz {

std::uint8_t crc8_maxim(byte_span data) noexcept { return depz_crc8_maxim(detail::u8p(data), data.size()); }

std::uint16_t crc16_modbus(byte_span data) noexcept {
    return depz_crc16_modbus(detail::u8p(data), data.size());
}

std::uint32_t crc32_iso_hdlc(byte_span data) noexcept {
    return depz_crc32_iso_hdlc(detail::u8p(data), data.size());
}

std::uint16_t crc16_ccitt_false(byte_span data) noexcept {
    return depz_crc16_ccitt_false(detail::u8p(data), data.size());
}

}  // namespace depz
