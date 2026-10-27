/* recorder.c — contract 09 datasets over live devices: the record value of
 * the result types that only exist in this layer, and the session recorder
 * that syncs every device's clock, hooks its result stream and writes one
 * `.depzdata` on a shared host timeline (the Python SDK's SessionRecorder).
 * The writer itself (src/dataset.c) is single-threaded; results arrive on
 * each device's reader thread, so one mutex serializes the writes. */
#include "io_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── record values ───────────────────────────────────────────────────── */

static depz_json *doubles(const double *a, size_t n)
{
    depz_json *v = depz_json_array();
    for (size_t i = 0; v && i < n; i++) depz_json_push(v, depz_json_double(a[i]));
    return v;
}

depz_json *depz_dataset_sr04_record(const depz_sr04_measurement *m)
{
    depz_json *v = depz_json_object();
    if (!v || !m) { depz_json_free(v); return NULL; }
    depz_json_set(v, "echo_us", depz_json_int(m->echo_time_us));
    depz_json_set(v, "source", depz_json_string(m->from_loop ? "loop" : "once"));
    return v;
}

depz_json *depz_dataset_vl53l8_live_record(const depz_vl53l8_live_frame *f, bool layers)
{
    if (!f) return NULL;
    depz_json *v = depz_dataset_vl53l8_record(&f->f, layers);
    if (!v) return NULL;
    if (f->has_motion) {
        const depz_vl53l8_motion *mo = &f->motion;
        depz_json *m = depz_json_object();
        depz_json_set(m, "global_indicator_1", depz_json_int(mo->global_indicator_1));
        depz_json_set(m, "global_indicator_2", depz_json_int(mo->global_indicator_2));
        depz_json_set(m, "status", depz_json_int(mo->status));
        depz_json_set(m, "nb_of_detected_aggregates", depz_json_int(mo->nb_of_detected_aggregates));
        depz_json_set(m, "nb_of_aggregates", depz_json_int(mo->nb_of_aggregates));
        depz_json *arr = depz_json_array();
        for (size_t i = 0; arr && i < 32; i++) depz_json_push(arr, depz_json_int(mo->motion[i]));
        depz_json_set(m, "motion", arr);
        depz_json_set(v, "motion", m);
    }
    return v;
}

/* Standard base64 with padding. */
static char *base64(const uint8_t *data, size_t n)
{
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *out = (char *)malloc((n + 2) / 3 * 4 + 1), *o = out;
    if (!out) return NULL;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t w = (uint32_t)data[i] << 16;
        if (i + 1 < n) w |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < n) w |= data[i + 2];
        *o++ = A[(w >> 18) & 63];
        *o++ = A[(w >> 12) & 63];
        *o++ = i + 1 < n ? A[(w >> 6) & 63] : '=';
        *o++ = i + 2 < n ? A[w & 63] : '=';
    }
    *o = '\0';
    return out;
}

depz_json *depz_dataset_vl53l8_cnh_record(const uint8_t *cnh_raw, size_t len,
                                          const depz_vl53l8_cnh_setup *setup)
{
    if (!setup || (!cnh_raw && len)) return NULL;
    char *b64 = base64(cnh_raw, len);
    depz_json *v = depz_json_object();
    if (!v || !b64) { free(b64); depz_json_free(v); return NULL; }
    depz_json_set(v, "nb_aggregates", depz_json_int(setup->nb_of_aggregates));
    depz_json_set(v, "feature_length", depz_json_int(setup->feature_length));
    depz_json_set(v, "sum_span", depz_json_int(setup->sum_span));
    /* Python: ref_bin_offset // 2048 (floor division) */
    depz_json_set(v, "start_bin", depz_json_int((int64_t)floor(setup->ref_bin_offset / 2048.0)));
    depz_json_set(v, "cnh_raw_b64", depz_json_string(b64));
    free(b64);
    return v;
}

