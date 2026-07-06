/*
 * VL53L8 register-bridge streaming: frame reassembly + raw-results decode
 * (contract 04). Byte-exact port of the reference decode path in the TS/Python
 * ULD `parseFrame` (block-header walk over a 32-bit-word-swapped buffer).
 *
 * SHARED CX/CH: this ranging-frame decoder serves BOTH ToF variants —
 * VL53L8CX (base) and VL53L8CH (CX + CNH). They emit the same ranging-results
 * layout, so there is a single decode path (no per-variant duplication).
 *
 * OUT OF SCOPE (hardware-dependent extension points, not implemented):
 *   - the live ULD init/config register-bridge driver (firmware download +
 *     DCI register sequences) — see the TS/Python `uld` reference;
 *   - CNH compact-network-histogram decode — the CH-specific extension point
 *     (see the DEPZ_VL53L8_CNH_* stub note below).
 * Only the verifiable, shared decode path lives here.
 */
#include "depz_sensor_sdk.h"
#include <string.h>

/* ---- little-endian readers -------------------------------------------------*/
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
static int16_t rd_i16(const uint8_t *p) { return (int16_t)rd_u16(p); }

static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* ---- encoders --------------------------------------------------------------*/
size_t depz_vl53l8_pack_read_reg(uint16_t addr, uint16_t length, uint8_t *out)
{
    put_u16(out, addr);
    put_u16(out + 2, length);
    return 4;
}

size_t depz_vl53l8_pack_start_stream(uint16_t frame_size, uint8_t *out)
{
    put_u16(out, frame_size);
    return 2;
}

/* ---- chunk / reg-data unpack -----------------------------------------------*/
int depz_vl53l8_unpack_chunk(const uint8_t *payload, size_t len,
                             depz_vl53l8_chunk *out)
{
    if (len < 12)
        return -1;
    out->timestamp_us = rd_u64(payload);
    out->full_size = rd_u16(payload + 8);
    out->offset = rd_u16(payload + 10);
    out->data = payload + 12;
    out->data_len = len - 12;
    return 0;
}

int depz_vl53l8_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_vl53l8_reg_data *out)
{
    if (len < 9)
        return -1;
    out->cmd = payload[0];
    out->timestamp_us = rd_u64(payload + 1);
    out->data = payload + 9;
    out->data_len = len - 9;
    return 0;
}

/* ---- reassembler -----------------------------------------------------------*/
void depz_vl53l8_reasm_init(depz_vl53l8_reassembler *r)
{
    memset(r, 0, sizeof(*r));
}

static void reasm_reset(depz_vl53l8_reassembler *r)
{
    r->len = 0;
    r->full_size = 0;
}

int depz_vl53l8_reasm_feed(depz_vl53l8_reassembler *r, const depz_vl53l8_chunk *c,
                           const uint8_t **frame, size_t *frame_len, uint64_t *ts)
{
    if (c->offset == 0) {
        if (r->len > 0 && r->len != r->full_size)
            r->discarded++;
        reasm_reset(r);
        if (c->full_size > DEPZ_VL53L8_STREAM_TOTAL_MAX ||
            c->data_len > sizeof(r->buf)) {
            r->discarded++;
            return 0;
        }
        memcpy(r->buf, c->data, c->data_len);
        r->len = c->data_len;
        r->full_size = c->full_size;
        r->timestamp_us = c->timestamp_us;
    } else if (c->offset == r->len && r->full_size == c->full_size && r->len > 0 &&
               r->len + c->data_len <= sizeof(r->buf)) {
        memcpy(r->buf + r->len, c->data, c->data_len);
        r->len += c->data_len;
    } else {
        if (r->len > 0)
            r->discarded++;
        reasm_reset(r);
        return 0;
    }

    if (r->len == r->full_size && r->full_size > 0) {
        *frame = r->buf;
        *frame_len = r->len;
        *ts = r->timestamp_us;
        r->completed++;
        reasm_reset(r);
        return 1;
    }
    if (r->len > r->full_size) {
        r->discarded++;
        reasm_reset(r);
    }
    return 0;
}

