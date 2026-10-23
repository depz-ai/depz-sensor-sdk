//! The ULD itself: [`Vl53l8Driver`].

use super::error::{status, UldResult, Vl53l8UldError};
use super::firmware::Vl53l8Firmware;
use super::frame::{Vl53l8Frame, Vl53l8Motion};
use super::tables;
use super::Vl53l8Platform;
use crate::vl53l8::advanced::{
    thresh_scale, try_default_motion_config, try_pack_detection_thresholds, try_xtalk_margin_to_raw,
    DetectionThreshold, MotionConfig, NB_THRESHOLDS, POWER_MODE_DEEP_SLEEP, POWER_MODE_SLEEP, POWER_MODE_WAKEUP,
};
use crate::vl53l8::decode::{self, swap_buffer, Variant, NB_TARGET_PER_ZONE, RESOLUTION_4X4, RESOLUTION_8X8};

/// `set_ranging_mode`: continuous ranging (the integration time has no effect).
pub const RANGING_MODE_CONTINUOUS: u8 = 1;
/// `set_ranging_mode`: autonomous ranging (frames at the ranging frequency, set integration time).
pub const RANGING_MODE_AUTONOMOUS: u8 = 3;
/// `set_target_order`: closest target first.
pub const TARGET_ORDER_CLOSEST: u8 = 1;
/// `set_target_order`: strongest target first.
pub const TARGET_ORDER_STRONGEST: u8 = 2;

/// [`Vl53l8Driver::module_type`]: VL53L5CX.
pub const MODULE_TYPE_MZ: u8 = 0;
/// [`Vl53l8Driver::module_type`]: VL53L7CX / VL53L7CH.
pub const MODULE_TYPE_MZEVO: u8 = 1;
/// [`Vl53l8Driver::module_type`]: VL53L8.
pub const MODULE_TYPE_MZPLUS: u8 = 2;

/// Bytes of NVM data the ULD reads at init.
pub const NVM_DATA_SIZE: usize = 492;
/// Bytes of the offset buffer the ULD uploads.
pub const OFFSET_BUFFER_SIZE: usize = 488;
/// Bytes of the xtalk calibration buffer (save / restore with the caldata calls).
pub const XTALK_BUFFER_SIZE: usize = 776;

const UI_CMD_STATUS: u16 = 0x2C00;
const UI_CMD_START: u16 = 0x2C04;
const UI_CMD_END: u16 = 0x2FFF;

const DCI_ZONE_CONFIG: u16 = 0x5450;
const DCI_FREQ_HZ: u16 = 0x5458;
const DCI_INT_TIME: u16 = 0x545C;
const DCI_RANGING_MODE: u16 = 0xAD30;
const DCI_DSS_CONFIG: u16 = 0xAD38;
const DCI_TARGET_ORDER: u16 = 0xAE64;
const DCI_SHARPENER: u16 = 0xAED8;
const DCI_SINGLE_RANGE: u16 = 0xD964;
const DCI_OUTPUT_CONFIG: u16 = 0xD968;
const DCI_OUTPUT_ENABLES: u16 = 0xD970;
const DCI_OUTPUT_LIST: u16 = 0xD980;
const DCI_PIPE_CONTROL: u16 = 0xDB80;
const DCI_FW_FLAGS: u16 = 0xE0C4;
const DCI_CAL_CFG: u16 = 0x5470;
const DCI_XTALK_CFG: u16 = 0xAD94;
const DCI_MOTION_DETECTOR_CFG: u16 = 0xBFAC;
const DCI_DET_THRESH_CONFIG: u16 = 0x5488;
const DCI_DET_THRESH_GLOBAL_CONFIG: u16 = 0xB6E0;
const DCI_DET_THRESH_START: u16 = 0xB6E8;
const DCI_DET_THRESH_VALID_STATUS: u16 = 0xB9F0;

const START_BH: u32 = 0x0000000D;
const METADATA_BH: u32 = 0x54B400C0;
const COMMONDATA_BH: u32 = 0x54C00040;
const AMBIENT_RATE_BH: u32 = 0x54D00104;
const SPAD_COUNT_BH: u32 = 0x55D00404;
const NB_TARGET_DETECTED_BH: u32 = 0xDB840401;
const SIGNAL_RATE_BH: u32 = 0xDBC40404;
const RANGE_SIGMA_MM_BH: u32 = 0xDEC40402;
const DISTANCE_BH: u32 = 0xDF440402;
const REFLECTANCE_BH: u32 = 0xE0440401;
const TARGET_STATUS_BH: u32 = 0xE0840401;
const MOTION_DETECT_BH: u32 = 0xD85808C0;
const CNH_DATA_IDX: u32 = 0xC048;
const MOTION_DETEC_IDX: u32 = 0xD858;

const NB_TARGET: u32 = NB_TARGET_PER_ZONE as u32;

/// The ST VL53L8CX ULD 2.1.0 / VL53LMZ ULD 2.0.16 / VL53L5CX-L7CX ULD 2.0.1 on the host.
///
/// Ported from the reference Python driver: init (sensor firmware download), resolution,
/// frequency, mode, integration time, sharpener, target order, ranging, frame parsing, power
/// modes, xtalk margin and calibration, detection thresholds, motion indicator. Register sequences
/// are a 1:1 port — every request matches the Python one, so captures replay in both. `Send`
/// (the platform is `Send + Sync`): a device class holds it in a `Mutex`.
pub struct Vl53l8Driver<P: Vl53l8Platform> {
    p: P,
    firmware: Vl53l8Firmware,
    i2c: bool,
    fw_checksum: Option<u32>,
    output_enable_w3: u32,
    frame_tail: u32,
    variant: Variant,
    offset_data: Vec<u8>,
    xtalk_data: Vec<u8>,
    stream_count: i32,
    motion_present: bool,
    module_type: Option<u8>,
    data_read_size: usize,
    frame_size_mismatch: Option<(usize, usize)>,
    resolution: Option<usize>,
    xtalk_calibration_failed: bool,
}

impl<P: Vl53l8Platform> Vl53l8Driver<P> {
    /// A driver over `platform` for `firmware` ([`Vl53l8Firmware::Cx`] is the default). Nothing
    /// is sent until the first call.
    pub fn new(platform: P, firmware: Vl53l8Firmware) -> Self {
        let i2c = matches!(firmware, Vl53l8Firmware::L7cx | Vl53l8Firmware::L7ch);
        let fw_checksum = match firmware {
            Vl53l8Firmware::Cx => Some(0xCADF7CAF),
            Vl53l8Firmware::L7cx => None, // ULD 2.0.1 has no checksum step
            _ => Some(0x0C0B6C9E),
        };
        // Output enables word 3: 0 on the LMZ firmware; the frame tail and footer-id offset differ
        // between ULD 2.1.0 (cx) and the 2.0.x drivers.
        let output_enable_w3 = if matches!(firmware, Vl53l8Firmware::Ch | Vl53l8Firmware::L7ch) { 0 } else { 0xC0000000 };
        let frame_tail = if firmware == Vl53l8Firmware::Cx { 32 } else { 24 };
        // Cx: footer id at size-12; Ch: size-4, no trim (the L5/L7 trim to the zones in use is
        // done here, to the resolution start_ranging programmed).
        let variant = if firmware == Vl53l8Firmware::Cx { Variant::Cx } else { Variant::Ch };
        Vl53l8Driver {
            p: platform,
            firmware,
            i2c,
            fw_checksum,
            output_enable_w3,
            frame_tail,
            variant,
            offset_data: Vec::new(),
            xtalk_data: Vec::new(),
            stream_count: 255,
            motion_present: false,
            module_type: None,
            data_read_size: 0,
            frame_size_mismatch: None,
            resolution: None,
            xtalk_calibration_failed: false,
        }
    }

