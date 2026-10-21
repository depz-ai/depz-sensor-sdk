//! Frames in stream order → measurements (contracts/14_SENSOR_TMF8829.md §6).

use super::clock::{CaptureTimer, FrameTime, SensorClock};
use super::frames::{
    fp_grid, merge_halves, parse_frame, parse_results_info, FrameInfo, HistogramFrame,
    PixelResults, FID_HISTOGRAMS, FID_REF_SPAD, FID_RESULTS,
};

/// One measurement: its results frame (8x8, 16x16) or both half-frames
/// (32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.
///
/// `distance_mm` is flat `[peaks][rows][cols]`, clock-corrected, in mm
/// (`None` when the result format has no peaks). `results` keeps the raw
/// sensor integers. `problems` lists what went wrong since the previous
/// measurement.
#[derive(Debug, Clone, PartialEq)]
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

impl Tmf8829Measurement {
    /// (rows, cols) of the full focal plane.
    pub fn shape(&self) -> (usize, usize) {
        fp_grid(self.fp_mode).unwrap_or((0, 0))
    }

    pub fn peaks(&self) -> usize {
        self.results.peaks
    }

    /// Sensor die temperature (header byte 0), °C.
    pub fn temperature_c(&self) -> i8 {
        self.frames.last().map_or(0, |f| f.temperature[0])
    }
}

/// Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units).
pub fn distance_mm(results: &PixelResults, ratio: f64, in_mm: bool) -> Option<Vec<f64>> {
    results.distance.as_ref().map(|d| {
        d.iter()
            .map(|&x| {
                let v = x as f64 * ratio;
                if in_mm {
                    v
                } else {
                    v / 4.0
                }
            })
            .collect()
    })
}

/// Histogram frames between two results frames belong to the second; a
/// ref-SPAD frame comes right before its results frame (each half-frame has
/// its own). 32x32/48x32: the first half (layout 0x40 clear) holds the even
/// rows. Call [`MeasurementBuilder::reset`] at the start of every stream:
/// the clock model and T1 are per stream. Mirrors the Python builder,
/// problem texts included.
#[derive(Debug, Default)]
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

impl MeasurementBuilder {
    pub fn new() -> Self {
        MeasurementBuilder::default()
    }

    pub fn reset(&mut self) {
        *self = MeasurementBuilder::default();
    }

    /// One frame with the MCU time of its header read and its INT edge
    /// (0 = none) → the measurement it completes, or `None`.
    pub fn add(
        &mut self,
        raw: &[u8],
        timestamp_us: u64,
        edge_us: u64,
    ) -> Option<Tmf8829Measurement> {
        let f = match parse_frame(raw) {
            Ok(f) => f,
            Err(e) => {
                self.parse_errors += 1;
                self.problems.push(e.0);
                return None;
            }
        };
        if f.kind() == FID_REF_SPAD {
            self.pending.push((f, raw.to_vec())); // zero pre-header: no clock pair
            return None;
        }
        if f.kind() != FID_RESULTS && f.kind() != FID_HISTOGRAMS {
            self.parse_errors += 1;
            self.problems.push(format!("frame id 0x{:02X}: unknown kind", f.id));
            return None;
        }
        let flags = self.clock.add(timestamp_us, f.systick);
        let bad = f.problems();
        if !bad.is_empty() {
            self.problems.push(format!(
                "frame id=0x{:02X} fN={} {} B: {}",
                f.id,
                f.f_number,
                f.size,
                bad.join(", ")
            ));
        }
        if f.kind() == FID_HISTOGRAMS {
            self.timer.histograms();
            self.pending.push((f, raw.to_vec()));
            return None;
        }

        let t = self
            .timer
            .results_frame(&self.clock, &f, timestamp_us, edge_us, flags);
        for (pf, praw) in self.pending.drain(..) {
            if pf.kind() == FID_HISTOGRAMS {
                self.hist.push(HistogramFrame { info: pf, raw: praw });
            } else {
                self.refs.push(praw);
            }
        }
        let res = match parse_results_info(raw, &f) {
            Ok(r) => r,
            Err(e) => {
                self.parse_errors += 1;
                self.problems.push(format!("fN={}: {}", f.f_number, e));
                self.half = None;
                return None;
            }
        };

        let (res, frames, times) = if fp_grid(f.fp_mode()).map_or(0, |g| g.0) == 32 {
            let Some((f0, res0, t0)) = self.half.take() else {
                if f.is_second_half() {
                    self.problems.push(format!(
                        "second half fN={} without a first one - dropped",
                        f.f_number
                    ));
                    self.hist.clear();
                    self.refs.clear();
                    return None;
                }
                self.half = Some((f, res, t));
                return None;
            };
            if !f.is_second_half() {
                self.problems.push(format!(
                    "two first halves in a row (fN {}, {})",
                    f0.f_number, f.f_number
                ));
                self.half = Some((f, res, t));
                return None;
            }
            match merge_halves(&res0, &res) {
                Ok(m) => (m, vec![f0, f.clone()], vec![t0, t]),
                Err(e) => {
                    // Not reachable on the board (both halves share one
                    // config); the reference has no branch for it.
                    self.parse_errors += 1;
                    self.problems.push(format!("fN={}: {}", f.f_number, e));
                    return None;
                }
            }
        } else {
            (res, vec![f.clone()], vec![t])
        };

        let n = times.len() as i64;
        let m = Tmf8829Measurement {
            timestamp_us: times.iter().map(|x| x.timestamp_us).sum::<i64>().div_euclid(n),
            flags: times.iter().fold(0, |a, x| a | x.flags),
            fp_mode: f.fp_mode(),
            f_number: f.f_number,
            distance_mm: distance_mm(&res, self.clock.ratio(), f.distance_in_mm()),
            results: res,
            frames,
            times,
            histograms: std::mem::take(&mut self.hist),
            ref_frames: std::mem::take(&mut self.refs),
            problems: std::mem::take(&mut self.problems),
        };
        Some(m)
    }
}