/* ---- frame decode ----------------------------------------------------------*/

/* VL53L8CX_SwapBuffer: byte-reverse every complete 32-bit word (tail as-is). */
static void swap_buffer(uint8_t *d, size_t n)
{
    size_t n4 = (n >> 2) << 2;
    for (size_t i = 0; i < n4; i += 4) {
        uint8_t t0 = d[i], t1 = d[i + 1];
        d[i] = d[i + 3];
        d[i + 1] = d[i + 2];
        d[i + 2] = t1;
        d[i + 3] = t0;
    }
}

/* Block-header index constants (NB_TARGET_PER_ZONE == 1). */
#define BH_METADATA_IDX 0x54b4u
#define BH_SPAD_IDX     0x55d0u
#define BH_AMBIENT_IDX  0x54d0u
#define BH_NBT_IDX      0xdb84u
#define BH_SIGNAL_IDX   0xdbc4u
#define BH_SIGMA_IDX    0xdec4u
#define BH_DISTANCE_IDX 0xdf44u
#define BH_REFL_IDX     0xe044u
#define BH_STATUS_IDX   0xe084u

/* floor division toward negative infinity (distances can be negative). */
static int32_t floordiv4(int32_t v)
{
    return (v >= 0) ? (v / 4) : -(((-v) + 3) / 4);
}

int depz_vl53l8_decode_frame(const uint8_t *raw, size_t raw_len,
                             uint64_t timestamp_us, depz_vl53l8_frame *out)
{
    if (raw_len < 24 || raw_len > DEPZ_VL53L8_STREAM_TOTAL_MAX)
        return -2;

    uint8_t buf[DEPZ_VL53L8_STREAM_TOTAL_MAX];
    memcpy(buf, raw, raw_len);
    swap_buffer(buf, raw_len);

    memset(out, 0, sizeof(*out));
    out->timestamp_us = timestamp_us;

    /* zone counts we actually decoded (for arrays sized differently) */
    int dist_n = 0, status_n = 0, nbt_n = 0;
    int sig_n = 0, amb_n = 0, spad_n = 0, sigma_n = 0, refl_n = 0;

    size_t i = 16;
    while (i + 4 <= raw_len) {
        uint32_t bh = rd_u32(buf + i);
        uint32_t type = bh & 0xf;
        uint32_t size = (bh >> 4) & 0xfff;
        uint32_t idx = (bh >> 16) & 0xffff;
        size_t msize = (type > 0x1 && type < 0xd) ? (size_t)type * size : size;
        if (i + 4 + msize > raw_len)
            break; /* reached the footer; all data blocks precede it */

        const uint8_t *body = buf + i + 4;
        if (idx == BH_METADATA_IDX) {
            out->silicon_temp_degc = (int8_t)buf[i + 12];
        } else if (idx == BH_DISTANCE_IDX) {
            dist_n = (int)(msize / 2);
            if (dist_n > DEPZ_VL53L8_MAX_ZONES) dist_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < dist_n; k++)
                out->distance_mm[k] = rd_i16(body + 2 * k);
        } else if (idx == BH_STATUS_IDX) {
            status_n = (int)msize;
            if (status_n > DEPZ_VL53L8_MAX_ZONES) status_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < status_n; k++) out->target_status[k] = body[k];
        } else if (idx == BH_NBT_IDX) {
            nbt_n = (int)msize;
            if (nbt_n > DEPZ_VL53L8_MAX_ZONES) nbt_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < nbt_n; k++) out->nb_target_detected[k] = body[k];
        } else if (idx == BH_SIGNAL_IDX) {
            sig_n = (int)(msize / 4);
            if (sig_n > DEPZ_VL53L8_MAX_ZONES) sig_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < sig_n; k++) out->signal_per_spad[k] = rd_u32(body + 4 * k);
        } else if (idx == BH_AMBIENT_IDX) {
            amb_n = (int)(msize / 4);
            if (amb_n > DEPZ_VL53L8_MAX_ZONES) amb_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < amb_n; k++) out->ambient_per_spad[k] = rd_u32(body + 4 * k);
        } else if (idx == BH_SPAD_IDX) {
            spad_n = (int)(msize / 4);
            if (spad_n > DEPZ_VL53L8_MAX_ZONES) spad_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < spad_n; k++) out->nb_spads_enabled[k] = rd_u32(body + 4 * k);
        } else if (idx == BH_SIGMA_IDX) {
            sigma_n = (int)(msize / 2);
            if (sigma_n > DEPZ_VL53L8_MAX_ZONES) sigma_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < sigma_n; k++) out->range_sigma_mm_raw[k] = rd_u16(body + 2 * k);
        } else if (idx == BH_REFL_IDX) {
            refl_n = (int)msize;
            if (refl_n > DEPZ_VL53L8_MAX_ZONES) refl_n = DEPZ_VL53L8_MAX_ZONES;
            for (int k = 0; k < refl_n; k++) out->reflectance[k] = body[k];
        }

        i += msize + 4;
    }

    /* ST GetRangingData fixed-point scaling: distance_mm = raw / 4 (floored). */
    for (int k = 0; k < dist_n; k++)
        out->distance_mm[k] = floordiv4(out->distance_mm[k]);

    /* Zone count = nb_target_detected block length (16 for 4x4, 64 for 8x8). */
    int nzones = nbt_n;
    out->resolution = nzones;

    /* No target detected -> status 255. */
    for (int z = 0; z < nzones; z++) {
        if (out->nb_target_detected[z] == 0 && z < status_n)
            out->target_status[z] = 255;
    }
    (void)dist_n; (void)sig_n; (void)amb_n; (void)spad_n; (void)sigma_n; (void)refl_n;

    /* Header/footer id match check. The footer id sits at raw_len - N; for the
     * CX / shared CH ranging frame, N = DEPZ_VL53L8CX_FOOTER_ID_OFFSET (12).
     * A CH CNH histogram frame uses a different footer offset/layout and is not
     * decoded here (see the CNH extension-point note below). */
    if (buf[0x8] != buf[raw_len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET] ||
        buf[0x9] != buf[raw_len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET + 1])
        return -1;

    return 0;
}

