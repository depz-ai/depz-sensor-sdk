/*
 * depz_sensor_sdk.h — DEPZ USB sensor-line C SDK (transport + protocol
 * foundation). Contract-first: byte-exact with the contract docs and the
 * golden vectors in contracts/vectors/. C11, no dependencies.
 *
 * Layers implemented here:
 *   - CRCs (contract 01 §3)
 *   - packet framing: build_packet + incremental parser (contract 01 §2/§5,
 *     ERRATA E6)
 *   - USB identity table + is_known_depz_usb (contract 02 §4.2)
 *   - firmware-name identity parsing (contract 02 §4.1)
 *   - common command/report codecs (contract 02)
 *   - SR04 codecs (contract 03)
 *   - VL53L4CD register-bridge + host-ULD codecs (contract 10)
 *   - VL53L5CX/L7CX/L7CH I2C bridge codecs + frame decode (contract 11)
 *   - VL53L 1D family (L0X/L1CX/L1CB/L3CX/L4CD/L4CX) bridge v2.00 codecs,
 *     product table and stateless block decoders (contract 12)
 *   - BNO055 register-bridge codecs, units / calibration / axis-remap /
 *     page-1 config codecs and register-window decode (contract 13)
 *   - TMF8829 SPI-bridge codecs, chunk reassembly, frame / pixel decode,
 *     clock model + capture time, measurement assembly, config page and the
 *     embedded sensor firmware (contract 14)
 *   - .fwdepz container parse/validate (contract 06 §2)
 *   - .depzdata datasets: JSON values, reader (also .gz), writer, the
 *     record value of each kind, the player (contract 09)
 *   - the error codes (depz_err) and the per-thread failure detail the live
 *     layer and the dataset reader report with (depz_last_error())
 *
 * Sensor host-logic (the ST drivers, the BNO086 SH-2 hub protocol) lives in
 * the live layer, depz_sensor_io.h; this header is pure codecs.
 */
#ifndef DEPZ_SENSOR_SDK_H
#define DEPZ_SENSOR_SDK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Errors. Codecs report a bad buffer by their return value; the live layer  */
/* (depz_sensor_io.h) and the .depzdata reader also leave a message here.    */
/* ======================================================================== */

typedef enum {
    DEPZ_OK                 = 0,
    DEPZ_E_ARG              = -1,  /* bad argument, or not valid in this state */
    DEPZ_E_NOMEM            = -2,
    DEPZ_E_IO               = -3,  /* the OS refused: port cannot be opened... */
    DEPZ_E_CLOSED           = -4,  /* link closed or device lost */
    DEPZ_E_TIMEOUT          = -5,  /* no reply in time */
    DEPZ_E_STATUS           = -6,  /* device answered a non-OK RPT_STATUS */
    DEPZ_E_BUSY             = -7,  /* ERR_BUSY from the device, or the same
                                      opcode is already in flight */
    DEPZ_E_PROTOCOL         = -8,  /* a reply that does not parse */
    DEPZ_E_NO_DEVICE        = -9,  /* discovery found no (matching) DEPZ device */
    DEPZ_E_WRONG_TYPE       = -10, /* sensor call on a device of another type */
    DEPZ_E_REPLAY_MISMATCH  = -11, /* strict replay: written bytes differ */
    DEPZ_E_BOOTLOADER       = -12  /* the device is in bootloader mode */
} depz_err;

/* Short constant name of an error code ("DEPZ_E_TIMEOUT"). */
const char *depz_err_str(int err);
/* Human-readable detail of the last failure on the calling thread. */
const char *depz_last_error(void);
/* depz_status of the last DEPZ_E_STATUS / DEPZ_E_BUSY on the calling thread
 * (and the opcode it answered), -1 when the last failure was something else. */
int depz_last_status(void);
int depz_last_status_cmd(void);

/* ======================================================================== */
/* CRC algorithms (contract 01 §3). All reflected table implementations      */
/* except CCITT-FALSE. CRC-8 init is 0x00 for every device (ERRATA E1).      */
/* ======================================================================== */

uint8_t  depz_crc8_maxim(const uint8_t *data, size_t len);
uint16_t depz_crc16_modbus(const uint8_t *data, size_t len);
uint32_t depz_crc32_iso_hdlc(const uint8_t *data, size_t len);
/* CRC-16/CCITT-FALSE — only the .fwdepz file header, never on the wire. */
uint16_t depz_crc16_ccitt_false(const uint8_t *data, size_t len);

/* ======================================================================== */
/* Packet framing (contract 01)                                              */
/* ======================================================================== */

#define DEPZ_MAGIC0      0xA5u
#define DEPZ_MAGIC1      0xC3u
#define DEPZ_HEADER_SIZE 7u
#define DEPZ_MAX_PAYLOAD 0x3FFFu /* 16383 */

typedef enum {
    DEPZ_CRC_NONE  = 0,
    DEPZ_CRC8      = 1,
    DEPZ_CRC16     = 2,
    DEPZ_CRC32     = 3
} depz_crc_type;

/* Worst-case frame size (header + max payload + CRC32 trailer). */
#define DEPZ_MAX_FRAME (DEPZ_HEADER_SIZE + DEPZ_MAX_PAYLOAD + 4u)

/*
 * Frame one packet into `out` (capacity `out_cap`), writing the length to
 * *out_len. crc_type bits are set in the header even for an empty payload
 * (matching device TX), but a CRC trailer is appended only for a non-empty
 * payload (ERRATA E6). `seq` is taken modulo 256.
 * Returns 0 on success, -1 on payload-too-long, -2 on insufficient capacity,
 * -3 when crc_type is not one of the listed depz_crc_type values.
 */
int depz_build_packet(uint8_t cmd, const uint8_t *payload, size_t payload_len,
                      unsigned int seq, depz_crc_type crc_type,
                      uint8_t *out, size_t out_cap, size_t *out_len);

/* Incremental parser -------------------------------------------------------*/

typedef enum {
    DEPZ_EV_PACKET,
    DEPZ_EV_TRASH,
    DEPZ_EV_CRC_ERROR
} depz_event_type;

typedef struct {
    depz_event_type type;
    /* PACKET / CRC_ERROR */
    uint8_t cmd;
    uint8_t seq;
    /* PACKET: payload bytes (valid only for the duration of the callback). */
    const uint8_t *payload;
    size_t payload_len;
    /* TRASH: discarded bytes (valid only during the callback). */
    const uint8_t *trash;
    size_t trash_len;
} depz_event;

typedef void (*depz_event_cb)(const depz_event *ev, void *user);

typedef struct {
    uint8_t *buf;
    size_t   len;
    size_t   cap;
    /* Diagnostics counters (monotonic across the parser's life). */
    uint64_t packets;
    uint64_t crc_errors;
    uint64_t header_errors;
    uint64_t trash_bytes;
} depz_parser;

void depz_parser_init(depz_parser *p);
void depz_parser_free(depz_parser *p);
/*
 * Append `data` and drain every complete event, invoking `cb` (may be NULL)
 * for each. Event ordering is invariant to how the byte stream is chunked
 * (contract 01 §5); only Trash event *boundaries* depend on chunking.
 * Returns 0 on success, -1 on allocation failure.
 */
int depz_parser_feed(depz_parser *p, const uint8_t *data, size_t len,
                     depz_event_cb cb, void *user);

/* ======================================================================== */
/* USB identity table (contract 02 §4.2)                                     */
/* ======================================================================== */

#define DEPZ_USB_VID   0x1BCFu /* 7119  — production VID for all DEPZ sensors */
#define DEPZ_PID_SR04  0xEC78u /* 60536 */
#define DEPZ_PID_VL53L8 0xED40u /* 60736 */
#define DEPZ_PID_VL53L4CD 0xED45u /* 60741 */
#define DEPZ_PID_TMF8829 0xED4Cu /* 60748 */
#define DEPZ_PID_BNO086 0xEE08u /* 60936 */
#define DEPZ_PID_RANGE_LO 60536u
#define DEPZ_PID_RANGE_HI 65535u
#define DEPZ_DEV_USB_VID 0x0483u /* 1155  — STMicro dev/unprogrammed default */
#define DEPZ_DEV_USB_PID 0x56DCu /* 22236 */

/* True when (vid, pid) is a recognized DEPZ (or dev-default) USB id. */
bool depz_is_known_depz_usb(int vid, int pid);
/* Best-guess model name for a (vid, pid), or NULL. Informational only. */
const char *depz_usb_model_hint(int vid, int pid);
/* Both at once, checked: -1 when vid or pid is not a u16 (0..0xFFFF) — an
 * invalid id, not an unknown one (contract 07 §3); else 0 with *known /
 * *model (either may be NULL) as the two functions above give them. */
int depz_usb_identity(int vid, int pid, bool *known, const char **model);

/* One serial port as the discovery order sees it (contract 02 §4): its path
 * and its USB iSerial, each with an explicit length so a string holding an
 * embedded NUL compares whole. `serial` NULL or empty = no serial. */
typedef struct {
    const char *port;
    size_t      port_len;
    const char *serial;
    size_t      serial_len;
} depz_port_key;

/* The discovery order of two ports (contract 02 §4, the comparator
 * depz_find_devices() sorts with): by USB serial ascending (bytewise), every
 * port without a serial after every port with one, then by port path.
 * Returns < 0, 0 or > 0 like strcmp. */
int depz_port_order_cmp(const depz_port_key *a, const depz_port_key *b);

/* ======================================================================== */
/* Firmware-name identity (contract 02 §4.1)                                 */
/* ======================================================================== */

typedef enum {
    DEPZ_SENSOR_NONE = -1, /* null: bootloader/unknown mode */
    DEPZ_SENSOR_SR04 = 0,
    DEPZ_SENSOR_VL53L8,    /* firmware id shared by both VL53L8CX and VL53L8CH
                            * (the APP_* name reports "VL53L8" for either);
                            * the CX/CH split is DEPZ_VL53L8_VARIANT_*. */
    DEPZ_SENSOR_BNO086,
    DEPZ_SENSOR_VL53L4,    /* VL53L4CD single-zone ToF (contract 10) */
    DEPZ_SENSOR_VL53L7,    /* APP_VL53L7: one firmware for VL53L5CX, VL53L7CX
                            * and VL53L7CH (contract 11); the class split is
                            * depz_vl53l7_resolve_model(). */
    DEPZ_SENSOR_VL53LX,    /* APP_VL53L0_4 (spec name APP_VL53LX): one bridge
                            * for VL53L0X/L1CX/L1CB/L3CX/L4CD/L4CX (contract
                            * 12); the product is depz_vl53lx_resolve_product(). */
    DEPZ_SENSOR_BNO055,    /* APP_BNO055: 9-axis IMU register bridge, fusion
                            * on chip (contract 13) */
    DEPZ_SENSOR_TMF8829,   /* APP_TMF8829: multizone dToF over an SPI register
                            * bridge (contract 14) */
    DEPZ_SENSOR_UNKNOWN    /* an APP_* name we don't recognize; must stay last */
} depz_sensor_type;

typedef struct {
    const char *mode; /* "app" | "bootloader" | "unknown" */
    depz_sensor_type sensor_type;
    char software_name[64];
    char version[24]; /* "" when not parseable */
} depz_identity;

const char *depz_sensor_type_str(depz_sensor_type t); /* NULL for NONE */

/* Strip trailing 0x00/0xFF filler and decode the rest as ASCII into `out`
 * (contract 02 §4.1): every byte >= 0x80 becomes U+FFFD (written as UTF-8,
 * EF BF BD), and an embedded 0x00 before the filler is KEPT as a character.
 * `out` is NUL-filled to `out_cap`; a character that does not fit whole is
 * dropped with the rest. Returns the string length written (embedded NULs
 * included) — or, when out_cap == 0, the length the whole string needs. */
size_t depz_strip_device_string(const uint8_t *data, size_t len,
                                char *out, size_t out_cap);

/* Length of a stripped device string held in a NUL-filled buffer of `cap`
 * bytes (depz_text_report.text, depz_identity.software_name): up to its last
 * non-NUL byte, so embedded NULs count. */
size_t depz_device_string_len(const char *buf, size_t cap);

/* Classify a GET_NAME_ACTIVE_SOFTWARE string (already stripped). Product
 * tokens are checked in the contract's order (SR04, VL53L4, VL53L8, VL53L7,
 * VL53L0_4, VL53LX, BNO086, BNO055, TMF8829): the first one found wins.
 * `software_name` / `version` are NUL-filled copies, truncated to fit. */
void depz_parse_software_name(const char *name, depz_identity *out);
/* The same over name[0..len): a name holding embedded NULs (as
 * depz_strip_device_string keeps them) is classified whole. */
void depz_parse_software_name_n(const char *name, size_t len, depz_identity *out);

/*
 * Class resolution (contract 02 §4.4, pure): the canonical class name for a
 * device's probe answers — `software_name` (GET_NAME_ACTIVE_SOFTWARE) and
 * `device_name` (GET_DEVICE_NAME; NULL or "" when there was none) — and its
 * port's USB id (-1 = unknown: a replay, a bridge). One of the
 * DEPZ_SENSOR_CLASS_COUNT names of depz_sensor_class_name():
 *   - BOOTDEPZ_*  -> "bootloader" (no sensor class: contract 06 client);
 *   - APP_VL53L8  -> "vl53l8ch" if the USB model is VL53L8CH, else "vl53l8cx";
 *   - APP_VL53L7  -> depz_vl53l7_resolve_model() (USB model, then the name);
 *   - APP_VL53L0_4 -> depz_vl53lx_resolve_class() (USB model, then the name,
 *     else the generic "vl53lx"; VL53L4CD is generic on this firmware);
 *   - any other known token -> its class; unknown APP_* or a name that is
 *     neither APP_ nor BOOTDEPZ -> "device" (the base device).
 * Returns a static string. Vectors: contracts/vectors/probe.json.
 */
const char *depz_resolve_sensor_class(const char *software_name, const char *device_name,
                                      int usb_vid, int usb_pid);
/* The same with explicit lengths (strings may hold embedded NULs);
 * `device_name` may be NULL (its length is then ignored). The USB ids are
 * pointers: NULL = unknown; a given id must be a u16, else the result is
 * NULL (an invalid id, contract 07 §3). */
const char *depz_resolve_sensor_class_n(const char *software_name, size_t software_name_len,
                                        const char *device_name, size_t device_name_len,
                                        const int *usb_vid, const int *usb_pid);
/* The canonical class names (SENSOR_CLASS_NAMES), in contract order:
 * "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx",
 * "vl53l7ch", "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx",
 * "vl53lx", "bno086", "bno055", "tmf8829", "device", "bootloader".
 * depz_sensor_class_name(i) is NULL for i >= DEPZ_SENSOR_CLASS_COUNT. */
#define DEPZ_SENSOR_CLASS_COUNT 18
const char *depz_sensor_class_name(size_t i);

/* ======================================================================== */
/* Common command / report codecs (contract 02)                              */
/* ======================================================================== */

typedef enum {
    DEPZ_CMD_BOOTLOADER               = 0x01,
    DEPZ_CMD_DEVICE_RESET             = 0x02,
    DEPZ_CMD_GET_DEVICE_NAME          = 0x03,
    DEPZ_CMD_GET_NAME_ACTIVE_SOFTWARE = 0x04,
    DEPZ_CMD_GET_SERIAL               = 0x05,
    DEPZ_CMD_SYNC_TIME                = 0x06,
    DEPZ_CMD_GET_MCU_TEMPERATURE      = 0x07,
    DEPZ_CMD_GET_PAYLOAD_CRC_TYPE     = 0x08,
    DEPZ_CMD_SET_PAYLOAD_CRC_TYPE     = 0x09,
    DEPZ_CMD_USB_TEST_TX_START        = 0x1C,
    DEPZ_CMD_USB_TEST_TX_STOP         = 0x1D,
    DEPZ_CMD_USB_TEST_RX_DATA         = 0x1E,
    DEPZ_CMD_USB_TEST_STATS           = 0x1F,
    DEPZ_CMD_GET_SYNC_PIN_CONFIG      = 0x30,
    DEPZ_CMD_SET_SYNC_PIN_CONFIG      = 0x31
} depz_cmd;

typedef enum {
    DEPZ_RPT_STATUS           = 0x80,
    DEPZ_RPT_TEXT             = 0x81,
    DEPZ_RPT_SYNC_TIME        = 0x82,
    DEPZ_RPT_TEMPERATURE      = 0x83,
    DEPZ_RPT_SEQUENCE_ERROR   = 0x84,
    DEPZ_RPT_PAYLOAD_CRC_TYPE = 0x87,
    DEPZ_RPT_USB_TEST_DATA    = 0x88,
    DEPZ_RPT_USB_TEST_STATS   = 0x8F,
    DEPZ_RPT_SYNC_PIN_CONFIG  = 0x90
} depz_rpt;

typedef enum {
    DEPZ_STATUS_OK                    = 0x00,
    DEPZ_STATUS_ERROR                 = 0x01,
    DEPZ_STATUS_ERR_INVALID_CMD       = 0x02,
    DEPZ_STATUS_ERR_PAYLOAD_FORMAT    = 0x03,
    DEPZ_STATUS_ERR_INVALID_PARAM     = 0x04,
    DEPZ_STATUS_ERR_PAYLOAD_CRC       = 0x05,
    DEPZ_STATUS_ERR_BUSY              = 0x06,
    DEPZ_STATUS_ERR_CMD_NOT_SUPPORTED = 0x07,
    DEPZ_STATUS_ERR_NOT_INITIALIZED   = 0x08,
    DEPZ_STATUS_ERR_HARDWARE_FAULT    = 0x09
} depz_status;

typedef enum {
    DEPZ_SYNC_PIN_DISABLE   = 0x00,
    DEPZ_SYNC_PIN_IN        = 0x01,
    DEPZ_SYNC_PIN_OUT_START = 0x02,
    DEPZ_SYNC_PIN_OUT_END   = 0x03,
    DEPZ_SYNC_PIN_OUT_BOTH  = 0x04
} depz_sync_pin_mode;

typedef struct { uint8_t cmd; uint8_t status; } depz_status_report;
typedef struct { uint8_t cmd; char text[256]; } depz_text_report;
typedef struct { uint64_t pc_timestamp_us; uint64_t mcu_rx_us; uint64_t mcu_tx_us; } depz_sync_time_report;
typedef struct { uint64_t timestamp_us; int16_t raw_decidegrees; } depz_temperature_report;
typedef struct { uint8_t expected_seq; uint8_t received_seq; } depz_sequence_error_report;
typedef struct { uint8_t pin; uint8_t mode; uint8_t polarity; } depz_sync_pin_config;

/* Encoders (return payload length written). */
size_t depz_pack_sync_time(uint64_t pc_timestamp_us, uint8_t *out); /* 8 B */
/* SET_PAYLOAD_CRC_TYPE payload: crc_type u8. Returns 1, or 0 when crc_type is
 * not 0..3 (contract 02 §2). */
size_t depz_pack_set_payload_crc_type(uint8_t crc_type, uint8_t *out);
/* 3 B, or 0 (nothing written) unless pin is 1..5, mode a depz_sync_pin_mode
 * (0..4) and polarity 0 (idle low) or 1 (idle high) — contract 02 §6. */
size_t depz_pack_sync_pin_config(const depz_sync_pin_config *c, uint8_t *out);

/* Decoders (contract 07 §3): return 0 on success, -1 when the payload is
 * shorter than the report's fields; trailing bytes are ignored. RPT_TEXT's
 * text is depz_strip_device_string()'s (NUL-filled; embedded NULs kept —
 * depz_device_string_len() gives its length). */
