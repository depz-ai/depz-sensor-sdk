/* vl53l4cd_device.c — the VL53L4CD sensor class (contract 10): the ST ULD
 * 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) ported register for
 * register over the register bridge, as the Python SDK's vl53l4/uld.py does.
 * Register sequences, integer widths and poll loops match it exactly, so a
 * capture made by either SDK replays in the other byte for byte. */
#include "io_internal.h"

#include <stdlib.h>
#include <string.h>

/* ── registers (VL53L4CD_api.h) ─────────────────────────────────────────── */

#define OSC_FREQUENCY                         0x0006
#define VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND 0x0008
#define XTALK_PLANE_OFFSET_KCPS               0x0016
#define XTALK_X_PLANE_GRADIENT_KCPS           0x0018
#define XTALK_Y_PLANE_GRADIENT_KCPS           0x001A
#define RANGE_OFFSET_MM                       0x001E
#define INNER_OFFSET_MM                       0x0020
#define OUTER_OFFSET_MM                       0x0022
#define GPIO_HV_MUX__CTRL                     0x0030
#define GPIO__TIO_HV_STATUS                   0x0031
#define SYSTEM__INTERRUPT                     0x0046
#define RANGE_CONFIG_A                        0x005E
#define RANGE_CONFIG_B                        0x0061
#define RANGE_CONFIG__SIGMA_THRESH            0x0064
#define MIN_COUNT_RATE_RTN_LIMIT_MCPS         0x0066
#define INTERMEASUREMENT_MS                   0x006C
#define THRESH_HIGH                           0x0072
#define THRESH_LOW                            0x0074
#define SYSTEM__INTERRUPT_CLEAR               0x0086
#define SYSTEM_START                          0x0087
#define RESULT__OSC_CALIBRATE_VAL             0x00DE
#define FIRMWARE__SYSTEM_STATUS               0x00E5
#define IDENTIFICATION__MODEL_ID              0x010F

#define I2C_KHZ_BOOT 400u

/* ── state ─────────────────────────────────────────────────────────────── */

typedef struct {
    depz_regbridge rb;
    depz_mutex     lock;          /* ranging / initialized flags */
    bool           ranging, initialized;
    depz_cb_list   cbs;
    depz_hub      *hub;
    uint64_t       parse_errors;  /* reader thread writes, under `lock` */
} l4_state;

typedef void (*l4_fn)(const depz_vl53l4cd_measurement *, void *);

static l4_state *st(depz_device *dev) { return (l4_state *)dev->sensor; }

static bool l4_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    l4_state *s = st(dev);
    depz_vl53l4_stream sample;
    depz_vl53l4cd_measurement m;
    depz_cb_entry *cbs;
    size_t i, n;
    if (cmd != DEPZ_VL53L4_RPT_STREAM || len < 12) return false;
    if (depz_vl53l4_unpack_stream(p, len, &sample) != 0 ||
        depz_vl53l4_parse_result_block(sample.data, sample.len, &m.r) != 0) {
        /* A stream re-armed on another block: count it, keep the reader alive. */
        depz_mutex_lock(&s->lock);
        s->parse_errors++;
        depz_mutex_unlock(&s->lock);
        return true;
    }
    m.timestamp_us = sample.timestamp_us;
    n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) ((l4_fn)cbs[i].fn)(&m, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, &m);
    return true;
}

static void l4_closed(depz_device *dev) { depz_hub_mark_closed(st(dev)->hub); }

static void l4_destroy(depz_device *dev)
{
    l4_state *s = st(dev);
    if (!s) return;
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_mutex_destroy(&s->lock);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops l4_ops = {DEPZ_SENSOR_VL53L4, l4_report, l4_closed, l4_destroy};

static l4_state *l4_new(void)
{
    l4_state *s = (l4_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->lock)) { free(s); return NULL; }
    if (depz_cb_list_init(&s->cbs)) { depz_mutex_destroy(&s->lock); free(s); return NULL; }
    s->hub = depz_hub_new(sizeof(depz_vl53l4cd_measurement));
    if (!s->hub) {
        depz_cb_list_free(&s->cbs);
        depz_mutex_destroy(&s->lock);
        free(s);
        return NULL;
    }
    s->rb.cmd_read = DEPZ_VL53L4_CMD_READ_REG;
    s->rb.cmd_write = DEPZ_VL53L4_CMD_WRITE_REG;
    s->rb.rpt_reg_data = DEPZ_VL53L4_RPT_REG_DATA;
    s->rb.xfer_max = DEPZ_VL53L4_XFER_MAX;
    s->rb.timeout_ms = 2000;
    return s;
}

