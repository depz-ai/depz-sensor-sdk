//! The BareDriver's structs (`core.py` of the reference): the histogram configuration, the
//! post-processing configuration, one frame of bins, the ll-driver state and the range results.

use crate::json::Value;

use super::image::{header_default, DeviceImage};
use super::regs;

/// `VL53LX_histogram_config_t`. It has no registers of its own: the bin sequence is smuggled into
/// static_config and timing_config fields that mean something else in lite mode
/// ([`HistConfig::copy_to_static_cfg`]).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
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

impl HistConfig {
    /// `VL53LX_init_histogram_config_structure()`: six even and six odd bin codes, packed two to a
    /// byte, repeated across the three ambient levels.
    pub fn set_bin_sequence(&mut self, even: &[i64; 6], odd: &[i64; 6]) {
        self.low_amb_even_bin_0_1 = (even[1] << 4) + even[0];
        self.low_amb_even_bin_2_3 = (even[3] << 4) + even[2];
        self.low_amb_even_bin_4_5 = (even[5] << 4) + even[4];
        self.low_amb_odd_bin_0_1 = (odd[1] << 4) + odd[0];
        self.low_amb_odd_bin_2_3 = (odd[3] << 4) + odd[2];
        self.low_amb_odd_bin_4_5 = (odd[5] << 4) + odd[4];

        self.mid_amb_even_bin_0_1 = self.low_amb_even_bin_0_1;
        self.mid_amb_even_bin_2_3 = self.low_amb_even_bin_2_3;
        self.mid_amb_even_bin_4_5 = self.low_amb_even_bin_4_5;
        self.mid_amb_odd_bin_0_1 = self.low_amb_odd_bin_0_1;
        self.mid_amb_odd_bin_2 = odd[2];
        self.mid_amb_odd_bin_3_4 = (odd[4] << 4) + odd[3];
        self.mid_amb_odd_bin_5 = odd[5];
        self.user_bin_offset = 0x00;

        self.high_amb_even_bin_0_1 = self.low_amb_even_bin_0_1;
        self.high_amb_even_bin_2_3 = self.low_amb_even_bin_2_3;
        self.high_amb_even_bin_4_5 = self.low_amb_even_bin_4_5;
        self.high_amb_odd_bin_0_1 = self.low_amb_odd_bin_0_1;
        self.high_amb_odd_bin_2_3 = self.low_amb_odd_bin_2_3;
        self.high_amb_odd_bin_4_5 = self.low_amb_odd_bin_4_5;

        self.amb_thresh_low = 0xFFFF;
        self.amb_thresh_high = 0xFFFF;
        self.spad_array_selection = 0x00;
    }

