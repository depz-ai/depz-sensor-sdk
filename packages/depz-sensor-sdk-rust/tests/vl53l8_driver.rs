//! The VL53L8 / L5 / L7 ULD over an emulated register bridge and DCI transport (a port of the
//! Java SDK's Vl53l8DriverTests, itself a port of the Python FakeUldPlatform tests).

use std::collections::HashMap;
use std::sync::{Arc, Mutex, MutexGuard};

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::vl53l8::advanced::{
    DetectionThreshold, DIST_MM, POWER_MODE_DEEP_SLEEP, POWER_MODE_SLEEP, POWER_MODE_WAKEUP,
};
use depz_sensor_sdk::vl53l8::decode::swap_buffer;
use depz_sensor_sdk::vl53l8::driver::{
    status, tables, UldResult, Vl53l8CnhSetup, Vl53l8Driver, Vl53l8Firmware, Vl53l8Platform,
    Vl53l8UldError, MODULE_TYPE_MZEVO, OFFSET_BUFFER_SIZE, RANGING_MODE_AUTONOMOUS,
    TARGET_ORDER_CLOSEST, XTALK_BUFFER_SIZE,
};

const DCI_ZONE_CONFIG: u16 = 0x5450;
const DCI_FREQ_HZ: u16 = 0x5458;
const DCI_INT_TIME: u16 = 0x545C;
const DCI_RANGING_MODE: u16 = 0xAD30;
const DCI_TARGET_ORDER: u16 = 0xAE64;
const DCI_OUTPUT_CONFIG: u16 = 0xD968;
const DCI_OUTPUT_ENABLES: u16 = 0xD970;
const DCI_OUTPUT_LIST: u16 = 0xD980;
const DCI_PIPE_CONTROL: u16 = 0xDB80;
const DCI_SINGLE_RANGE: u16 = 0xD964;
const DCI_FW_FLAGS: u16 = 0xE0C4;
const DCI_CAL_CFG: u16 = 0x5470;
const DCI_MOTION_CFG: u16 = 0xBFAC;
const DCI_THRESH_START: u16 = 0xB6E8;
const DCI_THRESH_VALID: u16 = 0xB9F0;
const UI_CMD_STATUS: u16 = 0x2C00;
const UI_CMD_START: u16 = 0x2C04;
const UI_CMD_END: u16 = 0x2FFF;

type W = (u16, Vec<u8>);

/// The fake's state: DCI reads and writes land in `dci`, plain writes in `reg`; the command
/// status answers both the NVM poll (byte 0 = 2) and the DCI polls (byte 1 = 3); firmware banks
/// are recorded, not stored; `xtalk_reply` answers the GET_XTALK command.
#[derive(Default)]
struct State {
    reg: HashMap<u32, u8>,
    dci: HashMap<u16, Vec<u8>>,
    writes: Vec<W>,
    reads: Vec<(u16, usize)>,
    banks: Vec<(i32, usize)>,
    status: Vec<u8>,
    xtalk_reply: Option<Vec<u8>>,
    pending_read: Option<(u16, usize)>,
    xtalk_pending: bool,
    page: i32,
}

/// The register bridge and DCI transport, emulated.
struct Fake(Mutex<State>);

impl Fake {
    fn new() -> Arc<Fake> {
        Arc::new(Fake(Mutex::new(State {
            status: vec![0x02, 0x03, 0x00, 0x00],
            page: -1,
            ..State::default()
        })))
    }

    fn s(&self) -> MutexGuard<'_, State> {
        self.0.lock().unwrap()
    }

    fn set_u32(&self, addr: u32, value: u32, swapped: bool) {
        let mut b = value.to_le_bytes().to_vec();
        if swapped {
            b = swap_buffer(&b);
        }
        let mut s = self.s();
        for (i, v) in b.into_iter().enumerate() {
            s.reg.insert(addr + i as u32, v);
        }
    }

    fn put(&self, addr: u32, value: u8) {
        self.s().reg.insert(addr, value);
    }

    fn dci_put(&self, index: u16, data: &[u8]) {
        self.s().dci.insert(index, data.to_vec());
    }

    fn dci(&self, index: u16) -> Option<Vec<u8>> {
        self.s().dci.get(&index).cloned()
    }

    fn fw_reports(&self, frame_size: usize) {
        let mut b = vec![0u8; 12];
        b[8] = frame_size as u8;
        b[9] = (frame_size >> 8) as u8;
        self.dci_put(0x5440, &b);
    }

    fn zones(&self, resolution: usize) {
        let side = if resolution == 16 { 4 } else { 8 };
        self.dci_put(DCI_ZONE_CONFIG, &[side, side, 0, 0, 0, 0, 0, 0]);
    }

    fn count(&self, addr: u16, size: usize) -> usize {
        self.s()
            .reads
            .iter()
            .filter(|r| **r == (addr, size))
            .count()
    }

    fn writes(&self) -> Vec<W> {
        self.s().writes.clone()
    }

    fn addrs(&self) -> Vec<u16> {
        self.s().writes.iter().map(|w| w.0).collect()
    }

    fn wrote(&self, addr: u16, first: u8) -> bool {
        self.s()
            .writes
            .iter()
            .any(|w| w.0 == addr && w.1[0] == first)
    }

    fn banks(&self) -> Vec<(i32, usize)> {
        self.s().banks.clone()
    }
}

