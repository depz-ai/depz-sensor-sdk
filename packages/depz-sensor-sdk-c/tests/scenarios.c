/* scenarios.c — contract 15 device scenarios: the C runner.
 *
 * Each contracts/scenarios/<id>.json names a committed real-hardware capture
 * and the API calls that produced it. The runner opens the capture as its
 * `probe` field says — "canonical": depz_open_on_link() (the contract 02
 * §4.4 probe, then the class in place); "legacy": the requests the capture
 * holds, depz_resolve_sensor_class() and depz_attach_sensor_class() — over a
 * causal, strict_tx replay link, so every byte the SDK writes must equal the
 * recorded tx stream, executes the steps (§2), writes each result as canonical JSON
 * (§3: records keyed by the Python field names) and matches it against
 * `expect`. C has no reflection, so the binding is one dispatch table:
 * canonical name -> wrapper that calls the C function and writes its result.
 * A name missing from the table fails the scenario; it never skips.
 */
#include "scenarios.h"

#include "depz_sensor_io.h"
#include "json.h"
#include "vector_seen.h"

#include "../src/io/platform.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DEPZ_VECTORS_DIR
#define DEPZ_VECTORS_DIR "."
#endif
#ifndef DEPZ_SCENARIOS_DIR
#define DEPZ_SCENARIOS_DIR DEPZ_VECTORS_DIR "/../scenarios"
#endif

const char *const scenario_ids[] = {
#include "scenario_ids.h"
    NULL};

#define TIMEOUT_MS      2000 /* request timeout, as the replay tests always used */
#define TAKE_TIMEOUT_MS 5000 /* per streamed item; replay serves them instantly */
#define RUNNER_BUG      (-1000)

/* ── canonical JSON writer ───────────────────────────────────────────────
 * Every value is written with a trailing comma; jw_finish() drops the ones
 * before a closing bracket and at the end. */

typedef struct { char *b; size_t n, cap; } jw;

static void jw_put(jw *w, const char *fmt, ...)
{
    va_list ap;
    int need;
    for (;;) {
        size_t room = w->cap - w->n;
        va_start(ap, fmt);
        need = vsnprintf(w->b ? w->b + w->n : NULL, w->b ? room : 0, fmt, ap);
        va_end(ap);
        if (need < 0) return;
        if (w->b && (size_t)need < room) { w->n += (size_t)need; return; }
        w->cap = (w->cap + (size_t)need + 1) * 2;
        w->b = (char *)realloc(w->b, w->cap);
        if (!w->b) abort();
    }
}

static void jw_str(jw *w, const char *s)
{
    jw_put(w, "\"");
    for (; s && *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '"' || ch == '\\') jw_put(w, "\\%c", ch);
        else if (ch < 0x20) jw_put(w, "\\u%04x", ch);
        else jw_put(w, "%c", ch);
    }
    jw_put(w, "\"");
}

static void jw_finish(jw *w)
{
    size_t i, o = 0;
    bool in_str = false;
    if (!w->b) jw_put(w, "");
    for (i = 0; i < w->n; i++) {
        char ch = w->b[i];
        if (in_str) {
            if (ch == '\\') { w->b[o++] = ch; ch = w->b[++i]; }
            else if (ch == '"') in_str = false;
        } else if (ch == '"') {
            in_str = true;
        } else if (ch == ',') {
            size_t j = i + 1;
            if (j == w->n || w->b[j] == '}' || w->b[j] == ']') continue;
        }
        w->b[o++] = ch;
    }
    w->n = o;
    w->b[o] = '\0';
}

#define K(k)        jw_put(w, "\"%s\":", k)
#define V_I(v)      jw_put(w, "%lld,", (long long)(v))
#define V_U(v)      jw_put(w, "%llu,", (unsigned long long)(v))
#define V_B(v)      jw_put(w, "%s,", (v) ? "true" : "false")
#define V_NULL()    jw_put(w, "null,")
#define V_S(v)      (jw_str(w, v), jw_put(w, ","))
#define OPEN()      jw_put(w, "{")
#define CLOSE()     jw_put(w, "},")
#define AOPEN()     jw_put(w, "[")
#define ACLOSE()    jw_put(w, "],")
#define KI(k, v)    (K(k), V_I(v))
#define KU(k, v)    (K(k), V_U(v))
#define KB(k, v)    (K(k), V_B(v))
#define KS(k, v)    (K(k), V_S(v))
#define KN(k)       (K(k), V_NULL())
/* A field per struct member of the same name. */
#define FI(p, f)    KI(#f, (p)->f)
#define FU(p, f)    KU(#f, (p)->f)
#define FB(p, f)    KB(#f, (p)->f)
#define ARR_I(a, n) do { size_t i_; AOPEN(); for (i_ = 0; i_ < (size_t)(n); i_++) V_I((a)[i_]); ACLOSE(); } while (0)

static void V_D(jw *w, double d)
{
    char t[40];
    if (!isfinite(d)) { V_NULL(); return; }
    snprintf(t, sizeof t, "%.17g", d);
    jw_put(w, strpbrk(t, ".eE") ? "%s," : "%s.0,", t);
}

static void V_HEX(jw *w, const uint8_t *p, size_t n)
{
    size_t i;
    jw_put(w, "\"");
    for (i = 0; i < n; i++) jw_put(w, "%02x", p[i]);
    jw_put(w, "\",");
}

static void ARR_D(jw *w, const double *a, size_t n)
{
    size_t i;
    AOPEN();
    for (i = 0; i < n; i++) V_D(w, a[i]);
    ACLOSE();
}

/* A json_value back to text (messages, step descriptions). */
static void dump(jw *w, const json_value *v)
{
    size_t i;
    if (!v) { jw_put(w, "?"); return; }
    switch (v->kind) {
    case JSON_NULL: jw_put(w, "null"); break;
    case JSON_BOOL: jw_put(w, v->b ? "true" : "false"); break;
    case JSON_INT: jw_put(w, "%lld", (long long)v->i); break;
    case JSON_DOUBLE: jw_put(w, "%.17g", v->d); break;
    case JSON_STRING: jw_str(w, v->s); break;
    case JSON_ARRAY:
        jw_put(w, "[");
        for (i = 0; i < v->count; i++) { if (i) jw_put(w, ", "); dump(w, v->items[i]); }
        jw_put(w, "]");
        break;
    case JSON_OBJECT:
        jw_put(w, "{");
        for (i = 0; i < v->count; i++) {
            if (i) jw_put(w, ", ");
            jw_str(w, v->keys[i]);
            jw_put(w, ": ");
            dump(w, v->items[i]);
        }
        jw_put(w, "}");
        break;
    }
}

/* Short text of a value for a message (truncated). */
static const char *repr(const json_value *v, char *buf, size_t cap)
{
    jw w = {0};
    dump(&w, v);
    snprintf(buf, cap, "%s%s", w.b ? w.b : "", w.n + 1 > cap ? "..." : "");
    if (w.n + 1 > cap && cap > 4) strcpy(buf + cap - 4, "...");
    free(w.b);
    return buf;
}

/* ── §3 matching ─────────────────────────────────────────────────────── */

static const char *kind_name(const json_value *v)
{
    static const char *const n[] = {"null", "a boolean", "an integer", "a float", "a string", "an array", "an object"};
    return v ? n[v->kind] : "nothing";
}