    /// `VL53LX_copy_hist_cfg_to_static_cfg()`: lite-mode fields the firmware reads as the bin
    /// sequence.
    pub fn copy_to_static_cfg(&self, img: &mut DeviceImage) {
        let stat = img.static_config_mut();
        stat.set(
            "sigma_estimator__effective_pulse_width_ns",
            self.high_amb_even_bin_0_1,
        );
        stat.set(
            "sigma_estimator__effective_ambient_width_ns",
            self.high_amb_even_bin_2_3,
        );
        stat.set("sigma_estimator__sigma_ref_mm", self.high_amb_even_bin_4_5);
        stat.set(
            "algo__crosstalk_compensation_valid_height_mm",
            self.high_amb_odd_bin_0_1,
        );
        stat.set(
            "spare_host_config__static_config_spare_0",
            self.high_amb_odd_bin_2_3,
        );
        stat.set(
            "spare_host_config__static_config_spare_1",
            self.high_amb_odd_bin_4_5,
        );

        stat.set(
            "algo__range_ignore_threshold_mcps",
            (self.mid_amb_even_bin_0_1 << 8) + self.mid_amb_even_bin_2_3,
        );
        stat.set(
            "algo__range_ignore_valid_height_mm",
            self.mid_amb_even_bin_4_5,
        );
        stat.set("algo__range_min_clip", self.mid_amb_odd_bin_0_1);
        stat.set("algo__consistency_check__tolerance", self.mid_amb_odd_bin_2);
        stat.set(
            "spare_host_config__static_config_spare_2",
            self.mid_amb_odd_bin_3_4,
        );
        stat.set("sd_config__reset_stages_msb", self.mid_amb_odd_bin_5);
        stat.set("sd_config__reset_stages_lsb", self.user_bin_offset);

        let timing = img.timing_config_mut();
        timing.set(
            "range_config__sigma_thresh",
            (self.low_amb_even_bin_0_1 << 8) + self.low_amb_even_bin_2_3,
        );
        timing.set(
            "range_config__min_count_rate_rtn_limit_mcps",
            (self.low_amb_even_bin_4_5 << 8) + self.low_amb_odd_bin_0_1,
        );
        timing.set("range_config__valid_phase_low", self.low_amb_odd_bin_2_3);
        timing.set("range_config__valid_phase_high", self.low_amb_odd_bin_4_5);

        let dynamic = img.dynamic_config_mut();
        dynamic.set("system__thresh_high", self.amb_thresh_low);
        dynamic.set("system__thresh_low", self.amb_thresh_high);
        dynamic.set(
            "system__enable_xtalk_per_quadrant",
            self.spad_array_selection,
        );
    }

    /// `VL53LX_hist_get_bin_sequence_config()`: which sequence the device used for this frame, even
    /// or odd by the stream count.
    pub fn bin_sequence(&self, stream_count: i64, ambient_events_sum: i64) -> [i64; 6] {
        #[derive(PartialEq)]
        enum Level {
            Low,
            Mid,
            High,
        }
        let mut level = Level::Mid;
        if ambient_events_sum > 1024 * self.amb_thresh_high {
            level = Level::High;
        }
        if ambient_events_sum < 1024 * self.amb_thresh_low {
            level = Level::Low;
        }
        let packed = if stream_count & 0x01 == 0 {
            match level {
                Level::Low => [
                    self.low_amb_even_bin_0_1,
                    self.low_amb_even_bin_2_3,
                    self.low_amb_even_bin_4_5,
                ],
                Level::High => [
                    self.high_amb_even_bin_0_1,
                    self.high_amb_even_bin_2_3,
                    self.high_amb_even_bin_4_5,
                ],
                Level::Mid => [
                    self.mid_amb_even_bin_0_1,
                    self.mid_amb_even_bin_2_3,
                    self.mid_amb_even_bin_4_5,
                ],
            }
        } else if level == Level::Mid {
            // The mid odd sequence is the one packed differently.
            return [
                self.mid_amb_odd_bin_0_1 & 0x0F,
                self.mid_amb_odd_bin_0_1 >> 4,
                self.mid_amb_odd_bin_2,
                self.mid_amb_odd_bin_3_4 >> 4,
                self.mid_amb_odd_bin_3_4 & 0x0F,
                self.mid_amb_odd_bin_5 & 0x0F,
            ];
        } else if level == Level::Low {
            [
                self.low_amb_odd_bin_0_1,
                self.low_amb_odd_bin_2_3,
                self.low_amb_odd_bin_4_5,
            ]
        } else {
            [
                self.high_amb_odd_bin_0_1,
                self.high_amb_odd_bin_2_3,
                self.high_amb_odd_bin_4_5,
            ]
        };
        let mut seq = [0i64; 6];
        for i in 0..3 {
            seq[2 * i] = packed[i] & 0x0F;
            seq[2 * i + 1] = packed[i] >> 4;
        }
        seq
    }
}

fn d(name: &str) -> i64 {
    header_default(name).unwrap_or_else(|| panic!("tuning header has no {name}"))
}