impl Vl53l8Platform for Fake {
    fn sleep_ms(&self, _ms: u32) {}

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        let mut s = self.s();
        if addr == 0x7FFF && data.len() == 1 {
            s.page = data[0] as i32;
        }
        s.writes.push((addr, data.to_vec()));
        if data.len() >= 0x5000 {
            let page = s.page;
            s.banks.push((page, data.len()));
            return Ok(());
        }
        if addr == 0x2FB8 {
            s.xtalk_pending = true;
            s.pending_read = None;
            return Ok(());
        }
        let n = data.len();
        if addr == UI_CMD_END - 11 && n == 12 {
            s.xtalk_pending = false;
            s.pending_read = Some((
                (data[0] as u16) << 8 | data[1] as u16,
                (data[2] as usize) << 4 | (data[3] as usize) >> 4,
            ));
            return Ok(());
        }
        if n >= 12 && data[n - 5] == 0x0F && data[n - 4] == 0x05 && data[n - 3] == 0x01 {
            let index = (data[0] as u16) << 8 | data[1] as u16;
            let size = (data[2] as usize) << 4 | (data[3] as usize) >> 4;
            s.dci.insert(index, swap_buffer(&data[4..4 + size]));
            return Ok(());
        }
        for (i, &v) in data.iter().enumerate() {
            s.reg.insert(addr as u32 + i as u32, v);
        }
        Ok(())
    }

    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut s = self.s();
        s.reads.push((addr, size));
        if addr == UI_CMD_STATUS {
            let mut out = s.status.clone();
            out.resize(size, 0);
            return Ok(out);
        }
        if addr == UI_CMD_START && s.xtalk_pending {
            if let Some(reply) = &s.xtalk_reply {
                return Ok(reply[..size.min(reply.len())].to_vec());
            }
        }
        if addr == UI_CMD_START {
            if let Some((index, len)) = s.pending_read {
                let stored = s.dci.get(&index).cloned().unwrap_or_default();
                let mut raw = vec![0u8; len + 12];
                let n = len.min(stored.len());
                raw[4..4 + n].copy_from_slice(&stored[..n]);
                return Ok(swap_buffer(&raw));
            }
        }
        Ok((0..size)
            .map(|i| *s.reg.get(&(addr as u32 + i as u32)).unwrap_or(&0))
            .collect())
    }
}

type Drv = Vl53l8Driver<Arc<Fake>>;

fn driver(p: &Arc<Fake>, fw: Vl53l8Firmware) -> Drv {
    Vl53l8Driver::new(p.clone(), fw)
}

fn u32s(b: &[u8]) -> Vec<u32> {
    b.as_chunks::<4>()
        .0
        .iter()
        .map(|c| u32::from_le_bytes(*c))
        .collect()
}

fn put_u16(b: &mut [u8], off: usize, v: i32) {
    b[off] = v as u8;
    b[off + 1] = (v >> 8) as u8;
}

fn put_u32(b: &mut [u8], off: usize, v: u32) {
    b[off..off + 4].copy_from_slice(&v.to_le_bytes());
}

fn booting(fw: Vl53l8Firmware) -> Arc<Fake> {
    let p = Fake::new();
    let i2c = matches!(fw, Vl53l8Firmware::L7cx | Vl53l8Firmware::L7ch);
    p.put(0x06, 0x01); // sensor boot / MCU boot
    p.put(0x21, if i2c { 0x10 } else { 0x04 });
    if fw == Vl53l8Firmware::Cx {
        p.set_u32(0x2FFC, 0xCADF7CAF, true);
    } else if fw != Vl53l8Firmware::L7cx {
        p.set_u32(0x2FFC, 0x0C0B6C9E, true);
    }
    if i2c {
        p.put(0x00, 0xF0);
        p.put(0x01, 0x02);
    }
    p
}

fn ranging(resolution: usize) -> Arc<Fake> {
    let p = Fake::new();
    p.zones(resolution);
    p
}

fn undo() -> Vec<W> {
    vec![
        (0x7FFF, vec![0x00]),
        (0x14, vec![0x00]),
        (0x15, vec![0x00]),
        (0x09, vec![0x04]),
        (0x7FFF, vec![0x02]),
    ]
}

/// The ULD status code of a failed call.
#[track_caller]
fn code<T: std::fmt::Debug>(r: UldResult<T>) -> u8 {
    match r {
        Err(e) => e
            .status_code()
            .unwrap_or_else(|| panic!("not a ULD status: {e:?}")),
        Ok(v) => panic!("expected a ULD status, got Ok({v:?})"),
    }
}

#[track_caller]
fn invalid_argument<T: std::fmt::Debug>(r: UldResult<T>) {
    match r {
        Err(Vl53l8UldError::InvalidArgument(_)) => {}
        other => panic!("expected InvalidArgument, got {other:?}"),
    }
}

// ── init ─────────────────────────────────────────────────────────────────────

