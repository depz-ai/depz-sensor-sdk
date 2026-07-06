#include "depz/sr04.hpp"

#include "depz/detail/byteio.hpp"

namespace depz {

using detail::rd_u16;
using detail::rd_u32;
using detail::rd_u64;
using detail::require;
using detail::u8;
using detail::wr_le;

Sr04Data Sr04Data::unpack(byte_span p) {
    require(p, 11, "Sr04Data payload too short");
    return Sr04Data{u8(p[0]), rd_u64(p, 1), rd_u16(p, 9)};
}

bytes pack_sample_period(std::uint32_t period_us) {
    bytes out;
    wr_le(out, period_us, 4);
    return out;
}

std::uint32_t unpack_sample_period(byte_span p) {
    require(p, 4, "sample_period payload too short");
    return rd_u32(p, 0);
}

bytes pack_echo_decay(std::uint16_t decay_us) {
    bytes out;
    wr_le(out, decay_us, 2);
    return out;
}

std::uint16_t unpack_echo_decay(byte_span p) {
    require(p, 2, "echo_decay payload too short");
    return rd_u16(p, 0);
}

std::optional<double> distance_mm_from_echo(std::uint16_t echo_time_us,
                                            std::optional<double> air_temp_c) {
    if (echo_time_us == ECHO_TIMEOUT) return std::nullopt;
    const double c_m_s = air_temp_c ? (331.3 + 0.606 * *air_temp_c) : 343.0;
    return echo_time_us * c_m_s / 2000.0;
}

}  // namespace depz
