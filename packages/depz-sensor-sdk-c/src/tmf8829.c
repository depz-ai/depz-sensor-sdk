/* TMF8829 SPI register bridge (contract 14): wire codecs, chunk reassembly,
 * frame / pixel decode, the clock model and capture time, measurement
 * assembly, presets and config-page fields — a port of the Python SDK's
 * protocol/tmf8829.py and tmf8829/{frames,clock,builder,config}.py.
 *
 * The clock math is integer and bit-exact with the reference: u32
 * differences modulo 2^32, signed 32-bit tick deltas, 64-bit µs, floor
 * division and floor shifts also for negative operands (C's `/` truncates
 * toward zero and `>>` of a negative value is implementation-defined, so
 * both go through helpers here). */
#include "depz_sensor_sdk.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t rd64(const uint8_t *p) { return (uint64_t)rd32(p) | (uint64_t)rd32(p + 4) << 32; }

/* floor(a / b) for b > 0. */
static int64_t floor_div(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if (a % b != 0 && a < 0) q--;
    return q;
}

/* floor(x / 2^16), i.e. Python's arithmetic x >> 16. */
static int64_t floor_shr16(int64_t x)
{
    if (x >= 0) return x >> 16;
    return -(int64_t)(((uint64_t)(-(x + 1))) >> 16) - 1;
}

/* u32 -> s32 without the implementation-defined conversion. */
static int64_t s32(uint32_t v) { return v >= 0x80000000u ? (int64_t)v - 0x100000000LL : (int64_t)v; }

/* ── encoders ──────────────────────────────────────────────────────────── */

size_t depz_tmf8829_pack_read_reg(int addr, int len, uint8_t out[3])
{
    if (addr < 0 || addr > 0xFF || len < 1 || len > (int)DEPZ_TMF8829_READ_MAX) return 0;
    out[0] = (uint8_t)addr;
    out[1] = (uint8_t)len;
    out[2] = (uint8_t)(len >> 8);
    return 3;
}

size_t depz_tmf8829_pack_write_reg(int addr, const uint8_t *data, size_t data_len, uint8_t *out)
{
    if (addr < 0 || addr > 0xFF || data_len < 1 || data_len > DEPZ_TMF8829_WRITE_MAX) return 0;
    out[0] = (uint8_t)addr;
    memcpy(out + 1, data, data_len);
    return 1 + data_len;
}

size_t depz_tmf8829_pack_start_stream(int int_mask, int flags, uint8_t out[2])
{
    if (int_mask < 1 || int_mask > (int)DEPZ_TMF8829_INT_MASK_ALL) return 0;
    if (flags < 0 || (flags & ~(int)DEPZ_TMF8829_SF_REF_FRAME)) return 0;
    out[0] = (uint8_t)int_mask;
    out[1] = (uint8_t)flags;
    return 2;
}

/* ── report decoders ───────────────────────────────────────────────────── */

int depz_tmf8829_unpack_reg_data(const uint8_t *p, size_t len, depz_tmf8829_reg_data *out)
{
    if (len < DEPZ_TMF8829_REG_DATA_HEADER_SIZE || p[0] != DEPZ_TMF8829_CMD_READ_REG) return -1;
    out->timestamp_us = rd64(p + 1);
    out->data = p + DEPZ_TMF8829_REG_DATA_HEADER_SIZE;
    out->data_len = len - DEPZ_TMF8829_REG_DATA_HEADER_SIZE;
    return 0;
}

int depz_tmf8829_unpack_info(const uint8_t *p, size_t len, depz_tmf8829_info *out)
{
    if (len < DEPZ_TMF8829_INFO_SIZE) return -1;
    out->int_edges = rd32(p);
    out->frames = rd32(p + 4);
    out->frames_aborted = rd32(p + 8);
    out->frame_errors = rd32(p + 12);
    out->spi_timeouts = rd32(p + 16);
    out->int_level = p[20];
    out->streaming = p[21] != 0;
    out->int_mask = p[22];
    out->last_int_status = p[23];
    out->last_fifo_status = p[24];
    return 0;
}

int depz_tmf8829_unpack_frame_chunk(const uint8_t *p, size_t len, depz_tmf8829_frame_chunk *out)
{
    if (len < DEPZ_TMF8829_FRAME_CHUNK_HEADER_SIZE) return -1;
    out->timestamp_us = rd64(p);
    out->full_size = rd16(p + 8);
    out->offset = rd16(p + 10);
    out->frame_seq = p[12];
    out->edge_us = rd64(p + 13);
    out->data = p + DEPZ_TMF8829_FRAME_CHUNK_HEADER_SIZE;
    out->data_len = len - DEPZ_TMF8829_FRAME_CHUNK_HEADER_SIZE;
    return 0;
}

