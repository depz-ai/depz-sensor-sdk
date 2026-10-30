/*
 * `.depzdata` datasets (contract 09): the JSON value tree records are made
 * of, the reader (JSON Lines, plain or a gzip container), the writer (a gzip
 * container of stored blocks for `.gz` paths — no compression dependency),
 * the record value of the codec-layer result types, and the tick-driven
 * player of contract 09 §Player. Mirrors the Python reference `dataset.py`;
 * the live layer (src/io/recorder.c) builds the remaining record kinds and
 * the session recorder on top of this.
 *
 * Dependency-free: a compact JSON parser / serializer and a small inflate (a
 * port of zlib's `puff`, as the Rust SDK's) live here.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* gmtime_r, clock_gettime */
#endif
#include "internal.h" /* depz_fail: a damaged file names its line */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ======================================================================== */
/* JSON value tree                                                            */
/* ======================================================================== */
struct depz_json {
    depz_json_kind kind;
    bool b;
    int64_t i;
    double d;
    char *s;
    size_t slen;
    struct depz_json **items;
    char **keys;
    size_t count, cap;
};

static depz_json *j_new(depz_json_kind k)
{
    depz_json *v = (depz_json *)calloc(1, sizeof(depz_json));
    if (v) v->kind = k;
    return v;
}

depz_json *depz_json_null(void) { return j_new(DEPZ_JSON_NULL); }
depz_json *depz_json_bool(bool b) { depz_json *v = j_new(DEPZ_JSON_BOOL); if (v) v->b = b; return v; }
depz_json *depz_json_int(int64_t i) { depz_json *v = j_new(DEPZ_JSON_INT); if (v) v->i = i; return v; }
depz_json *depz_json_double(double d) { depz_json *v = j_new(DEPZ_JSON_DOUBLE); if (v) v->d = d; return v; }
depz_json *depz_json_array(void) { return j_new(DEPZ_JSON_ARRAY); }
depz_json *depz_json_object(void) { return j_new(DEPZ_JSON_OBJECT); }

depz_json *depz_json_string_n(const char *s, size_t n)
{
    depz_json *v = j_new(DEPZ_JSON_STRING);
    if (!v) return NULL;
    v->s = (char *)malloc(n + 1);
    if (!v->s) { free(v); return NULL; }
    if (n) memcpy(v->s, s, n);
    v->s[n] = '\0';
    v->slen = n;
    return v;
}

depz_json *depz_json_string(const char *s) { return depz_json_string_n(s ? s : "", s ? strlen(s) : 0); }

void depz_json_free(depz_json *v)
{
    if (!v) return;
    free(v->s);
    for (size_t i = 0; i < v->count; i++) {
        depz_json_free(v->items[i]);
        if (v->keys) free(v->keys[i]);
    }
    free(v->items);
    free(v->keys);
    free(v);
}

static int j_grow(depz_json *c, bool keyed)
{
    if (c->count < c->cap) return 0;
    size_t ncap = c->cap ? c->cap * 2 : 8;
    depz_json **ni = (depz_json **)realloc(c->items, ncap * sizeof(depz_json *));
    if (!ni) return -1;
    c->items = ni;
    if (keyed) {
        char **nk = (char **)realloc(c->keys, ncap * sizeof(char *));
        if (!nk) return -1;
        c->keys = nk;
    }
    c->cap = ncap;
    return 0;
}

int depz_json_push(depz_json *array, depz_json *item)
{
    if (!array || array->kind != DEPZ_JSON_ARRAY || !item || j_grow(array, false) != 0) {
        depz_json_free(item);
        return -1;
    }
    array->items[array->count++] = item;
    return 0;
}

int depz_json_set(depz_json *object, const char *key, depz_json *item)
{
    if (!object || object->kind != DEPZ_JSON_OBJECT || !key || !item) { depz_json_free(item); return -1; }
    for (size_t i = 0; i < object->count; i++) {
        if (strcmp(object->keys[i], key) == 0) {
            depz_json_free(object->items[i]);
            object->items[i] = item;
            return 0;
        }
    }
    char *k = (char *)malloc(strlen(key) + 1);
    if (!k || j_grow(object, true) != 0) { free(k); depz_json_free(item); return -1; }
    strcpy(k, key);
    object->keys[object->count] = k;
    object->items[object->count] = item;
    object->count++;
    return 0;
}

depz_json_kind depz_json_kind_of(const depz_json *v) { return v ? v->kind : DEPZ_JSON_NULL; }
bool depz_json_bool_value(const depz_json *v) { return v && v->kind == DEPZ_JSON_BOOL && v->b; }

int64_t depz_json_int_value(const depz_json *v)
{
    if (!v) return 0;
    if (v->kind == DEPZ_JSON_INT) return v->i;
    if (v->kind == DEPZ_JSON_DOUBLE) return (int64_t)v->d;
    return 0;
}

double depz_json_double_value(const depz_json *v)
{
    if (!v) return 0.0;
    if (v->kind == DEPZ_JSON_DOUBLE) return v->d;
    if (v->kind == DEPZ_JSON_INT) return (double)v->i;
    return 0.0;
}

const char *depz_json_string_value(const depz_json *v)
{
    return v && v->kind == DEPZ_JSON_STRING ? v->s : NULL;
}

size_t depz_json_string_len(const depz_json *v)
{
    return v && v->kind == DEPZ_JSON_STRING ? v->slen : 0;
}

size_t depz_json_len(const depz_json *v)
{
    return v && (v->kind == DEPZ_JSON_ARRAY || v->kind == DEPZ_JSON_OBJECT) ? v->count : 0;
}

const depz_json *depz_json_at(const depz_json *v, size_t i)
{
    return i < depz_json_len(v) ? v->items[i] : NULL;
}

const char *depz_json_key_at(const depz_json *v, size_t i)
{
    return v && v->kind == DEPZ_JSON_OBJECT && i < v->count ? v->keys[i] : NULL;
}

const depz_json *depz_json_get(const depz_json *v, const char *key)
{
    if (!v || v->kind != DEPZ_JSON_OBJECT || !key) return NULL;
    for (size_t i = 0; i < v->count; i++)
        if (strcmp(v->keys[i], key) == 0) return v->items[i];
    return NULL;
}

bool depz_json_equal(const depz_json *a, const depz_json *b)
{
    if (!a || !b) return a == b;
    const bool na = a->kind == DEPZ_JSON_INT || a->kind == DEPZ_JSON_DOUBLE;
    const bool nb = b->kind == DEPZ_JSON_INT || b->kind == DEPZ_JSON_DOUBLE;
    if (na && nb) {
        if (a->kind == DEPZ_JSON_INT && b->kind == DEPZ_JSON_INT) return a->i == b->i;
        return depz_json_double_value(a) == depz_json_double_value(b);
    }
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case DEPZ_JSON_NULL: return true;
    case DEPZ_JSON_BOOL: return a->b == b->b;
    case DEPZ_JSON_STRING: return a->slen == b->slen && memcmp(a->s, b->s, a->slen) == 0;
    case DEPZ_JSON_ARRAY:
        if (a->count != b->count) return false;
        for (size_t i = 0; i < a->count; i++)
            if (!depz_json_equal(a->items[i], b->items[i])) return false;
        return true;
    case DEPZ_JSON_OBJECT:
        if (a->count != b->count) return false;
        for (size_t i = 0; i < a->count; i++)
            if (!depz_json_equal(a->items[i], depz_json_get(b, a->keys[i]))) return false;
        return true;
    default: return false;
    }
}

/* ---- parser ------------------------------------------------------------- */

/* Contract 08 «Line rules»: arrays and objects nest at most this deep (the
 * outermost value is level 1) — deeper is a parse error, not a stack overflow. */
#define J_MAX_DEPTH 256

typedef struct { const char *p, *end; bool error; unsigned depth; } jparser;

static depz_json *j_parse_value(jparser *ps);

static void j_skip_ws(jparser *ps)
{
    while (ps->p < ps->end && (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r'))
        ps->p++;
}

static int j_peek(const jparser *ps) { return ps->p < ps->end ? (unsigned char)*ps->p : -1; }

static void j_put_utf8(char **out, size_t *len, size_t *cap, uint32_t cp)
{
    if (*len + 5 >= *cap) { *cap = *cap * 2 + 8; *out = (char *)realloc(*out, *cap); }
    char *o = *out + *len;
    if (cp < 0x80) { o[0] = (char)cp; *len += 1; }
    else if (cp < 0x800) { o[0] = (char)(0xC0 | (cp >> 6)); o[1] = (char)(0x80 | (cp & 0x3F)); *len += 2; }
    else if (cp < 0x10000) {
        o[0] = (char)(0xE0 | (cp >> 12)); o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F)); *len += 3;
    } else {
        o[0] = (char)(0xF0 | (cp >> 18)); o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        o[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); o[3] = (char)(0x80 | (cp & 0x3F)); *len += 4;
    }
}

