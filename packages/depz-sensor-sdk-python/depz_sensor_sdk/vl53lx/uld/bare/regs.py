"""
uld/vl53lx/regs.py — the VL53LX register blocks, as ST's BareDriver sees them.

The BareDriver does not touch registers one at a time. It keeps the device
image in a handful of C structs (`VL53LX_static_config_t`, `..._timing_config_t`,
...), each mapping onto one contiguous I2C block, and moves a whole block at a
time with `VL53LX_get_*` / `VL53LX_set_*`. Ranging start writes five of them —
static_nvm through system_control, 0x0001..0x0087 — in a single transfer.

`BLOCKS` below is that mapping: base address, byte length, whether the block is
writable, and the field list with offset, width, signedness and the bit mask the
C codec applies. It is generated from `vl53lx_register_funcs.c`
(`VL53LX_i2c_decode_*`), not hand-written, so the masks and the reserved holes
— general_config has one at offset 10 — match the C driver exactly.

Reference C driver: ../../../temp/STSW-IMG033_L3/…/VL53L3CX_BareDriver/
"""

from collections import namedtuple

F = namedtuple('F', 'name off width signed mask')
Block = namedtuple('Block', 'base size writable fields')

BLOCKS = {
    'static_nvm_managed': Block(0x0001, 11, True, (
        F('i2c_slave__device_address',                           0, 1, False, 0x7F),
        F('ana_config__vhv_ref_sel_vddpix',                      1, 1, False, 0xF),
        F('ana_config__vhv_ref_sel_vquench',                     2, 1, False, 0x7F),
        F('ana_config__reg_avdd1v2_sel',                         3, 1, False, 0x3),
        F('ana_config__fast_osc__trim',                          4, 1, False, 0x7F),
        F('osc_measured__fast_osc__frequency',                   5, 2, False, None),
        F('vhv_config__timeout_macrop_loop_bound',               7, 1, False, None),
        F('vhv_config__count_thresh',                            8, 1, False, None),
        F('vhv_config__offset',                                  9, 1, False, 0x3F),
        F('vhv_config__init',                                   10, 1, False, None),
    )),
    'customer_nvm_managed': Block(0x000D, 23, True, (
        F('global_config__spad_enables_ref_0',                   0, 1, False, None),
        F('global_config__spad_enables_ref_1',                   1, 1, False, None),
        F('global_config__spad_enables_ref_2',                   2, 1, False, None),
        F('global_config__spad_enables_ref_3',                   3, 1, False, None),
        F('global_config__spad_enables_ref_4',                   4, 1, False, None),
        F('global_config__spad_enables_ref_5',                   5, 1, False, 0xF),
        F('global_config__ref_en_start_select',                  6, 1, False, None),
        F('ref_spad_man__num_requested_ref_spads',               7, 1, False, 0x3F),
        F('ref_spad_man__ref_location',                          8, 1, False, 0x3),
        F('algo__crosstalk_compensation_plane_offset_kcps',      9, 2, False, None),
        F('algo__crosstalk_compensation_x_plane_gradient_kcps',  11, 2, True,  None),
        F('algo__crosstalk_compensation_y_plane_gradient_kcps',  13, 2, True,  None),
        F('ref_spad_char__total_rate_target_mcps',              15, 2, False, None),
        F('algo__part_to_part_range_offset_mm',                 17, 2, True,  0x1FFF),
        F('mm_config__inner_offset_mm',                         19, 2, True,  None),
        F('mm_config__outer_offset_mm',                         21, 2, True,  None),
    )),
    'static_config': Block(0x0024, 32, True, (
        F('dss_config__target_total_rate_mcps',                  0, 2, False, None),
        F('debug__ctrl',                                         2, 1, False, 0x1),
        F('test_mode__ctrl',                                     3, 1, False, 0xF),
        F('clk_gating__ctrl',                                    4, 1, False, 0xF),
        F('nvm_bist__ctrl',                                      5, 1, False, 0x1F),
        F('nvm_bist__num_nvm_words',                             6, 1, False, 0x7F),
        F('nvm_bist__start_address',                             7, 1, False, 0x7F),
        F('host_if__status',                                     8, 1, False, 0x1),
        F('pad_i2c_hv__config',                                  9, 1, False, None),
        F('pad_i2c_hv__extsup_config',                          10, 1, False, 0x1),
        F('gpio_hv_pad__ctrl',                                  11, 1, False, 0x3),
        F('gpio_hv_mux__ctrl',                                  12, 1, False, 0x1F),
        F('gpio__tio_hv_status',                                13, 1, False, 0x3),
        F('gpio__fio_hv_status',                                14, 1, False, 0x3),
        F('ana_config__spad_sel_pswidth',                       15, 1, False, 0x7),
        F('ana_config__vcsel_pulse_width_offset',               16, 1, False, 0x1F),
        F('ana_config__fast_osc__config_ctrl',                  17, 1, False, 0x1),
        F('sigma_estimator__effective_pulse_width_ns',          18, 1, False, None),
        F('sigma_estimator__effective_ambient_width_ns',        19, 1, False, None),
        F('sigma_estimator__sigma_ref_mm',                      20, 1, False, None),
        F('algo__crosstalk_compensation_valid_height_mm',       21, 1, False, None),
        F('spare_host_config__static_config_spare_0',           22, 1, False, None),
        F('spare_host_config__static_config_spare_1',           23, 1, False, None),
        F('algo__range_ignore_threshold_mcps',                  24, 2, False, None),
        F('algo__range_ignore_valid_height_mm',                 26, 1, False, None),
        F('algo__range_min_clip',                               27, 1, False, None),
        F('algo__consistency_check__tolerance',                 28, 1, False, 0xF),
        F('spare_host_config__static_config_spare_2',           29, 1, False, None),
        F('sd_config__reset_stages_msb',                        30, 1, False, 0xF),
        F('sd_config__reset_stages_lsb',                        31, 1, False, None),
    )),
    'general_config': Block(0x0044, 22, True, (
        F('gph_config__stream_count_update_value',               0, 1, False, None),
        F('global_config__stream_divider',                       1, 1, False, None),
        F('system__interrupt_config_gpio',                       2, 1, False, None),
        F('cal_config__vcsel_start',                             3, 1, False, 0x7F),
        F('cal_config__repeat_rate',                             4, 2, False, 0xFFF),
        F('global_config__vcsel_width',                          6, 1, False, 0x7F),
        F('phasecal_config__timeout_macrop',                     7, 1, False, None),
        F('phasecal_config__target',                             8, 1, False, None),
        F('phasecal_config__override',                           9, 1, False, 0x1),
        F('dss_config__roi_mode_control',                       11, 1, False, 0x7),
        F('system__thresh_rate_high',                           12, 2, False, None),
        F('system__thresh_rate_low',                            14, 2, False, None),
        F('dss_config__manual_effective_spads_select',          16, 2, False, None),
        F('dss_config__manual_block_select',                    18, 1, False, None),
        F('dss_config__aperture_attenuation',                   19, 1, False, None),
        F('dss_config__max_spads_limit',                        20, 1, False, None),
        F('dss_config__min_spads_limit',                        21, 1, False, None),
    )),
    'timing_config': Block(0x005A, 23, True, (
        F('mm_config__timeout_macrop_a_hi',                      0, 1, False, 0xF),
        F('mm_config__timeout_macrop_a_lo',                      1, 1, False, None),
        F('mm_config__timeout_macrop_b_hi',                      2, 1, False, 0xF),
        F('mm_config__timeout_macrop_b_lo',                      3, 1, False, None),
        F('range_config__timeout_macrop_a_hi',                   4, 1, False, 0xF),
        F('range_config__timeout_macrop_a_lo',                   5, 1, False, None),
        F('range_config__vcsel_period_a',                        6, 1, False, 0x3F),
        F('range_config__timeout_macrop_b_hi',                   7, 1, False, 0xF),
        F('range_config__timeout_macrop_b_lo',                   8, 1, False, None),
        F('range_config__vcsel_period_b',                        9, 1, False, 0x3F),
        F('range_config__sigma_thresh',                         10, 2, False, None),
        F('range_config__min_count_rate_rtn_limit_mcps',        12, 2, False, None),
        F('range_config__valid_phase_low',                      14, 1, False, None),
        F('range_config__valid_phase_high',                     15, 1, False, None),
        F('system__intermeasurement_period',                    18, 4, False, None),
        F('system__fractional_enable',                          22, 1, False, 0x1),
    )),
    'dynamic_config': Block(0x0071, 18, True, (
        F('system__grouped_parameter_hold_0',                    0, 1, False, 0x3),
        F('system__thresh_high',                                 1, 2, False, None),
        F('system__thresh_low',                                  3, 2, False, None),
        F('system__enable_xtalk_per_quadrant',                   5, 1, False, 0x1),
        F('system__seed_config',                                 6, 1, False, 0x7),
        F('sd_config__woi_sd0',                                  7, 1, False, None),
        F('sd_config__woi_sd1',                                  8, 1, False, None),
        F('sd_config__initial_phase_sd0',                        9, 1, False, 0x7F),
        F('sd_config__initial_phase_sd1',                       10, 1, False, 0x7F),
        F('system__grouped_parameter_hold_1',                   11, 1, False, 0x3),
        F('sd_config__first_order_select',                      12, 1, False, 0x3),
        F('sd_config__quantifier',                              13, 1, False, 0xF),
        F('roi_config__user_roi_centre_spad',                   14, 1, False, None),
        F('roi_config__user_roi_requested_global_xy_size',      15, 1, False, None),
        F('system__sequence_config',                            16, 1, False, None),
        F('system__grouped_parameter_hold',                     17, 1, False, 0x3),
    )),
    'system_control': Block(0x0083, 5, True, (
        F('power_management__go1_power_force',                   0, 1, False, 0x1),
        F('system__stream_count_ctrl',                           1, 1, False, 0x1),
        F('firmware__enable',                                    2, 1, False, 0x1),
        F('system__interrupt_clear',                             3, 1, False, 0x3),
        F('system__mode_start',                                  4, 1, False, None),
    )),
    'system_results': Block(0x0088, 44, True, (
        F('result__interrupt_status',                            0, 1, False, 0x3F),
        F('result__range_status',                                1, 1, False, None),
        F('result__report_status',                               2, 1, False, 0xF),
        F('result__stream_count',                                3, 1, False, None),
        F('result__dss_actual_effective_spads_sd0',              4, 2, False, None),
        F('result__peak_signal_count_rate_mcps_sd0',             6, 2, False, None),
        F('result__ambient_count_rate_mcps_sd0',                 8, 2, False, None),
        F('result__sigma_sd0',                                  10, 2, False, None),
        F('result__phase_sd0',                                  12, 2, False, None),
        F('result__final_crosstalk_corrected_range_mm_sd0',     14, 2, False, None),
        F('result__peak_signal_count_rate_crosstalk_corrected_mcps_sd0',  16, 2, False, None),
        F('result__mm_inner_actual_effective_spads_sd0',        18, 2, False, None),
        F('result__mm_outer_actual_effective_spads_sd0',        20, 2, False, None),
        F('result__avg_signal_count_rate_mcps_sd0',             22, 2, False, None),
        F('result__dss_actual_effective_spads_sd1',             24, 2, False, None),
        F('result__peak_signal_count_rate_mcps_sd1',            26, 2, False, None),
        F('result__ambient_count_rate_mcps_sd1',                28, 2, False, None),
        F('result__sigma_sd1',                                  30, 2, False, None),
        F('result__phase_sd1',                                  32, 2, False, None),
        F('result__final_crosstalk_corrected_range_mm_sd1',     34, 2, False, None),
        F('result__spare_0_sd1',                                36, 2, False, None),
        F('result__spare_1_sd1',                                38, 2, False, None),
        F('result__spare_2_sd1',                                40, 2, False, None),
        F('result__spare_3_sd1',                                42, 1, False, None),
        F('result__thresh_info',                                43, 1, False, None),
    )),
    'core_results': Block(0x00B4, 33, True, (
        F('result_core__ambient_window_events_sd0',              0, 4, False, None),
        F('result_core__ranging_total_events_sd0',               4, 4, False, None),
        F('result_core__signal_total_events_sd0',                8, 4, True,  None),
        F('result_core__total_periods_elapsed_sd0',             12, 4, False, None),
        F('result_core__ambient_window_events_sd1',             16, 4, False, None),
        F('result_core__ranging_total_events_sd1',              20, 4, False, None),
        F('result_core__signal_total_events_sd1',               24, 4, True,  None),
        F('result_core__total_periods_elapsed_sd1',             28, 4, False, None),
        F('result_core__spare_0',                               32, 1, False, None),
    )),
    'debug_results': Block(0x00D6, 56, True, (
        F('phasecal_result__reference_phase',                    0, 2, False, None),
        F('phasecal_result__vcsel_start',                        2, 1, False, 0x7F),
        F('ref_spad_char_result__num_actual_ref_spads',          3, 1, False, 0x3F),
        F('ref_spad_char_result__ref_location',                  4, 1, False, 0x3),
        F('vhv_result__coldboot_status',                         5, 1, False, 0x1),
        F('vhv_result__search_result',                           6, 1, False, 0x3F),
        F('vhv_result__latest_setting',                          7, 1, False, 0x3F),
        F('result__osc_calibrate_val',                           8, 2, False, 0x3FF),
        F('ana_config__powerdown_go1',                          10, 1, False, 0x3),
        F('ana_config__ref_bg_ctrl',                            11, 1, False, 0x3),
        F('ana_config__regdvdd1v2_ctrl',                        12, 1, False, 0xF),
        F('ana_config__osc_slow_ctrl',                          13, 1, False, 0x7),
        F('test_mode__status',                                  14, 1, False, 0x1),
        F('firmware__system_status',                            15, 1, False, 0x3),
        F('firmware__mode_status',                              16, 1, False, None),
        F('firmware__secondary_mode_status',                    17, 1, False, None),
        F('firmware__cal_repeat_rate_counter',                  18, 2, False, 0xFFF),
        F('gph__system__thresh_high',                           22, 2, False, None),
        F('gph__system__thresh_low',                            24, 2, False, None),
        F('gph__system__enable_xtalk_per_quadrant',             26, 1, False, 0x1),
        F('gph__spare_0',                                       27, 1, False, 0x7),
        F('gph__sd_config__woi_sd0',                            28, 1, False, None),
        F('gph__sd_config__woi_sd1',                            29, 1, False, None),
        F('gph__sd_config__initial_phase_sd0',                  30, 1, False, 0x7F),
        F('gph__sd_config__initial_phase_sd1',                  31, 1, False, 0x7F),
        F('gph__sd_config__first_order_select',                 32, 1, False, 0x3),
        F('gph__sd_config__quantifier',                         33, 1, False, 0xF),
        F('gph__roi_config__user_roi_centre_spad',              34, 1, False, None),
        F('gph__roi_config__user_roi_requested_global_xy_size',  35, 1, False, None),
        F('gph__system__sequence_config',                       36, 1, False, None),
        F('gph__gph_id',                                        37, 1, False, 0x1),
        F('system__interrupt_set',                              38, 1, False, 0x3),
        F('interrupt_manager__enables',                         39, 1, False, 0x1F),
        F('interrupt_manager__clear',                           40, 1, False, 0x1F),
        F('interrupt_manager__status',                          41, 1, False, 0x1F),
        F('mcu_to_host_bank__wr_access_en',                     42, 1, False, 0x1),
        F('power_management__go1_reset_status',                 43, 1, False, 0x1),
        F('pad_startup_mode__value_ro',                         44, 1, False, 0x3),
        F('pad_startup_mode__value_ctrl',                       45, 1, False, 0x3F),
        F('pll_period_us',                                      46, 4, False, 0x3FFFF),
        F('interrupt_scheduler__data_out',                      50, 4, False, None),
        F('nvm_bist__complete',                                 54, 1, False, 0x1),
        F('nvm_bist__status',                                   55, 1, False, 0x1),
    )),
    'nvm_copy_data': Block(0x010F, 49, True, (
        F('identification__model_id',                            0, 1, False, None),
        F('identification__module_type',                         1, 1, False, None),
        F('identification__revision_id',                         2, 1, False, None),
        F('identification__module_id',                           3, 2, False, None),
        F('ana_config__fast_osc__trim_max',                      5, 1, False, 0x7F),
        F('ana_config__fast_osc__freq_set',                      6, 1, False, 0x7),
        F('ana_config__vcsel_trim',                              7, 1, False, 0x7),
        F('ana_config__vcsel_selion',                            8, 1, False, 0x3F),
        F('ana_config__vcsel_selion_max',                        9, 1, False, 0x3F),
        F('protected_laser_safety__lock_bit',                   10, 1, False, 0x1),
        F('laser_safety__key',                                  11, 1, False, 0x7F),
        F('laser_safety__key_ro',                               12, 1, False, 0x1),
        F('laser_safety__clip',                                 13, 1, False, 0x3F),
        F('laser_safety__mult',                                 14, 1, False, 0x3F),
        F('global_config__spad_enables_rtn_0',                  15, 1, False, None),
        F('global_config__spad_enables_rtn_1',                  16, 1, False, None),
        F('global_config__spad_enables_rtn_2',                  17, 1, False, None),
        F('global_config__spad_enables_rtn_3',                  18, 1, False, None),
        F('global_config__spad_enables_rtn_4',                  19, 1, False, None),
        F('global_config__spad_enables_rtn_5',                  20, 1, False, None),
        F('global_config__spad_enables_rtn_6',                  21, 1, False, None),
        F('global_config__spad_enables_rtn_7',                  22, 1, False, None),
        F('global_config__spad_enables_rtn_8',                  23, 1, False, None),
        F('global_config__spad_enables_rtn_9',                  24, 1, False, None),
        F('global_config__spad_enables_rtn_10',                 25, 1, False, None),
        F('global_config__spad_enables_rtn_11',                 26, 1, False, None),
        F('global_config__spad_enables_rtn_12',                 27, 1, False, None),
        F('global_config__spad_enables_rtn_13',                 28, 1, False, None),
        F('global_config__spad_enables_rtn_14',                 29, 1, False, None),
        F('global_config__spad_enables_rtn_15',                 30, 1, False, None),
        F('global_config__spad_enables_rtn_16',                 31, 1, False, None),
        F('global_config__spad_enables_rtn_17',                 32, 1, False, None),
        F('global_config__spad_enables_rtn_18',                 33, 1, False, None),
        F('global_config__spad_enables_rtn_19',                 34, 1, False, None),
        F('global_config__spad_enables_rtn_20',                 35, 1, False, None),
        F('global_config__spad_enables_rtn_21',                 36, 1, False, None),
        F('global_config__spad_enables_rtn_22',                 37, 1, False, None),
        F('global_config__spad_enables_rtn_23',                 38, 1, False, None),
        F('global_config__spad_enables_rtn_24',                 39, 1, False, None),
        F('global_config__spad_enables_rtn_25',                 40, 1, False, None),
        F('global_config__spad_enables_rtn_26',                 41, 1, False, None),
        F('global_config__spad_enables_rtn_27',                 42, 1, False, None),
        F('global_config__spad_enables_rtn_28',                 43, 1, False, None),
        F('global_config__spad_enables_rtn_29',                 44, 1, False, None),
        F('global_config__spad_enables_rtn_30',                 45, 1, False, None),
        F('global_config__spad_enables_rtn_31',                 46, 1, False, None),
        F('roi_config__mode_roi_centre_spad',                   47, 1, False, None),
        F('roi_config__mode_roi_xy_size',                       48, 1, False, None),
    )),
    'patch_debug': Block(0x0F20, 2, True, (
        F('result__debug_status',                                0, 1, False, None),
        F('result__debug_stage',                                 1, 1, False, None),
    )),
    'gph_general_config': Block(0x0F24, 5, True, (
        F('gph__system__thresh_rate_high',                       0, 2, False, None),
        F('gph__system__thresh_rate_low',                        2, 2, False, None),
        F('gph__system__interrupt_config_gpio',                  4, 1, False, None),
    )),
    'gph_static_config': Block(0x0F2F, 6, True, (
        F('gph__dss_config__roi_mode_control',                   0, 1, False, 0x7),
        F('gph__dss_config__manual_effective_spads_select',      1, 2, False, None),
        F('gph__dss_config__manual_block_select',                3, 1, False, None),
        F('gph__dss_config__max_spads_limit',                    4, 1, False, None),
        F('gph__dss_config__min_spads_limit',                    5, 1, False, None),
    )),
    'gph_timing_config': Block(0x0F36, 16, True, (
        F('gph__mm_config__timeout_macrop_a_hi',                 0, 1, False, 0xF),
        F('gph__mm_config__timeout_macrop_a_lo',                 1, 1, False, None),
        F('gph__mm_config__timeout_macrop_b_hi',                 2, 1, False, 0xF),
        F('gph__mm_config__timeout_macrop_b_lo',                 3, 1, False, None),
        F('gph__range_config__timeout_macrop_a_hi',              4, 1, False, 0xF),
        F('gph__range_config__timeout_macrop_a_lo',              5, 1, False, None),
        F('gph__range_config__vcsel_period_a',                   6, 1, False, 0x3F),
        F('gph__range_config__vcsel_period_b',                   7, 1, False, 0x3F),
        F('gph__range_config__timeout_macrop_b_hi',              8, 1, False, 0xF),
        F('gph__range_config__timeout_macrop_b_lo',              9, 1, False, None),
        F('gph__range_config__sigma_thresh',                    10, 2, False, None),
        F('gph__range_config__min_count_rate_rtn_limit_mcps',   12, 2, False, None),
        F('gph__range_config__valid_phase_low',                 14, 1, False, None),
        F('gph__range_config__valid_phase_high',                15, 1, False, None),
    )),
    'fw_internal': Block(0x0F46, 2, True, (
        F('firmware__internal_stream_count_div',                 0, 1, False, None),
        F('firmware__internal_stream_counter_val',               1, 1, False, None),
    )),
}


