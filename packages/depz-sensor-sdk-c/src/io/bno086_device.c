/* bno086_device.c — the BNO085 / BNO086 class (contract 05): SHTP framing and
 * the SH-2 sensor-hub protocol over the bridge's pass-through, as the Python
 * SDK's bno086/__init__.py does it — same requests in the same order, so a
 * capture made by either SDK replays in the other byte for byte.
 *
 * The bridge acknowledges SEND_SHTP_PACKET with RPT_STATUS at once and hands
 * every inbound SHTP frame over as RPT_DATA(cmd 0) (ERRATA E2): answers are
 * matched on their SH-2 content only. Control-channel cargos go to the
 * waiters registered for their report id; input cargos become reports. */
#include "io_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define CMD_SENSOR_RESET     DEPZ_BNO086_CMD_SENSOR_RESET
#define CMD_SENSOR_WAKE_UP   DEPZ_BNO086_CMD_SENSOR_WAKE_UP
#define CMD_SEND_SHTP_PACKET DEPZ_BNO086_CMD_SEND_SHTP_PACKET

#define CONTROL_TIMEOUT_MS   1000
#define FRS_TIMEOUT_MS       2000
#define RESET_TIMEOUT_MS     2000
/* The executable reset-complete is waited for this long at most, best effort
 * (ERRATA E9: firmware <= v0.95 never sends it). */
#define RESET_COMPLETE_WAIT_MS 500
/* Re-reads allowed when enable()'s read-back sees a "disabled" answer: a
 * stale one left in flight by a preceding disable() (the hub answers every
 * state-changing Set Feature unsolicited, and Get Feature Response carries no
 * correlation token), or the hub answering before it applied the new rate —
 * the lab BNO085 does that for one or two read-backs right after Set Feature. */
#define VERIFY_STALE_RETRIES 5
#define EXECUTABLE_RESET_COMPLETE 0x01

/* Control answers are at most 17 bytes; a waiter keeps the first CTRL_MAX of
 * each and up to WAITER_SLOTS unread ones (the oldest go first). */
#define CTRL_MAX     32
#define WAITER_SLOTS 16
#define ADV_MAX      1024
#define UNKNOWN_MAX  256

typedef struct waiter {
    uint8_t rid;
    uint8_t msg[WAITER_SLOTS][CTRL_MAX];
    size_t  len[WAITER_SLOTS];
    unsigned head, count;
    struct waiter *next;
} waiter;

typedef struct {
    depz_mutex op;             /* serialises the public operations */
    depz_mutex lock;           /* everything below; `cond` signals it */
    depz_cond  cond;
    depz_shtp_layer shtp;
    waiter *waiters;
    bool reset_seen;
    uint8_t cmd_seq;
    uint8_t adv[ADV_MAX];
    size_t adv_len;
    depz_cb_list cbs;
    depz_hub *hub;
} bno_state;

static bno_state *st(depz_device *dev) { return (bno_state *)dev->sensor; }

#define TRY(expr) do { int rc_ = (expr); if (rc_) return rc_; } while (0)

static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ── RX path (reader thread) ──────────────────────────────────────────── */

typedef void (*report_fn)(const depz_bno_report *, void *);

static void emit(depz_device *dev, depz_bno_report *reps, size_t n)
{
    bno_state *s = st(dev);
    depz_cb_entry *cbs;
    size_t i, k, ncb;
    if (!n) return;
    ncb = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++) {
        for (k = 0; k < ncb; k++) ((report_fn)cbs[k].fn)(&reps[i], cbs[k].user);
        /* A streamed copy outlives the cargo it points into. */
        reps[i].data = NULL;
        depz_hub_push(s->hub, &reps[i]);
    }
    free(cbs);
}

/* Under `lock`: hand a control cargo to every waiter of its report id. */
static void deliver_control(bno_state *s, const uint8_t *p, size_t len)
{
    waiter *w;
    bool any = false;
    for (w = s->waiters; w; w = w->next) {
        unsigned slot;
        if (w->rid != p[0]) continue;
        if (w->count == WAITER_SLOTS) { /* full: drop the oldest */
            w->head = (w->head + 1) % WAITER_SLOTS;
            w->count--;
        }
        slot = (w->head + w->count) % WAITER_SLOTS;
        w->len[slot] = len < CTRL_MAX ? len : CTRL_MAX;
        memcpy(w->msg[slot], p, w->len[slot]);
        w->count++;
        any = true;
    }
    if (any) depz_cond_broadcast(&s->cond);
}

