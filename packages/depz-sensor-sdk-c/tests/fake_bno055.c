/* fake_bno055.c — see fake_bno055.h. */
#include "fake_bno055.h"

#include "../src/io/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPR_MODE    0x3D
#define PWR_MODE    0x3E
#define SYS_TRIGGER 0x3F
#define SYS_STATUS  0x39
#define SYS_ERR     0x3A
#define PAGE_ID     0x07
#define QUA_DATA    0x20
#define INT_STA     0x37
#define ST_RESULT   0x36

/* RPT_BNO_INFO of the live board (vectors/bno055.json info_live_board). */
static const uint8_t LIVE_INFO[38] = {
    0x28, 0xa0, 0xfb, 0x32, 0x0f, 0x11, 0x03, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x01, 0x2b, 0x09, 0xd1,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x00};
/* The quaternion a running fusion outputs (vectors stream_20_8). */
static const uint8_t LIVE_QUAT[8] = {0xbd, 0x29, 0x63, 0xf9, 0xef, 0xcf, 0x00, 0x00};

typedef struct { uint8_t cmd; uint8_t p[136]; size_t len; } sent_rec;
typedef struct { bool write; int page; uint8_t addr; uint8_t data[128]; size_t len; } op_rec;

struct fake_bno055 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    uint8_t     regs[2][256];
    sent_rec   *sent;
    size_t      n_sent, cap_sent;
    op_rec     *ops;
    size_t      n_ops, cap_ops;
    uint64_t    mcu_time_us;
    uint8_t     info[64];
    size_t      info_len;
    uint8_t     fail_next[256];      /* 0 = none */
    int         fail_write[256];     /* page-0 addr -> status, 0 = none */
    bool        silent[256];
    size_t      short_read_by;
    uint8_t     script_v;
    unsigned    script_n;
    bool        fusion_stuck;
    unsigned    fusion_zero_left;
    bool        self_test_leftover;
    bool        streaming;
    int         reset_count;
};

static void send_locked(fake_bno055 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[300];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq = (uint8_t)(f->tx_seq + 1); /* wraps mod 256 (contract 01 §5) */
}

