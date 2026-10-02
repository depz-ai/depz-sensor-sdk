/* vl53lx_bare.c — ST's VL53LX BareDriver on the host: VL53L1CX / L1CB / L3CX /
 * L4CD / L4CX in histogram mode (the Python SDK's vl53lx/uld/bare/: image.py,
 * nvm.py, core.py, driver.py). The device is configured in one shot: the
 * preset builders only edit the host image, and 0x0001..0x0087 goes out as one
 * transfer when ranging starts. A faithful port — register traffic, order and
 * integer widths included; the post-processing is vl53lx_bare_hist.c. */
#include "vl53lx_bare.h"

#include <stdlib.h>

/* ── registers (vl53lx_register_map.h, vl53lx_hist_map.h) ──────────────── */

#define POWER_MANAGEMENT__GO1_POWER_FORCE 0x0083
#define FIRMWARE__ENABLE                  0x0085
#define RESULT__OSC_CALIBRATE_VAL         0x00DE
#define PATCH__CTRL                       0x0470
#define PATCH__JMP_ENABLES                0x0472
#define PATCH__DATA_ENABLES               0x0474
#define PATCH__OFFSET_0                   0x0476
#define PATCH__ADDRESS_0                  0x0496
#define GPIO_HV_MUX__CTRL                 0x0030
#define GPIO__TIO_HV_STATUS               0x0031
#define SYSTEM__INTERRUPT_CLEAR           0x0086
#define FIRMWARE__SYSTEM_STATUS           0x00E5
#define PAD_I2C_HV__CONFIG                0x002D
#define IDENTIFICATION__MODEL_ID          0x010F

#define HIST_BLOCK_ADDR 0x0088  /* result__interrupt_status */
#define HIST_BLOCK_LEN  83      /* ..0x00DA */
#define RESULT__HISTOGRAM_BIN_0_2      0x008E
#define RESULT__HISTOGRAM_BIN_23_0     0x00D5
#define PHASECAL_RESULT__REFERENCE_PHASE 0x00D6
#define PHASECAL_RESULT__VCSEL_START   0x00D8
#define RESULT__HISTOGRAM_BIN_23_0_MSB 0x00D9
#define RESULT__HISTOGRAM_BIN_23_0_LSB 0x00DA

#define RANGING_CORE__CLK_CTRL1                  0x0683
#define RANGING_CORE__NVM_CTRL__MODE             0x0780
#define RANGING_CORE__NVM_CTRL__PDN              0x0781
#define RANGING_CORE__NVM_CTRL__READN            0x0783
#define RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB  0x0784
#define RANGING_CORE__NVM_CTRL__DATAOUT_MMM      0x0790
#define RANGING_CORE__NVM_CTRL__ADDR             0x0794
#define NVM_CTRL_PULSE_WIDTH 0x0004

/* ── register settings ─────────────────────────────────────────────────── */

#define MEASUREMENTMODE_BACKTOBACK  0x20
#define MEASUREMENTMODE_ABORT       0x80
#define MEASUREMENTMODE_STOP_MASK   0x0F
#define MEASUREMENTMODE_MODE_MASK   0xF0
#define SCHEDULERMODE_STREAMING     0x01
#define SCHEDULERMODE_HISTOGRAM     0x02
#define READOUTMODE_SINGLE_SD       (0x00 << 2)
#define READOUTMODE_DUAL_SD         (0x01 << 2)
#define GPH_ID_MASK                 0x02
#define INTERRUPT_CONFIG_NEW_SAMPLE_READY 0x20
#define CLEAR_RANGE_INT             0x01
#define INTERRUPTPOLARITY_ACTIVE_LOW 0x10
#define GPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS 0x01
#define DSSMODE__TARGET_RATE        1

#define SEQUENCE_VHV_EN      0x01
#define SEQUENCE_PHASECAL_EN 0x02
#define SEQUENCE_DSS1_EN     0x08
#define SEQUENCE_DSS2_EN     0x10
#define SEQUENCE_MM1_EN      0x20
#define SEQUENCE_MM2_EN      0x40
#define SEQUENCE_RANGE_EN    0x80

#define SPAD_ARRAY_WIDTH  16
#define SPAD_ARRAY_HEIGHT 16
#define RTN_SPAD_APERTURE_TRANSMISSION 0x0038
#define RTN_SPAD_UNITY_TRANSMISSION    0x0100
#define MACRO_PERIOD_VCSEL_PERIODS (256 + 2048)

/* The family-wide timing call: a fixed guard comes off the budget and the
 * rest is split six ways (VL53LX_SetMeasurementTimingBudgetMicroSeconds). */
#define TIMING_GUARD_US 1700
#define TIMING_DIVISOR  6
#define FDA_MAX_TIMING_BUDGET_US 550000
/* The L4CX BareDriver (STSW-IMG029) narrows both for an L4 die: a lower budget
 * ceiling, and no short mode at all (see is_l4()). */
#define L4_FDA_MAX_TIMING_BUDGET_US 200000

/* ST's histogram preset leaves the pad at 0x00; Fast Mode Plus is a property
 * of the die, so it is set here and never cleared (driver.py). */
#define FMP_PAD_CONFIG 0x12

typedef enum { MODE_SHORT = 0, MODE_MEDIUM = 1, MODE_LONG = 2 } dist_mode;
static const char *const MODE_NAMES[3] = {"short", "medium", "long"};

/* core.py HistConfig: VL53LX_histogram_config_t. */
typedef struct {
    int64_t low_even_0_1, low_even_2_3, low_even_4_5, low_odd_0_1, low_odd_2_3, low_odd_4_5;
    int64_t mid_even_0_1, mid_even_2_3, mid_even_4_5, mid_odd_0_1, mid_odd_2, mid_odd_3_4, mid_odd_5;
    int64_t user_bin_offset;
    int64_t high_even_0_1, high_even_2_3, high_even_4_5, high_odd_0_1, high_odd_2_3, high_odd_4_5;
    int64_t amb_thresh_low, amb_thresh_high, spad_array_selection;
} hist_cfg;

/* core.py LLState, single zone. */
typedef struct {
    vlxb_state_id cfg_device_state;
    int64_t cfg_stream_count, cfg_gph_id, cfg_timing_status;
    vlxb_state_id rd_device_state;
    int64_t rd_stream_count, rd_gph_id, rd_timing_status;
} ll_state;

/* The four NVM-only factory values read_p2p_data keeps. */
typedef struct { int64_t inner_spads, outer_spads, inner_rate, outer_rate; } add_off_cal;

typedef struct {
    vlx_driver base;
    /* BareDriver */
    vlxb_image img;
    hist_cfg hcfg;
    vlxb_hpp hpp;
    uint8_t rtn_good_spads[32];
    bool have_add_off;
    add_off_cal add_off;
    int64_t result_osc_calibrate_val;
    int64_t phasecal_timeout_us, mm_timeout_us, range_timeout_us;
    int64_t inter_measurement_period_ms;
    ll_state state;
    int64_t ambient_events_sum;   /* last frame's, for the bin sequence */
    /* VL53LX (driver.py) */
    dist_mode mode;
    int budget_ms, inter_ms;
    bool have_last_stream_count;
    int64_t last_stream_count;
    vlxb_history history;
} bare_drv;

static bare_drv *bd(vlx_driver *d) { return (bare_drv *)d; }
#define P (d->p)

/* ── image moves (image.py) ────────────────────────────────────────────── */

static int img_pull(vlx_driver *d, vlxb_block_id id)
{
    uint8_t raw[64];
    const vlxb_block_geom *g = &vlxb_blocks[id];
    VLX_TRY(vlx_rd_multi(P, g->base, raw, g->size));
    vlxb_decode(&bd(d)->img, id, raw);
    return DEPZ_OK;
}

static int img_push(vlx_driver *d, vlxb_block_id id)
{
    uint8_t raw[64];
    const vlxb_block_geom *g = &vlxb_blocks[id];
    vlxb_encode(&bd(d)->img, id, raw);
    return vlx_wr_multi(P, g->base, raw, g->size);
}

/* A run of blocks as one transfer; the reserved bytes between them go out as
 * zeros, as in the C driver. */
static int img_push_range(vlx_driver *d, const vlxb_block_id *ids, size_t n)
{
    uint8_t buf[256], blk[64];
    unsigned base = 0xFFFF, end = 0;
    size_t i;
    for (i = 0; i < n; i++) {
        const vlxb_block_geom *g = &vlxb_blocks[ids[i]];
        if (g->base < base) base = g->base;
        if ((unsigned)(g->base + g->size) > end) end = g->base + g->size;
    }
    memset(buf, 0, sizeof buf);
    for (i = 0; i < n; i++) {
        const vlxb_block_geom *g = &vlxb_blocks[ids[i]];
        vlxb_encode(&bd(d)->img, ids[i], blk);
        memcpy(buf + (g->base - base), blk, g->size);
    }
    return vlx_wr_multi(P, (uint16_t)base, buf, end - base);
}

/* ── NVM (nvm.py) ──────────────────────────────────────────────────────── */

