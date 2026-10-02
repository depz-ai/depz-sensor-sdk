/* vl53lx_l0x.c — the VL53L0X API 1.0.4 ranging path (uld/l0x.py): DataInit,
 * StaticInit (reference SPADs from NVM + tuning settings), the reference
 * calibration, the timing budget arithmetic, the ranging profiles, the PAL
 * range status with its sigma estimate and dmax, the offset / crosstalk
 * calibrations and PerformRefSpadManagement. A faithful port of the Python
 * port — register sequences, integer widths and the deliberate overflows
 * included; do not "simplify". Python ints are unbounded, so the arithmetic
 * runs in int64 with the port's own masks, and `//` is floor division. */
#include "vl53lx_internal.h"

#include <math.h>
#include <stdlib.h>

/* Registers (vl53l0x_device.h). */
#define SYSRANGE_START                              0x00
#define SYSRANGE_MODE_SINGLESHOT                    0x00
#define SYSRANGE_MODE_START_STOP                    0x01
#define SYSRANGE_MODE_BACKTOBACK                    0x02
#define SYSRANGE_MODE_TIMED                         0x04
#define SYSTEM_SEQUENCE_CONFIG                      0x01
#define SYSTEM_INTERMEASUREMENT_PERIOD              0x04
#define SYSTEM_RANGE_CONFIG                         0x09
#define SYSTEM_INTERRUPT_CONFIG_GPIO                0x0A
#define SYSTEM_INTERRUPT_CLEAR                      0x0B
#define RESULT_INTERRUPT_STATUS                     0x13
#define RESULT_RANGE_STATUS                         0x14
#define CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       0x20
#define ALGO_PART_TO_PART_RANGE_OFFSET_MM           0x28
#define ALGO_PHASECAL_CONFIG_TIMEOUT                0x30
#define GLOBAL_CONFIG_VCSEL_WIDTH                   0x32
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define MSRC_CONFIG_TIMEOUT_MACROP                  0x46
#define FINAL_RANGE_CONFIG_VALID_PHASE_LOW          0x47
#define FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         0x48
#define DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         0x4E
#define DYNAMIC_SPAD_REF_EN_START_OFFSET            0x4F
#define PRE_RANGE_CONFIG_VCSEL_PERIOD               0x50
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x51
#define PRE_RANGE_CONFIG_VALID_PHASE_LOW            0x56
#define PRE_RANGE_CONFIG_VALID_PHASE_HIGH           0x57
#define MSRC_CONFIG_CONTROL                         0x60
#define PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          0x64
#define FINAL_RANGE_CONFIG_VCSEL_PERIOD             0x70
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        0x71
#define POWER_MANAGEMENT_GO1_POWER_FORCE            0x80
#define GPIO_HV_MUX_ACTIVE_HIGH                     0x84
#define SOFT_RESET_GO2_SOFT_RESET_N                 0xBF
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_0            0xB0
#define GLOBAL_CONFIG_REF_EN_START_SELECT           0xB6
#define IDENTIFICATION_MODEL_ID                     0xC0
#define OSC_CALIBRATE_VAL                           0xF8
/* Page 1 (0xFF <- 0x01 first). */
#define ALGO_PHASECAL_LIM                           0x30
#define RESULT_PEAK_SIGNAL_RATE_REF                 0xB6

#define MODEL_ID_VL53L0X 0xEE

#define DEVICEMODE_SINGLE_RANGING     0
#define DEVICEMODE_CONTINUOUS_RANGING 1
#define DEVICEMODE_CONTINUOUS_TIMED   3

#define GPIOFUNCTIONALITY_NEW_MEASURE_READY 4

enum { SEQ_TCC, SEQ_DSS, SEQ_MSRC, SEQ_PRE_RANGE, SEQ_FINAL_RANGE, SEQ_COUNT };
static const uint8_t SEQ_SET[SEQ_COUNT]   = {0x10, 0x28, 0x04, 0x40, 0x80};
static const uint8_t SEQ_CLEAR[SEQ_COUNT] = {0xEF, 0xD7, 0xFB, 0xBF, 0x7F};
static const uint8_t SEQ_TEST[SEQ_COUNT]  = {0x10, 0x08, 0x04, 0x40, 0x80};

enum { VCSEL_PRE_RANGE, VCSEL_FINAL_RANGE };

enum {
    CHECK_SIGMA_FINAL_RANGE, CHECK_SIGNAL_RATE_FINAL_RANGE, CHECK_SIGNAL_REF_CLIP,
    CHECK_RANGE_IGNORE_THRESHOLD, CHECK_SIGNAL_RATE_MSRC, CHECK_SIGNAL_RATE_PRE_RANGE,
    CHECK_COUNT
};

#define DEFAULT_MAX_LOOP      2000
#define TARGET_REF_RATE       0x0A00 /* 20 Mcps in 9.7, the ref-SPAD target */
#define SPEED_OF_LIGHT_IN_AIR 2997
#define REF_SPAD_BUFFER_SIZE  6
#define RESULT_BLOCK_LEN      12

/* The ranging modes of the four ST examples: signal Mcps, sigma mm, budget us,
 * pre-range PCLK, final PCLK. 'default' is what StaticInit leaves. */
typedef struct { const char *name; double signal_mcps; int sigma_mm; int64_t budget_us; int pre, fin; } l0x_mode;
static const l0x_mode MODES[4] = {
    {"default",       0.25, 18,  33000, 14, 10},
    {"long-range",    0.10, 60,  33000, 18, 14},
    {"high-speed",    0.25, 32,  30000, 14, 10},
    {"high-accuracy", 0.25, 18, 200000, 14, 10},
};

/* DefaultTuningSettings[] (vl53l0x_tuning.h, v36): {count, address, bytes}
 * records ended by a zero count. */
static const uint8_t DEFAULT_TUNING_SETTINGS[] = {
    0x01, 0xFF, 0x01, 0x01, 0x00, 0x00,
    0x01, 0xFF, 0x00, 0x01, 0x09, 0x00, 0x01, 0x10, 0x00, 0x01, 0x11, 0x00,
    0x01, 0x24, 0x01, 0x01, 0x25, 0xff, 0x01, 0x75, 0x00,
    0x01, 0xFF, 0x01, 0x01, 0x4e, 0x2c, 0x01, 0x48, 0x00, 0x01, 0x30, 0x20,
    0x01, 0xFF, 0x00, 0x01, 0x30, 0x09, 0x01, 0x54, 0x00, 0x01, 0x31, 0x04,
    0x01, 0x32, 0x03, 0x01, 0x40, 0x83, 0x01, 0x46, 0x25, 0x01, 0x60, 0x00,
    0x01, 0x27, 0x00, 0x01, 0x50, 0x06, 0x01, 0x51, 0x00, 0x01, 0x52, 0x96,
    0x01, 0x56, 0x08, 0x01, 0x57, 0x30, 0x01, 0x61, 0x00, 0x01, 0x62, 0x00,
    0x01, 0x64, 0x00, 0x01, 0x65, 0x00, 0x01, 0x66, 0xa0,
    0x01, 0xFF, 0x01, 0x01, 0x22, 0x32, 0x01, 0x47, 0x14, 0x01, 0x49, 0xff, 0x01, 0x4a, 0x00,
    0x01, 0xFF, 0x00, 0x01, 0x7a, 0x0a, 0x01, 0x7b, 0x00, 0x01, 0x78, 0x21,
    0x01, 0xFF, 0x01, 0x01, 0x23, 0x34, 0x01, 0x42, 0x00, 0x01, 0x44, 0xff,
    0x01, 0x45, 0x26, 0x01, 0x46, 0x05, 0x01, 0x40, 0x40, 0x01, 0x0E, 0x06,
    0x01, 0x20, 0x1a, 0x01, 0x43, 0x40,
    0x01, 0xFF, 0x00, 0x01, 0x34, 0x03, 0x01, 0x35, 0x44,
    0x01, 0xFF, 0x01, 0x01, 0x31, 0x04, 0x01, 0x4b, 0x09, 0x01, 0x4c, 0x05, 0x01, 0x4d, 0x04,
    0x01, 0xFF, 0x00, 0x01, 0x44, 0x00, 0x01, 0x45, 0x20, 0x01, 0x47, 0x08,
    0x01, 0x48, 0x28, 0x01, 0x67, 0x00, 0x01, 0x70, 0x04, 0x01, 0x71, 0x01,
    0x01, 0x72, 0xfe, 0x01, 0x76, 0x00, 0x01, 0x77, 0x00,
    0x01, 0xFF, 0x01, 0x01, 0x0d, 0x01,
    0x01, 0xFF, 0x00, 0x01, 0x80, 0x01, 0x01, 0x01, 0xF8,
    0x01, 0xFF, 0x01, 0x01, 0x8e, 0x01, 0x01, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x01, 0x80, 0x00,
    0x00, 0x00, 0x00,
};

/* Dmax lookup set up by DataInit, FixPoint16.16. */
static const int64_t DMAX_LUT_AMB[7] = {0x00000000, 0x0000B333, 0x00020000, 0x0003CCCC,
                                        0x00074CCC, 0x000A0000, 0x000F0000};
static const int64_t DMAX_LUT_MM[7]  = {0x04B00000, 0x044C0000, 0x03840000, 0x02EE0000,
                                        0x02260000, 0x01F40000, 0x01900000};

/* Reference SPAD array geometry: quadrant of index >> 6. */
static const int REF_ARRAY_QUADRANTS[4] = {10, 5, 0, 5};

/* PALDevData / DeviceSpecificParameters: the state the C driver keeps beside
 * the sensor. It survives re-inits exactly as the Python dict does. */
