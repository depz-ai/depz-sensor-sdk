/*
 * depz_sensor_io.h — DEPZ USB sensor-line C SDK, live-hardware layer.
 *
 * Everything that talks to a board (contract 07 §1 layers 1–3): byte links
 * (serial port, loopback, .depzrec record/replay), the device core (reader
 * thread, request/response correlation, events, streams, the contract-02
 * common commands), discovery (USB-id filtered port enumeration + protocol
 * probe) and the sensor classes. Builds on the codecs of depz_sensor_sdk.h.
 * Linux, macOS and Windows.
 *
 * Conventions
 *   - Functions return 0 (DEPZ_OK) or a negative depz_err. After a failure,
 *     depz_last_error() holds a message and depz_last_status() the device
 *     status code (DEPZ_E_STATUS / DEPZ_E_BUSY) — both per calling thread.
 *   - Timeouts are milliseconds; a negative timeout means "the default"
 *     (the device's request timeout, 200 ms unless changed).
 *   - Callbacks run on the device's reader thread: keep them short, never
 *     block in them, and never call depz_device_close() from one.
 *   - depz_device_close() may run while another thread waits inside a call
 *     on the device: that call then fails with DEPZ_E_CLOSED.
 *   - A device owns the link it was opened on; depz_device_close() closes
 *     and frees both.
 *
 * Sensor classes: SR04 (contract 03), VL53L4CD (contract 10), the
 * multizone VL53L8CX / VL53L8CH / VL53L5CX / VL53L7CX / VL53L7CH (contracts
 * 04, 11), the VL53L 1D family VL53L0X / L1CX / L1CB / L3CX / L4CX (contract
 * 12), BNO055 (contract 13), TMF8829 (contract 14), BNO085 / BNO086
 * (contract 05). A board of an
 * unknown firmware opens as a plain device — common commands work.
 */
#ifndef DEPZ_SENSOR_IO_H
#define DEPZ_SENSOR_IO_H

#include "depz_sensor_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Errors: the depz_err codes, depz_err_str() and the per-thread failure
 * detail (depz_last_error(), depz_last_status()) are in depz_sensor_sdk.h —
 * the dataset reader reports a damaged file the same way. */

/* ======================================================================== */
/* Byte links (contract 07 §1 layer 1, contract 08)                          */
/* ======================================================================== */

typedef struct depz_link depz_link;

/* Custom links: `read` returns DEPZ_OK with *got = 0 on timeout, or
 * DEPZ_E_CLOSED once closed; `close` must be idempotent, callable from any
 * thread, and wake a blocked `read`; `destroy` frees `self`. */
typedef struct {
    int  (*read)(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got);
    int  (*write)(void *self, const uint8_t *data, size_t len);
    void (*close)(void *self);
    void (*destroy)(void *self);
} depz_link_vtable;

int depz_link_new(const depz_link_vtable *vt, void *self, const char *name, depz_link **out);

/* The CDC-ACM serial port `port` ("/dev/ttyACM0", "/dev/cu.usbmodem1101",
 * "COM7"), opened exclusively (a second opener fails instead of silently
 * sharing the byte stream). On POSIX the tty is left in a sane cooked state
 * on close, so the next program (e.g. a browser's Web Serial) can use it. */
int depz_link_open_serial(const char *port, depz_link **out);

/* Replay the rx side of a .depzrec capture, causally: an rx chunk is served
 * only once the host has written as many bytes as preceded it. `strict_tx`
 * fails any write that differs from the recorded tx stream
 * (DEPZ_E_REPLAY_MISMATCH); `realtime` paces rx by the recorded times. */
int depz_link_open_replay(const char *path, bool strict_tx, bool realtime, depz_link **out);

/* Tee `inner` into a new .depzrec file at `path` (tx journaled before the
 * physical write). `header_extra_json` is either NULL or the inside of a JSON
 * object (`"port":"live","note":"x"`) merged into the header line. Takes
 * ownership of `inner`, also on failure. */
int depz_link_open_recording(depz_link *inner, const char *path,
                             const char *header_extra_json, depz_link **out);

/* Two in-memory links: what one writes, the other reads (tests, fakes). */
int depz_link_loopback_pair(depz_link **a, depz_link **b);

int  depz_link_read(depz_link *l, uint8_t *buf, size_t cap, int timeout_ms, size_t *got);
int  depz_link_write(depz_link *l, const uint8_t *data, size_t len);
void depz_link_close(depz_link *l);
bool depz_link_closed(const depz_link *l);
const char *depz_link_name(const depz_link *l);
/* Close (if needed) and free. Not for a link a device owns. */
void depz_link_free(depz_link *l);

/* Replay only: true once every recorded rx chunk has been served. */
bool depz_link_replay_exhausted(const depz_link *l);
/* Replay only: recorded tx bytes not yet written and rx chunks not yet served
 * (both 0 = the whole capture was played). DEPZ_E_ARG on another link. */
int  depz_link_replay_pending(const depz_link *l, size_t *tx_bytes_left, size_t *rx_chunks_left);

/* ======================================================================== */
/* Bounded drop-oldest streams (contract 07 §3)                              */
/* ======================================================================== */

/* A pull subscription: the reader thread pushes, the owner pulls. When full,
 * the oldest item goes and dropped_count() grows. Registered at creation, so
 * nothing produced after the call is missed. */
typedef struct depz_stream depz_stream;

/* Next item into `item` (its size is fixed by the stream's type). DEPZ_OK,
 * DEPZ_E_TIMEOUT, or DEPZ_E_CLOSED once the device is closed and drained. */
int      depz_stream_next(depz_stream *s, void *item, int timeout_ms);
uint64_t depz_stream_dropped_count(const depz_stream *s);
/* Unsubscribe and free. Call before closing its device, or after — both fine. */
void     depz_stream_close(depz_stream *s);

/* ======================================================================== */
/* Device core (contract 02 / 07)                                            */
/* ======================================================================== */

typedef struct depz_device depz_device;

#define DEPZ_DEFAULT_TIMEOUT_MS 200

typedef struct {
    uint64_t tx_packets, rx_packets, tx_bytes, rx_bytes;
    uint64_t crc_errors, header_errors, trash_bytes;
    uint64_t seq_gaps;          /* gaps in device->host seq (host-observed) */
    uint64_t device_seq_errors; /* RPT_SEQUENCE_ERROR count (device-observed) */
    uint64_t callback_errors;   /* callbacks that raised (contract 07 §3): always 0 here, a C
                                 * callback cannot raise; the C++ wrapper counts its own */
} depz_link_stats;

/* A plain device on a serial port / on a link (takes ownership of `link`,
 * also on failure). No identity probe: the sensor type stays
 * DEPZ_SENSOR_NONE until depz_device_promote(). */
int depz_device_open(const char *port, depz_device **out);
int depz_device_open_link(depz_link *link, depz_device **out);

/* The in-place probe captures older than contract 02 §4.4 hold (contract 15
 * `probe: "legacy"`): ask GET_NAME_ACTIVE_SOFTWARE — plus GET_DEVICE_NAME
 * for the L5/L7 and 1D families only — then attach the class
 * depz_resolve_sensor_class() picks, in place. DEPZ_E_BOOTLOADER in
 * bootloader mode. New code opens with depz_open_device() or
 * depz_open_on_link() (the §4.4 probe). */
int depz_device_promote(depz_device *dev);
/* depz_device_promote() with the board's USB id (-1 = unknown), which picks
 * the class the probe alone cannot (VL53L8CX vs VL53L8CH, the L5/L7 and
 * 1D-family models); *ident (optional) receives the parsed answer. */
int depz_device_promote_ex(depz_device *dev, int usb_vid, int usb_pid, depz_identity *ident);

/* Continue the open plain device `dev` as the class `class_name` (one of
 * depz_sensor_class_name(), as from depz_resolve_sensor_class()) on the same
 * connection — contract 02 §4.4 attach_sensor_class: no reopen, the reader
 * and the host sequence numbers run on. `dev` must have no class yet and no
 * request in flight (DEPZ_E_ARG otherwise); "device" leaves it the base
 * device; "bootloader" fails with DEPZ_E_BOOTLOADER. */
int depz_attach_sensor_class(depz_device *dev, const char *class_name);

/* Open a device on a connection that cannot be reopened — a replay or
 * recording link, an adopted transport (contract 02 §4.4 open_on_link).
 * Takes ownership of `link` (also on failure), runs the probe
 * depz_probe_port() runs — GET_NAME_ACTIVE_SOFTWARE, GET_DEVICE_NAME,
 * GET_SERIAL — on it, resolves the class with depz_resolve_sensor_class()
 * from the answers and (`usb_vid`, `usb_pid`) (-1 = unknown), and continues
 * as that class on the same link. `timeout_ms` <= 0 = default. No answer to
 * GET_NAME_ACTIVE_SOFTWARE -> DEPZ_E_NO_DEVICE; a bootloader ->
 * DEPZ_E_BOOTLOADER. */
int depz_open_on_link(depz_link *link, int usb_vid, int usb_pid, int timeout_ms, depz_device **out);

/* Stop the reader, close the link, free everything. NULL is a no-op.
 * May be called from another thread while a call on `dev` is in progress:
 * a call waiting for the board, a frame or a measurement returns
 * DEPZ_E_CLOSED, and close frees nothing until every call has returned.
 * Never from a callback, and no call may begin on `dev` once close has
 * returned. */
void depz_device_close(depz_device *dev);

void             depz_device_set_timeout_ms(depz_device *dev, int timeout_ms);
depz_sensor_type depz_device_sensor_type(const depz_device *dev);
const char      *depz_device_port(const depz_device *dev);
bool             depz_device_closed(const depz_device *dev);
void             depz_device_stats(const depz_device *dev, depz_link_stats *out);

/* Requests ----------------------------------------------------------------- */

/* A matcher sees every non-status packet while its request is in flight (on
 * the reader thread, under the device lock — copy what you need into `ctx`,
 * do nothing else). Return true to claim the packet and complete. */
typedef bool (*depz_matcher)(uint8_t cmd, const uint8_t *payload, size_t len, void *ctx);

/* Send `cmd` and wait for its completion: `matcher` claiming a packet, or,
 * with `ok_completes`, RPT_STATUS(cmd, OK). A non-OK RPT_STATUS echoing
 * `cmd` fails with DEPZ_E_STATUS (DEPZ_E_BUSY for ERR_BUSY). One request per
 * opcode in flight. */
int depz_device_request(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len,
                        depz_matcher matcher, void *ctx, bool ok_completes, int timeout_ms);
/* Fire-and-forget (escape hatch). */
int depz_device_send(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len);

/* Common commands (contract 02) -------------------------------------------- */