/* ---- CNH (compact-network-histograms) — CH-specific decode -----------------
 *
 * VL53L8CH (DEPZ_VL53L8_VARIANT_CH) adds compact-network-histogram output on
 * top of the shared ranging frame decoded above. depz_vl53l8ch_decode_cnh()
 * is a byte-exact port of the ST ULD CNH plugin block-address math
 * (vl53lmz_cnh_get_block_addresses / _cnh_get_mem_block_addresses, ULD 2.0.16)
 * for the fixed cnh_cfg the DEPZ firmware uses (DISABLE_PING_PONG |
 * DISABLE_VARIANCE + ambient/xtalk/zero-invalid/ref-residual). It mirrors the
 * Python reference depz_sensor_sdk/vl53l8/cnh.py decode()/_decode_aggregate().
 *
 * Persistent-data header layout (32-bit words, buffer already in decode order):
 *   word[0] = buffer state (MI_STATE_PING == 0 for the active/ping buffer)
 *   word[1] = buffer info: bits[15:0] = ping-pong buffer size in words
 *   word[2] = ref-residual word (u32; float value = word / 2048.0)
 *   word[3] = flags (bit4 = 0x10 selects the second buffer)
 * The persistent header is CNH_PER_HEADER_WORDS (5) words; each ping/pong
 * buffer starts with a CNH_PER_BUFFER_HEADER_WORDS (2) word header
 * (state, nb_accumulated), then the data blocks:
 *   FEAT_INT   : int32[nb_agg*feat]            (histogram raw)
 *   FEAT_FRAC  : int8 [nb_agg*feat] (4-byte pad) (per-bin scaler)
 *   AMBIENT_INT: int32[nb_agg]
 *   AMBIENT_FRAC:int8 [nb_agg]      (4-byte pad)
 * With DISABLE_VARIANCE set there are no VARIANCE_* blocks. Aggregate `a`'s
 * histogram starts at element a*feat within FEAT_INT / FEAT_FRAC.
 * -------------------------------------------------------------------------- */

