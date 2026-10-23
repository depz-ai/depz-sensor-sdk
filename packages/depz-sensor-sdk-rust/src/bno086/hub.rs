//! SH-2 hub protocol (contract 05 §6): the bridge codecs, the control-response decoders, the request
//! parameters, the FRS sessions and the control-cargo walk the device layer needs on top of [`sh2`].
//! Ported from the reference `sh2.py`.
//!
//! [`sh2`]: super::sh2

use super::sh2;
use crate::protocol::common::CodecError;

// ── the bridge (contract 05 §2) ─────────────────────────────────────────────

/// Bridge commands of the BNO086 board: the MCU only shuttles raw SHTP frames.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Bno086Cmd {
    /// Hardware reset through nRST; RPT_STATUS OK.
    SensorReset = 0x32,
    /// 1 ms WAKE (PS0) pulse; RPT_STATUS OK.
    SensorWakeUp = 0x33,
    /// Payload = one raw SHTP frame; RPT_STATUS OK or ERR_BUSY (both TX slots full).
    SendShtpPacket = 0x34,
}

/// RPT_DATA: cmd u8 (always 0x00, ERRATA E2), timestamp_us u64, raw SHTP frame.
pub const RPT_DATA: u8 = 0x91;

/// Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY, to back off
/// at least this long, ms.
pub const BUSY_BACKOFF_MS: u64 = 200;

/// One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Bno086Data {
    pub cmd: u8,
    pub timestamp_us: u64,
    pub shtp: Vec<u8>,
}

impl Bno086Data {
    pub fn unpack(p: &[u8]) -> Result<Bno086Data, CodecError> {
        if p.len() < 9 {
            return Err(CodecError("RPT_DATA needs 9 bytes"));
        }
        Ok(Bno086Data { cmd: p[0], timestamp_us: u64::from_le_bytes(p[1..9].try_into().unwrap()), shtp: p[9..].to_vec() })
    }
}

// ── control reports and commands ────────────────────────────────────────────

/// Host → hub on the control channel; answered by Flush Completed (0xEF) on the input channel.
pub const FORCE_SENSOR_FLUSH: u8 = 0xF0;

/// Inbound on the executable channel: the hub finished a reset.
pub const EXECUTABLE_RESET_COMPLETE: u8 = 0x01;

pub const CMD_ERRORS: u8 = 0x01;
pub const CMD_COUNTER: u8 = 0x02;
pub const CMD_TARE: u8 = 0x03;
pub const CMD_INITIALIZE: u8 = 0x04;
pub const CMD_SAVE_DCD: u8 = 0x06;
pub const CMD_ME_CALIBRATE: u8 = 0x07;
pub const CMD_PERIODIC_DCD_CONFIG: u8 = 0x09;
pub const CMD_GET_OSCILLATOR_TYPE: u8 = 0x0A;
pub const CMD_CLEAR_DCD_AND_RESET: u8 = 0x0B;
/// BNO086 only; no response.
pub const CMD_INTERACTIVE_CALIBRATION: u8 = 0x0E;

/// Get Oscillator Type R0: internal.
pub const OSC_INTERNAL: u8 = 0;
/// Get Oscillator Type R0: external crystal.
pub const OSC_EXT_CRYSTAL: u8 = 1;
/// Get Oscillator Type R0: external clock.
pub const OSC_EXT_CLOCK: u8 = 2;

/// The end of the error queue (the `source` of an error record, SH-2 §6.4.1).
pub const ERROR_SOURCE_NO_MORE_ERRORS: u8 = 255;

/// Tare axes, a bitmap.
pub const TARE_X: u8 = 1;
pub const TARE_Y: u8 = 2;
pub const TARE_Z: u8 = 4;
pub const TARE_ALL: u8 = 7;

