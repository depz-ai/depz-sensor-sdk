/* sr04_device.c — the SR04 sensor class (contract 03) on the device core. */
#include "io_internal.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    depz_cb_list cbs;
    depz_hub    *hub;
} sr04_state;

typedef void (*sr04_fn)(const depz_sr04_measurement *, void *);

static void from_data(const depz_sr04_data *d, depz_sr04_measurement *m)
{
    m->timestamp_us = d->timestamp_us;
    m->echo_time_us = d->echo_time_us;
    m->from_loop = d->source_cmd == DEPZ_SR04_START_MEASUREMENT_LOOP;
}

static bool sr04_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    sr04_state *s = (sr04_state *)dev->sensor;
    depz_sr04_data d;
    depz_sr04_measurement m;
    depz_cb_entry *cbs;
    size_t i, n;
    if (cmd != DEPZ_SR04_RPT_DATA || depz_sr04_unpack_data(p, len, &d) != 0) return false;
    from_data(&d, &m);
    n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) ((sr04_fn)cbs[i].fn)(&m, cbs[i].user);
    free(cbs);
    depz_hub_push(s->hub, &m);
    return true;
}

static void sr04_closed(depz_device *dev)
{
    depz_hub_mark_closed(((sr04_state *)dev->sensor)->hub);
}

static void sr04_destroy(depz_device *dev)
{
    sr04_state *s = (sr04_state *)dev->sensor;
    if (!s) return;
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops sr04_ops = {DEPZ_SENSOR_SR04, sr04_report, sr04_closed, sr04_destroy};

static sr04_state *sr04_new(void)
{
    sr04_state *s = (sr04_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_cb_list_init(&s->cbs)) { free(s); return NULL; }
    s->hub = depz_hub_new(sizeof(depz_sr04_measurement));
    if (!s->hub) { depz_cb_list_free(&s->cbs); free(s); return NULL; }
    return s;
}

int depz_sr04_attach(depz_device *dev)
{
    sr04_state *s;
    if (dev->ops == &sr04_ops) return DEPZ_OK;
    s = sr04_new();
    if (!s) return depz_fail(DEPZ_E_NOMEM, "sr04: out of memory");
    depz_device_attach(dev, &sr04_ops, s);
    return DEPZ_OK;
}

int depz_sr04_open_link(depz_link *link, depz_device **out)
{
    sr04_state *s = sr04_new();
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "sr04: out of memory");
    }
    return depz_device_create(link, &sr04_ops, s, out);
}

bool depz_is_sr04(const depz_device *dev)
{
    bool yes;
    if (!dev) return false;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    yes = dev->ops == &sr04_ops;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return yes;
}

