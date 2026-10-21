//! Sensor clock model and frame capture time (contracts/14_SENSOR_TMF8829.md §5).
//!
//! Integer arithmetic exactly as the contract (and the Python reference):
//! `u32` tick differences modulo 2^32, signed 32-bit tick offsets, 64-bit
//! signed µs, floor division, arithmetic `>>`. The vectors in
//! `contracts/vectors/tmf8829_frame_time.json` pin it bit for bit.

use super::frames::FrameInfo;

/// SYSTICK 125 kHz = 8 MCU µs per tick, Q16.
pub const NOMINAL_Q16: u32 = 8 << 16;
/// First ratio estimate after this span.
pub const CLOCK_WINDOW_MIN_US: i64 = 200_000;
/// Then a sliding 1..2 s window.
pub const CLOCK_WINDOW_SLIDE_US: i64 = 1_000_000;
pub const EDGE_LATE_US: i64 = 2000;

/// Clock ratio not measured yet (nominal 8 µs/tick used).
pub const FT_NOMINAL: u8 = 0x01;
/// Clock model restarted at this frame (SYSTICK jump).
pub const FT_CLOCK_RESET: u8 = 0x02;
/// INT edge > 2 ms later after t1 than usual in this stream.
pub const FT_EDGE_LATE: u8 = 0x04;
/// Sub-integrations not back to back (histogram FIFO wait).
pub const FT_STALLED: u8 = 0x08;
/// Histograms + 32x32/48x32: T1 may be too long, centre late.
pub const FT_T1_UNSURE: u8 = 0x10;

pub const FT_NAMES: &[(u8, &str)] = &[
    (FT_NOMINAL, "nominal"),
    (FT_CLOCK_RESET, "clock_reset"),
    (FT_EDGE_LATE, "edge_late"),
    (FT_STALLED, "stalled"),
    (FT_T1_UNSURE, "t1_unsure"),
];

/// Names of the `FT_*` bits set in `flags`.
pub fn flag_names(flags: u8) -> Vec<&'static str> {
    FT_NAMES
        .iter()
        .filter(|(bit, _)| flags & bit != 0)
        .map(|(_, n)| *n)
        .collect()
}

/// Sub-integrations per results frame by `fp_mode` (unknown → 2).
pub fn fp_subints(fp_mode: u8) -> u32 {
    match fp_mode {
        3 | 4 => 4,
        5 => 6,
        _ => 2,
    }
}

/// Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame (the MCU
/// time of the header read and the pre-header SYSTICK sampled by it). The
/// ratio is taken over a sliding 1..2 s window, first after 200 ms; it is
/// also the distance correction (ams `tmf8829ClockCorrection`).
#[derive(Debug, Clone)]
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

impl Default for SensorClock {
    fn default() -> Self {
        SensorClock {
            ratio_q16: NOMINAL_Q16,
            nominal: true,
            valid: false,
            last: (0, 0),
            anchor: (0, 0),
            mid: None,
        }
    }
}

impl SensorClock {
    pub fn new() -> Self {
        SensorClock::default()
    }

    /// Restart the model — call at every stream start.
    pub fn reset(&mut self) {
        *self = SensorClock::default();
    }

    /// MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.
    pub fn ratio(&self) -> f64 {
        self.ratio_q16 as f64 / NOMINAL_Q16 as f64
    }

    fn step_ok(d_us: i64, d_tick: u32) -> bool {
        let expect = d_tick as i64 * 8;
        let tol = d_us.div_euclid(10) + 50;
        expect + tol >= d_us && expect <= d_us + tol
    }

    /// Add a pair → [`FT_CLOCK_RESET`] if it does not fit the model (which
    /// then restarts from it), else 0.
    pub fn add(&mut self, us: u64, tick: u32) -> u8 {
        let mut flags = 0;
        if self.valid
            && !Self::step_ok(
                us as i64 - self.last.0 as i64,
                tick.wrapping_sub(self.last.1),
            )
        {
            self.valid = false;
            flags = FT_CLOCK_RESET;
        }
        self.last = (us, tick);
        if !self.valid {
            self.valid = true;
            self.anchor = self.last;
            self.mid = None;
            self.ratio_q16 = NOMINAL_Q16;
            self.nominal = true;
            return flags;
        }
        let span = us as i64 - self.anchor.0 as i64;
        if span >= CLOCK_WINDOW_MIN_US {
            let d_tick = tick.wrapping_sub(self.anchor.1) as i64;
            // d_tick == 0 over ≥ 200 ms cannot pass the step check; guard the
            // division anyway (the reference would raise).
            if d_tick != 0 {
                self.ratio_q16 = ((span << 16).div_euclid(d_tick)) as u32;
                self.nominal = false;
            }
        }
        if self.mid.is_none() && span >= CLOCK_WINDOW_SLIDE_US {
            self.mid = Some(self.last);
        } else if self.mid.is_some() && span >= 2 * CLOCK_WINDOW_SLIDE_US {
            self.anchor = self.mid.take().unwrap();
        }
        flags
    }

