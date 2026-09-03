# API reference

Auto-generated from the crate's public surface (the `pub` items and
their `///` doc-comments across `src/**`) by `scripts/gen_api_md.py`.
Regenerate with `python3 scripts/gen_api_md.py` from the crate root.
Edit the doc-comments in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [BNO086](bno086/api.md).

## Contents

- **Discovery**: [`SensorType`](#sensortype), [`Mode`](#mode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name), [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`DEV_USB_VID`](#dev_usb_vid), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortEntry`](#portentry), [`order_ports`](#order_ports)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L4CD (ToF)**: [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`I2C_KHZ_STEPS`](#i2c_khz_steps), [`i2c_error_name`](#i2c_error_name), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`Vl53l4Info`](#vl53l4info), [`StreamData`](#streamdata), [`SOFT_RESET`](#soft_reset), [`I2C_SLAVE__DEVICE_ADDRESS`](#i2c_slave__device_address), [`OSC_FREQUENCY`](#osc_frequency), [`VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`](#vhv_config__timeout_macrop_loop_bound), [`XTALK_PLANE_OFFSET_KCPS`](#xtalk_plane_offset_kcps), [`XTALK_X_PLANE_GRADIENT_KCPS`](#xtalk_x_plane_gradient_kcps), [`XTALK_Y_PLANE_GRADIENT_KCPS`](#xtalk_y_plane_gradient_kcps), [`RANGE_OFFSET_MM`](#range_offset_mm), [`INNER_OFFSET_MM`](#inner_offset_mm), [`OUTER_OFFSET_MM`](#outer_offset_mm), [`GPIO_HV_MUX__CTRL`](#gpio_hv_mux__ctrl), [`GPIO__TIO_HV_STATUS`](#gpio__tio_hv_status), [`SYSTEM__INTERRUPT`](#system__interrupt), [`RANGE_CONFIG_A`](#range_config_a), [`RANGE_CONFIG_B`](#range_config_b), [`RANGE_CONFIG__SIGMA_THRESH`](#range_config__sigma_thresh), [`MIN_COUNT_RATE_RTN_LIMIT_MCPS`](#min_count_rate_rtn_limit_mcps), [`INTERMEASUREMENT_MS`](#intermeasurement_ms), [`THRESH_HIGH`](#thresh_high), [`THRESH_LOW`](#thresh_low), [`SYSTEM__INTERRUPT_CLEAR`](#system__interrupt_clear), [`SYSTEM_START`](#system_start), [`RESULT__RANGE_STATUS`](#result__range_status), [`RESULT__SPAD_NB`](#result__spad_nb), [`RESULT__SIGNAL_RATE`](#result__signal_rate), [`RESULT__AMBIENT_RATE`](#result__ambient_rate), [`RESULT__SIGMA`](#result__sigma), [`RESULT__DISTANCE`](#result__distance), [`RESULT__OSC_CALIBRATE_VAL`](#result__osc_calibrate_val), [`FIRMWARE__SYSTEM_STATUS`](#firmware__system_status), [`IDENTIFICATION__MODEL_ID`](#identification__model_id), [`MODEL_ID_VL53L4CD`](#model_id_vl53l4cd), [`WINDOW_BELOW`](#window_below), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`CONFIG_ADDR`](#config_addr), [`CONFIG_END`](#config_end), [`DEFAULT_CONFIGURATION`](#default_configuration), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`config_block`](#config_block), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`I2C_KHZ_BOOT`](#i2c_khz_boot), [`I2C_KHZ_DEFAULT`](#i2c_khz_default), [`STATUS_RTN`](#status_rtn), [`range_status_name`](#range_status_name), [`Vl53l4Error`](#vl53l4error), [`Vl53l4Results`](#vl53l4results), [`parse_result_block`](#parse_result_block), [`range_timing_registers`](#range_timing_registers), [`decode_range_timing`](#decode_range_timing), [`offset_raw`](#offset_raw), [`decode_offset`](#decode_offset), [`xtalk_raw`](#xtalk_raw), [`decode_xtalk`](#decode_xtalk), [`signal_threshold_raw`](#signal_threshold_raw), [`decode_signal_threshold`](#decode_signal_threshold), [`sigma_threshold_raw`](#sigma_threshold_raw), [`decode_sigma_threshold`](#decode_sigma_threshold)
- **VL53L8 (ToF)**: [`DIST_MM`](#dist_mm), [`SIGNAL_PER_SPAD_KCPS`](#signal_per_spad_kcps), [`RANGE_SIGMA_MM`](#range_sigma_mm), [`AMBIENT_PER_SPAD_KCPS`](#ambient_per_spad_kcps), [`NB_TARGET_DETECTED`](#nb_target_detected), [`TAR_STATUS`](#tar_status), [`NB_SPADS_ENABLED`](#nb_spads_enabled), [`MOTION_INDICATOR`](#motion_indicator), [`NB_THRESHOLDS`](#nb_thresholds), [`POWER_MODE_SLEEP`](#power_mode_sleep), [`POWER_MODE_WAKEUP`](#power_mode_wakeup), [`POWER_MODE_DEEP_SLEEP`](#power_mode_deep_sleep), [`xtalk_margin_to_raw`](#xtalk_margin_to_raw), [`MotionConfig`](#motionconfig), [`default_motion_config`](#default_motion_config), [`DetectionThreshold`](#detectionthreshold), [`DetectionThresholdBlocks`](#detectionthresholdblocks), [`pack_detection_thresholds`](#pack_detection_thresholds), [`CnhDecodeConfig`](#cnhdecodeconfig), [`CnhAggregate`](#cnhaggregate), [`CnhData`](#cnhdata), [`CnhError`](#cnherror), [`decode_cnh`](#decode_cnh), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`NB_TARGET_PER_ZONE`](#nb_target_per_zone), [`CNH_DATA_IDX`](#cnh_data_idx), [`Variant`](#variant), [`Vl53l8Error`](#vl53l8error), [`Vl53l8Results`](#vl53l8results), [`swap_buffer`](#swap_buffer), [`parse_frame`](#parse_frame), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`STREAM_TOTAL_MAX`](#stream_total_max), [`FrameChunk`](#framechunk), [`unpack_frame_chunk`](#unpack_frame_chunk), [`CompletedFrame`](#completedframe), [`FrameReassembler`](#framereassembler)
- **BNO086 (IMU)**: [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`q_point`](#q_point), [`report_length`](#report_length), [`Report`](#report), [`Vector3Kind`](#vector3kind), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo), [`REPORT_COMMAND_RESPONSE`](#report_command_response), [`REPORT_COMMAND_REQUEST`](#report_command_request), [`REPORT_FRS_READ_RESPONSE`](#report_frs_read_response), [`REPORT_FRS_READ_REQUEST`](#report_frs_read_request), [`REPORT_FRS_WRITE_RESPONSE`](#report_frs_write_response), [`REPORT_FRS_WRITE_DATA`](#report_frs_write_data), [`REPORT_FRS_WRITE_REQUEST`](#report_frs_write_request), [`REPORT_PRODUCT_ID_RESPONSE`](#report_product_id_response), [`REPORT_PRODUCT_ID_REQUEST`](#report_product_id_request), [`REPORT_GET_FEATURE_RESPONSE`](#report_get_feature_response), [`REPORT_SET_FEATURE_COMMAND`](#report_set_feature_command), [`REPORT_GET_FEATURE_REQUEST`](#report_get_feature_request), [`Sh2Error`](#sh2error), [`build_set_feature`](#build_set_feature), [`build_get_feature_request`](#build_get_feature_request), [`build_product_id_request`](#build_product_id_request), [`build_command_request`](#build_command_request), [`build_frs_read_request`](#build_frs_read_request), [`build_frs_write_request`](#build_frs_write_request), [`build_frs_write_data`](#build_frs_write_data), [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`pack_shtp_header`](#pack_shtp_header), [`unpack_shtp_header`](#unpack_shtp_header), [`ShtpCargo`](#shtpcargo), [`build_frame`](#build_frame), [`ShtpLayer`](#shtplayer)
- **Bootloader / firmware update**: [`HEADER_SIZE`](#header_size), [`MAGIC`](#magic), [`FwdepzHeader`](#fwdepzheader), [`FwdepzError`](#fwdepzerror), [`parse`](#parse)
- **Datasets (record & replay)**: [`DatasetError`](#dataseterror), [`Device`](#device), [`RecordValue`](#recordvalue), [`Record`](#record), [`Dataset`](#dataset)
- **Transport**: [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false), [`MAGIC`](#magic), [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`CrcType`](#crctype), [`FramingError`](#framingerror), [`payload_crc_bytes`](#payload_crc_bytes), [`build_packet`](#build_packet), [`Packet`](#packet), [`Event`](#event), [`PacketParser`](#packetparser)
- **Protocol codecs**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`UNSOLICITED`](#unsolicited), [`CodecError`](#codecerror), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`pack_sync_time`](#pack_sync_time), [`pack_set_payload_crc_type`](#pack_set_payload_crc_type), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string)
- **JSON**: [`Value`](#value), [`JsonError`](#jsonerror), [`object_map`](#object_map)

## Discovery

### SensorType

```rust
pub enum SensorType {
    Sr04,
    Vl53l4,
    Vl53l8,
    Bno086,
    Unknown,
}
```

Product family classified from the active-software name.

#### SensorType::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase wire-string form (matches the golden vectors).

### Mode

```rust
pub enum Mode {
    App,
    Bootloader,
    Unknown,
}
```

Firmware operating mode.

#### Mode::as_str

```rust
pub fn as_str(&self) -> &'static str
```

### Identity

```rust
pub struct Identity {
    pub mode: Mode,
    /// `None` in bootloader/unknown mode.
    pub sensor_type: Option<SensorType>,
    pub software_name: String,
    /// `""` when not parseable.
    pub version: String,
}
```

Classified `GET_NAME_ACTIVE_SOFTWARE` result.

### parse_software_name

```rust
pub fn parse_software_name(name: &str) -> Identity
```

Classify an already-stripped active-software string.

### DEPZ_USB_VID

```rust
pub const DEPZ_USB_VID: u16 = 0x1BCF;
```

Production VID shared by every DEPZ sensor (0x1BCF).

### PID_SR04

```rust
pub const PID_SR04: u16 = 0xEC78; // 60536 — HC-SR04 ultrasonic
pub const PID_VL53L8CH: u16 = 0xED40; // 60736 — VL53L8CH ToF
pub const PID_VL53L4CD: u16 = 0xED45; // 60741 — VL53L4CD ToF
pub const PID_VL53L8CX: u16 = 0xED4B; // 60747 — VL53L8CX ToF (hw-verified)
pub const PID_BNO086: u16 = 0xEE08; // 60936 — BNO086 IMU

/// Inclusive PID range under `DEPZ_USB_VID` treated as candidate DEPZ sensors.
pub const DEPZ_PID_RANGE: (u16, u16) = (60536, 65535);
```

Per-model production PIDs (block 60536+, ascending).

### DEV_USB_VID

```rust
pub const DEV_USB_VID: u16 = 0x0483; // 1155
pub const DEV_USB_PID: u16 = 0x56DC; // 22236

/// PID → sensor-model hint. Informational: the protocol probe is authoritative.
const PID_MODEL: &[(u16, &str)] = &[
    (PID_SR04, "sr04"),
    (PID_VL53L8CH, "vl53l8ch"),
    (0xED41, "vl53l0x"),  // 60737
    (0xED42, "vl53l1cb"), // 60738
    (0xED43, "vl53l1cx"), // 60739
    (0xED44, "vl53l3cx"), // 60740
    (PID_VL53L4CD, "vl53l4cd"),
    (0xED46, "vl53l4cx"), // 60742
    (0xED47, "vl53l4ed"), // 60743
    (0xED48, "vl53l5cx"), // 60744
    (0xED49, "vl53l7cx"), // 60745
    (0xED4A, "vl53l7ch"), // 60746
    (PID_VL53L8CX, "vl53l8cx"),
    (PID_BNO086, "bno086"),
    (0xEE09, "bno085"), // 60937
    (0xEE0A, "bno055"), // 60938
];
```

Dev / unprogrammed default: STMicroelectronics VID/PID.

### is_known_depz_usb

```rust
pub fn is_known_depz_usb(vid: Option<u16>, pid: Option<u16>) -> bool
```

True when `(vid, pid)` is a recognized DEPZ (or dev-default) USB id.

### usb_model_hint

```rust
pub fn usb_model_hint(vid: Option<u16>, pid: Option<u16>) -> Option<&'static str>
```

Best-guess model name for a `(vid, pid)`, or `None`. Informational only.

### PortEntry

```rust
pub struct PortEntry {
    pub port: String,
    pub serial: Option<String>,
}
```

A candidate serial port for discovery ordering.

### order_ports

```rust
pub fn order_ports(mut ports: Vec<PortEntry>) -> Vec<PortEntry>
```

Order candidate ports by USB iSerial ascending; `None`/empty serials sort
last, tie-broken by port path (contract 02 §4).

## SR04

### Sr04Cmd

```rust
pub enum Sr04Cmd {
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
}
```

SR04 host→device command opcodes.

### Sr04Rpt

```rust
pub enum Sr04Rpt {
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
}
```

SR04 device→host report opcodes.

### ECHO_TIMEOUT

```rust
pub const ECHO_TIMEOUT: u16 = 0xFFFF;
```

`echo_time_us` sentinel: no echo received.

### SAMPLE_PERIOD_DEFAULT_US

```rust
pub const SAMPLE_PERIOD_DEFAULT_US: u32 = 50_000;
```

### ECHO_DECAY_DEFAULT_US

```rust
pub const ECHO_DECAY_DEFAULT_US: u16 = 5_000;
```

### ECHO_DECAY_MIN_US

```rust
pub const ECHO_DECAY_MIN_US: u16 = 4_000;
```

### ECHO_DECAY_MAX_US

```rust
pub const ECHO_DECAY_MAX_US: u16 = 65_000;
```

### Sr04Data

```rust
pub struct Sr04Data {
    /// 0x36 single shot (host or SYNC_IN), 0x37 loop sample (ERRATA E3).
    pub source_cmd: u8,
    pub timestamp_us: u64,
    pub echo_time_us: u16,
}
```

A decoded `RPT_DATA` sample.

#### Sr04Data::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Sr04Data, CodecError>
```

#### Sr04Data::is_timeout

```rust
pub fn is_timeout(&self) -> bool
```

`true` when the sample is the timeout sentinel (no echo).

### pack_sample_period

```rust
pub fn pack_sample_period(period_us: u32) -> [u8; 4]
```

### unpack_sample_period

```rust
pub fn unpack_sample_period(payload: &[u8]) -> Result<u32, CodecError>
```

### pack_echo_decay

```rust
pub fn pack_echo_decay(decay_us: u16) -> [u8; 2]
```

### unpack_echo_decay

```rust
pub fn unpack_echo_decay(payload: &[u8]) -> Result<u16, CodecError>
```

### distance_mm_from_echo

```rust
pub fn distance_mm_from_echo(echo_time_us: u16, air_temp_c: Option<f64>) -> Option<f64>
```

Round-trip echo time → distance in mm; `None` for the timeout sentinel.

Default speed of sound 343 m/s; with `air_temp_c` uses `c = 331.3 + 0.606·T`.

## VL53L4CD (ToF)

### Vl53l4Cmd

```rust
pub enum Vl53l4Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}
```

VL53L4 host→device command opcodes.

### Vl53l4Rpt

```rust
pub enum Vl53l4Rpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

VL53L4 device→host report opcodes.

### XFER_MAX

```rust
pub const XFER_MAX: usize = 253;
```

Largest `len` (read) / data length (write) per transfer. The STM32 I2C
NBYTES field is 8-bit and a write spends two bytes on the register address;
the firmware applies the same 253 to both directions.

### XSHUT_OFF

```rust
pub const XSHUT_OFF: u8 = 0;
```

`VL53_XSHUT` action: drive XSHUT low (sensor powered down).

### XSHUT_ON

```rust
pub const XSHUT_ON: u8 = 1;
```

`VL53_XSHUT` action: drive XSHUT high, no boot handshake.

### XSHUT_RESET

```rust
pub const XSHUT_RESET: u8 = 2;
```

`VL53_XSHUT` action: pulse low then poll the boot handshake (answered after
it completes — allow ≥ 1.5 s).

### SF_INT_ACT_HIGH

```rust
pub const SF_INT_ACT_HIGH: u8 = 0x02;
```

`VL53_START_STREAM` flag: INT active high, mirroring bit 4 of
`GPIO_HV_MUX__CTRL` (0x0030). Clear (default): INT active low.

### I2C_KHZ_STEPS

```rust
pub const I2C_KHZ_STEPS: [u16; 9] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];
```

Nominal SCL steps the firmware carries a TIMINGR for (`VL53_SET_I2C_SPEED`
clamps to the nearest one).

### i2c_error_name

```rust
pub fn i2c_error_name(code: u8) -> &'static str
```

`last_i2c_error` code in [`Vl53l4Info`] → human-readable name.

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u16, len: u16) -> [u8; 4]
```

`VL53_READ_REG` payload: `addr u16, len u16` (little-endian).

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Vec<u8>
```

`VL53_WRITE_REG` payload: `addr u16` followed by the raw register bytes.

### pack_xshut

```rust
pub fn pack_xshut(action: u8) -> [u8; 1]
```

`VL53_XSHUT` payload: `action u8` ([`XSHUT_OFF`] / [`XSHUT_ON`] /
[`XSHUT_RESET`]).

### pack_start_stream

```rust
pub fn pack_start_stream(addr: u16, len: u16, flags: u8) -> [u8; 5]
```

`VL53_START_STREAM` payload: `addr u16, len u16, flags u8`.

### pack_set_i2c_speed

```rust
pub fn pack_set_i2c_speed(khz: u16) -> [u8; 2]
```

`VL53_SET_I2C_SPEED` payload: `khz u16`.

### RegData

```rust
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
    /// MCU uptime at I2C-read completion.
    pub timestamp_us: u64,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}
```

A decoded `RPT_VL53_REG_DATA` report (one register read).

#### RegData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError>
```

### Vl53l4Info

```rust
pub struct Vl53l4Info {
    pub int_edges: u32,
    pub slots_skipped: u32,
    pub i2c_errors: u32,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR ([`i2c_error_name`]).
    pub last_i2c_error: u8,
    /// 0x010F..0x0110 — expected [`super::MODEL_ID_VL53L4CD`] (0xEBAA).
    pub model_id: u16,
    /// 0x00E5 — expected 0x03 (booted).
    pub fw_status: u8,
    /// 1 = MODEL_ID matched on this read.
    pub initialized: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
}
```

A decoded `RPT_VL53_INFO` report — bridge diagnostics. Counters are
free-running and wrap silently; watch increments, not absolute values.

#### Vl53l4Info::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Vl53l4Info, CodecError>
```

### StreamData

```rust
pub struct StreamData {
    /// MCU uptime at the INT edge (the sensor event, not the I2C completion).
    pub timestamp_us: u64,
    pub addr: u16,
    pub len: u16,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}
```

A decoded `RPT_VL53_STREAM` report — one streamed register block.
`addr`/`len` echo the stream configuration so each report is
self-describing.

#### StreamData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<StreamData, CodecError>
```

### SOFT_RESET

```rust
pub const SOFT_RESET: u16 = 0x0000;
```

### I2C_SLAVE__DEVICE_ADDRESS

```rust
pub const I2C_SLAVE__DEVICE_ADDRESS: u16 = 0x0001;
```

### OSC_FREQUENCY

```rust
pub const OSC_FREQUENCY: u16 = 0x0006;
```

Oscillator-frequency word (unnamed in the C driver).

### VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND

```rust
pub const VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND: u16 = 0x0008;
```

### XTALK_PLANE_OFFSET_KCPS

```rust
pub const XTALK_PLANE_OFFSET_KCPS: u16 = 0x0016;
```

### XTALK_X_PLANE_GRADIENT_KCPS

```rust
pub const XTALK_X_PLANE_GRADIENT_KCPS: u16 = 0x0018;
```

### XTALK_Y_PLANE_GRADIENT_KCPS

```rust
pub const XTALK_Y_PLANE_GRADIENT_KCPS: u16 = 0x001A;
```

### RANGE_OFFSET_MM

```rust
pub const RANGE_OFFSET_MM: u16 = 0x001E;
```

### INNER_OFFSET_MM

```rust
pub const INNER_OFFSET_MM: u16 = 0x0020;
```

### OUTER_OFFSET_MM

```rust
pub const OUTER_OFFSET_MM: u16 = 0x0022;
```

### GPIO_HV_MUX__CTRL

```rust
pub const GPIO_HV_MUX__CTRL: u16 = 0x0030;
```

### GPIO__TIO_HV_STATUS

```rust
pub const GPIO__TIO_HV_STATUS: u16 = 0x0031;
```

### SYSTEM__INTERRUPT

```rust
pub const SYSTEM__INTERRUPT: u16 = 0x0046;
```

### RANGE_CONFIG_A

```rust
pub const RANGE_CONFIG_A: u16 = 0x005E;
```

### RANGE_CONFIG_B

```rust
pub const RANGE_CONFIG_B: u16 = 0x0061;
```

### RANGE_CONFIG__SIGMA_THRESH

```rust
pub const RANGE_CONFIG__SIGMA_THRESH: u16 = 0x0064;
```

### MIN_COUNT_RATE_RTN_LIMIT_MCPS

```rust
pub const MIN_COUNT_RATE_RTN_LIMIT_MCPS: u16 = 0x0066;
```

### INTERMEASUREMENT_MS

```rust
pub const INTERMEASUREMENT_MS: u16 = 0x006C;
```

### THRESH_HIGH

```rust
pub const THRESH_HIGH: u16 = 0x0072;
```

### THRESH_LOW

```rust
pub const THRESH_LOW: u16 = 0x0074;
```

### SYSTEM__INTERRUPT_CLEAR

```rust
pub const SYSTEM__INTERRUPT_CLEAR: u16 = 0x0086;
```

### SYSTEM_START

```rust
pub const SYSTEM_START: u16 = 0x0087;
```

### RESULT__RANGE_STATUS

```rust
pub const RESULT__RANGE_STATUS: u16 = 0x0089;
```

### RESULT__SPAD_NB

```rust
pub const RESULT__SPAD_NB: u16 = 0x008C;
```

### RESULT__SIGNAL_RATE

```rust
pub const RESULT__SIGNAL_RATE: u16 = 0x008E;
```

### RESULT__AMBIENT_RATE

```rust
pub const RESULT__AMBIENT_RATE: u16 = 0x0090;
```

### RESULT__SIGMA

```rust
pub const RESULT__SIGMA: u16 = 0x0092;
```

### RESULT__DISTANCE

```rust
pub const RESULT__DISTANCE: u16 = 0x0096;
```

### RESULT__OSC_CALIBRATE_VAL

```rust
pub const RESULT__OSC_CALIBRATE_VAL: u16 = 0x00DE;
```

### FIRMWARE__SYSTEM_STATUS

```rust
pub const FIRMWARE__SYSTEM_STATUS: u16 = 0x00E5;
```

### IDENTIFICATION__MODEL_ID

```rust
pub const IDENTIFICATION__MODEL_ID: u16 = 0x010F;
```

### MODEL_ID_VL53L4CD

```rust
pub const MODEL_ID_VL53L4CD: u16 = 0xEBAA;
```

Expected `IDENTIFICATION__MODEL_ID` word for a VL53L4CD.

### WINDOW_BELOW

```rust
pub const WINDOW_BELOW: u8 = 0;
```

Detection-threshold window mode (`SYSTEM__INTERRUPT`): below.

### WINDOW_ABOVE

```rust
pub const WINDOW_ABOVE: u8 = 1;
```

Detection-threshold window mode (`SYSTEM__INTERRUPT`): above.

### WINDOW_OUT

```rust
pub const WINDOW_OUT: u8 = 2;
```

Detection-threshold window mode (`SYSTEM__INTERRUPT`): out of window.

### WINDOW_IN

```rust
pub const WINDOW_IN: u8 = 3;
```

Detection-threshold window mode (`SYSTEM__INTERRUPT`): in window.

### CONFIG_ADDR

```rust
pub const CONFIG_ADDR: u16 = 0x002D;
```

First register of the init configuration block (0x2D).

### CONFIG_END

```rust
pub const CONFIG_END: u16 = 0x0087;
```

Last register of the init configuration block (0x87).

### DEFAULT_CONFIGURATION

```rust
pub const DEFAULT_CONFIGURATION: [u8; 91] = [
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, // 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, // 0x35..0x3C
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, // 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21, // 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, // 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, // 0x55..0x5C
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, // 0x5D..0x64
    0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, // 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x6D..0x74
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00, // 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, // 0x7D..0x84
    0x01, 0x00, 0x00, // 0x85..0x87
];
```

`VL53L4CD_DEFAULT_CONFIGURATION[]` — 91 bytes, registers 0x2D..0x87.
[`config_block`] always overrides byte 0 (register 0x2D) with
[`CONFIG_FMP_BYTE`].

### CONFIG_FMP_BYTE

```rust
pub const CONFIG_FMP_BYTE: u8 = 0x12;
```

Value forced into byte 0 of the config block (register 0x2D): I2C Fast Mode
Plus pad, set unconditionally and never cleared (FM+ pads work at every bus
step down to 100 kHz; clearing it mid-block NACKs and truncates the write).

### config_block

```rust
pub fn config_block() -> [u8; 91]
```

The 91-byte block `sensor_init` writes at [`CONFIG_ADDR`]: the ST default
configuration with byte 0 forced to [`CONFIG_FMP_BYTE`] (Fast Mode Plus).

### RESULT_BLOCK_ADDR

```rust
pub const RESULT_BLOCK_ADDR: u16 = RESULT__RANGE_STATUS;
```

The block the MCU streams: `RESULT__RANGE_STATUS` .. 0x0099 — every field of
`VL53L4CD_ResultsData_t` in one read.

### RESULT_BLOCK_LEN

```rust
pub const RESULT_BLOCK_LEN: u16 = 17;
```

Length of the streamed result block in bytes.

### I2C_KHZ_BOOT

```rust
pub const I2C_KHZ_BOOT: u16 = 400;
```

The bridge boots at 400 kHz; an unconfigured sensor is only specified for
that speed, so init always runs its configuration block there.

### I2C_KHZ_DEFAULT

```rust
pub const I2C_KHZ_DEFAULT: u16 = 1000;
```

The bus speed the bridge is left at after init (the result-block read is
~4x faster than at boot speed).

### STATUS_RTN

```rust
pub const STATUS_RTN: [u8; 24] = [
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11,
    12,
];
```

`GetResult()` raw status → ULD status (`status_rtn[24]` in
`VL53L4CD_api.c`); raw ≥ 24 passes through unmapped.

### range_status_name

```rust
pub fn range_status_name(status: u8) -> &'static str
```

Range-status → human-readable description (UM2931, "Range status
description"). Unknown codes map to `"unknown"`.

### Vl53l4Error

```rust
pub enum Vl53l4Error {
    /// Result block shorter than the 15 bytes the decode reads.
    ShortResultBlock,
    /// `osc_frequency` register reads 0.
    OscFrequencyZero,
    /// `timing_budget_ms` outside 10..=200.
    TimingBudgetOutOfRange,
    /// `inter_measurement_ms` must be 0 (continuous) or greater than the
    /// timing budget (autonomous).
    InterMeasurementInvalid,
    /// `sigma_mm` exceeds the 16383 mm register ceiling.
    SigmaTooLarge,
}
```

VL53L4 codec/math failure.

### Vl53l4Results

```rust
pub struct Vl53l4Results {
    /// 0 = valid ([`range_status_name`]); raw ≥ 24 passes through unmapped.
    pub range_status: u8,
    pub distance_mm: u16,
    pub ambient_rate_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub signal_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub number_of_spad: u16,
    pub sigma_mm: u16,
    /// 0x008B `RESULT__STREAM_COUNT`: wraps at 255. The C ULD ignores it; it
    /// tells a frame the host never received from one the sensor never
    /// produced.
    pub stream_count: u8,
}
```

`VL53L4CD_ResultsData_t` plus the sensor's own frame counter.

#### Vl53l4Results::status_text

```rust
pub fn status_text(&self) -> &'static str
```

Human-readable [`Self::range_status`] ([`range_status_name`]).

### parse_result_block

```rust
pub fn parse_result_block(raw: &[u8]) -> Result<Vl53l4Results, Vl53l4Error>
```

Decode the streamed 0x0089..0x0099 block exactly as `VL53L4CD_GetResult()`
decodes the same registers read one by one. Register contents are
big-endian words (the bridge passes them through untouched).

### range_timing_registers

```rust
pub fn range_timing_registers(
    timing_budget_ms: u32,
    inter_measurement_ms: u32,
    osc_frequency: u16,
    clock_pll: u16,
) -> Result<(u16, u16, u32), Vl53l4Error>
```

`SetRangeTiming` register math → `(RANGE_CONFIG_A, RANGE_CONFIG_B,
INTERMEASUREMENT_MS raw dword)`.

`osc_frequency` is the word read from 0x0006; `clock_pll` is the word read
from `RESULT__OSC_CALIBRATE_VAL` (used only in autonomous mode, i.e. when
`inter_measurement_ms > 0`). `inter_measurement_ms == 0` selects continuous
mode; a value greater than the budget selects autonomous low power.

### decode_range_timing

```rust
pub fn decode_range_timing(
    intermeasurement_raw: u32,
    clock_pll: u16,
    osc_frequency: u16,
    range_config_a: u16,
) -> Result<(u32, u32), Vl53l4Error>
```

`GetRangeTiming` register math → `(timing_budget_ms, inter_measurement_ms)`.

Inputs are the raw register reads: the `INTERMEASUREMENT_MS` dword, the
`RESULT__OSC_CALIBRATE_VAL` word, the 0x0006 word and the `RANGE_CONFIG_A`
word.

### offset_raw

```rust
pub fn offset_raw(offset_mm: i32) -> u16
```

`RANGE_OFFSET_MM` word for `SetOffset` (`INNER`/`OUTER` are zeroed
alongside).

### decode_offset

```rust
pub fn decode_offset(raw_word: u16) -> i32
```

`GetOffset`: `RANGE_OFFSET_MM` word → signed millimetres.

### xtalk_raw

```rust
pub fn xtalk_raw(xtalk_kcps: u16) -> u16
```

`XTALK_PLANE_OFFSET_KCPS` word for `SetXtalk`.

### decode_xtalk

```rust
pub fn decode_xtalk(raw_word: u16) -> u16
```

`GetXtalk`: `XTALK_PLANE_OFFSET_KCPS` word → kcps (round half to even,
matching the Python reference's `round`).

### signal_threshold_raw

```rust
pub fn signal_threshold_raw(signal_kcps: u16) -> u16
```

`MIN_COUNT_RATE_RTN_LIMIT_MCPS` word for `SetSignalThreshold`.

### decode_signal_threshold

```rust
pub fn decode_signal_threshold(raw_word: u16) -> u16
```

`GetSignalThreshold`: register word → kcps.

### sigma_threshold_raw

```rust
pub fn sigma_threshold_raw(sigma_mm: u16) -> Result<u16, Vl53l4Error>
```

`RANGE_CONFIG__SIGMA_THRESH` word for `SetSigmaThreshold`;
[`Vl53l4Error::SigmaTooLarge`] above 16383 mm.

### decode_sigma_threshold

```rust
pub fn decode_sigma_threshold(raw_word: u16) -> u16
```

`GetSigmaThreshold`: register word → millimetres.

## VL53L8 (ToF)

### DIST_MM

```rust
pub const DIST_MM: u8 = 1;
```

Detection-threshold measurement selectors (`measurement` field).

### SIGNAL_PER_SPAD_KCPS

```rust
pub const SIGNAL_PER_SPAD_KCPS: u8 = 2;
```

### RANGE_SIGMA_MM

```rust
pub const RANGE_SIGMA_MM: u8 = 4;
```

### AMBIENT_PER_SPAD_KCPS

```rust
pub const AMBIENT_PER_SPAD_KCPS: u8 = 8;
```

### NB_TARGET_DETECTED

```rust
pub const NB_TARGET_DETECTED: u8 = 9;
```

### TAR_STATUS

```rust
pub const TAR_STATUS: u8 = 12;
```

### NB_SPADS_ENABLED

```rust
pub const NB_SPADS_ENABLED: u8 = 13;
```

### MOTION_INDICATOR

```rust
pub const MOTION_INDICATOR: u8 = 19;
```

### NB_THRESHOLDS

```rust
pub const NB_THRESHOLDS: usize = 64;
```

### POWER_MODE_SLEEP

```rust
pub const POWER_MODE_SLEEP: u8 = 0;
```

Power modes (`vl53l8cx_api.h`).

### POWER_MODE_WAKEUP

```rust
pub const POWER_MODE_WAKEUP: u8 = 1;
```

### POWER_MODE_DEEP_SLEEP

```rust
pub const POWER_MODE_DEEP_SLEEP: u8 = 2;
```

### xtalk_margin_to_raw

```rust
pub fn xtalk_margin_to_raw(margin_kcps: f64) -> u32
```

Xtalk margin (kcps/SPAD) → raw DCI value: `round(kcps * 2048)`.

### MotionConfig

```rust
pub struct MotionConfig {
    pub ref_bin_offset: i32,
    pub detection_threshold: u32,
    pub extra_noise_sigma: u32,
    pub null_den_clip_value: u32,
    pub mem_update_mode: u8,
    pub mem_update_choice: u8,
    pub sum_span: u8,
    pub feature_length: u8,
    pub nb_of_aggregates: u8,
    pub nb_of_temporal_accumulations: u8,
    pub min_nb_for_global_detection: u8,
    pub global_indicator_format_1: u8,
    pub global_indicator_format_2: u8,
    pub spare1: u8,
    pub spare2: u8,
    pub spare3: u8,
    pub map_id: [i8; 64],
    pub indicator_format_1: [u8; 32],
    pub indicator_format_2: [u8; 32],
}
```

Mirror of `VL53L8CX_Motion_Configuration` (156 bytes, plugin source).
`pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).

#### MotionConfig::pack

```rust
pub fn pack(&self) -> Vec<u8>
```

Serialize to the 156-byte DCI payload.

#### MotionConfig::set_resolution

```rust
pub fn set_resolution(&mut self, resolution: usize)
```

Set `map_id` for the resolution (pure; mirrors `_set_resolution`).

### default_motion_config

```rust
pub fn default_motion_config(resolution: usize) -> MotionConfig
```

The default motion-indicator configuration `motion_indicator_init` programs
for a resolution (the exact bytes written to the sensor).

### DetectionThreshold

```rust
pub struct DetectionThreshold {
    pub low_thresh: i32,
    pub high_thresh: i32,
    pub measurement: u8,
    pub type_: u8,
    pub zone_num: u8,
    pub operation: u8,
}
```

One detection-threshold entry (real units; scaled on pack).

### DetectionThresholdBlocks

```rust
pub struct DetectionThresholdBlocks {
    /// `DCI_DET_THRESH_START` payload: 64 × 12 bytes.
    pub start: Vec<u8>,
    /// `DCI_DET_THRESH_VALID_STATUS`: 8 bytes, all `0x05`.
    pub valid: Vec<u8>,
}
```

The two DCI blocks written by `set_detection_thresholds`.

### pack_detection_thresholds

```rust
pub fn pack_detection_thresholds(thresholds: &[DetectionThreshold]) -> DetectionThresholdBlocks
```

Pack up to 64 detection thresholds into their DCI blocks. Entries beyond
the supplied list are zero-filled. Each entry's low/high are multiplied by
its measurement's fixed-point scale.

### CnhDecodeConfig

```rust
pub struct CnhDecodeConfig {
    /// Number of CNH aggregates (`cfg.nb_of_aggregates`).
    pub nb_of_aggregates: usize,
    /// CNH bins per aggregate (`cfg.feature_length`).
    pub feature_length: usize,
}
```

Minimal config needed to decode a captured CNH block: the aggregate count and
per-aggregate feature (bin) length the sensor was configured with. These must
match the `CnhConfig` used when programming the device (see the Python
`CnhConfig.nb_of_aggregates` / `feature_length`).

### CnhAggregate

```rust
pub struct CnhAggregate {
    /// Per-bin integer mantissa (`FEAT_INT`), length == `feature_length`.
    pub hist_raw: Vec<i32>,
    /// Per-bin power-of-two scaler (`FEAT_FRAC`), length == `feature_length`.
    pub hist_scaler: Vec<i8>,
}
```

One decoded CNH aggregate. The real histogram value for bin `i` is
`hist_raw[i] as f64 / 2f64.powi(hist_scaler[i] as i32)`.

### CnhData

```rust
pub struct CnhData {
    /// Reference residual word, u32 at byte offset 8 (`words[2]`). Real value is
    /// `ref_residual_word as f64 / 2048.0` (11 fractional bits).
    pub ref_residual_word: u32,
    /// Per-aggregate histograms, length == `nb_of_aggregates`.
    pub aggregates: Vec<CnhAggregate>,
}
```

Result of [`decode_cnh`].

### CnhError

```rust
pub enum CnhError {
    /// `raw` is too short for the header or the computed block extends past it.
    Truncated,
    /// `nb_of_aggregates` or `feature_length` is zero.
    EmptyConfig,
}
```

CNH decode failure.

### decode_cnh

```rust
pub fn decode_cnh(cfg: &CnhDecodeConfig, raw: &[u8]) -> Result<CnhData, CnhError>
```

Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like the
standard ranging blocks — i.e. [`crate::vl53l8::decode::Vl53l8Results::cnh_raw`])
into per-aggregate integer histograms plus the reference-residual word.

Faithful port of the Python `cnh.decode` / `_decode_aggregate` for the fixed
DEPZ `cnh_cfg` (ping-pong + variance disabled). With ping-pong disabled the
device reports a single buffer and the ping/pong selection resolves to the
sole buffer.

### RESOLUTION_4X4

```rust
pub const RESOLUTION_4X4: usize = 16;
```

### RESOLUTION_8X8

```rust
pub const RESOLUTION_8X8: usize = 64;
```

### NB_TARGET_PER_ZONE

```rust
pub const NB_TARGET_PER_ZONE: usize = 1;
```

### CNH_DATA_IDX

```rust
pub const CNH_DATA_IDX: u16 = 0xc048;
```

CNH (compact network histogram) output block id — **CH-only**. The DEPZ
decode surfaces this block's raw bytes ([`Vl53l8Results::cnh_raw`]); the full
histogram unpack is a not-yet-implemented CH extension point (see below).

### Variant

```rust
pub enum Variant {
    /// Base VL53L8CX (dev default), or any device on ULD 2.1.0 footer geometry.
    Cx,
    /// VL53L8CH on ULD 2.0.16 footer geometry.
    Ch,
}
```

ToF silicon/firmware variant. Both the base **VL53L8CX** and the
**VL53L8CH** (CX + CNH + production PID 0xED40) share one results-frame
layout; only the frame-tail geometry differs for decoding, and that is all
this enum selects. The footer-id offset is `size-12` (ULD 2.1.0, selected by
[`Variant::Cx`]) or `size-4` (ULD 2.0.16, selected by [`Variant::Ch`]).

Note the geometry tracks the *ULD version the firmware embeds*, not the
silicon: the DEPZ firmware streams 2.1.0-footer frames on both CX and CH
devices, so a CH capture still decodes with [`Variant::Cx`] geometry.

### Vl53l8Error

```rust
pub enum Vl53l8Error {
    /// Header/footer id mismatch (`STATUS_CORRUPTED_FRAME`).
    CorruptedFrame,
    /// Frame shorter than the fixed 16-byte prologue.
    ShortFrame,
}
```

Frame-decode failure.

### Vl53l8Results

```rust
pub struct Vl53l8Results {
    /// mm, already scaled `floor(raw/4)` per ST `GetRangingData`.
    pub distance_mm: Vec<i32>,
    /// 5/9 = valid; 255 = no target detected in the zone.
    pub target_status: Vec<u8>,
    pub nb_target_detected: Vec<u8>,
    /// kcps/SPAD, raw fixed-point (÷2048 for real units).
    pub signal_per_spad: Vec<u32>,
    /// kcps/SPAD, raw fixed-point.
    pub ambient_per_spad: Vec<u32>,
    pub nb_spads_enabled: Vec<u32>,
    /// mm, raw fixed-point (÷128 for real units).
    pub range_sigma_mm_raw: Vec<u16>,
    /// reflectance %.
    pub reflectance: Vec<u8>,
    pub silicon_temp_degc: i8,
    /// Raw compact-network-histogram block bytes, present only on **VL53L8CH**
    /// frames that carry a CNH output block. Pass this to
    /// [`super::cnh::decode_cnh`] to unpack it into per-aggregate histograms.
    /// `None` on CX frames and on CH frames without a CNH block.
    pub cnh_raw: Option<Vec<u8>>,
}
```

One decoded results frame. Per-zone arrays are sized to the resolution
actually present in the frame (`resolution` = `nb_target_detected.len()`).

#### Vl53l8Results::resolution

```rust
pub fn resolution(&self) -> usize
```

Active resolution (16 or 64), from the number of zones decoded.

### swap_buffer

```rust
pub fn swap_buffer(data: &[u8]) -> Vec<u8>
```

VL53L8CX `SwapBuffer`: byte-reverse every complete 32-bit word (a <4-byte
tail is left untouched).

### parse_frame

```rust
pub fn parse_frame(raw: &[u8], variant: Variant) -> Result<Vl53l8Results, Vl53l8Error>
```

Decode one raw results frame (`raw` = `full_size` bytes read from reg 0x00).

The frame's own length is authoritative (`data_read_size = raw.len()`); the
FW streams exactly the size advertised in each `RPT_VL53_FRAME` chunk.

### STREAM_CHUNK_MAX

```rust
pub const STREAM_CHUNK_MAX: usize = 1528;
```

Bytes of frame data carried per `RPT_VL53_FRAME` chunk.

### STREAM_TOTAL_MAX

```rust
pub const STREAM_TOTAL_MAX: usize = 8192;
```

Largest `frame_size` accepted by `START_STREAM`.

### FrameChunk

```rust
pub struct FrameChunk {
    pub timestamp_us: u64,
    pub full_size: u16,
    pub offset: u16,
    pub data: Vec<u8>,
}
```

One `RPT_VL53_FRAME` chunk (header + a slice of the frame).

### unpack_frame_chunk

```rust
pub fn unpack_frame_chunk(payload: &[u8]) -> Option<FrameChunk>
```

Decode an `RPT_VL53_FRAME` payload: `timestamp_us` u64 LE, `full_size` u16
LE, `offset` u16 LE, then the chunk data. Returns `None` if the payload is
shorter than the 12-byte header.

### CompletedFrame

```rust
pub struct CompletedFrame {
    pub timestamp_us: u64,
    pub frame: Vec<u8>,
}
```

A completed sensor frame with its capture timestamp.

### FrameReassembler

```rust
pub struct FrameReassembler {
    pub completed: u64,
    pub discarded: u64,
    buf: Vec<u8>,
    full_size: usize,
    timestamp_us: u64,
}
```

Rebuilds full sensor frames from chunked `RPT_VL53_FRAME` reports.

#### FrameReassembler::new

```rust
pub fn new() -> Self
```

#### FrameReassembler::feed

```rust
pub fn feed(&mut self, chunk: FrameChunk) -> Option<CompletedFrame>
```

Feed one chunk; returns a `CompletedFrame` when a frame finishes.

## BNO086 (IMU)

### BASE_TIMESTAMP_REF

```rust
pub const BASE_TIMESTAMP_REF: u8 = 0xfb;
```

In-cargo control IDs on the input channels.

### TIMESTAMP_REBASE

```rust
pub const TIMESTAMP_REBASE: u8 = 0xfa;
```

### RV_ACCURACY_Q

```rust
pub const RV_ACCURACY_Q: u32 = 12;
```

Rotation-vector accuracy estimate Q point (radians).

### GYRO_RV_ANGVEL_Q

```rust
pub const GYRO_RV_ANGVEL_Q: u32 = 10;
```

Gyro-integrated RV angular-velocity Q point (rad/s).

### q_point

```rust
pub fn q_point(sensor_id: u8) -> u32
```

Q point of the primary fields (value = raw / 2**Q).

### report_length

```rust
pub fn report_length(sensor_id: u8) -> Option<usize>
```

Total report length on the wire, 4-byte SH-2 header included.

### Report

```rust
pub enum Report {
    /// 0x01 accel / 0x04 linear accel / 0x06 gravity (Q8), 0x02 gyro (Q9),
    /// 0x03 magnetometer (Q4).
    Vector3 {
        kind: Vector3Kind,
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
    },
    /// 0x07 uncal gyro (Q9) / 0x0F uncal mag (Q4), primary + bias.
    Vector3WithBias {
        is_gyro: bool,
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
        bias_x_raw: i32,
        bias_y_raw: i32,
        bias_z_raw: i32,
    },
    /// 0x05/0x08/0x09/0x28/0x29 quaternion (Q14); `accuracy_raw` present for
    /// 0x05/0x09/0x28 only (Q12 radians).
    RotationVector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        i_raw: i32,
        j_raw: i32,
        k_raw: i32,
        real_raw: i32,
        accuracy_raw: Option<i32>,
    },
    /// 0x0A–0x0E single-value environment reports.
    Scalar {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        value_raw: i64,
    },
    /// 0x10 tap detector; `flags` bit 6 = double tap.
    TapDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        flags: u8,
    },
    /// 0x11 step counter.
    StepCounter {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        latency_us: u32,
        steps: u16,
    },
    /// 0x18 step detector.
    StepDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        latency_us: u32,
    },
    /// 0x12 significant motion.
    SignificantMotion {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        motion: u16,
    },
    /// 0x13 stability classifier.
    StabilityClassifier {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        classification: u8,
    },
    /// 0x19 shake detector; bits 0/1/2 = X/Y/Z.
    ShakeDetector {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        flags: u16,
    },
    /// 0x1E personal activity classifier.
    PersonalActivityClassifier {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        page_number: u8,
        end_of_sequence: bool,
        most_likely_state: u8,
        confidences: Vec<u8>,
    },
    /// 0x14/0x15/0x16 raw ADC + sensor-clock timestamp (temp for raw gyro).
    RawSensor {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        x_raw: i32,
        y_raw: i32,
        z_raw: i32,
        sensor_timestamp_us: u32,
        temperature_raw: i32,
    },
    /// In-table detector without a dedicated shape (u16 value).
    GenericEvent {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        value_raw: u16,
    },
    /// 0x2A gyro-integrated rotation vector (channel 5, dense).
    GyroIntegratedRv {
        sensor_id: u8,
        timestamp_us: i64,
        i_raw: i32,
        j_raw: i32,
        k_raw: i32,
        real_raw: i32,
        vx_raw: i32,
        vy_raw: i32,
        vz_raw: i32,
    },
    /// Unrecognized report ID: raw bytes from the ID to the end of the cargo.
    Unknown {
        sensor_id: u8,
        timestamp_us: i64,
        data: Vec<u8>,
    },
}
```

A parsed report. Raw fields are authoritative; scale with the Q points.

### Vector3Kind

```rust
pub enum Vector3Kind {
    Acceleration,
    Gyroscope,
    Magnetometer,
}
```

Which of the plain 3-vector reports.

### parse_input_cargo

```rust
pub fn parse_input_cargo(payload: &[u8], capture_timestamp_us: i64) -> Vec<Report>
```

Parse a channel-3/4 cargo into typed reports. `capture_timestamp_us` is the
bridge RPT_DATA capture time (MCU uptime).

### parse_gyro_rv_cargo

```rust
pub fn parse_gyro_rv_cargo(payload: &[u8], capture_timestamp_us: i64) -> Option<Report>
```

Parse a channel-5 cargo (gyro-integrated RV, dense format).

Two shapes: 7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay
(both 100 µs ticks). Returns `None` if the cargo is too short.

### REPORT_COMMAND_RESPONSE

```rust
pub const REPORT_COMMAND_RESPONSE: u8 = 0xf1;
```

Report IDs on SHTP channel 2 (control).

### REPORT_COMMAND_REQUEST

```rust
pub const REPORT_COMMAND_REQUEST: u8 = 0xf2;
```

### REPORT_FRS_READ_RESPONSE

```rust
pub const REPORT_FRS_READ_RESPONSE: u8 = 0xf3;
```

### REPORT_FRS_READ_REQUEST

```rust
pub const REPORT_FRS_READ_REQUEST: u8 = 0xf4;
```

### REPORT_FRS_WRITE_RESPONSE

```rust
pub const REPORT_FRS_WRITE_RESPONSE: u8 = 0xf5;
```

### REPORT_FRS_WRITE_DATA

```rust
pub const REPORT_FRS_WRITE_DATA: u8 = 0xf6;
```

### REPORT_FRS_WRITE_REQUEST

```rust
pub const REPORT_FRS_WRITE_REQUEST: u8 = 0xf7;
```

### REPORT_PRODUCT_ID_RESPONSE

```rust
pub const REPORT_PRODUCT_ID_RESPONSE: u8 = 0xf8;
```

### REPORT_PRODUCT_ID_REQUEST

```rust
pub const REPORT_PRODUCT_ID_REQUEST: u8 = 0xf9;
```

### REPORT_GET_FEATURE_RESPONSE

```rust
pub const REPORT_GET_FEATURE_RESPONSE: u8 = 0xfc;
```

### REPORT_SET_FEATURE_COMMAND

```rust
pub const REPORT_SET_FEATURE_COMMAND: u8 = 0xfd;
```

### REPORT_GET_FEATURE_REQUEST

```rust
pub const REPORT_GET_FEATURE_REQUEST: u8 = 0xfe;
```

### Sh2Error

```rust
pub enum Sh2Error {
    /// Command request carries at most 9 parameter bytes.
    TooManyParams(usize),
    /// FRS write data carries 1 or 2 words.
    BadWriteWordCount(usize),
}
```

SH-2 command builder error.

### build_set_feature

```rust
pub fn build_set_feature(
    sensor_id: u8,
    interval_us: u32,
    batch_us: u32,
    sensitivity: u16,
    flags: u8,
    cfg_word: u32,
) -> [u8; 17]
```

Set Feature Command (0xFD), 17 bytes. `interval_us` = 0 disables the sensor.

### build_get_feature_request

```rust
pub fn build_get_feature_request(sensor_id: u8) -> [u8; 2]
```

Get Feature Request (0xFE), 2 bytes.

### build_product_id_request

```rust
pub fn build_product_id_request() -> [u8; 2]
```

Product ID Request (0xF9), 2 bytes.

### build_command_request

```rust
pub fn build_command_request(seq: u8, command: u8, params: &[u8]) -> Result<[u8; 12], Sh2Error>
```

Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.

### build_frs_read_request

```rust
pub fn build_frs_read_request(frs_type: u16, offset_words: u16, block_words: u16) -> [u8; 8]
```

FRS Read Request (0xF4), 8 bytes. `block_words` = 0 reads the whole record.

### build_frs_write_request

```rust
pub fn build_frs_write_request(frs_type: u16, length_words: u16) -> [u8; 6]
```

FRS Write Request (0xF7), 6 bytes. `length_words` = 0 erases the record.

### build_frs_write_data

```rust
pub fn build_frs_write_data(offset_words: u16, words: &[u32]) -> Result<[u8; 12], Sh2Error>
```

FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.

### SHTP_HEADER_SIZE

```rust
pub const SHTP_HEADER_SIZE: usize = 4;
```

### LENGTH_MASK

```rust
pub const LENGTH_MASK: u16 = 0x7fff;
```

### CONTINUATION_BIT

```rust
pub const CONTINUATION_BIT: u16 = 0x8000;
```

### NUM_CHANNELS

```rust
pub const NUM_CHANNELS: usize = 6;
```

### MAX_TX_FRAME

```rust
pub const MAX_TX_FRAME: usize = 64;
```

Host→sensor frames must fit one MCU transmit slot (ERRATA E2).

### ShtpChannel

```rust
pub enum ShtpChannel {
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
}
```

SHTP channels (contract 05 §3).

### ShtpHeader

```rust
pub struct ShtpHeader {
    /// Bits 14:0 — cargo length incl. this 4-byte header.
    pub length: u16,
    pub channel: u8,
    pub seq: u8,
    pub continuation: bool,
}
```

A decoded SHTP header.

### pack_shtp_header

```rust
pub fn pack_shtp_header(hdr: &ShtpHeader) -> [u8; 4]
```

Serialize an SHTP header to 4 bytes.

### unpack_shtp_header

```rust
pub fn unpack_shtp_header(data: &[u8]) -> ShtpHeader
```

Decode a 4-byte SHTP header (reads the first 4 bytes of `data`).

### ShtpCargo

```rust
pub struct ShtpCargo {
    pub channel: u8,
    /// seq of the first fragment.
    pub seq: u8,
    pub payload: Vec<u8>,
}
```

One reassembled cargo: `payload` excludes all SHTP headers.

### build_frame

```rust
pub fn build_frame(channel: u8, payload: &[u8], seq: u8) -> Vec<u8>
```

Single-fragment frame: length = header + payload.

### ShtpLayer

```rust
pub struct ShtpLayer {
    /// Incomplete cargos thrown away.
    pub discarded: u64,
    tx_seqs: [u8; NUM_CHANNELS],
    rx: Vec<ChannelRx>,
}
```

Per-channel TX sequence counters + RX cargo reassembly.

#### ShtpLayer::new

```rust
pub fn new() -> Self
```

#### ShtpLayer::next_frame

```rust
pub fn next_frame(&mut self, channel: u8, payload: &[u8]) -> Vec<u8>
```

Build a single-fragment frame, consuming the channel's TX seq.

#### ShtpLayer::tx_seq

```rust
pub fn tx_seq(&self, channel: u8) -> u8
```

Current TX seq counter for a channel (next frame's seq).

#### ShtpLayer::feed

```rust
pub fn feed(&mut self, frame: &[u8]) -> Option<ShtpCargo>
```

Consume one inbound frame; return the cargo when complete.

Rules (contract 05 §3): a non-continuation fragment starts a new cargo
(discarding any partial one on that channel); a continuation without a
cargo in progress is dropped; the cargo completes when the accumulated
bytes reach the first fragment's advertised total.

#### ShtpLayer::reset

```rust
pub fn reset(&mut self)
```

Forget all TX seq counters and partial cargos (sensor reset).

## Bootloader / firmware update

### HEADER_SIZE

```rust
pub const HEADER_SIZE: usize = 64;
```

64-byte fixed header.

### MAGIC

```rust
pub const MAGIC: &[u8; 8] = b"FWDEPZ00";
```

Magic prefix.

### FwdepzHeader

```rust
pub struct FwdepzHeader {
    pub load_addr: u32,
    pub fw_size: u32,
    pub fw_crc32: u32,
    pub cur_sec: u8,
    pub tot_sec: u8,
    /// `true` when CRC-32/ISO-HDLC of the payload equals `fw_crc32`.
    pub payload_crc_ok: bool,
}
```

Parsed `.fwdepz` header plus a payload-CRC verdict.

### FwdepzError

```rust
pub enum FwdepzError {
    /// Too short to hold a header, or bad magic.
    Magic,
    /// Header CRC-16/CCITT-FALSE mismatch.
    HeaderCrc,
    /// `fw_size` != payload length.
    Size,
}
```

Reasons a `.fwdepz` blob fails validation, in the normative check order.

### parse

```rust
pub fn parse(file: &[u8]) -> Result<FwdepzHeader, FwdepzError>
```

Parse and validate a `.fwdepz` blob. Validation order: magic → header CRC →
`fw_size == len(payload)` (contract 06 §2).

## Datasets (record & replay)

### DatasetError

```rust
pub enum DatasetError {
    /// The file has no header line.
    Empty,
    /// A line was not valid JSON.
    Json(JsonError),
    /// A structural expectation failed (missing field, wrong type, …).
    Structure(String),
}
```

Dataset parse failure.

### Device

```rust
pub struct Device {
    /// Header key (`d0`, `d1`, …).
    pub id: String,
    pub serial: Option<String>,
    pub sensor_type: Option<String>,
    pub software_name: Option<String>,
    /// `time_sync.offset_us` — `t = device_ts_us − offset_us` (contract 02 §5).
    pub offset_us: Option<i64>,
    pub rtt_us: Option<i64>,
}
```

A device declared in the dataset header, in `d0`, `d1`, … assignment order.

### RecordValue

```rust
pub enum RecordValue {
    /// `sr04`: `echo_us` (0xFFFF = timeout), `source` = "once" | "loop".
    Sr04 { echo_us: i64, source: String },
    /// `temperature`: MCU temperature in °C.
    Temperature { celsius: f64 },
    /// Any other (or reserved) kind — the raw `v` object.
    Other(Value),
}
```

A decoded record value; unknown kinds keep their raw JSON (`Other`).

### Record

```rust
pub struct Record {
    /// Device id (`d`), referencing a header device.
    pub device: String,
    /// Host-monotonic µs (`t`).
    pub t: i64,
    /// Record kind (`k`).
    pub kind: String,
    pub value: RecordValue,
}
```

One dataset record on the shared host timeline.

### Dataset

```rust
pub struct Dataset {
    pub schema: String,
    pub created_utc: Option<String>,
    pub note: Option<String>,
    pub devices: Vec<Device>,
    pub records: Vec<Record>,
}
```

A parsed dataset: header devices plus the record stream (file order).

#### Dataset::parse

```rust
pub fn parse(text: &str) -> Result<Dataset, DatasetError>
```

Parse a `.depzdata` document from its UTF-8 text.

#### Dataset::records_for

```rust
pub fn records_for<'a>(&'a self, device: &'a str) -> impl Iterator<Item = &'a Record>
```

Records for one device id, in file order.

## Transport

### crc8_maxim

```rust
pub fn crc8_maxim(data: &[u8]) -> u8
```

CRC-8/MAXIM: poly 0x31 reflected (0x8C), init 0x00, xorout 0x00.

### crc16_modbus

```rust
pub fn crc16_modbus(data: &[u8]) -> u16
```

CRC-16/MODBUS: poly 0x8005 reflected (0xA001), init 0xFFFF, xorout 0x0000.

### crc32_iso_hdlc

```rust
pub fn crc32_iso_hdlc(data: &[u8]) -> u32
```

CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected (0xEDB88320), init/xorout 0xFFFFFFFF.

### crc16_ccitt_false

```rust
pub fn crc16_ccitt_false(data: &[u8]) -> u16
```

CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.

Used only for the `.fwdepz` file header (contract 06), never on the wire.

### MAGIC

```rust
pub const MAGIC: [u8; 2] = [0xA5, 0xC3];
```

Frame start marker.

### HEADER_SIZE

```rust
pub const HEADER_SIZE: usize = 7;
```

Fixed header length: magic(2) + data_size(2) + cmd(1) + seq(1) + hdr_crc(1).

### MAX_PAYLOAD

```rust
pub const MAX_PAYLOAD: usize = 0x3FFF;
```

Maximum payload length (14-bit field).

### CrcType

```rust
pub enum CrcType {
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
}
```

Payload CRC selector encoded in the top two bits of `data_size`.

#### CrcType::from_bits

```rust
pub fn from_bits(v: u16) -> CrcType
```

Decode from the 2-bit field value.

### FramingError

```rust
pub enum FramingError {
    PayloadTooLong(usize),
}
```

Errors from `build_packet`.

### payload_crc_bytes

```rust
pub fn payload_crc_bytes(crc_type: CrcType, payload: &[u8]) -> Vec<u8>
```

CRC trailer for a payload; empty payloads never carry CRC bytes (ERRATA E6).

### build_packet

```rust
pub fn build_packet(
    cmd: u8,
    payload: &[u8],
    seq: u32,
    crc_type: CrcType,
) -> Result<Vec<u8>, FramingError>
```

Frame one packet. `crc_type` bits are set in the header even for an empty
payload (matching device TX), but CRC bytes are only appended for non-empty
payloads. `seq` is taken modulo 256.

### Packet

```rust
pub struct Packet {
    pub cmd: u8,
    pub seq: u8,
    pub payload: Vec<u8>,
}
```

A decoded packet.

### Event

```rust
pub enum Event {
    /// A well-formed packet.
    Packet(Packet),
    /// Bytes discarded while hunting for a valid frame. Boundaries between
    /// consecutive `Trash` events depend on read chunking; only the
    /// concatenated byte stream is deterministic.
    Trash(Vec<u8>),
    /// A frame with a valid header whose payload CRC failed; dropped.
    CrcError { cmd: u8, seq: u8 },
}
```

Events yielded by the incremental parser.

### PacketParser

```rust
pub struct PacketParser {
    buf: Vec<u8>,
    pub packets: u64,
    pub crc_errors: u64,
    pub header_errors: u64,
    pub trash_bytes: u64,
}
```

Incremental frame parser. Feed arbitrary byte chunks; get events.

Event order is invariant to chunking (contract 01 §5) except `Trash` event
boundaries — concatenate `Trash` data when comparing streams.

#### PacketParser::new

```rust
pub fn new() -> Self
```

#### PacketParser::residue

```rust
pub fn residue(&self) -> &[u8]
```

Bytes still buffered (an incomplete frame or a trailing lone `0xA5`).

#### PacketParser::feed

```rust
pub fn feed(&mut self, data: &[u8]) -> Vec<Event>
```

Feed a chunk and drain all events it makes available.

## Protocol codecs

### Cmd

```rust
pub enum Cmd {
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
}
```

Host→device command opcodes.

### Rpt

```rust
pub enum Rpt {
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    ThroughputData = 0x88,
    SyncPinConfig = 0x90,
}
```

Device→host report opcodes.

### Status

```rust
pub enum Status {
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
}
```

Status codes carried by `Rpt::Status`.

### SyncPinMode

```rust
pub enum SyncPinMode {
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
}
```

Sync-pin operating mode.

### SyncPinPolarity

```rust
pub enum SyncPinPolarity {
    IdleLow = 0x00,
    IdleHigh = 0x01,
}
```

Sync-pin idle polarity.

### UNSOLICITED

```rust
pub const UNSOLICITED: u8 = 0x00;
```

Value of the echoed-cmd byte in unsolicited reports.

### CodecError

```rust
pub struct CodecError(pub &'static str);
```

Payload codec error.

### StatusReport

```rust
pub struct StatusReport {
    pub cmd: u8,
    pub status: u8,
}
```

Decoded `Rpt::Status`: echoed request opcode (0x00 = unsolicited) + status.

#### StatusReport::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<StatusReport, CodecError>
```

### TextReport

```rust
pub struct TextReport {
    pub cmd: u8,
    pub text: String,
}
```

Decoded `Rpt::Text`: echoed opcode + ASCII text (trailing NUL/0xFF stripped).

#### TextReport::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<TextReport, CodecError>
```

### SyncTimeReport

```rust
pub struct SyncTimeReport {
    pub pc_timestamp_us: u64,
    pub mcu_rx_us: u64,
    pub mcu_tx_us: u64,
}
```

Decoded `Rpt::SyncTime`: T1 echoed, T2 mcu-rx, T3 mcu-tx (all µs).

#### SyncTimeReport::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<SyncTimeReport, CodecError>
```

### TemperatureReport

```rust
pub struct TemperatureReport {
    pub timestamp_us: u64,
    pub raw_decidegrees: i16,
}
```

Decoded `Rpt::Temperature`: timestamp + raw int16 in units of 0.1 °C.

#### TemperatureReport::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<TemperatureReport, CodecError>
```

#### TemperatureReport::celsius

```rust
pub fn celsius(&self) -> f64
```

### SequenceErrorReport

```rust
pub struct SequenceErrorReport {
    pub expected_seq: u8,
    pub received_seq: u8,
}
```

Decoded `Rpt::SequenceError`.

#### SequenceErrorReport::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<SequenceErrorReport, CodecError>
```

### SyncPinConfig

```rust
pub struct SyncPinConfig {
    pub pin: u8,
    pub mode: u8,
    pub polarity: u8,
}
```

Sync-pin configuration (pin 1..5, mode, polarity).

#### SyncPinConfig::pack

```rust
pub fn pack(&self) -> [u8; 3]
```

#### SyncPinConfig::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<SyncPinConfig, CodecError>
```

### pack_sync_time

```rust
pub fn pack_sync_time(pc_timestamp_us: u64) -> [u8; 8]
```

Encode the `SYNC_TIME` request payload (T1, µs, u64 LE).

### pack_set_payload_crc_type

```rust
pub fn pack_set_payload_crc_type(crc_type: u8) -> [u8; 1]
```

Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte).

### sync_time_offset_rtt

```rust
pub fn sync_time_offset_rtt(t1: i64, t2: i64, t3: i64, t4: i64) -> (i64, i64)
```

NTP-style clock math, all µs (contract 02 §5).

Returns `(offset_us, rtt_us)` where `offset = device_clock - host_clock`,
computed as `((T2-T1)+(T3-T4)) / 2` truncated toward zero (Rust integer
division truncates toward zero natively). Computed in `i128` to avoid
overflow, matching the shared i64/i128 rule.

### strip_device_string

```rust
pub fn strip_device_string(raw: &[u8]) -> String
```

Decode an ASCII device string, dropping trailing NUL/0xFF filler.

## JSON

### Value

```rust
pub enum Value {
    Null,
    Bool(bool),
    /// Numbers are kept as their source text so integers stay exact.
    Number(String),
    String(String),
    Array(Vec<Value>),
    /// Insertion-ordered key/value pairs.
    Object(Vec<(String, Value)>),
}
```

A parsed JSON value.

#### Value::parse

```rust
pub fn parse(text: &str) -> Result<Value, JsonError>
```

Parse a single JSON value from `text` (trailing whitespace allowed).

#### Value::get

```rust
pub fn get(&self, key: &str) -> Option<&Value>
```

Object lookup (`None` for non-objects or missing keys).

#### Value::entries

```rust
pub fn entries(&self) -> Option<&[(String, Value)]>
```

The insertion-ordered entries of an object.

#### Value::as_array

```rust
pub fn as_array(&self) -> Option<&[Value]>
```

#### Value::as_str

```rust
pub fn as_str(&self) -> Option<&str>
```

#### Value::as_i64

```rust
pub fn as_i64(&self) -> Option<i64>
```

Parse the number text as a signed integer.

#### Value::as_f64

```rust
pub fn as_f64(&self) -> Option<f64>
```

Parse the number text as a float.

#### Value::as_bool

```rust
pub fn as_bool(&self) -> Option<bool>
```

### JsonError

```rust
pub struct JsonError {
    pub pos: usize,
    pub msg: String,
}
```

JSON parse error with a byte offset into the source.

### object_map

```rust
pub fn object_map(v: &Value) -> Option<BTreeMap<String, Value>>
```

Convenience: collect an object into a key→value map (last key wins).