#[test]
fn init_downloads_three_banks_and_uploads_the_configuration() {
    let p = booting(Vl53l8Firmware::Cx);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    let mut notes: Vec<String> = Vec::new();
    drv.init_with_progress(&mut |s| notes.push(s.to_string()))
        .unwrap();
    assert_eq!(
        vec![(0x09, 0x8000), (0x0A, 0x8000), (0x0B, 0x5000)],
        p.banks()
    );
    assert!(
        p.writes()
            .iter()
            .filter(|w| w.1.len() >= 0x5000)
            .all(|w| w.0 == 0),
        "banks at 0"
    );
    let addrs = p.addrs();
    let at = |a: u16| addrs.iter().position(|&x| x == a).unwrap();
    assert!(at(0x2FD8) < at(0x2E18), "nvm before offset");
    assert!(at(0x2E18) < at(0x2CF8), "offset before xtalk");
    assert!(at(0x2CF8) < at(0x2C34), "xtalk before config");
    let writes = p.writes();
    let offset = &writes.iter().find(|w| w.0 == 0x2E18).unwrap().1;
    assert_eq!(OFFSET_BUFFER_SIZE, offset.len());
    assert_eq!(
        &[0, 0, 0, 0x0F, 0x03, 0x01, 0x01, 0xE4],
        &offset[offset.len() - 8..]
    );
    assert_eq!(
        XTALK_BUFFER_SIZE,
        writes.iter().find(|w| w.0 == 0x2CF8).unwrap().1.len()
    );
    assert_eq!(Some(vec![1, 0, 1, 0]), p.dci(DCI_PIPE_CONTROL));
    assert_eq!(Some(vec![1, 0, 0, 0]), p.dci(DCI_SINGLE_RANGE));
    assert!(
        notes.iter().any(|n| n == "Sensor FW checksum OK"),
        "checksum note"
    );
    assert_eq!("Sensor init done", notes.last().unwrap());
    assert_eq!(None, drv.module_type());
    assert_eq!(XTALK_BUFFER_SIZE, drv.xtalk_data().len());
}

#[test]
fn init_refuses_a_wrong_firmware_checksum() {
    let p = booting(Vl53l8Firmware::Cx);
    p.set_u32(0x2FFC, 0xDEADBEEF, true);
    let r = driver(&p, Vl53l8Firmware::Cx).init();
    assert_eq!(status::FW_CHECKSUM_FAIL, code(r));
}

#[test]
fn init_times_out_when_the_sensor_never_boots() {
    let p = booting(Vl53l8Firmware::Cx);
    p.put(0x06, 0x00);
    assert_eq!(
        status::TIMEOUT_ERROR,
        code(driver(&p, Vl53l8Firmware::Cx).init())
    );
    assert_eq!(201, p.count(0x06, 1), "2 s at 10 ms, then the timeout");
    assert!(p.banks().is_empty());
}

#[test]
fn init_stops_on_an_mcu_error_in_the_command_status() {
    let p = booting(Vl53l8Firmware::Cx);
    p.s().status = vec![0x02, 0x03, 0x7F, 0x00];
    assert_eq!(
        status::MCU_ERROR,
        code(driver(&p, Vl53l8Firmware::Cx).init())
    );
}

#[test]
fn l7cx_init_checks_silicon_skips_the_checksum_and_reads_the_module_type() {
    let p = booting(Vl53l8Firmware::L7cx);
    p.dci_put(DCI_FW_FLAGS, &[0, MODULE_TYPE_MZEVO, 0, 0, 0, 0, 0, 0]);
    let mut drv = driver(&p, Vl53l8Firmware::L7cx);
    let mut notes: Vec<String> = Vec::new();
    drv.init_with_progress(&mut |s| notes.push(s.to_string()))
        .unwrap();
    assert_eq!(Some(MODULE_TYPE_MZEVO), drv.module_type());
    assert_eq!(
        "Sensor init done (module type MZEVO)",
        notes.last().unwrap()
    );
    assert!(
        !notes.iter().any(|n| n == "Sensor FW checksum OK"),
        "no checksum on L7CX"
    );
    assert_eq!(0, p.count(0x2FFC, 4));
    assert!(
        p.wrote(0x20, 0x07) && p.wrote(0x20, 0x06),
        "L7 boot writes to 0x20"
    );
}

#[test]
fn l7cx_init_refuses_l8_silicon() {
    let p = booting(Vl53l8Firmware::L7cx);
    p.put(0x01, 0x0C);
    let e = driver(&p, Vl53l8Firmware::L7cx).init().unwrap_err();
    assert_eq!(Some(status::ERROR), e.status_code());
    assert!(e.to_string().contains("not a VL53L5/L7 sensor"), "{e}");
    assert!(p.banks().is_empty());
}

#[test]
fn every_firmware_set_loads_and_boots() {
    for fw in Vl53l8Firmware::ALL {
        let p = booting(fw);
        let mut drv = driver(&p, fw);
        drv.init().unwrap();
        assert_eq!(3, p.banks().len(), "{fw:?}");
        assert_eq!(
            matches!(fw, Vl53l8Firmware::L7cx | Vl53l8Firmware::L7ch),
            drv.is_i2c(),
            "{fw:?}"
        );
    }
}

// ── start / stop ranging ─────────────────────────────────────────────────────

#[test]
fn start_ranging_sizes_the_frame_and_programs_the_outputs() {
    let cases = [
        (Vl53l8Firmware::Cx, 64, 1452),
        (Vl53l8Firmware::Cx, 16, 540),
        (Vl53l8Firmware::Ch, 64, 1444), // 24-byte tail on the 2.0.x firmware
        (Vl53l8Firmware::L7cx, 16, 1060), // per-target blocks keep 64 entries
        (Vl53l8Firmware::L7cx, 64, 1444),
    ];
    for (fw, resolution, size) in cases {
        let at = format!("{fw:?}/{resolution}");
        let p = ranging(resolution);
        let mut drv = driver(&p, fw);
        p.fw_reports(size);
        drv.start_ranging(None).unwrap();
        assert_eq!(size, drv.data_read_size(), "{at}");
        assert_eq!(None, drv.frame_size_mismatch(), "{at}");
        assert_eq!(Some(resolution), drv.resolution(), "{at}");
        assert_eq!(
            vec![size as u32, 13],
            u32s(&p.dci(DCI_OUTPUT_CONFIG).unwrap()),
            "{at}"
        );
        let w3 = if matches!(fw, Vl53l8Firmware::Ch | Vl53l8Firmware::L7ch) {
            0
        } else {
            0xC0000000
        };
        assert_eq!(
            vec![0xFFF, 0, 0, w3],
            u32s(&p.dci(DCI_OUTPUT_ENABLES).unwrap()),
            "{at}"
        );
        let output = u32s(&p.dci(DCI_OUTPUT_LIST).unwrap());
        assert_eq!(12, output.len(), "{at}");
        let sizes: HashMap<u32, u32> = output
            .iter()
            .map(|&bh| (bh >> 16, (bh >> 4) & 0xFFF))
            .collect();
        assert_eq!(resolution as u32, sizes[&0x54D0], "{at} ambient");
        assert_eq!(
            if drv.is_i2c() { 64 } else { resolution as u32 },
            sizes[&0xDF44],
            "{at} distance"
        );
        assert_eq!(0x8C, sizes[&0xD858], "{at} motion: not zone-scaled");
    }
}

