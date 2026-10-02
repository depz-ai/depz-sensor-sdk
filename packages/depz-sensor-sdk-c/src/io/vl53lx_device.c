/* vl53lx_device.c — the VL53L 1D family class (contract 12): the bridge, the
 * (product, driver) binding, configuration, the stream. The Python SDK's
 * vl53lx/__init__.py, request for request; the drivers are vl53lx_*.c. */
#include "vl53lx_internal.h"

#include <stdlib.h>

#define REG_TIMEOUT_MS   2000
#define XSHUT_TIMEOUT_MS 1000

typedef struct {
    depz_mutex seq;               /* serialises multi-step sequences */
    depz_mutex lock;              /* the fields the reader reads */
    vlx_plat   plat;
    vlx_driver *drv;
    depz_vl53lx_product class_product;  /* fixed by the class; NONE = generic */
    depz_vl53lx_product product;
    depz_vl53lx_driver  kind;
    const char *caveat;
    bool name_read;
    char board_name[128];
    bool ranging;
    uint64_t parse_errors;
    depz_cb_list cbs;
    depz_hub *hub;
} vlx_state;

typedef void (*vlx_fn)(const depz_vl53lx_measurement *, void *);

static vlx_state *st(depz_device *dev) { return (vlx_state *)dev->sensor; }

/* ── platform ──────────────────────────────────────────────────────────── */

int vlx_rd_multi(vlx_plat *p, uint16_t addr, uint8_t *out, size_t len) { return depz_rb_read(&p->rb, addr, out, len); }
int vlx_wr_multi(vlx_plat *p, uint16_t addr, const uint8_t *data, size_t len) { return depz_rb_write(&p->rb, addr, data, len); }

int vlx_rd_byte(vlx_plat *p, uint16_t addr, uint8_t *v) { return vlx_rd_multi(p, addr, v, 1); }

int vlx_rd_word(vlx_plat *p, uint16_t addr, uint16_t *v)
{
    uint8_t b[2];
    VLX_TRY(vlx_rd_multi(p, addr, b, 2));
    *v = (uint16_t)(b[0] << 8 | b[1]);
    return DEPZ_OK;
}

int vlx_rd_dword(vlx_plat *p, uint16_t addr, uint32_t *v)
{
    uint8_t b[4];
    VLX_TRY(vlx_rd_multi(p, addr, b, 4));
    *v = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    return DEPZ_OK;
}

int vlx_wr_byte(vlx_plat *p, uint16_t addr, uint8_t v) { return vlx_wr_multi(p, addr, &v, 1); }

int vlx_wr_word(vlx_plat *p, uint16_t addr, uint16_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)(v >> 8);
    b[1] = (uint8_t)v;
    return vlx_wr_multi(p, addr, b, 2);
}

int vlx_wr_dword(vlx_plat *p, uint16_t addr, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
    return vlx_wr_multi(p, addr, b, 4);
}

int vlx_set_i2c_speed(vlx_plat *p, uint16_t khz)
{
    uint8_t b[2];
    b[0] = (uint8_t)khz;
    b[1] = (uint8_t)(khz >> 8);
    return depz_device_request(p->dev, DEPZ_VL53LX_CMD_SET_I2C_SPEED, b, 2, NULL, NULL, true, -1);
}

int vlx_set_addr_width(vlx_plat *p, uint8_t width)
{
    uint8_t b[1];
    if (!depz_vl53lx_pack_set_addr_width(width, b)) return depz_fail(DEPZ_E_ARG, "vl53lx: address width %u", width);
    return depz_device_request(p->dev, DEPZ_VL53LX_CMD_SET_ADDR_WIDTH, b, 1, NULL, NULL, true, -1);
}

static int xshut_cmd(vlx_plat *p, uint8_t action)
{
    uint8_t b[1];
    b[0] = action;
    return depz_device_request(p->dev, DEPZ_VL53LX_CMD_XSHUT, b, 1, NULL, NULL, true, XSHUT_TIMEOUT_MS);
}

int vlx_xshut_reset(vlx_plat *p) { return xshut_cmd(p, DEPZ_VL53L4_XSHUT_RESET); }
void vlx_sleep_ms(vlx_plat *p, int ms) { depz_device_sleep_ms(p->dev, ms); }
uint64_t vlx_last_timestamp_us(const vlx_plat *p) { return p->rb.last_timestamp_us; }

/* Zero the bridge's I2C error counters: a resetting die NACKs its own
 * address for a moment, and only the driver knows those were expected. */
static int clear_i2c_errors(vlx_plat *p)
{
    int rc = depz_device_request(p->dev, DEPZ_VL53LX_CMD_CLEAR_I2C_ERRORS, NULL, 0, NULL, NULL, true, -1);
    if (rc == DEPZ_E_STATUS && depz_last_status() == DEPZ_STATUS_ERR_INVALID_CMD)
        return depz_fail(DEPZ_E_STATUS, "the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) and "
                                         "cannot clear its I2C error counter — reflash it");
    return rc;
}

