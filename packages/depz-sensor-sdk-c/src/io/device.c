/* device.c — the device core (contract 02 / 07): reader thread, request
 * correlation by echoed opcode, events, the common commands, time sync. */
#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct depz_pending {
    uint8_t       cmd;
    depz_matcher  matcher;
    void         *ctx;
    bool          ok_completes;
    bool          done;
    int           result;   /* DEPZ_OK / DEPZ_E_STATUS / DEPZ_E_BUSY / DEPZ_E_CLOSED */
    uint8_t       status;
    depz_pending *next;
};

/* ── lifecycle ────────────────────────────────────────────────────────── */

static void reader_main(void *arg);

static void set_closing(depz_device *dev)
{
    depz_mutex_lock(&dev->lock);
    dev->closing = true;
    depz_mutex_unlock(&dev->lock);
}

static bool is_closing(depz_device *dev)
{
    bool c;
    depz_mutex_lock(&dev->lock);
    c = dev->closing;
    depz_mutex_unlock(&dev->lock);
    return c;
}

int depz_device_create(depz_link *link, const depz_sensor_ops *ops, void *sensor,
                       depz_device **out)
{
    depz_device *dev;
    if (!link || !out) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_ARG, "device: NULL argument");
    }
    *out = NULL;
    dev = (depz_device *)calloc(1, sizeof *dev);
    if (!dev) goto nomem;
    dev->link = link;
    depz_strlcpy(dev->port, depz_link_name(link), sizeof dev->port);
    dev->timeout_ms = DEPZ_DEFAULT_TIMEOUT_MS;
    dev->tx_crc = DEPZ_CRC_NONE;
    dev->last_rx_seq = -1;
    dev->sensor_type = DEPZ_SENSOR_NONE;
    if (depz_mutex_init(&dev->lock) || depz_mutex_init(&dev->tx_lock) || depz_cond_init(&dev->cond) ||
        depz_cb_list_init(&dev->event_cbs))
        goto nomem; /* init failures here mean resource exhaustion */
    dev->event_hub = depz_hub_new(sizeof(depz_device_event));
    if (!dev->event_hub) goto nomem;
    depz_parser_init(&dev->parser);
    /* The sensor state exists before the reader can dispatch into it. */
    dev->ops = ops;
    dev->sensor = sensor;
    if (ops) dev->sensor_type = ops->type;
    dev->reader_running = true;
    if (depz_thread_start(&dev->reader, reader_main, dev)) {
        dev->reader_running = false;
        depz_device_close(dev);
        return depz_fail(DEPZ_E_NOMEM, "device: cannot start the reader thread");
    }
    *out = dev;
    return DEPZ_OK;

nomem:
    if (dev) {
        dev->link = NULL;
        free(dev);
    }
    if (ops && ops->destroy) {
        depz_device tmp;
        memset(&tmp, 0, sizeof tmp);
        tmp.sensor = sensor;
        ops->destroy(&tmp);
    }
    depz_link_free(link);
    return depz_fail(DEPZ_E_NOMEM, "device: out of memory");
}

int depz_device_open_link(depz_link *link, depz_device **out)
{
    return depz_device_create(link, NULL, NULL, out);
}

int depz_device_open(const char *port, depz_device **out)
{
    depz_link *link;
    int rc;
    if (!port || !out) return depz_fail(DEPZ_E_ARG, "device: NULL argument");
    rc = depz_serial_link_open(port, &link);
    if (rc) return rc;
    return depz_device_create(link, NULL, NULL, out);
}

void depz_device_attach(depz_device *dev, const depz_sensor_ops *ops, void *sensor)
{
    depz_mutex_lock(&dev->lock);
    dev->ops = ops;
    dev->sensor = sensor;
    dev->sensor_type = ops ? ops->type : dev->sensor_type;
    depz_mutex_unlock(&dev->lock);
}

void depz_device_close(depz_device *dev)
{
    if (!dev) return;
    set_closing(dev);
    depz_link_close(dev->link);
    if (dev->reader_running) {
        /* Never from a callback: the reader cannot join itself. */
        if (!depz_thread_is_current(dev->reader)) depz_thread_join(dev->reader);
        dev->reader_running = false;
    }
    if (dev->ops && dev->ops->destroy) dev->ops->destroy(dev);
    depz_hub_close(dev->event_hub);
    depz_cb_list_free(&dev->event_cbs);
    depz_parser_free(&dev->parser);
    depz_link_free(dev->link);
    depz_cond_destroy(&dev->cond);
    depz_mutex_destroy(&dev->tx_lock);
    depz_mutex_destroy(&dev->lock);
    free(dev);
}

