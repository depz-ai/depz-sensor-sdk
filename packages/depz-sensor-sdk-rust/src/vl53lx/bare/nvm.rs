//! The raw NVM, which the register blocks do not cover: the optical centre, the 5x5 peak-rate map,
//! the mode-mitigation offset calibration and the FMT range results. Each word costs four writes and
//! a read, so only the regions the driver wants are read, each inside its own enable/disable pair so
//! the sensor is never left with its firmware stopped (vl53lx_nvm.c).

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::Vl53lxPlatform;

use super::image::DeviceImage;

/// `VL53LX_optical_centre_t`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct OpticalCentre {
    /// X centre, 4.4 fixed point SPADs.
    pub x_centre: i64,
    /// Y centre, 4.4 fixed point SPADs.
    pub y_centre: i64,
}

/// `VL53LX_cal_peak_rate_map_t`.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct CalPeakRateMap {
    /// Calibration distance, mm.
    pub cal_distance_mm: i64,
    /// Calibration reflectance, %.
    pub cal_reflectance_pc: i64,
    /// Samples in the map.
    pub max_samples: i64,
    /// Map width.
    pub width: i64,
    /// Map height.
    pub height: i64,
    /// The peak rates, 9.7 Mcps, row by row.
    pub peak_rate_mcps: Vec<i64>,
}

/// `VL53LX_additional_offset_cal_data_t`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct AdditionalOffsetCalData {
    /// MM1 effective SPADs, 1/256.
    pub mm_inner_actual_effective_spads: i64,
    /// MM2 effective SPADs, 1/256.
    pub mm_outer_actual_effective_spads: i64,
    /// MM1 peak signal rate, 9.7 Mcps.
    pub mm_inner_peak_signal_count_rtn_mcps: i64,
    /// MM2 peak signal rate, 9.7 Mcps.
    pub mm_outer_peak_signal_count_rtn_mcps: i64,
}

/// `VL53LX_decoded_nvm_fmt_range_data_t`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct FmtRangeData {
    /// Effective return SPADs, 1/256.
    pub actual_effective_rtn_spads: i64,
    /// Requested reference SPADs.
    pub num_requested_ref_spads: i64,
    /// Reference SPAD location.
    pub ref_location: i64,
    /// Return peak signal rate, 9.7 Mcps.
    pub peak_signal_count_rate_rtn_mcps: i64,
    /// Return ambient rate, 9.7 Mcps.
    pub ambient_count_rate_rtn_mcps: i64,
    /// Reference peak signal rate, 9.7 Mcps.
    pub peak_signal_count_rate_ref_mcps: i64,
    /// Reference ambient rate, 9.7 Mcps.
    pub ambient_count_rate_ref_mcps: i64,
    /// Measured distance, mm.
    pub measured_distance_mm: i64,
    /// Its standard deviation, mm.
    pub measured_distance_stdev_mm: i64,
}

const RANGING_CORE_CLK_CTRL1: u16 = 0x0683;
const NVM_CTRL_MODE: u16 = 0x0780;
const NVM_CTRL_PDN: u16 = 0x0781;
const NVM_CTRL_READN: u16 = 0x0783;
const NVM_CTRL_PULSE_WIDTH_MSB: u16 = 0x0784;
const NVM_CTRL_DATAOUT_MMM: u16 = 0x0790;
const NVM_CTRL_ADDR: u16 = 0x0794;
const NVM_CTRL_PULSE_WIDTH: u16 = 0x0004;

/// NVM region of the optical centre, (byte index, byte size).
pub const FMT_OPTICAL_CENTRE: (usize, usize) = (0x00B8, 4);
/// NVM region of the peak-rate map.
pub const FMT_CAL_PEAK_RATE_MAP: (usize, usize) = (0x015C, 56);
/// NVM region of the mode-mitigation offset calibration.
pub const FMT_ADDITIONAL_OFFSET_CAL: (usize, usize) = (0x0194, 8);
/// NVM region of the FMT range results at 140 mm, dark.
pub const FMT_RANGE_RESULTS_140MM_DARK: (usize, usize) = (0x01AC, 16);

const PEAK_RATE_MAP_WIDTH: i64 = 5;
const PEAK_RATE_MAP_HEIGHT: i64 = 5;
const PEAK_RATE_MAP_SAMPLES: usize = 25;

fn u16_at(buf: &[u8], off: usize) -> i64 {
    ((buf[off] as i64) << 8) | buf[off + 1] as i64
}

fn enable(p: &dyn Vl53lxPlatform, img: &mut DeviceImage) -> Result<()> {
    // disable_firmware / enable_powerforce, through system_control so the image stays truthful.
    img.system_control_mut().set("firmware__enable", 0);
    p.write_byte(super::regs::FIRMWARE_ENABLE, 0)?;
    img.system_control_mut()
        .set("power_management__go1_power_force", 1);
    p.write_byte(super::regs::POWER_MANAGEMENT_GO1_POWER_FORCE, 1)?;
    p.sleep_ms(1); // 250 us settling, rounded up

    p.write_byte(NVM_CTRL_PDN, 0x01)?;
    p.write_byte(RANGING_CORE_CLK_CTRL1, 0x05)?;
    p.sleep_ms(1); // 50 us power-up, rounded up
    p.write_byte(NVM_CTRL_MODE, 0x01)?;
    p.write_word(NVM_CTRL_PULSE_WIDTH_MSB, NVM_CTRL_PULSE_WIDTH)
}

