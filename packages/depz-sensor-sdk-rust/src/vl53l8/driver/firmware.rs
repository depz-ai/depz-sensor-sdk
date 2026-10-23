//! The sensor firmware sets and their blobs (compiled in from `src/vl53l8/data`).

/// The sensor firmware a [`super::Vl53l8Driver`] downloads.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum Vl53l8Firmware {
    /// VL53L8CX, ST ULD 2.1.0.
    #[default]
    Cx,
    /// VL53L8CH, VL53LMZ ULD 2.0.16 (CNH).
    Ch,
    /// VL53L5CX / VL53L7CX over I2C, ULD 2.0.1.
    L7cx,
    /// VL53L7CH over I2C, VL53LMZ ULD 2.0.16 with the VL53L7 configuration.
    L7ch,
}

// The array types pin the firmware size (0x15000 bytes, three banks): a wrong blob fails the build.
const FW_CX: &[u8; 0x15000] = include_bytes!("../data/cx/firmware.bin");
const FW_CH: &[u8; 0x15000] = include_bytes!("../data/ch/firmware.bin");
const FW_L7CX: &[u8; 0x15000] = include_bytes!("../data/l7cx/firmware.bin");
const CFG_CX: &[u8] = include_bytes!("../data/cx/default_configuration.bin");
const CFG_CH: &[u8] = include_bytes!("../data/ch/default_configuration.bin");
const CFG_L7CX: &[u8] = include_bytes!("../data/l7cx/default_configuration.bin");
const CFG_L7CH: &[u8] = include_bytes!("../data/l7ch/default_configuration.bin");
// Identical in every set (the C generator checks it): the driver takes the cx copies.
const DEFAULT_XTALK: &[u8; 776] = include_bytes!("../data/cx/default_xtalk.bin");
const GET_NVM_CMD: &[u8] = include_bytes!("../data/cx/get_nvm_cmd.bin");

impl Vl53l8Firmware {
    /// Every firmware set, in declaration order.
    pub const ALL: [Vl53l8Firmware; 4] = [Vl53l8Firmware::Cx, Vl53l8Firmware::Ch, Vl53l8Firmware::L7cx, Vl53l8Firmware::L7ch];

    /// The blob set name under `vl53l8/data` (`cx`, `ch`, `l7cx`, `l7ch`).
    pub fn set_name(self) -> &'static str {
        match self {
            Vl53l8Firmware::Cx => "cx",
            Vl53l8Firmware::Ch => "ch",
            Vl53l8Firmware::L7cx => "l7cx",
            Vl53l8Firmware::L7ch => "l7ch",
        }
    }

    /// The 84 KB sensor firmware image (l7ch runs the ch firmware).
    pub(crate) fn image(self) -> &'static [u8] {
        match self {
            Vl53l8Firmware::Cx => FW_CX,
            Vl53l8Firmware::Ch | Vl53l8Firmware::L7ch => FW_CH,
            Vl53l8Firmware::L7cx => FW_L7CX,
        }
    }

    /// The default configuration the ULD uploads at init and after a calibration.
    pub(crate) fn default_configuration(self) -> &'static [u8] {
        match self {
            Vl53l8Firmware::Cx => CFG_CX,
            Vl53l8Firmware::Ch => CFG_CH,
            Vl53l8Firmware::L7cx => CFG_L7CX,
            Vl53l8Firmware::L7ch => CFG_L7CH,
        }
    }

    /// The default 776-byte xtalk buffer.
    pub(crate) fn default_xtalk() -> &'static [u8] {
        DEFAULT_XTALK
    }

    /// The command that reads the NVM offset data.
    pub(crate) fn get_nvm_cmd() -> &'static [u8] {
        GET_NVM_CMD
    }
}
