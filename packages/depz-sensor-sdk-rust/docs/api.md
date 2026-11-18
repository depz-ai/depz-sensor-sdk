# API reference

Generated from the crate's public surface (the `pub` items and
their `///` doc-comments across `src/**`).
Edit the doc-comments in the source, not this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L4CX](vl53l4cx/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [TMF8829](tmf8829/api.md) · [BNO055](bno055/api.md) · [BNO085](bno085/api.md) · [BNO086](bno086/api.md).

## Contents

- **Discovery**: [`SENSOR_CLASS_NAMES`](#sensor_class_names), [`resolve_sensor_class`](#resolve_sensor_class), [`SensorType`](#sensortype), [`Mode`](#mode), [`Identity`](#identity), [`parse_software_name`](#parse_software_name), [`DEPZ_USB_VID`](#depz_usb_vid), [`PID_SR04`](#pid_sr04), [`DEV_USB_VID`](#dev_usb_vid), [`DEPZ_PID_MODEL`](#depz_pid_model), [`is_known_depz_usb`](#is_known_depz_usb), [`usb_model_hint`](#usb_model_hint), [`PortEntry`](#portentry), [`order_ports`](#order_ports)
- **Live devices**: [`DEFAULT_TIMEOUT`](#default_timeout), [`DeviceEvent`](#deviceevent), [`LinkStats`](#linkstats), [`TimeSync`](#timesync), [`host_now_us`](#host_now_us), [`DepzDevice`](#depzdevice), [`expect_report`](#expect_report), [`expect_text`](#expect_text), [`DepzError`](#depzerror), [`Result`](#result), [`Link`](#link), [`ReplayLink`](#replaylink), [`RecordingLink`](#recordinglink), [`RECORDING_SCHEMA`](#recording_schema), [`DeviceInfo`](#deviceinfo), [`AnyDevice`](#anydevice), [`attach_sensor_class`](#attach_sensor_class), [`open_on_link`](#open_on_link), [`probe_port`](#probe_port), [`list_depz_devices`](#list_depz_devices), [`OpenOptions`](#openoptions), [`open_device`](#open_device), [`DEFAULT_BAUD_RATE`](#default_baud_rate), [`SerialPortInfo`](#serialportinfo), [`list_serial_ports`](#list_serial_ports), [`find_port`](#find_port), [`by_serial`](#by_serial), [`list_depz_ports`](#list_depz_ports), [`SerialPortLink`](#serialportlink), [`Stream`](#stream), [`Subscription`](#subscription), [`Hook`](#hook)
- **SR04**: [`Sr04Measurement`](#sr04measurement), [`Sr04Device`](#sr04device), [`Sr04Cmd`](#sr04cmd), [`Sr04Rpt`](#sr04rpt), [`ECHO_TIMEOUT`](#echo_timeout), [`SAMPLE_PERIOD_DEFAULT_US`](#sample_period_default_us), [`ECHO_DECAY_DEFAULT_US`](#echo_decay_default_us), [`ECHO_DECAY_MIN_US`](#echo_decay_min_us), [`ECHO_DECAY_MAX_US`](#echo_decay_max_us), [`Sr04Data`](#sr04data), [`pack_sample_period`](#pack_sample_period), [`unpack_sample_period`](#unpack_sample_period), [`pack_echo_decay`](#pack_echo_decay), [`unpack_echo_decay`](#unpack_echo_decay), [`distance_mm_from_echo`](#distance_mm_from_echo)
- **VL53L4CD (ToF)**: [`Vl53l4Measurement`](#vl53l4measurement), [`Vl53l4cdDevice`](#vl53l4cddevice), [`Vl53l4Cmd`](#vl53l4cmd), [`Vl53l4Rpt`](#vl53l4rpt), [`XFER_MAX`](#xfer_max), [`XSHUT_OFF`](#xshut_off), [`XSHUT_ON`](#xshut_on), [`XSHUT_RESET`](#xshut_reset), [`SF_INT_ACT_HIGH`](#sf_int_act_high), [`I2C_KHZ_STEPS`](#i2c_khz_steps), [`i2c_error_name`](#i2c_error_name), [`try_pack_read_reg`](#try_pack_read_reg), [`pack_read_reg`](#pack_read_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_write_reg`](#pack_write_reg), [`pack_xshut`](#pack_xshut), [`pack_start_stream`](#pack_start_stream), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`RegData`](#regdata), [`Vl53l4Info`](#vl53l4info), [`StreamData`](#streamdata), [`DetectionThresholds`](#detectionthresholds), [`SOFT_RESET`](#soft_reset), [`I2C_SLAVE__DEVICE_ADDRESS`](#i2c_slave__device_address), [`OSC_FREQUENCY`](#osc_frequency), [`VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`](#vhv_config__timeout_macrop_loop_bound), [`XTALK_PLANE_OFFSET_KCPS`](#xtalk_plane_offset_kcps), [`XTALK_X_PLANE_GRADIENT_KCPS`](#xtalk_x_plane_gradient_kcps), [`XTALK_Y_PLANE_GRADIENT_KCPS`](#xtalk_y_plane_gradient_kcps), [`RANGE_OFFSET_MM`](#range_offset_mm), [`INNER_OFFSET_MM`](#inner_offset_mm), [`OUTER_OFFSET_MM`](#outer_offset_mm), [`GPIO_HV_MUX__CTRL`](#gpio_hv_mux__ctrl), [`GPIO__TIO_HV_STATUS`](#gpio__tio_hv_status), [`SYSTEM__INTERRUPT`](#system__interrupt), [`RANGE_CONFIG_A`](#range_config_a), [`RANGE_CONFIG_B`](#range_config_b), [`RANGE_CONFIG__SIGMA_THRESH`](#range_config__sigma_thresh), [`MIN_COUNT_RATE_RTN_LIMIT_MCPS`](#min_count_rate_rtn_limit_mcps), [`INTERMEASUREMENT_MS`](#intermeasurement_ms), [`THRESH_HIGH`](#thresh_high), [`THRESH_LOW`](#thresh_low), [`SYSTEM__INTERRUPT_CLEAR`](#system__interrupt_clear), [`SYSTEM_START`](#system_start), [`RESULT__RANGE_STATUS`](#result__range_status), [`RESULT__SPAD_NB`](#result__spad_nb), [`RESULT__SIGNAL_RATE`](#result__signal_rate), [`RESULT__AMBIENT_RATE`](#result__ambient_rate), [`RESULT__SIGMA`](#result__sigma), [`RESULT__DISTANCE`](#result__distance), [`RESULT__OSC_CALIBRATE_VAL`](#result__osc_calibrate_val), [`FIRMWARE__SYSTEM_STATUS`](#firmware__system_status), [`IDENTIFICATION__MODEL_ID`](#identification__model_id), [`MODEL_ID_VL53L4CD`](#model_id_vl53l4cd), [`WINDOW_BELOW`](#window_below), [`WINDOW_ABOVE`](#window_above), [`WINDOW_OUT`](#window_out), [`WINDOW_IN`](#window_in), [`CONFIG_ADDR`](#config_addr), [`CONFIG_END`](#config_end), [`DEFAULT_CONFIGURATION`](#default_configuration), [`CONFIG_FMP_BYTE`](#config_fmp_byte), [`config_block`](#config_block), [`RESULT_BLOCK_ADDR`](#result_block_addr), [`RESULT_BLOCK_LEN`](#result_block_len), [`I2C_KHZ_BOOT`](#i2c_khz_boot), [`I2C_KHZ_DEFAULT`](#i2c_khz_default), [`STATUS_RTN`](#status_rtn), [`range_status_name`](#range_status_name), [`Vl53l4Error`](#vl53l4error), [`Vl53l4Results`](#vl53l4results), [`parse_result_block`](#parse_result_block), [`check_range_timing`](#check_range_timing), [`range_timing_registers`](#range_timing_registers), [`decode_range_timing`](#decode_range_timing), [`try_offset_raw`](#try_offset_raw), [`decode_offset`](#decode_offset), [`try_xtalk_raw`](#try_xtalk_raw), [`decode_xtalk`](#decode_xtalk), [`signal_threshold_raw`](#signal_threshold_raw), [`decode_signal_threshold`](#decode_signal_threshold), [`sigma_threshold_raw`](#sigma_threshold_raw), [`decode_sigma_threshold`](#decode_sigma_threshold)
- **VL53L8 (ToF)**: [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`max_ranging_frequency_hz`](#max_ranging_frequency_hz), [`BridgeUld`](#bridgeuld), [`Vl53l8cxDevice`](#vl53l8cxdevice), [`Vl53l8chDevice`](#vl53l8chdevice), [`DIST_MM`](#dist_mm), [`SIGNAL_PER_SPAD_KCPS`](#signal_per_spad_kcps), [`RANGE_SIGMA_MM`](#range_sigma_mm), [`AMBIENT_PER_SPAD_KCPS`](#ambient_per_spad_kcps), [`NB_TARGET_DETECTED`](#nb_target_detected), [`TAR_STATUS`](#tar_status), [`NB_SPADS_ENABLED`](#nb_spads_enabled), [`MOTION_INDICATOR`](#motion_indicator), [`NB_THRESHOLDS`](#nb_thresholds), [`POWER_MODE_SLEEP`](#power_mode_sleep), [`POWER_MODE_WAKEUP`](#power_mode_wakeup), [`POWER_MODE_DEEP_SLEEP`](#power_mode_deep_sleep), [`AdvancedError`](#advancederror), [`XTALK_MARGIN_MAX_KCPS`](#xtalk_margin_max_kcps), [`try_xtalk_margin_to_raw`](#try_xtalk_margin_to_raw), [`xtalk_margin_from_raw`](#xtalk_margin_from_raw), [`MotionConfig`](#motionconfig), [`try_default_motion_config`](#try_default_motion_config), [`DetectionThreshold`](#detectionthreshold), [`DetectionThresholdBlocks`](#detectionthresholdblocks), [`try_pack_detection_thresholds`](#try_pack_detection_thresholds), [`CnhDecodeConfig`](#cnhdecodeconfig), [`CnhAggregate`](#cnhaggregate), [`CnhData`](#cnhdata), [`CnhError`](#cnherror), [`decode_cnh`](#decode_cnh), [`Vl53l8Cmd`](#vl53l8cmd), [`Vl53l8Rpt`](#vl53l8rpt), [`READ_MAX_LEN`](#read_max_len), [`CHUNK_SIZE`](#chunk_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`RESOLUTION_4X4`](#resolution_4x4), [`RESOLUTION_8X8`](#resolution_8x8), [`NB_TARGET_PER_ZONE`](#nb_target_per_zone), [`CNH_DATA_IDX`](#cnh_data_idx), [`Variant`](#variant), [`Vl53l8Error`](#vl53l8error), [`Vl53l8Results`](#vl53l8results), [`swap_buffer`](#swap_buffer), [`parse_frame`](#parse_frame), [`Vl53l8CnhSetup`](#vl53l8cnhsetup), [`Vl53l8Motion`](#vl53l8motion), [`Vl53l8Frame`](#vl53l8frame), [`RANGING_MODE_CONTINUOUS`](#ranging_mode_continuous), [`RANGING_MODE_AUTONOMOUS`](#ranging_mode_autonomous), [`TARGET_ORDER_CLOSEST`](#target_order_closest), [`TARGET_ORDER_STRONGEST`](#target_order_strongest), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`STREAM_TOTAL_MAX`](#stream_total_max), [`FrameChunk`](#framechunk), [`unpack_frame_chunk`](#unpack_frame_chunk), [`CompletedFrame`](#completedframe), [`FrameReassembler`](#framereassembler)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`Vl53l7cxDevice`](#vl53l7cxdevice), [`Vl53l5cxDevice`](#vl53l5cxdevice), [`Vl53l7chDevice`](#vl53l7chdevice), [`Vl53l7Cmd`](#vl53l7cmd), [`Vl53l7Rpt`](#vl53l7rpt), [`PIN_LPN_OFF`](#pin_lpn_off), [`PIN_LPN_ON`](#pin_lpn_on), [`PIN_I2C_RST`](#pin_i2c_rst), [`PIN_SOFT_CYCLE`](#pin_soft_cycle), [`READ_MAX_LEN`](#read_max_len), [`WRITE_MAX_LEN`](#write_max_len), [`STREAM_CHUNK_MAX`](#stream_chunk_max), [`INFO_SIZE`](#info_size), [`I2C_SPEED_STEPS_KHZ`](#i2c_speed_steps_khz), [`i2c_error_name`](#i2c_error_name), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_pin_ctrl`](#pack_pin_ctrl), [`pack_set_i2c_speed`](#pack_set_i2c_speed), [`Vl53l7Info`](#vl53l7info), [`MIN_RANGING_FREQUENCY_HZ`](#min_ranging_frequency_hz), [`Vl53l7Model`](#vl53l7model), [`resolve_model`](#resolve_model), [`MODULE_TYPE_MZ`](#module_type_mz), [`MODULE_TYPE_MZEVO`](#module_type_mzevo)
- **VL53L0X / L1CX / L1CB / L3CX / L4CX (ToF)**: [`PLOTTABLE_STATUSES`](#plottable_statuses), [`Vl53lxMeasurement`](#vl53lxmeasurement), [`Vl53lxProductInfo`](#vl53lxproductinfo), [`Vl53lxDevice`](#vl53lxdevice), [`Vl53l0xDevice`](#vl53l0xdevice), [`Vl53l1cxDevice`](#vl53l1cxdevice), [`Vl53l1cbDevice`](#vl53l1cbdevice), [`Vl53l3cxDevice`](#vl53l3cxdevice), [`Vl53l4cxDevice`](#vl53l4cxdevice), [`HistogramBinData`](#histogrambindata), [`Vl53lxCmd`](#vl53lxcmd), [`Vl53lxRpt`](#vl53lxrpt), [`CLEAR_STEPS_MAX`](#clear_steps_max), [`INFO_SIZE`](#info_size), [`pack_set_addr_width`](#pack_set_addr_width), [`pack_start_stream`](#pack_start_stream), [`Vl53lxInfo`](#vl53lxinfo), [`DIE_BLOCK_ADDR`](#die_block_addr), [`DIE_BLOCK_LEN`](#die_block_len), [`L0X_BLOCK_ADDR`](#l0x_block_addr), [`L0X_BLOCK_LEN`](#l0x_block_len), [`HISTOGRAM_BLOCK_ADDR`](#histogram_block_addr), [`HISTOGRAM_BLOCK_LEN`](#histogram_block_len), [`HISTOGRAM_BINS`](#histogram_bins), [`DieVariant`](#dievariant), [`ShortBlock`](#shortblock), [`DieResult`](#dieresult), [`decode_die_block`](#decode_die_block), [`L0xRaw`](#l0xraw), [`decode_l0x_raw`](#decode_l0x_raw), [`HistogramRaw`](#histogramraw), [`decode_histogram_raw`](#decode_histogram_raw), [`Vl53lxTarget`](#vl53lxtarget), [`ExtraValue`](#extravalue), [`Vl53lxRange`](#vl53lxrange), [`Thresholds`](#thresholds), [`DriverKind`](#driverkind), [`DRIVER_KINDS`](#driver_kinds), [`BusParams`](#busparams), [`Product`](#product), [`PRODUCTS`](#products), [`product`](#product), [`product_from_board_name`](#product_from_board_name), [`Vl53lxClass`](#vl53lxclass), [`resolve_product`](#resolve_product), [`resolve_class`](#resolve_class)
- **BNO086 (IMU)**: [`Bno086Cmd`](#bno086cmd), [`RPT_DATA`](#rpt_data), [`BUSY_BACKOFF_MS`](#busy_backoff_ms), [`Bno086Data`](#bno086data), [`FORCE_SENSOR_FLUSH`](#force_sensor_flush), [`EXECUTABLE_RESET_COMPLETE`](#executable_reset_complete), [`CMD_ERRORS`](#cmd_errors), [`CMD_COUNTER`](#cmd_counter), [`CMD_TARE`](#cmd_tare), [`CMD_INITIALIZE`](#cmd_initialize), [`CMD_SAVE_DCD`](#cmd_save_dcd), [`CMD_ME_CALIBRATE`](#cmd_me_calibrate), [`CMD_PERIODIC_DCD_CONFIG`](#cmd_periodic_dcd_config), [`CMD_GET_OSCILLATOR_TYPE`](#cmd_get_oscillator_type), [`CMD_CLEAR_DCD_AND_RESET`](#cmd_clear_dcd_and_reset), [`CMD_INTERACTIVE_CALIBRATION`](#cmd_interactive_calibration), [`OSC_INTERNAL`](#osc_internal), [`OSC_EXT_CRYSTAL`](#osc_ext_crystal), [`OSC_EXT_CLOCK`](#osc_ext_clock), [`ERROR_SOURCE_NO_MORE_ERRORS`](#error_source_no_more_errors), [`TARE_X`](#tare_x), [`TARE_Y`](#tare_y), [`TARE_Z`](#tare_z), [`TARE_ALL`](#tare_all), [`TareBasis`](#tarebasis), [`MotionIntent`](#motionintent), [`FRS_STATIC_CALIBRATION_AGM`](#frs_static_calibration_agm), [`FRS_NOMINAL_CALIBRATION`](#frs_nominal_calibration), [`FRS_DYNAMIC_CALIBRATION`](#frs_dynamic_calibration), [`FRS_ME_POWER_MGMT`](#frs_me_power_mgmt), [`FRS_SYSTEM_ORIENTATION`](#frs_system_orientation), [`FRS_ACCEL_ORIENTATION`](#frs_accel_orientation), [`FRS_GYROSCOPE_ORIENTATION`](#frs_gyroscope_orientation), [`FRS_MAGNETOMETER_ORIENTATION`](#frs_magnetometer_orientation), [`FRS_ARVR_STABILIZATION_RV`](#frs_arvr_stabilization_rv), [`FRS_ARVR_STABILIZATION_GRV`](#frs_arvr_stabilization_grv), [`FRS_SERIAL_NUMBER`](#frs_serial_number), [`FRS_USER_RECORD`](#frs_user_record), [`FRS_SIG_MOTION_DETECT_CONFIG`](#frs_sig_motion_detect_config), [`FRS_SHAKE_DETECT_CONFIG`](#frs_shake_detect_config), [`FRS_STABILITY_DETECTOR_CONFIG`](#frs_stability_detector_config), [`FRS_ACTIVITY_TRACKER_CONFIG`](#frs_activity_tracker_config), [`FRS_GYRO_INTEGRATED_RV_CONFIG`](#frs_gyro_integrated_rv_config), [`METADATA_RECORDS`](#metadata_records), [`FRS_NO_ERROR`](#frs_no_error), [`FRS_UNRECOGNIZED_FRS_TYPE`](#frs_unrecognized_frs_type), [`FRS_BUSY`](#frs_busy), [`FRS_READ_COMPLETED`](#frs_read_completed), [`FRS_OFFSET_OUT_OF_RANGE`](#frs_offset_out_of_range), [`FRS_RECORD_EMPTY`](#frs_record_empty), [`FRS_BLOCK_COMPLETED`](#frs_block_completed), [`FRS_BLOCK_AND_READ_COMPLETED`](#frs_block_and_read_completed), [`FRS_DEVICE_ERROR`](#frs_device_error), [`FRS_WRITE_WORDS_RECEIVED`](#frs_write_words_received), [`FRS_WRITE_UNRECOGNIZED_FRS_TYPE`](#frs_write_unrecognized_frs_type), [`FRS_WRITE_BUSY`](#frs_write_busy), [`FRS_WRITE_COMPLETED`](#frs_write_completed), [`FRS_WRITE_MODE_READY`](#frs_write_mode_ready), [`FRS_WRITE_FAILED`](#frs_write_failed), [`FRS_WRITE_NOT_IN_WRITE_MODE`](#frs_write_not_in_write_mode), [`FRS_WRITE_INVALID_LENGTH`](#frs_write_invalid_length), [`FRS_WRITE_RECORD_VALID`](#frs_write_record_valid), [`FRS_WRITE_RECORD_INVALID`](#frs_write_record_invalid), [`Sh2HubError`](#sh2huberror), [`FeatureResponse`](#featureresponse), [`ProductId`](#productid), [`CommandResponse`](#commandresponse), [`FrsReadResponse`](#frsreadresponse), [`FrsWriteResponse`](#frswriteresponse), [`ErrorRecord`](#errorrecord), [`Counts`](#counts), [`CalibrationConfig`](#calibrationconfig), [`SensorMetadata`](#sensormetadata), [`COUNTS_GET`](#counts_get), [`COUNTS_CLEAR`](#counts_clear), [`ME_CAL_GET`](#me_cal_get), [`split_control_cargo`](#split_control_cargo), [`counts_get_params`](#counts_get_params), [`counts_clear_params`](#counts_clear_params), [`errors_params`](#errors_params), [`tare_now_params`](#tare_now_params), [`persist_tare_params`](#persist_tare_params), [`set_reorientation_params`](#set_reorientation_params), [`me_calibration_params`](#me_calibration_params), [`interactive_calibration_params`](#interactive_calibration_params), [`force_flush`](#force_flush), [`periodic_dcd_params`](#periodic_dcd_params), [`FrsReadSession`](#frsreadsession), [`FrsWriteSession`](#frswritesession), [`BASE_TIMESTAMP_REF`](#base_timestamp_ref), [`TIMESTAMP_REBASE`](#timestamp_rebase), [`FLUSH_COMPLETED`](#flush_completed), [`RV_ACCURACY_Q`](#rv_accuracy_q), [`GYRO_RV_ANGVEL_Q`](#gyro_rv_angvel_q), [`q_point`](#q_point), [`report_length`](#report_length), [`Report`](#report), [`Vector3Kind`](#vector3kind), [`try_parse_input_cargo`](#try_parse_input_cargo), [`try_parse_gyro_rv_cargo`](#try_parse_gyro_rv_cargo), [`parse_input_cargo`](#parse_input_cargo), [`parse_gyro_rv_cargo`](#parse_gyro_rv_cargo), [`REPORT_COMMAND_RESPONSE`](#report_command_response), [`REPORT_COMMAND_REQUEST`](#report_command_request), [`REPORT_FRS_READ_RESPONSE`](#report_frs_read_response), [`REPORT_FRS_READ_REQUEST`](#report_frs_read_request), [`REPORT_FRS_WRITE_RESPONSE`](#report_frs_write_response), [`REPORT_FRS_WRITE_DATA`](#report_frs_write_data), [`REPORT_FRS_WRITE_REQUEST`](#report_frs_write_request), [`REPORT_PRODUCT_ID_RESPONSE`](#report_product_id_response), [`REPORT_PRODUCT_ID_REQUEST`](#report_product_id_request), [`REPORT_GET_FEATURE_RESPONSE`](#report_get_feature_response), [`REPORT_SET_FEATURE_COMMAND`](#report_set_feature_command), [`REPORT_GET_FEATURE_REQUEST`](#report_get_feature_request), [`Sh2Error`](#sh2error), [`FEATURE_FLAG_WAKEUP`](#feature_flag_wakeup), [`FEATURE_FLAG_ALWAYS_ON`](#feature_flag_always_on), [`build_set_feature`](#build_set_feature), [`build_get_feature_request`](#build_get_feature_request), [`build_product_id_request`](#build_product_id_request), [`build_command_request`](#build_command_request), [`build_frs_read_request`](#build_frs_read_request), [`build_frs_write_request`](#build_frs_write_request), [`build_frs_write_data`](#build_frs_write_data), [`SHTP_HEADER_SIZE`](#shtp_header_size), [`LENGTH_MASK`](#length_mask), [`CONTINUATION_BIT`](#continuation_bit), [`NUM_CHANNELS`](#num_channels), [`MAX_TX_FRAME`](#max_tx_frame), [`ShtpChannel`](#shtpchannel), [`ExecutableCommand`](#executablecommand), [`build_sleep`](#build_sleep), [`build_power_on`](#build_power_on), [`ShtpHeader`](#shtpheader), [`ShtpError`](#shtperror), [`try_pack_shtp_header`](#try_pack_shtp_header), [`try_unpack_shtp_header`](#try_unpack_shtp_header), [`ShtpCargo`](#shtpcargo), [`try_build_frame`](#try_build_frame), [`ShtpLayer`](#shtplayer), [`RATE_LOW_FACTOR`](#rate_low_factor), [`RATE_HIGH_FACTOR`](#rate_high_factor), [`Bno086Device`](#bno086device)
- **BNO055 (IMU)**: [`Bno055Cmd`](#bno055cmd), [`Bno055Rpt`](#bno055rpt), [`XFER_MAX`](#xfer_max), [`TRIGGER_TIMER`](#trigger_timer), [`TRIGGER_INT`](#trigger_int), [`INFO_SIZE`](#info_size), [`I2C_ADDR`](#i2c_addr), [`EXPECTED_CHIP_ID`](#expected_chip_id), [`EXPECTED_ACC_ID`](#expected_acc_id), [`EXPECTED_MAG_ID`](#expected_mag_id), [`EXPECTED_GYR_ID`](#expected_gyr_id), [`TIMER_PERIOD_MIN_MS`](#timer_period_min_ms), [`TIMER_PERIOD_MAX_MS`](#timer_period_max_ms), [`try_pack_read_reg`](#try_pack_read_reg), [`pack_read_reg`](#pack_read_reg), [`try_pack_write_reg`](#try_pack_write_reg), [`pack_write_reg`](#pack_write_reg), [`pack_reset`](#pack_reset), [`pack_stop_stream`](#pack_stop_stream), [`pack_get_info`](#pack_get_info), [`try_pack_start_stream`](#try_pack_start_stream), [`pack_start_stream`](#pack_start_stream), [`RegData`](#regdata), [`StreamData`](#streamdata), [`Bno055Info`](#bno055info), [`REG_CHIP_ID`](#reg_chip_id), [`REG_PAGE_ID`](#reg_page_id), [`REG_ACC_DATA`](#reg_acc_data), [`REG_MAG_DATA`](#reg_mag_data), [`REG_GYR_DATA`](#reg_gyr_data), [`REG_EUL_DATA`](#reg_eul_data), [`REG_QUA_DATA`](#reg_qua_data), [`REG_LIA_DATA`](#reg_lia_data), [`REG_GRV_DATA`](#reg_grv_data), [`REG_TEMP`](#reg_temp), [`REG_CALIB_STAT`](#reg_calib_stat), [`REG_ST_RESULT`](#reg_st_result), [`REG_INT_STA`](#reg_int_sta), [`REG_SYS_CLK_STATUS`](#reg_sys_clk_status), [`REG_SYS_STATUS`](#reg_sys_status), [`REG_SYS_ERR`](#reg_sys_err), [`REG_UNIT_SEL`](#reg_unit_sel), [`REG_OPR_MODE`](#reg_opr_mode), [`REG_PWR_MODE`](#reg_pwr_mode), [`REG_SYS_TRIGGER`](#reg_sys_trigger), [`REG_TEMP_SOURCE`](#reg_temp_source), [`REG_AXIS_MAP_CONFIG`](#reg_axis_map_config), [`REG_AXIS_MAP_SIGN`](#reg_axis_map_sign), [`REG_SIC_MATRIX`](#reg_sic_matrix), [`REG_CALIB_PROFILE`](#reg_calib_profile), [`CALIB_PROFILE_LEN`](#calib_profile_len), [`REG1_ACC_CONFIG`](#reg1_acc_config), [`REG1_MAG_CONFIG`](#reg1_mag_config), [`REG1_GYR_CONFIG_0`](#reg1_gyr_config_0), [`REG1_GYR_CONFIG_1`](#reg1_gyr_config_1), [`REG1_ACC_SLEEP_CONFIG`](#reg1_acc_sleep_config), [`REG1_GYR_SLEEP_CONFIG`](#reg1_gyr_sleep_config), [`REG1_INT_MSK`](#reg1_int_msk), [`REG1_INT_EN`](#reg1_int_en), [`REG1_ACC_AM_THRES`](#reg1_acc_am_thres), [`REG1_GYR_AM_SET`](#reg1_gyr_am_set), [`REG1_UNIQUE_ID`](#reg1_unique_id), [`UNIQUE_ID_LEN`](#unique_id_len), [`FULL_BLOCK`](#full_block), [`QUAT_BLOCK`](#quat_block), [`SYS_TRIGGER_SELF_TEST`](#sys_trigger_self_test), [`SYS_TRIGGER_RST_SYS`](#sys_trigger_rst_sys), [`SYS_TRIGGER_RST_INT`](#sys_trigger_rst_int), [`SYS_TRIGGER_CLK_SEL`](#sys_trigger_clk_sel), [`INT_ACC_BSX_DRDY`](#int_acc_bsx_drdy), [`INT_MAG_DRDY`](#int_mag_drdy), [`INT_GYR_AM`](#int_gyr_am), [`INT_GYR_HIGH_RATE`](#int_gyr_high_rate), [`INT_GYR_DRDY`](#int_gyr_drdy), [`INT_ACC_HIGH_G`](#int_acc_high_g), [`INT_ACC_AM`](#int_acc_am), [`INT_ACC_NM`](#int_acc_nm), [`ST_ACC`](#st_acc), [`ST_MAG`](#st_mag), [`ST_GYR`](#st_gyr), [`ST_MCU`](#st_mcu), [`EXPECTED_SELF_TEST`](#expected_self_test), [`SYS_STATUS_BOOTING`](#sys_status_booting), [`OprMode`](#oprmode), [`PwrMode`](#pwrmode), [`TempSource`](#tempsource), [`UNIT_ACC_MG`](#unit_acc_mg), [`UNIT_GYR_RPS`](#unit_gyr_rps), [`UNIT_EUL_RAD`](#unit_eul_rad), [`UNIT_TEMP_F`](#unit_temp_f), [`UNIT_ORI_ANDROID`](#unit_ori_android), [`Units`](#units), [`MAG_LSB`](#mag_lsb), [`QUAT_LSB`](#quat_lsb), [`FUSION_ACCEL_LSB`](#fusion_accel_lsb), [`CalibStatus`](#calibstatus), [`CalibrationProfile`](#calibrationprofile), [`pack_sic_matrix`](#pack_sic_matrix), [`unpack_sic_matrix`](#unpack_sic_matrix), [`SIC_IDENTITY`](#sic_identity), [`AXIS_X`](#axis_x), [`AXIS_Y`](#axis_y), [`AXIS_Z`](#axis_z), [`AxisRemap`](#axisremap), [`PLACEMENTS`](#placements), [`ACC_RANGE_G`](#acc_range_g), [`ACC_BANDWIDTH_HZ`](#acc_bandwidth_hz), [`ACC_POWER_NAMES`](#acc_power_names), [`GYR_RANGE_DPS`](#gyr_range_dps), [`GYR_BANDWIDTH_HZ`](#gyr_bandwidth_hz), [`GYR_POWER_NAMES`](#gyr_power_names), [`MAG_RATE_HZ`](#mag_rate_hz), [`MAG_OPR_NAMES`](#mag_opr_names), [`MAG_POWER_NAMES`](#mag_power_names), [`AccelConfig`](#accelconfig), [`GyroConfig`](#gyroconfig), [`MagConfig`](#magconfig), [`RawBlock`](#rawblock), [`decode_block`](#decode_block), [`MODE_SWITCH_FROM_CONFIG`](#mode_switch_from_config), [`MODE_SWITCH_TO_CONFIG`](#mode_switch_to_config), [`SELF_TEST_TIME`](#self_test_time), [`BOOT_SETTLE_TIMEOUT`](#boot_settle_timeout), [`STUCK_SELF_TEST_POLLS`](#stuck_self_test_polls), [`FUSION_START_TIMEOUT`](#fusion_start_timeout), [`Bno055SystemStatus`](#bno055systemstatus), [`Bno055Sample`](#bno055sample), [`Bno055Device`](#bno055device)
- **TMF8829 (dToF)**: [`Tmf8829InitResult`](#tmf8829initresult), [`Tmf8829AppliedConfig`](#tmf8829appliedconfig), [`Tmf8829Options`](#tmf8829options), [`Tmf8829Device`](#tmf8829device), [`Tmf8829Measurement`](#tmf8829measurement), [`distance_mm`](#distance_mm), [`MeasurementBuilder`](#measurementbuilder), [`NOMINAL_Q16`](#nominal_q16), [`CLOCK_WINDOW_MIN_US`](#clock_window_min_us), [`CLOCK_WINDOW_SLIDE_US`](#clock_window_slide_us), [`EDGE_LATE_US`](#edge_late_us), [`FT_NOMINAL`](#ft_nominal), [`FT_CLOCK_RESET`](#ft_clock_reset), [`FT_EDGE_LATE`](#ft_edge_late), [`FT_STALLED`](#ft_stalled), [`FT_T1_UNSURE`](#ft_t1_unsure), [`FT_NAMES`](#ft_names), [`flag_names`](#flag_names), [`fp_subints`](#fp_subints), [`SensorClock`](#sensorclock), [`FrameTime`](#frametime), [`CaptureTimer`](#capturetimer), [`Tmf8829Cmd`](#tmf8829cmd), [`Tmf8829Rpt`](#tmf8829rpt), [`EV_INT`](#ev_int), [`EV_BAD_HEADER`](#ev_bad_header), [`READ_MAX`](#read_max), [`WRITE_MAX`](#write_max), [`CHUNK_MAX`](#chunk_max), [`INT_RESULTS`](#int_results), [`INT_MOTION`](#int_motion), [`INT_PROXIMITY`](#int_proximity), [`INT_HISTOGRAMS`](#int_histograms), [`INT_MASK_ALL`](#int_mask_all), [`SF_REF_FRAME`](#sf_ref_frame), [`REG_DATA_HEADER_SIZE`](#reg_data_header_size), [`FRAME_CHUNK_HEADER_SIZE`](#frame_chunk_header_size), [`EVENT_SIZE`](#event_size), [`INFO_SIZE`](#info_size), [`pack_read_reg`](#pack_read_reg), [`pack_write_reg`](#pack_write_reg), [`pack_start_stream`](#pack_start_stream), [`pack_stop_stream`](#pack_stop_stream), [`pack_get_info`](#pack_get_info), [`RegData`](#regdata), [`Tmf8829Info`](#tmf8829info), [`FrameChunk`](#framechunk), [`Tmf8829Event`](#tmf8829event), [`RawFrame`](#rawframe), [`FrameReassembler`](#framereassembler), [`REG_CFG`](#reg_cfg), [`CFG_SIZE`](#cfg_size), [`PRESETS`](#presets), [`PRESET_KILO_ITERATIONS`](#preset_kilo_iterations), [`DEFAULT_PERIOD_MS`](#default_period_ms), [`preset_cmd`](#preset_cmd), [`preset_kilo_iterations`](#preset_kilo_iterations), [`CfgField`](#cfgfield), [`CFG_FIELDS`](#cfg_fields), [`cfg_field`](#cfg_field), [`cfg_get`](#cfg_get), [`cfg_set`](#cfg_set), [`config_fields`](#config_fields), [`FIRMWARE_LOAD_ADDR`](#firmware_load_addr), [`FIRMWARE_IMAGE`](#firmware_image), [`FIRMWARE_LICENSE`](#firmware_license), [`firmware_image`](#firmware_image), [`firmware_words`](#firmware_words), [`PRE_HEADER_SIZE`](#pre_header_size), [`HEADER_SIZE`](#header_size), [`FOOTER_SIZE`](#footer_size), [`FRAME_HEAD`](#frame_head), [`FRAME_EOF`](#frame_eof), [`FID_MASK`](#fid_mask), [`FID_RESULTS`](#fid_results), [`FID_HISTOGRAMS`](#fid_histograms), [`FID_REF_SPAD`](#fid_ref_spad), [`LAYOUT_PEAKS`](#layout_peaks), [`LAYOUT_SIGNAL`](#layout_signal), [`LAYOUT_NOISE`](#layout_noise), [`LAYOUT_XTALK`](#layout_xtalk), [`LAYOUT_SUBFRAME`](#layout_subframe), [`FRAME_VALID`](#frame_valid), [`FRAME_ABORTED`](#frame_aborted), [`FRAME_WARNINGS`](#frame_warnings), [`FOOTER_DIST_IN_MM`](#footer_dist_in_mm), [`FIFO_TX_UNDERRUN`](#fifo_tx_underrun), [`FIFO_RX_OVERRUN`](#fifo_rx_overrun), [`fp_grid`](#fp_grid), [`fp_name`](#fp_name), [`frame_rows`](#frame_rows), [`FrameError`](#frameerror), [`FrameInfo`](#frameinfo), [`frame_size`](#frame_size), [`parse_frame`](#parse_frame), [`PixelResults`](#pixelresults), [`pixel_size`](#pixel_size), [`parse_results`](#parse_results), [`parse_results_info`](#parse_results_info), [`merge_halves`](#merge_halves), [`HistogramFrame`](#histogramframe)
- **Bootloader / firmware update**: [`HEADER_SIZE`](#header_size), [`MAGIC`](#magic), [`FwdepzHeader`](#fwdepzheader), [`FwdepzError`](#fwdepzerror), [`parse`](#parse)
- **Datasets (record & replay)**: [`THREAD_NAME`](#thread_name), [`PlayerState`](#playerstate), [`DatasetPlayer`](#datasetplayer), [`RecordSink`](#recordsink), [`Recordable`](#recordable), [`SessionRecorder`](#sessionrecorder), [`sr04`](#sr04), [`vl53l4`](#vl53l4), [`vl53l8`](#vl53l8), [`vl53l8_cnh`](#vl53l8_cnh), [`vl53lx`](#vl53lx), [`bno055`](#bno055), [`tmf8829`](#tmf8829), [`bno086_type`](#bno086_type), [`bno086`](#bno086), [`round1`](#round1), [`SCHEMA`](#schema), [`DatasetWriter`](#datasetwriter), [`DatasetError`](#dataseterror), [`Device`](#device), [`RecordValue`](#recordvalue), [`Record`](#record), [`DatasetReader`](#datasetreader)
- **Transport**: [`crc8_maxim`](#crc8_maxim), [`crc16_modbus`](#crc16_modbus), [`crc32_iso_hdlc`](#crc32_iso_hdlc), [`crc16_ccitt_false`](#crc16_ccitt_false), [`MAGIC`](#magic), [`HEADER_SIZE`](#header_size), [`MAX_PAYLOAD`](#max_payload), [`CrcType`](#crctype), [`FramingError`](#framingerror), [`payload_crc_bytes`](#payload_crc_bytes), [`build_packet`](#build_packet), [`Packet`](#packet), [`Event`](#event), [`PacketParser`](#packetparser)
- **Protocol codecs**: [`Cmd`](#cmd), [`Rpt`](#rpt), [`Status`](#status), [`SyncPinMode`](#syncpinmode), [`SyncPinPolarity`](#syncpinpolarity), [`UNSOLICITED`](#unsolicited), [`CodecError`](#codecerror), [`StatusReport`](#statusreport), [`TextReport`](#textreport), [`SyncTimeReport`](#synctimereport), [`TemperatureReport`](#temperaturereport), [`SequenceErrorReport`](#sequenceerrorreport), [`SyncPinConfig`](#syncpinconfig), [`SYNC_PIN_MIN`](#sync_pin_min), [`SYNC_PIN_MAX`](#sync_pin_max), [`pack_sync_time`](#pack_sync_time), [`PAYLOAD_CRC_TYPE_MAX`](#payload_crc_type_max), [`try_pack_set_payload_crc_type`](#try_pack_set_payload_crc_type), [`pack_set_payload_crc_type`](#pack_set_payload_crc_type), [`sync_time_offset_rtt`](#sync_time_offset_rtt), [`strip_device_string`](#strip_device_string)
- **JSON**: [`Value`](#value), [`JsonError`](#jsonerror)

## Discovery

### SENSOR_CLASS_NAMES

```rust
pub const SENSOR_CLASS_NAMES: [&str; 18] = [
    "sr04",
    "vl53l4cd",
    "vl53l8cx",
    "vl53l8ch",
    "vl53l5cx",
    "vl53l7cx",
    "vl53l7ch",
    "vl53l0x",
    "vl53l1cx",
    "vl53l1cb",
    "vl53l3cx",
    "vl53l4cx",
    "vl53lx",
    "bno086",
    "bno055",
    "tmf8829",
    "device",
    "bootloader",
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
#[non_exhaustive]
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
#[non_exhaustive]
pub enum Mode {
    App,
    Bootloader,
    #[default]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct PortEntry {
    pub port: String,
    pub serial: Option<String>,
}
```

A candidate serial port for discovery ordering.

#### PortEntry::new

```rust
pub fn new(port: impl Into<String>, serial: Option<String>) -> PortEntry
```

A port with its USB serial, if the OS reported one.

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
#[non_exhaustive]
pub enum DeviceEvent {
    /// The board saw a gap in host TX (RPT_SEQUENCE_ERROR); `reported_by_device` is always true.
    SequenceError {
        expected_seq: u8,
        received_seq: u8,
        reported_by_device: bool,
    },
    /// The host saw a gap in the board's packets (contract 01 §5 step 5): the packet still
    /// arrives, `missed` = (received − expected) mod 256 did not. Counted in `seq_gaps`.
    SeqGap {
        expected_seq: u8,
        received_seq: u8,
        missed: u8,
    },
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
    /// A TMF8829 `RPT_TMF_EVENT` that is not an expected stop artefact: a MOTION / PROXIMITY
    /// interrupt (code 0) or a refused frame header (code 1 — the sensor is lost, run
    /// [`Tmf8829Device::init`](super::Tmf8829Device::init) again).
    Tmf8829Interrupt(Tmf8829Event),
}
```

Something the device or the link reports outside any request (contract 07 §4).

### LinkStats

```rust
#[non_exhaustive]
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
    /// Callbacks that panicked on the reader thread (contract 07 §3).
    pub callback_errors: u64,
    /// Reports whose handler panicked: the packet was dropped (contract 07 §3).
    pub report_errors: u64,
}
```

Link counters (a snapshot).

### TimeSync

```rust
#[non_exhaustive]
pub struct TimeSync {
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

#### DepzDevice::open

```rust
pub fn open(port: &str, timeout: Option<Duration>) -> Result<DepzDevice>
```

Open a plain connection on the serial port `port` ([`SerialPortLink::open`]): the common
commands, no sensor class. `timeout` `None`: 200 ms. [`DepzError::DeviceLost`] when the port
does not open.

#### DepzDevice::port

```rust
pub fn port(&self) -> String
```

The link's name: the port path, or what the link wraps.

#### DepzDevice::is_closed

```rust
pub fn is_closed(&self) -> bool
```

#### DepzDevice::lost

```rust
pub fn lost(&self) -> Option<String>
```

The message of the loss that ended this device (with its cause), or `None`: still open, or
ended by the program's own `close()` (contract 07 §3).

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

#### DepzDevice::events

```rust
pub fn events(&self) -> &Stream<DeviceEvent>
```

The unsolicited / diagnostic events (contract 07 §2, [`DeviceEvent`]) as a stream:
`events().on(cb)` calls back on the reader thread, `events().subscribe(capacity)` is a
bounded drop-oldest queue, `events().dropped_counts()` its queues' drops. There is no
single report to wait for, so `events().next()` is [`DepzError::InvalidState`].

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
pub fn sync_time(&self, samples: usize) -> Result<TimeSync>
```

NTP-style time sync keeping the lowest-RTT of `samples` rounds (contract 02 §5).

#### DepzDevice::time_sync

```rust
pub fn time_sync(&self) -> Option<TimeSync>
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
#[non_exhaustive]
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
    InvalidReply(String),
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

# How the SDK calls a link

Contract 07 §3, "The host-supplied link": what a link given to [`open_on_link`] or a device
constructor has to cope with.

- `read` runs only on the device's own thread `depz-reader-<name()>`, one call at a time, back
  to back, in short slices: `timeout` is 50 ms today, the SDK's choice, and it may change.
- `write` runs on the thread of the call that sends (any program thread; the reader thread when
  a callback sends), concurrently with a blocked `read`. The SDK serializes the writes of a
  device: one at a time, one whole frame per call.
- `write` writes everything or returns an error, within a bounded time. The SDK looks at no
  count, does not retry and passes no timeout: the link picks one. A `write` that never returns
  holds the device's write lock, so every later request of that device blocks too, whatever its
  timeout.
- `close` runs on the thread that closes the device, possibly during a blocked `read` or a
  `write`, and after a loss on the reader thread and then again from the program's own
  `close()` or the drop: it must be idempotent. It must make a blocked `read` return promptly,
  and so must a `read` that starts just after it. Unlike Python, Java and C#, the device
  **waits for the reader with no limit**: a `read` that never returns hangs `close()` and the
  drop.
- `is_closed` and `name` are called from any thread.
- Every `Err` from `read` is a loss unless the program is closing the device, whichever variant
  it is, and so is a panic in `read`: the device closes, every later request fails with
  [`DepzError::DeviceLost`] carrying the error's text. A panic is not rethrown.
- An `Err` from `write` fails only that call: the device ends when `read` reports the loss.
  [`DepzError::LinkClosed`] while the program has not closed the device reaches the caller as
  [`DepzError::DeviceLost`]; any other as it is.
- A panic in `close` is dropped on the reader's path; from the program's own `close()` it
  reaches the program.

[`open_on_link`]: crate::devices::open_on_link

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
written byte must also equal the recorded tx stream ([`DepzError::ReplayMismatch`]). A damaged
capture is refused by [`ReplayLink::open`] with [`DepzError::InvalidArgument`] naming the line.

#### ReplayLink::open

```rust
pub fn open(path: impl AsRef<Path>, strict_tx: bool, realtime: bool) -> Result<ReplayLink>
```

Load `path`; `strict_tx` fails a write that differs from the recorded tx stream, `realtime`
paces rx chunks by their recorded timestamps. A damaged capture (contract 08: an empty file,
a header that is not a JSON object, an event line that is not one or whose `t` / `dir` /
`data` are not a non-negative integer / a string / hex digit pairs) is
[`DepzError::InvalidArgument`] naming the line; blank lines are skipped, any `dir` other
than `"rx"` counts as tx.

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
pub fn create(
    inner: L,
    path: impl AsRef<Path>,
    header_extra: Vec<(String, Value)>,
) -> Result<RecordingLink<L>>
```

Record `inner` into `path` (overwritten); `header_extra` adds header fields (contract 08),
e.g. `device`. Closing this link closes `inner`.

### RECORDING_SCHEMA

```rust
pub const RECORDING_SCHEMA: &str = "depz.rec/1";
```

The schema id of a `.depzrec` capture.

### DeviceInfo

```rust
#[non_exhaustive]
pub struct DeviceInfo {
    pub port: String,
    pub mode: Mode,
    /// `None` in bootloader / unknown mode.
    pub sensor_type: Option<SensorType>,
    pub software_name: String,
    pub fw_version: String,
    pub device_name: String,
    /// The device's protocol serial (GET_SERIAL).
    pub serial_number: String,
    /// The port's USB id, informative only — never filtered on.
    pub usb_vid: Option<u16>,
    pub usb_pid: Option<u16>,
    /// The USB iSerial, the ordering / selection key (contract 02 §4.3).
    pub usb_serial: Option<String>,
}
```

What the contract 02 §4.4 probe learned about one port ([`probe_port`], [`list_depz_devices`]).

#### DeviceInfo::usb_model_hint

```rust
pub fn usb_model_hint(&self) -> Option<&'static str>
```

The model the USB-id table gives the port (contract 02 §4.2), or `None`.

### AnyDevice

```rust
#[non_exhaustive]
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

### attach_sensor_class

```rust
pub fn attach_sensor_class(dev: DepzDevice, class_name: &str) -> Result<AnyDevice>
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

### probe_port

```rust
pub fn probe_port(port: &str, timeout: Option<Duration>) -> Option<DeviceInfo>
```

Probe one port: open it, run the contract 02 §4.4 probe, close. `None` when the port does not open or nothing
DEPZ-shaped answers. Probing opens the port: leave ports other software owns alone.

### list_depz_devices

```rust
pub fn list_depz_devices(
    ports: Option<&[&str]>,
    match_usb: bool,
    timeout: Option<Duration>,
) -> Result<Vec<DeviceInfo>>
```

Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With
`match_usb` only ports with a known DEPZ USB id are probed; `ports` (`None`: enumerate)
replaces enumeration with an explicit list (still USB-annotated when the OS knows the port).
[`DepzError::Io`] when the OS refuses to enumerate.

### OpenOptions

```rust
#[non_exhaustive]
pub struct OpenOptions {
    /// Exactly this port, even one with an unknown USB id (a warning on stderr).
    pub port: Option<String>,
    /// The candidate with this USB serial.
    pub serial: Option<String>,
    /// The `index`-th candidate (0-based) — of those with `serial` when both are set.
    pub index: Option<usize>,
    /// A [`list_depz_devices`] result: its port, the class its probe answers resolve to, no
    /// second probe.
    pub info: Option<DeviceInfo>,
    /// The request timeout.
    pub timeout: Option<Duration>,
}
```

Which board [`open_device`] opens (contract 02 §4.3), the C / C++ shape: `port` wins, then
`info`, else the DEPZ candidates (ports with a known DEPZ USB id, sorted by USB serial) narrowed
to `serial` and picked by `index` (0, the alphabetically smallest serial, when `None`).
`timeout` is the request timeout (`None`: 200 ms). `OpenOptions::default()` opens the first
candidate.

### open_device

```rust
pub fn open_device(opt: &OpenOptions) -> Result<AnyDevice>
```

Find, probe and open a board as its sensor class (contract 02 §4.3, §4.4; [`OpenOptions`] says
which). The probe runs on a connection of its own, which is closed; the class the answers and
the port's USB id resolve to then opens the port afresh (host sequence numbers restart at 0).
With `port` the USB id is that of the enumerated port it names under any alias
(`/dev/serial/by-id/…`, a symlink, another name of the same device node); a port whose USB id
is not a DEPZ one — or that the OS does not list — is opened too, with a warning on stderr (an
unprogrammed unit, a custom setup). Without `port` only ports with a known DEPZ USB id are
candidates — never "the first port the OS lists".

[`DepzError::NoDevice`]: no candidate matches, the port does not open (the message carries the
OS error) or nothing DEPZ answered; [`DepzError::BootloaderMode`]: use the contract 06 client.

### DEFAULT_BAUD_RATE

```rust
pub const DEFAULT_BAUD_RATE: u32 = 115_200;
```

The nominal baud rate the port opens with (115200 8N1, as the other SDKs); the USB CDC boards
ignore it.

### SerialPortInfo

```rust
#[non_exhaustive]
pub struct SerialPortInfo {
    /// The path (`/dev/ttyACM0`, `COM5`, `/dev/cu.usbmodem…`).
    pub port: String,
    pub vid: Option<u16>,
    pub pid: Option<u16>,
    /// The USB iSerial — the flashed serial on production units (contract 02 §4.3).
    pub usb_serial: Option<String>,
}
```

One serial port the OS lists: its path, and its USB ids and iSerial when it is a USB device
(else `None`).

#### SerialPortInfo::is_depz

```rust
pub fn is_depz(&self) -> bool
```

The port's USB id is a known DEPZ (or dev-default) one (contract 02 §4.2).

#### SerialPortInfo::usb_model_hint

```rust
pub fn usb_model_hint(&self) -> Option<&'static str>
```

The model the USB-id table gives the port, or `None`. Informational: the probe decides.

### list_serial_ports

```rust
pub fn list_serial_ports() -> Result<Vec<SerialPortInfo>>
```

Every serial port the OS lists, with its USB identity where known. [`DepzError::Io`] when the
OS refuses to enumerate.

### find_port

```rust
pub fn find_port<'a>(ports: &'a [SerialPortInfo], path: &str) -> Option<&'a SerialPortInfo>
```

The enumerated port `path` names, under any alias (contract 02 §4.3, ERRATA A5): the one whose
path is the same string, else — on POSIX — the one whose resolved path is the same
(`/dev/serial/by-id/…` is a symlink to the node), else the one that is the same character
device under another name. On Windows a COM name is matched by string only: a COM port is not
a file, and resolving one (canonicalize, metadata) opens it — a DTR pulse on every board of the
bench. `None` when the OS does not list the port.

### by_serial

```rust
pub fn by_serial(mut ports: Vec<SerialPortInfo>) -> Vec<SerialPortInfo>
```

Ports by USB iSerial ascending, those without one last, ties by path (contract 02 §4.3).

### list_depz_ports

```rust
pub fn list_depz_ports() -> Result<Vec<SerialPortInfo>>
```

The DEPZ candidates: every port with a known DEPZ USB id, by USB serial (contract 02 §4.3).

### SerialPortLink

```rust
pub struct SerialPortLink {
    reader: Mutex<Option<Box<dyn SerialPort>>>,
    writer: Mutex<Option<Box<dyn SerialPort>>>,
    name: String,
    closed: AtomicBool,
    read_timeout: Mutex<Option<Duration>>,
}
```

A serial port as a [`Link`]: 115200 8N1 with DTR and RTS raised, opened exclusively (a second
opener of a CDC-ACM tty would silently share the byte stream). Reads block up to their timeout,
so [`Link::close`] returns within one read slice; a lost port fails the next read or write with
[`DepzError::DeviceLost`] and the link is closed from then on.

#### SerialPortLink::open

```rust
pub fn open(port: &str) -> Result<SerialPortLink>
```

Open `port` at [`DEFAULT_BAUD_RATE`]. [`DepzError::DeviceLost`] when it does not open (no
such port, busy, no permission).

#### SerialPortLink::open_with_baud_rate

```rust
pub fn open_with_baud_rate(port: &str, baud_rate: u32) -> Result<SerialPortLink>
```

[`SerialPortLink::open`] at `baud_rate`.

#### SerialPortLink::port

```rust
pub fn port(&self) -> &str
```

The port path.

### Stream

```rust
pub struct Stream<T> {
    inner: Arc<Inner<T>>,
}
```

One kind of item a device emits, as a stream (contract 07 §3): `measurements` (SR04,
VL53L4CD, the VL53Lx family, TMF8829), `frames` (VL53L8CX/CH, L5CX, L7CX/CH), `samples`
(BNO055), `reports` (BNO086) and `events` (every device) are `Stream`s owned by the device.
[`Stream::on`] calls back on the reader thread and returns the [`Hook`] that unregisters;
[`Stream::subscribe`] is a bounded drop-oldest pull queue ([`Subscription`]);
[`Stream::next`] waits for one item; [`Stream::dropped_counts`] and [`Stream::parse_errors`]
are its counters. Items reach every callback, then every queue, in arrival order; the stream
ends with the device.

#### Stream::on

```rust
pub fn on(&self, callback: impl Fn(&T) + Send + Sync + 'static) -> Hook
```

Call `callback` for every item (on the reader thread: keep it short, never block, never
close the device from it). Drop the hook to stop.

#### Stream::on_where

```rust
pub fn on_where(
    &self,
    callback: impl Fn(&T) + Send + Sync + 'static,
    accept: impl Fn(&T) -> bool + Send + Sync + 'static,
) -> Hook
```

[`Stream::on`] for the items `accept` keeps (a BNO086 sensor-id filter, say).

#### Stream::subscribe

```rust
pub fn subscribe(&self, capacity: usize) -> Subscription<T>
```

A pull queue of up to `capacity` items (bounded, drop-oldest), subscribed when this
returns: nothing emitted after it is missed.

#### Stream::subscribe_where

```rust
pub fn subscribe_where(
    &self,
    capacity: usize,
    accept: impl Fn(&T) -> bool + Send + Sync + 'static,
) -> Subscription<T>
```

[`Stream::subscribe`] for the items `accept` keeps.

#### Stream::next

```rust
pub fn next(&self, timeout: Option<Duration>) -> Result<T>
```

The next item, on a one-item queue subscribed for this call (contract 07 §3); `timeout`
`None`: 2 s. [`DepzError::Timeout`] naming the stream's report when nothing arrived; at
once when the device has ended, or as soon as it ends while waiting (an item already
queued still wins), [`DepzError::LinkClosed`] after the program's own `close()` and
[`DepzError::DeviceLost`] with the cause after a loss (contract 07 §3). The events stream has no single report to wait for: there the call is
[`DepzError::InvalidState`].

#### Stream::dropped_counts

```rust
pub fn dropped_counts(&self) -> Vec<u64>
```

The drop counter of every live queue ([`Subscription::dropped_count`]); a dropped
subscription leaves the list.

#### Stream::parse_errors

```rust
pub fn parse_errors(&self) -> u64
```

Reports of this stream dropped because they did not decode (contract 07 §2): shorter
than their header, or a value the decoder refused. Monotonic over the device's life, except
where a sensor contract resets it (TMF8829 `start()`).

### Subscription

```rust
pub struct Subscription<T> {
    queue: Arc<Queue<T>>,
    stream: Weak<Inner<T>>,
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
arrived; with the queue empty and the device ended, [`DepzError::LinkClosed`] after the
program's own `close()` and [`DepzError::DeviceLost`] with the cause after a loss
(contract 07 §3).

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
#[non_exhaustive]
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

HC-SR04 ultrasonic ranger (contract 03). Measurements stream through
[`Sr04Device::measurements`]: loop samples and the single shots an AUX SYNC_IN edge triggers.
Derefs to [`DepzDevice`] for the common commands.

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

#### Sr04Device::measurements

```rust
pub fn measurements(&self) -> &Stream<Sr04Measurement>
```

The measurement stream (contract 07 §3): `measurements().on(cb)` calls back on the reader
thread (don't block), `measurements().subscribe(capacity)` is a bounded drop-oldest queue
subscribed at the call, `measurements().next(timeout)` waits for one; `dropped_counts()`
and `parse_errors()` (RPT_DATA shorter than its 11 bytes) are its counters.

### Sr04Cmd

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

unstable — not covered by the 1.0 freeze.

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

Hardware reset through XSHUT (≤ 20 ms on the MCU, typ. ~2 ms); init again.

#### Vl53l4cdDevice::bridge_info

```rust
pub fn bridge_info(&self) -> Result<Vl53l4Info>
```

RPT_VL53_INFO: sensor identity, pin levels and bridge counters (`i2c_errors` is cleared by a successful XSHUT reset). Safe while streaming.

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
pub fn set_detection_thresholds(
    &self,
    distance_low_mm: u16,
    distance_high_mm: u16,
    window: u8,
) -> Result<()>
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

#### Vl53l4cdDevice::measurements

```rust
pub fn measurements(&self) -> &Stream<Vl53l4Measurement>
```

The measurement stream (contract 07 §3): `measurements().on(cb)` calls back on the reader
thread, `measurements().subscribe(capacity)` is a bounded drop-oldest queue subscribed at
the call, `measurements().next(timeout)` waits for one; `dropped_counts()` and
`parse_errors()` (a result block that did not decode) are its counters.

### Vl53l4Cmd

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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

### try_pack_read_reg

```rust
pub fn try_pack_read_reg(addr: u16, len: u16) -> Result<[u8; 4], CodecError>
```

`VL53_READ_REG` payload: `addr u16, len u16` with the transfer limit of
contract 10 §1 — `len` 1..=[`XFER_MAX`] and `addr + len <= 0x10000` —
refused before anything is written (contract 07 §3).

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u16, len: u16) -> [u8; 4]
```

`VL53_READ_REG` payload: `addr u16, len u16` (little-endian), unchecked —
prefer [`try_pack_read_reg`].

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u16, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`VL53_WRITE_REG` payload: `addr u16` followed by 1..=[`XFER_MAX`] raw
register bytes with `addr + N <= 0x10000`; anything else is refused
(contract 07 §3).

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
#[non_exhaustive]
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
#[non_exhaustive]
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
    /// 1 = MODEL_ID read back as 0xEBAA on this GET_INFO.
    pub initialized: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
}
```

A decoded `RPT_VL53_INFO` report — bridge diagnostics. Counters wrap
silently; watch increments, not absolute values. `i2c_errors` counts since
the last successful XSHUT reset (the bridge clears it there), not since
power-up.

#### Vl53l4Info::unpack

```rust
pub fn unpack(payload: &[u8]) -> Result<Vl53l4Info, CodecError>
```

### StreamData

```rust
#[non_exhaustive]
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

### DetectionThresholds

```rust
#[non_exhaustive]
pub struct DetectionThresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}
```

The distance-window interrupt: low and high distance, mm, and the window
([`uld::WINDOW_BELOW`] … [`uld::WINDOW_IN`]).

#### DetectionThresholds::new

```rust
pub fn new(distance_low_mm: u16, distance_high_mm: u16, window: u8) -> DetectionThresholds
```

A window from its three fields.

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
#[non_exhaustive]
pub enum Vl53l4Error {
    /// Result block shorter than `RESULT_BLOCK_LEN` (17) bytes (contract 10 §5).
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

#### Vl53l4Error::is_argument

```rust
pub fn is_argument(&self) -> bool
```

A refused argument (contract 07 §3 `invalid_argument`), not a value the
sensor produced: the budget, the period, the tuning values.

### Vl53l4Results

```rust
#[non_exhaustive]
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

### check_range_timing

```rust
pub fn check_range_timing(
    timing_budget_ms: u32,
    inter_measurement_ms: u32,
) -> Result<(), Vl53l4Error>
```

`SetRangeTiming`'s own argument checks (ST's `VL53L4CD_ERROR_INVALID_ARGUMENT`),
which need no register: budget 10..=200 ms
([`Vl53l4Error::TimingBudgetOutOfRange`]), period 0 (continuous) or above
the budget (autonomous; [`Vl53l4Error::InterMeasurementInvalid`]) —
contract 10 §5.

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

Rejected (contract 07 §3, never wrapped): the arguments
[`check_range_timing`] refuses, an `osc_frequency` of 0 or so
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

### max_ranging_frequency_hz

```rust
pub fn max_ranging_frequency_hz(resolution: usize) -> i32
```

Highest ranging frequency at `resolution` zones (contract 11 §3; the same
silicon limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.

### BridgeUld

```rust
pub type BridgeUld = Vl53l8Driver<Arc<dyn Vl53l8Platform>>;
```

The ULD driver a VL53L8 / L5 / L7 device runs ([`Vl53l8cxDevice::with_uld`]):
unstable — not covered by the 1.0 freeze.

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

unstable — not covered by the 1.0 freeze.

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
pub fn init_with_progress(
    &self,
    progress: &mut dyn FnMut(&str),
    write_progress: Option<WriteProgress>,
) -> Result<()>
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

[`crate::vl53l8::RANGING_MODE_CONTINUOUS`] or [`crate::vl53l8::RANGING_MODE_AUTONOMOUS`].

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

[`crate::vl53l8::TARGET_ORDER_CLOSEST`] or [`crate::vl53l8::TARGET_ORDER_STRONGEST`].

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
pub fn get_xtalk_margin(&self) -> Result<u32>
```

Xtalk margin, whole kcps/SPAD (as ST).

#### Vl53l8cxDevice::set_xtalk_margin

```rust
pub fn set_xtalk_margin(&self, margin_kcps: u32) -> Result<()>
```

Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); above is refused.

#### Vl53l8cxDevice::calibrate_xtalk

```rust
pub fn calibrate_xtalk(
    &self,
    reflectance_percent: i32,
    nb_samples: i32,
    distance_mm: i32,
) -> Result<()>
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
pub fn configure_motion_indicator(
    &self,
    distance_mm: Option<(i32, i32)>,
) -> Result<MotionConfig>
```

Enable the motion indicator over [min, max] mm (`None`: 400–1500); every frame then carries
[`Vl53l8Frame::motion`]. Returns the configuration for further tuning.

On CH firmware the motion indicator and CNH share one DCI struct (0xBFAC): this disarms CNH
(`cnh_setup()` → `None`, no [`Vl53l8Frame::cnh_raw`]).

#### Vl53l8cxDevice::start_ranging

```rust
pub fn start_ranging(&self) -> Result<()>
```

Program the output list, start the sensor and the board's stream.

#### Vl53l8cxDevice::stop_ranging

```rust
pub fn stop_ranging(&self) -> Result<()>
```

#### Vl53l8cxDevice::frames

```rust
pub fn frames(&self) -> &Stream<Vl53l8Frame>
```

The frame stream (contract 07 §3): `frames().on(cb)` calls back on the reader thread (or
the thread of a call that held the driver while the frame came in),
`frames().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call,
`frames().next(timeout)` waits for one; `dropped_counts()` and `parse_errors()` (a chunk
shorter than its header, a frame that did not parse — not reassembly gaps) are its counters.
A callback may take the ULD (`with_uld`); on the reader thread a bus request
(`get_resolution`, `get_ranging_mode`, …) times out — the reader that would deliver its
answer is in the callback — while on the thread of a call that held the ULD it goes through.

#### Vl53l8cxDevice::reassembler_discards

```rust
pub fn reassembler_discards(&self) -> u64
```

Chunked frames the reassembler discarded (gaps, offset errors).

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
[`Vl53l8CnhSetup::decode_config`]. CNH and the motion indicator share one DCI struct
(0xBFAC): this disarms the motion indicator (frames carry no [`Vl53l8Frame::motion`]).

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
#[non_exhaustive]
pub enum AdvancedError {
    /// Motion indicator resolution other than 16 (4×4) or 64 (8×8).
    InvalidResolution(usize),
    /// Xtalk margin above 10000 kcps.
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
pub const XTALK_MARGIN_MAX_KCPS: u32 = 10000;
```

Largest xtalk margin `set_xtalk_margin` accepts, whole kcps/SPAD (as ST).

### try_xtalk_margin_to_raw

```rust
pub fn try_xtalk_margin_to_raw(margin_kcps: u32) -> Result<u32, AdvancedError>
```

Xtalk margin, a whole number of kcps/SPAD (as ST) → raw DCI value
`kcps * 2048`, as `set_xtalk_margin` writes it: 0..=[`XTALK_MARGIN_MAX_KCPS`],
else [`AdvancedError::XtalkMarginOutOfRange`].

### xtalk_margin_from_raw

```rust
pub fn xtalk_margin_from_raw(raw: u32) -> u32
```

Inverse: raw DCI value → whole kcps/SPAD (`raw / 2048`, integer division as ST).

### MotionConfig

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

#### CnhDecodeConfig::new

```rust
pub fn new(nb_of_aggregates: usize, feature_length: usize) -> CnhDecodeConfig
```

The two numbers the sensor was armed with.

### CnhAggregate

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub enum CnhError {
    /// `raw` is too short for the header or the computed block extends past it.
    Truncated,
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
sole buffer. As the reference, an empty configuration (no aggregates or no
bins) decodes to empty histograms rather than an error.

### Vl53l8Cmd

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
pub fn pack_start_stream(frame_size: u16) -> Result<[u8; 2], CodecError>
```

`VL53_START_STREAM` payload: `frame_size u16`, which must be
1..=[`STREAM_TOTAL_MAX`] (contract 04 §4 rule 4 — the firmware refuses
anything else too).

### RegData

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

### Vl53l8Motion

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
    chunk_max: usize,
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

A reassembler for the VL53L8 board's [`STREAM_CHUNK_MAX`] chunks.

#### FrameReassembler::with_chunk_max

```rust
pub fn with_chunk_max(chunk_max: usize) -> Self
```

A reassembler for a board whose `VL53_STREAM_FRAME_MAX` is `chunk_max`
(`vl53l7::STREAM_CHUNK_MAX` = 1536 on the I2C board, contract 11 §3).

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
SOFT_CYCLE drop the sensor's state: `init` again afterwards. An action outside 0..3 is
[`DepzError::InvalidArgument`] before the wire.

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

Arm CNH for the next start (as [`super::Vl53l8chDevice::configure_cnh`]); disarms the motion
indicator (one DCI block).

### Vl53l7Cmd

```rust
#[non_exhaustive]
pub enum Vl53l7Cmd {
    PinCtrl = 0x34,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}
```

Host→device opcodes the L5/L7 bridge adds on top of the VL53L8 set.

### Vl53l7Rpt

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

The board's device name, e.g. `DEPZ ToF Sensor VL53L4CX USB v2.1 TEXAMPLE03` (asked once).

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

unstable — not covered by the 1.0 freeze.

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
pub fn set_detection_thresholds(
    &self,
    distance_low_mm: u16,
    distance_high_mm: u16,
    window: u8,
) -> Result<()>
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

#### Vl53lxDevice::measurements

```rust
pub fn measurements(&self) -> &Stream<Vl53lxMeasurement>
```

The measurement stream (contract 07 §3): `measurements().on(cb)` calls back on the reader
thread (or the thread of a call that held the driver while the frame came in),
`measurements().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call
— before or after `start_ranging()` —, `measurements().next(timeout)` waits for one;
`dropped_counts()` and `parse_errors()` (a report the driver did not decode, or one
shorter than its header, before `init()` too) are its counters. A callback may take the
driver for what it knows locally (`supports`, `modes`, `budget_range`, …); a bus request
(`get_range_timing`, `identify_product`, …) from a callback on the reader thread times
out — the reader that would deliver its answer is in the callback.

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

### HistogramBinData

```rust
#[non_exhaustive]
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

### Vl53lxCmd

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct ShortBlock {
    pub need: usize,
    pub got: usize,
}
```

A block shorter than its decoder needs.

### DieResult

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

### Vl53lxTarget

```rust
#[non_exhaustive]
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
#[non_exhaustive]
pub enum ExtraValue {
    Int(i64),
    Float(f64),
}
```

A value of [`Vl53lxRange::extra`] / [`Vl53lxDriver::driver_info`]: driver-specific numbers.

### Vl53lxRange

```rust
#[non_exhaustive]
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
#[non_exhaustive]
pub struct Thresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}
```

The distance-window interrupt: `window` is one of the `WINDOW_*` constants of the die.

#### Thresholds::new

```rust
pub fn new(distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Thresholds
```

A window from its three fields.

### DriverKind

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L1)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L1CB",
        usb_pid: 0xED42,
        model_id: 0xEACC,
        reach_mm: 8000,
        default_driver: DriverKind::Uld,
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L1)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L3CX",
        usb_pid: 0xED44,
        model_id: 0xEAAA,
        reach_mm: 3000,
        default_driver: DriverKind::Ulp,
        drivers: &[
            (DriverKind::Ulp, die(DieVariant::L4)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
    },
    Product {
        name: "VL53L4CD",
        usb_pid: 0xED45,
        model_id: 0xEBAA,
        reach_mm: 1300,
        default_driver: DriverKind::Uld,
        drivers: &[
            (DriverKind::Uld, die(DieVariant::L4)),
            (DriverKind::Histogram, HISTOGRAM),
        ],
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
#[non_exhaustive]
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

### Bno086Cmd

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

### FRS_GYRO_INTEGRATED_RV_CONFIG

```rust
pub const FRS_GYRO_INTEGRATED_RV_CONFIG: u16 = 0xA1A2;
```

Reference, sync interval, max error, prediction (SH-2 §4.3.24).

### METADATA_RECORDS

```rust
pub const METADATA_RECORDS: [(u8, u16); 24] = [
    (0x14, 0xE301),
    (0x01, 0xE302),
    (0x04, 0xE303),
    (0x06, 0xE304),
    (0x15, 0xE305),
    (0x02, 0xE306),
    (0x07, 0xE307),
    (0x16, 0xE308),
    (0x03, 0xE309),
    (0x0F, 0xE30A),
    (0x05, 0xE30B),
    (0x08, 0xE30C),
    (0x09, 0xE30D),
    (0x10, 0xE313),
    (0x18, 0xE314),
    (0x11, 0xE315),
    (0x12, 0xE316),
    (0x13, 0xE317),
    (0x19, 0xE318),
    (0x1E, 0xE31C),
    (0x28, 0xE322),
    (0x29, 0xE323),
    (0x2A, 0xE324),
    (0x2B, 0xE325),
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct CalibrationConfig {
    pub accel: bool,
    pub gyro: bool,
    pub mag: bool,
    pub planar: bool,
}
```

ME calibration enables as the sensor reports them.

#### CalibrationConfig::new

```rust
pub fn new(accel: bool, gyro: bool, mag: bool, planar: bool) -> CalibrationConfig
```

Which calibrations run.

### SensorMetadata

```rust
#[non_exhaustive]
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
pub fn me_calibration_params(
    accel: bool,
    gyro: bool,
    mag: bool,
    planar: bool,
    subcommand: u8,
) -> [u8; 5]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub enum Vector3Kind {
    Acceleration,
    Gyroscope,
    Magnetometer,
}
```

Which of the plain 3-vector reports.

### try_parse_input_cargo

```rust
pub fn try_parse_input_cargo(
    payload: &[u8],
    capture_timestamp_us: i64,
) -> Result<Vec<Report>, CodecError>
```

[`parse_input_cargo`] that refuses a negative `capture_timestamp_us`: the
capture time is the MCU uptime, a u64 (contract 07 §3).

### try_parse_gyro_rv_cargo

```rust
pub fn try_parse_gyro_rv_cargo(
    payload: &[u8],
    capture_timestamp_us: i64,
) -> Result<Option<Report>, CodecError>
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct ShtpHeader {
    /// Bits 14:0 — cargo length incl. this 4-byte header.
    pub length: u16,
    pub channel: u8,
    pub seq: u8,
    pub continuation: bool,
}
```

A decoded SHTP header.

#### ShtpHeader::new

```rust
pub fn new(length: u16, channel: u8, seq: u8) -> ShtpHeader
```

A header for one cargo (`continuation` false).

### ShtpError

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
    /// Incomplete cargos and junk-channel (≥ 6) frames thrown away.
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
method (`enable`, `tare_now`, …) from a [`Bno086Device::reports`] callback.

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
pub fn product_ids(
    &self,
    timeout: Option<Duration>,
    settle: Option<Duration>,
) -> Result<Vec<ProductId>>
```

Every component's Product ID Response, the SH-2 application first. Collects until no response
came for `settle` (default 150 ms), so late ones cannot answer the next request.

#### Bno086Device::interval_us

```rust
pub fn interval_us(hz: f64) -> Result<u32>
```

Hz → the Set Feature interval, rounded like the reference (half to even), at least 1 µs. A
rate whose interval does not fit the u32 of the wire is [`DepzError::InvalidArgument`].

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

#### Bno086Device::reports

```rust
pub fn reports(&self) -> &Stream<Report>
```

The report stream (contract 07 §3): `reports().on(cb)` calls back on the reader thread,
`reports().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call,
`reports().next(timeout)` waits for one; `on_where` / `subscribe_where` keep the reports a
predicate accepts (`|r| r.sensor_id() == 0x05`, say); `dropped_counts()` and
`parse_errors()` (RPT_DATA shorter than its 9-byte header) are its counters.

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
pub fn frs_write(
    &self,
    record_id: u16,
    words: &[u32],
    timeout: Option<Duration>,
) -> Result<()>
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
#[non_exhaustive]
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
#[non_exhaustive]
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

### TIMER_PERIOD_MIN_MS

```rust
pub const TIMER_PERIOD_MIN_MS: u16 = 1;
```

`BNO_START_STREAM` TIMER period limits, ms (contract 13 §2).

### TIMER_PERIOD_MAX_MS

```rust
pub const TIMER_PERIOD_MAX_MS: u16 = 60000;
```

### try_pack_read_reg

```rust
pub fn try_pack_read_reg(addr: u8, len: u8) -> Result<[u8; 2], CodecError>
```

`BNO_READ_REG` payload: `addr u8, len u8` with the transfer limit of
contract 13 §1 — `len` 1..=[`XFER_MAX`] and `addr + len <= 0x100` —
refused before anything is written (contract 07 §3).

### pack_read_reg

```rust
pub fn pack_read_reg(addr: u8, len: u8) -> [u8; 2]
```

`BNO_READ_REG` payload: `addr u8, len u8`, unchecked — prefer
[`try_pack_read_reg`].

### try_pack_write_reg

```rust
pub fn try_pack_write_reg(addr: u8, data: &[u8]) -> Result<Vec<u8>, CodecError>
```

`BNO_WRITE_REG` payload: `addr u8, data[1..128]` with `addr + N <= 0x100`;
anything else is refused (contract 07 §3), never sent.

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

### try_pack_start_stream

```rust
pub fn try_pack_start_stream(
    trigger: u8,
    addr: u8,
    len: u8,
    period_ms: u16,
) -> Result<[u8; 5], CodecError>
```

`BNO_START_STREAM` payload with the TIMER rule of contract 13 §2: with
[`TRIGGER_TIMER`] the period is 1..=60000 ms, else refused before anything
is written; with [`TRIGGER_INT`] it is the missed-edge watchdog (0 = none).

### pack_start_stream

```rust
pub fn pack_start_stream(trigger: u8, addr: u8, len: u8, period_ms: u16) -> [u8; 5]
```

`BNO_START_STREAM` payload: `trigger u8, addr u8, len u8, period_ms u16 LE`
— 5 bytes, unchecked — prefer [`try_pack_start_stream`]. Replaces any
running stream.

### RegData

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
    /// Failed transfers since the last successful reset handshake.
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
0x00..0x06) plus bridge diagnostics. Counters wrap silently: watch
increments. `i2c_errors` is cleared by every successful reset handshake.
A rising `sensor_resets` means the bridge
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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

#### CalibrationProfile::new

```rust
pub fn new(
    accel_offset: [i16; 3],
    mag_offset: [i16; 3],
    gyro_offset: [i16; 3],
    accel_radius: i16,
    mag_radius: i16,
) -> CalibrationProfile
```

A profile from its five groups (the sensor's LSB).

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
#[non_exhaustive]
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
pub const ACC_POWER_NAMES: [&str; 6] = [
    "normal",
    "suspend",
    "low power 1",
    "standby",
    "low power 2",
    "deep suspend",
];
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
pub const GYR_POWER_NAMES: [&str; 5] = [
    "normal",
    "fast power up",
    "deep suspend",
    "suspend",
    "advanced powersave",
];
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
#[non_exhaustive]
pub struct AccelConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}
```

ACC_CONFIG (page 1, 0x08) as register codes: `range<1:0>`,
`bandwidth<4:2>`, `power<7:5>`. Default = power-on 0x0D (±4 g, 62.5 Hz,
normal).

#### AccelConfig::new

```rust
pub fn new(range: u8, bandwidth: u8, power: u8) -> AccelConfig
```

A configuration from its three register codes.

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
#[non_exhaustive]
pub struct GyroConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}
```

GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), two bytes: byte 0 `range<2:0>`
`bandwidth<5:3>`, byte 1 `power<2:0>`. Default = power-on 0x38/0x00
(2000 dps, 32 Hz, normal).

#### GyroConfig::new

```rust
pub fn new(range: u8, bandwidth: u8, power: u8) -> GyroConfig
```

A configuration from its three register codes.

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
#[non_exhaustive]
pub struct MagConfig {
    pub rate: u8,
    pub mode: u8,
    pub power: u8,
}
```

MAG_CONFIG (page 1, 0x09): `rate<2:0>`, `mode<4:3>`, `power<6:5>` (bit 7
unused — a repack clears it). Default = power-on 0x0B (10 Hz, regular,
normal).

#### MagConfig::new

```rust
pub fn new(rate: u8, mode: u8, power: u8) -> MagConfig
```

A configuration from its three register codes.

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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
pub fn start_stream(
    &self,
    period_ms: u16,
    block: Option<(u8, u8)>,
    int_trigger: bool,
) -> Result<()>
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

#### Bno055Device::samples

```rust
pub fn samples(&self) -> &Stream<Bno055Sample>
```

The sample stream (contract 07 §3): `samples().on(cb)` calls back on the reader thread,
`samples().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call,
`samples().next(timeout)` waits for one; `dropped_counts()` and `parse_errors()` (a short
block) are its counters.

## TMF8829 (dToF)

### Tmf8829InitResult

```rust
#[non_exhaustive]
pub struct Tmf8829InitResult {
    pub app_version: String,
    pub sensor_serial: u32,
}
```

What [`Tmf8829Device::init`] reports: the RAM application version and the sensor's serial number.

### Tmf8829AppliedConfig

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
through [`Tmf8829Device::measurements`], [`Tmf8829Device::stop`] ends it. The sensor's EN pin is wired high: there is no sleep, and recovery from any state is
[`Tmf8829Device::init`] (it starts with a soft reset). Closing (or dropping) the device stops a
running stream first.

#### Tmf8829Device::attach

```rust
pub fn attach(dev: DepzDevice) -> Tmf8829Device
```

Continue the open connection `dev` as a TMF8829.

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
[`DepzError::InvalidReply`].

#### Tmf8829Device::read_register

```rust
pub fn read_register(&self, addr: u8) -> Result<u8>
```

#### Tmf8829Device::write_registers

```rust
pub fn write_registers(
    &self,
    addr: u8,
    data: &[u8],
    progress: Option<&mut dyn FnMut(usize, usize)>,
) -> Result<()>
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
pub fn init(
    &self,
    progress: Option<&mut dyn FnMut(usize, usize)>,
) -> Result<Tmf8829InitResult>
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

#### Tmf8829Device::measurements

```rust
pub fn measurements(&self) -> &Stream<Tmf8829Measurement>
```

The measurement stream (contract 07 §3): `measurements().on(cb)` calls back on the reader
thread (don't block), `measurements().subscribe(capacity)` is a bounded drop-oldest queue
subscribed at the call, `measurements().next(timeout)` waits for one; `dropped_counts()`
and `parse_errors()` (a FRAME chunk or EVENT shorter than its header, reset by
[`Tmf8829Device::start`]; [`Tmf8829Device::frame_parse_errors`] adds the builder's) are
its counters.

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

Reports of this class dropped because they failed to decode: a FRAME chunk or EVENT shorter
than its header (contract 07 §2, the stream's `parse_errors()`), plus the frames the
builder could not parse. Reset by [`Tmf8829Device::start`].

#### Tmf8829Device::clock_ratio_q16

```rust
pub fn clock_ratio_q16(&self) -> u32
```

Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per
SYSTICK), for diagnostics.

### Tmf8829Measurement

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub enum FwdepzError {
    /// Too short to hold a header, or bad magic.
    Magic,
    /// Header CRC-16/CCITT-FALSE mismatch.
    HeaderCrc,
    /// `fw_size` != payload length.
    Size,
    /// The header is not section 0 of 1 (sections are 0-based; multi-section
    /// files are not supported).
    Section,
}
```

Reasons a `.fwdepz` blob fails validation, in the normative check order.

### parse

```rust
pub fn parse(file: &[u8]) -> Result<FwdepzHeader, FwdepzError>
```

Parse and validate a `.fwdepz` blob. Validation order: magic → header CRC →
`fw_size == len(payload)` → section 0 of 1 (contract 06 §2).

## Datasets (record & replay)

### THREAD_NAME

```rust
pub const THREAD_NAME: &str = "depz-dataset-player";
```

The thread name of a threaded player.

### PlayerState

```rust
#[non_exhaustive]
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
(unless `pause` was called from that callback). They must not block. A callback that panics is
counted in [`DatasetPlayer::callback_errors`], reported on stderr and dropped: the other callbacks
still get the record and playback goes on (contract 09). With `threaded = false`
no thread starts and the owner calls [`DatasetPlayer::tick`] (a GUI timer, or a test with a
fake clock). Dropping the player stops it.

#### DatasetPlayer::new

```rust
pub fn new(
    dataset: &DatasetReader,
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

#### DatasetPlayer::callback_errors

```rust
pub fn callback_errors(&self) -> u64
```

Record / state callbacks that panicked and were dropped (contract 09).

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
is due, or `None` when not playing. For `threaded = false`; harmless with a player thread.

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
    last_written: u64, // the finished file's count, after stop()
}
```

Record decoded data from several devices onto one host timeline (contract 09).
[`SessionRecorder::add`] each open device (it runs [`DepzDevice::sync_time`] and hooks the
device's result stream), then [`SessionRecorder::start`]: a record per result at device time −
offset. It never reconfigures a device — start and stop streaming yourself.
[`SessionRecorder::stop`] (or dropping it) closes the file; the hooks go with the recorder.

#### SessionRecorder::new

```rust
pub fn new(
    path: impl AsRef<Path>,
    note: Option<&str>,
    vl53l8_layers: bool,
    temperature_hz: Option<f64>,
) -> Self
```

A recorder writing `path` (`.depzdata`, or `.depzdata.gz`). `vl53l8_layers`: VL53L8 records
also carry signal / ambient / sigma / reflectance. `temperature_hz`: about this many
`temperature` records per device and second (the MCU thermometer, and the sensor die where
frames carry one); `None` for none.

#### SessionRecorder::add

```rust
pub fn add(
    &mut self,
    device: &dyn Recordable,
    device_id: Option<&str>,
    sync_samples: usize,
) -> Result<String>
```

Register a device before recording starts: runs [`DepzDevice::sync_time`] with
`sync_samples` rounds so its timestamps land on the shared timeline, and hooks its stream.
Returns its device id (`d0`, `d1`, … unless given); an id given twice is the later device's,
the earlier one unhooked.

#### SessionRecorder::records_written

```rust
pub fn records_written(&self) -> u64
```

Records in the file being written; after [`stop`](Self::stop), in the finished one.

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

Low-level thread-safe `.depzdata` record writer (gzip-compressed when the path ends in `.gz`).
Prefer [`super::SessionRecorder`]. Dropping it closes the file.

#### DatasetWriter::create

```rust
pub fn create(
    path: impl AsRef<Path>,
    devices: Vec<(String, Value)>,
    note: Option<&str>,
) -> io::Result<DatasetWriter>
```

Create `path` (overwritten) and write the header: `devices` are the header device entries by
device id (`serial`, `sensor_type`, `software_name`, `time_sync`, …), `note` free text. A
header no reader would take (see [`DatasetWriter::write`]) is [`io::ErrorKind::InvalidInput`],
before the file is touched.

#### DatasetWriter::records_written

```rust
pub fn records_written(&self) -> u64
```

#### DatasetWriter::write

```rust
pub fn write(&self, device_id: &str, t_host_us: i64, kind: &str, value: Value) -> Result<()>
```

One record: device id, host µs, kind, value. Once the writer is closed:
[`DepzError::InvalidState`]. A value that is not an object, holds a number with no JSON form
(NaN, Infinity, text that is no number) or nests past the line rules:
[`DepzError::InvalidArgument`], nothing written — no reader would take the line.

#### DatasetWriter::close

```rust
pub fn close(&self) -> io::Result<()>
```

Flush and close the file (a gzip file gets its trailer). Idempotent.

### DatasetError

```rust
#[non_exhaustive]
pub enum DatasetError {
    /// The file has no header line.
    Empty,
    /// Line `line` (1-based) was not valid JSON.
    Json { line: usize, error: JsonError },
    /// A structural expectation failed on line `line` (not an object, missing field, wrong type, …).
    Structure { line: usize, what: String },
    /// The file is damaged as a whole: a cut or corrupt gzip container, content that is no UTF-8.
    Damaged(String),
    /// The file could not be read.
    Io(String),
}
```

A .depzdata parse failure: a damaged file is the caller's bad argument (contract 09, 07 §3), named
by its line.

### Device

```rust
#[non_exhaustive]
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
#[non_exhaustive]
pub enum RecordValue {
    /// `sr04`: `echo_us` (0xFFFF = timeout), `source` = "once" | "loop".
    Sr04 { echo_us: i64, source: String },
    /// `temperature`: MCU temperature in °C.
    Temperature { celsius: f64 },
    /// Any other (or reserved) kind — the raw `v` object.
    Other(Value),
}
```

A decoded record value; unknown kinds, and known ones without their typed field, keep their
raw JSON (`Other`).

### Record

```rust
#[non_exhaustive]
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

### DatasetReader

```rust
#[non_exhaustive]
pub struct DatasetReader {
    pub schema: String,
    pub created_utc: Option<String>,
    pub note: Option<String>,
    pub devices: Vec<Device>,
    pub records: Vec<Record>,
}
```

A parsed dataset: header devices plus the record stream, stable-sorted by `t` (records of
equal `t` keep their file order).

#### DatasetReader::read

```rust
pub fn read(path: impl AsRef<Path>) -> Result<DatasetReader, DatasetError>
```

Read the `.depzdata` file at `path`; gzip content (by its magic bytes) is inflated.

#### DatasetReader::duration_us

```rust
pub fn duration_us(&self) -> i64
```

Span of the timeline: the last record's `t` minus the first's (0 with fewer than two).

#### DatasetReader::parse

```rust
pub fn parse(text: &str) -> Result<DatasetReader, DatasetError>
```

Parse a `.depzdata` document from its UTF-8 text. A damaged document (contract 09: an empty
one, a header that is not a JSON object or whose `schema` is not `depz.dataset/*`, a record
line that is not an object or whose `d` / `k` are not strings, `t` not an integer, `v` not
an object) is a [`DatasetError`] naming the line; blank lines are skipped; a record of an
unknown `k` (or a known one without its typed field) is kept as [`RecordValue::Other`].

#### DatasetReader::records_for

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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct Packet {
    pub cmd: u8,
    pub seq: u8,
    pub payload: Vec<u8>,
}
```

A decoded packet.

### Event

```rust
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
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
#[non_exhaustive]
pub struct SyncPinConfig {
    pub pin: u8,
    pub mode: u8,
    pub polarity: u8,
}
```

Sync-pin configuration (pin 1..5, mode, polarity).

#### SyncPinConfig::new

```rust
pub fn new(pin: u8, mode: u8, polarity: u8) -> SyncPinConfig
```

A configuration from its three wire fields (`mode` a [`SyncPinMode`], `polarity` a
[`SyncPinPolarity`], as `u8`).

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

### PAYLOAD_CRC_TYPE_MAX

```rust
pub const PAYLOAD_CRC_TYPE_MAX: u8 = 3;
```

`SET_PAYLOAD_CRC_TYPE` takes 0..3 (contract 02 §2).

### try_pack_set_payload_crc_type

```rust
pub fn try_pack_set_payload_crc_type(crc_type: u8) -> Result<[u8; 1], CodecError>
```

Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte); a value
outside 0..=[`PAYLOAD_CRC_TYPE_MAX`] is refused before anything is written
(contract 07 §3).

### pack_set_payload_crc_type

```rust
pub fn pack_set_payload_crc_type(crc_type: u8) -> [u8; 1]
```

Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte), unchecked
— prefer [`try_pack_set_payload_crc_type`] for an untyped value (the
device layer passes a [`crate::framing::CrcType`], always 0..3).

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
    /// Numbers are kept as their source text so integers stay exact (the one normalisation, when
    /// parsed and when written: an exponent is `e` with its sign, `1e9` → `1e+9`). Other text
    /// (say the `NaN` of [`Value::float`]) has no JSON form: [`Value::to_json`] refuses it.
    Number(String),
    String(String),
    Array(Vec<Value>),
    /// Insertion-ordered key/value pairs, each key once: a repeated key in the text keeps its last
    /// value, as the reference SDK's `json.loads` does.
    Object(Vec<(String, Value)>),
}
```

A parsed JSON value.

#### Value::parse

```rust
pub fn parse(text: &str) -> Result<Value, JsonError>
```

Parse a single strict RFC 8259 JSON value from `text` (surrounding whitespace allowed,
nothing else after the value); nested deeper than 256 is `nested deeper than 256`.

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
its `.0`, as Python writes floats). Not finite: the text `NaN` / `inf` / `-inf`, which
[`Value::to_json`] and so the writers refuse (contract 09).

#### Value::to_json

```rust
pub fn to_json(&self) -> Result<String, JsonError>
```

Compact JSON text (`", "` / `": "`-free, as `json.dumps(separators=(",", ":"))`). A
[`Value::Number`] whose text is no JSON number is the error.

### JsonError

```rust
#[non_exhaustive]
pub struct JsonError {
    pub pos: usize,
    pub msg: String,
}
```

JSON parse error: `msg` and the position `pos` it was detected at — serde_json's 1-based column
(the texts parsed are single lines); 0 when the error has no position.
