//! TMF8829 frame layout and pixel decoding (contracts/14_SENSOR_TMF8829.md §3).
//!
//! A frame is the sensor bytes from register 0xFA on: 5-byte pre-header
//! (FIFOSTATUS, SYSTICK), 16-byte header, data (pixel results or
//! histograms), 12-byte footer. Pixel arrays are flat, row-major, per peak
//! first (`[peaks][rows][cols]`), raw sensor integers.

pub const PRE_HEADER_SIZE: usize = 5;
pub const HEADER_SIZE: usize = 16;
pub const FOOTER_SIZE: usize = 12;
/// The 21 B the bridge reads at 0xFA (pre-header + header).
pub const FRAME_HEAD: usize = PRE_HEADER_SIZE + HEADER_SIZE;
pub const FRAME_EOF: u16 = 0xE0F7;

pub const FID_MASK: u8 = 0xF0;
pub const FID_RESULTS: u8 = 0x10;
pub const FID_HISTOGRAMS: u8 = 0x20;
pub const FID_REF_SPAD: u8 = 0x30;

pub const LAYOUT_PEAKS: u8 = 0x07;
pub const LAYOUT_SIGNAL: u8 = 0x08;
pub const LAYOUT_NOISE: u8 = 0x10;
pub const LAYOUT_XTALK: u8 = 0x20;
/// 32x32/48x32: set on the second (odd-row) half-frame.
pub const LAYOUT_SUBFRAME: u8 = 0x40;

pub const FRAME_VALID: u8 = 0x01;
pub const FRAME_ABORTED: u8 = 0xC0;
/// `frame_status` warning bits, in the order `problems()` reports them.
pub const FRAME_WARNINGS: &[(u8, &str)] = &[
    (0x08, "HV_CP_OVERLOAD"),
    (0x10, "VCDRV_OVERLOAD"),
    (0x20, "VCDRV_BURST_EXCEEDED"),
];
/// `footer.reserved` b0: distances in mm, else 0.25 mm.
pub const FOOTER_DIST_IN_MM: u8 = 0x01;

pub const FIFO_TX_UNDERRUN: u8 = 0x10;
pub const FIFO_RX_OVERRUN: u8 = 0x20;

/// `fp_mode` → (rows, cols) of the full focal plane.
pub fn fp_grid(fp_mode: u8) -> Option<(usize, usize)> {
    match fp_mode {
        0 | 1 => Some((8, 8)),
        2 => Some((16, 16)),
        3 | 4 => Some((32, 32)),
        5 => Some((32, 48)),
        _ => None,
    }
}

/// `fp_mode` → its name (`8x8A`, `8x8B`, `16x16`, `32x32`, `32x32s`, `48x32`).
pub fn fp_name(fp_mode: u8) -> Option<&'static str> {
    ["8x8A", "8x8B", "16x16", "32x32", "32x32s", "48x32"]
        .get(fp_mode as usize)
        .copied()
}

/// (rows, cols) one results frame carries: half the rows for 32x32/48x32.
pub fn frame_rows(fp_mode: u8) -> Option<(usize, usize)> {
    fp_grid(fp_mode).map(|(r, c)| if r == 32 { (r / 2, c) } else { (r, c) })
}

/// A frame whose bytes do not match its own header (message as the
/// reference SDK words it).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FrameError(pub String);

impl std::fmt::Display for FrameError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for FrameError {}

/// Pre-header, header and footer of one frame (any kind).
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl FrameInfo {
    /// [`FID_RESULTS`], [`FID_HISTOGRAMS`] or [`FID_REF_SPAD`].
    pub fn kind(&self) -> u8 {
        self.id & FID_MASK
    }

    pub fn fp_mode(&self) -> u8 {
        self.id & 0x0F
    }

    pub fn is_second_half(&self) -> bool {
        self.layout & LAYOUT_SUBFRAME != 0
    }

    pub fn distance_in_mm(&self) -> bool {
        self.reserved & FOOTER_DIST_IN_MM != 0
    }

    /// What is wrong with the frame, human-readable; empty when fine.
    pub fn problems(&self) -> Vec<String> {
        let mut out = Vec::new();
        if self.eof != FRAME_EOF {
            out.push(format!("EOF 0x{:04X}", self.eof));
        }
        if self.frame_status & FRAME_VALID == 0 {
            out.push(format!("status 0x{:02X} not valid", self.frame_status));
        }
        if self.frame_status & FRAME_ABORTED != 0 {
            out.push("aborted".to_string());
        }
        for (bit, name) in FRAME_WARNINGS {
            if self.frame_status & bit != 0 {
                out.push(name.to_string());
            }
        }
        if self.fifo_status & FIFO_TX_UNDERRUN != 0 {
            out.push("txfifo_underrun".to_string());
        }
        if self.fifo_status & FIFO_RX_OVERRUN != 0 {
            out.push("rxfifo_overrun".to_string());
        }
        out
    }
}

