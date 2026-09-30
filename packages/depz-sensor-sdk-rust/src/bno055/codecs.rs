//! BNO055 register-bridge wire codecs, protocol v0.10
//! (contracts/13_SENSOR_BNO055.md §2–§3).
//!
//! Like the VL53L4 bridge of contract 10 the MCU is a thin register bridge,
//! but the BNO055 register space is 8-bit: `addr` and `len` are one byte each
//! on the wire, and the info report carries the sensor identity registers.
//! Transfer limit: `len` 1..=[`XFER_MAX`], `addr + len ≤ 0x100` — the bridge
//! answers anything else with ERR_INVALID_PARAM; these encoders pass values
//! through unchecked, like the reference SDKs.

use crate::protocol::common::CodecError;

/// BNO055 host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Bno055Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    /// Pulses nRESET and answers after the chip-ID handshake (~0.5 s; allow
    /// ≥ 1.5 s). Stops any stream.
    Reset = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}

/// BNO055 device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Bno055Rpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100.
pub const XFER_MAX: usize = 128;
/// `BNO_START_STREAM` trigger: read every `period_ms` (1..60000) — the only
/// data trigger on sensor SW rev 03.11 (no data-ready interrupt there).
pub const TRIGGER_TIMER: u8 = 0;
/// `BNO_START_STREAM` trigger: read on each INT rising edge; `period_ms` is a
/// missed-edge watchdog, 0 disables it. For motion interrupts.
pub const TRIGGER_INT: u8 = 1;
/// `RPT_BNO_INFO` payload size.
pub const INFO_SIZE: usize = 38;
/// Sensor 7-bit I2C address (SA0 held low).
pub const I2C_ADDR: u8 = 0x28;

/// Identity registers 0x00..0x03 of a healthy BNO055.
pub const EXPECTED_CHIP_ID: u8 = 0xA0;
pub const EXPECTED_ACC_ID: u8 = 0xFB;
pub const EXPECTED_MAG_ID: u8 = 0x32;
pub const EXPECTED_GYR_ID: u8 = 0x0F;

/// `BNO_READ_REG` payload: `addr u8, len u8`.
pub fn pack_read_reg(addr: u8, len: u8) -> [u8; 2] {
    [addr, len]
}

/// `BNO_WRITE_REG` payload: `addr u8, data[1..128]`.
pub fn pack_write_reg(addr: u8, data: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(1 + data.len());
    out.push(addr);
    out.extend_from_slice(data);
    out
}

/// `BNO_RESET` payload (empty).
pub fn pack_reset() -> [u8; 0] {
    []
}

/// `BNO_STOP_STREAM` payload (empty).
pub fn pack_stop_stream() -> [u8; 0] {
    []
}

/// `BNO_GET_INFO` payload (empty).
pub fn pack_get_info() -> [u8; 0] {
    []
}

/// `BNO_START_STREAM` payload: `trigger u8, addr u8, len u8, period_ms u16 LE`
/// — 5 bytes. Replaces any running stream.
pub fn pack_start_stream(trigger: u8, addr: u8, len: u8, period_ms: u16) -> [u8; 5] {
    let p = period_ms.to_le_bytes();
    [trigger, addr, len, p[0], p[1]]
}

/// A decoded `RPT_BNO_REG_DATA` report: the reply to `BNO_READ_REG`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RegData {
    /// Echoed `BNO_READ_REG` opcode (0x32).
    pub cmd: u8,
    /// MCU uptime at I2C-read completion.
    pub timestamp_us: u64,
    /// Raw register bytes (little-endian sensor contents, passed through).
    pub data: Vec<u8>,
}

impl RegData {
    pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError> {
        if payload.len() < 9 {
            return Err(CodecError("RPT_BNO_REG_DATA must be at least 9 bytes"));
        }
        Ok(RegData {
            cmd: payload[0],
            timestamp_us: u64::from_le_bytes(payload[1..9].try_into().unwrap()),
            data: payload[9..].to_vec(),
        })
    }
}

/// A decoded `RPT_BNO_REG_STREAM` report — one streamed register block.
/// `addr` / `len` echo the stream configuration, so each report is
/// self-describing ([`crate::bno055::decode_block`] takes them as is).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct StreamData {
    /// MCU uptime at the trigger (timer expiry or INT edge), not the I2C
    /// completion. Dropped samples show as jumps.
    pub timestamp_us: u64,
    pub addr: u8,
    pub len: u8,
    pub data: Vec<u8>,
}