void depz_device_set_timeout_ms(depz_device *dev, int timeout_ms)
{
    if (dev) dev->timeout_ms = timeout_ms > 0 ? timeout_ms : DEPZ_DEFAULT_TIMEOUT_MS;
}

depz_sensor_type depz_device_sensor_type(const depz_device *dev)
{
    depz_sensor_type t;
    if (!dev) return DEPZ_SENSOR_NONE;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    t = dev->sensor_type;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return t;
}

const char *depz_device_port(const depz_device *dev) { return dev ? dev->port : ""; }

bool depz_device_closed(const depz_device *dev)
{
    return !dev || depz_link_closed(dev->link);
}

void depz_device_stats(const depz_device *dev, depz_link_stats *out)
{
    depz_mutex_lock((depz_mutex *)&dev->lock);
    *out = dev->stats;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
}

/* ── events ───────────────────────────────────────────────────────────── */

typedef void (*event_fn)(const depz_device_event *, void *);

void depz_device_emit(depz_device *dev, const depz_device_event *ev)
{
    depz_cb_entry *cbs;
    size_t i, n = depz_cb_list_snapshot(&dev->event_cbs, &cbs);
    for (i = 0; i < n; i++) ((event_fn)cbs[i].fn)(ev, cbs[i].user);
    free(cbs);
    depz_hub_push(dev->event_hub, ev);
}

int depz_device_on_event(depz_device *dev, depz_device_event_cb cb, void *user, int *token)
{
    if (!dev || !cb) return depz_fail(DEPZ_E_ARG, "on_event: NULL argument");
    return depz_cb_list_add(&dev->event_cbs, (void (*)(void))cb, user, token);
}

void depz_device_off_event(depz_device *dev, int token)
{
    if (dev) depz_cb_list_remove(&dev->event_cbs, token);
}

depz_stream *depz_device_events(depz_device *dev, size_t maxsize)
{
    return dev ? depz_hub_subscribe(dev->event_hub, maxsize ? maxsize : 256) : NULL;
}

static void emit_simple(depz_device *dev, depz_device_event_type t, depz_device_event *ev)
{
    ev->type = t;
    depz_device_emit(dev, ev);
}

/* ── reader ───────────────────────────────────────────────────────────── */

static void fail_all_pending(depz_device *dev, int err)
{
    depz_pending *p;
    depz_mutex_lock(&dev->lock);
    for (p = dev->pending; p; p = p->next) {
        p->done = true;
        p->result = err;
    }
    dev->pending = NULL;
    depz_cond_broadcast(&dev->cond);
    depz_mutex_unlock(&dev->lock);
}

