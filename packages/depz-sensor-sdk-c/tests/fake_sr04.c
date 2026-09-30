/* fake_sr04.c — see fake_sr04.h. */
#include "fake_sr04.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

struct fake_sr04 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    bool        silent;
    uint8_t     tx_seq;
    uint32_t    sample_period_us;
    uint16_t    echo_decay_us;
    uint16_t    echo_time_us;
    bool        loop_running;
    uint64_t    mcu_time_us;
    int16_t     temperature_raw;
};

static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32(uint8_t *p, uint32_t v) { int i; for (i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put_u64(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint64_t get_u64(const uint8_t *p) { uint64_t v = 0; int i; for (i = 7; i >= 0; i--) v = v << 8 | p[i]; return v; }

/* Caller holds f->lock. */
static void send_locked(fake_sr04 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[300];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0) {
        if (depz_link_write(f->side, frame, n) == DEPZ_OK) f->tx_seq++;
    }
}

static void status(fake_sr04 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

static void text(fake_sr04 *f, uint8_t cmd, const char *s)
{
    uint8_t p[128];
    size_t n = strlen(s);
    p[0] = cmd;
    memcpy(p + 1, s, n);
    p[n + 1] = 0;
    send_locked(f, DEPZ_RPT_TEXT, p, n + 2);
}

static void measurement_locked(fake_sr04 *f, uint8_t source)
{
    uint8_t p[11];
    f->mcu_time_us += f->sample_period_us;
    p[0] = source;
    put_u64(p + 1, f->mcu_time_us);
    put_u16(p + 9, f->echo_time_us);
    send_locked(f, DEPZ_SR04_RPT_DATA, p, 11);
}

static void handle(fake_sr04 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[24];
    switch (cmd) {
    case 0x03: text(f, cmd, FAKE_SR04_NAME); break;
    case 0x04: text(f, cmd, FAKE_SR04_SOFTWARE); break;
    case 0x05: text(f, cmd, FAKE_SR04_SERIAL); break;
    case 0x06: { /* SYNC_TIME: zero device-processing gap, so rtt >= 0 */
        uint64_t t2 = f->mcu_time_us + 500;
        if (len != 8) { status(f, cmd, 0x03); break; }
        put_u64(out, get_u64(p));
        put_u64(out + 8, t2);
        put_u64(out + 16, t2);
        send_locked(f, 0x82, out, 24);
        break;
    }
    case 0x07:
        put_u64(out, f->mcu_time_us);
        put_u16(out + 8, (uint16_t)f->temperature_raw);
        send_locked(f, 0x83, out, 10);
        break;
    case 0x31:
        if (len != 3 || p[0] < 1 || p[0] > 5 || p[1] > 4 || p[2] > 1) status(f, cmd, 0x04);
        else status(f, cmd, 0x00);
        break;
    case 0x30:
        if (len != 1 || p[0] < 1 || p[0] > 5) { status(f, cmd, 0x04); break; }
        out[0] = p[0];
        out[1] = 0;
        out[2] = 0;
        send_locked(f, 0x90, out, 3);
        break;
    case 0x32:
        put_u32(out, f->sample_period_us);
        send_locked(f, 0x92, out, 4);
        break;
    case 0x33:
        f->sample_period_us = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
        status(f, cmd, 0x00);
        break;
    case 0x34:
        put_u16(out, f->echo_decay_us);
        send_locked(f, 0x93, out, 2);
        break;
    case 0x35: {
        uint16_t req = (uint16_t)(p[0] | p[1] << 8);
        f->echo_decay_us = req < 4000 ? 4000 : req > 65000 ? 65000 : req;
        status(f, cmd, 0x00);
        break;
    }
    case 0x36:
        if (f->loop_running) status(f, cmd, 0x06); /* ERR_BUSY */
        else measurement_locked(f, 0x36);          /* no OK ack: the data IS the reply */
        break;
    case 0x37: f->loop_running = true; status(f, cmd, 0x00); break;
    case 0x38: f->loop_running = false; status(f, cmd, 0x00); break;
    default: status(f, cmd, 0x02); /* ERR_INVALID_CMD */
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_sr04 *f = (fake_sr04 *)user;
    if (e->type != DEPZ_EV_PACKET || f->silent) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_sr04 *f = (fake_sr04 *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

static fake_sr04 *start(depz_link **host_link, bool silent)
{
    fake_sr04 *f = (fake_sr04 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->silent = silent;
    f->sample_period_us = 50000;
    f->echo_decay_us = 5000;
    f->echo_time_us = 5831; /* ~1 m */
    f->mcu_time_us = 1000000;
    f->temperature_raw = 273;
    depz_thread_start(&f->thread, run, f);
    return f;
}

fake_sr04 *fake_sr04_start(depz_link **host_link) { return start(host_link, false); }
fake_sr04 *fake_silent_start(depz_link **host_link) { return start(host_link, true); }

void fake_sr04_stop(fake_sr04 *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    free(f);
}

uint32_t fake_sr04_sample_period(fake_sr04 *f)
{
    uint32_t v;
    depz_mutex_lock(&f->lock);
    v = f->sample_period_us;
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_sr04_set_echo(fake_sr04 *f, uint16_t echo_us)
{
    depz_mutex_lock(&f->lock);
    f->echo_time_us = echo_us;
    depz_mutex_unlock(&f->lock);
}

bool fake_sr04_loop_running(fake_sr04 *f)
{
    bool v;
    depz_mutex_lock(&f->lock);
    v = f->loop_running;
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_sr04_send_measurement(fake_sr04 *f, uint8_t source_cmd)
{
    depz_mutex_lock(&f->lock);
    measurement_locked(f, source_cmd);
    depz_mutex_unlock(&f->lock);
}

void fake_sr04_send_raw(fake_sr04 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    depz_mutex_lock(&f->lock);
    send_locked(f, cmd, payload, len);
    depz_mutex_unlock(&f->lock);
}