static int j_hex4(jparser *ps, uint32_t *cp)
{
    uint32_t v = 0;
    for (int k = 0; k < 4; k++) {
        int h = j_peek(ps);
        if (h < 0) return -1;
        ps->p++;
        v <<= 4;
        if (h >= '0' && h <= '9') v |= (uint32_t)(h - '0');
        else if (h >= 'a' && h <= 'f') v |= (uint32_t)(h - 'a' + 10);
        else if (h >= 'A' && h <= 'F') v |= (uint32_t)(h - 'A' + 10);
        else return -1;
    }
    *cp = v;
    return 0;
}

/* The string body after the opening quote; *n receives its length. */
static char *j_parse_string_raw(jparser *ps, size_t *n)
{
    if (j_peek(ps) != '"') { ps->error = true; return NULL; }
    ps->p++;
    size_t cap = 16, len = 0;
    char *out = (char *)malloc(cap);
    if (!out) { ps->error = true; return NULL; }
    for (;;) {
        int c = j_peek(ps);
        if (c < 0) { ps->error = true; free(out); return NULL; }
        ps->p++;
        if (c == '"') break;
        if (c == '\\') {
            int e = j_peek(ps);
            if (e < 0) { ps->error = true; free(out); return NULL; }
            ps->p++;
            switch (e) {
            case '"': c = '"'; break;   case '\\': c = '\\'; break;
            case '/': c = '/'; break;   case 'n': c = '\n'; break;
            case 't': c = '\t'; break;  case 'r': c = '\r'; break;
            case 'b': c = '\b'; break;  case 'f': c = '\f'; break;
            case 'u': {
                uint32_t cp;
                if (j_hex4(ps, &cp) != 0) { ps->error = true; free(out); return NULL; }
                if (cp >= 0xD800 && cp < 0xDC00 && ps->end - ps->p >= 6 && ps->p[0] == '\\' && ps->p[1] == 'u') {
                    jparser save = *ps;
                    uint32_t lo;
                    ps->p += 2;
                    if (j_hex4(ps, &lo) == 0 && lo >= 0xDC00 && lo < 0xE000)
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    else
                        *ps = save;
                }
                j_put_utf8(&out, &len, &cap, cp);
                continue;
            }
            default: ps->error = true; free(out); return NULL;
            }
        }
        if (c < 0x20) { ps->error = true; free(out); return NULL; }   /* raw control character */
        if (len + 2 >= cap) { cap *= 2; out = (char *)realloc(out, cap); }
        out[len++] = (char)c;
    }
    out[len] = '\0';
    *n = len;
    return out;
}

static bool j_digits(jparser *ps)   /* skips ASCII digits; false when there was none */
{
    const char *from = ps->p;
    while (j_peek(ps) >= '0' && j_peek(ps) <= '9') ps->p++;
    return ps->p > from;
}

/* RFC 8259: -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)? */
static depz_json *j_parse_number(jparser *ps)
{
    const char *start = ps->p;
    bool is_double = false;
    if (j_peek(ps) == '-') ps->p++;
    if (j_peek(ps) == '0') ps->p++;
    else if (!j_digits(ps)) { ps->error = true; return NULL; }
    if (j_peek(ps) == '.') {
        is_double = true; ps->p++;
        if (!j_digits(ps)) { ps->error = true; return NULL; }
    }
    if (j_peek(ps) == 'e' || j_peek(ps) == 'E') {
        is_double = true; ps->p++;
        if (j_peek(ps) == '+' || j_peek(ps) == '-') ps->p++;
        if (!j_digits(ps)) { ps->error = true; return NULL; }
    }
    char stackbuf[64], *buf = stackbuf;
    size_t n = (size_t)(ps->p - start);
    if (n >= sizeof(stackbuf) && !(buf = (char *)malloc(n + 1))) { ps->error = true; return NULL; }
    memcpy(buf, start, n);
    buf[n] = '\0';
    depz_json *v = NULL;
    if (!is_double) {
        char *endp;
        errno = 0;
        long long ll = strtoll(buf, &endp, 10);
        if (*endp == '\0' && errno != ERANGE) v = depz_json_int(ll);
        /* out of int64 range: keep it as a real */
    }
    if (!v) v = depz_json_double(strtod(buf, NULL));
    if (buf != stackbuf) free(buf);
    if (!v) ps->error = true;
    return v;
}

static depz_json *j_parse_array(jparser *ps)
{
    ps->p++;
    depz_json *v = depz_json_array();
    if (!v) { ps->error = true; return NULL; }
    j_skip_ws(ps);
    if (j_peek(ps) == ']') { ps->p++; return v; }
    for (;;) {
        j_skip_ws(ps);
        depz_json *item = j_parse_value(ps);
        if (ps->error) { depz_json_free(item); depz_json_free(v); return NULL; }
        if (depz_json_push(v, item) != 0) { ps->error = true; depz_json_free(v); return NULL; }
        j_skip_ws(ps);
        if (j_peek(ps) == ',') { ps->p++; continue; }
        if (j_peek(ps) == ']') { ps->p++; break; }
        ps->error = true; depz_json_free(v); return NULL;
    }
    return v;
}

static depz_json *j_parse_object(jparser *ps)
{
    ps->p++;
    depz_json *v = depz_json_object();
    if (!v) { ps->error = true; return NULL; }
    j_skip_ws(ps);
    if (j_peek(ps) == '}') { ps->p++; return v; }
    for (;;) {
        size_t klen;
        j_skip_ws(ps);
        char *key = j_parse_string_raw(ps, &klen);
        if (ps->error) { free(key); depz_json_free(v); return NULL; }
        j_skip_ws(ps);
        if (j_peek(ps) != ':') { ps->error = true; free(key); depz_json_free(v); return NULL; }
        ps->p++;
        j_skip_ws(ps);
        depz_json *val = j_parse_value(ps);
        if (ps->error) { free(key); depz_json_free(val); depz_json_free(v); return NULL; }
        int rc = depz_json_set(v, key, val);
        free(key);
        if (rc != 0) { ps->error = true; depz_json_free(v); return NULL; }
        j_skip_ws(ps);
        if (j_peek(ps) == ',') { ps->p++; continue; }
        if (j_peek(ps) == '}') { ps->p++; break; }
        ps->error = true; depz_json_free(v); return NULL;
    }
    return v;
}

static bool j_lit(jparser *ps, const char *lit)
{
    size_t n = strlen(lit);
    if ((size_t)(ps->end - ps->p) < n || strncmp(ps->p, lit, n) != 0) return false;
    ps->p += n;
    return true;
}

static depz_json *j_parse_value(jparser *ps)
{
    j_skip_ws(ps);
    int c = j_peek(ps);
    if (c == '"') {
        size_t n;
        char *s = j_parse_string_raw(ps, &n);
        if (ps->error) return NULL;
        depz_json *v = j_new(DEPZ_JSON_STRING);
        if (!v) { free(s); ps->error = true; return NULL; }
        v->s = s;
        v->slen = n;
        return v;
    }
    if (c == '{' || c == '[') {
        depz_json *v;
        if (ps->depth == J_MAX_DEPTH) { ps->error = true; return NULL; }
        ps->depth++;
        v = c == '{' ? j_parse_object(ps) : j_parse_array(ps);
        ps->depth--;
        return v;
    }
    if (c == '-' || (c >= '0' && c <= '9')) return j_parse_number(ps);
    if (j_lit(ps, "true")) return depz_json_bool(true);
    if (j_lit(ps, "false")) return depz_json_bool(false);
    if (j_lit(ps, "null")) return depz_json_null();
    ps->error = true;
    return NULL;
}

depz_json *depz_json_parse(const char *text, size_t len)
{
    if (!text) return NULL;
    jparser ps = { text, text + len, false, 0 };
    depz_json *v = j_parse_value(&ps);
    if (ps.error) { depz_json_free(v); return NULL; }
    j_skip_ws(&ps);
    if (ps.p != ps.end) { depz_json_free(v); return NULL; }
    return v;
}

/* ---- serializer --------------------------------------------------------- */

typedef struct { char *out; size_t cap, len; } jout;

static void jo_put(jout *o, const char *s, size_t n)
{
    if (o->out && o->len < o->cap) {
        size_t room = o->cap - 1 - o->len;
        size_t k = n < room ? n : room;
        if (k) memcpy(o->out + o->len, s, k);
    }
    o->len += n;
}

