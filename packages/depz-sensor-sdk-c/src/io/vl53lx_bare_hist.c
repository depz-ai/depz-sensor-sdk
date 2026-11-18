/* vl53lx_bare_hist.c — histogram post-processing: 24 bins -> targets, and the
 * frame-to-frame consistency checks (the Python SDK's vl53lx/uld/bare/hist.py,
 * itself VL53LX_hist_process_data() & co. with crosstalk off). Integer for
 * integer: Python's `//` floors, the C driver's division truncates (cdiv), and
 * the port keeps whichever the Python has at each spot. */
#include "vl53lx_bare.h"

#include <stdlib.h>

#define MAX_ALLOWED_PHASE           0xFFFF
#define SPAD_TOTAL_COUNT_MAX        ((1LL << 29) - 1)
#define SPAD_TOTAL_COUNT_RES_THRES  (1LL << 24)
#define SPEED_OF_LIGHT_IN_AIR_DIV_8 (299704 >> 3)

#define SIGMA_INVALID 0xFFFF
#define D_003 0xFFFFFFULL
#define D_004 0xFFFFFFFFFFFFFFULL
#define D_005 0x7FFFFFFFFFULL
#define D_006 0x7FFFFFFFFFFFFFFFULL
#define D_007 0xFFFFFFFFULL

#define DEVICEERROR_NOUPDATE                     0
#define DEVICEERROR_RANGEPHASECHECK              5
#define DEVICEERROR_SIGMATHRESHOLDCHECK          6
#define DEVICEERROR_PHASECONSISTENCY             7
#define DEVICEERROR_RANGECOMPLETE                9
#define DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK  19
#define DEVICEERROR_EVENTCONSISTENCY             20
#define DEVICEERROR_RANGECOMPLETE_MERGED_PULSE   22
#define DEVICEERROR_PREV_RANGE_NO_TARGETS        23

#define HIST_TARGET_ORDER_STRONGEST_FIRST 1

/* ── C and Python arithmetic ───────────────────────────────────────────── */

/* C division: truncates towards zero. */
static int64_t cdiv(int64_t a, int64_t b) { return a / b; }