    /// The platform the driver runs on.
    pub fn platform(&self) -> &P {
        &self.p
    }

    /// The sensor firmware this driver downloads.
    pub fn firmware(&self) -> Vl53l8Firmware {
        self.firmware
    }

    /// The L5/L7 boards: the sensor sits on I2C (other boot sequence, per-target blocks keep 64
    /// entries in 4×4).
    pub fn is_i2c(&self) -> bool {
        self.i2c
    }

    /// L5/L7: [`MODULE_TYPE_MZ`] (VL53L5CX) or [`MODULE_TYPE_MZEVO`] (VL53L7CX/CH), read during
    /// [`Vl53l8Driver::init`]; else `None`.
    pub fn module_type(&self) -> Option<u8> {
        self.module_type
    }

    /// The frame size the firmware streams (bytes), set by [`Vl53l8Driver::start_ranging`].
    pub fn data_read_size(&self) -> usize {
        self.data_read_size
    }

    /// (firmware, host) frame sizes when the firmware reported another size than the ULD formula;
    /// else `None`.
    pub fn frame_size_mismatch(&self) -> Option<(usize, usize)> {
        self.frame_size_mismatch
    }

    /// Zones the last [`Vl53l8Driver::start_ranging`] used, or `None`.
    pub fn resolution(&self) -> Option<usize> {
        self.resolution
    }

    /// The last [`Vl53l8Driver::calibrate_xtalk`] ran into "coverglass too good": the default
    /// xtalk buffer is kept.
    pub fn xtalk_calibration_failed(&self) -> bool {
        self.xtalk_calibration_failed
    }

    /// The 776-byte xtalk buffer the driver uploads (default, calibrated or restored); empty
    /// before [`Vl53l8Driver::init`].
    pub fn xtalk_data(&self) -> &[u8] {
        &self.xtalk_data
    }

    /// Whether the silicon ids are the sensor's: (0xF0, 0x0C) on L8 over SPI; revision 0x02, or
    /// 0x01 with device id 0xF0, on L5/L7 over I2C.
    pub fn silicon_id_matches(device_id: u8, revision_id: u8, i2c: bool) -> bool {
        if i2c {
            revision_id == 0x02 || (device_id == 0xF0 && revision_id == 0x01)
        } else {
            device_id == 0xF0 && revision_id == 0x0C
        }
    }

    // ── low level ────────────────────────────────────────────────────────────

    /// `read_multi` that refuses a short answer (the C driver would read past it).
    fn read(&self, addr: u16, size: usize) -> UldResult<Vec<u8>> {
        let buf = self.p.read_multi(addr, size)?;
        if buf.len() < size {
            return Err(Vl53l8UldError::Platform(crate::devices::DepzError::Protocol(format!(
                "VL53L8 read 0x{addr:04X}: {} of {size} bytes",
                buf.len()
            ))));
        }
        Ok(buf)
    }

    fn rd(&self, addr: u16) -> UldResult<u8> {
        Ok(self.read(addr, 1)?[0])
    }

    fn wr(&self, addr: u16, value: u8) -> UldResult<()> {
        Ok(self.p.write_multi(addr, &[value])?)
    }

    fn write(&self, addr: u16, data: &[u8]) -> UldResult<()> {
        Ok(self.p.write_multi(addr, data)?)
    }

    /// `_vl53l8cx_poll_for_answer`: until `buf[pos] & mask == expected`; 10 ms period, 2 s timeout.
    fn poll_for_answer(&self, size: usize, pos: usize, address: u16, mask: u8, expected: u8, at: &str) -> UldResult<()> {
        let mut timeout = 0;
        loop {
            let buf = self.read(address, size)?;
            self.p.sleep_ms(10);
            if timeout >= 200 {
                return Err(Vl53l8UldError::status(status::TIMEOUT_ERROR, at));
            }
            if size >= 4 && buf[2] >= 0x7F {
                return Err(Vl53l8UldError::status(status::MCU_ERROR, at));
            }
            timeout += 1;
            if buf[pos] & mask == expected {
                return Ok(());
            }
        }
    }

    fn poll_for_mcu_boot(&self) -> UldResult<()> {
        for _ in 0..500 {
            let go2_status0 = self.rd(0x06)?;
            if go2_status0 & 0x80 != 0 && self.rd(0x07)? & 0x01 != 0 {
                return Ok(());
            }
            self.p.sleep_ms(1);
            if go2_status0 & 0x01 != 0 {
                return Ok(());
            }
        }
        Err(Vl53l8UldError::status(status::TIMEOUT_ERROR, "mcu boot"))
    }

    // ── DCI access ───────────────────────────────────────────────────────────