int depz_tmf8829_unpack_event(const uint8_t *p, size_t len, depz_tmf8829_event *out)
{
    if (len < DEPZ_TMF8829_EVENT_SIZE) return -1;
    out->timestamp_us = rd64(p);
    out->int_status = p[8];
    out->code = p[9];
    out->frame_id = p[10];
    out->fifo_status = p[11];
    return 0;
}

/* ── chunk reassembly ──────────────────────────────────────────────────── */

void depz_tmf8829_reassembler_reset(depz_tmf8829_reassembler *r)
{
    r->completed = r->discarded = r->seq_gaps = 0;
    r->has_head = r->has_next_seq = false;
    r->next_seq = 0;
    r->len = 0;
}

static void drop_head(depz_tmf8829_reassembler *r)
{
    r->has_head = false;
    r->len = 0;
}

int depz_tmf8829_reassembler_feed(depz_tmf8829_reassembler *r, const depz_tmf8829_frame_chunk *c,
                                  depz_tmf8829_raw_frame *out)
{
    if (c->offset == 0) {
        if (r->has_head) r->discarded++; /* the previous frame never completed */
        if (r->has_next_seq && c->frame_seq != r->next_seq)
            r->seq_gaps += (uint8_t)(c->frame_seq - r->next_seq);
        r->next_seq = (uint8_t)(c->frame_seq + 1);
        r->has_next_seq = true;
        r->has_head = true;
        r->head_timestamp_us = c->timestamp_us;
        r->head_edge_us = c->edge_us;
        r->head_full_size = c->full_size;
        r->head_seq = c->frame_seq;
        if (c->data_len > c->full_size) { /* already past full_size */
            r->discarded++;
            drop_head(r);
            return 0;
        }
        memcpy(r->buf, c->data, c->data_len);
        r->len = c->data_len;
    } else if (r->has_head && c->frame_seq == r->head_seq && c->full_size == r->head_full_size &&
               c->offset == r->len) {
        if (r->len + c->data_len > r->head_full_size) {
            r->discarded++;
            drop_head(r);
            return 0;
        }
        memcpy(r->buf + r->len, c->data, c->data_len);
        r->len += c->data_len;
    } else {
        if (r->has_head) r->discarded++;
        drop_head(r);
        return 0;
    }
    if (r->len == r->head_full_size) {
        out->timestamp_us = r->head_timestamp_us;
        out->edge_us = r->head_edge_us;
        out->frame_seq = r->head_seq;
        out->data = r->buf;
        out->len = r->len;
        drop_head(r);
        r->completed++;
        return 1;
    }
    return 0;
}

/* ── frames ────────────────────────────────────────────────────────────── */

static const struct { uint8_t rows, cols; const char *name; } FP_GRID[6] = {
    {8, 8, "8x8A"}, {8, 8, "8x8B"}, {16, 16, "16x16"}, {32, 32, "32x32"}, {32, 32, "32x32s"}, {32, 48, "48x32"},
};

int depz_tmf8829_fp_grid(unsigned fp_mode, unsigned *rows, unsigned *cols)
{
    if (fp_mode >= 6) return -1;
    if (rows) *rows = FP_GRID[fp_mode].rows;
    if (cols) *cols = FP_GRID[fp_mode].cols;
    return 0;
}

int depz_tmf8829_frame_rows(unsigned fp_mode, unsigned *rows, unsigned *cols)
{
    unsigned r, c;
    if (depz_tmf8829_fp_grid(fp_mode, &r, &c)) return -1;
    if (rows) *rows = r == 32 ? r / 2 : r;
    if (cols) *cols = c;
    return 0;
}

const char *depz_tmf8829_fp_name(unsigned fp_mode) { return fp_mode < 6 ? FP_GRID[fp_mode].name : NULL; }

size_t depz_tmf8829_frame_size(const uint8_t head[21])
{
    return DEPZ_TMF8829_PRE_HEADER_SIZE + 4u + rd16(head + 7);
}