int depz_unpack_status(const uint8_t *p, size_t len, depz_status_report *out);
int depz_unpack_text(const uint8_t *p, size_t len, depz_text_report *out);
/* RPT_TEXT into a caller buffer of any size (depz_text_report.text holds 255
 * bytes): *cmd, the stripped text in text[text_cap] (as
 * depz_strip_device_string; text_cap = 3 * (len - 1) + 1 always holds it
 * whole) and its length in *text_len (may be NULL). 0, or -1 for an empty
 * payload. */
int depz_unpack_text_ex(const uint8_t *p, size_t len, uint8_t *cmd,
                        char *text, size_t text_cap, size_t *text_len);
int depz_unpack_sync_time(const uint8_t *p, size_t len, depz_sync_time_report *out);
int depz_unpack_temperature(const uint8_t *p, size_t len, depz_temperature_report *out);
int depz_unpack_sequence_error(const uint8_t *p, size_t len, depz_sequence_error_report *out);
int depz_unpack_sync_pin_config(const uint8_t *p, size_t len, depz_sync_pin_config *out);

/* Time-sync math (contract 02 §5). offset = ((T2-T1)+(T3-T4))/2 truncated
 * toward zero; rtt = (T4-T1)-(T3-T2). */
void depz_sync_time_offset_rtt(int64_t t1, int64_t t2, int64_t t3, int64_t t4,
                               int64_t *offset_us, int64_t *rtt_us);

/* ======================================================================== */
/* SR04 codecs (contract 03)                                                 */
/* ======================================================================== */

typedef enum {
    DEPZ_SR04_GET_SAMPLE_PERIOD      = 0x32,
    DEPZ_SR04_SET_SAMPLE_PERIOD      = 0x33,
    DEPZ_SR04_GET_ECHO_DECAY         = 0x34,
    DEPZ_SR04_SET_ECHO_DECAY         = 0x35,
    DEPZ_SR04_MEASURE_ONCE           = 0x36,
    DEPZ_SR04_START_MEASUREMENT_LOOP = 0x37,
    DEPZ_SR04_STOP_MEASUREMENT_LOOP  = 0x38
} depz_sr04_cmd;

typedef enum {
    DEPZ_SR04_RPT_DATA          = 0x91,
    DEPZ_SR04_RPT_SAMPLE_PERIOD = 0x92,
    DEPZ_SR04_RPT_ECHO_DECAY    = 0x93
} depz_sr04_rpt;

#define DEPZ_SR04_ECHO_TIMEOUT       0xFFFFu /* echo_time_us sentinel: no echo */
#define DEPZ_SR04_ECHO_DECAY_MIN_US  4000u
#define DEPZ_SR04_ECHO_DECAY_MAX_US  65000u

typedef struct {
    uint8_t  source_cmd;   /* 0x36 single shot (host or SYNC_IN), 0x37 loop */
    uint64_t timestamp_us;
    uint16_t echo_time_us; /* 0xFFFF = timeout sentinel */
} depz_sr04_data;

size_t depz_sr04_pack_sample_period(uint32_t period_us, uint8_t *out); /* 4 B */
size_t depz_sr04_pack_echo_decay(uint16_t decay_us, uint8_t *out);     /* 2 B */

int depz_sr04_unpack_data(const uint8_t *p, size_t len, depz_sr04_data *out);
int depz_sr04_unpack_sample_period(const uint8_t *p, size_t len, uint32_t *out);
int depz_sr04_unpack_echo_decay(const uint8_t *p, size_t len, uint16_t *out);

/* Round-trip echo time -> distance in mm; returns false for the timeout
 * sentinel. Default 343 m/s; if air_temp_c is finite, c = 331.3 + 0.606*T. */
bool depz_sr04_distance_mm(uint16_t echo_time_us, double air_temp_c,
                           bool have_temp, double *out_mm);

/* ======================================================================== */
/* .fwdepz container (contract 06 §2)                                        */
/* ======================================================================== */

#define DEPZ_FWDEPZ_HEADER_SIZE 64u

typedef enum {
    DEPZ_FWDEPZ_OK = 0,
    DEPZ_FWDEPZ_ERR_TOO_SHORT,
    DEPZ_FWDEPZ_ERR_MAGIC,
    DEPZ_FWDEPZ_ERR_HEADER_CRC,
    DEPZ_FWDEPZ_ERR_SIZE,
    DEPZ_FWDEPZ_ERR_SECTION /* not section 0 of 1: multi-section unsupported */
} depz_fwdepz_result;

typedef struct {
    uint32_t load_addr;
    uint32_t fw_size;
    uint32_t fw_crc32;
    uint8_t  cur_sec;
    uint8_t  tot_sec;
    const uint8_t *payload; /* points into the input blob */
    size_t   payload_len;
    bool     payload_crc_ok;
} depz_fwdepz_image;

/* Parse & validate (magic -> header CRC-16/CCITT-FALSE -> fw_size==payload
 * -> section 0 of 1; sections are 0-based). */
depz_fwdepz_result depz_fwdepz_parse(const uint8_t *blob, size_t len,
                                     depz_fwdepz_image *out);

/* Bootloader opcode space (contract 06 §1) — DIFFERENT from application. */
typedef enum {
    DEPZ_BL_BOOT_APPLICATION = 0x01,
    DEPZ_BL_DEVICE_RESET     = 0x02,
    DEPZ_BL_GET_DEVICE_NAME  = 0x03,
    DEPZ_BL_GET_FIRMWARE_NAME = 0x04,
    DEPZ_BL_GET_SERIAL       = 0x05,
    DEPZ_BL_GET_MCU_ID       = 0x06,
    DEPZ_BL_GET_MCU_UID      = 0x07,
    DEPZ_BL_GET_FLASH_INFO   = 0x08,
    DEPZ_BL_ERASE_APP        = 0x09,
    DEPZ_BL_WRITE_PAGE       = 0x0A,
    DEPZ_BL_READ_PAGE        = 0x0B,
    DEPZ_BL_VERIFY_APP_CRC   = 0x0C
    /* 0x0D ENTER_DFU deliberately absent: out of SDK scope (contract 06). */
} depz_bl_cmd;

typedef struct { uint16_t page_size; uint32_t app_start; uint32_t app_size; } depz_flash_info;

size_t depz_bl_pack_write_page(uint32_t addr, const uint8_t *data, size_t data_len,
                               uint8_t *out, size_t out_cap); /* 6 + data_len */
size_t depz_bl_pack_read_page(uint32_t addr, uint16_t size, uint8_t *out); /* 6 B */
int depz_bl_unpack_flash_info(const uint8_t *p, size_t len, depz_flash_info *out);

/* ======================================================================== */
/* VL53L8 register-bridge streaming: frame reassembly + decode (contract 04) */
/*                                                                           */
/* The DEPZ ToF line is TWO sensors that share this register-bridge frame    */
/* format: VL53L8CX (base ToF, the dev-default) and VL53L8CH (CX plus the    */
/* CNH compact-network-histogram feature; its own production USB PID 0xED40).*/
/* The chunk parse / reassembler / raw-results frame decoder below are SHARED */
/* by BOTH variants and are the only VERIFIABLE decode path implemented here. */
/*                                                                           */
/* The live ULD driver (firmware download, DCI register sequences) is the    */
/* depz_vl53l8_* sensor class of depz_sensor_io.h, built on these codecs.     */
/* The CH frame and CNH decoders are below (depz_vl53l8ch_decode_frame /      */
/* _decode_cnh).                                                              */
/* ======================================================================== */

/* Which ToF sensor produced a frame. CX and CH share the ranging-results    */
/* blocks but not the footer position: decode CX frames with                  */
/* depz_vl53l8_decode_frame, CH frames with depz_vl53l8ch_decode_frame. */
typedef enum {
    DEPZ_VL53L8_VARIANT_CX = 0, /* base ToF (dev-default)                    */
    DEPZ_VL53L8_VARIANT_CH = 1  /* CX + CNH compact-network-histograms (ED40)*/
} depz_vl53l8_variant;

/* Footer-id offset in a decoded frame: the header id at [8..9] must equal    */
/* the footer id at [raw_len - N]. N = 12 for the VL53L8CX ULD 2.1.0 frame;   */
/* the CH (VL53LMZ) frame uses 4 — depz_vl53l8ch_decode_frame, which also     */
/* hands out the CNH block for depz_vl53l8ch_decode_cnh(). */
#define DEPZ_VL53L8CX_FOOTER_ID_OFFSET 12

#define DEPZ_VL53L8_CMD_READ_REG     0x32
#define DEPZ_VL53L8_CMD_WRITE_REG    0x33
#define DEPZ_VL53L8_CMD_START_STREAM 0x35
#define DEPZ_VL53L8_CMD_STOP_STREAM  0x36
#define DEPZ_VL53L8_RPT_REG_DATA     0x91
#define DEPZ_VL53L8_RPT_FRAME        0x93

#define DEPZ_VL53L8_STREAM_CHUNK_MAX 1528u
#define DEPZ_VL53L8_STREAM_TOTAL_MAX 8192u
#define DEPZ_VL53L8_RES_4X4          16
#define DEPZ_VL53L8_RES_8X8          64
#define DEPZ_VL53L8_MAX_ZONES        64

/* Encoders (return payload length written). */
size_t depz_vl53l8_pack_read_reg(uint16_t addr, uint16_t length, uint8_t *out);  /* 4 B */
/* 2 B; 0 (nothing written) when frame_size is outside 1..8192 (contract 04 §4
 * rule 4 — the firmware refuses it too). */
size_t depz_vl53l8_pack_start_stream(uint16_t frame_size, uint8_t *out);

/* One RPT_VL53_FRAME chunk of a (possibly multi-chunk) sensor frame. */
typedef struct {
    uint64_t timestamp_us;
    uint16_t full_size;
    uint16_t offset;
    const uint8_t *data;  /* points into the report payload */
    size_t   data_len;
} depz_vl53l8_chunk;

/* Parse a RPT_VL53_FRAME payload (>=12 B). Returns 0 on success, -1 on short. */
int depz_vl53l8_unpack_chunk(const uint8_t *payload, size_t len,
                             depz_vl53l8_chunk *out);

/* RPT_REG_DATA payload: echoed opcode, u64 timestamp, register bytes. */
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us;
    const uint8_t *data;
    size_t   data_len;
} depz_vl53l8_reg_data;
int depz_vl53l8_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_vl53l8_reg_data *out); /* needs >=9 B */

/*
 * Frame reassembler (contract 04 §4): reset on offset==0; chunks must be
 * contiguous (offset == accumulated length) and agree on full_size; a gap
 * discards the frame in progress; completes when accumulated == full_size.
 * A chunk out of the rule-4 bounds (more than chunk_max bytes, full_size over
 * DEPZ_VL53L8_STREAM_TOTAL_MAX, or running past full_size) is damaged: dropped
 * and counted as a lost frame like the other damaged input.
 */
typedef struct {
    uint8_t  buf[DEPZ_VL53L8_STREAM_TOTAL_MAX];
    size_t   chunk_max;
    size_t   len;
    uint16_t full_size;
    uint64_t timestamp_us;
    uint64_t completed;
    uint64_t discarded;
} depz_vl53l8_reassembler;

/* chunk_max: the device's VL53_STREAM_FRAME_MAX — DEPZ_VL53L8_STREAM_CHUNK_MAX,
 * or DEPZ_VL53L7_STREAM_CHUNK_MAX on the I2C board (contract 11 §3). */
void depz_vl53l8_reasm_init(depz_vl53l8_reassembler *r, size_t chunk_max);
/*
 * Feed one chunk. Returns 1 when a frame completes (sets *frame -> the
 * internal buffer, *frame_len, *ts), 0 otherwise. The frame pointer stays
 * valid until the next feed.
 */
int depz_vl53l8_reasm_feed(depz_vl53l8_reassembler *r, const depz_vl53l8_chunk *c,
                           const uint8_t **frame, size_t *frame_len, uint64_t *ts);

/*
 * One decoded ranging frame. Arrays are valid for [0, resolution); zone index
 * runs row-major. Values are in the real units ST's GetRangingData gives
 * (contract 04 §5): distance_mm = raw/4 floored, a negative result clamped to
 * 0; signal/ambient = raw/2048 kcps/SPAD; range_sigma_mm = raw/128 mm;
 * reflectance = raw/2 %. ST divides integers; the floating-point fields here
 * keep the fraction.
 */
typedef struct {
    uint64_t timestamp_us;
    int      resolution;                 /* 16 | 64 (zone count present)      */
    int8_t   silicon_temp_degc;
    int32_t  distance_mm[DEPZ_VL53L8_MAX_ZONES];       /* mm, >= 0            */
    uint8_t  target_status[DEPZ_VL53L8_MAX_ZONES];
    uint8_t  nb_target_detected[DEPZ_VL53L8_MAX_ZONES];
    double   signal_per_spad[DEPZ_VL53L8_MAX_ZONES];   /* kcps/SPAD           */
    double   ambient_per_spad[DEPZ_VL53L8_MAX_ZONES];  /* kcps/SPAD           */
    uint32_t nb_spads_enabled[DEPZ_VL53L8_MAX_ZONES];
    double   range_sigma_mm[DEPZ_VL53L8_MAX_ZONES];    /* mm                  */
    double   reflectance[DEPZ_VL53L8_MAX_ZONES];       /* %                   */
} depz_vl53l8_frame;

/*
 * Decode a reassembled raw VL53L8CX ranging frame (ULD 2.1.0 layout: the
 * footer id sits at raw_len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET, 12). Returns 0
 * on success, -1 on corrupted frame (header/footer id mismatch), -2 on bad
 * length. `timestamp_us` is carried through from the reassembler.
 *
 * VL53L8CH frames come from the VL53LMZ firmware, whose footer id sits at
 * raw_len - 4: this function rejects them (-1). Decode them with
 * depz_vl53l8ch_decode_frame().
 */
int depz_vl53l8_decode_frame(const uint8_t *raw, size_t raw_len,
                             uint64_t timestamp_us, depz_vl53l8_frame *out);

/*
 * Decode a reassembled raw VL53L8CH ranging frame: the same block walk as
 * depz_vl53l8_decode_frame(), with the VL53LMZ footer id at raw_len - 4 (the
 * Python / TS / C++ SDKs use the same variant-specific offset). `cnh_out`
 * (may be NULL) receives the CNH data block when the frame carries one — the
 * bytes depz_vl53l8ch_decode_cnh() takes; *cnh_len (may be NULL) is set to its
 * length, 0 when there is none. Returns 0, -1 (id mismatch), -2 (bad length)
 * or -3 (CNH block exceeds cnh_cap).
 */
int depz_vl53l8ch_decode_frame(const uint8_t *raw, size_t raw_len,
                               uint64_t timestamp_us, depz_vl53l8_frame *out,
                               uint8_t *cnh_out, size_t cnh_cap, size_t *cnh_len);

/* ======================================================================== */
/* VL53L8CH CNH (compact-network-histogram) decode — CH-only extension.       */
/*                                                                            */
/* Byte-exact port of the ST ULD CNH plugin decode path                       */
/* (vl53lmz_cnh_get_block_addresses / _cnh_get_mem_block_addresses) for the   */
/* fixed cnh_cfg used by the DEPZ firmware (DISABLE_PING_PONG |               */
/* DISABLE_VARIANCE + ambient/xtalk/zero-invalid/ref-residual). It turns a    */
/* captured CNH data block into per-aggregate integer histograms. See the     */
/* Python reference depz_sensor_sdk/vl53l8/cnh.py (decode/_decode_aggregate). */
/* ======================================================================== */

/* MI_MAP_ID_LENGTH (max device zones mapped to aggregates, 8x8 resolution). */
#define DEPZ_VL53L8_CNH_MAX_AGGREGATES 64
/* CNH feature_length is packed as a uint8 on the device (num_bins & 0xFF).   */
#define DEPZ_VL53L8_CNH_MAX_FEATURE    255

/* Config needed to decode a CNH block: the aggregate count and per-aggregate
 * feature (bin) length that the device was configured with (must match the
 * cnh_send_config values). */
typedef struct {
    int nb_of_aggregates; /* 1..DEPZ_VL53L8_CNH_MAX_AGGREGATES */
    int feature_length;   /* 0..DEPZ_VL53L8_CNH_MAX_FEATURE (0: empty histograms) */
} depz_vl53l8ch_cnh_config;

/* Decoded CNH block: per-aggregate raw histogram and per-bin scaler. The
 * float histogram value for bin f of aggregate a is
 *   hist_raw[a][f] / 2^hist_scaler[a][f].
 * Only the first nb_aggregates rows and feature_length columns are valid.
 * This struct is ~82 KB — heap-allocate it. */
typedef struct {
    uint32_t ref_residual_word; /* raw u32; float = word / 2048.0            */
    int      nb_aggregates;
    int      feature_length;
    int32_t  hist_raw[DEPZ_VL53L8_CNH_MAX_AGGREGATES][DEPZ_VL53L8_CNH_MAX_FEATURE];
    int8_t   hist_scaler[DEPZ_VL53L8_CNH_MAX_AGGREGATES][DEPZ_VL53L8_CNH_MAX_FEATURE];
} depz_vl53l8ch_cnh_frame;

/* Decode a captured CNH data block. `raw` is the CNH block already in decode
 * order (word-swapped exactly like the standard ranging blocks). Returns 0 on
 * success; -1 on out-of-range config, -2/-3 on a raw buffer too short for the
 * header / computed layout. */
int depz_vl53l8ch_decode_cnh(const depz_vl53l8ch_cnh_config *cfg,
                             const uint8_t *raw, size_t raw_len,
                             depz_vl53l8ch_cnh_frame *out);

/* ======================================================================== */
/* VL53L8 advanced-feature DCI codecs (ST ULD, UM3109) — pure encode/decode.  */
/* These DCI codecs (xtalk margin, detection thresholds, motion config) are   */
/* SHARED by both the CX and CH variants.                                     */
/* ======================================================================== */

/* Detection-threshold measurement selectors (get divides / set multiplies). */
#define DEPZ_VL53L8_DIST_MM               1
#define DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS  2
#define DEPZ_VL53L8_RANGE_SIGMA_MM        4
#define DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS 8
#define DEPZ_VL53L8_NB_TARGET_DETECTED    9
#define DEPZ_VL53L8_TAR_STATUS            12
#define DEPZ_VL53L8_NB_SPADS_ENABLED      13
#define DEPZ_VL53L8_MOTION_INDICATOR      19

#define DEPZ_VL53L8_POWER_MODE_SLEEP      0
#define DEPZ_VL53L8_POWER_MODE_WAKEUP     1
#define DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP 2

#define DEPZ_VL53L8_NB_THRESHOLDS         64
#define DEPZ_VL53L8_THRESH_START_SIZE     (DEPZ_VL53L8_NB_THRESHOLDS * 12) /* 768 */
#define DEPZ_VL53L8_MOTION_CFG_SIZE       156

/* Largest xtalk margin depz_vl53l8_set_xtalk_margin accepts, kcps/SPAD (as ST). */
#define DEPZ_VL53L8_XTALK_MARGIN_MAX_KCPS 10000u

/* Xtalk margin, a whole number of kcps/SPAD (as ST) -> raw DCI value
 * (kcps * 2048). Refuses (-1, *raw untouched) a margin above
 * DEPZ_VL53L8_XTALK_MARGIN_MAX_KCPS, else 0. */
