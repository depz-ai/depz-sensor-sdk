/* test_io.c — the live-hardware layer against a fake SR04 over loopback, and
 * record/replay round trips. Mirrors the Python SDK's test_device_core.py and
 * test_sr04_coverage.py. Usage: test_io [name] (no name = all). */
#include "depz_sensor_io.h"
#include "fake_sr04.h"
#include "fake_vl53l4.h"
#include "fake_bno086.h"
#include "json.h"

#include "../src/io/platform.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failed;

#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            fprintf(stderr, "  FAIL %s:%d: %s  [last error: %s]\n", __FILE__,     \
                    __LINE__, #c, depz_last_error());                             \
            g_failed++;                                                           \
            return;                                                               \
        }                                                                         \
    } while (0)

#define CHECK_RC(expr, want) CHECK((expr) == (want))

typedef struct {
    fake_sr04   *fake;
    depz_device *dev;
} rig;

static bool rig_up(rig *r)
{
    depz_link *link;
    r->fake = fake_sr04_start(&link);
    if (!r->fake) return false;
    if (depz_sr04_open_link(link, &r->dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(r->dev, 1000);
    return true;
}

static void rig_down(rig *r)
{
    depz_device_close(r->dev);
    fake_sr04_stop(r->fake);
}

/* ── device core ─────────────────────────────────────────────────────── */

static void identity_roundtrip(rig *r)
{
    char s[128];
    CHECK_RC(depz_device_get_software_name(r->dev, s, sizeof s), DEPZ_OK);
    CHECK(strcmp(s, FAKE_SR04_SOFTWARE) == 0);
    CHECK_RC(depz_device_get_device_name(r->dev, s, sizeof s), DEPZ_OK);
    CHECK(strcmp(s, FAKE_SR04_NAME) == 0);
    CHECK_RC(depz_device_get_serial_number(r->dev, s, sizeof s), DEPZ_OK);
    CHECK(strcmp(s, FAKE_SR04_SERIAL) == 0);
}

static void temperature(rig *r)
{
    double c = 0;
    CHECK_RC(depz_device_read_mcu_temperature(r->dev, &c), DEPZ_OK);
    CHECK(fabs(c - 27.3) < 1e-9);
}

static void sync_time_produces_offset(rig *r)
{
    depz_time_sync s;
    int64_t host;
    CHECK(!depz_device_time_sync(r->dev, NULL));
    CHECK_RC(depz_device_to_host_time_us(r->dev, 1, &host), DEPZ_E_ARG);
    CHECK_RC(depz_device_sync_time(r->dev, 3, &s), DEPZ_OK);
    CHECK(s.rtt_us >= 0);
    CHECK(depz_device_time_sync(r->dev, NULL));
    CHECK_RC(depz_device_to_host_time_us(r->dev, 2000000, &host), DEPZ_OK);
    CHECK(host == 2000000 - s.offset_us);
}

static void unknown_cmd_raises_status_error(rig *r)
{
    CHECK_RC(depz_device_request(r->dev, 0x5A, NULL, 0, NULL, NULL, true, -1), DEPZ_E_STATUS);
    CHECK(depz_last_status() == DEPZ_STATUS_ERR_INVALID_CMD);
    CHECK(depz_last_status_cmd() == 0x5A);
}

static void sync_pin_validation(rig *r)
{
    depz_sync_pin_config c = {1, DEPZ_SYNC_PIN_IN, 0}, got;
    CHECK_RC(depz_device_set_sync_pin(r->dev, &c), DEPZ_OK);
    c.pin = 9;
    CHECK_RC(depz_device_set_sync_pin(r->dev, &c), DEPZ_E_STATUS);
    CHECK(depz_last_status() == DEPZ_STATUS_ERR_INVALID_PARAM);
    CHECK_RC(depz_device_get_sync_pin(r->dev, 2, &got), DEPZ_OK);
    CHECK(got.pin == 2);
}

static void timeout_when_device_silent(void)
{
    depz_link *link;
    depz_device *dev;
    char s[64];
    uint64_t t0;
    fake_sr04 *f = fake_silent_start(&link);
    CHECK(f);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 100);
    t0 = depz_host_now_us();
    CHECK_RC(depz_device_get_software_name(dev, s, sizeof s), DEPZ_E_TIMEOUT);
    CHECK(depz_host_now_us() - t0 >= 90000);
    CHECK(strstr(depz_last_error(), "0x04") != NULL);
    depz_device_close(dev);
    fake_sr04_stop(f);
}

static void wrong_type_is_refused(void)
{
    depz_link *link;
    depz_device *dev;
    uint32_t v;
    fake_sr04 *f = fake_sr04_start(&link);
    CHECK(f);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    CHECK(!depz_is_sr04(dev));
    CHECK_RC(depz_sr04_get_sample_period_us(dev, &v), DEPZ_E_WRONG_TYPE);
    CHECK(depz_sr04_stream(dev, 4) == NULL);
    /* promote = identify + attach, as open_device does after its probe */
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_sr04(dev));
    CHECK(depz_device_sensor_type(dev) == DEPZ_SENSOR_SR04);
    CHECK_RC(depz_sr04_get_sample_period_us(dev, &v), DEPZ_OK);
    CHECK(v == 50000);
    depz_device_close(dev);
    fake_sr04_stop(f);
}

/* ── SR04 ────────────────────────────────────────────────────────────── */

static void sample_period_readback_is_stored(rig *r)
{
    uint32_t v;
    CHECK_RC(depz_sr04_set_sample_period_us(r->dev, 7000), DEPZ_OK);
    CHECK_RC(depz_sr04_get_sample_period_us(r->dev, &v), DEPZ_OK);
    CHECK(v == 7000);
    CHECK(fake_sr04_sample_period(r->fake) == 7000);
}

static void echo_decay_clamps_both_ends(rig *r)
{
    uint16_t eff, v;
    CHECK_RC(depz_sr04_set_echo_decay_us(r->dev, 100, &eff), DEPZ_OK);
    CHECK(eff == DEPZ_SR04_ECHO_DECAY_MIN_US);
    CHECK_RC(depz_sr04_set_echo_decay_us(r->dev, 65535, &eff), DEPZ_OK);
    CHECK(eff == DEPZ_SR04_ECHO_DECAY_MAX_US);
    CHECK_RC(depz_sr04_set_echo_decay_us(r->dev, 9000, &eff), DEPZ_OK);
    CHECK(eff == 9000);
    CHECK_RC(depz_sr04_get_echo_decay_us(r->dev, &v), DEPZ_OK);
    CHECK(v == 9000);
    CHECK_RC(depz_sr04_set_echo_decay_us(r->dev, 65536, &eff), DEPZ_E_ARG);
}

static void measure_once_is_source_once(rig *r)
{
    depz_sr04_measurement m;
    double mm;
    fake_sr04_set_echo(r->fake, 5831);
    CHECK_RC(depz_sr04_measure_once(r->dev, -1, &m), DEPZ_OK);
    CHECK(!m.from_loop && depz_sr04_valid(&m));
    CHECK(m.echo_time_us == 5831);
    CHECK(depz_sr04_measurement_distance_mm(&m, &mm));
    CHECK(fabs(mm - 5831 * 0.343 / 2) < 1e-6);
}

typedef struct { depz_mutex lock; int n; bool from_loop_all; } counter;

static void count_cb(const depz_sr04_measurement *m, void *user)
{
    counter *c = (counter *)user;
    depz_mutex_lock(&c->lock);
    c->n++;
    if (!m->from_loop) c->from_loop_all = false;
    depz_mutex_unlock(&c->lock);
}

static int counter_get(counter *c)
{
    int n;
    depz_mutex_lock(&c->lock);
    n = c->n;
    depz_mutex_unlock(&c->lock);
    return n;
}

static void loop_samples_are_source_loop(rig *r)
{
    depz_sr04_measurement m;
    depz_stream *s = depz_sr04_stream(r->dev, 16);
    counter c;
    int tok, i;
    CHECK(s);
    depz_mutex_init(&c.lock);
    c.n = 0;
    c.from_loop_all = true;
    CHECK_RC(depz_sr04_on_measurement(r->dev, count_cb, &c, &tok), DEPZ_OK);
    CHECK_RC(depz_sr04_start(r->dev), DEPZ_OK);
    CHECK(fake_sr04_loop_running(r->fake));
    for (i = 0; i < 3; i++) fake_sr04_send_measurement(r->fake, 0x37);
    for (i = 0; i < 3; i++) {
        CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
        CHECK(m.from_loop);
    }
    CHECK(counter_get(&c) == 3 && c.from_loop_all);
    depz_sr04_off_measurement(r->dev, tok);
    fake_sr04_send_measurement(r->fake, 0x37);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    CHECK(counter_get(&c) == 3); /* unsubscribed */
    CHECK_RC(depz_sr04_stop(r->dev), DEPZ_OK);
    depz_stream_close(s);
    depz_mutex_destroy(&c.lock);
}

static void start_stop_idempotent(rig *r)
{
    CHECK_RC(depz_sr04_start(r->dev), DEPZ_OK);
    CHECK_RC(depz_sr04_start(r->dev), DEPZ_OK);
    CHECK(fake_sr04_loop_running(r->fake));
    CHECK_RC(depz_sr04_stop(r->dev), DEPZ_OK);
    CHECK_RC(depz_sr04_stop(r->dev), DEPZ_OK);
    CHECK(!fake_sr04_loop_running(r->fake));
}

static void measure_once_busy_during_loop(rig *r)
{
    depz_sr04_measurement m;
    CHECK_RC(depz_sr04_start(r->dev), DEPZ_OK);
    CHECK_RC(depz_sr04_measure_once(r->dev, -1, &m), DEPZ_E_BUSY);
    CHECK(depz_last_status() == DEPZ_STATUS_ERR_BUSY);
    CHECK_RC(depz_sr04_stop(r->dev), DEPZ_OK);
    CHECK_RC(depz_sr04_measure_once(r->dev, -1, &m), DEPZ_OK);
}

static void no_echo_timeout_sentinel(rig *r)
{
    depz_sr04_measurement m;
    double mm;
    fake_sr04_set_echo(r->fake, DEPZ_SR04_ECHO_TIMEOUT);
    CHECK_RC(depz_sr04_measure_once(r->dev, -1, &m), DEPZ_OK);
    CHECK(!depz_sr04_valid(&m));
    CHECK(!depz_sr04_measurement_distance_mm(&m, &mm));
}