int depz_tmf8829_parse_frame(const uint8_t *raw, size_t len, depz_tmf8829_frame_info *f)
{
    const uint8_t *h = raw + DEPZ_TMF8829_PRE_HEADER_SIZE, *t;
    if (len < DEPZ_TMF8829_FRAME_HEAD + DEPZ_TMF8829_FOOTER_SIZE) return -1;
    t = raw + len - DEPZ_TMF8829_FOOTER_SIZE;
    f->fifo_status = raw[0];
    f->systick = rd32(raw + 1);
    f->id = h[0];
    f->layout = h[1];
    f->payload = rd16(h + 2);
    f->f_number = rd32(h + 4);
    f->temperature[0] = (int8_t)(h[8] >= 0x80 ? h[8] - 0x100 : h[8]);
    f->temperature[1] = (int8_t)(h[9] >= 0x80 ? h[9] - 0x100 : h[9]);
    f->temperature[2] = (int8_t)(h[10] >= 0x80 ? h[10] - 0x100 : h[10]);
    f->bdv = h[11];
    f->ref_pos[0] = rd16(h + 12);
    f->ref_pos[1] = rd16(h + 14);
    f->t0_integration = rd32(t);
    f->t1_integration = rd32(t + 4);
    f->frame_status = t[8];
    f->reserved = t[9];
    f->eof = rd16(t + 10);
    f->size = len;
    f->kind = (uint8_t)(f->id & DEPZ_TMF8829_FID_MASK);
    f->fp_mode = (uint8_t)(f->id & 0x0F);
    f->second_half = (f->layout & DEPZ_TMF8829_LAYOUT_SUBFRAME) != 0;
    f->distance_in_mm = (f->reserved & DEPZ_TMF8829_FOOTER_DIST_IN_MM) != 0;
    return 0;
}

static void problem(depz_tmf8829_frame_problems *out, const char *fmt, unsigned v)
{
    if (out->count < DEPZ_TMF8829_FRAME_PROBLEMS_MAX)
        snprintf(out->text[out->count++], sizeof out->text[0], fmt, v);
}

void depz_tmf8829_frame_problems_of(const depz_tmf8829_frame_info *f, depz_tmf8829_frame_problems *out)
{
    out->count = 0;
    if (f->eof != DEPZ_TMF8829_FRAME_EOF) problem(out, "EOF 0x%04X", f->eof);
    if (!(f->frame_status & DEPZ_TMF8829_FRAME_VALID)) problem(out, "status 0x%02X not valid", f->frame_status);
    if (f->frame_status & DEPZ_TMF8829_FRAME_ABORTED) problem(out, "aborted", 0);
    if (f->frame_status & DEPZ_TMF8829_FRAME_HV_CP_OVERLOAD) problem(out, "HV_CP_OVERLOAD", 0);
    if (f->frame_status & DEPZ_TMF8829_FRAME_VCDRV_OVERLOAD) problem(out, "VCDRV_OVERLOAD", 0);
    if (f->frame_status & DEPZ_TMF8829_FRAME_VCDRV_BURST_EXCEEDED) problem(out, "VCDRV_BURST_EXCEEDED", 0);
    if (f->fifo_status & DEPZ_TMF8829_FIFO_TX_UNDERRUN) problem(out, "txfifo_underrun", 0);
    if (f->fifo_status & DEPZ_TMF8829_FIFO_RX_OVERRUN) problem(out, "rxfifo_overrun", 0);
}

/* ── results pixels ────────────────────────────────────────────────────── */

