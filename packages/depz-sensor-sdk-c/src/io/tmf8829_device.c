/* tmf8829_device.c — the TMF8829 class (contract 14): the host side of the
 * SPI register bridge — soft reset, firmware download through the sensor
 * bootloader, presets and the config page, start / stop — and the chunked
 * frame stream turned into measurements, as the Python SDK's
 * tmf8829/__init__.py does it: same request sequences, so a capture made by
 * either SDK replays in the other byte for byte. */
#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* READ/WRITE_REG may wait behind a 25 KB histogram frame (contract 14 §2). */
#define REGISTER_TIMEOUT_MS 1000
#define BL_TIMEOUT_MS       100
#define APP_TIMEOUT_MS      1000

#define APP_ID_APP          0x01u
#define APP_ID_BL           0x80u
#define ENABLE_CPU_READY    0x80u
#define ENABLE_PUSEL_MASK   0x30u
#define ENABLE_PUSEL_BOOTMON 0x10u
#define ENABLE_PUSEL_RAM    0x20u
#define ENABLE_PON          0x04u
#define RESET_SOFT          0x40u
#define FIFO_ERROR_FLAGS    0x30u

#define BL_START_RAM_APP    0x16u
#define BL_I2C_OFF          0x22u
#define BL_W_FIFO_BOTH      0x45u

typedef struct {
    depz_mutex seq;               /* serialises multi-step register sequences */
    depz_mutex lock;              /* the fields below shared with the reader */
    depz_tmf8829_reassembler *reasm;
    depz_tmf8829_builder *builder;
    bool streaming, stopping;
    uint64_t chunk_errors, measurements, events_swallowed;
    /* reader-thread scratch: the frame and measurement handed to callbacks */
    uint8_t *frame_buf;
    depz_tmf8829_measurement *meas;
    /* caller side, under `seq` */
    bool initialized;
    char app_version[16];
    uint32_t serial;
    bool configured;
    depz_tmf8829_config config;
    depz_cb_list meas_cbs, frame_cbs, event_cbs;
    depz_hub *hub;
} tmf_state;

typedef void (*meas_fn)(const depz_tmf8829_measurement *, void *);
typedef void (*frame_fn)(const depz_tmf8829_raw_frame *, void *);
typedef void (*event_fn)(const depz_tmf8829_event *, void *);

static tmf_state *st(depz_device *dev) { return (tmf_state *)dev->sensor; }

#define TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

/* ── reports (reader thread) ──────────────────────────────────────────── */

static void on_chunk(depz_device *dev, const uint8_t *p, size_t len)
{
    tmf_state *s = st(dev);
    depz_tmf8829_frame_chunk ch;
    depz_tmf8829_raw_frame raw;
    depz_cb_entry *cbs;
    size_t i, n;
    int done, got = 0;
    if (depz_tmf8829_unpack_frame_chunk(p, len, &ch)) {
        depz_mutex_lock(&s->lock);
        s->chunk_errors++;
        depz_mutex_unlock(&s->lock);
        return;
    }
    depz_mutex_lock(&s->lock);
    done = depz_tmf8829_reassembler_feed(s->reasm, &ch, &raw);
    if (done) {
        /* Copy out: start() may reset the reassembler once the lock goes. */
        memcpy(s->frame_buf, raw.data, raw.len);
        raw.data = s->frame_buf;
        got = depz_tmf8829_builder_add(s->builder, raw.data, raw.len, (int64_t)raw.timestamp_us,
                                       (int64_t)raw.edge_us, s->meas);
        if (got == 1) s->measurements++;
        else got = 0;
    }
    depz_mutex_unlock(&s->lock);
    if (!done) return;
    n = depz_cb_list_snapshot(&s->frame_cbs, &cbs);
    for (i = 0; i < n; i++) ((frame_fn)cbs[i].fn)(&raw, cbs[i].user);
    free(cbs);
    if (!got) return;
    n = depz_cb_list_snapshot(&s->meas_cbs, &cbs);
    for (i = 0; i < n; i++) ((meas_fn)cbs[i].fn)(s->meas, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, s->meas);
}