static void jo_puts(jout *o, const char *s) { jo_put(o, s, strlen(s)); }

static void jo_string(jout *o, const char *s, size_t n)
{
    static const char *H = "0123456789abcdef";
    jo_put(o, "\"", 1);
    for (size_t i = 0; i < n;) {
        unsigned c = (unsigned char)s[i];
        if (c == '"') { jo_put(o, "\\\"", 2); i++; }
        else if (c == '\\') { jo_put(o, "\\\\", 2); i++; }
        else if (c == '\n') { jo_put(o, "\\n", 2); i++; }
        else if (c == '\r') { jo_put(o, "\\r", 2); i++; }
        else if (c == '\t') { jo_put(o, "\\t", 2); i++; }
        else if (c < 0x20 || c == 0x7F) {
            char b[7] = { '\\', 'u', '0', '0', H[c >> 4], H[c & 15], 0 };
            jo_put(o, b, 6); i++;
        } else if (c < 0x80) { jo_put(o, s + i, 1); i++; }
        else {
            /* decode one UTF-8 sequence; an invalid byte becomes U+FFFD */
            uint32_t cp = 0xFFFD;
            size_t need = 0;
            if (c >= 0xC2 && c <= 0xDF) { need = 1; cp = c & 0x1F; }
            else if (c >= 0xE0 && c <= 0xEF) { need = 2; cp = c & 0x0F; }
            else if (c >= 0xF0 && c <= 0xF4) { need = 3; cp = c & 0x07; }
            size_t j = i + 1, k;
            for (k = 0; k < need && j < n; k++, j++) {
                unsigned b = (unsigned char)s[j];
                if ((b & 0xC0) != 0x80) break;
                cp = (cp << 6) | (b & 0x3F);
            }
            if (need == 0 || k < need) { cp = 0xFFFD; j = i + 1; }
            i = j;
            char b[16];
            if (cp < 0x10000) {
                snprintf(b, sizeof b, "\\u%04x", (unsigned)cp);
                jo_put(o, b, 6);
            } else {
                cp -= 0x10000;
                snprintf(b, sizeof b, "\\u%04x\\u%04x", (unsigned)(0xD800 | (cp >> 10)), (unsigned)(0xDC00 | (cp & 0x3FF)));
                jo_put(o, b, 12);
            }
        }
    }
    jo_put(o, "\"", 1);
}

/* The shortest decimal that reads back to the same double, with a fraction
 * mark (as Python's repr: 2.0 stays a real). */
static void jo_double(jout *o, double d)
{
    char b[40];
    if (isnan(d) || isinf(d)) { jo_puts(o, "null"); return; }
    for (int prec = 15; prec <= 17; prec++) {
        snprintf(b, sizeof b, "%.*g", prec, d);
        if (strtod(b, NULL) == d) break;
    }
    if (!strpbrk(b, ".eEn")) strcat(b, ".0");
    jo_puts(o, b);
}

static void jo_value(jout *o, const depz_json *v)
{
    char b[32];
    if (!v) { jo_puts(o, "null"); return; }
    switch (v->kind) {
    case DEPZ_JSON_NULL: jo_puts(o, "null"); break;
    case DEPZ_JSON_BOOL: jo_puts(o, v->b ? "true" : "false"); break;
    case DEPZ_JSON_INT: snprintf(b, sizeof b, "%lld", (long long)v->i); jo_puts(o, b); break;
    case DEPZ_JSON_DOUBLE: jo_double(o, v->d); break;
    case DEPZ_JSON_STRING: jo_string(o, v->s, v->slen); break;
    case DEPZ_JSON_ARRAY:
        jo_put(o, "[", 1);
        for (size_t i = 0; i < v->count; i++) { if (i) jo_put(o, ",", 1); jo_value(o, v->items[i]); }
        jo_put(o, "]", 1);
        break;
    case DEPZ_JSON_OBJECT:
        jo_put(o, "{", 1);
        for (size_t i = 0; i < v->count; i++) {
            if (i) jo_put(o, ",", 1);
            jo_string(o, v->keys[i], strlen(v->keys[i]));
            jo_put(o, ":", 1);
            jo_value(o, v->items[i]);
        }
        jo_put(o, "}", 1);
        break;
    }
}

size_t depz_json_dump(const depz_json *v, char *out, size_t cap)
{
    jout o = { out, cap, 0 };
    jo_value(&o, v);
    if (out && cap) out[o.len < cap ? o.len : cap - 1] = '\0';
    return o.len;
}

char *depz_json_dumps(const depz_json *v)
{
    size_t n = depz_json_dump(v, NULL, 0);
    char *s = (char *)malloc(n + 1);
    if (!s) return NULL;
    depz_json_dump(v, s, n + 1);
    return s;
}

/* ======================================================================== */
/* gzip: a running CRC-32, inflate (zlib's puff), a stored-block writer       */
/* ======================================================================== */

static uint32_t crc32_update(uint32_t crc, const uint8_t *p, size_t n)
{
    static uint32_t table[256];
    static bool ready;
    if (!ready) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        ready = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < n; i++) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static bool is_gzip(const uint8_t *p, size_t n) { return n >= 2 && p[0] == 0x1F && p[1] == 0x8B; }

#define INF_MAX_BITS 15

typedef struct { uint16_t count[INF_MAX_BITS + 1]; uint16_t *symbol; } huffman;

typedef struct {
    const uint8_t *src;
    size_t pos, len;
    uint32_t bit_buf, bit_cnt;
    uint8_t *out;
    size_t out_len, out_cap;
    bool error;
} inflater;

static int inf_out_push(inflater *f, uint8_t b)
{
    if (f->out_len == f->out_cap) {
        size_t ncap = f->out_cap ? f->out_cap * 2 : 4096;
        uint8_t *no = (uint8_t *)realloc(f->out, ncap);
        if (!no) { f->error = true; return -1; }
        f->out = no;
        f->out_cap = ncap;
    }
    f->out[f->out_len++] = b;
    return 0;
}

static uint32_t inf_bits(inflater *f, uint32_t need)
{
    while (f->bit_cnt < need) {
        if (f->pos >= f->len) { f->error = true; return 0; }
        f->bit_buf |= (uint32_t)f->src[f->pos++] << f->bit_cnt;
        f->bit_cnt += 8;
    }
    uint32_t v = need == 32 ? f->bit_buf : f->bit_buf & ((1u << need) - 1);
    f->bit_buf >>= need;
    f->bit_cnt -= need;
    return v;
}

static int huffman_build(huffman *h, const uint8_t *lengths, size_t n)
{
    memset(h->count, 0, sizeof h->count);
    for (size_t i = 0; i < n; i++) h->count[lengths[i]]++;
    int left = 1;
    for (int len = 1; len <= INF_MAX_BITS; len++) {
        left = left * 2 - h->count[len];
        if (left < 0) return -1;
    }
    uint16_t offs[INF_MAX_BITS + 2] = {0};
    for (int len = 1; len <= INF_MAX_BITS; len++) offs[len + 1] = (uint16_t)(offs[len] + h->count[len]);
    h->symbol = (uint16_t *)calloc(n ? n : 1, sizeof(uint16_t));
    if (!h->symbol) return -1;
    for (size_t s = 0; s < n; s++)
        if (lengths[s]) h->symbol[offs[lengths[s]]++] = (uint16_t)s;
    return 0;
}