int depz_device_get_device_name(depz_device *dev, char *out, size_t cap);
int depz_device_get_software_name(depz_device *dev, char *out, size_t cap);
int depz_device_get_serial_number(depz_device *dev, char *out, size_t cap);
/* Last cached MCU temperature, °C (the device refreshes it ~2 Hz). */
int depz_device_read_mcu_temperature(depz_device *dev, double *celsius);
int depz_device_get_payload_crc_type(depz_device *dev, depz_crc_type *out);
/* Device->host payload CRC; host->device CRC is per packet. DEPZ_E_ARG for a
 * value that is not a listed depz_crc_type. */
int depz_device_set_payload_crc_type(depz_device *dev, depz_crc_type t);
/* Sync pins 1..5, mode 0..4, polarity 0..1 (contract 02 §6): DEPZ_E_ARG
 * outside, nothing sent. */
int depz_device_get_sync_pin(depz_device *dev, uint8_t pin, depz_sync_pin_config *out);
int depz_device_set_sync_pin(depz_device *dev, const depz_sync_pin_config *c);
/* DEVICE_RESET: the device acknowledges, then reboots; the link drops. */
int depz_device_reset(depz_device *dev);
/* Reboot into the bootloader; the device is then closed for I/O (still call
 * depz_device_close() to free it). */
int depz_device_enter_bootloader(depz_device *dev);

/* Time sync (contract 02 §5) ------------------------------------------------ */

typedef struct {
    int64_t  offset_us;         /* device clock - host clock */
    int64_t  rtt_us;
    uint64_t synced_at_host_us;
} depz_time_sync;

/* Host monotonic clock, µs — the host side of all time-sync math. */
uint64_t depz_host_now_us(void);
/* NTP-style, `samples` round trips, keeps the lowest-RTT one. */
int  depz_device_sync_time(depz_device *dev, int samples, depz_time_sync *out);
bool depz_device_time_sync(const depz_device *dev, depz_time_sync *out);
/* Device µs -> host monotonic µs; DEPZ_E_ARG before the first sync. */
int  depz_device_to_host_time_us(const depz_device *dev, uint64_t device_us, int64_t *out);

/* Events (contract 07 §2) --------------------------------------------------- */

typedef enum {
    DEPZ_DEV_EV_SEQUENCE_ERROR,      /* expected_seq, received_seq, by_device */
    DEPZ_DEV_EV_SEQ_GAP,             /* expected_seq, received_seq, missed: a gap
                                        the host saw in the device->host seq
                                        (contract 01 §5), ahead of its packet */
    DEPZ_DEV_EV_CRC_ERROR,           /* cmd, seq */
    DEPZ_DEV_EV_TRASH,               /* data / data_len (first bytes), trash_len */
    DEPZ_DEV_EV_UNSOLICITED_STATUS,  /* status (ERR_HARDWARE_FAULT: a fault) */
    DEPZ_DEV_EV_TEXT,                /* cmd, text (unrouted reports: hex text) */
    DEPZ_DEV_EV_TEMPERATURE,         /* timestamp_us, celsius */
    DEPZ_DEV_EV_DISCONNECTED         /* text = reason */
} depz_device_event_type;

typedef struct {
    depz_device_event_type type;
    uint8_t  cmd, seq;
    uint8_t  expected_seq, received_seq;
    uint8_t  missed;   /* SEQ_GAP: (received_seq - expected_seq) mod 256 */
    bool     by_device;
    uint8_t  status;
    uint64_t timestamp_us;
    double   celsius;
    size_t   trash_len;
    uint8_t  data[64];
    size_t   data_len;
    char     text[256];
} depz_device_event;

typedef void (*depz_device_event_cb)(const depz_device_event *ev, void *user);

/* Subscribe; *token (optional) identifies the subscription for _off. */
int  depz_device_on_event(depz_device *dev, depz_device_event_cb cb, void *user, int *token);
void depz_device_off_event(depz_device *dev, int token);
/* Pull stream of depz_device_event items. NULL on allocation failure. */
depz_stream *depz_device_events(depz_device *dev, size_t maxsize);

/* ======================================================================== */
/* Discovery (contract 02 §4, contract 07 §3)                                */
/* ======================================================================== */

typedef struct {
    char port[256];
    int  vid, pid;        /* -1 when the OS does not know (not a USB port) */
    char usb_serial[128]; /* USB iSerial, "" when unknown */
} depz_port_info;

/* Every serial port the OS lists, with its USB identity where known. */
int  depz_list_serial_ports(depz_port_info **out, size_t *count);
void depz_free_port_list(depz_port_info *list);
/* The discovery order (contract 02 §4), in place: by USB serial ascending,
 * ports without a serial last, then by port path (depz_port_order_cmp). */
void depz_sort_ports_by_serial(depz_port_info *ports, size_t count);

typedef struct {
    char port[256];
    char mode[16];                /* "app" | "bootloader" | "unknown" */
    depz_sensor_type sensor_type; /* DEPZ_SENSOR_NONE in bootloader mode */
    char software_name[64];
    char fw_version[24];
    char device_name[128];
    char serial_number[64];       /* protocol serial (GET_SERIAL) */
    int  usb_vid, usb_pid;        /* -1 when unknown */
    char usb_serial[128];
} depz_device_info;

/* Open `port`, run the contract 02 §4.4 probe (GET_NAME_ACTIVE_SOFTWARE,
 * then GET_DEVICE_NAME and GET_SERIAL, each optional), close.
 * DEPZ_E_NO_DEVICE when nothing DEPZ-shaped answers. */
int depz_probe_port(const char *port, int timeout_ms, depz_device_info *out);

/* Probe the candidate ports — with `match_usb`, only those whose USB id is a
 * known DEPZ id — ordered by USB serial (unknown last). */
int  depz_list_depz_devices(bool match_usb, int timeout_ms,
                            depz_device_info **out, size_t *count);
void depz_free_device_list(depz_device_info *list);

typedef struct {
    const char *port;   /* open exactly this port (warns via depz_last_error()
                           text only; never refuses an unknown USB id) */
    const char *serial; /* else: the candidate with this USB serial */
    int index;          /* else: the Nth candidate by USB serial; -1 = first */
    int timeout_ms;     /* request timeout, <= 0 = default */
} depz_open_options;

#define DEPZ_OPEN_OPTIONS_INIT { NULL, NULL, -1, 0 }

/* Find, probe and open a DEPZ device with its sensor class attached.
 * `opt` NULL = the candidate with the smallest USB serial. Contract 02
 * §4.4: the probe runs on a connection of its own, which is closed; the
 * class (depz_resolve_sensor_class() of the answers and the port's USB id)
 * then opens the port afresh. */
int depz_open_device(const depz_open_options *opt, depz_device **out);

/* ======================================================================== */
/* SR04 ultrasonic ranger (contract 03)                                      */
/* ======================================================================== */

typedef struct {
    uint64_t timestamp_us; /* device µs */
    uint16_t echo_time_us; /* DEPZ_SR04_ECHO_TIMEOUT = no echo */
    bool     from_loop;    /* false: MEASURE_ONCE or a SYNC_IN edge */
} depz_sr04_measurement;

typedef void (*depz_sr04_measurement_cb)(const depz_sr04_measurement *m, void *user);

/* An SR04 on a link without an identity probe (tests, replay). */
int depz_sr04_open_link(depz_link *link, depz_device **out);
bool depz_is_sr04(const depz_device *dev);

bool depz_sr04_valid(const depz_sr04_measurement *m);
/* Distance at 343 m/s; false when there was no echo. */
bool depz_sr04_measurement_distance_mm(const depz_sr04_measurement *m, double *mm);
/* Temperature-compensated speed of sound (331.3 + 0.606·T m/s). */
bool depz_sr04_measurement_distance_mm_at(const depz_sr04_measurement *m, double air_temp_c,
                                          double *mm);

/* Stored minimum interval between measurement starts (default 50000). The
 * effective rate is also limited by the echo window (contract 03 §3). */
int depz_sr04_get_sample_period_us(depz_device *dev, uint32_t *out);
int depz_sr04_set_sample_period_us(depz_device *dev, uint32_t period_us);
int depz_sr04_get_echo_decay_us(depz_device *dev, uint16_t *out);
/* The device clamps to 4000..65000 µs; *effective (optional) is re-read.
 * DEPZ_E_ARG above 65535 (the u16 wire field). */
int depz_sr04_set_echo_decay_us(depz_device *dev, uint32_t decay_us, uint16_t *effective);

/* Single shot. DEPZ_E_BUSY while the loop runs. The reply comes when the
 * echo completes (or times out at ~65.5 ms): < 0 timeout means 1000 ms. */
int depz_sr04_measure_once(depz_device *dev, int timeout_ms, depz_sr04_measurement *out);
int depz_sr04_start(depz_device *dev); /* measurement loop; idempotent */
int depz_sr04_stop(depz_device *dev);

/* Loop samples and SYNC_IN single shots, as callbacks and/or streams. */
int  depz_sr04_on_measurement(depz_device *dev, depz_sr04_measurement_cb cb, void *user,
                              int *token);
void depz_sr04_off_measurement(depz_device *dev, int token);
/* Pull stream of depz_sr04_measurement items. NULL on wrong type / no memory. */
depz_stream *depz_sr04_stream(depz_device *dev, size_t maxsize);

/* ======================================================================== */
/* VL53L4CD single-zone ToF (contract 10)                                    */
/* ======================================================================== */

/* The board is a thin I2C register bridge; the ST ULD 2.2.3 (STSW-IMG026)
 * runs here on the host, ported register for register. init() before any
 * configuration; configuration is refused while ranging (the INT-driven
 * stream owns the register bank): DEPZ_E_ARG "stop ranging first". */

typedef struct {
    uint64_t timestamp_us;     /* MCU µs at the INT edge (stream) / the read (poll) */
    depz_vl53l4_result r;      /* range_status 0 = valid, distance_mm, ... */
} depz_vl53l4cd_measurement;

typedef void (*depz_vl53l4cd_measurement_cb)(const depz_vl53l4cd_measurement *m, void *user);

/* Detection-threshold windows (SYSTEM__INTERRUPT). */
#define DEPZ_VL53L4CD_WINDOW_BELOW 0u
#define DEPZ_VL53L4CD_WINDOW_ABOVE 1u
#define DEPZ_VL53L4CD_WINDOW_OUT   2u
#define DEPZ_VL53L4CD_WINDOW_IN    3u
/* init() runs its configuration block at 400 kHz and leaves the bus here. */
#define DEPZ_VL53L4CD_I2C_KHZ_DEFAULT 1000u

/* A VL53L4CD on a link without an identity probe (tests, replay). */
int  depz_vl53l4cd_open_link(depz_link *link, depz_device **out);
bool depz_is_vl53l4cd(const depz_device *dev);
/* UM2931 name of a range status ("valid", "sigma above threshold", ...). */
const char *depz_vl53l4cd_status_text(int range_status);

