//! Register behaviour of a VL53L0X die, a port of the Python SDK's `tests/fake_vl53l0x.py`: what the
//! VL53L0X ULD port (`vl53lx::uld::Vl53l0xUldDriver`) needs to run its init, calibrations and ranging
//! profiles with no hardware. It sits directly under the driver as its [`Vl53lxPlatform`].
//!
//! One 256-byte bank per page (register 0xFF selects it, as on the die), with the registers that
//! *behave* modelled on top: the soft reset and the NACKs around it, the NVM strobe and data, the
//! single-shot start bit, data-ready and interrupt clear, the result block of each shot and the
//! reference-signal measurement the SPAD management converges on. A NACK goes back as the bridge's
//! ERR_HARDWARE_FAULT status for the READ_REG.
//!
//! The defaults are what the live VL53L0X board (TB9BGETA6M) answered in the committed capture
//! `vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec`: stop variable 0x3C, oscillator word
//! 0x975C, NVM reference-SPAD record "6 non-aperture", good SPAD map FB FF FF DF B7 0F, VHV 0x22,
//! phase 0x01, OSC_CALIBRATE_VAL 0x28, one NACK on the model id after a soft reset.

use std::collections::{BTreeMap, VecDeque};
use std::sync::{Arc, Mutex, MutexGuard};

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::vl53lx::driver::Vl53lxPlatform;

pub const MODEL_ID: u8 = 0xEE;
/// GLOBAL_CONFIG_REF_EN_START_SELECT.
const START_SELECT: usize = 0xB4;
/// SPAD indices from here lie in an aperture quadrant.
const FIRST_APERTURE_SPAD: usize = 192;
const READ_REG: u8 = 0x32;
const ERR_HARDWARE_FAULT: u8 = 0x09;

/// The first frame of a run, no target yet (8191 mm, device status 0x58).
pub const FRAME_NO_TARGET: [u8; 12] = [
    0x58, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff,
];
/// A flat target at 635 mm (status 0x5A: internal 11, signal 0x022B in 9.7, ambient 0x011B,
/// 0xC104 effective SPADs in 8.8).
pub const FRAME_635_MM: [u8; 12] = [
    0x5a, 0x06, 0xc1, 0x04, 0x00, 0x71, 0x02, 0x2b, 0x01, 0x1b, 0x02, 0x7b,
];

pub const PRODUCT_ID: &str = "VL53L0X-TEST-18CHR";

/// NVM words (index -> dword) of the capture plus the identification words.
pub fn nvm_defaults() -> BTreeMap<u8, u32> {
    let mut nvm = BTreeMap::from([
        (0x6B, 0x1D80060B), // reference SPADs: (>>8)&0x7F = 6, (>>15)&1 = 0 (non-aperture)
        (0x24, 0xFBFFFFDF), // good SPAD map bytes 0..3
        (0x25, 0xB70F0000), // good SPAD map bytes 4..5
        (0x02, 0x2A000000), // module id (first byte)
        (0x7B, 0x0C345678), // revision (first byte) / part UID upper
        (0x7C, 0x9ABCDEF0), // part UID lower
        (0x73, 0x000000A5), // signal rate at 400 mm, high byte
        (0x74, 0x3C000000), // ... low byte -> 0xA53C in 9.7
        (0x75, 0x00000019), // distance at 400 mm (1/16 mm), high byte
        (0x76, 0x40000000), // ... low byte -> 0x1940 = 6464 = 404 mm
    ]);
    nvm.extend(nvm_product_id(PRODUCT_ID));
    nvm
}

/// The four NVM words 0x77..0x7A holding an 18-character product id: 7 bits per character, MSB
/// first, two padding bits at the end.
pub fn nvm_product_id(text: &str) -> BTreeMap<u8, u32> {
    assert_eq!(text.len(), 18);
    let mut bits: String = text.bytes().map(|c| format!("{:07b}", c & 0x7F)).collect();
    bits.push_str("00");
    (0..4)
        .map(|i| {
            (
                0x77 + i as u8,
                u32::from_str_radix(&bits[32 * i..32 * i + 32], 2).unwrap(),
            )
        })
        .collect()
}

/// One platform call the driver made, in order.
#[derive(Debug, Clone, PartialEq)]
pub enum Op {
    Read(u16, usize),
    Write(u16, Vec<u8>),
    Speed(u16),
    Width(u8),
    Xshut,
}

pub struct Die {
    pub page: u8,
    pub banks: BTreeMap<u8, [u8; 256]>,
    pub nvm: BTreeMap<u8, u32>,

