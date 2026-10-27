//! SHTP framing layer for the BNO086 (contract 05 §3).
//!
//! Pure codec — no I/O. A frame is a 4-byte header plus a cargo fragment:
//!
//! - `length` u16 LE — bits 14:0 cargo length *including* the 4-byte header;
//!   bit 15 set marks a continuation fragment.
//! - `channel` u8, `seq` u8 — per-channel, per-direction free-running counter.
//!
//! A multi-frame cargo's first fragment advertises the TOTAL cargo length;
//! continuations carry the remaining length with bit 15 set (informative).
//!
//! Mirrors the TS/Python `ShtpLayer`.

pub const SHTP_HEADER_SIZE: usize = 4;
pub const LENGTH_MASK: u16 = 0x7fff;
pub const CONTINUATION_BIT: u16 = 0x8000;
pub const NUM_CHANNELS: usize = 6;
/// Host→sensor frames must fit one MCU transmit slot (ERRATA E2).
pub const MAX_TX_FRAME: usize = 64;

/// SHTP channels (contract 05 §3).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ShtpChannel {
    Command = 0,
    Executable = 1,
    Control = 2,
    InputNormal = 3,
    InputWake = 4,
    GyroRv = 5,
}

/// One-byte host writes on the executable channel (SHTP channel 1,
/// contract 05 §6a). The hub answers none of them.
///
/// `Reset` restarts the hub (answered later by the 0x01 reset-complete on the
/// same channel); `Sleep` stops every report except those enabled with
/// `sh2::FEATURE_FLAG_WAKEUP | sh2::FEATURE_FLAG_ALWAYS_ON`, keeping the
/// configuration; `On` ends `Sleep` and every report resumes at its rate.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ExecutableCommand {
    Reset = 1,
    On = 2,
    Sleep = 3,
}

/// SH-2 "Sleep" payload for the executable channel: `[0x03]`. Send it with
/// `try_next_frame(ShtpChannel::Executable as u8, &build_sleep())` through the
/// bridge's SEND_SHTP_PACKET (0x34); the hub sends no response.
///
/// Every report stops, except those enabled with flags wake-up | always-on,
/// which keep their rate; the configuration (enabled sensors, rates) is kept.
/// [`build_power_on`] ends it. On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16) all
/// reports stopped within one sample; a WAKEUP|ALWAYS_ON report kept
/// streaming at full rate on channel 4.
pub fn build_sleep() -> [u8; 1] {
    [ExecutableCommand::Sleep as u8]
}

/// SH-2 "On" payload for the executable channel: `[0x02]` — ends
/// [`build_sleep`]'s sleep. Every report that was enabled comes back at its
/// rate, without re-enabling; the hub sends no response.
///
/// Not the bridge's SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin —
/// a hardware signal, not an SH-2 command.
pub fn build_power_on() -> [u8; 1] {
    [ExecutableCommand::On as u8]
}

/// A decoded SHTP header.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ShtpHeader {
    /// Bits 14:0 — cargo length incl. this 4-byte header.
    pub length: u16,
    pub channel: u8,
    pub seq: u8,
    pub continuation: bool,
}

/// SHTP codec error (contract 07 §3: an argument that does not fit its wire
/// field, or input shorter than the fields it must hold, is rejected —
/// never masked, wrapped or read past).
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ShtpError {
    /// `length` does not fit the 15-bit length field (max 0x7FFF).
    LengthTooWide(usize),
    /// Fewer than the 4 header bytes.
    ShortHeader(usize),
    /// Channel outside 0..[`NUM_CHANNELS`].
    ChannelOutOfRange(u8),
    /// A host→sensor frame larger than one MCU transmit slot ([`MAX_TX_FRAME`]).
    TxCargoTooLong(usize),
}

impl std::fmt::Display for ShtpError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ShtpError::LengthTooWide(n) => write!(f, "SHTP length {} out of range for u15", n),
            ShtpError::ShortHeader(n) => write!(f, "SHTP header needs 4 bytes, got {}", n),
            ShtpError::ChannelOutOfRange(c) => {
                write!(f, "SHTP channel {} out of range 0..{}", c, NUM_CHANNELS - 1)
            }
            ShtpError::TxCargoTooLong(n) => {
                write!(f, "TX cargo {}B exceeds the {}B MCU slot", n, MAX_TX_FRAME)
            }
        }
    }
}

