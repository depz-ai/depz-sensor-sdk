/* error.c — per-thread error detail: the message and device status behind
 * the last failure (depz_last_error(), depz_last_status()). In the codec
 * library so the dataset reader can name a damaged line the same way the
 * live layer names a failed request. */
#include "internal.h"

#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#  define DEPZ_TLS __declspec(thread)
#else
#  define DEPZ_TLS _Thread_local
#endif

static DEPZ_TLS char t_msg[DEPZ_ERROR_MSG_MAX];
static DEPZ_TLS int  t_status = -1;
static DEPZ_TLS int  t_status_cmd = -1;

const char *depz_err_str(int err)
{
    switch (err) {
    case DEPZ_OK:                return "DEPZ_OK";
    case DEPZ_E_ARG:             return "DEPZ_E_ARG";
    case DEPZ_E_NOMEM:           return "DEPZ_E_NOMEM";
    case DEPZ_E_IO:              return "DEPZ_E_IO";
    case DEPZ_E_CLOSED:          return "DEPZ_E_CLOSED";
    case DEPZ_E_TIMEOUT:         return "DEPZ_E_TIMEOUT";
    case DEPZ_E_STATUS:          return "DEPZ_E_STATUS";
    case DEPZ_E_BUSY:            return "DEPZ_E_BUSY";
    case DEPZ_E_PROTOCOL:        return "DEPZ_E_PROTOCOL";
    case DEPZ_E_NO_DEVICE:       return "DEPZ_E_NO_DEVICE";
    case DEPZ_E_WRONG_TYPE:      return "DEPZ_E_WRONG_TYPE";
    case DEPZ_E_REPLAY_MISMATCH: return "DEPZ_E_REPLAY_MISMATCH";
    case DEPZ_E_BOOTLOADER:      return "DEPZ_E_BOOTLOADER";
    default:                     return "DEPZ_E_?";
    }
}

const char *depz_last_error(void) { return t_msg; }
int depz_last_status(void) { return t_status; }
int depz_last_status_cmd(void) { return t_status_cmd; }

int depz_fail(int err, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t_msg, sizeof t_msg, fmt, ap);
    va_end(ap);
    t_status = -1;
    t_status_cmd = -1;
    return err;
}

void depz_error_save(depz_saved_error *out)
{
    memcpy(out->msg, t_msg, sizeof out->msg);
    out->status = t_status;
    out->status_cmd = t_status_cmd;
}

void depz_error_restore(const depz_saved_error *saved)
{
    memcpy(t_msg, saved->msg, sizeof t_msg);
    t_status = saved->status;
    t_status_cmd = saved->status_cmd;
}

static const char *status_name(uint8_t s)
{
    static const char *names[] = {
        "OK", "ERROR", "ERR_INVALID_CMD", "ERR_PAYLOAD_FORMAT", "ERR_INVALID_PARAM",
        "ERR_PAYLOAD_CRC", "ERR_BUSY", "ERR_CMD_NOT_SUPPORTED", "ERR_NOT_INITIALIZED",
        "ERR_HARDWARE_FAULT"};
    return s < sizeof names / sizeof names[0] ? names[s] : "?";
}

int depz_fail_status(int err, uint8_t cmd, uint8_t status)
{
    snprintf(t_msg, sizeof t_msg, "cmd 0x%02X failed: %s (0x%02X)", cmd, status_name(status),
             status);
    t_status = status;
    t_status_cmd = cmd;
    return err;
}
