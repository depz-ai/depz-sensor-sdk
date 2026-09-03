//! Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).
//!
//! Codecs return raw integers exactly as on the wire; unit conversions happen in
//! the device layer.

/// Host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Cmd {
    Bootloader = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetNameActiveSoftware = 0x04,
    GetSerial = 0x05,
    SyncTime = 0x06,
    GetMcuTemperature = 0x07,
    GetPayloadCrcType = 0x08,
    SetPayloadCrcType = 0x09,
    ThroughputTxStart = 0x1C,
    ThroughputTxStop = 0x1D,
    ThroughputRxData = 0x1E,
    GetSyncPinConfig = 0x30,
    SetSyncPinConfig = 0x31,
}

/// Device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Rpt {
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    ThroughputData = 0x88,
    SyncPinConfig = 0x90,
}

/// Status codes carried by `Rpt::Status`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Status {
    Ok = 0x00,
    Error = 0x01,
    ErrInvalidCmd = 0x02,
    ErrPayloadFormat = 0x03,
    ErrInvalidParam = 0x04,
    ErrPayloadCrc = 0x05,
    ErrBusy = 0x06,
    ErrCmdNotSupported = 0x07,
    ErrNotInitialized = 0x08,
    ErrHardwareFault = 0x09,
}

/// Sync-pin operating mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum SyncPinMode {
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
}

/// Sync-pin idle polarity.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum SyncPinPolarity {
    IdleLow = 0x00,
    IdleHigh = 0x01,
}

/// Value of the echoed-cmd byte in unsolicited reports.
pub const UNSOLICITED: u8 = 0x00;

/// Payload codec error.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CodecError(pub &'static str);

impl std::fmt::Display for CodecError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "codec error: {}", self.0)
    }
}

impl std::error::Error for CodecError {}

/// Decoded `Rpt::Status`: echoed request opcode (0x00 = unsolicited) + status.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct StatusReport {
    pub cmd: u8,
    pub status: u8,
}

impl StatusReport {
    pub fn unpack(payload: &[u8]) -> Result<StatusReport, CodecError> {
        if payload.len() != 2 {
            return Err(CodecError("status report must be 2 bytes"));
        }
        Ok(StatusReport {
            cmd: payload[0],
            status: payload[1],
        })
    }
}

/// Decoded `Rpt::Text`: echoed opcode + ASCII text (trailing NUL/0xFF stripped).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TextReport {
    pub cmd: u8,
    pub text: String,
}

impl TextReport {
    pub fn unpack(payload: &[u8]) -> Result<TextReport, CodecError> {
        if payload.is_empty() {
            return Err(CodecError("text report needs an echoed-cmd byte"));
        }
        Ok(TextReport {
            cmd: payload[0],
            text: strip_device_string(&payload[1..]),
        })
    }
}

/// Decoded `Rpt::SyncTime`: T1 echoed, T2 mcu-rx, T3 mcu-tx (all µs).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SyncTimeReport {
    pub pc_timestamp_us: u64,
    pub mcu_rx_us: u64,
    pub mcu_tx_us: u64,
}

impl SyncTimeReport {
    pub fn unpack(payload: &[u8]) -> Result<SyncTimeReport, CodecError> {
        if payload.len() != 24 {
            return Err(CodecError("sync-time report must be 24 bytes"));
        }
        Ok(SyncTimeReport {
            pc_timestamp_us: u64::from_le_bytes(payload[0..8].try_into().unwrap()),
            mcu_rx_us: u64::from_le_bytes(payload[8..16].try_into().unwrap()),
            mcu_tx_us: u64::from_le_bytes(payload[16..24].try_into().unwrap()),
        })
    }
}

/// Decoded `Rpt::Temperature`: timestamp + raw int16 in units of 0.1 °C.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TemperatureReport {
    pub timestamp_us: u64,
    pub raw_decidegrees: i16,
}

impl TemperatureReport {
    pub fn unpack(payload: &[u8]) -> Result<TemperatureReport, CodecError> {
        if payload.len() != 10 {
            return Err(CodecError("temperature report must be 10 bytes"));
        }
        Ok(TemperatureReport {
            timestamp_us: u64::from_le_bytes(payload[0..8].try_into().unwrap()),
            raw_decidegrees: i16::from_le_bytes(payload[8..10].try_into().unwrap()),
        })
    }

    pub fn celsius(&self) -> f64 {
        self.raw_decidegrees as f64 / 10.0
    }
}

/// Decoded `Rpt::SequenceError`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SequenceErrorReport {
    pub expected_seq: u8,
    pub received_seq: u8,
}

impl SequenceErrorReport {
    pub fn unpack(payload: &[u8]) -> Result<SequenceErrorReport, CodecError> {
        if payload.len() != 2 {
            return Err(CodecError("sequence-error report must be 2 bytes"));
        }
        Ok(SequenceErrorReport {
            expected_seq: payload[0],
            received_seq: payload[1],
        })
    }
}

/// Sync-pin configuration (pin 1..5, mode, polarity).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SyncPinConfig {
    pub pin: u8,
    pub mode: u8,
    pub polarity: u8,
}

impl SyncPinConfig {
    pub fn pack(&self) -> [u8; 3] {
        [self.pin, self.mode, self.polarity]
    }

    pub fn unpack(payload: &[u8]) -> Result<SyncPinConfig, CodecError> {
        if payload.len() != 3 {
            return Err(CodecError("sync-pin config must be 3 bytes"));
        }
        Ok(SyncPinConfig {
            pin: payload[0],
            mode: payload[1],
            polarity: payload[2],
        })
    }
}

/// Encode the `SYNC_TIME` request payload (T1, µs, u64 LE).
pub fn pack_sync_time(pc_timestamp_us: u64) -> [u8; 8] {
    pc_timestamp_us.to_le_bytes()
}

/// Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte).
pub fn pack_set_payload_crc_type(crc_type: u8) -> [u8; 1] {
    [crc_type]
}

/// NTP-style clock math, all µs (contract 02 §5).
///
/// Returns `(offset_us, rtt_us)` where `offset = device_clock - host_clock`,
/// computed as `((T2-T1)+(T3-T4)) / 2` truncated toward zero (Rust integer
/// division truncates toward zero natively). Computed in `i128` to avoid
/// overflow, matching the shared i64/i128 rule.
pub fn sync_time_offset_rtt(t1: i64, t2: i64, t3: i64, t4: i64) -> (i64, i64) {
    let (t1, t2, t3, t4) = (t1 as i128, t2 as i128, t3 as i128, t4 as i128);
    let offset = ((t2 - t1) + (t3 - t4)) / 2;
    let rtt = (t4 - t1) - (t3 - t2);
    (offset as i64, rtt as i64)
}

/// Decode an ASCII device string, dropping trailing NUL/0xFF filler.
pub fn strip_device_string(raw: &[u8]) -> String {
    let end = raw
        .iter()
        .rposition(|&b| b != 0x00 && b != 0xFF)
        .map_or(0, |i| i + 1);
    String::from_utf8_lossy(&raw[..end]).into_owned()
}