#[test]
fn start_ranging_with_cnh_appends_block_13() {
    let p = ranging(64);
    let mut drv = driver(&p, Vl53l8Firmware::Ch);
    let cnh: u32 = 1708;
    let size = 1444 + cnh + 4;
    p.fw_reports(size as usize);
    drv.start_ranging(Some(cnh)).unwrap();
    let output = u32s(&p.dci(DCI_OUTPUT_LIST).unwrap());
    assert_eq!(13, output.len());
    assert_eq!((0xC048 << 16) | ((cnh / 4) << 4) | 4, output[12]);
    assert_eq!(0x1FFF, u32s(&p.dci(DCI_OUTPUT_ENABLES).unwrap())[0]);
    assert_eq!(vec![size, 14], u32s(&p.dci(DCI_OUTPUT_CONFIG).unwrap()));
}

#[test]
fn start_ranging_trusts_the_firmware_frame_size() {
    let p = ranging(64);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.fw_reports(1448);
    drv.start_ranging(None).unwrap();
    assert_eq!(Some((1448, 1452)), drv.frame_size_mismatch());
    assert_eq!(1448, drv.data_read_size());
}

#[test]
fn start_ranging_without_a_resolution_is_refused_before_any_write() {
    let p = Fake::new();
    p.dci_put(DCI_ZONE_CONFIG, &[0; 8]);
    assert_eq!(
        status::INVALID_PARAM,
        code(driver(&p, Vl53l8Firmware::Cx).start_ranging(None))
    );
    assert!(p.dci(DCI_OUTPUT_LIST).is_none(), "no output list");
}

#[test]
fn start_ranging_reports_a_laser_safety_fault() {
    let p = ranging(64);
    p.fw_reports(1452);
    p.dci_put(DCI_FW_FLAGS, &[0, 0, 0, 0, 0, 0, 1, 0]);
    assert_eq!(
        status::LASER_SAFETY,
        code(driver(&p, Vl53l8Firmware::Cx).start_ranging(None))
    );
}

#[test]
fn stop_ranging_provokes_an_mcu_stop_then_undoes_it() {
    let p = Fake::new();
    p.put(0x06, 0x80);
    p.put(0x07, 0x84);
    driver(&p, Vl53l8Firmware::Cx).stop_ranging().unwrap();
    let mut want = vec![(0x7FFF, vec![0]), (0x15, vec![0x16]), (0x14, vec![0x01])];
    want.extend(undo());
    assert_eq!(want, p.writes());
    assert_eq!(
        (0x2FFC, 4),
        p.s().reads[0],
        "the auto-stop flag, not swapped"
    );
}

#[test]
fn stop_ranging_after_auto_stop_skips_the_mcu_stop() {
    let p = Fake::new();
    p.set_u32(0x2FFC, 0x4FF, false);
    driver(&p, Vl53l8Firmware::Cx).stop_ranging().unwrap();
    assert_eq!(undo(), p.writes());
}

#[test]
fn stop_ranging_gives_up_on_a_stuck_mcu_and_still_undoes() {
    let p = Fake::new();
    driver(&p, Vl53l8Firmware::Cx).stop_ranging().unwrap();
    assert_eq!(502, p.count(0x06, 1), "501 polls, then the GO2 status read");
    let w = p.writes();
    assert_eq!(undo(), w[w.len() - 5..].to_vec());
}

// ── poll mode and frame parsing ──────────────────────────────────────────────

fn go2(p: &Fake, b: [u8; 4]) {
    for (i, v) in b.into_iter().enumerate() {
        p.put(i as u32, v);
    }
}

#[test]
fn check_data_ready_fires_once_per_stream_count() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    go2(&p, [5, 0x05, 0x05, 0x10]);
    assert!(drv.check_data_ready().unwrap(), "first");
    assert!(!drv.check_data_ready().unwrap(), "same stream count");
    go2(&p, [6, 0x05, 0x05, 0x10]);
    assert!(drv.check_data_ready().unwrap(), "next");
    for g in [
        [255, 5, 5, 0x10],
        [7, 4, 5, 0x10],
        [7, 5, 4, 0x10],
        [7, 5, 5, 0],
    ] {
        go2(&p, g);
        assert!(!drv.check_data_ready().unwrap(), "{g:?}");
    }
    go2(&p, [8, 0x00, 0x42, 0x80]);
    assert_eq!(0x42, code(drv.check_data_ready()));
}

