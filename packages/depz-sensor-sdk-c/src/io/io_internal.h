/* io_internal.h — shared internals of the live-hardware layer. Not installed. */
#ifndef DEPZ_IO_INTERNAL_H
#define DEPZ_IO_INTERNAL_H

#include "depz_sensor_io.h"
#include "platform.h"
#include "../internal.h" /* depz_fail / depz_fail_status (error.c) */

/* ── strings ───────────────────────────────────────────────────────────── */

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
void depz_replay_pending_impl(void *self, size_t *tx_left, size_t *rx_left);

/* Platform serial port (serial_posix.c / serial_win32.c). */
int depz_serial_link_open(const char *port, depz_link **out);
/* Platform port enumeration (ports_linux.c / ports_macos.c / ports_win32.c). */
int depz_enumerate_ports(depz_port_info **out, size_t *count);
/* `a` and `b` name the same port (contract 02 §4.3, ERRATA A5): the same
 * resolved path (/dev/serial/by-id/… is a symlink to the node), or the same
 * character device under two names. Never true of a COM name: Windows ports
 * are not files (serial_posix.c / serial_win32.c). */
bool depz_same_port(const char *a, const char *b);

/* ── callback lists ─────────────────────────────────────────────────────── */

typedef struct {
    int   token;
    void (*fn)(void);
    void *user;
    depz_stream_where where;    /* NULL: every item */
} depz_cb_entry;

/* The callbacks of one stream. A dispatch is a snapshot taken under the
 * lock, the calls made without it, then the release: remove() returns only
 * once no dispatch is in progress on another thread, so a caller that
 * unsubscribed may free what the callback used (the session recorder's
 * entries). A callback removing itself (or any entry) from the dispatching
 * thread does not wait. */
typedef struct {
    depz_mutex     lock;
    depz_cond      released;    /* a dispatch ended */
    depz_cb_entry *items;
    size_t         count, cap;
    int            next_token;
    unsigned       dispatching; /* snapshots not yet released */
    depz_thread_id dispatcher;  /* the thread of the latest one */
} depz_cb_list;

int  depz_cb_list_init(depz_cb_list *l);
void depz_cb_list_free(depz_cb_list *l);
int  depz_cb_list_add(depz_cb_list *l, void (*fn)(void), void *user, depz_stream_where where,
                      int *token);
/* Remove the entry, then wait out a dispatch running on another thread. */
void depz_cb_list_remove(depz_cb_list *l, int token);
/* Snapshot under the lock; callbacks run without it; release when done
 * (frees *out, wakes remove()). A snapshot of zero entries needs no release. */
size_t depz_cb_list_snapshot(depz_cb_list *l, depz_cb_entry **out);
void   depz_cb_list_release(depz_cb_list *l, depz_cb_entry *out);

/* ── streams: one per kind of item a device emits (contract 07 §3) ──────── */

/* The subscriptions of a stream, reference-counted: a subscription may
 * outlive the device it was made on (util.c). */
typedef struct depz_hub depz_hub;

/* What a sensor class says about its stream, in its depz_sensor_ops: the
 * device core makes the stream from it on attach. */
typedef struct {
    const char *name;            /* "sr04 measurements": error messages */
    size_t      item_size;       /* sizeof the item type */
    size_t      default_maxsize; /* of a subscription made with maxsize 0 */
    int         report;          /* the report id next() names; < 0 = no next() (events) */
    /* Items that point into reader-owned memory (a TMF8829 measurement's
     * frames), both NULL otherwise: after the memcpy into a subscription,
     * clone(item) gives the copy its own heap part (0; -1 out of memory: the
     * copy is not queued, the subscription counts it dropped); drop(item)
     * frees that part of a queued copy the subscription lets go (drop-oldest,
     * closed with items inside). A copy next() hands out is the consumer's
     * to free. */
    int  (*clone)(void *item);
    void (*drop)(void *item);
} depz_stream_desc;

/* The public depz_stream: the owner side. Every public call enters `dev`,
 * so depz_device_close() on another thread frees nothing under it. */
struct depz_stream {
    depz_device *dev;
    depz_stream_desc desc;
    depz_cb_list cbs;
    depz_hub    *hub;
    depz_mutex   lock;           /* parse_errors */
    uint64_t     parse_errors;
    bool         live;           /* between init and free */
};

int  depz_stream_init(depz_stream *s, depz_device *dev, const depz_stream_desc *desc);
/* The producer is gone: callbacks freed, subscriptions end once drained. */
void depz_stream_free(depz_stream *s);
/* No more items (reader stopped): subscriptions drain what they hold, then
 * report DEPZ_E_LOST with `lost_reason` after a loss, DEPZ_E_CLOSED when it
 * is NULL; later ones start the same. */
void depz_stream_end(depz_stream *s, const char *lost_reason);
/* Reader thread: `item` to every callback (its `where` permitting), then
 * `queued` (NULL = `item`; a copy without reader-owned pointers) to every
 * subscription (contract 07 §3). */
void depz_stream_emit(depz_stream *s, const void *item);
void depz_stream_emit_as(depz_stream *s, const void *item, const void *queued);
/* Reader thread: a report of this stream that did not decode (contract 07 §2). */
void depz_stream_count_parse_error(depz_stream *s);
/* TMF8829 start(): its counters restart with the stream (contract 14). */
void depz_stream_reset_parse_errors(depz_stream *s);

/* ── device lifetime ────────────────────────────────────────────────────── */

/* Called by depz_device_close() on the closing thread, after the reader is
 * joined and every call has left, before the device is freed: whoever keeps
 * a depz_device* beyond the caller's control (the session recorder) drops
 * it here. Never remove from inside the callback another thread waits on. */
