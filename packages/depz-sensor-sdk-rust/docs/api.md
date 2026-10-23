# API reference

Auto-generated from the crate's public surface (the `pub` items and
their `///` doc-comments across `src/**`) by `scripts/gen_api_md.py`.
Regenerate with `python3 scripts/gen_api_md.py` from the crate root.
Edit the doc-comments in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L4CX](vl53l4cx/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [TMF8829](tmf8829/api.md) · [BNO055](bno055/api.md) · [BNO085](bno085/api.md) · [BNO086](bno086/api.md).

## Contents

- **Discovery**: [`SENSOR_CLASS_NAMES`](#sensor_class_names), [`resolve_sensor_class`](#resolve_sensor_class), [`SensorType`](#sensortype), [`Mode`](#mode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name), [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`DEV_USB_VID`](#dev_usb_vid), [`DEPZ_PID_MODEL`](#depz_pid_model), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortEntry`](#portentry), [`order_ports`](#order_ports)
- **Live devices**: [`DEFAULT_TIMEOUT`](#default_timeout), [`DeviceEvent`](#deviceevent), [`LinkStats`](#linkstats), [`TimeSyncResult`](#timesyncresult), [`host_now_us`](#host_now_us), [`ReportHandler`](#reporthandler), [`DepzDevice`](#depzdevice), [`expect_report`](#expect_report), [`expect_text`](#expect_text), [`DepzError`](#depzerror), [`Result`](#result), [`Link`](#link), [`ReplayLink`](#replaylink), [`RecordingLink`](#recordinglink), [`RECORDING_SCHEMA`](#recording_schema), [`ProbeResult`](#proberesult), [`AnyDevice`](#anydevice), [`probe`](#probe), [`attach`](#attach), [`open_on_link`](#open_on_link), [`Subscription`](#subscription), [`Hook`](#hook)
- **SR04**: [`Sr04Measurement`](#sr04measurement), [`Sr04Device`](#sr04device), [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L4CD (ToF)**: [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdDevice`](#vl53l4cddevice), [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`I2C_KHZ_STEPS`](#i2c_khz_steps), [`i2c_error_name`](#i2c_error_name), [`pack_read_reg`](#pack_read_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`Vl53l4Info`](#vl53l4info), [`StreamData`](#streamdata), [`Vl53l4Platform`](#vl53l4platform), [`DetectionThresholds`](#detectionthresholds), [`Vl53l4cdDriver`](#vl53l4cddriver), [`SOFT_RESET`](#soft_reset), [`I2C_SLAVE__DEVICE_ADDRESS`](#i2c_slave__device_address), [`OSC_FREQUENCY`](#osc_frequency), [`VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`](#vhv_config__timeout_macrop_loop_bound), [`XTALK_PLANE_OFFSET_KCPS`](#xtalk_plane_offset_kcps), [`XTALK_X_PLANE_GRADIENT_KCPS`](#xtalk_x_plane_gradient_kcps), [`XTALK_Y_PLANE_GRADIENT_KCPS`](#xtalk_y_plane_gradient_kcps), [`RANGE_OFFSET_MM`](#range_offset_mm), [`INNER_OFFSET_MM`](#inner_offset_mm), [`OUTER_OFFSET_MM`](#outer_offset_mm), [`GPIO_HV_MUX__CTRL`](#gpio_hv_mux__ctrl), [`GPIO__TIO_HV_STATUS`](#gpio__tio_hv_status), [`SYSTEM__INTERRUPT`](#system__interrupt), [`RANGE_CONFIG_A`](#range_config_a), [`RANGE_CONFIG_B`](#range_config_b), [`RANGE_CONFIG__SIGMA_THRESH`](#range_config__sigma_thresh), [`MIN_COUNT_RATE_RTN_LIMIT_MCPS`](#min_count_rate_rtn_limit_mcps), [`INTERMEASUREMENT_MS`](#intermeasurement_ms), [`THRESH_HIGH`](#thresh_high), [`THRESH_LOW`](#thresh_low), [`SYSTEM__INTERRUPT_CLEAR`](#system__interrupt_clear), [`SYSTEM_START`](#system_start), [`RESULT__RANGE_STATUS`](#result__range_status), [`RESULT__SPAD_NB`](#result__spad_nb), [`RESULT__SIGNAL_RATE`](#result__signal_rate), [`RESULT__AMBIENT_RATE`](#result__ambient_rate), [`RESULT__SIGMA`](#result__sigma), [`RESULT__DISTANCE`](#result__distance), [`RESULT__OSC_CALIBRATE_VAL`](#result__osc_calibrate_val), [`FIRMWARE__SYSTEM_STATUS`](#firmware__system_status), [`IDENTIFICATION__MODEL_ID`](#identification__model_id), [`MODEL_ID_VL53L4CD`](#model_id_vl53l4cd), [`WINDOW_BELOW`](#window_below), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`CONFIG_ADDR`](#config_addr), [`CONFIG_END`](#config_end), [`DEFAULT_CONFIGURATION`](#default_configuration), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`config_block`](#config_block), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`I2C_KHZ_BOOT`](#i2c_khz_boot), [`I2C_KHZ_DEFAULT`](#i2c_khz_default), [`STATUS_RTN`](#status_rtn), [`range_status_name`](#range_status_name), [`Vl53l4Error`](#vl53l4error), [`Vl53l4Results`](#vl53l4results), [`parse_result_block`](#parse_result_block), [`range_timing_registers`](#range_timing_registers), [`decode_range_timing`](#decode_range_timing), [`try_offset_raw`](#try_offset_raw), [`decode_offset`](#decode_offset), [`try_xtalk_raw`](#try_xtalk_raw), [`decode_xtalk`](#decode_xtalk), [`signal_threshold_raw`](#signal_threshold_raw), [`decode_signal_threshold`](#decode_signal_threshold), [`sigma_threshold_raw`](#sigma_threshold_raw), [`decode_sigma_threshold`](#decode_sigma_threshold)
- **VL53L8 (ToF)**: [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`BridgeUld`](#bridgeuld), [`Vl53l8cxDevice`](#vl53l8cxdevice), [`Vl53l8chDevice`](#vl53l8chdevice), [`DIST_MM`](#dist_mm), [`SIGNAL_PER_SPAD_KCPS`](#signal_per_spad_kcps), [`RANGE_SIGMA_MM`](#range_sigma_mm), [`AMBIENT_PER_SPAD_KCPS`](#ambient_per_spad_kcps), [`NB_TARGET_DETECTED`](#nb_target_detected), [`TAR_STATUS`](#tar_status), [`NB_SPADS_ENABLED`](#nb_spads_enabled), [`MOTION_INDICATOR`](#motion_indicator), [`NB_THRESHOLDS`](#nb_thresholds), [`POWER_MODE_SLEEP`](#power_mode_sleep), [`POWER_MODE_WAKEUP`](#power_mode_wakeup), [`POWER_MODE_DEEP_SLEEP`](#power_mode_deep_sleep), [`AdvancedError`](#advancederror), [`XTALK_MARGIN_MAX_KCPS`](#xtalk_margin_max_kcps), [`try_xtalk_margin_to_raw`](#try_xtalk_margin_to_raw), [`MotionConfig`](#motionconfig), [`try_default_motion_config`](#try_default_motion_config), [`DetectionThreshold`](#detectionthreshold), [`DetectionThresholdBlocks`](#detectionthresholdblocks), [`try_pack_detection_thresholds`](#try_pack_detection_thresholds), [`CnhDecodeConfig`](#cnhdecodeconfig), [`CnhAggregate`](#cnhaggregate), [`CnhData`](#cnhdata), [`CnhError`](#cnherror), [`decode_cnh`](#decode_cnh), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`READ_MAX_LEN`](#read_max_len), [`CHUNK_SIZE`](#chunk_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`NB_TARGET_PER_ZONE`](#nb_target_per_zone), [`CNH_DATA_IDX`](#cnh_data_idx), [`Variant`](#variant), [`Vl53l8Error`](#vl53l8error), [`Vl53l8Results`](#vl53l8results), [`swap_buffer`](#swap_buffer), [`parse_frame`](#parse_frame), [`Vl53l8CnhSetup`](#vl53l8cnhsetup), [`Vl53l8UldError`](#vl53l8ulderror), [`UldResult`](#uldresult), [`Vl53l8Firmware`](#vl53l8firmware), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8Frame`](#vl53l8frame), [`Vl53l8Platform`](#vl53l8platform), [`get_xtalk_cmd`](#get_xtalk_cmd), [`calibrate_xtalk_cx`](#calibrate_xtalk_cx), [`calibrate_xtalk_lmz`](#calibrate_xtalk_lmz), [`calibrate_xtalk_l7cx`](#calibrate_xtalk_l7cx), [`xtalk_cal_output`](#xtalk_cal_output), [`xtalk_cal_output_enable`](#xtalk_cal_output_enable), [`RANGING_MODE_CONTINUOUS`](#ranging_mode_continuous), [`RANGING_MODE_AUTONOMOUS`](#ranging_mode_autonomous), [`TARGET_ORDER_CLOSEST`](#target_order_closest), [`TARGET_ORDER_STRONGEST`](#target_order_strongest), [`MODULE_TYPE_MZ`](#module_type_mz), [`MODULE_TYPE_MZEVO`](#module_type_mzevo), [`MODULE_TYPE_MZPLUS`](#module_type_mzplus), [`NVM_DATA_SIZE`](#nvm_data_size), [`OFFSET_BUFFER_SIZE`](#offset_buffer_size), [`XTALK_BUFFER_SIZE`](#xtalk_buffer_size), [`Vl53l8Driver`](#vl53l8driver), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`STREAM_TOTAL_MAX`](#stream_total_max), [`FrameChunk`](#framechunk), [`unpack_frame_chunk`](#unpack_frame_chunk), [`CompletedFrame`](#completedframe), [`FrameReassembler`](#framereassembler)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7cxDevice`](#vl53l7cxdevice), [`Vl53l5cxDevice`](#vl53l5cxdevice), [`Vl53l7chDevice`](#vl53l7chdevice), [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`PIN_LPN_OFF`](#pin_lpn_off), [`PIN_LPN_ON`](#pin_lpn_on), [`PIN_I2C_RST`](#pin_i2c_rst), [`PIN_SOFT_CYCLE`](#pin_soft_cycle), [`READ_MAX_LEN`](#read_max_len), [`WRITE_MAX_LEN`](#write_max_len), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`INFO_SIZE`](#info_size), [`I2C_SPEED_STEPS_KHZ`](#i2c_speed_steps_khz), [`i2c_error_name`](#i2c_error_name), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_pin_ctrl`](#pack_pin_ctrl), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`Vl53l7Info`](#vl53l7info), [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`Vl53l7Model`](#vl53l7model), [`resolve_model`](#resolve_model)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`PLOTTABLE_STATUSES`](#plottable_statuses), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lxProductInfo`](#vl53lxproductinfo), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`BareDriver`](#baredriver), [`HistConfig`](#histconfig), [`HistPostProcessConfig`](#histpostprocessconfig), [`HistogramBinData`](#histogrambindata), [`BareDeviceState`](#baredevicestate), [`LlState`](#llstate), [`RangeData`](#rangedata), [`RangeResults`](#rangeresults), [`MAX_BIN_SEQUENCE_LENGTH`](#max_bin_sequence_length), [`MAX_BIN_SEQUENCE_CODE`](#max_bin_sequence_code), [`MAX_PULSES`](#max_pulses), [`MAX_RANGE_RESULTS`](#max_range_results), [`MAX_ALLOWED_PHASE`](#max_allowed_phase), [`SPAD_TOTAL_COUNT_MAX`](#spad_total_count_max), [`SPAD_TOTAL_COUNT_RES_THRES`](#spad_total_count_res_thres), [`SPEED_OF_LIGHT_IN_AIR_DIV8`](#speed_of_light_in_air_div8), [`SIGMA_INVALID`](#sigma_invalid), [`DEVICE_ERROR_NO_UPDATE`](#device_error_no_update), [`DEVICE_ERROR_RANGE_PHASE_CHECK`](#device_error_range_phase_check), [`DEVICE_ERROR_SIGMA_THRESHOLD_CHECK`](#device_error_sigma_threshold_check), [`DEVICE_ERROR_PHASE_CONSISTENCY`](#device_error_phase_consistency), [`DEVICE_ERROR_RANGE_COMPLETE`](#device_error_range_complete), [`DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK`](#device_error_range_complete_no_wrap_check), [`DEVICE_ERROR_EVENT_CONSISTENCY`](#device_error_event_consistency), [`DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE`](#device_error_range_complete_merged_pulse), [`DEVICE_ERROR_PREV_RANGE_NO_TARGETS`](#device_error_prev_range_no_targets), [`HIST_TARGET_ORDER_STRONGEST_FIRST`](#hist_target_order_strongest_first), [`HIST_TARGET_ORDER_CLOSEST_FIRST`](#hist_target_order_closest_first), [`RANGE_STATUS_RANGE_VALID`](#range_status_range_valid), [`RANGE_STATUS_SIGMA_FAIL`](#range_status_sigma_fail), [`RANGE_STATUS_OUT_OF_BOUNDS_FAIL`](#range_status_out_of_bounds_fail), [`RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK`](#range_status_range_valid_no_wrap_check), [`RANGE_STATUS_WRAP_TARGET_FAIL`](#range_status_wrap_target_fail), [`RANGE_STATUS_RANGE_VALID_MERGED_PULSE`](#range_status_range_valid_merged_pulse), [`RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL`](#range_status_target_present_lack_of_signal), [`RANGE_STATUS_NONE`](#range_status_none), [`status_name`](#status_name), [`PrevTarget`](#prevtarget), [`py_mod`](#py_mod), [`isqrt`](#isqrt), [`calc_pll_period_mm`](#calc_pll_period_mm), [`rate_maths`](#rate_maths), [`rate_per_spad_maths`](#rate_per_spad_maths), [`events_per_spad_maths`](#events_per_spad_maths), [`range_maths`](#range_maths), [`average_repeated_bins`](#average_repeated_bins), [`calc_zero_distance_phase`](#calc_zero_distance_phase), [`estimate_ambient_from_thresholded_bins`](#estimate_ambient_from_thresholded_bins), [`estimate_ambient_from_ambient_bins`](#estimate_ambient_from_ambient_bins), [`remove_ambient_bins`](#remove_ambient_bins), [`process_data`](#process_data), [`process_data_merged`](#process_data_merged), [`events_consistency_check`](#events_consistency_check), [`merged_pulse_check`](#merged_pulse_check), [`convert_status`](#convert_status), [`to_range`](#to_range), [`Vl53lxHistogramDriver`](#vl53lxhistogramdriver), [`FrameHistory`](#framehistory), [`RegField`](#regfield), [`RegBlockDef`](#regblockdef), [`block_def`](#block_def), [`tuning_default`](#tuning_default), [`header_default`](#header_default), [`RegBlock`](#regblock), [`RANGE_START_BLOCKS`](#range_start_blocks), [`DeviceImage`](#deviceimage), [`encode_row_col`](#encode_row_col), [`decode_row_col`](#decode_row_col), [`decode_zone_size`](#decode_zone_size), [`encode_zone_size`](#encode_zone_size), [`decode_zone_limits`](#decode_zone_limits), [`is_aperture_location`](#is_aperture_location), [`calc_mm_effective_spads`](#calc_mm_effective_spads), [`combine_mm1_mm2_offsets`](#combine_mm1_mm2_offsets), [`rtn_good_spads`](#rtn_good_spads), [`calc_pll_period_us`](#calc_pll_period_us), [`decode_vcsel_period`](#decode_vcsel_period), [`calc_macro_period_us`](#calc_macro_period_us), [`calc_timeout_mclks`](#calc_timeout_mclks), [`encode_timeout`](#encode_timeout), [`decode_timeout`](#decode_timeout), [`calc_encoded_timeout`](#calc_encoded_timeout), [`duration_maths`](#duration_maths), [`calc_timeout_register_values`](#calc_timeout_register_values), [`OpticalCentre`](#opticalcentre), [`CalPeakRateMap`](#calpeakratemap), [`AdditionalOffsetCalData`](#additionaloffsetcaldata), [`FmtRangeData`](#fmtrangedata), [`FMT_OPTICAL_CENTRE`](#fmt_optical_centre), [`FMT_CAL_PEAK_RATE_MAP`](#fmt_cal_peak_rate_map), [`FMT_ADDITIONAL_OFFSET_CAL`](#fmt_additional_offset_cal), [`FMT_RANGE_RESULTS_140MM_DARK`](#fmt_range_results_140mm_dark), [`read_region`](#read_region), [`read_optical_centre`](#read_optical_centre), [`read_cal_peak_rate_map`](#read_cal_peak_rate_map), [`read_additional_offset_cal_data`](#read_additional_offset_cal_data), [`read_fmt_range_results`](#read_fmt_range_results), [`POWER_MANAGEMENT_GO1_POWER_FORCE`](#power_management_go1_power_force), [`FIRMWARE_ENABLE`](#firmware_enable), [`RESULT_OSC_CALIBRATE_VAL`](#result_osc_calibrate_val), [`PATCH_CTRL`](#patch_ctrl), [`PATCH_JMP_ENABLES`](#patch_jmp_enables), [`PATCH_DATA_ENABLES`](#patch_data_enables), [`PATCH_OFFSET_0`](#patch_offset_0), [`PATCH_ADDRESS_0`](#patch_address_0), [`HISTOGRAM_BIN_DATA_I2C_INDEX`](#histogram_bin_data_i2c_index), [`RESULT_HISTOGRAM_BIN_0_2`](#result_histogram_bin_0_2), [`RESULT_HISTOGRAM_BIN_23_0`](#result_histogram_bin_23_0), [`PHASECAL_RESULT_REFERENCE_PHASE`](#phasecal_result_reference_phase), [`PHASECAL_RESULT_VCSEL_START`](#phasecal_result_vcsel_start), [`RESULT_HISTOGRAM_BIN_23_0_MSB`](#result_histogram_bin_23_0_msb), [`RESULT_HISTOGRAM_BIN_23_0_LSB`](#result_histogram_bin_23_0_lsb), [`HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES`](#histogram_bin_data_i2c_size_bytes), [`DEVICE_MEASUREMENT_MODE_BACK_TO_BACK`](#device_measurement_mode_back_to_back), [`DEVICE_MEASUREMENT_MODE_ABORT`](#device_measurement_mode_abort), [`DEVICE_MEASUREMENT_MODE_STOP_MASK`](#device_measurement_mode_stop_mask), [`DEVICE_MEASUREMENT_MODE_MODE_MASK`](#device_measurement_mode_mode_mask), [`DEVICE_SCHEDULER_MODE_STREAMING`](#device_scheduler_mode_streaming), [`DEVICE_SCHEDULER_MODE_HISTOGRAM`](#device_scheduler_mode_histogram), [`DEVICE_READOUT_MODE_SINGLE_SD`](#device_readout_mode_single_sd), [`DEVICE_READOUT_MODE_DUAL_SD`](#device_readout_mode_dual_sd), [`GROUPED_PARAMETER_HOLD_ID_MASK`](#grouped_parameter_hold_id_mask), [`INTERRUPT_CONFIG_NEW_SAMPLE_READY`](#interrupt_config_new_sample_ready), [`CLEAR_RANGE_INT`](#clear_range_int), [`RANGE_STATUS_MASK`](#range_status_mask), [`DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW`](#device_interrupt_polarity_active_low), [`DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS`](#device_gpio_mode_output_range_and_error_interrupts), [`DEVICE_DSS_MODE_TARGET_RATE`](#device_dss_mode_target_rate), [`SEQUENCE_VHV_EN`](#sequence_vhv_en), [`SEQUENCE_PHASECAL_EN`](#sequence_phasecal_en), [`SEQUENCE_DSS1_EN`](#sequence_dss1_en), [`SEQUENCE_DSS2_EN`](#sequence_dss2_en), [`SEQUENCE_MM1_EN`](#sequence_mm1_en), [`SEQUENCE_MM2_EN`](#sequence_mm2_en), [`SEQUENCE_RANGE_EN`](#sequence_range_en), [`SPAD_ARRAY_WIDTH`](#spad_array_width), [`SPAD_ARRAY_HEIGHT`](#spad_array_height), [`RTN_SPAD_APERTURE_TRANSMISSION`](#rtn_spad_aperture_transmission), [`RTN_SPAD_UNITY_TRANSMISSION`](#rtn_spad_unity_transmission), [`AMBIENT_WINDOW_VCSEL_PERIODS`](#ambient_window_vcsel_periods), [`RANGING_WINDOW_VCSEL_PERIODS`](#ranging_window_vcsel_periods), [`MACRO_PERIOD_VCSEL_PERIODS`](#macro_period_vcsel_periods), [`HISTOGRAM_BUFFER_SIZE`](#histogram_buffer_size), [`DISTANCE_MODES`](#distance_modes), [`L4_DISTANCE_MODES`](#l4_distance_modes), [`TIMING_GUARD_US`](#timing_guard_us), [`TIMING_DIVISOR`](#timing_divisor), [`FDA_MAX_TIMING_BUDGET_US`](#fda_max_timing_budget_us), [`L4_FDA_MAX_TIMING_BUDGET_US`](#l4_fda_max_timing_budget_us), [`BLOCKS`](#blocks), [`TUNING`](#tuning), [`DEFAULTS`](#defaults), [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`CLEAR_STEPS_MAX`](#clear_steps_max), [`INFO_SIZE`](#info_size), [`pack_set_addr_width`](#pack_set_addr_width), [`pack_start_stream`](#pack_start_stream), [`Vl53lxInfo`](#vl53lxinfo), [`DIE_BLOCK_ADDR`](#die_block_addr), [`DIE_BLOCK_LEN`](#die_block_len), [`L0X_BLOCK_ADDR`](#l0x_block_addr), [`L0X_BLOCK_LEN`](#l0x_block_len), [`HISTOGRAM_BLOCK_ADDR`](#histogram_block_addr), [`HISTOGRAM_BLOCK_LEN`](#histogram_block_len), [`HISTOGRAM_BINS`](#histogram_bins), [`DieVariant`](#dievariant), [`ShortBlock`](#shortblock), [`DieResult`](#dieresult), [`decode_die_block`](#decode_die_block), [`L0xRaw`](#l0xraw), [`decode_l0x_raw`](#decode_l0x_raw), [`HistogramRaw`](#histogramraw), [`decode_histogram_raw`](#decode_histogram_raw), [`Vl53lxPlatform`](#vl53lxplatform), [`Vl53lxTarget`](#vl53lxtarget), [`ExtraValue`](#extravalue), [`Vl53lxRange`](#vl53lxrange), [`Thresholds`](#thresholds), [`BinData`](#bindata), [`Vl53lxDriver`](#vl53lxdriver), [`DriverKind`](#driverkind), [`DRIVER_KINDS`](#driver_kinds), [`BusParams`](#busparams), [`Product`](#product), [`PRODUCTS`](#products), [`product`](#product), [`product_from_board_name`](#product_from_board_name), [`Vl53lxClass`](#vl53lxclass), [`resolve_product`](#resolve_product), [`resolve_class`](#resolve_class), [`SOFT_RESET`](#soft_reset), [`I2C_SLAVE_DEVICE_ADDRESS`](#i2c_slave_device_address), [`OSC_FREQUENCY`](#osc_frequency), [`VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND`](#vhv_config_timeout_macrop_loop_bound), [`XTALK_PLANE_OFFSET_KCPS`](#xtalk_plane_offset_kcps), [`XTALK_X_PLANE_GRADIENT_KCPS`](#xtalk_x_plane_gradient_kcps), [`XTALK_Y_PLANE_GRADIENT_KCPS`](#xtalk_y_plane_gradient_kcps), [`RANGE_OFFSET_MM`](#range_offset_mm), [`INNER_OFFSET_MM`](#inner_offset_mm), [`OUTER_OFFSET_MM`](#outer_offset_mm), [`GPIO_HV_MUX_CTRL`](#gpio_hv_mux_ctrl), [`GPIO_TIO_HV_STATUS`](#gpio_tio_hv_status), [`SYSTEM_INTERRUPT_CONFIG_GPIO`](#system_interrupt_config_gpio), [`PHASECAL_CONFIG_TIMEOUT_MACROP`](#phasecal_config_timeout_macrop), [`RANGE_CONFIG_TIMEOUT_MACROP_A_HI`](#range_config_timeout_macrop_a_hi), [`RANGE_CONFIG_VCSEL_PERIOD_A`](#range_config_vcsel_period_a), [`RANGE_CONFIG_TIMEOUT_MACROP_B_HI`](#range_config_timeout_macrop_b_hi), [`RANGE_CONFIG_VCSEL_PERIOD_B`](#range_config_vcsel_period_b), [`RANGE_CONFIG_SIGMA_THRESH`](#range_config_sigma_thresh), [`MIN_COUNT_RATE_RTN_LIMIT_MCPS`](#min_count_rate_rtn_limit_mcps), [`RANGE_CONFIG_VALID_PHASE_HIGH`](#range_config_valid_phase_high), [`INTERMEASUREMENT_MS`](#intermeasurement_ms), [`THRESH_HIGH`](#thresh_high), [`THRESH_LOW`](#thresh_low), [`SD_CONFIG_WOI_SD0`](#sd_config_woi_sd0), [`SD_CONFIG_INITIAL_PHASE_SD0`](#sd_config_initial_phase_sd0), [`ROI_CONFIG_USER_ROI_CENTRE_SPAD`](#roi_config_user_roi_centre_spad), [`ROI_CONFIG_USER_ROI_XY_SIZE`](#roi_config_user_roi_xy_size), [`SYSTEM_INTERRUPT_CLEAR`](#system_interrupt_clear), [`SYSTEM_MODE_START`](#system_mode_start), [`RESULT_RANGE_STATUS`](#result_range_status), [`RESULT_OSC_CALIBRATE_VAL`](#result_osc_calibrate_val), [`FIRMWARE_SYSTEM_STATUS`](#firmware_system_status), [`IDENTIFICATION_MODEL_ID`](#identification_model_id), [`ROI_CONFIG_MODE_ROI_CENTRE_SPAD`](#roi_config_mode_roi_centre_spad), [`WINDOW_BELOW`](#window_below), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`CONFIG_ADDR`](#config_addr), [`CONFIG_END`](#config_end), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`DieConfig`](#dieconfig), [`phase_window_mm`](#phase_window_mm), [`status_text`](#status_text), [`Die`](#die), [`MODEL_ID_VL53L0X`](#model_id_vl53l0x), [`RefCalibration`](#refcalibration), [`RefSpads`](#refspads), [`isqrt`](#isqrt), [`Vl53l0xUldDriver`](#vl53l0xulddriver), [`Vl53l1UldDriver`](#vl53l1ulddriver), [`Vl53l4UldDriver`](#vl53l4ulddriver), [`Vl53l3UlpDriver`](#vl53l3ulpdriver), [`DriverCtor`](#driverctor), [`HIST_CAVEAT`](#hist_caveat), [`driver_kinds`](#driver_kinds), [`create_driver`](#create_driver)
- **BNO086 (IMU)**: [`Bno086Cmd`](#bno086cmd), [`RPT_DATA`](#rpt_data), [`BUSY_BACKOFF_MS`](#busy_backoff_ms), [`Bno086Data`](#bno086data), [`FORCE_SENSOR_FLUSH`](#force_sensor_flush), [`EXECUTABLE_RESET_COMPLETE`](#executable_reset_complete), [`CMD_ERRORS`](#cmd_errors), [`CMD_COUNTER`](#cmd_counter), [`CMD_TARE`](#cmd_tare), [`CMD_INITIALIZE`](#cmd_initialize), [`CMD_SAVE_DCD`](#cmd_save_dcd), [`CMD_ME_CALIBRATE`](#cmd_me_calibrate), [`CMD_PERIODIC_DCD_CONFIG`](#cmd_periodic_dcd_config), [`CMD_GET_OSCILLATOR_TYPE`](#cmd_get_oscillator_type), [`CMD_CLEAR_DCD_AND_RESET`](#cmd_clear_dcd_and_reset), [`CMD_INTERACTIVE_CALIBRATION`](#cmd_interactive_calibration), [`OSC_INTERNAL`](#osc_internal), [`OSC_EXT_CRYSTAL`](#osc_ext_crystal), [`OSC_EXT_CLOCK`](#osc_ext_clock), [`ERROR_SOURCE_NO_MORE_ERRORS`](#error_source_no_more_errors), [`TARE_X`](#tare_x), [`TARE_Y`](#tare_y), [`TARE_Z`](#tare_z), [`TARE_ALL`](#tare_all), [`TareBasis`](#tarebasis), [`MotionIntent`](#motionintent), [`FRS_STATIC_CALIBRATION_AGM`](#frs_static_calibration_agm), [`FRS_NOMINAL_CALIBRATION`](#frs_nominal_calibration), [`FRS_DYNAMIC_CALIBRATION`](#frs_dynamic_calibration), [`FRS_ME_POWER_MGMT`](#frs_me_power_mgmt), [`FRS_SYSTEM_ORIENTATION`](#frs_system_orientation), [`FRS_ACCEL_ORIENTATION`](#frs_accel_orientation), [`FRS_GYROSCOPE_ORIENTATION`](#frs_gyroscope_orientation), [`FRS_MAGNETOMETER_ORIENTATION`](#frs_magnetometer_orientation), [`FRS_ARVR_STABILIZATION_RV`](#frs_arvr_stabilization_rv), [`FRS_ARVR_STABILIZATION_GRV`](#frs_arvr_stabilization_grv), [`FRS_SERIAL_NUMBER`](#frs_serial_number), [`FRS_USER_RECORD`](#frs_user_record), [`FRS_SIG_MOTION_DETECT_CONFIG`](#frs_sig_motion_detect_config), [`FRS_SHAKE_DETECT_CONFIG`](#frs_shake_detect_config), [`FRS_STABILITY_DETECTOR_CONFIG`](#frs_stability_detector_config), [`FRS_ACTIVITY_TRACKER_CONFIG`](#frs_activity_tracker_config), [`METADATA_RECORDS`](#metadata_records), [`FRS_NO_ERROR`](#frs_no_error), [`FRS_UNRECOGNIZED_FRS_TYPE`](#frs_unrecognized_frs_type), [`FRS_BUSY`](#frs_busy), [`FRS_READ_COMPLETED`](#frs_read_completed), [`FRS_OFFSET_OUT_OF_RANGE`](#frs_offset_out_of_range), [`FRS_RECORD_EMPTY`](#frs_record_empty), [`FRS_BLOCK_COMPLETED`](#frs_block_completed), [`FRS_BLOCK_AND_READ_COMPLETED`](#frs_block_and_read_completed), [`FRS_DEVICE_ERROR`](#frs_device_error), [`FRS_WRITE_WORDS_RECEIVED`](#frs_write_words_received), [`FRS_WRITE_UNRECOGNIZED_FRS_TYPE`](#frs_write_unrecognized_frs_type), [`FRS_WRITE_BUSY`](#frs_write_busy), [`FRS_WRITE_COMPLETED`](#frs_write_completed), [`FRS_WRITE_MODE_READY`](#frs_write_mode_ready), [`FRS_WRITE_FAILED`](#frs_write_failed), [`FRS_WRITE_NOT_IN_WRITE_MODE`](#frs_write_not_in_write_mode), [`FRS_WRITE_INVALID_LENGTH`](#frs_write_invalid_length), [`FRS_WRITE_RECORD_VALID`](#frs_write_record_valid), [`FRS_WRITE_RECORD_INVALID`](#frs_write_record_invalid), [`Sh2HubError`](#sh2huberror), [`FeatureResponse`](#featureresponse), [`ProductId`](#productid), [`CommandResponse`](#commandresponse), [`FrsReadResponse`](#frsreadresponse), [`FrsWriteResponse`](#frswriteresponse), [`ErrorRecord`](#errorrecord), [`Counts`](#counts), [`CalibrationConfig`](#calibrationconfig), [`SensorMetadata`](#sensormetadata), [`COUNTS_GET`](#counts_get), [`COUNTS_CLEAR`](#counts_clear), [`ME_CAL_GET`](#me_cal_get), [`split_control_cargo`](#split_control_cargo), [`counts_get_params`](#counts_get_params), [`counts_clear_params`](#counts_clear_params), [`errors_params`](#errors_params), [`tare_now_params`](#tare_now_params), [`persist_tare_params`](#persist_tare_params), [`set_reorientation_params`](#set_reorientation_params), [`me_calibration_params`](#me_calibration_params), [`interactive_calibration_params`](#interactive_calibration_params), [`force_flush`](#force_flush), [`periodic_dcd_params`](#periodic_dcd_params), [`FrsReadSession`](#frsreadsession), [`FrsWriteSession`](#frswritesession), [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`FLUSH_COMPLETED`](#flush_completed), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`q_point`](#q_point), [`report_length`](#report_length), [`Report`](#report), [`Vector3Kind`](#vector3kind), [`try_parse_input_cargo`](#try_parse_input_cargo), [`try_parse_gyro_rv_cargo`](#try_parse_gyro_rv_cargo), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo), [`REPORT_COMMAND_RESPONSE`](#report_command_response), [`REPORT_COMMAND_REQUEST`](#report_command_request), [`REPORT_FRS_READ_RESPONSE`](#report_frs_read_response), [`REPORT_FRS_READ_REQUEST`](#report_frs_read_request), [`REPORT_FRS_WRITE_RESPONSE`](#report_frs_write_response), [`REPORT_FRS_WRITE_DATA`](#report_frs_write_data), [`REPORT_FRS_WRITE_REQUEST`](#report_frs_write_request), [`REPORT_PRODUCT_ID_RESPONSE`](#report_product_id_response), [`REPORT_PRODUCT_ID_REQUEST`](#report_product_id_request), [`REPORT_GET_FEATURE_RESPONSE`](#report_get_feature_response), [`REPORT_SET_FEATURE_COMMAND`](#report_set_feature_command), [`REPORT_GET_FEATURE_REQUEST`](#report_get_feature_request), [`Sh2Error`](#sh2error), [`FEATURE_FLAG_WAKEUP`](#feature_flag_wakeup), [`FEATURE_FLAG_ALWAYS_ON`](#feature_flag_always_on), [`build_set_feature`](#build_set_feature), [`build_get_feature_request`](#build_get_feature_request), [`build_product_id_request`](#build_product_id_request), [`build_command_request`](#build_command_request), [`build_frs_read_request`](#build_frs_read_request), [`build_frs_write_request`](#build_frs_write_request), [`build_frs_write_data`](#build_frs_write_data), [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ExecutableCommand`](#executablecommand), [`build_sleep`](#build_sleep), [`build_power_on`](#build_power_on), [`ShtpHeader`](#shtpheader), [`ShtpError`](#shtperror), [`try_pack_shtp_header`](#try_pack_shtp_header), [`try_unpack_shtp_header`](#try_unpack_shtp_header), [`ShtpCargo`](#shtpcargo), [`try_build_frame`](#try_build_frame), [`ShtpLayer`](#shtplayer), [`RATE_LOW_FACTOR`](#rate_low_factor), [`RATE_HIGH_FACTOR`](#rate_high_factor), [`Bno086Device`](#bno086device)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`XFER_MAX`](#xfer_max), [`TRIGGER_TIMER`](#trigger_timer), [`TRIGGER_INT`](#trigger_int), [`INFO_SIZE`](#info_size), [`I2C_ADDR`](#i2c_addr), [`EXPECTED_CHIP_ID`](#expected_chip_id), [`EXPECTED_ACC_ID`](#expected_acc_id), [`EXPECTED_MAG_ID`](#expected_mag_id), [`EXPECTED_GYR_ID`](#expected_gyr_id), [`pack_read_reg`](#pack_read_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_write_reg`](#pack_write_reg), [`pack_reset`](#pack_reset), [`pack_stop_stream`](#pack_stop_stream), [`pack_get_info`](#pack_get_info), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`StreamData`](#streamdata), [`Bno055Info`](#bno055info), [`REG_CHIP_ID`](#reg_chip_id), [`REG_PAGE_ID`](#reg_page_id), [`REG_ACC_DATA`](#reg_acc_data), [`REG_MAG_DATA`](#reg_mag_data), [`REG_GYR_DATA`](#reg_gyr_data), [`REG_EUL_DATA`](#reg_eul_data), [`REG_QUA_DATA`](#reg_qua_data), [`REG_LIA_DATA`](#reg_lia_data), [`REG_GRV_DATA`](#reg_grv_data), [`REG_TEMP`](#reg_temp), [`REG_CALIB_STAT`](#reg_calib_stat), [`REG_ST_RESULT`](#reg_st_result), [`REG_INT_STA`](#reg_int_sta), [`REG_SYS_CLK_STATUS`](#reg_sys_clk_status), [`REG_SYS_STATUS`](#reg_sys_status), [`REG_SYS_ERR`](#reg_sys_err), [`REG_UNIT_SEL`](#reg_unit_sel), [`REG_OPR_MODE`](#reg_opr_mode), [`REG_PWR_MODE`](#reg_pwr_mode), [`REG_SYS_TRIGGER`](#reg_sys_trigger), [`REG_TEMP_SOURCE`](#reg_temp_source), [`REG_AXIS_MAP_CONFIG`](#reg_axis_map_config), [`REG_AXIS_MAP_SIGN`](#reg_axis_map_sign), [`REG_SIC_MATRIX`](#reg_sic_matrix), [`REG_CALIB_PROFILE`](#reg_calib_profile), [`CALIB_PROFILE_LEN`](#calib_profile_len), [`REG1_ACC_CONFIG`](#reg1_acc_config), [`REG1_MAG_CONFIG`](#reg1_mag_config), [`REG1_GYR_CONFIG_0`](#reg1_gyr_config_0), [`REG1_GYR_CONFIG_1`](#reg1_gyr_config_1), [`REG1_ACC_SLEEP_CONFIG`](#reg1_acc_sleep_config), [`REG1_GYR_SLEEP_CONFIG`](#reg1_gyr_sleep_config), [`REG1_INT_MSK`](#reg1_int_msk), [`REG1_INT_EN`](#reg1_int_en), [`REG1_ACC_AM_THRES`](#reg1_acc_am_thres), [`REG1_GYR_AM_SET`](#reg1_gyr_am_set), [`REG1_UNIQUE_ID`](#reg1_unique_id), [`UNIQUE_ID_LEN`](#unique_id_len), [`FULL_BLOCK`](#full_block), [`QUAT_BLOCK`](#quat_block), [`SYS_TRIGGER_SELF_TEST`](#sys_trigger_self_test), [`SYS_TRIGGER_RST_SYS`](#sys_trigger_rst_sys), [`SYS_TRIGGER_RST_INT`](#sys_trigger_rst_int), [`SYS_TRIGGER_CLK_SEL`](#sys_trigger_clk_sel), [`INT_ACC_BSX_DRDY`](#int_acc_bsx_drdy), [`INT_MAG_DRDY`](#int_mag_drdy), [`INT_GYR_AM`](#int_gyr_am), [`INT_GYR_HIGH_RATE`](#int_gyr_high_rate), [`INT_GYR_DRDY`](#int_gyr_drdy), [`INT_ACC_HIGH_G`](#int_acc_high_g), [`INT_ACC_AM`](#int_acc_am), [`INT_ACC_NM`](#int_acc_nm), [`ST_ACC`](#st_acc), [`ST_MAG`](#st_mag), [`ST_GYR`](#st_gyr), [`ST_MCU`](#st_mcu), [`EXPECTED_SELF_TEST`](#expected_self_test), [`SYS_STATUS_BOOTING`](#sys_status_booting), [`OprMode`](#oprmode), [`PwrMode`](#pwrmode), [`TempSource`](#tempsource), [`UNIT_ACC_MG`](#unit_acc_mg), [`UNIT_GYR_RPS`](#unit_gyr_rps), [`UNIT_EUL_RAD`](#unit_eul_rad), [`UNIT_TEMP_F`](#unit_temp_f), [`UNIT_ORI_ANDROID`](#unit_ori_android), [`Units`](#units), [`MAG_LSB`](#mag_lsb), [`QUAT_LSB`](#quat_lsb), [`FUSION_ACCEL_LSB`](#fusion_accel_lsb), [`CalibStatus`](#calibstatus), [`CalibrationProfile`](#calibrationprofile), [`pack_sic_matrix`](#pack_sic_matrix), [`unpack_sic_matrix`](#unpack_sic_matrix), [`SIC_IDENTITY`](#sic_identity), [`AXIS_X`](#axis_x), [`AXIS_Y`](#axis_y), [`AXIS_Z`](#axis_z), [`AxisRemap`](#axisremap), [`PLACEMENTS`](#placements), [`ACC_RANGE_G`](#acc_range_g), [`ACC_BANDWIDTH_HZ`](#acc_bandwidth_hz), [`ACC_POWER_NAMES`](#acc_power_names), [`GYR_RANGE_DPS`](#gyr_range_dps), [`GYR_BANDWIDTH_HZ`](#gyr_bandwidth_hz), [`GYR_POWER_NAMES`](#gyr_power_names), [`MAG_RATE_HZ`](#mag_rate_hz), [`MAG_OPR_NAMES`](#mag_opr_names), [`MAG_POWER_NAMES`](#mag_power_names), [`AccelConfig`](#accelconfig), [`GyroConfig`](#gyroconfig), [`MagConfig`](#magconfig), [`RawBlock`](#rawblock), [`decode_block`](#decode_block), [`MODE_SWITCH_FROM_CONFIG`](#mode_switch_from_config), [`MODE_SWITCH_TO_CONFIG`](#mode_switch_to_config), [`SELF_TEST_TIME`](#self_test_time), [`BOOT_SETTLE_TIMEOUT`](#boot_settle_timeout), [`STUCK_SELF_TEST_POLLS`](#stuck_self_test_polls), [`FUSION_START_TIMEOUT`](#fusion_start_timeout), [`Bno055SystemStatus`](#bno055systemstatus), [`Bno055Sample`](#bno055sample), [`Bno055Device`](#bno055device)
- **TMF8829 (dToF)**: [`Tmf8829InitResult`](#tmf8829initresult), [`Tmf8829AppliedConfig`](#tmf8829appliedconfig), [`Tmf8829Options`](#tmf8829options), [`Tmf8829Device`](#tmf8829device), [`Tmf8829Measurement`](#tmf8829measurement), [`distance_mm`](#distance_mm), [`MeasurementBuilder`](#measurementbuilder), [`NOMINAL_Q16`](#nominal_q16), [`CLOCK_WINDOW_MIN_US`](#clock_window_min_us), [`CLOCK_WINDOW_SLIDE_US`](#clock_window_slide_us), [`EDGE_LATE_US`](#edge_late_us), [`FT_NOMINAL`](#ft_nominal), [`FT_CLOCK_RESET`](#ft_clock_reset), [`FT_EDGE_LATE`](#ft_edge_late), [`FT_STALLED`](#ft_stalled), [`FT_T1_UNSURE`](#ft_t1_unsure), [`FT_NAMES`](#ft_names), [`flag_names`](#flag_names), [`fp_subints`](#fp_subints), [`SensorClock`](#sensorclock), [`FrameTime`](#frametime), [`CaptureTimer`](#capturetimer), [`Tmf8829Cmd`](#tmf8829cmd), [`Tmf8829Rpt`](#tmf8829rpt), [`EV_INT`](#ev_int), [`EV_BAD_HEADER`](#ev_bad_header), [`READ_MAX`](#read_max), [`WRITE_MAX`](#write_max), [`CHUNK_MAX`](#chunk_max), [`INT_RESULTS`](#int_results), [`INT_MOTION`](#int_motion), [`INT_PROXIMITY`](#int_proximity), [`INT_HISTOGRAMS`](#int_histograms), [`INT_MASK_ALL`](#int_mask_all), [`SF_REF_FRAME`](#sf_ref_frame), [`REG_DATA_HEADER_SIZE`](#reg_data_header_size), [`FRAME_CHUNK_HEADER_SIZE`](#frame_chunk_header_size), [`EVENT_SIZE`](#event_size), [`INFO_SIZE`](#info_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`pack_stop_stream`](#pack_stop_stream), [`pack_get_info`](#pack_get_info), [`RegData`](#regdata), [`Tmf8829Info`](#tmf8829info), [`FrameChunk`](#framechunk), [`Tmf8829Event`](#tmf8829event), [`RawFrame`](#rawframe), [`FrameReassembler`](#framereassembler), [`REG_CFG`](#reg_cfg), [`CFG_SIZE`](#cfg_size), [`PRESETS`](#presets), [`PRESET_KILO_ITERATIONS`](#preset_kilo_iterations), [`DEFAULT_PERIOD_MS`](#default_period_ms), [`preset_cmd`](#preset_cmd), [`preset_kilo_iterations`](#preset_kilo_iterations), [`CfgField`](#cfgfield), [`CFG_FIELDS`](#cfg_fields), [`cfg_field`](#cfg_field), [`cfg_get`](#cfg_get), [`cfg_set`](#cfg_set), [`config_fields`](#config_fields), [`FIRMWARE_LOAD_ADDR`](#firmware_load_addr), [`FIRMWARE_IMAGE`](#firmware_image), [`FIRMWARE_LICENSE`](#firmware_license), [`firmware_image`](#firmware_image), [`firmware_words`](#firmware_words), [`PRE_HEADER_SIZE`](#pre_header_size), [`HEADER_SIZE`](#header_size), [`FOOTER_SIZE`](#footer_size), [`FRAME_HEAD`](#frame_head), [`FRAME_EOF`](#frame_eof), [`FID_MASK`](#fid_mask), [`FID_RESULTS`](#fid_results), [`FID_HISTOGRAMS`](#fid_histograms), [`FID_REF_SPAD`](#fid_ref_spad), [`LAYOUT_PEAKS`](#layout_peaks), [`LAYOUT_SIGNAL`](#layout_signal), [`LAYOUT_NOISE`](#layout_noise), [`LAYOUT_XTALK`](#layout_xtalk), [`LAYOUT_SUBFRAME`](#layout_subframe), [`FRAME_VALID`](#frame_valid), [`FRAME_ABORTED`](#frame_aborted), [`FRAME_WARNINGS`](#frame_warnings), [`FOOTER_DIST_IN_MM`](#footer_dist_in_mm), [`FIFO_TX_UNDERRUN`](#fifo_tx_underrun), [`FIFO_RX_OVERRUN`](#fifo_rx_overrun), [`fp_grid`](#fp_grid), [`fp_name`](#fp_name), [`frame_rows`](#frame_rows), [`FrameError`](#frameerror), [`FrameInfo`](#frameinfo), [`frame_size`](#frame_size), [`parse_frame`](#parse_frame), [`PixelResults`](#pixelresults), [`pixel_size`](#pixel_size), [`parse_results`](#parse_results), [`parse_results_info`](#parse_results_info), [`merge_halves`](#merge_halves), [`HistogramFrame`](#histogramframe)
- **Bootloader / firmware update**: [`HEADER_SIZE`](#header_size), [`MAGIC`](#magic), [`FwdepzHeader`](#fwdepzheader), [`FwdepzError`](#fwdepzerror), [`parse`](#parse)
- **Datasets (record & replay)**: [`THREAD_NAME`](#thread_name), [`PlayerState`](#playerstate), [`DatasetPlayer`](#datasetplayer), [`RecordSink`](#recordsink), [`Recordable`](#recordable), [`SessionRecorder`](#sessionrecorder), [`sr04`](#sr04), [`vl53l4`](#vl53l4), [`vl53l8`](#vl53l8), [`vl53l8_cnh`](#vl53l8_cnh), [`vl53lx`](#vl53lx), [`bno055`](#bno055), [`tmf8829`](#tmf8829), [`bno086_type`](#bno086_type), [`bno086`](#bno086), [`round1`](#round1), [`SCHEMA`](#schema), [`DatasetWriter`](#datasetwriter), [`DatasetError`](#dataseterror), [`Device`](#device), [`RecordValue`](#recordvalue), [`Record`](#record), [`Dataset`](#dataset)
- **Transport**: [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false), [`MAGIC`](#magic), [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`CrcType`](#crctype), [`FramingError`](#framingerror), [`payload_crc_bytes`](#payload_crc_bytes), [`build_packet`](#build_packet), [`Packet`](#packet), [`Event`](#event), [`PacketParser`](#packetparser)
- **Protocol codecs**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`UNSOLICITED`](#unsolicited), [`CodecError`](#codecerror), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`SYNC_PIN_MIN`](#sync_pin_min), [`SYNC_PIN_MAX`](#sync_pin_max), [`pack_sync_time`](#pack_sync_time), [`pack_set_payload_crc_type`](#pack_set_payload_crc_type), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string)
- **JSON**: [`Value`](#value), [`JsonError`](#jsonerror), [`object_map`](#object_map)

## Discovery

### SENSOR_CLASS_NAMES

```rust
pub const SENSOR_CLASS_NAMES: [&str; 18] = [
    "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch", "vl53l0x",
    "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx", "bno086", "bno055", "tmf8829",
    "device", "bootloader",
];
```

Every name [`resolve_sensor_class`] returns: the contract 07 sensor
classes, `"vl53lx"` (the generic 1D-family class: an unstamped board, or a
VL53L4CD on the `APP_VL53L0_4` firmware), `"tmf8829"`, `"device"` (the
base device: an unknown application, or a name that is neither `APP_` nor
`BOOTDEPZ`) and `"bootloader"` (use the contract 06 client). BNO085 boards
run `APP_BNO086` and resolve to `"bno086"`.

### resolve_sensor_class

```rust
pub fn resolve_sensor_class(
    software_name: &str,
    device_name: &str,
    usb_vid: Option<u16>,
    usb_pid: Option<u16>,
) -> &'static str
```

The canonical class name (one of [`SENSOR_CLASS_NAMES`]) for a device's
probe answers and its port's USB id — contract 02 §4.4, pure.

`software_name` is the `GET_NAME_ACTIVE_SOFTWARE` answer, `device_name`
the `GET_DEVICE_NAME` answer (`""` when there was none); the USB id is
`None` when unknown (a replay, a bridge).

- `BOOTDEPZ_*` → `"bootloader"`;
- `vl53l8` → `"vl53l8ch"` if the USB model is VL53L8CH, else `"vl53l8cx"`;
- `vl53l7` → [`vl53l7::resolve_model`] (USB model, else device name, else CX);
- `vl53lx` → [`vl53lx::resolve_class`] (USB model, else device name, else
  the generic `"vl53lx"`; VL53L4CD on this firmware is generic too);
- any other known token → its class; anything else → `"device"`.

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
    /// TMF8829 multizone dToF (`APP_TMF8829` SPI register bridge, contract 14;
    /// the host runs the sensor).
    Tmf8829,
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
pub const PID_TMF8829: u16 = 0xED4C; // 60748 — TMF8829 dToF (catalog p_TM88)
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
    (PID_TMF8829, "tmf8829"),
    (PID_BNO086, "bno086"),
    (0xEE09, "bno085"), // 60937
    (0xEE0A, "bno055"), // 60938
];
```

Dev / unprogrammed default: STMicroelectronics VID/PID.

### DEPZ_PID_MODEL

```rust
pub const DEPZ_PID_MODEL: &[(u16, &str)] = PID_MODEL;
```

The DEPZ production PIDs and the sensor model each names (contract 02 §4.2).

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

## Live devices

### DEFAULT_TIMEOUT

```rust
pub const DEFAULT_TIMEOUT: Duration = Duration::from_millis(200);
```

The default request timeout, 200 ms (contract 07 §3).

### DeviceEvent

```rust
pub enum DeviceEvent {
    /// A gap in packet sequence numbers; `reported_by_device`: the board saw a gap in host TX
    /// (RPT_SEQUENCE_ERROR), else the host saw one in the board's packets.
    SequenceError { expected_seq: u8, received_seq: u8, reported_by_device: bool },
    /// A frame whose payload CRC failed (dropped).
    LinkCrcError { cmd: u8, seq: u8 },
    /// Bytes discarded while hunting for a frame.
    Trash(Vec<u8>),
    /// RPT_STATUS not answering a request in flight.
    UnsolicitedStatus(u8),
    /// RPT_TEXT outside a request, or an unrouted report (`text` is then its payload in hex).
    Text { cmd: u8, text: String },
    /// The MCU temperature the board pushed.
    Temperature { timestamp_us: u64, celsius: f64 },
    /// The link dropped or the reader stopped.
    Disconnected(String),
    /// A sensor class's own event: `kind` names it, `payload` is the report's payload.
    Sensor { kind: &'static str, code: u8, payload: Vec<u8> },
}
```

Something the device or the link reports outside any request (contract 07 §4).

### LinkStats

```rust
pub struct LinkStats {
    pub tx_packets: u64,
    pub tx_bytes: u64,
    pub rx_packets: u64,
    pub rx_bytes: u64,
    pub crc_errors: u64,
    pub header_errors: u64,
    pub trash_bytes: u64,
    pub seq_gaps: u64,
    pub device_seq_errors: u64,
}
```

Link counters (a snapshot).

### TimeSyncResult

```rust
pub struct TimeSyncResult {
    pub offset_us: i64,
    pub rtt_us: i64,
    pub synced_at_host_us: i64,
}
```

Result of [`DepzDevice::sync_time`]: device clock − host clock, the round trip of the kept
sample, and when (host µs) it was taken.

### host_now_us

```rust
pub fn host_now_us() -> i64
```

The host monotonic clock in µs — the host side of all time-sync math.

### ReportHandler

```rust
pub trait ReportHandler: Send + Sync {
    /// Return true when the packet was consumed.
    fn handle_report(&self, packet: &Packet) -> bool;

    /// The reader stopped: end the class's streams.
    fn reader_stopped(&self) {}
}
```

Routes the streaming reports of a sensor class (runs on the reader thread: keep it short).

### DepzDevice

```rust
pub struct DepzDevice {
    pub(crate) core: Arc<Core>,
}
```

A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread, replies
correlated to requests by echoed opcode (one request in flight per opcode), events, and the common
commands. Sensor classes wrap it (and deref to it). Requests block the calling thread up to
[`DepzDevice::timeout`] (default 200 ms). Callbacks run on the reader thread: keep them short,
never block, never close the device from one. Dropping the device closes the link.

#### DepzDevice::new

```rust
pub fn new(link: impl Link + 'static, timeout: Option<Duration>) -> DepzDevice
```

Open a connection on any link; the device owns it and closes it. `timeout` `None`: 200 ms.

#### DepzDevice::set_report_handler

```rust
pub fn set_report_handler(&self, handler: Arc<dyn ReportHandler>)
```

Route this connection's streaming reports to a sensor class (one handler; the last wins).

#### DepzDevice::port

```rust
pub fn port(&self) -> String
```

The link's name: the port path, or what the link wraps.

#### DepzDevice::is_closed

```rust
pub fn is_closed(&self) -> bool
```

#### DepzDevice::timeout

```rust
pub fn timeout(&self) -> Duration
```

The timeout of requests that take none of their own.

#### DepzDevice::set_timeout

```rust
pub fn set_timeout(&self, timeout: Duration) -> Result<()>
```

#### DepzDevice::tx_crc_type

```rust
pub fn tx_crc_type(&self) -> CrcType
```

The payload CRC the host puts on its packets (contract 01).

#### DepzDevice::set_tx_crc_type

```rust
pub fn set_tx_crc_type(&self, crc: CrcType)
```

#### DepzDevice::stats

```rust
pub fn stats(&self) -> LinkStats
```

#### DepzDevice::close

```rust
pub fn close(&self)
```

Close the link and stop the reader; pending requests and streams end. Dropping does the same.

#### DepzDevice::on_event

```rust
pub fn on_event(&self, callback: impl Fn(&DeviceEvent) + Send + Sync + 'static) -> Hook
```

Call `callback` for every event (reader thread). Drop the hook to stop.

#### DepzDevice::events

```rust
pub fn events(&self, capacity: usize) -> Subscription<DeviceEvent>
```

A pull queue of events (bounded, drop-oldest); subscribed when this returns.

#### DepzDevice::emit_event

```rust
pub fn emit_event(&self, event: DeviceEvent)
```

Hand an event to the event subscribers (a sensor class's own events).

#### DepzDevice::send

```rust
pub fn send(&self, cmd: u8, payload: &[u8]) -> Result<()>
```

Send a packet without waiting for anything (escape hatch; prefer a request).

#### DepzDevice::request

```rust
pub fn request<T: Send + 'static>(
    &self,
    cmd: u8,
    payload: &[u8],
    matcher: impl Fn(&Packet) -> Option<Result<T>> + Send + 'static,
    timeout: Option<Duration>,
) -> Result<T>
```

Send `cmd` and wait for the first packet `matcher` claims (escape hatch, contract 07 §2).
The matcher answers `None` for "not mine", `Some(Err)` for "mine, but it does not decode". A
non-OK RPT_STATUS echoing `cmd` fails with [`DepzError::Status`]; one request per opcode may
be in flight.

#### DepzDevice::request_ok

```rust
pub fn request_ok(&self, cmd: u8, payload: &[u8], timeout: Option<Duration>) -> Result<()>
```

Send `cmd` and wait for RPT_STATUS OK echoing it.

#### DepzDevice::get_device_name

```rust
pub fn get_device_name(&self) -> Result<String>
```

#### DepzDevice::get_software_name

```rust
pub fn get_software_name(&self) -> Result<String>
```

#### DepzDevice::get_serial_number

```rust
pub fn get_serial_number(&self) -> Result<String>
```

#### DepzDevice::identify

```rust
pub fn identify(&self) -> Result<Identity>
```

The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4).

#### DepzDevice::read_mcu_temperature

```rust
pub fn read_mcu_temperature(&self) -> Result<f64>
```

The MCU temperature the device last cached, °C (it refreshes about twice a second).

#### DepzDevice::sync_time

```rust
pub fn sync_time(&self, samples: usize) -> Result<TimeSyncResult>
```

NTP-style time sync keeping the lowest-RTT of `samples` rounds (contract 02 §5).

#### DepzDevice::last_time_sync

```rust
pub fn last_time_sync(&self) -> Option<TimeSyncResult>
```

The last [`DepzDevice::sync_time`] result.

#### DepzDevice::to_host_time_us

```rust
pub fn to_host_time_us(&self, device_timestamp_us: u64) -> Result<i64>
```

Device µs → host monotonic µs ([`host_now_us`]); needs a [`DepzDevice::sync_time`] first.

#### DepzDevice::get_report_payload_crc

```rust
pub fn get_report_payload_crc(&self) -> Result<CrcType>
```

The device → host payload CRC mode.

#### DepzDevice::set_report_payload_crc

```rust
pub fn set_report_payload_crc(&self, crc: CrcType) -> Result<()>
```

Set the device → host payload CRC mode (host → device is per packet: [`DepzDevice::set_tx_crc_type`]).

#### DepzDevice::get_sync_pin

```rust
pub fn get_sync_pin(&self, pin: u8) -> Result<SyncPinConfig>
```

#### DepzDevice::set_sync_pin

```rust
pub fn set_sync_pin(&self, config: &SyncPinConfig) -> Result<()>
```

#### DepzDevice::reset

```rust
pub fn reset(&self) -> Result<()>
```

DEVICE_RESET: the device acknowledges, then reboots; the link drops.

#### DepzDevice::enter_bootloader_mode

```rust
pub fn enter_bootloader_mode(&self) -> Result<()>
```

Reboot into the resident bootloader and close this connection (flashing: contract 06).

### expect_report

```rust
pub fn expect_report<T, E: Into<DepzError>>(
    report_id: u8,
    unpack: impl Fn(&[u8]) -> std::result::Result<T, E> + Send + 'static,
) -> impl Fn(&Packet) -> Option<Result<T>> + Send + 'static
```

A matcher for a report identified by its report id alone.

### expect_text

```rust
pub fn expect_text(request_cmd: u8) -> impl Fn(&Packet) -> Option<Result<String>> + Send + 'static
```

A matcher for RPT_TEXT echoing `request_cmd`.

### DepzError

```rust
pub enum DepzError {
    /// No reply (or no stream item) in time. `cmd` is the request or report id waited for.
    Timeout { cmd: Option<u8>, message: String },
    /// The device answered RPT_STATUS with a non-OK status for `cmd` (ERR_BUSY: [`DepzError::is_busy`]).
    Status { cmd: u8, status: u8 },
    /// The device is gone (unplugged, the link failed) or the stream ended.
    DeviceLost(String),
    /// The link was closed by this side.
    LinkClosed(String),
    /// Nothing DEPZ answered.
    NoDevice(String),
    /// The device runs its bootloader, not an application.
    BootloaderMode,
    /// The device is not of the class asked for.
    WrongType(String),
    /// A strict replay saw the host write bytes the capture does not hold.
    ReplayMismatch(String),
    /// The SDK refused an argument before sending anything.
    InvalidArgument(String),
    /// The product / driver does not serve this.
    Unsupported(String),
    /// Not in this state (e.g. configuring while streaming).
    InvalidState(String),
    /// A reply that does not decode.
    Protocol(String),
    /// A sensor-level failure (a driver status, a refused command).
    Sensor(String),
    /// The OS refused (a port, a file).
    Io(std::io::Error),
}
```

What a live-device call can fail with. Arguments the SDK refuses before sending anything are
[`DepzError::InvalidArgument`].

#### DepzError::timeout

```rust
pub fn timeout(cmd: u8, timeout: Duration) -> DepzError
```

A request timeout for `cmd` after `timeout`.

#### DepzError::is_busy

```rust
pub fn is_busy(&self) -> bool
```

RPT_STATUS ERR_BUSY (or a second request of an opcode already in flight).

#### DepzError::kind

```rust
pub fn kind(&self) -> Option<&'static str>
```

The canonical error kind of contract 15 §4, or `None` for the rest.

### Result

```rust
pub type Result<T> = std::result::Result<T, DepzError>;
```

`Result` with [`DepzError`].

### Link

```rust
pub trait Link: Send + Sync {
    /// The next available chunk (one byte or more), or an empty vector when nothing arrived within
    /// `timeout`. [`DepzError::LinkClosed`] / [`DepzError::DeviceLost`] when the link is gone.
    fn read(&self, timeout: Duration) -> Result<Vec<u8>>;

    fn write(&self, data: &[u8]) -> Result<()>;

    fn close(&self);

    fn is_closed(&self) -> bool;

    /// A name for messages: the port path, or what the link wraps.
    fn name(&self) -> String;
}
```

A blocking byte pipe to one device: a serial port, a `.depzrec` record / replay wrapper, or
anything a host supplies. Device classes accept any link, which keeps every layer above the port
testable without hardware. Implementations are shared between the reader thread and callers, so
every method takes `&self`.

### ReplayLink

```rust
pub struct ReplayLink {
    rx: Vec<Rx>,
    tx: Vec<u8>,
    strict_tx: bool,
    realtime: bool,
    t0: Instant,
    name: String,
    header: Value,
    state: Mutex<ReplayState>,
    cond: Condvar,
}
```

Replays the rx side of a `.depzrec` capture causally (contract 08): an rx chunk is served only
after the host has written as many tx bytes as preceded it in the capture. With `strict_tx` every
written byte must also equal the recorded tx stream ([`DepzError::ReplayMismatch`]).

#### ReplayLink::open

```rust
pub fn open(path: impl AsRef<Path>, strict_tx: bool, realtime: bool) -> Result<ReplayLink>
```

Load `path`; `strict_tx` fails a write that differs from the recorded tx stream, `realtime`
paces rx chunks by their recorded timestamps.

#### ReplayLink::header

```rust
pub fn header(&self) -> &Value
```

The capture's header object (contract 08).

#### ReplayLink::rx_exhausted

```rust
pub fn rx_exhausted(&self) -> bool
```

Every rx chunk was delivered.

#### ReplayLink::rx_remaining

```rust
pub fn rx_remaining(&self) -> usize
```

Recorded rx chunks not delivered yet.

#### ReplayLink::tx_remaining

```rust
pub fn tx_remaining(&self) -> usize
```

Recorded tx bytes the host has not written yet.

### RecordingLink

```rust
pub struct RecordingLink<L: Link> {
    inner: L,
    file: Mutex<Option<BufWriter<File>>>,
    t0: Instant,
}
```

Wraps any link and tees both directions into a `.depzrec` file (contract 08). TX is journaled
before the physical write: a fast device reply could otherwise reach the reader and be journaled
ahead of its own request, which breaks the causal rx gating replay relies on.

#### RecordingLink::create

```rust
pub fn create(inner: L, path: impl AsRef<Path>, header_extra: Vec<(String, Value)>) -> Result<RecordingLink<L>>
```

Record `inner` into `path` (overwritten); `header_extra` adds header fields (contract 08),
e.g. `device`. Closing this link closes `inner`.

### RECORDING_SCHEMA

```rust
pub const RECORDING_SCHEMA: &str = "depz.rec/1";
```

The schema id of a `.depzrec` capture.

### ProbeResult

```rust
pub struct ProbeResult {
    pub software_name: String,
    pub device_name: String,
    pub serial_number: String,
}
```

The probe's answers: software name, device name, serial number.

### AnyDevice

```rust
pub enum AnyDevice {
    Plain(DepzDevice),
    Sr04(Sr04Device),
    Vl53l4cd(Vl53l4cdDevice),
    Bno055(Bno055Device),
    Bno086(Bno086Device),
    Vl53l8cx(Vl53l8cxDevice),
    Vl53l8ch(Vl53l8chDevice),
    Vl53l5cx(Vl53l5cxDevice),
    Vl53l7cx(Vl53l7cxDevice),
    Vl53l7ch(Vl53l7chDevice),
    Tmf8829(Tmf8829Device),
    Vl53lx(Vl53lxDevice),
    Vl53l0x(Vl53l0xDevice),
    Vl53l1cx(Vl53l1cxDevice),
    Vl53l1cb(Vl53l1cbDevice),
    Vl53l3cx(Vl53l3cxDevice),
    Vl53l4cx(Vl53l4cxDevice),
}
```

A device opened as its sensor class. `Plain` is a device this SDK has no class for (or
`"device"`): the common commands work.

#### AnyDevice::device

```rust
pub fn device(&self) -> &DepzDevice
```

The connection, whatever the class.

#### AnyDevice::class_name

```rust
pub fn class_name(&self) -> &'static str
```

The canonical class name (contract 07 §1).

#### AnyDevice::into_sr04

```rust
pub fn into_sr04(self) -> Result<Sr04Device>
```

The SR04 class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l4cd

```rust
pub fn into_vl53l4cd(self) -> Result<Vl53l4cdDevice>
```

The VL53L4CD class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_bno055

```rust
pub fn into_bno055(self) -> Result<Bno055Device>
```

The BNO055 class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_bno086

```rust
pub fn into_bno086(self) -> Result<Bno086Device>
```

The BNO085 / BNO086 class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l8cx

```rust
pub fn into_vl53l8cx(self) -> Result<Vl53l8cxDevice>
```

The VL53L8CX class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l8ch

```rust
pub fn into_vl53l8ch(self) -> Result<Vl53l8chDevice>
```

The VL53L8CH class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l5cx

```rust
pub fn into_vl53l5cx(self) -> Result<Vl53l5cxDevice>
```

The VL53L5CX class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l7cx

```rust
pub fn into_vl53l7cx(self) -> Result<Vl53l7cxDevice>
```

The VL53L7CX class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53l7ch

```rust
pub fn into_vl53l7ch(self) -> Result<Vl53l7chDevice>
```

The VL53L7CH class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_tmf8829

```rust
pub fn into_tmf8829(self) -> Result<Tmf8829Device>
```

The TMF8829 class, or [`DepzError::WrongType`] (the device is closed).

#### AnyDevice::into_vl53lx

```rust
pub fn into_vl53lx(self) -> Result<Vl53lxDevice>
```

A board of the VL53L 1D family, whichever of its classes (the per-product ones give their
family class, product kept), or [`DepzError::WrongType`] (the device is closed).

### probe

```rust
pub fn probe(dev: &DepzDevice) -> Result<ProbeResult>
```

The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE (required: its
failure fails the probe), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error leaves "").

### attach

```rust
pub fn attach(dev: DepzDevice, class_name: &str) -> Result<AnyDevice>
```

Continue the open plain device `dev` as `class_name` (a name [`resolve_sensor_class`] returns)
on the same connection — no reopen; the reader and the host sequence numbers run on (contract 02
§4.4). `"device"`, and a class this SDK has no device layer for, give [`AnyDevice::Plain`].

### open_on_link

```rust
pub fn open_on_link(
    link: impl Link + 'static,
    usb_vid: Option<u16>,
    usb_pid: Option<u16>,
    timeout: Option<Duration>,
) -> Result<AnyDevice>
```

Open a device on a connection that cannot be reopened — a replay or recording link, a link the
host opened (contract 02 §4.4). Runs the probe on `link`, resolves the class from the answers and
the port's USB id (`None`: unknown; a link knows none) and continues as that class on the same
connection. The device owns the link; on failure it is closed.

### Subscription

```rust
pub struct Subscription<T> {
    queue: Arc<Queue<T>>,
    fanout: Weak<FanoutInner<T>>,
}
```

A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest item
when full and counts the drops. It is registered when created, so nothing produced after that is
missed. Dropping it unsubscribes.

#### Subscription::dropped_count

```rust
pub fn dropped_count(&self) -> u64
```

Items dropped because the queue was full (monotonic).

#### Subscription::len

```rust
pub fn len(&self) -> usize
```

Items waiting.

#### Subscription::is_empty

```rust
pub fn is_empty(&self) -> bool
```

#### Subscription::is_closed

```rust
pub fn is_closed(&self) -> bool
```

The device closed: no more items will arrive.

#### Subscription::try_take

```rust
pub fn try_take(&self) -> Option<T>
```

The oldest waiting item, without blocking.

#### Subscription::take

```rust
pub fn take(&self, timeout: Duration) -> Result<T>
```

The oldest item, waiting up to `timeout` for one: [`DepzError::Timeout`] when nothing
arrived, [`DepzError::DeviceLost`] when the device closed with the queue empty.

#### Subscription::drain

```rust
pub fn drain(&self) -> Vec<T>
```

Everything waiting now, oldest first, without blocking.

### Hook

```rust
pub struct Hook {
    undo: Option<Box<dyn FnOnce() + Send>>,
}
```

A registered callback; dropping it (or [`Hook::cancel`]) unregisters. [`Hook::forget`] keeps the
callback for the device's lifetime.

#### Hook::cancel

```rust
pub fn cancel(mut self)
```

Unregister now.

#### Hook::forget

```rust
pub fn forget(mut self)
```

Keep the callback registered for as long as the device lives.

## SR04

### Sr04Measurement

```rust
pub struct Sr04Measurement {
    pub timestamp_us: u64,
    pub echo_time_us: u16,
    pub source: &'static str,
}
```

One SR04 ranging result. `source` is `"once"` (a host command or a SYNC_IN edge) or `"loop"`;
[`Sr04Measurement::valid`] is false for the no-echo timeout.

#### Sr04Measurement::valid

```rust
pub fn valid(&self) -> bool
```

#### Sr04Measurement::distance_mm

```rust
pub fn distance_mm(&self) -> Option<f64>
```

Distance at 343 m/s, or `None` when no echo came back.

#### Sr04Measurement::distance_mm_at

```rust
pub fn distance_mm_at(&self, air_temp_c: f64) -> Option<f64>
```

Distance with the speed of sound for `air_temp_c` °C.

### Sr04Device

```rust
pub struct Sr04Device {
    dev: DepzDevice,
    streams: Arc<Streams>,
}
```

HC-SR04 ultrasonic ranger (contract 03). Measurements stream to [`Sr04Device::on_measurement`]
callbacks and [`Sr04Device::measurements`] queues: loop samples and the single shots an AUX
SYNC_IN edge triggers. Derefs to [`DepzDevice`] for the common commands.

#### Sr04Device::attach

```rust
pub fn attach(dev: DepzDevice) -> Sr04Device
```

Continue the open connection `dev` as an SR04 (or open one on a link with [`DepzDevice::new`]).

#### Sr04Device::device

```rust
pub fn device(&self) -> &DepzDevice
```

The underlying connection.

#### Sr04Device::get_sample_period_us

```rust
pub fn get_sample_period_us(&self) -> Result<u32>
```

The stored minimum interval between measurement starts, µs (default 50000) — not the
effective rate, which the echo window throttles (contract 03 §3).

#### Sr04Device::set_sample_period_us

```rust
pub fn set_sample_period_us(&self, period_us: u32) -> Result<()>
```

Set the minimum interval between measurement starts, µs (u32 wire field).

#### Sr04Device::get_echo_decay_us

```rust
pub fn get_echo_decay_us(&self) -> Result<u16>
```

The post-echo settle pause, µs (4000–65000).

#### Sr04Device::set_echo_decay_us

```rust
pub fn set_echo_decay_us(&self, decay_us: u16) -> Result<u16>
```

Set the settle pause. The device clamps it to 4000–65000 µs silently, so this reads it back and
returns the value in effect.

#### Sr04Device::measure_once

```rust
pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Sr04Measurement>
```

One shot. The reply comes when the echo completes (or times out at ~65.5 ms), hence the 1 s
default. Busy while the loop runs.

#### Sr04Device::start

```rust
pub fn start(&self) -> Result<()>
```

Start the measurement loop (idempotent).

#### Sr04Device::stop

```rust
pub fn stop(&self) -> Result<()>
```

Stop the measurement loop (idempotent).

#### Sr04Device::on_measurement

```rust
pub fn on_measurement(&self, callback: impl Fn(&Sr04Measurement) + Send + Sync + 'static) -> Hook
```

Call `callback` for every measurement (reader thread). Drop the hook to stop.

#### Sr04Device::measurements

```rust
pub fn measurements(&self, capacity: usize) -> Subscription<Sr04Measurement>
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Sr04Device::get_measurement

```rust
pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Sr04Measurement>
```

The next measurement (contract 07 §3); `timeout` `None`: 2 s.

#### Sr04Device::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

The drop counter of each open [`Sr04Device::measurements`] queue.

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

### Vl53l4Measurement

```rust
pub struct Vl53l4Measurement {
    pub timestamp_us: u64,
    pub range_status: u8,
    pub distance_mm: u16,
    pub sigma_mm: u16,
    pub signal_rate_kcps: u32,
    pub ambient_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub number_of_spad: u16,
    pub stream_count: u8,
}
```

One VL53L4CD result block with the MCU timestamp of the read or the INT edge.

#### Vl53l4Measurement::valid

```rust
pub fn valid(&self) -> bool
```

#### Vl53l4Measurement::status_text

```rust
pub fn status_text(&self) -> String
```

What `range_status` means ("unknown (N)" for a code the table lacks).

### Vl53l4cdDevice

```rust
pub struct Vl53l4cdDevice {
    dev: DepzDevice,
    streams: Arc<Streams>,
    platform: Arc<BridgePlatform>,
    uld: Vl53l4cdDriver,
    ranging: AtomicBool,
    initialized: AtomicBool,
}
```

VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs here
([`Vl53l4cdDevice::uld`]). [`Vl53l4cdDevice::init`] first, then configure and
[`Vl53l4cdDevice::start_ranging`]: one result block per INT edge streams to
[`Vl53l4cdDevice::measurements`]. Configuration while ranging fails with
[`DepzError::InvalidState`] — the stream owns the register bank.

#### Vl53l4cdDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l4cdDevice
```

Continue the open connection `dev` as a VL53L4CD.

#### Vl53l4cdDevice::uld

```rust
pub fn uld(&self) -> &Vl53l4cdDriver
```

The ULD driver (escape hatch for raw register access).

#### Vl53l4cdDevice::initialized

```rust
pub fn initialized(&self) -> bool
```

True after a successful [`Vl53l4cdDevice::init`]; cleared by [`Vl53l4cdDevice::xshut`].

#### Vl53l4cdDevice::ranging

```rust
pub fn ranging(&self) -> bool
```

#### Vl53l4cdDevice::is_alive

```rust
pub fn is_alive(&self) -> bool
```

True when the sensor answers with the VL53L4CD model id (0xEBAA).

#### Vl53l4cdDevice::init

```rust
pub fn init(&self, bus_khz: Option<u16>) -> Result<()>
```

Default configuration block + VHV calibration (ULD sensor_init); the bus is left at `bus_khz`
(`None`: 1000).

#### Vl53l4cdDevice::xshut

```rust
pub fn xshut(&self, action: u8) -> Result<()>
```

Drive XSHUT: off (0) / on (1) / reset (2). Off and reset stop a stream; init again after.

#### Vl53l4cdDevice::reset_sensor

```rust
pub fn reset_sensor(&self) -> Result<()>
```

Hardware reset through XSHUT (~3 ms on the MCU); init again.

#### Vl53l4cdDevice::bridge_info

```rust
pub fn bridge_info(&self) -> Result<Vl53l4Info>
```

RPT_VL53_INFO: sensor identity, pin levels and free-running bridge counters. Safe while streaming.

#### Vl53l4cdDevice::set_i2c_speed_khz

```rust
pub fn set_i2c_speed_khz(&self, khz: u16) -> Result<()>
```

Re-time the bridge's I2C bus to the step nearest `khz`. Not while ranging.

#### Vl53l4cdDevice::get_range_timing

```rust
pub fn get_range_timing(&self) -> Result<(u32, u32)>
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDevice::set_range_timing

```rust
pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()>
```

Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget = autonomous
low power.

#### Vl53l4cdDevice::get_offset_mm

```rust
pub fn get_offset_mm(&self) -> Result<i32>
```

#### Vl53l4cdDevice::set_offset_mm

```rust
pub fn set_offset_mm(&self, offset_mm: i32) -> Result<()>
```

#### Vl53l4cdDevice::get_xtalk_kcps

```rust
pub fn get_xtalk_kcps(&self) -> Result<u16>
```

Crosstalk compensation, kcps (0 = off).

#### Vl53l4cdDevice::set_xtalk_kcps

```rust
pub fn set_xtalk_kcps(&self, xtalk_kcps: u16) -> Result<()>
```

#### Vl53l4cdDevice::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<DetectionThresholds>
```

#### Vl53l4cdDevice::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()>
```

The distance-window interrupt: INT only fires while the window holds.

#### Vl53l4cdDevice::get_signal_threshold_kcps

```rust
pub fn get_signal_threshold_kcps(&self) -> Result<u16>
```

#### Vl53l4cdDevice::set_signal_threshold_kcps

```rust
pub fn set_signal_threshold_kcps(&self, signal_kcps: u16) -> Result<()>
```

Drop measurements whose return signal is below `signal_kcps`.

#### Vl53l4cdDevice::get_sigma_threshold_mm

```rust
pub fn get_sigma_threshold_mm(&self) -> Result<u16>
```

#### Vl53l4cdDevice::set_sigma_threshold_mm

```rust
pub fn set_sigma_threshold_mm(&self, sigma_mm: u16) -> Result<()>
```

Drop measurements whose sigma exceeds `sigma_mm` (≤ 16383).

#### Vl53l4cdDevice::start_temperature_update

```rust
pub fn start_temperature_update(&self) -> Result<()>
```

Re-run VHV calibration, recommended after a >8 °C ambient change.

#### Vl53l4cdDevice::calibrate_offset

```rust
pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Offset calibration against a target at `target_dist_mm` (10–1000 mm); returns the offset now
programmed. `nb_samples` `None`: 20.

#### Vl53l4cdDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Crosstalk calibration against a target at `target_dist_mm` (10–5000 mm); returns the xtalk now
programmed, kcps. `nb_samples` `None`: 20.

#### Vl53l4cdDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge.

#### Vl53l4cdDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53l4cdDevice::measure_once

```rust
pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Vl53l4Measurement>
```

One polled measurement: start, wait for data ready, read, stop. Not while streaming. `timeout`
`None`: 1 s.

#### Vl53l4cdDevice::on_measurement

```rust
pub fn on_measurement(&self, callback: impl Fn(&Vl53l4Measurement) + Send + Sync + 'static) -> Hook
```

Call `callback` for every streamed measurement (reader thread).

#### Vl53l4cdDevice::measurements

```rust
pub fn measurements(&self, capacity: usize) -> Subscription<Vl53l4Measurement>
```

A pull queue of streamed measurements (bounded, drop-oldest).

#### Vl53l4cdDevice::get_measurement

```rust
pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Vl53l4Measurement>
```

The next streamed measurement (contract 07 §3); `timeout` `None`: 2 s.

#### Vl53l4cdDevice::stream_parse_errors

```rust
pub fn stream_parse_errors(&self) -> u64
```

Stream reports dropped because their result block did not decode.

#### Vl53l4cdDevice::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

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

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u16, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`VL53_WRITE_REG` payload: `addr u16` followed by 1..=[`XFER_MAX`] raw
register bytes; any other data length is refused (contract 07 §3).

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Vec<u8>
```

`VL53_WRITE_REG` payload: `addr u16` followed by the raw register bytes,
unchecked — prefer [`try_pack_write_reg`].

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

### Vl53l4Platform

```rust
pub trait Vl53l4Platform: Send + Sync {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(std::time::Duration::from_millis(ms as u64));
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()>;
}
```

What the ULD needs from the board: the register bridge (16-bit addresses, auto-increment), a delay,
and the bus speed. `Vl53l4cdDevice` maps it onto the firmware bridge; a test can script it.

### DetectionThresholds

```rust
pub struct DetectionThresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}
```

The distance-window interrupt: low and high distance, mm, and the window
([`uld::WINDOW_BELOW`] … [`uld::WINDOW_IN`]).

### Vl53l4cdDriver

```rust
pub struct Vl53l4cdDriver {
    p: Arc<dyn Vl53l4Platform>,
}
```

The ST VL53L4CD ULD 2.2.3 on the host, ported from the reference Python driver: init, ranging,
timing, offset, crosstalk, thresholds, temperature update, calibration. Every request sequence is
the Python one, so captures of either replay in both. Register contents are big-endian.

#### Vl53l4cdDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53l4Platform>) -> Vl53l4cdDriver
```

#### Vl53l4cdDriver::read_byte

```rust
pub fn read_byte(&self, addr: u16) -> Result<u8>
```

#### Vl53l4cdDriver::read_word

```rust
pub fn read_word(&self, addr: u16) -> Result<u16>
```

#### Vl53l4cdDriver::read_dword

```rust
pub fn read_dword(&self, addr: u16) -> Result<u32>
```

#### Vl53l4cdDriver::write_byte

```rust
pub fn write_byte(&self, addr: u16, value: u8) -> Result<()>
```

#### Vl53l4cdDriver::write_word

```rust
pub fn write_word(&self, addr: u16, value: u16) -> Result<()>
```

#### Vl53l4cdDriver::write_dword

```rust
pub fn write_dword(&self, addr: u16, value: u32) -> Result<()>
```

#### Vl53l4cdDriver::get_sensor_id

```rust
pub fn get_sensor_id(&self) -> Result<u16>
```

#### Vl53l4cdDriver::is_alive

```rust
pub fn is_alive(&self) -> Result<bool>
```

#### Vl53l4cdDriver::wait_boot

```rust
pub fn wait_boot(&self, timeout_ms: u32) -> Result<()>
```

Fails when FIRMWARE__SYSTEM_STATUS never reads 0x03.

#### Vl53l4cdDriver::sensor_init

```rust
pub fn sensor_init(&self, bus_khz: u16) -> Result<()>
```

Initialise the sensor and leave the bus at `bus_khz`. The configuration block goes at 400 kHz,
the only speed an unconfigured sensor is specified for; the bridge is re-timed after.

#### Vl53l4cdDriver::clear_interrupt

```rust
pub fn clear_interrupt(&self) -> Result<()>
```

#### Vl53l4cdDriver::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

#### Vl53l4cdDriver::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53l4cdDriver::check_for_data_ready

```rust
pub fn check_for_data_ready(&self) -> Result<bool>
```

#### Vl53l4cdDriver::wait_data_ready

```rust
pub fn wait_data_ready(&self, timeout_ms: u32) -> Result<()>
```

Fails when no data is ready within `timeout_ms` polls.

#### Vl53l4cdDriver::get_result

```rust
pub fn get_result(&self) -> Result<Vl53l4Results>
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Vl53l4cdDriver::set_range_timing

```rust
pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()>
```

#### Vl53l4cdDriver::get_range_timing

```rust
pub fn get_range_timing(&self) -> Result<(u32, u32)>
```

(timing budget ms, inter-measurement ms; 0 = continuous).

#### Vl53l4cdDriver::set_offset

```rust
pub fn set_offset(&self, offset_mm: i32) -> Result<()>
```

#### Vl53l4cdDriver::get_offset

```rust
pub fn get_offset(&self) -> Result<i32>
```

#### Vl53l4cdDriver::set_xtalk

```rust
pub fn set_xtalk(&self, xtalk_kcps: u16) -> Result<()>
```

#### Vl53l4cdDriver::get_xtalk

```rust
pub fn get_xtalk(&self) -> Result<u16>
```

#### Vl53l4cdDriver::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()>
```

The distance-window interrupt; `window` is one of [`uld::WINDOW_BELOW`] … [`uld::WINDOW_IN`].

#### Vl53l4cdDriver::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<DetectionThresholds>
```

#### Vl53l4cdDriver::set_signal_threshold

```rust
pub fn set_signal_threshold(&self, signal_kcps: u16) -> Result<()>
```

#### Vl53l4cdDriver::get_signal_threshold

```rust
pub fn get_signal_threshold(&self) -> Result<u16>
```

#### Vl53l4cdDriver::set_sigma_threshold

```rust
pub fn set_sigma_threshold(&self, sigma_mm: u16) -> Result<()>
```

#### Vl53l4cdDriver::get_sigma_threshold

```rust
pub fn get_sigma_threshold(&self) -> Result<u16>
```

#### Vl53l4cdDriver::start_temperature_update

```rust
pub fn start_temperature_update(&self) -> Result<()>
```

Re-run VHV; recommended after a >8 °C ambient change (ST Example_3).

#### Vl53l4cdDriver::calibrate_offset

```rust
pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: u32) -> Result<i32>
```

Offset calibration against a target at `target_dist_mm` (10–1000); returns the offset now
programmed, mm.

#### Vl53l4cdDriver::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: u32) -> Result<i32>
```

Crosstalk calibration against a target at `target_dist_mm` (10–5000); returns the xtalk now
programmed, kcps (C rounding).

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
    /// Autonomous mode: `1.055 × inter_measurement_ms × clock_pll` does not
    /// fit the u32 `INTERMEASUREMENT_MS` register.
    InterMeasurementOverflow,
    /// `osc_frequency` so small that the macro period truncates to 0 (the
    /// ULD would divide by it).
    MacroPeriodZero,
    /// `offset_mm` outside the register's 11-bit signed range -1024..=1023.
    OffsetOutOfRange,
    /// `xtalk_kcps` outside the register's 7-bit range 0..=127.
    XtalkOutOfRange,
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

Rejected (contract 07 §3, never wrapped): an `osc_frequency` of 0 or so
small that the macro period truncates to 0
([`Vl53l4Error::MacroPeriodZero`]), and an autonomous period whose
register value overflows u32 ([`Vl53l4Error::InterMeasurementOverflow`]).

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

### try_offset_raw

```rust
pub fn try_offset_raw(offset_mm: i32) -> Result<u16, Vl53l4Error>
```

`RANGE_OFFSET_MM` word for `SetOffset` (`INNER`/`OUTER` are zeroed
alongside); `offset_mm` must fit the register's 11-bit signed range
-1024..=1023 ([`Vl53l4Error::OffsetOutOfRange`] otherwise).

### decode_offset

```rust
pub fn decode_offset(raw_word: u16) -> i32
```

`GetOffset`: `RANGE_OFFSET_MM` word → signed millimetres.

### try_xtalk_raw

```rust
pub fn try_xtalk_raw(xtalk_kcps: u16) -> Result<u16, Vl53l4Error>
```

`XTALK_PLANE_OFFSET_KCPS` word for `SetXtalk`; `xtalk_kcps` must be
0..=127 — the value the 16-bit word holds at 9 fractional bits
([`Vl53l4Error::XtalkOutOfRange`] otherwise).

### decode_xtalk

```rust
pub fn decode_xtalk(raw_word: u16) -> u16
```

`GetXtalk`: `XTALK_PLANE_OFFSET_KCPS` word → kcps (an exact .5 goes away
from zero, as C's `round()` in ST's ULD).

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

### MIN_RANGING_FREQUENCY_HZ

```rust
pub const MIN_RANGING_FREQUENCY_HZ: i32 = 2;
```

Below this the VL53L8 never enters its ranging loop and streams nothing (contract 04).

### BridgeUld

```rust
pub type BridgeUld = Vl53l8Driver<Arc<dyn Vl53l8Platform>>;
```

The ULD driver a VL53L8 / L5 / L7 device runs ([`Vl53l8cxDevice::with_uld`]).

### Vl53l8cxDevice

```rust
pub struct Vl53l8cxDevice {
    dev: DepzDevice,
    shared: Arc<Shared>,
    platform: Arc<BridgePlatform>,
    profile: &'static Profile,
    ranging: AtomicBool,
    armed_cnh: Arc<Mutex<Option<Vl53l8CnhSetup>>>,
}
```

VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here
([`Vl53l8cxDevice::with_uld`]). [`Vl53l8cxDevice::init`] downloads the ~84 KB sensor firmware
(about a second), then configure and [`Vl53l8cxDevice::start_ranging`]: the board pushes one
frame per INT edge in chunks, reassembled and parsed into [`Vl53l8cxDevice::frames`].
Configuration needs `init` and fails with [`DepzError::InvalidState`] while ranging — the stream
owns the register bank. The VL53L8CH superset is [`Vl53l8chDevice`]; the L5/L7 classes deref to
this one too.

#### Vl53l8cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l8cxDevice
```

Continue the open connection `dev` as a VL53L8CX.

#### Vl53l8cxDevice::with_uld

```rust
pub fn with_uld<R>(&self, f: impl FnOnce(&mut BridgeUld) -> UldResult<R>) -> Result<R>
```

Run `f` on the ULD driver (escape hatch for DCI access). [`DepzError::InvalidState`] before
[`Vl53l8cxDevice::init`].

#### Vl53l8cxDevice::firmware

```rust
pub fn firmware(&self) -> Vl53l8Firmware
```

The sensor firmware [`Vl53l8cxDevice::init`] downloads (cx / ch / l7cx / l7ch).

#### Vl53l8cxDevice::initialized

```rust
pub fn initialized(&self) -> bool
```

True after a successful [`Vl53l8cxDevice::init`], until the sensor loses its state.

#### Vl53l8cxDevice::ranging

```rust
pub fn ranging(&self) -> bool
```

#### Vl53l8cxDevice::is_alive

```rust
pub fn is_alive(&self) -> bool
```

True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).

#### Vl53l8cxDevice::init

```rust
pub fn init(&self) -> Result<()>
```

Download the sensor firmware and the default configuration.

#### Vl53l8cxDevice::init_with_progress

```rust
pub fn init_with_progress(&self, progress: &mut dyn FnMut(&str), write_progress: Option<WriteProgress>) -> Result<()>
```

[`Vl53l8cxDevice::init`] reporting its phases to `progress` and the big firmware writes to
`write_progress` (done, total bytes). On L5/L7, a module type other than the class expects
(a board labelled L7 carrying an L5, or the reverse) is a warning on stderr: ranging still
works, the firmware is shared.

#### Vl53l8cxDevice::get_resolution

```rust
pub fn get_resolution(&self) -> Result<usize>
```

Zones in use: 16 (4×4) or 64 (8×8).

#### Vl53l8cxDevice::set_resolution

```rust
pub fn set_resolution(&self, zones: usize) -> Result<()>
```

16 (4×4) or 64 (8×8) zones.

#### Vl53l8cxDevice::get_ranging_frequency_hz

```rust
pub fn get_ranging_frequency_hz(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_ranging_frequency_hz

```rust
pub fn set_ranging_frequency_hz(&self, hz: i32) -> Result<()>
```

At least 2 Hz (1 Hz on L5/L7); at most 60 Hz at 4×4 and 15 Hz at 8×8.

#### Vl53l8cxDevice::get_ranging_mode

```rust
pub fn get_ranging_mode(&self) -> Result<u8>
```

[`crate::vl53l8::driver::RANGING_MODE_CONTINUOUS`] or
[`crate::vl53l8::driver::RANGING_MODE_AUTONOMOUS`].

#### Vl53l8cxDevice::set_ranging_mode

```rust
pub fn set_ranging_mode(&self, mode: u8) -> Result<()>
```

#### Vl53l8cxDevice::get_integration_time_ms

```rust
pub fn get_integration_time_ms(&self) -> Result<u32>
```

#### Vl53l8cxDevice::set_integration_time_ms

```rust
pub fn set_integration_time_ms(&self, ms: i32) -> Result<()>
```

2–1000 ms; autonomous mode only.

#### Vl53l8cxDevice::get_sharpener_percent

```rust
pub fn get_sharpener_percent(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_sharpener_percent

```rust
pub fn set_sharpener_percent(&self, pct: i32) -> Result<()>
```

Edge sharpener, 0–99 % (0 = off).

#### Vl53l8cxDevice::get_target_order

```rust
pub fn get_target_order(&self) -> Result<u8>
```

[`crate::vl53l8::driver::TARGET_ORDER_CLOSEST`] or
[`crate::vl53l8::driver::TARGET_ORDER_STRONGEST`].

#### Vl53l8cxDevice::set_target_order

```rust
pub fn set_target_order(&self, order: u8) -> Result<()>
```

#### Vl53l8cxDevice::get_power_mode

```rust
pub fn get_power_mode(&self) -> Result<u8>
```

[`crate::vl53l8::advanced::POWER_MODE_SLEEP`], `POWER_MODE_WAKEUP` or `POWER_MODE_DEEP_SLEEP`.

#### Vl53l8cxDevice::set_power_mode

```rust
pub fn set_power_mode(&self, mode: u8) -> Result<()>
```

Waking from deep sleep downloads the firmware again.

#### Vl53l8cxDevice::get_xtalk_margin

```rust
pub fn get_xtalk_margin(&self) -> Result<f64>
```

#### Vl53l8cxDevice::set_xtalk_margin

```rust
pub fn set_xtalk_margin(&self, margin_kcps: f64) -> Result<()>
```

#### Vl53l8cxDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&self, reflectance_percent: i32, nb_samples: i32, distance_mm: i32) -> Result<()>
```

On-device crosstalk calibration against a flat target; read the result with
[`Vl53l8cxDevice::get_caldata_xtalk`].

#### Vl53l8cxDevice::get_caldata_xtalk

```rust
pub fn get_caldata_xtalk(&self) -> Result<Vec<u8>>
```

The 776-byte xtalk calibration buffer (save / restore).

#### Vl53l8cxDevice::set_caldata_xtalk

```rust
pub fn set_caldata_xtalk(&self, blob: &[u8]) -> Result<()>
```

#### Vl53l8cxDevice::get_detection_thresholds_enable

```rust
pub fn get_detection_thresholds_enable(&self) -> Result<u8>
```

#### Vl53l8cxDevice::set_detection_thresholds_enable

```rust
pub fn set_detection_thresholds_enable(&self, enabled: bool) -> Result<()>
```

#### Vl53l8cxDevice::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<Vec<DetectionThreshold>>
```

#### Vl53l8cxDevice::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, thresholds: &[DetectionThreshold]) -> Result<()>
```

The 64 detection thresholds (interrupt on threshold).

#### Vl53l8cxDevice::set_detection_thresholds_auto_stop

```rust
pub fn set_detection_thresholds_auto_stop(&self, auto_stop: bool) -> Result<()>
```

#### Vl53l8cxDevice::configure_motion_indicator

```rust
pub fn configure_motion_indicator(&self, distance_mm: Option<(i32, i32)>) -> Result<MotionConfig>
```

Enable the motion indicator over [min, max] mm (`None`: 400–1500); every frame then carries
[`Vl53l8Frame::motion`]. Returns the configuration for further tuning.

#### Vl53l8cxDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Program the output list, start the sensor and the board's stream.

#### Vl53l8cxDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53l8cxDevice::on_frame

```rust
pub fn on_frame(&self, callback: impl Fn(&Vl53l8Frame) + Send + Sync + 'static) -> Hook
```

Call `callback` for every parsed frame (reader thread).

#### Vl53l8cxDevice::frames

```rust
pub fn frames(&self, capacity: usize) -> Subscription<Vl53l8Frame>
```

A pull queue of parsed frames (bounded, drop-oldest).

#### Vl53l8cxDevice::get_frame

```rust
pub fn get_frame(&self, timeout: Option<Duration>) -> Result<Vl53l8Frame>
```

The next frame (contract 07 §3); `timeout` `None`: 2 s.

#### Vl53l8cxDevice::frame_parse_errors

```rust
pub fn frame_parse_errors(&self) -> u64
```

Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps.

#### Vl53l8cxDevice::reassembler_discards

```rust
pub fn reassembler_discards(&self) -> u64
```

Chunked frames the reassembler discarded (gaps, offset errors).

#### Vl53l8cxDevice::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

### Vl53l8chDevice

```rust
pub struct Vl53l8chDevice(Vl53l8cxDevice);
```

VL53L8CH: the VL53L8CX superset (every CX method, through `Deref`) plus CNH, compact network
histograms — each frame can carry a per-aggregate distance histogram. `init` downloads the CH
firmware (VL53LMZ ULD 2.0.16).

#### Vl53l8chDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l8chDevice
```

Continue the open connection `dev` as a VL53L8CH.

#### Vl53l8chDevice::cnh_setup

```rust
pub fn cnh_setup(&self) -> Option<Vl53l8CnhSetup>
```

The CNH setup armed by [`Vl53l8chDevice::configure_cnh`], or `None`; recorders keep it next
to the raw blocks to decode them later.

#### Vl53l8chDevice::configure_cnh

```rust
pub fn configure_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()>
```

Arm CNH for the next [`Vl53l8cxDevice::start_ranging`]: every frame then carries
[`Vl53l8Frame::cnh_raw`], decoded with [`crate::vl53l8::decode_cnh`] and
[`Vl53l8CnhSetup::decode_config`].

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

### AdvancedError

```rust
pub enum AdvancedError {
    /// Motion indicator resolution other than 16 (4×4) or 64 (8×8).
    InvalidResolution(usize),
    /// Xtalk margin outside 0..=10000 kcps (or not a number).
    XtalkMarginOutOfRange,
    /// Threshold `index`: low/high × its measurement scale does not fit i32.
    ThresholdOverflow(usize),
    /// More than [`NB_THRESHOLDS`] thresholds.
    TooManyThresholds(usize),
}
```

Advanced-codec argument error (contract 07 §3: an argument the driver
would refuse, or one that does not fit its DCI field, is rejected before
anything is written — the ULD's `VL53L8CX_STATUS_INVALID_PARAM`).

### XTALK_MARGIN_MAX_KCPS

```rust
pub const XTALK_MARGIN_MAX_KCPS: f64 = 10000.0;
```

Largest xtalk margin `set_xtalk_margin` accepts, kcps/SPAD.

### try_xtalk_margin_to_raw

```rust
pub fn try_xtalk_margin_to_raw(margin_kcps: f64) -> Result<u32, AdvancedError>
```

Xtalk margin (kcps/SPAD) → raw DCI value `round(kcps * 2048)`, as
`set_xtalk_margin` writes it: 0..=[`XTALK_MARGIN_MAX_KCPS`], else
[`AdvancedError::XtalkMarginOutOfRange`].

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

#### MotionConfig::try_set_resolution

```rust
pub fn try_set_resolution(&mut self, resolution: usize) -> Result<(), AdvancedError>
```

Set `map_id` for the resolution (pure; mirrors `_set_resolution`):
16 or 64 zones, else [`AdvancedError::InvalidResolution`] with
`map_id` untouched.

### try_default_motion_config

```rust
pub fn try_default_motion_config(resolution: usize) -> Result<MotionConfig, AdvancedError>
```

The default motion-indicator configuration `motion_indicator_init` programs
for a resolution (the exact bytes written to the sensor); a resolution
other than 16 or 64 is [`AdvancedError::InvalidResolution`].

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

### try_pack_detection_thresholds

```rust
pub fn try_pack_detection_thresholds(
    thresholds: &[DetectionThreshold],
) -> Result<DetectionThresholdBlocks, AdvancedError>
```

Pack up to 64 detection thresholds into their DCI blocks. Entries beyond
the supplied list are zero-filled. Each entry's low/high are multiplied by
its measurement's fixed-point scale; a product that does not fit the i32
field is [`AdvancedError::ThresholdOverflow`] (never wrapped), more than
64 entries [`AdvancedError::TooManyThresholds`].

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

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u16, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

[`pack_write_reg`] that refuses empty data (contract 04 §2: at least one
data byte).

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
    /// mm: `floor(raw/4)`, a negative distance clamped to 0, as ST `GetRangingData`.
    pub distance_mm: Vec<i32>,
    /// 5/9 = valid; 255 = no target detected in the zone.
    pub target_status: Vec<u8>,
    pub nb_target_detected: Vec<u8>,
    /// Signal rate, kcps/SPAD (`raw / 2048`).
    pub signal_per_spad: Vec<f64>,
    /// Ambient rate, kcps/SPAD (`raw / 2048`).
    pub ambient_per_spad: Vec<f64>,
    pub nb_spads_enabled: Vec<u32>,
    /// Measurement noise, mm (`raw / 128`).
    pub range_sigma_mm: Vec<f64>,
    /// Estimated target reflectance, % (`raw / 2`).
    pub reflectance: Vec<f64>,
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

### Vl53l8CnhSetup

```rust
pub struct Vl53l8CnhSetup {
    ref_bin_offset: i32,
    sum_span: i32,
    feature_length: i32,
    nb_of_aggregates: i32,
    cnh_cfg: u8,
    cnh_flex_shift: u8,
    map_id: [i8; 64],
}
```

The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH.

A mirror of `VL53LMZ_Motion_Configuration` with the plugin helpers that fill it
(`vl53lmz_plugin_cnh.c`, ULD 2.0.16), ported from the reference Python `CnhConfig`. Build it
with [`Vl53l8CnhSetup::init_config`] and [`Vl53l8CnhSetup::create_agg_map`], check
[`Vl53l8CnhSetup::required_memory`], then arm it on the device (the CNH block size goes to
[`super::Vl53l8Driver::start_ranging`], the struct to DCI [`Vl53l8CnhSetup::MI_CFG_DEV_IDX`]).
The flags are fixed to ping-pong and variance off (`Example_12_cnh_data.c`), which
[`crate::vl53l8::decode_cnh`] assumes. Arguments outside their fields are
[`DepzError::InvalidArgument`], never wrapped.

#### Vl53l8CnhSetup::new

```rust
pub fn new() -> Self
```

An empty configuration: no bins, no aggregates, every zone in none.

#### Vl53l8CnhSetup::ref_bin_offset

```rust
pub fn ref_bin_offset(&self) -> i32
```

The first device bin, ×2048 (`ref_bin_offset`).

#### Vl53l8CnhSetup::sum_span

```rust
pub fn sum_span(&self) -> i32
```

Device bins summed per CNH bin.

#### Vl53l8CnhSetup::feature_length

```rust
pub fn feature_length(&self) -> i32
```

CNH bins per aggregate.

#### Vl53l8CnhSetup::nb_of_aggregates

```rust
pub fn nb_of_aggregates(&self) -> i32
```

Aggregates in the map.

#### Vl53l8CnhSetup::cnh_cfg

```rust
pub fn cnh_cfg(&self) -> u8
```

The CNH flags byte.

#### Vl53l8CnhSetup::cnh_flex_shift

```rust
pub fn cnh_flex_shift(&self) -> u8
```

The CNH flex shift.

#### Vl53l8CnhSetup::map_id

```rust
pub fn map_id(&self) -> [i8; 64]
```

Device zone → CNH aggregate, −1 for a zone in none (64 entries).

#### Vl53l8CnhSetup::decode_config

```rust
pub fn decode_config(&self) -> CnhDecodeConfig
```

The counts [`crate::vl53l8::decode_cnh`] needs to unpack this configuration's blocks.

#### Vl53l8CnhSetup::init_config

```rust
pub fn init_config(&mut self, start_bin: i32, num_bins: i32, sub_sample: i32) -> Result<()>
```

`vl53lmz_cnh_init_config`. `start_bin`: first device histogram bin; `num_bins`: CNH bins;
`sub_sample`: device bins summed per CNH bin.

#### Vl53l8CnhSetup::create_agg_map

```rust
pub fn create_agg_map(
    &mut self,
    resolution: usize,
    start_x: i32,
    start_y: i32,
    merge_x: i32,
    merge_y: i32,
    cols: i32,
    rows: i32,
) -> Result<()>
```

`vl53lmz_cnh_create_agg_map`: aggregates of `merge_x`×`merge_y` zones, `cols`×`rows` of
them from (`start_x`, `start_y`). `resolution` (16 or 64) must be the sensor's.

#### Vl53l8CnhSetup::required_memory

```rust
pub fn required_memory(&self) -> Result<u32>
```

`vl53lmz_cnh_calc_required_memory`: the on-device CNH buffer, bytes. No aggregate map yet,
or more than [`Vl53l8CnhSetup::CNH_MAX_DATA_BYTES`], is [`DepzError::InvalidArgument`].

#### Vl53l8CnhSetup::min_max_distance_mm

```rust
pub fn min_max_distance_mm(&self) -> (i32, i32)
```

(min, max) target distance, mm, the histogram captures whole.

#### Vl53l8CnhSetup::bin_center_mm

```rust
pub fn bin_center_mm(&self, bin_idx: i32) -> f64
```

Distance, mm, at the centre of CNH bin `bin_idx`.

#### Vl53l8CnhSetup::pack

```rust
pub fn pack(&self) -> Vec<u8>
```

The 156-byte `VL53LMZ_Motion_Configuration` struct for the device (DCI 0xBFAC).

### Vl53l8UldError

```rust
pub enum Vl53l8UldError {
    /// A ULD status ([`status`] codes, or the GO2 error byte the sensor reports) and where it
    /// happened.
    Status {
        /// The status code.
        code: u8,
        /// The step that failed (`"sensor boot"`, `"dci read 0x5450"` …); may be empty.
        context: String,
    },
    /// An argument the SDK refuses before sending anything that the ULD itself would not catch (a
    /// field it would wrap): a Java `IllegalArgumentException`.
    InvalidArgument(String),
    /// The platform (the register bridge) failed.
    Platform(DepzError),
}
```

What a [`super::Vl53l8Driver`] call can fail with.

`?` turns it into [`DepzError`]: a ULD status becomes [`DepzError::Sensor`] with the status
name and code in the text (`"VL53L8CX TIMEOUT (code 1) sensor boot"`, as the other SDKs word
it), a refused argument [`DepzError::InvalidArgument`], and a platform failure the
[`DepzError`] the platform returned. Match on this type before converting when the status
code matters ([`Vl53l8UldError::status_code`]).

#### Vl53l8UldError::status

```rust
pub fn status(code: u8, context: impl Into<String>) -> Self
```

A ULD status error.

#### Vl53l8UldError::status_code

```rust
pub fn status_code(&self) -> Option<u8>
```

The ULD status code, for a [`Vl53l8UldError::Status`].

### UldResult

```rust
pub type UldResult<T> = std::result::Result<T, Vl53l8UldError>;
```

`Result` with [`Vl53l8UldError`].

### Vl53l8Firmware

```rust
pub enum Vl53l8Firmware {
    /// VL53L8CX, ST ULD 2.1.0.
    #[default]
    Cx,
    /// VL53L8CH, VL53LMZ ULD 2.0.16 (CNH).
    Ch,
    /// VL53L5CX / VL53L7CX over I2C, ULD 2.0.1.
    L7cx,
    /// VL53L7CH over I2C, VL53LMZ ULD 2.0.16 with the VL53L7 configuration.
    L7ch,
}
```

The sensor firmware a [`super::Vl53l8Driver`] downloads.

#### Vl53l8Firmware::set_name

```rust
pub fn set_name(self) -> &'static str
```

The blob set name under `vl53l8/data` (`cx`, `ch`, `l7cx`, `l7ch`).

### Vl53l8Motion

```rust
pub struct Vl53l8Motion {
    /// Global indicator 1.
    pub global_indicator_1: u32,
    /// Global indicator 2.
    pub global_indicator_2: u32,
    /// Motion-indicator status.
    pub status: u8,
    /// Aggregates with motion detected.
    pub nb_of_detected_aggregates: u8,
    /// Aggregates configured.
    pub nb_of_aggregates: u8,
    /// Per-aggregate motion value (32 entries).
    pub motion: Vec<u32>,
}
```

The motion-indicator block of a frame (`VL53L8CX_ResultsData MotionIndicator`).

### Vl53l8Frame

```rust
pub struct Vl53l8Frame {
    /// Host timestamp the caller gave the frame, µs.
    pub timestamp_us: u64,
    /// Zones in this frame (16 or 64).
    pub resolution: usize,
    /// Silicon temperature, °C.
    pub silicon_temp_degc: i8,
    /// Distance per zone and target, mm (≥ 0).
    pub distance_mm: Vec<i32>,
    /// Target status per zone and target: 5 / 9 valid, 255 no target detected.
    pub target_status: Vec<u8>,
    /// Targets detected per zone.
    pub nb_target_detected: Vec<u8>,
    /// Signal rate per zone and target, kcps/SPAD.
    pub signal_per_spad: Vec<f64>,
    /// Ambient rate per zone, kcps/SPAD.
    pub ambient_per_spad: Vec<f64>,
    /// SPADs enabled per zone (raw, ÷256).
    pub nb_spads_enabled: Vec<u32>,
    /// Range sigma per zone and target, mm.
    pub range_sigma_mm: Vec<f64>,
    /// Reflectance estimate per zone and target, %.
    pub reflectance: Vec<f64>,
    /// The raw CNH block (word-swapped like every block), or `None`; decode with
    /// [`crate::vl53l8::decode_cnh`].
    pub cnh_raw: Option<Vec<u8>>,
    /// The motion-indicator output, or `None` when the indicator is not configured.
    pub motion: Option<Vl53l8Motion>,
}
```

One decoded ranging frame.

Per-zone vectors are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs
row-major. Values are in ST GetRangingData's real units (contract 04 §5): a negative distance
is clamped to 0, the rates and the reflectance keep their fraction. `cnh_raw` is the raw CNH data block (VL53L8CH / VL53L7CH with CNH armed),
else `None`; `motion` the motion-indicator output when it is configured, else `None`.

#### Vl53l8Frame::with_resolution

```rust
pub fn with_resolution(mut self, zones: usize) -> Self
```

This frame with `resolution` replaced (the device's own count of zones in use).

### Vl53l8Platform

```rust
pub trait Vl53l8Platform: Send + Sync {
    /// Read `size` bytes from `addr` on. Fewer bytes than asked is a failure the driver reports.
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    /// Write `data` from `addr` on.
    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    /// Wait `ms` milliseconds (the default sleeps the thread).
    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(Duration::from_millis(ms as u64));
    }
}
```

What the ULD needs from the board: register access through the bridge (16-bit addresses,
auto-increment) and a delay. The device class maps it onto the firmware bridge; a test can
script it. `Send + Sync`, so a driver over it can live in a device's `Mutex` and be called from
any thread.

### get_xtalk_cmd

```rust
pub fn get_xtalk_cmd() -> &'static [u8]
```

vl53l8cx_plugin_xtalk.h: the command that reads the xtalk buffer back; 72 bytes.

### calibrate_xtalk_cx

```rust
pub fn calibrate_xtalk_cx() -> &'static [u8]
```

VL53L8CX_CALIBRATE_XTALK (ULD 2.1.0); 1000 bytes.

### calibrate_xtalk_lmz

```rust
pub fn calibrate_xtalk_lmz() -> &'static [u8]
```

VL53LMZ_CALIBRATE_XTALK (ULD 2.0.16: L8CH, L7CH); 984 bytes.

### calibrate_xtalk_l7cx

```rust
pub fn calibrate_xtalk_l7cx() -> &'static [u8]
```

VL53L7CX_CALIBRATE_XTALK (ULD 2.0.1: L5CX, L7CX); 984 bytes.

### xtalk_cal_output

```rust
pub fn xtalk_cal_output() -> &'static [u32]
```

The 17-block output list of the xtalk calibration run (u32 block headers); 17 words.

### xtalk_cal_output_enable

```rust
pub fn xtalk_cal_output_enable() -> &'static [u32]
```

The output enables of [`xtalk_cal_output`] (one bit per block); 4 words.

### RANGING_MODE_CONTINUOUS

```rust
pub const RANGING_MODE_CONTINUOUS: u8 = 1;
```

`set_ranging_mode`: continuous ranging (the integration time has no effect).

### RANGING_MODE_AUTONOMOUS

```rust
pub const RANGING_MODE_AUTONOMOUS: u8 = 3;
```

`set_ranging_mode`: autonomous ranging (frames at the ranging frequency, set integration time).

### TARGET_ORDER_CLOSEST

```rust
pub const TARGET_ORDER_CLOSEST: u8 = 1;
```

`set_target_order`: closest target first.

### TARGET_ORDER_STRONGEST

```rust
pub const TARGET_ORDER_STRONGEST: u8 = 2;
```

`set_target_order`: strongest target first.

### MODULE_TYPE_MZ

```rust
pub const MODULE_TYPE_MZ: u8 = 0;
```

[`Vl53l8Driver::module_type`]: VL53L5CX.

### MODULE_TYPE_MZEVO

```rust
pub const MODULE_TYPE_MZEVO: u8 = 1;
```

[`Vl53l8Driver::module_type`]: VL53L7CX / VL53L7CH.

### MODULE_TYPE_MZPLUS

```rust
pub const MODULE_TYPE_MZPLUS: u8 = 2;
```

[`Vl53l8Driver::module_type`]: VL53L8.

### NVM_DATA_SIZE

```rust
pub const NVM_DATA_SIZE: usize = 492;
```

Bytes of NVM data the ULD reads at init.

### OFFSET_BUFFER_SIZE

```rust
pub const OFFSET_BUFFER_SIZE: usize = 488;
```

Bytes of the offset buffer the ULD uploads.

### XTALK_BUFFER_SIZE

```rust
pub const XTALK_BUFFER_SIZE: usize = 776;
```

Bytes of the xtalk calibration buffer (save / restore with the caldata calls).

### Vl53l8Driver

```rust
pub struct Vl53l8Driver<P: Vl53l8Platform> {
    p: P,
    firmware: Vl53l8Firmware,
    i2c: bool,
    fw_checksum: Option<u32>,
    output_enable_w3: u32,
    frame_tail: u32,
    variant: Variant,
    offset_data: Vec<u8>,
    xtalk_data: Vec<u8>,
    stream_count: i32,
    motion_present: bool,
    module_type: Option<u8>,
    data_read_size: usize,
    frame_size_mismatch: Option<(usize, usize)>,
    resolution: Option<usize>,
    xtalk_calibration_failed: bool,
}
```

The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on the host.

Ported from the reference Python driver: init (sensor firmware download), resolution,
frequency, mode, integration time, sharpener, target order, ranging, frame parsing, power
modes, xtalk margin and calibration, detection thresholds, motion indicator. Register sequences
are a 1:1 port — every request matches the Python one, so captures replay in both. `Send`
(the platform is `Send + Sync`): a device class holds it in a `Mutex`.

#### Vl53l8Driver::new

```rust
pub fn new(platform: P, firmware: Vl53l8Firmware) -> Self
```

A driver over `platform` for `firmware` ([`Vl53l8Firmware::Cx`] is the default). Nothing
is sent until the first call.

#### Vl53l8Driver::platform

```rust
pub fn platform(&self) -> &P
```

The platform the driver runs on.

#### Vl53l8Driver::firmware

```rust
pub fn firmware(&self) -> Vl53l8Firmware
```

The sensor firmware this driver downloads.

#### Vl53l8Driver::is_i2c

```rust
pub fn is_i2c(&self) -> bool
```

The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks keep 64
entries in 4×4).

#### Vl53l8Driver::module_type

```rust
pub fn module_type(&self) -> Option<u8>
```

L5/L7: [`MODULE_TYPE_MZ`] (VL53L5CX) or [`MODULE_TYPE_MZEVO`] (VL53L7CX/CH), read during
[`Vl53l8Driver::init`]; else `None`.

#### Vl53l8Driver::data_read_size

```rust
pub fn data_read_size(&self) -> usize
```

The frame size the firmware streams (bytes), set by [`Vl53l8Driver::start_ranging`].

#### Vl53l8Driver::frame_size_mismatch

```rust
pub fn frame_size_mismatch(&self) -> Option<(usize, usize)>
```

(firmware, host) frame sizes when the firmware reported another size than the ULD formula;
else `None`.

#### Vl53l8Driver::resolution

```rust
pub fn resolution(&self) -> Option<usize>
```

Zones the last [`Vl53l8Driver::start_ranging`] used, or `None`.

#### Vl53l8Driver::xtalk_calibration_failed

```rust
pub fn xtalk_calibration_failed(&self) -> bool
```

The last [`Vl53l8Driver::calibrate_xtalk`] ran into "coverglass too good": the default
xtalk buffer is kept.

#### Vl53l8Driver::xtalk_data

```rust
pub fn xtalk_data(&self) -> &[u8]
```

The 776-byte xtalk buffer the driver uploads (default, calibrated or restored); empty
before [`Vl53l8Driver::init`].

#### Vl53l8Driver::silicon_id_matches

```rust
pub fn silicon_id_matches(device_id: u8, revision_id: u8, i2c: bool) -> bool
```

Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision 0x02, or
0x01 with device id 0xF0, on L5/L7 over I2C.

#### Vl53l8Driver::dci_read_data

```rust
pub fn dci_read_data(&self, index: u16, data_size: usize) -> UldResult<Vec<u8>>
```

`vl53l8cx_dci_read_data`: `data_size` bytes of DCI `index` (byte order fixed: the swap is
undone).

#### Vl53l8Driver::dci_write_data

```rust
pub fn dci_write_data(&self, index: u16, data: &[u8]) -> UldResult<()>
```

`vl53l8cx_dci_write_data`: write `data` to DCI `index`.

#### Vl53l8Driver::dci_replace_data

```rust
pub fn dci_replace_data(&self, index: u16, data_size: usize, new_data: &[u8], new_data_pos: usize) -> UldResult<()>
```

`vl53l8cx_dci_replace_data`: read `data_size` bytes of DCI `index`, put `new_data` at
`new_data_pos`, write it back.

#### Vl53l8Driver::is_alive

```rust
pub fn is_alive(&self) -> UldResult<(u8, u8)>
```

(device id, revision id) of the silicon; [`Vl53l8Driver::silicon_id_matches`] tells whether
they are the sensor's.

#### Vl53l8Driver::init

```rust
pub fn init(&mut self) -> UldResult<()>
```

`vl53l8cx_init`: boot the sensor MCU, download the 84 KB sensor firmware, upload the NVM
offsets, the default xtalk and the default configuration.

#### Vl53l8Driver::init_with_progress

```rust
pub fn init_with_progress(&mut self, progress: &mut dyn FnMut(&str)) -> UldResult<()>
```

[`Vl53l8Driver::init`] that tells `progress` the phase texts (`"SW reboot..."`,
`"Downloading sensor FW (84 KB)... bank 1/3"` … `"Sensor init done"`).

#### Vl53l8Driver::get_resolution

```rust
pub fn get_resolution(&self) -> UldResult<usize>
```

Zones in use (16 or 64; 0 before a resolution is set).

#### Vl53l8Driver::set_resolution

```rust
pub fn set_resolution(&mut self, res: usize) -> UldResult<()>
```

16 (4×4) or 64 (8×8) zones; uploads the offsets and xtalk for it.

#### Vl53l8Driver::get_ranging_frequency_hz

```rust
pub fn get_ranging_frequency_hz(&self) -> UldResult<u8>
```

Ranging frequency, Hz.

#### Vl53l8Driver::set_ranging_frequency_hz

```rust
pub fn set_ranging_frequency_hz(&mut self, hz: i32) -> UldResult<()>
```

Ranging frequency, Hz: a u8 field, so 0..255 (outside is
[`Vl53l8UldError::InvalidArgument`]); the sensor's own limits apply on top.

#### Vl53l8Driver::set_ranging_mode

```rust
pub fn set_ranging_mode(&mut self, mode: u8) -> UldResult<()>
```

[`RANGING_MODE_CONTINUOUS`] or [`RANGING_MODE_AUTONOMOUS`].

#### Vl53l8Driver::get_ranging_mode

```rust
pub fn get_ranging_mode(&self) -> UldResult<u8>
```

[`RANGING_MODE_CONTINUOUS`] or [`RANGING_MODE_AUTONOMOUS`].

#### Vl53l8Driver::get_integration_time_ms

```rust
pub fn get_integration_time_ms(&self) -> UldResult<u32>
```

Integration time, ms.

#### Vl53l8Driver::set_integration_time_ms

```rust
pub fn set_integration_time_ms(&mut self, time_ms: i32) -> UldResult<()>
```

2..1000 ms; no effect in continuous mode.

#### Vl53l8Driver::get_sharpener_percent

```rust
pub fn get_sharpener_percent(&self) -> UldResult<u8>
```

0..99 %, rounded to nearest on the way back (ST truncates and loses a count for 95 of the
100 legal values; the stored byte is the same).

#### Vl53l8Driver::set_sharpener_percent

```rust
pub fn set_sharpener_percent(&mut self, pct: i32) -> UldResult<()>
```

0..99 %: 100 and up is the ULD's INVALID_PARAM, a negative value
[`Vl53l8UldError::InvalidArgument`].

#### Vl53l8Driver::get_target_order

```rust
pub fn get_target_order(&self) -> UldResult<u8>
```

[`TARGET_ORDER_CLOSEST`] or [`TARGET_ORDER_STRONGEST`].

#### Vl53l8Driver::set_target_order

```rust
pub fn set_target_order(&mut self, order: u8) -> UldResult<()>
```

[`TARGET_ORDER_CLOSEST`] or [`TARGET_ORDER_STRONGEST`].

#### Vl53l8Driver::start_ranging

```rust
pub fn start_ranging(&mut self, cnh_data_size: Option<u32>) -> UldResult<()>
```

Program the output list and start. `cnh_data_size` (bytes, from
[`super::Vl53l8CnhSetup::required_memory`]; `None` for none) appends the CNH block to every
frame. The frame size the firmware reports wins over the ULD formula
([`Vl53l8Driver::frame_size_mismatch`]).

#### Vl53l8Driver::stop_ranging

```rust
pub fn stop_ranging(&mut self) -> UldResult<()>
```

`vl53l8cx_stop_ranging`: provoke an MCU stop (unless the detection thresholds auto-stopped
it), wait up to ~5 s, then undo the stop and the xshut bypass. A stuck MCU is not an error.

#### Vl53l8Driver::check_data_ready

```rust
pub fn check_data_ready(&mut self) -> UldResult<bool>
```

Poll mode: a new frame is ready. A GO2 error is a [`Vl53l8UldError::Status`] carrying the
sensor's error byte as the code.

#### Vl53l8Driver::get_ranging_data

```rust
pub fn get_ranging_data(&mut self, timestamp_us: u64) -> UldResult<Vl53l8Frame>
```

Poll mode: read one frame from register 0x00 and parse it.

#### Vl53l8Driver::parse_frame

```rust
pub fn parse_frame(&mut self, timestamp_us: u64, raw: &[u8]) -> UldResult<Vl53l8Frame>
```

Parse one raw frame ([`Vl53l8Driver::data_read_size`] bytes) — polled, or pushed by the
board's stream. L5/L7 frames are trimmed to the zones in use.

A header / footer id mismatch (or a frame too short to hold them) is
[`status::CORRUPTED_FRAME`].

#### Vl53l8Driver::get_power_mode

```rust
pub fn get_power_mode(&self) -> UldResult<u8>
```

[`POWER_MODE_WAKEUP`], [`POWER_MODE_SLEEP`] or [`POWER_MODE_DEEP_SLEEP`]
(`crate::vl53l8::advanced` constants); an unknown state is [`status::ERROR`].

#### Vl53l8Driver::set_power_mode

```rust
pub fn set_power_mode(&mut self, power_mode: u8) -> UldResult<()>
```

Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs
[`Vl53l8Driver::init`]: the sensor firmware is lost.

#### Vl53l8Driver::get_xtalk_margin

```rust
pub fn get_xtalk_margin(&self) -> UldResult<f64>
```

Xtalk margin, kcps/SPAD.

#### Vl53l8Driver::set_xtalk_margin

```rust
pub fn set_xtalk_margin(&mut self, margin_kcps: f64) -> UldResult<()>
```

Xtalk margin, kcps/SPAD: 0..10000 (NaN and anything else is [`status::INVALID_PARAM`],
nothing written).

#### Vl53l8Driver::get_caldata_xtalk

```rust
pub fn get_caldata_xtalk(&mut self) -> UldResult<Vec<u8>>
```

The live 776-byte xtalk calibration buffer (save / restore); the resolution is restored
afterwards.

#### Vl53l8Driver::set_caldata_xtalk

```rust
pub fn set_caldata_xtalk(&mut self, blob: &[u8]) -> UldResult<()>
```

Restore a saved 776-byte xtalk buffer; it is uploaded again by the next resolution change
and at once here. Another length is [`status::INVALID_PARAM`].

#### Vl53l8Driver::get_detection_thresholds_enable

```rust
pub fn get_detection_thresholds_enable(&self) -> UldResult<u8>
```

Whether the detection thresholds are enabled (the raw enable byte: 1 on, 0 off).

#### Vl53l8Driver::set_detection_thresholds_enable

```rust
pub fn set_detection_thresholds_enable(&mut self, enabled: bool) -> UldResult<()>
```

Enable or disable the detection thresholds.

#### Vl53l8Driver::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> UldResult<Vec<DetectionThreshold>>
```

The 64 thresholds, low / high rescaled to the units of their measurement (floor division).

#### Vl53l8Driver::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&mut self, thresholds: &[DetectionThreshold]) -> UldResult<()>
```

Program the 64 thresholds (missing entries are zeros). More than 64, or a low / high whose
scaled value does not fit i32, is [`Vl53l8UldError::InvalidArgument`], nothing written.

#### Vl53l8Driver::set_detection_thresholds_auto_stop

```rust
pub fn set_detection_thresholds_auto_stop(&mut self, auto_stop: bool) -> UldResult<()>
```

Stop ranging when a threshold fires (not in the L5CX/L7CX ULD: [`status::INVALID_PARAM`]).

#### Vl53l8Driver::motion_indicator_init

```rust
pub fn motion_indicator_init(&mut self, res: usize) -> UldResult<MotionConfig>
```

A default motion-indicator configuration for `res`, written to the sensor; frames carry
motion output from here on.

#### Vl53l8Driver::motion_indicator_set_resolution

```rust
pub fn motion_indicator_set_resolution(&mut self, cfg: &mut MotionConfig, res: usize) -> UldResult<()>
```

Set `cfg`'s zone map for `res` (16 or 64, else [`status::INVALID_PARAM`]) and write it.

#### Vl53l8Driver::motion_indicator_set_distance_motion

```rust
pub fn motion_indicator_set_distance_motion(
    &mut self,
    cfg: &mut MotionConfig,
    distance_min_mm: i32,
    distance_max_mm: i32,
) -> UldResult<()>
```

Set `cfg`'s distance window (400..4000 mm, at most 1500 mm wide, else
[`status::INVALID_PARAM`]) and write it.

#### Vl53l8Driver::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&mut self, reflectance_percent: i32, nb_samples: i32, distance_mm: i32) -> UldResult<()>
```

`vl53l8cx_calibrate_xtalk`: on-device crosstalk calibration against a flat target at
`distance_mm` (600..3000) of `reflectance_percent` (1..99), averaging `nb_samples` (1..16);
outside is [`status::INVALID_PARAM`], nothing sent. Takes seconds; the configuration is
saved and restored around it. Without cover glass the firmware answers "coverglass too
good": [`Vl53l8Driver::xtalk_calibration_failed`], and the default buffer stays.

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

### Vl53l7cxDevice

```rust
pub struct Vl53l7cxDevice(Vl53l8cxDevice);
```

VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge board (contract 11): a
[`Vl53l8cxDevice`] (every method, through `Deref`) with the L5/L7 firmware set (ULD 2.0.1),
smaller register reads, ranging from 1 Hz, and the board commands
[`Vl53l7cxDevice::get_bridge_info`], [`Vl53l7cxDevice::set_i2c_speed_khz`],
[`Vl53l7cxDevice::pin_ctrl`]. L5CX / L7CX have no deep sleep and no threshold auto-stop.

#### Vl53l7cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l7cxDevice
```

Continue the open connection `dev` as a VL53L7CX.

#### Vl53l7cxDevice::module_type

```rust
pub fn module_type(&self) -> Option<u8>
```

The sensor's module type read at `init`: [`MODULE_TYPE_MZ`] (VL53L5CX) or
[`MODULE_TYPE_MZEVO`] (VL53L7CX / CH); `None` before.

#### Vl53l7cxDevice::xtalk_calibration_failed

```rust
pub fn xtalk_calibration_failed(&self) -> bool
```

The last `calibrate_xtalk` found nothing to calibrate ("coverglass too good").

#### Vl53l7cxDevice::get_bridge_info

```rust
pub fn get_bridge_info(&self) -> Result<Vl53l7Info>
```

Bridge counters and pin levels (the sensor is not touched). Read it before and after a run:
during one each call takes the bus from the stream and can cost a frame.

#### Vl53l7cxDevice::set_i2c_speed_khz

```rust
pub fn set_i2c_speed_khz(&self, khz: u32) -> Result<u16>
```

Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns the
value in effect. A busy status mid-transfer ([`DepzError::is_busy`]): stop ranging first.

#### Vl53l7cxDevice::pin_ctrl

```rust
pub fn pin_ctrl(&self, action: u8) -> Result<()>
```

Drive the sensor's LPn / I2C_RST pins ([`crate::vl53l7::PIN_LPN_OFF`] …). LPN_OFF and
SOFT_CYCLE drop the sensor's state: `init` again afterwards.

### Vl53l5cxDevice

```rust
pub struct Vl53l5cxDevice(Vl53l7cxDevice);
```

VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of
[`Vl53l7cxDevice`]; the sensor reports module type MZ.

#### Vl53l5cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l5cxDevice
```

Continue the open connection `dev` as a VL53L5CX.

### Vl53l7chDevice

```rust
pub struct Vl53l7chDevice(Vl53l7cxDevice);
```

VL53L7CH: the VL53L7CX superset with CNH histograms. `init` downloads the CH firmware (VL53LMZ
ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream in chunks.

#### Vl53l7chDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l7chDevice
```

Continue the open connection `dev` as a VL53L7CH.

#### Vl53l7chDevice::cnh_setup

```rust
pub fn cnh_setup(&self) -> Option<Vl53l8CnhSetup>
```

The CNH setup armed by [`Vl53l7chDevice::configure_cnh`], or `None`.

#### Vl53l7chDevice::configure_cnh

```rust
pub fn configure_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()>
```

Arm CNH for the next start (as [`super::Vl53l8chDevice::configure_cnh`]).

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

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u16, len: u16) -> Result<[u8; 4], CodecError>
```

`VL53_READ_REG` payload: `addr u16, len u16` (little-endian), with the
L5/L7 limits (contract 11 §2): `len` 1..=[`READ_MAX_LEN`] and
`addr + len <= 0x10000`.

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`VL53_WRITE_REG` payload: `addr u16` + the raw register bytes, with the
L5/L7 limits (contract 11 §2): 1..=[`WRITE_MAX_LEN`] bytes and
`addr + N <= 0x10000`.

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

### PLOTTABLE_STATUSES

```rust
pub const PLOTTABLE_STATUSES: [i32; 3] = [0, 6, 11];
```

Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a
stream and 11 for two merged targets. Prefer [`Vl53lxMeasurement::plottable`] over
[`Vl53lxMeasurement::valid`] there.

### Vl53lxMeasurement

```rust
pub struct Vl53lxMeasurement {
    pub timestamp_us: u64,
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub spads: f64,
    pub targets: Vec<Vl53lxTarget>,
    pub extra: BTreeMap<String, ExtraValue>,
    pub bins: Option<Arc<HistogramBinData>>,
}
```

One ranging result of the 1D family, the same shape for every product. On the histogram driver
`targets` holds every return, strongest first, the top level repeats the first and `bins` carries
the raw 24-bin histogram; the light drivers leave both empty.

#### Vl53lxMeasurement::valid

```rust
pub fn valid(&self) -> bool
```

#### Vl53lxMeasurement::plottable

```rust
pub fn plottable(&self) -> bool
```

#### Vl53lxMeasurement::plot_distances

```rust
pub fn plot_distances(&self) -> Vec<i32>
```

The distances a chart draws: the plottable targets in driver order, or the single distance.

#### Vl53lxMeasurement::primary_target

```rust
pub fn primary_target(&self) -> Option<&Vl53lxTarget>
```

The one target a single-number readout shows, or `None`.

### Vl53lxProductInfo

```rust
pub struct Vl53lxProductInfo {
    /// The board's device name.
    pub board: String,
    /// The product the device name carries, or `None` on an unstamped board.
    pub detected: Option<&'static str>,
    /// The product [`Vl53lxDevice::init`] bound.
    pub product: &'static str,
    pub driver: &'static str,
    /// The driver's own name ("Vl53l1UldDriver", …).
    pub driver_class: &'static str,
    /// The driver kinds the bound product has.
    pub kinds: Vec<&'static str>,
    pub model_id: u16,
    /// A cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.
    pub model_id_ok: bool,
    pub supports: Vec<&'static str>,
    pub modes: Vec<&'static str>,
    /// The board's rated reach, mm.
    pub reach_mm: Option<u32>,
    /// How far the loaded configuration can measure, mm.
    pub driver_reach_mm: Option<i32>,
    pub budget_ms: (u32, u32),
    pub budget_choices: Vec<u32>,
    pub histogram: bool,
    pub max_khz: u16,
    pub caveat: Option<&'static str>,
}
```

Everything about what is connected ([`Vl53lxDevice::identify_product`]).

### Vl53lxDevice

```rust
pub struct Vl53lxDevice {
    dev: DepzDevice,
    fixed_product: Option<&'static str>,
    streams: Arc<Streams>,
    platform: Arc<BridgePlatform>,
    bound: Arc<Mutex<Option<Bound>>>,
    board_name: Mutex<Option<String>>,
    ranging: AtomicBool,
}
```

A board of the VL53L 1D family on the `APP_VL53L0_4` register bridge (contract 12): VL53L0X,
VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board
is driven — the product (whose parameter set: the board's own, or a sibling's to borrow its
driver) and the driver kind (`uld`, `ulp`, `histogram`). [`Vl53lxDevice::init`] binds the pair,
[`Vl53lxDevice::configure`] re-initialises and applies a ranging configuration (before every
run), then [`Vl53lxDevice::start_ranging`] and read [`Vl53lxDevice::measurements`]. Ask
[`Vl53lxDevice::supports`] before an optional group; a missing one fails with
[`DepzError::Unsupported`]. This generic class reads the product from the board's name; the
per-product classes ([`Vl53l0xDevice`], …) fix it. Configuration while ranging fails with
[`DepzError::InvalidState`] — the stream owns the bus.

#### Vl53lxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53lxDevice
```

Continue the open connection `dev` as a 1D-family board; the product comes from its name.

#### Vl53lxDevice::board_name

```rust
pub fn board_name(&self) -> Result<String>
```

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523` (asked once).

#### Vl53lxDevice::detected

```rust
pub fn detected(&self) -> Result<Option<&'static str>>
```

The product the device name carries, or `None` on an unstamped board.

#### Vl53lxDevice::product

```rust
pub fn product(&self) -> Option<&'static str>
```

The product [`Vl53lxDevice::init`] bound (`None` before).

#### Vl53lxDevice::driver_kind

```rust
pub fn driver_kind(&self) -> Option<&'static str>
```

#### Vl53lxDevice::caveat

```rust
pub fn caveat(&self) -> Option<&'static str>
```

#### Vl53lxDevice::with_driver

```rust
pub fn with_driver<R>(&self, f: impl FnOnce(&mut dyn Vl53lxDriver) -> Result<R>) -> Result<R>
```

The ULD port in use — escape hatch for product-specific calls: `f` runs with the driver (do
not call this device's driver-backed methods from `f`: the driver is held).
[`DepzError::InvalidState`] before [`Vl53lxDevice::init`].

#### Vl53lxDevice::initialized

```rust
pub fn initialized(&self) -> bool
```

#### Vl53lxDevice::ranging

```rust
pub fn ranging(&self) -> bool
```

#### Vl53lxDevice::driver_kinds

```rust
pub fn driver_kinds(&self, product: Option<&str>) -> Result<Vec<&'static str>>
```

The driver kinds a product has (`None`: this board's; empty when it carries no product).

#### Vl53lxDevice::init

```rust
pub fn init(&self, driver: Option<&str>, product: Option<&str>) -> Result<()>
```

Bind the (product, driver kind) pair and initialise the sensor. `product` `None`: this
class's, else the board name's; `driver` `None`: the product's first kind. A pair the table
lacks fails with [`DepzError::Unsupported`] naming what the product has.

#### Vl53lxDevice::identify_product

```rust
pub fn identify_product(&self) -> Result<Vl53lxProductInfo>
```

Everything about what is connected (needs [`Vl53lxDevice::init`]).

#### Vl53lxDevice::notes

```rust
pub fn notes(&self) -> Result<Vec<String>>
```

The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a pair
that reaches less than the board is rated for, the driver's caveat.

#### Vl53lxDevice::supports

```rust
pub fn supports(&self, group: &str) -> Result<bool>
```

Whether this product / driver serves an optional group.

#### Vl53lxDevice::modes

```rust
pub fn modes(&self) -> Result<Vec<&'static str>>
```

Named ranging modes, the first being what init leaves; empty if none.

#### Vl53lxDevice::budget_range

```rust
pub fn budget_range(&self) -> Result<(u32, u32)>
```

The (min, max) budget the driver offers, ms.

#### Vl53lxDevice::driver_reach_mm

```rust
pub fn driver_reach_mm(&self) -> Result<Option<i32>>
```

How far the loaded configuration can measure, mm, or `None`.

#### Vl53lxDevice::xshut

```rust
pub fn xshut(&self, action: u8) -> Result<()>
```

Drive XSHUT: off (0) / on (1) / reset (2). Off and reset stop the stream; the sensor then holds
none of the configuration — [`Vl53lxDevice::init`] again.

#### Vl53lxDevice::bridge_info

```rust
pub fn bridge_info(&self) -> Result<Vl53lxInfo>
```

RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.

#### Vl53lxDevice::configure

```rust
pub fn configure(
    &self,
    budget_ms: u32,
    inter_ms: u32,
    mode: Option<&str>,
    offset_mm: Option<i32>,
    xtalk_kcps: Option<i32>,
    signal_kcps: Option<u32>,
) -> Result<()>
```

Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is
the only way to know what the configuration registers hold. `mode` goes on before the budget;
`offset_mm` / `xtalk_kcps` re-apply a stored calibration; `signal_kcps` replaces the blob's
signal threshold (each `None`: leave it). `inter_ms` 0 = continuous. An unsupported group is
refused before the re-init.

#### Vl53lxDevice::get_range_timing

```rust
pub fn get_range_timing(&self) -> Result<(u32, u32)>
```

(timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.

#### Vl53lxDevice::get_mode

```rust
pub fn get_mode(&self) -> Result<Option<String>>
```

The mode in use, or `None` on a product without modes.

#### Vl53lxDevice::budget_choices

```rust
pub fn budget_choices(&self) -> Result<Vec<u32>>
```

The only budgets accepted right now, or empty when any in [`Vl53lxDevice::budget_range`] will do.

#### Vl53lxDevice::snap_budget

```rust
pub fn snap_budget(&self, budget_ms: u32) -> Result<u32>
```

The nearest budget this product accepts.

#### Vl53lxDevice::get_offset_mm

```rust
pub fn get_offset_mm(&self) -> Result<i32>
```

#### Vl53lxDevice::set_offset_mm

```rust
pub fn set_offset_mm(&self, offset_mm: i32) -> Result<()>
```

#### Vl53lxDevice::get_xtalk_kcps

```rust
pub fn get_xtalk_kcps(&self) -> Result<i32>
```

#### Vl53lxDevice::set_xtalk_kcps

```rust
pub fn set_xtalk_kcps(&self, xtalk_kcps: i32) -> Result<()>
```

#### Vl53lxDevice::calibrate_offset

```rust
pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Offset calibration against a flat target; returns the offset programmed (store it on the host).
`nb_samples` `None`: the driver's default.

#### Vl53lxDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32>
```

Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).

#### Vl53lxDevice::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<Thresholds>
```

#### Vl53lxDevice::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()>
```

The distance-window interrupt; stays armed until the next init / configure.

#### Vl53lxDevice::get_signal_threshold_kcps

```rust
pub fn get_signal_threshold_kcps(&self) -> Result<u32>
```

#### Vl53lxDevice::set_signal_threshold_kcps

```rust
pub fn set_signal_threshold_kcps(&self, signal_kcps: u32) -> Result<()>
```

#### Vl53lxDevice::get_sigma_threshold_mm

```rust
pub fn get_sigma_threshold_mm(&self) -> Result<u32>
```

#### Vl53lxDevice::set_sigma_threshold_mm

```rust
pub fn set_sigma_threshold_mm(&self, sigma_mm: u32) -> Result<()>
```

#### Vl53lxDevice::get_roi

```rust
pub fn get_roi(&self) -> Result<(u8, u8)>
```

(x, y) SPADs.

#### Vl53lxDevice::set_roi

```rust
pub fn set_roi(&self, x: u8, y: u8) -> Result<()>
```

#### Vl53lxDevice::get_roi_center

```rust
pub fn get_roi_center(&self) -> Result<u8>
```

#### Vl53lxDevice::set_roi_center

```rust
pub fn set_roi_center(&self, center_spad: u8) -> Result<()>
```

#### Vl53lxDevice::start_temperature_update

```rust
pub fn start_temperature_update(&self) -> Result<()>
```

Re-run VHV after an ambient change over 8 °C.

#### Vl53lxDevice::perform_ref_spad_management

```rust
pub fn perform_ref_spad_management(&self) -> Result<(u32, bool)>
```

VL53L0X: re-measure the reference SPADs → (count, is_aperture).

#### Vl53lxDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by
the driver's interrupt-release writes.

#### Vl53lxDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53lxDevice::measure_once

```rust
pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Vl53lxMeasurement>
```

One polled measurement: start, data ready, read, release the interrupt, stop. Not while
streaming. `timeout` `None`: 1 s.

#### Vl53lxDevice::on_measurement

```rust
pub fn on_measurement(&self, callback: impl Fn(&Vl53lxMeasurement) + Send + Sync + 'static) -> Hook
```

Call `callback` for every streamed measurement (reader thread, or the thread of a call that
held the driver while the frame came in).

#### Vl53lxDevice::measurements

```rust
pub fn measurements(&self, capacity: usize) -> Subscription<Vl53lxMeasurement>
```

A pull queue of streamed measurements (bounded, drop-oldest).

#### Vl53lxDevice::get_measurement

```rust
pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Vl53lxMeasurement>
```

The next streamed measurement (contract 07 §3); `timeout` `None`: 2 s.

#### Vl53lxDevice::stream_parse_errors

```rust
pub fn stream_parse_errors(&self) -> u64
```

Stream reports dropped because the driver did not decode them.

#### Vl53lxDevice::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

### Vl53l0xDevice

```rust
pub struct Vl53l0xDevice(Vl53lxDevice);
```

VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.

#### Vl53l0xDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l0xDevice
```

Continue the open connection `dev` as a VL53L0X.

#### Vl53l0xDevice::into_inner

```rust
pub fn into_inner(self) -> Vl53lxDevice
```

The family class underneath (the product stays fixed).

### Vl53l1cxDevice

```rust
pub struct Vl53l1cxDevice(Vl53lxDevice);
```

VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.

#### Vl53l1cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l1cxDevice
```

Continue the open connection `dev` as a VL53L1CX.

#### Vl53l1cxDevice::into_inner

```rust
pub fn into_inner(self) -> Vl53lxDevice
```

The family class underneath (the product stays fixed).

### Vl53l1cbDevice

```rust
pub struct Vl53l1cbDevice(Vl53lxDevice);
```

VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk
calibration.

#### Vl53l1cbDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l1cbDevice
```

Continue the open connection `dev` as a VL53L1CB.

#### Vl53l1cbDevice::into_inner

```rust
pub fn into_inner(self) -> Vl53lxDevice
```

The family class underneath (the product stays fixed).

### Vl53l3cxDevice

```rust
pub struct Vl53l3cxDevice(Vl53lxDevice);
```

VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).

#### Vl53l3cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l3cxDevice
```

Continue the open connection `dev` as a VL53L3CX.

#### Vl53l3cxDevice::into_inner

```rust
pub fn into_inner(self) -> Vl53lxDevice
```

The family class underneath (the product stays fixed).

### Vl53l4cxDevice

```rust
pub struct Vl53l4cxDevice(Vl53lxDevice);
```

VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with
`init(Some("uld"), Some("VL53L4CD"))`.

#### Vl53l4cxDevice::attach

```rust
pub fn attach(dev: DepzDevice) -> Vl53l4cxDevice
```

Continue the open connection `dev` as a VL53L4CX.

#### Vl53l4cxDevice::into_inner

```rust
pub fn into_inner(self) -> Vl53lxDevice
```

The family class underneath (the product stays fixed).

### BareDriver

```rust
pub struct BareDriver {
    p: Arc<dyn Vl53lxPlatform>,
    img: DeviceImage,
    tuning: BTreeMap<&'static str, i64>,
    hist_cfg: HistConfig,
    hpp: HistPostProcessConfig,
    state: LlState,

    // Carried from frame to frame: the C driver keeps one bin_data struct and reads last frame's ambient
    // out of it when it picks the bin sequence, before this frame's ambient is estimated.
    ambient_events_sum: i64,

    // Factory data with no register of its own (read_p2p_data).
    rtn_good_spads_map: [u8; 32],
    optical_centre: Option<OpticalCentre>,
    cal_peak_rate_map: Option<CalPeakRateMap>,
    add_off_cal_data: Option<AdditionalOffsetCalData>,
    fmt_dmax_cal: Vec<(&'static str, i64)>,
    mm_roi: Vec<(&'static str, i64)>,
    result_osc_calibrate_val: i64,

    preset_mode: &'static str,
    phasecal_config_timeout_us: i64,
    mm_config_timeout_us: i64,
    range_config_timeout_us: i64,
    inter_measurement_period_ms: i64,
    measurement_mode: i64,
}
```

ST's VL53LX BareDriver ranging path, on top of [`DeviceImage`]: `VL53LX_read_p2p_data()`, the
histogram preset modes, `VL53LX_set_timeouts_us()`, `VL53LX_init_and_start_range()` and
`VL53LX_get_histogram_bin_data()`. Nothing here writes a register on its own: the preset builders
edit the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when ranging
starts.

#### BareDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>) -> BareDriver
```

A driver over `platform`, nothing read yet.

#### BareDriver::img

```rust
pub fn img(&self) -> &DeviceImage
```

The host-side register image.

#### BareDriver::img_mut

```rust
pub fn img_mut(&mut self) -> &mut DeviceImage
```

The host-side register image, to edit.

#### BareDriver::tuning

```rust
pub fn tuning(&self) -> &BTreeMap<&'static str, i64>
```

The tuning parameters, a mutable copy of [`TUNING`].

#### BareDriver::tuning_mut

```rust
pub fn tuning_mut(&mut self) -> &mut BTreeMap<&'static str, i64>
```

The tuning parameters, to edit (takes effect at the next preset).

#### BareDriver::hist_cfg

```rust
pub fn hist_cfg(&self) -> &HistConfig
```

The histogram (bin sequence) configuration.

#### BareDriver::hpp

```rust
pub fn hpp(&self) -> &HistPostProcessConfig
```

The post-processing configuration.

#### BareDriver::hpp_mut

```rust
pub fn hpp_mut(&mut self) -> &mut HistPostProcessConfig
```

The post-processing configuration, to edit.

#### BareDriver::state

```rust
pub fn state(&self) -> &LlState
```

The ll-driver state machine.

#### BareDriver::state_mut

```rust
pub fn state_mut(&mut self) -> &mut LlState
```

The ll-driver state machine, to step.

#### BareDriver::rtn_good_spads_map

```rust
pub fn rtn_good_spads_map(&self) -> &[u8; 32]
```

The good return SPADs, one bit each.

#### BareDriver::optical_centre

```rust
pub fn optical_centre(&self) -> Option<OpticalCentre>
```

The optical centre (after [`BareDriver::read_p2p_data`]).

#### BareDriver::cal_peak_rate_map

```rust
pub fn cal_peak_rate_map(&self) -> Option<&CalPeakRateMap>
```

The calibration peak-rate map (after [`BareDriver::read_p2p_data`]).

#### BareDriver::add_off_cal_data

```rust
pub fn add_off_cal_data(&self) -> Option<AdditionalOffsetCalData>
```

The mode-mitigation offset calibration, or ST's fallback (after [`BareDriver::read_p2p_data`]).

#### BareDriver::fmt_dmax_cal

```rust
pub fn fmt_dmax_cal(&self) -> &[(&'static str, i64)]
```

The dmax calibration inputs, by the reference's key.

#### BareDriver::mm_roi

```rust
pub fn mm_roi(&self) -> &[(&'static str, i64)]
```

The mode-mitigation ROI: x_centre, y_centre, width, height.

#### BareDriver::result_osc_calibrate_val

```rust
pub fn result_osc_calibrate_val(&self) -> i64
```

`result__osc_calibrate_val`.

#### BareDriver::preset_mode

```rust
pub fn preset_mode(&self) -> &'static str
```

The distance mode of the last preset.

#### BareDriver::phasecal_config_timeout_us

```rust
pub fn phasecal_config_timeout_us(&self) -> i64
```

The phasecal timeout, µs.

#### BareDriver::mm_config_timeout_us

```rust
pub fn mm_config_timeout_us(&self) -> i64
```

The MM timeout, µs.

#### BareDriver::range_config_timeout_us

```rust
pub fn range_config_timeout_us(&self) -> i64
```

The range timeout, µs (how long one range actually runs).

#### BareDriver::inter_measurement_period_ms

```rust
pub fn inter_measurement_period_ms(&self) -> i64
```

The inter-measurement period, ms.

#### BareDriver::measurement_mode

```rust
pub fn measurement_mode(&self) -> i64
```

The measurement mode of the last start.

#### BareDriver::read_p2p_data

```rust
pub fn read_p2p_data(&mut self) -> Result<()>
```

`VL53LX_read_p2p_data()`: three register blocks and four NVM regions (~0.1 s over USB).

#### BareDriver::data_init

```rust
pub fn data_init(&mut self) -> Result<()>
```

`VL53LX_data_init` + `VL53LX_DataInit`: the factory data, the default preset mode and timing
budget. Leaves the sensor untouched apart from the reads.

#### BareDriver::set_preset_mode

```rust
pub fn set_preset_mode(&mut self, mode: &str) -> Result<()>
```

`VL53LX_set_preset_mode()` for the three histogram ranging modes, by distance mode.

#### BareDriver::set_preset_mode_with

```rust
pub fn set_preset_mode_with(
    &mut self,
    mode: &str,
    dss_target_mcps: Option<i64>,
    inter_measurement_period_ms: Option<i64>,
) -> Result<()>
```

`VL53LX_set_preset_mode()` for the three histogram ranging modes, by distance mode; a `None`
DSS target or period keeps the tuning default / the current period.

#### BareDriver::is_l4

```rust
pub fn is_l4(&self) -> bool
```

`IsL4()` of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED.

#### BareDriver::distance_modes_for_die

```rust
pub fn distance_modes_for_die(&self) -> &'static [&'static str]
```

The distance modes this die offers.

#### BareDriver::fda_max_timing_budget_us

```rust
pub fn fda_max_timing_budget_us(&self) -> i64
```

The budget ceiling of this die, µs.

#### BareDriver::set_distance_mode

```rust
pub fn set_distance_mode(&mut self, mode: &str) -> Result<()>
```

`VL53LX_SetDistanceMode()`: the preset mode with the caller's timeouts put back. Short is
refused on an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong
side of the wrap).

#### BareDriver::set_timeouts_us

```rust
pub fn set_timeouts_us(&mut self, phasecal_us: i64, mm_us: i64, range_us: i64) -> Result<()>
```

`VL53LX_set_timeouts_us()`: the three timeouts into the timing registers.

#### BareDriver::set_inter_measurement_period_ms

```rust
pub fn set_inter_measurement_period_ms(&mut self, period_ms: i64) -> Result<()>
```

The period between two measurements, ms (0: back to back).

#### BareDriver::set_measurement_timing_budget_us

```rust
pub fn set_measurement_timing_budget_us(&mut self, budget_us: i64) -> Result<()>
```

`VL53LX_SetMeasurementTimingBudgetMicroSeconds()`: the range timeout is one sixth of the budget
after a fixed guard comes off.

#### BareDriver::load_patch

```rust
pub fn load_patch(&mut self) -> Result<()>
```

`VL53LX_load_patch()`: a six-instruction phasecal patch, written with the firmware stopped.

#### BareDriver::unload_patch

```rust
pub fn unload_patch(&mut self) -> Result<()>
```

`VL53LX_unload_patch()`.

#### BareDriver::init_and_start_range

```rust
pub fn init_and_start_range(&mut self) -> Result<()>
```

`VL53LX_init_and_start_range()` with every block: write the whole configuration and start.

#### BareDriver::init_and_start_range_blocks

```rust
pub fn init_and_start_range_blocks(&mut self, blocks: &[&str]) -> Result<()>
```

`VL53LX_init_and_start_range()`: write the configuration and start. `blocks` is ST's config
level — all of them to start, general_config onwards to let the next frame run.

#### BareDriver::clear_interrupt_and_start_next_range

```rust
pub fn clear_interrupt_and_start_next_range(&mut self) -> Result<()>
```

`VL53LX_clear_interrupt_and_enable_next_range()`: in histogram mode the interrupt is cleared by
rewriting general_config onwards.

#### BareDriver::stop_range

```rust
pub fn stop_range(&mut self) -> Result<()>
```

`VL53LX_stop_range()`.

#### BareDriver::get_histogram_bin_data

```rust
pub fn get_histogram_bin_data(&mut self, raw: Option<&[u8]>) -> Result<HistogramBinData>
```

`VL53LX_get_histogram_bin_data()`: one frame, 83 bytes, decoded into bins plus the metadata the
post-processing needs. `raw` is bytes that arrived some other way (a stream report); `None`
reads them here.

### HistConfig

```rust
pub struct HistConfig {
    pub low_amb_even_bin_0_1: i64,
    pub low_amb_even_bin_2_3: i64,
    pub low_amb_even_bin_4_5: i64,
    pub low_amb_odd_bin_0_1: i64,
    pub low_amb_odd_bin_2_3: i64,
    pub low_amb_odd_bin_4_5: i64,
    pub mid_amb_even_bin_0_1: i64,
    pub mid_amb_even_bin_2_3: i64,
    pub mid_amb_even_bin_4_5: i64,
    pub mid_amb_odd_bin_0_1: i64,
    pub mid_amb_odd_bin_2: i64,
    pub mid_amb_odd_bin_3_4: i64,
    pub mid_amb_odd_bin_5: i64,
    pub user_bin_offset: i64,
    pub high_amb_even_bin_0_1: i64,
    pub high_amb_even_bin_2_3: i64,
    pub high_amb_even_bin_4_5: i64,
    pub high_amb_odd_bin_0_1: i64,
    pub high_amb_odd_bin_2_3: i64,
    pub high_amb_odd_bin_4_5: i64,
    pub amb_thresh_low: i64,
    pub amb_thresh_high: i64,
    pub spad_array_selection: i64,
}
```

`VL53LX_histogram_config_t`. It has no registers of its own: the bin sequence is smuggled into
static_config and timing_config fields that mean something else in lite mode
([`HistConfig::copy_to_static_cfg`]).

#### HistConfig::set_bin_sequence

```rust
pub fn set_bin_sequence(&mut self, even: &[i64; 6], odd: &[i64; 6])
```

`VL53LX_init_histogram_config_structure()`: six even and six odd bin codes, packed two to a
byte, repeated across the three ambient levels.

#### HistConfig::copy_to_static_cfg

```rust
pub fn copy_to_static_cfg(&self, img: &mut DeviceImage)
```

`VL53LX_copy_hist_cfg_to_static_cfg()`: lite-mode fields the firmware reads as the bin
sequence.

#### HistConfig::bin_sequence

```rust
pub fn bin_sequence(&self, stream_count: i64, ambient_events_sum: i64) -> [i64; 6]
```

`VL53LX_hist_get_bin_sequence_config()`: which sequence the device used for this frame, even
or odd by the stream count.

### HistPostProcessConfig

```rust
pub struct HistPostProcessConfig {
    pub hist_algo_select: i64,
    pub hist_target_order: i64,
    pub filter_woi0: i64,
    pub filter_woi1: i64,
    pub hist_amb_est_method: i64,
    pub ambient_thresh_sigma0: i64,
    pub ambient_thresh_sigma1: i64,
    pub ambient_thresh_events_scaler: i64,
    pub min_ambient_thresh_events: i64,
    pub noise_threshold: i64,
    pub signal_total_events_limit: i64,
    pub sigma_estimator_sigma_ref_mm: i64,
    pub sigma_thresh: i64,
    /// The MM1/MM2 offset, quarter millimetres.
    pub range_offset_mm: i64,
    pub gain_factor: i64,
    pub valid_phase_low: i64,
    pub valid_phase_high: i64,
    pub consistency_phase_tolerance: i64,
    pub consistency_event_sigma: i64,
    pub consistency_event_min_spad_count: i64,
    pub consistency_min_max_tolerance: i64,
    pub crosstalk_compensation_enable: i64,
    pub crosstalk_detect_min_valid_range_mm: i64,
    pub crosstalk_detect_max_valid_range_mm: i64,
    pub crosstalk_detect_max_valid_rate_kcps: i64,
    pub crosstalk_detect_max_sigma_mm: i64,
    pub crosstalk_detect_event_sigma: i64,
    pub crosstalk_detect_min_max_tolerance: i64,
    /// Filled in by `read_p2p_data()` from the customer NVM block.
    pub crosstalk_compensation_plane_offset_kcps: i64,
    pub crosstalk_compensation_x_plane_gradient_kcps: i64,
    pub crosstalk_compensation_y_plane_gradient_kcps: i64,
}
```

`VL53LX_hist_post_process_config_t`, as `VL53LX_init_hist_post_process_config_struct()` leaves
it; the preset modes set the valid phase window here.

### HistogramBinData

```rust
pub struct HistogramBinData {
    /// `result__interrupt_status`.
    pub result_interrupt_status: i64,
    /// `result__range_status`.
    pub result_range_status: i64,
    /// `result__report_status`.
    pub result_report_status: i64,
    /// `result__stream_count`: the device's frame counter.
    pub result_stream_count: i64,
    /// `result__dss_actual_effective_spads`, 1/256 SPAD.
    pub result_dss_actual_effective_spads: i64,
    /// `phasecal_result__reference_phase`.
    pub phasecal_result_reference_phase: i64,
    /// `phasecal_result__vcsel_start`.
    pub phasecal_result_vcsel_start: i64,
    /// The photon counts (always [`regs::HISTOGRAM_BUFFER_SIZE`] long; `bins_in_data` are in use).
    pub bin_data: Vec<i64>,
    /// `zone_id`.
    pub zone_id: i64,
    /// `VL53LX_p_019`: the first bin.
    pub first_bin: i64,
    /// `VL53LX_p_020`: bins in the buffer.
    pub number_of_bins: i64,
    /// `VL53LX_p_021`: bins in use.
    pub bins_in_data: i64,
    /// `cal_config__vcsel_start`.
    pub cal_config_vcsel_start: i64,
    /// The VCSEL pulse width.
    pub vcsel_width: i64,
    /// `VL53LX_p_015`: the fast oscillator frequency.
    pub fast_osc_frequency: i64,
    /// `VL53LX_p_005`: the VCSEL period register value the frame ran at.
    pub vcsel_period: i64,
    /// The bin-sequence codes.
    pub bin_seq: [i64; 6],
    /// How often each code was sampled.
    pub bin_rep: [i64; 6],
    /// The smallest bin.
    pub min_bin_value: i64,
    /// The largest bin.
    pub max_bin_value: i64,
    /// Ambient-only bins at the front.
    pub number_of_ambient_bins: i64,
    /// Samples the ambient estimate is over.
    pub number_of_ambient_samples: i64,
    /// Their sum.
    pub ambient_events_sum: i64,
    /// `VL53LX_p_028`: ambient events per bin.
    pub ambient_per_bin: i64,
    /// Macro periods the frame ran for.
    pub total_periods_elapsed: i64,
    /// Peak window duration, µs.
    pub peak_duration_us: i64,
    /// Window-of-interest duration, µs.
    pub woi_duration_us: i64,
    /// The phase of zero distance.
    pub zero_distance_phase: i64,
    /// `roi_config__user_roi_centre_spad`.
    pub roi_config_user_roi_centre_spad: i64,
    /// `roi_config__user_roi_requested_global_xy_size`.
    pub roi_config_user_roi_requested_global_xy_size: i64,
}
```

`VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read them —
the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient
estimate. Field names are the reference's (the C names, `__` folded to `_`);
[`HistogramBinData::fields`] gives them under the exact reference keys.

#### HistogramBinData::range_status

```rust
pub fn range_status(&self) -> i64
```

The device's range status (`result__range_status` masked).

#### HistogramBinData::fields

```rust
pub fn fields(&self) -> Vec<(&'static str, Value)>
```

The fields under the C driver's (and the Python SDK's) names, in the reference's order.

#### HistogramBinData::set_field

```rust
pub fn set_field(&mut self, key: &str, value: &Value) -> bool
```

Set one field by its reference key (the inverse of [`HistogramBinData::fields`]); `false` when
the key is unknown or the value has the wrong shape.

### BareDeviceState

```rust
pub enum BareDeviceState {
    /// Not ranging.
    #[default]
    SwStandby,
    /// Waiting for the grouped-parameter-hold handshake.
    RangingWaitGphSync,
    /// Ranging, frames valid.
    RangingOutputData,
    /// Ranging, DSS automatic.
    RangingDssAuto,
}
```

The ll-driver device states, single-zone case.

### LlState

```rust
pub struct LlState {
    pub cfg_device_state: BareDeviceState,
    pub cfg_stream_count: i64,
    pub cfg_gph_id: i64,
    pub cfg_timing_status: i64,
    pub rd_device_state: BareDeviceState,
    pub rd_stream_count: i64,
    pub rd_gph_id: i64,
    pub rd_timing_status: i64,
}
```

`VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to one zone: the GPH handshake (the id stamped
into the dynamic config tells the device a configuration is new) and the timing-status flip
(whether a frame ran on VCSEL period A or B, which the post-processing has to know).

#### LlState::reset

```rust
pub fn reset(&mut self)
```

Back to standby.

#### LlState::update_rd

```rust
pub fn update_rd(&mut self, mode_start: i64, grouped_parameter_hold: i64)
```

`VL53LX_update_ll_driver_rd_state()`: one frame read.

#### LlState::update_cfg

```rust
pub fn update_cfg(&mut self, mode_start: i64)
```

`VL53LX_update_ll_driver_cfg_state()`: one configuration written.

### RangeData

```rust
pub struct RangeData {
    pub range_id: i64,
    /// `VL53LX_p_012`, window start.
    pub start_bin: i64,
    /// `VL53LX_p_019`, first bin over threshold.
    pub first_bin: i64,
    /// `VL53LX_p_023`.
    pub peak_bin: i64,
    /// `VL53LX_p_024`.
    pub last_bin: i64,
    /// `VL53LX_p_013`.
    pub end_bin: i64,
    /// `VL53LX_p_025`, bins over threshold.
    pub width_bins: i64,
    /// `VL53LX_p_029`, bins in the window.
    pub window_bins: i64,
    pub vcsel_width: i64,
    pub fast_osc_frequency: i64,
    pub zero_distance_phase: i64,
    /// `VL53LX_p_004`, 1/256 SPAD.
    pub spads: i64,
    pub total_periods_elapsed: i64,
    pub peak_duration_us: i64,
    pub woi_duration_us: i64,
    /// `VL53LX_p_016`.
    pub ambient_events: i64,
    /// `VL53LX_p_017`.
    pub total_events: i64,
    /// `VL53LX_p_010`.
    pub signal_events: i64,
    pub peak_signal_count_rate_mcps: i64,
    pub avg_signal_count_rate_mcps: i64,
    pub ambient_count_rate_mcps: i64,
    pub total_rate_per_spad_mcps: i64,
    /// `VL53LX_p_009`.
    pub signal_events_per_spad_kcps: i64,
    /// `VL53LX_p_002`, 9.7 fixed point mm.
    pub sigma: i64,
    /// `VL53LX_p_026`.
    pub phase_start: i64,
    /// `VL53LX_p_011`.
    pub phase_mean: i64,
    /// `VL53LX_p_027`.
    pub phase_end: i64,
    pub min_range_mm: i64,
    pub median_range_mm: i64,
    pub max_range_mm: i64,
    /// A `VL53LX_DeviceError` (the `DEVICE_ERROR_*` constants of [`super::hist`]).
    pub range_status: i64,
}
```

`VL53LX_range_data_t`: one target. The event sums and phases are what the algorithm produced; the
`*_range_mm` fields and the rates are what they mean.

#### RangeData::sigma_mm

```rust
pub fn sigma_mm(&self) -> i64
```

Sigma, whole mm.

#### RangeData::signal_kcps

```rust
pub fn signal_kcps(&self) -> i64
```

The peak signal rate (9.7 Mcps) in kcps.

#### RangeData::ambient_kcps

```rust
pub fn ambient_kcps(&self) -> i64
```

The ambient rate (9.7 Mcps) in kcps.

#### RangeData::fields

```rust
pub fn fields(&self) -> Vec<(&'static str, i64)>
```

Every field under the reference's name, in its order.

### RangeResults

```rust
pub struct RangeResults {
    /// The frame's `result__stream_count`.
    pub stream_count: i64,
    /// `VL53LX_p_003`: the active results, at most four.
    pub targets: Vec<RangeData>,
    /// `VL53LX_p_022`: the ambient dmax per reflectance (zero: not ported).
    pub ambient_dmax_mm: [i64; 5],
    /// The wrap dmax.
    pub wrap_dmax_mm: i64,
}
```

`VL53LX_range_results_t`, reduced to the single-zone case.

### MAX_BIN_SEQUENCE_LENGTH

```rust
pub const MAX_BIN_SEQUENCE_LENGTH: usize = 6;
```

Codes in one bin sequence.

### MAX_BIN_SEQUENCE_CODE

```rust
pub const MAX_BIN_SEQUENCE_CODE: i64 = 15;
```

The largest bin-sequence code.

### MAX_PULSES

```rust
pub const MAX_PULSES: usize = 8;
```

`VL53LX_D_001`: pulses tracked per frame.

### MAX_RANGE_RESULTS

```rust
pub const MAX_RANGE_RESULTS: usize = 4;
```

Targets reported per frame.

### MAX_ALLOWED_PHASE

```rust
pub const MAX_ALLOWED_PHASE: i64 = 0xFFFF;
```

The phase ceiling.

### SPAD_TOTAL_COUNT_MAX

```rust
pub const SPAD_TOTAL_COUNT_MAX: i64 = (1 << 29) - 1;
```

The event-count ceiling of the rate maths.

### SPAD_TOTAL_COUNT_RES_THRES

```rust
pub const SPAD_TOTAL_COUNT_RES_THRES: i64 = 1 << 24;
```

Above this many events the rate maths drops resolution.

### SPEED_OF_LIGHT_IN_AIR_DIV8

```rust
pub const SPEED_OF_LIGHT_IN_AIR_DIV8: i64 = 299704 >> 3;
```

The speed of light in air / 8, mm/µs-ish units of the C driver.

### SIGMA_INVALID

```rust
pub const SIGMA_INVALID: i64 = 0xFFFF;
```

`VL53LX_D_002`: an invalid sigma.

### DEVICE_ERROR_NO_UPDATE

```rust
pub const DEVICE_ERROR_NO_UPDATE: i64 = 0;
```

`VL53LX_DEVICEERROR_NOUPDATE`.

### DEVICE_ERROR_RANGE_PHASE_CHECK

```rust
pub const DEVICE_ERROR_RANGE_PHASE_CHECK: i64 = 5;
```

`VL53LX_DEVICEERROR_RANGEPHASECHECK`.

### DEVICE_ERROR_SIGMA_THRESHOLD_CHECK

```rust
pub const DEVICE_ERROR_SIGMA_THRESHOLD_CHECK: i64 = 6;
```

`VL53LX_DEVICEERROR_SIGMATHRESHOLDCHECK`.

### DEVICE_ERROR_PHASE_CONSISTENCY

```rust
pub const DEVICE_ERROR_PHASE_CONSISTENCY: i64 = 7;
```

`VL53LX_DEVICEERROR_PHASECONSISTENCY`.

### DEVICE_ERROR_RANGE_COMPLETE

```rust
pub const DEVICE_ERROR_RANGE_COMPLETE: i64 = 9;
```

`VL53LX_DEVICEERROR_RANGECOMPLETE`.

### DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK

```rust
pub const DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK: i64 = 19;
```

`VL53LX_DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK`.

### DEVICE_ERROR_EVENT_CONSISTENCY

```rust
pub const DEVICE_ERROR_EVENT_CONSISTENCY: i64 = 20;
```

`VL53LX_DEVICEERROR_EVENTCONSISTENCY`.

### DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE

```rust
pub const DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE: i64 = 22;
```

`VL53LX_DEVICEERROR_RANGECOMPLETE_MERGED_PULSE`.

### DEVICE_ERROR_PREV_RANGE_NO_TARGETS

```rust
pub const DEVICE_ERROR_PREV_RANGE_NO_TARGETS: i64 = 23;
```

`VL53LX_DEVICEERROR_PREV_RANGE_NO_TARGETS`.

### HIST_TARGET_ORDER_STRONGEST_FIRST

```rust
pub const HIST_TARGET_ORDER_STRONGEST_FIRST: i64 = 1;
```

Target order: strongest first.

### HIST_TARGET_ORDER_CLOSEST_FIRST

```rust
pub const HIST_TARGET_ORDER_CLOSEST_FIRST: i64 = 2;
```

Target order: closest first.

### RANGE_STATUS_RANGE_VALID

```rust
pub const RANGE_STATUS_RANGE_VALID: i32 = 0;
```

`VL53LX_RANGESTATUS_RANGE_VALID`.

### RANGE_STATUS_SIGMA_FAIL

```rust
pub const RANGE_STATUS_SIGMA_FAIL: i32 = 1;
```

`VL53LX_RANGESTATUS_SIGMA_FAIL`.

### RANGE_STATUS_OUT_OF_BOUNDS_FAIL

```rust
pub const RANGE_STATUS_OUT_OF_BOUNDS_FAIL: i32 = 4;
```

`VL53LX_RANGESTATUS_OUTOFBOUNDS_FAIL`.

### RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK

```rust
pub const RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK: i32 = 6;
```

`VL53LX_RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK_FAIL`.

### RANGE_STATUS_WRAP_TARGET_FAIL

```rust
pub const RANGE_STATUS_WRAP_TARGET_FAIL: i32 = 7;
```

`VL53LX_RANGESTATUS_WRAP_TARGET_FAIL`.

### RANGE_STATUS_RANGE_VALID_MERGED_PULSE

```rust
pub const RANGE_STATUS_RANGE_VALID_MERGED_PULSE: i32 = 11;
```

`VL53LX_RANGESTATUS_RANGE_VALID_MERGED_PULSE`.

### RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL

```rust
pub const RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL: i32 = 12;
```

`VL53LX_RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL`.

### RANGE_STATUS_NONE

```rust
pub const RANGE_STATUS_NONE: i32 = 255;
```

`VL53LX_RANGESTATUS_NONE`.

### status_name

```rust
pub fn status_name(status: i32) -> Option<&'static str>
```

UM2931 range status name, or `None` for a status the histogram path does not produce.

### PrevTarget

```rust
pub struct PrevTarget {
    /// Its ambient events.
    pub ambient_events: i64,
    /// Its total events.
    pub total_events: i64,
    /// Its mean phase.
    pub phase_mean: i64,
    /// Its device error.
    pub range_status: i64,
}
```

One target of the frame before, reduced to what the consistency checks read.

### py_mod

```rust
pub fn py_mod(a: i64, b: i64) -> i64
```

The modulo of the reference (the sign of the divisor).

### isqrt

```rust
pub fn isqrt(num: i64) -> i64
```

`VL53LX_isqrt()`: the floor of the square root of a uint32 argument.

### calc_pll_period_mm

```rust
pub fn calc_pll_period_mm(fast_osc_frequency: i64) -> i64
```

`VL53LX_calc_pll_period_mm()`.

### rate_maths

```rust
pub fn rate_maths(events: i64, time_us: i64) -> i64
```

`VL53LX_rate_maths()`: events over a duration → count rate in 9.7 Mcps.

### rate_per_spad_maths

```rust
pub fn rate_per_spad_maths(frac_bits: u32, peak_count_rate: i64, num_spads: i64, max_output_value: i64) -> i64
```

`VL53LX_rate_per_spad_maths()`.

### events_per_spad_maths

```rust
pub fn events_per_spad_maths(events: i64, num_spads: i64, duration: i64) -> i64
```

`VL53LX_events_per_spad_maths()` → kcps per SPAD.

### range_maths

```rust
pub fn range_maths(
    fast_osc_frequency: i64,
    phase: i64,
    zero_distance_phase: i64,
    fractional_bits: i32,
    gain_factor: i64,
    range_offset_mm: i64,
) -> i64
```

`VL53LX_range_maths()`: phase → millimetres.

### average_repeated_bins

```rust
pub fn average_repeated_bins(src: &HistogramBinData) -> HistogramBinData
```

`VL53LX_f_031`: fold the six bin-sequence codes together. Repeats of a code land on the same four
output bins and are averaged, so what comes out is one bin per distinct code.

### calc_zero_distance_phase

```rust
pub fn calc_zero_distance_phase(bins: &mut HistogramBinData)
```

`VL53LX_hist_calc_zero_distance_phase`.

### estimate_ambient_from_thresholded_bins

```rust
pub fn estimate_ambient_from_thresholded_bins(ambient_threshold_sigma: i64, bins: &mut HistogramBinData)
```

`VL53LX_hist_estimate_ambient_from_thresholded_bins()`: the floor from every bin near the
quietest.

### estimate_ambient_from_ambient_bins

```rust
pub fn estimate_ambient_from_ambient_bins(bins: &mut HistogramBinData)
```

`VL53LX_hist_estimate_ambient_from_ambient_bins()`: the better estimate, from the VCSEL-off bins.

### remove_ambient_bins

```rust
pub fn remove_ambient_bins(bins: &mut HistogramBinData)
```

`VL53LX_hist_remove_ambient_bins`: drop the ambient-only bins off the front, so bin 0 is phase
zero.

### process_data

```rust
pub fn process_data(bins_input: &HistogramBinData, hpp: &HistPostProcessConfig) -> Result<RangeResults>
```

`VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off, one merged frame.

### process_data_merged

```rust
pub fn process_data_merged(
    bins_input: &HistogramBinData,
    hpp: &HistPostProcessConfig,
    merge_nb: i64,
) -> Result<RangeResults>
```

`VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off, `merge_nb` frames merged.

### events_consistency_check

```rust
pub fn events_consistency_check(
    event_sigma: i64,
    min_effective_spad_count: i64,
    prev_frame: &FrameHistory,
    prev: &PrevTarget,
    target: &RangeData,
) -> i64
```

`VL53LX_hist_events_consistency_check()`: scale the previous frame's events onto this exposure and
ask whether the difference fits inside the shot noise of the pair, `event_sigma` sixty-fourths
wide. → a device error.

### merged_pulse_check

```rust
pub fn merged_pulse_check(min_max_tolerance_mm: i64, target: &RangeData) -> i64
```

`VL53LX_hist_merged_pulse_check()`: a pulse far wider than one target should be is two targets in
one. → a device error.

### convert_status

```rust
pub fn convert_status(device_error: i64) -> i32
```

`ConvertStatusHisto()` (vl53lx_api.c), the cases the histogram path produces here.

### to_range

```rust
pub fn to_range(results: &RangeResults, bins: &HistogramBinData) -> Vl53lxRange
```

The frame's results as the family-wide range: every return in `targets`, the first one also the
range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the family
does. `extra`: `stream_count`, `ambient_per_bin`, and with a target `min_range_mm`,
`max_range_mm`, `peak_bin`.

### Vl53lxHistogramDriver

```rust
pub struct Vl53lxHistogramDriver {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    bare: BareDriver,
    mode: &'static str,
    budget_ms: u32,
    inter_ms: u32,
    modes: &'static [&'static str],
    budget_range: (u32, u32),
    // The A/B alternation rides on the ll-driver state, which the polled C loop advances in the call
    // that clears the interrupt. Here the bridge clears it, so the state is advanced when a new frame
    // arrives — told by the device's own result__stream_count, so decoding the same sample twice does
    // not step it twice.
    last_stream_count: Option<u8>,
    // The phase and event consistency checks read the frame before this one.
    history: FrameHistory,
}
```

ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD /
L4CX: the die hands over 24 raw photon-count bins per frame and [`hist`] turns them into up to
four targets. What it costs against the light drivers: no calibrations and no detection
thresholds, and the first frame of a stream reports status 6 (no wrap check done) because the
check needs the frame before. [`Vl53lxDriver::bin_data`] hands out a [`HistogramBinData`].

#### Vl53lxHistogramDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Self
```

A driver for `product` over `platform`; nothing touches the bus until
[`Vl53lxDriver::sensor_init`].

#### Vl53lxHistogramDriver::bare

```rust
pub fn bare(&self) -> &BareDriver
```

The BareDriver underneath: device image, factory data, preset and post-processing config.

#### Vl53lxHistogramDriver::bare_mut

```rust
pub fn bare_mut(&mut self) -> &mut BareDriver
```

The BareDriver underneath, to tune.

#### Vl53lxHistogramDriver::history

```rust
pub fn history(&self) -> &FrameHistory
```

The frame history the consistency checks read.

#### Vl53lxHistogramDriver::wait_boot

```rust
pub fn wait_boot(&self, timeout: Duration) -> Result<()>
```

Poll `FIRMWARE__SYSTEM_STATUS` until the die has booted.

### FrameHistory

```rust
pub struct FrameHistory {
    rd_device_state: Option<BareDeviceState>,
    targets: Vec<PrevTarget>,
    total_periods_elapsed: i64,
    spads: i64,
}
```

The frame before this one, reduced to what the consistency checks read (`VL53LX_zone_hist_info_t`
and `VL53LX_zone_objects_t` for one zone). The engine alternates two VCSEL periods, so one frame
cannot say whether a target is inside the unambiguous range or an echo wrapped into it; the API
compares consecutive frames. The BareDriver keeps no history, so the driver owns one of these and
hands it every frame.

#### FrameHistory::new

```rust
pub fn new() -> FrameHistory
```

An empty history: the first [`FrameHistory::apply`] only remembers.

#### FrameHistory::total_periods_elapsed

```rust
pub fn total_periods_elapsed(&self) -> i64
```

Macro periods the remembered frame ran for.

#### FrameHistory::spads

```rust
pub fn spads(&self) -> i64
```

Effective SPADs of the remembered frame, 1/256.

#### FrameHistory::targets

```rust
pub fn targets(&self) -> &[PrevTarget]
```

The remembered frame's targets.

#### FrameHistory::reset

```rust
pub fn reset(&mut self)
```

Forget the stream so far: the next [`FrameHistory::apply`] only remembers.

#### FrameHistory::apply

```rust
pub fn apply(
    &mut self,
    results: &mut RangeResults,
    bins: &HistogramBinData,
    rd_device_state: BareDeviceState,
    hpp: &HistPostProcessConfig,
)
```

Check `results` against the remembered frame, then remember it — C's order: both checks run
before hist_data and range_results are copied into the zone history.

### RegField

```rust
pub struct RegField {
    /// The C field name.
    pub name: &'static str,
    /// Byte offset inside the block.
    pub off: usize,
    /// Width in bytes, big-endian on the wire.
    pub width: usize,
    /// Two's complement over `width` bytes.
    pub signed: bool,
    /// The bits the C codec keeps.
    pub mask: Option<i64>,
}
```

One field of a register block: byte offset, width in bytes, signedness and the mask the C codec
applies (`None`: none).

### RegBlockDef

```rust
pub struct RegBlockDef {
    /// The block's name (the C struct without `VL53LX_` / `_t`).
    pub name: &'static str,
    /// First register address.
    pub base: u16,
    /// Bytes in the block, reserved holes included.
    pub size: usize,
    /// Whether the driver may write it.
    pub writable: bool,
    /// The named fields, in block order.
    pub fields: &'static [RegField],
}
```

One contiguous I2C block of the device image (a `VL53LX_*_t` struct of the C driver).

### block_def

```rust
pub fn block_def(name: &str) -> Option<&'static RegBlockDef>
```

The layout of the block `name`, or `None` when there is none.

### tuning_default

```rust
pub fn tuning_default(name: &str) -> Option<i64>
```

A tuning parameter by its C field name (`tp_*`), as [`TUNING`] holds it after init.

### header_default

```rust
pub fn header_default(name: &str) -> Option<i64>
```

A raw tuning-header value (`TUNINGPARM_*_DEFAULT`) from [`DEFAULTS`].

### RegBlock

```rust
pub struct RegBlock {
    def: &'static RegBlockDef,
    values: Vec<i64>,
    raw: Vec<u8>,
}
```

One register block's worth of device image, addressed by the C field name:
`img.static_config().get("dss_config__target_total_rate_mcps")`. Bytes no field names (the
reserved holes) keep whatever was read, so a read-modify-write round trip is byte-exact.

#### RegBlock::from_def

```rust
pub fn from_def(def: &'static RegBlockDef) -> RegBlock
```

A zeroed block of the layout `def`.

#### RegBlock::named

```rust
pub fn named(name: &str) -> Option<RegBlock>
```

A zeroed block by name, or `None` when there is no such block.

#### RegBlock::name

```rust
pub fn name(&self) -> &'static str
```

The block's name.

#### RegBlock::def

```rust
pub fn def(&self) -> &'static RegBlockDef
```

The block's layout.

#### RegBlock::base

```rust
pub fn base(&self) -> u16
```

First register address.

#### RegBlock::size

```rust
pub fn size(&self) -> usize
```

Bytes in the block.

#### RegBlock::try_get

```rust
pub fn try_get(&self, field: &str) -> Option<i64>
```

The field by its C name, or `None` when the block has none.

#### RegBlock::get

```rust
pub fn get(&self, field: &str) -> i64
```

The field by its C name.

# Panics
When the block has no such field (the driver only names fields of the generated layout).

#### RegBlock::try_set

```rust
pub fn try_set(&mut self, field: &str, value: i64) -> bool
```

Set a field (the value is clipped only on [`RegBlock::encode`]); `false` when there is no such
field.

#### RegBlock::set

```rust
pub fn set(&mut self, field: &str, value: i64)
```

Set a field (the value is clipped only on [`RegBlock::encode`]).

# Panics
When the block has no such field.

#### RegBlock::items

```rust
pub fn items(&self) -> Vec<(&'static str, i64)>
```

Every field in block order, `(name, value)`.

#### RegBlock::decode

```rust
pub fn decode(&mut self, raw: &[u8]) -> Result<()>
```

Take the block's bytes (at least [`RegBlock::size`] of them) into the fields.

#### RegBlock::encode

```rust
pub fn encode(&self) -> Vec<u8>
```

The block's bytes: every field masked and clipped to its width the way the C codec does, the
holes as last decoded.

### RANGE_START_BLOCKS

```rust
pub const RANGE_START_BLOCKS: &[&str] = &[
    "static_nvm_managed",
    "customer_nvm_managed",
    "static_config",
    "general_config",
    "timing_config",
    "dynamic_config",
    "system_control",
];
```

The blocks `init_and_start_range` pushes, in address order: one transfer.

### DeviceImage

```rust
pub struct DeviceImage {
    blocks: Vec<RegBlock>,
}
```

The device's register image, block by block. The C driver never reads a configuration register
on its own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087
in a single 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the
count where the C driver has it. Editing a field only touches the host copy; [`DeviceImage::push`]
reaches the sensor.

#### DeviceImage::new

```rust
pub fn new() -> DeviceImage
```

Every block of [`BLOCKS`], zeroed.

#### DeviceImage::try_block

```rust
pub fn try_block(&self, name: &str) -> Option<&RegBlock>
```

The block `name`, or `None` when there is none.

#### DeviceImage::block

```rust
pub fn block(&self, name: &str) -> &RegBlock
```

The block `name`.

# Panics
When there is no such block.

#### DeviceImage::block_mut

```rust
pub fn block_mut(&mut self, name: &str) -> &mut RegBlock
```

The block `name`, to edit.

# Panics
When there is no such block.

#### DeviceImage::pull

```rust
pub fn pull(&mut self, p: &dyn Vl53lxPlatform, name: &str) -> Result<()>
```

Read one block off the sensor into the image.

#### DeviceImage::push

```rust
pub fn push(&self, p: &dyn Vl53lxPlatform, name: &str) -> Result<()>
```

Write one block from the image to the sensor.

#### DeviceImage::pull_range

```rust
pub fn pull_range(&mut self, p: &dyn Vl53lxPlatform, names: &[&str]) -> Result<()>
```

Read the named blocks as one transfer.

#### DeviceImage::push_range

```rust
pub fn push_range(&self, p: &dyn Vl53lxPlatform, names: &[&str]) -> Result<()>
```

The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver.

#### DeviceImage::dump

```rust
pub fn dump(&self, names: &[&str]) -> String
```

Every field of the named blocks, one per line.

#### DeviceImage::dump_all

```rust
pub fn dump_all(&self) -> String
```

Every field of every block, one per line.

### encode_row_col

```rust
pub fn encode_row_col(row: i64, col: i64) -> i64
```

A SPAD's (row, col) → its number.

### decode_row_col

```rust
pub fn decode_row_col(spad_number: i64) -> (i64, i64)
```

A SPAD number → (row, col).

### decode_zone_size

```rust
pub fn decode_zone_size(encoded_xy_size: i64) -> (i64, i64)
```

An encoded zone size → (width, height).

### encode_zone_size

```rust
pub fn encode_zone_size(width: i64, height: i64) -> i64
```

(width, height) → the encoded zone size.

### decode_zone_limits

```rust
pub fn decode_zone_limits(encoded_xy_centre: i64, encoded_xy_size: i64) -> (i64, i64, i64, i64)
```

→ (x_ll, y_ll, x_ur, y_ur), clipped to the SPAD array.

### is_aperture_location

```rust
pub fn is_aperture_location(row: i64, col: i64) -> bool
```

Whether the SPAD at (row, col) sits under the aperture.

### calc_mm_effective_spads

```rust
pub fn calc_mm_effective_spads(
    encoded_mm_roi_centre: i64,
    encoded_mm_roi_size: i64,
    encoded_zone_centre: i64,
    encoded_zone_size: i64,
    good_spads: &[u8; 32],
    aperture_attenuation: i64,
) -> (i64, i64)
```

→ (mm inner, mm outer) effective SPADs, in 1/256 of a SPAD. Needed because our boards ship
without the FMT offset calibration: the driver works out the SPAD counts the nominal rates apply
to.

### combine_mm1_mm2_offsets

```rust
pub fn combine_mm1_mm2_offsets(
    mm1_offset_mm: i64,
    mm2_offset_mm: i64,
    encoded_mm_roi_centre: i64,
    encoded_mm_roi_size: i64,
    encoded_zone_centre: i64,
    encoded_zone_size: i64,
    cal: &AdditionalOffsetCalData,
    good_spads: &[u8; 32],
    aperture_attenuation: i64,
) -> i64
```

`VL53LX_hist_combine_mm1_mm2_offsets()` → `range_offset_mm`, in quarter millimetres: the MM1
(inner) and MM2 (outer) offsets weighted by the share of each region the ROI covers.

### rtn_good_spads

```rust
pub fn rtn_good_spads(nvm_copy_data: &RegBlock) -> [u8; 32]
```

The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap.

### calc_pll_period_us

```rust
pub fn calc_pll_period_us(fast_osc_frequency: i64) -> i64
```

The PLL period, 2^-30 s units (0 for a zero frequency).

### decode_vcsel_period

```rust
pub fn decode_vcsel_period(vcsel_period_reg: i64) -> i64
```

A VCSEL period register value → PLL clocks.

### calc_macro_period_us

```rust
pub fn calc_macro_period_us(fast_osc_frequency: i64, vcsel_period_reg: i64) -> i64
```

The macro period, 12.12 µs.

### calc_timeout_mclks

```rust
pub fn calc_timeout_mclks(timeout_us: i64, macro_period_us: i64) -> i64
```

A timeout in µs → macro periods.

### encode_timeout

```rust
pub fn encode_timeout(timeout_mclks: i64) -> i64
```

Macro periods → the (mantissa, exponent) register encoding.

### decode_timeout

```rust
pub fn decode_timeout(encoded: i64) -> i64
```

The register encoding → macro periods. The exponent shift wraps at 64 as Java's does (a real
register never comes near it).

### calc_encoded_timeout

```rust
pub fn calc_encoded_timeout(timeout_us: i64, macro_period_us: i64) -> i64
```

A timeout in µs → its register encoding.

### duration_maths

```rust
pub fn duration_maths(pll_period_us: i64, vcsel_parm_pclks: i64, window_vclks: i64, elapsed_mclks: i64) -> i64
```

`VL53LX_duration_maths()`: a window duration, µs.

### calc_timeout_register_values

```rust
pub fn calc_timeout_register_values(
    phasecal_us: i64,
    mm_us: i64,
    range_us: i64,
    fast_osc_frequency: i64,
    img: &mut DeviceImage,
) -> Option<()>
```

Fill the six timeout registers, for VCSEL period A and then B. `None` when the oscillator
frequency is 0.

### OpticalCentre

```rust
pub struct OpticalCentre {
    /// X centre, 4.4 fixed point SPADs.
    pub x_centre: i64,
    /// Y centre, 4.4 fixed point SPADs.
    pub y_centre: i64,
}
```

`VL53LX_optical_centre_t`.

### CalPeakRateMap

```rust
pub struct CalPeakRateMap {
    /// Calibration distance, mm.
    pub cal_distance_mm: i64,
    /// Calibration reflectance, %.
    pub cal_reflectance_pc: i64,
    /// Samples in the map.
    pub max_samples: i64,
    /// Map width.
    pub width: i64,
    /// Map height.
    pub height: i64,
    /// The peak rates, 9.7 Mcps, row by row.
    pub peak_rate_mcps: Vec<i64>,
}
```

`VL53LX_cal_peak_rate_map_t`.

### AdditionalOffsetCalData

```rust
pub struct AdditionalOffsetCalData {
    /// MM1 effective SPADs, 1/256.
    pub mm_inner_actual_effective_spads: i64,
    /// MM2 effective SPADs, 1/256.
    pub mm_outer_actual_effective_spads: i64,
    /// MM1 peak signal rate, 9.7 Mcps.
    pub mm_inner_peak_signal_count_rtn_mcps: i64,
    /// MM2 peak signal rate, 9.7 Mcps.
    pub mm_outer_peak_signal_count_rtn_mcps: i64,
}
```

`VL53LX_additional_offset_cal_data_t`.

### FmtRangeData

```rust
pub struct FmtRangeData {
    /// Effective return SPADs, 1/256.
    pub actual_effective_rtn_spads: i64,
    /// Requested reference SPADs.
    pub num_requested_ref_spads: i64,
    /// Reference SPAD location.
    pub ref_location: i64,
    /// Return peak signal rate, 9.7 Mcps.
    pub peak_signal_count_rate_rtn_mcps: i64,
    /// Return ambient rate, 9.7 Mcps.
    pub ambient_count_rate_rtn_mcps: i64,
    /// Reference peak signal rate, 9.7 Mcps.
    pub peak_signal_count_rate_ref_mcps: i64,
    /// Reference ambient rate, 9.7 Mcps.
    pub ambient_count_rate_ref_mcps: i64,
    /// Measured distance, mm.
    pub measured_distance_mm: i64,
    /// Its standard deviation, mm.
    pub measured_distance_stdev_mm: i64,
}
```

`VL53LX_decoded_nvm_fmt_range_data_t`.

### FMT_OPTICAL_CENTRE

```rust
pub const FMT_OPTICAL_CENTRE: (usize, usize) = (0x00B8, 4);
```

NVM region of the optical centre, (byte index, byte size).

### FMT_CAL_PEAK_RATE_MAP

```rust
pub const FMT_CAL_PEAK_RATE_MAP: (usize, usize) = (0x015C, 56);
```

NVM region of the peak-rate map.

### FMT_ADDITIONAL_OFFSET_CAL

```rust
pub const FMT_ADDITIONAL_OFFSET_CAL: (usize, usize) = (0x0194, 8);
```

NVM region of the mode-mitigation offset calibration.

### FMT_RANGE_RESULTS_140MM_DARK

```rust
pub const FMT_RANGE_RESULTS_140MM_DARK: (usize, usize) = (0x01AC, 16);
```

NVM region of the FMT range results at 140 mm, dark.

### read_region

```rust
pub fn read_region(p: &dyn Vl53lxPlatform, img: &mut DeviceImage, region: (usize, usize)) -> Result<Vec<u8>>
```

`VL53LX_read_nvm_raw_data()`: (byte index, byte size), both word-aligned → the bytes. The image
is passed so the firmware / power-force fields it toggles stay truthful.

### read_optical_centre

```rust
pub fn read_optical_centre(p: &dyn Vl53lxPlatform, img: &mut DeviceImage) -> Result<OpticalCentre>
```

The optical centre (zeroed on a never-programmed part).

### read_cal_peak_rate_map

```rust
pub fn read_cal_peak_rate_map(p: &dyn Vl53lxPlatform, img: &mut DeviceImage) -> Result<CalPeakRateMap>
```

The 5x5 calibration peak-rate map.

### read_additional_offset_cal_data

```rust
pub fn read_additional_offset_cal_data(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
) -> Result<AdditionalOffsetCalData>
```

The mode-mitigation offset calibration.

### read_fmt_range_results

```rust
pub fn read_fmt_range_results(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
    region: (usize, usize),
) -> Result<FmtRangeData>
```

The FMT range results of `region` ([`FMT_RANGE_RESULTS_140MM_DARK`] is the one the driver reads).

### POWER_MANAGEMENT_GO1_POWER_FORCE

```rust
pub const POWER_MANAGEMENT_GO1_POWER_FORCE: u16 = 0x0083;
```

`POWER_MANAGEMENT__GO1_POWER_FORCE`.

### FIRMWARE_ENABLE

```rust
pub const FIRMWARE_ENABLE: u16 = 0x0085;
```

`FIRMWARE__ENABLE`.

### RESULT_OSC_CALIBRATE_VAL

```rust
pub const RESULT_OSC_CALIBRATE_VAL: u16 = 0x00DE;
```

`RESULT__OSC_CALIBRATE_VAL`.

### PATCH_CTRL

```rust
pub const PATCH_CTRL: u16 = 0x0470;
```

`PATCH__CTRL`.

### PATCH_JMP_ENABLES

```rust
pub const PATCH_JMP_ENABLES: u16 = 0x0472;
```

`PATCH__JMP_ENABLES`.

### PATCH_DATA_ENABLES

```rust
pub const PATCH_DATA_ENABLES: u16 = 0x0474;
```

`PATCH__DATA_ENABLES`.

### PATCH_OFFSET_0

```rust
pub const PATCH_OFFSET_0: u16 = 0x0476;
```

`PATCH__OFFSET_0`.

### PATCH_ADDRESS_0

```rust
pub const PATCH_ADDRESS_0: u16 = 0x0496;
```

`PATCH__ADDRESS_0`.

### HISTOGRAM_BIN_DATA_I2C_INDEX

```rust
pub const HISTOGRAM_BIN_DATA_I2C_INDEX: u16 = 0x0088;
```

First byte of the histogram frame (`RESULT__INTERRUPT_STATUS`).

### RESULT_HISTOGRAM_BIN_0_2

```rust
pub const RESULT_HISTOGRAM_BIN_0_2: u16 = 0x008E;
```

`RESULT__HISTOGRAM_BIN_0_2`.

### RESULT_HISTOGRAM_BIN_23_0

```rust
pub const RESULT_HISTOGRAM_BIN_23_0: u16 = 0x00D5;
```

`RESULT__HISTOGRAM_BIN_23_0`.

### PHASECAL_RESULT_REFERENCE_PHASE

```rust
pub const PHASECAL_RESULT_REFERENCE_PHASE: u16 = 0x00D6;
```

`PHASECAL_RESULT__REFERENCE_PHASE`.

### PHASECAL_RESULT_VCSEL_START

```rust
pub const PHASECAL_RESULT_VCSEL_START: u16 = 0x00D8;
```

`PHASECAL_RESULT__VCSEL_START`.

### RESULT_HISTOGRAM_BIN_23_0_MSB

```rust
pub const RESULT_HISTOGRAM_BIN_23_0_MSB: u16 = 0x00D9;
```

`RESULT__HISTOGRAM_BIN_23_0_MSB`.

### RESULT_HISTOGRAM_BIN_23_0_LSB

```rust
pub const RESULT_HISTOGRAM_BIN_23_0_LSB: u16 = 0x00DA;
```

`RESULT__HISTOGRAM_BIN_23_0_LSB`.

### HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES

```rust
pub const HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES: u16 = RESULT_HISTOGRAM_BIN_23_0_LSB - HISTOGRAM_BIN_DATA_I2C_INDEX + 1;
```

Bytes in one histogram frame (83).

### DEVICE_MEASUREMENT_MODE_BACK_TO_BACK

```rust
pub const DEVICE_MEASUREMENT_MODE_BACK_TO_BACK: i64 = 0x20;
```

`VL53LX_DEVICEMEASUREMENTMODE_BACKTOBACK`.

### DEVICE_MEASUREMENT_MODE_ABORT

```rust
pub const DEVICE_MEASUREMENT_MODE_ABORT: i64 = 0x80;
```

`VL53LX_DEVICEMEASUREMENTMODE_ABORT`.

### DEVICE_MEASUREMENT_MODE_STOP_MASK

```rust
pub const DEVICE_MEASUREMENT_MODE_STOP_MASK: i64 = 0x0F;
```

`VL53LX_DEVICEMEASUREMENTMODE_STOP_MASK`.

### DEVICE_MEASUREMENT_MODE_MODE_MASK

```rust
pub const DEVICE_MEASUREMENT_MODE_MODE_MASK: i64 = 0xF0;
```

`VL53LX_DEVICEMEASUREMENTMODE_MODE_MASK`.

### DEVICE_SCHEDULER_MODE_STREAMING

```rust
pub const DEVICE_SCHEDULER_MODE_STREAMING: i64 = 0x01;
```

`VL53LX_DEVICESCHEDULERMODE_STREAMING`.

### DEVICE_SCHEDULER_MODE_HISTOGRAM

```rust
pub const DEVICE_SCHEDULER_MODE_HISTOGRAM: i64 = 0x02;
```

`VL53LX_DEVICESCHEDULERMODE_HISTOGRAM`.

### DEVICE_READOUT_MODE_SINGLE_SD

```rust
pub const DEVICE_READOUT_MODE_SINGLE_SD: i64 = 0x00 << 2;
```

`VL53LX_DEVICEREADOUTMODE_SINGLE_SD`.

### DEVICE_READOUT_MODE_DUAL_SD

```rust
pub const DEVICE_READOUT_MODE_DUAL_SD: i64 = 0x01 << 2;
```

`VL53LX_DEVICEREADOUTMODE_DUAL_SD`.

### GROUPED_PARAMETER_HOLD_ID_MASK

```rust
pub const GROUPED_PARAMETER_HOLD_ID_MASK: i64 = 0x02;
```

`VL53LX_GROUPEDPARAMETERHOLD_ID_MASK`.

### INTERRUPT_CONFIG_NEW_SAMPLE_READY

```rust
pub const INTERRUPT_CONFIG_NEW_SAMPLE_READY: i64 = 0x20;
```

`VL53LX_INTERRUPT_CONFIG_NEW_SAMPLE_READY`.

### CLEAR_RANGE_INT

```rust
pub const CLEAR_RANGE_INT: u8 = 0x01;
```

`VL53LX_CLEAR_RANGE_INT`.

### RANGE_STATUS_MASK

```rust
pub const RANGE_STATUS_MASK: i64 = 0x1F;
```

`VL53LX_RANGE_STATUS__RANGE_STATUS_MASK`.

### DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW

```rust
pub const DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW: i64 = 0x10;
```

`VL53LX_DEVICEINTERRUPTPOLARITY_ACTIVE_LOW`.

### DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS

```rust
pub const DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS: i64 = 0x01;
```

`VL53LX_DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS`.

### DEVICE_DSS_MODE_TARGET_RATE

```rust
pub const DEVICE_DSS_MODE_TARGET_RATE: i64 = 1;
```

`VL53LX_DEVICEDSSMODE__TARGET_RATE`.

### SEQUENCE_VHV_EN

```rust
pub const SEQUENCE_VHV_EN: i64 = 0x01;
```

`VL53LX_SEQUENCE_VHV_EN`.

### SEQUENCE_PHASECAL_EN

```rust
pub const SEQUENCE_PHASECAL_EN: i64 = 0x02;
```

`VL53LX_SEQUENCE_PHASECAL_EN`.

### SEQUENCE_DSS1_EN

```rust
pub const SEQUENCE_DSS1_EN: i64 = 0x08;
```

`VL53LX_SEQUENCE_DSS1_EN`.

### SEQUENCE_DSS2_EN

```rust
pub const SEQUENCE_DSS2_EN: i64 = 0x10;
```

`VL53LX_SEQUENCE_DSS2_EN`.

### SEQUENCE_MM1_EN

```rust
pub const SEQUENCE_MM1_EN: i64 = 0x20;
```

`VL53LX_SEQUENCE_MM1_EN`.

### SEQUENCE_MM2_EN

```rust
pub const SEQUENCE_MM2_EN: i64 = 0x40;
```

`VL53LX_SEQUENCE_MM2_EN`.

### SEQUENCE_RANGE_EN

```rust
pub const SEQUENCE_RANGE_EN: i64 = 0x80;
```

`VL53LX_SEQUENCE_RANGE_EN`.

### SPAD_ARRAY_WIDTH

```rust
pub const SPAD_ARRAY_WIDTH: i64 = 16;
```

SPAD array columns.

### SPAD_ARRAY_HEIGHT

```rust
pub const SPAD_ARRAY_HEIGHT: i64 = 16;
```

SPAD array rows.

### RTN_SPAD_APERTURE_TRANSMISSION

```rust
pub const RTN_SPAD_APERTURE_TRANSMISSION: i64 = 0x0038;
```

Transmission of an aperture SPAD, 1/256.

### RTN_SPAD_UNITY_TRANSMISSION

```rust
pub const RTN_SPAD_UNITY_TRANSMISSION: i64 = 0x0100;
```

Transmission of an ordinary SPAD, 1/256.

### AMBIENT_WINDOW_VCSEL_PERIODS

```rust
pub const AMBIENT_WINDOW_VCSEL_PERIODS: i64 = 256;
```

VCSEL periods of the ambient window.

### RANGING_WINDOW_VCSEL_PERIODS

```rust
pub const RANGING_WINDOW_VCSEL_PERIODS: i64 = 2048;
```

VCSEL periods of the ranging window.

### MACRO_PERIOD_VCSEL_PERIODS

```rust
pub const MACRO_PERIOD_VCSEL_PERIODS: i64 = AMBIENT_WINDOW_VCSEL_PERIODS + RANGING_WINDOW_VCSEL_PERIODS;
```

VCSEL periods of one macro period.

### HISTOGRAM_BUFFER_SIZE

```rust
pub const HISTOGRAM_BUFFER_SIZE: usize = 24;
```

Bins in one frame.

### DISTANCE_MODES

```rust
pub const DISTANCE_MODES: &[&str] = &["short", "medium", "long"];
```

The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that picks them.

### L4_DISTANCE_MODES

```rust
pub const L4_DISTANCE_MODES: &[&str] = &["medium", "long"];
```

The L4CX BareDriver (STSW-IMG029) has no short mode on an L4 die.

### TIMING_GUARD_US

```rust
pub const TIMING_GUARD_US: i64 = 1700;
```

`VL53LX_SetMeasurementTimingBudgetMicroSeconds`: the fixed guard that comes off the budget, µs.

### TIMING_DIVISOR

```rust
pub const TIMING_DIVISOR: i64 = 6;
```

… and the rest splits six ways.

### FDA_MAX_TIMING_BUDGET_US

```rust
pub const FDA_MAX_TIMING_BUDGET_US: i64 = 550000;
```

The budget ceiling, µs.

### L4_FDA_MAX_TIMING_BUDGET_US

```rust
pub const L4_FDA_MAX_TIMING_BUDGET_US: i64 = 200000;
```

The budget ceiling on an L4 die, µs.

### BLOCKS

```rust
pub static BLOCKS: &[RegBlockDef] = &[
    RegBlockDef {
        name: "static_nvm_managed",
        base: 0x0001,
        size: 11,
        writable: true,
        fields: &[
            f("i2c_slave__device_address", 0, 1, false, Some(0x7F)),
            f("ana_config__vhv_ref_sel_vddpix", 1, 1, false, Some(0xF)),
            f("ana_config__vhv_ref_sel_vquench", 2, 1, false, Some(0x7F)),
            f("ana_config__reg_avdd1v2_sel", 3, 1, false, Some(0x3)),
            f("ana_config__fast_osc__trim", 4, 1, false, Some(0x7F)),
            f("osc_measured__fast_osc__frequency", 5, 2, false, None),
            f("vhv_config__timeout_macrop_loop_bound", 7, 1, false, None),
            f("vhv_config__count_thresh", 8, 1, false, None),
            f("vhv_config__offset", 9, 1, false, Some(0x3F)),
            f("vhv_config__init", 10, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "customer_nvm_managed",
        base: 0x000D,
        size: 23,
        writable: true,
        fields: &[
            f("global_config__spad_enables_ref_0", 0, 1, false, None),
            f("global_config__spad_enables_ref_1", 1, 1, false, None),
            f("global_config__spad_enables_ref_2", 2, 1, false, None),
            f("global_config__spad_enables_ref_3", 3, 1, false, None),
            f("global_config__spad_enables_ref_4", 4, 1, false, None),
            f("global_config__spad_enables_ref_5", 5, 1, false, Some(0xF)),
            f("global_config__ref_en_start_select", 6, 1, false, None),
            f("ref_spad_man__num_requested_ref_spads", 7, 1, false, Some(0x3F)),
            f("ref_spad_man__ref_location", 8, 1, false, Some(0x3)),
            f("algo__crosstalk_compensation_plane_offset_kcps", 9, 2, false, None),
            f("algo__crosstalk_compensation_x_plane_gradient_kcps", 11, 2, true, None),
            f("algo__crosstalk_compensation_y_plane_gradient_kcps", 13, 2, true, None),
            f("ref_spad_char__total_rate_target_mcps", 15, 2, false, None),
            f("algo__part_to_part_range_offset_mm", 17, 2, true, Some(0x1FFF)),
            f("mm_config__inner_offset_mm", 19, 2, true, None),
            f("mm_config__outer_offset_mm", 21, 2, true, None),
        ],
    },
    RegBlockDef {
        name: "static_config",
        base: 0x0024,
        size: 32,
        writable: true,
        fields: &[
            f("dss_config__target_total_rate_mcps", 0, 2, false, None),
            f("debug__ctrl", 2, 1, false, Some(0x1)),
            f("test_mode__ctrl", 3, 1, false, Some(0xF)),
            f("clk_gating__ctrl", 4, 1, false, Some(0xF)),
            f("nvm_bist__ctrl", 5, 1, false, Some(0x1F)),
            f("nvm_bist__num_nvm_words", 6, 1, false, Some(0x7F)),
            f("nvm_bist__start_address", 7, 1, false, Some(0x7F)),
            f("host_if__status", 8, 1, false, Some(0x1)),
            f("pad_i2c_hv__config", 9, 1, false, None),
            f("pad_i2c_hv__extsup_config", 10, 1, false, Some(0x1)),
            f("gpio_hv_pad__ctrl", 11, 1, false, Some(0x3)),
            f("gpio_hv_mux__ctrl", 12, 1, false, Some(0x1F)),
            f("gpio__tio_hv_status", 13, 1, false, Some(0x3)),
            f("gpio__fio_hv_status", 14, 1, false, Some(0x3)),
            f("ana_config__spad_sel_pswidth", 15, 1, false, Some(0x7)),
            f("ana_config__vcsel_pulse_width_offset", 16, 1, false, Some(0x1F)),
            f("ana_config__fast_osc__config_ctrl", 17, 1, false, Some(0x1)),
            f("sigma_estimator__effective_pulse_width_ns", 18, 1, false, None),
            f("sigma_estimator__effective_ambient_width_ns", 19, 1, false, None),
            f("sigma_estimator__sigma_ref_mm", 20, 1, false, None),
            f("algo__crosstalk_compensation_valid_height_mm", 21, 1, false, None),
            f("spare_host_config__static_config_spare_0", 22, 1, false, None),
            f("spare_host_config__static_config_spare_1", 23, 1, false, None),
            f("algo__range_ignore_threshold_mcps", 24, 2, false, None),
            f("algo__range_ignore_valid_height_mm", 26, 1, false, None),
            f("algo__range_min_clip", 27, 1, false, None),
            f("algo__consistency_check__tolerance", 28, 1, false, Some(0xF)),
            f("spare_host_config__static_config_spare_2", 29, 1, false, None),
            f("sd_config__reset_stages_msb", 30, 1, false, Some(0xF)),
            f("sd_config__reset_stages_lsb", 31, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "general_config",
        base: 0x0044,
        size: 22,
        writable: true,
        fields: &[
            f("gph_config__stream_count_update_value", 0, 1, false, None),
            f("global_config__stream_divider", 1, 1, false, None),
            f("system__interrupt_config_gpio", 2, 1, false, None),
            f("cal_config__vcsel_start", 3, 1, false, Some(0x7F)),
            f("cal_config__repeat_rate", 4, 2, false, Some(0xFFF)),
            f("global_config__vcsel_width", 6, 1, false, Some(0x7F)),
            f("phasecal_config__timeout_macrop", 7, 1, false, None),
            f("phasecal_config__target", 8, 1, false, None),
            f("phasecal_config__override", 9, 1, false, Some(0x1)),
            f("dss_config__roi_mode_control", 11, 1, false, Some(0x7)),
            f("system__thresh_rate_high", 12, 2, false, None),
            f("system__thresh_rate_low", 14, 2, false, None),
            f("dss_config__manual_effective_spads_select", 16, 2, false, None),
            f("dss_config__manual_block_select", 18, 1, false, None),
            f("dss_config__aperture_attenuation", 19, 1, false, None),
            f("dss_config__max_spads_limit", 20, 1, false, None),
            f("dss_config__min_spads_limit", 21, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "timing_config",
        base: 0x005A,
        size: 23,
        writable: true,
        fields: &[
            f("mm_config__timeout_macrop_a_hi", 0, 1, false, Some(0xF)),
            f("mm_config__timeout_macrop_a_lo", 1, 1, false, None),
            f("mm_config__timeout_macrop_b_hi", 2, 1, false, Some(0xF)),
            f("mm_config__timeout_macrop_b_lo", 3, 1, false, None),
            f("range_config__timeout_macrop_a_hi", 4, 1, false, Some(0xF)),
            f("range_config__timeout_macrop_a_lo", 5, 1, false, None),
            f("range_config__vcsel_period_a", 6, 1, false, Some(0x3F)),
            f("range_config__timeout_macrop_b_hi", 7, 1, false, Some(0xF)),
            f("range_config__timeout_macrop_b_lo", 8, 1, false, None),
            f("range_config__vcsel_period_b", 9, 1, false, Some(0x3F)),
            f("range_config__sigma_thresh", 10, 2, false, None),
            f("range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, None),
            f("range_config__valid_phase_low", 14, 1, false, None),
            f("range_config__valid_phase_high", 15, 1, false, None),
            f("system__intermeasurement_period", 18, 4, false, None),
            f("system__fractional_enable", 22, 1, false, Some(0x1)),
        ],
    },
    RegBlockDef {
        name: "dynamic_config",
        base: 0x0071,
        size: 18,
        writable: true,
        fields: &[
            f("system__grouped_parameter_hold_0", 0, 1, false, Some(0x3)),
            f("system__thresh_high", 1, 2, false, None),
            f("system__thresh_low", 3, 2, false, None),
            f("system__enable_xtalk_per_quadrant", 5, 1, false, Some(0x1)),
            f("system__seed_config", 6, 1, false, Some(0x7)),
            f("sd_config__woi_sd0", 7, 1, false, None),
            f("sd_config__woi_sd1", 8, 1, false, None),
            f("sd_config__initial_phase_sd0", 9, 1, false, Some(0x7F)),
            f("sd_config__initial_phase_sd1", 10, 1, false, Some(0x7F)),
            f("system__grouped_parameter_hold_1", 11, 1, false, Some(0x3)),
            f("sd_config__first_order_select", 12, 1, false, Some(0x3)),
            f("sd_config__quantifier", 13, 1, false, Some(0xF)),
            f("roi_config__user_roi_centre_spad", 14, 1, false, None),
            f("roi_config__user_roi_requested_global_xy_size", 15, 1, false, None),
            f("system__sequence_config", 16, 1, false, None),
            f("system__grouped_parameter_hold", 17, 1, false, Some(0x3)),
        ],
    },
    RegBlockDef {
        name: "system_control",
        base: 0x0083,
        size: 5,
        writable: true,
        fields: &[
            f("power_management__go1_power_force", 0, 1, false, Some(0x1)),
            f("system__stream_count_ctrl", 1, 1, false, Some(0x1)),
            f("firmware__enable", 2, 1, false, Some(0x1)),
            f("system__interrupt_clear", 3, 1, false, Some(0x3)),
            f("system__mode_start", 4, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "system_results",
        base: 0x0088,
        size: 44,
        writable: true,
        fields: &[
            f("result__interrupt_status", 0, 1, false, Some(0x3F)),
            f("result__range_status", 1, 1, false, None),
            f("result__report_status", 2, 1, false, Some(0xF)),
            f("result__stream_count", 3, 1, false, None),
            f("result__dss_actual_effective_spads_sd0", 4, 2, false, None),
            f("result__peak_signal_count_rate_mcps_sd0", 6, 2, false, None),
            f("result__ambient_count_rate_mcps_sd0", 8, 2, false, None),
            f("result__sigma_sd0", 10, 2, false, None),
            f("result__phase_sd0", 12, 2, false, None),
            f("result__final_crosstalk_corrected_range_mm_sd0", 14, 2, false, None),
            f("result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0", 16, 2, false, None),
            f("result__mm_inner_actual_effective_spads_sd0", 18, 2, false, None),
            f("result__mm_outer_actual_effective_spads_sd0", 20, 2, false, None),
            f("result__avg_signal_count_rate_mcps_sd0", 22, 2, false, None),
            f("result__dss_actual_effective_spads_sd1", 24, 2, false, None),
            f("result__peak_signal_count_rate_mcps_sd1", 26, 2, false, None),
            f("result__ambient_count_rate_mcps_sd1", 28, 2, false, None),
            f("result__sigma_sd1", 30, 2, false, None),
            f("result__phase_sd1", 32, 2, false, None),
            f("result__final_crosstalk_corrected_range_mm_sd1", 34, 2, false, None),
            f("result__spare_0_sd1", 36, 2, false, None),
            f("result__spare_1_sd1", 38, 2, false, None),
            f("result__spare_2_sd1", 40, 2, false, None),
            f("result__spare_3_sd1", 42, 1, false, None),
            f("result__thresh_info", 43, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "core_results",
        base: 0x00B4,
        size: 33,
        writable: true,
        fields: &[
            f("result_core__ambient_window_events_sd0", 0, 4, false, None),
            f("result_core__ranging_total_events_sd0", 4, 4, false, None),
            f("result_core__signal_total_events_sd0", 8, 4, true, None),
            f("result_core__total_periods_elapsed_sd0", 12, 4, false, None),
            f("result_core__ambient_window_events_sd1", 16, 4, false, None),
            f("result_core__ranging_total_events_sd1", 20, 4, false, None),
            f("result_core__signal_total_events_sd1", 24, 4, true, None),
            f("result_core__total_periods_elapsed_sd1", 28, 4, false, None),
            f("result_core__spare_0", 32, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "debug_results",
        base: 0x00D6,
        size: 56,
        writable: true,
        fields: &[
            f("phasecal_result__reference_phase", 0, 2, false, None),
            f("phasecal_result__vcsel_start", 2, 1, false, Some(0x7F)),
            f("ref_spad_char_result__num_actual_ref_spads", 3, 1, false, Some(0x3F)),
            f("ref_spad_char_result__ref_location", 4, 1, false, Some(0x3)),
            f("vhv_result__coldboot_status", 5, 1, false, Some(0x1)),
            f("vhv_result__search_result", 6, 1, false, Some(0x3F)),
            f("vhv_result__latest_setting", 7, 1, false, Some(0x3F)),
            f("result__osc_calibrate_val", 8, 2, false, Some(0x3FF)),
            f("ana_config__powerdown_go1", 10, 1, false, Some(0x3)),
            f("ana_config__ref_bg_ctrl", 11, 1, false, Some(0x3)),
            f("ana_config__regdvdd1v2_ctrl", 12, 1, false, Some(0xF)),
            f("ana_config__osc_slow_ctrl", 13, 1, false, Some(0x7)),
            f("test_mode__status", 14, 1, false, Some(0x1)),
            f("firmware__system_status", 15, 1, false, Some(0x3)),
            f("firmware__mode_status", 16, 1, false, None),
            f("firmware__secondary_mode_status", 17, 1, false, None),
            f("firmware__cal_repeat_rate_counter", 18, 2, false, Some(0xFFF)),
            f("gph__system__thresh_high", 22, 2, false, None),
            f("gph__system__thresh_low", 24, 2, false, None),
            f("gph__system__enable_xtalk_per_quadrant", 26, 1, false, Some(0x1)),
            f("gph__spare_0", 27, 1, false, Some(0x7)),
            f("gph__sd_config__woi_sd0", 28, 1, false, None),
            f("gph__sd_config__woi_sd1", 29, 1, false, None),
            f("gph__sd_config__initial_phase_sd0", 30, 1, false, Some(0x7F)),
            f("gph__sd_config__initial_phase_sd1", 31, 1, false, Some(0x7F)),
            f("gph__sd_config__first_order_select", 32, 1, false, Some(0x3)),
            f("gph__sd_config__quantifier", 33, 1, false, Some(0xF)),
            f("gph__roi_config__user_roi_centre_spad", 34, 1, false, None),
            f("gph__roi_config__user_roi_requested_global_xy_size", 35, 1, false, None),
            f("gph__system__sequence_config", 36, 1, false, None),
            f("gph__gph_id", 37, 1, false, Some(0x1)),
            f("system__interrupt_set", 38, 1, false, Some(0x3)),
            f("interrupt_manager__enables", 39, 1, false, Some(0x1F)),
            f("interrupt_manager__clear", 40, 1, false, Some(0x1F)),
            f("interrupt_manager__status", 41, 1, false, Some(0x1F)),
            f("mcu_to_host_bank__wr_access_en", 42, 1, false, Some(0x1)),
            f("power_management__go1_reset_status", 43, 1, false, Some(0x1)),
            f("pad_startup_mode__value_ro", 44, 1, false, Some(0x3)),
            f("pad_startup_mode__value_ctrl", 45, 1, false, Some(0x3F)),
            f("pll_period_us", 46, 4, false, Some(0x3FFFF)),
            f("interrupt_scheduler__data_out", 50, 4, false, None),
            f("nvm_bist__complete", 54, 1, false, Some(0x1)),
            f("nvm_bist__status", 55, 1, false, Some(0x1)),
        ],
    },
    RegBlockDef {
        name: "nvm_copy_data",
        base: 0x010F,
        size: 49,
        writable: true,
        fields: &[
            f("identification__model_id", 0, 1, false, None),
            f("identification__module_type", 1, 1, false, None),
            f("identification__revision_id", 2, 1, false, None),
            f("identification__module_id", 3, 2, false, None),
            f("ana_config__fast_osc__trim_max", 5, 1, false, Some(0x7F)),
            f("ana_config__fast_osc__freq_set", 6, 1, false, Some(0x7)),
            f("ana_config__vcsel_trim", 7, 1, false, Some(0x7)),
            f("ana_config__vcsel_selion", 8, 1, false, Some(0x3F)),
            f("ana_config__vcsel_selion_max", 9, 1, false, Some(0x3F)),
            f("protected_laser_safety__lock_bit", 10, 1, false, Some(0x1)),
            f("laser_safety__key", 11, 1, false, Some(0x7F)),
            f("laser_safety__key_ro", 12, 1, false, Some(0x1)),
            f("laser_safety__clip", 13, 1, false, Some(0x3F)),
            f("laser_safety__mult", 14, 1, false, Some(0x3F)),
            f("global_config__spad_enables_rtn_0", 15, 1, false, None),
            f("global_config__spad_enables_rtn_1", 16, 1, false, None),
            f("global_config__spad_enables_rtn_2", 17, 1, false, None),
            f("global_config__spad_enables_rtn_3", 18, 1, false, None),
            f("global_config__spad_enables_rtn_4", 19, 1, false, None),
            f("global_config__spad_enables_rtn_5", 20, 1, false, None),
            f("global_config__spad_enables_rtn_6", 21, 1, false, None),
            f("global_config__spad_enables_rtn_7", 22, 1, false, None),
            f("global_config__spad_enables_rtn_8", 23, 1, false, None),
            f("global_config__spad_enables_rtn_9", 24, 1, false, None),
            f("global_config__spad_enables_rtn_10", 25, 1, false, None),
            f("global_config__spad_enables_rtn_11", 26, 1, false, None),
            f("global_config__spad_enables_rtn_12", 27, 1, false, None),
            f("global_config__spad_enables_rtn_13", 28, 1, false, None),
            f("global_config__spad_enables_rtn_14", 29, 1, false, None),
            f("global_config__spad_enables_rtn_15", 30, 1, false, None),
            f("global_config__spad_enables_rtn_16", 31, 1, false, None),
            f("global_config__spad_enables_rtn_17", 32, 1, false, None),
            f("global_config__spad_enables_rtn_18", 33, 1, false, None),
            f("global_config__spad_enables_rtn_19", 34, 1, false, None),
            f("global_config__spad_enables_rtn_20", 35, 1, false, None),
            f("global_config__spad_enables_rtn_21", 36, 1, false, None),
            f("global_config__spad_enables_rtn_22", 37, 1, false, None),
            f("global_config__spad_enables_rtn_23", 38, 1, false, None),
            f("global_config__spad_enables_rtn_24", 39, 1, false, None),
            f("global_config__spad_enables_rtn_25", 40, 1, false, None),
            f("global_config__spad_enables_rtn_26", 41, 1, false, None),
            f("global_config__spad_enables_rtn_27", 42, 1, false, None),
            f("global_config__spad_enables_rtn_28", 43, 1, false, None),
            f("global_config__spad_enables_rtn_29", 44, 1, false, None),
            f("global_config__spad_enables_rtn_30", 45, 1, false, None),
            f("global_config__spad_enables_rtn_31", 46, 1, false, None),
            f("roi_config__mode_roi_centre_spad", 47, 1, false, None),
            f("roi_config__mode_roi_xy_size", 48, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "patch_debug",
        base: 0x0F20,
        size: 2,
        writable: true,
        fields: &[
            f("result__debug_status", 0, 1, false, None),
            f("result__debug_stage", 1, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "gph_general_config",
        base: 0x0F24,
        size: 5,
        writable: true,
        fields: &[
            f("gph__system__thresh_rate_high", 0, 2, false, None),
            f("gph__system__thresh_rate_low", 2, 2, false, None),
            f("gph__system__interrupt_config_gpio", 4, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "gph_static_config",
        base: 0x0F2F,
        size: 6,
        writable: true,
        fields: &[
            f("gph__dss_config__roi_mode_control", 0, 1, false, Some(0x7)),
            f("gph__dss_config__manual_effective_spads_select", 1, 2, false, None),
            f("gph__dss_config__manual_block_select", 3, 1, false, None),
            f("gph__dss_config__max_spads_limit", 4, 1, false, None),
            f("gph__dss_config__min_spads_limit", 5, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "gph_timing_config",
        base: 0x0F36,
        size: 16,
        writable: true,
        fields: &[
            f("gph__mm_config__timeout_macrop_a_hi", 0, 1, false, Some(0xF)),
            f("gph__mm_config__timeout_macrop_a_lo", 1, 1, false, None),
            f("gph__mm_config__timeout_macrop_b_hi", 2, 1, false, Some(0xF)),
            f("gph__mm_config__timeout_macrop_b_lo", 3, 1, false, None),
            f("gph__range_config__timeout_macrop_a_hi", 4, 1, false, Some(0xF)),
            f("gph__range_config__timeout_macrop_a_lo", 5, 1, false, None),
            f("gph__range_config__vcsel_period_a", 6, 1, false, Some(0x3F)),
            f("gph__range_config__vcsel_period_b", 7, 1, false, Some(0x3F)),
            f("gph__range_config__timeout_macrop_b_hi", 8, 1, false, Some(0xF)),
            f("gph__range_config__timeout_macrop_b_lo", 9, 1, false, None),
            f("gph__range_config__sigma_thresh", 10, 2, false, None),
            f("gph__range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, None),
            f("gph__range_config__valid_phase_low", 14, 1, false, None),
            f("gph__range_config__valid_phase_high", 15, 1, false, None),
        ],
    },
    RegBlockDef {
        name: "fw_internal",
        base: 0x0F46,
        size: 2,
        writable: true,
        fields: &[
            f("firmware__internal_stream_count_div", 0, 1, false, None),
            f("firmware__internal_stream_counter_val", 1, 1, false, None),
        ],
    },
];
```

The register blocks of the device image (one `VL53LX_*_t` struct of the C driver each), in
address order: base, size, writable and the fields with offset, width, signedness and the mask
the C codec applies (`None`: none).

### TUNING

```rust
pub static TUNING: &[(&str, i64)] = &[
    ("tp_tuning_parm_version", 30),
    ("tp_tuning_parm_key_table_version", 14),
    ("tp_tuning_parm_lld_version", 12180),
    ("tp_init_phase_rtn_lite_long", 14),
    ("tp_init_phase_rtn_lite_med", 10),
    ("tp_init_phase_rtn_lite_short", 6),
    ("tp_init_phase_ref_lite_long", 14),
    ("tp_init_phase_ref_lite_med", 10),
    ("tp_init_phase_ref_lite_short", 6),
    ("tp_init_phase_rtn_hist_long", 9),
    ("tp_init_phase_rtn_hist_med", 5),
    ("tp_init_phase_rtn_hist_short", 3),
    ("tp_init_phase_ref_hist_long", 6),
    ("tp_init_phase_ref_hist_med", 6),
    ("tp_init_phase_ref_hist_short", 6),
    ("tp_consistency_lite_phase_tolerance", 2),
    ("tp_phasecal_target", 33),
    ("tp_cal_repeat_rate", 0),
    ("tp_lite_min_clip", 0),
    ("tp_lite_long_sigma_thresh_mm", 60),
    ("tp_lite_med_sigma_thresh_mm", 60),
    ("tp_lite_short_sigma_thresh_mm", 60),
    ("tp_lite_long_min_count_rate_rtn_mcps", 128),
    ("tp_lite_med_min_count_rate_rtn_mcps", 128),
    ("tp_lite_short_min_count_rate_rtn_mcps", 128),
    ("tp_lite_sigma_est_pulse_width_ns", 8),
    ("tp_lite_sigma_est_amb_width_ns", 16),
    ("tp_lite_sigma_ref_mm", 1),
    ("tp_lite_seed_cfg", 2),
    ("tp_timed_seed_cfg", 1),
    ("tp_lite_quantifier", 2),
    ("tp_lite_first_order_select", 0),
    ("tp_uwr_enable", 1),
    ("tp_uwr_med_z_1_min", 2000),
    ("tp_uwr_med_z_1_max", 2750),
    ("tp_uwr_med_z_2_min", 250),
    ("tp_uwr_med_z_2_max", 1000),
    ("tp_uwr_med_z_3_min", 1250),
    ("tp_uwr_med_z_3_max", 1750),
    ("tp_uwr_med_z_4_min", 1250),
    ("tp_uwr_med_z_4_max", 1750),
    ("tp_uwr_med_z_5_min", -200),
    ("tp_uwr_med_z_5_max", 200),
    ("tp_uwr_med_corr_z_1_rangea", 2300),
    ("tp_uwr_med_corr_z_1_rangeb", 0),
    ("tp_uwr_med_corr_z_2_rangea", 2300),
    ("tp_uwr_med_corr_z_2_rangeb", 3050),
    ("tp_uwr_med_corr_z_3_rangea", 4600),
    ("tp_uwr_med_corr_z_3_rangeb", 3050),
    ("tp_uwr_med_corr_z_4_rangea", 4600),
    ("tp_uwr_med_corr_z_4_rangeb", 6200),
    ("tp_uwr_med_corr_z_5_rangea", 0),
    ("tp_uwr_med_corr_z_5_rangeb", 0),
    ("tp_uwr_lng_z_1_min", 250),
    ("tp_uwr_lng_z_1_max", 1250),
    ("tp_uwr_lng_z_2_min", 3250),
    ("tp_uwr_lng_z_2_max", 4500),
    ("tp_uwr_lng_z_3_min", -200),
    ("tp_uwr_lng_z_3_max", 200),
    ("tp_uwr_lng_z_4_min", 0),
    ("tp_uwr_lng_z_4_max", 0),
    ("tp_uwr_lng_z_5_min", 0),
    ("tp_uwr_lng_z_5_max", 0),
    ("tp_uwr_lng_corr_z_1_rangea", 3850),
    ("tp_uwr_lng_corr_z_1_rangeb", 4600),
    ("tp_uwr_lng_corr_z_2_rangea", 3850),
    ("tp_uwr_lng_corr_z_2_rangeb", 0),
    ("tp_uwr_lng_corr_z_3_rangea", 0),
    ("tp_uwr_lng_corr_z_3_rangeb", 0),
    ("tp_uwr_lng_corr_z_4_rangea", 0),
    ("tp_uwr_lng_corr_z_4_rangeb", 0),
    ("tp_uwr_lng_corr_z_5_rangea", 0),
    ("tp_uwr_lng_corr_z_5_rangeb", 0),
    ("tp_dss_target_lite_mcps", 2560),
    ("tp_dss_target_histo_mcps", 5120),
    ("tp_dss_target_histo_mz_mcps", 5120),
    ("tp_dss_target_timed_mcps", 2560),
    ("tp_phasecal_timeout_lite_us", 1000),
    ("tp_phasecal_timeout_hist_long_us", 15000),
    ("tp_phasecal_timeout_hist_med_us", 9000),
    ("tp_phasecal_timeout_hist_short_us", 6000),
    ("tp_phasecal_timeout_mz_long_us", 15000),
    ("tp_phasecal_timeout_mz_med_us", 9000),
    ("tp_phasecal_timeout_mz_short_us", 6000),
    ("tp_phasecal_timeout_timed_us", 1000),
    ("tp_mm_timeout_lite_us", 2000),
    ("tp_mm_timeout_histo_us", 2000),
    ("tp_mm_timeout_mz_us", 2000),
    ("tp_mm_timeout_timed_us", 2000),
    ("tp_range_timeout_lite_us", 63000),
    ("tp_range_timeout_histo_us", 2500),
    ("tp_range_timeout_mz_us", 2500),
    ("tp_range_timeout_timed_us", 13000),
    ("tp_mm_timeout_lpa_us", 1),
    ("tp_range_timeout_lpa_us", 8000),
    ("tp_dss_target_very_short_mcps", 10240),
    ("tp_phasecal_patch_power", 0),
    ("tp_hist_merge", 1),
    ("tp_reset_merge_threshold", 15000),
    ("tp_hist_merge_max_size", 6),
];
```

`VL53LX_tuning_parm_storage_t` after initialisation, keyed by the C field name.

### DEFAULTS

```rust
pub static DEFAULTS: &[(&str, i64)] = &[
    ("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT", 0),
    ("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT", 2048),
    ("TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT", 0),
    ("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT", 8),
    ("TUNINGPARM_CONSISTENCY_LITE_PHASE_TOLERANCE_DEFAULT", 2),
    ("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_0_DEFAULT", 15),
    ("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_1_DEFAULT", 52),
    ("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_2_DEFAULT", 200),
    ("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_3_DEFAULT", 364),
    ("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_4_DEFAULT", 400),
    ("TUNINGPARM_DMAX_CFG_SIGNAL_THRESH_SIGMA_DEFAULT", 32),
    ("TUNINGPARM_DYNXTALK_AVERAGED_XTALK_DELTA_DEFAULT", 308),
    ("TUNINGPARM_DYNXTALK_CLIP_LIMIT_DEFAULT", 10240),
    ("TUNINGPARM_DYNXTALK_MAX_SMUDGE_FACTOR_DEFAULT", 2000),
    ("TUNINGPARM_DYNXTALK_NODETECT_AMB_THRESHOLD_KCPS_DEFAULT", 57671680),
    ("TUNINGPARM_DYNXTALK_NODETECT_MIN_RANGE_MM_DEFAULT", 900),
    ("TUNINGPARM_DYNXTALK_NODETECT_SAMPLE_LIMIT_DEFAULT", 40),
    ("TUNINGPARM_DYNXTALK_NODETECT_XTALK_OFFSET_KCPS_DEFAULT", 410),
    ("TUNINGPARM_DYNXTALK_NOISE_MARGIN_DEFAULT", 100),
    ("TUNINGPARM_DYNXTALK_SAMPLE_LIMIT_DEFAULT", 200),
    ("TUNINGPARM_DYNXTALK_SCALER_CALC_METHOD_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_SINGLE_XTALK_DELTA_DEFAULT", 2048),
    ("TUNINGPARM_DYNXTALK_SMUDGE_COR_SINGLE_APPLY_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_SMUDGE_MARGIN_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_USER_SCALER_SET_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_XGRADIENT_SCALER_DEFAULT", 256),
    ("TUNINGPARM_DYNXTALK_XTALK_AMB_THRESHOLD_DEFAULT", 128),
    ("TUNINGPARM_DYNXTALK_XTALK_OFFSET_LIMIT_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_XTALK_OFFSET_LIMIT_HI_DEFAULT", 0),
    ("TUNINGPARM_DYNXTALK_YGRADIENT_SCALER_DEFAULT", 256),
    ("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT", 4),
    ("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT", 1),
    ("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT", 4157),
    ("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT", 80),
    ("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT", 100),
    ("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT", 1),
    ("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT", 2),
    ("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT", 1987),
    ("TUNINGPARM_HIST_MERGE_DEFAULT", 1),
    ("TUNINGPARM_HIST_MERGE_MAX_SIZE_DEFAULT", 6),
    ("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT", 16),
    ("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT", 50),
    ("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT", 1),
    ("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT", 180),
    ("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT", 100),
    ("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT", 1),
    ("TUNINGPARM_HIST_XTALK_MARGIN_KCPS_DEFAULT", 0),
    ("TUNINGPARM_INITIAL_PHASE_REF_HISTO_LONG_RANGE_DEFAULT", 6),
    ("TUNINGPARM_INITIAL_PHASE_REF_HISTO_MED_RANGE_DEFAULT", 6),
    ("TUNINGPARM_INITIAL_PHASE_REF_HISTO_SHORT_RANGE_DEFAULT", 6),
    ("TUNINGPARM_INITIAL_PHASE_REF_LITE_LONG_RANGE_DEFAULT", 14),
    ("TUNINGPARM_INITIAL_PHASE_REF_LITE_MED_RANGE_DEFAULT", 10),
    ("TUNINGPARM_INITIAL_PHASE_REF_LITE_SHORT_RANGE_DEFAULT", 6),
    ("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_LONG_RANGE_DEFAULT", 9),
    ("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_MED_RANGE_DEFAULT", 5),
    ("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_SHORT_RANGE_DEFAULT", 3),
    ("TUNINGPARM_INITIAL_PHASE_RTN_LITE_LONG_RANGE_DEFAULT", 14),
    ("TUNINGPARM_INITIAL_PHASE_RTN_LITE_MED_RANGE_DEFAULT", 10),
    ("TUNINGPARM_INITIAL_PHASE_RTN_LITE_SHORT_RANGE_DEFAULT", 6),
    ("TUNINGPARM_KEY_TABLE_VERSION_DEFAULT", 14),
    ("TUNINGPARM_LITE_CAL_REPEAT_RATE_DEFAULT", 0),
    ("TUNINGPARM_LITE_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 2560),
    ("TUNINGPARM_LITE_FIRST_ORDER_SELECT_DEFAULT", 0),
    ("TUNINGPARM_LITE_LONG_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128),
    ("TUNINGPARM_LITE_LONG_SIGMA_THRESH_MM_DEFAULT", 60),
    ("TUNINGPARM_LITE_MED_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128),
    ("TUNINGPARM_LITE_MED_SIGMA_THRESH_MM_DEFAULT", 60),
    ("TUNINGPARM_LITE_MIN_CLIP_MM_DEFAULT", 0),
    ("TUNINGPARM_LITE_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_LITE_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 1000),
    ("TUNINGPARM_LITE_QUANTIFIER_DEFAULT", 2),
    ("TUNINGPARM_LITE_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 63000),
    ("TUNINGPARM_LITE_RANGING_GAIN_FACTOR_DEFAULT", 2011),
    ("TUNINGPARM_LITE_RIT_MULT_DEFAULT", 64),
    ("TUNINGPARM_LITE_SEED_CONFIG_DEFAULT", 2),
    ("TUNINGPARM_LITE_SHORT_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128),
    ("TUNINGPARM_LITE_SHORT_SIGMA_THRESH_MM_DEFAULT", 60),
    ("TUNINGPARM_LITE_SIGMA_EST_AMB_WIDTH_NS_DEFAULT", 16),
    ("TUNINGPARM_LITE_SIGMA_EST_PULSE_WIDTH_DEFAULT", 8),
    ("TUNINGPARM_LITE_SIGMA_REF_MM_DEFAULT", 1),
    ("TUNINGPARM_LITE_XTALK_MARGIN_KCPS_DEFAULT", 0),
    ("TUNINGPARM_LLD_VERSION_DEFAULT", 12180),
    ("TUNINGPARM_LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US_DEFAULT", 1),
    ("TUNINGPARM_LOWPOWERAUTO_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 8000),
    ("TUNINGPARM_LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT", 3),
    ("TUNINGPARM_MZ_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 5120),
    ("TUNINGPARM_MZ_LONG_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 15000),
    ("TUNINGPARM_MZ_MED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 9000),
    ("TUNINGPARM_MZ_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_MZ_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 2500),
    ("TUNINGPARM_MZ_SHORT_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 6000),
    ("TUNINGPARM_OFFSET_CAL_DSS_RATE_MCPS_DEFAULT", 2560),
    ("TUNINGPARM_OFFSET_CAL_MM1_SAMPLES_DEFAULT", 40),
    ("TUNINGPARM_OFFSET_CAL_MM2_SAMPLES_DEFAULT", 9),
    ("TUNINGPARM_OFFSET_CAL_MM_TIMEOUT_US_DEFAULT", 13000),
    ("TUNINGPARM_OFFSET_CAL_PHASECAL_TIMEOUT_US_DEFAULT", 15000),
    ("TUNINGPARM_OFFSET_CAL_PRE_SAMPLES_DEFAULT", 8),
    ("TUNINGPARM_OFFSET_CAL_RANGE_TIMEOUT_US_DEFAULT", 13000),
    ("TUNINGPARM_PHASECAL_PATCH_POWER_DEFAULT", 0),
    ("TUNINGPARM_PHASECAL_TARGET_DEFAULT", 33),
    ("TUNINGPARM_RANGING_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 5120),
    ("TUNINGPARM_RANGING_LONG_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 15000),
    ("TUNINGPARM_RANGING_MED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 9000),
    ("TUNINGPARM_RANGING_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_RANGING_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 2500),
    ("TUNINGPARM_RANGING_SHORT_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 6000),
    ("TUNINGPARM_REFSPADCHAR_DEVICE_TEST_MODE_DEFAULT", 8),
    ("TUNINGPARM_REFSPADCHAR_MAX_COUNTRATE_LIMIT_MCPS_DEFAULT", 5120),
    ("TUNINGPARM_REFSPADCHAR_MIN_COUNTRATE_LIMIT_MCPS_DEFAULT", 1280),
    ("TUNINGPARM_REFSPADCHAR_PHASECAL_TIMEOUT_US_DEFAULT", 1000),
    ("TUNINGPARM_REFSPADCHAR_TARGET_COUNT_RATE_MCPS_DEFAULT", 2560),
    ("TUNINGPARM_REFSPADCHAR_VCSEL_PERIOD_DEFAULT", 11),
    ("TUNINGPARM_RESET_MERGE_THRESHOLD_DEFAULT", 15000),
    ("TUNINGPARM_SPADMAP_RATE_LIMIT_MCPS_DEFAULT", 12),
    ("TUNINGPARM_SPADMAP_VCSEL_PERIOD_DEFAULT", 18),
    ("TUNINGPARM_SPADMAP_VCSEL_START_DEFAULT", 15),
    ("TUNINGPARM_TIMED_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 2560),
    ("TUNINGPARM_TIMED_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 1000),
    ("TUNINGPARM_TIMED_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 13000),
    ("TUNINGPARM_TIMED_SEED_CONFIG_DEFAULT", 1),
    ("TUNINGPARM_UWR_ENABLE_DEFAULT", 1),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_1_RANGEA_DEFAULT", 3850),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_1_RANGEB_DEFAULT", 4600),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_2_RANGEA_DEFAULT", 3850),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_2_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_3_RANGEA_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_3_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_4_RANGEA_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_4_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_5_RANGEA_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_5_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_ZONE_1_MAX_DEFAULT", 1250),
    ("TUNINGPARM_UWR_LONG_ZONE_1_MIN_DEFAULT", 250),
    ("TUNINGPARM_UWR_LONG_ZONE_2_MAX_DEFAULT", 4500),
    ("TUNINGPARM_UWR_LONG_ZONE_2_MIN_DEFAULT", 3250),
    ("TUNINGPARM_UWR_LONG_ZONE_3_MAX_DEFAULT", 200),
    ("TUNINGPARM_UWR_LONG_ZONE_3_MIN_DEFAULT", -200),
    ("TUNINGPARM_UWR_LONG_ZONE_4_MAX_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_ZONE_4_MIN_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_ZONE_5_MAX_DEFAULT", 0),
    ("TUNINGPARM_UWR_LONG_ZONE_5_MIN_DEFAULT", 0),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_1_RANGEA_DEFAULT", 2300),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_1_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_2_RANGEA_DEFAULT", 2300),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_2_RANGEB_DEFAULT", 3050),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_3_RANGEA_DEFAULT", 4600),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_3_RANGEB_DEFAULT", 3050),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_4_RANGEA_DEFAULT", 4600),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_4_RANGEB_DEFAULT", 6200),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_5_RANGEA_DEFAULT", 0),
    ("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_5_RANGEB_DEFAULT", 0),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_1_MAX_DEFAULT", 2750),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_1_MIN_DEFAULT", 2000),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_2_MAX_DEFAULT", 1000),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_2_MIN_DEFAULT", 250),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_3_MAX_DEFAULT", 1750),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_3_MIN_DEFAULT", 1250),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_4_MAX_DEFAULT", 1750),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_4_MIN_DEFAULT", 1250),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_5_MAX_DEFAULT", 200),
    ("TUNINGPARM_UWR_MEDIUM_ZONE_5_MIN_DEFAULT", -200),
    ("TUNINGPARM_VERSION_DEFAULT", 30),
    ("TUNINGPARM_VERY_SHORT_DSS_RATE_MCPS_DEFAULT", 10240),
    ("TUNINGPARM_VHV_LOOPBOUND_DEFAULT", 129),
    ("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT", 80),
    ("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT", 140),
    ("TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT", 50),
    ("TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT", 400),
    ("TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT", 50),
    ("TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT", -50),
    ("TUNINGPARM_XTALK_EXTRACT_BIN_TIMEOUT_US_DEFAULT", 10000),
    ("TUNINGPARM_XTALK_EXTRACT_DSS_RATE_MCPS_DEFAULT", 5120),
    ("TUNINGPARM_XTALK_EXTRACT_DSS_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_XTALK_EXTRACT_MAX_FILTER_THRESH_MM_DEFAULT", 70),
    ("TUNINGPARM_XTALK_EXTRACT_MAX_VALID_RATE_KCPS_DEFAULT", 640),
    ("TUNINGPARM_XTALK_EXTRACT_MIN_FILTER_THRESH_MM_DEFAULT", -70),
    ("TUNINGPARM_XTALK_EXTRACT_NUM_OF_SAMPLES_DEFAULT", 7),
    ("TUNINGPARM_XTALK_EXTRACT_PHASECAL_TIMEOUT_US_DEFAULT", 15000),
    ("TUNINGPARM_XTALK_EXTRACT_SIGMA_THRESHOLD_MM_DEFAULT", 140),
    ("TUNINGPARM_ZONE_CAL_DSS_RATE_MCPS_DEFAULT", 5120),
    ("TUNINGPARM_ZONE_CAL_DSS_TIMEOUT_US_DEFAULT", 2000),
    ("TUNINGPARM_ZONE_CAL_PHASECAL_NUM_SAMPLES_DEFAULT", 16),
    ("TUNINGPARM_ZONE_CAL_PHASECAL_TIMEOUT_US_DEFAULT", 15000),
    ("TUNINGPARM_ZONE_CAL_RANGE_TIMEOUT_US_DEFAULT", 1000),
    ("TUNINGPARM_ZONE_CAL_ZONE_NUM_SAMPLES_DEFAULT", 8),
];
```

The raw tuning header, for the parameters the post-processing reads without a `tp_` field.

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

### Vl53lxPlatform

```rust
pub trait Vl53lxPlatform: Send + Sync {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    /// The bridge's bus timing (not part of the C platform layer).
    fn set_i2c_speed(&self, khz: u16) -> Result<()>;

    /// How wide a register address the bridge puts on the wire; sticky.
    fn set_addr_width(&self, width: u8) -> Result<()>;

    /// Reset through XSHUT, for a sensor that no longer acknowledges writes.
    fn xshut_reset(&self) -> Result<()>;

    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(Duration::from_millis(ms as u64));
    }

    fn read_byte(&self, addr: u16) -> Result<u8> {
        Ok(self.read_multi(addr, 1)?[0])
    }

    fn read_word(&self, addr: u16) -> Result<u16> {
        let b = self.read_multi(addr, 2)?;
        Ok(u16::from_be_bytes([b[0], b[1]]))
    }

    fn read_dword(&self, addr: u16) -> Result<u32> {
        let b = self.read_multi(addr, 4)?;
        Ok(u32::from_be_bytes([b[0], b[1], b[2], b[3]]))
    }

    fn write_byte(&self, addr: u16, value: u8) -> Result<()> {
        self.write_multi(addr, &[value])
    }

    fn write_word(&self, addr: u16, value: u16) -> Result<()> {
        self.write_multi(addr, &value.to_be_bytes())
    }

    fn write_dword(&self, addr: u16, value: u32) -> Result<()> {
        self.write_multi(addr, &value.to_be_bytes())
    }
}
```

ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus
speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers
assume. The device class implements it over the board; a test can script it.

### Vl53lxTarget

```rust
pub struct Vl53lxTarget {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub min_range_mm: i32,
    pub max_range_mm: i32,
}
```

One return of a multi-target frame. `min_range_mm` / `max_range_mm` are the edges of the target's
own pulse, which only the histogram driver knows.

### ExtraValue

```rust
pub enum ExtraValue {
    Int(i64),
    Float(f64),
}
```

A value of [`Vl53lxRange::extra`] / [`Vl53lxDriver::driver_info`]: driver-specific numbers.

### Vl53lxRange

```rust
pub struct Vl53lxRange {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub spads: f64,
    pub targets: Vec<Vl53lxTarget>,
    pub extra: BTreeMap<String, ExtraValue>,
}
```

One range result as a driver reports it, the same shape for every product of the family. On the
histogram driver `targets` holds every return and the top level repeats the first; the light
drivers leave it empty. `extra` is driver-specific (keys as the reference SDK spells them) — show
it, don't branch on it.

#### Vl53lxRange::valid

```rust
pub fn valid(&self) -> bool
```

### Thresholds

```rust
pub struct Thresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}
```

The distance-window interrupt: `window` is one of the `WINDOW_*` constants of the die.

### BinData

```rust
pub type BinData = Box<dyn std::any::Any + Send>;
```

A raw histogram frame of the histogram driver (its concrete type is the driver's).

### Vl53lxDriver

```rust
pub trait Vl53lxDriver: Send {
    /// The product this driver was bound for (one driver serves several).
    fn product(&self) -> &str;

    /// The driver's own name, for messages ("Vl53l1UldDriver", …).
    fn driver_name(&self) -> &'static str;

    /// Bytes of register address on the wire.
    fn addr_width(&self) -> u8 {
        2
    }

    /// The interrupt-release writes handed to START_STREAM, `(addr, value)` each.
    fn clear_steps(&self) -> Vec<(u16, u8)> {
        Vec::new()
    }

    /// The bus ceiling; the driver runs the bus there itself.
    fn max_khz(&self) -> u16 {
        400
    }

    fn supports(&self) -> &'static [&'static str] {
        &[]
    }

    /// The named ranging modes, the first being what init leaves; empty when there are none.
    fn modes(&self) -> Vec<&'static str> {
        Vec::new()
    }

    /// The (min, max) budget a UI should offer, ms; see [`Vl53lxDriver::budget_choices`].
    fn budget_ms(&self) -> (u32, u32) {
        (10, 200)
    }

    /// The frame is a raw histogram (the histogram driver).
    fn histogram(&self) -> bool {
        false
    }

    fn model_id(&mut self) -> Result<u16>;

    /// Reset and configure the sensor; leaves the bus at [`Vl53lxDriver::max_khz`]. Safe to re-run.
    fn sensor_init(&mut self) -> Result<()>;

    fn start_ranging(&mut self) -> Result<()>;

    fn stop_ranging(&mut self) -> Result<()>;

    fn check_for_data_ready(&mut self) -> Result<bool>;

    /// Poll [`Vl53lxDriver::check_for_data_ready`] every 1 ms up to `timeout`.
    fn wait_data_ready(&mut self, timeout: Duration) -> Result<()> {
        let deadline = Instant::now() + timeout;
        while !self.check_for_data_ready()? {
            if Instant::now() > deadline {
                return Err(DepzError::Sensor("timeout waiting for data ready".into()));
            }
            std::thread::sleep(Duration::from_millis(1));
        }
        Ok(())
    }

    fn clear_interrupt(&mut self) -> Result<()>;

    /// (addr, len) — the register block the bridge streams on each INT.
    fn stream_block(&self) -> (u16, u16);

    /// Streamed block bytes → a range.
    fn decode(&mut self, raw: &[u8]) -> Result<Vl53lxRange>;

    /// One polled result: the stream block read and decoded.
    fn read_measurement(&mut self) -> Result<Vl53lxRange>;

    /// Budget of one measurement and the period between two; a period of 0 means continuous.
    fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()>;

    /// (timing budget ms, inter-measurement ms).
    fn get_range_timing(&mut self) -> Result<(u32, u32)>;

    /// The only budgets accepted right now (ascending), or empty when any integer of
    /// [`Vl53lxDriver::budget_ms`] will do.
    fn budget_choices(&self) -> Vec<u32> {
        Vec::new()
    }

    fn set_mode(&mut self, _name: &str) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "mode"))
    }

    fn get_mode(&mut self) -> Result<String> {
        Err(unsupported(self.product(), self.driver_name(), "mode"))
    }

    /// How far the loaded configuration can measure, mm, or `None` when not characterised.
    fn reach_mm(&mut self) -> Result<Option<i32>> {
        Ok(None)
    }

    /// Numbers this driver shows that the contract has no field for.
    fn driver_info(&mut self) -> Result<BTreeMap<String, ExtraValue>> {
        Ok(BTreeMap::new())
    }

    // ── optional groups ──────────────────────────────────────────────────────

    fn get_offset(&mut self) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "offset"))
    }

    fn set_offset(&mut self, _offset_mm: i32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "offset"))
    }

    fn get_xtalk(&mut self) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "xtalk"))
    }

    fn set_xtalk(&mut self, _xtalk_kcps: i32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "xtalk"))
    }

    /// `nb_samples` `None`: the driver's default.
    fn calibrate_offset(&mut self, _target_dist_mm: i32, _nb_samples: Option<u32>) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "calib_offset"))
    }

    fn calibrate_xtalk(&mut self, _target_dist_mm: i32, _nb_samples: Option<u32>) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "calib_xtalk"))
    }

    fn get_detection_thresholds(&mut self) -> Result<Thresholds> {
        Err(unsupported(self.product(), self.driver_name(), "thresholds"))
    }

    fn set_detection_thresholds(&mut self, _distance_low_mm: u16, _distance_high_mm: u16, _window: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "thresholds"))
    }

    fn get_signal_threshold(&mut self) -> Result<u32> {
        Err(unsupported(self.product(), self.driver_name(), "signal_thresh"))
    }

    fn set_signal_threshold(&mut self, _signal_kcps: u32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "signal_thresh"))
    }

    fn get_sigma_threshold(&mut self) -> Result<u32> {
        Err(unsupported(self.product(), self.driver_name(), "sigma_thresh"))
    }

    fn set_sigma_threshold(&mut self, _sigma_mm: u32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "sigma_thresh"))
    }

    /// (x, y) SPADs.
    fn get_roi(&mut self) -> Result<(u8, u8)> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn set_roi(&mut self, _x: u8, _y: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn get_roi_center(&mut self) -> Result<u8> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn set_roi_center(&mut self, _center_spad: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn start_temperature_update(&mut self) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "temp_update"))
    }

    /// VL53L0X: re-measure the reference SPADs → (count, is_aperture).
    fn perform_ref_spad_management(&mut self) -> Result<(u32, bool)> {
        Err(unsupported(self.product(), self.driver_name(), "refspad"))
    }

    // ── histogram driver ─────────────────────────────────────────────────────

    /// The raw histogram of a frame (read from the sensor when `raw` is `None`).
    fn bin_data(&mut self, _raw: Option<&[u8]>) -> Result<BinData> {
        Err(DepzError::Unsupported(format!("{} has no histogram", self.driver_name())))
    }

    /// A histogram frame → a range (the host-side ranging of the histogram driver).
    fn to_measurement(&mut self, _bins: &BinData) -> Result<Vl53lxRange> {
        Err(DepzError::Unsupported(format!("{} has no histogram", self.driver_name())))
    }
}
```

The contract every driver of the 1D family implements (`uld/base.py`). The methods say what the
product is as far as the bridge is concerned and which optional groups it serves
([`Vl53lxDriver::supports`]): mode, timing, offset, xtalk, calib_offset, calib_xtalk,
thresholds, signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks fails
with [`DepzError::Unsupported`]. Methods that touch the sensor take `&mut self`: a driver may keep
state (the histogram driver keeps the frame pair).

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

### SOFT_RESET

```rust
pub const SOFT_RESET: u16 = 0x0000;
```

`SOFT_RESET`.

### I2C_SLAVE_DEVICE_ADDRESS

```rust
pub const I2C_SLAVE_DEVICE_ADDRESS: u16 = 0x0001;
```

`I2C_SLAVE__DEVICE_ADDRESS`.

### OSC_FREQUENCY

```rust
pub const OSC_FREQUENCY: u16 = 0x0006;
```

`OSC_MEASURED__FAST_OSC__FREQUENCY`.

### VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND

```rust
pub const VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND: u16 = 0x0008;
```

`VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`.

### XTALK_PLANE_OFFSET_KCPS

```rust
pub const XTALK_PLANE_OFFSET_KCPS: u16 = 0x0016;
```

`ALGO__CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS`.

### XTALK_X_PLANE_GRADIENT_KCPS

```rust
pub const XTALK_X_PLANE_GRADIENT_KCPS: u16 = 0x0018;
```

`ALGO__CROSSTALK_COMPENSATION_X_PLANE_GRADIENT_KCPS`.

### XTALK_Y_PLANE_GRADIENT_KCPS

```rust
pub const XTALK_Y_PLANE_GRADIENT_KCPS: u16 = 0x001A;
```

`ALGO__CROSSTALK_COMPENSATION_Y_PLANE_GRADIENT_KCPS`.

### RANGE_OFFSET_MM

```rust
pub const RANGE_OFFSET_MM: u16 = 0x001E;
```

`ALGO__PART_TO_PART_RANGE_OFFSET_MM`.

### INNER_OFFSET_MM

```rust
pub const INNER_OFFSET_MM: u16 = 0x0020;
```

`MM_CONFIG__INNER_OFFSET_MM`.

### OUTER_OFFSET_MM

```rust
pub const OUTER_OFFSET_MM: u16 = 0x0022;
```

`MM_CONFIG__OUTER_OFFSET_MM`.

### GPIO_HV_MUX_CTRL

```rust
pub const GPIO_HV_MUX_CTRL: u16 = 0x0030;
```

`GPIO_HV_MUX__CTRL`.

### GPIO_TIO_HV_STATUS

```rust
pub const GPIO_TIO_HV_STATUS: u16 = 0x0031;
```

`GPIO__TIO_HV_STATUS`.

### SYSTEM_INTERRUPT_CONFIG_GPIO

```rust
pub const SYSTEM_INTERRUPT_CONFIG_GPIO: u16 = 0x0046;
```

`SYSTEM__INTERRUPT_CONFIG_GPIO`.

### PHASECAL_CONFIG_TIMEOUT_MACROP

```rust
pub const PHASECAL_CONFIG_TIMEOUT_MACROP: u16 = 0x004B;
```

`PHASECAL_CONFIG__TIMEOUT_MACROP`.

### RANGE_CONFIG_TIMEOUT_MACROP_A_HI

```rust
pub const RANGE_CONFIG_TIMEOUT_MACROP_A_HI: u16 = 0x005E;
```

`RANGE_CONFIG__TIMEOUT_MACROP_A_HI`.

### RANGE_CONFIG_VCSEL_PERIOD_A

```rust
pub const RANGE_CONFIG_VCSEL_PERIOD_A: u16 = 0x0060;
```

`RANGE_CONFIG__VCSEL_PERIOD_A`.

### RANGE_CONFIG_TIMEOUT_MACROP_B_HI

```rust
pub const RANGE_CONFIG_TIMEOUT_MACROP_B_HI: u16 = 0x0061;
```

`RANGE_CONFIG__TIMEOUT_MACROP_B_HI`.

### RANGE_CONFIG_VCSEL_PERIOD_B

```rust
pub const RANGE_CONFIG_VCSEL_PERIOD_B: u16 = 0x0063;
```

`RANGE_CONFIG__VCSEL_PERIOD_B`.

### RANGE_CONFIG_SIGMA_THRESH

```rust
pub const RANGE_CONFIG_SIGMA_THRESH: u16 = 0x0064;
```

`RANGE_CONFIG__SIGMA_THRESH`.

### MIN_COUNT_RATE_RTN_LIMIT_MCPS

```rust
pub const MIN_COUNT_RATE_RTN_LIMIT_MCPS: u16 = 0x0066;
```

`RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS`.

### RANGE_CONFIG_VALID_PHASE_HIGH

```rust
pub const RANGE_CONFIG_VALID_PHASE_HIGH: u16 = 0x0069;
```

`RANGE_CONFIG__VALID_PHASE_HIGH`.

### INTERMEASUREMENT_MS

```rust
pub const INTERMEASUREMENT_MS: u16 = 0x006C;
```

`SYSTEM__INTERMEASUREMENT_PERIOD`.

### THRESH_HIGH

```rust
pub const THRESH_HIGH: u16 = 0x0072;
```

`SYSTEM__THRESH_HIGH`.

### THRESH_LOW

```rust
pub const THRESH_LOW: u16 = 0x0074;
```

`SYSTEM__THRESH_LOW`.

### SD_CONFIG_WOI_SD0

```rust
pub const SD_CONFIG_WOI_SD0: u16 = 0x0078;
```

`SD_CONFIG__WOI_SD0`.

### SD_CONFIG_INITIAL_PHASE_SD0

```rust
pub const SD_CONFIG_INITIAL_PHASE_SD0: u16 = 0x007A;
```

`SD_CONFIG__INITIAL_PHASE_SD0`.

### ROI_CONFIG_USER_ROI_CENTRE_SPAD

```rust
pub const ROI_CONFIG_USER_ROI_CENTRE_SPAD: u16 = 0x007F;
```

`ROI_CONFIG__USER_ROI_CENTRE_SPAD`.

### ROI_CONFIG_USER_ROI_XY_SIZE

```rust
pub const ROI_CONFIG_USER_ROI_XY_SIZE: u16 = 0x0080;
```

`ROI_CONFIG__USER_ROI_REQUESTED_GLOBAL_XY_SIZE`.

### SYSTEM_INTERRUPT_CLEAR

```rust
pub const SYSTEM_INTERRUPT_CLEAR: u16 = 0x0086;
```

`SYSTEM__INTERRUPT_CLEAR`.

### SYSTEM_MODE_START

```rust
pub const SYSTEM_MODE_START: u16 = 0x0087;
```

`SYSTEM__MODE_START`.

### RESULT_RANGE_STATUS

```rust
pub const RESULT_RANGE_STATUS: u16 = 0x0089;
```

`RESULT__RANGE_STATUS`.

### RESULT_OSC_CALIBRATE_VAL

```rust
pub const RESULT_OSC_CALIBRATE_VAL: u16 = 0x00DE;
```

`RESULT__OSC_CALIBRATE_VAL`.

### FIRMWARE_SYSTEM_STATUS

```rust
pub const FIRMWARE_SYSTEM_STATUS: u16 = 0x00E5;
```

`FIRMWARE__SYSTEM_STATUS`.

### IDENTIFICATION_MODEL_ID

```rust
pub const IDENTIFICATION_MODEL_ID: u16 = 0x010F;
```

`IDENTIFICATION__MODEL_ID`.

### ROI_CONFIG_MODE_ROI_CENTRE_SPAD

```rust
pub const ROI_CONFIG_MODE_ROI_CENTRE_SPAD: u16 = 0x013E;
```

`ROI_CONFIG__MODE_ROI_CENTRE_SPAD`.

### WINDOW_BELOW

```rust
pub const WINDOW_BELOW: u8 = 0;
```

Detection-threshold window: interrupt below `distance_low_mm`.

### WINDOW_ABOVE

```rust
pub const WINDOW_ABOVE: u8 = 1;
```

Detection-threshold window: interrupt above `distance_high_mm`.

### WINDOW_OUT

```rust
pub const WINDOW_OUT: u8 = 2;
```

Detection-threshold window: interrupt outside the window.

### WINDOW_IN

```rust
pub const WINDOW_IN: u8 = 3;
```

Detection-threshold window: interrupt inside the window.

### CONFIG_ADDR

```rust
pub const CONFIG_ADDR: u16 = 0x002D;
```

The configuration blob spans 0x2D..0x87 on every part.

### CONFIG_END

```rust
pub const CONFIG_END: u16 = 0x0087;
```

End (exclusive) of the configuration blob.

### CONFIG_FMP_BYTE

```rust
pub const CONFIG_FMP_BYTE: u8 = 0x12;
```

Byte 0 of the blob, always: the Fast Mode Plus byte.

### RESULT_BLOCK_ADDR

```rust
pub const RESULT_BLOCK_ADDR: u16 = RESULT_RANGE_STATUS;
```

Start of the streamed result block (`RESULT__RANGE_STATUS`).

### RESULT_BLOCK_LEN

```rust
pub const RESULT_BLOCK_LEN: usize = 17;
```

Length of the streamed result block, bytes.

### DieConfig

```rust
pub struct DieConfig {
    /// The 91-byte blob written at [`CONFIG_ADDR`] (byte 0 is replaced by [`CONFIG_FMP_BYTE`]).
    pub configuration: &'static [u8; 91],
    /// What stop_ranging writes: the L4CD ULD 0x80, the L3CX ULP and the L1X ULD 0x00.
    pub stop_mode: u8,
    /// Where the signal rate comes from in the block: 0x008E (offset 5) on the L4CD ULD, 0x0098 (15)
    /// on the L1X ULD.
    pub signal_at: usize,
    /// Per-SPAD scale: 256 on the L4CD ULD, 25 on the L1X ULD (ST's own difference, kept).
    pub per_spad_k: i64,
}
```

What differs between the drivers on the die (the overridable constants of the Java
`Vl53l1Die`).

### phase_window_mm

```rust
pub fn phase_window_mm(fast_osc_frequency: i64, valid_phase_high: i64) -> i64
```

How far a target can be before the die fails it with status 4, mm.

### status_text

```rust
pub fn status_text(status: i32) -> String
```

The ULD status text: `"unknown (N)"` for a code the table lacks.

### Die

```rust
pub struct Die {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    cfg: DieConfig,
}
```

The VL53L1 die one of the three die drivers runs: the platform it is reached through, the product
it was bound for and the per-driver constants. Each driver exposes it ([`super::Vl53l4UldDriver::die`]
…) for the die-level calls the contract has no method for (boot state, reset, interrupt polarity).

#### Die::platform

```rust
pub fn platform(&self) -> &Arc<dyn Vl53lxPlatform>
```

The platform the die is reached through.

#### Die::product

```rust
pub fn product(&self) -> &str
```

The product the driver was bound for.

#### Die::config

```rust
pub fn config(&self) -> &DieConfig
```

The per-driver constants.

#### Die::decode

```rust
pub fn decode(&self, raw: &[u8]) -> Result<Vl53lxRange>
```

The 17-byte result block → a range.

#### Die::get_result

```rust
pub fn get_result(&self) -> Result<Vl53lxRange>
```

One block read instead of the C driver's six register reads; the decoding is identical.

#### Die::model_id

```rust
pub fn model_id(&self) -> Result<u16>
```

`IDENTIFICATION__MODEL_ID`.

#### Die::boot_state

```rust
pub fn boot_state(&self) -> Result<u8>
```

`FIRMWARE__SYSTEM_STATUS` (0x03 = booted).

#### Die::wait_boot

```rust
pub fn wait_boot(&self, timeout_ms: u64) -> Result<()>
```

Poll the boot state every 1 ms until 0x03, up to `timeout_ms`.

#### Die::reset_device

```rust
pub fn reset_device(&self, timeout_ms: u64) -> Result<()>
```

The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT.

#### Die::clear_interrupt

```rust
pub fn clear_interrupt(&self) -> Result<()>
```

`SYSTEM__INTERRUPT_CLEAR` ← 1.

#### Die::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

The L4CD ULD start: continuous when the period register is 0, else autonomous low power.

#### Die::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

`SYSTEM__MODE_START` ← the driver's stop mode.

#### Die::get_interrupt_polarity

```rust
pub fn get_interrupt_polarity(&self) -> Result<u8>
```

1 = active high, 0 = active low (`GPIO_HV_MUX__CTRL` bit 4, inverted).

#### Die::set_interrupt_polarity

```rust
pub fn set_interrupt_polarity(&self, polarity: u8) -> Result<()>
```

Set the interrupt polarity (1 = active high), keeping the other bits.

#### Die::check_for_data_ready

```rust
pub fn check_for_data_ready(&self) -> Result<bool>
```

The polarity first, as both C drivers read it, then `GPIO__TIO_HV_STATUS` bit 0.

#### Die::wait_data_ready

```rust
pub fn wait_data_ready(&self, timeout_ms: u64) -> Result<()>
```

Poll [`Die::check_for_data_ready`] every 1 ms (the platform's sleep) up to `timeout_ms`.

#### Die::set_range_timing

```rust
pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()>
```

The L4CD ULD timing: any budget of 10..200 ms; `inter_measurement_ms` 0 = continuous, else it
must exceed the budget (autonomous low power).

#### Die::get_range_timing

```rust
pub fn get_range_timing(&self) -> Result<(u32, u32)>
```

The L4CD ULD read-back: (budget ms, inter-measurement ms).

#### Die::reach_mm

```rust
pub fn reach_mm(&self) -> Result<Option<i32>>
```

Read off the loaded configuration: 1388 mm for the 0x38 window, 4563 mm for 0xB8.

#### Die::set_detection_thresholds

```rust
pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()>
```

The L4CD ULD thresholds: the window byte written whole.

#### Die::get_detection_thresholds

```rust
pub fn get_detection_thresholds(&self) -> Result<Thresholds>
```

The distance window and its mode.

#### Die::set_signal_threshold

```rust
pub fn set_signal_threshold(&self, signal_kcps: u32) -> Result<()>
```

Minimum signal, kcps (the register keeps kcps / 8).

#### Die::get_signal_threshold

```rust
pub fn get_signal_threshold(&self) -> Result<u32>
```

Minimum signal, kcps.

#### Die::set_sigma_threshold

```rust
pub fn set_sigma_threshold(&self, sigma_mm: u32) -> Result<()>
```

Maximum sigma, mm (≤ 16383).

#### Die::get_sigma_threshold

```rust
pub fn get_sigma_threshold(&self) -> Result<u32>
```

Maximum sigma, mm.

#### Die::set_offset

```rust
pub fn set_offset(&self, offset_mm: i32) -> Result<()>
```

The range offset, mm (the inner / outer offsets are zeroed).

#### Die::get_offset

```rust
pub fn get_offset(&self) -> Result<i32>
```

The range offset, mm.

#### Die::set_xtalk

```rust
pub fn set_xtalk(&self, xtalk_kcps: i32) -> Result<()>
```

The crosstalk plane offset, kcps (the gradients are zeroed).

#### Die::get_xtalk

```rust
pub fn get_xtalk(&self) -> Result<i32>
```

The crosstalk plane offset, kcps.

#### Die::set_roi

```rust
pub fn set_roi(&self, x: u8, y: u8) -> Result<()>
```

An X by Y window of SPADs, 4..16 each way.

#### Die::get_roi

```rust
pub fn get_roi(&self) -> Result<(u8, u8)>
```

(x, y) SPADs.

#### Die::set_roi_center

```rust
pub fn set_roi_center(&self, center_spad: u8) -> Result<()>
```

The ROI centre SPAD.

#### Die::get_roi_center

```rust
pub fn get_roi_center(&self) -> Result<u8>
```

The ROI centre SPAD.

### MODEL_ID_VL53L0X

```rust
pub const MODEL_ID_VL53L0X: u8 = 0xEE;
```

`IDENTIFICATION_MODEL_ID` (0xC0) of a VL53L0X.

### RefCalibration

```rust
pub struct RefCalibration {
    /// VHV setting.
    pub vhv: u8,
    /// Phase setting.
    pub phase: u8,
}
```

The VHV and phase settings the reference calibration found.

### RefSpads

```rust
pub struct RefSpads {
    /// Number of reference SPADs.
    pub count: i32,
    /// 1 = aperture SPADs, 0 = not.
    pub is_aperture: i32,
}
```

The reference SPAD set: how many and whether they are aperture SPADs (1) or not (0).

### isqrt

```rust
pub fn isqrt(mut num: i64) -> i64
```

VL53L0X_isqrt(): the bit-by-bit integer square root.

### Vl53l0xUldDriver

```rust
pub struct Vl53l0xUldDriver {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    // PALDevData / DeviceSpecificParameters: the state the C driver keeps beside the sensor.
    read_data_from_device_done: i32,
    xtalk_compensation_rate_mega_cps: i64,
    xtalk_compensation_enable: i32,
    stop_variable: u8,
    sequence_config: u8,
    range_fractional_enable: i32,
    device_mode: i32,
    continuous_mode: Option<i32>, // not in the C driver: resumed after a single shot
    measurement_timing_budget_us: i64,
    pin0_gpio_functionality: u8,
    reference_spad_count: i32,
    reference_spad_type: i32,
    ref_good_spad_map: [u8; REF_SPAD_BUFFER_SIZE],
    #[allow(dead_code)] // kept as the C driver keeps it
    ref_spad_enables: [u8; REF_SPAD_BUFFER_SIZE],
    pre_range_vcsel_pulse_period: i32,
    final_range_vcsel_pulse_period: i32,
    pre_range_timeout_us: i64,
    final_range_timeout_us: i64,
    target_ref_rate: i32,
    limit_checks_enable: [i32; CHECK_NUMBER_OF_CHECKS],
    limit_checks_value: [i64; CHECK_NUMBER_OF_CHECKS],
    mode: String,

    ref_spads_from_nvm: bool,
    module_id: u8,
    revision: u8,
    product_id_string: String,
    part_uid_upper: i64,
    part_uid_lower: i64,
    signal_rate_meas_fixed_400mm: i64,
    part2part_offset_adjustment_nvm_um: i64,
    osc_frequency_mhz: i64,
}
```

The VL53L0X API 1.0.4 ranging path: 8-bit register addresses, no Fast Mode Plus, four ST ranging
profiles as modes, offset / crosstalk calibration and reference SPAD management. There is no ROI
and there are no detection thresholds on this die.

#### Vl53l0xUldDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l0xUldDriver
```

A driver over `platform`, bound for `product`.

#### Vl53l0xUldDriver::ref_spads_from_nvm

```rust
pub fn ref_spads_from_nvm(&self) -> bool
```

False when the NVM reference-SPAD record was out of range and was measured instead.

#### Vl53l0xUldDriver::module_id

```rust
pub fn module_id(&self) -> u8
```

NVM module id.

#### Vl53l0xUldDriver::revision

```rust
pub fn revision(&self) -> u8
```

NVM revision.

#### Vl53l0xUldDriver::product_id_string

```rust
pub fn product_id_string(&self) -> &str
```

NVM product id string.

#### Vl53l0xUldDriver::part_uid_upper

```rust
pub fn part_uid_upper(&self) -> i64
```

NVM part UID, upper 32 bits.

#### Vl53l0xUldDriver::part_uid_lower

```rust
pub fn part_uid_lower(&self) -> i64
```

NVM part UID, lower 32 bits.

#### Vl53l0xUldDriver::signal_rate_meas_fixed_400mm

```rust
pub fn signal_rate_meas_fixed_400mm(&self) -> i64
```

Factory signal rate at 400 mm, Mcps 16.16.

#### Vl53l0xUldDriver::part2part_offset_adjustment_nvm_um

```rust
pub fn part2part_offset_adjustment_nvm_um(&self) -> i64
```

Factory part-to-part offset, µm.

#### Vl53l0xUldDriver::osc_frequency_mhz

```rust
pub fn osc_frequency_mhz(&self) -> i64
```

Oscillator frequency, MHz 16.16.

#### Vl53l0xUldDriver::get_reference_spads

```rust
pub fn get_reference_spads(&self) -> RefSpads
```

The reference SPAD set in use.

#### Vl53l0xUldDriver::wait_boot

```rust
pub fn wait_boot(&self, timeout_ms: u64) -> Result<()>
```

No firmware-status register: the model id answering says the die is up.

#### Vl53l0xUldDriver::reset_device

```rust
pub fn reset_device(&self, timeout_ms: u64) -> Result<()>
```

VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it.

#### Vl53l0xUldDriver::perform_ref_calibration

```rust
pub fn perform_ref_calibration(&mut self) -> Result<RefCalibration>
```

VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice.

#### Vl53l0xUldDriver::get_info_from_device

```rust
pub fn get_info_from_device(&mut self, option: i32) -> Result<()>
```

VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product
identification (2), the part UID and factory offset (4).

#### Vl53l0xUldDriver::set_offset_um

```rust
pub fn set_offset_um(&self, offset_um: i64) -> Result<()>
```

The register is 10.2 format in mm: steps of 250 µm.

#### Vl53l0xUldDriver::get_offset_um

```rust
pub fn get_offset_um(&self) -> Result<i64>
```

The part-to-part offset, µm.

#### Vl53l0xUldDriver::perform_offset_calibration

```rust
pub fn perform_offset_calibration(&mut self, cal_distance_mm: i32, nb_samples: i32) -> Result<i64>
```

VL53L0X_perform_offset_calibration() → µm (the contract's default is 50 samples). TCC and the
range-ignore threshold are off for the run.

#### Vl53l0xUldDriver::perform_xtalk_calibration

```rust
pub fn perform_xtalk_calibration(&mut self, cal_distance_mm: i32, nb_samples: i32) -> Result<i64>
```

VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16 (the contract's default is 50
samples).

### Vl53l1UldDriver

```rust
pub struct Vl53l1UldDriver {
    die: Die,
    // The mode last applied: budget_choices answers from it (PHASECAL reads 0 before init).
    applied_mode: &'static str,
}
```

VL53L1X ULD 3.5.5 for VL53L1CX and VL53L1CB: a tabulated timing budget, short / long distance
modes, an init that resets the die first. Crosstalk is in kcps, the register's own unit (ST's
wrapper converts to cps).

#### Vl53l1UldDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l1UldDriver
```

A driver over `platform`, bound for `product`.

#### Vl53l1UldDriver::die

```rust
pub fn die(&self) -> &Die
```

The die underneath (boot state, reset, interrupt polarity).

#### Vl53l1UldDriver::set_detection_thresholds_ext

```rust
pub fn set_detection_thresholds_ext(
    &mut self,
    distance_low_mm: u16,
    distance_high_mm: u16,
    window: u8,
    int_on_no_target: bool,
) -> Result<()>
```

VL53L1X_SetDistanceThreshold(): a read-modify-write keeping the bits outside 0x6F;
`int_on_no_target` sets bit 6.

### Vl53l4UldDriver

```rust
pub struct Vl53l4UldDriver {
    die: Die,
}
```

VL53L4CD ULD 2.2.3 for VL53L4CD and VL53L4CX alike.

#### Vl53l4UldDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l4UldDriver
```

A driver over `platform`, bound for `product`.

#### Vl53l4UldDriver::die

```rust
pub fn die(&self) -> &Die
```

The die underneath (boot state, reset, interrupt polarity).

### Vl53l3UlpDriver

```rust
pub struct Vl53l3UlpDriver {
    die: Die,
}
```

VL53L3CX ULP 1.0.0: plain single-target ranging on the histogram part.

#### Vl53l3UlpDriver::new

```rust
pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l3UlpDriver
```

A driver over `platform`, bound for `product`.

#### Vl53l3UlpDriver::die

```rust
pub fn die(&self) -> &Die
```

The die underneath (boot state, reset, interrupt polarity).

### DriverCtor

```rust
pub type DriverCtor = fn(Arc<dyn Vl53lxPlatform>, &str) -> Box<dyn Vl53lxDriver>;
```

Builds a driver over a platform, bound for a product.

### HIST_CAVEAT

```rust
pub const HIST_CAVEAT: &str =
    "the histogram driver has no calibrations and no detection thresholds - the light drivers are the ones with those";
```

What the UI says about every histogram pair.

### driver_kinds

```rust
pub fn driver_kinds(product: &str) -> Result<Vec<&'static str>>
```

The driver kinds `product` has, in [`DRIVER_KINDS`] order; [`DepzError::Unsupported`] for no such
product. Empty for a product whose only kinds this crate does not run yet.

### create_driver

```rust
pub fn create_driver(
    product: &str,
    kind: &str,
    platform: Arc<dyn Vl53lxPlatform>,
) -> Result<(Box<dyn Vl53lxDriver>, Option<&'static str>)>
```

The driver for one (product, kind) pair, and the caveat a UI shows about the pair (or `None`).
[`DepzError::Unsupported`] for a pair the table has no row for, naming the kinds the product has.

## BNO086 (IMU)

### Bno086Cmd

```rust
pub enum Bno086Cmd {
    /// Hardware reset through nRST; RPT_STATUS OK.
    SensorReset = 0x32,
    /// 1 ms WAKE (PS0) pulse; RPT_STATUS OK.
    SensorWakeUp = 0x33,
    /// Payload = one raw SHTP frame; RPT_STATUS OK or ERR_BUSY (both TX slots full).
    SendShtpPacket = 0x34,
}
```

Bridge commands of the BNO086 board: the MCU only shuttles raw SHTP frames.

### RPT_DATA

```rust
pub const RPT_DATA: u8 = 0x91;
```

RPT_DATA: cmd u8 (always 0x00, ERRATA E2), timestamp_us u64, raw SHTP frame.

### BUSY_BACKOFF_MS

```rust
pub const BUSY_BACKOFF_MS: u64 = 200;
```

Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY, to back off
at least this long, ms.

### Bno086Data

```rust
pub struct Bno086Data {
    pub cmd: u8,
    pub timestamp_us: u64,
    pub shtp: Vec<u8>,
}
```

One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime.

#### Bno086Data::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<Bno086Data, CodecError>
```

### FORCE_SENSOR_FLUSH

```rust
pub const FORCE_SENSOR_FLUSH: u8 = 0xF0;
```

Host → hub on the control channel; answered by Flush Completed (0xEF) on the input channel.

### EXECUTABLE_RESET_COMPLETE

```rust
pub const EXECUTABLE_RESET_COMPLETE: u8 = 0x01;
```

Inbound on the executable channel: the hub finished a reset.

### CMD_ERRORS

```rust
pub const CMD_ERRORS: u8 = 0x01;
```

### CMD_COUNTER

```rust
pub const CMD_COUNTER: u8 = 0x02;
```

### CMD_TARE

```rust
pub const CMD_TARE: u8 = 0x03;
```

### CMD_INITIALIZE

```rust
pub const CMD_INITIALIZE: u8 = 0x04;
```

### CMD_SAVE_DCD

```rust
pub const CMD_SAVE_DCD: u8 = 0x06;
```

### CMD_ME_CALIBRATE

```rust
pub const CMD_ME_CALIBRATE: u8 = 0x07;
```

### CMD_PERIODIC_DCD_CONFIG

```rust
pub const CMD_PERIODIC_DCD_CONFIG: u8 = 0x09;
```

### CMD_GET_OSCILLATOR_TYPE

```rust
pub const CMD_GET_OSCILLATOR_TYPE: u8 = 0x0A;
```

### CMD_CLEAR_DCD_AND_RESET

```rust
pub const CMD_CLEAR_DCD_AND_RESET: u8 = 0x0B;
```

### CMD_INTERACTIVE_CALIBRATION

```rust
pub const CMD_INTERACTIVE_CALIBRATION: u8 = 0x0E;
```

BNO086 only; no response.

### OSC_INTERNAL

```rust
pub const OSC_INTERNAL: u8 = 0;
```

Get Oscillator Type R0: internal.

### OSC_EXT_CRYSTAL

```rust
pub const OSC_EXT_CRYSTAL: u8 = 1;
```

Get Oscillator Type R0: external crystal.

### OSC_EXT_CLOCK

```rust
pub const OSC_EXT_CLOCK: u8 = 2;
```

Get Oscillator Type R0: external clock.

### ERROR_SOURCE_NO_MORE_ERRORS

```rust
pub const ERROR_SOURCE_NO_MORE_ERRORS: u8 = 255;
```

The end of the error queue (the `source` of an error record, SH-2 §6.4.1).

### TARE_X

```rust
pub const TARE_X: u8 = 1;
```

Tare axes, a bitmap.

### TARE_Y

```rust
pub const TARE_Y: u8 = 2;
```

### TARE_Z

```rust
pub const TARE_Z: u8 = 4;
```

### TARE_ALL

```rust
pub const TARE_ALL: u8 = 7;
```

### TareBasis

```rust
pub enum TareBasis {
    RotationVector = 0,
    GameRotationVector = 1,
    GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3,
    ArvrStabilizedRv = 4,
    ArvrStabilizedGameRv = 5,
}
```

The rotation vector a tare refers to (Tare Now P2).

### MotionIntent

```rust
pub enum MotionIntent {
    Unknown = 0,
    StationaryNoVibration = 1,
    StationaryWithVibration = 2,
    InMotion = 3,
    InMotionAccelerating = 4,
}
```

P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the device's
motion. `Unknown` is the hub's initial state.

### FRS_STATIC_CALIBRATION_AGM

```rust
pub const FRS_STATIC_CALIBRATION_AGM: u16 = 0x7979;
```

### FRS_NOMINAL_CALIBRATION

```rust
pub const FRS_NOMINAL_CALIBRATION: u16 = 0x4D4D;
```

### FRS_DYNAMIC_CALIBRATION

```rust
pub const FRS_DYNAMIC_CALIBRATION: u16 = 0x1F1F;
```

### FRS_ME_POWER_MGMT

```rust
pub const FRS_ME_POWER_MGMT: u16 = 0xD3E2;
```

### FRS_SYSTEM_ORIENTATION

```rust
pub const FRS_SYSTEM_ORIENTATION: u16 = 0x2D3E;
```

Mounting quaternion, 4 × Q30 words.

### FRS_ACCEL_ORIENTATION

```rust
pub const FRS_ACCEL_ORIENTATION: u16 = 0x2D41;
```

### FRS_GYROSCOPE_ORIENTATION

```rust
pub const FRS_GYROSCOPE_ORIENTATION: u16 = 0x2D46;
```

### FRS_MAGNETOMETER_ORIENTATION

```rust
pub const FRS_MAGNETOMETER_ORIENTATION: u16 = 0x2D4C;
```

### FRS_ARVR_STABILIZATION_RV

```rust
pub const FRS_ARVR_STABILIZATION_RV: u16 = 0x3E2D;
```

### FRS_ARVR_STABILIZATION_GRV

```rust
pub const FRS_ARVR_STABILIZATION_GRV: u16 = 0x3E2E;
```

### FRS_SERIAL_NUMBER

```rust
pub const FRS_SERIAL_NUMBER: u16 = 0x4B4B;
```

One 32-bit word; empty unless programmed.

### FRS_USER_RECORD

```rust
pub const FRS_USER_RECORD: u16 = 0x74B4;
```

Free for the user, up to 64 words.

### FRS_SIG_MOTION_DETECT_CONFIG

```rust
pub const FRS_SIG_MOTION_DETECT_CONFIG: u16 = 0xC274;
```

### FRS_SHAKE_DETECT_CONFIG

```rust
pub const FRS_SHAKE_DETECT_CONFIG: u16 = 0x7D7D;
```

### FRS_STABILITY_DETECTOR_CONFIG

```rust
pub const FRS_STABILITY_DETECTOR_CONFIG: u16 = 0xED85;
```

### FRS_ACTIVITY_TRACKER_CONFIG

```rust
pub const FRS_ACTIVITY_TRACKER_CONFIG: u16 = 0xED88;
```

### METADATA_RECORDS

```rust
pub const METADATA_RECORDS: [(u8, u16); 24] = [
    (0x14, 0xE301), (0x01, 0xE302), (0x04, 0xE303), (0x06, 0xE304), (0x15, 0xE305), (0x02, 0xE306),
    (0x07, 0xE307), (0x16, 0xE308), (0x03, 0xE309), (0x0F, 0xE30A), (0x05, 0xE30B), (0x08, 0xE30C),
    (0x09, 0xE30D), (0x10, 0xE313), (0x18, 0xE314), (0x11, 0xE315), (0x12, 0xE316), (0x13, 0xE317),
    (0x19, 0xE318), (0x1E, 0xE31C), (0x28, 0xE322), (0x29, 0xE323), (0x2A, 0xE324), (0x2B, 0xE325),
];
```

Sensor id → its metadata FRS record.

### FRS_NO_ERROR

```rust
pub const FRS_NO_ERROR: u8 = 0;
```

### FRS_UNRECOGNIZED_FRS_TYPE

```rust
pub const FRS_UNRECOGNIZED_FRS_TYPE: u8 = 1;
```

### FRS_BUSY

```rust
pub const FRS_BUSY: u8 = 2;
```

### FRS_READ_COMPLETED

```rust
pub const FRS_READ_COMPLETED: u8 = 3;
```

### FRS_OFFSET_OUT_OF_RANGE

```rust
pub const FRS_OFFSET_OUT_OF_RANGE: u8 = 4;
```

### FRS_RECORD_EMPTY

```rust
pub const FRS_RECORD_EMPTY: u8 = 5;
```

### FRS_BLOCK_COMPLETED

```rust
pub const FRS_BLOCK_COMPLETED: u8 = 6;
```

### FRS_BLOCK_AND_READ_COMPLETED

```rust
pub const FRS_BLOCK_AND_READ_COMPLETED: u8 = 7;
```

### FRS_DEVICE_ERROR

```rust
pub const FRS_DEVICE_ERROR: u8 = 8;
```

### FRS_WRITE_WORDS_RECEIVED

```rust
pub const FRS_WRITE_WORDS_RECEIVED: u8 = 0;
```

### FRS_WRITE_UNRECOGNIZED_FRS_TYPE

```rust
pub const FRS_WRITE_UNRECOGNIZED_FRS_TYPE: u8 = 1;
```

### FRS_WRITE_BUSY

```rust
pub const FRS_WRITE_BUSY: u8 = 2;
```

### FRS_WRITE_COMPLETED

```rust
pub const FRS_WRITE_COMPLETED: u8 = 3;
```

### FRS_WRITE_MODE_READY

```rust
pub const FRS_WRITE_MODE_READY: u8 = 4;
```

### FRS_WRITE_FAILED

```rust
pub const FRS_WRITE_FAILED: u8 = 5;
```

### FRS_WRITE_NOT_IN_WRITE_MODE

```rust
pub const FRS_WRITE_NOT_IN_WRITE_MODE: u8 = 6;
```

### FRS_WRITE_INVALID_LENGTH

```rust
pub const FRS_WRITE_INVALID_LENGTH: u8 = 7;
```

### FRS_WRITE_RECORD_VALID

```rust
pub const FRS_WRITE_RECORD_VALID: u8 = 8;
```

### FRS_WRITE_RECORD_INVALID

```rust
pub const FRS_WRITE_RECORD_INVALID: u8 = 9;
```

### Sh2HubError

```rust
pub struct Sh2HubError(pub String);
```

A SH-2 level failure: a response that is not the one asked for, a non-zero command status, an
FRS error.

### FeatureResponse

```rust
pub struct FeatureResponse {
    pub sensor_id: u8,
    pub flags: u8,
    pub sensitivity: u16,
    pub interval_us: u32,
    pub batch_us: u32,
    pub cfg_word: u32,
}
```

Get Feature Response (0xFC), 17 bytes: the rates in effect.

#### FeatureResponse::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<FeatureResponse, Sh2HubError>
```

### ProductId

```rust
pub struct ProductId {
    pub reset_cause: u8,
    pub sw_version_major: u8,
    pub sw_version_minor: u8,
    pub sw_part_number: u32,
    pub sw_build_number: u32,
    pub sw_version_patch: u16,
}
```

Product ID Response (0xF8), 16 bytes, one per software component (the SH-2 application first);
the reset cause (SH-2 §6.4.5.2) is set on the first only.

#### ProductId::version

```rust
pub fn version(&self) -> String
```

"major.minor.patch".

#### ProductId::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<ProductId, Sh2HubError>
```

### CommandResponse

```rust
pub struct CommandResponse {
    pub seq: u8,
    pub command: u8,
    pub command_seq: u8,
    pub response_seq: u8,
    pub r: [u8; 11],
}
```

Command Response (0xF1), 16 bytes. `command_seq` echoes the request (correlate on it plus
`command`); `response_seq` counts responses to one request; R0 is the status of most commands
(0 = success).

#### CommandResponse::status

```rust
pub fn status(&self) -> u8
```

#### CommandResponse::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<CommandResponse, Sh2HubError>
```

### FrsReadResponse

```rust
pub struct FrsReadResponse {
    pub status: u8,
    pub data_length: u8,
    pub offset_words: u16,
    pub data0: u32,
    pub data1: u32,
    pub frs_type: u16,
}
```

FRS Read Response (0xF3), 16 bytes, up to two data words.

#### FrsReadResponse::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<FrsReadResponse, Sh2HubError>
```

### FrsWriteResponse

```rust
pub struct FrsWriteResponse {
    pub status: u8,
    pub offset_words: u16,
}
```

FRS Write Response (0xF5), 4 bytes.

#### FrsWriteResponse::unpack

```rust
pub fn unpack(p: &[u8]) -> Result<FrsWriteResponse, Sh2HubError>
```

### ErrorRecord

```rust
pub struct ErrorRecord {
    pub severity: u8,
    pub seq: u8,
    pub source: u8,
    pub error: u8,
    pub module: u8,
    pub code: u8,
}
```

One error-queue entry (command 0x01 response, R0..R5).

#### ErrorRecord::from_response

```rust
pub fn from_response(r: &CommandResponse) -> ErrorRecord
```

### Counts

```rust
pub struct Counts {
    pub sensor_id: u8,
    pub offered: u32,
    pub accepted: u32,
    pub on: u32,
    pub attempted: u32,
}
```

Per-sensor event counts (command 0x02, two responses).

### CalibrationConfig

```rust
pub struct CalibrationConfig {
    pub accel: bool,
    pub gyro: bool,
    pub mag: bool,
    pub planar: bool,
}
```

ME calibration enables as the sensor reports them.

### SensorMetadata

```rust
pub struct SensorMetadata {
    pub me_version: u8,
    pub mh_version: u8,
    pub sh_version: u8,
    pub range_raw: u32,
    pub resolution_raw: u32,
    pub revision: u16,
    pub power_ma_q10: u16,
    pub min_period_us: u32,
    pub max_period_us: u32,
    pub fifo_max: u16,
    pub fifo_reserved: u16,
    pub batch_buffer_bytes: u16,
    pub q_point_1: u16,
    pub q_point_2: u16,
    pub q_point_3: u16,
    pub raw_words: Vec<u32>,
}
```

A sensor's metadata FRS record (revision-gated fields are 0 on older records); `raw_words` is
authoritative.

#### SensorMetadata::from_words

```rust
pub fn from_words(words: &[u32]) -> SensorMetadata
```

### COUNTS_GET

```rust
pub const COUNTS_GET: u8 = 0;
```

### COUNTS_CLEAR

```rust
pub const COUNTS_CLEAR: u8 = 1;
```

### ME_CAL_GET

```rust
pub const ME_CAL_GET: u8 = 0x01;
```

ME Calibration subcommand: report the current configuration.

### split_control_cargo

```rust
pub fn split_control_cargo(payload: &[u8]) -> Vec<Vec<u8>>
```

Split a control cargo into its reports; an unknown id or a short tail ends the walk (the rest is
one piece: its length is unknowable).

### counts_get_params

```rust
pub fn counts_get_params(sensor_id: u8) -> [u8; 2]
```

### counts_clear_params

```rust
pub fn counts_clear_params(sensor_id: u8) -> [u8; 2]
```

### errors_params

```rust
pub fn errors_params(severity: u8) -> [u8; 1]
```

Errors of `severity` or greater (0 = all).

### tare_now_params

```rust
pub fn tare_now_params(axes: u8, basis: TareBasis) -> [u8; 3]
```

`axes`: a `TARE_*` bitmap.

### persist_tare_params

```rust
pub fn persist_tare_params() -> [u8; 1]
```

### set_reorientation_params

```rust
pub fn set_reorientation_params(x: f64, y: f64, z: f64, w: f64) -> Result<[u8; 9], Sh2HubError>
```

Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears); a component
out of the Q14 int16 range is refused.

### me_calibration_params

```rust
pub fn me_calibration_params(accel: bool, gyro: bool, mag: bool, planar: bool, subcommand: u8) -> [u8; 5]
```

ME Calibration (0x07): subcommand 0 configures, 1 ([`ME_CAL_GET`]) reads back.

### interactive_calibration_params

```rust
pub fn interactive_calibration_params(intent: u8) -> [u8; 1]
```

Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response.

### force_flush

```rust
pub fn force_flush(sensor_id: u8) -> [u8; 2]
```

Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF).

### periodic_dcd_params

```rust
pub fn periodic_dcd_params(enable: bool) -> [u8; 1]
```

Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response.

### FrsReadSession

```rust
pub struct FrsReadSession {
    pub frs_type: u16,
    pub words: Vec<u32>,
    pub done: bool,
}
```

A multi-packet FRS read: send [`FrsReadSession::request`], [`FrsReadSession::feed`] every 0xF3 of
the record until it returns true; `words` holds the record. Error statuses fail.

#### FrsReadSession::new

```rust
pub fn new(frs_type: u16) -> FrsReadSession
```

#### FrsReadSession::request

```rust
pub fn request(&self) -> [u8; 8]
```

#### FrsReadSession::feed

```rust
pub fn feed(&mut self, r: &FrsReadResponse) -> Result<bool, Sh2HubError>
```

### FrsWriteSession

```rust
pub struct FrsWriteSession {
    pub frs_type: u16,
    pub words: Vec<u32>,
    pub done: bool,
    offset: usize,
}
```

A multi-packet FRS write: send [`FrsWriteSession::request`]; for every 0xF5
[`FrsWriteSession::feed`] returns the next Write Data payload, or `None`; `done` on
WRITE_COMPLETED. Error statuses fail.

#### FrsWriteSession::new

```rust
pub fn new(frs_type: u16, words: &[u32]) -> FrsWriteSession
```

#### FrsWriteSession::request

```rust
pub fn request(&self) -> [u8; 6]
```

#### FrsWriteSession::feed

```rust
pub fn feed(&mut self, r: &FrsWriteResponse) -> Result<Option<[u8; 12]>, Sh2HubError>
```

### BASE_TIMESTAMP_REF

```rust
pub const BASE_TIMESTAMP_REF: u8 = 0xfb;
```

In-cargo control IDs on the input channels.

### TIMESTAMP_REBASE

```rust
pub const TIMESTAMP_REBASE: u8 = 0xfa;
```

### FLUSH_COMPLETED

```rust
pub const FLUSH_COMPLETED: u8 = 0xef;
```

Flush Completed: + u8 sensor ID, no SH-2 header; the answer to Force Sensor
Flush (0xF0).

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
    /// 0x2B motion request (BNO086 interactive calibration): the motion
    /// intent the hub was last told, and what it asks of the device (SH-2
    /// §6.5.45.2; 1, 3, 4 = stay still). Sent when the request changes, not
    /// periodically, whatever the granted rate.
    MotionRequest {
        sensor_id: u8,
        timestamp_us: i64,
        seq: u8,
        accuracy: u8,
        delay_us: i64,
        intent: u8,
        request: u8,
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
    /// 0xEF Flush Completed: every batched report of `flushed_sensor` as of
    /// the matching Force Sensor Flush has been sent. `sensor_id` is 0xEF; no
    /// SH-2 header, so the timestamp is the cargo's current base.
    FlushCompleted {
        sensor_id: u8,
        timestamp_us: i64,
        flushed_sensor: u8,
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

#### Report::sensor_id

```rust
pub fn sensor_id(&self) -> u8
```

The report id (0xEF for Flush Completed).

#### Report::timestamp_us

```rust
pub fn timestamp_us(&self) -> i64
```

The report's measurement time on the MCU clock, µs.

### Vector3Kind

```rust
pub enum Vector3Kind {
    Acceleration,
    Gyroscope,
    Magnetometer,
}
```

Which of the plain 3-vector reports.

### try_parse_input_cargo

```rust
pub fn try_parse_input_cargo(payload: &[u8], capture_timestamp_us: i64) -> Result<Vec<Report>, CodecError>
```

[`parse_input_cargo`] that refuses a negative `capture_timestamp_us`: the
capture time is the MCU uptime, a u64 (contract 07 §3).

### try_parse_gyro_rv_cargo

```rust
pub fn try_parse_gyro_rv_cargo(payload: &[u8], capture_timestamp_us: i64) -> Result<Option<Report>, CodecError>
```

[`parse_gyro_rv_cargo`] that refuses a negative `capture_timestamp_us`
(contract 07 §3).

### parse_input_cargo

```rust
pub fn parse_input_cargo(payload: &[u8], capture_timestamp_us: i64) -> Vec<Report>
```

Parse a channel-3/4 cargo into typed reports. `capture_timestamp_us` is the
bridge RPT_DATA capture time (MCU uptime); a negative one is not checked —
prefer [`try_parse_input_cargo`].

### parse_gyro_rv_cargo

```rust
pub fn parse_gyro_rv_cargo(payload: &[u8], capture_timestamp_us: i64) -> Option<Report>
```

Parse a channel-5 cargo (gyro-integrated RV, dense format).

Two shapes: 7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay
(both 100 µs ticks). Returns `None` if the cargo is too short. A negative
`capture_timestamp_us` is not checked — prefer [`try_parse_gyro_rv_cargo`].

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

### FEATURE_FLAG_WAKEUP

```rust
pub const FEATURE_FLAG_WAKEUP: u8 = 0x04;
```

Set Feature `flags` bit 2 (SH-2 §6.5.4): wake-up enabled — the sensor
reports on SHTP channel 4 (wake input).

### FEATURE_FLAG_ALWAYS_ON

```rust
pub const FEATURE_FLAG_ALWAYS_ON: u8 = 0x08;
```

Set Feature `flags` bit 3 (SH-2 §6.5.4): always-on — the sensor keeps
running while the hub sleeps. With [`FEATURE_FLAG_WAKEUP`] a report kept
its full rate through the executable-channel sleep on a DEPZ BNO086
(SH-2 3.12.6, 2026-10-16), on channel 4, while every other report stopped.

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

### ExecutableCommand

```rust
pub enum ExecutableCommand {
    Reset = 1,
    On = 2,
    Sleep = 3,
}
```

One-byte host writes on the executable channel (SHTP channel 1,
contract 05 §6a). The hub answers none of them.

`Reset` restarts the hub (answered later by the 0x01 reset-complete on the
same channel); `Sleep` stops every report except those enabled with
`sh2::FEATURE_FLAG_WAKEUP | sh2::FEATURE_FLAG_ALWAYS_ON`, keeping the
configuration; `On` ends `Sleep` and every report resumes at its rate.

### build_sleep

```rust
pub fn build_sleep() -> [u8; 1]
```

SH-2 "Sleep" payload for the executable channel: `[0x03]`. Send it with
`try_next_frame(ShtpChannel::Executable as u8, &build_sleep())` through the
bridge's SEND_SHTP_PACKET (0x34); the hub sends no response.

Every report stops, except those enabled with flags wake-up | always-on,
which keep their rate; the configuration (enabled sensors, rates) is kept.
[`build_power_on`] ends it. On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16) all
reports stopped within one sample; a WAKEUP|ALWAYS_ON report kept
streaming at full rate on channel 4.

### build_power_on

```rust
pub fn build_power_on() -> [u8; 1]
```

SH-2 "On" payload for the executable channel: `[0x02]` — ends
[`build_sleep`]'s sleep. Every report that was enabled comes back at its
rate, without re-enabling; the hub sends no response.

Not the bridge's SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin —
a hardware signal, not an SH-2 command.

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

### ShtpError

```rust
pub enum ShtpError {
    /// `length` does not fit the 15-bit length field (max 0x7FFF).
    LengthTooWide(usize),
    /// Fewer than the 4 header bytes.
    ShortHeader(usize),
    /// Channel outside 0..[`NUM_CHANNELS`].
    ChannelOutOfRange(u8),
    /// A host→sensor frame larger than one MCU transmit slot ([`MAX_TX_FRAME`]).
    TxCargoTooLong(usize),
}
```

SHTP codec error (contract 07 §3: an argument that does not fit its wire
field, or input shorter than the fields it must hold, is rejected —
never masked, wrapped or read past).

### try_pack_shtp_header

```rust
pub fn try_pack_shtp_header(hdr: &ShtpHeader) -> Result<[u8; 4], ShtpError>
```

Serialize an SHTP header to 4 bytes; `length` must fit 15 bits
([`ShtpError::LengthTooWide`] otherwise — never masked).

### try_unpack_shtp_header

```rust
pub fn try_unpack_shtp_header(data: &[u8]) -> Result<ShtpHeader, ShtpError>
```

Decode an SHTP header from the first 4 bytes of `data` (trailing bytes —
the cargo — are ignored); [`ShtpError::ShortHeader`] when shorter.

### ShtpCargo

```rust
pub struct ShtpCargo {
    pub channel: u8,
    /// seq of the first fragment.
    pub seq: u8,
    pub payload: Vec<u8>,
    /// `RPT_DATA.timestamp_us` of the FIRST fragment (contract 05 §3) — the
    /// capture the cargo's 0xFB base delta refers to; 0 when fed via [`ShtpLayer::feed`].
    pub capture_us: u64,
}
```

One reassembled cargo: `payload` excludes all SHTP headers.

### try_build_frame

```rust
pub fn try_build_frame(channel: u8, payload: &[u8], seq: u8) -> Result<Vec<u8>, ShtpError>
```

Single-fragment frame: length = header + payload. A frame longer than the
15-bit length field holds is [`ShtpError::LengthTooWide`].

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

#### ShtpLayer::try_next_frame

```rust
pub fn try_next_frame(&mut self, channel: u8, payload: &[u8]) -> Result<Vec<u8>, ShtpError>
```

Build a single-fragment frame, consuming the channel's TX seq (the seq
wraps 255 → 0 by design). The frame must fit one MCU transmit slot
([`ShtpError::TxCargoTooLong`]; host-side control payloads are ≤ 21
bytes) and `channel` must be 0..[`NUM_CHANNELS`]
([`ShtpError::ChannelOutOfRange`]). Nothing is consumed on an error.

#### ShtpLayer::try_tx_seq

```rust
pub fn try_tx_seq(&self, channel: u8) -> Option<u8>
```

Current TX seq counter for a channel (next frame's seq); `None` for a
channel outside 0..[`NUM_CHANNELS`].

#### ShtpLayer::tx_seq

```rust
pub fn tx_seq(&self, channel: u8) -> u8
```

Current TX seq counter for a channel (next frame's seq).

# Panics

When `channel` ≥ [`NUM_CHANNELS`]; use [`ShtpLayer::try_tx_seq`].

#### ShtpLayer::feed

```rust
pub fn feed(&mut self, frame: &[u8]) -> Option<ShtpCargo>
```

Consume one inbound frame; return the cargo when complete.

Rules (contract 05 §3): a non-continuation fragment starts a new cargo
(discarding any partial one on that channel); a continuation without a
cargo in progress is dropped; the cargo completes when the accumulated
bytes reach the first fragment's advertised total.

Same as [`ShtpLayer::feed_at`] with capture 0 (cargo `capture_us` = 0).

#### ShtpLayer::feed_at

```rust
pub fn feed_at(&mut self, frame: &[u8], capture_us: u64) -> Option<ShtpCargo>
```

[`ShtpLayer::feed`] with the frame's `RPT_DATA.timestamp_us`: the cargo
carries the FIRST fragment's capture in `capture_us` — the base its 0xFB
timestamps refer to (contract 05 §3), not the fragment that completed it.

#### ShtpLayer::reset

```rust
pub fn reset(&mut self)
```

Forget all TX seq counters and partial cargos (sensor reset).

### RATE_LOW_FACTOR

```rust
pub const RATE_LOW_FACTOR: f64 = 0.9;
```

Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error
(contract 05 §7).

### RATE_HIGH_FACTOR

```rust
pub const RATE_HIGH_FACTOR: f64 = 2.1;
```

### Bno086Device

```rust
pub struct Bno086Device {
    dev: DepzDevice,
    hub: Arc<Hub>,
    tx: Mutex<()>, // serialise SEND_SHTP_PACKET
    cmd_seq: AtomicU8,
    busy: Mutex<(u32, Duration)>,
}
```

BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames
(SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK.
Every send is acknowledged by the bridge at once and every inbound frame arrives as
RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only. Never call a blocking device
method (`enable`, `tare_now`, …) from an [`Bno086Device::on_report`] callback.

#### Bno086Device::attach

```rust
pub fn attach(dev: DepzDevice) -> Bno086Device
```

Continue the open connection `dev` as a BNO085 / BNO086.

#### Bno086Device::set_busy_retry

```rust
pub fn set_busy_retry(&self, retries: u32, backoff: Duration) -> Result<()>
```

SEND_SHTP_PACKET attempts on ERR_BUSY before giving up, and the back-off between them (at least
200 ms per the bridge spec).

#### Bno086Device::advertisement

```rust
pub fn advertisement(&self) -> Vec<u8>
```

The raw SHTP channel-0 advertisement seen since open or reset.

#### Bno086Device::hardware_reset

```rust
pub fn hardware_reset(&self, timeout: Option<Duration>) -> Result<()>
```

Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2 executable
reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it, ERRATA E9). SHTP state
and cached features restart from zero. `timeout` `None`: 2 s.

#### Bno086Device::wake

```rust
pub fn wake(&self) -> Result<()>
```

Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends [`Bno086Device::sleep`] (that
is [`Bno086Device::power_on`]).

#### Bno086Device::sleep

```rust
pub fn sleep(&self) -> Result<()>
```

SH-2 "Sleep": every report stops except those enabled with wake-up | always-on; the
configuration is kept. No response.

#### Bno086Device::power_on

```rust
pub fn power_on(&self) -> Result<()>
```

SH-2 "On": ends [`Bno086Device::sleep`]; every enabled report comes back. No response.

#### Bno086Device::product_id

```rust
pub fn product_id(&self, timeout: Option<Duration>) -> Result<ProductId>
```

The SH-2 application's Product ID — the first of [`Bno086Device::product_ids`].

#### Bno086Device::product_ids

```rust
pub fn product_ids(&self, timeout: Option<Duration>, settle: Option<Duration>) -> Result<Vec<ProductId>>
```

Every component's Product ID Response, the SH-2 application first. Collects until no response
came for `settle` (default 150 ms), so late ones cannot answer the next request.

#### Bno086Device::interval_us

```rust
pub fn interval_us(hz: f64) -> Result<u32>
```

Hz → the Set Feature interval, rounded like the reference (half to even), at least 1 µs.

#### Bno086Device::enable_interval

```rust
pub fn enable_interval(
    &self,
    sensor: u8,
    interval_us: u32,
    batch_us: u32,
    sensitivity: u16,
    flags: u8,
    cfg_word: u32,
    verify: bool,
    timeout: Option<Duration>,
) -> Result<Option<FeatureResponse>>
```

Enable `sensor` through Set Feature (0xFD) every `interval_us`. The hub rounds to its grid;
with `verify` the granted rate is read back through Get Feature and a rate outside 0.9–2.1 × the
request is reported on stderr as a warning (contract 05 §7). Returns that read-back (`None`
without verify).

#### Bno086Device::enable

```rust
pub fn enable(&self, sensor: u8, hz: f64) -> Result<FeatureResponse>
```

[`Bno086Device::enable_interval`] at `hz`, verified.

#### Bno086Device::disable

```rust
pub fn disable(&self, sensor: u8) -> Result<()>
```

Disable `sensor` (Set Feature with interval 0).

#### Bno086Device::get_feature

```rust
pub fn get_feature(&self, sensor: u8, timeout: Option<Duration>) -> Result<FeatureResponse>
```

Get Feature round trip: the rates in effect for `sensor`.

#### Bno086Device::on_report

```rust
pub fn on_report(&self, callback: impl Fn(&Report) + Send + Sync + 'static) -> Hook
```

Call `callback` for every report on the reader thread. Drop the hook to stop.

#### Bno086Device::reports

```rust
pub fn reports(&self, sensors: Option<&[u8]>, capacity: usize) -> Subscription<Report>
```

A pull queue of reports (of `sensors` only, when given), bounded, drop-oldest; subscribed when
this returns.

#### Bno086Device::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

#### Bno086Device::tare_now

```rust
pub fn tare_now(&self, axes: u8, basis: TareBasis) -> Result<()>
```

Tare `axes` (a `hub::TARE_*` bitmap) against `basis` (no response).

#### Bno086Device::persist_tare

```rust
pub fn persist_tare(&self) -> Result<()>
```

Persist the current tare into FRS (no response).

#### Bno086Device::set_reorientation

```rust
pub fn set_reorientation(&self, x: f64, y: f64, z: f64, w: f64) -> Result<()>
```

The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It is
applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the
reports.

#### Bno086Device::set_calibration

```rust
pub fn set_calibration(&self, accel: bool, gyro: bool, mag: bool, planar: bool) -> Result<()>
```

Configure the motion-engine calibration; fails on a non-zero status.

#### Bno086Device::get_calibration

```rust
pub fn get_calibration(&self) -> Result<CalibrationConfig>
```

Which motion-engine calibrations run.

#### Bno086Device::save_dcd

```rust
pub fn save_dcd(&self) -> Result<()>
```

Save the dynamic calibration data to flash (DCD Save Now).

#### Bno086Device::configure_periodic_dcd

```rust
pub fn configure_periodic_dcd(&self, enable: bool) -> Result<()>
```

The hub's periodic DCD autosave (no response).

#### Bno086Device::set_motion_intent

```rust
pub fn set_motion_intent(&self, intent: MotionIntent) -> Result<()>
```

Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub can
refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it.

#### Bno086Device::enable_motion_request

```rust
pub fn enable_motion_request(&self, hz: f64) -> Result<()>
```

Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and reads the
rate back late, so the granted rate is not checked.

#### Bno086Device::flush

```rust
pub fn flush(&self, sensor: u8, timeout: Option<Duration>) -> Result<()>
```

Force Sensor Flush (0xF0): the hub sends `sensor`'s batched reports now; returns once its Flush
Completed (0xEF) arrives.

#### Bno086Device::frs_read

```rust
pub fn frs_read(&self, record_id: u16, timeout: Option<Duration>) -> Result<Vec<u32>>
```

Read a whole FRS record: its 32-bit words.

#### Bno086Device::frs_write

```rust
pub fn frs_write(&self, record_id: u16, words: &[u32], timeout: Option<Duration>) -> Result<()>
```

Write a whole FRS record.

#### Bno086Device::get_metadata

```rust
pub fn get_metadata(&self, sensor: u8, timeout: Option<Duration>) -> Result<SensorMetadata>
```

Read and parse `sensor`'s metadata FRS record.

#### Bno086Device::get_oscillator_type

```rust
pub fn get_oscillator_type(&self) -> Result<u8>
```

Get Oscillator Type (0x0A): R0, one of `hub::OSC_*` (an unknown value comes back as is).

#### Bno086Device::clear_dcd_and_reset

```rust
pub fn clear_dcd_and_reset(&self, timeout: Option<Duration>) -> Result<()>
```

Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of answering, so
this waits for the executable reset-complete.

#### Bno086Device::get_errors

```rust
pub fn get_errors(&self, severity: u8) -> Result<Vec<ErrorRecord>>
```

The error queue (0x01), `severity` or greater; records stream until one with source 255.

#### Bno086Device::get_counts

```rust
pub fn get_counts(&self, sensor: u8) -> Result<Counts>
```

A sensor's event counts (0x02): two responses, response_seq 0 then 1.

#### Bno086Device::clear_counts

```rust
pub fn clear_counts(&self, sensor: u8) -> Result<()>
```

Clear a sensor's event counts (0x02, subcommand 1).

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

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u8, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`BNO_WRITE_REG` payload: `addr u8, data[1..128]`; a data length outside
1..=[`XFER_MAX`] is refused (contract 07 §3), never sent.

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u8, data: &[u8]) -> Vec<u8>
```

`BNO_WRITE_REG` payload: `addr u8, data[1..128]`, unchecked — prefer
[`try_pack_write_reg`].

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

### MODE_SWITCH_FROM_CONFIG

```rust
pub const MODE_SWITCH_FROM_CONFIG: Duration = Duration::from_millis(10);
```

Settle time after an OPR_MODE write out of CONFIG (datasheet: 7 ms).

### MODE_SWITCH_TO_CONFIG

```rust
pub const MODE_SWITCH_TO_CONFIG: Duration = Duration::from_millis(25);
```

Settle time after an OPR_MODE write into CONFIG (datasheet: 19 ms).

### SELF_TEST_TIME

```rust
pub const SELF_TEST_TIME: Duration = Duration::from_millis(450);
```

The built-in self-test runs ~400 ms (datasheet §3.9.2).

### BOOT_SETTLE_TIMEOUT

```rust
pub const BOOT_SETTLE_TIMEOUT: Duration = Duration::from_secs(1);
```

How long the sensor's own boot may take.

### STUCK_SELF_TEST_POLLS

```rust
pub const STUCK_SELF_TEST_POLLS: u32 = 20;
```

SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in CONFIG, not POST.

### FUSION_START_TIMEOUT

```rust
pub const FUSION_START_TIMEOUT: Duration = Duration::from_secs(1);
```

After CONFIG → a fusion mode the fusion outputs read zero for ~70 ms.

### Bno055SystemStatus

```rust
pub struct Bno055SystemStatus {
    pub self_test: u8,
    pub clk_status: u8,
    pub status: u8,
    pub error: u8,
}
```

ST_RESULT (0x36) + SYS_CLK_STATUS / SYS_STATUS / SYS_ERR (0x38..0x3A); INT_STA (0x37) sits in
between and clears on read, so it is not read.

#### Bno055SystemStatus::status_text

```rust
pub fn status_text(&self) -> String
```

#### Bno055SystemStatus::error_text

```rust
pub fn error_text(&self) -> String
```

Meaningful when `status` is 1 (system error).

#### Bno055SystemStatus::self_test_passed

```rust
pub fn self_test_passed(&self) -> bool
```

### Bno055Sample

```rust
pub struct Bno055Sample {
    pub timestamp_us: u64,
    pub addr: u8,
    pub raw: Vec<u8>,
    pub units: Units,
    pub accel: Option<[f64; 3]>,
    pub mag: Option<[f64; 3]>,
    pub gyro: Option<[f64; 3]>,
    pub euler: Option<[f64; 3]>,
    pub quaternion: Option<[f64; 4]>,
    pub linear_accel: Option<[f64; 3]>,
    pub gravity: Option<[f64; 3]>,
    pub temperature: Option<f64>,
    pub calibration: Option<CalibStatus>,
}
```

One decoded BNO055 register block (streamed or polled). A channel is `None` when the block did not
cover it; values are scaled by `units`, the units in effect when the stream started. Euler is
(heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and gravity are always
m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.

#### Bno055Sample::decode

```rust
pub fn decode(timestamp_us: u64, addr: u8, data: &[u8], units: Units) -> Bno055Sample
```

Decode `data` read at `addr` with `units`.

### Bno055Device

```rust
pub struct Bno055Device {
    dev: DepzDevice,
    req: Requester,
    streams: Arc<Streams>,
    // One lock serialises multi-step register sequences (page switch + access, CONFIG round trips).
    state: Mutex<State>,
    streaming: AtomicBool,
}
```

BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode,
units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz.
Typical use: [`Bno055Device::configure`], then [`Bno055Device::start_stream`] and read
[`Bno055Device::samples`], or poll [`Bno055Device::read_sample`]. Page 1 (sensor configs,
interrupts, unique id) is reached through PAGE_ID and refused while streaming.

#### Bno055Device::attach

```rust
pub fn attach(dev: DepzDevice) -> Bno055Device
```

Continue the open connection `dev` as a BNO055.

#### Bno055Device::bridge_info

```rust
pub fn bridge_info(&self) -> Result<Bno055Info>
```

RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming.

#### Bno055Device::is_alive

```rust
pub fn is_alive(&self) -> bool
```

True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids.

#### Bno055Device::reset_sensor

```rust
pub fn reset_sensor(&self) -> Result<()>
```

Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake). Stops a
stream; the sensor returns in CONFIG with power-on units and no calibration — configure again,
or [`Bno055Device::restore_configuration`].

#### Bno055Device::read_registers

```rust
pub fn read_registers(&self, addr: u8, length: usize, page: u8) -> Result<Vec<u8>>
```

Read `length` bytes at `addr` on `page` (0 or 1; 128-byte transfers). Page 1 is refused while
streaming.

#### Bno055Device::write_registers

```rust
pub fn write_registers(&self, addr: u8, data: &[u8], page: u8) -> Result<()>
```

Write at `addr` on `page`. Most configuration registers only take writes in CONFIG mode (the
sensor ignores the rest); the typed setters handle that.

#### Bno055Device::read_register

```rust
pub fn read_register(&self, addr: u8, page: u8) -> Result<u8>
```

#### Bno055Device::write_register

```rust
pub fn write_register(&self, addr: u8, value: u8, page: u8) -> Result<()>
```

#### Bno055Device::get_operation_mode

```rust
pub fn get_operation_mode(&self) -> Result<OprMode>
```

#### Bno055Device::set_operation_mode

```rust
pub fn set_operation_mode(&self, mode: OprMode) -> Result<()>
```

Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to 1 s) for
the fusion outputs. The sensor only switches between CONFIG and another mode, so a switch
between two operating modes goes through CONFIG.

#### Bno055Device::get_power_mode

```rust
pub fn get_power_mode(&self) -> Result<PwrMode>
```

#### Bno055Device::set_power_mode

```rust
pub fn set_power_mode(&self, mode: PwrMode) -> Result<()>
```

Normal / low power / suspend, written in CONFIG with the operating mode restored after. A
suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF
when the chip was already asleep at connect, contract 13 §4.6).

#### Bno055Device::get_units

```rust
pub fn get_units(&self) -> Result<Units>
```

#### Bno055Device::set_units

```rust
pub fn set_units(&self, units: Units) -> Result<()>
```

#### Bno055Device::get_axis_remap

```rust
pub fn get_axis_remap(&self) -> Result<AxisRemap>
```

#### Bno055Device::set_axis_remap

```rust
pub fn set_axis_remap(&self, remap: &AxisRemap) -> Result<()>
```

Remap the output axes for the board's mounting (`AxisRemap::placement` gives the datasheet
placements P0..P7; P1 is the default).

#### Bno055Device::get_temperature_source

```rust
pub fn get_temperature_source(&self) -> Result<TempSource>
```

#### Bno055Device::set_temperature_source

```rust
pub fn set_temperature_source(&self, source: TempSource) -> Result<()>
```

#### Bno055Device::configure

```rust
pub fn configure(
    &self,
    mode: OprMode,
    units: Option<Units>,
    axis_remap: Option<AxisRemap>,
    calibration: Option<CalibrationProfile>,
) -> Result<()>
```

The usual session setup: CONFIG → units → axis remap → calibration profile → `mode`; in a
fusion mode it returns once the fusion outputs are live (~70 ms). Remembered for
[`Bno055Device::restore_configuration`]. `units` `None`: the power-on units; `axis_remap` /
`calibration` `None` leave the sensor's.

#### Bno055Device::restore_configuration

```rust
pub fn restore_configuration(&self) -> Result<()>
```

Re-apply the last [`Bno055Device::configure`] — after [`Bno055Device::reset_sensor`], or when
`bridge_info().sensor_resets` rose (bus recovery pulses nRESET). A running stream keeps running.

#### Bno055Device::system_status

```rust
pub fn system_status(&self) -> Result<Bno055SystemStatus>
```

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.

#### Bno055Device::self_test

```rust
pub fn self_test(&self) -> Result<Bno055SystemStatus>
```

The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is restored
after. Not while streaming.

#### Bno055Device::calibration_status

```rust
pub fn calibration_status(&self) -> Result<CalibStatus>
```

CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background.

#### Bno055Device::read_calibration_profile

```rust
pub fn read_calibration_profile(&self) -> Result<CalibrationProfile>
```

Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back.

#### Bno055Device::write_calibration_profile

```rust
pub fn write_calibration_profile(&self, profile: &CalibrationProfile) -> Result<()>
```

Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background
calibration refines it once fusion resumes.

#### Bno055Device::get_sic_matrix

```rust
pub fn get_sic_matrix(&self) -> Result<[i16; 9]>
```

Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384.

#### Bno055Device::set_sic_matrix

```rust
pub fn set_sic_matrix(&self, matrix: Option<[i16; 9]>) -> Result<()>
```

`matrix` `None` writes the identity.

#### Bno055Device::get_accel_config

```rust
pub fn get_accel_config(&self) -> Result<AccelConfig>
```

#### Bno055Device::set_accel_config

```rust
pub fn set_accel_config(&self, config: &AccelConfig) -> Result<()>
```

Accelerometer range / bandwidth / power; fusion modes override it.

#### Bno055Device::get_gyro_config

```rust
pub fn get_gyro_config(&self) -> Result<GyroConfig>
```

#### Bno055Device::set_gyro_config

```rust
pub fn set_gyro_config(&self, config: &GyroConfig) -> Result<()>
```

#### Bno055Device::get_mag_config

```rust
pub fn get_mag_config(&self) -> Result<MagConfig>
```

#### Bno055Device::set_mag_config

```rust
pub fn set_mag_config(&self, config: &MagConfig) -> Result<()>
```

#### Bno055Device::unique_id

```rust
pub fn unique_id(&self) -> Result<Vec<u8>>
```

The chip's 16-byte unique id (page 1, 0x50..0x5F).

#### Bno055Device::get_interrupt_enable

```rust
pub fn get_interrupt_enable(&self) -> Result<u8>
```

#### Bno055Device::set_interrupt_enable

```rust
pub fn set_interrupt_enable(&self, mask: u8) -> Result<()>
```

INT_EN: which interrupt engines run.

#### Bno055Device::get_interrupt_mask

```rust
pub fn get_interrupt_mask(&self) -> Result<u8>
```

#### Bno055Device::set_interrupt_mask

```rust
pub fn set_interrupt_mask(&self, mask: u8) -> Result<()>
```

INT_MSK: which enabled interrupts drive the INT pin.

#### Bno055Device::set_interrupt_setting

```rust
pub fn set_interrupt_setting(&self, register: u8, value: u8) -> Result<()>
```

One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming.

#### Bno055Device::read_interrupt_status

```rust
pub fn read_interrupt_status(&self) -> Result<u8>
```

INT_STA — which interrupts fired. Clears on read.

#### Bno055Device::clear_interrupt

```rust
pub fn clear_interrupt(&self) -> Result<()>
```

SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.

#### Bno055Device::read_sample

```rust
pub fn read_sample(&self, block: Option<(u8, u8)>) -> Result<Bno055Sample>
```

Poll one register block `(addr, length)` (`None`: the full 46 bytes 0x08..0x35) and decode it.
Works alongside a stream.

#### Bno055Device::read_quaternion

```rust
pub fn read_quaternion(&self) -> Result<[f64; 4]>
```

(w, x, y, z) — the cheapest orientation read (8 bytes).

#### Bno055Device::start_stream

```rust
pub fn start_stream(&self, period_ms: u16, block: Option<(u8, u8)>, int_trigger: bool) -> Result<()>
```

Arm the bridge to read `block` (`None`: the full block) every `period_ms` and push it. Fusion
runs at 100 Hz, so 10 ms is the useful floor. `int_trigger` reads on the INT edge, `period_ms`
then a missed-edge watchdog (0 = none). Replaces a running stream.

#### Bno055Device::stop_stream

```rust
pub fn stop_stream(&self) -> Result<()>
```

#### Bno055Device::streaming

```rust
pub fn streaming(&self) -> bool
```

#### Bno055Device::on_sample

```rust
pub fn on_sample(&self, callback: impl Fn(&Bno055Sample) + Send + Sync + 'static) -> Hook
```

Call `callback` for every streamed sample (reader thread).

#### Bno055Device::samples

```rust
pub fn samples(&self, capacity: usize) -> Subscription<Bno055Sample>
```

A pull queue of streamed samples (bounded, drop-oldest).

#### Bno055Device::get_sample

```rust
pub fn get_sample(&self, timeout: Option<Duration>) -> Result<Bno055Sample>
```

The next streamed sample (contract 07 §3); `timeout` `None`: 1 s.

#### Bno055Device::stream_parse_errors

```rust
pub fn stream_parse_errors(&self) -> u64
```

Stream reports dropped because they did not decode (a short block).

#### Bno055Device::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

## TMF8829 (dToF)

### Tmf8829InitResult

```rust
pub struct Tmf8829InitResult {
    pub app_version: String,
    pub sensor_serial: u32,
}
```

What [`Tmf8829Device::init`] reports: the RAM application version and the sensor's serial number.

### Tmf8829AppliedConfig

```rust
pub struct Tmf8829AppliedConfig {
    pub preset: String,
    pub page: Vec<u8>,
    pub histograms: bool,
    pub ref_frame: bool,
    pub fields: Vec<(&'static str, u16)>,
}
```

The config [`Tmf8829Device::configure`] wrote, as read back from the sensor: the preset name, the
190-byte page 0x22..0xDF, whether histogram frames are dumped and a ref-SPAD frame comes with each
measurement, and every named field of the page (table order).

#### Tmf8829AppliedConfig::field

```rust
pub fn field(&self, name: &str) -> Option<u16>
```

A named field of the page.

#### Tmf8829AppliedConfig::grid

```rust
pub fn grid(&self) -> (usize, usize)
```

(rows, cols) of a measurement.

### Tmf8829Options

```rust
pub struct Tmf8829Options {
    pub period_ms: u32,
    /// `None`: the preset's own.
    pub kilo_iterations: Option<u32>,
    /// 1..4.
    pub nr_peaks: u32,
    pub signal_strength: bool,
    pub noise_strength: bool,
    pub xtalk: bool,
    /// Dump histogram frames with each measurement.
    pub histograms: bool,
    /// A ref-SPAD frame before each results frame.
    pub ref_frame: bool,
    /// Any other page field by name, written last.
    pub fields: Vec<(String, u32)>,
}
```

[`Tmf8829Device::configure`]'s options beyond the preset. `Default`: the default period, the
preset's `kilo_iterations`, one peak, nothing optional.

### Tmf8829Device

```rust
pub struct Tmf8829Device {
    dev: DepzDevice,
    streams: Arc<Streams>,
    reg: Mutex<()>, // the bridge takes one register command at a time
    streaming: AtomicBool,
    app_version: Mutex<Option<String>>,
    sensor_serial: Mutex<Option<u32>>,
    config: Mutex<Option<Tmf8829AppliedConfig>>,
}
```

TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register
bridge (contract 14). The board is a thin bridge: this class runs the sensor.
[`Tmf8829Device::init`] downloads the 14.4 KB sensor firmware (~0.15 s),
[`Tmf8829Device::configure`] writes the config page, [`Tmf8829Device::start`] streams measurements
to [`Tmf8829Device::on_measurement`] / [`Tmf8829Device::measurements`], [`Tmf8829Device::stop`]
ends it. The sensor's EN pin is wired high: there is no sleep, and recovery from any state is
[`Tmf8829Device::init`] (it starts with a soft reset). Closing (or dropping) the device stops a
running stream first.

#### Tmf8829Device::attach

```rust
pub fn attach(dev: DepzDevice) -> Tmf8829Device
```

Continue the open connection `dev` as a TMF8829.

#### Tmf8829Device::interrupt_event

```rust
pub fn interrupt_event(event: &DeviceEvent) -> Option<Tmf8829Event>
```

The `RPT_TMF_EVENT` an [`Tmf8829Device::INTERRUPT_EVENT`] event carries (`None` for any other
event).

#### Tmf8829Device::app_version

```rust
pub fn app_version(&self) -> Option<String>
```

The RAM application version [`Tmf8829Device::init`] started ("2.200.0").

#### Tmf8829Device::sensor_serial

```rust
pub fn sensor_serial(&self) -> Option<u32>
```

The sensor's serial number, read by [`Tmf8829Device::init`].

#### Tmf8829Device::config

```rust
pub fn config(&self) -> Option<Tmf8829AppliedConfig>
```

The last [`Tmf8829Device::configure`] result (`None` after [`Tmf8829Device::init`]).

#### Tmf8829Device::streaming

```rust
pub fn streaming(&self) -> bool
```

#### Tmf8829Device::read_registers

```rust
pub fn read_registers(&self, addr: u8, length: usize) -> Result<Vec<u8>>
```

Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the FIFO port 0xFF stay
there (the sensor does not auto-increment it). A reply shorter or longer than asked is
[`DepzError::Protocol`].

#### Tmf8829Device::read_register

```rust
pub fn read_register(&self, addr: u8) -> Result<u8>
```

#### Tmf8829Device::write_registers

```rust
pub fn write_registers(&self, addr: u8, data: &[u8], progress: Option<&mut dyn FnMut(usize, usize)>) -> Result<()>
```

Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF); `progress` gets
(done, total) after each.

#### Tmf8829Device::write_register

```rust
pub fn write_register(&self, addr: u8, value: u8) -> Result<()>
```

Write one register.

#### Tmf8829Device::sensor_state

```rust
pub fn sensor_state(&self) -> Result<Vec<(&'static str, u8)>>
```

Raw host-interface registers, for diagnostics (name, value; in a fixed order).

#### Tmf8829Device::get_bridge_info

```rust
pub fn get_bridge_info(&self) -> Result<Tmf8829Info>
```

Bridge counters (never touches the sensor, safe mid-stream).

#### Tmf8829Device::init

```rust
pub fn init(&self, progress: Option<&mut dyn FnMut(usize, usize)>) -> Result<Tmf8829InitResult>
```

Bring the sensor from any state to its running RAM application: soft reset, bootloader, firmware
download, start. Stops a running stream first. `progress` (done, total) tracks the image
download. A sensor that refuses or does not answer is [`DepzError::Sensor`].

#### Tmf8829Device::config_page

```rust
pub fn config_page(&self) -> Result<Vec<u8>>
```

LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.

#### Tmf8829Device::configure

```rust
pub fn configure(&self, preset: &str, opts: &Tmf8829Options) -> Result<Tmf8829AppliedConfig>
```

Load `preset` (a name of [`PRESETS`]) and write the config page. Every field a preset does not
reset is written (contract 14 §4), so the result never depends on an earlier config;
`opts.fields` sets any other page field by name. Not while streaming
([`DepzError::InvalidState`]). [`DepzError::Sensor`]: the sensor refuses the page (e.g.
ERR_CONFIG_RESULT_SIZE: the result format is too big for the grid) or a written field does not
read back.

#### Tmf8829Device::start

```rust
pub fn start(&self) -> Result<()>
```

MEASURE with the bridge stream armed. Configures the default 8x8 preset first when
[`Tmf8829Device::configure`] was not called since [`Tmf8829Device::init`]. No-op while streaming.

#### Tmf8829Device::stop

```rust
pub fn stop(&self) -> Result<()>
```

Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a STOP
raises mid histogram dump is expected and swallowed.

#### Tmf8829Device::close

```rust
pub fn close(&self)
```

Stop a running stream (a failing stop is ignored), then close the link.

#### Tmf8829Device::on_measurement

```rust
pub fn on_measurement(&self, callback: impl Fn(&Tmf8829Measurement) + Send + Sync + 'static) -> Hook
```

Call `callback` for every measurement (reader thread; don't block).

#### Tmf8829Device::measurements

```rust
pub fn measurements(&self, capacity: usize) -> Subscription<Tmf8829Measurement>
```

A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.

#### Tmf8829Device::get_measurement

```rust
pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Tmf8829Measurement>
```

The next measurement (contract 07 §3); `timeout` `None`: 2 s.

#### Tmf8829Device::stream_dropped_counts

```rust
pub fn stream_dropped_counts(&self) -> Vec<u64>
```

#### Tmf8829Device::frames_dropped

```rust
pub fn frames_dropped(&self) -> u64
```

Frames the bridge dropped (frame_seq gaps) in this stream.

#### Tmf8829Device::frames_discarded

```rust
pub fn frames_discarded(&self) -> u64
```

Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.

#### Tmf8829Device::frame_parse_errors

```rust
pub fn frame_parse_errors(&self) -> u64
```

#### Tmf8829Device::clock_ratio_q16

```rust
pub fn clock_ratio_q16(&self) -> u32
```

Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per
SYSTICK), for diagnostics.

### Tmf8829Measurement

```rust
pub struct Tmf8829Measurement {
    /// Capture time, MCU µs (floor mean of the half-frame captures).
    pub timestamp_us: i64,
    /// `FT_*` of all its results frames (OR).
    pub flags: u8,
    pub fp_mode: u8,
    pub f_number: u32,
    pub results: PixelResults,
    pub distance_mm: Option<Vec<f64>>,
    pub frames: Vec<FrameInfo>,
    pub times: Vec<FrameTime>,
    pub histograms: Vec<HistogramFrame>,
    /// 65 B each, zero pre-header.
    pub ref_frames: Vec<Vec<u8>>,
    pub problems: Vec<String>,
}
```

One measurement: its results frame (8x8, 16x16) or both half-frames
(32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.

`distance_mm` is flat `[peaks][rows][cols]`, clock-corrected, in mm
(`None` when the result format has no peaks). `results` keeps the raw
sensor integers. `problems` lists what went wrong since the previous
measurement.

#### Tmf8829Measurement::shape

```rust
pub fn shape(&self) -> (usize, usize)
```

(rows, cols) of the full focal plane.

#### Tmf8829Measurement::peaks

```rust
pub fn peaks(&self) -> usize
```

#### Tmf8829Measurement::temperature_c

```rust
pub fn temperature_c(&self) -> i8
```

Sensor die temperature (header byte 0), °C.

### distance_mm

```rust
pub fn distance_mm(results: &PixelResults, ratio: f64, in_mm: bool) -> Option<Vec<f64>>
```

Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units).

### MeasurementBuilder

```rust
pub struct MeasurementBuilder {
    pub clock: SensorClock,
    pub timer: CaptureTimer,
    /// Frames that failed to parse (or of an unknown kind / corrupt results).
    pub parse_errors: u64,
    pending: Vec<(FrameInfo, Vec<u8>)>,
    hist: Vec<HistogramFrame>,
    refs: Vec<Vec<u8>>,
    half: Option<(FrameInfo, PixelResults, FrameTime)>,
    problems: Vec<String>,
}
```

Histogram frames between two results frames belong to the second; a
ref-SPAD frame comes right before its results frame (each half-frame has
its own). 32x32/48x32: the first half (layout 0x40 clear) holds the even
rows. Call [`MeasurementBuilder::reset`] at the start of every stream:
the clock model and T1 are per stream. Mirrors the Python builder,
problem texts included.

#### MeasurementBuilder::new

```rust
pub fn new() -> Self
```

#### MeasurementBuilder::reset

```rust
pub fn reset(&mut self)
```

#### MeasurementBuilder::add

```rust
pub fn add(
    &mut self,
    raw: &[u8],
    timestamp_us: u64,
    edge_us: u64,
) -> Option<Tmf8829Measurement>
```

One frame with the MCU time of its header read and its INT edge
(0 = none) → the measurement it completes, or `None`.

### NOMINAL_Q16

```rust
pub const NOMINAL_Q16: u32 = 8 << 16;
```

SYSTICK 125 kHz = 8 MCU µs per tick, Q16.

### CLOCK_WINDOW_MIN_US

```rust
pub const CLOCK_WINDOW_MIN_US: i64 = 200_000;
```

First ratio estimate after this span.

### CLOCK_WINDOW_SLIDE_US

```rust
pub const CLOCK_WINDOW_SLIDE_US: i64 = 1_000_000;
```

Then a sliding 1..2 s window.

### EDGE_LATE_US

```rust
pub const EDGE_LATE_US: i64 = 2000;
```

### FT_NOMINAL

```rust
pub const FT_NOMINAL: u8 = 0x01;
```

Clock ratio not measured yet (nominal 8 µs/tick used).

### FT_CLOCK_RESET

```rust
pub const FT_CLOCK_RESET: u8 = 0x02;
```

Clock model restarted at this frame (SYSTICK jump).

### FT_EDGE_LATE

```rust
pub const FT_EDGE_LATE: u8 = 0x04;
```

INT edge > 2 ms later after t1 than usual in this stream.

### FT_STALLED

```rust
pub const FT_STALLED: u8 = 0x08;
```

Sub-integrations not back to back (histogram FIFO wait).

### FT_T1_UNSURE

```rust
pub const FT_T1_UNSURE: u8 = 0x10;
```

Histograms + 32x32/48x32: T1 may be too long, centre late.

### FT_NAMES

```rust
pub const FT_NAMES: &[(u8, &str)] = &[
    (FT_NOMINAL, "nominal"),
    (FT_CLOCK_RESET, "clock_reset"),
    (FT_EDGE_LATE, "edge_late"),
    (FT_STALLED, "stalled"),
    (FT_T1_UNSURE, "t1_unsure"),
];
```

### flag_names

```rust
pub fn flag_names(flags: u8) -> Vec<&'static str>
```

Names of the `FT_*` bits set in `flags`.

### fp_subints

```rust
pub fn fp_subints(fp_mode: u8) -> u32
```

Sub-integrations per results frame by `fp_mode` (unknown → 2).

### SensorClock

```rust
pub struct SensorClock {
    /// MCU µs per sensor tick, Q16 ([`NOMINAL_Q16`] until measured).
    pub ratio_q16: u32,
    /// True until the ratio is first measured (or after a reset).
    pub nominal: bool,
    valid: bool,
    last: (u64, u32),
    anchor: (u64, u32),
    mid: Option<(u64, u32)>,
}
```

Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame (the MCU
time of the header read and the pre-header SYSTICK sampled by it). The
ratio is taken over a sliding 1..2 s window, first after 200 ms; it is
also the distance correction (ams `tmf8829ClockCorrection`).

#### SensorClock::new

```rust
pub fn new() -> Self
```

#### SensorClock::reset

```rust
pub fn reset(&mut self)
```

Restart the model — call at every stream start.

#### SensorClock::ratio

```rust
pub fn ratio(&self) -> f64
```

MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.

#### SensorClock::add

```rust
pub fn add(&mut self, us: u64, tick: u32) -> u8
```

Add a pair → [`FT_CLOCK_RESET`] if it does not fit the model (which
then restarts from it), else 0.

#### SensorClock::to_us

```rust
pub fn to_us(&self, pair_us: u64, pair_tick: u32, tick: u32) -> i64
```

SYSTICK `tick` → MCU µs through the pair (`pair_us`, `pair_tick`):
`pair_us + ((s32(tick − pair_tick) × ratio_q16) >> 16)`.

### FrameTime

```rust
pub struct FrameTime {
    /// Centre of the capture window.
    pub timestamp_us: i64,
    /// `FT_*` bits.
    pub flags: u8,
    /// Sub-integration length used.
    pub t1_len_us: u32,
    /// Start of the first sub-integration.
    pub t0_us: i64,
    /// Start of the last one.
    pub t1_us: i64,
    /// INT edge credited to the frame, 0 = none.
    pub edge_us: u64,
    /// MCU time of the header read (the clock pair).
    pub pair_us: u64,
    /// Clock ratio after this frame.
    pub ratio_q16: u32,
}
```

Capture time of one results frame, MCU µs.

### CaptureTimer

```rust
pub struct CaptureTimer {
    lag_min: Option<i64>,
    hist_seen: bool,
    sub_min: u32,
}
```

When a results frame was captured. A results frame is N equal
sub-integrations back to back; the footer has the SYSTICK of the start of
the first (t0) and of the last (t1). The window is `[t0, t1 + T1]`, T1 the
stream's shortest `(t1 − t0)/(N − 1)`; its centre is the frame time.

Holds only its own per-stream state; the [`SensorClock`] it reads is
passed to [`CaptureTimer::results`].

#### CaptureTimer::new

```rust
pub fn new() -> Self
```

#### CaptureTimer::reset

```rust
pub fn reset(&mut self)
```

Restart — call at every stream start.

#### CaptureTimer::histograms

```rust
pub fn histograms(&mut self)
```

A histogram frame was seen in the stream.

#### CaptureTimer::results

```rust
pub fn results(
    &mut self,
    clock: &SensorClock,
    fp_mode: u8,
    systick: u32,
    t0_tick: u32,
    t1_tick: u32,
    pair_us: u64,
    edge_us: u64,
    mut flags: u8,
) -> FrameTime
```

Capture time of a results frame. `flags` is what
[`SensorClock::add`] returned for this frame's pair.

#### CaptureTimer::results_frame

```rust
pub fn results_frame(
    &mut self,
    clock: &SensorClock,
    f: &FrameInfo,
    pair_us: u64,
    edge_us: u64,
    flags: u8,
) -> FrameTime
```

[`CaptureTimer::results`] from a parsed results frame.

### Tmf8829Cmd

```rust
pub enum Tmf8829Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}
```

TMF8829 host→device command opcodes.

### Tmf8829Rpt

```rust
pub enum Tmf8829Rpt {
    RegData = 0x91,
    /// No echoed cmd byte: a host matching replies by `payload[0] == cmd`
    /// must exempt it.
    Info = 0x92,
    Frame = 0x93,
    Event = 0x94,
}
```

TMF8829 device→host report opcodes.

### EV_INT

```rust
pub const EV_INT: u8 = 0;
```

`RPT_TMF_EVENT.code`: a MOTION/PROXIMITY interrupt, no frame; its bits
already cleared.

### EV_BAD_HEADER

```rust
pub const EV_BAD_HEADER: u8 = 1;
```

`RPT_TMF_EVENT.code`: a frame header refused, the frame not read. Expected
exactly once on STOP mid histogram dump (SDKs swallow it); otherwise the
sensor is lost — re-run init.

### READ_MAX

```rust
pub const READ_MAX: usize = 1024;
```

`TMF_READ_REG` `len` 1..=1024.

### WRITE_MAX

```rust
pub const WRITE_MAX: usize = 1023;
```

`TMF_WRITE_REG` N 1..=1023 (1024 B command buffer minus the address).

### CHUNK_MAX

```rust
pub const CHUNK_MAX: usize = 1024;
```

Frame bytes per `RPT_TMF_FRAME` chunk.

### INT_RESULTS

```rust
pub const INT_RESULTS: u8 = 0x01;
```

`INT_STATUS` bits the bridge serves (`TMF_START_STREAM` `int_mask`).

### INT_MOTION

```rust
pub const INT_MOTION: u8 = 0x02;
```

### INT_PROXIMITY

```rust
pub const INT_PROXIMITY: u8 = 0x04;
```

### INT_HISTOGRAMS

```rust
pub const INT_HISTOGRAMS: u8 = 0x08;
```

### INT_MASK_ALL

```rust
pub const INT_MASK_ALL: u8 = 0x0F;
```

### SF_REF_FRAME

```rust
pub const SF_REF_FRAME: u8 = 0x01;
```

`TMF_START_STREAM` flags: emit the 60-byte ref-SPAD frame (behind a zero
5-byte pre-header) before each results frame.

### REG_DATA_HEADER_SIZE

```rust
pub const REG_DATA_HEADER_SIZE: usize = 9;
```

### FRAME_CHUNK_HEADER_SIZE

```rust
pub const FRAME_CHUNK_HEADER_SIZE: usize = 21;
```

### EVENT_SIZE

```rust
pub const EVENT_SIZE: usize = 12;
```

### INFO_SIZE

```rust
pub const INFO_SIZE: usize = 25;
```

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u8, len: u16) -> Result<[u8; 3], CodecError>
```

`TMF_READ_REG` payload: `addr u8, len u16 LE`. A read reaching `0xFF`
(the FIFO port) keeps streaming from the FIFO — there is no
`addr + len ≤ 0x100` check.

### pack_write_reg

```rust
pub fn pack_write_reg(addr: u8, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`TMF_WRITE_REG` payload: `addr u8, data[1..=1023]`.

### pack_start_stream

```rust
pub fn pack_start_stream(int_mask: u8, flags: u8) -> Result<[u8; 2], CodecError>
```

`TMF_START_STREAM` payload: `int_mask u8, flags u8`. `int_mask` a
non-zero subset of [`INT_MASK_ALL`], `flags` only [`SF_REF_FRAME`].

### pack_stop_stream

```rust
pub fn pack_stop_stream() -> [u8; 0]
```

`TMF_STOP_STREAM` payload (empty).

### pack_get_info

```rust
pub fn pack_get_info() -> [u8; 0]
```

`TMF_GET_INFO` payload (empty).

### RegData

```rust
pub struct RegData {
    pub timestamp_us: u64,
    pub data: Vec<u8>,
}
```

A decoded `RPT_TMF_REG_DATA`: register bytes and the MCU time the SPI
read completed. The echoed cmd byte is checked (must be 0x32).

#### RegData::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError>
```

### Tmf8829Info

```rust
pub struct Tmf8829Info {
    pub int_edges: u32,
    pub frames: u32,
    pub frames_aborted: u32,
    pub frame_errors: u32,
    pub spi_timeouts: u32,
    /// INT pin level now, 0 = asserted.
    pub int_level: u8,
    pub streaming: bool,
    pub int_mask: u8,
    pub last_int_status: u8,
    pub last_fifo_status: u8,
}
```

A decoded `RPT_TMF_INFO` (25 B, no echoed cmd byte): bridge state only,
the sensor is never touched. `int_edges` runs from MCU power-up; the frame
counters restart at `TMF_START_STREAM`.

#### Tmf8829Info::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Tmf8829Info, CodecError>
```

### FrameChunk

```rust
pub struct FrameChunk {
    pub timestamp_us: u64,
    pub full_size: u16,
    pub offset: u16,
    pub frame_seq: u8,
    pub edge_us: u64,
    pub data: Vec<u8>,
}
```

One `RPT_TMF_FRAME` chunk. `timestamp_us` + the frame's pre-header
SYSTICK are a clock pair (not the capture time); `edge_us` is the INT
falling edge credited to the frame, 0 = none. `timestamp_us`,
`full_size`, `frame_seq` and `edge_us` are the same in every chunk of a
frame.

#### FrameChunk::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<FrameChunk, CodecError>
```

### Tmf8829Event

```rust
pub struct Tmf8829Event {
    pub timestamp_us: u64,
    pub int_status: u8,
    pub code: u8,
    /// Header id byte (BAD_HEADER), else 0.
    pub frame_id: u8,
    /// Pre-header byte (BAD_HEADER), else 0.
    pub fifo_status: u8,
}
```

A decoded `RPT_TMF_EVENT` (12 B). `int_status` is INT_STATUS as read
(unmasked); `code` is [`EV_INT`] or [`EV_BAD_HEADER`].

#### Tmf8829Event::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Tmf8829Event, CodecError>
```

### RawFrame

```rust
pub struct RawFrame {
    pub timestamp_us: u64,
    pub edge_us: u64,
    pub frame_seq: u8,
    pub data: Vec<u8>,
}
```

A reassembled frame: the sensor bytes from register 0xFA on.

### FrameReassembler

```rust
pub struct FrameReassembler {
    pub completed: u64,
    pub discarded: u64,
    pub seq_gaps: u64,
    buf: Vec<u8>,
    head: Option<Head>,
    next_seq: Option<u8>,
}
```

Rebuilds frames from `RPT_TMF_FRAME` chunks (contract 14 §2): reset on
offset 0; append only a chunk of the same `frame_seq` (and `full_size`)
at the expected offset; complete at `full_size`. Never stitches chunks of
different `frame_seq`. `seq_gaps` counts frames the bridge dropped
(`frame_seq` jumps, u8 wrap). Mirrors the Python `FrameReassembler`.

#### FrameReassembler::new

```rust
pub fn new() -> Self
```

#### FrameReassembler::reset

```rust
pub fn reset(&mut self)
```

Forget everything — call at every stream start (`frame_seq` restarts at 0).

#### FrameReassembler::feed

```rust
pub fn feed(&mut self, chunk: FrameChunk) -> Option<RawFrame>
```

Feed one chunk; returns the frame it completes.

### REG_CFG

```rust
pub const REG_CFG: u8 = 0x22;
```

Config page 0x22..=0xDF.

### CFG_SIZE

```rust
pub const CFG_SIZE: usize = 0xDF - REG_CFG as usize + 1;
```

190 B.

### PRESETS

```rust
pub const PRESETS: &[(&str, u8)] = &[
    ("8x8", 0x40),
    ("8x8-lr", 0x41), // long range
    ("8x8-ha", 0x42), // high accuracy
    ("16x16", 0x43),
    ("16x16-ha", 0x44),
    ("32x32", 0x45),
    ("32x32-ha", 0x46),
    ("48x32", 0x47),
    ("48x32-ha", 0x48),
    ("8x8-xr", 0x49), // extended range, distances in mm
];
```

Preset name → load command (each loads its config page; then
LOAD_CONFIG_PAGE).

### PRESET_KILO_ITERATIONS

```rust
pub const PRESET_KILO_ITERATIONS: &[(&str, u16)] = &[
    ("8x8", 586),
    ("8x8-lr", 586),
    ("8x8-ha", 586),
    ("8x8-xr", 586),
    ("16x16", 586),
    ("16x16-ha", 586),
    ("32x32", 2344),
    ("32x32-ha", 2344),
    ("48x32", 3516),
    ("48x32-ha", 3516),
];
```

The preset's own `kilo_iterations` (a preset rescales a stale value, it
does not reset it — SDKs write it on every configure).

### DEFAULT_PERIOD_MS

```rust
pub const DEFAULT_PERIOD_MS: u16 = 33;
```

The sensor's own `period_ms` after init.

### preset_cmd

```rust
pub fn preset_cmd(name: &str) -> Option<u8>
```

Load command of a preset by name.

### preset_kilo_iterations

```rust
pub fn preset_kilo_iterations(name: &str) -> Option<u16>
```

The preset's own `kilo_iterations` by name.

### CfgField

```rust
pub struct CfgField {
    pub name: &'static str,
    pub reg: u8,
    pub shift: u8,
    pub width: u8,
}
```

One named config-page field: register, bit shift, width in bits
(width 16 = little-endian register pair).

### CFG_FIELDS

```rust
pub const CFG_FIELDS: &[CfgField] = &[
    fld("period_ms", 0x22, 0, 16),
    fld("kilo_iterations", 0x24, 0, 16),
    fld("fp_mode", 0x26, 0, 3),
    fld("nr_peaks", 0x2A, 0, 3),
    fld("signal_strength", 0x2A, 3, 1),
    fld("noise_strength", 0x2A, 4, 1),
    fld("xtalk", 0x2A, 5, 1),
    fld("dump_histograms", 0x2B, 0, 1),
    fld("ref_spad_frame", 0x2C, 0, 1),
    fld("power_modes", 0x2E, 0, 8),
    fld("ha_kilo_iterations", 0x4A, 0, 16),
    fld("dual_mode", 0x4C, 0, 2),
    fld("fov_correction", 0x78, 0, 4),
    fld("distance_in_mm", 0x8F, 0, 1),
];
```

The contract's field table. The raw page is always available for
anything else.

### cfg_field

```rust
pub fn cfg_field(name: &str) -> Option<&'static CfgField>
```

A field of [`CFG_FIELDS`] by name.

### cfg_get

```rust
pub fn cfg_get(page: &[u8], name: &str) -> Result<u16, CodecError>
```

Read a named field from a config page (register 0x22 at index 0).

### cfg_set

```rust
pub fn cfg_set(page: &mut [u8], name: &str, value: u32) -> Result<(), CodecError>
```

Write a named field into a config page; refuses a value that does not fit
the field width (the page is then untouched).

### config_fields

```rust
pub fn config_fields(page: &[u8]) -> Vec<(&'static str, u16)>
```

Every named field of a config page, in table order.

### FIRMWARE_LOAD_ADDR

```rust
pub const FIRMWARE_LOAD_ADDR: u32 = 0x0001_0000;
```

Where the sensor bootloader loads the image.

### FIRMWARE_IMAGE

```rust
pub static FIRMWARE_IMAGE: &[u8] = include_bytes!("data/tmf8829_application.bin");
```

The image bytes.

### FIRMWARE_LICENSE

```rust
pub static FIRMWARE_LICENSE: &str = include_str!("data/LICENSES-MIT.TXT");
```

The image's MIT license (ams-OSRAM), to be shipped with any binary that
embeds it.

### firmware_image

```rust
pub fn firmware_image() -> &'static [u8]
```

The sensor firmware image downloaded by init (14 444 B).

### firmware_words

```rust
pub fn firmware_words() -> u16
```

Image size in 32-bit words, as the bootloader `W_FIFO_BOTH` takes it.

### PRE_HEADER_SIZE

```rust
pub const PRE_HEADER_SIZE: usize = 5;
```

### HEADER_SIZE

```rust
pub const HEADER_SIZE: usize = 16;
```

### FOOTER_SIZE

```rust
pub const FOOTER_SIZE: usize = 12;
```

### FRAME_HEAD

```rust
pub const FRAME_HEAD: usize = PRE_HEADER_SIZE + HEADER_SIZE;
```

The 21 B the bridge reads at 0xFA (pre-header + header).

### FRAME_EOF

```rust
pub const FRAME_EOF: u16 = 0xE0F7;
```

### FID_MASK

```rust
pub const FID_MASK: u8 = 0xF0;
```

### FID_RESULTS

```rust
pub const FID_RESULTS: u8 = 0x10;
```

### FID_HISTOGRAMS

```rust
pub const FID_HISTOGRAMS: u8 = 0x20;
```

### FID_REF_SPAD

```rust
pub const FID_REF_SPAD: u8 = 0x30;
```

### LAYOUT_PEAKS

```rust
pub const LAYOUT_PEAKS: u8 = 0x07;
```

### LAYOUT_SIGNAL

```rust
pub const LAYOUT_SIGNAL: u8 = 0x08;
```

### LAYOUT_NOISE

```rust
pub const LAYOUT_NOISE: u8 = 0x10;
```

### LAYOUT_XTALK

```rust
pub const LAYOUT_XTALK: u8 = 0x20;
```

### LAYOUT_SUBFRAME

```rust
pub const LAYOUT_SUBFRAME: u8 = 0x40;
```

32x32/48x32: set on the second (odd-row) half-frame.

### FRAME_VALID

```rust
pub const FRAME_VALID: u8 = 0x01;
```

### FRAME_ABORTED

```rust
pub const FRAME_ABORTED: u8 = 0xC0;
```

### FRAME_WARNINGS

```rust
pub const FRAME_WARNINGS: &[(u8, &str)] = &[
    (0x08, "HV_CP_OVERLOAD"),
    (0x10, "VCDRV_OVERLOAD"),
    (0x20, "VCDRV_BURST_EXCEEDED"),
];
```

`frame_status` warning bits, in the order `problems()` reports them.

### FOOTER_DIST_IN_MM

```rust
pub const FOOTER_DIST_IN_MM: u8 = 0x01;
```

`footer.reserved` b0: distances in mm, else 0.25 mm.

### FIFO_TX_UNDERRUN

```rust
pub const FIFO_TX_UNDERRUN: u8 = 0x10;
```

### FIFO_RX_OVERRUN

```rust
pub const FIFO_RX_OVERRUN: u8 = 0x20;
```

### fp_grid

```rust
pub fn fp_grid(fp_mode: u8) -> Option<(usize, usize)>
```

`fp_mode` → (rows, cols) of the full focal plane.

### fp_name

```rust
pub fn fp_name(fp_mode: u8) -> Option<&'static str>
```

`fp_mode` → its name (`8x8A`, `8x8B`, `16x16`, `32x32`, `32x32s`, `48x32`).

### frame_rows

```rust
pub fn frame_rows(fp_mode: u8) -> Option<(usize, usize)>
```

(rows, cols) one results frame carries: half the rows for 32x32/48x32.

### FrameError

```rust
pub struct FrameError(pub String);
```

A frame whose bytes do not match its own header (message as the
reference SDK words it).

### FrameInfo

```rust
pub struct FrameInfo {
    pub fifo_status: u8,
    /// Sensor SYSTICK (125 kHz) sampled at the header read.
    pub systick: u32,
    pub id: u8,
    pub layout: u8,
    pub payload: u16,
    pub f_number: u32,
    pub temperature: [i8; 3],
    pub bdv: u8,
    pub ref_pos: [u16; 2],
    /// SYSTICK at the start of the first sub-integration.
    pub t0_integration: u32,
    /// SYSTICK at the start of the last sub-integration.
    pub t1_integration: u32,
    pub frame_status: u8,
    pub reserved: u8,
    pub eof: u16,
    /// Whole frame size, pre-header included.
    pub size: usize,
}
```

Pre-header, header and footer of one frame (any kind).

#### FrameInfo::kind

```rust
pub fn kind(&self) -> u8
```

[`FID_RESULTS`], [`FID_HISTOGRAMS`] or [`FID_REF_SPAD`].

#### FrameInfo::fp_mode

```rust
pub fn fp_mode(&self) -> u8
```

#### FrameInfo::is_second_half

```rust
pub fn is_second_half(&self) -> bool
```

#### FrameInfo::distance_in_mm

```rust
pub fn distance_in_mm(&self) -> bool
```

#### FrameInfo::problems

```rust
pub fn problems(&self) -> Vec<String>
```

What is wrong with the frame, human-readable; empty when fine.

### frame_size

```rust
pub fn frame_size(head: &[u8]) -> Option<usize>
```

Full frame size (pre-header included) from its first 21 bytes
(`5 + 4 + payload`); `None` if fewer than 9 bytes are given.

### parse_frame

```rust
pub fn parse_frame(raw: &[u8]) -> Result<FrameInfo, FrameError>
```

Parse the pre-header, header and footer of a reassembled frame.

### PixelResults

```rust
pub struct PixelResults {
    pub peaks: usize,
    /// Grid of these arrays (half the focal-plane rows for a half-frame).
    pub rows: usize,
    pub cols: usize,
    pub distance: Option<Vec<u16>>,
    pub snr: Option<Vec<u8>>,
    pub signal: Option<Vec<u16>>,
    pub noise: Option<Vec<u16>>,
    pub xtalk: Option<Vec<u16>>,
}
```

Decoded results of one frame or of a merged measurement.

`distance`, `snr`, `signal` are flat `[peaks][rows][cols]`; `noise`,
`xtalk` flat `[rows][cols]`. Raw sensor integers: distance in 0.25 mm (or
mm with the footer flag), not yet clock-corrected. Absent fields are
`None` (no peaks → no distance/snr/signal).

#### PixelResults::shape

```rust
pub fn shape(&self) -> (usize, usize)
```

(rows, cols) of the arrays present; (0, 0) when the result format has
no peaks, noise nor xtalk (as the reference SDK).

#### PixelResults::distance_at

```rust
pub fn distance_at(&self, peak: usize, row: usize, col: usize) -> Option<u16>
```

Raw distance of `peak` at (`row`, `col`).

### pixel_size

```rust
pub fn pixel_size(layout: u8) -> usize
```

Bytes of one results pixel for a `layout` byte.

### parse_results

```rust
pub fn parse_results(raw: &[u8]) -> Result<PixelResults, FrameError>
```

Pixel results of one results frame, row-major.

### parse_results_info

```rust
pub fn parse_results_info(raw: &[u8], f: &FrameInfo) -> Result<PixelResults, FrameError>
```

[`parse_results`] with the frame already parsed.

### merge_halves

```rust
pub fn merge_halves(
    first: &PixelResults,
    second: &PixelResults,
) -> Result<PixelResults, FrameError>
```

32x32/48x32: the first half-frame holds the even rows, the second the
odd ones (`full[2i] = first[i]`, `full[2i+1] = second[i]`). Errors when
the halves have different grids or result formats.

### HistogramFrame

```rust
pub struct HistogramFrame {
    pub info: FrameInfo,
    pub raw: Vec<u8>,
}
```

A raw histogram frame (25 377 B); decoding the bins is left to the
caller (ams histogram layout).

#### HistogramFrame::data

```rust
pub fn data(&self) -> &[u8]
```

The bytes between the header and the footer.

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

### THREAD_NAME

```rust
pub const THREAD_NAME: &str = "depz-dataset-player";
```

The thread name of a threaded player.

### PlayerState

```rust
pub enum PlayerState {
    /// Never played.
    Idle,
    /// Delivering records paced by the clock.
    Playing,
    /// Stopped mid-timeline; [`DatasetPlayer::play`] resumes.
    Paused,
    /// Past the last record; [`DatasetPlayer::play`] restarts from the top.
    Done,
}
```

[`DatasetPlayer`] state (contract 09 §Player).

### DatasetPlayer

```rust
pub struct DatasetPlayer {
    inner: Arc<Inner>,
}
```

Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine
behind a viewer's playback mode.

Timing: a media clock anchored at play / resume / seek to (now, `t` of the last delivered
record — of the first when none was). Record *i* is due at
`anchor_wall + (t_i − anchor_t) / speed`: the first record goes out at once, each later one
after its gap to the previous one. Pacing is against the clock, so a slow consumer gets overdue
records back to back and playback does not drift. Speed 0 delivers as fast as possible.

Threads: records and the [`PlayerState::Done`] state are delivered on a player thread (started
by the first [`DatasetPlayer::play`]); a state change a call makes is reported on the calling
thread before it returns. Callbacks run outside the player's lock, so they may call its
methods; once [`DatasetPlayer::pause`] returns no record callback is running or will start
(unless `pause` was called from that callback). They must not block. With `threaded = false`
no thread starts and the owner calls [`DatasetPlayer::tick`] (a GUI timer, or a test with a
fake clock). Dropping the player stops it.

#### DatasetPlayer::new

```rust
pub fn new(
    dataset: &Dataset,
    speed: f64,
    clock: Option<Box<dyn Fn() -> f64 + Send + Sync>>,
    threaded: bool,
) -> Result<DatasetPlayer>
```

A player over `dataset`. `speed` ≥ 0 (0 = as fast as possible); `clock`: monotonic seconds
(`None`: the system's); `threaded = false`: no player thread, drive it with
[`DatasetPlayer::tick`].

#### DatasetPlayer::state

```rust
pub fn state(&self) -> PlayerState
```

#### DatasetPlayer::duration_us

```rust
pub fn duration_us(&self) -> i64
```

Span of the timeline: the last record's `t` minus the first's.

#### DatasetPlayer::position_us

```rust
pub fn position_us(&self) -> i64
```

µs from the first record to the last delivered one (after a seek: to the record just before
the target); 0 before anything was delivered.

#### DatasetPlayer::speed

```rust
pub fn speed(&self) -> f64
```

Playback speed (finite, ≥ 0; 0 = as fast as possible).

#### DatasetPlayer::set_speed

```rust
pub fn set_speed(&self, value: f64) -> Result<()>
```

Set the playback speed (finite, ≥ 0, else [`DepzError::InvalidArgument`]). A change while
playing applies at once: the rest of the current wait is rescaled.

#### DatasetPlayer::on_record

```rust
pub fn on_record(&self, callback: impl Fn(&Record) + Send + Sync + 'static) -> Hook
```

Call `callback` for every delivered record; dropping the hook unsubscribes.

#### DatasetPlayer::on_state

```rust
pub fn on_state(&self, callback: impl Fn(PlayerState) + Send + Sync + 'static) -> Hook
```

Call `callback` with the new state on every transition.

#### DatasetPlayer::on_delivery_error

```rust
pub fn on_delivery_error(&self, callback: impl Fn(&str) + Send + Sync + 'static) -> Hook
```

Call `callback` with the message of a record callback that panicked on the player thread:
playback paused there (the record counts as delivered).

#### DatasetPlayer::play

```rust
pub fn play(&self) -> Result<()>
```

Start, or resume after [`DatasetPlayer::pause`]; no-op while playing. From
[`PlayerState::Done`] (or a position past the end) playback restarts at the first record.
An empty dataset goes to done at once.

#### DatasetPlayer::pause

```rust
pub fn pause(&self)
```

Stop delivering; [`DatasetPlayer::play`] resumes from the next record, which then waits its
full gap again. No-op unless playing.

#### DatasetPlayer::seek_us

```rust
pub fn seek_us(&self, offset_us: f64) -> Result<()>
```

Move to `offset_us` µs from the first record: the next record delivered is the first with
`t ≥ first t + offset` (negative: the start); records in between are skipped. The state does
not change, except that a seek past the end while playing ends playback. NaN is
[`DepzError::InvalidArgument`].

#### DatasetPlayer::tick

```rust
pub fn tick(&self) -> Option<f64>
```

Deliver every record due now, on the calling thread. Returns the seconds until the next one
is due, or `None` when not playing. For `threaded = false`; harmless with a player thread. A
record callback's panic pauses playback and propagates.

#### DatasetPlayer::wait

```rust
pub fn wait(&self, timeout: Option<Duration>) -> Result<bool>
```

Block until the player is not playing (done, or paused by someone else): `true`, or `false`
on timeout (`None`: no timeout). Needs the player thread, else
[`DepzError::InvalidState`].

#### DatasetPlayer::close

```rust
pub fn close(&self)
```

Stop playback and the player thread. Idempotent.

### RecordSink

```rust
pub struct RecordSink {
    id: Arc<str>,
    offset_us: i64,
    shared: Arc<Shared>,
}
```

Where a device class's hooks write their records: the device id and time offset of one
[`SessionRecorder`] entry. Records written while the recorder is not started are dropped.

#### RecordSink::recording

```rust
pub fn recording(&self) -> bool
```

True while the recorder writes (build a record only then).

#### RecordSink::write

```rust
pub fn write(&self, device_ts_us: i64, kind: &str, value: Value)
```

One record of `kind` at device time `device_ts_us` (mapped onto the host timeline).

#### RecordSink::vl53l8_layers

```rust
pub fn vl53l8_layers(&self) -> bool
```

VL53L8 records also carry signal / ambient / sigma / reflectance.

#### RecordSink::note_sensor_temperature

```rust
pub fn note_sensor_temperature(&self, celsius: f64)
```

The sensor die's latest temperature, sampled into `temperature` records.

### Recordable

```rust
pub trait Recordable {
    fn depz_device(&self) -> &DepzDevice;

    fn hook_records(&self, sink: RecordSink) -> Vec<Hook>;
}
```

A device class a [`SessionRecorder`] can record: its connection, and hooks on its result
stream that write each result to the sink.

### SessionRecorder

```rust
pub struct SessionRecorder {
    path: PathBuf,
    note: String,
    temperature_hz: Option<f64>,
    shared: Arc<Shared>,
    entries: Vec<Entry>,
    temp_thread: Option<(Sender<()>, JoinHandle<()>)>,
}
```

Record decoded data from several devices onto one host timeline (contract 09).
[`SessionRecorder::add`] each open device (it runs [`DepzDevice::sync_time`] and hooks the
device's result stream), then [`SessionRecorder::start`]: a record per result at device time −
offset. It never reconfigures a device — start and stop streaming yourself.
[`SessionRecorder::stop`] (or dropping it) closes the file; the hooks go with the recorder.

#### SessionRecorder::new

```rust
pub fn new(path: impl AsRef<Path>, note: Option<&str>, vl53l8_layers: bool, temperature_hz: Option<f64>) -> Self
```

A recorder writing `path` (`.depzdata`, or `.depzdata.gz`). `vl53l8_layers`: VL53L8 records
also carry signal / ambient / sigma / reflectance. `temperature_hz`: about this many
`temperature` records per device and second (the MCU thermometer, and the sensor die where
frames carry one); `None` for none.

#### SessionRecorder::add

```rust
pub fn add(&mut self, device: &dyn Recordable, device_id: Option<&str>, sync_samples: usize) -> Result<String>
```

Register a device before recording starts: runs [`DepzDevice::sync_time`] with
`sync_samples` rounds so its timestamps land on the shared timeline, and hooks its stream.
Returns its device id (`d0`, `d1`, … unless given).

#### SessionRecorder::records_written

```rust
pub fn records_written(&self) -> u64
```

#### SessionRecorder::start

```rust
pub fn start(&mut self) -> Result<()>
```

Open the file and start writing. No-op while recording.

#### SessionRecorder::stop

```rust
pub fn stop(&mut self) -> Result<()>
```

Stop writing and close the file. Idempotent.

### sr04

```rust
pub fn sr04(m: &Sr04Measurement) -> Value
```

`sr04`: `{echo_us, source}`.

### vl53l4

```rust
pub fn vl53l4(m: &Vl53l4Measurement) -> Value
```

`vl53l4`: the result fields, distance raw whatever the status.

### vl53l8

```rust
pub fn vl53l8(f: &Vl53l8Frame, layers: bool) -> Value
```

`vl53l8`: per-zone arrays cut to the resolution; `layers` adds signal, ambient, sigma and
reflectance; `motion` when the indicator is configured.

### vl53l8_cnh

```rust
pub fn vl53l8_cnh(cnh_raw: &[u8], setup: &Vl53l8CnhSetup) -> Value
```

`vl53l8_cnh`: the raw CNH block of a frame with the armed setup's decode parameters.

### vl53lx

```rust
pub fn vl53lx(m: &Vl53lxMeasurement, product: &str, driver: &str) -> Value
```

`vl53lx`: the family-wide result, plus `targets` when the driver reported several.

### bno055

```rust
pub fn bno055(s: &Bno055Sample) -> Value
```

`bno055`: `unit_sel` and every channel the streamed block covered, scaled.

### tmf8829

```rust
pub fn tmf8829(m: &Tmf8829Measurement) -> Value
```

`tmf8829`: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays.

### bno086_type

```rust
pub fn bno086_type(r: &Report) -> &'static str
```

The report type name of the reference SDK (`Acceleration`, `RotationVector`, `ScalarReport` …).

### bno086

```rust
pub fn bno086(r: &Report) -> Value
```

`bno086`: the report in snake_case — type and sensor id, then only the fields the report
carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.

### round1

```rust
pub fn round1(x: f64) -> f64
```

Python's `round(x, 1)`: the nearest tenth of the exact binary value, ties to even.

### SCHEMA

```rust
pub const SCHEMA: &str = "depz.dataset/2";
```

The schema id this SDK writes.

### DatasetWriter

```rust
pub struct DatasetWriter {
    state: Mutex<State>,
}
```

Low-level thread-safe `.depzdata` record writer (a gzip container when the path ends in `.gz`,
stored uncompressed: the crate has no compression dependency). Prefer
[`super::SessionRecorder`]. Dropping it closes the file.

#### DatasetWriter::create

```rust
pub fn create(path: impl AsRef<Path>, devices: Vec<(String, Value)>, note: Option<&str>) -> io::Result<DatasetWriter>
```

Create `path` (overwritten) and write the header: `devices` are the header device entries by
device id (`serial`, `sensor_type`, `software_name`, `time_sync`, …), `note` free text.

#### DatasetWriter::records_written

```rust
pub fn records_written(&self) -> u64
```

#### DatasetWriter::write

```rust
pub fn write(&self, device_id: &str, t_host_us: i64, kind: &str, value: Value) -> io::Result<()>
```

One record: device id, host µs, kind, value. Fails once the writer is closed.

#### DatasetWriter::close

```rust
pub fn close(&self) -> io::Result<()>
```

Flush and close the file (a gzip file gets its trailer). Idempotent.

### DatasetError

```rust
pub enum DatasetError {
    /// The file has no header line.
    Empty,
    /// A line was not valid JSON.
    Json(JsonError),
    /// A structural expectation failed (missing field, wrong type, …).
    Structure(String),
    /// The file could not be read (or its gzip content inflated).
    Io(String),
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

A parsed dataset: header devices plus the record stream, stable-sorted by `t` (records of
equal `t` keep their file order).

#### Dataset::read

```rust
pub fn read(path: impl AsRef<Path>) -> Result<Dataset, DatasetError>
```

Read the `.depzdata` file at `path`; gzip content (by its magic bytes) is inflated.

#### Dataset::duration_us

```rust
pub fn duration_us(&self) -> i64
```

Span of the timeline: the last record's `t` minus the first's (0 with fewer than two).

#### Dataset::parse

```rust
pub fn parse(text: &str) -> Result<Dataset, DatasetError>
```

Parse a `.depzdata` document from its UTF-8 text.

#### Dataset::records_for

```rust
pub fn records_for<'a>(&'a self, device: &'a str) -> impl Iterator<Item = &'a Record>
```

Records for one device id, in timeline order.

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

#### CrcType::from_value

```rust
pub fn from_value(v: u8) -> Option<CrcType>
```

The listed wire value (0..=3) as a `CrcType`; `None` for any other
value (contract 07 §3: an unlisted enum value is refused, never masked).

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
    UsbTestTxStart = 0x1C,
    UsbTestTxStop = 0x1D,
    UsbTestRxData = 0x1E,
    UsbTestStats = 0x1F,
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
    UsbTestData = 0x88,
    UsbTestStats = 0x8F,
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

#### SyncPinConfig::try_pack

```rust
pub fn try_pack(&self) -> Result<[u8; 3], CodecError>
```

`SET_SYNC_PIN_CONFIG` payload. Contract 02 §6: pin 1..5, mode a
[`SyncPinMode`] (0..4), polarity a [`SyncPinPolarity`] (0..1) — anything
else is refused.

#### SyncPinConfig::pack

```rust
pub fn pack(&self) -> [u8; 3]
```

[`SyncPinConfig::try_pack`] that panics on an invalid configuration.

#### SyncPinConfig::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<SyncPinConfig, CodecError>
```

### SYNC_PIN_MIN

```rust
pub const SYNC_PIN_MIN: u8 = 1;
```

First / last logical AUX sync pin (contract 02 §6).

### SYNC_PIN_MAX

```rust
pub const SYNC_PIN_MAX: u8 = 5;
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

Decode an ASCII device string, dropping trailing NUL/0xFF filler
(contract 02 §4.1).

Device strings are ASCII: every byte ≥ 0x80 becomes one U+FFFD (never
decoded as UTF-8 or Latin-1), and a `0x00` before the trailing filler is
kept as a character, not a terminator.

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

#### Value::int

```rust
pub fn int(v: i64) -> Value
```

A number value from an integer.

#### Value::float

```rust
pub fn float(v: f64) -> Value
```

A number value from a float (shortest text that reads back the same; a whole number keeps
its `.0`, as Python writes floats). Not finite → `null`, which JSON has no number for.

#### Value::to_json

```rust
pub fn to_json(&self) -> String
```

Compact JSON text (`", "` / `": "`-free, as `json.dumps(separators=(",", ":"))`).

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