typedef struct {
    vlx_driver base;
    int      read_done;
    uint8_t  stop_variable;
    uint8_t  sequence_config;
    int      range_fractional;
    int      device_mode;
    /* Not in the C driver: the continuous mode last chosen (0 = none yet), so
     * a single shot in between (the calibrations run on one) does not leave
     * start_ranging() stranded. */
    int      continuous_mode;
    int64_t  budget_us;
    int      pin0_functionality;
    int      ref_spad_count, ref_spad_type;
    uint8_t  good_spad_map[REF_SPAD_BUFFER_SIZE];
    uint8_t  ref_spad_enables[REF_SPAD_BUFFER_SIZE];
    int      pre_pclks, final_pclks;
    int64_t  pre_timeout_us, final_timeout_us;
    int      target_ref_rate;
    int      limit_enable[CHECK_COUNT];
    int64_t  limit_value[CHECK_COUNT];
    int64_t  offset_nvm_um;
    int64_t  signal_rate_400mm;
    uint8_t  module_id, revision;
    char     product_id[20];
    uint32_t uid_upper, uid_lower;
    int64_t  osc_frequency;
    int      xtalk_enable;
    int64_t  xtalk_rate_mcps;
    int64_t  linearity_gain;
    bool     ref_spads_from_nvm;
    const char *mode;
} l0x_drv;

static l0x_drv *ld(vlx_driver *d) { return (l0x_drv *)d; }
#define P (d->p)

/* A Vl53Error: the sensor did not do what the driver needed. */
#define VL53_ERR(...) depz_fail(DEPZ_E_PROTOCOL, "vl53l0x: " __VA_ARGS__)

/* Python's ProtocolError also covers a reply that did not parse. */
static bool protocol_error(int rc) { return vlx_is_protocol_error(rc) || rc == DEPZ_E_PROTOCOL; }

/* ── integer helpers ───────────────────────────────────────────────────── */

static int64_t fdiv(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

static int64_t u32(int64_t v) { return (int64_t)((uint64_t)v & 0xFFFFFFFFu); }

/* VL53L0X_isqrt(): the bit-by-bit root, bit starting at 1 << 30. */
static int64_t isqrt(int64_t num)
{
    int64_t res = 0, bit = (int64_t)1 << 30;
    while (bit > num) bit >>= 2;
    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

static int decode_vcsel_period(int reg) { return (reg + 1) << 1; }
static int encode_vcsel_period(int pclks) { return (pclks >> 1) - 1; }

static uint16_t encode_timeout(int64_t mclks)
{
    int64_t ls_byte, ms_byte = 0;
    if (mclks <= 0) return 0;
    ls_byte = mclks - 1;
    while (ls_byte & 0xFFFFFF00) {
        ls_byte >>= 1;
        ms_byte++;
    }
    return (uint16_t)(((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF);
}

static int64_t decode_timeout(int64_t encoded)
{
    int shift = (int)((encoded & 0xFF00) >> 8);
    /* Unbounded in Python; no real register gets near 64 bits. */
    if (shift > 55) shift = 55;
    return ((encoded & 0x00FF) << shift) + 1;
}

static int64_t calc_macro_period_ps(int64_t pclks) { return 2304 * pclks * 1655; }

static int64_t calc_timeout_mclks(int64_t timeout_us, int64_t pclks)
{
    int64_t macro_ns = fdiv(calc_macro_period_ps(pclks) + 500, 1000);
    return fdiv(timeout_us * 1000 + fdiv(macro_ns, 2), macro_ns);
}

static int64_t calc_timeout_us(int64_t mclks, int64_t pclks)
{
    int64_t macro_ns = fdiv(calc_macro_period_ps(pclks) + 500, 1000);
    return fdiv(mclks * macro_ns + 500, 1000);
}

static int is_aperture(int index, bool *out)
{
    if (index < 0 || (index >> 6) >= 4) return VL53_ERR("reference SPAD index %d out of range", index);
    *out = REF_ARRAY_QUADRANTS[index >> 6] != 0;
    return DEPZ_OK;
}

/* The next enabled bit at or after `curr`, or -1. */
static int get_next_good_spad(const uint8_t *good, int size, int curr)
{
    int start = curr / 8, fine_offset = curr % 8, coarse;
    for (coarse = start; coarse < size; coarse++) {
        int data = good[coarse], fine = 0;
        if (coarse == start) {
            data >>= fine_offset;
            fine = fine_offset;
        }
        while (fine < 8) {
            if (data & 1) return coarse * 8 + fine;
            data >>= 1;
            fine++;
        }
    }
    return -1;
}

/* VL53L0X_GetRangeStatusString(). */
static const char *status_text(int status)
{
    switch (status) {
    case 0:   return "Range Valid";
    case 1:   return "Sigma Fail";
    case 2:   return "Signal Fail";
    case 3:   return "Min Range Fail";
    case 4:   return "Phase Fail";
    case 5:   return "Hardware Fail";
    case 255: return "No Update";
    default:  return "unknown";
    }
}

/* VL53L0X_RangingMeasurementData_t, the fields the port fills. */
typedef struct {
    int     range_status;
    int64_t distance_mm, signal_rate_mcps, ambient_rate_mcps, effective_spads;
    int64_t dmax_mm, sigma_mm;
    int     device_range_status;
} ranging_data;

/* ── limit checks ──────────────────────────────────────────────────────── */

static int set_limit_check_enable(vlx_driver *d, int check, int enable)
{
    l0x_drv *s = ld(d);
    int64_t value;
    int disable;
    uint8_t cur;
    if (check >= CHECK_COUNT) return VL53_ERR("no limit check %d", check);
    if (enable == 0) {
        value = 0;
        disable = 1;
    } else {
        value = s->limit_value[check];
        disable = 0;
    }
    if (check == CHECK_SIGNAL_RATE_FINAL_RANGE) {
        VLX_TRY(vlx_wr_word(P, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)((value >> 9) & 0xFFFF)));
    } else if (check == CHECK_SIGNAL_RATE_MSRC) {
        VLX_TRY(vlx_rd_byte(P, MSRC_CONFIG_CONTROL, &cur));
        VLX_TRY(vlx_wr_byte(P, MSRC_CONFIG_CONTROL, (uint8_t)((cur & 0xFE) | (disable << 1))));
    } else if (check == CHECK_SIGNAL_RATE_PRE_RANGE) {
        VLX_TRY(vlx_rd_byte(P, MSRC_CONFIG_CONTROL, &cur));
        VLX_TRY(vlx_wr_byte(P, MSRC_CONFIG_CONTROL, (uint8_t)((cur & 0xEF) | (disable << 4))));
    }
    s->limit_enable[check] = enable ? 1 : 0;
    return DEPZ_OK;
}

static int set_limit_check_value(vlx_driver *d, int check, int64_t value)
{
    l0x_drv *s = ld(d);
    if (!s->limit_enable[check]) {
        s->limit_value[check] = value; /* disabled: kept host-side only */
        return DEPZ_OK;
    }
    if (check == CHECK_SIGNAL_RATE_FINAL_RANGE)
        VLX_TRY(vlx_wr_word(P, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)((value >> 9) & 0xFFFF)));
    else if (check == CHECK_SIGNAL_RATE_MSRC || check == CHECK_SIGNAL_RATE_PRE_RANGE)
        VLX_TRY(vlx_wr_word(P, PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)((value >> 9) & 0xFFFF)));
    s->limit_value[check] = value;
    return DEPZ_OK;
}

/* ── boot and reset ────────────────────────────────────────────────────── */

static int model_id(vlx_driver *d, uint16_t *out)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, IDENTIFICATION_MODEL_ID, &v));
    *out = v;
    return DEPZ_OK;
}

/* No firmware-status register: the model id answering says the die is up. */
static int wait_boot(vlx_driver *d, int timeout_ms)
{
    uint64_t deadline = vlx_deadline_ms(timeout_ms);
    for (;;) {
        uint8_t v;
        int rc = vlx_rd_byte(P, IDENTIFICATION_MODEL_ID, &v);
        if (rc == DEPZ_OK && v == MODEL_ID_VL53L0X) return DEPZ_OK;
        if (rc && !protocol_error(rc)) return rc; /* NACK while booting is fine */
        if (vlx_past(deadline)) return depz_fail(DEPZ_E_TIMEOUT, "vl53l0x: timeout waiting for MODEL_ID 0xEE at 0xC0");
        vlx_sleep_ms(P, 1);
    }
}

/* Poll the model id until it reads zero / non-zero; a NACK (the die reboots)
 * is "not yet". No sleep in the loop, as the Python. */
static int poll_model_id(vlx_driver *d, bool want_zero, uint64_t deadline)
{
    for (;;) {
        uint8_t v;
        int rc = vlx_rd_byte(P, IDENTIFICATION_MODEL_ID, &v);
        if (rc && !protocol_error(rc)) return rc;
        if (rc == DEPZ_OK && (v == 0x00) == want_zero) return DEPZ_OK;
        if (vlx_past(deadline)) return depz_fail(DEPZ_E_TIMEOUT, "vl53l0x: timeout in the soft reset of the sensor");
    }
}

/* VL53L0X_ResetDevice(): DataInit and StaticInit configure a fresh device. */
static int reset_device(vlx_driver *d, int timeout_ms)
{
    uint64_t deadline = vlx_deadline_ms(timeout_ms);
    VLX_TRY(vlx_wr_byte(P, SOFT_RESET_GO2_SOFT_RESET_N, 0x00));
    VLX_TRY(poll_model_id(d, true, deadline));
    vlx_sleep_ms(P, 1);
    VLX_TRY(vlx_wr_byte(P, SOFT_RESET_GO2_SOFT_RESET_N, 0x01));
    VLX_TRY(poll_model_id(d, false, deadline));
    vlx_sleep_ms(P, 1);
    return DEPZ_OK;
}

/* ── interrupt ─────────────────────────────────────────────────────────── */

/* VL53L0X_ClearInterruptMask(): two writes, then confirm — three rounds. */
static int clear_interrupt(vlx_driver *d)
{
    int i;
    for (i = 0; i < 3; i++) {
        uint8_t v;
        VLX_TRY(vlx_wr_byte(P, SYSTEM_INTERRUPT_CLEAR, 0x01));
        VLX_TRY(vlx_wr_byte(P, SYSTEM_INTERRUPT_CLEAR, 0x00));
        VLX_TRY(vlx_rd_byte(P, RESULT_INTERRUPT_STATUS, &v));
        if ((v & 0x07) == 0x00) return DEPZ_OK;
    }
    return VL53_ERR("interrupt not cleared after three rounds");
}

