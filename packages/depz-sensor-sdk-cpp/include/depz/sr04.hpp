// SR04 wire codecs (contracts/03_SENSOR_SR04.md).
#pragma once

#include <cstdint>
#include <optional>

#include "depz/span.hpp"

namespace depz {

enum class Sr04Cmd : std::uint8_t {
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
};

enum class Sr04Rpt : std::uint8_t {
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
};

// echo_time_us sentinel: no echo received.
inline constexpr std::uint16_t ECHO_TIMEOUT = 0xFFFF;

inline constexpr std::uint32_t SAMPLE_PERIOD_DEFAULT_US = 50000;
inline constexpr std::uint16_t ECHO_DECAY_DEFAULT_US = 5000;
inline constexpr std::uint16_t ECHO_DECAY_MIN_US = 4000;
inline constexpr std::uint16_t ECHO_DECAY_MAX_US = 65000;

struct Sr04Data {
    std::uint8_t source_cmd;  // 0x36 single shot (host or SYNC_IN), 0x37 loop sample
    std::uint64_t timestamp_us;
    std::uint16_t echo_time_us;
    static Sr04Data unpack(byte_span payload);
};

bytes pack_sample_period(std::uint32_t period_us);
std::uint32_t unpack_sample_period(byte_span payload);
bytes pack_echo_decay(std::uint16_t decay_us);
std::uint16_t unpack_echo_decay(byte_span payload);

// Round-trip echo time -> distance in mm; nullopt for the timeout sentinel.
// Default speed of sound 343 m/s; with air_temp_c uses c = 331.3 + 0.606*T.
std::optional<double> distance_mm_from_echo(std::uint16_t echo_time_us,
                                            std::optional<double> air_temp_c = std::nullopt);

}  // namespace depz
