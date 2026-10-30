/* test_io.c — the live-hardware layer against a fake SR04 over loopback, and
 * record/replay round trips. Mirrors the Python SDK's test_device_core.py and
 * test_sr04_coverage.py. Usage: test_io [name] (no name = all). */
#include "depz_sensor_io.h"
#include "fake_sr04.h"
#include "fake_vl53l4.h"
#include "fake_vl53l8.h"
#include "fake_bno086.h"
#include "fake_tmf8829.h"
#include "fake_vl53lx.h"
#include "fake_bno055.h"
#include "fake_vl53l8.h"
#include "json.h"
#include "scenarios.h"

#include "../src/io/io_internal.h"
#include "../src/io/platform.h"
#include "../src/io/vl53lx_bare_tables.h"
#include "../src/io/vl53l8_blobs.h"

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

/* Contract 07 §3 (ERRATA A4): a reply shorter than its fields fails that one
 * request with DEPZ_E_PROTOCOL; the reader goes on and the next request is
 * answered — the device is not dead. */
static void short_reply_fails_its_request_and_the_reader_goes_on(rig *r)
{
    char s[128];
    double c = 0;
    fake_sr04_set_short_temperature(r->fake, true);
    CHECK_RC(depz_device_read_mcu_temperature(r->dev, &c), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "report 0x83: 3 bytes, expected 10") != NULL);
    CHECK_RC(depz_device_read_mcu_temperature(r->dev, &c), DEPZ_E_PROTOCOL);
    fake_sr04_set_short_temperature(r->fake, false);
    CHECK_RC(depz_device_get_device_name(r->dev, s, sizeof s), DEPZ_OK);
    CHECK(strcmp(s, FAKE_SR04_NAME) == 0);
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
    /* contract 07 §3: refused on the host, nothing sent */
    CHECK_RC(depz_device_set_sync_pin(r->dev, &c), DEPZ_E_ARG);
    CHECK_RC(depz_device_get_sync_pin(r->dev, 0, &got), DEPZ_E_ARG);
    {
        /* the device's own validation still answers ERR_INVALID_PARAM */
        const uint8_t raw[3] = {9, DEPZ_SYNC_PIN_IN, 0};
        CHECK_RC(depz_device_request(r->dev, DEPZ_CMD_SET_SYNC_PIN_CONFIG, raw, 3, NULL, NULL, true, -1),
                 DEPZ_E_STATUS);
        CHECK(depz_last_status() == DEPZ_STATUS_ERR_INVALID_PARAM);
    }
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
    {   /* other classes too: the type test comes before anything else */
        depz_tmf8829_info info;
        const char *modes[4];
        int lo, hi;
        CHECK_RC(depz_vl53lx_budget_range(NULL, &lo, &hi), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_budget_range(dev, &lo, &hi), DEPZ_E_WRONG_TYPE);
        CHECK(depz_vl53lx_modes(NULL, modes, 4) == 0 && depz_vl53lx_modes(dev, modes, 4) == 0);
        CHECK(!depz_vl53lx_supports(dev, 0) && !depz_vl53lx_initialized(dev));
        CHECK_RC(depz_tmf8829_bridge_info(NULL, &info), DEPZ_E_ARG);
        CHECK_RC(depz_tmf8829_bridge_info(dev, &info), DEPZ_E_WRONG_TYPE); /* no GET_INFO sent */
        CHECK_RC(depz_bno055_set_axis_placement(dev, "P9"), DEPZ_E_WRONG_TYPE);
    }
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

/* Contract 01 §5 step 5 (ERRATA A1): a gap the host sees in the device->host
 * seq is a SEQ_GAP event of its own, ahead of the packet it was seen on, and
 * counts in stats.seq_gaps; the first packet after open is no gap, and the
 * device-reported RPT 0x84 stays a SEQUENCE_ERROR. */
static void host_seq_gap_is_an_event(void)
{
    static const uint8_t seqs[3] = {5, 9, 10};
    static const uint8_t payload[1] = {0x00};
    depz_link *a, *b;
    depz_device *dev;
    depz_stream *evs;
    depz_device_event ev;
    depz_link_stats st;
    uint8_t frame[16];
    size_t i, frame_len;
    CHECK_RC(depz_link_loopback_pair(&a, &b), DEPZ_OK);
    CHECK_RC(depz_device_open_link(a, &dev), DEPZ_OK);
    evs = depz_device_events(dev, 16);
    CHECK(evs);
    for (i = 0; i < 3; i++) {
        CHECK_RC(depz_build_packet(0x7F, payload, 1, seqs[i], DEPZ_CRC_NONE, frame, sizeof frame, &frame_len), 0);
        CHECK_RC(depz_link_write(b, frame, frame_len), DEPZ_OK);
    }
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK); /* seq 5: the first packet, no gap */
    CHECK(ev.type == DEPZ_DEV_EV_TEXT);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_SEQ_GAP && ev.expected_seq == 6 && ev.received_seq == 9 && ev.missed == 3);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK); /* the packet is still delivered */
    CHECK(ev.type == DEPZ_DEV_EV_TEXT);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK); /* seq 10 follows 9: no gap */
    CHECK(ev.type == DEPZ_DEV_EV_TEXT);
    depz_device_stats(dev, &st);
    CHECK(st.seq_gaps == 1 && st.device_seq_errors == 0 && st.rx_packets == 3);
    CHECK(st.callback_errors == 0); /* contract 07 §3: a C callback cannot raise */
    depz_stream_close(evs);
    depz_device_close(dev);
    depz_link_free(b);
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

/* The device end of a bare loopback link, played by the test (Python's
 * `peer`): frames at consecutive seqs, raw bytes, and the next host command
 * read back with a bounded wait. */
typedef struct {
    depz_link  *side;
    depz_parser parser;
    uint8_t     seq;
    bool        got;
    uint8_t     cmd;
} peer;

static void peer_on_event(const depz_event *e, void *user)
{
    peer *p = (peer *)user;
    if (e->type == DEPZ_EV_PACKET && !p->got) {
        p->got = true;
        p->cmd = e->cmd;
    }
}

static bool peer_up(peer *p, depz_device **dev)
{
    depz_link *host;
    memset(p, 0, sizeof *p);
    *dev = NULL;
    depz_parser_init(&p->parser);
    if (depz_link_loopback_pair(&host, &p->side) != DEPZ_OK) return false;
    if (depz_device_open_link(host, dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(*dev, 1000);
    return true;
}

static void peer_down(peer *p, depz_device *dev)
{
    depz_device_close(dev);
    depz_link_free(p->side);
    depz_parser_free(&p->parser);
}

static void peer_send(peer *p, uint8_t cmd, const uint8_t *payload, size_t len)
{
    uint8_t frame[64];
    size_t n = 0;
    if (depz_build_packet(cmd, payload, len, p->seq, DEPZ_CRC_NONE, frame, sizeof frame, &n) == 0 &&
        depz_link_write(p->side, frame, n) == DEPZ_OK)
        p->seq = (uint8_t)(p->seq + 1);
}

/* The next host command is `cmd` (bounded: a second). */
static bool peer_expect(peer *p, uint8_t cmd)
{
    uint64_t deadline = depz_host_now_us() + 1000000u;
    uint8_t buf[256];
    p->got = false;
    while (!p->got && depz_host_now_us() < deadline) {
        size_t got = 0;
        if (depz_link_read(p->side, buf, sizeof buf, 50, &got) != DEPZ_OK) return false;
        if (got) depz_parser_feed(&p->parser, buf, got, peer_on_event, p);
    }
    return p->got && p->cmd == cmd;
}

/* A call waiting for the board on another thread when depz_device_close()
 * runs ends with the closed error (contract 07 §3, as Python); close frees
 * nothing until it has left (C-2: it destroyed the lock and freed the device
 * under the waiter — ASan / TSan). */
typedef struct { depz_device *dev; int rc; char err[128]; } closing_arg;

static void closing_request(void *a)
{
    closing_arg *x = (closing_arg *)a;
    x->rc = depz_device_request(x->dev, 0x5A, NULL, 0, NULL, NULL, true, 5000);
    depz_strlcpy(x->err, depz_last_error(), sizeof x->err);
}

/* Bounded: the request is in the device's pending list. */
static bool request_in_flight(depz_device *dev)
{
    uint64_t deadline = depz_host_now_us() + 2000000u;
    while (depz_host_now_us() < deadline) {
        bool in;
        depz_mutex_lock(&dev->lock);
        in = dev->pending != NULL;
        depz_mutex_unlock(&dev->lock);
        if (in) return true;
        depz_sleep_ms(1);
    }
    return false;
}

static void close_while_a_request_waits(void)
{
    peer p;
    depz_device *dev;
    depz_thread t;
    closing_arg a;
    uint64_t t0;
    CHECK(peer_up(&p, &dev));
    memset(&a, 0, sizeof a);
    a.dev = dev;
    a.rc = 1;
    t0 = depz_host_now_us();
    CHECK(depz_thread_start(&t, closing_request, &a) == 0);
    CHECK(peer_expect(&p, 0x5A) && request_in_flight(dev)); /* the peer stays silent */
    depz_device_close(dev);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_CLOSED && strstr(a.err, "device closed") != NULL);
    CHECK(depz_host_now_us() - t0 < 4000000u); /* not the 5 s timeout */
    depz_link_free(p.side);
    depz_parser_free(&p.parser);
}

static void closing_product_ids(void *a)
{
    closing_arg *x = (closing_arg *)a;
    depz_bno_product_id ids[4];
    size_t n = 0;
    x->rc = depz_bno086_product_ids(x->dev, ids, 4, &n);
    depz_strlcpy(x->err, depz_last_error(), sizeof x->err);
}

/* The same for a BNO086 control request: SEND_SHTP_PACKET is acknowledged,
 * the SH-2 answer never comes — close ends the wait for it. */
static void bno086_close_while_a_control_request_waits(void)
{
    static const uint8_t ack[2] = {DEPZ_BNO086_CMD_SEND_SHTP_PACKET, DEPZ_STATUS_OK};
    peer p;
    depz_link *host;
    depz_device *dev = NULL;
    depz_thread t;
    closing_arg a;
    uint64_t t0;
    memset(&p, 0, sizeof p);
    depz_parser_init(&p.parser);
    CHECK_RC(depz_link_loopback_pair(&host, &p.side), DEPZ_OK);
    CHECK_RC(depz_bno086_open_link(host, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 5000);
    memset(&a, 0, sizeof a);
    a.dev = dev;
    a.rc = 1;
    t0 = depz_host_now_us();
    CHECK(depz_thread_start(&t, closing_product_ids, &a) == 0);
    CHECK(peer_expect(&p, DEPZ_BNO086_CMD_SEND_SHTP_PACKET));
    peer_send(&p, DEPZ_RPT_STATUS, ack, sizeof ack);
    {   /* bounded: the ack has completed the request; the thread waits for the answer */
        uint64_t deadline = depz_host_now_us() + 2000000u;
        bool in = true;
        while (in && depz_host_now_us() < deadline) {
            depz_mutex_lock(&dev->lock);
            in = dev->pending != NULL;
            depz_mutex_unlock(&dev->lock);
            if (in) depz_sleep_ms(1);
        }
        CHECK(!in);
    }
    depz_device_close(dev);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_CLOSED && strstr(a.err, "device closed") != NULL);
    CHECK(depz_host_now_us() - t0 < 4000000u);
    depz_link_free(p.side);
    depz_parser_free(&p.parser);
}

/* Contract 07 §3: unsolicited reports become events, the callback and the
 * stream see the same ones; a report no class knows stays visible as hex
 * text; after off_event the stream is still fed (test_device_events.py). */
static void unsolicited_reports_become_events(void)
{
    static const uint8_t seqerr[2] = {7, 9};
    static const uint8_t fault[2] = {0x00, DEPZ_STATUS_ERR_HARDWARE_FAULT};
    static const uint8_t hello[7] = {0x04, 'h', 'e', 'l', 'l', 'o', 0x00};
    static const uint8_t again[7] = {0x04, 'a', 'g', 'a', 'i', 'n', 0x00};
    static const uint8_t unknown[2] = {0x01, 0x02};
    /* <Qh 123456, 251: 25.1 °C */
    static const uint8_t temp[10] = {0x40, 0xE2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0x00};
    peer p;
    depz_device *dev;
    depz_stream *evs;
    depz_device_event ev;
    depz_link_stats st;
    ev_count c;
    int tok = -1, seen;
    CHECK(peer_up(&p, &dev));
    memset(&c, 0, sizeof c);
    depz_mutex_init(&c.lock);
    CHECK_RC(depz_device_on_event(dev, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_device_on_event(dev, ev_cb, &c, &tok), DEPZ_OK);
    evs = depz_device_events(dev, 8);
    CHECK(evs);
    peer_send(&p, DEPZ_RPT_SEQUENCE_ERROR, seqerr, sizeof seqerr);
    peer_send(&p, DEPZ_RPT_STATUS, fault, sizeof fault);
    peer_send(&p, DEPZ_RPT_TEXT, hello, sizeof hello);
    peer_send(&p, DEPZ_RPT_TEMPERATURE, temp, sizeof temp);
    peer_send(&p, 0x7F, unknown, sizeof unknown);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_SEQUENCE_ERROR && ev.expected_seq == 7 && ev.received_seq == 9 && ev.by_device);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_UNSOLICITED_STATUS && ev.status == DEPZ_STATUS_ERR_HARDWARE_FAULT);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_TEXT && ev.cmd == 0x04 && strcmp(ev.text, "hello") == 0);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_TEMPERATURE && ev.timestamp_us == 123456u && fabs(ev.celsius - 25.1) < 1e-9);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_TEXT && ev.cmd == 0x7F && strcmp(ev.text, "0102") == 0);
    /* the callback ran before each push, so it has seen all five */
    depz_mutex_lock(&c.lock);
    seen = c.unsolicited == 1 && c.text == 2 && c.temperature == 1;
    depz_mutex_unlock(&c.lock);
    CHECK(seen);
    depz_device_stats(dev, &st);
    CHECK(st.device_seq_errors == 1 && st.rx_packets == 5 && st.seq_gaps == 0);
    depz_device_off_event(dev, tok);
    depz_device_off_event(NULL, tok); /* no device: a no-op */
    peer_send(&p, DEPZ_RPT_TEXT, again, sizeof again);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_TEXT && strcmp(ev.text, "again") == 0);
    depz_mutex_lock(&c.lock);
    seen = c.text;
    depz_mutex_unlock(&c.lock);
    CHECK(seen == 2); /* callback unsubscribed, the stream still fed */
    depz_stream_close(evs);
    peer_down(&p, dev);
    depz_mutex_destroy(&c.lock);
}

/* Contract 01 §5: bytes before any magic are TRASH, a frame whose payload CRC
 * trailer is damaged is a CRC_ERROR event, not a packet; both are counted. */
static void link_damage_is_counted_and_reported(void)
{
    static const uint8_t garbage[9] = {0x00, 'g', 'a', 'r', 'b', 'a', 'g', 'e', 0x00};
    static const uint8_t ok[4] = {0x04, 'o', 'k', 0x00};
    static const uint8_t fine[6] = {0x04, 'f', 'i', 'n', 'e', 0x00};
    peer p;
    depz_device *dev;
    depz_stream *evs;
    depz_device_event ev;
    depz_link_stats st;
    uint8_t frame[32], trash[16];
    size_t n = 0, ntrash = 0;
    bool crc_seen = false;
    CHECK(peer_up(&p, &dev));
    evs = depz_device_events(dev, 16);
    CHECK(evs);
    CHECK_RC(depz_link_write(p.side, garbage, sizeof garbage), DEPZ_OK);
    CHECK_RC(depz_build_packet(DEPZ_RPT_TEXT, ok, sizeof ok, 0, DEPZ_CRC8, frame, sizeof frame, &n), 0);
    frame[n - 1] ^= 0xFF; /* the payload CRC trailer corrupted */
    CHECK_RC(depz_link_write(p.side, frame, n), DEPZ_OK);
    p.seq = 1;
    peer_send(&p, DEPZ_RPT_TEXT, fine, sizeof fine);
    /* Trash event boundaries follow read chunking: compare the bytes only. */
    for (;;) {
        CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
        if (ev.type == DEPZ_DEV_EV_TRASH) {
            CHECK(ev.data_len == ev.trash_len && ntrash + ev.data_len <= sizeof trash);
            memcpy(trash + ntrash, ev.data, ev.data_len);
            ntrash += ev.data_len;
        } else if (ev.type == DEPZ_DEV_EV_CRC_ERROR) {
            CHECK(ev.cmd == DEPZ_RPT_TEXT && ev.seq == 0);
            crc_seen = true;
        } else {
            CHECK(ev.type == DEPZ_DEV_EV_TEXT && strcmp(ev.text, "fine") == 0);
            break;
        }
    }
    CHECK(crc_seen && ntrash == sizeof garbage && memcmp(trash, garbage, sizeof garbage) == 0);
    depz_device_stats(dev, &st);
    CHECK(st.trash_bytes == 9 && st.crc_errors == 1 && st.rx_packets == 1 && st.seq_gaps == 0);
    depz_stream_close(evs);
    peer_down(&p, dev);
}

typedef struct { depz_device *dev; int rc; char s[32]; } call_arg;

static void name_call(void *a)
{
    call_arg *x = (call_arg *)a;
    x->rc = depz_device_get_device_name(x->dev, x->s, sizeof x->s);
}

static void serial_call(void *a)
{
    call_arg *x = (call_arg *)a;
    x->rc = depz_device_get_serial_number(x->dev, x->s, sizeof x->s);
}

static void temperature_call(void *a)
{
    call_arg *x = (call_arg *)a;
    double c = 0;
    x->rc = depz_device_read_mcu_temperature(x->dev, &c);
    snprintf(x->s, sizeof x->s, "%.1f", c);
}

static void bootloader_call(void *a)
{
    call_arg *x = (call_arg *)a;
    x->rc = depz_device_enter_bootloader(x->dev);
}

static bool call_start(depz_thread *t, depz_thread_fn fn, call_arg *a, depz_device *dev)
{
    memset(a, 0, sizeof *a);
    a->dev = dev;
    a->rc = 1;
    return depz_thread_start(t, fn, a) == 0;
}

/* A pending RPT_TEXT matcher claims only the text echoing its own opcode; an
 * OK ack for a data request is swallowed, the request waits for its report
 * (contract 02 §1); a vanished device fails the pending request as closed,
 * fires DISCONNECTED and every later request is refused at once
 * (test_device_events.py). */
static void pending_requests_meet_echoes_acks_and_a_drop(void)
{
    static const uint8_t other[5] = {DEPZ_CMD_GET_SERIAL, 'S', 'N', '1', 0x00};
    static const uint8_t board[7] = {DEPZ_CMD_GET_DEVICE_NAME, 'B', 'o', 'a', 'r', 'd', 0x00};
    static const uint8_t ack[2] = {DEPZ_CMD_GET_MCU_TEMPERATURE, DEPZ_STATUS_OK};
    /* <Qh 5, -15: -1.5 °C */
    static const uint8_t temp[10] = {0x05, 0, 0, 0, 0, 0, 0, 0, 0xF1, 0xFF};
    peer p;
    depz_device *dev;
    depz_stream *evs;
    depz_device_event ev;
    depz_thread t;
    call_arg a;
    char s[32];
    CHECK(peer_up(&p, &dev));
    evs = depz_device_events(dev, 16);
    CHECK(evs);

    CHECK(call_start(&t, name_call, &a, dev));
    CHECK(peer_expect(&p, DEPZ_CMD_GET_DEVICE_NAME));
    peer_send(&p, DEPZ_RPT_TEXT, other, sizeof other);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_TEXT && ev.cmd == DEPZ_CMD_GET_SERIAL && strcmp(ev.text, "SN1") == 0);
    peer_send(&p, DEPZ_RPT_TEXT, board, sizeof board);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_OK && strcmp(a.s, "Board") == 0);

    /* had the ack completed it, the request would fail on a 0-byte report */
    CHECK(call_start(&t, temperature_call, &a, dev));
    CHECK(peer_expect(&p, DEPZ_CMD_GET_MCU_TEMPERATURE));
    peer_send(&p, DEPZ_RPT_STATUS, ack, sizeof ack);
    peer_send(&p, DEPZ_RPT_TEMPERATURE, temp, sizeof temp);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_OK && strcmp(a.s, "-1.5") == 0);

    CHECK(call_start(&t, serial_call, &a, dev));
    CHECK(peer_expect(&p, DEPZ_CMD_GET_SERIAL));
    depz_link_close(p.side); /* the device vanished mid-request */
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_CLOSED);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_DISCONNECTED);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_E_CLOSED); /* the stream ends with the device */
    CHECK(depz_device_closed(dev));
    CHECK_RC(depz_device_get_serial_number(dev, s, sizeof s), DEPZ_E_CLOSED);
    depz_stream_close(evs);
    peer_down(&p, dev);
}

/* Contract 02: the bootloader command acks OK, then the device is closed
 * for I/O by the host itself (no DISCONNECTED: the close is intended). */
static void bootloader_ack_closes_the_device(void)
{
    static const uint8_t ack[2] = {DEPZ_CMD_BOOTLOADER, DEPZ_STATUS_OK};
    peer p;
    depz_device *dev;
    depz_thread t;
    call_arg a;
    char s[32];
    ev_count c;
    CHECK(peer_up(&p, &dev));
    memset(&c, 0, sizeof c);
    depz_mutex_init(&c.lock);
    CHECK_RC(depz_device_on_event(dev, ev_cb, &c, NULL), DEPZ_OK);
    CHECK(call_start(&t, bootloader_call, &a, dev));
    CHECK(peer_expect(&p, DEPZ_CMD_BOOTLOADER));
    peer_send(&p, DEPZ_RPT_STATUS, ack, sizeof ack);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_OK);
    CHECK(depz_device_closed(dev));
    CHECK_RC(depz_device_get_serial_number(dev, s, sizeof s), DEPZ_E_CLOSED);
    peer_down(&p, dev);
    CHECK(c.disconnected == 0);
    depz_mutex_destroy(&c.lock);
}

/* contract 02: GET -> RPT_PAYLOAD_CRC_TYPE; SET takes 0..3, else the device
 * answers ERR_INVALID_PARAM and keeps its setting; a value that is no
 * depz_crc_type is refused on the host, nothing sent (ERRATA A2). */
static void payload_crc_type_roundtrip(rig *r)
{
    static const int bad[5] = {4, 7, 255, 300, -1};
    static const uint8_t seven = 7;
    depz_crc_type t = DEPZ_CRC32;
    uint8_t cmds[64];
    size_t i, before;
    CHECK_RC(depz_device_get_payload_crc_type(r->dev, &t), DEPZ_OK);
    CHECK(t == DEPZ_CRC_NONE);
    CHECK_RC(depz_device_set_payload_crc_type(r->dev, DEPZ_CRC16), DEPZ_OK);
    CHECK(fake_sr04_payload_crc_type(r->fake) == 2);
    CHECK_RC(depz_device_get_payload_crc_type(r->dev, &t), DEPZ_OK);
    CHECK(t == DEPZ_CRC16);
    CHECK_RC(depz_device_request(r->dev, DEPZ_CMD_SET_PAYLOAD_CRC_TYPE, &seven, 1, NULL, NULL, true, -1),
             DEPZ_E_STATUS);
    CHECK(depz_last_status() == DEPZ_STATUS_ERR_INVALID_PARAM);
    CHECK_RC(depz_device_get_payload_crc_type(r->dev, &t), DEPZ_OK);
    CHECK(t == DEPZ_CRC16);
    before = fake_sr04_commands(r->fake, cmds, sizeof cmds);
    for (i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        CHECK_RC(depz_device_set_payload_crc_type(r->dev, (depz_crc_type)bad[i]), DEPZ_E_ARG);
        CHECK(strstr(depz_last_error(), "set_payload_crc_type") != NULL);
    }
    CHECK(fake_sr04_commands(r->fake, cmds, sizeof cmds) == before);
    CHECK(cmds[before - 1] == DEPZ_CMD_GET_PAYLOAD_CRC_TYPE);
    CHECK(fake_sr04_payload_crc_type(r->fake) == 2);
}

/* DEVICE_RESET acks, then the link drops with the reboot. */
static void reset_acks_then_the_link_drops(rig *r)
{
    depz_device_event ev;
    depz_stream *evs = depz_device_events(r->dev, 8);
    CHECK(evs);
    CHECK_RC(depz_device_reset(r->dev), DEPZ_OK);
    CHECK(fake_sr04_reset_count(r->fake) == 1);
    CHECK_RC(depz_stream_next(evs, &ev, 1000), DEPZ_OK);
    CHECK(ev.type == DEPZ_DEV_EV_DISCONNECTED);
    CHECK(depz_device_closed(r->dev));
    depz_stream_close(evs);
}

/* Host-side refusals: a request with no completion path, a payload too big
 * for a packet (nothing written), NULL handles. */
static void requests_refused_on_the_host(rig *r)
{
    uint8_t *big = (uint8_t *)calloc(DEPZ_MAX_PAYLOAD + 1u, 1);
    depz_link_stats before, after;
    depz_device *dev = NULL;
    CHECK(big);
    CHECK_RC(depz_device_request(r->dev, DEPZ_CMD_GET_SERIAL, NULL, 0, NULL, NULL, false, -1), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "matcher or ok_completes") != NULL);
    depz_device_stats(r->dev, &before);
    CHECK_RC(depz_device_send(r->dev, 0x40, big, DEPZ_MAX_PAYLOAD + 1u), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "16384 bytes does not fit a packet") != NULL);
    depz_device_stats(r->dev, &after);
    CHECK(after.tx_packets == before.tx_packets && after.tx_bytes == before.tx_bytes);
    free(big);
    CHECK_RC(depz_device_send(NULL, 0x40, NULL, 0), DEPZ_E_ARG);
    CHECK_RC(depz_device_request(NULL, 0x40, NULL, 0, NULL, NULL, true, -1), DEPZ_E_ARG);
    CHECK_RC(depz_device_create(NULL, NULL, NULL, &dev), DEPZ_E_ARG);
    CHECK(strcmp(depz_device_port(NULL), "") == 0);
    CHECK(strcmp(depz_device_port(r->dev), "loopback-a") == 0);
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

typedef struct {
    depz_link *link;
    int rc;
    bool done;
} exhausted_read_arg;

static void exhausted_read_long(void *p)
{
    exhausted_read_arg *a = (exhausted_read_arg *)p;
    uint8_t buf[16];
    size_t got = 0;
    a->rc = depz_link_read(a->link, buf, sizeof buf, 30000, &got);
    a->done = true;
}

/* A capture with nothing left behaves like a quiet port: a read returns
 * empty at its timeout (or at close), never at once — the reader thread
 * would spin a core on empty reads (contract 08 «Replay semantics»). */
static void replay_exhausted_waits_its_timeout(void)
{
    static const char header_only[] = "{\"schema\": \"depz.rec/1\"}\n";
    const char *path = "test_io_exhausted.depzrec";
    depz_link *rp = NULL;
    depz_thread t;
    exhausted_read_arg a;
    uint8_t buf[16];
    size_t got = 1;
    uint64_t t0;
    FILE *f = fopen(path, "wb");
    CHECK(f);
    if (!f) return;
    fwrite(header_only, 1, sizeof header_only - 1, f);
    fclose(f);
    CHECK_RC(depz_link_open_replay(path, false, false, &rp), DEPZ_OK);
    if (!rp) return;
    CHECK(depz_link_replay_exhausted(rp));
    t0 = depz_host_now_us();
    CHECK_RC(depz_link_read(rp, buf, sizeof buf, 100, &got), DEPZ_OK);
    CHECK(got == 0);
    CHECK(depz_host_now_us() - t0 >= 80000);
    /* a long read ends at close(), not at its 30 s timeout */
    a.link = rp;
    a.rc = 1;
    a.done = false;
    CHECK(depz_thread_start(&t, exhausted_read_long, &a) == 0);
    t0 = depz_host_now_us();
    depz_link_close(rp);
    depz_thread_join(t);
    CHECK(a.done && a.rc == DEPZ_E_CLOSED);
    CHECK(depz_host_now_us() - t0 < 5000000);
    depz_link_free(rp);
    remove(path);
}

/* ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
 * container is not inflated but refused like any damaged file. The gzipped
 * bytes (mtime 0) are the same 70 in every SDK's test; `plain` is what they
 * inflate to, a valid capture. */
static void replay_refuses_a_gzipped_capture(void)
{
    static const char plain[] = "{\"schema\": \"depz.rec/1\"}\n{\"t\": 0, \"dir\": \"rx\", \"data\": \"aa\"}\n";
    static const uint8_t gzipped[70] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xab, 0x56, 0x2a, 0x4e,
        0xce, 0x48, 0xcd, 0x4d, 0x54, 0xb2, 0x52, 0x50, 0x4a, 0x49, 0x2d, 0xa8, 0xd2, 0x2b,
        0x4a, 0x4d, 0xd6, 0x37, 0x54, 0xaa, 0xe5, 0xaa, 0x56, 0x2a, 0x01, 0x8a, 0x19, 0xe8,
        0x00, 0x85, 0x33, 0x8b, 0x40, 0xb2, 0x45, 0x15, 0x4a, 0x20, 0x4e, 0x62, 0x09, 0x58,
        0x6d, 0x62, 0x22, 0x50, 0x0d, 0x00, 0x7e, 0x3b, 0xc0, 0x0b, 0x3d, 0x00, 0x00, 0x00,
    };
    const char *path = "test_io_gzipped.depzrec";
    depz_link *rp = NULL;
    FILE *f = fopen(path, "wb");
    CHECK(f);
    if (!f) return;
    fwrite(plain, 1, sizeof plain - 1, f);
    fclose(f);
    CHECK_RC(depz_link_open_replay(path, false, false, &rp), DEPZ_OK);
    depz_link_free(rp);
    rp = NULL;
    f = fopen(path, "wb");
    CHECK(f);
    if (!f) return;
    fwrite(gzipped, 1, sizeof gzipped, f);
    fclose(f);
    CHECK_RC(depz_link_open_replay(path, false, false, &rp), DEPZ_E_ARG);
    CHECK(rp == NULL);
    remove(path);
}

/* contract 08 «Line rules»: a line nested deeper than 256 is damaged — the
 * invalid-argument error naming the line, not a stack overflow in the
 * recursive-descent parser (a 200 KB line of `[[[[…` used to SIGSEGV). */
