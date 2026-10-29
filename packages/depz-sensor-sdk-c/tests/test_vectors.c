/* Golden-vector test runner for the DEPZ C SDK.
 *
 * Loads contracts/vectors/<file>.json and asserts byte-exact behaviour of the
 * SDK. Invoked once per vector file: argv[1] = file stem (e.g. "crc"). The
 * vectors directory is passed via -DDEPZ_VECTORS_DIR at compile time.
 */
#include "depz_sensor_sdk.h"
#if DEPZ_VECTORS_HAS_IO
#include "depz_sensor_io.h" /* the CNH setup helpers (vl53l8_advanced input_rules) */
#include "../src/io/vl53lx_internal.h" /* the VL53L0X ULD seams (vl53lx l0x_uld) */
#include "fake_vl53lx_trace.h"
#endif
#include "json.h"
#include "vector_seen.h"
#include "sha256.h"

#if DEPZ_VECTORS_HAS_IO && !defined(_WIN32)
#include <unistd.h> /* close(): the temporary file of the depzrec_read op */
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef DEPZ_VECTORS_DIR
#define DEPZ_VECTORS_DIR "."
#endif

static int g_fail = 0;
static int g_checks = 0;

#define CHECK(cond, ...) do {                          \
    g_checks++;                                         \
    if (!(cond)) {                                      \
        g_fail++;                                        \
        fprintf(stderr, "  FAIL: ");                     \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n");                           \
    }                                                   \
} while (0)

/* Coverage gate (vector_seen.h): report an ID only when no CHECK failed since
 * `fails_before` was taken — a failing case is never reported. */
static void seen_id(const char *id, int fails_before)
{
    if (g_fail == fails_before) vector_seen(id);
}

/* A case object: its "id" (every case of a case-list group carries one). */
static void seen_case(const json_value *c, int fails_before)
{
    const char *id = json_as_str(json_obj_get(c, "id"));
    CHECK(id != NULL, "vector case without an \"id\"");
    seen_id(id, fails_before);
}

/* --- hex helpers ---------------------------------------------------------- */

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Decode hex string into freshly malloc'd buffer; returns length, sets *out. */
static size_t hex_decode(const char *hex, uint8_t **out)
{
    size_t hl = strlen(hex);
    size_t n = hl / 2;
    uint8_t *b = (uint8_t *)malloc(n ? n : 1);
    for (size_t i = 0; i < n; i++)
        b[i] = (uint8_t)((hexval(hex[2 * i]) << 4) | hexval(hex[2 * i + 1]));
    *out = b;
    return n;
}

static void hex_encode(const uint8_t *b, size_t n, char *out)
{
    static const char *H = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = H[b[i] >> 4];
        out[2 * i + 1] = H[b[i] & 0xF];
    }
    out[2 * n] = '\0';
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(sz + 1);
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static json_value *load_vectors(const char *stem)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.json", DEPZ_VECTORS_DIR, stem);
    char *text = read_file(path);
    if (!text) return NULL;
    json_value *v = json_parse(text);
    free(text);
    if (!v) fprintf(stderr, "JSON parse failed for %s\n", path);
    return v;
}

