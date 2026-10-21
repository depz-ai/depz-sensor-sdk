/* fake_tmf8829.c — see fake_tmf8829.h. */
#include "fake_tmf8829.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

#define CFG 0x22u
#define MAX_FRAMES 16
#define IMAGE_CAP 16384

typedef struct { uint8_t *raw; size_t len; uint64_t ts, edge; } script_frame;

struct fake_tmf8829 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    uint8_t     reg[256];
    bool        app;              /* RAM application running (else bootloader / ROM) */
    bool        cmd_busy;         /* the next CMD_STAT read still shows the command */
    uint8_t     cmd_status;
    uint8_t     base_page[190], cfg[190];
    bool        pin_fov;
    uint8_t     image[IMAGE_CAP];
    size_t      image_len, fifo_writes, fifo_max;
    uint32_t    fifo_addr;
    uint16_t    fifo_words;
    uint8_t     read_addr[256];
    uint16_t    read_len[256];
    size_t      n_reads;
    bool        armed;
    uint8_t     int_mask, flags, frame_seq;
    uint32_t    frames_sent;
    script_frame frames[MAX_FRAMES];
    int         n_frames, skip_before;
    bool        bad_header_on_stop;
    uint64_t    mcu_time_us;
};

static void put_u16le(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32le(uint8_t *p, uint32_t v) { int i; for (i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put_u64le(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }

static void send_locked(fake_tmf8829 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[1200];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq++;
}

static void status(fake_tmf8829 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

static void text(fake_tmf8829 *f, uint8_t cmd, const char *s)
{
    uint8_t out[128];
    size_t n = strlen(s) + 1;
    out[0] = cmd;
    memcpy(out + 1, s, n);
    send_locked(f, DEPZ_RPT_TEXT, out, 1 + n);
}

static void event_locked(fake_tmf8829 *f, uint8_t int_status, uint8_t code, uint8_t id, uint8_t fifo)
{
    uint8_t p[12];
    put_u64le(p, f->mcu_time_us);
    p[8] = int_status;
    p[9] = code;
    p[10] = id;
    p[11] = fifo;
    send_locked(f, 0x94, p, 12);
}

/* A command into CMD_STAT: `st` is what the register reads once done. */
static void cmd_done(fake_tmf8829 *f, uint8_t cmd, uint8_t st)
{
    f->reg[0x08] = cmd;
    f->cmd_busy = true;
    f->cmd_status = st;
}

static void play_frames(fake_tmf8829 *f)
{
    int k;
    for (k = 0; k < f->n_frames; k++) {
        const script_frame *s = &f->frames[k];
        size_t off = 0;
        if (k == f->skip_before) f->frame_seq++;
        do {
            uint8_t p[21 + 1024];
            size_t n = s->len - off < 1024 ? s->len - off : 1024;
            put_u64le(p, s->ts);
            put_u16le(p + 8, (uint16_t)s->len);
            put_u16le(p + 10, (uint16_t)off);
            p[12] = f->frame_seq;
            put_u64le(p + 13, s->edge);
            memcpy(p + 21, s->raw + off, n);
            send_locked(f, 0x93, p, 21 + n);
            off += n;
        } while (off < s->len);
        f->frame_seq++;
        f->frames_sent++;
    }
}

static void preset_load(fake_tmf8829 *f, uint8_t cmd)
{
    static const uint16_t kilo[10] = {586, 586, 586, 586, 586, 2344, 2344, 3516, 3516, 586};
    static const uint8_t fp[10] = {0, 0, 0, 2, 2, 3, 3, 5, 5, 0};
    memcpy(f->cfg, f->base_page, sizeof f->cfg);
    f->cfg[0x26 - CFG] = (uint8_t)((f->cfg[0x26 - CFG] & ~7u) | fp[cmd - 0x40]);
    put_u16le(f->cfg + (0x24 - CFG), kilo[cmd - 0x40]);
}

/* A write to CMD_STAT. */
static void command(fake_tmf8829 *f, const uint8_t *d, size_t n)
{
    uint8_t cmd = d[0];
    if (!f->app) {
        switch (cmd) {
        case 0x22: /* I2C_OFF: SPI reads ENABLE from now on */
            f->reg[0xF8] |= 0x80;
            cmd_done(f, cmd, 0);
            break;
        case 0x45: /* W_FIFO_BOTH addr u32, words u16 */
            if (n != 8 || d[1] != 6) { cmd_done(f, cmd, 1); break; }
            f->fifo_addr = (uint32_t)d[2] | (uint32_t)d[3] << 8 | (uint32_t)d[4] << 16 | (uint32_t)d[5] << 24;
            f->fifo_words = (uint16_t)(d[6] | d[7] << 8);
            f->image_len = f->fifo_writes = f->fifo_max = 0;
            cmd_done(f, cmd, 0);
            break;
        case 0x16: /* START_RAM_APP */
            if (f->image_len != (size_t)f->fifo_words * 4) { cmd_done(f, cmd, 4); break; }
            f->app = true;
            f->reg[0x00] = 0x01;
            f->reg[0x01] = 2;
            f->reg[0x02] = 200;
            f->reg[0x03] = 0;
            put_u32le(f->reg + 0x1C, FAKE_TMF8829_SERIAL);
            cmd_done(f, cmd, 0);
            break;
        default: cmd_done(f, cmd, 1);
        }
        return;
    }
    if (cmd >= 0x40 && cmd <= 0x49) { preset_load(f, cmd); cmd_done(f, cmd, 0); return; }
    switch (cmd) {
    case 0x16: /* LOAD_CONFIG_PAGE */
        memcpy(f->reg + CFG, f->cfg, sizeof f->cfg);
        f->reg[0x20] = 0x16;
        cmd_done(f, cmd, 0);
        break;
    case 0x15: /* WRITE_PAGE */
        memcpy(f->cfg, f->reg + CFG, sizeof f->cfg);
        if (f->pin_fov) f->cfg[0x78 - CFG] = (uint8_t)((f->cfg[0x78 - CFG] & 0xF0) | 5);
        cmd_done(f, cmd, 0);
        break;
    case 0x10: /* MEASURE */
        cmd_done(f, cmd, 1);
        if (f->armed) play_frames(f);
        break;
    case 0xFF: /* STOP */
        if (f->bad_header_on_stop) event_locked(f, 0x08, 1, 0xFF, 0x04);
        cmd_done(f, cmd, 0);
        break;
    default: cmd_done(f, cmd, 8); /* ERR_UNKNOWN_CMD */
    }
}

static void handle(fake_tmf8829 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[9 + 1024];
    f->mcu_time_us += 1000;
    switch (cmd) {
    case 0x03: text(f, cmd, "DEPZ ToF Sensor TMF8829 USB v2.1 FAKE"); break;
    case 0x04: text(f, cmd, "APP_TMF8829_v0.52"); break;
    case 0x32: { /* TMF_READ_REG: addr u8, len u16 */
        unsigned a, n, i;
        if (len != 3) { status(f, cmd, 0x04); break; }
        a = p[0];
        n = (unsigned)(p[1] | p[2] << 8);
        if (n < 1 || n > 1024) { status(f, cmd, 0x04); break; }
        if (f->n_reads < 256) {
            f->read_addr[f->n_reads] = (uint8_t)a;
            f->read_len[f->n_reads++] = (uint16_t)n;
        }
        out[0] = cmd;
        put_u64le(out + 1, f->mcu_time_us);
        for (i = 0; i < n; i++) {
            if (a == 0x08 && f->cmd_busy) {
                out[9 + i] = f->reg[0x08];
                f->reg[0x08] = f->cmd_status;
                f->cmd_busy = false;
            } else {
                out[9 + i] = f->reg[a];
            }
            if (a != 0xFF) a++;
        }
        send_locked(f, 0x91, out, 9 + (size_t)n);
        break;
    }
    case 0x33: { /* TMF_WRITE_REG: addr u8, data */
        unsigned a, i;
        size_t n;
        if (len < 2 || len > 1024) { status(f, cmd, 0x04); break; }
        a = p[0];
        n = len - 1;
        if (a == 0xFF) {
            if (f->image_len + n <= IMAGE_CAP) memcpy(f->image + f->image_len, p + 1, n);
            f->image_len += n;
            f->fifo_writes++;
            if (n > f->fifo_max) f->fifo_max = n;
        } else if (a == 0x08) {
            command(f, p + 1, n);
        } else {
            for (i = 0; i < n && a + i < 0xFF; i++) f->reg[a + i] = p[1 + i];
            if (a == 0xF7 && (p[1] & 0x40)) { /* soft reset: ROM bootloader, I2C on */
                f->app = false;
                f->reg[0x00] = 0x80;
                f->reg[0xF8] &= (uint8_t)~0x80;
                f->armed = false;
            }
        }
        status(f, cmd, 0x00);
        break;
    }
    case 0x35: /* TMF_START_STREAM */
        if (len != 2 || p[0] == 0 || (p[0] & ~0x0F) || (p[1] & ~0x01)) { status(f, cmd, 0x04); break; }
        f->armed = true;
        f->int_mask = p[0];
        f->flags = p[1];
        f->frame_seq = 0;
        f->frames_sent = 0;
        status(f, cmd, 0x00);
        break;
    case 0x36: f->armed = false; status(f, cmd, 0x00); break;
    case 0x37: /* TMF_GET_INFO: 25 B, no echoed command */
        memset(out, 0, 25);
        put_u32le(out, 1234);
        put_u32le(out + 4, f->frames_sent);
        out[20] = 1;
        out[21] = f->armed;
        out[22] = f->int_mask;
        out[23] = 1;
        send_locked(f, 0x92, out, 25);
        break;
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_tmf8829 *f = (fake_tmf8829 *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_tmf8829 *f = (fake_tmf8829 *)arg;
    uint8_t buf[2048];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_tmf8829 *fake_tmf8829_start(depz_link **host_link)
{
    fake_tmf8829 *f = (fake_tmf8829 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->reg[0xE3] = 0x9E; /* ID */
    f->reg[0x00] = 0x01; /* whatever ran before: init() must not care */
    f->skip_before = -1;
    f->mcu_time_us = 1000000;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_tmf8829_stop(fake_tmf8829 *f)
{
    int k;
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    for (k = 0; k < f->n_frames; k++) free(f->frames[k].raw);
    free(f);
}

void fake_tmf8829_set_base_page(fake_tmf8829 *f, const uint8_t page[190])
{
    depz_mutex_lock(&f->lock);
    memcpy(f->base_page, page, sizeof f->base_page);
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_pin_fov_correction(fake_tmf8829 *f, bool on)
{
    depz_mutex_lock(&f->lock);
    f->pin_fov = on;
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_add_frame(fake_tmf8829 *f, const uint8_t *raw, size_t len, uint64_t ts, uint64_t edge_us)
{
    depz_mutex_lock(&f->lock);
    if (f->n_frames < MAX_FRAMES) {
        script_frame *s = &f->frames[f->n_frames];
        s->raw = (uint8_t *)malloc(len);
        if (s->raw) {
            memcpy(s->raw, raw, len);
            s->len = len;
            s->ts = ts;
            s->edge = edge_us;
            f->n_frames++;
        }
    }
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_clear_frames(fake_tmf8829 *f)
{
    int k;
    depz_mutex_lock(&f->lock);
    for (k = 0; k < f->n_frames; k++) free(f->frames[k].raw);
    f->n_frames = 0;
    f->skip_before = -1;
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_skip_seq_before(fake_tmf8829 *f, int k)
{
    depz_mutex_lock(&f->lock);
    f->skip_before = k;
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_bad_header_on_stop(fake_tmf8829 *f, bool on)
{
    depz_mutex_lock(&f->lock);
    f->bad_header_on_stop = on;
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_send_event(fake_tmf8829 *f, uint8_t int_status, uint8_t code)
{
    depz_mutex_lock(&f->lock);
    event_locked(f, int_status, code, 0, 0);
    depz_mutex_unlock(&f->lock);
}

uint8_t fake_tmf8829_reg(fake_tmf8829 *f, uint8_t addr)
{
    uint8_t v;
    depz_mutex_lock(&f->lock);
    v = f->reg[addr];
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_tmf8829_set_reg(fake_tmf8829 *f, uint8_t addr, uint8_t v)
{
    depz_mutex_lock(&f->lock);
    f->reg[addr] = v;
    depz_mutex_unlock(&f->lock);
}

size_t fake_tmf8829_image(fake_tmf8829 *f, const uint8_t **data)
{
    size_t n;
    depz_mutex_lock(&f->lock);
    *data = f->image;
    n = f->image_len;
    depz_mutex_unlock(&f->lock);
    return n;
}

void fake_tmf8829_fifo_params(fake_tmf8829 *f, uint32_t *addr, uint16_t *words, size_t *writes, size_t *max_write)
{
    depz_mutex_lock(&f->lock);
    *addr = f->fifo_addr;
    *words = f->fifo_words;
    *writes = f->fifo_writes;
    *max_write = f->fifo_max;
    depz_mutex_unlock(&f->lock);
}

void fake_tmf8829_page(fake_tmf8829 *f, uint8_t out[190])
{
    depz_mutex_lock(&f->lock);
    memcpy(out, f->cfg, 190);
    depz_mutex_unlock(&f->lock);
}

size_t fake_tmf8829_reads(fake_tmf8829 *f, uint8_t *addr, uint16_t *len, size_t cap)
{
    size_t i, n;
    depz_mutex_lock(&f->lock);
    n = f->n_reads;
    for (i = 0; i < n && i < cap; i++) {
        addr[i] = f->read_addr[i];
        len[i] = f->read_len[i];
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

void fake_tmf8829_clear_reads(fake_tmf8829 *f)
{
    depz_mutex_lock(&f->lock);
    f->n_reads = 0;
    depz_mutex_unlock(&f->lock);
}

bool fake_tmf8829_armed(fake_tmf8829 *f, uint8_t *int_mask, uint8_t *flags)
{
    bool v;
    depz_mutex_lock(&f->lock);
    v = f->armed;
    if (int_mask) *int_mask = f->int_mask;
    if (flags) *flags = f->flags;
    depz_mutex_unlock(&f->lock);
    return v;
}