static int check_for_data_ready(vlx_driver *d, bool *ready)
{
    uint8_t v;
    if (ld(d)->pin0_functionality == GPIOFUNCTIONALITY_NEW_MEASURE_READY) {
        VLX_TRY(vlx_rd_byte(P, RESULT_INTERRUPT_STATUS, &v));
        if (v & 0x18) return VL53_ERR("range error reported in RESULT_INTERRUPT_STATUS 0x%02X", v);
        *ready = (v & 0x07) == GPIOFUNCTIONALITY_NEW_MEASURE_READY;
        return DEPZ_OK;
    }
    VLX_TRY(vlx_rd_byte(P, RESULT_RANGE_STATUS, &v));
    *ready = (v & 0x01) != 0;
    return DEPZ_OK;
}

static int set_gpio_config(vlx_driver *d, int functionality, bool polarity_low)
{
    uint8_t cur;
    VLX_TRY(vlx_wr_byte(P, SYSTEM_INTERRUPT_CONFIG_GPIO, (uint8_t)functionality));
    VLX_TRY(vlx_rd_byte(P, GPIO_HV_MUX_ACTIVE_HIGH, &cur));
    VLX_TRY(vlx_wr_byte(P, GPIO_HV_MUX_ACTIVE_HIGH, (uint8_t)((cur & 0xEF) | (polarity_low ? 0x00 : 0x10))));
    ld(d)->pin0_functionality = functionality;
    return clear_interrupt(d);
}

/* ── sequence steps and timeouts ───────────────────────────────────────── */

static int get_sequence_step_enables(vlx_driver *d, bool steps[SEQ_COUNT])
{
    uint8_t config;
    int i;
    VLX_TRY(vlx_rd_byte(P, SYSTEM_SEQUENCE_CONFIG, &config));
    for (i = 0; i < SEQ_COUNT; i++) steps[i] = (config & SEQ_TEST[i]) != 0;
    return DEPZ_OK;
}

static int get_vcsel_pulse_period(vlx_driver *d, int type, int *pclks)
{
    uint8_t v;
    VLX_TRY(vlx_rd_byte(P, type == VCSEL_PRE_RANGE ? PRE_RANGE_CONFIG_VCSEL_PERIOD : FINAL_RANGE_CONFIG_VCSEL_PERIOD, &v));
    *pclks = decode_vcsel_period(v);
    return DEPZ_OK;
}

static int get_sequence_step_timeout(vlx_driver *d, int step, int64_t *us)
{
    int vcsel;
    uint16_t w;
    if (step == SEQ_TCC || step == SEQ_DSS || step == SEQ_MSRC) {
        uint8_t b;
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_PRE_RANGE, &vcsel));
        VLX_TRY(vlx_rd_byte(P, MSRC_CONFIG_TIMEOUT_MACROP, &b));
        *us = calc_timeout_us(decode_timeout(b), vcsel);
        return DEPZ_OK;
    }
    if (step == SEQ_PRE_RANGE) {
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_PRE_RANGE, &vcsel));
        VLX_TRY(vlx_rd_word(P, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, &w));
        *us = calc_timeout_us(decode_timeout(w), vcsel);
        return DEPZ_OK;
    }
    if (step == SEQ_FINAL_RANGE) {
        bool steps[SEQ_COUNT];
        int64_t pre_mclks = 0, final_mclks;
        VLX_TRY(get_sequence_step_enables(d, steps));
        if (steps[SEQ_PRE_RANGE]) {
            VLX_TRY(vlx_rd_word(P, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, &w));
            pre_mclks = decode_timeout(w);
        }
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_FINAL_RANGE, &vcsel));
        VLX_TRY(vlx_rd_word(P, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, &w));
        final_mclks = decode_timeout(w);
        *us = calc_timeout_us((final_mclks - pre_mclks) & 0xFFFF, vcsel);
        return DEPZ_OK;
    }
    return VL53_ERR("no timeout for sequence step %d", step);
}

static int set_sequence_step_timeout(vlx_driver *d, int step, int64_t timeout_us)
{
    l0x_drv *s = ld(d);
    int vcsel;
    int64_t mclks;
    if (step == SEQ_TCC || step == SEQ_DSS || step == SEQ_MSRC) {
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_PRE_RANGE, &vcsel));
        mclks = calc_timeout_mclks(timeout_us, vcsel);
        return vlx_wr_byte(P, MSRC_CONFIG_TIMEOUT_MACROP, (uint8_t)(mclks > 256 ? 255 : ((mclks - 1) & 0xFF)));
    }
    if (step == SEQ_PRE_RANGE) {
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_PRE_RANGE, &vcsel));
        mclks = calc_timeout_mclks(timeout_us, vcsel);
        VLX_TRY(vlx_wr_word(P, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, encode_timeout(mclks)));
        s->pre_timeout_us = timeout_us;
        return DEPZ_OK;
    }
    if (step == SEQ_FINAL_RANGE) {
        /* The register carries pre-range + final range, in macro periods. */
        bool steps[SEQ_COUNT];
        int64_t pre_mclks = 0;
        uint16_t w;
        VLX_TRY(get_sequence_step_enables(d, steps));
        if (steps[SEQ_PRE_RANGE]) {
            VLX_TRY(vlx_rd_word(P, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, &w));
            pre_mclks = decode_timeout(w);
        }
        VLX_TRY(get_vcsel_pulse_period(d, VCSEL_FINAL_RANGE, &vcsel));
        mclks = calc_timeout_mclks(timeout_us, vcsel) + pre_mclks;
        VLX_TRY(vlx_wr_word(P, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encode_timeout(mclks)));
        s->final_timeout_us = timeout_us;
        return DEPZ_OK;
    }
    return VL53_ERR("no timeout for sequence step %d", step);
}

/* Fixed scheduler overheads, us (set_measurement_timing_budget_micro_seconds). */
#define START_OVERHEAD_US       1910
#define END_OVERHEAD_US         960
#define MSRC_OVERHEAD_US        660
#define TCC_OVERHEAD_US         590
#define DSS_OVERHEAD_US         690
#define PRE_RANGE_OVERHEAD_US   660
#define FINAL_RANGE_OVERHEAD_US 550

static int take(int64_t *final_budget_us, int64_t sub, int64_t budget_us)
{
    if (sub >= *final_budget_us)
        return depz_fail(DEPZ_E_ARG, "vl53l0x: timing budget %lld us is too small for the enabled sequence steps",
                         (long long)budget_us);
    *final_budget_us -= sub;
    return DEPZ_OK;
}

static int set_measurement_timing_budget(vlx_driver *d, int64_t budget_us)
{
    bool steps[SEQ_COUNT];
    int64_t final_budget_us = budget_us - (START_OVERHEAD_US + END_OVERHEAD_US), us;
    VLX_TRY(get_sequence_step_enables(d, steps));
    if (steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS]) {
        /* TCC, MSRC and DSS share one timeout. */
        VLX_TRY(get_sequence_step_timeout(d, SEQ_MSRC, &us));
        if (steps[SEQ_TCC]) VLX_TRY(take(&final_budget_us, us + TCC_OVERHEAD_US, budget_us));
        if (steps[SEQ_DSS]) VLX_TRY(take(&final_budget_us, 2 * (us + DSS_OVERHEAD_US), budget_us));
        else if (steps[SEQ_MSRC]) VLX_TRY(take(&final_budget_us, us + MSRC_OVERHEAD_US, budget_us));
    }
    if (steps[SEQ_PRE_RANGE]) {
        VLX_TRY(get_sequence_step_timeout(d, SEQ_PRE_RANGE, &us));
        VLX_TRY(take(&final_budget_us, us + PRE_RANGE_OVERHEAD_US, budget_us));
    }
    if (steps[SEQ_FINAL_RANGE]) {
        /* Whatever is left goes to the final range. */
        final_budget_us -= FINAL_RANGE_OVERHEAD_US;
        VLX_TRY(set_sequence_step_timeout(d, SEQ_FINAL_RANGE, final_budget_us));
        ld(d)->budget_us = budget_us;
    }
    return DEPZ_OK;
}

static int get_measurement_timing_budget(vlx_driver *d, int64_t *out)
{
    bool steps[SEQ_COUNT];
    int64_t budget_us = START_OVERHEAD_US + END_OVERHEAD_US, us;
    VLX_TRY(get_sequence_step_enables(d, steps));
    if (steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS]) {
        VLX_TRY(get_sequence_step_timeout(d, SEQ_MSRC, &us));
        if (steps[SEQ_TCC]) budget_us += us + TCC_OVERHEAD_US;
        if (steps[SEQ_DSS]) budget_us += 2 * (us + DSS_OVERHEAD_US);
        else if (steps[SEQ_MSRC]) budget_us += us + MSRC_OVERHEAD_US;
    }
    if (steps[SEQ_PRE_RANGE]) {
        VLX_TRY(get_sequence_step_timeout(d, SEQ_PRE_RANGE, &us));
        budget_us += us + PRE_RANGE_OVERHEAD_US;
    }
    if (steps[SEQ_FINAL_RANGE]) {
        VLX_TRY(get_sequence_step_timeout(d, SEQ_FINAL_RANGE, &us));
        budget_us += us + FINAL_RANGE_OVERHEAD_US;
    }
    ld(d)->budget_us = budget_us;
    *out = budget_us;
    return DEPZ_OK;
}

static int set_sequence_step_enable(vlx_driver *d, int step, int enabled)
{
    uint8_t config, next;
    VLX_TRY(vlx_rd_byte(P, SYSTEM_SEQUENCE_CONFIG, &config));
    next = enabled ? (uint8_t)(config | SEQ_SET[step]) : (uint8_t)(config & SEQ_CLEAR[step]);
    if (next == config) return DEPZ_OK;
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, next));
    ld(d)->sequence_config = next;
    /* The budget is spread over the enabled steps: re-apply it. */
    return set_measurement_timing_budget(d, ld(d)->budget_us);
}

/* ── NVM ───────────────────────────────────────────────────────────────── */

