/* io_internal.h — shared internals of the live-hardware layer. Not installed. */
#ifndef DEPZ_IO_INTERNAL_H
#define DEPZ_IO_INTERNAL_H

#include "depz_sensor_io.h"
#include "platform.h"

#include <stdarg.h>

/* ── errors ────────────────────────────────────────────────────────────── */

/* Record the calling thread's failure detail and return `err`. */
int depz_fail(int err, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
/* As depz_fail, plus the device status that caused it. */
int depz_fail_status(int err, uint8_t cmd, uint8_t status);
/* Copy `src` into `dst` (capacity `cap`), always terminated. */
void depz_strlcpy(char *dst, const char *src, size_t cap);

/* ── links ─────────────────────────────────────────────────────────────── */

struct depz_link {
    const depz_link_vtable *vt;
    void *self;
    char name[256];
    depz_mutex lock;
    bool closed;
    bool is_replay;
};

bool depz_replay_exhausted_impl(void *self);

/* Platform serial port (serial_posix.c / serial_win32.c). */
int depz_serial_link_open(const char *port, depz_link **out);
/* Platform port enumeration (ports_linux.c / ports_macos.c / ports_win32.c). */
int depz_enumerate_ports(depz_port_info **out, size_t *count);

/* ── stream hubs: one per kind of item a device publishes ───────────────── */

typedef struct depz_hub depz_hub;

depz_hub    *depz_hub_new(size_t item_size);
/* Subscribe a new bounded stream; NULL on allocation failure. */
depz_stream *depz_hub_subscribe(depz_hub *h, size_t maxsize);
void         depz_hub_push(depz_hub *h, const void *item);
/* No more items (device closed or lost): streams drain what they hold, then
 * report CLOSED; later subscribers start closed. */
void         depz_hub_mark_closed(depz_hub *h);
/* Mark closed and drop the producer's reference; the hub lives on while
 * streams do. */
void         depz_hub_close(depz_hub *h);

/* ── callback lists ─────────────────────────────────────────────────────── */

typedef struct {
    int   token;
    void (*fn)(void);
    void *user;
} depz_cb_entry;

typedef struct {
    depz_mutex     lock;
    depz_cb_entry *items;
    size_t         count, cap;
    int            next_token;
} depz_cb_list;

int  depz_cb_list_init(depz_cb_list *l);
void depz_cb_list_free(depz_cb_list *l);
int  depz_cb_list_add(depz_cb_list *l, void (*fn)(void), void *user, int *token);
void depz_cb_list_remove(depz_cb_list *l, int token);
/* Snapshot under the lock (caller frees *out); callbacks run without it. */
size_t depz_cb_list_snapshot(depz_cb_list *l, depz_cb_entry **out);

/* ── device ─────────────────────────────────────────────────────────────── */

typedef struct depz_pending depz_pending;

/* What a sensor class plugs into the device core. */
typedef struct {
    depz_sensor_type type;
    /* Reader thread, device lock NOT held: consume a report, return true. */
    bool (*handle_report)(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len);
    /* Reader gone (close or disconnect): close the sensor's hubs. */
    void (*on_closed)(depz_device *dev);
    /* depz_device_close, after the reader has stopped. */
    void (*destroy)(depz_device *dev);
} depz_sensor_ops;

struct depz_device {
    depz_link *link;
    char port[256];
    int  timeout_ms;
    depz_crc_type tx_crc;

    depz_mutex lock;      /* pending list, stats, sensor attachment, sync */
    depz_cond  cond;
    depz_pending *pending;

    depz_mutex tx_lock;
    uint8_t tx_seq;

    depz_parser parser;
    depz_link_stats stats;
    int last_rx_seq;      /* -1 before the first packet */

    bool closing;         /* under `lock` */
    bool reader_running;
    bool reader_done;
    depz_thread reader;

    depz_cb_list event_cbs;
    depz_hub    *event_hub;

    bool synced;
    depz_time_sync sync;

    depz_sensor_type sensor_type;
    const depz_sensor_ops *ops;
    void *sensor;
};

/* Create the device on `link` with `ops`/`sensor` attached (either NULL) and
 * start its reader. On failure frees `link` and calls ops->destroy. */
int depz_device_create(depz_link *link, const depz_sensor_ops *ops, void *sensor,
                       depz_device **out);
/* Attach a sensor class to a running device (promote). */
void depz_device_attach(depz_device *dev, const depz_sensor_ops *ops, void *sensor);

/* Emit to event callbacks + event streams. */
void depz_device_emit(depz_device *dev, const depz_device_event *ev);

/* ── register bridges (VL53L4CD, VL53L5/L7, the VL53L 1D family, BNO055) ── */

/* READ_REG / WRITE_REG over a device, split at the bridge's transfer limit.
 * The reply is RPT_*_REG_DATA: echoed opcode, u64 MCU timestamp, the bytes. */
typedef struct {
    depz_device *dev;
    uint8_t  cmd_read, cmd_write, rpt_reg_data;
    size_t   xfer_max;           /* write chunk; also the read chunk ... */
    size_t   read_max;           /* ... unless this is set */
    int      timeout_ms;
    uint64_t last_timestamp_us;  /* MCU time of the latest register read */
    /* Called after each chunk of a write longer than one chunk. */
    void   (*write_progress)(size_t done, size_t total, void *user);
    void    *progress_user;
} depz_regbridge;

int depz_rb_read(depz_regbridge *rb, uint16_t addr, uint8_t *out, size_t len);
int depz_rb_write(depz_regbridge *rb, uint16_t addr, const uint8_t *data, size_t len);

/* A driver-side wait (ULD poll period, boot delay). Skipped on a replay link:
 * the answers are recorded, so waiting only slows the replay down. */
void depz_device_sleep_ms(depz_device *dev, int ms);

/* The sensor classes' promote hooks (discovery.c dispatches on the type). */
int depz_sr04_attach(depz_device *dev);
int depz_vl53l4cd_attach(depz_device *dev);
int depz_vl53l8_attach(depz_device *dev, depz_vl53l8_model model);
int depz_bno055_attach(depz_device *dev);
int depz_bno086_attach(depz_device *dev);
/* usb_model / device_name (either may be NULL) pick the product class. */
int depz_vl53lx_attach(depz_device *dev, const char *usb_model, const char *device_name);

#endif /* DEPZ_IO_INTERNAL_H */