/// `VL53LX_hist_post_process_config_t`, as `VL53LX_init_hist_post_process_config_struct()` leaves
/// it; the preset modes set the valid phase window here.
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl Default for HistPostProcessConfig {
    fn default() -> Self {
        HistPostProcessConfig {
            hist_algo_select: d("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT"),
            hist_target_order: d("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT"),
            filter_woi0: d("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT"),
            filter_woi1: d("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT"),
            hist_amb_est_method: d("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT"),
            ambient_thresh_sigma0: d("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT"),
            ambient_thresh_sigma1: d("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT"),
            ambient_thresh_events_scaler: d("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT"),
            min_ambient_thresh_events: d("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT"),
            noise_threshold: d("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT"),
            signal_total_events_limit: d("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT"),
            sigma_estimator_sigma_ref_mm: d("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT"),
            sigma_thresh: d("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT"),
            range_offset_mm: 0,
            gain_factor: d("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT"),
            valid_phase_low: 0x08,
            valid_phase_high: 0x88,
            consistency_phase_tolerance: d("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT"),
            consistency_event_sigma: d("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT"),
            consistency_event_min_spad_count: d(
                "TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT",
            ),
            consistency_min_max_tolerance: d(
                "TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT",
            ),
            crosstalk_compensation_enable: 0,
            crosstalk_detect_min_valid_range_mm: d(
                "TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT",
            ),
            crosstalk_detect_max_valid_range_mm: d(
                "TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT",
            ),
            crosstalk_detect_max_valid_rate_kcps: d(
                "TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT",
            ),
            crosstalk_detect_max_sigma_mm: d("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT"),
            crosstalk_detect_event_sigma: d("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT"),
            crosstalk_detect_min_max_tolerance: d(
                "TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT",
            ),
            crosstalk_compensation_plane_offset_kcps: 0,
            crosstalk_compensation_x_plane_gradient_kcps: 0,
            crosstalk_compensation_y_plane_gradient_kcps: 0,
        }
    }
}

/// `VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing needs to read them —
/// the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the ambient
/// estimate. Field names are the reference's (the C names, `__` folded to `_`);
/// [`HistogramBinData::fields`] gives them under the exact reference keys.
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl Default for HistogramBinData {
    fn default() -> Self {
        HistogramBinData {
            result_interrupt_status: 0,
            result_range_status: 0,
            result_report_status: 0,
            result_stream_count: 0,
            result_dss_actual_effective_spads: 0,
            phasecal_result_reference_phase: 0,
            phasecal_result_vcsel_start: 0,
            bin_data: vec![0; regs::HISTOGRAM_BUFFER_SIZE],
            zone_id: 0,
            first_bin: 0,
            number_of_bins: regs::HISTOGRAM_BUFFER_SIZE as i64,
            bins_in_data: regs::HISTOGRAM_BUFFER_SIZE as i64,
            cal_config_vcsel_start: 0,
            vcsel_width: 0,
            fast_osc_frequency: 0,
            vcsel_period: 0,
            bin_seq: [0; 6],
            bin_rep: [0; 6],
            min_bin_value: 0,
            max_bin_value: 0,
            number_of_ambient_bins: 0,
            number_of_ambient_samples: 0,
            ambient_events_sum: 0,
            ambient_per_bin: 0,
            total_periods_elapsed: 0,
            peak_duration_us: 0,
            woi_duration_us: 0,
            zero_distance_phase: 0,
            roi_config_user_roi_centre_spad: 0,
            roi_config_user_roi_requested_global_xy_size: 0,
        }
    }
}

fn ints(a: &[i64]) -> Value {
    Value::Array(a.iter().map(|&x| Value::int(x)).collect())
}

impl HistogramBinData {
    /// The device's range status (`result__range_status` masked).
    pub fn range_status(&self) -> i64 {
        self.result_range_status & regs::RANGE_STATUS_MASK
    }

