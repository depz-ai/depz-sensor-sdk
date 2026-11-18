/**
 * uld/bare/regs — the VL53LX register blocks, as ST's BareDriver sees them.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.regs`.
 *
 * The BareDriver does not touch registers one at a time. It keeps the device
 * image in a handful of C structs (`VL53LX_static_config_t`, ...), each mapping
 * onto one contiguous I2C block, and moves a whole block at a time. `BLOCKS`
 * is that mapping: base address, byte length, whether the block is writable,
 * and the field list with offset, width, signedness and the bit mask the C
 * codec applies. Generated from the Python table (itself generated from
 * `vl53lx_register_funcs.c`) — data, not hand-written: the masks and the
 * reserved holes (general_config has one at offset 10) match the C driver.
 */

/** One register field: name, byte offset, byte width, signed, C codec mask. */
export interface RegField {
  readonly name: string;
  readonly off: number;
  readonly width: number;
  readonly signed: boolean;
  readonly mask: number | null;
}

export interface Block {
  readonly base: number;
  readonly size: number;
  readonly writable: boolean;
  readonly fields: readonly RegField[];
}

function F(name: string, off: number, width: number, signed: boolean, mask: number | null): RegField {
  return { name, off, width, signed, mask };
}

export type BlockName =
  | "static_nvm_managed"
  | "customer_nvm_managed"
  | "static_config"
  | "general_config"
  | "timing_config"
  | "dynamic_config"
  | "system_control"
  | "system_results"
  | "core_results"
  | "debug_results"
  | "nvm_copy_data"
  | "patch_debug"
  | "gph_general_config"
  | "gph_static_config"
  | "gph_timing_config"
  | "fw_internal";

export const BLOCK_NAMES: readonly BlockName[] = [
  "static_nvm_managed",
  "customer_nvm_managed",
  "static_config",
  "general_config",
  "timing_config",
  "dynamic_config",
  "system_control",
  "system_results",
  "core_results",
  "debug_results",
  "nvm_copy_data",
  "patch_debug",
  "gph_general_config",
  "gph_static_config",
  "gph_timing_config",
  "fw_internal",
];