fn disable(p: &dyn Vl53lxPlatform, img: &mut DeviceImage) -> Result<()> {
    p.write_byte(NVM_CTRL_READN, 0x01)?;
    p.write_byte(NVM_CTRL_PDN, 0x00)?;
    img.system_control_mut()
        .set("power_management__go1_power_force", 0);
    p.write_byte(super::regs::POWER_MANAGEMENT_GO1_POWER_FORCE, 0)?;
    img.system_control_mut().set("firmware__enable", 1);
    p.write_byte(super::regs::FIRMWARE_ENABLE, 1)
}

fn read_words(p: &dyn Vl53lxPlatform, start_word: usize, count: usize) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(count * 4);
    for addr in start_word..start_word + count {
        p.write_byte(NVM_CTRL_ADDR, addr as u8)?;
        p.write_byte(NVM_CTRL_READN, 0x00)?;
        // 5 us trigger delay: the next USB round trip is the wait.
        p.write_byte(NVM_CTRL_READN, 0x01)?;
        out.extend_from_slice(&p.read_multi(NVM_CTRL_DATAOUT_MMM, 4)?);
    }
    Ok(out)
}

/// `VL53LX_read_nvm_raw_data()`: (byte index, byte size), both word-aligned → the bytes. The image
/// is passed so the firmware / power-force fields it toggles stay truthful.
pub fn read_region(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
    region: (usize, usize),
) -> Result<Vec<u8>> {
    let (index, size) = region;
    if index & 3 != 0 || size & 3 != 0 {
        return Err(DepzError::InvalidArgument(format!(
            "NVM region 0x{index:04X}+{size} is not word-aligned"
        )));
    }
    enable(p, img)?;
    let words = read_words(p, index >> 2, size >> 2);
    // Disable even when the read failed, so the firmware is not left stopped.
    let off = disable(p, img);
    let words = words?;
    off?;
    Ok(words)
}

/// The optical centre (zeroed on a never-programmed part).
pub fn read_optical_centre(p: &dyn Vl53lxPlatform, img: &mut DeviceImage) -> Result<OpticalCentre> {
    let buf = read_region(p, img, FMT_OPTICAL_CENTRE)?;
    // Stored as a distance down from 0x0100; on a never-programmed part the driver zeroes it, which is
    // what makes read_p2p_data fall back to the mm ROI centre.
    let x = 0x0100 - buf[2] as i64;
    Ok(OpticalCentre {
        x_centre: if x <= 0xFF { x } else { 0 },
        y_centre: buf[3] as i64,
    })
}

/// The 5x5 calibration peak-rate map.
pub fn read_cal_peak_rate_map(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
) -> Result<CalPeakRateMap> {
    let buf = read_region(p, img, FMT_CAL_PEAK_RATE_MAP)?;
    Ok(CalPeakRateMap {
        cal_distance_mm: u16_at(&buf, 0),
        cal_reflectance_pc: u16_at(&buf, 2) >> 6,
        max_samples: PEAK_RATE_MAP_SAMPLES as i64,
        width: PEAK_RATE_MAP_WIDTH,
        height: PEAK_RATE_MAP_HEIGHT,
        peak_rate_mcps: (0..PEAK_RATE_MAP_SAMPLES)
            .map(|i| u16_at(&buf, 4 + 2 * i))
            .collect(),
    })
}

/// The mode-mitigation offset calibration.
pub fn read_additional_offset_cal_data(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
) -> Result<AdditionalOffsetCalData> {
    let buf = read_region(p, img, FMT_ADDITIONAL_OFFSET_CAL)?;
    Ok(AdditionalOffsetCalData {
        mm_inner_actual_effective_spads: u16_at(&buf, 0),
        mm_outer_actual_effective_spads: u16_at(&buf, 2),
        mm_inner_peak_signal_count_rtn_mcps: u16_at(&buf, 4),
        mm_outer_peak_signal_count_rtn_mcps: u16_at(&buf, 6),
    })
}

/// The FMT range results of `region` ([`FMT_RANGE_RESULTS_140MM_DARK`] is the one the driver reads).
pub fn read_fmt_range_results(
    p: &dyn Vl53lxPlatform,
    img: &mut DeviceImage,
    region: (usize, usize),
) -> Result<FmtRangeData> {
    let buf = read_region(p, img, region)?;
    Ok(FmtRangeData {
        actual_effective_rtn_spads: u16_at(&buf, 0),
        num_requested_ref_spads: buf[2] as i64,
        ref_location: buf[3] as i64,
        peak_signal_count_rate_rtn_mcps: u16_at(&buf, 4),
        ambient_count_rate_rtn_mcps: u16_at(&buf, 6),
        peak_signal_count_rate_ref_mcps: u16_at(&buf, 8),
        ambient_count_rate_ref_mcps: u16_at(&buf, 10),
        measured_distance_mm: u16_at(&buf, 12),
        measured_distance_stdev_mm: u16_at(&buf, 14),
    })
}
