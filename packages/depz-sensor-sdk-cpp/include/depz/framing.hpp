// Packet framing and incremental parser (contracts/01_TRANSPORT_FRAMING.md).
//
// A C++ face of the C SDK's depz_build_packet / depz_parser, byte-exact with
// firmware transport. The parser applies ERRATA E6: a packet whose header
// advertises a payload-CRC type but carries an empty payload has NO CRC bytes
// on the wire.
#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include <depz_sensor_sdk.h>

#include "depz/span.hpp"

namespace depz {

inline constexpr std::size_t HEADER_SIZE = DEPZ_HEADER_SIZE;
inline constexpr std::uint16_t MAX_PAYLOAD = DEPZ_MAX_PAYLOAD;
inline constexpr std::byte MAGIC0 = std::byte{DEPZ_MAGIC0};
inline constexpr std::byte MAGIC1 = std::byte{DEPZ_MAGIC1};

enum class CrcType : std::uint8_t {
    None = DEPZ_CRC_NONE,
    Crc8 = DEPZ_CRC8,
    Crc16 = DEPZ_CRC16,
    Crc32 = DEPZ_CRC32,
};

// CRC trailer for a payload (depz_payload_crc_trailer): little-endian CRC-8 /
// -16 / -32 of the payload, empty for CrcType::None, for a value that is not a
// listed CrcType and for an empty payload (ERRATA E6: no CRC bytes on the wire).
bytes payload_crc_bytes(CrcType crc_type, byte_span payload);

// Frame one packet. `crc_type` bits are set in the header even for an empty
// payload (matching device TX), but CRC bytes are appended only for non-empty
// payloads. `seq` is a free-running counter taken modulo 256 on the wire.
// Throws std::length_error if payload exceeds MAX_PAYLOAD, std::invalid_argument
// if crc_type is not one of the four listed CrcType values (contract 07 §3).
bytes build_packet(std::uint8_t cmd, byte_span payload = {}, std::uint32_t seq = 0,
                   CrcType crc_type = CrcType::None);

struct Packet {
    std::uint8_t cmd;
    std::uint8_t seq;
    bytes payload;
};

// Bytes discarded while hunting for a valid frame. Boundaries between
// consecutive Trash events depend on read chunking; only the concatenated
// byte stream is deterministic.
struct Trash {
    bytes data;
};

// A frame with a valid header whose payload CRC failed; dropped.
struct CrcError {
    std::uint8_t cmd;
    std::uint8_t seq;
};

using ParserEvent = std::variant<Packet, Trash, CrcError>;

// Incremental frame parser (depz_parser). Feed arbitrary byte chunks; get
// events. Event order is invariant to chunking (contract 01 §5) except Trash
// event boundaries — concatenate Trash data when comparing streams.
class PacketParser {
public:
    PacketParser() { depz_parser_init(&p_); }
    ~PacketParser() { depz_parser_free(&p_); }
    PacketParser(const PacketParser&) = delete;
    PacketParser& operator=(const PacketParser&) = delete;

    // std::bad_alloc when the parser cannot grow its buffer.
    std::vector<ParserEvent> feed(byte_span data);

    // Bytes still buffered (unconsumed residue), e.g. a partial frame.
    byte_span residue() const noexcept {
        return byte_span(reinterpret_cast<const std::byte*>(p_.buf), p_.len);
    }

    // Diagnostics counters, monotonic across the parser's life.
    std::uint64_t packets() const noexcept { return p_.packets; }
    std::uint64_t crc_errors() const noexcept { return p_.crc_errors; }
    std::uint64_t header_errors() const noexcept { return p_.header_errors; }
    std::uint64_t trash_bytes() const noexcept { return p_.trash_bytes; }

private:
    depz_parser p_{};
};

}  // namespace depz