static void reader_refuses_a_line_nested_100000_deep(void)
{
    enum { DEEP = 100000 };
    static const char rec_head[] = "{\"schema\": \"depz.dataset/2\"}\n{\"d\":\"d0\",\"t\":1,\"k\":\"x\",\"v\":{\"a\":";
    static const char cap_head[] = "{\"schema\": \"depz.rec/1\", \"x\": ";
    const char *path = "test_io_deep.depzrec";
    size_t hl = sizeof rec_head - 1, n;
    char *text = (char *)malloc(hl + 2 * DEEP + 8);
    depz_link *rp = NULL;
    FILE *f;
    CHECK(text);
    if (!text) return;
    memset(text, '[', DEEP);
    memset(text + DEEP, ']', DEEP);
    CHECK(depz_json_parse(text, 2 * DEEP) == NULL);
    memmove(text + hl, text, 2 * DEEP);
    memcpy(text, rec_head, hl);
    memcpy(text + hl + 2 * DEEP, "}}\n", 3);
    n = hl + 2 * DEEP + 3;
    CHECK(depz_dataset_parse(text, n) == NULL);
    CHECK(strstr(depz_last_error(), "dataset: line 2: record is not JSON") != NULL);
    f = fopen(path, "wb");
    CHECK(f);
    if (!f) { free(text); return; }
    fwrite(cap_head, 1, sizeof cap_head - 1, f);
    fwrite(text + hl, 1, 2 * DEEP, f);
    fwrite("}\n", 1, 2, f);
    fclose(f);
    free(text);
    CHECK_RC(depz_link_open_replay(path, false, false, &rp), DEPZ_E_ARG);
    CHECK(rp == NULL && strstr(depz_last_error(), ":1: not JSON") != NULL);
    remove(path);
}

/* ── golden data (contracts/vectors) ─────────────────────────────────────
 * The real-hardware captures (vectors/recordings) are replayed by the
 * contract 15 scenarios (scenarios.c), run as test_io scenario:<id>. */

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

/* START_STREAM refused after the ULD started the die: the caller gets the
 * START_STREAM error, the host says not ranging, so the die is stopped again
 * (SYSTEM_START <- 0x80) and configuration works afterwards (PY-2/TS-5). */
L4_RIG(l4_a_refused_start_stream_stops_the_sensor_again)
{
    uint8_t got[1];
    size_t starts;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    starts = fake_vl53l4_writes_at(r->fake, 0x0087);
    fake_vl53l4_fail_next(r->fake, 0x35, DEPZ_STATUS_ERR_HARDWARE_FAULT);
    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_E_STATUS);
    CHECK(depz_last_status_cmd() == 0x35 && depz_last_status() == DEPZ_STATUS_ERR_HARDWARE_FAULT); /* not an error of the stop */
    CHECK(!depz_vl53l4cd_ranging(r->dev) && !fake_vl53l4_streaming(r->fake));
    CHECK(fake_vl53l4_writes_at(r->fake, 0x0087) == starts + 2); /* start, then the stop */
    CHECK(fake_vl53l4_write_at(r->fake, 0x0087, starts + 1, got, 1) == 1 && got[0] == 0x80);
    CHECK(fake_vl53l4_reg(r->fake, 0x0087) == 0x80);
    CHECK_RC(depz_vl53l4cd_set_offset_mm(r->dev, 3), DEPZ_OK); /* not wedged */
    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_OK);    /* a retry starts cleanly */
    CHECK(depz_vl53l4cd_ranging(r->dev) && fake_vl53l4_streaming(r->fake));
    CHECK_RC(depz_vl53l4cd_stop_ranging(r->dev), DEPZ_OK);
}

/* The stop after a refused START_STREAM is refused too: the caller still gets
 * the START_STREAM error and status, not the stop's WRITE_REG ones — the C++
 * exception is built from them. */
L4_RIG(l4_a_refused_stop_after_a_refused_start_stream_keeps_its_error)
{
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    fake_vl53l4_fail_next_then(r->fake, 0x35, DEPZ_STATUS_ERR_HARDWARE_FAULT, 0x33, DEPZ_STATUS_ERR_BUSY);
    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_E_STATUS);
    CHECK(depz_last_status_cmd() == 0x35);
    CHECK(depz_last_status() == DEPZ_STATUS_ERR_HARDWARE_FAULT);
    CHECK(strstr(depz_last_error(), "0x35") != NULL);
    CHECK(!depz_vl53l4cd_ranging(r->dev) && !fake_vl53l4_streaming(r->fake));
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

/* A raw result block (big-endian registers 0x0089..0x0099) seeded into the
 * fake's register file, as fake_vl53l4.py result_block(): raw status 9 is
 * ULD status 0 "valid", 2048 raw SPADs are 8 SPADs. */
static void l4_seed_result(fake_vl53l4 *f, uint16_t distance_mm, uint8_t status, uint16_t signal_kcps)
{
    uint8_t raw[17];
    size_t i;
    memset(raw, 0, sizeof raw);
    raw[0] = status;
    raw[2] = 1;
    raw[3] = 0x08; /* spads 2048 */
    raw[5] = (uint8_t)((signal_kcps / 8u) >> 8);
    raw[6] = (uint8_t)(signal_kcps / 8u);
    raw[10] = 12; /* sigma 3 mm x 4 */
    raw[13] = (uint8_t)(distance_mm >> 8);
    raw[14] = (uint8_t)distance_mm;
    for (i = 0; i < sizeof raw; i++) fake_vl53l4_set_reg(f, (uint16_t)(DEPZ_VL53L4_RESULT_BLOCK_ADDR + i), raw[i]);
}

/* The last byte written exactly at `addr`, and the one before it. */
static bool l4_last_two_writes(fake_vl53l4 *f, uint16_t addr, uint8_t first, uint8_t second)
{
    uint8_t a = 0, b = 0;
    size_t n = fake_vl53l4_writes_at(f, addr);
    if (n < 2) return false;
    fake_vl53l4_write_at(f, addr, n - 2, &a, 1);
    fake_vl53l4_write_at(f, addr, n - 1, &b, 1);
    return a == first && b == second;
}

/* VL53L4CD_calibration.c over the bridge (test_vl53l4cd_device.py,
 * test_vl53l4cd_uld.py): offset = target - mean after a heat loop, xtalk =
 * (1 - mean/target) * signal/SPADs over the valid samples after the first;
 * bad arguments are refused before any traffic, no valid sample is an
 * error; the temperature update reruns VHV and restores the loop bound. */
L4_RIG(l4_calibrations_and_temperature_update)
{
    int32_t off = 0;
    uint16_t x = 0, xr = 0;
    size_t starts;
    CHECK_RC(depz_vl53l4cd_init(r->dev, 0), DEPZ_OK);
    l4_seed_result(r->fake, 120, 9, 1200);
    CHECK_RC(depz_vl53l4cd_calibrate_offset(r->dev, 150, 5, &off), DEPZ_OK);
    CHECK(off == 30);
    CHECK_RC(depz_vl53l4cd_get_offset_mm(r->dev, &off), DEPZ_OK);
    CHECK(off == 30);
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 240, 5, &x), DEPZ_OK);
    CHECK(x == 75); /* (1 - 120/240) * 1200/8 */
    CHECK_RC(depz_vl53l4cd_get_xtalk_kcps(r->dev, &xr), DEPZ_OK);
    CHECK(xr == 75);
    CHECK(fake_vl53l4_reg(r->fake, 0x0087) == 0x80); /* each burst ends stopped */

    starts = fake_vl53l4_writes_at(r->fake, 0x0087);
    CHECK_RC(depz_vl53l4cd_calibrate_offset(r->dev, 9, 20, &off), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "nb_samples must be 5..255, target 10..1000 mm") != NULL);
    CHECK_RC(depz_vl53l4cd_calibrate_offset(r->dev, 100, 4, &off), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 5001, 20, &x), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "target 10..5000 mm") != NULL);
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 100, 4, &x), DEPZ_E_ARG);
    CHECK(fake_vl53l4_writes_at(r->fake, 0x0087) == starts); /* no burst started */

    l4_seed_result(r->fake, 100, 4, 1200); /* every frame "signal below threshold" */
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 200, 5, &x), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "no valid samples") != NULL);
    CHECK(fake_vl53l4_reg(r->fake, 0x0087) == 0x80); /* stopped before the refusal */
    l4_seed_result(r->fake, 90, 9, 1000); /* (1 - 90/200) * 1000/8 = 68.75 */
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 200, 5, &x), DEPZ_OK);
    CHECK(x == 69);

    CHECK_RC(depz_vl53l4cd_start_temperature_update(r->dev), DEPZ_OK);
    CHECK(l4_last_two_writes(r->fake, 0x0008, 0x81, 0x09));
    CHECK(l4_last_two_writes(r->fake, 0x000B, 0x92, 0x00));
    CHECK(l4_last_two_writes(r->fake, 0x0087, 0x21, 0x80));
    CHECK(!depz_vl53l4cd_ranging(r->dev));

    CHECK_RC(depz_vl53l4cd_start_ranging(r->dev), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_start_temperature_update(r->dev), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_calibrate_offset(r->dev, 100, 0, &off), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_calibrate_xtalk(r->dev, 100, 0, &x), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "stop ranging first") != NULL);
    CHECK_RC(depz_vl53l4cd_stop_ranging(r->dev), DEPZ_OK);
    CHECK_RC(depz_vl53l4cd_reset_sensor(r->dev), DEPZ_OK);
    CHECK(!depz_vl53l4cd_initialized(r->dev));
}

typedef struct { depz_mutex lock; int n; int last_mm; } l4_counter;

static void l4_count_cb(const depz_vl53l4cd_measurement *m, void *user)
{
    l4_counter *c = (l4_counter *)user;
    depz_mutex_lock(&c->lock);
    c->n++;
    c->last_mm = m->r.distance_mm;
    depz_mutex_unlock(&c->lock);
}

/* Streamed blocks reach the callback and the stream; an unsubscribed
 * callback is not called again; get_measurement times out when nothing
 * streams; every ULD range status has its text (test_vl53l4cd_device.py). */
L4_RIG(l4_callbacks_get_measurement_and_status_texts)
{
    static const char *const texts[13] = {
        "valid", "sigma above threshold", "signal below threshold", "distance below detection threshold",
        "phase out of valid limit", "hardware fail", "no wrap-around check done",
        "wrapped target, phase mismatch", "processing fail", "crosstalk signal fail", "interrupt error",
        "merged target", "signal too low"};
    /* distance 0x0123 = 291 mm, raw status 9 -> 0 (valid); the second 77 mm */
    static const uint8_t block[17] = {0x09, 0x00, 0x07, 0x0f, 0x00, 0x01, 0xf4, 0x00, 0x64,
                                      0x00, 0x28, 0x00, 0x00, 0x01, 0x23, 0x00, 0xff};
    uint8_t block2[17];
    depz_vl53l4cd_measurement m;
    depz_stream *s;
    l4_counter c;
    int tok = -1, n;
    int last;
    int i;
    for (i = 0; i < 13; i++) CHECK(strcmp(depz_vl53l4cd_status_text(i), texts[i]) == 0);
    CHECK(strcmp(depz_vl53l4cd_status_text(255), "other error") == 0);
    CHECK(strcmp(depz_vl53l4cd_status_text(13), "unknown") == 0);

    memset(&c, 0, sizeof c);
    depz_mutex_init(&c.lock);
    CHECK_RC(depz_vl53l4cd_on_measurement(r->dev, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l4cd_on_measurement(r->dev, l4_count_cb, &c, &tok), DEPZ_OK);
    s = depz_vl53l4cd_stream(r->dev, 4);
    CHECK(s);
    CHECK_RC(depz_vl53l4cd_get_measurement(r->dev, 30, &m), DEPZ_E_TIMEOUT);
    fake_vl53l4_send_stream(r->fake, 1234, block, sizeof block);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    CHECK(m.timestamp_us == 1234 && m.r.distance_mm == 291);
    depz_vl53l4cd_off_measurement(r->dev, tok);
    memcpy(block2, block, sizeof block2);
    block2[13] = 0x00;
    block2[14] = 77;
    fake_vl53l4_send_stream(r->fake, 2345, block2, sizeof block2);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    CHECK(m.timestamp_us == 2345 && m.r.distance_mm == 77);
    depz_mutex_lock(&c.lock);
    n = c.n;
    last = c.last_mm;
    depz_mutex_unlock(&c.lock);
    CHECK(n == 1 && last == 291); /* unsubscribed before the second block */
    depz_stream_close(s);
    depz_mutex_destroy(&c.lock);
}

/* ── VL53L8 / L5 / L7 ────────────────────────────────────────────────── */

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
    /* contract 11 §3 (ERRATA A13): at most 60 Hz at 4x4 (the resolution after open), 15 Hz at 8x8 */
    CHECK(depz_vl53l8_max_ranging_frequency_hz(DEPZ_VL53L8_RES_4X4) == 60 &&
          depz_vl53l8_max_ranging_frequency_hz(DEPZ_VL53L8_RES_8X8) == 15);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(dev, 61), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);                   /* idempotent */
    CHECK_RC(depz_sr04_start(dev), DEPZ_E_WRONG_TYPE);
    /* 16 aggregates x 20 bins -> 1708 B, the size the L8CH capture streams. */
    depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
    CHECK_RC(depz_vl53l8_cnh_required_memory(&cnh, &bytes), DEPZ_E_ARG); /* no agg map yet */
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4), DEPZ_OK);
    CHECK_RC(depz_vl53l8_cnh_required_memory(&cnh, &bytes), DEPZ_OK);
    CHECK(bytes == 1708 && cnh.nb_of_aggregates == 16 && cnh.map_id[0] == 0 && cnh.map_id[63] == 15);
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 5, 4), DEPZ_E_ARG);
    /* Regression (difftest crash): a negative start wrote map_id[] before its
     * start; an unlisted resolution, merge / cols / rows < 1 and an int
     * overflow of start + cols * merge are refused the same way. */
    {
        depz_vl53l8_cnh_setup bad = cnh;
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&bad, 64, -24, 0, 2, 2, 4, 4), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&bad, 64, 0, -1, 1, 1, 4, 4), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&bad, 32, 0, 0, 1, 1, 4, 4), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&bad, 16, 0, 0, 1, 1, 0, 4), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_create_agg_map(&bad, 64, 1, 0, 0x40000000, 1, 4, 1), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_init_config_checked(&bad, 10, 256, 2), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_init_config_checked(&bad, 10, 20, -1), DEPZ_E_ARG);
        CHECK_RC(depz_vl53l8_cnh_init_config_checked(&bad, 10, 0, 2), DEPZ_OK); /* 0 bins: empty histograms */
    }
    depz_vl53l8_cnh_pack(&cnh, packed);
    CHECK(packed[0] == 0x00 && packed[1] == 0x50 && packed[18] == 2 && packed[19] == 20 && packed[25] == 0x3F);
    depz_device_close(dev);
    depz_link_free(b);
}

/* One frame of the running stream: subscribe, push it from the fake, take it. */
static int l8_take_frame(depz_device *dev, fake_vl53l8 *f, depz_vl53l8_live_frame *out)
{
    depz_stream *s = depz_vl53l8_frames(dev, 2);
    int rc;
    if (!s) return DEPZ_E_NOMEM;
    rc = fake_vl53l8_send_frame(f) ? depz_stream_next(s, out, 2000) : DEPZ_E_ARG;
    depz_stream_close(s);
    return rc;
}

/* VL53L8CH: CNH and the motion indicator share DCI 0xBFAC (contract 04 §7,
 * ERRATA B4) — configuring one disarms the other, init() disarms both. */