    // ── knobs ──
    /// Result block per ranging shot; the last one repeats.
    pub frames: VecDeque<[u8; 12]>,
    pub frame: [u8; 12],
    /// 9.7 Mcps each enabled non-aperture reference SPAD adds.
    pub ref_rate_per_spad: u32,
    /// ... each aperture one.
    pub ref_rate_per_aperture_spad: u32,
    /// RESULT_INTERRUPT_STATUS polls before a shot is ready.
    pub ready_after_polls: u32,
    /// SYSRANGE_START reads that still show the start bit.
    pub start_bit_reads: u32,
    pub start_bit_stuck: bool,
    pub nacks_after_reset: u64,
    /// The soft reset does nothing.
    pub reset_ignored: bool,
    /// The NVM read strobe never comes.
    pub strobe_stuck: bool,
    /// SYSTEM_INTERRUPT_CLEAR does not clear.
    pub interrupt_sticky: bool,
    /// Writes to the SPAD enables are lost.
    pub spad_map_stuck: bool,
    /// RESULT_INTERRUPT_STATUS with nothing pending.
    pub interrupt_idle: u8,

    // ── what happened ──
    /// (SYSRANGE_START value, sequence config) of every shot.
    pub shots: Vec<(u8, u8)>,
    /// Reference rate of each reference shot.
    pub ref_peaks: Vec<u32>,
    pub ops: Vec<Op>,

    in_reset: bool,
    nacks: u64,
    start_reads: u32,
    /// Polls until ready; None = nothing pending.
    ready_in: Option<u32>,
}

impl Die {
    fn new(nvm: &[(u8, u32)]) -> Die {
        let mut all = nvm_defaults();
        all.extend(nvm.iter().copied());
        let mut d = Die {
            page: 0,
            banks: BTreeMap::new(),
            nvm: all,
            frames: VecDeque::new(),
            frame: FRAME_635_MM,
            ref_rate_per_spad: 400,
            ref_rate_per_aperture_spad: 100,
            ready_after_polls: 0,
            start_bit_reads: 0,
            start_bit_stuck: false,
            nacks_after_reset: 1,
            reset_ignored: false,
            strobe_stuck: false,
            interrupt_sticky: false,
            spad_map_stuck: false,
            interrupt_idle: 0x40,
            shots: Vec::new(),
            ref_peaks: Vec::new(),
            ops: Vec::new(),
            in_reset: false,
            nacks: 0,
            start_reads: 0,
            ready_in: None,
        };
        let b0 = d.bank(0);
        b0[0xC0] = MODEL_ID;
        b0[0x84] = 0x01; // GPIO_HV_MUX_ACTIVE_HIGH as read on the board
        b0[0xCB] = 0x22; // VHV settings after the reference calibration
        b0[0xEE] = 0x01; // phase cal
        b0[0xF8..0xFA].copy_from_slice(&[0x00, 0x28]); // OSC_CALIBRATE_VAL
        b0[0x14..0x20].copy_from_slice(&FRAME_635_MM);
        let b1 = d.bank(1);
        b1[0x91] = 0x3C; // the stop variable DataInit reads
        b1[0x84..0x86].copy_from_slice(&[0x97, 0x5C]); // oscillator frequency, FixPoint4.12
        d.bank(6)[0x83] = 0x01;
        d
    }

    pub fn bank(&mut self, page: u8) -> &mut [u8; 256] {
        self.banks.entry(page).or_insert([0u8; 256])
    }

    pub fn spad_enables(&mut self) -> Vec<u8> {
        self.bank(0)[0xB0..0xB6].to_vec()
    }

    /// The writes the driver made, as (address, bytes).
    pub fn writes(&self) -> Vec<(u16, Vec<u8>)> {
        self.ops
            .iter()
            .filter_map(|op| match op {
                Op::Write(a, d) => Some((*a, d.clone())),
                _ => None,
            })
            .collect()
    }

    /// The addresses the driver read.
    pub fn reads(&self) -> Vec<u16> {
        self.ops
            .iter()
            .filter_map(|op| match op {
                Op::Read(a, _) => Some(*a),
                _ => None,
            })
            .collect()
    }

