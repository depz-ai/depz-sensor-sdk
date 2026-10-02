/* vl53lx_die.c — the drivers of the VL53L1 die: the VL53L4CD ULD 2.2.3 body
 * (uld/vl53l1_die.py VL53L1Die) with VL53L4CD / VL53L4CX (uld/l4.py) and the
 * VL53L3CX ULP 1.0.0 (uld/l3.py) on it, and the VL53L1X ULD 3.5.5 for
 * VL53L1CX / VL53L1CB (uld/l1.py). A faithful port of the Python ports —
 * register sequences and integer widths included; do not "simplify". */
#include "vl53lx_internal.h"

#include <math.h>
#include <stdlib.h>

/* The register map of the die (uld/vl53l1_die.py). */
#define SOFT_RESET                            0x0000
#define OSC_FREQUENCY                         0x0006
#define VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND 0x0008
#define XTALK_PLANE_OFFSET_KCPS               0x0016
#define XTALK_X_PLANE_GRADIENT_KCPS           0x0018
#define XTALK_Y_PLANE_GRADIENT_KCPS           0x001A
#define RANGE_OFFSET_MM                       0x001E
#define INNER_OFFSET_MM                       0x0020
#define OUTER_OFFSET_MM                       0x0022
#define GPIO_HV_MUX__CTRL                     0x0030
#define GPIO__TIO_HV_STATUS                   0x0031
#define SYSTEM__INTERRUPT_CONFIG_GPIO         0x0046
#define PHASECAL_CONFIG__TIMEOUT_MACROP       0x004B
#define RANGE_CONFIG__TIMEOUT_MACROP_A_HI     0x005E
#define RANGE_CONFIG__VCSEL_PERIOD_A          0x0060
#define RANGE_CONFIG__TIMEOUT_MACROP_B_HI     0x0061
#define RANGE_CONFIG__VCSEL_PERIOD_B          0x0063
#define RANGE_CONFIG__SIGMA_THRESH            0x0064
#define MIN_COUNT_RATE_RTN_LIMIT_MCPS         0x0066
#define RANGE_CONFIG__VALID_PHASE_HIGH        0x0069
#define INTERMEASUREMENT_MS                   0x006C
#define THRESH_HIGH                           0x0072
#define THRESH_LOW                            0x0074
#define SD_CONFIG__WOI_SD0                    0x0078
#define SD_CONFIG__INITIAL_PHASE_SD0          0x007A
#define ROI_CONFIG__USER_ROI_CENTRE_SPAD      0x007F
#define ROI_CONFIG__USER_ROI_XY_SIZE          0x0080
#define SYSTEM__INTERRUPT_CLEAR               0x0086
#define SYSTEM__MODE_START                    0x0087
#define RESULT__OSC_CALIBRATE_VAL             0x00DE
#define FIRMWARE__SYSTEM_STATUS               0x00E5
#define IDENTIFICATION__MODEL_ID              0x010F
#define ROI_CONFIG__MODE_ROI_CENTRE_SPAD      0x013E

#define CONFIG_ADDR     0x002D
#define CONFIG_LEN      91
#define CONFIG_FMP_BYTE 0x12

