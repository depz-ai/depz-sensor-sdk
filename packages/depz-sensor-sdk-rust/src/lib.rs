//! Contract-first Rust SDK for the DEPZ USB sensor line.
//!
//! Transport + protocol foundation (CRCs, packet framing/parsing, the USB id
//! table, common/identity/SR04 payload codecs, `.fwdepz` container parsing)
//! plus the verifiable sensor-decode layer:
//!
//! - [`vl53l8`] — one decode layer for BOTH ToF variants (VL53L8CX base and
//!   VL53L8CH = CX + CNH + PID 0xED40): frame reassembly, raw-frame decode,
//!   advanced DCI codecs, and CH-only CNH histogram decode. The live ULD
//!   register-bridge init/config is a documented extension point, out of scope.
//! - [`vl53l7`] — VL53L5CX / VL53L7CX / VL53L7CH (one `APP_VL53L7` I2C bridge,
//!   contract 11): the bridge's extra codecs and class resolution; frames
//!   decode through [`vl53l8`] with [`vl53l8::Variant::L7`].
//! - [`vl53l4`] — VL53L4CD single-zone ToF: register-bridge wire codecs,
//!   result-block decode, ULD timing/tuning register math, init config block.
//! - [`vl53lx`] — VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX (one
//!   `APP_VL53L0_4` register bridge, contract 12, protocol v2.00): the v2.00
//!   codecs, the product table and class resolution, and the stateless decode
//!   of the three streamed blocks (die result, VL53L0X raw, histogram raw).
//! - [`bno055`] — BNO055 9-axis IMU (one `APP_BNO055` register bridge,
//!   contract 13): the bridge wire codecs, the §4 register codecs (units,
//!   calibration, axis remap, page-1 configs) and register-window decode.
//! - [`bno086`] — SHTP framing, SH-2 report parsers, control request builders.
//! - [`dataset`] — `.depzdata` decoded multi-device dataset reader (contract 09).
//!
//! Everything here is byte-exact with the golden vectors in
//! `contracts/vectors/` and the Python/Java/C/C++ reference SDKs.

pub mod bno055;
pub mod bno086;
pub mod crc;
pub mod dataset;
pub mod framing;
pub mod fwdepz;
pub mod json;
pub mod protocol;
pub mod usb_ids;
pub mod vl53l4;
pub mod vl53l7;
pub mod vl53lx;
pub mod vl53l8;

pub use framing::{
    build_packet, CrcType, Event, FramingError, Packet, PacketParser, HEADER_SIZE, MAGIC,
    MAX_PAYLOAD,
};
pub use usb_ids::{is_known_depz_usb, usb_model_hint};