static int inf_decode(inflater *f, const huffman *h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= INF_MAX_BITS; len++) {
        code |= (int)inf_bits(f, 1);
        if (f->error) return -1;
        int count = h->count[len];
        if (code - count < first) return h->symbol[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    f->error = true;
    return -1;
}

static const uint16_t LEN_BASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const uint8_t LEN_EXTRA[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const uint16_t DIST_BASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const uint8_t DIST_EXTRA[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
static const uint8_t CL_ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

static int inf_codes(inflater *f, const huffman *lit, const huffman *dist)
{
    for (;;) {
        int sym = inf_decode(f, lit);
        if (sym < 0) return -1;
        if (sym < 256) { if (inf_out_push(f, (uint8_t)sym)) return -1; continue; }
        if (sym == 256) return 0;
        sym -= 257;
        if (sym >= 29) { f->error = true; return -1; }
        size_t len = LEN_BASE[sym] + inf_bits(f, LEN_EXTRA[sym]);
        int d = inf_decode(f, dist);
        if (d < 0 || d >= 30) { f->error = true; return -1; }
        size_t back = DIST_BASE[d] + inf_bits(f, DIST_EXTRA[d]);
        if (f->error || back > f->out_len) { f->error = true; return -1; }
        size_t from = f->out_len - back;
        for (size_t k = 0; k < len; k++)
            if (inf_out_push(f, f->out[from + k])) return -1;
    }
}

static int inf_stored(inflater *f)
{
    f->bit_buf = 0;
    f->bit_cnt = 0;
    if (f->pos + 4 > f->len) { f->error = true; return -1; }
    unsigned len = f->src[f->pos] | (f->src[f->pos + 1] << 8);
    unsigned nlen = f->src[f->pos + 2] | (f->src[f->pos + 3] << 8);
    if (len != (~nlen & 0xFFFF)) { f->error = true; return -1; }
    f->pos += 4;
    if (f->pos + len > f->len) { f->error = true; return -1; }
    for (unsigned i = 0; i < len; i++)
        if (inf_out_push(f, f->src[f->pos + i])) return -1;
    f->pos += len;
    return 0;
}

static int inf_fixed(inflater *f)
{
    uint8_t lengths[288];
    memset(lengths, 8, 144);
    memset(lengths + 144, 9, 112);
    memset(lengths + 256, 7, 24);
    memset(lengths + 280, 8, 8);
    uint8_t dl[30];
    memset(dl, 5, sizeof dl);
    huffman lit = {{0}, NULL}, dist = {{0}, NULL};
    int rc = -1;
    if (huffman_build(&lit, lengths, 288) == 0 && huffman_build(&dist, dl, 30) == 0) rc = inf_codes(f, &lit, &dist);
    else f->error = true;
    free(lit.symbol);
    free(dist.symbol);
    return rc;
}

static int inf_dynamic(inflater *f)
{
    size_t nlen = inf_bits(f, 5) + 257, ndist = inf_bits(f, 5) + 1, ncode = inf_bits(f, 4) + 4;
    if (f->error || nlen > 286 || ndist > 30) { f->error = true; return -1; }
    uint8_t cl[19] = {0};
    for (size_t i = 0; i < ncode; i++) cl[CL_ORDER[i]] = (uint8_t)inf_bits(f, 3);
    huffman clc = {{0}, NULL}, lit = {{0}, NULL}, dist = {{0}, NULL};
    uint8_t lengths[320] = {0};
    int rc = -1;
    if (f->error || huffman_build(&clc, cl, 19) != 0) { f->error = true; goto out; }
    for (size_t i = 0; i < nlen + ndist;) {
        int sym = inf_decode(f, &clc);
        if (sym < 0) goto out;
        uint8_t value = 0;
        size_t repeat = 1;
        if (sym < 16) value = (uint8_t)sym;
        else if (sym == 16) {
            if (i == 0) { f->error = true; goto out; }
            value = lengths[i - 1];
            repeat = 3 + inf_bits(f, 2);
        } else if (sym == 17) repeat = 3 + inf_bits(f, 3);
        else repeat = 11 + inf_bits(f, 7);
        if (f->error || i + repeat > nlen + ndist) { f->error = true; goto out; }
        memset(lengths + i, value, repeat);
        i += repeat;
    }
    if (lengths[256] == 0) { f->error = true; goto out; }
    if (huffman_build(&lit, lengths, nlen) != 0 || huffman_build(&dist, lengths + nlen, ndist) != 0) {
        f->error = true;
        goto out;
    }
    rc = inf_codes(f, &lit, &dist);
out:
    free(clc.symbol);
    free(lit.symbol);
    free(dist.symbol);
    return rc;
}

/* The content of a gzip file (members one after the other). NULL on error. */
static uint8_t *gunzip(const uint8_t *data, size_t len, size_t *out_len)
{
    inflater f = { data, 0, len, 0, 0, NULL, 0, 0, false };
    while (f.pos < len) {
        if (f.pos + 10 > len || data[f.pos] != 0x1F || data[f.pos + 1] != 0x8B || data[f.pos + 2] != 8) goto fail;
        unsigned flags = data[f.pos + 3];
        f.pos += 10;
        if (flags & 0x04) {
            if (f.pos + 2 > len) goto fail;
            f.pos += 2 + (size_t)(data[f.pos] | (data[f.pos + 1] << 8));
        }
        for (unsigned bit = 0x08; bit <= 0x10; bit <<= 1) {
            if (flags & bit) {
                while (f.pos < len && data[f.pos]) f.pos++;
                if (f.pos >= len) goto fail;
                f.pos++;
            }
        }
        if (flags & 0x02) f.pos += 2;
        size_t start = f.out_len;
        f.bit_buf = 0;
        f.bit_cnt = 0;
        for (;;) {
            uint32_t last = inf_bits(&f, 1), type = inf_bits(&f, 2);
            if (f.error) goto fail;
            int rc = type == 0 ? inf_stored(&f) : type == 1 ? inf_fixed(&f) : type == 2 ? inf_dynamic(&f) : -1;
            if (rc != 0 || f.error) goto fail;
            if (last) break;
        }
        if (f.pos + 8 > len) goto fail;
        uint32_t crc = (uint32_t)data[f.pos] | (uint32_t)data[f.pos + 1] << 8 | (uint32_t)data[f.pos + 2] << 16 | (uint32_t)data[f.pos + 3] << 24;
        uint32_t isize = (uint32_t)data[f.pos + 4] | (uint32_t)data[f.pos + 5] << 8 | (uint32_t)data[f.pos + 6] << 16 | (uint32_t)data[f.pos + 7] << 24;
        if (crc != crc32_update(0, f.out + start, f.out_len - start) || isize != (uint32_t)(f.out_len - start)) goto fail;
        f.pos += 8;
    }
    if (!f.out) f.out = (uint8_t *)calloc(1, 1);
    *out_len = f.out_len;
    return f.out;
fail:
    free(f.out);
    return NULL;
}

/* A gzip stream of stored blocks over a FILE. */
#define GZ_STORED_MAX 0xFFFFu

typedef struct {
    FILE *fp;
    uint8_t buf[GZ_STORED_MAX];
    size_t buf_len;
    uint32_t crc, size;
} gz_writer;

static int gz_begin(gz_writer *g, FILE *fp)
{
    static const uint8_t head[10] = {0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0xFF};
    g->fp = fp;
    g->buf_len = 0;
    g->crc = 0;
    g->size = 0;
    return fwrite(head, 1, sizeof head, fp) == sizeof head ? 0 : -1;
}

static int gz_block(gz_writer *g, bool last)
{
    uint8_t h[5];
    h[0] = last ? 1 : 0;
    h[1] = (uint8_t)(g->buf_len & 0xFF);
    h[2] = (uint8_t)(g->buf_len >> 8);
    h[3] = (uint8_t)(~g->buf_len & 0xFF);
    h[4] = (uint8_t)((~g->buf_len >> 8) & 0xFF);
    if (fwrite(h, 1, 5, g->fp) != 5) return -1;
    if (g->buf_len && fwrite(g->buf, 1, g->buf_len, g->fp) != g->buf_len) return -1;
    g->buf_len = 0;
    return 0;
}

static int gz_write(gz_writer *g, const uint8_t *p, size_t n)
{
    while (n) {
        size_t k = GZ_STORED_MAX - g->buf_len;
        if (k > n) k = n;
        memcpy(g->buf + g->buf_len, p, k);
        g->crc = crc32_update(g->crc, p, k);
        g->size += (uint32_t)k;
        g->buf_len += k;
        p += k;
        n -= k;
        if (g->buf_len == GZ_STORED_MAX && gz_block(g, false) != 0) return -1;
    }
    return 0;
}

static int gz_finish(gz_writer *g)
{
    if (gz_block(g, true) != 0) return -1;
    uint8_t t[8];
    for (int i = 0; i < 4; i++) { t[i] = (uint8_t)(g->crc >> (8 * i)); t[4 + i] = (uint8_t)(g->size >> (8 * i)); }
    return fwrite(t, 1, 8, g->fp) == 8 ? 0 : -1;
}

/* ======================================================================== */
/* dataset reader                                                            */
/* ======================================================================== */
struct depz_dataset {
    char schema[32];
    char created_utc[40];
    char *note;
    depz_json *header;            /* owned */
    depz_dataset_device *devices;
    size_t device_count;
    depz_json **record_json;      /* owned line docs, in merged order */
    depz_dataset_record *records;
    size_t record_count;
};

/* A record line while it is sorted: its document, `t` and its index in the
 * file (qsort is not stable; the index keeps the file order on equal `t`). */
typedef struct { depz_json *doc; int64_t t; size_t idx; } rec_entry;

static int rec_cmp(const void *pa, const void *pb)
{
    const rec_entry *a = (const rec_entry *)pa;
    const rec_entry *b = (const rec_entry *)pb;
    if (a->t < b->t) return -1;
    if (a->t > b->t) return 1;
    if (a->idx < b->idx) return -1;
    if (a->idx > b->idx) return 1;
    return 0;
}

static void str_copy(const depz_json *v, char *out, size_t cap)
{
    out[0] = '\0';
    if (v && v->kind == DEPZ_JSON_STRING && cap) {
        strncpy(out, v->s, cap - 1);
        out[cap - 1] = '\0';
    }
}

/* The next line of text[0..len): [*line, *line + *llen) without its "\n";
 * false past the last one. Contract 08 «Line rules»: LF alone ends a line;
 * a final "\n" does not open a last line (it would be blank anyway). */
static bool next_line(const char *text, size_t len, size_t *pos, const char **line, size_t *llen)
{
    size_t i = *pos, j;
    if (i >= len) return false;
    for (j = i; j < len && text[j] != '\n'; j++) {}
    *line = text + i;
    *llen = j - i;
    *pos = j + 1;
    return true;
}

/* Contract 08 «Line rules»: only JSON whitespace (space, tab, CR) is blank. */
static bool line_blank(const char *p, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (p[i] != ' ' && p[i] != '\t' && p[i] != '\r') return false;
    return true;
}

/* A damaged line is the caller's bad argument (contract 07 §3): the whole
 * file is refused, the message names the line — never skipped or coerced. */
static int bad_line(size_t lineno, const char *what)
{
    return depz_fail(DEPZ_E_ARG, "dataset: line %zu: %s", lineno, what);
}

static int grow(void **items, size_t *cap, size_t need, size_t item_size)
{
    size_t nc = *cap ? *cap : 64;
    void *ni;
    if (need <= *cap) return 0;
    while (nc < need) nc *= 2;
    ni = realloc(*items, nc * item_size);
    if (!ni) return -1;
    *items = ni;
    *cap = nc;
    return 0;
}

static depz_dataset *dataset_parse_text(const char *text, size_t len)
{
    depz_dataset *ds;
    const char *line;
    size_t llen, pos = 0, lineno = 1, i;
    depz_json *doc = NULL;
    rec_entry *ents = NULL;
    size_t nrec = 0, rcap = 0;

    /* line 1 is the header, blank or not (the Python reference's readline) */
    if (!next_line(text, len, &pos, &line, &llen)) {
        depz_fail(DEPZ_E_ARG, "dataset: empty file (no header line)");
        return NULL;
    }
    ds = (depz_dataset *)calloc(1, sizeof(*ds));
    if (!ds) { depz_fail(DEPZ_E_NOMEM, "dataset: out of memory"); return NULL; }

    ds->header = depz_json_parse(line, llen);
    if (!ds->header) { bad_line(1, "header is not JSON"); goto fail; }
    if (ds->header->kind != DEPZ_JSON_OBJECT) { bad_line(1, "header is not a JSON object"); goto fail; }
    {
        const depz_json *sc = depz_json_get(ds->header, "schema");
        const depz_json *note;
        const char *ns;
        if (!sc || sc->kind != DEPZ_JSON_STRING || strncmp(sc->s, "depz.dataset/", 13) != 0) {
            bad_line(1, "header \"schema\" is not a \"depz.dataset/...\" string");
            goto fail;
        }
        str_copy(sc, ds->schema, sizeof(ds->schema));
        str_copy(depz_json_get(ds->header, "created_utc"), ds->created_utc, sizeof(ds->created_utc));
        note = depz_json_get(ds->header, "note");
        ns = depz_json_string_value(note);
        ds->note = (char *)malloc(ns ? strlen(ns) + 1 : 1);
        if (!ds->note) goto nomem;
        strcpy(ds->note, ns ? ns : "");
    }

    {
        const depz_json *devs = depz_json_get(ds->header, "devices");
        if (devs && devs->kind == DEPZ_JSON_OBJECT) {
            ds->device_count = devs->count;
            ds->devices = (depz_dataset_device *)calloc(devs->count ? devs->count : 1, sizeof(depz_dataset_device));
            if (!ds->devices) goto nomem;
            for (i = 0; i < devs->count; i++) {
                depz_dataset_device *dd = &ds->devices[i];
                const depz_json *m = devs->items[i];
                const depz_json *sync;
                strncpy(dd->id, devs->keys[i], sizeof(dd->id) - 1);
                str_copy(depz_json_get(m, "serial"), dd->serial, sizeof(dd->serial));
                str_copy(depz_json_get(m, "sensor_type"), dd->sensor_type, sizeof(dd->sensor_type));
                str_copy(depz_json_get(m, "software_name"), dd->software_name, sizeof(dd->software_name));
                str_copy(depz_json_get(m, "model"), dd->model, sizeof(dd->model));
                sync = depz_json_get(m, "time_sync");
                dd->offset_us = depz_json_int_value(depz_json_get(sync, "offset_us"));
                dd->rtt_us = depz_json_int_value(depz_json_get(sync, "rtt_us"));
            }
        }
    }

    /* record lines: `d` and `k` strings, `t` an integer, `v` an object;
     * blank lines skipped; an unknown `k` kept (players ignore it) */
    while (next_line(text, len, &pos, &line, &llen)) {
        const depz_json *d, *t, *k, *v;
        lineno++;
        if (line_blank(line, llen)) continue;
        doc = depz_json_parse(line, llen);
        if (!doc) { bad_line(lineno, "record is not JSON"); goto fail; }
        if (doc->kind != DEPZ_JSON_OBJECT) { bad_line(lineno, "record is not a JSON object"); goto fail; }
        d = depz_json_get(doc, "d");
        t = depz_json_get(doc, "t");
        k = depz_json_get(doc, "k");
        v = depz_json_get(doc, "v");
        if (!d || d->kind != DEPZ_JSON_STRING) { bad_line(lineno, "\"d\" must be a device id string"); goto fail; }
        if (!t || t->kind != DEPZ_JSON_INT) { bad_line(lineno, "\"t\" must be an integer (host microseconds)"); goto fail; }
        if (!k || k->kind != DEPZ_JSON_STRING) { bad_line(lineno, "\"k\" must be a record kind string"); goto fail; }
        if (!v || v->kind != DEPZ_JSON_OBJECT) { bad_line(lineno, "\"v\" must be an object"); goto fail; }
        if (grow((void **)&ents, &rcap, nrec + 1, sizeof *ents)) goto nomem;
        ents[nrec].doc = doc;
        ents[nrec].t = t->i;
        ents[nrec].idx = nrec;
        nrec++;
        doc = NULL;
    }
    if (nrec > 1) qsort(ents, nrec, sizeof(rec_entry), rec_cmp);   /* ents is NULL when nrec is 0 */
    ds->record_count = nrec;
    ds->record_json = (depz_json **)calloc(nrec ? nrec : 1, sizeof(depz_json *));
    if (!ds->record_json) goto nomem;
    for (i = 0; i < nrec; i++) ds->record_json[i] = ents[i].doc;   /* now owned by ds */
    ds->records = (depz_dataset_record *)calloc(nrec ? nrec : 1, sizeof(depz_dataset_record));
    if (!ds->records) goto nomem;
    for (i = 0; i < nrec; i++) {
        depz_json *rd = ents[i].doc;
        depz_dataset_record *r = &ds->records[i];
        str_copy(depz_json_get(rd, "d"), r->device_id, sizeof(r->device_id));
        r->t_host_us = ents[i].t;
        str_copy(depz_json_get(rd, "k"), r->kind, sizeof(r->kind));
        r->value = depz_json_get(rd, "v");
    }
    free(ents);
    return ds;

nomem:
    depz_fail(DEPZ_E_NOMEM, "dataset: out of memory");
fail:
    depz_json_free(doc);
    if (!ds->record_json)   /* the line documents not yet owned by the dataset */
        for (i = 0; i < nrec; i++) depz_json_free(ents[i].doc);
    free(ents);
    depz_dataset_free(ds);
    return NULL;
}

depz_dataset *depz_dataset_parse(const char *text, size_t len)
{
    if (!text) { depz_fail(DEPZ_E_ARG, "dataset: NULL text"); return NULL; }
    if (is_gzip((const uint8_t *)text, len)) {
        size_t n;
        uint8_t *plain = gunzip((const uint8_t *)text, len, &n);
        depz_dataset *ds;
        if (!plain) { depz_fail(DEPZ_E_ARG, "dataset: corrupt gzip container"); return NULL; }
        ds = dataset_parse_text((const char *)plain, n);
        free(plain);
        return ds;
    }
    return dataset_parse_text(text, len);
}

depz_dataset *depz_dataset_read_file(const char *path)
{
    FILE *fp;
    size_t cap = 1 << 16, len = 0;
    char *buf;
    depz_dataset *ds;
    if (!path) { depz_fail(DEPZ_E_ARG, "dataset: NULL path"); return NULL; }
    fp = fopen(path, "rb");
    if (!fp) { depz_fail(DEPZ_E_IO, "dataset: cannot read %s", path); return NULL; }
    buf = (char *)malloc(cap);
    if (!buf) { fclose(fp); depz_fail(DEPZ_E_NOMEM, "dataset: out of memory"); return NULL; }
    for (;;) {
        size_t n = fread(buf + len, 1, cap - len, fp);
        char *nb;
        len += n;
        if (len < cap) break;
        nb = (char *)realloc(buf, cap * 2);
        if (!nb) { free(buf); fclose(fp); depz_fail(DEPZ_E_NOMEM, "dataset: out of memory"); return NULL; }
        buf = nb;
        cap *= 2;
    }
    fclose(fp);
    ds = depz_dataset_parse(buf, len);
    free(buf);
    return ds;
}

void depz_dataset_free(depz_dataset *ds)
{
    if (!ds) return;
    if (ds->record_json) {
        for (size_t i = 0; i < ds->record_count; i++)
            depz_json_free(ds->record_json[i]);
        free(ds->record_json);
    }
    free(ds->records);
    free(ds->devices);
    free(ds->note);
    depz_json_free(ds->header);
    free(ds);
}

const char *depz_dataset_schema(const depz_dataset *d) { return d ? d->schema : NULL; }
const char *depz_dataset_created_utc(const depz_dataset *d) { return d ? d->created_utc : ""; }
const char *depz_dataset_note(const depz_dataset *d) { return d && d->note ? d->note : ""; }
const depz_json *depz_dataset_header(const depz_dataset *d) { return d ? d->header : NULL; }
size_t depz_dataset_device_count(const depz_dataset *d) { return d ? d->device_count : 0; }

int depz_dataset_get_device(const depz_dataset *d, size_t i, depz_dataset_device *out)
{
    if (!d || i >= d->device_count) return -1;
    *out = d->devices[i];
    return 0;
}

size_t depz_dataset_record_count(const depz_dataset *d) { return d ? d->record_count : 0; }

int depz_dataset_get_record(const depz_dataset *d, size_t i, depz_dataset_record *out)
{
    if (!d || i >= d->record_count) return -1;
    *out = d->records[i];
    return 0;
}

int64_t depz_dataset_duration_us(const depz_dataset *d)
{
    if (!d || d->record_count < 2) return 0;
    return d->records[d->record_count - 1].t_host_us - d->records[0].t_host_us;
}

int depz_dataset_value_int(const depz_dataset_record *r, const char *key, int64_t *out)
{
    if (!r || !r->value) return -1;
    const depz_json *v = depz_json_get(r->value, key);
    if (!v || (v->kind != DEPZ_JSON_INT && v->kind != DEPZ_JSON_DOUBLE)) return -1;
    *out = depz_json_int_value(v);
    return 0;
}

int depz_dataset_value_str(const depz_dataset_record *r, const char *key, char *out, size_t cap)
{
    if (!r || !r->value) return -1;
    const depz_json *v = depz_json_get(r->value, key);
    if (!v || v->kind != DEPZ_JSON_STRING) return -1;
    str_copy(v, out, cap);
    return 0;
}

/* ======================================================================== */
/* record values                                                             */
/* ======================================================================== */

static depz_json *ints_i32(const int32_t *a, size_t n)
{
    depz_json *v = depz_json_array();
    for (size_t i = 0; v && i < n; i++) depz_json_push(v, depz_json_int(a[i]));
    return v;
}

static depz_json *ints_u8(const uint8_t *a, size_t n)
{
    depz_json *v = depz_json_array();
    for (size_t i = 0; v && i < n; i++) depz_json_push(v, depz_json_int(a[i]));
    return v;
}

static depz_json *ints_u16(const uint16_t *a, size_t n)
{
    depz_json *v = depz_json_array();
    for (size_t i = 0; v && i < n; i++) depz_json_push(v, depz_json_int(a[i]));
    return v;
}

static depz_json *doubles(const double *a, size_t n)
{
    depz_json *v = depz_json_array();
    for (size_t i = 0; v && i < n; i++) depz_json_push(v, depz_json_double(a[i]));
    return v;
}

static double q_scale(int64_t raw, int q) { return (double)raw / (double)(1ull << q); }

depz_json *depz_dataset_bno086_record(const depz_bno_report *r)
{
    depz_json *v = depz_json_object();
    if (!v || !r) { depz_json_free(v); return NULL; }
    depz_json_set(v, "type", depz_json_string(depz_bno_report_type_str(r->type)));
    depz_json_set(v, "sensor_id", depz_json_int(r->sensor_id));
    double a[4];
    switch (r->type) {
    case DEPZ_BNO_ACCELERATION: case DEPZ_BNO_GYROSCOPE: case DEPZ_BNO_MAGNETOMETER:
    case DEPZ_BNO_UNCAL_GYROSCOPE: case DEPZ_BNO_UNCAL_MAGNETOMETER:
        depz_bno_report_xyz(r, a);
        depz_json_set(v, "x", depz_json_double(a[0]));
        depz_json_set(v, "y", depz_json_double(a[1]));
        depz_json_set(v, "z", depz_json_double(a[2]));
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        if (r->type == DEPZ_BNO_UNCAL_GYROSCOPE || r->type == DEPZ_BNO_UNCAL_MAGNETOMETER) {
            depz_bno_report_bias(r, a);
            depz_json_set(v, "bias", doubles(a, 3));
        }
        break;
    case DEPZ_BNO_ROTATION_VECTOR:
        depz_bno_report_quaternion(r, a);
        depz_json_set(v, "i", depz_json_double(a[0]));
        depz_json_set(v, "j", depz_json_double(a[1]));
        depz_json_set(v, "k", depz_json_double(a[2]));
        depz_json_set(v, "real", depz_json_double(a[3]));
        if (r->has_accuracy_raw)
            depz_json_set(v, "accuracy_rad", depz_json_double(q_scale(r->accuracy_raw, DEPZ_BNO_RV_ACCURACY_Q)));
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        break;
    case DEPZ_BNO_GYRO_INTEGRATED_RV:
        depz_bno_report_quaternion(r, a);
        depz_json_set(v, "i", depz_json_double(a[0]));
        depz_json_set(v, "j", depz_json_double(a[1]));
        depz_json_set(v, "k", depz_json_double(a[2]));
        depz_json_set(v, "real", depz_json_double(a[3]));
        depz_bno_report_angular_velocity(r, a);
        depz_json_set(v, "angular_velocity", doubles(a, 3));
        break;
    case DEPZ_BNO_SCALAR_REPORT:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "value", depz_json_double(depz_bno_report_scalar(r)));
        break;
    case DEPZ_BNO_TAP_DETECTOR:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "flags", depz_json_int(r->flags));
        depz_json_set(v, "double_tap", depz_json_bool((r->flags & 0x40) != 0));
        break;
    case DEPZ_BNO_STEP_COUNTER:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "steps", depz_json_int(r->steps));
        depz_json_set(v, "latency_us", depz_json_int(r->latency_us));
        break;
    case DEPZ_BNO_STEP_DETECTOR:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "latency_us", depz_json_int(r->latency_us));
        break;
    case DEPZ_BNO_SIGNIFICANT_MOTION:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "motion", depz_json_int(r->motion));
        break;
    case DEPZ_BNO_STABILITY_CLASSIFIER:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "classification", depz_json_int(r->classification));
        break;
    case DEPZ_BNO_MOTION_REQUEST:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "intent", depz_json_int(r->intent));
        depz_json_set(v, "request", depz_json_int(r->request));
        break;
    case DEPZ_BNO_SHAKE_DETECTOR:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "flags", depz_json_int(r->flags));
        break;
    case DEPZ_BNO_ACTIVITY_CLASSIFIER:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "most_likely_state", depz_json_int(r->most_likely_state));
        depz_json_set(v, "confidences", ints_u8(r->confidences, 10));
        break;
    case DEPZ_BNO_RAW_SENSOR:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        /* the raw ADC streams (0x14..0x16) keep their integer counts */
        if (r->sensor_id >= 0x14 && r->sensor_id <= 0x16) {
            depz_json_set(v, "x_raw", depz_json_int(r->x_raw));
            depz_json_set(v, "y_raw", depz_json_int(r->y_raw));
            depz_json_set(v, "z_raw", depz_json_int(r->z_raw));
            depz_json_set(v, "temperature_raw", depz_json_int(r->temperature_raw));
            depz_json_set(v, "sensor_timestamp_us", depz_json_int(r->sensor_timestamp_us));
        }
        break;
    case DEPZ_BNO_GENERIC_EVENT:
        depz_json_set(v, "accuracy", depz_json_int(r->accuracy));
        depz_json_set(v, "value_raw", depz_json_int(r->value_raw));
        break;
    case DEPZ_BNO_FLUSH_COMPLETED_REPORT:
        depz_json_set(v, "flushed_sensor", depz_json_int(r->flushed_sensor));
        break;
    case DEPZ_BNO_UNKNOWN_REPORT:
    default:
        break;
    }
    depz_json_set(v, "timestamp_us", depz_json_int(r->timestamp_us));
    return v;
}

