# API reference

Auto-generated from the public headers `include/depz_sensor_sdk.h`
(codecs) and `include/depz_sensor_io.h` (live layer) — their
declarations and doc-comments — by `scripts/gen_api_md.py`; run
`make docs` to regenerate. Edit the doc-comments in the headers, not
this file.

Each sensor also has a focused reference with just its own symbols:
[SR04](sr04/api.md) · [VL53L4CD](vl53l4cd/api.md) · [VL53L8CX](vl53l8cx/api.md) · [VL53L8CH](vl53l8ch/api.md) · [VL53L5CX](vl53l5cx/api.md) · [VL53L7CX](vl53l7cx/api.md) · [VL53L7CH](vl53l7ch/api.md) · [VL53L0X](vl53l0x/api.md) · [VL53L1CX](vl53l1cx/api.md) · [VL53L1CB](vl53l1cb/api.md) · [VL53L3CX](vl53l3cx/api.md) · [VL53L4CX](vl53l4cx/api.md) · [BNO086](bno086/api.md) · [BNO055](bno055/api.md).

## Contents

- **Discovery**: [`DEPZ_USB_VID`](#depz_usb_vid), [`DEPZ_PID_SR04`](#depz_pid_sr04), [`DEPZ_PID_VL53L8`](#depz_pid_vl53l8), [`DEPZ_PID_VL53L4CD`](#depz_pid_vl53l4cd), [`DEPZ_PID_BNO086`](#depz_pid_bno086), [`DEPZ_PID_RANGE_LO`](#depz_pid_range_lo), [`DEPZ_PID_RANGE_HI`](#depz_pid_range_hi), [`DEPZ_DEV_USB_VID`](#depz_dev_usb_vid), [`DEPZ_DEV_USB_PID`](#depz_dev_usb_pid), [`depz_is_known_depz_usb`](#depz_is_known_depz_usb), [`depz_usb_model_hint`](#depz_usb_model_hint), [`depz_sensor_type`](#depz_sensor_type), [`depz_identity`](#depz_identity), [`depz_sensor_type_str`](#depz_sensor_type_str), [`depz_strip_device_string`](#depz_strip_device_string), [`depz_parse_software_name`](#depz_parse_software_name)
- **Transport**: [`depz_crc8_maxim`](#depz_crc8_maxim), [`depz_crc16_modbus`](#depz_crc16_modbus), [`depz_crc32_iso_hdlc`](#depz_crc32_iso_hdlc), [`depz_crc16_ccitt_false`](#depz_crc16_ccitt_false), [`DEPZ_MAGIC0`](#depz_magic0), [`DEPZ_MAGIC1`](#depz_magic1), [`DEPZ_HEADER_SIZE`](#depz_header_size), [`DEPZ_MAX_PAYLOAD`](#depz_max_payload), [`depz_crc_type`](#depz_crc_type), [`DEPZ_MAX_FRAME`](#depz_max_frame), [`depz_build_packet`](#depz_build_packet), [`depz_event_type`](#depz_event_type), [`depz_event`](#depz_event), [`depz_event_cb`](#depz_event_cb), [`depz_parser`](#depz_parser), [`depz_parser_init`](#depz_parser_init), [`depz_parser_free`](#depz_parser_free), [`depz_parser_feed`](#depz_parser_feed)
- **Common protocol**: [`depz_cmd`](#depz_cmd), [`depz_rpt`](#depz_rpt), [`depz_status`](#depz_status), [`depz_sync_pin_mode`](#depz_sync_pin_mode), [`depz_status_report`](#depz_status_report), [`depz_text_report`](#depz_text_report), [`depz_sync_time_report`](#depz_sync_time_report), [`depz_temperature_report`](#depz_temperature_report), [`depz_sequence_error_report`](#depz_sequence_error_report), [`depz_sync_pin_config`](#depz_sync_pin_config), [`depz_pack_sync_time`](#depz_pack_sync_time), [`depz_pack_set_payload_crc_type`](#depz_pack_set_payload_crc_type), [`depz_pack_sync_pin_config`](#depz_pack_sync_pin_config), [`depz_unpack_status`](#depz_unpack_status), [`depz_unpack_text`](#depz_unpack_text), [`depz_unpack_sync_time`](#depz_unpack_sync_time), [`depz_unpack_temperature`](#depz_unpack_temperature), [`depz_unpack_sequence_error`](#depz_unpack_sequence_error), [`depz_unpack_sync_pin_config`](#depz_unpack_sync_pin_config), [`depz_sync_time_offset_rtt`](#depz_sync_time_offset_rtt)
- **SR04**: [`depz_sr04_cmd`](#depz_sr04_cmd), [`depz_sr04_rpt`](#depz_sr04_rpt), [`DEPZ_SR04_ECHO_TIMEOUT`](#depz_sr04_echo_timeout), [`DEPZ_SR04_ECHO_DECAY_MIN_US`](#depz_sr04_echo_decay_min_us), [`DEPZ_SR04_ECHO_DECAY_MAX_US`](#depz_sr04_echo_decay_max_us), [`depz_sr04_data`](#depz_sr04_data), [`depz_sr04_pack_sample_period`](#depz_sr04_pack_sample_period), [`depz_sr04_pack_echo_decay`](#depz_sr04_pack_echo_decay), [`depz_sr04_unpack_data`](#depz_sr04_unpack_data), [`depz_sr04_unpack_sample_period`](#depz_sr04_unpack_sample_period), [`depz_sr04_unpack_echo_decay`](#depz_sr04_unpack_echo_decay), [`depz_sr04_distance_mm`](#depz_sr04_distance_mm)
- **VL53L4CD (ToF)**: [`depz_vl53l4_cmd`](#depz_vl53l4_cmd), [`depz_vl53l4_rpt`](#depz_vl53l4_rpt), [`DEPZ_VL53L4_XFER_MAX`](#depz_vl53l4_xfer_max), [`DEPZ_VL53L4_XSHUT_OFF`](#depz_vl53l4_xshut_off), [`DEPZ_VL53L4_XSHUT_ON`](#depz_vl53l4_xshut_on), [`DEPZ_VL53L4_XSHUT_RESET`](#depz_vl53l4_xshut_reset), [`DEPZ_VL53L4_SF_INT_ACT_HIGH`](#depz_vl53l4_sf_int_act_high), [`DEPZ_VL53L4_RESULT_BLOCK_ADDR`](#depz_vl53l4_result_block_addr), [`DEPZ_VL53L4_RESULT_BLOCK_LEN`](#depz_vl53l4_result_block_len), [`DEPZ_VL53L4_MODEL_ID`](#depz_vl53l4_model_id), [`DEPZ_VL53L4_CONFIG_ADDR`](#depz_vl53l4_config_addr), [`DEPZ_VL53L4_CONFIG_FMP_BYTE`](#depz_vl53l4_config_fmp_byte), [`depz_vl53l4_pack_read_reg`](#depz_vl53l4_pack_read_reg), [`depz_vl53l4_pack_write_reg`](#depz_vl53l4_pack_write_reg), [`depz_vl53l4_pack_xshut`](#depz_vl53l4_pack_xshut), [`depz_vl53l4_pack_start_stream`](#depz_vl53l4_pack_start_stream), [`depz_vl53l4_pack_set_i2c_speed`](#depz_vl53l4_pack_set_i2c_speed), [`depz_vl53l4_reg_data`](#depz_vl53l4_reg_data), [`depz_vl53l4_unpack_reg_data`](#depz_vl53l4_unpack_reg_data), [`depz_vl53l4_info`](#depz_vl53l4_info), [`depz_vl53l4_unpack_info`](#depz_vl53l4_unpack_info), [`depz_vl53l4_stream`](#depz_vl53l4_stream), [`depz_vl53l4_unpack_stream`](#depz_vl53l4_unpack_stream), [`depz_vl53l4_result`](#depz_vl53l4_result), [`depz_vl53l4_parse_result_block`](#depz_vl53l4_parse_result_block), [`depz_vl53l4_range_timing_registers`](#depz_vl53l4_range_timing_registers), [`depz_vl53l4_decode_range_timing`](#depz_vl53l4_decode_range_timing), [`depz_vl53l4_offset_raw`](#depz_vl53l4_offset_raw), [`depz_vl53l4_decode_offset`](#depz_vl53l4_decode_offset), [`depz_vl53l4_xtalk_raw`](#depz_vl53l4_xtalk_raw), [`depz_vl53l4_decode_xtalk`](#depz_vl53l4_decode_xtalk), [`depz_vl53l4_signal_threshold_raw`](#depz_vl53l4_signal_threshold_raw), [`depz_vl53l4_decode_signal_threshold`](#depz_vl53l4_decode_signal_threshold), [`depz_vl53l4_sigma_threshold_raw`](#depz_vl53l4_sigma_threshold_raw), [`depz_vl53l4_decode_sigma_threshold`](#depz_vl53l4_decode_sigma_threshold), [`DEPZ_VL53L4_DEFAULT_CONFIGURATION`](#depz_vl53l4_default_configuration), [`depz_vl53l4_config_block`](#depz_vl53l4_config_block)
- **VL53L8 (ToF)**: [`depz_vl53l8_variant`](#depz_vl53l8_variant), [`DEPZ_VL53L8CX_FOOTER_ID_OFFSET`](#depz_vl53l8cx_footer_id_offset), [`DEPZ_VL53L8_CMD_READ_REG`](#depz_vl53l8_cmd_read_reg), [`DEPZ_VL53L8_CMD_WRITE_REG`](#depz_vl53l8_cmd_write_reg), [`DEPZ_VL53L8_CMD_START_STREAM`](#depz_vl53l8_cmd_start_stream), [`DEPZ_VL53L8_CMD_STOP_STREAM`](#depz_vl53l8_cmd_stop_stream), [`DEPZ_VL53L8_RPT_REG_DATA`](#depz_vl53l8_rpt_reg_data), [`DEPZ_VL53L8_RPT_FRAME`](#depz_vl53l8_rpt_frame), [`DEPZ_VL53L8_STREAM_CHUNK_MAX`](#depz_vl53l8_stream_chunk_max), [`DEPZ_VL53L8_STREAM_TOTAL_MAX`](#depz_vl53l8_stream_total_max), [`DEPZ_VL53L8_RES_4X4`](#depz_vl53l8_res_4x4), [`DEPZ_VL53L8_RES_8X8`](#depz_vl53l8_res_8x8), [`DEPZ_VL53L8_MAX_ZONES`](#depz_vl53l8_max_zones), [`depz_vl53l8_pack_read_reg`](#depz_vl53l8_pack_read_reg), [`depz_vl53l8_pack_start_stream`](#depz_vl53l8_pack_start_stream), [`depz_vl53l8_chunk`](#depz_vl53l8_chunk), [`depz_vl53l8_unpack_chunk`](#depz_vl53l8_unpack_chunk), [`depz_vl53l8_reg_data`](#depz_vl53l8_reg_data), [`depz_vl53l8_unpack_reg_data`](#depz_vl53l8_unpack_reg_data), [`depz_vl53l8_reassembler`](#depz_vl53l8_reassembler), [`depz_vl53l8_reasm_init`](#depz_vl53l8_reasm_init), [`depz_vl53l8_reasm_feed`](#depz_vl53l8_reasm_feed), [`depz_vl53l8_frame`](#depz_vl53l8_frame), [`depz_vl53l8_decode_frame`](#depz_vl53l8_decode_frame), [`depz_vl53l8ch_decode_frame`](#depz_vl53l8ch_decode_frame), [`DEPZ_VL53L8_CNH_MAX_AGGREGATES`](#depz_vl53l8_cnh_max_aggregates), [`DEPZ_VL53L8_CNH_MAX_FEATURE`](#depz_vl53l8_cnh_max_feature), [`depz_vl53l8ch_cnh_config`](#depz_vl53l8ch_cnh_config), [`depz_vl53l8ch_cnh_frame`](#depz_vl53l8ch_cnh_frame), [`depz_vl53l8ch_decode_cnh`](#depz_vl53l8ch_decode_cnh), [`DEPZ_VL53L8_DIST_MM`](#depz_vl53l8_dist_mm), [`DEPZ_VL53L8_SIGNAL_PER_SPAD_KCPS`](#depz_vl53l8_signal_per_spad_kcps), [`DEPZ_VL53L8_RANGE_SIGMA_MM`](#depz_vl53l8_range_sigma_mm), [`DEPZ_VL53L8_AMBIENT_PER_SPAD_KCPS`](#depz_vl53l8_ambient_per_spad_kcps), [`DEPZ_VL53L8_NB_TARGET_DETECTED`](#depz_vl53l8_nb_target_detected), [`DEPZ_VL53L8_TAR_STATUS`](#depz_vl53l8_tar_status), [`DEPZ_VL53L8_NB_SPADS_ENABLED`](#depz_vl53l8_nb_spads_enabled), [`DEPZ_VL53L8_MOTION_INDICATOR`](#depz_vl53l8_motion_indicator), [`DEPZ_VL53L8_POWER_MODE_SLEEP`](#depz_vl53l8_power_mode_sleep), [`DEPZ_VL53L8_POWER_MODE_WAKEUP`](#depz_vl53l8_power_mode_wakeup), [`DEPZ_VL53L8_POWER_MODE_DEEP_SLEEP`](#depz_vl53l8_power_mode_deep_sleep), [`DEPZ_VL53L8_NB_THRESHOLDS`](#depz_vl53l8_nb_thresholds), [`DEPZ_VL53L8_THRESH_START_SIZE`](#depz_vl53l8_thresh_start_size), [`DEPZ_VL53L8_MOTION_CFG_SIZE`](#depz_vl53l8_motion_cfg_size), [`depz_vl53l8_xtalk_margin_to_raw`](#depz_vl53l8_xtalk_margin_to_raw), [`depz_vl53l8_xtalk_margin_from_raw`](#depz_vl53l8_xtalk_margin_from_raw), [`depz_vl53l8_threshold`](#depz_vl53l8_threshold), [`depz_vl53l8_pack_thresholds`](#depz_vl53l8_pack_thresholds), [`depz_vl53l8_motion_cfg_default_pack`](#depz_vl53l8_motion_cfg_default_pack)
- **VL53L5CX / VL53L7CX / VL53L7CH (ToF)**: [`depz_vl53l7_model`](#depz_vl53l7_model), [`DEPZ_VL53L7_CMD_PIN_CTRL`](#depz_vl53l7_cmd_pin_ctrl), [`DEPZ_VL53L7_CMD_GET_INFO`](#depz_vl53l7_cmd_get_info), [`DEPZ_VL53L7_CMD_SET_I2C_SPEED`](#depz_vl53l7_cmd_set_i2c_speed), [`DEPZ_VL53L7_RPT_INFO`](#depz_vl53l7_rpt_info), [`DEPZ_VL53L7_PIN_LPN_OFF`](#depz_vl53l7_pin_lpn_off), [`DEPZ_VL53L7_PIN_LPN_ON`](#depz_vl53l7_pin_lpn_on), [`DEPZ_VL53L7_PIN_I2C_RST`](#depz_vl53l7_pin_i2c_rst), [`DEPZ_VL53L7_PIN_SOFT_CYCLE`](#depz_vl53l7_pin_soft_cycle), [`DEPZ_VL53L7_I2C_OK`](#depz_vl53l7_i2c_ok), [`DEPZ_VL53L7_I2C_NACK`](#depz_vl53l7_i2c_nack), [`DEPZ_VL53L7_I2C_TIMEOUT`](#depz_vl53l7_i2c_timeout), [`DEPZ_VL53L7_I2C_BUS_ERROR`](#depz_vl53l7_i2c_bus_error), [`DEPZ_VL53L7_READ_MAX_LEN`](#depz_vl53l7_read_max_len), [`DEPZ_VL53L7_WRITE_MAX_LEN`](#depz_vl53l7_write_max_len), [`DEPZ_VL53L7_STREAM_CHUNK_MAX`](#depz_vl53l7_stream_chunk_max), [`DEPZ_VL53L7_INFO_SIZE`](#depz_vl53l7_info_size), [`DEPZ_VL53L7_FOOTER_ID_OFFSET`](#depz_vl53l7_footer_id_offset), [`depz_vl53l7_pack_read_reg`](#depz_vl53l7_pack_read_reg), [`depz_vl53l7_pack_write_reg`](#depz_vl53l7_pack_write_reg), [`depz_vl53l7_pack_pin_ctrl`](#depz_vl53l7_pack_pin_ctrl), [`depz_vl53l7_pack_set_i2c_speed`](#depz_vl53l7_pack_set_i2c_speed), [`depz_vl53l7_info`](#depz_vl53l7_info), [`depz_vl53l7_unpack_info`](#depz_vl53l7_unpack_info), [`depz_vl53l7_resolve_model`](#depz_vl53l7_resolve_model), [`depz_vl53l7_model_str`](#depz_vl53l7_model_str), [`depz_vl53l7_decode_frame`](#depz_vl53l7_decode_frame)
- **VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)**: [`DEPZ_VL53LX_CMD_READ_REG`](#depz_vl53lx_cmd_read_reg), [`DEPZ_VL53LX_CMD_WRITE_REG`](#depz_vl53lx_cmd_write_reg), [`DEPZ_VL53LX_CMD_XSHUT`](#depz_vl53lx_cmd_xshut), [`DEPZ_VL53LX_CMD_START_STREAM`](#depz_vl53lx_cmd_start_stream), [`DEPZ_VL53LX_CMD_STOP_STREAM`](#depz_vl53lx_cmd_stop_stream), [`DEPZ_VL53LX_CMD_GET_INFO`](#depz_vl53lx_cmd_get_info), [`DEPZ_VL53LX_CMD_SET_I2C_SPEED`](#depz_vl53lx_cmd_set_i2c_speed), [`DEPZ_VL53LX_CMD_SET_ADDR_WIDTH`](#depz_vl53lx_cmd_set_addr_width), [`DEPZ_VL53LX_CMD_CLEAR_I2C_ERRORS`](#depz_vl53lx_cmd_clear_i2c_errors), [`DEPZ_VL53LX_RPT_REG_DATA`](#depz_vl53lx_rpt_reg_data), [`DEPZ_VL53LX_RPT_INFO`](#depz_vl53lx_rpt_info), [`DEPZ_VL53LX_RPT_STREAM`](#depz_vl53lx_rpt_stream), [`DEPZ_VL53LX_CLEAR_STEPS_MAX`](#depz_vl53lx_clear_steps_max), [`DEPZ_VL53LX_START_STREAM_MAX`](#depz_vl53lx_start_stream_max), [`DEPZ_VL53LX_INFO_SIZE`](#depz_vl53lx_info_size), [`depz_vl53lx_clear_step`](#depz_vl53lx_clear_step), [`depz_vl53lx_pack_set_addr_width`](#depz_vl53lx_pack_set_addr_width), [`depz_vl53lx_pack_start_stream`](#depz_vl53lx_pack_start_stream), [`depz_vl53lx_info`](#depz_vl53lx_info), [`depz_vl53lx_unpack_info`](#depz_vl53lx_unpack_info), [`depz_vl53lx_product`](#depz_vl53lx_product), [`depz_vl53lx_driver`](#depz_vl53lx_driver), [`depz_vl53lx_product_info`](#depz_vl53lx_product_info), [`depz_vl53lx_product_get`](#depz_vl53lx_product_get), [`depz_vl53lx_driver_str`](#depz_vl53lx_driver_str), [`depz_vl53lx_product_from_str`](#depz_vl53lx_product_from_str), [`depz_vl53lx_product_from_board_name`](#depz_vl53lx_product_from_board_name), [`depz_vl53lx_resolve_product`](#depz_vl53lx_resolve_product), [`depz_vl53lx_class`](#depz_vl53lx_class), [`depz_vl53lx_resolve_class`](#depz_vl53lx_resolve_class), [`depz_vl53lx_class_str`](#depz_vl53lx_class_str), [`DEPZ_VL53LX_DIE_BLOCK_ADDR`](#depz_vl53lx_die_block_addr), [`DEPZ_VL53LX_DIE_BLOCK_LEN`](#depz_vl53lx_die_block_len), [`DEPZ_VL53LX_L0X_BLOCK_ADDR`](#depz_vl53lx_l0x_block_addr), [`DEPZ_VL53LX_L0X_BLOCK_LEN`](#depz_vl53lx_l0x_block_len), [`DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR`](#depz_vl53lx_histogram_block_addr), [`DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN`](#depz_vl53lx_histogram_block_len), [`DEPZ_VL53LX_HISTOGRAM_BINS`](#depz_vl53lx_histogram_bins), [`depz_vl53lx_die_variant`](#depz_vl53lx_die_variant), [`depz_vl53lx_decode_die_block`](#depz_vl53lx_decode_die_block), [`depz_vl53lx_l0x_raw`](#depz_vl53lx_l0x_raw), [`depz_vl53lx_decode_l0x_raw`](#depz_vl53lx_decode_l0x_raw), [`depz_vl53lx_histogram_raw`](#depz_vl53lx_histogram_raw), [`depz_vl53lx_decode_histogram_raw`](#depz_vl53lx_decode_histogram_raw)
- **BNO086 (IMU)**: [`DEPZ_BNO086_CMD_SENSOR_RESET`](#depz_bno086_cmd_sensor_reset), [`DEPZ_BNO086_CMD_SENSOR_WAKE_UP`](#depz_bno086_cmd_sensor_wake_up), [`DEPZ_BNO086_CMD_SEND_SHTP_PACKET`](#depz_bno086_cmd_send_shtp_packet), [`DEPZ_BNO086_RPT_DATA`](#depz_bno086_rpt_data), [`depz_bno086_unpack_data`](#depz_bno086_unpack_data), [`DEPZ_SHTP_HEADER_SIZE`](#depz_shtp_header_size), [`DEPZ_SHTP_LENGTH_MASK`](#depz_shtp_length_mask), [`DEPZ_SHTP_CONTINUATION`](#depz_shtp_continuation), [`DEPZ_SHTP_NUM_CHANNELS`](#depz_shtp_num_channels), [`DEPZ_SHTP_MAX_TX_FRAME`](#depz_shtp_max_tx_frame), [`depz_shtp_channel_id`](#depz_shtp_channel_id), [`depz_shtp_header`](#depz_shtp_header), [`depz_shtp_pack_header`](#depz_shtp_pack_header), [`depz_shtp_unpack_header`](#depz_shtp_unpack_header), [`depz_shtp_rx_channel`](#depz_shtp_rx_channel), [`depz_shtp_layer`](#depz_shtp_layer), [`depz_shtp_init`](#depz_shtp_init), [`depz_shtp_free`](#depz_shtp_free), [`depz_shtp_next_frame`](#depz_shtp_next_frame), [`depz_shtp_cargo`](#depz_shtp_cargo), [`depz_shtp_feed`](#depz_shtp_feed), [`depz_bno_pack_set_feature`](#depz_bno_pack_set_feature), [`depz_bno_pack_get_feature_request`](#depz_bno_pack_get_feature_request), [`depz_bno_pack_product_id_request`](#depz_bno_pack_product_id_request), [`depz_bno_pack_command_request`](#depz_bno_pack_command_request), [`depz_bno_pack_frs_read_request`](#depz_bno_pack_frs_read_request), [`depz_bno_pack_frs_write_request`](#depz_bno_pack_frs_write_request), [`depz_bno_pack_frs_write_data`](#depz_bno_pack_frs_write_data), [`DEPZ_BNO_SENSOR_ACCELEROMETER`](#depz_bno_sensor_accelerometer), [`DEPZ_BNO_SENSOR_GYROSCOPE`](#depz_bno_sensor_gyroscope), [`DEPZ_BNO_SENSOR_MAGNETOMETER`](#depz_bno_sensor_magnetometer), [`DEPZ_BNO_SENSOR_LINEAR_ACCELERATION`](#depz_bno_sensor_linear_acceleration), [`DEPZ_BNO_SENSOR_ROTATION_VECTOR`](#depz_bno_sensor_rotation_vector), [`DEPZ_BNO_SENSOR_GRAVITY`](#depz_bno_sensor_gravity), [`DEPZ_BNO_SENSOR_UNCALIBRATED_GYROSCOPE`](#depz_bno_sensor_uncalibrated_gyroscope), [`DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR`](#depz_bno_sensor_game_rotation_vector), [`DEPZ_BNO_SENSOR_GEOMAGNETIC_ROTATION_VECTOR`](#depz_bno_sensor_geomagnetic_rotation_vector), [`DEPZ_BNO_SENSOR_UNCALIBRATED_MAGNETOMETER`](#depz_bno_sensor_uncalibrated_magnetometer), [`DEPZ_BNO_SENSOR_TAP_DETECTOR`](#depz_bno_sensor_tap_detector), [`DEPZ_BNO_SENSOR_STEP_COUNTER`](#depz_bno_sensor_step_counter), [`DEPZ_BNO_SENSOR_SIGNIFICANT_MOTION`](#depz_bno_sensor_significant_motion), [`DEPZ_BNO_SENSOR_STABILITY_CLASSIFIER`](#depz_bno_sensor_stability_classifier), [`DEPZ_BNO_SENSOR_RAW_ACCELEROMETER`](#depz_bno_sensor_raw_accelerometer), [`DEPZ_BNO_SENSOR_RAW_GYROSCOPE`](#depz_bno_sensor_raw_gyroscope), [`DEPZ_BNO_SENSOR_RAW_MAGNETOMETER`](#depz_bno_sensor_raw_magnetometer), [`DEPZ_BNO_SENSOR_STEP_DETECTOR`](#depz_bno_sensor_step_detector), [`DEPZ_BNO_SENSOR_SHAKE_DETECTOR`](#depz_bno_sensor_shake_detector), [`DEPZ_BNO_SENSOR_FLIP_DETECTOR`](#depz_bno_sensor_flip_detector), [`DEPZ_BNO_SENSOR_PICKUP_DETECTOR`](#depz_bno_sensor_pickup_detector), [`DEPZ_BNO_SENSOR_STABILITY_DETECTOR`](#depz_bno_sensor_stability_detector), [`DEPZ_BNO_SENSOR_PERSONAL_ACTIVITY_CLASSIFIER`](#depz_bno_sensor_personal_activity_classifier), [`DEPZ_BNO_SENSOR_SLEEP_DETECTOR`](#depz_bno_sensor_sleep_detector), [`DEPZ_BNO_SENSOR_TILT_DETECTOR`](#depz_bno_sensor_tilt_detector), [`DEPZ_BNO_SENSOR_POCKET_DETECTOR`](#depz_bno_sensor_pocket_detector), [`DEPZ_BNO_SENSOR_CIRCLE_DETECTOR`](#depz_bno_sensor_circle_detector), [`DEPZ_BNO_SENSOR_HEART_RATE_MONITOR`](#depz_bno_sensor_heart_rate_monitor), [`DEPZ_BNO_SENSOR_ARVR_STABILIZED_RV`](#depz_bno_sensor_arvr_stabilized_rv), [`DEPZ_BNO_SENSOR_ARVR_STABILIZED_GAME_RV`](#depz_bno_sensor_arvr_stabilized_game_rv), [`DEPZ_BNO_SENSOR_GYRO_INTEGRATED_RV`](#depz_bno_sensor_gyro_integrated_rv), [`DEPZ_SH2_COMMAND_RESPONSE`](#depz_sh2_command_response), [`DEPZ_SH2_COMMAND_REQUEST`](#depz_sh2_command_request), [`DEPZ_SH2_FRS_READ_RESPONSE`](#depz_sh2_frs_read_response), [`DEPZ_SH2_FRS_READ_REQUEST`](#depz_sh2_frs_read_request), [`DEPZ_SH2_FRS_WRITE_RESPONSE`](#depz_sh2_frs_write_response), [`DEPZ_SH2_FRS_WRITE_DATA`](#depz_sh2_frs_write_data), [`DEPZ_SH2_FRS_WRITE_REQUEST`](#depz_sh2_frs_write_request), [`DEPZ_SH2_PRODUCT_ID_RESPONSE`](#depz_sh2_product_id_response), [`DEPZ_SH2_PRODUCT_ID_REQUEST`](#depz_sh2_product_id_request), [`DEPZ_SH2_GET_FEATURE_RESPONSE`](#depz_sh2_get_feature_response), [`DEPZ_SH2_SET_FEATURE_COMMAND`](#depz_sh2_set_feature_command), [`DEPZ_SH2_GET_FEATURE_REQUEST`](#depz_sh2_get_feature_request), [`DEPZ_SH2_CMD_ERRORS`](#depz_sh2_cmd_errors), [`DEPZ_SH2_CMD_COUNTER`](#depz_sh2_cmd_counter), [`DEPZ_SH2_CMD_TARE`](#depz_sh2_cmd_tare), [`DEPZ_SH2_CMD_INITIALIZE`](#depz_sh2_cmd_initialize), [`DEPZ_SH2_CMD_SAVE_DCD`](#depz_sh2_cmd_save_dcd), [`DEPZ_SH2_CMD_ME_CALIBRATE`](#depz_sh2_cmd_me_calibrate), [`DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG`](#depz_sh2_cmd_periodic_dcd_config), [`DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE`](#depz_sh2_cmd_get_oscillator_type), [`DEPZ_SH2_CMD_CLEAR_DCD_AND_RESET`](#depz_sh2_cmd_clear_dcd_and_reset), [`DEPZ_BNO_TARE_X`](#depz_bno_tare_x), [`DEPZ_BNO_TARE_Y`](#depz_bno_tare_y), [`DEPZ_BNO_TARE_Z`](#depz_bno_tare_z), [`DEPZ_BNO_TARE_ALL`](#depz_bno_tare_all), [`DEPZ_BNO_TARE_BASIS_RV`](#depz_bno_tare_basis_rv), [`DEPZ_BNO_TARE_BASIS_GAME_RV`](#depz_bno_tare_basis_game_rv), [`DEPZ_BNO_TARE_BASIS_GEOMAG_RV`](#depz_bno_tare_basis_geomag_rv), [`DEPZ_BNO_TARE_BASIS_GYRO_RV`](#depz_bno_tare_basis_gyro_rv), [`DEPZ_BNO_TARE_BASIS_ARVR_RV`](#depz_bno_tare_basis_arvr_rv), [`DEPZ_BNO_TARE_BASIS_ARVR_GAME_RV`](#depz_bno_tare_basis_arvr_game_rv), [`DEPZ_BNO_OSC_INTERNAL`](#depz_bno_osc_internal), [`DEPZ_BNO_OSC_EXT_CRYSTAL`](#depz_bno_osc_ext_crystal), [`DEPZ_BNO_OSC_EXT_CLOCK`](#depz_bno_osc_ext_clock), [`DEPZ_BNO_ERR_SOURCE_NO_MORE`](#depz_bno_err_source_no_more), [`DEPZ_BNO_FRS_STATIC_CALIBRATION_AGM`](#depz_bno_frs_static_calibration_agm), [`DEPZ_BNO_FRS_NOMINAL_CALIBRATION`](#depz_bno_frs_nominal_calibration), [`DEPZ_BNO_FRS_DYNAMIC_CALIBRATION`](#depz_bno_frs_dynamic_calibration), [`DEPZ_BNO_FRS_ME_POWER_MGMT`](#depz_bno_frs_me_power_mgmt), [`DEPZ_BNO_FRS_SYSTEM_ORIENTATION`](#depz_bno_frs_system_orientation), [`DEPZ_BNO_FRS_ACCEL_ORIENTATION`](#depz_bno_frs_accel_orientation), [`DEPZ_BNO_FRS_GYROSCOPE_ORIENTATION`](#depz_bno_frs_gyroscope_orientation), [`DEPZ_BNO_FRS_MAGNETOMETER_ORIENTATION`](#depz_bno_frs_magnetometer_orientation), [`DEPZ_BNO_FRS_ARVR_STABILIZATION_RV`](#depz_bno_frs_arvr_stabilization_rv), [`DEPZ_BNO_FRS_ARVR_STABILIZATION_GRV`](#depz_bno_frs_arvr_stabilization_grv), [`DEPZ_BNO_FRS_SIG_MOTION_DETECT_CONFIG`](#depz_bno_frs_sig_motion_detect_config), [`DEPZ_BNO_FRS_SHAKE_DETECT_CONFIG`](#depz_bno_frs_shake_detect_config), [`DEPZ_BNO_FRS_STABILITY_DETECTOR_CONFIG`](#depz_bno_frs_stability_detector_config), [`DEPZ_BNO_FRS_ACTIVITY_TRACKER_CONFIG`](#depz_bno_frs_activity_tracker_config), [`DEPZ_BNO_FRS_READ_NO_ERROR`](#depz_bno_frs_read_no_error), [`DEPZ_BNO_FRS_READ_UNRECOGNIZED_TYPE`](#depz_bno_frs_read_unrecognized_type), [`DEPZ_BNO_FRS_READ_BUSY`](#depz_bno_frs_read_busy), [`DEPZ_BNO_FRS_READ_COMPLETED`](#depz_bno_frs_read_completed), [`DEPZ_BNO_FRS_READ_OFFSET_OUT_OF_RANGE`](#depz_bno_frs_read_offset_out_of_range), [`DEPZ_BNO_FRS_READ_RECORD_EMPTY`](#depz_bno_frs_read_record_empty), [`DEPZ_BNO_FRS_READ_BLOCK_COMPLETED`](#depz_bno_frs_read_block_completed), [`DEPZ_BNO_FRS_READ_BLOCK_AND_READ_COMPLETED`](#depz_bno_frs_read_block_and_read_completed), [`DEPZ_BNO_FRS_READ_DEVICE_ERROR`](#depz_bno_frs_read_device_error), [`DEPZ_BNO_FRS_WRITE_WORDS_RECEIVED`](#depz_bno_frs_write_words_received), [`DEPZ_BNO_FRS_WRITE_UNRECOGNIZED_TYPE`](#depz_bno_frs_write_unrecognized_type), [`DEPZ_BNO_FRS_WRITE_BUSY`](#depz_bno_frs_write_busy), [`DEPZ_BNO_FRS_WRITE_COMPLETED`](#depz_bno_frs_write_completed), [`DEPZ_BNO_FRS_WRITE_MODE_READY`](#depz_bno_frs_write_mode_ready), [`DEPZ_BNO_FRS_WRITE_FAILED`](#depz_bno_frs_write_failed), [`DEPZ_BNO_FRS_WRITE_NOT_IN_WRITE_MODE`](#depz_bno_frs_write_not_in_write_mode), [`DEPZ_BNO_FRS_WRITE_INVALID_LENGTH`](#depz_bno_frs_write_invalid_length), [`DEPZ_BNO_FRS_WRITE_RECORD_VALID`](#depz_bno_frs_write_record_valid), [`DEPZ_BNO_FRS_WRITE_RECORD_INVALID`](#depz_bno_frs_write_record_invalid), [`depz_bno_feature`](#depz_bno_feature), [`depz_bno_product_id`](#depz_bno_product_id), [`depz_bno_command_response`](#depz_bno_command_response), [`depz_bno_frs_read_response`](#depz_bno_frs_read_response), [`depz_bno_frs_write_response`](#depz_bno_frs_write_response), [`depz_bno_unpack_feature_response`](#depz_bno_unpack_feature_response), [`depz_bno_unpack_product_id`](#depz_bno_unpack_product_id), [`depz_bno_unpack_command_response`](#depz_bno_unpack_command_response), [`depz_bno_unpack_frs_read_response`](#depz_bno_unpack_frs_read_response), [`depz_bno_unpack_frs_write_response`](#depz_bno_unpack_frs_write_response), [`depz_bno_metadata`](#depz_bno_metadata), [`depz_bno_metadata_from_words`](#depz_bno_metadata_from_words), [`depz_bno_metadata_record`](#depz_bno_metadata_record), [`DEPZ_BNO_BASE_TIMESTAMP_REF`](#depz_bno_base_timestamp_ref), [`DEPZ_BNO_TIMESTAMP_REBASE`](#depz_bno_timestamp_rebase), [`depz_bno_report_type`](#depz_bno_report_type), [`depz_bno_report_type_str`](#depz_bno_report_type_str), [`depz_bno_report`](#depz_bno_report), [`depz_bno_parse_input_cargo`](#depz_bno_parse_input_cargo), [`depz_bno_parse_gyro_rv`](#depz_bno_parse_gyro_rv), [`DEPZ_BNO_RV_ACCURACY_Q`](#depz_bno_rv_accuracy_q), [`DEPZ_BNO_GYRO_RV_ANGVEL_Q`](#depz_bno_gyro_rv_angvel_q), [`depz_bno_q_point`](#depz_bno_q_point), [`depz_bno_report_xyz`](#depz_bno_report_xyz), [`depz_bno_report_bias`](#depz_bno_report_bias), [`depz_bno_report_quaternion`](#depz_bno_report_quaternion), [`depz_bno_report_accuracy_rad`](#depz_bno_report_accuracy_rad), [`depz_bno_report_angular_velocity`](#depz_bno_report_angular_velocity), [`depz_bno_report_scalar`](#depz_bno_report_scalar)
- **BNO055 (IMU)**: [`DEPZ_BNO055_CMD_READ_REG`](#depz_bno055_cmd_read_reg), [`DEPZ_BNO055_CMD_WRITE_REG`](#depz_bno055_cmd_write_reg), [`DEPZ_BNO055_CMD_RESET`](#depz_bno055_cmd_reset), [`DEPZ_BNO055_CMD_START_STREAM`](#depz_bno055_cmd_start_stream), [`DEPZ_BNO055_CMD_STOP_STREAM`](#depz_bno055_cmd_stop_stream), [`DEPZ_BNO055_CMD_GET_INFO`](#depz_bno055_cmd_get_info), [`DEPZ_BNO055_RPT_REG_DATA`](#depz_bno055_rpt_reg_data), [`DEPZ_BNO055_RPT_INFO`](#depz_bno055_rpt_info), [`DEPZ_BNO055_RPT_STREAM`](#depz_bno055_rpt_stream), [`DEPZ_BNO055_XFER_MAX`](#depz_bno055_xfer_max), [`DEPZ_BNO055_INFO_SIZE`](#depz_bno055_info_size), [`DEPZ_BNO055_TRIGGER_TIMER`](#depz_bno055_trigger_timer), [`DEPZ_BNO055_TRIGGER_INT`](#depz_bno055_trigger_int), [`depz_bno055_pack_read_reg`](#depz_bno055_pack_read_reg), [`depz_bno055_pack_write_reg`](#depz_bno055_pack_write_reg), [`depz_bno055_pack_start_stream`](#depz_bno055_pack_start_stream), [`depz_bno055_reg_data`](#depz_bno055_reg_data), [`depz_bno055_unpack_reg_data`](#depz_bno055_unpack_reg_data), [`depz_bno055_info`](#depz_bno055_info), [`depz_bno055_unpack_info`](#depz_bno055_unpack_info), [`depz_bno055_stream`](#depz_bno055_stream), [`depz_bno055_unpack_stream`](#depz_bno055_unpack_stream), [`DEPZ_BNO055_REG_CHIP_ID`](#depz_bno055_reg_chip_id), [`DEPZ_BNO055_REG_PAGE_ID`](#depz_bno055_reg_page_id), [`DEPZ_BNO055_REG_ACC_DATA`](#depz_bno055_reg_acc_data), [`DEPZ_BNO055_REG_MAG_DATA`](#depz_bno055_reg_mag_data), [`DEPZ_BNO055_REG_GYR_DATA`](#depz_bno055_reg_gyr_data), [`DEPZ_BNO055_REG_EUL_DATA`](#depz_bno055_reg_eul_data), [`DEPZ_BNO055_REG_QUA_DATA`](#depz_bno055_reg_qua_data), [`DEPZ_BNO055_REG_LIA_DATA`](#depz_bno055_reg_lia_data), [`DEPZ_BNO055_REG_GRV_DATA`](#depz_bno055_reg_grv_data), [`DEPZ_BNO055_REG_TEMP`](#depz_bno055_reg_temp), [`DEPZ_BNO055_REG_CALIB_STAT`](#depz_bno055_reg_calib_stat), [`DEPZ_BNO055_REG_ST_RESULT`](#depz_bno055_reg_st_result), [`DEPZ_BNO055_REG_INT_STA`](#depz_bno055_reg_int_sta), [`DEPZ_BNO055_REG_SYS_CLK_STATUS`](#depz_bno055_reg_sys_clk_status), [`DEPZ_BNO055_REG_SYS_STATUS`](#depz_bno055_reg_sys_status), [`DEPZ_BNO055_REG_SYS_ERR`](#depz_bno055_reg_sys_err), [`DEPZ_BNO055_REG_UNIT_SEL`](#depz_bno055_reg_unit_sel), [`DEPZ_BNO055_REG_OPR_MODE`](#depz_bno055_reg_opr_mode), [`DEPZ_BNO055_REG_PWR_MODE`](#depz_bno055_reg_pwr_mode), [`DEPZ_BNO055_REG_SYS_TRIGGER`](#depz_bno055_reg_sys_trigger), [`DEPZ_BNO055_REG_TEMP_SOURCE`](#depz_bno055_reg_temp_source), [`DEPZ_BNO055_REG_AXIS_MAP_CONFIG`](#depz_bno055_reg_axis_map_config), [`DEPZ_BNO055_REG_AXIS_MAP_SIGN`](#depz_bno055_reg_axis_map_sign), [`DEPZ_BNO055_REG_SIC_MATRIX`](#depz_bno055_reg_sic_matrix), [`DEPZ_BNO055_REG_CALIB_PROFILE`](#depz_bno055_reg_calib_profile), [`DEPZ_BNO055_REG1_ACC_CONFIG`](#depz_bno055_reg1_acc_config), [`DEPZ_BNO055_REG1_MAG_CONFIG`](#depz_bno055_reg1_mag_config), [`DEPZ_BNO055_REG1_GYR_CONFIG_0`](#depz_bno055_reg1_gyr_config_0), [`DEPZ_BNO055_REG1_GYR_CONFIG_1`](#depz_bno055_reg1_gyr_config_1), [`DEPZ_BNO055_REG1_INT_MSK`](#depz_bno055_reg1_int_msk), [`DEPZ_BNO055_REG1_INT_EN`](#depz_bno055_reg1_int_en), [`DEPZ_BNO055_REG1_UNIQUE_ID`](#depz_bno055_reg1_unique_id), [`DEPZ_BNO055_FULL_BLOCK_ADDR`](#depz_bno055_full_block_addr), [`DEPZ_BNO055_FULL_BLOCK_LEN`](#depz_bno055_full_block_len), [`DEPZ_BNO055_QUAT_BLOCK_ADDR`](#depz_bno055_quat_block_addr), [`DEPZ_BNO055_QUAT_BLOCK_LEN`](#depz_bno055_quat_block_len), [`depz_bno055_opr_mode`](#depz_bno055_opr_mode), [`DEPZ_BNO055_UNIT_ACC_MG`](#depz_bno055_unit_acc_mg), [`DEPZ_BNO055_UNIT_GYR_RPS`](#depz_bno055_unit_gyr_rps), [`DEPZ_BNO055_UNIT_EUL_RAD`](#depz_bno055_unit_eul_rad), [`DEPZ_BNO055_UNIT_TEMP_F`](#depz_bno055_unit_temp_f), [`DEPZ_BNO055_UNIT_ORI_ANDROID`](#depz_bno055_unit_ori_android), [`depz_bno055_units`](#depz_bno055_units), [`depz_bno055_unpack_units`](#depz_bno055_unpack_units), [`depz_bno055_pack_units`](#depz_bno055_pack_units), [`depz_bno055_accel_lsb`](#depz_bno055_accel_lsb), [`depz_bno055_gyro_lsb`](#depz_bno055_gyro_lsb), [`depz_bno055_euler_lsb`](#depz_bno055_euler_lsb), [`depz_bno055_temp_lsb`](#depz_bno055_temp_lsb), [`DEPZ_BNO055_MAG_LSB`](#depz_bno055_mag_lsb), [`DEPZ_BNO055_QUAT_LSB`](#depz_bno055_quat_lsb), [`DEPZ_BNO055_FUSION_ACCEL_LSB`](#depz_bno055_fusion_accel_lsb), [`depz_bno055_calib_status`](#depz_bno055_calib_status), [`depz_bno055_unpack_calib_status`](#depz_bno055_unpack_calib_status), [`depz_bno055_pack_calib_status`](#depz_bno055_pack_calib_status), [`depz_bno055_fully_calibrated`](#depz_bno055_fully_calibrated), [`DEPZ_BNO055_CALIB_PROFILE_LEN`](#depz_bno055_calib_profile_len), [`depz_bno055_calib_profile`](#depz_bno055_calib_profile), [`depz_bno055_unpack_calib_profile`](#depz_bno055_unpack_calib_profile), [`depz_bno055_pack_calib_profile`](#depz_bno055_pack_calib_profile), [`DEPZ_BNO055_AXIS_X`](#depz_bno055_axis_x), [`DEPZ_BNO055_AXIS_Y`](#depz_bno055_axis_y), [`DEPZ_BNO055_AXIS_Z`](#depz_bno055_axis_z), [`depz_bno055_axis_remap`](#depz_bno055_axis_remap), [`depz_bno055_unpack_axis_remap`](#depz_bno055_unpack_axis_remap), [`depz_bno055_pack_axis_remap`](#depz_bno055_pack_axis_remap), [`DEPZ_BNO055_PLACEMENTS`](#depz_bno055_placements), [`depz_bno055_placement`](#depz_bno055_placement), [`depz_bno055_accel_config`](#depz_bno055_accel_config), [`depz_bno055_unpack_accel_config`](#depz_bno055_unpack_accel_config), [`depz_bno055_pack_accel_config`](#depz_bno055_pack_accel_config), [`depz_bno055_gyro_config`](#depz_bno055_gyro_config), [`depz_bno055_unpack_gyro_config`](#depz_bno055_unpack_gyro_config), [`depz_bno055_pack_gyro_config`](#depz_bno055_pack_gyro_config), [`depz_bno055_mag_config`](#depz_bno055_mag_config), [`depz_bno055_unpack_mag_config`](#depz_bno055_unpack_mag_config), [`depz_bno055_pack_mag_config`](#depz_bno055_pack_mag_config), [`depz_bno055_block`](#depz_bno055_block), [`depz_bno055_decode_block`](#depz_bno055_decode_block)
- **Bootloader / firmware update**: [`DEPZ_FWDEPZ_HEADER_SIZE`](#depz_fwdepz_header_size), [`depz_fwdepz_result`](#depz_fwdepz_result), [`depz_fwdepz_image`](#depz_fwdepz_image), [`depz_fwdepz_parse`](#depz_fwdepz_parse), [`depz_bl_cmd`](#depz_bl_cmd), [`depz_flash_info`](#depz_flash_info), [`depz_bl_pack_write_page`](#depz_bl_pack_write_page), [`depz_bl_pack_read_page`](#depz_bl_pack_read_page), [`depz_bl_unpack_flash_info`](#depz_bl_unpack_flash_info)
- **Datasets (record & replay)**: [`depz_dataset`](#depz_dataset), [`depz_dataset_device`](#depz_dataset_device), [`depz_dataset_record`](#depz_dataset_record), [`depz_dataset_parse`](#depz_dataset_parse), [`depz_dataset_free`](#depz_dataset_free), [`depz_dataset_schema`](#depz_dataset_schema), [`depz_dataset_device_count`](#depz_dataset_device_count), [`depz_dataset_get_device`](#depz_dataset_get_device), [`depz_dataset_record_count`](#depz_dataset_record_count), [`depz_dataset_get_record`](#depz_dataset_get_record), [`depz_dataset_value_int`](#depz_dataset_value_int), [`depz_dataset_value_str`](#depz_dataset_value_str)
- **Live layer: errors**: [`depz_err`](#depz_err), [`depz_err_str`](#depz_err_str), [`depz_last_error`](#depz_last_error), [`depz_last_status`](#depz_last_status), [`depz_last_status_cmd`](#depz_last_status_cmd)
- **Live layer: byte links**: [`depz_link`](#depz_link), [`depz_link_vtable`](#depz_link_vtable), [`depz_link_new`](#depz_link_new), [`depz_link_open_serial`](#depz_link_open_serial), [`depz_link_open_replay`](#depz_link_open_replay), [`depz_link_open_recording`](#depz_link_open_recording), [`depz_link_loopback_pair`](#depz_link_loopback_pair), [`depz_link_read`](#depz_link_read), [`depz_link_write`](#depz_link_write), [`depz_link_close`](#depz_link_close), [`depz_link_closed`](#depz_link_closed), [`depz_link_name`](#depz_link_name), [`depz_link_free`](#depz_link_free), [`depz_link_replay_exhausted`](#depz_link_replay_exhausted)
- **Live layer: streams**: [`depz_stream`](#depz_stream), [`depz_stream_next`](#depz_stream_next), [`depz_stream_dropped_count`](#depz_stream_dropped_count), [`depz_stream_close`](#depz_stream_close)
- **Live layer: device core**: [`depz_device`](#depz_device), [`DEPZ_DEFAULT_TIMEOUT_MS`](#depz_default_timeout_ms), [`depz_link_stats`](#depz_link_stats), [`depz_device_open`](#depz_device_open), [`depz_device_open_link`](#depz_device_open_link), [`depz_device_promote`](#depz_device_promote), [`depz_device_close`](#depz_device_close), [`depz_device_set_timeout_ms`](#depz_device_set_timeout_ms), [`depz_device_sensor_type`](#depz_device_sensor_type), [`depz_device_port`](#depz_device_port), [`depz_device_closed`](#depz_device_closed), [`depz_device_stats`](#depz_device_stats), [`depz_matcher`](#depz_matcher), [`depz_device_request`](#depz_device_request), [`depz_device_send`](#depz_device_send), [`depz_device_get_device_name`](#depz_device_get_device_name), [`depz_device_get_software_name`](#depz_device_get_software_name), [`depz_device_get_serial_number`](#depz_device_get_serial_number), [`depz_device_read_mcu_temperature`](#depz_device_read_mcu_temperature), [`depz_device_get_payload_crc_type`](#depz_device_get_payload_crc_type), [`depz_device_set_payload_crc_type`](#depz_device_set_payload_crc_type), [`depz_device_get_sync_pin`](#depz_device_get_sync_pin), [`depz_device_set_sync_pin`](#depz_device_set_sync_pin), [`depz_device_reset`](#depz_device_reset), [`depz_device_enter_bootloader`](#depz_device_enter_bootloader), [`depz_time_sync`](#depz_time_sync), [`depz_host_now_us`](#depz_host_now_us), [`depz_device_sync_time`](#depz_device_sync_time), [`depz_device_time_sync`](#depz_device_time_sync), [`depz_device_to_host_time_us`](#depz_device_to_host_time_us), [`depz_device_event_type`](#depz_device_event_type), [`depz_device_event`](#depz_device_event), [`depz_device_event_cb`](#depz_device_event_cb), [`depz_device_on_event`](#depz_device_on_event), [`depz_device_off_event`](#depz_device_off_event), [`depz_device_events`](#depz_device_events)
- **Live layer: discovery**: [`depz_port_info`](#depz_port_info), [`depz_list_serial_ports`](#depz_list_serial_ports), [`depz_free_port_list`](#depz_free_port_list), [`depz_device_info`](#depz_device_info), [`depz_probe_port`](#depz_probe_port), [`depz_list_depz_devices`](#depz_list_depz_devices), [`depz_free_device_list`](#depz_free_device_list), [`depz_open_options`](#depz_open_options), [`DEPZ_OPEN_OPTIONS_INIT`](#depz_open_options_init), [`depz_open_device`](#depz_open_device)
- **SR04 sensor class (live layer)**: [`depz_sr04_measurement`](#depz_sr04_measurement), [`depz_sr04_measurement_cb`](#depz_sr04_measurement_cb), [`depz_sr04_open_link`](#depz_sr04_open_link), [`depz_is_sr04`](#depz_is_sr04), [`depz_sr04_valid`](#depz_sr04_valid), [`depz_sr04_measurement_distance_mm`](#depz_sr04_measurement_distance_mm), [`depz_sr04_measurement_distance_mm_at`](#depz_sr04_measurement_distance_mm_at), [`depz_sr04_get_sample_period_us`](#depz_sr04_get_sample_period_us), [`depz_sr04_set_sample_period_us`](#depz_sr04_set_sample_period_us), [`depz_sr04_get_echo_decay_us`](#depz_sr04_get_echo_decay_us), [`depz_sr04_set_echo_decay_us`](#depz_sr04_set_echo_decay_us), [`depz_sr04_measure_once`](#depz_sr04_measure_once), [`depz_sr04_start`](#depz_sr04_start), [`depz_sr04_stop`](#depz_sr04_stop), [`depz_sr04_on_measurement`](#depz_sr04_on_measurement), [`depz_sr04_off_measurement`](#depz_sr04_off_measurement), [`depz_sr04_stream`](#depz_sr04_stream)
- **VL53L4CD sensor class (live layer)**: [`depz_vl53l4cd_measurement`](#depz_vl53l4cd_measurement), [`depz_vl53l4cd_measurement_cb`](#depz_vl53l4cd_measurement_cb), [`DEPZ_VL53L4CD_WINDOW_BELOW`](#depz_vl53l4cd_window_below), [`DEPZ_VL53L4CD_WINDOW_ABOVE`](#depz_vl53l4cd_window_above), [`DEPZ_VL53L4CD_WINDOW_OUT`](#depz_vl53l4cd_window_out), [`DEPZ_VL53L4CD_WINDOW_IN`](#depz_vl53l4cd_window_in), [`DEPZ_VL53L4CD_I2C_KHZ_DEFAULT`](#depz_vl53l4cd_i2c_khz_default), [`depz_vl53l4cd_open_link`](#depz_vl53l4cd_open_link), [`depz_is_vl53l4cd`](#depz_is_vl53l4cd), [`depz_vl53l4cd_status_text`](#depz_vl53l4cd_status_text), [`depz_vl53l4cd_is_alive`](#depz_vl53l4cd_is_alive), [`depz_vl53l4cd_init`](#depz_vl53l4cd_init), [`depz_vl53l4cd_initialized`](#depz_vl53l4cd_initialized), [`depz_vl53l4cd_ranging`](#depz_vl53l4cd_ranging), [`depz_vl53l4cd_xshut`](#depz_vl53l4cd_xshut), [`depz_vl53l4cd_reset_sensor`](#depz_vl53l4cd_reset_sensor), [`depz_vl53l4cd_bridge_info`](#depz_vl53l4cd_bridge_info), [`depz_vl53l4cd_set_i2c_speed_khz`](#depz_vl53l4cd_set_i2c_speed_khz), [`depz_vl53l4cd_set_range_timing`](#depz_vl53l4cd_set_range_timing), [`depz_vl53l4cd_get_range_timing`](#depz_vl53l4cd_get_range_timing), [`depz_vl53l4cd_set_offset_mm`](#depz_vl53l4cd_set_offset_mm), [`depz_vl53l4cd_get_offset_mm`](#depz_vl53l4cd_get_offset_mm), [`depz_vl53l4cd_set_xtalk_kcps`](#depz_vl53l4cd_set_xtalk_kcps), [`depz_vl53l4cd_get_xtalk_kcps`](#depz_vl53l4cd_get_xtalk_kcps), [`depz_vl53l4cd_set_detection_thresholds`](#depz_vl53l4cd_set_detection_thresholds), [`depz_vl53l4cd_get_detection_thresholds`](#depz_vl53l4cd_get_detection_thresholds), [`depz_vl53l4cd_set_signal_threshold_kcps`](#depz_vl53l4cd_set_signal_threshold_kcps), [`depz_vl53l4cd_get_signal_threshold_kcps`](#depz_vl53l4cd_get_signal_threshold_kcps), [`depz_vl53l4cd_set_sigma_threshold_mm`](#depz_vl53l4cd_set_sigma_threshold_mm), [`depz_vl53l4cd_get_sigma_threshold_mm`](#depz_vl53l4cd_get_sigma_threshold_mm), [`depz_vl53l4cd_start_temperature_update`](#depz_vl53l4cd_start_temperature_update), [`depz_vl53l4cd_calibrate_offset`](#depz_vl53l4cd_calibrate_offset), [`depz_vl53l4cd_calibrate_xtalk`](#depz_vl53l4cd_calibrate_xtalk), [`depz_vl53l4cd_start_ranging`](#depz_vl53l4cd_start_ranging), [`depz_vl53l4cd_stop_ranging`](#depz_vl53l4cd_stop_ranging), [`depz_vl53l4cd_measure_once`](#depz_vl53l4cd_measure_once), [`depz_vl53l4cd_on_measurement`](#depz_vl53l4cd_on_measurement), [`depz_vl53l4cd_off_measurement`](#depz_vl53l4cd_off_measurement), [`depz_vl53l4cd_stream`](#depz_vl53l4cd_stream), [`depz_vl53l4cd_get_measurement`](#depz_vl53l4cd_get_measurement), [`depz_vl53l4cd_stream_parse_errors`](#depz_vl53l4cd_stream_parse_errors), [`depz_vl53l4cd_read_reg`](#depz_vl53l4cd_read_reg), [`depz_vl53l4cd_write_reg`](#depz_vl53l4cd_write_reg)
- **VL53L8 multizone sensor class (live layer)**: [`depz_vl53l8_model`](#depz_vl53l8_model), [`DEPZ_VL53L8_RANGING_MODE_CONTINUOUS`](#depz_vl53l8_ranging_mode_continuous), [`DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS`](#depz_vl53l8_ranging_mode_autonomous), [`DEPZ_VL53L8_TARGET_ORDER_CLOSEST`](#depz_vl53l8_target_order_closest), [`DEPZ_VL53L8_TARGET_ORDER_STRONGEST`](#depz_vl53l8_target_order_strongest), [`DEPZ_VL53L8_XTALK_BUFFER_SIZE`](#depz_vl53l8_xtalk_buffer_size), [`DEPZ_VL53L8_CNH_MAX_BYTES`](#depz_vl53l8_cnh_max_bytes), [`DEPZ_VL53L8_THRESH_IN_WINDOW`](#depz_vl53l8_thresh_in_window), [`DEPZ_VL53L8_THRESH_OUT_OF_WINDOW`](#depz_vl53l8_thresh_out_of_window), [`DEPZ_VL53L8_THRESH_LESS_THAN_EQUAL_MIN`](#depz_vl53l8_thresh_less_than_equal_min), [`DEPZ_VL53L8_THRESH_GREATER_THAN_MAX`](#depz_vl53l8_thresh_greater_than_max), [`DEPZ_VL53L8_THRESH_EQUAL_MIN`](#depz_vl53l8_thresh_equal_min), [`DEPZ_VL53L8_THRESH_NOT_EQUAL_MIN`](#depz_vl53l8_thresh_not_equal_min), [`DEPZ_VL53L8_THRESH_OP_OR`](#depz_vl53l8_thresh_op_or), [`DEPZ_VL53L8_THRESH_OP_AND`](#depz_vl53l8_thresh_op_and), [`DEPZ_VL53L8_THRESH_LAST`](#depz_vl53l8_thresh_last), [`depz_vl53l8_motion`](#depz_vl53l8_motion), [`depz_vl53l8_live_frame`](#depz_vl53l8_live_frame), [`depz_vl53l8_frame_cb`](#depz_vl53l8_frame_cb), [`depz_vl53l8_progress_cb`](#depz_vl53l8_progress_cb), [`depz_vl53l8_cnh_setup`](#depz_vl53l8_cnh_setup), [`depz_vl53l8_cnh_init_config`](#depz_vl53l8_cnh_init_config), [`depz_vl53l8_cnh_create_agg_map`](#depz_vl53l8_cnh_create_agg_map), [`depz_vl53l8_cnh_required_memory`](#depz_vl53l8_cnh_required_memory), [`depz_vl53l8_cnh_pack`](#depz_vl53l8_cnh_pack), [`depz_vl53l8_cnh_decode_config`](#depz_vl53l8_cnh_decode_config), [`depz_vl53l8_open_link`](#depz_vl53l8_open_link), [`depz_is_vl53l8`](#depz_is_vl53l8), [`depz_vl53l8_get_model`](#depz_vl53l8_get_model), [`depz_vl53l8_is_alive`](#depz_vl53l8_is_alive), [`depz_vl53l8_init`](#depz_vl53l8_init), [`depz_vl53l8_initialized`](#depz_vl53l8_initialized), [`depz_vl53l8_ranging`](#depz_vl53l8_ranging), [`depz_vl53l8_get_resolution`](#depz_vl53l8_get_resolution), [`depz_vl53l8_set_resolution`](#depz_vl53l8_set_resolution), [`depz_vl53l8_get_ranging_frequency_hz`](#depz_vl53l8_get_ranging_frequency_hz), [`depz_vl53l8_set_ranging_frequency_hz`](#depz_vl53l8_set_ranging_frequency_hz), [`depz_vl53l8_get_ranging_mode`](#depz_vl53l8_get_ranging_mode), [`depz_vl53l8_set_ranging_mode`](#depz_vl53l8_set_ranging_mode), [`depz_vl53l8_get_integration_time_ms`](#depz_vl53l8_get_integration_time_ms), [`depz_vl53l8_set_integration_time_ms`](#depz_vl53l8_set_integration_time_ms), [`depz_vl53l8_get_sharpener_percent`](#depz_vl53l8_get_sharpener_percent), [`depz_vl53l8_set_sharpener_percent`](#depz_vl53l8_set_sharpener_percent), [`depz_vl53l8_get_target_order`](#depz_vl53l8_get_target_order), [`depz_vl53l8_set_target_order`](#depz_vl53l8_set_target_order), [`depz_vl53l8_get_power_mode`](#depz_vl53l8_get_power_mode), [`depz_vl53l8_set_power_mode`](#depz_vl53l8_set_power_mode), [`depz_vl53l8_get_xtalk_margin`](#depz_vl53l8_get_xtalk_margin), [`depz_vl53l8_set_xtalk_margin`](#depz_vl53l8_set_xtalk_margin), [`depz_vl53l8_calibrate_xtalk`](#depz_vl53l8_calibrate_xtalk), [`depz_vl53l8_get_caldata_xtalk`](#depz_vl53l8_get_caldata_xtalk), [`depz_vl53l8_set_caldata_xtalk`](#depz_vl53l8_set_caldata_xtalk), [`depz_vl53l8_get_detection_thresholds_enable`](#depz_vl53l8_get_detection_thresholds_enable), [`depz_vl53l8_set_detection_thresholds_enable`](#depz_vl53l8_set_detection_thresholds_enable), [`depz_vl53l8_get_detection_thresholds`](#depz_vl53l8_get_detection_thresholds), [`depz_vl53l8_set_detection_thresholds`](#depz_vl53l8_set_detection_thresholds), [`depz_vl53l8_set_detection_thresholds_auto_stop`](#depz_vl53l8_set_detection_thresholds_auto_stop), [`depz_vl53l8_configure_motion_indicator`](#depz_vl53l8_configure_motion_indicator), [`depz_vl53l8_configure_cnh`](#depz_vl53l8_configure_cnh), [`depz_vl53l8_start_ranging`](#depz_vl53l8_start_ranging), [`depz_vl53l8_stop_ranging`](#depz_vl53l8_stop_ranging), [`depz_vl53l8_on_frame`](#depz_vl53l8_on_frame), [`depz_vl53l8_off_frame`](#depz_vl53l8_off_frame), [`depz_vl53l8_frames`](#depz_vl53l8_frames), [`depz_vl53l8_get_frame`](#depz_vl53l8_get_frame), [`depz_vl53l8_frame_parse_errors`](#depz_vl53l8_frame_parse_errors), [`depz_vl53l8_reassembler_discards`](#depz_vl53l8_reassembler_discards), [`depz_vl53l8_module_type`](#depz_vl53l8_module_type), [`depz_vl53l7_bridge_info`](#depz_vl53l7_bridge_info), [`depz_vl53l7_set_i2c_speed_khz`](#depz_vl53l7_set_i2c_speed_khz), [`depz_vl53l7_pin_ctrl`](#depz_vl53l7_pin_ctrl), [`depz_vl53l8_read_reg`](#depz_vl53l8_read_reg), [`depz_vl53l8_write_reg`](#depz_vl53l8_write_reg), [`depz_vl53l8_dci_read`](#depz_vl53l8_dci_read), [`depz_vl53l8_dci_write`](#depz_vl53l8_dci_write)
- **VL53L 1D family sensor class (live layer)**: [`DEPZ_VL53LX_CAP_MODE`](#depz_vl53lx_cap_mode), [`DEPZ_VL53LX_CAP_TIMING`](#depz_vl53lx_cap_timing), [`DEPZ_VL53LX_CAP_OFFSET`](#depz_vl53lx_cap_offset), [`DEPZ_VL53LX_CAP_XTALK`](#depz_vl53lx_cap_xtalk), [`DEPZ_VL53LX_CAP_CALIB_OFFSET`](#depz_vl53lx_cap_calib_offset), [`DEPZ_VL53LX_CAP_CALIB_XTALK`](#depz_vl53lx_cap_calib_xtalk), [`DEPZ_VL53LX_CAP_THRESHOLDS`](#depz_vl53lx_cap_thresholds), [`DEPZ_VL53LX_CAP_SIGNAL_THRESH`](#depz_vl53lx_cap_signal_thresh), [`DEPZ_VL53LX_CAP_SIGMA_THRESH`](#depz_vl53lx_cap_sigma_thresh), [`DEPZ_VL53LX_CAP_ROI`](#depz_vl53lx_cap_roi), [`DEPZ_VL53LX_CAP_TEMP_UPDATE`](#depz_vl53lx_cap_temp_update), [`DEPZ_VL53LX_CAP_REFSPAD`](#depz_vl53lx_cap_refspad), [`DEPZ_VL53LX_WINDOW_BELOW`](#depz_vl53lx_window_below), [`DEPZ_VL53LX_WINDOW_ABOVE`](#depz_vl53lx_window_above), [`DEPZ_VL53LX_WINDOW_OUT`](#depz_vl53lx_window_out), [`DEPZ_VL53LX_WINDOW_IN`](#depz_vl53lx_window_in), [`DEPZ_VL53LX_MAX_TARGETS`](#depz_vl53lx_max_targets), [`DEPZ_VL53LX_MAX_MODES`](#depz_vl53lx_max_modes), [`depz_vl53lx_target`](#depz_vl53lx_target), [`depz_vl53lx_bins`](#depz_vl53lx_bins), [`depz_vl53lx_measurement`](#depz_vl53lx_measurement), [`depz_vl53lx_measurement_cb`](#depz_vl53lx_measurement_cb), [`depz_vl53lx_status_plottable`](#depz_vl53lx_status_plottable), [`depz_vl53lx_plottable`](#depz_vl53lx_plottable), [`depz_vl53lx_primary_distance`](#depz_vl53lx_primary_distance), [`depz_vl53lx_open_link`](#depz_vl53lx_open_link), [`depz_is_vl53lx`](#depz_is_vl53lx), [`depz_vl53lx_detected_product`](#depz_vl53lx_detected_product), [`depz_vl53lx_driver_kinds`](#depz_vl53lx_driver_kinds), [`depz_vl53lx_init`](#depz_vl53lx_init), [`depz_vl53lx_initialized`](#depz_vl53lx_initialized), [`depz_vl53lx_product_bound`](#depz_vl53lx_product_bound), [`depz_vl53lx_driver_bound`](#depz_vl53lx_driver_bound), [`depz_vl53lx_caveat`](#depz_vl53lx_caveat), [`depz_vl53lx_driver_reach_mm`](#depz_vl53lx_driver_reach_mm), [`depz_vl53lx_supports`](#depz_vl53lx_supports), [`depz_vl53lx_ranging`](#depz_vl53lx_ranging), [`depz_vl53lx_model_id`](#depz_vl53lx_model_id), [`depz_vl53lx_modes`](#depz_vl53lx_modes), [`depz_vl53lx_budget_range`](#depz_vl53lx_budget_range), [`depz_vl53lx_budget_choices`](#depz_vl53lx_budget_choices), [`depz_vl53lx_xshut`](#depz_vl53lx_xshut), [`depz_vl53lx_bridge_info`](#depz_vl53lx_bridge_info), [`depz_vl53lx_configure`](#depz_vl53lx_configure), [`depz_vl53lx_configure_ex`](#depz_vl53lx_configure_ex), [`depz_vl53lx_get_range_timing`](#depz_vl53lx_get_range_timing), [`depz_vl53lx_set_mode`](#depz_vl53lx_set_mode), [`depz_vl53lx_get_mode`](#depz_vl53lx_get_mode), [`depz_vl53lx_get_offset_mm`](#depz_vl53lx_get_offset_mm), [`depz_vl53lx_set_offset_mm`](#depz_vl53lx_set_offset_mm), [`depz_vl53lx_get_xtalk_kcps`](#depz_vl53lx_get_xtalk_kcps), [`depz_vl53lx_set_xtalk_kcps`](#depz_vl53lx_set_xtalk_kcps), [`depz_vl53lx_calibrate_offset`](#depz_vl53lx_calibrate_offset), [`depz_vl53lx_calibrate_xtalk`](#depz_vl53lx_calibrate_xtalk), [`depz_vl53lx_get_detection_thresholds`](#depz_vl53lx_get_detection_thresholds), [`depz_vl53lx_set_detection_thresholds`](#depz_vl53lx_set_detection_thresholds), [`depz_vl53lx_get_signal_threshold_kcps`](#depz_vl53lx_get_signal_threshold_kcps), [`depz_vl53lx_set_signal_threshold_kcps`](#depz_vl53lx_set_signal_threshold_kcps), [`depz_vl53lx_get_sigma_threshold_mm`](#depz_vl53lx_get_sigma_threshold_mm), [`depz_vl53lx_set_sigma_threshold_mm`](#depz_vl53lx_set_sigma_threshold_mm), [`depz_vl53lx_get_roi`](#depz_vl53lx_get_roi), [`depz_vl53lx_set_roi`](#depz_vl53lx_set_roi), [`depz_vl53lx_get_roi_center`](#depz_vl53lx_get_roi_center), [`depz_vl53lx_set_roi_center`](#depz_vl53lx_set_roi_center), [`depz_vl53lx_start_temperature_update`](#depz_vl53lx_start_temperature_update), [`depz_vl53lx_perform_ref_spad_management`](#depz_vl53lx_perform_ref_spad_management), [`depz_vl53lx_start_ranging`](#depz_vl53lx_start_ranging), [`depz_vl53lx_stop_ranging`](#depz_vl53lx_stop_ranging), [`depz_vl53lx_measure_once`](#depz_vl53lx_measure_once), [`depz_vl53lx_on_measurement`](#depz_vl53lx_on_measurement), [`depz_vl53lx_off_measurement`](#depz_vl53lx_off_measurement), [`depz_vl53lx_measurements`](#depz_vl53lx_measurements), [`depz_vl53lx_get_measurement`](#depz_vl53lx_get_measurement), [`depz_vl53lx_stream_parse_errors`](#depz_vl53lx_stream_parse_errors), [`depz_vl53lx_read_reg`](#depz_vl53lx_read_reg), [`depz_vl53lx_write_reg`](#depz_vl53lx_write_reg)
- **BNO055 sensor class (live layer)**: [`DEPZ_BNO055_POWER_NORMAL`](#depz_bno055_power_normal), [`DEPZ_BNO055_POWER_LOW`](#depz_bno055_power_low), [`DEPZ_BNO055_POWER_SUSPEND`](#depz_bno055_power_suspend), [`DEPZ_BNO055_TEMP_FROM_ACCEL`](#depz_bno055_temp_from_accel), [`DEPZ_BNO055_TEMP_FROM_GYRO`](#depz_bno055_temp_from_gyro), [`DEPZ_BNO055_HAS_ACCEL`](#depz_bno055_has_accel), [`DEPZ_BNO055_HAS_MAG`](#depz_bno055_has_mag), [`DEPZ_BNO055_HAS_GYRO`](#depz_bno055_has_gyro), [`DEPZ_BNO055_HAS_EULER`](#depz_bno055_has_euler), [`DEPZ_BNO055_HAS_QUATERNION`](#depz_bno055_has_quaternion), [`DEPZ_BNO055_HAS_LINEAR_ACCEL`](#depz_bno055_has_linear_accel), [`DEPZ_BNO055_HAS_GRAVITY`](#depz_bno055_has_gravity), [`DEPZ_BNO055_HAS_TEMPERATURE`](#depz_bno055_has_temperature), [`DEPZ_BNO055_HAS_CALIBRATION`](#depz_bno055_has_calibration), [`depz_bno055_sample`](#depz_bno055_sample), [`depz_bno055_status_regs`](#depz_bno055_status_regs), [`depz_bno055_sample_cb`](#depz_bno055_sample_cb), [`depz_bno055_decode_sample`](#depz_bno055_decode_sample), [`depz_bno055_open_link`](#depz_bno055_open_link), [`depz_is_bno055`](#depz_is_bno055), [`depz_bno055_bridge_info`](#depz_bno055_bridge_info), [`depz_bno055_is_alive`](#depz_bno055_is_alive), [`depz_bno055_reset_sensor`](#depz_bno055_reset_sensor), [`depz_bno055_read_registers`](#depz_bno055_read_registers), [`depz_bno055_write_registers`](#depz_bno055_write_registers), [`depz_bno055_get_operation_mode`](#depz_bno055_get_operation_mode), [`depz_bno055_set_operation_mode`](#depz_bno055_set_operation_mode), [`depz_bno055_get_power_mode`](#depz_bno055_get_power_mode), [`depz_bno055_set_power_mode`](#depz_bno055_set_power_mode), [`depz_bno055_get_units`](#depz_bno055_get_units), [`depz_bno055_set_units`](#depz_bno055_set_units), [`depz_bno055_get_axis_remap`](#depz_bno055_get_axis_remap), [`depz_bno055_set_axis_remap`](#depz_bno055_set_axis_remap), [`depz_bno055_set_axis_placement`](#depz_bno055_set_axis_placement), [`depz_bno055_get_temperature_source`](#depz_bno055_get_temperature_source), [`depz_bno055_set_temperature_source`](#depz_bno055_set_temperature_source), [`depz_bno055_configure`](#depz_bno055_configure), [`depz_bno055_restore_configuration`](#depz_bno055_restore_configuration), [`depz_bno055_system_status`](#depz_bno055_system_status), [`depz_bno055_self_test`](#depz_bno055_self_test), [`depz_bno055_calibration_status`](#depz_bno055_calibration_status), [`depz_bno055_read_calibration_profile`](#depz_bno055_read_calibration_profile), [`depz_bno055_write_calibration_profile`](#depz_bno055_write_calibration_profile), [`depz_bno055_get_sic_matrix`](#depz_bno055_get_sic_matrix), [`depz_bno055_set_sic_matrix`](#depz_bno055_set_sic_matrix), [`depz_bno055_get_accel_config`](#depz_bno055_get_accel_config), [`depz_bno055_set_accel_config`](#depz_bno055_set_accel_config), [`depz_bno055_get_gyro_config`](#depz_bno055_get_gyro_config), [`depz_bno055_set_gyro_config`](#depz_bno055_set_gyro_config), [`depz_bno055_get_mag_config`](#depz_bno055_get_mag_config), [`depz_bno055_set_mag_config`](#depz_bno055_set_mag_config), [`depz_bno055_unique_id`](#depz_bno055_unique_id), [`depz_bno055_get_interrupt_enable`](#depz_bno055_get_interrupt_enable), [`depz_bno055_set_interrupt_enable`](#depz_bno055_set_interrupt_enable), [`depz_bno055_get_interrupt_mask`](#depz_bno055_get_interrupt_mask), [`depz_bno055_set_interrupt_mask`](#depz_bno055_set_interrupt_mask), [`depz_bno055_set_interrupt_setting`](#depz_bno055_set_interrupt_setting), [`depz_bno055_read_interrupt_status`](#depz_bno055_read_interrupt_status), [`depz_bno055_clear_interrupt`](#depz_bno055_clear_interrupt), [`depz_bno055_read_sample`](#depz_bno055_read_sample), [`depz_bno055_read_quaternion`](#depz_bno055_read_quaternion), [`depz_bno055_start_stream`](#depz_bno055_start_stream), [`depz_bno055_stop_stream`](#depz_bno055_stop_stream), [`depz_bno055_streaming`](#depz_bno055_streaming), [`depz_bno055_on_sample`](#depz_bno055_on_sample), [`depz_bno055_off_sample`](#depz_bno055_off_sample), [`depz_bno055_samples`](#depz_bno055_samples), [`depz_bno055_get_sample`](#depz_bno055_get_sample), [`depz_bno055_stream_parse_errors`](#depz_bno055_stream_parse_errors)
- **BNO085 / BNO086 sensor class (live layer)**: [`DEPZ_BNO086_BUSY_RETRIES`](#depz_bno086_busy_retries), [`DEPZ_BNO086_BUSY_BACKOFF_MS`](#depz_bno086_busy_backoff_ms), [`DEPZ_BNO086_RATE_LOW_FACTOR`](#depz_bno086_rate_low_factor), [`DEPZ_BNO086_RATE_HIGH_FACTOR`](#depz_bno086_rate_high_factor), [`depz_bno086_feature_request`](#depz_bno086_feature_request), [`depz_bno086_calibration`](#depz_bno086_calibration), [`depz_bno086_error_record`](#depz_bno086_error_record), [`depz_bno086_counts`](#depz_bno086_counts), [`depz_bno086_report_cb`](#depz_bno086_report_cb), [`depz_bno086_open_link`](#depz_bno086_open_link), [`depz_is_bno086`](#depz_is_bno086), [`depz_bno086_hardware_reset`](#depz_bno086_hardware_reset), [`depz_bno086_wake`](#depz_bno086_wake), [`depz_bno086_advertisement`](#depz_bno086_advertisement), [`depz_bno086_product_id`](#depz_bno086_product_id), [`depz_bno086_enable`](#depz_bno086_enable), [`depz_bno086_enable_ex`](#depz_bno086_enable_ex), [`depz_bno086_rate_ok`](#depz_bno086_rate_ok), [`depz_bno086_disable`](#depz_bno086_disable), [`depz_bno086_get_feature`](#depz_bno086_get_feature), [`depz_bno086_on_report`](#depz_bno086_on_report), [`depz_bno086_off_report`](#depz_bno086_off_report), [`depz_bno086_reports`](#depz_bno086_reports), [`depz_bno086_get_report`](#depz_bno086_get_report), [`depz_bno086_shtp_discarded`](#depz_bno086_shtp_discarded), [`depz_bno086_tare_now`](#depz_bno086_tare_now), [`depz_bno086_persist_tare`](#depz_bno086_persist_tare), [`depz_bno086_set_reorientation`](#depz_bno086_set_reorientation), [`depz_bno086_set_calibration`](#depz_bno086_set_calibration), [`depz_bno086_get_calibration`](#depz_bno086_get_calibration), [`depz_bno086_save_dcd`](#depz_bno086_save_dcd), [`depz_bno086_configure_periodic_dcd`](#depz_bno086_configure_periodic_dcd), [`depz_bno086_frs_read`](#depz_bno086_frs_read), [`depz_bno086_frs_write`](#depz_bno086_frs_write), [`depz_bno086_get_metadata`](#depz_bno086_get_metadata), [`depz_bno086_get_oscillator_type`](#depz_bno086_get_oscillator_type), [`depz_bno086_clear_dcd_and_reset`](#depz_bno086_clear_dcd_and_reset), [`depz_bno086_get_errors`](#depz_bno086_get_errors), [`depz_bno086_get_counts`](#depz_bno086_get_counts), [`depz_bno086_clear_counts`](#depz_bno086_clear_counts), [`depz_bno086_command`](#depz_bno086_command), [`depz_bno086_send_shtp`](#depz_bno086_send_shtp)

## Discovery

### DEPZ_USB_VID

```c
#define DEPZ_USB_VID   0x1BCFu
```

7119 — production VID for all DEPZ sensors

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

### DEPZ_PID_VL53L4CD

```c
#define DEPZ_PID_VL53L4CD 0xED45u
```

60741

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

1155 — STMicro dev/unprogrammed default

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

## VL53L4CD (ToF)

### depz_vl53l4_cmd

```c
typedef enum {
    DEPZ_VL53L4_CMD_READ_REG      = 0x32,
    DEPZ_VL53L4_CMD_WRITE_REG     = 0x33,
    DEPZ_VL53L4_CMD_XSHUT         = 0x34,
    DEPZ_VL53L4_CMD_START_STREAM  = 0x35,
    DEPZ_VL53L4_CMD_STOP_STREAM   = 0x36,
    DEPZ_VL53L4_CMD_GET_INFO      = 0x37,
    DEPZ_VL53L4_CMD_SET_I2C_SPEED = 0x38
} depz_vl53l4_cmd;
```

### depz_vl53l4_rpt

```c
typedef enum {
    DEPZ_VL53L4_RPT_REG_DATA = 0x91,
    DEPZ_VL53L4_RPT_INFO     = 0x92,
    DEPZ_VL53L4_RPT_STREAM   = 0x93
} depz_vl53l4_rpt;
```

### DEPZ_VL53L4_XFER_MAX

```c
#define DEPZ_VL53L4_XFER_MAX 253u
```

Max read length / write data length per transfer (STM32 I2C NBYTES is
8-bit; a write spends two bytes on the register address; the firmware
applies one number to both directions). addr + len must be <= 0x10000.

### DEPZ_VL53L4_XSHUT_OFF

```c
#define DEPZ_VL53L4_XSHUT_OFF   0u
```

VL53_XSHUT actions. RESET is answered after the boot handshake.

### DEPZ_VL53L4_XSHUT_ON

```c
#define DEPZ_VL53L4_XSHUT_ON    1u
```

### DEPZ_VL53L4_XSHUT_RESET

```c
#define DEPZ_VL53L4_XSHUT_RESET 2u
```

### DEPZ_VL53L4_SF_INT_ACT_HIGH

```c
#define DEPZ_VL53L4_SF_INT_ACT_HIGH 0x02u
```

VL53_START_STREAM flags bit 1: INT active high (mirrors bit 4 of
GPIO_HV_MUX__CTRL 0x0030). Clear (default) = INT active low.

### DEPZ_VL53L4_RESULT_BLOCK_ADDR

```c
#define DEPZ_VL53L4_RESULT_BLOCK_ADDR 0x0089u
```

The usual stream configuration: the whole result block in one read.

### DEPZ_VL53L4_RESULT_BLOCK_LEN

```c
#define DEPZ_VL53L4_RESULT_BLOCK_LEN  17u
```

### DEPZ_VL53L4_MODEL_ID

```c
#define DEPZ_VL53L4_MODEL_ID 0xEBAAu
```

IDENTIFICATION__MODEL_ID (0x010F) expected value.

### DEPZ_VL53L4_CONFIG_ADDR

```c
#define DEPZ_VL53L4_CONFIG_ADDR 0x2Du
```

First register of the 91-byte init configuration block (0x2D..0x87).

### DEPZ_VL53L4_CONFIG_FMP_BYTE

```c
#define DEPZ_VL53L4_CONFIG_FMP_BYTE 0x12u
```

Byte 0 of the config block is always forced to 0x12 (I2C Fast Mode Plus
pad, never cleared) — what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD.

### depz_vl53l4_pack_read_reg

```c
size_t depz_vl53l4_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out);
```

Encoders (return payload length written).
/* 4 B */

### depz_vl53l4_pack_write_reg

```c
size_t depz_vl53l4_pack_write_reg(uint16_t addr, const uint8_t *data, size_t data_len, uint8_t *out);
```

VL53_WRITE_REG payload: addr u16 + data. Returns 2 + data_len, or 0 when
data_len is outside 1..DEPZ_VL53L4_XFER_MAX.

### depz_vl53l4_pack_xshut

```c
size_t depz_vl53l4_pack_xshut(uint8_t action, uint8_t *out);
```

/* 1 B */

### depz_vl53l4_pack_start_stream

```c
size_t depz_vl53l4_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags, uint8_t *out);
```

/* 5 B */

### depz_vl53l4_pack_set_i2c_speed

```c
size_t depz_vl53l4_pack_set_i2c_speed(uint16_t khz, uint8_t *out);
```

/* 2 B */

### depz_vl53l4_reg_data

```c
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us; /* MCU uptime at I2C-read completion */
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_vl53l4_reg_data;
```

RPT_VL53_REG_DATA payload: echoed opcode, u64 timestamp, register bytes.

### depz_vl53l4_unpack_reg_data

```c
int depz_vl53l4_unpack_reg_data(const uint8_t *payload, size_t len, depz_vl53l4_reg_data *out);
```

/* needs >= 9 B */

### depz_vl53l4_info

```c
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
```

RPT_VL53_INFO — bridge diagnostics (21 B, little-endian). Counters are
free-running and wrap silently; watch increments, not absolute values.

### depz_vl53l4_unpack_info

```c
int depz_vl53l4_unpack_info(const uint8_t *payload, size_t len, depz_vl53l4_info *out);
```

### depz_vl53l4_stream

```c
typedef struct {
    uint64_t timestamp_us; /* MCU uptime at the INT edge (the sensor event) */
    uint16_t addr;
    uint16_t len;
    const uint8_t *data;   /* points into the report payload (len bytes) */
} depz_vl53l4_stream;
```

RPT_VL53_STREAM — one streamed register block. `addr`/`len` echo the
stream configuration so each report is self-describing.

### depz_vl53l4_unpack_stream

```c
int depz_vl53l4_unpack_stream(const uint8_t *payload, size_t len, depz_vl53l4_stream *out);
```

### depz_vl53l4_result

```c
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
```

VL53L4CD_ResultsData_t plus the sensor's own frame counter.

### depz_vl53l4_parse_result_block

```c
int depz_vl53l4_parse_result_block(const uint8_t *raw, size_t len, depz_vl53l4_result *out);
```

Decode the streamed 17-byte 0x0089..0x0099 block exactly as
VL53L4CD_GetResult() decodes the same registers read one by one. Register
contents are big-endian words (the bridge passes them through untouched).
Returns 0 on success, -1 when len < 15.

### depz_vl53l4_range_timing_registers

```c
int depz_vl53l4_range_timing_registers(uint32_t budget_ms, uint32_t inter_ms, uint16_t osc_frequency, uint16_t clock_pll, uint16_t *range_config_a, uint16_t *range_config_b, uint32_t *intermeasurement_raw);
```

SetRangeTiming register math -> RANGE_CONFIG_A (0x005E), RANGE_CONFIG_B
(0x0061) and the INTERMEASUREMENT_MS (0x006C) raw dword. `osc_frequency` is
the word read from 0x0006; `clock_pll` is the word read from
RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
inter_ms > 0). inter_ms == 0 selects continuous mode; a value greater than
the budget selects autonomous low power. Returns 0 on success, -1 when
osc_frequency == 0, budget_ms outside 10..200, or 0 < inter_ms <= budget_ms.

### depz_vl53l4_decode_range_timing

```c
int depz_vl53l4_decode_range_timing(uint32_t intermeasurement_raw, uint16_t clock_pll, uint16_t osc_frequency, uint16_t range_config_a, uint32_t *budget_ms, uint32_t *inter_ms);
```

GetRangeTiming register math -> (budget_ms, inter_ms) from the raw register
reads: the INTERMEASUREMENT_MS dword, the RESULT__OSC_CALIBRATE_VAL word,
the 0x0006 word and the RANGE_CONFIG_A word. Returns 0 on success, -1 when
osc_frequency == 0.

### depz_vl53l4_offset_raw

```c
uint16_t depz_vl53l4_offset_raw(int32_t mm);
```

Tuning word codecs (register word <-> user units).
/* RANGE_OFFSET_MM: mm*4 */

### depz_vl53l4_decode_offset

```c
int32_t depz_vl53l4_decode_offset(uint16_t raw);
```

/* -> signed millimetres */

### depz_vl53l4_xtalk_raw

```c
uint16_t depz_vl53l4_xtalk_raw(uint16_t kcps);
```

/* XTALK_PLANE_OFFSET: kcps*512 */

### depz_vl53l4_decode_xtalk

```c
uint16_t depz_vl53l4_decode_xtalk(uint16_t raw);
```

/* lround(raw/512.0) */

### depz_vl53l4_signal_threshold_raw

```c
uint16_t depz_vl53l4_signal_threshold_raw(uint16_t kcps);
```

/* kcps/8 */

### depz_vl53l4_decode_signal_threshold

```c
uint16_t depz_vl53l4_decode_signal_threshold(uint16_t raw);
```

/* raw*8 */

### depz_vl53l4_sigma_threshold_raw

```c
int depz_vl53l4_sigma_threshold_raw(uint16_t mm, uint16_t *raw);
```

RANGE_CONFIG__SIGMA_THRESH: mm*4. Returns 0 on success, -1 when mm > 16383.

### depz_vl53l4_decode_sigma_threshold

```c
uint16_t depz_vl53l4_decode_sigma_threshold(uint16_t raw);
```

/* raw/4 */

### DEPZ_VL53L4_DEFAULT_CONFIGURATION

```c
extern const uint8_t DEPZ_VL53L4_DEFAULT_CONFIGURATION[91];
```

VL53L4CD_DEFAULT_CONFIGURATION[] — the stock ST 91-byte block for registers
0x2D..0x87 (byte 0 as shipped, i.e. NOT the FM+ override).

### depz_vl53l4_config_block

```c
size_t depz_vl53l4_config_block(uint8_t *out);
```

Write the 91-byte block sensor_init() sends at DEPZ_VL53L4_CONFIG_ADDR: the
ST default configuration with byte 0 forced to DEPZ_VL53L4_CONFIG_FMP_BYTE
(I2C Fast Mode Plus). Returns 91.

## VL53L8 (ToF)

### depz_vl53l8_variant

```c
typedef enum {
    DEPZ_VL53L8_VARIANT_CX = 0, /* base ToF (dev-default)                    */
    DEPZ_VL53L8_VARIANT_CH = 1  /* CX + CNH compact-network-histograms (ED40)*/
} depz_vl53l8_variant;
```

Which ToF sensor produced a frame. CX and CH share the ranging-results
blocks but not the footer position: decode CX frames with
depz_vl53l8_decode_frame, CH frames with depz_vl53l8ch_decode_frame.

### DEPZ_VL53L8CX_FOOTER_ID_OFFSET

```c
#define DEPZ_VL53L8CX_FOOTER_ID_OFFSET 12
```

Footer-id offset in a decoded frame: the header id at [8..9] must equal
the footer id at [raw_len - N]. N = 12 for the VL53L8CX ULD 2.1.0 frame;
the CH (VL53LMZ) frame uses 4 — depz_vl53l8ch_decode_frame, which also
hands out the CNH block for depz_vl53l8ch_decode_cnh().

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

Frame reassembler (contract 04): reset on offset==0; chunks must be
contiguous (offset == accumulated length) and agree on full_size; a gap
discards the frame in progress; completes when accumulated == full_size.

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

Decode a reassembled raw VL53L8CX ranging frame (ULD 2.1.0 layout: the
footer id sits at raw_len - DEPZ_VL53L8CX_FOOTER_ID_OFFSET, 12). Returns 0
on success, -1 on corrupted frame (header/footer id mismatch), -2 on bad
length. `timestamp_us` is carried through from the reassembler.

VL53L8CH frames come from the VL53LMZ firmware, whose footer id sits at
raw_len - 4: this function rejects them (-1). Decode them with
depz_vl53l8ch_decode_frame().

### depz_vl53l8ch_decode_frame

```c
int depz_vl53l8ch_decode_frame(const uint8_t *raw, size_t raw_len, uint64_t timestamp_us, depz_vl53l8_frame *out, uint8_t *cnh_out, size_t cnh_cap, size_t *cnh_len);
```

Decode a reassembled raw VL53L8CH ranging frame: the same block walk as
depz_vl53l8_decode_frame(), with the VL53LMZ footer id at raw_len - 4 (the
Python / TS / C++ SDKs use the same variant-specific offset). `cnh_out`
(may be NULL) receives the CNH data block when the frame carries one — the
bytes depz_vl53l8ch_decode_cnh() takes; *cnh_len (may be NULL) is set to its
length, 0 when there is none. Returns 0, -1 (id mismatch), -2 (bad length)
or -3 (CNH block exceeds cnh_cap).

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
the pure codec half of the motion indicator (the live DCI write is
depz_vl53l8_configure_motion_indicator). Returns 0 on success, -1 on bad resolution.

## VL53L5CX / VL53L7CX / VL53L7CH (ToF)

### depz_vl53l7_model

```c
typedef enum {
    DEPZ_VL53L7_MODEL_L7CX = 0, /* base class (default)                      */
    DEPZ_VL53L7_MODEL_L5CX = 1, /* same API, 63° optics (PID 0xED48)         */
    DEPZ_VL53L7_MODEL_L7CH = 2  /* L7CX + CNH, as VL53L8CH (PID 0xED4A)      */
} depz_vl53l7_model;
```

Which sensor an APP_VL53L7 board opens as (contract 11 §1).

### DEPZ_VL53L7_CMD_PIN_CTRL

```c
#define DEPZ_VL53L7_CMD_PIN_CTRL      0x34
```

### DEPZ_VL53L7_CMD_GET_INFO

```c
#define DEPZ_VL53L7_CMD_GET_INFO      0x37
```

### DEPZ_VL53L7_CMD_SET_I2C_SPEED

```c
#define DEPZ_VL53L7_CMD_SET_I2C_SPEED 0x38
```

### DEPZ_VL53L7_RPT_INFO

```c
#define DEPZ_VL53L7_RPT_INFO          0x92
```

no echoed command byte

### DEPZ_VL53L7_PIN_LPN_OFF

```c
#define DEPZ_VL53L7_PIN_LPN_OFF    0u
```

VL53_PIN_CTRL actions. None is a true sensor reset (no power GPIO): after
LPN_OFF or SOFT_CYCLE the host must re-run init().
stop streaming, LPn low (I2C off)

### DEPZ_VL53L7_PIN_LPN_ON

```c
#define DEPZ_VL53L7_PIN_LPN_ON     1u
```

LPn high (power-up default)

### DEPZ_VL53L7_PIN_I2C_RST

```c
#define DEPZ_VL53L7_PIN_I2C_RST    2u
```

pulse I2C_RST

### DEPZ_VL53L7_PIN_SOFT_CYCLE

```c
#define DEPZ_VL53L7_PIN_SOFT_CYCLE 3u
```

stop, LPn low 1 ms, high, I2C_RST; clears the I2C error counters

### DEPZ_VL53L7_I2C_OK

```c
#define DEPZ_VL53L7_I2C_OK        0u
```

RPT_VL53_INFO.last_i2c_error

### DEPZ_VL53L7_I2C_NACK

```c
#define DEPZ_VL53L7_I2C_NACK      1u
```

### DEPZ_VL53L7_I2C_TIMEOUT

```c
#define DEPZ_VL53L7_I2C_TIMEOUT   2u
```

### DEPZ_VL53L7_I2C_BUS_ERROR

```c
#define DEPZ_VL53L7_I2C_BUS_ERROR 3u
```

### DEPZ_VL53L7_READ_MAX_LEN

```c
#define DEPZ_VL53L7_READ_MAX_LEN     1536u
```

READ_REG len 1..1536 (L8: 2048)

### DEPZ_VL53L7_WRITE_MAX_LEN

```c
#define DEPZ_VL53L7_WRITE_MAX_LEN    2048u
```

WRITE_REG N 1..2048

### DEPZ_VL53L7_STREAM_CHUNK_MAX

```c
#define DEPZ_VL53L7_STREAM_CHUNK_MAX 1536u
```

frame bytes per RPT_VL53_FRAME

### DEPZ_VL53L7_INFO_SIZE

```c
#define DEPZ_VL53L7_INFO_SIZE        20u
```

### DEPZ_VL53L7_FOOTER_ID_OFFSET

```c
#define DEPZ_VL53L7_FOOTER_ID_OFFSET 4
```

Header id at [8..9] must equal the footer id at [raw_len - 4] (both the
l7cx and l7ch blob sets; VL53L8CX uses 12).

### depz_vl53l7_pack_read_reg

```c
size_t depz_vl53l7_pack_read_reg(uint16_t addr, uint16_t len, uint8_t *out);
```

Encoders (return payload length written). READ_REG / WRITE_REG return 0
when the length is outside 1..READ_MAX_LEN / 1..WRITE_MAX_LEN or
addr + length > 0x10000 (the firmware would answer ERR_INVALID_PARAM).
/* 4 B */

### depz_vl53l7_pack_write_reg

```c
size_t depz_vl53l7_pack_write_reg(uint16_t addr, const uint8_t *data, size_t data_len, uint8_t *out);
```

/* 2 + N B */

### depz_vl53l7_pack_pin_ctrl

```c
size_t depz_vl53l7_pack_pin_ctrl(uint8_t action, uint8_t *out);
```

/* 1 B */

### depz_vl53l7_pack_set_i2c_speed

```c
size_t depz_vl53l7_pack_set_i2c_speed(uint16_t khz, uint8_t *out);
```

/* 2 B */

### depz_vl53l7_info

```c
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
```

RPT_VL53_INFO — bridge state only (20 B, little-endian `<IIIBBBHHB`).
Counters run from power-up / DEVICE_RESET; SOFT_CYCLE clears the I2C ones.

### depz_vl53l7_unpack_info

```c
int depz_vl53l7_unpack_info(const uint8_t *payload, size_t len, depz_vl53l7_info *out);
```

Returns 0 on success, -1 when len < DEPZ_VL53L7_INFO_SIZE.

### depz_vl53l7_resolve_model

```c
depz_vl53l7_model depz_vl53l7_resolve_model(const char *usb_model, const char *device_name);
```

Class resolution (normative order, contract 11 §1): the production USB PID
model (`usb_model` as returned by depz_usb_model_hint(): "vl53l5cx" |
"vl53l7cx" | "vl53l7ch"; NULL or anything else = none), else the first
match of `VL53L([57])(CX|CH)` in GET_DEVICE_NAME (`device_name`, may be
NULL), else DEPZ_VL53L7_MODEL_L7CX.

### depz_vl53l7_model_str

```c
const char *depz_vl53l7_model_str(depz_vl53l7_model m);
```

/* "vl53l7cx", ... */

### depz_vl53l7_decode_frame

```c
int depz_vl53l7_decode_frame(const uint8_t *raw, size_t raw_len, uint64_t timestamp_us, depz_vl53l8_frame *out, uint8_t *cnh_out, size_t cnh_cap, size_t *cnh_len);
```

Decode a reassembled VL53L5CX/L7CX/L7CH ranging frame (shared block walk
with depz_vl53l8_decode_frame). Differences: the footer id sits at
raw_len - DEPZ_VL53L7_FOOTER_ID_OFFSET, and every per-zone array is trimmed
to the frame's resolution — on L5/L7 the per-target blocks carry 64
entries even in 4x4. The resolution is read from the zone-sized ambient
block (index 0x54D0), so out->resolution is 16 or 64 and arrays past it
are zero.

`cnh_out` (may be NULL) receives the CNH data block (VL53L7CH with CNH
configured) in decode order — the bytes depz_vl53l8ch_decode_cnh() takes;
*cnh_len (may be NULL) is set to its length, 0 when the frame has none.
Returns 0 on success, -1 on header/footer id mismatch, -2 on bad length,
-3 when the CNH block exceeds cnh_cap.

## VL53L 1D family (VL53L0X / L1CX / L1CB / L3CX / L4CX)

### DEPZ_VL53LX_CMD_READ_REG

```c
#define DEPZ_VL53LX_CMD_READ_REG       0x32
```

= DEPZ_VL53L4_CMD_READ_REG

### DEPZ_VL53LX_CMD_WRITE_REG

```c
#define DEPZ_VL53LX_CMD_WRITE_REG      0x33
```

### DEPZ_VL53LX_CMD_XSHUT

```c
#define DEPZ_VL53LX_CMD_XSHUT          0x34
```

RESET: no boot handshake (v2)

### DEPZ_VL53LX_CMD_START_STREAM

```c
#define DEPZ_VL53LX_CMD_START_STREAM   0x35
```

### DEPZ_VL53LX_CMD_STOP_STREAM

```c
#define DEPZ_VL53LX_CMD_STOP_STREAM    0x36
```

### DEPZ_VL53LX_CMD_GET_INFO

```c
#define DEPZ_VL53LX_CMD_GET_INFO       0x37
```

### DEPZ_VL53LX_CMD_SET_I2C_SPEED

```c
#define DEPZ_VL53LX_CMD_SET_I2C_SPEED  0x38
```

### DEPZ_VL53LX_CMD_SET_ADDR_WIDTH

```c
#define DEPZ_VL53LX_CMD_SET_ADDR_WIDTH 0x39
```

new in v2.00

### DEPZ_VL53LX_CMD_CLEAR_I2C_ERRORS

```c
#define DEPZ_VL53LX_CMD_CLEAR_I2C_ERRORS 0x3A
```

v2.01 (fw v0.24), no payload; sent after every sensor init

### DEPZ_VL53LX_RPT_REG_DATA

```c
#define DEPZ_VL53LX_RPT_REG_DATA       0x91
```

### DEPZ_VL53LX_RPT_INFO

```c
#define DEPZ_VL53LX_RPT_INFO           0x92
```

### DEPZ_VL53LX_RPT_STREAM

```c
#define DEPZ_VL53LX_RPT_STREAM         0x93
```

### DEPZ_VL53LX_CLEAR_STEPS_MAX

```c
#define DEPZ_VL53LX_CLEAR_STEPS_MAX 4u
```

interrupt-release steps per stream

### DEPZ_VL53LX_START_STREAM_MAX

```c
#define DEPZ_VL53LX_START_STREAM_MAX (6u + 3u * DEPZ_VL53LX_CLEAR_STEPS_MAX)
```

### DEPZ_VL53LX_INFO_SIZE

```c
#define DEPZ_VL53LX_INFO_SIZE 23u
```

### depz_vl53lx_clear_step

```c
typedef struct {
    uint16_t addr;
    uint8_t  value;
} depz_vl53lx_clear_step;
```

One interrupt-release write the bridge plays after every block read.

### depz_vl53lx_pack_set_addr_width

```c
size_t depz_vl53lx_pack_set_addr_width(uint8_t width, uint8_t *out);
```

VL53_SET_ADDR_WIDTH payload (1 B). Returns 0 when width is not 1 or 2.

### depz_vl53lx_pack_start_stream

```c
size_t depz_vl53lx_pack_start_stream(uint16_t addr, uint16_t len, uint8_t flags, const depz_vl53lx_clear_step *clear, size_t n_clear, uint8_t *out);
```

VL53_START_STREAM payload: addr u16, len u16, flags u8, n_clear u8, then
n_clear x {addr u16, value u8} — 6 + 3n bytes (out needs
DEPZ_VL53LX_START_STREAM_MAX). Returns 0 when n_clear > 4. `clear` may be
NULL when n_clear == 0. flags: DEPZ_VL53L4_SF_INT_ACT_HIGH as contract 10.

### depz_vl53lx_info

```c
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
```

RPT_VL53_INFO v2.00 (0x92, 23 B `<IIIBBBHBBI`) — bridge state only.

### depz_vl53lx_unpack_info

```c
int depz_vl53lx_unpack_info(const uint8_t *payload, size_t len, depz_vl53lx_info *out);
```

Returns 0 on success, -1 when len < DEPZ_VL53LX_INFO_SIZE.

### depz_vl53lx_product

```c
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
```

### depz_vl53lx_driver

```c
typedef enum {
    DEPZ_VL53LX_DRIVER_ULD       = 1 << 0, /* ST Ultra Lite Driver           */
    DEPZ_VL53LX_DRIVER_ULP       = 1 << 1, /* Ultra Low Power (L3CX only)    */
    DEPZ_VL53LX_DRIVER_HISTOGRAM = 1 << 2  /* Bare Driver: 24 bins, host     */
} depz_vl53lx_driver;
```

Driver kinds, as a bitmask in depz_vl53lx_product_info.driver_kinds.

### depz_vl53lx_product_info

```c
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
```

### depz_vl53lx_product_get

```c
const depz_vl53lx_product_info *depz_vl53lx_product_get(depz_vl53lx_product p);
```

Row of the table, or NULL for NONE / out of range.

### depz_vl53lx_driver_str

```c
const char *depz_vl53lx_driver_str(depz_vl53lx_driver d);
```

"uld" | "ulp" | "histogram" (NULL for anything else).

### depz_vl53lx_product_from_str

```c
depz_vl53lx_product depz_vl53lx_product_from_str(const char *name);
```

Exact product name, case-insensitive ("vl53l4cx" -> L4CX), else NONE.

### depz_vl53lx_product_from_board_name

```c
depz_vl53lx_product depz_vl53lx_product_from_board_name(const char *name);
```

Product named on the board (GET_DEVICE_NAME): the first
`VL53L<digit>[A-Z0-9]*` in the upper-cased name, NONE when there is no
match or the match is not a family product. `name` may be NULL.

### depz_vl53lx_resolve_product

```c
depz_vl53lx_product depz_vl53lx_resolve_product(const char *usb_model, const char *device_name);
```

The PID model (`usb_model` as from depz_usb_model_hint(), may be NULL) if
it names a family product, else depz_vl53lx_product_from_board_name().

### depz_vl53lx_class

```c
typedef enum {
    DEPZ_VL53LX_CLASS_GENERIC = 0, /* "vl53lx": takes the product at init   */
    DEPZ_VL53LX_CLASS_L0X,
    DEPZ_VL53LX_CLASS_L1CX,
    DEPZ_VL53LX_CLASS_L1CB,
    DEPZ_VL53LX_CLASS_L3CX,
    DEPZ_VL53LX_CLASS_L4CX
} depz_vl53lx_class;
```

Which class an APP_VL53L0_4 board opens as (contract 12 §1). VL53L4CD has
no class of its own here (its vl53l4cd class belongs to APP_VL53L4): it,
and an unresolved product, open as the generic class.

### depz_vl53lx_resolve_class

```c
depz_vl53lx_class depz_vl53lx_resolve_class(const char *usb_model, const char *device_name);
```

### depz_vl53lx_class_str

```c
const char *depz_vl53lx_class_str(depz_vl53lx_class c);
```

/* "vl53lx", "vl53l0x", ... */

### DEPZ_VL53LX_DIE_BLOCK_ADDR

```c
#define DEPZ_VL53LX_DIE_BLOCK_ADDR       0x0089u
```

### DEPZ_VL53LX_DIE_BLOCK_LEN

```c
#define DEPZ_VL53LX_DIE_BLOCK_LEN        17u
```

### DEPZ_VL53LX_L0X_BLOCK_ADDR

```c
#define DEPZ_VL53LX_L0X_BLOCK_ADDR       0x14u
```

### DEPZ_VL53LX_L0X_BLOCK_LEN

```c
#define DEPZ_VL53LX_L0X_BLOCK_LEN        12u
```

### DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR

```c
#define DEPZ_VL53LX_HISTOGRAM_BLOCK_ADDR 0x0088u
```

### DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN

```c
#define DEPZ_VL53LX_HISTOGRAM_BLOCK_LEN  83u
```

### DEPZ_VL53LX_HISTOGRAM_BINS

```c
#define DEPZ_VL53LX_HISTOGRAM_BINS       24u
```

### depz_vl53lx_die_variant

```c
typedef enum {
    DEPZ_VL53LX_DIE_L4 = 0, /* VL53L4CD ULD, L3CX ULP: (5, 256) — exactly
                             * depz_vl53l4_parse_result_block()           */
    DEPZ_VL53LX_DIE_L1 = 1  /* VL53L1X ULD: (15, 25), crosstalk-corrected
                             * peak signal at 0x0098                      */
} depz_vl53lx_die_variant;
```

Which ULD reads the 17-byte die block: (signal offset, per-SPAD K).

### depz_vl53lx_decode_die_block

```c
int depz_vl53lx_decode_die_block(const uint8_t *raw, size_t len, depz_vl53lx_die_variant variant, depz_vl53l4_result *out);
```

Decode the die block (0x0089..0x0099) as `variant` reads it. Returns 0 on
success, -1 when len < 17 or the variant is unknown.

### depz_vl53lx_l0x_raw

```c
typedef struct {
    uint16_t distance_raw;            /* mm (quarter-mm if RangeFractional) */
    uint8_t  device_range_status;     /* raw byte 0                         */
    uint32_t signal_rate_mcps_1616;   /* FixPoint16.16 Mcps (wire 9.7 << 9) */
    uint32_t ambient_rate_mcps_1616;
    uint16_t effective_spad_count_88; /* 8.8                                */
} depz_vl53lx_l0x_raw;
```

Raw fields of the VL53L0X 12-byte block at 0x14. The PAL range status,
sigma and dmax need the device data cached by init — full driver only.

### depz_vl53lx_decode_l0x_raw

```c
int depz_vl53lx_decode_l0x_raw(const uint8_t *raw, size_t len, depz_vl53lx_l0x_raw *out);
```

Returns 0 on success, -1 when len < 12.

### depz_vl53lx_histogram_raw

```c
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
```

Status bytes and the 24 photon bins of the 83-byte histogram block at
0x0088. Turning bins into targets is the full driver's job.

### depz_vl53lx_decode_histogram_raw

```c
int depz_vl53lx_decode_histogram_raw(const uint8_t *raw, size_t len, depz_vl53lx_histogram_raw *out);
```

Bin 23's low byte is rebuilt from its MSB/LSB pair ((MSB << 2) + LSB,
truncated to 8 bits) before the bins are read; `raw` is not modified.
Returns 0 on success, -1 when len < 83.

## BNO086 (IMU)

### DEPZ_BNO086_CMD_SENSOR_RESET

```c
#define DEPZ_BNO086_CMD_SENSOR_RESET      0x32
```

Bridge commands and report (contract 05 §1). SEND_SHTP_PACKET is answered
with RPT_STATUS at once (OK, or ERR_BUSY: retry after >= 200 ms); every
inbound SHTP frame arrives as RPT_DATA with cmd 0 (ERRATA E2).

### DEPZ_BNO086_CMD_SENSOR_WAKE_UP

```c
#define DEPZ_BNO086_CMD_SENSOR_WAKE_UP    0x33
```

### DEPZ_BNO086_CMD_SEND_SHTP_PACKET

```c
#define DEPZ_BNO086_CMD_SEND_SHTP_PACKET  0x34
```

### DEPZ_BNO086_RPT_DATA

```c
#define DEPZ_BNO086_RPT_DATA              0x91
```

### depz_bno086_unpack_data

```c
int depz_bno086_unpack_data(const uint8_t *p, size_t len, uint64_t *capture_us, const uint8_t **shtp, size_t *shtp_len);
```

RPT_DATA payload: cmd u8, capture timestamp u64 (MCU µs), SHTP frame.
Returns 0 (the frame points into `p`), -1 when shorter than 9 bytes.

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

### DEPZ_BNO_SENSOR_ACCELEROMETER

```c
#define DEPZ_BNO_SENSOR_ACCELEROMETER            0x01u
```

SH-2 sensor ids (the input report ids) — what Set Feature takes, and the
`sensor_id` of a report. Not the depz_bno_report_type catalog.

### DEPZ_BNO_SENSOR_GYROSCOPE

```c
#define DEPZ_BNO_SENSOR_GYROSCOPE                0x02u
```

### DEPZ_BNO_SENSOR_MAGNETOMETER

```c
#define DEPZ_BNO_SENSOR_MAGNETOMETER             0x03u
```

### DEPZ_BNO_SENSOR_LINEAR_ACCELERATION

```c
#define DEPZ_BNO_SENSOR_LINEAR_ACCELERATION      0x04u
```

### DEPZ_BNO_SENSOR_ROTATION_VECTOR

```c
#define DEPZ_BNO_SENSOR_ROTATION_VECTOR          0x05u
```

### DEPZ_BNO_SENSOR_GRAVITY

```c
#define DEPZ_BNO_SENSOR_GRAVITY                  0x06u
```

### DEPZ_BNO_SENSOR_UNCALIBRATED_GYROSCOPE

```c
#define DEPZ_BNO_SENSOR_UNCALIBRATED_GYROSCOPE   0x07u
```

### DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR

```c
#define DEPZ_BNO_SENSOR_GAME_ROTATION_VECTOR     0x08u
```

### DEPZ_BNO_SENSOR_GEOMAGNETIC_ROTATION_VECTOR

```c
#define DEPZ_BNO_SENSOR_GEOMAGNETIC_ROTATION_VECTOR 0x09u
```

### DEPZ_BNO_SENSOR_UNCALIBRATED_MAGNETOMETER

```c
#define DEPZ_BNO_SENSOR_UNCALIBRATED_MAGNETOMETER 0x0Fu
```

### DEPZ_BNO_SENSOR_TAP_DETECTOR

```c
#define DEPZ_BNO_SENSOR_TAP_DETECTOR             0x10u
```

### DEPZ_BNO_SENSOR_STEP_COUNTER

```c
#define DEPZ_BNO_SENSOR_STEP_COUNTER             0x11u
```

### DEPZ_BNO_SENSOR_SIGNIFICANT_MOTION

```c
#define DEPZ_BNO_SENSOR_SIGNIFICANT_MOTION       0x12u
```

### DEPZ_BNO_SENSOR_STABILITY_CLASSIFIER

```c
#define DEPZ_BNO_SENSOR_STABILITY_CLASSIFIER     0x13u
```

### DEPZ_BNO_SENSOR_RAW_ACCELEROMETER

```c
#define DEPZ_BNO_SENSOR_RAW_ACCELEROMETER        0x14u
```

### DEPZ_BNO_SENSOR_RAW_GYROSCOPE

```c
#define DEPZ_BNO_SENSOR_RAW_GYROSCOPE            0x15u
```

### DEPZ_BNO_SENSOR_RAW_MAGNETOMETER

```c
#define DEPZ_BNO_SENSOR_RAW_MAGNETOMETER         0x16u
```

### DEPZ_BNO_SENSOR_STEP_DETECTOR

```c
#define DEPZ_BNO_SENSOR_STEP_DETECTOR            0x18u
```

### DEPZ_BNO_SENSOR_SHAKE_DETECTOR

```c
#define DEPZ_BNO_SENSOR_SHAKE_DETECTOR           0x19u
```

### DEPZ_BNO_SENSOR_FLIP_DETECTOR

```c
#define DEPZ_BNO_SENSOR_FLIP_DETECTOR            0x1Au
```

### DEPZ_BNO_SENSOR_PICKUP_DETECTOR

```c
#define DEPZ_BNO_SENSOR_PICKUP_DETECTOR          0x1Bu
```

### DEPZ_BNO_SENSOR_STABILITY_DETECTOR

```c
#define DEPZ_BNO_SENSOR_STABILITY_DETECTOR       0x1Cu
```

### DEPZ_BNO_SENSOR_PERSONAL_ACTIVITY_CLASSIFIER

```c
#define DEPZ_BNO_SENSOR_PERSONAL_ACTIVITY_CLASSIFIER 0x1Eu
```

### DEPZ_BNO_SENSOR_SLEEP_DETECTOR

```c
#define DEPZ_BNO_SENSOR_SLEEP_DETECTOR           0x1Fu
```

### DEPZ_BNO_SENSOR_TILT_DETECTOR

```c
#define DEPZ_BNO_SENSOR_TILT_DETECTOR            0x20u
```

### DEPZ_BNO_SENSOR_POCKET_DETECTOR

```c
#define DEPZ_BNO_SENSOR_POCKET_DETECTOR          0x21u
```

### DEPZ_BNO_SENSOR_CIRCLE_DETECTOR

```c
#define DEPZ_BNO_SENSOR_CIRCLE_DETECTOR          0x22u
```

### DEPZ_BNO_SENSOR_HEART_RATE_MONITOR

```c
#define DEPZ_BNO_SENSOR_HEART_RATE_MONITOR       0x23u
```

### DEPZ_BNO_SENSOR_ARVR_STABILIZED_RV

```c
#define DEPZ_BNO_SENSOR_ARVR_STABILIZED_RV       0x28u
```

### DEPZ_BNO_SENSOR_ARVR_STABILIZED_GAME_RV

```c
#define DEPZ_BNO_SENSOR_ARVR_STABILIZED_GAME_RV  0x29u
```

### DEPZ_BNO_SENSOR_GYRO_INTEGRATED_RV

```c
#define DEPZ_BNO_SENSOR_GYRO_INTEGRATED_RV       0x2Au
```

### DEPZ_SH2_COMMAND_RESPONSE

```c
#define DEPZ_SH2_COMMAND_RESPONSE     0xF1
```

Control-channel report ids.

### DEPZ_SH2_COMMAND_REQUEST

```c
#define DEPZ_SH2_COMMAND_REQUEST      0xF2
```

### DEPZ_SH2_FRS_READ_RESPONSE

```c
#define DEPZ_SH2_FRS_READ_RESPONSE    0xF3
```

### DEPZ_SH2_FRS_READ_REQUEST

```c
#define DEPZ_SH2_FRS_READ_REQUEST     0xF4
```

### DEPZ_SH2_FRS_WRITE_RESPONSE

```c
#define DEPZ_SH2_FRS_WRITE_RESPONSE   0xF5
```

### DEPZ_SH2_FRS_WRITE_DATA

```c
#define DEPZ_SH2_FRS_WRITE_DATA       0xF6
```

### DEPZ_SH2_FRS_WRITE_REQUEST

```c
#define DEPZ_SH2_FRS_WRITE_REQUEST    0xF7
```

### DEPZ_SH2_PRODUCT_ID_RESPONSE

```c
#define DEPZ_SH2_PRODUCT_ID_RESPONSE  0xF8
```

### DEPZ_SH2_PRODUCT_ID_REQUEST

```c
#define DEPZ_SH2_PRODUCT_ID_REQUEST   0xF9
```

### DEPZ_SH2_GET_FEATURE_RESPONSE

```c
#define DEPZ_SH2_GET_FEATURE_RESPONSE 0xFC
```

### DEPZ_SH2_SET_FEATURE_COMMAND

```c
#define DEPZ_SH2_SET_FEATURE_COMMAND  0xFD
```

### DEPZ_SH2_GET_FEATURE_REQUEST

```c
#define DEPZ_SH2_GET_FEATURE_REQUEST  0xFE
```

### DEPZ_SH2_CMD_ERRORS

```c
#define DEPZ_SH2_CMD_ERRORS              0x01
```

Command Request `command` values.

### DEPZ_SH2_CMD_COUNTER

```c
#define DEPZ_SH2_CMD_COUNTER             0x02
```

### DEPZ_SH2_CMD_TARE

```c
#define DEPZ_SH2_CMD_TARE                0x03
```

### DEPZ_SH2_CMD_INITIALIZE

```c
#define DEPZ_SH2_CMD_INITIALIZE          0x04
```

### DEPZ_SH2_CMD_SAVE_DCD

```c
#define DEPZ_SH2_CMD_SAVE_DCD            0x06
```

### DEPZ_SH2_CMD_ME_CALIBRATE

```c
#define DEPZ_SH2_CMD_ME_CALIBRATE        0x07
```

### DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG

```c
#define DEPZ_SH2_CMD_PERIODIC_DCD_CONFIG 0x09
```

### DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE

```c
#define DEPZ_SH2_CMD_GET_OSCILLATOR_TYPE 0x0A
```

### DEPZ_SH2_CMD_CLEAR_DCD_AND_RESET

```c
#define DEPZ_SH2_CMD_CLEAR_DCD_AND_RESET 0x0B
```

### DEPZ_BNO_TARE_X

```c
#define DEPZ_BNO_TARE_X   1u
```

Tare axes (bitmap) and basis (the rotation vector tared against).

### DEPZ_BNO_TARE_Y

```c
#define DEPZ_BNO_TARE_Y   2u
```

### DEPZ_BNO_TARE_Z

```c
#define DEPZ_BNO_TARE_Z   4u
```

### DEPZ_BNO_TARE_ALL

```c
#define DEPZ_BNO_TARE_ALL 7u
```

### DEPZ_BNO_TARE_BASIS_RV

```c
#define DEPZ_BNO_TARE_BASIS_RV            0u
```

### DEPZ_BNO_TARE_BASIS_GAME_RV

```c
#define DEPZ_BNO_TARE_BASIS_GAME_RV       1u
```

### DEPZ_BNO_TARE_BASIS_GEOMAG_RV

```c
#define DEPZ_BNO_TARE_BASIS_GEOMAG_RV     2u
```

### DEPZ_BNO_TARE_BASIS_GYRO_RV

```c
#define DEPZ_BNO_TARE_BASIS_GYRO_RV       3u
```

### DEPZ_BNO_TARE_BASIS_ARVR_RV

```c
#define DEPZ_BNO_TARE_BASIS_ARVR_RV       4u
```

### DEPZ_BNO_TARE_BASIS_ARVR_GAME_RV

```c
#define DEPZ_BNO_TARE_BASIS_ARVR_GAME_RV  5u
```

### DEPZ_BNO_OSC_INTERNAL

```c
#define DEPZ_BNO_OSC_INTERNAL     0u
```

Get Oscillator Type results; error-record source 255 ends the queue.

### DEPZ_BNO_OSC_EXT_CRYSTAL

```c
#define DEPZ_BNO_OSC_EXT_CRYSTAL  1u
```

### DEPZ_BNO_OSC_EXT_CLOCK

```c
#define DEPZ_BNO_OSC_EXT_CLOCK    2u
```

### DEPZ_BNO_ERR_SOURCE_NO_MORE

```c
#define DEPZ_BNO_ERR_SOURCE_NO_MORE 255u
```

### DEPZ_BNO_FRS_STATIC_CALIBRATION_AGM

```c
#define DEPZ_BNO_FRS_STATIC_CALIBRATION_AGM 0x7979u
```

FRS record ids used by the SDK (SH-2 figure 28).

### DEPZ_BNO_FRS_NOMINAL_CALIBRATION

```c
#define DEPZ_BNO_FRS_NOMINAL_CALIBRATION    0x4D4Du
```

### DEPZ_BNO_FRS_DYNAMIC_CALIBRATION

```c
#define DEPZ_BNO_FRS_DYNAMIC_CALIBRATION    0x1F1Fu
```

### DEPZ_BNO_FRS_ME_POWER_MGMT

```c
#define DEPZ_BNO_FRS_ME_POWER_MGMT          0xD3E2u
```

### DEPZ_BNO_FRS_SYSTEM_ORIENTATION

```c
#define DEPZ_BNO_FRS_SYSTEM_ORIENTATION     0x2D3Eu
```

4 x Q30 words

### DEPZ_BNO_FRS_ACCEL_ORIENTATION

```c
#define DEPZ_BNO_FRS_ACCEL_ORIENTATION      0x2D41u
```

### DEPZ_BNO_FRS_GYROSCOPE_ORIENTATION

```c
#define DEPZ_BNO_FRS_GYROSCOPE_ORIENTATION  0x2D46u
```

### DEPZ_BNO_FRS_MAGNETOMETER_ORIENTATION

```c
#define DEPZ_BNO_FRS_MAGNETOMETER_ORIENTATION 0x2D4Cu
```

### DEPZ_BNO_FRS_ARVR_STABILIZATION_RV

```c
#define DEPZ_BNO_FRS_ARVR_STABILIZATION_RV  0x3E2Du
```

### DEPZ_BNO_FRS_ARVR_STABILIZATION_GRV

```c
#define DEPZ_BNO_FRS_ARVR_STABILIZATION_GRV 0x3E2Eu
```

### DEPZ_BNO_FRS_SIG_MOTION_DETECT_CONFIG

```c
#define DEPZ_BNO_FRS_SIG_MOTION_DETECT_CONFIG 0xC274u
```

### DEPZ_BNO_FRS_SHAKE_DETECT_CONFIG

```c
#define DEPZ_BNO_FRS_SHAKE_DETECT_CONFIG    0x7D7Du
```

### DEPZ_BNO_FRS_STABILITY_DETECTOR_CONFIG

```c
#define DEPZ_BNO_FRS_STABILITY_DETECTOR_CONFIG 0xED85u
```

### DEPZ_BNO_FRS_ACTIVITY_TRACKER_CONFIG

```c
#define DEPZ_BNO_FRS_ACTIVITY_TRACKER_CONFIG 0xED88u
```

### DEPZ_BNO_FRS_READ_NO_ERROR

```c
#define DEPZ_BNO_FRS_READ_NO_ERROR               0u
```

FRS read statuses (low nibble) and write statuses.

### DEPZ_BNO_FRS_READ_UNRECOGNIZED_TYPE

```c
#define DEPZ_BNO_FRS_READ_UNRECOGNIZED_TYPE      1u
```

### DEPZ_BNO_FRS_READ_BUSY

```c
#define DEPZ_BNO_FRS_READ_BUSY                   2u
```

### DEPZ_BNO_FRS_READ_COMPLETED

```c
#define DEPZ_BNO_FRS_READ_COMPLETED              3u
```

### DEPZ_BNO_FRS_READ_OFFSET_OUT_OF_RANGE

```c
#define DEPZ_BNO_FRS_READ_OFFSET_OUT_OF_RANGE    4u
```

### DEPZ_BNO_FRS_READ_RECORD_EMPTY

```c
#define DEPZ_BNO_FRS_READ_RECORD_EMPTY           5u
```

### DEPZ_BNO_FRS_READ_BLOCK_COMPLETED

```c
#define DEPZ_BNO_FRS_READ_BLOCK_COMPLETED        6u
```

### DEPZ_BNO_FRS_READ_BLOCK_AND_READ_COMPLETED

```c
#define DEPZ_BNO_FRS_READ_BLOCK_AND_READ_COMPLETED 7u
```

### DEPZ_BNO_FRS_READ_DEVICE_ERROR

```c
#define DEPZ_BNO_FRS_READ_DEVICE_ERROR           8u
```

### DEPZ_BNO_FRS_WRITE_WORDS_RECEIVED

```c
#define DEPZ_BNO_FRS_WRITE_WORDS_RECEIVED        0u
```

### DEPZ_BNO_FRS_WRITE_UNRECOGNIZED_TYPE

```c
#define DEPZ_BNO_FRS_WRITE_UNRECOGNIZED_TYPE     1u
```

### DEPZ_BNO_FRS_WRITE_BUSY

```c
#define DEPZ_BNO_FRS_WRITE_BUSY                  2u
```

### DEPZ_BNO_FRS_WRITE_COMPLETED

```c
#define DEPZ_BNO_FRS_WRITE_COMPLETED             3u
```

### DEPZ_BNO_FRS_WRITE_MODE_READY

```c
#define DEPZ_BNO_FRS_WRITE_MODE_READY            4u
```

### DEPZ_BNO_FRS_WRITE_FAILED

```c
#define DEPZ_BNO_FRS_WRITE_FAILED                5u
```

### DEPZ_BNO_FRS_WRITE_NOT_IN_WRITE_MODE

```c
#define DEPZ_BNO_FRS_WRITE_NOT_IN_WRITE_MODE     6u
```

### DEPZ_BNO_FRS_WRITE_INVALID_LENGTH

```c
#define DEPZ_BNO_FRS_WRITE_INVALID_LENGTH        7u
```

### DEPZ_BNO_FRS_WRITE_RECORD_VALID

```c
#define DEPZ_BNO_FRS_WRITE_RECORD_VALID          8u
```

### DEPZ_BNO_FRS_WRITE_RECORD_INVALID

```c
#define DEPZ_BNO_FRS_WRITE_RECORD_INVALID        9u
```

### depz_bno_feature

```c
typedef struct {
    uint8_t  sensor_id, flags;
    uint16_t sensitivity;
    uint32_t interval_us;   /* granted report interval; 0 = disabled */
    uint32_t batch_us;
    uint32_t cfg_word;
} depz_bno_feature;
```

Get Feature Response 0xFC (17 B): the rates in effect.

### depz_bno_product_id

```c
typedef struct {
    uint8_t  reset_cause, sw_version_major, sw_version_minor;
    uint32_t sw_part_number;  /* 10004148 = BNO085, 10004563 = BNO086 */
    uint32_t sw_build_number;
    uint16_t sw_version_patch;
} depz_bno_product_id;
```

Product ID Response 0xF8 (16 B), one per subsystem.

### depz_bno_command_response

```c
typedef struct {
    uint8_t seq, command, command_seq, response_seq;
    uint8_t r[11];
} depz_bno_command_response;
```

Command Response 0xF1 (16 B). r[0] is the status for most commands.

### depz_bno_frs_read_response

```c
typedef struct {
    uint8_t  status;        /* DEPZ_BNO_FRS_READ_* */
    uint8_t  data_length;   /* valid words in data0 / data1 (0..2) */
    uint16_t offset_words;
    uint32_t data0, data1;
    uint16_t frs_type;
} depz_bno_frs_read_response;
```

FRS Read Response 0xF3 (16 B): up to two words per packet.

### depz_bno_frs_write_response

```c
typedef struct {
    uint8_t  status;        /* DEPZ_BNO_FRS_WRITE_* */
    uint16_t offset_words;
} depz_bno_frs_write_response;
```

FRS Write Response 0xF5 (4 B).

### depz_bno_unpack_feature_response

```c
int depz_bno_unpack_feature_response(const uint8_t *p, size_t len, depz_bno_feature *out);
```

Each returns 0, or -1 when `p` is too short or not that report.

### depz_bno_unpack_product_id

```c
int depz_bno_unpack_product_id(const uint8_t *p, size_t len, depz_bno_product_id *out);
```

### depz_bno_unpack_command_response

```c
int depz_bno_unpack_command_response(const uint8_t *p, size_t len, depz_bno_command_response *out);
```

### depz_bno_unpack_frs_read_response

```c
int depz_bno_unpack_frs_read_response(const uint8_t *p, size_t len, depz_bno_frs_read_response *out);
```

### depz_bno_unpack_frs_write_response

```c
int depz_bno_unpack_frs_write_response(const uint8_t *p, size_t len, depz_bno_frs_write_response *out);
```

### depz_bno_metadata

```c
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
```

Sensor metadata FRS record (0xE301..0xE324), sh2 reference layout:
revision-gated fields read 0 when the record predates them.

### depz_bno_metadata_from_words

```c
void depz_bno_metadata_from_words(const uint32_t *words, size_t n, depz_bno_metadata *out);
```

### depz_bno_metadata_record

```c
uint16_t depz_bno_metadata_record(uint8_t sensor_id);
```

The metadata FRS record of a sensor id, 0 when none is known.

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

### DEPZ_BNO_RV_ACCURACY_Q

```c
#define DEPZ_BNO_RV_ACCURACY_Q     12
```

rotation-vector accuracy, rad

### DEPZ_BNO_GYRO_RV_ANGVEL_Q

```c
#define DEPZ_BNO_GYRO_RV_ANGVEL_Q  10
```

gyro-integrated RV angular velocity, rad/s

### depz_bno_q_point

```c
int depz_bno_q_point(uint8_t sensor_id);
```

Q point of a sensor's primary fields; -1 for event reports.

### depz_bno_report_xyz

```c
void depz_bno_report_xyz(const depz_bno_report *r, double out[3]);
```

x, y, z in m/s², rad/s or µT (raw counts for the raw sensors).

### depz_bno_report_bias

```c
void depz_bno_report_bias(const depz_bno_report *r, double out[3]);
```

Uncalibrated gyroscope / magnetometer bias, same units.

### depz_bno_report_quaternion

```c
void depz_bno_report_quaternion(const depz_bno_report *r, double out[4]);
```

Unit quaternion i, j, k, real (rotation vectors, gyro-integrated RV).

### depz_bno_report_accuracy_rad

```c
bool depz_bno_report_accuracy_rad(const depz_bno_report *r, double *out);
```

Heading accuracy estimate in radians; false for the game variants.

### depz_bno_report_angular_velocity

```c
void depz_bno_report_angular_velocity(const depz_bno_report *r, double out[3]);
```

Gyro-integrated RV angular velocity, rad/s.

### depz_bno_report_scalar

```c
double depz_bno_report_scalar(const depz_bno_report *r);
```

Environment reports 0x0A..0x0E: hPa, lux, %, cm, °C.

## BNO055 (IMU)

### DEPZ_BNO055_CMD_READ_REG

```c
#define DEPZ_BNO055_CMD_READ_REG     0x32
```

### DEPZ_BNO055_CMD_WRITE_REG

```c
#define DEPZ_BNO055_CMD_WRITE_REG    0x33
```

### DEPZ_BNO055_CMD_RESET

```c
#define DEPZ_BNO055_CMD_RESET        0x34
```

deferred reply, allow >= 1.5 s

### DEPZ_BNO055_CMD_START_STREAM

```c
#define DEPZ_BNO055_CMD_START_STREAM 0x35
```

### DEPZ_BNO055_CMD_STOP_STREAM

```c
#define DEPZ_BNO055_CMD_STOP_STREAM  0x36
```

### DEPZ_BNO055_CMD_GET_INFO

```c
#define DEPZ_BNO055_CMD_GET_INFO     0x37
```

### DEPZ_BNO055_RPT_REG_DATA

```c
#define DEPZ_BNO055_RPT_REG_DATA     0x91
```

### DEPZ_BNO055_RPT_INFO

```c
#define DEPZ_BNO055_RPT_INFO         0x92
```

### DEPZ_BNO055_RPT_STREAM

```c
#define DEPZ_BNO055_RPT_STREAM       0x93
```

### DEPZ_BNO055_XFER_MAX

```c
#define DEPZ_BNO055_XFER_MAX 128u
```

Max bytes per READ_REG / WRITE_REG / streamed block; addr + len <= 0x100.

### DEPZ_BNO055_INFO_SIZE

```c
#define DEPZ_BNO055_INFO_SIZE 38u
```

### DEPZ_BNO055_TRIGGER_TIMER

```c
#define DEPZ_BNO055_TRIGGER_TIMER 0u
```

BNO_START_STREAM trigger. Data-ready interrupts do not exist on sensor SW
03.11: TIMER is the only data trigger; INT is for motion interrupts, and
then period_ms is a missed-edge watchdog (0 disables it).

### DEPZ_BNO055_TRIGGER_INT

```c
#define DEPZ_BNO055_TRIGGER_INT   1u
```

### depz_bno055_pack_read_reg

```c
size_t depz_bno055_pack_read_reg(uint8_t addr, uint8_t len, uint8_t *out);
```

Encoders (return payload length written). RESET / STOP_STREAM / GET_INFO
have an empty payload.
/* 2 B */

### depz_bno055_pack_write_reg

```c
size_t depz_bno055_pack_write_reg(uint8_t addr, const uint8_t *data, size_t data_len, uint8_t *out);
```

BNO_WRITE_REG payload: addr u8 + data. Returns 1 + data_len, or 0 when
data_len is outside 1..DEPZ_BNO055_XFER_MAX.

### depz_bno055_pack_start_stream

```c
size_t depz_bno055_pack_start_stream(uint8_t trigger, uint8_t addr, uint8_t len, uint16_t period_ms, uint8_t *out);
```

BNO_START_STREAM payload: trigger u8, addr u8, len u8, period_ms u16.
/* 5 B */

### depz_bno055_reg_data

```c
typedef struct {
    uint8_t  cmd;
    uint64_t timestamp_us; /* MCU uptime at I2C-read completion */
    const uint8_t *data;   /* points into the report payload */
    size_t   data_len;
} depz_bno055_reg_data;
```

RPT_BNO_REG_DATA (0x91): echoed opcode, u64 timestamp, register bytes.

### depz_bno055_unpack_reg_data

```c
int depz_bno055_unpack_reg_data(const uint8_t *payload, size_t len, depz_bno055_reg_data *out);
```

/* needs >= 9 B */

### depz_bno055_info

```c
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
```

RPT_BNO_INFO (0x92, 38 B `<BBBBBHBBBIHHHIIIHBBH`): sensor identity
(registers 0x00..0x06) plus bridge diagnostics. Counters are free-running
and wrap; read_*_us, slots_skipped, loop_max_us reset at START_STREAM. A
rising sensor_resets means the sensor is back in CONFIG: re-configure.

### depz_bno055_unpack_info

```c
int depz_bno055_unpack_info(const uint8_t *payload, size_t len, depz_bno055_info *out);
```

Returns 0 on success, -1 when len < DEPZ_BNO055_INFO_SIZE.

### depz_bno055_stream

```c
typedef struct {
    uint64_t timestamp_us;
    uint8_t  addr;
    uint8_t  len;
    const uint8_t *data;   /* points into the report payload (len bytes) */
} depz_bno055_stream;
```

RPT_BNO_REG_STREAM (0x93): one streamed register block; addr/len echo the
stream configuration. timestamp_us is the trigger time, not I2C completion.

### depz_bno055_unpack_stream

```c
int depz_bno055_unpack_stream(const uint8_t *payload, size_t len, depz_bno055_stream *out);
```

Returns 0 on success, -1 when len < 10 + the echoed block length.

### DEPZ_BNO055_REG_CHIP_ID

```c
#define DEPZ_BNO055_REG_CHIP_ID          0x00u
```

---- register map essentials (§4.1; page 0 unless REG1) ------------------

### DEPZ_BNO055_REG_PAGE_ID

```c
#define DEPZ_BNO055_REG_PAGE_ID          0x07u
```

host-owned; always back to 0

### DEPZ_BNO055_REG_ACC_DATA

```c
#define DEPZ_BNO055_REG_ACC_DATA         0x08u
```

3 x i16 LE (x, y, z)

### DEPZ_BNO055_REG_MAG_DATA

```c
#define DEPZ_BNO055_REG_MAG_DATA         0x0Eu
```

### DEPZ_BNO055_REG_GYR_DATA

```c
#define DEPZ_BNO055_REG_GYR_DATA         0x14u
```

### DEPZ_BNO055_REG_EUL_DATA

```c
#define DEPZ_BNO055_REG_EUL_DATA         0x1Au
```

heading, roll, pitch

### DEPZ_BNO055_REG_QUA_DATA

```c
#define DEPZ_BNO055_REG_QUA_DATA         0x20u
```

4 x i16: w, x, y, z

### DEPZ_BNO055_REG_LIA_DATA

```c
#define DEPZ_BNO055_REG_LIA_DATA         0x28u
```

linear accel (no gravity)

### DEPZ_BNO055_REG_GRV_DATA

```c
#define DEPZ_BNO055_REG_GRV_DATA         0x2Eu
```

### DEPZ_BNO055_REG_TEMP

```c
#define DEPZ_BNO055_REG_TEMP             0x34u
```

i8

### DEPZ_BNO055_REG_CALIB_STAT

```c
#define DEPZ_BNO055_REG_CALIB_STAT       0x35u
```

### DEPZ_BNO055_REG_ST_RESULT

```c
#define DEPZ_BNO055_REG_ST_RESULT        0x36u
```

### DEPZ_BNO055_REG_INT_STA

```c
#define DEPZ_BNO055_REG_INT_STA          0x37u
```

clears on read

### DEPZ_BNO055_REG_SYS_CLK_STATUS

```c
#define DEPZ_BNO055_REG_SYS_CLK_STATUS   0x38u
```

### DEPZ_BNO055_REG_SYS_STATUS

```c
#define DEPZ_BNO055_REG_SYS_STATUS       0x39u
```

### DEPZ_BNO055_REG_SYS_ERR

```c
#define DEPZ_BNO055_REG_SYS_ERR          0x3Au
```

### DEPZ_BNO055_REG_UNIT_SEL

```c
#define DEPZ_BNO055_REG_UNIT_SEL         0x3Bu
```

### DEPZ_BNO055_REG_OPR_MODE

```c
#define DEPZ_BNO055_REG_OPR_MODE         0x3Du
```

bits 3:0

### DEPZ_BNO055_REG_PWR_MODE

```c
#define DEPZ_BNO055_REG_PWR_MODE         0x3Eu
```

### DEPZ_BNO055_REG_SYS_TRIGGER

```c
#define DEPZ_BNO055_REG_SYS_TRIGGER      0x3Fu
```

### DEPZ_BNO055_REG_TEMP_SOURCE

```c
#define DEPZ_BNO055_REG_TEMP_SOURCE      0x40u
```

### DEPZ_BNO055_REG_AXIS_MAP_CONFIG

```c
#define DEPZ_BNO055_REG_AXIS_MAP_CONFIG  0x41u
```

### DEPZ_BNO055_REG_AXIS_MAP_SIGN

```c
#define DEPZ_BNO055_REG_AXIS_MAP_SIGN    0x42u
```

### DEPZ_BNO055_REG_SIC_MATRIX

```c
#define DEPZ_BNO055_REG_SIC_MATRIX       0x43u
```

9 x i16, 1.0 = 16384

### DEPZ_BNO055_REG_CALIB_PROFILE

```c
#define DEPZ_BNO055_REG_CALIB_PROFILE    0x55u
```

22 B, CONFIG mode only

### DEPZ_BNO055_REG1_ACC_CONFIG

```c
#define DEPZ_BNO055_REG1_ACC_CONFIG      0x08u
```

### DEPZ_BNO055_REG1_MAG_CONFIG

```c
#define DEPZ_BNO055_REG1_MAG_CONFIG      0x09u
```

### DEPZ_BNO055_REG1_GYR_CONFIG_0

```c
#define DEPZ_BNO055_REG1_GYR_CONFIG_0    0x0Au
```

### DEPZ_BNO055_REG1_GYR_CONFIG_1

```c
#define DEPZ_BNO055_REG1_GYR_CONFIG_1    0x0Bu
```

### DEPZ_BNO055_REG1_INT_MSK

```c
#define DEPZ_BNO055_REG1_INT_MSK         0x0Fu
```

### DEPZ_BNO055_REG1_INT_EN

```c
#define DEPZ_BNO055_REG1_INT_EN          0x10u
```

### DEPZ_BNO055_REG1_UNIQUE_ID

```c
#define DEPZ_BNO055_REG1_UNIQUE_ID       0x50u
```

16 B

### DEPZ_BNO055_FULL_BLOCK_ADDR

```c
#define DEPZ_BNO055_FULL_BLOCK_ADDR 0x08u
```

Every output channel in one read (0x08..0x35), and the quaternion alone.

### DEPZ_BNO055_FULL_BLOCK_LEN

```c
#define DEPZ_BNO055_FULL_BLOCK_LEN  46u
```

### DEPZ_BNO055_QUAT_BLOCK_ADDR

```c
#define DEPZ_BNO055_QUAT_BLOCK_ADDR 0x20u
```

### DEPZ_BNO055_QUAT_BLOCK_LEN

```c
#define DEPZ_BNO055_QUAT_BLOCK_LEN  8u
```

### depz_bno055_opr_mode

```c
typedef enum {
    DEPZ_BNO055_MODE_CONFIG = 0x00,
    DEPZ_BNO055_MODE_ACCONLY, DEPZ_BNO055_MODE_MAGONLY, DEPZ_BNO055_MODE_GYROONLY,
    DEPZ_BNO055_MODE_ACCMAG, DEPZ_BNO055_MODE_ACCGYRO, DEPZ_BNO055_MODE_MAGGYRO,
    DEPZ_BNO055_MODE_AMG,
    DEPZ_BNO055_MODE_IMU = 0x08,
    DEPZ_BNO055_MODE_COMPASS, DEPZ_BNO055_MODE_M4G, DEPZ_BNO055_MODE_NDOF_FMC_OFF,
    DEPZ_BNO055_MODE_NDOF = 0x0C
} depz_bno055_opr_mode;
```

OPR_MODE values; >= IMU are fusion modes.

### DEPZ_BNO055_UNIT_ACC_MG

```c
#define DEPZ_BNO055_UNIT_ACC_MG      0x01u
```

---- units (UNIT_SEL 0x3B, §4.2 — as the silicon implements them) --------
ACC_DATA in mg, else m/s^2

### DEPZ_BNO055_UNIT_GYR_RPS

```c
#define DEPZ_BNO055_UNIT_GYR_RPS     0x02u
```

rad/s, else dps

### DEPZ_BNO055_UNIT_EUL_RAD

```c
#define DEPZ_BNO055_UNIT_EUL_RAD     0x04u
```

radians, else degrees

### DEPZ_BNO055_UNIT_TEMP_F

```c
#define DEPZ_BNO055_UNIT_TEMP_F      0x10u
```

deg F (1 LSB = 2 F), else C

### DEPZ_BNO055_UNIT_ORI_ANDROID

```c
#define DEPZ_BNO055_UNIT_ORI_ANDROID 0x80u
```

power-on value is 0x80

### depz_bno055_units

```c
typedef struct {
    bool accel_mg;
    bool gyro_rps;
    bool euler_rad;
    bool temp_f;
    bool android;
} depz_bno055_units;
```

### depz_bno055_unpack_units

```c
void depz_bno055_unpack_units(uint8_t unit_sel, depz_bno055_units *out);
```

Other UNIT_SEL bits are ignored (they do nothing on the sensor).

### depz_bno055_pack_units

```c
uint8_t depz_bno055_pack_units(const depz_bno055_units *u);
```

### depz_bno055_accel_lsb

```c
double depz_bno055_accel_lsb(const depz_bno055_units *u);
```

LSB per unit: value = raw / lsb. accel 100 (m/s^2) or 1 (mg); gyro 16
(dps) or 900 (rps); euler 16 (deg) or 900 (rad); temp 1 (C) or 0.5 (F).

### depz_bno055_gyro_lsb

```c
double depz_bno055_gyro_lsb(const depz_bno055_units *u);
```

### depz_bno055_euler_lsb

```c
double depz_bno055_euler_lsb(const depz_bno055_units *u);
```

### depz_bno055_temp_lsb

```c
double depz_bno055_temp_lsb(const depz_bno055_units *u);
```

### DEPZ_BNO055_MAG_LSB

```c
#define DEPZ_BNO055_MAG_LSB          16.0
```

uT, fixed

### DEPZ_BNO055_QUAT_LSB

```c
#define DEPZ_BNO055_QUAT_LSB         16384.0
```

2^14, unit-less

### DEPZ_BNO055_FUSION_ACCEL_LSB

```c
#define DEPZ_BNO055_FUSION_ACCEL_LSB 100.0
```

LIA and GRV ignore the ACC_Unit bit: always m/s^2 at 100 LSB (measured).

### depz_bno055_calib_status

```c
typedef struct {
    uint8_t system;
    uint8_t gyro;
    uint8_t accel;
    uint8_t mag;
} depz_bno055_calib_status;
```

CALIB_STAT (0x35): sys<7:6> gyr<5:4> acc<3:2> mag<1:0>, 0..3 each.

### depz_bno055_unpack_calib_status

```c
void depz_bno055_unpack_calib_status(uint8_t value, depz_bno055_calib_status *out);
```

### depz_bno055_pack_calib_status

```c
uint8_t depz_bno055_pack_calib_status(const depz_bno055_calib_status *s);
```

### depz_bno055_fully_calibrated

```c
bool depz_bno055_fully_calibrated(const depz_bno055_calib_status *s);
```

3/3/3/3.

### DEPZ_BNO055_CALIB_PROFILE_LEN

```c
#define DEPZ_BNO055_CALIB_PROFILE_LEN 22u
```

### depz_bno055_calib_profile

```c
typedef struct {
    int16_t accel_offset[3];
    int16_t mag_offset[3];
    int16_t gyro_offset[3];
    int16_t accel_radius;
    int16_t mag_radius;
} depz_bno055_calib_profile;
```

Offsets and radii at 0x55..0x6A, 11 x i16 LE, sensor LSB. Read/write only
in CONFIG, all 22 bytes in one transfer.

### depz_bno055_unpack_calib_profile

```c
int depz_bno055_unpack_calib_profile(const uint8_t *data, size_t len, depz_bno055_calib_profile *out);
```

Returns 0 on success, -1 when len != DEPZ_BNO055_CALIB_PROFILE_LEN.

### depz_bno055_pack_calib_profile

```c
size_t depz_bno055_pack_calib_profile(const depz_bno055_calib_profile *p, uint8_t *out);
```

/* 22 B */

### DEPZ_BNO055_AXIS_X

```c
#define DEPZ_BNO055_AXIS_X 0u
```

---- axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42, §4.4) --------

### DEPZ_BNO055_AXIS_Y

```c
#define DEPZ_BNO055_AXIS_Y 1u
```

### DEPZ_BNO055_AXIS_Z

```c
#define DEPZ_BNO055_AXIS_Z 2u
```

### depz_bno055_axis_remap

```c
typedef struct {
    uint8_t x, y, z;
    bool x_negative, y_negative, z_negative;
} depz_bno055_axis_remap;
```

Which chip axis feeds each output axis (x = AXIS_Y: output X is chip Y).

### depz_bno055_unpack_axis_remap

```c
void depz_bno055_unpack_axis_remap(uint8_t config, uint8_t sign, depz_bno055_axis_remap *out);
```

config = z<5:4> y<3:2> x<1:0>; sign = x 2, y 1, z 0 (1 = negative).

### depz_bno055_pack_axis_remap

```c
int depz_bno055_pack_axis_remap(const depz_bno055_axis_remap *a, uint8_t *config, uint8_t *sign);
```

Returns 0 and writes both bytes, or -1 (nothing written) when x/y/z is not
a permutation of 0/1/2 — the sensor would silently keep the old value.

### DEPZ_BNO055_PLACEMENTS

```c
extern const uint8_t DEPZ_BNO055_PLACEMENTS[8][2];
```

Datasheet §3.4 placements P0..P7 as {AXIS_MAP_CONFIG, AXIS_MAP_SIGN};
P1 (0x24/0x00) is the power-on default.

### depz_bno055_placement

```c
int depz_bno055_placement(const char *name, depz_bno055_axis_remap *out);
```

"P0".."P7" (case-insensitive). Returns 0, or -1 for any other name/NULL.

### depz_bno055_accel_config

```c
typedef struct { uint8_t range, bandwidth, power; } depz_bno055_accel_config;
```

ACC_CONFIG (p1 0x08): range<1:0> (2/4/8/16 g), bandwidth<4:2>
(7.81..1000 Hz), power<7:5>. Power-on 0x0D = 4 g, 62.5 Hz, normal.

### depz_bno055_unpack_accel_config

```c
void depz_bno055_unpack_accel_config(uint8_t value, depz_bno055_accel_config *out);
```

### depz_bno055_pack_accel_config

```c
uint8_t depz_bno055_pack_accel_config(const depz_bno055_accel_config *c);
```

### depz_bno055_gyro_config

```c
typedef struct { uint8_t range, bandwidth, power; } depz_bno055_gyro_config;
```

GYR_CONFIG_0/1 (p1 0x0A/0x0B): byte 0 range<2:0> (2000..125 dps),
bandwidth<5:3>; byte 1 power<2:0>. Power-on 0x38/0x00.

### depz_bno055_unpack_gyro_config

```c
void depz_bno055_unpack_gyro_config(const uint8_t bytes[2], depz_bno055_gyro_config *out);
```

### depz_bno055_pack_gyro_config

```c
size_t depz_bno055_pack_gyro_config(const depz_bno055_gyro_config *c, uint8_t out[2]);
```

/* 2 B */

### depz_bno055_mag_config

```c
typedef struct { uint8_t rate, mode, power; } depz_bno055_mag_config;
```

MAG_CONFIG (p1 0x09): rate<2:0> (2..30 Hz), mode<4:3>, power<6:5>; bit 7
is not a field (a repack drops it). Power-on 0x0B = 10 Hz, regular, normal.

### depz_bno055_unpack_mag_config

```c
void depz_bno055_unpack_mag_config(uint8_t value, depz_bno055_mag_config *out);
```

### depz_bno055_pack_mag_config

```c
uint8_t depz_bno055_pack_mag_config(const depz_bno055_mag_config *c);
```

### depz_bno055_block

```c
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
```

Raw register values found in one block read. A channel is present
(has_* true) only when the window addr..addr+len covers all its bytes.

### depz_bno055_decode_block

```c
void depz_bno055_decode_block(uint8_t addr, const uint8_t *data, size_t len, depz_bno055_block *out);
```

Unpack whatever channels the window starting at `addr` holds (data may be
NULL when len == 0). Absent channels are zeroed.

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

Parse & validate (magic -> header CRC-16/CCITT-FALSE -> fw_size==payload).

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

## Live layer: errors

### depz_err

```c
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
```

### depz_err_str

```c
const char *depz_err_str(int err);
```

Short constant name of an error code ("DEPZ_E_TIMEOUT").

### depz_last_error

```c
const char *depz_last_error(void);
```

Human-readable detail of the last failure on the calling thread.

### depz_last_status

```c
int depz_last_status(void);
```

depz_status of the last DEPZ_E_STATUS / DEPZ_E_BUSY on the calling thread
(and the opcode it answered), -1 when the last failure was something else.

### depz_last_status_cmd

```c
int depz_last_status_cmd(void);
```

## Live layer: byte links

### depz_link

```c
typedef struct depz_link depz_link;
```

### depz_link_vtable

```c
typedef struct {
    int  (*read)(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got);
    int  (*write)(void *self, const uint8_t *data, size_t len);
    void (*close)(void *self);
    void (*destroy)(void *self);
} depz_link_vtable;
```

Custom links: `read` returns DEPZ_OK with *got = 0 on timeout, or
DEPZ_E_CLOSED once closed; `close` must be idempotent, callable from any
thread, and wake a blocked `read`; `destroy` frees `self`.

### depz_link_new

```c
int depz_link_new(const depz_link_vtable *vt, void *self, const char *name, depz_link **out);
```

### depz_link_open_serial

```c
int depz_link_open_serial(const char *port, depz_link **out);
```

The CDC-ACM serial port `port` ("/dev/ttyACM0", "/dev/cu.usbmodem1101",
"COM7"), opened exclusively (a second opener fails instead of silently
sharing the byte stream). On POSIX the tty is left in a sane cooked state
on close, so the next program (e.g. a browser's Web Serial) can use it.

### depz_link_open_replay

```c
int depz_link_open_replay(const char *path, bool strict_tx, bool realtime, depz_link **out);
```

Replay the rx side of a .depzrec capture, causally: an rx chunk is served
only once the host has written as many bytes as preceded it. `strict_tx`
fails any write that differs from the recorded tx stream
(DEPZ_E_REPLAY_MISMATCH); `realtime` paces rx by the recorded times.

### depz_link_open_recording

```c
int depz_link_open_recording(depz_link *inner, const char *path, const char *header_extra_json, depz_link **out);
```

Tee `inner` into a new .depzrec file at `path` (tx journaled before the
physical write). `header_extra_json` is either NULL or the inside of a JSON
object (`"port":"live","note":"x"`) merged into the header line. Takes
ownership of `inner`, also on failure.

### depz_link_loopback_pair

```c
int depz_link_loopback_pair(depz_link **a, depz_link **b);
```

Two in-memory links: what one writes, the other reads (tests, fakes).

### depz_link_read

```c
int depz_link_read(depz_link *l, uint8_t *buf, size_t cap, int timeout_ms, size_t *got);
```

### depz_link_write

```c
int depz_link_write(depz_link *l, const uint8_t *data, size_t len);
```

### depz_link_close

```c
void depz_link_close(depz_link *l);
```

### depz_link_closed

```c
bool depz_link_closed(const depz_link *l);
```

### depz_link_name

```c
const char *depz_link_name(const depz_link *l);
```

### depz_link_free

```c
void depz_link_free(depz_link *l);
```

Close (if needed) and free. Not for a link a device owns.

### depz_link_replay_exhausted

```c
bool depz_link_replay_exhausted(const depz_link *l);
```

Replay only: true once every recorded rx chunk has been served.

## Live layer: streams

### depz_stream

```c
typedef struct depz_stream depz_stream;
```

A pull subscription: the reader thread pushes, the owner pulls. When full,
the oldest item goes and dropped_count() grows. Registered at creation, so
nothing produced after the call is missed.

### depz_stream_next

```c
int depz_stream_next(depz_stream *s, void *item, int timeout_ms);
```

Next item into `item` (its size is fixed by the stream's type). DEPZ_OK,
DEPZ_E_TIMEOUT, or DEPZ_E_CLOSED once the device is closed and drained.

### depz_stream_dropped_count

```c
uint64_t depz_stream_dropped_count(const depz_stream *s);
```

### depz_stream_close

```c
void depz_stream_close(depz_stream *s);
```

Unsubscribe and free. Call before closing its device, or after — both fine.

## Live layer: device core

### depz_device

```c
typedef struct depz_device depz_device;
```

### DEPZ_DEFAULT_TIMEOUT_MS

```c
#define DEPZ_DEFAULT_TIMEOUT_MS 200
```

### depz_link_stats

```c
typedef struct {
    uint64_t tx_packets, rx_packets, tx_bytes, rx_bytes;
    uint64_t crc_errors, header_errors, trash_bytes;
    uint64_t seq_gaps;          /* gaps in device->host seq (host-observed) */
    uint64_t device_seq_errors; /* RPT_SEQUENCE_ERROR count (device-observed) */
} depz_link_stats;
```

### depz_device_open

```c
int depz_device_open(const char *port, depz_device **out);
```

A plain device on a serial port / on a link (takes ownership of `link`,
also on failure). No identity probe: the sensor type stays
DEPZ_SENSOR_NONE until depz_device_promote().

### depz_device_open_link

```c
int depz_device_open_link(depz_link *link, depz_device **out);
```

### depz_device_promote

```c
int depz_device_promote(depz_device *dev);
```

Ask GET_NAME_ACTIVE_SOFTWARE and attach the matching sensor class, as
depz_open_device() does after its probe. DEPZ_E_BOOTLOADER in bootloader
mode.

### depz_device_close

```c
void depz_device_close(depz_device *dev);
```

Stop the reader, close the link, free everything. NULL is a no-op.

### depz_device_set_timeout_ms

```c
void depz_device_set_timeout_ms(depz_device *dev, int timeout_ms);
```

### depz_device_sensor_type

```c
depz_sensor_type depz_device_sensor_type(const depz_device *dev);
```

### depz_device_port

```c
const char *depz_device_port(const depz_device *dev);
```

### depz_device_closed

```c
bool depz_device_closed(const depz_device *dev);
```

### depz_device_stats

```c
void depz_device_stats(const depz_device *dev, depz_link_stats *out);
```

### depz_matcher

```c
typedef bool (*depz_matcher)(uint8_t cmd, const uint8_t *payload, size_t len, void *ctx);
```

A matcher sees every non-status packet while its request is in flight (on
the reader thread, under the device lock — copy what you need into `ctx`,
do nothing else). Return true to claim the packet and complete.

### depz_device_request

```c
int depz_device_request(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len, depz_matcher matcher, void *ctx, bool ok_completes, int timeout_ms);
```

Send `cmd` and wait for its completion: `matcher` claiming a packet, or,
with `ok_completes`, RPT_STATUS(cmd, OK). A non-OK RPT_STATUS echoing
`cmd` fails with DEPZ_E_STATUS (DEPZ_E_BUSY for ERR_BUSY). One request per
opcode in flight.

### depz_device_send

```c
int depz_device_send(depz_device *dev, uint8_t cmd, const uint8_t *payload, size_t len);
```

Fire-and-forget (escape hatch).

### depz_device_get_device_name

```c
int depz_device_get_device_name(depz_device *dev, char *out, size_t cap);
```

### depz_device_get_software_name

```c
int depz_device_get_software_name(depz_device *dev, char *out, size_t cap);
```

### depz_device_get_serial_number

```c
int depz_device_get_serial_number(depz_device *dev, char *out, size_t cap);
```

### depz_device_read_mcu_temperature

```c
int depz_device_read_mcu_temperature(depz_device *dev, double *celsius);
```

Last cached MCU temperature, °C (the device refreshes it ~2 Hz).

### depz_device_get_payload_crc_type

```c
int depz_device_get_payload_crc_type(depz_device *dev, depz_crc_type *out);
```

### depz_device_set_payload_crc_type

```c
int depz_device_set_payload_crc_type(depz_device *dev, depz_crc_type t);
```

Device->host payload CRC; host->device CRC is per packet.

### depz_device_get_sync_pin

```c
int depz_device_get_sync_pin(depz_device *dev, uint8_t pin, depz_sync_pin_config *out);
```

### depz_device_set_sync_pin

```c
int depz_device_set_sync_pin(depz_device *dev, const depz_sync_pin_config *c);
```

### depz_device_reset

```c
int depz_device_reset(depz_device *dev);
```

DEVICE_RESET: the device acknowledges, then reboots; the link drops.

### depz_device_enter_bootloader

```c
int depz_device_enter_bootloader(depz_device *dev);
```

Reboot into the bootloader; the device is then closed for I/O (still call
depz_device_close() to free it).

### depz_time_sync

```c
typedef struct {
    int64_t  offset_us;         /* device clock - host clock */
    int64_t  rtt_us;
    uint64_t synced_at_host_us;
} depz_time_sync;
```

### depz_host_now_us

```c
uint64_t depz_host_now_us(void);
```

Host monotonic clock, µs — the host side of all time-sync math.

### depz_device_sync_time

```c
int depz_device_sync_time(depz_device *dev, int samples, depz_time_sync *out);
```

NTP-style, `samples` round trips, keeps the lowest-RTT one.

### depz_device_time_sync

```c
bool depz_device_time_sync(const depz_device *dev, depz_time_sync *out);
```

### depz_device_to_host_time_us

```c
int depz_device_to_host_time_us(const depz_device *dev, uint64_t device_us, int64_t *out);
```

Device µs -> host monotonic µs; DEPZ_E_ARG before the first sync.

### depz_device_event_type

```c
typedef enum {
    DEPZ_DEV_EV_SEQUENCE_ERROR,      /* expected_seq, received_seq, by_device */
    DEPZ_DEV_EV_CRC_ERROR,           /* cmd, seq */
    DEPZ_DEV_EV_TRASH,               /* data / data_len (first bytes), trash_len */
    DEPZ_DEV_EV_UNSOLICITED_STATUS,  /* status (ERR_HARDWARE_FAULT: a fault) */
    DEPZ_DEV_EV_TEXT,                /* cmd, text (unrouted reports: hex text) */
    DEPZ_DEV_EV_TEMPERATURE,         /* timestamp_us, celsius */
    DEPZ_DEV_EV_DISCONNECTED         /* text = reason */
} depz_device_event_type;
```

### depz_device_event

```c
typedef struct {
    depz_device_event_type type;
    uint8_t  cmd, seq;
    uint8_t  expected_seq, received_seq;
    bool     by_device;
    uint8_t  status;
    uint64_t timestamp_us;
    double   celsius;
    size_t   trash_len;
    uint8_t  data[64];
    size_t   data_len;
    char     text[256];
} depz_device_event;
```

### depz_device_event_cb

```c
typedef void (*depz_device_event_cb)(const depz_device_event *ev, void *user);
```

### depz_device_on_event

```c
int depz_device_on_event(depz_device *dev, depz_device_event_cb cb, void *user, int *token);
```

Subscribe; *token (optional) identifies the subscription for _off.

### depz_device_off_event

```c
void depz_device_off_event(depz_device *dev, int token);
```

### depz_device_events

```c
depz_stream *depz_device_events(depz_device *dev, size_t maxsize);
```

Pull stream of depz_device_event items. NULL on allocation failure.

## Live layer: discovery

### depz_port_info

```c
typedef struct {
    char port[256];
    int  vid, pid;        /* -1 when the OS does not know (not a USB port) */
    char usb_serial[128]; /* USB iSerial, "" when unknown */
} depz_port_info;
```

### depz_list_serial_ports

```c
int depz_list_serial_ports(depz_port_info **out, size_t *count);
```

Every serial port the OS lists, with its USB identity where known.

### depz_free_port_list

```c
void depz_free_port_list(depz_port_info *list);
```

### depz_device_info

```c
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
```

### depz_probe_port

```c
int depz_probe_port(const char *port, int timeout_ms, depz_device_info *out);
```

Open `port`, ask the software name (+ device name, serial), close.
DEPZ_E_NO_DEVICE when nothing DEPZ-shaped answers.

### depz_list_depz_devices

```c
int depz_list_depz_devices(bool match_usb, int timeout_ms, depz_device_info **out, size_t *count);
```

Probe the candidate ports — with `match_usb`, only those whose USB id is a
known DEPZ id — ordered by USB serial (unknown last).

### depz_free_device_list

```c
void depz_free_device_list(depz_device_info *list);
```

### depz_open_options

```c
typedef struct {
    const char *port;   /* open exactly this port (warns via depz_last_error()
                           text only; never refuses an unknown USB id) */
    const char *serial; /* else: the candidate with this USB serial */
    int index;          /* else: the Nth candidate by USB serial; -1 = first */
    int timeout_ms;     /* request timeout, <= 0 = default */
} depz_open_options;
```

### DEPZ_OPEN_OPTIONS_INIT

```c
#define DEPZ_OPEN_OPTIONS_INIT { NULL, NULL, -1, 0 }
```

### depz_open_device

```c
int depz_open_device(const depz_open_options *opt, depz_device **out);
```

Find, probe and open a DEPZ device with its sensor class attached.
`opt` NULL = the candidate with the smallest USB serial.

## SR04 sensor class (live layer)

### depz_sr04_measurement

```c
typedef struct {
    uint64_t timestamp_us; /* device µs */
    uint16_t echo_time_us; /* DEPZ_SR04_ECHO_TIMEOUT = no echo */
    bool     from_loop;    /* false: MEASURE_ONCE or a SYNC_IN edge */
} depz_sr04_measurement;
```

### depz_sr04_measurement_cb

```c
typedef void (*depz_sr04_measurement_cb)(const depz_sr04_measurement *m, void *user);
```

### depz_sr04_open_link

```c
int depz_sr04_open_link(depz_link *link, depz_device **out);
```

An SR04 on a link without an identity probe (tests, replay).

### depz_is_sr04

```c
bool depz_is_sr04(const depz_device *dev);
```

### depz_sr04_valid

```c
bool depz_sr04_valid(const depz_sr04_measurement *m);
```

### depz_sr04_measurement_distance_mm

```c
bool depz_sr04_measurement_distance_mm(const depz_sr04_measurement *m, double *mm);
```

Distance at 343 m/s; false when there was no echo.

### depz_sr04_measurement_distance_mm_at

```c
bool depz_sr04_measurement_distance_mm_at(const depz_sr04_measurement *m, double air_temp_c, double *mm);
```

Temperature-compensated speed of sound (331.3 + 0.606·T m/s).

### depz_sr04_get_sample_period_us

```c
int depz_sr04_get_sample_period_us(depz_device *dev, uint32_t *out);
```

Stored minimum interval between measurement starts (default 50000). The
effective rate is also limited by the echo window (contract 03 §3).

### depz_sr04_set_sample_period_us

```c
int depz_sr04_set_sample_period_us(depz_device *dev, uint32_t period_us);
```

### depz_sr04_get_echo_decay_us

```c
int depz_sr04_get_echo_decay_us(depz_device *dev, uint16_t *out);
```

### depz_sr04_set_echo_decay_us

```c
int depz_sr04_set_echo_decay_us(depz_device *dev, uint32_t decay_us, uint16_t *effective);
```

The device clamps to 4000..65000 µs; *effective (optional) is re-read.
DEPZ_E_ARG above 65535 (the u16 wire field).

### depz_sr04_measure_once

```c
int depz_sr04_measure_once(depz_device *dev, int timeout_ms, depz_sr04_measurement *out);
```

Single shot. DEPZ_E_BUSY while the loop runs. The reply comes when the
echo completes (or times out at ~65.5 ms): < 0 timeout means 1000 ms.

### depz_sr04_start

```c
int depz_sr04_start(depz_device *dev);
```

/* measurement loop; idempotent */

### depz_sr04_stop

```c
int depz_sr04_stop(depz_device *dev);
```

### depz_sr04_on_measurement

```c
int depz_sr04_on_measurement(depz_device *dev, depz_sr04_measurement_cb cb, void *user, int *token);
```

Loop samples and SYNC_IN single shots, as callbacks and/or streams.

### depz_sr04_off_measurement

```c
void depz_sr04_off_measurement(depz_device *dev, int token);
```

### depz_sr04_stream

```c
depz_stream *depz_sr04_stream(depz_device *dev, size_t maxsize);
```

Pull stream of depz_sr04_measurement items. NULL on wrong type / no memory.

## VL53L4CD sensor class (live layer)

### depz_vl53l4cd_measurement

```c
typedef struct {
    uint64_t timestamp_us;     /* MCU µs at the INT edge (stream) / the read (poll) */
    depz_vl53l4_result r;      /* range_status 0 = valid, distance_mm, ... */
} depz_vl53l4cd_measurement;
```

### depz_vl53l4cd_measurement_cb

```c
typedef void (*depz_vl53l4cd_measurement_cb)(const depz_vl53l4cd_measurement *m, void *user);
```

### DEPZ_VL53L4CD_WINDOW_BELOW

```c
#define DEPZ_VL53L4CD_WINDOW_BELOW 0u
```

Detection-threshold windows (SYSTEM__INTERRUPT).

### DEPZ_VL53L4CD_WINDOW_ABOVE

```c
#define DEPZ_VL53L4CD_WINDOW_ABOVE 1u
```

### DEPZ_VL53L4CD_WINDOW_OUT

```c
#define DEPZ_VL53L4CD_WINDOW_OUT   2u
```

### DEPZ_VL53L4CD_WINDOW_IN

```c
#define DEPZ_VL53L4CD_WINDOW_IN    3u
```

### DEPZ_VL53L4CD_I2C_KHZ_DEFAULT

```c
#define DEPZ_VL53L4CD_I2C_KHZ_DEFAULT 1000u
```

init() runs its configuration block at 400 kHz and leaves the bus here.

### depz_vl53l4cd_open_link

```c
int depz_vl53l4cd_open_link(depz_link *link, depz_device **out);
```

A VL53L4CD on a link without an identity probe (tests, replay).

### depz_is_vl53l4cd

```c
bool depz_is_vl53l4cd(const depz_device *dev);
```

### depz_vl53l4cd_status_text

```c
const char *depz_vl53l4cd_status_text(int range_status);
```

UM2931 name of a range status ("valid", "sigma above threshold", ...).

### depz_vl53l4cd_is_alive

```c
int depz_vl53l4cd_is_alive(depz_device *dev, bool *alive);
```

The sensor answers with its model id (0xEBAA).

### depz_vl53l4cd_init

```c
int depz_vl53l4cd_init(depz_device *dev, uint16_t bus_khz);
```

Default configuration block + VHV calibration (ULD sensor_init), then the
bus at `bus_khz` (0 = DEPZ_VL53L4CD_I2C_KHZ_DEFAULT). Well under a second.

### depz_vl53l4cd_initialized

```c
bool depz_vl53l4cd_initialized(const depz_device *dev);
```

### depz_vl53l4cd_ranging

```c
bool depz_vl53l4cd_ranging(const depz_device *dev);
```

### depz_vl53l4cd_xshut

```c
int depz_vl53l4cd_xshut(depz_device *dev, uint8_t action);
```

XSHUT pin: DEPZ_VL53L4_XSHUT_OFF / _ON / _RESET. OFF and RESET stop any
stream; a power-cycled sensor needs init() again.

### depz_vl53l4cd_reset_sensor

```c
int depz_vl53l4cd_reset_sensor(depz_device *dev);
```

/* = xshut(RESET) */

### depz_vl53l4cd_bridge_info

```c
int depz_vl53l4cd_bridge_info(depz_device *dev, depz_vl53l4_info *out);
```

Bridge identity, pin levels and counters; safe while streaming.

### depz_vl53l4cd_set_i2c_speed_khz

```c
int depz_vl53l4cd_set_i2c_speed_khz(depz_device *dev, uint16_t khz);
```

Re-time the bus to the nominal step nearest `khz`. Not while ranging.

### depz_vl53l4cd_set_range_timing

```c
int depz_vl53l4cd_set_range_timing(depz_device *dev, uint32_t budget_ms, uint32_t inter_ms);
```

Configuration — init() first, never while ranging.
Budget 10..200 ms; inter 0 = continuous, > budget = autonomous low power.

### depz_vl53l4cd_get_range_timing

```c
int depz_vl53l4cd_get_range_timing(depz_device *dev, uint32_t *budget_ms, uint32_t *inter_ms);
```

### depz_vl53l4cd_set_offset_mm

```c
int depz_vl53l4cd_set_offset_mm(depz_device *dev, int32_t offset_mm);
```

### depz_vl53l4cd_get_offset_mm

```c
int depz_vl53l4cd_get_offset_mm(depz_device *dev, int32_t *offset_mm);
```

### depz_vl53l4cd_set_xtalk_kcps

```c
int depz_vl53l4cd_set_xtalk_kcps(depz_device *dev, uint16_t xtalk_kcps);
```

/* 0 = off */

### depz_vl53l4cd_get_xtalk_kcps

```c
int depz_vl53l4cd_get_xtalk_kcps(depz_device *dev, uint16_t *xtalk_kcps);
```

### depz_vl53l4cd_set_detection_thresholds

```c
int depz_vl53l4cd_set_detection_thresholds(depz_device *dev, uint16_t low_mm, uint16_t high_mm, uint8_t window);
```

INT fires only when the window condition holds (DEPZ_VL53L4CD_WINDOW_*).

### depz_vl53l4cd_get_detection_thresholds

```c
int depz_vl53l4cd_get_detection_thresholds(depz_device *dev, uint16_t *low_mm, uint16_t *high_mm, uint8_t *window);
```

### depz_vl53l4cd_set_signal_threshold_kcps

```c
int depz_vl53l4cd_set_signal_threshold_kcps(depz_device *dev, uint16_t kcps);
```

### depz_vl53l4cd_get_signal_threshold_kcps

```c
int depz_vl53l4cd_get_signal_threshold_kcps(depz_device *dev, uint16_t *kcps);
```

### depz_vl53l4cd_set_sigma_threshold_mm

```c
int depz_vl53l4cd_set_sigma_threshold_mm(depz_device *dev, uint16_t mm);
```

/* <= 16383 */

### depz_vl53l4cd_get_sigma_threshold_mm

```c
int depz_vl53l4cd_get_sigma_threshold_mm(depz_device *dev, uint16_t *mm);
```

### depz_vl53l4cd_start_temperature_update

```c
int depz_vl53l4cd_start_temperature_update(depz_device *dev);
```

Re-run VHV calibration; after a > 8 °C ambient change.

### depz_vl53l4cd_calibrate_offset

```c
int depz_vl53l4cd_calibrate_offset(depz_device *dev, uint16_t target_mm, uint8_t nb_samples, int32_t *offset_mm);
```

Against a target at `target_mm` (10..1000); nb_samples 5..255 (0 = 20).
Blocks for the bursts; *offset_mm (optional) = the offset now programmed.

### depz_vl53l4cd_calibrate_xtalk

```c
int depz_vl53l4cd_calibrate_xtalk(depz_device *dev, uint16_t target_mm, uint8_t nb_samples, uint16_t *xtalk_kcps);
```

Against a target at `target_mm` (10..5000); *xtalk_kcps = programmed value.

### depz_vl53l4cd_start_ranging

```c
int depz_vl53l4cd_start_ranging(depz_device *dev);
```

Ranging.
Start the sensor and arm the MCU stream: one measurement per INT edge.

### depz_vl53l4cd_stop_ranging

```c
int depz_vl53l4cd_stop_ranging(depz_device *dev);
```

Idempotent; the sensor is stopped even when the stream stop fails.

### depz_vl53l4cd_measure_once

```c
int depz_vl53l4cd_measure_once(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out);
```

Poll mode: start, wait data-ready, read, stop (< 0 timeout = 1000 ms).
Refused while the stream runs.

### depz_vl53l4cd_on_measurement

```c
int depz_vl53l4cd_on_measurement(depz_device *dev, depz_vl53l4cd_measurement_cb cb, void *user, int *token);
```

### depz_vl53l4cd_off_measurement

```c
void depz_vl53l4cd_off_measurement(depz_device *dev, int token);
```

### depz_vl53l4cd_stream

```c
depz_stream *depz_vl53l4cd_stream(depz_device *dev, size_t maxsize);
```

Pull stream of depz_vl53l4cd_measurement items.

### depz_vl53l4cd_get_measurement

```c
int depz_vl53l4cd_get_measurement(depz_device *dev, int timeout_ms, depz_vl53l4cd_measurement *out);
```

The next streamed measurement (< 0 timeout = 2000 ms).

### depz_vl53l4cd_stream_parse_errors

```c
uint64_t depz_vl53l4cd_stream_parse_errors(const depz_device *dev);
```

Stream reports dropped because their block did not decode.

### depz_vl53l4cd_read_reg

```c
int depz_vl53l4cd_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
```

Raw register access (escape hatch): 16-bit address, contents as the
sensor has them (big-endian words). Split at the bridge's 253-byte limit.

### depz_vl53l4cd_write_reg

```c
int depz_vl53l4cd_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);
```

## VL53L8 multizone sensor class (live layer)

### depz_vl53l8_model

```c
typedef enum {
    DEPZ_VL53L8_MODEL_L8CX = 0, /* VL53L8CX ULD 2.1.0 blob, SPI bridge        */
    DEPZ_VL53L8_MODEL_L8CH = 1, /* VL53LMZ ULD 2.0.16 blob, adds CNH output   */
    DEPZ_VL53L8_MODEL_L5CX = 2, /* I2C bridge (APP_VL53L7), ULD 2.0.1 blob     */
    DEPZ_VL53L8_MODEL_L7CX = 3, /* same blob and API as L5CX, 90° optics       */
    DEPZ_VL53L8_MODEL_L7CH = 4  /* VL53LMZ 2.0.16 blob (as L8CH), adds CNH     */
} depz_vl53l8_model;
```

The board model: fixes the sensor-firmware blob and the bridge.

### DEPZ_VL53L8_RANGING_MODE_CONTINUOUS

```c
#define DEPZ_VL53L8_RANGING_MODE_CONTINUOUS 1u
```

### DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS

```c
#define DEPZ_VL53L8_RANGING_MODE_AUTONOMOUS 3u
```

### DEPZ_VL53L8_TARGET_ORDER_CLOSEST

```c
#define DEPZ_VL53L8_TARGET_ORDER_CLOSEST    1u
```

### DEPZ_VL53L8_TARGET_ORDER_STRONGEST

```c
#define DEPZ_VL53L8_TARGET_ORDER_STRONGEST  2u
```

### DEPZ_VL53L8_XTALK_BUFFER_SIZE

```c
#define DEPZ_VL53L8_XTALK_BUFFER_SIZE       776u
```

### DEPZ_VL53L8_CNH_MAX_BYTES

```c
#define DEPZ_VL53L8_CNH_MAX_BYTES           6160u
```

on-device CNH buffer cap

### DEPZ_VL53L8_THRESH_IN_WINDOW

```c
#define DEPZ_VL53L8_THRESH_IN_WINDOW           0u
```

Detection-threshold windows and combine operations (plugin constants).

### DEPZ_VL53L8_THRESH_OUT_OF_WINDOW

```c
#define DEPZ_VL53L8_THRESH_OUT_OF_WINDOW       1u
```

### DEPZ_VL53L8_THRESH_LESS_THAN_EQUAL_MIN

```c
#define DEPZ_VL53L8_THRESH_LESS_THAN_EQUAL_MIN 2u
```

### DEPZ_VL53L8_THRESH_GREATER_THAN_MAX

```c
#define DEPZ_VL53L8_THRESH_GREATER_THAN_MAX    3u
```

### DEPZ_VL53L8_THRESH_EQUAL_MIN

```c
#define DEPZ_VL53L8_THRESH_EQUAL_MIN           4u
```

### DEPZ_VL53L8_THRESH_NOT_EQUAL_MIN

```c
#define DEPZ_VL53L8_THRESH_NOT_EQUAL_MIN       5u
```

### DEPZ_VL53L8_THRESH_OP_OR

```c
#define DEPZ_VL53L8_THRESH_OP_OR               0u
```

### DEPZ_VL53L8_THRESH_OP_AND

```c
#define DEPZ_VL53L8_THRESH_OP_AND              2u
```

### DEPZ_VL53L8_THRESH_LAST

```c
#define DEPZ_VL53L8_THRESH_LAST                128u
```

OR into the zone_num of the last threshold programmed (ST LAST_THRESHOLD).

### depz_vl53l8_motion

```c
typedef struct {
    uint32_t global_indicator_1, global_indicator_2;
    uint8_t  status, nb_of_detected_aggregates, nb_of_aggregates;
    uint32_t motion[32];
} depz_vl53l8_motion;
```

The motion-indicator output of a frame (configure_motion_indicator).

### depz_vl53l8_live_frame

```c
typedef struct {
    depz_vl53l8_frame  f;
    bool               has_motion;
    depz_vl53l8_motion motion;
    size_t             cnh_len;
    uint8_t            cnh[DEPZ_VL53L8_CNH_MAX_BYTES];
} depz_vl53l8_live_frame;
```

One streamed frame. `f` holds the zone arrays (distance_mm in mm, sigma
raw/128, ...; `f.resolution` zones, row-major). CH with CNH armed: the raw
CNH block in cnh[0..cnh_len) — decode it with depz_vl53l8ch_decode_cnh().

### depz_vl53l8_frame_cb

```c
typedef void (*depz_vl53l8_frame_cb)(const depz_vl53l8_live_frame *f, void *user);
```

### depz_vl53l8_progress_cb

```c
typedef void (*depz_vl53l8_progress_cb)(const char *phase, size_t done, size_t total, void *user);
```

init() progress: a phase text, and for the big blob writes done/total bytes
(both 0 otherwise). Runs on the calling thread.

### depz_vl53l8_cnh_setup

```c
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
```

CNH (compact network histograms, CH only): VL53LMZ_Motion_Configuration and
the plugin helpers that fill it (vl53lmz_plugin_cnh.c).

### depz_vl53l8_cnh_init_config

```c
void depz_vl53l8_cnh_init_config(depz_vl53l8_cnh_setup *s, int start_bin, int num_bins, int sub_sample);
```

vl53lmz_cnh_init_config: histogram start bin, CNH bins, device bins per CNH bin.

### depz_vl53l8_cnh_create_agg_map

```c
int depz_vl53l8_cnh_create_agg_map(depz_vl53l8_cnh_setup *s, int resolution, int start_x, int start_y, int merge_x, int merge_y, int cols, int rows);
```

vl53lmz_cnh_create_agg_map: map zones (resolution 16 | 64) to aggregates.

### depz_vl53l8_cnh_required_memory

```c
int depz_vl53l8_cnh_required_memory(const depz_vl53l8_cnh_setup *s, size_t *bytes);
```

On-device CNH buffer bytes; DEPZ_E_ARG when blank or above the cap.

### depz_vl53l8_cnh_pack

```c
void depz_vl53l8_cnh_pack(const depz_vl53l8_cnh_setup *s, uint8_t out[156]);
```

The 156-byte struct cnh_send_config writes.

### depz_vl53l8_cnh_decode_config

```c
void depz_vl53l8_cnh_decode_config(const depz_vl53l8_cnh_setup *s, depz_vl53l8ch_cnh_config *out);
```

The decode parameters of a setup, for depz_vl53l8ch_decode_cnh().

### depz_vl53l8_open_link

```c
int depz_vl53l8_open_link(depz_link *link, depz_vl53l8_model model, depz_device **out);
```

A multizone ToF on a link without an identity probe (tests, replay).

### depz_is_vl53l8

```c
bool depz_is_vl53l8(const depz_device *dev);
```

### depz_vl53l8_get_model

```c
depz_vl53l8_model depz_vl53l8_get_model(const depz_device *dev);
```

### depz_vl53l8_is_alive

```c
int depz_vl53l8_is_alive(depz_device *dev, bool *alive, uint8_t *device_id, uint8_t *revision_id);
```

Probe: device id / revision (0xF0 / 0x0C on VL53L8, revision 0x02 on L5/L7).
It and get_power_mode switch the register bank: not while ranging.

### depz_vl53l8_init

```c
int depz_vl53l8_init(depz_device *dev, depz_vl53l8_progress_cb progress, void *user);
```

Boot the sensor MCU, download its firmware, upload NVM offset data, the
default xtalk and configuration. `progress` may be NULL.

### depz_vl53l8_initialized

```c
bool depz_vl53l8_initialized(const depz_device *dev);
```

### depz_vl53l8_ranging

```c
bool depz_vl53l8_ranging(const depz_device *dev);
```

### depz_vl53l8_get_resolution

```c
int depz_vl53l8_get_resolution(depz_device *dev, int *zones);
```

Configuration — init() first, never while ranging.
/* 16 | 64 */

### depz_vl53l8_set_resolution

```c
int depz_vl53l8_set_resolution(depz_device *dev, int zones);
```

### depz_vl53l8_get_ranging_frequency_hz

```c
int depz_vl53l8_get_ranging_frequency_hz(depz_device *dev, uint8_t *hz);
```

>= 2 Hz on VL53L8 (below that it never ranges), >= 1 Hz on L5/L7; max 60
at 4x4, 15 at 8x8.

### depz_vl53l8_set_ranging_frequency_hz

```c
int depz_vl53l8_set_ranging_frequency_hz(depz_device *dev, uint8_t hz);
```

### depz_vl53l8_get_ranging_mode

```c
int depz_vl53l8_get_ranging_mode(depz_device *dev, uint8_t *mode);
```

### depz_vl53l8_set_ranging_mode

```c
int depz_vl53l8_set_ranging_mode(depz_device *dev, uint8_t mode);
```

### depz_vl53l8_get_integration_time_ms

```c
int depz_vl53l8_get_integration_time_ms(depz_device *dev, uint32_t *ms);
```

2..1000 ms; autonomous mode only.

### depz_vl53l8_set_integration_time_ms

```c
int depz_vl53l8_set_integration_time_ms(depz_device *dev, uint32_t ms);
```

### depz_vl53l8_get_sharpener_percent

```c
int depz_vl53l8_get_sharpener_percent(depz_device *dev, uint8_t *pct);
```

0..99 % (0 = off); read back rounded to nearest, like the Python SDK.

### depz_vl53l8_set_sharpener_percent

```c
int depz_vl53l8_set_sharpener_percent(depz_device *dev, uint8_t pct);
```

### depz_vl53l8_get_target_order

```c
int depz_vl53l8_get_target_order(depz_device *dev, uint8_t *order);
```

### depz_vl53l8_set_target_order

```c
int depz_vl53l8_set_target_order(depz_device *dev, uint8_t order);
```

### depz_vl53l8_get_power_mode

```c
int depz_vl53l8_get_power_mode(depz_device *dev, uint8_t *mode);
```

DEPZ_VL53L8_POWER_MODE_*; waking from deep sleep re-runs init().

### depz_vl53l8_set_power_mode

```c
int depz_vl53l8_set_power_mode(depz_device *dev, uint8_t mode);
```

### depz_vl53l8_get_xtalk_margin

```c
int depz_vl53l8_get_xtalk_margin(depz_device *dev, double *kcps_per_spad);
```

### depz_vl53l8_set_xtalk_margin

```c
int depz_vl53l8_set_xtalk_margin(depz_device *dev, double kcps_per_spad);
```

/* <= 10000 */

### depz_vl53l8_calibrate_xtalk

```c
int depz_vl53l8_calibrate_xtalk(depz_device *dev, uint8_t reflectance_percent, uint8_t nb_samples, uint16_t distance_mm, bool *failed);
```

Crosstalk calibration against a flat target (reflectance 1..99 %,
nb_samples 1..16, distance 600..3000 mm); blocks several seconds. With no
cover glass the firmware answers "nothing to calibrate": *failed (optional)
is then true and the default xtalk data stays.

### depz_vl53l8_get_caldata_xtalk

```c
int depz_vl53l8_get_caldata_xtalk(depz_device *dev, uint8_t out[776]);
```

The 776-byte xtalk calibration blob (save / restore).

### depz_vl53l8_set_caldata_xtalk

```c
int depz_vl53l8_set_caldata_xtalk(depz_device *dev, const uint8_t blob[776]);
```

### depz_vl53l8_get_detection_thresholds_enable

```c
int depz_vl53l8_get_detection_thresholds_enable(depz_device *dev, bool *enabled);
```

### depz_vl53l8_set_detection_thresholds_enable

```c
int depz_vl53l8_set_detection_thresholds_enable(depz_device *dev, bool enabled);
```

### depz_vl53l8_get_detection_thresholds

```c
int depz_vl53l8_get_detection_thresholds(depz_device *dev, depz_vl53l8_threshold out[64]);
```

All 64 thresholds, low/high in real units of their measurement.

### depz_vl53l8_set_detection_thresholds

```c
int depz_vl53l8_set_detection_thresholds(depz_device *dev, const depz_vl53l8_threshold *th, size_t n);
```

### depz_vl53l8_set_detection_thresholds_auto_stop

```c
int depz_vl53l8_set_detection_thresholds_auto_stop(depz_device *dev, bool auto_stop);
```

### depz_vl53l8_configure_motion_indicator

```c
int depz_vl53l8_configure_motion_indicator(depz_device *dev, uint16_t min_mm, uint16_t max_mm);
```

Motion indicator over [min, max] mm (400..4000, span <= 1500); frames then
carry `motion`.

### depz_vl53l8_configure_cnh

```c
int depz_vl53l8_configure_cnh(depz_device *dev, const depz_vl53l8_cnh_setup *setup);
```

CH only: arm the CNH block for the next start_ranging(). init() disarms it
(the fresh sensor holds no CNH configuration).

### depz_vl53l8_start_ranging

```c
int depz_vl53l8_start_ranging(depz_device *dev);
```

Ranging.

### depz_vl53l8_stop_ranging

```c
int depz_vl53l8_stop_ranging(depz_device *dev);
```

/* idempotent */

### depz_vl53l8_on_frame

```c
int depz_vl53l8_on_frame(depz_device *dev, depz_vl53l8_frame_cb cb, void *user, int *token);
```

### depz_vl53l8_off_frame

```c
void depz_vl53l8_off_frame(depz_device *dev, int token);
```

### depz_vl53l8_frames

```c
depz_stream *depz_vl53l8_frames(depz_device *dev, size_t maxsize);
```

Pull stream of depz_vl53l8_live_frame items (~7.5 KB each: keep maxsize
small; 0 = 8).

### depz_vl53l8_get_frame

```c
int depz_vl53l8_get_frame(depz_device *dev, int timeout_ms, depz_vl53l8_live_frame *out);
```

The next frame (< 0 timeout = 2000 ms).

### depz_vl53l8_frame_parse_errors

```c
uint64_t depz_vl53l8_frame_parse_errors(const depz_device *dev);
```

Frames dropped because they did not parse / chunked frames the reassembler
discarded (gaps).

### depz_vl53l8_reassembler_discards

```c
uint64_t depz_vl53l8_reassembler_discards(const depz_device *dev);
```

### depz_vl53l8_module_type

```c
int depz_vl53l8_module_type(const depz_device *dev);
```

L5/L7 only (DEPZ_E_WRONG_TYPE on VL53L8): the I2C bridge's own commands.
Sensor module type read at init(): 0 MZ = VL53L5CX, 1 MZEVO = VL53L7CX/CH,
-1 before init. L5 and L7 share a blob and a board: this is how the silicon
tells them apart (never CX from CH).

### depz_vl53l7_bridge_info

```c
int depz_vl53l7_bridge_info(depz_device *dev, depz_vl53l7_info *out);
```

Bridge counters and pin levels; read before / after a run, not during one
(each call takes the bus from the stream and can cost a frame).

### depz_vl53l7_set_i2c_speed_khz

```c
int depz_vl53l7_set_i2c_speed_khz(depz_device *dev, uint16_t khz, uint16_t *effective);
```

SCL snaps to 100, 200, 400, 500 ... 1000 kHz; *effective (optional) = the
value now in effect. DEPZ_E_BUSY mid-transfer — stop ranging first.

### depz_vl53l7_pin_ctrl

```c
int depz_vl53l7_pin_ctrl(depz_device *dev, uint8_t action);
```

DEPZ_VL53L7_PIN_*: LPN_OFF and SOFT_CYCLE drop the sensor's state — run
init() again (firmware download included).

### depz_vl53l8_read_reg

```c
int depz_vl53l8_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
```

Escape hatches: raw registers (16-bit address) and DCI indices.

### depz_vl53l8_write_reg

```c
int depz_vl53l8_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);
```

### depz_vl53l8_dci_read

```c
int depz_vl53l8_dci_read(depz_device *dev, uint16_t index, uint8_t *buf, size_t len);
```

### depz_vl53l8_dci_write

```c
int depz_vl53l8_dci_write(depz_device *dev, uint16_t index, const uint8_t *data, size_t len);
```

## VL53L 1D family sensor class (live layer)

### DEPZ_VL53LX_CAP_MODE

```c
#define DEPZ_VL53LX_CAP_MODE          0x0001u
```

Optional capability groups (depz_vl53lx_supports).
named ranging modes

### DEPZ_VL53LX_CAP_TIMING

```c
#define DEPZ_VL53LX_CAP_TIMING        0x0002u
```

### DEPZ_VL53LX_CAP_OFFSET

```c
#define DEPZ_VL53LX_CAP_OFFSET        0x0004u
```

### DEPZ_VL53LX_CAP_XTALK

```c
#define DEPZ_VL53LX_CAP_XTALK         0x0008u
```

### DEPZ_VL53LX_CAP_CALIB_OFFSET

```c
#define DEPZ_VL53LX_CAP_CALIB_OFFSET  0x0010u
```

### DEPZ_VL53LX_CAP_CALIB_XTALK

```c
#define DEPZ_VL53LX_CAP_CALIB_XTALK   0x0020u
```

### DEPZ_VL53LX_CAP_THRESHOLDS

```c
#define DEPZ_VL53LX_CAP_THRESHOLDS    0x0040u
```

distance-window interrupt

### DEPZ_VL53LX_CAP_SIGNAL_THRESH

```c
#define DEPZ_VL53LX_CAP_SIGNAL_THRESH 0x0080u
```

### DEPZ_VL53LX_CAP_SIGMA_THRESH

```c
#define DEPZ_VL53LX_CAP_SIGMA_THRESH  0x0100u
```

### DEPZ_VL53LX_CAP_ROI

```c
#define DEPZ_VL53LX_CAP_ROI           0x0200u
```

### DEPZ_VL53LX_CAP_TEMP_UPDATE

```c
#define DEPZ_VL53LX_CAP_TEMP_UPDATE   0x0400u
```

### DEPZ_VL53LX_CAP_REFSPAD

```c
#define DEPZ_VL53LX_CAP_REFSPAD       0x0800u
```

VL53L0X reference SPADs

### DEPZ_VL53LX_WINDOW_BELOW

```c
#define DEPZ_VL53LX_WINDOW_BELOW 0u
```

Detection-threshold windows (the die ULDs' SYSTEM__INTERRUPT_CONFIG).

### DEPZ_VL53LX_WINDOW_ABOVE

```c
#define DEPZ_VL53LX_WINDOW_ABOVE 1u
```

### DEPZ_VL53LX_WINDOW_OUT

```c
#define DEPZ_VL53LX_WINDOW_OUT   2u
```

### DEPZ_VL53LX_WINDOW_IN

```c
#define DEPZ_VL53LX_WINDOW_IN    3u
```

### DEPZ_VL53LX_MAX_TARGETS

```c
#define DEPZ_VL53LX_MAX_TARGETS 4
```

### DEPZ_VL53LX_MAX_MODES

```c
#define DEPZ_VL53LX_MAX_MODES   4
```

### depz_vl53lx_target

```c
typedef struct {
    int32_t     distance_mm;
    int         status;
    const char *status_text;  /* static string */
    double      signal_kcps, ambient_kcps, sigma_mm;
    int32_t     min_range_mm, max_range_mm;
} depz_vl53lx_target;
```

One return of a histogram frame. min / max_range_mm are the edges of the
target's own pulse.

### depz_vl53lx_bins

```c
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
```

The histogram frame the targets were found in (VL53LX_histogram_bin_data_t
after the driver read it: the VCSEL period, the bin sequence, the ambient
estimate). bin_data[0 .. number_of_bins) are the photon counts.

### depz_vl53lx_measurement

```c
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
```

One ranging result, the same shape for every product. On the histogram
driver `targets` holds every return (the top-level fields repeat
targets[0]) and `bins` the raw frame; the light drivers leave both empty.
The driver extras read -1 (integers) or 0 where the driver has none. Use
depz_vl53lx_plottable() rather than status == 0 on histogram products.

### depz_vl53lx_measurement_cb

```c
typedef void (*depz_vl53lx_measurement_cb)(const depz_vl53lx_measurement *m, void *user);
```

### depz_vl53lx_status_plottable

```c
bool depz_vl53lx_status_plottable(int status);
```

Statuses that mean "this distance is real": 0, 6 (first histogram frame,
no wrap check yet) and 11 (merged target).

### depz_vl53lx_plottable

```c
bool depz_vl53lx_plottable(const depz_vl53lx_measurement *m);
```

### depz_vl53lx_primary_distance

```c
bool depz_vl53lx_primary_distance(const depz_vl53lx_measurement *m, int32_t *distance_mm);
```

The first plottable target (or the single distance of a light driver);
false when the frame has none.

### depz_vl53lx_open_link

```c
int depz_vl53lx_open_link(depz_link *link, depz_device **out);
```

A 1D-family board on a link without an identity probe (tests, replay).

### depz_is_vl53lx

```c
bool depz_is_vl53lx(const depz_device *dev);
```

### depz_vl53lx_detected_product

```c
depz_vl53lx_product depz_vl53lx_detected_product(depz_device *dev);
```

The product the board's device name carries (read once, cached), or
DEPZ_VL53LX_PRODUCT_NONE on an unstamped board — then name it at init().

### depz_vl53lx_driver_kinds

```c
unsigned depz_vl53lx_driver_kinds(depz_vl53lx_product product);
```

The driver kinds a product has (OR of depz_vl53lx_driver).

### depz_vl53lx_init

```c
int depz_vl53lx_init(depz_device *dev, depz_vl53lx_driver driver, depz_vl53lx_product product);
```

Bind (product, driver) and initialise the sensor. `product` NONE = the
detected one; `driver` 0 = the product's first kind (uld, ulp, histogram).
A pair without a driver: DEPZ_E_ARG naming what the product has.

### depz_vl53lx_initialized

```c
bool depz_vl53lx_initialized(const depz_device *dev);
```

### depz_vl53lx_product_bound

```c
depz_vl53lx_product depz_vl53lx_product_bound(const depz_device *dev);
```

### depz_vl53lx_driver_bound

```c
depz_vl53lx_driver depz_vl53lx_driver_bound(const depz_device *dev);
```

### depz_vl53lx_caveat

```c
const char *depz_vl53lx_caveat(const depz_device *dev);
```

The driver's caveat for the pair ("" when none).

### depz_vl53lx_driver_reach_mm

```c
uint32_t depz_vl53lx_driver_reach_mm(const depz_device *dev);
```

What the configuration in use can reach, mm (0 = not characterised, or a
failed read). Read off the sensor: the valid phase window on the VL53L1 die,
the final-range VCSEL period's figure on the VL53L0X. The histogram driver
answers its preset's window host-side, with no traffic.

### depz_vl53lx_supports

```c
bool depz_vl53lx_supports(const depz_device *dev, unsigned cap);
```

### depz_vl53lx_ranging

```c
bool depz_vl53lx_ranging(const depz_device *dev);
```

### depz_vl53lx_model_id

```c
int depz_vl53lx_model_id(depz_device *dev, uint16_t *model_id);
```

The model-id register (L1CX = L1CB, L4CD = L4CX: a cross-check only).

### depz_vl53lx_modes

```c
size_t depz_vl53lx_modes(const depz_device *dev, const char **names, size_t cap);
```

Named ranging modes of the bound driver in UI order (get_mode() says which
one init left — medium on the histogram driver):
returns how many, fills up to `cap` static names.

### depz_vl53lx_budget_range

```c
int depz_vl53lx_budget_range(const depz_device *dev, int *min_ms, int *max_ms);
```

Budget range (inclusive) and, on products that only take some values, the
choices (ascending; *n = 0 when any integer in the range will do).

### depz_vl53lx_budget_choices

```c
int depz_vl53lx_budget_choices(depz_device *dev, int *choices, size_t cap, size_t *n);
```

### depz_vl53lx_xshut

```c
int depz_vl53lx_xshut(depz_device *dev, uint8_t action);
```

XSHUT pin (DEPZ_VL53L4_XSHUT_*): OFF and RESET stop the stream and forget
init — init() again.

### depz_vl53lx_bridge_info

```c
int depz_vl53lx_bridge_info(depz_device *dev, depz_vl53lx_info *out);
```

Bridge counters and settings; safe while streaming.

### depz_vl53lx_configure

```c
int depz_vl53lx_configure(depz_device *dev, int budget_ms, int inter_ms, const char *mode, const int32_t *offset_mm, const int32_t *xtalk_kcps);
```

Re-initialise and apply a configuration: `mode` NULL = leave the init
default, `offset_mm` / `xtalk_kcps` NULL = leave (a stored calibration
re-applied otherwise). inter_ms 0 = continuous. An unsupported group is
refused before the re-init.

### depz_vl53lx_configure_ex

```c
int depz_vl53lx_configure_ex(depz_device *dev, int budget_ms, int inter_ms, const char *mode, const int32_t *offset_mm, const int32_t *xtalk_kcps, const int *signal_kcps);
```

depz_vl53lx_configure() plus `signal_kcps` (NULL = leave): replaces the
blob's signal threshold, applied last. The re-init puts it back to the
default, so a lowered one goes here, not in a set_signal_threshold_kcps()
before configure: frames past the default threshold come back status 2 with
the distance right (L1 long at ~4 m, a light driver borrowed onto a die
without the lens it was tuned for).

### depz_vl53lx_get_range_timing

```c
int depz_vl53lx_get_range_timing(depz_device *dev, int *budget_ms, int *inter_ms);
```

### depz_vl53lx_set_mode

```c
int depz_vl53lx_set_mode(depz_device *dev, const char *mode);
```

### depz_vl53lx_get_mode

```c
int depz_vl53lx_get_mode(depz_device *dev, const char **mode);
```

The mode in use; NULL (and DEPZ_OK) on a product without modes.

### depz_vl53lx_get_offset_mm

```c
int depz_vl53lx_get_offset_mm(depz_device *dev, int32_t *offset_mm);
```

Capability-gated settings (init() first, not while ranging).

### depz_vl53lx_set_offset_mm

```c
int depz_vl53lx_set_offset_mm(depz_device *dev, int32_t offset_mm);
```

### depz_vl53lx_get_xtalk_kcps

```c
int depz_vl53lx_get_xtalk_kcps(depz_device *dev, int32_t *xtalk_kcps);
```

### depz_vl53lx_set_xtalk_kcps

```c
int depz_vl53lx_set_xtalk_kcps(depz_device *dev, int32_t xtalk_kcps);
```

### depz_vl53lx_calibrate_offset

```c
int depz_vl53lx_calibrate_offset(depz_device *dev, int target_mm, int nb_samples, int32_t *offset_mm);
```

Against a flat target; nb_samples 0 = the driver's default. Returns the
value now programmed — store it: the sensor forgets it on reset.

### depz_vl53lx_calibrate_xtalk

```c
int depz_vl53lx_calibrate_xtalk(depz_device *dev, int target_mm, int nb_samples, int32_t *xtalk_kcps);
```

### depz_vl53lx_get_detection_thresholds

```c
int depz_vl53lx_get_detection_thresholds(depz_device *dev, int *low_mm, int *high_mm, int *window);
```

### depz_vl53lx_set_detection_thresholds

```c
int depz_vl53lx_set_detection_thresholds(depz_device *dev, int low_mm, int high_mm, int window);
```

### depz_vl53lx_get_signal_threshold_kcps

```c
int depz_vl53lx_get_signal_threshold_kcps(depz_device *dev, int *kcps);
```

### depz_vl53lx_set_signal_threshold_kcps

```c
int depz_vl53lx_set_signal_threshold_kcps(depz_device *dev, int kcps);
```

### depz_vl53lx_get_sigma_threshold_mm

```c
int depz_vl53lx_get_sigma_threshold_mm(depz_device *dev, int *mm);
```

### depz_vl53lx_set_sigma_threshold_mm

```c
int depz_vl53lx_set_sigma_threshold_mm(depz_device *dev, int mm);
```

### depz_vl53lx_get_roi

```c
int depz_vl53lx_get_roi(depz_device *dev, int *x, int *y);
```

### depz_vl53lx_set_roi

```c
int depz_vl53lx_set_roi(depz_device *dev, int x, int y);
```

### depz_vl53lx_get_roi_center

```c
int depz_vl53lx_get_roi_center(depz_device *dev, int *spad);
```

### depz_vl53lx_set_roi_center

```c
int depz_vl53lx_set_roi_center(depz_device *dev, int spad);
```

### depz_vl53lx_start_temperature_update

```c
int depz_vl53lx_start_temperature_update(depz_device *dev);
```

### depz_vl53lx_perform_ref_spad_management

```c
int depz_vl53lx_perform_ref_spad_management(depz_device *dev, uint32_t *count, bool *is_aperture);
```

VL53L0X: re-measure the reference SPADs -> (count, is_aperture).

### depz_vl53lx_start_ranging

```c
int depz_vl53lx_start_ranging(depz_device *dev);
```

Ranging.

### depz_vl53lx_stop_ranging

```c
int depz_vl53lx_stop_ranging(depz_device *dev);
```

/* idempotent */

### depz_vl53lx_measure_once

```c
int depz_vl53lx_measure_once(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out);
```

Poll mode (< 0 timeout = 1000 ms); refused while the stream runs.

### depz_vl53lx_on_measurement

```c
int depz_vl53lx_on_measurement(depz_device *dev, depz_vl53lx_measurement_cb cb, void *user, int *token);
```

### depz_vl53lx_off_measurement

```c
void depz_vl53lx_off_measurement(depz_device *dev, int token);
```

### depz_vl53lx_measurements

```c
depz_stream *depz_vl53lx_measurements(depz_device *dev, size_t maxsize);
```

/* 0 = 64 */

### depz_vl53lx_get_measurement

```c
int depz_vl53lx_get_measurement(depz_device *dev, int timeout_ms, depz_vl53lx_measurement *out);
```

The next streamed measurement (< 0 timeout = 2000 ms).

### depz_vl53lx_stream_parse_errors

```c
uint64_t depz_vl53lx_stream_parse_errors(const depz_device *dev);
```

### depz_vl53lx_read_reg

```c
int depz_vl53lx_read_reg(depz_device *dev, uint16_t addr, uint8_t *buf, size_t len);
```

Raw register access (escape hatch), at the bound address width.

### depz_vl53lx_write_reg

```c
int depz_vl53lx_write_reg(depz_device *dev, uint16_t addr, const uint8_t *data, size_t len);
```

## BNO055 sensor class (live layer)

### DEPZ_BNO055_POWER_NORMAL

```c
#define DEPZ_BNO055_POWER_NORMAL  0u
```

### DEPZ_BNO055_POWER_LOW

```c
#define DEPZ_BNO055_POWER_LOW     1u
```

### DEPZ_BNO055_POWER_SUSPEND

```c
#define DEPZ_BNO055_POWER_SUSPEND 2u
```

### DEPZ_BNO055_TEMP_FROM_ACCEL

```c
#define DEPZ_BNO055_TEMP_FROM_ACCEL 0u
```

### DEPZ_BNO055_TEMP_FROM_GYRO

```c
#define DEPZ_BNO055_TEMP_FROM_GYRO  1u
```

### DEPZ_BNO055_HAS_ACCEL

```c
#define DEPZ_BNO055_HAS_ACCEL        0x001u
```

Which channels a sample carries (the block covered them whole).

### DEPZ_BNO055_HAS_MAG

```c
#define DEPZ_BNO055_HAS_MAG          0x002u
```

### DEPZ_BNO055_HAS_GYRO

```c
#define DEPZ_BNO055_HAS_GYRO         0x004u
```

### DEPZ_BNO055_HAS_EULER

```c
#define DEPZ_BNO055_HAS_EULER        0x008u
```

### DEPZ_BNO055_HAS_QUATERNION

```c
#define DEPZ_BNO055_HAS_QUATERNION   0x010u
```

### DEPZ_BNO055_HAS_LINEAR_ACCEL

```c
#define DEPZ_BNO055_HAS_LINEAR_ACCEL 0x020u
```

### DEPZ_BNO055_HAS_GRAVITY

```c
#define DEPZ_BNO055_HAS_GRAVITY      0x040u
```

### DEPZ_BNO055_HAS_TEMPERATURE

```c
#define DEPZ_BNO055_HAS_TEMPERATURE  0x080u
```

### DEPZ_BNO055_HAS_CALIBRATION

```c
#define DEPZ_BNO055_HAS_CALIBRATION  0x100u
```

### depz_bno055_sample

```c
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
```

One decoded register block, scaled by the units in effect when it was read
(streams: the units at start_stream). Fusion outputs read zero outside the
fusion modes; linear_accel / gravity are always m/s^2.

### depz_bno055_status_regs

```c
typedef struct { uint8_t self_test, clk_status, status, error; } depz_bno055_status_regs;
```

ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.

### depz_bno055_sample_cb

```c
typedef void (*depz_bno055_sample_cb)(const depz_bno055_sample *s, void *user);
```

### depz_bno055_decode_sample

```c
void depz_bno055_decode_sample(uint64_t timestamp_us, uint8_t addr, const uint8_t *data, size_t len, const depz_bno055_units *units, depz_bno055_sample *out);
```

Decode and scale a register block (pure).

### depz_bno055_open_link

```c
int depz_bno055_open_link(depz_link *link, depz_device **out);
```

A BNO055 on a link without an identity probe (tests, replay).

### depz_is_bno055

```c
bool depz_is_bno055(const depz_device *dev);
```

### depz_bno055_bridge_info

```c
int depz_bno055_bridge_info(depz_device *dev, depz_bno055_info *out);
```

Chip ids, sensor firmware revision, bridge counters; safe while streaming.

### depz_bno055_is_alive

```c
int depz_bno055_is_alive(depz_device *dev, bool *alive);
```

The bridge passed its chip-id handshake and the ids are the BNO055's.

### depz_bno055_reset_sensor

```c
int depz_bno055_reset_sensor(depz_device *dev);
```

nRESET (~0.5 s handshake): stops any stream; CONFIG mode, power-on units,
no calibration — configure() again or restore_configuration().

### depz_bno055_read_registers

```c
int depz_bno055_read_registers(depz_device *dev, uint8_t addr, uint8_t *buf, size_t len, int page);
```

Raw registers on page 0 or 1 (page 1 refused while streaming). Most
configuration registers only take writes in CONFIG mode.

### depz_bno055_write_registers

```c
int depz_bno055_write_registers(depz_device *dev, uint8_t addr, const uint8_t *data, size_t len, int page);
```

### depz_bno055_get_operation_mode

```c
int depz_bno055_get_operation_mode(depz_device *dev, uint8_t *mode);
```

Mode switches go through CONFIG (the sensor ignores mode -> mode) and wait
the datasheet times; into a fusion mode they wait for the fusion to run.

### depz_bno055_set_operation_mode

```c
int depz_bno055_set_operation_mode(depz_device *dev, uint8_t mode);
```

### depz_bno055_get_power_mode

```c
int depz_bno055_get_power_mode(depz_device *dev, uint8_t *mode);
```

### depz_bno055_set_power_mode

```c
int depz_bno055_set_power_mode(depz_device *dev, uint8_t mode);
```

### depz_bno055_get_units

```c
int depz_bno055_get_units(depz_device *dev, depz_bno055_units *out);
```

### depz_bno055_set_units

```c
int depz_bno055_set_units(depz_device *dev, const depz_bno055_units *units);
```

### depz_bno055_get_axis_remap

```c
int depz_bno055_get_axis_remap(depz_device *dev, depz_bno055_axis_remap *out);
```

### depz_bno055_set_axis_remap

```c
int depz_bno055_set_axis_remap(depz_device *dev, const depz_bno055_axis_remap *remap);
```

### depz_bno055_set_axis_placement

```c
int depz_bno055_set_axis_placement(depz_device *dev, const char *placement);
```

A datasheet placement "P0".."P7" (P1 = default).

### depz_bno055_get_temperature_source

```c
int depz_bno055_get_temperature_source(depz_device *dev, uint8_t *source);
```

### depz_bno055_set_temperature_source

```c
int depz_bno055_set_temperature_source(depz_device *dev, uint8_t source);
```

### depz_bno055_configure

```c
int depz_bno055_configure(depz_device *dev, uint8_t mode, const depz_bno055_units *units, const depz_bno055_axis_remap *remap, const depz_bno055_calib_profile *calibration);
```

The usual session setup: CONFIG -> units (NULL = defaults) -> axis remap
(NULL = leave) -> calibration profile (NULL = leave) -> `mode`; in a fusion
mode it returns once the fusion outputs are live. Remembered for
restore_configuration() (after a reset, or when sensor_resets rose).

### depz_bno055_restore_configuration

```c
int depz_bno055_restore_configuration(depz_device *dev);
```

### depz_bno055_system_status

```c
int depz_bno055_system_status(depz_device *dev, depz_bno055_status_regs *out);
```

Status, self-test, calibration.

### depz_bno055_self_test

```c
int depz_bno055_self_test(depz_device *dev, depz_bno055_status_regs *out);
```

~0.45 s in CONFIG mode, the operating mode restored. Not while streaming.

### depz_bno055_calibration_status

```c
int depz_bno055_calibration_status(depz_device *dev, depz_bno055_calib_status *out);
```

### depz_bno055_read_calibration_profile

```c
int depz_bno055_read_calibration_profile(depz_device *dev, depz_bno055_calib_profile *out);
```

Offsets and radii; the sensor exposes them in CONFIG mode only (the driver
goes there and back). A written profile is a starting point: fusion refines
it at once.

### depz_bno055_write_calibration_profile

```c
int depz_bno055_write_calibration_profile(depz_device *dev, const depz_bno055_calib_profile *p);
```

### depz_bno055_get_sic_matrix

```c
int depz_bno055_get_sic_matrix(depz_device *dev, int16_t out[9]);
```

Soft-iron matrix, 9 x i16 row-major, 1.0 = 16384.

### depz_bno055_set_sic_matrix

```c
int depz_bno055_set_sic_matrix(depz_device *dev, const int16_t m[9]);
```

### depz_bno055_get_accel_config

```c
int depz_bno055_get_accel_config(depz_device *dev, depz_bno055_accel_config *out);
```

Page 1: raw sensor configuration (non-fusion modes only; fusion overrides
it), unique id, motion interrupts (the only interrupts on SW rev 03.11).

### depz_bno055_set_accel_config

```c
int depz_bno055_set_accel_config(depz_device *dev, const depz_bno055_accel_config *c);
```

### depz_bno055_get_gyro_config

```c
int depz_bno055_get_gyro_config(depz_device *dev, depz_bno055_gyro_config *out);
```

### depz_bno055_set_gyro_config

```c
int depz_bno055_set_gyro_config(depz_device *dev, const depz_bno055_gyro_config *c);
```

### depz_bno055_get_mag_config

```c
int depz_bno055_get_mag_config(depz_device *dev, depz_bno055_mag_config *out);
```

### depz_bno055_set_mag_config

```c
int depz_bno055_set_mag_config(depz_device *dev, const depz_bno055_mag_config *c);
```

### depz_bno055_unique_id

```c
int depz_bno055_unique_id(depz_device *dev, uint8_t out[16]);
```

### depz_bno055_get_interrupt_enable

```c
int depz_bno055_get_interrupt_enable(depz_device *dev, uint8_t *mask);
```

### depz_bno055_set_interrupt_enable

```c
int depz_bno055_set_interrupt_enable(depz_device *dev, uint8_t mask);
```

### depz_bno055_get_interrupt_mask

```c
int depz_bno055_get_interrupt_mask(depz_device *dev, uint8_t *mask);
```

### depz_bno055_set_interrupt_mask

```c
int depz_bno055_set_interrupt_mask(depz_device *dev, uint8_t mask);
```

### depz_bno055_set_interrupt_setting

```c
int depz_bno055_set_interrupt_setting(depz_device *dev, uint8_t reg, uint8_t value);
```

One page-1 motion-interrupt setting (0x11..0x1F), raw.

### depz_bno055_read_interrupt_status

```c
int depz_bno055_read_interrupt_status(depz_device *dev, uint8_t *status);
```

INT_STA — clears on read.

### depz_bno055_clear_interrupt

```c
int depz_bno055_clear_interrupt(depz_device *dev);
```

SYS_TRIGGER RST_INT: reset the status bits and the INT pin.

### depz_bno055_read_sample

```c
int depz_bno055_read_sample(depz_device *dev, uint8_t addr, uint8_t len, depz_bno055_sample *out);
```

Data.
Poll a block (addr, len = DEPZ_BNO055_FULL_BLOCK_* by default); works
alongside a stream.

### depz_bno055_read_quaternion

```c
int depz_bno055_read_quaternion(depz_device *dev, double q[4]);
```

/* w, x, y, z */

### depz_bno055_start_stream

```c
int depz_bno055_start_stream(depz_device *dev, uint16_t period_ms, uint8_t addr, uint8_t len, uint8_t trigger);
```

Read `len` bytes at `addr` every `period_ms` (TIMER; fusion runs at 100 Hz,
so 10 ms is the useful floor) — or on each INT edge (motion interrupts only
on SW 03.11) with `period_ms` as a watchdog, 0 = none. Replaces a stream.

### depz_bno055_stop_stream

```c
int depz_bno055_stop_stream(depz_device *dev);
```

### depz_bno055_streaming

```c
bool depz_bno055_streaming(const depz_device *dev);
```

### depz_bno055_on_sample

```c
int depz_bno055_on_sample(depz_device *dev, depz_bno055_sample_cb cb, void *user, int *token);
```

### depz_bno055_off_sample

```c
void depz_bno055_off_sample(depz_device *dev, int token);
```

### depz_bno055_samples

```c
depz_stream *depz_bno055_samples(depz_device *dev, size_t maxsize);
```

### depz_bno055_get_sample

```c
int depz_bno055_get_sample(depz_device *dev, int timeout_ms, depz_bno055_sample *out);
```

The next streamed sample (< 0 timeout = 1000 ms).

### depz_bno055_stream_parse_errors

```c
uint64_t depz_bno055_stream_parse_errors(const depz_device *dev);
```

## BNO085 / BNO086 sensor class (live layer)

### DEPZ_BNO086_BUSY_RETRIES

```c
#define DEPZ_BNO086_BUSY_RETRIES    5
```

SEND_SHTP_PACKET attempts on ERR_BUSY (both MCU TX slots full), and the
back-off between them (contract 05 §2).

### DEPZ_BNO086_BUSY_BACKOFF_MS

```c
#define DEPZ_BNO086_BUSY_BACKOFF_MS 200
```

### DEPZ_BNO086_RATE_LOW_FACTOR

```c
#define DEPZ_BNO086_RATE_LOW_FACTOR  0.9
```

A granted rate outside [0.9, 2.1] x the requested one is worth a warning
(contract 05 §7): the hub rounds to its 1 kHz / 2^n grid.

### DEPZ_BNO086_RATE_HIGH_FACTOR

```c
#define DEPZ_BNO086_RATE_HIGH_FACTOR 2.1
```

### depz_bno086_feature_request

```c
typedef struct {
    uint32_t interval_us;   /* report interval; 0 is refused (use disable) */
    uint32_t batch_us;
    uint16_t sensitivity;   /* change sensitivity, sensor units */
    uint8_t  flags;         /* SH-2 §6.5.4 */
    uint32_t cfg_word;      /* sensor-specific configuration */
} depz_bno086_feature_request;
```

Everything Set Feature (0xFD) carries; zero-initialise and fill.

### depz_bno086_calibration

```c
typedef struct { bool accel, gyro, mag, planar; } depz_bno086_calibration;
```

ME calibration enables as the sensor reports them.

### depz_bno086_error_record

```c
typedef struct { uint8_t severity, seq, source, error, module, code; } depz_bno086_error_record;
```

One error-queue entry (command 0x01).

### depz_bno086_counts

```c
typedef struct {
    uint8_t  sensor_id;
    uint32_t offered, accepted, on, attempted;
} depz_bno086_counts;
```

Per-sensor event counts (command 0x02).

### depz_bno086_report_cb

```c
typedef void (*depz_bno086_report_cb)(const depz_bno_report *r, void *user);
```

Reports reach callbacks and streams as depz_bno_report. `data` (the bytes of
an UNKNOWN_REPORT) is valid inside a callback only; streamed copies hold
NULL there.

### depz_bno086_open_link

```c
int depz_bno086_open_link(depz_link *link, depz_device **out);
```

A BNO085 / BNO086 on a link without an identity probe (tests, replay).

### depz_is_bno086

```c
bool depz_is_bno086(const depz_device *dev);
```

### depz_bno086_hardware_reset

```c
int depz_bno086_hardware_reset(depz_device *dev, int timeout_ms);
```

nRST pulse (bridge 0x32): SHTP sequence counters, partial cargos, the
advertisement and every enabled sensor start from zero. Waits up to 0.5 s
for the executable reset-complete, best effort — older firmware never sends
it (ERRATA E9). < 0 timeout = 2000 ms.

### depz_bno086_wake

```c
int depz_bno086_wake(depz_device *dev);
```

WAKE (PS0) pulse: out of sleep, no state lost.

### depz_bno086_advertisement

```c
int depz_bno086_advertisement(depz_device *dev, uint8_t *buf, size_t cap, size_t *len);
```

SHTP channel-0 advertisement bytes seen since open / reset; *len = full
length (copies at most `cap`).

### depz_bno086_product_id

```c
int depz_bno086_product_id(depz_device *dev, depz_bno_product_id *out);
```

Product ID round trip (the first responding subsystem).

### depz_bno086_enable

```c
int depz_bno086_enable(depz_device *dev, uint8_t sensor, double hz, depz_bno_feature *granted);
```

Enable `sensor` at `hz` (Set Feature), then read the granted rate back
(Get Feature) into *granted (optional). A "disabled" answer is re-read, five
times at most: it is stale (left by a preceding disable()) or early (the hub
reports 0 for a read-back or two before it applies the rate). enable_ex:
`granted` NULL skips the read-back.

### depz_bno086_enable_ex

```c
int depz_bno086_enable_ex(depz_device *dev, uint8_t sensor, const depz_bno086_feature_request *req, depz_bno_feature *granted);
```

### depz_bno086_rate_ok

```c
bool depz_bno086_rate_ok(uint32_t requested_interval_us, const depz_bno_feature *granted);
```

Is `granted` within [0.9, 2.1] x the requested interval's rate?

### depz_bno086_disable

```c
int depz_bno086_disable(depz_device *dev, uint8_t sensor);
```

### depz_bno086_get_feature

```c
int depz_bno086_get_feature(depz_device *dev, uint8_t sensor, depz_bno_feature *out);
```

### depz_bno086_on_report

```c
int depz_bno086_on_report(depz_device *dev, depz_bno086_report_cb cb, void *user, int *token);
```

Reports (channels 3, 4 and 5, timestamps in MCU µs).

### depz_bno086_off_report

```c
void depz_bno086_off_report(depz_device *dev, int token);
```

### depz_bno086_reports

```c
depz_stream *depz_bno086_reports(depz_device *dev, size_t maxsize);
```

Pull stream of depz_bno_report items (0 = 1024).

### depz_bno086_get_report

```c
int depz_bno086_get_report(depz_device *dev, int timeout_ms, depz_bno_report *out);
```

The next report (< 0 timeout = 1000 ms).

### depz_bno086_shtp_discarded

```c
uint64_t depz_bno086_shtp_discarded(const depz_device *dev);
```

Incomplete / orphan SHTP cargos thrown away by the reassembler.

### depz_bno086_tare_now

```c
int depz_bno086_tare_now(depz_device *dev, uint8_t axes, uint8_t basis);
```

Tare and calibration (commands 3, 6, 7, 9). Tare and periodic DCD have no
SH-2 answer: they return once the bridge took the frame.

### depz_bno086_persist_tare

```c
int depz_bno086_persist_tare(depz_device *dev);
```

### depz_bno086_set_reorientation

```c
int depz_bno086_set_reorientation(depz_device *dev, double x, double y, double z, double w);
```

Runtime reorientation quaternion, Q14 on the wire; all zeros clears.

### depz_bno086_set_calibration

```c
int depz_bno086_set_calibration(depz_device *dev, bool accel, bool gyro, bool mag, bool planar);
```

### depz_bno086_get_calibration

```c
int depz_bno086_get_calibration(depz_device *dev, depz_bno086_calibration *out);
```

### depz_bno086_save_dcd

```c
int depz_bno086_save_dcd(depz_device *dev);
```

### depz_bno086_configure_periodic_dcd

```c
int depz_bno086_configure_periodic_dcd(depz_device *dev, bool enable);
```

### depz_bno086_frs_read

```c
int depz_bno086_frs_read(depz_device *dev, uint16_t record, uint32_t *words, size_t cap, size_t *n);
```

FRS records. read: *n = the record's word count (DEPZ_E_ARG after reading
when it exceeds `cap`).

### depz_bno086_frs_write

```c
int depz_bno086_frs_write(depz_device *dev, uint16_t record, const uint32_t *words, size_t n);
```

### depz_bno086_get_metadata

```c
int depz_bno086_get_metadata(depz_device *dev, uint8_t sensor, depz_bno_metadata *out);
```

A sensor's metadata record; DEPZ_E_ARG for a sensor without one.

### depz_bno086_get_oscillator_type

```c
int depz_bno086_get_oscillator_type(depz_device *dev, uint8_t *type);
```

Diagnostics and housekeeping.
/* DEPZ_BNO_OSC_* */

### depz_bno086_clear_dcd_and_reset

```c
int depz_bno086_clear_dcd_and_reset(depz_device *dev, int timeout_ms);
```

Clear the in-RAM dynamic calibration and reset (command 0x0B); waits for the
executable reset-complete (< 0 timeout = 2000 ms).

### depz_bno086_get_errors

```c
int depz_bno086_get_errors(depz_device *dev, uint8_t severity, depz_bno086_error_record *out, size_t cap, size_t *n);
```

The error queue, `severity` or worse (0 = all); *n = entries returned.

### depz_bno086_get_counts

```c
int depz_bno086_get_counts(depz_device *dev, uint8_t sensor, depz_bno086_counts *out);
```

### depz_bno086_clear_counts

```c
int depz_bno086_clear_counts(depz_device *dev, uint8_t sensor);
```

### depz_bno086_command

```c
int depz_bno086_command(depz_device *dev, uint8_t command, const uint8_t *params, size_t n, depz_bno_command_response *resp);
```

Escape hatches: a Command Request (`params` up to 9 bytes; *resp NULL = do
not wait for an answer), and a raw SHTP cargo on `channel`.

### depz_bno086_send_shtp

```c
int depz_bno086_send_shtp(depz_device *dev, uint8_t channel, const uint8_t *payload, size_t len);
```