impl std::error::Error for ShtpError {}

/// Serialize an SHTP header to 4 bytes; `length` must fit 15 bits
/// ([`ShtpError::LengthTooWide`] otherwise — never masked).
pub fn try_pack_shtp_header(hdr: &ShtpHeader) -> Result<[u8; 4], ShtpError> {
    if hdr.length > LENGTH_MASK {
        return Err(ShtpError::LengthTooWide(hdr.length as usize));
    }
    let word = hdr.length
        | if hdr.continuation {
            CONTINUATION_BIT
        } else {
            0
        };
    let w = word.to_le_bytes();
    Ok([w[0], w[1], hdr.channel, hdr.seq])
}

/// Decode an SHTP header from the first 4 bytes of `data` (trailing bytes —
/// the cargo — are ignored); [`ShtpError::ShortHeader`] when shorter.
pub fn try_unpack_shtp_header(data: &[u8]) -> Result<ShtpHeader, ShtpError> {
    if data.len() < SHTP_HEADER_SIZE {
        return Err(ShtpError::ShortHeader(data.len()));
    }
    let word = u16::from_le_bytes([data[0], data[1]]);
    Ok(ShtpHeader {
        length: word & LENGTH_MASK,
        channel: data[2],
        seq: data[3],
        continuation: (word & CONTINUATION_BIT) != 0,
    })
}

/// One reassembled cargo: `payload` excludes all SHTP headers.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ShtpCargo {
    pub channel: u8,
    /// seq of the first fragment.
    pub seq: u8,
    pub payload: Vec<u8>,
    /// `RPT_DATA.timestamp_us` of the FIRST fragment (contract 05 §3) — the
    /// capture the cargo's 0xFB base delta refers to; 0 when fed via [`ShtpLayer::feed`].
    pub capture_us: u64,
}

/// Single-fragment frame: length = header + payload. A frame longer than the
/// 15-bit length field holds is [`ShtpError::LengthTooWide`].
pub fn try_build_frame(channel: u8, payload: &[u8], seq: u8) -> Result<Vec<u8>, ShtpError> {
    let total = SHTP_HEADER_SIZE + payload.len();
    if total > LENGTH_MASK as usize {
        return Err(ShtpError::LengthTooWide(total));
    }
    let hdr = try_pack_shtp_header(&ShtpHeader {
        length: total as u16,
        channel,
        seq,
        continuation: false,
    })?;
    let mut out = Vec::with_capacity(total);
    out.extend_from_slice(&hdr);
    out.extend_from_slice(payload);
    Ok(out)
}

#[derive(Debug, Default, Clone)]
struct ChannelRx {
    chunks: Vec<u8>,
    received: usize,
    /// Total cargo payload bytes (headers excluded).
    expected: usize,
    seq: u8,
    capture_us: u64,
}

/// Per-channel TX sequence counters + RX cargo reassembly.
#[derive(Debug, Clone)]
pub struct ShtpLayer {
    /// Incomplete cargos and junk-channel (≥ 6) frames thrown away.
    pub discarded: u64,
    tx_seqs: [u8; NUM_CHANNELS],
    rx: Vec<ChannelRx>,
}

impl Default for ShtpLayer {
    fn default() -> Self {
        ShtpLayer {
            discarded: 0,
            tx_seqs: [0; NUM_CHANNELS],
            rx: vec![ChannelRx::default(); NUM_CHANNELS],
        }
    }
}

impl ShtpLayer {
    pub fn new() -> Self {
        ShtpLayer::default()
    }