depz_json *depz_dataset_vl53lx_record(const depz_vl53lx_measurement *m, const char *product,
                                      const char *driver)
{
    depz_json *v = depz_json_object();
    if (!v || !m) { depz_json_free(v); return NULL; }
    depz_json_set(v, "product", depz_json_string(product ? product : ""));
    depz_json_set(v, "driver", depz_json_string(driver ? driver : ""));
    depz_json_set(v, "status", depz_json_int(m->status));
    depz_json_set(v, "distance_mm", depz_json_int(m->distance_mm));
    depz_json_set(v, "sigma_mm", depz_json_double(m->sigma_mm));
    depz_json_set(v, "signal_kcps", depz_json_double(m->signal_kcps));
    depz_json_set(v, "ambient_kcps", depz_json_double(m->ambient_kcps));
    depz_json_set(v, "spads", depz_json_double(m->spads));
    if (m->n_targets) {
        depz_json *targets = depz_json_array();
        for (size_t i = 0; targets && i < m->n_targets && i < DEPZ_VL53LX_MAX_TARGETS; i++) {
            const depz_vl53lx_target *t = &m->targets[i];
            depz_json *tv = depz_json_object();
            depz_json_set(tv, "distance_mm", depz_json_int(t->distance_mm));
            depz_json_set(tv, "status", depz_json_int(t->status));
            depz_json_set(tv, "signal_kcps", depz_json_double(t->signal_kcps));
            depz_json_push(targets, tv);
        }
        depz_json_set(v, "targets", targets);
    }
    return v;
}

depz_json *depz_dataset_bno055_record(const depz_bno055_sample *s)
{
    depz_json *v = depz_json_object();
    if (!v || !s) { depz_json_free(v); return NULL; }
    depz_json_set(v, "unit_sel", depz_json_int(depz_bno055_pack_units(&s->units)));
    if (s->present & DEPZ_BNO055_HAS_QUATERNION) depz_json_set(v, "quaternion", doubles(s->quaternion, 4));
    if (s->present & DEPZ_BNO055_HAS_EULER) depz_json_set(v, "euler", doubles(s->euler, 3));
    if (s->present & DEPZ_BNO055_HAS_ACCEL) depz_json_set(v, "accel", doubles(s->accel, 3));
    if (s->present & DEPZ_BNO055_HAS_GYRO) depz_json_set(v, "gyro", doubles(s->gyro, 3));
    if (s->present & DEPZ_BNO055_HAS_MAG) depz_json_set(v, "mag", doubles(s->mag, 3));
    if (s->present & DEPZ_BNO055_HAS_LINEAR_ACCEL) depz_json_set(v, "linear_accel", doubles(s->linear_accel, 3));
    if (s->present & DEPZ_BNO055_HAS_GRAVITY) depz_json_set(v, "gravity", doubles(s->gravity, 3));
    if (s->present & DEPZ_BNO055_HAS_TEMPERATURE) depz_json_set(v, "temperature", depz_json_double(s->temperature));
    if (s->present & DEPZ_BNO055_HAS_CALIBRATION) {
        depz_json *c = depz_json_array();
        depz_json_push(c, depz_json_int(s->calibration.system));
        depz_json_push(c, depz_json_int(s->calibration.gyro));
        depz_json_push(c, depz_json_int(s->calibration.accel));
        depz_json_push(c, depz_json_int(s->calibration.mag));
        depz_json_set(v, "calib", c);
    }
    return v;
}

/* ── session recorder ────────────────────────────────────────────────── */

typedef struct {
    depz_session_recorder *rec;
    depz_device *dev;
    char id[32];
    int64_t offset_us;
    depz_json *meta;        /* the header entry (owned until start) */
    int token;              /* the stream hook, 0 = none */
    bool has_silicon;
    double silicon;         /* the sensor die's last temperature (VL53L8 frames) */
} rec_entry;

struct depz_session_recorder {
    char *path, *note;
    bool vl53l8_layers;
    double temperature_hz;
    rec_entry *entries;
    size_t n_entries, cap_entries;
    depz_mutex lock;        /* the writer, the entries' silicon readings */
    depz_dataset_writer *writer;
    /* the temperature sampler */
    depz_thread thread;
    bool thread_live, stop;
    depz_mutex tlock;
    depz_cond tcond;
};

