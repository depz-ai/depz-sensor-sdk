//! BNO055 9-axis IMU decode layer (contract 13, protocol v0.10).
//!
//! The `APP_BNO055_v*` firmware is a thin register bridge: the MCU owns the
//! I2C bus, the reset/INT pins and one streaming loop, while Bosch's sensor
//! fusion runs on the chip. Operating mode, units, axis remap, calibration
//! and decoding are host logic expressed as register access.
//!
//! - [`codecs`] — the wire codecs: `BNO_*` command payloads (0x32..0x37) and
//!   the `RPT_BNO_REG_DATA` / `RPT_BNO_INFO` / `RPT_BNO_REG_STREAM` reports.
//! - [`regs`] — the register map and the §4 pure codecs: UNIT_SEL flags and
//!   LSB scales, CALIB_STAT, the 22-byte calibration profile, axis remap +
//!   placements P0..P7, the page-1 accel/gyro/mag configs, and the decoding
//!   of any register window into raw channel integers.
//!
//! The live driver (mode switching, boot/fusion-start polls, page discipline,
//! scaled samples) is a documented extension point, out of scope for this
//! decode-layer crate.

pub mod codecs;
pub mod regs;

pub use crate::vl53l4::i2c_error_name;
pub use codecs::{
    pack_get_info, pack_read_reg, pack_reset, pack_start_stream, pack_stop_stream,
    pack_write_reg, Bno055Cmd, Bno055Info, Bno055Rpt, RegData, StreamData, EXPECTED_ACC_ID,
    EXPECTED_CHIP_ID, EXPECTED_GYR_ID, EXPECTED_MAG_ID, I2C_ADDR, INFO_SIZE, TRIGGER_INT,
    TRIGGER_TIMER, XFER_MAX,
};
pub use regs::{
    decode_block, pack_sic_matrix, unpack_sic_matrix, AccelConfig, AxisRemap, CalibStatus,
    CalibrationProfile, GyroConfig, MagConfig, OprMode, PwrMode, RawBlock, TempSource, Units,
    CALIB_PROFILE_LEN, FULL_BLOCK, FUSION_ACCEL_LSB, MAG_LSB, PLACEMENTS, QUAT_BLOCK, QUAT_LSB,
    SIC_IDENTITY,
};
