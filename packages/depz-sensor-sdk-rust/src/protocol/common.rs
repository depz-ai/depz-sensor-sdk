//! Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).
//!
//! Codecs return raw integers exactly as on the wire; unit conversions happen in
//! the device layer. Decoders reject a payload shorter than its fields and
//! ignore trailing bytes (a newer firmware may append fields; contract 07 §3).

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
    UsbTestTxStart = 0x1C,
    UsbTestTxStop = 0x1D,
    UsbTestRxData = 0x1E,
    UsbTestStats = 0x1F,
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
    UsbTestData = 0x88,
    UsbTestStats = 0x8F,
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
        if payload.len() < 2 {
            return Err(CodecError("status report needs at least 2 bytes"));
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
        if payload.len() < 24 {
            return Err(CodecError("sync-time report needs at least 24 bytes"));
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
        if payload.len() < 10 {
            return Err(CodecError("temperature report needs at least 10 bytes"));
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
        if payload.len() < 2 {
            return Err(CodecError("sequence-error report needs at least 2 bytes"));
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

/// First / last logical AUX sync pin (contract 02 §6).
pub const SYNC_PIN_MIN: u8 = 1;
pub const SYNC_PIN_MAX: u8 = 5;

impl SyncPinConfig {
    /// `SET_SYNC_PIN_CONFIG` payload. Contract 02 §6: pin 1..5, mode a
    /// [`SyncPinMode`] (0..4), polarity a [`SyncPinPolarity`] (0..1) — anything
    /// else is refused.
    pub fn try_pack(&self) -> Result<[u8; 3], CodecError> {
        if !(SYNC_PIN_MIN..=SYNC_PIN_MAX).contains(&self.pin) {
            return Err(CodecError("sync pin must be 1..5"));
        }
        if self.mode > SyncPinMode::OutBoth as u8 {
            return Err(CodecError("sync-pin mode is not a SyncPinMode (0..4)"));
        }
        if self.polarity > SyncPinPolarity::IdleHigh as u8 {
            return Err(CodecError(
                "sync-pin polarity is not a SyncPinPolarity (0..1)",
            ));
        }
        Ok([self.pin, self.mode, self.polarity])
    }

    /// [`SyncPinConfig::try_pack`] that panics on an invalid configuration.
    pub fn pack(&self) -> [u8; 3] {
        self.try_pack().unwrap_or_else(|e| panic!("{}", e))
    }

    pub fn unpack(payload: &[u8]) -> Result<SyncPinConfig, CodecError> {
        if payload.len() < 3 {
            return Err(CodecError("sync-pin config needs at least 3 bytes"));
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

/// `SET_PAYLOAD_CRC_TYPE` takes 0..3 (contract 02 §2).
pub const PAYLOAD_CRC_TYPE_MAX: u8 = 3;

/// Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte); a value
/// outside 0..=[`PAYLOAD_CRC_TYPE_MAX`] is refused before anything is written
/// (contract 07 §3).
pub fn try_pack_set_payload_crc_type(crc_type: u8) -> Result<[u8; 1], CodecError> {
    if crc_type > PAYLOAD_CRC_TYPE_MAX {
        return Err(CodecError(
            "SET_PAYLOAD_CRC_TYPE: crc_type is not a CrcType 0..3",
        ));
    }
    Ok(pack_set_payload_crc_type(crc_type))
}

/// Encode the `SET_PAYLOAD_CRC_TYPE` request payload (single byte), unchecked
/// — prefer [`try_pack_set_payload_crc_type`] for an untyped value (the
/// device layer passes a [`crate::framing::CrcType`], always 0..3).
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

/// Decode an ASCII device string, dropping trailing NUL/0xFF filler
/// (contract 02 §4.1).
///
/// Device strings are ASCII: every byte ≥ 0x80 becomes one U+FFFD (never
/// decoded as UTF-8 or Latin-1), and a `0x00` before the trailing filler is
/// kept as a character, not a terminator.
pub fn strip_device_string(raw: &[u8]) -> String {
    let end = raw
        .iter()
        .rposition(|&b| b != 0x00 && b != 0xFF)
        .map_or(0, |i| i + 1);
    raw[..end]
        .iter()
        .map(|&b| {
            if b < 0x80 {
                b as char
            } else {
                char::REPLACEMENT_CHARACTER
            }
        })
        .collect()
}