    /// Build a single-fragment frame, consuming the channel's TX seq (the seq
    /// wraps 255 → 0 by design). The frame must fit one MCU transmit slot
    /// ([`ShtpError::TxCargoTooLong`]; host-side control payloads are ≤ 21
    /// bytes) and `channel` must be 0..[`NUM_CHANNELS`]
    /// ([`ShtpError::ChannelOutOfRange`]). Nothing is consumed on an error.
    pub fn try_next_frame(&mut self, channel: u8, payload: &[u8]) -> Result<Vec<u8>, ShtpError> {
        if SHTP_HEADER_SIZE + payload.len() > MAX_TX_FRAME {
            return Err(ShtpError::TxCargoTooLong(payload.len()));
        }
        let ch = channel as usize;
        if ch >= NUM_CHANNELS {
            return Err(ShtpError::ChannelOutOfRange(channel));
        }
        let seq = self.tx_seqs[ch];
        let frame = try_build_frame(channel, payload, seq)?;
        self.tx_seqs[ch] = seq.wrapping_add(1);
        Ok(frame)
    }

    /// Current TX seq counter for a channel (next frame's seq); `None` for a
    /// channel outside 0..[`NUM_CHANNELS`].
    pub fn try_tx_seq(&self, channel: u8) -> Option<u8> {
        self.tx_seqs.get(channel as usize).copied()
    }

    /// Current TX seq counter for a channel (next frame's seq).
    ///
    /// # Panics
    ///
    /// When `channel` ≥ [`NUM_CHANNELS`]; use [`ShtpLayer::try_tx_seq`].
    pub fn tx_seq(&self, channel: u8) -> u8 {
        self.try_tx_seq(channel)
            .unwrap_or_else(|| panic!("{}", ShtpError::ChannelOutOfRange(channel)))
    }

    /// Consume one inbound frame; return the cargo when complete.
    ///
    /// Rules (contract 05 §3): a non-continuation fragment starts a new cargo
    /// (discarding any partial one on that channel); a continuation without a
    /// cargo in progress is dropped; the cargo completes when the accumulated
    /// bytes reach the first fragment's advertised total.
    ///
    /// Same as [`ShtpLayer::feed_at`] with capture 0 (cargo `capture_us` = 0).
    pub fn feed(&mut self, frame: &[u8]) -> Option<ShtpCargo> {
        self.feed_at(frame, 0)
    }

    /// [`ShtpLayer::feed`] with the frame's `RPT_DATA.timestamp_us`: the cargo
    /// carries the FIRST fragment's capture in `capture_us` — the base its 0xFB
    /// timestamps refer to (contract 05 §3), not the fragment that completed it.
    pub fn feed_at(&mut self, frame: &[u8], capture_us: u64) -> Option<ShtpCargo> {
        let Ok(hdr) = try_unpack_shtp_header(frame) else {
            return None; // shorter than a header: junk
        };
        if (hdr.length as usize) < SHTP_HEADER_SIZE {
            return None; // padding/"no data" header
        }
        if hdr.channel as usize >= NUM_CHANNELS {
            self.discarded += 1; // junk channel
            return None;
        }
        let chunk = &frame[SHTP_HEADER_SIZE..];
        let ch = hdr.channel as usize;

        if !hdr.continuation {
            {
                let rx = &self.rx[ch];
                if rx.expected != 0 {
                    self.discarded += 1; // a partial, even one with no bytes yet
                }
            }
            let rx = &mut self.rx[ch];
            rx.chunks = chunk.to_vec();
            rx.received = chunk.len();
            rx.expected = hdr.length as usize - SHTP_HEADER_SIZE;
            rx.seq = hdr.seq;
            rx.capture_us = capture_us;
        } else {
            if self.rx[ch].expected == 0 {
                self.discarded += 1;
                return None;
            }
            let rx = &mut self.rx[ch];
            rx.chunks.extend_from_slice(chunk);
            rx.received += chunk.len();
        }

        let rx = &mut self.rx[ch];
        if rx.received < rx.expected {
            return None;
        }
        if rx.received > rx.expected {
            self.discarded += 1;
            rx.chunks.clear();
            rx.received = 0;
            rx.expected = 0;
            return None;
        }
        let cargo = ShtpCargo {
            channel: hdr.channel,
            seq: rx.seq,
            payload: std::mem::take(&mut rx.chunks),
            capture_us: rx.capture_us,
        };
        rx.received = 0;
        rx.expected = 0;
        Some(cargo)
    }

    /// Forget all TX seq counters and partial cargos (sensor reset).
    pub fn reset(&mut self) {
        self.tx_seqs = [0; NUM_CHANNELS];
        self.rx = vec![ChannelRx::default(); NUM_CHANNELS];
    }
}