static void l8ch_cnh_and_motion_exclusive(void)
{
    static depz_vl53l8_live_frame fr;
    depz_link *link;
    depz_device *dev = NULL;
    depz_vl53l8_cnh_setup cnh, armed;
    uint8_t mi[156], packed[156];
    fake_vl53l8 *f = fake_vl53l8_start(0x0C0B6C9Eu, &link); /* the CH blob's checksum */
    CHECK(f != NULL);
    if (!f) return;
    CHECK_RC(depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L8CH, &dev), DEPZ_OK);
    CHECK_RC(depz_vl53l8_init(dev, NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4), DEPZ_OK);
    depz_vl53l8_cnh_pack(&cnh, packed);

    /* motion, then CNH: 0xBFAC holds the CNH setup, frames carry no motion */
    CHECK_RC(depz_vl53l8_configure_motion_indicator(dev, 400, 1500), DEPZ_OK);
    CHECK_RC(depz_vl53l8_configure_cnh(dev, &cnh), DEPZ_OK);
    CHECK(fake_vl53l8_dci(f, 0xBFAC, mi, sizeof mi) == sizeof mi && memcmp(mi, packed, sizeof mi) == 0);
    CHECK(depz_vl53l8_cnh_armed(dev, &armed) && armed.nb_of_aggregates == 16);
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_OK);
    CHECK(fake_vl53l8_frame_size(f) == 3156); /* 8x8 + the 16x20 CNH block, as the lab capture */
    CHECK_RC(l8_take_frame(dev, f, &fr), DEPZ_OK);
    CHECK(!fr.has_motion && fr.cnh_len == 1708);
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);

    /* CNH, then motion: 0xBFAC holds the motion config, the CNH block is gone */
    CHECK_RC(depz_vl53l8_configure_motion_indicator(dev, 400, 1500), DEPZ_OK);
    CHECK(fake_vl53l8_dci(f, 0xBFAC, mi, sizeof mi) == sizeof mi && mi[25] == 0);
    CHECK(!depz_vl53l8_cnh_armed(dev, NULL));
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_OK);
    CHECK(fake_vl53l8_frame_size(f) == 1444); /* 8x8 CH without CNH */
    CHECK_RC(l8_take_frame(dev, f, &fr), DEPZ_OK);
    CHECK(fr.has_motion && fr.cnh_len == 0);
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);

    /* init() disarms both: the motion armed above, then a CNH setup */
    CHECK_RC(depz_vl53l8_init(dev, NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_OK);
    CHECK_RC(l8_take_frame(dev, f, &fr), DEPZ_OK);
    CHECK(!fr.has_motion && fr.cnh_len == 0);
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);
    CHECK_RC(depz_vl53l8_configure_cnh(dev, &cnh), DEPZ_OK);
    CHECK_RC(depz_vl53l8_init(dev, NULL, NULL), DEPZ_OK);
    CHECK(!depz_vl53l8_cnh_armed(dev, NULL));
    CHECK_RC(depz_vl53l8_set_resolution(dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK_RC(depz_vl53l8_start_ranging(dev), DEPZ_OK);
    CHECK(fake_vl53l8_frame_size(f) == 1444);
    CHECK_RC(depz_vl53l8_stop_ranging(dev), DEPZ_OK);
    CHECK(depz_vl53l8_frame_parse_errors(dev) == 0);
    depz_device_close(dev);
    fake_vl53l8_stop(f);
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
    {
        /* Every component, in order; the late three come in one cargo. */
        depz_bno_product_id all[8];
        size_t n = 0;
        int i;
        CHECK_RC(depz_bno086_product_ids(dev, all, 8, &n), DEPZ_OK);
        CHECK(n == 4 && all[0].sw_part_number == FAKE_BNO086_PART && all[0].reset_cause == 1);
        CHECK(n == 4 && all[1].sw_part_number == 10003606u && all[1].sw_version_major == 1);
        CHECK(n == 4 && all[3].sw_part_number == 10003171u && all[3].sw_version_patch == 10);
        CHECK_RC(depz_bno086_product_ids(dev, all, 2, &n), DEPZ_OK);
        CHECK(n == 2);
        /* Back to back, never a late component (seen live: 9 of 20 calls on a
         * BNO085 returned the bootloader's when only the first was taken). */
        for (i = 0; i < 5; i++) {
            CHECK_RC(depz_bno086_product_id(dev, &p), DEPZ_OK);
            CHECK(p.sw_part_number == FAKE_BNO086_PART);
        }
    }
    {
        static const uint8_t ids[] = {0xF1, 0xF3, 0xF5, 0xF8, 0xFC, 0x99};
        static const size_t lens[] = {16, 16, 4, 16, 17, 0};
        size_t i;
        for (i = 0; i < sizeof ids; i++) CHECK(depz_bno_control_report_len(ids[i]) == lens[i]);
    }
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
    {   /* interactive calibration (0x0E, no answer) and the MotionRequest enable without read-back */
        uint8_t params[9];
        CHECK_RC(depz_bno086_set_motion_intent(dev, DEPZ_BNO_MOTION_INTENT_IN_MOTION), DEPZ_OK);
        CHECK(fake_bno086_last_command(f, DEPZ_SH2_CMD_INTERACTIVE_CALIBRATION, params) && params[0] == 3 &&
              params[1] == 0);
        CHECK_RC(depz_bno086_enable_motion_request(dev, 1), DEPZ_OK);
        /* no read-back of its own: a Get Feature queued behind it sees the fake's grid value */
        CHECK_RC(depz_bno086_get_feature(dev, DEPZ_BNO_SENSOR_MOTION_REQUEST, &g), DEPZ_OK);
        CHECK(g.interval_us == 512000 && fake_bno086_interval(f, DEPZ_BNO_SENSOR_MOTION_REQUEST) == 512000);
        CHECK_RC(depz_bno086_enable_motion_request(dev, 0), DEPZ_E_ARG);
    }
    {   /* force flush (0xF0): returns once the fake's `ef <sensor>` input report arrived */
        depz_stream *s = depz_bno086_reports(dev, 8);
        depz_bno_report r;
        CHECK_RC(depz_bno086_flush(dev, DEPZ_BNO_SENSOR_ACCELEROMETER), DEPZ_OK);
        CHECK_RC(depz_stream_next(s, &r, 1000), DEPZ_OK);
        CHECK(r.type == DEPZ_BNO_FLUSH_COMPLETED_REPORT && r.flushed_sensor == DEPZ_BNO_SENSOR_ACCELEROMETER &&
              r.sensor_id == DEPZ_BNO_FLUSH_COMPLETED);
        depz_stream_close(s);
    }
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

/* A multi-fragment input cargo is timestamped from its FIRST fragment: the
 * 0xFB base delta refers to the capture of the transfer that started it
 * (contract 05 §3, §5), not the one that completed it. */
static void bno086_cargo_capture(void)
{
    /* 0xFB base delta 10 ticks, then 12 accelerometer reports, delay 0..11. */
    uint8_t cargo[5 + 12 * 10], frame[64];
    static const uint64_t captures[3] = {7000000, 7002000, 7004000};
    size_t len = 0, off = 0;
    int k = 0, i;
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    depz_stream *s = depz_bno086_reports(dev, 16);
    depz_bno_report r;
    CHECK(dev);
    CHECK(s);
    cargo[len++] = 0xFB;
    cargo[len++] = 10; cargo[len++] = 0; cargo[len++] = 0; cargo[len++] = 0;
    for (i = 0; i < 12; i++) {
        static const uint8_t xyz[6] = {0, 0, 0, 0, 0xCF, 0x09}; /* 0, 0, 2511 */
        cargo[len++] = DEPZ_BNO_SENSOR_ACCELEROMETER;
        cargo[len++] = (uint8_t)i;
        cargo[len++] = 0;
        cargo[len++] = (uint8_t)i;
        memcpy(cargo + len, xyz, sizeof xyz);
        len += sizeof xyz;
    }
    /* Three ≤64-byte frames: the first advertises the total, continuations
     * the remaining length with bit 15 set; seq increments per frame. */
    while (off < len) {
        size_t chunk = len - off < 60 ? len - off : 60;
        uint16_t hl = (uint16_t)(4 + len - off) | (off ? 0x8000u : 0);
        frame[0] = (uint8_t)hl;
        frame[1] = (uint8_t)(hl >> 8);
        frame[2] = DEPZ_SHTP_CH_INPUT_NORMAL;
        frame[3] = (uint8_t)k;
        memcpy(frame + 4, cargo + off, chunk);
        CHECK(k < 3);
        if (k < 3) fake_bno086_send_frame(f, captures[k], frame, 4 + chunk);
        off += chunk;
        k++;
    }
    CHECK(k == 3);
    for (i = 0; i < 12; i++) {
        CHECK_RC(depz_stream_next(s, &r, 2000), DEPZ_OK);
        CHECK(r.type == DEPZ_BNO_ACCELERATION && r.seq == i);
        CHECK(r.timestamp_us == 6999000 + 100 * (int64_t)i);
    }
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
    {   /* hub power (contract 05 §6a): one byte on channel 1, no answer */
        static const uint8_t want_sleep[5] = {0x05, 0x00, 0x01, 0x00, DEPZ_SHTP_EXEC_SLEEP};
        static const uint8_t want_on[5] = {0x05, 0x00, 0x01, 0x01, DEPZ_SHTP_EXEC_ON};
        uint8_t fr[8];
        CHECK_RC(depz_bno086_sleep(dev), DEPZ_OK);
        CHECK_RC(depz_bno086_power_on(dev), DEPZ_OK);
        CHECK(fake_bno086_executable_frame(f, 0, fr) == 5 && memcmp(fr, want_sleep, 5) == 0);
        CHECK(fake_bno086_executable_frame(f, 1, fr) == 5 && memcmp(fr, want_on, 5) == 0);
    }
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

/* ── the VL53L 1D family ─────────────────────────────────────────────── */

static depz_vl53lx_driver vlx_kind(const char *s)
{
    if (!strcmp(s, "uld")) return DEPZ_VL53LX_DRIVER_ULD;
    if (!strcmp(s, "ulp")) return DEPZ_VL53LX_DRIVER_ULP;
    return DEPZ_VL53LX_DRIVER_HISTOGRAM;
}

/* configure_ex's signal threshold on a driver without that group is refused
 * before the re-init, with no traffic (a scenario cannot name configure_ex).
 * Each capture replays strictly up to init; the scenarios assert the rest. */
static void vlx_unsupported_group_refused(void)
{
    static const struct { const char *stem, *driver, *product; int budget; } caps[] = {
        {"vl53l4cx_uld_as_l4cd_50ms", "uld", "VL53L4CD", 50}, {"vl53l1cx_uld_long_33ms", "uld", NULL, 33},
        {"vl53l1cb_uld_short_50ms", "uld", NULL, 50},          {"vl53l3cx_ulp_33ms", "ulp", NULL, 33},
        {"vl53l0x_uld_long-range_33ms", "uld", NULL, 33},       {"vl53l1cx_histogram_long_33ms", "histogram", NULL, 33},
    };
    const int kcps = 512;
    size_t i, refused = 0;
    for (i = 0; i < sizeof caps / sizeof caps[0]; i++) {
        char path[1024], name[64];
        depz_link *link;
        depz_device *dev;
        snprintf(path, sizeof path, "%s/recordings/%s.depzrec", DEPZ_VECTORS_DIR, caps[i].stem);
        CHECK_RC(depz_link_open_replay(path, true, false, &link), DEPZ_OK);
        CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
        depz_device_set_timeout_ms(dev, 2000);
        CHECK_RC(depz_device_promote(dev), DEPZ_OK);
        CHECK_RC(depz_device_get_software_name(dev, name, sizeof name), DEPZ_OK);
        CHECK_RC(depz_vl53lx_init(dev, vlx_kind(caps[i].driver),
                                  caps[i].product ? depz_vl53lx_product_from_str(caps[i].product)
                                                  : DEPZ_VL53LX_PRODUCT_NONE), DEPZ_OK);
        if (!depz_vl53lx_supports(dev, DEPZ_VL53LX_CAP_SIGNAL_THRESH)) {
            CHECK_RC(depz_vl53lx_configure_ex(dev, caps[i].budget, 0, NULL, NULL, NULL, &kcps), DEPZ_E_ARG);
            CHECK(strstr(depz_last_error(), "does not support") != NULL);
            refused++;
        }
        depz_device_close(dev);
    }
    CHECK(refused > 0);
}

/* The public depz_vl53lx_set_mode over the L1X ULD on the fake register file
 * (ERRATA A9; the vectors' l1_uld cases drive the bare driver op): 15 ms is
 * short-only, so going to long hands back the nearest long budget, 20 ms —
 * the one get_range_timing then reads; a NULL out-pointer still changes mode.
 * The histogram driver's mode change never moves the budget: 0. */
static void vlx_l1_set_mode_hands_back_the_budget(void)
{
    fake_vl53l4 *fake;
    depz_link *link;
    depz_device *dev;
    const char *mode = NULL;
    int b = -1, im = -1;
    fake = fake_vl53l4_start(&link);
    if (!fake) { g_failed++; return; }
    CHECK_RC(depz_vl53lx_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 1000);
    CHECK_RC(depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_PRODUCT_L1CX), DEPZ_OK);
    CHECK_RC(depz_vl53lx_configure(dev, 15, 0, "short", NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_vl53lx_set_mode(dev, "long", &b), DEPZ_OK);
    CHECK(b == 20);
    CHECK_RC(depz_vl53lx_get_range_timing(dev, &b, &im), DEPZ_OK);
    CHECK(b == 20);
    CHECK_RC(depz_vl53lx_get_mode(dev, &mode), DEPZ_OK);
    CHECK(mode && !strcmp(mode, "long"));
    CHECK_RC(depz_vl53lx_set_mode(dev, "short", NULL), DEPZ_OK); /* 20 ms is tabulated in short */
    CHECK_RC(depz_vl53lx_get_mode(dev, &mode), DEPZ_OK);
    CHECK(mode && !strcmp(mode, "short"));
    b = -1;
    CHECK_RC(depz_vl53lx_set_mode(dev, "medium", &b), DEPZ_E_ARG);
    CHECK(b == -1); /* a refused mode leaves the output alone */
    CHECK_RC(depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_HISTOGRAM, DEPZ_VL53LX_PRODUCT_L1CX), DEPZ_OK);
    CHECK_RC(depz_vl53lx_set_mode(dev, "long", &b), DEPZ_OK);
    CHECK(b == 0);
    depz_device_close(dev);
    fake_vl53l4_stop(fake);
}

/* ── TMF8829 against the fake bridge (contracts/vectors/tmf8829.json) ────── */

static uint8_t *tmf_unhex(const char *hex, size_t *n)
{
    size_t k, len = hex ? strlen(hex) / 2 : 0;
    uint8_t *b = (uint8_t *)malloc(len ? len : 1);
    for (k = 0; b && k < len; k++) {
        unsigned v = 0;
        if (sscanf(hex + 2 * k, "%2x", &v) != 1) v = 0;
        b[k] = (uint8_t)v;
    }
    *n = len;
    return b;
}

static const json_value *tmf_named(const json_value *arr, const char *name)
{
    size_t i;
    for (i = 0; i < json_arr_size(arr); i++)
        if (!strcmp(json_as_str(json_obj_get(json_arr_get(arr, i), "name")), name)) return json_arr_get(arr, i);
    return NULL;
}

typedef struct {
    depz_mutex lock;
    int frames, measurements, events;
    size_t progress_calls, progress_done, progress_total;
} tmf_counts;

static void tmf_progress(size_t done, size_t total, void *u)
{
    tmf_counts *c = (tmf_counts *)u;
    c->progress_calls++;
    c->progress_done = done;
    c->progress_total = total;
}

static void tmf_frame_cb(const depz_tmf8829_raw_frame *f, void *u)
{
    tmf_counts *c = (tmf_counts *)u;
    (void)f;
    depz_mutex_lock(&c->lock);
    c->frames++;
    depz_mutex_unlock(&c->lock);
}

static void tmf_meas_cb(const depz_tmf8829_measurement *m, void *u)
{
    tmf_counts *c = (tmf_counts *)u;
    (void)m;
    depz_mutex_lock(&c->lock);
    c->measurements++;
    depz_mutex_unlock(&c->lock);
}

static void tmf_event_cb(const depz_tmf8829_event *e, void *u)
{
    tmf_counts *c = (tmf_counts *)u;
    (void)e;
    depz_mutex_lock(&c->lock);
    c->events++;
    depz_mutex_unlock(&c->lock);
}

static int tmf_count(tmf_counts *c, const int *field, int want)
{
    int i, v = 0;
    for (i = 0; i < 200; i++) {
        depz_mutex_lock(&c->lock);
        v = *field;
        depz_mutex_unlock(&c->lock);
        if (v >= want) break;
        depz_sleep_ms(5);
    }
    return v;
}

/* Queue the frames of one tmf8829.json measurement sequence on the fake. */
static bool tmf_script(fake_tmf8829 *f, const json_value *root, const char *seq)
{
    const json_value *s = tmf_named(json_obj_get(root, "measurements"), seq), *fr;
    size_t i;
    if (!s) return false;
    fake_tmf8829_clear_frames(f);
    fr = json_obj_get(s, "frames");
    for (i = 0; i < json_arr_size(fr); i++) {
        const json_value *e = json_arr_get(fr, i);
        const json_value *src = tmf_named(json_obj_get(root, "frames"), json_as_str(json_obj_get(e, "raw_case")));
        uint8_t *raw;
        size_t n;
        if (!src) return false;
        raw = tmf_unhex(json_as_str(json_obj_get(src, "raw")), &n);
        fake_tmf8829_add_frame(f, raw, n, (uint64_t)json_as_int(json_obj_get(e, "timestamp_us")),
                               (uint64_t)json_as_int(json_obj_get(e, "edge_us")));
        free(raw);
    }
    return true;
}

static bool tmf_same(const depz_tmf8829_measurement *m, const json_value *e)
{
    return e && m->timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")) &&
           m->flags == json_as_int(json_obj_get(e, "flags")) &&
           m->f_number == (uint32_t)json_as_int(json_obj_get(e, "f_number")) &&
           m->ratio_q16 == (uint32_t)json_as_int(json_obj_get(e, "ratio_q16")) &&
           m->n_ref_frames == (size_t)json_as_int(json_obj_get(e, "n_ref_frames")) &&
           m->n_histograms == (size_t)json_as_int(json_obj_get(e, "n_histograms")) &&
           m->n_problems == (size_t)json_as_int(json_obj_get(e, "n_problems")) &&
           m->results.peaks == json_as_int(json_obj_get(e, "peaks")) &&
           m->results.rows == json_as_int(json_obj_get(e, "rows")) &&
           m->results.cols == json_as_int(json_obj_get(e, "cols")) &&
           m->results.distance[0] == json_as_int(json_arr_get(json_obj_get(e, "distance"), 0));
}

static void tmf8829_session(fake_tmf8829 *f, depz_device *dev, const json_value *root, tmf_counts *cnt,
                            depz_tmf8829_measurement *m)
{
    const json_value *ex32 = json_obj_get(tmf_named(json_obj_get(root, "measurements"), "32x32_ref"), "expect");
    const json_value *ex8 = json_obj_get(tmf_named(json_obj_get(root, "measurements"), "8x8"), "expect");
    depz_tmf8829_options opt = DEPZ_TMF8829_OPTIONS_INIT;
    depz_tmf8829_config cfg;
    depz_tmf8829_stream_stats stats;
    depz_tmf8829_info info;
    char ver[16];
    uint32_t serial = 0, addr = 0, v;
    uint16_t words = 0;
    size_t writes = 0, maxw = 0, nimg, i;
    const uint8_t *img;
    uint8_t page[190], mask = 0, flags = 0;
    depz_stream *s;

    /* discovery wiring: APP_TMF8829_* promotes to the TMF8829 class */
    CHECK_RC(depz_device_promote(dev), DEPZ_OK);
    CHECK(depz_is_tmf8829(dev) && !depz_is_bno055(dev));
    CHECK(depz_device_sensor_type(dev) == DEPZ_SENSOR_TMF8829);
    CHECK_RC(depz_tmf8829_sensor_id(dev, ver, sizeof ver, &serial), DEPZ_E_ARG);

    /* init: soft reset, bootloader, the image in <= 1023 B FIFO writes */
    CHECK_RC(depz_tmf8829_init(dev, tmf_progress, cnt, ver, sizeof ver, &serial), DEPZ_OK);
    CHECK(!strcmp(ver, "2.200.0") && serial == FAKE_TMF8829_SERIAL);
    nimg = fake_tmf8829_image(f, &img);
    CHECK(nimg == depz_tmf8829_firmware_len && !memcmp(img, depz_tmf8829_firmware, nimg));
    fake_tmf8829_fifo_params(f, &addr, &words, &writes, &maxw);
    CHECK(addr == DEPZ_TMF8829_FIRMWARE_LOAD_ADDR && words == 3611 && writes == 15 && maxw == 1023);
    CHECK(cnt->progress_calls == 15 && cnt->progress_done == 14444 && cnt->progress_total == 14444);
    CHECK((fake_tmf8829_reg(f, DEPZ_TMF8829_REG_ENABLE) & 0x30) == 0x20); /* powerup_select = RAM */

    /* configure: every survivor field written over the stale captured page
     * (4 peaks + all strengths), verified by read-back */
    opt.period_ms = 50;
    opt.ref_frame = true;
    CHECK_RC(depz_tmf8829_configure(dev, "32x32", &opt, &cfg), DEPZ_OK);
    CHECK(cfg.rows == 32 && cfg.cols == 32 && cfg.ref_frame && !cfg.histograms && !strcmp(cfg.preset, "32x32"));
    fake_tmf8829_page(f, page);
    CHECK(depz_tmf8829_cfg_get(page, "period_ms", &v) == 0 && v == 50);
    CHECK(depz_tmf8829_cfg_get(page, "kilo_iterations", &v) == 0 && v == 2344);
    CHECK(depz_tmf8829_cfg_get(page, "nr_peaks", &v) == 0 && v == 1);
    CHECK(depz_tmf8829_cfg_get(page, "signal_strength", &v) == 0 && v == 0);
    CHECK(depz_tmf8829_cfg_get(page, "xtalk", &v) == 0 && v == 0);
    CHECK(depz_tmf8829_cfg_get(page, "ref_spad_frame", &v) == 0 && v == 1);
    CHECK(depz_tmf8829_cfg_get(page, "fp_mode", &v) == 0 && v == 3 && cfg.fields[2] == 3);
    CHECK_RC(depz_tmf8829_configure(dev, "64x64", NULL, NULL), DEPZ_E_ARG);
    {
        depz_tmf8829_options bad = DEPZ_TMF8829_OPTIONS_INIT;
        depz_tmf8829_field_value fv[1] = {{"no_such_field", 1}};
        bad.nr_peaks = 8;
        CHECK_RC(depz_tmf8829_configure(dev, "8x8", &bad, NULL), DEPZ_E_ARG);
        bad.nr_peaks = 1;
        bad.fields = fv;
        bad.n_fields = 1;
        CHECK_RC(depz_tmf8829_configure(dev, "8x8", &bad, NULL), DEPZ_E_ARG);
        fv[0].name = "fov_correction";
        fv[0].value = 3;
        fake_tmf8829_pin_fov_correction(f, true);
        CHECK_RC(depz_tmf8829_configure(dev, "8x8", &bad, NULL), DEPZ_E_PROTOCOL);
        CHECK(strstr(depz_last_error(), "did not stick") && strstr(depz_last_error(), "fov_correction wrote 3"));
        fake_tmf8829_pin_fov_correction(f, false);
    }
    CHECK_RC(depz_tmf8829_get_config(dev, &cfg), DEPZ_OK);
    CHECK(!strcmp(cfg.preset, "32x32")); /* a refused configure keeps the last good one */
    CHECK_RC(depz_tmf8829_configure(dev, "32x32", &opt, NULL), DEPZ_OK);

    /* stream: two 32x32 half-frames, each behind its ref-SPAD frame */
    CHECK(tmf_script(f, root, "32x32_ref"));
    CHECK_RC(depz_tmf8829_on_frame(dev, tmf_frame_cb, cnt, NULL), DEPZ_OK);
    CHECK_RC(depz_tmf8829_on_measurement(dev, tmf_meas_cb, cnt, NULL), DEPZ_OK);
    CHECK_RC(depz_tmf8829_on_event(dev, tmf_event_cb, cnt, NULL), DEPZ_OK);
    s = depz_tmf8829_measurements(dev, 4);
    CHECK(s);
    CHECK_RC(depz_tmf8829_start(dev), DEPZ_OK);
    CHECK(depz_tmf8829_streaming(dev));
    CHECK(fake_tmf8829_armed(f, &mask, &flags) && mask == DEPZ_TMF8829_INT_RESULTS && flags == DEPZ_TMF8829_SF_REF_FRAME);
    CHECK(fake_tmf8829_reg(f, DEPZ_TMF8829_REG_INT_ENAB) == DEPZ_TMF8829_INT_RESULTS);
    CHECK_RC(depz_stream_next(s, m, 2000), DEPZ_OK);
    CHECK(tmf_same(m, json_arr_get(ex32, 0)) && m->n_frames == 2 && m->rows == 32);
    CHECK(tmf_count(cnt, &cnt->frames, 4) == 4 && tmf_count(cnt, &cnt->measurements, 1) == 1);
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", NULL, NULL), DEPZ_E_ARG); /* not while streaming */

    /* an INT event reaches the callback */
    fake_tmf8829_send_event(f, DEPZ_TMF8829_INT_MOTION, DEPZ_TMF8829_EV_INT);
    CHECK(tmf_count(cnt, &cnt->events, 1) == 1);

    /* bridge info + raw reads that run into the FIFO port */
    CHECK_RC(depz_tmf8829_bridge_info(dev, &info), DEPZ_OK);
    CHECK(info.streaming && info.int_mask == 1 && info.frames == 4);
    {
        uint8_t buf[1100];
        uint8_t ra[8];
        uint16_t rl[8];
        for (i = 0; i < 15; i++) fake_tmf8829_set_reg(f, (uint8_t)(0xF0 + i), (uint8_t)(0x60 + i));
        fake_tmf8829_set_reg(f, 0xFF, 0xAB);
        fake_tmf8829_clear_reads(f);
        CHECK_RC(depz_tmf8829_read_registers(dev, 0xF0, buf, sizeof buf), DEPZ_OK);
        CHECK(fake_tmf8829_reads(f, ra, rl, 8) == 2 && ra[0] == 0xF0 && rl[0] == 1024 && ra[1] == 0xFF && rl[1] == 76);
        CHECK(buf[0] == 0x60 && buf[14] == 0x6E && buf[15] == 0xAB && buf[1099] == 0xAB);
        buf[0] = 0x5A;
        CHECK_RC(depz_tmf8829_write_registers(dev, 0xF0, buf, 1), DEPZ_OK);
        CHECK(fake_tmf8829_reg(f, 0xF0) == 0x5A);
    }

    /* stop swallows the one BAD_HEADER a STOP raises mid histogram dump */
    fake_tmf8829_bad_header_on_stop(f, true);
    CHECK_RC(depz_tmf8829_stop(dev), DEPZ_OK);
    CHECK(!depz_tmf8829_streaming(dev) && !fake_tmf8829_armed(f, NULL, NULL));
    CHECK(fake_tmf8829_reg(f, DEPZ_TMF8829_REG_INT_ENAB) == 0);
    depz_tmf8829_get_stream_stats(dev, &stats);
    CHECK(stats.events_swallowed == 1 && stats.frames_completed == 4 && stats.measurements == 1 &&
          stats.frames_dropped == 0 && stats.frames_discarded == 0 && stats.frame_parse_errors == 0);
    CHECK(tmf_count(cnt, &cnt->events, 1) == 1);
    fake_tmf8829_bad_header_on_stop(f, false);

    /* a second stream (8x8): a dropped frame_seq is counted, not fatal */
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", NULL, &cfg), DEPZ_OK);
    CHECK(cfg.rows == 8 && !cfg.ref_frame);
    CHECK(tmf_script(f, root, "8x8"));
    fake_tmf8829_skip_seq_before(f, 1);
    CHECK_RC(depz_tmf8829_start(dev), DEPZ_OK);
    CHECK(fake_tmf8829_armed(f, &mask, &flags) && mask == 1 && flags == 0);
    for (i = 0; i < 2; i++) {
        CHECK_RC(depz_stream_next(s, m, 2000), DEPZ_OK);
        CHECK(tmf_same(m, json_arr_get(ex8, i)) && m->n_frames == 1 && m->rows == 8);
    }
    depz_tmf8829_get_stream_stats(dev, &stats);
    CHECK(stats.frames_dropped == 1 && stats.frames_completed == 2 && stats.measurements == 2);
    CHECK_RC(depz_tmf8829_stop(dev), DEPZ_OK);
    CHECK(depz_stream_dropped_count(s) == 0);
    depz_stream_close(s);

    /* re-init forgets the config; start() then configures "8x8" itself */
    CHECK_RC(depz_tmf8829_init(dev, NULL, NULL, NULL, 0, NULL), DEPZ_OK);
    CHECK_RC(depz_tmf8829_get_config(dev, &cfg), DEPZ_E_ARG);
    fake_tmf8829_clear_frames(f);
    CHECK_RC(depz_tmf8829_start(dev), DEPZ_OK);
    CHECK_RC(depz_tmf8829_get_config(dev, &cfg), DEPZ_OK);
    CHECK(!strcmp(cfg.preset, "8x8"));
    CHECK_RC(depz_tmf8829_stop(dev), DEPZ_OK);
}

static void tmf8829_fake_session(void)
{
    char *text = slurp(DEPZ_VECTORS_DIR "/tmf8829.json");
    json_value *root;
    depz_link *link;
    depz_device *dev = NULL;
    fake_tmf8829 *f;
    tmf_counts cnt;
    depz_tmf8829_measurement *m;
    uint8_t *page;
    size_t n;
    CHECK(text);
    root = json_parse(text);
    free(text);
    CHECK(root);
    memset(&cnt, 0, sizeof cnt);
    depz_mutex_init(&cnt.lock);
    m = (depz_tmf8829_measurement *)malloc(sizeof *m);
    f = fake_tmf8829_start(&link);
    CHECK(f && m);
    page = tmf_unhex(json_as_str(json_obj_get(json_obj_get(root, "config"), "page")), &n);
    fake_tmf8829_set_base_page(f, page);
    free(page);
    if (depz_device_open_link(link, &dev) == DEPZ_OK) tmf8829_session(f, dev, root, &cnt, m);
    else { g_failed++; fprintf(stderr, "  FAIL open: %s\n", depz_last_error()); }
    depz_device_close(dev);
    fake_tmf8829_stop(f);
    depz_mutex_destroy(&cnt.lock);
    free(m);
    json_free(root);
}

/* The stream counters reach chunk_errors >= want (bounded poll). */
static uint64_t tmf_chunk_errors(depz_device *dev, uint64_t want)
{
    depz_tmf8829_stream_stats st;
    int i;
    for (i = 0; i < 200; i++) {
        depz_tmf8829_get_stream_stats(dev, &st);
        if (st.chunk_errors >= want) break;
        depz_sleep_ms(5);
    }
    return st.chunk_errors;
}

/* Contract 14 §2 on the reader side: a frame chunk or an event shorter than
 * its header is counted in chunk_errors and dropped; an unsubscribed event
 * callback is not called again; NULL callbacks / outputs and a device of
 * another class are refused (test_tmf8829_device.py). */
static void tmf8829_damaged_reports_and_refusals(void)
{
    static const uint8_t ev_int[12] = {0x10, 0x27, 0, 0, 0, 0, 0, 0, DEPZ_TMF8829_INT_MOTION, DEPZ_TMF8829_EV_INT, 0, 0};
    static const uint8_t short_chunk[20] = {0}; /* the chunk header alone is 21 bytes */
    peer p;
    depz_device *dev, *plain;
    depz_link *a, *b;
    depz_tmf8829_measurement *m = (depz_tmf8829_measurement *)malloc(sizeof *m);
    depz_tmf8829_stream_stats st;
    tmf_counts cnt;
    int tok_ev = -1, tok_fr = -1, tok_ms = -1, events_seen, frames_seen;
    CHECK(m);
    CHECK(peer_up(&p, &dev));
    CHECK_RC(depz_tmf8829_attach(dev), DEPZ_OK);
    CHECK_RC(depz_tmf8829_attach(dev), DEPZ_OK); /* already one: kept */
    CHECK(depz_is_tmf8829(dev) && !depz_tmf8829_streaming(dev));
    memset(&cnt, 0, sizeof cnt);
    depz_mutex_init(&cnt.lock);

    CHECK_RC(depz_tmf8829_on_measurement(dev, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_tmf8829_on_frame(dev, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_tmf8829_on_event(dev, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_tmf8829_on_measurement(dev, tmf_meas_cb, &cnt, &tok_ms), DEPZ_OK);
    CHECK_RC(depz_tmf8829_on_frame(dev, tmf_frame_cb, &cnt, &tok_fr), DEPZ_OK);
    CHECK_RC(depz_tmf8829_on_event(dev, tmf_event_cb, &cnt, &tok_ev), DEPZ_OK);
    CHECK_RC(depz_tmf8829_get_measurement(dev, 10, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_tmf8829_config_page(dev, NULL), DEPZ_E_ARG);

    peer_send(&p, DEPZ_TMF8829_RPT_EVENT, ev_int, sizeof ev_int);
    CHECK(tmf_count(&cnt, &cnt.events, 1) == 1);
    depz_tmf8829_off_event(dev, tok_ev);
    depz_tmf8829_off_frame(dev, tok_fr);
    depz_tmf8829_off_measurement(dev, tok_ms);
    peer_send(&p, DEPZ_TMF8829_RPT_EVENT, ev_int, sizeof ev_int);  /* nobody listens now */
    peer_send(&p, DEPZ_TMF8829_RPT_EVENT, ev_int, 11);             /* one byte short */
    peer_send(&p, DEPZ_TMF8829_RPT_FRAME, short_chunk, sizeof short_chunk);
    CHECK(tmf_chunk_errors(dev, 2) == 2);
    depz_tmf8829_get_stream_stats(dev, &st);
    CHECK(st.frames_completed == 0 && st.measurements == 0 && st.frame_parse_errors == 0);
    depz_mutex_lock(&cnt.lock);
    events_seen = cnt.events;
    frames_seen = cnt.frames;
    depz_mutex_unlock(&cnt.lock);
    CHECK(events_seen == 1 && frames_seen == 0); /* in order on the reader: the 2nd event came before the errors */
    CHECK_RC(depz_tmf8829_get_measurement(dev, 20, m), DEPZ_E_TIMEOUT); /* not streaming, nothing comes */
    peer_down(&p, dev);

    /* a device of another class */
    CHECK_RC(depz_link_loopback_pair(&a, &b), DEPZ_OK);
    CHECK_RC(depz_device_open_link(a, &plain), DEPZ_OK);
    CHECK(depz_tmf8829_measurements(plain, 4) == NULL);
    CHECK_RC(depz_tmf8829_on_event(plain, tmf_event_cb, &cnt, NULL), DEPZ_E_WRONG_TYPE);
    CHECK_RC(depz_tmf8829_get_measurement(plain, 10, m), DEPZ_E_WRONG_TYPE);
    depz_tmf8829_off_event(plain, 0); /* no-ops on another class */
    depz_tmf8829_off_frame(plain, 0);
    depz_tmf8829_off_measurement(plain, 0);
    memset(&st, 0xAB, sizeof st);
    depz_tmf8829_get_stream_stats(plain, &st);
    CHECK(st.chunk_errors == 0 && st.measurements == 0 && st.frames_completed == 0);
    depz_device_close(plain);
    depz_link_free(b);
    depz_mutex_destroy(&cnt.lock);
    free(m);
}

/* init() checks the sensor ID before the bootloader (no TMF8829 on the
 * bus); configure() refuses an unnamed or too-wide field before anything is
 * written; config_page() reads back the page LOAD_CONFIG_PAGE loaded. */
static void tmf8829_init_and_configure_refusals(void)
{
    depz_link *link;
    depz_device *dev = NULL;
    fake_tmf8829 *f = fake_tmf8829_start(&link);
    depz_tmf8829_options opt = DEPZ_TMF8829_OPTIONS_INIT;
    depz_tmf8829_field_value fv[1];
    depz_tmf8829_config cfg;
    const depz_tmf8829_cfg_field *dual = depz_tmf8829_find_cfg_field("dual_mode");
    uint8_t base[190], page[190];
    char want[96];
    size_t i;
    CHECK(f && dual && dual->width == 2);
    for (i = 0; i < sizeof base; i++) base[i] = (uint8_t)(i * 7u + 3u);
    fake_tmf8829_set_base_page(f, base);
    CHECK_RC(depz_tmf8829_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 1000);

    fake_tmf8829_set_reg(f, DEPZ_TMF8829_REG_ID, 0x00);
    CHECK_RC(depz_tmf8829_init(dev, NULL, NULL, NULL, 0, NULL), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "sensor ID 0x00, expected 0x9E") != NULL);
    fake_tmf8829_set_reg(f, DEPZ_TMF8829_REG_ID, DEPZ_TMF8829_DEVICE_ID);
    CHECK_RC(depz_tmf8829_init(dev, NULL, NULL, NULL, 0, NULL), DEPZ_OK);

    fv[0].name = NULL;
    fv[0].value = 1;
    opt.fields = fv;
    opt.n_fields = 1;
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", &opt, NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "field 0 has no name") != NULL);
    opt.fields = NULL; /* n_fields with no array: the same refusal */
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", &opt, NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "field 0 has no name") != NULL);
    fv[0].name = "dual_mode";
    fv[0].value = 4; /* 2 bits hold 0..3 */
    opt.fields = fv;
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", &opt, NULL), DEPZ_E_ARG);
    snprintf(want, sizeof want, "dual_mode: 4 out of range for %u bit", (unsigned)dual->width);
    CHECK(strstr(depz_last_error(), want) != NULL);
    CHECK_RC(depz_tmf8829_get_config(dev, &cfg), DEPZ_E_ARG); /* nothing configured yet */

    /* "8x8" loads the base page with fp_mode / kilo_iterations of the preset */
    CHECK_RC(depz_tmf8829_configure(dev, "8x8", NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_tmf8829_config_page(dev, page), DEPZ_OK);
    fake_tmf8829_page(f, base);
    CHECK(memcmp(page, base, sizeof page) == 0);
    CHECK(fake_tmf8829_reg(f, DEPZ_TMF8829_REG_CID_RID) == DEPZ_TMF8829_CMD_LOAD_CONFIG_PAGE);
    depz_device_close(dev);
    fake_tmf8829_stop(f);
}

/* ── VL53L 1D family against the fake APP_VL53L0_4 bridge ────────────── */

/* The C side of test_vl53lx_device.py and test_vl53l1_uld.py: the die ULDs
 * (L4CD, L3CX ULP, L1X) over fake_vl53lx, the same register file and the same
 * numbers as the Python fake. Blocks are vectors/vl53lx.json die_block. */

#define VLX_L4CD_BOARD "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL"
#define VLX_L1CX_BOARD "DEPZ ToF Sensor VL53L1CX USB v2.1 SER"
#define VLX_L3CX_BOARD "DEPZ ToF Sensor VL53L3CX USB v2.1 SER"

/* die_block vl53l4cx_uld_as_l4cd_50ms_f2: 624 mm, 5160 kcps, 211 SPADs */
static const uint8_t VLX_L4_BLOCK[17] = {0x09, 0x09, 0x01, 0xd3, 0x20, 0x02, 0x85, 0x00, 0x89,
                                         0x00, 0x08, 0x2b, 0xa7, 0x02, 0x70, 0x02, 0x85};
/* die_block vl53l1cx_uld_long_33ms_f2: 576 mm, 16072 kcps, 67 SPADs */
static const uint8_t VLX_L1_BLOCK[17] = {0x09, 0x09, 0x01, 0x43, 0x90, 0x07, 0xd9, 0x01, 0x3f,
                                         0x00, 0x0d, 0x22, 0x34, 0x02, 0x40, 0x07, 0xd9};

typedef struct {
    fake_vl53lx *fake;
    depz_device *dev;
} vlxrig;

/* The board, opened and init()-ed with the defaults (the product from the
 * board name), the packet log cleared — Python's `ready` fixture. */
static bool vlx_up(vlxrig *r, const char *board)
{
    depz_link *link;
    r->dev = NULL;
    r->fake = fake_vl53lx_start(board, &link);
    if (!r->fake) return false;
    if (depz_vl53lx_open_link(link, &r->dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(r->dev, 500);
    if (depz_vl53lx_init(r->dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_NONE) != DEPZ_OK) {
        fprintf(stderr, "  FAIL vl53lx init: %s\n", depz_last_error());
        return false;
    }
    fake_vl53lx_clear(r->fake);
    return true;
}

static void vlx_down(vlxrig *r)
{
    depz_device_close(r->dev);
    fake_vl53lx_stop(r->fake);
}

#define VLX_RIG(name, board)                           \
    static void name##_body(vlxrig *r);                \
    static void name(void)                             \
    {                                                  \
        vlxrig r;                                      \
        if (!vlx_up(&r, board)) g_failed++;            \
        else name##_body(&r);                          \
        vlx_down(&r);                                  \
    }                                                  \
    static void name##_body(vlxrig *r)

typedef struct { uint16_t addr; size_t n; uint8_t d[4]; } vlx_w;

/* The WRITE_REG payloads first .. first+n of the log are exactly `want`. */
static bool vlx_writes_are(fake_vl53lx *f, size_t first, const vlx_w *want, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        uint8_t p[300];
        size_t len = fake_vl53lx_sent(f, 0x33, first + i, p, sizeof p);
        if (len != 2 + want[i].n || (uint16_t)(p[0] | p[1] << 8) != want[i].addr ||
            memcmp(p + 2, want[i].d, want[i].n) != 0) {
            fprintf(stderr, "  write %zu: 0x%04X+%zu, want 0x%04X+%zu\n", first + i,
                    len >= 2 ? (unsigned)(p[0] | p[1] << 8) : 0u, len >= 2 ? len - 2 : 0,
                    (unsigned)want[i].addr, want[i].n);
            return false;
        }
    }
    return true;
}

/* How many WRITE_REGs of the log started at `addr`. */
static size_t vlx_writes_to(fake_vl53lx *f, uint16_t addr)
{
    size_t i, n = 0, total = fake_vl53lx_count(f, 0x33);
    for (i = 0; i < total; i++) {
        uint8_t p[300];
        if (fake_vl53lx_sent(f, 0x33, i, p, sizeof p) >= 2 && (uint16_t)(p[0] | p[1] << 8) == addr) n++;
    }
    return n;
}

/* VL53L1X ULD: 15 ms exists in short mode only — going back to long keeps
 * the period and takes the nearest budget long has, 20 ms. */
VLX_RIG(vlx_l1_mode_change_moves_the_budget, VLX_L1CX_BOARD)
{
    static const vlx_w want[8] = {
        {0x004B, 1, {0x0A}}, {0x0060, 1, {0x0F}}, {0x0063, 1, {0x0D}}, {0x0069, 1, {0xB8}},
        {0x0078, 2, {0x0F, 0x0D}}, {0x007A, 2, {0x0E, 0x0E}},
        {0x005E, 2, {0x00, 0x1E}}, {0x0061, 2, {0x00, 0x22}}};
    static const int long_budgets[6] = {20, 33, 50, 100, 200, 500};
    const char *names[4], *mode = NULL;
    int choices[8], b = 0, i = 0, lo = 0, hi = 0;
    size_t n = 0, k;
    CHECK(depz_vl53lx_product_bound(r->dev) == DEPZ_VL53LX_PRODUCT_L1CX);
    CHECK(depz_vl53lx_driver_bound(r->dev) == DEPZ_VL53LX_DRIVER_ULD);
    CHECK(depz_vl53lx_modes(r->dev, names, 4) == 2 && !strcmp(names[0], "long") && !strcmp(names[1], "short"));
    CHECK_RC(depz_vl53lx_budget_range(r->dev, &lo, &hi), DEPZ_OK);
    CHECK(lo == 15 && hi == 500);
    CHECK_RC(depz_vl53lx_get_mode(r->dev, &mode), DEPZ_OK);
    CHECK(mode && !strcmp(mode, "long"));
    /* the 0xB8 phase window of the long blob at the die's 0xBCCC oscillator */
    CHECK(depz_vl53lx_driver_reach_mm(r->dev) == 4563);
    CHECK_RC(depz_vl53lx_configure(r->dev, 15, 0, "short", NULL, NULL), DEPZ_OK);
    CHECK_RC(depz_vl53lx_get_range_timing(r->dev, &b, &i), DEPZ_OK);
    CHECK(b == 15 && i == 15);
    CHECK_RC(depz_vl53lx_budget_choices(r->dev, choices, 8, &n), DEPZ_OK);
    CHECK(n == 7 && choices[0] == 15);
    CHECK(depz_vl53lx_driver_reach_mm(r->dev) == 1388); /* short: the 0x38 window */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_set_mode(r->dev, "long", NULL), DEPZ_OK);
    CHECK_RC(depz_vl53lx_get_mode(r->dev, &mode), DEPZ_OK);
    CHECK(!strcmp(mode, "long"));
    CHECK_RC(depz_vl53lx_budget_choices(r->dev, choices, 8, &n), DEPZ_OK);
    CHECK(n == 6);
    for (k = 0; k < 6; k++) CHECK(choices[k] == long_budgets[k]);
    CHECK_RC(depz_vl53lx_get_range_timing(r->dev, &b, &i), DEPZ_OK);
    CHECK(b == 20 && i == 15);
    CHECK(vlx_writes_are(r->fake, 0, want, 8));
    CHECK_RC(depz_vl53lx_set_mode(r->dev, "medium", NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "no such mode") != NULL);
    /* a budget the mode does not have is refused, not rounded */
    CHECK_RC(depz_vl53lx_configure(r->dev, 40, 0, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "40 ms is not one the long mode has") != NULL);
}

VLX_RIG(vlx_l1_unreadable_mode_byte_is_an_error, VLX_L1CX_BOARD)
{
    const char *mode = NULL;
    fake_vl53lx_set_reg(r->fake, 0x004B, 0x00); /* a part fresh out of reset */
    CHECK_RC(depz_vl53lx_get_mode(r->dev, &mode), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "reads 0x00, which is neither distance mode") != NULL);
}

/* VL53L1X_SetDistanceThreshold: the window bits rewritten, the others kept. */
VLX_RIG(vlx_l1_thresholds_keep_the_other_bits, VLX_L1CX_BOARD)
{
    static const vlx_w want[3] = {{0x0046, 1, {0x93}}, {0x0072, 2, {0x01, 0x2C}}, {0x0074, 2, {0x00, 0x64}}};
    int lo = 0, hi = 0, w = 0;
    CHECK(fake_vl53lx_reg(r->fake, 0x0046) == 0x20); /* the blob's byte */
    fake_vl53lx_set_reg(r->fake, 0x0046, 0x90);
    CHECK_RC(depz_vl53lx_set_detection_thresholds(r->dev, 100, 300, 3), DEPZ_OK);
    CHECK(fake_vl53lx_count(r->fake, 0x33) == 3 && vlx_writes_are(r->fake, 0, want, 3));
    CHECK_RC(depz_vl53lx_get_detection_thresholds(r->dev, &lo, &hi, &w), DEPZ_OK);
    CHECK(lo == 100 && hi == 300 && w == 3);
}

/* VL53L1X_calibration.c on the L1 block (576 mm): no heat loop, the three
 * offsets zeroed, timed start, 5 frames each released, the L1X stop code. */
VLX_RIG(vlx_l1_offset_calibration, VLX_L1CX_BOARD)
{
    static const vlx_w head[4] = {{0x001E, 2, {0, 0}}, {0x0020, 2, {0, 0}}, {0x0022, 2, {0, 0}}, {0x0087, 1, {0x40}}};
    static const vlx_w tail[2] = {{0x0087, 1, {0x00}}, {0x001E, 2, {0x00, 0x60}}};
    static const vlx_w clr = {0x0086, 1, {0x01}};
    int32_t off = 0;
    size_t k;
    fake_vl53lx_set_block(r->fake, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 600, 5, &off), DEPZ_OK);
    CHECK(off == 24 && fake_vl53lx_word(r->fake, 0x001E) == 96);
    CHECK(fake_vl53lx_count(r->fake, 0x33) == 11 && vlx_writes_are(r->fake, 0, head, 4));
    for (k = 0; k < 5; k++) CHECK(vlx_writes_are(r->fake, 4 + k, &clr, 1));
    CHECK(vlx_writes_are(r->fake, 9, tail, 2));
    CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &off), DEPZ_OK);
    CHECK(off == 24);
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 500, 5, &off), DEPZ_OK);
    CHECK(off == -76 && fake_vl53lx_word(r->fake, 0x001E) == ((-76 * 4) & 0xFFFF));
    CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &off), DEPZ_OK);
    CHECK(off == -76);
}

