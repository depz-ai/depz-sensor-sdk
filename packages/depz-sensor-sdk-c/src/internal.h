/* internal.h — internals shared by the codec layer and the live-hardware
 * layer. Not installed. */
#ifndef DEPZ_INTERNAL_H
#define DEPZ_INTERNAL_H

#include "depz_sensor_sdk.h"

#include <stdarg.h>

/* ── TMF8829 (tmf8829.c) ───────────────────────────────────────────────── */

/* Give a measurement whose histograms / ref_frames point into a builder its
 * own heap copy of them (frames_block; depz_tmf8829_measurement_free()
 * releases it): 0, or -1 out of memory with the measurement unchanged. */
int depz_tmf8829_measurement_own_frames(depz_tmf8829_measurement *m);

/* ── errors (error.c) ──────────────────────────────────────────────────── */

/* Whether `text` is well-formed UTF-8, as the strict decoders of the other SDKs
 * read it: no overlong forms, no surrogates, nothing past U+10FFFF. */
bool depz_utf8_valid(const char *text, size_t len);

/* Record the calling thread's failure detail and return `err`. */
int depz_fail(int err, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
/* As depz_fail, plus the device status that caused it. */
int depz_fail_status(int err, uint8_t cmd, uint8_t status);

/* The calling thread's failure detail, kept across a clean-up step that may
 * fail too, so the caller still sees the error that came first. */
#define DEPZ_ERROR_MSG_MAX 512
typedef struct depz_saved_error {
    char msg[DEPZ_ERROR_MSG_MAX];
    int  status;
    int  status_cmd;
} depz_saved_error;
void depz_error_save(depz_saved_error *out);
void depz_error_restore(const depz_saved_error *saved);

#endif /* DEPZ_INTERNAL_H */