    /// The fields under the C driver's (and the Python SDK's) names, in the reference's order.
    pub fn fields(&self) -> Vec<(&'static str, Value)> {
        vec![
            (
                "result__interrupt_status",
                Value::int(self.result_interrupt_status),
            ),
            ("result__range_status", Value::int(self.result_range_status)),
            (
                "result__report_status",
                Value::int(self.result_report_status),
            ),
            ("result__stream_count", Value::int(self.result_stream_count)),
            (
                "result__dss_actual_effective_spads",
                Value::int(self.result_dss_actual_effective_spads),
            ),
            (
                "phasecal_result__reference_phase",
                Value::int(self.phasecal_result_reference_phase),
            ),
            (
                "phasecal_result__vcsel_start",
                Value::int(self.phasecal_result_vcsel_start),
            ),
            ("bin_data", ints(&self.bin_data)),
            ("zone_id", Value::int(self.zone_id)),
            ("first_bin", Value::int(self.first_bin)),
            ("number_of_bins", Value::int(self.number_of_bins)),
            ("bins_in_data", Value::int(self.bins_in_data)),
            (
                "cal_config__vcsel_start",
                Value::int(self.cal_config_vcsel_start),
            ),
            ("vcsel_width", Value::int(self.vcsel_width)),
            ("fast_osc_frequency", Value::int(self.fast_osc_frequency)),
            ("vcsel_period", Value::int(self.vcsel_period)),
            ("bin_seq", ints(&self.bin_seq)),
            ("bin_rep", ints(&self.bin_rep)),
            ("min_bin_value", Value::int(self.min_bin_value)),
            ("max_bin_value", Value::int(self.max_bin_value)),
            (
                "number_of_ambient_bins",
                Value::int(self.number_of_ambient_bins),
            ),
            (
                "number_of_ambient_samples",
                Value::int(self.number_of_ambient_samples),
            ),
            ("ambient_events_sum", Value::int(self.ambient_events_sum)),
            ("ambient_per_bin", Value::int(self.ambient_per_bin)),
            (
                "total_periods_elapsed",
                Value::int(self.total_periods_elapsed),
            ),
            ("peak_duration_us", Value::int(self.peak_duration_us)),
            ("woi_duration_us", Value::int(self.woi_duration_us)),
            ("zero_distance_phase", Value::int(self.zero_distance_phase)),
            (
                "roi_config__user_roi_centre_spad",
                Value::int(self.roi_config_user_roi_centre_spad),
            ),
            (
                "roi_config__user_roi_requested_global_xy_size",
                Value::int(self.roi_config_user_roi_requested_global_xy_size),
            ),
        ]
    }