/* VL53L4CD_DEFAULT_CONFIGURATION[] (uld/l4.py). */
static const uint8_t L4_CONFIG[CONFIG_LEN] = {
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

/* VL53L3CX_ULP_DEFAULT_CONFIGURATION[] (uld/l3.py). */
static const uint8_t L3_CONFIG[CONFIG_LEN] = {
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x00, 0x01, 0x07, 0x00, 0x02, 0x05, 0x00, 0xb4, 0x00, 0xbb, 0x08, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

/* VL51L1X_DEFAULT_CONFIGURATION[] (uld/l1.py). */
static const uint8_t L1_CONFIG[CONFIG_LEN] = {
    0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01, 0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

/* UM2931 "Range status description" (uld/vl53l1_die.py RANGE_STATUS_NAMES). */
static const char *status_text(int status)
{
    switch (status) {
    case 0:   return "valid";
    case 1:   return "sigma above threshold";
    case 2:   return "signal below threshold";
    case 3:   return "distance below detection threshold";
    case 4:   return "phase out of valid limit";
    case 5:   return "hardware fail";
    case 6:   return "no wrap-around check done";
    case 7:   return "wrapped target, phase mismatch";
    case 8:   return "processing fail";
    case 9:   return "crosstalk signal fail";
    case 10:  return "interrupt error";
    case 11:  return "merged target";
    case 12:  return "signal too low";
    case 255: return "other error";
    default:  return "unknown";
    }
}

typedef enum { DIE_L4, DIE_L3, DIE_L1 } die_kind;

typedef struct {
    vlx_driver base;
    die_kind   kind;
    const char *applied_mode;   /* L1: the mode last put in, for budget_choices */
} die_drv;

static die_drv *dd(vlx_driver *d) { return (die_drv *)d; }
#define P (d->p)

/* ── the result block (VL53L1ResultBlock) ──────────────────────────────── */

static void stream_block(vlx_driver *d, uint16_t *addr, uint16_t *len)
{
    (void)d;
    *addr = DEPZ_VL53LX_DIE_BLOCK_ADDR;
    *len = DEPZ_VL53LX_DIE_BLOCK_LEN;
}

/* parse_result_block(): the die block as this ULD reads it. */
static int parse_block(vlx_driver *d, const uint8_t *raw, size_t len, depz_vl53l4_result *r)
{
    if (depz_vl53lx_decode_die_block(raw, len, dd(d)->kind == DIE_L1 ? DEPZ_VL53LX_DIE_L1 : DEPZ_VL53LX_DIE_L4, r))
        return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: result block too short: %zu bytes", len);
    return DEPZ_OK;
}

static int get_result(vlx_driver *d, depz_vl53l4_result *r)
{
    uint8_t raw[DEPZ_VL53LX_DIE_BLOCK_LEN];
    VLX_TRY(vlx_rd_multi(P, DEPZ_VL53LX_DIE_BLOCK_ADDR, raw, sizeof raw));
    return parse_block(d, raw, sizeof raw, r);
}

static void as_measurement(const depz_vl53l4_result *r, depz_vl53lx_measurement *m)
{
    m->distance_mm = r->distance_mm;
    m->status = r->range_status;
    m->status_text = status_text(r->range_status);
    m->signal_kcps = r->signal_rate_kcps;
    m->ambient_kcps = r->ambient_rate_kcps;
    m->sigma_mm = r->sigma_mm;
    m->spads = r->number_of_spad;
    m->signal_per_spad_kcps = r->signal_per_spad_kcps;
    m->ambient_per_spad_kcps = r->ambient_per_spad_kcps;
    m->stream_count = r->stream_count;
}

static int decode(vlx_driver *d, const uint8_t *raw, size_t len, depz_vl53lx_measurement *m)
{
    depz_vl53l4_result r;
    if (raw) VLX_TRY(parse_block(d, raw, len, &r));
    else VLX_TRY(get_result(d, &r));
    as_measurement(&r, m);
    return DEPZ_OK;
}

/* ── identity and boot ─────────────────────────────────────────────────── */

static int model_id(vlx_driver *d, uint16_t *out) { return vlx_rd_word(P, IDENTIFICATION__MODEL_ID, out); }

static int wait_boot(vlx_driver *d, int timeout_ms)
{
    uint64_t deadline = vlx_deadline_ms(timeout_ms);
    for (;;) {
        uint8_t v;
        VLX_TRY(vlx_rd_byte(P, FIRMWARE__SYSTEM_STATUS, &v));
        if (v == 0x03) return DEPZ_OK;
        if (vlx_past(deadline)) return depz_fail(DEPZ_E_TIMEOUT, "vl53lx: timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
        vlx_sleep_ms(P, 1);
    }
}

/* reset_device(): the soft reset, or XSHUT when the die no longer ACKs. */
static int reset_device(vlx_driver *d)
{
    int rc = vlx_wr_byte(P, SOFT_RESET, 0x00);
    if (!rc) {
        vlx_sleep_ms(P, 1);
        rc = vlx_wr_byte(P, SOFT_RESET, 0x01);
    }
    if (!rc) rc = wait_boot(d, 1000);
    if (rc == DEPZ_OK) return DEPZ_OK;
    if (!vlx_is_protocol_error(rc) && rc != DEPZ_E_PROTOCOL) return rc;
    VLX_TRY(vlx_xshut_reset(P));
    return wait_boot(d, 1000);
}

static int clear_interrupt(vlx_driver *d) { return vlx_wr_byte(P, SYSTEM__INTERRUPT_CLEAR, 0x01); }

static int start_ranging(vlx_driver *d)
{
    uint32_t im;
    if (dd(d)->kind == DIE_L1) return vlx_wr_byte(P, SYSTEM__MODE_START, 0x40);
    VLX_TRY(vlx_rd_dword(P, INTERMEASUREMENT_MS, &im));
    return vlx_wr_byte(P, SYSTEM__MODE_START, im == 0 ? 0x21 : 0x40);
}

static int stop_ranging(vlx_driver *d)
{
    return vlx_wr_byte(P, SYSTEM__MODE_START, dd(d)->kind == DIE_L4 ? 0x80 : 0x00);
}

static int get_interrupt_polarity(vlx_driver *d, int *pol)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, GPIO_HV_MUX__CTRL, &v));
    *pol = (v & 0x10) ? 0 : 1;
    return DEPZ_OK;
}

static int set_interrupt_polarity(vlx_driver *d, int polarity)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, GPIO_HV_MUX__CTRL, &v));
    v &= 0xEF;
    return vlx_wr_byte(P, GPIO_HV_MUX__CTRL, (uint8_t)(v | ((!(polarity & 1)) << 4)));
}

static int check_for_data_ready(vlx_driver *d, bool *ready)
{
    int pol;
    uint8_t v;
    VLX_TRY(get_interrupt_polarity(d, &pol));
    VLX_TRY(vlx_rd_byte(P, GPIO__TIO_HV_STATUS, &v));
    *ready = (v & 1) == pol;
    return DEPZ_OK;
}

static int set_range_timing(vlx_driver *d, int budget_ms, int inter_ms);