depz_json *depz_dataset_vl53l8_record(const depz_vl53l8_frame *f, bool layers)
{
    depz_json *v = depz_json_object();
    if (!v || !f) { depz_json_free(v); return NULL; }
    size_t n = f->resolution < 0 ? 0 : (size_t)f->resolution;
    if (n > DEPZ_VL53L8_MAX_ZONES) n = DEPZ_VL53L8_MAX_ZONES;
    depz_json_set(v, "resolution", depz_json_int(f->resolution));
    depz_json_set(v, "silicon_temp_degc", depz_json_int(f->silicon_temp_degc));
    depz_json_set(v, "distance_mm", ints_i32(f->distance_mm, n));
    depz_json_set(v, "target_status", ints_u8(f->target_status, n));
    depz_json_set(v, "nb_target_detected", ints_u8(f->nb_target_detected, n));
    if (layers) {
        depz_json_set(v, "signal_per_spad", doubles(f->signal_per_spad, n));
        depz_json_set(v, "ambient_per_spad", doubles(f->ambient_per_spad, n));
        depz_json_set(v, "range_sigma_mm", doubles(f->range_sigma_mm, n));
        depz_json_set(v, "reflectance", doubles(f->reflectance, n));
    }
    return v;
}

depz_json *depz_dataset_vl53l4_record(const depz_vl53l4_result *r)
{
    depz_json *v = depz_json_object();
    if (!v || !r) { depz_json_free(v); return NULL; }
    depz_json_set(v, "range_status", depz_json_int(r->range_status));
    depz_json_set(v, "distance_mm", depz_json_int(r->distance_mm));
    depz_json_set(v, "sigma_mm", depz_json_int(r->sigma_mm));
    depz_json_set(v, "signal_rate_kcps", depz_json_int(r->signal_rate_kcps));
    depz_json_set(v, "ambient_rate_kcps", depz_json_int(r->ambient_rate_kcps));
    depz_json_set(v, "number_of_spad", depz_json_int(r->number_of_spad));
    depz_json_set(v, "stream_count", depz_json_int(r->stream_count));
    return v;
}