depz_session_recorder *depz_session_recorder_new(const char *path, const depz_session_recorder_options *opt)
{
    static const depz_session_recorder_options defaults = DEPZ_SESSION_RECORDER_OPTIONS_INIT;
    if (!path) return NULL;
    if (!opt) opt = &defaults;
    depz_session_recorder *r = (depz_session_recorder *)calloc(1, sizeof *r);
    if (!r) return NULL;
    r->path = (char *)malloc(strlen(path) + 1);
    r->note = (char *)malloc(opt->note ? strlen(opt->note) + 1 : 1);
    if (!r->path || !r->note || depz_mutex_init(&r->lock)) {
        free(r->path); free(r->note); free(r);
        return NULL;
    }
    if (depz_mutex_init(&r->tlock)) { depz_mutex_destroy(&r->lock); free(r->path); free(r->note); free(r); return NULL; }
    if (depz_cond_init(&r->tcond)) {
        depz_mutex_destroy(&r->tlock); depz_mutex_destroy(&r->lock); free(r->path); free(r->note); free(r);
        return NULL;
    }
    strcpy(r->path, path);
    strcpy(r->note, opt->note ? opt->note : "");
    r->vl53l8_layers = opt->vl53l8_layers;
    r->temperature_hz = opt->temperature_hz;
    return r;
}

/* One record at device time `device_us`, mapped onto the host timeline. A
 * result racing stop() (the file already closed) is dropped. */
static void emit(rec_entry *e, int64_t device_us, const char *kind, depz_json *value)
{
    depz_session_recorder *r = e->rec;
    if (!value) return;
    depz_mutex_lock(&r->lock);
    if (r->writer) depz_dataset_writer_write(r->writer, e->id, device_us - e->offset_us, kind, value);
    else depz_json_free(value);
    depz_mutex_unlock(&r->lock);
}

static void on_sr04(const depz_sr04_measurement *m, void *user)
{
    emit((rec_entry *)user, (int64_t)m->timestamp_us, "sr04", depz_dataset_sr04_record(m));
}

static void on_vl53l4(const depz_vl53l4cd_measurement *m, void *user)
{
    emit((rec_entry *)user, (int64_t)m->timestamp_us, "vl53l4", depz_dataset_vl53l4_record(&m->r));
}

static void on_vl53l8(const depz_vl53l8_live_frame *f, void *user)
{
    rec_entry *e = (rec_entry *)user;
    depz_vl53l8_cnh_setup setup;
    depz_mutex_lock(&e->rec->lock);
    e->silicon = f->f.silicon_temp_degc;
    e->has_silicon = true;
    depz_mutex_unlock(&e->rec->lock);
    emit(e, (int64_t)f->f.timestamp_us, "vl53l8", depz_dataset_vl53l8_live_record(f, e->rec->vl53l8_layers));
    /* CH: the raw CNH block riding this frame, with the armed setup's decode
     * parameters (contract 09 `vl53l8_cnh`). */
    if (f->cnh_len && depz_vl53l8_cnh_armed(e->dev, &setup))
        emit(e, (int64_t)f->f.timestamp_us, "vl53l8_cnh", depz_dataset_vl53l8_cnh_record(f->cnh, f->cnh_len, &setup));
}

static void on_vl53lx(const depz_vl53lx_measurement *m, void *user)
{
    rec_entry *e = (rec_entry *)user;
    const depz_vl53lx_product_info *pi = depz_vl53lx_product_get(depz_vl53lx_product_bound(e->dev));
    const char *driver = depz_vl53lx_driver_str(depz_vl53lx_driver_bound(e->dev));
    emit(e, (int64_t)m->timestamp_us, "vl53lx", depz_dataset_vl53lx_record(m, pi ? pi->name : "", driver ? driver : ""));
}

static void on_bno055(const depz_bno055_sample *s, void *user)
{
    emit((rec_entry *)user, (int64_t)s->timestamp_us, "bno055", depz_dataset_bno055_record(s));
}

static void on_tmf8829(const depz_tmf8829_measurement *m, void *user)
{
    emit((rec_entry *)user, m->timestamp_us, "tmf8829", depz_dataset_tmf8829_record(m));
}

/* Every enabled sensor's report at its measurement time; reports of one cargo
 * carry out-of-order device times, so t may step back within a device —
 * readers sort (contract 09). */
static void on_bno086(const depz_bno_report *rp, void *user)
{
    emit((rec_entry *)user, rp->timestamp_us, "bno086", depz_dataset_bno086_record(rp));
}

static int hook(rec_entry *e)
{
    depz_device *d = e->dev;
    if (depz_is_sr04(d)) return depz_sr04_on_measurement(d, on_sr04, e, &e->token);
    if (depz_is_vl53l4cd(d)) return depz_vl53l4cd_on_measurement(d, on_vl53l4, e, &e->token);
    if (depz_is_vl53l8(d)) return depz_vl53l8_on_frame(d, on_vl53l8, e, &e->token);
    if (depz_is_vl53lx(d)) return depz_vl53lx_on_measurement(d, on_vl53lx, e, &e->token);
    if (depz_is_bno055(d)) return depz_bno055_on_sample(d, on_bno055, e, &e->token);
    if (depz_is_tmf8829(d)) return depz_tmf8829_on_measurement(d, on_tmf8829, e, &e->token);
    if (depz_is_bno086(d)) return depz_bno086_on_report(d, on_bno086, e, &e->token);
    return DEPZ_OK; /* a plain device: nothing to hook */
}

