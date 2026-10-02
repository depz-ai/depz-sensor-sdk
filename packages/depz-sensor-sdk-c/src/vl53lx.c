/*
 * VL53L 1D family on the APP_VL53L0_4 bridge, protocol v2.00 (contract 12,
 * a delta against contract 10): the new wire codecs, the product table with
 * class resolution, and the stateless block decoders ("base" level).
 *
 * Port of the hardware-proven Python reference
 * (depz_sensor_sdk/protocol/vl53lx.py, vl53lx/decode.py,
 * vl53lx/uld/registry.py and discovery.py::_vl53lx_class). The codecs shared
 * with contract 10 (READ_REG, WRITE_REG, XSHUT, SET_I2C_SPEED, REG_DATA,
 * STREAM) are the depz_vl53l4_* functions, not duplicated here.
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

/* Register contents are big-endian (the bridge passes sensor bytes through). */
static uint16_t rd_be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

/* ---- encoders --------------------------------------------------------------*/
size_t depz_vl53lx_pack_set_addr_width(uint8_t width, uint8_t *out)
{
    if (width != 1 && width != 2)
        return 0;
    out[0] = width;
    return 1;
}

size_t depz_vl53lx_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags,
                                     const depz_vl53lx_clear_step *clear,
                                     size_t n_clear, uint8_t *out)
{
    if (n_clear > DEPZ_VL53LX_CLEAR_STEPS_MAX || (n_clear && !clear))
        return 0;
    put_u16(out, addr);
    put_u16(out + 2, len);
    out[4] = flags;
    out[5] = (uint8_t)n_clear;
    for (size_t i = 0; i < n_clear; i++) {
        put_u16(out + 6 + 3 * i, clear[i].addr);
        out[6 + 3 * i + 2] = clear[i].value;
    }
    return 6 + 3 * n_clear;
}

/* ---- report decoders -------------------------------------------------------*/
int depz_vl53lx_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53lx_info *out)
{
    /* `<IIIBBBHBBI` */
    if (len < DEPZ_VL53LX_INFO_SIZE)
        return -1;
    out->int_edges      = rd_u32(payload);
    out->slots_skipped  = rd_u32(payload + 4);
    out->i2c_errors     = rd_u32(payload + 8);
    out->last_i2c_error = payload[12];
    out->xshut_level    = payload[13];
    out->int_level      = payload[14];
    out->i2c_khz        = rd_u16(payload + 15);
    out->addr_width     = payload[17];
    out->n_clear        = payload[18];
    out->frames_dropped = rd_u32(payload + 19);
    return 0;
}

/* ---- product table (contract 12 §1) ---------------------------------------*/
#define ULD  DEPZ_VL53LX_DRIVER_ULD
#define ULP  DEPZ_VL53LX_DRIVER_ULP
#define HIST DEPZ_VL53LX_DRIVER_HISTOGRAM
/* Die parts: block 0x0089 / 0x0088, interrupt released by 0x0086 <- 1. */
#define DIE_CLEAR 1, { { 0x0086, 0x01 }, { 0, 0 } }

static const depz_vl53lx_product_info PRODUCTS[DEPZ_VL53LX_PRODUCT_COUNT] = {
    /* The only part on its own silicon: 8-bit register addresses, no FM+. */
    { "VL53L0X",  0xED41, 0x00EE, 2000, ULD,        ULD,  1,
      2, { { 0x0B, 0x01 }, { 0x0B, 0x00 } }, 400 },
    { "VL53L1CX", 0xED43, 0xEACC, 4000, ULD | HIST, ULD,  2, DIE_CLEAR, 1000 },
    { "VL53L1CB", 0xED42, 0xEACC, 8000, ULD | HIST, ULD,  2, DIE_CLEAR, 1000 },
    { "VL53L3CX", 0xED44, 0xEAAA, 3000, ULP | HIST, ULP,  2, DIE_CLEAR, 1000 },
    { "VL53L4CD", 0xED45, 0xEBAA, 1300, ULD | HIST, ULD,  2, DIE_CLEAR, 1000 },
    { "VL53L4CX", 0xED46, 0xEBAA, 6000, HIST,       HIST, 2, DIE_CLEAR, 1000 },
};
#undef ULD
#undef ULP
#undef HIST
#undef DIE_CLEAR