static int nvm_enable(vlx_driver *d)
{
    vlxb_system_control *sc = &bd(d)->img.system_control;
    sc->firmware__enable = 0;
    VLX_TRY(vlx_wr_byte(P, FIRMWARE__ENABLE, 0));
    sc->power_management__go1_power_force = 1;
    VLX_TRY(vlx_wr_byte(P, POWER_MANAGEMENT__GO1_POWER_FORCE, 1));
    vlx_sleep_ms(P, 1); /* 250 us settling, rounded up */
    VLX_TRY(vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__PDN, 0x01));
    VLX_TRY(vlx_wr_byte(P, RANGING_CORE__CLK_CTRL1, 0x05));
    vlx_sleep_ms(P, 1); /* 50 us power-up, rounded up */
    VLX_TRY(vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__MODE, 0x01));
    return vlx_wr_word(P, RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB, NVM_CTRL_PULSE_WIDTH);
}

static int nvm_disable(vlx_driver *d)
{
    vlxb_system_control *sc = &bd(d)->img.system_control;
    VLX_TRY(vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__READN, 0x01));
    VLX_TRY(vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__PDN, 0x00));
    sc->power_management__go1_power_force = 0;
    VLX_TRY(vlx_wr_byte(P, POWER_MANAGEMENT__GO1_POWER_FORCE, 0));
    sc->firmware__enable = 1;
    return vlx_wr_byte(P, FIRMWARE__ENABLE, 1);
}

/* read_region(): (byte index, byte size), word-aligned; the firmware is never
 * left stopped — the disable runs even when a read fails. */
static int nvm_read_region(vlx_driver *d, unsigned index, unsigned size, uint8_t *out)
{
    unsigned addr;
    int rc = DEPZ_OK, rc2;
    VLX_TRY(nvm_enable(d));
    for (addr = index >> 2; addr < (index >> 2) + (size >> 2) && rc == DEPZ_OK; addr++) {
        rc = vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__ADDR, (uint8_t)addr);
        if (!rc) rc = vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__READN, 0x00);
        /* The 5 us trigger delay is shorter than the next USB round trip. */
        if (!rc) rc = vlx_wr_byte(P, RANGING_CORE__NVM_CTRL__READN, 0x01);
        if (!rc) rc = vlx_rd_multi(P, RANGING_CORE__NVM_CTRL__DATAOUT_MMM, out + 4 * (addr - (index >> 2)), 4);
    }
    rc2 = nvm_disable(d);
    return rc ? rc : rc2;
}

static int64_t u16be(const uint8_t *p) { return (int64_t)(p[0] << 8 | p[1]); }

/* ── SPAD geometry and the MM1/MM2 offsets (core.py) ───────────────────── */

static int64_t encode_row_col(int64_t row, int64_t col)
{
    if (row > 7) return (128 + (col << 3) + (15 - row)) & 0xFF;
    return (((15 - col) << 3) + row) & 0xFF;
}

static void decode_row_col(int64_t spad, int64_t *row, int64_t *col)
{
    if (spad > 127) {
        *row = 8 + ((255 - spad) & 0x07);
        *col = (spad - 128) >> 3;
    } else {
        *row = spad & 0x07;
        *col = (127 - spad) >> 3;
    }
}

static void decode_zone_limits(int64_t centre, int64_t size, int64_t *x_ll, int64_t *y_ll, int64_t *x_ur, int64_t *y_ur)
{
    int64_t yc, xc, w = size & 0x0F, h = size >> 4;
    decode_row_col(centre, &yc, &xc);
    /* Python floors: (w + 1) // 2 is non-negative here. */
    *x_ll = xc - (w + 1) / 2 < 0 ? 0 : xc - (w + 1) / 2;
    *x_ur = *x_ll + w > SPAD_ARRAY_WIDTH - 1 ? SPAD_ARRAY_WIDTH - 1 : *x_ll + w;
    *y_ll = yc - (h + 1) / 2 < 0 ? 0 : yc - (h + 1) / 2;
    *y_ur = *y_ll + h > SPAD_ARRAY_HEIGHT - 1 ? SPAD_ARRAY_HEIGHT - 1 : *y_ll + h;
}

static bool is_aperture_location(int64_t row, int64_t col)
{
    return (row % 4 == 0 && col % 4 == 2) || (row % 4 == 2 && col % 4 == 0);
}

static void calc_mm_effective_spads(int64_t mm_centre, int64_t mm_size, int64_t zone_centre, int64_t zone_size,
                                    const uint8_t *good, int64_t aperture, int64_t *inner, int64_t *outer)
{
    int64_t mx0, my0, mx1, my1, zx0, zy0, zx1, zy1, x, y;
    decode_zone_limits(mm_centre, mm_size, &mx0, &my0, &mx1, &my1);
    decode_zone_limits(zone_centre, zone_size, &zx0, &zy0, &zx1, &zy1);
    *inner = *outer = 0;
    for (y = zy0; y <= zy1; y++) {
        for (x = zx0; x <= zx1; x++) {
            int64_t spad = encode_row_col(y, x), att;
            if (!(good[spad >> 3] & (1 << (spad & 0x07)))) continue;
            att = is_aperture_location(y, x) ? aperture : RTN_SPAD_UNITY_TRANSMISSION;
            if (mx0 <= x && x <= mx1 && my0 <= y && y <= my1) *inner += att;
            else *outer += att;
        }
    }
}

/* VL53LX_hist_combine_mm1_mm2_offsets() -> range_offset_mm (quarter mm). */
static int64_t combine_mm1_mm2_offsets(int64_t mm1_off, int64_t mm2_off, int64_t mm_centre, int64_t mm_size,
                                       int64_t zone_centre, int64_t zone_size, const add_off_cal *cal,
                                       const uint8_t *good, int64_t aperture)
{
    int64_t max_inner, max_outer, inner, outer, mm1_rate, mm2_rate, total, num;
    calc_mm_effective_spads(mm_centre, mm_size, 0xC7, 0xFF, good, aperture, &max_inner, &max_outer);
    if (max_inner == 0 || max_outer == 0) return 0;
    calc_mm_effective_spads(mm_centre, mm_size, zone_centre, zone_size, good, aperture, &inner, &outer);
    mm1_rate = cal->inner_rate * inner / max_inner;
    mm2_rate = cal->outer_rate * outer / max_outer;
    total = mm1_rate + mm2_rate;
    if (total == 0) return 0;
    /* The offsets are signed: truncate towards zero, as C does. */
    num = (mm1_off * mm1_rate + mm2_off * mm2_rate) * 4;
    return num / total;
}

/* ── timing maths (core.py) ────────────────────────────────────────────── */

int64_t vlxb_calc_pll_period_us(int64_t f) { return f > 0 ? (1LL << 30) / f : 0; }
int64_t vlxb_decode_vcsel_period(int64_t reg) { return (reg + 1) << 1; }

static int64_t calc_macro_period_us(int64_t f, int64_t vcsel_reg)
{
    int64_t macro = MACRO_PERIOD_VCSEL_PERIODS * vlxb_calc_pll_period_us(f);
    macro >>= 6;
    macro *= vlxb_decode_vcsel_period(vcsel_reg);
    return macro >> 6;
}

static int64_t calc_timeout_mclks(int64_t timeout_us, int64_t macro_us)
{
    if (macro_us == 0) return 0;
    return ((timeout_us << 12) + (macro_us >> 1)) / macro_us;
}

static int64_t encode_timeout(int64_t mclks)
{
    int64_t ls, ms = 0;
    if (mclks <= 0) return 0;
    ls = mclks - 1;
    while (ls & 0xFFFFFF00) {
        ls >>= 1;
        ms++;
    }
    return ((ms << 8) + (ls & 0xFF)) & 0xFFFF;
}

static int64_t decode_timeout(int64_t enc)
{
    int64_t sh = (enc & 0xFF00) >> 8;
    return ((enc & 0x00FF) << (sh > 40 ? 40 : sh)) + 1; /* sh stays small: encode_timeout wrote it */
}

int64_t vlxb_duration_maths(int64_t pll_us, int64_t vcsel_pclks, int64_t window_vclks, int64_t elapsed_mclks)
{
    int64_t duration = (window_vclks * pll_us) >> 12;
    duration *= (elapsed_mclks * vcsel_pclks) >> 4;
    duration >>= 12;
    return duration > 0xFFFFFFFFLL ? 0xFFFFFFFFLL : duration;
}

static void calc_timeout_register_values(vlx_driver *d, int64_t phasecal_us, int64_t mm_us, int64_t range_us)
{
    bare_drv *s = bd(d);
    vlxb_general_config *gen = &s->img.general_config;
    vlxb_timing_config *tim = &s->img.timing_config;
    int64_t f = s->img.static_nvm_managed.osc_measured__fast_osc__frequency, macro, enc;
    macro = calc_macro_period_us(f, tim->range_config__vcsel_period_a);
    enc = calc_timeout_mclks(phasecal_us, macro);
    gen->phasecal_config__timeout_macrop = enc < 0xFF ? enc : 0xFF;
    enc = encode_timeout(calc_timeout_mclks(mm_us, macro));
    tim->mm_config__timeout_macrop_a_hi = enc >> 8;
    tim->mm_config__timeout_macrop_a_lo = enc & 0xFF;
    enc = encode_timeout(calc_timeout_mclks(range_us, macro));
    tim->range_config__timeout_macrop_a_hi = enc >> 8;
    tim->range_config__timeout_macrop_a_lo = enc & 0xFF;
    macro = calc_macro_period_us(f, tim->range_config__vcsel_period_b);
    enc = encode_timeout(calc_timeout_mclks(mm_us, macro));
    tim->mm_config__timeout_macrop_b_hi = enc >> 8;
    tim->mm_config__timeout_macrop_b_lo = enc & 0xFF;
    enc = encode_timeout(calc_timeout_mclks(range_us, macro));
    tim->range_config__timeout_macrop_b_hi = enc >> 8;
    tim->range_config__timeout_macrop_b_lo = enc & 0xFF;
}