static int sensor_init(vlx_driver *d)
{
    die_drv *s = dd(d);
    uint8_t config[CONFIG_LEN];
    const uint8_t *blob = s->kind == DIE_L4 ? L4_CONFIG : s->kind == DIE_L3 ? L3_CONFIG : L1_CONFIG;
    VLX_TRY(vlx_set_addr_width(P, 2));
    VLX_TRY(vlx_set_i2c_speed(P, VLX_I2C_KHZ_BOOT));
    /* init_boot(): the L1X ULD resets first, the others just wait. */
    VLX_TRY(s->kind == DIE_L1 ? reset_device(d) : wait_boot(d, 1000));
    memcpy(config, blob, CONFIG_LEN);
    config[0] = CONFIG_FMP_BYTE;
    VLX_TRY(vlx_wr_multi(P, CONFIG_ADDR, config, CONFIG_LEN));
    VLX_TRY(vlx_set_i2c_speed(P, 1000));
    /* init_after_config(): the L1X blob boots the interrupt active-high. */
    if (s->kind == DIE_L1) VLX_TRY(set_interrupt_polarity(d, 0));
    VLX_TRY(vlx_wr_byte(P, SYSTEM__MODE_START, 0x40)); /* start VHV */
    VLX_TRY(vlx_wait_data_ready(d, 1000));
    VLX_TRY(clear_interrupt(d));
    VLX_TRY(stop_ranging(d));
    VLX_TRY(vlx_wr_byte(P, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09));
    VLX_TRY(vlx_wr_byte(P, 0x000B, 0x00));
    /* init_extra() */
    if (s->kind == DIE_L4) {
        VLX_TRY(vlx_wr_word(P, 0x0024, 0x0500));
    } else if (s->kind == DIE_L3) {
        VLX_TRY(vlx_wr_word(P, 0x0024, 0x0500));
        VLX_TRY(vlx_wr_byte(P, 0x0081, 0x8A));
        VLX_TRY(vlx_wr_byte(P, PHASECAL_CONFIG__TIMEOUT_MACROP, 0x03));
    } else {
        s->applied_mode = "long";
    }
    return set_range_timing(d, 50, 0);
}

/* ── timing ────────────────────────────────────────────────────────────── */

/* VL53L1Die.set_range_timing (the L4CD ULD arithmetic, L4 and L3). */
static int die_set_range_timing(vlx_driver *d, int budget_ms, int inter_ms)
{
    uint16_t osc, pll;
    uint32_t budget_us, macro_period_us;
    int k;
    static const uint16_t regs[2] = {RANGE_CONFIG__TIMEOUT_MACROP_A_HI, RANGE_CONFIG__TIMEOUT_MACROP_B_HI};
    static const uint32_t mults[2] = {16, 12};
    VLX_TRY(vlx_rd_word(P, OSC_FREQUENCY, &osc));
    if (osc == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: osc_frequency reads 0");
    if (budget_ms < 10 || budget_ms > 200) return depz_fail(DEPZ_E_ARG, "vl53lx: timing_budget_ms must be 10..200");
    budget_us = (uint32_t)budget_ms * 1000u;
    macro_period_us = (uint32_t)(2304u * (0x40000000u / osc)) >> 6;
    if (inter_ms == 0) {
        VLX_TRY(vlx_wr_dword(P, INTERMEASUREMENT_MS, 0));
        budget_us -= 2500;
    } else if (inter_ms > budget_ms) {
        double factor;
        VLX_TRY(vlx_rd_word(P, RESULT__OSC_CALIBRATE_VAL, &pll));
        pll &= 0x3FF;
        factor = 1.055 * inter_ms * pll;
        VLX_TRY(vlx_wr_dword(P, INTERMEASUREMENT_MS, (uint32_t)(int64_t)factor));
        budget_us = (budget_us - 4300) / 2;
    } else {
        return depz_fail(DEPZ_E_ARG, "vl53lx: inter_measurement_ms must be 0 or > timing_budget_ms");
    }
    budget_us <<= 12;
    for (k = 0; k < 2; k++) {
        uint32_t tmp = (macro_period_us * mults[k]) >> 6;
        uint32_t ls_byte = ((budget_us + (tmp >> 1)) / tmp) - 1;
        uint32_t ms_byte = 0;
        while (ls_byte & 0xFFFFFF00u) {
            ls_byte >>= 1;
            ms_byte++;
        }
        VLX_TRY(vlx_wr_word(P, regs[k], (uint16_t)((ms_byte << 8) + (ls_byte & 0xFF))));
    }
    return DEPZ_OK;
}

static int die_get_range_timing(vlx_driver *d, int *budget_ms, int *inter_ms)
{
    uint32_t tmp, macro_period_us, ls_byte, ms_byte, budget;
    uint16_t pll, osc, macrop;
    uint32_t clock_pll;
    VLX_TRY(vlx_rd_dword(P, INTERMEASUREMENT_MS, &tmp));
    VLX_TRY(vlx_rd_word(P, RESULT__OSC_CALIBRATE_VAL, &pll));
    clock_pll = (uint32_t)(int64_t)(1.065 * (pll & 0x3FF)) & 0xFFFF;
    *inter_ms = clock_pll ? (int)((tmp / clock_pll) & 0xFFFF) : 0;
    VLX_TRY(vlx_rd_word(P, OSC_FREQUENCY, &osc));
    if (osc == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: osc_frequency reads 0");
    VLX_TRY(vlx_rd_word(P, RANGE_CONFIG__TIMEOUT_MACROP_A_HI, &macrop));
    macro_period_us = (uint32_t)(2304u * (0x40000000u / osc)) >> 6;
    ls_byte = (uint32_t)(macrop & 0x00FF) << 4;
    ms_byte = (uint32_t)(macrop & 0xFF00) >> 8;
    ms_byte = 0x04u - (ms_byte - 1u) - 1u;
    macro_period_us *= 16u;
    budget = (((ls_byte + 1u) * (macro_period_us >> 6)) - ((macro_period_us >> 6) >> 1)) >> 12;
    if (ms_byte < 12) budget >>= ms_byte;
    budget = tmp == 0 ? budget + 2500u : budget * 2u + 4300u;
    *budget_ms = (int)(budget / 1000u);
    return DEPZ_OK;
}

/* The L1X ULD: a tabulated budget per distance mode. */
typedef struct { int ms; uint16_t a, b; } l1_budget;

static const l1_budget L1_SHORT[] = {
    {15, 0x001D, 0x0027}, {20, 0x0051, 0x006E}, {33, 0x00D6, 0x006E}, {50, 0x01AE, 0x01E8},
    {100, 0x02E1, 0x0388}, {200, 0x03E1, 0x0496}, {500, 0x0591, 0x05C1}};
static const l1_budget L1_LONG[] = {
    {20, 0x001E, 0x0022}, {33, 0x0060, 0x006E}, {50, 0x00AD, 0x00C6},
    {100, 0x01CC, 0x01EA}, {200, 0x02D9, 0x02F8}, {500, 0x048F, 0x04A4}};
#define N_SHORT (sizeof L1_SHORT / sizeof L1_SHORT[0])
#define N_LONG  (sizeof L1_LONG / sizeof L1_LONG[0])

static int l1_mode_code(vlx_driver *d, bool *is_short)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, PHASECAL_CONFIG__TIMEOUT_MACROP, &v));
    if (v == 0x14) { *is_short = true; return DEPZ_OK; }
    if (v == 0x0A) { *is_short = false; return DEPZ_OK; }
    return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x%02X, which is neither distance mode", v);
}