int depz_tmf8829_parse_results(const uint8_t *raw, size_t len, const depz_tmf8829_frame_info *info,
                               depz_tmf8829_results *out, char *err, size_t err_cap)
{
    depz_tmf8829_frame_info parsed;
    const depz_tmf8829_frame_info *f = info;
    unsigned rows, cols, peaks, n, i, p;
    size_t item, data_size;
    bool sig, noise, xtalk;
    char scratch[160];
    if (!err || !err_cap) { err = scratch; err_cap = sizeof scratch; }
    if (!f) {
        if (depz_tmf8829_parse_frame(raw, len, &parsed)) {
            snprintf(err, err_cap, "frame of %zu B is shorter than header + footer", len);
            return -1;
        }
        f = &parsed;
    }
    if (f->kind != DEPZ_TMF8829_FID_RESULTS) {
        snprintf(err, err_cap, "frame id 0x%02X is not a results frame", f->id);
        return -1;
    }
    if (depz_tmf8829_frame_rows(f->fp_mode, &rows, &cols)) {
        snprintf(err, err_cap, "unknown fp_mode %u", f->fp_mode);
        return -1;
    }
    peaks = f->layout & DEPZ_TMF8829_LAYOUT_PEAKS;
    sig = (f->layout & DEPZ_TMF8829_LAYOUT_SIGNAL) != 0;
    noise = (f->layout & DEPZ_TMF8829_LAYOUT_NOISE) != 0;
    xtalk = (f->layout & DEPZ_TMF8829_LAYOUT_XTALK) != 0;
    if (peaks > DEPZ_TMF8829_MAX_PEAKS) {
        snprintf(err, err_cap, "%u peaks (layout 0x%02X): the sensor gives at most %u", peaks, f->layout,
                 DEPZ_TMF8829_MAX_PEAKS);
        return -1;
    }
    /* [noise u16] [xtalk u16] then per peak: distance u16, snr u8 [, signal u16] */
    item = (noise ? 2u : 0u) + (xtalk ? 2u : 0u) + peaks * (3u + (sig ? 2u : 0u));
    n = rows * cols;
    data_size = f->size - DEPZ_TMF8829_FRAME_HEAD - DEPZ_TMF8829_FOOTER_SIZE;
    if (item * n != data_size || f->size > len) {
        snprintf(err, err_cap, "result data %zu B, expected %u px \xC3\x97 %zu B (layout 0x%02X)", data_size, n,
                 item, f->layout);
        return -1;
    }
    out->peaks = (uint8_t)peaks;
    out->rows = (uint16_t)rows;
    out->cols = (uint16_t)cols;
    out->has_signal = sig && peaks > 0;
    out->has_noise = noise;
    out->has_xtalk = xtalk;
    for (i = 0; i < n; i++) {
        const uint8_t *px = raw + DEPZ_TMF8829_FRAME_HEAD + (size_t)i * item;
        if (noise) { out->noise[i] = rd16(px); px += 2; }
        if (xtalk) { out->xtalk[i] = rd16(px); px += 2; }
        for (p = 0; p < peaks; p++) {
            out->distance[p * n + i] = rd16(px);
            out->snr[p * n + i] = px[2];
            px += 3;
            if (sig) { out->signal[p * n + i] = rd16(px); px += 2; }
        }
    }
    return 0;
}

int depz_tmf8829_merge_halves(const depz_tmf8829_results *a, const depz_tmf8829_results *b,
                              depz_tmf8829_results *out)
{
    unsigned rows = a->rows, cols = a->cols, half = rows * cols, full = 2 * half, p, r;
    if (a->peaks != b->peaks || a->rows != b->rows || a->cols != b->cols ||
        2u * rows * cols > DEPZ_TMF8829_MAX_ZONES)
        return -1;
    out->peaks = a->peaks;
    out->rows = (uint16_t)(2 * rows);
    out->cols = (uint16_t)cols;
    out->has_signal = a->has_signal && b->has_signal;
    out->has_noise = a->has_noise && b->has_noise;
    out->has_xtalk = a->has_xtalk && b->has_xtalk;
    for (r = 0; r < rows; r++) {
        size_t ev = (size_t)(2 * r) * cols, od = (size_t)(2 * r + 1) * cols, src = (size_t)r * cols;
        for (p = 0; p < a->peaks; p++) {
            memcpy(&out->distance[p * full + ev], &a->distance[p * half + src], cols * sizeof out->distance[0]);
            memcpy(&out->distance[p * full + od], &b->distance[p * half + src], cols * sizeof out->distance[0]);
            memcpy(&out->snr[p * full + ev], &a->snr[p * half + src], cols);
            memcpy(&out->snr[p * full + od], &b->snr[p * half + src], cols);
            if (out->has_signal) {
                memcpy(&out->signal[p * full + ev], &a->signal[p * half + src], cols * sizeof out->signal[0]);
                memcpy(&out->signal[p * full + od], &b->signal[p * half + src], cols * sizeof out->signal[0]);
            }
        }
        if (out->has_noise) {
            memcpy(&out->noise[ev], &a->noise[src], cols * sizeof out->noise[0]);
            memcpy(&out->noise[od], &b->noise[src], cols * sizeof out->noise[0]);
        }
        if (out->has_xtalk) {
            memcpy(&out->xtalk[ev], &a->xtalk[src], cols * sizeof out->xtalk[0]);
            memcpy(&out->xtalk[od], &b->xtalk[src], cols * sizeof out->xtalk[0]);
        }
    }
    return 0;
}

/* ── clock model ───────────────────────────────────────────────────────── */

void depz_tmf8829_clock_reset(depz_tmf8829_clock *c)
{
    memset(c, 0, sizeof *c);
    c->ratio_q16 = DEPZ_TMF8829_NOMINAL_Q16;
    c->nominal = true;
}