/* false when `got` matches `want`; else true with where and how in `msg`. */
static bool mismatch(const json_value *got, const json_value *want, const char *path, char *msg, size_t cap)
{
    const char *at = path[0] ? path : "result";
    char sub[512], a[200], b[200];
    size_t i;
    if (want->kind == JSON_OBJECT) {
        if (got->kind != JSON_OBJECT) {
            snprintf(msg, cap, "%s: expected an object, got %s", at, repr(got, a, sizeof a));
            return true;
        }
        for (i = 0; i < want->count; i++) {
            const json_value *g = json_obj_get(got, want->keys[i]);
            if (!g) {
                jw f = {0};
                size_t k;
                for (k = 0; k < got->count; k++) jw_put(&f, "%s%s", k ? ", " : "", got->keys[k]);
                snprintf(msg, cap, "%s.%s: missing (fields: %s)", at, want->keys[i], f.b ? f.b : "");
                free(f.b);
                return true;
            }
            snprintf(sub, sizeof sub, "%s.%s", path, want->keys[i]);
            if (mismatch(g, want->items[i], sub, msg, cap)) return true;
        }
        return false;
    }
    if (want->kind == JSON_ARRAY) {
        if (got->kind != JSON_ARRAY) {
            snprintf(msg, cap, "%s: expected an array, got %s", at, repr(got, a, sizeof a));
            return true;
        }
        if (got->count != want->count) {
            snprintf(msg, cap, "%s: expected %zu elements, got %zu", at, want->count, got->count);
            return true;
        }
        for (i = 0; i < want->count; i++) {
            snprintf(sub, sizeof sub, "%s[%zu]", path, i);
            if (mismatch(got->items[i], want->items[i], sub, msg, cap)) return true;
        }
        return false;
    }
    if (want->kind == JSON_DOUBLE) {
        if (got->kind == JSON_DOUBLE || got->kind == JSON_INT) {
            double g = got->kind == JSON_INT ? (double)got->i : got->d;
            double tol = fabs(want->d) * 1e-9;
            if (fabs(g - want->d) <= (tol > 1e-12 ? tol : 1e-12)) return false;
        }
    } else if (got->kind == want->kind) {
        /* integers, strings, booleans, null: exact, a bool never equals an int */
        if (want->kind == JSON_NULL) return false;
        if (want->kind == JSON_BOOL && got->b == want->b) return false;
        if (want->kind == JSON_INT && got->i == want->i) return false;
        if (want->kind == JSON_STRING && !strcmp(got->s, want->s)) return false;
    }
    snprintf(msg, cap, "%s: expected %s, got %s%s%s", at, repr(want, a, sizeof a), repr(got, b, sizeof b),
             got->kind != want->kind ? " which is " : "", got->kind != want->kind ? kind_name(got) : "");
    return true;
}

/* ── the runner's state ──────────────────────────────────────────────── */

typedef struct {
    const char  *name;
    depz_stream *s;
    size_t       item_size;
    void       (*write)(jw *w, const void *item);
} sub_t;

typedef struct {
    depz_device *dev;
    depz_link   *link;
    const char  *cls, *group;
    sub_t        subs[4];
    size_t       n_subs;
    char         err[1024];
} sc_ctx;

static int bug(sc_ctx *c, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(c->err, sizeof c->err, fmt, ap);
    va_end(ap);
    return RUNNER_BUG;
}

/* Positional arguments, checked: a shape the wrapper does not expect is a
 * runner bug, never a silent default. */
static bool a_int(sc_ctx *c, const json_value *args, size_t i, int64_t *v)
{
    const json_value *a = json_arr_get(args, i);
    char t[100];
    if (!a || a->kind != JSON_INT) { bug(c, "argument %zu: expected an integer, got %s", i, a ? repr(a, t, sizeof t) : "none"); return false; }
    *v = a->i;
    return true;
}

static bool a_str(sc_ctx *c, const json_value *args, size_t i, const char **s, bool nullable)
{
    const json_value *a = json_arr_get(args, i);
    char t[100];
    if (a && nullable && a->kind == JSON_NULL) { *s = NULL; return true; }
    if (!a || a->kind != JSON_STRING) { bug(c, "argument %zu: expected a string, got %s", i, a ? repr(a, t, sizeof t) : "none"); return false; }
    *s = a->s;
    return true;
}

static bool a_kind(sc_ctx *c, const json_value *args, size_t i, json_kind k, const char *what, const json_value **out)
{
    const json_value *a = json_arr_get(args, i);
    char t[100];
    if (!a || a->kind != k) { bug(c, "argument %zu: expected %s, got %s", i, what, a ? repr(a, t, sizeof t) : "none"); return false; }
    *out = a;
    return true;
}

#define ARG_I(i, v)  int64_t v; if (!a_int(c, args, i, &v)) return RUNNER_BUG
#define DEV          (c->dev)
#define TRY(e)       do { int rc_ = (e); if (rc_) return rc_; } while (0)

/* ── result writers (§3: Python field names) ─────────────────────────── */

static void w_sr04(jw *w, const void *p)
{
    const depz_sr04_measurement *m = (const depz_sr04_measurement *)p;
    OPEN(); FU(m, timestamp_us); FU(m, echo_time_us); KS("source", m->from_loop ? "loop" : "once"); CLOSE();
}

static void w_l4cd(jw *w, const void *p)
{
    const depz_vl53l4cd_measurement *m = (const depz_vl53l4cd_measurement *)p;
    const depz_vl53l4_result *r = &m->r;
    OPEN(); FU(m, timestamp_us);
    FI(r, range_status); FI(r, distance_mm); FI(r, sigma_mm); FI(r, signal_rate_kcps); FI(r, ambient_rate_kcps);
    FI(r, signal_per_spad_kcps); FI(r, ambient_per_spad_kcps); FI(r, number_of_spad); FI(r, stream_count);
    CLOSE();
}

static void w_l8(jw *w, const void *p)
{
    const depz_vl53l8_live_frame *lf = (const depz_vl53l8_live_frame *)p;
    const depz_vl53l8_frame *f = &lf->f;
    int n = f->resolution;
    OPEN(); FU(f, timestamp_us); FI(f, resolution); FI(f, silicon_temp_degc);
    K("distance_mm"); ARR_I(f->distance_mm, n);
    K("target_status"); ARR_I(f->target_status, n);
    K("nb_target_detected"); ARR_I(f->nb_target_detected, n);
    K("nb_spads_enabled"); ARR_I(f->nb_spads_enabled, n);
    K("reflectance"); ARR_D(w, f->reflectance, (size_t)n);
    K("cnh_raw"); if (lf->cnh_len) V_HEX(w, lf->cnh, lf->cnh_len); else V_NULL();
    CLOSE();
}

static void w_vlx_bins(jw *w, const depz_vl53lx_bins *b)
{
    OPEN();
    KI("result__interrupt_status", b->interrupt_status); KI("result__range_status", b->range_status);
    KI("result__report_status", b->report_status); KI("result__stream_count", b->stream_count);
    KI("result__dss_actual_effective_spads", b->dss_actual_effective_spads);
    KI("phasecal_result__reference_phase", b->reference_phase); KI("phasecal_result__vcsel_start", b->vcsel_start);
    K("bin_data"); ARR_I(b->bin_data, b->number_of_bins);
    FI(b, zone_id); FI(b, first_bin); FI(b, number_of_bins); FI(b, bins_in_data);
    KI("cal_config__vcsel_start", b->cal_config_vcsel_start); FI(b, vcsel_width); FI(b, fast_osc_frequency);
    FI(b, vcsel_period); K("bin_seq"); ARR_I(b->bin_seq, 6); K("bin_rep"); ARR_I(b->bin_rep, 6);
    FI(b, min_bin_value); FI(b, max_bin_value); FI(b, number_of_ambient_bins); FI(b, number_of_ambient_samples);
    FI(b, ambient_events_sum); FI(b, ambient_per_bin); FI(b, total_periods_elapsed); FI(b, peak_duration_us);
    FI(b, woi_duration_us); FI(b, zero_distance_phase);
    KI("roi_config__user_roi_centre_spad", b->roi_centre_spad);
    KI("roi_config__user_roi_requested_global_xy_size", b->roi_xy_size);
    CLOSE();
}

