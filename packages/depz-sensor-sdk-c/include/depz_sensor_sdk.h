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
 *   - .fwdepz container parse/validate (contract 06 §2)
 *
 * Sensor host-logic (VL53L8 ULD, BNO086 SH-2/SHTP) is intentionally out of
 * scope for this milestone.
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
    DEPZ_SENSOR_UNKNOWN    /* an APP_* name we don't recognize */
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
/* NOT-YET-IMPLEMENTED extension points (hardware-dependent, out of scope):   */
/*   - the live ULD init/config register-bridge driver (firmware download,    */
/*     DCI register sequences) — see the TS/Python reference `uld` modules;   */
/*   - CNH compact-network-histogram decode — the CH-specific extension       */
/*     point (see DEPZ_VL53L8_VARIANT_CH / the CNH note in vl53l8_decode.c).  */
/* ======================================================================== */

/* Which ToF sensor produced a frame. The frame decoder is variant-agnostic  */
/* (CX and CH share the ranging-results layout); the enum exists to name the  */
/* CX/CH split and anchor the CH-only CNH extension point. */
typedef enum {
    DEPZ_VL53L8_VARIANT_CX = 0, /* base ToF (dev-default)                    */
    DEPZ_VL53L8_VARIANT_CH = 1  /* CX + CNH compact-network-histograms (ED40)*/
} depz_vl53l8_variant;

/* Footer-id offset in a decoded frame: the header id at [8..9] must equal    */
/* the footer id at [raw_len - N]. N = 12 for the CX / shared CH ranging      */
/* frame. CNH histogram frames (CH-only) are a different layout and are not   */
/* decoded here — see the CNH extension-point note in vl53l8_decode.c. */
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
 * Decode a reassembled raw ranging frame. The layout is shared by BOTH
 * VL53L8CX and VL53L8CH (the CX firmware layout; CH emits the same ranging
 * results), so this decoder serves both variants. Returns 0 on success, -1 on
 * corrupted frame (header/footer id mismatch), -2 on bad length.
 * `timestamp_us` is carried through from the reassembler.
 *
 * NOTE: the CH-only CNH (compact-network-histogram) frame is a distinct layout
 * and is NOT handled here — see the CNH extension-point note in
 * vl53l8_decode.c.
 */
int depz_vl53l8_decode_frame(const uint8_t *raw, size_t raw_len,
                             uint64_t timestamp_us, depz_vl53l8_frame *out);

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
 * the pure codec half of the motion indicator; the live DCI write is out of
 * scope. Returns 0 on success, -1 on bad resolution.
 */
int depz_vl53l8_motion_cfg_default_pack(int resolution,
                                        uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE]);

/* ======================================================================== */
/* BNO086 SHTP framing (contract 05 §3) + SH-2 control encoders (§6)          */
/* ======================================================================== */

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
