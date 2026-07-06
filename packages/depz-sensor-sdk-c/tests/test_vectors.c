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
    else if (strcmp(stem, "bno086_shtp") == 0)     test_bno086_shtp();
    else if (strcmp(stem, "bno086_reports") == 0)  test_bno086_reports();
    else if (strcmp(stem, "vl53l8_replay") == 0)   test_vl53l8_replay();
    else if (strcmp(stem, "vl53l8_cnh") == 0)      test_vl53l8_cnh();
    else if (strcmp(stem, "dataset") == 0)         test_dataset();
    else { fprintf(stderr, "unknown vector stem: %s\n", stem); return 2; }

    printf("[%s] %d checks, %d failures\n", stem, g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