/* ======================================================================== */
/* crc.json                                                                  */
/* ======================================================================== */
static void test_crc(void)
{
    json_value *root = load_vectors("crc");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *data; size_t n = hex_decode(json_as_str(json_obj_get(c, "input")), &data);

        CHECK(depz_crc8_maxim(data, n) == (uint8_t)json_as_int(json_obj_get(c, "crc8_maxim")),
              "crc8 %s", name);
        CHECK(depz_crc16_modbus(data, n) == (uint16_t)json_as_int(json_obj_get(c, "crc16_modbus")),
              "crc16_modbus %s", name);
        CHECK(depz_crc32_iso_hdlc(data, n) == (uint32_t)json_as_int(json_obj_get(c, "crc32_iso_hdlc")),
              "crc32 %s", name);
        CHECK(depz_crc16_ccitt_false(data, n) == (uint16_t)json_as_int(json_obj_get(c, "crc16_ccitt_false")),
              "crc16_ccitt_false %s", name);
        free(data);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* usb_ids.json                                                              */
/* ======================================================================== */

/* The serial ordering (serial ascending, None/empty last, tie-break by port
 * path) is the SDK's depz_port_order_cmp, the comparator discovery sorts with. */
typedef struct { const char *port; const char *serial; /* NULL = None */ } port_ent;

static int port_cmp(const void *pa, const void *pb)
{
    const port_ent *a = (const port_ent *)pa;
    const port_ent *b = (const port_ent *)pb;
    const depz_port_key ka = { a->port, strlen(a->port), a->serial, a->serial ? strlen(a->serial) : 0 };
    const depz_port_key kb = { b->port, strlen(b->port), b->serial, b->serial ? strlen(b->serial) : 0 };
    return depz_port_order_cmp(&ka, &kb);
}

static void test_usb_ids(void)
{
    json_value *root = load_vectors("usb_ids");
    if (!root) { g_fail++; return; }

    const json_value *ident = json_obj_get(root, "identity");
    for (size_t i = 0; i < json_arr_size(ident); i++) {
        const json_value *c = json_arr_get(ident, i);
        const int f0 = g_fail;
        int vid = (int)json_as_int(json_obj_get(c, "vid"));
        int pid = (int)json_as_int(json_obj_get(c, "pid"));
        bool known = json_as_bool(json_obj_get(c, "known"));
        const json_value *mv = json_obj_get(c, "model");
        const char *model = json_is_null(mv) ? NULL : json_as_str(mv);

        CHECK(depz_is_known_depz_usb(vid, pid) == known,
              "is_known_depz_usb(%04x,%04x) expected %d", vid, pid, known);

        const char *hint = depz_usb_model_hint(vid, pid);
        if (model == NULL)
            CHECK(hint == NULL, "model hint(%04x,%04x) expected NULL got %s", vid, pid, hint ? hint : "NULL");
        else
            CHECK(hint && strcmp(hint, model) == 0,
                  "model hint(%04x,%04x) expected %s got %s", vid, pid, model, hint ? hint : "NULL");
        seen_case(c, f0);
    }

    const json_value *ord = json_obj_get(root, "serial_ordering");
    for (size_t i = 0; i < json_arr_size(ord); i++) {
        const json_value *c = json_arr_get(ord, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *ports = json_obj_get(c, "ports");
        size_t np = json_arr_size(ports);
        port_ent *ents = (port_ent *)calloc(np, sizeof(port_ent));
        for (size_t j = 0; j < np; j++) {
            const json_value *pe = json_arr_get(ports, j);
            ents[j].port = json_as_str(json_obj_get(pe, "port"));
            const json_value *sv = json_obj_get(pe, "serial");
            ents[j].serial = json_is_null(sv) ? NULL : json_as_str(sv);
        }
        qsort(ents, np, sizeof(port_ent), port_cmp);
        const json_value *order = json_obj_get(c, "order");
        for (size_t j = 0; j < np; j++) {
            const char *want = json_as_str(json_arr_get(order, j));
            CHECK(strcmp(ents[j].port, want) == 0,
                  "serial_ordering %s idx %zu expected %s got %s", name, j, want, ents[j].port);
        }
        free(ents);
        seen_case(c, f0);
    }

    /* the id constants themselves */
    {
        const json_value *pr = json_obj_get(root, "pid_range");
        int f0 = g_fail;
        CHECK(json_as_int(json_obj_get(root, "vid")) == DEPZ_USB_VID, "usb_ids vid");
        seen_id("usb_ids/vid", f0);
        f0 = g_fail;
        CHECK(json_as_int(json_obj_get(root, "dev_vid")) == DEPZ_DEV_USB_VID, "usb_ids dev_vid");
        seen_id("usb_ids/dev_vid", f0);
        f0 = g_fail;
        CHECK(json_as_int(json_obj_get(root, "dev_pid")) == DEPZ_DEV_USB_PID, "usb_ids dev_pid");
        seen_id("usb_ids/dev_pid", f0);
        f0 = g_fail;
        CHECK(json_arr_size(pr) == 2 && json_as_int(json_arr_get(pr, 0)) == DEPZ_PID_RANGE_LO &&
                  json_as_int(json_arr_get(pr, 1)) == DEPZ_PID_RANGE_HI,
              "usb_ids pid_range");
        seen_id("usb_ids/pid_range", f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* identity.json                                                             */
/* ======================================================================== */
static void test_identity(void)
{
    json_value *root = load_vectors("identity");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);

        char stripped[128];
        depz_strip_device_string(raw, n, stripped, sizeof(stripped));

        depz_identity id;
        depz_parse_software_name(stripped, &id);

        const json_value *e = json_obj_get(c, "expect");
        CHECK(strcmp(id.mode, json_as_str(json_obj_get(e, "mode"))) == 0, "identity mode %s", name);

        const json_value *st = json_obj_get(e, "sensor_type");
        const char *st_str = depz_sensor_type_str(id.sensor_type);
        if (json_is_null(st))
            CHECK(st_str == NULL, "identity sensor_type %s expected null got %s", name, st_str ? st_str : "null");
        else
            CHECK(st_str && strcmp(st_str, json_as_str(st)) == 0, "identity sensor_type %s", name);

        CHECK(strcmp(id.software_name, json_as_str(json_obj_get(e, "software_name"))) == 0,
              "identity software_name %s: got '%s'", name, id.software_name);
        CHECK(strcmp(id.version, json_as_str(json_obj_get(e, "version"))) == 0,
              "identity version %s: got '%s'", name, id.version);
        free(raw);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* common_commands.json                                                      */
/* ======================================================================== */
static void check_payload(const char *label, const uint8_t *got, size_t got_n, const char *want_hex)
{
    char buf[4096];
    if (!want_hex || got_n * 2 >= sizeof buf) {
        CHECK(0, "%s: %zu bytes, no expected payload to compare", label, got_n);
        return;
    }
    hex_encode(got, got_n, buf);
    CHECK(strcmp(buf, want_hex) == 0, "%s: got %s want %s", label, buf, want_hex);
}

static void test_common_commands(void)
{
    json_value *root = load_vectors("common_commands");
    if (!root) { g_fail++; return; }

    const json_value *enc = json_obj_get(root, "encode");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[64]; size_t n = 0;
        if (strcmp(kind, "sync_time_request") == 0) {
            n = depz_pack_sync_time((uint64_t)json_as_int(json_obj_get(c, "pc_timestamp_us")), out);
        } else if (strcmp(kind, "set_payload_crc_type") == 0) {
            n = depz_pack_set_payload_crc_type((uint8_t)json_as_int(json_obj_get(c, "crc_type")), out);
        } else if (strcmp(kind, "sync_pin_config") == 0) {
            depz_sync_pin_config cfg = {
                (uint8_t)json_as_int(json_obj_get(c, "pin")),
                (uint8_t)json_as_int(json_obj_get(c, "mode")),
                (uint8_t)json_as_int(json_obj_get(c, "polarity")) };
            n = depz_pack_sync_pin_config(&cfg, out);
        } else { CHECK(0, "unknown common encode kind %s", kind); continue; }
        check_payload(json_as_str(json_obj_get(c, "name")), out, n, want);
        seen_case(c, f0);
    }
    /* refusals: SET_PAYLOAD_CRC_TYPE takes 0..3 (contract 02 §2, ERRATA A2) */
    {
        uint8_t out[4];
        CHECK(depz_pack_set_payload_crc_type(3, out) == 1 && out[0] == 3, "crc_type 3 packs");
        CHECK(depz_pack_set_payload_crc_type(4, out) == 0 && depz_pack_set_payload_crc_type(255, out) == 0,
              "crc_type 4 / 255 are refused");
    }

    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int rpt = (int)json_as_int(json_obj_get(c, "report"));
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");

        if (rpt == DEPZ_RPT_STATUS) {
            depz_status_report r;
            CHECK(depz_unpack_status(p, n, &r) == 0, "status decode %s", name);
            CHECK(r.cmd == json_as_int(json_obj_get(e, "cmd")) &&
                  r.status == json_as_int(json_obj_get(e, "status")), "status fields %s", name);
        } else if (rpt == DEPZ_RPT_TEXT) {
            depz_text_report r;
            CHECK(depz_unpack_text(p, n, &r) == 0, "text decode %s", name);
            CHECK(r.cmd == json_as_int(json_obj_get(e, "cmd")) &&
                  strcmp(r.text, json_as_str(json_obj_get(e, "text"))) == 0,
                  "text fields %s: got cmd=%d '%s'", name, r.cmd, r.text);
        } else if (rpt == DEPZ_RPT_TEMPERATURE) {
            depz_temperature_report r;
            CHECK(depz_unpack_temperature(p, n, &r) == 0, "temp decode %s", name);
            CHECK((int64_t)r.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")) &&
                  r.raw_decidegrees == json_as_int(json_obj_get(e, "raw_decidegrees")),
                  "temp fields %s: ts=%llu raw=%d", name,
                  (unsigned long long)r.timestamp_us, r.raw_decidegrees);
        } else if (rpt == DEPZ_RPT_SEQUENCE_ERROR) {
            depz_sequence_error_report r;
            CHECK(depz_unpack_sequence_error(p, n, &r) == 0, "seqerr decode %s", name);
            CHECK(r.expected_seq == json_as_int(json_obj_get(e, "expected_seq")) &&
                  r.received_seq == json_as_int(json_obj_get(e, "received_seq")), "seqerr fields %s", name);
        } else {
            CHECK(0, "unknown common decode report 0x%02x (%s)", rpt, name);
        }
        free(p);
        seen_case(c, f0);
    }

    const json_value *st = json_obj_get(root, "sync_time_math");
    for (size_t i = 0; i < json_arr_size(st); i++) {
        const json_value *c = json_arr_get(st, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int64_t off, rtt;
        depz_sync_time_offset_rtt(
            json_as_int(json_obj_get(c, "t1")), json_as_int(json_obj_get(c, "t2")),
            json_as_int(json_obj_get(c, "t3")), json_as_int(json_obj_get(c, "t4")), &off, &rtt);
        CHECK(off == json_as_int(json_obj_get(c, "offset_us")), "sync offset %s: got %lld", name, (long long)off);
        CHECK(rtt == json_as_int(json_obj_get(c, "rtt_us")), "sync rtt %s: got %lld", name, (long long)rtt);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* sr04.json                                                                 */
/* ======================================================================== */
static void test_sr04(void)
{
    json_value *root = load_vectors("sr04");
    if (!root) { g_fail++; return; }

    const json_value *enc = json_obj_get(root, "encode");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[8]; size_t n = 0;
        if (strcmp(kind, "set_sample_period") == 0)
            n = depz_sr04_pack_sample_period((uint32_t)json_as_int(json_obj_get(c, "period_us")), out);
        else if (strcmp(kind, "set_echo_decay") == 0)
            n = depz_sr04_pack_echo_decay((uint16_t)json_as_int(json_obj_get(c, "decay_us")), out);
        else { CHECK(0, "unknown sr04 encode kind %s", kind); continue; }
        check_payload(json_as_str(json_obj_get(c, "name")), out, n, want);
        seen_case(c, f0);
    }

    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int rpt = (int)json_as_int(json_obj_get(c, "report"));
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");

        if (rpt == DEPZ_SR04_RPT_DATA) {
            depz_sr04_data d;
            CHECK(depz_sr04_unpack_data(p, n, &d) == 0, "sr04 data decode %s", name);
            CHECK(d.source_cmd == json_as_int(json_obj_get(e, "source_cmd")) &&
                  (int64_t)d.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")) &&
                  d.echo_time_us == json_as_int(json_obj_get(e, "echo_time_us")),
                  "sr04 data fields %s: src=%d ts=%llu echo=%u", name,
                  d.source_cmd, (unsigned long long)d.timestamp_us, d.echo_time_us);
        } else if (rpt == DEPZ_SR04_RPT_SAMPLE_PERIOD) {
            uint32_t v;
            CHECK(depz_sr04_unpack_sample_period(p, n, &v) == 0, "sr04 period decode %s", name);
            CHECK((int64_t)v == json_as_int(json_obj_get(e, "period_us")), "sr04 period %s", name);
        } else if (rpt == DEPZ_SR04_RPT_ECHO_DECAY) {
            uint16_t v;
            CHECK(depz_sr04_unpack_echo_decay(p, n, &v) == 0, "sr04 decay decode %s", name);
            CHECK((int64_t)v == json_as_int(json_obj_get(e, "decay_us")), "sr04 decay %s", name);
        } else {
            CHECK(0, "unknown sr04 decode report 0x%02x (%s)", rpt, name);
        }
        free(p);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* framing_encode.json                                                       */
/* ======================================================================== */
static void test_framing_encode(void)
{
    json_value *root = load_vectors("framing_encode");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t cmd = (uint8_t)json_as_int(json_obj_get(c, "cmd"));
        unsigned int seq = (unsigned int)json_as_int(json_obj_get(c, "seq"));
        depz_crc_type ct = (depz_crc_type)json_as_int(json_obj_get(c, "crc_type"));
        uint8_t *payload; size_t pn = hex_decode(json_as_str(json_obj_get(c, "payload")), &payload);
        const char *want = json_as_str(json_obj_get(c, "frame"));

        uint8_t *out = (uint8_t *)malloc(DEPZ_MAX_FRAME);
        size_t on = 0;
        int rc = depz_build_packet(cmd, pn ? payload : NULL, pn, seq, ct, out, DEPZ_MAX_FRAME, &on);
        CHECK(rc == 0, "build_packet %s rc=%d", name, rc);
        char *hex = (char *)malloc(2 * on + 1);
        hex_encode(out, on, hex);
        CHECK(strcmp(hex, want) == 0, "framing_encode %s:\n    got  %s\n    want %s", name, hex, want);
        free(hex); free(out); free(payload);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* framing_decode.json — with chunking-invariance                            */
/* ======================================================================== */

typedef struct {
    char   *events;    /* canonical event string */
    size_t  ev_cap, ev_len;
    uint8_t *trash;    /* concatenated trash bytes */
    size_t  tr_cap, tr_len;
} decode_result;

static void dr_append_str(decode_result *r, const char *s)
{
    size_t sl = strlen(s);
    if (r->ev_len + sl + 1 > r->ev_cap) {
        r->ev_cap = (r->ev_cap + sl + 1) * 2;
        r->events = (char *)realloc(r->events, r->ev_cap);
    }
    memcpy(r->events + r->ev_len, s, sl + 1);
    r->ev_len += sl;
}

static void dr_append_trash(decode_result *r, const uint8_t *b, size_t n)
{
    if (r->tr_len + n > r->tr_cap) {
        r->tr_cap = (r->tr_len + n) * 2 + 16;
        r->trash = (uint8_t *)realloc(r->trash, r->tr_cap);
    }
    memcpy(r->trash + r->tr_len, b, n);
    r->tr_len += n;
}

static void decode_cb(const depz_event *ev, void *user)
{
    decode_result *r = (decode_result *)user;
    char hdr[64];
    if (ev->type == DEPZ_EV_PACKET) {
        snprintf(hdr, sizeof(hdr), "P:%u:%u:", ev->cmd, ev->seq);
        dr_append_str(r, hdr);
        char *ph = (char *)malloc(ev->payload_len * 2 + 1);
        hex_encode(ev->payload, ev->payload_len, ph);
        dr_append_str(r, ph);
        free(ph);
        dr_append_str(r, ";");
    } else if (ev->type == DEPZ_EV_CRC_ERROR) {
        snprintf(hdr, sizeof(hdr), "C:%u:%u;", ev->cmd, ev->seq);
        dr_append_str(r, hdr);
    } else { /* trash */
        dr_append_trash(r, ev->trash, ev->trash_len);
    }
}

/* Run parser over `stream` with a chunking mode; capture result + residue. */
static void run_decode(const uint8_t *stream, size_t n, int mode, unsigned seed,
                       decode_result *r, char *residue_hex, uint64_t *hdr_errors)
{
    memset(r, 0, sizeof(*r));
    r->events = (char *)malloc(1); r->events[0] = '\0'; r->ev_cap = 1;
    depz_parser p;
    depz_parser_init(&p);
    if (mode == 0) {
        depz_parser_feed(&p, stream, n, decode_cb, r);
    } else if (mode == 1) {
        for (size_t i = 0; i < n; i++)
            depz_parser_feed(&p, stream + i, 1, decode_cb, r);
    } else {
        unsigned st = seed;
        size_t i = 0;
        while (i < n) {
            st = st * 1103515245u + 12345u;
            size_t chunk = 1 + (st >> 8) % 7;
            if (i + chunk > n) chunk = n - i;
            depz_parser_feed(&p, stream + i, chunk, decode_cb, r);
            i += chunk;
        }
    }
    hex_encode(p.buf, p.len, residue_hex);
    *hdr_errors = p.header_errors;
    depz_parser_free(&p);
}

static void dr_free(decode_result *r) { free(r->events); free(r->trash); }

static void test_framing_decode(void)
{
    json_value *root = load_vectors("framing_decode");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *stream; size_t n = hex_decode(json_as_str(json_obj_get(c, "stream")), &stream);
        const json_value *e = json_obj_get(c, "expect");

        /* Build expected canonical event string (dynamic — payloads can be
         * up to 16 KB, e.g. max_payload_roundtrip). */
        decode_result expect_r; memset(&expect_r, 0, sizeof(expect_r));
        expect_r.events = (char *)malloc(1); expect_r.events[0] = '\0'; expect_r.ev_cap = 1;
        const json_value *evs = json_obj_get(e, "events");
        for (size_t j = 0; j < json_arr_size(evs); j++) {
            const json_value *ev = json_arr_get(evs, j);
            const char *type = json_as_str(json_obj_get(ev, "type"));
            char hdr[64];
            if (strcmp(type, "packet") == 0) {
                snprintf(hdr, sizeof(hdr), "P:%d:%d:",
                         (int)json_as_int(json_obj_get(ev, "cmd")),
                         (int)json_as_int(json_obj_get(ev, "seq")));
                dr_append_str(&expect_r, hdr);
                dr_append_str(&expect_r, json_as_str(json_obj_get(ev, "payload")));
                dr_append_str(&expect_r, ";");
            } else {
                snprintf(hdr, sizeof(hdr), "C:%d:%d;",
                         (int)json_as_int(json_obj_get(ev, "cmd")),
                         (int)json_as_int(json_obj_get(ev, "seq")));
                dr_append_str(&expect_r, hdr);
            }
        }
        const char *expect_events = expect_r.events;
        const char *want_trash = json_as_str(json_obj_get(e, "trash"));
        const char *want_residue = json_as_str(json_obj_get(e, "residue"));
        uint64_t want_hdr = (uint64_t)json_as_int(json_obj_get(e, "header_errors"));

        /* Residue buffer sized for whole stream. */
        char *residue = (char *)malloc(2 * n + 1);

        int modes[6] = {0, 1, 2, 2, 2, 2};
        for (int m = 0; m < 6; m++) {
            decode_result r;
            uint64_t hdr;
            run_decode(stream, n, modes[m], (unsigned)(0x1234 + m * 7919), &r, residue, &hdr);

            char *trash_hex = (char *)malloc(2 * r.tr_len + 1);
            hex_encode(r.trash, r.tr_len, trash_hex);

            const char *tag = modes[m] == 0 ? "whole" : modes[m] == 1 ? "bytewise" : "random";
            CHECK(strcmp(r.events, expect_events) == 0,
                  "decode %s [%s] events:\n    got  %s\n    want %s", name, tag, r.events, expect_events);
            CHECK(strcmp(trash_hex, want_trash) == 0,
                  "decode %s [%s] trash: got %s want %s", name, tag, trash_hex, want_trash);
            CHECK(strcmp(residue, want_residue) == 0,
                  "decode %s [%s] residue: got %s want %s", name, tag, residue, want_residue);
            CHECK(hdr == want_hdr, "decode %s [%s] header_errors: got %llu want %llu",
                  name, tag, (unsigned long long)hdr, (unsigned long long)want_hdr);
            free(trash_hex);
            dr_free(&r);
        }
        free(residue);
        dr_free(&expect_r);
        free(stream);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* fwdepz.json                                                               */
/* ======================================================================== */
static void test_bootloader_codecs(void);

static void test_fwdepz(void)
{
    json_value *root = load_vectors("fwdepz");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *blob; size_t n = hex_decode(json_as_str(json_obj_get(c, "file")), &blob);

        depz_fwdepz_image img;
        depz_fwdepz_result rc = depz_fwdepz_parse(blob, n, &img);

        const json_value *err = json_obj_get(c, "error");
        if (err && !json_is_null(err)) {
            const char *want = json_as_str(err);
            depz_fwdepz_result exp =
                strcmp(want, "magic") == 0 ? DEPZ_FWDEPZ_ERR_MAGIC :
                strcmp(want, "header_crc") == 0 ? DEPZ_FWDEPZ_ERR_HEADER_CRC :
                strcmp(want, "size") == 0 ? DEPZ_FWDEPZ_ERR_SIZE :
                strcmp(want, "section") == 0 ? DEPZ_FWDEPZ_ERR_SECTION : DEPZ_FWDEPZ_ERR_TOO_SHORT;
            CHECK(rc == exp, "fwdepz %s: expected error %s got %d", name, want, rc);
        } else {
            const json_value *e = json_obj_get(c, "expect");
            CHECK(rc == DEPZ_FWDEPZ_OK, "fwdepz %s: expected OK got %d", name, rc);
            if (rc == DEPZ_FWDEPZ_OK) {
                CHECK((int64_t)img.load_addr == json_as_int(json_obj_get(e, "load_addr")), "fwdepz load_addr %s", name);
                CHECK((int64_t)img.fw_size == json_as_int(json_obj_get(e, "fw_size")), "fwdepz fw_size %s", name);
                CHECK((int64_t)img.fw_crc32 == json_as_int(json_obj_get(e, "fw_crc32")), "fwdepz fw_crc32 %s", name);
                CHECK(img.cur_sec == json_as_int(json_obj_get(e, "cur_sec")), "fwdepz cur_sec %s", name);
                CHECK(img.tot_sec == json_as_int(json_obj_get(e, "tot_sec")), "fwdepz tot_sec %s", name);
                CHECK(img.payload_crc_ok == json_as_bool(json_obj_get(e, "payload_crc_ok")), "fwdepz payload_crc_ok %s", name);
            }
        }
        free(blob);
        seen_case(c, f0);
    }
    json_free(root);
    test_bootloader_codecs();
}

/* Contract 06 §1: the bootloader payloads the flashing client sends and reads
 * (the Python reference's pack_write_page / pack_read_page / FlashInfo). */
static void test_bootloader_codecs(void)
{
    /* WRITE_PAGE: addr u32 LE, size u16 LE, data[size] */
    static const uint8_t DATA[3] = {0xAA, 0xBB, 0xCC};
    static const uint8_t WANT_WRITE[9] = {0x00, 0x10, 0x00, 0x08, 0x03, 0x00, 0xAA, 0xBB, 0xCC};
    /* READ_PAGE: addr u32 LE, size u16 LE */
    static const uint8_t WANT_READ[6] = {0x00, 0xF8, 0x00, 0x08, 0x00, 0x01};
    /* RPT_FLASH_INFO: cmd, page_size u16, rsvd u16, app_start u32, app_size u32 */
    static const uint8_t FLASH_INFO[13] = {0x89, 0x00, 0x08, 0x00, 0x00,
                                           0x00, 0x40, 0x00, 0x08, 0x00, 0xC0, 0x01, 0x00};
    uint8_t out[16];
    depz_flash_info fi;

    memset(out, 0x5A, sizeof out);
    CHECK(depz_bl_pack_write_page(0x08001000u, DATA, 3, out, sizeof out) == 9, "write_page: 6 + 3 bytes");
    CHECK(memcmp(out, WANT_WRITE, 9) == 0, "write_page: addr, size, data little-endian");
    CHECK(depz_bl_pack_write_page(0x08001000u, DATA, 3, out, 8) == 0, "write_page: refused when out_cap < 6 + len");
    CHECK(depz_bl_pack_write_page(0x08001000u, NULL, 0, out, 6) == 6, "write_page: an empty page is the header alone");
    CHECK(out[4] == 0 && out[5] == 0, "write_page: empty page has size 0");

    memset(out, 0x5A, sizeof out);
    CHECK(depz_bl_pack_read_page(0x0800F800u, 0x0100, out) == 6, "read_page: 6 bytes");
    CHECK(memcmp(out, WANT_READ, 6) == 0, "read_page: addr, size little-endian");

    memset(&fi, 0, sizeof fi);
    CHECK(depz_bl_unpack_flash_info(FLASH_INFO, 13, &fi) == 0, "flash_info: 13 bytes decode");
    CHECK(fi.page_size == 2048, "flash_info: page_size 0x0800, got %u", (unsigned)fi.page_size);
    CHECK(fi.app_start == 0x08004000u, "flash_info: app_start, got 0x%08X", (unsigned)fi.app_start);
    CHECK(fi.app_size == 0x0001C000u, "flash_info: app_size, got 0x%08X", (unsigned)fi.app_size);
    CHECK(depz_bl_unpack_flash_info(FLASH_INFO, 12, &fi) == -1, "flash_info: a short report is refused");
    CHECK(depz_bl_unpack_flash_info(FLASH_INFO, 14, &fi) == -1, "flash_info: a long report is refused");
}

/* ======================================================================== */
/* vl53l8_advanced.json — advanced DCI codecs SHARED by VL53L8CX and CH.      */
/* ======================================================================== */
static void test_vl53l8_advanced(void)
{
    json_value *root = load_vectors("vl53l8_advanced");
    if (!root) { g_fail++; return; }

    const json_value *motion = json_obj_get(root, "motion");
    for (size_t i = 0; i < json_arr_size(motion); i++) {
        const json_value *c = json_arr_get(motion, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int res = (int)json_as_int(json_obj_get(c, "resolution"));
        uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE];
        CHECK(depz_vl53l8_motion_cfg_default_pack(res, out) == 0, "motion pack rc %s", name);
        char hex[DEPZ_VL53L8_MOTION_CFG_SIZE * 2 + 1];
        hex_encode(out, sizeof(out), hex);
        check_payload(name, out, sizeof(out), json_as_str(json_obj_get(c, "pack")));
        seen_case(c, f0);
    }

    const json_value *thr = json_obj_get(root, "thresholds");
    for (size_t i = 0; i < json_arr_size(thr); i++) {
        const json_value *c = json_arr_get(thr, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *list = json_obj_get(c, "thresholds");
        size_t n = json_arr_size(list);
        depz_vl53l8_threshold th[DEPZ_VL53L8_NB_THRESHOLDS];
        memset(th, 0, sizeof(th));
        for (size_t k = 0; k < n && k < DEPZ_VL53L8_NB_THRESHOLDS; k++) {
            const json_value *e = json_arr_get(list, k);
            th[k].low_thresh  = (int32_t)json_as_int(json_obj_get(e, "low_thresh"));
            th[k].high_thresh = (int32_t)json_as_int(json_obj_get(e, "high_thresh"));
            th[k].measurement = (uint8_t)json_as_int(json_obj_get(e, "measurement"));
            th[k].type        = (uint8_t)json_as_int(json_obj_get(e, "type"));
            th[k].zone_num    = (uint8_t)json_as_int(json_obj_get(e, "zone_num"));
            th[k].operation   = (uint8_t)json_as_int(json_obj_get(e, "operation"));
        }
        uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
        depz_vl53l8_pack_thresholds(th, n, start, valid);
        check_payload(name, start, sizeof(start), json_as_str(json_obj_get(c, "start_block")));
        check_payload(name, valid, sizeof(valid), json_as_str(json_obj_get(c, "valid_status")));
        seen_case(c, f0);
    }

    const json_value *xm = json_obj_get(root, "xtalk_margin");
    for (size_t i = 0; i < json_arr_size(xm); i++) {
        const json_value *c = json_arr_get(xm, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint32_t kcps = (uint32_t)json_as_int(json_obj_get(c, "kcps"));
        uint32_t raw = 0;
        CHECK(depz_vl53l8_xtalk_margin_to_raw(kcps, &raw) == 0, "xtalk margin %s: refused", name);
        CHECK((int64_t)raw == json_as_int(json_obj_get(c, "raw")),
              "xtalk margin %s: got %u", name, raw);
        CHECK(depz_vl53l8_xtalk_margin_from_raw(raw) == kcps, "xtalk margin %s: from_raw", name);
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* vl53l4.json — VL53L4CD wire codecs + host-ULD math (contract 10).         */
/* Consumes every section: encode / decode / result_block / timing.encode /  */
/* timing.decode / tuning (both directions) / xtalk_decode / config_block.   */
/* ======================================================================== */
static void test_vl53l4(void)
{
    json_value *root = load_vectors("vl53l4");
    if (!root) { g_fail++; return; }

    /* encode: command payloads 0x32..0x38 */
    const json_value *enc = json_obj_get(root, "encode");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[2 + DEPZ_VL53L4_XFER_MAX]; size_t n = 0;
        if (strcmp(kind, "read_reg") == 0) {
            n = depz_vl53l4_pack_read_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")),
                (uint16_t)json_as_int(json_obj_get(c, "len")), out);
        } else if (strcmp(kind, "write_reg") == 0) {
            uint8_t *data; size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = depz_vl53l4_pack_write_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")), data, dn, out);
            free(data);
        } else if (strcmp(kind, "xshut") == 0) {
            n = depz_vl53l4_pack_xshut(
                (uint8_t)json_as_int(json_obj_get(c, "action")), out);
        } else if (strcmp(kind, "start_stream") == 0) {
            n = depz_vl53l4_pack_start_stream(
                (uint16_t)json_as_int(json_obj_get(c, "addr")),
                (uint16_t)json_as_int(json_obj_get(c, "len")),
                (uint8_t)json_as_int(json_obj_get(c, "flags")), out);
        } else if (strcmp(kind, "set_i2c_speed") == 0) {
            n = depz_vl53l4_pack_set_i2c_speed(
                (uint16_t)json_as_int(json_obj_get(c, "khz")), out);
        } else { CHECK(0, "unknown vl53l4 encode kind %s", kind); continue; }
        check_payload(name, out, n, want);
        seen_case(c, f0);
    }
    /* refusals (contract 10 §1, ERRATA A13): READ_REG len 1..253 and addr+len <= 0x10000, like WRITE_REG */
    {
        uint8_t out[2 + DEPZ_VL53L4_XFER_MAX];
        static const uint8_t two[2] = {1, 2};
        CHECK(depz_vl53l4_pack_read_reg(0x89, 0, out) == 0, "vl53l4 read_reg rejects 0 B");
        CHECK(depz_vl53l4_pack_read_reg(0, DEPZ_VL53L4_XFER_MAX + 1, out) == 0, "vl53l4 read_reg rejects 254 B");
        CHECK(depz_vl53l4_pack_read_reg(0xFFFF, 2, out) == 0, "vl53l4 read_reg rejects addr+len > 0x10000");
        CHECK(depz_vl53l4_pack_read_reg(0xFF03, DEPZ_VL53L4_XFER_MAX, out) == 4 &&
                  depz_vl53l4_pack_read_reg(0xFFFF, 1, out) == 4, "vl53l4 read_reg accepts the limits");
        CHECK(depz_vl53l4_pack_write_reg(0xFFFF, two, 2, out) == 0, "vl53l4 write_reg rejects addr+len > 0x10000");
        CHECK(depz_vl53l4_pack_write_reg(0xFFFF, two, 1, out) == 3, "vl53l4 write_reg accepts the last byte");
    }
    /* contract 10 §5 (ERRATA A8): the result block is 17 bytes — 15 and 16 are short, a tail is ignored */
    {
        uint8_t raw[DEPZ_VL53L4_RESULT_BLOCK_LEN + 2] = {0};
        depz_vl53l4_result a, b;
        CHECK(depz_vl53l4_parse_result_block(raw, 14, &a) != 0 && depz_vl53l4_parse_result_block(raw, 15, &a) != 0 &&
                  depz_vl53l4_parse_result_block(raw, 16, &a) != 0, "vl53l4 result block < 17 B is rejected");
        CHECK(depz_vl53l4_parse_result_block(raw, 17, &a) == 0 && depz_vl53l4_parse_result_block(raw, 19, &b) == 0 &&
                  memcmp(&a, &b, sizeof a) == 0, "vl53l4 result block: 17 B decodes, a tail is ignored");
    }

    /* decode: RPT_VL53_REG_DATA / _INFO / _STREAM */
    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int rpt = (int)json_as_int(json_obj_get(c, "report"));
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");

        if (rpt == DEPZ_VL53L4_RPT_REG_DATA) {
            depz_vl53l4_reg_data r;
            CHECK(depz_vl53l4_unpack_reg_data(p, n, &r) == 0, "vl53l4 reg_data decode %s", name);
            CHECK(r.cmd == json_as_int(json_obj_get(e, "cmd")) &&
                  (int64_t)r.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")),
                  "vl53l4 reg_data fields %s: cmd=%u ts=%llu", name,
                  r.cmd, (unsigned long long)r.timestamp_us);
            char buf[512];
            hex_encode(r.data, r.data_len, buf);
            CHECK(strcmp(buf, json_as_str(json_obj_get(e, "data"))) == 0,
                  "vl53l4 reg_data data %s: got %s", name, buf);
        } else if (rpt == DEPZ_VL53L4_RPT_INFO) {
            depz_vl53l4_info r;
            CHECK(depz_vl53l4_unpack_info(p, n, &r) == 0, "vl53l4 info decode %s", name);
            CHECK((int64_t)r.int_edges == json_as_int(json_obj_get(e, "int_edges")) &&
                  (int64_t)r.slots_skipped == json_as_int(json_obj_get(e, "slots_skipped")) &&
                  (int64_t)r.i2c_errors == json_as_int(json_obj_get(e, "i2c_errors")) &&
                  r.last_i2c_error == json_as_int(json_obj_get(e, "last_i2c_error")) &&
                  r.model_id == json_as_int(json_obj_get(e, "model_id")) &&
                  r.fw_status == json_as_int(json_obj_get(e, "fw_status")) &&
                  r.initialized == json_as_int(json_obj_get(e, "initialized")) &&
                  r.xshut_level == json_as_int(json_obj_get(e, "xshut_level")) &&
                  r.int_level == json_as_int(json_obj_get(e, "int_level")) &&
                  r.i2c_khz == json_as_int(json_obj_get(e, "i2c_khz")),
                  "vl53l4 info fields %s", name);
        } else if (rpt == DEPZ_VL53L4_RPT_STREAM) {
            depz_vl53l4_stream r;
            CHECK(depz_vl53l4_unpack_stream(p, n, &r) == 0, "vl53l4 stream decode %s", name);
            CHECK((int64_t)r.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")) &&
                  r.addr == json_as_int(json_obj_get(e, "addr")) &&
                  r.len == json_as_int(json_obj_get(e, "len")),
                  "vl53l4 stream fields %s: ts=%llu addr=%u len=%u", name,
                  (unsigned long long)r.timestamp_us, r.addr, r.len);
            char buf[512];
            hex_encode(r.data, r.len, buf);
            CHECK(strcmp(buf, json_as_str(json_obj_get(e, "data"))) == 0,
                  "vl53l4 stream data %s: got %s", name, buf);
        } else {
            CHECK(0, "unknown vl53l4 decode report 0x%02x (%s)", rpt, name);
        }
        free(p);
        seen_case(c, f0);
    }

    /* result_block: 17-byte 0x0089 block -> VL53L4CD_ResultsData_t */
    const json_value *rb = json_obj_get(root, "result_block");
    for (size_t i = 0; i < json_arr_size(rb); i++) {
        const json_value *c = json_arr_get(rb, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);
        const json_value *e = json_obj_get(c, "expect");
        depz_vl53l4_result r;
        CHECK(depz_vl53l4_parse_result_block(raw, n, &r) == 0,
              "vl53l4 result_block %s parse", name);
        CHECK((int64_t)r.range_status == json_as_int(json_obj_get(e, "range_status")),
              "vl53l4 %s range_status: got %d", name, r.range_status);
        CHECK((int64_t)r.distance_mm == json_as_int(json_obj_get(e, "distance_mm")),
              "vl53l4 %s distance_mm: got %d", name, r.distance_mm);
        CHECK((int64_t)r.ambient_rate_kcps == json_as_int(json_obj_get(e, "ambient_rate_kcps")),
              "vl53l4 %s ambient_rate_kcps: got %d", name, r.ambient_rate_kcps);
        CHECK((int64_t)r.ambient_per_spad_kcps == json_as_int(json_obj_get(e, "ambient_per_spad_kcps")),
              "vl53l4 %s ambient_per_spad_kcps: got %d", name, r.ambient_per_spad_kcps);
        CHECK((int64_t)r.signal_rate_kcps == json_as_int(json_obj_get(e, "signal_rate_kcps")),
              "vl53l4 %s signal_rate_kcps: got %d", name, r.signal_rate_kcps);
        CHECK((int64_t)r.signal_per_spad_kcps == json_as_int(json_obj_get(e, "signal_per_spad_kcps")),
              "vl53l4 %s signal_per_spad_kcps: got %d", name, r.signal_per_spad_kcps);
        CHECK((int64_t)r.number_of_spad == json_as_int(json_obj_get(e, "number_of_spad")),
              "vl53l4 %s number_of_spad: got %d", name, r.number_of_spad);
        CHECK((int64_t)r.sigma_mm == json_as_int(json_obj_get(e, "sigma_mm")),
              "vl53l4 %s sigma_mm: got %d", name, r.sigma_mm);
        CHECK((int64_t)r.stream_count == json_as_int(json_obj_get(e, "stream_count")),
              "vl53l4 %s stream_count: got %d", name, r.stream_count);
        free(raw);
        seen_case(c, f0);
    }

    /* timing.encode / timing.decode: SetRangeTiming / GetRangeTiming math */
    const json_value *timing = json_obj_get(root, "timing");
    const json_value *tenc = json_obj_get(timing, "encode");
    const int timing_f0 = g_fail;
    CHECK(json_arr_size(tenc) > 0 && json_arr_size(json_obj_get(timing, "decode")) > 0,
          "vl53l4 timing encode/decode cases present");
    for (size_t i = 0; i < json_arr_size(tenc); i++) {
        const json_value *c = json_arr_get(tenc, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint16_t a = 0, b = 0; uint32_t inter_raw = 0;
        int rc = depz_vl53l4_range_timing_registers(
            (uint32_t)json_as_int(json_obj_get(c, "timing_budget_ms")),
            (uint32_t)json_as_int(json_obj_get(c, "inter_measurement_ms")),
            (uint16_t)json_as_int(json_obj_get(c, "osc_frequency")),
            (uint16_t)json_as_int(json_obj_get(c, "clock_pll")),
            &a, &b, &inter_raw);
        CHECK(rc == 0, "vl53l4 timing encode %s rc=%d", name, rc);
        CHECK((int64_t)a == json_as_int(json_obj_get(c, "range_config_a")),
              "vl53l4 timing %s range_config_a: got %u", name, a);
        CHECK((int64_t)b == json_as_int(json_obj_get(c, "range_config_b")),
              "vl53l4 timing %s range_config_b: got %u", name, b);
        CHECK((int64_t)inter_raw == json_as_int(json_obj_get(c, "intermeasurement_raw")),
              "vl53l4 timing %s intermeasurement_raw: got %u", name, inter_raw);
    }
    const json_value *tdec = json_obj_get(timing, "decode");
    for (size_t i = 0; i < json_arr_size(tdec); i++) {
        const json_value *c = json_arr_get(tdec, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint32_t budget = 0, inter = 0;
        int rc = depz_vl53l4_decode_range_timing(
            (uint32_t)json_as_int(json_obj_get(c, "intermeasurement_raw")),
            (uint16_t)json_as_int(json_obj_get(c, "clock_pll")),
            (uint16_t)json_as_int(json_obj_get(c, "osc_frequency")),
            (uint16_t)json_as_int(json_obj_get(c, "range_config_a")),
            &budget, &inter);
        CHECK(rc == 0, "vl53l4 timing decode %s rc=%d", name, rc);
        CHECK((int64_t)budget == json_as_int(json_obj_get(c, "timing_budget_ms")),
              "vl53l4 timing %s timing_budget_ms: got %u", name, budget);
        CHECK((int64_t)inter == json_as_int(json_obj_get(c, "inter_measurement_ms")),
              "vl53l4 timing %s inter_measurement_ms: got %u", name, inter);
    }
    seen_id("vl53l4/timing", timing_f0);

    /* tuning: word codecs, both directions */
    const json_value *tun = json_obj_get(root, "tuning");
    for (size_t i = 0; i < json_arr_size(tun); i++) {
        const json_value *c = json_arr_get(tun, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        int64_t value = json_as_int(json_obj_get(c, "value"));
        int64_t raw = json_as_int(json_obj_get(c, "raw"));
        if (strcmp(kind, "offset") == 0) {
            CHECK((int64_t)depz_vl53l4_offset_raw((int32_t)value) == raw,
                  "vl53l4 tuning %s raw: got %u", name, depz_vl53l4_offset_raw((int32_t)value));
            CHECK((int64_t)depz_vl53l4_decode_offset((uint16_t)raw) == value,
                  "vl53l4 tuning %s decode: got %d", name, depz_vl53l4_decode_offset((uint16_t)raw));
        } else if (strcmp(kind, "xtalk") == 0) {
            CHECK((int64_t)depz_vl53l4_xtalk_raw((uint16_t)value) == raw,
                  "vl53l4 tuning %s raw: got %u", name, depz_vl53l4_xtalk_raw((uint16_t)value));
            CHECK((int64_t)depz_vl53l4_decode_xtalk((uint16_t)raw) == value,
                  "vl53l4 tuning %s decode: got %u", name, depz_vl53l4_decode_xtalk((uint16_t)raw));
        } else if (strcmp(kind, "signal_threshold") == 0) {
            CHECK((int64_t)depz_vl53l4_signal_threshold_raw((uint16_t)value) == raw,
                  "vl53l4 tuning %s raw: got %u", name,
                  depz_vl53l4_signal_threshold_raw((uint16_t)value));
            CHECK((int64_t)depz_vl53l4_decode_signal_threshold((uint16_t)raw) == value,
                  "vl53l4 tuning %s decode: got %u", name,
                  depz_vl53l4_decode_signal_threshold((uint16_t)raw));
        } else if (strcmp(kind, "sigma_threshold") == 0) {
            uint16_t r = 0;
            CHECK(depz_vl53l4_sigma_threshold_raw((uint16_t)value, &r) == 0 &&
                  (int64_t)r == raw,
                  "vl53l4 tuning %s raw: got %u", name, r);
            CHECK((int64_t)depz_vl53l4_decode_sigma_threshold((uint16_t)raw) == value,
                  "vl53l4 tuning %s decode: got %u", name,
                  depz_vl53l4_decode_sigma_threshold((uint16_t)raw));
        } else {
            CHECK(0, "unknown vl53l4 tuning kind %s", kind);
        }
        seen_case(c, f0);
    }

    /* xtalk_decode: C round(), an exact .5 goes away from zero (256 -> 1) */
    const json_value *xd = json_obj_get(root, "xtalk_decode");
    CHECK(json_arr_size(xd) > 0, "vl53l4 xtalk_decode missing");
    for (size_t i = 0; i < json_arr_size(xd); i++) {
        const json_value *c = json_arr_get(xd, i);
        const int f0 = g_fail;
        uint16_t raw = (uint16_t)json_as_int(json_obj_get(c, "raw"));
        CHECK((int64_t)depz_vl53l4_decode_xtalk(raw) == json_as_int(json_obj_get(c, "kcps")),
              "vl53l4 xtalk_decode %u: got %u", raw, depz_vl53l4_decode_xtalk(raw));
        seen_case(c, f0);
    }

    /* config_block: the 91-byte init block with byte 0 forced to 0x12 */
    const json_value *cb = json_obj_get(root, "config_block");
    const int cb_f0 = g_fail;
    CHECK((int64_t)DEPZ_VL53L4_CONFIG_ADDR == json_as_int(json_obj_get(cb, "addr")),
          "vl53l4 config_block addr");
    {
        uint8_t out[91];
        size_t n = depz_vl53l4_config_block(out);
        CHECK(n == sizeof(out), "vl53l4 config_block length: got %zu", n);
        check_payload("config_block", out, n, json_as_str(json_obj_get(cb, "data")));
        CHECK(out[0] == DEPZ_VL53L4_CONFIG_FMP_BYTE, "vl53l4 config_block fmp byte");
    }
    seen_id("vl53l4/config_block", cb_f0);

    json_free(root);
}

/* ======================================================================== */
/* bno086_shtp.json                                                          */
/* ======================================================================== */
static void test_bno086_shtp(void)
{
    json_value *root = load_vectors("bno086_shtp");
    if (!root) { g_fail++; return; }

    /* header pack/unpack */
    const json_value *hdr = json_obj_get(root, "header");
    for (size_t i = 0; i < json_arr_size(hdr); i++) {
        const json_value *c = json_arr_get(hdr, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        depz_shtp_header h = {
            (uint16_t)json_as_int(json_obj_get(c, "length")),
            (uint8_t)json_as_int(json_obj_get(c, "channel")),
            (uint8_t)json_as_int(json_obj_get(c, "seq")),
            json_as_bool(json_obj_get(c, "continuation")),
        };
        uint8_t out[4];
        depz_shtp_pack_header(&h, out);
        check_payload(name, out, 4, json_as_str(json_obj_get(c, "bytes")));

        uint8_t *raw; (void)hex_decode(json_as_str(json_obj_get(c, "bytes")), &raw);
        depz_shtp_header u;
        depz_shtp_unpack_header(raw, &u);
        CHECK(u.length == h.length && u.channel == h.channel && u.seq == h.seq &&
              u.continuation == h.continuation, "shtp header unpack %s", name);
        free(raw);
        seen_case(c, f0);
    }

    /* tx_seq: shared layer, per-channel counters */
    {
        depz_shtp_layer layer;
        depz_shtp_init(&layer);
        const json_value *tx = json_obj_get(root, "tx_seq");
        for (size_t i = 0; i < json_arr_size(tx); i++) {
            const json_value *c = json_arr_get(tx, i);
            const int f0 = g_fail;
            uint8_t ch = (uint8_t)json_as_int(json_obj_get(c, "channel"));
            uint8_t *pl; size_t pn = hex_decode(json_as_str(json_obj_get(c, "payload")), &pl);
            uint8_t out[DEPZ_SHTP_MAX_TX_FRAME];
            size_t on = depz_shtp_next_frame(&layer, ch, pl, pn, out, sizeof(out));
            char label[32]; snprintf(label, sizeof(label), "tx_seq[%zu]", i);
            check_payload(label, out, on, json_as_str(json_obj_get(c, "frame")));
            free(pl);
            seen_case(c, f0);
        }
        depz_shtp_free(&layer);
    }

    /* reassembly */
    const json_value *reasm = json_obj_get(root, "reassembly");
    for (size_t i = 0; i < json_arr_size(reasm); i++) {
        const json_value *c = json_arr_get(reasm, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        depz_shtp_layer layer;
        depz_shtp_init(&layer);
        const json_value *frames = json_obj_get(c, "frames");
        const json_value *captures = json_obj_get(c, "captures_us");
        const json_value *expect = json_obj_get(c, "expect");
        const json_value *cargos = json_obj_get(expect, "cargos");
        size_t got_cargo = 0;
        CHECK(json_arr_size(captures) == json_arr_size(frames), "shtp %s: captures_us per frame", name);
        for (size_t j = 0; j < json_arr_size(frames); j++) {
            uint8_t *fr; size_t fn = hex_decode(json_as_str(json_arr_get(frames, j)), &fr);
            depz_shtp_cargo cargo;
            int rc = depz_shtp_feed_ex(&layer, fr, fn, (uint64_t)json_as_int(json_arr_get(captures, j)), &cargo);
            if (rc == 1) {
                const json_value *ec = json_arr_get(cargos, got_cargo);
                CHECK(ec != NULL, "shtp %s: unexpected extra cargo", name);
                if (ec) {
                    CHECK(cargo.channel == json_as_int(json_obj_get(ec, "channel")) &&
                          cargo.seq == json_as_int(json_obj_get(ec, "seq")),
                          "shtp %s cargo %zu header", name, got_cargo);
                    CHECK((int64_t)cargo.capture_us == json_as_int(json_obj_get(ec, "capture_us")),
                          "shtp %s cargo %zu capture_us: got %llu", name, got_cargo,
                          (unsigned long long)cargo.capture_us);
                    char buf[8192];
                    hex_encode(cargo.payload, cargo.payload_len, buf);
                    CHECK(strcmp(buf, json_as_str(json_obj_get(ec, "payload"))) == 0,
                          "shtp %s cargo %zu payload", name, got_cargo);
                }
                got_cargo++;
            }
            free(fr);
        }
        CHECK(got_cargo == json_arr_size(cargos), "shtp %s cargo count: got %zu want %zu",
              name, got_cargo, json_arr_size(cargos));
        CHECK((int64_t)layer.discarded == json_as_int(json_obj_get(expect, "discarded")),
              "shtp %s discarded: got %llu", name, (unsigned long long)layer.discarded);
        depz_shtp_free(&layer);
        seen_case(c, f0);
    }

    /* control_encode */
    const json_value *ctl = json_obj_get(root, "control_encode");
    for (size_t i = 0; i < json_arr_size(ctl); i++) {
        const json_value *c = json_arr_get(ctl, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        uint8_t out[64]; size_t on = 0;
        if (strcmp(kind, "set_feature") == 0) {
            on = depz_bno_pack_set_feature(
                (uint8_t)json_as_int(json_obj_get(c, "sensor_id")),
                (uint8_t)json_as_int(json_obj_get(c, "flags")),
                (uint16_t)json_as_int(json_obj_get(c, "sensitivity")),
                (uint32_t)json_as_int(json_obj_get(c, "interval_us")),
                (uint32_t)json_as_int(json_obj_get(c, "batch_us")),
                (uint32_t)json_as_int(json_obj_get(c, "cfg_word")), out);
        } else if (strcmp(kind, "get_feature_request") == 0) {
            on = depz_bno_pack_get_feature_request(
                (uint8_t)json_as_int(json_obj_get(c, "sensor_id")), out);
        } else if (strcmp(kind, "product_id_request") == 0) {
            on = depz_bno_pack_product_id_request(out);
        } else if (strcmp(kind, "command_request") == 0) {
            uint8_t *pr; size_t pn = hex_decode(json_as_str(json_obj_get(c, "params")), &pr);
            on = depz_bno_pack_command_request(
                (uint8_t)json_as_int(json_obj_get(c, "seq")),
                (uint8_t)json_as_int(json_obj_get(c, "command")), pr, pn, out);
            free(pr);
        } else if (strcmp(kind, "frs_read_request") == 0) {
            on = depz_bno_pack_frs_read_request(
                (uint16_t)json_as_int(json_obj_get(c, "frs_type")),
                (uint16_t)json_as_int(json_obj_get(c, "offset_words")),
                (uint16_t)json_as_int(json_obj_get(c, "block_words")), out);
        } else if (strcmp(kind, "frs_write_request") == 0) {
            on = depz_bno_pack_frs_write_request(
                (uint16_t)json_as_int(json_obj_get(c, "frs_type")),
                (uint16_t)json_as_int(json_obj_get(c, "length_words")), out);
        } else if (strcmp(kind, "frs_write_data") == 0) {
            const json_value *words = json_obj_get(c, "words");
            uint32_t w[8]; size_t wn = json_arr_size(words);
            for (size_t k = 0; k < wn && k < 8; k++)
                w[k] = (uint32_t)json_as_int(json_arr_get(words, k));
            on = depz_bno_pack_frs_write_data(
                (uint16_t)json_as_int(json_obj_get(c, "offset_words")), w, wn, out, sizeof(out));
        } else { CHECK(0, "unknown control_encode kind %s", kind); continue; }
        check_payload(name, out, on, json_as_str(json_obj_get(c, "payload")));
        seen_case(c, f0);
    }

    /* interactive_and_flush: Interactive Calibration (command 0x0E, P0 =
     * motion intent) and Force Sensor Flush (0xF0 + sensor id) */
    const json_value *iaf = json_obj_get(root, "interactive_and_flush");
    CHECK(iaf != NULL, "bno086_shtp interactive_and_flush present");
    for (size_t i = 0; i < json_arr_size(iaf); i++) {
        const json_value *c = json_arr_get(iaf, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        uint8_t out[12]; size_t on = 0;
        if (strcmp(kind, "command_request") == 0) {
            uint8_t *pr; size_t pn = hex_decode(json_as_str(json_obj_get(c, "params")), &pr);
            CHECK(json_as_int(json_obj_get(c, "command")) == DEPZ_SH2_CMD_INTERACTIVE_CALIBRATION,
                  "%s command id", name);
            CHECK(pn == 1, "%s: one parameter byte, got %zu", name, pn);
            on = depz_bno_pack_interactive_calibration((uint8_t)json_as_int(json_obj_get(c, "seq")),
                                                       pn ? pr[0] : 0, out);
            free(pr);
        } else if (strcmp(kind, "force_flush") == 0) {
            on = depz_bno_pack_force_flush((uint8_t)json_as_int(json_obj_get(c, "sensor_id")), out);
        } else { CHECK(0, "unknown interactive_and_flush kind %s", kind); continue; }
        check_payload(name, out, on, json_as_str(json_obj_get(c, "payload")));
        seen_case(c, f0);
    }

    /* executable: sleep / power_on (contract 05 §6a) — fresh layer, seq 0, 1 */
    {
        depz_shtp_layer layer;
        depz_shtp_init(&layer);
        const json_value *ex = json_obj_get(root, "executable");
        CHECK(json_arr_size(ex) == 2, "executable: want 2 cases");
        for (size_t i = 0; i < json_arr_size(ex); i++) {
            const json_value *c = json_arr_get(ex, i);
            const int f0 = g_fail;
            const char *name = json_as_str(json_obj_get(c, "name"));
            uint8_t command;
            if (strcmp(name, "sleep") == 0) command = DEPZ_SHTP_EXEC_SLEEP;
            else if (strcmp(name, "power_on") == 0) command = DEPZ_SHTP_EXEC_ON;
            else { CHECK(0, "unknown executable command %s", name); continue; }
            CHECK(command == json_as_int(json_obj_get(c, "command")), "executable %s command", name);
            CHECK(json_as_int(json_obj_get(c, "channel")) == DEPZ_SHTP_CH_EXECUTABLE,
                  "executable %s channel", name);
            check_payload(name, &command, 1, json_as_str(json_obj_get(c, "payload")));
            uint8_t frame[DEPZ_SHTP_MAX_TX_FRAME];
            size_t fn = depz_shtp_next_frame(&layer, DEPZ_SHTP_CH_EXECUTABLE, &command, 1,
                                             frame, sizeof(frame));
            check_payload(name, frame, fn, json_as_str(json_obj_get(c, "shtp_frame")));
            CHECK(json_as_int(json_obj_get(c, "bridge_cmd")) == DEPZ_BNO086_CMD_SEND_SHTP_PACKET,
                  "executable %s bridge cmd", name);
            uint8_t bridge[64]; size_t bn = 0;
            int rc = depz_build_packet(DEPZ_BNO086_CMD_SEND_SHTP_PACKET, frame, fn,
                                       (unsigned int)json_as_int(json_obj_get(c, "bridge_seq")),
                                       (depz_crc_type)json_as_int(json_obj_get(c, "crc_type")),
                                       bridge, sizeof(bridge), &bn);
            CHECK(rc == 0, "executable %s build_packet rc=%d", name, rc);
            check_payload(name, bridge, bn, json_as_str(json_obj_get(c, "bridge_frame")));
            seen_case(c, f0);
        }
        depz_shtp_free(&layer);
    }

    /* Set Feature flag bits */
    {
        const json_value *ff = json_obj_get(root, "feature_flags");
        const int f0 = g_fail;
        CHECK(ff && ff->count == 2, "feature_flags: want wakeup + always_on");
        CHECK(json_as_int(json_obj_get(ff, "wakeup")) == DEPZ_BNO_FEATURE_WAKEUP, "feature flag wakeup");
        CHECK(json_as_int(json_obj_get(ff, "always_on")) == DEPZ_BNO_FEATURE_ALWAYS_ON,
              "feature flag always_on");
        seen_id("bno086_shtp/feature_flags", f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* bno086_reports.json                                                       */
/* ======================================================================== */
static void check_report_fields(const char *name, const depz_bno_report *r,
                                const json_value *f, bool with_header)
{
    CHECK(r->sensor_id == json_as_int(json_obj_get(f, "sensor_id")),
          "%s sensor_id: got %u", name, r->sensor_id);
    CHECK(r->timestamp_us == json_as_int(json_obj_get(f, "timestamp_us")),
          "%s timestamp: got %lld", name, (long long)r->timestamp_us);
    if (with_header) {
        if (json_obj_get(f, "seq"))
            CHECK(r->seq == json_as_int(json_obj_get(f, "seq")), "%s seq", name);
        if (json_obj_get(f, "accuracy"))
            CHECK(r->accuracy == json_as_int(json_obj_get(f, "accuracy")), "%s accuracy", name);
        if (json_obj_get(f, "delay_us"))
            CHECK(r->delay_us == json_as_int(json_obj_get(f, "delay_us")), "%s delay_us", name);
    }
#define FI(field, member) do { const json_value *_v = json_obj_get(f, field); \
    if (_v) CHECK((int64_t)(r->member) == json_as_int(_v), "%s " field ": got %lld", \
                  name, (long long)(int64_t)(r->member)); } while (0)
    FI("x_raw", x_raw); FI("y_raw", y_raw); FI("z_raw", z_raw);
    FI("bias_x_raw", bias_x_raw); FI("bias_y_raw", bias_y_raw); FI("bias_z_raw", bias_z_raw);
    FI("i_raw", i_raw); FI("j_raw", j_raw); FI("k_raw", k_raw); FI("real_raw", real_raw);
    FI("value_raw", value_raw); FI("flags", flags);
    FI("latency_us", latency_us); FI("steps", steps);
    FI("motion", motion); FI("classification", classification);
    FI("page_number", page_number); FI("most_likely_state", most_likely_state);
    FI("sensor_timestamp_us", sensor_timestamp_us); FI("temperature_raw", temperature_raw);
    FI("vx_raw", vx_raw); FI("vy_raw", vy_raw); FI("vz_raw", vz_raw);
    FI("intent", intent); FI("request", request); FI("flushed_sensor", flushed_sensor);
#undef FI
    const json_value *acc = json_obj_get(f, "accuracy_raw");
    if (acc) {
        if (json_is_null(acc))
            CHECK(!r->has_accuracy_raw, "%s accuracy_raw should be null", name);
        else
            CHECK(r->has_accuracy_raw && (int64_t)r->accuracy_raw == json_as_int(acc),
                  "%s accuracy_raw: got %d", name, r->accuracy_raw);
    }
    const json_value *eos = json_obj_get(f, "end_of_sequence");
    if (eos)
        CHECK((int)r->end_of_sequence == (int)json_as_int(eos), "%s end_of_sequence", name);
    const json_value *conf = json_obj_get(f, "confidences");
    if (conf) {
        for (size_t k = 0; k < json_arr_size(conf) && k < 10; k++)
            CHECK(r->confidences[k] == json_as_int(json_arr_get(conf, k)),
                  "%s confidence[%zu]", name, k);
    }
    const json_value *data = json_obj_get(f, "data");
    if (data) {
        char buf[64];
        hex_encode(r->data, r->data_len, buf);
        CHECK(strcmp(buf, json_as_str(data)) == 0, "%s data: got %s", name, buf);
    }
}

static double json_num(const json_value *v)
{
    return v && v->kind == JSON_DOUBLE ? v->d : (double)json_as_int(v);
}

static void check_doubles(const char *name, const char *key, const json_value *want,
                          const double *got, size_t n)
{
    CHECK(json_arr_size(want) == n, "%s record %s: %zu values", name, key, json_arr_size(want));
    for (size_t k = 0; k < n && k < json_arr_size(want); k++)
        CHECK(got[k] == json_num(json_arr_get(want, k)), "%s record %s[%zu]: got %.17g want %.17g",
              name, key, k, got[k], json_num(json_arr_get(want, k)));
}

/* A vector JSON value against an SDK depz_json tree: same shape, same keys,
 * same values (reals exact: the vectors hold binary-exact numbers). */
static bool json_matches(const json_value *want, const depz_json *got, const char *name, const char *where)
{
    bool ok = true;
    switch (want ? want->kind : JSON_NULL) {
    case JSON_NULL: ok = depz_json_kind_of(got) == DEPZ_JSON_NULL; break;
    case JSON_BOOL: ok = depz_json_kind_of(got) == DEPZ_JSON_BOOL && depz_json_bool_value(got) == want->b; break;
    case JSON_INT:
        ok = depz_json_kind_of(got) == DEPZ_JSON_INT && depz_json_int_value(got) == want->i;
        break;
    case JSON_DOUBLE:
        ok = depz_json_kind_of(got) == DEPZ_JSON_DOUBLE && depz_json_double_value(got) == want->d;
        break;
    case JSON_STRING:
        ok = depz_json_kind_of(got) == DEPZ_JSON_STRING && depz_json_string_len(got) == want->slen &&
             memcmp(depz_json_string_value(got), want->s, want->slen) == 0;
        break;
    case JSON_ARRAY:
        ok = depz_json_kind_of(got) == DEPZ_JSON_ARRAY && depz_json_len(got) == want->count;
        for (size_t i = 0; ok && i < want->count; i++) {
            char sub[128];
            snprintf(sub, sizeof sub, "%s[%zu]", where, i);
            ok = json_matches(want->items[i], depz_json_at(got, i), name, sub);
        }
        return ok;
    case JSON_OBJECT:
        ok = depz_json_kind_of(got) == DEPZ_JSON_OBJECT && depz_json_len(got) == want->count;
        CHECK(ok, "%s %s: object of %zu keys, got kind %d with %zu", name, where, want->count,
              (int)depz_json_kind_of(got), depz_json_len(got));
        for (size_t i = 0; ok && i < want->count; i++) {
            char sub[128];
            snprintf(sub, sizeof sub, "%s.%s", where, want->keys[i]);
            const depz_json *g = depz_json_get(got, want->keys[i]);
            CHECK(g != NULL, "%s %s: missing", name, sub);
            ok = g && json_matches(want->items[i], g, name, sub);
        }
        return ok;
    }
    CHECK(ok, "%s %s: value differs", name, where);
    return ok;
}

/* `record`: the contract 09 dataset value, i.e. the report in SI units
 * (raw / 2^Q, exact in binary). Its integer keys repeat `fields` (checked by
 * check_report_fields); here the scaled ones go through the SDK's scalers,
 * and the whole value through depz_dataset_bno086_record(). */
static void check_report_record(const char *name, const depz_bno_report *r, const json_value *rec)
{
    double v[4], acc = 0;
    CHECK(rec != NULL, "%s: no record", name);
    if (!rec) return;
    {
        depz_json *got = depz_dataset_bno086_record(r);
        CHECK(got != NULL, "%s: dataset record built", name);
        if (got) json_matches(rec, got, name, "record");
        depz_json_free(got);
    }
    CHECK(strcmp(depz_bno_report_type_str(r->type), json_as_str(json_obj_get(rec, "type"))) == 0,
          "%s record type", name);
    if (json_obj_get(rec, "x")) {
        static const char *const XYZ[3] = {"x", "y", "z"};
        depz_bno_report_xyz(r, v);
        for (int k = 0; k < 3; k++)
            CHECK(v[k] == json_num(json_obj_get(rec, XYZ[k])), "%s record %s: got %.17g", name, XYZ[k], v[k]);
    }
    if (json_obj_get(rec, "bias")) {
        depz_bno_report_bias(r, v);
        check_doubles(name, "bias", json_obj_get(rec, "bias"), v, 3);
    }
    if (json_obj_get(rec, "i")) {
        static const char *const Q[4] = {"i", "j", "k", "real"};
        depz_bno_report_quaternion(r, v);
        for (int k = 0; k < 4; k++)
            CHECK(v[k] == json_num(json_obj_get(rec, Q[k])), "%s record %s: got %.17g", name, Q[k], v[k]);
        if (r->type != DEPZ_BNO_GYRO_INTEGRATED_RV) {
            /* the game variants carry no heading accuracy: the key is absent */
            const json_value *want = json_obj_get(rec, "accuracy_rad");
            bool has = depz_bno_report_accuracy_rad(r, &acc);
            CHECK(has == (want != NULL), "%s record accuracy_rad presence", name);
            if (has && want)
                CHECK(acc == json_num(want), "%s record accuracy_rad: got %.17g", name, acc);
        }
    }
    if (json_obj_get(rec, "value"))
        CHECK(depz_bno_report_scalar(r) == json_num(json_obj_get(rec, "value")),
              "%s record value: got %.17g", name, depz_bno_report_scalar(r));
    if (json_obj_get(rec, "angular_velocity")) {
        depz_bno_report_angular_velocity(r, v);
        check_doubles(name, "angular_velocity", json_obj_get(rec, "angular_velocity"), v, 3);
    }
}

static void test_bno086_reports(void)
{
    json_value *root = load_vectors("bno086_reports");
    if (!root) { g_fail++; return; }

    /* interactive_and_flush: MotionRequest (0x2B) and the bare 0xEF Flush
     * Completed, in the same input-cargo shape */
    static const char *const CARGO_GROUPS[] = {"input_cargos", "interactive_and_flush"};
    for (size_t g = 0; g < 2; g++) {
        const json_value *cargos = json_obj_get(root, CARGO_GROUPS[g]);
        CHECK(cargos != NULL, "bno086_reports %s present", CARGO_GROUPS[g]);
        for (size_t i = 0; i < json_arr_size(cargos); i++) {
            const json_value *c = json_arr_get(cargos, i);
            const int f0 = g_fail;
            const char *name = json_as_str(json_obj_get(c, "name"));
            uint64_t cap_ts = (uint64_t)json_as_int(json_obj_get(c, "capture_timestamp_us"));
            uint8_t *cargo; size_t cn = hex_decode(json_as_str(json_obj_get(c, "cargo")), &cargo);
            depz_bno_report reps[16];
            size_t got = depz_bno_parse_input_cargo(cargo, cn, cap_ts, reps, 16);
            const json_value *expect = json_obj_get(c, "expect");
            CHECK(got == json_arr_size(expect), "%s report count: got %zu want %zu",
                  name, got, json_arr_size(expect));
            for (size_t j = 0; j < got && j < json_arr_size(expect); j++) {
                const json_value *e = json_arr_get(expect, j);
                const char *wtype = json_as_str(json_obj_get(e, "type"));
                CHECK(strcmp(depz_bno_report_type_str(reps[j].type), wtype) == 0,
                      "%s report %zu type: got %s want %s", name, j,
                      depz_bno_report_type_str(reps[j].type), wtype);
                bool with_header = strcmp(wtype, "UnknownReport") != 0;
                check_report_fields(name, &reps[j], json_obj_get(e, "fields"), with_header);
                check_report_record(name, &reps[j], json_obj_get(e, "record"));
            }
            free(cargo);
            seen_case(c, f0);
        }
    }

    const json_value *grv = json_obj_get(root, "gyro_rv");
    for (size_t i = 0; i < json_arr_size(grv); i++) {
        const json_value *c = json_arr_get(grv, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint64_t cap_ts = (uint64_t)json_as_int(json_obj_get(c, "capture_timestamp_us"));
        uint8_t *cargo; size_t cn = hex_decode(json_as_str(json_obj_get(c, "cargo")), &cargo);
        depz_bno_report r;
        CHECK(depz_bno_parse_gyro_rv(cargo, cn, cap_ts, &r) == 0, "gyro_rv %s parse", name);
        const json_value *e = json_obj_get(c, "expect");
        CHECK(strcmp(depz_bno_report_type_str(r.type), json_as_str(json_obj_get(e, "type"))) == 0,
              "gyro_rv %s type", name);
        check_report_fields(name, &r, json_obj_get(e, "fields"), false);
        check_report_record(name, &r, json_obj_get(e, "record"));
        free(cargo);
        seen_case(c, f0);
    }

    /* Q points: the per-sensor table and the two fixed ones */
    {
        const json_value *qp = json_obj_get(root, "q_points");
        int f0 = g_fail;
        CHECK(qp && qp->count > 0, "bno086 q_points present");
        for (size_t i = 0; qp && i < qp->count; i++) {
            unsigned sid = (unsigned)strtoul(qp->keys[i], NULL, 16);
            CHECK(depz_bno_q_point((uint8_t)sid) == json_as_int(qp->items[i]),
                  "bno086 q_point(%s): got %d want %lld", qp->keys[i], depz_bno_q_point((uint8_t)sid),
                  (long long)json_as_int(qp->items[i]));
        }
        /* and nothing beyond the table: every other id is an event report */
        for (unsigned sid = 0; sid < 256; sid++) {
            char key[8];
            snprintf(key, sizeof key, "0x%02X", sid);
            if (!json_obj_get(qp, key))
                CHECK(depz_bno_q_point((uint8_t)sid) == -1, "bno086 q_point(%s) not in the table", key);
        }
        seen_id("bno086_reports/q_points", f0);
        f0 = g_fail;
        CHECK(json_as_int(json_obj_get(root, "rv_accuracy_q")) == DEPZ_BNO_RV_ACCURACY_Q,
              "bno086 rv_accuracy_q");
        seen_id("bno086_reports/rv_accuracy_q", f0);
        f0 = g_fail;
        CHECK(json_as_int(json_obj_get(root, "gyro_rv_angvel_q")) == DEPZ_BNO_GYRO_RV_ANGVEL_Q,
              "bno086 gyro_rv_angvel_q");
        seen_id("bno086_reports/gyro_rv_angvel_q", f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* vl53l8 replay: recordings/vl53l8_8x8_15hz_3s.depzrec -> expected.json     */
/* Capture is from a VL53L8CX (dev-default) device streaming the shared frame  */
/* (8x8 @ 15 Hz); it exercises the CX/CH-shared reassembler + frame decoder.  */
/* (CNH histogram output is not in this capture — see the CNH extension       */
/*  point in vl53l8_decode.c.)                                                */
/* ======================================================================== */

/* Real units (contract 04 §5), as ST GetRangingData: distance >= 0;
 * reflectance raw/2 (%), signal/ambient raw/2048 (kcps/SPAD), sigma raw/128
 * (mm) — each multiplied back is a whole wire integer in the field's range. */
static int is_whole(double v, double limit) { return v >= 0 && v <= limit && v == (double)(int64_t)v; }

static void check_real_units(const depz_vl53l8_frame *f, const char *what, size_t i)
{
    for (int z = 0; z < f->resolution; z++) {
        CHECK(f->distance_mm[z] >= 0, "%s frame %zu dist[%d] negative: %d",
              what, i, z, f->distance_mm[z]);
        CHECK(is_whole(f->reflectance[z] * 2.0, 255.0),
              "%s frame %zu reflectance[%d] not raw/2: %g", what, i, z, f->reflectance[z]);
        CHECK(is_whole(f->signal_per_spad[z] * 2048.0, 4294967295.0),
              "%s frame %zu signal[%d] not raw/2048: %g", what, i, z, f->signal_per_spad[z]);
        CHECK(is_whole(f->ambient_per_spad[z] * 2048.0, 4294967295.0),
              "%s frame %zu ambient[%d] not raw/2048: %g", what, i, z, f->ambient_per_spad[z]);
        CHECK(is_whole(f->range_sigma_mm[z] * 128.0, 65535.0),
              "%s frame %zu sigma[%d] not raw/128: %g", what, i, z, f->range_sigma_mm[z]);
    }
}

typedef struct {
    depz_vl53l8_reassembler reasm;
    depz_vl53l8_frame *frames;
    size_t n, cap;
} replay_ctx;

static void replay_cb(const depz_event *ev, void *user)
{
    replay_ctx *ctx = (replay_ctx *)user;
    if (ev->type != DEPZ_EV_PACKET || ev->cmd != DEPZ_VL53L8_RPT_FRAME ||
        ev->payload_len < 12)
        return;
    depz_vl53l8_chunk chunk;
    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0)
        return;
    const uint8_t *frame; size_t frame_len; uint64_t ts;
    if (depz_vl53l8_reasm_feed(&ctx->reasm, &chunk, &frame, &frame_len, &ts) != 1)
        return;
    depz_vl53l8_frame f;
    if (depz_vl53l8_decode_frame(frame, frame_len, ts, &f) != 0)
        return; /* corrupted frame — dropped, matching the reference */
    if (ctx->n == ctx->cap) {
        ctx->cap = ctx->cap ? ctx->cap * 2 : 64;
        ctx->frames = (depz_vl53l8_frame *)realloc(ctx->frames, ctx->cap * sizeof(*ctx->frames));
    }
    ctx->frames[ctx->n++] = f;
}

/* A hand-built 4x4 frame with known wire values: the scaling to real units
 * (contract 04 §5), as the Python test_get_ranging_data_*_and_scales. Blocks
 * are laid out in the swapped domain, then byte-reversed per 32-bit word like
 * the sensor sends them. */
static size_t synth_block(uint8_t *b, size_t i, uint32_t type, uint32_t size, uint32_t idx,
                          const uint8_t *body)
{
    uint32_t bh = type | (size << 4) | (idx << 16);
    size_t msize = (type > 0x1 && type < 0xd) ? (size_t)type * size : size;
    b[i] = (uint8_t)bh; b[i + 1] = (uint8_t)(bh >> 8);
    b[i + 2] = (uint8_t)(bh >> 16); b[i + 3] = (uint8_t)(bh >> 24);
    memcpy(b + i + 4, body, msize);
    return i + 4 + msize;
}

static void test_vl53l8_real_units(void)
{
    enum { N = 16 };
    static const int16_t dist_raw[N] = { 400 * 4 + 3, -5, -1, 0, 3, 4, 7, 8 };
    static const uint32_t rate_raw[N] = { 1, 2048, 3072, 0, 4096 };
    static const uint16_t sigma_raw[N] = { 256, 1, 129 };
    static const uint8_t refl_raw[N] = { 3, 200, 255, 0 };
    uint8_t d16[2 * N], s16[2 * N], r32[4 * N], st[N], nbt[N];
    for (int k = 0; k < N; k++) {
        d16[2 * k] = (uint8_t)dist_raw[k]; d16[2 * k + 1] = (uint8_t)((uint16_t)dist_raw[k] >> 8);
        s16[2 * k] = (uint8_t)sigma_raw[k]; s16[2 * k + 1] = (uint8_t)(sigma_raw[k] >> 8);
        for (int b = 0; b < 4; b++) r32[4 * k + b] = (uint8_t)(rate_raw[k] >> (8 * b));
        st[k] = 5;
        nbt[k] = 1;
    }
    uint8_t buf[512];
    memset(buf, 0, sizeof buf);
    buf[8] = 0x12; buf[9] = 0x34;
    size_t i = 16;
    i = synth_block(buf, i, 4, N, 0x54d0u, r32);           /* ambient   */
    i = synth_block(buf, i, 1, N, 0xdb84u, nbt);           /* nb target */
    i = synth_block(buf, i, 4, N, 0xdbc4u, r32);           /* signal    */
    i = synth_block(buf, i, 2, N, 0xdec4u, s16);           /* sigma     */
    i = synth_block(buf, i, 2, N, 0xdf44u, d16);           /* distance  */
    i = synth_block(buf, i, 1, N, 0xe044u, refl_raw);      /* reflect.  */
    i = synth_block(buf, i, 1, N, 0xe084u, st);            /* status    */
    /* a header whose block runs past the end stops the walk before the footer */
    buf[i] = 0xf0; buf[i + 1] = 0xff;
    size_t len = 400;
    buf[len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET] = 0x12;
    buf[len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET + 1] = 0x34;
    for (size_t w = 0; w < len; w += 4) {
        uint8_t t0 = buf[w], t1 = buf[w + 1];
        buf[w] = buf[w + 3]; buf[w + 1] = buf[w + 2]; buf[w + 2] = t1; buf[w + 3] = t0;
    }

    depz_vl53l8_frame f;
    CHECK(depz_vl53l8_decode_frame(buf, len, 7, &f) == 0, "synthetic frame did not decode");
    CHECK(f.resolution == N, "synthetic resolution %d", f.resolution);
    static const int32_t dist_mm[8] = { 400, 0, 0, 0, 0, 1, 1, 2 };   /* /4 floored, < 0 -> 0 */
    for (int k = 0; k < 8; k++)
        CHECK(f.distance_mm[k] == dist_mm[k], "synthetic dist[%d]: got %d want %d",
              k, f.distance_mm[k], dist_mm[k]);
    static const double rate[5] = { 1.0 / 2048, 1.0, 1.5, 0.0, 2.0 }; /* kcps/SPAD */
    for (int k = 0; k < 5; k++) {
        CHECK(f.signal_per_spad[k] == rate[k], "synthetic signal[%d]: got %g", k, f.signal_per_spad[k]);
        CHECK(f.ambient_per_spad[k] == rate[k], "synthetic ambient[%d]: got %g", k, f.ambient_per_spad[k]);
    }
    static const double sigma[3] = { 2.0, 1.0 / 128, 129.0 / 128 };  /* mm */
    for (int k = 0; k < 3; k++)
        CHECK(f.range_sigma_mm[k] == sigma[k], "synthetic sigma[%d]: got %g", k, f.range_sigma_mm[k]);
    static const double refl[4] = { 1.5, 100.0, 127.5, 0.0 };        /* % */
    for (int k = 0; k < 4; k++)
        CHECK(f.reflectance[k] == refl[k], "synthetic reflectance[%d]: got %g", k, f.reflectance[k]);
}

static void test_vl53l8_replay(void)
{
    test_vl53l8_real_units();
    char path[512];
    snprintf(path, sizeof(path), "%s/recordings/vl53l8_8x8_15hz_3s.depzrec", DEPZ_VECTORS_DIR);
    char *rec = read_file(path);
    if (!rec) { g_fail++; return; }
    const int f0 = g_fail;

    replay_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    depz_vl53l8_reasm_init(&ctx.reasm, DEPZ_VL53L8_STREAM_CHUNK_MAX);
    depz_parser parser;
    depz_parser_init(&parser);

    /* Iterate JSONL records in order; feed rx bytes; stop at the tx
     * STOP_STREAM (frames after stopRanging are not delivered). */
    char *p = rec;
    bool stop = false;
    while (*p && !stop) {
        char *nl = strchr(p, '\n');
        char *end = nl ? nl : p + strlen(p);
        char save = *end; *end = '\0';
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;
        if (*p == '{') {
            json_value *line = json_parse(p);
            const json_value *dir = line ? json_obj_get(line, "dir") : NULL;
            const json_value *data = line ? json_obj_get(line, "data") : NULL;
            if (dir && data) {
                uint8_t *bytes; size_t bn = hex_decode(json_as_str(data), &bytes);
                if (strcmp(json_as_str(dir), "rx") == 0) {
                    depz_parser_feed(&parser, bytes, bn, replay_cb, &ctx);
                } else if (strcmp(json_as_str(dir), "tx") == 0 && bn >= 5 &&
                           bytes[0] == DEPZ_MAGIC0 && bytes[4] == DEPZ_VL53L8_CMD_STOP_STREAM) {
                    stop = true;
                }
                free(bytes);
            }
            json_free(line);
        }
        *end = save;
        if (!nl) break;
        p = nl + 1;
    }
    depz_parser_free(&parser);
    free(rec);

    /* compare against expected.json */
    snprintf(path, sizeof(path), "%s/recordings/vl53l8_8x8_15hz_3s.expected.json", DEPZ_VECTORS_DIR);
    char *etext = read_file(path);
    if (!etext) { g_fail++; free(ctx.frames); return; }
    json_value *eroot = json_parse(etext);
    free(etext);
    const json_value *efr = json_obj_get(eroot, "frames");

    CHECK(ctx.n == json_arr_size(efr), "vl53l8 replay frame count: got %zu want %zu",
          ctx.n, json_arr_size(efr));

    for (size_t i = 0; i < ctx.n && i < json_arr_size(efr); i++) {
        const depz_vl53l8_frame *f = &ctx.frames[i];
        const json_value *e = json_arr_get(efr, i);
        CHECK((int64_t)f->timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")),
              "replay frame %zu timestamp: got %llu", i, (unsigned long long)f->timestamp_us);
        CHECK(f->resolution == json_as_int(json_obj_get(e, "resolution")),
              "replay frame %zu resolution: got %d", i, f->resolution);
        CHECK(f->silicon_temp_degc == json_as_int(json_obj_get(e, "silicon_temp_degc")),
              "replay frame %zu temp: got %d", i, f->silicon_temp_degc);
        const json_value *dist = json_obj_get(e, "distance_mm");
        const json_value *stat = json_obj_get(e, "target_status");
        const json_value *nbt = json_obj_get(e, "nb_target_detected");
        for (int z = 0; z < f->resolution; z++) {
            CHECK(f->distance_mm[z] == json_as_int(json_arr_get(dist, z)),
                  "replay frame %zu dist[%d]: got %d", i, z, f->distance_mm[z]);
            CHECK(f->target_status[z] == json_as_int(json_arr_get(stat, z)),
                  "replay frame %zu status[%d]: got %d", i, z, f->target_status[z]);
            CHECK(f->nb_target_detected[z] == json_as_int(json_arr_get(nbt, z)),
                  "replay frame %zu nbt[%d]: got %d", i, z, f->nb_target_detected[z]);
        }
        check_real_units(f, "vl53l8", i);
    }
    json_free(eroot);
    free(ctx.frames);
    seen_id("recordings/vl53l8_8x8_15hz_3s.depzrec", f0);
}

/* ======================================================================== */
/* dataset: recordings/dataset_dual_sr04.depzdata                            */
/* ======================================================================== */
static void test_dataset_json(void);
static void test_dataset_writer_round_trip(void);
static void test_dataset_gzip(void);
static void test_dataset_player(void);

static void test_dataset(void)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/recordings/dataset_dual_sr04.depzdata", DEPZ_VECTORS_DIR);
    char *text = read_file(path);
    if (!text) { g_fail++; return; }
    const int f0 = g_fail;

    depz_dataset *ds = depz_dataset_parse(text, strlen(text));
    CHECK(ds != NULL, "dataset parse");
    if (!ds) { free(text); return; }

    CHECK(strcmp(depz_dataset_schema(ds), "depz.dataset/1") == 0, "dataset schema: %s",
          depz_dataset_schema(ds));
    CHECK(depz_dataset_device_count(ds) == 2, "dataset device count: %zu",
          depz_dataset_device_count(ds));

    /* device d0 metadata */
    for (size_t i = 0; i < depz_dataset_device_count(ds); i++) {
        depz_dataset_device dev;
        depz_dataset_get_device(ds, i, &dev);
        CHECK(strcmp(dev.serial, "SN0042") == 0, "device %zu serial: %s", i, dev.serial);
        CHECK(strcmp(dev.sensor_type, "sr04") == 0, "device %zu sensor_type: %s", i, dev.sensor_type);
        if (strcmp(dev.id, "d0") == 0) {
            CHECK(dev.offset_us == -8524738852LL, "d0 offset_us: %lld", (long long)dev.offset_us);
            CHECK(dev.rtt_us == 13, "d0 rtt_us: %lld", (long long)dev.rtt_us);
        }
    }

    CHECK(depz_dataset_record_count(ds) == 10, "dataset record count: %zu",
          depz_dataset_record_count(ds));

    /* merged order is monotonic by t_host_us; every record is an sr04 loop/once */
    int64_t prev = 0;
    for (size_t i = 0; i < depz_dataset_record_count(ds); i++) {
        depz_dataset_record r;
        depz_dataset_get_record(ds, i, &r);
        CHECK(strcmp(r.kind, "sr04") == 0, "record %zu kind: %s", i, r.kind);
        if (i > 0) CHECK(r.t_host_us >= prev, "record %zu order", i);
        prev = r.t_host_us;
        int64_t echo = -1;
        CHECK(depz_dataset_value_int(&r, "echo_us", &echo) == 0 && echo == 5831,
              "record %zu echo_us: %lld", i, (long long)echo);
        char src[16];
        CHECK(depz_dataset_value_str(&r, "source", src, sizeof(src)) == 0 &&
              (strcmp(src, "loop") == 0 || strcmp(src, "once") == 0),
              "record %zu source: %s", i, src);
    }

    /* spot-check first record: d0 @ 8525788852, loop */
    {
        depz_dataset_record r;
        depz_dataset_get_record(ds, 0, &r);
        CHECK(strcmp(r.device_id, "d0") == 0 && r.t_host_us == 8525788852LL,
              "first record: %s @ %lld", r.device_id, (long long)r.t_host_us);
    }

    /* record for record against the file itself: the header's devices, then
     * every line (already host-time sorted) as one record, values included */
    {
        size_t line_no = 0, rec = 0;
        char *p = text;
        while (*p) {
            char *nl = strchr(p, '\n');
            if (nl) *nl = '\0';
            if (*p == '{') {
                json_value *v = json_parse(p);
                CHECK(v != NULL, "dataset line %zu parses", line_no);
                if (v && line_no == 0) {
                    const json_value *devs = json_obj_get(v, "devices");
                    CHECK(devs && devs->count == depz_dataset_device_count(ds), "dataset header devices");
                    for (size_t i = 0; devs && i < devs->count; i++) {
                        const json_value *w = devs->items[i];
                        const json_value *sync = json_obj_get(w, "time_sync");
                        depz_dataset_device dev;
                        bool found = false;
                        for (size_t k = 0; k < depz_dataset_device_count(ds); k++) {
                            depz_dataset_get_device(ds, k, &dev);
                            if (strcmp(dev.id, devs->keys[i]) == 0) { found = true; break; }
                        }
                        CHECK(found, "dataset device %s", devs->keys[i]);
                        if (!found) continue;
                        CHECK(strcmp(dev.serial, json_as_str(json_obj_get(w, "serial"))) == 0 &&
                                  strcmp(dev.sensor_type, json_as_str(json_obj_get(w, "sensor_type"))) == 0 &&
                                  strcmp(dev.software_name, json_as_str(json_obj_get(w, "software_name"))) == 0 &&
                                  dev.offset_us == json_as_int(json_obj_get(sync, "offset_us")) &&
                                  dev.rtt_us == json_as_int(json_obj_get(sync, "rtt_us")),
                              "dataset device %s metadata", devs->keys[i]);
                    }
                } else if (v) {
                    depz_dataset_record r;
                    const json_value *val = json_obj_get(v, "v");
                    int64_t echo = -1;
                    char src[16] = "";
                    CHECK(rec < depz_dataset_record_count(ds), "dataset line %zu: no record", line_no);
                    if (rec < depz_dataset_record_count(ds)) {
                        depz_dataset_get_record(ds, rec, &r);
                        CHECK(strcmp(r.device_id, json_as_str(json_obj_get(v, "d"))) == 0 &&
                                  r.t_host_us == json_as_int(json_obj_get(v, "t")) &&
                                  strcmp(r.kind, json_as_str(json_obj_get(v, "k"))) == 0,
                              "dataset record %zu: %s @ %lld %s", rec, r.device_id,
                              (long long)r.t_host_us, r.kind);
                        CHECK(val && val->count == 2 &&
                                  depz_dataset_value_int(&r, "echo_us", &echo) == 0 &&
                                  echo == json_as_int(json_obj_get(val, "echo_us")) &&
                                  depz_dataset_value_str(&r, "source", src, sizeof(src)) == 0 &&
                                  strcmp(src, json_as_str(json_obj_get(val, "source"))) == 0,
                              "dataset record %zu value", rec);
                    }
                    rec++;
                }
                json_free(v);
                line_no++;
            }
            if (!nl) break;
            p = nl + 1;
        }
        CHECK(rec == depz_dataset_record_count(ds), "dataset: %zu lines, %zu records", rec,
              depz_dataset_record_count(ds));
    }

    depz_dataset_free(ds);
    free(text);
    seen_id("recordings/dataset_dual_sr04.depzdata", f0);

    test_dataset_json();
    test_dataset_writer_round_trip();
    test_dataset_gzip();
    test_dataset_player();
}


/* ======================================================================== */
/* datasets: the JSON values, the writer, the gzip container, the player     */
/* ======================================================================== */

/* A scratch file path (the OS temp dir; unique per process and call). */
static void tmp_path(char *out, size_t cap, const char *name)
{
    static int n;
    const char *dir = getenv("TMPDIR");
#ifdef _WIN32
    if (!dir) dir = getenv("TEMP");
#endif
    if (!dir) dir = "/tmp";
    snprintf(out, cap, "%s/depz_c_vectors_%ld_%d_%s", dir, (long)time(NULL), n++, name);
}

static void test_dataset_json(void)
{
    /* build -> dump -> parse -> equal; the text is the Python form */
    depz_json *o = depz_json_object();
    depz_json *arr = depz_json_array();
    depz_json_push(arr, depz_json_int(1));
    depz_json_push(arr, depz_json_double(2.5));
    depz_json_push(arr, depz_json_double(2.0));
    depz_json_push(arr, depz_json_double(0.1));
    depz_json_push(arr, depz_json_bool(true));
    depz_json_push(arr, depz_json_null());
    depz_json_set(o, "arr", arr);
    depz_json_set(o, "s", depz_json_string("a\"b\\c\né"));
    depz_json_set(o, "big", depz_json_int(-9007199254740993LL));
    depz_json_set(o, "s", depz_json_string("replaced"));
    char *text = depz_json_dumps(o);
    CHECK(text && strcmp(text, "{\"arr\":[1,2.5,2.0,0.1,true,null],\"s\":\"replaced\",\"big\":-9007199254740993}") == 0,
          "json dump: %s", text ? text : "(null)");
    depz_json *back = text ? depz_json_parse(text, strlen(text)) : NULL;
    CHECK(back && depz_json_equal(o, back), "json round trip");
    CHECK(depz_json_dump(o, NULL, 0) == (text ? strlen(text) : 0), "json dump sizes");
    free(text);
    depz_json_free(back);
    depz_json_free(o);

    /* parse: escapes, surrogate pairs, a NUL, nesting, errors */
    const char *src = "{\"k\": [\"\\u00e9\\ud83d\\ude00\\u0000x\", {\"n\": -3e2}], \"t\": true}";
    depz_json *v = depz_json_parse(src, strlen(src));
    CHECK(v != NULL, "json parse");
    if (v) {
        const depz_json *str = depz_json_at(depz_json_get(v, "k"), 0);
        CHECK(depz_json_string_len(str) == 8 && memcmp(depz_json_string_value(str), "\xc3\xa9\xf0\x9f\x98\x80\0x", 8) == 0,
              "json string decode (len %zu)", depz_json_string_len(str));
        CHECK(depz_json_kind_of(depz_json_get(depz_json_at(depz_json_get(v, "k"), 1), "n")) == DEPZ_JSON_DOUBLE &&
                  depz_json_double_value(depz_json_get(depz_json_at(depz_json_get(v, "k"), 1), "n")) == -300.0,
              "json exponent is a real");
        CHECK(depz_json_bool_value(depz_json_get(v, "t")), "json bool");
        char *dumped = depz_json_dumps(v);
        CHECK(dumped && strstr(dumped, "\\u00e9\\ud83d\\ude00\\u0000x") != NULL, "json re-escapes: %s", dumped ? dumped : "");
        free(dumped);
    }
    depz_json_free(v);
    CHECK(depz_json_parse("{\"a\": }", 7) == NULL, "json rejects a missing value");
    CHECK(depz_json_parse("[1, 2] x", 8) == NULL, "json rejects trailing data");
    CHECK(depz_json_parse("", 0) == NULL, "json rejects empty text");
}

static depz_json *two_devices(void)
{
    depz_json *devs = depz_json_object();
    depz_json *d0 = depz_json_object(), *d1 = depz_json_object(), *ts;
    depz_json_set(d0, "sensor_type", depz_json_string("sr04"));
    ts = depz_json_object();
    depz_json_set(ts, "offset_us", depz_json_int(100));
    depz_json_set(ts, "rtt_us", depz_json_int(5));
    depz_json_set(d0, "time_sync", ts);
    depz_json_set(d1, "sensor_type", depz_json_string("vl53l8"));
    ts = depz_json_object();
    depz_json_set(ts, "offset_us", depz_json_int(-5000));
    depz_json_set(ts, "rtt_us", depz_json_int(7));
    depz_json_set(d1, "time_sync", ts);
    depz_json_set(devs, "d0", d0);
    depz_json_set(devs, "d1", d1);
    return devs;
}

typedef struct { const char *d; int64_t t; } ev_t;

/* The Python / TS player fixture writer: one `sr04` record {"n": i} per event. */
static void write_events(const char *path, const ev_t *events, size_t n)
{
    depz_json *devs = two_devices();
    depz_dataset_writer *w = depz_dataset_writer_open(path, devs, "test");
    depz_json_free(devs);
    CHECK(w != NULL, "writer opens %s", path);
    for (size_t i = 0; w && i < n; i++) {
        depz_json *v = depz_json_object();
        depz_json_set(v, "n", depz_json_int((int64_t)i));
        CHECK(depz_dataset_writer_write(w, events[i].d, events[i].t, "sr04", v) == 0, "writer writes %zu", i);
    }
    CHECK(depz_dataset_writer_records_written(w) == n, "records written");
    CHECK(depz_dataset_writer_close(w) == 0, "writer closes");
}

static void test_dataset_writer_round_trip(void)
{
    char path[512];
    tmp_path(path, sizeof path, "rt.depzdata");
    static const ev_t events[] = {{"d0", 300}, {"d0", 100}, {"d1", 200}, {"d0", 250}, {"d1", 50}};
    write_events(path, events, 5);

    /* the file: a header line then one line per record, in write order */
    char *text = read_file(path);
    CHECK(text != NULL, "written file reads");
    if (text) {
        char *nl = strchr(text, '\n');
        if (nl) *nl = '\0';
        json_value *h = json_parse(text);
        CHECK(h && strcmp(json_as_str(json_obj_get(h, "schema")), DEPZ_DATASET_SCHEMA) == 0, "header schema");
        CHECK(h && strcmp(json_as_str(json_obj_get(h, "note")), "test") == 0, "header note");
        CHECK(h && strlen(json_as_str(json_obj_get(h, "created_utc"))) == 20, "header created_utc");
        CHECK(h && json_obj_get(json_obj_get(json_obj_get(h, "devices"), "d1"), "time_sync") != NULL, "header devices");
        json_free(h);
        if (nl) CHECK(strncmp(nl + 1, "{\"d\":\"d0\",\"t\":300,\"k\":\"sr04\",\"v\":{\"n\":0}}\n", 41) == 0,
                      "first record line: %.60s", nl + 1);
        free(text);
    }

    /* read back: merged by t, stable within a device */
    depz_dataset *ds = depz_dataset_read_file(path);
    CHECK(ds != NULL, "written file parses");
    if (ds) {
        static const char *const ORDER[] = {"d1:50", "d0:100", "d1:200", "d0:250", "d0:300"};
        CHECK(strcmp(depz_dataset_schema(ds), DEPZ_DATASET_SCHEMA) == 0, "schema read back");
        CHECK(strcmp(depz_dataset_note(ds), "test") == 0, "note read back");
        CHECK(depz_dataset_record_count(ds) == 5 && depz_dataset_duration_us(ds) == 250, "count / duration");
        for (size_t i = 0; i < depz_dataset_record_count(ds); i++) {
            depz_dataset_record r;
            char got[48];
            depz_dataset_get_record(ds, i, &r);
            snprintf(got, sizeof got, "%s:%lld", r.device_id, (long long)r.t_host_us);
            CHECK(i < 5 && strcmp(got, ORDER[i]) == 0, "record %zu order: %s", i, got);
            CHECK(depz_json_kind_of(depz_json_get(r.value, "n")) == DEPZ_JSON_INT, "record %zu value", i);
        }
        depz_dataset_device dev;
        CHECK(depz_dataset_device_count(ds) == 2 && depz_dataset_get_device(ds, 1, &dev) == 0 &&
                  strcmp(dev.id, "d1") == 0 && dev.offset_us == -5000 && dev.rtt_us == 7 &&
                  strcmp(dev.sensor_type, "vl53l8") == 0,
              "devices read back");
        depz_dataset_free(ds);
    }
    remove(path);

    /* an empty dataset: a header only */
    tmp_path(path, sizeof path, "empty.depzdata");
    write_events(path, NULL, 0);
    ds = depz_dataset_read_file(path);
    CHECK(ds && depz_dataset_record_count(ds) == 0 && depz_dataset_duration_us(ds) == 0, "empty dataset");
    depz_dataset_free(ds);
    remove(path);
    CHECK(depz_dataset_read_file(path) == NULL, "a missing file is NULL");
}

static void test_dataset_gzip(void)
{
    char path[512];
    tmp_path(path, sizeof path, "big.depzdata.gz");
    /* enough records for several 64 KiB stored blocks */
    enum { N = 3000 };
    ev_t *events = (ev_t *)malloc(N * sizeof *events);
    for (size_t i = 0; i < N; i++) { events[i].d = i & 1 ? "d1" : "d0"; events[i].t = 1000000 + (int64_t)i * 1000; }
    write_events(path, events, N);
    free(events);
    char *raw = read_file(path);
    CHECK(raw && (unsigned char)raw[0] == 0x1F && (unsigned char)raw[1] == 0x8B, ".gz is a gzip file");
    free(raw);
    depz_dataset *ds = depz_dataset_read_file(path);
    CHECK(ds != NULL, "gzip dataset parses");
    if (ds) {
        depz_dataset_record r;
        CHECK(depz_dataset_record_count(ds) == N, "gzip record count %zu", depz_dataset_record_count(ds));
        depz_dataset_get_record(ds, N - 1, &r);
        CHECK(r.t_host_us == 1000000 + (N - 1) * 1000LL && depz_json_int_value(depz_json_get(r.value, "n")) == N - 1,
              "gzip last record");
        depz_dataset_free(ds);
    }
    remove(path);

    /* a corrupt trailer is refused */
    {
        tmp_path(path, sizeof path, "bad.depzdata.gz");
        static const ev_t one[] = {{"d0", 5}};
        write_events(path, one, 1);
        FILE *f = fopen(path, "rb");
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char *buf = (unsigned char *)malloc((size_t)sz);
        size_t rd = fread(buf, 1, (size_t)sz, f);
        fclose(f);
        CHECK(rd == (size_t)sz, "gzip file read whole");
        depz_dataset *whole = depz_dataset_parse((const char *)buf, rd);
        CHECK(whole != NULL, "gzip bytes parse before corruption");
        depz_dataset_free(whole);
        buf[sz - 8] ^= 1;
        CHECK(depz_dataset_parse((const char *)buf, rd) == NULL, "a corrupt gzip trailer is refused");
        free(buf);
        remove(path);
    }

    /* compressed members (Python gzip.compress): fixed and dynamic Huffman codes */
    {
        static const uint8_t fixed[] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xcb, 0x48, 0xcd, 0xc9,
                                        0xc9, 0x57, 0xc8, 0x40, 0x90, 0x5c, 0x00, 0x3b, 0x7c, 0x8a, 0xdf, 0x12, 0x00,
                                        0x00, 0x00};
        /* "hello hello hello\n" is no dataset: the reader inflates it and
         * then refuses the schema — a parse of the gzip wrapper alone */
        CHECK(depz_dataset_parse((const char *)fixed, sizeof fixed) == NULL, "a non-dataset gzip member is refused");
        static const char *const HEX =
            "1f8b08000000000002039d94bd0a023110847b1f63eb2b76926cfe5ec6c213040bc1539b70efee8995a4b8386502c3c0"
            "f07ddb64962ab3ca240fa93ac9757b2e770ddbc74b6a93f3e9723b3e17a9a6aaeb7a683f81cfe74e085d08ba9f727d6a"
            "a0cb772937d015fad4409775293fd015fbd440573f7c5066f960ccf2a6ccf266ccf25199e5a331cb2765964f14f39962"
            "3e53cc178af94231ff75f9efe9a114f500853d40710f47810f47910f4fa10f4fb18f40c18fc05d7c234f3ec53f222500"
            "226740e20c489c01993320730614ce803268c01bbba45af17e080000";
        uint8_t *gz;
        size_t n = hex_decode(HEX, &gz);
        /* 40 sr04 record lines, no header: prepend one as a stored member */
        const char *head = "{\"schema\":\"depz.dataset/2\",\"devices\":{}}\n";
        size_t hl = strlen(head);
        uint8_t *doc = (uint8_t *)malloc(hl + 23 + n);
        uint8_t sh[10] = {0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0xFF};
        memcpy(doc, sh, 10);
        doc[10] = 1; doc[11] = (uint8_t)hl; doc[12] = (uint8_t)(hl >> 8); doc[13] = (uint8_t)~hl; doc[14] = (uint8_t)(~hl >> 8);
        memcpy(doc + 15, head, hl);
        uint32_t crc = depz_crc32_iso_hdlc((const uint8_t *)head, hl);
        for (int i = 0; i < 4; i++) { doc[15 + hl + i] = (uint8_t)(crc >> (8 * i)); doc[19 + hl + i] = (uint8_t)(hl >> (8 * i)); }
        memcpy(doc + hl + 23, gz, n);
        depz_dataset *d = depz_dataset_parse((const char *)doc, hl + 23 + n);
        CHECK(d != NULL, "a dynamic-Huffman member inflates");
        if (d) {
            depz_dataset_record r;
            CHECK(depz_dataset_record_count(d) == 40, "inflated records: %zu", depz_dataset_record_count(d));
            depz_dataset_get_record(d, 39, &r);
            CHECK(r.t_host_us == 39 * 50000 && depz_json_int_value(depz_json_get(r.value, "echo_us")) == 5000 + 39 % 7,
                  "inflated last record");
            depz_dataset_free(d);
        }
        free(doc);
        free(gz);
    }
}

/* ---- player (contract 09 §Player), on a fake clock ------------------------ */

static double g_clock = 100.0;
static double fake_clock(void *user) { (void)user; return g_clock; }

typedef struct {
    depz_dataset_player *p;
    int64_t seen[64];
    size_t n;
    depz_player_state states[16];
    size_t ns;
    int64_t pause_at;   /* a callback pauses the player at this t (0 = never) */
} ptrace;

static void on_rec(const depz_dataset_record *r, void *u)
{
    ptrace *t = (ptrace *)u;
    if (t->n < 64) t->seen[t->n++] = r->t_host_us;
    if (t->pause_at && r->t_host_us == t->pause_at) depz_dataset_player_pause(t->p);
}

static void on_state(depz_player_state s, void *u)
{
    ptrace *t = (ptrace *)u;
    if (t->ns < 16) t->states[t->ns++] = s;
}

static bool nearly(double a, double b) { return a >= 0 && b >= 0 ? (a - b < 1e-6 && b - a < 1e-6) : a == b; }

static bool states_are(const ptrace *t, const depz_player_state *want, size_t n)
{
    if (t->ns != n) return false;
    for (size_t i = 0; i < n; i++) if (t->states[i] != want[i]) return false;
    return true;
}

static void player_open(ptrace *t, const depz_dataset *ds, double speed)
{
    memset(t, 0, sizeof *t);
    g_clock = 100.0;
    t->p = depz_dataset_player_new(ds, speed, fake_clock, NULL);
    CHECK(t->p != NULL, "player opens");
    depz_dataset_player_on_record(t->p, on_rec, t, NULL);
    depz_dataset_player_on_state(t->p, on_state, t, NULL);
}

static depz_dataset *evenly(char *path, size_t cap, int n, int64_t gap_us)
{
    ev_t events[16];
    tmp_path(path, cap, "even.depzdata");
    for (int i = 0; i < n && i < 16; i++) { events[i].d = "d0"; events[i].t = 1000000 + i * gap_us; }
    write_events(path, events, (size_t)n);
    depz_dataset *ds = depz_dataset_read_file(path);
    remove(path);
    return ds;
}

static void test_dataset_player(void)
{
    char path[512];
    ptrace t;
    double w;
    static const depz_player_state PLAYING = DEPZ_PLAYER_PLAYING, PAUSED = DEPZ_PLAYER_PAUSED, DONE = DEPZ_PLAYER_DONE;

    /* the TS scenario: paced, pause, resume, seek, speed 0 */
    tmp_path(path, sizeof path, "s.depzdata");
    static const ev_t sample[] = {{"d0", 1000}, {"d1", 900}, {"d0", 51000}, {"d1", 30900}};
    write_events(path, sample, 4);
    depz_dataset *ds = depz_dataset_read_file(path);
    remove(path);
    CHECK(ds && depz_dataset_duration_us(ds) == 50100, "sample duration");
    if (!ds) return;
    player_open(&t, ds, 1.0);
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_IDLE && depz_dataset_player_position_us(t.p) == 0, "idle");
    CHECK(depz_dataset_player_duration_us(t.p) == 50100, "player duration");
    depz_dataset_player_play(t.p);
    CHECK(t.n == 0, "delivery happens in tick");
    w = depz_dataset_player_tick(t.p);
    CHECK(t.n == 1 && t.seen[0] == 900, "the first record goes out at once");
    CHECK(nearly(w, 100e-6), "next due in 100 us: %g", w);
    g_clock += 100e-6 + 1e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2 && t.seen[1] == 1000, "second record");
    g_clock += 0.0299 - 2e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2, "not yet due");
    g_clock += 2e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 3 && t.seen[2] == 30900, "third record");
    CHECK(depz_dataset_player_position_us(t.p) == 30000, "position");
    depz_dataset_player_pause(t.p);
    g_clock += 1.0;
    CHECK(depz_dataset_player_tick(t.p) < 0 && t.n == 3, "paused: nothing goes out");
    depz_dataset_player_play(t.p); /* resume: the next record waits its full gap (20.1 ms) again */
    g_clock += 0.0201 - 1e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 3, "resume waits the full gap");
    g_clock += 2e-6;
    CHECK(depz_dataset_player_tick(t.p) < 0 && t.n == 4 && t.seen[3] == 51000, "last record");
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_DONE && depz_dataset_player_position_us(t.p) == 50100, "done");
    {
        const depz_player_state want[] = {PLAYING, PAUSED, PLAYING, DONE};
        CHECK(states_are(&t, want, 4), "state sequence (%zu states)", t.ns);
    }
    t.n = 0;
    depz_dataset_player_seek_us(t.p, 30000); /* -> 30900 */
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_DONE, "seek never changes the state");
    CHECK(depz_dataset_player_position_us(t.p) == 100, "position after seek: last record before the target");
    depz_dataset_player_set_speed(t.p, 0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2 && t.seen[0] == 30900 && t.seen[1] == 51000 && depz_dataset_player_state(t.p) == DEPZ_PLAYER_DONE,
          "speed 0 from the seek target");
    depz_dataset_player_free(t.p);
    depz_dataset_free(ds);

    /* speed scales the gaps */
    ds = evenly(path, sizeof path, 5, 100000);
    player_open(&t, ds, 2.0);
    depz_dataset_player_play(t.p);
    CHECK(nearly(depz_dataset_player_tick(t.p), 0.05), "2x: 50 ms gap");
    g_clock += 0.05;
    CHECK(nearly(depz_dataset_player_tick(t.p), 0.05) && t.n == 2, "2x: second gap");
    depz_dataset_player_free(t.p);

    /* a speed change mid-wait rescales the remainder */
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    g_clock += 0.05;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 1, "half of the gap played");
    depz_dataset_player_set_speed(t.p, 0.5); /* remaining 50 ms of media -> 100 ms of wall time */
    CHECK(nearly(depz_dataset_player_tick(t.p), 0.1), "remainder rescaled");
    g_clock += 0.1 - 1e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 1, "still waiting");
    g_clock += 2e-6;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2, "delivered after the rescaled wait");
    depz_dataset_player_set_speed(t.p, 4.0);
    w = depz_dataset_player_tick(t.p);
    CHECK(w > 0.0249 && w < 0.0251, "4x: 25 ms gaps (%g)", w);
    depz_dataset_player_free(t.p);

    /* speed 0 delivers everything at once; back to paced mid-play */
    player_open(&t, ds, 0.0);
    depz_dataset_player_play(t.p);
    CHECK(depz_dataset_player_tick(t.p) < 0 && t.n == 5, "speed 0: all at once");
    {
        const depz_player_state want[] = {PLAYING, DONE};
        CHECK(states_are(&t, want, 2), "speed 0 states");
    }
    depz_dataset_player_free(t.p);
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    depz_dataset_player_set_speed(t.p, 0);
    depz_dataset_player_set_speed(t.p, 1.0);
    CHECK(nearly(depz_dataset_player_tick(t.p), 0.1), "from 0 back to paced: the next record waits its gap");
    depz_dataset_player_free(t.p);

    /* a slow consumer catches up without drift */
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    g_clock += 0.35; /* records 1..3 are overdue */
    w = depz_dataset_player_tick(t.p);
    CHECK(t.n == 4 && nearly(w, 0.05), "overdue records go out back to back, record 4 stays on schedule (%g)", w);
    depz_dataset_player_free(t.p);

    /* speed validation */
    CHECK(depz_dataset_player_new(ds, -0.5, fake_clock, NULL) == NULL, "negative speed refused at open");
    CHECK(depz_dataset_player_new(ds, INFINITY, fake_clock, NULL) == NULL, "infinite speed refused at open");
    player_open(&t, ds, 1.0);
    CHECK(depz_dataset_player_set_speed(t.p, -1) == -1 && depz_dataset_player_set_speed(t.p, NAN) == -1 &&
              depz_dataset_player_speed(t.p) == 1.0,
          "bad speeds refused, speed unchanged");
    depz_dataset_player_free(t.p);

    /* seek while paused skips and waits the gap */
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    depz_dataset_player_pause(t.p);
    depz_dataset_player_seek_us(t.p, 250000); /* between records 2 (200 ms) and 3 (300 ms) */
    CHECK(depz_dataset_player_position_us(t.p) == 200000 && depz_dataset_player_state(t.p) == DEPZ_PLAYER_PAUSED,
          "seek while paused");
    depz_dataset_player_play(t.p);
    CHECK(nearly(depz_dataset_player_tick(t.p), 0.1), "gap from record 2 to record 3");
    g_clock += 0.1;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2 && t.seen[0] == 1000000 && t.seen[1] == 1300000, "records 1 and 2 were skipped");
    {
        const depz_player_state want[] = {PLAYING, PAUSED, PLAYING};
        CHECK(states_are(&t, want, 3), "seek states");
    }
    depz_dataset_player_free(t.p);

    /* seek while playing continues without state events */
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    g_clock += 0.03;
    depz_dataset_player_seek_us(t.p, 300000); /* exactly a record: it is the next one delivered */
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_PLAYING && nearly(depz_dataset_player_tick(t.p), 0.1),
          "seek while playing");
    g_clock += 0.1;
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 2 && t.seen[1] == 1300000 && t.ns == 1, "seek while playing delivered the target");
    depz_dataset_player_free(t.p);

    /* seek backwards / negative; past the end; NaN; empty */
    player_open(&t, ds, 0.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    depz_dataset_player_seek_us(t.p, -5);
    CHECK(depz_dataset_player_position_us(t.p) == 0, "negative seek -> the start");
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 10, "replayed from the start");
    depz_dataset_player_free(t.p);
    player_open(&t, ds, 1.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    depz_dataset_player_seek_us(t.p, 1e9); /* while playing -> done */
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_DONE && depz_dataset_player_position_us(t.p) == 400000,
          "seek past the end while playing");
    CHECK(depz_dataset_player_tick(t.p) < 0 && t.n == 1, "nothing more");
    {
        const depz_player_state want[] = {PLAYING, DONE};
        CHECK(states_are(&t, want, 2), "seek past end states");
    }
    depz_dataset_player_free(t.p);
    player_open(&t, ds, 0.0);
    depz_dataset_player_seek_us(t.p, 1e9); /* while idle: play() restarts from the top */
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_IDLE, "seek while idle keeps idle");
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 5, "play after a seek past the end restarts");
    CHECK(depz_dataset_player_seek_us(t.p, NAN) == -1, "NaN seek refused");
    depz_dataset_player_free(t.p);

    /* play / pause idempotent; done then play restarts; a callback controls the player */
    player_open(&t, ds, 1.0);
    depz_dataset_player_pause(t.p); /* idle: no-op */
    CHECK(depz_dataset_player_state(t.p) == DEPZ_PLAYER_IDLE, "pause while idle");
    depz_dataset_player_play(t.p);
    depz_dataset_player_play(t.p);
    depz_dataset_player_pause(t.p);
    depz_dataset_player_pause(t.p);
    {
        const depz_player_state want[] = {PLAYING, PAUSED};
        CHECK(states_are(&t, want, 2), "idempotent play / pause");
    }
    depz_dataset_player_free(t.p);
    depz_dataset_free(ds);
    ds = evenly(path, sizeof path, 2, 100000);
    player_open(&t, ds, 0.0);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    depz_dataset_player_play(t.p);
    depz_dataset_player_tick(t.p);
    CHECK(t.n == 4 && t.seen[2] == 1000000 && t.seen[3] == 1100000, "done then play restarts");
    {
        const depz_player_state want[] = {PLAYING, DONE, PLAYING, DONE};
        CHECK(states_are(&t, want, 4), "restart states");
    }
    depz_dataset_player_free(t.p);
    depz_dataset_free(ds);
    ds = evenly(path, sizeof path, 5, 100000);
    player_open(&t, ds, 0.0);
    t.pause_at = 1100000;
    depz_dataset_player_play(t.p);
    CHECK(depz_dataset_player_tick(t.p) < 0 && t.n == 2, "a callback paused the player");
    {
        const depz_player_state want[] = {PLAYING, PAUSED};
        CHECK(states_are(&t, want, 2), "callback pause states");
    }
    depz_dataset_player_free(t.p);

    /* unsubscribe is idempotent */
    {
        int tok_r = 0, tok_s = 0;
        g_clock = 100.0;
        memset(&t, 0, sizeof t);
        t.p = depz_dataset_player_new(ds, 0.0, fake_clock, NULL);
        depz_dataset_player_on_record(t.p, on_rec, &t, &tok_r);
        depz_dataset_player_on_state(t.p, on_state, &t, &tok_s);
        depz_dataset_player_off_record(t.p, tok_r);
        depz_dataset_player_off_record(t.p, tok_r);
        depz_dataset_player_off_state(t.p, tok_s);
        depz_dataset_player_off_state(t.p, tok_s);
        depz_dataset_player_play(t.p);
        depz_dataset_player_tick(t.p);
        CHECK(t.n == 0 && t.ns == 0, "unsubscribed callbacks are silent");
        depz_dataset_player_free(t.p);
    }
    depz_dataset_free(ds);

    /* an empty dataset goes straight to done */
    tmp_path(path, sizeof path, "e.depzdata");
    write_events(path, NULL, 0);
    ds = depz_dataset_read_file(path);
    remove(path);
    player_open(&t, ds, 1.0);
    depz_dataset_player_seek_us(t.p, 100); /* no-op */
    CHECK(depz_dataset_player_position_us(t.p) == 0 && depz_dataset_player_duration_us(t.p) == 0, "empty: zero");
    depz_dataset_player_play(t.p);
    {
        const depz_player_state want[] = {PLAYING, DONE};
        CHECK(states_are(&t, want, 2) && depz_dataset_player_tick(t.p) < 0 && t.n == 0, "empty -> done at once");
    }
    depz_dataset_player_free(t.p);
    depz_dataset_free(ds);

    /* the committed fixture plays in reader order */
    {
        char fx[512];
        snprintf(fx, sizeof fx, "%s/recordings/dataset_dual_sr04.depzdata", DEPZ_VECTORS_DIR);
        ds = depz_dataset_read_file(fx);
        CHECK(ds != NULL, "fixture reads from a path");
        if (ds) {
            player_open(&t, ds, 0.0);
            depz_dataset_player_play(t.p);
            depz_dataset_player_tick(t.p);
            CHECK(t.n == 10, "fixture: 10 records");
            for (size_t i = 0; i < t.n; i++) {
                depz_dataset_record r;
                depz_dataset_get_record(ds, i, &r);
                CHECK(r.t_host_us == t.seen[i], "fixture record %zu in reader order", i);
            }
            CHECK(depz_dataset_player_position_us(t.p) == depz_dataset_duration_us(ds) && depz_dataset_duration_us(ds) > 0,
                  "fixture position == duration");
            depz_dataset_player_free(t.p);
            depz_dataset_free(ds);
        }
    }
}

/* ======================================================================== */
/* vl53l8 CNH: vl53l8_cnh.json -> per-aggregate integer histograms           */
/* Byte/value-exact CNH decode against a live-VL53L8CH golden capture.       */
/* ======================================================================== */
static void test_vl53l8_cnh(void)
{
    json_value *root = load_vectors("vl53l8_cnh");
    if (!root) { g_fail++; return; }
    const int f0 = g_fail;

    const json_value *cfg_j = json_obj_get(root, "config");
    depz_vl53l8ch_cnh_config cfg = {
        .nb_of_aggregates = (int)json_as_int(json_obj_get(cfg_j, "nb_of_aggregates")),
        .feature_length   = (int)json_as_int(json_obj_get(cfg_j, "feature_length")),
    };

    uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(root, "cnh_raw")), &raw);

    depz_vl53l8ch_cnh_frame *out =
        (depz_vl53l8ch_cnh_frame *)malloc(sizeof(*out));
    CHECK(out != NULL, "cnh: alloc frame");
    if (!out) { free(raw); json_free(root); return; }

    int rc = depz_vl53l8ch_decode_cnh(&cfg, raw, n, out);
    CHECK(rc == 0, "cnh decode rc: got %d", rc);

    const json_value *exp = json_obj_get(root, "expected");
    CHECK(out->ref_residual_word ==
              (uint32_t)json_as_int(json_obj_get(exp, "ref_residual_word")),
          "cnh ref_residual_word: got %u want %lld", out->ref_residual_word,
          (long long)json_as_int(json_obj_get(exp, "ref_residual_word")));

    int want_nb = (int)json_as_int(json_obj_get(exp, "nb_aggregates"));
    CHECK(out->nb_aggregates == want_nb, "cnh nb_aggregates: got %d want %d",
          out->nb_aggregates, want_nb);

    const json_value *aggs = json_obj_get(exp, "aggregates");
    CHECK((int)json_arr_size(aggs) == want_nb, "cnh aggregate array size");

    int agg_pass = 0;
    for (int a = 0; a < out->nb_aggregates && a < (int)json_arr_size(aggs); a++) {
        const json_value *e = json_arr_get(aggs, a);
        const json_value *hr = json_obj_get(e, "hist_raw");
        const json_value *hs = json_obj_get(e, "hist_scaler");
        int ok = 1;
        ok &= ((int)json_arr_size(hr) == out->feature_length);
        ok &= ((int)json_arr_size(hs) == out->feature_length);
        for (int f = 0; f < out->feature_length; f++) {
            if (out->hist_raw[a][f] != (int32_t)json_as_int(json_arr_get(hr, f))) {
                CHECK(0, "cnh agg %d hist_raw[%d]: got %d want %lld", a, f,
                      out->hist_raw[a][f], (long long)json_as_int(json_arr_get(hr, f)));
                ok = 0;
            }
            if (out->hist_scaler[a][f] != (int8_t)json_as_int(json_arr_get(hs, f))) {
                CHECK(0, "cnh agg %d hist_scaler[%d]: got %d want %lld", a, f,
                      out->hist_scaler[a][f], (long long)json_as_int(json_arr_get(hs, f)));
                ok = 0;
            }
        }
        CHECK(ok, "cnh aggregate %d matches", a);
        if (ok) agg_pass++;
    }
    CHECK(agg_pass == want_nb, "cnh all aggregates pass: %d/%d", agg_pass, want_nb);
    seen_id("vl53l8_cnh", f0);

    free(out);
    free(raw);
    json_free(root);
}

/* ======================================================================== */
/* vl53l8.json — VL53L8 SPI bridge, the streaming side (contract 04 §2–§4)   */
/* ======================================================================== */
static void test_vl53l8(void)
{
    json_value *root = load_vectors("vl53l8");
    if (!root) { g_fail++; return; }

    /* encode: VL53_START_STREAM (frame_size u16 LE) */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "vl53l8 encode cases present");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t out[2];
        CHECK(strcmp(json_as_str(json_obj_get(c, "kind")), "start_stream") == 0, "vl53l8 %s: kind", name);
        size_t n = depz_vl53l8_pack_start_stream((uint16_t)json_as_int(json_obj_get(c, "frame_size")), out);
        check_payload(name, out, n, json_as_str(json_obj_get(c, "payload")));
        seen_case(c, f0);
    }

    /* decode: RPT_VL53_FRAME chunks as a live board emitted them */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "vl53l8 decode cases present");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *e = json_obj_get(c, "expect");
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        uint8_t *want; size_t wn = hex_decode(json_as_str(json_obj_get(e, "data")), &want);
        depz_vl53l8_chunk ch;
        CHECK(json_as_int(json_obj_get(c, "report")) == DEPZ_VL53L8_RPT_FRAME, "vl53l8 %s: report id", name);
        CHECK(depz_vl53l8_unpack_chunk(p, n, &ch) == 0, "vl53l8 %s: chunk unpack", name);
        CHECK((int64_t)ch.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")) &&
              ch.full_size == json_as_int(json_obj_get(e, "full_size")) &&
              ch.offset == json_as_int(json_obj_get(e, "offset")), "vl53l8 %s: header fields", name);
        CHECK(ch.data_len == wn && memcmp(ch.data, want, wn) == 0, "vl53l8 %s: data", name);
        free(p); free(want);
        seen_case(c, f0);
    }
    {
        uint8_t hdr[12] = {0};
        depz_vl53l8_chunk ch;
        CHECK(depz_vl53l8_unpack_chunk(hdr, 11, &ch) == -1, "vl53l8 chunk rejects 11 B");
        CHECK(depz_vl53l8_unpack_chunk(hdr, 12, &ch) == 0 && ch.data_len == 0, "vl53l8 chunk accepts 12 B");
    }

    /* reassembly: §4 rules 1–4 over those chunks, through one fresh reassembler */
    const json_value *rs = json_obj_get(root, "reassembly");
    CHECK(json_arr_size(rs) > 0, "vl53l8 reassembly cases present");
    depz_vl53l8_reassembler *ra = (depz_vl53l8_reassembler *)malloc(sizeof *ra);
    for (size_t i = 0; i < json_arr_size(rs); i++) {
        const json_value *c = json_arr_get(rs, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *chunks = json_obj_get(c, "chunks");
        const json_value *e = json_obj_get(c, "expect");
        const json_value *frames = json_obj_get(e, "frames");
        size_t done = 0;
        depz_vl53l8_reasm_init(ra, DEPZ_VL53L8_STREAM_CHUNK_MAX);
        for (size_t j = 0; j < json_arr_size(chunks); j++) {
            uint8_t *p; size_t n = hex_decode(json_as_str(json_arr_get(chunks, j)), &p);
            depz_vl53l8_chunk ch;
            const uint8_t *frame; size_t frame_len; uint64_t ts;
            CHECK(depz_vl53l8_unpack_chunk(p, n, &ch) == 0, "vl53l8 %s chunk %zu", name, j);
            if (depz_vl53l8_reasm_feed(ra, &ch, &frame, &frame_len, &ts)) {
                const json_value *w = json_arr_get(frames, done++);
                char sha[65];
                CHECK(w != NULL, "vl53l8 %s: more frames than expected", name);
                if (w) {
                    sha256_hex(frame, frame_len, sha);
                    CHECK((int64_t)ts == json_as_int(json_obj_get(w, "timestamp_us")) &&
                          (int64_t)frame_len == json_as_int(json_obj_get(w, "size")) &&
                          strcmp(sha, json_as_str(json_obj_get(w, "sha256"))) == 0,
                          "vl53l8 %s frame %zu: timestamp / size / sha256", name, done - 1);
                }
            }
            free(p);
        }
        CHECK(done == json_arr_size(frames), "vl53l8 %s: %zu frames, want %zu", name, done, json_arr_size(frames));
        CHECK((int64_t)ra->completed == json_as_int(json_obj_get(e, "completed")) &&
              (int64_t)ra->discarded == json_as_int(json_obj_get(e, "discarded")),
              "vl53l8 %s: completed %llu / discarded %llu", name,
              (unsigned long long)ra->completed, (unsigned long long)ra->discarded);
        seen_case(c, f0);
    }
    free(ra);
    json_free(root);
}

