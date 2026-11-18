//! The frame before this one, for the wrap and event consistency checks.

use super::core::{BareDeviceState, HistPostProcessConfig, HistogramBinData, RangeResults};
use super::hist::{self, PrevTarget};

/// The frame before this one, reduced to what the consistency checks read (`VL53LX_zone_hist_info_t`
/// and `VL53LX_zone_objects_t` for one zone). The engine alternates two VCSEL periods, so one frame
/// cannot say whether a target is inside the unambiguous range or an echo wrapped into it; the API
/// compares consecutive frames. The BareDriver keeps no history, so the driver owns one of these and
/// hands it every frame.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct FrameHistory {
    rd_device_state: Option<BareDeviceState>,
    targets: Vec<PrevTarget>,
    total_periods_elapsed: i64,
    spads: i64,
}

impl FrameHistory {
    /// An empty history: the first [`FrameHistory::apply`] only remembers.
    pub fn new() -> FrameHistory {
        FrameHistory::default()
    }

    /// Macro periods the remembered frame ran for.
    pub fn total_periods_elapsed(&self) -> i64 {
        self.total_periods_elapsed
    }

    /// Effective SPADs of the remembered frame, 1/256.
    pub fn spads(&self) -> i64 {
        self.spads
    }

    /// The remembered frame's targets.
    pub fn targets(&self) -> &[PrevTarget] {
        &self.targets
    }

    /// Forget the stream so far: the next [`FrameHistory::apply`] only remembers.
    pub fn reset(&mut self) {
        *self = FrameHistory::default();
    }

    /// Check `results` against the remembered frame, then remember it — C's order: both checks run
    /// before hist_data and range_results are copied into the zone history.
    pub fn apply(
        &mut self,
        results: &mut RangeResults,
        bins: &HistogramBinData,
        rd_device_state: BareDeviceState,
        hpp: &HistPostProcessConfig,
    ) {
        self.phase_consistency_check(results, hpp);
        self.rd_device_state = Some(rd_device_state);
        self.total_periods_elapsed = bins.total_periods_elapsed;
        self.spads = bins.result_dss_actual_effective_spads;
        self.targets = results
            .targets
            .iter()
            .map(|t| PrevTarget {
                ambient_events: t.ambient_events,
                total_events: t.total_events,
                phase_mean: t.phase_mean,
                range_status: t.range_status,
            })
            .collect();
    }

    // VL53LX_hist_phase_consistency_check(): every ranged target starts guilty and has to find itself in
    // the previous frame, within the phase tolerance and with consistent events.
    fn phase_consistency_check(&self, results: &mut RangeResults, hpp: &HistPostProcessConfig) {
        let phase_tolerance = hpp.consistency_phase_tolerance << 8; // a whole phase unit; phases are 8.8
                                                                    // C also admits RANGING_GATHER_DATA, the multizone path.
        if self.rd_device_state != Some(BareDeviceState::RangingOutputData) {
            return;
        }
        if phase_tolerance == 0 {
            return;
        }
        for target in results.targets.iter_mut() {
            if target.range_status != hist::DEVICE_ERROR_RANGE_COMPLETE
                && target.range_status != hist::DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK
            {
                continue;
            }
            target.range_status = if !self.targets.is_empty() {
                hist::DEVICE_ERROR_PHASE_CONSISTENCY
            } else {
                hist::DEVICE_ERROR_PREV_RANGE_NO_TARGETS
            };
            for prev in &self.targets {
                if (target.phase_mean - prev.phase_mean).abs() >= phase_tolerance {
                    continue;
                }
                let mut status = hist::events_consistency_check(
                    hpp.consistency_event_sigma,
                    hpp.consistency_event_min_spad_count,
                    self,
                    prev,
                    target,
                );
                if status == hist::DEVICE_ERROR_RANGE_COMPLETE {
                    status = hist::merged_pulse_check(hpp.consistency_min_max_tolerance, target);
                }
                target.range_status = status;
            }
        }
    }
}