    /// `vl53l8cx_dci_read_data`: `data_size` bytes of DCI `index` (byte order fixed: the swap is
    /// undone).
    pub fn dci_read_data(&self, index: u16, data_size: usize) -> UldResult<Vec<u8>> {
        let mut cmd = [0u8; 12];
        cmd[0] = (index >> 8) as u8;
        cmd[1] = index as u8;
        cmd[2] = ((data_size & 0xFF0) >> 4) as u8;
        cmd[3] = ((data_size & 0xF) << 4) as u8;
        cmd[7] = 0x0F;
        cmd[9] = 0x02;
        cmd[11] = 0x08;
        self.write(UI_CMD_END - 11, &cmd)?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, &format!("dci read 0x{index:04X}"))?;
        let buf = swap_buffer(&self.read(UI_CMD_START, data_size + 12)?);
        Ok(buf[4..4 + data_size].to_vec())
    }

    /// `vl53l8cx_dci_write_data`: write `data` to DCI `index`.
    pub fn dci_write_data(&self, index: u16, data: &[u8]) -> UldResult<()> {
        let n = data.len();
        if n + 12 > UI_CMD_END as usize + 1 {
            return Err(Vl53l8UldError::InvalidArgument(format!("dci write 0x{index:04X}: {n} bytes do not fit")));
        }
        let mut buf = vec![0u8; n + 12];
        buf[0] = (index >> 8) as u8;
        buf[1] = index as u8;
        buf[2] = ((n & 0xFF0) >> 4) as u8;
        buf[3] = ((n & 0xF) << 4) as u8;
        buf[4..4 + n].copy_from_slice(&swap_buffer(data));
        let tail = [0x00, 0x00, 0x00, 0x0F, 0x05, 0x01, ((n + 8) >> 8) as u8, (n + 8) as u8];
        buf[4 + n..].copy_from_slice(&tail);
        self.write(UI_CMD_END - (n as u16 + 12) + 1, &buf)?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, &format!("dci write 0x{index:04X}"))
    }

    /// `vl53l8cx_dci_replace_data`: read `data_size` bytes of DCI `index`, put `new_data` at
    /// `new_data_pos`, write it back.
    pub fn dci_replace_data(&self, index: u16, data_size: usize, new_data: &[u8], new_data_pos: usize) -> UldResult<()> {
        if new_data_pos + new_data.len() > data_size {
            return Err(Vl53l8UldError::InvalidArgument(format!(
                "dci replace 0x{index:04X}: {} bytes at {new_data_pos} past {data_size}",
                new_data.len()
            )));
        }
        let mut data = self.dci_read_data(index, data_size)?;
        data[new_data_pos..new_data_pos + new_data.len()].copy_from_slice(new_data);
        self.dci_write_data(index, &data)
    }

    // ── offset / xtalk upload ────────────────────────────────────────────────

    fn send_offset_data(&self, res: usize) -> UldResult<()> {
        let mut buf = vec![0u8; OFFSET_BUFFER_SIZE];
        let n = OFFSET_BUFFER_SIZE.min(self.offset_data.len());
        buf[..n].copy_from_slice(&self.offset_data[..n]);
        if res == RESOLUTION_4X4 {
            buf[0x10..0x18].copy_from_slice(&[0x0F, 0x04, 0x04, 0x00, 0x08, 0x10, 0x10, 0x07]);
            buf = swap_buffer(&buf);
            let mut signal = [0i64; 64];
            let mut range = [0i64; 64];
            for k in 0..64 {
                signal[k] = u32le(&buf, 0x3C + 4 * k) as i64;
                range[k] = i16::from_le_bytes([buf[0x140 + 2 * k], buf[0x141 + 2 * k]]) as i64;
            }
            average_4x4(&mut signal);
            average_4x4(&mut range);
            for k in 0..64 {
                put_u32le(&mut buf, 0x3C + 4 * k, signal[k] as u32);
                let r = range[k].clamp(-32768, 32767);
                buf[0x140 + 2 * k] = r as u8;
                buf[0x141 + 2 * k] = (r >> 8) as u8;
            }
            buf = swap_buffer(&buf);
        }
        // Drop the 8-byte NVM header and append the footer at 0x1E0.
        let mut output = vec![0u8; OFFSET_BUFFER_SIZE];
        output[..OFFSET_BUFFER_SIZE - 8].copy_from_slice(&buf[8..]);
        output[OFFSET_BUFFER_SIZE - 8..].copy_from_slice(&[0x00, 0x00, 0x00, 0x0F, 0x03, 0x01, 0x01, 0xE4]);
        self.write(0x2E18, &output)?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "offset data")
    }

    fn send_xtalk_data(&self, res: usize) -> UldResult<()> {
        let mut buf = vec![0u8; XTALK_BUFFER_SIZE];
        let n = XTALK_BUFFER_SIZE.min(self.xtalk_data.len());
        buf[..n].copy_from_slice(&self.xtalk_data[..n]);
        if res == RESOLUTION_4X4 {
            buf[0x08..0x10].copy_from_slice(&[0x0F, 0x04, 0x04, 0x17, 0x08, 0x10, 0x10, 0x07]);
            buf[0x20..0x28].copy_from_slice(&[0x00, 0x78, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08]);
            buf = swap_buffer(&buf);
            let mut signal = [0i64; 64];
            for k in 0..64 {
                signal[k] = u32le(&buf, 0x34 + 4 * k) as i64;
            }
            average_4x4(&mut signal);
            for k in 0..64 {
                put_u32le(&mut buf, 0x34 + 4 * k, signal[k] as u32);
            }
            buf = swap_buffer(&buf);
            buf[0x134..0x138].copy_from_slice(&[0xA0, 0xFC, 0x01, 0x00]);
            buf[0x078..0x078 + 4].fill(0);
        }
        self.write(0x2CF8, &buf)?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "xtalk data")
    }

    // ── identity and init ────────────────────────────────────────────────────

    /// (device id, revision id) of the silicon; [`Vl53l8Driver::silicon_id_matches`] tells whether
    /// they are the sensor's.
    pub fn is_alive(&self) -> UldResult<(u8, u8)> {
        self.wr(0x7FFF, 0x00)?;
        let device_id = self.rd(0x00)?;
        let revision_id = self.rd(0x01)?;
        self.wr(0x7FFF, 0x02)?;
        Ok((device_id, revision_id))
    }

    /// `vl53l8cx_init`: boot the sensor MCU, download the 84 KB sensor firmware, upload the NVM
    /// offsets, the default xtalk and the default configuration.
    pub fn init(&mut self) -> UldResult<()> {
        self.init_with_progress(&mut |_| {})
    }

    /// [`Vl53l8Driver::init`] that tells `progress` the phase texts (`"SW reboot..."`,
    /// `"Downloading sensor FW (84 KB)... bank 1/3"` … `"Sensor init done"`).
    pub fn init_with_progress(&mut self, progress: &mut dyn FnMut(&str)) -> UldResult<()> {
        let note = progress;
        self.module_type = None;
        if self.i2c {
            let (device_id, revision_id) = self.is_alive()?;
            if !Self::silicon_id_matches(device_id, revision_id, true) {
                return Err(Vl53l8UldError::status(
                    status::ERROR,
                    format!("not a VL53L5/L7 sensor: device_id=0x{device_id:02X} revision_id=0x{revision_id:02X}"),
                ));
            }
        }

        note("SW reboot...");
        self.wr(0x7FFF, 0x00)?;
        self.wr(0x0009, 0x04)?;
        self.wr(0x000F, 0x40)?;
        self.wr(0x000A, 0x03)?;
        self.rd(0x7FFF)?;
        self.wr(0x000C, 0x01)?;

        self.wr(0x0101, 0x00)?;
        self.wr(0x0102, 0x00)?;
        self.wr(0x010A, 0x01)?;
        self.wr(0x4002, 0x01)?;
        self.wr(0x4002, 0x00)?;
        self.wr(0x010A, 0x03)?;
        self.wr(0x0103, 0x01)?;
        self.wr(0x000C, 0x00)?;
        self.wr(0x000F, 0x43)?;
        self.p.sleep_ms(1);

        self.wr(0x000F, 0x40)?;
        self.wr(0x000A, 0x01)?;
        self.p.sleep_ms(100);

        note("Waiting for sensor boot...");
        self.wr(0x7FFF, 0x00)?;
        self.poll_for_answer(1, 0, 0x06, 0xFF, 1, "sensor boot")?;

        self.wr(0x000E, 0x01)?;
        self.wr(0x7FFF, 0x02)?;

        // Enable FW access
        if self.i2c {
            self.wr(0x03, 0x0D)?;
            self.wr(0x7FFF, 0x01)?;
            self.poll_for_answer(1, 0, 0x21, 0x10, 0x10, "fw access")?;
        } else {
            self.wr(0x7FFF, 0x01)?;
            self.wr(0x06, 0x01)?;
            self.poll_for_answer(1, 0, 0x21, 0xFF, 0x04, "fw access")?;
        }

        self.wr(0x7FFF, 0x00)?;

        // Enable host access to GO1
        self.rd(0x7FFF)?;
        self.wr(0x0C, 0x01)?;

        // Power ON status
        self.wr(0x7FFF, 0x00)?;
        self.wr(0x101, 0x00)?;
        self.wr(0x102, 0x00)?;
        self.wr(0x010A, 0x01)?;
        self.wr(0x4002, 0x01)?;
        self.wr(0x4002, 0x00)?;
        self.wr(0x010A, 0x03)?;
        self.wr(0x103, 0x01)?;
        self.wr(0x400F, 0x00)?;
        self.wr(0x21A, 0x43)?;
        self.wr(0x21A, 0x03)?;
        self.wr(0x21A, 0x01)?;
        self.wr(0x21A, 0x00)?;
        self.wr(0x219, 0x00)?;
        self.wr(0x21B, 0x00)?;

        // Wake up MCU
        self.wr(0x7FFF, 0x00)?;
        self.rd(0x7FFF)?;
        if self.i2c {
            self.wr(0x0C, 0x00)?;
        }
        self.wr(0x7FFF, 0x01)?;
        if self.i2c {
            self.wr(0x20, 0x07)?;
            self.wr(0x20, 0x06)?;
        }

        // Download FW into the sensor
        let fw = self.firmware.image();
        note("Downloading sensor FW (84 KB)... bank 1/3");
        self.wr(0x7FFF, 0x09)?;
        self.write(0, &fw[0..0x8000])?;
        note("Downloading sensor FW... bank 2/3");
        self.wr(0x7FFF, 0x0A)?;
        self.write(0, &fw[0x8000..0x10000])?;
        note("Downloading sensor FW... bank 3/3");
        self.wr(0x7FFF, 0x0B)?;
        self.write(0, &fw[0x10000..0x15000])?;
        self.wr(0x7FFF, 0x01)?;

        // Check if FW correctly downloaded
        if self.i2c {
            self.wr(0x7FFF, 0x02)?;
            self.wr(0x03, 0x0D)?;
            self.wr(0x7FFF, 0x01)?;
            self.poll_for_answer(1, 0, 0x21, 0x10, 0x10, "fw downloaded")?;
        } else {
            self.wr(0x7FFF, 0x01)?;
            self.wr(0x06, 0x03)?;
            self.p.sleep_ms(5);
        }

        self.wr(0x7FFF, 0x00)?;
        self.rd(0x7FFF)?;
        self.wr(0x0C, 0x01)?;

        // Reset MCU and wait boot
        note("Booting sensor MCU...");
        self.wr(0x7FFF, 0x00)?;
        self.wr(0x114, 0x00)?;
        self.wr(0x115, 0x00)?;
        self.wr(0x116, 0x42)?;
        self.wr(0x117, 0x00)?;
        self.wr(0x0B, 0x00)?;
        self.rd(0x7FFF)?;
        self.wr(0x0C, 0x00)?;
        self.wr(0x0B, 0x01)?;

        self.poll_for_mcu_boot()?;

        self.wr(0x7FFF, 0x02)?;

        if let Some(expected) = self.fw_checksum {
            let crc = u32le(&swap_buffer(&self.read(0x2FFC, 4)?), 0);
            if crc != expected {
                return Err(Vl53l8UldError::status(
                    status::FW_CHECKSUM_FAIL,
                    format!("crc=0x{crc:08X} (expected 0x{expected:08X} for {})", self.firmware.set_name()),
                ));
            }
            note("Sensor FW checksum OK");
        }

        note("Reading NVM offset data...");
        self.write(0x2FD8, Vl53l8Firmware::get_nvm_cmd())?;
        self.poll_for_answer(4, 0, UI_CMD_STATUS, 0xFF, 2, "nvm read")?;
        let mut offset = self.read(UI_CMD_START, NVM_DATA_SIZE)?;
        offset.resize(OFFSET_BUFFER_SIZE, 0);
        self.offset_data = offset;
        self.send_offset_data(RESOLUTION_4X4)?;

        note("Uploading default xtalk...");
        self.xtalk_data = Vl53l8Firmware::default_xtalk().to_vec();
        self.send_xtalk_data(RESOLUTION_4X4)?;

        note("Uploading default configuration...");
        self.write(0x2C34, self.firmware.default_configuration())?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "default config")?;

        self.dci_write_data(DCI_PIPE_CONTROL, &[NB_TARGET as u8, 0x00, 0x01, 0x00])?;
        self.dci_write_data(DCI_SINGLE_RANGE, &0x01u32.to_le_bytes())?;
        if self.i2c {
            // Readable once the sensor FW runs; tells L5 (MZ) from L7 (MZEVO): one blob, one board.
            let module_type = self.dci_read_data(DCI_FW_FLAGS, 8)?[1];
            self.module_type = Some(module_type);
            let name = match module_type {
                MODULE_TYPE_MZ => "MZ".to_string(),
                MODULE_TYPE_MZEVO => "MZEVO".to_string(),
                MODULE_TYPE_MZPLUS => "MZPlus".to_string(),
                other => other.to_string(),
            };
            note(&format!("Sensor init done (module type {name})"));
        } else {
            note("Sensor init done");
        }
        Ok(())
    }

    // ── configuration ────────────────────────────────────────────────────────

    /// Zones in use (16 or 64; 0 before a resolution is set).
    pub fn get_resolution(&self) -> UldResult<usize> {
        let buf = self.dci_read_data(DCI_ZONE_CONFIG, 8)?;
        Ok(buf[0] as usize * buf[1] as usize)
    }

    /// 16 (4×4) or 64 (8×8) zones; uploads the offsets and xtalk for it.
    pub fn set_resolution(&mut self, res: usize) -> UldResult<()> {
        if res != RESOLUTION_4X4 && res != RESOLUTION_8X8 {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_resolution"));
        }
        let four = res == RESOLUTION_4X4;
        let mut buf = self.dci_read_data(DCI_DSS_CONFIG, 16)?;
        buf[0x04] = if four { 64 } else { 16 };
        buf[0x06] = if four { 64 } else { 16 };
        buf[0x09] = if four { 4 } else { 1 };
        self.dci_write_data(DCI_DSS_CONFIG, &buf)?;
        let mut buf = self.dci_read_data(DCI_ZONE_CONFIG, 8)?;
        buf[0x00] = if four { 4 } else { 8 };
        buf[0x01] = if four { 4 } else { 8 };
        buf[0x04] = if four { 8 } else { 4 };
        buf[0x05] = if four { 8 } else { 4 };
        self.dci_write_data(DCI_ZONE_CONFIG, &buf)?;
        self.send_offset_data(res)?;
        self.send_xtalk_data(res)
    }

    /// Ranging frequency, Hz.
    pub fn get_ranging_frequency_hz(&self) -> UldResult<u8> {
        Ok(self.dci_read_data(DCI_FREQ_HZ, 4)?[1])
    }

    /// Ranging frequency, Hz: a u8 field, so 0..255 (outside is
    /// [`Vl53l8UldError::InvalidArgument`]); the sensor's own limits apply on top.
    pub fn set_ranging_frequency_hz(&mut self, hz: i32) -> UldResult<()> {
        if !(0..=255).contains(&hz) {
            return Err(Vl53l8UldError::InvalidArgument(format!(
                "ranging frequency is a u8 field (0..255 Hz), got {hz}"
            )));
        }
        self.dci_replace_data(DCI_FREQ_HZ, 4, &[hz as u8], 0x01)
    }

    /// [`RANGING_MODE_CONTINUOUS`] or [`RANGING_MODE_AUTONOMOUS`].
    pub fn set_ranging_mode(&mut self, mode: u8) -> UldResult<()> {
        let mut buf = self.dci_read_data(DCI_RANGING_MODE, 8)?;
        let single_range: u32 = if mode == RANGING_MODE_CONTINUOUS {
            buf[0x01] = 0x1;
            buf[0x03] = 0x3;
            0x00
        } else if mode == RANGING_MODE_AUTONOMOUS {
            buf[0x01] = 0x3;
            buf[0x03] = 0x2;
            0x01
        } else {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_ranging_mode"));
        };
        self.dci_write_data(DCI_RANGING_MODE, &buf)?;
        self.dci_write_data(DCI_SINGLE_RANGE, &single_range.to_le_bytes())
    }

    /// [`RANGING_MODE_CONTINUOUS`] or [`RANGING_MODE_AUTONOMOUS`].
    pub fn get_ranging_mode(&self) -> UldResult<u8> {
        Ok(if self.dci_read_data(DCI_RANGING_MODE, 8)?[0x01] == 0x1 {
            RANGING_MODE_CONTINUOUS
        } else {
            RANGING_MODE_AUTONOMOUS
        })
    }

    /// Integration time, ms.
    pub fn get_integration_time_ms(&self) -> UldResult<u32> {
        Ok(u32le(&self.dci_read_data(DCI_INT_TIME, 20)?, 0) / 1000)
    }

    /// 2..1000 ms; no effect in continuous mode.
    pub fn set_integration_time_ms(&mut self, time_ms: i32) -> UldResult<()> {
        if !(2..=1000).contains(&time_ms) {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_integration_time_ms"));
        }
        self.dci_replace_data(DCI_INT_TIME, 20, &(time_ms as u32 * 1000).to_le_bytes(), 0x00)
    }

    /// 0..99 %, rounded to nearest on the way back (ST truncates and loses a count for 95 of the
    /// 100 legal values; the stored byte is the same).
    pub fn get_sharpener_percent(&self) -> UldResult<u8> {
        let raw = self.dci_read_data(DCI_SHARPENER, 16)?[0xD];
        Ok((raw as f64 * 100.0 / 255.0).round_ties_even() as u8)
    }

    /// 0..99 %: 100 and up is the ULD's INVALID_PARAM, a negative value
    /// [`Vl53l8UldError::InvalidArgument`].
    pub fn set_sharpener_percent(&mut self, pct: i32) -> UldResult<()> {
        if pct >= 100 {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_sharpener_percent"));
        }
        if pct < 0 {
            return Err(Vl53l8UldError::InvalidArgument(format!("sharpener is 0..99 %, got {pct}")));
        }
        self.dci_replace_data(DCI_SHARPENER, 16, &[(pct * 255 / 100) as u8], 0xD)
    }

    /// [`TARGET_ORDER_CLOSEST`] or [`TARGET_ORDER_STRONGEST`].
    pub fn get_target_order(&self) -> UldResult<u8> {
        Ok(self.dci_read_data(DCI_TARGET_ORDER, 4)?[0])
    }

    /// [`TARGET_ORDER_CLOSEST`] or [`TARGET_ORDER_STRONGEST`].
    pub fn set_target_order(&mut self, order: u8) -> UldResult<()> {
        if order != TARGET_ORDER_CLOSEST && order != TARGET_ORDER_STRONGEST {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_target_order"));
        }
        self.dci_replace_data(DCI_TARGET_ORDER, 4, &[order], 0x0)
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// Program the output list and start. `cnh_data_size` (bytes, from
    /// [`super::Vl53l8CnhSetup::required_memory`]; `None` for none) appends the CNH block to every
    /// frame. The frame size the firmware reports wins over the ULD formula
    /// ([`Vl53l8Driver::frame_size_mismatch`]).
    pub fn start_ranging(&mut self, cnh_data_size: Option<u32>) -> UldResult<()> {
        let res = self.get_resolution()?;
        // A zero / odd resolution sizes the output config to nothing and faults the sensor MCU.
        if res != RESOLUTION_4X4 && res != RESOLUTION_8X8 {
            return Err(Vl53l8UldError::status(
                status::INVALID_PARAM,
                format!("bad resolution {res} — call set_resolution first"),
            ));
        }
        self.data_read_size = 0;
        self.stream_count = 255;

        let mut enable: [u32; 4] = [0x00000FFF, 0, 0, self.output_enable_w3];
        let mut output: Vec<u32> = vec![
            START_BH,
            METADATA_BH,
            COMMONDATA_BH,
            AMBIENT_RATE_BH,
            SPAD_COUNT_BH,
            NB_TARGET_DETECTED_BH,
            SIGNAL_RATE_BH,
            RANGE_SIGMA_MM_BH,
            DISTANCE_BH,
            REFLECTANCE_BH,
            TARGET_STATUS_BH,
            MOTION_DETECT_BH,
        ];
        if let Some(cnh) = cnh_data_size {
            // vl53lmz_add_output_block: CNH block, type 4, size in 32-bit words.
            output.push((CNH_DATA_IDX << 16) | (((cnh / 4) & 0xFFF) << 4) | 4);
            enable[0] |= 1 << (output.len() - 1);
        }

        let res32 = res as u32;
        let mut size: u32 = 0;
        for i in 0..output.len() {
            let bh = output[i];
            if bh == 0 || enable[i / 32] & (1 << (i % 32)) == 0 {
                continue;
            }
            let (kind, mut bh_size, idx) = bh_fields(bh);
            if (0x1..0x0D).contains(&kind) {
                if self.i2c {
                    // vl53lmz_start_ranging (2.0.16), as on L5/L7 hardware: per-target blocks above
                    // 0x6C90 keep their 64-entry size even in 4x4 (the frame parser trims them).
                    if (0x54D0..0x5890).contains(&idx) {
                        bh_size = res32;
                    } else if idx < 0x6C90 {
                        bh_size = res32 * NB_TARGET;
                    }
                } else if (0x54D0..0x54D0 + 960).contains(&idx) {
                    bh_size = res32;
                } else if idx != CNH_DATA_IDX {
                    // the CNH block keeps its size
                    bh_size = res32 * NB_TARGET;
                }
                output[i] = bh_set_size(bh, bh_size);
                size += kind * bh_size;
            } else {
                size += bh_size;
            }
            size += 4;
        }
        self.data_read_size = (size + self.frame_tail) as usize;
        self.resolution = Some(res);

        self.dci_write_data(DCI_OUTPUT_LIST, &u32s(&output))?;
        self.dci_write_data(DCI_OUTPUT_CONFIG, &u32s(&[self.data_read_size as u32, output.len() as u32 + 1]))?;
        self.dci_write_data(DCI_OUTPUT_ENABLES, &u32s(&enable))?;

        // Start xshut bypass (interrupt mode)
        self.wr(0x7FFF, 0x00)?;
        self.wr(0x09, 0x05)?;
        self.wr(0x7FFF, 0x02)?;

        // Start the ranging session
        self.write(UI_CMD_END - 3, &[0x00, 0x03, 0x00, 0x00])?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "start ranging")?;

        // The firmware tells the frame size it streams; the released blobs report 4 bytes less than
        // the C formula, so trust the firmware value.
        let cfg = self.dci_read_data(0x5440, 12)?;
        let fw_size = cfg[0x8] as usize | (cfg[0x9] as usize) << 8;
        self.frame_size_mismatch = None;
        if fw_size != self.data_read_size {
            self.frame_size_mismatch = Some((fw_size, self.data_read_size));
            self.data_read_size = fw_size;
        }

        if self.dci_read_data(DCI_FW_FLAGS, 8)?[0x6] != 0 {
            return Err(Vl53l8UldError::status(status::LASER_SAFETY, ""));
        }
        Ok(())
    }

    /// `vl53l8cx_stop_ranging`: provoke an MCU stop (unless the detection thresholds auto-stopped
    /// it), wait up to ~5 s, then undo the stop and the xshut bypass. A stuck MCU is not an error.
    pub fn stop_ranging(&mut self) -> UldResult<()> {
        let auto_stop_flag = u32le(&self.read(0x2FFC, 4)?, 0);
        if auto_stop_flag != 0x4FF {
            self.wr(0x7FFF, 0x00)?;
            // Provoke MCU stop
            self.wr(0x15, 0x16)?;
            self.wr(0x14, 0x01)?;
            // Poll for G02 status 0 MCU stop
            let mut tmp: u8 = 0;
            let mut timeout = 0;
            while (tmp & 0x80) >> 7 == 0 {
                tmp = self.rd(0x06)?;
                self.p.sleep_ms(10);
                timeout += 1;
                if timeout > 500 {
                    break;
                }
            }
        }
        // GO2 status 1: the C driver ORs a 0x84/0x85 mismatch into its status; non-fatal here.
        if self.rd(0x06)? & 0x80 != 0 {
            self.rd(0x07)?;
        }
        // Undo MCU stop
        self.wr(0x7FFF, 0x00)?;
        self.wr(0x14, 0x00)?;
        self.wr(0x15, 0x00)?;
        // Stop xshut bypass
        self.wr(0x09, 0x04)?;
        self.wr(0x7FFF, 0x02)
    }

    /// Poll mode: a new frame is ready. A GO2 error is a [`Vl53l8UldError::Status`] carrying the
    /// sensor's error byte as the code.
    pub fn check_data_ready(&mut self) -> UldResult<bool> {
        let buf = self.read(0x0, 4)?;
        let b0 = buf[0] as i32;
        if b0 != self.stream_count && b0 != 255 && buf[1] == 0x05 && buf[2] & 0x05 == 0x05 && buf[3] & 0x10 == 0x10 {
            self.stream_count = b0;
            return Ok(true);
        }
        if buf[3] & 0x80 != 0 {
            return Err(Vl53l8UldError::status(buf[2], "GO2 error status"));
        }
        Ok(false)
    }

    /// Poll mode: read one frame from register 0x00 and parse it.
    pub fn get_ranging_data(&mut self, timestamp_us: u64) -> UldResult<Vl53l8Frame> {
        let raw = self.read(0x0, self.data_read_size)?;
        self.parse_frame(timestamp_us, &raw)
    }

    /// Parse one raw frame ([`Vl53l8Driver::data_read_size`] bytes) — polled, or pushed by the
    /// board's stream. L5/L7 frames are trimmed to the zones in use.
    ///
    /// A header / footer id mismatch (or a frame too short to hold them) is
    /// [`status::CORRUPTED_FRAME`].
    pub fn parse_frame(&mut self, timestamp_us: u64, raw: &[u8]) -> UldResult<Vl53l8Frame> {
        if let Some(&b0) = raw.first() {
            self.stream_count = b0 as i32;
        }
        let mut r = decode::parse_frame(raw, self.variant).map_err(|e| {
            let at = match e {
                decode::Vl53l8Error::CorruptedFrame => "parse_frame",
                decode::Vl53l8Error::ShortFrame => "parse_frame: frame too short",
            };
            Vl53l8UldError::status(status::CORRUPTED_FRAME, at)
        })?;
        if self.i2c {
            if let Some(n) = self.resolution {
                // Trimming after the scaling and the no-target marks is the same as before them:
                // both are per entry.
                let nt = n * NB_TARGET_PER_ZONE;
                r.ambient_per_spad.truncate(n);
                r.nb_spads_enabled.truncate(n);
                r.nb_target_detected.truncate(n);
                r.distance_mm.truncate(nt);
                r.target_status.truncate(nt);
                r.signal_per_spad.truncate(nt);
                r.range_sigma_mm.truncate(nt);
                r.reflectance.truncate(nt);
            }
        }
        let motion = if self.motion_present { parse_motion(&swap_buffer(raw))? } else { None };
        Ok(Vl53l8Frame::from_results(timestamp_us, r, motion))
    }

    // ── power modes ──────────────────────────────────────────────────────────

    /// [`POWER_MODE_WAKEUP`], [`POWER_MODE_SLEEP`] or [`POWER_MODE_DEEP_SLEEP`]
    /// (`crate::vl53l8::advanced` constants); an unknown state is [`status::ERROR`].
    pub fn get_power_mode(&self) -> UldResult<u8> {
        self.wr(0x7FFF, 0x00)?;
        let tmp = self.rd(0x09)?;
        let mode = if tmp == 0x04 {
            POWER_MODE_WAKEUP
        } else if tmp == 0x02 {
            // ULD 2.0.1 (l7cx) has no deep sleep and never reads 0x000F.
            if self.firmware == Vl53l8Firmware::L7cx || self.rd(0x000F)? != 0x43 {
                POWER_MODE_SLEEP
            } else {
                POWER_MODE_DEEP_SLEEP
            }
        } else {
            self.wr(0x7FFF, 0x02)?;
            return Err(Vl53l8UldError::status(status::ERROR, "get_power_mode"));
        };
        self.wr(0x7FFF, 0x02)?;
        Ok(mode)
    }

    /// Wakeup, sleep or deep sleep (not on L5CX/L7CX). Waking from deep sleep re-runs
    /// [`Vl53l8Driver::init`]: the sensor firmware is lost.
    pub fn set_power_mode(&mut self, power_mode: u8) -> UldResult<()> {
        let l7cx = self.firmware == Vl53l8Firmware::L7cx;
        if l7cx && power_mode != POWER_MODE_SLEEP && power_mode != POWER_MODE_WAKEUP {
            return Err(Vl53l8UldError::status(
                status::INVALID_PARAM,
                "set_power_mode: VL53L5CX/L7CX have no deep sleep",
            ));
        }
        let current = self.get_power_mode()?;
        if power_mode == current {
            return Ok(());
        }
        if power_mode == POWER_MODE_WAKEUP && l7cx {
            self.wr(0x7FFF, 0x00)?;
            self.wr(0x09, 0x04)?;
            self.poll_for_answer(1, 0, 0x06, 0x01, 1, "wakeup")?;
            self.wr(0x7FFF, 0x02)?;
        } else if power_mode == POWER_MODE_WAKEUP {
            self.wr(0x7FFF, 0x00)?;
            self.wr(0x09, 0x04)?;
            let stored = self.rd(0x000F)?;
            if stored == 0x43 {
                self.wr(0x000F, 0x40)?;
            }
            self.poll_for_answer(1, 0, 0x06, 0x01, 1, "wakeup")?;
            self.wr(0x7FFF, 0x02)?;
            if stored == 0x43 {
                self.init()?;
            }
        } else if power_mode == POWER_MODE_SLEEP {
            self.wr(0x7FFF, 0x00)?;
            self.wr(0x09, 0x02)?;
            self.poll_for_answer(1, 0, 0x06, 0x01, 0, "sleep")?;
            self.wr(0x7FFF, 0x02)?;
        } else if power_mode == POWER_MODE_DEEP_SLEEP {
            self.wr(0x7FFF, 0x00)?;
            self.wr(0x09, 0x02)?;
            self.poll_for_answer(1, 0, 0x06, 0x01, 0, "deep sleep")?;
            self.wr(0x000F, 0x43)?;
            self.wr(0x7FFF, 0x02)?;
        } else {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "set_power_mode"));
        }
        Ok(())
    }

    // ── xtalk ────────────────────────────────────────────────────────────────

    /// Xtalk margin, kcps/SPAD.
    pub fn get_xtalk_margin(&self) -> UldResult<f64> {
        Ok(u32le(&self.dci_read_data(DCI_XTALK_CFG, 16)?, 0) as f64 / 2048.0)
    }

    /// Xtalk margin, kcps/SPAD: 0..10000 (NaN and anything else is [`status::INVALID_PARAM`],
    /// nothing written).
    pub fn set_xtalk_margin(&mut self, margin_kcps: f64) -> UldResult<()> {
        let raw = try_xtalk_margin_to_raw(margin_kcps)
            .map_err(|_| Vl53l8UldError::status(status::INVALID_PARAM, "set_xtalk_margin"))?;
        self.dci_replace_data(DCI_XTALK_CFG, 16, &raw.to_le_bytes(), 0x00)
    }

    fn read_xtalk_buffer(&self) -> UldResult<Vec<u8>> {
        self.write(0x2FB8, tables::get_xtalk_cmd())?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "get xtalk")?;
        let buf = self.read(UI_CMD_START, XTALK_BUFFER_SIZE + 4)?;
        let mut output = vec![0u8; XTALK_BUFFER_SIZE];
        output[..XTALK_BUFFER_SIZE - 8].copy_from_slice(&buf[8..XTALK_BUFFER_SIZE]);
        output[XTALK_BUFFER_SIZE - 8..].copy_from_slice(&[0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04]);
        Ok(output)
    }

    /// The live 776-byte xtalk calibration buffer (save / restore); the resolution is restored
    /// afterwards.
    pub fn get_caldata_xtalk(&mut self) -> UldResult<Vec<u8>> {
        let res = self.get_resolution()?;
        self.set_resolution(RESOLUTION_8X8)?;
        self.xtalk_data = self.read_xtalk_buffer()?;
        self.set_resolution(res)?;
        Ok(self.xtalk_data.clone())
    }

    /// Restore a saved 776-byte xtalk buffer; it is uploaded again by the next resolution change
    /// and at once here. Another length is [`status::INVALID_PARAM`].
    pub fn set_caldata_xtalk(&mut self, blob: &[u8]) -> UldResult<()> {
        if blob.len() != XTALK_BUFFER_SIZE {
            return Err(Vl53l8UldError::status(
                status::INVALID_PARAM,
                format!("xtalk blob must be {XTALK_BUFFER_SIZE} bytes"),
            ));
        }
        let res = self.get_resolution()?;
        self.xtalk_data = blob.to_vec();
        self.set_resolution(res)
    }

    // ── detection thresholds ─────────────────────────────────────────────────

    /// Whether the detection thresholds are enabled (the raw enable byte: 1 on, 0 off).
    pub fn get_detection_thresholds_enable(&self) -> UldResult<u8> {
        Ok(self.dci_read_data(DCI_DET_THRESH_GLOBAL_CONFIG, 8)?[1])
    }

    /// Enable or disable the detection thresholds.
    pub fn set_detection_thresholds_enable(&mut self, enabled: bool) -> UldResult<()> {
        let grp = [0x01, if enabled { 0x01 } else { 0x00 }, 0x01, 0x00];
        self.dci_replace_data(DCI_DET_THRESH_GLOBAL_CONFIG, 8, &grp, 0x00)?;
        self.dci_replace_data(DCI_DET_THRESH_CONFIG, 20, &[if enabled { 0x04 } else { 0x0C }], 0x11)
    }

    /// The 64 thresholds, low / high rescaled to the units of their measurement (floor division).
    pub fn get_detection_thresholds(&self) -> UldResult<Vec<DetectionThreshold>> {
        let raw = self.dci_read_data(DCI_DET_THRESH_START, NB_THRESHOLDS * 12)?;
        let mut result = Vec::with_capacity(NB_THRESHOLDS);
        for k in 0..NB_THRESHOLDS {
            let b = k * 12;
            let meas = raw[b + 8];
            let scale = thresh_scale(meas);
            result.push(DetectionThreshold {
                low_thresh: (u32le(&raw, b) as i32 as i64).div_euclid(scale) as i32,
                high_thresh: (u32le(&raw, b + 4) as i32 as i64).div_euclid(scale) as i32,
                measurement: meas,
                type_: raw[b + 9],
                zone_num: raw[b + 10],
                operation: raw[b + 11],
            });
        }
        Ok(result)
    }

    /// Program the 64 thresholds (missing entries are zeros). More than 64, or a low / high whose
    /// scaled value does not fit i32, is [`Vl53l8UldError::InvalidArgument`], nothing written.
    pub fn set_detection_thresholds(&mut self, thresholds: &[DetectionThreshold]) -> UldResult<()> {
        let packed =
            try_pack_detection_thresholds(thresholds).map_err(|e| Vl53l8UldError::InvalidArgument(e.to_string()))?;
        self.dci_write_data(DCI_DET_THRESH_VALID_STATUS, &packed.valid)?;
        self.dci_write_data(DCI_DET_THRESH_START, &packed.start)
    }

    /// Stop ranging when a threshold fires (not in the L5CX/L7CX ULD: [`status::INVALID_PARAM`]).
    pub fn set_detection_thresholds_auto_stop(&mut self, auto_stop: bool) -> UldResult<()> {
        if self.firmware == Vl53l8Firmware::L7cx {
            return Err(Vl53l8UldError::status(
                status::INVALID_PARAM,
                "detection-threshold auto-stop: not in the L5CX/L7CX ULD",
            ));
        }
        self.dci_replace_data(DCI_PIPE_CONTROL, 4, &[auto_stop as u8], 0x03)
    }

    // ── motion indicator ─────────────────────────────────────────────────────

    /// A default motion-indicator configuration for `res`, written to the sensor; frames carry
    /// motion output from here on.
    pub fn motion_indicator_init(&mut self, res: usize) -> UldResult<MotionConfig> {
        let mut cfg = try_default_motion_config(RESOLUTION_4X4)
            .map_err(|_| Vl53l8UldError::status(status::INVALID_PARAM, "motion init"))?;
        self.motion_indicator_set_resolution(&mut cfg, res)?;
        self.motion_present = true;
        Ok(cfg)
    }

    /// Set `cfg`'s zone map for `res` (16 or 64, else [`status::INVALID_PARAM`]) and write it.
    pub fn motion_indicator_set_resolution(&mut self, cfg: &mut MotionConfig, res: usize) -> UldResult<()> {
        cfg.try_set_resolution(res).map_err(|_| Vl53l8UldError::status(status::INVALID_PARAM, "motion set_resolution"))?;
        self.dci_write_data(DCI_MOTION_DETECTOR_CFG, &cfg.pack())
    }

    /// Set `cfg`'s distance window (400..4000 mm, at most 1500 mm wide, else
    /// [`status::INVALID_PARAM`]) and write it.
    pub fn motion_indicator_set_distance_motion(
        &mut self,
        cfg: &mut MotionConfig,
        distance_min_mm: i32,
        distance_max_mm: i32,
    ) -> UldResult<()> {
        // wrapping_sub: the C / Java int subtraction, never a panic.
        if distance_max_mm.wrapping_sub(distance_min_mm) > 1500 || distance_min_mm < 400 || distance_max_mm > 4000 {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "motion set_distance"));
        }
        cfg.ref_bin_offset = ((distance_min_mm as f64 / 37.5348 - 4.0) * 2048.5) as i32;
        cfg.feature_length =
            ((((distance_max_mm - distance_min_mm) as f64 / 10.0 + 30.02784) / 15.01392 + 0.5) as i32 & 0xFF) as u8;
        self.dci_write_data(DCI_MOTION_DETECTOR_CFG, &cfg.pack())
    }

    // ── xtalk calibration run ────────────────────────────────────────────────

    /// `_vlxxx_program_output_config` of the ST xtalk plugin (every version): the 17-block
    /// calibration output list, sized with the 0x54D0+960 rule, 24-byte tail.
    fn program_xtalk_output_config(&self, res: usize) -> UldResult<()> {
        let mut output = tables::xtalk_cal_output().to_vec();
        let enable = tables::xtalk_cal_output_enable();
        let res32 = res as u32;
        let mut size: u32 = 0;
        for i in 0..output.len() {
            if output[i] == 0 || enable[i / 32] & (1 << (i % 32)) == 0 {
                continue;
            }
            let (kind, mut bh_size, idx) = bh_fields(output[i]);
            if (0x1..0x0D).contains(&kind) {
                bh_size = if (0x54D0..0x54D0 + 960).contains(&idx) { res32 } else { res32 * NB_TARGET };
                output[i] = bh_set_size(output[i], bh_size);
                size += kind * bh_size;
            } else {
                size += bh_size;
            }
            size += 4;
        }
        size += 24;
        self.dci_write_data(DCI_OUTPUT_LIST, &u32s(&output))?;
        self.dci_write_data(DCI_OUTPUT_CONFIG, &u32s(&[size, output.len() as u32 + 1]))?;
        self.dci_write_data(DCI_OUTPUT_ENABLES, &u32s(enable))
    }

    /// `vl53l8cx_calibrate_xtalk`: on-device crosstalk calibration against a flat target at
    /// `distance_mm` (600..3000) of `reflectance_percent` (1..99), averaging `nb_samples` (1..16);
    /// outside is [`status::INVALID_PARAM`], nothing sent. Takes seconds; the configuration is
    /// saved and restored around it. Without cover glass the firmware answers "coverglass too
    /// good": [`Vl53l8Driver::xtalk_calibration_failed`], and the default buffer stays.
    pub fn calibrate_xtalk(&mut self, reflectance_percent: i32, nb_samples: i32, distance_mm: i32) -> UldResult<()> {
        if !(1..=99).contains(&reflectance_percent) {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "reflectance 1..99"));
        }
        if !(600..=3000).contains(&distance_mm) {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "distance 600..3000"));
        }
        if !(1..=16).contains(&nb_samples) {
            return Err(Vl53l8UldError::status(status::INVALID_PARAM, "nb_samples 1..16"));
        }

        let saved_resolution = self.get_resolution()?;
        let saved_frequency = self.get_ranging_frequency_hz()?;
        let saved_integration = self.get_integration_time_ms()?;
        let saved_sharpener = self.get_sharpener_percent()?;
        let saved_target_order = self.get_target_order()?;
        let saved_margin = self.get_xtalk_margin()?;
        let saved_mode = self.get_ranging_mode()?;

        self.set_resolution(RESOLUTION_8X8)?;
        let table: &[u8] = match self.firmware {
            Vl53l8Firmware::L7cx => tables::calibrate_xtalk_l7cx(),
            Vl53l8Firmware::Cx => tables::calibrate_xtalk_cx(),
            _ => tables::calibrate_xtalk_lmz(),
        };
        self.write(0x2C28, table)?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib cmd")?;
        let d = distance_mm * 4;
        let r = reflectance_percent * 16;
        self.dci_replace_data(DCI_CAL_CFG, 8, &[d as u8, (d >> 8) as u8], 0x00)?;
        self.dci_replace_data(DCI_CAL_CFG, 8, &[r as u8, (r >> 8) as u8], 0x02)?;
        self.dci_replace_data(DCI_CAL_CFG, 8, &[nb_samples as u8], 0x04)?;
        self.program_xtalk_output_config(RESOLUTION_8X8)?;
        self.write(UI_CMD_END - 3, &[0x00, 0x03, 0x00, 0x00])?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "calib start")?;

        self.xtalk_calibration_failed = false;
        let mut timeout = 0;
        loop {
            let buf = self.read(0x0, 4)?;
            if buf[0] != status::ERROR {
                if buf[2] >= 0x7F && (buf[3] & 0x80) >> 7 == 1 {
                    // ST: "Coverglass too good for Xtalk calibration" (STATUS_XTALK_FAILED).
                    self.xtalk_data = Vl53l8Firmware::default_xtalk().to_vec();
                    self.xtalk_calibration_failed = true;
                }
                break;
            }
            if timeout >= 400 {
                return Err(Vl53l8UldError::status(status::ERROR, "xtalk calibration"));
            }
            self.p.sleep_ms(50);
            timeout += 1;
        }
        self.xtalk_data = self.read_xtalk_buffer()?;
        self.write(0x2C34, self.firmware.default_configuration())?;
        self.poll_for_answer(4, 1, UI_CMD_STATUS, 0xFF, 0x03, "restore cfg")?;

        self.set_resolution(saved_resolution)?;
        self.set_ranging_frequency_hz(saved_frequency as i32)?;
        self.set_integration_time_ms(saved_integration as i32)?;
        self.set_sharpener_percent(saved_sharpener as i32)?;
        self.set_target_order(saved_target_order)?;
        self.set_xtalk_margin(saved_margin)?;
        self.set_ranging_mode(saved_mode)
    }
}