int      depz_vl53l8_xtalk_margin_to_raw(uint32_t margin_kcps, uint32_t *raw);
/* Inverse: raw DCI -> whole kcps/SPAD (raw / 2048, integer division as ST). */
uint32_t depz_vl53l8_xtalk_margin_from_raw(uint32_t raw);

/* One detection-threshold entry in real units (mirror of DetectionThreshold). */
typedef struct {
    int32_t low_thresh;
    int32_t high_thresh;
    uint8_t measurement;
    uint8_t type;
    uint8_t zone_num;
    uint8_t operation;
} depz_vl53l8_threshold;

/*
 * Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload
 * (768 B) plus the 8-byte valid-status block (all 0x05). Missing entries are
 * zero-filled. low/high are scaled by the entry's measurement selector.
 */
void depz_vl53l8_pack_thresholds(const depz_vl53l8_threshold *th, size_t n,
                                 uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE],
                                 uint8_t valid[8]);
/* The same, refusing (-1, nothing written) more than 64 entries or an entry
 * whose scaled low/high does not fit the i32 DCI field (contract 07 §3); 0
 * on success. The
 * unchecked form above wraps such a value to 32 bits. */
int depz_vl53l8_pack_thresholds_checked(const depz_vl53l8_threshold *th, size_t n,
                                        uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE],
                                        uint8_t valid[8]);

/*
 * Build the default 156-byte VL53L8CX_Motion_Configuration bytes that
 * motion_indicator_init programs for the given resolution (16 | 64). This is
 * the pure codec half of the motion indicator (the live DCI write is
 * depz_vl53l8_configure_motion_indicator). Returns 0 on success, -1 on bad resolution.
 */
int depz_vl53l8_motion_cfg_default_pack(int resolution,
                                        uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE]);

/* ======================================================================== */
/* VL53L5CX / VL53L7CX / VL53L7CH I2C register bridge (contract 11)           */
/*                                                                            */
/* A delta against the VL53L8 bridge above: commands 0x32/0x33/0x35/0x36 and  */
/* reports 0x91/0x93 keep the VL53L8 wire format (chunk parse, reassembler    */
/* and reg-data decode are shared — use depz_vl53l8_*), but the sensor is on  */
/* I2C with tighter transfer ceilings, adds PIN_CTRL / GET_INFO /             */
/* SET_I2C_SPEED, and its frames carry the footer id at size-4 with per-zone  */
/* arrays trimmed to the resolution. The live driver is the multizone class   */
/* of depz_sensor_io.h (models DEPZ_VL53L8_MODEL_L5CX / _L7CX / _L7CH).       */
/* ======================================================================== */

/* Which sensor an APP_VL53L7 board opens as (contract 11 §1). */
typedef enum {
    DEPZ_VL53L7_MODEL_L7CX = 0, /* base class (default)                      */
    DEPZ_VL53L7_MODEL_L5CX = 1, /* same API, 63° optics (PID 0xED48)         */
    DEPZ_VL53L7_MODEL_L7CH = 2  /* L7CX + CNH, as VL53L8CH (PID 0xED4A)      */
} depz_vl53l7_model;

#define DEPZ_VL53L7_CMD_PIN_CTRL      0x34
#define DEPZ_VL53L7_CMD_GET_INFO      0x37
#define DEPZ_VL53L7_CMD_SET_I2C_SPEED 0x38
#define DEPZ_VL53L7_RPT_INFO          0x92 /* no echoed command byte */

/* VL53_PIN_CTRL actions. None is a true sensor reset (no power GPIO): after
 * LPN_OFF or SOFT_CYCLE the host must re-run init(). */
#define DEPZ_VL53L7_PIN_LPN_OFF    0u /* stop streaming, LPn low (I2C off)    */
#define DEPZ_VL53L7_PIN_LPN_ON     1u /* LPn high (power-up default)          */
#define DEPZ_VL53L7_PIN_I2C_RST    2u /* pulse I2C_RST                        */
#define DEPZ_VL53L7_PIN_SOFT_CYCLE 3u /* stop, LPn low 1 ms, high, I2C_RST;
                                        clears the I2C error counters        */

/* RPT_VL53_INFO.last_i2c_error */
#define DEPZ_VL53L7_I2C_OK        0u
#define DEPZ_VL53L7_I2C_NACK      1u
#define DEPZ_VL53L7_I2C_TIMEOUT   2u
#define DEPZ_VL53L7_I2C_BUS_ERROR 3u

#define DEPZ_VL53L7_READ_MAX_LEN     1536u /* READ_REG len 1..1536 (L8: 2048) */
#define DEPZ_VL53L7_WRITE_MAX_LEN    2048u /* WRITE_REG N 1..2048             */
#define DEPZ_VL53L7_STREAM_CHUNK_MAX 1536u /* frame bytes per RPT_VL53_FRAME  */
#define DEPZ_VL53L7_INFO_SIZE        20u
/* Header id at [8..9] must equal the footer id at [raw_len - 4] (both the
 * l7cx and l7ch blob sets; VL53L8CX uses 12). */
#define DEPZ_VL53L7_FOOTER_ID_OFFSET 4

/* Encoders (return payload length written). READ_REG / WRITE_REG return 0
 * when the length is outside 1..READ_MAX_LEN / 1..WRITE_MAX_LEN or
 * addr + length > 0x10000 (the firmware would answer ERR_INVALID_PARAM). */
size_t depz_vl53l7_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out); /* 4 B */
size_t depz_vl53l7_pack_write_reg(uint16_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out); /* 2 + N B */
size_t depz_vl53l7_pack_pin_ctrl(uint8_t action, uint8_t *out);     /* 1 B */
size_t depz_vl53l7_pack_set_i2c_speed(uint16_t khz, uint8_t *out);  /* 2 B */

/* RPT_VL53_INFO — bridge state only (20 B, little-endian `<IIIBBBHHB`).
 * Counters run from power-up / DEVICE_RESET; SOFT_CYCLE clears the I2C ones. */
typedef struct {
    uint32_t int_edges;
    uint32_t frames_dropped;
    uint32_t i2c_errors;
    uint8_t  last_i2c_error; /* DEPZ_VL53L7_I2C_* */
    uint8_t  lpn_level;
    uint8_t  int_level;
    uint16_t i2c_khz;        /* effective SCL after SET_I2C_SPEED snapping */
    uint16_t frame_size;
    bool     streaming;
} depz_vl53l7_info;
/* Returns 0 on success, -1 when len < DEPZ_VL53L7_INFO_SIZE. */
int depz_vl53l7_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53l7_info *out);

/*
 * Class resolution (normative order, contract 11 §1): the production USB PID
 * model (`usb_model` as returned by depz_usb_model_hint(): "vl53l5cx" |
 * "vl53l7cx" | "vl53l7ch"; NULL or anything else = none), else the first
 * match of `VL53L([57])(CX|CH)` in GET_DEVICE_NAME (`device_name`, may be
 * NULL), else DEPZ_VL53L7_MODEL_L7CX.
 */
depz_vl53l7_model depz_vl53l7_resolve_model(const char *usb_model,
                                            const char *device_name);
/* The same with explicit lengths (strings may hold embedded NULs; NULL = none). */
depz_vl53l7_model depz_vl53l7_resolve_model_n(const char *usb_model, size_t usb_model_len,
                                              const char *device_name, size_t device_name_len);
const char *depz_vl53l7_model_str(depz_vl53l7_model m); /* "vl53l7cx", ... */

/*
 * Decode a reassembled VL53L5CX/L7CX/L7CH ranging frame (shared block walk
 * with depz_vl53l8_decode_frame). Differences: the footer id sits at
 * raw_len - DEPZ_VL53L7_FOOTER_ID_OFFSET, and every per-zone array is trimmed
 * to the frame's resolution — on L5/L7 the per-target blocks carry 64
 * entries even in 4x4. The resolution is read from the zone-sized ambient
 * block (index 0x54D0), so out->resolution is 16 or 64 and arrays past it
 * are zero.
 *
 * `cnh_out` (may be NULL) receives the CNH data block (VL53L7CH with CNH
 * configured) in decode order — the bytes depz_vl53l8ch_decode_cnh() takes;
 * *cnh_len (may be NULL) is set to its length, 0 when the frame has none.
 * Returns 0 on success, -1 on header/footer id mismatch, -2 on bad length,
 * -3 when the CNH block exceeds cnh_cap.
 */
int depz_vl53l7_decode_frame(const uint8_t *raw, size_t raw_len,
                             uint64_t timestamp_us, depz_vl53l8_frame *out,
                             uint8_t *cnh_out, size_t cnh_cap, size_t *cnh_len);

/* ======================================================================== */
/* VL53L4CD register bridge + host-ULD codecs (contract 10)                   */
/*                                                                            */
/* Single-zone ToF behind a thin I2C register bridge. The MCU owns nothing    */
/* but the I2C bus, XSHUT/INT and one streaming FSM; the ST ULD 2.2.3         */
/* (STSW-IMG026) semantics run on the host as plain register access. This     */
/* section carries the wire codecs (commands 0x32..0x38, reports 0x91..0x93)  */
/* plus the pure host-ULD math: the 17-byte result-block decode, the          */
/* SetRangeTiming/GetRangeTiming register math, the tuning word codecs and    */
/* the 91-byte init configuration block. All of it is frozen byte-exact in    */
/* contracts/vectors/vl53l4.json.                                             */
/* ======================================================================== */

typedef enum {
    DEPZ_VL53L4_CMD_READ_REG      = 0x32,
    DEPZ_VL53L4_CMD_WRITE_REG     = 0x33,
    DEPZ_VL53L4_CMD_XSHUT         = 0x34,
    DEPZ_VL53L4_CMD_START_STREAM  = 0x35,
    DEPZ_VL53L4_CMD_STOP_STREAM   = 0x36,
    DEPZ_VL53L4_CMD_GET_INFO      = 0x37,
    DEPZ_VL53L4_CMD_SET_I2C_SPEED = 0x38
} depz_vl53l4_cmd;

typedef enum {
    DEPZ_VL53L4_RPT_REG_DATA = 0x91,
    DEPZ_VL53L4_RPT_INFO     = 0x92,
    DEPZ_VL53L4_RPT_STREAM   = 0x93
} depz_vl53l4_rpt;

/* Max read length / write data length per transfer (STM32 I2C NBYTES is
 * 8-bit; a write spends two bytes on the register address; the firmware
 * applies one number to both directions). addr + len must be <= 0x10000. */
#define DEPZ_VL53L4_XFER_MAX 253u

/* VL53_XSHUT actions. RESET is answered after the boot handshake. */
#define DEPZ_VL53L4_XSHUT_OFF   0u
#define DEPZ_VL53L4_XSHUT_ON    1u
#define DEPZ_VL53L4_XSHUT_RESET 2u

/* VL53_START_STREAM flags bit 1: INT active high (mirrors bit 4 of
 * GPIO_HV_MUX__CTRL 0x0030). Clear (default) = INT active low. */
#define DEPZ_VL53L4_SF_INT_ACT_HIGH 0x02u

/* The usual stream configuration: the whole result block in one read. */
#define DEPZ_VL53L4_RESULT_BLOCK_ADDR 0x0089u
#define DEPZ_VL53L4_RESULT_BLOCK_LEN  17u
/* IDENTIFICATION__MODEL_ID (0x010F) expected value. */
#define DEPZ_VL53L4_MODEL_ID 0xEBAAu
/* First register of the 91-byte init configuration block (0x2D..0x87). */
#define DEPZ_VL53L4_CONFIG_ADDR 0x2Du
/* Byte 0 of the config block is always forced to 0x12 (I2C Fast Mode Plus
 * pad, never cleared) — what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD. */
#define DEPZ_VL53L4_CONFIG_FMP_BYTE 0x12u

/* Encoders (return payload length written). */
/* VL53_READ_REG payload: addr u16, len u16. Returns 4, or 0 when len is outside
 * 1..DEPZ_VL53L4_XFER_MAX or addr + len > 0x10000 (contract 10 §1). */
size_t depz_vl53l4_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out);
/* VL53_WRITE_REG payload: addr u16 + data. Returns 2 + data_len, or 0 when
 * data_len is outside 1..DEPZ_VL53L4_XFER_MAX or addr + data_len > 0x10000. */
size_t depz_vl53l4_pack_write_reg(uint16_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out);
size_t depz_vl53l4_pack_xshut(uint8_t action, uint8_t *out); /* 1 B */
size_t depz_vl53l4_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags,
                                     uint8_t *out); /* 5 B */
size_t depz_vl53l4_pack_set_i2c_speed(uint16_t khz, uint8_t *out); /* 2 B */

/* RPT_VL53_REG_DATA payload: echoed opcode, u64 timestamp, register bytes. */
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us; /* MCU uptime at I2C-read completion */
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_vl53l4_reg_data;
int depz_vl53l4_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_vl53l4_reg_data *out); /* needs >= 9 B */

/* RPT_VL53_INFO — bridge diagnostics (21 B, little-endian). Counters wrap
 * silently; watch increments, not absolute values. i2c_errors counts since the
 * last successful XSHUT reset (the bridge clears it there), not since power-up. */
typedef struct {
    uint32_t int_edges;
    uint32_t slots_skipped;
    uint32_t i2c_errors;
    uint8_t  last_i2c_error; /* 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR */
    uint16_t model_id;       /* expected DEPZ_VL53L4_MODEL_ID (0xEBAA) */
    uint8_t  fw_status;      /* expected 0x03 (booted) */
    uint8_t  initialized;    /* 1 = MODEL_ID read back as 0xEBAA on this GET_INFO */
    uint8_t  xshut_level;
    uint8_t  int_level;
    uint16_t i2c_khz;
} depz_vl53l4_info;
int depz_vl53l4_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53l4_info *out);

/* RPT_VL53_STREAM — one streamed register block. `addr`/`len` echo the
 * stream configuration so each report is self-describing. */
typedef struct {
    uint64_t timestamp_us; /* MCU uptime at the INT edge (the sensor event) */
    uint16_t addr;
    uint16_t len;
    const uint8_t *data;   /* points into the report payload (len bytes) */
} depz_vl53l4_stream;
int depz_vl53l4_unpack_stream(const uint8_t *payload, size_t len,
                              depz_vl53l4_stream *out);

/* VL53L4CD_ResultsData_t plus the sensor's own frame counter. */
typedef struct {
    int range_status;          /* 0 = valid; raw >= 24 passes through unmapped */
    int distance_mm;
    int ambient_rate_kcps;
    int ambient_per_spad_kcps;
    int signal_rate_kcps;
    int signal_per_spad_kcps;
    int number_of_spad;
    int sigma_mm;
    int stream_count;          /* RESULT__STREAM_COUNT, wraps at 255 */
} depz_vl53l4_result;

/*
 * Decode the streamed 17-byte 0x0089..0x0099 block exactly as
 * VL53L4CD_GetResult() decodes the same registers read one by one. Register
 * contents are big-endian words (the bridge passes them through untouched).
 * Returns 0 on success, -1 when len < DEPZ_VL53L4_RESULT_BLOCK_LEN (17); a
 * longer buffer is decoded and the tail ignored (contract 07 §3).
 */
int depz_vl53l4_parse_result_block(const uint8_t *raw, size_t len,
                                   depz_vl53l4_result *out);

/*
 * SetRangeTiming register math -> RANGE_CONFIG_A (0x005E), RANGE_CONFIG_B
 * (0x0061) and the INTERMEASUREMENT_MS (0x006C) raw dword. `osc_frequency` is
 * the word read from 0x0006; `clock_pll` is the word read from
 * RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
 * inter_ms > 0). inter_ms == 0 selects continuous mode; a value greater than
 * the budget selects autonomous low power. Returns 0 on success, -1 when
 * osc_frequency == 0, budget_ms outside 10..200, 0 < inter_ms <= budget_ms,
 * or an INTERMEASUREMENT_MS value that would overflow u32.
 */
int depz_vl53l4_range_timing_registers(uint32_t budget_ms, uint32_t inter_ms,
                                       uint16_t osc_frequency, uint16_t clock_pll,
                                       uint16_t *range_config_a,
                                       uint16_t *range_config_b,
                                       uint32_t *intermeasurement_raw);

/*
 * GetRangeTiming register math -> (budget_ms, inter_ms) from the raw register
 * reads: the INTERMEASUREMENT_MS dword, the RESULT__OSC_CALIBRATE_VAL word,
 * the 0x0006 word and the RANGE_CONFIG_A word. Returns 0 on success, -1 when
 * osc_frequency == 0.
 */
int depz_vl53l4_decode_range_timing(uint32_t intermeasurement_raw,
                                    uint16_t clock_pll, uint16_t osc_frequency,
                                    uint16_t range_config_a,
                                    uint32_t *budget_ms, uint32_t *inter_ms);

/* Tuning word codecs (register word <-> user units). */
uint16_t depz_vl53l4_offset_raw(int32_t mm);       /* RANGE_OFFSET_MM: mm*4 */
int32_t  depz_vl53l4_decode_offset(uint16_t raw);  /* -> signed millimetres */
uint16_t depz_vl53l4_xtalk_raw(uint16_t kcps);     /* XTALK_PLANE_OFFSET: kcps*512 */
/* Range-checked forms (contract 07 §3 — prefer these; the unchecked ones
 * above wrap): offset -1024..1023 mm (the 11-bit word decode_offset reads
 * back), xtalk 0..127 kcps (fits the word after << 9). 0, or -1 with *raw
 * untouched. */
int      depz_vl53l4_offset_raw_checked(int32_t mm, uint16_t *raw);
int      depz_vl53l4_xtalk_raw_checked(uint16_t kcps, uint16_t *raw);
uint16_t depz_vl53l4_decode_xtalk(uint16_t raw);   /* lround(raw/512.0) */
uint16_t depz_vl53l4_signal_threshold_raw(uint16_t kcps);    /* kcps/8 */
uint16_t depz_vl53l4_decode_signal_threshold(uint16_t raw);  /* raw*8 */
/* RANGE_CONFIG__SIGMA_THRESH: mm*4. Returns 0 on success, -1 when mm > 16383. */
int      depz_vl53l4_sigma_threshold_raw(uint16_t mm, uint16_t *raw);
uint16_t depz_vl53l4_decode_sigma_threshold(uint16_t raw);   /* raw/4 */

/* VL53L4CD_DEFAULT_CONFIGURATION[] — the stock ST 91-byte block for registers
 * 0x2D..0x87 (byte 0 as shipped, i.e. NOT the FM+ override). */
extern const uint8_t DEPZ_VL53L4_DEFAULT_CONFIGURATION[91];

/* Write the 91-byte block sensor_init() sends at DEPZ_VL53L4_CONFIG_ADDR: the
 * ST default configuration with byte 0 forced to DEPZ_VL53L4_CONFIG_FMP_BYTE
 * (I2C Fast Mode Plus). Returns 91. */
size_t depz_vl53l4_config_block(uint8_t *out);

/* ======================================================================== */
/* VL53L 1D family on the APP_VL53L0_4 bridge, protocol v2.00 (contract 12)  */
/*                                                                            */
/* One bridge firmware serves VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD */
/* and VL53L4CX; every ULD runs on the host. A delta against contract 10:     */
/* READ_REG / WRITE_REG / XSHUT / STOP_STREAM / SET_I2C_SPEED and the         */
/* REG_DATA / STREAM reports are the depz_vl53l4_* codecs unchanged. New here:*/
/* SET_ADDR_WIDTH, START_STREAM with its interrupt-release (clear) list, the  */
/* 23-byte RPT_VL53_INFO, the product table + class resolution, and the three */
/* stateless block decoders ("base" level — no live driver). All of it is     */
/* frozen in contracts/vectors/vl53lx.json.                                   */
/* ======================================================================== */