/// The rotation vector a tare refers to (Tare Now P2).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum TareBasis {
    RotationVector = 0,
    GameRotationVector = 1,
    GeomagneticRotationVector = 2,
    GyroIntegratedRv = 3,
    ArvrStabilizedRv = 4,
    ArvrStabilizedGameRv = 5,
}

/// P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the device's
/// motion. `Unknown` is the hub's initial state.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum MotionIntent {
    Unknown = 0,
    StationaryNoVibration = 1,
    StationaryWithVibration = 2,
    InMotion = 3,
    InMotionAccelerating = 4,
}

// ── FRS ─────────────────────────────────────────────────────────────────────

pub const FRS_STATIC_CALIBRATION_AGM: u16 = 0x7979;
pub const FRS_NOMINAL_CALIBRATION: u16 = 0x4D4D;
pub const FRS_DYNAMIC_CALIBRATION: u16 = 0x1F1F;
pub const FRS_ME_POWER_MGMT: u16 = 0xD3E2;
/// Mounting quaternion, 4 × Q30 words.
pub const FRS_SYSTEM_ORIENTATION: u16 = 0x2D3E;
pub const FRS_ACCEL_ORIENTATION: u16 = 0x2D41;
pub const FRS_GYROSCOPE_ORIENTATION: u16 = 0x2D46;
pub const FRS_MAGNETOMETER_ORIENTATION: u16 = 0x2D4C;
pub const FRS_ARVR_STABILIZATION_RV: u16 = 0x3E2D;
pub const FRS_ARVR_STABILIZATION_GRV: u16 = 0x3E2E;
/// One 32-bit word; empty unless programmed.
pub const FRS_SERIAL_NUMBER: u16 = 0x4B4B;
/// Free for the user, up to 64 words.
pub const FRS_USER_RECORD: u16 = 0x74B4;
pub const FRS_SIG_MOTION_DETECT_CONFIG: u16 = 0xC274;
pub const FRS_SHAKE_DETECT_CONFIG: u16 = 0x7D7D;
pub const FRS_STABILITY_DETECTOR_CONFIG: u16 = 0xED85;
pub const FRS_ACTIVITY_TRACKER_CONFIG: u16 = 0xED88;

/// Sensor id → its metadata FRS record.
pub const METADATA_RECORDS: [(u8, u16); 24] = [
    (0x14, 0xE301), (0x01, 0xE302), (0x04, 0xE303), (0x06, 0xE304), (0x15, 0xE305), (0x02, 0xE306),
    (0x07, 0xE307), (0x16, 0xE308), (0x03, 0xE309), (0x0F, 0xE30A), (0x05, 0xE30B), (0x08, 0xE30C),
    (0x09, 0xE30D), (0x10, 0xE313), (0x18, 0xE314), (0x11, 0xE315), (0x12, 0xE316), (0x13, 0xE317),
    (0x19, 0xE318), (0x1E, 0xE31C), (0x28, 0xE322), (0x29, 0xE323), (0x2A, 0xE324), (0x2B, 0xE325),
];

// FRS Read Response status.
pub const FRS_NO_ERROR: u8 = 0;
pub const FRS_UNRECOGNIZED_FRS_TYPE: u8 = 1;
pub const FRS_BUSY: u8 = 2;
pub const FRS_READ_COMPLETED: u8 = 3;
pub const FRS_OFFSET_OUT_OF_RANGE: u8 = 4;
pub const FRS_RECORD_EMPTY: u8 = 5;
pub const FRS_BLOCK_COMPLETED: u8 = 6;
pub const FRS_BLOCK_AND_READ_COMPLETED: u8 = 7;
pub const FRS_DEVICE_ERROR: u8 = 8;

