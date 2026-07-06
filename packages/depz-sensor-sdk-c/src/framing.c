/* Packet framing and incremental parser (contract 01).
 *
 * Byte-exact with firmware common/transport/transport.c. The parser fixes the
 * reference host tool's empty-payload sizing bug (ERRATA E6): a packet whose
 * header advertises a payload CRC type but carries an empty payload has NO CRC
 * bytes on the wire.
 */
#include "depz_sensor_sdk.h"
#include <stdlib.h>
#include <string.h>

static size_t crc_size_for(depz_crc_type t)
{
    switch (t) {
    case DEPZ_CRC8:  return 1;
    case DEPZ_CRC16: return 2;
    case DEPZ_CRC32: return 4;
    default:         return 0;
    }
}

static size_t payload_crc_trailer(depz_crc_type t, const uint8_t *payload,
                                  size_t len, uint8_t *out)
{
    if (t == DEPZ_CRC_NONE || len == 0)
        return 0;
    if (t == DEPZ_CRC8) {
        out[0] = depz_crc8_maxim(payload, len);
        return 1;
    }
    if (t == DEPZ_CRC16) {
        uint16_t c = depz_crc16_modbus(payload, len);
        out[0] = (uint8_t)(c & 0xFFu);
        out[1] = (uint8_t)((c >> 8) & 0xFFu);
        return 2;
    }
    uint32_t c = depz_crc32_iso_hdlc(payload, len);
    out[0] = (uint8_t)(c & 0xFFu);
    out[1] = (uint8_t)((c >> 8) & 0xFFu);
    out[2] = (uint8_t)((c >> 16) & 0xFFu);
    out[3] = (uint8_t)((c >> 24) & 0xFFu);
    return 4;
}

int depz_build_packet(uint8_t cmd, const uint8_t *payload, size_t payload_len,
                      unsigned int seq, depz_crc_type crc_type,
                      uint8_t *out, size_t out_cap, size_t *out_len)
{
    if (payload_len > DEPZ_MAX_PAYLOAD)
        return -1;

    uint16_t data_size = (uint16_t)(payload_len | ((unsigned)crc_type << 14));
    uint8_t hdr[4] = {
        (uint8_t)(data_size & 0xFFu),
        (uint8_t)((data_size >> 8) & 0xFFu),
        cmd,
        (uint8_t)(seq & 0xFFu)
    };
    uint8_t hdr_crc = depz_crc8_maxim(hdr, 4);

    uint8_t trailer[4];
    size_t trailer_len = payload_crc_trailer(crc_type, payload, payload_len, trailer);

    size_t total = DEPZ_HEADER_SIZE + payload_len + trailer_len;
    if (out_cap < total)
        return -2;

    out[0] = DEPZ_MAGIC0;
    out[1] = DEPZ_MAGIC1;
    out[2] = hdr[0];
    out[3] = hdr[1];
    out[4] = hdr[2];
    out[5] = hdr[3];
    out[6] = hdr_crc;
    if (payload_len)
        memcpy(out + DEPZ_HEADER_SIZE, payload, payload_len);
    if (trailer_len)
        memcpy(out + DEPZ_HEADER_SIZE + payload_len, trailer, trailer_len);

    if (out_len)
        *out_len = total;
    return 0;
}

/* --- incremental parser --------------------------------------------------- */

void depz_parser_init(depz_parser *p)
{
    memset(p, 0, sizeof(*p));
}

void depz_parser_free(depz_parser *p)
{
    free(p->buf);
    p->buf = NULL;
    p->len = p->cap = 0;
}

static int buf_reserve(depz_parser *p, size_t extra)
{
    size_t need = p->len + extra;
    if (need <= p->cap)
        return 0;
    size_t cap = p->cap ? p->cap : 64;
    while (cap < need)
        cap *= 2;
    uint8_t *nb = (uint8_t *)realloc(p->buf, cap);
    if (!nb)
        return -1;
    p->buf = nb;
    p->cap = cap;
    return 0;
}

