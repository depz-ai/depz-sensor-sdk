//! VL53L5CX / VL53L7CX / VL53L7CH multizone ToF on the I2C bridge board (contract 11): the
//! VL53L8 protocol plus three board commands, and the same ST ULD family on the host.

use std::ops::Deref;

use super::device::{expect_report, DepzDevice};
use super::error::{DepzError, Result};
use super::vl53l8::{Profile, Vl53l8cxDevice};
use crate::vl53l7::{
    pack_pin_ctrl, pack_set_i2c_speed, Vl53l7Cmd, Vl53l7Info, Vl53l7Rpt, MIN_RANGING_FREQUENCY_HZ,
    PIN_LPN_OFF, PIN_SOFT_CYCLE, READ_MAX_LEN, STREAM_CHUNK_MAX, WRITE_MAX_LEN,
};
use crate::vl53l8::driver::{Vl53l8CnhSetup, Vl53l8Firmware, MODULE_TYPE_MZ, MODULE_TYPE_MZEVO};

const L7CX: Profile = Profile {
    firmware: Vl53l8Firmware::L7cx,
    read_chunk: READ_MAX_LEN,
    write_chunk: WRITE_MAX_LEN,
    stream_chunk: STREAM_CHUNK_MAX,
    min_hz: MIN_RANGING_FREQUENCY_HZ as i32,
    expected_module: Some(MODULE_TYPE_MZEVO),
    class: "Vl53l7cxDevice",
};

const L5CX: Profile = Profile {
    expected_module: Some(MODULE_TYPE_MZ),
    class: "Vl53l5cxDevice",
    ..L7CX
};

const L7CH: Profile = Profile {
    firmware: Vl53l8Firmware::L7ch,
    class: "Vl53l7chDevice",
    ..L7CX
};

/// VL53L7CX multizone ToF (8×8 zones, 90° field of view) on the I2C bridge board (contract 11): a
/// [`Vl53l8cxDevice`] (every method, through `Deref`) with the L5/L7 firmware set (ULD 2.0.1),
/// smaller register reads, ranging from 1 Hz, and the board commands
/// [`Vl53l7cxDevice::get_bridge_info`], [`Vl53l7cxDevice::set_i2c_speed_khz`],
/// [`Vl53l7cxDevice::pin_ctrl`]. L5CX / L7CX have no deep sleep and no threshold auto-stop.
pub struct Vl53l7cxDevice(Vl53l8cxDevice);

impl Deref for Vl53l7cxDevice {
    type Target = Vl53l8cxDevice;

    fn deref(&self) -> &Vl53l8cxDevice {
        &self.0
    }
}

impl Vl53l7cxDevice {
    /// Continue the open connection `dev` as a VL53L7CX.
    pub fn attach(dev: DepzDevice) -> Vl53l7cxDevice {
        Vl53l7cxDevice(Vl53l8cxDevice::with_profile(dev, &L7CX))
    }

    /// The sensor's module type read at `init`: [`MODULE_TYPE_MZ`] (VL53L5CX) or
    /// [`MODULE_TYPE_MZEVO`] (VL53L7CX / CH); `None` before.
    pub fn module_type(&self) -> Option<u8> {
        self.0.with_uld(|u| Ok(u.module_type())).ok().flatten()
    }

    /// The last `calibrate_xtalk` found nothing to calibrate ("coverglass too good").
    pub fn xtalk_calibration_failed(&self) -> bool {
        self.0
            .with_uld(|u| Ok(u.xtalk_calibration_failed()))
            .unwrap_or(false)
    }

    // ── board commands (contract 11 §2) ─────────────────────────────────────

    /// Bridge counters and pin levels (the sensor is not touched). Read it before and after a run:
    /// during one each call takes the bus from the stream and can cost a frame.
    pub fn get_bridge_info(&self) -> Result<Vl53l7Info> {
        self.request(
            Vl53l7Cmd::GetInfo as u8,
            &[],
            expect_report(Vl53l7Rpt::Info as u8, Vl53l7Info::unpack),
            None,
        )
    }

    /// Sensor bus SCL; the board takes the nearest of 100, 200, 400, 500 … 1000 kHz. Returns the
    /// value in effect. A busy status mid-transfer ([`DepzError::is_busy`]): stop ranging first.
    pub fn set_i2c_speed_khz(&self, khz: u32) -> Result<u16> {
        if !(1..=0xFFFF).contains(&khz) {
            return Err(DepzError::InvalidArgument(format!(
                "khz must be 1..65535, got {khz}"
            )));
        }
        self.request_ok(
            Vl53l7Cmd::SetI2cSpeed as u8,
            &pack_set_i2c_speed(khz as u16),
            None,
        )?;
        Ok(self.get_bridge_info()?.i2c_khz)
    }

    /// Drive the sensor's LPn / I2C_RST pins ([`crate::vl53l7::PIN_LPN_OFF`] …). LPN_OFF and
    /// SOFT_CYCLE drop the sensor's state: `init` again afterwards.
    pub fn pin_ctrl(&self, action: u8) -> Result<()> {
        self.request_ok(Vl53l7Cmd::PinCtrl as u8, &pack_pin_ctrl(action), None)?;
        if action == PIN_LPN_OFF || action == PIN_SOFT_CYCLE {
            self.0.forget_sensor();
        }
        Ok(())
    }
}

/// VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of
/// [`Vl53l7cxDevice`]; the sensor reports module type MZ.
pub struct Vl53l5cxDevice(Vl53l7cxDevice);

impl Deref for Vl53l5cxDevice {
    type Target = Vl53l7cxDevice;

    fn deref(&self) -> &Vl53l7cxDevice {
        &self.0
    }
}

impl Vl53l5cxDevice {
    /// Continue the open connection `dev` as a VL53L5CX.
    pub fn attach(dev: DepzDevice) -> Vl53l5cxDevice {
        Vl53l5cxDevice(Vl53l7cxDevice(Vl53l8cxDevice::with_profile(dev, &L5CX)))
    }
}

/// VL53L7CH: the VL53L7CX superset with CNH histograms. `init` downloads the CH firmware (VL53LMZ
/// ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream in chunks.
pub struct Vl53l7chDevice(Vl53l7cxDevice);

impl Deref for Vl53l7chDevice {
    type Target = Vl53l7cxDevice;

    fn deref(&self) -> &Vl53l7cxDevice {
        &self.0
    }
}

impl Vl53l7chDevice {
    /// Continue the open connection `dev` as a VL53L7CH.
    pub fn attach(dev: DepzDevice) -> Vl53l7chDevice {
        Vl53l7chDevice(Vl53l7cxDevice(Vl53l8cxDevice::with_profile(dev, &L7CH)))
    }

    /// The CNH setup armed by [`Vl53l7chDevice::configure_cnh`], or `None`.
    pub fn cnh_setup(&self) -> Option<Vl53l8CnhSetup> {
        self.armed_cnh()
    }

    /// Arm CNH for the next start (as [`super::Vl53l8chDevice::configure_cnh`]); disarms the motion
    /// indicator (one DCI block).
    pub fn configure_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()> {
        self.arm_cnh(setup)
    }
}