static void status(fake_bno055 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

static void *grow(void *arr, size_t *cap, size_t n, size_t elem)
{
    void *na;
    size_t nc;
    if (n < *cap) return arr;
    nc = *cap ? *cap * 2 : 128;
    na = realloc(arr, nc * elem);
    if (!na) return NULL;
    *cap = nc;
    return na;
}

static void log_op(fake_bno055 *f, bool write, uint8_t addr, const uint8_t *data, size_t len)
{
    op_rec *o, *na = (op_rec *)grow(f->ops, &f->cap_ops, f->n_ops, sizeof *o);
    if (!na) return;
    f->ops = na;
    o = &f->ops[f->n_ops++];
    o->write = write;
    o->page = f->regs[0][PAGE_ID] & 1;
    o->addr = addr;
    o->len = len < sizeof o->data ? len : sizeof o->data;
    if (data) memcpy(o->data, data, o->len);
    else memset(o->data, 0, o->len);
}

static void power_on(fake_bno055 *f)
{
    static const uint8_t ids[7] = {0xA0, 0xFB, 0x32, 0x0F, 0x11, 0x15, 0x03};
    int i;
    memset(f->regs, 0, sizeof f->regs);
    memcpy(f->regs[0], ids, sizeof ids);
    f->regs[0][0x3B] = 0x80; /* UNIT_SEL: Android orientation */
    f->regs[0][0x41] = 0x24; /* AXIS_MAP_CONFIG P1 */
    f->regs[0][ST_RESULT] = 0x0F;
    for (i = 0; i < 16; i++) f->regs[1][0x50 + i] = (uint8_t)(0xC0 + i); /* UNIQUE_ID */
    f->regs[1][0x08] = 0x0D; /* ACC_CONFIG reset value */
    f->regs[1][0x09] = 0x6D;
    f->regs[1][0x0A] = 0x38;
    f->self_test_leftover = false;
    f->fusion_zero_left = 0;
}

static uint8_t sys_status(fake_bno055 *f)
{
    uint8_t mode = f->regs[0][OPR_MODE] & 0x0F;
    if (f->script_n) { f->script_n--; return f->script_v; }
    if (f->self_test_leftover) return 4;
    return mode == 0 ? 0 : mode >= 8 ? 5 : 6;
}

static void model_read(fake_bno055 *f, uint8_t addr, size_t n, uint8_t *out)
{
    int page = f->regs[0][PAGE_ID] & 1;
    size_t end = (size_t)addr + n;
    memcpy(out, f->regs[page] + addr, n);
    if (page != 0) return;
    if (addr <= SYS_STATUS && SYS_STATUS < end) out[SYS_STATUS - addr] = sys_status(f);
    if (addr <= QUA_DATA && QUA_DATA < end && f->fusion_zero_left > 0) {
        size_t lo = (size_t)(QUA_DATA - addr), hi = lo + 8 < n ? lo + 8 : n;
        f->fusion_zero_left--;
        memset(out + lo, 0, hi - lo);
        if (f->fusion_zero_left == 0 && !f->fusion_stuck) memcpy(f->regs[0] + QUA_DATA, LIVE_QUAT, 8);
    }
    if (addr <= INT_STA && INT_STA < end) f->regs[0][INT_STA] = 0; /* clears on read */
}

static void write_opr_mode(fake_bno055 *f, uint8_t mode)
{
    uint8_t current = f->regs[0][OPR_MODE] & 0x0F;
    bool suspended = (f->regs[0][PWR_MODE] & 3) == 2;
    if (mode != 0 && (current != 0 || suspended)) return; /* silently ignored (§4.1, §4.6) */
    f->regs[0][OPR_MODE] = mode;
    if (mode == 0) {
        memset(f->regs[0] + 0x08, 0, 0x34 - 0x08); /* CONFIG reads zero everywhere */
    } else {
        f->self_test_leftover = false;
        if (mode >= 8) f->fusion_zero_left = f->fusion_stuck ? 0xFFFFFFFFu : 2;
    }
}

static void model_write(fake_bno055 *f, uint8_t addr, const uint8_t *data, size_t n)
{
    int page = f->regs[0][PAGE_ID] & 1;
    size_t i;
    for (i = 0; i < n; i++) {
        unsigned reg = addr + (unsigned)i;
        uint8_t v = data[i];
        if (page == 0 && reg == OPR_MODE) {
            write_opr_mode(f, v & 0x0F);
        } else if (page == 0 && reg == SYS_TRIGGER) {
            f->regs[0][SYS_TRIGGER] = v & 0x80; /* CLK_SEL sticks, action bits self-clear */
            if ((v & 0x01) && (f->regs[0][OPR_MODE] & 0x0F) == 0) {
                f->regs[0][ST_RESULT] = 0x0F;
                f->regs[0][SYS_ERR] = 0;
                f->self_test_leftover = true;
            }
            if (v & 0x40) f->regs[0][INT_STA] = 0;
        } else if (reg == PAGE_ID) {
            f->regs[0][PAGE_ID] = v & 1; /* PAGE_ID is on both pages */
        } else {
            f->regs[page][reg] = v;
        }
    }
}

static void handle(fake_bno055 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[160];
    sent_rec *na = (sent_rec *)grow(f->sent, &f->cap_sent, f->n_sent, sizeof *na);
    if (na) {
        sent_rec *s;
        f->sent = na;
        s = &f->sent[f->n_sent++];
        s->cmd = cmd;
        s->len = len < sizeof s->p ? len : sizeof s->p;
        if (s->len) memcpy(s->p, p, s->len);
    }
    if (f->silent[cmd]) return;
    if (f->fail_next[cmd]) {
        status(f, cmd, f->fail_next[cmd]);
        f->fail_next[cmd] = 0;
        return;
    }
    f->mcu_time_us += 1000;
    switch (cmd) {
    case 0x32: { /* READ_REG: addr, len -> RPT_BNO_REG_DATA: cmd, ts u64, data */
        size_t n, k;
        if (len != 2) { status(f, cmd, 0x03); return; }
        n = p[1];
        if (n < 1 || n > 128 || (size_t)p[0] + n > 0x100) { status(f, cmd, 0x04); return; }
        log_op(f, false, p[0], NULL, n);
        out[0] = cmd;
        for (k = 0; k < 8; k++) out[1 + k] = (uint8_t)(f->mcu_time_us >> (8 * k));
        model_read(f, p[0], n, out + 9);
        if (f->short_read_by) {
            n = n > f->short_read_by ? n - f->short_read_by : 0;
            f->short_read_by = 0;
        }
        send_locked(f, 0x91, out, 9 + n);
        return;
    }
    case 0x33: /* WRITE_REG: addr, data */
        if (len < 2 || len > 129) { status(f, cmd, 0x03); return; }
        if ((size_t)p[0] + (len - 1) > 0x100) { status(f, cmd, 0x04); return; }
        if ((f->regs[0][PAGE_ID] & 1) == 0 && f->fail_write[p[0]]) {
            status(f, cmd, (uint8_t)f->fail_write[p[0]]);
            f->fail_write[p[0]] = 0;
            return;
        }
        log_op(f, true, p[0], p + 1, len - 1);
        model_write(f, p[0], p + 1, len - 1);
        status(f, cmd, 0x00);
        return;
    case 0x34: /* RESET */
        f->reset_count++;
        f->streaming = false;
        power_on(f);
        status(f, cmd, 0x00);
        return;
    case 0x35: /* START_STREAM */
        if (len != 5) { status(f, cmd, 0x03); return; }
        f->streaming = true;
        status(f, cmd, 0x00);
        return;
    case 0x36: f->streaming = false; status(f, cmd, 0x00); return;
    case 0x37: send_locked(f, 0x92, f->info, f->info_len); return;
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_bno055 *f = (fake_bno055 *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_bno055 *f = (fake_bno055 *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_bno055 *fake_bno055_start(depz_link **host_link)
{
    fake_bno055 *f = (fake_bno055 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    memcpy(f->info, LIVE_INFO, sizeof LIVE_INFO);
    f->info_len = sizeof LIVE_INFO;
    f->mcu_time_us = 5000000;
    power_on(f);
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_bno055_stop(fake_bno055 *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    free(f->sent);
    free(f->ops);
    free(f);
}

uint8_t fake_bno055_reg(fake_bno055 *f, int page, uint8_t addr)
{
    uint8_t v;
    depz_mutex_lock(&f->lock);
    v = f->regs[page & 1][addr];
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_bno055_set_reg(fake_bno055 *f, int page, uint8_t addr, uint8_t v)
{
    depz_mutex_lock(&f->lock);
    f->regs[page & 1][addr] = v;
    depz_mutex_unlock(&f->lock);
}

void fake_bno055_set_mode(fake_bno055 *f, uint8_t mode)
{
    depz_mutex_lock(&f->lock);
    f->regs[0][OPR_MODE] = mode;
    if (mode >= 8) memcpy(f->regs[0] + QUA_DATA, LIVE_QUAT, 8);
    depz_mutex_unlock(&f->lock);
}

uint8_t fake_bno055_opr_mode(fake_bno055 *f) { return fake_bno055_reg(f, 0, OPR_MODE) & 0x0F; }
int fake_bno055_page(fake_bno055 *f) { return fake_bno055_reg(f, 0, PAGE_ID) & 1; }

void fake_bno055_clear(fake_bno055 *f)
{
    depz_mutex_lock(&f->lock);
    f->n_sent = 0;
    f->n_ops = 0;
    depz_mutex_unlock(&f->lock);
}

void fake_bno055_writes(fake_bno055 *f, char *out, size_t cap)
{
    size_t i, k, at = 0;
    if (!cap) return;
    out[0] = '\0';
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_ops; i++) {
        const op_rec *o = &f->ops[i];
        if (!o->write) continue;
        if (at && at + 1 < cap) out[at++] = ' ';
        at += (size_t)snprintf(out + at, at < cap ? cap - at : 0, "%d:%02x=", o->page, o->addr);
        for (k = 0; k < o->len && at < cap; k++)
            at += (size_t)snprintf(out + at, cap - at, "%02x", o->data[k]);
        if (at >= cap) { out[cap - 1] = '\0'; break; }
    }
    depz_mutex_unlock(&f->lock);
}

size_t fake_bno055_reads(fake_bno055 *f, uint8_t addr)
{
    size_t i, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_ops; i++) n += !f->ops[i].write && f->ops[i].addr == addr;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_bno055_sent(fake_bno055 *f, uint8_t cmd)
{
    size_t i, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_sent; i++) n += f->sent[i].cmd == cmd;
    depz_mutex_unlock(&f->lock);
    return n;
}

size_t fake_bno055_payload(fake_bno055 *f, uint8_t cmd, size_t k, uint8_t *out, size_t cap)
{
    size_t i, seen = 0, n = 0;
    depz_mutex_lock(&f->lock);
    for (i = 0; i < f->n_sent; i++) {
        if (f->sent[i].cmd != cmd || seen++ != k) continue;
        n = f->sent[i].len;
        memcpy(out, f->sent[i].p, n < cap ? n : cap);
        break;
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

#define KNOB(stmt) do { depz_mutex_lock(&f->lock); stmt; depz_mutex_unlock(&f->lock); } while (0)

void fake_bno055_fail_next(fake_bno055 *f, uint8_t cmd, uint8_t st) { KNOB(f->fail_next[cmd] = st); }
void fake_bno055_fail_write_reg(fake_bno055 *f, uint8_t addr, uint8_t st) { KNOB(f->fail_write[addr] = st); }
void fake_bno055_silent(fake_bno055 *f, uint8_t cmd, bool on) { KNOB(f->silent[cmd] = on); }
void fake_bno055_short_read(fake_bno055 *f, size_t by) { KNOB(f->short_read_by = by); }
void fake_bno055_sys_status(fake_bno055 *f, uint8_t v, unsigned count) { KNOB((f->script_v = v, f->script_n = count)); }
void fake_bno055_fusion_stuck(fake_bno055 *f, bool on) { KNOB(f->fusion_stuck = on); }

void fake_bno055_set_info(fake_bno055 *f, const uint8_t *info, size_t len)
{
    KNOB((memcpy(f->info, info, len < sizeof f->info ? len : sizeof f->info),
          f->info_len = len < sizeof f->info ? len : sizeof f->info));
}

unsigned fake_bno055_sys_status_left(fake_bno055 *f)
{
    unsigned n;
    depz_mutex_lock(&f->lock);
    n = f->script_n;
    depz_mutex_unlock(&f->lock);
    return n;
}

int fake_bno055_reset_count(fake_bno055 *f)
{
    int n;
    depz_mutex_lock(&f->lock);
    n = f->reset_count;
    depz_mutex_unlock(&f->lock);
    return n;
}

bool fake_bno055_streaming(fake_bno055 *f)
{
    bool v;
    depz_mutex_lock(&f->lock);
    v = f->streaming;
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_bno055_push_stream(fake_bno055 *f, uint64_t ts, uint8_t addr, uint8_t len_field,
                             const uint8_t *data, size_t len)
{
    uint8_t p[10 + 128];
    int k;
    for (k = 0; k < 8; k++) p[k] = (uint8_t)(ts >> (8 * k));
    p[8] = addr;
    p[9] = len_field;
    memcpy(p + 10, data, len);
    depz_mutex_lock(&f->lock);
    send_locked(f, 0x93, p, 10 + len);
    depz_mutex_unlock(&f->lock);
}