    /// Set one field by its reference key (the inverse of [`HistogramBinData::fields`]); `false` when
    /// the key is unknown or the value has the wrong shape.
    pub fn set_field(&mut self, key: &str, value: &Value) -> bool {
        fn arr(v: &Value) -> Option<Vec<i64>> {
            v.as_array()?.iter().map(Value::as_i64).collect()
        }
        if let Some(n) = value.as_i64() {
            let slot = match key {
                "result__interrupt_status" => &mut self.result_interrupt_status,
                "result__range_status" => &mut self.result_range_status,
                "result__report_status" => &mut self.result_report_status,
                "result__stream_count" => &mut self.result_stream_count,
                "result__dss_actual_effective_spads" => &mut self.result_dss_actual_effective_spads,
                "phasecal_result__reference_phase" => &mut self.phasecal_result_reference_phase,
                "phasecal_result__vcsel_start" => &mut self.phasecal_result_vcsel_start,
                "zone_id" => &mut self.zone_id,
                "first_bin" => &mut self.first_bin,
                "number_of_bins" => &mut self.number_of_bins,
                "bins_in_data" => &mut self.bins_in_data,
                "cal_config__vcsel_start" => &mut self.cal_config_vcsel_start,
                "vcsel_width" => &mut self.vcsel_width,
                "fast_osc_frequency" => &mut self.fast_osc_frequency,
                "vcsel_period" => &mut self.vcsel_period,
                "min_bin_value" => &mut self.min_bin_value,
                "max_bin_value" => &mut self.max_bin_value,
                "number_of_ambient_bins" => &mut self.number_of_ambient_bins,
                "number_of_ambient_samples" => &mut self.number_of_ambient_samples,
                "ambient_events_sum" => &mut self.ambient_events_sum,
                "ambient_per_bin" => &mut self.ambient_per_bin,
                "total_periods_elapsed" => &mut self.total_periods_elapsed,
                "peak_duration_us" => &mut self.peak_duration_us,
                "woi_duration_us" => &mut self.woi_duration_us,
                "zero_distance_phase" => &mut self.zero_distance_phase,
                "roi_config__user_roi_centre_spad" => &mut self.roi_config_user_roi_centre_spad,
                "roi_config__user_roi_requested_global_xy_size" => {
                    &mut self.roi_config_user_roi_requested_global_xy_size
                }
                _ => return false,
            };
            *slot = n;
            return true;
        }
        let Some(a) = arr(value) else { return false };
        match key {
            "bin_data" => self.bin_data = a,
            "bin_seq" | "bin_rep" => {
                let Ok(six) = <[i64; 6]>::try_from(a) else {
                    return false;
                };
                if key == "bin_seq" {
                    self.bin_seq = six;
                } else {
                    self.bin_rep = six;
                }
            }
            _ => return false,
        }
        true
    }
}

/// The ll-driver device states, single-zone case.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
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

/// `VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to one zone: the GPH handshake (the id stamped
/// into the dynamic config tells the device a configuration is new) and the timing-status flip
/// (whether a frame ran on VCSEL period A or B, which the post-processing has to know).
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl Default for LlState {
    fn default() -> Self {
        LlState {
            cfg_device_state: BareDeviceState::SwStandby,
            cfg_stream_count: 0,
            cfg_gph_id: regs::GROUPED_PARAMETER_HOLD_ID_MASK,
            cfg_timing_status: 0,
            rd_device_state: BareDeviceState::SwStandby,
            rd_stream_count: 0,
            rd_gph_id: regs::GROUPED_PARAMETER_HOLD_ID_MASK,
            rd_timing_status: 0,
        }
    }
}

impl LlState {
    /// Back to standby.
    pub fn reset(&mut self) {
        *self = LlState::default();
    }

    /// `VL53LX_update_ll_driver_rd_state()`: one frame read.
    pub fn update_rd(&mut self, mode_start: i64, grouped_parameter_hold: i64) {
        if mode_start & regs::DEVICE_MEASUREMENT_MODE_MODE_MASK == 0 {
            self.rd_device_state = BareDeviceState::SwStandby;
            self.rd_stream_count = 0;
            self.rd_gph_id = regs::GROUPED_PARAMETER_HOLD_ID_MASK;
            self.rd_timing_status = 0;
            return;
        }
        self.rd_stream_count = if self.rd_stream_count == 0xFF {
            0x80
        } else {
            self.rd_stream_count + 1
        };
        self.rd_gph_id ^= regs::GROUPED_PARAMETER_HOLD_ID_MASK;
        match self.rd_device_state {
            BareDeviceState::SwStandby => {
                self.rd_device_state =
                    if grouped_parameter_hold & regs::GROUPED_PARAMETER_HOLD_ID_MASK != 0 {
                        BareDeviceState::RangingWaitGphSync
                    } else {
                        BareDeviceState::RangingOutputData
                    };
                self.rd_stream_count = 0;
                self.rd_timing_status = 0;
            }
            BareDeviceState::RangingWaitGphSync => {
                self.rd_stream_count = 0;
                self.rd_device_state = BareDeviceState::RangingOutputData;
            }
            BareDeviceState::RangingOutputData => self.rd_timing_status ^= 0x01,
            _ => self.reset(),
        }
    }

