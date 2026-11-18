#include "depz/framing.hpp"

#include <new>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {

bytes payload_crc_bytes(CrcType crc_type, byte_span payload) {
    if (payload.empty() || (crc_type != CrcType::Crc8 && crc_type != CrcType::Crc16 && crc_type != CrcType::Crc32))
        return {};
    std::uint8_t out[4];
    return detail::to_bytes(out, depz_payload_crc_trailer(static_cast<depz_crc_type>(crc_type), detail::u8p(payload),
                                                          payload.size(), out));
}

bytes build_packet(std::uint8_t cmd, byte_span payload, std::uint32_t seq32, CrcType crc_type) {
    bytes out(DEPZ_MAX_FRAME);
    std::size_t n = 0;
    const int rc = depz_build_packet(cmd, detail::u8p(payload), payload.size(), seq32,
                                     static_cast<depz_crc_type>(crc_type),
                                     reinterpret_cast<std::uint8_t*>(out.data()), out.size(), &n);
    if (rc == -1) throw std::length_error("payload too long");
    if (rc != 0) throw std::invalid_argument("crc_type is not a listed CrcType");
    out.resize(n);
    return out;
}

namespace {

void collect(const depz_event* ev, void* user) {
    auto& out = *static_cast<std::vector<ParserEvent>*>(user);
    switch (ev->type) {
        case DEPZ_EV_PACKET:
            out.push_back(Packet{ev->cmd, ev->seq, detail::to_bytes(ev->payload, ev->payload_len)});
            break;
        case DEPZ_EV_TRASH: out.push_back(Trash{detail::to_bytes(ev->trash, ev->trash_len)}); break;
        case DEPZ_EV_CRC_ERROR: out.push_back(CrcError{ev->cmd, ev->seq}); break;
    }
}

}  // namespace

std::vector<ParserEvent> PacketParser::feed(byte_span data) {
    std::vector<ParserEvent> out;
    if (depz_parser_feed(&p_, detail::u8p(data), data.size(), collect, &out) != 0) throw std::bad_alloc();
    return out;
}

}  // namespace depz