// FRS Write Response status.
pub const FRS_WRITE_WORDS_RECEIVED: u8 = 0;
pub const FRS_WRITE_UNRECOGNIZED_FRS_TYPE: u8 = 1;
pub const FRS_WRITE_BUSY: u8 = 2;
pub const FRS_WRITE_COMPLETED: u8 = 3;
pub const FRS_WRITE_MODE_READY: u8 = 4;
pub const FRS_WRITE_FAILED: u8 = 5;
pub const FRS_WRITE_NOT_IN_WRITE_MODE: u8 = 6;
pub const FRS_WRITE_INVALID_LENGTH: u8 = 7;
pub const FRS_WRITE_RECORD_VALID: u8 = 8;
pub const FRS_WRITE_RECORD_INVALID: u8 = 9;

/// A SH-2 level failure: a response that is not the one asked for, a non-zero command status, an
/// FRS error.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Sh2HubError(pub String);

impl std::fmt::Display for Sh2HubError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for Sh2HubError {}

fn need(p: &[u8], n: usize, what: &str) -> Result<(), Sh2HubError> {
    if p.len() < n {
        return Err(Sh2HubError(format!("{what}: need {n} bytes, got {}", p.len())));
    }
    Ok(())
}

fn expect_id(p: &[u8], id: u8, what: &str) -> Result<(), Sh2HubError> {
    if p[0] != id {
        return Err(Sh2HubError(format!("not a {what} response: 0x{:02X}", p[0])));
    }
    Ok(())
}

fn u16le(b: &[u8], o: usize) -> u16 {
    u16::from_le_bytes([b[o], b[o + 1]])
}

fn u32le(b: &[u8], o: usize) -> u32 {
    u32::from_le_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

/// Get Feature Response (0xFC), 17 bytes: the rates in effect.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FeatureResponse {
    pub sensor_id: u8,
    pub flags: u8,
    pub sensitivity: u16,
    pub interval_us: u32,
    pub batch_us: u32,
    pub cfg_word: u32,
}

impl FeatureResponse {
    pub fn unpack(p: &[u8]) -> Result<FeatureResponse, Sh2HubError> {
        need(p, 17, "Get Feature Response")?;
        expect_id(p, sh2::REPORT_GET_FEATURE_RESPONSE, "get-feature")?;
        Ok(FeatureResponse {
            sensor_id: p[1],
            flags: p[2],
            sensitivity: u16le(p, 3),
            interval_us: u32le(p, 5),
            batch_us: u32le(p, 9),
            cfg_word: u32le(p, 13),
        })
    }
}

/// Product ID Response (0xF8), 16 bytes, one per software component (the SH-2 application first);
/// the reset cause (SH-2 §6.4.5.2) is set on the first only.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ProductId {
    pub reset_cause: u8,
    pub sw_version_major: u8,
    pub sw_version_minor: u8,
    pub sw_part_number: u32,
    pub sw_build_number: u32,
    pub sw_version_patch: u16,
}

impl ProductId {
    /// "major.minor.patch".
    pub fn version(&self) -> String {
        format!("{}.{}.{}", self.sw_version_major, self.sw_version_minor, self.sw_version_patch)
    }

    pub fn unpack(p: &[u8]) -> Result<ProductId, Sh2HubError> {
        need(p, 14, "Product ID Response")?;
        expect_id(p, sh2::REPORT_PRODUCT_ID_RESPONSE, "product-id")?;
        Ok(ProductId {
            reset_cause: p[1],
            sw_version_major: p[2],
            sw_version_minor: p[3],
            sw_part_number: u32le(p, 4),
            sw_build_number: u32le(p, 8),
            sw_version_patch: u16le(p, 12),
        })
    }
}

/// Command Response (0xF1), 16 bytes. `command_seq` echoes the request (correlate on it plus
/// `command`); `response_seq` counts responses to one request; R0 is the status of most commands
/// (0 = success).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CommandResponse {
    pub seq: u8,
    pub command: u8,
    pub command_seq: u8,
    pub response_seq: u8,
    pub r: [u8; 11],
}

impl CommandResponse {
    pub fn status(&self) -> u8 {
        self.r[0]
    }