static bool bno_report(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len)
{
    bno_state *s = st(dev);
    depz_shtp_cargo cargo;
    depz_bno_report local[32], *reps = local;
    uint8_t unknown[UNKNOWN_MAX];
    const uint8_t *frame;
    size_t frame_len, n = 0;
    uint64_t capture;
    int got;
    if (cmd != DEPZ_BNO086_RPT_DATA || depz_bno086_unpack_data(p, len, &capture, &frame, &frame_len)) return false;
    depz_mutex_lock(&s->lock);
    got = depz_shtp_feed(&s->shtp, frame, frame_len, &cargo);
    if (got != 1) {
        depz_mutex_unlock(&s->lock);
        return true;
    }
    switch (cargo.channel) {
    case DEPZ_SHTP_CH_COMMAND: {
        size_t room = ADV_MAX - s->adv_len;
        size_t take = cargo.payload_len < room ? cargo.payload_len : room;
        memcpy(s->adv + s->adv_len, cargo.payload, take);
        s->adv_len += take;
        break;
    }
    case DEPZ_SHTP_CH_EXECUTABLE:
        if (cargo.payload_len && cargo.payload[0] == EXECUTABLE_RESET_COMPLETE) {
            s->reset_seen = true;
            depz_cond_broadcast(&s->cond);
        }
        break;
    case DEPZ_SHTP_CH_CONTROL:
        if (cargo.payload_len) deliver_control(s, cargo.payload, cargo.payload_len);
        break;
    case DEPZ_SHTP_CH_INPUT_NORMAL:
    case DEPZ_SHTP_CH_INPUT_WAKE: {
        /* The shortest report is 5 bytes; parse into a big enough array. */
        size_t cap = cargo.payload_len / 5 + 1;
        if (cap > sizeof local / sizeof local[0]) {
            reps = (depz_bno_report *)malloc(cap * sizeof *reps);
            if (!reps) break;
        } else {
            cap = sizeof local / sizeof local[0];
        }
        n = depz_bno_parse_input_cargo(cargo.payload, cargo.payload_len, capture, reps, cap);
        /* The cargo buffer is the layer's: copy an unknown tail out. */
        if (n && reps[n - 1].type == DEPZ_BNO_UNKNOWN_REPORT) {
            depz_bno_report *u = &reps[n - 1];
            if (u->data_len > UNKNOWN_MAX) u->data_len = UNKNOWN_MAX;
            memcpy(unknown, u->data, u->data_len);
            u->data = unknown;
        }
        break;
    }
    case DEPZ_SHTP_CH_GYRO_RV:
        if (depz_bno_parse_gyro_rv(cargo.payload, cargo.payload_len, capture, &local[0]) == 0) n = 1;
        break;
    default:
        break;
    }
    depz_mutex_unlock(&s->lock);
    emit(dev, reps, n);
    if (reps != local) free(reps);
    return true;
}

/* ── class plumbing ────────────────────────────────────────────────────── */

static void bno_closed(depz_device *dev)
{
    bno_state *s = st(dev);
    depz_hub_mark_closed(s->hub);
    depz_mutex_lock(&s->lock);
    depz_cond_broadcast(&s->cond);
    depz_mutex_unlock(&s->lock);
}

static void bno_destroy(depz_device *dev)
{
    bno_state *s = st(dev);
    if (!s) return;
    depz_hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_shtp_free(&s->shtp);
    depz_cond_destroy(&s->cond);
    depz_mutex_destroy(&s->lock);
    depz_mutex_destroy(&s->op);
    free(s);
    dev->sensor = NULL;
}

static const depz_sensor_ops bno_ops = {DEPZ_SENSOR_BNO086, bno_report, bno_closed, bno_destroy};

static bno_state *bno_new(void)
{
    bno_state *s = (bno_state *)calloc(1, sizeof *s);
    if (!s) return NULL;
    if (depz_mutex_init(&s->op)) goto fail0;
    if (depz_mutex_init(&s->lock)) goto fail1;
    if (depz_cond_init(&s->cond)) goto fail2;
    if (depz_cb_list_init(&s->cbs)) goto fail3;
    s->hub = depz_hub_new(sizeof(depz_bno_report));
    if (!s->hub) goto fail4;
    depz_shtp_init(&s->shtp);
    return s;
fail4:
    depz_cb_list_free(&s->cbs);
fail3:
    depz_cond_destroy(&s->cond);
fail2:
    depz_mutex_destroy(&s->lock);
fail1:
    depz_mutex_destroy(&s->op);
fail0:
    free(s);
    return NULL;
}

int depz_bno086_attach(depz_device *dev)
{
    bno_state *s;
    if (dev->ops == &bno_ops) return DEPZ_OK;
    s = bno_new();
    if (!s) return depz_fail(DEPZ_E_NOMEM, "bno086: out of memory");
    depz_device_attach(dev, &bno_ops, s);
    return DEPZ_OK;
}

int depz_bno086_open_link(depz_link *link, depz_device **out)
{
    bno_state *s = bno_new();
    if (!s) {
        depz_link_free(link);
        return depz_fail(DEPZ_E_NOMEM, "bno086: out of memory");
    }
    return depz_device_create(link, &bno_ops, s, out);
}

bool depz_is_bno086(const depz_device *dev)
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
    if (!dev) return depz_fail(DEPZ_E_ARG, "bno086: NULL device");
    if (!depz_is_bno086(dev)) return depz_fail(DEPZ_E_WRONG_TYPE, "%s is not a BNO085 / BNO086", dev->port);
    return DEPZ_OK;
}

/* Public entry: serialise the whole sequence. */
#define LOCKED(body)                          \
    do {                                      \
        int rc_;                              \
        TRY(need(dev));                       \
        depz_mutex_lock(&st(dev)->op);        \
        rc_ = (body);                         \
        depz_mutex_unlock(&st(dev)->op);      \
        return rc_;                           \
    } while (0)