static int device_read_strobe(vlx_driver *d)
{
    int i;
    VLX_TRY(vlx_wr_byte(P, 0x83, 0x00));
    for (i = 0; i < DEFAULT_MAX_LOOP; i++) {
        uint8_t v;
        VLX_TRY(vlx_rd_byte(P, 0x83, &v));
        if (v != 0x00) break;
    }
    if (i == DEFAULT_MAX_LOOP) return VL53_ERR("timeout waiting for the NVM read strobe");
    return vlx_wr_byte(P, 0x83, 0x01);
}

static int nvm_dword(vlx_driver *d, uint8_t index, uint32_t *out)
{
    VLX_TRY(vlx_wr_byte(P, 0x94, index));
    VLX_TRY(device_read_strobe(d));
    return vlx_rd_dword(P, 0x90, out);
}

/* VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the
 * product identification (2), the part UID and factory offset (4). */
static int get_info_from_device(vlx_driver *d, int option)
{
    l0x_drv *s = ld(d);
    int done = s->read_done;
    uint8_t good[REF_SPAD_BUFFER_SIZE], b, module_id = 0, revision = 0;
    char product_id[20] = {0};
    uint32_t tmp, t2, uid_upper = 0, uid_lower = 0;
    int ref_count = 0, ref_type = 0, nchars = 0;
    int64_t sig_400 = 0, dist_400 = 0;
    if (done == 7) return DEPZ_OK;
    memcpy(good, s->good_spad_map, sizeof good);

    VLX_TRY(vlx_wr_byte(P, 0x80, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x06));
    VLX_TRY(vlx_rd_byte(P, 0x83, &b));
    VLX_TRY(vlx_wr_byte(P, 0x83, (uint8_t)(b | 4)));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x07));
    VLX_TRY(vlx_wr_byte(P, 0x81, 0x01));
    vlx_sleep_ms(P, 1); /* VL53L0X_PollingDelay() */
    VLX_TRY(vlx_wr_byte(P, 0x80, 0x01));

    if ((option & 1) == 1 && (done & 1) == 0) {
        VLX_TRY(nvm_dword(d, 0x6B, &tmp));
        ref_count = (int)((tmp >> 8) & 0x7F);
        ref_type = (int)((tmp >> 15) & 0x01);
        VLX_TRY(nvm_dword(d, 0x24, &tmp));
        good[0] = (uint8_t)(tmp >> 24);
        good[1] = (uint8_t)(tmp >> 16);
        good[2] = (uint8_t)(tmp >> 8);
        good[3] = (uint8_t)tmp;
        VLX_TRY(nvm_dword(d, 0x25, &tmp));
        good[4] = (uint8_t)(tmp >> 24);
        good[5] = (uint8_t)(tmp >> 16);
    }
    if ((option & 2) == 2 && (done & 2) == 0) {
        int byte;
        VLX_TRY(vlx_wr_byte(P, 0x94, 0x02));
        VLX_TRY(device_read_strobe(d));
        VLX_TRY(vlx_rd_byte(P, 0x90, &module_id));
        VLX_TRY(vlx_wr_byte(P, 0x94, 0x7B));
        VLX_TRY(device_read_strobe(d));
        VLX_TRY(vlx_rd_byte(P, 0x90, &revision));
#define CH(v) (product_id[nchars++] = (char)((v) & 0x7F))
        VLX_TRY(nvm_dword(d, 0x77, &tmp));
        CH(tmp >> 25); CH(tmp >> 18); CH(tmp >> 11); CH(tmp >> 4);
        byte = (int)((tmp & 0x00F) << 3);
        VLX_TRY(nvm_dword(d, 0x78, &tmp));
        CH(byte + ((tmp >> 29) & 0x7F)); CH(tmp >> 22); CH(tmp >> 15); CH(tmp >> 8); CH(tmp >> 1);
        byte = (int)((tmp & 0x001) << 6);
        VLX_TRY(nvm_dword(d, 0x79, &tmp));
        CH(byte + ((tmp >> 26) & 0x7F)); CH(tmp >> 19); CH(tmp >> 12); CH(tmp >> 5);
        byte = (int)((tmp & 0x01F) << 2);
        VLX_TRY(nvm_dword(d, 0x7A, &tmp));
        CH(byte + ((tmp >> 30) & 0x7F)); CH(tmp >> 23); CH(tmp >> 16); CH(tmp >> 9); CH(tmp >> 2);
#undef CH
    }
    if ((option & 4) == 4 && (done & 4) == 0) {
        VLX_TRY(nvm_dword(d, 0x7B, &uid_upper));
        VLX_TRY(nvm_dword(d, 0x7C, &uid_lower));
        VLX_TRY(nvm_dword(d, 0x73, &tmp));
        VLX_TRY(nvm_dword(d, 0x74, &t2));
        sig_400 = ((int64_t)(tmp & 0xFF) << 8) | ((t2 & 0xFF000000u) >> 24);
        VLX_TRY(nvm_dword(d, 0x75, &tmp));
        VLX_TRY(nvm_dword(d, 0x76, &t2));
        dist_400 = ((int64_t)(tmp & 0xFF) << 8) | ((t2 & 0xFF000000u) >> 24);
    }

    VLX_TRY(vlx_wr_byte(P, 0x81, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x06));
    VLX_TRY(vlx_rd_byte(P, 0x83, &b));
    VLX_TRY(vlx_wr_byte(P, 0x83, (uint8_t)(b & 0xFB)));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0x80, 0x00));

    if ((option & 1) == 1 && (done & 1) == 0) {
        s->ref_spad_count = ref_count;
        s->ref_spad_type = ref_type;
        memcpy(s->good_spad_map, good, sizeof good);
    }
    if ((option & 2) == 2 && (done & 2) == 0) {
        s->module_id = module_id;
        s->revision = revision;
        memcpy(s->product_id, product_id, sizeof product_id);
    }
    if ((option & 4) == 4 && (done & 4) == 0) {
        s->uid_upper = uid_upper;
        s->uid_lower = uid_lower;
        s->signal_rate_400mm = sig_400 << 9;
        s->offset_nvm_um = 0;
        if (dist_400 != 0) {
            int64_t offset_1104_mm = u32(dist_400 - (400 << 4));
            s->offset_nvm_um = -((offset_1104_mm * 1000) >> 4);
        }
    }
    s->read_done = done | option;
    return DEPZ_OK;
}

/* ── reference SPADs ───────────────────────────────────────────────────── */

/* Append `count` good SPADs of the requested kind from index `curr` to
 * `spad_array`; *next = the index after the last one. */
static int add_good_spads(l0x_drv *s, uint8_t *spad_array, int start, int curr, int count, int aperture, int *next)
{
    int i;
    for (i = 0; i < count; i++) {
        bool ap;
        int next_good = get_next_good_spad(s->good_spad_map, REF_SPAD_BUFFER_SIZE, curr);
        if (next_good == -1) return VL53_ERR("ran out of good reference SPADs");
        VLX_TRY(is_aperture(start + next_good, &ap));
        if (ap != (aperture != 0)) return VL53_ERR("the good SPAD map leaves the requested quadrant");
        curr = next_good;
        spad_array[curr / 8] |= (uint8_t)(1 << (curr % 8));
        curr++;
    }
    *next = curr;
    return DEPZ_OK;
}

/* Skip to the first aperture SPAD (at most 44 in). */
static int skip_to_aperture(int start, int *curr)
{
    for (;;) {
        bool ap;
        VLX_TRY(is_aperture(start + *curr, &ap));
        if (ap || *curr >= 44) return DEPZ_OK;
        (*curr)++;
    }
}

static int write_check_spad_map(vlx_driver *d, const uint8_t *spad_array)
{
    uint8_t check[REF_SPAD_BUFFER_SIZE];
    VLX_TRY(vlx_wr_multi(P, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_array, REF_SPAD_BUFFER_SIZE));
    VLX_TRY(vlx_rd_multi(P, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, check, REF_SPAD_BUFFER_SIZE));
    if (memcmp(check, spad_array, REF_SPAD_BUFFER_SIZE)) return VL53_ERR("reference SPAD map did not read back");
    return DEPZ_OK;
}

static int spad_prologue(vlx_driver *d, int start_select)
{
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00));
    VLX_TRY(vlx_wr_byte(P, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    return vlx_wr_byte(P, GLOBAL_CONFIG_REF_EN_START_SELECT, (uint8_t)start_select);
}

/* VL53L0X_set_reference_spads(): apply the NVM record and verify it. */
static int set_reference_spads(vlx_driver *d, int count, int aperture)
{
    l0x_drv *s = ld(d);
    const int start = 0xB4;
    uint8_t spad_array[REF_SPAD_BUFFER_SIZE] = {0};
    int curr = 0;
    VLX_TRY(spad_prologue(d, start));
    if (aperture) VLX_TRY(skip_to_aperture(start, &curr));
    VLX_TRY(add_good_spads(s, spad_array, start, curr, count, aperture, &curr));
    VLX_TRY(write_check_spad_map(d, spad_array));
    memcpy(s->ref_spad_enables, spad_array, sizeof spad_array);
    s->ref_spad_count = count;
    s->ref_spad_type = aperture;
    return DEPZ_OK;
}

/* ── tuning settings ───────────────────────────────────────────────────── */

static int load_tuning_settings(vlx_driver *d, const uint8_t *buf)
{
    size_t index = 0;
    while (buf[index] != 0) {
        int n = buf[index++];
        if (n == 0xFF) {
            index += 3; /* host-side sigma parameters, unused by 1.0.4 */
        } else if (n <= 4) {
            uint8_t address = buf[index++];
            VLX_TRY(vlx_wr_multi(P, address, buf + index, (size_t)n));
            index += (size_t)n;
        } else {
            return VL53_ERR("bad tuning record at offset %zu", index);
        }
    }
    return DEPZ_OK;
}

/* ── init ──────────────────────────────────────────────────────────────── */

static int data_init(vlx_driver *d)
{
    l0x_drv *s = ld(d);
    int check;
    VLX_TRY(vlx_wr_byte(P, 0x88, 0x00)); /* I2C standard mode */
    s->read_done = 0;
    s->linearity_gain = 1000;
    s->osc_frequency = 618660;
    s->xtalk_rate_mcps = 0;
    s->xtalk_enable = 0;
    s->device_mode = DEVICEMODE_SINGLE_RANGING;

    VLX_TRY(vlx_wr_byte(P, 0x80, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x00));
    VLX_TRY(vlx_rd_byte(P, 0x91, &s->stop_variable));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0x80, 0x00));

    for (check = 0; check < CHECK_COUNT; check++) VLX_TRY(set_limit_check_enable(d, check, 1));
    VLX_TRY(set_limit_check_enable(d, CHECK_SIGNAL_REF_CLIP, 0));
    VLX_TRY(set_limit_check_enable(d, CHECK_RANGE_IGNORE_THRESHOLD, 0));
    VLX_TRY(set_limit_check_enable(d, CHECK_SIGNAL_RATE_MSRC, 0));
    VLX_TRY(set_limit_check_enable(d, CHECK_SIGNAL_RATE_PRE_RANGE, 0));

    VLX_TRY(set_limit_check_value(d, CHECK_SIGMA_FINAL_RANGE, 18 * 65536));
    VLX_TRY(set_limit_check_value(d, CHECK_SIGNAL_RATE_FINAL_RANGE, 25 * 65536 / 100));
    VLX_TRY(set_limit_check_value(d, CHECK_SIGNAL_REF_CLIP, 35 * 65536));
    VLX_TRY(set_limit_check_value(d, CHECK_RANGE_IGNORE_THRESHOLD, 0));

    s->sequence_config = 0xFF;
    return vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, 0xFF);
}

