/* Golden-vector test runner for the DEPZ C SDK.
 *
 * Loads contracts/vectors/<file>.json and asserts byte-exact behaviour of the
 * SDK. Invoked once per vector file: argv[1] = file stem (e.g. "crc"). The
 * vectors directory is passed via -DDEPZ_VECTORS_DIR at compile time.
 */
#include "depz_sensor_sdk.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    }
    json_free(root);
}

/* ======================================================================== */
/* usb_ids.json                                                              */
/* ======================================================================== */

/* Serial-ordering comparator matching the vector rule: serial ascending,
 * None/empty last, tie-break by port path. */
typedef struct { const char *port; const char *serial; /* NULL = None */ } port_ent;

static int port_cmp(const void *pa, const void *pb)
{
    const port_ent *a = (const port_ent *)pa;
    const port_ent *b = (const port_ent *)pb;
    int a_empty = (a->serial == NULL || a->serial[0] == '\0');
    int b_empty = (b->serial == NULL || b->serial[0] == '\0');
    if (a_empty != b_empty)
        return a_empty - b_empty; /* empty sorts last */
    if (!a_empty) {
        int r = strcmp(a->serial, b->serial);
        if (r) return r;
    }
    return strcmp(a->port, b->port);
}

static void test_usb_ids(void)
{
    json_value *root = load_vectors("usb_ids");
    if (!root) { g_fail++; return; }

    const json_value *ident = json_obj_get(root, "identity");
    for (size_t i = 0; i < json_arr_size(ident); i++) {
        const json_value *c = json_arr_get(ident, i);
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
    }

    const json_value *ord = json_obj_get(root, "serial_ordering");
    for (size_t i = 0; i < json_arr_size(ord); i++) {
        const json_value *c = json_arr_get(ord, i);
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
    }
    json_free(root);
}

/* ======================================================================== */
/* common_commands.json                                                      */
/* ======================================================================== */
static void check_payload(const char *label, const uint8_t *got, size_t got_n, const char *want_hex)
{
    char buf[4096];
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
    }

    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
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
    }

    const json_value *st = json_obj_get(root, "sync_time_math");
    for (size_t i = 0; i < json_arr_size(st); i++) {
        const json_value *c = json_arr_get(st, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        int64_t off, rtt;
        depz_sync_time_offset_rtt(
            json_as_int(json_obj_get(c, "t1")), json_as_int(json_obj_get(c, "t2")),
            json_as_int(json_obj_get(c, "t3")), json_as_int(json_obj_get(c, "t4")), &off, &rtt);
        CHECK(off == json_as_int(json_obj_get(c, "offset_us")), "sync offset %s: got %lld", name, (long long)off);
        CHECK(rtt == json_as_int(json_obj_get(c, "rtt_us")), "sync rtt %s: got %lld", name, (long long)rtt);
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
        const char *kind = json_as_str(json_obj_get(c, "kind"));
        const char *want = json_as_str(json_obj_get(c, "payload"));
        uint8_t out[8]; size_t n = 0;
        if (strcmp(kind, "set_sample_period") == 0)
            n = depz_sr04_pack_sample_period((uint32_t)json_as_int(json_obj_get(c, "period_us")), out);
        else if (strcmp(kind, "set_echo_decay") == 0)
            n = depz_sr04_pack_echo_decay((uint16_t)json_as_int(json_obj_get(c, "decay_us")), out);
        else { CHECK(0, "unknown sr04 encode kind %s", kind); continue; }
        check_payload(json_as_str(json_obj_get(c, "name")), out, n, want);
    }

    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
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
    }
    json_free(root);
}

