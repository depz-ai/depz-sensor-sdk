#include "depz/fwdepz.hpp"

#include <cstring>

#include "depz/crc.hpp"
#include "detail/c_bridge.hpp"

namespace depz {

using detail::to_bytes;
using detail::u8p;

FlashInfo FlashInfo::unpack(byte_span p) {
    depz_flash_info c{};
    if (depz_bl_unpack_flash_info(u8p(p), p.size(), &c) != 0) throw std::invalid_argument("FlashInfo payload too short");
    return FlashInfo{c.page_size, c.app_start, c.app_size};
}

bytes pack_write_page(std::uint32_t addr, byte_span data) {
    bytes out(6 + data.size());
    const std::size_t n = depz_bl_pack_write_page(addr, u8p(data), data.size(),
                                                  reinterpret_cast<std::uint8_t*>(out.data()), out.size());
    out.resize(n);
    return out;
}

bytes pack_read_page(std::uint32_t addr, std::uint16_t size) {
    std::uint8_t out[6];
    return to_bytes(out, depz_bl_pack_read_page(addr, size, out));
}

FwDepzImage FwDepzImage::parse(byte_span blob) {
    depz_fwdepz_image c{};
    switch (depz_fwdepz_parse(u8p(blob), blob.size(), &c)) {
        case DEPZ_FWDEPZ_OK: break;
        case DEPZ_FWDEPZ_ERR_TOO_SHORT: throw FwDepzError(FwDepzErrorKind::TooShort, "file too short");
        case DEPZ_FWDEPZ_ERR_MAGIC: throw FwDepzError(FwDepzErrorKind::Magic, "bad magic (not a .fwdepz file)");
        case DEPZ_FWDEPZ_ERR_HEADER_CRC: throw FwDepzError(FwDepzErrorKind::HeaderCrc, "header CRC mismatch");
        case DEPZ_FWDEPZ_ERR_SIZE: throw FwDepzError(FwDepzErrorKind::Size, "fw_size does not match payload length");
        case DEPZ_FWDEPZ_ERR_SECTION:
            // Sections are 0-based; every shipped file is section 0 of 1 (ERRATA B9).
            throw FwDepzError(FwDepzErrorKind::Section,
                              "only section 0 of 1 is supported (multi-section files are not)");
    }
    return FwDepzImage{c.load_addr, c.fw_size, c.fw_crc32, c.cur_sec, c.tot_sec, to_bytes(c.payload, c.payload_len)};
}

// A test / tooling helper the C SDK has no twin of: the header laid out over
// the C CRCs (contract 06 §2).
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

bool FwDepzImage::payload_crc_ok() const { return crc32_iso_hdlc(payload) == fw_crc32; }

}  // namespace depz
