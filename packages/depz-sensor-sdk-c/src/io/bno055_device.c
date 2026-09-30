/* bno055_device.c — the BNO055 class (contract 13): register access over the
 * bridge, mode / units / remap / calibration logic and sample decoding, as
 * the Python SDK's bno055/__init__.py does it — same request sequences, so a
 * capture made by either SDK replays in the other byte for byte. */
#include "io_internal.h"

#include <stdlib.h>
#include <string.h>

#define MODE_SWITCH_FROM_CONFIG_MS 10
#define MODE_SWITCH_TO_CONFIG_MS   25
#define SELF_TEST_MS               450
#define BOOT_SETTLE_TIMEOUT_US     1000000u
#define FUSION_START_TIMEOUT_US    1000000u
#define RESET_TIMEOUT_MS           3000
/* SYS_STATUS 4 ("executing self-test") polls in a row taken as the leftover
 * of a self-test run in CONFIG mode rather than POST: POST shows 4 for ~35 ms
 * (a handful of polls), the leftover stays until the mode leaves CONFIG.
 * Counted, not timed, so a replay makes the same decision. */
#define STUCK_SELF_TEST_POLLS      20

#define SYS_TRIGGER_SELF_TEST 0x01u
#define SYS_TRIGGER_RST_INT   0x40u
#define SYS_TRIGGER_CLK_SEL   0x80u

typedef struct {
    depz_mutex seq;               /* serialises multi-step register sequences */
    depz_mutex lock;              /* the fields below read by the reader */
    bool units_known;
    depz_bno055_units units;
    bool streaming;
    depz_bno055_units stream_units;
    uint64_t parse_errors;
    /* last configure() */
    bool configured;
    uint8_t cfg_mode;
    depz_bno055_units cfg_units;
    bool cfg_has_remap, cfg_has_calib;
    depz_bno055_axis_remap cfg_remap;
    depz_bno055_calib_profile cfg_calib;
    depz_cb_list cbs;
    depz_hub *hub;
} bno_state;

typedef void (*bno_fn)(const depz_bno055_sample *, void *);

static bno_state *st(depz_device *dev) { return (bno_state *)dev->sensor; }

#define TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

/* ── decode ────────────────────────────────────────────────────────────── */

static bool covers(uint8_t addr, size_t len, unsigned reg, unsigned n)
{
    return addr <= reg && reg + n <= addr + len;
}

void depz_bno055_decode_sample(uint64_t timestamp_us, uint8_t addr, const uint8_t *data, size_t len,
                               const depz_bno055_units *units, depz_bno055_sample *out)
{
    depz_bno055_block b;
    double acc = depz_bno055_accel_lsb(units), gyr = depz_bno055_gyro_lsb(units);
    double eul = depz_bno055_euler_lsb(units), tmp = depz_bno055_temp_lsb(units);
    int k;
    memset(out, 0, sizeof *out);
    out->timestamp_us = timestamp_us;
    out->addr = addr;
    out->len = (uint8_t)(len > sizeof out->raw ? sizeof out->raw : len);
    memcpy(out->raw, data, out->len);
    out->units = *units;
    depz_bno055_decode_block(addr, data, len, &b);
    /* A channel is there when the window covers it whole (regs.decode_block). */
    if (covers(addr, len, DEPZ_BNO055_REG_ACC_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_ACCEL;
        for (k = 0; k < 3; k++) out->accel[k] = b.accel[k] / acc;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_MAG_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_MAG;
        for (k = 0; k < 3; k++) out->mag[k] = b.mag[k] / DEPZ_BNO055_MAG_LSB;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_GYR_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_GYRO;
        for (k = 0; k < 3; k++) out->gyro[k] = b.gyro[k] / gyr;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_EUL_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_EULER;
        for (k = 0; k < 3; k++) out->euler[k] = b.euler[k] / eul;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_QUA_DATA, 8)) {
        out->present |= DEPZ_BNO055_HAS_QUATERNION;
        for (k = 0; k < 4; k++) out->quaternion[k] = b.quaternion[k] / DEPZ_BNO055_QUAT_LSB;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_LIA_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_LINEAR_ACCEL;
        for (k = 0; k < 3; k++) out->linear_accel[k] = b.linear_accel[k] / DEPZ_BNO055_FUSION_ACCEL_LSB;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_GRV_DATA, 6)) {
        out->present |= DEPZ_BNO055_HAS_GRAVITY;
        for (k = 0; k < 3; k++) out->gravity[k] = b.gravity[k] / DEPZ_BNO055_FUSION_ACCEL_LSB;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_TEMP, 1)) {
        out->present |= DEPZ_BNO055_HAS_TEMPERATURE;
        out->temperature = b.temperature / tmp;
    }
    if (covers(addr, len, DEPZ_BNO055_REG_CALIB_STAT, 1)) {
        out->present |= DEPZ_BNO055_HAS_CALIBRATION;
        depz_bno055_unpack_calib_status(b.calib_stat, &out->calibration);
    }
}

