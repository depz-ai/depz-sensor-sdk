#include "depz/common.hpp"

#include "depz/detail/byteio.hpp"

#include <stdexcept>

namespace depz {

using detail::rd_i16;
using detail::rd_u64;
using detail::require;
using detail::u8;
using detail::wr_le;

StatusReport StatusReport::unpack(byte_span p) {
    require(p, 2, "StatusReport payload too short");
    return StatusReport{u8(p[0]), u8(p[1])};
}

TextReport TextReport::unpack(byte_span p) {
    require(p, 1, "TextReport payload too short");
    return TextReport{u8(p[0]), strip_device_string(p.subspan(1))};
}

SyncTimeReport SyncTimeReport::unpack(byte_span p) {
    require(p, 24, "SyncTimeReport payload too short");
    return SyncTimeReport{rd_u64(p, 0), rd_u64(p, 8), rd_u64(p, 16)};
}

TemperatureReport TemperatureReport::unpack(byte_span p) {
    require(p, 10, "TemperatureReport payload too short");
    return TemperatureReport{rd_u64(p, 0), rd_i16(p, 8)};
}

SequenceErrorReport SequenceErrorReport::unpack(byte_span p) {
    require(p, 2, "SequenceErrorReport payload too short");
    return SequenceErrorReport{u8(p[0]), u8(p[1])};
}

bytes SyncPinConfig::pack() const {
    // contract 02 §6: logical pins 1..5, modes 0..4, polarity 0..1
    if (pin < 1 || pin > 5) throw std::out_of_range("SyncPinConfig: pin is not a sync pin 1..5");
    if (static_cast<std::uint8_t>(mode) > 4) throw std::out_of_range("SyncPinConfig: mode is not a SyncPinMode");
    if (static_cast<std::uint8_t>(polarity) > 1)
        throw std::out_of_range("SyncPinConfig: polarity is not a SyncPinPolarity");
    bytes out;
    out.push_back(std::byte{pin});
    out.push_back(std::byte{static_cast<std::uint8_t>(mode)});
    out.push_back(std::byte{static_cast<std::uint8_t>(polarity)});
    return out;
}

SyncPinConfig SyncPinConfig::unpack(byte_span p) {
    require(p, 3, "SyncPinConfig payload too short");
    return SyncPinConfig{u8(p[0]), static_cast<SyncPinMode>(u8(p[1])),
                         static_cast<SyncPinPolarity>(u8(p[2]))};
}

bytes pack_sync_time(std::uint64_t pc_timestamp_us) {
    bytes out;
    wr_le(out, pc_timestamp_us, 8);
    return out;
}

bytes pack_set_payload_crc_type(std::uint8_t crc_type) {
    return bytes{std::byte{crc_type}};
}

std::pair<std::int64_t, std::int64_t> sync_time_offset_rtt(std::int64_t t1, std::int64_t t2,
                                                           std::int64_t t3, std::int64_t t4) {
    const std::int64_t num = (t2 - t1) + (t3 - t4);
    // Truncate toward zero (C++ integer division already does this).
    const std::int64_t offset = num / 2;
    const std::int64_t rtt = (t4 - t1) - (t3 - t2);
    return {offset, rtt};
}

std::string strip_device_string(byte_span raw) {
    std::size_t end = raw.size();
    while (end > 0) {
        std::uint8_t c = u8(raw[end - 1]);
        if (c == 0x00 || c == 0xFF) {
            --end;
        } else {
            break;
        }
    }
    // ASCII (contract 02 §4.1): a byte >= 0x80 is U+FFFD (UTF-8 EF BF BD), an
    // embedded 0x00 is kept as a character.
    std::string out;
    out.reserve(end);
    for (std::size_t i = 0; i < end; ++i) {
        const std::uint8_t c = u8(raw[i]);
        if (c < 0x80) out.push_back(static_cast<char>(c));
        else out.append("\xEF\xBF\xBD");
    }
    return out;
}

}  // namespace depz