static void temperature_compensated_distance(rig *r)
{
    depz_sr04_measurement m;
    double mm;
    fake_sr04_set_echo(r->fake, 5831);
    CHECK_RC(depz_sr04_measure_once(r->dev, -1, &m), DEPZ_OK);
    CHECK(depz_sr04_measurement_distance_mm_at(&m, 20.0, &mm));
    CHECK(fabs(mm - 5831 * (331.3 + 0.606 * 20.0) / 1000.0 / 2) < 1e-6);
}

static void stream_drop_oldest_counter(rig *r)
{
    depz_sr04_measurement m;
    depz_stream *s = depz_sr04_stream(r->dev, 2);
    uint64_t first_ts;
    int i;
    CHECK(s);
    for (i = 0; i < 5; i++) fake_sr04_send_measurement(r->fake, 0x37);
    /* Round-trip a request so all five reports have been dispatched. */
    CHECK_RC(depz_sr04_get_sample_period_us(r->dev, (uint32_t *)&i), DEPZ_OK);
    CHECK(depz_stream_dropped_count(s) == 3);
    CHECK_RC(depz_stream_next(s, &m, 100), DEPZ_OK);
    first_ts = m.timestamp_us;
    CHECK_RC(depz_stream_next(s, &m, 100), DEPZ_OK);
    CHECK(m.timestamp_us > first_ts);         /* the newest two survived, in order */
    CHECK_RC(depz_stream_next(s, &m, 50), DEPZ_E_TIMEOUT);
    depz_stream_close(s);
}

static void two_independent_streams(rig *r)
{
    depz_sr04_measurement m;
    depz_stream *a = depz_sr04_stream(r->dev, 8), *b = depz_sr04_stream(r->dev, 8);
    CHECK(a && b);
    fake_sr04_send_measurement(r->fake, 0x37);
    CHECK_RC(depz_stream_next(a, &m, 1000), DEPZ_OK);
    CHECK_RC(depz_stream_next(b, &m, 1000), DEPZ_OK);
    depz_stream_close(a);
    fake_sr04_send_measurement(r->fake, 0x37);
    CHECK_RC(depz_stream_next(b, &m, 1000), DEPZ_OK);
    depz_stream_close(b);
}

static void sync_in_single_shot_reaches_stream(rig *r)
{
    depz_sr04_measurement m;
    depz_stream *s = depz_sr04_stream(r->dev, 8);
    CHECK(s);
    /* A SYNC_IN edge makes the firmware emit a MEASURE_ONCE-sourced report
     * nobody asked for: it must reach streams, not vanish. */
    fake_sr04_send_measurement(r->fake, 0x36);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    CHECK(!m.from_loop);
    depz_stream_close(s);
}

typedef struct { depz_mutex lock; int unsolicited, text, temperature, disconnected; uint8_t status; } ev_count;

static void ev_cb(const depz_device_event *ev, void *user)
{
    ev_count *c = (ev_count *)user;
    depz_mutex_lock(&c->lock);
    if (ev->type == DEPZ_DEV_EV_UNSOLICITED_STATUS) { c->unsolicited++; c->status = ev->status; }
    if (ev->type == DEPZ_DEV_EV_TEXT) c->text++;
    if (ev->type == DEPZ_DEV_EV_TEMPERATURE) c->temperature++;
    if (ev->type == DEPZ_DEV_EV_DISCONNECTED) c->disconnected++;
    depz_mutex_unlock(&c->lock);
}

static void events_and_disconnect(void)
{
    depz_link *link;
    depz_device *dev;
    depz_device_event ev;
    depz_sr04_measurement m;
    depz_stream *evs, *ms;
    ev_count c;
    uint8_t fault[2] = {0x00, DEPZ_STATUS_ERR_HARDWARE_FAULT};
    int tok;
    fake_sr04 *f = fake_sr04_start(&link);
    CHECK(f);
    CHECK_RC(depz_sr04_open_link(link, &dev), DEPZ_OK);
    memset(&c, 0, sizeof c);
    depz_mutex_init(&c.lock);
    CHECK_RC(depz_device_on_event(dev, ev_cb, &c, &tok), DEPZ_OK);
    evs = depz_device_events(dev, 16);
    ms = depz_sr04_stream(dev, 16);
    CHECK(evs && ms);
    fake_sr04_send_raw(f, DEPZ_RPT_STATUS, fault, 2);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_UNSOLICITED_STATUS && ev.status == DEPZ_STATUS_ERR_HARDWARE_FAULT);
    fake_sr04_send_measurement(f, 0x37);
    /* The cable goes: pending waits and streams end, DISCONNECTED fires. */
    fake_sr04_stop(f);
    CHECK_RC(depz_stream_next(ms, &m, 1000), DEPZ_OK);   /* drains first */
    CHECK_RC(depz_stream_next(ms, &m, 1000), DEPZ_E_CLOSED);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_DISCONNECTED);
    CHECK(c.unsolicited == 1 && c.disconnected == 1);
    CHECK(depz_device_closed(dev));
    CHECK_RC(depz_sr04_get_sample_period_us(dev, (uint32_t *)&tok), DEPZ_E_CLOSED);
    depz_device_close(dev);
    /* Streams outlive their device and stay closed. */
    CHECK_RC(depz_stream_next(ms, &m, 10), DEPZ_E_CLOSED);
    depz_stream_close(ms);
    depz_stream_close(evs);
    depz_mutex_destroy(&c.lock);
}

typedef struct { depz_device *dev; int rc; } inflight_arg;

static void inflight_first(void *a)
{
    inflight_arg *x = (inflight_arg *)a;
    char s[16];
    x->rc = depz_device_get_software_name(x->dev, s, sizeof s);
}

static void same_opcode_in_flight_is_busy(void)
{
    /* A silent device keeps the first request pending; a second one with the
     * same opcode is refused at once, a different opcode is not. */
    depz_link *link;
    depz_device *dev;
    depz_thread t;
    inflight_arg a;
    char s[16];
    uint64_t t0;
    fake_sr04 *f = fake_silent_start(&link);
    CHECK(f);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 400);
    a.dev = dev;
    a.rc = 1;
    depz_thread_start(&t, inflight_first, &a);
    depz_sleep_ms(100);
    t0 = depz_host_now_us();
    CHECK_RC(depz_device_get_software_name(dev, s, sizeof s), DEPZ_E_BUSY);
    CHECK(depz_host_now_us() - t0 < 50000);
    depz_device_set_timeout_ms(dev, 50);
    CHECK_RC(depz_device_get_device_name(dev, s, sizeof s), DEPZ_E_TIMEOUT);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_TIMEOUT);
    depz_device_close(dev);
    fake_sr04_stop(f);
}

/* ── record / replay ─────────────────────────────────────────────────── */

static void scripted_session(depz_device *dev, uint32_t *period, depz_sr04_measurement *m)
{
    char s[64];
    depz_device_get_software_name(dev, s, sizeof s);
    depz_sr04_set_sample_period_us(dev, 20000);
    depz_sr04_get_sample_period_us(dev, period);
    depz_sr04_measure_once(dev, -1, m);
}

static void record_then_replay(void)
{
    const char *path = "test_io_roundtrip.depzrec";
    depz_link *link, *rec, *rp;
    depz_device *dev;
    depz_sr04_measurement m1, m2;
    uint32_t p1 = 0, p2 = 0;
    char s[64];
    fake_sr04 *f = fake_sr04_start(&link);
    CHECK(f);
    CHECK_RC(depz_link_open_recording(link, path, "\"port\": \"loopback\"", &rec), DEPZ_OK);
    CHECK_RC(depz_sr04_open_link(rec, &dev), DEPZ_OK);
    scripted_session(dev, &p1, &m1);
    depz_device_close(dev);
    fake_sr04_stop(f);
    CHECK(p1 == 20000);

    /* Strict replay: the same calls re-issue byte-identical requests and get
     * the same answers without any device. */
    CHECK_RC(depz_link_open_replay(path, true, false, &rp), DEPZ_OK);
    CHECK_RC(depz_sr04_open_link(rp, &dev), DEPZ_OK);
    scripted_session(dev, &p2, &m2);
    CHECK(p2 == 20000);
    CHECK(m2.timestamp_us == m1.timestamp_us && m2.echo_time_us == m1.echo_time_us);
    depz_device_close(dev);

    /* A different request is caught. */
    CHECK_RC(depz_link_open_replay(path, true, false, &rp), DEPZ_OK);
    CHECK_RC(depz_sr04_open_link(rp, &dev), DEPZ_OK);
    CHECK_RC(depz_device_get_device_name(dev, s, sizeof s), DEPZ_E_REPLAY_MISMATCH);
    depz_device_close(dev);
    remove(path);
}

/* ── a real SR04 capture (contracts/vectors/recordings) ────────────────── */

#ifndef DEPZ_VECTORS_DIR
#define DEPZ_VECTORS_DIR "."
#endif

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (char *)malloc((size_t)n + 1);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); buf = NULL; }
    if (buf) buf[n] = '\0';
    fclose(f);
    return buf;
}

static bool same_measurement(const depz_sr04_measurement *m, const json_value *want)
{
    const char *src = json_as_str(json_obj_get(want, "source"));
    return m->timestamp_us == (uint64_t)json_as_int(json_obj_get(want, "timestamp_us")) &&
           m->echo_time_us == (uint16_t)json_as_int(json_obj_get(want, "echo_time_us")) &&
           m->from_loop == (src && strcmp(src, "loop") == 0);
}

/* The lab SR04's session, recorded by the Python SDK: strict replay makes the
 * C SDK re-issue every request byte for byte and decode the same values. */