#define DEPZ_VL53LX_CMD_READ_REG       0x32 /* = DEPZ_VL53L4_CMD_READ_REG      */
#define DEPZ_VL53LX_CMD_WRITE_REG      0x33
#define DEPZ_VL53LX_CMD_XSHUT          0x34 /* RESET: no boot handshake (v2) */
#define DEPZ_VL53LX_CMD_START_STREAM   0x35
#define DEPZ_VL53LX_CMD_STOP_STREAM    0x36
#define DEPZ_VL53LX_CMD_GET_INFO       0x37
#define DEPZ_VL53LX_CMD_SET_I2C_SPEED  0x38
#define DEPZ_VL53LX_CMD_SET_ADDR_WIDTH 0x39 /* new in v2.00                  */
#define DEPZ_VL53LX_CMD_CLEAR_I2C_ERRORS 0x3A /* v2.01 (fw v0.24), no payload;
                                                sent after every sensor init */
#define DEPZ_VL53LX_RPT_REG_DATA       0x91
#define DEPZ_VL53LX_RPT_INFO           0x92
#define DEPZ_VL53LX_RPT_STREAM         0x93

#define DEPZ_VL53LX_CLEAR_STEPS_MAX 4u  /* interrupt-release steps per stream */
#define DEPZ_VL53LX_START_STREAM_MAX (6u + 3u * DEPZ_VL53LX_CLEAR_STEPS_MAX)
#define DEPZ_VL53LX_INFO_SIZE 23u

/* One interrupt-release write the bridge plays after every block read. */
typedef struct {
    uint16_t addr;
    uint8_t  value;
} depz_vl53lx_clear_step;

/* VL53_SET_ADDR_WIDTH payload (1 B). Returns 0 when width is not 1 or 2. */
size_t depz_vl53lx_pack_set_addr_width(uint8_t width, uint8_t *out);
/* VL53_START_STREAM payload: addr u16, len u16, flags u8, n_clear u8, then
 * n_clear x {addr u16, value u8} — 6 + 3n bytes (out needs
 * DEPZ_VL53LX_START_STREAM_MAX). Returns 0 when n_clear > 4. `clear` may be
 * NULL when n_clear == 0. flags: DEPZ_VL53L4_SF_INT_ACT_HIGH as contract 10. */
size_t depz_vl53lx_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags,
                                     const depz_vl53lx_clear_step *clear,
                                     size_t n_clear, uint8_t *out);

/* RPT_VL53_INFO v2.00 (0x92, 23 B `<IIIBBBHBBI`) — bridge state only. */
typedef struct {
    uint32_t int_edges;
    uint32_t slots_skipped;  /* reset at START_STREAM */
    uint32_t i2c_errors;     /* free-running */
    uint8_t  last_i2c_error; /* 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR */
    uint8_t  xshut_level;
    uint8_t  int_level;
    uint16_t i2c_khz;
    uint8_t  addr_width;     /* 1 or 2 */
    uint8_t  n_clear;        /* clear steps of the armed stream */
    uint32_t frames_dropped; /* reset at START_STREAM */
} depz_vl53lx_info;
/* Returns 0 on success, -1 when len < DEPZ_VL53LX_INFO_SIZE. */
int depz_vl53lx_unpack_info(const uint8_t *payload, size_t len,
                            depz_vl53lx_info *out);

/* ---- product table (contract 12 §1) ------------------------------------- */

typedef enum {
    DEPZ_VL53LX_PRODUCT_NONE = -1, /* unknown / unstamped board             */
    DEPZ_VL53LX_PRODUCT_L0X = 0,
    DEPZ_VL53LX_PRODUCT_L1CX,
    DEPZ_VL53LX_PRODUCT_L1CB,
    DEPZ_VL53LX_PRODUCT_L3CX,
    DEPZ_VL53LX_PRODUCT_L4CD,
    DEPZ_VL53LX_PRODUCT_L4CX,
    DEPZ_VL53LX_PRODUCT_COUNT      /* table order = UI order                */
} depz_vl53lx_product;

/* Driver kinds, as a bitmask in depz_vl53lx_product_info.driver_kinds. */
typedef enum {
    DEPZ_VL53LX_DRIVER_ULD       = 1 << 0, /* ST Ultra Lite Driver           */
    DEPZ_VL53LX_DRIVER_ULP       = 1 << 1, /* Ultra Low Power (L3CX only)    */
    DEPZ_VL53LX_DRIVER_HISTOGRAM = 1 << 2  /* Bare Driver: 24 bins, host     */
} depz_vl53lx_driver;

typedef struct {
    const char *name;           /* "VL53L0X", ...                          */
    uint16_t usb_pid;           /* production PID                          */
    uint16_t model_id;          /* cross-check only (L1CX=L1CB, L4CD=L4CX) */
    uint32_t reach_mm;          /* datasheet rating                        */
    unsigned driver_kinds;      /* OR of depz_vl53lx_driver                */
    depz_vl53lx_driver default_driver;
    /* bridge parameters of the default driver: */
    uint8_t  addr_width;        /* VL53_SET_ADDR_WIDTH                     */
    uint8_t  n_clear;
    depz_vl53lx_clear_step clear[2];
    uint16_t max_khz;           /* bus ceiling after init (inits at 400)   */
} depz_vl53lx_product_info;

/* Row of the table, or NULL for NONE / out of range. */
const depz_vl53lx_product_info *depz_vl53lx_product_get(depz_vl53lx_product p);
/* "uld" | "ulp" | "histogram" (NULL for anything else). */
const char *depz_vl53lx_driver_str(depz_vl53lx_driver d);
/* Exact product name, case-insensitive ("vl53l4cx" -> L4CX), else NONE. */
depz_vl53lx_product depz_vl53lx_product_from_str(const char *name);
/* Product named on the board (GET_DEVICE_NAME): the first
 * `VL53L<digit>[A-Z0-9]*` in the upper-cased name, NONE when there is no
 * match or the match is not a family product. `name` may be NULL. */
depz_vl53lx_product depz_vl53lx_product_from_board_name(const char *name);
depz_vl53lx_product depz_vl53lx_product_from_board_name_n(const char *name, size_t len);
/* The PID model (`usb_model` as from depz_usb_model_hint(), may be NULL) if
 * it names a family product, else depz_vl53lx_product_from_board_name(). */
depz_vl53lx_product depz_vl53lx_resolve_product(const char *usb_model,
                                                const char *device_name);
depz_vl53lx_product depz_vl53lx_resolve_product_n(const char *usb_model, size_t usb_model_len,
                                                  const char *device_name, size_t device_name_len);

/* Which class an APP_VL53L0_4 board opens as (contract 12 §1). VL53L4CD has
 * no class of its own here (its vl53l4cd class belongs to APP_VL53L4): it,
 * and an unresolved product, open as the generic class. */
typedef enum {
    DEPZ_VL53LX_CLASS_GENERIC = 0, /* "vl53lx": takes the product at init   */
    DEPZ_VL53LX_CLASS_L0X,
    DEPZ_VL53LX_CLASS_L1CX,
    DEPZ_VL53LX_CLASS_L1CB,
    DEPZ_VL53LX_CLASS_L3CX,
    DEPZ_VL53LX_CLASS_L4CX
} depz_vl53lx_class;

depz_vl53lx_class depz_vl53lx_resolve_class(const char *usb_model,
                                            const char *device_name);
/* _n variants: explicit lengths, strings may hold embedded NULs (NULL = none). */
depz_vl53lx_class depz_vl53lx_resolve_class_n(const char *usb_model, size_t usb_model_len,
                                              const char *device_name, size_t device_name_len);
const char *depz_vl53lx_class_str(depz_vl53lx_class c); /* "vl53lx", "vl53l0x", ... */

/* ---- stateless block decoders (contract 12 §4) -------------------------- */

#define DEPZ_VL53LX_DIE_BLOCK_ADDR       0x0089u
#define DEPZ_VL53LX_DIE_BLOCK_LEN        17u
#define DEPZ_VL53LX_L0X_BLOCK_ADDR       0x14u
#define DEPZ_VL53LX_L0X_BLOCK_LEN        12u
#define DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR 0x0088u
#define DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN  83u
#define DEPZ_VL53LX_HISTOGRAM_BINS       24u

/* Which ULD reads the 17-byte die block: (signal offset, per-SPAD K). */
typedef enum {
    DEPZ_VL53LX_DIE_L4 = 0, /* VL53L4CD ULD, L3CX ULP: (5, 256) — exactly
                             * depz_vl53l4_parse_result_block()           */
    DEPZ_VL53LX_DIE_L1 = 1  /* VL53L1X ULD: (15, 25), crosstalk-corrected
                             * peak signal at 0x0098                      */
} depz_vl53lx_die_variant;

/* Decode the die block (0x0089..0x0099) as `variant` reads it. Returns 0 on
 * success, -1 when len < 17 or the variant is unknown. */
int depz_vl53lx_decode_die_block(const uint8_t *raw, size_t len,
                                 depz_vl53lx_die_variant variant,
                                 depz_vl53l4_result *out);

/* Raw fields of the VL53L0X 12-byte block at 0x14. The PAL range status,
 * sigma and dmax need the device data cached by init — full driver only. */
typedef struct {
    uint16_t distance_raw;            /* mm (quarter-mm if RangeFractional) */
    uint8_t  device_range_status;     /* raw byte 0                         */
    uint32_t signal_rate_mcps_1616;   /* FixPoint16.16 Mcps (wire 9.7 << 9) */
    uint32_t ambient_rate_mcps_1616;
    uint16_t effective_spad_count_88; /* 8.8                                */
} depz_vl53lx_l0x_raw;
/* Returns 0 on success, -1 when len < 12. */
int depz_vl53lx_decode_l0x_raw(const uint8_t *raw, size_t len,
                               depz_vl53lx_l0x_raw *out);

/* Status bytes and the 24 photon bins of the 83-byte histogram block at
 * 0x0088. Turning bins into targets is the full driver's job. */
typedef struct {
    uint8_t  interrupt_status;
    uint8_t  range_status;
    uint8_t  report_status;
    uint8_t  stream_count;
    uint16_t dss_actual_effective_spads;
    uint16_t reference_phase;
    uint8_t  vcsel_start;
    uint32_t bins[DEPZ_VL53LX_HISTOGRAM_BINS]; /* 24-bit counts */
} depz_vl53lx_histogram_raw;
/* Bin 23's low byte is rebuilt from its MSB/LSB pair ((MSB << 2) + LSB,
 * truncated to 8 bits) before the bins are read; `raw` is not modified.
 * Returns 0 on success, -1 when len < 83. */
int depz_vl53lx_decode_histogram_raw(const uint8_t *raw, size_t len,
                                     depz_vl53lx_histogram_raw *out);

/* ======================================================================== */
/* BNO055 9-axis IMU register bridge, protocol v0.10 (contract 13)           */
/*                                                                            */
/* APP_BNO055 is a thin register bridge: the MCU owns I2C (0x28, 400 kHz),    */
/* the reset pin and one streaming loop; Bosch's fusion runs on the chip.     */
/* Mode, units, axis remap and calibration are host logic over register      */
/* access. "Base" level: wire codecs, the pure §4 codecs and window decode to */
/* raw integers (scaling is value = raw / LSB, §4.2). All of it is frozen in  */
/* contracts/vectors/bno055.json.                                             */
/* ======================================================================== */

#define DEPZ_BNO055_CMD_READ_REG     0x32
#define DEPZ_BNO055_CMD_WRITE_REG    0x33
#define DEPZ_BNO055_CMD_RESET        0x34 /* deferred reply, allow >= 1.5 s   */
#define DEPZ_BNO055_CMD_START_STREAM 0x35
#define DEPZ_BNO055_CMD_STOP_STREAM  0x36
#define DEPZ_BNO055_CMD_GET_INFO     0x37
#define DEPZ_BNO055_RPT_REG_DATA     0x91
#define DEPZ_BNO055_RPT_INFO         0x92
#define DEPZ_BNO055_RPT_STREAM       0x93

/* Max bytes per READ_REG / WRITE_REG / streamed block; addr + len <= 0x100. */
#define DEPZ_BNO055_XFER_MAX 128u
#define DEPZ_BNO055_INFO_SIZE 38u

/* BNO_START_STREAM trigger. Data-ready interrupts do not exist on sensor SW
 * 03.11: TIMER is the only data trigger; INT is for motion interrupts, and
 * then period_ms is a missed-edge watchdog (0 disables it). */
#define DEPZ_BNO055_TRIGGER_TIMER 0u
#define DEPZ_BNO055_TRIGGER_INT   1u

/* Encoders (return payload length written). RESET / STOP_STREAM / GET_INFO
 * have an empty payload. */
/* BNO_READ_REG payload: addr u8, len u8. Returns 2, or 0 when len is outside
 * 1..DEPZ_BNO055_XFER_MAX or addr + len > 0x100 (contract 13 §1). */
size_t depz_bno055_pack_read_reg(uint8_t addr, uint8_t len, uint8_t *out);
/* BNO_WRITE_REG payload: addr u8 + data. Returns 1 + data_len, or 0 when
 * data_len is outside 1..DEPZ_BNO055_XFER_MAX or addr + data_len > 0x100. */
size_t depz_bno055_pack_write_reg(uint8_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out);
/* BNO_START_STREAM TIMER period limits, ms (contract 13 §2). */
#define DEPZ_BNO055_TIMER_PERIOD_MIN_MS 1u
#define DEPZ_BNO055_TIMER_PERIOD_MAX_MS 60000u
/* BNO_START_STREAM payload: trigger u8, addr u8, len u8, period_ms u16.
 * Returns 5, or 0 when trigger is TIMER and period_ms is outside 1..60000
 * (with INT the period is the missed-edge watchdog, 0 = none). */
size_t depz_bno055_pack_start_stream(uint8_t trigger, uint8_t addr, uint8_t len,
                                     uint16_t period_ms, uint8_t *out); /* 5 B */

/* RPT_BNO_REG_DATA (0x91): echoed opcode, u64 timestamp, register bytes. */
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us; /* MCU uptime at I2C-read completion */
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_bno055_reg_data;
int depz_bno055_unpack_reg_data(const uint8_t *payload, size_t len,
                                depz_bno055_reg_data *out); /* needs >= 9 B */

/* RPT_BNO_INFO (0x92, 38 B `<BBBBBHBBBIHHHIIIHBBH`): sensor identity
 * (registers 0x00..0x06) plus bridge diagnostics. Counters wrap; read_*_us,
 * slots_skipped, loop_max_us reset at START_STREAM, i2c_errors at every
 * successful reset handshake (not free-running). A
 * rising sensor_resets means the sensor is back in CONFIG: re-configure. */
typedef struct {
    uint8_t  i2c_addr;       /* 0x28 */
    uint8_t  chip_id;        /* healthy 0xA0 */
    uint8_t  acc_id;         /* 0xFB */
    uint8_t  mag_id;         /* 0x32 */
    uint8_t  gyr_id;         /* 0x0F */
    uint16_t sw_rev;         /* BCD: 0x0311 = 03.11 */
    uint8_t  bl_rev;
    uint8_t  initialized;    /* 1 = chip-ID handshake passed */
    uint8_t  int_level;
    uint32_t int_edges;
    uint16_t read_min_us;
    uint16_t read_max_us;
    uint16_t read_avg_us;
    uint32_t tx_dropped;
    uint32_t i2c_errors;
    uint32_t slots_skipped;
    uint16_t bus_recoveries;
    uint8_t  last_i2c_error; /* 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR */
    uint8_t  sensor_resets;
    uint16_t loop_max_us;
} depz_bno055_info;
/* Returns 0 on success, -1 when len < DEPZ_BNO055_INFO_SIZE. */
int depz_bno055_unpack_info(const uint8_t *payload, size_t len,
                            depz_bno055_info *out);

/* RPT_BNO_REG_STREAM (0x93): one streamed register block; addr/len echo the
 * stream configuration. timestamp_us is the trigger time, not I2C completion. */
typedef struct {
    uint64_t timestamp_us;
    uint8_t  addr;
    uint8_t  len;
    const uint8_t *data;   /* points into the report payload (len bytes) */
} depz_bno055_stream;
/* Returns 0 on success, -1 when len < 10 + the echoed block length. */
int depz_bno055_unpack_stream(const uint8_t *payload, size_t len,
                              depz_bno055_stream *out);

/* ---- register map essentials (§4.1; page 0 unless REG1) ------------------ */
#define DEPZ_BNO055_REG_CHIP_ID          0x00u
#define DEPZ_BNO055_REG_PAGE_ID          0x07u /* host-owned; always back to 0 */
#define DEPZ_BNO055_REG_ACC_DATA         0x08u /* 3 x i16 LE (x, y, z)       */
#define DEPZ_BNO055_REG_MAG_DATA         0x0Eu
#define DEPZ_BNO055_REG_GYR_DATA         0x14u
#define DEPZ_BNO055_REG_EUL_DATA         0x1Au /* heading, roll, pitch        */
#define DEPZ_BNO055_REG_QUA_DATA         0x20u /* 4 x i16: w, x, y, z         */
#define DEPZ_BNO055_REG_LIA_DATA         0x28u /* linear accel (no gravity)   */
#define DEPZ_BNO055_REG_GRV_DATA         0x2Eu
#define DEPZ_BNO055_REG_TEMP             0x34u /* i8                          */
#define DEPZ_BNO055_REG_CALIB_STAT       0x35u
#define DEPZ_BNO055_REG_ST_RESULT        0x36u
#define DEPZ_BNO055_REG_INT_STA          0x37u /* clears on read              */
#define DEPZ_BNO055_REG_SYS_CLK_STATUS   0x38u
#define DEPZ_BNO055_REG_SYS_STATUS       0x39u
#define DEPZ_BNO055_REG_SYS_ERR          0x3Au
#define DEPZ_BNO055_REG_UNIT_SEL         0x3Bu
#define DEPZ_BNO055_REG_OPR_MODE         0x3Du /* bits 3:0                    */
#define DEPZ_BNO055_REG_PWR_MODE         0x3Eu
#define DEPZ_BNO055_REG_SYS_TRIGGER      0x3Fu
#define DEPZ_BNO055_REG_TEMP_SOURCE      0x40u
#define DEPZ_BNO055_REG_AXIS_MAP_CONFIG  0x41u
#define DEPZ_BNO055_REG_AXIS_MAP_SIGN    0x42u
#define DEPZ_BNO055_REG_SIC_MATRIX       0x43u /* 9 x i16, 1.0 = 16384        */
#define DEPZ_BNO055_REG_CALIB_PROFILE    0x55u /* 22 B, CONFIG mode only      */
#define DEPZ_BNO055_REG1_ACC_CONFIG      0x08u
#define DEPZ_BNO055_REG1_MAG_CONFIG      0x09u
#define DEPZ_BNO055_REG1_GYR_CONFIG_0    0x0Au
#define DEPZ_BNO055_REG1_GYR_CONFIG_1    0x0Bu
#define DEPZ_BNO055_REG1_INT_MSK         0x0Fu
#define DEPZ_BNO055_REG1_INT_EN          0x10u
#define DEPZ_BNO055_REG1_UNIQUE_ID       0x50u /* 16 B                        */

