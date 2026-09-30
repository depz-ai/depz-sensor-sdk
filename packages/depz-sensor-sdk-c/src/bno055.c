/*
 * BNO055 9-axis IMU on the APP_BNO055 register bridge, protocol v0.10
 * (contract 13): the wire codecs, the pure register codecs of §4 (units,
 * CALIB_STAT, calibration profile, axis remap + placements, page-1 configs)
 * and register-window decode to raw integers ("base" level).
 *
 * Port of the hardware-proven Python reference
 * (depz_sensor_sdk/protocol/bno055.py and bno055/regs.py).
 */
#include "depz_sensor_sdk.h"
#include <string.h>

/* ---- little-endian readers/writers ----------------------------------------*/
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  rd_i16(const uint8_t *p) { return (int16_t)rd_u16(p); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t rd_u64(const uint8_t *p)
{
    return (uint64_t)rd_u32(p) | ((uint64_t)rd_u32(p + 4) << 32);
}
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* ---- encoders --------------------------------------------------------------*/
size_t depz_bno055_pack_read_reg(uint8_t addr, uint8_t len, uint8_t *out)
{
    out[0] = addr;
    out[1] = len;
    return 2;
}

size_t depz_bno055_pack_write_reg(uint8_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out)
{
    if (data_len < 1 || data_len > DEPZ_BNO055_XFER_MAX)
        return 0;
    out[0] = addr;
    memcpy(out + 1, data, data_len);
    return 1 + data_len;
}

size_t depz_bno055_pack_start_stream(uint8_t trigger, uint8_t addr, uint8_t len,
                                     uint16_t period_ms, uint8_t *out)
{
    out[0] = trigger;
    out[1] = addr;
    out[2] = len;
    put_u16(out + 3, period_ms);
    return 5;
}

/* ---- report decoders -------------------------------------------------------*/
int depz_bno055_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_bno055_reg_data *out)
{
    if (len < 9)
        return -1;
    out->cmd = payload[0];
    out->timestamp_us = rd_u64(payload + 1);
    out->data = payload + 9;
    out->data_len = len - 9;
    return 0;
}

int depz_bno055_unpack_info(const uint8_t *payload, size_t len,
                            depz_bno055_info *out)
{
    /* `<BBBBBHBBBIHHHIIIHBBH` */
    if (len < DEPZ_BNO055_INFO_SIZE)
        return -1;
    out->i2c_addr       = payload[0];
    out->chip_id        = payload[1];
    out->acc_id         = payload[2];
    out->mag_id         = payload[3];
    out->gyr_id         = payload[4];
    out->sw_rev         = rd_u16(payload + 5);
    out->bl_rev         = payload[7];
    out->initialized    = payload[8];
    out->int_level      = payload[9];
    out->int_edges      = rd_u32(payload + 10);
    out->read_min_us    = rd_u16(payload + 14);
    out->read_max_us    = rd_u16(payload + 16);
    out->read_avg_us    = rd_u16(payload + 18);
    out->tx_dropped     = rd_u32(payload + 20);
    out->i2c_errors     = rd_u32(payload + 24);
    out->slots_skipped  = rd_u32(payload + 28);
    out->bus_recoveries = rd_u16(payload + 32);
    out->last_i2c_error = payload[34];
    out->sensor_resets  = payload[35];
    out->loop_max_us    = rd_u16(payload + 36);
    return 0;
}

int depz_bno055_unpack_stream(const uint8_t *payload, size_t len,
                              depz_bno055_stream *out)
{
    /* `<QBB` + data[len] */
    if (len < 10)
        return -1;
    uint8_t block_len = payload[9];
    if (len < (size_t)10 + block_len)
        return -1;
    out->timestamp_us = rd_u64(payload);
    out->addr = payload[8];
    out->len = block_len;
    out->data = payload + 10;
    return 0;
}

/* ---- units (§4.2) ----------------------------------------------------------*/
void depz_bno055_unpack_units(uint8_t unit_sel, depz_bno055_units *out)
{
    out->accel_mg  = (unit_sel & DEPZ_BNO055_UNIT_ACC_MG) != 0;
    out->gyro_rps  = (unit_sel & DEPZ_BNO055_UNIT_GYR_RPS) != 0;
    out->euler_rad = (unit_sel & DEPZ_BNO055_UNIT_EUL_RAD) != 0;
    out->temp_f    = (unit_sel & DEPZ_BNO055_UNIT_TEMP_F) != 0;
    out->android   = (unit_sel & DEPZ_BNO055_UNIT_ORI_ANDROID) != 0;
}