typedef void (*depz_device_close_fn)(depz_device *dev, void *user);
int  depz_device_on_close(depz_device *dev, depz_device_close_fn fn, void *user, int *token);
void depz_device_off_close(depz_device *dev, int token);

/* ── device ─────────────────────────────────────────────────────────────── */

typedef struct depz_pending depz_pending;

/* What a sensor class plugs into the device core. */
typedef struct {
    depz_sensor_type type;
    /* The class's stream (measurements / frames / samples / reports): the
     * core makes `dev->stream` from it on attach, ends it when the reader
     * stops and frees it on close. A NULL name: the class streams nothing. */
    depz_stream_desc stream;
    /* Reader thread, device lock NOT held: consume a report, return true. */
    bool (*handle_report)(depz_device *dev, uint8_t cmd, const uint8_t *p, size_t len);
    /* Reader gone (close or disconnect), the stream already ended: wake
     * whatever else waits on the class. May be NULL. */
    void (*on_closed)(depz_device *dev);
    /* depz_device_close, after the reader has stopped. */
    void (*destroy)(depz_device *dev);
    /* depz_device_close, first, with the link up and the reader answering: a
     * sensor that keeps running on its own stops here. Nothing it fails stops
     * the close. May be NULL. */
    void (*before_close)(depz_device *dev);
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
    bool lost;            /* under `lock`: the reader ended on its own; set before the link closes */
    char lost_reason[256];/* set with `lost`, then never changed */
    int  users;           /* under `lock`: threads inside public calls */
    bool reader_running;
    bool reader_done;
    bool shut_down; /* depz_device_shutdown() has finished; under `life_lock` */
    /* depz_device_shutdown() and depz_device_free() run one at a time: a free on
     * another thread waits for a shutdown in progress (its close callbacks)
     * instead of running a second one over it. Not on the reader's own thread
     * (a shutdown from a callback): the holder may be joining that thread. */
    depz_mutex life_lock;
    bool life_held; /* under `life_lock`: its holder is inside a shutdown or free */
    depz_thread reader;

    depz_cb_list close_cbs;  /* depz_device_on_close: run by close() before the device is freed */
    depz_stream events;      /* contract 07 §2: every device's */
    depz_stream stream;      /* the sensor class's (ops->stream), once attached */

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
/* Attach a sensor class to a running plain device (depz_attach_sensor_class):
 * makes its stream, then publishes `ops`/`sensor` to the reader. On failure
 * calls ops->destroy on `sensor`: DEPZ_E_NOMEM, or DEPZ_E_CLOSED once the
 * reader has left (a lost device: nothing would end the class stream). */
int depz_device_attach(depz_device *dev, const depz_sensor_ops *ops, void *sensor);

/* The device's default request timeout (depz_device_set_timeout_ms), read
 * under the lock: the setter may run on another thread. */
int  depz_device_timeout_ms(const depz_device *dev);

/* A public call entering / leaving the device: depz_device_close() waits
 * for every thread inside to leave before it frees anything, so a call may
 * be in flight on another thread when close begins. Enter fails with
 * DEPZ_E_CLOSED once close has begun; every successful enter is paired with
 * one leave, after the last use of the device (error messages included). */
/* Fail with what an ended device says (contract 07 §3): DEPZ_E_LOST after a
 * loss, whatever came after it; else DEPZ_E_CLOSED. */
int  depz_device_fail_ended(depz_device *dev);
/* DEPZ_OK on a device whose reader still runs; else what a request says
 * (depz_device_fail_ended). A sensor calls it before a guard of its own state
 * — "stop the stream first" — so that a board unplugged while it streamed
 * says it is lost, not that it still streams (contract 07 §3). */
int  depz_device_require_open(depz_device *dev);
int  depz_device_enter(depz_device *dev);
void depz_device_leave(depz_device *dev);
/* The same for a const getter, which has no rc to carry DEPZ_E_CLOSED: true
 * = entered (leave with depz_device_leave), false = close has begun and the
 * getter answers its "not this device" value. Sets no error. */
bool depz_device_enter_const(const depz_device *dev);

/* The body of a public call, inside enter/leave: the device is entered
 * first (a NULL `dev` gets `check`'s own message), then `check` (the class's
 * type test) and `body` run entered — nothing of `dev`, its port name
 * included, is read before, as close() on another thread may free it then. */
#define DEPZ_ENTERED(dev, check, body)                                \
    do {                                                              \
        int entered_rc_;                                              \
        if (!(dev)) return (check);                                   \
        entered_rc_ = depz_device_enter(dev);                         \
        if (entered_rc_) return entered_rc_;                          \
        entered_rc_ = (check);                                        \
        if (!entered_rc_) entered_rc_ = (body);                       \
        depz_device_leave(dev);                                       \
        return entered_rc_;                                           \
    } while (0)

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

/* The sensor classes' attach hooks (depz_attach_sensor_class dispatches on the name). */
int depz_sr04_attach(depz_device *dev);
int depz_vl53l4cd_attach(depz_device *dev);
int depz_vl53l8_attach(depz_device *dev, depz_vl53l8_model model);
int depz_bno055_attach(depz_device *dev);
int depz_tmf8829_attach(depz_device *dev);
int depz_bno086_attach(depz_device *dev);
/* usb_model / device_name (either may be NULL) pick the product class. */
int depz_vl53lx_attach(depz_device *dev, const char *usb_model, const char *device_name);

#endif /* DEPZ_IO_INTERNAL_H */