/* ======================================================================== */
/* fwdepz.json                                                               */
/* ======================================================================== */
static void test_fwdepz(void)
{
    json_value *root = load_vectors("fwdepz");
    if (!root) { g_fail++; return; }
    const json_value *cases = json_obj_get(root, "cases");
    for (size_t i = 0; i < json_arr_size(cases); i++) {
        const json_value *c = json_arr_get(cases, i);
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
                strcmp(want, "size") == 0 ? DEPZ_FWDEPZ_ERR_SIZE : DEPZ_FWDEPZ_ERR_TOO_SHORT;
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
    }
    json_free(root);
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
        const char *name = json_as_str(json_obj_get(c, "name"));
        int res = (int)json_as_int(json_obj_get(c, "resolution"));
        uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE];
        CHECK(depz_vl53l8_motion_cfg_default_pack(res, out) == 0, "motion pack rc %s", name);
        char hex[DEPZ_VL53L8_MOTION_CFG_SIZE * 2 + 1];
        hex_encode(out, sizeof(out), hex);
        check_payload(name, out, sizeof(out), json_as_str(json_obj_get(c, "pack")));
    }

    const json_value *thr = json_obj_get(root, "thresholds");
    for (size_t i = 0; i < json_arr_size(thr); i++) {
        const json_value *c = json_arr_get(thr, i);
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
    }

    const json_value *xm = json_obj_get(root, "xtalk_margin");
    for (size_t i = 0; i < json_arr_size(xm); i++) {
        const json_value *c = json_arr_get(xm, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        double kcps = (double)json_as_int(json_obj_get(c, "kcps"));
        uint32_t raw = depz_vl53l8_xtalk_margin_to_raw(kcps);
        CHECK((int64_t)raw == json_as_int(json_obj_get(c, "raw")),
              "xtalk margin %s: got %u", name, raw);
    }
    json_free(root);
}

