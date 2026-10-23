// Generated from the C# port's BareTables.g.cs (packages/depz-sensor-sdk-csharp/src/Depz.Sensor/Vl53lx/Bare),
// itself generated from the Python reference (vl53lx/uld/bare/regs.py and tuning.py, themselves generated
// from ST's VL53LX BareDriver). Data only - edit the reference, then regenerate.
package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** The register-block layout and the tuning parameters of the VL53LX BareDriver (generated data). */
public final class BareTables {
    private BareTables() {}

    /**
     * The register blocks of the device image: base, size, writable and the fields with offset,
     * width, signedness and the mask the C codec applies (null: none).
     */
    public static final Map<String, RegBlockDef> BLOCKS = Collections.unmodifiableMap(blocks());

    /** {@code VL53LX_tuning_parm_storage_t} after initialisation, keyed by the C field name. */
    public static final Map<String, Long> TUNING = Collections.unmodifiableMap(tuning());

    /** The raw tuning header, for the parameters the post-processing reads without a {@code tp_} field. */
    public static final Map<String, Long> DEFAULTS = Collections.unmodifiableMap(defaults());

    private static RegField f(String name, int off, int width, boolean signed, Long mask) {
        return new RegField(name, off, width, signed, mask);
    }

    private static Map<String, RegBlockDef> blocks() {
        Map<String, RegBlockDef> m = new LinkedHashMap<>();
        m.put("static_nvm_managed", new RegBlockDef(0x0001, 11, true, List.of(
                f("i2c_slave__device_address", 0, 1, false, 0x7FL),
                f("ana_config__vhv_ref_sel_vddpix", 1, 1, false, 0xFL),
                f("ana_config__vhv_ref_sel_vquench", 2, 1, false, 0x7FL),
                f("ana_config__reg_avdd1v2_sel", 3, 1, false, 0x3L),
                f("ana_config__fast_osc__trim", 4, 1, false, 0x7FL),
                f("osc_measured__fast_osc__frequency", 5, 2, false, null),
                f("vhv_config__timeout_macrop_loop_bound", 7, 1, false, null),
                f("vhv_config__count_thresh", 8, 1, false, null),
                f("vhv_config__offset", 9, 1, false, 0x3FL),
                f("vhv_config__init", 10, 1, false, null))));
        m.put("customer_nvm_managed", new RegBlockDef(0x000D, 23, true, List.of(
                f("global_config__spad_enables_ref_0", 0, 1, false, null),
                f("global_config__spad_enables_ref_1", 1, 1, false, null),
                f("global_config__spad_enables_ref_2", 2, 1, false, null),
                f("global_config__spad_enables_ref_3", 3, 1, false, null),
                f("global_config__spad_enables_ref_4", 4, 1, false, null),
                f("global_config__spad_enables_ref_5", 5, 1, false, 0xFL),
                f("global_config__ref_en_start_select", 6, 1, false, null),
                f("ref_spad_man__num_requested_ref_spads", 7, 1, false, 0x3FL),
                f("ref_spad_man__ref_location", 8, 1, false, 0x3L),
                f("algo__crosstalk_compensation_plane_offset_kcps", 9, 2, false, null),
                f("algo__crosstalk_compensation_x_plane_gradient_kcps", 11, 2, true, null),
                f("algo__crosstalk_compensation_y_plane_gradient_kcps", 13, 2, true, null),
                f("ref_spad_char__total_rate_target_mcps", 15, 2, false, null),
                f("algo__part_to_part_range_offset_mm", 17, 2, true, 0x1FFFL),
                f("mm_config__inner_offset_mm", 19, 2, true, null),
                f("mm_config__outer_offset_mm", 21, 2, true, null))));
        m.put("static_config", new RegBlockDef(0x0024, 32, true, List.of(
                f("dss_config__target_total_rate_mcps", 0, 2, false, null),
                f("debug__ctrl", 2, 1, false, 0x1L),
                f("test_mode__ctrl", 3, 1, false, 0xFL),
                f("clk_gating__ctrl", 4, 1, false, 0xFL),
                f("nvm_bist__ctrl", 5, 1, false, 0x1FL),
                f("nvm_bist__num_nvm_words", 6, 1, false, 0x7FL),
                f("nvm_bist__start_address", 7, 1, false, 0x7FL),
                f("host_if__status", 8, 1, false, 0x1L),
                f("pad_i2c_hv__config", 9, 1, false, null),
                f("pad_i2c_hv__extsup_config", 10, 1, false, 0x1L),
                f("gpio_hv_pad__ctrl", 11, 1, false, 0x3L),
                f("gpio_hv_mux__ctrl", 12, 1, false, 0x1FL),
                f("gpio__tio_hv_status", 13, 1, false, 0x3L),
                f("gpio__fio_hv_status", 14, 1, false, 0x3L),
                f("ana_config__spad_sel_pswidth", 15, 1, false, 0x7L),
                f("ana_config__vcsel_pulse_width_offset", 16, 1, false, 0x1FL),
                f("ana_config__fast_osc__config_ctrl", 17, 1, false, 0x1L),
                f("sigma_estimator__effective_pulse_width_ns", 18, 1, false, null),
                f("sigma_estimator__effective_ambient_width_ns", 19, 1, false, null),
                f("sigma_estimator__sigma_ref_mm", 20, 1, false, null),
                f("algo__crosstalk_compensation_valid_height_mm", 21, 1, false, null),
                f("spare_host_config__static_config_spare_0", 22, 1, false, null),
                f("spare_host_config__static_config_spare_1", 23, 1, false, null),
                f("algo__range_ignore_threshold_mcps", 24, 2, false, null),
                f("algo__range_ignore_valid_height_mm", 26, 1, false, null),
                f("algo__range_min_clip", 27, 1, false, null),
                f("algo__consistency_check__tolerance", 28, 1, false, 0xFL),
                f("spare_host_config__static_config_spare_2", 29, 1, false, null),
                f("sd_config__reset_stages_msb", 30, 1, false, 0xFL),
                f("sd_config__reset_stages_lsb", 31, 1, false, null))));
        m.put("general_config", new RegBlockDef(0x0044, 22, true, List.of(
                f("gph_config__stream_count_update_value", 0, 1, false, null),
                f("global_config__stream_divider", 1, 1, false, null),
                f("system__interrupt_config_gpio", 2, 1, false, null),
                f("cal_config__vcsel_start", 3, 1, false, 0x7FL),
                f("cal_config__repeat_rate", 4, 2, false, 0xFFFL),
                f("global_config__vcsel_width", 6, 1, false, 0x7FL),
                f("phasecal_config__timeout_macrop", 7, 1, false, null),
                f("phasecal_config__target", 8, 1, false, null),
                f("phasecal_config__override", 9, 1, false, 0x1L),
                f("dss_config__roi_mode_control", 11, 1, false, 0x7L),
                f("system__thresh_rate_high", 12, 2, false, null),
                f("system__thresh_rate_low", 14, 2, false, null),
                f("dss_config__manual_effective_spads_select", 16, 2, false, null),
                f("dss_config__manual_block_select", 18, 1, false, null),
                f("dss_config__aperture_attenuation", 19, 1, false, null),
                f("dss_config__max_spads_limit", 20, 1, false, null),
                f("dss_config__min_spads_limit", 21, 1, false, null))));
        m.put("timing_config", new RegBlockDef(0x005A, 23, true, List.of(
                f("mm_config__timeout_macrop_a_hi", 0, 1, false, 0xFL),
                f("mm_config__timeout_macrop_a_lo", 1, 1, false, null),
                f("mm_config__timeout_macrop_b_hi", 2, 1, false, 0xFL),
                f("mm_config__timeout_macrop_b_lo", 3, 1, false, null),
                f("range_config__timeout_macrop_a_hi", 4, 1, false, 0xFL),
                f("range_config__timeout_macrop_a_lo", 5, 1, false, null),
                f("range_config__vcsel_period_a", 6, 1, false, 0x3FL),
                f("range_config__timeout_macrop_b_hi", 7, 1, false, 0xFL),
                f("range_config__timeout_macrop_b_lo", 8, 1, false, null),
                f("range_config__vcsel_period_b", 9, 1, false, 0x3FL),
                f("range_config__sigma_thresh", 10, 2, false, null),
                f("range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, null),
                f("range_config__valid_phase_low", 14, 1, false, null),
                f("range_config__valid_phase_high", 15, 1, false, null),
                f("system__intermeasurement_period", 18, 4, false, null),
                f("system__fractional_enable", 22, 1, false, 0x1L))));
        m.put("dynamic_config", new RegBlockDef(0x0071, 18, true, List.of(
                f("system__grouped_parameter_hold_0", 0, 1, false, 0x3L),
                f("system__thresh_high", 1, 2, false, null),
                f("system__thresh_low", 3, 2, false, null),
                f("system__enable_xtalk_per_quadrant", 5, 1, false, 0x1L),
                f("system__seed_config", 6, 1, false, 0x7L),
                f("sd_config__woi_sd0", 7, 1, false, null),
                f("sd_config__woi_sd1", 8, 1, false, null),
                f("sd_config__initial_phase_sd0", 9, 1, false, 0x7FL),
                f("sd_config__initial_phase_sd1", 10, 1, false, 0x7FL),
                f("system__grouped_parameter_hold_1", 11, 1, false, 0x3L),
                f("sd_config__first_order_select", 12, 1, false, 0x3L),
                f("sd_config__quantifier", 13, 1, false, 0xFL),
                f("roi_config__user_roi_centre_spad", 14, 1, false, null),
                f("roi_config__user_roi_requested_global_xy_size", 15, 1, false, null),
                f("system__sequence_config", 16, 1, false, null),
                f("system__grouped_parameter_hold", 17, 1, false, 0x3L))));
        m.put("system_control", new RegBlockDef(0x0083, 5, true, List.of(
                f("power_management__go1_power_force", 0, 1, false, 0x1L),
                f("system__stream_count_ctrl", 1, 1, false, 0x1L),
                f("firmware__enable", 2, 1, false, 0x1L),
                f("system__interrupt_clear", 3, 1, false, 0x3L),
                f("system__mode_start", 4, 1, false, null))));
        m.put("system_results", new RegBlockDef(0x0088, 44, true, List.of(
                f("result__interrupt_status", 0, 1, false, 0x3FL),
                f("result__range_status", 1, 1, false, null),
                f("result__report_status", 2, 1, false, 0xFL),
                f("result__stream_count", 3, 1, false, null),
                f("result__dss_actual_effective_spads_sd0", 4, 2, false, null),
                f("result__peak_signal_count_rate_mcps_sd0", 6, 2, false, null),
                f("result__ambient_count_rate_mcps_sd0", 8, 2, false, null),
                f("result__sigma_sd0", 10, 2, false, null),
                f("result__phase_sd0", 12, 2, false, null),
                f("result__final_crosstalk_corrected_range_mm_sd0", 14, 2, false, null),
                f("result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0", 16, 2, false, null),
                f("result__mm_inner_actual_effective_spads_sd0", 18, 2, false, null),
                f("result__mm_outer_actual_effective_spads_sd0", 20, 2, false, null),
                f("result__avg_signal_count_rate_mcps_sd0", 22, 2, false, null),
                f("result__dss_actual_effective_spads_sd1", 24, 2, false, null),
                f("result__peak_signal_count_rate_mcps_sd1", 26, 2, false, null),
                f("result__ambient_count_rate_mcps_sd1", 28, 2, false, null),
                f("result__sigma_sd1", 30, 2, false, null),
                f("result__phase_sd1", 32, 2, false, null),
                f("result__final_crosstalk_corrected_range_mm_sd1", 34, 2, false, null),
                f("result__spare_0_sd1", 36, 2, false, null),
                f("result__spare_1_sd1", 38, 2, false, null),
                f("result__spare_2_sd1", 40, 2, false, null),
                f("result__spare_3_sd1", 42, 1, false, null),
                f("result__thresh_info", 43, 1, false, null))));
        m.put("core_results", new RegBlockDef(0x00B4, 33, true, List.of(
                f("result_core__ambient_window_events_sd0", 0, 4, false, null),
                f("result_core__ranging_total_events_sd0", 4, 4, false, null),
                f("result_core__signal_total_events_sd0", 8, 4, true, null),
                f("result_core__total_periods_elapsed_sd0", 12, 4, false, null),
                f("result_core__ambient_window_events_sd1", 16, 4, false, null),
                f("result_core__ranging_total_events_sd1", 20, 4, false, null),
                f("result_core__signal_total_events_sd1", 24, 4, true, null),
                f("result_core__total_periods_elapsed_sd1", 28, 4, false, null),
                f("result_core__spare_0", 32, 1, false, null))));
        m.put("debug_results", new RegBlockDef(0x00D6, 56, true, List.of(
                f("phasecal_result__reference_phase", 0, 2, false, null),
                f("phasecal_result__vcsel_start", 2, 1, false, 0x7FL),
                f("ref_spad_char_result__num_actual_ref_spads", 3, 1, false, 0x3FL),
                f("ref_spad_char_result__ref_location", 4, 1, false, 0x3L),
                f("vhv_result__coldboot_status", 5, 1, false, 0x1L),
                f("vhv_result__search_result", 6, 1, false, 0x3FL),
                f("vhv_result__latest_setting", 7, 1, false, 0x3FL),
                f("result__osc_calibrate_val", 8, 2, false, 0x3FFL),
                f("ana_config__powerdown_go1", 10, 1, false, 0x3L),
                f("ana_config__ref_bg_ctrl", 11, 1, false, 0x3L),
                f("ana_config__regdvdd1v2_ctrl", 12, 1, false, 0xFL),
                f("ana_config__osc_slow_ctrl", 13, 1, false, 0x7L),
                f("test_mode__status", 14, 1, false, 0x1L),
                f("firmware__system_status", 15, 1, false, 0x3L),
                f("firmware__mode_status", 16, 1, false, null),
                f("firmware__secondary_mode_status", 17, 1, false, null),
                f("firmware__cal_repeat_rate_counter", 18, 2, false, 0xFFFL),
                f("gph__system__thresh_high", 22, 2, false, null),
                f("gph__system__thresh_low", 24, 2, false, null),
                f("gph__system__enable_xtalk_per_quadrant", 26, 1, false, 0x1L),
                f("gph__spare_0", 27, 1, false, 0x7L),
                f("gph__sd_config__woi_sd0", 28, 1, false, null),
                f("gph__sd_config__woi_sd1", 29, 1, false, null),
                f("gph__sd_config__initial_phase_sd0", 30, 1, false, 0x7FL),
                f("gph__sd_config__initial_phase_sd1", 31, 1, false, 0x7FL),
                f("gph__sd_config__first_order_select", 32, 1, false, 0x3L),
                f("gph__sd_config__quantifier", 33, 1, false, 0xFL),
                f("gph__roi_config__user_roi_centre_spad", 34, 1, false, null),
                f("gph__roi_config__user_roi_requested_global_xy_size", 35, 1, false, null),
                f("gph__system__sequence_config", 36, 1, false, null),
                f("gph__gph_id", 37, 1, false, 0x1L),
                f("system__interrupt_set", 38, 1, false, 0x3L),
                f("interrupt_manager__enables", 39, 1, false, 0x1FL),
                f("interrupt_manager__clear", 40, 1, false, 0x1FL),
                f("interrupt_manager__status", 41, 1, false, 0x1FL),
                f("mcu_to_host_bank__wr_access_en", 42, 1, false, 0x1L),
                f("power_management__go1_reset_status", 43, 1, false, 0x1L),
                f("pad_startup_mode__value_ro", 44, 1, false, 0x3L),
                f("pad_startup_mode__value_ctrl", 45, 1, false, 0x3FL),
                f("pll_period_us", 46, 4, false, 0x3FFFFL),
                f("interrupt_scheduler__data_out", 50, 4, false, null),
                f("nvm_bist__complete", 54, 1, false, 0x1L),
                f("nvm_bist__status", 55, 1, false, 0x1L))));
        m.put("nvm_copy_data", new RegBlockDef(0x010F, 49, true, List.of(
                f("identification__model_id", 0, 1, false, null),
                f("identification__module_type", 1, 1, false, null),
                f("identification__revision_id", 2, 1, false, null),
                f("identification__module_id", 3, 2, false, null),
                f("ana_config__fast_osc__trim_max", 5, 1, false, 0x7FL),
                f("ana_config__fast_osc__freq_set", 6, 1, false, 0x7L),
                f("ana_config__vcsel_trim", 7, 1, false, 0x7L),
                f("ana_config__vcsel_selion", 8, 1, false, 0x3FL),
                f("ana_config__vcsel_selion_max", 9, 1, false, 0x3FL),
                f("protected_laser_safety__lock_bit", 10, 1, false, 0x1L),
                f("laser_safety__key", 11, 1, false, 0x7FL),
                f("laser_safety__key_ro", 12, 1, false, 0x1L),
                f("laser_safety__clip", 13, 1, false, 0x3FL),
                f("laser_safety__mult", 14, 1, false, 0x3FL),
                f("global_config__spad_enables_rtn_0", 15, 1, false, null),
                f("global_config__spad_enables_rtn_1", 16, 1, false, null),
                f("global_config__spad_enables_rtn_2", 17, 1, false, null),
                f("global_config__spad_enables_rtn_3", 18, 1, false, null),
                f("global_config__spad_enables_rtn_4", 19, 1, false, null),
                f("global_config__spad_enables_rtn_5", 20, 1, false, null),
                f("global_config__spad_enables_rtn_6", 21, 1, false, null),
                f("global_config__spad_enables_rtn_7", 22, 1, false, null),
                f("global_config__spad_enables_rtn_8", 23, 1, false, null),
                f("global_config__spad_enables_rtn_9", 24, 1, false, null),
                f("global_config__spad_enables_rtn_10", 25, 1, false, null),
                f("global_config__spad_enables_rtn_11", 26, 1, false, null),
                f("global_config__spad_enables_rtn_12", 27, 1, false, null),
                f("global_config__spad_enables_rtn_13", 28, 1, false, null),
                f("global_config__spad_enables_rtn_14", 29, 1, false, null),
                f("global_config__spad_enables_rtn_15", 30, 1, false, null),
                f("global_config__spad_enables_rtn_16", 31, 1, false, null),
                f("global_config__spad_enables_rtn_17", 32, 1, false, null),
                f("global_config__spad_enables_rtn_18", 33, 1, false, null),
                f("global_config__spad_enables_rtn_19", 34, 1, false, null),
                f("global_config__spad_enables_rtn_20", 35, 1, false, null),
                f("global_config__spad_enables_rtn_21", 36, 1, false, null),
                f("global_config__spad_enables_rtn_22", 37, 1, false, null),
                f("global_config__spad_enables_rtn_23", 38, 1, false, null),
                f("global_config__spad_enables_rtn_24", 39, 1, false, null),
                f("global_config__spad_enables_rtn_25", 40, 1, false, null),
                f("global_config__spad_enables_rtn_26", 41, 1, false, null),
                f("global_config__spad_enables_rtn_27", 42, 1, false, null),
                f("global_config__spad_enables_rtn_28", 43, 1, false, null),
                f("global_config__spad_enables_rtn_29", 44, 1, false, null),
                f("global_config__spad_enables_rtn_30", 45, 1, false, null),
                f("global_config__spad_enables_rtn_31", 46, 1, false, null),
                f("roi_config__mode_roi_centre_spad", 47, 1, false, null),
                f("roi_config__mode_roi_xy_size", 48, 1, false, null))));
        m.put("patch_debug", new RegBlockDef(0x0F20, 2, true, List.of(
                f("result__debug_status", 0, 1, false, null),
                f("result__debug_stage", 1, 1, false, null))));
        m.put("gph_general_config", new RegBlockDef(0x0F24, 5, true, List.of(
                f("gph__system__thresh_rate_high", 0, 2, false, null),
                f("gph__system__thresh_rate_low", 2, 2, false, null),
                f("gph__system__interrupt_config_gpio", 4, 1, false, null))));
        m.put("gph_static_config", new RegBlockDef(0x0F2F, 6, true, List.of(
                f("gph__dss_config__roi_mode_control", 0, 1, false, 0x7L),
                f("gph__dss_config__manual_effective_spads_select", 1, 2, false, null),
                f("gph__dss_config__manual_block_select", 3, 1, false, null),
                f("gph__dss_config__max_spads_limit", 4, 1, false, null),
                f("gph__dss_config__min_spads_limit", 5, 1, false, null))));
        m.put("gph_timing_config", new RegBlockDef(0x0F36, 16, true, List.of(
                f("gph__mm_config__timeout_macrop_a_hi", 0, 1, false, 0xFL),
                f("gph__mm_config__timeout_macrop_a_lo", 1, 1, false, null),
                f("gph__mm_config__timeout_macrop_b_hi", 2, 1, false, 0xFL),
                f("gph__mm_config__timeout_macrop_b_lo", 3, 1, false, null),
                f("gph__range_config__timeout_macrop_a_hi", 4, 1, false, 0xFL),
                f("gph__range_config__timeout_macrop_a_lo", 5, 1, false, null),
                f("gph__range_config__vcsel_period_a", 6, 1, false, 0x3FL),
                f("gph__range_config__vcsel_period_b", 7, 1, false, 0x3FL),
                f("gph__range_config__timeout_macrop_b_hi", 8, 1, false, 0xFL),
                f("gph__range_config__timeout_macrop_b_lo", 9, 1, false, null),
                f("gph__range_config__sigma_thresh", 10, 2, false, null),
                f("gph__range_config__min_count_rate_rtn_limit_mcps", 12, 2, false, null),
                f("gph__range_config__valid_phase_low", 14, 1, false, null),
                f("gph__range_config__valid_phase_high", 15, 1, false, null))));
        m.put("fw_internal", new RegBlockDef(0x0F46, 2, true, List.of(
                f("firmware__internal_stream_count_div", 0, 1, false, null),
                f("firmware__internal_stream_counter_val", 1, 1, false, null))));
        return m;
    }

    private static Map<String, Long> tuning() {
        Map<String, Long> m = new LinkedHashMap<>();
        m.put("tp_tuning_parm_version", 30L);
        m.put("tp_tuning_parm_key_table_version", 14L);
        m.put("tp_tuning_parm_lld_version", 12180L);
        m.put("tp_init_phase_rtn_lite_long", 14L);
        m.put("tp_init_phase_rtn_lite_med", 10L);
        m.put("tp_init_phase_rtn_lite_short", 6L);
        m.put("tp_init_phase_ref_lite_long", 14L);
        m.put("tp_init_phase_ref_lite_med", 10L);
        m.put("tp_init_phase_ref_lite_short", 6L);
        m.put("tp_init_phase_rtn_hist_long", 9L);
        m.put("tp_init_phase_rtn_hist_med", 5L);
        m.put("tp_init_phase_rtn_hist_short", 3L);
        m.put("tp_init_phase_ref_hist_long", 6L);
        m.put("tp_init_phase_ref_hist_med", 6L);
        m.put("tp_init_phase_ref_hist_short", 6L);
        m.put("tp_consistency_lite_phase_tolerance", 2L);
        m.put("tp_phasecal_target", 33L);
        m.put("tp_cal_repeat_rate", 0L);
        m.put("tp_lite_min_clip", 0L);
        m.put("tp_lite_long_sigma_thresh_mm", 60L);
        m.put("tp_lite_med_sigma_thresh_mm", 60L);
        m.put("tp_lite_short_sigma_thresh_mm", 60L);
        m.put("tp_lite_long_min_count_rate_rtn_mcps", 128L);
        m.put("tp_lite_med_min_count_rate_rtn_mcps", 128L);
        m.put("tp_lite_short_min_count_rate_rtn_mcps", 128L);
        m.put("tp_lite_sigma_est_pulse_width_ns", 8L);
        m.put("tp_lite_sigma_est_amb_width_ns", 16L);
        m.put("tp_lite_sigma_ref_mm", 1L);
        m.put("tp_lite_seed_cfg", 2L);
        m.put("tp_timed_seed_cfg", 1L);
        m.put("tp_lite_quantifier", 2L);
        m.put("tp_lite_first_order_select", 0L);
        m.put("tp_uwr_enable", 1L);
        m.put("tp_uwr_med_z_1_min", 2000L);
        m.put("tp_uwr_med_z_1_max", 2750L);
        m.put("tp_uwr_med_z_2_min", 250L);
        m.put("tp_uwr_med_z_2_max", 1000L);
        m.put("tp_uwr_med_z_3_min", 1250L);
        m.put("tp_uwr_med_z_3_max", 1750L);
        m.put("tp_uwr_med_z_4_min", 1250L);
        m.put("tp_uwr_med_z_4_max", 1750L);
        m.put("tp_uwr_med_z_5_min", -200L);
        m.put("tp_uwr_med_z_5_max", 200L);
        m.put("tp_uwr_med_corr_z_1_rangea", 2300L);
        m.put("tp_uwr_med_corr_z_1_rangeb", 0L);
        m.put("tp_uwr_med_corr_z_2_rangea", 2300L);
        m.put("tp_uwr_med_corr_z_2_rangeb", 3050L);
        m.put("tp_uwr_med_corr_z_3_rangea", 4600L);
        m.put("tp_uwr_med_corr_z_3_rangeb", 3050L);
        m.put("tp_uwr_med_corr_z_4_rangea", 4600L);
        m.put("tp_uwr_med_corr_z_4_rangeb", 6200L);
        m.put("tp_uwr_med_corr_z_5_rangea", 0L);
        m.put("tp_uwr_med_corr_z_5_rangeb", 0L);
        m.put("tp_uwr_lng_z_1_min", 250L);
        m.put("tp_uwr_lng_z_1_max", 1250L);
        m.put("tp_uwr_lng_z_2_min", 3250L);
        m.put("tp_uwr_lng_z_2_max", 4500L);
        m.put("tp_uwr_lng_z_3_min", -200L);
        m.put("tp_uwr_lng_z_3_max", 200L);
        m.put("tp_uwr_lng_z_4_min", 0L);
        m.put("tp_uwr_lng_z_4_max", 0L);
        m.put("tp_uwr_lng_z_5_min", 0L);
        m.put("tp_uwr_lng_z_5_max", 0L);
        m.put("tp_uwr_lng_corr_z_1_rangea", 3850L);
        m.put("tp_uwr_lng_corr_z_1_rangeb", 4600L);
        m.put("tp_uwr_lng_corr_z_2_rangea", 3850L);
        m.put("tp_uwr_lng_corr_z_2_rangeb", 0L);
        m.put("tp_uwr_lng_corr_z_3_rangea", 0L);
        m.put("tp_uwr_lng_corr_z_3_rangeb", 0L);
        m.put("tp_uwr_lng_corr_z_4_rangea", 0L);
        m.put("tp_uwr_lng_corr_z_4_rangeb", 0L);
        m.put("tp_uwr_lng_corr_z_5_rangea", 0L);
        m.put("tp_uwr_lng_corr_z_5_rangeb", 0L);
        m.put("tp_dss_target_lite_mcps", 2560L);
        m.put("tp_dss_target_histo_mcps", 5120L);
        m.put("tp_dss_target_histo_mz_mcps", 5120L);
        m.put("tp_dss_target_timed_mcps", 2560L);
        m.put("tp_phasecal_timeout_lite_us", 1000L);
        m.put("tp_phasecal_timeout_hist_long_us", 15000L);
        m.put("tp_phasecal_timeout_hist_med_us", 9000L);
        m.put("tp_phasecal_timeout_hist_short_us", 6000L);
        m.put("tp_phasecal_timeout_mz_long_us", 15000L);
        m.put("tp_phasecal_timeout_mz_med_us", 9000L);
        m.put("tp_phasecal_timeout_mz_short_us", 6000L);
        m.put("tp_phasecal_timeout_timed_us", 1000L);
        m.put("tp_mm_timeout_lite_us", 2000L);
        m.put("tp_mm_timeout_histo_us", 2000L);
        m.put("tp_mm_timeout_mz_us", 2000L);
        m.put("tp_mm_timeout_timed_us", 2000L);
        m.put("tp_range_timeout_lite_us", 63000L);
        m.put("tp_range_timeout_histo_us", 2500L);
        m.put("tp_range_timeout_mz_us", 2500L);
        m.put("tp_range_timeout_timed_us", 13000L);
        m.put("tp_mm_timeout_lpa_us", 1L);
        m.put("tp_range_timeout_lpa_us", 8000L);
        m.put("tp_dss_target_very_short_mcps", 10240L);
        m.put("tp_phasecal_patch_power", 0L);
        m.put("tp_hist_merge", 1L);
        m.put("tp_reset_merge_threshold", 15000L);
        m.put("tp_hist_merge_max_size", 6L);
        return m;
    }

    private static Map<String, Long> defaults() {
        Map<String, Long> m = new LinkedHashMap<>();
        m.put("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT", 0L);
        m.put("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT", 2048L);
        m.put("TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT", 0L);
        m.put("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT", 8L);
        m.put("TUNINGPARM_CONSISTENCY_LITE_PHASE_TOLERANCE_DEFAULT", 2L);
        m.put("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_0_DEFAULT", 15L);
        m.put("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_1_DEFAULT", 52L);
        m.put("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_2_DEFAULT", 200L);
        m.put("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_3_DEFAULT", 364L);
        m.put("TUNINGPARM_DMAX_CFG_REFLECTANCE_ARRAY_4_DEFAULT", 400L);
        m.put("TUNINGPARM_DMAX_CFG_SIGNAL_THRESH_SIGMA_DEFAULT", 32L);
        m.put("TUNINGPARM_DYNXTALK_AVERAGED_XTALK_DELTA_DEFAULT", 308L);
        m.put("TUNINGPARM_DYNXTALK_CLIP_LIMIT_DEFAULT", 10240L);
        m.put("TUNINGPARM_DYNXTALK_MAX_SMUDGE_FACTOR_DEFAULT", 2000L);
        m.put("TUNINGPARM_DYNXTALK_NODETECT_AMB_THRESHOLD_KCPS_DEFAULT", 57671680L);
        m.put("TUNINGPARM_DYNXTALK_NODETECT_MIN_RANGE_MM_DEFAULT", 900L);
        m.put("TUNINGPARM_DYNXTALK_NODETECT_SAMPLE_LIMIT_DEFAULT", 40L);
        m.put("TUNINGPARM_DYNXTALK_NODETECT_XTALK_OFFSET_KCPS_DEFAULT", 410L);
        m.put("TUNINGPARM_DYNXTALK_NOISE_MARGIN_DEFAULT", 100L);
        m.put("TUNINGPARM_DYNXTALK_SAMPLE_LIMIT_DEFAULT", 200L);
        m.put("TUNINGPARM_DYNXTALK_SCALER_CALC_METHOD_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_SINGLE_XTALK_DELTA_DEFAULT", 2048L);
        m.put("TUNINGPARM_DYNXTALK_SMUDGE_COR_SINGLE_APPLY_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_SMUDGE_MARGIN_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_USER_SCALER_SET_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_XGRADIENT_SCALER_DEFAULT", 256L);
        m.put("TUNINGPARM_DYNXTALK_XTALK_AMB_THRESHOLD_DEFAULT", 128L);
        m.put("TUNINGPARM_DYNXTALK_XTALK_OFFSET_LIMIT_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_XTALK_OFFSET_LIMIT_HI_DEFAULT", 0L);
        m.put("TUNINGPARM_DYNXTALK_YGRADIENT_SCALER_DEFAULT", 256L);
        m.put("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT", 4L);
        m.put("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT", 1L);
        m.put("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT", 4157L);
        m.put("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT", 80L);
        m.put("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT", 100L);
        m.put("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT", 1L);
        m.put("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT", 2L);
        m.put("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT", 1987L);
        m.put("TUNINGPARM_HIST_MERGE_DEFAULT", 1L);
        m.put("TUNINGPARM_HIST_MERGE_MAX_SIZE_DEFAULT", 6L);
        m.put("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT", 16L);
        m.put("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT", 50L);
        m.put("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT", 1L);
        m.put("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT", 180L);
        m.put("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT", 100L);
        m.put("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT", 1L);
        m.put("TUNINGPARM_HIST_XTALK_MARGIN_KCPS_DEFAULT", 0L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_HISTO_LONG_RANGE_DEFAULT", 6L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_HISTO_MED_RANGE_DEFAULT", 6L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_HISTO_SHORT_RANGE_DEFAULT", 6L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_LITE_LONG_RANGE_DEFAULT", 14L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_LITE_MED_RANGE_DEFAULT", 10L);
        m.put("TUNINGPARM_INITIAL_PHASE_REF_LITE_SHORT_RANGE_DEFAULT", 6L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_LONG_RANGE_DEFAULT", 9L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_MED_RANGE_DEFAULT", 5L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_HISTO_SHORT_RANGE_DEFAULT", 3L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_LITE_LONG_RANGE_DEFAULT", 14L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_LITE_MED_RANGE_DEFAULT", 10L);
        m.put("TUNINGPARM_INITIAL_PHASE_RTN_LITE_SHORT_RANGE_DEFAULT", 6L);
        m.put("TUNINGPARM_KEY_TABLE_VERSION_DEFAULT", 14L);
        m.put("TUNINGPARM_LITE_CAL_REPEAT_RATE_DEFAULT", 0L);
        m.put("TUNINGPARM_LITE_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 2560L);
        m.put("TUNINGPARM_LITE_FIRST_ORDER_SELECT_DEFAULT", 0L);
        m.put("TUNINGPARM_LITE_LONG_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128L);
        m.put("TUNINGPARM_LITE_LONG_SIGMA_THRESH_MM_DEFAULT", 60L);
        m.put("TUNINGPARM_LITE_MED_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128L);
        m.put("TUNINGPARM_LITE_MED_SIGMA_THRESH_MM_DEFAULT", 60L);
        m.put("TUNINGPARM_LITE_MIN_CLIP_MM_DEFAULT", 0L);
        m.put("TUNINGPARM_LITE_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_LITE_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 1000L);
        m.put("TUNINGPARM_LITE_QUANTIFIER_DEFAULT", 2L);
        m.put("TUNINGPARM_LITE_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 63000L);
        m.put("TUNINGPARM_LITE_RANGING_GAIN_FACTOR_DEFAULT", 2011L);
        m.put("TUNINGPARM_LITE_RIT_MULT_DEFAULT", 64L);
        m.put("TUNINGPARM_LITE_SEED_CONFIG_DEFAULT", 2L);
        m.put("TUNINGPARM_LITE_SHORT_MIN_COUNT_RATE_RTN_MCPS_DEFAULT", 128L);
        m.put("TUNINGPARM_LITE_SHORT_SIGMA_THRESH_MM_DEFAULT", 60L);
        m.put("TUNINGPARM_LITE_SIGMA_EST_AMB_WIDTH_NS_DEFAULT", 16L);
        m.put("TUNINGPARM_LITE_SIGMA_EST_PULSE_WIDTH_DEFAULT", 8L);
        m.put("TUNINGPARM_LITE_SIGMA_REF_MM_DEFAULT", 1L);
        m.put("TUNINGPARM_LITE_XTALK_MARGIN_KCPS_DEFAULT", 0L);
        m.put("TUNINGPARM_LLD_VERSION_DEFAULT", 12180L);
        m.put("TUNINGPARM_LOWPOWERAUTO_MM_CONFIG_TIMEOUT_US_DEFAULT", 1L);
        m.put("TUNINGPARM_LOWPOWERAUTO_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 8000L);
        m.put("TUNINGPARM_LOWPOWERAUTO_VHV_LOOP_BOUND_DEFAULT", 3L);
        m.put("TUNINGPARM_MZ_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 5120L);
        m.put("TUNINGPARM_MZ_LONG_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 15000L);
        m.put("TUNINGPARM_MZ_MED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 9000L);
        m.put("TUNINGPARM_MZ_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_MZ_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 2500L);
        m.put("TUNINGPARM_MZ_SHORT_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 6000L);
        m.put("TUNINGPARM_OFFSET_CAL_DSS_RATE_MCPS_DEFAULT", 2560L);
        m.put("TUNINGPARM_OFFSET_CAL_MM1_SAMPLES_DEFAULT", 40L);
        m.put("TUNINGPARM_OFFSET_CAL_MM2_SAMPLES_DEFAULT", 9L);
        m.put("TUNINGPARM_OFFSET_CAL_MM_TIMEOUT_US_DEFAULT", 13000L);
        m.put("TUNINGPARM_OFFSET_CAL_PHASECAL_TIMEOUT_US_DEFAULT", 15000L);
        m.put("TUNINGPARM_OFFSET_CAL_PRE_SAMPLES_DEFAULT", 8L);
        m.put("TUNINGPARM_OFFSET_CAL_RANGE_TIMEOUT_US_DEFAULT", 13000L);
        m.put("TUNINGPARM_PHASECAL_PATCH_POWER_DEFAULT", 0L);
        m.put("TUNINGPARM_PHASECAL_TARGET_DEFAULT", 33L);
        m.put("TUNINGPARM_RANGING_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 5120L);
        m.put("TUNINGPARM_RANGING_LONG_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 15000L);
        m.put("TUNINGPARM_RANGING_MED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 9000L);
        m.put("TUNINGPARM_RANGING_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_RANGING_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 2500L);
        m.put("TUNINGPARM_RANGING_SHORT_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 6000L);
        m.put("TUNINGPARM_REFSPADCHAR_DEVICE_TEST_MODE_DEFAULT", 8L);
        m.put("TUNINGPARM_REFSPADCHAR_MAX_COUNTRATE_LIMIT_MCPS_DEFAULT", 5120L);
        m.put("TUNINGPARM_REFSPADCHAR_MIN_COUNTRATE_LIMIT_MCPS_DEFAULT", 1280L);
        m.put("TUNINGPARM_REFSPADCHAR_PHASECAL_TIMEOUT_US_DEFAULT", 1000L);
        m.put("TUNINGPARM_REFSPADCHAR_TARGET_COUNT_RATE_MCPS_DEFAULT", 2560L);
        m.put("TUNINGPARM_REFSPADCHAR_VCSEL_PERIOD_DEFAULT", 11L);
        m.put("TUNINGPARM_RESET_MERGE_THRESHOLD_DEFAULT", 15000L);
        m.put("TUNINGPARM_SPADMAP_RATE_LIMIT_MCPS_DEFAULT", 12L);
        m.put("TUNINGPARM_SPADMAP_VCSEL_PERIOD_DEFAULT", 18L);
        m.put("TUNINGPARM_SPADMAP_VCSEL_START_DEFAULT", 15L);
        m.put("TUNINGPARM_TIMED_DSS_CONFIG_TARGET_TOTAL_RATE_MCPS_DEFAULT", 2560L);
        m.put("TUNINGPARM_TIMED_MM_CONFIG_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_TIMED_PHASECAL_CONFIG_TIMEOUT_US_DEFAULT", 1000L);
        m.put("TUNINGPARM_TIMED_RANGE_CONFIG_TIMEOUT_US_DEFAULT", 13000L);
        m.put("TUNINGPARM_TIMED_SEED_CONFIG_DEFAULT", 1L);
        m.put("TUNINGPARM_UWR_ENABLE_DEFAULT", 1L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_1_RANGEA_DEFAULT", 3850L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_1_RANGEB_DEFAULT", 4600L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_2_RANGEA_DEFAULT", 3850L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_2_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_3_RANGEA_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_3_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_4_RANGEA_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_4_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_5_RANGEA_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_CORRECTION_ZONE_5_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_1_MAX_DEFAULT", 1250L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_1_MIN_DEFAULT", 250L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_2_MAX_DEFAULT", 4500L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_2_MIN_DEFAULT", 3250L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_3_MAX_DEFAULT", 200L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_3_MIN_DEFAULT", -200L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_4_MAX_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_4_MIN_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_5_MAX_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_LONG_ZONE_5_MIN_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_1_RANGEA_DEFAULT", 2300L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_1_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_2_RANGEA_DEFAULT", 2300L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_2_RANGEB_DEFAULT", 3050L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_3_RANGEA_DEFAULT", 4600L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_3_RANGEB_DEFAULT", 3050L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_4_RANGEA_DEFAULT", 4600L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_4_RANGEB_DEFAULT", 6200L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_5_RANGEA_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_MEDIUM_CORRECTION_ZONE_5_RANGEB_DEFAULT", 0L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_1_MAX_DEFAULT", 2750L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_1_MIN_DEFAULT", 2000L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_2_MAX_DEFAULT", 1000L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_2_MIN_DEFAULT", 250L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_3_MAX_DEFAULT", 1750L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_3_MIN_DEFAULT", 1250L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_4_MAX_DEFAULT", 1750L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_4_MIN_DEFAULT", 1250L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_5_MAX_DEFAULT", 200L);
        m.put("TUNINGPARM_UWR_MEDIUM_ZONE_5_MIN_DEFAULT", -200L);
        m.put("TUNINGPARM_VERSION_DEFAULT", 30L);
        m.put("TUNINGPARM_VERY_SHORT_DSS_RATE_MCPS_DEFAULT", 10240L);
        m.put("TUNINGPARM_VHV_LOOPBOUND_DEFAULT", 129L);
        m.put("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT", 80L);
        m.put("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT", 140L);
        m.put("TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT", 50L);
        m.put("TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT", 400L);
        m.put("TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT", 50L);
        m.put("TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT", -50L);
        m.put("TUNINGPARM_XTALK_EXTRACT_BIN_TIMEOUT_US_DEFAULT", 10000L);
        m.put("TUNINGPARM_XTALK_EXTRACT_DSS_RATE_MCPS_DEFAULT", 5120L);
        m.put("TUNINGPARM_XTALK_EXTRACT_DSS_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_XTALK_EXTRACT_MAX_FILTER_THRESH_MM_DEFAULT", 70L);
        m.put("TUNINGPARM_XTALK_EXTRACT_MAX_VALID_RATE_KCPS_DEFAULT", 640L);
        m.put("TUNINGPARM_XTALK_EXTRACT_MIN_FILTER_THRESH_MM_DEFAULT", -70L);
        m.put("TUNINGPARM_XTALK_EXTRACT_NUM_OF_SAMPLES_DEFAULT", 7L);
        m.put("TUNINGPARM_XTALK_EXTRACT_PHASECAL_TIMEOUT_US_DEFAULT", 15000L);
        m.put("TUNINGPARM_XTALK_EXTRACT_SIGMA_THRESHOLD_MM_DEFAULT", 140L);
        m.put("TUNINGPARM_ZONE_CAL_DSS_RATE_MCPS_DEFAULT", 5120L);
        m.put("TUNINGPARM_ZONE_CAL_DSS_TIMEOUT_US_DEFAULT", 2000L);
        m.put("TUNINGPARM_ZONE_CAL_PHASECAL_NUM_SAMPLES_DEFAULT", 16L);
        m.put("TUNINGPARM_ZONE_CAL_PHASECAL_TIMEOUT_US_DEFAULT", 15000L);
        m.put("TUNINGPARM_ZONE_CAL_RANGE_TIMEOUT_US_DEFAULT", 1000L);
        m.put("TUNINGPARM_ZONE_CAL_ZONE_NUM_SAMPLES_DEFAULT", 8L);
        return m;
    }
}