static int timeout_at_least(depz_device *dev, int floor_ms)
{
    return dev->timeout_ms > floor_ms ? dev->timeout_ms : floor_ms;
}

/* ── TX path ───────────────────────────────────────────────────────────── */

/* Frame `payload` and push it through SEND_SHTP_PACKET; ERR_BUSY (both MCU
 * TX slots full) backs off and retransmits the same frame (contract 05 §2). */
static int send_shtp(depz_device *dev, uint8_t channel, const uint8_t *payload, size_t len)
{
    bno_state *s = st(dev);
    uint8_t frame[DEPZ_SHTP_MAX_TX_FRAME];
    size_t n;
    int attempt, rc = DEPZ_OK;
    depz_mutex_lock(&s->lock);
    n = depz_shtp_next_frame(&s->shtp, channel, payload, len, frame, sizeof frame);
    depz_mutex_unlock(&s->lock);
    if (!n) return depz_fail(DEPZ_E_ARG, "bno086: %zu-byte cargo on channel %u does not fit one %d-byte frame",
                             len, channel, DEPZ_SHTP_MAX_TX_FRAME);
    for (attempt = 0; attempt < DEPZ_BNO086_BUSY_RETRIES; attempt++) {
        rc = depz_device_request(dev, CMD_SEND_SHTP_PACKET, frame, n, NULL, NULL, true, -1);
        if (rc != DEPZ_E_BUSY) return rc;
        if (attempt < DEPZ_BNO086_BUSY_RETRIES - 1) depz_sleep_ms(DEPZ_BNO086_BUSY_BACKOFF_MS);
    }
    return rc;
}

static void waiter_add(bno_state *s, waiter *w, uint8_t rid)
{
    memset(w, 0, sizeof *w);
    w->rid = rid;
    depz_mutex_lock(&s->lock);
    w->next = s->waiters;
    s->waiters = w;
    depz_mutex_unlock(&s->lock);
}

static void waiter_remove(bno_state *s, waiter *w)
{
    waiter **pp;
    depz_mutex_lock(&s->lock);
    for (pp = &s->waiters; *pp; pp = &(*pp)->next) {
        if (*pp == w) {
            *pp = w->next;
            break;
        }
    }
    depz_mutex_unlock(&s->lock);
}

/* The next answer for `w` (copied into msg/len) before `deadline_us`. */
static int waiter_next(depz_device *dev, waiter *w, uint64_t deadline_us, uint8_t msg[CTRL_MAX], size_t *len)
{
    bno_state *s = st(dev);
    depz_mutex_lock(&s->lock);
    while (!w->count) {
        int left = depz_ms_until(deadline_us);
        if (depz_device_closed(dev)) {
            depz_mutex_unlock(&s->lock);
            return depz_fail(DEPZ_E_CLOSED, "bno086: device closed");
        }
        if (left <= 0) {
            depz_mutex_unlock(&s->lock);
            return depz_fail(DEPZ_E_TIMEOUT, "bno086: no SH-2 report 0x%02X within the timeout", w->rid);
        }
        depz_cond_wait_ms(&s->cond, &s->lock, left);
    }
    *len = w->len[w->head];
    memcpy(msg, w->msg[w->head], *len);
    w->head = (w->head + 1) % WAITER_SLOTS;
    w->count--;
    depz_mutex_unlock(&s->lock);
    return DEPZ_OK;
}

/* Send a control cargo and wait for the first answer with report id `rid`
 * that `accept` takes (NULL = any). */
typedef bool (*accept_fn)(const uint8_t *msg, size_t len, void *ctx);

static int control_request(depz_device *dev, const uint8_t *payload, size_t len, uint8_t rid,
                           accept_fn accept, void *ctx, int timeout_ms, uint8_t msg[CTRL_MAX], size_t *mlen)
{
    bno_state *s = st(dev);
    waiter w;
    uint64_t deadline;
    int rc;
    waiter_add(s, &w, rid);
    rc = send_shtp(dev, DEPZ_SHTP_CH_CONTROL, payload, len);
    deadline = depz_now_us() + (uint64_t)timeout_ms * 1000u;
    while (rc == DEPZ_OK) {
        rc = waiter_next(dev, &w, deadline, msg, mlen);
        if (rc == DEPZ_OK && (!accept || accept(msg, *mlen, ctx))) break;
    }
    waiter_remove(s, &w);
    return rc;
}

static uint8_t next_cmd_seq(bno_state *s)
{
    uint8_t seq;
    depz_mutex_lock(&s->lock);
    seq = s->cmd_seq++;
    depz_mutex_unlock(&s->lock);
    return seq;
}

typedef struct { uint8_t command, seq; } cmd_key;

static bool accept_command(const uint8_t *msg, size_t len, void *ctx)
{
    const cmd_key *k = (const cmd_key *)ctx;
    depz_bno_command_response r;
    return depz_bno_unpack_command_response(msg, len, &r) == 0 && r.command == k->command && r.command_seq == k->seq;
}

/* SH-2 Command Request; with `resp`, waits for the answer correlated on
 * (command, command sequence). */