/* Every output channel in one read (0x08..0x35), and the quaternion alone. */
#define DEPZ_BNO055_FULL_BLOCK_ADDR 0x08u
#define DEPZ_BNO055_FULL_BLOCK_LEN  46u
#define DEPZ_BNO055_QUAT_BLOCK_ADDR 0x20u
#define DEPZ_BNO055_QUAT_BLOCK_LEN  8u

/* OPR_MODE values; >= IMU are fusion modes. */
typedef enum {
    DEPZ_BNO055_MODE_CONFIG = 0x00,
    DEPZ_BNO055_MODE_ACCONLY, DEPZ_BNO055_MODE_MAGONLY, DEPZ_BNO055_MODE_GYROONLY,
    DEPZ_BNO055_MODE_ACCMAG, DEPZ_BNO055_MODE_ACCGYRO, DEPZ_BNO055_MODE_MAGGYRO,
    DEPZ_BNO055_MODE_AMG,
    DEPZ_BNO055_MODE_IMU = 0x08,
    DEPZ_BNO055_MODE_COMPASS, DEPZ_BNO055_MODE_M4G, DEPZ_BNO055_MODE_NDOF_FMC_OFF,
    DEPZ_BNO055_MODE_NDOF = 0x0C
} depz_bno055_opr_mode;

/* ---- units (UNIT_SEL 0x3B, §4.2 — as the silicon implements them) -------- */
#define DEPZ_BNO055_UNIT_ACC_MG      0x01u /* ACC_DATA in mg, else m/s^2     */
#define DEPZ_BNO055_UNIT_GYR_RPS     0x02u /* rad/s, else dps                */
#define DEPZ_BNO055_UNIT_EUL_RAD     0x04u /* radians, else degrees          */
#define DEPZ_BNO055_UNIT_TEMP_F      0x10u /* deg F (1 LSB = 2 F), else C    */
#define DEPZ_BNO055_UNIT_ORI_ANDROID 0x80u /* power-on value is 0x80         */

typedef struct {
    bool accel_mg;
    bool gyro_rps;
    bool euler_rad;
    bool temp_f;
    bool android;
} depz_bno055_units;
/* Other UNIT_SEL bits are ignored (they do nothing on the sensor). */
void    depz_bno055_unpack_units(uint8_t unit_sel, depz_bno055_units *out);
uint8_t depz_bno055_pack_units(const depz_bno055_units *u);
/* LSB per unit: value = raw / lsb. accel 100 (m/s^2) or 1 (mg); gyro 16
 * (dps) or 900 (rps); euler 16 (deg) or 900 (rad); temp 1 (C) or 0.5 (F). */
double depz_bno055_accel_lsb(const depz_bno055_units *u);
double depz_bno055_gyro_lsb(const depz_bno055_units *u);
double depz_bno055_euler_lsb(const depz_bno055_units *u);
double depz_bno055_temp_lsb(const depz_bno055_units *u);
#define DEPZ_BNO055_MAG_LSB          16.0    /* uT, fixed                    */
#define DEPZ_BNO055_QUAT_LSB         16384.0 /* 2^14, unit-less              */
/* LIA and GRV ignore the ACC_Unit bit: always m/s^2 at 100 LSB (measured). */
#define DEPZ_BNO055_FUSION_ACCEL_LSB 100.0

/* ---- calibration (§4.3) -------------------------------------------------- */

/* CALIB_STAT (0x35): sys<7:6> gyr<5:4> acc<3:2> mag<1:0>, 0..3 each. */
typedef struct {
    uint8_t system;
    uint8_t gyro;
    uint8_t accel;
    uint8_t mag;
} depz_bno055_calib_status;
void    depz_bno055_unpack_calib_status(uint8_t value, depz_bno055_calib_status *out);
uint8_t depz_bno055_pack_calib_status(const depz_bno055_calib_status *s);
/* 3/3/3/3. */
bool    depz_bno055_fully_calibrated(const depz_bno055_calib_status *s);

#define DEPZ_BNO055_CALIB_PROFILE_LEN 22u
/* Offsets and radii at 0x55..0x6A, 11 x i16 LE, sensor LSB. Read/write only
 * in CONFIG, all 22 bytes in one transfer. */
typedef struct {
    int16_t accel_offset[3];
    int16_t mag_offset[3];
    int16_t gyro_offset[3];
    int16_t accel_radius;
    int16_t mag_radius;
} depz_bno055_calib_profile;
/* Returns 0 on success, -1 when len != DEPZ_BNO055_CALIB_PROFILE_LEN. */
int    depz_bno055_unpack_calib_profile(const uint8_t *data, size_t len,
                                        depz_bno055_calib_profile *out);
size_t depz_bno055_pack_calib_profile(const depz_bno055_calib_profile *p,
                                      uint8_t *out); /* 22 B */

/* ---- axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42, §4.4) -------- */
#define DEPZ_BNO055_AXIS_X 0u
#define DEPZ_BNO055_AXIS_Y 1u
#define DEPZ_BNO055_AXIS_Z 2u

/* Which chip axis feeds each output axis (x = AXIS_Y: output X is chip Y). */
typedef struct {
    uint8_t x, y, z;
    bool x_negative, y_negative, z_negative;
} depz_bno055_axis_remap;
/* config = z<5:4> y<3:2> x<1:0>; sign = x 2, y 1, z 0 (1 = negative). */
void depz_bno055_unpack_axis_remap(uint8_t config, uint8_t sign,
                                   depz_bno055_axis_remap *out);
/* Returns 0 and writes both bytes, or -1 (nothing written) when x/y/z is not
 * a permutation of 0/1/2 — the sensor would silently keep the old value. */
int  depz_bno055_pack_axis_remap(const depz_bno055_axis_remap *a,
                                 uint8_t *config, uint8_t *sign);
/* Datasheet §3.4 placements P0..P7 as {AXIS_MAP_CONFIG, AXIS_MAP_SIGN};
 * P1 (0x24/0x00) is the power-on default. */
extern const uint8_t DEPZ_BNO055_PLACEMENTS[8][2];
/* "P0".."P7" (case-insensitive). Returns 0, or -1 for any other name/NULL. */
int  depz_bno055_placement(const char *name, depz_bno055_axis_remap *out);

/* ---- page-1 sensor configuration (non-fusion modes only) ------------------ */

/* ACC_CONFIG (p1 0x08): range<1:0> (2/4/8/16 g), bandwidth<4:2>
 * (7.81..1000 Hz), power<7:5>. Power-on 0x0D = 4 g, 62.5 Hz, normal. */
typedef struct { uint8_t range, bandwidth, power; } depz_bno055_accel_config;
void    depz_bno055_unpack_accel_config(uint8_t value, depz_bno055_accel_config *out);
uint8_t depz_bno055_pack_accel_config(const depz_bno055_accel_config *c);

/* GYR_CONFIG_0/1 (p1 0x0A/0x0B): byte 0 range<2:0> (2000..125 dps),
 * bandwidth<5:3>; byte 1 power<2:0>. Power-on 0x38/0x00. */
typedef struct { uint8_t range, bandwidth, power; } depz_bno055_gyro_config;
void   depz_bno055_unpack_gyro_config(const uint8_t bytes[2], depz_bno055_gyro_config *out);
/* The same from a buffer of `len` bytes: -1 when shorter than 2 (contract 07
 * §3), else 0; trailing bytes are ignored. */
int    depz_bno055_unpack_gyro_config_n(const uint8_t *bytes, size_t len, depz_bno055_gyro_config *out);
size_t depz_bno055_pack_gyro_config(const depz_bno055_gyro_config *c,
                                    uint8_t out[2]); /* 2 B */

/* MAG_CONFIG (p1 0x09): rate<2:0> (2..30 Hz), mode<4:3>, power<6:5>; bit 7
 * is not a field (a repack drops it). Power-on 0x0B = 10 Hz, regular, normal. */
typedef struct { uint8_t rate, mode, power; } depz_bno055_mag_config;
void    depz_bno055_unpack_mag_config(uint8_t value, depz_bno055_mag_config *out);
uint8_t depz_bno055_pack_mag_config(const depz_bno055_mag_config *c);

/* ---- register-window decode (§4.1) --------------------------------------- */

/* Raw register values found in one block read. A channel is present
 * (has_* true) only when the window addr..addr+len covers all its bytes. */
typedef struct {
    bool    has_accel, has_mag, has_gyro, has_euler, has_quaternion,
            has_linear_accel, has_gravity, has_temperature, has_calib_stat;
    int16_t accel[3];
    int16_t mag[3];
    int16_t gyro[3];
    int16_t euler[3];        /* heading, roll, pitch */
    int16_t quaternion[4];   /* w, x, y, z */
    int16_t linear_accel[3];
    int16_t gravity[3];
    int8_t  temperature;
    uint8_t calib_stat;      /* CALIB_STAT byte */
} depz_bno055_block;
/* Unpack whatever channels the window starting at `addr` holds (data may be
 * NULL when len == 0). Absent channels are zeroed. */
void depz_bno055_decode_block(uint8_t addr, const uint8_t *data, size_t len,
                              depz_bno055_block *out);

/* ======================================================================== */
/* TMF8829 multizone dToF over the SPI register bridge, v1.00 (contract 14)  */
/*                                                                            */
/* APP_TMF8829 is a thin SPI register bridge: it reads self-sized frames off */
/* the sensor FIFO on INT and pushes them in chunks. The host runs the       */
/* sensor (live layer: depz_tmf8829_* in depz_sensor_io.h). Here: the wire   */
/* codecs, the chunk reassembler, frame / pixel decode with the half-frame   */
/* merge, the clock model and capture time, measurement assembly, presets    */
/* and config-page fields, and the embedded ams sensor firmware. All of it   */
/* is frozen in contracts/vectors/tmf8829.json and tmf8829_frame_time.json.  */
/* ======================================================================== */

#define DEPZ_TMF8829_CMD_READ_REG     0x32
#define DEPZ_TMF8829_CMD_WRITE_REG    0x33
#define DEPZ_TMF8829_CMD_START_STREAM 0x35
#define DEPZ_TMF8829_CMD_STOP_STREAM  0x36
#define DEPZ_TMF8829_CMD_GET_INFO     0x37
#define DEPZ_TMF8829_RPT_REG_DATA     0x91
#define DEPZ_TMF8829_RPT_INFO         0x92 /* no echoed cmd byte */
#define DEPZ_TMF8829_RPT_FRAME        0x93 /* unsolicited, chunked */
#define DEPZ_TMF8829_RPT_EVENT        0x94 /* unsolicited */

/* RPT_TMF_EVENT.code */
#define DEPZ_TMF8829_EV_INT        0u /* MOTION/PROXIMITY interrupt, no frame  */
#define DEPZ_TMF8829_EV_BAD_HEADER 1u /* header refused, frame not read: once
                                       * expected on STOP mid histogram dump  */

#define DEPZ_TMF8829_READ_MAX  1024u /* TMF_READ_REG len 1..1024             */
#define DEPZ_TMF8829_WRITE_MAX 1023u /* TMF_WRITE_REG data 1..1023           */
#define DEPZ_TMF8829_CHUNK_MAX 1024u /* frame bytes per RPT_TMF_FRAME        */

/* INT_STATUS bits the bridge serves (TMF_START_STREAM int_mask). */
#define DEPZ_TMF8829_INT_RESULTS    0x01u
#define DEPZ_TMF8829_INT_MOTION     0x02u
#define DEPZ_TMF8829_INT_PROXIMITY  0x04u
#define DEPZ_TMF8829_INT_HISTOGRAMS 0x08u
#define DEPZ_TMF8829_INT_MASK_ALL   0x0Fu
/* TMF_START_STREAM flags: the 60-byte ref-SPAD frame before each results frame. */
#define DEPZ_TMF8829_SF_REF_FRAME   0x01u

#define DEPZ_TMF8829_REG_DATA_HEADER_SIZE    9u
#define DEPZ_TMF8829_FRAME_CHUNK_HEADER_SIZE 21u
#define DEPZ_TMF8829_EVENT_SIZE              12u
#define DEPZ_TMF8829_INFO_SIZE               25u

/* Encoders: the payload length written, or 0 (nothing written) when an
 * argument is out of range — addr 0..255, len 1..READ_MAX, data 1..WRITE_MAX
 * bytes, int_mask a non-zero subset of 0x0F, flags only SF_REF_FRAME.
 * STOP_STREAM and GET_INFO have an empty payload. */
size_t depz_tmf8829_pack_read_reg(int addr, int len, uint8_t out[3]);
/* `out` holds 1 + data_len bytes. */
size_t depz_tmf8829_pack_write_reg(int addr, const uint8_t *data, size_t data_len,
                                   uint8_t *out);
size_t depz_tmf8829_pack_start_stream(int int_mask, int flags, uint8_t out[2]);

/* RPT_TMF_REG_DATA (0x91): echoed 0x32, MCU time at SPI-read completion,
 * the register bytes. Returns 0, or -1 when shorter than 9 B or the echoed
 * opcode is not 0x32. */
