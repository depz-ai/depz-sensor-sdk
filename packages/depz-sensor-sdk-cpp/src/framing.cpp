#include "depz/framing.hpp"

#include <stdexcept>

#include "depz/crc.hpp"

namespace depz {
namespace {

inline std::byte b(std::uint8_t v) noexcept { return std::byte{v}; }
inline std::uint8_t u8(std::byte x) noexcept { return std::to_integer<std::uint8_t>(x); }

std::size_t crc_size(CrcType t) noexcept {
    switch (t) {
        case CrcType::Crc8: return 1;
        case CrcType::Crc16: return 2;
        case CrcType::Crc32: return 4;
        default: return 0;
    }
}

void append_le(bytes& out, std::uint32_t value, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(b(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF)));
    }
}

}  // namespace

bytes payload_crc_bytes(CrcType crc_type, byte_span payload) {
    bytes out;
    if (crc_type == CrcType::None || payload.empty()) return out;
    switch (crc_type) {
        case CrcType::Crc8: append_le(out, crc8_maxim(payload), 1); break;
        case CrcType::Crc16: append_le(out, crc16_modbus(payload), 2); break;
        case CrcType::Crc32: append_le(out, crc32_iso_hdlc(payload), 4); break;
        default: break;
    }
    return out;
}

bytes build_packet(std::uint8_t cmd, byte_span payload, std::uint8_t seq, CrcType crc_type) {
    if (payload.size() > MAX_PAYLOAD) {
        throw std::length_error("payload too long");
    }
    const std::uint16_t data_size = static_cast<std::uint16_t>(
        payload.size() | (static_cast<std::uint16_t>(crc_type) << 14));
    const std::byte ds0 = b(static_cast<std::uint8_t>(data_size & 0xFF));
    const std::byte ds1 = b(static_cast<std::uint8_t>((data_size >> 8) & 0xFF));

    std::array<std::byte, 4> hdr_crc_input{ds0, ds1, b(cmd), b(seq)};
    const std::uint8_t hdr_crc = crc8_maxim(byte_span(hdr_crc_input.data(), 4));

    bytes out;
    out.reserve(HEADER_SIZE + payload.size() + crc_size(crc_type));
    out.push_back(MAGIC0);
    out.push_back(MAGIC1);
    out.push_back(ds0);
    out.push_back(ds1);
    out.push_back(b(cmd));
    out.push_back(b(seq));
    out.push_back(b(hdr_crc));
    for (std::byte x : payload) out.push_back(x);
    bytes trailer = payload_crc_bytes(crc_type, payload);
    out.insert(out.end(), trailer.begin(), trailer.end());
    return out;
}

Trash PacketParser::emit_trash(std::size_t count) {
    Trash t;
    t.data.assign(buf_.begin(), buf_.begin() + static_cast<std::ptrdiff_t>(count));
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<std::ptrdiff_t>(count));
    trash_bytes += count;
    return t;
}

bool PacketParser::parse_one(std::vector<ParserEvent>& out) {
    const std::size_t n = buf_.size();

    // Find MAGIC (0xA5 0xC3).
    bool found = false;
    std::size_t pos = 0;
    for (std::size_t i = 0; i + 1 < n; ++i) {
        if (buf_[i] == MAGIC0 && buf_[i + 1] == MAGIC1) {
            pos = i;
            found = true;
            break;
        }
    }

    if (!found) {
        // Keep the last byte: it may be a split 0xA5.
        if (n > 1) {
            out.push_back(emit_trash(n - 1));
            return true;
        }
        return false;
    }
    if (pos > 0) {
        out.push_back(emit_trash(pos));
        return true;
    }
    if (n < HEADER_SIZE) return false;

    const std::uint16_t data_size =
        static_cast<std::uint16_t>(u8(buf_[2]) | (u8(buf_[3]) << 8));
    const std::uint16_t payload_size = data_size & MAX_PAYLOAD;
    const CrcType crc_type = static_cast<CrcType>((data_size >> 14) & 0x03);

    std::array<std::byte, 4> hdr_input{buf_[2], buf_[3], buf_[4], buf_[5]};
    if (crc8_maxim(byte_span(hdr_input.data(), 4)) != u8(buf_[6])) {
        // Header corrupt: advance one byte past the magic start; magic hunt resyncs.
        ++header_errors;
        out.push_back(emit_trash(1));
        return true;
    }

    // ERRATA E6: empty payload never carries CRC bytes.
    const std::size_t csize = payload_size > 0 ? crc_size(crc_type) : 0;
    const std::size_t total = HEADER_SIZE + payload_size + csize;
    if (n < total) return false;

    const std::uint8_t cmd = u8(buf_[4]);
    const std::uint8_t seq = u8(buf_[5]);
    bytes payload(buf_.begin() + HEADER_SIZE,
                  buf_.begin() + static_cast<std::ptrdiff_t>(HEADER_SIZE + payload_size));
    bytes trailer(buf_.begin() + static_cast<std::ptrdiff_t>(HEADER_SIZE + payload_size),
                  buf_.begin() + static_cast<std::ptrdiff_t>(total));
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<std::ptrdiff_t>(total));

    if (csize > 0 && payload_crc_bytes(crc_type, as_bytes(payload)) != trailer) {
        ++crc_errors;
        out.push_back(CrcError{cmd, seq});
        return true;
    }
    ++packets;
    out.push_back(Packet{cmd, seq, std::move(payload)});
    return true;
}

std::vector<ParserEvent> PacketParser::feed(byte_span data) {
    buf_.insert(buf_.end(), data.begin(), data.end());
    std::vector<ParserEvent> out;
    while (parse_one(out)) {
    }
    return out;
}

}  // namespace depz