/// A raw frame (as read from register 0) for the programmed output list.
fn frame(
    drv: &Drv,
    output: &[u32],
    fill: &dyn Fn(u32, usize) -> Vec<u8>,
    footer_off: usize,
) -> Vec<u8> {
    let mut buf = vec![0u8; drv.data_read_size()];
    buf[8] = 0x12;
    buf[9] = 0x34;
    let mut i = 16;
    for &bh in output {
        let t = bh & 0xF;
        let s = ((bh >> 4) & 0xFFF) as usize;
        let idx = bh >> 16;
        let msize = if t > 1 && t < 0xD { t as usize * s } else { s };
        put_u32(&mut buf, i, bh);
        let body = fill(idx, msize);
        buf[i + 4..i + 4 + body.len()].copy_from_slice(&body);
        i += 4 + msize;
    }
    let n = buf.len();
    buf[n - footer_off] = 0x12;
    buf[n - footer_off + 1] = 0x34;
    swap_buffer(&buf)
}

fn fill_4x4(idx: u32, msize: usize) -> Vec<u8> {
    let mut b = vec![0u8; msize];
    match idx {
        0xDF44 => {
            // distance ×4: 403 → 400, -5 → 0 (negative → 0, as ST), 8 → 2
            put_u16(&mut b, 0, 400 * 4 + 3);
            put_u16(&mut b, 2, -5);
            for k in 2..16 {
                put_u16(&mut b, 2 * k, 8);
            }
        }
        0xDEC4 => {
            // sigma ×128
            for k in 0..16 {
                put_u16(&mut b, 2 * k, 256);
            }
        }
        0xDB84 => {
            // nb target detected: zone 1 has none
            b[..16].fill(1);
            b[1] = 0;
        }
        0xE084 => b[..16].fill(5),
        0x54B4 => b[8] = 0xF6, // metadata: −10 °C
        _ => {}
    }
    b
}

#[test]
fn get_ranging_data_reads_the_frame_and_scales() {
    let p = ranging(16);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.fw_reports(540);
    drv.start_ranging(None).unwrap();
    let raw = frame(&drv, &u32s(&p.dci(DCI_OUTPUT_LIST).unwrap()), &fill_4x4, 12);
    for (i, &v) in raw.iter().enumerate() {
        p.put(i as u32, v);
    }
    let f = drv.get_ranging_data(0).unwrap();
    assert_eq!((0, 540), *p.s().reads.last().unwrap());
    assert_eq!(16, f.distance_mm.len());
    assert_eq!(&[400, 0, 2], &f.distance_mm[..3]); // a negative distance → 0, as ST
    assert_eq!(2.0, f.range_sigma_mm[0]);
    assert_eq!(&[5, 255], &f.target_status[..2]);
    assert_eq!(-10, f.silicon_temp_degc);
    assert_eq!(None, f.cnh_raw);
    assert_eq!(None, f.motion);
    assert_eq!(16, f.resolution);
}

#[test]
fn a_frame_whose_footer_id_differs_is_corrupted() {
    let p = ranging(16);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.fw_reports(540);
    drv.start_ranging(None).unwrap();
    let mut buf = swap_buffer(&frame(
        &drv,
        &u32s(&p.dci(DCI_OUTPUT_LIST).unwrap()),
        &fill_4x4,
        12,
    ));
    let n = buf.len();
    buf[n - 12] ^= 0xFF;
    assert_eq!(
        status::CORRUPTED_FRAME,
        code(drv.parse_frame(0, &swap_buffer(&buf)))
    );
}

#[test]
fn the_motion_block_is_parsed_once_the_indicator_is_configured() {
    let p = ranging(16);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.fw_reports(540);
    drv.motion_indicator_init(16).unwrap();
    drv.start_ranging(None).unwrap();
    let fill = |idx: u32, msize: usize| {
        if idx != 0xD858 {
            return fill_4x4(idx, msize);
        }
        let mut b = vec![0u8; msize];
        put_u32(&mut b, 0, 7);
        put_u32(&mut b, 4, 9);
        b[8] = 1;
        b[9] = 2;
        b[10] = 16;
        for k in 0..32 {
            put_u32(&mut b, 12 + 4 * k, 100 + k as u32);
        }
        b
    };
    let raw = frame(&drv, &u32s(&p.dci(DCI_OUTPUT_LIST).unwrap()), &fill, 12);
    let m = drv
        .parse_frame(0, &raw)
        .unwrap()
        .motion
        .expect("motion parsed");
    assert_eq!(
        (7, 9, 1, 2, 16),
        (
            m.global_indicator_1,
            m.global_indicator_2,
            m.status,
            m.nb_of_detected_aggregates,
            m.nb_of_aggregates
        )
    );
    assert_eq!((100..132).collect::<Vec<u32>>(), m.motion);
}

#[test]
fn l7_frames_are_trimmed_to_the_zones_in_use() {
    let p = ranging(16);
    let mut drv = driver(&p, Vl53l8Firmware::L7cx);
    p.fw_reports(1060);
    drv.start_ranging(None).unwrap();
    let fill = |idx: u32, msize: usize| {
        let mut b = vec![0u8; msize];
        if idx == 0xDF44 {
            for k in 0..64 {
                put_u16(&mut b, 2 * k, if k < 16 { 40 } else { 999 });
            }
        }
        if idx == 0xDB84 {
            b[..64].fill(1);
        }
        b
    };
    let f = drv
        .parse_frame(
            0,
            &frame(&drv, &u32s(&p.dci(DCI_OUTPUT_LIST).unwrap()), &fill, 4),
        )
        .unwrap();
    assert_eq!(vec![10; 16], f.distance_mm);
    assert_eq!(16, f.target_status.len());
    assert_eq!(16, f.nb_spads_enabled.len());
    assert_eq!(16, f.resolution);
}