static const l1_budget *l1_find(bool is_short, int ms)
{
    const l1_budget *t = is_short ? L1_SHORT : L1_LONG;
    size_t n = is_short ? N_SHORT : N_LONG, i;
    for (i = 0; i < n; i++)
        if (t[i].ms == ms) return &t[i];
    return NULL;
}

static int l1_set_range_timing(vlx_driver *d, int budget_ms, int inter_ms)
{
    bool is_short;
    const l1_budget *b;
    uint16_t pll;
    VLX_TRY(l1_mode_code(d, &is_short));
    b = l1_find(is_short, budget_ms);
    if (!b) return depz_fail(DEPZ_E_ARG, "vl53lx: timing budget %d ms is not one the %s mode has", budget_ms,
                             is_short ? "short" : "long");
    VLX_TRY(vlx_wr_word(P, RANGE_CONFIG__TIMEOUT_MACROP_A_HI, b->a));
    VLX_TRY(vlx_wr_word(P, RANGE_CONFIG__TIMEOUT_MACROP_B_HI, b->b));
    if (inter_ms == 0) inter_ms = budget_ms;
    VLX_TRY(vlx_rd_word(P, RESULT__OSC_CALIBRATE_VAL, &pll));
    pll &= 0x3FF;
    return vlx_wr_dword(P, INTERMEASUREMENT_MS, (uint32_t)(int64_t)(pll * inter_ms * 1.075));
}

static int l1_get_range_timing(vlx_driver *d, int *budget_ms, int *inter_ms)
{
    uint16_t a, pll;
    uint32_t tmp;
    size_t i;
    VLX_TRY(vlx_rd_word(P, RANGE_CONFIG__TIMEOUT_MACROP_A_HI, &a));
    *budget_ms = 0;
    for (i = 0; i < N_SHORT; i++) if (L1_SHORT[i].a == a) *budget_ms = L1_SHORT[i].ms;
    for (i = 0; i < N_LONG; i++) if (L1_LONG[i].a == a) *budget_ms = L1_LONG[i].ms;
    VLX_TRY(vlx_rd_dword(P, INTERMEASUREMENT_MS, &tmp));
    VLX_TRY(vlx_rd_word(P, RESULT__OSC_CALIBRATE_VAL, &pll));
    pll &= 0x3FF;
    *inter_ms = pll ? (int)(int64_t)(tmp / (pll * 1.065)) : 0;
    return DEPZ_OK;
}

static int set_range_timing(vlx_driver *d, int budget_ms, int inter_ms)
{
    return dd(d)->kind == DIE_L1 ? l1_set_range_timing(d, budget_ms, inter_ms) : die_set_range_timing(d, budget_ms, inter_ms);
}

static int get_range_timing(vlx_driver *d, int *budget_ms, int *inter_ms)
{
    return dd(d)->kind == DIE_L1 ? l1_get_range_timing(d, budget_ms, inter_ms) : die_get_range_timing(d, budget_ms, inter_ms);
}

