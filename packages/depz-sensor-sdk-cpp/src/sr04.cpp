#include "depz/sr04.hpp"

#include <cmath>
#include <stdexcept>

#include "detail/c_bridge.hpp"

namespace depz {

using detail::u8p;

Sr04Data Sr04Data::unpack(byte_span p) {
    depz_sr04_data c{};
    if (depz_sr04_unpack_data(u8p(p), p.size(), &c) != 0) throw std::invalid_argument("Sr04Data payload too short");
    return Sr04Data{c.source_cmd, c.timestamp_us, c.echo_time_us};
}

bytes pack_sample_period(std::uint32_t period_us) {
    std::uint8_t out[4];
    return detail::to_bytes(out, depz_sr04_pack_sample_period(period_us, out));
}

std::uint32_t unpack_sample_period(byte_span p) {
    std::uint32_t v = 0;
    if (depz_sr04_unpack_sample_period(u8p(p), p.size(), &v) != 0)
        throw std::invalid_argument("sample period payload too short");
    return v;
}

bytes pack_echo_decay(std::uint16_t decay_us) {
    std::uint8_t out[2];
    return detail::to_bytes(out, depz_sr04_pack_echo_decay(decay_us, out));
}

std::uint16_t unpack_echo_decay(byte_span p) {
    std::uint16_t v = 0;
    if (depz_sr04_unpack_echo_decay(u8p(p), p.size(), &v) != 0)
        throw std::invalid_argument("echo decay payload too short");
    return v;
}

std::optional<double> distance_mm_from_echo(std::uint16_t echo_time_us, std::optional<double> air_temp_c) {
    double mm = 0.0;
    if (!depz_sr04_distance_mm(echo_time_us, air_temp_c.value_or(0.0), air_temp_c.has_value(), &mm))
        return std::nullopt;
    return mm;
}

}  // namespace depz