// ── power modes ──────────────────────────────────────────────────────────────

#[test]
fn power_modes() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.put(0x09, 0x07);
    assert_eq!(status::ERROR, code(drv.get_power_mode()));
    assert_eq!(
        (0x7FFF, vec![2]),
        *p.writes().last().unwrap(),
        "page restored"
    );

    p.s().writes.clear();
    p.put(0x09, 0x04);
    drv.set_power_mode(POWER_MODE_WAKEUP).unwrap(); // already awake: only the page dance
    assert_eq!(vec![(0x7FFF, vec![0]), (0x7FFF, vec![2])], p.writes());

    p.put(0x06, 0x00);
    drv.set_power_mode(POWER_MODE_DEEP_SLEEP).unwrap();
    let w = p.writes();
    assert_eq!(
        vec![
            (0x7FFF, vec![0]),
            (0x09, vec![2]),
            (0x0F, vec![0x43]),
            (0x7FFF, vec![2])
        ],
        w[w.len() - 4..].to_vec()
    );

    p.put(0x09, 0x04);
    p.put(0x06, 0x01); // never clears bit 0
    assert_eq!(
        status::TIMEOUT_ERROR,
        code(drv.set_power_mode(POWER_MODE_SLEEP))
    );
    assert_eq!(status::INVALID_PARAM, code(drv.set_power_mode(7)));
}

#[test]
fn l7cx_has_no_deep_sleep() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::L7cx);
    p.put(0x09, 0x02);
    p.put(0x0F, 0x43);
    assert_eq!(POWER_MODE_SLEEP, drv.get_power_mode().unwrap());
    assert_eq!(0, p.count(0x0F, 1));
    assert_eq!(
        status::INVALID_PARAM,
        code(drv.set_power_mode(POWER_MODE_DEEP_SLEEP))
    );
    assert_eq!(
        status::INVALID_PARAM,
        code(drv.set_detection_thresholds_auto_stop(true))
    );
}

// ── xtalk, thresholds, motion ────────────────────────────────────────────────

#[test]
fn xtalk_margin_out_of_range_is_refused() {
    // as ST: whole kcps, at most 10000 (a fraction or a negative is not a u32)
    for bad in [10001, u32::MAX] {
        let p = Fake::new();
        assert_eq!(
            status::INVALID_PARAM,
            code(driver(&p, Vl53l8Firmware::Cx).set_xtalk_margin(bad))
        );
        assert!(p.dci(0xAD94).is_none(), "nothing written for {bad}");
    }
}

#[test]
fn xtalk_margin_roundtrips_in_whole_kcps() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    drv.set_xtalk_margin(10000).unwrap();
    assert_eq!(p.dci(0xAD94).unwrap()[..4], (10000u32 * 2048).to_le_bytes());
    assert_eq!(drv.get_xtalk_margin().unwrap(), 10000);
}

fn xtalk_reply() -> Vec<u8> {
    (0..1024).map(|i| i as u8).collect()
}

#[test]
fn caldata_xtalk_reads_the_buffer_and_restores_the_resolution() {
    let p = ranging(16);
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.s().xtalk_reply = Some(xtalk_reply());
    let blob = drv.get_caldata_xtalk().unwrap();
    assert!(
        p.writes()
            .iter()
            .any(|w| w.0 == 0x2FB8 && w.1 == tables::get_xtalk_cmd()),
        "GET_XTALK sent"
    );
    let n = XTALK_BUFFER_SIZE;
    assert_eq!(n, blob.len());
    assert_eq!(&xtalk_reply()[8..n], &blob[..n - 8]);
    assert_eq!(&[0, 0, 0, 0x0F, 0, 0x01, 0x03, 0x04], &blob[n - 8..]);
    assert_eq!(blob, drv.xtalk_data());
    assert_eq!(16, drv.get_resolution().unwrap());
    assert_eq!(status::INVALID_PARAM, code(drv.set_caldata_xtalk(&[0; 10])));
}

#[test]
fn detection_thresholds() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    let one = DetectionThreshold {
        measurement: 1,
        ..Default::default()
    };
    invalid_argument(drv.set_detection_thresholds(&[one; 65]));
    let overflow = DetectionThreshold {
        high_thresh: i32::MAX / 4 + 1,
        measurement: DIST_MM,
        ..Default::default()
    };
    invalid_argument(drv.set_detection_thresholds(&[overflow]));
    assert!(p.dci(DCI_THRESH_START).is_none(), "nothing written");

    drv.set_detection_thresholds(&[]).unwrap();
    assert_eq!(Some(vec![5u8; 8]), p.dci(DCI_THRESH_VALID));
    assert_eq!(Some(vec![0u8; 64 * 12]), p.dci(DCI_THRESH_START));

    let t = DetectionThreshold {
        low_thresh: 100,
        high_thresh: 300,
        measurement: DIST_MM,
        type_: 1,
        zone_num: 7,
        operation: 0,
    };
    drv.set_detection_thresholds(&[t]).unwrap();
    assert_eq!(t, drv.get_detection_thresholds().unwrap()[0]);
}

