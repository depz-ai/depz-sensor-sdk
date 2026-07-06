#include "depz/fwdepz.hpp"

#include <cstring>

#include "depz/crc.hpp"
#include "depz/detail/byteio.hpp"

namespace depz {

using detail::rd_u16;
using detail::rd_u32;
using detail::require;
using detail::u8;
using detail::wr_le;

FlashInfo FlashInfo::unpack(byte_span p) {
    // payload: cmd u8, page_size u16, reserved u16, app_start u32, app_size u32
    require(p, 13, "FlashInfo payload too short");
    return FlashInfo{rd_u16(p, 1), rd_u32(p, 5), rd_u32(p, 9)};
}

bytes pack_write_page(std::uint32_t addr, byte_span data) {
    bytes out;
    wr_le(out, addr, 4);
    wr_le(out, static_cast<std::uint16_t>(data.size()), 2);
    for (std::byte x : data) out.push_back(x);
    return out;
}

bytes pack_read_page(std::uint32_t addr, std::uint16_t size) {
    bytes out;
    wr_le(out, addr, 4);
    wr_le(out, size, 2);
    return out;
}

FwDepzImage FwDepzImage::parse(byte_span blob) {
    if (blob.size() < FWDEPZ_HEADER_SIZE) {
        throw FwDepzError(FwDepzErrorKind::TooShort, "file too short");
    }
    for (std::size_t i = 0; i < 8; ++i) {
        if (static_cast<char>(u8(blob[i])) != FWDEPZ_MAGIC[i]) {
            throw FwDepzError(FwDepzErrorKind::Magic, "bad magic (not a .fwdepz file)");
        }
    }
    const std::uint16_t hdr_crc = rd_u16(blob, 62);
    const std::uint16_t actual = crc16_ccitt_false(blob.first(62));
    if (hdr_crc != actual) {
        throw FwDepzError(FwDepzErrorKind::HeaderCrc, "header CRC mismatch");
    }
    const std::uint32_t load_addr = rd_u32(blob, 8);
    const std::uint32_t fw_size = rd_u32(blob, 12);
    const std::uint32_t fw_crc32 = rd_u32(blob, 16);
    const std::uint8_t cur_sec = u8(blob[20]);
    const std::uint8_t tot_sec = u8(blob[21]);
    bytes payload(blob.begin() + static_cast<std::ptrdiff_t>(FWDEPZ_HEADER_SIZE), blob.end());
    if (fw_size != payload.size()) {
        throw FwDepzError(FwDepzErrorKind::Size, "fw_size does not match payload length");
    }
    return FwDepzImage{load_addr, fw_size, fw_crc32, cur_sec, tot_sec, std::move(payload)};
}

bytes FwDepzImage::build(std::uint32_t load_addr, byte_span payload, std::uint8_t cur_sec,
                         std::uint8_t tot_sec) {
    bytes hdr(FWDEPZ_HEADER_SIZE, std::byte{0});
    std::memcpy(hdr.data(), FWDEPZ_MAGIC, 8);
    // load_addr, fw_size, fw_crc32 at offset 8; cur_sec, tot_sec at 20, 21.
    auto put32 = [&](std::size_t off, std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            hdr[off + i] = std::byte{static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF)};
    };
    put32(8, load_addr);
    put32(12, static_cast<std::uint32_t>(payload.size()));
    put32(16, crc32_iso_hdlc(payload));
    hdr[20] = std::byte{cur_sec};
    hdr[21] = std::byte{tot_sec};
    const std::uint16_t crc = crc16_ccitt_false(byte_span(hdr.data(), 62));
    hdr[62] = std::byte{static_cast<std::uint8_t>(crc & 0xFF)};
    hdr[63] = std::byte{static_cast<std::uint8_t>((crc >> 8) & 0xFF)};
    bytes out = std::move(hdr);
    for (std::byte x : payload) out.push_back(x);
    return out;
}

bool FwDepzImage::payload_crc_ok() const {
    return crc32_iso_hdlc(payload) == fw_crc32;
}

}  // namespace depz
