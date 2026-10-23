/* Differential-testing adapter for the DEPZ C SDK (contracts/16_DIFFTEST.md).
 *
 * A long-lived process speaking JSON Lines on stdin/stdout:
 *   {"id": n, "op": "<op>", "in": {...}}  ->  {"id": n, "out": ...}
 *                                         or  {"id": n, "error": "invalid" | "unsupported"}
 * `invalid` = the SDK rejected the input (an error return); `unsupported` = this
 * SDK has no function for the op. Built as the CMake target `difftest_adapter`
 * of the test build; the driver (scripts/difftest.py) builds and starts it.
 * Output shapes are the golden-vector `expect` shapes the vector runner
 * (test_vectors.c) asserts field by field.
 *
 * Arguments: the C API's parameter types are the argument range a C caller can
 * pass at all. An input outside the parameter's C type (300 for a uint8_t, a
 * negative value for an unsigned, 2^64 anywhere) is answered `invalid` — the
 * call cannot be written — rather than silently wrapped into a different
 * argument. Inside the type, the SDK's own checks decide.
 *
 * Declared gaps (`unsupported`):
 *   usb_serial_ordering — the serial ordering is internal to depz_find_devices
 *                         (a static comparator in src/io/discovery.c); there is
 *                         no public function to call.
 *   bno086_record       — no contract 09 dataset recorder: the C SDK only reads
 *                         datasets (depz_dataset_*), it never builds a record.
 *   vl53l8_cnh_decode   — only when built without the live-hardware layer
 *                         (the CNH setup helpers live in depz_sensor_io.h).
 *
 * The vl53l8_* advanced ops: the C test tree has no fake VL53L8 DCI platform,
 * so they call the codecs the driver methods write with —
 * depz_vl53l8_motion_cfg_default_pack (motion_indicator_init: the resolution
 * check is in the codec), depz_vl53l8_pack_thresholds_checked (what
 * depz_vl53l8_set_detection_thresholds writes, with its checks) and
 * depz_vl53l8_xtalk_margin_to_raw_checked (the 0..10000 refusal
 * depz_vl53l8_set_xtalk_margin applies before the write).
 */
#include "depz_sensor_sdk.h"
#if DEPZ_DIFFTEST_HAS_IO
#include "depz_sensor_io.h"
#endif
#include "json.h"
#include "sha256.h"

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── per-request arena + error exit ─────────────────────────────────────── */

static void **g_allocs;
static size_t g_nallocs, g_capallocs;

static void *amalloc(size_t n)
{
    void *p = calloc(1, n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(3); }
    if (g_nallocs == g_capallocs) {
        g_capallocs = g_capallocs ? g_capallocs * 2 : 64;
        g_allocs = (void **)realloc(g_allocs, g_capallocs * sizeof(void *));
    }
    g_allocs[g_nallocs++] = p;
    return p;
}

static void arena_free(void)
{
    for (size_t i = 0; i < g_nallocs; i++) free(g_allocs[i]);
    g_nallocs = 0;
}

static jmp_buf g_jb;
static const char *g_err_kind;
static char g_err_detail[300];

static void fail(const char *kind, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err_detail, sizeof g_err_detail, fmt, ap);
    va_end(ap);
    g_err_kind = kind;
    longjmp(g_jb, 1);
}
#define INVALID(...) fail("invalid", __VA_ARGS__)

/* ── JSON output ─────────────────────────────────────────────────────────── */

static char *ob;
static size_t ob_len, ob_cap;
static int o_depth;
static bool o_first[256];
static bool o_after_key;

static void o_put(const char *s, size_t n)
{
    if (ob_len + n + 1 > ob_cap) {
        ob_cap = (ob_len + n + 1) * 2 + 4096;
        ob = (char *)realloc(ob, ob_cap);
    }
    memcpy(ob + ob_len, s, n);
    ob_len += n;
    ob[ob_len] = '\0';
}
static void o_puts(const char *s) { o_put(s, strlen(s)); }

static void o_reset(void) { ob_len = 0; o_depth = 0; o_after_key = false; o_put("", 0); }

static void o_sep(void)
{
    if (o_after_key) { o_after_key = false; return; }
    if (o_depth > 0) {
        if (!o_first[o_depth]) o_put(",", 1);
        o_first[o_depth] = false;
    }
}
static void o_obj(void) { o_sep(); o_put("{", 1); o_first[++o_depth] = true; }
static void o_end_obj(void) { o_depth--; o_put("}", 1); }
static void o_arr(void) { o_sep(); o_put("[", 1); o_first[++o_depth] = true; }
static void o_end_arr(void) { o_depth--; o_put("]", 1); }

static void o_u16esc(uint32_t u)
{
    static const char *H = "0123456789abcdef";
    char e[6] = {'\\', 'u', H[(u >> 12) & 15], H[(u >> 8) & 15], H[(u >> 4) & 15], H[u & 15]};
    o_put(e, 6);
}

/* A C string as JSON text. Its bytes are read as UTF-8 the way a WHATWG
 * TextDecoder reads them (each maximal invalid subsequence -> U+FFFD), the
 * same text the TS reference makes of the same bytes; everything non-ASCII
 * is written as \u escapes. */
static void o_strn_raw(const char *s, size_t n)
{
    size_t i = 0;
    o_put("\"", 1);
    while (i < n) {
        unsigned c = (unsigned char)s[i];
        if (c < 0x80) {
            if (c == '"' || c == '\\') { char e[2] = {'\\', (char)c}; o_put(e, 2); }
            else if (c < 0x20 || c == 0x7F) o_u16esc(c);
            else { char ch = (char)c; o_put(&ch, 1); }
            i++;
            continue;
        }
        uint32_t cp;
        int need, k;
        unsigned lo = 0x80, hi = 0xBF;
        if (c >= 0xC2 && c <= 0xDF) { need = 1; cp = c & 0x1F; }
        else if (c >= 0xE0 && c <= 0xEF) { need = 2; cp = c & 0x0F; if (c == 0xE0) lo = 0xA0; if (c == 0xED) hi = 0x9F; }
        else if (c >= 0xF0 && c <= 0xF4) { need = 3; cp = c & 0x07; if (c == 0xF0) lo = 0x90; if (c == 0xF4) hi = 0x8F; }
        else { o_u16esc(0xFFFD); i++; continue; }
        size_t j = i + 1;
        for (k = 0; k < need && j < n; k++, j++) {
            unsigned b = (unsigned char)s[j];
            if (b < lo || b > hi) break;
            lo = 0x80; hi = 0xBF;
            cp = (cp << 6) | (b & 0x3F);
        }
        i = j;
        if (k < need) { o_u16esc(0xFFFD); continue; }
        if (cp < 0x10000) o_u16esc(cp);
        else { cp -= 0x10000; o_u16esc(0xD800 | (cp >> 10)); o_u16esc(0xDC00 | (cp & 0x3FF)); }
    }
    o_put("\"", 1);
}
static void o_key(const char *k) { o_sep(); o_strn_raw(k, strlen(k)); o_put(":", 1); o_after_key = true; }
static void o_str(const char *s) { o_sep(); if (s) o_strn_raw(s, strlen(s)); else o_puts("null"); }
static void o_strl(const char *s, size_t n) { o_sep(); o_strn_raw(s, n); }
static void o_null(void) { o_sep(); o_puts("null"); }
static void o_bool(bool b) { o_sep(); o_puts(b ? "true" : "false"); }
static void o_i64(int64_t v) { char b[32]; o_sep(); snprintf(b, sizeof b, "%lld", (long long)v); o_puts(b); }
static void o_u64(uint64_t v) { char b[32]; o_sep(); snprintf(b, sizeof b, "%llu", (unsigned long long)v); o_puts(b); }
static void o_dbl(double v)
{
    char b[40];
    o_sep();
    if (isnan(v)) { o_puts("\"NaN\""); return; }
    if (isinf(v)) { o_puts(v > 0 ? "\"Infinity\"" : "\"-Infinity\""); return; }
    snprintf(b, sizeof b, "%.17g", v);
    o_puts(b);
}
static void o_hex(const uint8_t *p, size_t n)
{
    static const char *H = "0123456789abcdef";
    char *s = (char *)amalloc(2 * n + 1);
    for (size_t i = 0; i < n; i++) { s[2 * i] = H[p[i] >> 4]; s[2 * i + 1] = H[p[i] & 15]; }
    o_sep();
    o_put("\"", 1);
    o_put(s, 2 * n);
    o_put("\"", 1);
}

#define KI(k, v) (o_key(k), o_i64((int64_t)(v)))
#define KU(k, v) (o_key(k), o_u64((uint64_t)(v)))
#define KB(k, v) (o_key(k), o_bool((v) ? true : false))
#define KS(k, v) (o_key(k), o_str(v))
#define KH(k, p, n) (o_key(k), o_hex((p), (n)))

/* ── argument readers (invalid when absent, wrong kind or outside the type) */

static const json_value *field(const json_value *in, const char *k)
{
    const json_value *v = json_obj_get(in, k);
    if (!v) INVALID("missing field %s", k);
    return v;
}

/* An integer in [lo, hi] (both int64). */
static int64_t ival(const json_value *v, const char *k, int64_t lo, int64_t hi)
{
    if (!v || v->kind != JSON_INT) INVALID("%s: not an integer", k);
    if (v->range != 0 || v->i < lo || v->i > hi)
        INVALID("%s: outside the C parameter type", k);
    return v->i;
}
static int64_t iarg(const json_value *in, const char *k, int64_t lo, int64_t hi)
{
    return ival(field(in, k), k, lo, hi);
}
#define U8(in, k)  ((uint8_t)iarg(in, k, 0, 0xFF))
#define U16(in, k) ((uint16_t)iarg(in, k, 0, 0xFFFF))
#define U32(in, k) ((uint32_t)iarg(in, k, 0, 0xFFFFFFFFLL))
#define I32(in, k) ((int32_t)iarg(in, k, INT32_MIN, INT32_MAX))
#define I64(in, k) iarg(in, k, INT64_MIN, INT64_MAX)