/* 512 x 16072 x (1 - 576/650) / 67 = 13982.6 -> 13982, 27 kcps rounded. */
VLX_RIG(vlx_l1_xtalk_calibration, VLX_L1CX_BOARD)
{
    static const vlx_w head[2] = {{0x0016, 2, {0, 0}}, {0x0087, 1, {0x40}}};
    static const vlx_w tail[2] = {{0x0087, 1, {0x00}}, {0x0016, 2, {0x36, 0x9E}}};
    uint8_t no_spads[17];
    int32_t x = -1;
    size_t n;
    fake_vl53lx_set_block(r->fake, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 650, 5, &x), DEPZ_OK);
    CHECK(x == 27 && fake_vl53lx_word(r->fake, 0x0016) == 13982);
    n = fake_vl53lx_count(r->fake, 0x33);
    CHECK(vlx_writes_are(r->fake, 0, head, 2) && vlx_writes_are(r->fake, n - 2, tail, 2));
    CHECK(vlx_writes_to(r->fake, 0x0086) == 5);
    CHECK_RC(depz_vl53lx_get_xtalk_kcps(r->dev, &x), DEPZ_OK);
    CHECK(x == 27);
    /* a target nearer than the reading: 0, no compensation */
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 500, 5, &x), DEPZ_OK);
    CHECK(x == 0 && fake_vl53lx_word(r->fake, 0x0016) == 0);
    memcpy(no_spads, VLX_L1_BLOCK, sizeof no_spads);
    no_spads[3] = no_spads[4] = 0; /* RESULT__DSS_ACTUAL_EFFECTIVE_SPADS_SD0 = 0 */
    fake_vl53lx_set_block(r->fake, no_spads, sizeof no_spads);
    fake_vl53lx_set_reg(r->fake, 0x0016, 0x12);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 650, 5, &x), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "no SPADs enabled") != NULL);
    CHECK(fake_vl53lx_word(r->fake, 0x0016) == 0); /* left disabled */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 600, 4, &x), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "at least 5") != NULL);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 600, 4, &x), DEPZ_E_ARG);
    CHECK(fake_vl53lx_rx_count(r->fake) == 0);
}

VLX_RIG(vlx_l1_roi_size_and_centre, VLX_L1CX_BOARD)
{
    int x = 0, y = 0, c = 0;
    CHECK_RC(depz_vl53lx_set_roi(r->dev, 8, 6), DEPZ_OK);
    CHECK(fake_vl53lx_reg(r->fake, 0x007F) == 0x91); /* the optical centre the die reports */
    CHECK(fake_vl53lx_reg(r->fake, 0x0080) == 0x57);
    CHECK_RC(depz_vl53lx_get_roi(r->dev, &x, &y), DEPZ_OK);
    CHECK(x == 8 && y == 6);
    CHECK_RC(depz_vl53lx_set_roi(r->dev, 16, 16), DEPZ_OK); /* over 10: the centre moves to 199 */
    CHECK_RC(depz_vl53lx_get_roi_center(r->dev, &c), DEPZ_OK);
    CHECK(c == 199);
    CHECK_RC(depz_vl53lx_set_roi_center(r->dev, 0x88), DEPZ_OK);
    CHECK_RC(depz_vl53lx_get_roi_center(r->dev, &c), DEPZ_OK);
    CHECK(c == 0x88);
}

/* Streamed die blocks decode with the L1 variant; a report shorter than its
 * own length field and a block the driver cannot decode are counted and the
 * reader goes on. */
VLX_RIG(vlx_l1_stream_decodes_and_counts_bad_blocks, VLX_L1CX_BOARD)
{
    static const uint8_t start_die[9] = {0x89, 0x00, 0x11, 0x00, 0x00, 0x01, 0x86, 0x00, 0x01};
    depz_vl53lx_measurement m;
    depz_vl53lx_info info;
    depz_stream *s;
    uint8_t p[16];
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_OK);
    CHECK(depz_vl53lx_ranging(r->dev));
    CHECK(fake_vl53lx_sent(r->fake, 0x35, 0, p, sizeof p) == 9 && memcmp(p, start_die, 9) == 0);
    s = depz_vl53lx_measurements(r->dev, 4);
    CHECK(s);
    fake_vl53lx_stream(r->fake, 1, 17, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
    CHECK_RC(depz_stream_next(s, &m, 1000), DEPZ_OK);
    depz_stream_close(s);
    CHECK(m.timestamp_us == 1 && m.distance_mm == 576 && m.status == 0 && !strcmp(m.status_text, "valid"));
    CHECK(m.sigma_mm == 3 && m.signal_kcps == 16072 && m.ambient_kcps == 2552 && m.spads == 67);
    CHECK(m.signal_per_spad_kcps == 23 && m.ambient_per_spad_kcps == 3 && m.stream_count == 1);
    CHECK(depz_vl53lx_plottable(&m));
    fake_vl53lx_stream(r->fake, 2, 17, VLX_L1_BLOCK, 4); /* shorter than its length field */
    fake_vl53lx_stream(r->fake, 3, 4, VLX_L1_BLOCK, 4);  /* 4 B: no die block */
    CHECK_RC(depz_vl53lx_bridge_info(r->dev, &info), DEPZ_OK); /* behind the reports */
    CHECK(depz_vl53lx_stream_parse_errors(r->dev) == 2);
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_stop_ranging(r->dev), DEPZ_OK);
    CHECK(!depz_vl53lx_ranging(r->dev));
    CHECK(fake_vl53lx_count(r->fake, 0x36) == 1);
    CHECK(fake_vl53lx_reg(r->fake, 0x0087) == 0x00); /* the L1X stop code */
    CHECK_RC(depz_vl53lx_stop_ranging(r->dev), DEPZ_OK); /* idempotent */
    CHECK(fake_vl53lx_count(r->fake, 0x36) == 1);
}

/* xshut() is allowed while ranging (as Python): it stops ranging and frees
 * the driver while the reader may be decoding a stream report with it (C-1:
 * use-after-free — TSan / ASan). */
typedef struct { fake_vl53lx *fake; depz_mutex lock; bool stop; unsigned sent; } vlx_streamer;

static void vlx_stream_until_stopped(void *a)
{
    vlx_streamer *x = (vlx_streamer *)a;
    uint64_t ts = 1;
    for (;;) {
        bool stop;
        fake_vl53lx_stream(x->fake, ts++, 17, VLX_L1_BLOCK, sizeof VLX_L1_BLOCK);
        depz_mutex_lock(&x->lock);
        x->sent++;
        stop = x->stop;
        depz_mutex_unlock(&x->lock);
        if (stop) break;
    }
}

static unsigned vlx_streamed(vlx_streamer *x)
{
    unsigned n;
    depz_mutex_lock(&x->lock);
    n = x->sent;
    depz_mutex_unlock(&x->lock);
    return n;
}

VLX_RIG(vlx_xshut_while_streaming, VLX_L1CX_BOARD)
{
    vlx_streamer x;
    depz_thread t;
    depz_vl53lx_measurement m;
    depz_vl53lx_info info;
    depz_stream *s;
    uint64_t deadline;
    uint16_t id = 0;
    unsigned at_xshut;
    int rc;
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_OK);
    s = depz_vl53lx_measurements(r->dev, 4);
    CHECK(s);
    memset(&x, 0, sizeof x);
    x.fake = r->fake;
    CHECK(depz_mutex_init(&x.lock) == 0);
    CHECK(depz_thread_start(&t, vlx_stream_until_stopped, &x) == 0);
    rc = depz_stream_next(s, &m, 1000); /* the reader is decoding */
    if (rc == DEPZ_OK) rc = depz_vl53lx_xshut(r->dev, DEPZ_VL53L4_XSHUT_OFF);
    /* reports keep coming with no driver to decode them (bounded) */
    at_xshut = vlx_streamed(&x);
    deadline = depz_host_now_us() + 2000000u;
    while (vlx_streamed(&x) < at_xshut + 200 && depz_host_now_us() < deadline) depz_sleep_ms(1);
    if (rc == DEPZ_OK) rc = depz_vl53lx_bridge_info(r->dev, &info); /* the reader is alive */
    depz_mutex_lock(&x.lock);
    x.stop = true;
    depz_mutex_unlock(&x.lock);
    depz_thread_join(t);
    depz_mutex_destroy(&x.lock);
    depz_stream_close(s);
    CHECK_RC(rc, DEPZ_OK);
    CHECK(!depz_vl53lx_ranging(r->dev) && !depz_vl53lx_initialized(r->dev));
    CHECK_RC(depz_vl53lx_model_id(r->dev, &id), DEPZ_E_ARG); /* init() again first */
}

/* configure() re-inits, then timing, offset, xtalk, signal threshold
 * (vectors/vl53l4.json tuning: -10 -> 65496, 20 -> 10240, 1024 -> 128). */
VLX_RIG(vlx_l4_configure_reinits_then_tunes, VLX_L4CD_BOARD)
{
    static const vlx_w tail[7] = {
        {0x001E, 2, {0xFF, 0xD8}}, {0x0020, 2, {0, 0}}, {0x0022, 2, {0, 0}}, {0x0018, 2, {0, 0}},
        {0x001A, 2, {0, 0}}, {0x0016, 2, {0x28, 0x00}}, {0x0066, 2, {0x00, 0x80}}};
    const int32_t off = -10, xt = 20;
    const int sig = 1024;
    int32_t v = 0;
    int b = 0, i = -1, k = 0, lo = 0, hi = 0;
    const char *mode = "x";
    uint16_t id = 0;
    size_t n;
    CHECK(depz_vl53lx_detected_product(r->dev) == DEPZ_VL53LX_PRODUCT_L4CD);
    CHECK_RC(depz_vl53lx_model_id(r->dev, &id), DEPZ_OK);
    CHECK(id == 0xEBAA);
    CHECK_RC(depz_vl53lx_budget_range(r->dev, &lo, &hi), DEPZ_OK);
    CHECK(lo == 10 && hi == 200);
    CHECK(depz_vl53lx_driver_reach_mm(r->dev) == 1388); /* the L4CD blob's 0x38 window */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_configure_ex(r->dev, 33, 0, NULL, &off, &xt, &sig), DEPZ_OK);
    CHECK(fake_vl53lx_count(r->fake, 0x3A) == 1); /* the re-init clears the I2C errors */
    n = fake_vl53lx_count(r->fake, 0x33);
    CHECK(n >= 7 && vlx_writes_are(r->fake, n - 7, tail, 7));
    CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &v), DEPZ_OK);
    CHECK(v == -10);
    CHECK_RC(depz_vl53lx_get_xtalk_kcps(r->dev, &v), DEPZ_OK);
    CHECK(v == 20);
    CHECK_RC(depz_vl53lx_get_signal_threshold_kcps(r->dev, &k), DEPZ_OK);
    CHECK(k == 1024);
    CHECK_RC(depz_vl53lx_get_range_timing(r->dev, &b, &i), DEPZ_OK);
    CHECK(i == 0 && abs(b - 33) <= 1);
    CHECK_RC(depz_vl53lx_get_mode(r->dev, &mode), DEPZ_OK);
    CHECK(mode == NULL);
    CHECK_RC(depz_vl53lx_configure(r->dev, 50, 20, NULL, NULL, NULL), DEPZ_E_ARG); /* period <= budget */
    CHECK_RC(depz_vl53lx_configure(r->dev, 300, 0, NULL, NULL, NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "10..200") != NULL);
    /* a mode on a product without modes is refused before the re-init */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_configure(r->dev, 50, 0, "long", NULL, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_vl53lx_set_mode(r->dev, "long", NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "VL53L4CD/uld does not support 'mode'") != NULL);
    CHECK(fake_vl53lx_rx_count(r->fake) == 0);
}

/* A period selects autonomous mode: INTERMEASUREMENT_MS = 1.055 x period x PLL. */
VLX_RIG(vlx_l4_period_selects_autonomous_mode, VLX_L4CD_BOARD)
{
    static const vlx_w start = {0x0087, 1, {0x40}};
    static const uint8_t start_die[9] = {0x89, 0x00, 0x11, 0x00, 0x00, 0x01, 0x86, 0x00, 0x01};
    uint8_t p[16];
    int b = 0, i = 0;
    CHECK_RC(depz_vl53lx_configure(r->dev, 50, 100, NULL, NULL, NULL), DEPZ_OK);
    CHECK(fake_vl53lx_dword(r->fake, 0x006C) == (uint32_t)(1.055 * 100 * (FAKE_VLX_DIE_PLL & 0x3FF)));
    CHECK_RC(depz_vl53lx_get_range_timing(r->dev, &b, &i), DEPZ_OK);
    CHECK(i == 99 && abs(b - 50) <= 1); /* ST's 1.055 / 1.065 pair reads one ms short */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_OK);
    CHECK(fake_vl53lx_count(r->fake, 0x33) == 1 && vlx_writes_are(r->fake, 0, &start, 1));
    CHECK(fake_vl53lx_sent(r->fake, 0x35, 0, p, sizeof p) == 9 && memcmp(p, start_die, 9) == 0);
    /* the stream owns the bus: configuration and register writes are refused */
    {
        int32_t v = 0;
        uint8_t reg = 0;
        depz_vl53lx_measurement m;
        CHECK_RC(depz_vl53lx_set_offset_mm(r->dev, 5), DEPZ_E_ARG);
        CHECK(strstr(depz_last_error(), "stop ranging first") != NULL);
        CHECK_RC(depz_vl53lx_configure(r->dev, 50, 0, NULL, NULL, NULL), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_init(r->dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_NONE), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 100, 5, &v), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_write_reg(r->dev, 0x0087, &reg, 1), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_measure_once(r->dev, -1, &m), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_E_ARG);
        CHECK_RC(depz_vl53lx_read_reg(r->dev, 0x0087, &reg, 1), DEPZ_OK); /* reads are fine */
        CHECK(reg == 0x40);
        CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &v), DEPZ_OK);
    }
    CHECK_RC(depz_vl53lx_stop_ranging(r->dev), DEPZ_OK);
    CHECK(fake_vl53lx_reg(r->fake, 0x0087) == 0x80); /* the L4CD stop code */
}

/* START_STREAM refused after the driver started the die: the caller gets the
 * START_STREAM error, the host says not ranging, so the die is stopped again
 * (the L4CD stop code 0x80 into SYSTEM_START) and configuration works
 * afterwards (PY-2/TS-5). */
VLX_RIG(vlx_a_refused_start_stream_stops_the_sensor_again, VLX_L4CD_BOARD)
{
    static const vlx_w stop = {0x0087, 1, {0x80}};
    size_t n;
    fake_vl53lx_clear(r->fake);
    fake_vl53lx_fail_next(r->fake, 0x35, 0x03);
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_E_STATUS);
    CHECK(depz_last_status_cmd() == 0x35 && depz_last_status() == 0x03); /* not an error of the stop */
    CHECK(!depz_vl53lx_ranging(r->dev));
    CHECK(fake_vl53lx_count(r->fake, 0x35) == 1);
    n = fake_vl53lx_count(r->fake, 0x33);
    CHECK(n >= 2 && vlx_writes_are(r->fake, n - 1, &stop, 1)); /* the die stopped again */
    CHECK(fake_vl53lx_reg(r->fake, 0x0087) == 0x80);
    CHECK_RC(depz_vl53lx_set_offset_mm(r->dev, 0), DEPZ_OK); /* configuration allowed again */
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_OK);
    CHECK(depz_vl53lx_ranging(r->dev));
    CHECK_RC(depz_vl53lx_stop_ranging(r->dev), DEPZ_OK);
}

VLX_RIG(vlx_l4_tuning_round_trips, VLX_L4CD_BOARD)
{
    int32_t v = 0;
    int lo = 0, hi = 0, w = 0, k = 0, x = 0, y = 0;
    uint32_t cnt = 0;
    bool ap = false;
    CHECK_RC(depz_vl53lx_set_offset_mm(r->dev, 10), DEPZ_OK);
    CHECK(fake_vl53lx_word(r->fake, 0x001E) == 40);
    CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &v), DEPZ_OK);
    CHECK(v == 10);
    CHECK_RC(depz_vl53lx_set_xtalk_kcps(r->dev, 3), DEPZ_OK);
    CHECK(fake_vl53lx_word(r->fake, 0x0016) == 1536);
    CHECK_RC(depz_vl53lx_get_xtalk_kcps(r->dev, &v), DEPZ_OK);
    CHECK(v == 3);
    CHECK_RC(depz_vl53lx_set_detection_thresholds(r->dev, 100, 300, 3), DEPZ_OK);
    CHECK(fake_vl53lx_reg(r->fake, 0x0046) == 3);
    CHECK(fake_vl53lx_word(r->fake, 0x0072) == 300 && fake_vl53lx_word(r->fake, 0x0074) == 100);
    CHECK_RC(depz_vl53lx_get_detection_thresholds(r->dev, &lo, &hi, &w), DEPZ_OK);
    CHECK(lo == 100 && hi == 300 && w == 3);
    CHECK_RC(depz_vl53lx_set_signal_threshold_kcps(r->dev, 2048), DEPZ_OK);
    CHECK(fake_vl53lx_word(r->fake, 0x0066) == 256);
    CHECK_RC(depz_vl53lx_get_signal_threshold_kcps(r->dev, &k), DEPZ_OK);
    CHECK(k == 2048);
    CHECK_RC(depz_vl53lx_set_sigma_threshold_mm(r->dev, 15), DEPZ_OK);
    CHECK(fake_vl53lx_word(r->fake, 0x0064) == 60);
    CHECK_RC(depz_vl53lx_get_sigma_threshold_mm(r->dev, &k), DEPZ_OK);
    CHECK(k == 15);
    CHECK_RC(depz_vl53lx_set_sigma_threshold_mm(r->dev, 16384), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "16383") != NULL);
    /* the groups the L4CD ULD lacks are refused with no traffic */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_get_roi(r->dev, &x, &y), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "VL53L4CD/uld does not support 'roi'") != NULL);
    CHECK_RC(depz_vl53lx_set_roi(r->dev, 4, 4), DEPZ_E_ARG);
    CHECK_RC(depz_vl53lx_get_roi_center(r->dev, &k), DEPZ_E_ARG);
    CHECK_RC(depz_vl53lx_set_roi_center(r->dev, 199), DEPZ_E_ARG);
    CHECK_RC(depz_vl53lx_perform_ref_spad_management(r->dev, &cnt, &ap), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "'refspad'") != NULL);
    CHECK(fake_vl53lx_rx_count(r->fake) == 0);
}

VLX_RIG(vlx_l4_temperature_update_reruns_vhv, VLX_L4CD_BOARD)
{
    static const vlx_w want[7] = {
        {0x0008, 1, {0x81}}, {0x000B, 1, {0x92}}, {0x0087, 1, {0x21}}, {0x0086, 1, {0x01}},
        {0x0087, 1, {0x80}}, {0x0008, 1, {0x09}}, {0x000B, 1, {0x00}}};
    CHECK_RC(depz_vl53lx_start_temperature_update(r->dev), DEPZ_OK);
    CHECK(fake_vl53lx_count(r->fake, 0x33) == 7 && vlx_writes_are(r->fake, 0, want, 7));
}

/* VL53L4CD_calibration.c on the L4 block (624 mm, 5160 kcps, 211 SPADs):
 * 10 heat-loop frames, then the samples. */
VLX_RIG(vlx_l4_calibrations, VLX_L4CD_BOARD)
{
    static const vlx_w head[3] = {{0x001E, 2, {0, 0}}, {0x0020, 2, {0, 0}}, {0x0022, 2, {0, 0}}};
    uint8_t bad[17];
    int32_t v = 0;
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 600, 4, &v), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "5..255") != NULL);
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 1001, 0, &v), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "10..1000") != NULL);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 5001, 0, &v), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "10..5000") != NULL);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 600, 256, &v), DEPZ_E_ARG);
    CHECK(fake_vl53lx_rx_count(r->fake) == 0);
    fake_vl53lx_set_block(r->fake, VLX_L4_BLOCK, sizeof VLX_L4_BLOCK);
    CHECK_RC(depz_vl53lx_calibrate_offset(r->dev, 600, 5, &v), DEPZ_OK);
    CHECK(v == -24 && fake_vl53lx_word(r->fake, 0x001E) == (-96 & 0xFFFF));
    CHECK(vlx_writes_are(r->fake, 0, head, 3) && vlx_writes_to(r->fake, 0x0086) == 15);
    CHECK_RC(depz_vl53lx_get_offset_mm(r->dev, &v), DEPZ_OK);
    CHECK(v == -24);
    /* (1 - 624/650) x 5160/211 = 0.978 kcps -> x512 */
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 650, 5, &v), DEPZ_OK);
    CHECK(v == 1 && fake_vl53lx_word(r->fake, 0x0016) == 500);
    memcpy(bad, VLX_L4_BLOCK, sizeof bad);
    bad[0] = 0x04; /* raw 4 -> status 2, signal below threshold */
    fake_vl53lx_set_block(r->fake, bad, sizeof bad);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 650, 5, &v), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "no valid samples") != NULL);
    memcpy(bad, VLX_L4_BLOCK, sizeof bad);
    bad[3] = 0x01; bad[4] = 0x00; bad[5] = 0xFF; bad[6] = 0xFF; /* 1 SPAD, 524280 kcps */
    fake_vl53lx_set_block(r->fake, bad, sizeof bad);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 5000, 5, &v), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "> 127") != NULL);
}

/* measure_once: start, wait for data ready, read the block, release, stop. */
VLX_RIG(vlx_l4_measure_once_polls_the_block, VLX_L4CD_BOARD)
{
    static const vlx_w want[3] = {{0x0087, 1, {0x21}}, {0x0086, 1, {0x01}}, {0x0087, 1, {0x80}}};
    depz_vl53lx_measurement m;
    int32_t d = 0;
    fake_vl53lx_set_block(r->fake, VLX_L4_BLOCK, sizeof VLX_L4_BLOCK);
    CHECK_RC(depz_vl53lx_measure_once(r->dev, -1, &m), DEPZ_OK);
    CHECK(m.distance_mm == 624 && m.status == 0 && !strcmp(m.status_text, "valid"));
    CHECK(m.signal_kcps == 5160 && m.spads == 211 && m.stream_count == 1 && m.n_targets == 0);
    CHECK(m.timestamp_us >= 5000000); /* the MCU time of the block read */
    CHECK(depz_vl53lx_primary_distance(&m, &d) && d == 624);
    CHECK(fake_vl53lx_count(r->fake, 0x33) == 3 && vlx_writes_are(r->fake, 0, want, 3));
    /* data ready never comes: still stopped */
    fake_vl53lx_set_reg(r->fake, 0x0031, 0x01); /* INT active low, never asserted */
    fake_vl53lx_clear(r->fake);
    CHECK_RC(depz_vl53lx_measure_once(r->dev, 5, &m), DEPZ_E_TIMEOUT);
    CHECK(strstr(depz_last_error(), "data ready") != NULL);
    CHECK(fake_vl53lx_reg(r->fake, 0x0087) == 0x80);
}

/* The L3CX ULP: no offset / calibrations (the caveat names why), ROI yes. */
VLX_RIG(vlx_l3_refusals_name_the_caveat, VLX_L3CX_BOARD)
{
    const int32_t off = 5;
    int32_t v = 0;
    int x = 0, y = 0;
    CHECK(depz_vl53lx_driver_bound(r->dev) == DEPZ_VL53LX_DRIVER_ULP);
    CHECK(strstr(depz_vl53lx_caveat(r->dev), "single-target ranging only") != NULL);
    CHECK(depz_vl53lx_supports(r->dev, DEPZ_VL53LX_CAP_ROI | DEPZ_VL53LX_CAP_SIGMA_THRESH));
    CHECK(!depz_vl53lx_supports(r->dev, DEPZ_VL53LX_CAP_OFFSET));
    CHECK_RC(depz_vl53lx_configure(r->dev, 33, 0, NULL, &off, NULL), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "VL53L3CX/ulp does not support 'offset' (single-target ranging only") != NULL);
    CHECK_RC(depz_vl53lx_calibrate_xtalk(r->dev, 600, 0, &v), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "'calib_xtalk'") != NULL);
    CHECK_RC(depz_vl53lx_start_temperature_update(r->dev), DEPZ_E_ARG);
    CHECK(fake_vl53lx_rx_count(r->fake) == 0);
    CHECK(fake_vl53lx_reg(r->fake, 0x004B) == 0x03); /* init_extra: PHASECAL_CONFIG__TIMEOUT_MACROP */
    CHECK_RC(depz_vl53lx_set_roi(r->dev, 4, 4), DEPZ_OK);
    CHECK(fake_vl53lx_reg(r->fake, 0x0080) == 0x33);
    CHECK_RC(depz_vl53lx_get_roi(r->dev, &x, &y), DEPZ_OK);
    CHECK(x == 4 && y == 4);
    CHECK_RC(depz_vl53lx_start_ranging(r->dev), DEPZ_OK);
    CHECK_RC(depz_vl53lx_stop_ranging(r->dev), DEPZ_OK);
    CHECK(fake_vl53lx_reg(r->fake, 0x0087) == 0x00); /* the ULP stop code */
}

/* The (product, driver) table and the bridge's refusals at init (contract
 * 12 §3): a pair without a row and an unstamped board are refused before any
 * traffic; a v2.00 bridge is told to reflash; a soft reset the die does not
 * ACK falls back to XSHUT; xshut() drops the driver. */