/* plugin_cnh.c / plugin_motion_indicator.h layout constants. */
#define CNH_PER_HEADER_WORDS            5
#define CNH_PER_BUFFER_HEADER_WORDS     2
#define CNH_PER_HEADER_BUFFER_INFO_IDX  1
#define CNH_PER_HEADER_FLAGS_IDX        3
#define CNH_BUFFER_INFO_WORDS_MASK      0xFFFFu
#define CNH_MI_STATE_PING               0
#define CNH_FLAGS_SELECT_BUFFER_BIT     0x10u

static int32_t rd_i32(const uint8_t *p) { return (int32_t)rd_u32(p); }

int depz_vl53l8ch_decode_cnh(const depz_vl53l8ch_cnh_config *cfg,
                             const uint8_t *raw, size_t raw_len,
                             depz_vl53l8ch_cnh_frame *out)
{
    int nb_agg = cfg->nb_of_aggregates;
    int feat = cfg->feature_length;
    if (nb_agg <= 0 || nb_agg > DEPZ_VL53L8_CNH_MAX_AGGREGATES ||
        feat <= 0 || feat > DEPZ_VL53L8_CNH_MAX_FEATURE)
        return -1;

    /* Need at least the 5-word persistent-data header. */
    if (raw_len < (size_t)CNH_PER_HEADER_WORDS * 4)
        return -2;

    int32_t agg_x_feat = nb_agg * feat;

    /* Persistent-data header words. */
    int32_t  state      = rd_i32(raw + 0 * 4);                              /* words[0] */
    uint32_t info       = rd_u32(raw + CNH_PER_HEADER_BUFFER_INFO_IDX * 4); /* words[1] */
    uint32_t flags_word = rd_u32(raw + CNH_PER_HEADER_FLAGS_IDX * 4);       /* words[3] */

    /* vl53lmz_cnh_get_ref_residual: raw u32 at word[2]. */
    out->ref_residual_word = rd_u32(raw + 2 * 4);

    uint32_t pp_size = info & CNH_BUFFER_INFO_WORDS_MASK;

    /* Select ping or pong buffer exactly as the C plugin does. With ping-pong
     * disabled the device reports a single buffer and this resolves to ping
     * (local_pp == 0 -> base does not skip the ping-pong region). */
    int local_pp = 1;
    if ((flags_word & CNH_FLAGS_SELECT_BUFFER_BIT) == CNH_FLAGS_SELECT_BUFFER_BIT)
        local_pp = 1;
    if (state == CNH_MI_STATE_PING)
        local_pp = 1 - local_pp;

    size_t base = CNH_PER_HEADER_WORDS;            /* in words */
    if (local_pp == 1)
        base += pp_size;
    /* Byte offset of the buffer data (after the 2-word buffer header). */
    size_t feat_int_off  = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4;
    size_t feat_frac_off = feat_int_off + (size_t)agg_x_feat * 4;
    size_t feat_frac_pad = ((size_t)(3 + agg_x_feat) / 4) * 4;
    size_t amb_int_off   = feat_frac_off + feat_frac_pad;
    size_t amb_frac_off  = amb_int_off + (size_t)nb_agg * 4;
    /* Everything through the last AMBIENT_FRAC byte must fit the raw block. */
    if (amb_frac_off + (size_t)nb_agg > raw_len)
        return -3;

    out->nb_aggregates = nb_agg;
    out->feature_length = feat;
    for (int agg = 0; agg < nb_agg; agg++) {
        int agg_off = agg * feat;
        for (int f = 0; f < feat; f++) {
            out->hist_raw[agg][f] =
                rd_i32(raw + feat_int_off + (size_t)(agg_off + f) * 4);
            out->hist_scaler[agg][f] =
                (int8_t)raw[feat_frac_off + (size_t)(agg_off + f)];
        }
    }
    return 0;
}
