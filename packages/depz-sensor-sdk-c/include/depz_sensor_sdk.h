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
 *   - .fwdepz container parse/validate (contract 06 §2)
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
 * Returns 0 on success, -1 on payload-too-long, -2 on insufficient capacity.
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
#define DEPZ_PID_BNO086 0xEE08u /* 60936 */
#define DEPZ_PID_RANGE_LO 60536u
#define DEPZ_PID_RANGE_HI 65535u
#define DEPZ_DEV_USB_VID 0x0483u /* 1155  — STMicro dev/unprogrammed default */
#define DEPZ_DEV_USB_PID 0x56DCu /* 22236 */

/* True when (vid, pid) is a recognized DEPZ (or dev-default) USB id. */
bool depz_is_known_depz_usb(int vid, int pid);
/* Best-guess model name for a (vid, pid), or NULL. Informational only. */
const char *depz_usb_model_hint(int vid, int pid);

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
    DEPZ_SENSOR_UNKNOWN    /* an APP_* name we don't recognize; must stay last */
} depz_sensor_type;

typedef struct {
    const char *mode; /* "app" | "bootloader" | "unknown" */
    depz_sensor_type sensor_type;
    char software_name[64];
    char version[24]; /* "" when not parseable */
} depz_identity;

const char *depz_sensor_type_str(depz_sensor_type t); /* NULL for NONE */

/* Strip trailing 0x00/0xFF filler and copy as an ASCII C-string into `out`.
 * Returns the resulting string length. */
size_t depz_strip_device_string(const uint8_t *data, size_t len,
                                char *out, size_t out_cap);

/* Classify a GET_NAME_ACTIVE_SOFTWARE string (already stripped). */
void depz_parse_software_name(const char *name, depz_identity *out);

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
    DEPZ_CMD_THROUGHPUT_TX_START      = 0x1C,
    DEPZ_CMD_THROUGHPUT_TX_STOP       = 0x1D,
    DEPZ_CMD_THROUGHPUT_RX_DATA       = 0x1E,
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
    DEPZ_RPT_THROUGHPUT_DATA  = 0x88,
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
size_t depz_pack_set_payload_crc_type(uint8_t crc_type, uint8_t *out); /* 1 B */
size_t depz_pack_sync_pin_config(const depz_sync_pin_config *c, uint8_t *out); /* 3 B */

/* Decoders (return 0 on success, -1 on wrong payload length). */
int depz_unpack_status(const uint8_t *p, size_t len, depz_status_report *out);
int depz_unpack_text(const uint8_t *p, size_t len, depz_text_report *out);
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
    DEPZ_FWDEPZ_ERR_SIZE
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

/* Parse & validate (magic -> header CRC-16/CCITT-FALSE -> fw_size==payload). */
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
size_t depz_vl53l8_pack_start_stream(uint16_t frame_size, uint8_t *out);         /* 2 B */

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
 * Frame reassembler (contract 04): reset on offset==0; chunks must be
 * contiguous (offset == accumulated length) and agree on full_size; a gap
 * discards the frame in progress; completes when accumulated == full_size.
 */
typedef struct {
    uint8_t  buf[DEPZ_VL53L8_STREAM_TOTAL_MAX];
    size_t   len;
    uint16_t full_size;
    uint64_t timestamp_us;
    uint64_t completed;
    uint64_t discarded;
} depz_vl53l8_reassembler;

void depz_vl53l8_reasm_init(depz_vl53l8_reassembler *r);
/*
 * Feed one chunk. Returns 1 when a frame completes (sets *frame -> the
 * internal buffer, *frame_len, *ts), 0 otherwise. The frame pointer stays
 * valid until the next feed.
 */
int depz_vl53l8_reasm_feed(depz_vl53l8_reassembler *r, const depz_vl53l8_chunk *c,
                           const uint8_t **frame, size_t *frame_len, uint64_t *ts);

/*
 * One decoded ranging frame. Arrays are valid for [0, resolution); zone index
 * runs row-major. Values are RAW fixed-point integers exactly as on the wire
 * (no float scaling — the host applies range_sigma/128, signal/kcps, etc.).
 * distance_mm is the ST-scaled value (raw/4, floored) to match GetRangingData.
 */