int vlx_wait_data_ready(vlx_driver *d, int timeout_ms)
{
    uint64_t deadline = vlx_deadline_ms(timeout_ms);
    for (;;) {
        bool ready = false;
        VLX_TRY(d->ops->check_for_data_ready(d, &ready));
        if (ready) return DEPZ_OK;
        if (vlx_past(deadline)) return depz_fail(DEPZ_E_TIMEOUT, "vl53lx: timeout waiting for data ready");
        vlx_sleep_ms(d->p, 1);
    }
}

void vlx_measurement_init(depz_vl53lx_measurement *m)
{
    memset(m, 0, sizeof *m);
    m->status_text = "";
    m->stream_count = -1;
    m->dmax_mm = -1;
    m->device_range_status = -1;
    m->min_range_mm = m->max_range_mm = m->peak_bin = -1;
}

/* ── the product x driver table (uld/registry.py) ─────────────────────── */

#define HIST_CAVEAT "the histogram driver has no calibrations and no detection thresholds - the light drivers are the ones with those"

typedef vlx_driver *(*vlx_ctor)(vlx_plat *, depz_vl53lx_product);

static vlx_ctor driver_ctor(depz_vl53lx_product p, depz_vl53lx_driver kind, const char **caveat)
{
    *caveat = NULL;
    switch (p) {
    case DEPZ_VL53LX_PRODUCT_L0X:
        if (kind == DEPZ_VL53LX_DRIVER_ULD) {
            *caveat = "no detection thresholds; there is no ROI on this die at all";
            return vlx_new_l0x;
        }
        return NULL;
    case DEPZ_VL53LX_PRODUCT_L1CX:
    case DEPZ_VL53LX_PRODUCT_L1CB:
        if (kind == DEPZ_VL53LX_DRIVER_ULD) return vlx_new_l1;
        if (kind == DEPZ_VL53LX_DRIVER_HISTOGRAM) { *caveat = HIST_CAVEAT; return vlx_new_bare; }
        return NULL;
    case DEPZ_VL53LX_PRODUCT_L3CX:
        if (kind == DEPZ_VL53LX_DRIVER_ULP) {
            *caveat = "single-target ranging only - the histogram driver gives several targets instead";
            return vlx_new_l3;
        }
        if (kind == DEPZ_VL53LX_DRIVER_HISTOGRAM) { *caveat = HIST_CAVEAT; return vlx_new_bare; }
        return NULL;
    case DEPZ_VL53LX_PRODUCT_L4CD:
        if (kind == DEPZ_VL53LX_DRIVER_ULD) return vlx_new_l4;
        if (kind == DEPZ_VL53LX_DRIVER_HISTOGRAM) { *caveat = HIST_CAVEAT; return vlx_new_bare; }
        return NULL;
    case DEPZ_VL53LX_PRODUCT_L4CX:
        if (kind == DEPZ_VL53LX_DRIVER_HISTOGRAM) { *caveat = HIST_CAVEAT; return vlx_new_bare; }
        return NULL;
    default:
        return NULL;
    }
}

unsigned depz_vl53lx_driver_kinds(depz_vl53lx_product product)
{
    static const depz_vl53lx_driver order[3] = {DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_DRIVER_ULP,
                                                DEPZ_VL53LX_DRIVER_HISTOGRAM};
    unsigned kinds = 0;
    const char *c;
    int i;
    for (i = 0; i < 3; i++)
        if (driver_ctor(product, order[i], &c)) kinds |= (unsigned)order[i];
    return kinds;
}

/* ── status helpers ────────────────────────────────────────────────────── */

bool depz_vl53lx_status_plottable(int status) { return status == 0 || status == 6 || status == 11; }

bool depz_vl53lx_plottable(const depz_vl53lx_measurement *m) { return m && depz_vl53lx_status_plottable(m->status); }

bool depz_vl53lx_primary_distance(const depz_vl53lx_measurement *m, int32_t *distance_mm)
{
    size_t i;
    if (!m) return false;
    if (m->n_targets) {
        for (i = 0; i < m->n_targets; i++) {
            if (depz_vl53lx_status_plottable(m->targets[i].status)) {
                if (distance_mm) *distance_mm = m->targets[i].distance_mm;
                return true;
            }
        }
        return false;
    }
    if (!depz_vl53lx_plottable(m)) return false;
    if (distance_mm) *distance_mm = m->distance_mm;
    return true;
}

/* ── class plumbing ────────────────────────────────────────────────────── */