static int static_init(vlx_driver *d)
{
    l0x_drv *s = ld(d);
    uint16_t tempword;
    uint8_t v;
    int64_t budget;
    int count, aperture;
    VLX_TRY(get_info_from_device(d, 1));
    count = s->ref_spad_count;
    aperture = s->ref_spad_type;
    /* An out-of-range record: never factory-programmed; measured instead. */
    s->ref_spads_from_nvm = !(aperture > 1 || (aperture == 1 && count > 32) || (aperture == 0 && count > 12));
    if (s->ref_spads_from_nvm) VLX_TRY(set_reference_spads(d, count, aperture));
    VLX_TRY(load_tuning_settings(d, DEFAULT_TUNING_SETTINGS));
    VLX_TRY(set_gpio_config(d, GPIOFUNCTIONALITY_NEW_MEASURE_READY, true));

    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_rd_word(P, 0x84, &tempword));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    s->osc_frequency = (int64_t)tempword << 4; /* FixPoint4.12 -> 16.16 */

    VLX_TRY(get_measurement_timing_budget(d, &budget));
    VLX_TRY(vlx_rd_byte(P, SYSTEM_RANGE_CONFIG, &v));
    s->range_fractional = v & 1;
    VLX_TRY(vlx_rd_byte(P, SYSTEM_SEQUENCE_CONFIG, &s->sequence_config));

    /* MSRC and TCC off by default, as in the C driver. */
    VLX_TRY(set_sequence_step_enable(d, SEQ_TCC, 0));
    VLX_TRY(set_sequence_step_enable(d, SEQ_MSRC, 0));

    VLX_TRY(get_vcsel_pulse_period(d, VCSEL_PRE_RANGE, &s->pre_pclks));
    VLX_TRY(get_vcsel_pulse_period(d, VCSEL_FINAL_RANGE, &s->final_pclks));
    VLX_TRY(get_sequence_step_timeout(d, SEQ_PRE_RANGE, &s->pre_timeout_us));
    return get_sequence_step_timeout(d, SEQ_FINAL_RANGE, &s->final_timeout_us);
}

/* ── reference calibration (VHV / phase) ───────────────────────────────── */

static int perform_single_ref_calibration(vlx_driver *d, uint8_t vhv_init_byte)
{
    VLX_TRY(vlx_wr_byte(P, SYSRANGE_START, (uint8_t)(SYSRANGE_MODE_START_STOP | vhv_init_byte)));
    VLX_TRY(vlx_wait_data_ready(d, 1000));
    VLX_TRY(clear_interrupt(d));
    return vlx_wr_byte(P, SYSRANGE_START, 0x00);
}

/* VL53L0X_ref_calibration_io() in its read direction. */
static int ref_calibration_io_read(vlx_driver *d, bool vhv_enable, bool phase_enable)
{
    uint8_t v;
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    if (vhv_enable) VLX_TRY(vlx_rd_byte(P, 0xCB, &v));
    if (phase_enable) VLX_TRY(vlx_rd_byte(P, 0xEE, &v));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x01));
    return vlx_wr_byte(P, 0xFF, 0x00);
}

/* VL53L0X_PerformRefCalibration(): without it every frame has signal 0. */
static int perform_ref_calibration(vlx_driver *d)
{
    l0x_drv *s = ld(d);
    uint8_t sequence_config = s->sequence_config;
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, 0x01)); /* VHV only */
    VLX_TRY(perform_single_ref_calibration(d, 0x40));
    VLX_TRY(ref_calibration_io_read(d, true, false));
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, 0x02)); /* phase only */
    VLX_TRY(perform_single_ref_calibration(d, 0x00));
    VLX_TRY(ref_calibration_io_read(d, false, true));
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, sequence_config));
    s->sequence_config = sequence_config;
    return DEPZ_OK;
}

static int perform_phase_calibration(vlx_driver *d)
{
    l0x_drv *s = ld(d);
    uint8_t sequence_config = s->sequence_config;
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, 0x02));
    VLX_TRY(perform_single_ref_calibration(d, 0x00));
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, sequence_config));
    s->sequence_config = sequence_config;
    return DEPZ_OK;
}

static int perform_ref_spad_management(vlx_driver *d, uint32_t *count, bool *is_aperture_out);

static int sensor_init(vlx_driver *d)
{
    VLX_TRY(vlx_set_addr_width(P, 1));
    VLX_TRY(vlx_set_i2c_speed(P, VLX_I2C_KHZ_BOOT)); /* no FM+ pad: 400 kHz is also the ceiling */
    VLX_TRY(wait_boot(d, 1000));
    VLX_TRY(reset_device(d, 1000));
    VLX_TRY(data_init(d));
    VLX_TRY(static_init(d));
    if (!ld(d)->ref_spads_from_nvm) {
        uint32_t c;
        bool a;
        VLX_TRY(perform_ref_spad_management(d, &c, &a));
    }
    VLX_TRY(perform_ref_calibration(d));
    ld(d)->device_mode = ld(d)->continuous_mode = DEVICEMODE_CONTINUOUS_RANGING;
    ld(d)->mode = MODES[0].name;
    return DEPZ_OK;
}

/* ── ranging ───────────────────────────────────────────────────────────── */

/* The prologue of every VL53L0X_StartMeasurement(): the stop variable back. */
static int arm_stop_variable(vlx_driver *d)
{
    VLX_TRY(vlx_wr_byte(P, 0x80, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0x91, ld(d)->stop_variable));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    return vlx_wr_byte(P, 0x80, 0x00);
}

/* A single shot (a calibration, a poll) leaves the device mode on single
 * ranging; the continuous mode chosen before it is resumed. */
static int start_ranging(vlx_driver *d)
{
    int mode;
    if (ld(d)->device_mode == DEVICEMODE_SINGLE_RANGING && ld(d)->continuous_mode != 0)
        ld(d)->device_mode = ld(d)->continuous_mode;
    mode = ld(d)->device_mode;
    VLX_TRY(arm_stop_variable(d));
    if (mode == DEVICEMODE_CONTINUOUS_RANGING) return vlx_wr_byte(P, SYSRANGE_START, SYSRANGE_MODE_BACKTOBACK);
    if (mode == DEVICEMODE_CONTINUOUS_TIMED) return vlx_wr_byte(P, SYSRANGE_START, SYSRANGE_MODE_TIMED);
    return VL53_ERR("start_ranging() needs a continuous device mode");
}

static int stop_ranging(vlx_driver *d)
{
    VLX_TRY(vlx_wr_byte(P, SYSRANGE_START, SYSRANGE_MODE_SINGLESHOT));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0x91, 0x00));
    VLX_TRY(vlx_wr_byte(P, 0x00, 0x01));
    return vlx_wr_byte(P, 0xFF, 0x00);
}

static void stream_block(vlx_driver *d, uint16_t *addr, uint16_t *len)
{
    (void)d;
    *addr = RESULT_RANGE_STATUS;
    *len = RESULT_BLOCK_LEN;
}

/* ── result decoding: pure arithmetic over the block and cached data ───── */

static int64_t get_total_xtalk_rate(l0x_drv *s, const ranging_data *r)
{
    if (!s->xtalk_enable) return 0;
    return (r->effective_spads * s->xtalk_rate_mcps + 0x80) >> 8;
}

/* VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out. */
static int64_t calc_sigma_estimate(l0x_drv *s, const ranging_data *r)
{
    const int64_t c_pulse_width = 800, c_ambient_width = 600;
    const int64_t c_dflt_final_range_integration_time_ms = 0x00190000;
    const int64_t c_vcsel_pulse_width_ps = 4700, c_sigma_est_max = 0x028F87AE, c_sigma_est_rtn_max = 0xF000;
    const int64_t c_amb_to_signal_ratio_max = 0xF0000000LL / 600, c_tof_per_mm_ps = 0x0006999A;
    const int64_t c_16bit_rounding = 0x00008000, c_max_xtalk_kcps = 0x00320000, c_pll_period_ps = 1655;
    int64_t ambient_kcps = (r->ambient_rate_mcps * 1000) >> 16;
    int64_t xtalk_mcps = get_total_xtalk_rate(s, r);
    int64_t total_signal = r->signal_rate_mcps + xtalk_mcps;
    int64_t peak_kcps = ((total_signal * 1000) + 0x8000) >> 16;
    int64_t xtalk_kcps = xtalk_mcps * 1000 < c_max_xtalk_kcps ? xtalk_mcps * 1000 : c_max_xtalk_kcps;
    int64_t final_mclks = calc_timeout_mclks(s->final_timeout_us, s->final_pclks);
    int64_t pre_mclks = calc_timeout_mclks(s->pre_timeout_us, s->pre_pclks);
    int64_t vcsel_width = s->final_pclks == 8 ? 2 : 3;
    int64_t peak_dur, t2408, events, p1, p2, p3, delta_t_ps, diff1, diff2, xtalk_corr, pw, sqr1, sqr2;
    int64_t sqrt_centi_ns, rtn, integration_ms, ref, estimate;

    peak_dur = vcsel_width * 2048 * (pre_mclks + final_mclks);
    peak_dur = fdiv(peak_dur + 500, 1000);
    peak_dur *= c_pll_period_ps;
    peak_dur = fdiv(peak_dur + 500, 1000);

    t2408 = (total_signal + 0x80) >> 8;
    events = ((t2408 * peak_dur) + 0x80) >> 8;
    if (peak_kcps == 0) return c_sigma_est_max;
    if (events < 1) events = 1;

    p1 = c_pulse_width;
    p2 = fdiv(ambient_kcps << 16, peak_kcps);
    if (p2 > c_amb_to_signal_ratio_max) p2 = c_amb_to_signal_ratio_max;
    p2 *= c_ambient_width;
    p3 = 2 * isqrt(events * 12);

    delta_t_ps = r->distance_mm * c_tof_per_mm_ps;

    diff1 = fdiv(u32(u32((peak_kcps << 16) - 2 * xtalk_kcps) + 500), 1000);
    diff2 = fdiv((peak_kcps << 16) + 500, 1000);
    diff1 <<= 8;
    xtalk_corr = llabs(fdiv(diff1, diff2)) << 8;

    pw = fdiv(delta_t_ps, c_vcsel_pulse_width_ps);
    pw = u32(pw * u32(((int64_t)1 << 16) - xtalk_corr));
    pw = (pw + c_16bit_rounding) >> 16;
    pw += (int64_t)1 << 16;
    /* Squaring 1.xx would leave 32 bits, so the C driver halves it first. */
    pw >>= 1;
    pw = (pw * pw) >> 14;

    sqr1 = ((pw * p1) + 0x8000) >> 16;
    sqr1 *= sqr1;
    sqr2 = (p2 + 0x8000) >> 16;
    sqr2 *= sqr2;

    sqrt_centi_ns = isqrt(sqr1 + sqr2) << 16;
    rtn = fdiv(fdiv(sqrt_centi_ns + 50, 100), p3);
    rtn *= SPEED_OF_LIGHT_IN_AIR;
    rtn = fdiv(rtn + 5000, 10000);
    if (rtn > c_sigma_est_rtn_max) rtn = c_sigma_est_rtn_max;

    integration_ms = fdiv(s->final_timeout_us + s->pre_timeout_us + 500, 1000);
    /* 1 mm * 25 ms / the actual integration time. */
    ref = isqrt(fdiv(c_dflt_final_range_integration_time_ms + fdiv(integration_ms, 2), integration_ms)) << 8;
    ref = fdiv(ref + 500, 1000);

    estimate = 1000 * isqrt(rtn * rtn + ref * ref);
    if (peak_kcps < 1 || events < 1 || estimate > c_sigma_est_max) estimate = c_sigma_est_max;
    return estimate;
}

/* VL53L0X_calc_dmax(): ambient rate -> max range, linear between points. */
static int64_t calc_dmax(int64_t amb_rate)
{
    int i1, i0;
    int64_t slope;
    if (amb_rate <= DMAX_LUT_AMB[0]) return DMAX_LUT_MM[0] >> 16;
    if (amb_rate >= DMAX_LUT_AMB[6]) return DMAX_LUT_MM[6] >> 16;
    for (i1 = 0; i1 < 7 && amb_rate > DMAX_LUT_AMB[i1]; i1++) {}
    i0 = i1 ? i1 - 1 : 0;
    if (i0 == i1) return DMAX_LUT_MM[i0] >> 16;
    if (DMAX_LUT_AMB[i1] == DMAX_LUT_AMB[i0]) return DMAX_LUT_MM[i0] >> 16;
    slope = fdiv(u32(DMAX_LUT_MM[i0] - DMAX_LUT_MM[i1]), (DMAX_LUT_AMB[i1] - DMAX_LUT_AMB[i0]) >> 8);
    return (((DMAX_LUT_AMB[i1] - amb_rate) >> 8) * slope + DMAX_LUT_MM[i1]) >> 16;
}

/* VL53L0X_get_pal_range_status(): device status plus the enabled limit
 * checks. DataInit leaves the signal-ref clip and range-ignore checks off, so
 * only the sigma check runs — arithmetic over cached data. */
static void get_pal_range_status(l0x_drv *s, ranging_data *r)
{
    int internal = (r->device_range_status & 0x78) >> 3;
    bool none_flag = internal == 0 || internal == 5 || internal == 7 || internal >= 12;
    bool sigma_limit_flag = false;
    if (s->limit_enable[CHECK_SIGMA_FINAL_RANGE]) {
        int64_t est = calc_sigma_estimate(s, r), limit = s->limit_value[CHECK_SIGMA_FINAL_RANGE];
        r->sigma_mm = est >> 16;
        r->dmax_mm = calc_dmax(r->ambient_rate_mcps);
        sigma_limit_flag = limit > 0 && est > limit;
    }
    if (none_flag) r->range_status = 255;
    else if (internal >= 1 && internal <= 3) r->range_status = 5; /* hardware fail */
    else if (internal == 6 || internal == 9) r->range_status = 4; /* phase fail */
    else if (internal == 8 || internal == 10) r->range_status = 3; /* min range */
    else if (internal == 4) r->range_status = 2; /* signal fail */
    else if (sigma_limit_flag) r->range_status = 1; /* sigma fail */
    else r->range_status = 0;
}

/* VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14. */
static int parse_result_block(l0x_drv *s, const uint8_t *raw, size_t len, ranging_data *r)
{
    int64_t distance;
    if (len < RESULT_BLOCK_LEN) return VL53_ERR("result block too short: %zu bytes", len);
    memset(r, 0, sizeof *r);
    distance = (raw[10] << 8) + raw[11];
    r->signal_rate_mcps = (int64_t)((raw[6] << 8) + raw[7]) << 9; /* 9.7 -> 16.16 */
    r->ambient_rate_mcps = (int64_t)((raw[8] << 8) + raw[9]) << 9;
    r->effective_spads = (raw[2] << 8) + raw[3]; /* 8.8 */
    r->device_range_status = raw[0];
    r->distance_mm = s->range_fractional ? distance >> 2 : distance;
    get_pal_range_status(s, r);
    return DEPZ_OK;
}

static int get_result(vlx_driver *d, ranging_data *r)
{
    uint8_t raw[RESULT_BLOCK_LEN];
    VLX_TRY(vlx_rd_multi(P, RESULT_RANGE_STATUS, raw, sizeof raw));
    return parse_result_block(ld(d), raw, sizeof raw, r);
}

static int decode(vlx_driver *d, const uint8_t *raw, size_t len, depz_vl53lx_measurement *m)
{
    ranging_data r;
    if (raw) VLX_TRY(parse_result_block(ld(d), raw, len, &r));
    else VLX_TRY(get_result(d, &r));
    m->distance_mm = (int32_t)r.distance_mm;
    m->status = r.range_status;
    m->status_text = status_text(r.range_status);
    m->signal_kcps = (double)((r.signal_rate_mcps * 1000) >> 16);
    m->ambient_kcps = (double)((r.ambient_rate_mcps * 1000) >> 16);
    m->sigma_mm = (double)r.sigma_mm;
    m->spads = (double)(r.effective_spads >> 8);
    m->dmax_mm = (int32_t)r.dmax_mm;
    m->device_range_status = r.device_range_status;
    return DEPZ_OK;
}

/* VL53L0X_PerformSingleRangingMeasurement(): leaves the device mode single. */
static int perform_single_ranging_measurement(vlx_driver *d, ranging_data *r)
{
    int i;
    ld(d)->device_mode = DEVICEMODE_SINGLE_RANGING;
    VLX_TRY(arm_stop_variable(d));
    VLX_TRY(vlx_wr_byte(P, SYSRANGE_START, 0x01));
    for (i = 0; i < DEFAULT_MAX_LOOP; i++) {
        uint8_t v;
        VLX_TRY(vlx_rd_byte(P, SYSRANGE_START, &v));
        if (!(v & SYSRANGE_MODE_START_STOP)) break;
    }
    if (i == DEFAULT_MAX_LOOP) return VL53_ERR("the single-shot start bit never cleared");
    VLX_TRY(vlx_wait_data_ready(d, 1000));
    VLX_TRY(get_result(d, r));
    return clear_interrupt(d);
}

/* ── timing ────────────────────────────────────────────────────────────── */

static int set_range_timing(vlx_driver *d, int budget_ms, int inter_ms)
{
    uint16_t osc;
    int64_t value;
    VLX_TRY(set_measurement_timing_budget(d, (int64_t)budget_ms * 1000));
    VLX_TRY(vlx_rd_word(P, OSC_CALIBRATE_VAL, &osc));
    value = osc ? (int64_t)inter_ms * osc : inter_ms;
    VLX_TRY(vlx_wr_dword(P, SYSTEM_INTERMEASUREMENT_PERIOD, (uint32_t)((uint64_t)value & 0xFFFFFFFFu)));
    ld(d)->device_mode = ld(d)->continuous_mode =
        inter_ms == 0 ? DEVICEMODE_CONTINUOUS_RANGING : DEVICEMODE_CONTINUOUS_TIMED;
    return DEPZ_OK;
}

static int get_range_timing(vlx_driver *d, int *budget_ms, int *inter_ms)
{
    int64_t budget;
    uint16_t osc;
    uint32_t value;
    VLX_TRY(get_measurement_timing_budget(d, &budget));
    VLX_TRY(vlx_rd_word(P, OSC_CALIBRATE_VAL, &osc));
    VLX_TRY(vlx_rd_dword(P, SYSTEM_INTERMEASUREMENT_PERIOD, &value));
    *budget_ms = (int)fdiv(budget, 1000);
    *inter_ms = (int)(osc ? value / osc : value);
    return DEPZ_OK;
}