/* The sensor answers with its model id (0xEBAA). */
int  depz_vl53l4cd_is_alive(depz_device *dev, bool *alive);
/* Default configuration block + VHV calibration (ULD sensor_init), then the
 * bus at `bus_khz` (0 = DEPZ_VL53L4CD_I2C_KHZ_DEFAULT). Well under a second. */
int  depz_vl53l4cd_init(depz_device *dev, uint16_t bus_khz);
bool depz_vl53l4cd_initialized(const depz_device *dev);
bool depz_vl53l4cd_ranging(const depz_device *dev);

/* XSHUT pin: DEPZ_VL53L4_XSHUT_OFF / _ON / _RESET. OFF and RESET stop any
 * stream; a power-cycled sensor needs init() again. */
int depz_vl53l4cd_xshut(depz_device *dev, uint8_t action);
int depz_vl53l4cd_reset_sensor(depz_device *dev); /* = xshut(RESET) */
/* Bridge identity, pin levels and counters; safe while streaming. */
int depz_vl53l4cd_bridge_info(depz_device *dev, depz_vl53l4_info *out);
/* Re-time the bus to the nominal step nearest `khz`. Not while ranging. */
int depz_vl53l4cd_set_i2c_speed_khz(depz_device *dev, uint16_t khz);

/* Configuration — init() first, never while ranging. */
/* Budget 10..200 ms; inter 0 = continuous, > budget = autonomous low power. */
int depz_vl53l4cd_set_range_timing(depz_device *dev, uint32_t budget_ms, uint32_t inter_ms);
int depz_vl53l4cd_get_range_timing(depz_device *dev, uint32_t *budget_ms, uint32_t *inter_ms);
/* Offset -1024..1023 mm, xtalk 0..127 kcps (0 = off): DEPZ_E_ARG outside,
 * nothing written. */
int depz_vl53l4cd_set_offset_mm(depz_device *dev, int32_t offset_mm);
int depz_vl53l4cd_get_offset_mm(depz_device *dev, int32_t *offset_mm);
int depz_vl53l4cd_set_xtalk_kcps(depz_device *dev, uint16_t xtalk_kcps);
int depz_vl53l4cd_get_xtalk_kcps(depz_device *dev, uint16_t *xtalk_kcps);
/* INT fires only when the window condition holds (DEPZ_VL53L4CD_WINDOW_*). */
int depz_vl53l4cd_set_detection_thresholds(depz_device *dev, uint16_t low_mm, uint16_t high_mm,
                                           uint8_t window);
int depz_vl53l4cd_get_detection_thresholds(depz_device *dev, uint16_t *low_mm, uint16_t *high_mm,
                                           uint8_t *window);
int depz_vl53l4cd_set_signal_threshold_kcps(depz_device *dev, uint16_t kcps);
int depz_vl53l4cd_get_signal_threshold_kcps(depz_device *dev, uint16_t *kcps);
int depz_vl53l4cd_set_sigma_threshold_mm(depz_device *dev, uint16_t mm); /* <= 16383 */
int depz_vl53l4cd_get_sigma_threshold_mm(depz_device *dev, uint16_t *mm);
/* Re-run VHV calibration; after a > 8 °C ambient change. */
int depz_vl53l4cd_start_temperature_update(depz_device *dev);
/* Against a target at `target_mm` (10..1000); nb_samples 5..255 (0 = 20).
 * Blocks for the bursts; *offset_mm (optional) = the offset now programmed. */
int depz_vl53l4cd_calibrate_offset(depz_device *dev, uint16_t target_mm, uint8_t nb_samples,
                                   int32_t *offset_mm);
/* Against a target at `target_mm` (10..5000); *xtalk_kcps = programmed value. */
int depz_vl53l4cd_calibrate_xtalk(depz_device *dev, uint16_t target_mm, uint8_t nb_samples,
                                  uint16_t *xtalk_kcps);

/* Ranging. */
/* Start the sensor and arm the MCU stream: one measurement per INT edge. */
int  depz_vl53l4cd_start_ranging(depz_device *dev);
/* Idempotent; the sensor is stopped even when the stream stop fails. */
int  depz_vl53l4cd_stop_ranging(depz_device *dev);
/* Poll mode: start, wait data-ready, read, stop (< 0 timeout = 1000 ms).
 * Refused while the stream runs. */
int  depz_vl53l4cd_measure_once(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out);
int  depz_vl53l4cd_on_measurement(depz_device *dev, depz_vl53l4cd_measurement_cb cb, void *user,
                                  int *token);
void depz_vl53l4cd_off_measurement(depz_device *dev, int token);
/* Pull stream of depz_vl53l4cd_measurement items. */
depz_stream *depz_vl53l4cd_stream(depz_device *dev, size_t maxsize);
/* The next streamed measurement (< 0 timeout = 2000 ms). */
int  depz_vl53l4cd_get_measurement(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out);
/* Stream reports dropped because their block did not decode. */
uint64_t depz_vl53l4cd_stream_parse_errors(const depz_device *dev);

/* Raw register access (escape hatch): 16-bit address, contents as the
 * sensor has them (big-endian words). Split at the bridge's 253-byte limit. */
int depz_vl53l4cd_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
int depz_vl53l4cd_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);

/* ======================================================================== */
/* Multizone ToF: VL53L8CX / VL53L8CH (contract 04),                         */
/* VL53L5CX / VL53L7CX / VL53L7CH (contract 11)                               */
/* ======================================================================== */

/* The board is a thin SPI register bridge; the ST ULD runs on the host,
 * ported register for register from the Python SDK's vl53l8/uld.py. init()
 * downloads the ~84 KB sensor firmware (0.8 s over SPI, 1.3 s over the L5/L7
 * I2C bridge), then configure and
 * start_ranging(). Configuration requires init() and is refused while ranging
 * (DEPZ_E_ARG "stop ranging first"). One implementation serves the whole
 * multizone family; the model fixes the sensor-firmware blob. */

/* The board model: fixes the sensor-firmware blob and the bridge. */
typedef enum {
    DEPZ_VL53L8_MODEL_L8CX = 0, /* VL53L8CX ULD 2.1.0 blob, SPI bridge        */
    DEPZ_VL53L8_MODEL_L8CH = 1, /* VL53LMZ ULD 2.0.16 blob, adds CNH output   */
    DEPZ_VL53L8_MODEL_L5CX = 2, /* I2C bridge (APP_VL53L7), ULD 2.0.1 blob     */
    DEPZ_VL53L8_MODEL_L7CX = 3, /* same blob and API as L5CX, 90° optics       */
    DEPZ_VL53L8_MODEL_L7CH = 4  /* VL53LMZ 2.0.16 blob (as L8CH), adds CNH     */
} depz_vl53l8_model;

#define DEPZ_VL53L8_RANGING_MODE_CONTINUOUS 1u
#define DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS 3u
#define DEPZ_VL53L8_TARGET_ORDER_CLOSEST    1u
#define DEPZ_VL53L8_TARGET_ORDER_STRONGEST  2u
#define DEPZ_VL53L8_XTALK_BUFFER_SIZE       776u
#define DEPZ_VL53L8_CNH_MAX_BYTES           6160u  /* on-device CNH buffer cap */
/* Detection-threshold windows and combine operations (plugin constants). */
#define DEPZ_VL53L8_THRESH_IN_WINDOW           0u
#define DEPZ_VL53L8_THRESH_OUT_OF_WINDOW       1u
#define DEPZ_VL53L8_THRESH_LESS_THAN_EQUAL_MIN 2u
#define DEPZ_VL53L8_THRESH_GREATER_THAN_MAX    3u
#define DEPZ_VL53L8_THRESH_EQUAL_MIN           4u
#define DEPZ_VL53L8_THRESH_NOT_EQUAL_MIN       5u
#define DEPZ_VL53L8_THRESH_OP_OR               0u
#define DEPZ_VL53L8_THRESH_OP_AND              2u
/* OR into the zone_num of the last threshold programmed (ST LAST_THRESHOLD). */
#define DEPZ_VL53L8_THRESH_LAST                128u

/* The motion-indicator output of a frame (configure_motion_indicator). */
typedef struct {
    uint32_t global_indicator_1, global_indicator_2;
    uint8_t  status, nb_of_detected_aggregates, nb_of_aggregates;
    uint32_t motion[32];
} depz_vl53l8_motion;

/* One streamed frame. `f` holds the zone arrays (distance_mm in mm, sigma
 * raw/128, ...; `f.resolution` zones, row-major). CH with CNH armed: the raw
 * CNH block in cnh[0..cnh_len) — decode it with depz_vl53l8ch_decode_cnh(). */
typedef struct {
    depz_vl53l8_frame  f;
    bool               has_motion;
    depz_vl53l8_motion motion;
    size_t             cnh_len;
    uint8_t            cnh[DEPZ_VL53L8_CNH_MAX_BYTES];
} depz_vl53l8_live_frame;

typedef void (*depz_vl53l8_frame_cb)(const depz_vl53l8_live_frame *f, void *user);
/* init() progress: a phase text, and for the big blob writes done/total bytes
 * (both 0 otherwise). Runs on the calling thread. */
typedef void (*depz_vl53l8_progress_cb)(const char *phase, size_t done, size_t total, void *user);

/* CNH (compact network histograms, CH only): VL53LMZ_Motion_Configuration and
 * the plugin helpers that fill it (vl53lmz_plugin_cnh.c). */
typedef struct {
    int32_t  ref_bin_offset;
    uint32_t detection_threshold, extra_noise_sigma, null_den_clip_value;
    uint8_t  mem_update_mode, mem_update_choice, sum_span, feature_length;
    uint8_t  nb_of_aggregates, nb_of_temporal_accumulations, min_nb_for_global_detection;
    uint8_t  global_indicator_format_1, global_indicator_format_2;
    uint8_t  cnh_cfg, cnh_flex_shift, spare_3;
    int8_t   map_id[64];
    uint8_t  indicator_format_1[32], indicator_format_2[32];
} depz_vl53l8_cnh_setup;

/* vl53lmz_cnh_init_config: histogram start bin, CNH bins, device bins per CNH
 * bin. num_bins / sub_sample are u8 fields: this form masks them to 8 bits;
 * the _checked form (preferred, contract 07 §3) answers DEPZ_E_ARG outside
 * 0..255 and leaves `s` untouched. */
void depz_vl53l8_cnh_init_config(depz_vl53l8_cnh_setup *s, int start_bin, int num_bins,
                                 int sub_sample);
int  depz_vl53l8_cnh_init_config_checked(depz_vl53l8_cnh_setup *s, int start_bin, int num_bins,
                                         int sub_sample);
/* vl53lmz_cnh_create_agg_map: map zones (resolution 16 | 64) to aggregates.
 * DEPZ_E_ARG for another resolution, start_x / start_y < 0, merge_x /
 * merge_y / cols / rows < 1, or a map past the zone grid. */