/* ── class plumbing ────────────────────────────────────────────────────── */

static bool bno_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    bno_state *s = st(dev);
    depz_bno055_stream sd;
    depz_bno055_sample sample;
    depz_bno055_units units;
    depz_cb_entry *cbs;
    size_t i, n;
    if (cmd != DEPZ_BNO055_RPT_STREAM) return false;
    /* The block must be exactly as long as the echoed length. */
    if (len < 10 || depz_bno055_unpack_stream(p, len, &sd) != 0 || len - 10 != sd.len) {
        depz_mutex_lock(&s->lock);
        s->parse_errors++;
        depz_mutex_unlock(&s->lock);
        return true;
    }
    depz_mutex_lock(&s->lock);
    units = s->stream_units;
    depz_mutex_unlock(&s->lock);
    depz_bno055_decode_sample(sd.timestamp_us, sd.addr, sd.data, sd.len, &units, &sample);
    n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) ((bno_fn)cbs[i].fn)(&sample, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, &sample);
    return true;
}

static void bno_closed(depz_device *dev) { depz_hub_mark_closed(st(dev)->hub); }

static void bno_destroy(depz_device *dev)
{
    bno_state *s = st(dev);
    if (!s) return;
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_mutex_destroy(&s->lock);
    depz_mutex_destroy(&s->seq);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops bno_ops = {DEPZ_SENSOR_BNO055, bno_report, bno_closed, bno_destroy};

static bno_state *bno_new(void)
{
    bno_state *s = (bno_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->lock)) { free(s); return NULL; }
    if (depz_mutex_init(&s->seq)) { depz_mutex_destroy(&s->lock); free(s); return NULL; }
    if (depz_cb_list_init(&s->cbs)) goto fail;
    s->hub = depz_hub_new(sizeof(depz_bno055_sample));
    if (!s->hub) { depz_cb_list_free(&s->cbs); goto fail; }
    return s;
fail:
    depz_mutex_destroy(&s->seq);
    depz_mutex_destroy(&s->lock);
    free(s);
    return NULL;
}

int depz_bno055_attach(depz_device *dev)
{
    bno_state *s;
    if (dev->ops == &bno_ops) return DEPZ_OK;
    s = bno_new();
    if (!s) return depz_fail(DEPZ_E_NOMEM, "bno055: out of memory");
    depz_device_attach(dev, &bno_ops, s);
    return DEPZ_OK;
}

int depz_bno055_open_link(depz_link *link, depz_device **out)
{
    bno_state *s = bno_new();
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "bno055: out of memory");
    }
    return depz_device_create(link, &bno_ops, s, out);
}

