//! ST's VL53LX BareDriver ranging path (`driver.py` of the reference).

use std::collections::BTreeMap;
use std::sync::Arc;

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::Vl53lxPlatform;

use super::core::{HistConfig, HistPostProcessConfig, HistogramBinData, LlState};
use super::image::{DeviceImage, RANGE_START_BLOCKS};
use super::math;
use super::nvm::{self, AdditionalOffsetCalData, CalPeakRateMap, OpticalCentre};
use super::regs::*;
use super::tables::TUNING;

// The distance-mode tails (vl53lx_api_preset_modes.c): bin sequence, VCSEL periods, timeouts, phases.
struct Tail {
    even: [i64; 6],
    odd: [i64; 6],
    vcsel_a: i64,
    vcsel_b: i64,
    mm_a: i64,
    mm_b: i64,
    range_a: i64,
    range_b: i64,
    cal_vcsel_start: i64,
    valid_phase_high: i64,
    phase_rtn: &'static str,
    phase_ref: &'static str,
    extra_sequence: i64,
    phasecal_key: &'static str,
}

fn tail(mode: &str) -> Option<Tail> {
    match mode {
        "long" => Some(Tail {
            even: [7, 0, 1, 2, 3, 4],
            odd: [0, 1, 2, 3, 4, 5],
            vcsel_a: 0x09,
            vcsel_b: 0x0B,
            mm_a: 0x0021,
            mm_b: 0x001B,
            range_a: 0x0029,
            range_b: 0x0022,
            cal_vcsel_start: 0x09,
            valid_phase_high: 0x88,
            phase_rtn: "tp_init_phase_rtn_hist_long",
            phase_ref: "tp_init_phase_ref_hist_long",
            extra_sequence: 0,
            phasecal_key: "tp_phasecal_timeout_hist_long_us",
        }),
        "medium" => Some(Tail {
            even: [7, 0, 1, 1, 2, 2],
            odd: [0, 1, 2, 1, 2, 3],
            vcsel_a: 0x05,
            vcsel_b: 0x07,
            mm_a: 0x0036,
            mm_b: 0x0028,
            range_a: 0x0044,
            range_b: 0x0033,
            cal_vcsel_start: 0x05,
            valid_phase_high: 0x48,
            phase_rtn: "tp_init_phase_rtn_hist_med",
            phase_ref: "tp_init_phase_ref_hist_med",
            extra_sequence: 0,
            phasecal_key: "tp_phasecal_timeout_hist_med_us",
        }),
        "short" => Some(Tail {
            even: [7, 7, 0, 1, 1, 1],
            odd: [0, 1, 1, 1, 2, 2],
            vcsel_a: 0x03,
            vcsel_b: 0x05,
            mm_a: 0x0052,
            mm_b: 0x0037,
            range_a: 0x0066,
            range_b: 0x0044,
            cal_vcsel_start: 0x03,
            valid_phase_high: 0x28,
            phase_rtn: "tp_init_phase_rtn_hist_short",
            phase_ref: "tp_init_phase_ref_hist_short",
            extra_sequence: SEQUENCE_MM1_EN,
            phasecal_key: "tp_phasecal_timeout_hist_short_us",
        }),
        _ => None,
    }
}

/// ST's VL53LX BareDriver ranging path, on top of [`DeviceImage`]: `VL53LX_read_p2p_data()`, the
/// histogram preset modes, `VL53LX_set_timeouts_us()`, `VL53LX_init_and_start_range()` and
/// `VL53LX_get_histogram_bin_data()`. Nothing here writes a register on its own: the preset builders
/// edit the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when ranging
/// starts.
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

impl BareDriver {
    /// A driver over `platform`, nothing read yet.
    pub fn new(platform: Arc<dyn Vl53lxPlatform>) -> BareDriver {
        BareDriver {
            p: platform,
            img: DeviceImage::new(),
            tuning: TUNING.iter().copied().collect(),
            hist_cfg: HistConfig::default(),
            hpp: HistPostProcessConfig::default(),
            state: LlState::default(),
            ambient_events_sum: 0,
            rtn_good_spads_map: [0; 32],
            optical_centre: None,
            cal_peak_rate_map: None,
            add_off_cal_data: None,
            fmt_dmax_cal: Vec::new(),
            mm_roi: Vec::new(),
            result_osc_calibrate_val: 0,
            preset_mode: "medium",
            phasecal_config_timeout_us: 1000,
            mm_config_timeout_us: 2000,
            range_config_timeout_us: 13000,
            inter_measurement_period_ms: 100,
            measurement_mode: 0,
        }
    }

    /// The host-side register image.
    pub fn img(&self) -> &DeviceImage {
        &self.img
    }

    /// The host-side register image, to edit.
    pub fn img_mut(&mut self) -> &mut DeviceImage {
        &mut self.img
    }

