/*
 * VL53L4CD register-bridge wire codecs + host-ULD math (contract 10).
 *
 * Faithful port of the hardware-proven Python reference
 * (depz_sensor_sdk/protocol/vl53l4.py and depz_sensor_sdk/vl53l4/uld.py,
 * itself a port of ST ULD 2.2.3 VL53L4CD_api.c). Register sequences and the
 * timing math keep the C driver's integer widths — the uint32 truncations and
 * the 1.055/1.065 PLL double factors are contract-frozen in
 * contracts/vectors/vl53l4.json; do not "simplify" them.
 */
#include "depz_sensor_sdk.h"
#include <math.h>

/* ---- little-endian readers/writers ----------------------------------------*/
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t rd_u64(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* Register contents are big-endian (the bridge passes sensor bytes through). */
static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

/* ---- encoders --------------------------------------------------------------*/
size_t depz_vl53l4_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out)
{
    put_u16(out, addr);
    put_u16(out + 2, len);
    return 4;
}

size_t depz_vl53l4_pack_write_reg(uint16_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out)
{
    if (data_len < 1 || data_len > DEPZ_VL53L4_XFER_MAX)
        return 0;
    put_u16(out, addr);
    for (size_t i = 0; i < data_len; i++)
        out[2 + i] = data[i];
    return 2 + data_len;
}

size_t depz_vl53l4_pack_xshut(uint8_t action, uint8_t *out)
{
    out[0] = action;
    return 1;
}

size_t depz_vl53l4_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags,
                                     uint8_t *out)
{
    put_u16(out, addr);
    put_u16(out + 2, len);
    out[4] = flags;
    return 5;
}

size_t depz_vl53l4_pack_set_i2c_speed(uint16_t khz, uint8_t *out)
{
    put_u16(out, khz);
    return 2;
}

/* ---- report decoders -------------------------------------------------------*/
int depz_vl53l4_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_vl53l4_reg_data *out)
{
    if (len < 9)
        return -1;
    out->cmd = payload[0];
    out->timestamp_us = rd_u64(payload + 1);
    out->data = payload + 9;
    out->data_len = len - 9;
    return 0;
}

int depz_vl53l4_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53l4_info *out)
{
    if (len < 21)
        return -1;
    out->int_edges      = rd_u32(payload);
    out->slots_skipped  = rd_u32(payload + 4);
    out->i2c_errors     = rd_u32(payload + 8);
    out->last_i2c_error = payload[12];
    out->model_id       = rd_u16(payload + 13);
    out->fw_status      = payload[15];
    out->initialized    = payload[16];
    out->xshut_level    = payload[17];
    out->int_level      = payload[18];
    out->i2c_khz        = rd_u16(payload + 19);
    return 0;
}

int depz_vl53l4_unpack_stream(const uint8_t *payload, size_t len,
                              depz_vl53l4_stream *out)
{
    if (len < 12)
        return -1;
    uint16_t block_len = rd_u16(payload + 10);
    if (len < (size_t)12 + block_len)
        return -1;
    out->timestamp_us = rd_u64(payload);
    out->addr = rd_u16(payload + 8);
    out->len = block_len;
    out->data = payload + 12;
    return 0;
}

/* ---- result-block decode ---------------------------------------------------*/

/* GetResult() raw status -> ULD status (status_rtn[24] in VL53L4CD_api.c);
 * raw 9 -> 0 valid, raw >= 24 passes through unmapped. */
static const uint8_t STATUS_RTN[24] = {
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13,
    255, 255, 255, 255, 10, 6, 255, 255, 11, 12
};

int depz_vl53l4_parse_result_block(const uint8_t *raw, size_t len,
                                   depz_vl53l4_result *out)
{
    if (len < 15)
        return -1;

    int status = raw[0] & 0x1F;
    if (status < (int)(sizeof(STATUS_RTN)))
        status = STATUS_RTN[status];

    int raw_spads = rd_be16(raw + 3);            /* 0x008C */
    int signal_kcps = rd_be16(raw + 5) * 8;      /* 0x008E */
    int ambient_kcps = rd_be16(raw + 7) * 8;     /* 0x0090 */

    out->range_status = status;
    out->stream_count = raw[2];
    out->number_of_spad = raw_spads / 256;
    out->signal_rate_kcps = signal_kcps;
    out->ambient_rate_kcps = ambient_kcps;
    out->sigma_mm = rd_be16(raw + 9) / 4;        /* 0x0092 */
    out->distance_mm = rd_be16(raw + 13);        /* 0x0096 */
    out->signal_per_spad_kcps =
        raw_spads ? signal_kcps * 256 / raw_spads : 0;
    out->ambient_per_spad_kcps =
        raw_spads ? ambient_kcps * 256 / raw_spads : 0;
    return 0;
}

/* ---- SetRangeTiming / GetRangeTiming register math -------------------------*/