impl StreamData {
    pub fn unpack(payload: &[u8]) -> Result<StreamData, CodecError> {
        if payload.len() < 10 {
            return Err(CodecError("RPT_BNO_REG_STREAM must be at least 10 bytes"));
        }
        let len = payload[9];
        if payload.len() < 10 + len as usize {
            return Err(CodecError("RPT_BNO_REG_STREAM shorter than its len field"));
        }
        Ok(StreamData {
            timestamp_us: u64::from_le_bytes(payload[0..8].try_into().unwrap()),
            addr: payload[8],
            len,
            data: payload[10..10 + len as usize].to_vec(),
        })
    }
}

/// A decoded `RPT_BNO_INFO` report (38 bytes) — sensor identity (registers
/// 0x00..0x06) plus bridge diagnostics. Counters are free-running and wrap
/// silently: watch increments. A rising `sensor_resets` means the bridge
/// pulsed nRESET to recover the bus — the sensor is back in CONFIG and the
/// host must re-apply its configuration.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Bno055Info {
    /// 7-bit sensor address in use (0x28).
    pub i2c_addr: u8,
    pub chip_id: u8,
    pub acc_id: u8,
    pub mag_id: u8,
    pub gyr_id: u8,
    /// Sensor firmware, BCD: 0x0311 = 03.11.
    pub sw_rev: u16,
    pub bl_rev: u8,
    /// 1 = chip-ID handshake passed.
    pub initialized: u8,
    /// Current INT pin level.
    pub int_level: u8,
    /// INT rising edges (counted only while an INT stream is armed).
    pub int_edges: u32,
    /// Streamed block read time since the last START_STREAM.
    pub read_min_us: u16,
    pub read_max_us: u16,
    pub read_avg_us: u16,
    /// Packets refused by a full USB TX ring.
    pub tx_dropped: u32,
    pub i2c_errors: u32,
    /// Stream slots dropped because the bus was still busy (since START_STREAM).
    pub slots_skipped: u32,
    pub bus_recoveries: u16,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    /// ([`crate::vl53l4::i2c_error_name`]).
    pub last_i2c_error: u8,
    /// nRESET recoveries — the sensor came back in CONFIG each time.
    pub sensor_resets: u8,
    /// Longest main-loop iteration since START_STREAM.
    pub loop_max_us: u16,
}

impl Bno055Info {
    pub fn unpack(payload: &[u8]) -> Result<Bno055Info, CodecError> {
        if payload.len() < INFO_SIZE {
            return Err(CodecError("RPT_BNO_INFO must be at least 38 bytes"));
        }
        let u16_at = |i: usize| u16::from_le_bytes([payload[i], payload[i + 1]]);
        let u32_at = |i: usize| u32::from_le_bytes(payload[i..i + 4].try_into().unwrap());
        Ok(Bno055Info {
            i2c_addr: payload[0],
            chip_id: payload[1],
            acc_id: payload[2],
            mag_id: payload[3],
            gyr_id: payload[4],
            sw_rev: u16_at(5),
            bl_rev: payload[7],
            initialized: payload[8],
            int_level: payload[9],
            int_edges: u32_at(10),
            read_min_us: u16_at(14),
            read_max_us: u16_at(16),
            read_avg_us: u16_at(18),
            tx_dropped: u32_at(20),
            i2c_errors: u32_at(24),
            slots_skipped: u32_at(28),
            bus_recoveries: u16_at(32),
            last_i2c_error: payload[34],
            sensor_resets: payload[35],
            loop_max_us: u16_at(36),
        })
    }

    /// All four identity registers hold the healthy values.
    pub fn ids_ok(&self) -> bool {
        (self.chip_id, self.acc_id, self.mag_id, self.gyr_id)
            == (EXPECTED_CHIP_ID, EXPECTED_ACC_ID, EXPECTED_MAG_ID, EXPECTED_GYR_ID)
    }

    /// Sensor firmware revision as Bosch writes it: 0x0311 → `"03.11"`.
    pub fn sw_rev_text(&self) -> String {
        format!("{:02X}.{:02X}", self.sw_rev >> 8, self.sw_rev & 0xFF)
    }
}