/* ── L1 distance modes ─────────────────────────────────────────────────── */

static const char *const L1_MODES[2] = {"long", "short"};

static size_t l1_modes(const vlx_driver *d, const char **names, size_t cap)
{
    size_t i;
    (void)d;
    for (i = 0; i < 2 && i < cap; i++) names[i] = L1_MODES[i];
    return 2;
}

static size_t l1_budget_choices(vlx_driver *d, int *out, size_t cap)
{
    bool is_short = !strcmp(dd(d)->applied_mode, "short");
    const l1_budget *t = is_short ? L1_SHORT : L1_LONG;
    size_t n = is_short ? N_SHORT : N_LONG, i;
    for (i = 0; i < n && i < cap; i++) out[i] = t[i].ms;
    return n;
}

static int l1_set_mode(vlx_driver *d, const char *name)
{
    int budget_ms, inter_ms;
    bool is_short;
    const l1_budget *t;
    size_t n, i;
    if (strcmp(name, "short") && strcmp(name, "long"))
        return depz_fail(DEPZ_E_ARG, "vl53lx: no such mode: %s (have long, short)", name);
    is_short = !strcmp(name, "short");
    VLX_TRY(l1_get_range_timing(d, &budget_ms, &inter_ms));
    dd(d)->applied_mode = is_short ? L1_MODES[1] : L1_MODES[0];
    if (is_short) {
        VLX_TRY(vlx_wr_byte(P, PHASECAL_CONFIG__TIMEOUT_MACROP, 0x14));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VCSEL_PERIOD_A, 0x07));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VCSEL_PERIOD_B, 0x05));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VALID_PHASE_HIGH, 0x38));
        VLX_TRY(vlx_wr_word(P, SD_CONFIG__WOI_SD0, 0x0705));
        VLX_TRY(vlx_wr_word(P, SD_CONFIG__INITIAL_PHASE_SD0, 0x0606));
    } else {
        VLX_TRY(vlx_wr_byte(P, PHASECAL_CONFIG__TIMEOUT_MACROP, 0x0A));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VCSEL_PERIOD_A, 0x0F));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VCSEL_PERIOD_B, 0x0D));
        VLX_TRY(vlx_wr_byte(P, RANGE_CONFIG__VALID_PHASE_HIGH, 0xB8));
        VLX_TRY(vlx_wr_word(P, SD_CONFIG__WOI_SD0, 0x0F0D));
        VLX_TRY(vlx_wr_word(P, SD_CONFIG__INITIAL_PHASE_SD0, 0x0E0E));
    }
    /* 15 ms exists in short mode only: fall back to the nearest budget
     * (Python's min() keeps the first of two equally near ones). */
    if (!l1_find(is_short, budget_ms)) {
        t = is_short ? L1_SHORT : L1_LONG;
        n = is_short ? N_SHORT : N_LONG;
        {
            int best = t[0].ms;
            for (i = 1; i < n; i++)
                if (abs(t[i].ms - budget_ms) < abs(best - budget_ms)) best = t[i].ms;
            budget_ms = best;
        }
    }
    return l1_set_range_timing(d, budget_ms, inter_ms);
}

static int l1_get_mode(vlx_driver *d, const char **name)
{
    bool is_short;
    VLX_TRY(l1_mode_code(d, &is_short));
    *name = is_short ? L1_MODES[1] : L1_MODES[0];
    return DEPZ_OK;
}

/* ── thresholds, offset, crosstalk, ROI, temperature ───────────────────── */

static int set_detection_thresholds(vlx_driver *d, int low, int high, int window)
{
    if (dd(d)->kind == DIE_L1) {
        uint8_t v;
        VLX_TRY(vlx_rd_byte(P, SYSTEM__INTERRUPT_CONFIG_GPIO, &v));
        v = (uint8_t)((v & ~0x6F & 0xFF) | window);
        VLX_TRY(vlx_wr_byte(P, SYSTEM__INTERRUPT_CONFIG_GPIO, v));
    } else {
        VLX_TRY(vlx_wr_byte(P, SYSTEM__INTERRUPT_CONFIG_GPIO, (uint8_t)window));
    }
    VLX_TRY(vlx_wr_word(P, THRESH_HIGH, (uint16_t)high));
    return vlx_wr_word(P, THRESH_LOW, (uint16_t)low);
}

static int get_detection_thresholds(vlx_driver *d, int *low, int *high, int *window)
{
    uint16_t h, l;
    uint8_t w;
    VLX_TRY(vlx_rd_word(P, THRESH_HIGH, &h));
    VLX_TRY(vlx_rd_word(P, THRESH_LOW, &l));
    VLX_TRY(vlx_rd_byte(P, SYSTEM__INTERRUPT_CONFIG_GPIO, &w));
    *low = l;
    *high = h;
    *window = w & 0x07;
    return DEPZ_OK;
}

static int set_signal_threshold(vlx_driver *d, int kcps)
{
    return vlx_wr_word(P, MIN_COUNT_RATE_RTN_LIMIT_MCPS, (uint16_t)(kcps >> 3));
}