    pub fn unpack(p: &[u8]) -> Result<CommandResponse, Sh2HubError> {
        need(p, 16, "Command Response")?;
        expect_id(p, sh2::REPORT_COMMAND_RESPONSE, "command")?;
        Ok(CommandResponse { seq: p[1], command: p[2], command_seq: p[3], response_seq: p[4], r: p[5..16].try_into().unwrap() })
    }
}

/// FRS Read Response (0xF3), 16 bytes, up to two data words.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FrsReadResponse {
    pub status: u8,
    pub data_length: u8,
    pub offset_words: u16,
    pub data0: u32,
    pub data1: u32,
    pub frs_type: u16,
}

impl FrsReadResponse {
    pub fn unpack(p: &[u8]) -> Result<FrsReadResponse, Sh2HubError> {
        need(p, 16, "FRS Read Response")?;
        expect_id(p, sh2::REPORT_FRS_READ_RESPONSE, "FRS read")?;
        Ok(FrsReadResponse {
            status: p[1] & 0x0F,
            data_length: p[1] >> 4,
            offset_words: u16le(p, 2),
            data0: u32le(p, 4),
            data1: u32le(p, 8),
            frs_type: u16le(p, 12),
        })
    }
}

/// FRS Write Response (0xF5), 4 bytes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FrsWriteResponse {
    pub status: u8,
    pub offset_words: u16,
}

impl FrsWriteResponse {
    pub fn unpack(p: &[u8]) -> Result<FrsWriteResponse, Sh2HubError> {
        need(p, 4, "FRS Write Response")?;
        expect_id(p, sh2::REPORT_FRS_WRITE_RESPONSE, "FRS write")?;
        Ok(FrsWriteResponse { status: p[1], offset_words: u16le(p, 2) })
    }
}

/// One error-queue entry (command 0x01 response, R0..R5).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ErrorRecord {
    pub severity: u8,
    pub seq: u8,
    pub source: u8,
    pub error: u8,
    pub module: u8,
    pub code: u8,
}

impl ErrorRecord {
    pub fn from_response(r: &CommandResponse) -> ErrorRecord {
        ErrorRecord { severity: r.r[0], seq: r.r[1], source: r.r[2], error: r.r[3], module: r.r[4], code: r.r[5] }
    }
}

/// Per-sensor event counts (command 0x02, two responses).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Counts {
    pub sensor_id: u8,
    pub offered: u32,
    pub accepted: u32,
    pub on: u32,
    pub attempted: u32,
}

/// ME calibration enables as the sensor reports them.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalibrationConfig {
    pub accel: bool,
    pub gyro: bool,
    pub mag: bool,
    pub planar: bool,
}

/// A sensor's metadata FRS record (revision-gated fields are 0 on older records); `raw_words` is
/// authoritative.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SensorMetadata {
    pub me_version: u8,
    pub mh_version: u8,
    pub sh_version: u8,
    pub range_raw: u32,
    pub resolution_raw: u32,
    pub revision: u16,
    pub power_ma_q10: u16,
    pub min_period_us: u32,
    pub max_period_us: u32,
    pub fifo_max: u16,
    pub fifo_reserved: u16,
    pub batch_buffer_bytes: u16,
    pub q_point_1: u16,
    pub q_point_2: u16,
    pub q_point_3: u16,
    pub raw_words: Vec<u32>,
}

impl SensorMetadata {
    pub fn from_words(words: &[u32]) -> SensorMetadata {
        let w = |i: usize| words.get(i).copied().unwrap_or(0);
        // Word 3 is power_mA u16 (Q10) | revision u16 << 16.
        let revision = (w(3) >> 16) as u16;
        SensorMetadata {
            me_version: w(0) as u8,
            mh_version: (w(0) >> 8) as u8,
            sh_version: (w(0) >> 16) as u8,
            range_raw: w(1),
            resolution_raw: w(2),
            revision,
            power_ma_q10: w(3) as u16,
            min_period_us: w(4),
            max_period_us: if revision >= 4 { w(9) } else { 0 },
            fifo_max: w(5) as u16,
            fifo_reserved: (w(5) >> 16) as u16,
            batch_buffer_bytes: w(6) as u16,
            q_point_1: w(7) as u16,
            q_point_2: (w(7) >> 16) as u16,
            q_point_3: if revision >= 3 { (w(8) >> 16) as u16 } else { 0 },
            raw_words: words.to_vec(),
        }
    }
}