static void w_vlx(jw *w, const void *p)
{
    const depz_vl53lx_measurement *m = (const depz_vl53lx_measurement *)p;
    size_t k;
    OPEN(); FU(m, timestamp_us); FI(m, distance_mm); FI(m, status); KS("status_text", m->status_text);
    K("targets"); AOPEN();
    for (k = 0; k < m->n_targets; k++) {
        const depz_vl53lx_target *t = &m->targets[k];
        OPEN(); FI(t, distance_mm); FI(t, status); KS("status_text", t->status_text);
        FI(t, min_range_mm); FI(t, max_range_mm); CLOSE();
    }
    ACLOSE();
    K("bins"); if (m->has_bins) w_vlx_bins(w, &m->bins); else V_NULL();
    CLOSE();
}

static void w_units(jw *w, const depz_bno055_units *u)
{
    OPEN(); FB(u, accel_mg); FB(u, gyro_rps); FB(u, euler_rad); FB(u, temp_f); FB(u, android); CLOSE();
}

static void w_bno055(jw *w, const void *p)
{
    const depz_bno055_sample *s = (const depz_bno055_sample *)p;
    static const struct { const char *name; uint32_t bit; size_t off, n; } vec[] = {
#define VEC(f, b, n) {#f, DEPZ_BNO055_HAS_##b, offsetof(depz_bno055_sample, f), n}
        VEC(accel, ACCEL, 3), VEC(mag, MAG, 3), VEC(gyro, GYRO, 3), VEC(euler, EULER, 3),
        VEC(quaternion, QUATERNION, 4), VEC(linear_accel, LINEAR_ACCEL, 3), VEC(gravity, GRAVITY, 3),
#undef VEC
    };
    size_t i;
    OPEN(); FU(s, timestamp_us); FI(s, addr); K("raw"); V_HEX(w, s->raw, s->len);
    K("units"); w_units(w, &s->units);
    for (i = 0; i < sizeof vec / sizeof vec[0]; i++) {
        K(vec[i].name);
        if (s->present & vec[i].bit) ARR_D(w, (const double *)((const char *)s + vec[i].off), vec[i].n);
        else V_NULL();
    }
    K("temperature"); if (s->present & DEPZ_BNO055_HAS_TEMPERATURE) V_D(w, s->temperature); else V_NULL();
    K("calibration");
    if (s->present & DEPZ_BNO055_HAS_CALIBRATION) {
        const depz_bno055_calib_status *k = &s->calibration;
        OPEN(); FI(k, system); FI(k, gyro); FI(k, accel); FI(k, mag); CLOSE();
    } else {
        V_NULL();
    }
    CLOSE();
}

/* BNO086 reports mix record types: each type writes the fields of its
 * Python dataclass (the scenario pins sensor_id, which fixes the type). */
static void w_bno_report(jw *w, const void *p)
{
    const depz_bno_report *r = (const depz_bno_report *)p;
    OPEN(); FI(r, sensor_id); FI(r, timestamp_us);
    if (r->type != DEPZ_BNO_GYRO_INTEGRATED_RV && r->type != DEPZ_BNO_UNKNOWN_REPORT &&
        r->type != DEPZ_BNO_FLUSH_COMPLETED_REPORT) {
        FI(r, seq); FI(r, accuracy); FI(r, delay_us);
    }
    switch (r->type) {
    case DEPZ_BNO_UNCAL_GYROSCOPE:
    case DEPZ_BNO_UNCAL_MAGNETOMETER:
        FI(r, bias_x_raw); FI(r, bias_y_raw); FI(r, bias_z_raw);
        /* fall through */
    case DEPZ_BNO_ACCELERATION:
    case DEPZ_BNO_GYROSCOPE:
    case DEPZ_BNO_MAGNETOMETER:
        FI(r, x_raw); FI(r, y_raw); FI(r, z_raw);
        break;
    case DEPZ_BNO_ROTATION_VECTOR:
        FI(r, i_raw); FI(r, j_raw); FI(r, k_raw); FI(r, real_raw);
        if (r->has_accuracy_raw) FI(r, accuracy_raw); else KN("accuracy_raw");
        break;
    case DEPZ_BNO_GYRO_INTEGRATED_RV:
        FI(r, i_raw); FI(r, j_raw); FI(r, k_raw); FI(r, real_raw); FI(r, vx_raw); FI(r, vy_raw); FI(r, vz_raw);
        break;
    case DEPZ_BNO_SCALAR_REPORT:
    case DEPZ_BNO_GENERIC_EVENT:
        FI(r, value_raw);
        break;
    default:
        break; /* other types: header only; a scenario pinning more fails as "missing" */
    }
    CLOSE();
}

static void w_feature(jw *w, const depz_bno_feature *g)
{
    OPEN(); FI(g, sensor_id); FI(g, flags); FI(g, sensitivity); FU(g, interval_us); FU(g, batch_us); FU(g, cfg_word); CLOSE();
}

/* ── call wrappers ───────────────────────────────────────────────────── */

typedef int (*call_fn)(sc_ctx *c, const json_value *args, jw *w);

/* common (contract 02) */
static int c_software_name(sc_ctx *c, const json_value *args, jw *w)
{ char s[128]; (void)args; TRY(depz_device_get_software_name(DEV, s, sizeof s)); V_S(s); return 0; }
static int c_device_name(sc_ctx *c, const json_value *args, jw *w)
{ char s[128]; (void)args; TRY(depz_device_get_device_name(DEV, s, sizeof s)); V_S(s); return 0; }

/* SR04 */
static int c_sr04_set_period(sc_ctx *c, const json_value *args, jw *w)
{ ARG_I(0, v); (void)w; return depz_sr04_set_sample_period_us(DEV, (uint32_t)v); }
static int c_sr04_get_period(sc_ctx *c, const json_value *args, jw *w)
{ uint32_t v; (void)args; TRY(depz_sr04_get_sample_period_us(DEV, &v)); V_U(v); return 0; }
static int c_sr04_get_decay(sc_ctx *c, const json_value *args, jw *w)
{ uint16_t v; (void)args; TRY(depz_sr04_get_echo_decay_us(DEV, &v)); V_U(v); return 0; }
static int c_sr04_once(sc_ctx *c, const json_value *args, jw *w)
{ depz_sr04_measurement m; (void)args; TRY(depz_sr04_measure_once(DEV, TIMEOUT_MS, &m)); w_sr04(w, &m); return 0; }
static int c_sr04_start(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_sr04_start(DEV); }
static int c_sr04_stop(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_sr04_stop(DEV); }