static void vlx_init_binding_and_bridge_refusals(void)
{
    depz_link *link;
    depz_device *dev = NULL;
    fake_vl53lx *f;
    uint8_t p[8];
    uint16_t id = 0;
    CHECK(depz_vl53lx_driver_kinds(DEPZ_VL53LX_PRODUCT_L3CX) == (DEPZ_VL53LX_DRIVER_ULP | DEPZ_VL53LX_DRIVER_HISTOGRAM));
    CHECK(depz_vl53lx_driver_kinds(DEPZ_VL53LX_PRODUCT_L4CX) == DEPZ_VL53LX_DRIVER_HISTOGRAM);
    CHECK(depz_vl53lx_driver_kinds(DEPZ_VL53LX_PRODUCT_L0X) == DEPZ_VL53LX_DRIVER_ULD);
    CHECK(depz_vl53lx_status_plottable(0) && depz_vl53lx_status_plottable(6) && depz_vl53lx_status_plottable(11));
    CHECK(!depz_vl53lx_status_plottable(4) && !depz_vl53lx_primary_distance(NULL, NULL));
    {
        /* the first plottable target in driver order, or nothing */
        depz_vl53lx_measurement m;
        int32_t d = 0;
        memset(&m, 0, sizeof m);
        m.n_targets = 2;
        m.targets[0].distance_mm = 100; m.targets[0].status = 4;
        m.targets[1].distance_mm = 250; m.targets[1].status = 11;
        CHECK(depz_vl53lx_primary_distance(&m, &d) && d == 250);
        m.targets[1].status = 7;
        CHECK(!depz_vl53lx_primary_distance(&m, &d));
        m.n_targets = 0;
        m.status = 4;
        CHECK(!depz_vl53lx_primary_distance(&m, &d));
    }

    f = fake_vl53lx_start("DEPZ ToF Sensor USB v2.1", &link); /* unstamped */
    CHECK(f);
    CHECK_RC(depz_vl53lx_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 500);
    CHECK(depz_is_vl53lx(dev) && !depz_vl53lx_initialized(dev));
    CHECK(depz_vl53lx_get_class(dev) == DEPZ_VL53LX_CLASS_GENERIC);
    CHECK_RC(depz_vl53lx_model_id(dev, &id), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "init") != NULL);
    CHECK_RC(depz_vl53lx_start_ranging(dev), DEPZ_E_ARG);
    CHECK(fake_vl53lx_rx_count(f) == 0);
    CHECK_RC(depz_vl53lx_init(dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_NONE), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "carries no product number") != NULL);
    CHECK(fake_vl53lx_count(f, 0x03) == 1 && fake_vl53lx_count(f, 0x33) == 0);
    fake_vl53lx_clear(f);
    CHECK_RC(depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_PRODUCT_L4CX), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "VL53L4CX has no 'uld' driver - it has histogram") != NULL);
    CHECK(fake_vl53lx_rx_count(f) == 0);
    /* a v2.00 bridge does not know CLEAR_I2C_ERRORS */
    fake_vl53lx_fail_next(f, 0x3A, 0x02);
    CHECK_RC(depz_vl53lx_init(dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_L4CD), DEPZ_E_STATUS);
    CHECK(strstr(depz_last_error(), "predates APP_VL53L0_4_v0.24") != NULL);
    CHECK(!depz_vl53lx_initialized(dev));
    /* SET_ADDR_WIDTH goes first, with 2 for the die */
    fake_vl53lx_clear(f);
    fake_vl53lx_fail_next(f, 0x39, 0x04);
    CHECK_RC(depz_vl53lx_init(dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_L4CD), DEPZ_E_STATUS);
    CHECK(fake_vl53lx_rx_count(f) == 1 && fake_vl53lx_sent(f, 0x39, 0, p, sizeof p) == 1 && p[0] == 2);
    /* the L1X ULD resets first; a soft reset the die NACKs falls back to XSHUT */
    fake_vl53lx_clear(f);
    fake_vl53lx_fail_next(f, 0x33, 0x09);
    CHECK_RC(depz_vl53lx_init(dev, DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_PRODUCT_L1CX), DEPZ_OK);
    CHECK(fake_vl53lx_count(f, 0x34) == 1 && fake_vl53lx_sent(f, 0x34, 0, p, sizeof p) == 1 && p[0] == 2);
    CHECK(depz_vl53lx_product_bound(dev) == DEPZ_VL53LX_PRODUCT_L1CX);
    CHECK_RC(depz_vl53lx_start_ranging(dev), DEPZ_OK);
    fake_vl53lx_clear(f);
    CHECK_RC(depz_vl53lx_xshut(dev, DEPZ_VL53L4_XSHUT_OFF), DEPZ_OK);
    CHECK(fake_vl53lx_sent(f, 0x34, 0, p, sizeof p) == 1 && p[0] == 0);
    CHECK(!depz_vl53lx_ranging(dev) && !depz_vl53lx_initialized(dev));
    CHECK_RC(depz_vl53lx_model_id(dev, &id), DEPZ_E_ARG);
    /* a die that never boots: FIRMWARE__SYSTEM_STATUS stays 0 */
    fake_vl53lx_set_reg(f, 0x00E5, 0x00);
    CHECK_RC(depz_vl53lx_init(dev, (depz_vl53lx_driver)0, DEPZ_VL53LX_PRODUCT_L4CD), DEPZ_E_TIMEOUT);
    CHECK(strstr(depz_last_error(), "FIRMWARE__SYSTEM_STATUS") != NULL);
    CHECK_RC(depz_sr04_start(dev), DEPZ_E_WRONG_TYPE);
    depz_device_close(dev);
    fake_vl53lx_stop(f);
}

/* ── BNO055 against the fake register bridge + sensor model ──────────── */

/* The C side of test_bno055_device.py over fake_bno055 (the twin of
 * fake_bno055.py): the request sequences, as "page:addr=hex" writes. */

typedef struct {
    fake_bno055 *fake;
    depz_device *dev;
    char w[2048];   /* the last fake_bno055_writes() */
} b055rig;

static bool b055_up(b055rig *r)
{
    depz_link *link;
    r->dev = NULL;
    r->fake = fake_bno055_start(&link);
    if (!r->fake) return false;
    if (depz_bno055_open_link(link, &r->dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(r->dev, 300);
    return true;
}

static void b055_down(b055rig *r)
{
    depz_device_close(r->dev);
    fake_bno055_stop(r->fake);
}

#define B055_RIG(name)                                 \
    static void name##_body(b055rig *r);               \
    static void name(void)                             \
    {                                                  \
        static b055rig r;                              \
        if (!b055_up(&r)) g_failed++;                  \
        else name##_body(&r);                          \
        b055_down(&r);                                 \
    }                                                  \
    static void name##_body(b055rig *r)

/* The writes since the last clear are exactly `want`. */
static bool b055_writes_are(b055rig *r, const char *want)
{
    fake_bno055_writes(r->fake, r->w, sizeof r->w);
    if (strcmp(r->w, want) == 0) return true;
    fprintf(stderr, "  writes: \"%s\"\n  want:   \"%s\"\n", r->w, want);
    return false;
}

static bool near3(const double *v, double a, double b, double c)
{
    return fabs(v[0] - a) < 1e-9 && fabs(v[1] - b) < 1e-9 && fabs(v[2] - c) < 1e-9;
}

static const depz_bno055_units B055_ALL_UNITS = {true, true, true, true, true}; /* UNIT_SEL 0x97 */
/* vectors/bno055.json calibration_profile extremes / typical */
static const char *const B055_EXTREMES = "0cfef4010000001900e70100d00730f8ffff00080005";
static const char *const B055_TYPICAL = "eaff0d00e1ff45ff9201a0fffffffeff0000e8036f02";

static void b055_profile(const char *hex, depz_bno055_calib_profile *out)
{
    uint8_t b[DEPZ_BNO055_CALIB_PROFILE_LEN];
    size_t k;
    for (k = 0; k < sizeof b; k++) {
        unsigned v = 0;
        if (sscanf(hex + 2 * k, "%2x", &v) != 1) v = 0;
        b[k] = (uint8_t)v;
    }
    depz_bno055_unpack_calib_profile(b, sizeof b, out);
}

B055_RIG(b055_identity_and_raw_registers)
{
    static const uint8_t live_info[38] = {
        0x28, 0xa0, 0xfb, 0x32, 0x0f, 0x11, 0x03, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x01, 0x2b, 0x09, 0xd1,
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x00};
    depz_bno055_info info;
    uint8_t b[200], p[8], info2[38];
    bool alive = false;
    size_t k;
    CHECK(depz_is_bno055(r->dev) && !depz_is_bno086(r->dev));
    CHECK_RC(depz_bno055_bridge_info(r->dev, &info), DEPZ_OK);
    CHECK(info.chip_id == 0xA0 && info.sw_rev == 0x0311 && info.initialized == 1);
    CHECK(fake_bno055_sent(r->fake, 0x37) == 1 && fake_bno055_sent(r->fake, 0x32) == 0);
    CHECK_RC(depz_bno055_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(alive);
    memcpy(info2, live_info, sizeof info2);
    info2[8] = 0; /* initialized = 0 */
    fake_bno055_set_info(r->fake, info2, sizeof info2);
    CHECK_RC(depz_bno055_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(!alive);
    info2[8] = 1;
    info2[1] = 0xA1; /* wrong chip id */
    fake_bno055_set_info(r->fake, info2, sizeof info2);
    CHECK_RC(depz_bno055_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(!alive);
    fake_bno055_set_info(r->fake, live_info, sizeof live_info);
    fake_bno055_fail_next(r->fake, 0x37, 0x08); /* an error is "not alive", not a throw */
    CHECK_RC(depz_bno055_is_alive(r->dev, &alive), DEPZ_OK);
    CHECK(!alive);

    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 4, 0), DEPZ_OK);
    CHECK(b[0] == 0xA0 && b[1] == 0xFB && b[2] == 0x32 && b[3] == 0x0F);
    CHECK(fake_bno055_payload(r->fake, 0x32, 0, p, sizeof p) == 2 && p[0] == 0x00 && p[1] == 0x04);
    b[0] = 1; b[1] = 2;
    CHECK_RC(depz_bno055_write_registers(r->dev, 0x55, b, 2, 0), DEPZ_OK);
    CHECK(fake_bno055_reg(r->fake, 0, 0x55) == 1 && fake_bno055_reg(r->fake, 0, 0x56) == 2);
    /* transfers over 128 bytes are split */
    for (k = 0; k < 200; k++) fake_bno055_set_reg(r->fake, 1, (uint8_t)(0x10 + k), (uint8_t)k);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x10, b, 200, 1), DEPZ_OK);
    for (k = 0; k < 200; k++) CHECK(b[k] == (uint8_t)k);
    CHECK(fake_bno055_sent(r->fake, 0x32) == 2);
    CHECK(fake_bno055_payload(r->fake, 0x32, 0, p, sizeof p) == 2 && p[0] == 0x10 && p[1] == 128);
    CHECK(fake_bno055_payload(r->fake, 0x32, 1, p, sizeof p) == 2 && p[0] == 0x90 && p[1] == 72);
    CHECK(fake_bno055_page(r->fake) == 0);
    /* page 1 is wrapped in PAGE_ID writes, also when the access fails */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x50, b, 1, 1), DEPZ_OK);
    CHECK(b[0] == 0x40); /* page 1 0x50, written by the split transfer above */
    CHECK(b055_writes_are(r, "0:07=01 1:07=00"));
    fake_bno055_clear(r->fake);
    fake_bno055_fail_next(r->fake, 0x32, 0x09);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x50, b, 1, 1), DEPZ_E_STATUS);
    CHECK(b055_writes_are(r, "0:07=01 1:07=00") && fake_bno055_page(r->fake) == 0);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 1, 0), DEPZ_OK); /* not wedged */
    CHECK(b[0] == 0xA0);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 1, 2), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "pages 0 and 1") != NULL);
    CHECK(fake_bno055_sent(r->fake, 0x33) + fake_bno055_sent(r->fake, 0x32) == 0);
    fake_bno055_short_read(r->fake, 1);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 4, 0), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "expected 4, got 3") != NULL);
    fake_bno055_silent(r->fake, 0x32, true);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 1, 0), DEPZ_E_TIMEOUT);
    fake_bno055_silent(r->fake, 0x32, false);
    CHECK_RC(depz_sr04_start(r->dev), DEPZ_E_WRONG_TYPE);
}

B055_RIG(b055_operation_and_power_modes)
{
    uint8_t mode = 0xFF;
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x1C);
    CHECK_RC(depz_bno055_get_operation_mode(r->dev, &mode), DEPZ_OK);
    CHECK(mode == DEPZ_BNO055_MODE_NDOF); /* bits 7:4 masked */
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    /* CONFIG -> NDOF waits for a nonzero quaternion: two zero reads, then the live one */
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, DEPZ_BNO055_MODE_NDOF), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=0c") && fake_bno055_reads(r->fake, 0x20) == 3);
    CHECK(fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    /* mode -> mode goes through CONFIG */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, DEPZ_BNO055_MODE_AMG), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:3d=07") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_AMG);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, DEPZ_BNO055_MODE_AMG), DEPZ_OK); /* the same mode */
    CHECK(b055_writes_are(r, ""));
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, DEPZ_BNO055_MODE_CONFIG), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00") && fake_bno055_reads(r->fake, 0x3D) == 0);
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, 0x0D), DEPZ_E_ARG);

    /* power (§4.6): suspend from NDOF, wake into NDOF */
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_SUSPEND), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:3e=02") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_CONFIG);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_NORMAL), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3e=00 0:3d=0c") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    CHECK_RC(depz_bno055_get_power_mode(r->dev, &mode), DEPZ_OK);
    CHECK(mode == DEPZ_BNO055_POWER_NORMAL);
    /* from AMG: wakes into AMG */
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_AMG);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_SUSPEND), DEPZ_OK);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_LOW), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3e=01 0:3d=07"));
    /* low power keeps the operating mode */
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_IMU);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_LOW), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:3e=01 0:3d=08") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_IMU);
    /* CONFIG and awake stays in CONFIG */
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_NORMAL), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3e=00") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_CONFIG);
}

/* A chip found asleep in CONFIG wakes into NDOF; a plain switch into a
 * fusion that never starts gives up waiting quietly (after its 1 s). */
B055_RIG(b055_found_asleep_and_stuck_fusion)
{
    uint64_t t0;
    fake_bno055_set_reg(r->fake, 0, 0x3E, DEPZ_BNO055_POWER_SUSPEND);
    CHECK_RC(depz_bno055_set_power_mode(r->dev, DEPZ_BNO055_POWER_NORMAL), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3e=00 0:3d=0c") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_fusion_stuck(r->fake, true);
    fake_bno055_clear(r->fake);
    t0 = depz_host_now_us();
    CHECK_RC(depz_bno055_set_operation_mode(r->dev, DEPZ_BNO055_MODE_IMU), DEPZ_OK);
    CHECK(fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_IMU);
    /* polled until the deadline, not once: the time, not a read count a slow
     * runner cannot reach in 10 ms steps */
    CHECK(depz_host_now_us() - t0 >= 900000 && fake_bno055_reads(r->fake, 0x20) >= 2);
    /* configure() is strict about it */
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, NULL), DEPZ_E_TIMEOUT);
    CHECK(strstr(depz_last_error(), "fusion did not start") != NULL);
}

B055_RIG(b055_units_axes_temperature_source)
{
    depz_bno055_units u;
    depz_bno055_axis_remap ax, p0;
    uint8_t src = 0;
    CHECK_RC(depz_bno055_get_units(r->dev, &u), DEPZ_OK);
    CHECK(u.android && !u.accel_mg && !u.gyro_rps && !u.euler_rad && !u.temp_f); /* power-on 0x80 */
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_units(r->dev, &B055_ALL_UNITS), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:3b=97 0:3d=0c"));
    CHECK_RC(depz_bno055_get_units(r->dev, &u), DEPZ_OK);
    CHECK(u.accel_mg && u.gyro_rps && u.euler_rad && u.temp_f && u.android);
    /* the previous mode comes back when the CONFIG-side write fails */
    fake_bno055_clear(r->fake);
    fake_bno055_fail_write_reg(r->fake, 0x3B, 0x09);
    memset(&u, 0, sizeof u);
    CHECK_RC(depz_bno055_set_units(r->dev, &u), DEPZ_E_STATUS);
    CHECK(b055_writes_are(r, "0:3d=00 0:3d=0c") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    /* axis remap by placement name: the write_reg_axis_map_p0 payload 41 21 04 */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_axis_placement(r->dev, "p0"), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:41=2104 0:3d=0c"));
    {
        uint8_t p[8];
        size_t k, n = fake_bno055_sent(r->fake, 0x33);
        bool seen = false;
        for (k = 0; k < n; k++)
            if (fake_bno055_payload(r->fake, 0x33, k, p, sizeof p) == 3 && p[0] == 0x41 && p[1] == 0x21 && p[2] == 0x04)
                seen = true;
        CHECK(seen);
    }
    CHECK_RC(depz_bno055_get_axis_remap(r->dev, &ax), DEPZ_OK);
    CHECK(depz_bno055_placement("P0", &p0) == 0);
    CHECK(ax.x == p0.x && ax.y == p0.y && ax.z == p0.z && ax.x_negative == p0.x_negative &&
          ax.y_negative == p0.y_negative && ax.z_negative == p0.z_negative);
    /* a non-permutation and an unknown placement are refused before any traffic */
    fake_bno055_clear(r->fake);
    memset(&ax, 0, sizeof ax);
    ax.z = 1;
    CHECK_RC(depz_bno055_set_axis_remap(r->dev, &ax), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "permutation") != NULL);
    CHECK_RC(depz_bno055_set_axis_placement(r->dev, "P9"), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "P0") != NULL);
    CHECK(fake_bno055_sent(r->fake, 0x33) + fake_bno055_sent(r->fake, 0x32) == 0);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_temperature_source(r->dev, 1), DEPZ_OK); /* gyro */
    CHECK(b055_writes_are(r, "0:3d=00 0:40=01 0:3d=0c"));
    CHECK_RC(depz_bno055_get_temperature_source(r->dev, &src), DEPZ_OK);
    CHECK(src == 1);
}

B055_RIG(b055_configure_restore_and_reset)
{
    depz_bno055_calib_profile prof;
    depz_bno055_axis_remap p2;
    char want[256], first[256];
    uint8_t cal[DEPZ_BNO055_CALIB_PROFILE_LEN];
    size_t k;
    b055_profile(B055_EXTREMES, &prof);
    CHECK(depz_bno055_placement("P2", &p2) == 0);
    CHECK_RC(depz_bno055_restore_configuration(r->dev), DEPZ_E_ARG); /* configure() first */
    fake_bno055_sys_status(r->fake, 3, 2); /* still booting for two polls */
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_NDOF, &B055_ALL_UNITS, &p2, &prof), DEPZ_OK);
    depz_bno055_pack_calib_profile(&prof, cal);
    snprintf(want, sizeof want, "0:3d=00 0:3b=97 0:41=2406 0:55=%s 0:3d=0c", B055_EXTREMES);
    CHECK(b055_writes_are(r, want));
    for (k = 0; k < sizeof cal; k++) CHECK(fake_bno055_reg(r->fake, 0, (uint8_t)(0x55 + k)) == cal[k]);
    CHECK(fake_bno055_sys_status_left(r->fake) == 0);
    CHECK(fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF && fake_bno055_reg(r->fake, 0, 0x3B) == 0x97);
    strcpy(first, r->w);
    /* reset_sensor: back to the power-on registers, the stream off */
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN,
                                      DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    CHECK(depz_bno055_streaming(r->dev) && fake_bno055_streaming(r->fake));
    fake_bno055_sys_status(r->fake, 2, 2);
    CHECK_RC(depz_bno055_reset_sensor(r->dev), DEPZ_OK);
    CHECK(fake_bno055_sent(r->fake, 0x34) == 1 && fake_bno055_reset_count(r->fake) == 1);
    CHECK(!fake_bno055_streaming(r->fake) && !depz_bno055_streaming(r->dev));
    CHECK(fake_bno055_sys_status_left(r->fake) == 0 && fake_bno055_reg(r->fake, 0, 0x3B) == 0x80);
    {
        /* the units cache was dropped: power-on 0x80 is read back */
        depz_bno055_sample s;
        CHECK_RC(depz_bno055_read_sample(r->dev, 0x34, 1, &s), DEPZ_OK);
        CHECK(s.units.android && !s.units.accel_mg);
    }
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_restore_configuration(r->dev), DEPZ_OK);
    CHECK(b055_writes_are(r, first) && fake_bno055_reg(r->fake, 0, 0x3B) == 0x97);
    fake_bno055_fail_next(r->fake, 0x34, 0x09);
    CHECK_RC(depz_bno055_reset_sensor(r->dev), DEPZ_E_STATUS);
    /* defaults from NDOF */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, NULL), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:3b=00 0:3d=0c"));
    /* a non-fusion mode does not wait for the quaternion */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_AMG, NULL, NULL, NULL), DEPZ_OK);
    CHECK(fake_bno055_reads(r->fake, 0x20) == 0 && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_AMG);
    CHECK_RC(depz_bno055_configure(r->dev, 0x0D, NULL, NULL, NULL), DEPZ_E_ARG);
}

B055_RIG(b055_configure_boot_states)
{
    /* SYS_STATUS 4 for 20 polls is a self-test leftover: out of CONFIG and back */
    fake_bno055_sys_status(r->fake, 4, 20);
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_AMG, NULL, NULL, NULL), DEPZ_OK);
    fake_bno055_writes(r->fake, r->w, sizeof r->w);
    CHECK(strncmp(r->w, "0:3d=01 0:3d=00 ", 16) == 0);
    CHECK(fake_bno055_reads(r->fake, 0x39) == 21);
    /* a sensor that never leaves boot: refused, nothing written */
    fake_bno055_clear(r->fake);
    fake_bno055_sys_status(r->fake, 3, 100000);
    CHECK_RC(depz_bno055_configure(r->dev, DEPZ_BNO055_MODE_NDOF, NULL, NULL, NULL), DEPZ_E_TIMEOUT);
    CHECK(strstr(depz_last_error(), "did not finish booting") != NULL);
    CHECK(b055_writes_are(r, ""));
}

B055_RIG(b055_status_self_test_calibration)
{
    depz_bno055_status_regs st;
    depz_bno055_calib_status cs;
    depz_bno055_calib_profile prof, typ;
    int16_t sic[9];
    char want[160];
    size_t k;
    fake_bno055_set_reg(r->fake, 0, 0x37, 0x40);
    fake_bno055_set_reg(r->fake, 0, 0x38, 1);
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
    CHECK_RC(depz_bno055_system_status(r->dev, &st), DEPZ_OK);
    CHECK(st.self_test == 0x0F && st.clk_status == 1 && st.status == 5 && st.error == 0);
    CHECK(fake_bno055_reg(r->fake, 0, 0x37) == 0x40); /* INT_STA never read */
    CHECK(fake_bno055_reads(r->fake, 0x36) == 1 && fake_bno055_reads(r->fake, 0x38) == 1 &&
          fake_bno055_reads(r->fake, 0x37) == 0);
    /* self-test from NDOF keeps CLK_SEL and comes back to NDOF */
    fake_bno055_set_reg(r->fake, 0, 0x3F, 0x80);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_self_test(r->dev, &st), DEPZ_OK);
    CHECK(st.self_test == 0x0F && st.error == 0);
    CHECK(b055_writes_are(r, "0:3d=00 0:3f=81 0:3d=0c"));
    CHECK(fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF && fake_bno055_reg(r->fake, 0, 0x3F) == 0x80);
    /* the trigger write fails: the mode is still restored */
    fake_bno055_clear(r->fake);
    fake_bno055_fail_write_reg(r->fake, 0x3F, 0x09);
    CHECK_RC(depz_bno055_self_test(r->dev, &st), DEPZ_E_STATUS);
    CHECK(b055_writes_are(r, "0:3d=00 0:3d=0c") && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    /* in CONFIG: the leftover SYS_STATUS 4 is cleared through ACCONLY */
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_set_reg(r->fake, 0, 0x3F, 0x00);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_self_test(r->dev, &st), DEPZ_OK);
    CHECK(st.status == 4); /* as the sensor reports it right after the test */
    CHECK(b055_writes_are(r, "0:3f=01 0:3d=01 0:3d=00"));
    CHECK_RC(depz_bno055_system_status(r->dev, &st), DEPZ_OK);
    CHECK(st.status == 0);
    /* calibration status */
    fake_bno055_set_reg(r->fake, 0, 0x35, 0xFF);
    CHECK_RC(depz_bno055_calibration_status(r->dev, &cs), DEPZ_OK);
    CHECK(depz_bno055_fully_calibrated(&cs));
    fake_bno055_set_reg(r->fake, 0, 0x35, 0xC4);
    CHECK_RC(depz_bno055_calibration_status(r->dev, &cs), DEPZ_OK);
    CHECK(cs.system == 3 && cs.gyro == 0 && cs.accel == 1 && cs.mag == 0);
    /* the calibration profile round trip, in CONFIG */
    b055_profile(B055_EXTREMES, &prof);
    {
        uint8_t b[DEPZ_BNO055_CALIB_PROFILE_LEN];
        depz_bno055_calib_profile got;
        depz_bno055_pack_calib_profile(&prof, b);
        for (k = 0; k < sizeof b; k++) fake_bno055_set_reg(r->fake, 0, (uint8_t)(0x55 + k), b[k]);
        fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
        fake_bno055_clear(r->fake);
        CHECK_RC(depz_bno055_read_calibration_profile(r->dev, &got), DEPZ_OK);
        CHECK(memcmp(&got, &prof, sizeof got) == 0);
        CHECK(b055_writes_are(r, "0:3d=00 0:3d=0c") && fake_bno055_reads(r->fake, 0x55) == 1);
    }
    b055_profile(B055_TYPICAL, &typ);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_write_calibration_profile(r->dev, &typ), DEPZ_OK);
    snprintf(want, sizeof want, "0:3d=00 0:55=%s 0:3d=0c", B055_TYPICAL);
    CHECK(b055_writes_are(r, want));
    /* the soft-iron matrix: identity is 1.0 = 16384 on the diagonal */
    for (k = 0; k < 9; k++) sic[k] = (int16_t)(k % 4 == 0 ? 16384 : 0);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_sic_matrix(r->dev, sic), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:43=004000000000000000400000000000000040 0:3d=0c"));
    memset(sic, 0, sizeof sic);
    CHECK_RC(depz_bno055_get_sic_matrix(r->dev, sic), DEPZ_OK);
    CHECK(sic[0] == 16384 && sic[4] == 16384 && sic[8] == 16384 && sic[1] == 0 && sic[7] == 0);
    /* refused while streaming, nothing sent */
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN,
                                      DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_self_test(r->dev, &st), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "stop the stream first") != NULL);
    CHECK(fake_bno055_sent(r->fake, 0x32) + fake_bno055_sent(r->fake, 0x33) == 0);
}

B055_RIG(b055_page1_configs_and_interrupts)
{
    depz_bno055_accel_config ac;
    depz_bno055_gyro_config gc;
    depz_bno055_mag_config mc;
    uint8_t uid[16], v = 0;
    size_t k;
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
    CHECK_RC(depz_bno055_get_accel_config(r->dev, &ac), DEPZ_OK);
    CHECK(ac.range == 1 && ac.bandwidth == 3 && ac.power == 0);
    fake_bno055_clear(r->fake);
    ac.range = 3; ac.bandwidth = 7; ac.power = 0;
    CHECK_RC(depz_bno055_set_accel_config(r->dev, &ac), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:07=01 1:08=1f 1:07=00 0:3d=0c"));
    CHECK(fake_bno055_page(r->fake) == 0 && fake_bno055_opr_mode(r->fake) == DEPZ_BNO055_MODE_NDOF);
    CHECK_RC(depz_bno055_get_gyro_config(r->dev, &gc), DEPZ_OK);
    CHECK(gc.range == 0 && gc.bandwidth == 7 && gc.power == 0);
    CHECK_RC(depz_bno055_get_mag_config(r->dev, &mc), DEPZ_OK);
    CHECK(mc.rate == 5 && mc.mode == 1 && mc.power == 3);
    gc.range = 2; gc.bandwidth = 3; gc.power = 1;
    mc.rate = 7; mc.mode = 3; mc.power = 0;
    CHECK_RC(depz_bno055_set_gyro_config(r->dev, &gc), DEPZ_OK);
    CHECK_RC(depz_bno055_set_mag_config(r->dev, &mc), DEPZ_OK);
    CHECK(fake_bno055_reg(r->fake, 1, 0x0A) == 0x1A && fake_bno055_reg(r->fake, 1, 0x0B) == 0x01 &&
          fake_bno055_reg(r->fake, 1, 0x09) == 0x1F);
    memset(&gc, 0, sizeof gc);
    memset(&mc, 0, sizeof mc);
    CHECK_RC(depz_bno055_get_gyro_config(r->dev, &gc), DEPZ_OK);
    CHECK(gc.range == 2 && gc.bandwidth == 3 && gc.power == 1);
    CHECK_RC(depz_bno055_get_mag_config(r->dev, &mc), DEPZ_OK);
    CHECK(mc.rate == 7 && mc.mode == 3 && mc.power == 0);
    CHECK_RC(depz_bno055_unique_id(r->dev, uid), DEPZ_OK);
    for (k = 0; k < 16; k++) CHECK(uid[k] == (uint8_t)(0xC0 + k));
    /* INT_EN / INT_MSK skip the CONFIG round trip */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_interrupt_enable(r->dev, 0x40), DEPZ_OK);
    CHECK_RC(depz_bno055_set_interrupt_mask(r->dev, 0xC0), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:07=01 1:10=40 1:07=00 0:07=01 1:0f=c0 1:07=00"));
    CHECK_RC(depz_bno055_get_interrupt_enable(r->dev, &v), DEPZ_OK);
    CHECK(v == 0x40);
    CHECK_RC(depz_bno055_get_interrupt_mask(r->dev, &v), DEPZ_OK);
    CHECK(v == 0xC0);
    /* interrupt settings: page 1 0x11..0x1F only */
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_set_interrupt_setting(r->dev, 0x10, 1), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_set_interrupt_setting(r->dev, 0x20, 1), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "interrupt setting") != NULL);
    CHECK(fake_bno055_sent(r->fake, 0x33) + fake_bno055_sent(r->fake, 0x32) == 0);
    fake_bno055_set_reg(r->fake, 0, 0x3D, 0x00);
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_AMG);
    CHECK_RC(depz_bno055_set_interrupt_setting(r->dev, 0x11, 0x0A), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3d=00 0:07=01 1:11=0a 1:07=00 0:3d=07"));
    /* INT_STA clears on read; clear_interrupt keeps CLK_SEL */
    fake_bno055_set_reg(r->fake, 0, 0x37, 0x40);
    fake_bno055_set_reg(r->fake, 0, 0x3F, 0x80);
    CHECK_RC(depz_bno055_read_interrupt_status(r->dev, &v), DEPZ_OK);
    CHECK(v == 0x40);
    CHECK_RC(depz_bno055_read_interrupt_status(r->dev, &v), DEPZ_OK);
    CHECK(v == 0);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_clear_interrupt(r->dev), DEPZ_OK);
    CHECK(b055_writes_are(r, "0:3f=c0") && fake_bno055_reg(r->fake, 0, 0x3F) == 0x80);
    /* page 1 is refused while streaming, nothing sent */
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN,
                                      DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_unique_id(r->dev, uid), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_set_accel_config(r->dev, &ac), DEPZ_E_ARG);
    CHECK(fake_bno055_sent(r->fake, 0x33) + fake_bno055_sent(r->fake, 0x32) == 0);
    CHECK_RC(depz_bno055_stop_stream(r->dev), DEPZ_OK);
    CHECK_RC(depz_bno055_unique_id(r->dev, uid), DEPZ_OK);
    CHECK(uid[0] == 0xC0);
}

/* vectors/bno055.json blocks full_block_live. */
static const char *const B055_FULL_LIVE =
    "a2037fff73ff17ff4e01d100fffffcff00009f05e50464f7ba2952f9efcf0000e2ff03000400c0037bff6eff1a00";

static void b055_seed_full_block(fake_bno055 *f)
{
    size_t k;
    for (k = 0; k < DEPZ_BNO055_FULL_BLOCK_LEN; k++) {
        unsigned v = 0;
        if (sscanf(B055_FULL_LIVE + 2 * k, "%2x", &v) != 1) v = 0;
        fake_bno055_set_reg(f, 0, (uint8_t)(0x08 + k), (uint8_t)v);
    }
}