/* ── VCSEL pulse period and the ranging profiles ───────────────────────── */

static int set_vcsel_pulse_period(vlx_driver *d, int type, int pclks)
{
    l0x_drv *s = ld(d);
    int phase_high = -1;
    if (type == VCSEL_PRE_RANGE) {
        phase_high = pclks == 12 ? 0x18 : pclks == 14 ? 0x30 : pclks == 16 ? 0x40 : pclks == 18 ? 0x50 : -1;
    } else {
        phase_high = pclks == 8 ? 0x10 : pclks == 10 ? 0x28 : pclks == 12 ? 0x38 : pclks == 14 ? 0x48 : -1;
    }
    if (pclks % 2 || phase_high < 0)
        return depz_fail(DEPZ_E_ARG, "vl53l0x: VCSEL period %d PCLK is out of range for the %s", pclks,
                         type == VCSEL_PRE_RANGE ? "pre-range" : "final range");

    if (type == VCSEL_PRE_RANGE) {
        VLX_TRY(vlx_wr_byte(P, PRE_RANGE_CONFIG_VALID_PHASE_HIGH, (uint8_t)phase_high));
        VLX_TRY(vlx_wr_byte(P, PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08));
    } else {
        /* The final range also retunes the VCSEL width and phase-cal limits. */
        uint8_t width = pclks == 8 ? 0x02 : 0x03;
        uint8_t timeout = pclks == 8 ? 0x0C : pclks == 10 ? 0x09 : pclks == 12 ? 0x08 : 0x07;
        uint8_t phasecal_lim = pclks == 8 ? 0x30 : 0x20;
        VLX_TRY(vlx_wr_byte(P, FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, (uint8_t)phase_high));
        VLX_TRY(vlx_wr_byte(P, FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08));
        VLX_TRY(vlx_wr_byte(P, GLOBAL_CONFIG_VCSEL_WIDTH, width));
        VLX_TRY(vlx_wr_byte(P, ALGO_PHASECAL_CONFIG_TIMEOUT, timeout));
        VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
        VLX_TRY(vlx_wr_byte(P, ALGO_PHASECAL_LIM, phasecal_lim));
        VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    }

    /* The timeouts are in macro periods, which the VCSEL period sets: read
     * each in the old period, write it in the new. */
    if (type == VCSEL_PRE_RANGE) {
        int64_t pre_timeout, msrc_timeout;
        VLX_TRY(get_sequence_step_timeout(d, SEQ_PRE_RANGE, &pre_timeout));
        VLX_TRY(get_sequence_step_timeout(d, SEQ_MSRC, &msrc_timeout));
        VLX_TRY(vlx_wr_byte(P, PRE_RANGE_CONFIG_VCSEL_PERIOD, (uint8_t)encode_vcsel_period(pclks)));
        VLX_TRY(set_sequence_step_timeout(d, SEQ_PRE_RANGE, pre_timeout));
        VLX_TRY(set_sequence_step_timeout(d, SEQ_MSRC, msrc_timeout));
        s->pre_pclks = pclks;
    } else {
        int64_t final_timeout;
        VLX_TRY(get_sequence_step_timeout(d, SEQ_FINAL_RANGE, &final_timeout));
        VLX_TRY(vlx_wr_byte(P, FINAL_RANGE_CONFIG_VCSEL_PERIOD, (uint8_t)encode_vcsel_period(pclks)));
        VLX_TRY(set_sequence_step_timeout(d, SEQ_FINAL_RANGE, final_timeout));
        s->final_pclks = pclks;
    }
    VLX_TRY(set_measurement_timing_budget(d, s->budget_us));
    return perform_phase_calibration(d);
}

static size_t modes(const vlx_driver *d, const char **names, size_t cap)
{
    size_t i;
    (void)d;
    for (i = 0; i < 4 && i < cap; i++) names[i] = MODES[i].name;
    return 4;
}

static int set_mode(vlx_driver *d, const char *name)
{
    const l0x_mode *m = NULL;
    int i;
    for (i = 0; i < 4; i++)
        if (!strcmp(MODES[i].name, name)) m = &MODES[i];
    if (!m) return depz_fail(DEPZ_E_ARG, "vl53l0x: no such mode: %s (have default, long-range, high-speed, high-accuracy)", name);
    VLX_TRY(set_limit_check_enable(d, CHECK_SIGMA_FINAL_RANGE, 1));
    VLX_TRY(set_limit_check_enable(d, CHECK_SIGNAL_RATE_FINAL_RANGE, 1));
    VLX_TRY(set_limit_check_value(d, CHECK_SIGNAL_RATE_FINAL_RANGE, (int64_t)(m->signal_mcps * 65536)));
    VLX_TRY(set_limit_check_value(d, CHECK_SIGMA_FINAL_RANGE, (int64_t)m->sigma_mm * 65536));
    VLX_TRY(set_measurement_timing_budget(d, m->budget_us));
    VLX_TRY(set_vcsel_pulse_period(d, VCSEL_PRE_RANGE, m->pre));
    VLX_TRY(set_vcsel_pulse_period(d, VCSEL_FINAL_RANGE, m->fin));
    ld(d)->mode = m->name;
    return DEPZ_OK;
}

static int get_mode(vlx_driver *d, const char **name)
{
    *name = ld(d)->mode;
    return DEPZ_OK;
}

/* ST's range for each final-range VCSEL period the profiles above use: the
 * period sets how far the phase stays unambiguous. Checked at a wall ~1.8 m:
 * final 10 fails it with status 4 in all three short profiles, final 14
 * ranges. Looked up by the period read from the sensor, so it follows the
 * configuration, not the profile name; 0 for a period no profile sets. */
static int reach_mm(vlx_driver *d, uint32_t *out)
{
    int pclks;
    VLX_TRY(get_vcsel_pulse_period(d, VCSEL_FINAL_RANGE, &pclks));
    *out = pclks == 10 ? 1200u : pclks == 14 ? 2000u : 0u;
    return DEPZ_OK;
}

/* ── offset ────────────────────────────────────────────────────────────── */

/* The register is 10.2 format in mm: 250 um steps. */
static int set_offset_um(vlx_driver *d, int64_t offset_um)
{
    int64_t steps, encoded;
    if (offset_um < -512000) offset_um = -512000;
    if (offset_um > 511000) offset_um = 511000;
    steps = llabs(offset_um) / 250;
    encoded = offset_um >= 0 ? steps : 4096 - steps;
    return vlx_wr_word(P, ALGO_PART_TO_PART_RANGE_OFFSET_MM, (uint16_t)(encoded & 0xFFFF));
}

static int get_offset_um(vlx_driver *d, int64_t *um)
{
    uint16_t v;
    int64_t reg;
    VLX_TRY(vlx_rd_word(P, ALGO_PART_TO_PART_RANGE_OFFSET_MM, &v));
    reg = v & 0x0FFF;
    *um = reg > 2047 ? (reg - 4096) * 250 : reg * 250;
    return DEPZ_OK;
}

static int set_offset(vlx_driver *d, int32_t mm) { return set_offset_um(d, (int64_t)mm * 1000); }

static int get_offset(vlx_driver *d, int32_t *mm)
{
    int64_t um;
    VLX_TRY(get_offset_um(d, &um));
    *mm = (int32_t)nearbyint(um / 1000.0); /* round half to even, as Python */
    return DEPZ_OK;
}

static int calibrate_offset(vlx_driver *d, int target_mm, int nb, int32_t *out)
{
    bool steps[SEQ_COUNT], tcc_was_on;
    int64_t sum = 0, count = 0, mean_mm, offset_um;
    int i;
    if (nb <= 0) nb = 50;
    if (target_mm <= 0) return depz_fail(DEPZ_E_ARG, "vl53l0x: the calibration distance must be positive");
    VLX_TRY(set_offset_um(d, 0));
    VLX_TRY(get_sequence_step_enables(d, steps));
    tcc_was_on = steps[SEQ_TCC];
    VLX_TRY(set_sequence_step_enable(d, SEQ_TCC, 0));
    VLX_TRY(set_limit_check_enable(d, CHECK_RANGE_IGNORE_THRESHOLD, 0));
    for (i = 0; i < nb; i++) {
        ranging_data r;
        VLX_TRY(perform_single_ranging_measurement(d, &r));
        if (r.range_status == 0) {
            sum = (sum + r.distance_mm) & 0xFFFF;
            count++;
        }
    }
    if (count == 0)
        return VL53_ERR("offset calibration got no valid measurement - check that a target is in front of the sensor");
    mean_mm = fdiv(2 * sum + count, 2 * count); /* round half up */
    if (mean_mm == 0)
        return VL53_ERR("the mean range came out 0 mm - the target is closer than the part-to-part offset; "
                        "move it out to 100..400 mm and calibrate again");
    offset_um = ((int64_t)target_mm - mean_mm) * 1000;
    VLX_TRY(set_offset_um(d, offset_um));
    if (tcc_was_on) VLX_TRY(set_sequence_step_enable(d, SEQ_TCC, 1));
    *out = (int32_t)nearbyint(offset_um / 1000.0);
    return DEPZ_OK;
}

/* ── crosstalk ─────────────────────────────────────────────────────────── */

/* Switching it off zeroes the register but keeps the host-side rate. */
static int set_xtalk_enable(vlx_driver *d, int enable)
{
    l0x_drv *s = ld(d);
    int64_t rate = enable ? s->xtalk_rate_mcps : 0;
    VLX_TRY(vlx_wr_word(P, CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, (uint16_t)((rate >> 3) & 0xFFFF))); /* 16.16 -> 3.13 */
    s->xtalk_enable = enable ? 1 : 0;
    return DEPZ_OK;
}