static int command(depz_device *dev, uint8_t cmd, const uint8_t *params, size_t n,
                   depz_bno_command_response *resp)
{
    uint8_t payload[12], msg[CTRL_MAX];
    size_t mlen;
    cmd_key key;
    if (n > 9) return depz_fail(DEPZ_E_ARG, "bno086: a command carries at most 9 parameter bytes");
    key.command = cmd;
    key.seq = next_cmd_seq(st(dev));
    depz_bno_pack_command_request(key.seq, cmd, params, n, payload);
    if (!resp) return send_shtp(dev, DEPZ_SHTP_CH_CONTROL, payload, sizeof payload);
    TRY(control_request(dev, payload, sizeof payload, DEPZ_SH2_COMMAND_RESPONSE, accept_command, &key,
                        timeout_at_least(dev, CONTROL_TIMEOUT_MS), msg, &mlen));
    depz_bno_unpack_command_response(msg, mlen, resp);
    return DEPZ_OK;
}

/* A Command Request answered by several responses: gather those correlated on
 * (command, sequence) until `done` says the set is complete. */
typedef bool (*collect_fn)(const depz_bno_command_response *r, void *ctx);

static int command_collect(depz_device *dev, uint8_t cmd, const uint8_t *params, size_t n,
                           collect_fn done, void *ctx)
{
    bno_state *s = st(dev);
    uint8_t payload[12], msg[CTRL_MAX];
    size_t mlen;
    waiter w;
    uint64_t deadline;
    uint8_t seq = next_cmd_seq(s);
    int rc;
    depz_bno_pack_command_request(seq, cmd, params, n, payload);
    waiter_add(s, &w, DEPZ_SH2_COMMAND_RESPONSE);
    rc = send_shtp(dev, DEPZ_SHTP_CH_CONTROL, payload, sizeof payload);
    deadline = depz_now_us() + (uint64_t)timeout_at_least(dev, CONTROL_TIMEOUT_MS) * 1000u;
    while (rc == DEPZ_OK) {
        depz_bno_command_response r;
        rc = waiter_next(dev, &w, deadline, msg, &mlen);
        if (rc) break;
        if (depz_bno_unpack_command_response(msg, mlen, &r) || r.command != cmd || r.command_seq != seq) continue;
        if (done(&r, ctx)) break;
    }
    waiter_remove(s, &w);
    return rc;
}

static int sh2_status(const char *what, uint8_t status)
{
    if (status) return depz_fail(DEPZ_E_STATUS, "bno086: %s failed: SH-2 status %u", what, status);
    return DEPZ_OK;
}

/* ── lifecycle ─────────────────────────────────────────────────────────── */

/* Forget the SHTP state and the advertisement (the sensor restarts its
 * sequence counters on reset). */
static void forget_link_state(bno_state *s, bool advertisement)
{
    depz_mutex_lock(&s->lock);
    depz_shtp_free(&s->shtp);
    depz_shtp_init(&s->shtp);
    if (advertisement) {
        s->reset_seen = false;
        s->adv_len = 0;
    }
    depz_mutex_unlock(&s->lock);
}

/* Wait until the executable reset-complete arrived (false on timeout). */
static bool wait_reset_seen(depz_device *dev, int timeout_ms)
{
    bno_state *s = st(dev);
    uint64_t deadline = depz_now_us() + (uint64_t)timeout_ms * 1000u;
    bool seen;
    depz_mutex_lock(&s->lock);
    while (!s->reset_seen && !depz_device_closed(dev)) {
        int left = depz_ms_until(deadline);
        if (left <= 0) break;
        depz_cond_wait_ms(&s->cond, &s->lock, left);
    }
    seen = s->reset_seen;
    depz_mutex_unlock(&s->lock);
    return seen;
}

static int do_hardware_reset(depz_device *dev, int timeout_ms)
{
    if (timeout_ms < 0) timeout_ms = RESET_TIMEOUT_MS;
    forget_link_state(st(dev), true);
    TRY(depz_device_request(dev, CMD_SENSOR_RESET, NULL, 0, NULL, NULL, true, timeout_ms));
    wait_reset_seen(dev, timeout_ms < RESET_COMPLETE_WAIT_MS ? timeout_ms : RESET_COMPLETE_WAIT_MS);
    return DEPZ_OK;
}

int depz_bno086_hardware_reset(depz_device *dev, int timeout_ms) { LOCKED(do_hardware_reset(dev, timeout_ms)); }

int depz_bno086_wake(depz_device *dev)
{
    LOCKED(depz_device_request(dev, CMD_SENSOR_WAKE_UP, NULL, 0, NULL, NULL, true, -1));
}

int depz_bno086_advertisement(depz_device *dev, uint8_t *buf, size_t cap, size_t *len)
{
    bno_state *s;
    TRY(need(dev));
    s = st(dev);
    depz_mutex_lock(&s->lock);
    if (buf) memcpy(buf, s->adv, s->adv_len < cap ? s->adv_len : cap);
    if (len) *len = s->adv_len;
    depz_mutex_unlock(&s->lock);
    return DEPZ_OK;
}

/* ── identification and features ───────────────────────────────────────── */