static void dispatch(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    depz_device_event ev;
    const depz_sensor_ops *ops;

    if (cmd == DEPZ_RPT_STATUS && len >= 2) {
        uint8_t rcmd = p[0], st = p[1];
        if (rcmd != 0x00) { /* 0x00 = unsolicited */
            depz_pending **pp, *pend = NULL;
            depz_mutex_lock(&dev->lock);
            for (pp = &dev->pending; *pp; pp = &(*pp)->next)
                if ((*pp)->cmd == rcmd) { pend = *pp; break; }
            if (pend) {
                bool finish = true;
                if (st == DEPZ_STATUS_ERR_BUSY) pend->result = DEPZ_E_BUSY;
                else if (st != DEPZ_STATUS_OK) pend->result = DEPZ_E_STATUS;
                else if (pend->ok_completes) pend->result = DEPZ_OK;
                else finish = false; /* an OK ack while its data report is due */
                if (finish) {
                    pend->status = st;
                    pend->done = true;
                    *pp = pend->next;
                    depz_cond_broadcast(&dev->cond);
                }
            }
            depz_mutex_unlock(&dev->lock);
            if (pend) return;
        }
        memset(&ev, 0, sizeof ev);
        ev.status = st;
        emit_simple(dev, DEPZ_DEV_EV_UNSOLICITED_STATUS, &ev);
        return;
    }

    /* Pending matchers get the first shot at any other packet. */
    depz_mutex_lock(&dev->lock);
    {
        depz_pending **pp;
        for (pp = &dev->pending; *pp; pp = &(*pp)->next) {
            depz_pending *pend = *pp;
            if (pend->matcher && pend->matcher(cmd, p, len, pend->ctx)) {
                pend->result = DEPZ_OK;
                pend->done = true;
                *pp = pend->next;
                depz_cond_broadcast(&dev->cond);
                depz_mutex_unlock(&dev->lock);
                return;
            }
        }
    }
    ops = dev->ops;
    depz_mutex_unlock(&dev->lock);

    memset(&ev, 0, sizeof ev);
    if (cmd == DEPZ_RPT_SEQUENCE_ERROR && len == 2) {
        depz_mutex_lock(&dev->lock);
        dev->stats.device_seq_errors++;
        depz_mutex_unlock(&dev->lock);
        ev.expected_seq = p[0];
        ev.received_seq = p[1];
        ev.by_device = true;
        emit_simple(dev, DEPZ_DEV_EV_SEQUENCE_ERROR, &ev);
        return;
    }
    if (cmd == DEPZ_RPT_TEXT && len) {
        ev.cmd = p[0];
        depz_strip_device_string(p + 1, len - 1, ev.text, sizeof ev.text);
        emit_simple(dev, DEPZ_DEV_EV_TEXT, &ev);
        return;
    }
    if (cmd == DEPZ_RPT_TEMPERATURE && len == 10) {
        depz_temperature_report t;
        depz_unpack_temperature(p, len, &t);
        ev.timestamp_us = t.timestamp_us;
        ev.celsius = t.raw_decidegrees / 10.0;
        emit_simple(dev, DEPZ_DEV_EV_TEMPERATURE, &ev);
        return;
    }
    if (ops && ops->handle_report && ops->handle_report(dev, cmd, p, len)) return;
    /* Unrouted report: keep it visible so new firmware stays debuggable. */
    {
        size_t i, o = 0;
        ev.cmd = cmd;
        for (i = 0; i < len && o + 3 < sizeof ev.text; i++)
            o += (size_t)snprintf(ev.text + o, sizeof ev.text - o, "%02x", p[i]);
        emit_simple(dev, DEPZ_DEV_EV_TEXT, &ev);
    }
}

static void on_parser_event(const depz_event *e, void *user)
{
    depz_device *dev = (depz_device *)user;
    depz_device_event ev;
    memset(&ev, 0, sizeof ev);
    switch (e->type) {
    case DEPZ_EV_PACKET:
        depz_mutex_lock(&dev->lock);
        dev->stats.rx_packets++;
        if (dev->last_rx_seq >= 0 && e->seq != ((dev->last_rx_seq + 1) & 0xFF))
            dev->stats.seq_gaps++;
        dev->last_rx_seq = e->seq;
        depz_mutex_unlock(&dev->lock);
        dispatch(dev, e->cmd, e->payload, e->payload_len);
        break;
    case DEPZ_EV_CRC_ERROR:
        depz_mutex_lock(&dev->lock);
        dev->stats.crc_errors++;
        depz_mutex_unlock(&dev->lock);
        ev.cmd = e->cmd;
        ev.seq = e->seq;
        emit_simple(dev, DEPZ_DEV_EV_CRC_ERROR, &ev);
        break;
    case DEPZ_EV_TRASH:
        depz_mutex_lock(&dev->lock);
        dev->stats.trash_bytes += e->trash_len;
        depz_mutex_unlock(&dev->lock);
        ev.trash_len = e->trash_len;
        ev.data_len = e->trash_len < sizeof ev.data ? e->trash_len : sizeof ev.data;
        memcpy(ev.data, e->trash, ev.data_len);
        emit_simple(dev, DEPZ_DEV_EV_TRASH, &ev);
        break;
    }
}