bool depz_is_bno055(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &bno_ops;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

static int need(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "bno055: NULL device");
    if (!depz_is_bno055(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a BNO055", dev->port);
    return DEPZ_OK;
}

static bool is_streaming(depz_device *dev)
{
    bno_state *s = st(dev);
    bool v;
    depz_mutex_lock(&s->lock);
    v = s->streaming;
    depz_mutex_unlock(&s->lock);
    return v;
}

static int need_not_streaming(depz_device *dev)
{
    if (is_streaming(dev)) return depz_fail(DEPZ_E_ARG, "stop the stream first — the stream reads page 0");
    return DEPZ_OK;
}

/* ── register access (callers hold `seq`) ─────────────────────────────── */

typedef struct { uint8_t *out; size_t want, got; uint64_t ts; } rd_ctx;

static bool match_reg(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    rd_ctx *r = (rd_ctx *)c;
    depz_bno055_reg_data rep;
    if (cmd != DEPZ_BNO055_RPT_REG_DATA || depz_bno055_unpack_reg_data(p, len, &rep) != 0) return false;
    r->got = rep.data_len;
    memcpy(r->out, rep.data, rep.data_len < r->want ? rep.data_len : r->want);
    r->ts = rep.timestamp_us;
    return true;
}

/* READ_REG in <= 128-byte pieces; *ts (optional) = MCU time of the last. */
static int rd(depz_device *dev, uint8_t addr, uint8_t *out, size_t len, uint64_t *ts)
{
    unsigned a = addr;
    while (len) {
        size_t n = len < DEPZ_BNO055_XFER_MAX ? len : DEPZ_BNO055_XFER_MAX;
        uint8_t p[2];
        rd_ctx c;
        c.out = out;
        c.want = n;
        c.got = 0;
        c.ts = 0;
        depz_bno055_pack_read_reg((uint8_t)a, (uint8_t)n, p);
        TRY(depz_device_request(dev, DEPZ_BNO055_CMD_READ_REG, p, 2, match_reg, &c, false, -1));
        if (c.got != n) return depz_fail(DEPZ_E_PROTOCOL, "READ_REG 0x%02X: expected %zu, got %zu", a, n, c.got);
        if (ts) *ts = c.ts;
        out += n;
        a += (unsigned)n;
        len -= n;
    }
    return DEPZ_OK;
}

static int rd1(depz_device *dev, uint8_t addr, uint8_t *v) { return rd(dev, addr, v, 1, NULL); }

static int wr(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len)
{
    unsigned a = addr;
    while (len) {
        size_t n = len < DEPZ_BNO055_XFER_MAX ? len : DEPZ_BNO055_XFER_MAX;
        uint8_t p[2 + DEPZ_BNO055_XFER_MAX];
        size_t plen = depz_bno055_pack_write_reg((uint8_t)a, data, n, p);
        TRY(depz_device_request(dev, DEPZ_BNO055_CMD_WRITE_REG, p, plen, NULL, NULL, true, -1));
        data += n;
        a += (unsigned)n;
        len -= n;
    }
    return DEPZ_OK;
}

static int wr1(depz_device *dev, uint8_t addr, uint8_t v) { return wr(dev, addr, &v, 1); }

static bool is_fusion(uint8_t mode) { return mode >= DEPZ_BNO055_MODE_IMU; }

/* A running fusion never outputs the all-zero quaternion. */
static int wait_fusion_started(depz_device *dev, bool strict)
{
    uint64_t deadline = depz_now_us() + FUSION_START_TIMEOUT_US;
    for (;;) {
        uint8_t q[8];
        int k, any = 0;
        TRY(rd(dev, DEPZ_BNO055_REG_QUA_DATA, q, 8, NULL));
        for (k = 0; k < 8; k++) any |= q[k];
        if (any) return DEPZ_OK;
        if (depz_now_us() > deadline) {
            if (strict) return depz_fail(DEPZ_E_TIMEOUT, "BNO055 fusion did not start (quaternion stays zero)");
            return DEPZ_OK; /* a suspended sensor never updates its registers */
        }
        depz_device_sleep_ms(dev, 10);
    }
}

/* One OPR_MODE write plus its settle time. */
static int switch_mode(depz_device *dev, uint8_t mode)
{
    TRY(wr1(dev, DEPZ_BNO055_REG_OPR_MODE, mode));
    depz_device_sleep_ms(dev, mode == DEPZ_BNO055_MODE_CONFIG ? MODE_SWITCH_TO_CONFIG_MS : MODE_SWITCH_FROM_CONFIG_MS);
    if (is_fusion(mode)) return wait_fusion_started(dev, false);
    return DEPZ_OK;
}

/* Enter CONFIG, remembering the mode to go back to. */
static int config_enter(depz_device *dev, uint8_t *previous)
{
    uint8_t v;
    TRY(rd1(dev, DEPZ_BNO055_REG_OPR_MODE, &v));
    *previous = v & 0x0F;
    if (*previous != DEPZ_BNO055_MODE_CONFIG) TRY(switch_mode(dev, DEPZ_BNO055_MODE_CONFIG));
    return DEPZ_OK;
}

static int config_leave(depz_device *dev, uint8_t previous, int rc)
{
    if (previous != DEPZ_BNO055_MODE_CONFIG) {
        int rc2 = switch_mode(dev, previous);
        if (rc == DEPZ_OK) rc = rc2;
    }
    return rc;
}

/* Page 1 around one access; always back to page 0. */
static int page_enter(depz_device *dev, int page)
{
    if (page == 0) return DEPZ_OK;
    if (page != 1) return depz_fail(DEPZ_E_ARG, "BNO055 has register pages 0 and 1, not %d", page);
    TRY(need_not_streaming(dev));
    return wr1(dev, DEPZ_BNO055_REG_PAGE_ID, 0x01);
}

static int page_leave(depz_device *dev, int page, int rc)
{
    if (page == 1) {
        int rc2 = wr1(dev, DEPZ_BNO055_REG_PAGE_ID, 0x00);
        if (rc == DEPZ_OK) rc = rc2;
    }
    return rc;
}

/* A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode leaves
 * CONFIG (measured on SW 03.11: rewriting CONFIG does not clear it); step
 * into ACCONLY and back. */
static int clear_self_test_status(depz_device *dev)
{
    TRY(switch_mode(dev, DEPZ_BNO055_MODE_ACCONLY));
    return switch_mode(dev, DEPZ_BNO055_MODE_CONFIG);
}

static int wait_booted(depz_device *dev)
{
    uint64_t deadline = depz_now_us() + BOOT_SETTLE_TIMEOUT_US;
    unsigned in_self_test = 0;
    bool cleared = false;
    for (;;) {
        uint8_t v;
        TRY(rd1(dev, DEPZ_BNO055_REG_SYS_STATUS, &v));
        if (v != 2 && v != 3 && v != 4) return DEPZ_OK;
        in_self_test = v == 4 ? in_self_test + 1 : 0;
        if (in_self_test >= STUCK_SELF_TEST_POLLS && !cleared) {
            TRY(clear_self_test_status(dev));
            cleared = true;
            in_self_test = 0;
            continue;
        }
        if (depz_now_us() > deadline) return depz_fail(DEPZ_E_TIMEOUT, "BNO055 did not finish booting (SYS_STATUS stuck)");
        depz_device_sleep_ms(dev, 5);
    }
}

static int current_units(depz_device *dev, depz_bno055_units *out)
{
    bno_state *s = st(dev);
    bool known;
    depz_mutex_lock(&s->lock);
    known = s->units_known;
    *out = s->units;
    depz_mutex_unlock(&s->lock);
    if (!known) {
        uint8_t v;
        TRY(rd1(dev, DEPZ_BNO055_REG_UNIT_SEL, &v));
        depz_bno055_unpack_units(v, out);
        depz_mutex_lock(&s->lock);
        s->units = *out;
        s->units_known = true;
        depz_mutex_unlock(&s->lock);
    }
    return DEPZ_OK;
}

static void remember_units(depz_device *dev, const depz_bno055_units *u, bool known)
{
    bno_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    if (u) s->units = *u;
    s->units_known = known;
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

/* A register write in CONFIG mode, the operating mode restored. */
static int config_write(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len)
{
    uint8_t prev;
    TRY(config_enter(dev, &prev));
    return config_leave(dev, prev, wr(dev, addr, data, len));
}

/* ── public: identity, reset, raw access ──────────────────────────────── */

typedef struct { depz_bno055_info *out; bool ok; } info_ctx;

static bool match_info(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    info_ctx *x = (info_ctx *)c;
    if (cmd != DEPZ_BNO055_RPT_INFO) return false;
    x->ok = depz_bno055_unpack_info(p, len, x->out) == 0;
    return true;
}

int depz_bno055_bridge_info(depz_device *dev, depz_bno055_info *out)
{
    info_ctx c;
    TRY(need(dev));
    if (!out) return depz_fail(DEPZ_E_ARG, "bridge_info: NULL output");
    c.out = out;
    c.ok = false;
    TRY(depz_device_request(dev, DEPZ_BNO055_CMD_GET_INFO, NULL, 0, match_info, &c, false, -1));
    if (!c.ok) return depz_fail(DEPZ_E_PROTOCOL, "bno055: info report did not decode");
    return DEPZ_OK;
}

int depz_bno055_is_alive(depz_device *dev, bool *alive)
{
    depz_bno055_info i;
    int rc = depz_bno055_bridge_info(dev, &i);
    if (alive) *alive = rc == DEPZ_OK && i.initialized && i.chip_id == 0xA0 && i.acc_id == 0xFB &&
                        i.mag_id == 0x32 && i.gyr_id == 0x0F;
    return rc == DEPZ_E_WRONG_TYPE || rc == DEPZ_E_ARG ? rc : DEPZ_OK;
}

static int do_reset(depz_device *dev)
{
    bno_state *s = st(dev);
    TRY(depz_device_request(dev, DEPZ_BNO055_CMD_RESET, NULL, 0, NULL, NULL, true, RESET_TIMEOUT_MS));
    depz_mutex_lock(&s->lock);
    s->streaming = false;
    s->units_known = false;
    depz_mutex_unlock(&s->lock);
    return wait_booted(dev);
}

int depz_bno055_reset_sensor(depz_device *dev) { LOCKED(do_reset(dev)); }

static int do_read_regs(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len, int page)
{
    TRY(page_enter(dev, page));
    return page_leave(dev, page, rd(dev, addr, buf, len, NULL));
}

static int do_write_regs(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len, int page)
{
    TRY(page_enter(dev, page));
    return page_leave(dev, page, wr(dev, addr, data, len));
}

int depz_bno055_read_registers(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len, int page)
{
    LOCKED(do_read_regs(dev, addr, buf, len, page));
}

int depz_bno055_write_registers(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len, int page)
{
    LOCKED(do_write_regs(dev, addr, data, len, page));
}

/* ── public: modes, units, axes ───────────────────────────────────────── */

static int get_masked(depz_device *dev, uint8_t reg, uint8_t mask, uint8_t *out)
{
    uint8_t v;
    TRY(rd1(dev, reg, &v));
    *out = v & mask;
    return DEPZ_OK;
}

int depz_bno055_get_operation_mode(depz_device *dev, uint8_t *mode) { LOCKED(get_masked(dev, DEPZ_BNO055_REG_OPR_MODE, 0x0F, mode)); }

static int do_set_mode(depz_device *dev, uint8_t mode)
{
    if (mode > DEPZ_BNO055_MODE_NDOF) return depz_fail(DEPZ_E_ARG, "bno055: no operating mode 0x%02X", mode);
    if (mode != DEPZ_BNO055_MODE_CONFIG) {
        uint8_t cur;
        TRY(get_masked(dev, DEPZ_BNO055_REG_OPR_MODE, 0x0F, &cur));
        if (cur == mode) return DEPZ_OK;
        /* mode -> mode writes are ignored by the sensor: go through CONFIG */
        if (cur != DEPZ_BNO055_MODE_CONFIG) TRY(switch_mode(dev, DEPZ_BNO055_MODE_CONFIG));
    }
    return switch_mode(dev, mode);
}

int depz_bno055_set_operation_mode(depz_device *dev, uint8_t mode) { LOCKED(do_set_mode(dev, mode)); }
int depz_bno055_get_power_mode(depz_device *dev, uint8_t *mode) { LOCKED(get_masked(dev, DEPZ_BNO055_REG_PWR_MODE, 0x03, mode)); }
int depz_bno055_set_power_mode(depz_device *dev, uint8_t mode) { LOCKED(config_write(dev, DEPZ_BNO055_REG_PWR_MODE, &mode, 1)); }

static int do_get_units(depz_device *dev, depz_bno055_units *out)
{
    uint8_t v;
    TRY(rd1(dev, DEPZ_BNO055_REG_UNIT_SEL, &v));
    depz_bno055_unpack_units(v, out);
    remember_units(dev, out, true);
    return DEPZ_OK;
}

int depz_bno055_get_units(depz_device *dev, depz_bno055_units *out) { LOCKED(do_get_units(dev, out)); }

static int do_set_units(depz_device *dev, const depz_bno055_units *u)
{
    uint8_t prev, v = depz_bno055_pack_units(u);
    int rc;
    TRY(config_enter(dev, &prev));
    rc = wr(dev, DEPZ_BNO055_REG_UNIT_SEL, &v, 1);
    if (rc == DEPZ_OK) remember_units(dev, u, true);
    return config_leave(dev, prev, rc);
}

int depz_bno055_set_units(depz_device *dev, const depz_bno055_units *units) { LOCKED(do_set_units(dev, units)); }

static int do_get_remap(depz_device *dev, depz_bno055_axis_remap *out)
{
    uint8_t b[2];
    TRY(rd(dev, DEPZ_BNO055_REG_AXIS_MAP_CONFIG, b, 2, NULL));
    depz_bno055_unpack_axis_remap(b[0], b[1], out);
    return DEPZ_OK;
}

int depz_bno055_get_axis_remap(depz_device *dev, depz_bno055_axis_remap *out) { LOCKED(do_get_remap(dev, out)); }

static int do_set_remap(depz_device *dev, const depz_bno055_axis_remap *r)
{
    uint8_t b[2];
    if (depz_bno055_pack_axis_remap(r, &b[0], &b[1]) != 0) return depz_fail(DEPZ_E_ARG, "bno055: axes must be a permutation of x, y, z");
    return config_write(dev, DEPZ_BNO055_REG_AXIS_MAP_CONFIG, b, 2);
}

int depz_bno055_set_axis_remap(depz_device *dev, const depz_bno055_axis_remap *remap) { LOCKED(do_set_remap(dev, remap)); }

int depz_bno055_set_axis_placement(depz_device *dev, const char *placement)
{
    depz_bno055_axis_remap r;
    TRY(need(dev));
    if (!placement || depz_bno055_placement(placement, &r) != 0)
        return depz_fail(DEPZ_E_ARG, "bno055: placement is \"P0\"..\"P7\"");
    return depz_bno055_set_axis_remap(dev, &r);
}

int depz_bno055_get_temperature_source(depz_device *dev, uint8_t *source) { LOCKED(get_masked(dev, DEPZ_BNO055_REG_TEMP_SOURCE, 0x03, source)); }
int depz_bno055_set_temperature_source(depz_device *dev, uint8_t source) { LOCKED(config_write(dev, DEPZ_BNO055_REG_TEMP_SOURCE, &source, 1)); }

static int do_configure(depz_device *dev, uint8_t mode, const depz_bno055_units *units,
                        const depz_bno055_axis_remap *remap, const depz_bno055_calib_profile *calib)
{
    bno_state *s = st(dev);
    depz_bno055_units u;
    uint8_t v;
    if (mode > DEPZ_BNO055_MODE_NDOF) return depz_fail(DEPZ_E_ARG, "bno055: no operating mode 0x%02X", mode);
    if (units) u = *units;
    else depz_bno055_unpack_units(0x00, &u); /* Units() defaults: SI, Windows orientation */
    TRY(wait_booted(dev));
    TRY(switch_mode(dev, DEPZ_BNO055_MODE_CONFIG));
    v = depz_bno055_pack_units(&u);
    TRY(wr(dev, DEPZ_BNO055_REG_UNIT_SEL, &v, 1));
    remember_units(dev, &u, true);
    if (remap) {
        uint8_t b[2];
        if (depz_bno055_pack_axis_remap(remap, &b[0], &b[1]) != 0) return depz_fail(DEPZ_E_ARG, "bno055: bad axis remap");
        TRY(wr(dev, DEPZ_BNO055_REG_AXIS_MAP_CONFIG, b, 2));
    }
    if (calib) {
        uint8_t b[DEPZ_BNO055_CALIB_PROFILE_LEN];
        depz_bno055_pack_calib_profile(calib, b);
        TRY(wr(dev, DEPZ_BNO055_REG_CALIB_PROFILE, b, sizeof b));
    }
    TRY(switch_mode(dev, mode));
    if (is_fusion(mode)) TRY(wait_fusion_started(dev, true));
    s->configured = true;
    s->cfg_mode = mode;
    s->cfg_units = u;
    s->cfg_has_remap = remap != NULL;
    if (remap) s->cfg_remap = *remap;
    s->cfg_has_calib = calib != NULL;
    if (calib) s->cfg_calib = *calib;
    return DEPZ_OK;
}

int depz_bno055_configure(depz_device *dev, uint8_t mode, const depz_bno055_units *units,
                          const depz_bno055_axis_remap *remap, const depz_bno055_calib_profile *calibration)
{
    LOCKED(do_configure(dev, mode, units, remap, calibration));
}

static int do_restore(depz_device *dev)
{
    bno_state *s = st(dev);
    depz_bno055_units u;
    depz_bno055_axis_remap r;
    depz_bno055_calib_profile c;
    if (!s->configured) return depz_fail(DEPZ_E_ARG, "bno055: configure() has not been called");
    u = s->cfg_units;
    r = s->cfg_remap;
    c = s->cfg_calib;
    return do_configure(dev, s->cfg_mode, &u, s->cfg_has_remap ? &r : NULL, s->cfg_has_calib ? &c : NULL);
}

int depz_bno055_restore_configuration(depz_device *dev) { LOCKED(do_restore(dev)); }

/* ── public: status, self-test, calibration ───────────────────────────── */

static int do_status(depz_device *dev, depz_bno055_status_regs *out)
{
    uint8_t b[3];
    TRY(rd1(dev, DEPZ_BNO055_REG_ST_RESULT, &out->self_test));
    TRY(rd(dev, DEPZ_BNO055_REG_SYS_CLK_STATUS, b, 3, NULL)); /* INT_STA skipped: clears on read */
    out->clk_status = b[0];
    out->status = b[1];
    out->error = b[2];
    return DEPZ_OK;
}

int depz_bno055_system_status(depz_device *dev, depz_bno055_status_regs *out) { LOCKED(do_status(dev, out)); }

static int do_self_test(depz_device *dev, depz_bno055_status_regs *out)
{
    uint8_t prev, trig;
    int rc;
    TRY(need_not_streaming(dev));
    TRY(config_enter(dev, &prev));
    rc = rd1(dev, DEPZ_BNO055_REG_SYS_TRIGGER, &trig);
    if (rc == DEPZ_OK) rc = wr1(dev, DEPZ_BNO055_REG_SYS_TRIGGER, (uint8_t)((trig & SYS_TRIGGER_CLK_SEL) | SYS_TRIGGER_SELF_TEST));
    if (rc == DEPZ_OK) {
        depz_device_sleep_ms(dev, SELF_TEST_MS);
        rc = do_status(dev, out);
    }
    if (rc == DEPZ_OK && prev == DEPZ_BNO055_MODE_CONFIG) rc = clear_self_test_status(dev);
    return config_leave(dev, prev, rc);
}

int depz_bno055_self_test(depz_device *dev, depz_bno055_status_regs *out) { LOCKED(do_self_test(dev, out)); }

static int do_calib_status(depz_device *dev, depz_bno055_calib_status *out)
{
    uint8_t v;
    TRY(rd1(dev, DEPZ_BNO055_REG_CALIB_STAT, &v));
    depz_bno055_unpack_calib_status(v, out);
    return DEPZ_OK;
}

int depz_bno055_calibration_status(depz_device *dev, depz_bno055_calib_status *out) { LOCKED(do_calib_status(dev, out)); }

static int do_read_profile(depz_device *dev, depz_bno055_calib_profile *out)
{
    uint8_t prev, b[DEPZ_BNO055_CALIB_PROFILE_LEN];
    int rc;
    TRY(config_enter(dev, &prev));
    rc = rd(dev, DEPZ_BNO055_REG_CALIB_PROFILE, b, sizeof b, NULL);
    if (rc == DEPZ_OK) depz_bno055_unpack_calib_profile(b, sizeof b, out);
    return config_leave(dev, prev, rc);
}

int depz_bno055_read_calibration_profile(depz_device *dev, depz_bno055_calib_profile *out) { LOCKED(do_read_profile(dev, out)); }

static int do_write_profile(depz_device *dev, const depz_bno055_calib_profile *p)
{
    uint8_t b[DEPZ_BNO055_CALIB_PROFILE_LEN];
    depz_bno055_pack_calib_profile(p, b);
    return config_write(dev, DEPZ_BNO055_REG_CALIB_PROFILE, b, sizeof b);
}

int depz_bno055_write_calibration_profile(depz_device *dev, const depz_bno055_calib_profile *p) { LOCKED(do_write_profile(dev, p)); }

static int do_get_sic(depz_device *dev, int16_t out[9])
{
    uint8_t b[18];
    int k;
    TRY(rd(dev, DEPZ_BNO055_REG_SIC_MATRIX, b, 18, NULL));
    for (k = 0; k < 9; k++) out[k] = (int16_t)(b[2 * k] | b[2 * k + 1] << 8);
    return DEPZ_OK;
}

int depz_bno055_get_sic_matrix(depz_device *dev, int16_t out[9]) { LOCKED(do_get_sic(dev, out)); }

static int do_set_sic(depz_device *dev, const int16_t m[9])
{
    uint8_t b[18];
    int k;
    for (k = 0; k < 9; k++) {
        b[2 * k] = (uint8_t)m[k];
        b[2 * k + 1] = (uint8_t)((uint16_t)m[k] >> 8);
    }
    return config_write(dev, DEPZ_BNO055_REG_SIC_MATRIX, b, 18);
}

int depz_bno055_set_sic_matrix(depz_device *dev, const int16_t m[9]) { LOCKED(do_set_sic(dev, m)); }

/* ── public: page 1 ───────────────────────────────────────────────────── */

/* CONFIG mode, page 1, write, page 0, the operating mode back. */
static int page1_config_write(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len)
{
    uint8_t prev;
    int rc;
    TRY(need_not_streaming(dev));
    TRY(config_enter(dev, &prev));
    rc = page_enter(dev, 1);
    if (rc == DEPZ_OK) rc = page_leave(dev, 1, wr(dev, addr, data, len));
    return config_leave(dev, prev, rc);
}

static int do_get_acc(depz_device *dev, depz_bno055_accel_config *out)
{
    uint8_t v;
    TRY(do_read_regs(dev, DEPZ_BNO055_REG1_ACC_CONFIG, &v, 1, 1));
    depz_bno055_unpack_accel_config(v, out);
    return DEPZ_OK;
}

int depz_bno055_get_accel_config(depz_device *dev, depz_bno055_accel_config *out) { LOCKED(do_get_acc(dev, out)); }

static int do_set_acc(depz_device *dev, const depz_bno055_accel_config *c)
{
    uint8_t v = depz_bno055_pack_accel_config(c);
    return page1_config_write(dev, DEPZ_BNO055_REG1_ACC_CONFIG, &v, 1);
}

int depz_bno055_set_accel_config(depz_device *dev, const depz_bno055_accel_config *c) { LOCKED(do_set_acc(dev, c)); }

static int do_get_gyr(depz_device *dev, depz_bno055_gyro_config *out)
{
    uint8_t b[2];
    TRY(do_read_regs(dev, DEPZ_BNO055_REG1_GYR_CONFIG_0, b, 2, 1));
    depz_bno055_unpack_gyro_config(b, out);
    return DEPZ_OK;
}

int depz_bno055_get_gyro_config(depz_device *dev, depz_bno055_gyro_config *out) { LOCKED(do_get_gyr(dev, out)); }

static int do_set_gyr(depz_device *dev, const depz_bno055_gyro_config *c)
{
    uint8_t b[2];
    depz_bno055_pack_gyro_config(c, b);
    return page1_config_write(dev, DEPZ_BNO055_REG1_GYR_CONFIG_0, b, 2);
}

int depz_bno055_set_gyro_config(depz_device *dev, const depz_bno055_gyro_config *c) { LOCKED(do_set_gyr(dev, c)); }

static int do_get_mag(depz_device *dev, depz_bno055_mag_config *out)
{
    uint8_t v;
    TRY(do_read_regs(dev, DEPZ_BNO055_REG1_MAG_CONFIG, &v, 1, 1));
    depz_bno055_unpack_mag_config(v, out);
    return DEPZ_OK;
}

int depz_bno055_get_mag_config(depz_device *dev, depz_bno055_mag_config *out) { LOCKED(do_get_mag(dev, out)); }

static int do_set_mag(depz_device *dev, const depz_bno055_mag_config *c)
{
    uint8_t v = depz_bno055_pack_mag_config(c);
    return page1_config_write(dev, DEPZ_BNO055_REG1_MAG_CONFIG, &v, 1);
}

int depz_bno055_set_mag_config(depz_device *dev, const depz_bno055_mag_config *c) { LOCKED(do_set_mag(dev, c)); }

int depz_bno055_unique_id(depz_device *dev, uint8_t out[16]) { LOCKED(do_read_regs(dev, DEPZ_BNO055_REG1_UNIQUE_ID, out, 16, 1)); }
int depz_bno055_get_interrupt_enable(depz_device *dev, uint8_t *mask) { LOCKED(do_read_regs(dev, DEPZ_BNO055_REG1_INT_EN, mask, 1, 1)); }
int depz_bno055_set_interrupt_enable(depz_device *dev, uint8_t mask) { LOCKED(do_write_regs(dev, DEPZ_BNO055_REG1_INT_EN, &mask, 1, 1)); }
int depz_bno055_get_interrupt_mask(depz_device *dev, uint8_t *mask) { LOCKED(do_read_regs(dev, DEPZ_BNO055_REG1_INT_MSK, mask, 1, 1)); }
int depz_bno055_set_interrupt_mask(depz_device *dev, uint8_t mask) { LOCKED(do_write_regs(dev, DEPZ_BNO055_REG1_INT_MSK, &mask, 1, 1)); }

int depz_bno055_set_interrupt_setting(depz_device *dev, uint8_t reg, uint8_t value)
{
    TRY(need(dev));
    if (reg < 0x11 || reg > 0x1F)
        return depz_fail(DEPZ_E_ARG, "0x%02X is not a page-1 interrupt setting (0x11..0x1F)", reg);
    LOCKED(page1_config_write(dev, reg, &value, 1));
}

int depz_bno055_read_interrupt_status(depz_device *dev, uint8_t *status) { LOCKED(rd1(dev, DEPZ_BNO055_REG_INT_STA, status)); }

static int do_clear_int(depz_device *dev)
{
    uint8_t v;
    TRY(rd1(dev, DEPZ_BNO055_REG_SYS_TRIGGER, &v));
    return wr1(dev, DEPZ_BNO055_REG_SYS_TRIGGER, (uint8_t)((v & SYS_TRIGGER_CLK_SEL) | SYS_TRIGGER_RST_INT));
}

int depz_bno055_clear_interrupt(depz_device *dev) { LOCKED(do_clear_int(dev)); }

/* ── public: data ─────────────────────────────────────────────────────── */

static int do_read_sample(depz_device *dev, uint8_t addr, uint8_t len, depz_bno055_sample *out)
{
    depz_bno055_units u;
    uint8_t buf[256];
    uint64_t ts = 0;
    if (!len || addr + len > 0x100) return depz_fail(DEPZ_E_ARG, "bno055: block 0x%02X+%u outside the page", addr, len);
    TRY(current_units(dev, &u));
    TRY(rd(dev, addr, buf, len, &ts));
    depz_bno055_decode_sample(ts, addr, buf, len, &u, out);
    return DEPZ_OK;
}

int depz_bno055_read_sample(depz_device *dev, uint8_t addr, uint8_t len, depz_bno055_sample *out)
{
    LOCKED(do_read_sample(dev, addr, len, out));
}

int depz_bno055_read_quaternion(depz_device *dev, double q[4])
{
    depz_bno055_sample s;
    TRY(depz_bno055_read_sample(dev, DEPZ_BNO055_QUAT_BLOCK_ADDR, DEPZ_BNO055_QUAT_BLOCK_LEN, &s));
    memcpy(q, s.quaternion, sizeof s.quaternion);
    return DEPZ_OK;
}

static int do_start_stream(depz_device *dev, uint16_t period_ms, uint8_t addr, uint8_t len, uint8_t trigger)
{
    bno_state *s = st(dev);
    depz_bno055_units u;
    uint8_t p[5];
    if (len < 1 || len > DEPZ_BNO055_XFER_MAX || addr + len > 0x100)
        return depz_fail(DEPZ_E_ARG, "bno055: block 0x%02X+%u outside 1..128 / page", addr, len);
    /* Units first: the reader may decode the first sample before the
     * request below returns. */
    TRY(current_units(dev, &u));
    depz_mutex_lock(&s->lock);
    s->stream_units = u;
    depz_mutex_unlock(&s->lock);
    depz_bno055_pack_start_stream(trigger, addr, len, period_ms, p);
    TRY(depz_device_request(dev, DEPZ_BNO055_CMD_START_STREAM, p, 5, NULL, NULL, true, -1));
    depz_mutex_lock(&s->lock);
    s->streaming = true;
    depz_mutex_unlock(&s->lock);
    return DEPZ_OK;
}

int depz_bno055_start_stream(depz_device *dev, uint16_t period_ms, uint8_t addr, uint8_t len, uint8_t trigger)
{
    LOCKED(do_start_stream(dev, period_ms, addr, len, trigger));
}

static int do_stop_stream(depz_device *dev)
{
    bno_state *s = st(dev);
    int rc;
    if (!is_streaming(dev)) return DEPZ_OK;
    rc = depz_device_request(dev, DEPZ_BNO055_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1);
    depz_mutex_lock(&s->lock);
    s->streaming = false;
    depz_mutex_unlock(&s->lock);
    return rc;
}

int depz_bno055_stop_stream(depz_device *dev) { LOCKED(do_stop_stream(dev)); }

bool depz_bno055_streaming(const depz_device *dev) { return depz_is_bno055(dev) && is_streaming((depz_device *)dev); }

int depz_bno055_on_sample(depz_device *dev, depz_bno055_sample_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "bno055: NULL callback");
    return depz_cb_list_add(&st(dev)->cbs, (void (*)(void))cb, user, token);
}

void depz_bno055_off_sample(depz_device *dev, int token)
{
    if (depz_is_bno055(dev)) depz_cb_list_remove(&st(dev)->cbs, token);
}

depz_stream *depz_bno055_samples(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 256);
}

int depz_bno055_get_sample(depz_device *dev, int timeout_ms, depz_bno055_sample *out)
{
    depz_stream *s;
    int rc;
    TRY(need(dev));
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "bno055: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 1000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

uint64_t depz_bno055_stream_parse_errors(const depz_device *dev)
{
    uint64_t n;
    bno_state *s;
    if (!depz_is_bno055(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->parse_errors;
    depz_mutex_unlock(&s->lock);
    return n;
}
