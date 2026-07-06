/*
 * BNO086 SHTP framing (contract 05 §3) + SH-2 control encoders (§6).
 * Pure codec, no I/O. Byte-exact port of the TS/Python `shtp` reference.
 */
#include "depz_sensor_sdk.h"
#include <stdlib.h>
#include <string.h>

static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* ---- header codec ----------------------------------------------------------*/
void depz_shtp_pack_header(const depz_shtp_header *hdr, uint8_t out[4])
{
    uint16_t word = (uint16_t)((hdr->length & DEPZ_SHTP_LENGTH_MASK) |
                               (hdr->continuation ? DEPZ_SHTP_CONTINUATION : 0));
    put_u16(out, word);
    out[2] = hdr->channel;
    out[3] = hdr->seq;
}

void depz_shtp_unpack_header(const uint8_t in[4], depz_shtp_header *out)
{
    uint16_t word = rd_u16(in);
    out->length = (uint16_t)(word & DEPZ_SHTP_LENGTH_MASK);
    out->channel = in[2];
    out->seq = in[3];
    out->continuation = (word & DEPZ_SHTP_CONTINUATION) != 0;
}

/* ---- reassembly layer ------------------------------------------------------*/
void depz_shtp_init(depz_shtp_layer *l)
{
    memset(l, 0, sizeof(*l));
}

void depz_shtp_free(depz_shtp_layer *l)
{
    for (int c = 0; c < DEPZ_SHTP_NUM_CHANNELS; c++) {
        free(l->rx[c].buf);
        l->rx[c].buf = NULL;
        l->rx[c].cap = 0;
    }
}

static int rx_reserve(depz_shtp_rx_channel *rx, size_t need)
{
    if (need <= rx->cap)
        return 0;
    size_t cap = rx->cap ? rx->cap : 64;
    while (cap < need) cap *= 2;
    uint8_t *nb = (uint8_t *)realloc(rx->buf, cap);
    if (!nb)
        return -1;
    rx->buf = nb;
    rx->cap = cap;
    return 0;
}

size_t depz_shtp_next_frame(depz_shtp_layer *l, uint8_t channel,
                            const uint8_t *payload, size_t payload_len,
                            uint8_t *out, size_t out_cap)
{
    if (channel >= DEPZ_SHTP_NUM_CHANNELS)
        return 0;
    if (DEPZ_SHTP_HEADER_SIZE + payload_len > DEPZ_SHTP_MAX_TX_FRAME)
        return 0;
    size_t total = DEPZ_SHTP_HEADER_SIZE + payload_len;
    if (out_cap < total)
        return 0;
    depz_shtp_header hdr = {
        .length = (uint16_t)total,
        .channel = channel,
        .seq = l->tx_seq[channel],
        .continuation = false,
    };
    depz_shtp_pack_header(&hdr, out);
    if (payload_len)
        memcpy(out + DEPZ_SHTP_HEADER_SIZE, payload, payload_len);
    l->tx_seq[channel] = (uint8_t)(l->tx_seq[channel] + 1);
    return total;
}

int depz_shtp_feed(depz_shtp_layer *l, const uint8_t *frame, size_t frame_len,
                   depz_shtp_cargo *out)
{
    if (frame_len < DEPZ_SHTP_HEADER_SIZE)
        return 0;
    depz_shtp_header hdr;
    depz_shtp_unpack_header(frame, &hdr);
    if (hdr.channel >= DEPZ_SHTP_NUM_CHANNELS || hdr.length < DEPZ_SHTP_HEADER_SIZE)
        return 0; /* empty/padding header or junk */

    const uint8_t *chunk = frame + DEPZ_SHTP_HEADER_SIZE;
    size_t chunk_len = frame_len - DEPZ_SHTP_HEADER_SIZE;
    depz_shtp_rx_channel *rx = &l->rx[hdr.channel];

    if (!hdr.continuation) {
        if (rx->expected != 0 && rx->received != 0)
            l->discarded++;
        if (rx_reserve(rx, chunk_len ? chunk_len : 1) != 0)
            return -1;
        memcpy(rx->buf, chunk, chunk_len);
        rx->received = chunk_len;
        rx->expected = (size_t)hdr.length - DEPZ_SHTP_HEADER_SIZE;
        rx->seq = hdr.seq;
        rx->active = true;
    } else {
        if (rx->expected == 0) {
            l->discarded++;
            return 0; /* orphan continuation */
        }
        if (rx_reserve(rx, rx->received + chunk_len) != 0)
            return -1;
        memcpy(rx->buf + rx->received, chunk, chunk_len);
        rx->received += chunk_len;
    }

    if (rx->received < rx->expected)
        return 0;
    if (rx->received > rx->expected) {
        l->discarded++;
        rx->received = 0;
        rx->expected = 0;
        rx->active = false;
        return 0;
    }

    out->channel = hdr.channel;
    out->seq = rx->seq;
    out->payload = rx->buf;
    out->payload_len = rx->received;
    rx->received = 0;
    rx->expected = 0;
    rx->active = false;
    return 1;
}

/* ---- SH-2 control encoders -------------------------------------------------*/
size_t depz_bno_pack_set_feature(uint8_t sensor_id, uint8_t flags,
                                 uint16_t sensitivity, uint32_t interval_us,
                                 uint32_t batch_us, uint32_t cfg_word,
                                 uint8_t out[17])
{
    out[0] = 0xFD;
    out[1] = sensor_id;
    out[2] = flags;
    put_u16(out + 3, sensitivity);
    put_u32(out + 5, interval_us);
    put_u32(out + 9, batch_us);
    put_u32(out + 13, cfg_word);
    return 17;
}

size_t depz_bno_pack_get_feature_request(uint8_t sensor_id, uint8_t out[2])
{
    out[0] = 0xFE;
    out[1] = sensor_id;
    return 2;
}

size_t depz_bno_pack_product_id_request(uint8_t out[2])
{
    out[0] = 0xF9;
    out[1] = 0x00;
    return 2;
}

size_t depz_bno_pack_command_request(uint8_t seq, uint8_t command,
                                     const uint8_t *params, size_t nparams,
                                     uint8_t out[12])
{
    memset(out, 0, 12);
    out[0] = 0xF2;
    out[1] = seq;
    out[2] = command;
    if (nparams > 9)
        nparams = 9;
    if (nparams && params)
        memcpy(out + 3, params, nparams);
    return 12;
}

size_t depz_bno_pack_frs_read_request(uint16_t frs_type, uint16_t offset_words,
                                      uint16_t block_words, uint8_t out[8])
{
    out[0] = 0xF4;
    out[1] = 0x00;
    put_u16(out + 2, offset_words);
    put_u16(out + 4, frs_type);
    put_u16(out + 6, block_words);
    return 8;
}

size_t depz_bno_pack_frs_write_request(uint16_t frs_type, uint16_t length_words,
                                       uint8_t out[6])
{
    out[0] = 0xF7;
    out[1] = 0x00;
    put_u16(out + 2, length_words);
    put_u16(out + 4, frs_type);
    return 6;
}

size_t depz_bno_pack_frs_write_data(uint16_t offset_words, const uint32_t *words,
                                    size_t nwords, uint8_t *out, size_t out_cap)
{
    size_t total = 4 + 4 * nwords;
    if (out_cap < total)
        return 0;
    out[0] = 0xF6;
    out[1] = 0x00;
    put_u16(out + 2, offset_words);
    for (size_t i = 0; i < nwords; i++)
        put_u32(out + 4 + 4 * i, words[i]);
    return total;
}
