/* SR04 wire codecs (contract 03). */
#include "depz_sensor_sdk.h"

static void put_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

size_t depz_sr04_pack_sample_period(uint32_t period_us, uint8_t *out)
{
    put_u32_le(out, period_us);
    return 4;
}

size_t depz_sr04_pack_echo_decay(uint16_t decay_us, uint8_t *out)
{
    out[0] = (uint8_t)(decay_us & 0xFFu);
    out[1] = (uint8_t)((decay_us >> 8) & 0xFFu);
    return 2;
}

int depz_sr04_unpack_data(const uint8_t *p, size_t len, depz_sr04_data *out)
{
    /* cmd u8, timestamp_us u64, echo_time_us u16 — 11 B */
    if (len != 11)
        return -1;
    out->source_cmd = p[0];
    uint64_t ts = 0;
    for (int i = 0; i < 8; i++)
        ts |= (uint64_t)p[1 + i] << (8 * i);
    out->timestamp_us = ts;
    out->echo_time_us = (uint16_t)((uint16_t)p[9] | ((uint16_t)p[10] << 8));
    return 0;
}

int depz_sr04_unpack_sample_period(const uint8_t *p, size_t len, uint32_t *out)
{
    if (len != 4)
        return -1;
    *out = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return 0;
}

int depz_sr04_unpack_echo_decay(const uint8_t *p, size_t len, uint16_t *out)
{
    if (len != 2)
        return -1;
    *out = (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    return 0;
}

bool depz_sr04_distance_mm(uint16_t echo_time_us, double air_temp_c,
                           bool have_temp, double *out_mm)
{
    if (echo_time_us == DEPZ_SR04_ECHO_TIMEOUT)
        return false;
    double c = have_temp ? (331.3 + 0.606 * air_temp_c) : 343.0;
    if (out_mm)
        *out_mm = (double)echo_time_us * c / 2000.0;
    return true;
}