// ── request parameters ──────────────────────────────────────────────────────

pub const COUNTS_GET: u8 = 0;
pub const COUNTS_CLEAR: u8 = 1;
/// ME Calibration subcommand: report the current configuration.
pub const ME_CAL_GET: u8 = 0x01;

// Fixed lengths of the sensor → host control reports; one cargo may hold several.
fn control_response_length(id: u8) -> Option<usize> {
    match id {
        sh2::REPORT_COMMAND_RESPONSE | sh2::REPORT_FRS_READ_RESPONSE | sh2::REPORT_PRODUCT_ID_RESPONSE => Some(16),
        sh2::REPORT_FRS_WRITE_RESPONSE => Some(4),
        sh2::REPORT_GET_FEATURE_RESPONSE => Some(17),
        _ => None,
    }
}

/// Split a control cargo into its reports; an unknown id or a short tail ends the walk (the rest is
/// one piece: its length is unknowable).
pub fn split_control_cargo(payload: &[u8]) -> Vec<Vec<u8>> {
    let mut out = Vec::new();
    let mut pos = 0;
    while pos < payload.len() {
        match control_response_length(payload[pos]) {
            Some(n) if pos + n <= payload.len() => {
                out.push(payload[pos..pos + n].to_vec());
                pos += n;
            }
            _ => {
                out.push(payload[pos..].to_vec());
                break;
            }
        }
    }
    out
}

pub fn counts_get_params(sensor_id: u8) -> [u8; 2] {
    [COUNTS_GET, sensor_id]
}

pub fn counts_clear_params(sensor_id: u8) -> [u8; 2] {
    [COUNTS_CLEAR, sensor_id]
}

/// Errors of `severity` or greater (0 = all).
pub fn errors_params(severity: u8) -> [u8; 1] {
    [severity]
}

/// `axes`: a `TARE_*` bitmap.
pub fn tare_now_params(axes: u8, basis: TareBasis) -> [u8; 3] {
    [0x00, axes, basis as u8]
}

pub fn persist_tare_params() -> [u8; 1] {
    [0x01]
}

/// Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears); a component
/// out of the Q14 int16 range is refused.
pub fn set_reorientation_params(x: f64, y: f64, z: f64, w: f64) -> Result<[u8; 9], Sh2HubError> {
    let mut p = [0u8; 9];
    p[0] = 0x02;
    for (i, q) in [x, y, z, w].iter().enumerate() {
        let v = (q * (1 << 14) as f64).round_ties_even(); // Python round()
        if v.is_nan() || v < i16::MIN as f64 || v > i16::MAX as f64 {
            return Err(Sh2HubError(format!("quaternion component {q} out of Q14 int16 range")));
        }
        p[1 + 2 * i..3 + 2 * i].copy_from_slice(&(v as i16).to_le_bytes());
    }
    Ok(p)
}

/// ME Calibration (0x07): subcommand 0 configures, 1 ([`ME_CAL_GET`]) reads back.
pub fn me_calibration_params(accel: bool, gyro: bool, mag: bool, planar: bool, subcommand: u8) -> [u8; 5] {
    [accel as u8, gyro as u8, mag as u8, subcommand, planar as u8]
}

/// Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response.
pub fn interactive_calibration_params(intent: u8) -> [u8; 1] {
    [intent]
}

/// Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF).
pub fn force_flush(sensor_id: u8) -> [u8; 2] {
    [FORCE_SENSOR_FLUSH, sensor_id]
}

/// Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response.
pub fn periodic_dcd_params(enable: bool) -> [u8; 1] {
    [if enable { 0x00 } else { 0x01 }]
}

// ── FRS sessions ────────────────────────────────────────────────────────────

/// A multi-packet FRS read: send [`FrsReadSession::request`], [`FrsReadSession::feed`] every 0xF3 of
/// the record until it returns true; `words` holds the record. Error statuses fail.
#[derive(Debug, Clone)]
pub struct FrsReadSession {
    pub frs_type: u16,
    pub words: Vec<u32>,
    pub done: bool,
}

impl FrsReadSession {
    pub fn new(frs_type: u16) -> FrsReadSession {
        FrsReadSession { frs_type, words: Vec::new(), done: false }
    }

    pub fn request(&self) -> [u8; 8] {
        sh2::build_frs_read_request(self.frs_type, 0, 0)
    }

    pub fn feed(&mut self, r: &FrsReadResponse) -> Result<bool, Sh2HubError> {
        if r.frs_type != self.frs_type {
            return Ok(self.done); // another record's traffic
        }
        if matches!(r.status, FRS_UNRECOGNIZED_FRS_TYPE | FRS_BUSY | FRS_OFFSET_OUT_OF_RANGE | FRS_RECORD_EMPTY | FRS_DEVICE_ERROR) {
            return Err(Sh2HubError(format!("FRS read 0x{:04X} failed: status {}", self.frs_type, r.status)));
        }
        if r.data_length > 0 {
            self.words.push(r.data0);
        }
        if r.data_length > 1 {
            self.words.push(r.data1);
        }
        if matches!(r.status, FRS_READ_COMPLETED | FRS_BLOCK_AND_READ_COMPLETED) {
            self.done = true;
        }
        Ok(self.done)
    }
}

/// A multi-packet FRS write: send [`FrsWriteSession::request`]; for every 0xF5
/// [`FrsWriteSession::feed`] returns the next Write Data payload, or `None`; `done` on
/// WRITE_COMPLETED. Error statuses fail.
#[derive(Debug, Clone)]
pub struct FrsWriteSession {
    pub frs_type: u16,
    pub words: Vec<u32>,
    pub done: bool,
    offset: usize,
}

impl FrsWriteSession {
    pub fn new(frs_type: u16, words: &[u32]) -> FrsWriteSession {
        FrsWriteSession { frs_type, words: words.to_vec(), done: false, offset: 0 }
    }

    pub fn request(&self) -> [u8; 6] {
        sh2::build_frs_write_request(self.frs_type, self.words.len() as u16)
    }

    pub fn feed(&mut self, r: &FrsWriteResponse) -> Result<Option<[u8; 12]>, Sh2HubError> {
        match r.status {
            FRS_WRITE_UNRECOGNIZED_FRS_TYPE | FRS_WRITE_BUSY | FRS_WRITE_FAILED | FRS_WRITE_NOT_IN_WRITE_MODE
            | FRS_WRITE_INVALID_LENGTH | FRS_WRITE_RECORD_INVALID => {
                Err(Sh2HubError(format!("FRS write 0x{:04X} failed: status {}", self.frs_type, r.status)))
            }
            FRS_WRITE_COMPLETED => {
                self.done = true;
                Ok(None)
            }
            FRS_WRITE_MODE_READY | FRS_WRITE_WORDS_RECEIVED => {
                if self.offset >= self.words.len() {
                    return Ok(None);
                }
                let end = (self.offset + 2).min(self.words.len());
                let data = sh2::build_frs_write_data(self.offset as u16, &self.words[self.offset..end])
                    .map_err(|e| Sh2HubError(e.to_string()))?;
                self.offset = end;
                Ok(Some(data))
            }
            _ => Ok(None), // RECORD_VALID and friends: informational
        }
    }
}