/* ======================================================================== */
/* vl53l7.json — VL53L5CX/L7CX/L7CH I2C bridge (contract 11)                  */
/* ======================================================================== */
static void test_vl53l7(void)
{
    json_value *root = load_vectors("vl53l7");
    if (!root) { g_fail++; return; }

    /* encode: READ_REG / WRITE_REG (L7 ceilings), PIN_CTRL, SET_I2C_SPEED */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "vl53l7 encode cases present");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[2 + DEPZ_VL53L7_WRITE_MAX_LEN]; size_t n = 0;
        if (strcmp(kind, "read_reg") == 0) {
            n = depz_vl53l7_pack_read_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")),
                (uint16_t)json_as_int(json_obj_get(c, "len")), out);
        } else if (strcmp(kind, "write_reg") == 0) {
            uint8_t *data; size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = depz_vl53l7_pack_write_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")), data, dn, out);
            free(data);
        } else if (strcmp(kind, "pin_ctrl") == 0) {
            n = depz_vl53l7_pack_pin_ctrl(
                (uint8_t)json_as_int(json_obj_get(c, "action")), out);
        } else if (strcmp(kind, "set_i2c_speed") == 0) {
            n = depz_vl53l7_pack_set_i2c_speed(
                (uint16_t)json_as_int(json_obj_get(c, "khz")), out);
        } else { CHECK(0, "unknown vl53l7 encode kind %s", kind); continue; }
        check_payload(name, out, n, want);
        seen_case(c, f0);
    }

    /* transfer ceilings: the L8 2048 B read must be refused on L5/L7 */
    {
        uint8_t out[2 + DEPZ_VL53L7_WRITE_MAX_LEN + 1];
        static uint8_t data[DEPZ_VL53L7_WRITE_MAX_LEN + 1];
        CHECK(depz_vl53l7_pack_read_reg(0, DEPZ_VL53L7_READ_MAX_LEN + 1, out) == 0,
              "vl53l7 read_reg rejects 1537 B");
        CHECK(depz_vl53l7_pack_read_reg(0, 0, out) == 0, "vl53l7 read_reg rejects 0 B");
        CHECK(depz_vl53l7_pack_read_reg(0xFFFF, 2, out) == 0,
              "vl53l7 read_reg rejects addr+len > 0x10000");
        CHECK(depz_vl53l7_pack_write_reg(0, data, DEPZ_VL53L7_WRITE_MAX_LEN + 1, out) == 0,
              "vl53l7 write_reg rejects 2049 B");
        CHECK(depz_vl53l7_pack_write_reg(0, data, DEPZ_VL53L7_WRITE_MAX_LEN, out) ==
                  2 + DEPZ_VL53L7_WRITE_MAX_LEN, "vl53l7 write_reg accepts 2048 B");
        CHECK(depz_vl53l7_pack_write_reg(0, data, 0, out) == 0, "vl53l7 write_reg rejects 0 B");
    }

    /* decode: RPT_VL53_INFO (0x92, 20 B, no echoed cmd byte) */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "vl53l7 decode cases present");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        CHECK(json_as_int(json_obj_get(c, "report")) == DEPZ_VL53L7_RPT_INFO,
              "vl53l7 decode %s report id", name);
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");
        depz_vl53l7_info info;
        int rc = depz_vl53l7_unpack_info(p, n, &info);
        CHECK(rc == 0, "vl53l7 info decode %s rc=%d", name, rc);
        if (rc == 0) {
#define L7_INFO_FIELD(f) \
            CHECK((int64_t)info.f == json_as_int(json_obj_get(e, #f)), \
                  "vl53l7 info %s " #f ": got %lld", name, (long long)info.f)
            L7_INFO_FIELD(int_edges);
            L7_INFO_FIELD(frames_dropped);
            L7_INFO_FIELD(i2c_errors);
            L7_INFO_FIELD(last_i2c_error);
            L7_INFO_FIELD(lpn_level);
            L7_INFO_FIELD(int_level);
            L7_INFO_FIELD(i2c_khz);
            L7_INFO_FIELD(frame_size);
#undef L7_INFO_FIELD
            CHECK(info.streaming == json_as_bool(json_obj_get(e, "streaming")),
                  "vl53l7 info %s streaming", name);
        }
        /* a short payload is rejected */
        CHECK(depz_vl53l7_unpack_info(p, DEPZ_VL53L7_INFO_SIZE - 1, &info) == -1,
              "vl53l7 info %s rejects 19 B", name);
        free(p);
        seen_case(c, f0);
    }

    /* model: PID model -> VL53L([57])(CX|CH) in the device name -> vl53l7cx */
    const json_value *model = json_obj_get(root, "model");
    CHECK(json_arr_size(model) > 0, "vl53l7 model cases present");
    for (size_t i = 0; i < json_arr_size(model); i++) {
        const json_value *c = json_arr_get(model, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *um = json_obj_get(c, "usb_model");
        const char *usb_model = json_is_null(um) ? NULL : json_as_str(um);
        const char *dev = json_as_str(json_obj_get(c, "device_name"));
        const char *got = depz_vl53l7_model_str(depz_vl53l7_resolve_model(usb_model, dev));
        const char *want = json_as_str(json_obj_get(c, "expect"));
        CHECK(strcmp(got, want) == 0, "vl53l7 model %s: got %s want %s", name, got, want);
        seen_case(c, f0);
    }
    /* regex semantics beyond the table: the first match wins even when it
     * names a part with no class (VL53L5CH -> default), a foreign PID model
     * falls through to the name, and matching is case-sensitive. */
    CHECK(depz_vl53l7_resolve_model(NULL, "x VL53L5CH y VL53L7CH") == DEPZ_VL53L7_MODEL_L7CX,
          "vl53l7 model: first match VL53L5CH -> vl53l7cx");
    CHECK(depz_vl53l7_resolve_model(NULL, "VL53L8CH VL53L5CX") == DEPZ_VL53L7_MODEL_L5CX,
          "vl53l7 model: skips non-[57] parts");
    CHECK(depz_vl53l7_resolve_model("vl53l8ch", "VL53L7CH") == DEPZ_VL53L7_MODEL_L7CH,
          "vl53l7 model: foreign PID model falls through to the name");
    CHECK(depz_vl53l7_resolve_model(NULL, "vl53l5cx") == DEPZ_VL53L7_MODEL_L7CX,
          "vl53l7 model: case-sensitive");
    CHECK(depz_vl53l7_resolve_model(NULL, NULL) == DEPZ_VL53L7_MODEL_L7CX,
          "vl53l7 model: NULL name");

    json_free(root);
}

/* ======================================================================== */
/* vl53l7 replay: recordings/vl53l5cx_* and vl53l7ch_* captures (contract 11) */
/* Live-board captures (APP_VL53L7_v0.53) replayed rx-side through framing -> */
/* reassembler -> depz_vl53l7_decode_frame (footer at size-4, per-zone trim,  */
/* CNH block) and compared field by field with the .expected.json sidecars.  */
/* The 4x4 capture pins the trim (per-target blocks arrive with 64 entries);  */
/* the CNH capture pins cnh_raw byte-exact over chunked 3156 B frames.        */
/* ======================================================================== */

static const char *const VL53L7_RECORDINGS[] = {
    "vl53l5cx_8x8_15hz_3s", "vl53l5cx_4x4_15hz",
    "vl53l7ch_8x8_15hz_3s", "vl53l7ch_cnh_8x8_15hz",
    /* VL53L8CH (APP_VL53L8_v0.92, lab board, 2026-09-25): same VL53LMZ blob,
     * decoded with depz_vl53l8ch_decode_frame — the CNH block streams inside
     * every frame. The only real CH frames the suite has. */
    "vl53l8ch_8x8_15hz_3s", "vl53l8ch_cnh_8x8_15hz",
};

typedef struct {
    depz_vl53l8_frame frame;
    uint8_t *cnh;   /* malloc'd copy, NULL when the frame carried none */
    size_t   cnh_len;
} l7_frame;

typedef struct {
    depz_vl53l8_reassembler reasm;
    l7_frame *frames;
    size_t n, cap;
    size_t parse_errors;
    size_t oversize_chunks;
    bool   l8ch;           /* a VL53L8CH capture: decode with the CH decoder */
    size_t ch_checked;     /* 8x8 VL53LMZ frames re-decoded as VL53L8CH */
    size_t ch_mismatch;    /* ...where depz_vl53l8ch_decode_frame disagreed */
    size_t cx_rejected;    /* ...that the CX (size-12) decoder refused */
    char software_name[256];
    char device_name[256];
} l7_replay_ctx;

static void l7_replay_cb(const depz_event *ev, void *user)
{
    l7_replay_ctx *ctx = (l7_replay_ctx *)user;
    if (ev->type != DEPZ_EV_PACKET)
        return;
    if (ev->cmd == DEPZ_RPT_TEXT) {
        depz_text_report t;
        if (depz_unpack_text(ev->payload, ev->payload_len, &t) != 0)
            return;
        if (t.cmd == DEPZ_CMD_GET_NAME_ACTIVE_SOFTWARE && !ctx->software_name[0])
            snprintf(ctx->software_name, sizeof(ctx->software_name), "%s", t.text);
        else if (t.cmd == DEPZ_CMD_GET_DEVICE_NAME && !ctx->device_name[0])
            snprintf(ctx->device_name, sizeof(ctx->device_name), "%s", t.text);
        return;
    }
    if (ev->cmd != DEPZ_VL53L8_RPT_FRAME || ev->payload_len < 12)
        return;
    depz_vl53l8_chunk chunk;
    if (depz_vl53l8_unpack_chunk(ev->payload, ev->payload_len, &chunk) != 0)
        return;
    if (chunk.data_len > DEPZ_VL53L7_STREAM_CHUNK_MAX)
        ctx->oversize_chunks++;
    const uint8_t *frame; size_t frame_len; uint64_t ts;
    if (depz_vl53l8_reasm_feed(&ctx->reasm, &chunk, &frame, &frame_len, &ts) != 1)
        return;
    static uint8_t cnh[DEPZ_VL53L8_STREAM_TOTAL_MAX];
    size_t cnh_len = 0;
    depz_vl53l8_frame f;
    if (ctx->l8ch) {
        depz_vl53l8_frame cx;
        ctx->ch_checked++;
        if (depz_vl53l8_decode_frame(frame, frame_len, ts, &cx) == -1)
            ctx->cx_rejected++;
        if (depz_vl53l8ch_decode_frame(frame, frame_len, ts, &f, cnh, sizeof(cnh), &cnh_len) != 0) {
            ctx->parse_errors++;
            return;
        }
    } else if (depz_vl53l7_decode_frame(frame, frame_len, ts, &f, cnh, sizeof(cnh), &cnh_len) != 0) {
        ctx->parse_errors++;
        return;
    }
    /* The L7CH runs the VL53L8CH blob (VL53LMZ): at 8x8, where the L5/L7 trim
     * is a no-op, its frames are VL53L8CH frames — pin the CH decoder on them. */
    if (strstr(ctx->software_name, "VL53L7") && f.resolution == 64 &&
        strstr(ctx->device_name, "L7CH")) {
        static uint8_t cnh2[DEPZ_VL53L8_STREAM_TOTAL_MAX];
        size_t cnh2_len = 0;
        depz_vl53l8_frame g, cx;
        ctx->ch_checked++;
        if (depz_vl53l8ch_decode_frame(frame, frame_len, ts, &g, cnh2, sizeof(cnh2), &cnh2_len) != 0 ||
            memcmp(g.distance_mm, f.distance_mm, sizeof(f.distance_mm)) != 0 ||
            memcmp(g.target_status, f.target_status, sizeof(f.target_status)) != 0 ||
            cnh2_len != cnh_len || (cnh_len && memcmp(cnh2, cnh, cnh_len) != 0))
            ctx->ch_mismatch++;
        if (depz_vl53l8_decode_frame(frame, frame_len, ts, &cx) == -1)
            ctx->cx_rejected++;
    }
    if (ctx->n == ctx->cap) {
        ctx->cap = ctx->cap ? ctx->cap * 2 : 64;
        ctx->frames = (l7_frame *)realloc(ctx->frames, ctx->cap * sizeof(*ctx->frames));
    }
    l7_frame *dst = &ctx->frames[ctx->n++];
    dst->frame = f;
    dst->cnh_len = cnh_len;
    dst->cnh = NULL;
    if (cnh_len) {
        dst->cnh = (uint8_t *)malloc(cnh_len);
        memcpy(dst->cnh, cnh, cnh_len);
    }
}

static void replay_vl53l7_recording(const char *stem)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/recordings/%s.depzrec", DEPZ_VECTORS_DIR, stem);
    char *rec = read_file(path);
    if (!rec) { g_fail++; return; }
    const int f0 = g_fail;

    l7_replay_ctx *ctx = (l7_replay_ctx *)calloc(1, sizeof(*ctx));
    ctx->l8ch = strncmp(stem, "vl53l8ch", 8) == 0;
    depz_vl53l8_reasm_init(&ctx->reasm, ctx->l8ch ? DEPZ_VL53L8_STREAM_CHUNK_MAX : DEPZ_VL53L7_STREAM_CHUNK_MAX);
    depz_parser parser;
    depz_parser_init(&parser);

    /* Iterate JSONL records in order; feed rx bytes; stop at the tx
     * STOP_STREAM (frames after stop_ranging are not delivered). */
    char *p = rec;
    bool stop = false;
    while (*p && !stop) {
        char *nl = strchr(p, '\n');
        char *end = nl ? nl : p + strlen(p);
        char save = *end; *end = '\0';
        while (*p == ' ' || *p == '\t' || *p == '\r') p++;
        if (*p == '{') {
            json_value *line = json_parse(p);
            const json_value *dir = line ? json_obj_get(line, "dir") : NULL;
            const json_value *data = line ? json_obj_get(line, "data") : NULL;
            if (dir && data) {
                uint8_t *bytes; size_t bn = hex_decode(json_as_str(data), &bytes);
                if (strcmp(json_as_str(dir), "rx") == 0) {
                    depz_parser_feed(&parser, bytes, bn, l7_replay_cb, ctx);
                } else if (strcmp(json_as_str(dir), "tx") == 0 && bn >= 5 &&
                           bytes[0] == DEPZ_MAGIC0 && bytes[4] == DEPZ_VL53L8_CMD_STOP_STREAM) {
                    stop = true;
                }
                free(bytes);
            }
            json_free(line);
        }
        *end = save;
        if (!nl) break;
        p = nl + 1;
    }
    depz_parser_free(&parser);
    free(rec);

    snprintf(path, sizeof(path), "%s/recordings/%s.expected.json", DEPZ_VECTORS_DIR, stem);
    char *etext = read_file(path);
    json_value *eroot = etext ? json_parse(etext) : NULL;
    free(etext);
    if (!eroot) { g_fail++; goto out; }

    /* identity: APP_VL53L7 -> vl53l7, class from the recorded device name */
    if (ctx->l8ch) {
        depz_identity id;
        depz_parse_software_name(ctx->software_name, &id);
        CHECK(id.sensor_type == DEPZ_SENSOR_VL53L8, "%s sensor_type", stem);
        CHECK(ctx->cx_rejected * 10 >= ctx->ch_checked * 9,
              "%s: the CX (size-12) decoder accepted %zu of %zu real VL53L8CH frames",
              stem, ctx->ch_checked - ctx->cx_rejected, ctx->ch_checked);
    } else {
        const char *want_sw = json_as_str(json_obj_get(eroot, "software_name"));
        CHECK(strcmp(ctx->software_name, want_sw) == 0, "%s software_name: got '%s'",
              stem, ctx->software_name);
        depz_identity id;
        depz_parse_software_name(ctx->software_name, &id);
        CHECK(id.sensor_type == DEPZ_SENSOR_VL53L7, "%s sensor_type: got %s", stem,
              depz_sensor_type_str(id.sensor_type) ? depz_sensor_type_str(id.sensor_type) : "null");
        const char *want_cls = json_as_str(json_obj_get(eroot, "class"));
        char cls_lower[32] = {0};
        for (size_t k = 0; want_cls && want_cls[k] && k + 1 < sizeof(cls_lower); k++)
            cls_lower[k] = (char)((want_cls[k] >= 'A' && want_cls[k] <= 'Z')
                                      ? want_cls[k] - 'A' + 'a' : want_cls[k]);
        const char *got_cls =
            depz_vl53l7_model_str(depz_vl53l7_resolve_model(NULL, ctx->device_name));
        CHECK(strcmp(got_cls, cls_lower) == 0, "%s class from '%s': got %s want %s",
              stem, ctx->device_name, got_cls, cls_lower);
    }

    CHECK(ctx->parse_errors == 0, "%s frame parse errors: %zu", stem, ctx->parse_errors);
    if (strstr(stem, "vl53l7ch")) {
        CHECK(ctx->ch_checked > 0, "%s: no 8x8 VL53LMZ frame to pin the CH decoder", stem);
        CHECK(ctx->ch_mismatch == 0, "%s: depz_vl53l8ch_decode_frame disagreed on %zu/%zu frames",
              stem, ctx->ch_mismatch, ctx->ch_checked);
        /* The CX offset is simply the wrong place: it refuses (nearly) every
         * VL53LMZ frame — an occasional one passes when the bytes at size-12
         * happen to equal the header id. */
        CHECK(ctx->cx_rejected * 10 >= ctx->ch_checked * 9,
              "%s: the CX (size-12) decoder accepted %zu of %zu VL53LMZ frames",
              stem, ctx->ch_checked - ctx->cx_rejected, ctx->ch_checked);
    }
    if (!ctx->l8ch)   /* the L8 bridge chunks at 1528 B, not the L7's 1536 */
        CHECK(ctx->oversize_chunks == 0, "%s chunks over %u B: %zu", stem,
              DEPZ_VL53L7_STREAM_CHUNK_MAX, ctx->oversize_chunks);

    const json_value *efr = json_obj_get(eroot, "frames");
    CHECK(ctx->n == json_arr_size(efr), "%s frame count: got %zu want %zu",
          stem, ctx->n, json_arr_size(efr));

    for (size_t i = 0; i < ctx->n && i < json_arr_size(efr); i++) {
        const depz_vl53l8_frame *f = &ctx->frames[i].frame;
        const json_value *e = json_arr_get(efr, i);
        CHECK((int64_t)f->timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")),
              "%s frame %zu timestamp: got %llu", stem, i, (unsigned long long)f->timestamp_us);
        CHECK(f->resolution == json_as_int(json_obj_get(e, "resolution")),
              "%s frame %zu resolution: got %d", stem, i, f->resolution);
        CHECK(f->silicon_temp_degc == json_as_int(json_obj_get(e, "silicon_temp_degc")),
              "%s frame %zu temp: got %d", stem, i, f->silicon_temp_degc);
        const json_value *dist = json_obj_get(e, "distance_mm");
        const json_value *stat = json_obj_get(e, "target_status");
        const json_value *nbt = json_obj_get(e, "nb_target_detected");
        CHECK((int)json_arr_size(dist) == f->resolution && (int)json_arr_size(stat) == f->resolution &&
              (int)json_arr_size(nbt) == f->resolution,
              "%s frame %zu array lengths vs resolution %d", stem, i, f->resolution);
        for (int z = 0; z < f->resolution; z++) {
            CHECK(f->distance_mm[z] == json_as_int(json_arr_get(dist, z)),
                  "%s frame %zu dist[%d]: got %d", stem, i, z, f->distance_mm[z]);
            CHECK(f->target_status[z] == json_as_int(json_arr_get(stat, z)),
                  "%s frame %zu status[%d]: got %d", stem, i, z, f->target_status[z]);
            CHECK(f->nb_target_detected[z] == json_as_int(json_arr_get(nbt, z)),
                  "%s frame %zu nbt[%d]: got %d", stem, i, z, f->nb_target_detected[z]);
        }
        check_real_units(f, stem, i);
        const json_value *ecnh = json_obj_get(e, "cnh_raw");
        if (ecnh && !json_is_null(ecnh)) {
            const char *want = json_as_str(ecnh);
            char *got = (char *)malloc(2 * ctx->frames[i].cnh_len + 1);
            hex_encode(ctx->frames[i].cnh ? ctx->frames[i].cnh : (const uint8_t *)"",
                       ctx->frames[i].cnh_len, got);
            CHECK(ctx->frames[i].cnh_len > 0 && strcmp(got, want) == 0,
                  "%s frame %zu cnh_raw mismatch (got %zu B, want %zu B)", stem, i,
                  ctx->frames[i].cnh_len, strlen(want) / 2);
            free(got);
        } else {
            CHECK(ctx->frames[i].cnh_len == 0, "%s frame %zu carries unexpected CNH (%zu B)",
                  stem, i, ctx->frames[i].cnh_len);
        }
    }
    json_free(eroot);
    {
        char id[256];
        snprintf(id, sizeof(id), "recordings/%s.depzrec", stem);
        seen_id(id, f0);
    }
out:
    for (size_t i = 0; i < ctx->n; i++) free(ctx->frames[i].cnh);
    free(ctx->frames);
    free(ctx);
}

/* argv[2] = one capture stem; none = all four. */
static void test_vl53l7_replay(const char *which)
{
    size_t n = sizeof(VL53L7_RECORDINGS) / sizeof(VL53L7_RECORDINGS[0]);
    bool found = false;
    for (size_t i = 0; i < n; i++) {
        if (which && strcmp(which, VL53L7_RECORDINGS[i]) != 0)
            continue;
        found = true;
        replay_vl53l7_recording(VL53L7_RECORDINGS[i]);
    }
    CHECK(found, "vl53l7_replay: unknown capture %s", which ? which : "(none)");
}

/* ======================================================================== */
/* vl53lx.json — VL53L 1D family on the APP_VL53L0_4 bridge (contract 12).   */
/* Consumes every section: encode / decode / products / model / die_block /  */
/* l0x_raw / histogram_raw / l0x_uld / l1_uld.                               */
/* ======================================================================== */
#if DEPZ_VECTORS_HAS_IO
/* l0x_uld: the VL53L0X ULD calls over the recorded register traces (the three
 * ST divergences of 2fa4be5), on a fresh driver behind a scripted bridge
 * (fake_vl53lx_trace.c): the ops must come out in the recorded order, the
 * trace must be used up, the host state / error match. */
static void test_vl53lx_l0x_uld(const json_value *cases)
{
    CHECK(json_arr_size(cases) >= 8, "vl53lx l0x_uld cases present: %zu", json_arr_size(cases));
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *error = json_as_str(json_obj_get(c, "error"));
        const json_value *trace = json_obj_get(c, "trace");
        size_t n_ops = json_arr_size(trace), issued;
        fake_trace_op *ops = (fake_trace_op *)calloc(n_ops ? n_ops : 1, sizeof *ops);
        fake_vl53lx_trace *fake;
        depz_link *link;
        depz_device *dev;
        vlx_plat plat;
        vlx_driver *d;
        vlx_l0x_host_state st;
        char mismatch[160];
        int rc;
        CHECK(ops != NULL, "vl53lx l0x_uld %s: out of memory", name);
        if (!ops) continue;
        for (size_t k = 0; k < n_ops; k++) {
            const json_value *op = json_arr_get(trace, k);
            uint8_t *data = NULL;
            size_t len = hex_decode(json_as_str(json_obj_get(op, "data")), &data);
            ops[k].write = strcmp(json_as_str(json_obj_get(op, "op")), "w") == 0;
            ops[k].addr = (uint16_t)json_as_int(json_obj_get(op, "addr"));
            CHECK(len <= FAKE_TRACE_DATA_MAX, "vl53lx l0x_uld %s op %zu: %zu B", name, k, len);
            ops[k].len = len <= FAKE_TRACE_DATA_MAX ? len : FAKE_TRACE_DATA_MAX;
            memcpy(ops[k].data, data, ops[k].len);
            free(data);
        }
        fake = fake_vl53lx_trace_start(ops, n_ops, &link);
        CHECK(fake != NULL, "vl53lx l0x_uld %s: fake start", name);
        if (!fake) { free(ops); continue; }
        rc = depz_device_open_link(link, &dev);
        CHECK(rc == DEPZ_OK, "vl53lx l0x_uld %s: open rc=%d", name, rc);
        if (rc != DEPZ_OK) { fake_vl53lx_trace_stop(fake); free(ops); continue; }
        memset(&plat, 0, sizeof plat);
        plat.dev = dev;
        plat.rb.dev = dev;
        plat.rb.cmd_read = DEPZ_VL53LX_CMD_READ_REG;
        plat.rb.cmd_write = DEPZ_VL53LX_CMD_WRITE_REG;
        plat.rb.rpt_reg_data = DEPZ_VL53LX_RPT_REG_DATA;
        plat.rb.xfer_max = DEPZ_VL53L4_XFER_MAX;
        plat.rb.timeout_ms = 2000;
        d = vlx_new_l0x(&plat, DEPZ_VL53LX_PRODUCT_L0X);
        CHECK(d != NULL, "vl53lx l0x_uld %s: driver", name);
        if (d) {
            if (strcmp(kind, "nvm_offset") == 0) {
                rc = vlx_l0x_get_info_from_device(d, (int)json_as_int(json_obj_get(c, "option")));
            } else if (strcmp(kind, "timing_budget") == 0) {
                rc = d->ops->set_range_timing(d, (int)json_as_int(json_obj_get(c, "timing_budget_ms")),
                                              (int)json_as_int(json_obj_get(c, "inter_measurement_ms")));
            } else if (strcmp(kind, "tuning") == 0) {
                uint8_t *buf = NULL;
                hex_decode(json_as_str(json_obj_get(c, "buffer")), &buf);
                rc = vlx_l0x_load_tuning_settings(d, buf);
                free(buf);
            } else {
                rc = DEPZ_E_ARG;
                CHECK(0, "unknown vl53lx l0x_uld kind %s", kind);
            }
            if (error) {
                /* A refused argument or a sensor error (Python's Vl53Error), never a
                 * bridge error — a strayed trace surfaces as one of those. */
                CHECK(rc != DEPZ_OK && !vlx_is_protocol_error(rc), "vl53lx l0x_uld %s: rc=%d, want an error",
                      name, rc);
                CHECK(rc != DEPZ_OK && strstr(depz_last_error(), error) != NULL,
                      "vl53lx l0x_uld %s: error \"%s\" lacks \"%s\"", name, depz_last_error(), error);
            } else {
                const json_value *expect = json_obj_get(c, "expect");
                CHECK(rc == DEPZ_OK, "vl53lx l0x_uld %s: rc=%d (%s)", name, rc, depz_last_error());
                vlx_l0x_host_state_get(d, &st);
                if (strcmp(kind, "nvm_offset") == 0) {
                    int64_t want = json_as_int(json_obj_get(expect, "part2part_offset_adjustment_nvm_um"));
                    CHECK(st.offset_nvm_um == want, "vl53lx l0x_uld %s: offset %lld um, want %lld", name,
                          (long long)st.offset_nvm_um, (long long)want);
                } else if (strcmp(kind, "tuning") == 0) {
                    CHECK(st.sigma_est_ref_array == json_as_int(json_obj_get(expect, "sigma_est_ref_array")) &&
                              st.sigma_est_eff_pulse_width ==
                                  json_as_int(json_obj_get(expect, "sigma_est_eff_pulse_width")) &&
                              st.sigma_est_eff_amb_width ==
                                  json_as_int(json_obj_get(expect, "sigma_est_eff_amb_width")) &&
                              st.target_ref_rate == json_as_int(json_obj_get(expect, "target_ref_rate")),
                          "vl53lx l0x_uld %s: params %d/%d/%d, target 0x%04X", name, st.sigma_est_ref_array,
                          st.sigma_est_eff_pulse_width, st.sigma_est_eff_amb_width, (unsigned)st.target_ref_rate);
                }
            }
            d->ops->destroy(d);
        }
        issued = fake_vl53lx_trace_issued(fake, mismatch, sizeof mismatch);
        CHECK(mismatch[0] == '\0', "vl53lx l0x_uld %s: %s", name, mismatch);
        CHECK(issued == n_ops, "vl53lx l0x_uld %s: %zu of %zu ops issued", name, issued, n_ops);
        depz_device_close(dev);
        fake_vl53lx_trace_stop(fake);
        free(ops);
        seen_case(c, f0);
    }
}