int depz_vl53l4cd_attach(depz_device *dev)
{
    l4_state *s;
    if (dev->ops == &l4_ops) return DEPZ_OK;
    s = l4_new();
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53l4cd: out of memory");
    s->rb.dev = dev;
    depz_device_attach(dev, &l4_ops, s);
    return DEPZ_OK;
}

int depz_vl53l4cd_open_link(depz_link *link, depz_device **out)
{
    l4_state *s = l4_new();
    int rc;
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "vl53l4cd: out of memory");
    }
    rc = depz_device_create(link, &l4_ops, s, out);
    if (rc == DEPZ_OK) s->rb.dev = *out;
    return rc;
}

bool depz_is_vl53l4cd(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &l4_ops;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

static int need(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "vl53l4cd: NULL device");
    if (!depz_is_vl53l4cd(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a VL53L4CD", dev->port);
    return DEPZ_OK;
}

static bool get_flag(depz_device *dev, bool ranging)
{
    l4_state *s = st(dev);
    bool v;
    depz_mutex_lock(&s->lock);
    v = ranging ? s->ranging : s->initialized;
    depz_mutex_unlock(&s->lock);
    return v;
}

static void set_flags(depz_device *dev, int ranging, int initialized)
{
    l4_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    if (ranging >= 0) s->ranging = ranging != 0;
    if (initialized >= 0) s->initialized = initialized != 0;
    depz_mutex_unlock(&s->lock);
}

/* Configuration is refused while the stream owns the register bank. */
static int need_idle(depz_device *dev)
{
    int rc = need(dev);
    if (rc) return rc;
    if (get_flag(dev, true))
        return depz_fail(DEPZ_E_ARG, "stop ranging first — the stream owns the register bank");
    return DEPZ_OK;
}

bool depz_vl53l4cd_initialized(const depz_device *dev)
{
    return depz_is_vl53l4cd(dev) && get_flag((depz_device *)dev, false);
}

bool depz_vl53l4cd_ranging(const depz_device *dev)
{
    return depz_is_vl53l4cd(dev) && get_flag((depz_device *)dev, true);
}

const char *depz_vl53l4cd_status_text(int s)
{
    switch (s) {
    case 0: return "valid";
    case 1: return "sigma above threshold";
    case 2: return "signal below threshold";
    case 3: return "distance below detection threshold";
    case 4: return "phase out of valid limit";
    case 5: return "hardware fail";
    case 6: return "no wrap-around check done";
    case 7: return "wrapped target, phase mismatch";
    case 8: return "processing fail";
    case 9: return "crosstalk signal fail";
    case 10: return "interrupt error";
    case 11: return "merged target";
    case 12: return "signal too low";
    case 255: return "other error";
    default: return "unknown";
    }
}

/* ── ULD register helpers (16-bit address, big-endian contents) ─────────── */

#define TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

static int rd_byte(depz_device *dev, uint16_t a, uint8_t *v)
{
    return depz_rb_read(&st(dev)->rb, a, v, 1);
}

static int rd_word(depz_device *dev, uint16_t a, uint16_t *v)
{
    uint8_t b[2];
    TRY(depz_rb_read(&st(dev)->rb, a, b, 2));
    *v = (uint16_t)(b[0] << 8 | b[1]);
    return DEPZ_OK;
}

static int rd_dword(depz_device *dev, uint16_t a, uint32_t *v)
{
    uint8_t b[4];
    TRY(depz_rb_read(&st(dev)->rb, a, b, 4));
    *v = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    return DEPZ_OK;
}

static int wr_byte(depz_device *dev, uint16_t a, uint8_t v)
{
    return depz_rb_write(&st(dev)->rb, a, &v, 1);
}

static int wr_word(depz_device *dev, uint16_t a, uint16_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)(v >> 8);
    b[1] = (uint8_t)v;
    return depz_rb_write(&st(dev)->rb, a, b, 2);
}

static int wr_dword(depz_device *dev, uint16_t a, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
    return depz_rb_write(&st(dev)->rb, a, b, 4);
}

static int set_speed(depz_device *dev, uint16_t khz)
{
    uint8_t b[2];
    depz_vl53l4_pack_set_i2c_speed(khz, b);
    return depz_device_request(dev, DEPZ_VL53L4_CMD_SET_I2C_SPEED, b, 2, NULL, NULL, true, -1);
}

/* ── ULD core ──────────────────────────────────────────────────────────── */

static int wait_boot(depz_device *dev, int timeout_ms)
{
    int i;
    for (i = 0; i < (timeout_ms > 1 ? timeout_ms : 1); i++) {
        uint8_t v;
        TRY(rd_byte(dev, FIRMWARE__SYSTEM_STATUS, &v));
        if (v == 0x03) return DEPZ_OK;
        depz_device_sleep_ms(dev, 1);
    }
    return depz_fail(DEPZ_E_TIMEOUT, "vl53l4cd: timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
}

static int clear_interrupt(depz_device *dev) { return wr_byte(dev, SYSTEM__INTERRUPT_CLEAR, 0x01); }

static int uld_start(depz_device *dev)
{
    uint32_t im;
    TRY(rd_dword(dev, INTERMEASUREMENT_MS, &im));
    /* 0 = continuous, anything else = autonomous low power. */
    return wr_byte(dev, SYSTEM_START, im == 0 ? 0x21 : 0x40);
}

static int uld_stop(depz_device *dev) { return wr_byte(dev, SYSTEM_START, 0x80); }

static int data_ready(depz_device *dev, bool *ready)
{
    uint8_t mux, status;
    int int_pol;
    TRY(rd_byte(dev, GPIO_HV_MUX__CTRL, &mux));
    int_pol = ((mux & 0x10) >> 4) == 1 ? 0 : 1;
    TRY(rd_byte(dev, GPIO__TIO_HV_STATUS, &status));
    *ready = (status & 1) == int_pol;
    return DEPZ_OK;
}

static int wait_data_ready(depz_device *dev, int timeout_ms)
{
    int i;
    for (i = 0; i < (timeout_ms > 1 ? timeout_ms : 1); i++) {
        bool ready;
        TRY(data_ready(dev, &ready));
        if (ready) return DEPZ_OK;
        depz_device_sleep_ms(dev, 1);
    }
    return depz_fail(DEPZ_E_TIMEOUT, "vl53l4cd: timeout waiting for data ready");
}

/* One block read instead of the C driver's six: the sensor auto-increments. */
static int get_result(depz_device *dev, depz_vl53l4_result *r)
{
    uint8_t raw[DEPZ_VL53L4_RESULT_BLOCK_LEN];
    TRY(depz_rb_read(&st(dev)->rb, DEPZ_VL53L4_RESULT_BLOCK_ADDR, raw, sizeof raw));
    if (depz_vl53l4_parse_result_block(raw, sizeof raw, r) != 0)
        return depz_fail(DEPZ_E_PROTOCOL, "vl53l4cd: result block did not decode");
    return DEPZ_OK;
}

static int uld_set_range_timing(depz_device *dev, uint32_t budget_ms, uint32_t inter_ms)
{
    uint16_t pll = 0, osc, a, b;
    uint32_t inter_raw;
    if (inter_ms > 0) TRY(rd_word(dev, RESULT__OSC_CALIBRATE_VAL, &pll));
    TRY(rd_word(dev, OSC_FREQUENCY, &osc));
    if (depz_vl53l4_range_timing_registers(budget_ms, inter_ms, osc, pll, &a, &b, &inter_raw))
        return depz_fail(DEPZ_E_ARG,
                         "vl53l4cd: budget must be 10..200 ms and inter 0 or above the budget "
                         "(got %lu / %lu)", (unsigned long)budget_ms, (unsigned long)inter_ms);
    TRY(wr_dword(dev, INTERMEASUREMENT_MS, inter_raw));
    TRY(wr_word(dev, RANGE_CONFIG_A, a));
    return wr_word(dev, RANGE_CONFIG_B, b);
}

static int uld_sensor_init(depz_device *dev, uint16_t bus_khz)
{
    uint8_t block[91];
    /* The configuration block goes at 400 kHz, the only speed an
     * unconfigured sensor is specified for; the bus is re-timed after it. */
    TRY(set_speed(dev, I2C_KHZ_BOOT));
    TRY(wait_boot(dev, 1000));
    depz_vl53l4_config_block(block);
    TRY(depz_rb_write(&st(dev)->rb, DEPZ_VL53L4_CONFIG_ADDR, block, sizeof block));
    if (bus_khz != I2C_KHZ_BOOT) TRY(set_speed(dev, bus_khz));
    TRY(wr_byte(dev, SYSTEM_START, 0x40)); /* start VHV */
    TRY(wait_data_ready(dev, 1000));
    TRY(clear_interrupt(dev));
    TRY(uld_stop(dev));
    TRY(wr_byte(dev, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09));
    TRY(wr_byte(dev, 0x000B, 0x00));
    TRY(wr_word(dev, 0x0024, 0x0500));
    return uld_set_range_timing(dev, 50, 0);
}

/* The ranging loop both calibrations share. */
typedef void (*sample_fn)(int i, const depz_vl53l4_result *r, void *ctx);

static int collect(depz_device *dev, int nb, sample_fn fn, void *ctx)
{
    int i;
    TRY(uld_start(dev));
    for (i = 0; i < nb; i++) {
        depz_vl53l4_result r;
        TRY(wait_data_ready(dev, 5000));
        TRY(get_result(dev, &r));
        TRY(clear_interrupt(dev));
        if (fn) fn(i, &r, ctx);
    }
    return uld_stop(dev);
}

/* ── public: lifecycle ─────────────────────────────────────────────────── */

int depz_vl53l4cd_is_alive(depz_device *dev, bool *alive)
{
    uint16_t id;
    int rc = need(dev);
    if (rc) return rc;
    rc = rd_word(dev, IDENTIFICATION__MODEL_ID, &id);
    if (alive) *alive = rc == DEPZ_OK && id == DEPZ_VL53L4_MODEL_ID;
    return rc;
}

int depz_vl53l4cd_init(depz_device *dev, uint16_t bus_khz)
{
    int rc = need_idle(dev);
    if (rc) return rc;
    rc = uld_sensor_init(dev, bus_khz ? bus_khz : DEPZ_VL53L4CD_I2C_KHZ_DEFAULT);
    if (rc == DEPZ_OK) set_flags(dev, -1, 1);
    return rc;
}

int depz_vl53l4cd_xshut(depz_device *dev, uint8_t action)
{
    int rc = need(dev);
    if (rc) return rc;
    rc = depz_device_request(dev, DEPZ_VL53L4_CMD_XSHUT, &action, 1, NULL, NULL, true,
                             action == DEPZ_VL53L4_XSHUT_RESET ? 2000 : -1);
    if (rc == DEPZ_OK) set_flags(dev, 0, 0);
    return rc;
}

int depz_vl53l4cd_reset_sensor(depz_device *dev)
{
    return depz_vl53l4cd_xshut(dev, DEPZ_VL53L4_XSHUT_RESET);
}

typedef struct { depz_vl53l4_info *out; bool ok; } info_ctx;

static bool match_info(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    info_ctx *x = (info_ctx *)c;
    if (cmd != DEPZ_VL53L4_RPT_INFO) return false;
    x->ok = depz_vl53l4_unpack_info(p, len, x->out) == 0;
    return true;
}

int depz_vl53l4cd_bridge_info(depz_device *dev, depz_vl53l4_info *out)
{
    info_ctx c;
    int rc = need(dev);
    if (rc) return rc;
    if (!out) return depz_fail(DEPZ_E_ARG, "bridge_info: NULL output");
    c.out = out;
    c.ok = false;
    rc = depz_device_request(dev, DEPZ_VL53L4_CMD_GET_INFO, NULL, 0, match_info, &c, false, -1);
    if (rc == DEPZ_OK && !c.ok) return depz_fail(DEPZ_E_PROTOCOL, "vl53l4cd: info report did not decode");
    return rc;
}

int depz_vl53l4cd_set_i2c_speed_khz(depz_device *dev, uint16_t khz)
{
    int rc = need_idle(dev);
    return rc ? rc : set_speed(dev, khz);
}

/* ── public: configuration ─────────────────────────────────────────────── */

int depz_vl53l4cd_set_range_timing(depz_device *dev, uint32_t budget_ms, uint32_t inter_ms)
{
    int rc = need_idle(dev);
    return rc ? rc : uld_set_range_timing(dev, budget_ms, inter_ms);
}

int depz_vl53l4cd_get_range_timing(depz_device *dev, uint32_t *budget_ms, uint32_t *inter_ms)
{
    uint32_t im;
    uint16_t pll, osc, a;
    TRY(need(dev));
    TRY(rd_dword(dev, INTERMEASUREMENT_MS, &im));
    TRY(rd_word(dev, RESULT__OSC_CALIBRATE_VAL, &pll));
    TRY(rd_word(dev, OSC_FREQUENCY, &osc));
    TRY(rd_word(dev, RANGE_CONFIG_A, &a));
    if (depz_vl53l4_decode_range_timing(im, pll, osc, a, budget_ms, inter_ms))
        return depz_fail(DEPZ_E_PROTOCOL, "vl53l4cd: OSC_FREQUENCY reads 0");
    return DEPZ_OK;
}

int depz_vl53l4cd_set_offset_mm(depz_device *dev, int32_t offset_mm)
{
    TRY(need_idle(dev));
    TRY(wr_word(dev, RANGE_OFFSET_MM, depz_vl53l4_offset_raw(offset_mm)));
    TRY(wr_word(dev, INNER_OFFSET_MM, 0));
    return wr_word(dev, OUTER_OFFSET_MM, 0);
}

int depz_vl53l4cd_get_offset_mm(depz_device *dev, int32_t *offset_mm)
{
    uint16_t w;
    TRY(need(dev));
    TRY(rd_word(dev, RANGE_OFFSET_MM, &w));
    *offset_mm = depz_vl53l4_decode_offset(w);
    return DEPZ_OK;
}

int depz_vl53l4cd_set_xtalk_kcps(depz_device *dev, uint16_t xtalk_kcps)
{
    TRY(need_idle(dev));
    TRY(wr_word(dev, XTALK_X_PLANE_GRADIENT_KCPS, 0));
    TRY(wr_word(dev, XTALK_Y_PLANE_GRADIENT_KCPS, 0));
    return wr_word(dev, XTALK_PLANE_OFFSET_KCPS, depz_vl53l4_xtalk_raw(xtalk_kcps));
}

int depz_vl53l4cd_get_xtalk_kcps(depz_device *dev, uint16_t *xtalk_kcps)
{
    uint16_t w;
    TRY(need(dev));
    TRY(rd_word(dev, XTALK_PLANE_OFFSET_KCPS, &w));
    *xtalk_kcps = depz_vl53l4_decode_xtalk(w);
    return DEPZ_OK;
}

int depz_vl53l4cd_set_detection_thresholds(depz_device *dev, uint16_t low_mm, uint16_t high_mm,
                                           uint8_t window)
{
    TRY(need_idle(dev));
    TRY(wr_byte(dev, SYSTEM__INTERRUPT, window));
    TRY(wr_word(dev, THRESH_HIGH, high_mm));
    return wr_word(dev, THRESH_LOW, low_mm);
}

int depz_vl53l4cd_get_detection_thresholds(depz_device *dev, uint16_t *low_mm, uint16_t *high_mm,
                                           uint8_t *window)
{
    uint8_t w;
    TRY(need(dev));
    TRY(rd_word(dev, THRESH_HIGH, high_mm));
    TRY(rd_word(dev, THRESH_LOW, low_mm));
    TRY(rd_byte(dev, SYSTEM__INTERRUPT, &w));
    *window = w & 0x07;
    return DEPZ_OK;
}

int depz_vl53l4cd_set_signal_threshold_kcps(depz_device *dev, uint16_t kcps)
{
    TRY(need_idle(dev));
    return wr_word(dev, MIN_COUNT_RATE_RTN_LIMIT_MCPS, depz_vl53l4_signal_threshold_raw(kcps));
}

int depz_vl53l4cd_get_signal_threshold_kcps(depz_device *dev, uint16_t *kcps)
{
    uint16_t w;
    TRY(need(dev));
    TRY(rd_word(dev, MIN_COUNT_RATE_RTN_LIMIT_MCPS, &w));
    *kcps = depz_vl53l4_decode_signal_threshold(w);
    return DEPZ_OK;
}

int depz_vl53l4cd_set_sigma_threshold_mm(depz_device *dev, uint16_t mm)
{
    uint16_t raw;
    TRY(need_idle(dev));
    if (depz_vl53l4_sigma_threshold_raw(mm, &raw))
        return depz_fail(DEPZ_E_ARG, "vl53l4cd: sigma threshold must be <= 16383 mm (got %u)", mm);
    return wr_word(dev, RANGE_CONFIG__SIGMA_THRESH, raw);
}

int depz_vl53l4cd_get_sigma_threshold_mm(depz_device *dev, uint16_t *mm)
{
    uint16_t w;
    TRY(need(dev));
    TRY(rd_word(dev, RANGE_CONFIG__SIGMA_THRESH, &w));
    *mm = depz_vl53l4_decode_sigma_threshold(w);
    return DEPZ_OK;
}

int depz_vl53l4cd_start_temperature_update(depz_device *dev)
{
    TRY(need_idle(dev));
    TRY(wr_byte(dev, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81));
    TRY(wr_byte(dev, 0x000B, 0x92));
    TRY(uld_start(dev));
    TRY(wait_data_ready(dev, 1000));
    TRY(clear_interrupt(dev));
    TRY(uld_stop(dev));
    TRY(wr_byte(dev, VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09));
    return wr_byte(dev, 0x000B, 0x00);
}

/* ── public: calibration (VL53L4CD_calibration.c) ─────────────────────── */

typedef struct { long sum; } offset_acc;

static void add_distance(int i, const depz_vl53l4_result *r, void *c)
{
    (void)i;
    ((offset_acc *)c)->sum += r->distance_mm;
}

int depz_vl53l4cd_calibrate_offset(depz_device *dev, uint16_t target_mm, uint8_t nb_samples,
                                   int32_t *offset_mm)
{
    offset_acc acc;
    int32_t off;
    int nb = nb_samples ? nb_samples : 20;
    TRY(need_idle(dev));
    if (nb < 5 || target_mm < 10 || target_mm > 1000)
        return depz_fail(DEPZ_E_ARG, "vl53l4cd: nb_samples must be 5..255, target 10..1000 mm");
    TRY(wr_word(dev, RANGE_OFFSET_MM, 0));
    TRY(wr_word(dev, INNER_OFFSET_MM, 0));
    TRY(wr_word(dev, OUTER_OFFSET_MM, 0));
    TRY(collect(dev, 10, NULL, NULL)); /* device heat loop */
    acc.sum = 0;
    TRY(collect(dev, nb, add_distance, &acc));
    off = (int32_t)target_mm - (int32_t)(acc.sum / nb);
    TRY(wr_word(dev, RANGE_OFFSET_MM, depz_vl53l4_offset_raw(off)));
    if (offset_mm) *offset_mm = off;
    return DEPZ_OK;
}

typedef struct { int n; double distance, spads, signal; } xtalk_acc;

static void add_xtalk(int i, const depz_vl53l4_result *r, void *c)
{
    xtalk_acc *a = (xtalk_acc *)c;
    /* Discard invalid measurements and the first frame. */
    if (r->range_status != 0 || i == 0) return;
    a->n++;
    a->distance += r->distance_mm;
    a->spads += r->number_of_spad;
    a->signal += r->signal_rate_kcps;
}

int depz_vl53l4cd_calibrate_xtalk(depz_device *dev, uint16_t target_mm, uint8_t nb_samples,
                                  uint16_t *xtalk_kcps)
{
    xtalk_acc acc;
    double tmp;
    int nb = nb_samples ? nb_samples : 20;
    TRY(need_idle(dev));
    if (nb < 5 || target_mm < 10 || target_mm > 5000)
        return depz_fail(DEPZ_E_ARG, "vl53l4cd: nb_samples must be 5..255, target 10..5000 mm");
    TRY(wr_word(dev, XTALK_PLANE_OFFSET_KCPS, 0)); /* disable compensation */
    TRY(collect(dev, 10, NULL, NULL));              /* device heat loop */
    memset(&acc, 0, sizeof acc);
    TRY(collect(dev, nb, add_xtalk, &acc));
    if (!acc.n) return depz_fail(DEPZ_E_PROTOCOL, "vl53l4cd: xtalk calibration failed: no valid samples");
    tmp = (1.0 - (acc.distance / acc.n) / (double)target_mm) * ((acc.signal / acc.n) / (acc.spads / acc.n));
    if (tmp > 127) /* 127 kcps is the max xtalk value (65536/512) */
        return depz_fail(DEPZ_E_PROTOCOL, "vl53l4cd: xtalk calibration failed: %.1f kcps > 127", tmp);
    TRY(wr_word(dev, XTALK_PLANE_OFFSET_KCPS, (uint16_t)((int)(tmp * 512.0) & 0xFFFF)));
    if (xtalk_kcps) *xtalk_kcps = (uint16_t)(tmp < 0 ? 0 : (int)(tmp + 0.5));
    return DEPZ_OK;
}

/* ── public: ranging ───────────────────────────────────────────────────── */

int depz_vl53l4cd_start_ranging(depz_device *dev)
{
    uint8_t p[5];
    TRY(need_idle(dev));
    TRY(uld_start(dev));
    depz_vl53l4_pack_start_stream(DEPZ_VL53L4_RESULT_BLOCK_ADDR, DEPZ_VL53L4_RESULT_BLOCK_LEN, 0, p);
    TRY(depz_device_request(dev, DEPZ_VL53L4_CMD_START_STREAM, p, 5, NULL, NULL, true, -1));
    set_flags(dev, 1, -1);
    return DEPZ_OK;
}

int depz_vl53l4cd_stop_ranging(depz_device *dev)
{
    int rc, rc2;
    TRY(need(dev));
    if (!get_flag(dev, true)) return DEPZ_OK;
    /* Clear host state and stop the sensor even when the stream stop fails,
     * or the device stays wedged "ranging". */
    rc = depz_device_request(dev, DEPZ_VL53L4_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1);
    set_flags(dev, 0, -1);
    rc2 = uld_stop(dev);
    return rc ? rc : rc2;
}

int depz_vl53l4cd_measure_once(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out)
{
    depz_vl53l4_result r;
    int rc;
    TRY(need_idle(dev));
    TRY(uld_start(dev));
    rc = wait_data_ready(dev, timeout_ms < 0 ? 1000 : timeout_ms);
    if (rc == DEPZ_OK) rc = get_result(dev, &r);
    if (rc == DEPZ_OK) rc = clear_interrupt(dev);
    if (rc) {
        /* Stop the sensor anyway, but report the first failure. */
        char msg[512];
        depz_strlcpy(msg, depz_last_error(), sizeof msg);
        uld_stop(dev);
        return depz_fail(rc, "%s", msg);
    }
    TRY(uld_stop(dev));
    if (out) {
        out->timestamp_us = st(dev)->rb.last_timestamp_us;
        out->r = r;
    }
    return DEPZ_OK;
}

int depz_vl53l4cd_on_measurement(depz_device *dev, depz_vl53l4cd_measurement_cb cb, void *user,
                                 int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "vl53l4cd: NULL callback");
    return depz_cb_list_add(&st(dev)->cbs, (void (*)(void))cb, user, token);
}

void depz_vl53l4cd_off_measurement(depz_device *dev, int token)
{
    if (depz_is_vl53l4cd(dev)) depz_cb_list_remove(&st(dev)->cbs, token);
}

depz_stream *depz_vl53l4cd_stream(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 64);
}

int depz_vl53l4cd_get_measurement(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out)
{
    depz_stream *s;
    int rc;
    TRY(need(dev));
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53l4cd: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 2000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

uint64_t depz_vl53l4cd_stream_parse_errors(const depz_device *dev)
{
    uint64_t n;
    l4_state *s;
    if (!depz_is_vl53l4cd(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->parse_errors;
    depz_mutex_unlock(&s->lock);
    return n;
}

int depz_vl53l4cd_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    TRY(need(dev));
    return depz_rb_read(&st(dev)->rb, addr, buf, len);
}

int depz_vl53l4cd_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len)
{
    TRY(need(dev));
    return depz_rb_write(&st(dev)->rb, addr, data, len);
}