/* VL53L4CD */
static int c_l4_info(sc_ctx *c, const json_value *args, jw *w)
{
    depz_vl53l4_info i;
    (void)args;
    TRY(depz_vl53l4cd_bridge_info(DEV, &i));
    OPEN(); FU(&i, int_edges); FU(&i, slots_skipped); FU(&i, i2c_errors); FU(&i, last_i2c_error); FU(&i, model_id);
    FU(&i, fw_status); FU(&i, initialized); FU(&i, xshut_level); FU(&i, int_level); FU(&i, i2c_khz); CLOSE();
    return 0;
}
static int c_l4_init(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l4cd_init(DEV, 0); }
static int c_l4_set_timing(sc_ctx *c, const json_value *args, jw *w)
{ ARG_I(0, b); ARG_I(1, i); (void)w; return depz_vl53l4cd_set_range_timing(DEV, (uint32_t)b, (uint32_t)i); }
static int c_l4_get_timing(sc_ctx *c, const json_value *args, jw *w)
{ uint32_t b, i; (void)args; TRY(depz_vl53l4cd_get_range_timing(DEV, &b, &i)); AOPEN(); V_U(b); V_U(i); ACLOSE(); return 0; }
static int c_l4_offset(sc_ctx *c, const json_value *args, jw *w)
{ int32_t v; (void)args; TRY(depz_vl53l4cd_get_offset_mm(DEV, &v)); V_I(v); return 0; }
static int c_l4_xtalk(sc_ctx *c, const json_value *args, jw *w)
{ uint16_t v; (void)args; TRY(depz_vl53l4cd_get_xtalk_kcps(DEV, &v)); V_U(v); return 0; }
static int c_l4_once(sc_ctx *c, const json_value *args, jw *w)
{ depz_vl53l4cd_measurement m; (void)args; TRY(depz_vl53l4cd_measure_once(DEV, TIMEOUT_MS, &m)); w_l4cd(w, &m); return 0; }
static int c_l4_start(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l4cd_start_ranging(DEV); }
static int c_l4_stop(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l4cd_stop_ranging(DEV); }
static int c_l4_parse_errors(sc_ctx *c, const json_value *args, jw *w)
{ (void)args; V_U(depz_vl53l4cd_stream_parse_errors(DEV)); return 0; }

/* VL53L8CX / L8CH / L5CX / L7CX / L7CH */
static int c_l8_init(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l8_init(DEV, NULL, NULL); }
static int c_l8_module_type(sc_ctx *c, const json_value *args, jw *w) { (void)args; V_I(depz_vl53l8_module_type(DEV)); return 0; }
static int c_l8_resolution(sc_ctx *c, const json_value *args, jw *w)
{ ARG_I(0, v); (void)w; return depz_vl53l8_set_resolution(DEV, (int)v); }
static int c_l8_freq(sc_ctx *c, const json_value *args, jw *w)
{ ARG_I(0, v); (void)w; return depz_vl53l8_set_ranging_frequency_hz(DEV, (uint8_t)v); }
static int c_l8_start(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l8_start_ranging(DEV); }
static int c_l8_stop(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53l8_stop_ranging(DEV); }
static int c_l8_parse_errors(sc_ctx *c, const json_value *args, jw *w)
{ (void)args; V_U(depz_vl53l8_frame_parse_errors(DEV)); return 0; }

/* Record argument: the CNH setup = cnh_init_config + cnh_create_agg_map. */
static int c_l8_cnh(sc_ctx *c, const json_value *args, jw *w)
{
    static const char *const keys[] = {"start_bin", "num_bins", "sub_sample", "resolution"};
    const json_value *rec, *map;
    int64_t v[4], m[6];
    depz_vl53l8_cnh_setup s;
    size_t i;
    (void)w;
    if (!a_kind(c, args, 0, JSON_OBJECT, "a CNH record", &rec)) return RUNNER_BUG;
    for (i = 0; i < 4; i++) {
        if (!json_obj_get(rec, keys[i]) || json_obj_get(rec, keys[i])->kind != JSON_INT)
            return bug(c, "CNH record: '%s' missing or not an integer", keys[i]);
        v[i] = json_obj_get(rec, keys[i])->i;
    }
    map = json_obj_get(rec, "agg_map");
    if (!map || map->kind != JSON_ARRAY || map->count != 6) return bug(c, "CNH record: agg_map must be 6 integers");
    for (i = 0; i < 6; i++) {
        if (map->items[i]->kind != JSON_INT) return bug(c, "CNH record: agg_map[%zu] not an integer", i);
        m[i] = map->items[i]->i;
    }
    if (rec->count != 5) return bug(c, "CNH record: unknown keys (has %zu, builder knows 5)", rec->count);
    depz_vl53l8_cnh_init_config(&s, (int)v[0], (int)v[1], (int)v[2]);
    TRY(depz_vl53l8_cnh_create_agg_map(&s, (int)v[3], (int)m[0], (int)m[1], (int)m[2], (int)m[3], (int)m[4], (int)m[5]));
    return depz_vl53l8_configure_cnh(DEV, &s);
}

/* VL53L 1D family */
static int c_vlx_init(sc_ctx *c, const json_value *args, jw *w)
{
    const char *drv = NULL, *prod = NULL;
    depz_vl53lx_driver kind = (depz_vl53lx_driver)0;
    depz_vl53lx_product p = DEPZ_VL53LX_PRODUCT_NONE;
    (void)w;
    if (json_arr_size(args) > 0 && !a_str(c, args, 0, &drv, true)) return RUNNER_BUG;
    if (json_arr_size(args) > 1 && !a_str(c, args, 1, &prod, true)) return RUNNER_BUG;
    if (drv) {
        if (!strcmp(drv, "uld")) kind = DEPZ_VL53LX_DRIVER_ULD;
        else if (!strcmp(drv, "ulp")) kind = DEPZ_VL53LX_DRIVER_ULP;
        else if (!strcmp(drv, "histogram")) kind = DEPZ_VL53LX_DRIVER_HISTOGRAM;
        else return DEPZ_E_ARG; /* what the SDK's enum cannot name: refused before sending */
    }
    if (prod && (p = depz_vl53lx_product_from_str(prod)) == DEPZ_VL53LX_PRODUCT_NONE) return DEPZ_E_ARG;
    return depz_vl53lx_init(DEV, kind, p);
}
static int c_vlx_product(sc_ctx *c, const json_value *args, jw *w)
{
    const depz_vl53lx_product_info *p = depz_vl53lx_product_get(depz_vl53lx_product_bound(DEV));
    (void)args;
    if (p) V_S(p->name); else V_NULL();
    return 0;
}
static int c_vlx_configure(sc_ctx *c, const json_value *args, jw *w)
{
    const char *mode;
    ARG_I(0, b); ARG_I(1, i);
    (void)w;
    if (!a_str(c, args, 2, &mode, true)) return RUNNER_BUG;
    return depz_vl53lx_configure(DEV, (int)b, (int)i, mode, NULL, NULL);
}
static int c_vlx_get_timing(sc_ctx *c, const json_value *args, jw *w)
{ int b, i; (void)args; TRY(depz_vl53lx_get_range_timing(DEV, &b, &i)); AOPEN(); V_I(b); V_I(i); ACLOSE(); return 0; }
static int c_vlx_modes(sc_ctx *c, const json_value *args, jw *w)
{
    const char *names[DEPZ_VL53LX_MAX_MODES];
    size_t n = depz_vl53lx_modes(DEV, names, DEPZ_VL53LX_MAX_MODES), i;
    (void)args;
    AOPEN(); for (i = 0; i < n; i++) V_S(names[i]); ACLOSE();
    return 0;
}
static int c_vlx_budget(sc_ctx *c, const json_value *args, jw *w)
{ int lo, hi; (void)args; TRY(depz_vl53lx_budget_range(DEV, &lo, &hi)); AOPEN(); V_I(lo); V_I(hi); ACLOSE(); return 0; }
static int c_vlx_reach(sc_ctx *c, const json_value *args, jw *w) { (void)args; V_U(depz_vl53lx_driver_reach_mm(DEV)); return 0; }
static int c_vlx_start(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53lx_start_ranging(DEV); }
static int c_vlx_stop(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_vl53lx_stop_ranging(DEV); }
static int c_vlx_parse_errors(sc_ctx *c, const json_value *args, jw *w)
{ (void)args; V_U(depz_vl53lx_stream_parse_errors(DEV)); return 0; }