static bool vlx_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    vlx_state *s = st(dev);
    depz_vl53l4_stream sd;
    depz_vl53lx_measurement m;
    depz_cb_entry *cbs;
    size_t i, n;
    int rc;
    if (cmd != DEPZ_VL53LX_RPT_STREAM || len < 12) return false;
    /* The driver's decode state is only touched here while ranging, and
     * configuration is refused then — no other thread uses the driver. */
    if (!s->drv || depz_vl53l4_unpack_stream(p, len, &sd) != 0) return true;
    vlx_measurement_init(&m);
    rc = s->drv->ops->decode(s->drv, sd.data, sd.len, &m);
    if (rc) {
        depz_mutex_lock(&s->lock);
        s->parse_errors++;
        depz_mutex_unlock(&s->lock);
        return true;
    }
    m.timestamp_us = sd.timestamp_us;
    n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) ((vlx_fn)cbs[i].fn)(&m, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, &m);
    return true;
}

static void vlx_closed(depz_device *dev) { depz_hub_mark_closed(st(dev)->hub); }

static void vlx_destroy(depz_device *dev)
{
    vlx_state *s = st(dev);
    if (!s) return;
    if (s->drv) s->drv->ops->destroy(s->drv);
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_mutex_destroy(&s->lock);
    depz_mutex_destroy(&s->seq);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops vlx_ops_class = {DEPZ_SENSOR_VL53LX, vlx_report, vlx_closed, vlx_destroy};

static vlx_state *vlx_new(depz_device *dev, depz_vl53lx_product class_product)
{
    vlx_state *s = (vlx_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->lock)) { free(s); return NULL; }
    if (depz_mutex_init(&s->seq)) goto fail1;
    if (depz_cb_list_init(&s->cbs)) goto fail2;
    s->hub = depz_hub_new(sizeof(depz_vl53lx_measurement));
    if (!s->hub) goto fail3;
    s->class_product = class_product;
    s->product = DEPZ_VL53LX_PRODUCT_NONE;
    s->plat.dev = dev;
    s->plat.rb.dev = dev;
    s->plat.rb.cmd_read = DEPZ_VL53LX_CMD_READ_REG;
    s->plat.rb.cmd_write = DEPZ_VL53LX_CMD_WRITE_REG;
    s->plat.rb.rpt_reg_data = DEPZ_VL53LX_RPT_REG_DATA;
    s->plat.rb.xfer_max = DEPZ_VL53L4_XFER_MAX;
    s->plat.rb.timeout_ms = REG_TIMEOUT_MS;
    return s;
fail3:
    depz_cb_list_free(&s->cbs);
fail2:
    depz_mutex_destroy(&s->seq);
fail1:
    depz_mutex_destroy(&s->lock);
    free(s);
    return NULL;
}

/* The product a resolved class fixes (NONE for the generic class). */
static depz_vl53lx_product class_product_of(depz_vl53lx_class c)
{
    switch (c) {
    case DEPZ_VL53LX_CLASS_L0X:  return DEPZ_VL53LX_PRODUCT_L0X;
    case DEPZ_VL53LX_CLASS_L1CX: return DEPZ_VL53LX_PRODUCT_L1CX;
    case DEPZ_VL53LX_CLASS_L1CB: return DEPZ_VL53LX_PRODUCT_L1CB;
    case DEPZ_VL53LX_CLASS_L3CX: return DEPZ_VL53LX_PRODUCT_L3CX;
    case DEPZ_VL53LX_CLASS_L4CX: return DEPZ_VL53LX_PRODUCT_L4CX;
    default:                     return DEPZ_VL53LX_PRODUCT_NONE;
    }
}

int depz_vl53lx_attach(depz_device *dev, const char *usb_model, const char *device_name)
{
    vlx_state *s;
    if (dev->ops == &vlx_ops_class) return DEPZ_OK;
    s = vlx_new(dev, class_product_of(depz_vl53lx_resolve_class(usb_model, device_name)));
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53lx: out of memory");
    depz_device_attach(dev, &vlx_ops_class, s);
    return DEPZ_OK;
}

int depz_vl53lx_open_link(depz_link *link, depz_device **out)
{
    int rc = depz_device_open_link(link, out);
    if (rc) return rc;
    rc = depz_vl53lx_attach(*out, NULL, NULL);
    if (rc) { depz_device_close(*out); *out = NULL; }
    return rc;
}