static int do_product_id(depz_device *dev, depz_bno_product_id *out)
{
    uint8_t req[2], msg[CTRL_MAX];
    size_t mlen;
    depz_bno_pack_product_id_request(req);
    TRY(control_request(dev, req, sizeof req, DEPZ_SH2_PRODUCT_ID_RESPONSE, NULL, NULL,
                        timeout_at_least(dev, CONTROL_TIMEOUT_MS), msg, &mlen));
    if (depz_bno_unpack_product_id(msg, mlen, out))
        return depz_fail(DEPZ_E_PROTOCOL, "bno086: short product-id response (%zu bytes)", mlen);
    return DEPZ_OK;
}

int depz_bno086_product_id(depz_device *dev, depz_bno_product_id *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_product_id(dev, out));
}

static bool accept_feature(const uint8_t *msg, size_t len, void *ctx)
{
    depz_bno_feature f;
    return depz_bno_unpack_feature_response(msg, len, &f) == 0 && f.sensor_id == *(const uint8_t *)ctx;
}

static int do_get_feature(depz_device *dev, uint8_t sensor, depz_bno_feature *out)
{
    uint8_t req[2], msg[CTRL_MAX];
    size_t mlen;
    depz_bno_pack_get_feature_request(sensor, req);
    TRY(control_request(dev, req, sizeof req, DEPZ_SH2_GET_FEATURE_RESPONSE, accept_feature, &sensor,
                        timeout_at_least(dev, CONTROL_TIMEOUT_MS), msg, &mlen));
    depz_bno_unpack_feature_response(msg, mlen, out);
    return DEPZ_OK;
}

int depz_bno086_get_feature(depz_device *dev, uint8_t sensor, depz_bno_feature *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_get_feature(dev, sensor, out));
}

static int do_enable(depz_device *dev, uint8_t sensor, const depz_bno086_feature_request *req,
                     depz_bno_feature *granted)
{
    uint8_t p[17];
    int k;
    if (!req->interval_us) return depz_fail(DEPZ_E_ARG, "bno086: interval 0 disables — use disable()");
    depz_bno_pack_set_feature(sensor, req->flags, req->sensitivity, req->interval_us, req->batch_us,
                              req->cfg_word, p);
    TRY(send_shtp(dev, DEPZ_SHTP_CH_CONTROL, p, sizeof p));
    if (!granted) return DEPZ_OK;
    /* A "disabled" answer to a sensor just enabled is stale, early or a
     * refusal: the real answer follows within a round trip or two. */
    TRY(do_get_feature(dev, sensor, granted));
    for (k = 0; k < VERIFY_STALE_RETRIES && granted->interval_us == 0; k++)
        TRY(do_get_feature(dev, sensor, granted));
    return DEPZ_OK;
}

int depz_bno086_enable(depz_device *dev, uint8_t sensor, double hz, depz_bno_feature *granted)
{
    depz_bno086_feature_request req;
    depz_bno_feature scratch;
    double us;
    if (!(hz > 0)) return depz_fail(DEPZ_E_ARG, "bno086: rate must be positive — use disable()");
    memset(&req, 0, sizeof req);
    us = nearbyint(1e6 / hz); /* round half to even, as the Python SDK */
    req.interval_us = us < 1 ? 1u : us > 4294967295.0 ? 0xFFFFFFFFu : (uint32_t)us;
    LOCKED(do_enable(dev, sensor, &req, granted ? granted : &scratch));
}

int depz_bno086_enable_ex(depz_device *dev, uint8_t sensor, const depz_bno086_feature_request *req,
                          depz_bno_feature *granted)
{
    if (!req) return depz_fail(DEPZ_E_ARG, "bno086: NULL feature request");
    LOCKED(do_enable(dev, sensor, req, granted));
}

bool depz_bno086_rate_ok(uint32_t requested_interval_us, const depz_bno_feature *granted)
{
    double want, got;
    if (!requested_interval_us || !granted) return false;
    want = 1e6 / requested_interval_us;
    got = granted->interval_us ? 1e6 / granted->interval_us : 0.0;
    return DEPZ_BNO086_RATE_LOW_FACTOR * want <= got && got <= DEPZ_BNO086_RATE_HIGH_FACTOR * want;
}

static int do_disable(depz_device *dev, uint8_t sensor)
{
    uint8_t p[17];
    depz_bno_pack_set_feature(sensor, 0, 0, 0, 0, 0, p);
    return send_shtp(dev, DEPZ_SHTP_CH_CONTROL, p, sizeof p);
}

int depz_bno086_disable(depz_device *dev, uint8_t sensor) { LOCKED(do_disable(dev, sensor)); }

/* ── reports ───────────────────────────────────────────────────────────── */

int depz_bno086_on_report(depz_device *dev, depz_bno086_report_cb cb, void *user, int *token)
{
    TRY(need(dev));
    if (!cb) return depz_fail(DEPZ_E_ARG, "bno086: NULL callback");
    return depz_cb_list_add(&st(dev)->cbs, (void (*)(void))cb, user, token);
}

void depz_bno086_off_report(depz_device *dev, int token)
{
    if (depz_is_bno086(dev)) depz_cb_list_remove(&st(dev)->cbs, token);
}

depz_stream *depz_bno086_reports(depz_device *dev, size_t maxsize)
{
    if (need(dev)) return NULL;
    return depz_hub_subscribe(st(dev)->hub, maxsize ? maxsize : 1024);
}