def _clip(value, f):
    """Apply the field's mask and width, the way the C codec does."""
    value = int(value)
    bits = f.width * 8
    if f.signed:
        value &= (1 << bits) - 1
    if f.mask is not None:
        value &= f.mask
    return value & ((1 << bits) - 1)


def _unsign(value, f):
    bits = f.width * 8
    if f.signed and value >= (1 << (bits - 1)):
        value -= 1 << bits
    return value


class RegBlock:
    """One register block's worth of device image, addressed by field name.

    Fields not named in the block (the reserved holes) keep whatever bytes were
    read, so a read-modify-write round trip is byte-exact.
    """

    __slots__ = ('_block', '_name', '_v', '_raw')

    def __init__(self, name):
        object.__setattr__(self, '_name', name)
        object.__setattr__(self, '_block', BLOCKS[name])
        object.__setattr__(self, '_v', {f.name: 0 for f in BLOCKS[name].fields})
        object.__setattr__(self, '_raw', bytearray(BLOCKS[name].size))

    # ── attribute access ──
    def __getattr__(self, key):
        try:
            return self._v[key]
        except KeyError:
            raise AttributeError(f'{self._name} has no field {key}') from None

    def __setattr__(self, key, value):
        if key in self._v:
            self._v[key] = int(value)
        else:
            raise AttributeError(f'{self._name} has no field {key}')

    # ── codec ──
    def decode(self, raw: bytes):
        if len(raw) < self._block.size:
            raise ValueError(f'{self._name}: {len(raw)} bytes, '
                             f'need {self._block.size}')
        object.__setattr__(self, '_raw', bytearray(raw[:self._block.size]))
        for f in self._block.fields:
            value = int.from_bytes(raw[f.off:f.off + f.width], 'big')
            if f.mask is not None:
                value &= f.mask
            self._v[f.name] = _unsign(value, f)
        return self

    def encode(self) -> bytes:
        buf = bytearray(self._raw)
        for f in self._block.fields:
            buf[f.off:f.off + f.width] = _clip(self._v[f.name], f).to_bytes(
                f.width, 'big')
        return bytes(buf)

    # ── block geometry ──
    @property
    def base(self):
        return self._block.base

    @property
    def size(self):
        return self._block.size

    def items(self):
        return [(f.name, self._v[f.name]) for f in self._block.fields]

    def __repr__(self):
        return f'<{self._name} 0x{self.base:04X}+{self.size}>'