bool depz_is_vl53lx(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &vlx_ops_class;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

static int need(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL device");
    if (!depz_is_vl53lx(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a VL53L 1D-family board", dev->port);
    return DEPZ_OK;
}

static bool is_ranging(depz_device *dev)
{
    vlx_state *s = st(dev);
    bool v;
    depz_mutex_lock(&s->lock);
    v = s->ranging;
    depz_mutex_unlock(&s->lock);
    return v;
}

static void set_ranging(depz_device *dev, bool v)
{
    vlx_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    s->ranging = v;
    depz_mutex_unlock(&s->lock);
}

static int need_not_ranging(depz_device *dev)
{
    if (is_ranging(dev)) return depz_fail(DEPZ_E_ARG, "vl53lx: stop ranging first — the stream owns the bus");
    return DEPZ_OK;
}

static int need_driver(depz_device *dev)
{
    if (!st(dev)->drv) return depz_fail(DEPZ_E_ARG, "vl53lx: call init() first");
    return DEPZ_OK;
}

static int need_cap(depz_device *dev, unsigned cap, const char *group)
{
    vlx_state *s = st(dev);
    VLX_TRY(need_driver(dev));
    if (!(s->drv->ops->supports & cap))
        return depz_fail(DEPZ_E_ARG, "%s/%s does not support '%s'%s%s%s", depz_vl53lx_product_get(s->product)->name,
                         depz_vl53lx_driver_str(s->kind), group, s->caveat ? " (" : "", s->caveat ? s->caveat : "",
                         s->caveat ? ")" : "");
    return DEPZ_OK;
}

#define LOCKED(body)                      \
    do {                                  \
        int rc_;                          \
        VLX_TRY(need(dev));               \
        depz_mutex_lock(&st(dev)->seq);   \
        rc_ = (body);                     \
        depz_mutex_unlock(&st(dev)->seq); \
        return rc_;                       \
    } while (0)

/* ── identity ──────────────────────────────────────────────────────────── */

/* The board's device name, read once (a request, as the Python property). */
static const char *board_name(depz_device *dev)
{
    vlx_state *s = st(dev);
    if (!s->name_read) {
        if (depz_device_get_device_name(dev, s->board_name, sizeof s->board_name) != DEPZ_OK) s->board_name[0] = '\0';
        s->name_read = true;
    }
    return s->board_name;
}

depz_vl53lx_product depz_vl53lx_detected_product(depz_device *dev)
{
    depz_vl53lx_product p;
    if (need(dev)) return DEPZ_VL53LX_PRODUCT_NONE;
    depz_mutex_lock(&st(dev)->seq);
    p = depz_vl53lx_product_from_board_name(board_name(dev));
    depz_mutex_unlock(&st(dev)->seq);
    return p;
}

/* ── lifecycle ─────────────────────────────────────────────────────────── */

static int sensor_init(vlx_driver *drv)
{
    VLX_TRY(drv->ops->sensor_init(drv));
    return clear_i2c_errors(drv->p);
}

static int do_init(depz_device *dev, depz_vl53lx_driver kind, depz_vl53lx_product product)
{
    vlx_state *s = st(dev);
    const char *caveat;
    vlx_ctor ctor;
    vlx_driver *drv;
    int rc;
    VLX_TRY(need_not_ranging(dev));
    if (product == DEPZ_VL53LX_PRODUCT_NONE) product = s->class_product;
    if (product == DEPZ_VL53LX_PRODUCT_NONE) product = depz_vl53lx_product_from_board_name(board_name(dev));
    if (product == DEPZ_VL53LX_PRODUCT_NONE || !depz_vl53lx_product_get(product))
        return depz_fail(DEPZ_E_ARG, "vl53lx: board '%s' carries no product number — name the product", s->board_name);
    if (!kind) {
        unsigned kinds = depz_vl53lx_driver_kinds(product);
        kind = kinds & DEPZ_VL53LX_DRIVER_ULD ? DEPZ_VL53LX_DRIVER_ULD
             : kinds & DEPZ_VL53LX_DRIVER_ULP ? DEPZ_VL53LX_DRIVER_ULP : DEPZ_VL53LX_DRIVER_HISTOGRAM;
    }
    ctor = driver_ctor(product, kind, &caveat);
    if (!ctor) {
        unsigned kinds = depz_vl53lx_driver_kinds(product);
        return depz_fail(DEPZ_E_ARG, "vl53lx: %s has no '%s' driver - it has%s%s%s",
                         depz_vl53lx_product_get(product)->name, depz_vl53lx_driver_str(kind) ? depz_vl53lx_driver_str(kind) : "?",
                         kinds & DEPZ_VL53LX_DRIVER_ULD ? " uld" : "", kinds & DEPZ_VL53LX_DRIVER_ULP ? " ulp" : "",
                         kinds & DEPZ_VL53LX_DRIVER_HISTOGRAM ? " histogram" : "");
    }
    drv = ctor(&s->plat, product);
    if (!drv) return depz_fail(DEPZ_E_NOMEM, "vl53lx: out of memory");
    /* Sticky on the bridge and 2 after a reset: set before the first register
     * access, model id included. */
    rc = vlx_set_addr_width(&s->plat, drv->ops->addr_width);
    if (!rc) rc = sensor_init(drv);
    if (rc) {
        drv->ops->destroy(drv);
        return rc;
    }
    if (s->drv) s->drv->ops->destroy(s->drv);
    s->drv = drv;
    s->product = product;
    s->kind = kind;
    s->caveat = caveat;
    return DEPZ_OK;
}

int depz_vl53lx_init(depz_device *dev, depz_vl53lx_driver driver, depz_vl53lx_product product)
{
    LOCKED(do_init(dev, driver, product));
}

bool depz_vl53lx_initialized(const depz_device *dev) { return depz_is_vl53lx(dev) && st((depz_device *)dev)->drv; }

depz_vl53lx_product depz_vl53lx_product_bound(const depz_device *dev)
{
    return depz_is_vl53lx(dev) ? st((depz_device *)dev)->product : DEPZ_VL53LX_PRODUCT_NONE;
}

depz_vl53lx_driver depz_vl53lx_driver_bound(const depz_device *dev)
{
    return depz_is_vl53lx(dev) ? st((depz_device *)dev)->kind : (depz_vl53lx_driver)0;
}

const char *depz_vl53lx_caveat(const depz_device *dev)
{
    const char *c = depz_is_vl53lx(dev) ? st((depz_device *)dev)->caveat : NULL;
    return c ? c : "";
}

static int do_driver_reach_mm(depz_device *dev, uint32_t *out)
{
    vlx_driver *d = st(dev)->drv;
    *out = 0;
    if (!d || !d->ops->reach_mm) return DEPZ_OK;
    return d->ops->reach_mm(d, out);
}

static int locked_driver_reach_mm(depz_device *dev, uint32_t *out) { LOCKED(do_driver_reach_mm(dev, out)); }

uint32_t depz_vl53lx_driver_reach_mm(const depz_device *dev)
{
    uint32_t mm = 0;
    if (!depz_is_vl53lx(dev)) return 0;
    /* Reads the sensor on the VL53L1 die and the VL53L0X: 0 on a failed read. */
    if (locked_driver_reach_mm((depz_device *)dev, &mm)) return 0;
    return mm;
}

bool depz_vl53lx_supports(const depz_device *dev, unsigned cap)
{
    vlx_state *s;
    if (!depz_is_vl53lx(dev)) return false;
    s = st((depz_device *)dev);
    return s->drv && (s->drv->ops->supports & cap) == cap;
}

bool depz_vl53lx_ranging(const depz_device *dev) { return depz_is_vl53lx(dev) && is_ranging((depz_device *)dev); }

static int do_model_id(depz_device *dev, uint16_t *out)
{
    VLX_TRY(need_driver(dev));
    return st(dev)->drv->ops->model_id(st(dev)->drv, out);
}

int depz_vl53lx_model_id(depz_device *dev, uint16_t *model_id) { LOCKED(do_model_id(dev, model_id)); }

size_t depz_vl53lx_modes(const depz_device *dev, const char **names, size_t cap)
{
    vlx_state *s;
    if (!depz_is_vl53lx(dev)) return 0;
    s = st((depz_device *)dev);
    if (!s->drv || !s->drv->ops->modes) return 0;
    return s->drv->ops->modes(s->drv, names, cap);
}

int depz_vl53lx_budget_range(const depz_device *dev, int *min_ms, int *max_ms)
{
    vlx_state *s;
    VLX_TRY(need(dev));
    s = st((depz_device *)dev);
    if (!s->drv) return depz_fail(DEPZ_E_ARG, "vl53lx: call init() first");
    if (s->drv->ops->budget_range) {
        int lo, hi;
        s->drv->ops->budget_range(s->drv, &lo, &hi);
        if (min_ms) *min_ms = lo;
        if (max_ms) *max_ms = hi;
    } else {
        if (min_ms) *min_ms = s->drv->ops->budget_min_ms;
        if (max_ms) *max_ms = s->drv->ops->budget_max_ms;
    }
    return DEPZ_OK;
}

static int do_budget_choices(depz_device *dev, int *choices, size_t cap, size_t *n)
{
    vlx_driver *d;
    VLX_TRY(need_driver(dev));
    d = st(dev)->drv;
    *n = d->ops->budget_choices ? d->ops->budget_choices(d, choices, cap) : 0;
    return DEPZ_OK;
}

int depz_vl53lx_budget_choices(depz_device *dev, int *choices, size_t cap, size_t *n)
{
    size_t dummy;
    if (!n) n = &dummy;
    LOCKED(do_budget_choices(dev, choices, cap, n));
}

static int do_xshut(depz_device *dev, uint8_t action)
{
    vlx_state *s = st(dev);
    VLX_TRY(xshut_cmd(&s->plat, action));
    set_ranging(dev, false);
    if (s->drv) s->drv->ops->destroy(s->drv);
    s->drv = NULL;
    return DEPZ_OK;
}

int depz_vl53lx_xshut(depz_device *dev, uint8_t action) { LOCKED(do_xshut(dev, action)); }

typedef struct { depz_vl53lx_info *out; bool ok; } info_ctx;

static bool match_info(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    info_ctx *i = (info_ctx *)c;
    if (cmd != DEPZ_VL53LX_RPT_INFO) return false;
    i->ok = depz_vl53lx_unpack_info(p, len, i->out) == 0;
    return true;
}

int depz_vl53lx_bridge_info(depz_device *dev, depz_vl53lx_info *out)
{
    info_ctx c;
    VLX_TRY(need(dev));
    if (!out) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL output");
    c.out = out;
    c.ok = false;
    VLX_TRY(depz_device_request(dev, DEPZ_VL53LX_CMD_GET_INFO, NULL, 0, match_info, &c, false, -1));
    if (!c.ok) return depz_fail(DEPZ_E_PROTOCOL, "RPT_VL53_INFO: short payload");
    return DEPZ_OK;
}

/* ── configuration ─────────────────────────────────────────────────────── */

static int do_configure(depz_device *dev, int budget_ms, int inter_ms, const char *mode, const int32_t *offset_mm,
                        const int32_t *xtalk_kcps, const int *signal_kcps)
{
    vlx_driver *d;
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_driver(dev));
    d = st(dev)->drv;
    if (offset_mm) VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_OFFSET, "offset"));
    if (xtalk_kcps) VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_XTALK, "xtalk"));
    if (signal_kcps) VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_SIGNAL_THRESH, "signal_thresh"));
    if (mode && !d->ops->set_mode) return depz_fail(DEPZ_E_ARG, "vl53lx: this driver has no modes");
    VLX_TRY(sensor_init(d));
    if (mode) VLX_TRY(d->ops->set_mode(d, mode));
    VLX_TRY(d->ops->set_range_timing(d, budget_ms, inter_ms));
    if (offset_mm) VLX_TRY(d->ops->set_offset(d, *offset_mm));
    if (xtalk_kcps) VLX_TRY(d->ops->set_xtalk(d, *xtalk_kcps));
    if (signal_kcps) VLX_TRY(d->ops->set_signal_threshold(d, *signal_kcps));
    return DEPZ_OK;
}