uint8_t depz_bno055_pack_units(const depz_bno055_units *u)
{
    return (uint8_t)((u->accel_mg  ? DEPZ_BNO055_UNIT_ACC_MG : 0u) |
                     (u->gyro_rps  ? DEPZ_BNO055_UNIT_GYR_RPS : 0u) |
                     (u->euler_rad ? DEPZ_BNO055_UNIT_EUL_RAD : 0u) |
                     (u->temp_f    ? DEPZ_BNO055_UNIT_TEMP_F : 0u) |
                     (u->android   ? DEPZ_BNO055_UNIT_ORI_ANDROID : 0u));
}

double depz_bno055_accel_lsb(const depz_bno055_units *u) { return u->accel_mg ? 1.0 : 100.0; }
double depz_bno055_gyro_lsb(const depz_bno055_units *u)  { return u->gyro_rps ? 900.0 : 16.0; }
double depz_bno055_euler_lsb(const depz_bno055_units *u) { return u->euler_rad ? 900.0 : 16.0; }
/* degrees F: 1 LSB = 2 F, so 0.5 LSB per unit. */
double depz_bno055_temp_lsb(const depz_bno055_units *u)  { return u->temp_f ? 0.5 : 1.0; }

/* ---- calibration (§4.3) ----------------------------------------------------*/
void depz_bno055_unpack_calib_status(uint8_t value, depz_bno055_calib_status *out)
{
    out->system = (uint8_t)(value >> 6 & 3);
    out->gyro   = (uint8_t)(value >> 4 & 3);
    out->accel  = (uint8_t)(value >> 2 & 3);
    out->mag    = (uint8_t)(value & 3);
}

uint8_t depz_bno055_pack_calib_status(const depz_bno055_calib_status *s)
{
    return (uint8_t)((s->system & 3) << 6 | (s->gyro & 3) << 4 |
                     (s->accel & 3) << 2 | (s->mag & 3));
}

bool depz_bno055_fully_calibrated(const depz_bno055_calib_status *s)
{
    return s->system == 3 && s->gyro == 3 && s->accel == 3 && s->mag == 3;
}

int depz_bno055_unpack_calib_profile(const uint8_t *data, size_t len,
                                     depz_bno055_calib_profile *out)
{
    /* `<11h`: acc offset xyz, mag offset xyz, gyr offset xyz, acc/mag radius */
    if (len != DEPZ_BNO055_CALIB_PROFILE_LEN)
        return -1;
    for (int i = 0; i < 3; i++) {
        out->accel_offset[i] = rd_i16(data + 2 * i);
        out->mag_offset[i]   = rd_i16(data + 6 + 2 * i);
        out->gyro_offset[i]  = rd_i16(data + 12 + 2 * i);
    }
    out->accel_radius = rd_i16(data + 18);
    out->mag_radius   = rd_i16(data + 20);
    return 0;
}

size_t depz_bno055_pack_calib_profile(const depz_bno055_calib_profile *p,
                                      uint8_t *out)
{
    for (int i = 0; i < 3; i++) {
        put_u16(out + 2 * i, (uint16_t)p->accel_offset[i]);
        put_u16(out + 6 + 2 * i, (uint16_t)p->mag_offset[i]);
        put_u16(out + 12 + 2 * i, (uint16_t)p->gyro_offset[i]);
    }
    put_u16(out + 18, (uint16_t)p->accel_radius);
    put_u16(out + 20, (uint16_t)p->mag_radius);
    return DEPZ_BNO055_CALIB_PROFILE_LEN;
}

/* ---- axis remap (§4.4) -----------------------------------------------------*/
void depz_bno055_unpack_axis_remap(uint8_t config, uint8_t sign,
                                   depz_bno055_axis_remap *out)
{
    out->x = (uint8_t)(config & 3);
    out->y = (uint8_t)(config >> 2 & 3);
    out->z = (uint8_t)(config >> 4 & 3);
    out->x_negative = (sign & 4) != 0;
    out->y_negative = (sign & 2) != 0;
    out->z_negative = (sign & 1) != 0;
}

int depz_bno055_pack_axis_remap(const depz_bno055_axis_remap *a,
                                uint8_t *config, uint8_t *sign)
{
    /* A permutation of 0/1/2: each axis used exactly once. */
    unsigned seen = 0;
    const uint8_t axes[3] = { a->x, a->y, a->z };
    for (int i = 0; i < 3; i++) {
        if (axes[i] > DEPZ_BNO055_AXIS_Z)
            return -1;
        seen |= 1u << axes[i];
    }
    if (seen != 7u)
        return -1;
    *config = (uint8_t)(a->z << 4 | a->y << 2 | a->x);
    *sign = (uint8_t)((a->x_negative ? 4 : 0) | (a->y_negative ? 2 : 0) |
                      (a->z_negative ? 1 : 0));
    return 0;
}