uint8_t depz_tmf8829_clock_add(depz_tmf8829_clock *c, int64_t us, uint32_t tick)
{
    uint8_t flags = 0;
    int64_t span;
    if (c->valid) {
        int64_t d_us = us - c->last_us;
        int64_t expect = (int64_t)(uint32_t)(tick - c->last_tick) * 8;
        int64_t tol = floor_div(d_us, 10) + 50;
        if (!(expect <= d_us + tol && expect + tol >= d_us)) {
            c->valid = false;
            flags = DEPZ_TMF8829_FT_CLOCK_RESET;
        }
    }
    c->last_us = us;
    c->last_tick = tick;
    if (!c->valid) {
        c->valid = true;
        c->anchor_us = us;
        c->anchor_tick = tick;
        c->has_mid = false;
        c->ratio_q16 = DEPZ_TMF8829_NOMINAL_Q16;
        c->nominal = true;
        return flags;
    }
    span = us - c->anchor_us;
    if (span >= DEPZ_TMF8829_WIN_MIN_US) {
        uint32_t dt = (uint32_t)(tick - c->anchor_tick);
        /* dt == 0 over >= 200 ms would have failed the step check above. */
        if (dt) {
            c->ratio_q16 = (uint32_t)(((uint64_t)span << 16) / dt);
            c->nominal = false;
        }
    }
    if (!c->has_mid && span >= DEPZ_TMF8829_WIN_SLIDE_US) {
        c->has_mid = true;
        c->mid_us = us;
        c->mid_tick = tick;
    } else if (c->has_mid && span >= 2 * DEPZ_TMF8829_WIN_SLIDE_US) {
        c->anchor_us = c->mid_us;
        c->anchor_tick = c->mid_tick;
        c->has_mid = false;
    }
    return flags;
}

int64_t depz_tmf8829_clock_to_us(const depz_tmf8829_clock *c, int64_t pair_us, uint32_t pair_tick, uint32_t tick)
{
    return pair_us + floor_shr16(s32((uint32_t)(tick - pair_tick)) * (int64_t)c->ratio_q16);
}

double depz_tmf8829_clock_ratio(const depz_tmf8829_clock *c)
{
    return (double)c->ratio_q16 / (double)DEPZ_TMF8829_NOMINAL_Q16;
}

/* ── capture time ──────────────────────────────────────────────────────── */

int depz_tmf8829_timer_results_checked(depz_tmf8829_timer *t, const depz_tmf8829_clock *c,
                                       unsigned fp_mode, uint32_t systick, uint32_t t0_tick,
                                       uint32_t t1_tick, int64_t pair_us, int64_t edge_us,
                                       uint8_t flags, depz_tmf8829_frame_time *out)
{
    if (fp_mode > 0xFFu || pair_us < 0 || edge_us < 0)
        return -1; /* fp_mode is a byte, MCU µs times are u64 (contract 07 §3) */
    depz_tmf8829_timer_results(t, c, fp_mode, systick, t0_tick, t1_tick, pair_us, edge_us, flags, out);
    return 0;
}

unsigned depz_tmf8829_subints(unsigned fp_mode)
{
    static const uint8_t N[6] = {2, 2, 2, 4, 4, 6};
    return fp_mode < 6 ? N[fp_mode] : 2;
}

void depz_tmf8829_timer_reset(depz_tmf8829_timer *t) { memset(t, 0, sizeof *t); }

int depz_tmf8829_clock_add_checked(depz_tmf8829_clock *c, int64_t us, uint32_t tick, uint8_t *flags)
{
    if (us < 0)
        return -1; /* an MCU µs time is a u64 (contract 07 §3) */
    *flags = depz_tmf8829_clock_add(c, us, tick);
    return 0;
}

void depz_tmf8829_timer_histograms(depz_tmf8829_timer *t) { t->hist_seen = true; }