    /// The tuning parameters, a mutable copy of [`TUNING`].
    pub fn tuning(&self) -> &BTreeMap<&'static str, i64> {
        &self.tuning
    }

    /// The tuning parameters, to edit (takes effect at the next preset).
    pub fn tuning_mut(&mut self) -> &mut BTreeMap<&'static str, i64> {
        &mut self.tuning
    }

    /// The histogram (bin sequence) configuration.
    pub fn hist_cfg(&self) -> &HistConfig {
        &self.hist_cfg
    }

    /// The post-processing configuration.
    pub fn hpp(&self) -> &HistPostProcessConfig {
        &self.hpp
    }

    /// The post-processing configuration, to edit.
    pub fn hpp_mut(&mut self) -> &mut HistPostProcessConfig {
        &mut self.hpp
    }

    /// The ll-driver state machine.
    pub fn state(&self) -> &LlState {
        &self.state
    }

    /// The ll-driver state machine, to step.
    pub fn state_mut(&mut self) -> &mut LlState {
        &mut self.state
    }

    /// The good return SPADs, one bit each.
    pub fn rtn_good_spads_map(&self) -> &[u8; 32] {
        &self.rtn_good_spads_map
    }

    /// The optical centre (after [`BareDriver::read_p2p_data`]).
    pub fn optical_centre(&self) -> Option<OpticalCentre> {
        self.optical_centre
    }

    /// The calibration peak-rate map (after [`BareDriver::read_p2p_data`]).
    pub fn cal_peak_rate_map(&self) -> Option<&CalPeakRateMap> {
        self.cal_peak_rate_map.as_ref()
    }

    /// The mode-mitigation offset calibration, or ST's fallback (after [`BareDriver::read_p2p_data`]).
    pub fn add_off_cal_data(&self) -> Option<AdditionalOffsetCalData> {
        self.add_off_cal_data
    }

    /// The dmax calibration inputs, by the reference's key.
    pub fn fmt_dmax_cal(&self) -> &[(&'static str, i64)] {
        &self.fmt_dmax_cal
    }

    /// The mode-mitigation ROI: x_centre, y_centre, width, height.
    pub fn mm_roi(&self) -> &[(&'static str, i64)] {
        &self.mm_roi
    }

    /// `result__osc_calibrate_val`.
    pub fn result_osc_calibrate_val(&self) -> i64 {
        self.result_osc_calibrate_val
    }

    /// The distance mode of the last preset.
    pub fn preset_mode(&self) -> &'static str {
        self.preset_mode
    }

    /// The phasecal timeout, µs.
    pub fn phasecal_config_timeout_us(&self) -> i64 {
        self.phasecal_config_timeout_us
    }

    /// The MM timeout, µs.
    pub fn mm_config_timeout_us(&self) -> i64 {
        self.mm_config_timeout_us
    }

    /// The range timeout, µs (how long one range actually runs).
    pub fn range_config_timeout_us(&self) -> i64 {
        self.range_config_timeout_us
    }

    /// The inter-measurement period, ms.
    pub fn inter_measurement_period_ms(&self) -> i64 {
        self.inter_measurement_period_ms
    }

    /// The measurement mode of the last start.
    pub fn measurement_mode(&self) -> i64 {
        self.measurement_mode
    }

    fn tp(&self, name: &str) -> i64 {
        *self.tuning.get(name).unwrap_or_else(|| panic!("no tuning parameter {name}"))
    }

    /// `VL53LX_read_p2p_data()`: three register blocks and four NVM regions (~0.1 s over USB).
    pub fn read_p2p_data(&mut self) -> Result<()> {
        let p = self.p.clone();
        let p = p.as_ref();
        self.img.pull(p, "static_nvm_managed")?;
        self.img.pull(p, "customer_nvm_managed")?;
        self.img.pull(p, "nvm_copy_data")?;
        self.rtn_good_spads_map = math::rtn_good_spads(self.img.nvm_copy_data());

        let customer = self.img.customer_nvm();
        self.hpp.crosstalk_compensation_plane_offset_kcps =
            customer.get("algo__crosstalk_compensation_plane_offset_kcps");
        self.hpp.crosstalk_compensation_x_plane_gradient_kcps =
            customer.get("algo__crosstalk_compensation_x_plane_gradient_kcps");
        self.hpp.crosstalk_compensation_y_plane_gradient_kcps =
            customer.get("algo__crosstalk_compensation_y_plane_gradient_kcps");

        let mut optical_centre = nvm::read_optical_centre(p, &mut self.img)?;
        let peak_map = nvm::read_cal_peak_rate_map(p, &mut self.img)?;
        let mut cal = nvm::read_additional_offset_cal_data(p, &mut self.img)?;

        // Our boards have no FMT offset calibration. ST's fallback: nominal MM1/MM2 peak rates, and the
        // effective SPAD counts worked out here.
        if cal.mm_inner_peak_signal_count_rtn_mcps == 0 && cal.mm_outer_peak_signal_count_rtn_mcps == 0 {
            let (inner, outer) = math::calc_mm_effective_spads(
                self.img.nvm_copy_data().get("roi_config__mode_roi_centre_spad"),
                self.img.nvm_copy_data().get("roi_config__mode_roi_xy_size"),
                0xC7,
                0xFF,
                &self.rtn_good_spads_map,
                RTN_SPAD_APERTURE_TRANSMISSION,
            );
            cal = AdditionalOffsetCalData {
                mm_inner_actual_effective_spads: inner,
                mm_outer_actual_effective_spads: outer,
                mm_inner_peak_signal_count_rtn_mcps: 0x0080,
                mm_outer_peak_signal_count_rtn_mcps: 0x0180,
            };
        }
        self.add_off_cal_data = Some(cal);

        let fmt = nvm::read_fmt_range_results(p, &mut self.img, nvm::FMT_RANGE_RESULTS_140MM_DARK)?;
        self.fmt_dmax_cal = vec![
            ("ref__actual_effective_spads", fmt.actual_effective_rtn_spads),
            ("ref__peak_signal_count_rate_mcps", fmt.peak_signal_count_rate_rtn_mcps),
            ("ref__distance_mm", fmt.measured_distance_mm),
            (
                "ref_reflectance_pc",
                if peak_map.cal_reflectance_pc != 0 { peak_map.cal_reflectance_pc } else { 0x0014 },
            ),
            ("coverglass_transmission", 0x0100),
        ];
        self.cal_peak_rate_map = Some(peak_map);

        self.result_osc_calibrate_val = p.read_word(RESULT_OSC_CALIBRATE_VAL)? as i64;

        if self.img.static_nvm().get("osc_measured__fast_osc__frequency") < 0x1000 {
            self.img.static_nvm_mut().set("osc_measured__fast_osc__frequency", 0xBCCC);
        }

        // VL53LX_get_mode_mitigation_roi
        let (y, x) = math::decode_row_col(self.img.nvm_copy_data().get("roi_config__mode_roi_centre_spad"));
        let xy_size = self.img.nvm_copy_data().get("roi_config__mode_roi_xy_size");
        self.mm_roi = vec![("x_centre", x), ("y_centre", y), ("width", xy_size & 0x0F), ("height", xy_size >> 4)];

        if optical_centre.x_centre == 0 && optical_centre.y_centre == 0 {
            optical_centre = OpticalCentre { x_centre: x << 4, y_centre: y << 4 };
        }
        self.optical_centre = Some(optical_centre);
        Ok(())
    }

    /// `VL53LX_data_init` + `VL53LX_DataInit`: the factory data, the default preset mode and timing
    /// budget. Leaves the sensor untouched apart from the reads.
    pub fn data_init(&mut self) -> Result<()> {
        self.state.reset();
        self.hpp = HistPostProcessConfig::default();
        self.read_p2p_data()?;
        self.set_preset_mode_with("medium", None, Some(1000))?;
        self.set_measurement_timing_budget_us(33333)
    }

    /// `VL53LX_set_preset_mode()` for the three histogram ranging modes, by distance mode.
    pub fn set_preset_mode(&mut self, mode: &str) -> Result<()> {
        self.set_preset_mode_with(mode, None, None)
    }

    /// `VL53LX_set_preset_mode()` for the three histogram ranging modes, by distance mode; a `None`
    /// DSS target or period keeps the tuning default / the current period.
    pub fn set_preset_mode_with(
        &mut self,
        mode: &str,
        dss_target_mcps: Option<i64>,
        inter_measurement_period_ms: Option<i64>,
    ) -> Result<()> {
        let Some(t) = tail(mode) else {
            return Err(DepzError::InvalidArgument(format!("unknown distance mode '{mode}'")));
        };
        self.preset_mode = DISTANCE_MODES.iter().find(|m| **m == mode).copied().unwrap_or("medium");
        let dss = dss_target_mcps.unwrap_or_else(|| self.tp("tp_dss_target_histo_mcps"));
        let period = inter_measurement_period_ms.unwrap_or(self.inter_measurement_period_ms);

        // VL53LX_get_preset_mode_timing_cfg: only the phasecal timeout differs between the three modes.
        let phasecal_us = self.tp(t.phasecal_key);
        let mm_us = self.tp("tp_mm_timeout_histo_us");
        let range_us = self.tp("tp_range_timeout_histo_us");

        self.state.reset();
        self.preset_mode_histogram_ranging();
        self.preset_tail(&t);

        self.img.static_config_mut().set("dss_config__target_total_rate_mcps", dss);
        self.set_timeouts_us(phasecal_us, mm_us, range_us)?;
        self.set_inter_measurement_period_ms(period)?;
        self.update_range_offset();
        Ok(())
    }

    // get_device_results() recomputes hpp.range_offset_mm every frame under MM1_MM2_OFFSETS; its inputs
    // are the factory data and the user ROI, so once per preset is the same answer.
    fn update_range_offset(&mut self) {
        let Some(cal) = self.add_off_cal_data else { return };
        let customer = self.img.customer_nvm();
        let nvm_data = self.img.nvm_copy_data();
        let dynamic = self.img.dynamic_config();
        self.hpp.range_offset_mm = math::combine_mm1_mm2_offsets(
            customer.get("mm_config__inner_offset_mm"),
            customer.get("mm_config__outer_offset_mm"),
            nvm_data.get("roi_config__mode_roi_centre_spad"),
            nvm_data.get("roi_config__mode_roi_xy_size"),
            dynamic.get("roi_config__user_roi_centre_spad"),
            dynamic.get("roi_config__user_roi_requested_global_xy_size"),
            &cal,
            &self.rtn_good_spads_map,
            self.img.general_config().get("dss_config__aperture_attenuation"),
        );
    }

    /// `IsL4()` of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED.
    pub fn is_l4(&self) -> bool {
        let n = self.img.nvm_copy_data();
        let model = n.get("identification__model_id");
        n.get("identification__module_type") == 0xAA && (model == 0xEB || model == 0xEC)
    }

    /// The distance modes this die offers.
    pub fn distance_modes_for_die(&self) -> &'static [&'static str] {
        if self.is_l4() {
            L4_DISTANCE_MODES
        } else {
            DISTANCE_MODES
        }
    }

    /// The budget ceiling of this die, µs.
    pub fn fda_max_timing_budget_us(&self) -> i64 {
        if self.is_l4() {
            L4_FDA_MAX_TIMING_BUDGET_US
        } else {
            FDA_MAX_TIMING_BUDGET_US
        }
    }

    /// `VL53LX_SetDistanceMode()`: the preset mode with the caller's timeouts put back. Short is
    /// refused on an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong
    /// side of the wrap).
    pub fn set_distance_mode(&mut self, mode: &str) -> Result<()> {
        if !self.distance_modes_for_die().contains(&mode) {
            return Err(DepzError::InvalidArgument(format!("distance mode {mode} not available on this die")));
        }
        let phasecal_us = self.phasecal_config_timeout_us;
        let mm_us = self.mm_config_timeout_us;
        let range_us = self.range_config_timeout_us;
        self.set_preset_mode(mode)?;
        self.set_timeouts_us(phasecal_us, mm_us, range_us)
    }

    // VL53LX_preset_mode_standard_ranging(): the base every histogram mode is built on.
    fn preset_mode_standard_ranging(&mut self) {
        let tp = |n: &str| self.tp(n);
        let (pw, aw, sref, clip, tol) = (
            tp("tp_lite_sigma_est_pulse_width_ns"),
            tp("tp_lite_sigma_est_amb_width_ns"),
            tp("tp_lite_sigma_ref_mm"),
            tp("tp_lite_min_clip"),
            tp("tp_consistency_lite_phase_tolerance"),
        );
        let (repeat, target) = (tp("tp_cal_repeat_rate"), tp("tp_phasecal_target"));
        let (sigma_thresh, min_rate) = (tp("tp_lite_med_sigma_thresh_mm"), tp("tp_lite_med_min_count_rate_rtn_mcps"));
        let (seed, phase_rtn, phase_ref, first_order, quantifier) = (
            tp("tp_lite_seed_cfg"),
            tp("tp_init_phase_rtn_lite_med"),
            tp("tp_init_phase_ref_lite_med"),
            tp("tp_lite_first_order_select"),
            tp("tp_lite_quantifier"),
        );

        let stat = self.img.static_config_mut();
        stat.set("dss_config__target_total_rate_mcps", 0x0A00);
        stat.set("debug__ctrl", 0x00);
        stat.set("test_mode__ctrl", 0x00);
        stat.set("clk_gating__ctrl", 0x00);
        stat.set("nvm_bist__ctrl", 0x00);
        stat.set("nvm_bist__num_nvm_words", 0x00);
        stat.set("nvm_bist__start_address", 0x00);
        stat.set("host_if__status", 0x00);
        stat.set("pad_i2c_hv__config", 0x00);
        stat.set("pad_i2c_hv__extsup_config", 0x00);
        stat.set("gpio_hv_pad__ctrl", 0x00);
        stat.set(
            "gpio_hv_mux__ctrl",
            DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW | DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS,
        );
        stat.set("gpio__tio_hv_status", 0x02);
        stat.set("gpio__fio_hv_status", 0x00);
        stat.set("ana_config__spad_sel_pswidth", 0x02);
        stat.set("ana_config__vcsel_pulse_width_offset", 0x08);
        stat.set("ana_config__fast_osc__config_ctrl", 0x00);
        stat.set("sigma_estimator__effective_pulse_width_ns", pw);
        stat.set("sigma_estimator__effective_ambient_width_ns", aw);
        stat.set("sigma_estimator__sigma_ref_mm", sref);
        stat.set("algo__crosstalk_compensation_valid_height_mm", 0x01);
        stat.set("spare_host_config__static_config_spare_0", 0x00);
        stat.set("spare_host_config__static_config_spare_1", 0x00);
        stat.set("algo__range_ignore_threshold_mcps", 0x0000);
        stat.set("algo__range_ignore_valid_height_mm", 0xFF);
        stat.set("algo__range_min_clip", clip);
        stat.set("algo__consistency_check__tolerance", tol);
        stat.set("spare_host_config__static_config_spare_2", 0x00);
        stat.set("sd_config__reset_stages_msb", 0x00);
        stat.set("sd_config__reset_stages_lsb", 0x00);

        let general = self.img.general_config_mut();
        general.set("gph_config__stream_count_update_value", 0x00);
        general.set("global_config__stream_divider", 0x00);
        general.set("system__interrupt_config_gpio", INTERRUPT_CONFIG_NEW_SAMPLE_READY);
        general.set("cal_config__vcsel_start", 0x0B);
        general.set("cal_config__repeat_rate", repeat);
        general.set("global_config__vcsel_width", 0x02);
        general.set("phasecal_config__timeout_macrop", 0x0D);
        general.set("phasecal_config__target", target);
        general.set("phasecal_config__override", 0x00);
        general.set("dss_config__roi_mode_control", DEVICE_DSS_MODE_TARGET_RATE);
        general.set("system__thresh_rate_high", 0x0000);
        general.set("system__thresh_rate_low", 0x0000);
        general.set("dss_config__manual_effective_spads_select", 0x8C00);
        general.set("dss_config__manual_block_select", 0x00);
        general.set("dss_config__aperture_attenuation", 0x38);
        general.set("dss_config__max_spads_limit", 0xFF);
        general.set("dss_config__min_spads_limit", 0x01);

        let timing = self.img.timing_config_mut();
        timing.set("mm_config__timeout_macrop_a_hi", 0x00);
        timing.set("mm_config__timeout_macrop_a_lo", 0x1A);
        timing.set("mm_config__timeout_macrop_b_hi", 0x00);
        timing.set("mm_config__timeout_macrop_b_lo", 0x20);
        timing.set("range_config__timeout_macrop_a_hi", 0x01);
        timing.set("range_config__timeout_macrop_a_lo", 0xCC);
        timing.set("range_config__vcsel_period_a", 0x0B);
        timing.set("range_config__timeout_macrop_b_hi", 0x01);
        timing.set("range_config__timeout_macrop_b_lo", 0xF5);
        timing.set("range_config__vcsel_period_b", 0x09);
        timing.set("range_config__sigma_thresh", sigma_thresh);
        timing.set("range_config__min_count_rate_rtn_limit_mcps", min_rate);
        timing.set("range_config__valid_phase_low", 0x08);
        timing.set("range_config__valid_phase_high", 0x78);
        timing.set("system__intermeasurement_period", 0x0000_0000);
        timing.set("system__fractional_enable", 0x00);

        // The struct's initial state; every histogram mode overwrites it.
        self.hist_cfg = HistConfig {
            low_amb_even_bin_0_1: 0x07,
            low_amb_even_bin_2_3: 0x21,
            low_amb_even_bin_4_5: 0x43,
            low_amb_odd_bin_0_1: 0x10,
            low_amb_odd_bin_2_3: 0x32,
            low_amb_odd_bin_4_5: 0x54,
            mid_amb_even_bin_0_1: 0x07,
            mid_amb_even_bin_2_3: 0x21,
            mid_amb_even_bin_4_5: 0x43,
            mid_amb_odd_bin_0_1: 0x10,
            mid_amb_odd_bin_2: 0x02,
            mid_amb_odd_bin_3_4: 0x43,
            mid_amb_odd_bin_5: 0x05,
            user_bin_offset: 0x00,
            high_amb_even_bin_0_1: 0x07,
            high_amb_even_bin_2_3: 0x21,
            high_amb_even_bin_4_5: 0x43,
            high_amb_odd_bin_0_1: 0x10,
            high_amb_odd_bin_2_3: 0x32,
            high_amb_odd_bin_4_5: 0x54,
            amb_thresh_low: 0xFFFF,
            amb_thresh_high: 0xFFFF,
            spad_array_selection: 0x00,
        };

        let dynamic = self.img.dynamic_config_mut();
        dynamic.set("system__grouped_parameter_hold_0", 0x01);
        dynamic.set("system__thresh_high", 0x0000);
        dynamic.set("system__thresh_low", 0x0000);
        dynamic.set("system__enable_xtalk_per_quadrant", 0x00);
        dynamic.set("system__seed_config", seed);
        dynamic.set("sd_config__woi_sd0", 0x0B);
        dynamic.set("sd_config__woi_sd1", 0x09);
        dynamic.set("sd_config__initial_phase_sd0", phase_rtn);
        dynamic.set("sd_config__initial_phase_sd1", phase_ref);
        dynamic.set("system__grouped_parameter_hold_1", 0x01);
        dynamic.set("sd_config__first_order_select", first_order);
        dynamic.set("sd_config__quantifier", quantifier);
        dynamic.set("roi_config__user_roi_centre_spad", 0xC7);
        dynamic.set("roi_config__user_roi_requested_global_xy_size", 0xFF);
        dynamic.set(
            "system__sequence_config",
            SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN | SEQUENCE_DSS2_EN | SEQUENCE_MM2_EN | SEQUENCE_RANGE_EN,
        );
        dynamic.set("system__grouped_parameter_hold", 0x02);

        let system = self.img.system_control_mut();
        system.set("system__stream_count_ctrl", 0x00);
        system.set("firmware__enable", 0x01);
        system.set("system__interrupt_clear", CLEAR_RANGE_INT as i64);
        system.set(
            "system__mode_start",
            DEVICE_SCHEDULER_MODE_STREAMING | DEVICE_READOUT_MODE_SINGLE_SD | DEVICE_MEASUREMENT_MODE_BACK_TO_BACK,
        );
    }

    // VL53LX_preset_mode_histogram_ranging(): standard ranging, then the histogram engine.
    fn preset_mode_histogram_ranging(&mut self) {
        self.preset_mode_standard_ranging();

        self.img.static_config_mut().set("dss_config__target_total_rate_mcps", 0x1400);
        self.hist_cfg.set_bin_sequence(&[7, 0, 1, 2, 3, 4], &[0, 1, 2, 3, 4, 5]);

        let timing = self.img.timing_config_mut();
        timing.set("range_config__vcsel_period_a", 0x09);
        timing.set("range_config__vcsel_period_b", 0x0B);
        timing.set("mm_config__timeout_macrop_a_hi", 0x00);
        timing.set("mm_config__timeout_macrop_a_lo", 0x20);
        timing.set("mm_config__timeout_macrop_b_hi", 0x00);
        timing.set("mm_config__timeout_macrop_b_lo", 0x1A);
        timing.set("range_config__timeout_macrop_a_hi", 0x00);
        timing.set("range_config__timeout_macrop_a_lo", 0x28);
        timing.set("range_config__timeout_macrop_b_hi", 0x00);
        timing.set("range_config__timeout_macrop_b_lo", 0x21);
        let dynamic = self.img.dynamic_config_mut();
        dynamic.set("sd_config__woi_sd0", 0x09);
        dynamic.set("sd_config__woi_sd1", 0x0B);
        self.img.general_config_mut().set("phasecal_config__timeout_macrop", 0xF5);

        self.hpp.valid_phase_low = 0x08;
        self.hpp.valid_phase_high = 0x88;

        self.hist_cfg.copy_to_static_cfg(&mut self.img);
        self.img.dynamic_config_mut().set(
            "system__sequence_config",
            SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN | SEQUENCE_DSS2_EN | SEQUENCE_RANGE_EN,
        );
        self.img.system_control_mut().set(
            "system__mode_start",
            DEVICE_SCHEDULER_MODE_HISTOGRAM | DEVICE_READOUT_MODE_DUAL_SD | DEVICE_MEASUREMENT_MODE_BACK_TO_BACK,
        );
    }

    // The shape all three distance modes share: a bin sequence, a VCSEL period pair and its timeouts.
    fn preset_tail(&mut self, t: &Tail) {
        self.hist_cfg.set_bin_sequence(&t.even, &t.odd);
        self.hist_cfg.copy_to_static_cfg(&mut self.img);
        let (phase_rtn, phase_ref) = (self.tp(t.phase_rtn), self.tp(t.phase_ref));

        let timing = self.img.timing_config_mut();
        timing.set("range_config__vcsel_period_a", t.vcsel_a);
        timing.set("range_config__vcsel_period_b", t.vcsel_b);
        timing.set("mm_config__timeout_macrop_a_hi", t.mm_a >> 8);
        timing.set("mm_config__timeout_macrop_a_lo", t.mm_a & 0xFF);
        timing.set("mm_config__timeout_macrop_b_hi", t.mm_b >> 8);
        timing.set("mm_config__timeout_macrop_b_lo", t.mm_b & 0xFF);
        timing.set("range_config__timeout_macrop_a_hi", t.range_a >> 8);
        timing.set("range_config__timeout_macrop_a_lo", t.range_a & 0xFF);
        timing.set("range_config__timeout_macrop_b_hi", t.range_b >> 8);
        timing.set("range_config__timeout_macrop_b_lo", t.range_b & 0xFF);

        let general = self.img.general_config_mut();
        general.set("cal_config__vcsel_start", t.cal_vcsel_start);
        general.set("phasecal_config__timeout_macrop", 0xF5);

        let dynamic = self.img.dynamic_config_mut();
        dynamic.set("sd_config__woi_sd0", t.vcsel_a);
        dynamic.set("sd_config__woi_sd1", t.vcsel_b);
        dynamic.set("sd_config__initial_phase_sd0", phase_rtn);
        dynamic.set("sd_config__initial_phase_sd1", phase_ref);
        dynamic.set(
            "system__sequence_config",
            SEQUENCE_VHV_EN
                | SEQUENCE_PHASECAL_EN
                | SEQUENCE_DSS1_EN
                | SEQUENCE_DSS2_EN
                | t.extra_sequence
                | SEQUENCE_RANGE_EN,
        );

        self.hpp.valid_phase_low = 0x08;
        self.hpp.valid_phase_high = t.valid_phase_high;

        self.img.system_control_mut().set(
            "system__mode_start",
            DEVICE_SCHEDULER_MODE_HISTOGRAM | DEVICE_READOUT_MODE_DUAL_SD | DEVICE_MEASUREMENT_MODE_BACK_TO_BACK,
        );
    }

    /// `VL53LX_set_timeouts_us()`: the three timeouts into the timing registers.
    pub fn set_timeouts_us(&mut self, phasecal_us: i64, mm_us: i64, range_us: i64) -> Result<()> {
        self.phasecal_config_timeout_us = phasecal_us;
        self.mm_config_timeout_us = mm_us;
        self.range_config_timeout_us = range_us;
        let freq = self.img.static_nvm().get("osc_measured__fast_osc__frequency");
        math::calc_timeout_register_values(phasecal_us, mm_us, range_us, freq, &mut self.img)
            .ok_or_else(|| DepzError::Sensor("osc_measured__fast_osc__frequency is 0".into()))
    }

    /// The period between two measurements, ms (0: back to back).
    pub fn set_inter_measurement_period_ms(&mut self, period_ms: i64) -> Result<()> {
        if self.result_osc_calibrate_val == 0 {
            return Err(DepzError::Sensor("result__osc_calibrate_val is 0".into()));
        }
        self.inter_measurement_period_ms = period_ms;
        let v = period_ms * self.result_osc_calibrate_val;
        self.img.timing_config_mut().set("system__intermeasurement_period", v);
        Ok(())
    }

    /// `VL53LX_SetMeasurementTimingBudgetMicroSeconds()`: the range timeout is one sixth of the budget
    /// after a fixed guard comes off.
    pub fn set_measurement_timing_budget_us(&mut self, budget_us: i64) -> Result<()> {
        if !(TIMING_GUARD_US < budget_us && budget_us <= 10_000_000) {
            return Err(DepzError::InvalidArgument(format!("timing budget {budget_us} us out of range")));
        }
        let range_us = (budget_us - TIMING_GUARD_US) / TIMING_DIVISOR;
        if range_us * TIMING_DIVISOR > self.fda_max_timing_budget_us() {
            return Err(DepzError::InvalidArgument(format!("timing budget {budget_us} us out of range")));
        }
        self.set_timeouts_us(self.phasecal_config_timeout_us, self.mm_config_timeout_us, range_us)
    }

    /// `VL53LX_load_patch()`: a six-instruction phasecal patch, written with the firmware stopped.
    pub fn load_patch(&mut self) -> Result<()> {
        let power: u8 = match self.tp("tp_phasecal_patch_power") {
            1 => 0x10,
            2 => 0x20,
            3 => 0x40,
            _ => 0x00,
        };
        let p = self.p.as_ref();
        p.write_byte(FIRMWARE_ENABLE, 0x00)?;
        p.write_byte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01)?;
        p.sleep_ms(1); // 250 us settling, rounded up
        p.write_multi(PATCH_OFFSET_0, &[0x29, 0xC9, 0x0E, 0x40, 0x28, power])?;
        p.write_multi(PATCH_ADDRESS_0, &[0x03, 0x6D, 0x03, 0x6F, 0x07, 0x29])?;
        p.write_multi(PATCH_JMP_ENABLES, &[0x00, 0x07])?;
        p.write_multi(PATCH_DATA_ENABLES, &[0x00, 0x07])?;
        p.write_byte(PATCH_CTRL, 0x01)?;
        p.write_byte(FIRMWARE_ENABLE, 0x01)
    }

    /// `VL53LX_unload_patch()`.
    pub fn unload_patch(&mut self) -> Result<()> {
        let p = self.p.as_ref();
        p.write_byte(FIRMWARE_ENABLE, 0x00)?;
        p.write_byte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00)?;
        p.write_byte(PATCH_CTRL, 0x00)?;
        p.write_byte(FIRMWARE_ENABLE, 0x01)
    }

    /// `VL53LX_init_and_start_range()` with every block: write the whole configuration and start.
    pub fn init_and_start_range(&mut self) -> Result<()> {
        self.init_and_start_range_blocks(RANGE_START_BLOCKS)
    }

    /// `VL53LX_init_and_start_range()`: write the configuration and start. `blocks` is ST's config
    /// level — all of them to start, general_config onwards to let the next frame run.
    pub fn init_and_start_range_blocks(&mut self, blocks: &[&str]) -> Result<()> {
        self.measurement_mode = DEVICE_MEASUREMENT_MODE_BACK_TO_BACK;
        let system = self.img.system_control_mut();
        let mode_start = (system.get("system__mode_start") & DEVICE_MEASUREMENT_MODE_STOP_MASK) | DEVICE_MEASUREMENT_MODE_BACK_TO_BACK;
        system.set("system__mode_start", mode_start);
        system.set("system__interrupt_clear", CLEAR_RANGE_INT as i64);

        let gph_id = self.state.cfg_gph_id;
        let dynamic = self.img.dynamic_config_mut();
        dynamic.set("system__grouped_parameter_hold_0", gph_id | 0x01);
        dynamic.set("system__grouped_parameter_hold_1", gph_id | 0x01);
        dynamic.set("system__grouped_parameter_hold", gph_id);

        self.img.push_range(self.p.as_ref(), blocks)?;

        let gph = self.img.dynamic_config().get("system__grouped_parameter_hold");
        self.state.update_rd(mode_start, gph);
        self.state.update_cfg(mode_start);
        Ok(())
    }

    /// `VL53LX_clear_interrupt_and_enable_next_range()`: in histogram mode the interrupt is cleared by
    /// rewriting general_config onwards.
    pub fn clear_interrupt_and_start_next_range(&mut self) -> Result<()> {
        self.init_and_start_range_blocks(&["general_config", "timing_config", "dynamic_config", "system_control"])
    }

    /// `VL53LX_stop_range()`.
    pub fn stop_range(&mut self) -> Result<()> {
        let system = self.img.system_control_mut();
        let v = system.get("system__mode_start");
        system.set("system__mode_start", (v & DEVICE_MEASUREMENT_MODE_STOP_MASK) | DEVICE_MEASUREMENT_MODE_ABORT);
        self.img.push(self.p.as_ref(), "system_control")?;
        let system = self.img.system_control_mut();
        let v = system.get("system__mode_start");
        system.set("system__mode_start", v & DEVICE_MEASUREMENT_MODE_STOP_MASK);
        self.state.reset();
        Ok(())
    }

    /// `VL53LX_get_histogram_bin_data()`: one frame, 83 bytes, decoded into bins plus the metadata the
    /// post-processing needs. `raw` is bytes that arrived some other way (a stream report); `None`
    /// reads them here.
    pub fn get_histogram_bin_data(&mut self, raw: Option<&[u8]>) -> Result<HistogramBinData> {
        let size = HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES as usize;
        let read;
        let raw = match raw {
            Some(r) => r,
            None => {
                read = self.p.read_multi(HISTOGRAM_BIN_DATA_I2C_INDEX, size)?;
                &read[..]
            }
        };
        if raw.len() < size {
            return Err(DepzError::InvalidArgument(format!(
                "histogram frame is {} bytes, need {}",
                raw.len(),
                size
            )));
        }
        let mut buf: Vec<i64> = raw.iter().map(|&b| b as i64).collect();
        let at = |a: u16| (a - HISTOGRAM_BIN_DATA_I2C_INDEX) as usize;

        let mut d = HistogramBinData {
            result_interrupt_status: buf[0],
            result_range_status: buf[1],
            result_report_status: buf[2],
            result_stream_count: buf[3],
            result_dss_actual_effective_spads: (buf[4] << 8) | buf[5],
            phasecal_result_reference_phase: (buf[at(PHASECAL_RESULT_REFERENCE_PHASE)] << 8)
                | buf[at(PHASECAL_RESULT_REFERENCE_PHASE) + 1],
            phasecal_result_vcsel_start: buf[at(PHASECAL_RESULT_VCSEL_START)],
            ..HistogramBinData::default()
        };

        // Bin 23 does not fit in its three bytes: its low byte comes in a separate MSB/LSB pair.
        buf[at(RESULT_HISTOGRAM_BIN_23_0)] =
            ((buf[at(RESULT_HISTOGRAM_BIN_23_0_MSB)] << 2) + buf[at(RESULT_HISTOGRAM_BIN_23_0_LSB)]) & 0xFF;
        let b0 = at(RESULT_HISTOGRAM_BIN_0_2);
        for i in 0..HISTOGRAM_BUFFER_SIZE {
            d.bin_data[i] = (buf[b0 + 3 * i] << 16) | (buf[b0 + 3 * i + 1] << 8) | buf[b0 + 3 * i + 2];
        }

        let general = self.img.general_config();
        let stat = self.img.static_config();
        let timing = self.img.timing_config();
        let dynamic = self.img.dynamic_config();

        d.cal_config_vcsel_start = general.get("cal_config__vcsel_start");
        d.vcsel_width =
            (general.get("global_config__vcsel_width") << 4) + stat.get("ana_config__vcsel_pulse_width_offset");
        d.fast_osc_frequency = self.img.static_nvm().get("osc_measured__fast_osc__frequency");
        d.roi_config_user_roi_centre_spad = dynamic.get("roi_config__user_roi_centre_spad");
        d.roi_config_user_roi_requested_global_xy_size = dynamic.get("roi_config__user_roi_requested_global_xy_size");

        d.zone_id = 0;
        d.bin_seq = self.hist_cfg.bin_sequence(self.state.rd_stream_count, self.ambient_events_sum);

        let encoded = if self.state.rd_timing_status == 0 {
            d.vcsel_period = timing.get("range_config__vcsel_period_a");
            (timing.get("range_config__timeout_macrop_a_hi") << 8) + timing.get("range_config__timeout_macrop_a_lo")
        } else {
            d.vcsel_period = timing.get("range_config__vcsel_period_b");
            (timing.get("range_config__timeout_macrop_b_hi") << 8) + timing.get("range_config__timeout_macrop_b_lo")
        };

        d.number_of_ambient_bins = 4 * d.bin_seq.iter().filter(|&&s| s & 0x07 == 0x07).count() as i64;
        d.total_periods_elapsed = math::decode_timeout(encoded);
        d.peak_duration_us = math::duration_maths(
            math::calc_pll_period_us(d.fast_osc_frequency),
            d.vcsel_width,
            RANGING_WINDOW_VCSEL_PERIODS,
            d.total_periods_elapsed.wrapping_add(1),
        );
        d.woi_duration_us = 0;

        super::hist::calc_zero_distance_phase(&mut d);
        super::hist::estimate_ambient_from_ambient_bins(&mut d);
        self.ambient_events_sum = d.ambient_events_sum;
        Ok(d)
    }
}