int depz_vl53lx_configure(depz_device *dev, int budget_ms, int inter_ms, const char *mode, const int32_t *offset_mm,
                          const int32_t *xtalk_kcps)
{
    LOCKED(do_configure(dev, budget_ms, inter_ms, mode, offset_mm, xtalk_kcps, NULL));
}

int depz_vl53lx_configure_ex(depz_device *dev, int budget_ms, int inter_ms, const char *mode,
                             const int32_t *offset_mm, const int32_t *xtalk_kcps, const int *signal_kcps)
{
    LOCKED(do_configure(dev, budget_ms, inter_ms, mode, offset_mm, xtalk_kcps, signal_kcps));
}

static int do_get_timing(depz_device *dev, int *budget_ms, int *inter_ms)
{
    int b = 0, i = 0;
    VLX_TRY(need_driver(dev));
    VLX_TRY(st(dev)->drv->ops->get_range_timing(st(dev)->drv, &b, &i));
    if (budget_ms) *budget_ms = b;
    if (inter_ms) *inter_ms = i;
    return DEPZ_OK;
}

int depz_vl53lx_get_range_timing(depz_device *dev, int *budget_ms, int *inter_ms) { LOCKED(do_get_timing(dev, budget_ms, inter_ms)); }

static int do_set_mode(depz_device *dev, const char *mode)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_MODE, "mode"));
    if (!mode) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL mode");
    return st(dev)->drv->ops->set_mode(st(dev)->drv, mode);
}