int depz_bno086_get_report(depz_device *dev, int timeout_ms, depz_bno_report *out)
{
    depz_stream *s;
    int rc;
    TRY(need(dev));
    s = depz_hub_subscribe(st(dev)->hub, 1);
    if (!s) return depz_fail(DEPZ_E_NOMEM, "bno086: cannot subscribe");
    rc = depz_stream_next(s, out, timeout_ms < 0 ? 1000 : timeout_ms);
    depz_stream_close(s);
    return rc;
}

uint64_t depz_bno086_shtp_discarded(const depz_device *dev)
{
    bno_state *s;
    uint64_t n;
    if (!depz_is_bno086(dev)) return 0;
    s = st((depz_device *)dev);
    depz_mutex_lock(&s->lock);
    n = s->shtp.discarded;
    depz_mutex_unlock(&s->lock);
    return n;
}

/* ── tare and calibration ──────────────────────────────────────────────── */

int depz_bno086_tare_now(depz_device *dev, uint8_t axes, uint8_t basis)
{
    uint8_t p[3];
    p[0] = 0x00;
    p[1] = axes;
    p[2] = basis;
    LOCKED(command(dev, DEPZ_SH2_CMD_TARE, p, sizeof p, NULL));
}

int depz_bno086_persist_tare(depz_device *dev)
{
    static const uint8_t p[1] = {0x01};
    LOCKED(command(dev, DEPZ_SH2_CMD_TARE, p, sizeof p, NULL));
}

int depz_bno086_set_reorientation(depz_device *dev, double x, double y, double z, double w)
{
    const double v[4] = {x, y, z, w};
    uint8_t p[9];
    int k;
    p[0] = 0x02;
    for (k = 0; k < 4; k++) {
        double q14 = nearbyint(v[k] * 16384.0);
        if (!(q14 >= -32768.0 && q14 <= 32767.0))
            return depz_fail(DEPZ_E_ARG, "bno086: quaternion component out of the Q14 int16 range");
        put_u16(p + 1 + 2 * k, (uint16_t)(int16_t)q14);
    }
    LOCKED(command(dev, DEPZ_SH2_CMD_TARE, p, sizeof p, NULL));
}

static int do_status_command(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t n, const char *what,
                             depz_bno_command_response *r)
{
    TRY(command(dev, cmd, p, n, r));
    return sh2_status(what, r->r[0]);
}

int depz_bno086_set_calibration(depz_device *dev, bool accel, bool gyro, bool mag, bool planar)
{
    uint8_t p[5];
    depz_bno_command_response r;
    p[0] = accel;
    p[1] = gyro;
    p[2] = mag;
    p[3] = 0x00; /* configure */
    p[4] = planar;
    LOCKED(do_status_command(dev, DEPZ_SH2_CMD_ME_CALIBRATE, p, sizeof p, "ME calibration configure", &r));
}

static int do_get_calibration(depz_device *dev, depz_bno086_calibration *out)
{
    static const uint8_t p[5] = {0, 0, 0, 0x01 /* get */, 0};
    depz_bno_command_response r;
    TRY(do_status_command(dev, DEPZ_SH2_CMD_ME_CALIBRATE, p, sizeof p, "ME calibration get", &r));
    out->accel = r.r[1] != 0;
    out->gyro = r.r[2] != 0;
    out->mag = r.r[3] != 0;
    out->planar = r.r[4] != 0;
    return DEPZ_OK;
}

int depz_bno086_get_calibration(depz_device *dev, depz_bno086_calibration *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_get_calibration(dev, out));
}

int depz_bno086_save_dcd(depz_device *dev)
{
    depz_bno_command_response r;
    LOCKED(do_status_command(dev, DEPZ_SH2_CMD_SAVE_DCD, NULL, 0, "DCD save", &r));
}

int depz_bno086_configure_periodic_dcd(depz_device *dev, bool enable)
{
    uint8_t p[1];
    p[0] = enable ? 0x00 : 0x01;
    LOCKED(command(dev, DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG, p, sizeof p, NULL));
}

/* ── FRS ───────────────────────────────────────────────────────────────── */