int  depz_vl53l8_cnh_create_agg_map(depz_vl53l8_cnh_setup *s, int resolution, int start_x,
                                    int start_y, int merge_x, int merge_y, int cols, int rows);
/* On-device CNH buffer bytes; DEPZ_E_ARG when blank or above the cap. */
int  depz_vl53l8_cnh_required_memory(const depz_vl53l8_cnh_setup *s, size_t *bytes);
/* The 156-byte struct cnh_send_config writes. */
void depz_vl53l8_cnh_pack(const depz_vl53l8_cnh_setup *s, uint8_t out[156]);
/* The decode parameters of a setup, for depz_vl53l8ch_decode_cnh(). */
void depz_vl53l8_cnh_decode_config(const depz_vl53l8_cnh_setup *s, depz_vl53l8ch_cnh_config *out);

/* A multizone ToF on a link without an identity probe (tests, replay). */
int  depz_vl53l8_open_link(depz_link *link, depz_vl53l8_model model, depz_device **out);
bool depz_is_vl53l8(const depz_device *dev);
depz_vl53l8_model depz_vl53l8_get_model(const depz_device *dev);

/* Probe: device id / revision (0xF0 / 0x0C on VL53L8, revision 0x02 on L5/L7).
 * It and get_power_mode switch the register bank: not while ranging. */
int  depz_vl53l8_is_alive(depz_device *dev, bool *alive, uint8_t *device_id, uint8_t *revision_id);
/* Boot the sensor MCU, download its firmware, upload NVM offset data, the
 * default xtalk and configuration. `progress` may be NULL. */
int  depz_vl53l8_init(depz_device *dev, depz_vl53l8_progress_cb progress, void *user);
bool depz_vl53l8_initialized(const depz_device *dev);
bool depz_vl53l8_ranging(const depz_device *dev);

/* Configuration — init() first, never while ranging. */
int depz_vl53l8_get_resolution(depz_device *dev, int *zones);        /* 16 | 64 */
int depz_vl53l8_set_resolution(depz_device *dev, int zones);
/* >= 2 Hz on VL53L8 (below that it never ranges), >= 1 Hz on L5/L7; at most
 * depz_vl53l8_max_ranging_frequency_hz() of the resolution last set or read
 * through this API (16 after open) — 60 Hz at 4x4, 15 Hz at 8x8 (contract 11
 * §3). Refused with DEPZ_E_ARG before anything reaches the bus. */
int depz_vl53l8_max_ranging_frequency_hz(int resolution);
int depz_vl53l8_get_ranging_frequency_hz(depz_device *dev, uint8_t *hz);
int depz_vl53l8_set_ranging_frequency_hz(depz_device *dev, uint8_t hz);
int depz_vl53l8_get_ranging_mode(depz_device *dev, uint8_t *mode);
int depz_vl53l8_set_ranging_mode(depz_device *dev, uint8_t mode);
/* 2..1000 ms; autonomous mode only. */
int depz_vl53l8_get_integration_time_ms(depz_device *dev, uint32_t *ms);
int depz_vl53l8_set_integration_time_ms(depz_device *dev, uint32_t ms);
/* 0..99 % (0 = off); read back rounded to nearest, like the Python SDK. */
int depz_vl53l8_get_sharpener_percent(depz_device *dev, uint8_t *pct);
int depz_vl53l8_set_sharpener_percent(depz_device *dev, uint8_t pct);
int depz_vl53l8_get_target_order(depz_device *dev, uint8_t *order);
int depz_vl53l8_set_target_order(depz_device *dev, uint8_t order);
/* DEPZ_VL53L8_POWER_MODE_*; waking from deep sleep re-runs init(). */
int depz_vl53l8_get_power_mode(depz_device *dev, uint8_t *mode);
int depz_vl53l8_set_power_mode(depz_device *dev, uint8_t mode);
/* Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); above is
 * refused, nothing written. get: raw / 2048 rounded down. */
int depz_vl53l8_get_xtalk_margin(depz_device *dev, uint32_t *kcps_per_spad);
int depz_vl53l8_set_xtalk_margin(depz_device *dev, uint32_t kcps_per_spad);
/* Crosstalk calibration against a flat target (reflectance 1..99 %,
 * nb_samples 1..16, distance 600..3000 mm); blocks several seconds. With no
 * cover glass the firmware answers "nothing to calibrate": *failed (optional)
 * is then true and the default xtalk data stays. */
int depz_vl53l8_calibrate_xtalk(depz_device *dev, uint8_t reflectance_percent, uint8_t nb_samples,
                                uint16_t distance_mm, bool *failed);
/* The 776-byte xtalk calibration blob (save / restore). */
int depz_vl53l8_get_caldata_xtalk(depz_device *dev, uint8_t out[776]);
int depz_vl53l8_set_caldata_xtalk(depz_device *dev, const uint8_t blob[776]);
int depz_vl53l8_get_detection_thresholds_enable(depz_device *dev, bool *enabled);
int depz_vl53l8_set_detection_thresholds_enable(depz_device *dev, bool enabled);
/* All 64 thresholds, low/high in real units of their measurement. */
int depz_vl53l8_get_detection_thresholds(depz_device *dev, depz_vl53l8_threshold out[64]);
int depz_vl53l8_set_detection_thresholds(depz_device *dev, const depz_vl53l8_threshold *th,
                                         size_t n);
int depz_vl53l8_set_detection_thresholds_auto_stop(depz_device *dev, bool auto_stop);
/* Motion indicator over [min, max] mm (400..4000, span <= 1500); frames then
 * carry `motion`. It shares DCI 0xBFAC with CNH: this disarms CNH. */
int depz_vl53l8_configure_motion_indicator(depz_device *dev, uint16_t min_mm, uint16_t max_mm);
/* CH only: arm the CNH block for the next start_ranging(); this disarms the
 * motion indicator (one DCI block: frames carry no `motion`). init() disarms
 * both (the fresh sensor holds neither configuration). */
int depz_vl53l8_configure_cnh(depz_device *dev, const depz_vl53l8_cnh_setup *setup);
/* The CNH setup armed by configure_cnh() (copied to *setup, which may be
 * NULL); false when none is armed. */
bool depz_vl53l8_cnh_armed(const depz_device *dev, depz_vl53l8_cnh_setup *setup);

/* Ranging. */
int  depz_vl53l8_start_ranging(depz_device *dev);
int  depz_vl53l8_stop_ranging(depz_device *dev); /* idempotent */
int  depz_vl53l8_on_frame(depz_device *dev, depz_vl53l8_frame_cb cb, void *user, int *token);
void depz_vl53l8_off_frame(depz_device *dev, int token);
/* Pull stream of depz_vl53l8_live_frame items (~7.5 KB each: keep maxsize
 * small; 0 = 8). */
depz_stream *depz_vl53l8_frames(depz_device *dev, size_t maxsize);
/* The next frame (< 0 timeout = 2000 ms). */
int  depz_vl53l8_get_frame(depz_device *dev, int timeout_ms, depz_vl53l8_live_frame *out);
/* Frames dropped because they did not parse / chunked frames the reassembler
 * discarded (gaps). */
uint64_t depz_vl53l8_frame_parse_errors(const depz_device *dev);
uint64_t depz_vl53l8_reassembler_discards(const depz_device *dev);

/* L5/L7 only (DEPZ_E_WRONG_TYPE on VL53L8): the I2C bridge's own commands. */
/* Sensor module type read at init(): 0 MZ = VL53L5CX, 1 MZEVO = VL53L7CX/CH,
 * -1 before init. L5 and L7 share a blob and a board: this is how the silicon
 * tells them apart (never CX from CH). */
int depz_vl53l8_module_type(const depz_device *dev);
/* Bridge counters and pin levels; read before / after a run, not during one
 * (each call takes the bus from the stream and can cost a frame). */
int depz_vl53l7_bridge_info(depz_device *dev, depz_vl53l7_info *out);
/* SCL snaps to 100, 200, 400, 500 ... 1000 kHz; *effective (optional) = the
 * value now in effect. DEPZ_E_BUSY mid-transfer — stop ranging first. */
int depz_vl53l7_set_i2c_speed_khz(depz_device *dev, uint16_t khz, uint16_t *effective);
/* DEPZ_VL53L7_PIN_*: LPN_OFF and SOFT_CYCLE drop the sensor's state — run
 * init() again (firmware download included). */
int depz_vl53l7_pin_ctrl(depz_device *dev, uint8_t action);

/* Escape hatches: raw registers (16-bit address) and DCI indices. */
int depz_vl53l8_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
int depz_vl53l8_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);
int depz_vl53l8_dci_read(depz_device *dev, uint16_t index, uint8_t *buf, size_t len);
int depz_vl53l8_dci_write(depz_device *dev, uint16_t index, const uint8_t *data, size_t len);

/* ======================================================================== */
/* BNO055 9-axis IMU (contract 13)                                           */
/* ======================================================================== */

/* A thin I2C register bridge; the BNO055 fuses on chip, so there is no driver
 * to port — mode, units, axis remap, calibration and decoding are register
 * access, done here. Typical use: configure() (CONFIG -> units -> remap ->
 * optional calibration profile -> NDOF), then start_stream(10 ms) and read
 * samples, or poll read_sample(). Page 1 (sensor configs, interrupts, unique
 * id) is refused while streaming — the bridge would read the wrong page. */

#define DEPZ_BNO055_POWER_NORMAL  0u
#define DEPZ_BNO055_POWER_LOW     1u
#define DEPZ_BNO055_POWER_SUSPEND 2u
#define DEPZ_BNO055_TEMP_FROM_ACCEL 0u
#define DEPZ_BNO055_TEMP_FROM_GYRO  1u

/* Which channels a sample carries (the block covered them whole). */
#define DEPZ_BNO055_HAS_ACCEL        0x001u
#define DEPZ_BNO055_HAS_MAG          0x002u
#define DEPZ_BNO055_HAS_GYRO         0x004u
#define DEPZ_BNO055_HAS_EULER        0x008u
#define DEPZ_BNO055_HAS_QUATERNION   0x010u
#define DEPZ_BNO055_HAS_LINEAR_ACCEL 0x020u
#define DEPZ_BNO055_HAS_GRAVITY      0x040u
#define DEPZ_BNO055_HAS_TEMPERATURE  0x080u
#define DEPZ_BNO055_HAS_CALIBRATION  0x100u

/* One decoded register block, scaled by the units in effect when it was read
 * (streams: the units at start_stream). Fusion outputs read zero outside the
 * fusion modes; linear_accel / gravity are always m/s^2. */
typedef struct {
    uint64_t timestamp_us;      /* MCU µs: trigger (stream) / read completion */
    uint8_t  addr, len;
    uint8_t  raw[128];
    depz_bno055_units units;
    uint32_t present;           /* DEPZ_BNO055_HAS_* */
    double   accel[3];          /* m/s^2 or mg */
    double   mag[3];            /* µT */
    double   gyro[3];           /* dps or rps */
    double   euler[3];          /* heading, roll, pitch: degrees or radians */
    double   quaternion[4];     /* w, x, y, z, unit */
    double   linear_accel[3];
    double   gravity[3];
    double   temperature;       /* °C or °F */
    depz_bno055_calib_status calibration;
} depz_bno055_sample;

/* ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR. */
typedef struct { uint8_t self_test, clk_status, status, error; } depz_bno055_status_regs;

typedef void (*depz_bno055_sample_cb)(const depz_bno055_sample *s, void *user);

/* Decode and scale a register block (pure). */
void depz_bno055_decode_sample(uint64_t timestamp_us, uint8_t addr, const uint8_t *data, size_t len,
                               const depz_bno055_units *units, depz_bno055_sample *out);

/* A BNO055 on a link without an identity probe (tests, replay). */
int  depz_bno055_open_link(depz_link *link, depz_device **out);
bool depz_is_bno055(const depz_device *dev);

/* Chip ids, sensor firmware revision, bridge counters; safe while streaming. */
int depz_bno055_bridge_info(depz_device *dev, depz_bno055_info *out);
/* The bridge passed its chip-id handshake and the ids are the BNO055's. */
int depz_bno055_is_alive(depz_device *dev, bool *alive);
/* nRESET (~0.5 s handshake): stops any stream; CONFIG mode, power-on units,
 * no calibration — configure() again or restore_configuration(). */
int depz_bno055_reset_sensor(depz_device *dev);

/* Raw registers on page 0 or 1 (page 1 refused while streaming). Most
 * configuration registers only take writes in CONFIG mode. */
int depz_bno055_read_registers(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len, int page);
int depz_bno055_write_registers(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len, int page);

/* Mode switches go through CONFIG (the sensor ignores mode -> mode) and wait
 * the datasheet times; into a fusion mode they wait for the fusion to run. */
int depz_bno055_get_operation_mode(depz_device *dev, uint8_t *mode);
int depz_bno055_set_operation_mode(depz_device *dev, uint8_t mode);
int depz_bno055_get_power_mode(depz_device *dev, uint8_t *mode);
int depz_bno055_set_power_mode(depz_device *dev, uint8_t mode);
int depz_bno055_get_units(depz_device *dev, depz_bno055_units *out);
int depz_bno055_set_units(depz_device *dev, const depz_bno055_units *units);
int depz_bno055_get_axis_remap(depz_device *dev, depz_bno055_axis_remap *out);
int depz_bno055_set_axis_remap(depz_device *dev, const depz_bno055_axis_remap *remap);
/* A datasheet placement "P0".."P7" (P1 = default). */
int depz_bno055_set_axis_placement(depz_device *dev, const char *placement);
int depz_bno055_get_temperature_source(depz_device *dev, uint8_t *source);
int depz_bno055_set_temperature_source(depz_device *dev, uint8_t source);

/* The usual session setup: CONFIG -> units (NULL = defaults) -> axis remap
 * (NULL = leave) -> calibration profile (NULL = leave) -> `mode`; in a fusion
 * mode it returns once the fusion outputs are live. Remembered for
 * restore_configuration() (after a reset, or when sensor_resets rose). */
int depz_bno055_configure(depz_device *dev, uint8_t mode, const depz_bno055_units *units,
                          const depz_bno055_axis_remap *remap, const depz_bno055_calib_profile *calibration);
int depz_bno055_restore_configuration(depz_device *dev);

/* Status, self-test, calibration. */
int depz_bno055_system_status(depz_device *dev, depz_bno055_status_regs *out);
/* ~0.45 s in CONFIG mode, the operating mode restored. Not while streaming. */
int depz_bno055_self_test(depz_device *dev, depz_bno055_status_regs *out);
int depz_bno055_calibration_status(depz_device *dev, depz_bno055_calib_status *out);
/* Offsets and radii; the sensor exposes them in CONFIG mode only (the driver
 * goes there and back). A written profile is a starting point: fusion refines
 * it at once. */
int depz_bno055_read_calibration_profile(depz_device *dev, depz_bno055_calib_profile *out);
int depz_bno055_write_calibration_profile(depz_device *dev, const depz_bno055_calib_profile *p);
/* Soft-iron matrix, 9 x i16 row-major, 1.0 = 16384. */
int depz_bno055_get_sic_matrix(depz_device *dev, int16_t out[9]);
int depz_bno055_set_sic_matrix(depz_device *dev, const int16_t m[9]);

/* Page 1: raw sensor configuration (non-fusion modes only; fusion overrides
 * it), unique id, motion interrupts (the only interrupts on SW rev 03.11). */
int depz_bno055_get_accel_config(depz_device *dev, depz_bno055_accel_config *out);
int depz_bno055_set_accel_config(depz_device *dev, const depz_bno055_accel_config *c);
int depz_bno055_get_gyro_config(depz_device *dev, depz_bno055_gyro_config *out);
int depz_bno055_set_gyro_config(depz_device *dev, const depz_bno055_gyro_config *c);
int depz_bno055_get_mag_config(depz_device *dev, depz_bno055_mag_config *out);
int depz_bno055_set_mag_config(depz_device *dev, const depz_bno055_mag_config *c);
int depz_bno055_unique_id(depz_device *dev, uint8_t out[16]);
int depz_bno055_get_interrupt_enable(depz_device *dev, uint8_t *mask);
int depz_bno055_set_interrupt_enable(depz_device *dev, uint8_t mask);
int depz_bno055_get_interrupt_mask(depz_device *dev, uint8_t *mask);
int depz_bno055_set_interrupt_mask(depz_device *dev, uint8_t mask);
/* One page-1 motion-interrupt setting (0x11..0x1F), raw. */
int depz_bno055_set_interrupt_setting(depz_device *dev, uint8_t reg, uint8_t value);
/* INT_STA — clears on read. */
int depz_bno055_read_interrupt_status(depz_device *dev, uint8_t *status);
/* SYS_TRIGGER RST_INT: reset the status bits and the INT pin. */
int depz_bno055_clear_interrupt(depz_device *dev);

/* Data. */
/* Poll a block (addr, len = DEPZ_BNO055_FULL_BLOCK_* by default); works
 * alongside a stream. */
int depz_bno055_read_sample(depz_device *dev, uint8_t addr, uint8_t len, depz_bno055_sample *out);
int depz_bno055_read_quaternion(depz_device *dev, double q[4]); /* w, x, y, z */
/* Read `len` bytes at `addr` every `period_ms` (TIMER; fusion runs at 100 Hz,
 * so 10 ms is the useful floor) — or on each INT edge (motion interrupts only
 * on SW 03.11) with `period_ms` as a watchdog, 0 = none. Replaces a stream. */
int  depz_bno055_start_stream(depz_device *dev, uint16_t period_ms, uint8_t addr, uint8_t len,
                              uint8_t trigger);
int  depz_bno055_stop_stream(depz_device *dev);
bool depz_bno055_streaming(const depz_device *dev);
int  depz_bno055_on_sample(depz_device *dev, depz_bno055_sample_cb cb, void *user, int *token);
void depz_bno055_off_sample(depz_device *dev, int token);
depz_stream *depz_bno055_samples(depz_device *dev, size_t maxsize);
/* The next streamed sample (< 0 timeout = 1000 ms). */
int  depz_bno055_get_sample(depz_device *dev, int timeout_ms, depz_bno055_sample *out);
uint64_t depz_bno055_stream_parse_errors(const depz_device *dev);

/* ======================================================================== */
/* TMF8829 multizone dToF (contract 14)                                      */
/* ======================================================================== */

/* 8x8 ... 48x32 zones, up to 4 peaks per zone, raw histograms, over a thin
 * SPI register bridge: the host runs the sensor, as the Python SDK's
 * tmf8829/__init__.py does it (same request sequences, so a capture made by
 * either SDK replays in the other). Typical use: init() (soft reset +
 * firmware download, ~0.15 s), configure("32x32", NULL), start(), then read
 * measurements; stop(). The sensor's EN pin is wired high: there is no sleep,
 * and recovery from any state is init() (it starts with a soft reset). A
 * DEPZ_TMF8829_EV_BAD_HEADER event outside stop() means the sensor is lost:
 * init() again. Sensor refusals (bootloader / application status, a field
 * that does not read back) fail with DEPZ_E_PROTOCOL, polls with
 * DEPZ_E_TIMEOUT. */

/* What configure() writes on top of the preset. Every field a preset does not
 * reset is written each time (contract 14 §4), so the result never depends on
 * an earlier config. */
typedef struct {
    const char *name;  /* a DEPZ_TMF8829_CFG_FIELDS name */
    int64_t     value;
} depz_tmf8829_field_value;

typedef struct {
    int  period_ms;          /* DEPZ_TMF8829_DEFAULT_PERIOD_MS */
    int  kilo_iterations;    /* < 0 = the preset's own */
    int  nr_peaks;           /* 1..4 */
    bool signal_strength, noise_strength, xtalk;
    bool histograms;         /* dump_histograms: + HISTOGRAMS frames */
    bool ref_frame;          /* ref_spad_frame: + the ref-SPAD frame per results frame */
    /* any other page field by name (also overrides the ones above) */
    const depz_tmf8829_field_value *fields;
    size_t n_fields;
} depz_tmf8829_options;

#define DEPZ_TMF8829_OPTIONS_INIT \
    { DEPZ_TMF8829_DEFAULT_PERIOD_MS, -1, 1, false, false, false, false, false, NULL, 0 }

/* The config written by configure(), as read back from the sensor. */
typedef struct {
    char     preset[16];
    uint8_t  page[DEPZ_TMF8829_CFG_SIZE];   /* 0x22..0xDF */
    bool     histograms, ref_frame;
    uint32_t fields[DEPZ_TMF8829_CFG_FIELD_COUNT]; /* in DEPZ_TMF8829_CFG_FIELDS order */
    unsigned rows, cols;                    /* grid of a measurement */
} depz_tmf8829_config;

/* Counters of the current stream (reset at start()). */
typedef struct {
    uint64_t frames_completed;    /* frames reassembled */
    uint64_t frames_dropped;      /* frame_seq gaps: frames the bridge dropped */
    uint64_t frames_discarded;    /* lost to reassembly (incomplete, out of order) */
    uint64_t frame_parse_errors;  /* frames that did not parse / decode */
    uint64_t chunk_errors;        /* RPT_TMF_FRAME / EVENT reports too short */
    uint64_t measurements;
    uint64_t events_swallowed;    /* the expected BAD_HEADER of stop() */
} depz_tmf8829_stream_stats;

/* init() progress over the image download: bytes done / total. */
typedef void (*depz_tmf8829_progress_cb)(size_t done, size_t total, void *user);
typedef void (*depz_tmf8829_measurement_cb)(const depz_tmf8829_measurement *m, void *user);
/* Every reassembled frame, before the measurement it may complete: the way to
 * the histogram (25 377 B) and ref-SPAD (65 B) bytes, which a measurement only
 * counts. `f->data` is valid during the call. */
