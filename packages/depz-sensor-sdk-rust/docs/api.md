# API reference

Auto-generated from the crate's public surface (the `pub` items and
their `///` doc-comments across `src/**`) by `scripts/gen_api_md.py`.
Regenerate with `python3 scripts/gen_api_md.py` from the crate root.
Edit the doc-comments in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · [BNO086](bno086/api.md) · [BNO055](bno055/api.md).

## Contents

- **Discovery**: [`SensorType`](#sensortype), [`Mode`](#mode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name), [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`DEV_USB_VID`](#dev_usb_vid), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortEntry`](#portentry), [`order_ports`](#order_ports)
- **SR04**: [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L4CD (ToF)**: [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`I2C_KHZ_STEPS`](#i2c_khz_steps), [`i2c_error_name`](#i2c_error_name), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`Vl53l4Info`](#vl53l4info), [`StreamData`](#streamdata), [`SOFT_RESET`](#soft_reset), [`I2C_SLAVE__DEVICE_ADDRESS`](#i2c_slave__device_address), [`OSC_FREQUENCY`](#osc_frequency), [`VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`](#vhv_config__timeout_macrop_loop_bound), [`XTALK_PLANE_OFFSET_KCPS`](#xtalk_plane_offset_kcps), [`XTALK_X_PLANE_GRADIENT_KCPS`](#xtalk_x_plane_gradient_kcps), [`XTALK_Y_PLANE_GRADIENT_KCPS`](#xtalk_y_plane_gradient_kcps), [`RANGE_OFFSET_MM`](#range_offset_mm), [`INNER_OFFSET_MM`](#inner_offset_mm), [`OUTER_OFFSET_MM`](#outer_offset_mm), [`GPIO_HV_MUX__CTRL`](#gpio_hv_mux__ctrl), [`GPIO__TIO_HV_STATUS`](#gpio__tio_hv_status), [`SYSTEM__INTERRUPT`](#system__interrupt), [`RANGE_CONFIG_A`](#range_config_a), [`RANGE_CONFIG_B`](#range_config_b), [`RANGE_CONFIG__SIGMA_THRESH`](#range_config__sigma_thresh), [`MIN_COUNT_RATE_RTN_LIMIT_MCPS`](#min_count_rate_rtn_limit_mcps), [`INTERMEASUREMENT_MS`](#intermeasurement_ms), [`THRESH_HIGH`](#thresh_high), [`THRESH_LOW`](#thresh_low), [`SYSTEM__INTERRUPT_CLEAR`](#system__interrupt_clear), [`SYSTEM_START`](#system_start), [`RESULT__RANGE_STATUS`](#result__range_status), [`RESULT__SPAD_NB`](#result__spad_nb), [`RESULT__SIGNAL_RATE`](#result__signal_rate), [`RESULT__AMBIENT_RATE`](#result__ambient_rate), [`RESULT__SIGMA`](#result__sigma), [`RESULT__DISTANCE`](#result__distance), [`RESULT__OSC_CALIBRATE_VAL`](#result__osc_calibrate_val), [`FIRMWARE__SYSTEM_STATUS`](#firmware__system_status), [`IDENTIFICATION__MODEL_ID`](#identification__model_id), [`MODEL_ID_VL53L4CD`](#model_id_vl53l4cd), [`WINDOW_BELOW`](#window_below), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`CONFIG_ADDR`](#config_addr), [`CONFIG_END`](#config_end), [`DEFAULT_CONFIGURATION`](#default_configuration), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`config_block`](#config_block), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`I2C_KHZ_BOOT`](#i2c_khz_boot), [`I2C_KHZ_DEFAULT`](#i2c_khz_default), [`STATUS_RTN`](#status_rtn), [`range_status_name`](#range_status_name), [`Vl53l4Error`](#vl53l4error), [`Vl53l4Results`](#vl53l4results), [`parse_result_block`](#parse_result_block), [`range_timing_registers`](#range_timing_registers), [`decode_range_timing`](#decode_range_timing), [`offset_raw`](#offset_raw), [`decode_offset`](#decode_offset), [`xtalk_raw`](#xtalk_raw), [`decode_xtalk`](#decode_xtalk), [`signal_threshold_raw`](#signal_threshold_raw), [`decode_signal_threshold`](#decode_signal_threshold), [`sigma_threshold_raw`](#sigma_threshold_raw), [`decode_sigma_threshold`](#decode_sigma_threshold)
- **VL53L8 (ToF)**: [`DIST_MM`](#dist_mm), [`SIGNAL_PER_SPAD_KCPS`](#signal_per_spad_kcps), [`RANGE_SIGMA_MM`](#range_sigma_mm), [`AMBIENT_PER_SPAD_KCPS`](#ambient_per_spad_kcps), [`NB_TARGET_DETECTED`](#nb_target_detected), [`TAR_STATUS`](#tar_status), [`NB_SPADS_ENABLED`](#nb_spads_enabled), [`MOTION_INDICATOR`](#motion_indicator), [`NB_THRESHOLDS`](#nb_thresholds), [`POWER_MODE_SLEEP`](#power_mode_sleep), [`POWER_MODE_WAKEUP`](#power_mode_wakeup), [`POWER_MODE_DEEP_SLEEP`](#power_mode_deep_sleep), [`xtalk_margin_to_raw`](#xtalk_margin_to_raw), [`MotionConfig`](#motionconfig), [`default_motion_config`](#default_motion_config), [`DetectionThreshold`](#detectionthreshold), [`DetectionThresholdBlocks`](#detectionthresholdblocks), [`pack_detection_thresholds`](#pack_detection_thresholds), [`CnhDecodeConfig`](#cnhdecodeconfig), [`CnhAggregate`](#cnhaggregate), [`CnhData`](#cnhdata), [`CnhError`](#cnherror), [`decode_cnh`](#decode_cnh), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`READ_MAX_LEN`](#read_max_len), [`CHUNK_SIZE`](#chunk_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`NB_TARGET_PER_ZONE`](#nb_target_per_zone), [`CNH_DATA_IDX`](#cnh_data_idx), [`Variant`](#variant), [`Vl53l8Error`](#vl53l8error), [`Vl53l8Results`](#vl53l8results), [`swap_buffer`](#swap_buffer), [`parse_frame`](#parse_frame), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`STREAM_TOTAL_MAX`](#stream_total_max), [`FrameChunk`](#framechunk), [`unpack_frame_chunk`](#unpack_frame_chunk), [`CompletedFrame`](#completedframe), [`FrameReassembler`](#framereassembler)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`PIN_LPN_OFF`](#pin_lpn_off), [`PIN_LPN_ON`](#pin_lpn_on), [`PIN_I2C_RST`](#pin_i2c_rst), [`PIN_SOFT_CYCLE`](#pin_soft_cycle), [`READ_MAX_LEN`](#read_max_len), [`WRITE_MAX_LEN`](#write_max_len), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`INFO_SIZE`](#info_size), [`I2C_SPEED_STEPS_KHZ`](#i2c_speed_steps_khz), [`i2c_error_name`](#i2c_error_name), [`pack_pin_ctrl`](#pack_pin_ctrl), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`Vl53l7Info`](#vl53l7info), [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`Vl53l7Model`](#vl53l7model), [`resolve_model`](#resolve_model)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`CLEAR_STEPS_MAX`](#clear_steps_max), [`INFO_SIZE`](#info_size), [`pack_set_addr_width`](#pack_set_addr_width), [`pack_start_stream`](#pack_start_stream), [`Vl53lxInfo`](#vl53lxinfo), [`DIE_BLOCK_ADDR`](#die_block_addr), [`DIE_BLOCK_LEN`](#die_block_len), [`L0X_BLOCK_ADDR`](#l0x_block_addr), [`L0X_BLOCK_LEN`](#l0x_block_len), [`HISTOGRAM_BLOCK_ADDR`](#histogram_block_addr), [`HISTOGRAM_BLOCK_LEN`](#histogram_block_len), [`HISTOGRAM_BINS`](#histogram_bins), [`DieVariant`](#dievariant), [`ShortBlock`](#shortblock), [`DieResult`](#dieresult), [`decode_die_block`](#decode_die_block), [`L0xRaw`](#l0xraw), [`decode_l0x_raw`](#decode_l0x_raw), [`HistogramRaw`](#histogramraw), [`decode_histogram_raw`](#decode_histogram_raw), [`DriverKind`](#driverkind), [`DRIVER_KINDS`](#driver_kinds), [`BusParams`](#busparams), [`Product`](#product), [`PRODUCTS`](#products), [`product`](#product), [`product_from_board_name`](#product_from_board_name), [`Vl53lxClass`](#vl53lxclass), [`resolve_product`](#resolve_product), [`resolve_class`](#resolve_class)
- **BNO086 (IMU)**: [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`q_point`](#q_point), [`report_length`](#report_length), [`Report`](#report), [`Vector3Kind`](#vector3kind), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo), [`REPORT_COMMAND_RESPONSE`](#report_command_response), [`REPORT_COMMAND_REQUEST`](#report_command_request), [`REPORT_FRS_READ_RESPONSE`](#report_frs_read_response), [`REPORT_FRS_READ_REQUEST`](#report_frs_read_request), [`REPORT_FRS_WRITE_RESPONSE`](#report_frs_write_response), [`REPORT_FRS_WRITE_DATA`](#report_frs_write_data), [`REPORT_FRS_WRITE_REQUEST`](#report_frs_write_request), [`REPORT_PRODUCT_ID_RESPONSE`](#report_product_id_response), [`REPORT_PRODUCT_ID_REQUEST`](#report_product_id_request), [`REPORT_GET_FEATURE_RESPONSE`](#report_get_feature_response), [`REPORT_SET_FEATURE_COMMAND`](#report_set_feature_command), [`REPORT_GET_FEATURE_REQUEST`](#report_get_feature_request), [`Sh2Error`](#sh2error), [`build_set_feature`](#build_set_feature), [`build_get_feature_request`](#build_get_feature_request), [`build_product_id_request`](#build_product_id_request), [`build_command_request`](#build_command_request), [`build_frs_read_request`](#build_frs_read_request), [`build_frs_write_request`](#build_frs_write_request), [`build_frs_write_data`](#build_frs_write_data), [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ShtpHeader`](#shtpheader), [`pack_shtp_header`](#pack_shtp_header), [`unpack_shtp_header`](#unpack_shtp_header), [`ShtpCargo`](#shtpcargo), [`build_frame`](#build_frame), [`ShtpLayer`](#shtplayer)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`XFER_MAX`](#xfer_max), [`TRIGGER_TIMER`](#trigger_timer), [`TRIGGER_INT`](#trigger_int), [`INFO_SIZE`](#info_size), [`I2C_ADDR`](#i2c_addr), [`EXPECTED_CHIP_ID`](#expected_chip_id), [`EXPECTED_ACC_ID`](#expected_acc_id), [`EXPECTED_MAG_ID`](#expected_mag_id), [`EXPECTED_GYR_ID`](#expected_gyr_id), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_reset`](#pack_reset), [`pack_stop_stream`](#pack_stop_stream), [`pack_get_info`](#pack_get_info), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`StreamData`](#streamdata), [`Bno055Info`](#bno055info), [`REG_CHIP_ID`](#reg_chip_id), [`REG_PAGE_ID`](#reg_page_id), [`REG_ACC_DATA`](#reg_acc_data), [`REG_MAG_DATA`](#reg_mag_data), [`REG_GYR_DATA`](#reg_gyr_data), [`REG_EUL_DATA`](#reg_eul_data), [`REG_QUA_DATA`](#reg_qua_data), [`REG_LIA_DATA`](#reg_lia_data), [`REG_GRV_DATA`](#reg_grv_data), [`REG_TEMP`](#reg_temp), [`REG_CALIB_STAT`](#reg_calib_stat), [`REG_ST_RESULT`](#reg_st_result), [`REG_INT_STA`](#reg_int_sta), [`REG_SYS_CLK_STATUS`](#reg_sys_clk_status), [`REG_SYS_STATUS`](#reg_sys_status), [`REG_SYS_ERR`](#reg_sys_err), [`REG_UNIT_SEL`](#reg_unit_sel), [`REG_OPR_MODE`](#reg_opr_mode), [`REG_PWR_MODE`](#reg_pwr_mode), [`REG_SYS_TRIGGER`](#reg_sys_trigger), [`REG_TEMP_SOURCE`](#reg_temp_source), [`REG_AXIS_MAP_CONFIG`](#reg_axis_map_config), [`REG_AXIS_MAP_SIGN`](#reg_axis_map_sign), [`REG_SIC_MATRIX`](#reg_sic_matrix), [`REG_CALIB_PROFILE`](#reg_calib_profile), [`CALIB_PROFILE_LEN`](#calib_profile_len), [`REG1_ACC_CONFIG`](#reg1_acc_config), [`REG1_MAG_CONFIG`](#reg1_mag_config), [`REG1_GYR_CONFIG_0`](#reg1_gyr_config_0), [`REG1_GYR_CONFIG_1`](#reg1_gyr_config_1), [`REG1_ACC_SLEEP_CONFIG`](#reg1_acc_sleep_config), [`REG1_GYR_SLEEP_CONFIG`](#reg1_gyr_sleep_config), [`REG1_INT_MSK`](#reg1_int_msk), [`REG1_INT_EN`](#reg1_int_en), [`REG1_ACC_AM_THRES`](#reg1_acc_am_thres), [`REG1_GYR_AM_SET`](#reg1_gyr_am_set), [`REG1_UNIQUE_ID`](#reg1_unique_id), [`UNIQUE_ID_LEN`](#unique_id_len), [`FULL_BLOCK`](#full_block), [`QUAT_BLOCK`](#quat_block), [`SYS_TRIGGER_SELF_TEST`](#sys_trigger_self_test), [`SYS_TRIGGER_RST_SYS`](#sys_trigger_rst_sys), [`SYS_TRIGGER_RST_INT`](#sys_trigger_rst_int), [`SYS_TRIGGER_CLK_SEL`](#sys_trigger_clk_sel), [`INT_ACC_BSX_DRDY`](#int_acc_bsx_drdy), [`INT_MAG_DRDY`](#int_mag_drdy), [`INT_GYR_AM`](#int_gyr_am), [`INT_GYR_HIGH_RATE`](#int_gyr_high_rate), [`INT_GYR_DRDY`](#int_gyr_drdy), [`INT_ACC_HIGH_G`](#int_acc_high_g), [`INT_ACC_AM`](#int_acc_am), [`INT_ACC_NM`](#int_acc_nm), [`ST_ACC`](#st_acc), [`ST_MAG`](#st_mag), [`ST_GYR`](#st_gyr), [`ST_MCU`](#st_mcu), [`EXPECTED_SELF_TEST`](#expected_self_test), [`SYS_STATUS_BOOTING`](#sys_status_booting), [`OprMode`](#oprmode), [`PwrMode`](#pwrmode), [`TempSource`](#tempsource), [`UNIT_ACC_MG`](#unit_acc_mg), [`UNIT_GYR_RPS`](#unit_gyr_rps), [`UNIT_EUL_RAD`](#unit_eul_rad), [`UNIT_TEMP_F`](#unit_temp_f), [`UNIT_ORI_ANDROID`](#unit_ori_android), [`Units`](#units), [`MAG_LSB`](#mag_lsb), [`QUAT_LSB`](#quat_lsb), [`FUSION_ACCEL_LSB`](#fusion_accel_lsb), [`CalibStatus`](#calibstatus), [`CalibrationProfile`](#calibrationprofile), [`pack_sic_matrix`](#pack_sic_matrix), [`unpack_sic_matrix`](#unpack_sic_matrix), [`SIC_IDENTITY`](#sic_identity), [`AXIS_X`](#axis_x), [`AXIS_Y`](#axis_y), [`AXIS_Z`](#axis_z), [`AxisRemap`](#axisremap), [`PLACEMENTS`](#placements), [`ACC_RANGE_G`](#acc_range_g), [`ACC_BANDWIDTH_HZ`](#acc_bandwidth_hz), [`ACC_POWER_NAMES`](#acc_power_names), [`GYR_RANGE_DPS`](#gyr_range_dps), [`GYR_BANDWIDTH_HZ`](#gyr_bandwidth_hz), [`GYR_POWER_NAMES`](#gyr_power_names), [`MAG_RATE_HZ`](#mag_rate_hz), [`MAG_OPR_NAMES`](#mag_opr_names), [`MAG_POWER_NAMES`](#mag_power_names), [`AccelConfig`](#accelconfig), [`GyroConfig`](#gyroconfig), [`MagConfig`](#magconfig), [`RawBlock`](#rawblock), [`decode_block`](#decode_block)
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
    /// VL53L5CX / VL53L7CX / VL53L7CH (one `APP_VL53L7` firmware, contract 11).
    Vl53l7,
    /// VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX (one `APP_VL53L0_4` bridge,
    /// contract 12; the protocol spec names it `APP_VL53LX`).
    Vl53lx,
    /// BNO055 9-axis IMU (`APP_BNO055` register bridge, contract 13).
    Bno055,
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

### Vl53l8Cmd

```rust
pub enum Vl53l8Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
}
```

VL53L8 host→device command opcodes (0x34 is unused on VL53L8).

### Vl53l8Rpt

```rust
pub enum Vl53l8Rpt {
    RegData = 0x91,
    Vl53Frame = 0x93,
}
```

VL53L8 device→host report opcodes.

### READ_MAX_LEN

```rust
pub const READ_MAX_LEN: usize = 2295;
```

Largest `VL53_READ_REG` `len` on VL53L8: the MCU transport buffer (2304 B)
minus the 9-byte `RPT_VL53_REG_DATA` header.

### CHUNK_SIZE

```rust
pub const CHUNK_SIZE: usize = 2048;
```

Transfer size the VL53L8 host splits reads/writes at.

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

### pack_start_stream

```rust
pub fn pack_start_stream(frame_size: u16) -> [u8; 2]
```

`VL53_START_STREAM` payload: `frame_size u16`.

### RegData

```rust
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
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

CNH (compact network histogram) output block id — **CH-only**. The frame
decode copies this block's raw bytes into [`Vl53l8Results::cnh_raw`];
[`super::cnh::decode_cnh`] unpacks them into per-aggregate histograms.

### Variant

```rust
pub enum Variant {
    /// Base VL53L8CX (dev default), or any device on ULD 2.1.0 footer geometry.
    Cx,
    /// VL53L8CH (VL53LMZ 2.0.16 firmware): footer id at `size-4`.
    Ch,
    /// VL53L5CX / VL53L7CX / VL53L7CH (contract 11): footer id at `size-4`,
    /// and every per-zone array trimmed to the frame's resolution. On these
    /// parts the per-target blocks (index ≥ 0x6C90) keep 64 entries even in
    /// 4×4 — the sensor fills the first `resolution` and zero-pads the rest.
    /// The resolution is read from the frame itself: the ambient-rate block
    /// (0x54D0, index range sized to the resolution by the vl53lmz 2.0.16
    /// output-list rule) holds exactly one entry per zone.
    L7,
}
```

ToF silicon/firmware variant. Both the base **VL53L8CX** and the
**VL53L8CH** (CX + CNH + production PID 0xED40) share one results-frame
layout; only the frame-tail geometry differs for decoding, and that is all
this enum selects. The footer-id offset is `size-12` (ULD 2.1.0, selected by
[`Variant::Cx`]) or `size-4` (VL53LMZ 2.0.16, selected by [`Variant::Ch`]).

The geometry follows the firmware each part runs: the VL53L8CX firmware
embeds ULD 2.1.0 (`size-12`), the VL53L8CH firmware VL53LMZ 2.0.16
(`size-4`), so decode CH frames with [`Variant::Ch`].

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

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### Vl53l7Cmd

```rust
pub enum Vl53l7Cmd {
    PinCtrl = 0x34,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}
```

Host→device opcodes the L5/L7 bridge adds on top of the VL53L8 set.

### Vl53l7Rpt

```rust
pub enum Vl53l7Rpt {
    /// `RPT_VL53_INFO` — carries **no** echoed command byte.
    Info = 0x92,
}
```

Device→host reports the L5/L7 bridge adds on top of the VL53L8 set.

### PIN_LPN_OFF

```rust
pub const PIN_LPN_OFF: u8 = 0;
```

`VL53_PIN_CTRL` action: stop streaming, drive LPn low (sensor I2C
interface off, reads NACK). None of the actions is a true sensor reset:
after [`PIN_LPN_OFF`] or [`PIN_SOFT_CYCLE`] the host must re-run `init()`.

### PIN_LPN_ON

```rust
pub const PIN_LPN_ON: u8 = 1;
```

`VL53_PIN_CTRL` action: drive LPn high (power-up default).

### PIN_I2C_RST

```rust
pub const PIN_I2C_RST: u8 = 2;
```

`VL53_PIN_CTRL` action: pulse I2C_RST.

### PIN_SOFT_CYCLE

```rust
pub const PIN_SOFT_CYCLE: u8 = 3;
```

`VL53_PIN_CTRL` action: stop streaming, LPn low 1 ms, high, I2C_RST pulse,
then clear the I2C error counters.

### READ_MAX_LEN

```rust
pub const READ_MAX_LEN: usize = 1536;
```

`VL53_READ_REG` `len` ceiling (`VL53LMZ_READ_MAX`): 1..1536. Hosts MUST
split reads here — the VL53L8 split (2048) fails with `ERR_INVALID_PARAM`.

### WRITE_MAX_LEN

```rust
pub const WRITE_MAX_LEN: usize = 2048;
```

`VL53_WRITE_REG` data ceiling (`VL53LMZ_XFER_MAX`): 1..2048.

### STREAM_CHUNK_MAX

```rust
pub const STREAM_CHUNK_MAX: usize = 1536;
```

Bytes of frame data per `RPT_VL53_FRAME` chunk (VL53L8: 1528).

### INFO_SIZE

```rust
pub const INFO_SIZE: usize = 20;
```

`RPT_VL53_INFO` payload size.

### I2C_SPEED_STEPS_KHZ

```rust
pub const I2C_SPEED_STEPS_KHZ: [u16; 9] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];
```

Nominal SCL steps the firmware carries a timing for; others snap to the
nearest one.

### i2c_error_name

```rust
pub fn i2c_error_name(code: u8) -> &'static str
```

`last_i2c_error` code in [`Vl53l7Info`] → human-readable name.

### pack_pin_ctrl

```rust
pub fn pack_pin_ctrl(action: u8) -> [u8; 1]
```

`VL53_PIN_CTRL` payload: `action u8` ([`PIN_LPN_OFF`] .. [`PIN_SOFT_CYCLE`]).

### pack_set_i2c_speed

```rust
pub fn pack_set_i2c_speed(khz: u16) -> [u8; 2]
```

`VL53_SET_I2C_SPEED` payload: `khz u16` (little-endian).

### Vl53l7Info

```rust
pub struct Vl53l7Info {
    pub int_edges: u32,
    pub frames_dropped: u32,
    pub i2c_errors: u32,
    /// 0 OK, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR ([`i2c_error_name`]).
    pub last_i2c_error: u8,
    pub lpn_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
    pub frame_size: u16,
    pub streaming: bool,
}
```

A decoded `RPT_VL53_INFO` report — bridge state only (the sensor is never
probed). Counters run from power-up / `DEVICE_RESET`; `SOFT_CYCLE` clears
the I2C ones.

#### Vl53l7Info::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Vl53l7Info, CodecError>
```

Unpack `<IIIBBBHHB` (20 bytes, little-endian); shorter payloads are
rejected.

### MIN_RANGING_FREQUENCY_HZ

```rust
pub const MIN_RANGING_FREQUENCY_HZ: u32 = 1;
```

Unlike VL53L8 (≥ 2 Hz), L5/L7 range and stream at 1 Hz.

### Vl53l7Model

```rust
pub enum Vl53l7Model {
    Vl53l5cx,
    Vl53l7cx,
    Vl53l7ch,
}
```

The sensor class an `APP_VL53L7` board opens as.

#### Vl53l7Model::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase class name (matches the golden vectors).

#### Vl53l7Model::has_cnh

```rust
pub fn has_cnh(&self) -> bool
```

True for the CNH-capable class (VL53L7CH).

### resolve_model

```rust
pub fn resolve_model(usb_model: Option<&str>, device_name: &str) -> Vl53l7Model
```

Resolve the L5/L7 class (contract 11 §1, pinned by `vl53l7.json` `model`):
1. the production USB PID model (`usb_model`, e.g. from
   [`crate::usb_model_hint`]) when it names one of the three;
2. else the first `VL53L([57])(CX|CH)` in `GET_DEVICE_NAME`
   (a part without its own class, e.g. `VL53L5CH`, falls back to CX);
3. else [`Vl53l7Model::Vl53l7cx`] — its blob runs on every L5/L7 part.

## VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)

### Vl53lxCmd

```rust
pub enum Vl53lxCmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
    /// New in v2.00: register-address width, 1 or 2 bytes (sticky, 2 after a
    /// reset). Set it before the first register access of a session and never
    /// under a running stream.
    SetAddrWidth = 0x39,
    /// New in v2.01 (firmware v0.24), no payload: zero `i2c_errors` /
    /// `last_i2c_error`. The host sends it after every sensor init — a
    /// resetting die NACKs for a moment, and that is no bus fault.
    ClearI2cErrors = 0x3A,
}
```

VL53LX host→device command opcodes.

### Vl53lxRpt

```rust
pub enum Vl53lxRpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

VL53LX device→host report opcodes.

### CLEAR_STEPS_MAX

```rust
pub const CLEAR_STEPS_MAX: usize = 4;
```

Interrupt-release steps a stream may carry (`VL53_CLEAR_STEPS_WIRE_MAX`).

### INFO_SIZE

```rust
pub const INFO_SIZE: usize = 23;
```

`RPT_VL53_INFO` payload size (v2.00).

### pack_set_addr_width

```rust
pub fn pack_set_addr_width(width: u8) -> Result<[u8; 1], CodecError>
```

`VL53_SET_ADDR_WIDTH` payload: `width u8` — 1 (VL53L0X) or 2 (the rest).

### pack_start_stream

```rust
pub fn pack_start_stream(
    addr: u16,
    len: u16,
    clear: &[(u16, u8)],
    flags: u8,
) -> Result<Vec<u8>, CodecError>
```

`VL53_START_STREAM` payload (v2.00): `addr u16, len u16, flags u8,
n_clear u8, clear[n] {addr u16, value u8}` — `6 + 3n` bytes. `clear` is the
interrupt-release list the bridge plays after every block read (0..4
steps); more than [`CLEAR_STEPS_MAX`] is refused.

### Vl53lxInfo

```rust
pub struct Vl53lxInfo {
    pub int_edges: u32,
    pub slots_skipped: u32,
    pub i2c_errors: u32,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    /// ([`crate::vl53l4::i2c_error_name`]).
    pub last_i2c_error: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
    pub addr_width: u8,
    pub n_clear: u8,
    pub frames_dropped: u32,
}
```

A decoded `RPT_VL53_INFO` report (v2.00, 23 bytes) — bridge state only; the
bridge reads no sensor register. Counters are free-running and wrap
silently: watch increments. `slots_skipped` = a slot that never got the
bus, `i2c_errors` = a bus that answered badly, `frames_dropped` = a good
sample the USB TX ring had no room for (since the stream was armed).

#### Vl53lxInfo::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Vl53lxInfo, CodecError>
```

### DIE_BLOCK_ADDR

```rust
pub const DIE_BLOCK_ADDR: u16 = 0x0089;
```

Die result block address (`RESULT__RANGE_STATUS`).

### DIE_BLOCK_LEN

```rust
pub const DIE_BLOCK_LEN: usize = 17;
```

Die result block length.

### L0X_BLOCK_ADDR

```rust
pub const L0X_BLOCK_ADDR: u16 = 0x14;
```

VL53L0X result block address (8-bit register space).

### L0X_BLOCK_LEN

```rust
pub const L0X_BLOCK_LEN: usize = 12;
```

VL53L0X result block length.

### HISTOGRAM_BLOCK_ADDR

```rust
pub const HISTOGRAM_BLOCK_ADDR: u16 = 0x0088;
```

Histogram block address (`result__interrupt_status`).

### HISTOGRAM_BLOCK_LEN

```rust
pub const HISTOGRAM_BLOCK_LEN: usize = 83;
```

Histogram block length (0x0088..=0x00DA).

### HISTOGRAM_BINS

```rust
pub const HISTOGRAM_BINS: usize = 24;
```

Photon bins per histogram block.

### DieVariant

```rust
pub enum DieVariant {
    /// VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD: `(5, 256)`.
    L4,
    /// VL53L1X ULD — crosstalk-corrected peak signal at 0x0098: `(15, 25)`.
    L1,
}
```

Which ULD reads the die block: `(signal-rate byte offset, per-SPAD K)`.

#### DieVariant::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase name (`"l4"` / `"l1"`, matches the golden vectors).

#### DieVariant::from_name

```rust
pub fn from_name(s: &str) -> Option<DieVariant>
```

#### DieVariant::params

```rust
pub fn params(&self) -> (usize, u32)
```

`(signal-rate byte offset, per-SPAD scale K)`.

### ShortBlock

```rust
pub struct ShortBlock {
    pub need: usize,
    pub got: usize,
}
```

A block shorter than its decoder needs.

### DieResult

```rust
pub struct DieResult {
    /// ULD status via `STATUS_RTN` (0 = valid).
    pub range_status: u8,
    pub distance_mm: u32,
    pub sigma_mm: u32,
    pub signal_rate_kcps: u32,
    pub ambient_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub number_of_spad: u32,
    pub stream_count: u8,
}
```

The die block decoded as a ULD reads it.

### decode_die_block

```rust
pub fn decode_die_block(raw: &[u8], variant: DieVariant) -> Result<DieResult, ShortBlock>
```

The 17-byte die block (0x0089..0x0099) as the named ULD reads it.

### L0xRaw

```rust
pub struct L0xRaw {
    /// mm (quarter-mm when RangeFractionalEnable, off by default).
    pub distance_raw: u32,
    /// Raw byte 0; the PAL status needs the init state.
    pub device_range_status: u8,
    /// FixPoint16.16 Mcps (9.7 on the wire `<< 9`).
    pub signal_rate_mcps_1616: u32,
    pub ambient_rate_mcps_1616: u32,
    /// 8.8.
    pub effective_spad_count_88: u32,
}
```

Raw fields of the VL53L0X result block.

### decode_l0x_raw

```rust
pub fn decode_l0x_raw(raw: &[u8]) -> Result<L0xRaw, ShortBlock>
```

Raw fields of the VL53L0X block at 0x14 (`VL53L0X_GetRangingMeasurementData`
before the PAL status/sigma step).

### HistogramRaw

```rust
pub struct HistogramRaw {
    pub interrupt_status: u8,
    pub range_status: u8,
    pub report_status: u8,
    pub stream_count: u8,
    pub dss_actual_effective_spads: u32,
    pub reference_phase: u32,
    pub vcsel_start: u8,
    /// 24 photon counts.
    pub bins: [u32; HISTOGRAM_BINS],
}
```

Status bytes and bins of the histogram block.

### decode_histogram_raw

```rust
pub fn decode_histogram_raw(raw: &[u8]) -> Result<HistogramRaw, ShortBlock>
```

The 83-byte histogram block at 0x0088: status bytes and the 24 bins
(bin 23's low byte is carried in a separate MSB/LSB pair and patched in as
`(MSB << 2) + LSB` before the bins are read).

### DriverKind

```rust
pub enum DriverKind {
    /// Ultra Lite Driver — the die computes the distance.
    Uld,
    /// Ultra Low Power (VL53L3CX only).
    Ulp,
    /// ST's Bare Driver — the die hands over 24 photon bins, the host finds
    /// up to four targets.
    Histogram,
}
```

The three kinds of driver ST ships for this family, in UI order.

#### DriverKind::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Lowercase wire-string form (matches the golden vectors).

#### DriverKind::from_name

```rust
pub fn from_name(s: &str) -> Option<DriverKind>
```

### DRIVER_KINDS

```rust
pub const DRIVER_KINDS: [DriverKind; 3] = [DriverKind::Uld, DriverKind::Ulp, DriverKind::Histogram];
```

Every driver kind, in UI order.

### BusParams

```rust
pub struct BusParams {
    /// `VL53_SET_ADDR_WIDTH` value (1 on VL53L0X, else 2).
    pub addr_width: u8,
    /// Interrupt-release steps handed to `VL53_START_STREAM`.
    pub clear_steps: &'static [(u16, u8)],
    /// Bus ceiling the driver raises the bridge to after the 400 kHz init.
    pub max_khz: u16,
    /// Streamed block address and length.
    pub block_addr: u16,
    pub block_len: u16,
    /// How the die block is read (`None` for the L0X and histogram blocks).
    pub die_variant: Option<DieVariant>,
}
```

The bridge parameters one product/driver pair runs with (contract 12 §1, §3).

### Product

```rust
pub struct Product {
    /// Part number, e.g. `"VL53L4CD"`.
    pub name: &'static str,
    /// Production USB PID.
    pub usb_pid: u16,
    /// Expected model id (cross-check only).
    pub model_id: u16,
    /// Datasheet rated reach, mm.
    pub reach_mm: u32,
    /// The driver kind a board of this product opens with.
    pub default_driver: DriverKind,
    drivers: &'static [(DriverKind, BusParams)],
}
```

One row of the product table.

#### Product::driver_kinds

```rust
pub fn driver_kinds(&self) -> Vec<DriverKind>
```

The driver kinds this product has, in [`DRIVER_KINDS`] order.

#### Product::bus_params

```rust
pub fn bus_params(&self, kind: DriverKind) -> Option<BusParams>
```

Bridge parameters of one product/driver pair; `None` for a pair the
table has no row for (a refusal, never a fallback).

#### Product::default_bus_params

```rust
pub fn default_bus_params(&self) -> BusParams
```

Bridge parameters of [`Product::default_driver`].

#### Product::model_id_ok

```rust
pub fn model_id_ok(&self, value: u16) -> bool
```

Cross-check a model id the sensor answered. `true` only says "not
something else entirely": the pairs that share an id can't be told apart.

### PRODUCTS

```rust
pub const PRODUCTS: [Product; 6] = [
    Product {
        name: "VL53L0X",
        usb_pid: 0xED41,
        model_id: 0x00EE,
        reach_mm: 2000,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, L0X_ULD)],
    },
    Product {
        name: "VL53L1CX",
        usb_pid: 0xED43,
        model_id: 0xEACC,
        reach_mm: 4000,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, die(DieVariant::L1)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L1CB",
        usb_pid: 0xED42,
        model_id: 0xEACC,
        reach_mm: 8000,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, die(DieVariant::L1)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L3CX",
        usb_pid: 0xED44,
        model_id: 0xEAAA,
        reach_mm: 3000,
        default_driver: DriverKind::Ulp,
        drivers: &[(DriverKind::Ulp, die(DieVariant::L4)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L4CD",
        usb_pid: 0xED45,
        model_id: 0xEBAA,
        reach_mm: 1300,
        default_driver: DriverKind::Uld,
        drivers: &[(DriverKind::Uld, die(DieVariant::L4)), (DriverKind::Histogram, HISTOGRAM)],
    },
    Product {
        name: "VL53L4CX",
        usb_pid: 0xED46,
        model_id: 0xEBAA,
        reach_mm: 6000,
        default_driver: DriverKind::Histogram,
        drivers: &[(DriverKind::Histogram, HISTOGRAM)],
    },
];
```

Every product of the family, in UI order.

### product

```rust
pub fn product(name: &str) -> Option<&'static Product>
```

Table row by part number (case-sensitive, uppercase), `None` if unserved.

### product_from_board_name

```rust
pub fn product_from_board_name(name: &str) -> Option<&'static str>
```

`"ToF Sensor VL53L4CD USB v2.1"` → `"VL53L4CD"`. Matches the first
`VL53L<digit>[A-Z0-9]*` in the upper-cased name; `None` when there is none
or it is not a family product (an unstamped board, or an unknown name).

### Vl53lxClass

```rust
pub enum Vl53lxClass {
    Vl53l0x,
    Vl53l1cx,
    Vl53l1cb,
    Vl53l3cx,
    Vl53l4cx,
    /// The generic class: takes the product at init (VL53L4CD boards on this
    /// firmware land here too — the `vl53l4cd` class belongs to `APP_VL53L4`).
    Vl53lx,
}
```

The sensor class an `APP_VL53L0_4` board opens as.

#### Vl53lxClass::as_str

```rust
pub fn as_str(&self) -> &'static str
```

Class name (matches the golden vectors).

### resolve_product

```rust
pub fn resolve_product(usb_model: Option<&str>, device_name: &str) -> Option<&'static str>
```

The product a board carries: the production USB PID model (`usb_model`,
e.g. from [`crate::usb_model_hint`]) when it names a family product, else
the device-name product ([`product_from_board_name`]), else `None`.

### resolve_class

```rust
pub fn resolve_class(usb_model: Option<&str>, device_name: &str) -> Vl53lxClass
```

Resolve the 1D-family class (contract 12 §1, pinned by `vl53lx.json`
`model`): PID model if it is a family product, else the device-name
product, else the generic [`Vl53lxClass::Vl53lx`].

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

## BNO055 (IMU)

### Bno055Cmd

```rust
pub enum Bno055Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    /// Pulses nRESET and answers after the chip-ID handshake (~0.5 s; allow
    /// ≥ 1.5 s). Stops any stream.
    Reset = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}
```

BNO055 host→device command opcodes.

### Bno055Rpt

```rust
pub enum Bno055Rpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}
```

BNO055 device→host report opcodes.

### XFER_MAX

```rust
pub const XFER_MAX: usize = 128;
```

Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100.

### TRIGGER_TIMER

```rust
pub const TRIGGER_TIMER: u8 = 0;
```

`BNO_START_STREAM` trigger: read every `period_ms` (1..60000) — the only
data trigger on sensor SW rev 03.11 (no data-ready interrupt there).

### TRIGGER_INT

```rust
pub const TRIGGER_INT: u8 = 1;
```

`BNO_START_STREAM` trigger: read on each INT rising edge; `period_ms` is a
missed-edge watchdog, 0 disables it. For motion interrupts.

### INFO_SIZE

```rust
pub const INFO_SIZE: usize = 38;
```

`RPT_BNO_INFO` payload size.

### I2C_ADDR

```rust
pub const I2C_ADDR: u8 = 0x28;
```

Sensor 7-bit I2C address (SA0 held low).

### EXPECTED_CHIP_ID

```rust
pub const EXPECTED_CHIP_ID: u8 = 0xA0;
```

Identity registers 0x00..0x03 of a healthy BNO055.

### EXPECTED_ACC_ID

```rust
pub const EXPECTED_ACC_ID: u8 = 0xFB;
```

### EXPECTED_MAG_ID

```rust
pub const EXPECTED_MAG_ID: u8 = 0x32;
```

### EXPECTED_GYR_ID

```rust
pub const EXPECTED_GYR_ID: u8 = 0x0F;
```

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u8, len: u8) -> [u8; 2]
```

`BNO_READ_REG` payload: `addr u8, len u8`.

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u8, data: &[u8]) -> Vec<u8>
```

`BNO_WRITE_REG` payload: `addr u8, data[1..128]`.

### pack_reset

```rust
pub fn pack_reset() -> [u8; 0]
```

`BNO_RESET` payload (empty).

### pack_stop_stream

```rust
pub fn pack_stop_stream() -> [u8; 0]
```

`BNO_STOP_STREAM` payload (empty).

### pack_get_info

```rust
pub fn pack_get_info() -> [u8; 0]
```

`BNO_GET_INFO` payload (empty).

### pack_start_stream

```rust
pub fn pack_start_stream(trigger: u8, addr: u8, len: u8, period_ms: u16) -> [u8; 5]
```

`BNO_START_STREAM` payload: `trigger u8, addr u8, len u8, period_ms u16 LE`
— 5 bytes. Replaces any running stream.

### RegData

```rust
pub struct RegData {
    /// Echoed `BNO_READ_REG` opcode (0x32).
    pub cmd: u8,
    /// MCU uptime at I2C-read completion.
    pub timestamp_us: u64,
    /// Raw register bytes (little-endian sensor contents, passed through).
    pub data: Vec<u8>,
}
```

A decoded `RPT_BNO_REG_DATA` report: the reply to `BNO_READ_REG`.

#### RegData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError>
```

### StreamData

```rust
pub struct StreamData {
    /// MCU uptime at the trigger (timer expiry or INT edge), not the I2C
    /// completion. Dropped samples show as jumps.
    pub timestamp_us: u64,
    pub addr: u8,
    pub len: u8,
    pub data: Vec<u8>,
}
```

A decoded `RPT_BNO_REG_STREAM` report — one streamed register block.
`addr` / `len` echo the stream configuration, so each report is
self-describing ([`crate::bno055::decode_block`] takes them as is).

#### StreamData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<StreamData, CodecError>
```

### Bno055Info

```rust
pub struct Bno055Info {
    /// 7-bit sensor address in use (0x28).
    pub i2c_addr: u8,
    pub chip_id: u8,
    pub acc_id: u8,
    pub mag_id: u8,
    pub gyr_id: u8,
    /// Sensor firmware, BCD: 0x0311 = 03.11.
    pub sw_rev: u16,
    pub bl_rev: u8,
    /// 1 = chip-ID handshake passed.
    pub initialized: u8,
    /// Current INT pin level.
    pub int_level: u8,
    /// INT rising edges (counted only while an INT stream is armed).
    pub int_edges: u32,
    /// Streamed block read time since the last START_STREAM.
    pub read_min_us: u16,
    pub read_max_us: u16,
    pub read_avg_us: u16,
    /// Packets refused by a full USB TX ring.
    pub tx_dropped: u32,
    pub i2c_errors: u32,
    /// Stream slots dropped because the bus was still busy (since START_STREAM).
    pub slots_skipped: u32,
    pub bus_recoveries: u16,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    /// ([`crate::vl53l4::i2c_error_name`]).
    pub last_i2c_error: u8,
    /// nRESET recoveries — the sensor came back in CONFIG each time.
    pub sensor_resets: u8,
    /// Longest main-loop iteration since START_STREAM.
    pub loop_max_us: u16,
}
```

A decoded `RPT_BNO_INFO` report (38 bytes) — sensor identity (registers
0x00..0x06) plus bridge diagnostics. Counters are free-running and wrap
silently: watch increments. A rising `sensor_resets` means the bridge
pulsed nRESET to recover the bus — the sensor is back in CONFIG and the
host must re-apply its configuration.

#### Bno055Info::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Bno055Info, CodecError>
```

#### Bno055Info::ids_ok

```rust
pub fn ids_ok(&self) -> bool
```

All four identity registers hold the healthy values.

#### Bno055Info::sw_rev_text

```rust
pub fn sw_rev_text(&self) -> String
```

Sensor firmware revision as Bosch writes it: 0x0311 → `"03.11"`.

### REG_CHIP_ID

```rust
pub const REG_CHIP_ID: u8 = 0x00;
```

### REG_PAGE_ID

```rust
pub const REG_PAGE_ID: u8 = 0x07;
```

Page select — owned by the host (the MCU never touches it); always return
to page 0, and never switch pages under a running stream.

### REG_ACC_DATA

```rust
pub const REG_ACC_DATA: u8 = 0x08;
```

ACC_DATA: x, y, z i16 LE.

### REG_MAG_DATA

```rust
pub const REG_MAG_DATA: u8 = 0x0E;
```

### REG_GYR_DATA

```rust
pub const REG_GYR_DATA: u8 = 0x14;
```

### REG_EUL_DATA

```rust
pub const REG_EUL_DATA: u8 = 0x1A;
```

EUL_DATA: heading, roll, pitch.

### REG_QUA_DATA

```rust
pub const REG_QUA_DATA: u8 = 0x20;
```

QUA_DATA: w, x, y, z.

### REG_LIA_DATA

```rust
pub const REG_LIA_DATA: u8 = 0x28;
```

Linear acceleration (gravity removed).

### REG_GRV_DATA

```rust
pub const REG_GRV_DATA: u8 = 0x2E;
```

Gravity vector.

### REG_TEMP

```rust
pub const REG_TEMP: u8 = 0x34;
```

TEMP: i8.

### REG_CALIB_STAT

```rust
pub const REG_CALIB_STAT: u8 = 0x35;
```

### REG_ST_RESULT

```rust
pub const REG_ST_RESULT: u8 = 0x36;
```

### REG_INT_STA

```rust
pub const REG_INT_STA: u8 = 0x37;
```

Clears on read — never part of a routine block read.

### REG_SYS_CLK_STATUS

```rust
pub const REG_SYS_CLK_STATUS: u8 = 0x38;
```

### REG_SYS_STATUS

```rust
pub const REG_SYS_STATUS: u8 = 0x39;
```

### REG_SYS_ERR

```rust
pub const REG_SYS_ERR: u8 = 0x3A;
```

### REG_UNIT_SEL

```rust
pub const REG_UNIT_SEL: u8 = 0x3B;
```

### REG_OPR_MODE

```rust
pub const REG_OPR_MODE: u8 = 0x3D;
```

OPR_MODE bits 3:0 (reads back 0x10 after reset — mask).

### REG_PWR_MODE

```rust
pub const REG_PWR_MODE: u8 = 0x3E;
```

### REG_SYS_TRIGGER

```rust
pub const REG_SYS_TRIGGER: u8 = 0x3F;
```

`CLK_SEL 7, RST_INT 6, RST_SYS 5, SELF_TEST 0` — preserve bit 7.

### REG_TEMP_SOURCE

```rust
pub const REG_TEMP_SOURCE: u8 = 0x40;
```

### REG_AXIS_MAP_CONFIG

```rust
pub const REG_AXIS_MAP_CONFIG: u8 = 0x41;
```

### REG_AXIS_MAP_SIGN

```rust
pub const REG_AXIS_MAP_SIGN: u8 = 0x42;
```

### REG_SIC_MATRIX

```rust
pub const REG_SIC_MATRIX: u8 = 0x43;
```

Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384.

### REG_CALIB_PROFILE

```rust
pub const REG_CALIB_PROFILE: u8 = 0x55;
```

Calibration profile, [`CALIB_PROFILE_LEN`] bytes (CONFIG mode only).

### CALIB_PROFILE_LEN

```rust
pub const CALIB_PROFILE_LEN: usize = 22;
```

### REG1_ACC_CONFIG

```rust
pub const REG1_ACC_CONFIG: u8 = 0x08;
```

### REG1_MAG_CONFIG

```rust
pub const REG1_MAG_CONFIG: u8 = 0x09;
```

### REG1_GYR_CONFIG_0

```rust
pub const REG1_GYR_CONFIG_0: u8 = 0x0A;
```

### REG1_GYR_CONFIG_1

```rust
pub const REG1_GYR_CONFIG_1: u8 = 0x0B;
```

### REG1_ACC_SLEEP_CONFIG

```rust
pub const REG1_ACC_SLEEP_CONFIG: u8 = 0x0C;
```

### REG1_GYR_SLEEP_CONFIG

```rust
pub const REG1_GYR_SLEEP_CONFIG: u8 = 0x0D;
```

### REG1_INT_MSK

```rust
pub const REG1_INT_MSK: u8 = 0x0F;
```

### REG1_INT_EN

```rust
pub const REG1_INT_EN: u8 = 0x10;
```

### REG1_ACC_AM_THRES

```rust
pub const REG1_ACC_AM_THRES: u8 = 0x11;
```

First of the motion-interrupt settings 0x11..=0x1F (written raw).

### REG1_GYR_AM_SET

```rust
pub const REG1_GYR_AM_SET: u8 = 0x1F;
```

Last of the motion-interrupt settings.

### REG1_UNIQUE_ID

```rust
pub const REG1_UNIQUE_ID: u8 = 0x50;
```

### UNIQUE_ID_LEN

```rust
pub const UNIQUE_ID_LEN: usize = 16;
```

### FULL_BLOCK

```rust
pub const FULL_BLOCK: (u8, u8) = (REG_ACC_DATA, REG_CALIB_STAT - REG_ACC_DATA + 1);
```

The one block that carries every output channel: `(addr, len)` = 0x08
(ACC_DATA_X_LSB) through 0x35 (CALIB_STAT), 46 bytes.

### QUAT_BLOCK

```rust
pub const QUAT_BLOCK: (u8, u8) = (REG_QUA_DATA, 8);
```

Quaternion only — the cheapest orientation read (8 bytes).

### SYS_TRIGGER_SELF_TEST

```rust
pub const SYS_TRIGGER_SELF_TEST: u8 = 0x01;
```

### SYS_TRIGGER_RST_SYS

```rust
pub const SYS_TRIGGER_RST_SYS: u8 = 0x20;
```

### SYS_TRIGGER_RST_INT

```rust
pub const SYS_TRIGGER_RST_INT: u8 = 0x40;
```

### SYS_TRIGGER_CLK_SEL

```rust
pub const SYS_TRIGGER_CLK_SEL: u8 = 0x80;
```

### INT_ACC_BSX_DRDY

```rust
pub const INT_ACC_BSX_DRDY: u8 = 0x01;
```

### INT_MAG_DRDY

```rust
pub const INT_MAG_DRDY: u8 = 0x02;
```

### INT_GYR_AM

```rust
pub const INT_GYR_AM: u8 = 0x04;
```

### INT_GYR_HIGH_RATE

```rust
pub const INT_GYR_HIGH_RATE: u8 = 0x08;
```

### INT_GYR_DRDY

```rust
pub const INT_GYR_DRDY: u8 = 0x10;
```

### INT_ACC_HIGH_G

```rust
pub const INT_ACC_HIGH_G: u8 = 0x20;
```

### INT_ACC_AM

```rust
pub const INT_ACC_AM: u8 = 0x40;
```

### INT_ACC_NM

```rust
pub const INT_ACC_NM: u8 = 0x80;
```

### ST_ACC

```rust
pub const ST_ACC: u8 = 0x01;
```

### ST_MAG

```rust
pub const ST_MAG: u8 = 0x02;
```

### ST_GYR

```rust
pub const ST_GYR: u8 = 0x04;
```

### ST_MCU

```rust
pub const ST_MCU: u8 = 0x08;
```

### EXPECTED_SELF_TEST

```rust
pub const EXPECTED_SELF_TEST: u8 = ST_ACC | ST_MAG | ST_GYR | ST_MCU;
```

### SYS_STATUS_BOOTING

```rust
pub const SYS_STATUS_BOOTING: [u8; 3] = [2, 3, 4];
```

SYS_STATUS values of a sensor still booting after BNO_RESET: poll until it
leaves these before configuring (contract 13 §5, ERRATA E13).

### OprMode

```rust
pub enum OprMode {
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
}
```

OPR_MODE (0x3D) bits 3:0.

#### OprMode::from_reg

```rust
pub fn from_reg(value: u8) -> Option<OprMode>
```

From an OPR_MODE register value (bits 7:4 masked off); `None` for the
unused codes 13..15.

#### OprMode::is_fusion

```rust
pub fn is_fusion(&self) -> bool
```

Fusion modes (IMU and up) run Bosch's on-chip sensor fusion.

### PwrMode

```rust
pub enum PwrMode {
    Normal = 0x00,
    LowPower = 0x01,
    Suspend = 0x02,
}
```

PWR_MODE (0x3E) bits 1:0.

### TempSource

```rust
pub enum TempSource {
    Accel = 0x00,
    Gyro = 0x01,
}
```

TEMP_SOURCE (0x40) bits 1:0.

### UNIT_ACC_MG

```rust
pub const UNIT_ACC_MG: u8 = 0x01;
```

### UNIT_GYR_RPS

```rust
pub const UNIT_GYR_RPS: u8 = 0x02;
```

### UNIT_EUL_RAD

```rust
pub const UNIT_EUL_RAD: u8 = 0x04;
```

### UNIT_TEMP_F

```rust
pub const UNIT_TEMP_F: u8 = 0x10;
```

### UNIT_ORI_ANDROID

```rust
pub const UNIT_ORI_ANDROID: u8 = 0x80;
```

### Units

```rust
pub struct Units {
    /// ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²).
    pub accel_mg: bool,
    /// Angular rate in rad/s, else deg/s.
    pub gyro_rps: bool,
    /// Euler angles in radians, else degrees.
    pub euler_rad: bool,
    /// Temperature in °F, else °C.
    pub temp_f: bool,
    /// Android pitch convention, else Windows.
    pub android: bool,
}
```

Output units. `Units::default()` is the SDK default UNIT_SEL = 0x00 (m/s²,
dps, degrees, °C, Windows orientation). The sensor's own power-on value is
0x80 (Android), so hosts write UNIT_SEL explicitly. A stream is scaled by
the units latched when it was armed.

#### Units::pack

```rust
pub fn pack(&self) -> u8
```

→ UNIT_SEL. Undefined bits are never set.

#### Units::unpack

```rust
pub fn unpack(value: u8) -> Units
```

From UNIT_SEL; the bits that do nothing are ignored.

#### Units::accel_lsb

```rust
pub fn accel_lsb(&self) -> f64
```

ACC_DATA LSB per unit: m/s² 100, mg 1.

#### Units::gyro_lsb

```rust
pub fn gyro_lsb(&self) -> f64
```

Angular-rate LSB per unit: dps 16, rps 900.

#### Units::euler_lsb

```rust
pub fn euler_lsb(&self) -> f64
```

Euler LSB per unit: degrees 16, radians 900.

#### Units::temp_lsb

```rust
pub fn temp_lsb(&self) -> f64
```

Temperature LSB per unit: °C 1, °F 0.5 (1 LSB = 2 °F, measured).

### MAG_LSB

```rust
pub const MAG_LSB: f64 = 16.0;
```

MAG_DATA: 16 LSB per µT, not selectable.

### QUAT_LSB

```rust
pub const QUAT_LSB: f64 = 16384.0;
```

Quaternion: 2¹⁴ LSB, unit-less.

### FUSION_ACCEL_LSB

```rust
pub const FUSION_ACCEL_LSB: f64 = 100.0;
```

Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at
100 LSB (measured on SW 03.11; datasheet Tables 3-33/3-35 claim mg).

### CalibStatus

```rust
pub struct CalibStatus {
    pub system: u8,
    pub gyro: u8,
    pub accel: u8,
    pub mag: u8,
}
```

CALIB_STAT (0x35) `sys<7:6> gyr<5:4> acc<3:2> mag<1:0>`: 0 = not
calibrated … 3 = fully calibrated.

#### CalibStatus::unpack

```rust
pub fn unpack(value: u8) -> CalibStatus
```

#### CalibStatus::pack

```rust
pub fn pack(&self) -> u8
```

#### CalibStatus::fully_calibrated

```rust
pub fn fully_calibrated(&self) -> bool
```

3/3/3/3.

### CalibrationProfile

```rust
pub struct CalibrationProfile {
    pub accel_offset: [i16; 3],
    pub mag_offset: [i16; 3],
    pub gyro_offset: [i16; 3],
    pub accel_radius: i16,
    pub mag_radius: i16,
}
```

Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE), in
the sensor's LSB — independent of UNIT_SEL once written back. Readable and
writable only in CONFIG; write all 22 bytes in one transfer (the sensor
latches each group on its MSB). A written profile is a starting point: the
sensor keeps calibrating in fusion modes.

#### CalibrationProfile::pack

```rust
pub fn pack(&self) -> [u8; CALIB_PROFILE_LEN]
```

#### CalibrationProfile::unpack

```rust
pub fn unpack(data: &[u8]) -> Result<CalibrationProfile, CodecError>
```

Exactly [`CALIB_PROFILE_LEN`] bytes, else an error.

### pack_sic_matrix

```rust
pub fn pack_sic_matrix(m: &[i16; 9]) -> [u8; 18]
```

Soft-iron matrix (SIC_MATRIX 0x43), 9 × i16 row-major, 1.0 = 16384.

### unpack_sic_matrix

```rust
pub fn unpack_sic_matrix(data: &[u8]) -> Result<[i16; 9], CodecError>
```

Inverse of [`pack_sic_matrix`]; needs at least 18 bytes.

### SIC_IDENTITY

```rust
pub const SIC_IDENTITY: [i16; 9] = [16384, 0, 0, 0, 16384, 0, 0, 0, 16384];
```

The identity soft-iron matrix.

### AXIS_X

```rust
pub const AXIS_X: u8 = 0;
```

### AXIS_Y

```rust
pub const AXIS_Y: u8 = 1;
```

### AXIS_Z

```rust
pub const AXIS_Z: u8 = 2;
```

### AxisRemap

```rust
pub struct AxisRemap {
    pub x: u8,
    pub y: u8,
    pub z: u8,
    pub x_negative: bool,
    pub y_negative: bool,
    pub z_negative: bool,
}
```

Which chip axis feeds each output axis, and its sign. `x = AXIS_Y` means
"output X is the chip's Y axis". The sensor silently keeps its old value
when given a mapping that uses one axis twice, so [`AxisRemap::pack`]
refuses it. `AxisRemap::default()` is placement P1 (identity).

#### AxisRemap::pack

```rust
pub fn pack(&self) -> Result<(u8, u8), CodecError>
```

→ `(AXIS_MAP_CONFIG, AXIS_MAP_SIGN)`: config `z<5:4> y<3:2> x<1:0>`,
sign `x 2, y 1, z 0` (1 = negative). Refuses a non-permutation of
X/Y/Z.

#### AxisRemap::unpack

```rust
pub fn unpack(config: u8, sign: u8) -> AxisRemap
```

#### AxisRemap::placement

```rust
pub fn placement(name: &str) -> Option<AxisRemap>
```

Datasheet §3.4 mounting presets `P0`..`P7` (case-insensitive; P1 is the
default). `None` for an unknown name.

### PLACEMENTS

```rust
pub const PLACEMENTS: [(&str, u8, u8); 8] = [
    ("P0", 0x21, 0x04),
    ("P1", 0x24, 0x00),
    ("P2", 0x24, 0x06),
    ("P3", 0x21, 0x02),
    ("P4", 0x24, 0x03),
    ("P5", 0x21, 0x01),
    ("P6", 0x21, 0x07),
    ("P7", 0x24, 0x05),
];
```

Datasheet §3.4: `(placement, AXIS_MAP_CONFIG, AXIS_MAP_SIGN)`.

### ACC_RANGE_G

```rust
pub const ACC_RANGE_G: [u8; 4] = [2, 4, 8, 16];
```

Accelerometer range per `AccelConfig::range` code, g.

### ACC_BANDWIDTH_HZ

```rust
pub const ACC_BANDWIDTH_HZ: [f64; 8] = [7.81, 15.63, 31.25, 62.5, 125.0, 250.0, 500.0, 1000.0];
```

Accelerometer bandwidth per `AccelConfig::bandwidth` code, Hz.

### ACC_POWER_NAMES

```rust
pub const ACC_POWER_NAMES: [&str; 6] =
    ["normal", "suspend", "low power 1", "standby", "low power 2", "deep suspend"];
```

Accelerometer power mode per `AccelConfig::power` code.

### GYR_RANGE_DPS

```rust
pub const GYR_RANGE_DPS: [u16; 5] = [2000, 1000, 500, 250, 125];
```

Gyroscope range per `GyroConfig::range` code, dps.

### GYR_BANDWIDTH_HZ

```rust
pub const GYR_BANDWIDTH_HZ: [u16; 8] = [523, 230, 116, 47, 23, 12, 64, 32];
```

Gyroscope bandwidth per `GyroConfig::bandwidth` code, Hz.

### GYR_POWER_NAMES

```rust
pub const GYR_POWER_NAMES: [&str; 5] =
    ["normal", "fast power up", "deep suspend", "suspend", "advanced powersave"];
```

Gyroscope power mode per `GyroConfig::power` code.

### MAG_RATE_HZ

```rust
pub const MAG_RATE_HZ: [u8; 8] = [2, 6, 8, 10, 15, 20, 25, 30];
```

Magnetometer output rate per `MagConfig::rate` code, Hz.

### MAG_OPR_NAMES

```rust
pub const MAG_OPR_NAMES: [&str; 4] = ["low power", "regular", "enhanced regular", "high accuracy"];
```

Magnetometer operation mode per `MagConfig::mode` code.

### MAG_POWER_NAMES

```rust
pub const MAG_POWER_NAMES: [&str; 4] = ["normal", "sleep", "suspend", "force"];
```

Magnetometer power mode per `MagConfig::power` code.

### AccelConfig

```rust
pub struct AccelConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}
```

ACC_CONFIG (page 1, 0x08) as register codes: `range<1:0>`,
`bandwidth<4:2>`, `power<7:5>`. Default = power-on 0x0D (±4 g, 62.5 Hz,
normal).

#### AccelConfig::pack

```rust
pub fn pack(&self) -> u8
```

#### AccelConfig::unpack

```rust
pub fn unpack(value: u8) -> AccelConfig
```

### GyroConfig

```rust
pub struct GyroConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}
```

GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), two bytes: byte 0 `range<2:0>`
`bandwidth<5:3>`, byte 1 `power<2:0>`. Default = power-on 0x38/0x00
(2000 dps, 32 Hz, normal).

#### GyroConfig::pack

```rust
pub fn pack(&self) -> [u8; 2]
```

#### GyroConfig::unpack

```rust
pub fn unpack(data: &[u8]) -> Result<GyroConfig, CodecError>
```

Needs at least 2 bytes (GYR_CONFIG_0, GYR_CONFIG_1).

### MagConfig

```rust
pub struct MagConfig {
    pub rate: u8,
    pub mode: u8,
    pub power: u8,
}
```

MAG_CONFIG (page 1, 0x09): `rate<2:0>`, `mode<4:3>`, `power<6:5>` (bit 7
unused — a repack clears it). Default = power-on 0x0B (10 Hz, regular,
normal).

#### MagConfig::pack

```rust
pub fn pack(&self) -> u8
```

#### MagConfig::unpack

```rust
pub fn unpack(value: u8) -> MagConfig
```

### RawBlock

```rust
pub struct RawBlock {
    pub accel: Option<[i16; 3]>,
    pub mag: Option<[i16; 3]>,
    pub gyro: Option<[i16; 3]>,
    pub euler: Option<[i16; 3]>,
    pub quaternion: Option<[i16; 4]>,
    pub linear_accel: Option<[i16; 3]>,
    pub gravity: Option<[i16; 3]>,
    /// TEMP, i8.
    pub temperature: Option<i8>,
    /// CALIB_STAT byte ([`CalibStatus::unpack`]).
    pub calib_stat: Option<u8>,
}
```

Raw register values found in one register window. A channel is `None` when
the window `addr..addr+len` does not cover all of its bytes. Euler is
`[heading, roll, pitch]`, quaternion `[w, x, y, z]`.

### decode_block

```rust
pub fn decode_block(addr: u8, data: &[u8]) -> RawBlock
```

Unpack whatever channels the register window starting at `addr` holds
(offsets are `reg − addr`). `data` is the block as read — e.g.
[`crate::bno055::StreamData::data`] with its `addr`.

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
