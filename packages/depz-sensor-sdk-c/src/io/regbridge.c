/* regbridge.c — register read/write over a DEPZ register-bridge firmware. */
#include "io_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t  rpt;
    uint8_t *out;
    size_t   want, got;
    uint64_t ts;
} rd_ctx;

static bool match_reg_data(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    rd_ctx *r = (rd_ctx *)c;
    depz_vl53l4_reg_data rep;
    size_t n;
    if (cmd != r->rpt || depz_vl53l4_unpack_reg_data(p, len, &rep) != 0) return false;
    n = rep.data_len < r->want ? rep.data_len : r->want;
    memcpy(r->out, rep.data, n);
    r->got = rep.data_len;
    r->ts = rep.timestamp_us;
    return true;
}

int depz_rb_read(depz_regbridge *rb, uint16_t addr, uint8_t *out, size_t len)
{
    size_t chunk = rb->read_max ? rb->read_max : rb->xfer_max;
    uint32_t a = addr;
    while (len) {
        size_t n = len < chunk ? len : chunk;
        uint8_t payload[4];
        rd_ctx c;
        int rc;
        c.rpt = rb->rpt_reg_data;
        c.out = out;
        c.want = n;
        c.got = 0;
        c.ts = 0;
        depz_vl53l4_pack_read_reg((uint16_t)a, (uint16_t)n, payload);
        rc = depz_device_request(rb->dev, rb->cmd_read, payload, 4, match_reg_data, &c, false,
                                 rb->timeout_ms);
        if (rc) return rc;
        if (c.got != n)
            return depz_fail(DEPZ_E_PROTOCOL, "READ_REG 0x%04X: expected %zu bytes, got %zu",
                             (unsigned)a, n, c.got);
        rb->last_timestamp_us = c.ts;
        out += n;
        a += (uint32_t)n;
        len -= n;
    }
    return DEPZ_OK;
}

/* WRITE_REG payload: addr u16 LE, then the bytes. */
int depz_rb_write(depz_regbridge *rb, uint16_t addr, const uint8_t *data, size_t len)
{
    uint8_t small[2 + 256], *payload = small;
    size_t total = len, done = 0;
    uint32_t a = addr;
    int rc = DEPZ_OK;
    if (rb->xfer_max + 2 > sizeof small) {
        payload = (uint8_t *)malloc(rb->xfer_max + 2);
        if (!payload) return depz_fail(DEPZ_E_NOMEM, "WRITE_REG: out of memory");
    }
    while (done < total) {
        size_t n = total - done < rb->xfer_max ? total - done : rb->xfer_max;
        payload[0] = (uint8_t)a;
        payload[1] = (uint8_t)(a >> 8);
        memcpy(payload + 2, data + done, n);
        rc = depz_device_request(rb->dev, rb->cmd_write, payload, 2 + n, NULL, NULL, true,
                                 rb->timeout_ms);
        if (rc) break;
        done += n;
        a += (uint32_t)n;
        if (rb->write_progress && total > rb->xfer_max) rb->write_progress(done, total, rb->progress_user);
    }
    if (payload != small) free(payload);
    return rc;
}
