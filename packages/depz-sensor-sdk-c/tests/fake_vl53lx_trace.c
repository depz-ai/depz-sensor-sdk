/* fake_vl53lx_trace.c — see fake_vl53lx_trace.h. Modelled on fake_vl53l4.c. */
#if DEPZ_VECTORS_HAS_IO
#include "fake_vl53lx_trace.h"

#include "../src/io/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fake_vl53lx_trace {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    const fake_trace_op *ops;
    size_t      n_ops, next;
    char        mismatch[160];
    uint64_t    mcu_time_us;
};

static void put_u64le(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }

static void send_locked(fake_vl53lx_trace *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[300];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq = (uint8_t)(f->tx_seq + 1); /* wraps mod 256 (contract 01 §5) */
}

static void status(fake_vl53lx_trace *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

/* The next op of the trace if it is (write, addr); NULL (and the mismatch
 * recorded) otherwise. Consumes the op either way, so a strayed driver stops. */
static const fake_trace_op *take(fake_vl53lx_trace *f, bool write, uint16_t addr)
{
    const fake_trace_op *want;
    if (f->next >= f->n_ops) {
        if (!f->mismatch[0])
            snprintf(f->mismatch, sizeof f->mismatch, "op #%u beyond the trace: %s 0x%02X",
                     (unsigned)f->next, write ? "w" : "r", (unsigned)addr);
        return NULL;
    }
    want = &f->ops[f->next++];
    if (want->write != write || want->addr != addr) {
        if (!f->mismatch[0])
            snprintf(f->mismatch, sizeof f->mismatch, "op #%u: expected %s 0x%02X, got %s 0x%02X",
                     (unsigned)(f->next - 1), want->write ? "w" : "r", (unsigned)want->addr,
                     write ? "w" : "r", (unsigned)addr);
        return NULL;
    }
    return want;
}

/* A consumed op whose length or bytes differ from what the driver sent/asked. */
static void differs(fake_vl53lx_trace *f, const char *what, uint16_t addr)
{
    if (!f->mismatch[0])
        snprintf(f->mismatch, sizeof f->mismatch, "op #%u: 0x%02X %s differs", (unsigned)(f->next - 1),
                 (unsigned)addr, what);
}

static void handle(fake_vl53lx_trace *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[9 + FAKE_TRACE_DATA_MAX];
    f->mcu_time_us += 1000;
    switch (cmd) {
    case 0x03: {
        static const char name[] = "DEPZ ToF Sensor VL53L0X USB v2.1 TRACE";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x04: {
        static const char name[] = "APP_VL53L0_4_v0.25";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x32: { /* READ_REG: addr u16, len u16 */
        uint16_t a, n;
        const fake_trace_op *want;
        if (len != 4) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        n = (uint16_t)(p[2] | p[3] << 8);
        want = take(f, false, a);
        if (!want) { status(f, cmd, 0x01); break; }
        if (want->len != n) { differs(f, "read length", a); status(f, cmd, 0x01); break; }
        out[0] = cmd;
        put_u64le(out + 1, f->mcu_time_us);
        memcpy(out + 9, want->data, want->len);
        send_locked(f, 0x91, out, 9 + want->len);
        break;
    }
    case 0x33: { /* WRITE_REG: addr u16, data */
        uint16_t a;
        size_t n = len - 2;
        const fake_trace_op *want;
        if (len < 3) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        want = take(f, true, a);
        if (!want) { status(f, cmd, 0x01); break; }
        if (want->len != n || memcmp(want->data, p + 2, n) != 0) { differs(f, "write data", a); status(f, cmd, 0x01); break; }
        status(f, cmd, 0x00);
        break;
    }
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_vl53lx_trace *f = (fake_vl53lx_trace *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_vl53lx_trace *f = (fake_vl53lx_trace *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_vl53lx_trace *fake_vl53lx_trace_start(const fake_trace_op *ops, size_t n_ops, depz_link **host_link)
{
    fake_vl53lx_trace *f = (fake_vl53lx_trace *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->ops = ops;
    f->n_ops = n_ops;
    f->mcu_time_us = 1000000;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_vl53lx_trace_stop(fake_vl53lx_trace *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    free(f);
}

size_t fake_vl53lx_trace_issued(fake_vl53lx_trace *f, char *mismatch_out, size_t cap)
{
    size_t n;
    depz_mutex_lock(&f->lock);
    n = f->next;
    if (cap) {
        strncpy(mismatch_out, f->mismatch, cap - 1);
        mismatch_out[cap - 1] = '\0';
    }
    depz_mutex_unlock(&f->lock);
    return n;
}
#else
typedef int fake_vl53lx_trace_needs_the_io_layer;
#endif
