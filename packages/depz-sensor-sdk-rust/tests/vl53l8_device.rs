//! The VL53L8 / L5 / L7 device classes over a fake register bridge: the host-side guards (the
//! scenarios replay the full sessions).
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::devices::{
    vl53l8_max_ranging_frequency_hz, DepzDevice, DepzError, Vl53l5cxDevice, Vl53l7cxDevice,
    Vl53l8chDevice, Vl53l8cxDevice,
};
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::vl53l7::{Vl53l7Cmd, Vl53l7Rpt, PIN_LPN_OFF};
use depz_sensor_sdk::vl53l8::decode::swap_buffer;
use depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup;
use depz_sensor_sdk::vl53l8::{Vl53l8Cmd, Vl53l8Rpt};

/// A bridge whose 64 KB register bank answers every read; the page register 0x7FFF is ordinary.
fn board(bank: Arc<Mutex<Vec<u8>>>) -> FakeBoard {
    let link = FakeBoard::new();
    let l = link.clone();
    link.on_packet(move |p| {
        let mut regs = bank.lock().unwrap();
        if p.cmd == Vl53l8Cmd::ReadReg as u8 {
            let addr = u16::from_le_bytes([p.payload[0], p.payload[1]]) as usize;
            let n = u16::from_le_bytes([p.payload[2], p.payload[3]]) as usize;
            let mut reply = vec![0u8; 9 + n];
            reply[0] = p.cmd;
            reply[9..].copy_from_slice(&regs[addr..addr + n]);
            return vec![l.reply(Vl53l8Rpt::RegData as u8, &reply)];
        }
        if p.cmd == Vl53l8Cmd::WriteReg as u8 {
            let addr = u16::from_le_bytes([p.payload[0], p.payload[1]]) as usize;
            let data = &p.payload[2..];
            regs[addr..addr + data.len()].copy_from_slice(data);
        }
        if p.cmd == Vl53l7Cmd::GetInfo as u8 {
            let mut info = vec![0u8; 20];
            info[15..17].copy_from_slice(&400u16.to_le_bytes());
            return vec![l.reply(Vl53l7Rpt::Info as u8, &info)];
        }
        vec![FakeBoard::status(p.cmd, Status::Ok as u8)]
    });
    link
}

fn dev(link: &FakeBoard) -> DepzDevice {
    DepzDevice::new(link.clone(), Some(Duration::from_millis(500)))
}

fn bank() -> Arc<Mutex<Vec<u8>>> {
    Arc::new(Mutex::new(vec![0u8; 0x10000]))
}

