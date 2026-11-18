#include "depz/common.hpp"

#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {

using detail::u8p;

namespace {

void require(int rc, const char* what) {
    if (rc != 0) throw std::invalid_argument(what);
}

}  // namespace

StatusReport StatusReport::unpack(byte_span p) {
    depz_status_report c{};
    require(depz_unpack_status(u8p(p), p.size(), &c), "StatusReport payload too short");
    return StatusReport{c.cmd, c.status};
}

TextReport TextReport::unpack(byte_span p) {
    if (p.empty()) throw std::invalid_argument("TextReport payload too short");
    std::uint8_t cmd = 0;
    std::size_t text_len = 0;
    std::string text(3 * (p.size() - 1) + 1, '\0');  // always holds the stripped text whole
    require(depz_unpack_text_ex(u8p(p), p.size(), &cmd, text.data(), text.size(), &text_len),
            "TextReport payload too short");
    text.resize(text_len);
    return TextReport{cmd, std::move(text)};
}

SyncTimeReport SyncTimeReport::unpack(byte_span p) {
    depz_sync_time_report c{};
    require(depz_unpack_sync_time(u8p(p), p.size(), &c), "SyncTimeReport payload too short");
    return SyncTimeReport{c.pc_timestamp_us, c.mcu_rx_us, c.mcu_tx_us};
}

TemperatureReport TemperatureReport::unpack(byte_span p) {
    depz_temperature_report c{};
    require(depz_unpack_temperature(u8p(p), p.size(), &c), "TemperatureReport payload too short");
    return TemperatureReport{c.timestamp_us, c.raw_decidegrees};
}

SequenceErrorReport SequenceErrorReport::unpack(byte_span p) {
    depz_sequence_error_report c{};
    require(depz_unpack_sequence_error(u8p(p), p.size(), &c), "SequenceErrorReport payload too short");
    return SequenceErrorReport{c.expected_seq, c.received_seq};
}

bytes SyncPinConfig::pack() const {
    const depz_sync_pin_config c{pin, static_cast<std::uint8_t>(mode), static_cast<std::uint8_t>(polarity)};
    std::uint8_t out[3];
    const std::size_t n = depz_pack_sync_pin_config(&c, out);
    // contract 02 §6: logical pins 1..5, modes 0..4, polarity 0..1
    if (n == 0) throw std::out_of_range("SyncPinConfig: pin is not 1..5, mode not a SyncPinMode or polarity not a SyncPinPolarity");
    return detail::to_bytes(out, n);
}

SyncPinConfig SyncPinConfig::unpack(byte_span p) {
    depz_sync_pin_config c{};
    require(depz_unpack_sync_pin_config(u8p(p), p.size(), &c), "SyncPinConfig payload too short");
    return SyncPinConfig{c.pin, static_cast<SyncPinMode>(c.mode), static_cast<SyncPinPolarity>(c.polarity)};
}

bytes pack_sync_time(std::uint64_t pc_timestamp_us) {
    std::uint8_t out[8];
    return detail::to_bytes(out, depz_pack_sync_time(pc_timestamp_us, out));
}

bytes pack_set_payload_crc_type(std::uint8_t crc_type) {
    std::uint8_t out[1];
    const std::size_t n = depz_pack_set_payload_crc_type(crc_type, out);
    // contract 02 §2: 0..3 — refused before anything is written (07 §3)
    if (n == 0) throw std::out_of_range("SET_PAYLOAD_CRC_TYPE: crc_type is not a CrcType 0..3");
    return bytes(1, std::byte{out[0]});  // the one byte (a range copy of a 1-byte array trips gcc's -O2 bounds analysis)
}

std::pair<std::int64_t, std::int64_t> sync_time_offset_rtt(std::int64_t t1, std::int64_t t2,
                                                           std::int64_t t3, std::int64_t t4) {
    std::int64_t offset = 0, rtt = 0;
    depz_sync_time_offset_rtt(t1, t2, t3, t4, &offset, &rtt);
    return {offset, rtt};
}

std::string strip_device_string(byte_span raw) {
    const std::size_t need = depz_strip_device_string(u8p(raw), raw.size(), nullptr, 0);
    std::string out(need, '\0');
    // the C codec NUL-terminates: the string's own terminator is that byte
    if (need) depz_strip_device_string(u8p(raw), raw.size(), out.data(), need + 1);
    return out;
}

}  // namespace depz