static void unhook(rec_entry *e)
{
    depz_device *d = e->dev;
    if (!e->token) return;
    if (depz_is_sr04(d)) depz_sr04_off_measurement(d, e->token);
    else if (depz_is_vl53l4cd(d)) depz_vl53l4cd_off_measurement(d, e->token);
    else if (depz_is_vl53l8(d)) depz_vl53l8_off_frame(d, e->token);
    else if (depz_is_vl53lx(d)) depz_vl53lx_off_measurement(d, e->token);
    else if (depz_is_bno055(d)) depz_bno055_off_sample(d, e->token);
    else if (depz_is_tmf8829(d)) depz_tmf8829_off_measurement(d, e->token);
    else if (depz_is_bno086(d)) depz_bno086_off_report(d, e->token);
    e->token = 0;
}

int depz_session_recorder_add(depz_session_recorder *r, depz_device *dev, const char *device_id,
                              int sync_samples, char *id_out, size_t cap)
{
    depz_time_sync sync;
    char text[128];
    depz_identity ident;
    int rc;
    if (!r || !dev) return depz_fail(DEPZ_E_ARG, "session_recorder_add: NULL argument");
    if (r->writer) return depz_fail(DEPZ_E_ARG, "add() every device before recording starts");
    if (device_id && strlen(device_id) >= sizeof ((rec_entry *)0)->id)
        return depz_fail(DEPZ_E_ARG, "device id longer than 31 bytes");
    if (r->n_entries == r->cap_entries) {
        size_t ncap = r->cap_entries ? r->cap_entries * 2 : 4;
        rec_entry *ne = (rec_entry *)realloc(r->entries, ncap * sizeof *ne);
        if (!ne) return depz_fail(DEPZ_E_NOMEM, "session_recorder_add: out of memory");
        r->entries = ne;
        r->cap_entries = ncap;
    }
    rec_entry *e = &r->entries[r->n_entries];
    memset(e, 0, sizeof *e);
    e->rec = r;
    e->dev = dev;
    if (device_id) strcpy(e->id, device_id);
    else snprintf(e->id, sizeof e->id, "d%zu", r->n_entries);

    rc = depz_device_sync_time(dev, sync_samples > 0 ? sync_samples : 5, &sync);
    if (rc) return rc;
    e->offset_us = sync.offset_us;
    e->meta = depz_json_object();
    if (!e->meta) return depz_fail(DEPZ_E_NOMEM, "session_recorder_add: out of memory");
    {
        depz_json *ts = depz_json_object();
        depz_json_set(ts, "offset_us", depz_json_int(sync.offset_us));
        depz_json_set(ts, "rtt_us", depz_json_int(sync.rtt_us));
        depz_json_set(e->meta, "time_sync", ts);
    }
    /* a board that does not answer these is still recorded */
    text[0] = '\0';
    if (depz_device_get_serial_number(dev, text, sizeof text) == DEPZ_OK) {
        depz_json_set(e->meta, "serial", depz_json_string(text));
        text[0] = '\0';
        if (depz_device_get_software_name(dev, text, sizeof text) == DEPZ_OK)
            depz_json_set(e->meta, "software_name", depz_json_string(text));
        else
            text[0] = '\0';
    } else {
        text[0] = '\0';
    }
    depz_parse_software_name(text, &ident);
    {
        const char *st = depz_sensor_type_str(ident.sensor_type);
        depz_json_set(e->meta, "sensor_type", depz_json_string(st ? st : "unknown"));
    }
    if (id_out && cap) { strncpy(id_out, e->id, cap - 1); id_out[cap - 1] = '\0'; }
    r->n_entries++;
    return DEPZ_OK;
}