fn u16_at(b: &[u8], i: usize) -> u16 {
    u16::from_le_bytes([b[i], b[i + 1]])
}

fn u32_at(b: &[u8], i: usize) -> u32 {
    u32::from_le_bytes(b[i..i + 4].try_into().unwrap())
}

/// Full frame size (pre-header included) from its first 21 bytes
/// (`5 + 4 + payload`); `None` if fewer than 9 bytes are given.
pub fn frame_size(head: &[u8]) -> Option<usize> {
    if head.len() < 9 {
        return None;
    }
    Some(PRE_HEADER_SIZE + 4 + u16_at(head, 7) as usize)
}

/// Parse the pre-header, header and footer of a reassembled frame.
pub fn parse_frame(raw: &[u8]) -> Result<FrameInfo, FrameError> {
    if raw.len() < FRAME_HEAD + FOOTER_SIZE {
        return Err(FrameError(format!(
            "frame of {} B is shorter than header + footer",
            raw.len()
        )));
    }
    let h = PRE_HEADER_SIZE;
    let f = raw.len() - FOOTER_SIZE;
    Ok(FrameInfo {
        fifo_status: raw[0],
        systick: u32_at(raw, 1),
        id: raw[h],
        layout: raw[h + 1],
        payload: u16_at(raw, h + 2),
        f_number: u32_at(raw, h + 4),
        temperature: [raw[h + 8] as i8, raw[h + 9] as i8, raw[h + 10] as i8],
        bdv: raw[h + 11],
        ref_pos: [u16_at(raw, h + 12), u16_at(raw, h + 14)],
        t0_integration: u32_at(raw, f),
        t1_integration: u32_at(raw, f + 4),
        frame_status: raw[f + 8],
        reserved: raw[f + 9],
        eof: u16_at(raw, f + 10),
        size: raw.len(),
    })
}

/// Decoded results of one frame or of a merged measurement.
///
/// `distance`, `snr`, `signal` are flat `[peaks][rows][cols]`; `noise`,
/// `xtalk` flat `[rows][cols]`. Raw sensor integers: distance in 0.25 mm (or
/// mm with the footer flag), not yet clock-corrected. Absent fields are
/// `None` (no peaks → no distance/snr/signal).
#[derive(Debug, Clone, PartialEq, Eq)]
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

impl PixelResults {
    /// (rows, cols) of the arrays present; (0, 0) when the result format has
    /// no peaks, noise nor xtalk (as the reference SDK).
    pub fn shape(&self) -> (usize, usize) {
        if self.distance.is_some() || self.noise.is_some() || self.xtalk.is_some() {
            (self.rows, self.cols)
        } else {
            (0, 0)
        }
    }

    /// Raw distance of `peak` at (`row`, `col`).
    pub fn distance_at(&self, peak: usize, row: usize, col: usize) -> Option<u16> {
        let i = (peak * self.rows + row) * self.cols + col;
        self.distance.as_ref().and_then(|d| d.get(i).copied())
    }
}

/// Bytes of one results pixel for a `layout` byte.
pub fn pixel_size(layout: u8) -> usize {
    let mut n = 0;
    if layout & LAYOUT_NOISE != 0 {
        n += 2;
    }
    if layout & LAYOUT_XTALK != 0 {
        n += 2;
    }
    let per_peak = if layout & LAYOUT_SIGNAL != 0 { 5 } else { 3 };
    n + (layout & LAYOUT_PEAKS) as usize * per_peak
}

/// Pixel results of one results frame, row-major.
pub fn parse_results(raw: &[u8]) -> Result<PixelResults, FrameError> {
    let info = parse_frame(raw)?;
    parse_results_info(raw, &info)
}