/* BNO055 */
static int c_b55_info(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno055_info i;
    (void)args;
    TRY(depz_bno055_bridge_info(DEV, &i));
    OPEN(); FU(&i, i2c_addr); FU(&i, chip_id); FU(&i, acc_id); FU(&i, mag_id); FU(&i, gyr_id); FU(&i, sw_rev);
    FU(&i, bl_rev); FU(&i, initialized); FU(&i, int_level); FU(&i, int_edges); FU(&i, read_min_us);
    FU(&i, read_max_us); FU(&i, read_avg_us); FU(&i, tx_dropped); FU(&i, i2c_errors); FU(&i, slots_skipped);
    FU(&i, bus_recoveries); FU(&i, last_i2c_error); FU(&i, sensor_resets); FU(&i, loop_max_us); CLOSE();
    return 0;
}
static int c_b55_reset(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_bno055_reset_sensor(DEV); }
/* Record argument: Units (every field named, nothing defaulted). */
static int c_b55_configure(sc_ctx *c, const json_value *args, jw *w)
{
    static const char *const keys[] = {"accel_mg", "gyro_rps", "euler_rad", "temp_f", "android"};
    depz_bno055_units u;
    bool *f[5];
    const json_value *rec = NULL;
    size_t i;
    ARG_I(0, mode);
    (void)w;
    f[0] = &u.accel_mg; f[1] = &u.gyro_rps; f[2] = &u.euler_rad; f[3] = &u.temp_f; f[4] = &u.android;
    if (json_arr_size(args) > 1) {
        if (!a_kind(c, args, 1, JSON_OBJECT, "a Units record", &rec)) return RUNNER_BUG;
        if (rec->count != 5) return bug(c, "Units record: %zu keys, the builder knows 5", rec->count);
        for (i = 0; i < 5; i++) {
            const json_value *v = json_obj_get(rec, keys[i]);
            if (!v || v->kind != JSON_BOOL) return bug(c, "Units record: '%s' missing or not a boolean", keys[i]);
            *f[i] = v->b;
        }
    }
    return depz_bno055_configure(DEV, (uint8_t)mode, rec ? &u : NULL, NULL, NULL);
}
static int c_b55_profile(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno055_calib_profile p;
    (void)args;
    TRY(depz_bno055_read_calibration_profile(DEV, &p));
    OPEN(); K("accel_offset"); ARR_I(p.accel_offset, 3); K("mag_offset"); ARR_I(p.mag_offset, 3);
    K("gyro_offset"); ARR_I(p.gyro_offset, 3); FI(&p, accel_radius); FI(&p, mag_radius); CLOSE();
    return 0;
}
static int c_b55_status(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno055_status_regs s;
    (void)args;
    TRY(depz_bno055_system_status(DEV, &s));
    OPEN(); FI(&s, self_test); FI(&s, clk_status); FI(&s, status); FI(&s, error); CLOSE();
    return 0;
}
/* start_stream(period_ms, (addr, len)[, trigger]) */
static int c_b55_start(sc_ctx *c, const json_value *args, jw *w)
{
    const json_value *blk;
    int64_t trig = DEPZ_BNO055_TRIGGER_TIMER;
    ARG_I(0, period);
    (void)w;
    if (!a_kind(c, args, 1, JSON_ARRAY, "an (addr, len) pair", &blk)) return RUNNER_BUG;
    if (blk->count != 2 || blk->items[0]->kind != JSON_INT || blk->items[1]->kind != JSON_INT)
        return bug(c, "argument 1: expected [addr, len]");
    if (json_arr_size(args) > 2 && !a_int(c, args, 2, &trig)) return RUNNER_BUG;
    return depz_bno055_start_stream(DEV, (uint16_t)period, (uint8_t)blk->items[0]->i, (uint8_t)blk->items[1]->i,
                                    (uint8_t)trig);
}
static int c_b55_stop(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_bno055_stop_stream(DEV); }
static int c_b55_parse_errors(sc_ctx *c, const json_value *args, jw *w)
{ (void)args; V_U(depz_bno055_stream_parse_errors(DEV)); return 0; }

/* BNO085 / BNO086 */
static int c_b86_reset(sc_ctx *c, const json_value *args, jw *w) { (void)args; (void)w; return depz_bno086_hardware_reset(DEV, -1); }
static int c_b86_pid(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno_product_id p;
    (void)args;
    TRY(depz_bno086_product_id(DEV, &p));
    OPEN(); FU(&p, reset_cause); FU(&p, sw_version_major); FU(&p, sw_version_minor); FU(&p, sw_part_number);
    FU(&p, sw_build_number); FU(&p, sw_version_patch); CLOSE();
    return 0;
}
static int c_b86_enable(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno_feature g;
    ARG_I(0, sensor); ARG_I(1, hz);
    TRY(depz_bno086_enable(DEV, (uint8_t)sensor, (double)hz, &g));
    w_feature(w, &g);
    return 0;
}
static int c_b86_disable(sc_ctx *c, const json_value *args, jw *w)
{ ARG_I(0, sensor); (void)w; return depz_bno086_disable(DEV, (uint8_t)sensor); }
static int c_b86_cal(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno086_calibration k;
    (void)args;
    TRY(depz_bno086_get_calibration(DEV, &k));
    OPEN(); FB(&k, accel); FB(&k, gyro); FB(&k, mag); FB(&k, planar); CLOSE();
    return 0;
}
static int c_b86_osc(sc_ctx *c, const json_value *args, jw *w)
{ uint8_t v; (void)args; TRY(depz_bno086_get_oscillator_type(DEV, &v)); V_U(v); return 0; }
/* Alias: depz_bno086_get_metadata() keeps only the decoded fields, Python's
 * SensorMetadata also the raw words. Same request (the metadata FRS record
 * read), decoded the same way: depz_bno_metadata_from_words(). */
static int c_b86_metadata(sc_ctx *c, const json_value *args, jw *w)
{
    uint32_t words[64];
    size_t n = 0;
    depz_bno_metadata m;
    uint16_t record;
    ARG_I(0, sensor);
    record = depz_bno_metadata_record((uint8_t)sensor);
    if (!record) return DEPZ_E_ARG;
    TRY(depz_bno086_frs_read(DEV, record, words, 64, &n));
    depz_bno_metadata_from_words(words, n, &m);
    OPEN(); FU(&m, me_version); FU(&m, mh_version); FU(&m, sh_version); FU(&m, range_raw); FU(&m, resolution_raw);
    FU(&m, revision); FU(&m, power_ma_q10); FU(&m, min_period_us); FU(&m, max_period_us); FU(&m, fifo_max);
    FU(&m, fifo_reserved); FU(&m, batch_buffer_bytes); FU(&m, q_point_1); FU(&m, q_point_2); FU(&m, q_point_3);
    K("raw_words"); ARR_I(words, n); CLOSE();
    return 0;
}
static int c_b86_counts(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno086_counts k;
    ARG_I(0, sensor);
    TRY(depz_bno086_get_counts(DEV, (uint8_t)sensor, &k));
    OPEN(); FU(&k, sensor_id); FU(&k, offered); FU(&k, accepted); FU(&k, on); FU(&k, attempted); CLOSE();
    return 0;
}
static int c_b86_errors(sc_ctx *c, const json_value *args, jw *w)
{
    depz_bno086_error_record e[64];
    size_t n = 0, i;
    int64_t sev = 0;
    if (json_arr_size(args) > 0 && !a_int(c, args, 0, &sev)) return RUNNER_BUG;
    TRY(depz_bno086_get_errors(DEV, (uint8_t)sev, e, 64, &n));
    AOPEN();
    for (i = 0; i < n; i++) {
        OPEN(); FU(&e[i], severity); FU(&e[i], seq); FU(&e[i], source); FU(&e[i], error); FU(&e[i], module);
        FU(&e[i], code); CLOSE();
    }
    ACLOSE();
    return 0;
}

