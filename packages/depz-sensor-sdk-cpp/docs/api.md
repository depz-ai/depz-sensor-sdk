# API reference

Auto-generated from the public headers under `include/depz/` (their
declarations and `//` doc-comments) by `scripts/gen_api_md.py` — run
`cmake --build build --target docs` (or `python3 scripts/gen_api_md.py`)
to regenerate. Edit the doc-comments in the headers, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · [BNO086](bno086/api.md) · [BNO055](bno055/api.md).

Two layers: the *decode layer* (every header but `depz/device.hpp`)
takes bytes and returns typed values, no I/O; the *live-hardware
layer* (`depz/device.hpp`, a wrapper over the C SDK) opens a board,
runs its commands and delivers its data — see the last section.

## Contents

- **Transport (framing & CRC)**: [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`MAGIC0`](#magic0), [`MAGIC1`](#magic1), [`CrcType`](#crctype), [`payload_crc_bytes`](#payload_crc_bytes), [`build_packet`](#build_packet), [`Packet`](#packet), [`Trash`](#trash), [`CrcError`](#crcerror), [`ParserEvent`](#parserevent), [`PacketParser`](#packetparser), [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false)
- **Common protocol**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`UNSOLICITED`](#unsolicited), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`pack_sync_time`](#pack_sync_time), [`pack_set_payload_crc_type`](#pack_set_payload_crc_type), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string)
- **Identity**: [`SensorType`](#sensortype), [`to_string`](#to_string), [`DeviceMode`](#devicemode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name)
- **USB identity**: [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`PID_VL53L8`](#pid_vl53l8), [`PID_VL53L4CD`](#pid_vl53l4cd), [`PID_BNO086`](#pid_bno086), [`DEV_USB_VID`](#dev_usb_vid), [`DEV_USB_PID`](#dev_usb_pid), [`DEPZ_PID_RANGE_LO`](#depz_pid_range_lo), [`DEPZ_PID_RANGE_HI`](#depz_pid_range_hi), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortInfo`](#portinfo), [`order_by_serial`](#order_by_serial)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L4CD (ToF)**: [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`MODEL_ID`](#model_id), [`CONFIG_ADDR`](#config_addr), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`Vl53l4Info`](#vl53l4info), [`StreamData`](#streamdata), [`Vl53l4Result`](#vl53l4result), [`parse_result_block`](#parse_result_block), [`RangeTimingRegs`](#rangetimingregs), [`range_timing_registers`](#range_timing_registers), [`RangeTiming`](#rangetiming), [`decode_range_timing`](#decode_range_timing), [`offset_raw`](#offset_raw), [`decode_offset`](#decode_offset), [`xtalk_raw`](#xtalk_raw), [`decode_xtalk`](#decode_xtalk), [`signal_threshold_raw`](#signal_threshold_raw), [`decode_signal_threshold`](#decode_signal_threshold), [`sigma_threshold_raw`](#sigma_threshold_raw), [`decode_sigma_threshold`](#decode_sigma_threshold), [`default_configuration`](#default_configuration), [`config_block`](#config_block)
- **VL53L8 (ToF)**: [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`Variant`](#variant), [`FOOTER_ID_OFF_CX`](#footer_id_off_cx), [`FOOTER_ID_OFF_CH`](#footer_id_off_ch), [`CNH_DATA_IDX`](#cnh_data_idx), [`footer_id_off`](#footer_id_off), [`FrameChunk`](#framechunk), [`FrameReassembler`](#framereassembler), [`Vl53l8Frame`](#vl53l8frame), [`swap_buffer`](#swap_buffer), [`decode_frame`](#decode_frame), [`xtalk_margin_raw`](#xtalk_margin_raw), [`xtalk_margin_kcps`](#xtalk_margin_kcps), [`ThreshMeasurement`](#threshmeasurement), [`DetectionThreshold`](#detectionthreshold), [`NB_THRESHOLDS`](#nb_thresholds), [`pack_detection_thresholds`](#pack_detection_thresholds), [`detection_thresholds_valid_status`](#detection_thresholds_valid_status), [`MotionConfig`](#motionconfig), [`motion_config_init`](#motion_config_init), [`CNH_MAX_AGGREGATES`](#cnh_max_aggregates), [`CNH_MAX_FEATURE_LENGTH`](#cnh_max_feature_length), [`CNH_PER_HEADER_WORDS`](#cnh_per_header_words), [`CNH_PER_BUFFER_HEADER_WORDS`](#cnh_per_buffer_header_words), [`CNH_PER_HEADER_BUFFER_INFO_IDX`](#cnh_per_header_buffer_info_idx), [`CNH_PER_HEADER_FLAGS_IDX`](#cnh_per_header_flags_idx), [`CNH_BUFFER_INFO_WORDS_MASK`](#cnh_buffer_info_words_mask), [`CNH_MI_STATE_PING`](#cnh_mi_state_ping), [`CnhAggregate`](#cnhaggregate), [`CnhFrame`](#cnhframe), [`decode_cnh`](#decode_cnh)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`PinAction`](#pinaction), [`I2cError`](#i2cerror), [`READ_MAX_LEN`](#read_max_len), [`WRITE_MAX_LEN`](#write_max_len), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`INFO_SIZE`](#info_size), [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`I2C_SPEED_STEPS_KHZ`](#i2c_speed_steps_khz), [`FOOTER_ID_OFF`](#footer_id_off), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_pin_ctrl`](#pack_pin_ctrl), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`Vl53l7Info`](#vl53l7info), [`Model`](#model), [`to_string`](#to_string), [`resolve_model`](#resolve_model), [`infer_resolution`](#infer_resolution), [`decode_frame`](#decode_frame)
- **VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)**: [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`RegData`](#regdata), [`StreamData`](#streamdata), [`CLEAR_STEPS_MAX`](#clear_steps_max), [`INFO_SIZE`](#info_size), [`DIE_BLOCK_ADDR`](#die_block_addr), [`DIE_BLOCK_LEN`](#die_block_len), [`L0X_BLOCK_ADDR`](#l0x_block_addr), [`L0X_BLOCK_LEN`](#l0x_block_len), [`HISTOGRAM_BLOCK_ADDR`](#histogram_block_addr), [`HISTOGRAM_BLOCK_LEN`](#histogram_block_len), [`HISTOGRAM_BINS`](#histogram_bins), [`ClearStep`](#clearstep), [`pack_read_reg`](#pack_read_reg), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_addr_width`](#pack_set_addr_width), [`Vl53lxInfo`](#vl53lxinfo), [`DriverKind`](#driverkind), [`to_string`](#to_string), [`Product`](#product), [`products`](#products), [`find_product`](#find_product), [`product_from_board_name`](#product_from_board_name), [`SensorClass`](#sensorclass), [`resolve_class`](#resolve_class), [`DieVariant`](#dievariant), [`DieResult`](#dieresult), [`decode_die_block`](#decode_die_block), [`L0xRaw`](#l0xraw), [`decode_l0x_raw`](#decode_l0x_raw), [`HistogramRaw`](#histogramraw), [`decode_histogram_raw`](#decode_histogram_raw)
- **BNO086 (IMU)**: [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`ShtpCargo`](#shtpcargo), [`shtp_build_frame`](#shtp_build_frame), [`shtp_fragment_cargo`](#shtp_fragment_cargo), [`ShtpLayer`](#shtplayer), [`sh2_build_set_feature`](#sh2_build_set_feature), [`sh2_build_get_feature_request`](#sh2_build_get_feature_request), [`sh2_build_product_id_request`](#sh2_build_product_id_request), [`sh2_build_command_request`](#sh2_build_command_request), [`sh2_build_frs_read_request`](#sh2_build_frs_read_request), [`sh2_build_frs_write_request`](#sh2_build_frs_write_request), [`sh2_build_frs_write_data`](#sh2_build_frs_write_data), [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`ReportType`](#reporttype), [`Report`](#report), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`XFER_MAX`](#xfer_max), [`INFO_SIZE`](#info_size), [`TRIGGER_TIMER`](#trigger_timer), [`TRIGGER_INT`](#trigger_int), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`Bno055Info`](#bno055info), [`StreamData`](#streamdata), [`REG_CHIP_ID`](#reg_chip_id), [`REG_PAGE_ID`](#reg_page_id), [`REG_ACC_DATA`](#reg_acc_data), [`REG_MAG_DATA`](#reg_mag_data), [`REG_GYR_DATA`](#reg_gyr_data), [`REG_EUL_DATA`](#reg_eul_data), [`REG_QUA_DATA`](#reg_qua_data), [`REG_LIA_DATA`](#reg_lia_data), [`REG_GRV_DATA`](#reg_grv_data), [`REG_TEMP`](#reg_temp), [`REG_CALIB_STAT`](#reg_calib_stat), [`REG_ST_RESULT`](#reg_st_result), [`REG_INT_STA`](#reg_int_sta), [`REG_SYS_CLK_STATUS`](#reg_sys_clk_status), [`REG_SYS_STATUS`](#reg_sys_status), [`REG_SYS_ERR`](#reg_sys_err), [`REG_UNIT_SEL`](#reg_unit_sel), [`REG_OPR_MODE`](#reg_opr_mode), [`REG_PWR_MODE`](#reg_pwr_mode), [`REG_SYS_TRIGGER`](#reg_sys_trigger), [`REG_TEMP_SOURCE`](#reg_temp_source), [`REG_AXIS_MAP_CONFIG`](#reg_axis_map_config), [`REG_AXIS_MAP_SIGN`](#reg_axis_map_sign), [`REG_SIC_MATRIX`](#reg_sic_matrix), [`REG_CALIB_PROFILE`](#reg_calib_profile), [`REG1_ACC_CONFIG`](#reg1_acc_config), [`REG1_MAG_CONFIG`](#reg1_mag_config), [`REG1_GYR_CONFIG_0`](#reg1_gyr_config_0), [`REG1_GYR_CONFIG_1`](#reg1_gyr_config_1), [`REG1_INT_MSK`](#reg1_int_msk), [`REG1_INT_EN`](#reg1_int_en), [`REG1_UNIQUE_ID`](#reg1_unique_id), [`FULL_BLOCK_ADDR`](#full_block_addr), [`FULL_BLOCK_LEN`](#full_block_len), [`QUAT_BLOCK_ADDR`](#quat_block_addr), [`QUAT_BLOCK_LEN`](#quat_block_len), [`OprMode`](#oprmode), [`UNIT_ACC_MG`](#unit_acc_mg), [`UNIT_GYR_RPS`](#unit_gyr_rps), [`UNIT_EUL_RAD`](#unit_eul_rad), [`UNIT_TEMP_F`](#unit_temp_f), [`UNIT_ORI_ANDROID`](#unit_ori_android), [`Units`](#units), [`MAG_LSB`](#mag_lsb), [`QUAT_LSB`](#quat_lsb), [`FUSION_ACCEL_LSB`](#fusion_accel_lsb), [`CalibStatus`](#calibstatus), [`CALIB_PROFILE_LEN`](#calib_profile_len), [`CalibrationProfile`](#calibrationprofile), [`AXIS_X`](#axis_x), [`AXIS_Y`](#axis_y), [`AXIS_Z`](#axis_z), [`AxisRemap`](#axisremap), [`PLACEMENTS`](#placements), [`placement`](#placement), [`AccelConfig`](#accelconfig), [`GyroConfig`](#gyroconfig), [`MagConfig`](#magconfig), [`Vec3`](#vec3), [`Quat`](#quat), [`RawBlock`](#rawblock), [`decode_block`](#decode_block)
- **Bootloader / firmware**: [`FWDEPZ_HEADER_SIZE`](#fwdepz_header_size), [`FWDEPZ_MAGIC`](#fwdepz_magic), [`BlCmd`](#blcmd), [`BlRpt`](#blrpt), [`BlStatus`](#blstatus), [`FlashInfo`](#flashinfo), [`pack_write_page`](#pack_write_page), [`pack_read_page`](#pack_read_page), [`FwDepzErrorKind`](#fwdepzerrorkind), [`FwDepzError`](#fwdepzerror), [`FwDepzImage`](#fwdepzimage)
- **Datasets**: [`SCHEMA_PREFIX`](#schema_prefix), [`TimeSync`](#timesync), [`DeviceMeta`](#devicemeta), [`Record`](#record), [`Reader`](#reader)
- **Live hardware (links, device, discovery)**: [`Error`](#error), [`ArgumentError`](#argumenterror), [`IoError`](#ioerror), [`DeviceLostError`](#devicelosterror), [`TimeoutError`](#timeouterror), [`ProtocolError`](#protocolerror), [`NoDeviceError`](#nodeviceerror), [`WrongTypeError`](#wrongtypeerror), [`ReplayMismatchError`](#replaymismatcherror), [`BootloaderModeError`](#bootloadermodeerror), [`StatusError`](#statuserror), [`BusyError`](#busyerror), [`Link`](#link), [`Stream`](#stream), [`LinkStats`](#linkstats), [`TimeSync`](#timesync), [`DeviceEvent`](#deviceevent), [`host_now_us`](#host_now_us), [`Device`](#device), [`Sr04Measurement`](#sr04measurement), [`Sr04`](#sr04), [`Vl53l4cdMeasurement`](#vl53l4cdmeasurement), [`DetectionWindow`](#detectionwindow), [`DetectionThresholds`](#detectionthresholds), [`Vl53l4cd`](#vl53l4cd), [`Vl53l8Model`](#vl53l8model), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8LiveFrame`](#vl53l8liveframe), [`CnhSetup`](#cnhsetup), [`Vl53l8Progress`](#vl53l8progress), [`Vl53l8`](#vl53l8), [`Bno055Sample`](#bno055sample), [`Bno055Status`](#bno055status), [`Bno055`](#bno055), [`Vl53lxCap`](#vl53lxcap), [`Vl53lxTarget`](#vl53lxtarget), [`Vl53lxBins`](#vl53lxbins), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lx`](#vl53lx), [`SensorId`](#sensorid), [`TareBasis`](#tarebasis), [`TARE_X`](#tare_x), [`Feature`](#feature), [`FeatureRequest`](#featurerequest), [`ProductId`](#productid), [`CalibrationConfig`](#calibrationconfig), [`ErrorRecord`](#errorrecord), [`Counts`](#counts), [`CommandResponse`](#commandresponse), [`SensorMetadata`](#sensormetadata), [`q_point`](#q_point), [`Bno086Report`](#bno086report), [`Bno086`](#bno086), [`SerialPortInfo`](#serialportinfo), [`DeviceInfo`](#deviceinfo), [`list_serial_ports`](#list_serial_ports), [`probe_port`](#probe_port), [`list_depz_devices`](#list_depz_devices), [`OpenOptions`](#openoptions), [`open_device`](#open_device), [`open_sr04`](#open_sr04), [`open_vl53l4cd`](#open_vl53l4cd), [`open_vl53l8`](#open_vl53l8), [`open_bno055`](#open_bno055), [`open_bno086`](#open_bno086), [`open_vl53lx`](#open_vl53lx)

## Transport (framing & CRC)

### HEADER_SIZE

```cpp
inline constexpr std::size_t HEADER_SIZE = 7;
```

### MAX_PAYLOAD

```cpp
inline constexpr std::uint16_t MAX_PAYLOAD = 0x3FFF;
```

### MAGIC0

```cpp
inline constexpr std::byte MAGIC0 = std::byte;
```

### MAGIC1

```cpp
inline constexpr std::byte MAGIC1 = std::byte;
```

### CrcType

```cpp
enum class CrcType : std::uint8_t {
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
};
```

### payload_crc_bytes

```cpp
bytes payload_crc_bytes(CrcType crc_type, byte_span payload);
```

CRC trailer for a payload; empty payloads never carry CRC bytes (ERRATA E6).

### build_packet

```cpp
bytes build_packet(std::uint8_t cmd, byte_span payload = {}, std::uint8_t seq = 0, CrcType crc_type = CrcType::None);
```

Frame one packet. `crc_type` bits are set in the header even for an empty
payload (matching device TX), but CRC bytes are appended only for non-empty
payloads. Throws std::length_error if payload exceeds MAX_PAYLOAD.

### Packet

```cpp
struct Packet {
    std::uint8_t cmd;
    std::uint8_t seq;
    bytes payload;
};
```

### Trash

```cpp
struct Trash {
    bytes data;
};
```

Bytes discarded while hunting for a valid frame. Boundaries between
consecutive Trash events depend on read chunking; only the concatenated
byte stream is deterministic.

### CrcError

```cpp
struct CrcError {
    std::uint8_t cmd;
    std::uint8_t seq;
};
```

A frame with a valid header whose payload CRC failed; dropped.

### ParserEvent

```cpp
using ParserEvent = std::variant<Packet, Trash, CrcError>;
```

### PacketParser

```cpp
class PacketParser {
    std::size_t packets = 0;
    std::size_t crc_errors = 0;
    std::size_t header_errors = 0;
    std::size_t trash_bytes = 0;
};
```

Incremental frame parser. Feed arbitrary byte chunks; get events. Event
order is invariant to chunking (contract 01 §5) except Trash event
boundaries — concatenate Trash data when comparing streams.

#### PacketParser.feed

```cpp
std::vector<ParserEvent> feed(byte_span data);
```

#### PacketParser.residue

```cpp
byte_span residue() const noexcept;
```

Bytes still buffered (unconsumed residue), e.g. a partial frame.

### crc8_maxim

```cpp
std::uint8_t crc8_maxim(byte_span data) noexcept;
```

CRC-8/MAXIM: poly 0x31 reflected (0x8C), init 0x00, xorout 0x00.

### crc16_modbus

```cpp
std::uint16_t crc16_modbus(byte_span data) noexcept;
```

CRC-16/MODBUS: poly 0x8005 reflected (0xA001), init 0xFFFF, xorout 0x0000.

### crc32_iso_hdlc

```cpp
std::uint32_t crc32_iso_hdlc(byte_span data) noexcept;
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected (0xEDB88320), init/xorout 0xFFFFFFFF.

### crc16_ccitt_false

```cpp
std::uint16_t crc16_ccitt_false(byte_span data) noexcept;
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
Used only for the `.fwdepz` file header (contract 06), never on the wire.

## Common protocol

### Cmd

```cpp
enum class Cmd : std::uint8_t {
    Bootloader = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetNameActiveSoftware = 0x04,
    GetSerial = 0x05,
    SyncTime = 0x06,
    GetMcuTemperature = 0x07,
    GetPayloadCrcType = 0x08,
    SetPayloadCrcType = 0x09,
    ThroughputTxStart = 0x1C,
    ThroughputTxStop = 0x1D,
    ThroughputRxData = 0x1E,
    GetSyncPinConfig = 0x30,
    SetSyncPinConfig = 0x31,
};
```

### Rpt

```cpp
enum class Rpt : std::uint8_t {
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    ThroughputData = 0x88,
    SyncPinConfig = 0x90,
};
```

### Status

```cpp
enum class Status : std::uint8_t {
    Ok = 0x00,
    Error = 0x01,
    ErrInvalidCmd = 0x02,
    ErrPayloadFormat = 0x03,
    ErrInvalidParam = 0x04,
    ErrPayloadCrc = 0x05,
    ErrBusy = 0x06,
    ErrCmdNotSupported = 0x07,
    ErrNotInitialized = 0x08,
    ErrHardwareFault = 0x09,
};
```

### SyncPinMode

```cpp
enum class SyncPinMode : std::uint8_t {
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
};
```

### SyncPinPolarity

```cpp
enum class SyncPinPolarity : std::uint8_t {
    IdleLow = 0x00,
    IdleHigh = 0x01,
};
```

### UNSOLICITED

```cpp
inline constexpr std::uint8_t UNSOLICITED = 0x00;
```

Value of the echoed-cmd byte in unsolicited reports.

### StatusReport

```cpp
struct StatusReport {
    std::uint8_t cmd;  // echoed request opcode; 0x00 = unsolicited
    std::uint8_t status;
};
```

#### StatusReport.unpack

```cpp
static StatusReport unpack(byte_span payload);
```

### TextReport

```cpp
struct TextReport {
    std::uint8_t cmd;
    std::string text;
};
```

#### TextReport.unpack

```cpp
static TextReport unpack(byte_span payload);
```

### SyncTimeReport

```cpp
struct SyncTimeReport {
    std::uint64_t pc_timestamp_us;  // T1 echoed
    std::uint64_t mcu_rx_us;  // T2
    std::uint64_t mcu_tx_us;  // T3
};
```

#### SyncTimeReport.unpack

```cpp
static SyncTimeReport unpack(byte_span payload);
```

### TemperatureReport

```cpp
struct TemperatureReport {
    std::uint64_t timestamp_us;
    std::int16_t raw_decidegrees;  // units of 0.1 degC
};
```

#### TemperatureReport.celsius

```cpp
double celsius() const;
```

#### TemperatureReport.unpack

```cpp
static TemperatureReport unpack(byte_span payload);
```

### SequenceErrorReport

```cpp
struct SequenceErrorReport {
    std::uint8_t expected_seq;
    std::uint8_t received_seq;
};
```

#### SequenceErrorReport.unpack

```cpp
static SequenceErrorReport unpack(byte_span payload);
```

### SyncPinConfig

```cpp
struct SyncPinConfig {
    std::uint8_t pin;  // 1..5
    SyncPinMode mode;
    SyncPinPolarity polarity;
};
```

#### SyncPinConfig.pack

```cpp
bytes pack() const;
```

#### SyncPinConfig.unpack

```cpp
static SyncPinConfig unpack(byte_span payload);
```

### pack_sync_time

```cpp
bytes pack_sync_time(std::uint64_t pc_timestamp_us);
```

### pack_set_payload_crc_type

```cpp
bytes pack_set_payload_crc_type(std::uint8_t crc_type);
```

### sync_time_offset_rtt

```cpp
std::pair<std::int64_t, std::int64_t> sync_time_offset_rtt(std::int64_t t1, std::int64_t t2, std::int64_t t3, std::int64_t t4);
```

NTP-style clock math, all us (contract 02 §5). Returns {offset_us, rtt_us}
where offset = device_clock - host_clock, computed with truncation toward
zero: offset = ((T2-T1)+(T3-T4)) / 2, rtt = (T4-T1)-(T3-T2).

### strip_device_string

```cpp
std::string strip_device_string(byte_span raw);
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

## Identity

### SensorType

```cpp
enum class SensorType {
    Sr04,
    Vl53l8,
    Vl53l7,  // VL53L5CX / VL53L7CX / VL53L7CH (one APP_VL53L7 firmware)
    Vl53l4,
    Vl53lx,  // VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX (one APP_VL53L0_4 firmware)
    Bno086,
    Bno055,  // 9-axis IMU on the APP_BNO055 register bridge (contract 13)
    Unknown,
};
```

### to_string

```cpp
std::string to_string(SensorType t);
```

String name for a SensorType (matches the vector encoding).

```cpp
std::string to_string(DeviceMode m);
```

### DeviceMode

```cpp
enum class DeviceMode {
    App,
    Bootloader,
    Unknown,
};
```

### Identity

```cpp
struct Identity {
    DeviceMode mode;
    std::optional<SensorType> sensor_type;  // nullopt in bootloader/unknown mode
    std::string software_name;
    std::string version;  // "" when not parseable
};
```

### parse_software_name

```cpp
Identity parse_software_name(const std::string& name);
```

Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
stripped of trailing NUL/0xFF (strip_device_string).

## USB identity

### DEPZ_USB_VID

```cpp
inline constexpr std::uint16_t DEPZ_USB_VID = 0x1BCF;
```

7119

### PID_SR04

```cpp
inline constexpr std::uint16_t PID_SR04 = 0xEC78;
```

60536

### PID_VL53L8

```cpp
inline constexpr std::uint16_t PID_VL53L8 = 0xED40;
```

60736

### PID_VL53L4CD

```cpp
inline constexpr std::uint16_t PID_VL53L4CD = 0xED45;
```

60741

### PID_BNO086

```cpp
inline constexpr std::uint16_t PID_BNO086 = 0xEE08;
```

60936

### DEV_USB_VID

```cpp
inline constexpr std::uint16_t DEV_USB_VID = 0x0483;
```

Dev / unprogrammed default (STMicroelectronics) that dev units carry.

### DEV_USB_PID

```cpp
inline constexpr std::uint16_t DEV_USB_PID = 0x56DC;
```

22236

### DEPZ_PID_RANGE_LO

```cpp
inline constexpr std::uint16_t DEPZ_PID_RANGE_LO = 60536;
```

Inclusive reserved sensor PID block under DEPZ_USB_VID.

### DEPZ_PID_RANGE_HI

```cpp
inline constexpr std::uint16_t DEPZ_PID_RANGE_HI = 65535;
```

### is_known_depz_usb

```cpp
bool is_known_depz_usb(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid) noexcept;
```

True when (vid, pid) is a recognized DEPZ (or dev-default) USB id.

### usb_model_hint

```cpp
std::optional<std::string> usb_model_hint(std::optional<std::uint16_t> vid, std::optional<std::uint16_t> pid);
```

Best-guess model name for a (vid, pid), or std::nullopt. Informational only.

### PortInfo

```cpp
struct PortInfo {
    std::string port;
    std::optional<std::string> usb_serial;
};
```

One enumerated serial port with its USB iSerial (used for ordering).

### order_by_serial

```cpp
std::vector<PortInfo> order_by_serial(std::vector<PortInfo> ports);
```

Order ports by USB serial ascending; None/empty serial sorts last, tie-break
by port path (contract 02 §4).

## SR04

### Sr04Cmd

```cpp
enum class Sr04Cmd : std::uint8_t {
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
};
```

### Sr04Rpt

```cpp
enum class Sr04Rpt : std::uint8_t {
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
};
```

### ECHO_TIMEOUT

```cpp
inline constexpr std::uint16_t ECHO_TIMEOUT = 0xFFFF;
```

echo_time_us sentinel: no echo received.

### SAMPLE_PERIOD_DEFAULT_US

```cpp
inline constexpr std::uint32_t SAMPLE_PERIOD_DEFAULT_US = 50000;
```

### ECHO_DECAY_DEFAULT_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_DEFAULT_US = 5000;
```

### ECHO_DECAY_MIN_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_MIN_US = 4000;
```

### ECHO_DECAY_MAX_US

```cpp
inline constexpr std::uint16_t ECHO_DECAY_MAX_US = 65000;
```

### Sr04Data

```cpp
struct Sr04Data {
    std::uint8_t source_cmd;  // 0x36 single shot (host or SYNC_IN), 0x37 loop sample
    std::uint64_t timestamp_us;
    std::uint16_t echo_time_us;
};
```

#### Sr04Data.unpack

```cpp
static Sr04Data unpack(byte_span payload);
```

### pack_sample_period

```cpp
bytes pack_sample_period(std::uint32_t period_us);
```

### unpack_sample_period

```cpp
std::uint32_t unpack_sample_period(byte_span payload);
```

### pack_echo_decay

```cpp
bytes pack_echo_decay(std::uint16_t decay_us);
```

### unpack_echo_decay

```cpp
std::uint16_t unpack_echo_decay(byte_span payload);
```

### distance_mm_from_echo

```cpp
std::optional<double> distance_mm_from_echo(std::uint16_t echo_time_us, std::optional<double> air_temp_c = std::nullopt);
```

Round-trip echo time -> distance in mm; nullopt for the timeout sentinel.
Default speed of sound 343 m/s; with air_temp_c uses c = 331.3 + 0.606*T.

## VL53L4CD (ToF)

### Vl53l4Cmd

```cpp
enum class Vl53l4Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u16, len u16 -> RPT_VL53_REG_DATA
    WriteReg = 0x33,     // addr u16, data[1..253]
    Xshut = 0x34,        // action u8 (XSHUT_OFF / XSHUT_ON / XSHUT_RESET)
    StartStream = 0x35,  // addr u16, len u16, flags u8
    StopStream = 0x36,
    GetInfo = 0x37,      // -> RPT_VL53_INFO
    SetI2cSpeed = 0x38,  // khz u16 (clamped to the nearest nominal step)
};
```

Bridge commands (0x32..0x38; 0x30/0x31 are the common sync pins).

### Vl53l4Rpt

```cpp
enum class Vl53l4Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA
    Info = 0x92,     // RPT_VL53_INFO
    Stream = 0x93,   // RPT_VL53_STREAM
};
```

Bridge reports.

### XFER_MAX

```cpp
inline constexpr std::uint16_t XFER_MAX = 253;
```

Max read len / write data length per transfer (STM32 I2C NBYTES is 8-bit; a
write spends two bytes on the register address; the firmware applies the
same limit to both directions).

### XSHUT_OFF

```cpp
inline constexpr std::uint8_t XSHUT_OFF = 0;
```

VL53_XSHUT actions.

### XSHUT_ON

```cpp
inline constexpr std::uint8_t XSHUT_ON = 1;
```

### XSHUT_RESET

```cpp
inline constexpr std::uint8_t XSHUT_RESET = 2;
```

answered after the boot handshake

### SF_INT_ACT_HIGH

```cpp
inline constexpr std::uint8_t SF_INT_ACT_HIGH = 0x02;
```

VL53_START_STREAM flags bit 1: INT active high, mirroring bit 4 of
GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.

### RESULT_BLOCK_ADDR

```cpp
inline constexpr std::uint16_t RESULT_BLOCK_ADDR = 0x0089;
```

The block the MCU streams: RESULT__RANGE_STATUS (0x0089) .. 0x0099 — every
field of the ULD results struct in one read.

### RESULT_BLOCK_LEN

```cpp
inline constexpr std::uint16_t RESULT_BLOCK_LEN = 17;
```

### MODEL_ID

```cpp
inline constexpr std::uint16_t MODEL_ID = 0xEBAA;
```

IDENTIFICATION__MODEL_ID (0x010F..0x0110) expected value.

### CONFIG_ADDR

```cpp
inline constexpr std::uint16_t CONFIG_ADDR = 0x2D;
```

First register of the 91-byte init configuration block (0x2D..0x87).

### CONFIG_FMP_BYTE

```cpp
inline constexpr std::uint8_t CONFIG_FMP_BYTE = 0x12;
```

config_block() forces byte 0 (register 0x2D) to this value: I2C Fast Mode
Plus pad, set unconditionally and never cleared.

### pack_read_reg

```cpp
bytes pack_read_reg(std::uint16_t addr, std::uint16_t len);
```

VL53_READ_REG payload: addr u16, len u16 (both little-endian).

### pack_write_reg

```cpp
bytes pack_write_reg(std::uint16_t addr, byte_span data);
```

VL53_WRITE_REG payload: addr u16 then the raw register data (1..XFER_MAX).

### pack_xshut

```cpp
bytes pack_xshut(std::uint8_t action);
```

VL53_XSHUT payload: action u8 (XSHUT_OFF / XSHUT_ON / XSHUT_RESET).

### pack_start_stream

```cpp
bytes pack_start_stream(std::uint16_t addr, std::uint16_t len, std::uint8_t flags);
```

VL53_START_STREAM payload: addr u16, len u16, flags u8 (SF_INT_ACT_HIGH).

### pack_set_i2c_speed

```cpp
bytes pack_set_i2c_speed(std::uint16_t khz);
```

VL53_SET_I2C_SPEED payload: khz u16 (firmware clamps to the nearest step).

### RegData

```cpp
struct RegData {
    std::uint8_t cmd = 0;
    std::uint64_t timestamp_us = 0;
    bytes data;
};
```

RPT_VL53_REG_DATA — one I2C read result. `cmd` echoes 0x32; `timestamp_us`
is MCU uptime at I2C-read completion.

#### RegData.unpack

```cpp
static std::optional<RegData> unpack(byte_span payload);
```

Decode a RPT_VL53_REG_DATA payload (9+N bytes); nullopt when too short.

### Vl53l4Info

```cpp
struct Vl53l4Info {
    std::uint32_t int_edges = 0;
    std::uint32_t slots_skipped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint16_t model_id = 0;  // expected MODEL_ID (0xEBAA)
    std::uint8_t fw_status = 0;  // expected 0x03 (booted)
    std::uint8_t initialized = 0;  // 1 = MODEL_ID matched on this read
    std::uint8_t xshut_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;
};
```

RPT_VL53_INFO — bridge diagnostics (21-byte little-endian payload).
Counters are free-running and wrap silently; watch increments, not absolute
values. Not a data path: safe to request while streaming.

#### Vl53l4Info.unpack

```cpp
static std::optional<Vl53l4Info> unpack(byte_span payload);
```

Decode a RPT_VL53_INFO payload; nullopt when shorter than 21 bytes.

### StreamData

```cpp
struct StreamData {
    std::uint64_t timestamp_us = 0;
    std::uint16_t addr = 0;
    std::uint16_t len = 0;
    bytes data;  // payload[12..12+len]
};
```

RPT_VL53_STREAM — one streamed register block. `timestamp_us` is MCU uptime
at the INT edge (the sensor event); `addr`/`len` echo the stream
configuration so each report is self-describing.

#### StreamData.unpack

```cpp
static std::optional<StreamData> unpack(byte_span payload);
```

Decode a RPT_VL53_STREAM payload (12+len bytes); nullopt when too short.

### Vl53l4Result

```cpp
struct Vl53l4Result {
    int range_status = 0;  // 0 = valid; raw >= 24 passes through unmapped
    int distance_mm = 0;
    int ambient_rate_kcps = 0;
    int ambient_per_spad_kcps = 0;
    int signal_rate_kcps = 0;
    int signal_per_spad_kcps = 0;
    int number_of_spad = 0;
    int sigma_mm = 0;
    int stream_count = 0;  // sensor frame counter, wraps at 255
};
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter. Rates are kcps,
distances/sigma are millimetres; everything is integer math per the vectors.

### parse_result_block

```cpp
std::optional<Vl53l4Result> parse_result_block(byte_span raw);
```

Decode the streamed RESULT_BLOCK_ADDR block exactly as VL53L4CD_GetResult()
decodes the same registers read one by one (big-endian words, x8 rates,
/4 sigma, per-SPAD rate x256/raw_spads with zero SPADs -> 0). Returns
nullopt when raw is shorter than 15 bytes.

### RangeTimingRegs

```cpp
struct RangeTimingRegs {
    std::uint16_t range_config_a = 0;
    std::uint16_t range_config_b = 0;
    std::uint32_t intermeasurement_raw = 0;
};
```

The register words SetRangeTiming programs: RANGE_CONFIG_A (0x005E),
RANGE_CONFIG_B (0x0061) and the INTERMEASUREMENT_MS (0x006C) raw dword.

### range_timing_registers

```cpp
std::optional<RangeTimingRegs> range_timing_registers(std::uint32_t budget_ms, std::uint32_t inter_ms, std::uint16_t osc_frequency, std::uint16_t clock_pll);
```

SetRangeTiming register math. `osc_frequency` is the word read from 0x0006;
`clock_pll` the word from RESULT__OSC_CALIBRATE_VAL (used only in autonomous
mode). budget 10..200 ms; inter_ms == 0 selects continuous mode, a value
greater than the budget selects autonomous low power. Returns nullopt when
osc_frequency is 0, the budget is out of range, or 0 < inter_ms <= budget.
Bit-exact with the ULD (32-bit truncations, 1.055 PLL factor).

### RangeTiming

```cpp
struct RangeTiming {
    std::uint32_t timing_budget_ms = 0;
    std::uint32_t inter_measurement_ms = 0;
};
```

GetRangeTiming result: the user-facing milliseconds.

### decode_range_timing

```cpp
std::optional<RangeTiming> decode_range_timing(std::uint32_t intermeasurement_raw, std::uint16_t clock_pll, std::uint16_t osc_frequency, std::uint16_t range_config_a);
```

GetRangeTiming register math from the raw register reads (INTERMEASUREMENT_MS
dword, RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word, RANGE_CONFIG_A word).
Returns nullopt when osc_frequency reads 0. Bit-exact with the ULD (32-bit
truncations, 1.065 PLL factor).

### offset_raw

```cpp
std::uint16_t offset_raw(int offset_mm);
```

RANGE_OFFSET_MM (0x001E) word for SetOffset (offset x4; INNER/OUTER zeroed
alongside).

### decode_offset

```cpp
int decode_offset(std::uint16_t raw_word);
```

GetOffset: RANGE_OFFSET_MM word -> signed millimetres.

### xtalk_raw

```cpp
std::uint16_t xtalk_raw(int xtalk_kcps);
```

XTALK_PLANE_OFFSET_KCPS (0x0016) word for SetXtalk (kcps x512).

### decode_xtalk

```cpp
int decode_xtalk(std::uint16_t raw_word);
```

GetXtalk: XTALK_PLANE_OFFSET_KCPS word -> kcps (std::lround(raw / 512.0)).

### signal_threshold_raw

```cpp
std::uint16_t signal_threshold_raw(int signal_kcps);
```

MIN_COUNT_RATE_RTN_LIMIT_MCPS (0x0066) word for SetSignalThreshold (kcps /8).

### decode_signal_threshold

```cpp
int decode_signal_threshold(std::uint16_t raw_word);
```

GetSignalThreshold: register word -> kcps.

### sigma_threshold_raw

```cpp
std::optional<std::uint16_t> sigma_threshold_raw(int sigma_mm);
```

RANGE_CONFIG__SIGMA_THRESH (0x0064) word for SetSigmaThreshold (mm x4);
nullopt when sigma_mm > 16383 (the word would overflow).

### decode_sigma_threshold

```cpp
int decode_sigma_threshold(std::uint16_t raw_word);
```

GetSigmaThreshold: register word -> millimetres.

### default_configuration

```cpp
const std::array<std::uint8_t, 91>& default_configuration();
```

The stock ST VL53L4CD_DEFAULT_CONFIGURATION[] table — 91 bytes for
registers 0x2D..0x87 (ULD 2.2.3), untouched.

### config_block

```cpp
bytes config_block();
```

The 91-byte block sensor_init writes at CONFIG_ADDR: the ST default
configuration with byte 0 forced to CONFIG_FMP_BYTE (I2C Fast Mode Plus).

## VL53L8 (ToF)

### RESOLUTION_4X4

```cpp
inline constexpr int RESOLUTION_4X4 = 16;
```

### RESOLUTION_8X8

```cpp
inline constexpr int RESOLUTION_8X8 = 64;
```

### STREAM_CHUNK_MAX

```cpp
inline constexpr std::size_t STREAM_CHUNK_MAX = 1528;
```

Max frame-data bytes carried in one RPT_VL53_FRAME chunk (contract 04).

### Variant

```cpp
enum class Variant {
    CX,  // base ToF (ULD 2.1.0); dev-default, no dedicated production USB PID
    CH,  // CX + CNH histograms; production USB PID 0xED40 (VL53LMZ 2.0.16)
};
```

The two DEPZ ToF die variants. Both stream the same results-frame layout
decoded by decode_frame(); they differ in the footer-id offset here, and the
CH additionally emits CNH histograms (decode_cnh, at the bottom).

### FOOTER_ID_OFF_CX

```cpp
inline constexpr int FOOTER_ID_OFF_CX = 12;
```

Footer-id offset used by decode_frame's header/footer match check, per
variant: 12 for VL53L8CX (ULD 2.1.0), 4 for VL53L8CH (VL53LMZ 2.0.16).

### FOOTER_ID_OFF_CH

```cpp
inline constexpr int FOOTER_ID_OFF_CH = 4;
```

### CNH_DATA_IDX

```cpp
inline constexpr std::uint32_t CNH_DATA_IDX = 0xC048;
```

Results-block index of the CNH data output block (VL53LMZ plugin_cnh).

### footer_id_off

```cpp
inline constexpr int footer_id_off(Variant v);
```

Footer-id offset for a variant (convenience for decode_frame's argument).

### FrameChunk

```cpp
struct FrameChunk {
    std::uint64_t timestamp_us = 0;
    std::uint16_t full_size = 0;
    std::uint16_t offset = 0;
    bytes data;
};
```

One RPT_VL53_FRAME payload: capture timestamp + this chunk's slice of the
full frame. Header is ts(u64) full_size(u16) offset(u16), then data.

#### FrameChunk.unpack

```cpp
static FrameChunk unpack(byte_span payload);
```

### FrameReassembler

```cpp
class FrameReassembler {
    std::size_t completed = 0;
    std::size_t discarded = 0;
};
```

Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports. Rules
(contract 04): reset on offset==0; chunks must be contiguous — a gap discards
the frame in progress; a frame completes when the accumulated bytes equal
full_size.

#### FrameReassembler.feed

```cpp
std::optional<std::pair<std::uint64_t, bytes>> feed(const FrameChunk& chunk);
```

Returns (timestamp_us, frame_bytes) when a frame completes, else nullopt.

### Vl53l8Frame

```cpp
struct Vl53l8Frame {
    std::uint64_t timestamp_us = 0;
    int resolution = 0;
    int silicon_temp_degc = 0;
    std::vector<std::int32_t> distance_mm;
    std::vector<std::uint8_t> target_status;
    std::vector<std::uint8_t> nb_target_detected;
    std::vector<std::uint32_t> signal_per_spad;
    std::vector<std::uint32_t> ambient_per_spad;
    std::vector<std::uint32_t> nb_spads_enabled;
    std::vector<std::uint16_t> range_sigma_mm_raw;
    std::vector<std::uint8_t> reflectance;
    std::optional<bytes> cnh_raw;
};
```

One decoded ranging frame. Arrays are sized to the active resolution (16 or
64 zones), row-major. Raw integers per the wire; `distance_mm` already has
the ST /4 floor scaling applied, `range_sigma_mm_raw` is the raw u16 (real
value = raw / 128).

### swap_buffer

```cpp
bytes swap_buffer(byte_span data);
```

VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail bytes unchanged).

### decode_frame

```cpp
std::optional<Vl53l8Frame> decode_frame(byte_span raw, int footer_id_off = FOOTER_ID_OFF_CX);
```

Decode one raw results frame (the full reassembled frame bytes). Shared by
both CX and CH — the layout is identical. Resolution is inferred from the
frame's block layout. Returns nullopt for a corrupted frame (header/footer id
mismatch). `footer_id_off` is variant-specific (FOOTER_ID_OFF_CX /
FOOTER_ID_OFF_CH, or footer_id_off(Variant)); it defaults to the CX offset.

### xtalk_margin_raw

```cpp
std::uint32_t xtalk_margin_raw(double margin_kcps);
```

Xtalk margin (kcps/SPAD) <-> DCI_XTALK_CFG raw word (raw = round(kcps*2048)).

### xtalk_margin_kcps

```cpp
double xtalk_margin_kcps(std::uint32_t raw);
```

### ThreshMeasurement

```cpp
enum ThreshMeasurement : std::uint8_t {
    THRESH_DIST_MM = 1,
    THRESH_SIGNAL_PER_SPAD_KCPS = 2,
    THRESH_RANGE_SIGMA_MM = 4,
    THRESH_AMBIENT_PER_SPAD_KCPS = 8,
    THRESH_NB_TARGET_DETECTED = 9,
    THRESH_TAR_STATUS = 12,
    THRESH_NB_SPADS_ENABLED = 13,
    THRESH_MOTION_INDICATOR = 19,
};
```

Detection-threshold measurement selectors (plugin source).

### DetectionThreshold

```cpp
struct DetectionThreshold {
    std::int32_t low_thresh = 0;  // real units; scaled on pack
    std::int32_t high_thresh = 0;  // real units; scaled on pack
    std::uint8_t measurement = 0;
    std::uint8_t type = 0;  // window selector
    std::uint8_t zone_num = 0;
    std::uint8_t operation = 0;  // combine op
};
```

### NB_THRESHOLDS

```cpp
inline constexpr int NB_THRESHOLDS = 64;
```

### pack_detection_thresholds

```cpp
bytes pack_detection_thresholds(const std::vector<DetectionThreshold>& thresholds);
```

Pack the 64-entry DCI_DET_THRESH_START block (768 bytes). Missing entries
default to zeros; low/high are multiplied by the measurement's scale factor.

### detection_thresholds_valid_status

```cpp
bytes detection_thresholds_valid_status();
```

The 8-byte DCI_DET_THRESH_VALID_STATUS write that accompanies the block.

### MotionConfig

```cpp
struct MotionConfig {
    std::int32_t ref_bin_offset = 0;
    std::uint32_t detection_threshold = 0;
    std::uint32_t extra_noise_sigma = 0;
    std::uint32_t null_den_clip_value = 0;
    std::uint8_t mem_update_mode = 0;
    std::uint8_t mem_update_choice = 0;
    std::uint8_t sum_span = 0;
    std::uint8_t feature_length = 0;
    std::uint8_t nb_of_aggregates = 0;
    std::uint8_t nb_of_temporal_accumulations = 0;
    std::uint8_t min_nb_for_global_detection = 0;
    std::uint8_t global_indicator_format_1 = 0;
    std::uint8_t global_indicator_format_2 = 0;
    std::uint8_t spare_1 = 0;
    std::uint8_t spare_2 = 0;
    std::uint8_t spare_3 = 0;
    std::int8_t map_id[64] = {0};
    std::uint8_t indicator_format_1[32] = {0};
    std::uint8_t indicator_format_2[32] = {0};
};
```

Mirror of VL53L8CX_Motion_Configuration (156 bytes, plugin source).

#### MotionConfig.pack

```cpp
bytes pack() const;
```

### motion_config_init

```cpp
MotionConfig motion_config_init(int resolution);
```

Build the default motion-indicator configuration for `resolution`
(RESOLUTION_4X4 or RESOLUTION_8X8), matching uld.motion_indicator_init.

### CNH_MAX_AGGREGATES

```cpp
inline constexpr int CNH_MAX_AGGREGATES = 64;
```

Persistent-data header / buffer layout constants (plugin_cnh.c).
decode_cnh() bounds (same limits as the C SDK's DEPZ_VL53L8_CNH_MAX_*).

### CNH_MAX_FEATURE_LENGTH

```cpp
inline constexpr int CNH_MAX_FEATURE_LENGTH = 255;
```

### CNH_PER_HEADER_WORDS

```cpp
inline constexpr int CNH_PER_HEADER_WORDS = 5;
```

CNH_PER_HEADER_BYTES / 4

### CNH_PER_BUFFER_HEADER_WORDS

```cpp
inline constexpr int CNH_PER_BUFFER_HEADER_WORDS = 2;
```

CNH_PER_BUFFER_HEADER_BYTES / 4

### CNH_PER_HEADER_BUFFER_INFO_IDX

```cpp
inline constexpr int CNH_PER_HEADER_BUFFER_INFO_IDX = 1;
```

### CNH_PER_HEADER_FLAGS_IDX

```cpp
inline constexpr int CNH_PER_HEADER_FLAGS_IDX = 3;
```

### CNH_BUFFER_INFO_WORDS_MASK

```cpp
inline constexpr std::uint32_t CNH_BUFFER_INFO_WORDS_MASK = 0xFFFF;
```

### CNH_MI_STATE_PING

```cpp
inline constexpr int CNH_MI_STATE_PING = 0;
```

### CnhAggregate

```cpp
struct CnhAggregate {
    std::vector<std::int32_t> hist_raw;
    std::vector<std::int8_t> hist_scaler;
    std::vector<double> hist;
    std::int32_t ambient_raw = 0;
    std::int8_t ambient_scaler = 0;
    double ambient = 0.0;
};
```

One decoded CNH aggregate. `hist_raw[i] / 2**hist_scaler[i]` is the float bin
value; `ambient = ambient_raw / 2**ambient_scaler`. hist_raw / hist_scaler are
length == feature_length.

### CnhFrame

```cpp
struct CnhFrame {
    std::uint32_t ref_residual_word = 0;
    double ref_residual = 0.0;
    std::vector<CnhAggregate> aggregates;  // len == nb_of_aggregates
};
```

A decoded CNH data block. `ref_residual_word` is the raw u32 at byte offset 8;
`ref_residual = ref_residual_word / 2048.0`.

### decode_cnh

```cpp
CnhFrame decode_cnh(int nb_of_aggregates, int feature_length, byte_span raw);
```

Decode a captured CNH data block (`raw`, byte-swapped exactly like the standard
ranging blocks) into per-aggregate histograms. `nb_of_aggregates` and
`feature_length` come from the CNH config used on the device (see CnhConfig /
MotionConfig). Faithful port of cnh.decode / _decode_aggregate for the fixed
cnh_cfg (ping-pong + variance disabled).
Throws std::invalid_argument for an out-of-range config and
std::length_error for a block shorter than the config implies.

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### Vl53l7Cmd

```cpp
enum class Vl53l7Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u16, len u16 (1..READ_MAX_LEN) -> RPT_VL53_REG_DATA
    WriteReg = 0x33,     // addr u16, data[1..WRITE_MAX_LEN]
    PinCtrl = 0x34,      // action u8 (PinAction)
    StartStream = 0x35,  // frame_size u16
    StopStream = 0x36,
    GetInfo = 0x37,      // -> RPT_VL53_INFO
    SetI2cSpeed = 0x38,  // khz u16 (snaps to the nearest I2C_SPEED_STEPS_KHZ)
};
```

Bridge commands. 0x32/0x33/0x35/0x36 are the VL53L8 bridge (contract 04);
0x34/0x37/0x38 are new on the I2C board.

### Vl53l7Rpt

```cpp
enum class Vl53l7Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA
    Info = 0x92,     // RPT_VL53_INFO (no echoed command byte)
    Frame = 0x93,    // RPT_VL53_FRAME (decode with vl53l8::FrameChunk)
};
```

Bridge reports. 0x91 / 0x93 are the VL53L8 ones.

### PinAction

```cpp
enum class PinAction : std::uint8_t {
    LpnOff = 0,     // stop streaming, drive LPn low: sensor I2C interface off
    LpnOn = 1,      // drive LPn high: interface on (power-up default)
    I2cRst = 2,     // pulse I2C_RST
    SoftCycle = 3,  // stop streaming, LPn low 1 ms, high, I2C_RST; clears I2C counters
};
```

VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
GPIO): after LPN_OFF or SOFT_CYCLE the host must re-run init().

### I2cError

```cpp
enum class I2cError : std::uint8_t {
    Ok = 0,
    Nack = 1,
    Timeout = 2,  // 250 ms deadline
    BusError = 3,
};
```

RPT_VL53_INFO.last_i2c_error.

### READ_MAX_LEN

```cpp
inline constexpr std::uint16_t READ_MAX_LEN = 1536;
```

Transfer ceilings. Hosts MUST split register reads at READ_MAX_LEN (the
VL53L8 host splits at 2048 — reusing that here fails with ERR_INVALID_PARAM).

### WRITE_MAX_LEN

```cpp
inline constexpr std::uint16_t WRITE_MAX_LEN = 2048;
```

VL53LMZ_XFER_MAX

### STREAM_CHUNK_MAX

```cpp
inline constexpr std::size_t STREAM_CHUNK_MAX = 1536;
```

Max frame-data bytes carried in one RPT_VL53_FRAME chunk (VL53L8: 1528).

### INFO_SIZE

```cpp
inline constexpr std::size_t INFO_SIZE = 20;
```

RPT_VL53_INFO payload size.

### MIN_RANGING_FREQUENCY_HZ

```cpp
inline constexpr int MIN_RANGING_FREQUENCY_HZ = 1;
```

L5/L7 range and stream at 1 Hz (VL53L8: >= 2 Hz).

### I2C_SPEED_STEPS_KHZ

```cpp
inline constexpr std::uint16_t I2C_SPEED_STEPS_KHZ[] = {100, 200, 400, 500, 600, 700, 800, 900, 1000};
```

Nominal SCL steps the firmware carries a timing for; others snap to nearest.

### FOOTER_ID_OFF

```cpp
inline constexpr int FOOTER_ID_OFF = vl53l8::FOOTER_ID_OFF_CH;
```

Footer-id offset of L5/L7 frames: `size − 4` for both blob sets (l7cx and
l7ch), like the VL53L8CH.

### pack_read_reg

```cpp
bytes pack_read_reg(std::uint16_t addr, std::uint16_t len);
```

VL53_READ_REG payload: addr u16, len u16 (both little-endian; VL53L8 format).

### pack_write_reg

```cpp
bytes pack_write_reg(std::uint16_t addr, byte_span data);
```

VL53_WRITE_REG payload: addr u16 then the raw register data (VL53L8 format).

### pack_pin_ctrl

```cpp
bytes pack_pin_ctrl(std::uint8_t action);
```

VL53_PIN_CTRL payload: action u8 (PinAction value).

### pack_set_i2c_speed

```cpp
bytes pack_set_i2c_speed(std::uint16_t khz);
```

VL53_SET_I2C_SPEED payload: khz u16.

### Vl53l7Info

```cpp
struct Vl53l7Info {
    std::uint32_t int_edges = 0;
    std::uint32_t frames_dropped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // I2cError
    std::uint8_t lpn_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;
    std::uint16_t frame_size = 0;
    bool streaming = false;
};
```

RPT_VL53_INFO — bridge state only (the sensor is never probed), 20-byte
little-endian payload `<IIIBBBHHB` with NO echoed command byte. Counters run
from power-up / DEVICE_RESET; SOFT_CYCLE clears the I2C ones.

#### Vl53l7Info.unpack

```cpp
static std::optional<Vl53l7Info> unpack(byte_span payload);
```

Decode a RPT_VL53_INFO payload; nullopt when shorter than INFO_SIZE.

### Model

```cpp
enum class Model {
    Vl53l5cx,
    Vl53l7cx,  // base; safe default (its blob runs on every L5/L7 part)
    Vl53l7ch,  // Vl53l7cx + CNH
};
```

The three sensor classes an APP_VL53L7 board opens as.

### to_string

```cpp
std::string to_string(Model m);
```

"vl53l5cx" / "vl53l7cx" / "vl53l7ch".

### resolve_model

```cpp
Model resolve_model(const std::optional<std::string>& usb_model, const std::string& device_name);
```

Resolve the class: the production USB PID model (usb_model_hint(): e.g.
"vl53l7ch") first; else the first match of `VL53L([57])(CX|CH)` in the
GET_DEVICE_NAME string (a VL53L5CH match falls back to Vl53l7cx); else
Vl53l7cx.

### infer_resolution

```cpp
std::optional<int> infer_resolution(byte_span raw);
```

Zone count inferred from an L5/L7 frame: the size of the zone-scaled ambient
block (index 0x54D0, sized to the resolution by the output-list rule). nullopt
when the frame carries no such block.

### decode_frame

```cpp
std::optional<vl53l8::Vl53l8Frame> decode_frame(byte_span raw, std::optional<int> resolution = std::nullopt);
```

Decode one raw L5/L7 results frame (the full reassembled frame bytes). Same
layout as VL53L8 (vl53l8::decode_frame), with the L5/L7 differences applied:
footer id at `size − 4`, and every per-zone array trimmed to `resolution`
entries (per-target blocks carry 64 entries even in 4x4; the sensor fills the
first `resolution` and zero-pads the rest). `resolution` is the value
start_ranging used (RESOLUTION_4X4 / RESOLUTION_8X8); when omitted it is
inferred with infer_resolution(). Returns nullopt for a corrupted frame
(header/footer id mismatch).

## VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)

### Vl53lxCmd

```cpp
enum class Vl53lxCmd : std::uint8_t {
    ReadReg = 0x32,       // addr u16, len u16 -> RPT_VL53_REG_DATA
    WriteReg = 0x33,      // addr u16, data[1..253]
    Xshut = 0x34,         // action u8; RESET has no boot handshake (host polls)
    StartStream = 0x35,   // addr u16, len u16, flags u8, n_clear u8, clear[n]
    StopStream = 0x36,
    GetInfo = 0x37,       // -> RPT_VL53_INFO (23 bytes)
    SetI2cSpeed = 0x38,   // khz u16
    SetAddrWidth = 0x39,  // width u8 (1 or 2); sticky, 2 after a reset
    ClearI2cErrors = 0x3A,  // v2.01 (fw v0.24), no payload; after every sensor init
};
```

Bridge commands (0x32..0x3A; 0x30/0x31 are the common sync pins).

### Vl53lxRpt

```cpp
enum class Vl53lxRpt : std::uint8_t {
    RegData = 0x91,  // RPT_VL53_REG_DATA (decode with RegData)
    Info = 0x92,     // RPT_VL53_INFO (v2.00 layout, Vl53lxInfo)
    Stream = 0x93,   // RPT_VL53_STREAM (decode with StreamData)
};
```

Bridge reports (0x91 / 0x93 are contract 10 unchanged).

### SF_INT_ACT_HIGH

```cpp
using vl53l4::SF_INT_ACT_HIGH;
```

Identical on the wire to contract 10.

### XFER_MAX

```cpp
using vl53l4::XFER_MAX;
```

### XSHUT_OFF

```cpp
using vl53l4::XSHUT_OFF;
```

### XSHUT_ON

```cpp
using vl53l4::XSHUT_ON;
```

### XSHUT_RESET

```cpp
using vl53l4::XSHUT_RESET;
```

### RegData

```cpp
using RegData = vl53l4::RegData;
```

### StreamData

```cpp
using StreamData = vl53l4::StreamData;
```

### CLEAR_STEPS_MAX

```cpp
inline constexpr std::size_t CLEAR_STEPS_MAX = 4;
```

Interrupt-release steps one START_STREAM may carry (VL53_CLEAR_STEPS_WIRE_MAX).

### INFO_SIZE

```cpp
inline constexpr std::size_t INFO_SIZE = 23;
```

RPT_VL53_INFO payload size (v2.00).

### DIE_BLOCK_ADDR

```cpp
inline constexpr std::uint16_t DIE_BLOCK_ADDR = 0x0089;
```

The streamed blocks (contract 12 §3).

### DIE_BLOCK_LEN

```cpp
inline constexpr std::size_t DIE_BLOCK_LEN = 17;
```

### L0X_BLOCK_ADDR

```cpp
inline constexpr std::uint16_t L0X_BLOCK_ADDR = 0x14;
```

VL53L0X, address width 1

### L0X_BLOCK_LEN

```cpp
inline constexpr std::size_t L0X_BLOCK_LEN = 12;
```

### HISTOGRAM_BLOCK_ADDR

```cpp
inline constexpr std::uint16_t HISTOGRAM_BLOCK_ADDR = 0x0088;
```

Bare Driver bins

### HISTOGRAM_BLOCK_LEN

```cpp
inline constexpr std::size_t HISTOGRAM_BLOCK_LEN = 83;
```

### HISTOGRAM_BINS

```cpp
inline constexpr std::size_t HISTOGRAM_BINS = 24;
```

### ClearStep

```cpp
struct ClearStep {
    std::uint16_t addr = 0;
    std::uint8_t value = 0;
};
```

One interrupt-release write the bridge plays after every block read.

### pack_read_reg

```cpp
using vl53l4::pack_read_reg;
```

Contract-10 codecs, identical on the wire: VL53_READ_REG, VL53_WRITE_REG,
VL53_XSHUT, VL53_SET_I2C_SPEED. At address width 1 only the low byte of
`addr` goes on the bus and addr+len must stay <= 0x100.

### pack_set_i2c_speed

```cpp
using vl53l4::pack_set_i2c_speed;
```

### pack_write_reg

```cpp
using vl53l4::pack_write_reg;
```

### pack_xshut

```cpp
using vl53l4::pack_xshut;
```

### pack_start_stream

```cpp
std::optional<bytes> pack_start_stream(std::uint16_t addr, std::uint16_t len, const std::vector<ClearStep>& clear, std::uint8_t flags = 0);
```

VL53_START_STREAM payload (6 + 3n bytes): addr u16, len u16, flags u8,
n_clear u8, then n x {addr u16, value u8} — the interrupt-release list the
bridge plays after every block read. nullopt for more than CLEAR_STEPS_MAX
steps (the bridge would answer ERR_PAYLOAD_FORMAT).

### pack_set_addr_width

```cpp
std::optional<bytes> pack_set_addr_width(std::uint8_t width);
```

VL53_SET_ADDR_WIDTH payload: width u8. nullopt unless width is 1 or 2.

### Vl53lxInfo

```cpp
struct Vl53lxInfo {
    std::uint32_t int_edges = 0;
    std::uint32_t slots_skipped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint8_t xshut_level = 0;
    std::uint8_t int_level = 0;
    std::uint16_t i2c_khz = 0;
    std::uint8_t addr_width = 0;
    std::uint8_t n_clear = 0;
    std::uint32_t frames_dropped = 0;
};
```

RPT_VL53_INFO (v2.00, 23 bytes, `<IIIBBBHBBI`) — bridge state only, the
bridge reads no sensor register. Counters are free-running (wrap silently);
slots_skipped, frames_dropped and the fault latch reset at START_STREAM.

#### Vl53lxInfo.unpack

```cpp
static std::optional<Vl53lxInfo> unpack(byte_span payload);
```

Decode a RPT_VL53_INFO payload; nullopt when shorter than INFO_SIZE.

### DriverKind

```cpp
enum class DriverKind {
    Uld,
    Ulp,
    Histogram,
};
```

The kinds of driver ST ships for this family: `uld` (the die computes the
distance), `ulp` (Ultra Low Power, L3CX only), `histogram` (Bare Driver:
24 photon bins, the host finds up to four targets).

### to_string

```cpp
std::string to_string(DriverKind k);
```

"uld" / "ulp" / "histogram".

```cpp
std::string to_string(SensorClass c);
```

### Product

```cpp
struct Product {
    std::string name;  // "VL53L0X", ...
    std::uint16_t usb_pid = 0;  // production USB PID
    std::uint16_t model_id = 0;  // cross-check only (L1CX/L1CB, L4CD/L4CX share)
    std::uint32_t reach_mm = 0;  // datasheet rating of the module
    std::vector<DriverKind> driver_kinds;  // pairs that exist, UI order
    DriverKind default_driver = DriverKind::Uld;
    std::uint8_t addr_width = 2;  // register-address width, bytes
    std::vector<ClearStep> clear_steps;
    std::uint16_t max_khz = 0;  // bus ceiling after init (init runs at 400)
};
```

One row of the product table. The bridge parameters (addr_width,
clear_steps, max_khz) are those of the product's default driver.

### products

```cpp
const std::vector<Product>& products();
```

The six products, in UI order: VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX,
VL53L4CD, VL53L4CX.

### find_product

```cpp
const Product* find_product(const std::string& name);
```

The table row for an exact product name ("VL53L4CX"); nullptr if unknown.

### product_from_board_name

```cpp
std::optional<std::string> product_from_board_name(const std::string& name);
```

`ToF Sensor VL53L4CD USB v2.1` -> "VL53L4CD": the first `VL53L<digit><part>`
match in the upper-cased name, if it is a family product; else nullopt (an
unstamped board, or a name we do not recognise).

### SensorClass

```cpp
enum class SensorClass {
    Vl53lx,
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
};
```

The classes a vl53lx board opens as. Vl53lx is the generic class that takes
the product at init; VL53L4CD boards on this firmware use it too (the
dedicated vl53l4cd class belongs to APP_VL53L4).

### resolve_class

```cpp
SensorClass resolve_class(const std::optional<std::string>& usb_model, const std::string& device_name);
```

Resolve the class: the production USB PID model (usb_model_hint(), e.g.
"vl53l4cx", case-insensitive) if it is a family product, else the product
the device name carries, else the generic class.

### DieVariant

```cpp
enum class DieVariant {
    L4,
    L1,
};
```

Which ULD reads the die block: L4 = VL53L4CD ULD (also the L3CX ULP and
L4CX-as-L4CD), signal at byte 5, per-SPAD K = 256; L1 = VL53L1X ULD
(crosstalk-corrected peak signal at 0x0098 = byte 15, K = 25).

### DieResult

```cpp
using DieResult = vl53l4::Vl53l4Result;
```

Same fields as the contract-10 result (range_status via STATUS_RTN, rates
kcps, sigma/distance mm, the sensor's frame counter).

### decode_die_block

```cpp
std::optional<DieResult> decode_die_block(byte_span raw, DieVariant variant = DieVariant::L4);
```

The 17-byte die block at DIE_BLOCK_ADDR as the named ULD reads it. The L4
variant is exactly vl53l4::parse_result_block. nullopt when raw is shorter
than DIE_BLOCK_LEN.

### L0xRaw

```cpp
struct L0xRaw {
    std::uint16_t distance_raw = 0;  // mm (quarter-mm if RangeFractionalEnable)
    std::uint8_t device_range_status = 0;  // raw byte 0
    std::uint32_t signal_rate_mcps_1616 = 0;  // FixPoint16.16 Mcps (wire 9.7 << 9)
    std::uint32_t ambient_rate_mcps_1616 = 0;
    std::uint16_t effective_spad_count_88 = 0;  // 8.8
};
```

Raw fields of the VL53L0X 12-byte block at L0X_BLOCK_ADDR. The PAL range
status, sigma and dmax need device data cached by init (full driver).

### decode_l0x_raw

```cpp
std::optional<L0xRaw> decode_l0x_raw(byte_span raw);
```

Decode the VL53L0X block; nullopt when raw is shorter than L0X_BLOCK_LEN.

### HistogramRaw

```cpp
struct HistogramRaw {
    std::uint8_t interrupt_status = 0;
    std::uint8_t range_status = 0;
    std::uint8_t report_status = 0;
    std::uint8_t stream_count = 0;
    std::uint16_t dss_actual_effective_spads = 0;
    std::uint16_t reference_phase = 0;
    std::uint8_t vcsel_start = 0;
    std::array<std::uint32_t, HISTOGRAM_BINS> bins{};  // big-endian 24-bit counts
};
```

Status bytes and the 24 photon bins of the 83-byte histogram block. Turning
bins into targets (preset, VCSEL period, A/B frame pairs) is the full driver.

### decode_histogram_raw

```cpp
std::optional<HistogramRaw> decode_histogram_raw(byte_span raw);
```

Decode the histogram block at HISTOGRAM_BLOCK_ADDR: bin 23's low byte is
carried in a separate MSB/LSB pair ((MSB << 2) + LSB, 8-bit) and patched in
before the bins are read. nullopt when raw is shorter than
HISTOGRAM_BLOCK_LEN.

## BNO086 (IMU)

### SHTP_HEADER_SIZE

```cpp
inline constexpr std::size_t SHTP_HEADER_SIZE = 4;
```

### LENGTH_MASK

```cpp
inline constexpr std::uint16_t LENGTH_MASK = 0x7FFF;
```

### CONTINUATION_BIT

```cpp
inline constexpr std::uint16_t CONTINUATION_BIT = 0x8000;
```

### NUM_CHANNELS

```cpp
inline constexpr int NUM_CHANNELS = 6;
```

### MAX_TX_FRAME

```cpp
inline constexpr std::size_t MAX_TX_FRAME = 64;
```

ERRATA E2 MCU slot

### ShtpChannel

```cpp
enum class ShtpChannel : std::uint8_t {
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
};
```

### ShtpHeader

```cpp
struct ShtpHeader {
    std::uint16_t length = 0;  // bits 14:0 — cargo length incl. this 4-byte header
    std::uint8_t channel = 0;
    std::uint8_t seq = 0;
    bool continuation = false;
};
```

#### ShtpHeader.pack

```cpp
bytes pack() const;
```

#### ShtpHeader.unpack

```cpp
static ShtpHeader unpack(byte_span data);
```

### ShtpCargo

```cpp
struct ShtpCargo {
    int channel = 0;
    int seq = 0;  // seq of the first fragment
    bytes payload;
};
```

One reassembled cargo: `payload` excludes all SHTP headers.

### shtp_build_frame

```cpp
bytes shtp_build_frame(int channel, byte_span payload, std::uint8_t seq);
```

Single-fragment frame: length = header + payload.

### shtp_fragment_cargo

```cpp
std::vector<bytes> shtp_fragment_cargo(int channel, byte_span payload, std::uint8_t seq_start, std::size_t max_frame = MAX_TX_FRAME);
```

Split a cargo into wire frames of at most `max_frame` bytes. First fragment
advertises the TOTAL cargo length; continuations carry the remaining length
with the continuation bit set; seq increments per frame.

### ShtpLayer

```cpp
class ShtpLayer {
    std::size_t discarded = 0;
};
```

Per-channel TX sequence counters + RX cargo reassembly (§3).

#### ShtpLayer.next_frame

```cpp
bytes next_frame(int channel, byte_span payload);
```

Build a single-fragment frame, consuming the channel's TX seq.

#### ShtpLayer.tx_seq

```cpp
int tx_seq(int channel) const;
```

#### ShtpLayer.feed

```cpp
std::optional<ShtpCargo> feed(byte_span frame);
```

Consume one inbound frame; return the cargo when complete, else nullopt.

#### ShtpLayer.reset

```cpp
void reset();
```

Forget all TX seq counters and partial cargos (sensor reset).

### sh2_build_set_feature

```cpp
bytes sh2_build_set_feature(std::uint8_t sensor_id, std::uint32_t interval_us, std::uint32_t batch_us = 0, std::uint16_t sensitivity = 0, std::uint8_t flags = 0, std::uint32_t cfg_word = 0);
```

Set Feature Command (0xFD), 17 bytes. interval_us = 0 disables the sensor.

### sh2_build_get_feature_request

```cpp
bytes sh2_build_get_feature_request(std::uint8_t sensor_id);
```

Get Feature Request (0xFE), 2 bytes.

### sh2_build_product_id_request

```cpp
bytes sh2_build_product_id_request();
```

Product ID Request (0xF9), 2 bytes.

### sh2_build_command_request

```cpp
bytes sh2_build_command_request(std::uint8_t seq, std::uint8_t command, byte_span params = {});
```

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8 (params, <=9 bytes).

### sh2_build_frs_read_request

```cpp
bytes sh2_build_frs_read_request(std::uint16_t frs_type, std::uint16_t offset_words = 0, std::uint16_t block_words = 0);
```

FRS Read Request (0xF4), 8 bytes. block_words = 0 reads the record.

### sh2_build_frs_write_request

```cpp
bytes sh2_build_frs_write_request(std::uint16_t frs_type, std::uint16_t length_words);
```

FRS Write Request (0xF7), 6 bytes. length_words = 0 erases the record.

### sh2_build_frs_write_data

```cpp
bytes sh2_build_frs_write_data(std::uint16_t offset_words, const std::vector<std::uint32_t>& words);
```

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

### BASE_TIMESTAMP_REF

```cpp
inline constexpr std::uint8_t BASE_TIMESTAMP_REF = 0xFB;
```

In-cargo control IDs on the input channels.

### TIMESTAMP_REBASE

```cpp
inline constexpr std::uint8_t TIMESTAMP_REBASE = 0xFA;
```

### RV_ACCURACY_Q

```cpp
inline constexpr int RV_ACCURACY_Q = 12;
```

### GYRO_RV_ANGVEL_Q

```cpp
inline constexpr int GYRO_RV_ANGVEL_Q = 10;
```

### ReportType

```cpp
enum class ReportType {
    Acceleration,
    Gyroscope,
    Magnetometer,
    UncalibratedGyroscope,
    UncalibratedMagnetometer,
    RotationVector,
    ScalarReport,
    TapDetector,
    StepCounter,
    StepDetector,
    SignificantMotion,
    StabilityClassifier,
    ShakeDetector,
    GenericEvent,
    PersonalActivityClassifier,
    RawSensor,
    GyroIntegratedRV,
    UnknownReport,
};
```

### Report

```cpp
struct Report {
    ReportType type = ReportType::UnknownReport;
    int sensor_id = 0;
    std::int64_t timestamp_us = 0;
    int seq = 0;
    int accuracy = 0;
    std::int64_t delay_us = 0;
    std::int64_t x_raw = 0, y_raw = 0, z_raw = 0;
    std::int64_t bias_x_raw = 0, bias_y_raw = 0, bias_z_raw = 0;
    std::int64_t i_raw = 0, j_raw = 0, k_raw = 0, real_raw = 0;
    bool has_accuracy_raw = false;
    std::int64_t accuracy_raw = 0;
    std::int64_t value_raw = 0;
    int flags = 0;
    std::int64_t latency_us = 0;
    int steps = 0;
    int motion = 0;
    int classification = 0;
    int page_number = 0;
    bool end_of_sequence = false;
    int most_likely_state = 0;
    std::vector<int> confidences;
    std::int64_t sensor_timestamp_us = 0;
    std::int64_t temperature_raw = 0;
    std::int64_t vx_raw = 0, vy_raw = 0, vz_raw = 0;
    bytes data;
};
```

Flat tagged report. Only the fields relevant to `type` are populated; the
rest keep their zero defaults. Raw integers are authoritative.

### parse_input_cargo

```cpp
std::vector<Report> parse_input_cargo(byte_span payload, std::int64_t capture_timestamp_us);
```

Parse a channel-3/4 cargo into typed reports. Handles 0xFB base timestamp
references and 0xFA rebases; every report's timestamp is base + delay where
base = capture − base_delta·100 µs.

### parse_gyro_rv_cargo

```cpp
std::optional<Report> parse_gyro_rv_cargo(byte_span payload, std::int64_t capture_timestamp_us);
```

Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes: 7×i16
bare, or prefixed with 0xFB + i32 base delta + u16 delay (100 µs ticks).

## BNO055 (IMU)

### Bno055Cmd

```cpp
enum class Bno055Cmd : std::uint8_t {
    ReadReg = 0x32,      // addr u8, len u8 -> RPT_BNO_REG_DATA
    WriteReg = 0x33,     // addr u8, data[1..128]
    Reset = 0x34,        // no payload; answered after the chip-ID handshake (~0.5 s)
    StartStream = 0x35,  // trigger u8, addr u8, len u8, period_ms u16
    StopStream = 0x36,   // no payload
    GetInfo = 0x37,      // no payload -> RPT_BNO_INFO
};
```

Bridge commands (0x32..0x37; 0x30/0x31 are the common sync pins).

### Bno055Rpt

```cpp
enum class Bno055Rpt : std::uint8_t {
    RegData = 0x91,  // RPT_BNO_REG_DATA
    Info = 0x92,     // RPT_BNO_INFO (38 bytes)
    Stream = 0x93,   // RPT_BNO_REG_STREAM
};
```

### XFER_MAX

```cpp
inline constexpr std::size_t XFER_MAX = 128;
```

Max bytes per READ_REG / WRITE_REG / streamed block; addr + len <= 0x100.

### INFO_SIZE

```cpp
inline constexpr std::size_t INFO_SIZE = 38;
```

RPT_BNO_INFO payload size.

### TRIGGER_TIMER

```cpp
inline constexpr std::uint8_t TRIGGER_TIMER = 0;
```

BNO_START_STREAM trigger. Data-ready interrupts do not exist on sensor SW
03.11: TIMER is the only data trigger. INT reads on each INT rising edge
(motion interrupts); period_ms is then a missed-edge watchdog, 0 = off.

### TRIGGER_INT

```cpp
inline constexpr std::uint8_t TRIGGER_INT = 1;
```

### pack_read_reg

```cpp
bytes pack_read_reg(std::uint8_t addr, std::uint8_t len);
```

BNO_READ_REG payload: addr u8, len u8.

### pack_write_reg

```cpp
bytes pack_write_reg(std::uint8_t addr, byte_span data);
```

BNO_WRITE_REG payload: addr u8 then the raw register data (1..XFER_MAX).

### pack_start_stream

```cpp
bytes pack_start_stream(std::uint8_t trigger, std::uint8_t addr, std::uint8_t len, std::uint16_t period_ms);
```

BNO_START_STREAM payload: trigger u8, addr u8, len u8, period_ms u16 LE.

### RegData

```cpp
struct RegData {
    std::uint8_t cmd = 0;
    std::uint64_t timestamp_us = 0;
    bytes data;
};
```

RPT_BNO_REG_DATA — one register read. `cmd` echoes 0x32; `timestamp_us` is
MCU uptime at I2C-read completion.

#### RegData.unpack

```cpp
static std::optional<RegData> unpack(byte_span payload);
```

Decode a RPT_BNO_REG_DATA payload (9+N bytes); nullopt when too short.

### Bno055Info

```cpp
struct Bno055Info {
    std::uint8_t i2c_addr = 0;  // 0x28
    std::uint8_t chip_id = 0;  // healthy 0xA0
    std::uint8_t acc_id = 0;  // 0xFB
    std::uint8_t mag_id = 0;  // 0x32
    std::uint8_t gyr_id = 0;  // 0x0F
    std::uint16_t sw_rev = 0;  // BCD: 0x0311 = 03.11
    std::uint8_t bl_rev = 0;
    std::uint8_t initialized = 0;  // 1 = chip-ID handshake passed
    std::uint8_t int_level = 0;
    std::uint32_t int_edges = 0;
    std::uint16_t read_min_us = 0;
    std::uint16_t read_max_us = 0;
    std::uint16_t read_avg_us = 0;
    std::uint32_t tx_dropped = 0;
    std::uint32_t i2c_errors = 0;
    std::uint32_t slots_skipped = 0;
    std::uint16_t bus_recoveries = 0;
    std::uint8_t last_i2c_error = 0;  // 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    std::uint8_t sensor_resets = 0;
    std::uint16_t loop_max_us = 0;
};
```

RPT_BNO_INFO (38 bytes, `<BBBBBHBBBIHHHIIIHBBH`) — sensor identity
(registers 0x00..0x06) plus bridge diagnostics. Counters are free-running
and wrap; read_*_us, slots_skipped and loop_max_us reset at START_STREAM. A
rising sensor_resets means the bridge pulsed nRESET: the sensor is back in
CONFIG and the host must re-apply its configuration.

#### Bno055Info.unpack

```cpp
static std::optional<Bno055Info> unpack(byte_span payload);
```

Decode a RPT_BNO_INFO payload; nullopt when shorter than INFO_SIZE.

### StreamData

```cpp
struct StreamData {
    std::uint64_t timestamp_us = 0;
    std::uint8_t addr = 0;
    std::uint8_t len = 0;
    bytes data;  // payload[10..10+len]
};
```

RPT_BNO_REG_STREAM — one streamed register block. `addr`/`len` echo the
stream configuration; `timestamp_us` is the trigger time (timer expiry or
INT edge), not the I2C completion.

#### StreamData.unpack

```cpp
static std::optional<StreamData> unpack(byte_span payload);
```

Decode a RPT_BNO_REG_STREAM payload (10+len bytes); nullopt when shorter
than the 10-byte header.

### REG_CHIP_ID

```cpp
inline constexpr std::uint8_t REG_CHIP_ID = 0x00;
```

### REG_PAGE_ID

```cpp
inline constexpr std::uint8_t REG_PAGE_ID = 0x07;
```

host-owned; always back to 0

### REG_ACC_DATA

```cpp
inline constexpr std::uint8_t REG_ACC_DATA = 0x08;
```

3 x i16 LE (x, y, z)

### REG_MAG_DATA

```cpp
inline constexpr std::uint8_t REG_MAG_DATA = 0x0E;
```

### REG_GYR_DATA

```cpp
inline constexpr std::uint8_t REG_GYR_DATA = 0x14;
```

### REG_EUL_DATA

```cpp
inline constexpr std::uint8_t REG_EUL_DATA = 0x1A;
```

heading, roll, pitch

### REG_QUA_DATA

```cpp
inline constexpr std::uint8_t REG_QUA_DATA = 0x20;
```

4 x i16: w, x, y, z

### REG_LIA_DATA

```cpp
inline constexpr std::uint8_t REG_LIA_DATA = 0x28;
```

linear accel (gravity removed)

### REG_GRV_DATA

```cpp
inline constexpr std::uint8_t REG_GRV_DATA = 0x2E;
```

gravity vector

### REG_TEMP

```cpp
inline constexpr std::uint8_t REG_TEMP = 0x34;
```

i8

### REG_CALIB_STAT

```cpp
inline constexpr std::uint8_t REG_CALIB_STAT = 0x35;
```

### REG_ST_RESULT

```cpp
inline constexpr std::uint8_t REG_ST_RESULT = 0x36;
```

### REG_INT_STA

```cpp
inline constexpr std::uint8_t REG_INT_STA = 0x37;
```

clears on read

### REG_SYS_CLK_STATUS

```cpp
inline constexpr std::uint8_t REG_SYS_CLK_STATUS = 0x38;
```

### REG_SYS_STATUS

```cpp
inline constexpr std::uint8_t REG_SYS_STATUS = 0x39;
```

### REG_SYS_ERR

```cpp
inline constexpr std::uint8_t REG_SYS_ERR = 0x3A;
```

### REG_UNIT_SEL

```cpp
inline constexpr std::uint8_t REG_UNIT_SEL = 0x3B;
```

### REG_OPR_MODE

```cpp
inline constexpr std::uint8_t REG_OPR_MODE = 0x3D;
```

bits 3:0

### REG_PWR_MODE

```cpp
inline constexpr std::uint8_t REG_PWR_MODE = 0x3E;
```

### REG_SYS_TRIGGER

```cpp
inline constexpr std::uint8_t REG_SYS_TRIGGER = 0x3F;
```

### REG_TEMP_SOURCE

```cpp
inline constexpr std::uint8_t REG_TEMP_SOURCE = 0x40;
```

### REG_AXIS_MAP_CONFIG

```cpp
inline constexpr std::uint8_t REG_AXIS_MAP_CONFIG = 0x41;
```

### REG_AXIS_MAP_SIGN

```cpp
inline constexpr std::uint8_t REG_AXIS_MAP_SIGN = 0x42;
```

### REG_SIC_MATRIX

```cpp
inline constexpr std::uint8_t REG_SIC_MATRIX = 0x43;
```

9 x i16, 1.0 = 16384

### REG_CALIB_PROFILE

```cpp
inline constexpr std::uint8_t REG_CALIB_PROFILE = 0x55;
```

22 bytes, CONFIG mode only

### REG1_ACC_CONFIG

```cpp
inline constexpr std::uint8_t REG1_ACC_CONFIG = 0x08;
```

### REG1_MAG_CONFIG

```cpp
inline constexpr std::uint8_t REG1_MAG_CONFIG = 0x09;
```

### REG1_GYR_CONFIG_0

```cpp
inline constexpr std::uint8_t REG1_GYR_CONFIG_0 = 0x0A;
```

### REG1_GYR_CONFIG_1

```cpp
inline constexpr std::uint8_t REG1_GYR_CONFIG_1 = 0x0B;
```

### REG1_INT_MSK

```cpp
inline constexpr std::uint8_t REG1_INT_MSK = 0x0F;
```

### REG1_INT_EN

```cpp
inline constexpr std::uint8_t REG1_INT_EN = 0x10;
```

### REG1_UNIQUE_ID

```cpp
inline constexpr std::uint8_t REG1_UNIQUE_ID = 0x50;
```

16 bytes

### FULL_BLOCK_ADDR

```cpp
inline constexpr std::uint8_t FULL_BLOCK_ADDR = REG_ACC_DATA;
```

Every output channel in one read (ACC_DATA_X_LSB .. CALIB_STAT), and the
quaternion alone (the cheapest orientation read).

### FULL_BLOCK_LEN

```cpp
inline constexpr std::uint8_t FULL_BLOCK_LEN = 46;
```

### QUAT_BLOCK_ADDR

```cpp
inline constexpr std::uint8_t QUAT_BLOCK_ADDR = REG_QUA_DATA;
```

### QUAT_BLOCK_LEN

```cpp
inline constexpr std::uint8_t QUAT_BLOCK_LEN = 8;
```

### OprMode

```cpp
enum class OprMode : std::uint8_t {
    Config = 0x00,
    AccOnly = 0x01,
    MagOnly = 0x02,
    GyroOnly = 0x03,
    AccMag = 0x04,
    AccGyro = 0x05,
    MagGyro = 0x06,
    Amg = 0x07,
    Imu = 0x08,
    Compass = 0x09,
    M4g = 0x0A,
    NdofFmcOff = 0x0B,
    Ndof = 0x0C,
};
```

OPR_MODE (0x3D) bits 3:0; IMU and above are fusion modes.

### UNIT_ACC_MG

```cpp
inline constexpr std::uint8_t UNIT_ACC_MG = 0x01;
```

ACC_DATA in mg, else m/s^2

### UNIT_GYR_RPS

```cpp
inline constexpr std::uint8_t UNIT_GYR_RPS = 0x02;
```

rad/s, else dps

### UNIT_EUL_RAD

```cpp
inline constexpr std::uint8_t UNIT_EUL_RAD = 0x04;
```

radians, else degrees

### UNIT_TEMP_F

```cpp
inline constexpr std::uint8_t UNIT_TEMP_F = 0x10;
```

deg F (1 LSB = 2 F), else C

### UNIT_ORI_ANDROID

```cpp
inline constexpr std::uint8_t UNIT_ORI_ANDROID = 0x80;
```

the power-on value is 0x80

### Units

```cpp
struct Units {
    bool accel_mg = false;  // ACC_DATA only: linear accel / gravity stay m/s^2
    bool gyro_rps = false;
    bool euler_rad = false;
    bool temp_f = false;
    bool android = false;
};
```

Output units. The default is UNIT_SEL = 0x00 (m/s^2, dps, degrees, deg C,
Windows orientation); a fresh sensor powers up in Android orientation, so
hosts write UNIT_SEL explicitly.

#### Units.pack

```cpp
std::uint8_t pack() const;
```

#### Units.unpack

```cpp
static Units unpack(std::uint8_t unit_sel);
```

Undefined UNIT_SEL bits are ignored (they do nothing on the sensor).

#### Units.accel_lsb

```cpp
double accel_lsb() const;
```

LSB per unit: value = raw / lsb.

#### Units.gyro_lsb

```cpp
double gyro_lsb() const;
```

#### Units.euler_lsb

```cpp
double euler_lsb() const;
```

#### Units.temp_lsb

```cpp
double temp_lsb() const;
```

deg F: 1 LSB = 2 F

### MAG_LSB

```cpp
inline constexpr double MAG_LSB = 16.0;
```

uT, not selectable

### QUAT_LSB

```cpp
inline constexpr double QUAT_LSB = 16384.0;
```

2^14, unit-less

### FUSION_ACCEL_LSB

```cpp
inline constexpr double FUSION_ACCEL_LSB = 100.0;
```

Linear acceleration and gravity ignore the ACC_Unit bit: always m/s^2 at
100 LSB (measured on SW 03.11; the datasheet's Tables 3-33/3-35 say mg).

### CalibStatus

```cpp
struct CalibStatus {
    std::uint8_t system = 0;
    std::uint8_t gyro = 0;
    std::uint8_t accel = 0;
    std::uint8_t mag = 0;
};
```

CALIB_STAT (0x35): sys<7:6> gyr<5:4> acc<3:2> mag<1:0>, 0 = not
calibrated .. 3 = fully calibrated.

#### CalibStatus.pack

```cpp
std::uint8_t pack() const;
```

#### CalibStatus.unpack

```cpp
static CalibStatus unpack(std::uint8_t value);
```

#### CalibStatus.fully_calibrated

```cpp
bool fully_calibrated() const;
```

3/3/3/3.

### CALIB_PROFILE_LEN

```cpp
inline constexpr std::size_t CALIB_PROFILE_LEN = 22;
```

### CalibrationProfile

```cpp
struct CalibrationProfile {
    std::array<std::int16_t, 3> accel_offset{};
    std::array<std::int16_t, 3> mag_offset{};
    std::array<std::int16_t, 3> gyro_offset{};
    std::int16_t accel_radius = 0;
    std::int16_t mag_radius = 0;
};
```

Offsets and radii at 0x55..0x6A (11 x i16 LE, sensor LSB). Readable and
writable only in CONFIG; write all 22 bytes in one transfer (the sensor
latches each group on its MSB). A written profile is a starting point: the
sensor keeps calibrating once back in a fusion mode.

#### CalibrationProfile.pack

```cpp
bytes pack() const;
```

22 bytes

#### CalibrationProfile.unpack

```cpp
static std::optional<CalibrationProfile> unpack(byte_span data);
```

nullopt unless data is exactly CALIB_PROFILE_LEN bytes.

### AXIS_X

```cpp
inline constexpr std::uint8_t AXIS_X = 0;
```

### AXIS_Y

```cpp
inline constexpr std::uint8_t AXIS_Y = 1;
```

### AXIS_Z

```cpp
inline constexpr std::uint8_t AXIS_Z = 2;
```

### AxisRemap

```cpp
struct AxisRemap {
    std::uint8_t x = AXIS_X;
    std::uint8_t y = AXIS_Y;
    std::uint8_t z = AXIS_Z;
    bool x_negative = false;
    bool y_negative = false;
    bool z_negative = false;
};
```

Which chip axis feeds each output axis, and its sign: `x = AXIS_Y` means
"output X is the chip's Y axis".

#### AxisRemap.pack

```cpp
std::optional<std::pair<std::uint8_t, std::uint8_t>> pack() const;
```

(AXIS_MAP_CONFIG = z<5:4> y<3:2> x<1:0>, AXIS_MAP_SIGN = x 2, y 1, z 0).
nullopt when x/y/z is not a permutation of 0/1/2: the sensor would
silently keep its old mapping.

#### AxisRemap.unpack

```cpp
static AxisRemap unpack(std::uint8_t config, std::uint8_t sign);
```

### PLACEMENTS

```cpp
inline constexpr std::array<std::pair<std::uint8_t, std::uint8_t>, 8> PLACEMENTS = {{ {0x21, 0x04}, {0x24, 0x00}, {0x24, 0x06}, {0x21, 0x02}, {0x24, 0x03}, {0x21, 0x01}, {0x21, 0x07}, {0x24, 0x05}, }};
```

Datasheet §3.4 mounting placements P0..P7 as (AXIS_MAP_CONFIG,
AXIS_MAP_SIGN); P1 (0x24/0x00) is the power-on default.

### placement

```cpp
std::optional<AxisRemap> placement(const std::string& name);
```

"P0".."P7" (case-insensitive); nullopt for any other name.

### AccelConfig

```cpp
struct AccelConfig {
    std::uint8_t range = 1;
    std::uint8_t bandwidth = 3;
    std::uint8_t power = 0;
};
```

ACC_CONFIG (page 1, 0x08): range<1:0> (2/4/8/16 g), bandwidth<4:2>
(7.81..1000 Hz), power<7:5>. Power-on 0x0D = 4 g, 62.5 Hz, normal.

#### AccelConfig.pack

```cpp
std::uint8_t pack() const;
```

#### AccelConfig.unpack

```cpp
static AccelConfig unpack(std::uint8_t value);
```

### GyroConfig

```cpp
struct GyroConfig {
    std::uint8_t range = 0;
    std::uint8_t bandwidth = 7;
    std::uint8_t power = 0;
};
```

GYR_CONFIG_0/1 (page 1, 0x0A/0x0B): byte 0 range<2:0> (2000..125 dps),
bandwidth<5:3>; byte 1 power<2:0>. Power-on 0x38/0x00 = 2000 dps, 32 Hz.

#### GyroConfig.pack

```cpp
bytes pack() const;
```

2 bytes

#### GyroConfig.unpack

```cpp
static std::optional<GyroConfig> unpack(byte_span data);
```

nullopt when data is shorter than 2 bytes.

### MagConfig

```cpp
struct MagConfig {
    std::uint8_t rate = 3;
    std::uint8_t mode = 1;
    std::uint8_t power = 0;
};
```

MAG_CONFIG (page 1, 0x09): rate<2:0> (2..30 Hz), mode<4:3>, power<6:5>;
bit 7 is not a field, so a repack drops it. Power-on 0x0B = 10 Hz, regular.

#### MagConfig.pack

```cpp
std::uint8_t pack() const;
```

#### MagConfig.unpack

```cpp
static MagConfig unpack(std::uint8_t value);
```

### Vec3

```cpp
using Vec3 = std::array<std::int16_t, 3>;
```

### Quat

```cpp
using Quat = std::array<std::int16_t, 4>;
```

### RawBlock

```cpp
struct RawBlock {
    std::optional<Vec3> accel;
    std::optional<Vec3> mag;
    std::optional<Vec3> gyro;
    std::optional<Vec3> euler;  // heading, roll, pitch
    std::optional<Quat> quaternion;  // w, x, y, z
    std::optional<Vec3> linear_accel;
    std::optional<Vec3> gravity;
    std::optional<std::int8_t> temperature;
    std::optional<std::uint8_t> calib_stat;  // CALIB_STAT byte
};
```

Raw register values found in one block read. A channel is nullopt when the
window addr..addr+len does not cover all of its bytes.

### decode_block

```cpp
RawBlock decode_block(std::uint8_t addr, byte_span data);
```

Unpack whatever channels the register window starting at `addr` holds.

## Bootloader / firmware

### FWDEPZ_HEADER_SIZE

```cpp
inline constexpr std::size_t FWDEPZ_HEADER_SIZE = 64;
```

### FWDEPZ_MAGIC

```cpp
inline constexpr char FWDEPZ_MAGIC[8] = {'F', 'W', 'D', 'E', 'P', 'Z', '0', '0'};
```

Magic "FWDEPZ00".

### BlCmd

```cpp
enum class BlCmd : std::uint8_t {
    BootApplication = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetFirmwareName = 0x04,
    GetSerial = 0x05,
    GetMcuId = 0x06,
    GetMcuUid = 0x07,
    GetFlashInfo = 0x08,
    EraseApp = 0x09,
    WritePage = 0x0A,
    ReadPage = 0x0B,
    VerifyAppCrc = 0x0C,
};
```

### BlRpt

```cpp
enum class BlRpt : std::uint8_t {
    Status = 0x80,
    String = 0x81,
    McuId = 0x86,
    McuUid = 0x87,
    FlashInfo = 0x89,
    WritePage = 0x8A,
    ReadPage = 0x8B,
    VerifyAppCrc = 0x8C,
};
```

### BlStatus

```cpp
enum class BlStatus : std::uint8_t {
    Ack = 0x00,
    Error = 0x01,
    ErrAddr = 0x03,
    ErrCrcHdr = 0x04,
    ErrCrcPkt = 0x05,
    ErrFlash = 0x06,
};
```

### FlashInfo

```cpp
struct FlashInfo {
    std::uint16_t page_size;
    std::uint32_t app_start;
    std::uint32_t app_size;
};
```

#### FlashInfo.unpack

```cpp
static FlashInfo unpack(byte_span payload);
```

### pack_write_page

```cpp
bytes pack_write_page(std::uint32_t addr, byte_span data);
```

### pack_read_page

```cpp
bytes pack_read_page(std::uint32_t addr, std::uint16_t size);
```

### FwDepzErrorKind

```cpp
enum class FwDepzErrorKind {
    TooShort, Magic, HeaderCrc, Size
};
```

### FwDepzError

```cpp
class FwDepzError : public std::runtime_error {
    FwDepzErrorKind kind;
};
```

#### FwDepzError.FwDepzError

```cpp
FwDepzError(FwDepzErrorKind kind, const std::string& msg);
```

### FwDepzImage

```cpp
struct FwDepzImage {
    std::uint32_t load_addr;
    std::uint32_t fw_size;
    std::uint32_t fw_crc32;
    std::uint8_t cur_sec;
    std::uint8_t tot_sec;
    bytes payload;
};
```

Parsed and validated .fwdepz firmware container.

#### FwDepzImage.parse

```cpp
static FwDepzImage parse(byte_span blob);
```

Validation order: length, magic, header CRC (CCITT-FALSE over [0..61]
vs u16 LE at 62), then fw_size == payload length. Throws FwDepzError.

#### FwDepzImage.build

```cpp
static bytes build(std::uint32_t load_addr, byte_span payload, std::uint8_t cur_sec = 1, std::uint8_t tot_sec = 1);
```

Assemble a container (test/tooling helper).

#### FwDepzImage.payload_crc_ok

```cpp
bool payload_crc_ok() const;
```

## Datasets

### SCHEMA_PREFIX

```cpp
inline constexpr const char* SCHEMA_PREFIX = "depz.dataset/";
```

### TimeSync

```cpp
struct TimeSync {
    std::int64_t offset_us = 0;
    std::int64_t rtt_us = 0;
};
```

### DeviceMeta

```cpp
struct DeviceMeta {
    std::string serial;
    std::string sensor_type;
    std::string software_name;
    TimeSync time_sync;
};
```

### Record

```cpp
struct Record {
    std::string device_id;
    std::int64_t t_host_us = 0;
    std::string kind;
    std::map<std::string, std::int64_t> ints;
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<std::int64_t>> arrays;
};
```

One record. The value object `v` is split by JSON type into typed maps
(numbers, strings, integer arrays) — enough for the SR04 and VL53L8 kinds.

### Reader

```cpp
class Reader {
    std::string schema;
    std::string created_utc;
    std::string note;
    std::map<std::string, DeviceMeta> devices;
    std::vector<Record> records;  // stable-sorted by t_host_us
};
```

#### Reader.Reader

```cpp
explicit Reader(const std::string& content);
```

Parse the whole `.depzdata` file content. Throws std::runtime_error on a
malformed file or a non-depz.dataset schema.

#### Reader.duration_us

```cpp
std::int64_t duration_us() const;
```

## Live hardware (links, device, discovery)

### Error

```cpp
class Error : public std::runtime_error {
    // (no public data members)
};
```

Base of every live-layer error; `code()` is the C SDK's depz_err value.

#### Error.Error

```cpp
Error(int code, const std::string& what);
```

#### Error.code

```cpp
int code() const noexcept;
```

### ArgumentError

```cpp
class ArgumentError : public Error {
    // (no public data members)
};
```

### IoError

```cpp
class IoError : public Error {
    // (no public data members)
};
```

the OS refused

### DeviceLostError

```cpp
class DeviceLostError : public Error {
    // (no public data members)
};
```

closed / unplugged

### TimeoutError

```cpp
class TimeoutError : public Error {
    // (no public data members)
};
```

### ProtocolError

```cpp
class ProtocolError : public Error {
    // (no public data members)
};
```

a reply that does not parse

### NoDeviceError

```cpp
class NoDeviceError : public Error {
    // (no public data members)
};
```

### WrongTypeError

```cpp
class WrongTypeError : public Error {
    // (no public data members)
};
```

### ReplayMismatchError

```cpp
class ReplayMismatchError : public Error {
    // (no public data members)
};
```

### BootloaderModeError

```cpp
class BootloaderModeError : public Error {
    // (no public data members)
};
```

### StatusError

```cpp
class StatusError : public Error {
    // (no public data members)
};
```

The device answered a non-OK RPT_STATUS.

#### StatusError.StatusError

```cpp
StatusError(int code, const std::string& what, std::uint8_t cmd, Status status);
```

#### StatusError.cmd

```cpp
std::uint8_t cmd() const noexcept;
```

#### StatusError.status

```cpp
Status status() const noexcept;
```

### BusyError

```cpp
class BusyError : public StatusError {
    // (no public data members)
};
```

ERR_BUSY, or the same opcode already in flight.

### Link

```cpp
class Link {
    // (no public data members)
};
```

A byte pipe to a device: serial port, loopback, .depzrec record/replay.

#### Link.serial

```cpp
static Link serial(const std::string& port);
```

The CDC-ACM port, opened exclusively ("/dev/ttyACM0", "COM7").

#### Link.replay

```cpp
static Link replay(const std::string& path, bool strict_tx = false, bool realtime = false);
```

Causal replay of a capture; `strict_tx` throws ReplayMismatchError on
any write that differs from the recorded requests.

#### Link.recording

```cpp
static Link recording(Link inner, const std::string& path, const std::string& header_extra_json = "");
```

Tee `inner` into a new .depzrec file. `header_extra_json`: the inside of
a JSON object merged into the header (`"port": "live"`), or "".

#### Link.loopback_pair

```cpp
static std::pair<Link, Link> loopback_pair();
```

Two in-memory ends: what one writes, the other reads.

#### Link.adopt

```cpp
static Link adopt(depz_link* l) noexcept;
```

Take ownership of a C link (a custom depz_link_new() one, say).

#### Link.read

```cpp
bytes read(std::chrono::milliseconds timeout);
```

Next chunk (empty on timeout); DeviceLostError once closed.

#### Link.write

```cpp
void write(byte_span data);
```

#### Link.close

```cpp
void close();
```

#### Link.closed

```cpp
bool closed() const;
```

#### Link.name

```cpp
std::string name() const;
```

#### Link.replay_exhausted

```cpp
bool replay_exhausted() const;
```

Replay links: every recorded rx chunk has been served.

#### Link.c_handle

```cpp
depz_link* c_handle() const noexcept;
```

#### Link.release

```cpp
depz_link* release() noexcept;
```

### Stream

```cpp
template <class T> class Stream {
    // (no public data members)
};
```

A bounded drop-oldest pull subscription (contract 07 §3). Registered at
creation: nothing produced after it is missed.

#### Stream.Stream

```cpp
explicit Stream(depz_stream* s);
```

#### Stream.next

```cpp
std::optional<T> next(std::chrono::milliseconds timeout = std::chrono::milliseconds(200));
```

The next item, or nullopt on timeout — and once the device is closed
and the stream drained (then closed() is true).

#### Stream.closed

```cpp
bool closed() const noexcept;
```

#### Stream.dropped_count

```cpp
std::uint64_t dropped_count() const;
```

### LinkStats

```cpp
struct LinkStats {
    std::uint64_t tx_packets, rx_packets, tx_bytes, rx_bytes;
    std::uint64_t crc_errors, header_errors, trash_bytes;
    std::uint64_t seq_gaps, device_seq_errors;
};
```

### TimeSync

```cpp
struct TimeSync {
    std::int64_t offset_us;  // device clock - host clock
    std::int64_t rtt_us;
    std::uint64_t synced_at_host_us;
};
```

### DeviceEvent

```cpp
struct DeviceEvent {
    enum class Type { SequenceError, CrcError, Trash, UnsolicitedStatus, Text, Temperature, Disconnected };
    Type type;
    std::uint8_t cmd = 0, seq = 0;
    std::uint8_t expected_seq = 0, received_seq = 0;
    bool by_device = false;  // SequenceError reported by the device
    std::uint8_t status = 0;  // UnsolicitedStatus (ERR_HARDWARE_FAULT = a fault)
    std::uint64_t timestamp_us = 0;  // Temperature
    double celsius = 0.0;
    std::size_t trash_len = 0;  // Trash: total length; `data` = the first bytes
    bytes data;
    std::string text;  // Text; Disconnected: the reason
};
```

#### DeviceEvent.is_hardware_fault

```cpp
bool is_hardware_fault() const;
```

#### DeviceEvent.pull

```cpp
static std::optional<DeviceEvent> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<DeviceEvent> plumbing.

### host_now_us

```cpp
std::uint64_t host_now_us();
```

Host monotonic clock, µs — the host side of all time-sync math.

### Device

```cpp
class Device {
    // (no public data members)
};
```

A connection to one DEPZ device in application mode (contract 02 / 07).

#### Device.open

```cpp
static std::unique_ptr<Device> open(const std::string& port);
```

A plain device on a port / a link, no identity probe.

#### Device.open

```cpp
static std::unique_ptr<Device> open(Link link);
```

#### Device.close

```cpp
void close();
```

Stop the reader and close the link (the destructor does it too).

#### Device.closed

```cpp
bool closed() const;
```

#### Device.port

```cpp
std::string port() const;
```

#### Device.sensor_type

```cpp
std::optional<SensorType> sensor_type() const;
```

nullopt until identified (open_device / promote).

#### Device.set_timeout

```cpp
void set_timeout(std::chrono::milliseconds t);
```

#### Device.stats

```cpp
LinkStats stats() const;
```

#### Device.device_name

```cpp
std::string device_name();
```

#### Device.software_name

```cpp
std::string software_name();
```

#### Device.serial_number

```cpp
std::string serial_number();
```

#### Device.read_mcu_temperature

```cpp
double read_mcu_temperature();
```

°C, cached by the device ~2 Hz

#### Device.payload_crc_type

```cpp
CrcType payload_crc_type();
```

#### Device.set_payload_crc_type

```cpp
void set_payload_crc_type(CrcType t);
```

device->host payload CRC

#### Device.sync_pin

```cpp
SyncPinConfig sync_pin(std::uint8_t pin);
```

#### Device.set_sync_pin

```cpp
void set_sync_pin(const SyncPinConfig& c);
```

#### Device.reset

```cpp
void reset();
```

the device ACKs, then reboots; the link drops

#### Device.enter_bootloader

```cpp
void enter_bootloader();
```

reboot into the bootloader; closes this device

#### Device.sync_time

```cpp
TimeSync sync_time(int samples = 5);
```

#### Device.time_sync

```cpp
std::optional<TimeSync> time_sync() const;
```

#### Device.to_host_time_us

```cpp
std::int64_t to_host_time_us(std::uint64_t device_us) const;
```

#### Device.promote

```cpp
static std::unique_ptr<Device> promote(std::unique_ptr<Device> dev);
```

Identify (GET_NAME_ACTIVE_SOFTWARE) and return the matching class,
consuming this plain device — what open_device() does after its probe.

#### Device.request_ok

```cpp
void request_ok(std::uint8_t cmd, byte_span payload = {}, std::optional<std::chrono::milliseconds> timeout = std::nullopt);
```

Escape hatch: send `cmd` and wait for RPT_STATUS(cmd, OK) ...

#### Device.request

```cpp
bytes request(std::uint8_t cmd, byte_span payload, std::function<bool(std::uint8_t rpt, byte_span payload)> match, std::optional<std::chrono::milliseconds> timeout = std::nullopt);
```

... or for the first packet `match` accepts (reader thread, device lock
held: copy what you need, nothing else). Returns that packet's payload.

#### Device.send

```cpp
void send(std::uint8_t cmd, byte_span payload = {});
```

#### Device.on_event

```cpp
Unsubscribe on_event(std::function<void(const DeviceEvent&)> cb);
```

#### Device.events

```cpp
Stream<DeviceEvent> events(std::size_t maxsize = 256);
```

#### Device.c_handle

```cpp
depz_device* c_handle() const noexcept;
```

### Sr04Measurement

```cpp
struct Sr04Measurement {
    std::uint64_t timestamp_us;  // device µs
    std::uint16_t echo_time_us;  // ECHO_TIMEOUT = no echo
    bool from_loop;  // false: MEASURE_ONCE or a SYNC_IN edge
};
```

#### Sr04Measurement.valid

```cpp
bool valid() const;
```

#### Sr04Measurement.distance_mm

```cpp
std::optional<double> distance_mm() const;
```

Distance at 343 m/s; nullopt without an echo.

#### Sr04Measurement.distance_mm_at

```cpp
std::optional<double> distance_mm_at(double air_temp_c) const;
```

Temperature-compensated speed of sound (331.3 + 0.606·T m/s).

#### Sr04Measurement.pull

```cpp
static std::optional<Sr04Measurement> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Sr04Measurement> plumbing.

### Sr04

```cpp
class Sr04 : public Device {
    // (no public data members)
};
```

#### Sr04.open

```cpp
static std::unique_ptr<Sr04> open(Link link);
```

An SR04 on a link without an identity probe (tests, replay).

#### Sr04.sample_period_us

```cpp
std::uint32_t sample_period_us();
```

Stored minimum interval between measurement starts (default 50000 µs);
the effective rate is also limited by the echo window (contract 03 §3).

#### Sr04.set_sample_period_us

```cpp
void set_sample_period_us(std::uint32_t period_us);
```

#### Sr04.echo_decay_us

```cpp
std::uint16_t echo_decay_us();
```

#### Sr04.set_echo_decay_us

```cpp
std::uint16_t set_echo_decay_us(std::uint32_t decay_us);
```

The device clamps to 4000..65000 µs; returns the value in effect.
ArgumentError above 65535 (the u16 wire field).

#### Sr04.measure_once

```cpp
Sr04Measurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
```

Single shot; BusyError while the loop runs.

#### Sr04.start

```cpp
void start();
```

measurement loop; idempotent

#### Sr04.stop

```cpp
void stop();
```

#### Sr04.on_measurement

```cpp
Unsubscribe on_measurement(std::function<void(const Sr04Measurement&)> cb);
```

Loop samples and SYNC_IN single shots.

#### Sr04.stream

```cpp
Stream<Sr04Measurement> stream(std::size_t maxsize = 256);
```

### Vl53l4cdMeasurement

```cpp
struct Vl53l4cdMeasurement {
    std::uint64_t timestamp_us;  // MCU µs at the INT edge (stream) / the read (poll)
    vl53l4::Vl53l4Result r;  // r.range_status 0 = valid, r.distance_mm, ...
};
```

#### Vl53l4cdMeasurement.valid

```cpp
bool valid() const;
```

#### Vl53l4cdMeasurement.status_text

```cpp
std::string status_text() const;
```

UM2931 name of the range status ("valid", "sigma above threshold", ...).

#### Vl53l4cdMeasurement.pull

```cpp
static std::optional<Vl53l4cdMeasurement> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Vl53l4cdMeasurement> plumbing.

### DetectionWindow

```cpp
enum class DetectionWindow : std::uint8_t {
    Below = 0, Above = 1, Out = 2, In = 3
};
```

When INT fires relative to the window [low_mm, high_mm] (SYSTEM__INTERRUPT).
Once programmed, only init() restores the "no window" default.

### DetectionThresholds

```cpp
struct DetectionThresholds {
    std::uint16_t low_mm, high_mm;
    DetectionWindow window;
};
```

The VL53L4CD distance window: INT only fires when `window` holds.

### Vl53l4cd

```cpp
class Vl53l4cd : public Device {
    // (no public data members)
};
```

VL53L4CD single-zone ToF: the ST ULD 2.2.3 on the host over the bridge.

#### Vl53l4cd.open

```cpp
static std::unique_ptr<Vl53l4cd> open(Link link);
```

A VL53L4CD on a link without an identity probe (tests, replay).

#### Vl53l4cd.is_alive

```cpp
bool is_alive();
```

the sensor answers with its model id

#### Vl53l4cd.init

```cpp
void init(std::uint16_t bus_khz = 1000);
```

Configuration block + VHV calibration, then the bus at `bus_khz`.

#### Vl53l4cd.initialized

```cpp
bool initialized() const;
```

#### Vl53l4cd.ranging

```cpp
bool ranging() const;
```

#### Vl53l4cd.xshut

```cpp
void xshut(std::uint8_t action);
```

vl53l4::XSHUT_OFF / _ON / _RESET

#### Vl53l4cd.reset_sensor

```cpp
void reset_sensor();
```

#### Vl53l4cd.bridge_info

```cpp
vl53l4::Vl53l4Info bridge_info();
```

#### Vl53l4cd.set_i2c_speed_khz

```cpp
void set_i2c_speed_khz(std::uint16_t khz);
```

#### Vl53l4cd.set_range_timing

```cpp
void set_range_timing(std::uint32_t budget_ms, std::uint32_t inter_ms = 0);
```

Budget 10..200 ms; inter 0 = continuous, > budget = autonomous.

#### Vl53l4cd.range_timing

```cpp
vl53l4::RangeTiming range_timing();
```

#### Vl53l4cd.set_offset_mm

```cpp
void set_offset_mm(std::int32_t mm);
```

#### Vl53l4cd.offset_mm

```cpp
std::int32_t offset_mm();
```

#### Vl53l4cd.set_xtalk_kcps

```cpp
void set_xtalk_kcps(std::uint16_t kcps);
```

0 = off

#### Vl53l4cd.xtalk_kcps

```cpp
std::uint16_t xtalk_kcps();
```

#### Vl53l4cd.set_detection_thresholds

```cpp
void set_detection_thresholds(const DetectionThresholds& t);
```

#### Vl53l4cd.detection_thresholds

```cpp
DetectionThresholds detection_thresholds();
```

#### Vl53l4cd.set_signal_threshold_kcps

```cpp
void set_signal_threshold_kcps(std::uint16_t kcps);
```

#### Vl53l4cd.signal_threshold_kcps

```cpp
std::uint16_t signal_threshold_kcps();
```

#### Vl53l4cd.set_sigma_threshold_mm

```cpp
void set_sigma_threshold_mm(std::uint16_t mm);
```

<= 16383

#### Vl53l4cd.sigma_threshold_mm

```cpp
std::uint16_t sigma_threshold_mm();
```

#### Vl53l4cd.start_temperature_update

```cpp
void start_temperature_update();
```

after a > 8 °C ambient change

#### Vl53l4cd.calibrate_offset

```cpp
std::int32_t calibrate_offset(std::uint16_t target_mm, std::uint8_t nb_samples = 20);
```

Against a target at `target_mm`; returns the programmed value.

#### Vl53l4cd.calibrate_xtalk

```cpp
std::uint16_t calibrate_xtalk(std::uint16_t target_mm, std::uint8_t nb_samples = 20);
```

#### Vl53l4cd.start_ranging

```cpp
void start_ranging();
```

#### Vl53l4cd.stop_ranging

```cpp
void stop_ranging();
```

idempotent

#### Vl53l4cd.measure_once

```cpp
Vl53l4cdMeasurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
```

Poll mode: start, wait data-ready, read, stop. Refused while ranging.

#### Vl53l4cd.on_measurement

```cpp
Unsubscribe on_measurement(std::function<void(const Vl53l4cdMeasurement&)> cb);
```

#### Vl53l4cd.measurements

```cpp
Stream<Vl53l4cdMeasurement> measurements(std::size_t maxsize = 64);
```

#### Vl53l4cd.get_measurement

```cpp
Vl53l4cdMeasurement get_measurement(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
```

The next streamed measurement; TimeoutError / DeviceLostError.

#### Vl53l4cd.stream_parse_errors

```cpp
std::uint64_t stream_parse_errors() const;
```

#### Vl53l4cd.read_reg

```cpp
bytes read_reg(std::uint16_t addr, std::size_t len);
```

Raw register access (16-bit address, contents as the sensor has them).

#### Vl53l4cd.write_reg

```cpp
void write_reg(std::uint16_t addr, byte_span data);
```

### Vl53l8Model

```cpp
enum class Vl53l8Model {
    L8CX = 0, L8CH = 1, L5CX = 2, L7CX = 3, L7CH = 4
};
```

The multizone boards this class serves: VL53L8CX/CH on the SPI bridge
(APP_VL53L8), VL53L5CX/L7CX/L7CH on the I2C bridge (APP_VL53L7). The model
fixes the sensor firmware init() downloads (L8CX: ST ULD 2.1.0, L8CH:
VL53LMZ 2.0.16 with CNH); open_device() picks L8CH for the VL53L8 production
USB PID 0xED40, else L8CX; on APP_VL53L7 the PID (0xED48 / 0xED49 / 0xED4A),
else the device name, else L7CX.

### Vl53l8Motion

```cpp
struct Vl53l8Motion {
    std::uint32_t global_indicator_1, global_indicator_2;
    std::uint8_t status, nb_of_detected_aggregates, nb_of_aggregates;
    std::array<std::uint32_t, 32> motion;
};
```

The motion-indicator output of a frame (configure_motion_indicator).

### Vl53l8LiveFrame

```cpp
struct Vl53l8LiveFrame {
    vl53l8::Vl53l8Frame frame;
    std::optional<Vl53l8Motion> motion;
};
```

One streamed frame: the zone arrays in `frame` (distance_mm in mm; CH with
CNH armed: frame.cnh_raw, decode with vl53l8::decode_cnh()), plus motion.

#### Vl53l8LiveFrame.pull

```cpp
static std::optional<Vl53l8LiveFrame> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Vl53l8LiveFrame> plumbing.

### CnhSetup

```cpp
struct CnhSetup {
    std::int32_t ref_bin_offset = 0;
    std::uint32_t detection_threshold = 0, extra_noise_sigma = 0, null_den_clip_value = 0;
    std::uint8_t mem_update_mode = 0, mem_update_choice = 0, sum_span = 0, feature_length = 0;
    std::uint8_t nb_of_aggregates = 0, nb_of_temporal_accumulations = 1, min_nb_for_global_detection = 0;
    std::uint8_t global_indicator_format_1 = 0, global_indicator_format_2 = 0;
    std::uint8_t cnh_cfg = 0, cnh_flex_shift = 0, spare_3 = 0;
    std::array<std::int8_t, 64> map_id{};
    std::array<std::uint8_t, 32> indicator_format_1{}, indicator_format_2{};
};
```

VL53LMZ_Motion_Configuration for CNH (compact network histograms, CH only),
filled by the plugin helpers.

#### CnhSetup.init_config

```cpp
void init_config(int start_bin, int num_bins, int sub_sample);
```

vl53lmz_cnh_init_config: histogram start bin, CNH bins, device bins per CNH bin.

#### CnhSetup.create_agg_map

```cpp
void create_agg_map(int resolution, int start_x, int start_y, int merge_x, int merge_y, int cols, int rows);
```

vl53lmz_cnh_create_agg_map: zones (resolution 16 | 64) -> aggregates.

#### CnhSetup.required_memory

```cpp
std::size_t required_memory() const;
```

On-device CNH buffer bytes; ArgumentError when blank or above the cap.

### Vl53l8Progress

```cpp
using Vl53l8Progress = std::function<void(const std::string& phase, std::size_t done, std::size_t total)>;
```

Progress of init(): phase text, and done/total bytes during the big writes.

### Vl53l8

```cpp
class Vl53l8 : public Device {
    // (no public data members)
};
```

Multizone ToF: the ST ULD on the host over the board's register bridge,
one class for every Vl53l8Model.

#### Vl53l8.open

```cpp
static std::unique_ptr<Vl53l8> open(Link link, Vl53l8Model model);
```

A multizone ToF on a link without an identity probe, the model given by
hand (tests, replay).

#### Vl53l8.model

```cpp
Vl53l8Model model() const;
```

fixed at open: the USB PID, or open(link, model)

#### Vl53l8.is_alive

```cpp
bool is_alive();
```

id 0xF0 / rev 0x0C over SPI; rev 0x02 (or 0xF0 / 0x01) on L5/L7

#### Vl53l8.init

```cpp
void init(Vl53l8Progress progress = {});
```

Boot the sensor MCU, download its firmware, upload NVM / xtalk / config
(~0.8 s over SPI, ~1.3 s over I2C). Needed after every power-up, deep
sleep and L5/L7 LpnOff / SoftCycle; it disarms CNH.

#### Vl53l8.initialized

```cpp
bool initialized() const;
```

#### Vl53l8.ranging

```cpp
bool ranging() const;
```

between start_ranging() and stop_ranging()

#### Vl53l8.resolution

```cpp
int resolution();
```

16 | 64 zones

#### Vl53l8.set_resolution

```cpp
void set_resolution(int zones);
```

vl53l8::RESOLUTION_4X4 | _8X8

#### Vl53l8.ranging_frequency_hz

```cpp
std::uint8_t ranging_frequency_hz();
```

Hz

#### Vl53l8.set_ranging_frequency_hz

```cpp
void set_ranging_frequency_hz(std::uint8_t hz);
```

>= 2 VL53L8, >= 1 L5/L7; max 60 (4x4) / 15 (8x8)

#### Vl53l8.ranging_mode

```cpp
std::uint8_t ranging_mode();
```

DEPZ_VL53L8_RANGING_MODE_*: 1 continuous, 3 autonomous

#### Vl53l8.set_ranging_mode

```cpp
void set_ranging_mode(std::uint8_t mode);
```

1 continuous, 3 autonomous

#### Vl53l8.integration_time_ms

```cpp
std::uint32_t integration_time_ms();
```

ms per frame in autonomous mode

#### Vl53l8.set_integration_time_ms

```cpp
void set_integration_time_ms(std::uint32_t ms);
```

2..1000, autonomous only

#### Vl53l8.sharpener_percent

```cpp
std::uint8_t sharpener_percent();
```

0 = off; read back rounded

#### Vl53l8.set_sharpener_percent

```cpp
void set_sharpener_percent(std::uint8_t pct);
```

0..99

#### Vl53l8.target_order

```cpp
std::uint8_t target_order();
```

1 closest, 2 strongest

#### Vl53l8.set_target_order

```cpp
void set_target_order(std::uint8_t order);
```

1 closest, 2 strongest

#### Vl53l8.power_mode

```cpp
std::uint8_t power_mode();
```

0 sleep, 1 wakeup, 2 deep sleep

#### Vl53l8.set_power_mode

```cpp
void set_power_mode(std::uint8_t mode);
```

no deep sleep on L5CX/L7CX; waking from it re-runs init()

#### Vl53l8.xtalk_margin

```cpp
double xtalk_margin();
```

kcps/SPAD

#### Vl53l8.set_xtalk_margin

```cpp
void set_xtalk_margin(double kcps_per_spad);
```

<= 10000

#### Vl53l8.calibrate_xtalk

```cpp
bool calibrate_xtalk(std::uint8_t reflectance_percent, std::uint8_t nb_samples, std::uint16_t distance_mm);
```

Crosstalk calibration (reflectance 1..99 %, samples 1..16, 600..3000 mm);
returns false when the firmware found nothing to calibrate (no cover glass).

#### Vl53l8.caldata_xtalk

```cpp
bytes caldata_xtalk();
```

the 776-byte blob, to save

#### Vl53l8.set_caldata_xtalk

```cpp
void set_caldata_xtalk(byte_span blob);
```

restore a saved blob (776 bytes)

#### Vl53l8.detection_thresholds_enabled

```cpp
bool detection_thresholds_enabled();
```

Detection thresholds: with them enabled, INT (and so the stream) only
fires for frames that meet them.

#### Vl53l8.set_detection_thresholds_enabled

```cpp
void set_detection_thresholds_enabled(bool enabled);
```

#### Vl53l8.detection_thresholds

```cpp
std::vector<vl53l8::DetectionThreshold> detection_thresholds();
```

all 64

#### Vl53l8.set_detection_thresholds

```cpp
void set_detection_thresholds(const std::vector<vl53l8::DetectionThreshold>& thresholds);
```

Up to 64, low/high in real units; bit 7 of zone_num marks the last one.

#### Vl53l8.set_detection_thresholds_auto_stop

```cpp
void set_detection_thresholds_auto_stop(bool auto_stop);
```

stop ranging on a hit; not on L5CX/L7CX

#### Vl53l8.configure_motion_indicator

```cpp
void configure_motion_indicator(std::uint16_t min_mm = 400, std::uint16_t max_mm = 1500);
```

Motion indicator over [min_mm, max_mm] (400..4000, span <= 1500) for the
current resolution; frames then carry `motion`.

#### Vl53l8.configure_cnh

```cpp
void configure_cnh(const CnhSetup& setup);
```

L8CH / L7CH only (WrongTypeError otherwise): arm CNH for the next
start_ranging(); frames then carry frame.cnh_raw. init() disarms it.

#### Vl53l8.start_ranging

```cpp
void start_ranging();
```

start the sensor and the MCU's frame stream

#### Vl53l8.stop_ranging

```cpp
void stop_ranging();
```

idempotent

#### Vl53l8.on_frame

```cpp
Unsubscribe on_frame(std::function<void(const Vl53l8LiveFrame&)> cb);
```

Every streamed frame, on the reader thread.

#### Vl53l8.frames

```cpp
Stream<Vl53l8LiveFrame> frames(std::size_t maxsize = 8);
```

A pull stream of frames; a frame is several KB, so keep maxsize small.

#### Vl53l8.get_frame

```cpp
Vl53l8LiveFrame get_frame(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
```

The next streamed frame; TimeoutError / DeviceLostError.

#### Vl53l8.frame_parse_errors

```cpp
std::uint64_t frame_parse_errors() const;
```

frames that did not decode

#### Vl53l8.reassembler_discards

```cpp
std::uint64_t reassembler_discards() const;
```

chunked frames lost on a gap

#### Vl53l8.read_reg

```cpp
bytes read_reg(std::uint16_t addr, std::size_t len);
```

Escape hatches: raw registers and DCI indices.

#### Vl53l8.write_reg

```cpp
void write_reg(std::uint16_t addr, byte_span data);
```

#### Vl53l8.dci_read

```cpp
bytes dci_read(std::uint16_t index, std::size_t len);
```

#### Vl53l8.dci_write

```cpp
void dci_write(std::uint16_t index, byte_span data);
```

#### Vl53l8.module_type

```cpp
std::optional<int> module_type() const;
```

L5/L7: the module type read at init(): 0 MZ = VL53L5CX, 1 MZEVO =
VL53L7CX/CH (never CX vs CH); nullopt before init and on a VL53L8.

#### Vl53l8.bridge_info

```cpp
vl53l7::Vl53l7Info bridge_info();
```

L5/L7 only (WrongTypeError on a VL53L8), like the next two: the bridge
counters; read before / after a run, not during one.

#### Vl53l8.set_i2c_speed_khz

```cpp
std::uint16_t set_i2c_speed_khz(std::uint16_t khz);
```

L5/L7 only. SCL snaps to 100, 200, 400, 500 ... 1000 kHz; returns the
value in effect.

#### Vl53l8.pin_ctrl

```cpp
void pin_ctrl(vl53l7::PinAction action);
```

L5/L7 only. LpnOff / SoftCycle drop the sensor's state: init() again.

### Bno055Sample

```cpp
struct Bno055Sample {
    std::uint64_t timestamp_us;  // MCU µs: trigger (stream) / read completion
    std::uint8_t addr;
    bytes raw;
    bno055::Units units;
    std::optional<std::array<double, 3>> accel, mag, gyro, euler, linear_accel, gravity;
    std::optional<std::array<double, 4>> quaternion;  // w, x, y, z
    std::optional<double> temperature;
    std::optional<bno055::CalibStatus> calibration;
};
```

One decoded register block scaled by the units in effect; a channel is
nullopt when the block did not cover it.

#### Bno055Sample.pull

```cpp
static std::optional<Bno055Sample> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Bno055Sample> plumbing.

### Bno055Status

```cpp
struct Bno055Status {
    std::uint8_t self_test, clk_status, status, error;
};
```

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.

### Bno055

```cpp
class Bno055 : public Device {
    // (no public data members)
};
```

#### Bno055.open

```cpp
static std::unique_ptr<Bno055> open(Link link);
```

A BNO055 on a link without an identity probe (tests, replay).

#### Bno055.bridge_info

```cpp
bno055::Bno055Info bridge_info();
```

safe while streaming

#### Bno055.is_alive

```cpp
bool is_alive();
```

#### Bno055.reset_sensor

```cpp
void reset_sensor();
```

nRESET: stops any stream; CONFIG mode, power-on units — configure again.

#### Bno055.read_registers

```cpp
bytes read_registers(std::uint8_t addr, std::size_t len, int page = 0);
```

#### Bno055.write_registers

```cpp
void write_registers(std::uint8_t addr, byte_span data, int page = 0);
```

#### Bno055.operation_mode

```cpp
bno055::OprMode operation_mode();
```

#### Bno055.set_operation_mode

```cpp
void set_operation_mode(bno055::OprMode mode);
```

Through CONFIG; into a fusion mode it waits for the fusion to run.

#### Bno055.power_mode

```cpp
std::uint8_t power_mode();
```

0 normal, 1 low power, 2 suspend

#### Bno055.set_power_mode

```cpp
void set_power_mode(std::uint8_t mode);
```

#### Bno055.units

```cpp
bno055::Units units();
```

#### Bno055.set_units

```cpp
void set_units(const bno055::Units& units);
```

#### Bno055.axis_remap

```cpp
bno055::AxisRemap axis_remap();
```

#### Bno055.set_axis_remap

```cpp
void set_axis_remap(const bno055::AxisRemap& remap);
```

#### Bno055.set_axis_placement

```cpp
void set_axis_placement(const std::string& placement);
```

"P0".."P7"

#### Bno055.temperature_source

```cpp
std::uint8_t temperature_source();
```

0 accel, 1 gyro

#### Bno055.set_temperature_source

```cpp
void set_temperature_source(std::uint8_t source);
```

#### Bno055.configure

```cpp
void configure(bno055::OprMode mode = bno055::OprMode::Ndof, const bno055::Units& units = {}, const std::optional<bno055::AxisRemap>& remap = std::nullopt, const std::optional<bno055::CalibrationProfile>& calibration = std::nullopt);
```

CONFIG -> units -> [remap] -> [calibration] -> mode; remembered.

#### Bno055.restore_configuration

```cpp
void restore_configuration();
```

#### Bno055.system_status

```cpp
Bno055Status system_status();
```

#### Bno055.self_test

```cpp
Bno055Status self_test();
```

~0.45 s, not while streaming

#### Bno055.calibration_status

```cpp
bno055::CalibStatus calibration_status();
```

#### Bno055.calibration_profile

```cpp
bno055::CalibrationProfile calibration_profile();
```

via CONFIG

#### Bno055.write_calibration_profile

```cpp
void write_calibration_profile(const bno055::CalibrationProfile& p);
```

#### Bno055.sic_matrix

```cpp
std::array<std::int16_t, 9> sic_matrix();
```

#### Bno055.set_sic_matrix

```cpp
void set_sic_matrix(const std::array<std::int16_t, 9>& m);
```

#### Bno055.accel_config

```cpp
bno055::AccelConfig accel_config();
```

#### Bno055.set_accel_config

```cpp
void set_accel_config(const bno055::AccelConfig& c);
```

#### Bno055.gyro_config

```cpp
bno055::GyroConfig gyro_config();
```

#### Bno055.set_gyro_config

```cpp
void set_gyro_config(const bno055::GyroConfig& c);
```

#### Bno055.mag_config

```cpp
bno055::MagConfig mag_config();
```

#### Bno055.set_mag_config

```cpp
void set_mag_config(const bno055::MagConfig& c);
```

#### Bno055.unique_id

```cpp
bytes unique_id();
```

16 bytes

#### Bno055.interrupt_enable

```cpp
std::uint8_t interrupt_enable();
```

#### Bno055.set_interrupt_enable

```cpp
void set_interrupt_enable(std::uint8_t mask);
```

#### Bno055.interrupt_mask

```cpp
std::uint8_t interrupt_mask();
```

#### Bno055.set_interrupt_mask

```cpp
void set_interrupt_mask(std::uint8_t mask);
```

#### Bno055.set_interrupt_setting

```cpp
void set_interrupt_setting(std::uint8_t reg, std::uint8_t value);
```

0x11..0x1F

#### Bno055.read_interrupt_status

```cpp
std::uint8_t read_interrupt_status();
```

clears on read

#### Bno055.clear_interrupt

```cpp
void clear_interrupt();
```

#### Bno055.read_sample

```cpp
Bno055Sample read_sample(std::uint8_t addr = bno055::FULL_BLOCK_ADDR, std::uint8_t len = bno055::FULL_BLOCK_LEN);
```

#### Bno055.read_quaternion

```cpp
std::array<double, 4> read_quaternion();
```

#### Bno055.start_stream

```cpp
void start_stream(std::uint16_t period_ms = 10, std::uint8_t addr = bno055::FULL_BLOCK_ADDR, std::uint8_t len = bno055::FULL_BLOCK_LEN, std::uint8_t trigger = bno055::TRIGGER_TIMER);
```

#### Bno055.stop_stream

```cpp
void stop_stream();
```

#### Bno055.streaming

```cpp
bool streaming() const;
```

#### Bno055.on_sample

```cpp
Unsubscribe on_sample(std::function<void(const Bno055Sample&)> cb);
```

#### Bno055.samples

```cpp
Stream<Bno055Sample> samples(std::size_t maxsize = 256);
```

#### Bno055.get_sample

```cpp
Bno055Sample get_sample(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
```

#### Bno055.stream_parse_errors

```cpp
std::uint64_t stream_parse_errors() const;
```

### Vl53lxCap

```cpp
enum class Vl53lxCap : unsigned {
    Mode = 0x0001, Timing = 0x0002, Offset = 0x0004, Xtalk = 0x0008, CalibOffset = 0x0010,
    CalibXtalk = 0x0020, Thresholds = 0x0040, SignalThresh = 0x0080, SigmaThresh = 0x0100,
    Roi = 0x0200, TempUpdate = 0x0400, RefSpad = 0x0800,
};
```

### Vl53lxTarget

```cpp
struct Vl53lxTarget {
    std::int32_t distance_mm = 0;
    int status = 0;
    std::string status_text;
    double signal_kcps = 0, ambient_kcps = 0, sigma_mm = 0;
    std::int32_t min_range_mm = 0, max_range_mm = 0;
};
```

One return of a histogram frame; min / max_range_mm bound its own pulse.

### Vl53lxBins

```cpp
struct Vl53lxBins {
    std::vector<std::int32_t> bin_data;  // number_of_bins photon counts
    std::uint8_t vcsel_period = 0;  // the register value
    std::uint8_t stream_count = 0;
    std::uint8_t range_status = 0;
    std::uint32_t dss_actual_effective_spads = 0;
    std::int32_t ambient_per_bin = 0;
    std::int32_t zero_distance_phase = 0;
};
```

The histogram frame the targets came from (after the driver read it).

### Vl53lxMeasurement

```cpp
struct Vl53lxMeasurement {
    std::uint64_t timestamp_us = 0;  // MCU µs at the INT edge / the read
    std::int32_t distance_mm = 0;
    int status = 0;
    std::string status_text;
    double signal_kcps = 0, ambient_kcps = 0, sigma_mm = 0, spads = 0;
    std::vector<Vl53lxTarget> targets;
    std::optional<std::int32_t> stream_count;
    std::optional<double> signal_per_spad_kcps, ambient_per_spad_kcps;  // die ULDs
    std::optional<std::int32_t> dmax_mm, device_range_status;  // VL53L0X
    std::optional<std::int32_t> min_range_mm, max_range_mm, peak_bin;  // histogram
    std::optional<Vl53lxBins> bins;
};
```

One ranging result, the same shape for every product. Histogram driver:
`targets` holds every return (the top level repeats targets[0]) and `bins`
the frame; light drivers leave both empty. Extras a driver lacks are nullopt.

#### Vl53lxMeasurement.plottable

```cpp
bool plottable() const;
```

Status 0, 6 (no wrap check yet) or 11 (merged target): a real distance.

#### Vl53lxMeasurement.primary_distance_mm

```cpp
std::optional<std::int32_t> primary_distance_mm() const;
```

The first plottable target, or the single distance of a light driver.

#### Vl53lxMeasurement.pull

```cpp
static std::optional<Vl53lxMeasurement> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Vl53lxMeasurement> plumbing.

### Vl53lx

```cpp
class Vl53lx : public Device {
    // (no public data members)
};
```

#### Vl53lx.open

```cpp
static std::unique_ptr<Vl53lx> open(Link link);
```

A 1D-family board on a link without an identity probe (tests, replay).

#### Vl53lx.detected_product

```cpp
std::optional<std::string> detected_product();
```

The product the board's device name carries ("VL53L4CX"), nullopt on
an unstamped board (then name it at init).

#### Vl53lx.driver_kinds

```cpp
static std::vector<vl53lx::DriverKind> driver_kinds(const std::string& product);
```

The driver kinds a product has (UI order).

#### Vl53lx.init

```cpp
void init(std::optional<vl53lx::DriverKind> driver = std::nullopt, const std::optional<std::string>& product = std::nullopt);
```

Bind (product, driver) and initialise; defaults: the detected product,
its first kind (uld, ulp, histogram).

#### Vl53lx.initialized

```cpp
bool initialized() const;
```

#### Vl53lx.product

```cpp
std::optional<std::string> product() const;
```

#### Vl53lx.driver_kind

```cpp
std::optional<vl53lx::DriverKind> driver_kind() const;
```

#### Vl53lx.caveat

```cpp
std::string caveat() const;
```

"" when none

#### Vl53lx.driver_reach_mm

```cpp
std::uint32_t driver_reach_mm() const;
```

0 = not characterised

#### Vl53lx.supports

```cpp
bool supports(Vl53lxCap cap) const;
```

#### Vl53lx.model_id

```cpp
std::uint16_t model_id();
```

cross-check only

#### Vl53lx.modes

```cpp
std::vector<std::string> modes() const;
```

UI order; mode() = the one in use

#### Vl53lx.budget_range

```cpp
std::pair<int, int> budget_range() const;
```

inclusive, ms

#### Vl53lx.budget_choices

```cpp
std::vector<int> budget_choices();
```

empty: any integer in range

#### Vl53lx.xshut

```cpp
void xshut(std::uint8_t action);
```

XSHUT_OFF / _ON / _RESET: init again

#### Vl53lx.bridge_info

```cpp
vl53lx::Vl53lxInfo bridge_info();
```

safe while streaming

#### Vl53lx.configure

```cpp
void configure(int budget_ms = 50, int inter_ms = 0, const std::optional<std::string>& mode = std::nullopt, std::optional<std::int32_t> offset_mm = std::nullopt, std::optional<std::int32_t> xtalk_kcps = std::nullopt, std::optional<int> signal_kcps = std::nullopt);
```

Re-initialise and apply budget / mode (and a stored calibration).
signal_kcps replaces the blob's signal threshold, last — the re-init
puts it back to the default (depz_vl53lx_configure_ex).

#### Vl53lx.range_timing

```cpp
std::pair<int, int> range_timing();
```

(budget ms, inter-measurement ms)

#### Vl53lx.set_mode

```cpp
void set_mode(const std::string& mode);
```

#### Vl53lx.mode

```cpp
std::optional<std::string> mode();
```

nullopt on a product without modes

#### Vl53lx.offset_mm

```cpp
std::int32_t offset_mm();
```

#### Vl53lx.set_offset_mm

```cpp
void set_offset_mm(std::int32_t mm);
```

#### Vl53lx.xtalk_kcps

```cpp
std::int32_t xtalk_kcps();
```

#### Vl53lx.set_xtalk_kcps

```cpp
void set_xtalk_kcps(std::int32_t kcps);
```

#### Vl53lx.calibrate_offset

```cpp
std::int32_t calibrate_offset(int target_mm, int nb_samples = 0);
```

Against a flat target; nb_samples 0 = the driver's default. Store the result.

#### Vl53lx.calibrate_xtalk

```cpp
std::int32_t calibrate_xtalk(int target_mm, int nb_samples = 0);
```

#### Vl53lx.detection_thresholds

```cpp
std::array<int, 3> detection_thresholds();
```

low mm, high mm, window

#### Vl53lx.set_detection_thresholds

```cpp
void set_detection_thresholds(int low_mm, int high_mm, int window);
```

#### Vl53lx.signal_threshold_kcps

```cpp
int signal_threshold_kcps();
```

#### Vl53lx.set_signal_threshold_kcps

```cpp
void set_signal_threshold_kcps(int kcps);
```

#### Vl53lx.sigma_threshold_mm

```cpp
int sigma_threshold_mm();
```

#### Vl53lx.set_sigma_threshold_mm

```cpp
void set_sigma_threshold_mm(int mm);
```

#### Vl53lx.roi

```cpp
std::pair<int, int> roi();
```

#### Vl53lx.set_roi

```cpp
void set_roi(int x, int y);
```

#### Vl53lx.roi_center

```cpp
int roi_center();
```

#### Vl53lx.set_roi_center

```cpp
void set_roi_center(int spad);
```

#### Vl53lx.start_temperature_update

```cpp
void start_temperature_update();
```

#### Vl53lx.perform_ref_spad_management

```cpp
std::pair<std::uint32_t, bool> perform_ref_spad_management();
```

VL53L0X

#### Vl53lx.start_ranging

```cpp
void start_ranging();
```

#### Vl53lx.stop_ranging

```cpp
void stop_ranging();
```

#### Vl53lx.ranging

```cpp
bool ranging() const;
```

#### Vl53lx.measure_once

```cpp
Vl53lxMeasurement measure_once(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
```

#### Vl53lx.on_measurement

```cpp
Unsubscribe on_measurement(std::function<void(const Vl53lxMeasurement&)> cb);
```

#### Vl53lx.measurements

```cpp
Stream<Vl53lxMeasurement> measurements(std::size_t maxsize = 64);
```

#### Vl53lx.get_measurement

```cpp
Vl53lxMeasurement get_measurement(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
```

#### Vl53lx.stream_parse_errors

```cpp
std::uint64_t stream_parse_errors() const;
```

#### Vl53lx.read_reg

```cpp
bytes read_reg(std::uint16_t addr, std::size_t len);
```

#### Vl53lx.write_reg

```cpp
void write_reg(std::uint16_t addr, byte_span data);
```

### SensorId

```cpp
enum class SensorId : std::uint8_t {
    Accelerometer = 0x01, Gyroscope = 0x02, Magnetometer = 0x03, LinearAcceleration = 0x04,
    RotationVector = 0x05, Gravity = 0x06, UncalibratedGyroscope = 0x07, GameRotationVector = 0x08,
    GeomagneticRotationVector = 0x09, UncalibratedMagnetometer = 0x0F, TapDetector = 0x10,
    StepCounter = 0x11, SignificantMotion = 0x12, StabilityClassifier = 0x13, RawAccelerometer = 0x14,
    RawGyroscope = 0x15, RawMagnetometer = 0x16, StepDetector = 0x18, ShakeDetector = 0x19,
    FlipDetector = 0x1A, PickupDetector = 0x1B, StabilityDetector = 0x1C,
    PersonalActivityClassifier = 0x1E, SleepDetector = 0x1F, TiltDetector = 0x20, PocketDetector = 0x21,
    CircleDetector = 0x22, HeartRateMonitor = 0x23, ArvrStabilizedRv = 0x28,
    ArvrStabilizedGameRv = 0x29, GyroIntegratedRv = 0x2A,
};
```

SH-2 sensor ids — also a report's `sensor_id`.

### TareBasis

```cpp
enum class TareBasis : std::uint8_t {
    RotationVector = 0, GameRotationVector = 1, GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3, ArvrStabilizedRv = 4, ArvrStabilizedGameRv = 5,
};
```

### TARE_X

```cpp
inline constexpr std::uint8_t TARE_X = 1, TARE_Y = 2, TARE_Z = 4, TARE_ALL = 7;
```

### Feature

```cpp
struct Feature {
    std::uint8_t sensor_id = 0, flags = 0;
    std::uint16_t sensitivity = 0;
    std::uint32_t interval_us = 0;  // granted interval; 0 = disabled
    std::uint32_t batch_us = 0;
    std::uint32_t cfg_word = 0;
};
```

Get Feature Response: the rates in effect.

### FeatureRequest

```cpp
struct FeatureRequest {
    std::uint32_t interval_us = 0;
    std::uint32_t batch_us = 0;
    std::uint16_t sensitivity = 0;
    std::uint8_t flags = 0;
    std::uint32_t cfg_word = 0;
};
```

Everything Set Feature carries.

### ProductId

```cpp
struct ProductId {
    std::uint8_t reset_cause = 0, sw_version_major = 0, sw_version_minor = 0;
    std::uint32_t sw_part_number = 0;  // 10004148 = BNO085, 10004563 = BNO086
    std::uint32_t sw_build_number = 0;
    std::uint16_t sw_version_patch = 0;
};
```

### CalibrationConfig

```cpp
struct CalibrationConfig {
    bool accel = false, gyro = false, mag = false, planar = false;
};
```

### ErrorRecord

```cpp
struct ErrorRecord {
    std::uint8_t severity, seq, source, error, module, code;
};
```

### Counts

```cpp
struct Counts {
    std::uint8_t sensor_id;
    std::uint32_t offered, accepted, on, attempted;
};
```

### CommandResponse

```cpp
struct CommandResponse {
    std::uint8_t seq = 0, command = 0, command_seq = 0, response_seq = 0;
    std::array<std::uint8_t, 11> r{};  // r[0]: status for most commands
};
```

### SensorMetadata

```cpp
struct SensorMetadata {
    std::uint8_t me_version = 0, mh_version = 0, sh_version = 0;
    std::uint32_t range_raw = 0, resolution_raw = 0;
    std::uint16_t revision = 0;
    std::uint16_t power_ma_q10 = 0;  // mA, Q10
    std::uint32_t min_period_us = 0, max_period_us = 0;
    std::uint16_t fifo_max = 0, fifo_reserved = 0, batch_buffer_bytes = 0;
    std::uint16_t q_point_1 = 0, q_point_2 = 0, q_point_3 = 0;
};
```

A sensor's metadata FRS record; revision-gated fields are 0 when older.

#### SensorMetadata.power_ma

```cpp
double power_ma() const;
```

### q_point

```cpp
std::optional<int> q_point(std::uint8_t sensor_id);
```

Q point of a sensor's primary fields; nullopt for event reports.

### Bno086Report

```cpp
struct Bno086Report : bno086::Report {
    // (no public data members)
};
```

A decoded report (codec fields, raw integers authoritative) plus scaling.

#### Bno086Report.xyz

```cpp
std::array<double, 3> xyz() const;
```

x, y, z in m/s², rad/s or µT (raw counts for the raw sensors).

#### Bno086Report.bias

```cpp
std::array<double, 3> bias() const;
```

uncalibrated gyro / mag

#### Bno086Report.quaternion

```cpp
std::array<double, 4> quaternion() const;
```

i, j, k, real

#### Bno086Report.accuracy_rad

```cpp
std::optional<double> accuracy_rad() const;
```

nullopt for game variants

#### Bno086Report.angular_velocity

```cpp
std::array<double, 3> angular_velocity() const;
```

gyro-integrated RV, rad/s

#### Bno086Report.scalar

```cpp
double scalar() const;
```

environment reports

#### Bno086Report.pull

```cpp
static std::optional<Bno086Report> pull(detail::StreamHandle& h, std::chrono::milliseconds timeout, bool& closed);
```

Stream<Bno086Report> plumbing.

### Bno086

```cpp
class Bno086 : public Device {
    // (no public data members)
};
```

#### Bno086.open

```cpp
static std::unique_ptr<Bno086> open(Link link);
```

A BNO085 / BNO086 on a link without an identity probe (tests, replay).

#### Bno086.hardware_reset

```cpp
void hardware_reset(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
```

nRST: SHTP state, the advertisement and every enabled sensor restart.

#### Bno086.wake

```cpp
void wake();
```

#### Bno086.advertisement

```cpp
bytes advertisement();
```

#### Bno086.product_id

```cpp
bno086::ProductId product_id();
```

#### Bno086.enable

```cpp
bno086::Feature enable(bno086::SensorId sensor, double hz);
```

Set Feature, then the granted rate read back (Get Feature).

#### Bno086.enable

```cpp
std::optional<bno086::Feature> enable(bno086::SensorId sensor, const bno086::FeatureRequest& req, bool verify = true);
```

No read-back when `verify` is false (then the result is nullopt).

#### Bno086.rate_ok

```cpp
static bool rate_ok(std::uint32_t requested_interval_us, const bno086::Feature& granted);
```

Within [0.9, 2.1] x the requested rate (contract 05 §7)?

#### Bno086.disable

```cpp
void disable(bno086::SensorId sensor);
```

#### Bno086.feature

```cpp
bno086::Feature feature(bno086::SensorId sensor);
```

#### Bno086.on_report

```cpp
Unsubscribe on_report(std::function<void(const Bno086Report&)> cb);
```

#### Bno086.reports

```cpp
Stream<Bno086Report> reports(std::size_t maxsize = 1024);
```

#### Bno086.get_report

```cpp
Bno086Report get_report(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
```

#### Bno086.shtp_discarded

```cpp
std::uint64_t shtp_discarded() const;
```

#### Bno086.tare_now

```cpp
void tare_now(std::uint8_t axes = bno086::TARE_ALL, bno086::TareBasis basis = bno086::TareBasis::RotationVector);
```

#### Bno086.persist_tare

```cpp
void persist_tare();
```

#### Bno086.set_reorientation

```cpp
void set_reorientation(double x, double y, double z, double w);
```

zeros clear

#### Bno086.set_calibration

```cpp
void set_calibration(bool accel = true, bool gyro = true, bool mag = true, bool planar = false);
```

#### Bno086.calibration

```cpp
bno086::CalibrationConfig calibration();
```

#### Bno086.save_dcd

```cpp
void save_dcd();
```

#### Bno086.configure_periodic_dcd

```cpp
void configure_periodic_dcd(bool enable);
```

#### Bno086.frs_read

```cpp
std::vector<std::uint32_t> frs_read(std::uint16_t record);
```

#### Bno086.frs_write

```cpp
void frs_write(std::uint16_t record, const std::vector<std::uint32_t>& words);
```

#### Bno086.metadata

```cpp
bno086::SensorMetadata metadata(bno086::SensorId sensor);
```

#### Bno086.oscillator_type

```cpp
std::uint8_t oscillator_type();
```

0 internal, 1 crystal, 2 external clock

#### Bno086.clear_dcd_and_reset

```cpp
void clear_dcd_and_reset(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
```

#### Bno086.errors

```cpp
std::vector<bno086::ErrorRecord> errors(std::uint8_t severity = 0);
```

#### Bno086.counts

```cpp
bno086::Counts counts(bno086::SensorId sensor);
```

#### Bno086.clear_counts

```cpp
void clear_counts(bno086::SensorId sensor);
```

#### Bno086.command

```cpp
std::optional<bno086::CommandResponse> command(std::uint8_t command, byte_span params = {}, bool wait_response = true);
```

Escape hatches: a Command Request (nullopt: no answer awaited), a raw cargo.

#### Bno086.send_shtp

```cpp
void send_shtp(std::uint8_t channel, byte_span payload);
```

### SerialPortInfo

```cpp
struct SerialPortInfo {
    std::string port;
    std::optional<std::uint16_t> vid, pid;  // nullopt: not a USB port
    std::string usb_serial;  // "" when unknown
};
```

### DeviceInfo

```cpp
struct DeviceInfo {
    std::string port;
    DeviceMode mode;
    std::optional<SensorType> sensor_type;  // nullopt in bootloader mode
    std::string software_name, fw_version, device_name;
    std::string serial_number;  // protocol serial (GET_SERIAL)
    std::optional<std::uint16_t> usb_vid, usb_pid;
    std::string usb_serial;
};
```

### list_serial_ports

```cpp
std::vector<SerialPortInfo> list_serial_ports();
```

Every serial port the OS lists, with its USB identity where known.

### probe_port

```cpp
std::optional<DeviceInfo> probe_port(const std::string& port, std::chrono::milliseconds timeout = std::chrono::milliseconds(200));
```

Probe one port; nullopt when nothing DEPZ-shaped answers.

### list_depz_devices

```cpp
std::vector<DeviceInfo> list_depz_devices(bool match_usb = true, std::chrono::milliseconds timeout = std::chrono::milliseconds(200));
```

Probe the candidates (with `match_usb`, only known DEPZ USB ids), by USB serial.

### OpenOptions

```cpp
struct OpenOptions {
    std::optional<std::string> port;  // exactly this port (even an unknown USB id)
    std::optional<std::string> serial;  // else the candidate with this USB serial
    std::optional<int> index;  // else the Nth candidate by USB serial
    std::chrono::milliseconds timeout{0};  // request timeout, 0 = default 200 ms
};
```

### open_device

```cpp
std::unique_ptr<Device> open_device(const OpenOptions& opt = {});
```

Find, probe and open with the right class: dynamic_cast<Sr04*> (or
<Vl53l4cd*>, <Vl53l8*>, <Bno055*>, <Bno086*>, <Vl53lx*>) tells.

### open_sr04

```cpp
std::unique_ptr<Sr04> open_sr04(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is an SR04.

### open_vl53l4cd

```cpp
std::unique_ptr<Vl53l4cd> open_vl53l4cd(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is a VL53L4CD.

### open_vl53l8

```cpp
std::unique_ptr<Vl53l8> open_vl53l8(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is a multizone ToF. The model
comes from the USB PID (0xED40 -> L8CH, else L8CX on APP_VL53L8; 0xED48 /
0xED49 / 0xED4A -> L5CX / L7CX / L7CH, else the device name, else L7CX on
APP_VL53L7).

### open_bno055

```cpp
std::unique_ptr<Bno055> open_bno055(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is a BNO055.

### open_bno086

```cpp
std::unique_ptr<Bno086> open_bno086(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is a BNO085 / BNO086.

### open_vl53lx

```cpp
std::unique_ptr<Vl53lx> open_vl53lx(const OpenOptions& opt = {});
```

open_device, and WrongTypeError unless it is a VL53L 1D-family board.
