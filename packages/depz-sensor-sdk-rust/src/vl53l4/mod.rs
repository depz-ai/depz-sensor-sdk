//! VL53L4CD decode layer (contract 10) — single-zone ToF register bridge.
//!
//! The **VL53L4CD** is a single-zone Time-of-Flight sensor (~1.3 m) behind a
//! thin I2C register bridge: the MCU owns nothing but the I2C bus, the XSHUT
//! and INT pins and one streaming FSM. The full ST VL53L4CD Ultra-Lite Driver
//! (ULD 2.2.3, STSW-IMG026) runs on the host as plain register access — one
//! 17-byte result block per measurement, no zone grids, no firmware download.
//!
//! - [`codecs`] — wire codecs: command payload packers (`VL53_READ_REG` ..
//!   `VL53_SET_I2C_SPEED`) and report unpackers (`RPT_VL53_REG_DATA` /
//!   `RPT_VL53_INFO` / `RPT_VL53_STREAM`).
//! - [`uld_math`] — pure host-ULD codecs: the result-block decode
//!   ([`parse_result_block`]), the `SetRangeTiming`/`GetRangeTiming` register
//!   math, tuning-register word codecs and the 91-byte init config block.
//!
//! The live ULD (ST's VL53L4CD ULD 2.2.3 on the host) is the crate-private
//! `driver` module: [`crate::devices::Vl53l4cdDevice`] runs it; its
//! [`crate::devices::DetectionThresholds`] is re-exported from `devices`.

pub mod codecs;
pub(crate) mod driver;
pub mod uld_math;

pub use codecs::{
    i2c_error_name, pack_read_reg, pack_set_i2c_speed, pack_start_stream, pack_write_reg,
    pack_xshut, try_pack_read_reg, try_pack_write_reg, RegData, StreamData, Vl53l4Cmd, Vl53l4Info,
    Vl53l4Rpt, I2C_KHZ_STEPS, SF_INT_ACT_HIGH, XFER_MAX, XSHUT_OFF, XSHUT_ON, XSHUT_RESET,
};
pub use uld_math::{
    check_range_timing, config_block, decode_offset, decode_range_timing, decode_sigma_threshold,
    decode_signal_threshold, decode_xtalk, parse_result_block, range_status_name,
    range_timing_registers, sigma_threshold_raw, signal_threshold_raw, try_offset_raw,
    try_xtalk_raw, Vl53l4Error, Vl53l4Results, CONFIG_ADDR, CONFIG_FMP_BYTE, DEFAULT_CONFIGURATION,
    I2C_KHZ_BOOT, I2C_KHZ_DEFAULT, MODEL_ID_VL53L4CD, RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN,
    STATUS_RTN,
};