/* Python's round(x, 1): the nearest tenth of the exact binary value, ties to
 * even — what printf's correctly rounded "%.1f" gives. */
static double round1(double x)
{
    char b[48];
    if (x == 0.0 || isnan(x) || isinf(x)) return x;
    snprintf(b, sizeof b, "%.1f", x);
    return strtod(b, NULL);
}

depz_json *depz_dataset_tmf8829_record(const depz_tmf8829_measurement *m)
{
    depz_json *v = depz_json_object();
    if (!v || !m) { depz_json_free(v); return NULL; }
    const depz_tmf8829_results *r = &m->results;
    const size_t zones = (size_t)m->rows * m->cols;
    const size_t per_peak = r->peaks ? zones * r->peaks : 0;
    depz_json_set(v, "fp_mode", depz_json_int(m->fp_mode));
    depz_json_set(v, "rows", depz_json_int(m->rows));
    depz_json_set(v, "cols", depz_json_int(m->cols));
    depz_json_set(v, "peaks", depz_json_int(r->peaks));
    depz_json_set(v, "f_number", depz_json_int(m->f_number));
    depz_json_set(v, "flags", depz_json_int(m->flags));
    depz_json_set(v, "temperature_c", depz_json_int(m->temperature_c));
    depz_json *dist = depz_json_array();
    for (size_t i = 0; dist && i < per_peak; i++) depz_json_push(dist, depz_json_double(round1(m->distance_mm[i])));
    depz_json_set(v, "distance_mm", dist);
    depz_json_set(v, "snr", ints_u8(r->snr, per_peak));
    if (r->has_signal) depz_json_set(v, "signal", ints_u16(r->signal, per_peak));
    if (r->has_noise) depz_json_set(v, "noise", ints_u16(r->noise, zones));
    if (r->has_xtalk) depz_json_set(v, "xtalk", ints_u16(r->xtalk, zones));
    return v;
}