/* ── the histogram config (core.py HistConfig) ─────────────────────────── */

static void hcfg_set_bin_sequence(hist_cfg *h, const int *even, const int *odd)
{
    h->low_even_0_1 = (even[1] << 4) + even[0];
    h->low_even_2_3 = (even[3] << 4) + even[2];
    h->low_even_4_5 = (even[5] << 4) + even[4];
    h->low_odd_0_1 = (odd[1] << 4) + odd[0];
    h->low_odd_2_3 = (odd[3] << 4) + odd[2];
    h->low_odd_4_5 = (odd[5] << 4) + odd[4];
    h->mid_even_0_1 = h->low_even_0_1;
    h->mid_even_2_3 = h->low_even_2_3;
    h->mid_even_4_5 = h->low_even_4_5;
    h->mid_odd_0_1 = h->low_odd_0_1;
    h->mid_odd_2 = odd[2];
    h->mid_odd_3_4 = (odd[4] << 4) + odd[3];
    h->mid_odd_5 = odd[5];
    h->user_bin_offset = 0x00;
    h->high_even_0_1 = h->low_even_0_1;
    h->high_even_2_3 = h->low_even_2_3;
    h->high_even_4_5 = h->low_even_4_5;
    h->high_odd_0_1 = h->low_odd_0_1;
    h->high_odd_2_3 = h->low_odd_2_3;
    h->high_odd_4_5 = h->low_odd_4_5;
    h->amb_thresh_low = 0xFFFF;
    h->amb_thresh_high = 0xFFFF;
    h->spad_array_selection = 0x00;
}

/* VL53LX_copy_hist_cfg_to_static_cfg(): lite-mode field names, read by the
 * firmware as the bin sequence in histogram mode. */
static void hcfg_copy_to_static(const hist_cfg *h, vlxb_image *img)
{
    vlxb_static_config *st = &img->static_config;
    vlxb_timing_config *tm = &img->timing_config;
    vlxb_dynamic_config *dy = &img->dynamic_config;
    st->sigma_estimator__effective_pulse_width_ns = h->high_even_0_1;
    st->sigma_estimator__effective_ambient_width_ns = h->high_even_2_3;
    st->sigma_estimator__sigma_ref_mm = h->high_even_4_5;
    st->algo__crosstalk_compensation_valid_height_mm = h->high_odd_0_1;
    st->spare_host_config__static_config_spare_0 = h->high_odd_2_3;
    st->spare_host_config__static_config_spare_1 = h->high_odd_4_5;
    st->algo__range_ignore_threshold_mcps = (h->mid_even_0_1 << 8) + h->mid_even_2_3;
    st->algo__range_ignore_valid_height_mm = h->mid_even_4_5;
    st->algo__range_min_clip = h->mid_odd_0_1;
    st->algo__consistency_check__tolerance = h->mid_odd_2;
    st->spare_host_config__static_config_spare_2 = h->mid_odd_3_4;
    st->sd_config__reset_stages_msb = h->mid_odd_5;
    st->sd_config__reset_stages_lsb = h->user_bin_offset;
    tm->range_config__sigma_thresh = (h->low_even_0_1 << 8) + h->low_even_2_3;
    tm->range_config__min_count_rate_rtn_limit_mcps = (h->low_even_4_5 << 8) + h->low_odd_0_1;
    tm->range_config__valid_phase_low = h->low_odd_2_3;
    tm->range_config__valid_phase_high = h->low_odd_4_5;
    dy->system__thresh_high = h->amb_thresh_low;
    dy->system__thresh_low = h->amb_thresh_high;
    dy->system__enable_xtalk_per_quadrant = h->spad_array_selection;
}

/* VL53LX_hist_get_bin_sequence_config(). */
static void hcfg_bin_sequence(const hist_cfg *h, int64_t stream_count, int64_t ambient_events_sum, int64_t *seq)
{
    int level = 1; /* 0 low, 1 mid, 2 high */
    int64_t packed[3];
    int k;
    if (ambient_events_sum > 1024 * h->amb_thresh_high) level = 2;
    if (ambient_events_sum < 1024 * h->amb_thresh_low) level = 0;
    if ((stream_count & 0x01) == 0) {
        packed[0] = level == 0 ? h->low_even_0_1 : level == 1 ? h->mid_even_0_1 : h->high_even_0_1;
        packed[1] = level == 0 ? h->low_even_2_3 : level == 1 ? h->mid_even_2_3 : h->high_even_2_3;
        packed[2] = level == 0 ? h->low_even_4_5 : level == 1 ? h->mid_even_4_5 : h->high_even_4_5;
    } else if (level == 1) {
        /* The mid odd sequence is the one packed differently. */
        seq[0] = h->mid_odd_0_1 & 0x0F;
        seq[1] = h->mid_odd_0_1 >> 4;
        seq[2] = h->mid_odd_2;
        seq[3] = h->mid_odd_3_4 >> 4;
        seq[4] = h->mid_odd_3_4 & 0x0F;
        seq[5] = h->mid_odd_5 & 0x0F;
        return;
    } else {
        packed[0] = level == 0 ? h->low_odd_0_1 : h->high_odd_0_1;
        packed[1] = level == 0 ? h->low_odd_2_3 : h->high_odd_2_3;
        packed[2] = level == 0 ? h->low_odd_4_5 : h->high_odd_4_5;
    }
    for (k = 0; k < 3; k++) {
        seq[2 * k] = packed[k] & 0x0F;
        seq[2 * k + 1] = packed[k] >> 4;
    }
}

/* VL53LX_init_hist_post_process_config_struct(). */
static void hpp_init(vlxb_hpp *h)
{
    memset(h, 0, sizeof *h);
    h->hist_algo_select = VLXB_TUNINGPARM_HIST_ALGO_SELECT_DEFAULT;
    h->hist_target_order = VLXB_TUNINGPARM_HIST_TARGET_ORDER_DEFAULT;
    h->filter_woi0 = VLXB_TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT;
    h->filter_woi1 = VLXB_TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT;
    h->hist_amb_est_method = VLXB_TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT;
    h->ambient_thresh_sigma0 = VLXB_TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT;
    h->ambient_thresh_sigma1 = VLXB_TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT;
    h->ambient_thresh_events_scaler = VLXB_TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT;
    h->min_ambient_thresh_events = VLXB_TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT;
    h->noise_threshold = VLXB_TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT;
    h->signal_total_events_limit = VLXB_TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT;
    h->sigma_estimator_sigma_ref_mm = VLXB_TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT;
    h->sigma_thresh = VLXB_TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT;
    h->range_offset_mm = 0;
    h->gain_factor = VLXB_TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT;
    h->valid_phase_low = 0x08;
    h->valid_phase_high = 0x88;
    h->phase_tolerance = VLXB_TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT;
    h->event_sigma = VLXB_TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT;
    h->event_min_spad_count = VLXB_TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT;
    h->min_max_tolerance = VLXB_TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT;
}

/* ── the ll-driver state machine (core.py LLState) ─────────────────────── */

static void ll_reset(ll_state *s)
{
    s->cfg_device_state = VLXB_STATE_SW_STANDBY;
    s->cfg_stream_count = 0;
    s->cfg_gph_id = GPH_ID_MASK;
    s->cfg_timing_status = 0;
    s->rd_device_state = VLXB_STATE_SW_STANDBY;
    s->rd_stream_count = 0;
    s->rd_gph_id = GPH_ID_MASK;
    s->rd_timing_status = 0;
}

static void ll_update_rd(ll_state *s, int64_t mode_start, int64_t gph)
{
    if ((mode_start & MEASUREMENTMODE_MODE_MASK) == 0) {
        s->rd_device_state = VLXB_STATE_SW_STANDBY;
        s->rd_stream_count = 0;
        s->rd_gph_id = GPH_ID_MASK;
        s->rd_timing_status = 0;
        return;
    }
    s->rd_stream_count = s->rd_stream_count == 0xFF ? 0x80 : s->rd_stream_count + 1;
    s->rd_gph_id ^= GPH_ID_MASK;
    if (s->rd_device_state == VLXB_STATE_SW_STANDBY) {
        s->rd_device_state = (gph & GPH_ID_MASK) ? VLXB_STATE_WAIT_GPH_SYNC : VLXB_STATE_OUTPUT_DATA;
        s->rd_stream_count = 0;
        s->rd_timing_status = 0;
    } else if (s->rd_device_state == VLXB_STATE_WAIT_GPH_SYNC) {
        s->rd_stream_count = 0;
        s->rd_device_state = VLXB_STATE_OUTPUT_DATA;
    } else if (s->rd_device_state == VLXB_STATE_OUTPUT_DATA) {
        s->rd_timing_status ^= 0x01;
    } else {
        ll_reset(s);
    }
}