int depz_vl53lx_set_mode(depz_device *dev, const char *mode) { LOCKED(do_set_mode(dev, mode)); }

static int do_get_mode(depz_device *dev, const char **mode)
{
    vlx_driver *d;
    VLX_TRY(need_driver(dev));
    d = st(dev)->drv;
    *mode = NULL;
    if (!(d->ops->supports & DEPZ_VL53LX_CAP_MODE)) return DEPZ_OK;
    return d->ops->get_mode(d, mode);
}

int depz_vl53lx_get_mode(depz_device *dev, const char **mode)
{
    if (!mode) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL output");
    LOCKED(do_get_mode(dev, mode));
}

/* Capability-gated pass-throughs: one macro per shape. */
#define SETTER(fn, cap, group, op, T)                                         \
    static int do_##fn(depz_device *dev, T v)                                 \
    {                                                                         \
        VLX_TRY(need_not_ranging(dev));                                       \
        VLX_TRY(need_cap(dev, cap, group));                                   \
        return st(dev)->drv->ops->op(st(dev)->drv, v);                        \
    }                                                                         \
    int depz_vl53lx_##fn(depz_device *dev, T v) { LOCKED(do_##fn(dev, v)); }

#define GETTER(fn, cap, group, op, T)                                         \
    static int do_##fn(depz_device *dev, T *v)                                \
    {                                                                         \
        VLX_TRY(need_cap(dev, cap, group));                                   \
        return st(dev)->drv->ops->op(st(dev)->drv, v);                        \
    }                                                                         \
    int depz_vl53lx_##fn(depz_device *dev, T *v)                              \
    {                                                                         \
        if (!v) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL output");          \
        LOCKED(do_##fn(dev, v));                                              \
    }

SETTER(set_offset_mm, DEPZ_VL53LX_CAP_OFFSET, "offset", set_offset, int32_t)
GETTER(get_offset_mm, DEPZ_VL53LX_CAP_OFFSET, "offset", get_offset, int32_t)
SETTER(set_xtalk_kcps, DEPZ_VL53LX_CAP_XTALK, "xtalk", set_xtalk, int32_t)
GETTER(get_xtalk_kcps, DEPZ_VL53LX_CAP_XTALK, "xtalk", get_xtalk, int32_t)
SETTER(set_signal_threshold_kcps, DEPZ_VL53LX_CAP_SIGNAL_THRESH, "signal_thresh", set_signal_threshold, int)
GETTER(get_signal_threshold_kcps, DEPZ_VL53LX_CAP_SIGNAL_THRESH, "signal_thresh", get_signal_threshold, int)
SETTER(set_sigma_threshold_mm, DEPZ_VL53LX_CAP_SIGMA_THRESH, "sigma_thresh", set_sigma_threshold, int)
GETTER(get_sigma_threshold_mm, DEPZ_VL53LX_CAP_SIGMA_THRESH, "sigma_thresh", get_sigma_threshold, int)
SETTER(set_roi_center, DEPZ_VL53LX_CAP_ROI, "roi", set_roi_center, int)
GETTER(get_roi_center, DEPZ_VL53LX_CAP_ROI, "roi", get_roi_center, int)

static int do_calib_offset(depz_device *dev, int target_mm, int nb, int32_t *out)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_CALIB_OFFSET, "calib_offset"));
    return st(dev)->drv->ops->calibrate_offset(st(dev)->drv, target_mm, nb, out);
}