typedef void (*depz_tmf8829_frame_cb)(const depz_tmf8829_raw_frame *f, void *user);
/* RPT_TMF_EVENT other than the expected stop() artefact. */
typedef void (*depz_tmf8829_event_cb)(const depz_tmf8829_event *ev, void *user);

/* A TMF8829 on a link without an identity probe (tests, replay). */
int  depz_tmf8829_open_link(depz_link *link, depz_device **out);
bool depz_is_tmf8829(const depz_device *dev);

/* Soft reset -> sensor bootloader -> firmware download -> RAM application.
 * Stops a running stream first; forgets the config. `progress` may be NULL;
 * `app_version` ("2.200.0", cap >= 12) and `serial` may be NULL. */
int depz_tmf8829_init(depz_device *dev, depz_tmf8829_progress_cb progress, void *user,
                      char *app_version, size_t cap, uint32_t *serial);
/* What the last init() read; DEPZ_E_ARG before one. */
int depz_tmf8829_sensor_id(depz_device *dev, char *app_version, size_t cap, uint32_t *serial);

/* Load `preset` (a DEPZ_TMF8829_PRESETS name) and write the config page with
 * `opt` (NULL = DEPZ_TMF8829_OPTIONS_INIT); every written field must read
 * back. `out` may be NULL. Not while streaming. */
int depz_tmf8829_configure(depz_device *dev, const char *preset, const depz_tmf8829_options *opt,
                           depz_tmf8829_config *out);
/* The last configure() result; DEPZ_E_ARG when none since init(). */
int depz_tmf8829_get_config(depz_device *dev, depz_tmf8829_config *out);
/* LOAD_CONFIG_PAGE -> the 190-byte page 0x22..0xDF now in the sensor. */
int depz_tmf8829_config_page(depz_device *dev, uint8_t page[DEPZ_TMF8829_CFG_SIZE]);

/* Raw registers in <= 1024 / 1023-byte requests. A read that reaches the FIFO
 * port 0xFF stays there (the next piece reads at min(addr + len, 0xFF)); a
 * write to 0xFF stays at 0xFF. */
int depz_tmf8829_read_registers(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len);
int depz_tmf8829_write_registers(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len);

/* Bridge counters (never touches the sensor, safe mid-stream). */
int depz_tmf8829_bridge_info(depz_device *dev, depz_tmf8829_info *out);

/* MEASURE with the bridge stream armed (RESULTS, + HISTOGRAMS / the ref frame
 * as configured). Configures the default "8x8" first when configure() was not
 * called since init(). No-op while streaming. */
int  depz_tmf8829_start(depz_device *dev);
/* Sensor STOP, then the bridge stream (frames already queued drain); the
 * bad-header event a STOP raises mid histogram dump is swallowed. */
int  depz_tmf8829_stop(depz_device *dev);
bool depz_tmf8829_streaming(const depz_device *dev);

int  depz_tmf8829_on_measurement(depz_device *dev, depz_tmf8829_measurement_cb cb, void *user, int *token);
void depz_tmf8829_off_measurement(depz_device *dev, int token);
int  depz_tmf8829_on_frame(depz_device *dev, depz_tmf8829_frame_cb cb, void *user, int *token);
void depz_tmf8829_off_frame(depz_device *dev, int token);
int  depz_tmf8829_on_event(depz_device *dev, depz_tmf8829_event_cb cb, void *user, int *token);
void depz_tmf8829_off_event(depz_device *dev, int token);
/* Pull stream of depz_tmf8829_measurement items (~90 KB each; maxsize 0 =
 * 16). Subscribe before start() to see the first one. */
depz_stream *depz_tmf8829_measurements(depz_device *dev, size_t maxsize);
/* The next measurement (< 0 timeout = 1000 ms). */
int  depz_tmf8829_get_measurement(depz_device *dev, int timeout_ms, depz_tmf8829_measurement *out);
void depz_tmf8829_get_stream_stats(const depz_device *dev, depz_tmf8829_stream_stats *out);

/* ======================================================================== */
/* VL53L 1D ToF family: VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CX     */
/* (and VL53L4CD) on the APP_VL53L0_4 bridge (contract 12)                    */
/* ======================================================================== */

/* The board is a thin I2C register bridge that knows no sensor; the ST
 * drivers run here, ported register for register from the Python SDK's
 * vl53lx/uld/ — same requests in the same order, so a capture made by either
 * SDK replays in the other. Two choices pick how a board is driven:
 *   - the product, whose parameter set is loaded — normally the one the
 *     board's device name carries; naming a sibling borrows its driver
 *     (a VL53L4CX run as VL53L4CD gets the light ULD with its calibrations);
 *   - the driver kind: DEPZ_VL53LX_DRIVER_ULD (Ultra Lite), _ULP (Ultra Low
 *     Power, VL53L3CX) or _HISTOGRAM (ST's Bare Driver: the die hands over 24
 *     photon bins and the host finds up to four targets).
 * init() binds the pair; configure() re-initialises and applies budget and
 * mode — call it before every run; then start_ranging() and read
 * measurements. Configuration is refused while ranging (DEPZ_E_ARG "stop
 * ranging first"), and an optional capability a product / driver does not
 * have is refused with DEPZ_E_ARG naming it (ask depz_vl53lx_supports()). */

/* Optional capability groups (depz_vl53lx_supports). */
#define DEPZ_VL53LX_CAP_MODE          0x0001u /* named ranging modes          */
#define DEPZ_VL53LX_CAP_TIMING        0x0002u
#define DEPZ_VL53LX_CAP_OFFSET        0x0004u
#define DEPZ_VL53LX_CAP_XTALK         0x0008u
#define DEPZ_VL53LX_CAP_CALIB_OFFSET  0x0010u
#define DEPZ_VL53LX_CAP_CALIB_XTALK   0x0020u
#define DEPZ_VL53LX_CAP_THRESHOLDS    0x0040u /* distance-window interrupt    */
#define DEPZ_VL53LX_CAP_SIGNAL_THRESH 0x0080u
#define DEPZ_VL53LX_CAP_SIGMA_THRESH  0x0100u
#define DEPZ_VL53LX_CAP_ROI           0x0200u
#define DEPZ_VL53LX_CAP_TEMP_UPDATE   0x0400u
#define DEPZ_VL53LX_CAP_REFSPAD       0x0800u /* VL53L0X reference SPADs      */

/* Detection-threshold windows (the die ULDs' SYSTEM__INTERRUPT_CONFIG). */
#define DEPZ_VL53LX_WINDOW_BELOW 0u
#define DEPZ_VL53LX_WINDOW_ABOVE 1u
#define DEPZ_VL53LX_WINDOW_OUT   2u
#define DEPZ_VL53LX_WINDOW_IN    3u

#define DEPZ_VL53LX_MAX_TARGETS 4
#define DEPZ_VL53LX_MAX_MODES   4

/* One return of a histogram frame. min / max_range_mm are the edges of the
 * target's own pulse. */
typedef struct {
    int32_t     distance_mm;
    int         status;
    const char *status_text;  /* static string */
    double      signal_kcps, ambient_kcps, sigma_mm;
    int32_t     min_range_mm, max_range_mm;
} depz_vl53lx_target;

/* The histogram frame the targets were found in (VL53LX_histogram_bin_data_t
 * after the driver read it: the VCSEL period, the bin sequence, the ambient
 * estimate). bin_data[0 .. number_of_bins) are the photon counts. */
typedef struct {
    uint8_t  interrupt_status, range_status, report_status, stream_count;
    uint32_t dss_actual_effective_spads;
    uint16_t reference_phase;
    uint8_t  vcsel_start;
    int32_t  bin_data[24];
    uint8_t  zone_id, first_bin, number_of_bins, bins_in_data;
    uint8_t  cal_config_vcsel_start, vcsel_width;
    uint16_t fast_osc_frequency;
    uint8_t  vcsel_period;     /* the register value */
    uint8_t  bin_seq[6], bin_rep[6];
    int32_t  min_bin_value, max_bin_value;
    uint8_t  number_of_ambient_bins;
    uint16_t number_of_ambient_samples;
    int32_t  ambient_events_sum, ambient_per_bin;
    uint32_t total_periods_elapsed, peak_duration_us, woi_duration_us;
    int32_t  zero_distance_phase;
    uint8_t  roi_centre_spad, roi_xy_size;
} depz_vl53lx_bins;

/* One ranging result, the same shape for every product. On the histogram
 * driver `targets` holds every return (the top-level fields repeat
 * targets[0]) and `bins` the raw frame; the light drivers leave both empty.
 * The driver extras read -1 (integers) or 0 where the driver has none. Use
 * depz_vl53lx_plottable() rather than status == 0 on histogram products. */
typedef struct {
    uint64_t    timestamp_us;  /* MCU µs at the INT edge (stream) / the read */
    int32_t     distance_mm;
    int         status;
    const char *status_text;   /* static string */
    double      signal_kcps, ambient_kcps, sigma_mm, spads;
    size_t      n_targets;
    depz_vl53lx_target targets[DEPZ_VL53LX_MAX_TARGETS];
    /* driver extras */
    int32_t     stream_count;          /* die ULDs, histogram               */
    double      signal_per_spad_kcps;  /* die ULDs                          */
    double      ambient_per_spad_kcps;
    int32_t     dmax_mm;               /* VL53L0X                           */
    int32_t     device_range_status;   /* VL53L0X                           */
    int32_t     min_range_mm, max_range_mm, peak_bin; /* histogram, targets[0] */
    bool        has_bins;
    depz_vl53lx_bins bins;
} depz_vl53lx_measurement;

typedef void (*depz_vl53lx_measurement_cb)(const depz_vl53lx_measurement *m, void *user);

/* Statuses that mean "this distance is real": 0, 6 (first histogram frame,
 * no wrap check yet) and 11 (merged target). */
bool depz_vl53lx_status_plottable(int status);
bool depz_vl53lx_plottable(const depz_vl53lx_measurement *m);
/* The first plottable target (or the single distance of a light driver);
 * false when the frame has none. */
bool depz_vl53lx_primary_distance(const depz_vl53lx_measurement *m, int32_t *distance_mm);

/* A 1D-family board on a link without an identity probe (tests, replay). */
int  depz_vl53lx_open_link(depz_link *link, depz_device **out);
bool depz_is_vl53lx(const depz_device *dev);
/* The product the board's device name carries (read once, cached), or
 * DEPZ_VL53LX_PRODUCT_NONE on an unstamped board — then name it at init(). */
depz_vl53lx_product depz_vl53lx_detected_product(depz_device *dev);
/* The driver kinds a product has (OR of depz_vl53lx_driver). */
unsigned depz_vl53lx_driver_kinds(depz_vl53lx_product product);

