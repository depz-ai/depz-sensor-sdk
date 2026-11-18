//! The embedded TMF8829 sensor firmware (contracts/14_SENSOR_TMF8829.md §4).
//!
//! ams `tmf8829_application.hex`, application 2.200.0, one contiguous
//! segment converted to a flat binary at build time (14 444 B, already a
//! multiple of 4 → 3611 words, sha256 `973c89b7…a20f8`). **MIT** — the
//! license text ships next to it ([`FIRMWARE_LICENSE`]). The host downloads
//! it through the sensor bootloader (`W_FIFO_BOTH` at [`FIRMWARE_LOAD_ADDR`],
//! then `0xFF` in ≤ 1023 B pieces).

/// Where the sensor bootloader loads the image.
pub const FIRMWARE_LOAD_ADDR: u32 = 0x0001_0000;

/// The image bytes.
pub static FIRMWARE_IMAGE: &[u8] = include_bytes!("data/tmf8829_application.bin");

/// The image's MIT license (ams-OSRAM), to be shipped with any binary that
/// embeds it.
pub static FIRMWARE_LICENSE: &str = include_str!("data/LICENSES-MIT.TXT");

/// The sensor firmware image downloaded by init (14 444 B).
pub fn firmware_image() -> &'static [u8] {
    FIRMWARE_IMAGE
}

/// Image size in 32-bit words, as the bootloader `W_FIFO_BOTH` takes it.
pub fn firmware_words() -> u16 {
    (FIRMWARE_IMAGE.len() / 4) as u16
}