static void sr04_session_replay(void)
{
    char *text = slurp(DEPZ_VECTORS_DIR "/recordings/sr04_session.expected.json");
    json_value *exp;
    const json_value *once, *loop;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_sr04_measurement m;
    char name[128];
    uint32_t period;
    uint16_t decay;
    size_t i;

    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    once = json_obj_get(exp, "once");
    loop = json_obj_get(exp, "loop");
    CHECK_RC(depz_link_open_replay(DEPZ_VECTORS_DIR "/recordings/sr04_session.depzrec", true, false,
                                   &link), DEPZ_OK);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 2000);
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_sr04(dev));
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_device_get_device_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "device_name"))) == 0);
    CHECK_RC(depz_sr04_set_sample_period_us(dev, 20000), DEPZ_OK);
    CHECK_RC(depz_sr04_get_sample_period_us(dev, &period), DEPZ_OK);
    CHECK(period == (uint32_t)json_as_int(json_obj_get(exp, "sample_period_us")));
    CHECK_RC(depz_sr04_get_echo_decay_us(dev, &decay), DEPZ_OK);
    CHECK(decay == (uint16_t)json_as_int(json_obj_get(exp, "echo_decay_us")));
    for (i = 0; i < json_arr_size(once); i++) {
        CHECK_RC(depz_sr04_measure_once(dev, 2000, &m), DEPZ_OK);
        CHECK(same_measurement(&m, json_arr_get(once, i)));
    }
    s = depz_sr04_stream(dev, json_arr_size(loop) + 64);
    CHECK(s);
    CHECK_RC(depz_sr04_start(dev), DEPZ_OK);
    for (i = 0; i < json_arr_size(loop); i++) {
        CHECK_RC(depz_stream_next(s, &m, 2000), DEPZ_OK);
        CHECK(same_measurement(&m, json_arr_get(loop, i)));
    }
    CHECK_RC(depz_sr04_stop(dev), DEPZ_OK);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
}

/* ── VL53L4CD (the ULD over the fake register bridge) ──────────────────── */

typedef struct {
    fake_vl53l4 *fake;
    depz_device *dev;
} l4rig;