static void reader_main(void *arg)
{
    depz_device *dev = (depz_device *)arg;
    uint8_t buf[4096];
    const depz_sensor_ops *ops;

    while (!is_closing(dev)) {
        size_t got = 0;
        int rc = depz_link_read(dev->link, buf, sizeof buf, 50, &got);
        if (rc != DEPZ_OK) {
            if (!is_closing(dev)) {
                depz_device_event ev;
                memset(&ev, 0, sizeof ev);
                depz_strlcpy(ev.text, depz_last_error(), sizeof ev.text);
                fail_all_pending(dev, DEPZ_E_CLOSED);
                depz_link_close(dev->link);
                emit_simple(dev, DEPZ_DEV_EV_DISCONNECTED, &ev);
            }
            break;
        }
        if (!got) continue;
        depz_mutex_lock(&dev->lock);
        dev->stats.rx_bytes += got;
        depz_mutex_unlock(&dev->lock);
        depz_parser_feed(&dev->parser, buf, got, on_parser_event, dev);
        depz_mutex_lock(&dev->lock);
        dev->stats.header_errors = dev->parser.header_errors;
        depz_mutex_unlock(&dev->lock);
    }
    fail_all_pending(dev, DEPZ_E_CLOSED);
    depz_mutex_lock(&dev->lock);
    ops = dev->ops;
    dev->reader_done = true;
    depz_mutex_unlock(&dev->lock);
    if (ops && ops->on_closed) ops->on_closed(dev);
    depz_hub_mark_closed(dev->event_hub);
}

/* ── TX / requests ────────────────────────────────────────────────────── */

int depz_device_send(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t small[512], *frame = small;
    size_t cap = DEPZ_HEADER_SIZE + len + 4, n = 0;
    int rc;
    if (!dev) return depz_fail(DEPZ_E_ARG, "send: NULL device");
    if (cap > sizeof small) {
        frame = (uint8_t *)malloc(cap);
        if (!frame) return depz_fail(DEPZ_E_NOMEM, "send: out of memory");
    }
    depz_mutex_lock(&dev->tx_lock);
    rc = depz_build_packet(cmd, payload, len, dev->tx_seq, dev->tx_crc, frame, cap, &n);
    if (rc) {
        rc = depz_fail(DEPZ_E_ARG, "send: payload of %zu bytes does not fit a packet", len);
    } else {
        rc = depz_link_write(dev->link, frame, n);
        if (rc == DEPZ_OK) {
            dev->tx_seq = (uint8_t)(dev->tx_seq + 1);
            depz_mutex_lock(&dev->lock);
            dev->stats.tx_packets++;
            dev->stats.tx_bytes += n;
            depz_mutex_unlock(&dev->lock);
        }
    }
    depz_mutex_unlock(&dev->tx_lock);
    if (frame != small) free(frame);
    return rc;
}

static void take_pending(depz_device *dev, depz_pending *pend)
{
    depz_pending **pp;
    for (pp = &dev->pending; *pp; pp = &(*pp)->next)
        if (*pp == pend) { *pp = pend->next; return; }
}

int depz_device_request(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len,
                        depz_matcher matcher, void *ctx, bool ok_completes, int timeout_ms)
{
    depz_pending pend, *q;
    uint64_t deadline;
    int rc;

    if (!dev) return depz_fail(DEPZ_E_ARG, "request: NULL device");
    if (!matcher && !ok_completes)
        return depz_fail(DEPZ_E_ARG, "request: configure a matcher or ok_completes");
    memset(&pend, 0, sizeof pend);
    pend.cmd = cmd;
    pend.matcher = matcher;
    pend.ctx = ctx;
    pend.ok_completes = ok_completes;
    if (timeout_ms < 0) timeout_ms = dev->timeout_ms;

    depz_mutex_lock(&dev->lock);
    if (dev->reader_done) {
        depz_mutex_unlock(&dev->lock);
        return depz_fail(DEPZ_E_CLOSED, "%s: device closed", dev->port);
    }
    for (q = dev->pending; q; q = q->next) {
        if (q->cmd == cmd) {
            depz_mutex_unlock(&dev->lock);
            return depz_fail_status(DEPZ_E_BUSY, cmd, DEPZ_STATUS_ERR_BUSY);
        }
    }
    pend.next = dev->pending;
    dev->pending = &pend;
    depz_mutex_unlock(&dev->lock);

    rc = depz_device_send(dev, cmd, payload, len);
    deadline = depz_now_us() + (uint64_t)timeout_ms * 1000u;
    depz_mutex_lock(&dev->lock);
    if (rc != DEPZ_OK) {
        if (!pend.done) take_pending(dev, &pend);
        depz_mutex_unlock(&dev->lock);
        return rc;
    }
    while (!pend.done) {
        int left = depz_ms_until(deadline);
        if (!left) break;
        depz_cond_wait_ms(&dev->cond, &dev->lock, left);
    }
    if (!pend.done) take_pending(dev, &pend);
    depz_mutex_unlock(&dev->lock);

    if (!pend.done)
        return depz_fail(DEPZ_E_TIMEOUT, "no reply to cmd 0x%02X within %d ms", cmd, timeout_ms);
    if (pend.result == DEPZ_E_STATUS || pend.result == DEPZ_E_BUSY)
        return depz_fail_status(pend.result, cmd, pend.status);
    if (pend.result == DEPZ_E_CLOSED)
        return depz_fail(DEPZ_E_CLOSED, "%s: device closed", dev->port);
    return DEPZ_OK;
}