depz_json *depz_dataset_temperature_record(double celsius, const char *source)
{
    depz_json *v = depz_json_object();
    if (!v) return NULL;
    depz_json_set(v, "celsius", depz_json_double(celsius));
    depz_json_set(v, "source", depz_json_string(source ? source : "mcu"));
    return v;
}

/* ======================================================================== */
/* writer                                                                    */
/* ======================================================================== */
struct depz_dataset_writer {
    FILE *fp;
    bool gz, failed;
    gz_writer gzw;
    uint64_t records;
};

static int w_put(depz_dataset_writer *w, const char *s, size_t n)
{
    if (w->gz) return gz_write(&w->gzw, (const uint8_t *)s, n);
    return fwrite(s, 1, n, w->fp) == n ? 0 : -1;
}

static int w_put_json_line(depz_dataset_writer *w, const depz_json *v)
{
    char *s = depz_json_dumps(v);
    if (!s) return -1;
    int rc = w_put(w, s, strlen(s));
    free(s);
    if (rc == 0) rc = w_put(w, "\n", 1);
    return rc;
}

static void utc_now(char out[32])
{
    time_t now = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    gmtime_s(&tmv, &now);
#else
    gmtime_r(&now, &tmv);
#endif
    strftime(out, 32, "%Y-%m-%dT%H:%M:%SZ", &tmv);
}

depz_dataset_writer *depz_dataset_writer_open(const char *path, const depz_json *devices, const char *note)
{
    if (!path || !devices || devices->kind != DEPZ_JSON_OBJECT) return NULL;
    FILE *fp = fopen(path, "wb");
    if (!fp) return NULL;
    depz_dataset_writer *w = (depz_dataset_writer *)calloc(1, sizeof *w);
    if (!w) { fclose(fp); return NULL; }
    w->fp = fp;
    size_t n = strlen(path);
    w->gz = n >= 3 && (strcmp(path + n - 3, ".gz") == 0 || strcmp(path + n - 3, ".GZ") == 0);
    if (w->gz && gz_begin(&w->gzw, fp) != 0) { fclose(fp); free(w); return NULL; }

    /* the header: schema, created_utc, devices (a copy via the serializer), note */
    char stamp[32];
    utc_now(stamp);
    depz_json *header = depz_json_object();
    depz_json_set(header, "schema", depz_json_string(DEPZ_DATASET_SCHEMA));
    depz_json_set(header, "created_utc", depz_json_string(stamp));
    char *devs_text = depz_json_dumps(devices);
    depz_json *devs_copy = devs_text ? depz_json_parse(devs_text, strlen(devs_text)) : NULL;
    free(devs_text);
    depz_json_set(header, "devices", devs_copy ? devs_copy : depz_json_object());
    if (note && *note) depz_json_set(header, "note", depz_json_string(note));
    int rc = w_put_json_line(w, header);
    depz_json_free(header);
    if (rc != 0) { fclose(fp); free(w); return NULL; }
    return w;
}

int depz_dataset_writer_write(depz_dataset_writer *w, const char *device_id, int64_t t_host_us,
                              const char *kind, depz_json *value)
{
    if (!w || !w->fp || w->failed || !device_id || !kind) { depz_json_free(value); return -1; }
    depz_json *rec = depz_json_object();
    if (!rec) { depz_json_free(value); return -1; }
    depz_json_set(rec, "d", depz_json_string(device_id));
    depz_json_set(rec, "t", depz_json_int(t_host_us));
    depz_json_set(rec, "k", depz_json_string(kind));
    depz_json_set(rec, "v", value ? value : depz_json_null());
    int rc = w_put_json_line(w, rec);
    depz_json_free(rec);
    if (rc != 0) { w->failed = true; return -1; }
    w->records++;
    return 0;
}

uint64_t depz_dataset_writer_records_written(const depz_dataset_writer *w) { return w ? w->records : 0; }