static void ll_update_cfg(ll_state *s, int64_t mode_start)
{
    if ((mode_start & MEASUREMENTMODE_MODE_MASK) == 0) {
        s->cfg_device_state = VLXB_STATE_SW_STANDBY;
        s->cfg_stream_count = 0;
        s->cfg_gph_id = GPH_ID_MASK;
        s->cfg_timing_status = 0;
        return;
    }
    s->cfg_stream_count = s->cfg_stream_count == 0xFF ? 0x80 : s->cfg_stream_count + 1;
    s->cfg_gph_id ^= GPH_ID_MASK;
    if (s->cfg_device_state == VLXB_STATE_SW_STANDBY) {
        s->cfg_timing_status ^= 0x01;
        s->cfg_stream_count = 1;
        s->cfg_device_state = VLXB_STATE_DSS_AUTO;
    } else if (s->cfg_device_state == VLXB_STATE_DSS_AUTO) {
        s->cfg_timing_status ^= 0x01;
    } else {
        ll_reset(s);
    }
}

/* ── VL53LX_read_p2p_data ──────────────────────────────────────────────── */

#define RTN(n) offsetof(vlxb_nvm_copy_data, global_config__spad_enables_rtn_##n)
static const size_t RTN_OFF[32] = {
    RTN(0), RTN(1), RTN(2), RTN(3), RTN(4), RTN(5), RTN(6), RTN(7), RTN(8), RTN(9), RTN(10),
    RTN(11), RTN(12), RTN(13), RTN(14), RTN(15), RTN(16), RTN(17), RTN(18), RTN(19), RTN(20),
    RTN(21), RTN(22), RTN(23), RTN(24), RTN(25), RTN(26), RTN(27), RTN(28), RTN(29), RTN(30), RTN(31)};
#undef RTN

static int read_p2p_data(vlx_driver *d)
{
    bare_drv *s = bd(d);
    vlxb_nvm_copy_data *nvm = &s->img.nvm_copy_data;
    vlxb_customer_nvm_managed *cust = &s->img.customer_nvm_managed;
    uint8_t buf[56];
    uint16_t osc;
    int i;
    VLX_TRY(img_pull(d, VLXB_BLK_STATIC_NVM_MANAGED));
    VLX_TRY(img_pull(d, VLXB_BLK_CUSTOMER_NVM_MANAGED));
    VLX_TRY(img_pull(d, VLXB_BLK_NVM_COPY_DATA));
    for (i = 0; i < 32; i++) s->rtn_good_spads[i] = (uint8_t)*(const int64_t *)((const char *)nvm + RTN_OFF[i]);
    s->hpp.xtalk_plane_offset_kcps = cust->algo__crosstalk_compensation_plane_offset_kcps;
    s->hpp.xtalk_x_plane_gradient_kcps = cust->algo__crosstalk_compensation_x_plane_gradient_kcps;
    s->hpp.xtalk_y_plane_gradient_kcps = cust->algo__crosstalk_compensation_y_plane_gradient_kcps;

    /* Optical centre, peak-rate map: read for the traffic's sake (the C
     * driver does); nothing on the ranging path uses them. */
    VLX_TRY(nvm_read_region(d, 0x00B8, 4, buf));
    VLX_TRY(nvm_read_region(d, 0x015C, 56, buf));
    VLX_TRY(nvm_read_region(d, 0x0194, 8, buf));
    s->add_off.inner_spads = u16be(buf + 0);
    s->add_off.outer_spads = u16be(buf + 2);
    s->add_off.inner_rate = u16be(buf + 4);
    s->add_off.outer_rate = u16be(buf + 6);
    s->have_add_off = true;
    /* Our boards have no FMT offset calibration. ST's fallback: nominal
     * MM1/MM2 peak rates, the effective SPAD counts worked out here. */
    if (s->add_off.inner_rate == 0 && s->add_off.outer_rate == 0) {
        calc_mm_effective_spads(nvm->roi_config__mode_roi_centre_spad, nvm->roi_config__mode_roi_xy_size, 0xC7,
                                0xFF, s->rtn_good_spads, RTN_SPAD_APERTURE_TRANSMISSION, &s->add_off.inner_spads,
                                &s->add_off.outer_spads);
        s->add_off.inner_rate = 0x0080;
        s->add_off.outer_rate = 0x0180;
    }
    VLX_TRY(nvm_read_region(d, 0x01AC, 16, buf)); /* FMT range results: dmax only */

    VLX_TRY(vlx_rd_word(P, RESULT__OSC_CALIBRATE_VAL, &osc));
    s->result_osc_calibrate_val = osc;
    if (s->img.static_nvm_managed.osc_measured__fast_osc__frequency < 0x1000)
        s->img.static_nvm_managed.osc_measured__fast_osc__frequency = 0xBCCC;
    return DEPZ_OK;
}

/* ── preset modes (core.py) ────────────────────────────────────────────── */

static void preset_standard_ranging(bare_drv *s)
{
    vlxb_static_config *st = &s->img.static_config;
    vlxb_general_config *ge = &s->img.general_config;
    vlxb_timing_config *tm = &s->img.timing_config;
    vlxb_dynamic_config *dy = &s->img.dynamic_config;
    vlxb_system_control *sy = &s->img.system_control;
    hist_cfg *h = &s->hcfg;

    st->dss_config__target_total_rate_mcps = 0x0A00;
    st->debug__ctrl = 0x00;
    st->test_mode__ctrl = 0x00;
    st->clk_gating__ctrl = 0x00;
    st->nvm_bist__ctrl = 0x00;
    st->nvm_bist__num_nvm_words = 0x00;
    st->nvm_bist__start_address = 0x00;
    st->host_if__status = 0x00;
    st->pad_i2c_hv__config = 0x00;
    st->pad_i2c_hv__extsup_config = 0x00;
    st->gpio_hv_pad__ctrl = 0x00;
    st->gpio_hv_mux__ctrl = INTERRUPTPOLARITY_ACTIVE_LOW | GPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS;
    st->gpio__tio_hv_status = 0x02;
    st->gpio__fio_hv_status = 0x00;
    st->ana_config__spad_sel_pswidth = 0x02;
    st->ana_config__vcsel_pulse_width_offset = 0x08;
    st->ana_config__fast_osc__config_ctrl = 0x00;
    st->sigma_estimator__effective_pulse_width_ns = VLXB_TP_LITE_SIGMA_EST_PULSE_WIDTH_NS;
    st->sigma_estimator__effective_ambient_width_ns = VLXB_TP_LITE_SIGMA_EST_AMB_WIDTH_NS;
    st->sigma_estimator__sigma_ref_mm = VLXB_TP_LITE_SIGMA_REF_MM;
    st->algo__crosstalk_compensation_valid_height_mm = 0x01;
    st->spare_host_config__static_config_spare_0 = 0x00;
    st->spare_host_config__static_config_spare_1 = 0x00;
    st->algo__range_ignore_threshold_mcps = 0x0000;
    st->algo__range_ignore_valid_height_mm = 0xFF;
    st->algo__range_min_clip = VLXB_TP_LITE_MIN_CLIP;
    st->algo__consistency_check__tolerance = VLXB_TP_CONSISTENCY_LITE_PHASE_TOLERANCE;
    st->spare_host_config__static_config_spare_2 = 0x00;
    st->sd_config__reset_stages_msb = 0x00;
    st->sd_config__reset_stages_lsb = 0x00;

    ge->gph_config__stream_count_update_value = 0x00;
    ge->global_config__stream_divider = 0x00;
    ge->system__interrupt_config_gpio = INTERRUPT_CONFIG_NEW_SAMPLE_READY;
    ge->cal_config__vcsel_start = 0x0B;
    ge->cal_config__repeat_rate = VLXB_TP_CAL_REPEAT_RATE;
    ge->global_config__vcsel_width = 0x02;
    ge->phasecal_config__timeout_macrop = 0x0D;
    ge->phasecal_config__target = VLXB_TP_PHASECAL_TARGET;
    ge->phasecal_config__override = 0x00;
    ge->dss_config__roi_mode_control = DSSMODE__TARGET_RATE;
    ge->system__thresh_rate_high = 0x0000;
    ge->system__thresh_rate_low = 0x0000;
    ge->dss_config__manual_effective_spads_select = 0x8C00;
    ge->dss_config__manual_block_select = 0x00;
    ge->dss_config__aperture_attenuation = 0x38;
    ge->dss_config__max_spads_limit = 0xFF;
    ge->dss_config__min_spads_limit = 0x01;

    tm->mm_config__timeout_macrop_a_hi = 0x00;
    tm->mm_config__timeout_macrop_a_lo = 0x1A;
    tm->mm_config__timeout_macrop_b_hi = 0x00;
    tm->mm_config__timeout_macrop_b_lo = 0x20;
    tm->range_config__timeout_macrop_a_hi = 0x01;
    tm->range_config__timeout_macrop_a_lo = 0xCC;
    tm->range_config__vcsel_period_a = 0x0B;
    tm->range_config__timeout_macrop_b_hi = 0x01;
    tm->range_config__timeout_macrop_b_lo = 0xF5;
    tm->range_config__vcsel_period_b = 0x09;
    tm->range_config__sigma_thresh = VLXB_TP_LITE_MED_SIGMA_THRESH_MM;
    tm->range_config__min_count_rate_rtn_limit_mcps = VLXB_TP_LITE_MED_MIN_COUNT_RATE_RTN_MCPS;
    tm->range_config__valid_phase_low = 0x08;
    tm->range_config__valid_phase_high = 0x78;
    tm->system__intermeasurement_period = 0x00000000;
    tm->system__fractional_enable = 0x00;

    /* The histogram config's initial state (every histogram mode overwrites it). */
    h->low_even_0_1 = 0x07; h->low_even_2_3 = 0x21; h->low_even_4_5 = 0x43;
    h->low_odd_0_1 = 0x10; h->low_odd_2_3 = 0x32; h->low_odd_4_5 = 0x54;
    h->mid_even_0_1 = 0x07; h->mid_even_2_3 = 0x21; h->mid_even_4_5 = 0x43;
    h->mid_odd_0_1 = 0x10; h->mid_odd_2 = 0x02; h->mid_odd_3_4 = 0x43; h->mid_odd_5 = 0x05;
    h->user_bin_offset = 0x00;
    h->high_even_0_1 = 0x07; h->high_even_2_3 = 0x21; h->high_even_4_5 = 0x43;
    h->high_odd_0_1 = 0x10; h->high_odd_2_3 = 0x32; h->high_odd_4_5 = 0x54;
    h->amb_thresh_low = 0xFFFF;
    h->amb_thresh_high = 0xFFFF;
    h->spad_array_selection = 0x00;

    dy->system__grouped_parameter_hold_0 = 0x01;
    dy->system__thresh_high = 0x0000;
    dy->system__thresh_low = 0x0000;
    dy->system__enable_xtalk_per_quadrant = 0x00;
    dy->system__seed_config = VLXB_TP_LITE_SEED_CFG;
    dy->sd_config__woi_sd0 = 0x0B;
    dy->sd_config__woi_sd1 = 0x09;
    dy->sd_config__initial_phase_sd0 = VLXB_TP_INIT_PHASE_RTN_LITE_MED;
    dy->sd_config__initial_phase_sd1 = VLXB_TP_INIT_PHASE_REF_LITE_MED;
    dy->system__grouped_parameter_hold_1 = 0x01;
    dy->sd_config__first_order_select = VLXB_TP_LITE_FIRST_ORDER_SELECT;
    dy->sd_config__quantifier = VLXB_TP_LITE_QUANTIFIER;
    dy->roi_config__user_roi_centre_spad = 0xC7;
    dy->roi_config__user_roi_requested_global_xy_size = 0xFF;
    dy->system__sequence_config = SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN | SEQUENCE_DSS2_EN |
                                  SEQUENCE_MM2_EN | SEQUENCE_RANGE_EN;
    dy->system__grouped_parameter_hold = 0x02;

    sy->system__stream_count_ctrl = 0x00;
    sy->firmware__enable = 0x01;
    sy->system__interrupt_clear = CLEAR_RANGE_INT;
    sy->system__mode_start = SCHEDULERMODE_STREAMING | READOUTMODE_SINGLE_SD | MEASUREMENTMODE_BACKTOBACK;
}