/* Bind (product, driver) and initialise the sensor. `product` NONE = the
 * detected one; `driver` 0 = the product's first kind (uld, ulp, histogram).
 * A pair without a driver: DEPZ_E_ARG naming what the product has. */
int  depz_vl53lx_init(depz_device *dev, depz_vl53lx_driver driver, depz_vl53lx_product product);
bool depz_vl53lx_initialized(const depz_device *dev);
depz_vl53lx_product depz_vl53lx_product_bound(const depz_device *dev);
/* The class the board opened as (fixed at open: PID / device name). */
depz_vl53lx_class   depz_vl53lx_get_class(const depz_device *dev);
depz_vl53lx_driver  depz_vl53lx_driver_bound(const depz_device *dev);
/* The driver's caveat for the pair ("" when none). */
const char *depz_vl53lx_caveat(const depz_device *dev);
/* What the configuration in use can reach, mm (0 = not characterised, or a
 * failed read). Read off the sensor: the valid phase window on the VL53L1 die,
 * the final-range VCSEL period's figure on the VL53L0X. The histogram driver
 * answers its preset's window host-side, with no traffic. */
uint32_t depz_vl53lx_driver_reach_mm(const depz_device *dev);
bool depz_vl53lx_supports(const depz_device *dev, unsigned cap);
bool depz_vl53lx_ranging(const depz_device *dev);
/* The model-id register (L1CX = L1CB, L4CD = L4CX: a cross-check only). */
int  depz_vl53lx_model_id(depz_device *dev, uint16_t *model_id);

/* Named ranging modes of the bound driver in UI order (get_mode() says which
 * one init left — medium on the histogram driver):
 * returns how many, fills up to `cap` static names. */
size_t depz_vl53lx_modes(const depz_device *dev, const char **names, size_t cap);
/* Budget range (inclusive) and, on products that only take some values, the
 * choices (ascending; *n = 0 when any integer in the range will do). */
int  depz_vl53lx_budget_range(const depz_device *dev, int *min_ms, int *max_ms);
int  depz_vl53lx_budget_choices(depz_device *dev, int *choices, size_t cap, size_t *n);

/* XSHUT pin (DEPZ_VL53L4_XSHUT_*): OFF and RESET stop the stream and forget
 * init — init() again. */
int  depz_vl53lx_xshut(depz_device *dev, uint8_t action);
/* Bridge counters and settings; safe while streaming. */
int  depz_vl53lx_bridge_info(depz_device *dev, depz_vl53lx_info *out);

/* Re-initialise and apply a configuration: `mode` NULL = leave the init
 * default, `offset_mm` / `xtalk_kcps` NULL = leave (a stored calibration
 * re-applied otherwise). inter_ms 0 = continuous. An unsupported group is
 * refused before the re-init. */
int  depz_vl53lx_configure(depz_device *dev, int budget_ms, int inter_ms, const char *mode,
                           const int32_t *offset_mm, const int32_t *xtalk_kcps);
/* depz_vl53lx_configure() plus `signal_kcps` (NULL = leave): replaces the
 * blob's signal threshold, applied last. The re-init puts it back to the
 * default, so a lowered one goes here, not in a set_signal_threshold_kcps()
 * before configure: frames past the default threshold come back status 2 with
 * the distance right (L1 long at ~4 m, a light driver borrowed onto a die
 * without the lens it was tuned for). */
int  depz_vl53lx_configure_ex(depz_device *dev, int budget_ms, int inter_ms, const char *mode,
                              const int32_t *offset_mm, const int32_t *xtalk_kcps, const int *signal_kcps);
int  depz_vl53lx_get_range_timing(depz_device *dev, int *budget_ms, int *inter_ms);
/* `budget_ms` (NULL = don't want it) gets the timing budget in effect after
 * the change on a driver whose mode change can move it — VL53L1: a budget the
 * new mode does not tabulate moves to the nearest one it does (15 ms short ->
 * 20 ms long; contract 12 §5) — else 0. */
int  depz_vl53lx_set_mode(depz_device *dev, const char *mode, int *budget_ms);
/* The mode in use; NULL (and DEPZ_OK) on a product without modes. */
int  depz_vl53lx_get_mode(depz_device *dev, const char **mode);

/* Capability-gated settings (init() first, not while ranging). */
int  depz_vl53lx_get_offset_mm(depz_device *dev, int32_t *offset_mm);
int  depz_vl53lx_set_offset_mm(depz_device *dev, int32_t offset_mm);
int  depz_vl53lx_get_xtalk_kcps(depz_device *dev, int32_t *xtalk_kcps);
int  depz_vl53lx_set_xtalk_kcps(depz_device *dev, int32_t xtalk_kcps);
/* Against a flat target; nb_samples 0 = the driver's default. Returns the
 * value now programmed — store it: the sensor forgets it on reset. */
int  depz_vl53lx_calibrate_offset(depz_device *dev, int target_mm, int nb_samples, int32_t *offset_mm);
int  depz_vl53lx_calibrate_xtalk(depz_device *dev, int target_mm, int nb_samples, int32_t *xtalk_kcps);
int  depz_vl53lx_get_detection_thresholds(depz_device *dev, int *low_mm, int *high_mm, int *window);
int  depz_vl53lx_set_detection_thresholds(depz_device *dev, int low_mm, int high_mm, int window);
int  depz_vl53lx_get_signal_threshold_kcps(depz_device *dev, int *kcps);
int  depz_vl53lx_set_signal_threshold_kcps(depz_device *dev, int kcps);
int  depz_vl53lx_get_sigma_threshold_mm(depz_device *dev, int *mm);
int  depz_vl53lx_set_sigma_threshold_mm(depz_device *dev, int mm);
int  depz_vl53lx_get_roi(depz_device *dev, int *x, int *y);
int  depz_vl53lx_set_roi(depz_device *dev, int x, int y);
int  depz_vl53lx_get_roi_center(depz_device *dev, int *spad);
int  depz_vl53lx_set_roi_center(depz_device *dev, int spad);
int  depz_vl53lx_start_temperature_update(depz_device *dev);
/* VL53L0X: re-measure the reference SPADs -> (count, is_aperture). */
int  depz_vl53lx_perform_ref_spad_management(depz_device *dev, uint32_t *count, bool *is_aperture);

/* Ranging. */
int  depz_vl53lx_start_ranging(depz_device *dev);
int  depz_vl53lx_stop_ranging(depz_device *dev); /* idempotent */
/* Poll mode (< 0 timeout = 1000 ms); refused while the stream runs. */
int  depz_vl53lx_measure_once(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out);
int  depz_vl53lx_on_measurement(depz_device *dev, depz_vl53lx_measurement_cb cb, void *user, int *token);
void depz_vl53lx_off_measurement(depz_device *dev, int token);
depz_stream *depz_vl53lx_measurements(depz_device *dev, size_t maxsize); /* 0 = 64 */
/* The next streamed measurement (< 0 timeout = 2000 ms). */
int  depz_vl53lx_get_measurement(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out);
uint64_t depz_vl53lx_stream_parse_errors(const depz_device *dev);

/* Raw register access (escape hatch), at the bound address width. */
int  depz_vl53lx_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
int  depz_vl53lx_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);

/* ======================================================================== */
/* BNO085 / BNO086 9-axis IMU (contract 05)                                  */
/* ======================================================================== */

/* The board is a thin SHTP pass-through: the whole SH-2 sensor-hub protocol
 * runs here, as the Python SDK's bno086/ does it (same requests in the same
 * order, so a capture made by either SDK replays in the other). The bridge
 * acknowledges each sent frame at once; answers arrive later as ordinary
 * sensor traffic and are matched on their SH-2 content (report id, sensor id,
 * command sequence). One class serves both chips: product_id() tells them
 * apart (part 10004148 = BNO085, 10004563 = BNO086).
 *
 * Typical use: hardware_reset(), enable(ROTATION_VECTOR, 100 Hz), then read
 * depz_bno_report items from reports() / get_report() and scale them with
 * depz_bno_report_quaternion() and friends. SH-2 refusals (a non-zero status
 * in a command or FRS answer) fail with DEPZ_E_STATUS; the message holds the
 * SH-2 status (depz_last_status() stays -1: it is not a bridge status). */

/* SEND_SHTP_PACKET attempts on ERR_BUSY (both MCU TX slots full), and the
 * back-off between them (contract 05 §2). */
#define DEPZ_BNO086_BUSY_RETRIES    5
#define DEPZ_BNO086_BUSY_BACKOFF_MS 200
/* A granted rate outside [0.9, 2.1] x the requested one is worth a warning
 * (contract 05 §7): the hub rounds to its 1 kHz / 2^n grid. */
#define DEPZ_BNO086_RATE_LOW_FACTOR  0.9
#define DEPZ_BNO086_RATE_HIGH_FACTOR 2.1

/* Everything Set Feature (0xFD) carries; zero-initialise and fill. */
typedef struct {
    uint32_t interval_us;   /* report interval; 0 is refused (use disable) */
    uint32_t batch_us;
    uint16_t sensitivity;   /* change sensitivity, sensor units */
    uint8_t  flags;         /* SH-2 §6.5.4 */
    uint32_t cfg_word;      /* sensor-specific configuration */
} depz_bno086_feature_request;

/* ME calibration enables as the sensor reports them. */
typedef struct { bool accel, gyro, mag, planar; } depz_bno086_calibration;

/* One error-queue entry (command 0x01). */
typedef struct { uint8_t severity, seq, source, error, module, code; } depz_bno086_error_record;

/* Per-sensor event counts (command 0x02). */
typedef struct {
    uint8_t  sensor_id;
    uint32_t offered, accepted, on, attempted;
} depz_bno086_counts;

/* Reports reach callbacks and streams as depz_bno_report. `data` (the bytes of
 * an UNKNOWN_REPORT) is valid inside a callback only; streamed copies hold
 * NULL there. */
typedef void (*depz_bno086_report_cb)(const depz_bno_report *r, void *user);

/* A BNO085 / BNO086 on a link without an identity probe (tests, replay). */
int  depz_bno086_open_link(depz_link *link, depz_device **out);
bool depz_is_bno086(const depz_device *dev);

/* nRST pulse (bridge 0x32): SHTP sequence counters, partial cargos, the
 * advertisement and every enabled sensor start from zero. Waits up to 0.5 s
 * for the executable reset-complete, best effort — older firmware never sends
 * it (ERRATA E9). < 0 timeout = 2000 ms. */
int depz_bno086_hardware_reset(depz_device *dev, int timeout_ms);
/* WAKE (PS0) pulse: out of sleep, no state lost. The board's hardware pin
 * pulse (bridge 0x33), not an SH-2 command — the SH-2 "On" that ends
 * depz_bno086_sleep() is depz_bno086_power_on(). */