static int get_signal_threshold(vlx_driver *d, int *kcps)
{
    uint16_t v;
    VLX_TRY(vlx_rd_word(P, MIN_COUNT_RATE_RTN_LIMIT_MCPS, &v));
    *kcps = (v << 3) & 0xFFFF;
    return DEPZ_OK;
}

static int set_sigma_threshold(vlx_driver *d, int mm)
{
    if (mm > (0xFFFF >> 2) || mm < 0) return depz_fail(DEPZ_E_ARG, "vl53lx: sigma_mm must be <= 16383");
    return vlx_wr_word(P, RANGE_CONFIG__SIGMA_THRESH, (uint16_t)(mm << 2));
}

static int get_sigma_threshold(vlx_driver *d, int *mm)
{
    uint16_t v;
    VLX_TRY(vlx_rd_word(P, RANGE_CONFIG__SIGMA_THRESH, &v));
    *mm = v >> 2;
    return DEPZ_OK;
}

static int set_offset(vlx_driver *d, int32_t mm)
{
    VLX_TRY(vlx_wr_word(P, RANGE_OFFSET_MM, (uint16_t)((mm * 4) & 0xFFFF)));
    VLX_TRY(vlx_wr_word(P, INNER_OFFSET_MM, 0));
    return vlx_wr_word(P, OUTER_OFFSET_MM, 0);
}

static int get_offset(vlx_driver *d, int32_t *mm)
{
    uint16_t v;
    int32_t temp;
    VLX_TRY(vlx_rd_word(P, RANGE_OFFSET_MM, &v));
    temp = (int32_t)(((uint32_t)v << 3) & 0xFFFF) >> 5;
    *mm = temp > 1024 ? temp - 2048 : temp;
    return DEPZ_OK;
}

static int set_xtalk(vlx_driver *d, int32_t kcps)
{
    VLX_TRY(vlx_wr_word(P, XTALK_X_PLANE_GRADIENT_KCPS, 0x0000));
    VLX_TRY(vlx_wr_word(P, XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000));
    return vlx_wr_word(P, XTALK_PLANE_OFFSET_KCPS, (uint16_t)((kcps << 9) & 0xFFFF));
}

static int get_xtalk(vlx_driver *d, int32_t *kcps)
{
    uint16_t v;
    VLX_TRY(vlx_rd_word(P, XTALK_PLANE_OFFSET_KCPS, &v));
    *kcps = (int32_t)nearbyint(v / 512.0); /* round half to even, as Python */
    return DEPZ_OK;
}

static int set_roi(vlx_driver *d, int x, int y)
{
    uint8_t center;
    VLX_TRY(vlx_rd_byte(P, ROI_CONFIG__MODE_ROI_CENTRE_SPAD, &center));
    if (x > 16) x = 16;
    if (y > 16) y = 16;
    if (x > 10 || y > 10) center = 199;
    VLX_TRY(vlx_wr_byte(P, ROI_CONFIG__USER_ROI_CENTRE_SPAD, center));
    return vlx_wr_byte(P, ROI_CONFIG__USER_ROI_XY_SIZE, (uint8_t)((y - 1) << 4 | (x - 1)));
}

static int get_roi(vlx_driver *d, int *x, int *y)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, ROI_CONFIG__USER_ROI_XY_SIZE, &v));
    *x = (v & 0x0F) + 1;
    *y = ((v & 0xF0) >> 4) + 1;
    return DEPZ_OK;
}

static int set_roi_center(vlx_driver *d, int spad) { return vlx_wr_byte(P, ROI_CONFIG__USER_ROI_CENTRE_SPAD, (uint8_t)spad); }

static int get_roi_center(vlx_driver *d, int *spad)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, ROI_CONFIG__USER_ROI_CENTRE_SPAD, &v));
    *spad = v;
    return DEPZ_OK;
}

static int start_temperature_update(vlx_driver *d)
{
    VLX_TRY(vlx_wr_byte(P, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81));
    VLX_TRY(vlx_wr_byte(P, 0x000B, 0x92));
    VLX_TRY(start_ranging(d));
    VLX_TRY(vlx_wait_data_ready(d, 1000));
    VLX_TRY(clear_interrupt(d));
    VLX_TRY(stop_ranging(d));
    VLX_TRY(vlx_wr_byte(P, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09));
    return vlx_wr_byte(P, 0x000B, 0x00);
}

/* ── calibration ───────────────────────────────────────────────────────── */

/* _collect(): data-ready, GetResult, ClearInterrupt, nb times (5 s each). */
static int collect(vlx_driver *d, int nb, depz_vl53l4_result *out)
{
    int i;
    VLX_TRY(start_ranging(d));
    for (i = 0; i < nb; i++) {
        depz_vl53l4_result r;
        VLX_TRY(vlx_wait_data_ready(d, 5000));
        VLX_TRY(get_result(d, &r));
        VLX_TRY(clear_interrupt(d));
        if (out) out[i] = r;
    }
    return stop_ranging(d);
}