    /// `VL53LX_update_ll_driver_cfg_state()`: one configuration written.
    pub fn update_cfg(&mut self, mode_start: i64) {
        if mode_start & regs::DEVICE_MEASUREMENT_MODE_MODE_MASK == 0 {
            self.cfg_device_state = BareDeviceState::SwStandby;
            self.cfg_stream_count = 0;
            self.cfg_gph_id = regs::GROUPED_PARAMETER_HOLD_ID_MASK;
            self.cfg_timing_status = 0;
            return;
        }
        self.cfg_stream_count = if self.cfg_stream_count == 0xFF {
            0x80
        } else {
            self.cfg_stream_count + 1
        };
        self.cfg_gph_id ^= regs::GROUPED_PARAMETER_HOLD_ID_MASK;
        match self.cfg_device_state {
            BareDeviceState::SwStandby => {
                self.cfg_timing_status ^= 0x01;
                self.cfg_stream_count = 1;
                self.cfg_device_state = BareDeviceState::RangingDssAuto;
            }
            BareDeviceState::RangingDssAuto => self.cfg_timing_status ^= 0x01,
            _ => self.reset(),
        }
    }
}

/// `VL53LX_range_data_t`: one target. The event sums and phases are what the algorithm produced; the
/// `*_range_mm` fields and the rates are what they mean.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
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

impl RangeData {
    /// Sigma, whole mm.
    pub fn sigma_mm(&self) -> i64 {
        self.sigma >> 7
    }

    /// The peak signal rate (9.7 Mcps) in kcps.
    pub fn signal_kcps(&self) -> i64 {
        self.peak_signal_count_rate_mcps * 1000 / 128
    }

    /// The ambient rate (9.7 Mcps) in kcps.
    pub fn ambient_kcps(&self) -> i64 {
        self.ambient_count_rate_mcps * 1000 / 128
    }

    /// Every field under the reference's name, in its order.
    pub fn fields(&self) -> Vec<(&'static str, i64)> {
        vec![
            ("range_id", self.range_id),
            ("start_bin", self.start_bin),
            ("first_bin", self.first_bin),
            ("peak_bin", self.peak_bin),
            ("last_bin", self.last_bin),
            ("end_bin", self.end_bin),
            ("width_bins", self.width_bins),
            ("window_bins", self.window_bins),
            ("vcsel_width", self.vcsel_width),
            ("fast_osc_frequency", self.fast_osc_frequency),
            ("zero_distance_phase", self.zero_distance_phase),
            ("spads", self.spads),
            ("total_periods_elapsed", self.total_periods_elapsed),
            ("peak_duration_us", self.peak_duration_us),
            ("woi_duration_us", self.woi_duration_us),
            ("ambient_events", self.ambient_events),
            ("total_events", self.total_events),
            ("signal_events", self.signal_events),
            (
                "peak_signal_count_rate_mcps",
                self.peak_signal_count_rate_mcps,
            ),
            (
                "avg_signal_count_rate_mcps",
                self.avg_signal_count_rate_mcps,
            ),
            ("ambient_count_rate_mcps", self.ambient_count_rate_mcps),
            ("total_rate_per_spad_mcps", self.total_rate_per_spad_mcps),
            (
                "signal_events_per_spad_kcps",
                self.signal_events_per_spad_kcps,
            ),
            ("sigma", self.sigma),
            ("phase_start", self.phase_start),
            ("phase_mean", self.phase_mean),
            ("phase_end", self.phase_end),
            ("min_range_mm", self.min_range_mm),
            ("median_range_mm", self.median_range_mm),
            ("max_range_mm", self.max_range_mm),
            ("range_status", self.range_status),
        ]
    }
}

/// `VL53LX_range_results_t`, reduced to the single-zone case.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
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