/* ── common commands ──────────────────────────────────────────────────── */

typedef struct { uint8_t want; char *out; size_t cap; } text_ctx;

static bool match_text(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    text_ctx *t = (text_ctx *)c;
    if (cmd != DEPZ_RPT_TEXT || !len || p[0] != t->want) return false;
    depz_strip_device_string(p + 1, len - 1, t->out, t->cap);
    return true;
}

static int get_text(depz_device *dev, uint8_t cmd, char *out, size_t cap)
{
    text_ctx t;
    if (!out || !cap) return depz_fail(DEPZ_E_ARG, "NULL output buffer");
    t.want = cmd;
    t.out = out;
    t.cap = cap;
    out[0] = '\0';
    return depz_device_request(dev, cmd, NULL, 0, match_text, &t, false, -1);
}

int depz_device_get_device_name(depz_device *dev, char *out, size_t cap)
{
    return get_text(dev, DEPZ_CMD_GET_DEVICE_NAME, out, cap);
}

int depz_device_get_software_name(depz_device *dev, char *out, size_t cap)
{
    return get_text(dev, DEPZ_CMD_GET_NAME_ACTIVE_SOFTWARE, out, cap);
}

int depz_device_get_serial_number(depz_device *dev, char *out, size_t cap)
{
    return get_text(dev, DEPZ_CMD_GET_SERIAL, out, cap);
}

/* A typed report identified by its report id alone, copied raw. */
typedef struct { uint8_t rpt; size_t want_len; uint8_t buf[32]; size_t len; } raw_ctx;

static bool match_raw(uint8_t cmd, const uint8_t *p, size_t len, void *c)
{
    raw_ctx *r = (raw_ctx *)c;
    if (cmd != r->rpt) return false;
    r->len = len < sizeof r->buf ? len : sizeof r->buf;
    memcpy(r->buf, p, r->len);
    return true;
}

static int request_raw(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t plen,
                       uint8_t rpt, size_t want_len, raw_ctx *r)
{
    int rc;
    memset(r, 0, sizeof *r);
    r->rpt = rpt;
    rc = depz_device_request(dev, cmd, payload, plen, match_raw, r, false, -1);
    if (rc == DEPZ_OK && r->len != want_len)
        return depz_fail(DEPZ_E_PROTOCOL, "report 0x%02X: %zu bytes, expected %zu", rpt, r->len,
                         want_len);
    return rc;
}

int depz_device_read_mcu_temperature(depz_device *dev, double *celsius)
{
    raw_ctx r;
    depz_temperature_report t;
    int rc = request_raw(dev, DEPZ_CMD_GET_MCU_TEMPERATURE, NULL, 0, DEPZ_RPT_TEMPERATURE, 10, &r);
    if (rc) return rc;
    depz_unpack_temperature(r.buf, r.len, &t);
    if (celsius) *celsius = t.raw_decidegrees / 10.0;
    return DEPZ_OK;
}

int depz_device_get_payload_crc_type(depz_device *dev, depz_crc_type *out)
{
    raw_ctx r;
    int rc = request_raw(dev, DEPZ_CMD_GET_PAYLOAD_CRC_TYPE, NULL, 0, DEPZ_RPT_PAYLOAD_CRC_TYPE, 1, &r);
    if (rc) return rc;
    if (out) *out = (depz_crc_type)r.buf[0];
    return DEPZ_OK;
}