#[test]
fn motion_indicator_window() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    let mut cfg = drv.motion_indicator_init(16).unwrap();
    let map: Vec<i8> = (0..64).map(|i| if i < 16 { i as i8 } else { -1 }).collect();
    assert_eq!(map, cfg.map_id.to_vec());
    assert_eq!(
        status::INVALID_PARAM,
        code(drv.motion_indicator_set_resolution(&mut cfg, 32))
    );
    for (lo, hi) in [(400, 1901), (300, 1000), (3000, 4001)] {
        assert_eq!(
            status::INVALID_PARAM,
            code(drv.motion_indicator_set_distance_motion(&mut cfg, lo, hi))
        );
    }
    // Java int arithmetic: the window width wraps, never panics.
    assert_eq!(
        status::INVALID_PARAM,
        code(drv.motion_indicator_set_distance_motion(&mut cfg, i32::MIN, i32::MAX))
    );
    drv.motion_indicator_set_distance_motion(&mut cfg, 1000, 2500)
        .unwrap();
    assert_eq!(
        ((1000.0 / 37.5348 - 4.0) * 2048.5) as i32,
        cfg.ref_bin_offset
    );
    assert_eq!(
        ((150.0 + 30.02784) / 15.01392 + 0.5) as i32 as u8,
        cfg.feature_length
    );
    assert_eq!(Some(cfg.pack()), p.dci(DCI_MOTION_CFG));
}

// ── xtalk calibration ────────────────────────────────────────────────────────

#[test]
fn calibration_arguments_are_checked_first() {
    for (r, n, d) in [
        (0, 4, 600),
        (100, 4, 600),
        (16, 0, 600),
        (16, 17, 600),
        (16, 4, 599),
        (16, 4, 3001),
    ] {
        let p = Fake::new();
        assert_eq!(
            status::INVALID_PARAM,
            code(driver(&p, Vl53l8Firmware::Cx).calibrate_xtalk(r, n, d)),
            "{r} {n} {d}"
        );
        assert!(p.writes().is_empty());
    }
}

fn calibration() -> Arc<Fake> {
    let p = ranging(16);
    p.dci_put(DCI_FREQ_HZ, &[0, 15, 0, 0]);
    let mut int_time = vec![0u8; 20];
    int_time[..4].copy_from_slice(&[0x20, 0x4E, 0, 0]); // 20000 µs
    p.dci_put(DCI_INT_TIME, &int_time);
    p.dci_put(DCI_RANGING_MODE, &[0, 3, 0, 2, 0, 0, 0, 0]); // autonomous
    p.dci_put(DCI_TARGET_ORDER, &[TARGET_ORDER_CLOSEST, 0, 0, 0]);
    p.s().xtalk_reply = Some(xtalk_reply());
    p
}

#[test]
fn calibration_runs_the_firmware_table_and_restores_the_settings() {
    for fw in [Vl53l8Firmware::Cx, Vl53l8Firmware::Ch, Vl53l8Firmware::L7cx] {
        let p = calibration();
        let mut drv = driver(&p, fw);
        drv.calibrate_xtalk(16, 4, 600).unwrap();
        let table: &[u8] = match fw {
            Vl53l8Firmware::Cx => tables::calibrate_xtalk_cx(),
            Vl53l8Firmware::L7cx => tables::calibrate_xtalk_l7cx(),
            _ => tables::calibrate_xtalk_lmz(),
        };
        let addrs = p.addrs();
        let at = |a: u16| addrs.iter().position(|&x| x == a).unwrap();
        assert!(
            p.writes().iter().any(|w| w.0 == 0x2C28 && w.1 == table),
            "{fw:?} table"
        );
        assert!(at(0x2C28) < at(UI_CMD_END - 3), "{fw:?} table before start");
        assert!(
            at(UI_CMD_END - 3) < at(0x2FB8),
            "{fw:?} start before readout"
        );
        let cal = p.dci(DCI_CAL_CFG).unwrap();
        assert_eq!(
            (2400, 256, 4),
            (
                cal[0] as u32 | (cal[1] as u32) << 8,
                cal[2] as u32 | (cal[3] as u32) << 8,
                cal[4] as u32
            ),
            "{fw:?}"
        );
        let got = u32s(&p.dci(DCI_OUTPUT_LIST).unwrap());
        assert_eq!(tables::xtalk_cal_output().len(), got.len(), "{fw:?}");
        for (k, (&want, &got)) in tables::xtalk_cal_output().iter().zip(&got).enumerate() {
            assert_eq!(want & !0xFFF0, got & !0xFFF0, "{fw:?} output {k}");
        }
        assert_eq!(
            vec![1204, 18],
            u32s(&p.dci(DCI_OUTPUT_CONFIG).unwrap()),
            "{fw:?}"
        );
        assert!(!drv.xtalk_calibration_failed(), "{fw:?} not failed");
        let n = XTALK_BUFFER_SIZE;
        assert_eq!(&xtalk_reply()[8..n], &drv.xtalk_data()[..n - 8], "{fw:?}");
        assert!(at(0x2FB8) < at(0x2C34), "{fw:?} readout before restore");
        assert_eq!(16, drv.get_resolution().unwrap(), "{fw:?}");
        assert_eq!(15, drv.get_ranging_frequency_hz().unwrap(), "{fw:?}");
        assert_eq!(20, drv.get_integration_time_ms().unwrap(), "{fw:?}");
        assert_eq!(
            RANGING_MODE_AUTONOMOUS,
            drv.get_ranging_mode().unwrap(),
            "{fw:?}"
        );
        assert_eq!(
            TARGET_ORDER_CLOSEST,
            drv.get_target_order().unwrap(),
            "{fw:?}"
        );
    }
}

#[test]
fn coverglass_too_good_is_flagged_not_thrown() {
    let p = calibration();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    go2(&p, [0x00, 0x00, 0x7F, 0x80]);
    drv.calibrate_xtalk(16, 4, 600).unwrap();
    assert!(drv.xtalk_calibration_failed(), "flagged");
}