static uint64_t uval(const json_value *v, const char *k)
{
    if (!v || v->kind != JSON_INT) INVALID("%s: not an integer", k);
    if (v->range == 1) return v->u;
    if (v->range != 0 || v->i < 0) INVALID("%s: outside uint64", k);
    return (uint64_t)v->i;
}
#define U64(in, k) uval(field(in, k), k)

static bool barg(const json_value *in, const char *k)
{
    const json_value *v = field(in, k);
    if (v->kind != JSON_BOOL) INVALID("%s: not a boolean", k);
    return v->b;
}

static double darg(const json_value *in, const char *k)
{
    const json_value *v = field(in, k);
    if (v->kind == JSON_DOUBLE) return v->d;
    if (v->kind != JSON_INT) INVALID("%s: not a number", k);
    if (v->range == 1) return (double)v->u;
    return (double)v->i; /* range 2 saturates: still far outside any API range */
}

static const char *sarg(const json_value *in, const char *k)
{
    const json_value *v = field(in, k);
    if (v->kind != JSON_STRING) INVALID("%s: not a string", k);
    if (strlen(v->s) != v->slen) INVALID("%s: holds a NUL, which a C string cannot carry", k);
    return v->s;
}
/* A string (may hold NULs) with its length, or JSON null (-> NULL, 0). */
static const char *slarg(const json_value *in, const char *k, size_t *len)
{
    const json_value *v = field(in, k);
    *len = 0;
    if (v->kind == JSON_NULL) return NULL;
    if (v->kind != JSON_STRING) INVALID("%s: not a string", k);
    *len = v->slen;
    return v->s;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
static uint8_t *hex_of(const json_value *v, const char *k, size_t *n)
{
    if (!v || v->kind != JSON_STRING) INVALID("%s: not a hex string", k);
    size_t hl = strlen(v->s);
    if (hl % 2) INVALID("%s: odd hex length", k);
    uint8_t *b = (uint8_t *)amalloc(hl / 2 + 1);
    for (size_t i = 0; i < hl / 2; i++) {
        int hi = hexval(v->s[2 * i]), lo = hexval(v->s[2 * i + 1]);
        if (hi < 0 || lo < 0) INVALID("%s: not a hex string", k);
        b[i] = (uint8_t)(hi << 4 | lo);
    }
    *n = hl / 2;
    return b;
}
#define HEX(in, k, np) hex_of(field(in, k), k, np)

static const json_value *larg(const json_value *in, const char *k)
{
    const json_value *v = field(in, k);
    if (v->kind != JSON_ARRAY) INVALID("%s: not a list", k);
    return v;
}

/* Scratch output buffer for encoders: far larger than any payload. */
static uint8_t *scratch(size_t need) { return (uint8_t *)amalloc(need + 70000); }

/* An encoder's size_t result: 0 = refused. */
static void payload_out(const uint8_t *p, size_t n)
{
    if (n == 0) INVALID("encoder refused the arguments");
    o_obj(); KH("payload", p, n); o_end_obj();
}

static bool is(const char *a, const char *b) { return a && strcmp(a, b) == 0; }

/* ── ops ─────────────────────────────────────────────────────────────────── */

static void op_crc(const json_value *in)
{
    size_t n; uint8_t *b = HEX(in, "input", &n);
    o_obj();
    KU("crc8_maxim", depz_crc8_maxim(b, n));
    KU("crc16_modbus", depz_crc16_modbus(b, n));
    KU("crc32_iso_hdlc", depz_crc32_iso_hdlc(b, n));
    KU("crc16_ccitt_false", depz_crc16_ccitt_false(b, n));
    o_end_obj();
}

static void op_framing_encode(const json_value *in)
{
    uint8_t cmd = U8(in, "cmd");
    /* `seq` is the API's unsigned int; the SDK takes it modulo 256 on the
     * wire (framing_encode seq_wraps_modulo_256) */
    unsigned seq = (unsigned)U32(in, "seq");
    int ct = (int)iarg(in, "crc_type", INT32_MIN, INT32_MAX);
    size_t pn; uint8_t *p = HEX(in, "payload", &pn);
    size_t cap = pn + 70000, on = 0;
    uint8_t *out = (uint8_t *)amalloc(cap);
    int rc = depz_build_packet(cmd, pn ? p : NULL, pn, seq, (depz_crc_type)ct, out, cap, &on);
    if (rc != 0) INVALID("depz_build_packet rc=%d", rc);
    o_obj(); KH("frame", out, on); o_end_obj();
}

typedef struct { uint8_t *trash; size_t len, cap; } trash_buf;

static void framing_cb(const depz_event *ev, void *user)
{
    trash_buf *t = (trash_buf *)user;
    if (ev->type == DEPZ_EV_PACKET) {
        o_obj(); KS("type", "packet"); KU("cmd", ev->cmd); KU("seq", ev->seq);
        KH("payload", ev->payload, ev->payload_len); o_end_obj();
    } else if (ev->type == DEPZ_EV_CRC_ERROR) {
        o_obj(); KS("type", "crc_error"); KU("cmd", ev->cmd); KU("seq", ev->seq); o_end_obj();
    } else {
        if (t->len + ev->trash_len > t->cap) {
            t->cap = (t->len + ev->trash_len) * 2 + 64;
            t->trash = (uint8_t *)realloc(t->trash, t->cap);
        }
        memcpy(t->trash + t->len, ev->trash, ev->trash_len);
        t->len += ev->trash_len;
    }
}

static void op_framing_decode(const json_value *in)
{
    const json_value *chunks = json_obj_get(in, "chunks");
    size_t nchunks = chunks ? json_arr_size(chunks) : 1;
    uint8_t **bufs = (uint8_t **)amalloc(sizeof(uint8_t *) * (nchunks + 1));
    size_t *lens = (size_t *)amalloc(sizeof(size_t) * (nchunks + 1));
    if (chunks) {
        if (chunks->kind != JSON_ARRAY) INVALID("chunks: not a list");
        for (size_t i = 0; i < nchunks; i++) bufs[i] = hex_of(json_arr_get(chunks, i), "chunks", &lens[i]);
    } else {
        bufs[0] = HEX(in, "stream", &lens[0]);
    }
    depz_parser p;
    trash_buf t = {NULL, 0, 0};
    depz_parser_init(&p);
    o_obj();
    o_key("events");
    o_arr();
    for (size_t i = 0; i < nchunks; i++) depz_parser_feed(&p, bufs[i], lens[i], framing_cb, &t);
    o_end_arr();
    KH("trash", t.trash ? t.trash : (const uint8_t *)"", t.len);
    KH("residue", p.buf ? p.buf : (const uint8_t *)"", p.len);
    KU("header_errors", p.header_errors);
    o_end_obj();
    free(t.trash);
    depz_parser_free(&p);
}

static void op_identity(const json_value *in)
{
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    /* depz_identity.software_name is a truncated copy: the name itself is
     * the stripped string (sized with depz_strip_device_string(.., 0)) */
    size_t cap = depz_strip_device_string(raw, n, NULL, 0) + 1;
    char *stripped = (char *)amalloc(cap);
    depz_identity id;
    size_t sl = depz_strip_device_string(raw, n, stripped, cap);
    depz_parse_software_name_n(stripped, sl, &id);
    o_obj();
    KS("mode", id.mode);
    KS("sensor_type", depz_sensor_type_str(id.sensor_type));
    o_key("software_name"); o_strl(stripped, sl);
    KS("version", id.version);
    o_end_obj();
}

static void op_usb_identity(const json_value *in)
{
    int vid = (int)iarg(in, "vid", INT32_MIN, INT32_MAX), pid = (int)iarg(in, "pid", INT32_MIN, INT32_MAX);
    bool known;
    const char *model;
    if (depz_usb_identity(vid, pid, &known, &model) != 0) INVALID("depz_usb_identity: not a u16 id");
    o_obj();
    KB("known", known);
    KS("model", model);
    o_end_obj();
}

static void op_probe_resolve(const json_value *in)
{
    const json_value *vv = field(in, "usb_vid"), *pv = field(in, "usb_pid");
    int vid = vv->kind == JSON_NULL ? 0 : (int)ival(vv, "usb_vid", INT32_MIN, INT32_MAX);
    int pid = pv->kind == JSON_NULL ? 0 : (int)ival(pv, "usb_pid", INT32_MIN, INT32_MAX);
    size_t sl, dl;
    const char *sw = slarg(in, "software_name", &sl), *dn = slarg(in, "device_name", &dl);
    if (!sw) INVALID("software_name: null");
    const char *cls = depz_resolve_sensor_class_n(sw, sl, dn, dl, vv->kind == JSON_NULL ? NULL : &vid,
                                                  pv->kind == JSON_NULL ? NULL : &pid);
    if (!cls) INVALID("no class");
    o_obj(); KS("class", cls); o_end_obj();
}

static void op_common_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(0);
    size_t n;
    if (is(kind, "sync_time_request")) {
        n = depz_pack_sync_time(U64(in, "pc_timestamp_us"), out);
    } else if (is(kind, "set_payload_crc_type")) {
        /* no codec check of its own: the enum is the argument's type */
        int64_t ct = iarg(in, "crc_type", 0, 0xFF);
        if (ct != DEPZ_CRC_NONE && ct != DEPZ_CRC8 && ct != DEPZ_CRC16 && ct != DEPZ_CRC32)
            INVALID("crc_type %lld is not a depz_crc_type", (long long)ct);
        n = depz_pack_set_payload_crc_type((uint8_t)ct, out);
    } else if (is(kind, "sync_pin_config")) {
        depz_sync_pin_config c = {U8(in, "pin"), U8(in, "mode"), U8(in, "polarity")};
        n = depz_pack_sync_pin_config(&c, out);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static void op_common_decode(const json_value *in)
{
    int64_t rpt = I64(in, "report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    if (rpt == DEPZ_RPT_STATUS) {
        depz_status_report r;
        if (depz_unpack_status(p, n, &r) != 0) INVALID("unpack_status");
        o_obj(); KU("cmd", r.cmd); KU("status", r.status); o_end_obj();
    } else if (rpt == DEPZ_RPT_TEXT) {
        uint8_t cmd;
        size_t cap = 3 * n + 1, tl;
        char *text = (char *)amalloc(cap);
        if (depz_unpack_text_ex(p, n, &cmd, text, cap, &tl) != 0) INVALID("unpack_text_ex");
        o_obj(); KU("cmd", cmd); o_key("text"); o_strl(text, tl); o_end_obj();
    } else if (rpt == DEPZ_RPT_TEMPERATURE) {
        depz_temperature_report r;
        if (depz_unpack_temperature(p, n, &r) != 0) INVALID("unpack_temperature");
        o_obj(); KU("timestamp_us", r.timestamp_us); KI("raw_decidegrees", r.raw_decidegrees); o_end_obj();
    } else if (rpt == DEPZ_RPT_SEQUENCE_ERROR) {
        depz_sequence_error_report r;
        if (depz_unpack_sequence_error(p, n, &r) != 0) INVALID("unpack_sequence_error");
        o_obj(); KU("expected_seq", r.expected_seq); KU("received_seq", r.received_seq); o_end_obj();
    } else {
        INVALID("report %lld", (long long)rpt);
    }
}

static void op_sync_time_math(const json_value *in)
{
    int64_t off, rtt;
    depz_sync_time_offset_rtt(I64(in, "t1"), I64(in, "t2"), I64(in, "t3"), I64(in, "t4"), &off, &rtt);
    o_obj(); KI("offset_us", off); KI("rtt_us", rtt); o_end_obj();
}

static void op_sr04_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(0);
    size_t n;
    if (is(kind, "set_sample_period")) n = depz_sr04_pack_sample_period(U32(in, "period_us"), out);
    else if (is(kind, "set_echo_decay")) n = depz_sr04_pack_echo_decay(U16(in, "decay_us"), out);
    else INVALID("kind %s", kind);
    payload_out(out, n);
}

static void op_sr04_decode(const json_value *in)
{
    int64_t rpt = I64(in, "report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    if (rpt == DEPZ_SR04_RPT_DATA) {
        depz_sr04_data d;
        if (depz_sr04_unpack_data(p, n, &d) != 0) INVALID("sr04_unpack_data");
        o_obj(); KU("source_cmd", d.source_cmd); KU("timestamp_us", d.timestamp_us);
        KU("echo_time_us", d.echo_time_us); o_end_obj();
    } else if (rpt == DEPZ_SR04_RPT_SAMPLE_PERIOD) {
        uint32_t v;
        if (depz_sr04_unpack_sample_period(p, n, &v) != 0) INVALID("sr04_unpack_sample_period");
        o_obj(); KU("period_us", v); o_end_obj();
    } else if (rpt == DEPZ_SR04_RPT_ECHO_DECAY) {
        uint16_t v;
        if (depz_sr04_unpack_echo_decay(p, n, &v) != 0) INVALID("sr04_unpack_echo_decay");
        o_obj(); KU("decay_us", v); o_end_obj();
    } else {
        INVALID("report %lld", (long long)rpt);
    }
}

static void op_fwdepz_parse(const json_value *in)
{
    size_t n; uint8_t *b = HEX(in, "file", &n);
    depz_fwdepz_image img;
    depz_fwdepz_result rc = depz_fwdepz_parse(b, n, &img);
    if (rc != DEPZ_FWDEPZ_OK) INVALID("depz_fwdepz_parse rc=%d", (int)rc);
    o_obj();
    KU("load_addr", img.load_addr); KU("fw_size", img.fw_size); KU("fw_crc32", img.fw_crc32);
    KU("cur_sec", img.cur_sec); KU("tot_sec", img.tot_sec); KB("payload_crc_ok", img.payload_crc_ok);
    o_end_obj();
}

/* ── BNO055 ── */

static void op_bno055_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    if (is(kind, "reset") || is(kind, "stop_stream") || is(kind, "get_info")) {
        o_obj(); KS("payload", ""); o_end_obj();
        return;
    }
    size_t n, dn = 0;
    uint8_t *out;
    if (is(kind, "read_reg")) {
        out = scratch(0);
        n = depz_bno055_pack_read_reg(U8(in, "addr"), U8(in, "len"), out);
    } else if (is(kind, "write_reg")) {
        uint8_t addr = U8(in, "addr");
        uint8_t *d = HEX(in, "data", &dn);
        out = scratch(dn);
        n = depz_bno055_pack_write_reg(addr, d, dn, out);
    } else if (is(kind, "start_stream")) {
        out = scratch(0);
        n = depz_bno055_pack_start_stream(U8(in, "trigger"), U8(in, "addr"), U8(in, "len"),
                                          U16(in, "period_ms"), out);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static void op_bno055_decode(const json_value *in)
{
    int64_t rpt = I64(in, "report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    if (rpt == DEPZ_BNO055_RPT_INFO) {
        depz_bno055_info i;
        if (depz_bno055_unpack_info(p, n, &i) != 0) INVALID("bno055_unpack_info");
        o_obj();
        KU("i2c_addr", i.i2c_addr); KU("chip_id", i.chip_id); KU("acc_id", i.acc_id); KU("mag_id", i.mag_id);
        KU("gyr_id", i.gyr_id); KU("sw_rev", i.sw_rev); KU("bl_rev", i.bl_rev); KU("initialized", i.initialized);
        KU("int_level", i.int_level); KU("int_edges", i.int_edges); KU("read_min_us", i.read_min_us);
        KU("read_max_us", i.read_max_us); KU("read_avg_us", i.read_avg_us); KU("tx_dropped", i.tx_dropped);
        KU("i2c_errors", i.i2c_errors); KU("slots_skipped", i.slots_skipped);
        KU("bus_recoveries", i.bus_recoveries); KU("last_i2c_error", i.last_i2c_error);
        KU("sensor_resets", i.sensor_resets); KU("loop_max_us", i.loop_max_us);
        o_end_obj();
    } else if (rpt == DEPZ_BNO055_RPT_REG_DATA) {
        depz_bno055_reg_data r;
        if (depz_bno055_unpack_reg_data(p, n, &r) != 0) INVALID("bno055_unpack_reg_data");
        o_obj(); KU("cmd", r.cmd); KU("timestamp_us", r.timestamp_us); KH("data", r.data, r.data_len); o_end_obj();
    } else if (rpt == DEPZ_BNO055_RPT_STREAM) {
        depz_bno055_stream s;
        if (depz_bno055_unpack_stream(p, n, &s) != 0) INVALID("bno055_unpack_stream");
        o_obj(); KU("timestamp_us", s.timestamp_us); KU("addr", s.addr); KU("len", s.len);
        KH("data", s.data, s.len); o_end_obj();
    } else {
        INVALID("report %lld", (long long)rpt);
    }
}

static void op_bno055_units(const json_value *in)
{
    depz_bno055_units u;
    depz_bno055_unpack_units(U8(in, "unit_sel"), &u);
    o_obj();
    KB("accel_mg", u.accel_mg); KB("gyro_rps", u.gyro_rps); KB("euler_rad", u.euler_rad);
    KB("temp_f", u.temp_f); KB("android", u.android); KU("repack", depz_bno055_pack_units(&u));
    o_end_obj();
}

static void op_bno055_calib_stat(const json_value *in)
{
    depz_bno055_calib_status s;
    depz_bno055_unpack_calib_status(U8(in, "value"), &s);
    o_obj();
    KU("system", s.system); KU("gyro", s.gyro); KU("accel", s.accel); KU("mag", s.mag);
    KB("fully_calibrated", depz_bno055_fully_calibrated(&s));
    KU("repack", depz_bno055_pack_calib_status(&s));
    o_end_obj();
}

static void o_i16s(const char *k, const int16_t *v, size_t n)
{
    o_key(k);
    o_arr();
    for (size_t i = 0; i < n; i++) o_i64(v[i]);
    o_end_arr();
}

static void op_bno055_calibration_profile(const json_value *in)
{
    size_t n; uint8_t *b = HEX(in, "bytes", &n);
    depz_bno055_calib_profile p;
    uint8_t back[DEPZ_BNO055_CALIB_PROFILE_LEN + 64];
    if (depz_bno055_unpack_calib_profile(b, n, &p) != 0) INVALID("bno055_unpack_calib_profile");
    size_t bn = depz_bno055_pack_calib_profile(&p, back);
    o_obj();
    o_i16s("accel_offset", p.accel_offset, 3);
    o_i16s("mag_offset", p.mag_offset, 3);
    o_i16s("gyro_offset", p.gyro_offset, 3);
    KI("accel_radius", p.accel_radius); KI("mag_radius", p.mag_radius);
    KH("repack", back, bn);
    o_end_obj();
}

static void o_repack_pair(const depz_bno055_axis_remap *a)
{
    uint8_t c = 0, s = 0;
    if (depz_bno055_pack_axis_remap(a, &c, &s) != 0) INVALID("bno055_pack_axis_remap refused");
    o_key("repack"); o_arr(); o_u64(c); o_u64(s); o_end_arr();
}

static void op_bno055_axis_remap(const json_value *in)
{
    depz_bno055_axis_remap a;
    depz_bno055_unpack_axis_remap(U8(in, "config"), U8(in, "sign"), &a);
    o_obj();
    KU("x", a.x); KU("y", a.y); KU("z", a.z);
    KB("x_negative", a.x_negative); KB("y_negative", a.y_negative); KB("z_negative", a.z_negative);
    o_repack_pair(&a);
    o_end_obj();
}

static void op_bno055_axis_remap_pack(const json_value *in)
{
    depz_bno055_axis_remap a;
    a.x = U8(in, "x"); a.y = U8(in, "y"); a.z = U8(in, "z");
    a.x_negative = barg(in, "x_negative"); a.y_negative = barg(in, "y_negative");
    a.z_negative = barg(in, "z_negative");
    o_obj(); o_repack_pair(&a); o_end_obj();
}

static void op_bno055_sensor_config(const json_value *in)
{
    const char *sensor = sarg(in, "sensor");
    if (is(sensor, "accel")) {
        depz_bno055_accel_config a;
        depz_bno055_unpack_accel_config(U8(in, "value"), &a);
        o_obj(); KU("range", a.range); KU("bandwidth", a.bandwidth); KU("power", a.power);
        KU("repack", depz_bno055_pack_accel_config(&a)); o_end_obj();
    } else if (is(sensor, "gyro")) {
        size_t n; uint8_t *b = HEX(in, "bytes", &n);
        depz_bno055_gyro_config g;
        uint8_t back[64];
        if (depz_bno055_unpack_gyro_config_n(b, n, &g) != 0) INVALID("gyro config shorter than 2 bytes");
        size_t bn = depz_bno055_pack_gyro_config(&g, back);
        o_obj(); KU("range", g.range); KU("bandwidth", g.bandwidth); KU("power", g.power);
        KH("repack", back, bn); o_end_obj();
    } else if (is(sensor, "mag")) {
        depz_bno055_mag_config m;
        depz_bno055_unpack_mag_config(U8(in, "value"), &m);
        o_obj(); KU("rate", m.rate); KU("mode", m.mode); KU("power", m.power);
        KU("repack", depz_bno055_pack_mag_config(&m)); o_end_obj();
    } else {
        INVALID("sensor %s", sensor);
    }
}

static void o_words(const char *k, bool has, const int16_t *v, size_t n)
{
    o_key(k);
    if (!has) { o_null(); return; }
    o_arr();
    for (size_t i = 0; i < n; i++) o_i64(v[i]);
    o_end_arr();
}

static void op_bno055_block(const json_value *in)
{
    uint8_t addr = U8(in, "addr");
    size_t n; uint8_t *d = HEX(in, "data", &n);
    depz_bno055_block b;
    depz_bno055_decode_block(addr, d, n, &b);
    o_obj();
    o_words("accel", b.has_accel, b.accel, 3);
    o_words("mag", b.has_mag, b.mag, 3);
    o_words("gyro", b.has_gyro, b.gyro, 3);
    o_words("euler", b.has_euler, b.euler, 3);
    o_words("quaternion", b.has_quaternion, b.quaternion, 4);
    o_words("linear_accel", b.has_linear_accel, b.linear_accel, 3);
    o_words("gravity", b.has_gravity, b.gravity, 3);
    o_key("temperature"); if (b.has_temperature) o_i64(b.temperature); else o_null();
    o_key("calib_stat"); if (b.has_calib_stat) o_u64(b.calib_stat); else o_null();
    o_end_obj();
}

/* ── BNO086 ── */

static void op_bno086_shtp_header_pack(const json_value *in)
{
    depz_shtp_header h = {U16(in, "length"), U8(in, "channel"), U8(in, "seq"), barg(in, "continuation")};
    uint8_t out[4];
    if (depz_shtp_pack_header_checked(&h, out) != 0) INVALID("length over 15 bits");
    o_obj(); KH("bytes", out, 4); o_end_obj();
}

static void op_bno086_shtp_header_unpack(const json_value *in)
{
    size_t n; uint8_t *b = HEX(in, "bytes", &n);
    depz_shtp_header h;
    if (n < 4) INVALID("the API reads 4 header bytes");
    depz_shtp_unpack_header(b, &h);
    o_obj(); KU("length", h.length); KU("channel", h.channel); KU("seq", h.seq);
    KB("continuation", h.continuation); o_end_obj();
}

static depz_shtp_layer g_layer;
static bool g_layer_live;

static void op_bno086_shtp_tx_seq(const json_value *in)
{
    const json_value *pk = larg(in, "packets");
    depz_shtp_init(&g_layer);
    g_layer_live = true;
    o_obj(); o_key("frames"); o_arr();
    for (size_t i = 0; i < json_arr_size(pk); i++) {
        const json_value *p = json_arr_get(pk, i);
        size_t pn; uint8_t *pl = HEX(p, "payload", &pn);
        uint8_t *out = scratch(pn);
        size_t on = depz_shtp_next_frame(&g_layer, U8(p, "channel"), pl, pn, out, pn + 70000);
        if (on == 0) INVALID("depz_shtp_next_frame refused packet %zu", i);
        o_hex(out, on);
    }
    o_end_arr(); o_end_obj();
}

static void op_bno086_shtp_reassembly(const json_value *in)
{
    const json_value *fr = larg(in, "frames"), *cap = larg(in, "captures_us");
    if (json_arr_size(fr) != json_arr_size(cap)) INVALID("frames / captures_us length");
    depz_shtp_init(&g_layer);
    g_layer_live = true;
    o_obj(); o_key("cargos"); o_arr();
    for (size_t i = 0; i < json_arr_size(fr); i++) {
        size_t fn; uint8_t *f = hex_of(json_arr_get(fr, i), "frames", &fn);
        uint64_t t = uval(json_arr_get(cap, i), "captures_us");
        depz_shtp_cargo c;
        int rc = depz_shtp_feed_ex(&g_layer, f, fn, t, &c);
        if (rc < 0) INVALID("depz_shtp_feed_ex rc=%d", rc);
        if (rc == 1) {
            o_obj(); KU("channel", c.channel); KU("seq", c.seq); KH("payload", c.payload, c.payload_len);
            KU("capture_us", c.capture_us); o_end_obj();
        }
    }
    o_end_arr();
    KU("discarded", g_layer.discarded);
    o_end_obj();
}

static void op_bno086_control_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(4096);
    size_t n;
    if (is(kind, "set_feature")) {
        n = depz_bno_pack_set_feature(U8(in, "sensor_id"), U8(in, "flags"), U16(in, "sensitivity"),
                                      U32(in, "interval_us"), U32(in, "batch_us"), U32(in, "cfg_word"), out);
    } else if (is(kind, "get_feature_request")) {
        n = depz_bno_pack_get_feature_request(U8(in, "sensor_id"), out);
    } else if (is(kind, "product_id_request")) {
        n = depz_bno_pack_product_id_request(out);
    } else if (is(kind, "command_request")) {
        uint8_t seq = U8(in, "seq"), cmd = U8(in, "command");
        size_t pn; uint8_t *pr = HEX(in, "params", &pn);
        out = scratch(pn);
        n = depz_bno_pack_command_request(seq, cmd, pr, pn, out);
    } else if (is(kind, "frs_read_request")) {
        n = depz_bno_pack_frs_read_request(U16(in, "frs_type"), U16(in, "offset_words"), U16(in, "block_words"),
                                           out);
    } else if (is(kind, "frs_write_request")) {
        n = depz_bno_pack_frs_write_request(U16(in, "frs_type"), U16(in, "length_words"), out);
    } else if (is(kind, "frs_write_data")) {
        uint16_t off = U16(in, "offset_words");
        const json_value *w = larg(in, "words");
        size_t wn = json_arr_size(w);
        uint32_t *words = (uint32_t *)amalloc(sizeof(uint32_t) * (wn + 1));
        for (size_t i = 0; i < wn; i++) words[i] = (uint32_t)ival(json_arr_get(w, i), "words", 0, 0xFFFFFFFFLL);
        out = scratch(wn * 4);
        n = depz_bno_pack_frs_write_data(off, words, wn, out, wn * 4 + 70000);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static const char *const XYZ_RAW[3] = {"x_raw", "y_raw", "z_raw"};

/* The raw `fields` of one report: the keys of the TS / Python report record
 * for its type (bools as 0/1, bytes hex). */
static void o_bno_fields(const depz_bno_report *r)
{
    const depz_bno_report_type t = r->type;
    o_obj();
    KU("sensor_id", r->sensor_id);
    KI("timestamp_us", r->timestamp_us);
    if (t == DEPZ_BNO_UNKNOWN_REPORT) {
        KH("data", r->data ? r->data : (const uint8_t *)"", r->data_len);
        o_end_obj();
        return;
    }
    if (t == DEPZ_BNO_FLUSH_COMPLETED_REPORT) {
        KI("flushed_sensor", r->flushed_sensor);
        o_end_obj();
        return;
    }
    if (t == DEPZ_BNO_GYRO_INTEGRATED_RV) {
        KI("i_raw", r->i_raw); KI("j_raw", r->j_raw); KI("k_raw", r->k_raw); KI("real_raw", r->real_raw);
        KI("vx_raw", r->vx_raw); KI("vy_raw", r->vy_raw); KI("vz_raw", r->vz_raw);
        o_end_obj();
        return;
    }
    KU("seq", r->seq); KU("accuracy", r->accuracy); KI("delay_us", r->delay_us);
    switch (t) {
    case DEPZ_BNO_ACCELERATION: case DEPZ_BNO_GYROSCOPE: case DEPZ_BNO_MAGNETOMETER:
        KI(XYZ_RAW[0], r->x_raw); KI(XYZ_RAW[1], r->y_raw); KI(XYZ_RAW[2], r->z_raw);
        break;
    case DEPZ_BNO_UNCAL_GYROSCOPE: case DEPZ_BNO_UNCAL_MAGNETOMETER:
        KI("x_raw", r->x_raw); KI("y_raw", r->y_raw); KI("z_raw", r->z_raw);
        KI("bias_x_raw", r->bias_x_raw); KI("bias_y_raw", r->bias_y_raw); KI("bias_z_raw", r->bias_z_raw);
        break;
    case DEPZ_BNO_ROTATION_VECTOR:
        KI("i_raw", r->i_raw); KI("j_raw", r->j_raw); KI("k_raw", r->k_raw); KI("real_raw", r->real_raw);
        o_key("accuracy_raw");
        if (r->has_accuracy_raw) o_i64(r->accuracy_raw); else o_null();
        break;
    case DEPZ_BNO_SCALAR_REPORT: case DEPZ_BNO_GENERIC_EVENT:
        KI("value_raw", r->value_raw);
        break;
    case DEPZ_BNO_TAP_DETECTOR: case DEPZ_BNO_SHAKE_DETECTOR:
        KI("flags", r->flags);
        break;
    case DEPZ_BNO_STEP_COUNTER:
        KU("latency_us", r->latency_us); KU("steps", r->steps);
        break;
    case DEPZ_BNO_STEP_DETECTOR:
        KU("latency_us", r->latency_us);
        break;
    case DEPZ_BNO_SIGNIFICANT_MOTION:
        KI("motion", r->motion);
        break;
    case DEPZ_BNO_STABILITY_CLASSIFIER:
        KI("classification", r->classification);
        break;
    case DEPZ_BNO_MOTION_REQUEST:
        KI("intent", r->intent); KI("request", r->request);
        break;
    case DEPZ_BNO_ACTIVITY_CLASSIFIER:
        KI("page_number", r->page_number); KI("end_of_sequence", r->end_of_sequence ? 1 : 0);
        KI("most_likely_state", r->most_likely_state);
        o_key("confidences"); o_arr();
        for (int k = 0; k < 10; k++) o_u64(r->confidences[k]);
        o_end_arr();
        break;
    case DEPZ_BNO_RAW_SENSOR:
        KI("x_raw", r->x_raw); KI("y_raw", r->y_raw); KI("z_raw", r->z_raw);
        KU("sensor_timestamp_us", r->sensor_timestamp_us); KI("temperature_raw", r->temperature_raw);
        break;
    default:
        break;
    }
    o_end_obj();
}

static void op_bno086_input_cargo(const json_value *in)
{
    size_t n; uint8_t *c = HEX(in, "cargo", &n);
    uint64_t cap_ts = U64(in, "capture_timestamp_us");
    depz_bno_report *reps = (depz_bno_report *)amalloc(sizeof(depz_bno_report) * (n + 2));
    size_t got = depz_bno_parse_input_cargo(c, n, cap_ts, reps, n + 2);
    o_obj(); o_key("reports"); o_arr();
    for (size_t i = 0; i < got; i++) {
        o_obj(); KS("type", depz_bno_report_type_str(reps[i].type)); o_key("fields"); o_bno_fields(&reps[i]);
        o_end_obj();
    }
    o_end_arr(); o_end_obj();
}

static void op_bno086_gyro_rv(const json_value *in)
{
    size_t n; uint8_t *c = HEX(in, "cargo", &n);
    uint64_t cap_ts = U64(in, "capture_timestamp_us");
    depz_bno_report r;
    if (depz_bno_parse_gyro_rv(c, n, cap_ts, &r) != 0) { o_null(); return; } /* short cargo -> no report */
    o_obj(); KS("type", depz_bno_report_type_str(r.type)); o_key("fields"); o_bno_fields(&r); o_end_obj();
}

/* ── TMF8829 ── */

static void op_tmf8829_encode(const json_value *in)
{
    int64_t cmd = I64(in, "cmd");
    size_t n;
    uint8_t *out;
    if (cmd == DEPZ_TMF8829_CMD_READ_REG) {
        out = scratch(0);
        n = depz_tmf8829_pack_read_reg((int)I32(in, "addr"), (int)I32(in, "len"), out);
    } else if (cmd == DEPZ_TMF8829_CMD_WRITE_REG) {
        int addr = (int)I32(in, "addr");
        size_t dn; uint8_t *d = HEX(in, "data", &dn);
        out = scratch(dn);
        n = depz_tmf8829_pack_write_reg(addr, d, dn, out);
    } else if (cmd == DEPZ_TMF8829_CMD_START_STREAM) {
        out = scratch(0);
        n = depz_tmf8829_pack_start_stream((int)I32(in, "int_mask"), (int)I32(in, "flags"), out);
    } else {
        o_obj(); KS("payload", ""); o_end_obj();
        return;
    }
    payload_out(out, n);
}

static void op_tmf8829_decode(const json_value *in)
{
    int64_t rpt = I64(in, "report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    if (rpt == DEPZ_TMF8829_RPT_REG_DATA) {
        depz_tmf8829_reg_data r;
        if (depz_tmf8829_unpack_reg_data(p, n, &r) != 0) INVALID("tmf8829_unpack_reg_data");
        o_obj(); KU("timestamp_us", r.timestamp_us); KH("data", r.data, r.data_len); o_end_obj();
    } else if (rpt == DEPZ_TMF8829_RPT_INFO) {
        depz_tmf8829_info r;
        if (depz_tmf8829_unpack_info(p, n, &r) != 0) INVALID("tmf8829_unpack_info");
        o_obj();
        KU("int_edges", r.int_edges); KU("frames", r.frames); KU("frames_aborted", r.frames_aborted);
        KU("frame_errors", r.frame_errors); KU("spi_timeouts", r.spi_timeouts); KU("int_level", r.int_level);
        KB("streaming", r.streaming); KU("int_mask", r.int_mask); KU("last_int_status", r.last_int_status);
        KU("last_fifo_status", r.last_fifo_status);
        o_end_obj();
    } else if (rpt == DEPZ_TMF8829_RPT_FRAME) {
        depz_tmf8829_frame_chunk r;
        if (depz_tmf8829_unpack_frame_chunk(p, n, &r) != 0) INVALID("tmf8829_unpack_frame_chunk");
        o_obj(); KU("timestamp_us", r.timestamp_us); KU("full_size", r.full_size); KU("offset", r.offset);
        KU("frame_seq", r.frame_seq); KU("edge_us", r.edge_us); KH("data", r.data, r.data_len); o_end_obj();
    } else if (rpt == DEPZ_TMF8829_RPT_EVENT) {
        depz_tmf8829_event r;
        if (depz_tmf8829_unpack_event(p, n, &r) != 0) INVALID("tmf8829_unpack_event");
        o_obj(); KU("timestamp_us", r.timestamp_us); KU("int_status", r.int_status); KU("code", r.code);
        KU("frame_id", r.frame_id); KU("fifo_status", r.fifo_status); o_end_obj();
    } else {
        INVALID("report %lld", (long long)rpt);
    }
}

static void op_tmf8829_reassembly(const json_value *in)
{
    const json_value *chunks = larg(in, "chunks");
    depz_tmf8829_reassembler *ra = (depz_tmf8829_reassembler *)amalloc(sizeof *ra);
    depz_tmf8829_reassembler_reset(ra);
    o_obj(); o_key("frames"); o_arr();
    for (size_t i = 0; i < json_arr_size(chunks); i++) {
        size_t n; uint8_t *p = hex_of(json_arr_get(chunks, i), "chunks", &n);
        depz_tmf8829_frame_chunk ch;
        depz_tmf8829_raw_frame f;
        char sha[65];
        if (depz_tmf8829_unpack_frame_chunk(p, n, &ch) != 0) INVALID("chunk %zu does not unpack", i);
        if (depz_tmf8829_reassembler_feed(ra, &ch, &f)) {
            sha256_hex(f.data, f.len, sha);
            o_obj(); KU("frame_seq", f.frame_seq); KU("timestamp_us", f.timestamp_us); KU("edge_us", f.edge_us);
            KU("size", f.len); KS("sha256", sha); o_end_obj();
        }
    }
    o_end_arr();
    KU("completed", ra->completed); KU("discarded", ra->discarded); KU("seq_gaps", ra->seq_gaps);
    o_end_obj();
}

/* tmf8829 `results` arrays: the shape check_results() in test_vectors.c
 * asserts — rows / cols 0 when no array is present, each array null when absent. */
static void o_tmf_arrays(const depz_tmf8829_results *r)
{
    size_t z = (size_t)r->rows * r->cols, np = (size_t)r->peaks * z, i;
    bool any = r->peaks > 0 || r->has_noise || r->has_xtalk;
    KU("peaks", r->peaks);
    KU("rows", any ? r->rows : 0);
    KU("cols", any ? r->cols : 0);
#define ARR(key, present, src, count)                     \
    o_key(key);                                           \
    if (!(present)) o_null();                             \
    else { o_arr(); for (i = 0; i < (count); i++) o_u64((src)[i]); o_end_arr(); }
    ARR("distance", r->peaks > 0, r->distance, np);
    ARR("snr", r->peaks > 0, r->snr, np);
    ARR("signal", r->has_signal, r->signal, np);
    ARR("noise", r->has_noise, r->noise, z);
    ARR("xtalk", r->has_xtalk, r->xtalk, z);
#undef ARR
}

static void op_tmf8829_frame(const json_value *in)
{
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    depz_tmf8829_frame_info f;
    depz_tmf8829_frame_problems pr;
    if (depz_tmf8829_parse_frame(raw, n, &f) != 0) INVALID("tmf8829_parse_frame");
    depz_tmf8829_frame_problems_of(&f, &pr);
    o_obj();
    o_key("header"); o_obj();
    KU("fifo_status", f.fifo_status); KU("systick", f.systick); KU("id", f.id); KU("kind", f.kind);
    KU("fp_mode", f.fp_mode); KU("layout", f.layout); KU("payload", f.payload); KU("f_number", f.f_number);
    o_key("temperature"); o_arr(); for (int k = 0; k < 3; k++) o_i64(f.temperature[k]); o_end_arr();
    KU("bdv", f.bdv);
    o_key("ref_pos"); o_arr(); for (int k = 0; k < 2; k++) o_u64(f.ref_pos[k]); o_end_arr();
    KU("t0_integration", f.t0_integration); KU("t1_integration", f.t1_integration);
    KU("frame_status", f.frame_status); KU("reserved", f.reserved); KU("eof", f.eof); KU("size", f.size);
    o_end_obj();
    o_key("problems"); o_arr(); for (int k = 0; k < pr.count; k++) o_str(pr.text[k]); o_end_arr();
    if (f.kind == DEPZ_TMF8829_FID_RESULTS) {
        depz_tmf8829_results *res = (depz_tmf8829_results *)amalloc(sizeof *res);
        char err[160] = "";
        if (depz_tmf8829_parse_results(raw, n, &f, res, err, sizeof err) != 0) INVALID("parse_results: %s", err);
        o_key("results"); o_obj(); o_tmf_arrays(res); o_end_obj();
    } else {
        char sha[65];
        if (n < 33) INVALID("frame shorter than head + footer");
        sha256_hex(raw + 21, n - 33, sha);
        KS("data_sha256", sha);
    }
    o_end_obj();
}

static void op_tmf8829_measurements(const json_value *in)
{
    const json_value *frames = larg(in, "frames");
    depz_tmf8829_builder *b = (depz_tmf8829_builder *)amalloc(sizeof *b);
    depz_tmf8829_measurement *m = (depz_tmf8829_measurement *)amalloc(sizeof *m);
    depz_tmf8829_builder_reset(b);
    o_obj(); o_key("measurements"); o_arr();
    for (size_t j = 0; j < json_arr_size(frames); j++) {
        const json_value *fj = json_arr_get(frames, j);
        size_t n; uint8_t *raw = HEX(fj, "raw", &n);
        int rc = depz_tmf8829_builder_add(b, raw, n, I64(fj, "timestamp_us"), I64(fj, "edge_us"), m);
        if (rc < 0) INVALID("builder_add rc=%d", rc);
        if (rc == 0) continue;
        o_obj();
        KI("timestamp_us", m->timestamp_us); KU("flags", m->flags); KU("fp_mode", m->fp_mode);
        KU("f_number", m->f_number);
        o_key("frame_times_us"); o_arr(); for (size_t t = 0; t < m->n_frames; t++) o_i64(m->times[t].timestamp_us);
        o_end_arr();
        o_key("t1_len_us"); o_arr(); for (size_t t = 0; t < m->n_frames; t++) o_u64(m->times[t].t1_len_us);
        o_end_arr();
        KU("ratio_q16", b->clock.ratio_q16); KU("n_histograms", m->n_histograms);
        KU("n_ref_frames", m->n_ref_frames); KU("n_problems", m->n_problems);
        KB("distance_in_mm", m->distance_in_mm);
        o_tmf_arrays(&m->results);
        o_end_obj();
    }
    o_end_arr(); o_end_obj();
}

static void op_tmf8829_config_get(const json_value *in)
{
    size_t n; uint8_t *pg = HEX(in, "page", &n);
    uint32_t v;
    int rc = depz_tmf8829_cfg_get_n(pg, n, sarg(in, "field"), &v);
    if (rc != 0) INVALID("cfg_get rc=%d", rc);
    o_obj(); KU("value", v); o_end_obj();
}

static void op_tmf8829_config_set(const json_value *in)
{
    size_t n; uint8_t *pg = HEX(in, "page", &n);
    int rc = depz_tmf8829_cfg_set_n(pg, n, sarg(in, "field"), I64(in, "value"));
    if (rc != 0) INVALID("cfg_set rc=%d", rc);
    o_obj(); KH("page", pg, n); o_end_obj();
}

static void op_tmf8829_frame_time(const json_value *in)
{
    const json_value *rows = larg(in, "rows");
    depz_tmf8829_clock clock;
    depz_tmf8829_timer timer;
    depz_tmf8829_clock_reset(&clock);
    depz_tmf8829_timer_reset(&timer);
    o_obj(); o_key("times"); o_arr();
    for (size_t j = 0; j < json_arr_size(rows); j++) {
        const json_value *r = json_arr_get(rows, j);
        /* the frame-id byte, split here as the SDK's frame parser does */
        uint8_t fid = U8(r, "id");
        unsigned kind = (unsigned)fid & 0xF0u;
        if (kind == 0x30) continue;
        int64_t ts = I64(r, "ts");
        uint32_t tick = U32(r, "systick");
        uint8_t flags;
        if (depz_tmf8829_clock_add_checked(&clock, ts, tick, &flags) != 0) INVALID("clock_add_checked refused");
        if (kind == 0x20) { depz_tmf8829_timer_histograms(&timer); continue; }
        depz_tmf8829_frame_time t;
        if (depz_tmf8829_timer_results_checked(&timer, &clock, (unsigned)fid & 0x0Fu, tick, U32(r, "t0"), U32(r, "t1"),
                                               ts, I64(r, "edge_us"), flags, &t) != 0)
            INVALID("timer_results_checked refused");
        o_obj(); KI("timestamp_us", t.timestamp_us); KU("t1_len_us", t.t1_len_us); KU("flags", t.flags);
        KU("ratio_q16", clock.ratio_q16); o_end_obj();
    }
    o_end_arr(); o_end_obj();
}

/* ── VL53L4CD ── */

static void op_vl53l4_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(0);
    size_t n;
    if (is(kind, "read_reg")) {
        n = depz_vl53l4_pack_read_reg(U16(in, "addr"), U16(in, "len"), out);
    } else if (is(kind, "write_reg")) {
        uint16_t addr = U16(in, "addr");
        size_t dn; uint8_t *d = HEX(in, "data", &dn);
        out = scratch(dn);
        n = depz_vl53l4_pack_write_reg(addr, d, dn, out);
    } else if (is(kind, "xshut")) {
        n = depz_vl53l4_pack_xshut(U8(in, "action"), out);
    } else if (is(kind, "start_stream")) {
        n = depz_vl53l4_pack_start_stream(U16(in, "addr"), U16(in, "len"), U8(in, "flags"), out);
    } else if (is(kind, "set_i2c_speed")) {
        n = depz_vl53l4_pack_set_i2c_speed(U16(in, "khz"), out);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static void op_vl53l4_decode(const json_value *in)
{
    int64_t rpt = I64(in, "report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    if (rpt == DEPZ_VL53L4_RPT_REG_DATA) {
        depz_vl53l4_reg_data r;
        if (depz_vl53l4_unpack_reg_data(p, n, &r) != 0) INVALID("vl53l4_unpack_reg_data");
        o_obj(); KU("cmd", r.cmd); KU("timestamp_us", r.timestamp_us); KH("data", r.data, r.data_len); o_end_obj();
    } else if (rpt == DEPZ_VL53L4_RPT_INFO) {
        depz_vl53l4_info r;
        if (depz_vl53l4_unpack_info(p, n, &r) != 0) INVALID("vl53l4_unpack_info");
        o_obj();
        KU("int_edges", r.int_edges); KU("slots_skipped", r.slots_skipped); KU("i2c_errors", r.i2c_errors);
        KU("last_i2c_error", r.last_i2c_error); KU("model_id", r.model_id); KU("fw_status", r.fw_status);
        KU("initialized", r.initialized); KU("xshut_level", r.xshut_level); KU("int_level", r.int_level);
        KU("i2c_khz", r.i2c_khz);
        o_end_obj();
    } else if (rpt == DEPZ_VL53L4_RPT_STREAM) {
        depz_vl53l4_stream r;
        if (depz_vl53l4_unpack_stream(p, n, &r) != 0) INVALID("vl53l4_unpack_stream");
        o_obj(); KU("timestamp_us", r.timestamp_us); KU("addr", r.addr); KU("len", r.len);
        KH("data", r.data, r.len); o_end_obj();
    } else {
        INVALID("report %lld", (long long)rpt);
    }
}

static void o_l4_result(const depz_vl53l4_result *r)
{
    o_obj();
    KI("range_status", r->range_status); KI("distance_mm", r->distance_mm);
    KI("ambient_rate_kcps", r->ambient_rate_kcps); KI("ambient_per_spad_kcps", r->ambient_per_spad_kcps);
    KI("signal_rate_kcps", r->signal_rate_kcps); KI("signal_per_spad_kcps", r->signal_per_spad_kcps);
    KI("number_of_spad", r->number_of_spad); KI("sigma_mm", r->sigma_mm); KI("stream_count", r->stream_count);
    o_end_obj();
}

static void op_vl53l4_result_block(const json_value *in)
{
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    depz_vl53l4_result r;
    if (depz_vl53l4_parse_result_block(raw, n, &r) != 0) INVALID("vl53l4_parse_result_block");
    o_l4_result(&r);
}

static void op_vl53l4_timing_encode(const json_value *in)
{
    uint16_t a = 0, b = 0; uint32_t im = 0;
    int rc = depz_vl53l4_range_timing_registers(U32(in, "timing_budget_ms"), U32(in, "inter_measurement_ms"),
                                                U16(in, "osc_frequency"), U16(in, "clock_pll"), &a, &b, &im);
    if (rc != 0) INVALID("range_timing_registers rc=%d", rc);
    o_obj(); KU("range_config_a", a); KU("range_config_b", b); KU("intermeasurement_raw", im); o_end_obj();
}

static void op_vl53l4_timing_decode(const json_value *in)
{
    uint32_t budget = 0, inter = 0;
    int rc = depz_vl53l4_decode_range_timing(U32(in, "intermeasurement_raw"), U16(in, "clock_pll"),
                                             U16(in, "osc_frequency"), U16(in, "range_config_a"), &budget, &inter);
    if (rc != 0) INVALID("decode_range_timing rc=%d", rc);
    o_obj(); KU("timing_budget_ms", budget); KU("inter_measurement_ms", inter); o_end_obj();
}

static void op_vl53l4_tuning_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    int64_t raw;
    uint16_t w = 0;
    if (is(kind, "offset")) {
        if (depz_vl53l4_offset_raw_checked(I32(in, "value"), &w) != 0) INVALID("offset_raw_checked refused");
        raw = w;
    } else if (is(kind, "xtalk")) {
        if (depz_vl53l4_xtalk_raw_checked(U16(in, "value"), &w) != 0) INVALID("xtalk_raw_checked refused");
        raw = w;
    }
    else if (is(kind, "signal_threshold")) raw = depz_vl53l4_signal_threshold_raw(U16(in, "value"));
    else if (is(kind, "sigma_threshold")) {
        uint16_t r = 0;
        if (depz_vl53l4_sigma_threshold_raw(U16(in, "value"), &r) != 0) INVALID("sigma_threshold_raw refused");
        raw = r;
    } else INVALID("kind %s", kind);
    o_obj(); KI("raw", raw); o_end_obj();
}

static void op_vl53l4_tuning_decode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint16_t raw = U16(in, "raw");
    int64_t v;
    if (is(kind, "offset")) v = depz_vl53l4_decode_offset(raw);
    else if (is(kind, "xtalk")) v = depz_vl53l4_decode_xtalk(raw);
    else if (is(kind, "signal_threshold")) v = depz_vl53l4_decode_signal_threshold(raw);
    else if (is(kind, "sigma_threshold")) v = depz_vl53l4_decode_sigma_threshold(raw);
    else INVALID("kind %s", kind);
    o_obj(); KI("value", v); o_end_obj();
}

/* ── VL53L5CX / L7 ── */

static void op_vl53l7_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(0);
    size_t n;
    if (is(kind, "read_reg")) {
        n = depz_vl53l7_pack_read_reg(U16(in, "addr"), U16(in, "len"), out);
    } else if (is(kind, "write_reg")) {
        uint16_t addr = U16(in, "addr");
        size_t dn; uint8_t *d = HEX(in, "data", &dn);
        out = scratch(dn);
        n = depz_vl53l7_pack_write_reg(addr, d, dn, out);
    } else if (is(kind, "pin_ctrl")) {
        n = depz_vl53l7_pack_pin_ctrl(U8(in, "action"), out);
    } else if (is(kind, "set_i2c_speed")) {
        n = depz_vl53l7_pack_set_i2c_speed(U16(in, "khz"), out);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static void op_vl53l7_decode(const json_value *in)
{
    if (I64(in, "report") != DEPZ_VL53L7_RPT_INFO) INVALID("report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    depz_vl53l7_info i;
    if (depz_vl53l7_unpack_info(p, n, &i) != 0) INVALID("vl53l7_unpack_info");
    o_obj();
    KU("int_edges", i.int_edges); KU("frames_dropped", i.frames_dropped); KU("i2c_errors", i.i2c_errors);
    KU("last_i2c_error", i.last_i2c_error); KU("lpn_level", i.lpn_level); KU("int_level", i.int_level);
    KU("i2c_khz", i.i2c_khz); KU("frame_size", i.frame_size); KB("streaming", i.streaming);
    o_end_obj();
}

static void op_vl53l7_model(const json_value *in)
{
    size_t ul, dl;
    const char *um = slarg(in, "usb_model", &ul), *dn = slarg(in, "device_name", &dl);
    const char *cls = depz_vl53l7_model_str(depz_vl53l7_resolve_model_n(um, ul, dn, dl));
    o_obj(); KS("class", cls); o_end_obj();
}

/* ── VL53L 1D family ── */

static void op_vl53lx_encode(const json_value *in)
{
    const char *kind = sarg(in, "kind");
    uint8_t *out = scratch(0);
    size_t n;
    if (is(kind, "set_addr_width")) {
        n = depz_vl53lx_pack_set_addr_width(U8(in, "width"), out);
    } else if (is(kind, "read_reg")) {
        n = depz_vl53l4_pack_read_reg(U16(in, "addr"), U16(in, "len"), out);
    } else if (is(kind, "write_reg")) {
        uint16_t addr = U16(in, "addr");
        size_t dn; uint8_t *d = HEX(in, "data", &dn);
        out = scratch(dn);
        n = depz_vl53l4_pack_write_reg(addr, d, dn, out);
    } else if (is(kind, "xshut")) {
        n = depz_vl53l4_pack_xshut(U8(in, "action"), out);
    } else if (is(kind, "set_i2c_speed")) {
        n = depz_vl53l4_pack_set_i2c_speed(U16(in, "khz"), out);
    } else if (is(kind, "start_stream")) {
        uint16_t addr = U16(in, "addr"), len = U16(in, "len");
        uint8_t flags = U8(in, "flags");
        const json_value *cl = larg(in, "clear");
        size_t ns = json_arr_size(cl);
        depz_vl53lx_clear_step *steps = (depz_vl53lx_clear_step *)amalloc(sizeof *steps * (ns + 1));
        for (size_t k = 0; k < ns; k++) {
            const json_value *st = json_arr_get(cl, k);
            if (json_arr_size(st) != 2) INVALID("clear step: not an [addr, value] pair");
            steps[k].addr = (uint16_t)ival(json_arr_get(st, 0), "clear.addr", 0, 0xFFFF);
            steps[k].value = (uint8_t)ival(json_arr_get(st, 1), "clear.value", 0, 0xFF);
        }
        out = scratch(3 * ns);
        n = depz_vl53lx_pack_start_stream(addr, len, flags, steps, ns, out);
    } else {
        INVALID("kind %s", kind);
    }
    payload_out(out, n);
}

static void op_vl53lx_decode(const json_value *in)
{
    if (I64(in, "report") != DEPZ_VL53LX_RPT_INFO) INVALID("report");
    size_t n; uint8_t *p = HEX(in, "payload", &n);
    depz_vl53lx_info i;
    if (depz_vl53lx_unpack_info(p, n, &i) != 0) INVALID("vl53lx_unpack_info");
    o_obj();
    KU("int_edges", i.int_edges); KU("slots_skipped", i.slots_skipped); KU("i2c_errors", i.i2c_errors);
    KU("last_i2c_error", i.last_i2c_error); KU("xshut_level", i.xshut_level); KU("int_level", i.int_level);
    KU("i2c_khz", i.i2c_khz); KU("addr_width", i.addr_width); KU("n_clear", i.n_clear);
    KU("frames_dropped", i.frames_dropped);
    o_end_obj();
}

static void op_vl53lx_model(const json_value *in)
{
    size_t ul, dl;
    const char *um = slarg(in, "usb_model", &ul), *dev = slarg(in, "device_name", &dl);
    /* the class name the vectors carry is the Python / TS class ("Vl53l0x"); ours is lower case */
    char cls[32];
    snprintf(cls, sizeof cls, "%s", depz_vl53lx_class_str(depz_vl53lx_resolve_class_n(um, ul, dev, dl)));
    cls[0] = (char)toupper((unsigned char)cls[0]);
    const depz_vl53lx_product_info *p = depz_vl53lx_product_get(depz_vl53lx_product_from_board_name_n(dev, dl));
    o_obj(); KS("class", cls); KS("product", p ? p->name : NULL); o_end_obj();
}

static void op_vl53lx_die_block(const json_value *in)
{
    const char *var = sarg(in, "variant");
    depz_vl53lx_die_variant v;
    if (is(var, "l4")) v = DEPZ_VL53LX_DIE_L4;
    else if (is(var, "l1")) v = DEPZ_VL53LX_DIE_L1;
    else INVALID("variant %s", var);
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    depz_vl53l4_result r;
    if (depz_vl53lx_decode_die_block(raw, n, v, &r) != 0) INVALID("vl53lx_decode_die_block");
    o_l4_result(&r);
}

static void op_vl53lx_l0x_raw(const json_value *in)
{
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    depz_vl53lx_l0x_raw r;
    if (depz_vl53lx_decode_l0x_raw(raw, n, &r) != 0) INVALID("vl53lx_decode_l0x_raw");
    o_obj();
    KI("distance_raw", r.distance_raw); KI("device_range_status", r.device_range_status);
    KI("signal_rate_mcps_1616", r.signal_rate_mcps_1616); KI("ambient_rate_mcps_1616", r.ambient_rate_mcps_1616);
    KI("effective_spad_count_88", r.effective_spad_count_88);
    o_end_obj();
}

static void op_vl53lx_histogram_raw(const json_value *in)
{
    size_t n; uint8_t *raw = HEX(in, "raw", &n);
    depz_vl53lx_histogram_raw r;
    if (depz_vl53lx_decode_histogram_raw(raw, n, &r) != 0) INVALID("vl53lx_decode_histogram_raw");
    o_obj();
    KI("interrupt_status", r.interrupt_status); KI("range_status", r.range_status);
    KI("report_status", r.report_status); KI("stream_count", r.stream_count);
    KI("dss_actual_effective_spads", r.dss_actual_effective_spads); KI("reference_phase", r.reference_phase);
    KI("vcsel_start", r.vcsel_start);
    o_key("bins"); o_arr();
    for (size_t b = 0; b < DEPZ_VL53LX_HISTOGRAM_BINS; b++) o_i64((int64_t)r.bins[b]);
    o_end_arr();
    o_end_obj();
}

/* ── VL53L8 advanced + CNH ── */

static void op_vl53l8_motion_config(const json_value *in)
{
    uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE];
    if (depz_vl53l8_motion_cfg_default_pack((int)I32(in, "resolution"), out) != 0) INVALID("resolution refused");
    o_obj(); KH("pack", out, sizeof out); o_end_obj();
}

static void op_vl53l8_detection_thresholds(const json_value *in)
{
    const json_value *list = larg(in, "thresholds");
    size_t n = json_arr_size(list);
    depz_vl53l8_threshold *th = (depz_vl53l8_threshold *)amalloc(sizeof *th * (n + 1));
    for (size_t k = 0; k < n; k++) {
        const json_value *e = json_arr_get(list, k);
        th[k].low_thresh = I32(e, "low_thresh");
        th[k].high_thresh = I32(e, "high_thresh");
        th[k].measurement = U8(e, "measurement");
        th[k].type = U8(e, "type");
        th[k].zone_num = U8(e, "zone_num");
        th[k].operation = U8(e, "operation");
    }
    uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], valid[8];
    if (depz_vl53l8_pack_thresholds_checked(th, n, start, valid) != 0) INVALID("scaled threshold overflows i32");
    o_obj(); KH("start_block", start, sizeof start); KH("valid_status", valid, sizeof valid); o_end_obj();
}

static void op_vl53l8_xtalk_margin(const json_value *in)
{
    double kcps = darg(in, "kcps");
    /* depz_vl53l8_set_xtalk_margin -> uld_set_xtalk_margin: `if (m > 10000) fail` before the write */
    uint32_t raw;
    if (depz_vl53l8_xtalk_margin_to_raw_checked(kcps, &raw) != 0) INVALID("xtalk margin %g refused", kcps);
    o_obj(); KU("raw", raw); o_end_obj();
}

#if DEPZ_DIFFTEST_HAS_IO
static void op_vl53l8_cnh_decode(const json_value *in)
{
    const json_value *c = field(in, "config");
    const json_value *am = field(c, "agg_map");
    int m[6];
    if (am->kind != JSON_ARRAY || json_arr_size(am) != 6) INVALID("agg_map: 6 integers");
    for (int k = 0; k < 6; k++) m[k] = (int)ival(json_arr_get(am, (size_t)k), "agg_map", INT32_MIN, INT32_MAX);
    depz_vl53l8_cnh_setup setup;
    if (depz_vl53l8_cnh_init_config_checked(&setup, (int)I32(c, "start_bin"), (int)I32(c, "num_bins"),
                                            (int)I32(c, "sub_sample")) != 0)
        INVALID("cnh_init_config_checked refused");
    if (depz_vl53l8_cnh_create_agg_map(&setup, (int)I32(c, "resolution"), m[0], m[1], m[2], m[3], m[4], m[5]) != 0)
        INVALID("cnh_create_agg_map refused");
    depz_vl53l8ch_cnh_config cfg;
    depz_vl53l8_cnh_decode_config(&setup, &cfg);
    size_t n; uint8_t *raw = HEX(in, "cnh_raw", &n);
    depz_vl53l8ch_cnh_frame *out = (depz_vl53l8ch_cnh_frame *)amalloc(sizeof *out);
    int rc = depz_vl53l8ch_decode_cnh(&cfg, raw, n, out);
    if (rc != 0) INVALID("decode_cnh rc=%d", rc);
    o_obj(); o_key("aggregates"); o_arr();
    for (int a = 0; a < out->nb_aggregates; a++) {
        o_obj();
        o_key("hist_raw"); o_arr(); for (int f = 0; f < out->feature_length; f++) o_i64(out->hist_raw[a][f]);
        o_end_arr();
        o_key("hist_scaler"); o_arr(); for (int f = 0; f < out->feature_length; f++) o_i64(out->hist_scaler[a][f]);
        o_end_arr();
        o_end_obj();
    }
    o_end_arr(); o_end_obj();
}
#endif

/* ── dispatch ────────────────────────────────────────────────────────────── */

typedef struct { const char *name; void (*fn)(const json_value *); } op_entry;

static const op_entry OPS[] = {
    {"crc", op_crc}, {"framing_encode", op_framing_encode}, {"framing_decode", op_framing_decode},
    {"identity", op_identity}, {"usb_identity", op_usb_identity}, {"probe_resolve", op_probe_resolve},
    {"common_encode", op_common_encode}, {"common_decode", op_common_decode},
    {"sync_time_math", op_sync_time_math}, {"sr04_encode", op_sr04_encode}, {"sr04_decode", op_sr04_decode},
    {"fwdepz_parse", op_fwdepz_parse},
    {"bno055_encode", op_bno055_encode}, {"bno055_decode", op_bno055_decode}, {"bno055_units", op_bno055_units},
    {"bno055_calib_stat", op_bno055_calib_stat}, {"bno055_calibration_profile", op_bno055_calibration_profile},
    {"bno055_axis_remap", op_bno055_axis_remap}, {"bno055_axis_remap_pack", op_bno055_axis_remap_pack},
    {"bno055_sensor_config", op_bno055_sensor_config}, {"bno055_block", op_bno055_block},
    {"bno086_shtp_header_pack", op_bno086_shtp_header_pack},
    {"bno086_shtp_header_unpack", op_bno086_shtp_header_unpack},
    {"bno086_shtp_tx_seq", op_bno086_shtp_tx_seq}, {"bno086_shtp_reassembly", op_bno086_shtp_reassembly},
    {"bno086_control_encode", op_bno086_control_encode}, {"bno086_input_cargo", op_bno086_input_cargo},
    {"bno086_gyro_rv", op_bno086_gyro_rv},
    {"tmf8829_encode", op_tmf8829_encode}, {"tmf8829_decode", op_tmf8829_decode},
    {"tmf8829_reassembly", op_tmf8829_reassembly}, {"tmf8829_frame", op_tmf8829_frame},
    {"tmf8829_measurements", op_tmf8829_measurements}, {"tmf8829_config_get", op_tmf8829_config_get},
    {"tmf8829_config_set", op_tmf8829_config_set}, {"tmf8829_frame_time", op_tmf8829_frame_time},
    {"vl53l4_encode", op_vl53l4_encode}, {"vl53l4_decode", op_vl53l4_decode},
    {"vl53l4_result_block", op_vl53l4_result_block}, {"vl53l4_timing_encode", op_vl53l4_timing_encode},
    {"vl53l4_timing_decode", op_vl53l4_timing_decode}, {"vl53l4_tuning_encode", op_vl53l4_tuning_encode},
    {"vl53l4_tuning_decode", op_vl53l4_tuning_decode},
    {"vl53l7_encode", op_vl53l7_encode}, {"vl53l7_decode", op_vl53l7_decode}, {"vl53l7_model", op_vl53l7_model},
    {"vl53lx_encode", op_vl53lx_encode}, {"vl53lx_decode", op_vl53lx_decode}, {"vl53lx_model", op_vl53lx_model},
    {"vl53lx_die_block", op_vl53lx_die_block}, {"vl53lx_l0x_raw", op_vl53lx_l0x_raw},
    {"vl53lx_histogram_raw", op_vl53lx_histogram_raw},
    {"vl53l8_motion_config", op_vl53l8_motion_config},
    {"vl53l8_detection_thresholds", op_vl53l8_detection_thresholds},
    {"vl53l8_xtalk_margin", op_vl53l8_xtalk_margin},
#if DEPZ_DIFFTEST_HAS_IO
    {"vl53l8_cnh_decode", op_vl53l8_cnh_decode},
#endif
};

/* Declared gaps: op -> reason (see the header comment). */
static const char *const UNSUPPORTED[][2] = {
    {"usb_serial_ordering", "no public serial-ordering function (static comparator inside depz_find_devices)"},
    {"bno086_record", "no contract 09 dataset recorder in the C SDK (reader only)"},
#if !DEPZ_DIFFTEST_HAS_IO
    {"vl53l8_cnh_decode", "built without the live-hardware layer (CNH setup helpers are in depz_sensor_io.h)"},
#endif
};

static void respond(const char *id_text, const char *op, const json_value *in)
{
    for (size_t i = 0; i < sizeof UNSUPPORTED / sizeof UNSUPPORTED[0]; i++) {
        if (is(op, UNSUPPORTED[i][0])) {
            o_reset();
            o_obj(); o_key("id"); o_sep(); o_puts(id_text); KS("error", "unsupported"); KS("detail", UNSUPPORTED[i][1]);
            o_end_obj();
            return;
        }
    }
    const op_entry *e = NULL;
    for (size_t i = 0; i < sizeof OPS / sizeof OPS[0]; i++)
        if (is(op, OPS[i].name)) e = &OPS[i];
    if (!e) {
        o_reset();
        o_obj(); o_key("id"); o_sep(); o_puts(id_text); KS("error", "unsupported"); KS("detail", "unknown op"); o_end_obj();
        return;
    }
    char *head = (char *)malloc(strlen(id_text) + 32);
    sprintf(head, "{\"id\":%s,\"out\":", id_text);
    o_reset();
    o_puts(head);
    free(head);
    if (setjmp(g_jb) == 0) {
        if (!in || in->kind != JSON_OBJECT) INVALID("`in` is not an object");
        e->fn(in);
        o_puts("}");
    } else {
        o_reset();
        o_obj(); o_key("id"); o_sep(); o_puts(id_text); KS("error", g_err_kind); KS("detail", g_err_detail); o_end_obj();
    }
    if (g_layer_live) { depz_shtp_free(&g_layer); g_layer_live = false; }
}

/* One request line (any length; NULL at end of input). */
static char *read_line(void)
{
    static char *buf;
    static size_t cap;
    size_t len = 0;
    if (!buf) { cap = 1 << 16; buf = (char *)malloc(cap); }
    for (;;) {
        if (!fgets(buf + len, (int)(cap - len), stdin)) return len ? buf : NULL;
        len += strlen(buf + len);
        if (len > 0 && buf[len - 1] == '\n') return buf;
        if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
    }
}

int main(void)
{
    char *line;
    while ((line = read_line()) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;
        json_value *req = json_parse(line);
        if (!req) { fprintf(stderr, "unparsable request line\n"); return 2; }
        const json_value *idv = json_obj_get(req, "id");
        char id_text[32];
        if (idv && idv->kind == JSON_INT && idv->range == 0) snprintf(id_text, sizeof id_text, "%lld", (long long)idv->i);
        else snprintf(id_text, sizeof id_text, "null");
        respond(id_text, json_as_str(json_obj_get(req, "op")), json_obj_get(req, "in"));
        arena_free();
        json_free(req);
        fwrite(ob, 1, ob_len, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    free(ob);
    return 0;
}