void depz_tmf8829_timer_results(depz_tmf8829_timer *t, const depz_tmf8829_clock *c, unsigned fp_mode,
                                uint32_t systick, uint32_t t0_tick, uint32_t t1_tick, int64_t pair_us,
                                int64_t edge_us, uint8_t flags, depz_tmf8829_frame_time *out)
{
    int64_t t0, t1;
    uint32_t n1, sub, t1_len;
    if (c->nominal) flags |= DEPZ_TMF8829_FT_NOMINAL;
    t0 = depz_tmf8829_clock_to_us(c, pair_us, systick, t0_tick);
    t1 = depz_tmf8829_clock_to_us(c, pair_us, systick, t1_tick);

    if (edge_us != 0 && !t->hist_seen) {
        int64_t lag = edge_us - t1;
        if (lag <= 0 || (t->has_lag_min && lag > t->lag_min + DEPZ_TMF8829_EDGE_LATE_US)) {
            flags |= DEPZ_TMF8829_FT_EDGE_LATE;
        } else if (!t->has_lag_min || lag < t->lag_min) {
            t->lag_min = lag;
            t->has_lag_min = true;
        }
    }

    n1 = depz_tmf8829_subints(fp_mode) - 1;
    sub = (uint32_t)(t1_tick - t0_tick) / n1;
    if (!t->sub_min || sub < t->sub_min) t->sub_min = sub;
    if ((uint64_t)sub > (uint64_t)t->sub_min + 1) flags |= DEPZ_TMF8829_FT_STALLED;
    if (t->hist_seen && n1 > 1) flags |= DEPZ_TMF8829_FT_T1_UNSURE;

    t1_len = (uint32_t)(((uint64_t)t->sub_min * c->ratio_q16) >> 16);
    out->timestamp_us = floor_div(t0 + t1 + (int64_t)t1_len, 2);
    out->flags = flags;
    out->t1_len_us = t1_len;
    out->t0_us = t0;
    out->t1_us = t1;
    out->edge_us = edge_us;
    out->pair_us = pair_us;
    out->ratio_q16 = c->ratio_q16;
}

/* ── measurement assembly ──────────────────────────────────────────────── */

void depz_tmf8829_builder_reset(depz_tmf8829_builder *b)
{
    depz_tmf8829_clock_reset(&b->clock);
    depz_tmf8829_timer_reset(&b->timer);
    b->parse_errors = 0;
    b->pending_hist = b->pending_ref = b->hist = b->refs = 0;
    b->has_half = false;
    b->n_problems = 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void log_problem(depz_tmf8829_builder *b, const char *fmt, ...)
{
    if (b->n_problems < DEPZ_TMF8829_PROBLEMS_KEPT) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(b->problems[b->n_problems], DEPZ_TMF8829_PROBLEM_LEN, fmt, ap);
        va_end(ap);
    }
    b->n_problems++;
}