typedef struct {
    uint64_t timestamp_us;
    int      resolution;                 /* 16 | 64 (zone count present)      */
    int8_t   silicon_temp_degc;
    int32_t  distance_mm[DEPZ_VL53L8_MAX_ZONES];
    uint8_t  target_status[DEPZ_VL53L8_MAX_ZONES];
    uint8_t  nb_target_detected[DEPZ_VL53L8_MAX_ZONES];
    uint32_t signal_per_spad[DEPZ_VL53L8_MAX_ZONES];   /* kcps/SPAD raw       */
    uint32_t ambient_per_spad[DEPZ_VL53L8_MAX_ZONES];  /* kcps/SPAD raw       */
    uint32_t nb_spads_enabled[DEPZ_VL53L8_MAX_ZONES];
    uint16_t range_sigma_mm_raw[DEPZ_VL53L8_MAX_ZONES];/* mm = raw/128        */
    uint8_t  reflectance[DEPZ_VL53L8_MAX_ZONES];       /* %                   */
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
    int feature_length;   /* 1..DEPZ_VL53L8_CNH_MAX_FEATURE    */
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

/* Xtalk margin (kcps/SPAD) -> raw DCI value = round(kcps * 2048). */
uint32_t depz_vl53l8_xtalk_margin_to_raw(double margin_kcps);
/* Inverse: raw DCI -> kcps/SPAD (raw / 2048.0). */
double   depz_vl53l8_xtalk_margin_from_raw(uint32_t raw);

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
size_t depz_vl53l4_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out); /* 4 B */
/* VL53_WRITE_REG payload: addr u16 + data. Returns 2 + data_len, or 0 when
 * data_len is outside 1..DEPZ_VL53L4_XFER_MAX. */
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

/* RPT_VL53_INFO — bridge diagnostics (21 B, little-endian). Counters are
 * free-running and wrap silently; watch increments, not absolute values. */
typedef struct {
    uint32_t int_edges;
    uint32_t slots_skipped;
    uint32_t i2c_errors;
    uint8_t  last_i2c_error; /* 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR */
    uint16_t model_id;       /* expected DEPZ_VL53L4_MODEL_ID (0xEBAA) */
    uint8_t  fw_status;      /* expected 0x03 (booted) */
    uint8_t  initialized;    /* 1 = MODEL_ID matched on this read */
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
 * Returns 0 on success, -1 when len < 15.
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
 * osc_frequency == 0, budget_ms outside 10..200, or 0 < inter_ms <= budget_ms.
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
/* The PID model (`usb_model` as from depz_usb_model_hint(), may be NULL) if
 * it names a family product, else depz_vl53lx_product_from_board_name(). */
depz_vl53lx_product depz_vl53lx_resolve_product(const char *usb_model,
                                                const char *device_name);

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
size_t depz_bno055_pack_read_reg(uint8_t addr, uint8_t len, uint8_t *out); /* 2 B */
/* BNO_WRITE_REG payload: addr u8 + data. Returns 1 + data_len, or 0 when
 * data_len is outside 1..DEPZ_BNO055_XFER_MAX. */
size_t depz_bno055_pack_write_reg(uint8_t addr, const uint8_t *data,
                                  size_t data_len, uint8_t *out);
/* BNO_START_STREAM payload: trigger u8, addr u8, len u8, period_ms u16. */
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
 * (registers 0x00..0x06) plus bridge diagnostics. Counters are free-running
 * and wrap; read_*_us, slots_skipped, loop_max_us reset at START_STREAM. A
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

typedef struct {
    uint16_t length;      /* cargo length incl. this 4-byte header */
    uint8_t  channel;
    uint8_t  seq;
    bool     continuation;
} depz_shtp_header;

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
} depz_shtp_rx_channel;

typedef struct {
    depz_shtp_rx_channel rx[DEPZ_SHTP_NUM_CHANNELS];
    uint8_t  tx_seq[DEPZ_SHTP_NUM_CHANNELS];
    uint64_t discarded;   /* incomplete/orphan cargos thrown away */
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
} depz_shtp_cargo;

