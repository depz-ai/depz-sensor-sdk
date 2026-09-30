/* fake_bno086.c — see fake_bno086.h. */
#include "fake_bno086.h"

#include "../src/io/platform.h"

#include <stdlib.h>
#include <string.h>

#define MAX_RECORDS 8
#define MAX_WORDS   32

typedef struct { uint16_t type; uint32_t words[MAX_WORDS]; size_t n; bool used; } record;

struct fake_bno086 {
    depz_link  *side;
    depz_thread thread;
    depz_mutex  lock;
    depz_parser parser;
    uint8_t     tx_seq;
    uint8_t     shtp_seq[6];
    uint64_t    mcu_time_us;
    int         busy_left;
    unsigned    accepted, busy;
    bool        delay_disable;
    bool        held;               /* a disable answer is being held */
    uint8_t     held_gfr[17];
    uint32_t    interval[256];
    record      rec[MAX_RECORDS];
    uint8_t     last_params[256][9];
    bool        have_params[256];
    uint8_t     cal_status;
    bool        cal[4];
    /* FRS write in progress */
    uint16_t    wr_type;
    size_t      wr_len;
    uint32_t    wr_words[MAX_WORDS];
    size_t      wr_got;
};

static void put_u16le(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put_u32le(uint8_t *p, uint32_t v) { int i; for (i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put_u64le(uint8_t *p, uint64_t v) { int i; for (i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void send_locked(fake_bno086 *f, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[600];
    size_t n;
    if (depz_build_packet(cmd, payload, len, f->tx_seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(f->side, frame, n) == DEPZ_OK)
        f->tx_seq++;
}

static void status(fake_bno086 *f, uint8_t cmd, uint8_t st)
{
    uint8_t p[2];
    p[0] = cmd;
    p[1] = st;
    send_locked(f, DEPZ_RPT_STATUS, p, 2);
}

/* One SHTP frame on `channel` as RPT_DATA(cmd 0). */
static void shtp_out(fake_bno086 *f, uint8_t channel, uint64_t capture_us, const uint8_t *cargo, size_t len)
{
    uint8_t p[9 + 4 + 512];
    if (len > 512) return;
    p[0] = 0x00;
    put_u64le(p + 1, capture_us);
    put_u16le(p + 9, (uint16_t)(4 + len));
    p[11] = channel;
    p[12] = f->shtp_seq[channel]++;
    memcpy(p + 13, cargo, len);
    send_locked(f, 0x91, p, 13 + len);
}

static void control_out(fake_bno086 *f, const uint8_t *cargo, size_t len)
{
    shtp_out(f, 2, f->mcu_time_us, cargo, len);
}

static void gfr(fake_bno086 *f, uint8_t sensor, uint8_t out[17])
{
    memset(out, 0, 17);
    out[0] = 0xFC;
    out[1] = sensor;
    put_u32le(out + 5, f->interval[sensor]);
}

static void release_held(fake_bno086 *f)
{
    if (!f->held) return;
    f->held = false;
    control_out(f, f->held_gfr, 17);
}

static void command_response(fake_bno086 *f, uint8_t cmd, uint8_t cseq, uint8_t rseq, const uint8_t r[11])
{
    uint8_t out[16];
    out[0] = 0xF1;
    out[1] = 0;
    out[2] = cmd;
    out[3] = cseq;
    out[4] = rseq;
    memcpy(out + 5, r, 11);
    control_out(f, out, 16);
}

static record *find_record(fake_bno086 *f, uint16_t type, bool create)
{
    int i;
    for (i = 0; i < MAX_RECORDS; i++)
        if (f->rec[i].used && f->rec[i].type == type) return &f->rec[i];
    if (!create) return NULL;
    for (i = 0; i < MAX_RECORDS; i++) {
        if (!f->rec[i].used) {
            f->rec[i].used = true;
            f->rec[i].type = type;
            f->rec[i].n = 0;
            return &f->rec[i];
        }
    }
    return NULL;
}

static void frs_read_out(fake_bno086 *f, uint8_t status_, uint8_t nwords, uint16_t off, uint32_t d0, uint32_t d1,
                         uint16_t type)
{
    uint8_t out[16];
    memset(out, 0, sizeof out);
    out[0] = 0xF3;
    out[1] = (uint8_t)(nwords << 4 | status_);
    put_u16le(out + 2, off);
    put_u32le(out + 4, d0);
    put_u32le(out + 8, d1);
    put_u16le(out + 12, type);
    control_out(f, out, 16);
}

static void frs_write_out(fake_bno086 *f, uint8_t status_, uint16_t off)
{
    uint8_t out[4];
    out[0] = 0xF5;
    out[1] = status_;
    put_u16le(out + 2, off);
    control_out(f, out, 4);
}

static void sh2_command(fake_bno086 *f, const uint8_t *c)
{
    uint8_t cseq = c[1], cmd = c[2], r[11];
    const uint8_t *p = c + 3;
    memcpy(f->last_params[cmd], p, 9);
    f->have_params[cmd] = true;
    memset(r, 0, sizeof r);
    switch (cmd) {
    case 0x01: /* errors: one record, then the terminator */
        r[0] = 2; r[1] = 7; r[2] = 3; r[3] = 0x11; r[4] = 0x22; r[5] = 0x33;
        command_response(f, cmd, cseq, 0, r);
        memset(r, 0, sizeof r);
        r[2] = 255;
        command_response(f, cmd, cseq, 1, r);
        break;
    case 0x02: /* counts */
        if (p[0] == 0) {
            put_u32le(r + 3, 100 + p[1]);
            put_u32le(r + 7, 90 + p[1]);
            command_response(f, cmd, cseq, 0, r);
            put_u32le(r + 3, 80 + p[1]);
            put_u32le(r + 7, 70 + p[1]);
            command_response(f, cmd, cseq, 1, r);
        } else {
            command_response(f, cmd, cseq, 0, r);
        }
        break;
    case 0x06: /* DCD save */
        command_response(f, cmd, cseq, 0, r);
        break;
    case 0x07: /* ME calibration */
        if (p[3] == 0) {
            r[0] = f->cal_status;
            if (!f->cal_status) {
                f->cal[0] = p[0]; f->cal[1] = p[1]; f->cal[2] = p[2]; f->cal[3] = p[4];
            }
        } else {
            r[1] = f->cal[0]; r[2] = f->cal[1]; r[3] = f->cal[2]; r[4] = f->cal[3];
        }
        command_response(f, cmd, cseq, 0, r);
        break;
    case 0x0A: /* oscillator */
        r[0] = 1;
        command_response(f, cmd, cseq, 0, r);
        break;
    case 0x0B: { /* clear DCD and reset: the hub restarts */
        static const uint8_t done = 0x01;
        memset(f->shtp_seq, 0, sizeof f->shtp_seq);
        shtp_out(f, 1, f->mcu_time_us, &done, 1);
        break;
    }
    default: /* tare (3), periodic DCD (9): no answer */
        break;
    }
}

static void sh2_control(fake_bno086 *f, const uint8_t *c, size_t len)
{
    uint8_t out[32];
    if (!len) return;
    switch (c[0]) {
    case 0xF9: /* product id: two subsystems */
        memset(out, 0, 16);
        out[0] = 0xF8; out[1] = 1; out[2] = 3; out[3] = 7;
        put_u32le(out + 4, FAKE_BNO086_PART);
        put_u32le(out + 8, 12);
        put_u16le(out + 12, 5);
        control_out(f, out, 16);
        put_u32le(out + 4, 10004384u);
        control_out(f, out, 16);
        break;
    case 0xFD: { /* set feature: answered unsolicited */
        uint32_t iv = rd_u32(c + 5);
        /* The grid: a rate rounds up to 1000 / 2^n Hz. */
        uint32_t granted = 0;
        if (iv) {
            granted = 1000;
            while (granted * 2 <= iv) granted *= 2;
        }
        f->interval[c[1]] = granted;
        gfr(f, c[1], out);
        if (!iv && f->delay_disable) {
            memcpy(f->held_gfr, out, 17);
            f->held = true;
        } else {
            control_out(f, out, 17);
        }
        break;
    }
    case 0xFE: /* get feature */
        release_held(f);
        gfr(f, c[1], out);
        control_out(f, out, 17);
        break;
    case 0xF2:
        if (len >= 12) sh2_command(f, c);
        break;
    case 0xF4: { /* FRS read: two words per answer */
        uint16_t type = rd_u16(c + 4);
        record *r = find_record(f, type, false);
        size_t off;
        if (!r) {
            frs_read_out(f, 1, 0, 0, 0, 0, type);
            break;
        }
        if (!r->n) {
            frs_read_out(f, 5, 0, 0, 0, 0, type);
            break;
        }
        for (off = 0; off < r->n; off += 2) {
            size_t k = r->n - off < 2 ? r->n - off : 2;
            bool last = off + k >= r->n;
            frs_read_out(f, last ? 3 : 0, (uint8_t)k, (uint16_t)off, r->words[off], k > 1 ? r->words[off + 1] : 0,
                         type);
        }
        break;
    }
    case 0xF7: /* FRS write request */
        f->wr_type = rd_u16(c + 4);
        f->wr_len = rd_u16(c + 2);
        f->wr_got = 0;
        if (f->wr_len > MAX_WORDS) {
            frs_write_out(f, 7, 0);
            break;
        }
        if (!f->wr_len) { /* erase */
            record *r = find_record(f, f->wr_type, true);
            if (r) r->n = 0;
            frs_write_out(f, 3, 0);
            break;
        }
        frs_write_out(f, 4, 0);
        break;
    case 0xF6: { /* FRS write data */
        uint16_t off = rd_u16(c + 2);
        size_t k;
        for (k = 0; k < 2 && off + k < f->wr_len; k++) f->wr_words[off + k] = rd_u32(c + 4 + 4 * k);
        f->wr_got = off + k;
        if (f->wr_got >= f->wr_len) {
            record *r = find_record(f, f->wr_type, true);
            if (r) {
                memcpy(r->words, f->wr_words, f->wr_len * sizeof(uint32_t));
                r->n = f->wr_len;
            }
            frs_write_out(f, 3, off);
        } else {
            frs_write_out(f, 0, off);
        }
        break;
    }
    default:
        break;
    }
}

static void handle(fake_bno086 *f, uint8_t cmd, const uint8_t *p, size_t len)
{
    uint8_t out[64];
    f->mcu_time_us += 1000;
    switch (cmd) {
    case 0x03: {
        static const char name[] = "DEPZ IMU BNO086 USB v2.1 FAKE";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x04: {
        static const char name[] = "APP_BNO086_v0.99";
        out[0] = cmd;
        memcpy(out + 1, name, sizeof name);
        send_locked(f, DEPZ_RPT_TEXT, out, 1 + sizeof name);
        break;
    }
    case 0x32: { /* SENSOR_RESET */
        static const uint8_t done = 0x01;
        memset(f->shtp_seq, 0, sizeof f->shtp_seq);
        memset(f->interval, 0, sizeof f->interval);
        f->held = false;
        status(f, cmd, 0x00);
        shtp_out(f, 1, f->mcu_time_us, &done, 1);
        break;
    }
    case 0x33: status(f, cmd, 0x00); break;
    case 0x34: /* SEND_SHTP_PACKET */
        if (f->busy_left > 0) {
            f->busy_left--;
            f->busy++;
            status(f, cmd, DEPZ_STATUS_ERR_BUSY);
            break;
        }
        f->accepted++;
        status(f, cmd, 0x00);
        if (len >= 4 && p[2] == 2) sh2_control(f, p + 4, len - 4);
        break;
    default: status(f, cmd, 0x02);
    }
}

static void on_event(const depz_event *e, void *user)
{
    fake_bno086 *f = (fake_bno086 *)user;
    if (e->type != DEPZ_EV_PACKET) return;
    depz_mutex_lock(&f->lock);
    handle(f, e->cmd, e->payload, e->payload_len);
    depz_mutex_unlock(&f->lock);
}

static void run(void *arg)
{
    fake_bno086 *f = (fake_bno086 *)arg;
    uint8_t buf[512];
    for (;;) {
        size_t got;
        if (depz_link_read(f->side, buf, sizeof buf, 1000, &got) != DEPZ_OK) return;
        if (got) depz_parser_feed(&f->parser, buf, got, on_event, f);
    }
}

fake_bno086 *fake_bno086_start(depz_link **host_link)
{
    fake_bno086 *f = (fake_bno086 *)calloc(1, sizeof *f);
    if (!f) return NULL;
    if (depz_link_loopback_pair(host_link, &f->side)) { free(f); return NULL; }
    depz_mutex_init(&f->lock);
    depz_parser_init(&f->parser);
    f->mcu_time_us = 5000000;
    f->cal[0] = f->cal[2] = true;
    depz_thread_start(&f->thread, run, f);
    return f;
}

void fake_bno086_stop(fake_bno086 *f)
{
    if (!f) return;
    depz_link_close(f->side);
    depz_thread_join(f->thread);
    depz_link_free(f->side);
    depz_parser_free(&f->parser);
    depz_mutex_destroy(&f->lock);
    free(f);
}

void fake_bno086_busy_next(fake_bno086 *f, int n)
{
    depz_mutex_lock(&f->lock);
    f->busy_left = n;
    depz_mutex_unlock(&f->lock);
}

unsigned fake_bno086_frames_accepted(fake_bno086 *f)
{
    unsigned n;
    depz_mutex_lock(&f->lock);
    n = f->accepted;
    depz_mutex_unlock(&f->lock);
    return n;
}

unsigned fake_bno086_frames_busy(fake_bno086 *f)
{
    unsigned n;
    depz_mutex_lock(&f->lock);
    n = f->busy;
    depz_mutex_unlock(&f->lock);
    return n;
}

void fake_bno086_delay_disable_answer(fake_bno086 *f, bool on)
{
    depz_mutex_lock(&f->lock);
    f->delay_disable = on;
    depz_mutex_unlock(&f->lock);
}

uint32_t fake_bno086_interval(fake_bno086 *f, uint8_t sensor)
{
    uint32_t v;
    depz_mutex_lock(&f->lock);
    v = f->interval[sensor];
    depz_mutex_unlock(&f->lock);
    return v;
}

void fake_bno086_set_record(fake_bno086 *f, uint16_t type, const uint32_t *words, size_t n)
{
    record *r;
    depz_mutex_lock(&f->lock);
    r = find_record(f, type, true);
    if (r) {
        r->n = n < MAX_WORDS ? n : MAX_WORDS;
        memcpy(r->words, words, r->n * sizeof *words);
    }
    depz_mutex_unlock(&f->lock);
}

size_t fake_bno086_record(fake_bno086 *f, uint16_t type, uint32_t *out, size_t cap)
{
    record *r;
    size_t n = 0;
    depz_mutex_lock(&f->lock);
    r = find_record(f, type, false);
    if (r) {
        n = r->n;
        memcpy(out, r->words, (n < cap ? n : cap) * sizeof *out);
    }
    depz_mutex_unlock(&f->lock);
    return n;
}

bool fake_bno086_last_command(fake_bno086 *f, uint8_t command, uint8_t params[9])
{
    bool have;
    depz_mutex_lock(&f->lock);
    have = f->have_params[command];
    if (have) memcpy(params, f->last_params[command], 9);
    depz_mutex_unlock(&f->lock);
    return have;
}

void fake_bno086_calibration_status(fake_bno086 *f, uint8_t status_)
{
    depz_mutex_lock(&f->lock);
    f->cal_status = status_;
    depz_mutex_unlock(&f->lock);
}

void fake_bno086_send_input(fake_bno086 *f, uint8_t channel, uint64_t capture_us, const uint8_t *cargo, size_t len)
{
    depz_mutex_lock(&f->lock);
    shtp_out(f, channel, capture_us, cargo, len);
    depz_mutex_unlock(&f->lock);
}