int depz_tmf8829_builder_add(depz_tmf8829_builder *b, const uint8_t *raw, size_t len, int64_t timestamp_us,
                             int64_t edge_us, depz_tmf8829_measurement *m)
{
    depz_tmf8829_frame_info f;
    depz_tmf8829_frame_problems bad;
    depz_tmf8829_frame_time t;
    char err[160];
    uint8_t flags;
    size_t i, n;
    double ratio;

    if (timestamp_us < 0 || edge_us < 0)
        return -1; /* MCU µs times are u64 (contract 07 §3): checked before the frame */

    if (depz_tmf8829_parse_frame(raw, len, &f)) {
        b->parse_errors++;
        log_problem(b, "frame of %zu B is shorter than header + footer", len);
        return 0;
    }
    if (f.kind == DEPZ_TMF8829_FID_REF_SPAD) {
        b->pending_ref++; /* zero pre-header: no clock pair */
        return 0;
    }
    if (f.kind != DEPZ_TMF8829_FID_RESULTS && f.kind != DEPZ_TMF8829_FID_HISTOGRAMS) {
        b->parse_errors++;
        log_problem(b, "frame id 0x%02X: unknown kind", f.id);
        return 0;
    }
    flags = depz_tmf8829_clock_add(&b->clock, timestamp_us, f.systick);
    depz_tmf8829_frame_problems_of(&f, &bad);
    if (bad.count) {
        char joined[DEPZ_TMF8829_PROBLEM_LEN];
        size_t at = 0;
        int k;
        joined[0] = '\0';
        for (k = 0; k < bad.count && at < sizeof joined; k++) {
            int w = snprintf(joined + at, sizeof joined - at, "%s%s", k ? ", " : "", bad.text[k]);
            if (w < 0) break;
            at += (size_t)w;
        }
        log_problem(b, "frame id=0x%02X fN=%lu %zu B: %s", f.id, (unsigned long)f.f_number, f.size, joined);
    }
    if (f.kind == DEPZ_TMF8829_FID_HISTOGRAMS) {
        depz_tmf8829_timer_histograms(&b->timer);
        b->pending_hist++;
        return 0;
    }

    depz_tmf8829_timer_results(&b->timer, &b->clock, f.fp_mode, f.systick, f.t0_integration, f.t1_integration,
                               timestamp_us, edge_us, flags, &t);
    b->hist += b->pending_hist;
    b->refs += b->pending_ref;
    b->pending_hist = b->pending_ref = 0;
    if (depz_tmf8829_parse_results(raw, len, &f, &b->cur, err, sizeof err)) {
        b->parse_errors++;
        log_problem(b, "fN=%lu: %s", (unsigned long)f.f_number, err);
        b->has_half = false;
        return 0;
    }

    if (FP_GRID[f.fp_mode].rows == 32) {
        if (!b->has_half) {
            if (f.second_half) {
                log_problem(b, "second half fN=%lu without a first one - dropped", (unsigned long)f.f_number);
                b->hist = b->refs = 0;
                return 0;
            }
            b->has_half = true;
            b->half_info = f;
            b->half_time = t;
            memcpy(&b->half, &b->cur, sizeof b->half);
            return 0;
        }
        if (!f.second_half) {
            log_problem(b, "two first halves in a row (fN %lu, %lu)", (unsigned long)b->half_info.f_number,
                        (unsigned long)f.f_number);
            b->half_info = f;
            b->half_time = t;
            memcpy(&b->half, &b->cur, sizeof b->half);
            return 0;
        }
        b->has_half = false;
        if (b->half_info.fp_mode != f.fp_mode ||
            (b->half_info.layout & ~DEPZ_TMF8829_LAYOUT_SUBFRAME) != (f.layout & ~DEPZ_TMF8829_LAYOUT_SUBFRAME) ||
            depz_tmf8829_merge_halves(&b->half, &b->cur, &m->results)) {
            /* Impossible within one stream (configure() needs a stop); never crash the reader. */
            log_problem(b, "half-frames of different formats (fN %lu, %lu) - dropped",
                        (unsigned long)b->half_info.f_number, (unsigned long)f.f_number);
            b->hist = b->refs = 0;
            return 0;
        }
        m->n_frames = 2;
        m->frames[0] = b->half_info;
        m->frames[1] = f;
        m->times[0] = b->half_time;
        m->times[1] = t;
    } else {
        memcpy(&m->results, &b->cur, sizeof m->results);
        m->n_frames = 1;
        m->frames[0] = f;
        m->times[0] = t;
    }

    {
        int64_t sum = 0;
        uint8_t fl = 0;
        for (i = 0; i < m->n_frames; i++) {
            sum += m->times[i].timestamp_us;
            fl |= m->times[i].flags;
        }
        m->timestamp_us = floor_div(sum, (int64_t)m->n_frames);
        m->flags = fl;
    }
    m->fp_mode = f.fp_mode;
    m->f_number = f.f_number;
    m->rows = FP_GRID[f.fp_mode].rows;
    m->cols = FP_GRID[f.fp_mode].cols;
    m->temperature_c = f.temperature[0];
    m->distance_in_mm = f.distance_in_mm;
    m->ratio_q16 = b->clock.ratio_q16;
    ratio = depz_tmf8829_clock_ratio(&b->clock);
    n = (size_t)m->results.peaks * m->results.rows * m->results.cols;
    for (i = 0; i < n; i++) {
        double d = (double)m->results.distance[i] * ratio;
        m->distance_mm[i] = m->distance_in_mm ? d : d / 4.0;
    }
    m->n_histograms = b->hist;
    m->n_ref_frames = b->refs;
    m->n_problems = b->n_problems;
    n = b->n_problems < DEPZ_TMF8829_PROBLEMS_KEPT ? b->n_problems : DEPZ_TMF8829_PROBLEMS_KEPT;
    for (i = 0; i < n; i++) memcpy(m->problems[i], b->problems[i], DEPZ_TMF8829_PROBLEM_LEN);
    b->hist = b->refs = 0;
    b->n_problems = 0;
    return 1;
}

/* ── presets, config page ──────────────────────────────────────────────── */

const depz_tmf8829_preset DEPZ_TMF8829_PRESETS[DEPZ_TMF8829_PRESET_COUNT] = {
    {"8x8", 0x40, 586},
    {"8x8-lr", 0x41, 586},  /* long range */
    {"8x8-ha", 0x42, 586},  /* high accuracy */
    {"16x16", 0x43, 586},
    {"16x16-ha", 0x44, 586},
    {"32x32", 0x45, 2344},
    {"32x32-ha", 0x46, 2344},
    {"48x32", 0x47, 3516},
    {"48x32-ha", 0x48, 3516},
    {"8x8-xr", 0x49, 586},  /* extended range, distances in mm */
};