const depz_vl53lx_product_info *depz_vl53lx_product_get(depz_vl53lx_product p)
{
    if ((int)p < 0 || p >= DEPZ_VL53LX_PRODUCT_COUNT)
        return NULL;
    return &PRODUCTS[p];
}

const char *depz_vl53lx_driver_str(depz_vl53lx_driver d)
{
    switch (d) {
    case DEPZ_VL53LX_DRIVER_ULD:       return "uld";
    case DEPZ_VL53LX_DRIVER_ULP:       return "ulp";
    case DEPZ_VL53LX_DRIVER_HISTOGRAM: return "histogram";
    default:                           return NULL;
    }
}

static char up(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

/* Case-insensitive compare of s[0..n) with the NUL-terminated upper-case `w`. */
static bool eq_upper_n(const char *s, size_t n, const char *w)
{
    size_t i = 0;
    for (; i < n; i++)
        if (w[i] == '\0' || up(s[i]) != w[i])
            return false;
    return w[i] == '\0';
}

static depz_vl53lx_product product_from_span(const char *s, size_t n)
{
    for (int p = 0; p < DEPZ_VL53LX_PRODUCT_COUNT; p++)
        if (eq_upper_n(s, n, PRODUCTS[p].name))
            return (depz_vl53lx_product)p;
    return DEPZ_VL53LX_PRODUCT_NONE;
}

depz_vl53lx_product depz_vl53lx_product_from_str(const char *name)
{
    if (!name)
        return DEPZ_VL53LX_PRODUCT_NONE;
    return product_from_span(name, strlen(name));
}

depz_vl53lx_product depz_vl53lx_product_from_board_name(const char *name)
{
    /* Regex `VL53L(\d[A-Z0-9]*)` searched in name.upper(): the leftmost
     * match decides; a match that is not a family product is NONE (the scan
     * does not continue past it). */
    if (!name)
        return DEPZ_VL53LX_PRODUCT_NONE;
    for (const char *p = name; *p; p++) {
        if (!eq_upper_n(p, 5, "VL53L") || !(p[5] >= '0' && p[5] <= '9'))
            continue;
        size_t n = 6;
        for (;;) {
            char c = up(p[n]);
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                n++;
            else
                break;
        }
        return product_from_span(p, n);
    }
    return DEPZ_VL53LX_PRODUCT_NONE;
}

depz_vl53lx_product depz_vl53lx_resolve_product(const char *usb_model,
                                                const char *device_name)
{
    depz_vl53lx_product p = depz_vl53lx_product_from_str(usb_model);
    if (p != DEPZ_VL53LX_PRODUCT_NONE)
        return p;
    return depz_vl53lx_product_from_board_name(device_name);
}

depz_vl53lx_class depz_vl53lx_resolve_class(const char *usb_model,
                                            const char *device_name)
{
    switch (depz_vl53lx_resolve_product(usb_model, device_name)) {
    case DEPZ_VL53LX_PRODUCT_L0X:  return DEPZ_VL53LX_CLASS_L0X;
    case DEPZ_VL53LX_PRODUCT_L1CX: return DEPZ_VL53LX_CLASS_L1CX;
    case DEPZ_VL53LX_PRODUCT_L1CB: return DEPZ_VL53LX_CLASS_L1CB;
    case DEPZ_VL53LX_PRODUCT_L3CX: return DEPZ_VL53LX_CLASS_L3CX;
    case DEPZ_VL53LX_PRODUCT_L4CX: return DEPZ_VL53LX_CLASS_L4CX;
    default:                       return DEPZ_VL53LX_CLASS_GENERIC; /* L4CD too */
    }
}

const char *depz_vl53lx_class_str(depz_vl53lx_class c)
{
    switch (c) {
    case DEPZ_VL53LX_CLASS_L0X:  return "vl53l0x";
    case DEPZ_VL53LX_CLASS_L1CX: return "vl53l1cx";
    case DEPZ_VL53LX_CLASS_L1CB: return "vl53l1cb";
    case DEPZ_VL53LX_CLASS_L3CX: return "vl53l3cx";
    case DEPZ_VL53LX_CLASS_L4CX: return "vl53l4cx";
    case DEPZ_VL53LX_CLASS_GENERIC:
    default:                     return "vl53lx";
    }
}

/* ---- stateless block decoders (contract 12 §4) ----------------------------*/
int depz_vl53lx_decode_die_block(const uint8_t *raw, size_t len,
                                 depz_vl53lx_die_variant variant,
                                 depz_vl53l4_result *out)
{
    if (len < DEPZ_VL53LX_DIE_BLOCK_LEN)
        return -1;
    if (variant != DEPZ_VL53LX_DIE_L4 && variant != DEPZ_VL53LX_DIE_L1)
        return -1;
    /* The l4 variant is contract 10's decode exactly. */
    if (depz_vl53l4_parse_result_block(raw, len, out) != 0)
        return -1;
    if (variant == DEPZ_VL53LX_DIE_L1) {
        /* VL53L1X ULD: signal = crosstalk-corrected peak at 0x0098, K = 25. */
        int raw_spads = rd_be16(raw + 3);
        int signal = rd_be16(raw + 15) * 8;
        out->signal_rate_kcps = signal;
        out->signal_per_spad_kcps = raw_spads ? signal * 25 / raw_spads : 0;
        out->ambient_per_spad_kcps =
            raw_spads ? out->ambient_rate_kcps * 25 / raw_spads : 0;
    }
    return 0;
}

int depz_vl53lx_decode_l0x_raw(const uint8_t *raw, size_t len,
                               depz_vl53lx_l0x_raw *out)
{
    if (len < DEPZ_VL53LX_L0X_BLOCK_LEN)
        return -1;
    out->distance_raw = rd_be16(raw + 10);
    out->device_range_status = raw[0];
    out->signal_rate_mcps_1616 = (uint32_t)rd_be16(raw + 6) << 9;
    out->ambient_rate_mcps_1616 = (uint32_t)rd_be16(raw + 8) << 9;
    out->effective_spad_count_88 = rd_be16(raw + 2);
    return 0;
}

/* Offsets inside the 0x0088 histogram block (register - 0x0088). */
#define H_BIN_0_2      (0x008E - 0x0088) /* RESULT__HISTOGRAM_BIN_0_2       */
#define H_BIN_23_0     (0x00D5 - 0x0088) /* RESULT__HISTOGRAM_BIN_23_0      */
#define H_REF_PHASE    (0x00D6 - 0x0088) /* PHASECAL_RESULT__REFERENCE_PHASE */
#define H_VCSEL_START  (0x00D8 - 0x0088) /* PHASECAL_RESULT__VCSEL_START    */
#define H_BIN_23_0_MSB (0x00D9 - 0x0088)
#define H_BIN_23_0_LSB (0x00DA - 0x0088)

int depz_vl53lx_decode_histogram_raw(const uint8_t *raw, size_t len,
                                     depz_vl53lx_histogram_raw *out)
{
    if (len < DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN)
        return -1;
    out->interrupt_status = raw[0];
    out->range_status = raw[1];
    out->report_status = raw[2];
    out->stream_count = raw[3];
    out->dss_actual_effective_spads = rd_be16(raw + 4);
    out->reference_phase = rd_be16(raw + H_REF_PHASE);
    out->vcsel_start = raw[H_VCSEL_START];
    /* Bin 23's low byte lives in the MSB/LSB pair, not at 0x00D5. */
    uint8_t bin23_lo = (uint8_t)((raw[H_BIN_23_0_MSB] << 2) + raw[H_BIN_23_0_LSB]);
    for (unsigned i = 0; i < DEPZ_VL53LX_HISTOGRAM_BINS; i++) {
        const uint8_t *b = raw + H_BIN_0_2 + 3 * i;
        uint8_t lo = (H_BIN_0_2 + 3 * i + 2 == H_BIN_23_0) ? bin23_lo : b[2];
        out->bins[i] = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | lo;
    }
    return 0;
}
