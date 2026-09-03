//! VL53L8 raw-frame decoder (ST ULD `GetRangingData`, contract 04).
//!
//! **One decoder serves BOTH ToF variants** — the base **VL53L8CX** and the
//! **VL53L8CH** (CX plus compact-network-histogram support and its own
//! production USB PID 0xED40). The results-frame block layout is identical
//! across the two; only the frame-tail footer-id geometry differs, and that is
//! selected by [`Variant`]. There is intentionally no duplicated CH decode path.
//!
//! Given one reassembled results frame (as read from register 0x00), this
//! reproduces the ULD block walk: byte-swap every 32-bit word, iterate the
//! block headers from offset 16, and scatter each output block into per-zone
//! arrays sized to the active resolution (16 or 64 zones).
//!
//! This is the *decode* half of the driver only — the live register-bridge
//! init/config (firmware download, DCI programming) is hardware-dependent and
//! intentionally out of scope here (see `advanced` for the pure DCI codecs,
//! which are shared by CX and CH alike).
//!
//! CNH (compact network histogram) decode is CH-only; this decoder surfaces the
//! raw block ([`Vl53l8Results::cnh_raw`]) and [`super::cnh::decode_cnh`] unpacks
//! it into per-aggregate histograms. See also [`CNH_DATA_IDX`].
//!
//! Mirrors the TS/Python `VL53L8CX.parseFrame`.

pub const RESOLUTION_4X4: usize = 16;
pub const RESOLUTION_8X8: usize = 64;
pub const NB_TARGET_PER_ZONE: usize = 1;

// Block indices (union Block_header idx field) for NB_TARGET_PER_ZONE == 1.
const METADATA_IDX: u16 = 0x54b4;
const SPAD_COUNT_IDX: u16 = 0x55d0;
const AMBIENT_RATE_IDX: u16 = 0x54d0;
const NB_TARGET_DETECTED_IDX: u16 = 0xdb84;
const SIGNAL_RATE_IDX: u16 = 0xdbc4;
const RANGE_SIGMA_MM_IDX: u16 = 0xdec4;
const DISTANCE_IDX: u16 = 0xdf44;
const REFLECTANCE_EST_PC_IDX: u16 = 0xe044;
const TARGET_STATUS_IDX: u16 = 0xe084;
/// CNH (compact network histogram) output block id — **CH-only**. The DEPZ
/// decode surfaces this block's raw bytes ([`Vl53l8Results::cnh_raw`]); the full
/// histogram unpack is a not-yet-implemented CH extension point (see below).
pub const CNH_DATA_IDX: u16 = 0xc048;

/// ToF silicon/firmware variant. Both the base **VL53L8CX** and the
/// **VL53L8CH** (CX + CNH + production PID 0xED40) share one results-frame
/// layout; only the frame-tail geometry differs for decoding, and that is all
/// this enum selects. The footer-id offset is `size-12` (ULD 2.1.0, selected by
/// [`Variant::Cx`]) or `size-4` (ULD 2.0.16, selected by [`Variant::Ch`]).
///
/// Note the geometry tracks the *ULD version the firmware embeds*, not the
/// silicon: the DEPZ firmware streams 2.1.0-footer frames on both CX and CH
/// devices, so a CH capture still decodes with [`Variant::Cx`] geometry.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Variant {
    /// Base VL53L8CX (dev default), or any device on ULD 2.1.0 footer geometry.
    Cx,
    /// VL53L8CH on ULD 2.0.16 footer geometry.
    Ch,
}

impl Variant {
    fn footer_id_off(self) -> usize {
        match self {
            Variant::Cx => 12,
            Variant::Ch => 4,
        }
    }
}

/// Frame-decode failure.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Vl53l8Error {
    /// Header/footer id mismatch (`STATUS_CORRUPTED_FRAME`).
    CorruptedFrame,
    /// Frame shorter than the fixed 16-byte prologue.
    ShortFrame,
}

impl std::fmt::Display for Vl53l8Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Vl53l8Error::CorruptedFrame => write!(f, "VL53L8 corrupted frame"),
            Vl53l8Error::ShortFrame => write!(f, "VL53L8 frame too short"),
        }
    }
}

impl std::error::Error for Vl53l8Error {}