/* ======================================================================== */
/* vl53l4.json — VL53L4CD wire codecs + host-ULD math (contract 10).         */
/* Consumes every section: encode / decode / result_block / timing.encode /  */
/* timing.decode / tuning (both directions) / config_block.                  */
/* ======================================================================== */
static void test_vl53l4(void)
{
    json_value *root = load_vectors("vl53l4");
    if (!root) { g_fail++; return; }

    /* encode: command payloads 0x32..0x38 */
    const json_value *enc = json_obj_get(root, "encode");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
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
    }

    /* decode: RPT_VL53_REG_DATA / _INFO / _STREAM */
    const json_value *dec = json_obj_get(root, "decode");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
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
    }

    /* result_block: 17-byte 0x0089 block -> VL53L4CD_ResultsData_t */
    const json_value *rb = json_obj_get(root, "result_block");
    for (size_t i = 0; i < json_arr_size(rb); i++) {
        const json_value *c = json_arr_get(rb, i);
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
    }

    /* timing.encode / timing.decode: SetRangeTiming / GetRangeTiming math */
    const json_value *timing = json_obj_get(root, "timing");
    const json_value *tenc = json_obj_get(timing, "encode");
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

    /* tuning: word codecs, both directions */
    const json_value *tun = json_obj_get(root, "tuning");
    for (size_t i = 0; i < json_arr_size(tun); i++) {
        const json_value *c = json_arr_get(tun, i);
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
    }

    /* config_block: the 91-byte init block with byte 0 forced to 0x12 */
    const json_value *cb = json_obj_get(root, "config_block");
    CHECK((int64_t)DEPZ_VL53L4_CONFIG_ADDR == json_as_int(json_obj_get(cb, "addr")),
          "vl53l4 config_block addr");
    {
        uint8_t out[91];
        size_t n = depz_vl53l4_config_block(out);
        CHECK(n == sizeof(out), "vl53l4 config_block length: got %zu", n);
        check_payload("config_block", out, n, json_as_str(json_obj_get(cb, "data")));
        CHECK(out[0] == DEPZ_VL53L4_CONFIG_FMP_BYTE, "vl53l4 config_block fmp byte");
    }

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
    }

    /* tx_seq: shared layer, per-channel counters */
    {
        depz_shtp_layer layer;
        depz_shtp_init(&layer);
        const json_value *tx = json_obj_get(root, "tx_seq");
        for (size_t i = 0; i < json_arr_size(tx); i++) {
            const json_value *c = json_arr_get(tx, i);
            uint8_t ch = (uint8_t)json_as_int(json_obj_get(c, "channel"));
            uint8_t *pl; size_t pn = hex_decode(json_as_str(json_obj_get(c, "payload")), &pl);
            uint8_t out[DEPZ_SHTP_MAX_TX_FRAME];
            size_t on = depz_shtp_next_frame(&layer, ch, pl, pn, out, sizeof(out));
            char label[32]; snprintf(label, sizeof(label), "tx_seq[%zu]", i);
            check_payload(label, out, on, json_as_str(json_obj_get(c, "frame")));
            free(pl);
        }
        depz_shtp_free(&layer);
    }

    /* reassembly */
    const json_value *reasm = json_obj_get(root, "reassembly");
    for (size_t i = 0; i < json_arr_size(reasm); i++) {
        const json_value *c = json_arr_get(reasm, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        depz_shtp_layer layer;
        depz_shtp_init(&layer);
        const json_value *frames = json_obj_get(c, "frames");
        const json_value *expect = json_obj_get(c, "expect");
        const json_value *cargos = json_obj_get(expect, "cargos");
        size_t got_cargo = 0;
        for (size_t j = 0; j < json_arr_size(frames); j++) {
            uint8_t *fr; size_t fn = hex_decode(json_as_str(json_arr_get(frames, j)), &fr);
            depz_shtp_cargo cargo;
            int rc = depz_shtp_feed(&layer, fr, fn, &cargo);
            if (rc == 1) {
                const json_value *ec = json_arr_get(cargos, got_cargo);
                CHECK(ec != NULL, "shtp %s: unexpected extra cargo", name);
                if (ec) {
                    CHECK(cargo.channel == json_as_int(json_obj_get(ec, "channel")) &&
                          cargo.seq == json_as_int(json_obj_get(ec, "seq")),
                          "shtp %s cargo %zu header", name, got_cargo);
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
    }

    /* control_encode */
    const json_value *ctl = json_obj_get(root, "control_encode");
    for (size_t i = 0; i < json_arr_size(ctl); i++) {
        const json_value *c = json_arr_get(ctl, i);
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

static void test_bno086_reports(void)
{
    json_value *root = load_vectors("bno086_reports");
    if (!root) { g_fail++; return; }

    const json_value *cargos = json_obj_get(root, "input_cargos");
    for (size_t i = 0; i < json_arr_size(cargos); i++) {
        const json_value *c = json_arr_get(cargos, i);
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
        }
        free(cargo);
    }

    const json_value *grv = json_obj_get(root, "gyro_rv");
    for (size_t i = 0; i < json_arr_size(grv); i++) {
        const json_value *c = json_arr_get(grv, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        uint64_t cap_ts = (uint64_t)json_as_int(json_obj_get(c, "capture_timestamp_us"));
        uint8_t *cargo; size_t cn = hex_decode(json_as_str(json_obj_get(c, "cargo")), &cargo);
        depz_bno_report r;
        CHECK(depz_bno_parse_gyro_rv(cargo, cn, cap_ts, &r) == 0, "gyro_rv %s parse", name);
        const json_value *e = json_obj_get(c, "expect");
        CHECK(strcmp(depz_bno_report_type_str(r.type), json_as_str(json_obj_get(e, "type"))) == 0,
              "gyro_rv %s type", name);
        check_report_fields(name, &r, json_obj_get(e, "fields"), false);
        free(cargo);
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

static void test_vl53l8_replay(void)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/recordings/vl53l8_8x8_15hz_3s.depzrec", DEPZ_VECTORS_DIR);
    char *rec = read_file(path);
    if (!rec) { g_fail++; return; }

    replay_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    depz_vl53l8_reasm_init(&ctx.reasm);
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
    }
    json_free(eroot);
    free(ctx.frames);
}

/* ======================================================================== */
/* dataset: recordings/dataset_dual_sr04.depzdata                            */
/* ======================================================================== */
static void test_dataset(void)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/recordings/dataset_dual_sr04.depzdata", DEPZ_VECTORS_DIR);
    char *text = read_file(path);
    if (!text) { g_fail++; return; }

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

    depz_dataset_free(ds);
    free(text);
}

/* ======================================================================== */
/* vl53l8 CNH: vl53l8_cnh.json -> per-aggregate integer histograms           */
/* Byte/value-exact CNH decode against a live-VL53L8CH golden capture.       */
/* ======================================================================== */
static void test_vl53l8_cnh(void)
{
    json_value *root = load_vectors("vl53l8_cnh");
    if (!root) { g_fail++; return; }

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

    free(out);
    free(raw);
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
    }

    /* model: PID model -> VL53L([57])(CX|CH) in the device name -> vl53l7cx */
    const json_value *model = json_obj_get(root, "model");
    CHECK(json_arr_size(model) > 0, "vl53l7 model cases present");
    for (size_t i = 0; i < json_arr_size(model); i++) {
        const json_value *c = json_arr_get(model, i);
        const char *name = json_as_str(json_obj_get(c, "name"));
        const json_value *um = json_obj_get(c, "usb_model");
        const char *usb_model = json_is_null(um) ? NULL : json_as_str(um);
        const char *dev = json_as_str(json_obj_get(c, "device_name"));
        const char *got = depz_vl53l7_model_str(depz_vl53l7_resolve_model(usb_model, dev));
        const char *want = json_as_str(json_obj_get(c, "expect"));
        CHECK(strcmp(got, want) == 0, "vl53l7 model %s: got %s want %s", name, got, want);
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

    l7_replay_ctx *ctx = (l7_replay_ctx *)calloc(1, sizeof(*ctx));
    ctx->l8ch = strncmp(stem, "vl53l8ch", 8) == 0;
    depz_vl53l8_reasm_init(&ctx->reasm);
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
/* l0x_raw / histogram_raw.                                                  */
/* ======================================================================== */
static void test_vl53lx(void)
{
    json_value *root = load_vectors("vl53lx");
    if (!root) { g_fail++; return; }

    /* encode: SET_ADDR_WIDTH, START_STREAM + clear list, contract-10 codecs */
    const json_value *enc = json_obj_get(root, "encode");
    CHECK(json_arr_size(enc) > 0, "vl53lx encode cases present");
    for (size_t i = 0; i < json_arr_size(enc); i++) {
        const json_value *c = json_arr_get(enc, i);
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
    }

    /* products: the table, in order, with the default driver's bridge params */
    const json_value *prods = json_obj_get(root, "products");
    CHECK(json_arr_size(prods) == DEPZ_VL53LX_PRODUCT_COUNT,
          "vl53lx products: %zu rows, table has %d", json_arr_size(prods),
          (int)DEPZ_VL53LX_PRODUCT_COUNT);
    for (size_t i = 0; i < json_arr_size(prods); i++) {
        const json_value *r = json_arr_get(prods, i);
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
    }
    CHECK(depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_NONE) == NULL &&
              depz_vl53lx_product_get(DEPZ_VL53LX_PRODUCT_COUNT) == NULL,
          "vl53lx product_get out of range -> NULL");

    /* model: PID model -> VL53L<part> in the device name -> generic class */
    const json_value *model = json_obj_get(root, "model");
    CHECK(json_arr_size(model) > 0, "vl53lx model cases present");
    for (size_t i = 0; i < json_arr_size(model); i++) {
        const json_value *c = json_arr_get(model, i);
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
    }

    /* l0x_raw: raw fields of the VL53L0X 12-byte 0x14 block */
    const json_value *l0x = json_obj_get(root, "l0x_raw");
    CHECK(json_arr_size(l0x) > 0, "vl53lx l0x_raw cases present");
    for (size_t i = 0; i < json_arr_size(l0x); i++) {
        const json_value *c = json_arr_get(l0x, i);
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
    }

    /* histogram_raw: status bytes + 24 bins (bin-23 MSB/LSB patch) */
    const json_value *hist = json_obj_get(root, "histogram_raw");
    CHECK(json_arr_size(hist) > 0, "vl53lx histogram_raw cases present");
    for (size_t i = 0; i < json_arr_size(hist); i++) {
        const json_value *c = json_arr_get(hist, i);
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
    }

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
    }

    /* decode: RPT_BNO_INFO 0x92, RPT_BNO_REG_DATA 0x91, RPT_BNO_REG_STREAM 0x93 */
    const json_value *dec = json_obj_get(root, "decode");
    CHECK(json_arr_size(dec) > 0, "bno055 decode cases present");
    for (size_t i = 0; i < json_arr_size(dec); i++) {
        const json_value *c = json_arr_get(dec, i);
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
    }

    /* units: UNIT_SEL flags, repack drops the undefined bits */
    const json_value *units = json_obj_get(root, "units");
    CHECK(json_arr_size(units) > 0, "bno055 units cases present");
    for (size_t i = 0; i < json_arr_size(units); i++) {
        const json_value *c = json_arr_get(units, i);
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
    }
    CHECK(DEPZ_BNO055_MAG_LSB == 16.0 && DEPZ_BNO055_QUAT_LSB == 16384.0 &&
              DEPZ_BNO055_FUSION_ACCEL_LSB == 100.0, "bno055 fixed LSBs");

    /* calib_stat: 2-bit fields, fully_calibrated, exact repack */
    const json_value *cs = json_obj_get(root, "calib_stat");
    CHECK(json_arr_size(cs) > 0, "bno055 calib_stat cases present");
    for (size_t i = 0; i < json_arr_size(cs); i++) {
        const json_value *c = json_arr_get(cs, i);
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
    }

    /* calibration_profile: 11 x i16 LE, byte-exact repack */
    const json_value *prof = json_obj_get(root, "calibration_profile");
    CHECK(json_arr_size(prof) > 0, "bno055 calibration_profile cases present");
    for (size_t i = 0; i < json_arr_size(prof); i++) {
        const json_value *c = json_arr_get(prof, i);
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
    }

    /* axis_remap: placements P0..P7, unpack / repack / placement lookup */
    const json_value *ar = json_obj_get(root, "axis_remap");
    CHECK(json_arr_size(ar) == 8, "bno055 axis_remap: %zu placements", json_arr_size(ar));
    for (size_t i = 0; i < json_arr_size(ar); i++) {
        const json_value *c = json_arr_get(ar, i);
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
        const char *name = json_as_str(json_obj_get(c, "name"));
        depz_bno055_axis_remap a = {
            (uint8_t)json_as_int(json_obj_get(c, "x")),
            (uint8_t)json_as_int(json_obj_get(c, "y")),
            (uint8_t)json_as_int(json_obj_get(c, "z")), false, false, false };
        uint8_t c2 = 0xAA, s2 = 0xBB;
        CHECK(depz_bno055_pack_axis_remap(&a, &c2, &s2) == -1 && c2 == 0xAA && s2 == 0xBB,
              "bno055 axis_remap_invalid %s refused", name);
    }
    {
        depz_bno055_axis_remap a = { 3, 1, 2, false, false, false };
        uint8_t c2, s2;
        CHECK(depz_bno055_pack_axis_remap(&a, &c2, &s2) == -1,
              "bno055 axis_remap refuses axis code 3");
    }

    /* sensor_config: page-1 ACC / GYR / MAG config codes */
    const json_value *sc = json_obj_get(root, "sensor_config");
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

    /* blocks: channel extraction from any register window */
    const json_value *blocks = json_obj_get(root, "blocks");
    CHECK(json_arr_size(blocks) > 0, "bno055 blocks cases present");
    for (size_t i = 0; i < json_arr_size(blocks); i++) {
        const json_value *c = json_arr_get(blocks, i);
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
int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <vector-stem>\n", argv[0]); return 2; }
    const char *stem = argv[1];

    if      (strcmp(stem, "crc") == 0)             test_crc();
    else if (strcmp(stem, "usb_ids") == 0)         test_usb_ids();
    else if (strcmp(stem, "identity") == 0)        test_identity();
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
    else if (strcmp(stem, "vl53l7") == 0)          test_vl53l7();
    else if (strcmp(stem, "vl53l7_replay") == 0)   test_vl53l7_replay(argc > 2 ? argv[2] : NULL);
    else if (strcmp(stem, "vl53lx") == 0)          test_vl53lx();
    else if (strcmp(stem, "bno055") == 0)          test_bno055();
    else { fprintf(stderr, "unknown vector stem: %s\n", stem); return 2; }

    printf("[%s] %d checks, %d failures\n", stem, g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