/* l1_uld: VL53L1X set_mode over the recorded register traces, on a fresh
 * driver behind the same scripted bridge (ERRATA A9): the ops come out in the
 * recorded order and use the trace up, and the call hands back the budget in
 * effect — one the new mode does not tabulate moved to the nearest one — or
 * fails as the case says. */
static void test_vl53lx_l1_uld(const json_value *cases)
{
    CHECK(json_arr_size(cases) >= 3, "vl53lx l1_uld cases present: %zu", json_arr_size(cases));
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *error = json_as_str(json_obj_get(c, "error"));
        const json_value *trace = json_obj_get(c, "trace");
        size_t n_ops = json_arr_size(trace), issued;
        fake_trace_op *ops = (fake_trace_op *)calloc(n_ops ? n_ops : 1, sizeof *ops);
        fake_vl53lx_trace *fake;
        depz_link *link;
        depz_device *dev;
        vlx_plat plat;
        vlx_driver *d;
        char mismatch[160];
        int rc, budget_ms = 0;
        CHECK(strcmp(json_as_str(json_obj_get(c, "kind")), "set_mode") == 0, "vl53lx l1_uld %s: kind", name);
        CHECK(ops != NULL, "vl53lx l1_uld %s: out of memory", name);
        if (!ops) continue;
        for (size_t k = 0; k < n_ops; k++) {
            const json_value *op = json_arr_get(trace, k);
            uint8_t *data = NULL;
            size_t len = hex_decode(json_as_str(json_obj_get(op, "data")), &data);
            ops[k].write = strcmp(json_as_str(json_obj_get(op, "op")), "w") == 0;
            ops[k].addr = (uint16_t)json_as_int(json_obj_get(op, "addr"));
            CHECK(len <= FAKE_TRACE_DATA_MAX, "vl53lx l1_uld %s op %zu: %zu B", name, k, len);
            ops[k].len = len <= FAKE_TRACE_DATA_MAX ? len : FAKE_TRACE_DATA_MAX;
            memcpy(ops[k].data, data, ops[k].len);
            free(data);
        }
        fake = fake_vl53lx_trace_start(ops, n_ops, &link);
        CHECK(fake != NULL, "vl53lx l1_uld %s: fake start", name);
        if (!fake) { free(ops); continue; }
        rc = depz_device_open_link(link, &dev);
        CHECK(rc == DEPZ_OK, "vl53lx l1_uld %s: open rc=%d", name, rc);
        if (rc != DEPZ_OK) { fake_vl53lx_trace_stop(fake); free(ops); continue; }
        memset(&plat, 0, sizeof plat);
        plat.dev = dev;
        plat.rb.dev = dev;
        plat.rb.cmd_read = DEPZ_VL53LX_CMD_READ_REG;
        plat.rb.cmd_write = DEPZ_VL53LX_CMD_WRITE_REG;
        plat.rb.rpt_reg_data = DEPZ_VL53LX_RPT_REG_DATA;
        plat.rb.xfer_max = DEPZ_VL53L4_XFER_MAX;
        plat.rb.timeout_ms = 2000;
        d = vlx_new_l1(&plat, DEPZ_VL53LX_PRODUCT_L1CX);
        CHECK(d != NULL, "vl53lx l1_uld %s: driver", name);
        if (d) {
            rc = d->ops->set_mode(d, json_as_str(json_obj_get(c, "mode")), &budget_ms);
            if (error) {
                CHECK(rc != DEPZ_OK && !vlx_is_protocol_error(rc), "vl53lx l1_uld %s: rc=%d, want an error",
                      name, rc);
                CHECK(rc != DEPZ_OK && strstr(depz_last_error(), error) != NULL,
                      "vl53lx l1_uld %s: error \"%s\" lacks \"%s\"", name, depz_last_error(), error);
            } else {
                int want = (int)json_as_int(json_obj_get(json_obj_get(c, "expect"), "timing_budget_ms"));
                CHECK(rc == DEPZ_OK, "vl53lx l1_uld %s: rc=%d (%s)", name, rc, depz_last_error());
                CHECK(budget_ms == want, "vl53lx l1_uld %s: budget %d ms, want %d", name, budget_ms, want);
            }
            d->ops->destroy(d);
        }
        issued = fake_vl53lx_trace_issued(fake, mismatch, sizeof mismatch);
        CHECK(mismatch[0] == '\0', "vl53lx l1_uld %s: %s", name, mismatch);
        CHECK(issued == n_ops, "vl53lx l1_uld %s: %zu of %zu ops issued", name, issued, n_ops);
        depz_device_close(dev);
        fake_vl53lx_trace_stop(fake);
        free(ops);
        seen_case(c, f0);
    }
}
#endif