int depz_vl53lx_calibrate_offset(depz_device *dev, int target_mm, int nb_samples, int32_t *offset_mm)
{
    int32_t dummy;
    if (!offset_mm) offset_mm = &dummy;
    LOCKED(do_calib_offset(dev, target_mm, nb_samples, offset_mm));
}

static int do_calib_xtalk(depz_device *dev, int target_mm, int nb, int32_t *out)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_CALIB_XTALK, "calib_xtalk"));
    return st(dev)->drv->ops->calibrate_xtalk(st(dev)->drv, target_mm, nb, out);
}

int depz_vl53lx_calibrate_xtalk(depz_device *dev, int target_mm, int nb_samples, int32_t *xtalk_kcps)
{
    int32_t dummy;
    if (!xtalk_kcps) xtalk_kcps = &dummy;
    LOCKED(do_calib_xtalk(dev, target_mm, nb_samples, xtalk_kcps));
}

static int do_get_thresholds(depz_device *dev, int *low, int *high, int *window)
{
    int l = 0, h = 0, w = 0;
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_THRESHOLDS, "thresholds"));
    VLX_TRY(st(dev)->drv->ops->get_detection_thresholds(st(dev)->drv, &l, &h, &w));
    if (low) *low = l;
    if (high) *high = h;
    if (window) *window = w;
    return DEPZ_OK;
}

int depz_vl53lx_get_detection_thresholds(depz_device *dev, int *low_mm, int *high_mm, int *window)
{
    LOCKED(do_get_thresholds(dev, low_mm, high_mm, window));
}

static int do_set_thresholds(depz_device *dev, int low, int high, int window)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_THRESHOLDS, "thresholds"));
    return st(dev)->drv->ops->set_detection_thresholds(st(dev)->drv, low, high, window);
}

int depz_vl53lx_set_detection_thresholds(depz_device *dev, int low_mm, int high_mm, int window)
{
    LOCKED(do_set_thresholds(dev, low_mm, high_mm, window));
}

static int do_get_roi(depz_device *dev, int *x, int *y)
{
    int a = 0, b = 0;
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_ROI, "roi"));
    VLX_TRY(st(dev)->drv->ops->get_roi(st(dev)->drv, &a, &b));
    if (x) *x = a;
    if (y) *y = b;
    return DEPZ_OK;
}

int depz_vl53lx_get_roi(depz_device *dev, int *x, int *y) { LOCKED(do_get_roi(dev, x, y)); }

static int do_set_roi(depz_device *dev, int x, int y)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_ROI, "roi"));
    return st(dev)->drv->ops->set_roi(st(dev)->drv, x, y);
}

