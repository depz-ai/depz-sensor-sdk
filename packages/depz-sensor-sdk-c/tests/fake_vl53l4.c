/* fake_vl53l4.c — see fake_vl53l4.h. */
#include "fake_vl53l4.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

typedef struct { uint16_t addr; uint8_t data[256]; size_t len; } wr_rec;

struct fake_vl53l4 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    uint8_t     reg[0x10000];
    wr_rec     *writes;
    size_t      n_writes, cap_writes;
    uint16_t    speeds[64];
    size_t      n_speeds;
    bool        streaming;
    uint64_t    mcu_time_us;
};

static void put_u16le(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u64le(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }

static void send_locked(fake_vl53l4 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[300];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq++;
}

static void status(fake_vl53l4 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

static void seed_word(fake_vl53l4 *f, uint16_t a, uint16_t v)
{
    f->reg[a] = (uint8_t)(v >> 8);
    f->reg[a + 1] = (uint8_t)v;
}

static void handle(fake_vl53l4 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[270];
    f->mcu_time_us += 1000;
    switch (cmd) {
    case 0x03: {
        static const char name[] = "DEPZ ToF VL53L4CD USB v2.1 FAKE";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x04: {
        static const char name[] = "APP_VL53L4_v0.83";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x32: { /* READ_REG: addr u16, len u16 */
        uint16_t a, n;
        if (len != 4) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        n = (uint16_t)(p[2] | p[3] << 8);
        if (n < 1 || n > 253) { status(f, cmd, 0x04); break; }
        out[0] = cmd;
        put_u64le(out + 1, f->mcu_time_us);
        memcpy(out + 9, f->reg + a, n);
        send_locked(f, 0x91, out, 9 + (size_t)n);
        break;
    }
    case 0x33: { /* WRITE_REG: addr u16, data */
        uint16_t a;
        size_t n = len - 2;
        wr_rec *w;
        if (len < 3) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        memcpy(f->reg + a, p + 2, n);
        if (f->n_writes == f->cap_writes) {
            size_t nc = f->cap_writes ? f->cap_writes * 2 : 64;
            wr_rec *nw = (wr_rec *)realloc(f->writes, nc * sizeof *nw);
            if (!nw) { status(f, cmd, 0x01); break; }
            f->writes = nw;
            f->cap_writes = nc;
        }
        w = &f->writes[f->n_writes++];
        w->addr = a;
        w->len = n;
        memcpy(w->data, p + 2, n);
        status(f, cmd, 0x00);
        break;
    }
    case 0x34: f->streaming = false; status(f, cmd, 0x00); break; /* XSHUT */
    case 0x35: f->streaming = true; status(f, cmd, 0x00); break;  /* START_STREAM */
    case 0x36: f->streaming = false; status(f, cmd, 0x00); break; /* STOP_STREAM */
    case 0x37: /* GET_INFO, 21 bytes */
        memset(out, 0, 21);
        put_u16le(out + 13, (uint16_t)(f->reg[0x010F] << 8 | f->reg[0x0110]));
        out[15] = f->reg[0x00E5];
        out[16] = 1;
        out[17] = 1;
        put_u16le(out + 19, f->n_speeds ? f->speeds[f->n_speeds - 1] : 400);
        send_locked(f, 0x92, out, 21);
        break;
    case 0x38: /* SET_I2C_SPEED */
        if (len == 2 && f->n_speeds < 64) f->speeds[f->n_speeds++] = (uint16_t)(p[0] | p[1] << 8);
        status(f, cmd, 0x00);
        break;
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_vl53l4 *f = (fake_vl53l4 *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_vl53l4 *f = (fake_vl53l4 *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_vl53l4 *fake_vl53l4_start(depz_link **host_link)
{
    fake_vl53l4 *f = (fake_vl53l4 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->reg[0x00E5] = 0x03; /* FIRMWARE__SYSTEM_STATUS: booted */
    seed_word(f, 0x010F, DEPZ_VL53L4_MODEL_ID);
    seed_word(f, 0x0006, FAKE_VL53L4_OSC_FREQUENCY);
    seed_word(f, 0x00DE, FAKE_VL53L4_CLOCK_PLL);
    f->mcu_time_us = 1000000;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_vl53l4_stop(fake_vl53l4 *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    free(f->writes);
    free(f);
}

uint8_t fake_vl53l4_reg(fake_vl53l4 *f, uint16_t addr)
{
    uint8_t v;
    depz_mutex_lock(&f->lock);
    v = f->reg[addr];
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_vl53l4_set_reg(fake_vl53l4 *f, uint16_t addr, uint8_t v)
{
    depz_mutex_lock(&f->lock);
    f->reg[addr] = v;
    depz_mutex_unlock(&f->lock);
}

size_t fake_vl53l4_speeds(fake_vl53l4 *f, uint16_t *out, size_t cap)
{
    size_t n;
    depz_mutex_lock(&f->lock);
    n = f->n_speeds < cap ? f->n_speeds : cap;
    memcpy(out, f->speeds, n * sizeof *out);
    n = f->n_speeds;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_vl53l4_writes_at(fake_vl53l4 *f, uint16_t addr)
{
    size_t i, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_writes; i++) n += f->writes[i].addr == addr;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_vl53l4_write_at(fake_vl53l4 *f, uint16_t addr, size_t k, uint8_t *out, size_t cap)
{
    size_t i, seen = 0, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_writes; i++) {
        if (f->writes[i].addr != addr) continue;
        if (seen++ == k) {
            n = f->writes[i].len < cap ? f->writes[i].len : cap;
            memcpy(out, f->writes[i].data, n);
            break;
        }
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

bool fake_vl53l4_streaming(fake_vl53l4 *f)
{
    bool v;
    depz_mutex_lock(&f->lock);
    v = f->streaming;
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_vl53l4_send_stream(fake_vl53l4 *f, uint64_t ts, const uint8_t *block, size_t len)
{
    uint8_t p[12 + 64];
    put_u64le(p, ts);
    put_u16le(p + 8, DEPZ_VL53L4_RESULT_BLOCK_ADDR);
    put_u16le(p + 10, (uint16_t)len);
    memcpy(p + 12, block, len);
    depz_mutex_lock(&f->lock);
    send_locked(f, 0x93, p, 12 + len);
    depz_mutex_unlock(&f->lock);
}