/// [`parse_results`] with the frame already parsed.
pub fn parse_results_info(raw: &[u8], f: &FrameInfo) -> Result<PixelResults, FrameError> {
    if f.kind() != FID_RESULTS {
        return Err(FrameError(format!(
            "frame id 0x{:02X} is not a results frame",
            f.id
        )));
    }
    let (rows, cols) = frame_rows(f.fp_mode())
        .ok_or_else(|| FrameError(format!("unknown fp_mode {}", f.fp_mode())))?;
    let px_size = pixel_size(f.layout);
    let n = rows * cols;
    let data_size = f.size as isize - (FRAME_HEAD + FOOTER_SIZE) as isize;
    if (px_size * n) as isize != data_size {
        return Err(FrameError(format!(
            "result data {} B, expected {} px × {} B (layout 0x{:02X})",
            data_size, n, px_size, f.layout
        )));
    }
    let peaks = (f.layout & LAYOUT_PEAKS) as usize;
    let has_signal = f.layout & LAYOUT_SIGNAL != 0;
    let has_noise = f.layout & LAYOUT_NOISE != 0;
    let has_xtalk = f.layout & LAYOUT_XTALK != 0;
    let mut distance = vec![0u16; peaks * n];
    let mut snr = vec![0u8; peaks * n];
    let mut signal = vec![0u16; if has_signal { peaks * n } else { 0 }];
    let mut noise = vec![0u16; if has_noise { n } else { 0 }];
    let mut xtalk = vec![0u16; if has_xtalk { n } else { 0 }];
    for px in 0..n {
        let mut o = FRAME_HEAD + px * px_size;
        if has_noise {
            noise[px] = u16_at(raw, o);
            o += 2;
        }
        if has_xtalk {
            xtalk[px] = u16_at(raw, o);
            o += 2;
        }
        for p in 0..peaks {
            distance[p * n + px] = u16_at(raw, o);
            snr[p * n + px] = raw[o + 2];
            o += 3;
            if has_signal {
                signal[p * n + px] = u16_at(raw, o);
                o += 2;
            }
        }
    }
    let has_peaks = peaks > 0;
    Ok(PixelResults {
        peaks,
        rows,
        cols,
        distance: has_peaks.then_some(distance),
        snr: has_peaks.then_some(snr),
        signal: (has_peaks && has_signal).then_some(signal),
        noise: has_noise.then_some(noise),
        xtalk: has_xtalk.then_some(xtalk),
    })
}

/// Interleave the rows of two `[planes][rows][cols]` arrays: even rows from
/// `a`, odd from `b`. `None` if either is absent or the sizes differ.
fn merge_rows<T: Copy>(
    a: &Option<Vec<T>>,
    b: &Option<Vec<T>>,
    rows: usize,
    cols: usize,
) -> Option<Vec<T>> {
    let (a, b) = (a.as_ref()?, b.as_ref()?);
    if a.len() != b.len() || rows * cols == 0 || a.len() % (rows * cols) != 0 {
        return None;
    }
    let planes = a.len() / (rows * cols);
    let mut out = Vec::with_capacity(a.len() * 2);
    for p in 0..planes {
        for r in 0..rows {
            let i = (p * rows + r) * cols;
            out.extend_from_slice(&a[i..i + cols]);
            out.extend_from_slice(&b[i..i + cols]);
        }
    }
    Some(out)
}

/// 32x32/48x32: the first half-frame holds the even rows, the second the
/// odd ones (`full[2i] = first[i]`, `full[2i+1] = second[i]`). Errors when
/// the halves have different grids or result formats.
pub fn merge_halves(
    first: &PixelResults,
    second: &PixelResults,
) -> Result<PixelResults, FrameError> {
    if (first.peaks, first.rows, first.cols) != (second.peaks, second.rows, second.cols)
        || first.signal.is_some() != second.signal.is_some()
        || first.noise.is_some() != second.noise.is_some()
        || first.xtalk.is_some() != second.xtalk.is_some()
    {
        return Err(FrameError(
            "half-frames of different result formats".to_string(),
        ));
    }
    let (rows, cols) = (first.rows, first.cols);
    Ok(PixelResults {
        peaks: first.peaks,
        rows: rows * 2,
        cols,
        distance: merge_rows(&first.distance, &second.distance, rows, cols),
        snr: merge_rows(&first.snr, &second.snr, rows, cols),
        signal: merge_rows(&first.signal, &second.signal, rows, cols),
        noise: merge_rows(&first.noise, &second.noise, rows, cols),
        xtalk: merge_rows(&first.xtalk, &second.xtalk, rows, cols),
    })
}

/// A raw histogram frame (25 377 B); decoding the bins is left to the
/// caller (ams histogram layout).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct HistogramFrame {
    pub info: FrameInfo,
    pub raw: Vec<u8>,
}

impl HistogramFrame {
    /// The bytes between the header and the footer.
    pub fn data(&self) -> &[u8] {
        &self.raw[FRAME_HEAD..self.raw.len() - FOOTER_SIZE]
    }
}