/// One decoded results frame. Per-zone arrays are sized to the resolution
/// actually present in the frame (`resolution` = `nb_target_detected.len()`).
#[derive(Debug, Clone, PartialEq)]
pub struct Vl53l8Results {
    /// mm, already scaled `floor(raw/4)` per ST `GetRangingData`.
    pub distance_mm: Vec<i32>,
    /// 5/9 = valid; 255 = no target detected in the zone.
    pub target_status: Vec<u8>,
    pub nb_target_detected: Vec<u8>,
    /// kcps/SPAD, raw fixed-point (÷2048 for real units).
    pub signal_per_spad: Vec<u32>,
    /// kcps/SPAD, raw fixed-point.
    pub ambient_per_spad: Vec<u32>,
    pub nb_spads_enabled: Vec<u32>,
    /// mm, raw fixed-point (÷128 for real units).
    pub range_sigma_mm_raw: Vec<u16>,
    /// reflectance %.
    pub reflectance: Vec<u8>,
    pub silicon_temp_degc: i8,
    /// Raw compact-network-histogram block bytes, present only on **VL53L8CH**
    /// frames that carry a CNH output block. Pass this to
    /// [`super::cnh::decode_cnh`] to unpack it into per-aggregate histograms.
    /// `None` on CX frames and on CH frames without a CNH block.
    pub cnh_raw: Option<Vec<u8>>,
}

impl Vl53l8Results {
    /// Active resolution (16 or 64), from the number of zones decoded.
    pub fn resolution(&self) -> usize {
        self.nb_target_detected.len()
    }
}

/// VL53L8CX `SwapBuffer`: byte-reverse every complete 32-bit word (a <4-byte
/// tail is left untouched).
pub fn swap_buffer(data: &[u8]) -> Vec<u8> {
    let mut out = data.to_vec();
    let n4 = (data.len() / 4) * 4;
    let mut i = 0;
    while i < n4 {
        out[i] = data[i + 3];
        out[i + 1] = data[i + 2];
        out[i + 2] = data[i + 1];
        out[i + 3] = data[i];
        i += 4;
    }
    out
}

/// union Block_header: `type[3:0]`, `size[15:4]`, `idx[31:16]`.
fn bh_fields(bh: u32) -> (u32, usize, u16) {
    (bh & 0xf, ((bh >> 4) & 0xfff) as usize, ((bh >> 16) & 0xffff) as u16)
}

fn u32_le(buf: &[u8], i: usize) -> u32 {
    u32::from_le_bytes([buf[i], buf[i + 1], buf[i + 2], buf[i + 3]])
}

fn u16_le(buf: &[u8], i: usize) -> u16 {
    u16::from_le_bytes([buf[i], buf[i + 1]])
}

fn i16_le(buf: &[u8], i: usize) -> i16 {
    i16::from_le_bytes([buf[i], buf[i + 1]])
}