B055_RIG(b055_polling_scales_by_the_units)
{
    depz_bno055_sample s;
    double q[4];
    b055_seed_full_block(r->fake);
    CHECK_RC(depz_bno055_read_sample(r->dev, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN, &s), DEPZ_OK);
    CHECK(s.units.android && s.addr == 0x08 && s.len == 46 && s.timestamp_us > 5000000);
    CHECK(near3(s.accel, 9.30, -1.29, -1.41));
    CHECK(near3(s.mag, -233 / 16.0, 334 / 16.0, 209 / 16.0));
    CHECK(near3(s.gyro, -1 / 16.0, -4 / 16.0, 0));
    CHECK(near3(s.euler, 1439 / 16.0, 1253 / 16.0, -2204 / 16.0));
    CHECK(fabs(s.quaternion[0] - 10682 / 16384.0) < 1e-12 && fabs(s.quaternion[1] + 1710 / 16384.0) < 1e-12);
    CHECK(near3(s.linear_accel, -0.30, 0.03, 0.04) && near3(s.gravity, 9.60, -1.33, -1.46));
    CHECK(s.temperature == 26 && s.calibration.system == 0 && s.present == 0x1FF);
    CHECK_RC(depz_bno055_read_sample(r->dev, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN, &s), DEPZ_OK);
    CHECK(fake_bno055_reads(r->fake, 0x3B) == 1); /* UNIT_SEL read once, then cached */
    /* a window leaves the channels it does not cover out */
    fake_bno055_set_reg(r->fake, 0, 0x34, 0xE7); /* -25 C */
    fake_bno055_set_reg(r->fake, 0, 0x35, 0xFF);
    CHECK_RC(depz_bno055_read_sample(r->dev, 0x34, 2, &s), DEPZ_OK);
    CHECK(s.temperature == -25 && depz_bno055_fully_calibrated(&s.calibration));
    CHECK(s.present == (DEPZ_BNO055_HAS_TEMPERATURE | DEPZ_BNO055_HAS_CALIBRATION));
    CHECK_RC(depz_bno055_read_sample(r->dev, 0xF9, 8, &s), DEPZ_E_ARG);
    /* every unit bit (set_units updates the cache) */
    CHECK_RC(depz_bno055_set_units(r->dev, &B055_ALL_UNITS), DEPZ_OK);
    b055_seed_full_block(r->fake);
    CHECK_RC(depz_bno055_read_sample(r->dev, DEPZ_BNO055_FULL_BLOCK_ADDR, DEPZ_BNO055_FULL_BLOCK_LEN, &s), DEPZ_OK);
    CHECK(s.units.accel_mg && near3(s.accel, 930, -129, -141));
    CHECK(near3(s.gyro, -1 / 900.0, -4 / 900.0, 0) && near3(s.euler, 1439 / 900.0, 1253 / 900.0, -2204 / 900.0));
    CHECK(s.temperature == 52); /* F: 1 LSB = 2 F */
    CHECK(near3(s.linear_accel, -0.30, 0.03, 0.04)); /* always m/s^2 */
    /* read_quaternion reads only the 8-byte block */
    fake_bno055_set_mode(r->fake, DEPZ_BNO055_MODE_NDOF);
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_read_quaternion(r->dev, q), DEPZ_OK);
    CHECK(fabs(q[0] - 10685 / 16384.0) < 1e-12 && fabs(q[1] + 1693 / 16384.0) < 1e-12 &&
          fabs(q[2] + 12305 / 16384.0) < 1e-12 && q[3] == 0);
    CHECK(fake_bno055_sent(r->fake, 0x32) == 1 && fake_bno055_reads(r->fake, 0x20) == 1);
}

static void b055_count_cb(const depz_bno055_sample *s, void *user)
{
    (void)s;
    (*(int *)user)++;
}

B055_RIG(b055_stream_wire_units_and_bad_reports)
{
    static const uint8_t want[3][5] = {{0x00, 0x08, 0x2e, 0x0a, 0x00}, {0x00, 0x20, 0x08, 0x14, 0x00},
                                       {0x01, 0x35, 0x01, 0xe8, 0x03}};
    static const uint8_t accel[6] = {0xa2, 0x03, 0x7f, 0xff, 0x73, 0xff};
    depz_bno055_sample s;
    depz_bno055_info info;
    depz_stream *it;
    uint8_t p[8], b[1];
    int got = 0, token = -1, k;
    CHECK(!depz_bno055_streaming(r->dev));
    /* refused before any traffic: blocks outside 1..128 / the page, TIMER period 0 / 60001 */
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0x08, 0, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0x08, 129, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0xF9, 8, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_start_stream(r->dev, 0, 0x08, 46, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_ARG);
    CHECK_RC(depz_bno055_start_stream(r->dev, 60001, 0x08, 46, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_ARG);
    CHECK(fake_bno055_sent(r->fake, 0x32) + fake_bno055_sent(r->fake, 0x35) == 0 && !depz_bno055_streaming(r->dev));
    /* the vectors' start_stream payloads */
    fake_bno055_set_reg(r->fake, 0, 0x3B, 0x97);
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0x08, 46, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    CHECK(fake_bno055_sent(r->fake, 0x32) == 1); /* UNIT_SEL read before START_STREAM */
    CHECK_RC(depz_bno055_start_stream(r->dev, 20, 0x20, 8, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    CHECK_RC(depz_bno055_start_stream(r->dev, 1000, 0x35, 1, DEPZ_BNO055_TRIGGER_INT), DEPZ_OK);
    for (k = 0; k < 3; k++) CHECK(fake_bno055_payload(r->fake, 0x35, (size_t)k, p, sizeof p) == 5 && !memcmp(p, want[k], 5));
    CHECK(depz_bno055_streaming(r->dev) && fake_bno055_streaming(r->fake));
    /* the stream scales by the units latched before START_STREAM */
    CHECK_RC(depz_bno055_on_sample(r->dev, b055_count_cb, &got, &token), DEPZ_OK);
    it = depz_bno055_samples(r->dev, 4);
    CHECK(it);
    fake_bno055_push_stream(r->fake, 1, 0x08, 6, accel, 6);
    CHECK_RC(depz_stream_next(it, &s, 1000), DEPZ_OK);
    CHECK(s.timestamp_us == 1 && s.units.accel_mg && near3(s.accel, 930, -129, -141) && s.present == DEPZ_BNO055_HAS_ACCEL);
    /* shorter than the header, a truncated block: counted, dropped */
    fake_bno055_push_stream(r->fake, 2, 0x08, 46, accel, 6);
    fake_bno055_push_stream(r->fake, 3, 0x08, 6, accel, 0);
    CHECK_RC(depz_bno055_bridge_info(r->dev, &info), DEPZ_OK); /* behind the reports */
    CHECK(depz_bno055_stream_parse_errors(r->dev) == 2 && got == 1);
    CHECK_RC(depz_stream_next(it, &s, 20), DEPZ_E_TIMEOUT);
    depz_bno055_off_sample(r->dev, token);
    fake_bno055_push_stream(r->fake, 4, 0x08, 6, accel, 6);
    CHECK_RC(depz_stream_next(it, &s, 1000), DEPZ_OK);
    CHECK(s.timestamp_us == 4 && got == 1); /* unsubscribed */
    depz_stream_close(it);
    CHECK_RC(depz_bno055_get_sample(r->dev, 20, &s), DEPZ_E_TIMEOUT);
    CHECK_RC(depz_bno055_read_registers(r->dev, 0x00, b, 1, 0), DEPZ_OK); /* the link keeps working */
    CHECK(b[0] == 0xA0);
    /* stop: idle sends nothing; a failure still clears the flag */
    CHECK_RC(depz_bno055_stop_stream(r->dev), DEPZ_OK);
    CHECK(!depz_bno055_streaming(r->dev) && !fake_bno055_streaming(r->fake));
    fake_bno055_clear(r->fake);
    CHECK_RC(depz_bno055_stop_stream(r->dev), DEPZ_OK);
    CHECK(fake_bno055_sent(r->fake, 0x36) == 0);
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0x08, 46, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_OK);
    fake_bno055_fail_next(r->fake, 0x36, 0x01);
    CHECK_RC(depz_bno055_stop_stream(r->dev), DEPZ_E_STATUS);
    CHECK(!depz_bno055_streaming(r->dev));
    fake_bno055_fail_next(r->fake, 0x35, 0x08);
    CHECK_RC(depz_bno055_start_stream(r->dev, 10, 0x08, 46, DEPZ_BNO055_TRIGGER_TIMER), DEPZ_E_STATUS);
    CHECK(!depz_bno055_streaming(r->dev));
}

/* depz_open_on_link (contract 02 §4.4) over the fakes, the C side of
 * test_discovery.py's open_on_link tests: exactly the three probe requests
 * (name, device name, serial), then the class attached on the same
 * connection; an unknown APP_* stays a base device; a bootloader, a silent
 * board and a NULL output are refused. */
static void discovery_open_on_link_attaches_in_place(void)
{
    static const uint8_t probe[3] = {0x04, 0x03, 0x05};
    depz_link *link;
    depz_device *dev = NULL;
    fake_sr04 *fake;
    fake_vl53lx *vlx;
    uint8_t cmds[16];
    uint32_t period = 0;

    fake = fake_sr04_start(&link);
    CHECK(fake);
    CHECK_RC(depz_open_on_link(link, (int)DEPZ_USB_VID, -1, 1000, &dev), DEPZ_OK);
    CHECK(depz_is_sr04(dev) && depz_device_sensor_type(dev) == DEPZ_SENSOR_SR04);
    CHECK(fake_sr04_commands(fake, cmds, sizeof cmds) == 3 && memcmp(cmds, probe, 3) == 0);
    CHECK_RC(depz_sr04_get_sample_period_us(dev, &period), DEPZ_OK); /* the same connection goes on */
    CHECK(period == fake_sr04_sample_period(fake));
    depz_device_close(dev);
    fake_sr04_stop(fake);

    fake = fake_sr04_start(&link);
    CHECK(fake);
    fake_sr04_set_software(fake, "APP_NEWSENSOR_v1.2");
    CHECK_RC(depz_open_on_link(link, -1, -1, 1000, &dev), DEPZ_OK);
    CHECK(!depz_is_sr04(dev) && depz_device_sensor_type(dev) == DEPZ_SENSOR_UNKNOWN);
    CHECK(fake_sr04_commands(fake, cmds, sizeof cmds) == 3 && memcmp(cmds, probe, 3) == 0);
    depz_device_close(dev);
    fake_sr04_stop(fake);

    fake = fake_sr04_start(&link);
    CHECK(fake);
    fake_sr04_set_software(fake, "BOOTDEPZ_v1.1");
    dev = (depz_device *)&period; /* overwritten with NULL on failure */
    CHECK_RC(depz_open_on_link(link, -1, -1, 1000, &dev), DEPZ_E_BOOTLOADER);
    CHECK(dev == NULL && strstr(depz_last_error(), "bootloader") != NULL);
    fake_sr04_stop(fake);

    fake = fake_silent_start(&link);
    CHECK(fake);
    CHECK_RC(depz_open_on_link(link, -1, -1, 50, &dev), DEPZ_E_NO_DEVICE);
    CHECK(dev == NULL && strstr(depz_last_error(), "no DEPZ device answered on") != NULL);
    fake_sr04_stop(fake);

    fake = fake_sr04_start(&link);
    CHECK(fake);
    CHECK_RC(depz_open_on_link(link, -1, -1, 1000, NULL), DEPZ_E_ARG); /* takes the link anyway */
    fake_sr04_stop(fake);

    /* The 1D family: the board stamp names the class, the product follows it. */
    vlx = fake_vl53lx_start("DEPZ ToF Sensor VL53L1CX USB v2.1 SER", &link);
    CHECK(vlx);
    CHECK_RC(depz_open_on_link(link, -1, -1, 1000, &dev), DEPZ_OK);
    CHECK(depz_is_vl53lx(dev) && depz_device_sensor_type(dev) == DEPZ_SENSOR_VL53LX);
    CHECK(fake_vl53lx_count(vlx, 0x04) == 1 && fake_vl53lx_count(vlx, 0x03) == 1 && fake_vl53lx_rx_count(vlx) == 3);
    CHECK(depz_vl53lx_detected_product(dev) == DEPZ_VL53LX_PRODUCT_L1CX);
    depz_device_close(dev);
    fake_vl53lx_stop(vlx);

    depz_free_device_list(NULL);
}

/* ── VL53L8 against the fake SPI bridge + chip model ─────────────────── */

/* The C side of test_vl53l8_device.py over fake_vl53l8 (the chip half of
 * fake_vl53l8.py): init, the DCI round trips, power modes, thresholds,
 * motion, the xtalk calibration and the start / stop rules. Frames are left
 * to the contract 15 scenarios. */

#define L8_CRC_CX 0xCADF7CAFu /* the checksum the VL53L8CX 2.1.0 blob boots with */
#define L8_CRC_CH 0x0C0B6C9Eu /* the LMZ blob (VL53L8CH) */

typedef struct {
    fake_vl53l8 *fake;
    depz_device *dev;
} l8rig;

typedef struct { int notes, downloads; bool saw_checksum, done; size_t last_done, last_total; } l8_progress;

static void l8_on_progress(const char *text, size_t done, size_t total, void *user)
{
    l8_progress *p = (l8_progress *)user;
    if (!strcmp(text, "download")) {
        p->downloads++;
        p->last_done = done;
        p->last_total = total;
        return;
    }
    p->notes++;
    if (!strcmp(text, "Sensor FW checksum OK")) p->saw_checksum = true;
    if (!strcmp(text, "Sensor init done")) p->done = true;
}

static bool l8_up(l8rig *r, depz_vl53l8_model model, uint32_t crc, bool init)
{
    depz_link *link;
    r->dev = NULL;
    r->fake = fake_vl53l8_start(crc, &link);
    if (!r->fake) return false;
    if (depz_vl53l8_open_link(link, model, &r->dev) != DEPZ_OK) return false;
    depz_device_set_timeout_ms(r->dev, 1000);
    if (init && depz_vl53l8_init(r->dev, NULL, NULL) != DEPZ_OK) {
        fprintf(stderr, "  FAIL vl53l8 init: %s\n", depz_last_error());
        return false;
    }
    return true;
}

static void l8_down(l8rig *r)
{
    depz_device_close(r->dev);
    fake_vl53l8_stop(r->fake);
}

#define L8_RIG(name, model, crc)                       \
    static void name##_body(l8rig *r);                 \
    static void name(void)                             \
    {                                                  \
        l8rig r;                                       \
        if (!l8_up(&r, model, crc, true)) g_failed++;  \
        else name##_body(&r);                          \
        l8_down(&r);                                   \
    }                                                  \
    static void name##_body(l8rig *r)

static void l8_swap(uint8_t *d, size_t n)
{
    size_t i;
    for (i = 0; i + 4 <= n; i += 4) {
        uint8_t a = d[i], b = d[i + 1];
        d[i] = d[i + 3];
        d[i + 1] = d[i + 2];
        d[i + 2] = b;
        d[i + 3] = a;
    }
}

static uint32_t l8_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* init downloads the blob in 2 KB steps, boots it (checksum checked), uploads
 * the 4x4 offsets, the default xtalk and configuration; the ULD defaults
 * read back: 4x4, 1 Hz, continuous, 5 ms, 5 %, closest, awake, margin 0. */
static void l8_init_downloads_and_reads_the_defaults(void)
{
    l8rig r;
    l8_progress pr;
    bool alive = false;
    uint8_t id = 0, rev = 0, b[8], u8 = 0;
    int zones = 0;
    uint32_t u32 = 1;
    memset(&pr, 0, sizeof pr);
    if (!l8_up(&r, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX, false)) { g_failed++; l8_down(&r); return; }
    CHECK_RC(depz_vl53l8_is_alive(r.dev, &alive, &id, &rev), DEPZ_OK);
    CHECK(alive && id == 0xF0 && rev == 0x0C);
    CHECK(fake_vl53l8_page(r.fake) == 0x02); /* the ids on page 0, the sensor left on page 2 */
    CHECK_RC(depz_vl53l8_dci_read(r.dev, 0x5450, b, 8), DEPZ_E_ARG); /* init() first */
    CHECK_RC(depz_vl53l8_init(r.dev, l8_on_progress, &pr), DEPZ_OK);
    CHECK(depz_vl53l8_initialized(r.dev) && !depz_vl53l8_ranging(r.dev));
    CHECK(fake_vl53l8_boots(r.fake) == 1 && fake_vl53l8_config_uploads(r.fake) == 1);
    CHECK(fake_vl53l8_offset_uploads(r.fake) == 1 && fake_vl53l8_xtalk_uploads(r.fake) == 1);
    CHECK(pr.saw_checksum && pr.done && pr.notes > 3);
    CHECK(pr.downloads > 0 && pr.last_done == 0x5000 && pr.last_total == 0x5000); /* bank 3 */
    CHECK(fake_vl53l8_dci(r.fake, 0xDB80, b, sizeof b) == 4 && b[0] == 1 && b[1] == 0 && b[2] == 1 && b[3] == 0);
    CHECK(fake_vl53l8_dci(r.fake, 0xD964, b, sizeof b) == 4 && l8_u32(b) == 1);
    CHECK(depz_vl53l8_module_type(r.dev) == -1); /* read on the L5/L7 only */
    CHECK_RC(depz_vl53l8_get_resolution(r.dev, &zones), DEPZ_OK);
    CHECK(zones == 16);
    CHECK_RC(depz_vl53l8_get_ranging_frequency_hz(r.dev, &u8), DEPZ_OK);
    CHECK(u8 == 1);
    CHECK_RC(depz_vl53l8_get_ranging_mode(r.dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS);
    CHECK_RC(depz_vl53l8_get_integration_time_ms(r.dev, &u32), DEPZ_OK);
    CHECK(u32 == 5);
    CHECK_RC(depz_vl53l8_get_sharpener_percent(r.dev, &u8), DEPZ_OK);
    CHECK(u8 == 5);
    CHECK_RC(depz_vl53l8_get_target_order(r.dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_TARGET_ORDER_CLOSEST);
    CHECK_RC(depz_vl53l8_get_power_mode(r.dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_POWER_MODE_WAKEUP);
    CHECK_RC(depz_vl53l8_get_xtalk_margin(r.dev, &u32), DEPZ_OK);
    CHECK(u32 == 0);
    /* foreign silicon is not alive */
    fake_vl53l8_set_reg(r.fake, 0, 0x00, 0xEB);
    CHECK_RC(depz_vl53l8_is_alive(r.dev, &alive, &id, &rev), DEPZ_OK);
    CHECK(!alive && id == 0xEB);
    l8_down(&r);

    /* the booted firmware reports another checksum: refused, not initialised */
    if (!l8_up(&r, DEPZ_VL53L8_MODEL_L8CX, 0x12345678u, false)) { g_failed++; l8_down(&r); return; }
    CHECK_RC(depz_vl53l8_init(r.dev, NULL, NULL), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "FW_CHECKSUM_FAIL") != NULL);
    CHECK(!depz_vl53l8_initialized(r.dev));
    CHECK_RC(depz_vl53l8_get_resolution(r.dev, &zones), DEPZ_E_ARG);
    l8_down(&r);
}

L8_RIG(l8_configuration_round_trips, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX)
{
    uint8_t b[32], u8 = 0;
    uint8_t xt[DEPZ_VL53L8_XTALK_BUFFER_SIZE];
    uint32_t u32 = 0;
    int zones = 0, pct;
    size_t writes;
    /* resolution: re-uploads offset + xtalk shaped for that grid */
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_resolution(r->dev, &zones), DEPZ_OK);
    CHECK(zones == 64);
    CHECK(fake_vl53l8_dci(r->fake, 0x5450, b, sizeof b) == 8 && b[0] == 8 && b[1] == 8);
    CHECK(fake_vl53l8_dci(r->fake, 0xAD38, b, sizeof b) == 16 && b[4] == 16 && b[9] == 1);
    CHECK(fake_vl53l8_offset_uploads(r->fake) == 2 && fake_vl53l8_xtalk_uploads(r->fake) == 2);
    CHECK(fake_vl53l8_last_xtalk(r->fake, xt) && memcmp(xt, depz_vl53l8_default_xtalk, sizeof xt) == 0); /* 8x8: verbatim */
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_4X4), DEPZ_OK);
    CHECK(fake_vl53l8_dci(r->fake, 0xAD38, b, sizeof b) == 16 && b[4] == 64 && b[9] == 4);
    CHECK(fake_vl53l8_last_xtalk(r->fake, xt) && memcmp(xt, depz_vl53l8_default_xtalk, sizeof xt) != 0); /* folded */
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, 32), DEPZ_E_ARG);
    /* frequency: the floor and the ceiling are refused host-side */
    writes = fake_vl53l8_write_count(r->fake);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 1), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 61), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "<= 60 Hz") != NULL);
    CHECK(fake_vl53l8_write_count(r->fake) == writes);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 60), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_ranging_frequency_hz(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == 60 && fake_vl53l8_dci(r->fake, 0x5458, b, sizeof b) == 4 && b[0] == 0 && b[1] == 60 && b[2] == 0);
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 16), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "<= 15 Hz") != NULL);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 15), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_4X4), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 16), DEPZ_OK); /* the ceiling moved back */
    /* ranging mode */
    CHECK_RC(depz_vl53l8_set_ranging_mode(r->dev, DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_ranging_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS);
    CHECK(fake_vl53l8_dci(r->fake, 0xAD30, b, sizeof b) == 8 && b[1] == 0x3 && b[3] == 0x2);
    CHECK(fake_vl53l8_dci(r->fake, 0xD964, b, sizeof b) == 4 && l8_u32(b) == 1);
    CHECK_RC(depz_vl53l8_set_ranging_mode(r->dev, DEPZ_VL53L8_RANGING_MODE_CONTINUOUS), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_ranging_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_RANGING_MODE_CONTINUOUS);
    CHECK(fake_vl53l8_dci(r->fake, 0xAD30, b, sizeof b) == 8 && b[1] == 0x1 && b[3] == 0x3);
    CHECK(fake_vl53l8_dci(r->fake, 0xD964, b, sizeof b) == 4 && l8_u32(b) == 0);
    CHECK_RC(depz_vl53l8_set_ranging_mode(r->dev, 2), DEPZ_E_ARG);
    /* integration time */
    CHECK_RC(depz_vl53l8_set_integration_time_ms(r->dev, 20), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_integration_time_ms(r->dev, &u32), DEPZ_OK);
    CHECK(u32 == 20 && fake_vl53l8_dci(r->fake, 0x545C, b, sizeof b) == 20 && l8_u32(b) == 20000);
    CHECK_RC(depz_vl53l8_set_integration_time_ms(r->dev, 1), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_integration_time_ms(r->dev, 1001), DEPZ_E_ARG);
    /* sharpener reads back what was set */
    for (pct = 0; pct < 100; pct += 25) {
        uint8_t want = (uint8_t)(pct == 75 ? 99 : pct);
        CHECK_RC(depz_vl53l8_set_sharpener_percent(r->dev, want), DEPZ_OK);
        CHECK_RC(depz_vl53l8_get_sharpener_percent(r->dev, &u8), DEPZ_OK);
        CHECK(u8 == want);
    }
    CHECK(fake_vl53l8_dci(r->fake, 0xAED8, b, sizeof b) == 16 && b[0xD] == 99 * 255 / 100);
    CHECK_RC(depz_vl53l8_set_sharpener_percent(r->dev, 100), DEPZ_E_ARG);
    /* target order */
    CHECK_RC(depz_vl53l8_set_target_order(r->dev, DEPZ_VL53L8_TARGET_ORDER_STRONGEST), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_target_order(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_TARGET_ORDER_STRONGEST && fake_vl53l8_dci(r->fake, 0xAE64, b, sizeof b) == 4 && b[0] == 2);
    CHECK_RC(depz_vl53l8_set_target_order(r->dev, 3), DEPZ_E_ARG);
    /* xtalk margin: 3 kcps -> 3 x 2048 on the wire */
    CHECK_RC(depz_vl53l8_set_xtalk_margin(r->dev, 3), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_xtalk_margin(r->dev, &u32), DEPZ_OK);
    CHECK(u32 == 3 && fake_vl53l8_dci(r->fake, 0xAD94, b, sizeof b) == 16 && l8_u32(b) == 3 * 2048);
    CHECK_RC(depz_vl53l8_set_xtalk_margin(r->dev, 10001), DEPZ_E_ARG);
    /* raw DCI access */
    CHECK_RC(depz_vl53l8_dci_read(r->dev, 0x5458, b, 4), DEPZ_OK);
    CHECK(b[1] == 16);
    b[1] = 20;
    CHECK_RC(depz_vl53l8_dci_write(r->dev, 0x5458, b, 4), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_ranging_frequency_hz(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == 20);
}

L8_RIG(l8_thresholds_motion_and_power, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX)
{
    static const uint8_t motion_head[32] = { /* the Python SDK's DCI_MOTION_DETECTOR_CFG for 400..1500 mm at 8x8 */
        0x44, 0x35, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x06, 0x02, 0x04, 0x09, 0x10, 0x10, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
    static depz_vl53l8_threshold th[2], got[64], many[65];
    uint8_t raw[DEPZ_VL53L8_THRESH_START_SIZE], b[160], u8 = 0;
    bool en = true;
    memset(th, 0, sizeof th);
    CHECK_RC(depz_vl53l8_get_detection_thresholds_enable(r->dev, &en), DEPZ_OK);
    CHECK(!en);
    th[0].low_thresh = 200; th[0].high_thresh = 600; th[0].measurement = DEPZ_VL53L8_DIST_MM;
    th[0].type = DEPZ_VL53L8_THRESH_IN_WINDOW; th[0].zone_num = 5; th[0].operation = DEPZ_VL53L8_THRESH_OP_OR;
    th[1].low_thresh = 1; th[1].high_thresh = 3; th[1].measurement = DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS;
    th[1].type = DEPZ_VL53L8_THRESH_GREATER_THAN_MAX; th[1].zone_num = 6; th[1].operation = DEPZ_VL53L8_THRESH_OP_AND;
    CHECK_RC(depz_vl53l8_set_detection_thresholds(r->dev, th, 2), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_detection_thresholds(r->dev, got), DEPZ_OK);
    CHECK(memcmp(&got[0], &th[0], sizeof th[0]) == 0 && memcmp(&got[1], &th[1], sizeof th[1]) == 0);
    CHECK(got[2].low_thresh == 0 && got[2].high_thresh == 0 && got[2].measurement == 0 && got[2].zone_num == 0);
    /* on the wire the distance is x4 and the signal x2048 (plugin scaling) */
    CHECK(fake_vl53l8_dci(r->fake, 0xB6E8, raw, sizeof raw) == sizeof raw);
    CHECK((int32_t)l8_u32(raw) == 800 && (int32_t)l8_u32(raw + 4) == 2400);
    CHECK((int32_t)l8_u32(raw + 12) == 2048 && (int32_t)l8_u32(raw + 16) == 3 * 2048);
    CHECK(fake_vl53l8_dci(r->fake, 0xB9F0, b, sizeof b) == 8 && b[0] == 5 && b[7] == 5);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_enable(r->dev, true), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_detection_thresholds_enable(r->dev, &en), DEPZ_OK);
    CHECK(en && fake_vl53l8_dci(r->fake, 0x5488, b, sizeof b) == 20 && b[0x11] == 0x04);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_enable(r->dev, false), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_detection_thresholds_enable(r->dev, &en), DEPZ_OK);
    CHECK(!en && fake_vl53l8_dci(r->fake, 0x5488, b, sizeof b) == 20 && b[0x11] == 0x0C);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_auto_stop(r->dev, true), DEPZ_OK);
    CHECK(fake_vl53l8_dci(r->fake, 0xDB80, b, sizeof b) == 4 && b[3] == 1);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_auto_stop(r->dev, false), DEPZ_OK);
    CHECK(fake_vl53l8_dci(r->fake, 0xDB80, b, sizeof b) == 4 && b[3] == 0);
    CHECK_RC(depz_vl53l8_set_detection_thresholds(r->dev, many, 65), DEPZ_E_ARG); /* more than 64 */

    /* the motion indicator: the plugin config, then the distance window */
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK_RC(depz_vl53l8_configure_motion_indicator(r->dev, 400, 1500), DEPZ_OK);
    CHECK(fake_vl53l8_dci(r->fake, 0xBFAC, b, sizeof b) == DEPZ_VL53L8_MOTION_CFG_SIZE);
    CHECK(memcmp(b, motion_head, sizeof motion_head) == 0);
    CHECK(l8_u32(b) == 13636 && b[19] == 9);
    CHECK_RC(depz_vl53l8_configure_motion_indicator(r->dev, 400, 2000), DEPZ_E_ARG); /* span > 1500 mm */

    /* power: sleep, wake (the firmware kept), deep sleep, wake (init again) */
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_SLEEP), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_power_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_POWER_MODE_SLEEP && fake_vl53l8_reg(r->fake, 0, 0x09) == 0x02);
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_SLEEP), DEPZ_OK); /* already there */
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_WAKEUP), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_power_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_POWER_MODE_WAKEUP && fake_vl53l8_boots(r->fake) == 1);
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_power_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP && fake_vl53l8_reg(r->fake, 0, 0x0F) == 0x43);
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_WAKEUP), DEPZ_OK);
    CHECK_RC(depz_vl53l8_get_power_mode(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == DEPZ_VL53L8_POWER_MODE_WAKEUP);
    CHECK(fake_vl53l8_boots(r->fake) == 2 && fake_vl53l8_config_uploads(r->fake) == 2);
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, 7), DEPZ_E_ARG);
    /* a register value the ULD does not know: an error, the page restored */
    fake_vl53l8_set_reg(r->fake, 0, 0x09, 0x07);
    CHECK_RC(depz_vl53l8_get_power_mode(r->dev, &u8), DEPZ_E_PROTOCOL);
    CHECK(fake_vl53l8_page(r->fake) == 0x02);
}

/* The xtalk calibration: the FW's own table, the target as the FW wants it
 * (x4 mm, x16 %), the grid it computed captured, the settings around the run
 * restored; get / set_caldata_xtalk round-trip the blob. */