/* ── the alias table (§2): canonical name -> binding, per class group ─── */

typedef struct { const char *group, *name; size_t nmin, nmax; call_fn fn; } call_entry;

static const call_entry k_calls[] = {
    {"*", "get_software_name", 0, 0, c_software_name},
    {"*", "get_device_name", 0, 0, c_device_name},

    {"sr04", "set_sample_period_us", 1, 1, c_sr04_set_period},
    {"sr04", "get_sample_period_us", 0, 0, c_sr04_get_period},
    {"sr04", "get_echo_decay_us", 0, 0, c_sr04_get_decay},
    {"sr04", "measure_once", 0, 0, c_sr04_once},
    {"sr04", "start", 0, 0, c_sr04_start},
    {"sr04", "stop", 0, 0, c_sr04_stop},

    {"vl53l4cd", "bridge_info", 0, 0, c_l4_info},
    {"vl53l4cd", "init", 0, 0, c_l4_init},
    {"vl53l4cd", "set_range_timing", 2, 2, c_l4_set_timing},
    {"vl53l4cd", "get_range_timing", 0, 0, c_l4_get_timing},
    {"vl53l4cd", "get_offset_mm", 0, 0, c_l4_offset},
    {"vl53l4cd", "get_xtalk_kcps", 0, 0, c_l4_xtalk},
    {"vl53l4cd", "measure_once", 0, 0, c_l4_once},
    {"vl53l4cd", "start_ranging", 0, 0, c_l4_start},
    {"vl53l4cd", "stop_ranging", 0, 0, c_l4_stop},
    {"vl53l4cd", "stream_parse_errors", 0, 0, c_l4_parse_errors},

    {"vl53l8", "init", 0, 0, c_l8_init},
    {"vl53l8", "module_type", 0, 0, c_l8_module_type},
    {"vl53l8", "set_resolution", 1, 1, c_l8_resolution},
    {"vl53l8", "set_ranging_frequency_hz", 1, 1, c_l8_freq},
    {"vl53l8", "configure_cnh", 1, 1, c_l8_cnh},
    {"vl53l8", "start_ranging", 0, 0, c_l8_start},
    {"vl53l8", "stop_ranging", 0, 0, c_l8_stop},
    {"vl53l8", "frame_parse_errors", 0, 0, c_l8_parse_errors},

    {"vl53lx", "init", 0, 2, c_vlx_init},
    {"vl53lx", "product", 0, 0, c_vlx_product},
    {"vl53lx", "configure", 3, 3, c_vlx_configure},
    {"vl53lx", "get_range_timing", 0, 0, c_vlx_get_timing},
    {"vl53lx", "modes", 0, 0, c_vlx_modes},
    {"vl53lx", "budget_range", 0, 0, c_vlx_budget},
    {"vl53lx", "driver_reach_mm", 0, 0, c_vlx_reach},
    {"vl53lx", "start_ranging", 0, 0, c_vlx_start},
    {"vl53lx", "stop_ranging", 0, 0, c_vlx_stop},
    {"vl53lx", "stream_parse_errors", 0, 0, c_vlx_parse_errors},

    {"bno055", "bridge_info", 0, 0, c_b55_info},
    {"bno055", "reset_sensor", 0, 0, c_b55_reset},
    {"bno055", "configure", 1, 2, c_b55_configure},
    {"bno055", "read_calibration_profile", 0, 0, c_b55_profile},
    {"bno055", "system_status", 0, 0, c_b55_status},
    {"bno055", "start_stream", 2, 3, c_b55_start},
    {"bno055", "stop_stream", 0, 0, c_b55_stop},
    {"bno055", "stream_parse_errors", 0, 0, c_b55_parse_errors},

    {"bno086", "hardware_reset", 0, 0, c_b86_reset},
    {"bno086", "product_id", 0, 0, c_b86_pid},
    {"bno086", "enable", 2, 2, c_b86_enable},
    {"bno086", "disable", 1, 1, c_b86_disable},
    {"bno086", "get_calibration", 0, 0, c_b86_cal},
    {"bno086", "get_oscillator_type", 0, 0, c_b86_osc},
    {"bno086", "get_metadata", 1, 1, c_b86_metadata},
    {"bno086", "get_counts", 1, 1, c_b86_counts},
    {"bno086", "get_errors", 0, 1, c_b86_errors},
};

static depz_stream *s_sr04(depz_device *d, size_t n) { return depz_sr04_stream(d, n); }
static depz_stream *s_l4(depz_device *d, size_t n) { return depz_vl53l4cd_stream(d, n); }
static depz_stream *s_l8(depz_device *d, size_t n) { return depz_vl53l8_frames(d, n); }
static depz_stream *s_vlx(depz_device *d, size_t n) { return depz_vl53lx_measurements(d, n); }
static depz_stream *s_b55(depz_device *d, size_t n) { return depz_bno055_samples(d, n); }
static depz_stream *s_b86(depz_device *d, size_t n) { return depz_bno086_reports(d, n); }

typedef struct {
    const char *group, *name;
    depz_stream *(*open)(depz_device *, size_t);
    size_t item_size;
    void (*write)(jw *, const void *);
} stream_entry;

static const stream_entry k_streams[] = {
    {"sr04", "measurements", s_sr04, sizeof(depz_sr04_measurement), w_sr04},
    {"vl53l4cd", "measurements", s_l4, sizeof(depz_vl53l4cd_measurement), w_l4cd},
    {"vl53l8", "frames", s_l8, sizeof(depz_vl53l8_live_frame), w_l8},
    {"vl53lx", "measurements", s_vlx, sizeof(depz_vl53lx_measurement), w_vlx},
    {"bno055", "samples", s_b55, sizeof(depz_bno055_sample), w_bno055},
    {"bno086", "reports", s_b86, sizeof(depz_bno_report), w_bno_report},
};

/* The canonical class (contract 07 §1 layer 3) the device opened as, and the
 * group of the alias table that serves it. */
static void classify(sc_ctx *c)
{
    static const char *const l8[] = {"vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch"};
    static const char *const lx[] = {"vl53lx", "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx"};
    c->cls = c->group = "?";
    if (depz_is_sr04(c->dev)) c->cls = c->group = "sr04";
    else if (depz_is_vl53l4cd(c->dev)) c->cls = c->group = "vl53l4cd";
    else if (depz_is_bno055(c->dev)) c->cls = c->group = "bno055";
    else if (depz_is_bno086(c->dev)) c->cls = c->group = "bno086";
    else if (depz_is_tmf8829(c->dev)) c->cls = c->group = "tmf8829";
    else if (depz_is_vl53l8(c->dev)) { c->cls = l8[depz_vl53l8_get_model(c->dev)]; c->group = "vl53l8"; }
    else if (depz_is_vl53lx(c->dev)) { c->cls = lx[depz_vl53lx_get_class(c->dev)]; c->group = "vl53lx"; }
}