/// Decode one raw results frame (`raw` = `full_size` bytes read from reg 0x00).
///
/// The frame's own length is authoritative (`data_read_size = raw.len()`); the
/// FW streams exactly the size advertised in each `RPT_VL53_FRAME` chunk.
pub fn parse_frame(raw: &[u8], variant: Variant) -> Result<Vl53l8Results, Vl53l8Error> {
    if raw.len() < 16 {
        return Err(Vl53l8Error::ShortFrame);
    }
    let data_read_size = raw.len();
    let buf = swap_buffer(raw);

    let max = RESOLUTION_8X8 * NB_TARGET_PER_ZONE;
    let mut distance_mm: Vec<i32> = vec![0; max];
    let mut target_status: Vec<u8> = vec![0; max];
    let mut nb_target_detected: Vec<u8> = vec![0; RESOLUTION_8X8];
    let mut signal_per_spad: Vec<u32> = vec![0; max];
    let mut ambient_per_spad: Vec<u32> = vec![0; RESOLUTION_8X8];
    let mut nb_spads_enabled: Vec<u32> = vec![0; RESOLUTION_8X8];
    let mut range_sigma_mm_raw: Vec<u16> = vec![0; max];
    let mut reflectance: Vec<u8> = vec![0; max];
    let mut silicon_temp_degc: i8 = 0;
    let mut cnh_raw: Option<Vec<u8>> = None;

    let mut i = 16usize;
    while i + 4 <= data_read_size {
        let bh = u32_le(&buf, i);
        let (bh_type, bh_size, bh_idx) = bh_fields(bh);
        let msize = if bh_type > 0x1 && bh_type < 0xd {
            bh_type as usize * bh_size
        } else {
            bh_size
        };
        // Exact-sized buffer: once a block would run past the end we have
        // reached the footer (all data blocks precede it).
        if i + 4 + msize > data_read_size {
            break;
        }

        if bh_idx == METADATA_IDX {
            silicon_temp_degc = buf[i + 12] as i8;
        } else if bh_idx == DISTANCE_IDX {
            let mut out = Vec::with_capacity(msize / 2);
            for k in 0..msize / 2 {
                out.push(i16_le(&buf, i + 4 + 2 * k) as i32);
            }
            distance_mm = out;
        } else if bh_idx == TARGET_STATUS_IDX {
            target_status = buf[i + 4..i + 4 + msize].to_vec();
        } else if bh_idx == NB_TARGET_DETECTED_IDX {
            nb_target_detected = buf[i + 4..i + 4 + msize].to_vec();
        } else if bh_idx == SIGNAL_RATE_IDX {
            let mut out = Vec::with_capacity(msize / 4);
            for k in 0..msize / 4 {
                out.push(u32_le(&buf, i + 4 + 4 * k));
            }
            signal_per_spad = out;
        } else if bh_idx == AMBIENT_RATE_IDX {
            let mut out = Vec::with_capacity(msize / 4);
            for k in 0..msize / 4 {
                out.push(u32_le(&buf, i + 4 + 4 * k));
            }
            ambient_per_spad = out;
        } else if bh_idx == SPAD_COUNT_IDX {
            let mut out = Vec::with_capacity(msize / 4);
            for k in 0..msize / 4 {
                out.push(u32_le(&buf, i + 4 + 4 * k));
            }
            nb_spads_enabled = out;
        } else if bh_idx == RANGE_SIGMA_MM_IDX {
            let mut out = Vec::with_capacity(msize / 2);
            for k in 0..msize / 2 {
                out.push(u16_le(&buf, i + 4 + 2 * k));
            }
            range_sigma_mm_raw = out;
        } else if bh_idx == REFLECTANCE_EST_PC_IDX {
            reflectance = buf[i + 4..i + 4 + msize].to_vec();
        } else if bh_idx == CNH_DATA_IDX {
            // CH extension point: capture the compact-network-histogram block
            // verbatim. Unpacking it into per-zone/per-bin histograms is a
            // CH-only feature that is not yet implemented — we surface the raw
            // bytes rather than fabricate a decode. Mirrors the live-ULD
            // register bridge, which is likewise left as a documented stub.
            cnh_raw = Some(buf[i + 4..i + 4 + msize].to_vec());
        }

        i += msize + 4;
    }

    // Fixed-point scaling per ST GetRangingData: distance is /4 (floor).
    for d in distance_mm.iter_mut() {
        *d = d.div_euclid(4);
    }

    // No target detected -> status 255. Iterate zones actually present.
    let nzones = nb_target_detected.len();
    for z in 0..nzones {
        if nb_target_detected[z] == 0 {
            for t in 0..NB_TARGET_PER_ZONE {
                let idx = NB_TARGET_PER_ZONE * z + t;
                if idx < target_status.len() {
                    target_status[idx] = 255;
                }
            }
        }
    }

    // Header/footer id match check. The footer-id offset is the only
    // variant-specific step in the whole decode: `size-12` for CX (ULD 2.1.0)
    // vs `size-4` for CH (ULD 2.0.16); everything above is shared CX/CH.
    let foff = variant.footer_id_off();
    if data_read_size >= foff + 2
        && (buf[0x8] != buf[data_read_size - foff] || buf[0x9] != buf[data_read_size - foff + 1])
    {
        return Err(Vl53l8Error::CorruptedFrame);
    }

    Ok(Vl53l8Results {
        distance_mm,
        target_status,
        nb_target_detected,
        signal_per_spad,
        ambient_per_spad,
        nb_spads_enabled,
        range_sigma_mm_raw,
        reflectance,
        silicon_temp_degc,
        cnh_raw,
    })
}
