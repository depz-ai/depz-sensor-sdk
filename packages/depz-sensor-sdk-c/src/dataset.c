/*
 * `.depzdata` dataset reader (contract 09). JSONL: line 0 is the header
 * object; each subsequent line is a record {"d","t","k","v"}. Records are
 * merged into a stable order by t_host_us (ties keep file order), mirroring
 * the TS/Python DatasetReader.
 *
 * A compact self-contained JSON value parser lives here so the SDK stays
 * dependency-free (the SDK proper otherwise carries no JSON).
 */
#include "depz_sensor_sdk.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== */
/* minimal JSON value tree                                                   */
/* ======================================================================== */
typedef enum { J_NULL, J_BOOL, J_INT, J_DOUBLE, J_STR, J_ARR, J_OBJ } jkind;

typedef struct jval {
    jkind kind;
    bool b;
    int64_t i;
    double d;
    char *s;
    struct jval **items;
    char **keys;
    size_t count;
} jval;

typedef struct { const char *p; bool error; } jparser;

static jval *j_parse_value(jparser *ps);

static void j_skip_ws(jparser *ps)
{
    while (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r')
        ps->p++;
}

static jval *j_new(jkind k)
{
    jval *v = (jval *)calloc(1, sizeof(jval));
    if (v) v->kind = k;
    return v;
}

static void j_free(jval *v)
{
    if (!v) return;
    free(v->s);
    for (size_t i = 0; i < v->count; i++) {
        j_free(v->items[i]);
        if (v->keys) free(v->keys[i]);
    }
    free(v->items);
    free(v->keys);
    free(v);
}

static void j_append(jval *arr, jval *item, char *key)
{
    jval **ni = (jval **)realloc(arr->items, (arr->count + 1) * sizeof(jval *));
    if (!ni) { j_free(item); free(key); return; }
    arr->items = ni;
    arr->items[arr->count] = item;
    if (key || arr->kind == J_OBJ) {
        char **nk = (char **)realloc(arr->keys, (arr->count + 1) * sizeof(char *));
        if (!nk) { free(key); return; }
        arr->keys = nk;
        arr->keys[arr->count] = key;
    }
    arr->count++;
}

static char *j_parse_string_raw(jparser *ps)
{
    if (*ps->p != '"') { ps->error = true; return NULL; }
    ps->p++;
    size_t cap = 16, len = 0;
    char *out = (char *)malloc(cap);
    while (*ps->p && *ps->p != '"') {
        char c = *ps->p++;
        if (c == '\\') {
            char e = *ps->p++;
            switch (e) {
            case '"': c = '"'; break;   case '\\': c = '\\'; break;
            case '/': c = '/'; break;   case 'n': c = '\n'; break;
            case 't': c = '\t'; break;  case 'r': c = '\r'; break;
            case 'b': c = '\b'; break;  case 'f': c = '\f'; break;
            case 'u': {
                int cp = 0;
                for (int k = 0; k < 4; k++) {
                    char h = *ps->p++;
                    cp <<= 4;
                    if (h >= '0' && h <= '9') cp |= h - '0';
                    else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                    else ps->error = true;
                }
                if (len + 4 >= cap) { cap = cap * 2 + 4; out = (char *)realloc(out, cap); }
                if (cp < 0x80) {
                    out[len++] = (char)cp;
                } else if (cp < 0x800) {
                    out[len++] = (char)(0xC0 | (cp >> 6));
                    out[len++] = (char)(0x80 | (cp & 0x3F));
                } else {
                    out[len++] = (char)(0xE0 | (cp >> 12));
                    out[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    out[len++] = (char)(0x80 | (cp & 0x3F));
                }
                continue;
            }
            default: ps->error = true; break;
            }
        }
        if (len + 1 >= cap) { cap *= 2; out = (char *)realloc(out, cap); }
        out[len++] = c;
    }
    if (*ps->p != '"') { ps->error = true; free(out); return NULL; }
    ps->p++;
    if (len + 1 > cap) out = (char *)realloc(out, len + 1);
    out[len] = '\0';
    return out;
}

static jval *j_parse_number(jparser *ps)
{
    const char *start = ps->p;
    bool is_double = false;
    if (*ps->p == '-') ps->p++;
    while (isdigit((unsigned char)*ps->p)) ps->p++;
    if (*ps->p == '.') { is_double = true; ps->p++; while (isdigit((unsigned char)*ps->p)) ps->p++; }
    if (*ps->p == 'e' || *ps->p == 'E') {
        is_double = true; ps->p++;
        if (*ps->p == '+' || *ps->p == '-') ps->p++;
        while (isdigit((unsigned char)*ps->p)) ps->p++;
    }
    char buf[64];
    size_t n = (size_t)(ps->p - start);
    if (n >= sizeof(buf)) { ps->error = true; return NULL; }
    memcpy(buf, start, n);
    buf[n] = '\0';
    if (is_double) { jval *v = j_new(J_DOUBLE); v->d = strtod(buf, NULL); return v; }
    jval *v = j_new(J_INT); v->i = strtoll(buf, NULL, 10); return v;
}

static jval *j_parse_array(jparser *ps)
{
    ps->p++;
    jval *v = j_new(J_ARR);
    j_skip_ws(ps);
    if (*ps->p == ']') { ps->p++; return v; }
    for (;;) {
        j_skip_ws(ps);
        jval *item = j_parse_value(ps);
        if (ps->error) { j_free(item); j_free(v); return NULL; }
        j_append(v, item, NULL);
        j_skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; break; }
        ps->error = true; j_free(v); return NULL;
    }
    return v;
}

static jval *j_parse_object(jparser *ps)
{
    ps->p++;
    jval *v = j_new(J_OBJ);
    j_skip_ws(ps);
    if (*ps->p == '}') { ps->p++; return v; }
    for (;;) {
        j_skip_ws(ps);
        char *key = j_parse_string_raw(ps);
        if (ps->error) { free(key); j_free(v); return NULL; }
        j_skip_ws(ps);
        if (*ps->p != ':') { ps->error = true; free(key); j_free(v); return NULL; }
        ps->p++;
        j_skip_ws(ps);
        jval *val = j_parse_value(ps);
        if (ps->error) { free(key); j_free(val); j_free(v); return NULL; }
        j_append(v, val, key);
        j_skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; break; }
        ps->error = true; j_free(v); return NULL;
    }
    return v;
}