static void preset_histogram_ranging(bare_drv *s)
{
    static const int even[6] = {7, 0, 1, 2, 3, 4}, odd[6] = {0, 1, 2, 3, 4, 5};
    vlxb_timing_config *tm = &s->img.timing_config;
    vlxb_dynamic_config *dy = &s->img.dynamic_config;
    preset_standard_ranging(s);
    s->img.static_config.dss_config__target_total_rate_mcps = 0x1400;
    hcfg_set_bin_sequence(&s->hcfg, even, odd);
    tm->range_config__vcsel_period_a = 0x09;
    tm->range_config__vcsel_period_b = 0x0B;
    dy->sd_config__woi_sd0 = 0x09;
    dy->sd_config__woi_sd1 = 0x0B;
    tm->mm_config__timeout_macrop_a_hi = 0x00;
    tm->mm_config__timeout_macrop_a_lo = 0x20;
    tm->mm_config__timeout_macrop_b_hi = 0x00;
    tm->mm_config__timeout_macrop_b_lo = 0x1A;
    tm->range_config__timeout_macrop_a_hi = 0x00;
    tm->range_config__timeout_macrop_a_lo = 0x28;
    tm->range_config__timeout_macrop_b_hi = 0x00;
    tm->range_config__timeout_macrop_b_lo = 0x21;
    s->img.general_config.phasecal_config__timeout_macrop = 0xF5;
    s->hpp.valid_phase_low = 0x08;
    s->hpp.valid_phase_high = 0x88;
    hcfg_copy_to_static(&s->hcfg, &s->img);
    dy->system__sequence_config =
        SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN | SEQUENCE_DSS2_EN | SEQUENCE_RANGE_EN;
    s->img.system_control.system__mode_start =
        SCHEDULERMODE_HISTOGRAM | READOUTMODE_DUAL_SD | MEASUREMENTMODE_BACKTOBACK;
}

typedef struct {
    int even[6], odd[6];
    int64_t vcsel_a, vcsel_b, mm_a, mm_b, range_a, range_b, cal_vcsel_start, valid_phase_high;
    int64_t phase_rtn, phase_ref, extra_sequence;
} preset_tail;

/* The three distance-mode tails (vl53lx_api_preset_modes.c). */
static const preset_tail TAILS[3] = {
    /* short */
    {{7, 7, 0, 1, 1, 1}, {0, 1, 1, 1, 2, 2}, 0x03, 0x05, 0x0052, 0x0037, 0x0066, 0x0044, 0x03, 0x28,
     VLXB_TP_INIT_PHASE_RTN_HIST_SHORT, VLXB_TP_INIT_PHASE_REF_HIST_SHORT, SEQUENCE_MM1_EN},
    /* medium */
    {{7, 0, 1, 1, 2, 2}, {0, 1, 2, 1, 2, 3}, 0x05, 0x07, 0x0036, 0x0028, 0x0044, 0x0033, 0x05, 0x48,
     VLXB_TP_INIT_PHASE_RTN_HIST_MED, VLXB_TP_INIT_PHASE_REF_HIST_MED, 0},
    /* long */
    {{7, 0, 1, 2, 3, 4}, {0, 1, 2, 3, 4, 5}, 0x09, 0x0B, 0x0021, 0x001B, 0x0029, 0x0022, 0x09, 0x88,
     VLXB_TP_INIT_PHASE_RTN_HIST_LONG, VLXB_TP_INIT_PHASE_REF_HIST_LONG, 0},
};

static void apply_tail(bare_drv *s, const preset_tail *t)
{
    vlxb_general_config *ge = &s->img.general_config;
    vlxb_timing_config *tm = &s->img.timing_config;
    vlxb_dynamic_config *dy = &s->img.dynamic_config;
    hcfg_set_bin_sequence(&s->hcfg, t->even, t->odd);
    hcfg_copy_to_static(&s->hcfg, &s->img);
    tm->range_config__vcsel_period_a = t->vcsel_a;
    tm->range_config__vcsel_period_b = t->vcsel_b;
    tm->mm_config__timeout_macrop_a_hi = t->mm_a >> 8;
    tm->mm_config__timeout_macrop_a_lo = t->mm_a & 0xFF;
    tm->mm_config__timeout_macrop_b_hi = t->mm_b >> 8;
    tm->mm_config__timeout_macrop_b_lo = t->mm_b & 0xFF;
    tm->range_config__timeout_macrop_a_hi = t->range_a >> 8;
    tm->range_config__timeout_macrop_a_lo = t->range_a & 0xFF;
    tm->range_config__timeout_macrop_b_hi = t->range_b >> 8;
    tm->range_config__timeout_macrop_b_lo = t->range_b & 0xFF;
    ge->cal_config__vcsel_start = t->cal_vcsel_start;
    ge->phasecal_config__timeout_macrop = 0xF5;
    dy->sd_config__woi_sd0 = t->vcsel_a;
    dy->sd_config__woi_sd1 = t->vcsel_b;
    dy->sd_config__initial_phase_sd0 = t->phase_rtn;
    dy->sd_config__initial_phase_sd1 = t->phase_ref;
    dy->system__sequence_config = SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN | SEQUENCE_DSS2_EN |
                                  t->extra_sequence | SEQUENCE_RANGE_EN;
    s->hpp.valid_phase_low = 0x08;
    s->hpp.valid_phase_high = t->valid_phase_high;
    s->img.system_control.system__mode_start =
        SCHEDULERMODE_HISTOGRAM | READOUTMODE_DUAL_SD | MEASUREMENTMODE_BACKTOBACK;
}