#[test]
fn calibration_waits_then_times_out() {
    let p = calibration();
    p.put(0, status::ERROR); // still calibrating
    let e = driver(&p, Vl53l8Firmware::Cx)
        .calibrate_xtalk(16, 4, 600)
        .unwrap_err();
    assert_eq!(Some(status::ERROR), e.status_code());
    assert!(e.to_string().contains("xtalk calibration"), "{e}");
    assert_eq!(401, p.count(0x0, 4), "400 waits of 50 ms, 401 reads");
}

#[test]
fn sharpener_reads_back_rounded_to_nearest() {
    let p = Fake::new();
    let mut drv = driver(&p, Vl53l8Firmware::Cx);
    p.dci_put(0xAED8, &[0; 16]);
    drv.set_sharpener_percent(25).unwrap();
    assert_eq!(25, drv.get_sharpener_percent().unwrap());
    assert_eq!(status::INVALID_PARAM, code(drv.set_sharpener_percent(100)));
    invalid_argument(drv.set_sharpener_percent(-1));
    invalid_argument(drv.set_ranging_frequency_hz(256));
}

// ── Rust-side additions: error mapping, thread safety, CNH setup ─────────────

#[test]
fn errors_convert_into_depz_error_with_the_status_in_the_text() {
    let e: DepzError = Vl53l8UldError::status(status::TIMEOUT_ERROR, "sensor boot").into();
    match e {
        DepzError::Sensor(m) => assert_eq!("VL53L8CX TIMEOUT (code 1) sensor boot", m),
        other => panic!("{other:?}"),
    }
    let e: DepzError = Vl53l8UldError::status(status::LASER_SAFETY, "").into();
    assert_eq!("VL53L8CX LASER_SAFETY (code 3)", e.to_string());
    let e: DepzError = Vl53l8UldError::status(0x41, "GO2 error status").into();
    assert_eq!("VL53L8CX 0x41 (code 65) GO2 error status", e.to_string());
    assert!(matches!(
        DepzError::from(Vl53l8UldError::InvalidArgument("x".into())),
        DepzError::InvalidArgument(_)
    ));
    let lost: DepzError = Vl53l8UldError::from(DepzError::DeviceLost("gone".into())).into();
    assert!(matches!(lost, DepzError::DeviceLost(_)));
}

#[test]
fn a_short_platform_read_is_an_error_not_a_panic() {
    struct Short;
    impl Vl53l8Platform for Short {
        fn read_multi(&self, _addr: u16, _size: usize) -> Result<Vec<u8>> {
            Ok(vec![])
        }
        fn write_multi(&self, _addr: u16, _data: &[u8]) -> Result<()> {
            Ok(())
        }
    }
    let mut drv = Vl53l8Driver::new(Short, Vl53l8Firmware::Cx);
    assert!(matches!(
        drv.check_data_ready(),
        Err(Vl53l8UldError::Platform(DepzError::Protocol(_)))
    ));
}

#[test]
fn the_driver_is_send() {
    fn send<T: Send>() {}
    send::<Vl53l8Driver<Arc<Fake>>>();
    send::<Vl53l8Driver<Box<dyn Vl53l8Platform>>>();
}

#[test]
fn cnh_setup_builds_the_motion_configuration() {
    let mut cnh = Vl53l8CnhSetup::new();
    assert!(matches!(
        cnh.required_memory(),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        cnh.init_config(0, 256, 1),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        cnh.init_config(1048576, 1, 1),
        Err(DepzError::InvalidArgument(_))
    ));
    cnh.init_config(8, 24, 4).unwrap();
    assert_eq!(8 * 2048, cnh.ref_bin_offset());
    assert_eq!(0x3F, cnh.cnh_cfg());
    assert!(matches!(
        cnh.create_agg_map(32, 0, 0, 1, 1, 4, 4),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        cnh.create_agg_map(16, 0, 0, 1, 1, 5, 4),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        cnh.create_agg_map(16, -1, 0, 1, 1, 4, 4),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        cnh.create_agg_map(64, 0, 0, i32::MAX, 1, 2, 1),
        Err(DepzError::InvalidArgument(_))
    ));
    cnh.create_agg_map(64, 0, 0, 2, 2, 4, 4).unwrap();
    assert_eq!(16, cnh.nb_of_aggregates());
    let map = cnh.map_id();
    assert_eq!((0, 0, 1, 4, 15), (map[0], map[1], map[2], map[16], map[63]));
    // 2·4 + 16·24·4 + 384 + 16·4 + 16 = 2008, + the 20-byte header.
    assert_eq!(2028, cnh.required_memory().unwrap());
    let packed = cnh.pack();
    assert_eq!(156, packed.len());
    assert_eq!(&(8 * 2048i32).to_le_bytes(), &packed[0..4]);
    assert_eq!(
        (4, 24, 16, 1, 0x3F, 1),
        (packed[18], packed[19], packed[20], packed[21], packed[25], packed[26])
    );
    assert_eq!(15, packed[28 + 63]);
    let d = cnh.decode_config();
    assert_eq!((16, 24), (d.nb_of_aggregates, d.feature_length));
    cnh.init_config(0, 255, 255).unwrap();
    cnh.create_agg_map(64, 0, 0, 1, 1, 8, 8).unwrap();
    assert!(
        matches!(cnh.required_memory(), Err(DepzError::InvalidArgument(_))),
        "over CNH_MAX_DATA_BYTES"
    );
}