static bool l4_up(l4rig *r)
{
    depz_link *link;
    r->fake = fake_vl53l4_start(&link);
    if (!r->fake) return false;
    if (depz_vl53l4cd_open_link(link, &r->dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(r->dev, 1000);
    return true;
}

static void l4_down(l4rig *r)
{
    depz_device_close(r->dev);
    fake_vl53l4_stop(r->fake);
}

#define L4_RIG(name)                                   \
    static void name##_body(l4rig *r);                 \
    static void name(void)                             \
    {                                                  \
        l4rig r;                                       \
        if (!l4_up(&r)) { g_failed++; return; }        \
        name##_body(&r);                               \
        l4_down(&r);                                   \
    }                                                  \
    static void name##_body(l4rig *r)

L4_RIG(l4_init_boot_speed_then_retimes)
{
    uint16_t sp[8];
    size_t n;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    n = fake_vl53l4_speeds(r->fake, sp, 8);
    CHECK(n >= 2 && sp[0] == 400 && sp[n - 1] == 1000);
    CHECK(depz_vl53l4cd_initialized(r->dev));
}

L4_RIG(l4_init_at_400_never_retimes)
{
    uint16_t sp[8];
    CHECK_RC(depz_vl53l4cd_init(r->dev, 400), DEPZ_OK);
    CHECK(fake_vl53l4_speeds(r->fake, sp, 8) == 1 && sp[0] == 400);
}

L4_RIG(l4_init_config_block_and_vhv)
{
    uint8_t want[91], got[256];
    uint32_t b, im;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    depz_vl53l4_config_block(want);
    CHECK(fake_vl53l4_writes_at(r->fake, 0x2D) >= 1);
    CHECK(fake_vl53l4_write_at(r->fake, 0x2D, 0, got, sizeof got) == 91);
    CHECK(memcmp(got, want, 91) == 0 && got[0] == DEPZ_VL53L4_CONFIG_FMP_BYTE);
    /* SYSTEM_START: VHV start 0x40, then stop 0x80 */
    CHECK(fake_vl53l4_write_at(r->fake, 0x0087, 0, got, 1) == 1 && got[0] == 0x40);
    CHECK(fake_vl53l4_write_at(r->fake, 0x0087, 1, got, 1) == 1 && got[0] == 0x80);
    CHECK(fake_vl53l4_write_at(r->fake, 0x0086, 0, got, 1) == 1 && got[0] == 0x01);
    CHECK(fake_vl53l4_reg(r->fake, 0x0008) == 0x09 && fake_vl53l4_reg(r->fake, 0x000B) == 0x00);
    CHECK(fake_vl53l4_reg(r->fake, 0x0024) == 0x05 && fake_vl53l4_reg(r->fake, 0x0025) == 0x00);
    CHECK_RC(depz_vl53l4cd_get_range_timing(r->dev, &b, &im), DEPZ_OK);
    CHECK(im == 0); /* init leaves 50 ms continuous */
}

L4_RIG(l4_range_timing_roundtrip_and_bounds)
{
    uint32_t b, im;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_set_range_timing(r->dev, 50, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_range_timing(r->dev, &b, &im), DEPZ_OK);
    CHECK(b >= 48 && b <= 50 && im == 0);
    CHECK_RC(depz_vl53l4cd_set_range_timing(r->dev, 50, 100), DEPZ_OK); /* autonomous */
    CHECK_RC(depz_vl53l4cd_get_range_timing(r->dev, &b, &im), DEPZ_OK);
    CHECK(im >= 98 && im <= 102);
    CHECK_RC(depz_vl53l4cd_set_range_timing(r->dev, 5, 0), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_range_timing(r->dev, 50, 40), DEPZ_E_ARG);
}

L4_RIG(l4_tuning_roundtrips)
{
    int32_t off;
    uint16_t x, lo, hi, sig, sgm;
    uint8_t w;
    CHECK_RC(depz_vl53l4cd_set_offset_mm(r->dev, -12), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_offset_mm(r->dev, &off), DEPZ_OK);
    CHECK(off == -12);
    CHECK_RC(depz_vl53l4cd_set_xtalk_kcps(r->dev, 20), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_xtalk_kcps(r->dev, &x), DEPZ_OK);
    CHECK(x == 20);
    CHECK_RC(depz_vl53l4cd_set_detection_thresholds(r->dev, 100, 300, DEPZ_VL53L4CD_WINDOW_IN), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_detection_thresholds(r->dev, &lo, &hi, &w), DEPZ_OK);
    CHECK(lo == 100 && hi == 300 && w == DEPZ_VL53L4CD_WINDOW_IN);
    CHECK_RC(depz_vl53l4cd_set_signal_threshold_kcps(r->dev, 1024), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_signal_threshold_kcps(r->dev, &sig), DEPZ_OK);
    CHECK(sig == 1024);
    CHECK_RC(depz_vl53l4cd_set_sigma_threshold_mm(r->dev, 15), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_sigma_threshold_mm(r->dev, &sgm), DEPZ_OK);
    CHECK(sgm == 15);
    CHECK_RC(depz_vl53l4cd_set_sigma_threshold_mm(r->dev, 16384), DEPZ_E_ARG);
}

L4_RIG(l4_is_alive_checks_model_id)
{
    bool alive = false;
    CHECK_RC(depz_vl53l4cd_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(alive);
    fake_vl53l4_set_reg(r->fake, 0x010F, 0x12);
    CHECK_RC(depz_vl53l4cd_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(!alive);
}

L4_RIG(l4_config_refused_while_ranging)
{
    depz_vl53l4cd_measurement m;
    int32_t off;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_stop_ranging(r->dev), DEPZ_OK); /* no-op when idle */
    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_OK);
    CHECK(depz_vl53l4cd_ranging(r->dev) && fake_vl53l4_streaming(r->fake));
    CHECK_RC(depz_vl53l4cd_set_range_timing(r->dev, 50, 0), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_offset_mm(r->dev, 5), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_xtalk_kcps(r->dev, 10), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_detection_thresholds(r->dev, 1, 2, 3), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_signal_threshold_kcps(r->dev, 8), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_sigma_threshold_mm(r->dev, 8), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_set_i2c_speed_khz(r->dev, 400), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_measure_once(r->dev, -1, &m), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_get_offset_mm(r->dev, &off), DEPZ_OK); /* reads are fine */
    CHECK_RC(depz_vl53l4cd_stop_ranging(r->dev), DEPZ_OK);
    CHECK(!depz_vl53l4cd_ranging(r->dev) && !fake_vl53l4_streaming(r->fake));
    CHECK(fake_vl53l4_reg(r->fake, 0x0087) == 0x80);
    CHECK_RC(depz_vl53l4cd_xshut(r->dev, DEPZ_VL53L4_XSHUT_RESET), DEPZ_OK);
    CHECK(!depz_vl53l4cd_initialized(r->dev));
}

L4_RIG(l4_stream_decodes_and_counts_short_blocks)
{
    /* distance 0x0123 = 291 mm, stream count 7, raw status 9 -> 0 (valid) */
    static const uint8_t block[17] = {0x09, 0x00, 0x07, 0x0f, 0x00, 0x01, 0xf4, 0x00, 0x64,
                                      0x00, 0x28, 0x00, 0x00, 0x01, 0x23, 0x00, 0xff};
    depz_vl53l4cd_measurement m;
    depz_stream *s = depz_vl53l4cd_stream(r->dev, 8);
    uint32_t b, im;
    CHECK(s);
    fake_vl53l4_send_stream(r->fake, 1234, block, sizeof block);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    CHECK(m.timestamp_us == 1234 && m.r.distance_mm == 291 && m.r.range_status == 0);
    CHECK(m.r.stream_count == 7);
    CHECK(strcmp(depz_vl53l4cd_status_text(m.r.range_status), "valid") == 0);
    fake_vl53l4_send_stream(r->fake, 1, block, 2);
    CHECK_RC(depz_vl53l4cd_get_range_timing(r->dev, &b, &im), DEPZ_OK); /* round trip */
    CHECK(depz_vl53l4cd_stream_parse_errors(r->dev) == 1);
    CHECK_RC(depz_stream_next(s, &m, 50), DEPZ_E_TIMEOUT);
    depz_stream_close(s);
}

L4_RIG(l4_measure_once_polls_and_stops)
{
    depz_vl53l4cd_measurement m;
    uint8_t v;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_measure_once(r->dev, -1, &m), DEPZ_OK);
    CHECK(m.timestamp_us > 0);
    v = fake_vl53l4_reg(r->fake, 0x0087);
    CHECK(v == 0x80); /* stopped after the read */
}

static bool same_l4(const depz_vl53l4cd_measurement *m, const json_value *w)
{
#define F(name) (json_as_int(json_obj_get(w, #name)))
    return m->timestamp_us == (uint64_t)F(timestamp_us) && m->r.range_status == F(range_status) &&
           m->r.distance_mm == F(distance_mm) && m->r.sigma_mm == F(sigma_mm) &&
           m->r.signal_rate_kcps == F(signal_rate_kcps) && m->r.ambient_rate_kcps == F(ambient_rate_kcps) &&
           m->r.signal_per_spad_kcps == F(signal_per_spad_kcps) &&
           m->r.ambient_per_spad_kcps == F(ambient_per_spad_kcps) &&
           m->r.number_of_spad == F(number_of_spad) && m->r.stream_count == F(stream_count);
#undef F
}

/* The lab VL53L4CD's session (Python SDK capture): init polls, timing, reads,
 * single shots and the stream, all byte-identical under strict replay. */
static void l4_session_replay(void)
{
    char *text = slurp(DEPZ_VECTORS_DIR "/recordings/vl53l4cd_session.expected.json");
    json_value *exp;
    const json_value *once, *frames, *info, *timing;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_vl53l4cd_measurement m;
    depz_vl53l4_info bi;
    char name[128];
    uint32_t b, im;
    int32_t off;
    uint16_t x;
    size_t i;

    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    once = json_obj_get(exp, "once");
    frames = json_obj_get(exp, "frames");
    info = json_obj_get(exp, "info");
    timing = json_obj_get(exp, "timing");
    CHECK_RC(depz_link_open_replay(DEPZ_VECTORS_DIR "/recordings/vl53l4cd_session.depzrec", true,
                                   false, &link), DEPZ_OK);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 2000);
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_vl53l4cd(dev));
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_device_get_device_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "device_name"))) == 0);
    CHECK_RC(depz_vl53l4cd_bridge_info(dev, &bi), DEPZ_OK);
    CHECK(bi.model_id == json_as_int(json_obj_get(info, "model_id")));
    CHECK(bi.fw_status == json_as_int(json_obj_get(info, "fw_status")));
    CHECK(bi.i2c_khz == json_as_int(json_obj_get(info, "i2c_khz")));
    CHECK_RC(depz_vl53l4cd_init(dev, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_set_range_timing(dev, 33, 0), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_get_range_timing(dev, &b, &im), DEPZ_OK);
    CHECK(b == json_as_int(json_arr_get(timing, 0)) && im == json_as_int(json_arr_get(timing, 1)));
    CHECK_RC(depz_vl53l4cd_get_offset_mm(dev, &off), DEPZ_OK);
    CHECK(off == json_as_int(json_obj_get(exp, "offset_mm")));
    CHECK_RC(depz_vl53l4cd_get_xtalk_kcps(dev, &x), DEPZ_OK);
    CHECK(x == json_as_int(json_obj_get(exp, "xtalk_kcps")));
    for (i = 0; i < json_arr_size(once); i++) {
        CHECK_RC(depz_vl53l4cd_measure_once(dev, -1, &m), DEPZ_OK);
        CHECK(same_l4(&m, json_arr_get(once, i)));
    }
    s = depz_vl53l4cd_stream(dev, json_arr_size(frames) + 64);
    CHECK(s);
    CHECK_RC(depz_vl53l4cd_start_ranging(dev), DEPZ_OK);
    for (i = 0; i < json_arr_size(frames); i++) {
        CHECK_RC(depz_stream_next(s, &m, 2000), DEPZ_OK);
        CHECK(same_l4(&m, json_arr_get(frames, i)));
    }
    CHECK_RC(depz_vl53l4cd_stop_ranging(dev), DEPZ_OK);
    CHECK(depz_vl53l4cd_stream_parse_errors(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
}

/* ── VL53L8 (real captures: init with the firmware download, 8x8 @ 15 Hz) ── */

static bool same_ints(const json_value *arr, const int32_t *v, int n)
{
    int k;
    if ((int)json_arr_size(arr) != n) return false;
    for (k = 0; k < n; k++)
        if (json_as_int(json_arr_get(arr, (size_t)k)) != v[k]) return false;
    return true;
}

static bool same_u8s(const json_value *arr, const uint8_t *v, int n)
{
    int k;
    if ((int)json_arr_size(arr) != n) return false;
    for (k = 0; k < n; k++)
        if (json_as_int(json_arr_get(arr, (size_t)k)) != v[k]) return false;
    return true;
}

static bool same_hex(const char *hex, const uint8_t *v, size_t n)
{
    size_t k;
    if (!hex || strlen(hex) != 2 * n) return false;
    for (k = 0; k < n; k++) {
        unsigned b;
        if (sscanf(hex + 2 * k, "%2x", &b) != 1 || b != v[k]) return false;
    }
    return true;
}

/* The Python SDK's capture replayed strict: the whole ULD init (the ~84 KB
 * firmware download included), 8x8, 15 Hz, [CNH], the stream. */
static void l8_replay_res(const char *stem, depz_vl53l8_model model, bool with_cnh, bool promote,
                          int resolution)
{
    char path[512];
    char *text;
    json_value *exp;
    const json_value *frames;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_vl53l8_live_frame *f = (depz_vl53l8_live_frame *)malloc(sizeof *f);
    char name[128];
    size_t i;

    CHECK(f);
    snprintf(path, sizeof path, "%s/recordings/%s.expected.json", DEPZ_VECTORS_DIR, stem);
    text = slurp(path);
    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    frames = json_obj_get(exp, "frames");
    snprintf(path, sizeof path, "%s/recordings/%s.depzrec", DEPZ_VECTORS_DIR, stem);
    CHECK_RC(depz_link_open_replay(path, true, false, &link), DEPZ_OK);
    if (promote) {
        /* Over a bare link there is no USB PID: promote picks the CX base. */
        CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
        depz_device_set_timeout_ms(dev, 2000);
        CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    } else {
        CHECK_RC(depz_vl53l8_open_link(link, model, &dev), DEPZ_OK);
        depz_device_set_timeout_ms(dev, 2000);
    }
    CHECK(depz_is_vl53l8(dev) && depz_vl53l8_get_model(dev) == model);
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_vl53l8_init(dev, NULL, NULL), DEPZ_OK);
    if (json_obj_get(exp, "module_type"))
        CHECK(depz_vl53l8_module_type(dev) == json_as_int(json_obj_get(exp, "module_type")));
    CHECK_RC(depz_vl53l8_set_resolution(dev, resolution), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(dev, 15), DEPZ_OK);
    if (with_cnh) {
        depz_vl53l8_cnh_setup cnh;
        depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4), DEPZ_OK);
        CHECK_RC(depz_vl53l8_configure_cnh(dev, &cnh), DEPZ_OK);
    }
    s = depz_vl53l8_frames(dev, json_arr_size(frames) + 8);
    CHECK(s);
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_OK);
    for (i = 0; i < json_arr_size(frames); i++) {
        const json_value *w = json_arr_get(frames, i);
        CHECK_RC(depz_stream_next(s, f, 5000), DEPZ_OK);
        CHECK(f->f.timestamp_us == (uint64_t)json_as_int(json_obj_get(w, "timestamp_us")));
        CHECK(f->f.resolution == json_as_int(json_obj_get(w, "resolution")));
        CHECK(f->f.silicon_temp_degc == json_as_int(json_obj_get(w, "silicon_temp_degc")));
        CHECK(same_ints(json_obj_get(w, "distance_mm"), f->f.distance_mm, f->f.resolution));
        CHECK(same_u8s(json_obj_get(w, "target_status"), f->f.target_status, f->f.resolution));
        CHECK(same_u8s(json_obj_get(w, "nb_target_detected"), f->f.nb_target_detected, f->f.resolution));
        if (with_cnh) CHECK(same_hex(json_as_str(json_obj_get(w, "cnh_raw")), f->cnh, f->cnh_len));
        else CHECK(f->cnh_len == 0);
    }
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);
    CHECK(depz_vl53l8_frame_parse_errors(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
    free(f);
}

/* Guards and the CNH setup math (no device answers: none is needed). */
static void l8_guards_and_cnh_math(void)
{
    depz_link *a, *b;
    depz_device *dev;
    depz_vl53l8_cnh_setup cnh;
    size_t bytes = 0;
    uint8_t packed[156];
    int zones;
    CHECK_RC(depz_link_loopback_pair(&a, &b), DEPZ_OK);
    CHECK_RC(depz_vl53l8_open_link(a, DEPZ_VL53L8_MODEL_L8CX, &dev), DEPZ_OK);
    CHECK(depz_is_vl53l8(dev) && !depz_vl53l8_initialized(dev) && !depz_vl53l8_ranging(dev));
    CHECK_RC(depz_vl53l8_get_resolution(dev, &zones), DEPZ_E_ARG);      /* init() first */
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(dev, 1), DEPZ_E_ARG); /* L8 needs >= 2 Hz */
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);                   /* idempotent */
    CHECK_RC(depz_sr04_start(dev), DEPZ_E_WRONG_TYPE);
    /* 16 aggregates x 20 bins -> 1708 B, the size the L8CH capture streams. */
    depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
    CHECK_RC(depz_vl53l8_cnh_required_memory(&cnh, &bytes), DEPZ_E_ARG); /* no agg map yet */
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4), DEPZ_OK);
    CHECK_RC(depz_vl53l8_cnh_required_memory(&cnh, &bytes), DEPZ_OK);
    CHECK(bytes == 1708 && cnh.nb_of_aggregates == 16 && cnh.map_id[0] == 0 && cnh.map_id[63] == 15);
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 5, 4), DEPZ_E_ARG);
    depz_vl53l8_cnh_pack(&cnh, packed);
    CHECK(packed[0] == 0x00 && packed[1] == 0x50 && packed[18] == 2 && packed[19] == 20 && packed[25] == 0x3F);
    depz_device_close(dev);
    depz_link_free(b);
}

static void l8_replay(const char *stem, depz_vl53l8_model model, bool with_cnh, bool promote)
{
    l8_replay_res(stem, model, with_cnh, promote, 64);
}