int depz_dataset_writer_close(depz_dataset_writer *w)
{
    if (!w) return 0;
    int rc = w->failed ? -1 : 0;
    if (w->fp) {
        if (w->gz && gz_finish(&w->gzw) != 0) rc = -1;
        if (fclose(w->fp) != 0) rc = -1;
    }
    free(w);
    return rc;
}

/* ======================================================================== */
/* player (contract 09 §Player)                                              */
/* ======================================================================== */

typedef struct { int token; void *user; depz_player_record_cb rec; depz_player_state_cb st; } player_cb;

struct depz_dataset_player {
    const depz_dataset *ds;
    double speed;
    depz_player_clock_fn clock;
    void *clock_user;
    depz_player_state state;
    size_t idx;                 /* the next record to deliver */
    double anchor_wall, anchor_t;
    player_cb *record_cbs, *state_cbs;
    size_t n_record_cbs, n_state_cbs;
    int next_token;
};

static double default_clock(void *user)
{
    (void)user;
#if defined(_WIN32)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

static bool speed_ok(double s) { return !isnan(s) && !isinf(s) && s >= 0; }

depz_dataset_player *depz_dataset_player_new(const depz_dataset *ds, double speed,
                                             depz_player_clock_fn clock, void *clock_user)
{
    if (!ds || !speed_ok(speed)) return NULL;
    depz_dataset_player *p = (depz_dataset_player *)calloc(1, sizeof *p);
    if (!p) return NULL;
    p->ds = ds;
    p->speed = speed;
    p->clock = clock ? clock : default_clock;
    p->clock_user = clock_user;
    p->state = DEPZ_PLAYER_IDLE;
    p->next_token = 1;
    return p;
}

void depz_dataset_player_free(depz_dataset_player *p)
{
    if (!p) return;
    free(p->record_cbs);
    free(p->state_cbs);
    free(p);
}

depz_player_state depz_dataset_player_state(const depz_dataset_player *p) { return p ? p->state : DEPZ_PLAYER_IDLE; }
int64_t depz_dataset_player_duration_us(const depz_dataset_player *p) { return p ? depz_dataset_duration_us(p->ds) : 0; }
double depz_dataset_player_speed(const depz_dataset_player *p) { return p ? p->speed : 0.0; }

static size_t p_count(const depz_dataset_player *p) { return depz_dataset_record_count(p->ds); }
static int64_t p_t(const depz_dataset_player *p, size_t i) { return p->ds->records[i].t_host_us; }

int64_t depz_dataset_player_position_us(const depz_dataset_player *p)
{
    if (!p) return 0;
    size_t n = p_count(p);
    if (n == 0 || p->idx == 0) return 0;
    size_t i = p->idx < n ? p->idx : n;
    return p_t(p, i - 1) - p_t(p, 0);
}

static double p_now(depz_dataset_player *p) { return p->clock(p->clock_user); }

/* The media clock restarts at `now` from the last delivered record's t (the
 * first record's when none was). */
static void p_anchor(depz_dataset_player *p, double now)
{
    p->anchor_wall = now;
    p->anchor_t = p_count(p) ? (double)p_t(p, p->idx > 0 ? p->idx - 1 : 0) : 0.0;
}

static double p_media_now(const depz_dataset_player *p, double now)
{
    if (p->speed <= 0) return (double)p_t(p, p->idx > 0 ? p->idx - 1 : 0);
    return p->anchor_t + (now - p->anchor_wall) * 1e6 * p->speed;
}

static void p_set_state(depz_dataset_player *p, depz_player_state s)
{
    p->state = s;
    /* a snapshot: a callback may unsubscribe */
    size_t n = p->n_state_cbs;
    player_cb *copy = n ? (player_cb *)malloc(n * sizeof *copy) : NULL;
    if (copy) memcpy(copy, p->state_cbs, n * sizeof *copy);
    for (size_t i = 0; copy && i < n; i++) copy[i].st(s, copy[i].user);
    free(copy);
}

int depz_dataset_player_set_speed(depz_dataset_player *p, double speed)
{
    if (!p || !speed_ok(speed)) return -1;
    if (p->state == DEPZ_PLAYER_PLAYING) {
        double now = p_now(p);
        p->anchor_t = p_media_now(p, now);
        p->anchor_wall = now;
    }
    p->speed = speed;
    return 0;
}

static int cb_add(player_cb **list, size_t *n, player_cb cb)
{
    player_cb *nl = (player_cb *)realloc(*list, (*n + 1) * sizeof **list);
    if (!nl) return -1;
    *list = nl;
    nl[(*n)++] = cb;
    return 0;
}

static void cb_remove(player_cb *list, size_t *n, int token)
{
    for (size_t i = 0; i < *n; i++) {
        if (list[i].token == token) {
            memmove(list + i, list + i + 1, (*n - i - 1) * sizeof *list);
            (*n)--;
            return;
        }
    }
}

int depz_dataset_player_on_record(depz_dataset_player *p, depz_player_record_cb cb, void *user, int *token)
{
    if (!p || !cb) return -1;
    player_cb e = { p->next_token++, user, cb, NULL };
    if (cb_add(&p->record_cbs, &p->n_record_cbs, e) != 0) return -1;
    if (token) *token = e.token;
    return 0;
}

void depz_dataset_player_off_record(depz_dataset_player *p, int token)
{
    if (p) cb_remove(p->record_cbs, &p->n_record_cbs, token);
}

int depz_dataset_player_on_state(depz_dataset_player *p, depz_player_state_cb cb, void *user, int *token)
{
    if (!p || !cb) return -1;
    player_cb e = { p->next_token++, user, NULL, cb };
    if (cb_add(&p->state_cbs, &p->n_state_cbs, e) != 0) return -1;
    if (token) *token = e.token;
    return 0;
}

void depz_dataset_player_off_state(depz_dataset_player *p, int token)
{
    if (p) cb_remove(p->state_cbs, &p->n_state_cbs, token);
}

void depz_dataset_player_play(depz_dataset_player *p)
{
    if (!p || p->state == DEPZ_PLAYER_PLAYING) return;
    if (p->idx >= p_count(p)) p->idx = 0;
    p_anchor(p, p_now(p));
    p_set_state(p, DEPZ_PLAYER_PLAYING);
    if (p_count(p) == 0) p_set_state(p, DEPZ_PLAYER_DONE);
}

void depz_dataset_player_pause(depz_dataset_player *p)
{
    if (p && p->state == DEPZ_PLAYER_PLAYING) p_set_state(p, DEPZ_PLAYER_PAUSED);
}

int depz_dataset_player_seek_us(depz_dataset_player *p, double offset_us)
{
    if (!p || isnan(offset_us)) return -1;
    size_t n = p_count(p);
    if (n == 0) return 0;
    double target = (double)p_t(p, 0) + offset_us;
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if ((double)p_t(p, mid) < target) lo = mid + 1;
        else hi = mid;
    }
    p->idx = lo;
    if (p->state == DEPZ_PLAYER_PLAYING) {
        if (p->idx >= n) p_set_state(p, DEPZ_PLAYER_DONE);
        else p_anchor(p, p_now(p));
    }
    return 0;
}

/* One step: 1 = a record went out (call again), 0 = wait *wait seconds,
 * -1 = not playing. */
static int p_step(depz_dataset_player *p, double *wait)
{
    if (p->state != DEPZ_PLAYER_PLAYING) return -1;
    size_t n = p_count(p);
    if (p->idx >= n) { p_set_state(p, DEPZ_PLAYER_DONE); return -1; }
    const depz_dataset_record *rec = &p->ds->records[p->idx];
    if (p->speed > 0) {
        double now = p_now(p);
        double due = p->anchor_wall + ((double)rec->t_host_us - p->anchor_t) / 1e6 / p->speed;
        if (due > now) { *wait = due - now; return 0; }
    }
    p->idx++;
    size_t nc = p->n_record_cbs;
    player_cb *copy = nc ? (player_cb *)malloc(nc * sizeof *copy) : NULL;
    if (copy) memcpy(copy, p->record_cbs, nc * sizeof *copy);
    for (size_t i = 0; copy && i < nc; i++) copy[i].rec(rec, copy[i].user);
    free(copy);
    if (p->state == DEPZ_PLAYER_PLAYING && p->idx >= n) p_set_state(p, DEPZ_PLAYER_DONE);
    return 1;
}

double depz_dataset_player_tick(depz_dataset_player *p)
{
    if (!p) return -1.0;
    double wait = 0.0;
    for (;;) {
        int r = p_step(p, &wait);
        if (r == 1) continue;
        return r == 0 ? wait : -1.0;
    }
}