/* Feed one inbound frame. Returns 1 when a cargo completes (fills *out, whose
 * payload points into the channel buffer, valid until the next feed on that
 * channel), 0 otherwise. Returns -1 on allocation failure. */
int depz_shtp_feed(depz_shtp_layer *l, const uint8_t *frame, size_t frame_len,
                   depz_shtp_cargo *out);

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
/* COMMAND_REQUEST 0xF2, 12 B (params zero-padded, up to 9 B). */
size_t depz_bno_pack_command_request(uint8_t seq, uint8_t command,
                                     const uint8_t *params, size_t nparams,
                                     uint8_t out[12]);
/* FRS_READ_REQUEST 0xF4, 8 B. */
size_t depz_bno_pack_frs_read_request(uint16_t frs_type, uint16_t offset_words,
                                      uint16_t block_words, uint8_t out[8]);
/* FRS_WRITE_REQUEST 0xF7, 6 B. */
size_t depz_bno_pack_frs_write_request(uint16_t frs_type, uint16_t length_words,
                                       uint8_t out[6]);
/* FRS_WRITE_DATA 0xF6, 4 + 4*nwords B. Returns 0 on capacity error. */
size_t depz_bno_pack_frs_write_data(uint16_t offset_words, const uint32_t *words,
                                    size_t nwords, uint8_t *out, size_t out_cap);

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

/* Product ID Response 0xF8 (16 B), one per subsystem. */
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

/* Sensor metadata FRS record (0xE301..0xE324), sh2 reference layout:
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
    DEPZ_BNO_UNKNOWN_REPORT
} depz_bno_report_type;

/* Report catalog name (matches the vector "type" strings). */
const char *depz_bno_report_type_str(depz_bno_report_type t);

/* One decoded report. Only the fields relevant to `type` are meaningful. All
 * *_raw values are the wire integers (no Q-point scaling). */
typedef struct {
    depz_bno_report_type type;
    uint8_t  sensor_id;
    int64_t  timestamp_us;    /* capture - base_delta*100 + delay*100 */
    /* channel-3/4 header (absent for GYRO_INTEGRATED_RV / UNKNOWN_REPORT) */
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
 * returns the count. Handles 0xFB base and 0xFA rebase; an unknown report id
 * stops the parse (last entry is UNKNOWN_REPORT).
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
/* .depzdata dataset reader (contract 09) — JSONL: header + records.          */
/* ======================================================================== */

typedef struct depz_dataset depz_dataset;

typedef struct {
    char    id[32];
    char    serial[64];
    char    sensor_type[32];
    char    software_name[64];
    int64_t offset_us;   /* time_sync.offset_us */
    int64_t rtt_us;      /* time_sync.rtt_us    */
} depz_dataset_device;

typedef struct {
    char        device_id[32];
    int64_t     t_host_us;
    char        kind[32];
    const void *value;   /* opaque JSON node — read via depz_dataset_value_* */
} depz_dataset_record;

/* Parse a full .depzdata blob. Returns NULL on parse error / bad schema.
 * Records are exposed in stable merge order (by t_host_us, then file order). */
depz_dataset *depz_dataset_parse(const char *text, size_t len);
void          depz_dataset_free(depz_dataset *d);

const char *depz_dataset_schema(const depz_dataset *d);
size_t      depz_dataset_device_count(const depz_dataset *d);
int         depz_dataset_get_device(const depz_dataset *d, size_t i,
                                    depz_dataset_device *out);
size_t      depz_dataset_record_count(const depz_dataset *d);
int         depz_dataset_get_record(const depz_dataset *d, size_t i,
                                    depz_dataset_record *out);
/* Read a scalar out of a record's `value` object. Return 0 on success. */
int depz_dataset_value_int(const depz_dataset_record *r, const char *key,
                           int64_t *out);
int depz_dataset_value_str(const depz_dataset_record *r, const char *key,
                           char *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* DEPZ_SENSOR_SDK_H */
