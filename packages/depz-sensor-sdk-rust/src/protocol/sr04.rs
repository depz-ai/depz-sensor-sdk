//! SR04 wire codecs (contracts/03_SENSOR_SR04.md).

use super::common::CodecError;

/// SR04 host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Sr04Cmd {
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
}

/// SR04 device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Sr04Rpt {
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
}

/// `echo_time_us` sentinel: no echo received.
pub const ECHO_TIMEOUT: u16 = 0xFFFF;

pub const SAMPLE_PERIOD_DEFAULT_US: u32 = 50_000;
pub const ECHO_DECAY_DEFAULT_US: u16 = 5_000;
pub const ECHO_DECAY_MIN_US: u16 = 4_000;
pub const ECHO_DECAY_MAX_US: u16 = 65_000;

/// A decoded `RPT_DATA` sample.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Sr04Data {
    /// 0x36 single shot (host or SYNC_IN), 0x37 loop sample (ERRATA E3).
    pub source_cmd: u8,
    pub timestamp_us: u64,
    pub echo_time_us: u16,
}

impl Sr04Data {
    pub fn unpack(payload: &[u8]) -> Result<Sr04Data, CodecError> {
        if payload.len() != 11 {
            return Err(CodecError("SR04 data must be 11 bytes"));
        }
        Ok(Sr04Data {
            source_cmd: payload[0],
            timestamp_us: u64::from_le_bytes(payload[1..9].try_into().unwrap()),
            echo_time_us: u16::from_le_bytes(payload[9..11].try_into().unwrap()),
        })
    }

    /// `true` when the sample is the timeout sentinel (no echo).
    pub fn is_timeout(&self) -> bool {
        self.echo_time_us == ECHO_TIMEOUT
    }
}

pub fn pack_sample_period(period_us: u32) -> [u8; 4] {
    period_us.to_le_bytes()
}

pub fn unpack_sample_period(payload: &[u8]) -> Result<u32, CodecError> {
    if payload.len() != 4 {
        return Err(CodecError("sample period must be 4 bytes"));
    }
    Ok(u32::from_le_bytes(payload.try_into().unwrap()))
}

pub fn pack_echo_decay(decay_us: u16) -> [u8; 2] {
    decay_us.to_le_bytes()
}

pub fn unpack_echo_decay(payload: &[u8]) -> Result<u16, CodecError> {
    if payload.len() != 2 {
        return Err(CodecError("echo decay must be 2 bytes"));
    }
    Ok(u16::from_le_bytes(payload.try_into().unwrap()))
}

/// Round-trip echo time → distance in mm; `None` for the timeout sentinel.
///
/// Default speed of sound 343 m/s; with `air_temp_c` uses `c = 331.3 + 0.606·T`.
pub fn distance_mm_from_echo(echo_time_us: u16, air_temp_c: Option<f64>) -> Option<f64> {
    if echo_time_us == ECHO_TIMEOUT {
        return None;
    }
    let c_m_s = match air_temp_c {
        None => 343.0,
        Some(t) => 331.3 + 0.606 * t,
    };
    Some(echo_time_us as f64 * c_m_s / 2000.0)
}