static void set_timeouts_us(vlx_driver *d, int64_t phasecal_us, int64_t mm_us, int64_t range_us)
{
    bare_drv *s = bd(d);
    s->phasecal_timeout_us = phasecal_us;
    s->mm_timeout_us = mm_us;
    s->range_timeout_us = range_us;
    calc_timeout_register_values(d, phasecal_us, mm_us, range_us);
}

static int set_inter_measurement_period_ms(vlx_driver *d, int64_t period_ms)
{
    bare_drv *s = bd(d);
    if (s->result_osc_calibrate_val == 0) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: result__osc_calibrate_val is 0");
    s->inter_measurement_period_ms = period_ms;
    s->img.timing_config.system__intermeasurement_period = period_ms * s->result_osc_calibrate_val;
    return DEPZ_OK;
}

/* `IsL4()` of the L4CX BareDriver: the die, read from NVM, not the product
 * name — an L4CD or L4CX board ranging under a borrowed name is still an L4.
 * 0xEC is the L4ED. False until data_init() has read the NVM. */
static bool is_l4(const bare_drv *s)
{
    return s->img.nvm_copy_data.identification__module_type == 0xAA &&
           (s->img.nvm_copy_data.identification__model_id == 0xEB ||
            s->img.nvm_copy_data.identification__model_id == 0xEC);
}

static int64_t fda_max_timing_budget_us(const bare_drv *s)
{
    return is_l4(s) ? L4_FDA_MAX_TIMING_BUDGET_US : FDA_MAX_TIMING_BUDGET_US;
}

static int set_measurement_timing_budget_us(vlx_driver *d, int64_t budget_us)
{
    bare_drv *s = bd(d);
    int64_t range_us;
    if (!(TIMING_GUARD_US < budget_us && budget_us <= 10000000))
        return depz_fail(DEPZ_E_ARG, "vl53lx: timing budget %lld us out of range", (long long)budget_us);
    range_us = (budget_us - TIMING_GUARD_US) / TIMING_DIVISOR;
    if (range_us * TIMING_DIVISOR > fda_max_timing_budget_us(s))
        return depz_fail(DEPZ_E_ARG, "vl53lx: timing budget %lld us out of range", (long long)budget_us);
    set_timeouts_us(d, s->phasecal_timeout_us, s->mm_timeout_us, range_us);
    return DEPZ_OK;
}

/* _update_range_offset(): once per preset — its inputs are the factory data
 * and the user ROI. */
static void update_range_offset(bare_drv *s)
{
    if (!s->have_add_off) return;
    s->hpp.range_offset_mm = combine_mm1_mm2_offsets(
        s->img.customer_nvm_managed.mm_config__inner_offset_mm, s->img.customer_nvm_managed.mm_config__outer_offset_mm,
        s->img.nvm_copy_data.roi_config__mode_roi_centre_spad, s->img.nvm_copy_data.roi_config__mode_roi_xy_size,
        s->img.dynamic_config.roi_config__user_roi_centre_spad,
        s->img.dynamic_config.roi_config__user_roi_requested_global_xy_size, &s->add_off, s->rtn_good_spads,
        s->img.general_config.dss_config__aperture_attenuation);
}

/* VL53LX_set_preset_mode(); dss target: the tuning default, inter: the
 * current period unless given. */
static int set_preset_mode(vlx_driver *d, dist_mode mode, int64_t inter_ms)
{
    static const int64_t PHASECAL_US[3] = {VLXB_TP_PHASECAL_TIMEOUT_HIST_SHORT_US, VLXB_TP_PHASECAL_TIMEOUT_HIST_MED_US,
                                           VLXB_TP_PHASECAL_TIMEOUT_HIST_LONG_US};
    bare_drv *s = bd(d);
    ll_reset(&s->state);
    preset_histogram_ranging(s);
    apply_tail(s, &TAILS[mode]);
    s->img.static_config.dss_config__target_total_rate_mcps = VLXB_TP_DSS_TARGET_HISTO_MCPS;
    set_timeouts_us(d, PHASECAL_US[mode], VLXB_TP_MM_TIMEOUT_HISTO_US, VLXB_TP_RANGE_TIMEOUT_HISTO_US);
    VLX_TRY(set_inter_measurement_period_ms(d, inter_ms));
    update_range_offset(s);
    return DEPZ_OK;
}

/* VL53LX_SetDistanceMode(): the preset, then the timeouts put back. */
static int set_distance_mode(vlx_driver *d, dist_mode mode)
{
    bare_drv *s = bd(d);
    int64_t ph = s->phasecal_timeout_us, mm = s->mm_timeout_us, rg = s->range_timeout_us;
    VLX_TRY(set_preset_mode(d, mode, s->inter_measurement_period_ms));
    set_timeouts_us(d, ph, mm, rg);
    return DEPZ_OK;
}

static int data_init(vlx_driver *d)
{
    bare_drv *s = bd(d);
    ll_reset(&s->state);
    hpp_init(&s->hpp);
    VLX_TRY(read_p2p_data(d));
    VLX_TRY(set_preset_mode(d, MODE_MEDIUM, 1000));
    return set_measurement_timing_budget_us(d, 33333);
}

/* ── the firmware patch and the ranging cycle ──────────────────────────── */

static int load_patch(vlx_driver *d)
{
    static const uint8_t addr[6] = {0x03, 0x6D, 0x03, 0x6F, 0x07, 0x29};
    static const uint8_t en[2] = {0x00, 0x07};
    int64_t pw = VLXB_TP_PHASECAL_PATCH_POWER;
    uint8_t offs[6] = {0x29, 0xC9, 0x0E, 0x40, 0x28, 0x00};
    offs[5] = pw == 1 ? 0x10 : pw == 2 ? 0x20 : pw == 3 ? 0x40 : 0x00;
    VLX_TRY(vlx_wr_byte(P, FIRMWARE__ENABLE, 0x00));
    VLX_TRY(vlx_wr_byte(P, POWER_MANAGEMENT__GO1_POWER_FORCE, 0x01));
    vlx_sleep_ms(P, 1);
    VLX_TRY(vlx_wr_multi(P, PATCH__OFFSET_0, offs, 6));
    VLX_TRY(vlx_wr_multi(P, PATCH__ADDRESS_0, addr, 6));
    VLX_TRY(vlx_wr_multi(P, PATCH__JMP_ENABLES, en, 2));
    VLX_TRY(vlx_wr_multi(P, PATCH__DATA_ENABLES, en, 2));
    VLX_TRY(vlx_wr_byte(P, PATCH__CTRL, 0x01));
    return vlx_wr_byte(P, FIRMWARE__ENABLE, 0x01);
}

static int unload_patch(vlx_driver *d)
{
    VLX_TRY(vlx_wr_byte(P, FIRMWARE__ENABLE, 0x00));
    VLX_TRY(vlx_wr_byte(P, POWER_MANAGEMENT__GO1_POWER_FORCE, 0x00));
    VLX_TRY(vlx_wr_byte(P, PATCH__CTRL, 0x00));
    return vlx_wr_byte(P, FIRMWARE__ENABLE, 0x01);
}

static const vlxb_block_id RANGE_START_BLOCKS[7] = {
    VLXB_BLK_STATIC_NVM_MANAGED, VLXB_BLK_CUSTOMER_NVM_MANAGED, VLXB_BLK_STATIC_CONFIG, VLXB_BLK_GENERAL_CONFIG,
    VLXB_BLK_TIMING_CONFIG, VLXB_BLK_DYNAMIC_CONFIG, VLXB_BLK_SYSTEM_CONTROL};

/* VL53LX_init_and_start_range(): all seven blocks to start, general_config
 * onwards to clear the interrupt and let the next frame run. */
static int init_and_start_range(vlx_driver *d, const vlxb_block_id *ids, size_t n)
{
    bare_drv *s = bd(d);
    vlxb_dynamic_config *dy = &s->img.dynamic_config;
    vlxb_system_control *sy = &s->img.system_control;
    int64_t gph_id;
    sy->system__mode_start = (sy->system__mode_start & MEASUREMENTMODE_STOP_MASK) | MEASUREMENTMODE_BACKTOBACK;
    sy->system__interrupt_clear = CLEAR_RANGE_INT;
    gph_id = s->state.cfg_gph_id;
    dy->system__grouped_parameter_hold_0 = gph_id | 0x01;
    dy->system__grouped_parameter_hold_1 = gph_id | 0x01;
    dy->system__grouped_parameter_hold = gph_id;
    VLX_TRY(img_push_range(d, ids, n));
    ll_update_rd(&s->state, sy->system__mode_start, dy->system__grouped_parameter_hold);
    ll_update_cfg(&s->state, sy->system__mode_start);
    return DEPZ_OK;
}

static int stop_range(vlx_driver *d)
{
    bare_drv *s = bd(d);
    vlxb_system_control *sy = &s->img.system_control;
    sy->system__mode_start = (sy->system__mode_start & MEASUREMENTMODE_STOP_MASK) | MEASUREMENTMODE_ABORT;
    VLX_TRY(img_push(d, VLXB_BLK_SYSTEM_CONTROL));
    sy->system__mode_start &= MEASUREMENTMODE_STOP_MASK;
    ll_reset(&s->state);
    return DEPZ_OK;
}