static void l8cx_replay(void) { l8_replay("vl53l8_8x8_15hz_3s", DEPZ_VL53L8_MODEL_L8CX, false, true); }
/* L5/L7: promote reads the device name to pick the class (contract 11 §1). */
static void l5cx_replay(void) { l8_replay("vl53l5cx_8x8_15hz_3s", DEPZ_VL53L8_MODEL_L5CX, false, true); }
static void l5cx_4x4_replay(void) { l8_replay_res("vl53l5cx_4x4_15hz", DEPZ_VL53L8_MODEL_L5CX, false, true, 16); }
static void l7ch_replay(void) { l8_replay("vl53l7ch_8x8_15hz_3s", DEPZ_VL53L8_MODEL_L7CH, false, true); }
static void l7ch_cnh_replay(void) { l8_replay("vl53l7ch_cnh_8x8_15hz", DEPZ_VL53L8_MODEL_L7CH, true, true); }
static void l8ch_replay(void) { l8_replay("vl53l8ch_8x8_15hz_3s", DEPZ_VL53L8_MODEL_L8CH, false, false); }
static void l8ch_cnh_replay(void) { l8_replay("vl53l8ch_cnh_8x8_15hz", DEPZ_VL53L8_MODEL_L8CH, true, false); }

/* ── BNO055 (real captures: reset, configure, profile, status, stream) ──── */

static void bno_replay(const char *stem)
{
    char path[512], name[128];
    char *text;
    json_value *exp;
    const json_value *info, *prof, *sys, *frames, *block;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_bno055_info bi;
    depz_bno055_units u;
    depz_bno055_calib_profile cp;
    depz_bno055_status_regs sr;
    depz_bno055_sample m;
    size_t i;
    int k;

    snprintf(path, sizeof path, "%s/recordings/%s.expected.json", DEPZ_VECTORS_DIR, stem);
    text = slurp(path);
    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    info = json_obj_get(exp, "info");
    prof = json_obj_get(exp, "calibration_profile");
    sys = json_obj_get(exp, "system_status");
    frames = json_obj_get(exp, "frames");
    block = json_obj_get(exp, "block");
    depz_bno055_unpack_units((uint8_t)json_as_int(json_obj_get(exp, "unit_sel")), &u);
    snprintf(path, sizeof path, "%s/recordings/%s.depzrec", DEPZ_VECTORS_DIR, stem);
    CHECK_RC(depz_link_open_replay(path, true, false, &link), DEPZ_OK);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 2000);
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_bno055(dev));
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_device_get_device_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "device_name"))) == 0);
    CHECK_RC(depz_bno055_bridge_info(dev, &bi), DEPZ_OK);
    CHECK(bi.chip_id == json_as_int(json_obj_get(info, "chip_id")) && bi.sw_rev == json_as_int(json_obj_get(info, "sw_rev")));
    CHECK(bi.read_avg_us == json_as_int(json_obj_get(info, "read_avg_us")) && bi.initialized == 1);
    CHECK_RC(depz_bno055_reset_sensor(dev), DEPZ_OK);
    CHECK_RC(depz_bno055_configure(dev, (uint8_t)json_as_int(json_obj_get(exp, "mode")), &u, NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_bno055_read_calibration_profile(dev, &cp), DEPZ_OK);
    for (k = 0; k < 3; k++) {
        CHECK(cp.accel_offset[k] == json_as_int(json_arr_get(json_obj_get(prof, "accel_offset"), (size_t)k)));
        CHECK(cp.gyro_offset[k] == json_as_int(json_arr_get(json_obj_get(prof, "gyro_offset"), (size_t)k)));
    }
    CHECK(cp.accel_radius == json_as_int(json_obj_get(prof, "accel_radius")));
    CHECK(cp.mag_radius == json_as_int(json_obj_get(prof, "mag_radius")));
    CHECK_RC(depz_bno055_system_status(dev, &sr), DEPZ_OK);
    CHECK(sr.self_test == json_as_int(json_obj_get(sys, "self_test")) && sr.status == json_as_int(json_obj_get(sys, "status")));
    s = depz_bno055_samples(dev, json_arr_size(frames) + 8);
    CHECK(s);
    CHECK_RC(depz_bno055_start_stream(dev, (uint16_t)json_as_int(json_obj_get(exp, "period_ms")),
                                      (uint8_t)json_as_int(json_arr_get(block, 0)),
                                      (uint8_t)json_as_int(json_arr_get(block, 1)), DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    for (i = 0; i < json_arr_size(frames); i++) {
        const json_value *w = json_arr_get(frames, i);
        CHECK_RC(depz_stream_next(s, &m, 2000), DEPZ_OK);
        CHECK(m.timestamp_us == (uint64_t)json_as_int(json_obj_get(w, "timestamp_us")));
        CHECK(m.addr == json_as_int(json_obj_get(w, "addr")));
        CHECK(same_hex(json_as_str(json_obj_get(w, "raw")), m.raw, m.len));
        CHECK(m.present & DEPZ_BNO055_HAS_QUATERNION);
        CHECK(m.quaternion[0] != 0 || m.quaternion[1] != 0 || m.quaternion[2] != 0 || m.quaternion[3] != 0);
        CHECK(memcmp(&m.units, &u, sizeof u) == 0);
    }
    CHECK_RC(depz_bno055_stop_stream(dev), DEPZ_OK);
    CHECK(depz_bno055_stream_parse_errors(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
}

static void bno_imu_replay(void) { bno_replay("bno055_imu_quat_units_50hz"); }
static void bno_ndof_replay(void) { bno_replay("bno055_ndof_full_100hz"); }

/* ── BNO085 / BNO086 over a real capture ─────────────────────────────── */

/* A report field by its Python dataclass name; false for an unknown name. */
static bool bno086_field(const depz_bno_report *r, const char *k, int64_t *v)
{
    static const struct { const char *name; size_t off; int kind; } f[] = {
#define F32(n) {#n, offsetof(depz_bno_report, n), 0}
        F32(x_raw), F32(y_raw), F32(z_raw), F32(bias_x_raw), F32(bias_y_raw), F32(bias_z_raw),
        F32(i_raw), F32(j_raw), F32(k_raw), F32(real_raw), F32(vx_raw), F32(vy_raw), F32(vz_raw),
        F32(delay_us), F32(accuracy_raw),
#undef F32
    };
    size_t i;
    if (!strcmp(k, "sensor_id")) { *v = r->sensor_id; return true; }
    if (!strcmp(k, "timestamp_us")) { *v = r->timestamp_us; return true; }
    if (!strcmp(k, "seq")) { *v = r->seq; return true; }
    if (!strcmp(k, "accuracy")) { *v = r->accuracy; return true; }
    for (i = 0; i < sizeof f / sizeof f[0]; i++) {
        if (!strcmp(k, f[i].name)) {
            *v = *(const int32_t *)((const char *)r + f[i].off);
            return true;
        }
    }
    return false;
}

static void bno086_session_replay(void)
{
    char path[1024], name[128];
    char *text;
    json_value *exp;
    const json_value *pid, *en, *feats, *reps, *cal, *counts, *meta;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_bno_product_id p;
    depz_bno_feature g;
    depz_bno086_calibration c;
    depz_bno086_counts n;
    depz_bno086_error_record errs[8];
    depz_bno_metadata md, want;
    uint32_t words[16];
    uint8_t osc;
    size_t i, k, nerr;

    snprintf(path, sizeof path, "%s/recordings/bno086_session.expected.json", DEPZ_VECTORS_DIR);
    text = slurp(path);
    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    pid = json_obj_get(exp, "product_id");
    en = json_obj_get(exp, "enable");
    feats = json_obj_get(exp, "features");
    reps = json_obj_get(exp, "reports");
    cal = json_obj_get(exp, "calibration");
    counts = json_obj_get(exp, "counts_rv");
    meta = json_obj_get(exp, "metadata_rv_words");
    snprintf(path, sizeof path, "%s/recordings/bno086_session.depzrec", DEPZ_VECTORS_DIR);
    CHECK_RC(depz_link_open_replay(path, true, false, &link), DEPZ_OK);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 2000);
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_bno086(dev));
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_device_get_device_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "device_name"))) == 0);
    CHECK_RC(depz_bno086_hardware_reset(dev, -1), DEPZ_OK);
    CHECK_RC(depz_bno086_product_id(dev, &p), DEPZ_OK);
    CHECK(p.sw_part_number == (uint32_t)json_as_int(json_obj_get(pid, "sw_part_number")));
    CHECK(p.sw_build_number == (uint32_t)json_as_int(json_obj_get(pid, "sw_build_number")));
    CHECK(p.sw_version_major == json_as_int(json_obj_get(pid, "sw_version_major")));
    CHECK(p.sw_version_patch == json_as_int(json_obj_get(pid, "sw_version_patch")));
    CHECK(p.reset_cause == json_as_int(json_obj_get(pid, "reset_cause")));
    s = depz_bno086_reports(dev, json_arr_size(reps) + 512);
    CHECK(s);
    for (i = 0; i < json_arr_size(en); i++) {
        const json_value *e = json_arr_get(en, i), *f = json_arr_get(feats, i);
        uint8_t sid = (uint8_t)json_as_int(json_arr_get(e, 0));
        CHECK_RC(depz_bno086_enable(dev, sid, (double)json_as_int(json_arr_get(e, 1)), &g), DEPZ_OK);
        CHECK(g.sensor_id == json_as_int(json_obj_get(f, "sensor_id")));
        CHECK(g.interval_us == (uint32_t)json_as_int(json_obj_get(f, "interval_us")));
        CHECK(g.batch_us == (uint32_t)json_as_int(json_obj_get(f, "batch_us")));
        CHECK(depz_bno086_rate_ok((uint32_t)(1000000 / json_as_int(json_arr_get(e, 1))), &g));
    }
    for (i = 0; i < json_arr_size(reps); i++) {
        const json_value *w = json_arr_get(reps, i);
        depz_bno_report r;
        CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
        CHECK(strcmp(depz_bno_report_type_str(r.type), json_as_str(json_obj_get(w, "type"))) == 0);
        for (k = 0; k < w->count; k++) {
            int64_t v;
            if (!strcmp(w->keys[k], "type")) continue;
            if (!strcmp(w->keys[k], "accuracy_raw") && json_is_null(w->items[k])) {
                CHECK(!r.has_accuracy_raw);
                continue;
            }
            CHECK(bno086_field(&r, w->keys[k], &v));
            if (v != json_as_int(w->items[k])) {
                fprintf(stderr, "  report %zu field %s: %lld != %lld\n", i, w->keys[k], (long long)v,
                        (long long)json_as_int(w->items[k]));
                CHECK(0);
            }
        }
    }
    for (i = 0; i < json_arr_size(en); i++)
        CHECK_RC(depz_bno086_disable(dev, (uint8_t)json_as_int(json_arr_get(json_arr_get(en, i), 0))), DEPZ_OK);
    CHECK_RC(depz_bno086_get_calibration(dev, &c), DEPZ_OK);
    CHECK(c.accel == json_as_bool(json_obj_get(cal, "accel")) && c.gyro == json_as_bool(json_obj_get(cal, "gyro")));
    CHECK(c.mag == json_as_bool(json_obj_get(cal, "mag")) && c.planar == json_as_bool(json_obj_get(cal, "planar")));
    CHECK_RC(depz_bno086_get_oscillator_type(dev, &osc), DEPZ_OK);
    CHECK(osc == json_as_int(json_obj_get(exp, "oscillator")));
    CHECK_RC(depz_bno086_get_metadata(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, &md), DEPZ_OK);
    for (i = 0; i < json_arr_size(meta) && i < 16; i++) words[i] = (uint32_t)json_as_int(json_arr_get(meta, i));
    depz_bno_metadata_from_words(words, json_arr_size(meta), &want);
    CHECK(memcmp(&md, &want, sizeof md) == 0);
    CHECK(md.revision == 4 && md.q_point_1 == 14 && md.q_point_2 == 12 && md.min_period_us == 1000);
    CHECK_RC(depz_bno086_get_counts(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, &n), DEPZ_OK);
    CHECK(n.offered == (uint32_t)json_as_int(json_obj_get(counts, "offered")));
    CHECK(n.accepted == (uint32_t)json_as_int(json_obj_get(counts, "accepted")));
    CHECK(n.on == (uint32_t)json_as_int(json_obj_get(counts, "on")));
    CHECK(n.attempted == (uint32_t)json_as_int(json_obj_get(counts, "attempted")));
    CHECK_RC(depz_bno086_get_errors(dev, 0, errs, 8, &nerr), DEPZ_OK);
    CHECK(nerr == json_arr_size(json_obj_get(exp, "errors")));
    CHECK(depz_bno086_shtp_discarded(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
}

/* ── BNO085 / BNO086 against the fake hub ────────────────────────────── */

/* A BNO086 on the fake hub, reset; NULL (and *f cleaned up) on failure. */
static depz_device *bno086_fake_open(fake_bno086 **f)
{
    depz_link *link;
    depz_device *dev = NULL;
    *f = fake_bno086_start(&link);
    if (!*f) return NULL;
    if (depz_bno086_open_link(link, &dev) == DEPZ_OK && depz_bno086_hardware_reset(dev, -1) == DEPZ_OK) return dev;
    depz_device_close(dev);
    fake_bno086_stop(*f);
    return NULL;
}

static void bno086_features(void)
{
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_bno_product_id p;
    depz_bno_feature g;
    depz_bno086_feature_request req;
    CHECK(dev);
    CHECK(depz_is_bno086(dev) && !depz_is_bno055(dev));
    CHECK_RC(depz_bno086_product_id(dev, &p), DEPZ_OK);
    CHECK(p.sw_part_number == FAKE_BNO086_PART && p.sw_version_major == 3 && p.sw_version_patch == 5);
    CHECK_RC(depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, 100, &g), DEPZ_OK);
    CHECK(g.sensor_id == DEPZ_BNO_SENSOR_ROTATION_VECTOR && g.interval_us == 8000);
    CHECK(depz_bno086_rate_ok(10000, &g));
    CHECK(fake_bno086_interval(f, DEPZ_BNO_SENSOR_ROTATION_VECTOR) == 8000);
    memset(&req, 0, sizeof req);
    req.interval_us = 3000; /* 333 Hz -> the 500 Hz grid step: 1.5x, fine */
    CHECK_RC(depz_bno086_enable_ex(dev, DEPZ_BNO_SENSOR_ACCELEROMETER, &req, &g), DEPZ_OK);
    CHECK(g.interval_us == 2000 && depz_bno086_rate_ok(3000, &g));
    req.interval_us = 7000; /* -> 4000: 1.75x */
    CHECK_RC(depz_bno086_enable_ex(dev, DEPZ_BNO_SENSOR_GYROSCOPE, &req, NULL), DEPZ_OK);
    CHECK_RC(depz_bno086_get_feature(dev, DEPZ_BNO_SENSOR_GYROSCOPE, &g), DEPZ_OK);
    CHECK(g.interval_us == 4000);
    g.interval_us = 1000;
    CHECK(!depz_bno086_rate_ok(10000, &g)); /* 10x the request: outside the band */
    CHECK_RC(depz_bno086_disable(dev, DEPZ_BNO_SENSOR_GYROSCOPE), DEPZ_OK);
    CHECK_RC(depz_bno086_get_feature(dev, DEPZ_BNO_SENSOR_GYROSCOPE, &g), DEPZ_OK);
    CHECK(g.interval_us == 0);
    CHECK_RC(depz_bno086_enable(dev, 1, 0, NULL), DEPZ_E_ARG);
    req.interval_us = 0;
    CHECK_RC(depz_bno086_enable_ex(dev, 1, &req, NULL), DEPZ_E_ARG);
    /* A disable's answer still in flight when the next enable reads back. */
    fake_bno086_delay_disable_answer(f, true);
    CHECK_RC(depz_bno086_disable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR), DEPZ_OK);
    CHECK_RC(depz_bno086_enable(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, 50, &g), DEPZ_OK);
    CHECK(g.interval_us == 16000);
    depz_device_close(dev);
    fake_bno086_stop(f);
}