int depz_device_set_payload_crc_type(depz_device *dev, depz_crc_type t)
{
    uint8_t b = (uint8_t)t;
    return depz_device_request(dev, DEPZ_CMD_SET_PAYLOAD_CRC_TYPE, &b, 1, NULL, NULL, true, -1);
}

int depz_device_get_sync_pin(depz_device *dev, uint8_t pin, depz_sync_pin_config *out)
{
    raw_ctx r;
    int rc = request_raw(dev, DEPZ_CMD_GET_SYNC_PIN_CONFIG, &pin, 1, DEPZ_RPT_SYNC_PIN_CONFIG, 3, &r);
    if (rc) return rc;
    if (out) depz_unpack_sync_pin_config(r.buf, r.len, out);
    return DEPZ_OK;
}

int depz_device_set_sync_pin(depz_device *dev, const depz_sync_pin_config *c)
{
    uint8_t b[3];
    if (!c) return depz_fail(DEPZ_E_ARG, "set_sync_pin: NULL config");
    depz_pack_sync_pin_config(c, b);
    return depz_device_request(dev, DEPZ_CMD_SET_SYNC_PIN_CONFIG, b, 3, NULL, NULL, true, -1);
}

int depz_device_reset(depz_device *dev)
{
    return depz_device_request(dev, DEPZ_CMD_DEVICE_RESET, NULL, 0, NULL, NULL, true, -1);
}

int depz_device_enter_bootloader(depz_device *dev)
{
    int rc = depz_device_request(dev, DEPZ_CMD_BOOTLOADER, NULL, 0, NULL, NULL, true, -1);
    if (rc == DEPZ_OK) {
        set_closing(dev);
        depz_link_close(dev->link);
    }
    return rc;
}

void depz_device_sleep_ms(depz_device *dev, int ms)
{
    if (!dev->link->is_replay) depz_sleep_ms(ms);
}

/* ── time sync ────────────────────────────────────────────────────────── */

uint64_t depz_host_now_us(void) { return depz_now_us(); }

int depz_device_sync_time(depz_device *dev, int samples, depz_time_sync *out)
{
    depz_time_sync best;
    bool have = false;
    int i;
    if (!dev) return depz_fail(DEPZ_E_ARG, "sync_time: NULL device");
    if (samples < 1) samples = 1;
    for (i = 0; i < samples; i++) {
        raw_ctx r;
        uint8_t payload[8];
        depz_sync_time_report rep;
        uint64_t t1 = depz_now_us(), t4;
        int64_t offset, rtt;
        int rc;
        depz_pack_sync_time(t1, payload);
        rc = request_raw(dev, DEPZ_CMD_SYNC_TIME, payload, 8, DEPZ_RPT_SYNC_TIME, 24, &r);
        if (rc) return rc;
        t4 = depz_now_us();
        depz_unpack_sync_time(r.buf, r.len, &rep);
        depz_sync_time_offset_rtt((int64_t)t1, (int64_t)rep.mcu_rx_us, (int64_t)rep.mcu_tx_us,
                                  (int64_t)t4, &offset, &rtt);
        if (!have || rtt < best.rtt_us) {
            best.offset_us = offset;
            best.rtt_us = rtt;
            best.synced_at_host_us = t4;
            have = true;
        }
    }
    depz_mutex_lock(&dev->lock);
    dev->sync = best;
    dev->synced = true;
    depz_mutex_unlock(&dev->lock);
    if (out) *out = best;
    return DEPZ_OK;
}

bool depz_device_time_sync(const depz_device *dev, depz_time_sync *out)
{
    bool have;
    depz_mutex_lock((depz_mutex *)&dev->lock);
    have = dev->synced;
    if (have && out) *out = dev->sync;
    depz_mutex_unlock((depz_mutex *)&dev->lock);
    return have;
}

int depz_device_to_host_time_us(const depz_device *dev, uint64_t device_us, int64_t *out)
{
    depz_time_sync s;
    if (!depz_device_time_sync(dev, &s))
        return depz_fail(DEPZ_E_ARG, "call depz_device_sync_time() first");
    *out = (int64_t)device_us - s.offset_us;
    return DEPZ_OK;
}