    // ── register map ──
    pub fn read(&mut self, addr: u16, n: usize) -> std::result::Result<Vec<u8>, ()> {
        let page = self.page;
        let a = addr as usize;
        if page == 0 && a == 0xC0 {
            if self.nacks > 0 {
                self.nacks -= 1;
                return Err(());
            }
            let mut out = vec![if self.in_reset { 0x00 } else { MODEL_ID }];
            out.extend_from_slice(&self.bank(0)[0xC1..0xC0 + n]);
            return Ok(out);
        }
        if page == 7 && a == 0x83 {
            return Ok(vec![if self.strobe_stuck { 0x00 } else { 0x10 }]);
        }
        if page == 7 && a == 0x90 {
            let index = self.bank(7)[0x94];
            let word = self.nvm.get(&index).copied().unwrap_or(0);
            return Ok(word.to_be_bytes()[..n].to_vec());
        }
        if page == 0 && a == 0x00 {
            let v = self.bank(0)[0x00];
            if self.start_reads > 0 || self.start_bit_stuck {
                self.start_reads = self.start_reads.saturating_sub(1);
                return Ok(vec![v | 0x01]);
            }
            return Ok(vec![v & 0xFE]);
        }
        if page == 0 && a == 0x13 {
            let ready = self.ready_in == Some(0);
            if let Some(r) = self.ready_in {
                if r > 0 {
                    self.ready_in = Some(r - 1);
                }
            }
            return Ok(vec![self.interrupt_idle | if ready { 0x04 } else { 0 }]);
        }
        Ok(self.bank(page)[a..a + n].to_vec())
    }

    pub fn write(&mut self, addr: u16, data: &[u8]) {
        let a = addr as usize;
        if a == 0xFF {
            self.page = data[0];
            return;
        }
        let page = self.page;
        if page == 0 && a == 0xBF && !self.reset_ignored {
            if data[0] == 0x00 {
                self.in_reset = true;
            } else {
                self.in_reset = false;
                self.nacks = self.nacks_after_reset;
            }
        } else if page == 0 && a == 0x00 && data[0] & 0x01 != 0 {
            self.shot(data[0]);
        } else if page == 0 && a == 0x0B && data[0] == 0x01 && !self.interrupt_sticky {
            self.ready_in = None;
        } else if page == 0 && (0xB0..=0xB5).contains(&a) && self.spad_map_stuck {
            return;
        }
        self.bank(page)[a..a + data.len()].copy_from_slice(data);
    }

    // ── a measurement ──
    fn shot(&mut self, value: u8) {
        let sequence = self.bank(0)[0x01];
        self.shots.push((value, sequence));
        self.start_reads = self.start_bit_reads;
        self.ready_in = Some(self.ready_after_polls);
        if sequence == 0xC0 {
            // Reference-signal measurement: the peak reference rate follows the SPAD enables.
            let peak = self.ref_peak();
            self.ref_peaks.push(peak);
            self.bank(1)[0xB6..0xB8].copy_from_slice(&(peak as u16).to_be_bytes());
        } else if sequence != 0x01 && sequence != 0x02 {
            // VHV / phase shots range nothing.
            if let Some(f) = self.frames.pop_front() {
                self.frame = f;
            }
            let frame = self.frame;
            self.bank(0)[0x14..0x20].copy_from_slice(&frame);
        }
    }

    fn ref_peak(&mut self) -> u32 {
        let enables = self.spad_enables();
        let mut peak = 0;
        for index in 0..8 * enables.len() {
            if (enables[index / 8] >> (index % 8)) & 1 != 0 {
                peak += if START_SELECT + index >= FIRST_APERTURE_SPAD {
                    self.ref_rate_per_aperture_spad
                } else {
                    self.ref_rate_per_spad
                };
            }
        }
        peak.min(0xFFFF)
    }
}

/// The die as the driver's platform. Sleeps are no-ops: every wait in the driver is a bounded poll.
pub struct FakeVl53l0x {
    die: Mutex<Die>,
}

impl FakeVl53l0x {
    /// A die with the capture's defaults, NVM words overridden by `nvm`.
    pub fn new(nvm: &[(u8, u32)]) -> Arc<FakeVl53l0x> {
        Arc::new(FakeVl53l0x {
            die: Mutex::new(Die::new(nvm)),
        })
    }

    pub fn die(&self) -> MutexGuard<'_, Die> {
        self.die.lock().unwrap_or_else(|e| e.into_inner())
    }
}

impl Vl53lxPlatform for FakeVl53l0x {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut d = self.die();
        d.ops.push(Op::Read(addr, size));
        d.read(addr, size).map_err(|_| DepzError::Status {
            cmd: READ_REG,
            status: ERR_HARDWARE_FAULT,
        })
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        let mut d = self.die();
        d.ops.push(Op::Write(addr, data.to_vec()));
        d.write(addr, data);
        Ok(())
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()> {
        self.die().ops.push(Op::Speed(khz));
        Ok(())
    }

    fn set_addr_width(&self, width: u8) -> Result<()> {
        self.die().ops.push(Op::Width(width));
        Ok(())
    }

    fn xshut_reset(&self) -> Result<()> {
        self.die().ops.push(Op::Xshut);
        Ok(())
    }

    fn sleep_ms(&self, _ms: u32) {}
}