    /// SYSTICK `tick` → MCU µs through the pair (`pair_us`, `pair_tick`):
    /// `pair_us + ((s32(tick − pair_tick) × ratio_q16) >> 16)`.
    pub fn to_us(&self, pair_us: u64, pair_tick: u32, tick: u32) -> i64 {
        let d = tick.wrapping_sub(pair_tick) as i32 as i128;
        pair_us as i64 + ((d * self.ratio_q16 as i128) >> 16) as i64
    }
}

/// Capture time of one results frame, MCU µs.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

/// When a results frame was captured. A results frame is N equal
/// sub-integrations back to back; the footer has the SYSTICK of the start of
/// the first (t0) and of the last (t1). The window is `[t0, t1 + T1]`, T1 the
/// stream's shortest `(t1 − t0)/(N − 1)`; its centre is the frame time.
///
/// Holds only its own per-stream state; the [`SensorClock`] it reads is
/// passed to [`CaptureTimer::results`].
#[derive(Debug, Clone, Default)]
pub struct CaptureTimer {
    lag_min: Option<i64>,
    hist_seen: bool,
    sub_min: u32,
}

impl CaptureTimer {
    pub fn new() -> Self {
        CaptureTimer::default()
    }

    /// Restart — call at every stream start.
    pub fn reset(&mut self) {
        *self = CaptureTimer::default();
    }

    /// A histogram frame was seen in the stream.
    pub fn histograms(&mut self) {
        self.hist_seen = true;
    }

    /// Capture time of a results frame. `flags` is what
    /// [`SensorClock::add`] returned for this frame's pair.
    #[allow(clippy::too_many_arguments)]
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
    ) -> FrameTime {
        if clock.nominal {
            flags |= FT_NOMINAL;
        }
        let t0 = clock.to_us(pair_us, systick, t0_tick);
        let t1 = clock.to_us(pair_us, systick, t1_tick);

        if edge_us != 0 && !self.hist_seen {
            let lag = edge_us as i64 - t1;
            if lag <= 0 || matches!(self.lag_min, Some(m) if lag > m + EDGE_LATE_US) {
                flags |= FT_EDGE_LATE;
            } else if self.lag_min.is_none_or(|m| lag < m) {
                self.lag_min = Some(lag);
            }
        }

        let n1 = fp_subints(fp_mode) - 1;
        let sub = t1_tick.wrapping_sub(t0_tick) / n1;
        if self.sub_min == 0 || sub < self.sub_min {
            self.sub_min = sub;
        }
        if sub as u64 > self.sub_min as u64 + 1 {
            flags |= FT_STALLED;
        }
        if self.hist_seen && n1 > 1 {
            flags |= FT_T1_UNSURE;
        }

        let t1_len = ((self.sub_min as u64 * clock.ratio_q16 as u64) >> 16) as u32;
        FrameTime {
            timestamp_us: (t0 + t1 + t1_len as i64).div_euclid(2),
            flags,
            t1_len_us: t1_len,
            t0_us: t0,
            t1_us: t1,
            edge_us,
            pair_us,
            ratio_q16: clock.ratio_q16,
        }
    }

    /// [`CaptureTimer::results`] from a parsed results frame.
    pub fn results_frame(
        &mut self,
        clock: &SensorClock,
        f: &FrameInfo,
        pair_us: u64,
        edge_us: u64,
        flags: u8,
    ) -> FrameTime {
        self.results(
            clock,
            f.fp_mode(),
            f.systick,
            f.t0_integration,
            f.t1_integration,
            pair_us,
            edge_us,
            flags,
        )
    }
}