/* Python's `//` and `%`: floor. */
static int64_t fdiv(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

static int64_t fmod64(int64_t a, int64_t b)
{
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    return r;
}

static int64_t imin(int64_t a, int64_t b) { return a < b ? a : b; }
static int64_t imax(int64_t a, int64_t b) { return a > b ? a : b; }
static int64_t iabs(int64_t a) { return a < 0 ? -a : a; }

/* VL53LX_isqrt(): floor of the square root of a uint32 (Python masks the
 * argument to 32 bits first, negative values included). */
static int64_t isqrt32(int64_t num)
{
    uint64_t x = (uint64_t)num & 0xFFFFFFFFu, r = 0, bit = (uint64_t)1 << 30;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= r + bit) {
            x -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (int64_t)r;
}

static int64_t isqrt64(uint64_t x)
{
    uint64_t r = 0, bit = (uint64_t)1 << 62;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= r + bit) {
            x -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (int64_t)r;
}

/* min(x * y, cap) without overflowing: Python multiplies unbounded first. */
static uint64_t sat_mul(uint64_t x, uint64_t y, uint64_t cap)
{
    if (y && x > cap / y) return cap;
    return x * y < cap ? x * y : cap;
}

static int64_t calc_pll_period_mm(int64_t fast_osc_frequency)
{
    int64_t pll_period_mm = SPEED_OF_LIGHT_IN_AIR_DIV_8 * (vlxb_calc_pll_period_us(fast_osc_frequency) >> 2);
    return (pll_period_mm + (1 << 15)) >> 16;
}

static int64_t rate_maths(int64_t events, int64_t time_us)
{
    int64_t tmp = 0;
    int frac_bits;
    if (events > SPAD_TOTAL_COUNT_MAX) tmp = SPAD_TOTAL_COUNT_MAX;
    else if (events > 0) tmp = events;
    frac_bits = events > SPAD_TOTAL_COUNT_RES_THRES ? 3 : 7;
    if (time_us > 0) tmp = ((tmp << frac_bits) + (time_us / 2)) / time_us;
    if (events > SPAD_TOTAL_COUNT_RES_THRES) tmp <<= 4;
    return imin(tmp, 0xFFFF);
}

static int64_t rate_per_spad_maths(int frac_bits, int64_t peak_count_rate, int64_t num_spads, int64_t max_out)
{
    int64_t tmp;
    if (num_spads > 0) {
        tmp = (peak_count_rate << 8) << frac_bits;
        tmp = (tmp + num_spads / 2) / num_spads;
    } else {
        tmp = peak_count_rate << frac_bits;
    }
    return imin(tmp, max_out);
}

/* Signal events can be negative here: Python floors. */
static int64_t events_per_spad_maths(int64_t events, int64_t num_spads, int64_t duration)
{
    int64_t total = 0, per_spad;
    if (num_spads != 0) total = fdiv(events * 1000 * 256, num_spads);
    if (duration > 0) per_spad = fdiv(total * 2048 + fdiv(duration, 2), duration);
    else per_spad = total * 2048;
    return (int64_t)((uint64_t)per_spad & 0xFFFFFFFFu);
}

static int64_t range_maths(int64_t fast_osc_frequency, int64_t phase, int64_t zero_distance_phase,
                           int fractional_bits, int64_t gain_factor, int64_t range_offset_mm)
{
    int64_t pll_period_us = vlxb_calc_pll_period_us(fast_osc_frequency);
    int64_t tmp = (phase - zero_distance_phase) * pll_period_us;
    int64_t range_mm, range_mm_10;
    tmp = cdiv(tmp, 1 << 9);
    tmp = tmp * SPEED_OF_LIGHT_IN_AIR_DIV_8;
    tmp = cdiv(tmp, 1 << 22);
    range_mm = tmp + range_offset_mm;
    range_mm *= gain_factor;
    range_mm += 0x0400;
    range_mm = cdiv(range_mm, 0x0800);
    if (fractional_bits == 0) {
        range_mm_10 = cdiv(range_mm * 10, 1 << 2);
        if (iabs(range_mm_10 - cdiv(range_mm_10, 10) * 10) < 5) range_mm = cdiv(range_mm_10, 10);
        else range_mm = cdiv(range_mm_10, 10) + 1;
    } else if (fractional_bits == 1) {
        range_mm = cdiv(range_mm, 1 << 1);
    }
    return range_mm;
}

/* ── working structures (VL53LX_hist_gen3/gen4 private data) ───────────── */

typedef struct {
    int64_t start_bin, first_bin, peak_bin, last_bin, end_bin, width_bins, filter_woi;
    int64_t ambient_events, total_events, signal_events;
    int64_t phase_start, phase_mean, phase_end, sigma;
} pulse_data;

typedef struct {
    int64_t first_bin, buffer_size, bins_in_data, vcsel_period, bins_over_threshold;
    int64_t ambient_per_bin, ambient_threshold;
    int64_t over_threshold[VLXB_BUFFER_SIZE], pulse_mask[VLXB_BUFFER_SIZE];
    int64_t pulse_no[VLXB_BUFFER_SIZE], threshold[VLXB_BUFFER_SIZE];
    int64_t first_rising_bin, max_pulses, pulse_count;
    pulse_data pulses[VLXB_MAX_PULSES];
    vlxb_bins pulse_amb, pulse_zero;
} gen3_algo;

typedef struct {
    int64_t a[VLXB_BUFFER_SIZE], b[VLXB_BUFFER_SIZE], c[VLXB_BUFFER_SIZE];
    int64_t left[VLXB_BUFFER_SIZE], right[VLXB_BUFFER_SIZE], is_peak[VLXB_BUFFER_SIZE];
} filtered_data;

static void algo_reset(gen3_algo *g)
{
    int i;
    memset(g, 0, sizeof *g);
    g->buffer_size = VLXB_BUFFER_SIZE;
    g->max_pulses = VLXB_MAX_PULSES;
    for (i = 0; i < VLXB_MAX_PULSES; i++) g->pulses[i].peak_bin = 0xFF;
}

/* ── bin housekeeping ──────────────────────────────────────────────────── */

/* VL53LX_f_031: fold repeated bin-sequence codes together. */
static void average_repeated_bins(const vlxb_bins *src, vlxb_bins *dst)
{
    int64_t initial_index[VLXB_MAX_BIN_SEQ_CODE + 2] = {0}, repeat_count[VLXB_MAX_BIN_SEQ_CODE + 2] = {0};
    int64_t seq_length = 0;
    int lc, i, code;
    *dst = *src;
    dst->bins_in_data = 0;
    for (lc = 0; lc < VLXB_MAX_BIN_SEQ_LENGTH; lc++) dst->bin_seq[lc] = VLXB_MAX_BIN_SEQ_CODE + 1;
    for (i = 0; i < VLXB_BUFFER_SIZE; i++) dst->bin_data[i] = 0;
    for (lc = 0; lc < VLXB_MAX_BIN_SEQ_LENGTH; lc++) {
        int64_t bin_cfg = src->bin_seq[lc], base;
        if (repeat_count[bin_cfg] == 0) {
            initial_index[bin_cfg] = seq_length * 4;
            dst->bin_seq[seq_length] = bin_cfg;
            seq_length++;
        }
        repeat_count[bin_cfg]++;
        base = initial_index[bin_cfg];
        for (i = 0; i < 4; i++) dst->bin_data[base + i] += src->bin_data[lc * 4 + i];
    }
    for (lc = 0; lc < VLXB_MAX_BIN_SEQ_LENGTH; lc++) {
        int64_t c = dst->bin_seq[lc];
        dst->bin_rep[lc] = c <= VLXB_MAX_BIN_SEQ_CODE ? repeat_count[c] : 0;
    }
    dst->bins_in_data = seq_length * 4;
    for (code = 0; code <= VLXB_MAX_BIN_SEQ_CODE; code++) {
        int64_t reps = repeat_count[code];
        if (reps > 0) {
            int64_t base = initial_index[code];
            for (i = 0; i < 4; i++) dst->bin_data[base + i] = (dst->bin_data[base + i] + reps / 2) / reps;
        }
    }
    /* Codes 7 and 15 are the ambient-only sequence entries. */
    dst->number_of_ambient_bins = (repeat_count[7] || repeat_count[15]) ? 4 : 0;
}

static void calc_zero_distance_phase(vlxb_bins *b)
{
    int64_t period = 2048 * vlxb_decode_vcsel_period(b->vcsel_period);
    int64_t phase = period + b->reference_phase + 2048 * b->vcsel_start - 2048 * b->cal_config_vcsel_start;
    b->zero_distance_phase = period ? fmod64(phase, period) : 0;
}

static void estimate_ambient_from_thresholded_bins(int64_t sigma, vlxb_bins *b)
{
    int64_t threshold, i;
    b->min_bin_value = b->max_bin_value = 0;
    for (i = 0; i < b->bins_in_data; i++) {
        if (i == 0 || b->bin_data[i] < b->min_bin_value) b->min_bin_value = b->bin_data[i];
        if (i == 0 || b->bin_data[i] > b->max_bin_value) b->max_bin_value = b->bin_data[i];
    }
    threshold = isqrt32(b->min_bin_value);
    threshold *= sigma;
    threshold += 0x07;
    threshold >>= 4;
    threshold += b->min_bin_value;
    b->number_of_ambient_samples = 0;
    b->ambient_events_sum = 0;
    for (i = 0; i < b->bins_in_data; i++) {
        if (b->bin_data[i] < threshold) {
            b->ambient_events_sum += b->bin_data[i];
            b->number_of_ambient_samples++;
        }
    }
    if (b->number_of_ambient_samples > 0)
        b->ambient_per_bin = cdiv(b->ambient_events_sum + b->number_of_ambient_samples / 2, b->number_of_ambient_samples);
}

static void estimate_ambient_from_ambient_bins(vlxb_bins *b)
{
    int64_t i;
    if (b->number_of_ambient_bins > 0) {
        b->number_of_ambient_samples = b->number_of_ambient_bins;
        b->ambient_events_sum = 0;
        for (i = 0; i < b->number_of_ambient_bins; i++) b->ambient_events_sum += b->bin_data[i];
        b->ambient_per_bin = cdiv(b->ambient_events_sum + b->number_of_ambient_bins / 2, b->number_of_ambient_bins);
    }
}

static void remove_ambient_bins(vlxb_bins *b)
{
    int64_t n = b->number_of_ambient_bins, i;
    if ((b->bin_seq[0] & 0x07) == 0x07) {
        int64_t seq[VLXB_MAX_BIN_SEQ_LENGTH], rep[VLXB_MAX_BIN_SEQ_LENGTH];
        int kept = 0, lc;
        for (lc = 0; lc < VLXB_MAX_BIN_SEQ_LENGTH; lc++) {
            if ((b->bin_seq[lc] & 0x07) != 0x07) {
                seq[kept] = b->bin_seq[lc];
                rep[kept] = b->bin_rep[lc];
                kept++;
            }
        }
        for (lc = 0; lc < VLXB_MAX_BIN_SEQ_LENGTH; lc++) {
            b->bin_seq[lc] = lc < kept ? seq[lc] : VLXB_MAX_BIN_SEQ_CODE + 1;
            b->bin_rep[lc] = lc < kept ? rep[lc] : 0;
        }
    }
    if (n > 0) {
        for (i = 0; i < VLXB_BUFFER_SIZE; i++) b->bin_data[i] = i + n < VLXB_BUFFER_SIZE ? b->bin_data[i + n] : 0;
        b->bins_in_data -= n;
        b->number_of_ambient_bins = 0;
    }
}

/* VL53LX_f_022: the three-tap window sums around `bin_index`. */
static void woi_sums(int64_t bin_index, int64_t filter_woi, const vlxb_bins *b, int64_t *a, int64_t *bb, int64_t *c)
{
    int64_t w;
    *a = 0;
    *bb = b->bin_data[bin_index];
    *c = 0;
    for (w = 0; w < (filter_woi << 1) + 1; w++) {
        int64_t j = fmod64((bin_index + w + b->bins_in_data) - filter_woi, b->bins_in_data);
        if (w < filter_woi) *a += b->bin_data[j];
        else if (w > filter_woi) *c += b->bin_data[j];
    }
}

/* VL53LX_f_018: event sums -> count rates. */
static void calc_rates(vlxb_range *t, int64_t vcsel_width, int64_t fast_osc, int64_t periods, int64_t spads)
{
    int64_t pll, elapsed, total;
    t->vcsel_width = vcsel_width;
    t->fast_osc_frequency = fast_osc;
    t->total_periods_elapsed = periods;
    t->spads = spads;
    if (fast_osc == 0 || periods == 0) return;
    pll = vlxb_calc_pll_period_us(fast_osc);
    elapsed = periods + 1;
    t->peak_duration_us = vlxb_duration_maths(pll, vcsel_width, VLXB_RANGING_WINDOW_VCSEL_PERIODS, elapsed);
    t->woi_duration_us = vlxb_duration_maths(pll, t->window_bins << 4, VLXB_RANGING_WINDOW_VCSEL_PERIODS, elapsed);
    t->peak_signal_count_rate_mcps = rate_maths(t->signal_events, t->peak_duration_us);
    t->avg_signal_count_rate_mcps = rate_maths(t->signal_events, t->woi_duration_us);
    t->ambient_count_rate_mcps = rate_maths(t->ambient_events, t->woi_duration_us);
    total = t->peak_signal_count_rate_mcps + t->ambient_count_rate_mcps; /* merge_nb 1 */
    t->total_rate_per_spad_mcps = rate_per_spad_maths(0x06, total, spads, 0xFFFF);
    t->signal_events_per_spad_kcps = events_per_spad_maths(t->signal_events, spads, t->peak_duration_us);
}

static void calc_ranges(int64_t gain, int64_t offset, vlxb_range *t)
{
    t->min_range_mm = range_maths(t->fast_osc_frequency, t->phase_start, t->zero_distance_phase, 0, gain, offset);
    t->median_range_mm = range_maths(t->fast_osc_frequency, t->phase_mean, t->zero_distance_phase, 0, gain, offset);
    t->max_range_mm = range_maths(t->fast_osc_frequency, t->phase_end, t->zero_distance_phase, 0, gain, offset);
}

/* ── sigma (VL53LX_f_023) ──────────────────────────────────────────────── */

static int64_t sigma_estimate(int64_t sigma_ref_mm, int64_t a, int64_t b, int64_t c, int64_t a_zp, int64_t c_zp,
                              int64_t bx, int64_t ax_zp, int64_t cx_zp, int64_t ambient_per_bin, int64_t fast_osc)
{
    uint64_t pll_mm, b_minus_amb, a_minus_c, tmp0, tmp1, sq;
    if (fast_osc == 0) return SIGMA_INVALID;
    pll_mm = (uint64_t)calc_pll_period_mm(fast_osc);
    b_minus_amb = (uint64_t)iabs(ambient_per_bin - b);
    a_minus_c = (uint64_t)iabs(a - c);
    if (b_minus_amb == 0) return SIGMA_INVALID;

    tmp0 = (uint64_t)imin(b + bx + ambient_per_bin, (int64_t)D_003);
    sq = a_minus_c * a_minus_c;
    tmp1 = sq > (D_004 >> 8) ? D_004 : sq << 8;
    tmp1 /= b_minus_amb;
    tmp1 /= b_minus_amb;
    if (tmp1 > D_005) tmp1 = D_005;
    tmp0 = tmp1 * tmp0;
    tmp1 = (uint64_t)imin(c_zp + cx_zp + a_zp + ax_zp, (int64_t)D_003) << 8;
    tmp0 = tmp1 + tmp0;
    if (tmp0 > D_006) tmp0 = D_006;

    if (tmp0 > D_007) tmp0 = sat_mul(tmp0 / b_minus_amb, pll_mm, D_006);
    else tmp0 = sat_mul(tmp0 * pll_mm / b_minus_amb, 1, D_006);
    if (tmp0 > D_007) tmp0 = sat_mul((tmp0 / b_minus_amb) / 4, pll_mm, D_006);
    else tmp0 = sat_mul((tmp0 * pll_mm / b_minus_amb) / 4, 1, D_006);

    tmp0 >>= 2;
    if (tmp0 > D_007) tmp0 = D_007;
    tmp1 = (uint64_t)sigma_ref_mm << 7;
    tmp0 = tmp0 + tmp1 * tmp1;
    if (tmp0 > D_007) tmp0 = D_007;
    return isqrt64(tmp0);
}

static int64_t pulse_sigma(const pulse_data *p, int64_t sigma_ref_mm, const gen3_algo *g)
{
    int64_t i, a_zp, bz, c_zp, a, b, c;
    if (g->vcsel_period == 0) return SIGMA_INVALID;
    i = fmod64(p->peak_bin, g->vcsel_period);
    woi_sums(i, p->filter_woi, &g->pulse_zero, &a_zp, &bz, &c_zp);
    woi_sums(i, p->filter_woi, &g->pulse_amb, &a, &b, &c);
    return sigma_estimate(sigma_ref_mm, a, b, c, a_zp, c_zp, 0, 0, 0, g->pulse_amb.ambient_per_bin,
                          g->pulse_amb.fast_osc_frequency);
}

/* ── pulse detection (gen3) ────────────────────────────────────────────── */

static void ambient_thresholds(int64_t scaler, int64_t threshold_sigma, int64_t min_threshold_events,
                               const vlxb_bins *b, gen3_algo *g)
{
    int64_t amb_events, lb;
    g->buffer_size = b->number_of_bins;
    g->first_bin = b->first_bin;
    g->bins_in_data = b->bins_in_data;
    g->ambient_per_bin = b->ambient_per_bin;
    g->vcsel_period = vlxb_decode_vcsel_period(b->vcsel_period);
    amb_events = cdiv(b->ambient_per_bin * scaler + 2048, 4096);
    for (lb = 0; lb < b->bins_in_data; lb++) {
        int64_t samples = b->bin_rep[lb >> 2], value;
        if (samples <= 0) continue;
        value = samples * amb_events; /* crosstalk off */
        value = isqrt32(value);
        value += samples / 2;
        value /= samples;
        value *= threshold_sigma;
        value += 8;
        value /= 16;
        value += amb_events;
        value = imax(value, min_threshold_events);
        g->threshold[lb] = value;
        g->ambient_threshold = value;
    }
    g->bins_over_threshold = 0;
    for (lb = b->first_bin; lb < b->bins_in_data; lb++) {
        int64_t over = b->bin_data[lb] > g->threshold[lb] ? 1 : 0;
        g->over_threshold[lb] = over;
        g->pulse_mask[lb] = over;
        g->bins_over_threshold += over;
    }
}

static void find_first_rising_edge(gen3_algo *g)
{
    int64_t i;
    bool found = false;
    g->first_rising_bin = 0;
    for (i = 0; i < g->vcsel_period; i++) {
        int64_t j = (i + 1) % g->vcsel_period;
        if (i < g->bins_in_data && j < g->bins_in_data && g->pulse_mask[i] == 0 && g->pulse_mask[j] == 1 && !found) {
            g->first_rising_bin = i;
            found = true;
        }
    }
}

static void assign_pulse_numbers(gen3_algo *g)
{
    int64_t lb;
    for (lb = g->first_rising_bin; lb < g->first_rising_bin + g->vcsel_period; lb++) {
        int64_t i = lb % g->vcsel_period, j = (lb + 1) % g->vcsel_period;
        if (!(i < g->bins_in_data && j < g->bins_in_data)) continue;
        if (g->pulse_mask[i] == 0 && g->pulse_mask[j] == 1) g->pulse_count++;
        g->pulse_count = imin(g->pulse_count, g->max_pulses);
        g->pulse_no[i] = g->pulse_mask[i] > 0 ? g->pulse_count : 0;
    }
}

static void pulse_extents(gen3_algo *g)
{
    int64_t max_half = (g->vcsel_period - 1) >> 1, blb;
    for (blb = g->first_rising_bin; blb < g->first_rising_bin + g->vcsel_period; blb++) {
        int64_t i = blb % g->vcsel_period, j = (blb + 1) % g->vcsel_period;
        if (!(i < g->bins_in_data && j < g->bins_in_data)) continue;
        if (g->pulse_no[i] == 0 && g->pulse_no[j] > 0) {
            int64_t no = g->pulse_no[j] - 1;
            if (no < g->max_pulses) {
                pulse_data *p = &g->pulses[no];
                p->start_bin = blb;
                p->first_bin = blb + 1;
                p->peak_bin = 0xFF;
                p->last_bin = 0;
                p->end_bin = 0;
            }
        }
        if (g->pulse_no[i] > 0 && g->pulse_no[j] == 0) {
            int64_t no = g->pulse_no[i] - 1;
            if (no < g->max_pulses) {
                pulse_data *p = &g->pulses[no];
                p->last_bin = blb;
                p->end_bin = blb + 1;
                p->width_bins = (p->last_bin + 1) - p->first_bin;
                p->filter_woi = imin((p->end_bin + 1) - p->start_bin, max_half);
            }
        }
    }
}

static void pulse_event_sums(pulse_data *p, const vlxb_bins *b, const gen3_algo *g)
{
    int64_t lb;
    p->total_events = 0;
    p->ambient_events = 0;
    for (lb = p->start_bin; lb <= p->end_bin; lb++) {
        p->total_events += b->bin_data[fmod64(lb, g->vcsel_period)];
        p->ambient_events += g->ambient_per_bin;
    }
    p->signal_events = p->total_events - p->ambient_events;
}

static void isolate_pulse(const pulse_data *p, const vlxb_bins *b, const gen3_algo *g, int64_t pad, vlxb_bins *out)
{
    int64_t lb;
    *out = *b;
    for (lb = g->first_rising_bin; lb < g->first_rising_bin + g->vcsel_period; lb++) {
        if (lb < p->start_bin || lb > p->end_bin) {
            int64_t i = lb % g->vcsel_period;
            if (i < out->bins_in_data) out->bin_data[i] = pad;
        }
    }
}

/* VL53LX_f_020: centre of mass of [start, end], in 1/2048ths of a bin. */
static int64_t weighted_phase(int64_t start, int64_t end, int64_t vcsel_period, bool clip, const vlxb_bins *b)
{
    int64_t event_sum = 0, weighted_sum = 0, lb;
    if (vcsel_period == 0) return MAX_ALLOWED_PHASE;
    for (lb = start; lb <= end; lb++) {
        int64_t i = lb < 0 ? lb + vcsel_period : lb % vcsel_period;
        if (i >= 0 && i < VLXB_BUFFER_SIZE) {
            int64_t value = b->bin_data[i] - b->ambient_per_bin;
            if (clip && value < 0) value = 0;
            event_sum += value;
            weighted_sum += value * (1024 + 2048 * lb);
        }
    }
    if (event_sum > 0) {
        weighted_sum += cdiv(event_sum, 2);
        weighted_sum = cdiv(weighted_sum, event_sum);
        return imax(weighted_sum, 0);
    }
    return MAX_ALLOWED_PHASE;
}

static void pulse_phase_limits(pulse_data *p, bool clip, const vlxb_bins *b, const gen3_algo *g)
{
    int64_t i, start, end, ww;
    if (p->peak_bin == 0xFF) p->peak_bin = 1;
    i = p->peak_bin % g->vcsel_period;
    start = i + p->start_bin - p->peak_bin;
    end = i + p->end_bin - p->peak_bin;
    ww = imin(end - start, 3);
    p->phase_start = weighted_phase(start, start + ww, g->vcsel_period, clip, b);
    p->phase_end = weighted_phase(end - ww, end, g->vcsel_period, clip, b);
    if (p->phase_start > p->phase_end) {
        int64_t t = p->phase_start;
        p->phase_start = p->phase_end;
        p->phase_end = t;
    }
    p->phase_start = imin(p->phase_start, p->phase_mean);
    p->phase_end = imax(p->phase_end, p->phase_mean);
}

/* Python's list.sort: stable. */
static void sort_pulses(int64_t order, gen3_algo *g)
{
    int64_t n = g->pulse_count, i, j;
    if (n <= 1) return;
    for (i = 1; i < n; i++) {
        pulse_data key = g->pulses[i];
        int64_t kv = order == HIST_TARGET_ORDER_STRONGEST_FIRST ? -key.signal_events : key.phase_mean;
        for (j = i - 1; j >= 0; j--) {
            int64_t v = order == HIST_TARGET_ORDER_STRONGEST_FIRST ? -g->pulses[j].signal_events : g->pulses[j].phase_mean;
            if (v <= kv) break;
            g->pulses[j + 1] = g->pulses[j];
        }
        g->pulses[j + 1] = key;
    }
}

static void fill_target(int64_t range_id, int64_t vpl, int64_t vph, int64_t sigma_thresh, const vlxb_bins *b,
                        const pulse_data *p, vlxb_range *t)
{
    int64_t lower, upper;
    memset(t, 0, sizeof *t);
    t->range_id = range_id;
    t->start_bin = p->start_bin;
    t->first_bin = p->first_bin;
    t->peak_bin = p->peak_bin;
    t->last_bin = p->last_bin;
    t->end_bin = p->end_bin;
    t->width_bins = p->width_bins;
    t->window_bins = (p->end_bin + 1) - p->start_bin;
    t->zero_distance_phase = b->zero_distance_phase;
    t->sigma = p->sigma;
    t->phase_start = p->phase_start & 0xFFFF;
    t->phase_mean = p->phase_mean & 0xFFFF;
    t->phase_end = p->phase_end & 0xFFFF;
    t->total_events = p->total_events;
    t->signal_events = p->signal_events;
    t->ambient_events = p->ambient_events;
    t->total_periods_elapsed = b->total_periods_elapsed;
    t->range_status = DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK;
    if (sigma_thresh > 0 && p->sigma > (sigma_thresh << 5)) t->range_status = DEVICEERROR_SIGMATHRESHOLDCHECK;
    lower = (vpl << 8) & 0xFFFF;
    lower = lower < t->zero_distance_phase ? t->zero_distance_phase - lower : 0;
    upper = ((vph << 8) + b->zero_distance_phase) & 0xFFFF;
    if (t->phase_mean < lower || t->phase_mean > upper) t->range_status = DEVICEERROR_RANGEPHASECHECK;
}

/* ── peak location (gen4) ──────────────────────────────────────────────── */

static void filter_pulse(const pulse_data *p, const vlxb_bins *pb, const gen3_algo *g, filtered_data *f)
{
    int64_t lb;
    for (lb = p->start_bin; lb <= p->end_bin; lb++) {
        int64_t i = fmod64(lb, g->vcsel_period), a, b, c;
        woi_sums(i, p->filter_woi, pb, &a, &b, &c);
        f->a[i] = a;
        f->b[i] = b;
        f->c[i] = c;
        f->left[i] = (a + b) - (c + g->ambient_per_bin);
        f->right[i] = (b + c) - (a + g->ambient_per_bin);
    }
}

/* VL53LX_f_028; false when the pulse has no height. */
static bool interpolate_phase(int64_t bin_index, int64_t a, int64_t b, int64_t c, int64_t amb, int64_t vcsel_period,
                              int64_t *out)
{
    int64_t numerator = 4096 * (c - a), half = 4096 * (b - amb), mean;
    if (half == 0) return false;
    mean = (4096 * numerator) + half;
    mean = cdiv(mean, half * 2);
    mean += 2048;
    mean += 4096 * bin_index;
    mean = cdiv(mean + 1, 2);
    mean = imin(imax(mean, 0), MAX_ALLOWED_PHASE);
    *out = mean % (vcsel_period * 2048);
    return true;
}

static void find_peak_bin(pulse_data *p, filtered_data *f, const gen3_algo *g)
{
    int64_t lb;
    for (lb = p->start_bin; lb < p->end_bin; lb++) {
        int64_t i = fmod64(lb, g->vcsel_period), j = fmod64(lb + 1, g->vcsel_period), is_peak, phase;
        if (!(i < g->bins_in_data && j < g->bins_in_data)) continue;
        if (f->left[i] == 0 && f->right[i] == 0) is_peak = 0;
        else if (f->left[i] >= 0 && f->right[i] >= 0) is_peak = 1;
        else if (f->left[i] < 0 && f->right[i] >= 0 && f->left[j] >= 0 && f->right[j] < 0) is_peak = 1;
        else is_peak = 0;
        f->is_peak[i] = is_peak;
        if (is_peak) {
            p->peak_bin = lb;
            if (!interpolate_phase(lb, f->a[i], f->b[i], f->c[i], g->ambient_per_bin, g->vcsel_period, &phase))
                f->is_peak[i] = 0;
            else
                p->phase_mean = phase;
        }
    }
}

/* ── entry point ───────────────────────────────────────────────────────── */

void vlxb_process_data(const vlxb_bins *in, const vlxb_hpp *hpp, vlxb_results *out)
{
    gen3_algo *g = (gen3_algo *)malloc(sizeof *g);
    filtered_data f;
    vlxb_bins bins;
    int64_t p;
    memset(out, 0, sizeof *out);
    if (!g) return; /* no memory: no targets */
    algo_reset(g);
    memset(&f, 0, sizeof f);
    average_repeated_bins(in, &bins);
    out->stream_count = in->stream_count;
    calc_zero_distance_phase(&bins);
    estimate_ambient_from_thresholded_bins(hpp->ambient_thresh_sigma0, &bins);
    estimate_ambient_from_ambient_bins(&bins);
    remove_ambient_bins(&bins);
    ambient_thresholds(hpp->ambient_thresh_events_scaler, hpp->ambient_thresh_sigma1, hpp->min_ambient_thresh_events,
                       &bins, g);
    find_first_rising_edge(g);
    assign_pulse_numbers(g);
    pulse_extents(g);
    for (p = 0; p < g->pulse_count; p++) {
        pulse_data *pd = &g->pulses[p];
        pulse_event_sums(pd, &bins, g);
        isolate_pulse(pd, &bins, g, bins.ambient_per_bin, &g->pulse_amb);
        isolate_pulse(pd, &bins, g, 0, &g->pulse_zero);
        filter_pulse(pd, &g->pulse_amb, g, &f);
        find_peak_bin(pd, &f, g);
        pd->sigma = pulse_sigma(pd, hpp->sigma_estimator_sigma_ref_mm, g);
        pulse_phase_limits(pd, true, &bins, g);
    }
    sort_pulses(hpp->hist_target_order, g);
    for (p = 0; p < g->pulse_count; p++) {
        const pulse_data *pd = &g->pulses[p];
        vlxb_range *t;
        if (out->n_targets >= VLXB_MAX_RANGE_RESULTS) break;
        if (!(pd->signal_events > hpp->signal_total_events_limit && pd->peak_bin < 0xFF)) continue;
        t = &out->targets[out->n_targets];
        fill_target((int64_t)out->n_targets, hpp->valid_phase_low, hpp->valid_phase_high, hpp->sigma_thresh, &bins, pd,
                    t);
        calc_rates(t, bins.vcsel_width, bins.fast_osc_frequency, bins.total_periods_elapsed,
                   bins.dss_actual_effective_spads);
        calc_ranges(hpp->gain_factor, hpp->range_offset_mm, t);
        out->n_targets++;
    }
    free(g);
}

/* ── frame-to-frame consistency ────────────────────────────────────────── */

void vlxb_history_reset(vlxb_history *h) { memset(h, 0, sizeof *h); }

/* VL53LX_hist_events_consistency_check(). */
static int64_t events_consistency_check(int64_t event_sigma, int64_t min_spads, const vlxb_history *h, size_t k,
                                        const vlxb_range *t)
{
    int64_t tmpp, tmpc, scaler, scaler_sq, c_signal, c_sig_noise_sq, c_amb_noise_sq, p_amb_noise_sq;
    int64_t noise_sq_sum, tolerance, p_signal, delta;
    if (event_sigma == 0) return DEVICEERROR_RANGECOMPLETE;
    tmpp = (1 + h->total_periods_elapsed) * h->spads;
    tmpc = (1 + t->total_periods_elapsed) * t->spads;
    /* tmpc 0 means no SPADs, which the last test below turns into
     * RANGECOMPLETE whatever the arithmetic says (and it would overflow). */
    if (tmpc == 0) return DEVICEERROR_RANGECOMPLETE;
    scaler = cdiv(tmpp * 4096 + tmpc / 2, tmpc);
    scaler_sq = cdiv(scaler * scaler + 2048, 4096);
    c_signal = cdiv((t->total_events - t->ambient_events) * scaler + 2048, 4096);
    c_sig_noise_sq = cdiv(scaler_sq * t->total_events + 2048, 4096);
    c_amb_noise_sq = cdiv(scaler_sq * t->ambient_events + 2048, 4096);
    c_amb_noise_sq = cdiv(c_amb_noise_sq + 2, 4);
    p_amb_noise_sq = cdiv(h->targets[k].ambient_events + 2, 4);
    noise_sq_sum = (int64_t)((uint64_t)(h->targets[k].total_events + c_sig_noise_sq + p_amb_noise_sq + c_amb_noise_sq) &
                             0xFFFFFFFFu);
    tolerance = isqrt32(noise_sq_sum * 16);
    tolerance = cdiv(tolerance * event_sigma + 32, 64);
    p_signal = h->targets[k].total_events - h->targets[k].ambient_events;
    delta = iabs(c_signal - p_signal);
    if (delta > tolerance && t->spads > min_spads) return DEVICEERROR_EVENTCONSISTENCY;
    return DEVICEERROR_RANGECOMPLETE;
}

static int64_t merged_pulse_check(int64_t min_max_tolerance_mm, const vlxb_range *t)
{
    if (min_max_tolerance_mm > 0 && iabs(t->max_range_mm - t->min_range_mm) > min_max_tolerance_mm)
        return DEVICEERROR_RANGECOMPLETE_MERGED_PULSE;
    return DEVICEERROR_RANGECOMPLETE;
}

/* VL53LX_hist_phase_consistency_check(), against the remembered frame. */
static void phase_consistency_check(const vlxb_history *h, vlxb_results *r, const vlxb_hpp *hpp)
{
    int64_t tol = hpp->phase_tolerance << 8;
    size_t i, k;
    if (h->rd_device_state != VLXB_STATE_OUTPUT_DATA || tol == 0) return;
    for (i = 0; i < r->n_targets; i++) {
        vlxb_range *t = &r->targets[i];
        if (t->range_status != DEVICEERROR_RANGECOMPLETE && t->range_status != DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK)
            continue;
        t->range_status = h->n_targets ? DEVICEERROR_PHASECONSISTENCY : DEVICEERROR_PREV_RANGE_NO_TARGETS;
        for (k = 0; k < h->n_targets; k++) {
            int64_t status;
            if (iabs(t->phase_mean - h->targets[k].phase_mean) >= tol) continue;
            status = events_consistency_check(hpp->event_sigma, hpp->event_min_spad_count, h, k, t);
            if (status == DEVICEERROR_RANGECOMPLETE) status = merged_pulse_check(hpp->min_max_tolerance, t);
            t->range_status = status;
        }
    }
}

void vlxb_history_apply(vlxb_history *h, vlxb_results *results, const vlxb_bins *bins, vlxb_state_id rd_device_state,
                        const vlxb_hpp *hpp)
{
    size_t i;
    phase_consistency_check(h, results, hpp);
    h->rd_device_state = rd_device_state;
    h->total_periods_elapsed = bins->total_periods_elapsed;
    h->spads = bins->dss_actual_effective_spads;
    h->n_targets = results->n_targets;
    for (i = 0; i < results->n_targets; i++) {
        h->targets[i].ambient_events = results->targets[i].ambient_events;
        h->targets[i].total_events = results->targets[i].total_events;
        h->targets[i].phase_mean = results->targets[i].phase_mean;
        h->targets[i].range_status = results->targets[i].range_status;
    }
}

/* ── status ────────────────────────────────────────────────────────────── */

int vlxb_convert_status(int64_t e)
{
    switch (e) {
    case DEVICEERROR_RANGEPHASECHECK:             return 4;
    case DEVICEERROR_SIGMATHRESHOLDCHECK:         return 1;
    case DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK: return 6;
    case DEVICEERROR_PHASECONSISTENCY:            return 7;
    case DEVICEERROR_EVENTCONSISTENCY:            return 7;
    case DEVICEERROR_PREV_RANGE_NO_TARGETS:       return 12;
    case DEVICEERROR_RANGECOMPLETE_MERGED_PULSE:  return 11;
    case DEVICEERROR_RANGECOMPLETE:               return 0;
    default:                                      return VLXB_RANGESTATUS_NONE;
    }
}

const char *vlxb_status_text(int s)
{
    switch (s) {
    case 0:   return "valid";
    case 1:   return "sigma above threshold";
    case 4:   return "phase out of valid limit";
    case 6:   return "no wrap-around check done";
    case 7:   return "wrapped target";
    case 11:  return "valid, merged pulse";
    case 12:  return "no target in the frame before";
    case 255: return "no target";
    default:  return "unknown";
    }
}