/* Remove `count` bytes from the front of the buffer. */
static void buf_consume(depz_parser *p, size_t count)
{
    if (count == 0)
        return;
    if (count >= p->len) {
        p->len = 0;
        return;
    }
    memmove(p->buf, p->buf + count, p->len - count);
    p->len -= count;
}

/* Find the first "A5 C3" in [start, len). Returns index or -1. */
static long find_magic(const uint8_t *b, size_t len)
{
    if (len < 2)
        return -1;
    for (size_t i = 0; i + 1 < len; i++)
        if (b[i] == DEPZ_MAGIC0 && b[i + 1] == DEPZ_MAGIC1)
            return (long)i;
    return -1;
}

static void emit_trash(depz_parser *p, size_t count, depz_event_cb cb, void *user)
{
    if (cb) {
        depz_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = DEPZ_EV_TRASH;
        ev.trash = p->buf;
        ev.trash_len = count;
        cb(&ev, user);
    }
    p->trash_bytes += count;
    buf_consume(p, count);
}

/* Returns 1 if an event was produced/consumed and parsing should continue,
 * 0 if more bytes are needed (stop). */
static int parse_one(depz_parser *p, depz_event_cb cb, void *user)
{
    long pos = find_magic(p->buf, p->len);
    if (pos == -1) {
        /* Keep the last byte: it may be a split 0xA5. */
        if (p->len > 1) {
            emit_trash(p, p->len - 1, cb, user);
            return 1;
        }
        return 0;
    }
    if (pos > 0) {
        emit_trash(p, (size_t)pos, cb, user);
        return 1;
    }
    if (p->len < DEPZ_HEADER_SIZE)
        return 0;

    uint16_t data_size = (uint16_t)(p->buf[2] | ((uint16_t)p->buf[3] << 8));
    size_t payload_size = data_size & DEPZ_MAX_PAYLOAD;
    depz_crc_type crc_type = (depz_crc_type)((data_size >> 14) & 0x03u);

    if (depz_crc8_maxim(p->buf + 2, 4) != p->buf[6]) {
        /* Header corrupt: advance one byte past the magic start; let the
         * magic hunt resync (firmware: rb_skip(off + 1)). */
        p->header_errors += 1;
        emit_trash(p, 1, cb, user);
        return 1;
    }

    /* ERRATA E6: empty payload never carries CRC bytes. */
    size_t crc_size = payload_size > 0 ? crc_size_for(crc_type) : 0;
    size_t total = DEPZ_HEADER_SIZE + payload_size + crc_size;
    if (p->len < total)
        return 0;

    uint8_t cmd = p->buf[4];
    uint8_t seq = p->buf[5];
    const uint8_t *payload = p->buf + DEPZ_HEADER_SIZE;

    int crc_ok = 1;
    if (crc_size) {
        uint8_t expect[4];
        size_t n = payload_crc_trailer(crc_type, payload, payload_size, expect);
        const uint8_t *trailer = p->buf + DEPZ_HEADER_SIZE + payload_size;
        if (n != crc_size || memcmp(expect, trailer, crc_size) != 0)
            crc_ok = 0;
    }

    if (!crc_ok) {
        p->crc_errors += 1;
        if (cb) {
            depz_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = DEPZ_EV_CRC_ERROR;
            ev.cmd = cmd;
            ev.seq = seq;
            cb(&ev, user);
        }
        buf_consume(p, total);
        return 1;
    }

    if (cb) {
        depz_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = DEPZ_EV_PACKET;
        ev.cmd = cmd;
        ev.seq = seq;
        ev.payload = payload_size ? payload : NULL;
        ev.payload_len = payload_size;
        cb(&ev, user);
    }
    p->packets += 1;
    buf_consume(p, total);
    return 1;
}

int depz_parser_feed(depz_parser *p, const uint8_t *data, size_t len,
                     depz_event_cb cb, void *user)
{
    if (len) {
        if (buf_reserve(p, len) != 0)
            return -1;
        memcpy(p->buf + p->len, data, len);
        p->len += len;
    }
    while (parse_one(p, cb, user))
        ;
    return 0;
}
