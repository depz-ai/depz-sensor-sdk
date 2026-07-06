#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *p;
    bool error;
} parser;

static json_value *parse_value(parser *ps);

static void skip_ws(parser *ps)
{
    while (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r')
        ps->p++;
}

static json_value *new_val(json_kind k)
{
    json_value *v = (json_value *)calloc(1, sizeof(json_value));
    if (v)
        v->kind = k;
    return v;
}

static void append_item(json_value *arr, json_value *item, char *key)
{
    json_value **ni = (json_value **)realloc(arr->items, (arr->count + 1) * sizeof(json_value *));
    arr->items = ni;
    arr->items[arr->count] = item;
    if (key || arr->kind == JSON_OBJECT) {
        char **nk = (char **)realloc(arr->keys, (arr->count + 1) * sizeof(char *));
        arr->keys = nk;
        arr->keys[arr->count] = key;
    }
    arr->count++;
}

/* Parse a JSON string literal (leading '"' already consumed check done here). */
static char *parse_string_raw(parser *ps)
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
            case '"':  c = '"';  break;
            case '\\': c = '\\'; break;
            case '/':  c = '/';  break;
            case 'n':  c = '\n'; break;
            case 't':  c = '\t'; break;
            case 'r':  c = '\r'; break;
            case 'b':  c = '\b'; break;
            case 'f':  c = '\f'; break;
            case 'u': {
                int cp = 0;
                for (int k = 0; k < 4; k++) {
                    char h = *ps->p++;
                    cp <<= 4;
                    if (h >= '0' && h <= '9') cp |= h - '0';
                    else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                    else { ps->error = true; }
                }
                /* Encode as UTF-8 (vectors are ASCII, but be safe). */
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
    if (len + 1 > cap) { out = (char *)realloc(out, len + 1); }
    out[len] = '\0';
    return out;
}

static json_value *parse_string(parser *ps)
{
    char *s = parse_string_raw(ps);
    if (ps->error) { free(s); return NULL; }
    json_value *v = new_val(JSON_STRING);
    v->s = s;
    return v;
}

static json_value *parse_number(parser *ps)
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
    if (is_double) {
        json_value *v = new_val(JSON_DOUBLE);
        v->d = strtod(buf, NULL);
        return v;
    }
    json_value *v = new_val(JSON_INT);
    v->i = strtoll(buf, NULL, 10);
    return v;
}

static json_value *parse_array(parser *ps)
{
    ps->p++; /* '[' */
    json_value *v = new_val(JSON_ARRAY);
    skip_ws(ps);
    if (*ps->p == ']') { ps->p++; return v; }
    for (;;) {
        skip_ws(ps);
        json_value *item = parse_value(ps);
        if (ps->error) { json_free(item); json_free(v); return NULL; }
        append_item(v, item, NULL);
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; break; }
        ps->error = true; json_free(v); return NULL;
    }
    return v;
}

static json_value *parse_object(parser *ps)
{
    ps->p++; /* '{' */
    json_value *v = new_val(JSON_OBJECT);
    skip_ws(ps);
    if (*ps->p == '}') { ps->p++; return v; }
    for (;;) {
        skip_ws(ps);
        char *key = parse_string_raw(ps);
        if (ps->error) { free(key); json_free(v); return NULL; }
        skip_ws(ps);
        if (*ps->p != ':') { ps->error = true; free(key); json_free(v); return NULL; }
        ps->p++;
        skip_ws(ps);
        json_value *val = parse_value(ps);
        if (ps->error) { free(key); json_free(val); json_free(v); return NULL; }
        append_item(v, val, key);
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; break; }
        ps->error = true; json_free(v); return NULL;
    }
    return v;
}

static json_value *parse_value(parser *ps)
{
    skip_ws(ps);
    char c = *ps->p;
    if (c == '"') return parse_string(ps);
    if (c == '{') return parse_object(ps);
    if (c == '[') return parse_array(ps);
    if (c == '-' || (c >= '0' && c <= '9')) return parse_number(ps);
    if (strncmp(ps->p, "true", 4) == 0)  { ps->p += 4; json_value *v = new_val(JSON_BOOL); v->b = true;  return v; }
    if (strncmp(ps->p, "false", 5) == 0) { ps->p += 5; json_value *v = new_val(JSON_BOOL); v->b = false; return v; }
    if (strncmp(ps->p, "null", 4) == 0)  { ps->p += 4; return new_val(JSON_NULL); }
    ps->error = true;
    return NULL;
}

json_value *json_parse(const char *text)
{
    parser ps = { text, false };
    json_value *v = parse_value(&ps);
    if (ps.error) { json_free(v); return NULL; }
    skip_ws(&ps);
    return v;
}

void json_free(json_value *v)
{
    if (!v) return;
    free(v->s);
    for (size_t i = 0; i < v->count; i++) {
        json_free(v->items[i]);
        if (v->keys) free(v->keys[i]);
    }
    free(v->items);
    free(v->keys);
    free(v);
}

const json_value *json_obj_get(const json_value *o, const char *key)
{
    if (!o || o->kind != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < o->count; i++)
        if (o->keys[i] && strcmp(o->keys[i], key) == 0)
            return o->items[i];
    return NULL;
}

const json_value *json_arr_get(const json_value *a, size_t idx)
{
    if (!a || a->kind != JSON_ARRAY || idx >= a->count) return NULL;
    return a->items[idx];
}

size_t json_arr_size(const json_value *a)
{
    return (a && a->kind == JSON_ARRAY) ? a->count : 0;
}

const char *json_as_str(const json_value *v)
{
    return (v && v->kind == JSON_STRING) ? v->s : NULL;
}

int64_t json_as_int(const json_value *v)
{
    if (!v) return 0;
    if (v->kind == JSON_INT) return v->i;
    if (v->kind == JSON_DOUBLE) return (int64_t)v->d;
    return 0;
}

bool json_as_bool(const json_value *v)
{
    return (v && v->kind == JSON_BOOL) ? v->b : false;
}

bool json_is_null(const json_value *v)
{
    return (!v) || v->kind == JSON_NULL;
}
