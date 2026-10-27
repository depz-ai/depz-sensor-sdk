//! The device's register image, block by block (`image.py` / `regs.py` of the reference).

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::Vl53lxPlatform;

use super::tables::{BLOCKS, DEFAULTS, TUNING};

/// One field of a register block: byte offset, width in bytes, signedness and the mask the C codec
/// applies (`None`: none).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RegField {
    /// The C field name.
    pub name: &'static str,
    /// Byte offset inside the block.
    pub off: usize,
    /// Width in bytes, big-endian on the wire.
    pub width: usize,
    /// Two's complement over `width` bytes.
    pub signed: bool,
    /// The bits the C codec keeps.
    pub mask: Option<i64>,
}

/// One contiguous I2C block of the device image (a `VL53LX_*_t` struct of the C driver).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RegBlockDef {
    /// The block's name (the C struct without `VL53LX_` / `_t`).
    pub name: &'static str,
    /// First register address.
    pub base: u16,
    /// Bytes in the block, reserved holes included.
    pub size: usize,
    /// Whether the driver may write it.
    pub writable: bool,
    /// The named fields, in block order.
    pub fields: &'static [RegField],
}

/// The layout of the block `name`, or `None` when there is none.
pub fn block_def(name: &str) -> Option<&'static RegBlockDef> {
    BLOCKS.iter().find(|b| b.name == name)
}

/// A tuning parameter by its C field name (`tp_*`), as [`TUNING`] holds it after init.
pub fn tuning_default(name: &str) -> Option<i64> {
    TUNING.iter().find(|(k, _)| *k == name).map(|(_, v)| *v)
}

/// A raw tuning-header value (`TUNINGPARM_*_DEFAULT`) from [`DEFAULTS`].
pub fn header_default(name: &str) -> Option<i64> {
    DEFAULTS.iter().find(|(k, _)| *k == name).map(|(_, v)| *v)
}

/// One register block's worth of device image, addressed by the C field name:
/// `img.static_config().get("dss_config__target_total_rate_mcps")`. Bytes no field names (the
/// reserved holes) keep whatever was read, so a read-modify-write round trip is byte-exact.
#[derive(Debug, Clone)]
pub struct RegBlock {
    def: &'static RegBlockDef,
    values: Vec<i64>,
    raw: Vec<u8>,
}

impl RegBlock {
    /// A zeroed block of the layout `def`.
    pub fn from_def(def: &'static RegBlockDef) -> RegBlock {
        RegBlock {
            def,
            values: vec![0; def.fields.len()],
            raw: vec![0; def.size],
        }
    }

    /// A zeroed block by name, or `None` when there is no such block.
    pub fn named(name: &str) -> Option<RegBlock> {
        block_def(name).map(RegBlock::from_def)
    }