const depz_tmf8829_preset *depz_tmf8829_find_preset(const char *name)
{
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < DEPZ_TMF8829_PRESET_COUNT; i++)
        if (!strcmp(DEPZ_TMF8829_PRESETS[i].name, name)) return &DEPZ_TMF8829_PRESETS[i];
    return NULL;
}

const depz_tmf8829_cfg_field DEPZ_TMF8829_CFG_FIELDS[DEPZ_TMF8829_CFG_FIELD_COUNT] = {
    {"period_ms", 0x22, 0, 16},
    {"kilo_iterations", 0x24, 0, 16},
    {"fp_mode", 0x26, 0, 3},
    {"nr_peaks", 0x2A, 0, 3},
    {"signal_strength", 0x2A, 3, 1},
    {"noise_strength", 0x2A, 4, 1},
    {"xtalk", 0x2A, 5, 1},
    {"dump_histograms", 0x2B, 0, 1},
    {"ref_spad_frame", 0x2C, 0, 1},
    {"power_modes", 0x2E, 0, 8},
    {"ha_kilo_iterations", 0x4A, 0, 16},
    {"dual_mode", 0x4C, 0, 2},
    {"fov_correction", 0x78, 0, 4},
    {"distance_in_mm", 0x8F, 0, 1},
};

const depz_tmf8829_cfg_field *depz_tmf8829_find_cfg_field(const char *name)
{
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < DEPZ_TMF8829_CFG_FIELD_COUNT; i++)
        if (!strcmp(DEPZ_TMF8829_CFG_FIELDS[i].name, name)) return &DEPZ_TMF8829_CFG_FIELDS[i];
    return NULL;
}

/* The field's first byte in a page of `page_len` bytes, or -3 when the page
 * is too short to hold the field (-1 for an unknown name). */
static int cfg_index(const char *name, size_t page_len, const depz_tmf8829_cfg_field **out)
{
    const depz_tmf8829_cfg_field *f = depz_tmf8829_find_cfg_field(name);
    size_t i;
    if (!f) return -1;
    i = (size_t)(f->reg - DEPZ_TMF8829_REG_CFG);
    if (page_len < i + (f->width == 16 ? 2u : 1u)) return -3;
    *out = f;
    return (int)i;
}

int depz_tmf8829_cfg_get_n(const uint8_t *page, size_t page_len, const char *name, uint32_t *value)
{
    const depz_tmf8829_cfg_field *f = NULL;
    int i = cfg_index(name, page_len, &f);
    if (i < 0) return i;
    if (f->width == 16) *value = (uint32_t)page[i] | (uint32_t)page[i + 1] << 8;
    else *value = (uint32_t)(page[i] >> f->shift) & ((1u << f->width) - 1u);
    return 0;
}

int depz_tmf8829_cfg_set_n(uint8_t *page, size_t page_len, const char *name, int64_t value)
{
    const depz_tmf8829_cfg_field *f = NULL;
    int i = cfg_index(name, page_len, &f);
    if (i < 0) return i;
    if (value < 0 || value >= ((int64_t)1 << f->width)) return -2;
    if (f->width == 16) {
        page[i] = (uint8_t)value;
        page[i + 1] = (uint8_t)(value >> 8);
    } else {
        unsigned mask = ((1u << f->width) - 1u) << f->shift;
        page[i] = (uint8_t)((page[i] & ~mask) | ((unsigned)value << f->shift));
    }
    return 0;
}

int depz_tmf8829_cfg_get(const uint8_t page[190], const char *name, uint32_t *value)
{
    return depz_tmf8829_cfg_get_n(page, 190, name, value);
}

int depz_tmf8829_cfg_set(uint8_t page[190], const char *name, int64_t value)
{
    return depz_tmf8829_cfg_set_n(page, 190, name, value);
}

const char *depz_tmf8829_status_name(uint8_t st)
{
    static const char *const NAMES[] = {
        "OK", "ACCEPTED", "ERR_CONFIG", "ERR_APPLICATION", "ERR_CONFIG_RESULT_SIZE", "ERR_CONFIG_VCSEL",
        "ERR_WAKEUP_TIMED", "ERR_RESET_UNEXPECTED", "ERR_UNKNOWN_CMD", "ERR_UNKNOWN_CID", "ERR_STOP_0",
        "ERR_STOP_1", "ERR_STOP_2", "ERR_STOP_3", "ERR_OSC_TUNE",
    };
    return st < sizeof NAMES / sizeof NAMES[0] ? NAMES[st] : NULL;
}