static int need_sr04(const depz_device *dev)
{
    if (!dev) return depz_fail(DEPZ_E_ARG, "sr04: NULL device");
    if (!depz_is_sr04(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not an SR04", dev->port);
    return DEPZ_OK;
}

/* ── measurements ─────────────────────────────────────────────────────── */

bool depz_sr04_valid(const depz_sr04_measurement *m)
{
    return m && m->echo_time_us != DEPZ_SR04_ECHO_TIMEOUT;
}

bool depz_sr04_measurement_distance_mm(const depz_sr04_measurement *m, double *mm)
{
    return m && depz_sr04_distance_mm(m->echo_time_us, 0.0, false, mm);
}

bool depz_sr04_measurement_distance_mm_at(const depz_sr04_measurement *m, double air_temp_c,
                                          double *mm)
{
    return m && depz_sr04_distance_mm(m->echo_time_us, air_temp_c, true, mm);
}

/* ── configuration ────────────────────────────────────────────────────── */

typedef struct { uint8_t rpt; uint8_t buf[8]; size_t len; } rpt_ctx;

static bool match_rpt(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    rpt_ctx *r = (rpt_ctx *)c;
    if (cmd != r->rpt) return false;
    r->len = len < sizeof r->buf ? len : sizeof r->buf;
    memcpy(r->buf, p, r->len);
    return true;
}

int depz_sr04_get_sample_period_us(depz_device *dev, uint32_t *out)
{
    rpt_ctx r;
    int rc = need_sr04(dev);
    if (rc) return rc;
    memset(&r, 0, sizeof r);
    r.rpt = DEPZ_SR04_RPT_SAMPLE_PERIOD;
    rc = depz_device_request(dev, DEPZ_SR04_GET_SAMPLE_PERIOD, NULL, 0, match_rpt, &r, false, -1);
    if (rc) return rc;
    if (depz_sr04_unpack_sample_period(r.buf, r.len, out))
        return depz_fail(DEPZ_E_PROTOCOL, "sr04: sample-period report of %zu bytes", r.len);
    return DEPZ_OK;
}

int depz_sr04_set_sample_period_us(depz_device *dev, uint32_t period_us)
{
    uint8_t b[4];
    int rc = need_sr04(dev);
    if (rc) return rc;
    depz_sr04_pack_sample_period(period_us, b);
    return depz_device_request(dev, DEPZ_SR04_SET_SAMPLE_PERIOD, b, 4, NULL, NULL, true, -1);
}

int depz_sr04_get_echo_decay_us(depz_device *dev, uint16_t *out)
{
    rpt_ctx r;
    int rc = need_sr04(dev);
    if (rc) return rc;
    memset(&r, 0, sizeof r);
    r.rpt = DEPZ_SR04_RPT_ECHO_DECAY;
    rc = depz_device_request(dev, DEPZ_SR04_GET_ECHO_DECAY, NULL, 0, match_rpt, &r, false, -1);
    if (rc) return rc;
    if (depz_sr04_unpack_echo_decay(r.buf, r.len, out))
        return depz_fail(DEPZ_E_PROTOCOL, "sr04: echo-decay report of %zu bytes", r.len);
    return DEPZ_OK;
}

int depz_sr04_set_echo_decay_us(depz_device *dev, uint32_t decay_us, uint16_t *effective)
{
    uint8_t b[2];
    uint16_t eff;
    int rc = need_sr04(dev);
    if (rc) return rc;
    /* Inside the u16 field but outside 4000..65000 is legal — the device
     * clamps, which is why the value is re-read. Beyond the field is a bug. */
    if (decay_us > 0xFFFFu)
        return depz_fail(DEPZ_E_ARG,
                         "echo decay must be 0..65535 us (u16 wire field; the device then clamps "
                         "to 4000..65000, contract 03 §3): got %lu",
                         (unsigned long)decay_us);
    depz_sr04_pack_echo_decay((uint16_t)decay_us, b);
    rc = depz_device_request(dev, DEPZ_SR04_SET_ECHO_DECAY, b, 2, NULL, NULL, true, -1);
    if (rc) return rc;
    rc = depz_sr04_get_echo_decay_us(dev, &eff);
    if (rc == DEPZ_OK && effective) *effective = eff;
    return rc;
}

/* ── measuring ────────────────────────────────────────────────────────── */

typedef struct { depz_sr04_measurement m; } once_ctx;

static bool match_once(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    depz_sr04_data d;
    if (cmd != DEPZ_SR04_RPT_DATA || depz_sr04_unpack_data(p, len, &d) != 0) return false;
    if (d.source_cmd != DEPZ_SR04_MEASURE_ONCE) return false;
    from_data(&d, &((once_ctx *)c)->m);
    return true;
}

int depz_sr04_measure_once(depz_device *dev, int timeout_ms, depz_sr04_measurement *out)
{
    once_ctx c;
    int rc = need_sr04(dev);
    if (rc) return rc;
    memset(&c, 0, sizeof c);
    /* The reply is the measurement itself, at echo end (or the ~65.5 ms
     * no-echo timeout): the 200 ms default is too tight. */
    rc = depz_device_request(dev, DEPZ_SR04_MEASURE_ONCE, NULL, 0, match_once, &c, false,
                             timeout_ms < 0 ? 1000 : timeout_ms);
    if (rc == DEPZ_OK && out) *out = c.m;
    return rc;
}

int depz_sr04_start(depz_device *dev)
{
    int rc = need_sr04(dev);
    if (rc) return rc;
    return depz_device_request(dev, DEPZ_SR04_START_MEASUREMENT_LOOP, NULL, 0, NULL, NULL, true, -1);
}

int depz_sr04_stop(depz_device *dev)
{
    int rc = need_sr04(dev);
    if (rc) return rc;
    return depz_device_request(dev, DEPZ_SR04_STOP_MEASUREMENT_LOOP, NULL, 0, NULL, NULL, true, -1);
}

int depz_sr04_on_measurement(depz_device *dev, depz_sr04_measurement_cb cb, void *user, int *token)
{
    int rc = need_sr04(dev);
    if (rc) return rc;
    if (!cb) return depz_fail(DEPZ_E_ARG, "sr04: NULL callback");
    return depz_cb_list_add(&((sr04_state *)dev->sensor)->cbs, (void (*)(void))cb, user, token);
}

void depz_sr04_off_measurement(depz_device *dev, int token)
{
    if (depz_is_sr04(dev)) depz_cb_list_remove(&((sr04_state *)dev->sensor)->cbs, token);
}

depz_stream *depz_sr04_stream(depz_device *dev, size_t maxsize)
{
    if (need_sr04(dev)) return NULL;
    return depz_hub_subscribe(((sr04_state *)dev->sensor)->hub, maxsize ? maxsize : 256);
}