/* VL53LX_get_histogram_bin_data(): 83 bytes -> bins + what the
 * post-processing needs to read them. */
static void get_histogram_bin_data(bare_drv *s, const uint8_t *raw, vlxb_bins *b)
{
    uint8_t buf[HIST_BLOCK_LEN];
    const int off = HIST_BLOCK_ADDR, base = RESULT__HISTOGRAM_BIN_0_2 - HIST_BLOCK_ADDR;
    int64_t period, phase, encoded;
    int i;
    memcpy(buf, raw, HIST_BLOCK_LEN);
    memset(b, 0, sizeof *b);
    b->number_of_bins = VLXB_BUFFER_SIZE;
    b->bins_in_data = VLXB_BUFFER_SIZE;
    b->interrupt_status = buf[0];
    b->range_status = buf[1];
    b->report_status = buf[2];
    b->stream_count = buf[3];
    b->dss_actual_effective_spads = u16be(buf + 4);
    b->reference_phase = u16be(buf + (PHASECAL_RESULT__REFERENCE_PHASE - off));
    b->vcsel_start = buf[PHASECAL_RESULT__VCSEL_START - off];
    /* Bin 23's low byte is carried in a separate MSB/LSB pair. */
    buf[RESULT__HISTOGRAM_BIN_23_0 - off] =
        (uint8_t)((buf[RESULT__HISTOGRAM_BIN_23_0_MSB - off] << 2) + buf[RESULT__HISTOGRAM_BIN_23_0_LSB - off]);
    for (i = 0; i < VLXB_BUFFER_SIZE; i++)
        b->bin_data[i] = (int64_t)buf[base + 3 * i] << 16 | buf[base + 3 * i + 1] << 8 | buf[base + 3 * i + 2];

    b->cal_config_vcsel_start = s->img.general_config.cal_config__vcsel_start;
    b->vcsel_width = (s->img.general_config.global_config__vcsel_width << 4) +
                     s->img.static_config.ana_config__vcsel_pulse_width_offset;
    b->fast_osc_frequency = s->img.static_nvm_managed.osc_measured__fast_osc__frequency;
    b->roi_centre_spad = s->img.dynamic_config.roi_config__user_roi_centre_spad;
    b->roi_xy_size = s->img.dynamic_config.roi_config__user_roi_requested_global_xy_size;
    b->zone_id = 0;
    hcfg_bin_sequence(&s->hcfg, s->state.rd_stream_count, s->ambient_events_sum, b->bin_seq);

    if (s->state.rd_timing_status == 0) {
        encoded = (s->img.timing_config.range_config__timeout_macrop_a_hi << 8) +
                  s->img.timing_config.range_config__timeout_macrop_a_lo;
        b->vcsel_period = s->img.timing_config.range_config__vcsel_period_a;
    } else {
        encoded = (s->img.timing_config.range_config__timeout_macrop_b_hi << 8) +
                  s->img.timing_config.range_config__timeout_macrop_b_lo;
        b->vcsel_period = s->img.timing_config.range_config__vcsel_period_b;
    }
    for (i = 0; i < VLXB_MAX_BIN_SEQ_LENGTH; i++)
        if ((b->bin_seq[i] & 0x07) == 0x07) b->number_of_ambient_bins += 4;
    b->total_periods_elapsed = decode_timeout(encoded);
    b->peak_duration_us = vlxb_duration_maths(vlxb_calc_pll_period_us(b->fast_osc_frequency), b->vcsel_width,
                                              VLXB_RANGING_WINDOW_VCSEL_PERIODS, b->total_periods_elapsed + 1);
    b->woi_duration_us = 0;

    period = 2048 * vlxb_decode_vcsel_period(b->vcsel_period);
    phase = period + b->reference_phase + 2048 * b->vcsel_start - 2048 * b->cal_config_vcsel_start;
    if (period) {
        b->zero_distance_phase = phase % period;
        if (b->zero_distance_phase < 0) b->zero_distance_phase += period; /* Python's % */
    }
    if (b->number_of_ambient_bins > 0) {
        b->number_of_ambient_samples = b->number_of_ambient_bins;
        for (i = 0; i < b->number_of_ambient_bins; i++) b->ambient_events_sum += b->bin_data[i];
        b->ambient_per_bin = (b->ambient_events_sum + b->number_of_ambient_bins / 2) / b->number_of_ambient_bins;
    }
    s->ambient_events_sum = b->ambient_events_sum;
}

/* ── the SensorDriver (driver.py VL53LX) ───────────────────────────────── */

static int model_id(vlx_driver *d, uint16_t *out) { return vlx_rd_word(P, IDENTIFICATION__MODEL_ID, out); }

