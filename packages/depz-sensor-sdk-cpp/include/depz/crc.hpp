// Transport CRCs (contracts/01_TRANSPORT_FRAMING.md §3).
//
// All wire CRCs are reflected table implementations, byte-exact with the
// firmware and the Python/TS reference SDKs. CRC-8/MAXIM init is 0x00 for
// every device (contracts/ERRATA.md E1).
#pragma once

#include <cstdint>

#include "depz/span.hpp"

namespace depz {

// CRC-8/MAXIM: poly 0x31 reflected (0x8C), init 0x00, xorout 0x00.
std::uint8_t crc8_maxim(byte_span data) noexcept;

// CRC-16/MODBUS: poly 0x8005 reflected (0xA001), init 0xFFFF, xorout 0x0000.
std::uint16_t crc16_modbus(byte_span data) noexcept;

// CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected (0xEDB88320), init/xorout 0xFFFFFFFF.
std::uint32_t crc32_iso_hdlc(byte_span data) noexcept;

// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
// Used only for the `.fwdepz` file header (contract 06), never on the wire.
std::uint16_t crc16_ccitt_false(byte_span data) noexcept;

}  // namespace depz