static void bno086_busy_retry(void)
{
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_bno_feature g;
    CHECK(dev);
    fake_bno086_busy_next(f, 2);
    CHECK_RC(depz_bno086_enable(dev, DEPZ_BNO_SENSOR_GRAVITY, 100, &g), DEPZ_OK);
    CHECK(fake_bno086_frames_busy(f) == 2 && g.interval_us == 8000);
    fake_bno086_busy_next(f, DEPZ_BNO086_BUSY_RETRIES);
    CHECK_RC(depz_bno086_disable(dev, DEPZ_BNO_SENSOR_GRAVITY), DEPZ_E_BUSY);
    CHECK(fake_bno086_frames_busy(f) == 2 + DEPZ_BNO086_BUSY_RETRIES);
    CHECK(fake_bno086_interval(f, DEPZ_BNO_SENSOR_GRAVITY) == 8000);
    depz_device_close(dev);
    fake_bno086_stop(f);
}

typedef struct { int n; uint8_t last_sensor; } bno086_cb_count;

static void bno086_count_cb(const depz_bno_report *r, void *user)
{
    bno086_cb_count *c = (bno086_cb_count *)user;
    c->n++;
    c->last_sensor = r->sensor_id;
}

static void bno086_reports(void)
{
    /* 0xFB base delta 20 ticks, a rotation vector (delay 3 ticks) and an
     * accelerometer; then a dense gyro-integrated RV on channel 5. */
    static const uint8_t cargo[] = {
        0xFB, 20, 0, 0, 0,
        0x05, 9, 0x03, 3, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
        0x01, 10, 0x02, 0, 0x00, 0x01, 0x00, 0xFF, 0x80, 0x09};
    static const uint8_t gyro_rv[] = {0, 0, 0, 0, 0, 0, 0x00, 0x40, 0x00, 0x04, 0, 0, 0, 0xFC};
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_stream *s = depz_bno086_reports(dev, 16);
    depz_bno_report r;
    bno086_cb_count cc = {0, 0};
    double v[4], acc;
    int token;
    CHECK(dev);
    CHECK(s);
    CHECK_RC(depz_bno086_on_report(dev, bno086_count_cb, &cc, &token), DEPZ_OK);
    fake_bno086_send_input(f, DEPZ_SHTP_CH_INPUT_NORMAL, 1000000, cargo, sizeof cargo);
    fake_bno086_send_input(f, DEPZ_SHTP_CH_GYRO_RV, 2000000, gyro_rv, sizeof gyro_rv);
    CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
    CHECK(r.type == DEPZ_BNO_ROTATION_VECTOR && r.sensor_id == 5 && r.seq == 9 && r.accuracy == 3);
    CHECK(r.timestamp_us == 1000000 - 2000 + 300);
    depz_bno_report_quaternion(&r, v);
    CHECK(v[0] == 0.5 && v[3] == 0.0);
    CHECK(depz_bno_report_accuracy_rad(&r, &acc) && acc == 0.5);
    CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
    CHECK(r.type == DEPZ_BNO_ACCELERATION && r.accuracy == 2);
    depz_bno_report_xyz(&r, v);
    CHECK(v[0] == 1.0 && v[1] == -1.0 && v[2] == 9.5);
    CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
    CHECK(r.type == DEPZ_BNO_GYRO_INTEGRATED_RV && r.timestamp_us == 2000000);
    depz_bno_report_quaternion(&r, v);
    CHECK(v[3] == 1.0);
    depz_bno_report_angular_velocity(&r, v);
    CHECK(v[0] == 1.0 && v[2] == -1.0);
    CHECK(cc.n == 3 && cc.last_sensor == DEPZ_BNO_SENSOR_GYRO_INTEGRATED_RV);
    depz_bno086_off_report(dev, token);
    fake_bno086_send_input(f, DEPZ_SHTP_CH_GYRO_RV, 3000000, gyro_rv, sizeof gyro_rv);
    /* get_report() subscribes when called: this one is already on its way. */
    CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
    CHECK(r.timestamp_us == 3000000 && cc.n == 3);
    CHECK_RC(depz_bno086_get_report(dev, 50, &r), DEPZ_E_TIMEOUT);
    CHECK(depz_bno086_shtp_discarded(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    fake_bno086_stop(f);
}

static void bno086_commands(void)
{
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_bno086_calibration c;
    depz_bno086_error_record e[4];
    depz_bno086_counts n;
    depz_bno_command_response r;
    uint8_t p[9], osc;
    size_t ne;
    CHECK(dev);
    CHECK_RC(depz_bno086_get_calibration(dev, &c), DEPZ_OK);
    CHECK(c.accel && !c.gyro && c.mag && !c.planar);
    CHECK_RC(depz_bno086_set_calibration(dev, true, true, false, true), DEPZ_OK);
    CHECK_RC(depz_bno086_get_calibration(dev, &c), DEPZ_OK);
    CHECK(c.accel && c.gyro && !c.mag && c.planar);
    fake_bno086_calibration_status(f, 3);
    CHECK_RC(depz_bno086_set_calibration(dev, false, false, false, false), DEPZ_E_STATUS);
    CHECK_RC(depz_bno086_tare_now(dev, DEPZ_BNO_TARE_Z, DEPZ_BNO_TARE_BASIS_GAME_RV), DEPZ_OK);
    CHECK_RC(depz_bno086_get_oscillator_type(dev, &osc), DEPZ_OK); /* the tare went first */
    CHECK(osc == DEPZ_BNO_OSC_EXT_CRYSTAL);
    CHECK(fake_bno086_last_command(f, DEPZ_SH2_CMD_TARE, p) && p[0] == 0 && p[1] == 4 && p[2] == 1);
    CHECK_RC(depz_bno086_set_reorientation(dev, 0, 0, -0.5, 1.0), DEPZ_OK);
    CHECK_RC(depz_bno086_save_dcd(dev), DEPZ_OK);
    CHECK(fake_bno086_last_command(f, DEPZ_SH2_CMD_TARE, p));
    CHECK(p[0] == 2 && p[1] == 0 && p[3] == 0 && p[5] == 0x00 && p[6] == 0xE0 && p[7] == 0x00 && p[8] == 0x40);
    CHECK_RC(depz_bno086_set_reorientation(dev, 2.0, 0, 0, 0), DEPZ_E_ARG);
    CHECK_RC(depz_bno086_persist_tare(dev), DEPZ_OK);
    CHECK_RC(depz_bno086_configure_periodic_dcd(dev, false), DEPZ_OK);
    CHECK_RC(depz_bno086_get_errors(dev, 0, e, 4, &ne), DEPZ_OK);
    CHECK(ne == 1 && e[0].severity == 2 && e[0].source == 3 && e[0].code == 0x33);
    CHECK(fake_bno086_last_command(f, DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG, p) && p[0] == 1);
    CHECK(fake_bno086_last_command(f, DEPZ_SH2_CMD_TARE, p) && p[0] == 1);
    CHECK_RC(depz_bno086_get_counts(dev, 5, &n), DEPZ_OK);
    CHECK(n.sensor_id == 5 && n.offered == 105 && n.accepted == 95 && n.on == 85 && n.attempted == 75);
    CHECK_RC(depz_bno086_clear_counts(dev, 5), DEPZ_OK);
    CHECK_RC(depz_bno086_command(dev, DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE, NULL, 0, &r), DEPZ_OK);
    CHECK(r.command == DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE && r.r[0] == 1);
    CHECK_RC(depz_bno086_command(dev, 0x55, p, 10, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_bno086_wake(dev), DEPZ_OK);
    CHECK_RC(depz_bno086_clear_dcd_and_reset(dev, -1), DEPZ_OK);
    CHECK_RC(depz_bno086_get_oscillator_type(dev, &osc), DEPZ_OK);
    depz_device_close(dev);
    fake_bno086_stop(f);
}

static void bno086_frs(void)
{
    /* The lab BNO085's rotation-vector metadata record. */
    static const uint32_t rv_meta[10] = {65537, 16384, 1, 267573, 1000, 65538, 14, 786446, 851968, 10000};
    static const uint32_t orient[4] = {0, 0, 0x2D413CCDu, 0x2D413CCDu};
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_bno_metadata m;
    uint32_t w[8];
    size_t n = 0;
    CHECK(dev);
    fake_bno086_set_record(f, 0xE30B, rv_meta, 10);
    CHECK_RC(depz_bno086_get_metadata(dev, DEPZ_BNO_SENSOR_ROTATION_VECTOR, &m), DEPZ_OK);
    CHECK(m.revision == 4 && m.power_ma_q10 == 5429 && m.min_period_us == 1000 && m.max_period_us == 10000);
    CHECK(m.range_raw == 16384 && m.q_point_1 == 14 && m.q_point_2 == 12 && m.q_point_3 == 13);
    CHECK(m.me_version == 1 && m.sh_version == 1 && m.batch_buffer_bytes == 14);
    CHECK_RC(depz_bno086_get_metadata(dev, 0x0A, &m), DEPZ_E_ARG);         /* no record known */
    CHECK_RC(depz_bno086_get_metadata(dev, DEPZ_BNO_SENSOR_GRAVITY, &m), DEPZ_E_STATUS); /* unrecognised */
    CHECK_RC(depz_bno086_frs_write(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, orient, 4), DEPZ_OK);
    CHECK(fake_bno086_record(f, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, w, 8) == 4 && w[2] == orient[2]);
    CHECK_RC(depz_bno086_frs_read(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, w, 8, &n), DEPZ_OK);
    CHECK(n == 4 && memcmp(w, orient, sizeof orient) == 0);
    CHECK_RC(depz_bno086_frs_read(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, w, 3, &n), DEPZ_E_ARG);
    CHECK(n == 4);
    CHECK_RC(depz_bno086_frs_write(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, NULL, 0), DEPZ_OK); /* erase */
    CHECK_RC(depz_bno086_frs_read(dev, DEPZ_BNO_FRS_SYSTEM_ORIENTATION, w, 8, &n), DEPZ_E_STATUS); /* empty */
    depz_device_close(dev);
    fake_bno086_stop(f);
}

static void bno086_codecs(void)
{
    static const uint8_t fc[17] = {0xFC, 5, 1, 2, 0, 0x10, 0x27, 0, 0, 1, 0, 0, 0, 7, 0, 0, 0};
    depz_bno_feature g;
    depz_bno_report r;
    double v[3];
    CHECK(depz_bno_unpack_feature_response(fc, 17, &g) == 0);
    CHECK(g.sensor_id == 5 && g.flags == 1 && g.sensitivity == 2 && g.interval_us == 10000 && g.batch_us == 1 &&
          g.cfg_word == 7);
    CHECK(depz_bno_unpack_feature_response(fc, 16, &g) == -1);
    CHECK(depz_bno_metadata_record(0x2A) == 0xE324 && depz_bno_metadata_record(0x0B) == 0);
    CHECK(depz_bno_q_point(0x02) == 9 && depz_bno_q_point(0x10) == -1 && depz_bno_q_point(0x0A) == 20);
    memset(&r, 0, sizeof r);
    r.sensor_id = 0x0F;
    r.bias_x_raw = 32;
    depz_bno_report_bias(&r, v);
    CHECK(v[0] == 2.0);
    r.sensor_id = 0x0E;
    r.value_raw = -256;
    CHECK(depz_bno_report_scalar(&r) == -2.0);
}

/* ── the VL53L 1D family over real captures ──────────────────────────── */

static depz_vl53lx_driver vlx_kind(const char *s)
{
    if (!strcmp(s, "uld")) return DEPZ_VL53LX_DRIVER_ULD;
    if (!strcmp(s, "ulp")) return DEPZ_VL53LX_DRIVER_ULP;
    return DEPZ_VL53LX_DRIVER_HISTOGRAM;
}

static void vlx_replay(const char *stem)
{
    char path[1024], name[64];
    char *text;
    json_value *exp;
    const json_value *frames, *timing, *parg, *mode;
    depz_link *link;
    depz_device *dev;
    depz_stream *s;
    depz_vl53lx_measurement m;
    int budget, inter;
    size_t i, k;

    snprintf(path, sizeof path, "%s/recordings/%s.expected.json", DEPZ_VECTORS_DIR, stem);
    text = slurp(path);
    CHECK(text);
    exp = json_parse(text);
    free(text);
    CHECK(exp);
    frames = json_obj_get(exp, "frames");
    timing = json_obj_get(exp, "timing");
    parg = json_obj_get(exp, "product_arg");
    mode = json_obj_get(exp, "mode");
    snprintf(path, sizeof path, "%s/recordings/%s.depzrec", DEPZ_VECTORS_DIR, stem);
    CHECK_RC(depz_link_open_replay(path, true, false, &link), DEPZ_OK);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 2000);
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_vl53lx(dev));
    CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
    CHECK(strcmp(name, json_as_str(json_obj_get(exp, "software_name"))) == 0);
    CHECK_RC(depz_vl53lx_init(dev, vlx_kind(json_as_str(json_obj_get(exp, "driver"))),
                              json_is_null(parg) ? DEPZ_VL53LX_PRODUCT_NONE
                                                 : depz_vl53lx_product_from_str(json_as_str(parg))), DEPZ_OK);
    CHECK(strcmp(depz_vl53lx_product_get(depz_vl53lx_product_bound(dev))->name,
                 json_as_str(json_obj_get(exp, "product"))) == 0);
    if (json_obj_get(exp, "refused")) {
        /* A configuration the driver no longer accepts (short on an L4 die):
         * init still replays, configure refuses the mode. */
        const json_value *after = json_obj_get(json_obj_get(exp, "refused"), "after_init");
        const json_value *want_modes = json_obj_get(after, "modes"), *want_budget = json_obj_get(after, "budget_ms");
        const char *names[8];
        int lo, hi;
        size_t n = depz_vl53lx_modes(dev, names, 8);
        CHECK(n == json_arr_size(want_modes));
        for (k = 0; k < n; k++) CHECK(strcmp(names[k], json_as_str(json_arr_get(want_modes, k))) == 0);
        CHECK_RC(depz_vl53lx_budget_range(dev, &lo, &hi), DEPZ_OK);
        CHECK(lo == json_as_int(json_arr_get(want_budget, 0)) && hi == json_as_int(json_arr_get(want_budget, 1)));
        CHECK(depz_vl53lx_driver_reach_mm(dev) == (uint32_t)json_as_int(json_obj_get(after, "driver_reach_mm")));
        CHECK_RC(depz_vl53lx_configure(dev, (int)json_as_int(json_obj_get(exp, "budget_ms")), 0, json_as_str(mode),
                                       NULL, NULL), DEPZ_E_ARG);
        depz_device_close(dev);
        json_free(exp);
        return;
    }
    if (!depz_vl53lx_supports(dev, DEPZ_VL53LX_CAP_SIGNAL_THRESH)) {
        /* A group the driver lacks is refused before the re-init: no traffic,
         * so the capture's own configure below still replays strictly. */
        const int kcps = 512;
        CHECK_RC(depz_vl53lx_configure_ex(dev, (int)json_as_int(json_obj_get(exp, "budget_ms")), 0,
                                          json_is_null(mode) ? NULL : json_as_str(mode), NULL, NULL, &kcps),
                 DEPZ_E_ARG);
    }
    CHECK_RC(depz_vl53lx_configure(dev, (int)json_as_int(json_obj_get(exp, "budget_ms")), 0,
                                   json_is_null(mode) ? NULL : json_as_str(mode), NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_vl53lx_get_range_timing(dev, &budget, &inter), DEPZ_OK);
    CHECK(budget == json_as_int(json_arr_get(timing, 0)) && inter == json_as_int(json_arr_get(timing, 1)));
    s = depz_vl53lx_measurements(dev, json_arr_size(frames) + 8);
    CHECK(s);
    CHECK_RC(depz_vl53lx_start_ranging(dev), DEPZ_OK);
    for (i = 0; i < json_arr_size(frames); i++) {
        const json_value *w = json_arr_get(frames, i), *t = json_obj_get(w, "targets"), *b = json_obj_get(w, "bins");
        CHECK_RC(depz_stream_next(s, &m, 3000), DEPZ_OK);
        if (m.timestamp_us != (uint64_t)json_as_int(json_obj_get(w, "timestamp_us")) ||
            m.distance_mm != json_as_int(json_obj_get(w, "distance_mm")) ||
            m.status != json_as_int(json_obj_get(w, "status")) || m.n_targets != json_arr_size(t)) {
            fprintf(stderr, "  frame %zu: %d mm st %d n %zu, want %lld mm st %lld n %zu\n", i, (int)m.distance_mm,
                    m.status, m.n_targets, (long long)json_as_int(json_obj_get(w, "distance_mm")),
                    (long long)json_as_int(json_obj_get(w, "status")), json_arr_size(t));
            CHECK(0);
        }
        for (k = 0; k < m.n_targets; k++) {
            const json_value *tk = json_arr_get(t, k);
            CHECK(m.targets[k].distance_mm == json_as_int(json_arr_get(tk, 0)));
            CHECK(m.targets[k].status == json_as_int(json_arr_get(tk, 1)));
        }
        if (b) {
            const json_value *bd = json_obj_get(b, "bin_data");
            CHECK(m.has_bins && m.bins.number_of_bins == json_arr_size(bd));
            for (k = 0; k < json_arr_size(bd); k++) CHECK(m.bins.bin_data[k] == json_as_int(json_arr_get(bd, k)));
            CHECK(m.bins.vcsel_period == json_as_int(json_obj_get(b, "vcsel_period")));
            CHECK(m.bins.stream_count == json_as_int(json_obj_get(b, "stream_count")));
        } else {
            CHECK(!m.has_bins);
        }
    }
    CHECK_RC(depz_vl53lx_stop_ranging(dev), DEPZ_OK);
    CHECK(depz_vl53lx_stream_parse_errors(dev) == 0);
    depz_stream_close(s);
    depz_device_close(dev);
    json_free(exp);
}

static void vlx_l4cd_uld_replay(void) { vlx_replay("vl53l4cx_uld_as_l4cd_50ms"); }
static void vlx_l1cx_uld_replay(void) { vlx_replay("vl53l1cx_uld_long_33ms"); }
static void vlx_l1cb_uld_replay(void) { vlx_replay("vl53l1cb_uld_short_50ms"); }
static void vlx_l3cx_ulp_replay(void) { vlx_replay("vl53l3cx_ulp_33ms"); }
static void vlx_l0x_uld_replay(void) { vlx_replay("vl53l0x_uld_long-range_33ms"); }
static void vlx_l4cx_hist_medium_replay(void) { vlx_replay("vl53l4cx_histogram_medium_50ms"); }
static void vlx_l4cx_hist_short_replay(void) { vlx_replay("vl53l4cx_histogram_short_33ms"); }
static void vlx_l1cx_hist_replay(void) { vlx_replay("vl53l1cx_histogram_long_33ms"); }
static void vlx_l1cb_hist_replay(void) { vlx_replay("vl53l1cb_histogram_medium_33ms"); }
static void vlx_l3cx_hist_replay(void) { vlx_replay("vl53l3cx_histogram_medium_33ms"); }

/* ── runner ──────────────────────────────────────────────────────────── */

typedef struct { const char *name; void (*rig_fn)(rig *); void (*plain_fn)(void); } test_case;

static const test_case k_tests[] = {
    {"identity_roundtrip", identity_roundtrip, NULL},
    {"temperature", temperature, NULL},
    {"sync_time_produces_offset", sync_time_produces_offset, NULL},
    {"unknown_cmd_raises_status_error", unknown_cmd_raises_status_error, NULL},
    {"sync_pin_validation", sync_pin_validation, NULL},
    {"timeout_when_device_silent", NULL, timeout_when_device_silent},
    {"wrong_type_is_refused", NULL, wrong_type_is_refused},
    {"sample_period_readback_is_stored", sample_period_readback_is_stored, NULL},
    {"echo_decay_clamps_both_ends", echo_decay_clamps_both_ends, NULL},
    {"measure_once_is_source_once", measure_once_is_source_once, NULL},
    {"loop_samples_are_source_loop", loop_samples_are_source_loop, NULL},
    {"start_stop_idempotent", start_stop_idempotent, NULL},
    {"measure_once_busy_during_loop", measure_once_busy_during_loop, NULL},
    {"no_echo_timeout_sentinel", no_echo_timeout_sentinel, NULL},
    {"temperature_compensated_distance", temperature_compensated_distance, NULL},
    {"stream_drop_oldest_counter", stream_drop_oldest_counter, NULL},
    {"two_independent_streams", two_independent_streams, NULL},
    {"sync_in_single_shot_reaches_stream", sync_in_single_shot_reaches_stream, NULL},
    {"events_and_disconnect", NULL, events_and_disconnect},
    {"same_opcode_in_flight_is_busy", NULL, same_opcode_in_flight_is_busy},
    {"record_then_replay", NULL, record_then_replay},
    {"sr04_session_replay", NULL, sr04_session_replay},
    {"l4_init_boot_speed_then_retimes", NULL, l4_init_boot_speed_then_retimes},
    {"l4_init_at_400_never_retimes", NULL, l4_init_at_400_never_retimes},
    {"l4_init_config_block_and_vhv", NULL, l4_init_config_block_and_vhv},
    {"l4_range_timing_roundtrip_and_bounds", NULL, l4_range_timing_roundtrip_and_bounds},
    {"l4_tuning_roundtrips", NULL, l4_tuning_roundtrips},
    {"l4_is_alive_checks_model_id", NULL, l4_is_alive_checks_model_id},
    {"l4_config_refused_while_ranging", NULL, l4_config_refused_while_ranging},
    {"l4_stream_decodes_and_counts_short_blocks", NULL, l4_stream_decodes_and_counts_short_blocks},
    {"l4_measure_once_polls_and_stops", NULL, l4_measure_once_polls_and_stops},
    {"l4_session_replay", NULL, l4_session_replay},
    {"l8_guards_and_cnh_math", NULL, l8_guards_and_cnh_math},
    {"l8cx_replay", NULL, l8cx_replay},
    {"l8ch_replay", NULL, l8ch_replay},
    {"l8ch_cnh_replay", NULL, l8ch_cnh_replay},
    {"l5cx_replay", NULL, l5cx_replay},
    {"l5cx_4x4_replay", NULL, l5cx_4x4_replay},
    {"l7ch_replay", NULL, l7ch_replay},
    {"l7ch_cnh_replay", NULL, l7ch_cnh_replay},
    {"bno_imu_replay", NULL, bno_imu_replay},
    {"bno_ndof_replay", NULL, bno_ndof_replay},
    {"bno086_session_replay", NULL, bno086_session_replay},
    {"bno086_features", NULL, bno086_features},
    {"bno086_busy_retry", NULL, bno086_busy_retry},
    {"bno086_reports", NULL, bno086_reports},
    {"bno086_commands", NULL, bno086_commands},
    {"bno086_frs", NULL, bno086_frs},
    {"bno086_codecs", NULL, bno086_codecs},
    {"vlx_l4cd_uld_replay", NULL, vlx_l4cd_uld_replay},
    {"vlx_l1cx_uld_replay", NULL, vlx_l1cx_uld_replay},
    {"vlx_l1cb_uld_replay", NULL, vlx_l1cb_uld_replay},
    {"vlx_l3cx_ulp_replay", NULL, vlx_l3cx_ulp_replay},
    {"vlx_l0x_uld_replay", NULL, vlx_l0x_uld_replay},
    {"vlx_l4cx_hist_medium_replay", NULL, vlx_l4cx_hist_medium_replay},
    {"vlx_l4cx_hist_short_replay", NULL, vlx_l4cx_hist_short_replay},
    {"vlx_l1cx_hist_replay", NULL, vlx_l1cx_hist_replay},
    {"vlx_l1cb_hist_replay", NULL, vlx_l1cb_hist_replay},
    {"vlx_l3cx_hist_replay", NULL, vlx_l3cx_hist_replay},
};

int main(int argc, char **argv)
{
    size_t i, ran = 0;
    for (i = 0; i < sizeof k_tests / sizeof k_tests[0]; i++) {
        const test_case *t = &k_tests[i];
        int before = g_failed;
        if (argc > 1 && strcmp(argv[1], t->name) != 0) continue;
        ran++;
        if (t->rig_fn) {
            rig r;
            if (!rig_up(&r)) { fprintf(stderr, "  FAIL rig: %s\n", depz_last_error()); g_failed++; continue; }
            t->rig_fn(&r);
            rig_down(&r);
        } else {
            t->plain_fn();
        }
        printf("%s %s\n", g_failed == before ? "ok  " : "FAIL", t->name);
    }
    if (!ran) { fprintf(stderr, "no test named %s\n", argc > 1 ? argv[1] : "?"); return 2; }
    return g_failed ? 1 : 0;
}