/* §4: the canonical kind of an SDK failure, NULL for anything else. */
static const char *error_kind(int rc, const char *msg)
{
    if (rc == DEPZ_E_TIMEOUT) return "timeout";
    if (rc == DEPZ_E_STATUS) return depz_last_status() == DEPZ_STATUS_ERR_CMD_NOT_SUPPORTED ? "not_supported" : "device_error";
    if (rc == DEPZ_E_WRONG_TYPE) return "not_supported";
    if (rc == DEPZ_E_PROTOCOL) return "invalid_reply";
    /* DEPZ_E_ARG is both "bad argument" and "this product/driver lacks the
     * group"; only the message tells them apart (as Python's exceptions). */
    if (rc == DEPZ_E_ARG) return strstr(msg, " does not support ") ? "not_supported" : "invalid_argument";
    return NULL;
}

/* ── steps ───────────────────────────────────────────────────────────── */

static void describe(const json_value *step, char *buf, size_t cap)
{
    jw w = {0};
    const json_value *v;
    if ((v = json_obj_get(step, "call"))) {
        const json_value *args = json_obj_get(step, "args");
        size_t i;
        jw_put(&w, "%s(", v->s);
        for (i = 0; i < json_arr_size(args); i++) { if (i) jw_put(&w, ", "); dump(&w, args->items[i]); }
        jw_put(&w, ")");
    } else if ((v = json_obj_get(step, "subscribe"))) {
        jw_put(&w, "subscribe %s as %s", v->s, json_as_str(json_obj_get(step, "as")));
    } else if ((v = json_obj_get(step, "take"))) {
        jw_put(&w, "take %lld from %s", (long long)json_as_int(json_obj_get(step, "count")), v->s);
    } else if ((v = json_obj_get(step, "get"))) {
        jw_put(&w, "get %s", v->s);
    } else {
        jw_put(&w, "?");
    }
    snprintf(buf, cap, "%s", w.b ? w.b : "");
    free(w.b);
}

/* Match a written result against `want`; false (and c->err) on a mismatch. */
static bool match_written(sc_ctx *c, jw *w, const json_value *want, const char *path)
{
    json_value *got;
    jw_finish(w);
    got = json_parse(w->b);
    if (!got) { bug(c, "the result writer produced invalid JSON: %.200s", w->b); return false; }
    if (mismatch(got, want, path, c->err, sizeof c->err)) { json_free(got); return false; }
    json_free(got);
    return true;
}

static const call_entry *find_call(sc_ctx *c, const char *name)
{
    size_t i;
    for (i = 0; i < sizeof k_calls / sizeof k_calls[0]; i++)
        if (!strcmp(k_calls[i].name, name) && (!strcmp(k_calls[i].group, c->group) || !strcmp(k_calls[i].group, "*")))
            return &k_calls[i];
    return NULL;
}

static bool run_call(sc_ctx *c, const char *name, const json_value *args, const json_value *step)
{
    const call_entry *e = find_call(c, name);
    const json_value *err = json_obj_get(step, "error"), *want = json_obj_get(step, "expect");
    size_t n = json_arr_size(args);
    jw w = {0};
    int rc;
    bool ok;
    if (!e) { bug(c, "%s has no '%s': add it to the alias table", c->cls, name); return false; }
    if (n < e->nmin || n > e->nmax) {
        bug(c, "%zu argument(s), the binding takes %zu..%zu", n, e->nmin, e->nmax);
        return false;
    }
    rc = e->fn(c, args, &w);
    if (rc == RUNNER_BUG) { free(w.b); return false; }
    if (err) {
        const char *kind;
        if (rc == DEPZ_OK) {
            jw_finish(&w);
            bug(c, "expected error %s, succeeded with %s", err->s, w.n ? w.b : "nothing");
            free(w.b);
            return false;
        }
        kind = error_kind(rc, depz_last_error());
        free(w.b);
        if (kind && !strcmp(kind, err->s)) return true;
        bug(c, "expected error %s, got %s (%s: %s)", err->s, kind ? kind : "unmapped", depz_err_str(rc), depz_last_error());
        return false;
    }
    if (rc) {
        free(w.b);
        bug(c, "raised %s: %s", depz_err_str(rc), depz_last_error());
        return false;
    }
    ok = !want || match_written(c, &w, want, step && json_obj_get(step, "get") ? name : "");
    free(w.b);
    return ok;
}

static bool run_step(sc_ctx *c, const json_value *step, const json_value *steps)
{
    const json_value *v;
    if ((v = json_obj_get(step, "call"))) return run_call(c, v->s, json_obj_get(step, "args"), step);
    if ((v = json_obj_get(step, "get"))) return run_call(c, v->s, NULL, step);
    if ((v = json_obj_get(step, "subscribe"))) {
        /* No drops: each queue holds everything its scenario takes, with room
         * for what streams on past the last take. */
        const char *as = json_as_str(json_obj_get(step, "as"));
        const stream_entry *e = NULL;
        size_t i, want = 0;
        for (i = 0; i < sizeof k_streams / sizeof k_streams[0]; i++)
            if (!strcmp(k_streams[i].group, c->group) && !strcmp(k_streams[i].name, v->s)) e = &k_streams[i];
        if (!e) { bug(c, "%s has no stream '%s': add it to the stream table", c->cls, v->s); return false; }
        if (c->n_subs == sizeof c->subs / sizeof c->subs[0]) { bug(c, "too many subscriptions"); return false; }
        for (i = 0; i < json_arr_size(steps); i++) {
            const json_value *t = json_obj_get(steps->items[i], "take");
            if (t && !strcmp(t->s, as)) want += (size_t)json_as_int(json_obj_get(steps->items[i], "count"));
        }
        want += e->item_size > 4096 ? 64 : 4096; /* a multizone frame is ~7.5 KB */
        c->subs[c->n_subs].name = as;
        c->subs[c->n_subs].item_size = e->item_size;
        c->subs[c->n_subs].write = e->write;
        c->subs[c->n_subs].s = e->open(c->dev, want);
        if (!c->subs[c->n_subs].s) { bug(c, "could not open the stream: %s", depz_last_error()); return false; }
        c->n_subs++;
        return true;
    }
    if ((v = json_obj_get(step, "take"))) {
        const json_value *want = json_obj_get(step, "expect");
        int64_t count = json_as_int(json_obj_get(step, "count"));
        sub_t *s = NULL;
        void *item;
        size_t i;
        bool ok = true;
        for (i = 0; i < c->n_subs; i++)
            if (!strcmp(c->subs[i].name, v->s)) s = &c->subs[i];
        if (!s) { bug(c, "no subscription named %s", v->s); return false; }
        if (!want || want->kind != JSON_ARRAY || (int64_t)want->count != count) {
            bug(c, "count and expect disagree");
            return false;
        }
        item = malloc(s->item_size);
        for (i = 0; ok && i < want->count; i++) {
            char path[32];
            jw w = {0};
            int rc = depz_stream_next(s->s, item, TAKE_TIMEOUT_MS);
            if (rc) {
                bug(c, "got %zu of %lld items, then %s (%s)", i, (long long)count, depz_err_str(rc), depz_last_error());
                ok = false;
                break;
            }
            s->write(&w, item);
            snprintf(path, sizeof path, "item[%zu]", i);
            ok = match_written(c, &w, want->items[i], path);
            free(w.b);
        }
        free(item);
        return ok;
    }
    bug(c, "unknown step");
    return false;
}

/* `end: exhausted`: every recorded tx byte written, every rx chunk served. */
static bool wait_exhausted(sc_ctx *c)
{
    size_t tx = 0, rx = 0;
    int i;
    for (i = 0; i < 200; i++) {
        if (depz_link_replay_pending(c->link, &tx, &rx)) { bug(c, "replay_pending: %s", depz_last_error()); return false; }
        if (!tx && !rx) return true;
        depz_sleep_ms(10);
    }
    bug(c, "capture not exhausted: %zu tx bytes never written, %zu rx chunks never delivered", tx, rx);
    return false;
}