static int set_xtalk_rate_mcps(vlx_driver *d, int64_t rate)
{
    if (ld(d)->xtalk_enable)
        VLX_TRY(vlx_wr_word(P, CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, (uint16_t)((rate >> 3) & 0xFFFF)));
    ld(d)->xtalk_rate_mcps = rate;
    return DEPZ_OK;
}

static int set_xtalk(vlx_driver *d, int32_t kcps)
{
    VLX_TRY(set_xtalk_enable(d, 1));
    return set_xtalk_rate_mcps(d, fdiv((int64_t)kcps * 65536, 1000));
}

static int get_xtalk(vlx_driver *d, int32_t *kcps)
{
    uint16_t v;
    VLX_TRY(vlx_rd_word(P, CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, &v));
    *kcps = (int32_t)((((int64_t)v << 3) * 1000) >> 16);
    return DEPZ_OK;
}

static int calibrate_xtalk(vlx_driver *d, int target_mm, int nb, int32_t *out)
{
    int64_t sum_range = 0, sum_signal = 0, sum_spads = 0, count = 0;
    int64_t mean_signal, mean_range, mean_spads, mean_spads_int, cal_distance, rate;
    int i;
    if (nb <= 0) nb = 50;
    if (target_mm <= 0) return depz_fail(DEPZ_E_ARG, "vl53l0x: the calibration distance must be positive");
    VLX_TRY(set_xtalk_enable(d, 0));
    VLX_TRY(set_limit_check_enable(d, CHECK_RANGE_IGNORE_THRESHOLD, 0));
    for (i = 0; i < nb; i++) {
        ranging_data r;
        VLX_TRY(perform_single_ranging_measurement(d, &r));
        if (r.range_status == 0) {
            /* The C sums are uint16 / uint32; kept that way on purpose. */
            sum_range = (sum_range + r.distance_mm) & 0xFFFF;
            sum_signal = u32(sum_signal + r.signal_rate_mcps);
            sum_spads = (sum_spads + r.effective_spads / 256) & 0xFFFF;
            count++;
        }
    }
    if (count == 0)
        return VL53_ERR("crosstalk calibration got no valid measurement - check that a target is in front of the sensor");
    mean_signal = sum_signal / count;
    mean_range = u32(sum_range << 16) / count;
    mean_spads = u32(sum_spads << 16) / count;
    mean_spads_int = (mean_spads + 0x8000) >> 16;
    cal_distance = (int64_t)target_mm << 16;
    if (mean_spads_int == 0 || mean_range >= cal_distance) {
        rate = 0;
    } else {
        int64_t per_spad = mean_signal / mean_spads_int;
        per_spad = u32(per_spad * (((int64_t)1 << 16) - mean_range / target_mm));
        rate = (per_spad + 0x8000) >> 16;
    }
    VLX_TRY(set_xtalk_enable(d, 1));
    VLX_TRY(set_xtalk_rate_mcps(d, rate));
    *out = (int32_t)((rate * 1000) >> 16);
    return DEPZ_OK;
}

/* ── reference SPAD management ─────────────────────────────────────────── */

/* One shot with only the reference steps on -> the peak ref rate, 9.7. */
static int perform_ref_signal_measurement(vlx_driver *d, int64_t *peak)
{
    l0x_drv *s = ld(d);
    uint8_t sequence_config = s->sequence_config;
    uint16_t v;
    ranging_data r;
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, 0xC0));
    VLX_TRY(perform_single_ranging_measurement(d, &r));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x01));
    VLX_TRY(vlx_rd_word(P, RESULT_PEAK_SIGNAL_RATE_REF, &v));
    VLX_TRY(vlx_wr_byte(P, 0xFF, 0x00));
    VLX_TRY(vlx_wr_byte(P, SYSTEM_SEQUENCE_CONFIG, sequence_config));
    s->sequence_config = sequence_config;
    *peak = v;
    return DEPZ_OK;
}

static void store_ref_spads(l0x_drv *s, int count, int aperture, const uint8_t *spad_array)
{
    memcpy(s->ref_spad_enables, spad_array, REF_SPAD_BUFFER_SIZE);
    s->ref_spad_count = count;
    s->ref_spad_type = aperture;
}

/* VL53L0X_perform_ref_spad_management(): the minimum non-aperture set, then
 * one SPAD at a time until the peak ref rate lands closest to 20 Mcps; the
 * aperture quadrant when even the minimum overshoots. */
static int perform_ref_spad_management(vlx_driver *d, uint32_t *count_out, bool *aperture_out)
{
    l0x_drv *s = ld(d);
    const int start = 0xB4, minimum = 3, max_spad_count = 44;
    const int64_t target = s->target_ref_rate;
    uint8_t spad_array[REF_SPAD_BUFFER_SIZE] = {0}, last[REF_SPAD_BUFFER_SIZE];
    int curr, need_apt = 0, ref_count;
    int64_t peak, last_diff;

    VLX_TRY(spad_prologue(d, start));
    VLX_TRY(vlx_wr_byte(P, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00));
    VLX_TRY(perform_ref_calibration(d));

    VLX_TRY(add_good_spads(s, spad_array, start, 0, minimum, need_apt, &curr));
    VLX_TRY(write_check_spad_map(d, spad_array));
    VLX_TRY(perform_ref_signal_measurement(d, &peak));

    if (peak > target) {
        /* Too bright even at the minimum: start over on aperture SPADs. */
        memset(spad_array, 0, sizeof spad_array);
        for (;;) {
            bool ap;
            VLX_TRY(is_aperture(start + curr, &ap));
            if (ap || curr >= max_spad_count) break;
            curr++;
        }
        need_apt = 1;
        VLX_TRY(add_good_spads(s, spad_array, start, curr, minimum, need_apt, &curr));
        VLX_TRY(write_check_spad_map(d, spad_array));
        VLX_TRY(perform_ref_signal_measurement(d, &peak));
        if (peak > target) {
            /* Nothing more to give: the minimum aperture set is the answer. */
            store_ref_spads(s, minimum, 1, spad_array);
            *count_out = (uint32_t)minimum;
            *aperture_out = true;
            return DEPZ_OK;
        }
    }

    ref_count = minimum;
    memcpy(last, spad_array, sizeof last);
    last_diff = llabs(peak - target);

    while (peak < target) {
        bool ap;
        int64_t diff;
        int next_good = get_next_good_spad(s->good_spad_map, REF_SPAD_BUFFER_SIZE, curr);
        if (next_good == -1) return VL53_ERR("ran out of good reference SPADs");
        VLX_TRY(is_aperture(start + next_good, &ap));
        if (ap != (need_apt != 0)) break; /* the quadrant is exhausted */
        ref_count++;
        curr = next_good;
        if (curr / 8 >= REF_SPAD_BUFFER_SIZE) return VL53_ERR("reference SPAD index out of range");
        spad_array[curr / 8] |= (uint8_t)(1 << (curr % 8));
        curr++;
        VLX_TRY(vlx_wr_multi(P, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_array, REF_SPAD_BUFFER_SIZE));
        VLX_TRY(perform_ref_signal_measurement(d, &peak));
        diff = llabs(peak - target);
        if (peak > target) {
            if (diff > last_diff) {
                /* The previous map came closer; go back to it. */
                VLX_TRY(vlx_wr_multi(P, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, last, REF_SPAD_BUFFER_SIZE));
                memcpy(spad_array, last, sizeof last);
                ref_count--;
            }
            break;
        }
        last_diff = diff;
        memcpy(last, spad_array, sizeof last);
    }
    store_ref_spads(s, ref_count, need_apt, spad_array);
    *count_out = (uint32_t)ref_count;
    *aperture_out = need_apt != 0;
    return DEPZ_OK;
}

/* ── the driver ────────────────────────────────────────────────────────── */

static void destroy(vlx_driver *d) { free(d); }

/* The public re-measure on a live session. Not in the C driver: the stop
 * variable goes back first. stop_ranging() (VL53L0X_StopMeasurement) zeroes
 * register 0x91 and the VHV/phase single shots do not arm it the way every
 * StartMeasurement does; left at 0 the reference rate reads far too high, the
 * aperture SPADs get picked and every frame after is Signal Fail (TB9BGETA6M,
 * 30.09.2026). init calls the bare routine: after the reset 0x91 holds it. */
static int ref_spad_management_live(vlx_driver *d, uint32_t *count_out, bool *aperture_out)
{
    VLX_TRY(arm_stop_variable(d));
    return perform_ref_spad_management(d, count_out, aperture_out);
}

static const vlx_ops L0X_OPS = {
    "VL53L0X", 1, 2, {{SYSTEM_INTERRUPT_CLEAR, 0x01}, {SYSTEM_INTERRUPT_CLEAR, 0x00}}, 400,
    DEPZ_VL53LX_CAP_MODE | DEPZ_VL53LX_CAP_TIMING | DEPZ_VL53LX_CAP_OFFSET | DEPZ_VL53LX_CAP_XTALK |
        DEPZ_VL53LX_CAP_CALIB_OFFSET | DEPZ_VL53LX_CAP_CALIB_XTALK | DEPZ_VL53LX_CAP_REFSPAD,
    false, 20, 200,
    model_id, sensor_init, start_ranging, stop_ranging, check_for_data_ready, clear_interrupt, stream_block,
    decode, set_range_timing, get_range_timing, destroy,
    modes, NULL, set_mode, get_mode, reach_mm, set_offset, get_offset, set_xtalk, get_xtalk, calibrate_offset,
    calibrate_xtalk, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    ref_spad_management_live,
    NULL};

vlx_driver *vlx_new_l0x(vlx_plat *p, depz_vl53lx_product product)
{
    l0x_drv *s = (l0x_drv *)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->base.ops = &L0X_OPS;
    s->base.p = p;
    s->base.product = product;
    s->linearity_gain = 1000;
    s->osc_frequency = 618660;
    s->device_mode = DEVICEMODE_SINGLE_RANGING;
    s->pin0_functionality = GPIOFUNCTIONALITY_NEW_MEASURE_READY;
    s->target_ref_rate = TARGET_REF_RATE;
    s->ref_spads_from_nvm = true;
    s->mode = MODES[0].name;
    return &s->base;
}
