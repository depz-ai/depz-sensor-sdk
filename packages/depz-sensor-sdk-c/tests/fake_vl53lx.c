/* fake_vl53lx.c — see fake_vl53lx.h. */
#include "fake_vl53lx.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t cmd; uint8_t *p; size_t len; } rx_rec;

struct fake_vl53lx {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    char        device_name[128];
    uint8_t     regs[0x10000];
    rx_rec     *rx;
    size_t      n_rx, cap_rx;
    int         fail[256];      /* 0 = none, else status, FAKE_VLX_SILENT */
    uint64_t    mcu_time_us;
};

static void put_u16le(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u64le(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }

static void send_locked(fake_vl53lx *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[300];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq = (uint8_t)(f->tx_seq + 1); /* wraps mod 256 (contract 01 §5) */
}

static void status(fake_vl53lx *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

static void text(fake_vl53lx *f, uint8_t cmd, const char *s)
{
    uint8_t out[160];
    size_t n = strlen(s) + 1;
    out[0] = cmd;
    memcpy(out + 1, s, n);
    send_locked(f, DEPZ_RPT_TEXT, out, 1 + n);
}

static void seed_word(fake_vl53lx *f, uint16_t a, uint16_t v)
{
    f->regs[a] = (uint8_t)(v >> 8);
    f->regs[a + 1] = (uint8_t)v;
}

static void log_rx(fake_vl53lx *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    rx_rec *r;
    if (f->n_rx == f->cap_rx) {
        size_t nc = f->cap_rx ? f->cap_rx * 2 : 256;
        rx_rec *nr = (rx_rec *)realloc(f->rx, nc * sizeof *nr);
        if (!nr) return;
        f->rx = nr;
        f->cap_rx = nc;
    }
    r = &f->rx[f->n_rx];
    r->p = (uint8_t *)malloc(len ? len : 1);
    if (!r->p) return;
    if (len) memcpy(r->p, p, len);
    r->cmd = cmd;
    r->len = len;
    f->n_rx++;
}

static void handle(fake_vl53lx *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[270];
    int fail;
    log_rx(f, cmd, p, len);
    fail = f->fail[cmd];
    f->fail[cmd] = 0;
    if (fail == FAKE_VLX_SILENT) return;
    if (fail) { status(f, cmd, (uint8_t)fail); return; }
    switch (cmd) {
    case 0x03: text(f, cmd, f->device_name); break;
    case 0x04: text(f, cmd, "APP_VL53L0_4_v0.25"); break;
    case 0x32: { /* READ_REG: addr u16, len u16 -> RPT_VL53_REG_DATA: cmd, ts u64, data */
        uint16_t a, n;
        if (len != 4) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        n = (uint16_t)(p[2] | p[3] << 8);
        if (n < 1 || n > 253 || (uint32_t)a + n > 0x10000u) { status(f, cmd, 0x04); break; }
        out[0] = cmd;
        put_u64le(out + 1, f->mcu_time_us);
        memcpy(out + 9, f->regs + a, n);
        send_locked(f, 0x91, out, 9 + (size_t)n);
        f->mcu_time_us += 100;
        break;
    }
    case 0x33: { /* WRITE_REG: addr u16, data */
        uint16_t a;
        if (len < 3) { status(f, cmd, 0x03); break; }
        a = (uint16_t)(p[0] | p[1] << 8);
        if ((uint32_t)a + (len - 2) > 0x10000u) { status(f, cmd, 0x04); break; }
        memcpy(f->regs + a, p + 2, len - 2);
        status(f, cmd, 0x00);
        break;
    }
    case 0x37: /* GET_INFO: RPT_VL53_INFO, 23 zero bytes */
        memset(out, 0, 23);
        send_locked(f, 0x92, out, 23);
        break;
    case 0x34: case 0x35: case 0x36: case 0x38: case 0x39: case 0x3A: /* XSHUT .. CLEAR_I2C_ERRORS */
        status(f, cmd, 0x00);
        break;
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_vl53lx *f = (fake_vl53lx *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_vl53lx *f = (fake_vl53lx *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_vl53lx *fake_vl53lx_start(const char *device_name, depz_link **host_link)
{
    fake_vl53lx *f = (fake_vl53lx *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    strncpy(f->device_name, device_name, sizeof f->device_name - 1);
    /* A booted die. */
    f->regs[0x00E5] = 0x03;
    seed_word(f, 0x0006, FAKE_VLX_DIE_OSC);
    seed_word(f, 0x010F, 0xEBAA);
    seed_word(f, 0x00DE, FAKE_VLX_DIE_PLL);
    f->regs[0x013E] = 0x91; /* ROI_CONFIG__MODE_ROI_CENTRE_SPAD */
    f->mcu_time_us = 5000000;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_vl53lx_stop(fake_vl53lx *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    fake_vl53lx_clear(f); /* takes the lock: before it is destroyed */
    depz_mutex_destroy(&f->lock);
    free(f->rx);
    free(f);
}

uint8_t fake_vl53lx_reg(fake_vl53lx *f, uint16_t addr)
{
    uint8_t v;
    depz_mutex_lock(&f->lock);
    v = f->regs[addr];
    depz_mutex_unlock(&f->lock);
    return v;
}

uint16_t fake_vl53lx_word(fake_vl53lx *f, uint16_t addr)
{
    return (uint16_t)(fake_vl53lx_reg(f, addr) << 8 | fake_vl53lx_reg(f, (uint16_t)(addr + 1)));
}

uint32_t fake_vl53lx_dword(fake_vl53lx *f, uint16_t addr)
{
    return (uint32_t)fake_vl53lx_word(f, addr) << 16 | fake_vl53lx_word(f, (uint16_t)(addr + 2));
}

void fake_vl53lx_set_reg(fake_vl53lx *f, uint16_t addr, uint8_t v)
{
    depz_mutex_lock(&f->lock);
    f->regs[addr] = v;
    depz_mutex_unlock(&f->lock);
}

void fake_vl53lx_set_block(fake_vl53lx *f, const uint8_t *raw, size_t len)
{
    depz_mutex_lock(&f->lock);
    memcpy(f->regs + 0x0089, raw, len);
    depz_mutex_unlock(&f->lock);
}

void fake_vl53lx_fail_next(fake_vl53lx *f, uint8_t cmd, int status_code)
{
    depz_mutex_lock(&f->lock);
    f->fail[cmd] = status_code;
    depz_mutex_unlock(&f->lock);
}

void fake_vl53lx_clear(fake_vl53lx *f)
{
    size_t i;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_rx; i++) free(f->rx[i].p);
    f->n_rx = 0;
    depz_mutex_unlock(&f->lock);
}

size_t fake_vl53lx_rx_count(fake_vl53lx *f)
{
    size_t n;
    depz_mutex_lock(&f->lock);
    n = f->n_rx;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_vl53lx_count(fake_vl53lx *f, uint8_t cmd)
{
    size_t i, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_rx; i++) n += f->rx[i].cmd == cmd;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_vl53lx_sent(fake_vl53lx *f, uint8_t cmd, size_t k, uint8_t *out, size_t cap)
{
    size_t i, seen = 0, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_rx; i++) {
        if (f->rx[i].cmd != cmd || seen++ != k) continue;
        n = f->rx[i].len;
        memcpy(out, f->rx[i].p, n < cap ? n : cap);
        break;
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

void fake_vl53lx_stream(fake_vl53lx *f, uint64_t ts, uint16_t len_field, const uint8_t *data, size_t len)
{
    uint8_t p[12 + 128];
    put_u64le(p, ts);
    put_u16le(p + 8, 0x0089);
    put_u16le(p + 10, len_field);
    memcpy(p + 12, data, len);
    depz_mutex_lock(&f->lock);
    send_locked(f, 0x93, p, 12 + len);
    depz_mutex_unlock(&f->lock);
}