static void on_event_report(depz_device *dev, const uint8_t *p, size_t len)
{
    tmf_state *s = st(dev);
    depz_tmf8829_event ev;
    depz_cb_entry *cbs;
    size_t i, n;
    bool swallow;
    depz_mutex_lock(&s->lock);
    if (depz_tmf8829_unpack_event(p, len, &ev)) {
        s->chunk_errors++;
        depz_mutex_unlock(&s->lock);
        return;
    }
    /* STOP mid histogram dump: expected (contract 14 §2). */
    swallow = ev.code == DEPZ_TMF8829_EV_BAD_HEADER && s->stopping;
    if (swallow) s->events_swallowed++;
    depz_mutex_unlock(&s->lock);
    if (swallow) return;
    n = depz_cb_list_snapshot(&s->event_cbs, &cbs);
    for (i = 0; i < n; i++) ((event_fn)cbs[i].fn)(&ev, cbs[i].user);
    free(cbs);
}

static bool tmf_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    if (cmd == DEPZ_TMF8829_RPT_FRAME) { on_chunk(dev, p, len); return true; }
    if (cmd == DEPZ_TMF8829_RPT_EVENT) { on_event_report(dev, p, len); return true; }
    return false;
}

/* ── class plumbing ────────────────────────────────────────────────────── */

static void tmf_closed(depz_device *dev) { depz_hub_mark_closed(st(dev)->hub); }

static void tmf_free(tmf_state *s)
{
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->meas_cbs);
    depz_cb_list_free(&s->frame_cbs);
    depz_cb_list_free(&s->event_cbs);
    depz_mutex_destroy(&s->lock);
    depz_mutex_destroy(&s->seq);
    free(s->reasm);
    free(s->builder);
    free(s->frame_buf);
    free(s->meas);
    free(s);
}

static void tmf_destroy(depz_device *dev)
{
    tmf_state *s = st(dev);
    if (!s) return;
    tmf_free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops tmf_ops = {DEPZ_SENSOR_TMF8829, tmf_report, tmf_closed, tmf_destroy};

static tmf_state *tmf_new(void)
{
    tmf_state *s = (tmf_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->lock)) { free(s); return NULL; }
    if (depz_mutex_init(&s->seq)) { depz_mutex_destroy(&s->lock); free(s); return NULL; }
    if (depz_cb_list_init(&s->meas_cbs) || depz_cb_list_init(&s->frame_cbs) || depz_cb_list_init(&s->event_cbs))
        goto fail;
    s->reasm = (depz_tmf8829_reassembler *)malloc(sizeof *s->reasm);
    s->builder = (depz_tmf8829_builder *)malloc(sizeof *s->builder);
    s->frame_buf = (uint8_t *)malloc(DEPZ_TMF8829_FRAME_MAX);
    s->meas = (depz_tmf8829_measurement *)malloc(sizeof *s->meas);
    s->hub = depz_hub_new(sizeof(depz_tmf8829_measurement));
    if (!s->reasm || !s->builder || !s->frame_buf || !s->meas || !s->hub) goto fail;
    depz_tmf8829_reassembler_reset(s->reasm);
    depz_tmf8829_builder_reset(s->builder);
    return s;
fail:
    tmf_free(s);
    return NULL;
}

int depz_tmf8829_attach(depz_device *dev)
{
    tmf_state *s;
    if (dev->ops == &tmf_ops) return DEPZ_OK;
    s = tmf_new();
    if (!s) return depz_fail(DEPZ_E_NOMEM, "tmf8829: out of memory");
    depz_device_attach(dev, &tmf_ops, s);
    return DEPZ_OK;
}

int depz_tmf8829_open_link(depz_link *link, depz_device **out)
{
    tmf_state *s = tmf_new();
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "tmf8829: out of memory");
    }
    return depz_device_create(link, &tmf_ops, s, out);
}