#[test]
fn configuration_before_init_is_refused_without_a_request() {
    let link = board(bank());
    let d = Vl53l8cxDevice::attach(dev(&link));
    assert!(!d.initialized());
    for r in [
        d.get_resolution().map(|_| ()),
        d.set_ranging_frequency_hz(15),
        d.start_ranging(),
    ] {
        assert!(matches!(r, Err(DepzError::InvalidState(_))), "{r:?}");
    }
    assert!(d.stop_ranging().is_ok(), "not ranging: nothing to stop");
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn arguments_are_checked_before_the_driver() {
    let link = board(bank());
    let d = Vl53l8cxDevice::attach(dev(&link));
    assert!(matches!(
        d.set_resolution(32),
        Err(DepzError::InvalidArgument(_))
    ));
    // Below 2 Hz the L8 never ranges; the L5/L7 go down to 1 Hz (and then need init).
    assert!(matches!(
        d.set_ranging_frequency_hz(1),
        Err(DepzError::InvalidArgument(_))
    ));
    // contract 11 §3 (ERRATA A13): at most 60 Hz at 4x4 (the resolution after
    // open) and 15 Hz at 8x8 — refused before the driver, nothing is sent.
    assert_eq!(vl53l8_max_ranging_frequency_hz(16), 60);
    assert_eq!(vl53l8_max_ranging_frequency_hz(64), 15);
    assert!(matches!(
        d.set_ranging_frequency_hz(61),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(link.sent_snapshot().is_empty());
    let l7 = Vl53l7cxDevice::attach(dev(&link));
    assert!(matches!(
        l7.set_ranging_frequency_hz(1),
        Err(DepzError::InvalidState(_))
    ));
    assert!(matches!(
        l7.set_ranging_frequency_hz(0),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        l7.set_i2c_speed_khz(0),
        Err(DepzError::InvalidArgument(_))
    ));
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn a_blank_cnh_setup_is_refused_before_anything_is_sent() {
    let link = board(bank());
    let d = Vl53l8chDevice::attach(dev(&link));
    let r = d.configure_cnh(&Vl53l8CnhSetup::new());
    assert!(matches!(r, Err(DepzError::InvalidArgument(_))), "{r:?}");
    assert!(d.cnh_setup().is_none());
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn is_alive_reads_the_silicon_ids_of_the_class() {
    // Each device owns its link (dropping one closes it): a fresh board per probe.
    let alive = |id: u8, rev: u8, attach: fn(DepzDevice) -> bool| {
        let regs = bank();
        regs.lock().unwrap()[..2].copy_from_slice(&[id, rev]);
        attach(dev(&board(regs)))
    };
    let l8 = |d: DepzDevice| Vl53l8cxDevice::attach(d).is_alive();
    let l7 = |d: DepzDevice| Vl53l7cxDevice::attach(d).is_alive();
    let l5 = |d: DepzDevice| Vl53l5cxDevice::attach(d).is_alive();
    assert!(alive(0xF0, 0x0C, l8));
    // The L8 ids are not an L5/L7 over I2C (revision 0x02, or 0x01 with id 0xF0), nor the reverse.
    assert!(!alive(0xF0, 0x0C, l7));
    assert!(alive(0xF0, 0x02, l5));
    assert!(alive(0xF0, 0x01, l7));
    assert!(!alive(0xF0, 0x02, l8));
}

#[test]
fn the_board_commands_of_the_i2c_bridge() {
    let link = board(bank());
    let d = Vl53l7cxDevice::attach(dev(&link));
    assert_eq!(d.get_bridge_info().unwrap().i2c_khz, 400);
    assert_eq!(d.set_i2c_speed_khz(400).unwrap(), 400);
    d.pin_ctrl(PIN_LPN_OFF).unwrap();
    assert!(!d.initialized());
    assert_eq!(d.module_type(), None);
    let sent: Vec<u8> = link.sent_snapshot().iter().map(|p| p.cmd).collect();
    assert_eq!(
        sent,
        vec![
            Vl53l7Cmd::GetInfo as u8,
            Vl53l7Cmd::SetI2cSpeed as u8,
            Vl53l7Cmd::GetInfo as u8,
            Vl53l7Cmd::PinCtrl as u8
        ]
    );
}

// ── CNH ↔ motion indicator: one DCI block, mutually exclusive (ERRATA B4) ────────────────────

/// A VL53L8 behind the bridge, enough for init / configuration / ranging: the page register, the
/// boot and firmware-access polls, the UI command area with a DCI store, the CH checksum; frames
/// are laid out from the output list the host programmed, every block zero.
#[derive(Default)]
struct Sensor {
    page: u8,
    reg: std::collections::HashMap<(u8, u16), u8>,
    dci: std::collections::HashMap<u16, Vec<u8>>,
    pending: Option<(u16, usize)>,
    frame_size: u16,
}

impl Sensor {
    fn write(&mut self, addr: u16, d: &[u8]) {
        if addr == 0x7FFF && d.len() == 1 {
            self.page = d[0];
            return;
        }
        if self.page > 2 {
            return; // firmware banks
        }
        let n = d.len();
        if self.page == 2 {
            if addr == 0x2FFF - 11 && n == 12 {
                let size = (d[2] as usize) << 4 | (d[3] as usize) >> 4;
                self.pending = Some(((d[0] as u16) << 8 | d[1] as u16, size));
                return;
            }
            if n >= 12 && d[n - 5] == 0x0F && d[n - 4] == 0x05 && d[n - 3] == 0x01 {
                let size = (d[2] as usize) << 4 | (d[3] as usize) >> 4;
                self.dci.insert(
                    (d[0] as u16) << 8 | d[1] as u16,
                    swap_buffer(&d[4..4 + size]),
                );
                return;
            }
            if addr >= 0x2C00 {
                return;
            }
        }
        for (i, &v) in d.iter().enumerate() {
            self.reg.insert((self.page, addr + i as u16), v);
        }
        if self.page == 0 && addr == 0x14 && d[0] == 0x01 {
            self.reg.insert((0, 0x06), 0x81); // MCU stopped
        }
        if self.page == 0 && addr == 0x14 && d[0] == 0x00 {
            self.reg.insert((0, 0x06), 0x01);
        }
    }

    fn read(&mut self, addr: u16, n: usize) -> Vec<u8> {
        let mut out = vec![0u8; n];
        if addr == 0x7FFF {
            out[0] = self.page;
        } else if self.page == 2 && addr == 0x2C00 {
            out[..4.min(n)].copy_from_slice(&[0x02, 0x03, 0x00, 0x00][..4.min(n)]);
        } else if self.page == 2 && addr == 0x2C04 && self.pending.is_some() {
            let (index, size) = self.pending.take().unwrap();
            let mut raw = vec![0u8; size + 12];
            if index == 0x5440 {
                raw[12..14].copy_from_slice(&self.dci_u32(0xD968, 0).to_le_bytes()[..2]);
            } else if let Some(v) = self.dci.get(&index) {
                let k = v.len().min(size);
                raw[4..4 + k].copy_from_slice(&v[..k]);
            }
            let raw = swap_buffer(&raw);
            let k = raw.len().min(n);
            out[..k].copy_from_slice(&raw[..k]);
        } else {
            for (i, b) in out.iter_mut().enumerate() {
                *b = *self.reg.get(&(self.page, addr + i as u16)).unwrap_or(&0);
            }
        }
        out
    }

    fn dci_u32(&self, index: u16, word: usize) -> u32 {
        let v = self.dci.get(&index).cloned().unwrap_or_default();
        let mut b = [0u8; 4];
        for (i, x) in b.iter_mut().enumerate() {
            *x = *v.get(4 * word + i).unwrap_or(&0);
        }
        u32::from_le_bytes(b)
    }

    /// The next frame (wire order) for the programmed output list.
    fn frame(&self) -> Vec<u8> {
        let size = self.dci_u32(0xD968, 0) as usize;
        let list = self.dci.get(&0xD980).cloned().unwrap_or_default();
        let mut buf = vec![0u8; size];
        buf[..4].copy_from_slice(&[0x31, 0x9D, 0x05, 0x01]);
        buf[8..10].copy_from_slice(&0x0017u16.to_le_bytes());
        buf[12..16].copy_from_slice(&list[..4]);
        let mut pos = 16;
        for i in 1..list.len() / 4 {
            let bh = u32::from_le_bytes(list[4 * i..4 * i + 4].try_into().unwrap());
            if (self.dci_u32(0xD970, i / 32) >> (i % 32)) & 1 == 0 {
                continue;
            }
            let (t, sz) = (bh & 0xF, ((bh >> 4) & 0xFFF) as usize);
            let msize = if t > 1 && t < 0xD {
                t as usize * sz
            } else {
                sz
            };
            buf[pos..pos + 4].copy_from_slice(&bh.to_le_bytes());
            pos += 4 + msize;
        }
        buf[pos..pos + 4].copy_from_slice(&0x0Eu32.to_le_bytes());
        buf[pos + 4..pos + 8].copy_from_slice(&0x0Fu32.to_le_bytes());
        buf[size - 4..size - 2].copy_from_slice(&0x0017u16.to_le_bytes()); // CH: footer id at size-4
        swap_buffer(&buf)
    }
}

fn l8ch_board() -> (FakeBoard, Arc<Mutex<Sensor>>) {
    let sensor = Arc::new(Mutex::new(Sensor::default()));
    {
        let mut s = sensor.lock().unwrap();
        s.reg.insert((0, 0x00), 0xF0);
        s.reg.insert((0, 0x01), 0x0C);
        s.reg.insert((0, 0x06), 0x01); // sensor / MCU booted
        s.reg.insert((1, 0x21), 0x04); // SPI firmware access
        for (i, b) in swap_buffer(&0x0C0B6C9Eu32.to_le_bytes())
            .into_iter()
            .enumerate()
        {
            s.reg.insert((2, 0x2FFC + i as u16), b); // the CH blob's checksum
        }
    }
    let link = FakeBoard::new();
    let (l, s) = (link.clone(), sensor.clone());
    link.on_packet(move |p| {
        let mut s = s.lock().unwrap();
        if p.cmd == Vl53l8Cmd::ReadReg as u8 {
            let addr = u16::from_le_bytes([p.payload[0], p.payload[1]]);
            let n = u16::from_le_bytes([p.payload[2], p.payload[3]]) as usize;
            let mut reply = vec![0u8; 9];
            reply[0] = p.cmd;
            reply.extend(s.read(addr, n));
            return vec![l.reply(Vl53l8Rpt::RegData as u8, &reply)];
        }
        if p.cmd == Vl53l8Cmd::WriteReg as u8 {
            s.write(
                u16::from_le_bytes([p.payload[0], p.payload[1]]),
                &p.payload[2..],
            );
        }
        if p.cmd == Vl53l8Cmd::StartStream as u8 {
            s.frame_size = u16::from_le_bytes([p.payload[0], p.payload[1]]);
        }
        vec![FakeBoard::status(p.cmd, Status::Ok as u8)]
    });
    (link, sensor)
}

/// One INT edge: the next frame in ≤ 1528-byte RPT_VL53_FRAME chunks.
fn send_frame(link: &FakeBoard, sensor: &Arc<Mutex<Sensor>>) {
    let raw = sensor.lock().unwrap().frame();
    for (k, part) in raw.chunks(1528).enumerate() {
        let mut p = 1_000_000u64.to_le_bytes().to_vec();
        p.extend((raw.len() as u16).to_le_bytes());
        p.extend(((k * 1528) as u16).to_le_bytes());
        p.extend(part);
        link.inject(link.reply(Vl53l8Rpt::Vl53Frame as u8, &p));
    }
}

#[test]
fn cnh_and_the_motion_indicator_disarm_each_other() {
    let (link, sensor) = l8ch_board();
    let d = Vl53l8chDevice::attach(dev(&link));
    let take = || {
        let frames = d.frames(2);
        send_frame(&link, &sensor);
        frames.take(Duration::from_secs(2)).unwrap()
    };
    let mut cnh = Vl53l8CnhSetup::new();
    cnh.init_config(10, 20, 2).unwrap();
    cnh.create_agg_map(64, 0, 0, 2, 2, 4, 4).unwrap();
    d.init().unwrap();
    d.set_resolution(64).unwrap();

    // motion, then CNH: 0xBFAC holds the CNH setup, frames carry no motion
    d.configure_motion_indicator(None).unwrap();
    d.configure_cnh(&cnh).unwrap();
    assert_eq!(sensor.lock().unwrap().dci[&0xBFAC], cnh.pack());
    assert!(d.cnh_setup().is_some());
    d.start_ranging().unwrap();
    assert_eq!(sensor.lock().unwrap().frame_size, 3156);
    let f = take();
    assert!(f.motion.is_none());
    assert_eq!(f.cnh_raw.map(|r| r.len()), Some(1708));
    d.stop_ranging().unwrap();

    // CNH, then motion: 0xBFAC holds the motion config, the CNH block is gone
    d.configure_motion_indicator(None).unwrap();
    assert!(d.cnh_setup().is_none());
    d.start_ranging().unwrap();
    assert_eq!(sensor.lock().unwrap().frame_size, 1444);
    let f = take();
    assert!(f.motion.is_some() && f.cnh_raw.is_none());
    d.stop_ranging().unwrap();

    // init() disarms both
    d.configure_cnh(&cnh).unwrap();
    d.init().unwrap();
    assert!(d.cnh_setup().is_none());
    d.set_resolution(64).unwrap();
    d.start_ranging().unwrap();
    assert_eq!(sensor.lock().unwrap().frame_size, 1444);
    let f = take();
    assert!(f.motion.is_none() && f.cnh_raw.is_none());
    d.stop_ranging().unwrap();
    assert_eq!(d.frame_parse_errors(), 0);
}
