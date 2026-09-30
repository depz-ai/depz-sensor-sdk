//! VL53L8 decode layer (contract 04) — serves BOTH ToF variants.
//!
//! The DEPZ line ships two distinct ToF sensors that share this one module:
//! - **VL53L8CX** — the base 8×8 multizone Time-of-Flight sensor (dev default).
//! - **VL53L8CH** — CX plus compact-network-histogram (CNH) output and its own
//!   production USB PID `0xED40`.
//!
//! Their results-frame layout is identical, so framing, decode and the advanced
//! DCI codecs are shared with no duplicated CH path; the only variant-specific
//! step is the frame-tail footer-id offset, selected by [`decode::Variant`].
//! The VL53L5CX / VL53L7CX / VL53L7CH frames (contract 11, [`crate::vl53l7`])
//! decode here too, under [`decode::Variant::L7`].
//!
//! - [`codecs`] — register-bridge command packers + `RPT_VL53_REG_DATA`.
//! - [`framing`] — `RPT_VL53_FRAME` chunk codec + multi-chunk reassembly.
//! - [`decode`] — raw results-frame → per-zone arrays (ULD `GetRangingData`).
//! - [`advanced`] — pure DCI codecs (motion, xtalk margin, detection thresholds).
//! - [`cnh`] — CH-only compact-network-histogram decode: the raw CNH block
//!   ([`decode::Vl53l8Results::cnh_raw`]) → per-aggregate integer histograms.
//!
//! One thing is intentionally left as a documented extension point: the live ULD
//! init/config register bridge (firmware download, DCI writes over the wire),
//! which is hardware-dependent.

pub mod advanced;
pub mod cnh;
pub mod codecs;
pub mod decode;
pub mod framing;

pub use cnh::{decode_cnh, CnhAggregate, CnhData, CnhDecodeConfig, CnhError};
pub use codecs::{pack_read_reg, pack_start_stream, pack_write_reg, RegData, Vl53l8Cmd, Vl53l8Rpt};
pub use decode::{parse_frame, Variant, Vl53l8Error, Vl53l8Results, RESOLUTION_4X4, RESOLUTION_8X8};
pub use framing::{unpack_frame_chunk, CompletedFrame, FrameChunk, FrameReassembler};