bool depz_is_tmf8829(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &tmf_ops;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

static int need(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL device");
    if (!depz_is_tmf8829(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a TMF8829", dev->port);
    return DEPZ_OK;
}

static bool is_streaming(depz_device *dev)
{
    tmf_state *s = st(dev);
    bool v;
    depz_mutex_lock(&s->lock);
    v = s->streaming;
    depz_mutex_unlock(&s->lock);
    return v;
}

static void set_flag(depz_device *dev, bool *flag, bool v)
{
    tmf_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    *flag = v;
    depz_mutex_unlock(&s->lock);
}

/* Public entry: serialise the whole sequence. */
#define LOCKED(body)                          \
    do {                                      \
        int rc_;                              \
        TRY(need(dev));                       \
        depz_mutex_lock(&st(dev)->seq);       \
        rc_ = (body);                         \
        depz_mutex_unlock(&st(dev)->seq);     \
        return rc_;                           \
    } while (0)

/* ── register access (callers hold `seq`) ─────────────────────────────── */

typedef struct { uint8_t *out; size_t want, got; } rd_ctx;

static bool match_reg(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    rd_ctx *r = (rd_ctx *)c;
    depz_tmf8829_reg_data rep;
    if (cmd != DEPZ_TMF8829_RPT_REG_DATA || depz_tmf8829_unpack_reg_data(p, len, &rep) != 0) return false;
    r->got = rep.data_len;
    memcpy(r->out, rep.data, rep.data_len < r->want ? rep.data_len : r->want);
    return true;
}

/* READ_REG in <= 1024-byte pieces; a read that reaches 0xFF stays there. */
static int rd(depz_device *dev, unsigned addr, uint8_t *out, size_t len)
{
    while (len) {
        size_t n = len < DEPZ_TMF8829_READ_MAX ? len : DEPZ_TMF8829_READ_MAX;
        uint8_t p[3];
        rd_ctx c;
        c.out = out;
        c.want = n;
        c.got = 0;
        depz_tmf8829_pack_read_reg((int)addr, (int)n, p);
        TRY(depz_device_request(dev, DEPZ_TMF8829_CMD_READ_REG, p, 3, match_reg, &c, false, REGISTER_TIMEOUT_MS));
        if (c.got != n) return depz_fail(DEPZ_E_PROTOCOL, "READ_REG 0x%02X: expected %zu, got %zu", addr, n, c.got);
        out += n;
        len -= n;
        addr = addr + n < DEPZ_TMF8829_REG_FIFO ? addr + (unsigned)n : DEPZ_TMF8829_REG_FIFO;
    }
    return DEPZ_OK;
}

static int rd1(depz_device *dev, unsigned addr, uint8_t *v) { return rd(dev, addr, v, 1); }

/* WRITE_REG in <= 1023-byte pieces; writes to 0xFF stay at 0xFF. */
static int wr(depz_device *dev, unsigned addr, const uint8_t *data, size_t len,
              depz_tmf8829_progress_cb progress, void *user)
{
    uint8_t p[1 + DEPZ_TMF8829_WRITE_MAX];
    size_t done = 0;
    while (done < len) {
        size_t n = len - done < DEPZ_TMF8829_WRITE_MAX ? len - done : DEPZ_TMF8829_WRITE_MAX;
        size_t plen = depz_tmf8829_pack_write_reg((int)addr, data + done, n, p);
        TRY(depz_device_request(dev, DEPZ_TMF8829_CMD_WRITE_REG, p, plen, NULL, NULL, true, REGISTER_TIMEOUT_MS));
        done += n;
        if (addr != DEPZ_TMF8829_REG_FIFO) addr += (unsigned)n;
        if (progress) progress(done, len, user);
    }
    return DEPZ_OK;
}

static int wr1(depz_device *dev, unsigned addr, uint8_t v) { return wr(dev, addr, &v, 1, NULL, NULL); }

/* Poll a register until `done(v)`; DEPZ_E_TIMEOUT after `timeout_ms`. */
typedef enum { UNTIL_EQ, UNTIL_NE, UNTIL_BITS, UNTIL_BELOW } until_kind;

static int poll_reg(depz_device *dev, unsigned addr, until_kind k, uint8_t ref, int timeout_ms, const char *what,
                    uint8_t *value)
{
    uint64_t deadline = depz_now_us() + (uint64_t)timeout_ms * 1000u;
    for (;;) {
        uint8_t v;
        bool ok;
        TRY(rd1(dev, addr, &v));
        ok = k == UNTIL_EQ ? v == ref : k == UNTIL_NE ? v != ref : k == UNTIL_BITS ? (v & ref) != 0 : v < ref;
        if (ok) {
            if (value) *value = v;
            return DEPZ_OK;
        }
        if (depz_now_us() > deadline)
            return depz_fail(DEPZ_E_TIMEOUT, "tmf8829 %s: timeout, reg 0x%02X = 0x%02X", what, addr, v);
    }
}

/* ── init: sensor bootloader + firmware download ──────────────────────── */

static const char *bl_status_name(uint8_t st)
{
    switch (st) {
    case 0: return "READY";
    case 1: return "ERR_PARAM";
    case 2: return "ERR_ADDR";
    case 4: return "ERR_FIFO";
    default: return NULL;
    }
}

/* [cmd] or [cmd, len, params...] to CMD_STAT; the status replaces the command. */
static int bl_cmd(depz_device *dev, uint8_t cmd, const uint8_t *params, size_t plen)
{
    uint8_t p[2 + 16], v = 0;
    char what[40];
    size_t n = 1;
    p[0] = cmd;
    if (plen) {
        p[1] = (uint8_t)plen;
        memcpy(p + 2, params, plen);
        n = 2 + plen;
    }
    TRY(wr(dev, DEPZ_TMF8829_REG_CMD_STAT, p, n, NULL, NULL));
    snprintf(what, sizeof what, "bootloader cmd 0x%02X", cmd);
    TRY(poll_reg(dev, DEPZ_TMF8829_REG_CMD_STAT, UNTIL_NE, cmd, BL_TIMEOUT_MS, what, &v));
    if (v != 0) {
        const char *nm = bl_status_name(v);
        if (nm) return depz_fail(DEPZ_E_PROTOCOL, "tmf8829 bootloader cmd 0x%02X -> %s", cmd, nm);
        return depz_fail(DEPZ_E_PROTOCOL, "tmf8829 bootloader cmd 0x%02X -> 0x%02X", cmd, v);
    }
    return DEPZ_OK;
}

static int do_stop(depz_device *dev);

static int do_init(depz_device *dev, depz_tmf8829_progress_cb progress, void *user)
{
    tmf_state *s = st(dev);
    uint8_t v, ver[4], ser[4], params[6], en;
    if (is_streaming(dev)) TRY(do_stop(dev));
    TRY(wr1(dev, DEPZ_TMF8829_REG_ENABLE, ENABLE_PON | ENABLE_PUSEL_BOOTMON));
    depz_device_sleep_ms(dev, 3);
    TRY(wr1(dev, DEPZ_TMF8829_REG_RESET, RESET_SOFT));
    depz_device_sleep_ms(dev, 3);
    TRY(rd1(dev, DEPZ_TMF8829_REG_ID, &v));
    if (v != DEPZ_TMF8829_DEVICE_ID)
        return depz_fail(DEPZ_E_PROTOCOL, "tmf8829: sensor ID 0x%02X, expected 0x%02X (no TMF8829 on the bus?)", v,
                         DEPZ_TMF8829_DEVICE_ID);
    TRY(poll_reg(dev, DEPZ_TMF8829_REG_APP_ID, UNTIL_EQ, APP_ID_BL, BL_TIMEOUT_MS, "sensor bootloader", NULL));
    TRY(bl_cmd(dev, BL_I2C_OFF, NULL, 0));
    /* ENABLE reads 0 over SPI until the sensor's I2C is off. */
    TRY(poll_reg(dev, DEPZ_TMF8829_REG_ENABLE, UNTIL_BITS, ENABLE_CPU_READY, BL_TIMEOUT_MS, "cpu_ready", NULL));
    /* The image is a multiple of 4 already (contract 14 §4): no padding. */
    params[0] = (uint8_t)DEPZ_TMF8829_FIRMWARE_LOAD_ADDR;
    params[1] = (uint8_t)(DEPZ_TMF8829_FIRMWARE_LOAD_ADDR >> 8);
    params[2] = (uint8_t)(DEPZ_TMF8829_FIRMWARE_LOAD_ADDR >> 16);
    params[3] = (uint8_t)(DEPZ_TMF8829_FIRMWARE_LOAD_ADDR >> 24);
    params[4] = (uint8_t)(depz_tmf8829_firmware_len / 4);
    params[5] = (uint8_t)((depz_tmf8829_firmware_len / 4) >> 8);
    TRY(bl_cmd(dev, BL_W_FIFO_BOTH, params, sizeof params));
    TRY(wr(dev, DEPZ_TMF8829_REG_FIFO, depz_tmf8829_firmware, depz_tmf8829_firmware_len, progress, user));
    TRY(bl_cmd(dev, BL_START_RAM_APP, NULL, 0));
    TRY(poll_reg(dev, DEPZ_TMF8829_REG_APP_ID, UNTIL_EQ, APP_ID_APP, BL_TIMEOUT_MS, "START_RAM_APP", NULL));
    /* Wake-ups must boot the RAM application. */
    TRY(rd1(dev, DEPZ_TMF8829_REG_ENABLE, &en));
    TRY(wr1(dev, DEPZ_TMF8829_REG_ENABLE, (uint8_t)((en & ~ENABLE_PUSEL_MASK) | ENABLE_PUSEL_RAM)));
    TRY(rd(dev, DEPZ_TMF8829_REG_APP_ID, ver, 4));
    TRY(rd(dev, DEPZ_TMF8829_REG_SERIAL, ser, 4));
    snprintf(s->app_version, sizeof s->app_version, "%u.%u.%u", ver[1], ver[2], ver[3]);
    s->serial = (uint32_t)ser[0] | (uint32_t)ser[1] << 8 | (uint32_t)ser[2] << 16 | (uint32_t)ser[3] << 24;
    s->initialized = true;
    s->configured = false;
    return DEPZ_OK;
}

static int copy_id(depz_device *dev, char *app_version, size_t cap, uint32_t *serial)
{
    tmf_state *s = st(dev);
    if (!s->initialized) return depz_fail(DEPZ_E_ARG, "tmf8829: init() first");
    if (app_version && cap) depz_strlcpy(app_version, s->app_version, cap);
    if (serial) *serial = s->serial;
    return DEPZ_OK;
}

static int do_init_id(depz_device *dev, depz_tmf8829_progress_cb progress, void *user, char *app_version,
                      size_t cap, uint32_t *serial)
{
    TRY(do_init(dev, progress, user));
    return copy_id(dev, app_version, cap, serial);
}

int depz_tmf8829_init(depz_device *dev, depz_tmf8829_progress_cb progress, void *user, char *app_version,
                      size_t cap, uint32_t *serial)
{
    LOCKED(do_init_id(dev, progress, user, app_version, cap, serial));
}

int depz_tmf8829_sensor_id(depz_device *dev, char *app_version, size_t cap, uint32_t *serial)
{
    LOCKED(copy_id(dev, app_version, cap, serial));
}

/* ── application commands, config page ────────────────────────────────── */

/* Write `cmd` to CMD_STAT, wait until it reads < 0x10; OK (or ACCEPTED). */
static int app_cmd(depz_device *dev, uint8_t cmd, bool accepted_ok)
{
    char what[32];
    uint8_t v = 0;
    TRY(wr1(dev, DEPZ_TMF8829_REG_CMD_STAT, cmd));
    snprintf(what, sizeof what, "sensor cmd 0x%02X", cmd);
    TRY(poll_reg(dev, DEPZ_TMF8829_REG_CMD_STAT, UNTIL_BELOW, 0x10, APP_TIMEOUT_MS, what, &v));
    if (v != DEPZ_TMF8829_STAT_OK && !(accepted_ok && v == DEPZ_TMF8829_STAT_ACCEPTED)) {
        const char *nm = depz_tmf8829_status_name(v);
        if (nm) return depz_fail(DEPZ_E_PROTOCOL, "tmf8829 sensor cmd 0x%02X -> %s", cmd, nm);
        return depz_fail(DEPZ_E_PROTOCOL, "tmf8829 sensor cmd 0x%02X -> 0x%02X", cmd, v);
    }
    return DEPZ_OK;
}

static int do_config_page(depz_device *dev, uint8_t page[DEPZ_TMF8829_CFG_SIZE])
{
    uint8_t cid;
    TRY(app_cmd(dev, DEPZ_TMF8829_CMD_LOAD_CONFIG_PAGE, false));
    TRY(rd1(dev, DEPZ_TMF8829_REG_CID_RID, &cid));
    if (cid != DEPZ_TMF8829_CMD_LOAD_CONFIG_PAGE)
        return depz_fail(DEPZ_E_PROTOCOL, "tmf8829: CID_RID 0x%02X after LOAD_CONFIG_PAGE", cid);
    return rd(dev, DEPZ_TMF8829_REG_CFG, page, DEPZ_TMF8829_CFG_SIZE);
}

int depz_tmf8829_config_page(depz_device *dev, uint8_t page[DEPZ_TMF8829_CFG_SIZE])
{
    if (!page) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL page");
    LOCKED(do_config_page(dev, page));
}

#define WANT_MAX (8 + 32)

typedef struct { const char *name; int64_t value; } want_t;

static void want_put(want_t *w, size_t *n, const char *name, int64_t value)
{
    size_t i;
    for (i = 0; i < *n; i++)
        if (!strcmp(w[i].name, name)) { w[i].value = value; return; }
    w[*n].name = name;
    w[*n].value = value;
    (*n)++;
}

static int do_configure(depz_device *dev, const char *preset, const depz_tmf8829_options *opt,
                        depz_tmf8829_config *out)
{
    static const depz_tmf8829_options defaults = DEPZ_TMF8829_OPTIONS_INIT;
    tmf_state *s = st(dev);
    const depz_tmf8829_preset *ps = depz_tmf8829_find_preset(preset);
    want_t want[WANT_MAX];
    uint8_t page[DEPZ_TMF8829_CFG_SIZE], back[DEPZ_TMF8829_CFG_SIZE];
    char stuck[256];
    size_t nw = 0, i, at = 0;
    depz_tmf8829_config cfg;
    if (!opt) opt = &defaults;
    if (!ps) return depz_fail(DEPZ_E_ARG, "tmf8829: unknown preset '%s'", preset ? preset : "(null)");
    if (is_streaming(dev)) return depz_fail(DEPZ_E_ARG, "tmf8829: configure() while streaming: stop() first");
    if (opt->n_fields > WANT_MAX - 8) return depz_fail(DEPZ_E_ARG, "tmf8829: too many fields (%zu)", opt->n_fields);
    /* The 3-bit field takes 0..7, the sensor reports at most 4 peaks. */
    if (opt->nr_peaks < 1 || opt->nr_peaks > 4) return depz_fail(DEPZ_E_ARG, "tmf8829: nr_peaks %d, must be 1..4", opt->nr_peaks);
    want_put(want, &nw, "period_ms", opt->period_ms);
    want_put(want, &nw, "kilo_iterations", opt->kilo_iterations < 0 ? ps->kilo_iterations : opt->kilo_iterations);
    want_put(want, &nw, "nr_peaks", opt->nr_peaks);
    want_put(want, &nw, "signal_strength", opt->signal_strength);
    want_put(want, &nw, "noise_strength", opt->noise_strength);
    want_put(want, &nw, "xtalk", opt->xtalk);
    want_put(want, &nw, "dump_histograms", opt->histograms);
    want_put(want, &nw, "ref_spad_frame", opt->ref_frame);
    for (i = 0; i < opt->n_fields; i++) {
        if (!opt->fields || !opt->fields[i].name)
            return depz_fail(DEPZ_E_ARG, "tmf8829: field %zu has no name", i);
        want_put(want, &nw, opt->fields[i].name, opt->fields[i].value);
    }
    /* Refuse a bad name / value before anything reaches the sensor. */
    memset(page, 0, sizeof page);
    for (i = 0; i < nw; i++) {
        int rc = depz_tmf8829_cfg_set(page, want[i].name, want[i].value);
        if (rc == -1) return depz_fail(DEPZ_E_ARG, "tmf8829: unknown config field '%s'", want[i].name);
        if (rc) {
            const depz_tmf8829_cfg_field *f = depz_tmf8829_find_cfg_field(want[i].name);
            return depz_fail(DEPZ_E_ARG, "tmf8829: %s: %lld out of range for %u bit", want[i].name,
                             (long long)want[i].value, f->width);
        }
    }

    TRY(app_cmd(dev, ps->cmd, false));
    TRY(do_config_page(dev, page));
    for (i = 0; i < nw; i++) depz_tmf8829_cfg_set(page, want[i].name, want[i].value);
    TRY(wr(dev, DEPZ_TMF8829_REG_CFG, page, sizeof page, NULL, NULL));
    TRY(app_cmd(dev, DEPZ_TMF8829_CMD_WRITE_PAGE, false));
    TRY(do_config_page(dev, back));

    stuck[0] = '\0';
    for (i = 0; i < nw; i++) {
        uint32_t v = 0;
        depz_tmf8829_cfg_get(back, want[i].name, &v);
        if ((int64_t)v != want[i].value && at < sizeof stuck) {
            int w = snprintf(stuck + at, sizeof stuck - at, "%s%s wrote %lld, reads %u", at ? ", " : "",
                             want[i].name, (long long)want[i].value, v);
            if (w > 0) at += (size_t)w;
        }
    }
    if (at) return depz_fail(DEPZ_E_PROTOCOL, "tmf8829: config fields did not stick: %s", stuck);

    memset(&cfg, 0, sizeof cfg);
    depz_strlcpy(cfg.preset, ps->name, sizeof cfg.preset);
    memcpy(cfg.page, back, sizeof back);
    cfg.histograms = opt->histograms;
    cfg.ref_frame = opt->ref_frame;
    for (i = 0; i < DEPZ_TMF8829_CFG_FIELD_COUNT; i++)
        depz_tmf8829_cfg_get(back, DEPZ_TMF8829_CFG_FIELDS[i].name, &cfg.fields[i]);
    {
        uint32_t fp = 0;
        depz_tmf8829_cfg_get(back, "fp_mode", &fp);
        depz_tmf8829_fp_grid(fp, &cfg.rows, &cfg.cols);
    }
    s->config = cfg;
    s->configured = true;
    if (out) *out = cfg;
    return DEPZ_OK;
}

int depz_tmf8829_configure(depz_device *dev, const char *preset, const depz_tmf8829_options *opt,
                           depz_tmf8829_config *out)
{
    LOCKED(do_configure(dev, preset, opt, out));
}

static int do_get_config(depz_device *dev, depz_tmf8829_config *out)
{
    tmf_state *s = st(dev);
    if (!s->configured) return depz_fail(DEPZ_E_ARG, "tmf8829: not configured since init()");
    *out = s->config;
    return DEPZ_OK;
}

int depz_tmf8829_get_config(depz_device *dev, depz_tmf8829_config *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL config");
    LOCKED(do_get_config(dev, out));
}

/* ── raw registers, bridge ─────────────────────────────────────────────── */

int depz_tmf8829_read_registers(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len)
{
    if (!buf || !len) return depz_fail(DEPZ_E_ARG, "tmf8829: read of nothing");
    LOCKED(rd(dev, addr, buf, len));
}

int depz_tmf8829_write_registers(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len)
{
    if (!data || !len) return depz_fail(DEPZ_E_ARG, "tmf8829: write of nothing");
    LOCKED(wr(dev, addr, data, len, NULL, NULL));
}

typedef struct { depz_tmf8829_info *out; bool ok; } info_ctx;

/* RPT_TMF_INFO carries no echoed command byte: match by report id alone. */
static bool match_info(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    info_ctx *x = (info_ctx *)c;
    if (cmd != DEPZ_TMF8829_RPT_INFO) return false;
    x->ok = depz_tmf8829_unpack_info(p, len, x->out) == 0;
    return true;
}

int depz_tmf8829_bridge_info(depz_device *dev, depz_tmf8829_info *out)
{
    info_ctx c;
    int rc;
    TRY(need(dev));
    if (!out) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL info");
    c.out = out;
    c.ok = false;
    rc = depz_device_request(dev, DEPZ_TMF8829_CMD_GET_INFO, NULL, 0, match_info, &c, false, -1);
    if (rc == DEPZ_OK && !c.ok) return depz_fail(DEPZ_E_PROTOCOL, "tmf8829: info report did not decode");
    return rc;
}

/* ── streaming ─────────────────────────────────────────────────────────── */

static int do_start(depz_device *dev)
{
    tmf_state *s = st(dev);
    uint8_t mask, p[2];
    int rc;
    if (is_streaming(dev)) return DEPZ_OK;
    if (!s->configured) TRY(do_configure(dev, "8x8", NULL, NULL));
    mask = (uint8_t)(DEPZ_TMF8829_INT_RESULTS | (s->config.histograms ? DEPZ_TMF8829_INT_HISTOGRAMS : 0));
    TRY(wr1(dev, DEPZ_TMF8829_REG_FIFOSTATUS, FIFO_ERROR_FLAGS)); /* sticky under/overrun */
    TRY(wr1(dev, DEPZ_TMF8829_REG_INT_STATUS, 0xFF));
    TRY(wr1(dev, DEPZ_TMF8829_REG_INT_ENAB, mask));
    depz_mutex_lock(&s->lock);
    depz_tmf8829_reassembler_reset(s->reasm);
    depz_tmf8829_builder_reset(s->builder);
    s->chunk_errors = s->measurements = s->events_swallowed = 0;
    depz_mutex_unlock(&s->lock);
    depz_tmf8829_pack_start_stream(mask, s->config.ref_frame ? DEPZ_TMF8829_SF_REF_FRAME : 0, p);
    TRY(depz_device_request(dev, DEPZ_TMF8829_CMD_START_STREAM, p, 2, NULL, NULL, true, -1));
    set_flag(dev, &s->streaming, true);
    rc = app_cmd(dev, DEPZ_TMF8829_CMD_MEASURE, true);
    if (rc) {
        set_flag(dev, &s->streaming, false);
        depz_device_request(dev, DEPZ_TMF8829_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1);
        return rc;
    }
    return DEPZ_OK;
}

int depz_tmf8829_start(depz_device *dev) { LOCKED(do_start(dev)); }

static int do_stop_steps(depz_device *dev)
{
    TRY(app_cmd(dev, DEPZ_TMF8829_CMD_STOP, false));
    TRY(wr1(dev, DEPZ_TMF8829_REG_INT_ENAB, 0));
    TRY(wr1(dev, DEPZ_TMF8829_REG_INT_STATUS, 0xFF));
    if (is_streaming(dev))
        TRY(depz_device_request(dev, DEPZ_TMF8829_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1));
    return DEPZ_OK;
}

static int do_stop(depz_device *dev)
{
    tmf_state *s = st(dev);
    int rc;
    set_flag(dev, &s->stopping, true);
    rc = do_stop_steps(dev);
    depz_mutex_lock(&s->lock);
    s->streaming = false;
    s->stopping = false;
    depz_mutex_unlock(&s->lock);
    return rc;
}

int depz_tmf8829_stop(depz_device *dev) { LOCKED(do_stop(dev)); }

bool depz_tmf8829_streaming(const depz_device *dev)
{
    return depz_is_tmf8829(dev) && is_streaming((depz_device *)dev);
}

int depz_tmf8829_on_measurement(depz_device *dev, depz_tmf8829_measurement_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL callback");
    return depz_cb_list_add(&st(dev)->meas_cbs, (void (*)(void))cb, user, token);
}

void depz_tmf8829_off_measurement(depz_device *dev, int token)
{
    if (depz_is_tmf8829(dev)) depz_cb_list_remove(&st(dev)->meas_cbs, token);
}

int depz_tmf8829_on_frame(depz_device *dev, depz_tmf8829_frame_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL callback");
    return depz_cb_list_add(&st(dev)->frame_cbs, (void (*)(void))cb, user, token);
}

void depz_tmf8829_off_frame(depz_device *dev, int token)
{
    if (depz_is_tmf8829(dev)) depz_cb_list_remove(&st(dev)->frame_cbs, token);
}

int depz_tmf8829_on_event(depz_device *dev, depz_tmf8829_event_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL callback");
    return depz_cb_list_add(&st(dev)->event_cbs, (void (*)(void))cb, user, token);
}

void depz_tmf8829_off_event(depz_device *dev, int token)
{
    if (depz_is_tmf8829(dev)) depz_cb_list_remove(&st(dev)->event_cbs, token);
}

depz_stream *depz_tmf8829_measurements(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 16);
}

int depz_tmf8829_get_measurement(depz_device *dev, int timeout_ms, depz_tmf8829_measurement *out)
{
    depz_stream *s;
    int rc;
    TRY(need(dev));
    if (!out) return depz_fail(DEPZ_E_ARG, "tmf8829: NULL measurement");
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "tmf8829: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 1000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

void depz_tmf8829_get_stream_stats(const depz_device *dev, depz_tmf8829_stream_stats *out)
{
    tmf_state *s;
    memset(out, 0, sizeof *out);
    if (!depz_is_tmf8829(dev)) return;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    out->frames_completed = s->reasm->completed;
    out->frames_dropped = s->reasm->seq_gaps;
    out->frames_discarded = s->reasm->discarded;
    out->frame_parse_errors = s->builder->parse_errors;
    out->chunk_errors = s->chunk_errors;
    out->measurements = s->measurements;
    out->events_swallowed = s->events_swallowed;
    depz_mutex_unlock(&s->lock);
}
