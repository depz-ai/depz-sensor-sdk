/* Tiny recursive-descent JSON parser — test-runner support only, not part of
 * the SDK. Sufficient for the vector JSON files: objects, arrays, strings
 * (with \uXXXX/\n/\t/... escapes), integers (parsed as int64), booleans, null.
 * Integer values in the vectors all fit in int64.
 */
#ifndef DEPZ_TEST_JSON_H
#define DEPZ_TEST_JSON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_INT,
    JSON_DOUBLE,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_kind;

typedef struct json_value json_value;

struct json_value {
    json_kind kind;
    /* scalars */
    bool     b;
    int64_t  i;
    double   d;
    char    *s;      /* NUL-terminated (JSON_STRING) */
    /* array / object */
    json_value **items;   /* array elements or object values */
    char       **keys;    /* object keys (JSON_OBJECT only) */
    size_t       count;
};

/* Parse a NUL-terminated JSON document. Returns NULL on error. */
json_value *json_parse(const char *text);
void        json_free(json_value *v);

/* Accessors (return NULL / defaults on type mismatch). */
const json_value *json_obj_get(const json_value *o, const char *key);
const json_value *json_arr_get(const json_value *a, size_t idx);
size_t            json_arr_size(const json_value *a);
const char       *json_as_str(const json_value *v);   /* NULL if not string */
int64_t           json_as_int(const json_value *v);   /* 0 if not int/double */
bool              json_as_bool(const json_value *v);
bool              json_is_null(const json_value *v);

#endif
