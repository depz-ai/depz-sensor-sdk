//! TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4).

use crate::protocol::common::CodecError;

/// Config page 0x22..=0xDF.
pub const REG_CFG: u8 = 0x22;
/// 190 B.
pub const CFG_SIZE: usize = 0xDF - REG_CFG as usize + 1;

/// Preset name → load command (each loads its config page; then
/// LOAD_CONFIG_PAGE).
pub const PRESETS: &[(&str, u8)] = &[
    ("8x8", 0x40),
    ("8x8-lr", 0x41), // long range
    ("8x8-ha", 0x42), // high accuracy
    ("16x16", 0x43),
    ("16x16-ha", 0x44),
    ("32x32", 0x45),
    ("32x32-ha", 0x46),
    ("48x32", 0x47),
    ("48x32-ha", 0x48),
    ("8x8-xr", 0x49), // extended range, distances in mm
];

/// The preset's own `kilo_iterations` (a preset rescales a stale value, it
/// does not reset it — SDKs write it on every configure).
pub const PRESET_KILO_ITERATIONS: &[(&str, u16)] = &[
    ("8x8", 586),
    ("8x8-lr", 586),
    ("8x8-ha", 586),
    ("8x8-xr", 586),
    ("16x16", 586),
    ("16x16-ha", 586),
    ("32x32", 2344),
    ("32x32-ha", 2344),
    ("48x32", 3516),
    ("48x32-ha", 3516),
];

/// The sensor's own `period_ms` after init.
pub const DEFAULT_PERIOD_MS: u16 = 33;

/// Load command of a preset by name.
pub fn preset_cmd(name: &str) -> Option<u8> {
    PRESETS.iter().find(|(n, _)| *n == name).map(|(_, c)| *c)
}

/// The preset's own `kilo_iterations` by name.
pub fn preset_kilo_iterations(name: &str) -> Option<u16> {
    PRESET_KILO_ITERATIONS
        .iter()
        .find(|(n, _)| *n == name)
        .map(|(_, k)| *k)
}

/// One named config-page field: register, bit shift, width in bits
/// (width 16 = little-endian register pair).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CfgField {
    pub name: &'static str,
    pub reg: u8,
    pub shift: u8,
    pub width: u8,
}

const fn fld(name: &'static str, reg: u8, shift: u8, width: u8) -> CfgField {
    CfgField {
        name,
        reg,
        shift,
        width,
    }
}

/// The contract's field table. The raw page is always available for
/// anything else.
pub const CFG_FIELDS: &[CfgField] = &[
    fld("period_ms", 0x22, 0, 16),
    fld("kilo_iterations", 0x24, 0, 16),
    fld("fp_mode", 0x26, 0, 3),
    fld("nr_peaks", 0x2A, 0, 3),
    fld("signal_strength", 0x2A, 3, 1),
    fld("noise_strength", 0x2A, 4, 1),
    fld("xtalk", 0x2A, 5, 1),
    fld("dump_histograms", 0x2B, 0, 1),
    fld("ref_spad_frame", 0x2C, 0, 1),
    fld("power_modes", 0x2E, 0, 8),
    fld("ha_kilo_iterations", 0x4A, 0, 16),
    fld("dual_mode", 0x4C, 0, 2),
    fld("fov_correction", 0x78, 0, 4),
    fld("distance_in_mm", 0x8F, 0, 1),
];

/// A field of [`CFG_FIELDS`] by name.
pub fn cfg_field(name: &str) -> Option<&'static CfgField> {
    CFG_FIELDS.iter().find(|f| f.name == name)
}

fn locate(page_len: usize, name: &str) -> Result<(&'static CfgField, usize), CodecError> {
    let f = cfg_field(name).ok_or(CodecError("unknown config field"))?;
    let i = (f.reg - REG_CFG) as usize;
    let need = if f.width == 16 { i + 2 } else { i + 1 };
    if page_len < need {
        return Err(CodecError("config page too short for the field"));
    }
    Ok((f, i))
}

/// Read a named field from a config page (register 0x22 at index 0).
pub fn cfg_get(page: &[u8], name: &str) -> Result<u16, CodecError> {
    let (f, i) = locate(page.len(), name)?;
    if f.width == 16 {
        return Ok(u16::from_le_bytes([page[i], page[i + 1]]));
    }
    Ok(((page[i] as u16) >> f.shift) & ((1u16 << f.width) - 1))
}

/// Write a named field into a config page; refuses a value that does not fit
/// the field width (the page is then untouched).
pub fn cfg_set(page: &mut [u8], name: &str, value: u32) -> Result<(), CodecError> {
    let (f, i) = locate(page.len(), name)?;
    if value >= 1u32 << f.width {
        return Err(CodecError("config value out of range for the field width"));
    }
    if f.width == 16 {
        page[i] = (value & 0xFF) as u8;
        page[i + 1] = (value >> 8) as u8;
    } else {
        let mask = (((1u32 << f.width) - 1) << f.shift) as u8;
        page[i] = (page[i] & !mask) | ((value << f.shift) as u8);
    }
    Ok(())
}

/// Every named field of a config page, in table order.
pub fn config_fields(page: &[u8]) -> Vec<(&'static str, u16)> {
    CFG_FIELDS
        .iter()
        .filter_map(|f| cfg_get(page, f.name).ok().map(|v| (f.name, v)))
        .collect()
}