L8_RIG(l8_xtalk_calibration_round_trip, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX)
{
    static const uint8_t footer[8] = {0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04};
    uint8_t blob[DEPZ_VL53L8_XTALK_BUFFER_SIZE], again[DEPZ_VL53L8_XTALK_BUFFER_SIZE], u8 = 0;
    size_t table_len = 0, writes;
    uint16_t d4 = 0, r16 = 0;
    uint8_t nb = 0;
    unsigned z;
    int zones = 0;
    bool failed = true;
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 15), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_sharpener_percent(r->dev, 25), DEPZ_OK);
    /* argument checks stay off the bus */
    writes = fake_vl53l8_write_count(r->fake);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 0, 4, 600, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 100, 4, 600, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 0, 600, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 17, 600, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 4, 599, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 4, 3001, &failed), DEPZ_E_ARG);
    CHECK(fake_vl53l8_write_count(r->fake) == writes);

    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 4, 600, &failed), DEPZ_OK);
    CHECK(!failed);
    CHECK(fake_vl53l8_calibrations(r->fake, &table_len, &d4, &r16, &nb) == 1);
    CHECK(table_len == depz_vl53l8_calibrate_xtalk_cx_len && d4 == 600 * 4 && r16 == 50 * 16 && nb == 4);
    {
        /* the dedicated 17-block output list went out whole (68 B; the Python
         * SDK's: data_read_size 1204, 18 = blocks + 1, last 0xA28C0402) */
        uint8_t list[80], cfg[8];
        CHECK(fake_vl53l8_dci(r->fake, 0xD980, list, sizeof list) == 68 && l8_u32(list + 64) == 0xA28C0402u);
        CHECK(fake_vl53l8_dci(r->fake, 0xD968, cfg, sizeof cfg) == 8 && l8_u32(cfg) == 1204 && l8_u32(cfg + 4) == 18);
    }
    CHECK(fake_vl53l8_config_uploads(r->fake) == 2); /* the default config restored */
    CHECK_RC(depz_vl53l8_get_resolution(r->dev, &zones), DEPZ_OK);
    CHECK(zones == 16);
    CHECK_RC(depz_vl53l8_get_ranging_frequency_hz(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == 15);
    CHECK_RC(depz_vl53l8_get_sharpener_percent(r->dev, &u8), DEPZ_OK);
    CHECK(u8 == 25);
    /* save: re-reads the FW buffer at 8x8, restores 4x4 */
    CHECK_RC(depz_vl53l8_get_caldata_xtalk(r->dev, blob), DEPZ_OK);
    CHECK(memcmp(blob + sizeof blob - 8, footer, 8) == 0);
    CHECK(memcmp(blob, depz_vl53l8_default_xtalk, sizeof blob) != 0);
    CHECK_RC(depz_vl53l8_get_resolution(r->dev, &zones), DEPZ_OK);
    CHECK(zones == 16);
    memcpy(again, blob, sizeof again);
    l8_swap(again, sizeof again);
    for (z = 0; z < 64; z++) CHECK(l8_u32(again + 0x34 + 4 * z) == fake_vl53l8_xtalk_grid_value(600, 50, 4, z));
    /* load: the blob reaches the FW verbatim on the next 8x8 upload */
    CHECK_RC(depz_vl53l8_set_caldata_xtalk(r->dev, blob), DEPZ_OK);
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_8X8), DEPZ_OK);
    CHECK(fake_vl53l8_last_xtalk(r->fake, again) && memcmp(again, blob, sizeof blob) == 0);
    /* "coverglass too good": reported, the default xtalk kept */
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_4X4), DEPZ_OK);
    fake_vl53l8_xtalk_cal_fails(r->fake, true);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 20, 1, 3000, &failed), DEPZ_OK);
    CHECK(failed);
    CHECK(fake_vl53l8_calibrations(r->fake, NULL, &d4, &r16, &nb) == 2 && d4 == 3000 * 4 && r16 == 20 * 16 && nb == 1);
    CHECK_RC(depz_vl53l8_get_resolution(r->dev, &zones), DEPZ_OK);
    CHECK(zones == 16);
}

/* start_ranging arms the stream with the frame size the FW reports (540 B
 * for the CX at 4x4, as the Python SDK computes); the stream owns the bank:
 * configuration is refused with nothing on the bus; stop clears the state
 * even when STOP_STREAM fails; a laser-safety fault refuses the start. */
L8_RIG(l8_start_stop_and_refusals, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX)
{
    depz_vl53l8_threshold th;
    depz_vl53l8_cnh_setup cnh;
    uint8_t xt[DEPZ_VL53L8_XTALK_BUFFER_SIZE], b[8];
    size_t writes;
    bool failed;
    memset(&th, 0, sizeof th);
    memset(xt, 0, sizeof xt);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 15), DEPZ_OK);
    CHECK_RC(depz_vl53l8_start_ranging(r->dev), DEPZ_OK);
    CHECK(depz_vl53l8_ranging(r->dev) && fake_vl53l8_ranging(r->fake));
    CHECK(fake_vl53l8_dci(r->fake, 0xD968, b, sizeof b) == 8 && l8_u32(b) == 540);
    CHECK(fake_vl53l8_reg(r->fake, 0, 0x09) == 0x05 && fake_vl53l8_page(r->fake) == 0x02); /* xshut bypass on */
    writes = fake_vl53l8_write_count(r->fake);
    CHECK_RC(depz_vl53l8_set_resolution(r->dev, DEPZ_VL53L8_RES_8X8), DEPZ_E_ARG);
    CHECK(strstr(depz_last_error(), "stop ranging first") != NULL);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 10), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_ranging_mode(r->dev, DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_integration_time_ms(r->dev, 20), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_sharpener_percent(r->dev, 10), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_target_order(r->dev, DEPZ_VL53L8_TARGET_ORDER_STRONGEST), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_power_mode(r->dev, DEPZ_VL53L8_POWER_MODE_SLEEP), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_xtalk_margin(r->dev, 1), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 4, 600, &failed), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_get_caldata_xtalk(r->dev, xt), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_caldata_xtalk(r->dev, xt), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_enable(r->dev, true), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_detection_thresholds(r->dev, &th, 1), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_set_detection_thresholds_auto_stop(r->dev, true), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_configure_motion_indicator(r->dev, 400, 1500), DEPZ_E_ARG);
    depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
    CHECK_RC(depz_vl53l8_configure_cnh(r->dev, &cnh), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_start_ranging(r->dev), DEPZ_E_ARG);
    CHECK_RC(depz_vl53l8_init(r->dev, NULL, NULL), DEPZ_E_ARG);
    CHECK(fake_vl53l8_write_count(r->fake) == writes);
    /* stop: STOP_STREAM refused, the ULD stop still runs, the flag clears */
    fake_vl53l8_fail_next(r->fake, 0x36, 0x01);
    CHECK_RC(depz_vl53l8_stop_ranging(r->dev), DEPZ_E_STATUS);
    CHECK(!depz_vl53l8_ranging(r->dev) && !fake_vl53l8_ranging(r->fake));
    CHECK(fake_vl53l8_reg(r->fake, 0, 0x09) == 0x04 && fake_vl53l8_reg(r->fake, 0, 0x14) == 0x00);
    writes = fake_vl53l8_write_count(r->fake);
    CHECK_RC(depz_vl53l8_stop_ranging(r->dev), DEPZ_OK); /* idempotent, nothing on the bus */
    CHECK(fake_vl53l8_write_count(r->fake) == writes);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 30), DEPZ_OK); /* not wedged */
    /* the CX firmware has no CNH */
    CHECK_RC(depz_vl53l8_configure_cnh(r->dev, &cnh), DEPZ_E_WRONG_TYPE);
    CHECK(!depz_vl53l8_cnh_armed(r->dev, NULL));
    /* laser safety */
    fake_vl53l8_laser_safety_fault(r->fake, true);
    CHECK_RC(depz_vl53l8_start_ranging(r->dev), DEPZ_E_PROTOCOL);
    CHECK(strstr(depz_last_error(), "LASER_SAFETY") != NULL);
    CHECK(!depz_vl53l8_ranging(r->dev));
    /* the L5/L7 board commands are refused on an L8 */
    CHECK_RC(depz_vl53l7_pin_ctrl(r->dev, 0), DEPZ_E_WRONG_TYPE);
}

/* START_STREAM refused after the ULD started the die: the caller gets the
 * START_STREAM error, the host says not ranging, so the ULD stop runs (MCU
 * stop undone, xshut bypass off) and configuration works afterwards
 * (PY-2/TS-5). */
L8_RIG(l8_a_refused_start_stream_stops_the_sensor_again, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX)
{
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 15), DEPZ_OK);
    fake_vl53l8_fail_next(r->fake, 0x35, 0x03);
    CHECK_RC(depz_vl53l8_start_ranging(r->dev), DEPZ_E_STATUS);
    CHECK(depz_last_status_cmd() == 0x35 && depz_last_status() == 0x03); /* not an error of the stop */
    CHECK(!depz_vl53l8_ranging(r->dev));
    CHECK(!fake_vl53l8_ranging(r->fake)); /* the die stopped again */
    CHECK(fake_vl53l8_reg(r->fake, 0, 0x09) == 0x04 && fake_vl53l8_reg(r->fake, 0, 0x14) == 0x00);
    CHECK_RC(depz_vl53l8_set_ranging_frequency_hz(r->dev, 30), DEPZ_OK); /* not wedged */
    CHECK_RC(depz_vl53l8_start_ranging(r->dev), DEPZ_OK);               /* a retry starts cleanly */
    CHECK(depz_vl53l8_ranging(r->dev) && fake_vl53l8_ranging(r->fake));
    CHECK_RC(depz_vl53l8_stop_ranging(r->dev), DEPZ_OK);
}

/* stop_ranging() goes on after its STOP_STREAM request (host flag, ULD
 * stop): close on another thread while that request waits ends the call with
 * the closed error and frees nothing until it has returned (C-2: the class
 * state was freed under the caller once the request itself had left — TSan /
 * ASan). */
static void closing_stop_ranging(void *a)
{
    closing_arg *x = (closing_arg *)a;
    x->rc = depz_vl53l8_stop_ranging(x->dev);
    depz_strlcpy(x->err, depz_last_error(), sizeof x->err);
}

static void l8_close_while_stop_ranging_waits(void)
{
    l8rig r;
    depz_thread t;
    closing_arg a;
    uint64_t t0;
    if (!l8_up(&r, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX, true)) {
        g_failed++;
        l8_down(&r);
        return;
    }
    CHECK_RC(depz_vl53l8_start_ranging(r.dev), DEPZ_OK);
    fake_vl53l8_silent_next(r.fake, DEPZ_VL53L8_CMD_STOP_STREAM);
    depz_device_set_timeout_ms(r.dev, 5000);
    memset(&a, 0, sizeof a);
    a.dev = r.dev;
    a.rc = 1;
    t0 = depz_host_now_us();
    CHECK(depz_thread_start(&t, closing_stop_ranging, &a) == 0);
    CHECK(request_in_flight(r.dev)); /* STOP_STREAM, never answered */
    depz_device_close(r.dev);
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_CLOSED && strstr(a.err, "device closed") != NULL);
    CHECK(depz_host_now_us() - t0 < 4000000u); /* not the 5 s timeout */
    fake_vl53l8_stop(r.fake);
}

/* get_frame() waiting on another thread when close runs: the reader's exit
 * marks the frame hub closed, so the wait ends at once with the closed error
 * (not the 10 s timeout) and close returns as soon as it has. */
static void closing_get_frame(void *a)
{
    closing_arg *x = (closing_arg *)a;
    depz_vl53l8_live_frame f;
    x->rc = depz_vl53l8_get_frame(x->dev, 10000, &f);
    depz_strlcpy(x->err, depz_last_error(), sizeof x->err);
}

static void l8_close_while_get_frame_waits(void)
{
    l8rig r;
    depz_thread t;
    closing_arg a;
    uint64_t t0, deadline;
    bool inside = false;
    if (!l8_up(&r, DEPZ_VL53L8_MODEL_L8CX, L8_CRC_CX, true)) {
        g_failed++;
        l8_down(&r);
        return;
    }
    memset(&a, 0, sizeof a);
    a.dev = r.dev;
    a.rc = 1;
    t0 = depz_host_now_us();
    CHECK(depz_thread_start(&t, closing_get_frame, &a) == 0);
    deadline = depz_host_now_us() + 2000000u; /* bounded: the call has entered */
    while (!inside && depz_host_now_us() < deadline) {
        depz_mutex_lock(&r.dev->lock);
        inside = r.dev->users > 0;
        depz_mutex_unlock(&r.dev->lock);
        if (!inside) depz_sleep_ms(1);
    }
    CHECK(inside);
    depz_device_close(r.dev);
    CHECK(depz_host_now_us() - t0 < 4000000u); /* close did not wait out the 10 s */
    depz_thread_join(t);
    CHECK(a.rc == DEPZ_E_CLOSED && strstr(a.err, "closed") != NULL);
    fake_vl53l8_stop(r.fake);
}

/* The VL53L8CH: the LMZ calibration table, CNH armed on its firmware. */
L8_RIG(l8ch_cnh_and_lmz_calibration_table, DEPZ_VL53L8_MODEL_L8CH, L8_CRC_CH)
{
    depz_vl53l8_cnh_setup cnh, armed;
    uint8_t packed[156], b[160];
    size_t table_len = 0;
    bool failed = true;
    depz_vl53l8_cnh_init_config(&cnh, 10, 20, 2);
    CHECK_RC(depz_vl53l8_configure_cnh(r->dev, &cnh), DEPZ_E_ARG); /* no aggregate map yet */
    CHECK_RC(depz_vl53l8_cnh_create_agg_map(&cnh, 64, 0, 0, 2, 2, 4, 4), DEPZ_OK);
    CHECK_RC(depz_vl53l8_configure_cnh(r->dev, &cnh), DEPZ_OK);
    CHECK(depz_vl53l8_cnh_armed(r->dev, &armed) && armed.nb_of_aggregates == 16);
    depz_vl53l8_cnh_pack(&cnh, packed);
    CHECK(fake_vl53l8_dci(r->fake, 0xBFAC, b, sizeof b) == 156 && memcmp(b, packed, 156) == 0);
    CHECK_RC(depz_vl53l8_calibrate_xtalk(r->dev, 50, 4, 600, &failed), DEPZ_OK);
    CHECK(!failed && fake_vl53l8_calibrations(r->fake, &table_len, NULL, NULL, NULL) == 1);
    CHECK(table_len == depz_vl53l8_calibrate_xtalk_lmz_len);
    CHECK_RC(depz_vl53l8_init(r->dev, NULL, NULL), DEPZ_OK); /* a fresh sensor: CNH disarmed */
    CHECK(!depz_vl53l8_cnh_armed(r->dev, NULL) && fake_vl53l8_boots(r->fake) == 2);
}

/* ── runner ──────────────────────────────────────────────────────────── */

/* ── codec edges the vectors do not reach ────────────────────────────── */

/* depz_strip_device_string's sizing call and truncation (a character that
 * does not fit whole is dropped with the rest), a version tail that is not
 * `\d+(\.\d+)*`, a NULL / over-long software name (cut on a UTF-8 boundary),
 * NULL device name and out-of-range USB ids in class resolution. */
static void identity_strings_edge_cases(void)
{
    /* 'A', embedded NUL, a byte >= 0x80, 'B', then 0xFF / 0x00 filler */
    static const uint8_t raw[6] = {'A', 0x00, 0xC3, 'B', 0xFF, 0x00};
    static const char whole[6] = {'A', '\0', '\xEF', '\xBF', '\xBD', 'B'};
    static const char zero8[8] = {0};
    char out[16], longname[80];
    depz_identity id;
    size_t i;
    const int good = 0x0483, bad_vid = 0x10000, bad_pid = -2;

    CHECK(depz_strip_device_string(raw, sizeof raw, NULL, 0) == 6);
    memset(out, 'x', sizeof out);
    CHECK(depz_strip_device_string(raw, sizeof raw, out, sizeof out) == 6);
    CHECK(memcmp(out, whole, 6) == 0 && memcmp(out + 6, zero8, 8) == 0 && out[15] == '\0');
    memset(out, 'x', sizeof out);
    CHECK(depz_strip_device_string(raw, sizeof raw, out, 4) == 2); /* U+FFFD needs 3 of the 1 left */
    CHECK(out[0] == 'A' && out[1] == '\0' && out[2] == '\0' && out[3] == '\0' && out[4] == 'x');
    memset(out, 'x', sizeof out);
    CHECK(depz_strip_device_string(raw, sizeof raw, out, 2) == 1); /* only 'A' fits before the NUL */
    CHECK(out[0] == 'A' && out[1] == '\0' && out[2] == 'x');

    depz_parse_software_name("APP_usonic_SR04_v1.", &id); /* a dot with no digits after it */
    CHECK(strcmp(id.mode, "app") == 0 && id.sensor_type == DEPZ_SENSOR_SR04 && id.version[0] == '\0');
    depz_parse_software_name("APP_usonic_SR04_vx2", &id); /* no digits at all */
    CHECK(id.sensor_type == DEPZ_SENSOR_SR04 && id.version[0] == '\0');
    CHECK(strcmp(id.software_name, "APP_usonic_SR04_vx2") == 0);
    depz_parse_software_name_n(NULL, 5, &id);
    CHECK(strcmp(id.mode, "unknown") == 0 && id.sensor_type == DEPZ_SENSOR_NONE);
    CHECK(id.software_name[0] == '\0' && id.version[0] == '\0');

    /* "APP_" then 37 two-byte 'é' (C3 A9): the 63-byte cut lands on a
     * continuation byte, so it backs off to 62 and keeps whole characters */
    memcpy(longname, "APP_", 4);
    for (i = 0; i < 37; i++) {
        longname[4 + 2 * i] = '\xC3';
        longname[5 + 2 * i] = '\xA9';
    }
    longname[78] = '\0';
    depz_parse_software_name(longname, &id);
    CHECK(strcmp(id.mode, "app") == 0 && id.sensor_type == DEPZ_SENSOR_UNKNOWN);
    CHECK(strlen(id.software_name) == 62 && memcmp(id.software_name, longname, 62) == 0);
    CHECK(id.software_name[61] == '\xA9');

    CHECK(strcmp(depz_resolve_sensor_class_n("APP_usonic_SR04_v1", 18, NULL, 99, NULL, NULL), "sr04") == 0);
    CHECK(strcmp(depz_resolve_sensor_class_n("APP_usonic_SR04_v1", 18, NULL, 0, &good, &good), "sr04") == 0);
    CHECK(depz_resolve_sensor_class_n("APP_usonic_SR04_v1", 18, NULL, 0, &bad_vid, NULL) == NULL);
    CHECK(depz_resolve_sensor_class_n("APP_usonic_SR04_v1", 18, NULL, 0, NULL, &bad_pid) == NULL);
}

/* Every depz_err code has its name; an unknown one is "DEPZ_E_?". */
static void error_names_are_the_enum_names(void)
{
    static const struct { int err; const char *name; } names[] = {
        {DEPZ_OK, "DEPZ_OK"},
        {DEPZ_E_ARG, "DEPZ_E_ARG"},
        {DEPZ_E_NOMEM, "DEPZ_E_NOMEM"},
        {DEPZ_E_IO, "DEPZ_E_IO"},
        {DEPZ_E_CLOSED, "DEPZ_E_CLOSED"},
        {DEPZ_E_TIMEOUT, "DEPZ_E_TIMEOUT"},
        {DEPZ_E_STATUS, "DEPZ_E_STATUS"},
        {DEPZ_E_BUSY, "DEPZ_E_BUSY"},
        {DEPZ_E_PROTOCOL, "DEPZ_E_PROTOCOL"},
        {DEPZ_E_NO_DEVICE, "DEPZ_E_NO_DEVICE"},
        {DEPZ_E_WRONG_TYPE, "DEPZ_E_WRONG_TYPE"},
        {DEPZ_E_REPLAY_MISMATCH, "DEPZ_E_REPLAY_MISMATCH"},
        {DEPZ_E_BOOTLOADER, "DEPZ_E_BOOTLOADER"},
    };
    size_t i;
    for (i = 0; i < sizeof names / sizeof names[0]; i++)
        CHECK(strcmp(depz_err_str(names[i].err), names[i].name) == 0);
    CHECK(strcmp(depz_err_str(-99), "DEPZ_E_?") == 0);
    CHECK(strcmp(depz_err_str(1), "DEPZ_E_?") == 0);
}

/* depz_usb_identity fills both outputs (or either); the port order breaks a
 * serial tie by length, then by port path and its length; short reports and
 * RPT_TEXT into a caller buffer. */
static void usb_order_and_short_report_edges(void)
{
    static const uint8_t text[6] = {0x05, 'S', 'N', '9', 0x00, 0xFF};
    static const uint8_t sync[23] = {0};
    char buf[8];
    bool known = false;
    const char *model = "x";
    uint8_t cmd = 0;
    size_t n = 99;
    depz_sync_time_report sr;
    depz_sync_pin_config pc;
    depz_port_key a, b;

    CHECK(depz_usb_identity((int)DEPZ_DEV_USB_VID, (int)DEPZ_DEV_USB_PID, &known, &model) == 0);
    CHECK(known && model && strcmp(model, "dev") == 0);
    CHECK(depz_usb_identity(0x1234, 0x5678, &known, &model) == 0);
    CHECK(!known && model == NULL);
    CHECK(depz_usb_identity(0x1234, 0x5678, NULL, NULL) == 0);
    CHECK(depz_usb_identity(0x1234, 0x10000, &known, &model) == -1);

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.serial = "AB";
    a.serial_len = 2;
    b.serial = "ABC";
    b.serial_len = 3;
    CHECK(depz_port_order_cmp(&a, &b) < 0 && depz_port_order_cmp(&b, &a) > 0); /* a prefix sorts first */
    b.serial_len = 2;
    a.port = "/dev/ttyACM1";
    a.port_len = 12;
    b.port = "/dev/ttyACM10";
    b.port_len = 13;
    CHECK(depz_port_order_cmp(&a, &b) < 0 && depz_port_order_cmp(&b, &a) > 0);
    b.port = "/dev/ttyACM0";
    b.port_len = 12;
    CHECK(depz_port_order_cmp(&a, &b) > 0);
    b.port = a.port;
    CHECK(depz_port_order_cmp(&a, &b) == 0);

    memset(buf, 'x', sizeof buf);
    CHECK(depz_unpack_text_ex(text, sizeof text, &cmd, buf, sizeof buf, &n) == 0);
    CHECK(cmd == 0x05 && n == 3 && strcmp(buf, "SN9") == 0 && buf[7] == '\0');
    CHECK(depz_unpack_text_ex(text, sizeof text, &cmd, buf, sizeof buf, NULL) == 0);
    CHECK(depz_unpack_text_ex(text, 0, &cmd, buf, sizeof buf, &n) == -1);
    CHECK(depz_unpack_sync_time(sync, sizeof sync, &sr) == -1); /* 23 of its 24 bytes */
    CHECK(depz_unpack_sync_pin_config(sync, 2, &pc) == -1);
}

typedef struct { const char *name; void (*rig_fn)(rig *); void (*plain_fn)(void); } test_case;


/* ── datasets: the session recorder (contract 09) ────────────────────── */

static void tmp_dataset_path(char *out, size_t cap, const char *name)
{
    static int n;
    const char *dir = getenv("TMPDIR");
#ifdef _WIN32
    if (!dir) dir = getenv("TEMP");
#endif
    if (!dir) dir = "/tmp";
    snprintf(out, cap, "%s/depz_c_io_%ld_%d_%s", dir, (long)depz_host_now_us(), n++, name);
}

/* Two fake SR04 boards on device clocks hours apart land on one host
 * timeline, ordered by capture time: t = device time − offset. */
static void dataset_recorder_two_sr04(void)
{
    rig a, b;
    char path[512], id[32];
    depz_session_recorder_options opt = DEPZ_SESSION_RECORDER_OPTIONS_INIT;
    depz_session_recorder *rec;
    depz_dataset *ds;
    size_t i;
    tmp_dataset_path(path, sizeof path, "two.depzdata");
    CHECK(rig_up(&a));
    CHECK(rig_up(&b));
    opt.note = "unit test";
    opt.temperature_hz = 0;
    rec = depz_session_recorder_new(path, &opt);
    CHECK(rec != NULL);
    CHECK_RC(depz_session_recorder_add(rec, a.dev, NULL, 5, id, sizeof id), DEPZ_OK);
    CHECK(strcmp(id, "d0") == 0);
    CHECK_RC(depz_session_recorder_add(rec, b.dev, NULL, 5, id, sizeof id), DEPZ_OK);
    CHECK(strcmp(id, "d1") == 0);
    CHECK(depz_session_recorder_records_written(rec) == 0);
    CHECK_RC(depz_session_recorder_start(rec), DEPZ_OK);
    CHECK_RC(depz_session_recorder_start(rec), DEPZ_OK); /* idempotent */
    CHECK_RC(depz_session_recorder_add(rec, a.dev, NULL, 5, NULL, 0), DEPZ_E_ARG);
    /* 50 ms device steps per sample: the step exceeds the gap between the two
     * syncs, so the merged order does not depend on how fast add() ran */
    fake_sr04_send_measurement(a.fake, 0x37);
    depz_sleep_ms(20);
    fake_sr04_send_measurement(b.fake, 0x37);
    depz_sleep_ms(20);
    fake_sr04_send_measurement(a.fake, 0x37);
    depz_sleep_ms(20);
    fake_sr04_send_measurement(b.fake, 0x36);
    for (i = 0; i < 100 && depz_session_recorder_records_written(rec) < 4; i++) depz_sleep_ms(10);
    CHECK(depz_session_recorder_records_written(rec) == 4);
    CHECK_RC(depz_session_recorder_stop(rec), DEPZ_OK);
    CHECK(depz_session_recorder_records_written(rec) == 4); /* the finished file's count */
    fake_sr04_send_measurement(a.fake, 0x37);              /* unhooked: dropped */
    depz_sleep_ms(20);
    depz_session_recorder_free(rec);
    rig_down(&a);
    rig_down(&b);

    ds = depz_dataset_read_file(path);
    remove(path);
    CHECK(ds != NULL);
    CHECK(strcmp(depz_dataset_note(ds), "unit test") == 0);
    CHECK(depz_dataset_device_count(ds) == 2);
    for (i = 0; i < depz_dataset_device_count(ds); i++) {
        depz_dataset_device d;
        depz_dataset_get_device(ds, i, &d);
        CHECK(strcmp(d.id, i ? "d1" : "d0") == 0);
        CHECK(strcmp(d.sensor_type, "sr04") == 0);
        CHECK(strcmp(d.serial, FAKE_SR04_SERIAL) == 0);
        CHECK(strcmp(d.software_name, FAKE_SR04_SOFTWARE) == 0);
    }
    CHECK(depz_dataset_record_count(ds) == 4);
    {
        static const char *const ORDER[] = {"d0", "d1", "d0", "d1"};
        int64_t prev = 0;
        for (i = 0; i < 4; i++) {
            depz_dataset_record r;
            char src[8] = "";
            int64_t echo = 0;
            depz_dataset_get_record(ds, i, &r);
            CHECK(strcmp(r.device_id, ORDER[i]) == 0);
            CHECK(strcmp(r.kind, "sr04") == 0);
            CHECK(i == 0 || r.t_host_us >= prev);
            prev = r.t_host_us;
            /* t on the host clock: the device clocks (1 s and 2.5 h) are gone */
            CHECK(llabs(r.t_host_us - (int64_t)depz_host_now_us()) < 10000000LL);
            CHECK(depz_dataset_value_int(&r, "echo_us", &echo) == 0 && echo == 5831);
            CHECK(depz_dataset_value_str(&r, "source", src, sizeof src) == 0 &&
                  strcmp(src, i == 3 ? "once" : "loop") == 0);
        }
    }
    depz_dataset_free(ds);
}

/* The temperature sampler writes `temperature` records (the MCU
 * thermometer) to a gzip file the reader opens. */
static void dataset_recorder_temperature_gz(void)
{
    rig a;
    char path[512];
    depz_session_recorder_options opt = DEPZ_SESSION_RECORDER_OPTIONS_INIT;
    depz_session_recorder *rec;
    depz_dataset *ds;
    FILE *f;
    unsigned char magic[2] = {0, 0};
    size_t i, temps = 0;
    tmp_dataset_path(path, sizeof path, "temp.depzdata.gz");
    CHECK(rig_up(&a));
    opt.temperature_hz = 50.0;
    rec = depz_session_recorder_new(path, &opt);
    CHECK(rec != NULL);
    CHECK_RC(depz_session_recorder_add(rec, a.dev, "imu", 3, NULL, 0), DEPZ_OK);
    CHECK_RC(depz_session_recorder_start(rec), DEPZ_OK);
    /* Wait for the samples rather than a fixed 150 ms: a loaded CI runner
     * (macOS) took longer than that for two MCU round trips. */
    for (i = 0; i < 300 && depz_session_recorder_records_written(rec) < 2; i++) depz_sleep_ms(10);
    depz_session_recorder_free(rec); /* stops: the file gets its gzip trailer */
    rig_down(&a);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fread(magic, 1, 2, f) == 2 && magic[0] == 0x1F && magic[1] == 0x8B);
    fclose(f);
    ds = depz_dataset_read_file(path);
    remove(path);
    CHECK(ds != NULL);
    for (i = 0; i < depz_dataset_record_count(ds); i++) {
        depz_dataset_record r;
        char src[8] = "";
        depz_dataset_get_record(ds, i, &r);
        if (strcmp(r.kind, "temperature") != 0) continue;
        temps++;
        CHECK(strcmp(r.device_id, "imu") == 0);
        CHECK(depz_json_double_value(depz_json_get(r.value, "celsius")) == 27.3);
        CHECK(depz_dataset_value_str(&r, "source", src, sizeof src) == 0 && strcmp(src, "mcu") == 0);
    }
    CHECK(temps >= 2);
    depz_dataset_free(ds);
}

/* Contract 09 `bno086`: every report at its measurement time on the host
 * timeline (device timestamp − offset), not its arrival time; one cargo's
 * reports come back sorted by t. */