static void temperature_loop(void *arg)
{
    depz_session_recorder *r = (depz_session_recorder *)arg;
    const int period_ms = (int)(1000.0 / r->temperature_hz);
    for (;;) {
        uint64_t deadline = depz_now_us() + (uint64_t)(period_ms > 0 ? period_ms : 1) * 1000u;
        depz_mutex_lock(&r->tlock);
        while (!r->stop && depz_now_us() < deadline)
            depz_cond_wait_ms(&r->tcond, &r->tlock, depz_ms_until(deadline));
        bool stop = r->stop;
        depz_mutex_unlock(&r->tlock);
        if (stop) return;
        for (size_t i = 0; i < r->n_entries; i++) {
            rec_entry *e = &r->entries[i];
            double celsius;
            bool has;
            depz_mutex_lock(&r->lock);
            has = e->has_silicon;
            celsius = e->silicon;
            if (has && r->writer)
                depz_dataset_writer_write(r->writer, e->id, (int64_t)depz_host_now_us(), "temperature",
                                          depz_dataset_temperature_record(celsius, "sensor"));
            depz_mutex_unlock(&r->lock);
            if (depz_device_read_mcu_temperature(e->dev, &celsius) != DEPZ_OK) continue;
            depz_mutex_lock(&r->lock);
            if (r->writer)
                depz_dataset_writer_write(r->writer, e->id, (int64_t)depz_host_now_us(), "temperature",
                                          depz_dataset_temperature_record(celsius, "mcu"));
            depz_mutex_unlock(&r->lock);
        }
    }
}

int depz_session_recorder_start(depz_session_recorder *r)
{
    if (!r) return depz_fail(DEPZ_E_ARG, "session_recorder_start: NULL recorder");
    if (r->writer) return DEPZ_OK;
    depz_json *devices = depz_json_object();
    if (!devices) return depz_fail(DEPZ_E_NOMEM, "session_recorder_start: out of memory");
    for (size_t i = 0; i < r->n_entries; i++) {
        /* the writer serializes the header: keep our copy for a restart */
        char *text = depz_json_dumps(r->entries[i].meta);
        depz_json_set(devices, r->entries[i].id, text ? depz_json_parse(text, strlen(text)) : depz_json_object());
        free(text);
    }
    depz_dataset_writer *w = depz_dataset_writer_open(r->path, devices, r->note[0] ? r->note : NULL);
    depz_json_free(devices);
    if (!w) return depz_fail(DEPZ_E_IO, "cannot create %s", r->path);
    depz_mutex_lock(&r->lock);
    r->writer = w;
    depz_mutex_unlock(&r->lock);
    for (size_t i = 0; i < r->n_entries; i++) {
        int rc = hook(&r->entries[i]);
        if (rc) { depz_session_recorder_stop(r); return rc; }
    }
    if (r->temperature_hz > 0) {
        r->stop = false;
        if (depz_thread_start(&r->thread, temperature_loop, r) == 0) r->thread_live = true;
    }
    return DEPZ_OK;
}

int depz_session_recorder_stop(depz_session_recorder *r)
{
    int rc = DEPZ_OK;
    if (!r) return DEPZ_OK;
    if (r->thread_live) {
        depz_mutex_lock(&r->tlock);
        r->stop = true;
        depz_cond_broadcast(&r->tcond);
        depz_mutex_unlock(&r->tlock);
        depz_thread_join(r->thread);
        r->thread_live = false;
    }
    for (size_t i = 0; i < r->n_entries; i++) unhook(&r->entries[i]);
    depz_mutex_lock(&r->lock);
    depz_dataset_writer *w = r->writer;
    r->writer = NULL;
    depz_mutex_unlock(&r->lock);
    if (w && depz_dataset_writer_close(w) != 0) rc = depz_fail(DEPZ_E_IO, "cannot finish %s", r->path);
    return rc;
}

uint64_t depz_session_recorder_records_written(const depz_session_recorder *r)
{
    uint64_t n = 0;
    if (!r) return 0;
    depz_mutex_lock((depz_mutex *)&r->lock);
    n = depz_dataset_writer_records_written(r->writer);
    depz_mutex_unlock((depz_mutex *)&r->lock);
    return n;
}

void depz_session_recorder_free(depz_session_recorder *r)
{
    if (!r) return;
    depz_session_recorder_stop(r);
    for (size_t i = 0; i < r->n_entries; i++) depz_json_free(r->entries[i].meta);
    free(r->entries);
    depz_cond_destroy(&r->tcond);
    depz_mutex_destroy(&r->tlock);
    depz_mutex_destroy(&r->lock);
    free(r->path);
    free(r->note);
    free(r);
}