/* Datasheet §3.4: placement -> (AXIS_MAP_CONFIG, AXIS_MAP_SIGN). */
const uint8_t DEPZ_BNO055_PLACEMENTS[8][2] = {
    { 0x21, 0x04 }, /* P0 */
    { 0x24, 0x00 }, /* P1 (default) */
    { 0x24, 0x06 }, /* P2 */
    { 0x21, 0x02 }, /* P3 */
    { 0x24, 0x03 }, /* P4 */
    { 0x21, 0x01 }, /* P5 */
    { 0x21, 0x07 }, /* P6 */
    { 0x24, 0x05 }, /* P7 */
};

int depz_bno055_placement(const char *name, depz_bno055_axis_remap *out)
{
    if (!name || (name[0] != 'P' && name[0] != 'p') ||
        name[1] < '0' || name[1] > '7' || name[2] != '\0')
        return -1;
    const uint8_t *p = DEPZ_BNO055_PLACEMENTS[name[1] - '0'];
    depz_bno055_unpack_axis_remap(p[0], p[1], out);
    return 0;
}

/* ---- page-1 sensor configuration --------------------------------------------*/
void depz_bno055_unpack_accel_config(uint8_t value, depz_bno055_accel_config *out)
{
    out->range     = (uint8_t)(value & 3);
    out->bandwidth = (uint8_t)(value >> 2 & 7);
    out->power     = (uint8_t)(value >> 5 & 7);
}

uint8_t depz_bno055_pack_accel_config(const depz_bno055_accel_config *c)
{
    return (uint8_t)((c->power & 7) << 5 | (c->bandwidth & 7) << 2 | (c->range & 3));
}

void depz_bno055_unpack_gyro_config(const uint8_t bytes[2], depz_bno055_gyro_config *out)
{
    out->range     = (uint8_t)(bytes[0] & 7);
    out->bandwidth = (uint8_t)(bytes[0] >> 3 & 7);
    out->power     = (uint8_t)(bytes[1] & 7);
}

size_t depz_bno055_pack_gyro_config(const depz_bno055_gyro_config *c, uint8_t out[2])
{
    out[0] = (uint8_t)((c->bandwidth & 7) << 3 | (c->range & 7));
    out[1] = (uint8_t)(c->power & 7);
    return 2;
}

void depz_bno055_unpack_mag_config(uint8_t value, depz_bno055_mag_config *out)
{
    out->rate  = (uint8_t)(value & 7);
    out->mode  = (uint8_t)(value >> 3 & 3);
    out->power = (uint8_t)(value >> 5 & 3);
}

uint8_t depz_bno055_pack_mag_config(const depz_bno055_mag_config *c)
{
    return (uint8_t)((c->power & 3) << 5 | (c->mode & 3) << 3 | (c->rate & 7));
}

/* ---- register-window decode (§4.1) -------------------------------------------*/

/* True when the window [addr, end) covers reg..reg+n-1; `*off` = reg - addr. */
static bool covers(unsigned addr, size_t end, unsigned reg, unsigned n, size_t *off)
{
    if (addr > reg || (size_t)reg + n > end)
        return false;
    *off = reg - addr;
    return true;
}

static bool words(unsigned addr, const uint8_t *data, size_t end, unsigned reg,
                  unsigned n, int16_t *out)
{
    size_t off;
    if (!covers(addr, end, reg, 2 * n, &off))
        return false;
    for (unsigned i = 0; i < n; i++)
        out[i] = rd_i16(data + off + 2 * i);
    return true;
}

void depz_bno055_decode_block(uint8_t addr, const uint8_t *data, size_t len,
                              depz_bno055_block *out)
{
    memset(out, 0, sizeof(*out));
    size_t end = (size_t)addr + len;
    size_t off;
    out->has_accel        = words(addr, data, end, DEPZ_BNO055_REG_ACC_DATA, 3, out->accel);
    out->has_mag          = words(addr, data, end, DEPZ_BNO055_REG_MAG_DATA, 3, out->mag);
    out->has_gyro         = words(addr, data, end, DEPZ_BNO055_REG_GYR_DATA, 3, out->gyro);
    out->has_euler        = words(addr, data, end, DEPZ_BNO055_REG_EUL_DATA, 3, out->euler);
    out->has_quaternion   = words(addr, data, end, DEPZ_BNO055_REG_QUA_DATA, 4, out->quaternion);
    out->has_linear_accel = words(addr, data, end, DEPZ_BNO055_REG_LIA_DATA, 3, out->linear_accel);
    out->has_gravity      = words(addr, data, end, DEPZ_BNO055_REG_GRV_DATA, 3, out->gravity);
    if (covers(addr, end, DEPZ_BNO055_REG_TEMP, 1, &off)) {
        out->has_temperature = true;
        out->temperature = (int8_t)data[off];
    }
    if (covers(addr, end, DEPZ_BNO055_REG_CALIB_STAT, 1, &off)) {
        out->has_calib_stat = true;
        out->calib_stat = data[off];
    }
}