/* Python's `//` on a possibly negative sum: floor division. */
static int64_t floordiv(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

static int calibrate_offset(vlx_driver *d, int target_mm, int nb, int32_t *out)
{
    depz_vl53l4_result *rs;
    int64_t sum = 0;
    int32_t offset;
    int i, rc;
    bool l1 = dd(d)->kind == DIE_L1;
    if (nb <= 0) nb = l1 ? 50 : 20;
    if (l1) {
        if (nb < 5) return depz_fail(DEPZ_E_ARG, "vl53lx: nb_samples must be at least 5");
    } else if (nb < 5 || nb > 255 || target_mm < 10 || target_mm > 1000) {
        return depz_fail(DEPZ_E_ARG, "vl53lx: nb_samples must be 5..255, target 10..1000 mm");
    }
    VLX_TRY(vlx_wr_word(P, RANGE_OFFSET_MM, 0));
    VLX_TRY(vlx_wr_word(P, INNER_OFFSET_MM, 0));
    VLX_TRY(vlx_wr_word(P, OUTER_OFFSET_MM, 0));
    if (!l1) VLX_TRY(collect(d, 10, NULL)); /* device heat loop */
    rs = (depz_vl53l4_result *)malloc((size_t)nb * sizeof *rs);
    if (!rs) return depz_fail(DEPZ_E_NOMEM, "vl53lx: out of memory");
    rc = collect(d, nb, rs);
    if (!rc) {
        for (i = 0; i < nb; i++) sum += rs[i].distance_mm;
        offset = (int32_t)(target_mm - floordiv(sum, nb));
        rc = vlx_wr_word(P, RANGE_OFFSET_MM, (uint16_t)((offset * 4) & 0xFFFF));
        *out = offset;
    }
    free(rs);
    return rc;
}

static int calibrate_xtalk(vlx_driver *d, int target_mm, int nb, int32_t *out)
{
    depz_vl53l4_result *rs;
    double n = 0, dist = 0, spads = 0, signal = 0;
    int i, rc;
    bool l1 = dd(d)->kind == DIE_L1;
    if (nb <= 0) nb = l1 ? 50 : 20;
    if (l1) {
        if (nb < 5) return depz_fail(DEPZ_E_ARG, "vl53lx: nb_samples must be at least 5");
    } else if (nb < 5 || nb > 255 || target_mm < 10 || target_mm > 5000) {
        return depz_fail(DEPZ_E_ARG, "vl53lx: nb_samples must be 5..255, target 10..5000 mm");
    }
    VLX_TRY(vlx_wr_word(P, XTALK_PLANE_OFFSET_KCPS, 0)); /* disable compensation */
    if (!l1) VLX_TRY(collect(d, 10, NULL));
    rs = (depz_vl53l4_result *)malloc((size_t)nb * sizeof *rs);
    if (!rs) return depz_fail(DEPZ_E_NOMEM, "vl53lx: out of memory");
    rc = collect(d, nb, rs);
    if (rc) { free(rs); return rc; }
    for (i = 0; i < nb; i++) {
        /* The L4CD ULD drops invalid samples and the first frame. */
        if (!l1 && (rs[i].range_status != 0 || i == 0)) continue;
        n += 1;
        dist += rs[i].distance_mm;
        spads += rs[i].number_of_spad;
        signal += rs[i].signal_rate_kcps;
    }
    free(rs);
    if (l1) {
        double avg_d = dist / n, avg_s = spads / n, avg_sig = signal / n;
        int64_t cal;
        if (avg_s == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: xtalk calibration failed: no SPADs enabled");
        cal = (int64_t)(512 * (avg_sig * (1 - avg_d / target_mm)) / avg_s);
        if (cal < 0) cal = 0;
        if (cal > 0xFFFF) cal = 0xFFFF;
        VLX_TRY(vlx_wr_word(P, XTALK_PLANE_OFFSET_KCPS, (uint16_t)cal));
        *out = (int32_t)nearbyint(cal / 512.0);
    } else {
        double tmp;
        if (n == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: xtalk calibration failed: no valid samples");
        tmp = (1.0 - (dist / n) / (double)target_mm) * ((signal / n) / (spads / n));
        if (tmp > 127) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: xtalk calibration failed: %.1f kcps > 127", tmp);
        VLX_TRY(vlx_wr_word(P, XTALK_PLANE_OFFSET_KCPS, (uint16_t)((int64_t)(tmp * 512.0) & 0xFFFF)));
        *out = (int32_t)nearbyint(tmp);
    }
    return DEPZ_OK;
}

/* ── reach (VL53L1Die.reach_mm) ────────────────────────────────────────── */

/* One PLL period is c/2 of distance at the oscillator the die measured for
 * itself (OSC_FREQUENCY, 4.12 MHz): 198.4 mm at the usual 0xBCCC. The same
 * arithmetic as VL53LX_range_maths() in the Bare Driver, which applies this
 * window on the host side in histogram mode. */
uint32_t vlx_phase_window_mm(uint32_t fast_osc_frequency, uint32_t valid_phase_high)
{
    uint64_t pll_period;
    if (fast_osc_frequency == 0) return 0;
    pll_period = (UINT64_C(1) << 30) / fast_osc_frequency; /* us, 0.18 */
    return (uint32_t)(((uint64_t)valid_phase_high * pll_period * (299704u >> 3)) >> 25);
}

/* Read off the loaded configuration, so it follows the blob and the mode:
 * 1388 mm for the 0x38 window of the L4CD, L3CX and L1 short blobs, 4563 mm
 * for the 0xB8 window L1 long writes. */
static int die_reach_mm(vlx_driver *d, uint32_t *out)
{
    uint16_t osc;
    uint8_t vph;
    VLX_TRY(vlx_rd_word(P, OSC_FREQUENCY, &osc));
    if (osc == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: osc_frequency reads 0");
    VLX_TRY(vlx_rd_byte(P, RANGE_CONFIG__VALID_PHASE_HIGH, &vph));
    *out = vlx_phase_window_mm(osc, vph);
    return DEPZ_OK;
}

static void destroy(vlx_driver *d) { free(d); }

/* ── the three drivers ─────────────────────────────────────────────────── */

#define DIE_COMMON                                                                  \
    2, 1, {{SYSTEM__INTERRUPT_CLEAR, 0x01}, {0, 0}}, 1000
#define DIE_MANDATORY                                                               \
    model_id, sensor_init, start_ranging, stop_ranging, check_for_data_ready,       \
    clear_interrupt, stream_block, decode, set_range_timing, get_range_timing, destroy

static const vlx_ops L4_OPS = {
    "VL53L4", DIE_COMMON,
    DEPZ_VL53LX_CAP_TIMING | DEPZ_VL53LX_CAP_OFFSET | DEPZ_VL53LX_CAP_XTALK | DEPZ_VL53LX_CAP_THRESHOLDS |
        DEPZ_VL53LX_CAP_SIGNAL_THRESH | DEPZ_VL53LX_CAP_SIGMA_THRESH | DEPZ_VL53LX_CAP_TEMP_UPDATE |
        DEPZ_VL53LX_CAP_CALIB_OFFSET | DEPZ_VL53LX_CAP_CALIB_XTALK,
    false, 10, 200, DIE_MANDATORY,
    NULL, NULL, NULL, NULL, die_reach_mm, set_offset, get_offset, set_xtalk, get_xtalk, calibrate_offset,
    calibrate_xtalk, set_detection_thresholds, get_detection_thresholds, set_signal_threshold,
    get_signal_threshold, set_sigma_threshold, get_sigma_threshold, NULL, NULL, NULL, NULL,
    start_temperature_update, NULL,
    NULL};

static const vlx_ops L3_OPS = {
    "VL53L3", DIE_COMMON,
    DEPZ_VL53LX_CAP_TIMING | DEPZ_VL53LX_CAP_THRESHOLDS | DEPZ_VL53LX_CAP_SIGNAL_THRESH |
        DEPZ_VL53LX_CAP_SIGMA_THRESH | DEPZ_VL53LX_CAP_ROI,
    false, 10, 200, DIE_MANDATORY,
    NULL, NULL, NULL, NULL, die_reach_mm, NULL, NULL, NULL, NULL, NULL, NULL, set_detection_thresholds,
    get_detection_thresholds, set_signal_threshold, get_signal_threshold, set_sigma_threshold,
    get_sigma_threshold, set_roi, get_roi, set_roi_center, get_roi_center, NULL, NULL,
    NULL};

static const vlx_ops L1_OPS = {
    "VL53L1", DIE_COMMON,
    DEPZ_VL53LX_CAP_MODE | DEPZ_VL53LX_CAP_TIMING | DEPZ_VL53LX_CAP_OFFSET | DEPZ_VL53LX_CAP_XTALK |
        DEPZ_VL53LX_CAP_THRESHOLDS | DEPZ_VL53LX_CAP_SIGNAL_THRESH | DEPZ_VL53LX_CAP_SIGMA_THRESH |
        DEPZ_VL53LX_CAP_TEMP_UPDATE | DEPZ_VL53LX_CAP_CALIB_OFFSET | DEPZ_VL53LX_CAP_CALIB_XTALK |
        DEPZ_VL53LX_CAP_ROI,
    false, 15, 500, DIE_MANDATORY,
    l1_modes, l1_budget_choices, l1_set_mode, l1_get_mode, die_reach_mm, set_offset, get_offset, set_xtalk,
    get_xtalk, calibrate_offset, calibrate_xtalk, set_detection_thresholds, get_detection_thresholds,
    set_signal_threshold, get_signal_threshold, set_sigma_threshold, get_sigma_threshold, set_roi,
    get_roi, set_roi_center, get_roi_center, start_temperature_update, NULL,
    NULL};

static vlx_driver *new_die(vlx_plat *p, depz_vl53lx_product product, die_kind kind, const vlx_ops *ops)
{
    die_drv *s = (die_drv *)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->base.ops = ops;
    s->base.p = p;
    s->base.product = product;
    s->kind = kind;
    s->applied_mode = L1_MODES[0];
    return &s->base;
}

vlx_driver *vlx_new_l4(vlx_plat *p, depz_vl53lx_product product) { return new_die(p, product, DIE_L4, &L4_OPS); }
vlx_driver *vlx_new_l3(vlx_plat *p, depz_vl53lx_product product) { return new_die(p, product, DIE_L3, &L3_OPS); }
vlx_driver *vlx_new_l1(vlx_plat *p, depz_vl53lx_product product) { return new_die(p, product, DIE_L1, &L1_OPS); }