int depz_vl53lx_set_roi(depz_device *dev, int x, int y) { LOCKED(do_set_roi(dev, x, y)); }

static int do_temp_update(depz_device *dev)
{
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_TEMP_UPDATE, "temp_update"));
    return st(dev)->drv->ops->start_temperature_update(st(dev)->drv);
}

int depz_vl53lx_start_temperature_update(depz_device *dev) { LOCKED(do_temp_update(dev)); }

static int do_refspad(depz_device *dev, uint32_t *count, bool *aperture)
{
    uint32_t c = 0;
    bool a = false;
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_cap(dev, DEPZ_VL53LX_CAP_REFSPAD, "refspad"));
    VLX_TRY(st(dev)->drv->ops->perform_ref_spad_management(st(dev)->drv, &c, &a));
    if (count) *count = c;
    if (aperture) *aperture = a;
    return DEPZ_OK;
}

int depz_vl53lx_perform_ref_spad_management(depz_device *dev, uint32_t *count, bool *is_aperture)
{
    LOCKED(do_refspad(dev, count, is_aperture));
}

/* ── ranging ───────────────────────────────────────────────────────────── */

static int do_start(depz_device *dev)
{
    vlx_driver *d;
    uint8_t p[DEPZ_VL53LX_START_STREAM_MAX];
    uint16_t addr, len;
    size_t n;
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_driver(dev));
    d = st(dev)->drv;
    VLX_TRY(d->ops->start_ranging(d));
    d->ops->stream_block(d, &addr, &len);
    n = depz_vl53lx_pack_start_stream(addr, len, 0, d->ops->clear, d->ops->n_clear, p);
    VLX_TRY(depz_device_request(dev, DEPZ_VL53LX_CMD_START_STREAM, p, n, NULL, NULL, true, -1));
    set_ranging(dev, true);
    return DEPZ_OK;
}

int depz_vl53lx_start_ranging(depz_device *dev) { LOCKED(do_start(dev)); }

static int do_stop(depz_device *dev)
{
    int rc, rc2;
    if (!is_ranging(dev)) return DEPZ_OK;
    /* Stop the sensor and clear the state even when the STOP_STREAM ack
     * fails, or the device is wedged "ranging". */
    rc = depz_device_request(dev, DEPZ_VL53LX_CMD_STOP_STREAM, NULL, 0, NULL, NULL, true, -1);
    set_ranging(dev, false);
    rc2 = st(dev)->drv->ops->stop_ranging(st(dev)->drv);
    return rc ? rc : rc2;
}

int depz_vl53lx_stop_ranging(depz_device *dev) { LOCKED(do_stop(dev)); }

static int do_measure_once(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out)
{
    vlx_driver *d;
    int rc, rc2;
    VLX_TRY(need_not_ranging(dev));
    VLX_TRY(need_driver(dev));
    d = st(dev)->drv;
    VLX_TRY(d->ops->start_ranging(d));
    vlx_measurement_init(out);
    rc = vlx_wait_data_ready(d, timeout_ms < 0 ? 1000 : timeout_ms);
    if (!rc) rc = d->ops->decode(d, NULL, 0, out);
    if (!rc) rc = d->ops->clear_interrupt(d);
    rc2 = d->ops->stop_ranging(d);
    if (rc) return rc;
    VLX_TRY(rc2);
    out->timestamp_us = vlx_last_timestamp_us(d->p);
    return DEPZ_OK;
}

int depz_vl53lx_measure_once(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL output");
    LOCKED(do_measure_once(dev, timeout_ms, out));
}

int depz_vl53lx_on_measurement(depz_device *dev, depz_vl53lx_measurement_cb cb, void *user, int *token)
{
    VLX_TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "vl53lx: NULL callback");
    return depz_cb_list_add(&st(dev)->cbs, (void (*)(void))cb, user, token);
}

void depz_vl53lx_off_measurement(depz_device *dev, int token)
{
    if (depz_is_vl53lx(dev)) depz_cb_list_remove(&st(dev)->cbs, token);
}

depz_stream *depz_vl53lx_measurements(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 64);
}

int depz_vl53lx_get_measurement(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out)
{
    depz_stream *s;
    int rc;
    VLX_TRY(need(dev));
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "vl53lx: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 2000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

uint64_t depz_vl53lx_stream_parse_errors(const depz_device *dev)
{
    vlx_state *s;
    uint64_t n;
    if (!depz_is_vl53lx(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->parse_errors;
    depz_mutex_unlock(&s->lock);
    return n;
}

int depz_vl53lx_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    LOCKED(vlx_rd_multi(&st(dev)->plat, addr, buf, len));
}

int depz_vl53lx_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len)
{
    LOCKED(need_not_ranging(dev) ? depz_fail(DEPZ_E_ARG, "vl53lx: stop ranging first") : vlx_wr_multi(&st(dev)->plat, addr, data, len));
}
