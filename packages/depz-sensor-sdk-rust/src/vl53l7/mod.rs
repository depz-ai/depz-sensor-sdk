//! VL53L5CX / VL53L7CX / VL53L7CH decode layer (contract 11, a delta against
//! contract 04).
//!
//! One firmware (`APP_VL53L7_*`) serves three boards that differ only in the
//! soldered sensor: **VL53L7CX** (base), **VL53L5CX** (same API, narrower
//! optics) and **VL53L7CH** (CX + compact-network-histogram output). The MCU
//! is a thin I2C register bridge; results frames share the VL53L8 layout, so
//! they decode through [`crate::vl53l8::parse_frame`] with
//! [`Variant::L7`] (footer id at `size-4`, per-zone arrays trimmed to the
//! frame's resolution), and CNH through [`crate::vl53l8::decode_cnh`].
//!
//! - [`codecs`] — the bridge's extra wire codecs (`VL53_PIN_CTRL`,
//!   `VL53_GET_INFO`, `VL53_SET_I2C_SPEED`, `RPT_VL53_INFO`) and its transfer
//!   ceilings; the shared VL53L8 codecs are re-exported here.
//! - [`resolve_model`] — which of the three sensors a board opens as.
//!
//! The live ULD register-bridge init/config is a documented extension point,
//! out of scope (as for VL53L8).

pub mod codecs;

pub use crate::vl53l8::codecs::{pack_read_reg, pack_start_stream, pack_write_reg, RegData};
pub use crate::vl53l8::decode::Variant;
pub use codecs::{
    i2c_error_name, pack_pin_ctrl, pack_set_i2c_speed, Vl53l7Cmd, Vl53l7Info, Vl53l7Rpt,
    I2C_SPEED_STEPS_KHZ, INFO_SIZE, PIN_I2C_RST, PIN_LPN_OFF, PIN_LPN_ON, PIN_SOFT_CYCLE,
    READ_MAX_LEN, STREAM_CHUNK_MAX, WRITE_MAX_LEN,
};

/// Unlike VL53L8 (≥ 2 Hz), L5/L7 range and stream at 1 Hz.
pub const MIN_RANGING_FREQUENCY_HZ: u32 = 1;

/// The sensor class an `APP_VL53L7` board opens as.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Vl53l7Model {
    Vl53l5cx,
    Vl53l7cx,
    Vl53l7ch,
}

impl Vl53l7Model {
    /// Lowercase class name (matches the golden vectors).
    pub fn as_str(&self) -> &'static str {
        match self {
            Vl53l7Model::Vl53l5cx => "vl53l5cx",
            Vl53l7Model::Vl53l7cx => "vl53l7cx",
            Vl53l7Model::Vl53l7ch => "vl53l7ch",
        }
    }

    /// True for the CNH-capable class (VL53L7CH).
    pub fn has_cnh(&self) -> bool {
        matches!(self, Vl53l7Model::Vl53l7ch)
    }

    fn from_str(s: &str) -> Option<Vl53l7Model> {
        match s {
            "vl53l5cx" => Some(Vl53l7Model::Vl53l5cx),
            "vl53l7cx" => Some(Vl53l7Model::Vl53l7cx),
            "vl53l7ch" => Some(Vl53l7Model::Vl53l7ch),
            _ => None,
        }
    }
}

/// First match of `VL53L([57])(CX|CH)` in `name` → `(digit, suffix)`.
fn find_part(name: &str) -> Option<(u8, &str)> {
    let b = name.as_bytes();
    const PREFIX: &[u8] = b"VL53L";
    if b.len() < PREFIX.len() + 3 {
        return None;
    }
    for i in 0..=b.len() - (PREFIX.len() + 3) {
        if &b[i..i + PREFIX.len()] != PREFIX {
            continue;
        }
        let d = b[i + PREFIX.len()];
        let suffix = &b[i + PREFIX.len() + 1..i + PREFIX.len() + 3];
        if (d == b'5' || d == b'7') && (suffix == b"CX" || suffix == b"CH") {
            let s = if suffix == b"CX" { "cx" } else { "ch" };
            return Some((d - b'0', s));
        }
    }
    None
}

/// Resolve the L5/L7 class (contract 11 §1, pinned by `vl53l7.json` `model`):
/// 1. the production USB PID model (`usb_model`, e.g. from
///    [`crate::usb_model_hint`]) when it names one of the three;
/// 2. else the first `VL53L([57])(CX|CH)` in `GET_DEVICE_NAME`
///    (a part without its own class, e.g. `VL53L5CH`, falls back to CX);
/// 3. else [`Vl53l7Model::Vl53l7cx`] — its blob runs on every L5/L7 part.
pub fn resolve_model(usb_model: Option<&str>, device_name: &str) -> Vl53l7Model {
    if let Some(m) = usb_model.and_then(Vl53l7Model::from_str) {
        return m;
    }
    if let Some((digit, suffix)) = find_part(device_name) {
        return Vl53l7Model::from_str(&format!("vl53l{}{}", digit, suffix))
            .unwrap_or(Vl53l7Model::Vl53l7cx);
    }
    Vl53l7Model::Vl53l7cx
}