static int do_frs_read(depz_device *dev, uint16_t record, uint32_t *words, size_t cap, size_t *n)
{
    bno_state *s = st(dev);
    uint8_t req[8], msg[CTRL_MAX];
    size_t mlen, count = 0;
    waiter w;
    uint64_t deadline;
    bool done = false;
    int rc;
    depz_bno_pack_frs_read_request(record, 0, 0, req);
    waiter_add(s, &w, DEPZ_SH2_FRS_READ_RESPONSE);
    rc = send_shtp(dev, DEPZ_SHTP_CH_CONTROL, req, sizeof req);
    deadline = depz_now_us() + (uint64_t)timeout_at_least(dev, FRS_TIMEOUT_MS) * 1000u;
    while (rc == DEPZ_OK && !done) {
        depz_bno_frs_read_response r;
        uint32_t d[2];
        unsigned k, valid;
        rc = waiter_next(dev, &w, deadline, msg, &mlen);
        if (rc) break;
        if (depz_bno_unpack_frs_read_response(msg, mlen, &r) || r.frs_type != record) continue;
        switch (r.status) {
        case DEPZ_BNO_FRS_READ_UNRECOGNIZED_TYPE:
        case DEPZ_BNO_FRS_READ_BUSY:
        case DEPZ_BNO_FRS_READ_OFFSET_OUT_OF_RANGE:
        case DEPZ_BNO_FRS_READ_RECORD_EMPTY:
        case DEPZ_BNO_FRS_READ_DEVICE_ERROR:
            rc = depz_fail(DEPZ_E_STATUS, "bno086: FRS read 0x%04X failed: status %u", record, r.status);
            continue;
        default:
            break;
        }
        d[0] = r.data0;
        d[1] = r.data1;
        valid = r.data_length < 2 ? r.data_length : 2;
        for (k = 0; k < valid; k++, count++)
            if (words && count < cap) words[count] = d[k];
        done = r.status == DEPZ_BNO_FRS_READ_COMPLETED || r.status == DEPZ_BNO_FRS_READ_BLOCK_AND_READ_COMPLETED;
    }
    waiter_remove(s, &w);
    if (rc) return rc;
    if (n) *n = count;
    if (count > cap) return depz_fail(DEPZ_E_ARG, "bno086: FRS record 0x%04X has %zu words, room for %zu",
                                      record, count, cap);
    return DEPZ_OK;
}

int depz_bno086_frs_read(depz_device *dev, uint16_t record, uint32_t *words, size_t cap, size_t *n)
{
    LOCKED(do_frs_read(dev, record, words, cap, n));
}

static int do_frs_write(depz_device *dev, uint16_t record, const uint32_t *words, size_t n)
{
    bno_state *s = st(dev);
    uint8_t req[12], msg[CTRL_MAX];
    size_t mlen, offset = 0;
    waiter w;
    uint64_t deadline;
    bool done = false;
    int rc;
    if (n > 0xFFFF) return depz_fail(DEPZ_E_ARG, "bno086: FRS record too long");
    depz_bno_pack_frs_write_request(record, (uint16_t)n, req);
    waiter_add(s, &w, DEPZ_SH2_FRS_WRITE_RESPONSE);
    rc = send_shtp(dev, DEPZ_SHTP_CH_CONTROL, req, 6);
    deadline = depz_now_us() + (uint64_t)timeout_at_least(dev, FRS_TIMEOUT_MS) * 1000u;
    while (rc == DEPZ_OK && !done) {
        depz_bno_frs_write_response r;
        rc = waiter_next(dev, &w, deadline, msg, &mlen);
        if (rc) break;
        if (depz_bno_unpack_frs_write_response(msg, mlen, &r)) continue;
        switch (r.status) {
        case DEPZ_BNO_FRS_WRITE_UNRECOGNIZED_TYPE:
        case DEPZ_BNO_FRS_WRITE_BUSY:
        case DEPZ_BNO_FRS_WRITE_FAILED:
        case DEPZ_BNO_FRS_WRITE_NOT_IN_WRITE_MODE:
        case DEPZ_BNO_FRS_WRITE_INVALID_LENGTH:
        case DEPZ_BNO_FRS_WRITE_RECORD_INVALID:
            rc = depz_fail(DEPZ_E_STATUS, "bno086: FRS write 0x%04X failed: status %u", record, r.status);
            break;
        case DEPZ_BNO_FRS_WRITE_COMPLETED:
            done = true;
            break;
        case DEPZ_BNO_FRS_WRITE_MODE_READY:
        case DEPZ_BNO_FRS_WRITE_WORDS_RECEIVED:
            if (offset < n) {
                size_t chunk = n - offset < 2 ? n - offset : 2;
                size_t len = depz_bno_pack_frs_write_data((uint16_t)offset, words + offset, chunk, req, sizeof req);
                offset += chunk;
                rc = send_shtp(dev, DEPZ_SHTP_CH_CONTROL, req, len);
            }
            break;
        default: /* RECORD_VALID and friends: informational */
            break;
        }
    }
    waiter_remove(s, &w);
    return rc;
}

int depz_bno086_frs_write(depz_device *dev, uint16_t record, const uint32_t *words, size_t n)
{
    if (n && !words) return depz_fail(DEPZ_E_ARG, "bno086: NULL words");
    LOCKED(do_frs_write(dev, record, words, n));
}

static int do_get_metadata(depz_device *dev, uint8_t sensor, depz_bno_metadata *out)
{
    uint32_t words[64];
    size_t n = 0;
    uint16_t record = depz_bno_metadata_record(sensor);
    if (!record) return depz_fail(DEPZ_E_ARG, "bno086: no metadata FRS record known for sensor 0x%02X", sensor);
    TRY(do_frs_read(dev, record, words, sizeof words / sizeof words[0], &n));
    depz_bno_metadata_from_words(words, n, out);
    return DEPZ_OK;
}

int depz_bno086_get_metadata(depz_device *dev, uint8_t sensor, depz_bno_metadata *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_get_metadata(dev, sensor, out));
}

/* ── diagnostics and housekeeping ──────────────────────────────────────── */