static void dataset_recorder_bno086(void)
{
    /* 0xFB base delta 20 ticks, a rotation vector (delay 3 ticks) and an
     * accelerometer — the reports of bno086_reports() */
    static const uint8_t cargo[] = {
        0xFB, 20, 0, 0, 0,
        0x05, 9, 0x03, 3, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
        0x01, 10, 0x02, 0, 0x00, 0x01, 0x00, 0xFF, 0x80, 0x09};
    fake_bno086 *f;
    depz_device *dev = bno086_fake_open(&f);
    char path[512];
    depz_session_recorder *rec;
    depz_dataset *ds;
    depz_dataset_device d;
    size_t i;
    CHECK(dev);
    tmp_dataset_path(path, sizeof path, "imu.depzdata");
    rec = depz_session_recorder_new(path, NULL);
    CHECK(rec != NULL);
    CHECK_RC(depz_session_recorder_add(rec, dev, NULL, 3, NULL, 0), DEPZ_OK);
    CHECK_RC(depz_session_recorder_start(rec), DEPZ_OK);
    fake_bno086_send_input(f, DEPZ_SHTP_CH_INPUT_NORMAL, 5000000, cargo, sizeof cargo);
    for (i = 0; i < 100 && depz_session_recorder_records_written(rec) < 2; i++) depz_sleep_ms(10);
    CHECK(depz_session_recorder_records_written(rec) == 2);
    depz_session_recorder_free(rec);
    depz_device_close(dev);
    fake_bno086_stop(f);

    ds = depz_dataset_read_file(path);
    remove(path);
    CHECK(ds != NULL);
    CHECK(depz_dataset_get_device(ds, 0, &d) == 0 && strcmp(d.sensor_type, "bno086") == 0 &&
          strcmp(d.serial, "SN0086") == 0);
    CHECK(depz_dataset_record_count(ds) == 2);
    {
        depz_dataset_record r0, r1;
        char type[32];
        depz_dataset_get_record(ds, 0, &r0);
        depz_dataset_get_record(ds, 1, &r1);
        /* the accelerometer (delay 0) comes before the rotation vector (delay 3 ticks) */
        CHECK(strcmp(r0.kind, "bno086") == 0 && strcmp(r1.kind, "bno086") == 0);
        CHECK(depz_dataset_value_str(&r0, "type", type, sizeof type) == 0 && strcmp(type, "Acceleration") == 0);
        CHECK(depz_dataset_value_str(&r1, "type", type, sizeof type) == 0 && strcmp(type, "RotationVector") == 0);
        CHECK(r0.t_host_us == 5000000 - 2000 - d.offset_us);
        CHECK(r1.t_host_us == 5000000 - 2000 + 300 - d.offset_us);
        CHECK(depz_json_int_value(depz_json_get(r0.value, "timestamp_us")) == 5000000 - 2000);
        CHECK(depz_json_double_value(depz_json_get(r1.value, "i")) == 0.5 &&
              depz_json_double_value(depz_json_get(r1.value, "real")) == 0.0);
        CHECK(depz_json_get(r0.value, "i") == NULL && depz_json_get(r1.value, "x") == NULL);
    }
    depz_dataset_free(ds);
}

/* ── discovery: an explicit path names its port under any alias (02 §4.3, A5) ── */

#ifndef _WIN32
#include <unistd.h>

static void explicit_port_alias_names_its_port(void)
{
    /* A /dev/serial/by-id/… symlink names the port it points at: a regular
     * file stands in for the node, the resolved path matches either way. */
    char dir[] = "/tmp/depz-a5-XXXXXX", node[64], alias[64], other[64];
    CHECK(mkdtemp(dir) != NULL);
    snprintf(node, sizeof node, "%s/ttyACM0", dir);
    snprintf(alias, sizeof alias, "%s/usb-DEPZ_ToF-if00", dir);
    snprintf(other, sizeof other, "%s/other", dir);
    {
        FILE *f = fopen(node, "wb");
        CHECK(f != NULL);
        fclose(f);
    }
    CHECK(symlink(node, alias) == 0);
    CHECK(depz_same_port(alias, node));
    CHECK(depz_same_port(node, alias));
    CHECK(!depz_same_port(other, node));     /* nothing there */
    CHECK(!depz_same_port(dir, node));       /* a directory is not a port */
    CHECK(depz_same_port("/dev/null", "/dev/null")); /* one device node, one name */
    CHECK(!depz_same_port("/dev/null", node));       /* a device and a file */
    unlink(alias);
    unlink(node);
    rmdir(dir);
}
#else
static void explicit_port_alias_names_its_port(void)
{
    /* COM ports are not files: nothing but the exact name matches. */
    CHECK(!depz_same_port("COM5", "COM6"));
    CHECK(!depz_same_port("COM5", "com5"));
}
#endif

/* ── contract 09 record values of the live layer ─────────────────────── */

/* `got` (consumed) equals the JSON text `want`, structurally. */
static bool record_is(depz_json *got, const char *want)
{
    depz_json *w = depz_json_parse(want, strlen(want));
    bool ok = got && w && depz_json_equal(got, w);
    if (!ok) {
        char *t = got ? depz_json_dumps(got) : NULL;
        fprintf(stderr, "  record: got  %s\n          want %s\n", t ? t : "(null)", want);
        free(t);
    }
    depz_json_free(w);
    depz_json_free(got);
    return ok;
}

/* The record value (`v`) of every result type that only exists in this layer,
 * key for key the Python SessionRecorder (contract 09): sr04, vl53l8 with and
 * without the motion block, vl53l8_cnh (base64, start_bin = floor(offset /
 * 2048)), vl53lx with and without targets, bno055 with every channel and with
 * one. A NULL input is a NULL record. */
static void dataset_live_records(void)
{
    static depz_vl53l8_live_frame f;
    static const uint8_t RAW[3] = {1, 2, 3};
    depz_sr04_measurement m = {1000, 5831, true};
    depz_vl53l8_cnh_setup setup;
    depz_vl53lx_measurement lx;
    depz_bno055_sample s;
    size_t i;

    CHECK(record_is(depz_dataset_sr04_record(&m), "{\"echo_us\": 5831, \"source\": \"loop\"}"));
    m.from_loop = false;
    m.echo_time_us = DEPZ_SR04_ECHO_TIMEOUT;
    CHECK(record_is(depz_dataset_sr04_record(&m), "{\"echo_us\": 65535, \"source\": \"once\"}"));
    CHECK(depz_dataset_sr04_record(NULL) == NULL);

    memset(&f, 0, sizeof f);
    f.f.resolution = 16;
    f.f.silicon_temp_degc = 31;
    for (i = 0; i < 16; i++) {
        f.f.distance_mm[i] = (int32_t)(100 + 10 * i);
        f.f.target_status[i] = 5;
        f.f.nb_target_detected[i] = i < 8;
    }
    CHECK(record_is(depz_dataset_vl53l8_live_record(&f, false), "{\"resolution\": 16, \"silicon_temp_degc\": 31, \"distance_mm\": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250], \"target_status\": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5], \"nb_target_detected\": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0]}"));
    f.has_motion = true;
    f.motion.global_indicator_1 = 7;
    f.motion.global_indicator_2 = 9;
    f.motion.status = 0;
    f.motion.nb_of_detected_aggregates = 2;
    f.motion.nb_of_aggregates = 4;
    for (i = 0; i < 32; i++) f.motion.motion[i] = (uint32_t)(3 * i);
    CHECK(record_is(depz_dataset_vl53l8_live_record(&f, false), "{\"resolution\": 16, \"silicon_temp_degc\": 31, \"distance_mm\": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250], \"target_status\": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5], \"nb_target_detected\": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0], \"motion\": {\"global_indicator_1\": 7, \"global_indicator_2\": 9, \"status\": 0, \"nb_of_detected_aggregates\": 2, \"nb_of_aggregates\": 4, \"motion\": [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93]}}"));
    CHECK(depz_dataset_vl53l8_live_record(NULL, false) == NULL);

    memset(&setup, 0, sizeof setup);
    setup.nb_of_aggregates = 8;
    setup.feature_length = 6;
    setup.sum_span = 2;
    setup.ref_bin_offset = -6143; /* -6143 // 2048 == -3 (floor, not truncation) */
    CHECK(record_is(depz_dataset_vl53l8_cnh_record(RAW, 3, &setup),
                    "{\"nb_aggregates\": 8, \"feature_length\": 6, \"sum_span\": 2, \"start_bin\": -3, \"cnh_raw_b64\": \"AQID\"}"));
    setup.ref_bin_offset = 4096;
    CHECK(record_is(depz_dataset_vl53l8_cnh_record(RAW, 2, &setup),
                    "{\"nb_aggregates\": 8, \"feature_length\": 6, \"sum_span\": 2, \"start_bin\": 2, \"cnh_raw_b64\": \"AQI=\"}"));
    CHECK(record_is(depz_dataset_vl53l8_cnh_record(RAW, 1, &setup),
                    "{\"nb_aggregates\": 8, \"feature_length\": 6, \"sum_span\": 2, \"start_bin\": 2, \"cnh_raw_b64\": \"AQ==\"}"));
    CHECK(record_is(depz_dataset_vl53l8_cnh_record(NULL, 0, &setup),
                    "{\"nb_aggregates\": 8, \"feature_length\": 6, \"sum_span\": 2, \"start_bin\": 2, \"cnh_raw_b64\": \"\"}"));
    CHECK(depz_dataset_vl53l8_cnh_record(NULL, 3, &setup) == NULL);
    CHECK(depz_dataset_vl53l8_cnh_record(RAW, 3, NULL) == NULL);

    memset(&lx, 0, sizeof lx);
    lx.distance_mm = 1234;
    lx.sigma_mm = 2.5;
    lx.signal_kcps = 120.25;
    lx.ambient_kcps = 3.5;
    lx.spads = 180.0;
    CHECK(record_is(depz_dataset_vl53lx_record(&lx, "vl53l1cb", "uld"),
                    "{\"product\": \"vl53l1cb\", \"driver\": \"uld\", \"status\": 0, \"distance_mm\": 1234, "
                    "\"sigma_mm\": 2.5, \"signal_kcps\": 120.25, \"ambient_kcps\": 3.5, \"spads\": 180.0}"));
    lx.n_targets = 2;
    lx.targets[0].distance_mm = 1234;
    lx.targets[0].status = 0;
    lx.targets[0].signal_kcps = 120.25;
    lx.targets[1].distance_mm = 2100;
    lx.targets[1].status = 6;
    lx.targets[1].signal_kcps = 8.5;
    CHECK(record_is(depz_dataset_vl53lx_record(&lx, NULL, NULL),
                    "{\"product\": \"\", \"driver\": \"\", \"status\": 0, \"distance_mm\": 1234, "
                    "\"sigma_mm\": 2.5, \"signal_kcps\": 120.25, \"ambient_kcps\": 3.5, \"spads\": 180.0, "
                    "\"targets\": [{\"distance_mm\": 1234, \"status\": 0, \"signal_kcps\": 120.25}, "
                    "{\"distance_mm\": 2100, \"status\": 6, \"signal_kcps\": 8.5}]}"));
    CHECK(depz_dataset_vl53lx_record(NULL, "vl53l1cb", "uld") == NULL);

    memset(&s, 0, sizeof s);
    s.units.accel_mg = true;  /* 0x01 */
    s.units.euler_rad = true; /* 0x04 */
    s.units.android = true;   /* 0x80 */
    s.present = DEPZ_BNO055_HAS_ACCEL | DEPZ_BNO055_HAS_MAG | DEPZ_BNO055_HAS_GYRO | DEPZ_BNO055_HAS_EULER |
                DEPZ_BNO055_HAS_QUATERNION | DEPZ_BNO055_HAS_LINEAR_ACCEL | DEPZ_BNO055_HAS_GRAVITY |
                DEPZ_BNO055_HAS_TEMPERATURE | DEPZ_BNO055_HAS_CALIBRATION;
    s.quaternion[0] = 1.0;
    s.euler[0] = 0.5; s.euler[1] = -0.25; s.euler[2] = 1.75;
    s.accel[2] = 9.81;
    s.gyro[0] = 0.1; s.gyro[1] = 0.2; s.gyro[2] = 0.3;
    s.mag[0] = 10.5; s.mag[1] = -20.25; s.mag[2] = 30.0;
    s.linear_accel[0] = 0.01; s.linear_accel[1] = 0.02; s.linear_accel[2] = 0.03;
    s.gravity[2] = 9.8;
    s.temperature = 24.0;
    s.calibration.system = 3; s.calibration.gyro = 3; s.calibration.accel = 2; s.calibration.mag = 1;
    CHECK(record_is(depz_dataset_bno055_record(&s),
                    "{\"unit_sel\": 133, \"quaternion\": [1.0, 0.0, 0.0, 0.0], \"euler\": [0.5, -0.25, 1.75], "
                    "\"accel\": [0.0, 0.0, 9.81], \"gyro\": [0.1, 0.2, 0.3], \"mag\": [10.5, -20.25, 30.0], "
                    "\"linear_accel\": [0.01, 0.02, 0.03], \"gravity\": [0.0, 0.0, 9.8], \"temperature\": 24.0, "
                    "\"calib\": [3, 3, 2, 1]}"));
    memset(&s.units, 0, sizeof s.units);
    s.present = DEPZ_BNO055_HAS_ACCEL;
    CHECK(record_is(depz_dataset_bno055_record(&s), "{\"unit_sel\": 0, \"accel\": [0.0, 0.0, 9.81]}"));
    CHECK(depz_dataset_bno055_record(NULL) == NULL);
}

/* ── VL53LX bare-driver register tables ──────────────────────────────── */

/* The generated block codecs (vl53lx_bare_tables.c, from the Python SDK's
 * regs.py) round-trip like ST's VL53LX_i2c_encode_* / decode_*: encoding the
 * decoded fields of a block's bytes — all zero, all one, random — gives the
 * same bytes except the reserved bits of a fielded byte, which the codec
 * drops (a hole byte no field covers rides through `raw` untouched); that
 * output is a fixed point: it decodes and encodes to itself. All-zero bytes
 * round-trip exactly. The block map itself is ascending and non-overlapping. */
static void vlx_bare_tables_roundtrip(void)
{
    static vlxb_image a, b;
    uint8_t raw[64], back[64], again[64];
    uint32_t lcg = 0x2545F491u;
    int id, pass;
    size_t i;
    for (id = 0; id < VLXB_BLK_COUNT; id++) {
        const vlxb_block_geom *g = &vlxb_blocks[id];
        CHECK(g->size > 0 && g->size < sizeof raw);
        CHECK(id == 0 || g->base >= vlxb_blocks[id - 1].base + vlxb_blocks[id - 1].size);
        for (pass = 0; pass < 4; pass++) {
            for (i = 0; i < g->size; i++) {
                lcg = lcg * 1664525u + 1013904223u;
                raw[i] = pass == 0 ? 0x00 : pass == 1 ? 0xFF : (uint8_t)(lcg >> 24);
            }
            memset(&a, 0, sizeof a);
            memset(&b, 0, sizeof b);
            memset(back, 0x5A, sizeof back);
            memset(again, 0xA5, sizeof again);
            vlxb_decode(&a, (vlxb_block_id)id, raw);
            vlxb_encode(&a, (vlxb_block_id)id, back);
            for (i = 0; i < g->size; i++) {
                if ((back[i] & raw[i]) != back[i]) {
                    fprintf(stderr, "  block %d pass %d byte %zu: encode set a bit the input lacked\n", id, pass, i);
                    CHECK(false);
                }
            }
            if (pass == 0) CHECK(memcmp(back, raw, g->size) == 0);
            vlxb_decode(&b, (vlxb_block_id)id, back);
            vlxb_encode(&b, (vlxb_block_id)id, again);
            if (memcmp(again, back, g->size) != 0) {
                fprintf(stderr, "  block %d pass %d: encode(decode(x)) != x for x = encode(decode(raw))\n", id, pass);
                CHECK(false);
            }
            CHECK(back[g->size] == 0x5A && again[g->size] == 0xA5); /* writes the block's size only */
        }
    }
}

/* ── discovery without a port ────────────────────────────────────────── */

/* Contract 02 §4 / 07 §3 on this machine's port list without opening any
 * port: the argument checks, the candidate ordering, a path that is not a
 * port, a USB serial / an index no candidate has (nothing is probed), and the
 * class attach rules on a plain device over the fake SR04. */
static void discovery_without_a_port(void)
{
    depz_port_info *ports = NULL;
    size_t n = 0;
    depz_device *dev = NULL;
    depz_device_info info;
    depz_open_options o = DEPZ_OPEN_OPTIONS_INIT;
    depz_link *link;
    fake_sr04 *fake;
    depz_identity ident;
#ifdef _WIN32
    const char *no_port = "\\\\.\\DEPZ_NO_SUCH_PORT";
#else
    const char *no_port = "/nonexistent/depz-no-such-port";
#endif

    CHECK_RC(depz_list_serial_ports(NULL, &n), DEPZ_E_ARG);
    CHECK_RC(depz_list_serial_ports(&ports, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_list_serial_ports(&ports, &n), DEPZ_OK); /* enumerates; opens nothing */
    depz_free_port_list(ports);

    {
        /* USB serial ascending, unknown serials last, then the port path. */
        depz_port_info p[4];
        memset(p, 0, sizeof p);
        strcpy(p[0].port, "/dev/ttyACM3"); strcpy(p[0].usb_serial, "SN0200");
        strcpy(p[1].port, "/dev/ttyACM1");
        strcpy(p[2].port, "/dev/ttyACM2"); strcpy(p[2].usb_serial, "SN0100");
        strcpy(p[3].port, "/dev/ttyACM0");
        depz_sort_ports_by_serial(p, 4);
        CHECK(strcmp(p[0].port, "/dev/ttyACM2") == 0 && strcmp(p[0].usb_serial, "SN0100") == 0);
        CHECK(strcmp(p[1].port, "/dev/ttyACM3") == 0 && strcmp(p[1].usb_serial, "SN0200") == 0);
        CHECK(strcmp(p[2].port, "/dev/ttyACM0") == 0 && p[2].usb_serial[0] == '\0');
        CHECK(strcmp(p[3].port, "/dev/ttyACM1") == 0 && p[3].usb_serial[0] == '\0');
        depz_sort_ports_by_serial(NULL, 4); /* no-ops */
        depz_sort_ports_by_serial(p, 1);
        CHECK(strcmp(p[0].port, "/dev/ttyACM2") == 0);
    }

    CHECK_RC(depz_probe_port(NULL, 100, &info), DEPZ_E_ARG);
    CHECK_RC(depz_probe_port(no_port, 100, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_probe_port(no_port, 100, &info), DEPZ_E_NO_DEVICE);
    CHECK(strstr(depz_last_error(), "no DEPZ device answered") != NULL);
    CHECK(info.sensor_type == DEPZ_SENSOR_NONE && info.software_name[0] == '\0');

    CHECK_RC(depz_open_device(&o, NULL), DEPZ_E_ARG);
    o.port = no_port;
    CHECK_RC(depz_open_device(&o, &dev), DEPZ_E_NO_DEVICE);
    CHECK(dev == NULL);
    {
        /* A port that does not open — as one another program holds — names the
           open failure after the no-device text (contract 07 §3). */
        char want[300];
        snprintf(want, sizeof want, "no DEPZ device answered on %s: could not open the port (", no_port);
        CHECK(strncmp(depz_last_error(), want, strlen(want)) == 0);
        CHECK(strstr(depz_last_error() + strlen(want), "cannot open") != NULL); /* the serial layer's text */
    }
    o.port = NULL;
    o.serial = "depz-test-no-such-serial"; /* no candidate matches: nothing is probed */
    CHECK_RC(depz_open_device(&o, &dev), DEPZ_E_NO_DEVICE);
    CHECK(strstr(depz_last_error(), "depz-test-no-such-serial") != NULL);
    o.serial = NULL;
    o.index = 100000;
    CHECK_RC(depz_open_device(&o, &dev), DEPZ_E_NO_DEVICE);
    CHECK(strstr(depz_last_error(), "index 100000") != NULL);
    CHECK(dev == NULL);

    CHECK_RC(depz_list_depz_devices(false, 100, NULL, &n), DEPZ_E_ARG);
    CHECK_RC(depz_open_on_link(NULL, -1, -1, 100, &dev), DEPZ_E_ARG);

    /* The class attach rules on a plain device over the fake SR04. */
    fake = fake_sr04_start(&link);
    CHECK(fake != NULL);
    CHECK_RC(depz_device_open_link(link, &dev), DEPZ_OK);
    depz_device_set_timeout_ms(dev, 1000);
    CHECK_RC(depz_attach_sensor_class(NULL, "sr04"), DEPZ_E_ARG);
    CHECK_RC(depz_attach_sensor_class(dev, NULL), DEPZ_E_ARG);
    CHECK_RC(depz_attach_sensor_class(dev, "bootloader"), DEPZ_E_BOOTLOADER);
    CHECK(strstr(depz_last_error(), "bootloader mode") != NULL);
    CHECK_RC(depz_attach_sensor_class(dev, "no_such_class"), DEPZ_E_ARG);
    CHECK_RC(depz_attach_sensor_class(dev, "device"), DEPZ_OK); /* stays the base device */
    CHECK_RC(depz_device_promote_ex(dev, -1, -1, &ident), DEPZ_OK);
    CHECK(ident.sensor_type == DEPZ_SENSOR_SR04 && strcmp(ident.mode, "app") == 0);
    CHECK_RC(depz_attach_sensor_class(dev, "sr04"), DEPZ_E_ARG); /* already has a class */
    CHECK(strstr(depz_last_error(), "already has a sensor class") != NULL);
    depz_device_close(dev);
    fake_sr04_stop(fake);
}

static const test_case k_tests[] = {
    {"identity_roundtrip", identity_roundtrip, NULL},
    {"temperature", temperature, NULL},
    {"short_reply_fails_its_request_and_the_reader_goes_on", short_reply_fails_its_request_and_the_reader_goes_on, NULL},
    {"sync_time_produces_offset", sync_time_produces_offset, NULL},
    {"unknown_cmd_raises_status_error", unknown_cmd_raises_status_error, NULL},
    {"sync_pin_validation", sync_pin_validation, NULL},
    {"timeout_when_device_silent", NULL, timeout_when_device_silent},
    {"explicit_port_alias_names_its_port", NULL, explicit_port_alias_names_its_port},
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
    {"host_seq_gap_is_an_event", NULL, host_seq_gap_is_an_event},
    {"close_while_a_request_waits", NULL, close_while_a_request_waits},
    {"bno086_close_while_a_control_request_waits", NULL, bno086_close_while_a_control_request_waits},
    {"same_opcode_in_flight_is_busy", NULL, same_opcode_in_flight_is_busy},
    {"record_then_replay", NULL, record_then_replay},
    {"replay_refuses_a_gzipped_capture", NULL, replay_refuses_a_gzipped_capture},
    {"replay_exhausted_waits_its_timeout", NULL, replay_exhausted_waits_its_timeout},
    {"reader_refuses_a_line_nested_100000_deep", NULL, reader_refuses_a_line_nested_100000_deep},
    {"l4_init_boot_speed_then_retimes", NULL, l4_init_boot_speed_then_retimes},
    {"l4_init_at_400_never_retimes", NULL, l4_init_at_400_never_retimes},
    {"l4_init_config_block_and_vhv", NULL, l4_init_config_block_and_vhv},
    {"l4_range_timing_roundtrip_and_bounds", NULL, l4_range_timing_roundtrip_and_bounds},
    {"l4_tuning_roundtrips", NULL, l4_tuning_roundtrips},
    {"l4_is_alive_checks_model_id", NULL, l4_is_alive_checks_model_id},
    {"l4_config_refused_while_ranging", NULL, l4_config_refused_while_ranging},
    {"l4_a_refused_start_stream_stops_the_sensor_again", NULL, l4_a_refused_start_stream_stops_the_sensor_again},
    {"l4_a_refused_stop_after_a_refused_start_stream_keeps_its_error", NULL, l4_a_refused_stop_after_a_refused_start_stream_keeps_its_error},
    {"l4_stream_decodes_and_counts_short_blocks", NULL, l4_stream_decodes_and_counts_short_blocks},
    {"l4_measure_once_polls_and_stops", NULL, l4_measure_once_polls_and_stops},
    {"l8_guards_and_cnh_math", NULL, l8_guards_and_cnh_math},
    {"l8_init_downloads_and_reads_the_defaults", NULL, l8_init_downloads_and_reads_the_defaults},
    {"l8_configuration_round_trips", NULL, l8_configuration_round_trips},
    {"l8_thresholds_motion_and_power", NULL, l8_thresholds_motion_and_power},
    {"l8_xtalk_calibration_round_trip", NULL, l8_xtalk_calibration_round_trip},
    {"l8_start_stop_and_refusals", NULL, l8_start_stop_and_refusals},
    {"l8_a_refused_start_stream_stops_the_sensor_again", NULL, l8_a_refused_start_stream_stops_the_sensor_again},
    {"l8_close_while_stop_ranging_waits", NULL, l8_close_while_stop_ranging_waits},
    {"l8_close_while_get_frame_waits", NULL, l8_close_while_get_frame_waits},
    {"l8ch_cnh_and_lmz_calibration_table", NULL, l8ch_cnh_and_lmz_calibration_table},
    {"l8ch_cnh_and_motion_exclusive", NULL, l8ch_cnh_and_motion_exclusive},
    {"bno086_features", NULL, bno086_features},
    {"bno086_busy_retry", NULL, bno086_busy_retry},
    {"bno086_reports", NULL, bno086_reports},
    {"bno086_cargo_capture", NULL, bno086_cargo_capture},
    {"bno086_commands", NULL, bno086_commands},
    {"bno086_frs", NULL, bno086_frs},
    {"bno086_codecs", NULL, bno086_codecs},
    {"tmf8829_fake_session", NULL, tmf8829_fake_session},
    {"vlx_unsupported_group_refused", NULL, vlx_unsupported_group_refused},
    {"vlx_l1_set_mode_hands_back_the_budget", NULL, vlx_l1_set_mode_hands_back_the_budget},
    {"dataset_recorder_two_sr04", NULL, dataset_recorder_two_sr04},
    {"dataset_recorder_temperature_gz", NULL, dataset_recorder_temperature_gz},
    {"dataset_recorder_bno086", NULL, dataset_recorder_bno086},
    {"dataset_live_records", NULL, dataset_live_records},
    {"vlx_bare_tables_roundtrip", NULL, vlx_bare_tables_roundtrip},
    {"discovery_without_a_port", NULL, discovery_without_a_port},
    {"discovery_open_on_link_attaches_in_place", NULL, discovery_open_on_link_attaches_in_place},
    {"vlx_l1_mode_change_moves_the_budget", NULL, vlx_l1_mode_change_moves_the_budget},
    {"vlx_l1_unreadable_mode_byte_is_an_error", NULL, vlx_l1_unreadable_mode_byte_is_an_error},
    {"vlx_l1_thresholds_keep_the_other_bits", NULL, vlx_l1_thresholds_keep_the_other_bits},
    {"vlx_l1_offset_calibration", NULL, vlx_l1_offset_calibration},
    {"vlx_l1_xtalk_calibration", NULL, vlx_l1_xtalk_calibration},
    {"vlx_l1_roi_size_and_centre", NULL, vlx_l1_roi_size_and_centre},
    {"vlx_l1_stream_decodes_and_counts_bad_blocks", NULL, vlx_l1_stream_decodes_and_counts_bad_blocks},
    {"vlx_xshut_while_streaming", NULL, vlx_xshut_while_streaming},
    {"vlx_l4_configure_reinits_then_tunes", NULL, vlx_l4_configure_reinits_then_tunes},
    {"vlx_l4_period_selects_autonomous_mode", NULL, vlx_l4_period_selects_autonomous_mode},
    {"vlx_l4_tuning_round_trips", NULL, vlx_l4_tuning_round_trips},
    {"vlx_a_refused_start_stream_stops_the_sensor_again", NULL, vlx_a_refused_start_stream_stops_the_sensor_again},
    {"vlx_l4_temperature_update_reruns_vhv", NULL, vlx_l4_temperature_update_reruns_vhv},
    {"vlx_l4_calibrations", NULL, vlx_l4_calibrations},
    {"vlx_l4_measure_once_polls_the_block", NULL, vlx_l4_measure_once_polls_the_block},
    {"vlx_l3_refusals_name_the_caveat", NULL, vlx_l3_refusals_name_the_caveat},
    {"vlx_init_binding_and_bridge_refusals", NULL, vlx_init_binding_and_bridge_refusals},
    {"b055_identity_and_raw_registers", NULL, b055_identity_and_raw_registers},
    {"b055_operation_and_power_modes", NULL, b055_operation_and_power_modes},
    {"b055_found_asleep_and_stuck_fusion", NULL, b055_found_asleep_and_stuck_fusion},
    {"b055_units_axes_temperature_source", NULL, b055_units_axes_temperature_source},
    {"b055_configure_restore_and_reset", NULL, b055_configure_restore_and_reset},
    {"b055_configure_boot_states", NULL, b055_configure_boot_states},
    {"b055_status_self_test_calibration", NULL, b055_status_self_test_calibration},
    {"b055_page1_configs_and_interrupts", NULL, b055_page1_configs_and_interrupts},
    {"b055_polling_scales_by_the_units", NULL, b055_polling_scales_by_the_units},
    {"b055_stream_wire_units_and_bad_reports", NULL, b055_stream_wire_units_and_bad_reports},
    {"unsolicited_reports_become_events", NULL, unsolicited_reports_become_events},
    {"link_damage_is_counted_and_reported", NULL, link_damage_is_counted_and_reported},
    {"pending_requests_meet_echoes_acks_and_a_drop", NULL, pending_requests_meet_echoes_acks_and_a_drop},
    {"bootloader_ack_closes_the_device", NULL, bootloader_ack_closes_the_device},
    {"payload_crc_type_roundtrip", payload_crc_type_roundtrip, NULL},
    {"reset_acks_then_the_link_drops", reset_acks_then_the_link_drops, NULL},
    {"requests_refused_on_the_host", requests_refused_on_the_host, NULL},
    {"tmf8829_damaged_reports_and_refusals", NULL, tmf8829_damaged_reports_and_refusals},
    {"tmf8829_init_and_configure_refusals", NULL, tmf8829_init_and_configure_refusals},
    {"identity_strings_edge_cases", NULL, identity_strings_edge_cases},
    {"error_names_are_the_enum_names", NULL, error_names_are_the_enum_names},
    {"usb_order_and_short_report_edges", NULL, usb_order_and_short_report_edges},
    {"l4_calibrations_and_temperature_update", NULL, l4_calibrations_and_temperature_update},
    {"l4_callbacks_get_measurement_and_status_texts", NULL, l4_callbacks_get_measurement_and_status_texts},
};

int main(int argc, char **argv)
{
    size_t i, ran = 0;
    if (argc > 1 && strncmp(argv[1], "scenario:", 9) == 0) {
        int rc = scenario_run(argv[1] + 9);
        printf("%s %s\n", rc ? "FAIL" : "ok  ", argv[1]);
        return rc ? 1 : 0;
    }
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
    for (i = 0; argc <= 1 && scenario_ids[i]; i++, ran++) {
        int rc = scenario_run(scenario_ids[i]);
        if (rc) g_failed++;
        printf("%s scenario:%s\n", rc ? "FAIL" : "ok  ", scenario_ids[i]);
    }
    if (!ran) { fprintf(stderr, "no test named %s\n", argc > 1 ? argv[1] : "?"); return 2; }
    return g_failed ? 1 : 0;
}