    /// The block's name.
    pub fn name(&self) -> &'static str {
        self.def.name
    }

    /// The block's layout.
    pub fn def(&self) -> &'static RegBlockDef {
        self.def
    }

    /// First register address.
    pub fn base(&self) -> u16 {
        self.def.base
    }

    /// Bytes in the block.
    pub fn size(&self) -> usize {
        self.def.size
    }

    fn index(&self, field: &str) -> Option<usize> {
        self.def.fields.iter().position(|f| f.name == field)
    }

    /// The field by its C name, or `None` when the block has none.
    pub fn try_get(&self, field: &str) -> Option<i64> {
        self.index(field).map(|i| self.values[i])
    }

    /// The field by its C name.
    ///
    /// # Panics
    /// When the block has no such field (the driver only names fields of the generated layout).
    pub fn get(&self, field: &str) -> i64 {
        self.try_get(field)
            .unwrap_or_else(|| panic!("{} has no field {}", self.def.name, field))
    }

    /// Set a field (the value is clipped only on [`RegBlock::encode`]); `false` when there is no such
    /// field.
    pub fn try_set(&mut self, field: &str, value: i64) -> bool {
        match self.index(field) {
            Some(i) => {
                self.values[i] = value;
                true
            }
            None => false,
        }
    }

    /// Set a field (the value is clipped only on [`RegBlock::encode`]).
    ///
    /// # Panics
    /// When the block has no such field.
    pub fn set(&mut self, field: &str, value: i64) {
        if !self.try_set(field, value) {
            panic!("{} has no field {}", self.def.name, field);
        }
    }

    /// Every field in block order, `(name, value)`.
    pub fn items(&self) -> Vec<(&'static str, i64)> {
        self.def
            .fields
            .iter()
            .zip(&self.values)
            .map(|(f, v)| (f.name, *v))
            .collect()
    }

    /// Take the block's bytes (at least [`RegBlock::size`] of them) into the fields.
    pub fn decode(&mut self, raw: &[u8]) -> Result<()> {
        if raw.len() < self.def.size {
            return Err(DepzError::InvalidArgument(format!(
                "{}: {} bytes, need {}",
                self.def.name,
                raw.len(),
                self.def.size
            )));
        }
        self.raw = raw[..self.def.size].to_vec();
        for (i, f) in self.def.fields.iter().enumerate() {
            let mut value: i64 = 0;
            for k in 0..f.width {
                value = (value << 8) | raw[f.off + k] as i64;
            }
            if let Some(m) = f.mask {
                value &= m;
            }
            let bits = (f.width * 8) as u32;
            if f.signed && value >= 1i64 << (bits - 1) {
                value -= 1i64 << bits;
            }
            self.values[i] = value;
        }
        Ok(())
    }

    /// The block's bytes: every field masked and clipped to its width the way the C codec does, the
    /// holes as last decoded.
    pub fn encode(&self) -> Vec<u8> {
        let mut buf = self.raw.clone();
        for (f, v) in self.def.fields.iter().zip(&self.values) {
            let bits = (f.width * 8) as u32;
            let all: i64 = if bits >= 64 { -1 } else { (1i64 << bits) - 1 };
            let mut value = *v;
            if let Some(m) = f.mask {
                value &= m;
            }
            value &= all;
            for k in (0..f.width).rev() {
                buf[f.off + k] = value as u8;
                value >>= 8;
            }
        }
        buf
    }
}

/// The blocks `init_and_start_range` pushes, in address order: one transfer.
pub const RANGE_START_BLOCKS: &[&str] = &[
    "static_nvm_managed",
    "customer_nvm_managed",
    "static_config",
    "general_config",
    "timing_config",
    "dynamic_config",
    "system_control",
];

/// The device's register image, block by block. The C driver never reads a configuration register
/// on its own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087
/// in a single 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the
/// count where the C driver has it. Editing a field only touches the host copy; [`DeviceImage::push`]
/// reaches the sensor.
#[derive(Debug, Clone)]
pub struct DeviceImage {
    blocks: Vec<RegBlock>,
}

impl Default for DeviceImage {
    fn default() -> Self {
        DeviceImage::new()
    }
}

macro_rules! accessors {
    ($($get:ident, $get_mut:ident, $name:literal;)*) => {
        $(
            #[doc = concat!("The `", $name, "` block.")]
            pub fn $get(&self) -> &RegBlock {
                self.block($name)
            }

            #[doc = concat!("The `", $name, "` block, to edit.")]
            pub fn $get_mut(&mut self) -> &mut RegBlock {
                self.block_mut($name)
            }
        )*
    };
}

impl DeviceImage {
    /// Every block of [`BLOCKS`], zeroed.
    pub fn new() -> DeviceImage {
        DeviceImage {
            blocks: BLOCKS.iter().map(RegBlock::from_def).collect(),
        }
    }

    /// The block `name`, or `None` when there is none.
    pub fn try_block(&self, name: &str) -> Option<&RegBlock> {
        self.blocks.iter().find(|b| b.name() == name)
    }

