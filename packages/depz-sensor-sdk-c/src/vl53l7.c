/*
 * VL53L5CX / VL53L7CX / VL53L7CH I2C register-bridge codecs + class
 * resolution (contract 11, a delta against contract 04).
 *
 * Port of the hardware-proven Python reference
 * (depz_sensor_sdk/protocol/vl53l7.py and discovery.py::_vl53l7_class).
 * Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are the VL53L8 bridge;
 * this file adds only what the I2C board changes: PIN_CTRL, GET_INFO,
 * SET_I2C_SPEED, RPT_VL53_INFO and the tighter transfer ceilings. The frame
 * decode (footer at size-4, per-zone trim, CNH block) lives with the shared
 * VL53L8 walk in vl53l8_decode.c (depz_vl53l7_decode_frame).
 */
#include "depz_sensor_sdk.h"
#include <string.h>

/* ---- little-endian readers/writers ----------------------------------------*/
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* ---- encoders --------------------------------------------------------------*/
size_t depz_vl53l7_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out)
{
    if (len < 1 || len > DEPZ_VL53L7_READ_MAX_LEN || (uint32_t)addr + len > 0x10000u)
        return 0;
    put_u16(out, addr);
    put_u16(out + 2, len);
    return 4;
}

size_t depz_vl53l7_pack_write_reg(uint16_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out)
{
    if (data_len < 1 || data_len > DEPZ_VL53L7_WRITE_MAX_LEN ||
        (uint32_t)addr + data_len > 0x10000u)
        return 0;
    put_u16(out, addr);
    memcpy(out + 2, data, data_len);
    return 2 + data_len;
}

size_t depz_vl53l7_pack_pin_ctrl(uint8_t action, uint8_t *out)
{
    out[0] = action;
    return 1;
}

size_t depz_vl53l7_pack_set_i2c_speed(uint16_t khz, uint8_t *out)
{
    put_u16(out, khz);
    return 2;
}

/* ---- report decoders -------------------------------------------------------*/
int depz_vl53l7_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53l7_info *out)
{
    /* `<IIIBBBHHB`, no echoed command byte. */
    if (len < DEPZ_VL53L7_INFO_SIZE)
        return -1;
    out->int_edges      = rd_u32(payload);
    out->frames_dropped = rd_u32(payload + 4);
    out->i2c_errors     = rd_u32(payload + 8);
    out->last_i2c_error = payload[12];
    out->lpn_level      = payload[13];
    out->int_level      = payload[14];
    out->i2c_khz        = rd_u16(payload + 15);
    out->frame_size     = rd_u16(payload + 17);
    out->streaming      = payload[19] != 0;
    return 0;
}

/* ---- class resolution (contract 11 §1) -------------------------------------*/
const char *depz_vl53l7_model_str(depz_vl53l7_model m)
{
    switch (m) {
    case DEPZ_VL53L7_MODEL_L5CX: return "vl53l5cx";
    case DEPZ_VL53L7_MODEL_L7CH: return "vl53l7ch";
    case DEPZ_VL53L7_MODEL_L7CX:
    default:                     return "vl53l7cx";
    }
}

depz_vl53l7_model depz_vl53l7_resolve_model(const char *usb_model,
                                            const char *device_name)
{
    /* 1. the production USB PID model. */
    if (usb_model) {
        if (strcmp(usb_model, "vl53l5cx") == 0) return DEPZ_VL53L7_MODEL_L5CX;
        if (strcmp(usb_model, "vl53l7cx") == 0) return DEPZ_VL53L7_MODEL_L7CX;
        if (strcmp(usb_model, "vl53l7ch") == 0) return DEPZ_VL53L7_MODEL_L7CH;
    }
    /* 2. the first match of `VL53L([57])(CX|CH)` in the device name. A match
     *    naming a part with no class (VL53L5CH) resolves to the default; the
     *    scan does not continue past the first match (regex `search`). */
    if (device_name) {
        for (const char *p = device_name; *p; p++) {
            if (strncmp(p, "VL53L", 5) != 0)
                continue;
            char digit = p[5];
            if (digit != '5' && digit != '7')
                continue;
            if (p[6] != 'C' || (p[7] != 'X' && p[7] != 'H'))
                continue;
            if (digit == '5' && p[7] == 'X') return DEPZ_VL53L7_MODEL_L5CX;
            if (digit == '7' && p[7] == 'H') return DEPZ_VL53L7_MODEL_L7CH;
            return DEPZ_VL53L7_MODEL_L7CX;
        }
    }
    /* 3. the CX base — its blob runs on every L5/L7 part. */
    return DEPZ_VL53L7_MODEL_L7CX;
}