export const BLOCKS: Readonly<Record<BlockName, Block>> = {
  static_nvm_managed: {
    base: 0x0001,
    size: 11,
    writable: true,
    fields: [
      F("i2c_slave__device_address", 0, 1, false, 0x7F),
      F("ana_config__vhv_ref_sel_vddpix", 1, 1, false, 0xF),
      F("ana_config__vhv_ref_sel_vquench", 2, 1, false, 0x7F),
      F("ana_config__reg_avdd1v2_sel", 3, 1, false, 0x3),
      F("ana_config__fast_osc__trim", 4, 1, false, 0x7F),
      F("osc_measured__fast_osc__frequency", 5, 2, false, null),
      F("vhv_config__timeout_macrop_loop_bound", 7, 1, false, null),
      F("vhv_config__count_thresh", 8, 1, false, null),
      F("vhv_config__offset", 9, 1, false, 0x3F),
      F("vhv_config__init", 10, 1, false, null),
    ],
  },
  customer_nvm_managed: {
    base: 0x000D,
    size: 23,
    writable: true,
    fields: [
      F("global_config__spad_enables_ref_0", 0, 1, false, null),
      F("global_config__spad_enables_ref_1", 1, 1, false, null),
      F("global_config__spad_enables_ref_2", 2, 1, false, null),
      F("global_config__spad_enables_ref_3", 3, 1, false, null),
      F("global_config__spad_enables_ref_4", 4, 1, false, null),
      F("global_config__spad_enables_ref_5", 5, 1, false, 0xF),
      F("global_config__ref_en_start_select", 6, 1, false, null),
      F("ref_spad_man__num_requested_ref_spads", 7, 1, false, 0x3F),
      F("ref_spad_man__ref_location", 8, 1, false, 0x3),
      F("algo__crosstalk_compensation_plane_offset_kcps", 9, 2, false, null),
      F("algo__crosstalk_compensation_x_plane_gradient_kcps", 11, 2, true, null),
      F("algo__crosstalk_compensation_y_plane_gradient_kcps", 13, 2, true, null),
      F("ref_spad_char__total_rate_target_mcps", 15, 2, false, null),
      F("algo__part_to_part_range_offset_mm", 17, 2, true, 0x1FFF),
      F("mm_config__inner_offset_mm", 19, 2, true, null),
      F("mm_config__outer_offset_mm", 21, 2, true, null),
    ],
  },
  static_config: {
    base: 0x0024,
    size: 32,
    writable: true,
    fields: [
      F("dss_config__target_total_rate_mcps", 0, 2, false, null),
      F("debug__ctrl", 2, 1, false, 0x1),
      F("test_mode__ctrl", 3, 1, false, 0xF),
      F("clk_gating__ctrl", 4, 1, false, 0xF),
      F("nvm_bist__ctrl", 5, 1, false, 0x1F),
      F("nvm_bist__num_nvm_words", 6, 1, false, 0x7F),
      F("nvm_bist__start_address", 7, 1, false, 0x7F),
      F("host_if__status", 8, 1, false, 0x1),
      F("pad_i2c_hv__config", 9, 1, false, null),
      F("pad_i2c_hv__extsup_config", 10, 1, false, 0x1),
      F("gpio_hv_pad__ctrl", 11, 1, false, 0x3),
      F("gpio_hv_mux__ctrl", 12, 1, false, 0x1F),
      F("gpio__tio_hv_status", 13, 1, false, 0x3),
      F("gpio__fio_hv_status", 14, 1, false, 0x3),
      F("ana_config__spad_sel_pswidth", 15, 1, false, 0x7),
      F("ana_config__vcsel_pulse_width_offset", 16, 1, false, 0x1F),
      F("ana_config__fast_osc__config_ctrl", 17, 1, false, 0x1),
      F("sigma_estimator__effective_pulse_width_ns", 18, 1, false, null),
      F("sigma_estimator__effective_ambient_width_ns", 19, 1, false, null),
      F("sigma_estimator__sigma_ref_mm", 20, 1, false, null),
      F("algo__crosstalk_compensation_valid_height_mm", 21, 1, false, null),
      F("spare_host_config__static_config_spare_0", 22, 1, false, null),
      F("spare_host_config__static_config_spare_1", 23, 1, false, null),
      F("algo__range_ignore_threshold_mcps", 24, 2, false, null),
      F("algo__range_ignore_valid_height_mm", 26, 1, false, null),
      F("algo__range_min_clip", 27, 1, false, null),
      F("algo__consistency_check__tolerance", 28, 1, false, 0xF),
      F("spare_host_config__static_config_spare_2", 29, 1, false, null),
      F("sd_config__reset_stages_msb", 30, 1, false, 0xF),
      F("sd_config__reset_stages_lsb", 31, 1, false, null),
    ],
  },
  general_config: {
    base: 0x0044,
    size: 22,
    writable: true,
    fields: [
      F("gph_config__stream_count_update_value", 0, 1, false, null),
      F("global_config__stream_divider", 1, 1, false, null),
      F("system__interrupt_config_gpio", 2, 1, false, null),
      F("cal_config__vcsel_start", 3, 1, false, 0x7F),
      F("cal_config__repeat_rate", 4, 2, false, 0xFFF),
      F("global_config__vcsel_width", 6, 1, false, 0x7F),
      F("phasecal_config__timeout_macrop", 7, 1, false, null),
      F("phasecal_config__target", 8, 1, false, null),
      F("phasecal_config__override", 9, 1, false, 0x1),
      F("dss_config__roi_mode_control", 11, 1, false, 0x7),
      F("system__thresh_rate_high", 12, 2, false, null),
      F("system__thresh_rate_low", 14, 2, false, null),
      F("dss_config__manual_effective_spads_select", 16, 2, false, null),
      F("dss_config__manual_block_select", 18, 1, false, null),
      F("dss_config__aperture_attenuation", 19, 1, false, null),
      F("dss_config__max_spads_limit", 20, 1, false, null),
      F("dss_config__min_spads_limit", 21, 1, false, null),
    ],
  },
  timing_config: {
    base: 0x005A,
    size: 23,
    writable: true,
    fields: [
      F("mm_config__timeout_macrop_a_hi", 0, 1, false, 0xF),
      F("mm_config__timeout_macrop_a_lo", 1, 1, false, null),
      F("mm_config__timeout_macrop_b_hi", 2, 1, false, 0xF),
      F("mm_config__timeout_macrop_b_lo", 3, 1, false, null),
      F("range_config__timeout_macrop_a_hi", 4, 1, false, 0xF),
      F("range_config__timeout_macrop_a_lo", 5, 1, false, null),
      F("range_config__vcsel_period_a", 6, 1, false, 0x3F),
      F("range_config__timeout_macrop_b_hi", 7, 1, false, 0xF),
      F("range_config__timeout_macrop_b_lo", 8, 1, false, null),
      F("range_config__vcsel_period_b", 9, 1, false, 0x3F),
      F("range_config__sigma_thresh", 10, 2, false, null),
      F("range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, null),
      F("range_config__valid_phase_low", 14, 1, false, null),
      F("range_config__valid_phase_high", 15, 1, false, null),
      F("system__intermeasurement_period", 18, 4, false, null),
      F("system__fractional_enable", 22, 1, false, 0x1),
    ],
  },
  dynamic_config: {
    base: 0x0071,
    size: 18,
    writable: true,
    fields: [
      F("system__grouped_parameter_hold_0", 0, 1, false, 0x3),
      F("system__thresh_high", 1, 2, false, null),
      F("system__thresh_low", 3, 2, false, null),
      F("system__enable_xtalk_per_quadrant", 5, 1, false, 0x1),
      F("system__seed_config", 6, 1, false, 0x7),
      F("sd_config__woi_sd0", 7, 1, false, null),
      F("sd_config__woi_sd1", 8, 1, false, null),
      F("sd_config__initial_phase_sd0", 9, 1, false, 0x7F),
      F("sd_config__initial_phase_sd1", 10, 1, false, 0x7F),
      F("system__grouped_parameter_hold_1", 11, 1, false, 0x3),
      F("sd_config__first_order_select", 12, 1, false, 0x3),
      F("sd_config__quantifier", 13, 1, false, 0xF),
      F("roi_config__user_roi_centre_spad", 14, 1, false, null),
      F("roi_config__user_roi_requested_global_xy_size", 15, 1, false, null),
      F("system__sequence_config", 16, 1, false, null),
      F("system__grouped_parameter_hold", 17, 1, false, 0x3),
    ],
  },
  system_control: {
    base: 0x0083,
    size: 5,
    writable: true,
    fields: [
      F("power_management__go1_power_force", 0, 1, false, 0x1),
      F("system__stream_count_ctrl", 1, 1, false, 0x1),
      F("firmware__enable", 2, 1, false, 0x1),
      F("system__interrupt_clear", 3, 1, false, 0x3),
      F("system__mode_start", 4, 1, false, null),
    ],
  },
  system_results: {
    base: 0x0088,
    size: 44,
    writable: true,
    fields: [
      F("result__interrupt_status", 0, 1, false, 0x3F),
      F("result__range_status", 1, 1, false, null),
      F("result__report_status", 2, 1, false, 0xF),
      F("result__stream_count", 3, 1, false, null),
      F("result__dss_actual_effective_spads_sd0", 4, 2, false, null),
      F("result__peak_signal_count_rate_mcps_sd0", 6, 2, false, null),
      F("result__ambient_count_rate_mcps_sd0", 8, 2, false, null),
      F("result__sigma_sd0", 10, 2, false, null),
      F("result__phase_sd0", 12, 2, false, null),
      F("result__final_crosstalk_corrected_range_mm_sd0", 14, 2, false, null),
      F("result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0", 16, 2, false, null),
      F("result__mm_inner_actual_effective_spads_sd0", 18, 2, false, null),
      F("result__mm_outer_actual_effective_spads_sd0", 20, 2, false, null),
      F("result__avg_signal_count_rate_mcps_sd0", 22, 2, false, null),
      F("result__dss_actual_effective_spads_sd1", 24, 2, false, null),
      F("result__peak_signal_count_rate_mcps_sd1", 26, 2, false, null),
      F("result__ambient_count_rate_mcps_sd1", 28, 2, false, null),
      F("result__sigma_sd1", 30, 2, false, null),
      F("result__phase_sd1", 32, 2, false, null),
      F("result__final_crosstalk_corrected_range_mm_sd1", 34, 2, false, null),
      F("result__spare_0_sd1", 36, 2, false, null),
      F("result__spare_1_sd1", 38, 2, false, null),
      F("result__spare_2_sd1", 40, 2, false, null),
      F("result__spare_3_sd1", 42, 1, false, null),
      F("result__thresh_info", 43, 1, false, null),
    ],
  },
  core_results: {
    base: 0x00B4,
    size: 33,
    writable: true,
    fields: [
      F("result_core__ambient_window_events_sd0", 0, 4, false, null),
      F("result_core__ranging_total_events_sd0", 4, 4, false, null),
      F("result_core__signal_total_events_sd0", 8, 4, true, null),
      F("result_core__total_periods_elapsed_sd0", 12, 4, false, null),
      F("result_core__ambient_window_events_sd1", 16, 4, false, null),
      F("result_core__ranging_total_events_sd1", 20, 4, false, null),
      F("result_core__signal_total_events_sd1", 24, 4, true, null),
      F("result_core__total_periods_elapsed_sd1", 28, 4, false, null),
      F("result_core__spare_0", 32, 1, false, null),
    ],
  },
  debug_results: {
    base: 0x00D6,
    size: 56,
    writable: true,
    fields: [
      F("phasecal_result__reference_phase", 0, 2, false, null),
      F("phasecal_result__vcsel_start", 2, 1, false, 0x7F),
      F("ref_spad_char_result__num_actual_ref_spads", 3, 1, false, 0x3F),
      F("ref_spad_char_result__ref_location", 4, 1, false, 0x3),
      F("vhv_result__coldboot_status", 5, 1, false, 0x1),
      F("vhv_result__search_result", 6, 1, false, 0x3F),
      F("vhv_result__latest_setting", 7, 1, false, 0x3F),
      F("result__osc_calibrate_val", 8, 2, false, 0x3FF),
      F("ana_config__powerdown_go1", 10, 1, false, 0x3),
      F("ana_config__ref_bg_ctrl", 11, 1, false, 0x3),
      F("ana_config__regdvdd1v2_ctrl", 12, 1, false, 0xF),
      F("ana_config__osc_slow_ctrl", 13, 1, false, 0x7),
      F("test_mode__status", 14, 1, false, 0x1),
      F("firmware__system_status", 15, 1, false, 0x3),
      F("firmware__mode_status", 16, 1, false, null),
      F("firmware__secondary_mode_status", 17, 1, false, null),
      F("firmware__cal_repeat_rate_counter", 18, 2, false, 0xFFF),
      F("gph__system__thresh_high", 22, 2, false, null),
      F("gph__system__thresh_low", 24, 2, false, null),
      F("gph__system__enable_xtalk_per_quadrant", 26, 1, false, 0x1),
      F("gph__spare_0", 27, 1, false, 0x7),
      F("gph__sd_config__woi_sd0", 28, 1, false, null),
      F("gph__sd_config__woi_sd1", 29, 1, false, null),
      F("gph__sd_config__initial_phase_sd0", 30, 1, false, 0x7F),
      F("gph__sd_config__initial_phase_sd1", 31, 1, false, 0x7F),
      F("gph__sd_config__first_order_select", 32, 1, false, 0x3),
      F("gph__sd_config__quantifier", 33, 1, false, 0xF),
      F("gph__roi_config__user_roi_centre_spad", 34, 1, false, null),
      F("gph__roi_config__user_roi_requested_global_xy_size", 35, 1, false, null),
      F("gph__system__sequence_config", 36, 1, false, null),
      F("gph__gph_id", 37, 1, false, 0x1),
      F("system__interrupt_set", 38, 1, false, 0x3),
      F("interrupt_manager__enables", 39, 1, false, 0x1F),
      F("interrupt_manager__clear", 40, 1, false, 0x1F),
      F("interrupt_manager__status", 41, 1, false, 0x1F),
      F("mcu_to_host_bank__wr_access_en", 42, 1, false, 0x1),
      F("power_management__go1_reset_status", 43, 1, false, 0x1),
      F("pad_startup_mode__value_ro", 44, 1, false, 0x3),
      F("pad_startup_mode__value_ctrl", 45, 1, false, 0x3F),
      F("pll_period_us", 46, 4, false, 0x3FFFF),
      F("interrupt_scheduler__data_out", 50, 4, false, null),
      F("nvm_bist__complete", 54, 1, false, 0x1),
      F("nvm_bist__status", 55, 1, false, 0x1),
    ],
  },
  nvm_copy_data: {
    base: 0x010F,
    size: 49,
    writable: true,
    fields: [
      F("identification__model_id", 0, 1, false, null),
      F("identification__module_type", 1, 1, false, null),
      F("identification__revision_id", 2, 1, false, null),
      F("identification__module_id", 3, 2, false, null),
      F("ana_config__fast_osc__trim_max", 5, 1, false, 0x7F),
      F("ana_config__fast_osc__freq_set", 6, 1, false, 0x7),
      F("ana_config__vcsel_trim", 7, 1, false, 0x7),
      F("ana_config__vcsel_selion", 8, 1, false, 0x3F),
      F("ana_config__vcsel_selion_max", 9, 1, false, 0x3F),
      F("protected_laser_safety__lock_bit", 10, 1, false, 0x1),
      F("laser_safety__key", 11, 1, false, 0x7F),
      F("laser_safety__key_ro", 12, 1, false, 0x1),
      F("laser_safety__clip", 13, 1, false, 0x3F),
      F("laser_safety__mult", 14, 1, false, 0x3F),
      F("global_config__spad_enables_rtn_0", 15, 1, false, null),
      F("global_config__spad_enables_rtn_1", 16, 1, false, null),
      F("global_config__spad_enables_rtn_2", 17, 1, false, null),
      F("global_config__spad_enables_rtn_3", 18, 1, false, null),
      F("global_config__spad_enables_rtn_4", 19, 1, false, null),
      F("global_config__spad_enables_rtn_5", 20, 1, false, null),
      F("global_config__spad_enables_rtn_6", 21, 1, false, null),
      F("global_config__spad_enables_rtn_7", 22, 1, false, null),
      F("global_config__spad_enables_rtn_8", 23, 1, false, null),
      F("global_config__spad_enables_rtn_9", 24, 1, false, null),
      F("global_config__spad_enables_rtn_10", 25, 1, false, null),
      F("global_config__spad_enables_rtn_11", 26, 1, false, null),
      F("global_config__spad_enables_rtn_12", 27, 1, false, null),
      F("global_config__spad_enables_rtn_13", 28, 1, false, null),
      F("global_config__spad_enables_rtn_14", 29, 1, false, null),
      F("global_config__spad_enables_rtn_15", 30, 1, false, null),
      F("global_config__spad_enables_rtn_16", 31, 1, false, null),
      F("global_config__spad_enables_rtn_17", 32, 1, false, null),
      F("global_config__spad_enables_rtn_18", 33, 1, false, null),
      F("global_config__spad_enables_rtn_19", 34, 1, false, null),
      F("global_config__spad_enables_rtn_20", 35, 1, false, null),
      F("global_config__spad_enables_rtn_21", 36, 1, false, null),
      F("global_config__spad_enables_rtn_22", 37, 1, false, null),
      F("global_config__spad_enables_rtn_23", 38, 1, false, null),
      F("global_config__spad_enables_rtn_24", 39, 1, false, null),
      F("global_config__spad_enables_rtn_25", 40, 1, false, null),
      F("global_config__spad_enables_rtn_26", 41, 1, false, null),
      F("global_config__spad_enables_rtn_27", 42, 1, false, null),
      F("global_config__spad_enables_rtn_28", 43, 1, false, null),
      F("global_config__spad_enables_rtn_29", 44, 1, false, null),
      F("global_config__spad_enables_rtn_30", 45, 1, false, null),
      F("global_config__spad_enables_rtn_31", 46, 1, false, null),
      F("roi_config__mode_roi_centre_spad", 47, 1, false, null),
      F("roi_config__mode_roi_xy_size", 48, 1, false, null),
    ],
  },
  patch_debug: {
    base: 0x0F20,
    size: 2,
    writable: true,
    fields: [
      F("result__debug_status", 0, 1, false, null),
      F("result__debug_stage", 1, 1, false, null),
    ],
  },
  gph_general_config: {
    base: 0x0F24,
    size: 5,
    writable: true,
    fields: [
      F("gph__system__thresh_rate_high", 0, 2, false, null),
      F("gph__system__thresh_rate_low", 2, 2, false, null),
      F("gph__system__interrupt_config_gpio", 4, 1, false, null),
    ],
  },
  gph_static_config: {
    base: 0x0F2F,
    size: 6,
    writable: true,
    fields: [
      F("gph__dss_config__roi_mode_control", 0, 1, false, 0x7),
      F("gph__dss_config__manual_effective_spads_select", 1, 2, false, null),
      F("gph__dss_config__manual_block_select", 3, 1, false, null),
      F("gph__dss_config__max_spads_limit", 4, 1, false, null),
      F("gph__dss_config__min_spads_limit", 5, 1, false, null),
    ],
  },
  gph_timing_config: {
    base: 0x0F36,
    size: 16,
    writable: true,
    fields: [
      F("gph__mm_config__timeout_macrop_a_hi", 0, 1, false, 0xF),
      F("gph__mm_config__timeout_macrop_a_lo", 1, 1, false, null),
      F("gph__mm_config__timeout_macrop_b_hi", 2, 1, false, 0xF),
      F("gph__mm_config__timeout_macrop_b_lo", 3, 1, false, null),
      F("gph__range_config__timeout_macrop_a_hi", 4, 1, false, 0xF),
      F("gph__range_config__timeout_macrop_a_lo", 5, 1, false, null),
      F("gph__range_config__vcsel_period_a", 6, 1, false, 0x3F),
      F("gph__range_config__vcsel_period_b", 7, 1, false, 0x3F),
      F("gph__range_config__timeout_macrop_b_hi", 8, 1, false, 0xF),
      F("gph__range_config__timeout_macrop_b_lo", 9, 1, false, null),
      F("gph__range_config__sigma_thresh", 10, 2, false, null),
      F("gph__range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, null),
      F("gph__range_config__valid_phase_low", 14, 1, false, null),
      F("gph__range_config__valid_phase_high", 15, 1, false, null),
    ],
  },
  fw_internal: {
    base: 0x0F46,
    size: 2,
    writable: true,
    fields: [
      F("firmware__internal_stream_count_div", 0, 1, false, null),
      F("firmware__internal_stream_counter_val", 1, 1, false, null),
    ],
  },
};

/** Python `x % m` for a positive modulus (result in [0, m)). */
function mod(x: number, m: number): number {
  const r = x % m;
  return r < 0 ? r + m : r;
}

/** Apply the field's mask and width, the way the C codec does (Python `_clip`). */
function clip(value: number, f: RegField): number {
  // Python: int(value); a signed field is taken mod 2^bits (two's complement),
  // then the mask, then the width. An unsigned field's `&` on a negative
  // Python int is the same mod-2^bits view, so one path serves both.
  let v = mod(Math.trunc(value), 2 ** (f.width * 8));
  if (f.mask !== null) v = (v & f.mask) >>> 0;
  return v;
}

function unsign(value: number, f: RegField): number {
  const bits = f.width * 8;
  if (f.signed && value >= 2 ** (bits - 1)) return value - 2 ** bits;
  return value;
}

function readBe(raw: Uint8Array, off: number, width: number): number {
  let v = 0;
  for (let i = 0; i < width; i++) v = v * 256 + raw[off + i]!;
  return v;
}

/**
 * One register block's worth of device image, addressed by field name.
 *
 * `v` holds the field values exactly as the Python `RegBlock` does: a write
 * stores the value as given (unclipped — Python keeps `int(value)`), a decode
 * stores it masked and sign-extended; the mask/width are applied only on
 * `encode()`. `v` is sealed, so writing a field the block does not have
 * throws (ES modules are strict mode), as Python's `AttributeError` does.
 * Fields not named in the block (the reserved holes) keep whatever bytes were
 * read, so a read-modify-write round trip is byte-exact.
 */
export class RegBlock {
  readonly block: Block;
  readonly v: Record<string, number>;
  private raw: Uint8Array;

  constructor(readonly name: BlockName) {
    this.block = BLOCKS[name];
    const v: Record<string, number> = {};
    for (const f of this.block.fields) v[f.name] = 0;
    this.v = Object.seal(v);
    this.raw = new Uint8Array(this.block.size);
  }

  decode(raw: Uint8Array): this {
    if (raw.length < this.block.size) {
      throw new RangeError(`${this.name}: ${raw.length} bytes, need ${this.block.size}`);
    }
    this.raw = raw.slice(0, this.block.size);
    for (const f of this.block.fields) {
      let value = readBe(raw, f.off, f.width);
      if (f.mask !== null) value = (value & f.mask) >>> 0;
      this.v[f.name] = unsign(value, f);
    }
    return this;
  }

  encode(): Uint8Array {
    const buf = this.raw.slice();
    for (const f of this.block.fields) {
      let x = clip(this.v[f.name]!, f);
      for (let i = f.width - 1; i >= 0; i--) {
        buf[f.off + i] = x % 256;
        x = Math.floor(x / 256);
      }
    }
    return buf;
  }

  get base(): number {
    return this.block.base;
  }

  get size(): number {
    return this.block.size;
  }

  items(): Array<[string, number]> {
    return this.block.fields.map((f) => [f.name, this.v[f.name]!]);
  }
}
