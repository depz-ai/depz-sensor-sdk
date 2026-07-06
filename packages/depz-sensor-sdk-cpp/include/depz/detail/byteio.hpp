// Little-endian integer readers/writers over byte spans (internal helper).
#pragma once

#include <cstdint>
#include <stdexcept>

#include "depz/span.hpp"

namespace depz {
namespace detail {

inline std::uint8_t u8(std::byte b) noexcept { return std::to_integer<std::uint8_t>(b); }

inline void require(byte_span s, std::size_t n, const char* what) {
    if (s.size() < n) throw std::invalid_argument(what);
}

inline std::uint16_t rd_u16(byte_span s, std::size_t off) {
    return static_cast<std::uint16_t>(u8(s[off]) | (u8(s[off + 1]) << 8));
}

inline std::uint32_t rd_u32(byte_span s, std::size_t off) {
    return static_cast<std::uint32_t>(u8(s[off]) | (u8(s[off + 1]) << 8) |
                                      (u8(s[off + 2]) << 16) |
                                      (static_cast<std::uint32_t>(u8(s[off + 3])) << 24));
}

inline std::uint64_t rd_u64(byte_span s, std::size_t off) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(u8(s[off + i])) << (8 * i);
    return v;
}

inline std::int16_t rd_i16(byte_span s, std::size_t off) {
    return static_cast<std::int16_t>(rd_u16(s, off));
}

inline void wr_le(bytes& out, std::uint64_t v, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(std::byte{static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)});
    }
}

}  // namespace detail
}  // namespace depz
