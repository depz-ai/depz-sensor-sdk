// Packet framing and incremental parser (contracts/01_TRANSPORT_FRAMING.md).
//
// Byte-exact with firmware transport. The parser applies ERRATA E6: a packet
// whose header advertises a payload-CRC type but carries an empty payload has
// NO CRC bytes on the wire.
#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include "depz/span.hpp"

namespace depz {

inline constexpr std::size_t HEADER_SIZE = 7;
inline constexpr std::uint16_t MAX_PAYLOAD = 0x3FFF;
inline constexpr std::byte MAGIC0 = std::byte{0xA5};
inline constexpr std::byte MAGIC1 = std::byte{0xC3};

enum class CrcType : std::uint8_t {
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
};

// CRC trailer for a payload; empty payloads never carry CRC bytes (ERRATA E6).
bytes payload_crc_bytes(CrcType crc_type, byte_span payload);

// Frame one packet. `crc_type` bits are set in the header even for an empty
// payload (matching device TX), but CRC bytes are appended only for non-empty
// payloads. Throws std::length_error if payload exceeds MAX_PAYLOAD.
bytes build_packet(std::uint8_t cmd, byte_span payload = {}, std::uint8_t seq = 0,
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

// Incremental frame parser. Feed arbitrary byte chunks; get events. Event
// order is invariant to chunking (contract 01 §5) except Trash event
// boundaries — concatenate Trash data when comparing streams.
class PacketParser {
public:
    std::vector<ParserEvent> feed(byte_span data);

    // Bytes still buffered (unconsumed residue), e.g. a partial frame.
    byte_span residue() const noexcept { return byte_span(buf_.data(), buf_.size()); }

    std::size_t packets = 0;
    std::size_t crc_errors = 0;
    std::size_t header_errors = 0;
    std::size_t trash_bytes = 0;

private:
    // Returns true and appends one event, or false if nothing more to parse.
    bool parse_one(std::vector<ParserEvent>& out);
    Trash emit_trash(std::size_t count);

    std::vector<std::byte> buf_;
};

}  // namespace depz