/* After the steps: invariants the C binding checks beyond the scenario. */
static bool post_checks(sc_ctx *c)
{
    size_t i;
    for (i = 0; i < c->n_subs; i++)
        if (depz_stream_dropped_count(c->subs[i].s)) {
            bug(c, "%s: %llu items dropped", c->subs[i].name, (unsigned long long)depz_stream_dropped_count(c->subs[i].s));
            return false;
        }
    if (!strcmp(c->group, "bno086") && depz_bno086_shtp_discarded(c->dev)) {
        bug(c, "%llu SHTP cargos discarded", (unsigned long long)depz_bno086_shtp_discarded(c->dev));
        return false;
    }
    return true;
}

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

/* A legacy capture's probe (contract 15 §1): exactly the requests it holds
 * — GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME for the L5/L7 and 1D
 * families — then the resolved class attached in place. */
static bool open_legacy(sc_ctx *c, int vid, int pid, depz_identity *ident)
{
    char sw[64], dn[128] = "";
    int rc;
    if (depz_device_open_link(c->link, &c->dev)) {
        c->link = NULL; /* the device took it, also on failure */
        bug(c, "open: %s", depz_last_error());
        return false;
    }
    depz_device_set_timeout_ms(c->dev, TIMEOUT_MS);
    rc = depz_device_get_software_name(c->dev, sw, sizeof sw);
    if (rc) { bug(c, "get_software_name: %s: %s", depz_err_str(rc), depz_last_error()); return false; }
    depz_parse_software_name(sw, ident);
    if (ident->sensor_type == DEPZ_SENSOR_VL53L7 || ident->sensor_type == DEPZ_SENSOR_VL53LX) {
        if (depz_device_get_device_name(c->dev, dn, sizeof dn)) dn[0] = '\0';
    }
    rc = depz_attach_sensor_class(c->dev, depz_resolve_sensor_class(sw, dn, vid, pid));
    if (rc) { bug(c, "attach_sensor_class: %s: %s", depz_err_str(rc), depz_last_error()); return false; }
    return true;
}

/* Open the capture as its `probe` says (contract 15 §1). `usb_model` stands
 * in for the USB enumeration a replay has none of: its production id picks
 * the class the probe alone cannot tell (VL53L8CX vs CH). */
static bool open_scenario(sc_ctx *c, const json_value *sc)
{
    const char *model = json_as_str(json_obj_get(sc, "usb_model"));
    const char *probe = json_as_str(json_obj_get(sc, "probe"));
    const json_value *want_id = json_obj_get(sc, "identity");
    int vid = -1, pid = -1;
    depz_identity ident = {0};
    char path[1024];
    if (!probe) probe = "canonical";
    if (strcmp(probe, "legacy") && strcmp(probe, "canonical")) { bug(c, "unknown probe %s", probe); return false; }
    if (want_id && strcmp(probe, "legacy")) { bug(c, "`identity` is for legacy captures only"); return false; }
    if (model) {
        unsigned p;
        for (p = DEPZ_PID_RANGE_LO; p <= DEPZ_PID_RANGE_HI && pid < 0; p++) {
            const char *h = depz_usb_model_hint(DEPZ_USB_VID, (int)p);
            if (h && !strcmp(h, model)) { vid = DEPZ_USB_VID; pid = (int)p; }
        }
        if (pid < 0) { bug(c, "usb_model %s has no PID in the USB-id table", model); return false; }
    }
    snprintf(path, sizeof path, "%s/recordings/%s", DEPZ_VECTORS_DIR, json_as_str(json_obj_get(sc, "recording")));
    if (depz_link_open_replay(path, true, false, &c->link)) { bug(c, "replay %s: %s", path, depz_last_error()); return false; }
    if (!strcmp(probe, "legacy")) {
        if (!open_legacy(c, vid, pid, &ident)) return false;
    } else {
        int rc = depz_open_on_link(c->link, vid, pid, TIMEOUT_MS, &c->dev);
        if (rc) {
            c->link = NULL; /* open_on_link took it, also on failure */
            bug(c, "open_on_link: %s: %s", depz_err_str(rc), depz_last_error());
            return false;
        }
    }
    if (want_id) {
        jw wb = {0}, *w = &wb;
        const char *st = depz_sensor_type_str(ident.sensor_type);
        bool ok;
        OPEN(); KS("software_name", ident.software_name); KS("mode", ident.mode); KS("version", ident.version);
        if (st) { KS("sensor_type", st); } else { KN("sensor_type"); }
        CLOSE();
        ok = match_written(c, w, want_id, "identity");
        free(wb.b);
        if (!ok) return false;
    }
    classify(c);
    if (strcmp(c->cls, json_as_str(json_obj_get(sc, "class")))) {
        bug(c, "resolved %s, not %s", c->cls, json_as_str(json_obj_get(sc, "class")));
        return false;
    }
    return true;
}

int scenario_run(const char *id)
{
    const char *dir = getenv("DEPZ_SCENARIOS_DIR");
    char path[1024], what[512];
    char *text;
    json_value *sc;
    const json_value *steps;
    const char *end;
    sc_ctx c;
    size_t i;
    bool ok = true;

    memset(&c, 0, sizeof c);
    snprintf(path, sizeof path, "%s/%s.json", dir && dir[0] ? dir : DEPZ_SCENARIOS_DIR, id);
    text = slurp(path);
    if (!text) { fprintf(stderr, "  FAIL %s: cannot read %s\n", id, path); return 1; }
    sc = json_parse(text);
    free(text);
    if (!sc) { fprintf(stderr, "  FAIL %s: %s is not JSON\n", id, path); return 1; }
    if (strcmp(json_as_str(json_obj_get(sc, "schema")) ? json_as_str(json_obj_get(sc, "schema")) : "", "depz.scenario/1") ||
        strcmp(json_as_str(json_obj_get(sc, "id")) ? json_as_str(json_obj_get(sc, "id")) : "", id)) {
        fprintf(stderr, "  FAIL %s: not a depz.scenario/1 file with id %s\n", id, id);
        json_free(sc);
        return 1;
    }
    steps = json_obj_get(sc, "steps");
    end = json_as_str(json_obj_get(sc, "end"));

    if (!open_scenario(&c, sc)) {
        fprintf(stderr, "  FAIL %s open: %s\n", id, c.err);
        ok = false;
    }
    for (i = 0; ok && i < json_arr_size(steps); i++) {
        if (!run_step(&c, steps->items[i], steps)) {
            describe(steps->items[i], what, sizeof what);
            fprintf(stderr, "  FAIL %s step %zu %s: %s\n", id, i, what, c.err);
            ok = false;
        }
    }
    if (ok && !post_checks(&c)) { fprintf(stderr, "  FAIL %s: %s\n", id, c.err); ok = false; }
    if (ok && end && !strcmp(end, "exhausted") && !wait_exhausted(&c)) {
        fprintf(stderr, "  FAIL %s: %s\n", id, c.err);
        ok = false;
    }
    for (i = 0; i < c.n_subs; i++) depz_stream_close(c.subs[i].s);
    if (c.dev) depz_device_close(c.dev);
    else if (c.link) depz_link_free(c.link);
    if (ok) {
        snprintf(path, sizeof path, "scenarios/%s", id);
        vector_seen(path);
        snprintf(path, sizeof path, "recordings/%s", json_as_str(json_obj_get(sc, "recording")));
        vector_seen(path);
    }
    json_free(sc);
    return ok ? 0 : 1;
}