static int wait_boot(vlx_driver *d)
{
    uint64_t deadline = vlx_deadline_ms(1000);
    for (;;) {
        uint8_t v;
        VLX_TRY(vlx_rd_byte(P, FIRMWARE__SYSTEM_STATUS, &v));
        if (v == 0x03) return DEPZ_OK;
        if (vlx_past(deadline)) return depz_fail(DEPZ_E_TIMEOUT, "vl53lx: timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
        vlx_sleep_ms(P, 1);
    }
}

/* The budget range of this die: TIMING_GUARD_US comes off the budget before
 * it is divided, and the range timeout is capped at the FDA maximum — 2..550
 * ms, 2..200 on an L4 die. Rounded inwards. */
static void budget_range(const vlx_driver *d, int *min_ms, int *max_ms)
{
    *min_ms = 2;
    *max_ms = (int)(fda_max_timing_budget_us((const bare_drv *)d) / 1000);
}

static int set_range_timing(vlx_driver *d, int budget_ms, int inter_ms)
{
    bare_drv *s = bd(d);
    int low, high;
    budget_range(d, &low, &high);
    if (!(low <= budget_ms && budget_ms <= high))
        return depz_fail(DEPZ_E_ARG, "vl53lx: timing_budget_ms must be %d..%d", low, high);
    s->budget_ms = budget_ms;
    s->inter_ms = inter_ms;
    VLX_TRY(set_measurement_timing_budget_us(d, (int64_t)budget_ms * 1000));
    /* 0 means continuous: back-to-back is a period of zero. */
    return set_inter_measurement_period_ms(d, inter_ms);
}

static int get_range_timing(vlx_driver *d, int *budget_ms, int *inter_ms)
{
    bare_drv *s = bd(d);
    *budget_ms = (int)((s->range_timeout_us * TIMING_DIVISOR + TIMING_GUARD_US) / 1000);
    *inter_ms = (int)s->inter_measurement_period_ms;
    return DEPZ_OK;
}

static int apply_mode(vlx_driver *d, dist_mode mode)
{
    bare_drv *s = bd(d);
    s->mode = mode;
    VLX_TRY(set_distance_mode(d, mode));
    /* The preset rewrote the pad byte: put FM+ back, or the first 135-byte
     * start write would switch the pad off under a 1 MHz bus. */
    s->img.static_config.pad_i2c_hv__config = FMP_PAD_CONFIG;
    return set_range_timing(d, s->budget_ms, s->inter_ms);
}

/* Short is refused on an L4 die, as the L4CX BareDriver does: there the A
 * frame of the short pair ranges on the wrong side of the wrap, one frame in
 * two, measured on the L4CX board. */
static int set_mode(vlx_driver *d, const char *name)
{
    int m;
    for (m = is_l4(bd(d)) ? MODE_MEDIUM : MODE_SHORT; m < 3; m++)
        if (!strcmp(name, MODE_NAMES[m])) return apply_mode(d, (dist_mode)m);
    return depz_fail(DEPZ_E_ARG, "vl53lx: no such mode: %s (have %s)", name,
                     is_l4(bd(d)) ? "medium, long" : "short, medium, long");
}

static int get_mode(vlx_driver *d, const char **name)
{
    *name = MODE_NAMES[bd(d)->mode];
    return DEPZ_OK;
}

static size_t modes(const vlx_driver *d, const char **names, size_t cap)
{
    size_t first = is_l4((const bare_drv *)d) ? MODE_MEDIUM : MODE_SHORT, i;
    for (i = 0; first + i < 3 && i < cap; i++) names[i] = MODE_NAMES[first + i];
    return 3 - first;
}

/* The valid phase window of the preset mode, which the post-processing
 * applies: 992 / 1785 / 3373 mm for short / medium / long at the usual
 * oscillator. Past it the target is status 4, although the bins only wrap
 * further out. Host-side: no register traffic. */
static int reach_mm(vlx_driver *d, uint32_t *out)
{
    bare_drv *s = bd(d);
    *out = vlx_phase_window_mm((uint32_t)s->img.static_nvm_managed.osc_measured__fast_osc__frequency,
                               (uint32_t)s->hpp.valid_phase_high);
    return DEPZ_OK;
}

/* A reset, not a boot check: the register image assumes the reset defaults. */
static int sensor_init(vlx_driver *d)
{
    VLX_TRY(vlx_set_addr_width(P, 2));
    VLX_TRY(vlx_set_i2c_speed(P, VLX_I2C_KHZ_BOOT));
    VLX_TRY(vlx_xshut_reset(P));
    VLX_TRY(wait_boot(d));
    VLX_TRY(data_init(d));
    VLX_TRY(apply_mode(d, bd(d)->mode));
    VLX_TRY(vlx_wr_byte(P, PAD_I2C_HV__CONFIG, FMP_PAD_CONFIG));
    return vlx_set_i2c_speed(P, 1000);
}

static int start_ranging(vlx_driver *d)
{
    bare_drv *s = bd(d);
    static const vlxb_block_id NEXT[4] = {VLXB_BLK_GENERAL_CONFIG, VLXB_BLK_TIMING_CONFIG, VLXB_BLK_DYNAMIC_CONFIG,
                                          VLXB_BLK_SYSTEM_CONTROL};
    s->have_last_stream_count = false;
    vlxb_history_reset(&s->history);
    VLX_TRY(load_patch(d));
    VLX_TRY(init_and_start_range(d, RANGE_START_BLOCKS, 7));
    /* The interrupt line comes out of reset asserted: one clear deasserts it
     * (ST's ClearInterruptAndStartMeasurement). */
    return init_and_start_range(d, NEXT, 4);
}

static int stop_ranging(vlx_driver *d)
{
    VLX_TRY(stop_range(d));
    return unload_patch(d);
}

static int check_for_data_ready(vlx_driver *d, bool *ready)
{
    uint8_t mux, tio;
    int pol;
    VLX_TRY(vlx_rd_byte(P, GPIO_HV_MUX__CTRL, &mux));
    pol = ((mux & 0x10) >> 4) == 1 ? 0 : 1;
    VLX_TRY(vlx_rd_byte(P, GPIO__TIO_HV_STATUS, &tio));
    *ready = (tio & 1) == pol;
    return DEPZ_OK;
}

static int clear_interrupt(vlx_driver *d) { return vlx_wr_byte(P, SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT); }

static void stream_block(vlx_driver *d, uint16_t *addr, uint16_t *len)
{
    (void)d;
    *addr = HIST_BLOCK_ADDR;
    *len = HIST_BLOCK_LEN;
}

static void to_public_bins(const vlxb_bins *b, depz_vl53lx_bins *o)
{
    int i;
    memset(o, 0, sizeof *o);
    o->interrupt_status = (uint8_t)b->interrupt_status;
    o->range_status = (uint8_t)b->range_status;
    o->report_status = (uint8_t)b->report_status;
    o->stream_count = (uint8_t)b->stream_count;
    o->dss_actual_effective_spads = (uint32_t)b->dss_actual_effective_spads;
    o->reference_phase = (uint16_t)b->reference_phase;
    o->vcsel_start = (uint8_t)b->vcsel_start;
    for (i = 0; i < VLXB_BUFFER_SIZE; i++) o->bin_data[i] = (int32_t)b->bin_data[i];
    o->zone_id = (uint8_t)b->zone_id;
    o->first_bin = (uint8_t)b->first_bin;
    o->number_of_bins = (uint8_t)b->number_of_bins;
    o->bins_in_data = (uint8_t)b->bins_in_data;
    o->cal_config_vcsel_start = (uint8_t)b->cal_config_vcsel_start;
    o->vcsel_width = (uint8_t)b->vcsel_width;
    o->fast_osc_frequency = (uint16_t)b->fast_osc_frequency;
    o->vcsel_period = (uint8_t)b->vcsel_period;
    for (i = 0; i < 6; i++) {
        o->bin_seq[i] = (uint8_t)b->bin_seq[i];
        o->bin_rep[i] = (uint8_t)b->bin_rep[i];
    }
    o->min_bin_value = (int32_t)b->min_bin_value;
    o->max_bin_value = (int32_t)b->max_bin_value;
    o->number_of_ambient_bins = (uint8_t)b->number_of_ambient_bins;
    o->number_of_ambient_samples = (uint16_t)b->number_of_ambient_samples;
    o->ambient_events_sum = (int32_t)b->ambient_events_sum;
    o->ambient_per_bin = (int32_t)b->ambient_per_bin;
    o->total_periods_elapsed = (uint32_t)b->total_periods_elapsed;
    o->peak_duration_us = (uint32_t)b->peak_duration_us;
    o->woi_duration_us = (uint32_t)b->woi_duration_us;
    o->zero_distance_phase = (int32_t)b->zero_distance_phase; /* up to 49151 in long mode */
    o->roi_centre_spad = (uint8_t)b->roi_centre_spad;
    o->roi_xy_size = (uint8_t)b->roi_xy_size;
}

static void to_target(const vlxb_range *r, depz_vl53lx_target *t)
{
    int status = vlxb_convert_status(r->range_status);
    t->distance_mm = (int32_t)r->median_range_mm;
    t->status = status;
    t->status_text = vlxb_status_text(status);
    t->signal_kcps = (double)(r->peak_signal_count_rate_mcps * 1000 / 128);
    t->ambient_kcps = (double)(r->ambient_count_rate_mcps * 1000 / 128);
    t->sigma_mm = (double)(r->sigma >> 7);
    t->min_range_mm = (int32_t)r->min_range_mm;
    t->max_range_mm = (int32_t)r->max_range_mm;
}

/* bin_data() + to_measurement(): the A/B state steps once per new device
 * stream count (a frame decoded twice must not step it twice), then the
 * post-processing and the history run once. */
static int decode(vlx_driver *d, const uint8_t *raw, size_t len, depz_vl53lx_measurement *m)
{
    bare_drv *s = bd(d);
    uint8_t buf[HIST_BLOCK_LEN];
    vlxb_bins bins;
    vlxb_results *res;
    size_t i;
    if (!raw) {
        VLX_TRY(vlx_rd_multi(P, HIST_BLOCK_ADDR, buf, HIST_BLOCK_LEN));
        raw = buf;
        len = HIST_BLOCK_LEN;
    }
    if (len < HIST_BLOCK_LEN) return depz_fail(DEPZ_E_PROTOCOL, "vl53lx: histogram frame is %zu bytes, need 83", len);
    if (!s->have_last_stream_count || raw[3] != s->last_stream_count) {
        if (s->have_last_stream_count)
            ll_update_rd(&s->state, s->img.system_control.system__mode_start,
                         s->img.dynamic_config.system__grouped_parameter_hold);
        s->last_stream_count = raw[3];
        s->have_last_stream_count = true;
    }
    get_histogram_bin_data(s, raw, &bins);
    res = (vlxb_results *)malloc(sizeof *res);
    if (!res) return depz_fail(DEPZ_E_NOMEM, "vl53lx: out of memory");
    vlxb_process_data(&bins, &s->hpp, res);
    vlxb_history_apply(&s->history, res, &bins, s->state.rd_device_state, &s->hpp);

    m->has_bins = true;
    to_public_bins(&bins, &m->bins);
    m->stream_count = (int32_t)res->stream_count;
    if (!res->n_targets) {
        m->distance_mm = 8191;
        m->status = VLXB_RANGESTATUS_NONE;
        m->status_text = vlxb_status_text(VLXB_RANGESTATUS_NONE);
        m->spads = (double)(bins.dss_actual_effective_spads >> 8);
    } else {
        const vlxb_range *first = &res->targets[0];
        m->n_targets = res->n_targets;
        for (i = 0; i < res->n_targets; i++) to_target(&res->targets[i], &m->targets[i]);
        m->distance_mm = m->targets[0].distance_mm;
        m->status = m->targets[0].status;
        m->status_text = m->targets[0].status_text;
        m->signal_kcps = m->targets[0].signal_kcps;
        m->ambient_kcps = m->targets[0].ambient_kcps;
        m->sigma_mm = m->targets[0].sigma_mm;
        /* The device counts effective SPADs in 1/256. */
        m->spads = (double)(first->spads >> 8);
        m->min_range_mm = (int32_t)first->min_range_mm;
        m->max_range_mm = (int32_t)first->max_range_mm;
        m->peak_bin = (int32_t)first->peak_bin;
    }
    free(res);
    return DEPZ_OK;
}

static void destroy(vlx_driver *d) { free(d); }

static const vlx_ops BARE_OPS = {
    "VL53LX", 2, 1, {{SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT}, {0, 0}}, 1000,
    DEPZ_VL53LX_CAP_MODE | DEPZ_VL53LX_CAP_TIMING, true, 2, 550,
    model_id, sensor_init, start_ranging, stop_ranging, check_for_data_ready, clear_interrupt, stream_block,
    decode, set_range_timing, get_range_timing, destroy,
    modes, NULL, set_mode, get_mode, reach_mm,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    budget_range};

vlx_driver *vlx_new_bare(vlx_plat *p, depz_vl53lx_product product)
{
    bare_drv *s = (bare_drv *)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->base.ops = &BARE_OPS;
    s->base.p = p;
    s->base.product = product;
    /* BareDriver.__init__ and VL53LX.__init__ */
    ll_reset(&s->state);
    hpp_init(&s->hpp);
    s->phasecal_timeout_us = 1000;
    s->mm_timeout_us = 2000;
    s->range_timeout_us = 13000;
    s->inter_measurement_period_ms = 100;
    s->mode = MODE_MEDIUM;
    s->budget_ms = 33;
    s->inter_ms = 0;
    vlxb_history_reset(&s->history);
    return &s->base;
}
