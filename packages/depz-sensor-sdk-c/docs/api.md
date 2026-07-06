# API reference

Auto-generated from the public header `include/depz_sensor_sdk.h`
(its declarations and doc-comments) by `scripts/gen_api_md.py` — run
`make docs` to regenerate. Edit the doc-comments in the header, not
this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

## Contents

- **Discovery**: [`DEPZ_USB_VID`](#depz_usb_vid), [`DEPZ_PID_SR04`](#depz_pid_sr04), [`DEPZ_PID_VL53L8`](#depz_pid_vl53l8), [`DEPZ_PID_BNO086`](#depz_pid_bno086), [`DEPZ_PID_RANGE_LO`](#depz_pid_range_lo), [`DEPZ_PID_RANGE_HI`](#depz_pid_range_hi), [`DEPZ_DEV_USB_VID`](#depz_dev_usb_vid), [`DEPZ_DEV_USB_PID`](#depz_dev_usb_pid), [`depz_is_known_depz_usb`](#depz_is_known_depz_usb), [`depz_usb_model_hint`](#depz_usb_model_hint), [`depz_sensor_type`](#depz_sensor_type), [`depz_identity`](#depz_identity), [`depz_sensor_type_str`](#depz_sensor_type_str), [`depz_strip_device_string`](#depz_strip_device_string), [`depz_parse_software_name`](#depz_parse_software_name)
- **Transport**: [`depz_crc8_maxim`](#depz_crc8_maxim), [`depz_crc16_modbus`](#depz_crc16_modbus), [`depz_crc32_iso_hdlc`](#depz_crc32_iso_hdlc), [`depz_crc16_ccitt_false`](#depz_crc16_ccitt_false), [`DEPZ_MAGIC0`](#depz_magic0), [`DEPZ_MAGIC1`](#depz_magic1), [`DEPZ_HEADER_SIZE`](#depz_header_size), [`DEPZ_MAX_PAYLOAD`](#depz_max_payload), [`depz_crc_type`](#depz_crc_type), [`DEPZ_MAX_FRAME`](#depz_max_frame), [`depz_build_packet`](#depz_build_packet), [`depz_event_type`](#depz_event_type), [`depz_event`](#depz_event), [`depz_event_cb`](#depz_event_cb), [`depz_parser`](#depz_parser), [`depz_parser_init`](#depz_parser_init), [`depz_parser_free`](#depz_parser_free), [`depz_parser_feed`](#depz_parser_feed)
- **Common protocol**: [`depz_cmd`](#depz_cmd), [`depz_rpt`](#depz_rpt), [`depz_status`](#depz_status), [`depz_sync_pin_mode`](#depz_sync_pin_mode), [`depz_status_report`](#depz_status_report), [`depz_text_report`](#depz_text_report), [`depz_sync_time_report`](#depz_sync_time_report), [`depz_temperature_report`](#depz_temperature_report), [`depz_sequence_error_report`](#depz_sequence_error_report), [`depz_sync_pin_config`](#depz_sync_pin_config), [`depz_pack_sync_time`](#depz_pack_sync_time), [`depz_pack_set_payload_crc_type`](#depz_pack_set_payload_crc_type), [`depz_pack_sync_pin_config`](#depz_pack_sync_pin_config), [`depz_unpack_status`](#depz_unpack_status), [`depz_unpack_text`](#depz_unpack_text), [`depz_unpack_sync_time`](#depz_unpack_sync_time), [`depz_unpack_temperature`](#depz_unpack_temperature), [`depz_unpack_sequence_error`](#depz_unpack_sequence_error), [`depz_unpack_sync_pin_config`](#depz_unpack_sync_pin_config), [`depz_sync_time_offset_rtt`](#depz_sync_time_offset_rtt)
- **SR04**: [`depz_sr04_cmd`](#depz_sr04_cmd), [`depz_sr04_rpt`](#depz_sr04_rpt), [`DEPZ_SR04_ECHO_TIMEOUT`](#depz_sr04_echo_timeout), [`DEPZ_SR04_ECHO_DECAY_MIN_US`](#depz_sr04_echo_decay_min_us), [`DEPZ_SR04_ECHO_DECAY_MAX_US`](#depz_sr04_echo_decay_max_us), [`depz_sr04_data`](#depz_sr04_data), [`depz_sr04_pack_sample_period`](#depz_sr04_pack_sample_period), [`depz_sr04_pack_echo_decay`](#depz_sr04_pack_echo_decay), [`depz_sr04_unpack_data`](#depz_sr04_unpack_data), [`depz_sr04_unpack_sample_period`](#depz_sr04_unpack_sample_period), [`depz_sr04_unpack_echo_decay`](#depz_sr04_unpack_echo_decay), [`depz_sr04_distance_mm`](#depz_sr04_distance_mm)
- **VL53L8 (ToF)**: [`depz_vl53l8_variant`](#depz_vl53l8_variant), [`DEPZ_VL53L8CX_FOOTER_ID_OFFSET`](#depz_vl53l8cx_footer_id_offset), [`DEPZ_VL53L8_CMD_READ_REG`](#depz_vl53l8_cmd_read_reg), [`DEPZ_VL53L8_CMD_WRITE_REG`](#depz_vl53l8_cmd_write_reg), [`DEPZ_VL53L8_CMD_START_STREAM`](#depz_vl53l8_cmd_start_stream), [`DEPZ_VL53L8_CMD_STOP_STREAM`](#depz_vl53l8_cmd_stop_stream), [`DEPZ_VL53L8_RPT_REG_DATA`](#depz_vl53l8_rpt_reg_data), [`DEPZ_VL53L8_RPT_FRAME`](#depz_vl53l8_rpt_frame), [`DEPZ_VL53L8_STREAM_CHUNK_MAX`](#depz_vl53l8_stream_chunk_max), [`DEPZ_VL53L8_STREAM_TOTAL_MAX`](#depz_vl53l8_stream_total_max), [`DEPZ_VL53L8_RES_4X4`](#depz_vl53l8_res_4x4), [`DEPZ_VL53L8_RES_8X8`](#depz_vl53l8_res_8x8), [`DEPZ_VL53L8_MAX_ZONES`](#depz_vl53l8_max_zones), [`depz_vl53l8_pack_read_reg`](#depz_vl53l8_pack_read_reg), [`depz_vl53l8_pack_start_stream`](#depz_vl53l8_pack_start_stream), [`depz_vl53l8_chunk`](#depz_vl53l8_chunk), [`depz_vl53l8_unpack_chunk`](#depz_vl53l8_unpack_chunk), [`depz_vl53l8_reg_data`](#depz_vl53l8_reg_data), [`depz_vl53l8_unpack_reg_data`](#depz_vl53l8_unpack_reg_data), [`depz_vl53l8_reassembler`](#depz_vl53l8_reassembler), [`depz_vl53l8_reasm_init`](#depz_vl53l8_reasm_init), [`depz_vl53l8_reasm_feed`](#depz_vl53l8_reasm_feed), [`depz_vl53l8_frame`](#depz_vl53l8_frame), [`depz_vl53l8_decode_frame`](#depz_vl53l8_decode_frame), [`DEPZ_VL53L8_CNH_MAX_AGGREGATES`](#depz_vl53l8_cnh_max_aggregates), [`DEPZ_VL53L8_CNH_MAX_FEATURE`](#depz_vl53l8_cnh_max_feature), [`depz_vl53l8ch_cnh_config`](#depz_vl53l8ch_cnh_config), [`depz_vl53l8ch_cnh_frame`](#depz_vl53l8ch_cnh_frame), [`depz_vl53l8ch_decode_cnh`](#depz_vl53l8ch_decode_cnh), [`DEPZ_VL53L8_DIST_MM`](#depz_vl53l8_dist_mm), [`DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS`](#depz_vl53l8_signal_per_spad_kcps), [`DEPZ_VL53L8_RANGE_SIGMA_MM`](#depz_vl53l8_range_sigma_mm), [`DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS`](#depz_vl53l8_ambient_per_spad_kcps), [`DEPZ_VL53L8_NB_TARGET_DETECTED`](#depz_vl53l8_nb_target_detected), [`DEPZ_VL53L8_TAR_STATUS`](#depz_vl53l8_tar_status), [`DEPZ_VL53L8_NB_SPADS_ENABLED`](#depz_vl53l8_nb_spads_enabled), [`DEPZ_VL53L8_MOTION_INDICATOR`](#depz_vl53l8_motion_indicator), [`DEPZ_VL53L8_POWER_MODE_SLEEP`](#depz_vl53l8_power_mode_sleep), [`DEPZ_VL53L8_POWER_MODE_WAKEUP`](#depz_vl53l8_power_mode_wakeup), [`DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP`](#depz_vl53l8_power_mode_deep_sleep), [`DEPZ_VL53L8_NB_THRESHOLDS`](#depz_vl53l8_nb_thresholds), [`DEPZ_VL53L8_THRESH_START_SIZE`](#depz_vl53l8_thresh_start_size), [`DEPZ_VL53L8_MOTION_CFG_SIZE`](#depz_vl53l8_motion_cfg_size), [`depz_vl53l8_xtalk_margin_to_raw`](#depz_vl53l8_xtalk_margin_to_raw), [`depz_vl53l8_xtalk_margin_from_raw`](#depz_vl53l8_xtalk_margin_from_raw), [`depz_vl53l8_threshold`](#depz_vl53l8_threshold), [`depz_vl53l8_pack_thresholds`](#depz_vl53l8_pack_thresholds), [`depz_vl53l8_motion_cfg_default_pack`](#depz_vl53l8_motion_cfg_default_pack)
- **BNO086 (IMU)**: [`DEPZ_SHTP_HEADER_SIZE`](#depz_shtp_header_size), [`DEPZ_SHTP_LENGTH_MASK`](#depz_shtp_length_mask), [`DEPZ_SHTP_CONTINUATION`](#depz_shtp_continuation), [`DEPZ_SHTP_NUM_CHANNELS`](#depz_shtp_num_channels), [`DEPZ_SHTP_MAX_TX_FRAME`](#depz_shtp_max_tx_frame), [`depz_shtp_channel_id`](#depz_shtp_channel_id), [`depz_shtp_header`](#depz_shtp_header), [`depz_shtp_pack_header`](#depz_shtp_pack_header), [`depz_shtp_unpack_header`](#depz_shtp_unpack_header), [`depz_shtp_rx_channel`](#depz_shtp_rx_channel), [`depz_shtp_layer`](#depz_shtp_layer), [`depz_shtp_init`](#depz_shtp_init), [`depz_shtp_free`](#depz_shtp_free), [`depz_shtp_next_frame`](#depz_shtp_next_frame), [`depz_shtp_cargo`](#depz_shtp_cargo), [`depz_shtp_feed`](#depz_shtp_feed), [`depz_bno_pack_set_feature`](#depz_bno_pack_set_feature), [`depz_bno_pack_get_feature_request`](#depz_bno_pack_get_feature_request), [`depz_bno_pack_product_id_request`](#depz_bno_pack_product_id_request), [`depz_bno_pack_command_request`](#depz_bno_pack_command_request), [`depz_bno_pack_frs_read_request`](#depz_bno_pack_frs_read_request), [`depz_bno_pack_frs_write_request`](#depz_bno_pack_frs_write_request), [`depz_bno_pack_frs_write_data`](#depz_bno_pack_frs_write_data), [`DEPZ_BNO_BASE_TIMESTAMP_REF`](#depz_bno_base_timestamp_ref), [`DEPZ_BNO_TIMESTAMP_REBASE`](#depz_bno_timestamp_rebase), [`depz_bno_report_type`](#depz_bno_report_type), [`depz_bno_report_type_str`](#depz_bno_report_type_str), [`depz_bno_report`](#depz_bno_report), [`depz_bno_parse_input_cargo`](#depz_bno_parse_input_cargo), [`depz_bno_parse_gyro_rv`](#depz_bno_parse_gyro_rv)
- **Bootloader / firmware update**: [`DEPZ_FWDEPZ_HEADER_SIZE`](#depz_fwdepz_header_size), [`depz_fwdepz_result`](#depz_fwdepz_result), [`depz_fwdepz_image`](#depz_fwdepz_image), [`depz_fwdepz_parse`](#depz_fwdepz_parse), [`depz_bl_cmd`](#depz_bl_cmd), [`depz_flash_info`](#depz_flash_info), [`depz_bl_pack_write_page`](#depz_bl_pack_write_page), [`depz_bl_pack_read_page`](#depz_bl_pack_read_page), [`depz_bl_unpack_flash_info`](#depz_bl_unpack_flash_info)
- **Datasets (record & replay)**: [`depz_dataset`](#depz_dataset), [`depz_dataset_device`](#depz_dataset_device), [`depz_dataset_record`](#depz_dataset_record), [`depz_dataset_parse`](#depz_dataset_parse), [`depz_dataset_free`](#depz_dataset_free), [`depz_dataset_schema`](#depz_dataset_schema), [`depz_dataset_device_count`](#depz_dataset_device_count), [`depz_dataset_get_device`](#depz_dataset_get_device), [`depz_dataset_record_count`](#depz_dataset_record_count), [`depz_dataset_get_record`](#depz_dataset_get_record), [`depz_dataset_value_int`](#depz_dataset_value_int), [`depz_dataset_value_str`](#depz_dataset_value_str)

## Discovery

### DEPZ_USB_VID

```c
#define DEPZ_USB_VID   0x1BCFu
```

7119  — production VID for all DEPZ sensors

### DEPZ_PID_SR04

```c
#define DEPZ_PID_SR04  0xEC78u
```

60536

### DEPZ_PID_VL53L8

```c
#define DEPZ_PID_VL53L8 0xED40u
```

60736

### DEPZ_PID_BNO086

```c
#define DEPZ_PID_BNO086 0xEE08u
```

60936

### DEPZ_PID_RANGE_LO

```c
#define DEPZ_PID_RANGE_LO 60536u
```

### DEPZ_PID_RANGE_HI

```c
#define DEPZ_PID_RANGE_HI 65535u
```

### DEPZ_DEV_USB_VID

```c
#define DEPZ_DEV_USB_VID 0x0483u
```

1155  — STMicro dev/unprogrammed default

### DEPZ_DEV_USB_PID

```c
#define DEPZ_DEV_USB_PID 0x56DCu
```

22236

### depz_is_known_depz_usb

```c
bool depz_is_known_depz_usb(int vid, int pid);
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

### depz_usb_model_hint

```c
const char *depz_usb_model_hint(int vid, int pid);
```

Best-guess model name for a (vid, pid), or NULL. Informational only.

### depz_sensor_type

```c
typedef enum {
    DEPZ_SENSOR_NONE = -1, /* null: bootloader/unknown mode */
    DEPZ_SENSOR_SR04 = 0,
    DEPZ_SENSOR_VL53L8,    /* firmware id shared by both VL53L8CX and VL53L8CH
                            * (the APP_* name reports "VL53L8" for either);
                            * the CX/CH split is DEPZ_VL53L8_VARIANT_*. */
    DEPZ_SENSOR_BNO086,
    DEPZ_SENSOR_UNKNOWN    /* an APP_* name we don't recognize */
} depz_sensor_type;
```

### depz_identity

```c
typedef struct {
    const char *mode; /* "app" | "bootloader" | "unknown" */
    depz_sensor_type sensor_type;
    char software_name[64];
    char version[24]; /* "" when not parseable */
} depz_identity;
```

### depz_sensor_type_str

```c
const char *depz_sensor_type_str(depz_sensor_type t);
```

/* NULL for NONE */

### depz_strip_device_string

```c
size_t depz_strip_device_string(const uint8_t *data, size_t len, char *out, size_t out_cap);
```

Strip trailing 0x00/0xFF filler and copy as an ASCII C-string into `out`.
Returns the resulting string length.

### depz_parse_software_name

```c
void depz_parse_software_name(const char *name, depz_identity *out);
```

Classify a GET_NAME_ACTIVE_SOFTWARE string (already stripped).

## Transport

### depz_crc8_maxim

```c
uint8_t depz_crc8_maxim(const uint8_t *data, size_t len);
```

### depz_crc16_modbus

```c
uint16_t depz_crc16_modbus(const uint8_t *data, size_t len);
```

### depz_crc32_iso_hdlc

```c
uint32_t depz_crc32_iso_hdlc(const uint8_t *data, size_t len);
```

### depz_crc16_ccitt_false

```c
uint16_t depz_crc16_ccitt_false(const uint8_t *data, size_t len);
```

CRC-16/CCITT-FALSE — only the .fwdepz file header, never on the wire.

### DEPZ_MAGIC0

```c
#define DEPZ_MAGIC0      0xA5u
```

### DEPZ_MAGIC1

```c
#define DEPZ_MAGIC1      0xC3u
```

### DEPZ_HEADER_SIZE

```c
#define DEPZ_HEADER_SIZE 7u
```

### DEPZ_MAX_PAYLOAD

```c
#define DEPZ_MAX_PAYLOAD 0x3FFFu
```

16383

### depz_crc_type

```c
typedef enum {
    DEPZ_CRC_NONE  = 0,
    DEPZ_CRC8      = 1,
    DEPZ_CRC16     = 2,
    DEPZ_CRC32     = 3
} depz_crc_type;
```

### DEPZ_MAX_FRAME

```c
#define DEPZ_MAX_FRAME (DEPZ_HEADER_SIZE + DEPZ_MAX_PAYLOAD + 4u)
```

Worst-case frame size (header + max payload + CRC32 trailer).

### depz_build_packet

```c
int depz_build_packet(uint8_t cmd, const uint8_t *payload, size_t payload_len, unsigned int seq, depz_crc_type crc_type, uint8_t *out, size_t out_cap, size_t *out_len);
```

Frame one packet into `out` (capacity `out_cap`), writing the length to
*out_len. crc_type bits are set in the header even for an empty payload
(matching device TX), but a CRC trailer is appended only for a non-empty
payload (ERRATA E6). `seq` is taken modulo 256.
Returns 0 on success, -1 on payload-too-long, -2 on insufficient capacity.

### depz_event_type

```c
typedef enum {
    DEPZ_EV_PACKET,
    DEPZ_EV_TRASH,
    DEPZ_EV_CRC_ERROR
} depz_event_type;
```

### depz_event

```c
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
```

### depz_event_cb

```c
typedef void (*depz_event_cb)(const depz_event *ev, void *user);
```

### depz_parser

```c
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
```

### depz_parser_init

```c
void depz_parser_init(depz_parser *p);
```

### depz_parser_free

```c
void depz_parser_free(depz_parser *p);
```

### depz_parser_feed

```c
int depz_parser_feed(depz_parser *p, const uint8_t *data, size_t len, depz_event_cb cb, void *user);
```

Append `data` and drain every complete event, invoking `cb` (may be NULL)
for each. Event ordering is invariant to how the byte stream is chunked
(contract 01 §5); only Trash event *boundaries* depend on chunking.
Returns 0 on success, -1 on allocation failure.

## Common protocol

### depz_cmd

```c
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
```

### depz_rpt

```c
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
```

### depz_status

```c
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
```

### depz_sync_pin_mode

```c
typedef enum {
    DEPZ_SYNC_PIN_DISABLE   = 0x00,
    DEPZ_SYNC_PIN_IN        = 0x01,
    DEPZ_SYNC_PIN_OUT_START = 0x02,
    DEPZ_SYNC_PIN_OUT_END   = 0x03,
    DEPZ_SYNC_PIN_OUT_BOTH  = 0x04
} depz_sync_pin_mode;
```

### depz_status_report

```c
typedef struct { uint8_t cmd; uint8_t status; } depz_status_report;
```

### depz_text_report

```c
typedef struct { uint8_t cmd; char text[256]; } depz_text_report;
```

### depz_sync_time_report

```c
typedef struct { uint64_t pc_timestamp_us; uint64_t mcu_rx_us; uint64_t mcu_tx_us; } depz_sync_time_report;
```

### depz_temperature_report

```c
typedef struct { uint64_t timestamp_us; int16_t raw_decidegrees; } depz_temperature_report;
```

### depz_sequence_error_report

```c
typedef struct { uint8_t expected_seq; uint8_t received_seq; } depz_sequence_error_report;
```

### depz_sync_pin_config

```c
typedef struct { uint8_t pin; uint8_t mode; uint8_t polarity; } depz_sync_pin_config;
```

### depz_pack_sync_time

```c
size_t depz_pack_sync_time(uint64_t pc_timestamp_us, uint8_t *out);
```

Encoders (return payload length written).
/* 8 B */

### depz_pack_set_payload_crc_type

```c
size_t depz_pack_set_payload_crc_type(uint8_t crc_type, uint8_t *out);
```

/* 1 B */

### depz_pack_sync_pin_config

```c
size_t depz_pack_sync_pin_config(const depz_sync_pin_config *c, uint8_t *out);
```

/* 3 B */

### depz_unpack_status

```c
int depz_unpack_status(const uint8_t *p, size_t len, depz_status_report *out);
```

Decoders (return 0 on success, -1 on wrong payload length).

### depz_unpack_text

```c
int depz_unpack_text(const uint8_t *p, size_t len, depz_text_report *out);
```

### depz_unpack_sync_time

```c
int depz_unpack_sync_time(const uint8_t *p, size_t len, depz_sync_time_report *out);
```

### depz_unpack_temperature

```c
int depz_unpack_temperature(const uint8_t *p, size_t len, depz_temperature_report *out);
```

### depz_unpack_sequence_error

```c
int depz_unpack_sequence_error(const uint8_t *p, size_t len, depz_sequence_error_report *out);
```

### depz_unpack_sync_pin_config

```c
int depz_unpack_sync_pin_config(const uint8_t *p, size_t len, depz_sync_pin_config *out);
```

### depz_sync_time_offset_rtt

```c
void depz_sync_time_offset_rtt(int64_t t1, int64_t t2, int64_t t3, int64_t t4, int64_t *offset_us, int64_t *rtt_us);
```

Time-sync math (contract 02 §5). offset = ((T2-T1)+(T3-T4))/2 truncated
toward zero; rtt = (T4-T1)-(T3-T2).

## SR04

### depz_sr04_cmd

```c
typedef enum {
    DEPZ_SR04_GET_SAMPLE_PERIOD      = 0x32,
    DEPZ_SR04_SET_SAMPLE_PERIOD      = 0x33,
    DEPZ_SR04_GET_ECHO_DECAY         = 0x34,
    DEPZ_SR04_SET_ECHO_DECAY         = 0x35,
    DEPZ_SR04_MEASURE_ONCE           = 0x36,
    DEPZ_SR04_START_MEASUREMENT_LOOP = 0x37,
    DEPZ_SR04_STOP_MEASUREMENT_LOOP  = 0x38
} depz_sr04_cmd;
```

### depz_sr04_rpt

```c
typedef enum {
    DEPZ_SR04_RPT_DATA          = 0x91,
    DEPZ_SR04_RPT_SAMPLE_PERIOD = 0x92,
    DEPZ_SR04_RPT_ECHO_DECAY    = 0x93
} depz_sr04_rpt;
```

### DEPZ_SR04_ECHO_TIMEOUT

```c
#define DEPZ_SR04_ECHO_TIMEOUT       0xFFFFu
```

echo_time_us sentinel: no echo

### DEPZ_SR04_ECHO_DECAY_MIN_US

```c
#define DEPZ_SR04_ECHO_DECAY_MIN_US  4000u
```

### DEPZ_SR04_ECHO_DECAY_MAX_US

```c
#define DEPZ_SR04_ECHO_DECAY_MAX_US  65000u
```

### depz_sr04_data

```c
typedef struct {
    uint8_t  source_cmd;   /* 0x36 single shot (host or SYNC_IN), 0x37 loop */
    uint64_t timestamp_us;
    uint16_t echo_time_us; /* 0xFFFF = timeout sentinel */
} depz_sr04_data;
```

### depz_sr04_pack_sample_period

```c
size_t depz_sr04_pack_sample_period(uint32_t period_us, uint8_t *out);
```

/* 4 B */

### depz_sr04_pack_echo_decay

```c
size_t depz_sr04_pack_echo_decay(uint16_t decay_us, uint8_t *out);
```

/* 2 B */

### depz_sr04_unpack_data

```c
int depz_sr04_unpack_data(const uint8_t *p, size_t len, depz_sr04_data *out);
```

### depz_sr04_unpack_sample_period

```c
int depz_sr04_unpack_sample_period(const uint8_t *p, size_t len, uint32_t *out);
```

### depz_sr04_unpack_echo_decay

```c
int depz_sr04_unpack_echo_decay(const uint8_t *p, size_t len, uint16_t *out);
```

### depz_sr04_distance_mm

```c
bool depz_sr04_distance_mm(uint16_t echo_time_us, double air_temp_c, bool have_temp, double *out_mm);
```

Round-trip echo time -> distance in mm; returns false for the timeout
sentinel. Default 343 m/s; if air_temp_c is finite, c = 331.3 + 0.606*T.

## VL53L8 (ToF)

### depz_vl53l8_variant

```c
typedef enum {
    DEPZ_VL53L8_VARIANT_CX = 0, /* base ToF (dev-default)                    */
    DEPZ_VL53L8_VARIANT_CH = 1  /* CX + CNH compact-network-histograms (ED40)*/
} depz_vl53l8_variant;
```

Which ToF sensor produced a frame. The frame decoder is variant-agnostic
(CX and CH share the ranging-results layout); the enum exists to name the
CX/CH split and anchor the CH-only CNH extension point.

### DEPZ_VL53L8CX_FOOTER_ID_OFFSET

```c
#define DEPZ_VL53L8CX_FOOTER_ID_OFFSET 12
```

Footer-id offset in a decoded frame: the header id at [8..9] must equal
the footer id at [raw_len - N]. N = 12 for the CX / shared CH ranging
frame. CNH histogram frames (CH-only) are a different layout and are not
decoded here — see the CNH extension-point note in vl53l8_decode.c.

### DEPZ_VL53L8_CMD_READ_REG

```c
#define DEPZ_VL53L8_CMD_READ_REG     0x32
```

### DEPZ_VL53L8_CMD_WRITE_REG

```c
#define DEPZ_VL53L8_CMD_WRITE_REG    0x33
```

### DEPZ_VL53L8_CMD_START_STREAM

```c
#define DEPZ_VL53L8_CMD_START_STREAM 0x35
```

### DEPZ_VL53L8_CMD_STOP_STREAM

```c
#define DEPZ_VL53L8_CMD_STOP_STREAM  0x36
```

### DEPZ_VL53L8_RPT_REG_DATA

```c
#define DEPZ_VL53L8_RPT_REG_DATA     0x91
```

### DEPZ_VL53L8_RPT_FRAME

```c
#define DEPZ_VL53L8_RPT_FRAME        0x93
```

### DEPZ_VL53L8_STREAM_CHUNK_MAX

```c
#define DEPZ_VL53L8_STREAM_CHUNK_MAX 1528u
```

### DEPZ_VL53L8_STREAM_TOTAL_MAX

```c
#define DEPZ_VL53L8_STREAM_TOTAL_MAX 8192u
```

### DEPZ_VL53L8_RES_4X4

```c
#define DEPZ_VL53L8_RES_4X4          16
```

### DEPZ_VL53L8_RES_8X8

```c
#define DEPZ_VL53L8_RES_8X8          64
```

### DEPZ_VL53L8_MAX_ZONES

```c
#define DEPZ_VL53L8_MAX_ZONES        64
```

### depz_vl53l8_pack_read_reg

```c
size_t depz_vl53l8_pack_read_reg(uint16_t addr, uint16_t length, uint8_t *out);
```

Encoders (return payload length written).
/* 4 B */

### depz_vl53l8_pack_start_stream

```c
size_t depz_vl53l8_pack_start_stream(uint16_t frame_size, uint8_t *out);
```

/* 2 B */

### depz_vl53l8_chunk

```c
typedef struct {
    uint64_t timestamp_us;
    uint16_t full_size;
    uint16_t offset;
    const uint8_t *data;  /* points into the report payload */
    size_t   data_len;
} depz_vl53l8_chunk;
```

One RPT_VL53_FRAME chunk of a (possibly multi-chunk) sensor frame.

### depz_vl53l8_unpack_chunk

```c
int depz_vl53l8_unpack_chunk(const uint8_t *payload, size_t len, depz_vl53l8_chunk *out);
```

Parse a RPT_VL53_FRAME payload (>=12 B). Returns 0 on success, -1 on short.

### depz_vl53l8_reg_data

```c
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us;
    const uint8_t *data;
    size_t   data_len;
} depz_vl53l8_reg_data;
```

RPT_REG_DATA payload: echoed opcode, u64 timestamp, register bytes.

### depz_vl53l8_unpack_reg_data

```c
int depz_vl53l8_unpack_reg_data(const uint8_t *payload, size_t len, depz_vl53l8_reg_data *out);
```

/* needs >=9 B */

### depz_vl53l8_reassembler

```c
typedef struct {
    uint8_t  buf[DEPZ_VL53L8_STREAM_TOTAL_MAX];
    size_t   len;
    uint16_t full_size;
    uint64_t timestamp_us;
    uint64_t completed;
    uint64_t discarded;
} depz_vl53l8_reassembler;
```

### depz_vl53l8_reasm_init

```c
void depz_vl53l8_reasm_init(depz_vl53l8_reassembler *r);
```

### depz_vl53l8_reasm_feed

```c
int depz_vl53l8_reasm_feed(depz_vl53l8_reassembler *r, const depz_vl53l8_chunk *c, const uint8_t **frame, size_t *frame_len, uint64_t *ts);
```

Feed one chunk. Returns 1 when a frame completes (sets *frame -> the
internal buffer, *frame_len, *ts), 0 otherwise. The frame pointer stays
valid until the next feed.

### depz_vl53l8_frame

```c
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
```

One decoded ranging frame. Arrays are valid for [0, resolution); zone index
runs row-major. Values are RAW fixed-point integers exactly as on the wire
(no float scaling — the host applies range_sigma/128, signal/kcps, etc.).
distance_mm is the ST-scaled value (raw/4, floored) to match GetRangingData.

### depz_vl53l8_decode_frame

```c
int depz_vl53l8_decode_frame(const uint8_t *raw, size_t raw_len, uint64_t timestamp_us, depz_vl53l8_frame *out);
```

Decode a reassembled raw ranging frame. The layout is shared by BOTH
VL53L8CX and VL53L8CH (the CX firmware layout; CH emits the same ranging
results), so this decoder serves both variants. Returns 0 on success, -1 on
corrupted frame (header/footer id mismatch), -2 on bad length.
`timestamp_us` is carried through from the reassembler.

NOTE: the CH-only CNH (compact-network-histogram) frame is a distinct layout
and is NOT handled here — see the CNH extension-point note in
vl53l8_decode.c.

### DEPZ_VL53L8_CNH_MAX_AGGREGATES

```c
#define DEPZ_VL53L8_CNH_MAX_AGGREGATES 64
```

MI_MAP_ID_LENGTH (max device zones mapped to aggregates, 8x8 resolution).

### DEPZ_VL53L8_CNH_MAX_FEATURE

```c
#define DEPZ_VL53L8_CNH_MAX_FEATURE    255
```

CNH feature_length is packed as a uint8 on the device (num_bins & 0xFF).

### depz_vl53l8ch_cnh_config

```c
typedef struct {
    int nb_of_aggregates; /* 1..DEPZ_VL53L8_CNH_MAX_AGGREGATES */
    int feature_length;   /* 1..DEPZ_VL53L8_CNH_MAX_FEATURE    */
} depz_vl53l8ch_cnh_config;
```

Config needed to decode a CNH block: the aggregate count and per-aggregate
feature (bin) length that the device was configured with (must match the
cnh_send_config values).

### depz_vl53l8ch_cnh_frame

```c
typedef struct {
    uint32_t ref_residual_word; /* raw u32; float = word / 2048.0            */
    int      nb_aggregates;
    int      feature_length;
    int32_t  hist_raw[DEPZ_VL53L8_CNH_MAX_AGGREGATES][DEPZ_VL53L8_CNH_MAX_FEATURE];
    int8_t   hist_scaler[DEPZ_VL53L8_CNH_MAX_AGGREGATES][DEPZ_VL53L8_CNH_MAX_FEATURE];
} depz_vl53l8ch_cnh_frame;
```

Decoded CNH block: per-aggregate raw histogram and per-bin scaler. The
float histogram value for bin f of aggregate a is
  hist_raw[a][f] / 2^hist_scaler[a][f].
Only the first nb_aggregates rows and feature_length columns are valid.
This struct is ~82 KB — heap-allocate it.

### depz_vl53l8ch_decode_cnh

```c
int depz_vl53l8ch_decode_cnh(const depz_vl53l8ch_cnh_config *cfg, const uint8_t *raw, size_t raw_len, depz_vl53l8ch_cnh_frame *out);
```

Decode a captured CNH data block. `raw` is the CNH block already in decode
order (word-swapped exactly like the standard ranging blocks). Returns 0 on
success; -1 on out-of-range config, -2/-3 on a raw buffer too short for the
header / computed layout.

### DEPZ_VL53L8_DIST_MM

```c
#define DEPZ_VL53L8_DIST_MM               1
```

Detection-threshold measurement selectors (get divides / set multiplies).

### DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS

```c
#define DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS  2
```

### DEPZ_VL53L8_RANGE_SIGMA_MM

```c
#define DEPZ_VL53L8_RANGE_SIGMA_MM        4
```

### DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS

```c
#define DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS 8
```

### DEPZ_VL53L8_NB_TARGET_DETECTED

```c
#define DEPZ_VL53L8_NB_TARGET_DETECTED    9
```

### DEPZ_VL53L8_TAR_STATUS

```c
#define DEPZ_VL53L8_TAR_STATUS            12
```

### DEPZ_VL53L8_NB_SPADS_ENABLED

```c
#define DEPZ_VL53L8_NB_SPADS_ENABLED      13
```

### DEPZ_VL53L8_MOTION_INDICATOR

```c
#define DEPZ_VL53L8_MOTION_INDICATOR      19
```

### DEPZ_VL53L8_POWER_MODE_SLEEP

```c
#define DEPZ_VL53L8_POWER_MODE_SLEEP      0
```

### DEPZ_VL53L8_POWER_MODE_WAKEUP

```c
#define DEPZ_VL53L8_POWER_MODE_WAKEUP     1
```

### DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP

```c
#define DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP 2
```

### DEPZ_VL53L8_NB_THRESHOLDS

```c
#define DEPZ_VL53L8_NB_THRESHOLDS         64
```

### DEPZ_VL53L8_THRESH_START_SIZE

```c
#define DEPZ_VL53L8_THRESH_START_SIZE     (DEPZ_VL53L8_NB_THRESHOLDS * 12)
```

768

### DEPZ_VL53L8_MOTION_CFG_SIZE

```c
#define DEPZ_VL53L8_MOTION_CFG_SIZE       156
```

### depz_vl53l8_xtalk_margin_to_raw

```c
uint32_t depz_vl53l8_xtalk_margin_to_raw(double margin_kcps);
```

Xtalk margin (kcps/SPAD) -> raw DCI value = round(kcps * 2048).

### depz_vl53l8_xtalk_margin_from_raw

```c
double depz_vl53l8_xtalk_margin_from_raw(uint32_t raw);
```

Inverse: raw DCI -> kcps/SPAD (raw / 2048.0).

### depz_vl53l8_threshold

```c
typedef struct {
    int32_t low_thresh;
    int32_t high_thresh;
    uint8_t measurement;
    uint8_t type;
    uint8_t zone_num;
    uint8_t operation;
} depz_vl53l8_threshold;
```

One detection-threshold entry in real units (mirror of DetectionThreshold).

### depz_vl53l8_pack_thresholds

```c
void depz_vl53l8_pack_thresholds(const depz_vl53l8_threshold *th, size_t n, uint8_t start[DEPZ_VL53L8_THRESH_START_SIZE], uint8_t valid[8]);
```

Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload
(768 B) plus the 8-byte valid-status block (all 0x05). Missing entries are
zero-filled. low/high are scaled by the entry's measurement selector.

### depz_vl53l8_motion_cfg_default_pack

```c
int depz_vl53l8_motion_cfg_default_pack(int resolution, uint8_t out[DEPZ_VL53L8_MOTION_CFG_SIZE]);
```

Build the default 156-byte VL53L8CX_Motion_Configuration bytes that
motion_indicator_init programs for the given resolution (16 | 64). This is
the pure codec half of the motion indicator; the live DCI write is out of
scope. Returns 0 on success, -1 on bad resolution.

## BNO086 (IMU)

### DEPZ_SHTP_HEADER_SIZE

```c
#define DEPZ_SHTP_HEADER_SIZE   4
```

### DEPZ_SHTP_LENGTH_MASK

```c
#define DEPZ_SHTP_LENGTH_MASK   0x7FFFu
```

### DEPZ_SHTP_CONTINUATION

```c
#define DEPZ_SHTP_CONTINUATION  0x8000u
```

### DEPZ_SHTP_NUM_CHANNELS

```c
#define DEPZ_SHTP_NUM_CHANNELS  6
```

### DEPZ_SHTP_MAX_TX_FRAME

```c
#define DEPZ_SHTP_MAX_TX_FRAME  64
```

one MCU transmit slot (ERRATA E2)

### depz_shtp_channel_id

```c
typedef enum {
    DEPZ_SHTP_CH_COMMAND     = 0,
    DEPZ_SHTP_CH_EXECUTABLE  = 1,
    DEPZ_SHTP_CH_CONTROL     = 2,
    DEPZ_SHTP_CH_INPUT_NORMAL = 3,
    DEPZ_SHTP_CH_INPUT_WAKE  = 4,
    DEPZ_SHTP_CH_GYRO_RV     = 5
} depz_shtp_channel_id;
```

### depz_shtp_header

```c
typedef struct {
    uint16_t length;      /* cargo length incl. this 4-byte header */
    uint8_t  channel;
    uint8_t  seq;
    bool     continuation;
} depz_shtp_header;
```

### depz_shtp_pack_header

```c
void depz_shtp_pack_header(const depz_shtp_header *hdr, uint8_t out[4]);
```

### depz_shtp_unpack_header

```c
void depz_shtp_unpack_header(const uint8_t in[4], depz_shtp_header *out);
```

### depz_shtp_rx_channel

```c
typedef struct {
    uint8_t *buf;
    size_t   received;
    size_t   expected;
    size_t   cap;
    uint8_t  seq;
    bool     active;
} depz_shtp_rx_channel;
```

Per-channel TX sequence counters + RX cargo reassembly.

### depz_shtp_layer

```c
typedef struct {
    depz_shtp_rx_channel rx[DEPZ_SHTP_NUM_CHANNELS];
    uint8_t  tx_seq[DEPZ_SHTP_NUM_CHANNELS];
    uint64_t discarded;   /* incomplete/orphan cargos thrown away */
} depz_shtp_layer;
```

### depz_shtp_init

```c
void depz_shtp_init(depz_shtp_layer *l);
```

### depz_shtp_free

```c
void depz_shtp_free(depz_shtp_layer *l);
```

### depz_shtp_next_frame

```c
size_t depz_shtp_next_frame(depz_shtp_layer *l, uint8_t channel, const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_cap);
```

Build one single-fragment TX frame, consuming the channel's TX seq. Returns
frame length, or 0 on error (bad channel / payload exceeds the MCU slot /
capacity).

### depz_shtp_cargo

```c
typedef struct {
    uint8_t  channel;
    uint8_t  seq;         /* first fragment's seq */
    const uint8_t *payload;
    size_t   payload_len;
} depz_shtp_cargo;
```

One reassembled cargo (payload excludes all SHTP headers).

### depz_shtp_feed

```c
int depz_shtp_feed(depz_shtp_layer *l, const uint8_t *frame, size_t frame_len, depz_shtp_cargo *out);
```

Feed one inbound frame. Returns 1 when a cargo completes (fills *out, whose
payload points into the channel buffer, valid until the next feed on that
channel), 0 otherwise. Returns -1 on allocation failure.

### depz_bno_pack_set_feature

```c
size_t depz_bno_pack_set_feature(uint8_t sensor_id, uint8_t flags, uint16_t sensitivity, uint32_t interval_us, uint32_t batch_us, uint32_t cfg_word, uint8_t out[17]);
```

SET_FEATURE_COMMAND 0xFD, 17 B.

### depz_bno_pack_get_feature_request

```c
size_t depz_bno_pack_get_feature_request(uint8_t sensor_id, uint8_t out[2]);
```

GET_FEATURE_REQUEST 0xFE, 2 B.

### depz_bno_pack_product_id_request

```c
size_t depz_bno_pack_product_id_request(uint8_t out[2]);
```

PRODUCT_ID_REQUEST 0xF9, 2 B.

### depz_bno_pack_command_request

```c
size_t depz_bno_pack_command_request(uint8_t seq, uint8_t command, const uint8_t *params, size_t nparams, uint8_t out[12]);
```

COMMAND_REQUEST 0xF2, 12 B (params zero-padded, up to 9 B).

### depz_bno_pack_frs_read_request

```c
size_t depz_bno_pack_frs_read_request(uint16_t frs_type, uint16_t offset_words, uint16_t block_words, uint8_t out[8]);
```

FRS_READ_REQUEST 0xF4, 8 B.

### depz_bno_pack_frs_write_request

```c
size_t depz_bno_pack_frs_write_request(uint16_t frs_type, uint16_t length_words, uint8_t out[6]);
```

FRS_WRITE_REQUEST 0xF7, 6 B.

### depz_bno_pack_frs_write_data

```c
size_t depz_bno_pack_frs_write_data(uint16_t offset_words, const uint32_t *words, size_t nwords, uint8_t *out, size_t out_cap);
```

FRS_WRITE_DATA 0xF6, 4 + 4*nwords B. Returns 0 on capacity error.

### DEPZ_BNO_BASE_TIMESTAMP_REF

```c
#define DEPZ_BNO_BASE_TIMESTAMP_REF 0xFB
```

### DEPZ_BNO_TIMESTAMP_REBASE

```c
#define DEPZ_BNO_TIMESTAMP_REBASE   0xFA
```

### depz_bno_report_type

```c
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
```

### depz_bno_report_type_str

```c
const char *depz_bno_report_type_str(depz_bno_report_type t);
```

Report catalog name (matches the vector "type" strings).

### depz_bno_report

```c
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
```

One decoded report. Only the fields relevant to `type` are meaningful. All
*_raw values are the wire integers (no Q-point scaling).

### depz_bno_parse_input_cargo

```c
size_t depz_bno_parse_input_cargo(const uint8_t *payload, size_t len, uint64_t capture_timestamp_us, depz_bno_report *out, size_t cap);
```

Parse a channel-3/4 input cargo into typed reports. `capture_timestamp_us`
is the bridge RPT_DATA capture time. Writes up to `cap` reports into `out`,
returns the count. Handles 0xFB base and 0xFA rebase; an unknown report id
stops the parse (last entry is UNKNOWN_REPORT).

### depz_bno_parse_gyro_rv

```c
int depz_bno_parse_gyro_rv(const uint8_t *payload, size_t len, uint64_t capture_timestamp_us, depz_bno_report *out);
```

Parse a channel-5 gyro-integrated RV cargo (dense 7×i16, optionally 0xFB +
i32 + u16 prefixed). Returns 0 on success (fills *out), -1 on short cargo.

## Bootloader / firmware update

### DEPZ_FWDEPZ_HEADER_SIZE

```c
#define DEPZ_FWDEPZ_HEADER_SIZE 64u
```

### depz_fwdepz_result

```c
typedef enum {
    DEPZ_FWDEPZ_OK = 0,
    DEPZ_FWDEPZ_ERR_TOO_SHORT,
    DEPZ_FWDEPZ_ERR_MAGIC,
    DEPZ_FWDEPZ_ERR_HEADER_CRC,
    DEPZ_FWDEPZ_ERR_SIZE
} depz_fwdepz_result;
```

### depz_fwdepz_image

```c
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
```

### depz_fwdepz_parse

```c
depz_fwdepz_result depz_fwdepz_parse(const uint8_t *blob, size_t len, depz_fwdepz_image *out);
```

### depz_bl_cmd

```c
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
```

Bootloader opcode space (contract 06 §1) — DIFFERENT from application.

### depz_flash_info

```c
typedef struct { uint16_t page_size; uint32_t app_start; uint32_t app_size; } depz_flash_info;
```

### depz_bl_pack_write_page

```c
size_t depz_bl_pack_write_page(uint32_t addr, const uint8_t *data, size_t data_len, uint8_t *out, size_t out_cap);
```

/* 6 + data_len */

### depz_bl_pack_read_page

```c
size_t depz_bl_pack_read_page(uint32_t addr, uint16_t size, uint8_t *out);
```

/* 6 B */

### depz_bl_unpack_flash_info

```c
int depz_bl_unpack_flash_info(const uint8_t *p, size_t len, depz_flash_info *out);
```

## Datasets (record & replay)

### depz_dataset

```c
typedef struct depz_dataset depz_dataset;
```

### depz_dataset_device

```c
typedef struct {
    char    id[32];
    char    serial[64];
    char    sensor_type[32];
    char    software_name[64];
    int64_t offset_us;   /* time_sync.offset_us */
    int64_t rtt_us;      /* time_sync.rtt_us    */
} depz_dataset_device;
```

### depz_dataset_record

```c
typedef struct {
    char        device_id[32];
    int64_t     t_host_us;
    char        kind[32];
    const void *value;   /* opaque JSON node — read via depz_dataset_value_* */
} depz_dataset_record;
```

### depz_dataset_parse

```c
depz_dataset *depz_dataset_parse(const char *text, size_t len);
```

Parse a full .depzdata blob. Returns NULL on parse error / bad schema.
Records are exposed in stable merge order (by t_host_us, then file order).

### depz_dataset_free

```c
void depz_dataset_free(depz_dataset *d);
```

### depz_dataset_schema

```c
const char *depz_dataset_schema(const depz_dataset *d);
```

### depz_dataset_device_count

```c
size_t depz_dataset_device_count(const depz_dataset *d);
```

### depz_dataset_get_device

```c
int depz_dataset_get_device(const depz_dataset *d, size_t i, depz_dataset_device *out);
```

### depz_dataset_record_count

```c
size_t depz_dataset_record_count(const depz_dataset *d);
```

### depz_dataset_get_record

```c
int depz_dataset_get_record(const depz_dataset *d, size_t i, depz_dataset_record *out);
```

### depz_dataset_value_int

```c
int depz_dataset_value_int(const depz_dataset_record *r, const char *key, int64_t *out);
```

Read a scalar out of a record's `value` object. Return 0 on success.

### depz_dataset_value_str

```c
int depz_dataset_value_str(const depz_dataset_record *r, const char *key, char *out, size_t cap);
```