static void test_vl53lx(void)
{
    json_value *root = load_vectors("vl53lx");
    if (!root) { g_fail++; return; }

    /* encode: SET_ADDR_WIDTH, START_STREAM + clear list, contract-10 codecs */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "vl53lx encode cases present");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[2 + DEPZ_VL53L4_XFER_MAX]; size_t n = 0;
        if (strcmp(kind, "set_addr_width") == 0) {
            n = depz_vl53lx_pack_set_addr_width(
                (uint8_t)json_as_int(json_obj_get(c, "width")), out);
        } else if (strcmp(kind, "read_reg") == 0) {
            n = depz_vl53l4_pack_read_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")),
                (uint16_t)json_as_int(json_obj_get(c, "len")), out);
        } else if (strcmp(kind, "write_reg") == 0) {
            uint8_t *data; size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = depz_vl53l4_pack_write_reg(
                (uint16_t)json_as_int(json_obj_get(c, "addr")), data, dn, out);
            free(data);
        } else if (strcmp(kind, "xshut") == 0) {
            n = depz_vl53l4_pack_xshut(
                (uint8_t)json_as_int(json_obj_get(c, "action")), out);
        } else if (strcmp(kind, "set_i2c_speed") == 0) {
            n = depz_vl53l4_pack_set_i2c_speed(
                (uint16_t)json_as_int(json_obj_get(c, "khz")), out);
        } else if (strcmp(kind, "start_stream") == 0) {
            const json_value *cl = json_obj_get(c, "clear");
            depz_vl53lx_clear_step steps[DEPZ_VL53LX_CLEAR_STEPS_MAX];
            size_t ns = json_arr_size(cl);
            CHECK(ns <= DEPZ_VL53LX_CLEAR_STEPS_MAX, "vl53lx %s clear list too long", name);
            if (ns > DEPZ_VL53LX_CLEAR_STEPS_MAX) continue;
            for (size_t k = 0; k < ns; k++) {
                const json_value *st = json_arr_get(cl, k);
                steps[k].addr = (uint16_t)json_as_int(json_arr_get(st, 0));
                steps[k].value = (uint8_t)json_as_int(json_arr_get(st, 1));
            }
            n = depz_vl53lx_pack_start_stream(
                (uint16_t)json_as_int(json_obj_get(c, "addr")),
                (uint16_t)json_as_int(json_obj_get(c, "len")),
                (uint8_t)json_as_int(json_obj_get(c, "flags")), steps, ns, out);
            CHECK(n == 6 + 3 * ns, "vl53lx %s length 6+3n: got %zu", name, n);
        } else { CHECK(0, "unknown vl53lx encode kind %s", kind); continue; }
        check_payload(name, out, n, want);
        seen_case(c, f0);
    }
    /* refusals: five clear steps, address width 0 / 3 */
    {
        uint8_t out[DEPZ_VL53LX_START_STREAM_MAX + 3];
        depz_vl53lx_clear_step five[5];
        for (int k = 0; k < 5; k++) { five[k].addr = 0x86; five[k].value = 1; }
        CHECK(depz_vl53lx_pack_start_stream(0x0089, 17, 0, five, 5, out) == 0,
              "vl53lx start_stream refuses five clear steps");
        CHECK(depz_vl53lx_pack_start_stream(0x0089, 17, 0, five, 4, out) ==
                  DEPZ_VL53LX_START_STREAM_MAX, "vl53lx start_stream accepts four steps");
        CHECK(depz_vl53lx_pack_set_addr_width(0, out) == 0 &&
                  depz_vl53lx_pack_set_addr_width(3, out) == 0,
              "vl53lx set_addr_width refuses widths other than 1/2");
    }

    /* decode: RPT_VL53_INFO (0x92, 23 B) */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "vl53lx decode cases present");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        CHECK(json_as_int(json_obj_get(c, "report")) == DEPZ_VL53LX_RPT_INFO,
              "vl53lx decode %s report id", name);
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");
        depz_vl53lx_info info;
        int rc = depz_vl53lx_unpack_info(p, n, &info);
        CHECK(rc == 0, "vl53lx info decode %s rc=%d", name, rc);
        if (rc == 0) {
#define LX_INFO_FIELD(f) \
            CHECK((int64_t)info.f == json_as_int(json_obj_get(e, #f)), \
                  "vl53lx info %s " #f ": got %lld", name, (long long)info.f)
            LX_INFO_FIELD(int_edges);
            LX_INFO_FIELD(slots_skipped);
            LX_INFO_FIELD(i2c_errors);
            LX_INFO_FIELD(last_i2c_error);
            LX_INFO_FIELD(xshut_level);
            LX_INFO_FIELD(int_level);
            LX_INFO_FIELD(i2c_khz);
            LX_INFO_FIELD(addr_width);
            LX_INFO_FIELD(n_clear);
            LX_INFO_FIELD(frames_dropped);
#undef LX_INFO_FIELD
            CHECK(e->count == 10, "vl53lx info %s: %zu expected fields, 10 checked",
                  name, e->count);
        }
        CHECK(depz_vl53lx_unpack_info(p, DEPZ_VL53LX_INFO_SIZE - 1, &info) == -1,
              "vl53lx info %s rejects 22 B", name);
        free(p);
        seen_case(c, f0);
    }

    /* products: the table, in order, with the default driver's bridge params */
    const json_value *prods = json_obj_get(root, "products");
    CHECK(json_arr_size(prods) == DEPZ_VL53LX_PRODUCT_COUNT,
          "vl53lx products: %zu rows, table has %d", json_arr_size(prods),
          (int)DEPZ_VL53LX_PRODUCT_COUNT);
    for (size_t i = 0; i < json_arr_size(prods); i++) {
        const json_value *r = json_arr_get(prods, i);
        const int f0 = g_fail;
        const char *pname = json_as_str(json_obj_get(r, "product"));
        const depz_vl53lx_product_info *row = depz_vl53lx_product_get((depz_vl53lx_product)i);
        CHECK(row && strcmp(row->name, pname) == 0, "vl53lx products[%zu]: got %s want %s",
              i, row ? row->name : "NULL", pname);
        if (!row) continue;
        CHECK(depz_vl53lx_product_from_str(pname) == (depz_vl53lx_product)i,
              "vl53lx product_from_str %s", pname);
        CHECK(row->model_id == json_as_int(json_obj_get(r, "model_id")),
              "vl53lx %s model_id: got 0x%04x", pname, row->model_id);
        CHECK(row->reach_mm == json_as_int(json_obj_get(r, "reach_mm")),
              "vl53lx %s reach_mm: got %u", pname, row->reach_mm);
        /* driver kinds, in DRIVER_KINDS order */
        const json_value *kinds = json_obj_get(r, "driver_kinds");
        static const depz_vl53lx_driver ORDER[3] = {
            DEPZ_VL53LX_DRIVER_ULD, DEPZ_VL53LX_DRIVER_ULP, DEPZ_VL53LX_DRIVER_HISTOGRAM };
        size_t nk = 0;
        for (int k = 0; k < 3; k++) {
            if (!(row->driver_kinds & ORDER[k])) continue;
            const char *want = json_as_str(json_arr_get(kinds, nk));
            CHECK(want && strcmp(depz_vl53lx_driver_str(ORDER[k]), want) == 0,
                  "vl53lx %s driver_kinds[%zu]: got %s", pname, nk,
                  depz_vl53lx_driver_str(ORDER[k]));
            nk++;
        }
        CHECK(nk == json_arr_size(kinds), "vl53lx %s driver_kinds count: got %zu want %zu",
              pname, nk, json_arr_size(kinds));
        CHECK(strcmp(depz_vl53lx_driver_str(row->default_driver),
                     json_as_str(json_obj_get(r, "default_driver"))) == 0 &&
                  (row->driver_kinds & row->default_driver),
              "vl53lx %s default_driver: got %s", pname,
              depz_vl53lx_driver_str(row->default_driver));
        CHECK(row->addr_width == json_as_int(json_obj_get(r, "addr_width")),
              "vl53lx %s addr_width: got %u", pname, row->addr_width);
        CHECK(row->max_khz == json_as_int(json_obj_get(r, "max_khz")),
              "vl53lx %s max_khz: got %u", pname, row->max_khz);
        const json_value *cs = json_obj_get(r, "clear_steps");
        CHECK(row->n_clear == json_arr_size(cs), "vl53lx %s clear_steps count: got %u",
              pname, row->n_clear);
        for (size_t k = 0; k < row->n_clear && k < json_arr_size(cs); k++) {
            const json_value *st = json_arr_get(cs, k);
            CHECK(row->clear[k].addr == json_as_int(json_arr_get(st, 0)) &&
                      row->clear[k].value == json_as_int(json_arr_get(st, 1)),
                  "vl53lx %s clear_steps[%zu]: got 0x%04x<-%u", pname, k,
                  row->clear[k].addr, row->clear[k].value);
        }
        seen_case(r, f0);
    }
    CHECK(depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_NONE) == NULL &&
              depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_COUNT) == NULL,
          "vl53lx product_get out of range -> NULL");

    /* model: PID model -> VL53L<part> in the device name -> generic class */
    const json_value *model = json_obj_get(root, "model");
    CHECK(json_arr_size(model) > 0, "vl53lx model cases present");
    for (size_t i = 0; i < json_arr_size(model); i++) {
        const json_value *c = json_arr_get(model, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *um = json_obj_get(c, "usb_model");
        const char *usb_model = json_is_null(um) ? NULL : json_as_str(um);
        const char *dev = json_as_str(json_obj_get(c, "device_name"));
        /* expect_class is the Python class name ("Vl53l0x"); ours is lower-case */
        const char *want_cls = json_as_str(json_obj_get(c, "expect_class"));
        char cls_lower[32] = {0};
        for (size_t k = 0; want_cls[k] && k < sizeof(cls_lower) - 1; k++)
            cls_lower[k] = (char)((want_cls[k] >= 'A' && want_cls[k] <= 'Z')
                                      ? want_cls[k] - 'A' + 'a' : want_cls[k]);
        const char *got_cls = depz_vl53lx_class_str(depz_vl53lx_resolve_class(usb_model, dev));
        CHECK(strcmp(got_cls, cls_lower) == 0, "vl53lx model %s class: got %s want %s",
              name, got_cls, cls_lower);
        const json_value *ep = json_obj_get(c, "expect_product");
        const depz_vl53lx_product_info *got_p =
            depz_vl53lx_product_get(depz_vl53lx_product_from_board_name(dev));
        if (json_is_null(ep))
            CHECK(got_p == NULL, "vl53lx model %s product: got %s want null", name, got_p->name);
        else
            CHECK(got_p && strcmp(got_p->name, json_as_str(ep)) == 0,
                  "vl53lx model %s product: got %s want %s", name,
                  got_p ? got_p->name : "null", json_as_str(ep));
        seen_case(c, f0);
    }
    /* regex semantics beyond the table: leftmost `VL53L\d` match decides,
     * greedy [A-Z0-9]* tail, a PID model outside the family falls through. */
    CHECK(depz_vl53lx_product_from_board_name("VL53LX VL53L1CX") == DEPZ_VL53LX_PRODUCT_L1CX,
          "vl53lx board name: skips VL53L without a digit");
    CHECK(depz_vl53lx_product_from_board_name("VL53L8CX VL53L1CX") == DEPZ_VL53LX_PRODUCT_NONE,
          "vl53lx board name: first match not a family product -> none");
    CHECK(depz_vl53lx_product_from_board_name("VL53L1CXUSB") == DEPZ_VL53LX_PRODUCT_NONE,
          "vl53lx board name: greedy tail");
    CHECK(depz_vl53lx_product_from_board_name(NULL) == DEPZ_VL53LX_PRODUCT_NONE,
          "vl53lx board name: NULL");
    CHECK(depz_vl53lx_resolve_class("vl53l8ch", "VL53L3CX") == DEPZ_VL53LX_CLASS_L3CX,
          "vl53lx class: foreign PID model falls through to the name");
    CHECK(depz_vl53lx_resolve_class("vl53l4cd", "VL53L1CB") == DEPZ_VL53LX_CLASS_GENERIC,
          "vl53lx class: PID model L4CD wins over the name, opens generic");

    /* die_block: the 17-byte 0x0089 block, l4 / l1 variants */
    const json_value *die = json_obj_get(root, "die_block");
    CHECK(json_arr_size(die) > 0, "vl53lx die_block cases present");
    for (size_t i = 0; i < json_arr_size(die); i++) {
        const json_value *c = json_arr_get(die, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *var = json_as_str(json_obj_get(c, "variant"));
        depz_vl53lx_die_variant v;
        if (strcmp(var, "l4") == 0) v = DEPZ_VL53LX_DIE_L4;
        else if (strcmp(var, "l1") == 0) v = DEPZ_VL53LX_DIE_L1;
        else { CHECK(0, "vl53lx die_block %s: unknown variant %s", name, var); continue; }
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);
        const json_value *e = json_obj_get(c, "expect");
        depz_vl53l4_result r;
        int rc = depz_vl53lx_decode_die_block(raw, n, v, &r);
        CHECK(rc == 0, "vl53lx die_block %s rc=%d", name, rc);
        if (rc == 0) {
#define DIE_FIELD(f) \
            CHECK(r.f == json_as_int(json_obj_get(e, #f)), \
                  "vl53lx die_block %s " #f ": got %d", name, r.f)
            DIE_FIELD(range_status);
            DIE_FIELD(distance_mm);
            DIE_FIELD(sigma_mm);
            DIE_FIELD(signal_rate_kcps);
            DIE_FIELD(ambient_rate_kcps);
            DIE_FIELD(signal_per_spad_kcps);
            DIE_FIELD(ambient_per_spad_kcps);
            DIE_FIELD(number_of_spad);
            DIE_FIELD(stream_count);
#undef DIE_FIELD
            CHECK(e->count == 9, "vl53lx die_block %s: %zu expected fields, 9 checked",
                  name, e->count);
        }
        CHECK(depz_vl53lx_decode_die_block(raw, DEPZ_VL53LX_DIE_BLOCK_LEN - 1, v, &r) == -1,
              "vl53lx die_block %s rejects 16 B", name);
        free(raw);
        seen_case(c, f0);
    }

    /* l0x_raw: raw fields of the VL53L0X 12-byte 0x14 block */
    const json_value *l0x = json_obj_get(root, "l0x_raw");
    CHECK(json_arr_size(l0x) > 0, "vl53lx l0x_raw cases present");
    for (size_t i = 0; i < json_arr_size(l0x); i++) {
        const json_value *c = json_arr_get(l0x, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);
        const json_value *e = json_obj_get(c, "expect");
        depz_vl53lx_l0x_raw r;
        int rc = depz_vl53lx_decode_l0x_raw(raw, n, &r);
        CHECK(rc == 0, "vl53lx l0x_raw %s rc=%d", name, rc);
        if (rc == 0) {
#define L0X_FIELD(f) \
            CHECK((int64_t)r.f == json_as_int(json_obj_get(e, #f)), \
                  "vl53lx l0x_raw %s " #f ": got %lld", name, (long long)r.f)
            L0X_FIELD(distance_raw);
            L0X_FIELD(device_range_status);
            L0X_FIELD(signal_rate_mcps_1616);
            L0X_FIELD(ambient_rate_mcps_1616);
            L0X_FIELD(effective_spad_count_88);
#undef L0X_FIELD
            CHECK(e->count == 5, "vl53lx l0x_raw %s: %zu expected fields, 5 checked",
                  name, e->count);
        }
        CHECK(depz_vl53lx_decode_l0x_raw(raw, DEPZ_VL53LX_L0X_BLOCK_LEN - 1, &r) == -1,
              "vl53lx l0x_raw %s rejects 11 B", name);
        free(raw);
        seen_case(c, f0);
    }

    /* histogram_raw: status bytes + 24 bins (bin-23 MSB/LSB patch) */
    const json_value *hist = json_obj_get(root, "histogram_raw");
    CHECK(json_arr_size(hist) > 0, "vl53lx histogram_raw cases present");
    for (size_t i = 0; i < json_arr_size(hist); i++) {
        const json_value *c = json_arr_get(hist, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);
        const json_value *e = json_obj_get(c, "expect");
        uint8_t before[DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN];
        if (n >= sizeof(before)) memcpy(before, raw, sizeof(before));
        depz_vl53lx_histogram_raw r;
        int rc = depz_vl53lx_decode_histogram_raw(raw, n, &r);
        CHECK(rc == 0, "vl53lx histogram_raw %s rc=%d", name, rc);
        if (rc == 0) {
#define HIST_FIELD(f) \
            CHECK((int64_t)r.f == json_as_int(json_obj_get(e, #f)), \
                  "vl53lx histogram_raw %s " #f ": got %lld", name, (long long)r.f)
            HIST_FIELD(interrupt_status);
            HIST_FIELD(range_status);
            HIST_FIELD(report_status);
            HIST_FIELD(stream_count);
            HIST_FIELD(dss_actual_effective_spads);
            HIST_FIELD(reference_phase);
            HIST_FIELD(vcsel_start);
#undef HIST_FIELD
            const json_value *bins = json_obj_get(e, "bins");
            CHECK(json_arr_size(bins) == DEPZ_VL53LX_HISTOGRAM_BINS,
                  "vl53lx histogram_raw %s bins: %zu expected", name, json_arr_size(bins));
            for (size_t b = 0; b < DEPZ_VL53LX_HISTOGRAM_BINS && b < json_arr_size(bins); b++)
                CHECK((int64_t)r.bins[b] == json_as_int(json_arr_get(bins, b)),
                      "vl53lx histogram_raw %s bin %zu: got %u want %lld", name, b,
                      r.bins[b], (long long)json_as_int(json_arr_get(bins, b)));
            CHECK(e->count == 8, "vl53lx histogram_raw %s: %zu expected fields, 8 checked",
                  name, e->count);
            CHECK(memcmp(before, raw, sizeof(before)) == 0,
                  "vl53lx histogram_raw %s: input block modified", name);
        }
        CHECK(depz_vl53lx_decode_histogram_raw(raw, DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN - 1, &r) == -1,
              "vl53lx histogram_raw %s rejects 82 B", name);
        free(raw);
        seen_case(c, f0);
    }

#if DEPZ_VECTORS_HAS_IO
    test_vl53lx_l0x_uld(json_obj_get(root, "l0x_uld"));
    test_vl53lx_l1_uld(json_obj_get(root, "l1_uld"));
#endif

    json_free(root);
}

/* ======================================================================== */
/* bno055.json — BNO055 register bridge (contract 13).                      */
/* Consumes every section: encode / decode / units / calib_stat /            */
/* calibration_profile / axis_remap / axis_remap_invalid / sensor_config /   */
/* blocks.                                                                   */
/* ======================================================================== */

/* Compare an int16 array channel against a JSON array or null. */
static void check_bno_words(const char *name, const char *ch, bool has,
                            const int16_t *got, size_t n, const json_value *want)
{
    if (json_is_null(want)) {
        CHECK(!has, "bno055 block %s %s: present, want null", name, ch);
        return;
    }
    CHECK(has, "bno055 block %s %s: absent", name, ch);
    CHECK(json_arr_size(want) == n, "bno055 block %s %s: %zu expected values",
          name, ch, json_arr_size(want));
    for (size_t k = 0; has && k < n && k < json_arr_size(want); k++)
        CHECK(got[k] == json_as_int(json_arr_get(want, k)),
              "bno055 block %s %s[%zu]: got %d want %lld", name, ch, k, got[k],
              (long long)json_as_int(json_arr_get(want, k)));
}

static void test_bno055(void)
{
    json_value *root = load_vectors("bno055");
    if (!root) { g_fail++; return; }

    /* encode: 0x32..0x37 payloads */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "bno055 encode cases present");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[1 + DEPZ_BNO055_XFER_MAX]; size_t n = 0;
        if (strcmp(kind, "read_reg") == 0) {
            n = depz_bno055_pack_read_reg(
                (uint8_t)json_as_int(json_obj_get(c, "addr")),
                (uint8_t)json_as_int(json_obj_get(c, "len")), out);
        } else if (strcmp(kind, "write_reg") == 0) {
            uint8_t *data; size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = depz_bno055_pack_write_reg(
                (uint8_t)json_as_int(json_obj_get(c, "addr")), data, dn, out);
            CHECK(n == 1 + dn, "bno055 %s length 1+n: got %zu", name, n);
            free(data);
        } else if (strcmp(kind, "start_stream") == 0) {
            n = depz_bno055_pack_start_stream(
                (uint8_t)json_as_int(json_obj_get(c, "trigger")),
                (uint8_t)json_as_int(json_obj_get(c, "addr")),
                (uint8_t)json_as_int(json_obj_get(c, "len")),
                (uint16_t)json_as_int(json_obj_get(c, "period_ms")), out);
        } else if (strcmp(kind, "reset") == 0 || strcmp(kind, "stop_stream") == 0 ||
                   strcmp(kind, "get_info") == 0) {
            n = 0; /* empty payload */
        } else { CHECK(0, "unknown bno055 encode kind %s", kind); continue; }
        check_payload(name, out, n, want);
        seen_case(c, f0);
    }
    /* refusals: write data length outside 1..128 */
    {
        uint8_t data[DEPZ_BNO055_XFER_MAX + 1] = {0};
        uint8_t out[2 + DEPZ_BNO055_XFER_MAX];
        CHECK(depz_bno055_pack_write_reg(0x3D, data, 0, out) == 0 &&
                  depz_bno055_pack_write_reg(0x3D, data, DEPZ_BNO055_XFER_MAX + 1, out) == 0,
              "bno055 write_reg refuses 0 / 129 data bytes");
        CHECK(depz_bno055_pack_write_reg(0x00, data, DEPZ_BNO055_XFER_MAX, out) ==
                  1 + DEPZ_BNO055_XFER_MAX, "bno055 write_reg accepts 128 data bytes");
        /* contract 13 §1 (ERRATA A13): READ_REG len 1..128 and addr+len <= 0x100, like WRITE_REG */
        CHECK(depz_bno055_pack_read_reg(0x00, 0, out) == 0 && depz_bno055_pack_read_reg(0x00, 129, out) == 0 &&
                  depz_bno055_pack_read_reg(0xFF, 2, out) == 0, "bno055 read_reg refuses 0 / 129 B and past the page");
        CHECK(depz_bno055_pack_read_reg(0x80, 128, out) == 2 && depz_bno055_pack_read_reg(0xFF, 1, out) == 2,
              "bno055 read_reg accepts the limits");
        CHECK(depz_bno055_pack_write_reg(0xFF, data, 2, out) == 0, "bno055 write_reg refuses a write past the page");
        /* contract 13 §2: TIMER period 1..60000 ms; INT: the watchdog, 0 = none */
        CHECK(depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_TIMER, 0x08, 46, 0, out) == 0 &&
                  depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_TIMER, 0x08, 46, 60001, out) == 0,
              "bno055 start_stream refuses a TIMER period of 0 / 60001 ms");
        CHECK(depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_TIMER, 0x08, 46, 1, out) == 5 &&
                  depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_TIMER, 0x08, 46, 60000, out) == 5 &&
                  depz_bno055_pack_start_stream(DEPZ_BNO055_TRIGGER_INT, 0x35, 1, 0, out) == 5,
              "bno055 start_stream accepts the TIMER limits and an INT watchdog of 0");
    }

    /* decode: RPT_BNO_INFO 0x92, RPT_BNO_REG_DATA 0x91, RPT_BNO_REG_STREAM 0x93 */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "bno055 decode cases present");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int64_t report = json_as_int(json_obj_get(c, "report"));
        uint8_t *p; size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        const json_value *e = json_obj_get(c, "expect");
        if (report == DEPZ_BNO055_RPT_INFO) {
            depz_bno055_info info;
            int rc = depz_bno055_unpack_info(p, n, &info);
            CHECK(rc == 0, "bno055 info decode %s rc=%d", name, rc);
            if (rc == 0) {
#define BNO_INFO_FIELD(f) \
                CHECK((int64_t)info.f == json_as_int(json_obj_get(e, #f)), \
                      "bno055 info %s " #f ": got %lld", name, (long long)info.f)
                BNO_INFO_FIELD(i2c_addr);
                BNO_INFO_FIELD(chip_id);
                BNO_INFO_FIELD(acc_id);
                BNO_INFO_FIELD(mag_id);
                BNO_INFO_FIELD(gyr_id);
                BNO_INFO_FIELD(sw_rev);
                BNO_INFO_FIELD(bl_rev);
                BNO_INFO_FIELD(initialized);
                BNO_INFO_FIELD(int_level);
                BNO_INFO_FIELD(int_edges);
                BNO_INFO_FIELD(read_min_us);
                BNO_INFO_FIELD(read_max_us);
                BNO_INFO_FIELD(read_avg_us);
                BNO_INFO_FIELD(tx_dropped);
                BNO_INFO_FIELD(i2c_errors);
                BNO_INFO_FIELD(slots_skipped);
                BNO_INFO_FIELD(bus_recoveries);
                BNO_INFO_FIELD(last_i2c_error);
                BNO_INFO_FIELD(sensor_resets);
                BNO_INFO_FIELD(loop_max_us);
#undef BNO_INFO_FIELD
                CHECK(e->count == 20, "bno055 info %s: %zu expected fields, 20 checked",
                      name, e->count);
            }
            CHECK(depz_bno055_unpack_info(p, DEPZ_BNO055_INFO_SIZE - 1, &info) == -1,
                  "bno055 info %s rejects 37 B", name);
        } else if (report == DEPZ_BNO055_RPT_REG_DATA) {
            depz_bno055_reg_data r;
            int rc = depz_bno055_unpack_reg_data(p, n, &r);
            CHECK(rc == 0, "bno055 reg_data decode %s rc=%d", name, rc);
            if (rc == 0) {
                CHECK(r.cmd == json_as_int(json_obj_get(e, "cmd")),
                      "bno055 reg_data %s cmd: got %u", name, r.cmd);
                CHECK((int64_t)r.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")),
                      "bno055 reg_data %s timestamp_us: got %llu", name,
                      (unsigned long long)r.timestamp_us);
                check_payload(name, r.data, r.data_len, json_as_str(json_obj_get(e, "data")));
                CHECK(e->count == 3, "bno055 reg_data %s: %zu expected fields, 3 checked",
                      name, e->count);
            }
            CHECK(depz_bno055_unpack_reg_data(p, 8, &r) == -1,
                  "bno055 reg_data %s rejects 8 B", name);
        } else if (report == DEPZ_BNO055_RPT_STREAM) {
            depz_bno055_stream s;
            int rc = depz_bno055_unpack_stream(p, n, &s);
            CHECK(rc == 0, "bno055 stream decode %s rc=%d", name, rc);
            if (rc == 0) {
                CHECK((int64_t)s.timestamp_us == json_as_int(json_obj_get(e, "timestamp_us")),
                      "bno055 stream %s timestamp_us: got %llu", name,
                      (unsigned long long)s.timestamp_us);
                CHECK(s.addr == json_as_int(json_obj_get(e, "addr")),
                      "bno055 stream %s addr: got %u", name, s.addr);
                CHECK(s.len == json_as_int(json_obj_get(e, "len")),
                      "bno055 stream %s len: got %u", name, s.len);
                check_payload(name, s.data, s.len, json_as_str(json_obj_get(e, "data")));
                CHECK(e->count == 4, "bno055 stream %s: %zu expected fields, 4 checked",
                      name, e->count);
                CHECK(depz_bno055_unpack_stream(p, n - 1, &s) == -1,
                      "bno055 stream %s rejects a truncated block", name);
            }
        } else {
            CHECK(0, "bno055 decode %s: unknown report %lld", name, (long long)report);
        }
        free(p);
        seen_case(c, f0);
    }

    /* units: UNIT_SEL flags, repack drops the undefined bits */
    const json_value *units = json_obj_get(root, "units");
    CHECK(json_arr_size(units) > 0, "bno055 units cases present");
    for (size_t i = 0; i < json_arr_size(units); i++) {
        const json_value *c = json_arr_get(units, i);
        const int f0 = g_fail;
        int64_t sel = json_as_int(json_obj_get(c, "unit_sel"));
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_units u;
        depz_bno055_unpack_units((uint8_t)sel, &u);
#define UNIT_FIELD(f) \
        CHECK(u.f == json_as_bool(json_obj_get(e, #f)), \
              "bno055 units 0x%02llx " #f ": got %d", (long long)sel, u.f)
        UNIT_FIELD(accel_mg);
        UNIT_FIELD(gyro_rps);
        UNIT_FIELD(euler_rad);
        UNIT_FIELD(temp_f);
        UNIT_FIELD(android);
#undef UNIT_FIELD
        CHECK(e->count == 5, "bno055 units 0x%02llx: %zu expected fields, 5 checked",
              (long long)sel, e->count);
        CHECK(depz_bno055_pack_units(&u) == json_as_int(json_obj_get(c, "repack")),
              "bno055 units 0x%02llx repack: got 0x%02x", (long long)sel,
              depz_bno055_pack_units(&u));
        /* LSB constants follow the flags (§4.2) */
        CHECK(depz_bno055_accel_lsb(&u) == (u.accel_mg ? 1.0 : 100.0) &&
                  depz_bno055_gyro_lsb(&u) == (u.gyro_rps ? 900.0 : 16.0) &&
                  depz_bno055_euler_lsb(&u) == (u.euler_rad ? 900.0 : 16.0) &&
                  depz_bno055_temp_lsb(&u) == (u.temp_f ? 0.5 : 1.0),
              "bno055 units 0x%02llx LSBs", (long long)sel);
        seen_case(c, f0);
    }
    /* through an array: MSVC rejects a condition of constants (C4127 under /WX) */
    const double fixed_lsb[] = {DEPZ_BNO055_MAG_LSB, DEPZ_BNO055_QUAT_LSB, DEPZ_BNO055_FUSION_ACCEL_LSB};
    CHECK(fixed_lsb[0] == 16.0 && fixed_lsb[1] == 16384.0 && fixed_lsb[2] == 100.0, "bno055 fixed LSBs");

    /* calib_stat: 2-bit fields, fully_calibrated, exact repack */
    const json_value *cs = json_obj_get(root, "calib_stat");
    CHECK(json_arr_size(cs) > 0, "bno055 calib_stat cases present");
    for (size_t i = 0; i < json_arr_size(cs); i++) {
        const json_value *c = json_arr_get(cs, i);
        const int f0 = g_fail;
        int64_t v = json_as_int(json_obj_get(c, "value"));
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_calib_status s;
        depz_bno055_unpack_calib_status((uint8_t)v, &s);
#define CS_FIELD(f) \
        CHECK(s.f == json_as_int(json_obj_get(e, #f)), \
              "bno055 calib_stat 0x%02llx " #f ": got %u", (long long)v, s.f)
        CS_FIELD(system);
        CS_FIELD(gyro);
        CS_FIELD(accel);
        CS_FIELD(mag);
#undef CS_FIELD
        CHECK(e->count == 4, "bno055 calib_stat 0x%02llx: %zu expected fields, 4 checked",
              (long long)v, e->count);
        CHECK(depz_bno055_fully_calibrated(&s) ==
                  json_as_bool(json_obj_get(c, "fully_calibrated")),
              "bno055 calib_stat 0x%02llx fully_calibrated", (long long)v);
        CHECK(depz_bno055_pack_calib_status(&s) == v,
              "bno055 calib_stat 0x%02llx repack: got 0x%02x", (long long)v,
              depz_bno055_pack_calib_status(&s));
        seen_case(c, f0);
    }

    /* calibration_profile: 11 x i16 LE, byte-exact repack */
    const json_value *prof = json_obj_get(root, "calibration_profile");
    CHECK(json_arr_size(prof) > 0, "bno055 calibration_profile cases present");
    for (size_t i = 0; i < json_arr_size(prof); i++) {
        const json_value *c = json_arr_get(prof, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *hex = json_as_str(json_obj_get(c, "bytes"));
        uint8_t *raw; size_t n = hex_decode(hex, &raw);
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_calib_profile pr;
        int rc = depz_bno055_unpack_calib_profile(raw, n, &pr);
        CHECK(rc == 0, "bno055 calibration_profile %s rc=%d", name, rc);
        if (rc == 0) {
            static const char *VEC[3] = { "accel_offset", "mag_offset", "gyro_offset" };
            const int16_t *got[3] = { pr.accel_offset, pr.mag_offset, pr.gyro_offset };
            for (int k = 0; k < 3; k++) {
                const json_value *w = json_obj_get(e, VEC[k]);
                CHECK(json_arr_size(w) == 3, "bno055 calibration_profile %s %s size",
                      name, VEC[k]);
                for (size_t j = 0; j < 3 && j < json_arr_size(w); j++)
                    CHECK(got[k][j] == json_as_int(json_arr_get(w, j)),
                          "bno055 calibration_profile %s %s[%zu]: got %d", name, VEC[k],
                          j, got[k][j]);
            }
            CHECK(pr.accel_radius == json_as_int(json_obj_get(e, "accel_radius")),
                  "bno055 calibration_profile %s accel_radius: got %d", name, pr.accel_radius);
            CHECK(pr.mag_radius == json_as_int(json_obj_get(e, "mag_radius")),
                  "bno055 calibration_profile %s mag_radius: got %d", name, pr.mag_radius);
            CHECK(e->count == 5, "bno055 calibration_profile %s: %zu expected fields, 5 checked",
                  name, e->count);
            uint8_t back[DEPZ_BNO055_CALIB_PROFILE_LEN];
            size_t bn = depz_bno055_pack_calib_profile(&pr, back);
            CHECK(bn == DEPZ_BNO055_CALIB_PROFILE_LEN, "bno055 calibration_profile %s pack len",
                  name);
            check_payload(name, back, bn, hex);
        }
        CHECK(depz_bno055_unpack_calib_profile(raw, n - 1, &pr) == -1,
              "bno055 calibration_profile %s rejects 21 B", name);
        free(raw);
        seen_case(c, f0);
    }

    /* axis_remap: placements P0..P7, unpack / repack / placement lookup */
    const json_value *ar = json_obj_get(root, "axis_remap");
    CHECK(json_arr_size(ar) == 8, "bno055 axis_remap: %zu placements", json_arr_size(ar));
    for (size_t i = 0; i < json_arr_size(ar); i++) {
        const json_value *c = json_arr_get(ar, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t cfg = (uint8_t)json_as_int(json_obj_get(c, "config"));
        uint8_t sgn = (uint8_t)json_as_int(json_obj_get(c, "sign"));
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_axis_remap a;
        depz_bno055_unpack_axis_remap(cfg, sgn, &a);
#define AR_FIELD(f) \
        CHECK((int64_t)a.f == json_as_int(json_obj_get(e, #f)), \
              "bno055 axis_remap %s " #f ": got %d", name, (int)a.f)
#define AR_FLAG(f) \
        CHECK(a.f == json_as_bool(json_obj_get(e, #f)), \
              "bno055 axis_remap %s " #f ": got %d", name, (int)a.f)
        AR_FIELD(x);
        AR_FIELD(y);
        AR_FIELD(z);
        AR_FLAG(x_negative);
        AR_FLAG(y_negative);
        AR_FLAG(z_negative);
#undef AR_FIELD
#undef AR_FLAG
        CHECK(e->count == 6, "bno055 axis_remap %s: %zu expected fields, 6 checked",
              name, e->count);
        const json_value *rp = json_obj_get(c, "repack");
        uint8_t c2 = 0, s2 = 0;
        int rc = depz_bno055_pack_axis_remap(&a, &c2, &s2);
        CHECK(rc == 0 && c2 == json_as_int(json_arr_get(rp, 0)) &&
                  s2 == json_as_int(json_arr_get(rp, 1)),
              "bno055 axis_remap %s repack: rc=%d got %02x/%02x", name, rc, c2, s2);
        depz_bno055_axis_remap pl;
        rc = depz_bno055_placement(name, &pl);
        CHECK(rc == 0 && memcmp(&pl, &a, sizeof(a)) == 0,
              "bno055 placement %s matches the vector", name);
        CHECK(DEPZ_BNO055_PLACEMENTS[i][0] == cfg && DEPZ_BNO055_PLACEMENTS[i][1] == sgn,
              "bno055 PLACEMENTS[%zu] = %s", i, name);
        seen_case(c, f0);
    }
    {
        depz_bno055_axis_remap pl;
        CHECK(depz_bno055_placement("p1", &pl) == 0 && pl.x == 0 && pl.y == 1 && pl.z == 2,
              "bno055 placement is case-insensitive");
        CHECK(depz_bno055_placement("P8", &pl) == -1 && depz_bno055_placement("P", &pl) == -1 &&
                  depz_bno055_placement("P10", &pl) == -1 &&
                  depz_bno055_placement(NULL, &pl) == -1,
              "bno055 placement refuses unknown names");
    }

    /* axis_remap_invalid: a non-permutation must be refused, outputs untouched */
    const json_value *bad = json_obj_get(root, "axis_remap_invalid");
    CHECK(json_arr_size(bad) > 0, "bno055 axis_remap_invalid cases present");
    for (size_t i = 0; i < json_arr_size(bad); i++) {
        const json_value *c = json_arr_get(bad, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        depz_bno055_axis_remap a = {
            (uint8_t)json_as_int(json_obj_get(c, "x")),
            (uint8_t)json_as_int(json_obj_get(c, "y")),
            (uint8_t)json_as_int(json_obj_get(c, "z")), false, false, false };
        uint8_t c2 = 0xAA, s2 = 0xBB;
        CHECK(depz_bno055_pack_axis_remap(&a, &c2, &s2) == -1 && c2 == 0xAA && s2 == 0xBB,
              "bno055 axis_remap_invalid %s refused", name);
        seen_case(c, f0);
    }
    {
        depz_bno055_axis_remap a = { 3, 1, 2, false, false, false };
        uint8_t c2, s2;
        CHECK(depz_bno055_pack_axis_remap(&a, &c2, &s2) == -1,
              "bno055 axis_remap refuses axis code 3");
    }

    /* sensor_config: page-1 ACC / GYR / MAG config codes */
    const json_value *sc = json_obj_get(root, "sensor_config");
    const int sc_f0 = g_fail;
    const json_value *acc = json_obj_get(sc, "accel");
    CHECK(json_arr_size(acc) > 0, "bno055 sensor_config accel cases present");
    for (size_t i = 0; i < json_arr_size(acc); i++) {
        const json_value *c = json_arr_get(acc, i);
        int64_t v = json_as_int(json_obj_get(c, "value"));
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_accel_config a;
        depz_bno055_unpack_accel_config((uint8_t)v, &a);
        CHECK(a.range == json_as_int(json_obj_get(e, "range")) &&
                  a.bandwidth == json_as_int(json_obj_get(e, "bandwidth")) &&
                  a.power == json_as_int(json_obj_get(e, "power")) && e->count == 3,
              "bno055 accel_config 0x%02llx: got %u/%u/%u", (long long)v, a.range,
              a.bandwidth, a.power);
        CHECK(depz_bno055_pack_accel_config(&a) == v,
              "bno055 accel_config 0x%02llx repack: got 0x%02x", (long long)v,
              depz_bno055_pack_accel_config(&a));
    }
    const json_value *gyr = json_obj_get(sc, "gyro");
    CHECK(json_arr_size(gyr) > 0, "bno055 sensor_config gyro cases present");
    for (size_t i = 0; i < json_arr_size(gyr); i++) {
        const json_value *c = json_arr_get(gyr, i);
        const char *hex = json_as_str(json_obj_get(c, "bytes"));
        const json_value *e = json_obj_get(c, "expect");
        uint8_t *raw; size_t n = hex_decode(hex, &raw);
        CHECK(n == 2, "bno055 gyro_config %s: 2 bytes", hex);
        if (n == 2) {
            depz_bno055_gyro_config g;
            depz_bno055_unpack_gyro_config(raw, &g);
            CHECK(g.range == json_as_int(json_obj_get(e, "range")) &&
                      g.bandwidth == json_as_int(json_obj_get(e, "bandwidth")) &&
                      g.power == json_as_int(json_obj_get(e, "power")) && e->count == 3,
                  "bno055 gyro_config %s: got %u/%u/%u", hex, g.range, g.bandwidth, g.power);
            uint8_t back[2];
            size_t bn = depz_bno055_pack_gyro_config(&g, back);
            char label[64];
            snprintf(label, sizeof(label), "bno055 gyro_config %s repack", hex);
            check_payload(label, back, bn, hex);
        }
        free(raw);
    }
    const json_value *mag = json_obj_get(sc, "mag");
    CHECK(json_arr_size(mag) > 0, "bno055 sensor_config mag cases present");
    for (size_t i = 0; i < json_arr_size(mag); i++) {
        const json_value *c = json_arr_get(mag, i);
        int64_t v = json_as_int(json_obj_get(c, "value"));
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_mag_config m;
        depz_bno055_unpack_mag_config((uint8_t)v, &m);
        CHECK(m.rate == json_as_int(json_obj_get(e, "rate")) &&
                  m.mode == json_as_int(json_obj_get(e, "mode")) &&
                  m.power == json_as_int(json_obj_get(e, "power")) && e->count == 3,
              "bno055 mag_config 0x%02llx: got %u/%u/%u", (long long)v, m.rate, m.mode,
              m.power);
        /* bit 7 is not a field: the repack is value & 0x7F */
        CHECK(depz_bno055_pack_mag_config(&m) == (v & 0x7F),
              "bno055 mag_config 0x%02llx repack: got 0x%02x", (long long)v,
              depz_bno055_pack_mag_config(&m));
    }
    {
        depz_bno055_mag_config m;
        depz_bno055_unpack_mag_config(0x8B, &m);
        CHECK(depz_bno055_pack_mag_config(&m) == 0x0B, "bno055 mag_config drops bit 7");
    }
    seen_id("bno055/sensor_config", sc_f0);

    /* blocks: channel extraction from any register window */
    const json_value *blocks = json_obj_get(root, "blocks");
    CHECK(json_arr_size(blocks) > 0, "bno055 blocks cases present");
    for (size_t i = 0; i < json_arr_size(blocks); i++) {
        const json_value *c = json_arr_get(blocks, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint8_t addr = (uint8_t)json_as_int(json_obj_get(c, "addr"));
        uint8_t *raw; size_t n = hex_decode(json_as_str(json_obj_get(c, "data")), &raw);
        const json_value *e = json_obj_get(c, "expect");
        depz_bno055_block b;
        depz_bno055_decode_block(addr, raw, n, &b);
        check_bno_words(name, "accel", b.has_accel, b.accel, 3, json_obj_get(e, "accel"));
        check_bno_words(name, "mag", b.has_mag, b.mag, 3, json_obj_get(e, "mag"));
        check_bno_words(name, "gyro", b.has_gyro, b.gyro, 3, json_obj_get(e, "gyro"));
        check_bno_words(name, "euler", b.has_euler, b.euler, 3, json_obj_get(e, "euler"));
        check_bno_words(name, "quaternion", b.has_quaternion, b.quaternion, 4,
                        json_obj_get(e, "quaternion"));
        check_bno_words(name, "linear_accel", b.has_linear_accel, b.linear_accel, 3,
                        json_obj_get(e, "linear_accel"));
        check_bno_words(name, "gravity", b.has_gravity, b.gravity, 3,
                        json_obj_get(e, "gravity"));
        const json_value *t = json_obj_get(e, "temperature");
        if (json_is_null(t))
            CHECK(!b.has_temperature, "bno055 block %s temperature: present, want null", name);
        else
            CHECK(b.has_temperature && b.temperature == json_as_int(t),
                  "bno055 block %s temperature: got %d", name, b.temperature);
        const json_value *k = json_obj_get(e, "calib_stat");
        if (json_is_null(k))
            CHECK(!b.has_calib_stat, "bno055 block %s calib_stat: present, want null", name);
        else
            CHECK(b.has_calib_stat && b.calib_stat == json_as_int(k),
                  "bno055 block %s calib_stat: got %u", name, b.calib_stat);
        CHECK(e->count == 9, "bno055 block %s: %zu expected fields, 9 checked", name, e->count);
        free(raw);
        seen_case(c, f0);
    }
    /* window edges beyond the vectors: the full block, and one byte short */
    {
        uint8_t full[DEPZ_BNO055_FULL_BLOCK_LEN] = {0};
        full[DEPZ_BNO055_REG_TEMP - DEPZ_BNO055_FULL_BLOCK_ADDR] = 0xE5;       /* -27 */
        full[DEPZ_BNO055_REG_CALIB_STAT - DEPZ_BNO055_FULL_BLOCK_ADDR] = 0xFF;
        depz_bno055_block b;
        depz_bno055_decode_block(DEPZ_BNO055_FULL_BLOCK_ADDR, full, sizeof(full), &b);
        CHECK(b.has_accel && b.has_mag && b.has_gyro && b.has_euler && b.has_quaternion &&
                  b.has_linear_accel && b.has_gravity && b.has_temperature &&
                  b.has_calib_stat && b.temperature == -27 && b.calib_stat == 0xFF,
              "bno055 full block: every channel present");
        depz_bno055_decode_block(DEPZ_BNO055_FULL_BLOCK_ADDR, full, sizeof(full) - 1, &b);
        CHECK(b.has_temperature && !b.has_calib_stat, "bno055 45 B block: no calib_stat");
        depz_bno055_decode_block(DEPZ_BNO055_REG_GRV_DATA, full, 5, &b);
        CHECK(!b.has_gravity, "bno055 5 B gravity window: gravity absent");
        depz_bno055_decode_block(0xF0, full, 16, &b);
        CHECK(!b.has_accel && !b.has_calib_stat, "bno055 window past the data registers");
    }

    json_free(root);
}

/* ======================================================================== */
/* tmf8829.json + tmf8829_frame_time.json (contract 14)                      */
/* ======================================================================== */

/* A JSON int array (or null when absent) against n values. */
static void check_ints(const char *name, const char *field, const json_value *want, bool present,
                       const int64_t *got, size_t n)
{
    size_t i, bad = 0;
    if (!present) { CHECK(json_is_null(want), "tmf8829 %s %s: expected absent", name, field); return; }
    CHECK(json_arr_size(want) == n, "tmf8829 %s %s: %zu values, want %zu", name, field, n, json_arr_size(want));
    if (json_arr_size(want) != n) return;
    for (i = 0; i < n; i++) bad += json_as_int(json_arr_get(want, i)) != got[i];
    CHECK(bad == 0, "tmf8829 %s %s: %zu values differ", name, field, bad);
}

/* Python's _arrays(): peaks, rows, cols (shape of the first present array,
 * (0, 0) when none) and the flattened per-zone integers. */
static void check_results(const char *name, const json_value *e, const depz_tmf8829_results *r)
{
    size_t z = (size_t)r->rows * r->cols, np = (size_t)r->peaks * z, i;
    bool any = r->peaks > 0 || r->has_noise || r->has_xtalk;
    int64_t *v = (int64_t *)calloc((np > z ? np : z) + 1, sizeof(int64_t));
    CHECK(json_as_int(json_obj_get(e, "peaks")) == r->peaks, "tmf8829 %s peaks", name);
    CHECK(json_as_int(json_obj_get(e, "rows")) == (any ? r->rows : 0), "tmf8829 %s rows", name);
    CHECK(json_as_int(json_obj_get(e, "cols")) == (any ? r->cols : 0), "tmf8829 %s cols", name);
    for (i = 0; i < np; i++) v[i] = r->distance[i];
    check_ints(name, "distance", json_obj_get(e, "distance"), r->peaks > 0, v, np);
    for (i = 0; i < np; i++) v[i] = r->snr[i];
    check_ints(name, "snr", json_obj_get(e, "snr"), r->peaks > 0, v, np);
    for (i = 0; i < np; i++) v[i] = r->signal[i];
    check_ints(name, "signal", json_obj_get(e, "signal"), r->has_signal, v, np);
    for (i = 0; i < z; i++) v[i] = r->noise[i];
    check_ints(name, "noise", json_obj_get(e, "noise"), r->has_noise, v, z);
    for (i = 0; i < z; i++) v[i] = r->xtalk[i];
    check_ints(name, "xtalk", json_obj_get(e, "xtalk"), r->has_xtalk, v, z);
    free(v);
}

#define TMF_EQ(e, key, got) \
    CHECK(json_as_int(json_obj_get(e, key)) == (int64_t)(got), "tmf8829 %s " key ": got %lld", name, \
          (long long)(got))

static void test_tmf8829(void)
{
    json_value *root = load_vectors("tmf8829");
    depz_tmf8829_reassembler *ra;
    depz_tmf8829_builder *b;
    depz_tmf8829_measurement *m;
    depz_tmf8829_results *res;
    size_t i, j;
    if (!root) { g_fail++; return; }
    ra = (depz_tmf8829_reassembler *)malloc(sizeof *ra);
    b = (depz_tmf8829_builder *)malloc(sizeof *b);
    m = (depz_tmf8829_measurement *)malloc(sizeof *m);
    res = (depz_tmf8829_results *)malloc(sizeof *res);

    /* encode */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "tmf8829 encode cases present");
    for (i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int64_t cmd = json_as_int(json_obj_get(c, "cmd"));
        uint8_t out[1 + DEPZ_TMF8829_WRITE_MAX];
        size_t n = 0;
        if (cmd == DEPZ_TMF8829_CMD_READ_REG) {
            n = depz_tmf8829_pack_read_reg((int)json_as_int(json_obj_get(c, "addr")),
                                           (int)json_as_int(json_obj_get(c, "len")), out);
        } else if (cmd == DEPZ_TMF8829_CMD_WRITE_REG) {
            uint8_t *data;
            size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = depz_tmf8829_pack_write_reg((int)json_as_int(json_obj_get(c, "addr")), data, dn, out);
            free(data);
        } else if (cmd == DEPZ_TMF8829_CMD_START_STREAM) {
            n = depz_tmf8829_pack_start_stream((int)json_as_int(json_obj_get(c, "int_mask")),
                                               (int)json_as_int(json_obj_get(c, "flags")), out);
        }
        check_payload(name, out, n, json_as_str(json_obj_get(c, "payload")));
        seen_case(c, f0);
    }

    /* encode_errors: refused before anything is written */
    const json_value *ee = json_obj_get(root, "encode_errors");
    CHECK(json_arr_size(ee) > 0, "tmf8829 encode_errors present");
    for (i = 0; i < json_arr_size(ee); i++) {
        const json_value *c = json_arr_get(ee, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        uint8_t out[2 + DEPZ_TMF8829_WRITE_MAX + 8];
        size_t n;
        if (!strcmp(kind, "read_reg")) {
            n = depz_tmf8829_pack_read_reg((int)json_as_int(json_obj_get(c, "addr")),
                                           (int)json_as_int(json_obj_get(c, "len")), out);
        } else if (!strcmp(kind, "write_reg")) {
            uint8_t *data;
            size_t dn = hex_decode(json_as_str(json_obj_get(c, "data")), &data);
            n = dn <= DEPZ_TMF8829_WRITE_MAX + 1 ?
                depz_tmf8829_pack_write_reg((int)json_as_int(json_obj_get(c, "addr")), data, dn, out) : 1;
            free(data);
        } else {
            n = depz_tmf8829_pack_start_stream((int)json_as_int(json_obj_get(c, "int_mask")),
                                               (int)json_as_int(json_obj_get(c, "flags")), out);
        }
        CHECK(n == 0, "tmf8829 encode_error %s accepted", name);
        seen_case(c, f0);
    }
    {
        uint8_t out[1 + DEPZ_TMF8829_WRITE_MAX], d = 0;
        CHECK(depz_tmf8829_pack_read_reg(256, 1, out) == 0 && depz_tmf8829_pack_read_reg(-1, 1, out) == 0 &&
                  depz_tmf8829_pack_write_reg(256, &d, 1, out) == 0,
              "tmf8829 encoders refuse addr outside 0..255");
    }

    /* decode */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "tmf8829 decode cases present");
    for (i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        int64_t rpt = json_as_int(json_obj_get(c, "report"));
        const json_value *e = json_obj_get(c, "expect");
        uint8_t *p;
        size_t n = hex_decode(json_as_str(json_obj_get(c, "payload")), &p);
        if (rpt == DEPZ_TMF8829_RPT_REG_DATA) {
            depz_tmf8829_reg_data r;
            CHECK(depz_tmf8829_unpack_reg_data(p, n, &r) == 0, "tmf8829 %s decodes", name);
            TMF_EQ(e, "timestamp_us", r.timestamp_us);
            check_payload(name, r.data, r.data_len, json_as_str(json_obj_get(e, "data")));
            CHECK(depz_tmf8829_unpack_reg_data(p, 8, &r) == -1, "tmf8829 %s short refused", name);
            p[0] = 0x33;
            CHECK(depz_tmf8829_unpack_reg_data(p, n, &r) == -1, "tmf8829 %s wrong echo refused", name);
        } else if (rpt == DEPZ_TMF8829_RPT_INFO) {
            depz_tmf8829_info r;
            CHECK(depz_tmf8829_unpack_info(p, n, &r) == 0, "tmf8829 %s decodes", name);
            TMF_EQ(e, "int_edges", r.int_edges);
            TMF_EQ(e, "frames", r.frames);
            TMF_EQ(e, "frames_aborted", r.frames_aborted);
            TMF_EQ(e, "frame_errors", r.frame_errors);
            TMF_EQ(e, "spi_timeouts", r.spi_timeouts);
            TMF_EQ(e, "int_level", r.int_level);
            CHECK(json_as_bool(json_obj_get(e, "streaming")) == r.streaming, "tmf8829 %s streaming", name);
            TMF_EQ(e, "int_mask", r.int_mask);
            TMF_EQ(e, "last_int_status", r.last_int_status);
            TMF_EQ(e, "last_fifo_status", r.last_fifo_status);
            CHECK(depz_tmf8829_unpack_info(p, n - 1, &r) == -1, "tmf8829 %s short refused", name);
        } else if (rpt == DEPZ_TMF8829_RPT_FRAME) {
            depz_tmf8829_frame_chunk r;
            CHECK(depz_tmf8829_unpack_frame_chunk(p, n, &r) == 0, "tmf8829 %s decodes", name);
            TMF_EQ(e, "timestamp_us", r.timestamp_us);
            TMF_EQ(e, "full_size", r.full_size);
            TMF_EQ(e, "offset", r.offset);
            TMF_EQ(e, "frame_seq", r.frame_seq);
            TMF_EQ(e, "edge_us", r.edge_us);
            check_payload(name, r.data, r.data_len, json_as_str(json_obj_get(e, "data")));
        } else {
            depz_tmf8829_event r;
            CHECK(depz_tmf8829_unpack_event(p, n, &r) == 0, "tmf8829 %s decodes", name);
            TMF_EQ(e, "timestamp_us", r.timestamp_us);
            TMF_EQ(e, "int_status", r.int_status);
            TMF_EQ(e, "code", r.code);
            TMF_EQ(e, "frame_id", r.frame_id);
            TMF_EQ(e, "fifo_status", r.fifo_status);
        }
        free(p);
        seen_case(c, f0);
    }

    /* reassembly */
    const json_value *rs = json_obj_get(root, "reassembly");
    CHECK(json_arr_size(rs) > 0, "tmf8829 reassembly cases present");
    for (i = 0; i < json_arr_size(rs); i++) {
        const json_value *c = json_arr_get(rs, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *chunks = json_obj_get(c, "chunks");
        const json_value *e = json_obj_get(c, "expect");
        const json_value *frames = json_obj_get(e, "frames");
        size_t done = 0;
        depz_tmf8829_reassembler_reset(ra);
        for (j = 0; j < json_arr_size(chunks); j++) {
            uint8_t *p;
            size_t n = hex_decode(json_as_str(json_arr_get(chunks, j)), &p);
            depz_tmf8829_frame_chunk ch;
            depz_tmf8829_raw_frame f;
            CHECK(depz_tmf8829_unpack_frame_chunk(p, n, &ch) == 0, "tmf8829 %s chunk %zu", name, j);
            if (depz_tmf8829_reassembler_feed(ra, &ch, &f)) {
                const json_value *w = json_arr_get(frames, done++);
                char sha[65];
                CHECK(w != NULL, "tmf8829 %s: more frames than expected", name);
                if (w) {
                    sha256_hex(f.data, f.len, sha);
                    TMF_EQ(w, "frame_seq", f.frame_seq);
                    TMF_EQ(w, "timestamp_us", f.timestamp_us);
                    TMF_EQ(w, "edge_us", f.edge_us);
                    TMF_EQ(w, "size", f.len);
                    CHECK(!strcmp(sha, json_as_str(json_obj_get(w, "sha256"))), "tmf8829 %s sha256", name);
                }
            }
            free(p);
        }
        CHECK(done == json_arr_size(frames), "tmf8829 %s: %zu frames, want %zu", name, done, json_arr_size(frames));
        TMF_EQ(e, "completed", ra->completed);
        TMF_EQ(e, "discarded", ra->discarded);
        TMF_EQ(e, "seq_gaps", ra->seq_gaps);
        seen_case(c, f0);
    }

    /* frames: header / footer, problems, pixels or data digest */
    const json_value *fr = json_obj_get(root, "frames");
    CHECK(json_arr_size(fr) > 0, "tmf8829 frames present");
    for (i = 0; i < json_arr_size(fr); i++) {
        const json_value *c = json_arr_get(fr, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *h = json_obj_get(c, "header");
        const json_value *probs = json_obj_get(c, "problems");
        depz_tmf8829_frame_info f;
        depz_tmf8829_frame_problems pr;
        uint8_t *raw;
        size_t n = hex_decode(json_as_str(json_obj_get(c, "raw")), &raw);
        CHECK(depz_tmf8829_parse_frame(raw, n, &f) == 0, "tmf8829 %s parses", name);
        TMF_EQ(h, "fifo_status", f.fifo_status);
        TMF_EQ(h, "systick", f.systick);
        TMF_EQ(h, "id", f.id);
        TMF_EQ(h, "kind", f.kind);
        TMF_EQ(h, "fp_mode", f.fp_mode);
        TMF_EQ(h, "layout", f.layout);
        TMF_EQ(h, "payload", f.payload);
        TMF_EQ(h, "f_number", f.f_number);
        for (j = 0; j < 3; j++)
            CHECK(json_as_int(json_arr_get(json_obj_get(h, "temperature"), j)) == f.temperature[j],
                  "tmf8829 %s temperature[%zu]", name, j);
        TMF_EQ(h, "bdv", f.bdv);
        for (j = 0; j < 2; j++)
            CHECK(json_as_int(json_arr_get(json_obj_get(h, "ref_pos"), j)) == f.ref_pos[j],
                  "tmf8829 %s ref_pos[%zu]", name, j);
        TMF_EQ(h, "t0_integration", f.t0_integration);
        TMF_EQ(h, "t1_integration", f.t1_integration);
        TMF_EQ(h, "frame_status", f.frame_status);
        TMF_EQ(h, "reserved", f.reserved);
        TMF_EQ(h, "eof", f.eof);
        TMF_EQ(h, "size", f.size);
        CHECK(depz_tmf8829_frame_size(raw) == f.size, "tmf8829 %s frame_size from the head", name);
        depz_tmf8829_frame_problems_of(&f, &pr);
        CHECK((size_t)pr.count == json_arr_size(probs), "tmf8829 %s problems count", name);
        for (j = 0; j < json_arr_size(probs) && j < (size_t)pr.count; j++)
            CHECK(!strcmp(pr.text[j], json_as_str(json_arr_get(probs, j))), "tmf8829 %s problem %zu", name, j);
        if (json_obj_get(c, "error")) {
            /* a results frame whose pixel decode must fail */
            CHECK(depz_tmf8829_parse_results(raw, n, &f, res, NULL, 0) == -1, "tmf8829 %s: decode refused", name);
        } else if (f.kind == DEPZ_TMF8829_FID_RESULTS) {
            char err[160];
            int rc = depz_tmf8829_parse_results(raw, n, &f, res, err, sizeof err);
            CHECK(rc == 0, "tmf8829 %s results: %s", name, err);
            if (rc == 0) check_results(name, json_obj_get(c, "results"), res);
            CHECK(depz_tmf8829_parse_results(raw, n - 1, NULL, res, err, sizeof err) == -1,
                  "tmf8829 %s: a byte short is corrupt", name);
        } else {
            char sha[65];
            sha256_hex(raw + 21, n - 33, sha);
            CHECK(!strcmp(sha, json_as_str(json_obj_get(c, "data_sha256"))), "tmf8829 %s data_sha256", name);
            CHECK(depz_tmf8829_parse_results(raw, n, &f, res, NULL, 0) == -1, "tmf8829 %s: not results", name);
        }
        free(raw);
        seen_case(c, f0);
    }
    {
        uint8_t shortf[32] = {0};
        depz_tmf8829_frame_info f;
        CHECK(depz_tmf8829_parse_frame(shortf, sizeof shortf, &f) == -1, "tmf8829 32 B frame refused");
    }

    /* measurements: the builder over captured frame sequences */
    const json_value *ms = json_obj_get(root, "measurements");
    CHECK(json_arr_size(ms) > 0, "tmf8829 measurements present");
    for (i = 0; i < json_arr_size(ms); i++) {
        const json_value *s = json_arr_get(ms, i);
        const int f0 = g_fail;
        const char *name = json_as_str(json_obj_get(s, "name"));
        const json_value *frames = json_obj_get(s, "frames");
        const json_value *expect = json_obj_get(s, "expect");
        size_t got = 0;
        depz_tmf8829_builder_reset(b);
        for (j = 0; j < json_arr_size(frames); j++) {
            const json_value *fj = json_arr_get(frames, j);
            const char *rc = json_as_str(json_obj_get(fj, "raw_case"));
            const json_value *src = NULL;
            uint8_t *raw;
            size_t k, n;
            for (k = 0; k < json_arr_size(fr); k++)
                if (!strcmp(json_as_str(json_obj_get(json_arr_get(fr, k), "name")), rc)) src = json_arr_get(fr, k);
            CHECK(src != NULL, "tmf8829 %s: raw_case %s", name, rc);
            if (!src) continue;
            n = hex_decode(json_as_str(json_obj_get(src, "raw")), &raw);
            if (depz_tmf8829_builder_add(b, raw, n, json_as_int(json_obj_get(fj, "timestamp_us")),
                                         json_as_int(json_obj_get(fj, "edge_us")), m)) {
                const json_value *e = json_arr_get(expect, got++);
                CHECK(e != NULL, "tmf8829 %s: more measurements than expected", name);
                if (e) {
                    size_t t;
                    TMF_EQ(e, "timestamp_us", m->timestamp_us);
                    TMF_EQ(e, "flags", m->flags);
                    TMF_EQ(e, "fp_mode", m->fp_mode);
                    TMF_EQ(e, "f_number", m->f_number);
                    CHECK(json_arr_size(json_obj_get(e, "frame_times_us")) == m->n_frames, "tmf8829 %s n_frames", name);
                    for (t = 0; t < m->n_frames; t++) {
                        CHECK(json_as_int(json_arr_get(json_obj_get(e, "frame_times_us"), t)) == m->times[t].timestamp_us,
                              "tmf8829 %s frame_times_us[%zu]", name, t);
                        CHECK(json_as_int(json_arr_get(json_obj_get(e, "t1_len_us"), t)) == m->times[t].t1_len_us,
                              "tmf8829 %s t1_len_us[%zu]", name, t);
                    }
                    TMF_EQ(e, "ratio_q16", b->clock.ratio_q16);
                    TMF_EQ(e, "n_histograms", m->n_histograms);
                    TMF_EQ(e, "n_ref_frames", m->n_ref_frames);
                    TMF_EQ(e, "n_problems", m->n_problems);
                    CHECK(json_as_bool(json_obj_get(e, "distance_in_mm")) == m->distance_in_mm,
                          "tmf8829 %s distance_in_mm", name);
                    check_results(name, e, &m->results);
                    /* distance_mm = raw x ratio (/ 4 for 0.25 mm units) */
                    for (t = 0; t < (size_t)m->results.peaks * m->results.rows * m->results.cols; t++) {
                        double want = m->results.distance[t] * ((double)m->ratio_q16 / (8 << 16));
                        if (!m->distance_in_mm) want /= 4.0;
                        if (m->distance_mm[t] != want) { CHECK(0, "tmf8829 %s distance_mm[%zu]", name, t); break; }
                    }
                }
            }
            free(raw);
        }
        CHECK(got == json_arr_size(expect), "tmf8829 %s: %zu measurements, want %zu", name, got,
              json_arr_size(expect));
        seen_case(s, f0);
    }

    /* the half-frame edge cases (no vector): orphan second half, two first
     * halves, halves of different formats — built from the captured 32x32 pair */
    {
        const json_value *h0 = NULL, *h1 = NULL;
        uint8_t *a, *c2, *mixed;
        size_t na = 0, nc = 0;
        for (j = 0; j < json_arr_size(fr); j++) {
            const char *nm = json_as_str(json_obj_get(json_arr_get(fr, j), "name"));
            if (!strcmp(nm, "32x32#0")) h0 = json_arr_get(fr, j);
            if (!strcmp(nm, "32x32#1")) h1 = json_arr_get(fr, j);
        }
        CHECK(h0 && h1, "tmf8829 32x32 halves present");
        if (h0 && h1) {
            const char *name = "half-frame edge cases";
            int64_t ts = 2000000;
            na = hex_decode(json_as_str(json_obj_get(h0, "raw")), &a);
            nc = hex_decode(json_as_str(json_obj_get(h1, "raw")), &c2);
            CHECK((a[6] & 0x40) == 0 && (c2[6] & 0x40) != 0, "tmf8829 %s: first / second half order", name);
            depz_tmf8829_builder_reset(b);
            CHECK(depz_tmf8829_builder_add(b, c2, nc, ts, 0, m) == 0, "tmf8829 %s: orphan second half", name);
            CHECK(depz_tmf8829_builder_add(b, a, na, ts + 30000, 0, m) == 0, "tmf8829 %s: first half", name);
            CHECK(depz_tmf8829_builder_add(b, a, na, ts + 60000, 0, m) == 0, "tmf8829 %s: first half again", name);
            CHECK(depz_tmf8829_builder_add(b, c2, nc, ts + 90000, 0, m) == 1, "tmf8829 %s: pair merges", name);
            CHECK(m->n_problems == 2 && m->rows == 32 && m->results.rows == 32 && m->n_frames == 2,
                  "tmf8829 %s: 2 problems logged, 32 rows", name);
            CHECK(strstr(m->problems[0], "without a first one") && strstr(m->problems[1], "two first halves"),
                  "tmf8829 %s: problem texts", name);
            /* a second half whose layout differs (unused bit 0x80: pixels still decode) */
            mixed = (uint8_t *)malloc(nc);
            memcpy(mixed, c2, nc);
            mixed[6] ^= 0x80;
            CHECK(depz_tmf8829_builder_add(b, a, na, ts + 120000, 0, m) == 0, "tmf8829 %s: first half", name);
            CHECK(depz_tmf8829_builder_add(b, mixed, nc, ts + 150000, 0, m) == 0,
                  "tmf8829 %s: different formats dropped", name);
            CHECK(depz_tmf8829_builder_add(b, a, na, ts + 180000, 0, m) == 0, "tmf8829 %s: first half", name);
            CHECK(depz_tmf8829_builder_add(b, c2, nc, ts + 210000, 0, m) == 1, "tmf8829 %s: next pair", name);
            CHECK(m->n_problems >= 1 && strstr(m->problems[0], "half-frames of different formats"),
                  "tmf8829 %s: different formats logged", name);
            /* fp_mode differs (32x32 -> 32x32s, same size) */
            memcpy(mixed, c2, nc);
            mixed[5] = (uint8_t)((mixed[5] & 0xF0) | 4);
            CHECK(depz_tmf8829_builder_add(b, a, na, ts + 240000, 0, m) == 0, "tmf8829 %s: first half", name);
            CHECK(depz_tmf8829_builder_add(b, mixed, nc, ts + 270000, 0, m) == 0,
                  "tmf8829 %s: different fp_mode dropped", name);
            free(mixed);
            free(a);
            free(c2);
        }
    }

    /* config page fields */
    const json_value *cfg = json_obj_get(root, "config");
    {
        const int f0 = g_fail;
        uint8_t *page, pg[DEPZ_TMF8829_CFG_SIZE];
        size_t n = hex_decode(json_as_str(json_obj_get(cfg, "page")), &page);
        const json_value *fields = json_obj_get(cfg, "fields");
        const json_value *get = json_obj_get(cfg, "get");
        const json_value *set = json_obj_get(cfg, "set");
        const json_value *se = json_obj_get(cfg, "set_errors");
        CHECK(n == DEPZ_TMF8829_CFG_SIZE, "tmf8829 config page is 190 B");
        CHECK(fields->count == DEPZ_TMF8829_CFG_FIELD_COUNT, "tmf8829 config field count");
        for (i = 0; i < fields->count; i++) {
            const depz_tmf8829_cfg_field *f = depz_tmf8829_find_cfg_field(fields->keys[i]);
            const json_value *w = fields->items[i];
            CHECK(f && f->reg == json_as_int(json_arr_get(w, 0)) && f->shift == json_as_int(json_arr_get(w, 1)) &&
                      f->width == json_as_int(json_arr_get(w, 2)),
                  "tmf8829 config field %s", fields->keys[i]);
        }
        for (i = 0; i < get->count; i++) {
            uint32_t v = 0;
            CHECK(depz_tmf8829_cfg_get(page, get->keys[i], &v) == 0 && v == (uint32_t)json_as_int(get->items[i]),
                  "tmf8829 cfg_get %s: got %u", get->keys[i], v);
        }
        for (i = 0; i < json_arr_size(set); i++) {
            const json_value *c = json_arr_get(set, i);
            const char *field = json_as_str(json_obj_get(c, "field"));
            memcpy(pg, page, sizeof pg);
            CHECK(depz_tmf8829_cfg_set(pg, field, json_as_int(json_obj_get(c, "value"))) == 0, "tmf8829 cfg_set %s", field);
            check_payload(field, pg, sizeof pg, json_as_str(json_obj_get(c, "page")));
        }
        CHECK(json_arr_size(se) > 0, "tmf8829 set_errors present");
        for (i = 0; i < json_arr_size(se); i++) {
            const json_value *c = json_arr_get(se, i);
            const char *field = json_as_str(json_obj_get(c, "field"));
            memcpy(pg, page, sizeof pg);
            CHECK(depz_tmf8829_cfg_set(pg, field, json_as_int(json_obj_get(c, "value"))) == -2 &&
                      !memcmp(pg, page, sizeof pg),
                  "tmf8829 cfg_set %s refused, page untouched", field);
        }
        {
            uint32_t v;
            CHECK(depz_tmf8829_cfg_get(page, "nope", &v) == -1 && depz_tmf8829_cfg_set(pg, "nope", 0) == -1 &&
                      depz_tmf8829_cfg_set(pg, "xtalk", -1) == -2,
                  "tmf8829 cfg unknown field / negative value");
        }
        free(page);
        seen_id("tmf8829/config", f0);
    }
    /* presets */
    {
        const depz_tmf8829_preset *p8 = depz_tmf8829_find_preset("8x8"), *p48 = depz_tmf8829_find_preset("48x32-ha");
        const depz_tmf8829_preset *pxr = depz_tmf8829_find_preset("8x8-xr"), *p32 = depz_tmf8829_find_preset("32x32");
        CHECK(p8 && p8->cmd == 0x40 && p8->kilo_iterations == 586, "tmf8829 preset 8x8");
        CHECK(p32 && p32->cmd == 0x45 && p32->kilo_iterations == 2344, "tmf8829 preset 32x32");
        CHECK(p48 && p48->cmd == 0x48 && p48->kilo_iterations == 3516, "tmf8829 preset 48x32-ha");
        CHECK(pxr && pxr->cmd == 0x49 && pxr->kilo_iterations == 586, "tmf8829 preset 8x8-xr");
        CHECK(!depz_tmf8829_find_preset("64x64"), "tmf8829 unknown preset");
    }

    /* firmware image */
    {
        const json_value *fw = json_obj_get(root, "firmware");
        const int f0 = g_fail;
        char sha[65];
        sha256_hex(depz_tmf8829_firmware, depz_tmf8829_firmware_len, sha);
        CHECK(json_as_int(json_obj_get(fw, "load_addr")) == DEPZ_TMF8829_FIRMWARE_LOAD_ADDR, "tmf8829 firmware load_addr");
        CHECK(json_as_int(json_obj_get(fw, "size")) == (int64_t)depz_tmf8829_firmware_len, "tmf8829 firmware size");
        CHECK(json_as_int(json_obj_get(fw, "words")) == (int64_t)(depz_tmf8829_firmware_len / 4), "tmf8829 firmware words");
        CHECK(!strcmp(sha, json_as_str(json_obj_get(fw, "sha256"))), "tmf8829 firmware sha256 %s", sha);
        seen_id("tmf8829/firmware", f0);
    }

    free(res);
    free(m);
    free(b);
    free(ra);
    json_free(root);
}

/* recordings/tmf8829_frame_time.jsonl: the board's own capture the vector was
 * generated from, replayed raw — each `scenario` line starts a stream. */
static void test_tmf8829_frame_time_recording(void)
{
    char path[512];
    char *text, *p;
    depz_tmf8829_clock clock;
    depz_tmf8829_timer timer;
    size_t checked = 0, lines = 0;
    const int f0 = g_fail;
    snprintf(path, sizeof(path), "%s/recordings/tmf8829_frame_time.jsonl", DEPZ_VECTORS_DIR);
    text = read_file(path);
    if (!text) { g_fail++; return; }
    depz_tmf8829_clock_reset(&clock);
    depz_tmf8829_timer_reset(&timer);
    for (p = text; *p;) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = '\0';
        if (*p == '{') {
            json_value *v = json_parse(p);
            CHECK(v != NULL, "tmf8829_frame_time.jsonl line %zu parses", lines + 1);
            if (v && json_obj_get(v, "scenario")) {
                depz_tmf8829_clock_reset(&clock);
                depz_tmf8829_timer_reset(&timer);
            } else if (v) {
                int64_t fid = json_as_int(json_obj_get(v, "id")), ts = json_as_int(json_obj_get(v, "ts"));
                uint32_t tick = (uint32_t)json_as_int(json_obj_get(v, "systick"));
                unsigned kind = (unsigned)fid & 0xF0u;
                const json_value *exp = json_obj_get(v, "exp");
                if (kind != 0x30) {
                    uint8_t flags = depz_tmf8829_clock_add(&clock, ts, tick);
                    if (kind == 0x20) {
                        depz_tmf8829_timer_histograms(&timer);
                    } else {
                        depz_tmf8829_frame_time t;
                        depz_tmf8829_timer_results(&timer, &clock, (unsigned)fid & 0x0Fu, tick,
                                                   (uint32_t)json_as_int(json_obj_get(v, "t0")),
                                                   (uint32_t)json_as_int(json_obj_get(v, "t1")), ts,
                                                   json_as_int(json_obj_get(v, "edge_us")), flags, &t);
                        CHECK(t.timestamp_us == json_as_int(json_obj_get(exp, "timestamp_us")) &&
                                  t.t1_len_us == json_as_int(json_obj_get(exp, "t1_len_us")) &&
                                  t.flags == json_as_int(json_obj_get(exp, "flags")) &&
                                  clock.ratio_q16 == json_as_int(json_obj_get(exp, "ratio_q16")),
                              "tmf8829_frame_time.jsonl line %zu", lines + 1);
                        checked++;
                    }
                }
            }
            json_free(v);
            lines++;
        }
        if (!nl) break;
        p = nl + 1;
    }
    free(text);
    CHECK(checked == 970, "tmf8829_frame_time.jsonl: %zu results frames checked, want 970", checked);
    seen_id("recordings/tmf8829_frame_time.jsonl", f0);
}

/* tmf8829_frame_time.json: the clock model and capture time, bit for bit. */
static void test_tmf8829_frame_time(void)
{
    json_value *root = load_vectors("tmf8829_frame_time");
    const json_value *cols, *streams;
    int ci[12];
    static const char *const COLS[12] = {"id", "layout", "f_number", "systick", "t0", "t1", "ts", "edge_us",
                                         "exp_timestamp_us", "exp_t1_len_us", "exp_flags", "exp_ratio_q16"};
    size_t i, j, k, checked = 0;
    if (!root) { g_fail++; return; }
    cols = json_obj_get(root, "columns");
    for (k = 0; k < 12; k++) {
        ci[k] = -1;
        for (i = 0; i < json_arr_size(cols); i++)
            if (!strcmp(json_as_str(json_arr_get(cols, i)), COLS[k])) ci[k] = (int)i;
        CHECK(ci[k] >= 0, "tmf8829_frame_time column %s", COLS[k]);
        if (ci[k] < 0) { json_free(root); return; }
    }
#define COL(row, k) json_as_int(json_arr_get(row, (size_t)ci[k]))
    streams = json_obj_get(root, "streams");
    for (i = 0; i < json_arr_size(streams); i++) {
        const json_value *s = json_arr_get(streams, i);
        const int f0 = g_fail;
        const char *scen = json_as_str(json_obj_get(s, "scenario"));
        const json_value *frames = json_obj_get(s, "frames");
        depz_tmf8829_clock clock;
        depz_tmf8829_timer timer;
        depz_tmf8829_clock_reset(&clock);
        depz_tmf8829_timer_reset(&timer);
        for (j = 0; j < json_arr_size(frames); j++) {
            const json_value *row = json_arr_get(frames, j);
            int64_t fid = COL(row, 0), ts = COL(row, 6);
            uint32_t tick = (uint32_t)COL(row, 3);
            unsigned kind = (unsigned)fid & 0xF0u;
            uint8_t flags;
            depz_tmf8829_frame_time t;
            if (kind == 0x30) continue;
            flags = depz_tmf8829_clock_add(&clock, ts, tick);
            if (kind == 0x20) { depz_tmf8829_timer_histograms(&timer); continue; }
            depz_tmf8829_timer_results(&timer, &clock, (unsigned)fid & 0x0Fu, tick, (uint32_t)COL(row, 4),
                                       (uint32_t)COL(row, 5), ts, COL(row, 7), flags, &t);
            CHECK(t.timestamp_us == COL(row, 8) && t.t1_len_us == COL(row, 9) && t.flags == COL(row, 10) &&
                      clock.ratio_q16 == COL(row, 11),
                  "tmf8829_frame_time %s frame %zu: ts %lld/%lld t1 %u/%lld flags %u/%lld ratio %u/%lld", scen, j,
                  (long long)t.timestamp_us, (long long)COL(row, 8), t.t1_len_us, (long long)COL(row, 9),
                  t.flags, (long long)COL(row, 10), clock.ratio_q16, (long long)COL(row, 11));
            checked++;
        }
        seen_case(s, f0);
    }
#undef COL
    CHECK(checked == 970, "tmf8829_frame_time: %zu results frames checked, want 970", checked);
    /* floor semantics on negative operands (C truncates toward zero) */
    {
        depz_tmf8829_clock c;
        depz_tmf8829_clock_reset(&c);
        CHECK(depz_tmf8829_clock_to_us(&c, 1000, 100, 99) == 992, "tmf8829 to_us one tick back");
        c.ratio_q16 = DEPZ_TMF8829_NOMINAL_Q16 + 1;
        CHECK(depz_tmf8829_clock_to_us(&c, 0, 100, 99) == -9, "tmf8829 to_us floors a negative product");
        CHECK(depz_tmf8829_clock_to_us(&c, 0, 0xFFFFFFFFu, 0) == 8, "tmf8829 to_us across the u32 wrap");
        depz_tmf8829_clock_reset(&c);
        CHECK(depz_tmf8829_clock_add(&c, 1000000, 0) == 0, "tmf8829 clock first pair");
        /* MCU time went back 15 us, sensor ticked 0: tol = -15 // 10 + 50 = 48 */
        CHECK(depz_tmf8829_clock_add(&c, 999985, 0) == 0, "tmf8829 clock: -15 us within tol 48");
        /* -46 us: tol = -46 // 10 + 50 = 45 < 46 -> reset (truncation would give 46 and accept) */
        CHECK(depz_tmf8829_clock_add(&c, 999939, 0) == DEPZ_TMF8829_FT_CLOCK_RESET,
              "tmf8829 clock: -46 us outside the floored tol -> CLOCK_RESET");
    }
    json_free(root);
    test_tmf8829_frame_time_recording();
}

/* ======================================================================== */
/* probe.json — class resolution (contract 02 §4.4)                          */
/* ======================================================================== */
static void test_probe(void)
{
    json_value *root = load_vectors("probe");
    if (!root) { g_fail++; return; }
    {
        const json_value *names = json_obj_get(root, "classes");
        const int f0 = g_fail;
        CHECK(json_arr_size(names) == DEPZ_SENSOR_CLASS_COUNT, "probe classes: %zu names, SDK has %d",
              json_arr_size(names), DEPZ_SENSOR_CLASS_COUNT);
        for (size_t i = 0; i < json_arr_size(names) && i < DEPZ_SENSOR_CLASS_COUNT; i++) {
            const char *want = json_as_str(json_arr_get(names, i));
            const char *got = depz_sensor_class_name(i);
            CHECK(got && want && strcmp(got, want) == 0, "probe class %zu: expected %s got %s", i, want,
                  got ? got : "NULL");
        }
        CHECK(depz_sensor_class_name(DEPZ_SENSOR_CLASS_COUNT) == NULL, "probe class past the end");
        seen_id("probe/classes", f0);
    }
    const json_value *cases = json_obj_get(root, "resolve");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
        const int f0 = g_fail;
        const json_value *vv = json_obj_get(c, "usb_vid"), *pv = json_obj_get(c, "usb_pid");
        int vid = json_is_null(vv) ? -1 : (int)json_as_int(vv);
        int pid = json_is_null(pv) ? -1 : (int)json_as_int(pv);
        const char *want = json_as_str(json_obj_get(c, "class"));
        const char *got = depz_resolve_sensor_class(json_as_str(json_obj_get(c, "software_name")),
                                                    json_as_str(json_obj_get(c, "device_name")), vid, pid);
        CHECK(got && want && strcmp(got, want) == 0, "probe resolve %s: expected %s got %s",
              json_as_str(json_obj_get(c, "name")), want, got ? got : "NULL");
        seen_case(c, f0);
    }
    json_free(root);
}

/* ======================================================================== */
/* input_rules groups (contract 07 §3), in every file that has one:       */
/* short payloads rejected, trailing bytes ignored, every encoder argument    */
/* range-checked. An argument outside the C parameter's own type (300 for a   */
/* uint8_t, -1 for an unsigned) cannot be passed at all: the case is the C    */
/* type's rejection (contract 16 §1), asserted as such. Everything inside the */
/* type goes through the SDK function, which must refuse or decode.           */
/* ======================================================================== */

/* `key` of `in` as an integer in [lo, hi]; false when absent / outside. */
static bool ir_int(const json_value *in, const char *key, int64_t lo, int64_t hi, int64_t *out)
{
    const json_value *v = json_obj_get(in, key);
    if (!v || v->kind != JSON_INT || v->range != 0 || v->i < lo || v->i > hi) return false;
    *out = v->i;
    return true;
}
/* `var` = the range-checked integer `key`, cast to `type` (it fits: the range is the type's). */
#define IR_INT(type, var, key, lo, hi) do { int64_t t_; if (!ir_int(in, key, lo, hi, &t_)) return true; var = (type)t_; } while (0)
#define IR_U8(var, key)  IR_INT(uint8_t, var, key, 0, 0xFF)
#define IR_U16(var, key) IR_INT(uint16_t, var, key, 0, 0xFFFF)
#define IR_U32(var, key) IR_INT(uint32_t, var, key, 0, 0xFFFFFFFFLL)
#define IR_I32(var, key) IR_INT(int32_t, var, key, INT32_MIN, INT32_MAX)

static int64_t ex_int(const json_value *e, const char *k) { return json_as_int(json_obj_get(e, k)); }
static double ex_dbl(const json_value *e, const char *k)
{
    const json_value *v = json_obj_get(e, k);
    return v && v->kind == JSON_DOUBLE ? v->d : (double)json_as_int(v);
}

#if DEPZ_VECTORS_HAS_IO
/* The event the device layer emits for `rpt`/`p` sent unsolicited (nothing
 * pending) on a loopback pair — the `common_event` op of contract 16. Returns
 * DEPZ_OK with the event in `ev`, else what went wrong. */
static int unsolicited_event(uint8_t rpt, const uint8_t *p, size_t pn, depz_device_event *ev)
{
    static uint8_t frame[DEPZ_MAX_FRAME];
    depz_link *host, *side;
    depz_device *dev;
    depz_stream *evs;
    size_t frame_len;
    int rc;
    if (depz_build_packet(rpt, p, pn, 0, DEPZ_CRC_NONE, frame, sizeof frame, &frame_len) != 0) return DEPZ_E_ARG;
    if ((rc = depz_link_loopback_pair(&host, &side)) != DEPZ_OK) return rc;
    if ((rc = depz_device_open_link(host, &dev)) != DEPZ_OK) { depz_link_free(side); return rc; }
    evs = depz_device_events(dev, 8);
    depz_link_write(side, frame, frame_len);
    rc = evs ? depz_stream_next(evs, ev, 2000) : DEPZ_E_NOMEM;
    depz_stream_close(evs);
    depz_device_close(dev);
    depz_link_free(side);
    return rc;
}

/* The `common_seq_gap` op of contract 16 (contract 01 §5 step 5): one
 * packet per seq (report 0x7F, payload 00 — each one a hex text event) on a
 * loopback pair; once every packet's text event is out, the seq_gap events
 * (at most `cap` kept in `gaps`, all counted in *n_gaps) and stats.seq_gaps.
 * A gap is emitted ahead of its packet, so none can still be on the way. */
static int seq_gap_events(const uint8_t *seqs, size_t n, depz_device_event *gaps, size_t cap,
                          size_t *n_gaps, uint64_t *seq_gaps)
{
    static const uint8_t payload[1] = {0x00};
    uint8_t frame[16];
    depz_link *host, *side;
    depz_device *dev;
    depz_stream *evs;
    depz_link_stats st;
    depz_device_event ev;
    size_t i, frame_len, texts = 0;
    int rc;
    *n_gaps = 0;
    if ((rc = depz_link_loopback_pair(&host, &side)) != DEPZ_OK) return rc;
    if ((rc = depz_device_open_link(host, &dev)) != DEPZ_OK) { depz_link_free(side); return rc; }
    evs = depz_device_events(dev, 64);
    rc = evs ? DEPZ_OK : DEPZ_E_NOMEM;
    for (i = 0; rc == DEPZ_OK && i < n; i++) {
        rc = depz_build_packet(0x7F, payload, sizeof payload, seqs[i], DEPZ_CRC_NONE, frame, sizeof frame, &frame_len);
        if (rc == DEPZ_OK) rc = depz_link_write(side, frame, frame_len);
    }
    while (rc == DEPZ_OK && texts < n) {
        rc = depz_stream_next(evs, &ev, 2000);
        if (rc != DEPZ_OK) break;
        if (ev.type == DEPZ_DEV_EV_TEXT) texts++;
        else if (ev.type == DEPZ_DEV_EV_SEQ_GAP) {
            if (*n_gaps < cap) gaps[*n_gaps] = ev;
            (*n_gaps)++;
        }
    }
    depz_device_stats(dev, &st);
    *seq_gaps = st.seq_gaps;
    depz_stream_close(evs);
    depz_device_close(dev);
    depz_link_free(side);
    return rc;
}

/* text[0..len) in a fresh temporary file (the depzrec_read op: the replay
 * link opens a path); its path in a static buffer — the caller removes the
 * file before the next call — or NULL. */
static const char *write_temp_file(const char *text, size_t len)
{
    static char path[512];
    FILE *f;
#ifdef _WIN32
    if (tmpnam_s(path, sizeof path) != 0) return NULL;
    f = fopen(path, "wb");
#else
    const char *dir = getenv("TMPDIR");
    int fd;
    snprintf(path, sizeof path, "%s/depz_vectors_XXXXXX", dir && *dir ? dir : "/tmp");
    fd = mkstemp(path);
    if (fd < 0) return NULL;
    f = fdopen(fd, "wb");
    if (!f) { close(fd); return NULL; }
#endif
    if (!f) return NULL;
    fwrite(text, 1, len, f);
    fclose(f);
    return path;
}

/* The depzrec_read op (contract 08): the capture replayed loosely, 64 KiB of
 * tx written first so every rx event's gate is open, every rx chunk read
 * (timeout 0) until the replay is exhausted. Returns the open's rc; the
 * bytes land in *rx / *rx_len (malloc'd) when it opened. */
static int replay_all_rx(const char *text, size_t len, uint8_t **rx, size_t *rx_len, const char *label)
{
    static uint8_t zeros[65536];
    uint8_t chunk[4096];
    const char *path = write_temp_file(text, len);
    depz_link *l = NULL;
    size_t cap = 0, got;
    int rc;
    *rx = NULL;
    *rx_len = 0;
    if (!path) { CHECK(0, "%s: cannot create a temporary file", label); return DEPZ_E_IO; }
    rc = depz_link_open_replay(path, false, false, &l);
    remove(path);
    if (rc != DEPZ_OK) return rc;
    CHECK(depz_link_write(l, zeros, sizeof zeros) == DEPZ_OK, "%s: tx write: %s", label, depz_last_error());
    while (!depz_link_replay_exhausted(l)) {
        size_t before, after;
        depz_link_replay_pending(l, NULL, &before);
        if (depz_link_read(l, chunk, sizeof chunk, 0, &got) != DEPZ_OK) {
            CHECK(0, "%s: rx read: %s", label, depz_last_error());
            break;
        }
        depz_link_replay_pending(l, NULL, &after);
        if (!got && after == before) {
            CHECK(0, "%s: an rx event stays gated after 64 KiB of tx", label);
            break;
        }
        if (*rx_len + got > cap) {
            cap = (*rx_len + got) * 2 + 16;
            *rx = (uint8_t *)realloc(*rx, cap);
        }
        memcpy(*rx + *rx_len, chunk, got);
        *rx_len += got;
    }
    depz_link_free(l);
    return DEPZ_OK;
}
#endif

/* Run one input_rules case; returns true when the SDK (or the C type)
 * refused it, else checks the decoded / encoded values against `e`. */
static bool ir_run(const char *label, const char *op, const json_value *in, const json_value *e)
{
    const char *kind = json_as_str(json_obj_get(in, "kind"));
    uint8_t out[256];
    size_t n = 0;
    if (!strcmp(op, "common_decode") || !strcmp(op, "sr04_decode")) {
        int rpt = (int)json_as_int(json_obj_get(in, "report"));
        uint8_t *p; size_t pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        bool rej = false;
        if (!strcmp(op, "common_decode") && rpt == DEPZ_RPT_STATUS) {
            depz_status_report r;
            rej = depz_unpack_status(p, pn, &r) != 0;
            if (!rej) CHECK(r.cmd == ex_int(e, "cmd") && r.status == ex_int(e, "status"), "%s: fields", label);
        } else if (!strcmp(op, "common_decode") && rpt == DEPZ_RPT_TEXT) {
            depz_text_report r;
            rej = depz_unpack_text(p, pn, &r) != 0;
            if (!rej) CHECK(r.cmd == ex_int(e, "cmd") && !strcmp(r.text, json_as_str(json_obj_get(e, "text"))),
                            "%s: fields", label);
        } else if (!strcmp(op, "common_decode") && rpt == DEPZ_RPT_TEMPERATURE) {
            depz_temperature_report r;
            rej = depz_unpack_temperature(p, pn, &r) != 0;
            if (!rej) CHECK((int64_t)r.timestamp_us == ex_int(e, "timestamp_us") &&
                            r.raw_decidegrees == ex_int(e, "raw_decidegrees"), "%s: fields", label);
        } else if (!strcmp(op, "common_decode") && rpt == DEPZ_RPT_SEQUENCE_ERROR) {
            depz_sequence_error_report r;
            rej = depz_unpack_sequence_error(p, pn, &r) != 0;
            if (!rej) CHECK(r.expected_seq == ex_int(e, "expected_seq") &&
                            r.received_seq == ex_int(e, "received_seq"), "%s: fields", label);
        } else if (rpt == DEPZ_SR04_RPT_DATA) {
            depz_sr04_data d;
            rej = depz_sr04_unpack_data(p, pn, &d) != 0;
            if (!rej) CHECK(d.source_cmd == ex_int(e, "source_cmd") && (int64_t)d.timestamp_us == ex_int(e, "timestamp_us") &&
                            d.echo_time_us == ex_int(e, "echo_time_us"), "%s: fields", label);
        } else if (rpt == DEPZ_SR04_RPT_SAMPLE_PERIOD) {
            uint32_t v;
            rej = depz_sr04_unpack_sample_period(p, pn, &v) != 0;
            if (!rej) CHECK((int64_t)v == ex_int(e, "period_us"), "%s: period", label);
        } else if (rpt == DEPZ_SR04_RPT_ECHO_DECAY) {
            uint16_t v;
            rej = depz_sr04_unpack_echo_decay(p, pn, &v) != 0;
            if (!rej) CHECK(v == ex_int(e, "decay_us"), "%s: decay", label);
        } else {
            CHECK(0, "%s: unknown report 0x%02x", label, rpt);
        }
        free(p);
        return rej;
    }
    if (!strcmp(op, "common_encode")) {
        if (!strcmp(kind, "sync_pin_config")) {
            depz_sync_pin_config c;
            IR_U8(c.pin, "pin"); IR_U8(c.mode, "mode"); IR_U8(c.polarity, "polarity");
            n = depz_pack_sync_pin_config(&c, out);
        } else if (!strcmp(kind, "sync_time_request")) {
            const json_value *v = json_obj_get(in, "pc_timestamp_us");
            uint64_t t;
            if (!v || v->kind != JSON_INT || v->range == 2 || (v->range == 0 && v->i < 0)) return true; /* not a uint64_t */
            t = v->range == 1 ? v->u : (uint64_t)v->i;
            n = depz_pack_sync_time(t, out);
        } else if (!strcmp(kind, "set_payload_crc_type")) {
            uint8_t ct; IR_U8(ct, "crc_type");
            n = depz_pack_set_payload_crc_type(ct, out);
        } else {
            CHECK(0, "%s: no input_rules handler for common_encode kind %s", label, kind);
            return false;
        }
    } else if (!strcmp(op, "sr04_encode")) {
        if (!strcmp(kind, "set_sample_period")) { uint32_t v; IR_U32(v, "period_us"); n = depz_sr04_pack_sample_period(v, out); }
        else if (!strcmp(kind, "set_echo_decay")) { uint16_t v; IR_U16(v, "decay_us"); n = depz_sr04_pack_echo_decay(v, out); }
    } else if (!strcmp(op, "framing_encode")) {
        uint8_t cmd; int ct; unsigned seq;
        size_t on = 0;
        uint8_t *p; size_t pn;
        IR_U8(cmd, "cmd"); IR_I32(ct, "crc_type"); IR_U32(seq, "seq");
        pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        uint8_t frame[DEPZ_MAX_FRAME];
        int rc = depz_build_packet(cmd, pn ? p : NULL, pn, seq, (depz_crc_type)ct, frame, sizeof frame, &on);
        free(p);
        if (rc != 0) return true;
        check_payload(label, frame, on, json_as_str(json_obj_get(e, "frame")));
        return false;
    } else if (!strcmp(op, "bno055_encode")) {
        if (!strcmp(kind, "read_reg")) {
            uint8_t a, l; IR_U8(a, "addr"); IR_U8(l, "len");
            n = depz_bno055_pack_read_reg(a, l, out);
        } else if (!strcmp(kind, "start_stream")) {
            uint8_t t, a, l; uint16_t per;
            IR_U8(t, "trigger"); IR_U8(a, "addr"); IR_U8(l, "len"); IR_U16(per, "period_ms");
            n = depz_bno055_pack_start_stream(t, a, l, per, out);
        } else if (!strcmp(kind, "write_reg")) {
            uint8_t a; uint8_t *d; size_t dn;
            IR_U8(a, "addr");
            dn = hex_decode(json_as_str(json_obj_get(in, "data")), &d);
            uint8_t *big = (uint8_t *)malloc(dn + 1);
            n = depz_bno055_pack_write_reg(a, d, dn, big);
            free(d);
            if (n != 0) check_payload(label, big, n, json_as_str(json_obj_get(e, "payload")));
            free(big);
            return n == 0;
        } else {
            CHECK(0, "%s: no input_rules handler for bno055_encode kind %s", label, kind);
            return false;
        }
    } else if (!strcmp(op, "bno055_sensor_config")) {
        uint8_t *b; size_t bn = hex_decode(json_as_str(json_obj_get(in, "bytes")), &b);
        depz_bno055_gyro_config g;
        int rc = depz_bno055_unpack_gyro_config_n(b, bn, &g);
        free(b);
        if (rc != 0) return true;
        CHECK(g.range == ex_int(e, "range") && g.bandwidth == ex_int(e, "bandwidth") && g.power == ex_int(e, "power"),
              "%s: fields", label);
        return false;
    } else if (!strcmp(op, "bno086_shtp_header_pack")) {
        depz_shtp_header h;
        const json_value *cv = json_obj_get(in, "continuation");
        IR_U16(h.length, "length"); IR_U8(h.channel, "channel"); IR_U8(h.seq, "seq");
        h.continuation = json_as_bool(cv);
        if (depz_shtp_pack_header_checked(&h, out) != 0) return true;
        check_payload(label, out, 4, json_as_str(json_obj_get(e, "bytes")));
        return false;
    } else if (!strcmp(op, "bno086_shtp_tx_seq")) {
        depz_shtp_layer l;
        const json_value *pk = json_obj_get(in, "packets");
        bool rej = false;
        depz_shtp_init(&l);
        for (size_t k = 0; k < json_arr_size(pk) && !rej; k++) {
            const json_value *pv = json_arr_get(pk, k);
            int64_t ch;
            uint8_t *p; size_t pn;
            if (!ir_int(pv, "channel", 0, 0xFF, &ch)) { rej = true; break; }
            pn = hex_decode(json_as_str(json_obj_get(pv, "payload")), &p);
            rej = depz_shtp_next_frame(&l, (uint8_t)ch, p, pn, out, sizeof out) == 0;
            free(p);
        }
        depz_shtp_free(&l);
        if (!rej) CHECK(0, "%s: accepted, the case has no frames to compare", label);
        return rej;
    } else if (!strcmp(op, "bno086_control_encode")) {
        if (!strcmp(kind, "set_feature")) {
            uint8_t id, fl; uint16_t sens; uint32_t iv, bt, cw;
            IR_U8(id, "sensor_id"); IR_U8(fl, "flags"); IR_U16(sens, "sensitivity");
            IR_U32(iv, "interval_us"); IR_U32(bt, "batch_us"); IR_U32(cw, "cfg_word");
            n = depz_bno_pack_set_feature(id, fl, sens, iv, bt, cw, out);
        } else if (!strcmp(kind, "command_request")) {
            uint8_t sq, cmd; uint8_t *p; size_t pn;
            IR_U8(sq, "seq"); IR_U8(cmd, "command");
            pn = hex_decode(json_as_str(json_obj_get(in, "params")), &p);
            n = depz_bno_pack_command_request(sq, cmd, p, pn, out);
            free(p);
        } else if (!strcmp(kind, "frs_write_data")) {
            uint16_t off; uint32_t w[8];
            const json_value *wl = json_obj_get(in, "words");
            size_t wn = json_arr_size(wl);
            IR_U16(off, "offset_words");
            for (size_t k = 0; k < wn && k < 8; k++) {
                const json_value *wv = json_arr_get(wl, k);
                if (wv->kind != JSON_INT || wv->range != 0 || wv->i < 0 || wv->i > 0xFFFFFFFFLL) return true;
                w[k] = (uint32_t)wv->i;
            }
            n = depz_bno_pack_frs_write_data(off, w, wn, out, sizeof out);
        }
    } else if (!strcmp(op, "tmf8829_encode")) {
        int cmd = (int)json_as_int(json_obj_get(in, "cmd"));
        if (cmd == DEPZ_TMF8829_CMD_START_STREAM) {
            int m, f; IR_I32(m, "int_mask"); IR_I32(f, "flags");
            n = depz_tmf8829_pack_start_stream(m, f, out);
        }
    } else if (!strcmp(op, "tmf8829_config_get") || !strcmp(op, "tmf8829_config_set")) {
        uint8_t *pg; size_t pn = hex_decode(json_as_str(json_obj_get(in, "page")), &pg);
        const char *field = json_as_str(json_obj_get(in, "field"));
        int rc;
        uint32_t v = 0;
        if (!strcmp(op, "tmf8829_config_get")) rc = depz_tmf8829_cfg_get_n(pg, pn, field, &v);
        else rc = depz_tmf8829_cfg_set_n(pg, pn, field, json_as_int(json_obj_get(in, "value")));
        free(pg);
        if (rc != 0) return true;
        CHECK(0, "%s: accepted, the case expects a refusal", label);
        return false;
    } else if (!strcmp(op, "vl53l4_encode") || !strcmp(op, "vl53lx_encode")) {
        /* contract 12 §2: the VL53LX bridge shares contract 10's READ_REG / WRITE_REG codecs */
        if (!strcmp(kind, "read_reg")) {
            uint16_t a, l; IR_U16(a, "addr"); IR_U16(l, "len");
            n = depz_vl53l4_pack_read_reg(a, l, out);
        } else if (!strcmp(kind, "xshut")) {
            uint8_t a; IR_U8(a, "action");
            n = depz_vl53l4_pack_xshut(a, out);
        } else if (!strcmp(kind, "write_reg")) {
            uint16_t a; uint8_t *d; size_t dn;
            IR_U16(a, "addr");
            dn = hex_decode(json_as_str(json_obj_get(in, "data")), &d);
            uint8_t *big = (uint8_t *)malloc(dn + 2);
            n = depz_vl53l4_pack_write_reg(a, d, dn, big);
            free(d);
            if (n != 0) check_payload(label, big, n, json_as_str(json_obj_get(e, "payload")));
            free(big);
            return n == 0;
        } else if (!strcmp(kind, "start_stream")) {
            uint16_t a, l; uint8_t fl;
            depz_vl53lx_clear_step st[8];
            const json_value *cl = json_obj_get(in, "clear");
            size_t cn = json_arr_size(cl);
            IR_U16(a, "addr"); IR_U16(l, "len"); IR_U8(fl, "flags");
            for (size_t k = 0; k < cn && k < 8; k++) {
                const json_value *pr = json_arr_get(cl, k);
                const json_value *sa = json_arr_get(pr, 0), *sv = json_arr_get(pr, 1);
                if (!sa || !sv || sa->kind != JSON_INT || sv->kind != JSON_INT || sa->range || sv->range ||
                    sa->i < 0 || sa->i > 0xFFFF || sv->i < 0 || sv->i > 0xFF)
                    return true;
                st[k].addr = (uint16_t)sa->i;
                st[k].value = (uint8_t)sv->i;
            }
            n = depz_vl53lx_pack_start_stream(a, l, fl, st, cn, out);
        } else {
            CHECK(0, "%s: no input_rules handler for %s kind %s", label, op, kind);
            return false;
        }
    } else if (!strcmp(op, "vl53l4_result_block")) {
        uint8_t *raw; size_t rn = hex_decode(json_as_str(json_obj_get(in, "raw")), &raw);
        depz_vl53l4_result r;
        int rc = depz_vl53l4_parse_result_block(raw, rn, &r);
        free(raw);
        if (rc != 0) return true;
        CHECK((int64_t)r.range_status == ex_int(e, "range_status") && (int64_t)r.distance_mm == ex_int(e, "distance_mm") &&
                  (int64_t)r.ambient_rate_kcps == ex_int(e, "ambient_rate_kcps") &&
                  (int64_t)r.ambient_per_spad_kcps == ex_int(e, "ambient_per_spad_kcps") &&
                  (int64_t)r.signal_rate_kcps == ex_int(e, "signal_rate_kcps") &&
                  (int64_t)r.signal_per_spad_kcps == ex_int(e, "signal_per_spad_kcps") &&
                  (int64_t)r.number_of_spad == ex_int(e, "number_of_spad") && (int64_t)r.sigma_mm == ex_int(e, "sigma_mm") &&
                  (int64_t)r.stream_count == ex_int(e, "stream_count"),
              "%s: result block fields", label);
        return false;
    } else if (!strcmp(op, "vl53l4_timing_encode")) {
        uint32_t b, im; uint16_t osc, pll, ra, rb; uint32_t raw;
        IR_U32(b, "timing_budget_ms"); IR_U32(im, "inter_measurement_ms");
        IR_U16(osc, "osc_frequency"); IR_U16(pll, "clock_pll");
        if (depz_vl53l4_range_timing_registers(b, im, osc, pll, &ra, &rb, &raw) != 0) return true;
        CHECK(0, "%s: accepted, the case expects a refusal", label);
        return false;
    } else if (!strcmp(op, "vl53l4_tuning_encode")) {
        uint16_t raw = 0;
        int rc = -1;
        if (!strcmp(kind, "offset")) { int32_t v; IR_I32(v, "value"); rc = depz_vl53l4_offset_raw_checked(v, &raw); }
        else if (!strcmp(kind, "xtalk")) { uint16_t v; IR_U16(v, "value"); rc = depz_vl53l4_xtalk_raw_checked(v, &raw); }
        else if (!strcmp(kind, "signal_threshold")) { uint16_t v; IR_U16(v, "value"); raw = depz_vl53l4_signal_threshold_raw(v); rc = 0; }
        else if (!strcmp(kind, "sigma_threshold")) { uint16_t v; IR_U16(v, "value"); rc = depz_vl53l4_sigma_threshold_raw(v, &raw); }
        if (rc != 0) return true;
        CHECK(raw == ex_int(e, "raw"), "%s: raw %u", label, raw);
        return false;
    } else if (!strcmp(op, "vl53l4_tuning_decode")) {
        uint16_t raw; IR_U16(raw, "raw");
        (void)raw;
        CHECK(0, "%s: a u16 raw word decodes; the case expects a refusal", label);
        return false;
    } else if (!strcmp(op, "vl53l8_encode")) {
        uint16_t fs;
        if (strcmp(kind, "start_stream") != 0) { CHECK(0, "%s: unknown vl53l8 encode kind %s", label, kind); return false; }
        IR_U16(fs, "frame_size");
        n = depz_vl53l8_pack_start_stream(fs, out);
    } else if (!strcmp(op, "vl53l8_decode")) {
        uint8_t *p; size_t pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        depz_vl53l8_chunk ch = {0};
        int rc = json_as_int(json_obj_get(in, "report")) == DEPZ_VL53L8_RPT_FRAME ?
                 depz_vl53l8_unpack_chunk(p, pn, &ch) : -1;
        if (rc == 0) {
            uint8_t *want; size_t wn = hex_decode(json_as_str(json_obj_get(e, "data")), &want);
            CHECK((int64_t)ch.timestamp_us == ex_int(e, "timestamp_us") && ch.full_size == ex_int(e, "full_size") &&
                  ch.offset == ex_int(e, "offset") && ch.data_len == wn && memcmp(ch.data, want, wn) == 0,
                  "%s: fields", label);
            free(want);
        }
        free(p);
        return rc != 0;
    } else if (!strcmp(op, "vl53l7_encode")) {
        if (!strcmp(kind, "pin_ctrl")) { uint8_t a; IR_U8(a, "action"); n = depz_vl53l7_pack_pin_ctrl(a, out); }
        else if (!strcmp(kind, "read_reg")) {
            uint16_t a, l; IR_U16(a, "addr"); IR_U16(l, "len");
            n = depz_vl53l7_pack_read_reg(a, l, out);
        } else if (!strcmp(kind, "write_reg")) {
            uint16_t a; uint8_t *d; size_t dn;
            IR_U16(a, "addr");
            dn = hex_decode(json_as_str(json_obj_get(in, "data")), &d);
            uint8_t *big = (uint8_t *)malloc(dn + 2);
            n = depz_vl53l7_pack_write_reg(a, d, dn, big);
            free(d);
            if (n != 0) check_payload(label, big, n, json_as_str(json_obj_get(e, "payload")));
            free(big);
            return n == 0;
        }
    } else if (!strcmp(op, "bno086_shtp_reassembly")) {
        const json_value *fr = json_obj_get(in, "frames"), *cap = json_obj_get(in, "captures_us");
        depz_shtp_layer l;
        bool rej = false;
        if (json_arr_size(fr) != json_arr_size(cap)) return true;
        depz_shtp_init(&l);
        for (size_t k = 0; k < json_arr_size(fr) && !rej; k++) {
            const json_value *t = json_arr_get(cap, k);
            uint8_t *f; size_t fn;
            depz_shtp_cargo cg;
            /* capture_us is the API's uint64_t */
            if (!t || t->kind != JSON_INT || t->range == 2 || (t->range == 0 && t->i < 0)) { rej = true; break; }
            fn = hex_decode(json_as_str(json_arr_get(fr, k)), &f);
            rej = depz_shtp_feed_ex(&l, f, fn, t->range == 1 ? t->u : (uint64_t)t->i, &cg) < 0;
            free(f);
        }
        depz_shtp_free(&l);
        if (!rej) CHECK(0, "%s: accepted, the case expects a refusal", label);
        return rej;
    } else if (!strcmp(op, "bno086_input_cargo") || !strcmp(op, "bno086_gyro_rv")) {
        /* capture_timestamp_us is the API's uint64_t: a negative one is unrepresentable */
        const json_value *t = json_obj_get(in, "capture_timestamp_us");
        bool rej = !t || t->kind != JSON_INT || t->range == 2 || (t->range == 0 && t->i < 0);
        if (!rej) CHECK(0, "%s: accepted, the case expects a refusal", label);
        return rej;
    } else if (!strcmp(op, "vl53l8_detection_thresholds")) {
        const json_value *tl = json_obj_get(in, "thresholds");
        size_t tn = json_arr_size(tl);
        depz_vl53l8_threshold th[DEPZ_VL53L8_NB_THRESHOLDS];
        uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
        if (tn > DEPZ_VL53L8_NB_THRESHOLDS) {
            /* the checked packer refuses a count past 64 before reading any entry */
            CHECK(depz_vl53l8_pack_thresholds_checked(th, tn, start, valid) != 0, "%s: %zu entries packed", label, tn);
            return true;
        }
        for (size_t k = 0; k < tn; k++) {
            const json_value *t = json_arr_get(tl, k);
            int64_t lo, hi, me, ty, zn, opn;
            if (!ir_int(t, "low_thresh", INT32_MIN, INT32_MAX, &lo) || !ir_int(t, "high_thresh", INT32_MIN, INT32_MAX, &hi) ||
                !ir_int(t, "measurement", 0, 0xFF, &me) || !ir_int(t, "type", 0, 0xFF, &ty) ||
                !ir_int(t, "zone_num", 0, 0xFF, &zn) || !ir_int(t, "operation", 0, 0xFF, &opn))
                return true;
            th[k].low_thresh = (int32_t)lo; th[k].high_thresh = (int32_t)hi;
            th[k].measurement = (uint8_t)me; th[k].type = (uint8_t)ty;
            th[k].zone_num = (uint8_t)zn; th[k].operation = (uint8_t)opn;
        }
        if (depz_vl53l8_pack_thresholds_checked(th, tn, start, valid) != 0) return true;
        CHECK(0, "%s: accepted, the case expects a refusal", label);
        return false;
    } else if (!strcmp(op, "vl53l8_xtalk_margin")) {
        int64_t kcps;
        uint32_t raw;
        IR_U32(kcps, "kcps"); /* a fraction or a negative: not a uint32_t */
        if (depz_vl53l8_xtalk_margin_to_raw((uint32_t)kcps, &raw) != 0) return true;
        CHECK((int64_t)raw == ex_int(e, "raw"), "%s: raw %u", label, raw);
        return false;
    } else if (!strcmp(op, "usb_identity")) {
        int vid, pid;
        bool known;
        const char *model;
        IR_I32(vid, "vid"); IR_I32(pid, "pid");
        if (depz_usb_identity(vid, pid, &known, &model) != 0) return true;
        CHECK(known == json_as_bool(json_obj_get(e, "known")), "%s: known", label);
        return false;
    } else if (!strcmp(op, "bno055_units")) {
        uint8_t v; depz_bno055_units u;
        IR_U8(v, "unit_sel");
        depz_bno055_unpack_units(v, &u);
        CHECK(0, "%s: a byte unpacks; the case expects a refusal", label);
        return false;
    } else if (!strcmp(op, "bno055_decode") || !strcmp(op, "vl53l4_decode")) {
        int rpt = (int)json_as_int(json_obj_get(in, "report"));
        uint8_t *p; size_t pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        int rc = -1;
        int64_t ts = 0, addr = 0, len = 0;
        const uint8_t *data = NULL;
        if (!strcmp(op, "bno055_decode") && rpt == DEPZ_BNO055_RPT_STREAM) {
            depz_bno055_stream st;
            rc = depz_bno055_unpack_stream(p, pn, &st);
            if (rc == 0) { ts = (int64_t)st.timestamp_us; addr = st.addr; len = st.len; data = st.data; }
        } else if (!strcmp(op, "vl53l4_decode") && rpt == DEPZ_VL53L4_RPT_STREAM) {
            depz_vl53l4_stream st;
            rc = depz_vl53l4_unpack_stream(p, pn, &st);
            if (rc == 0) { ts = (int64_t)st.timestamp_us; addr = st.addr; len = st.len; data = st.data; }
        } else {
            CHECK(0, "%s: no input_rules handler for report 0x%02x", label, rpt);
        }
        if (rc == 0) {
            CHECK(ts == ex_int(e, "timestamp_us") && addr == ex_int(e, "addr") && len == ex_int(e, "len"),
                  "%s: fields", label);
            check_payload(label, data, (size_t)len, json_as_str(json_obj_get(e, "data")));
        }
        free(p);
        return rc != 0;
    } else if (!strcmp(op, "vl53l4_timing_decode")) {
        uint32_t im, b, it; uint16_t pll, osc, ra;
        IR_U32(im, "intermeasurement_raw"); IR_U16(pll, "clock_pll"); IR_U16(osc, "osc_frequency");
        IR_U16(ra, "range_config_a");
        if (depz_vl53l4_decode_range_timing(im, pll, osc, ra, &b, &it) != 0) return true;
        CHECK(b == ex_int(e, "timing_budget_ms") && it == ex_int(e, "inter_measurement_ms"), "%s: fields", label);
        return false;
    } else if (!strcmp(op, "vl53l8_cnh_decode")) {
#if DEPZ_VECTORS_HAS_IO
        const json_value *c = json_obj_get(in, "config"), *am = json_obj_get(c, "agg_map");
        depz_vl53l8_cnh_setup setup;
        int m[6];
        for (int k = 0; k < 6; k++) m[k] = (int)json_as_int(json_arr_get(am, (size_t)k));
        if (depz_vl53l8_cnh_init_config_checked(&setup, (int)json_as_int(json_obj_get(c, "start_bin")),
                                                (int)json_as_int(json_obj_get(c, "num_bins")),
                                                (int)json_as_int(json_obj_get(c, "sub_sample"))) != 0)
            return true;
        if (depz_vl53l8_cnh_create_agg_map(&setup, (int)json_as_int(json_obj_get(c, "resolution")),
                                           m[0], m[1], m[2], m[3], m[4], m[5]) != 0)
            return true;
        CHECK(0, "%s: accepted, the case expects a refusal", label);
        return false;
#else
        CHECK(0, "%s: built without the live-hardware layer (CNH setup helpers)", label);
        return false;
#endif
    } else if (!strcmp(op, "common_event")) {
#if DEPZ_VECTORS_HAS_IO
        /* the unsolicited path: the report goes through the device dispatcher */
        uint8_t rpt;
        uint8_t *p; size_t pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        depz_device_event ev;
        const char *want = json_as_str(json_obj_get(e, "event"));
        int rc;
        IR_U8(rpt, "report");
        rc = unsolicited_event(rpt, p, pn, &ev);
        free(p);
        CHECK(rc == DEPZ_OK, "%s: the device emitted no event (rc=%d)", label, rc);
        if (rc != DEPZ_OK) return false;
        if (ev.type == DEPZ_DEV_EV_TEMPERATURE) {
            CHECK(!strcmp(want, "temperature") && (int64_t)ev.timestamp_us == ex_int(e, "timestamp_us") &&
                  ev.celsius == ex_dbl(e, "celsius"), "%s: temperature event fields", label);
        } else if (ev.type == DEPZ_DEV_EV_SEQUENCE_ERROR) {
            CHECK(!strcmp(want, "sequence_error") && ev.expected_seq == ex_int(e, "expected_seq") &&
                  ev.received_seq == ex_int(e, "received_seq") &&
                  ev.by_device == json_as_bool(json_obj_get(e, "reported_by_device")),
                  "%s: sequence_error event fields", label);
        } else if (ev.type == DEPZ_DEV_EV_TEXT) {
            CHECK(!strcmp(want, "text") && ev.cmd == ex_int(e, "cmd") &&
                  !strcmp(ev.text, json_as_str(json_obj_get(e, "text"))), "%s: text event fields", label);
        } else if (ev.type == DEPZ_DEV_EV_UNSOLICITED_STATUS) {
            CHECK(!strcmp(want, "unsolicited_status") && ev.status == ex_int(e, "status"),
                  "%s: unsolicited_status event fields", label);
        } else {
            CHECK(0, "%s: event type %d, expected %s", label, (int)ev.type, want);
        }
        return false;
#else
        CHECK(0, "%s: built without the live-hardware layer (the device dispatcher)", label);
        return false;
#endif
    } else if (!strcmp(op, "common_seq_gap")) {
#if DEPZ_VECTORS_HAS_IO
        /* host-observed gaps in the device->host seq (ERRATA A1) */
        const json_value *jseqs = json_obj_get(in, "seqs"), *wgaps = json_obj_get(e, "gaps");
        uint8_t seqs[64];
        depz_device_event gaps[64];
        size_t i, n_seqs = json_arr_size(jseqs), n_gaps;
        uint64_t seq_gaps;
        int rc;
        CHECK(n_seqs <= 64, "%s: at most 64 seqs", label);
        if (n_seqs > 64) return false;
        for (i = 0; i < n_seqs; i++) {
            int64_t v = json_as_int(json_arr_get(jseqs, i));
            if (v < 0 || v > 0xFF) return true; /* a seq is a byte */
            seqs[i] = (uint8_t)v;
        }
        rc = seq_gap_events(seqs, n_seqs, gaps, 64, &n_gaps, &seq_gaps);
        CHECK(rc == DEPZ_OK, "%s: not every packet produced its event (rc=%d)", label, rc);
        if (rc != DEPZ_OK) return false;
        CHECK(n_gaps == json_arr_size(wgaps), "%s: %zu seq_gap events, want %zu", label, n_gaps,
              json_arr_size(wgaps));
        for (i = 0; i < n_gaps && i < json_arr_size(wgaps); i++) {
            const json_value *w = json_arr_get(wgaps, i);
            CHECK(gaps[i].expected_seq == ex_int(w, "expected_seq") &&
                  gaps[i].received_seq == ex_int(w, "received_seq") && gaps[i].missed == ex_int(w, "missed"),
                  "%s: seq_gap event %zu fields", label, i);
        }
        CHECK((int64_t)seq_gaps == ex_int(e, "seq_gaps"), "%s: stats.seq_gaps %llu", label,
              (unsigned long long)seq_gaps);
        return false;
#else
        CHECK(0, "%s: built without the live-hardware layer (the device dispatcher)", label);
        return false;
#endif
    } else if (!strcmp(op, "bno086_control_decode")) {
        /* a control response shorter than its fields is refused (contract 05 §4 table) */
        uint8_t *p; size_t pn = hex_decode(json_as_str(json_obj_get(in, "payload")), &p);
        bool rej = true;
        if (!strcmp(kind, "feature_response")) {
            depz_bno_feature f;
            rej = depz_bno_unpack_feature_response(p, pn, &f) != 0;
            if (!rej) CHECK(f.sensor_id == ex_int(e, "sensor_id") && f.flags == ex_int(e, "flags") &&
                            f.sensitivity == ex_int(e, "sensitivity") && f.interval_us == ex_int(e, "interval_us") &&
                            f.batch_us == ex_int(e, "batch_us") && f.cfg_word == ex_int(e, "cfg_word"),
                            "%s: fields", label);
        } else if (!strcmp(kind, "product_id")) {
            depz_bno_product_id r;
            rej = depz_bno_unpack_product_id(p, pn, &r) != 0;
            if (!rej) CHECK(r.reset_cause == ex_int(e, "reset_cause") && r.sw_version_major == ex_int(e, "sw_version_major") &&
                            r.sw_version_minor == ex_int(e, "sw_version_minor") &&
                            r.sw_part_number == ex_int(e, "sw_part_number") &&
                            r.sw_build_number == ex_int(e, "sw_build_number") &&
                            r.sw_version_patch == ex_int(e, "sw_version_patch"), "%s: fields", label);
        } else if (!strcmp(kind, "command_response")) {
            depz_bno_command_response r;
            rej = depz_bno_unpack_command_response(p, pn, &r) != 0;
            if (!rej) {
                const json_value *rr = json_obj_get(e, "r");
                bool same = json_arr_size(rr) == 11;
                for (size_t k = 0; same && k < 11; k++) same = r.r[k] == json_as_int(json_arr_get(rr, k));
                CHECK(same && r.seq == ex_int(e, "seq") && r.command == ex_int(e, "command") &&
                      r.command_seq == ex_int(e, "command_seq") && r.response_seq == ex_int(e, "response_seq"),
                      "%s: fields", label);
            }
        } else if (!strcmp(kind, "frs_read_response")) {
            depz_bno_frs_read_response r;
            rej = depz_bno_unpack_frs_read_response(p, pn, &r) != 0;
            if (!rej) CHECK(r.status == ex_int(e, "status") && r.data_length == ex_int(e, "data_length") &&
                            r.offset_words == ex_int(e, "offset_words") && r.data0 == ex_int(e, "data0") &&
                            r.data1 == ex_int(e, "data1") && r.frs_type == ex_int(e, "frs_type"), "%s: fields", label);
        } else if (!strcmp(kind, "frs_write_response")) {
            depz_bno_frs_write_response r;
            rej = depz_bno_unpack_frs_write_response(p, pn, &r) != 0;
            if (!rej) CHECK(r.status == ex_int(e, "status") && r.offset_words == ex_int(e, "offset_words"),
                            "%s: fields", label);
        } else {
            CHECK(0, "%s: no input_rules handler for %s kind %s", label, op, kind);
        }
        free(p);
        return rej;
    } else if (!strcmp(op, "depzrec_read")) {
#if DEPZ_VECTORS_HAS_IO
        /* a damaged capture is refused whole, as a bad argument naming the line */
        const json_value *tv = json_obj_get(in, "text");
        const char *text = json_as_str(tv);   /* slen, not strlen: the text may hold a NUL */
        uint8_t *rx;
        size_t rx_len;
        int rc = replay_all_rx(text ? text : "", text ? tv->slen : 0, &rx, &rx_len, label);
        if (rc != DEPZ_OK) {
            CHECK(rc == DEPZ_E_ARG, "%s: refused with %s, expected DEPZ_E_ARG", label, depz_err_str(rc));
            CHECK(depz_last_error()[0] != '\0', "%s: a refusal without a message", label);
            return true;
        }
        check_payload(label, rx, rx_len, json_as_str(json_obj_get(e, "rx")));
        free(rx);
        return false;
#else
        CHECK(0, "%s: built without the live-hardware layer (the replay link)", label);
        return false;
#endif
    } else if (!strcmp(op, "depzdata_read")) {
        /* a damaged dataset is refused whole, as a bad argument naming the line */
        const json_value *tv = json_obj_get(in, "text");
        const char *text = json_as_str(tv);   /* slen, not strlen: the text may hold a NUL */
        depz_dataset *ds = depz_dataset_parse(text ? text : "", text ? tv->slen : 0);
        const json_value *want = json_obj_get(e, "records");
        size_t count, k;
        if (!ds) {
            CHECK(depz_last_error()[0] != '\0', "%s: a refusal without a message", label);
            return true;
        }
        count = depz_dataset_record_count(ds);
        CHECK(count == json_arr_size(want), "%s: %zu records, expected %zu", label, count, json_arr_size(want));
        for (k = 0; k < count && k < json_arr_size(want); k++) {
            const json_value *w = json_arr_get(want, k);
            const char *wd = json_as_str(json_arr_get(w, 0)), *wk = json_as_str(json_arr_get(w, 2));
            depz_dataset_record r;
            depz_dataset_get_record(ds, k, &r);
            CHECK(wd && wk && !strcmp(r.device_id, wd) && r.t_host_us == json_as_int(json_arr_get(w, 1)) &&
                  !strcmp(r.kind, wk), "%s: record %zu is %s/%lld/%s", label, k, r.device_id,
                  (long long)r.t_host_us, r.kind);
        }
        depz_dataset_free(ds);
        return false;
    } else {
        CHECK(0, "%s: no input_rules handler for op %s", label, op);
        return false;
    }
    if (n == 0) return true;
    check_payload(label, out, n, json_as_str(json_obj_get(e, "payload")));
    return false;
}

/* Every case of a file's `input_rules` list: a rejection where the case
 * expects one, else the decoded / encoded value. */
static void run_input_rules(const json_value *root)
{
    const json_value *list = json_obj_get(root, "input_rules");
    for (size_t i = 0; i < json_arr_size(list); i++) {
        const json_value *c = json_arr_get(list, i);
        const int f0 = g_fail;
        const char *id = json_as_str(json_obj_get(c, "id"));
        const json_value *e = json_obj_get(c, "expect");
        const bool want_invalid = json_obj_get(e, "error") != NULL;
        const bool rejected = ir_run(id, json_as_str(json_obj_get(c, "op")), json_obj_get(c, "in"), e);
        CHECK(rejected == want_invalid, "%s: %s, expected %s", id, rejected ? "rejected" : "accepted",
              want_invalid ? "a rejection" : "a decode");
        seen_case(c, f0);
    }
}

static void test_input_rules(const char *stem)
{
    json_value *root = load_vectors(stem);
    if (!root) { g_fail++; return; }
    run_input_rules(root);
    json_free(root);
}

/* The reader-rule files (contracts 08, 09: the .depzrec replay and the
 * .depzdata dataset on damaged files) carry only an input_rules group. */
static void test_reader_rules(const char *file)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", DEPZ_VECTORS_DIR, file);
    char *text = read_file(path);
    json_value *root = text ? json_parse(text) : NULL;
    free(text);
    if (!root) { fprintf(stderr, "JSON parse failed for %s\n", path); g_fail++; return; }
    run_input_rules(root);
    json_free(root);
}

/* ======================================================================== */
int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <vector-stem>\n", argv[0]); return 2; }
    const char *stem = argv[1];

    if      (strcmp(stem, "crc") == 0)             test_crc();
    else if (strcmp(stem, "usb_ids") == 0)         test_usb_ids();
    else if (strcmp(stem, "identity") == 0)        test_identity();
    else if (strcmp(stem, "probe") == 0)           test_probe();
    else if (strcmp(stem, "common_commands") == 0) test_common_commands();
    else if (strcmp(stem, "sr04") == 0)            test_sr04();
    else if (strcmp(stem, "framing_encode") == 0)  test_framing_encode();
    else if (strcmp(stem, "framing_decode") == 0)  test_framing_decode();
    else if (strcmp(stem, "fwdepz") == 0)          test_fwdepz();
    else if (strcmp(stem, "vl53l8_advanced") == 0) test_vl53l8_advanced();
    else if (strcmp(stem, "vl53l4") == 0)          test_vl53l4();
    else if (strcmp(stem, "bno086_shtp") == 0)     test_bno086_shtp();
    else if (strcmp(stem, "bno086_reports") == 0)  test_bno086_reports();
    else if (strcmp(stem, "vl53l8_replay") == 0)   test_vl53l8_replay();
    else if (strcmp(stem, "vl53l8_cnh") == 0)      test_vl53l8_cnh();
    else if (strcmp(stem, "dataset") == 0)         test_dataset();
    else if (strcmp(stem, "vl53l8") == 0)          test_vl53l8();
    else if (strcmp(stem, "vl53l7") == 0)          test_vl53l7();
    else if (strcmp(stem, "vl53l7_replay") == 0)   test_vl53l7_replay(argc > 2 ? argv[2] : NULL);
    else if (strcmp(stem, "vl53lx") == 0)          test_vl53lx();
    else if (strcmp(stem, "bno055") == 0)          test_bno055();
    else if (strcmp(stem, "tmf8829") == 0)         test_tmf8829();
    else if (strcmp(stem, "tmf8829_frame_time") == 0) test_tmf8829_frame_time();
    else if (strcmp(stem, "depzrec") == 0)         test_reader_rules("depzrec.json");
    else if (strcmp(stem, "depzdata") == 0)        test_reader_rules("depzdata.json");
    else { fprintf(stderr, "unknown vector stem: %s\n", stem); return 2; }
    {
        /* every vector file that has an input_rules group asserts it too */
        static const char *const WITH_RULES[] = {"common_commands", "sr04", "framing_encode", "usb_ids", "bno055",
                                                 "bno086_shtp", "tmf8829", "vl53l4", "vl53l7", "vl53lx",
                                                 "vl53l8", "vl53l8_advanced"};
        for (size_t k = 0; k < sizeof WITH_RULES / sizeof WITH_RULES[0]; k++)
            if (strcmp(stem, WITH_RULES[k]) == 0) test_input_rules(stem);
    }

    printf("[%s] %d checks, %d failures\n", stem, g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
