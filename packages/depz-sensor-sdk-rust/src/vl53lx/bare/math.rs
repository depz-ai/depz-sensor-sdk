//! SPAD geometry and timing maths (vl53lx_core.c, vl53lx_core_support.c).

use super::image::{DeviceImage, RegBlock};
use super::nvm::AdditionalOffsetCalData;
use super::regs;

/// A SPAD's (row, col) → its number.
pub fn encode_row_col(row: i64, col: i64) -> i64 {
    if row > 7 {
        (128 + (col << 3) + (15 - row)) & 0xFF
    } else {
        (((15 - col) << 3) + row) & 0xFF
    }
}

/// A SPAD number → (row, col).
pub fn decode_row_col(spad_number: i64) -> (i64, i64) {
    if spad_number > 127 {
        (8 + ((255 - spad_number) & 0x07), (spad_number - 128) >> 3)
    } else {
        (spad_number & 0x07, (127 - spad_number) >> 3)
    }
}

/// An encoded zone size → (width, height).
pub fn decode_zone_size(encoded_xy_size: i64) -> (i64, i64) {
    (encoded_xy_size & 0x0F, encoded_xy_size >> 4)
}

/// (width, height) → the encoded zone size.
pub fn encode_zone_size(width: i64, height: i64) -> i64 {
    ((height << 4) + width) & 0xFF
}

/// → (x_ll, y_ll, x_ur, y_ur), clipped to the SPAD array.
pub fn decode_zone_limits(encoded_xy_centre: i64, encoded_xy_size: i64) -> (i64, i64, i64, i64) {
    let (y_centre, x_centre) = decode_row_col(encoded_xy_centre);
    let (width, height) = decode_zone_size(encoded_xy_size);
    let x_ll = (x_centre - (width + 1).div_euclid(2)).max(0);
    let x_ur = (regs::SPAD_ARRAY_WIDTH - 1).min(x_ll + width);
    let y_ll = (y_centre - (height + 1).div_euclid(2)).max(0);
    let y_ur = (regs::SPAD_ARRAY_HEIGHT - 1).min(y_ll + height);
    (x_ll, y_ll, x_ur, y_ur)
}

/// Whether the SPAD at (row, col) sits under the aperture.
pub fn is_aperture_location(row: i64, col: i64) -> bool {
    (row % 4 == 0 && col % 4 == 2) || (row % 4 == 2 && col % 4 == 0)
}

/// → (mm inner, mm outer) effective SPADs, in 1/256 of a SPAD. Needed because our boards ship
/// without the FMT offset calibration: the driver works out the SPAD counts the nominal rates apply
/// to.
pub fn calc_mm_effective_spads(
    encoded_mm_roi_centre: i64,
    encoded_mm_roi_size: i64,
    encoded_zone_centre: i64,
    encoded_zone_size: i64,
    good_spads: &[u8; 32],
    aperture_attenuation: i64,
) -> (i64, i64) {
    let mm = decode_zone_limits(encoded_mm_roi_centre, encoded_mm_roi_size);
    let z = decode_zone_limits(encoded_zone_centre, encoded_zone_size);
    let mut inner = 0;
    let mut outer = 0;
    for y in z.1..=z.3 {
        for x in z.0..=z.2 {
            let spad = encode_row_col(y, x);
            if good_spads[(spad >> 3) as usize] & (1 << (spad & 0x07)) == 0 {
                continue;
            }
            let attenuation = if is_aperture_location(y, x) {
                aperture_attenuation
            } else {
                regs::RTN_SPAD_UNITY_TRANSMISSION
            };
            if mm.0 <= x && x <= mm.2 && mm.1 <= y && y <= mm.3 {
                inner += attenuation;
            } else {
                outer += attenuation;
            }
        }
    }
    (inner, outer)
}

/// `VL53LX_hist_combine_mm1_mm2_offsets()` → `range_offset_mm`, in quarter millimetres: the MM1
/// (inner) and MM2 (outer) offsets weighted by the share of each region the ROI covers.
#[allow(clippy::too_many_arguments)] // ST's parameter list, kept 1:1
pub fn combine_mm1_mm2_offsets(
    mm1_offset_mm: i64,
    mm2_offset_mm: i64,
    encoded_mm_roi_centre: i64,
    encoded_mm_roi_size: i64,
    encoded_zone_centre: i64,
    encoded_zone_size: i64,
    cal: &AdditionalOffsetCalData,
    good_spads: &[u8; 32],
    aperture_attenuation: i64,
) -> i64 {
    let max = calc_mm_effective_spads(
        encoded_mm_roi_centre,
        encoded_mm_roi_size,
        0xC7,
        0xFF,
        good_spads,
        aperture_attenuation,
    );
    if max.0 == 0 || max.1 == 0 {
        return 0;
    }
    let cur = calc_mm_effective_spads(
        encoded_mm_roi_centre,
        encoded_mm_roi_size,
        encoded_zone_centre,
        encoded_zone_size,
        good_spads,
        aperture_attenuation,
    );
    let mm1_rate = cal.mm_inner_peak_signal_count_rtn_mcps * cur.0 / max.0;
    let mm2_rate = cal.mm_outer_peak_signal_count_rtn_mcps * cur.1 / max.1;
    let total = mm1_rate + mm2_rate;
    if total == 0 {
        return 0;
    }
    // Signed offsets: C truncates towards zero, which Rust division does too.
    (mm1_offset_mm * mm1_rate + mm2_offset_mm * mm2_rate) * 4 / total
}