int depz_vl53l4_range_timing_registers(uint32_t budget_ms, uint32_t inter_ms,
                                       uint16_t osc_frequency, uint16_t clock_pll,
                                       uint16_t *range_config_a,
                                       uint16_t *range_config_b,
                                       uint32_t *intermeasurement_raw)
{
    if (osc_frequency == 0)
        return -1;
    if (budget_ms < 10 || budget_ms > 200)
        return -1;

    uint32_t timing_budget_us = budget_ms * 1000u;
    uint32_t macro_period_us =
        (uint32_t)(2304u * (0x40000000u / osc_frequency)) >> 6;

    uint32_t inter_raw;
    if (inter_ms == 0) { /* continuous */
        inter_raw = 0;
        timing_budget_us -= 2500u;
    } else if (inter_ms > budget_ms) { /* autonomous low power */
        double factor = 1.055 * (double)inter_ms * (double)(clock_pll & 0x3FFu);
        inter_raw = (uint32_t)factor; /* truncation toward zero, as the C ULD */
        timing_budget_us = (timing_budget_us - 4300u) / 2u;
    } else {
        return -1; /* inter_ms must be 0 or > budget_ms */
    }

    timing_budget_us <<= 12; /* uint32 wrap, as the C ULD */

    uint16_t words[2];
    static const uint32_t MULT[2] = { 16u, 12u }; /* RANGE_CONFIG_A, _B */
    for (int w = 0; w < 2; w++) {
        uint32_t tmp = (uint32_t)(macro_period_us * MULT[w]) >> 6;
        if (tmp == 0)
            return -1; /* degenerate osc_frequency (wrapped to zero) */
        uint32_t ls_byte = (timing_budget_us + (tmp >> 1)) / tmp - 1u;
        uint32_t ms_byte = 0;
        while (ls_byte & 0xFFFFFF00u) {
            ls_byte >>= 1;
            ms_byte++;
        }
        words[w] = (uint16_t)(((ms_byte << 8) + (ls_byte & 0xFFu)) & 0xFFFFu);
    }
    *range_config_a = words[0];
    *range_config_b = words[1];
    *intermeasurement_raw = inter_raw;
    return 0;
}

int depz_vl53l4_decode_range_timing(uint32_t intermeasurement_raw,
                                    uint16_t clock_pll, uint16_t osc_frequency,
                                    uint16_t range_config_a,
                                    uint32_t *budget_ms, uint32_t *inter_ms)
{
    if (osc_frequency == 0)
        return -1;

    uint32_t pll =
        (uint32_t)(1.065 * (double)(clock_pll & 0x3FFu)) & 0xFFFFu;
    uint32_t inter = pll ? ((intermeasurement_raw / pll) & 0xFFFFu) : 0u;

    uint32_t macro_period_us =
        (uint32_t)(2304u * (0x40000000u / osc_frequency)) >> 6;
    uint32_t ls_byte = (uint32_t)(range_config_a & 0x00FFu) << 4;
    uint32_t ms_byte = (uint32_t)(range_config_a & 0xFF00u) >> 8;
    ms_byte = 0x04u - (ms_byte - 1u) - 1u; /* natural uint32 wrap */
    macro_period_us = (uint32_t)(macro_period_us * 16u);

    uint32_t budget =
        (uint32_t)(((ls_byte + 1u) * (macro_period_us >> 6)) -
                   ((macro_period_us >> 6) >> 1)) >> 12;
    if (ms_byte < 12u)
        budget >>= ms_byte;
    budget = (intermeasurement_raw == 0u) ? budget + 2500u
                                          : budget * 2u + 4300u;
    *budget_ms = budget / 1000u;
    *inter_ms = inter;
    return 0;
}

/* ---- tuning word codecs ----------------------------------------------------*/

uint16_t depz_vl53l4_offset_raw(int32_t mm)
{
    return (uint16_t)((uint32_t)mm * 4u);
}

int32_t depz_vl53l4_decode_offset(uint16_t raw)
{
    uint16_t temp = (uint16_t)((uint16_t)(raw << 3) >> 5);
    return (temp > 1024) ? (int32_t)temp - 2048 : (int32_t)temp;
}

uint16_t depz_vl53l4_xtalk_raw(uint16_t kcps)
{
    return (uint16_t)(kcps << 9);
}

uint16_t depz_vl53l4_decode_xtalk(uint16_t raw)
{
    return (uint16_t)lround((double)raw / 512.0);
}

uint16_t depz_vl53l4_signal_threshold_raw(uint16_t kcps)
{
    return (uint16_t)(kcps >> 3);
}

uint16_t depz_vl53l4_decode_signal_threshold(uint16_t raw)
{
    return (uint16_t)(raw << 3);
}

int depz_vl53l4_sigma_threshold_raw(uint16_t mm, uint16_t *raw)
{
    if (mm > (0xFFFFu >> 2))
        return -1;
    *raw = (uint16_t)(mm << 2);
    return 0;
}

uint16_t depz_vl53l4_decode_sigma_threshold(uint16_t raw)
{
    return (uint16_t)(raw >> 2);
}

/* ---- init configuration block ----------------------------------------------*/

/* VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87, as
 * shipped by ST (byte 0 = 0x00; depz_vl53l4_config_block forces it to 0x12). */
const uint8_t DEPZ_VL53L4_DEFAULT_CONFIGURATION[91] = {
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   /* 0x2D..0x34 */
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   /* 0x35..0x3C */
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   /* 0x3D..0x44 */
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   /* 0x45..0x4C */
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   /* 0x4D..0x54 */
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   /* 0x55..0x5C */
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,   /* 0x5D..0x64 */
    0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,   /* 0x65..0x6C */
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 0x6D..0x74 */
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   /* 0x75..0x7C */
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,   /* 0x7D..0x84 */
    0x01, 0x00, 0x00                                   /* 0x85..0x87 */
};

size_t depz_vl53l4_config_block(uint8_t *out)
{
    out[0] = DEPZ_VL53L4_CONFIG_FMP_BYTE;
    for (size_t i = 1; i < sizeof(DEPZ_VL53L4_DEFAULT_CONFIGURATION); i++)
        out[i] = DEPZ_VL53L4_DEFAULT_CONFIGURATION[i];
    return sizeof(DEPZ_VL53L4_DEFAULT_CONFIGURATION);
}