int depz_bno086_wake(depz_device *dev);
/* SH-2 "Sleep" (0x03 on SHTP channel 1, contract 05 §6a): every report stops
 * except those enabled with DEPZ_BNO_FEATURE_WAKEUP | DEPZ_BNO_FEATURE_ALWAYS_ON,
 * which keep their rate; the configuration is kept. No SH-2 answer: returns
 * once the bridge took the frame. On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16)
 * all reports stopped within one sample; a WAKEUP|ALWAYS_ON report kept
 * streaming at full rate on channel 4. */
int depz_bno086_sleep(depz_device *dev);
/* SH-2 "On" (0x02 on SHTP channel 1): ends depz_bno086_sleep() — every report
 * that was enabled comes back at its rate without re-enabling. No SH-2 answer.
 * Not depz_bno086_wake(), which pulses the WAKE pin. */
int depz_bno086_power_on(depz_device *dev);
/* SHTP channel-0 advertisement bytes seen since open / reset; *len = full
 * length (copies at most `cap`). */
int depz_bno086_advertisement(depz_device *dev, uint8_t *buf, size_t cap, size_t *len);

/* The SH-2 application's Product ID — the first of depz_bno086_product_ids(). */
int depz_bno086_product_id(depz_device *dev, depz_bno_product_id *out);
/* Product ID Request → every software component's response, in the sensor's
 * order (the SH-2 application first; a BNO085 sends four). Collects until no
 * response came for 150 ms, so the late ones can't answer the next request.
 * Stores at most `cap`; *count = how many were stored. */
int depz_bno086_product_ids(depz_device *dev, depz_bno_product_id *out, size_t cap, size_t *count);

/* Enable `sensor` at `hz` (Set Feature), then read the granted rate back
 * (Get Feature) into *granted (optional). A "disabled" answer is re-read, five
 * times at most: it is stale (left by a preceding disable()) or early (the hub
 * reports 0 for a read-back or two before it applies the rate). enable_ex:
 * `granted` NULL skips the read-back. */
int depz_bno086_enable(depz_device *dev, uint8_t sensor, double hz, depz_bno_feature *granted);
int depz_bno086_enable_ex(depz_device *dev, uint8_t sensor, const depz_bno086_feature_request *req,
                          depz_bno_feature *granted);
/* Is `granted` within [0.9, 2.1] x the requested interval's rate? */
bool depz_bno086_rate_ok(uint32_t requested_interval_us, const depz_bno_feature *granted);
int  depz_bno086_disable(depz_device *dev, uint8_t sensor);
int  depz_bno086_get_feature(depz_device *dev, uint8_t sensor, depz_bno_feature *out);

/* Reports (channels 3, 4 and 5, timestamps in MCU µs). */
int  depz_bno086_on_report(depz_device *dev, depz_bno086_report_cb cb, void *user, int *token);
void depz_bno086_off_report(depz_device *dev, int token);
/* Pull stream of depz_bno_report items (0 = 1024). */
depz_stream *depz_bno086_reports(depz_device *dev, size_t maxsize);
/* The next report (< 0 timeout = 1000 ms). */
int  depz_bno086_get_report(depz_device *dev, int timeout_ms, depz_bno_report *out);
/* Incomplete / orphan SHTP cargos and junk-channel (>= 6) frames thrown away
 * by the reassembler. */
uint64_t depz_bno086_shtp_discarded(const depz_device *dev);

/* Tare and calibration (commands 3, 6, 7, 9). Tare and periodic DCD have no
 * SH-2 answer: they return once the bridge took the frame. */
int depz_bno086_tare_now(depz_device *dev, uint8_t axes, uint8_t basis);
int depz_bno086_persist_tare(depz_device *dev);
/* Runtime reorientation quaternion, Q14 on the wire; all zeros clears. */
int depz_bno086_set_reorientation(depz_device *dev, double x, double y, double z, double w);
int depz_bno086_set_calibration(depz_device *dev, bool accel, bool gyro, bool mag, bool planar);
int depz_bno086_get_calibration(depz_device *dev, depz_bno086_calibration *out);
int depz_bno086_save_dcd(depz_device *dev);
int depz_bno086_configure_periodic_dcd(depz_device *dev, bool enable);

/* Interactive calibration (BNO086 only; a BNO085 ignores it). Tell the hub
 * how the device is about to move (command 0x0E, `intent` =
 * DEPZ_BNO_MOTION_INTENT_*), so it can refine the gyro zero-rate offset while
 * the device is still. No SH-2 answer: returns once the bridge took the
 * frame. The hub's own request — hold still, or go on — arrives as a
 * DEPZ_BNO_MOTION_REQUEST report once DEPZ_BNO_SENSOR_MOTION_REQUEST is
 * enabled. */
int depz_bno086_set_motion_intent(depz_device *dev, uint8_t intent);
/* Enable MotionRequest (0x2B) reports. The hub sends one when its request
 * changes, not at `hz`, and reads the rate back late, so there is no
 * read-back of the granted rate. */
int depz_bno086_enable_motion_request(depz_device *dev, double hz);

/* Batching: Force Sensor Flush (0xF0) — the hub sends `sensor`'s batched
 * reports now instead of when its batch interval runs out. Returns once the
 * hub's Flush Completed (0xEF, a DEPZ_BNO_FLUSH_COMPLETED_REPORT) for `sensor`
 * arrived — every report batched before the call has been emitted by then —
 * or DEPZ_E_TIMEOUT after the device timeout (at least 1 s). */
int depz_bno086_flush(depz_device *dev, uint8_t sensor);

/* FRS records. read: *n = the record's word count (DEPZ_E_ARG after reading
 * when it exceeds `cap`). */
int depz_bno086_frs_read(depz_device *dev, uint16_t record, uint32_t *words, size_t cap, size_t *n);
int depz_bno086_frs_write(depz_device *dev, uint16_t record, const uint32_t *words, size_t n);
/* A sensor's metadata record; DEPZ_E_ARG for a sensor without one. */
int depz_bno086_get_metadata(depz_device *dev, uint8_t sensor, depz_bno_metadata *out);

/* Diagnostics and housekeeping. */
int depz_bno086_get_oscillator_type(depz_device *dev, uint8_t *type); /* DEPZ_BNO_OSC_* */
/* Clear the in-RAM dynamic calibration and reset (command 0x0B); waits for the
 * executable reset-complete (< 0 timeout = 2000 ms). */
int depz_bno086_clear_dcd_and_reset(depz_device *dev, int timeout_ms);
/* The error queue, `severity` or worse (0 = all); *n = entries returned. */
int depz_bno086_get_errors(depz_device *dev, uint8_t severity, depz_bno086_error_record *out,
                           size_t cap, size_t *n);
int depz_bno086_get_counts(depz_device *dev, uint8_t sensor, depz_bno086_counts *out);
int depz_bno086_clear_counts(depz_device *dev, uint8_t sensor);

/* Escape hatches: a Command Request (`params` up to 9 bytes; *resp NULL = do
 * not wait for an answer), and a raw SHTP cargo on `channel`. */
int depz_bno086_command(depz_device *dev, uint8_t command, const uint8_t *params, size_t n,
                        depz_bno_command_response *resp);
int depz_bno086_send_shtp(depz_device *dev, uint8_t channel, const uint8_t *payload, size_t len);

/* ======================================================================== */
/* Datasets: the live record values and the session recorder (contract 09)  */
/* ======================================================================== */

/* The contract 09 record value (`v`) of the result types that only exist in
 * the live layer, key for key the Python reference; the codec layer
 * (depz_sensor_sdk.h, depz_dataset_*_record) builds the rest. */

/* `sr04`: {echo_us, source: "once" | "loop"}. */
depz_json *depz_dataset_sr04_record(const depz_sr04_measurement *m);
/* `vl53l8`: the frame (depz_dataset_vl53l8_record) plus `motion` when the
 * frame carried the motion-indicator block. */
depz_json *depz_dataset_vl53l8_live_record(const depz_vl53l8_live_frame *f, bool layers);
/* `vl53l8_cnh`: the raw CNH block of a frame, base64, with the armed setup's
 * decode parameters (nb_aggregates, feature_length, sum_span, start_bin). */
depz_json *depz_dataset_vl53l8_cnh_record(const uint8_t *cnh_raw, size_t len,
                                          const depz_vl53l8_cnh_setup *setup);
/* `vl53lx`: product, driver, status, distance_mm, sigma_mm, signal_kcps,
 * ambient_kcps, spads, and `targets` when the driver reported several. */
depz_json *depz_dataset_vl53lx_record(const depz_vl53lx_measurement *m, const char *product,
                                      const char *driver);
/* `bno055`: unit_sel and every channel the block covered, scaled. */
depz_json *depz_dataset_bno055_record(const depz_bno055_sample *s);

/* Record decoded data from several devices onto one host timeline. add()
 * each open device (it runs depz_device_sync_time() and hooks the device's
 * result stream), then start(): a record per result at device time − offset,
 * written on the device's reader thread. The recorder never reconfigures a
 * device — start and stop streaming yourself. stop() closes the file; free()
 * stops first. A class without a result stream records nothing. */
typedef struct depz_session_recorder depz_session_recorder;

typedef struct {
    const char *note;      /* free text in the header, NULL for none */
    bool        vl53l8_layers; /* VL53L8 records also carry signal / ambient / sigma / reflectance */
    double      temperature_hz; /* about this many `temperature` records per device and
                                   second (the MCU thermometer, and the sensor die where
                                   frames carry one); 0 = none */
} depz_session_recorder_options;

#define DEPZ_SESSION_RECORDER_OPTIONS_INIT { NULL, false, 1.0 }

/* A recorder writing `path` (`.depzdata`, or `.depzdata.gz`); `opt` NULL =
 * the defaults. NULL out of memory. */
depz_session_recorder *depz_session_recorder_new(const char *path,
                                                 const depz_session_recorder_options *opt);
/* Register a device before recording starts (DEPZ_E_ARG after): runs
 * sync_time with `sync_samples` rounds so its timestamps land on the shared
 * timeline, reads its serial and software name, hooks its stream. The device
 * id is `device_id`, or d0, d1, … in add order; it is copied to `id_out`
 * (cap bytes, may be NULL). */
int depz_session_recorder_add(depz_session_recorder *rec, depz_device *dev, const char *device_id,
                              int sync_samples, char *id_out, size_t cap);
/* Open the file and start writing. No-op while recording; DEPZ_E_IO when the
 * file cannot be created. */
int depz_session_recorder_start(depz_session_recorder *rec);
/* Stop writing, unhook the devices and close the file. Idempotent. */
int depz_session_recorder_stop(depz_session_recorder *rec);
/* Records in the file being written; after stop(), in the finished one. */
uint64_t depz_session_recorder_records_written(const depz_session_recorder *rec);
/* stop() and free; NULL is a no-op. */
void depz_session_recorder_free(depz_session_recorder *rec);

#ifdef __cplusplus
}
#endif

#endif /* DEPZ_SENSOR_IO_H */