typedef struct {
    uint64_t timestamp_us;
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_tmf8829_reg_data;
int depz_tmf8829_unpack_reg_data(const uint8_t *payload, size_t len,
                                 depz_tmf8829_reg_data *out);

/* RPT_TMF_INFO (0x92, 25 B): bridge state only. int_edges runs from MCU
 * power-up; the frame counters restart at TMF_START_STREAM. -1 when short. */
typedef struct {
    uint32_t int_edges;
    uint32_t frames;
    uint32_t frames_aborted;
    uint32_t frame_errors;
    uint32_t spi_timeouts;
    uint8_t  int_level;        /* INT pin now, 0 = asserted */
    bool     streaming;
    uint8_t  int_mask;
    uint8_t  last_int_status;
    uint8_t  last_fifo_status;
} depz_tmf8829_info;
int depz_tmf8829_unpack_info(const uint8_t *payload, size_t len, depz_tmf8829_info *out);

/* RPT_TMF_FRAME (0x93) chunk. timestamp_us + the frame's pre-header SYSTICK
 * are a clock pair (not the capture time); edge_us is the INT falling edge
 * credited to the frame, 0 = none. -1 when shorter than 21 B. */
typedef struct {
    uint64_t timestamp_us;
    uint16_t full_size;
    uint16_t offset;
    uint8_t  frame_seq;
    uint64_t edge_us;
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_tmf8829_frame_chunk;
int depz_tmf8829_unpack_frame_chunk(const uint8_t *payload, size_t len,
                                    depz_tmf8829_frame_chunk *out);

/* RPT_TMF_EVENT (0x94, 12 B). int_status is INT_STATUS as read (unmasked);
 * frame_id / fifo_status are the refused header's (BAD_HEADER), else 0. */
typedef struct {
    uint64_t timestamp_us;
    uint8_t  int_status;
    uint8_t  code;         /* DEPZ_TMF8829_EV_* */
    uint8_t  frame_id;
    uint8_t  fifo_status;
} depz_tmf8829_event;
int depz_tmf8829_unpack_event(const uint8_t *payload, size_t len, depz_tmf8829_event *out);

/* ---- chunk reassembly (§2, normative) ------------------------------------ */

/* Largest frame a u16 full_size can announce. */
#define DEPZ_TMF8829_FRAME_MAX 65535u

/* A reassembled frame: the sensor bytes from register 0xFA on. */
typedef struct {
    uint64_t timestamp_us;
    uint64_t edge_us;
    uint8_t  frame_seq;
    const uint8_t *data;   /* points into the reassembler */
    size_t   len;
} depz_tmf8829_raw_frame;

/* Reset on offset 0 (an unfinished frame counts as discarded); append a
 * chunk only when its frame_seq and full_size equal the frame's and its
 * offset equals the bytes held, anything else discards the frame in
 * progress; complete at full_size, discarded past it. Never stitches chunks
 * of different frame_seq. seq_gaps counts frames the bridge dropped. ~64 KB:
 * allocate it, don't put it on a small stack. */
typedef struct {
    uint64_t completed;
    uint64_t discarded;
    uint64_t seq_gaps;
    /* private */
    bool     has_head, has_next_seq;
    uint8_t  next_seq;
    uint64_t head_timestamp_us, head_edge_us;
    uint16_t head_full_size;
    uint8_t  head_seq;
    size_t   len;
    uint8_t  buf[DEPZ_TMF8829_FRAME_MAX];
} depz_tmf8829_reassembler;

/* Forget everything (every stream start: frame_seq restarts at 0). */
void depz_tmf8829_reassembler_reset(depz_tmf8829_reassembler *r);
/* Returns 1 and fills *out when `chunk` completes a frame (out->data stays
 * valid until the next feed / reset), else 0. */
int  depz_tmf8829_reassembler_feed(depz_tmf8829_reassembler *r,
                                   const depz_tmf8829_frame_chunk *chunk,
                                   depz_tmf8829_raw_frame *out);

/* ---- frames (§3) ---------------------------------------------------------- */

#define DEPZ_TMF8829_PRE_HEADER_SIZE 5u
#define DEPZ_TMF8829_HEADER_SIZE     16u
#define DEPZ_TMF8829_FOOTER_SIZE     12u
#define DEPZ_TMF8829_FRAME_HEAD      21u    /* what the bridge reads @0xFA */
#define DEPZ_TMF8829_FRAME_EOF       0xE0F7u

#define DEPZ_TMF8829_FID_MASK       0xF0u
#define DEPZ_TMF8829_FID_RESULTS    0x10u
#define DEPZ_TMF8829_FID_HISTOGRAMS 0x20u
#define DEPZ_TMF8829_FID_REF_SPAD   0x30u

#define DEPZ_TMF8829_LAYOUT_PEAKS    0x07u
#define DEPZ_TMF8829_LAYOUT_SIGNAL   0x08u
#define DEPZ_TMF8829_LAYOUT_NOISE    0x10u
#define DEPZ_TMF8829_LAYOUT_XTALK    0x20u
#define DEPZ_TMF8829_LAYOUT_SUBFRAME 0x40u /* second (odd-row) half-frame */

#define DEPZ_TMF8829_FRAME_VALID                0x01u
#define DEPZ_TMF8829_FRAME_ABORTED              0xC0u
#define DEPZ_TMF8829_FRAME_HV_CP_OVERLOAD       0x08u
#define DEPZ_TMF8829_FRAME_VCDRV_OVERLOAD       0x10u
#define DEPZ_TMF8829_FRAME_VCDRV_BURST_EXCEEDED 0x20u
#define DEPZ_TMF8829_FOOTER_DIST_IN_MM          0x01u /* else 0.25 mm units */
#define DEPZ_TMF8829_FIFO_TX_UNDERRUN           0x10u
#define DEPZ_TMF8829_FIFO_RX_OVERRUN            0x20u

/* Pre-header, header and footer of one frame (any kind). */
typedef struct {
    uint8_t  fifo_status;
    uint32_t systick;          /* sensor SYSTICK, 125 kHz */
    uint8_t  id;
    uint8_t  layout;
    uint16_t payload;
    uint32_t f_number;
    int8_t   temperature[3];
    uint8_t  bdv;
    uint16_t ref_pos[2];
    uint32_t t0_integration;   /* SYSTICK at the first sub-integration */
    uint32_t t1_integration;   /* ... and at the last one */
    uint8_t  frame_status;
    uint8_t  reserved;
    uint16_t eof;
    size_t   size;             /* bytes of the whole frame */
    /* derived */
    uint8_t  kind;             /* id & 0xF0: DEPZ_TMF8829_FID_* */
    uint8_t  fp_mode;          /* id & 0x0F */
    bool     second_half;      /* layout & SUBFRAME */
    bool     distance_in_mm;   /* reserved & FOOTER_DIST_IN_MM */
} depz_tmf8829_frame_info;

/* Returns 0, or -1 when the frame is shorter than header + footer (33 B). */
int depz_tmf8829_parse_frame(const uint8_t *raw, size_t len, depz_tmf8829_frame_info *out);
/* Full frame size (pre-header included) from its first 21 bytes. */
size_t depz_tmf8829_frame_size(const uint8_t head[21]);

/* fp_mode -> full focal-plane grid (rows x cols): 8x8, 8x8, 16x16, 32x32,
 * 32x32, 32x48. -1 for an unknown fp_mode. */
int depz_tmf8829_fp_grid(unsigned fp_mode, unsigned *rows, unsigned *cols);
/* What one results frame carries: half the rows for 32x32 / 48x32. */
int depz_tmf8829_frame_rows(unsigned fp_mode, unsigned *rows, unsigned *cols);
/* "8x8A", "8x8B", "16x16", "32x32", "32x32s", "48x32"; NULL if unknown. */
const char *depz_tmf8829_fp_name(unsigned fp_mode);

/* What is wrong with a frame, in the reference wording ("EOF 0x1234",
 * "status 0x00 not valid", "aborted", "HV_CP_OVERLOAD", "VCDRV_OVERLOAD",
 * "VCDRV_BURST_EXCEEDED", "txfifo_underrun", "rxfifo_overrun"); count 0 =
 * fine. */
#define DEPZ_TMF8829_FRAME_PROBLEMS_MAX 8
typedef struct {
    int  count;
    char text[DEPZ_TMF8829_FRAME_PROBLEMS_MAX][32];
} depz_tmf8829_frame_problems;
void depz_tmf8829_frame_problems_of(const depz_tmf8829_frame_info *f,
                                    depz_tmf8829_frame_problems *out);

/* ---- results pixels and the half-frame merge (§3) -------------------------- */

#define DEPZ_TMF8829_MAX_ZONES 1536u /* 48 x 32 */
#define DEPZ_TMF8829_MAX_PEAKS 4u    /* nr_peaks the sensor supports */

/* Decoded results of one frame or of a merged measurement, raw sensor
 * integers (distance in 0.25 mm, or mm with the footer flag; not yet
 * clock-corrected). Per-peak arrays are [peaks][rows][cols] packed densely:
 * index p * rows * cols + r * cols + c; noise / xtalk are [rows][cols].
 * distance and snr are present when peaks > 0. ~37 KB. */
typedef struct {
    uint8_t  peaks;
    uint16_t rows, cols;
    bool     has_signal, has_noise, has_xtalk;
    uint16_t distance[DEPZ_TMF8829_MAX_PEAKS * DEPZ_TMF8829_MAX_ZONES];
    uint8_t  snr[DEPZ_TMF8829_MAX_PEAKS * DEPZ_TMF8829_MAX_ZONES];
    uint16_t signal[DEPZ_TMF8829_MAX_PEAKS * DEPZ_TMF8829_MAX_ZONES];
    uint16_t noise[DEPZ_TMF8829_MAX_ZONES];
    uint16_t xtalk[DEPZ_TMF8829_MAX_ZONES];
} depz_tmf8829_results;

/* Pixel results of one results frame, row-major. `info` may be NULL (the
 * frame is parsed). Returns 0, or -1 with a reason in `err` (optional): not a
 * results frame, unknown fp_mode, more than MAX_PEAKS peaks, or data size !=
 * pixels x pixel size (a corrupt frame). */
int depz_tmf8829_parse_results(const uint8_t *raw, size_t len,
                               const depz_tmf8829_frame_info *info,
                               depz_tmf8829_results *out, char *err, size_t err_cap);
/* 32x32 / 48x32: the first half-frame holds the even rows, the second the
 * odd ones; full[2i] = first[i], full[2i+1] = second[i]. A field is present
 * when both halves have it. Returns 0, or -1 (out untouched) when the halves
 * differ in peaks or shape. `out` must not alias an input. */
int depz_tmf8829_merge_halves(const depz_tmf8829_results *first,
                              const depz_tmf8829_results *second,
                              depz_tmf8829_results *out);

/* ---- clock model and capture time (§5, bit-exact) -------------------------- */

#define DEPZ_TMF8829_NOMINAL_Q16  (8u << 16) /* 8 MCU µs per tick, Q16 */
#define DEPZ_TMF8829_WIN_MIN_US   200000
#define DEPZ_TMF8829_WIN_SLIDE_US 1000000
#define DEPZ_TMF8829_EDGE_LATE_US 2000

/* Frame-time flags (FT_*). */
#define DEPZ_TMF8829_FT_NOMINAL     0x01u /* ratio not measured yet            */
#define DEPZ_TMF8829_FT_CLOCK_RESET 0x02u /* model restarted at this frame     */
#define DEPZ_TMF8829_FT_EDGE_LATE   0x04u /* INT edge > 2 ms later than usual  */
#define DEPZ_TMF8829_FT_STALLED     0x08u /* sub-integrations not back to back */
#define DEPZ_TMF8829_FT_T1_UNSURE   0x10u /* histograms + 32x32/48x32          */

/* Sensor SYSTICK -> MCU µs, fed one clock pair per non-ref frame (MCU time of
 * the header read, pre-header SYSTICK). Ratio over a sliding 1..2 s window,
 * first after 200 ms; it is also the distance correction. */
typedef struct {
    uint32_t ratio_q16;        /* MCU µs per tick, Q16 */
    bool     nominal;          /* ratio_q16 not measured yet */
    /* private */
    bool     valid, has_mid;
    int64_t  last_us, anchor_us, mid_us;
    uint32_t last_tick, anchor_tick, mid_tick;
} depz_tmf8829_clock;

void    depz_tmf8829_clock_reset(depz_tmf8829_clock *c);
/* Add a pair -> DEPZ_TMF8829_FT_CLOCK_RESET when it does not fit the model
 * (which then restarts from it), else 0. */
uint8_t depz_tmf8829_clock_add(depz_tmf8829_clock *c, int64_t us, uint32_t tick);
/* The same, refusing (-1, clock untouched) a negative MCU time — it is a u64
 * (contract 07 §3); else 0 with the flags in *flags. */
int     depz_tmf8829_clock_add_checked(depz_tmf8829_clock *c, int64_t us, uint32_t tick, uint8_t *flags);
/* SYSTICK `tick` -> MCU µs through the pair (pair_us, pair_tick). */
int64_t depz_tmf8829_clock_to_us(const depz_tmf8829_clock *c, int64_t pair_us,
                                 uint32_t pair_tick, uint32_t tick);
/* MCU/sensor clock ratio (~1.015): distances are scaled by it. */
double  depz_tmf8829_clock_ratio(const depz_tmf8829_clock *c);

/* Capture time of one results frame, MCU µs. */
typedef struct {
    int64_t  timestamp_us;     /* centre of the capture window */
    uint8_t  flags;            /* DEPZ_TMF8829_FT_* */
    uint32_t t1_len_us;        /* sub-integration length used */
    int64_t  t0_us, t1_us;     /* first / last sub-integration start */
    int64_t  edge_us;          /* INT edge credited to the frame, 0 = none */
    int64_t  pair_us;          /* the clock pair's MCU time */
    uint32_t ratio_q16;        /* clock ratio after this frame */
} depz_tmf8829_frame_time;

/* N equal sub-integrations per results frame; window [t0, t1 + T1] with T1
 * the stream's shortest (t1 - t0) / (N - 1); its centre is the frame time. */
typedef struct {
    /* private */
    bool     has_lag_min, hist_seen;
    int64_t  lag_min;
    uint32_t sub_min;
} depz_tmf8829_timer;

void depz_tmf8829_timer_reset(depz_tmf8829_timer *t);
/* A histogram frame was seen in the stream. */
void depz_tmf8829_timer_histograms(depz_tmf8829_timer *t);
/* `flags` = what clock_add returned for this frame. */
void depz_tmf8829_timer_results(depz_tmf8829_timer *t, const depz_tmf8829_clock *c,
                                unsigned fp_mode, uint32_t systick, uint32_t t0_tick,
                                uint32_t t1_tick, int64_t pair_us, int64_t edge_us,
                                uint8_t flags, depz_tmf8829_frame_time *out);
/* The same, refusing (-1, nothing changed) fp_mode > 255 or a negative
 * pair_us / edge_us (contract 07 §3); else 0. */
int  depz_tmf8829_timer_results_checked(depz_tmf8829_timer *t, const depz_tmf8829_clock *c,
                                        unsigned fp_mode, uint32_t systick, uint32_t t0_tick,
                                        uint32_t t1_tick, int64_t pair_us, int64_t edge_us,
                                        uint8_t flags, depz_tmf8829_frame_time *out);
/* Sub-integrations per results frame: fp_mode 0..2 -> 2, 3..4 -> 4, 5 -> 6,
 * any other -> 2. */
unsigned depz_tmf8829_subints(unsigned fp_mode);

/* ---- measurements (§6) ------------------------------------------------------ */

#define DEPZ_TMF8829_PROBLEMS_KEPT 16
#define DEPZ_TMF8829_PROBLEM_LEN   128

/* One measurement: its results frame (8x8, 16x16) or both half-frames
 * (32x32, 48x32) merged, with the histogram / ref-SPAD frames before it
 * counted (their bytes reach the live layer's frame callback). ~90 KB. */
typedef struct {
    int64_t  timestamp_us;     /* MCU µs: floor mean of the half-frame times */
    uint8_t  flags;            /* FT_* of all its results frames, OR-ed */
    uint8_t  fp_mode;
    uint32_t f_number;
    uint16_t rows, cols;       /* full focal plane */
    int8_t   temperature_c;    /* die temperature (last frame, header byte 0) */
    bool     distance_in_mm;   /* raw distance unit is mm (8x8-xr), else 0.25 mm */
    uint32_t ratio_q16;        /* clock ratio used for distance_mm */
    depz_tmf8829_results results;
    /* clock-corrected mm, [peaks][rows][cols] like results.distance; valid
     * when results.peaks > 0 */
    double   distance_mm[DEPZ_TMF8829_MAX_PEAKS * DEPZ_TMF8829_MAX_ZONES];
    size_t   n_frames;         /* 1 or 2 */
    depz_tmf8829_frame_info frames[2];
    depz_tmf8829_frame_time times[2];
    size_t   n_histograms;
    size_t   n_ref_frames;
    /* what went wrong since the previous measurement: n_problems in all, the
     * first DEPZ_TMF8829_PROBLEMS_KEPT of them as text */
    size_t   n_problems;
    char     problems[DEPZ_TMF8829_PROBLEMS_KEPT][DEPZ_TMF8829_PROBLEM_LEN];
} depz_tmf8829_measurement;

/* Frames in stream order -> measurements. Histogram frames between two
 * results frames belong to the second; a ref-SPAD frame comes right before
 * its results frame. 32x32/48x32: a second half without a first is dropped
 * with the frames collected for it; two first halves in a row keep the newer;
 * halves of different fp_mode / layout are dropped with their frames. Every
 * case is logged in the next measurement's problems. Reset at every stream
 * start (clock model and T1 are per stream). ~80 KB: allocate it. */
typedef struct {
    depz_tmf8829_clock clock;
    depz_tmf8829_timer timer;
    uint64_t parse_errors;
    /* private */
    size_t   pending_hist, pending_ref, hist, refs;
    bool     has_half;
    depz_tmf8829_frame_info half_info;
    depz_tmf8829_frame_time half_time;
    depz_tmf8829_results    half;
    depz_tmf8829_results    cur;
    size_t   n_problems;
    char     problems[DEPZ_TMF8829_PROBLEMS_KEPT][DEPZ_TMF8829_PROBLEM_LEN];
} depz_tmf8829_builder;

void depz_tmf8829_builder_reset(depz_tmf8829_builder *b);
/* One frame with the MCU time of its header read and its INT edge (0 =
 * none). Returns 1 and fills *out when it completes a measurement, else 0;
 * -1 (nothing changed) when timestamp_us or edge_us is negative — they are
 * u64 MCU times (contract 07 §3). */
int  depz_tmf8829_builder_add(depz_tmf8829_builder *b, const uint8_t *raw, size_t len,
                              int64_t timestamp_us, int64_t edge_us,
                              depz_tmf8829_measurement *out);

/* ---- sensor registers, presets, config page (§4) --------------------------- */

#define DEPZ_TMF8829_REG_APP_ID     0x00u /* bootloader 0x80, application 0x01 */
#define DEPZ_TMF8829_REG_CMD_STAT   0x08u
#define DEPZ_TMF8829_REG_SERIAL     0x1Cu /* u32 */
#define DEPZ_TMF8829_REG_CID_RID    0x20u
#define DEPZ_TMF8829_REG_CFG        0x22u /* config page 0x22..0xDF */
#define DEPZ_TMF8829_REG_INT_STATUS 0xE1u
#define DEPZ_TMF8829_REG_INT_ENAB   0xE2u
#define DEPZ_TMF8829_REG_ID         0xE3u /* reads 0x9E */
#define DEPZ_TMF8829_REG_REVID      0xE4u
#define DEPZ_TMF8829_REG_RESET      0xF7u
#define DEPZ_TMF8829_REG_ENABLE     0xF8u
#define DEPZ_TMF8829_REG_FIFOSTATUS 0xFAu
#define DEPZ_TMF8829_REG_FIFO       0xFFu /* the address stays here */
#define DEPZ_TMF8829_DEVICE_ID      0x9Eu
#define DEPZ_TMF8829_CFG_SIZE       190u

/* Application commands (CMD_STAT) and statuses. */
#define DEPZ_TMF8829_CMD_MEASURE          0x10u
#define DEPZ_TMF8829_CMD_WRITE_PAGE       0x15u
#define DEPZ_TMF8829_CMD_LOAD_CONFIG_PAGE 0x16u
#define DEPZ_TMF8829_CMD_STOP             0xFFu
#define DEPZ_TMF8829_STAT_OK       0x00u
#define DEPZ_TMF8829_STAT_ACCEPTED 0x01u
/* "OK", "ERR_CONFIG_RESULT_SIZE", ...; NULL for an unknown status. */
const char *depz_tmf8829_status_name(uint8_t status);

/* Presets: the load command (then LOAD_CONFIG_PAGE) and the preset's own
 * kilo_iterations (a preset rescales a stale value, it does not reset it). */
typedef struct {
    const char *name;          /* "8x8", "8x8-lr", ..., "8x8-xr" */
    uint8_t     cmd;           /* 0x40..0x49 */
    uint16_t    kilo_iterations;
} depz_tmf8829_preset;
#define DEPZ_TMF8829_PRESET_COUNT 10
extern const depz_tmf8829_preset DEPZ_TMF8829_PRESETS[DEPZ_TMF8829_PRESET_COUNT];
/* NULL for an unknown name. */
const depz_tmf8829_preset *depz_tmf8829_find_preset(const char *name);

#define DEPZ_TMF8829_DEFAULT_PERIOD_MS 33 /* the sensor's own after init() */

/* Config-page fields: register, bit shift, width (16 = little-endian pair). */
typedef struct {
    const char *name;
    uint8_t     reg, shift, width;
} depz_tmf8829_cfg_field;
#define DEPZ_TMF8829_CFG_FIELD_COUNT 14
extern const depz_tmf8829_cfg_field DEPZ_TMF8829_CFG_FIELDS[DEPZ_TMF8829_CFG_FIELD_COUNT];
/* NULL for an unknown name. */
const depz_tmf8829_cfg_field *depz_tmf8829_find_cfg_field(const char *name);
/* Read a field of a 190-byte page. 0, or -1 for an unknown name. */
int depz_tmf8829_cfg_get(const uint8_t page[190], const char *name, uint32_t *value);
/* Set a field. 0; -1 unknown name; -2 value outside 0 .. 2^width - 1 (page
 * untouched). */
int depz_tmf8829_cfg_set(uint8_t page[190], const char *name, int64_t value);
/* The same over a page of `page_len` bytes (a partial page read is fine as
 * long as it holds the field): -3 when it is too short for the field. */
int depz_tmf8829_cfg_get_n(const uint8_t *page, size_t page_len, const char *name, uint32_t *value);
int depz_tmf8829_cfg_set_n(uint8_t *page, size_t page_len, const char *name, int64_t value);

/* ---- sensor firmware ---------------------------------------------------------- */

/* ams tmf8829_application.hex as bytes (application 2.200.0, MIT — see
 * licenses/tmf8829/LICENSES-MIT.TXT shipped with this SDK), 14 444 B, a
 * multiple of 4, downloaded by depz_tmf8829_init(). */
#define DEPZ_TMF8829_FIRMWARE_LOAD_ADDR 0x00010000u
extern const uint8_t depz_tmf8829_firmware[];
extern const size_t  depz_tmf8829_firmware_len;

/* ======================================================================== */
/* BNO086 SHTP framing (contract 05 §3) + SH-2 control encoders (§6)          */
/* ======================================================================== */

/* Bridge commands and report (contract 05 §1). SEND_SHTP_PACKET is answered
 * with RPT_STATUS at once (OK, or ERR_BUSY: retry after >= 200 ms); every
 * inbound SHTP frame arrives as RPT_DATA with cmd 0 (ERRATA E2). */
#define DEPZ_BNO086_CMD_SENSOR_RESET      0x32
#define DEPZ_BNO086_CMD_SENSOR_WAKE_UP    0x33
#define DEPZ_BNO086_CMD_SEND_SHTP_PACKET  0x34
#define DEPZ_BNO086_RPT_DATA              0x91

/* RPT_DATA payload: cmd u8, capture timestamp u64 (MCU µs), SHTP frame.
 * Returns 0 (the frame points into `p`), -1 when shorter than 9 bytes. */
int depz_bno086_unpack_data(const uint8_t *p, size_t len, uint64_t *capture_us,
                            const uint8_t **shtp, size_t *shtp_len);

#define DEPZ_SHTP_HEADER_SIZE   4
#define DEPZ_SHTP_LENGTH_MASK   0x7FFFu
#define DEPZ_SHTP_CONTINUATION  0x8000u
#define DEPZ_SHTP_NUM_CHANNELS  6
#define DEPZ_SHTP_MAX_TX_FRAME  64      /* one MCU transmit slot (ERRATA E2) */

typedef enum {
    DEPZ_SHTP_CH_COMMAND     = 0,
    DEPZ_SHTP_CH_EXECUTABLE  = 1,
    DEPZ_SHTP_CH_CONTROL     = 2,
    DEPZ_SHTP_CH_INPUT_NORMAL = 3,
    DEPZ_SHTP_CH_INPUT_WAKE  = 4,
    DEPZ_SHTP_CH_GYRO_RV     = 5
} depz_shtp_channel_id;

/* One-byte host writes on the executable channel (contract 05 §6a); the hub
 * answers none of them. RESET restarts the hub (the 0x01 reset-complete
 * follows on the same channel); SLEEP stops every report except those enabled
 * with DEPZ_BNO_FEATURE_WAKEUP | DEPZ_BNO_FEATURE_ALWAYS_ON, keeping the
 * configuration; ON ends SLEEP and every report resumes at its rate. */
#define DEPZ_SHTP_EXEC_RESET 0x01
#define DEPZ_SHTP_EXEC_ON    0x02
#define DEPZ_SHTP_EXEC_SLEEP 0x03

typedef struct {
    uint16_t length;      /* cargo length incl. this 4-byte header */
    uint8_t  channel;
    uint8_t  seq;
    bool     continuation;
} depz_shtp_header;

/* Pack a header. `length` is a 15-bit field: depz_shtp_pack_header_checked
 * returns -1 and writes nothing when it is > 0x7FFF (contract 07 §3), else 0.
 * depz_shtp_pack_header (kept for compatibility) cannot report that and
 * masks the length to 15 bits — prefer the checked form. */
int depz_shtp_pack_header_checked(const depz_shtp_header *hdr, uint8_t out[4]);
void depz_shtp_pack_header(const depz_shtp_header *hdr, uint8_t out[4]);
void depz_shtp_unpack_header(const uint8_t in[4], depz_shtp_header *out);

/* Per-channel TX sequence counters + RX cargo reassembly. */
typedef struct {
    uint8_t *buf;
    size_t   received;
    size_t   expected;
    size_t   cap;
    uint8_t  seq;
    bool     active;
    uint64_t capture_us;  /* capture of the fragment that started the cargo */
} depz_shtp_rx_channel;

typedef struct {
    depz_shtp_rx_channel rx[DEPZ_SHTP_NUM_CHANNELS];
    uint8_t  tx_seq[DEPZ_SHTP_NUM_CHANNELS];
    uint64_t discarded;   /* incomplete/orphan cargos and channel >= 6 frames thrown away */
} depz_shtp_layer;

void depz_shtp_init(depz_shtp_layer *l);
void depz_shtp_free(depz_shtp_layer *l);

/* Build one single-fragment TX frame, consuming the channel's TX seq. Returns
 * frame length, or 0 on error (bad channel / payload exceeds the MCU slot /
 * capacity). */
size_t depz_shtp_next_frame(depz_shtp_layer *l, uint8_t channel,
                            const uint8_t *payload, size_t payload_len,
                            uint8_t *out, size_t out_cap);

/* One reassembled cargo (payload excludes all SHTP headers). */
typedef struct {
    uint8_t  channel;
    uint8_t  seq;         /* first fragment's seq */
    const uint8_t *payload;
    size_t   payload_len;
    uint64_t capture_us;  /* first fragment's capture (contract 05 §3); 0 via depz_shtp_feed */
} depz_shtp_cargo;

/* Feed one inbound frame. Returns 1 when a cargo completes (fills *out, whose
 * payload points into the channel buffer, valid until the next feed on that
 * channel), 0 otherwise. Returns -1 on allocation failure. Same as
 * depz_shtp_feed_ex with capture_us = 0. */
int depz_shtp_feed(depz_shtp_layer *l, const uint8_t *frame, size_t frame_len,
                   depz_shtp_cargo *out);

/* depz_shtp_feed with the frame's RPT_DATA capture time. The completed cargo
 * carries the FIRST fragment's capture in out->capture_us — the base its 0xFB
 * timestamps refer to (contract 05 §3, §5). */
int depz_shtp_feed_ex(depz_shtp_layer *l, const uint8_t *frame, size_t frame_len,
                      uint64_t capture_us, depz_shtp_cargo *out);

/* --- SH-2 control message encoders (return payload length written) -------- */

/* SET_FEATURE_COMMAND 0xFD, 17 B. */
size_t depz_bno_pack_set_feature(uint8_t sensor_id, uint8_t flags,
                                 uint16_t sensitivity, uint32_t interval_us,
                                 uint32_t batch_us, uint32_t cfg_word,
                                 uint8_t out[17]);
/* GET_FEATURE_REQUEST 0xFE, 2 B. */
size_t depz_bno_pack_get_feature_request(uint8_t sensor_id, uint8_t out[2]);
/* PRODUCT_ID_REQUEST 0xF9, 2 B. */
size_t depz_bno_pack_product_id_request(uint8_t out[2]);
/* COMMAND_REQUEST 0xF2, 12 B (params zero-padded, up to 9 B). Returns 12, or
 * 0 (nothing written) when nparams > 9. */
size_t depz_bno_pack_command_request(uint8_t seq, uint8_t command,
                                     const uint8_t *params, size_t nparams,
                                     uint8_t out[12]);
/* FRS_READ_REQUEST 0xF4, 8 B. */
size_t depz_bno_pack_frs_read_request(uint16_t frs_type, uint16_t offset_words,
                                      uint16_t block_words, uint8_t out[8]);
/* FRS_WRITE_REQUEST 0xF7, 6 B. */
size_t depz_bno_pack_frs_write_request(uint16_t frs_type, uint16_t length_words,
                                       uint8_t out[6]);
/* FRS_WRITE_DATA 0xF6, always 12 B (contract 05 §6): `nwords` is 1 or 2, and
 * a single word is followed by a zero word1. Returns 12, or 0 when nwords is
 * not 1 or 2 or out_cap < 12. */
size_t depz_bno_pack_frs_write_data(uint16_t offset_words, const uint32_t *words,
                                    size_t nwords, uint8_t *out, size_t out_cap);
/* Interactive Calibration: COMMAND_REQUEST 0xF2 with command 0x0E and P0 =
 * `intent` (DEPZ_BNO_MOTION_INTENT_*), 12 B. BNO086 only, no response (SH-2
 * §6.4.12); a BNO085 ignores it. */
size_t depz_bno_pack_interactive_calibration(uint8_t seq, uint8_t intent, uint8_t out[12]);
/* FORCE_SENSOR_FLUSH 0xF0 + sensor id, 2 B (SH-2 §6.5.6): the hub sends the
 * sensor's batched reports now and answers with the header-less Flush
 * Completed (DEPZ_BNO_FLUSH_COMPLETED) on the input channel. */
size_t depz_bno_pack_force_flush(uint8_t sensor_id, uint8_t out[2]);

/* --- SH-2 control message parsers (contract 05 §6) ------------------------ */

/* SH-2 sensor ids (the input report ids) — what Set Feature takes, and the
 * `sensor_id` of a report. Not the depz_bno_report_type catalog. */
#define DEPZ_BNO_SENSOR_ACCELEROMETER            0x01u
#define DEPZ_BNO_SENSOR_GYROSCOPE                0x02u
#define DEPZ_BNO_SENSOR_MAGNETOMETER             0x03u
#define DEPZ_BNO_SENSOR_LINEAR_ACCELERATION      0x04u
#define DEPZ_BNO_SENSOR_ROTATION_VECTOR          0x05u
#define DEPZ_BNO_SENSOR_GRAVITY                  0x06u
#define DEPZ_BNO_SENSOR_UNCALIBRATED_GYROSCOPE   0x07u
#define DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR     0x08u
#define DEPZ_BNO_SENSOR_GEOMAGNETIC_ROTATION_VECTOR 0x09u
#define DEPZ_BNO_SENSOR_UNCALIBRATED_MAGNETOMETER 0x0Fu
#define DEPZ_BNO_SENSOR_TAP_DETECTOR             0x10u
#define DEPZ_BNO_SENSOR_STEP_COUNTER             0x11u
#define DEPZ_BNO_SENSOR_SIGNIFICANT_MOTION       0x12u
#define DEPZ_BNO_SENSOR_STABILITY_CLASSIFIER     0x13u
#define DEPZ_BNO_SENSOR_RAW_ACCELEROMETER        0x14u
#define DEPZ_BNO_SENSOR_RAW_GYROSCOPE            0x15u
#define DEPZ_BNO_SENSOR_RAW_MAGNETOMETER         0x16u
#define DEPZ_BNO_SENSOR_STEP_DETECTOR            0x18u
#define DEPZ_BNO_SENSOR_SHAKE_DETECTOR           0x19u
#define DEPZ_BNO_SENSOR_FLIP_DETECTOR            0x1Au
#define DEPZ_BNO_SENSOR_PICKUP_DETECTOR          0x1Bu
#define DEPZ_BNO_SENSOR_STABILITY_DETECTOR       0x1Cu
#define DEPZ_BNO_SENSOR_PERSONAL_ACTIVITY_CLASSIFIER 0x1Eu
#define DEPZ_BNO_SENSOR_SLEEP_DETECTOR           0x1Fu
#define DEPZ_BNO_SENSOR_TILT_DETECTOR            0x20u
#define DEPZ_BNO_SENSOR_POCKET_DETECTOR          0x21u
#define DEPZ_BNO_SENSOR_CIRCLE_DETECTOR          0x22u
#define DEPZ_BNO_SENSOR_HEART_RATE_MONITOR       0x23u
#define DEPZ_BNO_SENSOR_ARVR_STABILIZED_RV       0x28u
#define DEPZ_BNO_SENSOR_ARVR_STABILIZED_GAME_RV  0x29u
#define DEPZ_BNO_SENSOR_GYRO_INTEGRATED_RV       0x2Au
#define DEPZ_BNO_SENSOR_MOTION_REQUEST           0x2Bu /* interactive calibration, BNO086 only */

/* Control-channel report ids. */
#define DEPZ_SH2_COMMAND_RESPONSE     0xF1
#define DEPZ_SH2_COMMAND_REQUEST      0xF2
#define DEPZ_SH2_FRS_READ_RESPONSE    0xF3
#define DEPZ_SH2_FRS_READ_REQUEST     0xF4
#define DEPZ_SH2_FRS_WRITE_RESPONSE   0xF5
#define DEPZ_SH2_FRS_WRITE_DATA       0xF6
#define DEPZ_SH2_FRS_WRITE_REQUEST    0xF7
#define DEPZ_SH2_PRODUCT_ID_RESPONSE  0xF8
#define DEPZ_SH2_PRODUCT_ID_REQUEST   0xF9
#define DEPZ_SH2_GET_FEATURE_RESPONSE 0xFC
#define DEPZ_SH2_SET_FEATURE_COMMAND  0xFD
#define DEPZ_SH2_GET_FEATURE_REQUEST  0xFE
#define DEPZ_SH2_FORCE_SENSOR_FLUSH   0xF0 /* host -> hub; answered by 0xEF on the input channel */

/* Command Request `command` values. */
#define DEPZ_SH2_CMD_ERRORS              0x01
#define DEPZ_SH2_CMD_COUNTER             0x02
#define DEPZ_SH2_CMD_TARE                0x03
#define DEPZ_SH2_CMD_INITIALIZE          0x04
#define DEPZ_SH2_CMD_SAVE_DCD            0x06
#define DEPZ_SH2_CMD_ME_CALIBRATE        0x07
#define DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG 0x09
#define DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE 0x0A
#define DEPZ_SH2_CMD_CLEAR_DCD_AND_RESET 0x0B
#define DEPZ_SH2_CMD_INTERACTIVE_CALIBRATION 0x0E /* BNO086 only; no response */

/* P0 of the Interactive Calibration command (SH-2 §6.4.12.1): what the host
 * knows about the device's motion. UNKNOWN is the hub's initial state. */
#define DEPZ_BNO_MOTION_INTENT_UNKNOWN                   0u
#define DEPZ_BNO_MOTION_INTENT_STATIONARY_NO_VIBRATION   1u
#define DEPZ_BNO_MOTION_INTENT_STATIONARY_WITH_VIBRATION 2u
#define DEPZ_BNO_MOTION_INTENT_IN_MOTION                 3u
#define DEPZ_BNO_MOTION_INTENT_IN_MOTION_ACCELERATING    4u

/* Set Feature `flags` bits (SH-2 §6.5.4) for depz_bno086_feature_request.flags.
 * WAKEUP: the sensor reports on SHTP channel 4 (wake input); ALWAYS_ON: it
 * keeps running while the hub sleeps. Together they kept a report at its full
 * rate through depz_bno086_sleep() on a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16),
 * while every other report stopped. */
#define DEPZ_BNO_FEATURE_WAKEUP    0x04u
#define DEPZ_BNO_FEATURE_ALWAYS_ON 0x08u

/* Tare axes (bitmap) and basis (the rotation vector tared against). */
#define DEPZ_BNO_TARE_X   1u
#define DEPZ_BNO_TARE_Y   2u
#define DEPZ_BNO_TARE_Z   4u
#define DEPZ_BNO_TARE_ALL 7u
#define DEPZ_BNO_TARE_BASIS_RV            0u
#define DEPZ_BNO_TARE_BASIS_GAME_RV       1u
#define DEPZ_BNO_TARE_BASIS_GEOMAG_RV     2u
#define DEPZ_BNO_TARE_BASIS_GYRO_RV       3u
#define DEPZ_BNO_TARE_BASIS_ARVR_RV       4u
#define DEPZ_BNO_TARE_BASIS_ARVR_GAME_RV  5u

/* Get Oscillator Type results; error-record source 255 ends the queue. */
#define DEPZ_BNO_OSC_INTERNAL     0u
#define DEPZ_BNO_OSC_EXT_CRYSTAL  1u
#define DEPZ_BNO_OSC_EXT_CLOCK    2u
#define DEPZ_BNO_ERR_SOURCE_NO_MORE 255u

/* FRS record ids used by the SDK (SH-2 figure 28). */
#define DEPZ_BNO_FRS_STATIC_CALIBRATION_AGM 0x7979u
#define DEPZ_BNO_FRS_NOMINAL_CALIBRATION    0x4D4Du
#define DEPZ_BNO_FRS_DYNAMIC_CALIBRATION    0x1F1Fu
#define DEPZ_BNO_FRS_ME_POWER_MGMT          0xD3E2u
#define DEPZ_BNO_FRS_SYSTEM_ORIENTATION     0x2D3Eu /* 4 x Q30 words */
#define DEPZ_BNO_FRS_ACCEL_ORIENTATION      0x2D41u
#define DEPZ_BNO_FRS_GYROSCOPE_ORIENTATION  0x2D46u
#define DEPZ_BNO_FRS_MAGNETOMETER_ORIENTATION 0x2D4Cu
#define DEPZ_BNO_FRS_ARVR_STABILIZATION_RV  0x3E2Du
#define DEPZ_BNO_FRS_ARVR_STABILIZATION_GRV 0x3E2Eu
#define DEPZ_BNO_FRS_SIG_MOTION_DETECT_CONFIG 0xC274u
#define DEPZ_BNO_FRS_SHAKE_DETECT_CONFIG    0x7D7Du
#define DEPZ_BNO_FRS_STABILITY_DETECTOR_CONFIG 0xED85u
#define DEPZ_BNO_FRS_ACTIVITY_TRACKER_CONFIG 0xED88u
#define DEPZ_BNO_FRS_GYRO_INTEGRATED_RV_CONFIG 0xA1A2u /* reference, sync interval, max error, prediction */

/* FRS read statuses (low nibble) and write statuses. */
#define DEPZ_BNO_FRS_READ_NO_ERROR               0u
#define DEPZ_BNO_FRS_READ_UNRECOGNIZED_TYPE      1u
#define DEPZ_BNO_FRS_READ_BUSY                   2u
#define DEPZ_BNO_FRS_READ_COMPLETED              3u
#define DEPZ_BNO_FRS_READ_OFFSET_OUT_OF_RANGE    4u
#define DEPZ_BNO_FRS_READ_RECORD_EMPTY           5u
#define DEPZ_BNO_FRS_READ_BLOCK_COMPLETED        6u
#define DEPZ_BNO_FRS_READ_BLOCK_AND_READ_COMPLETED 7u
#define DEPZ_BNO_FRS_READ_DEVICE_ERROR           8u
#define DEPZ_BNO_FRS_WRITE_WORDS_RECEIVED        0u
#define DEPZ_BNO_FRS_WRITE_UNRECOGNIZED_TYPE     1u
#define DEPZ_BNO_FRS_WRITE_BUSY                  2u
#define DEPZ_BNO_FRS_WRITE_COMPLETED             3u
#define DEPZ_BNO_FRS_WRITE_MODE_READY            4u
#define DEPZ_BNO_FRS_WRITE_FAILED                5u
#define DEPZ_BNO_FRS_WRITE_NOT_IN_WRITE_MODE     6u
#define DEPZ_BNO_FRS_WRITE_INVALID_LENGTH        7u
#define DEPZ_BNO_FRS_WRITE_RECORD_VALID          8u
#define DEPZ_BNO_FRS_WRITE_RECORD_INVALID        9u

/* Get Feature Response 0xFC (17 B): the rates in effect. */
typedef struct {
    uint8_t  sensor_id, flags;
    uint16_t sensitivity;
    uint32_t interval_us;   /* granted report interval; 0 = disabled */
    uint32_t batch_us;
    uint32_t cfg_word;
} depz_bno_feature;

/* Product ID Response 0xF8 (16 B), one per software component (a BNO085
 * sends four: the SH-2 application first, then the bootloader and two support
 * components). */
typedef struct {
    uint8_t  reset_cause, sw_version_major, sw_version_minor;
    uint32_t sw_part_number;  /* 10004148 = BNO085, 10004563 = BNO086 */
    uint32_t sw_build_number;
    uint16_t sw_version_patch;
} depz_bno_product_id;

/* Command Response 0xF1 (16 B). r[0] is the status for most commands. */
typedef struct {
    uint8_t seq, command, command_seq, response_seq;
    uint8_t r[11];
} depz_bno_command_response;

/* FRS Read Response 0xF3 (16 B): up to two words per packet. */
typedef struct {
    uint8_t  status;        /* DEPZ_BNO_FRS_READ_* */
    uint8_t  data_length;   /* valid words in data0 / data1 (0..2) */
    uint16_t offset_words;
    uint32_t data0, data1;
    uint16_t frs_type;
} depz_bno_frs_read_response;

/* FRS Write Response 0xF5 (4 B). */
typedef struct {
    uint8_t  status;        /* DEPZ_BNO_FRS_WRITE_* */
    uint16_t offset_words;
} depz_bno_frs_write_response;

/* Each returns 0, or -1 when `p` is too short or not that report. */
int depz_bno_unpack_feature_response(const uint8_t *p, size_t len, depz_bno_feature *out);
int depz_bno_unpack_product_id(const uint8_t *p, size_t len, depz_bno_product_id *out);
int depz_bno_unpack_command_response(const uint8_t *p, size_t len, depz_bno_command_response *out);
int depz_bno_unpack_frs_read_response(const uint8_t *p, size_t len, depz_bno_frs_read_response *out);
int depz_bno_unpack_frs_write_response(const uint8_t *p, size_t len, depz_bno_frs_write_response *out);
/* Fixed length of a sensor→host control report (contract 05 §6), 0 when the
 * id is not one: one control cargo may carry several back to back. */
size_t depz_bno_control_report_len(uint8_t rid);

/* Sensor metadata FRS record (0xE301..0xE325), sh2 reference layout:
 * revision-gated fields read 0 when the record predates them. */
typedef struct {
    uint8_t  me_version, mh_version, sh_version;
    uint32_t range_raw;        /* same units and Q point as the sensor's reports */
    uint32_t resolution_raw;
    uint16_t revision;         /* word 3 bits 31:16 */
    uint16_t power_ma_q10;     /* word 3 bits 15:0: mA, Q10 */
    uint32_t min_period_us;
    uint32_t max_period_us;    /* revision >= 4 */
    uint16_t fifo_max, fifo_reserved;
    uint16_t batch_buffer_bytes;
    uint16_t q_point_1, q_point_2;
    uint16_t q_point_3;        /* revision >= 3 */
} depz_bno_metadata;

void depz_bno_metadata_from_words(const uint32_t *words, size_t n, depz_bno_metadata *out);
/* The metadata FRS record of a sensor id, 0 when none is known. */
uint16_t depz_bno_metadata_record(uint8_t sensor_id);

/* ======================================================================== */
/* BNO086 SH-2 input-report parsers (contract 05 §5)                          */
/* ======================================================================== */

#define DEPZ_BNO_BASE_TIMESTAMP_REF 0xFB
#define DEPZ_BNO_TIMESTAMP_REBASE   0xFA
/* + u8 sensor id, no SH-2 header: the answer to Force Sensor Flush (0xF0) */
#define DEPZ_BNO_FLUSH_COMPLETED    0xEF

typedef enum {
    DEPZ_BNO_ACCELERATION = 0,
    DEPZ_BNO_GYROSCOPE,
    DEPZ_BNO_MAGNETOMETER,
    DEPZ_BNO_UNCAL_GYROSCOPE,
    DEPZ_BNO_UNCAL_MAGNETOMETER,
    DEPZ_BNO_ROTATION_VECTOR,
    DEPZ_BNO_SCALAR_REPORT,
    DEPZ_BNO_TAP_DETECTOR,
    DEPZ_BNO_STEP_COUNTER,
    DEPZ_BNO_STEP_DETECTOR,
    DEPZ_BNO_SIGNIFICANT_MOTION,
    DEPZ_BNO_STABILITY_CLASSIFIER,
    DEPZ_BNO_SHAKE_DETECTOR,
    DEPZ_BNO_ACTIVITY_CLASSIFIER,
    DEPZ_BNO_RAW_SENSOR,
    DEPZ_BNO_GENERIC_EVENT,
    DEPZ_BNO_GYRO_INTEGRATED_RV,
    DEPZ_BNO_UNKNOWN_REPORT,
    DEPZ_BNO_MOTION_REQUEST,   /* 0x2B: intent, request */
    DEPZ_BNO_FLUSH_COMPLETED_REPORT /* 0xEF: flushed_sensor; no seq/accuracy/delay */
} depz_bno_report_type;

/* Report catalog name (matches the vector "type" strings). */
const char *depz_bno_report_type_str(depz_bno_report_type t);

/* One decoded report. Only the fields relevant to `type` are meaningful. All
 * *_raw values are the wire integers (no Q-point scaling). */
typedef struct {
    depz_bno_report_type type;
    uint8_t  sensor_id;
    int64_t  timestamp_us;    /* capture - base_delta*100 + delay*100 */
    /* channel-3/4 header (absent for GYRO_INTEGRATED_RV / UNKNOWN_REPORT /
     * FLUSH_COMPLETED_REPORT) */
    uint8_t  seq;
    uint8_t  accuracy;
    int32_t  delay_us;
    /* vector / raw-sensor axes */
    int32_t  x_raw, y_raw, z_raw;
    int32_t  bias_x_raw, bias_y_raw, bias_z_raw;
    /* rotation vector */
    int32_t  i_raw, j_raw, k_raw, real_raw;
    int32_t  accuracy_raw;    /* valid only when has_accuracy_raw */
    bool     has_accuracy_raw;
    /* scalar / generic */
    int64_t  value_raw;
    /* detectors */
    int32_t  flags;
    uint32_t latency_us;
    uint32_t steps;
    int32_t  motion;
    int32_t  classification;
    /* activity classifier */
    int32_t  page_number;
    bool     end_of_sequence;
    int32_t  most_likely_state;
    uint8_t  confidences[10];
    /* motion request (0x2B): the motion intent the hub was last told and what
     * it asks of the device (SH-2 §6.5.45.2; 1, 3, 4 = stay still) */
    int32_t  intent;
    int32_t  request;
    /* flush completed (0xEF): the sensor whose batched reports are all sent */
    int32_t  flushed_sensor;
    /* raw sensor */
    uint32_t sensor_timestamp_us;
    int32_t  temperature_raw;
    /* gyro-integrated RV angular velocity */
    int32_t  vx_raw, vy_raw, vz_raw;
    /* unknown report tail */
    const uint8_t *data;
    size_t   data_len;
} depz_bno_report;

/*
 * Parse a channel-3/4 input cargo into typed reports. `capture_timestamp_us`
 * is the bridge RPT_DATA capture time. Writes up to `cap` reports into `out`,
 * returns the count. Handles 0xFB base and 0xFA rebase, and a bare 0xEF
 * Flush Completed (2 bytes, timestamp = the current base); an unknown report
 * id stops the parse (last entry is UNKNOWN_REPORT).
 */
size_t depz_bno_parse_input_cargo(const uint8_t *payload, size_t len,
                                  uint64_t capture_timestamp_us,
                                  depz_bno_report *out, size_t cap);

/*
 * Parse a channel-5 gyro-integrated RV cargo (dense 7×i16, optionally 0xFB +
 * i32 + u16 prefixed). Returns 0 on success (fills *out), -1 on short cargo.
 */
int depz_bno_parse_gyro_rv(const uint8_t *payload, size_t len,
                           uint64_t capture_timestamp_us, depz_bno_report *out);

/* --- Scaling (value = raw / 2^Q, contract 05 §4) ------------------------- */

#define DEPZ_BNO_RV_ACCURACY_Q     12  /* rotation-vector accuracy, rad */
#define DEPZ_BNO_GYRO_RV_ANGVEL_Q  10  /* gyro-integrated RV angular velocity, rad/s */

/* Q point of a sensor's primary fields; -1 for event reports. */
int depz_bno_q_point(uint8_t sensor_id);
/* x, y, z in m/s², rad/s or µT (raw counts for the raw sensors). */
void depz_bno_report_xyz(const depz_bno_report *r, double out[3]);
/* Uncalibrated gyroscope / magnetometer bias, same units. */
void depz_bno_report_bias(const depz_bno_report *r, double out[3]);
/* Unit quaternion i, j, k, real (rotation vectors, gyro-integrated RV). */
void depz_bno_report_quaternion(const depz_bno_report *r, double out[4]);
/* Heading accuracy estimate in radians; false for the game variants. */
bool depz_bno_report_accuracy_rad(const depz_bno_report *r, double *out);
/* Gyro-integrated RV angular velocity, rad/s. */
void depz_bno_report_angular_velocity(const depz_bno_report *r, double out[3]);
/* Environment reports 0x0A..0x0E: hPa, lux, %, cm, °C. */
double depz_bno_report_scalar(const depz_bno_report *r);

/* ======================================================================== */
/* JSON values (contract 09 dataset records)                                 */
/* ======================================================================== */

/* A JSON value: what a `.depzdata` record's `v` holds, built by the record
 * functions below (depz_dataset_*_record) and handed to the writer, read back
 * from a parsed dataset. Owned trees: free the root with depz_json_free();
 * depz_json_push() / depz_json_set() take ownership of the child. */
typedef struct depz_json depz_json;

typedef enum {
    DEPZ_JSON_NULL = 0,
    DEPZ_JSON_BOOL,
    DEPZ_JSON_INT,     /* a whole number (int64) */
    DEPZ_JSON_DOUBLE,  /* a real number */
    DEPZ_JSON_STRING,
    DEPZ_JSON_ARRAY,
    DEPZ_JSON_OBJECT
} depz_json_kind;

/* Constructors (NULL when out of memory). Integers stay integers and reals
 * stay reals in the file, as the Python reference writes them. */
depz_json *depz_json_null(void);
depz_json *depz_json_bool(bool b);
depz_json *depz_json_int(int64_t i);
depz_json *depz_json_double(double d);
depz_json *depz_json_string(const char *s);             /* copied, NUL-terminated */
depz_json *depz_json_string_n(const char *s, size_t n); /* copied, may hold NULs */
depz_json *depz_json_array(void);
depz_json *depz_json_object(void);
/* Append `item` to an array / set `key` of an object (replacing an existing
 * key). Both take ownership of `item` — also on failure (-1: wrong kind, out
 * of memory), so a chain of constructors never leaks. */
int  depz_json_push(depz_json *array, depz_json *item);
int  depz_json_set(depz_json *object, const char *key, depz_json *item);
void depz_json_free(depz_json *v);

/* Parse one JSON document out of text[0..len). NULL when it does not parse. */
depz_json *depz_json_parse(const char *text, size_t len);
/* Serialize compactly (no spaces, non-ASCII as \uXXXX, reals in the shortest
 * form that reads back exactly; NaN / infinities as null). Returns the text's
 * length, writing at most cap - 1 bytes plus a NUL into `out` (out may be
 * NULL with cap 0 to size a buffer). */
size_t depz_json_dump(const depz_json *v, char *out, size_t cap);
/* The same into a malloc'd string the caller frees; NULL out of memory. */
char *depz_json_dumps(const depz_json *v);

/* Accessors. Readers return a zero / NULL default on a kind mismatch;
 * depz_json_double_value also reads an integer. */
depz_json_kind   depz_json_kind_of(const depz_json *v);     /* NULL -> DEPZ_JSON_NULL */
bool             depz_json_bool_value(const depz_json *v);
int64_t          depz_json_int_value(const depz_json *v);   /* a real is truncated */
double           depz_json_double_value(const depz_json *v);
const char      *depz_json_string_value(const depz_json *v);  /* NULL unless a string */
size_t           depz_json_string_len(const depz_json *v);
/* Arrays and objects: element count, the i-th element, the i-th key (objects),
 * a member by key (NULL when absent). */
size_t           depz_json_len(const depz_json *v);
const depz_json *depz_json_at(const depz_json *v, size_t i);
const char      *depz_json_key_at(const depz_json *v, size_t i);
const depz_json *depz_json_get(const depz_json *v, const char *key);
/* Structural equality (reals by value, so 2 == 2.0). */
bool depz_json_equal(const depz_json *a, const depz_json *b);

/* ======================================================================== */
/* .depzdata datasets (contract 09): read, write, record values, playback    */
/* ======================================================================== */

/* The schema id this SDK writes. */
#define DEPZ_DATASET_SCHEMA "depz.dataset/2"

typedef struct depz_dataset depz_dataset;

/* A header device entry. `model` is the part when one sensor_type covers
 * several ("BNO085" / "BNO086" under bno086), "" when absent. */
typedef struct {
    char    id[32];
    char    serial[64];
    char    sensor_type[32];
    char    software_name[64];
    char    model[32];
    int64_t offset_us;   /* time_sync.offset_us */
    int64_t rtt_us;      /* time_sync.rtt_us    */
} depz_dataset_device;

typedef struct {
    char             device_id[32];
    int64_t          t_host_us;  /* host monotonic µs (device time - offset) */
    char             kind[32];
    const depz_json *value;      /* the record's `v`, owned by the dataset */
} depz_dataset_record;

/* Parse a whole `.depzdata` document (UTF-8 JSON Lines: header + records) or
 * a `.depzdata.gz` (a gzip container: inflated first). A damaged file is
 * refused whole, as a bad argument (contract 07 §3): NULL, with
 * depz_last_error() naming the line — the header not a JSON object or its
 * `schema` not a "depz.dataset/..." string, a record line not a JSON object,
 * `d` / `k` not strings, `t` not an integer (a real, null or absent), `v`
 * not an object, an empty file. Nothing is skipped or coerced; blank lines
 * are skipped, an unknown `k` is kept (only players ignore it). Records come
 * back as the reader orders them: each device's records stable-sorted by
 * t_host_us, then merged by it (contract 09) — per-device order is only
 * roughly monotonic in the file, so every reader sorts. */
depz_dataset *depz_dataset_parse(const char *text, size_t len);
/* The same from a file path (NULL + DEPZ_E_IO detail when unreadable). */
depz_dataset *depz_dataset_read_file(const char *path);
void          depz_dataset_free(depz_dataset *d);

const char *depz_dataset_schema(const depz_dataset *d);
const char *depz_dataset_created_utc(const depz_dataset *d);  /* "" when absent */
const char *depz_dataset_note(const depz_dataset *d);         /* "" when absent */
/* The parsed header line, for fields the struct view leaves out. */
const depz_json *depz_dataset_header(const depz_dataset *d);
size_t      depz_dataset_device_count(const depz_dataset *d);
int         depz_dataset_get_device(const depz_dataset *d, size_t i,
                                    depz_dataset_device *out);
size_t      depz_dataset_record_count(const depz_dataset *d);
int         depz_dataset_get_record(const depz_dataset *d, size_t i,
                                    depz_dataset_record *out);
/* Host-time span from the first record to the last; 0 with fewer than two. */
int64_t     depz_dataset_duration_us(const depz_dataset *d);
/* Read a scalar out of a record's `value` object. Return 0 on success. */
int depz_dataset_value_int(const depz_dataset_record *r, const char *key,
                           int64_t *out);
int depz_dataset_value_str(const depz_dataset_record *r, const char *key,
                           char *out, size_t cap);

/* --- Record values (contract 09 `v`), from the decoded results ------------ */
/* Key for key the reference Python SDK's values. The live layer
 * (depz_sensor_io.h) adds the kinds whose results only exist there (sr04,
 * vl53lx, bno055, the VL53L8 frame with its motion block, vl53l8_cnh) and
 * the session recorder that writes them all. */

/* `bno086`: the report in snake_case — type and sensor id, then only the
 * fields the report carries (scaled: x/y/z, i/j/k/real, accuracy_rad, bias,
 * angular_velocity, value), the detector outputs, the integer counts of the
 * raw ADC streams (0x14..0x16), and the device timestamp. */
depz_json *depz_dataset_bno086_record(const depz_bno_report *r);
/* `vl53l8`: resolution, silicon_temp_degc and the per-zone arrays cut to the
 * resolution; `layers` adds signal_per_spad, ambient_per_spad,
 * range_sigma_mm and reflectance. */
depz_json *depz_dataset_vl53l8_record(const depz_vl53l8_frame *f, bool layers);
/* `vl53l4`: the result fields; distance_mm raw whatever range_status says. */
depz_json *depz_dataset_vl53l4_record(const depz_vl53l4_result *r);
/* `tmf8829`: one measurement — fp_mode, rows, cols, peaks, f_number, flags,
 * temperature_c, distance_mm (mm, rounded to 0.1, flat [peak][row][col]),
 * snr, and signal / noise / xtalk when the result format carried them. */
depz_json *depz_dataset_tmf8829_record(const depz_tmf8829_measurement *m);
/* `temperature`: {celsius, source} — source "mcu" or "sensor". */
depz_json *depz_dataset_temperature_record(double celsius, const char *source);

/* --- Writer ---------------------------------------------------------------- */

/* Low-level `.depzdata` record writer: the header line at open, one record
 * line per write. A path ending in `.gz` is written as a gzip container of
 * stored (uncompressed) blocks — every gzip reader opens it; the SDK has no
 * compression dependency. Not thread-safe: one writer, one thread at a time
 * (the session recorder serializes its hooks). Prefer the session recorder
 * of depz_sensor_io.h for live devices. */
typedef struct depz_dataset_writer depz_dataset_writer;

/* Create `path` (overwritten) and write the header. `devices` is the header's
 * device object: {id: {serial, sensor_type, software_name, time_sync, …}}
 * (read, not consumed); `note` may be NULL. NULL when the file cannot be
 * created or `devices` is not an object. */
depz_dataset_writer *depz_dataset_writer_open(const char *path, const depz_json *devices,
                                              const char *note);
/* One record: device id, host µs, kind, value. Takes ownership of `value`
 * (also on failure). Returns 0, -1 after close / on a write error. */
int depz_dataset_writer_write(depz_dataset_writer *w, const char *device_id, int64_t t_host_us,
                              const char *kind, depz_json *value);
uint64_t depz_dataset_writer_records_written(const depz_dataset_writer *w);
/* Flush, finish the gzip trailer and close the file; then free the writer
 * (NULL is a no-op). Returns 0, or -1 when the last flush failed. */
int depz_dataset_writer_close(depz_dataset_writer *w);

/* --- Player (contract 09 §Player) ----------------------------------------- */

typedef enum {
    DEPZ_PLAYER_IDLE = 0, /* never played */
    DEPZ_PLAYER_PLAYING,  /* delivering records paced by the clock */
    DEPZ_PLAYER_PAUSED,   /* stopped mid-timeline; play resumes */
    DEPZ_PLAYER_DONE      /* past the last record; play restarts from the top */
} depz_player_state;

typedef struct depz_dataset_player depz_dataset_player;

/* A monotonic clock in seconds (the player's media clock is anchored to it). */
typedef double (*depz_player_clock_fn)(void *user);
typedef void (*depz_player_record_cb)(const depz_dataset_record *r, void *user);
typedef void (*depz_player_state_cb)(depz_player_state state, void *user);

/* A player over `ds` (which must outlive it): paced delivery with play /
 * pause / seek / speed — the engine behind a viewer's playback mode. Delivery
 * is tick-driven: the owner calls depz_dataset_player_tick() from its own
 * loop (a GUI timer, or a test with a fake clock) and the records due go out
 * on that thread. `clock` NULL = the host monotonic clock. `speed` ≥ 0 (0 =
 * as fast as possible); NULL for an invalid speed.
 *
 * Timing: a media clock anchored at play / resume / seek to (now, t of the
 * last delivered record — of the first record when none was). Record i is
 * due at anchor_wall + (t_i − anchor_t) / speed: the first record goes out at
 * once, each later one after its gap to the previous one, and a resumed or
 * seeked player waits the next record's full gap. Pacing is against the
 * clock, not cumulative: overdue records go out back to back. */
depz_dataset_player *depz_dataset_player_new(const depz_dataset *ds, double speed,
                                             depz_player_clock_fn clock, void *clock_user);
void depz_dataset_player_free(depz_dataset_player *p);

depz_player_state depz_dataset_player_state(const depz_dataset_player *p);
/* Span of the timeline: the last record's t minus the first's. */
int64_t depz_dataset_player_duration_us(const depz_dataset_player *p);
/* µs from the first record to the last delivered one (after a seek: to the
 * record just before the target); 0 before anything was delivered. */
int64_t depz_dataset_player_position_us(const depz_dataset_player *p);
double  depz_dataset_player_speed(const depz_dataset_player *p);
/* Change the speed (finite, ≥ 0; else -1, nothing changes). While playing it
 * applies at once: the rest of the current wait is rescaled. */
int     depz_dataset_player_set_speed(depz_dataset_player *p, double speed);

/* Subscribe to delivered records / state changes (called with the new state
 * on every transition). `token` identifies the subscription for the off_
 * call, which is idempotent. */
int  depz_dataset_player_on_record(depz_dataset_player *p, depz_player_record_cb cb, void *user,
                                   int *token);
void depz_dataset_player_off_record(depz_dataset_player *p, int token);
int  depz_dataset_player_on_state(depz_dataset_player *p, depz_player_state_cb cb, void *user,
                                  int *token);
void depz_dataset_player_off_state(depz_dataset_player *p, int token);

/* Start, or resume after pause; no-op while playing. From done (or a position
 * past the end) playback restarts at the first record. An empty dataset goes
 * playing -> done at once. */
void depz_dataset_player_play(depz_dataset_player *p);
/* Stop delivering; play resumes from the next record, which then waits its
 * full gap again. No-op unless playing. */
void depz_dataset_player_pause(depz_dataset_player *p);
/* Move to `offset_us` µs from the first record: the next record delivered is
 * the first with t >= first t + offset (negative: the start); records in
 * between are skipped, never delivered. The state does not change, except
 * that a seek past the end while playing ends playback (done). NaN is -1
 * (nothing changes); on an empty dataset a seek is a no-op. */
int  depz_dataset_player_seek_us(depz_dataset_player *p, double offset_us);
/* Deliver every record due now, on the calling thread. Returns the seconds
 * until the next one is due, or a negative value when not playing.
 * Callbacks may call the player back (pause / seek / set_speed). */
double depz_dataset_player_tick(depz_dataset_player *p);

#ifdef __cplusplus
}
#endif

#endif /* DEPZ_SENSOR_SDK_H */