static int do_oscillator(depz_device *dev, uint8_t *type)
{
    depz_bno_command_response r;
    TRY(command(dev, DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE, NULL, 0, &r));
    *type = r.r[0];
    return DEPZ_OK;
}

int depz_bno086_get_oscillator_type(depz_device *dev, uint8_t *type)
{
    if (!type) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_oscillator(dev, type));
}

static int do_clear_dcd_and_reset(depz_device *dev, int timeout_ms)
{
    if (timeout_ms < 0) timeout_ms = RESET_TIMEOUT_MS;
    forget_link_state(st(dev), true);
    TRY(command(dev, DEPZ_SH2_CMD_CLEAR_DCD_AND_RESET, NULL, 0, NULL));
    if (!wait_reset_seen(dev, timeout_ms))
        return depz_fail(DEPZ_E_TIMEOUT, "bno086: no reset-complete after clear-DCD-and-reset");
    /* The command consumed a host TX sequence number; the sensor restarted
     * its counters on reset. The advertisement stays. */
    forget_link_state(st(dev), false);
    return DEPZ_OK;
}

int depz_bno086_clear_dcd_and_reset(depz_device *dev, int timeout_ms)
{
    LOCKED(do_clear_dcd_and_reset(dev, timeout_ms));
}

typedef struct {
    depz_bno086_error_record *out;
    size_t cap, n;
} errors_ctx;

static bool collect_error(const depz_bno_command_response *r, void *ctx)
{
    errors_ctx *c = (errors_ctx *)ctx;
    if (r->r[2] == DEPZ_BNO_ERR_SOURCE_NO_MORE) return true;
    if (c->out && c->n < c->cap) {
        depz_bno086_error_record *e = &c->out[c->n];
        e->severity = r->r[0];
        e->seq = r->r[1];
        e->source = r->r[2];
        e->error = r->r[3];
        e->module = r->r[4];
        e->code = r->r[5];
    }
    c->n++;
    return false;
}

static int do_get_errors(depz_device *dev, uint8_t severity, depz_bno086_error_record *out, size_t cap, size_t *n)
{
    errors_ctx c;
    c.out = out;
    c.cap = cap;
    c.n = 0;
    TRY(command_collect(dev, DEPZ_SH2_CMD_ERRORS, &severity, 1, collect_error, &c));
    if (n) *n = c.n < cap ? c.n : cap;
    return DEPZ_OK;
}

int depz_bno086_get_errors(depz_device *dev, uint8_t severity, depz_bno086_error_record *out, size_t cap,
                           size_t *n)
{
    LOCKED(do_get_errors(dev, severity, out, cap, n));
}

typedef struct {
    depz_bno_command_response r[2];
    bool have[2];
} counts_ctx;

static bool collect_counts(const depz_bno_command_response *r, void *ctx)
{
    counts_ctx *c = (counts_ctx *)ctx;
    if (r->response_seq < 2) {
        c->r[r->response_seq] = *r;
        c->have[r->response_seq] = true;
    }
    return r->response_seq == 1 || c->have[1];
}

static int do_get_counts(depz_device *dev, uint8_t sensor, depz_bno086_counts *out)
{
    counts_ctx c;
    uint8_t p[2];
    p[0] = 0x00; /* get */
    p[1] = sensor;
    memset(&c, 0, sizeof c);
    TRY(command_collect(dev, DEPZ_SH2_CMD_COUNTER, p, sizeof p, collect_counts, &c));
    if (!c.have[0]) return depz_fail(DEPZ_E_PROTOCOL, "bno086: counts answer without its first part");
    out->sensor_id = sensor;
    out->offered = rd_u32(c.r[0].r + 3);
    out->accepted = rd_u32(c.r[0].r + 7);
    out->on = rd_u32(c.r[1].r + 3);
    out->attempted = rd_u32(c.r[1].r + 7);
    return DEPZ_OK;
}

int depz_bno086_get_counts(depz_device *dev, uint8_t sensor, depz_bno086_counts *out)
{
    if (!out) return depz_fail(DEPZ_E_ARG, "bno086: NULL output");
    LOCKED(do_get_counts(dev, sensor, out));
}

int depz_bno086_clear_counts(depz_device *dev, uint8_t sensor)
{
    uint8_t p[2];
    depz_bno_command_response r;
    p[0] = 0x01; /* clear */
    p[1] = sensor;
    LOCKED(do_status_command(dev, DEPZ_SH2_CMD_COUNTER, p, sizeof p, "clear counts", &r));
}

int depz_bno086_command(depz_device *dev, uint8_t cmd, const uint8_t *params, size_t n,
                        depz_bno_command_response *resp)
{
    if (n && !params) return depz_fail(DEPZ_E_ARG, "bno086: NULL params");
    LOCKED(command(dev, cmd, params, n, resp));
}

int depz_bno086_send_shtp(depz_device *dev, uint8_t channel, const uint8_t *payload, size_t len)
{
    if (len && !payload) return depz_fail(DEPZ_E_ARG, "bno086: NULL payload");
    if (channel >= DEPZ_SHTP_NUM_CHANNELS) return depz_fail(DEPZ_E_ARG, "bno086: no SHTP channel %u", channel);
    LOCKED(send_shtp(dev, channel, payload, len));
}