static jval *j_parse_value(jparser *ps)
{
    j_skip_ws(ps);
    char c = *ps->p;
    if (c == '"') { jval *v = j_new(J_STR); v->s = j_parse_string_raw(ps);
                    if (ps->error) { j_free(v); return NULL; } return v; }
    if (c == '{') return j_parse_object(ps);
    if (c == '[') return j_parse_array(ps);
    if (c == '-' || (c >= '0' && c <= '9')) return j_parse_number(ps);
    if (strncmp(ps->p, "true", 4) == 0)  { ps->p += 4; jval *v = j_new(J_BOOL); v->b = true; return v; }
    if (strncmp(ps->p, "false", 5) == 0) { ps->p += 5; jval *v = j_new(J_BOOL); v->b = false; return v; }
    if (strncmp(ps->p, "null", 4) == 0)  { ps->p += 4; return j_new(J_NULL); }
    ps->error = true;
    return NULL;
}

/* Parse one NUL-terminated JSON document. */
static jval *j_parse(const char *text)
{
    jparser ps = { text, false };
    jval *v = j_parse_value(&ps);
    if (ps.error) { j_free(v); return NULL; }
    j_skip_ws(&ps);
    return v;
}

static const jval *j_get(const jval *o, const char *key)
{
    if (!o || o->kind != J_OBJ) return NULL;
    for (size_t i = 0; i < o->count; i++)
        if (o->keys[i] && strcmp(o->keys[i], key) == 0)
            return o->items[i];
    return NULL;
}

static int64_t j_int(const jval *v)
{
    if (!v) return 0;
    if (v->kind == J_INT) return v->i;
    if (v->kind == J_DOUBLE) return (int64_t)v->d;
    return 0;
}

static void j_str_copy(const jval *v, char *out, size_t cap)
{
    out[0] = '\0';
    if (v && v->kind == J_STR && cap) {
        strncpy(out, v->s, cap - 1);
        out[cap - 1] = '\0';
    }
}

/* ======================================================================== */
/* dataset                                                                   */
/* ======================================================================== */
struct depz_dataset {
    char schema[32];
    jval *header;                 /* owned */
    depz_dataset_device *devices;
    size_t device_count;
    jval **record_json;           /* owned line docs, in merged order */
    depz_dataset_record *records;
    size_t record_count;
    int64_t *t_keys;              /* parallel, for stability */
    size_t *orig_idx;
};

typedef struct { jval *doc; int64_t t; size_t idx; } rec_entry;

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

