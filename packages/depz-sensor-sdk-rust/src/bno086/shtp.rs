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

/// A decoded SHTP header.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ShtpHeader {
    /// Bits 14:0 — cargo length incl. this 4-byte header.
    pub length: u16,
    pub channel: u8,
    pub seq: u8,
    pub continuation: bool,
}

/// Serialize an SHTP header to 4 bytes.
pub fn pack_shtp_header(hdr: &ShtpHeader) -> [u8; 4] {
    let word = (hdr.length & LENGTH_MASK) | if hdr.continuation { CONTINUATION_BIT } else { 0 };
    let w = word.to_le_bytes();
    [w[0], w[1], hdr.channel, hdr.seq]
}

/// Decode a 4-byte SHTP header (reads the first 4 bytes of `data`).
pub fn unpack_shtp_header(data: &[u8]) -> ShtpHeader {
    let word = u16::from_le_bytes([data[0], data[1]]);
    ShtpHeader {
        length: word & LENGTH_MASK,
        channel: data[2],
        seq: data[3],
        continuation: (word & CONTINUATION_BIT) != 0,
    }
}

/// One reassembled cargo: `payload` excludes all SHTP headers.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ShtpCargo {
    pub channel: u8,
    /// seq of the first fragment.
    pub seq: u8,
    pub payload: Vec<u8>,
}

/// Single-fragment frame: length = header + payload.
pub fn build_frame(channel: u8, payload: &[u8], seq: u8) -> Vec<u8> {
    let hdr = pack_shtp_header(&ShtpHeader {
        length: (SHTP_HEADER_SIZE + payload.len()) as u16,
        channel,
        seq,
        continuation: false,
    });
    let mut out = Vec::with_capacity(SHTP_HEADER_SIZE + payload.len());
    out.extend_from_slice(&hdr);
    out.extend_from_slice(payload);
    out
}

#[derive(Debug, Default, Clone)]
struct ChannelRx {
    chunks: Vec<u8>,
    received: usize,
    /// Total cargo payload bytes (headers excluded).
    expected: usize,
    seq: u8,
}

/// Per-channel TX sequence counters + RX cargo reassembly.
#[derive(Debug, Clone)]
pub struct ShtpLayer {
    /// Incomplete cargos thrown away.
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

    /// Build a single-fragment frame, consuming the channel's TX seq.
    pub fn next_frame(&mut self, channel: u8, payload: &[u8]) -> Vec<u8> {
        let ch = channel as usize;
        let seq = self.tx_seqs[ch];
        self.tx_seqs[ch] = seq.wrapping_add(1);
        build_frame(channel, payload, seq)
    }

    /// Current TX seq counter for a channel (next frame's seq).
    pub fn tx_seq(&self, channel: u8) -> u8 {
        self.tx_seqs[channel as usize]
    }

    /// Consume one inbound frame; return the cargo when complete.
    ///
    /// Rules (contract 05 §3): a non-continuation fragment starts a new cargo
    /// (discarding any partial one on that channel); a continuation without a
    /// cargo in progress is dropped; the cargo completes when the accumulated
    /// bytes reach the first fragment's advertised total.
    pub fn feed(&mut self, frame: &[u8]) -> Option<ShtpCargo> {
        if frame.len() < SHTP_HEADER_SIZE {
            return None;
        }
        let hdr = unpack_shtp_header(frame);
        if hdr.channel as usize >= NUM_CHANNELS || (hdr.length as usize) < SHTP_HEADER_SIZE {
            return None; // padding/"no data" header or junk
        }
        let chunk = &frame[SHTP_HEADER_SIZE..];
        let ch = hdr.channel as usize;

        if !hdr.continuation {
            {
                let rx = &self.rx[ch];
                if rx.expected != 0 && rx.received != 0 {
                    self.discarded += 1;
                }
            }
            let rx = &mut self.rx[ch];
            rx.chunks = chunk.to_vec();
            rx.received = chunk.len();
            rx.expected = hdr.length as usize - SHTP_HEADER_SIZE;
            rx.seq = hdr.seq;
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