    /// The block `name`.
    ///
    /// # Panics
    /// When there is no such block.
    pub fn block(&self, name: &str) -> &RegBlock {
        self.try_block(name)
            .unwrap_or_else(|| panic!("no register block {name}"))
    }

    /// The block `name`, to edit.
    ///
    /// # Panics
    /// When there is no such block.
    pub fn block_mut(&mut self, name: &str) -> &mut RegBlock {
        self.blocks
            .iter_mut()
            .find(|b| b.name() == name)
            .unwrap_or_else(|| panic!("no register block {name}"))
    }

    accessors! {
        static_nvm, static_nvm_mut, "static_nvm_managed";
        customer_nvm, customer_nvm_mut, "customer_nvm_managed";
        static_config, static_config_mut, "static_config";
        general_config, general_config_mut, "general_config";
        timing_config, timing_config_mut, "timing_config";
        dynamic_config, dynamic_config_mut, "dynamic_config";
        system_control, system_control_mut, "system_control";
        nvm_copy_data, nvm_copy_data_mut, "nvm_copy_data";
    }

    /// Read one block off the sensor into the image.
    pub fn pull(&mut self, p: &dyn Vl53lxPlatform, name: &str) -> Result<()> {
        let (base, size) = {
            let b = self.block(name);
            (b.base(), b.size())
        };
        let raw = p.read_multi(base, size)?;
        self.block_mut(name).decode(&raw)
    }

    /// Write one block from the image to the sensor.
    pub fn push(&self, p: &dyn Vl53lxPlatform, name: &str) -> Result<()> {
        let blk = self.block(name);
        if !blk.def().writable {
            return Err(DepzError::InvalidState(format!("{name} is read-only")));
        }
        p.write_multi(blk.base(), &blk.encode())
    }

    // (base, total size) covering the named blocks; reserved bytes between them are part of the span.
    fn span(&self, names: &[&str]) -> (u16, usize) {
        let mut lo = usize::MAX;
        let mut hi = 0usize;
        for n in names {
            let b = self.block(n);
            lo = lo.min(b.base() as usize);
            hi = hi.max(b.base() as usize + b.size());
        }
        (lo as u16, hi - lo)
    }

    /// Read the named blocks as one transfer.
    pub fn pull_range(&mut self, p: &dyn Vl53lxPlatform, names: &[&str]) -> Result<()> {
        let (base, size) = self.span(names);
        let raw = p.read_multi(base, size)?;
        for n in names {
            let blk = self.block_mut(n);
            let off = (blk.base() - base) as usize;
            let part = raw[off..off + blk.size()].to_vec();
            blk.decode(&part)?;
        }
        Ok(())
    }

    /// The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver.
    pub fn push_range(&self, p: &dyn Vl53lxPlatform, names: &[&str]) -> Result<()> {
        let (base, size) = self.span(names);
        let mut buf = vec![0u8; size];
        for n in names {
            let blk = self.block(n);
            if !blk.def().writable {
                return Err(DepzError::InvalidState(format!("{n} is read-only")));
            }
            let enc = blk.encode();
            let off = (blk.base() - base) as usize;
            buf[off..off + enc.len()].copy_from_slice(&enc);
        }
        p.write_multi(base, &buf)
    }

    /// Every field of the named blocks, one per line.
    pub fn dump(&self, names: &[&str]) -> String {
        let mut lines = Vec::new();
        for n in names {
            let blk = self.block(n);
            lines.push(format!(
                "--- {} @ 0x{:04X} ({} B)",
                n,
                blk.base(),
                blk.size()
            ));
            for (k, v) in blk.items() {
                lines.push(format!("  {k:<52} {v}"));
            }
        }
        lines.join("\n")
    }

    /// Every field of every block, one per line.
    pub fn dump_all(&self) -> String {
        let names: Vec<&str> = BLOCKS.iter().map(|b| b.name).collect();
        self.dump(&names)
    }
}