/// The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap.
pub fn rtn_good_spads(nvm_copy_data: &RegBlock) -> [u8; 32] {
    let mut out = [0u8; 32];
    for (i, o) in out.iter_mut().enumerate() {
        *o = nvm_copy_data.get(&format!("global_config__spad_enables_rtn_{i}")) as u8;
    }
    out
}

/// The PLL period, 2^-30 s units (0 for a zero frequency).
pub fn calc_pll_period_us(fast_osc_frequency: i64) -> i64 {
    if fast_osc_frequency > 0 {
        (1i64 << 30) / fast_osc_frequency
    } else {
        0
    }
}

/// A VCSEL period register value → PLL clocks.
pub fn decode_vcsel_period(vcsel_period_reg: i64) -> i64 {
    (vcsel_period_reg + 1) << 1
}

/// The macro period, 12.12 µs.
pub fn calc_macro_period_us(fast_osc_frequency: i64, vcsel_period_reg: i64) -> i64 {
    let mut macro_us = regs::MACRO_PERIOD_VCSEL_PERIODS * calc_pll_period_us(fast_osc_frequency);
    macro_us >>= 6;
    macro_us *= decode_vcsel_period(vcsel_period_reg);
    macro_us >> 6
}

/// A timeout in µs → macro periods.
pub fn calc_timeout_mclks(timeout_us: i64, macro_period_us: i64) -> i64 {
    if macro_period_us == 0 {
        0
    } else {
        ((timeout_us << 12) + (macro_period_us >> 1)) / macro_period_us
    }
}

/// Macro periods → the (mantissa, exponent) register encoding.
pub fn encode_timeout(timeout_mclks: i64) -> i64 {
    if timeout_mclks <= 0 {
        return 0;
    }
    let mut ls = timeout_mclks - 1;
    let mut ms = 0;
    while ls & 0xFFFF_FF00 != 0 {
        ls >>= 1;
        ms += 1;
    }
    ((ms << 8) + (ls & 0xFF)) & 0xFFFF
}

/// The register encoding → macro periods. The exponent shift wraps at 64 as Java's does (a real
/// register never comes near it).
pub fn decode_timeout(encoded: i64) -> i64 {
    ((encoded & 0x00FF).wrapping_shl(((encoded & 0xFF00) >> 8) as u32)).wrapping_add(1)
}

/// A timeout in µs → its register encoding.
pub fn calc_encoded_timeout(timeout_us: i64, macro_period_us: i64) -> i64 {
    encode_timeout(calc_timeout_mclks(timeout_us, macro_period_us))
}

/// `VL53LX_duration_maths()`: a window duration, µs.
pub fn duration_maths(
    pll_period_us: i64,
    vcsel_parm_pclks: i64,
    window_vclks: i64,
    elapsed_mclks: i64,
) -> i64 {
    let mut duration = window_vclks.wrapping_mul(pll_period_us) >> 12;
    duration = duration.wrapping_mul(elapsed_mclks.wrapping_mul(vcsel_parm_pclks) >> 4);
    (duration >> 12).min(0xFFFF_FFFF)
}

/// Fill the six timeout registers, for VCSEL period A and then B. `None` when the oscillator
/// frequency is 0.
pub fn calc_timeout_register_values(
    phasecal_us: i64,
    mm_us: i64,
    range_us: i64,
    fast_osc_frequency: i64,
    img: &mut DeviceImage,
) -> Option<()> {
    if fast_osc_frequency == 0 {
        return None;
    }
    let mut macro_us = calc_macro_period_us(
        fast_osc_frequency,
        img.timing_config().get("range_config__vcsel_period_a"),
    );
    img.general_config_mut().set(
        "phasecal_config__timeout_macrop",
        0xFF.min(calc_timeout_mclks(phasecal_us, macro_us)),
    );
    let tim = img.timing_config_mut();
    let encoded = calc_encoded_timeout(mm_us, macro_us);
    tim.set("mm_config__timeout_macrop_a_hi", encoded >> 8);
    tim.set("mm_config__timeout_macrop_a_lo", encoded & 0xFF);
    let encoded = calc_encoded_timeout(range_us, macro_us);
    tim.set("range_config__timeout_macrop_a_hi", encoded >> 8);
    tim.set("range_config__timeout_macrop_a_lo", encoded & 0xFF);

    macro_us = calc_macro_period_us(fast_osc_frequency, tim.get("range_config__vcsel_period_b"));
    let encoded = calc_encoded_timeout(mm_us, macro_us);
    tim.set("mm_config__timeout_macrop_b_hi", encoded >> 8);
    tim.set("mm_config__timeout_macrop_b_lo", encoded & 0xFF);
    let encoded = calc_encoded_timeout(range_us, macro_us);
    tim.set("range_config__timeout_macrop_b_hi", encoded >> 8);
    tim.set("range_config__timeout_macrop_b_lo", encoded & 0xFF);
    Some(())
}