// ── helpers ──────────────────────────────────────────────────────────────────

fn parse_motion(buf: &[u8]) -> UldResult<Option<Vl53l8Motion>> {
    let drs = buf.len();
    let mut i = 16;
    while i + 4 <= drs {
        let (t, s, idx) = bh_fields(u32le(buf, i));
        let msize = if t > 0x1 && t < 0xD { (t * s) as usize } else { s as usize };
        if i + 4 + msize > drs {
            break;
        }
        if idx == MOTION_DETEC_IDX {
            if i + 16 + 4 * 32 > drs {
                return Err(Vl53l8UldError::status(status::CORRUPTED_FRAME, "motion block past the frame end"));
            }
            let motion = (0..32).map(|k| u32le(buf, i + 16 + 4 * k)).collect();
            return Ok(Some(Vl53l8Motion {
                global_indicator_1: u32le(buf, i + 4),
                global_indicator_2: u32le(buf, i + 8),
                status: buf[i + 12],
                nb_of_detected_aggregates: buf[i + 13],
                nb_of_aggregates: buf[i + 14],
                motion,
            }));
        }
        i += msize + 4;
    }
    Ok(None)
}

/// union Block_header: (type[3:0], size[15:4], idx[31:16]).
fn bh_fields(bh: u32) -> (u32, u32, u32) {
    (bh & 0xF, (bh >> 4) & 0xFFF, (bh >> 16) & 0xFFFF)
}

fn bh_set_size(bh: u32, size: u32) -> u32 {
    (bh & !0xFFF0) | ((size & 0xFFF) << 4)
}

/// 8x8 → 4x4: each 4x4 zone is the mean of its 2x2 block (C integer division, floored).
fn average_4x4(grid: &mut [i64; 64]) {
    for j in 0..4 {
        for i in 0..4 {
            grid[i + 4 * j] = (grid[2 * i + 16 * j] + grid[2 * i + 16 * j + 1] + grid[2 * i + 16 * j + 8]
                + grid[2 * i + 16 * j + 9])
                .div_euclid(4);
        }
    }
    for v in grid.iter_mut().skip(16) {
        *v = 0;
    }
}

fn u32le(b: &[u8], off: usize) -> u32 {
    u32::from_le_bytes([b[off], b[off + 1], b[off + 2], b[off + 3]])
}

fn put_u32le(b: &mut [u8], off: usize, v: u32) {
    b[off..off + 4].copy_from_slice(&v.to_le_bytes());
}

fn u32s(words: &[u32]) -> Vec<u8> {
    words.iter().flat_map(|w| w.to_le_bytes()).collect()
}