depz_dataset *depz_dataset_parse(const char *text, size_t len)
{
    /* split into NUL-terminated lines */
    char *copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, text, len);
    copy[len] = '\0';

    /* collect non-empty lines */
    char **lines = NULL;
    size_t nlines = 0, lcap = 0;
    char *p = copy;
    while (*p) {
        char *nl = strchr(p, '\n');
        char *end = nl ? nl : p + strlen(p);
        char *line = p;
        *end = '\0';
        /* trim: consider empty if only whitespace */
        char *t = line;
        while (*t == ' ' || *t == '\t' || *t == '\r') t++;
        if (*t) {
            if (nlines == lcap) {
                lcap = lcap ? lcap * 2 : 16;
                lines = (char **)realloc(lines, lcap * sizeof(char *));
            }
            lines[nlines++] = line;
        }
        if (!nl) break;
        p = nl + 1;
    }

    if (nlines == 0) { free(lines); free(copy); return NULL; }

    depz_dataset *ds = (depz_dataset *)calloc(1, sizeof(*ds));
    if (!ds) { free(lines); free(copy); return NULL; }

    /* header */
    ds->header = j_parse(lines[0]);
    if (!ds->header) goto fail;
    {
        const jval *sc = j_get(ds->header, "schema");
        if (!sc || sc->kind != J_STR || strncmp(sc->s, "depz.dataset/", 13) != 0)
            goto fail;
        j_str_copy(sc, ds->schema, sizeof(ds->schema));
    }

    /* devices */
    {
        const jval *devs = j_get(ds->header, "devices");
        if (devs && devs->kind == J_OBJ) {
            ds->device_count = devs->count;
            ds->devices = (depz_dataset_device *)calloc(devs->count ? devs->count : 1,
                                                        sizeof(depz_dataset_device));
            for (size_t i = 0; i < devs->count; i++) {
                depz_dataset_device *dd = &ds->devices[i];
                strncpy(dd->id, devs->keys[i], sizeof(dd->id) - 1);
                const jval *m = devs->items[i];
                j_str_copy(j_get(m, "serial"), dd->serial, sizeof(dd->serial));
                j_str_copy(j_get(m, "sensor_type"), dd->sensor_type, sizeof(dd->sensor_type));
                j_str_copy(j_get(m, "software_name"), dd->software_name, sizeof(dd->software_name));
                const jval *sync = j_get(m, "time_sync");
                dd->offset_us = j_int(j_get(sync, "offset_us"));
                dd->rtt_us = j_int(j_get(sync, "rtt_us"));
            }
        }
    }

    /* records */
    {
        size_t nrec = nlines - 1;
        rec_entry *ents = (rec_entry *)calloc(nrec ? nrec : 1, sizeof(rec_entry));
        size_t rc = 0;
        for (size_t i = 1; i < nlines; i++) {
            jval *doc = j_parse(lines[i]);
            if (!doc) { /* skip unparseable line */ continue; }
            ents[rc].doc = doc;
            ents[rc].t = j_int(j_get(doc, "t"));
            ents[rc].idx = rc;
            rc++;
        }
        qsort(ents, rc, sizeof(rec_entry), rec_cmp);

        ds->record_count = rc;
        ds->record_json = (jval **)calloc(rc ? rc : 1, sizeof(jval *));
        ds->records = (depz_dataset_record *)calloc(rc ? rc : 1, sizeof(depz_dataset_record));
        for (size_t i = 0; i < rc; i++) {
            jval *doc = ents[i].doc;
            ds->record_json[i] = doc;
            depz_dataset_record *r = &ds->records[i];
            j_str_copy(j_get(doc, "d"), r->device_id, sizeof(r->device_id));
            r->t_host_us = j_int(j_get(doc, "t"));
            j_str_copy(j_get(doc, "k"), r->kind, sizeof(r->kind));
            r->value = j_get(doc, "v");
        }
        free(ents);
    }

    free(lines);
    free(copy);
    return ds;

fail:
    free(lines);
    free(copy);
    depz_dataset_free(ds);
    return NULL;
}

void depz_dataset_free(depz_dataset *ds)
{
    if (!ds) return;
    if (ds->record_json) {
        for (size_t i = 0; i < ds->record_count; i++)
            j_free(ds->record_json[i]);
        free(ds->record_json);
    }
    free(ds->records);
    free(ds->devices);
    free(ds->t_keys);
    free(ds->orig_idx);
    j_free(ds->header);
    free(ds);
}

const char *depz_dataset_schema(const depz_dataset *d)
{
    return d ? d->schema : NULL;
}

size_t depz_dataset_device_count(const depz_dataset *d)
{
    return d ? d->device_count : 0;
}

int depz_dataset_get_device(const depz_dataset *d, size_t i, depz_dataset_device *out)
{
    if (!d || i >= d->device_count) return -1;
    *out = d->devices[i];
    return 0;
}

size_t depz_dataset_record_count(const depz_dataset *d)
{
    return d ? d->record_count : 0;
}

int depz_dataset_get_record(const depz_dataset *d, size_t i, depz_dataset_record *out)
{
    if (!d || i >= d->record_count) return -1;
    *out = d->records[i];
    return 0;
}

int depz_dataset_value_int(const depz_dataset_record *r, const char *key, int64_t *out)
{
    if (!r || !r->value) return -1;
    const jval *v = j_get((const jval *)r->value, key);
    if (!v || (v->kind != J_INT && v->kind != J_DOUBLE)) return -1;
    *out = j_int(v);
    return 0;
}

int depz_dataset_value_str(const depz_dataset_record *r, const char *key, char *out, size_t cap)
{
    if (!r || !r->value) return -1;
    const jval *v = j_get((const jval *)r->value, key);
    if (!v || v->kind != J_STR) return -1;
    j_str_copy(v, out, cap);
    return 0;
}
