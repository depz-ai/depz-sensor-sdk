//! Packet framing and incremental parser (contracts/01_TRANSPORT_FRAMING.md).
//!
//! Byte-exact with firmware `common/transport/transport.c`. The parser follows
//! ERRATA E6: a packet whose header advertises a payload CRC type but carries an
//! empty payload has **no** CRC bytes on the wire.

use crate::crc::{crc16_modbus, crc32_iso_hdlc, crc8_maxim};

/// Frame start marker.
pub const MAGIC: [u8; 2] = [0xA5, 0xC3];
/// Fixed header length: magic(2) + data_size(2) + cmd(1) + seq(1) + hdr_crc(1).
pub const HEADER_SIZE: usize = 7;
/// Maximum payload length (14-bit field).
pub const MAX_PAYLOAD: usize = 0x3FFF;

/// Payload CRC selector encoded in the top two bits of `data_size`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CrcType {
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
}

impl CrcType {
    /// Decode from the 2-bit field value.
    pub fn from_bits(v: u16) -> CrcType {
        match v & 0x03 {
            0 => CrcType::None,
            1 => CrcType::Crc8,
            2 => CrcType::Crc16,
            _ => CrcType::Crc32,
        }
    }

    fn size(self) -> usize {
        match self {
            CrcType::None => 0,
            CrcType::Crc8 => 1,
            CrcType::Crc16 => 2,
            CrcType::Crc32 => 4,
        }
    }
}

/// Errors from `build_packet`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FramingError {
    PayloadTooLong(usize),
}

impl std::fmt::Display for FramingError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            FramingError::PayloadTooLong(n) => {
                write!(f, "payload too long: {} > {}", n, MAX_PAYLOAD)
            }
        }
    }
}

impl std::error::Error for FramingError {}

/// CRC trailer for a payload; empty payloads never carry CRC bytes (ERRATA E6).
pub fn payload_crc_bytes(crc_type: CrcType, payload: &[u8]) -> Vec<u8> {
    if crc_type == CrcType::None || payload.is_empty() {
        return Vec::new();
    }
    match crc_type {
        CrcType::None => Vec::new(),
        CrcType::Crc8 => vec![crc8_maxim(payload)],
        CrcType::Crc16 => crc16_modbus(payload).to_le_bytes().to_vec(),
        CrcType::Crc32 => crc32_iso_hdlc(payload).to_le_bytes().to_vec(),
    }
}

/// Frame one packet. `crc_type` bits are set in the header even for an empty
/// payload (matching device TX), but CRC bytes are only appended for non-empty
/// payloads. `seq` is taken modulo 256.
pub fn build_packet(
    cmd: u8,
    payload: &[u8],
    seq: u32,
    crc_type: CrcType,
) -> Result<Vec<u8>, FramingError> {
    if payload.len() > MAX_PAYLOAD {
        return Err(FramingError::PayloadTooLong(payload.len()));
    }
    let data_size: u16 = (payload.len() as u16) | ((crc_type as u16) << 14);
    let ds = data_size.to_le_bytes();
    let seq_b = (seq & 0xFF) as u8;
    let hdr_crc = crc8_maxim(&[ds[0], ds[1], cmd, seq_b]);

    let mut out = Vec::with_capacity(HEADER_SIZE + payload.len() + 4);
    out.extend_from_slice(&MAGIC);
    out.extend_from_slice(&ds);
    out.push(cmd);
    out.push(seq_b);
    out.push(hdr_crc);
    out.extend_from_slice(payload);
    out.extend_from_slice(&payload_crc_bytes(crc_type, payload));
    Ok(out)
}

/// A decoded packet.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Packet {
    pub cmd: u8,
    pub seq: u8,
    pub payload: Vec<u8>,
}

/// Events yielded by the incremental parser.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Event {
    /// A well-formed packet.
    Packet(Packet),
    /// Bytes discarded while hunting for a valid frame. Boundaries between
    /// consecutive `Trash` events depend on read chunking; only the
    /// concatenated byte stream is deterministic.
    Trash(Vec<u8>),
    /// A frame with a valid header whose payload CRC failed; dropped.
    CrcError { cmd: u8, seq: u8 },
}

/// Incremental frame parser. Feed arbitrary byte chunks; get events.
///
/// Event order is invariant to chunking (contract 01 §5) except `Trash` event
/// boundaries — concatenate `Trash` data when comparing streams.
#[derive(Debug, Default)]
pub struct PacketParser {
    buf: Vec<u8>,
    pub packets: u64,
    pub crc_errors: u64,
    pub header_errors: u64,
    pub trash_bytes: u64,
}

impl PacketParser {
    pub fn new() -> Self {
        PacketParser::default()
    }

    /// Bytes still buffered (an incomplete frame or a trailing lone `0xA5`).
    pub fn residue(&self) -> &[u8] {
        &self.buf
    }

    /// Feed a chunk and drain all events it makes available.
    pub fn feed(&mut self, data: &[u8]) -> Vec<Event> {
        self.buf.extend_from_slice(data);
        let mut out = Vec::new();
        while let Some(ev) = self.parse_one() {
            out.push(ev);
        }
        out
    }

    fn find_magic(&self) -> Option<usize> {
        if self.buf.len() < 2 {
            return None;
        }
        self.buf.windows(2).position(|w| w == MAGIC)
    }

    fn emit_trash(&mut self, count: usize) -> Event {
        let data: Vec<u8> = self.buf[..count].to_vec();
        self.buf.drain(..count);
        self.trash_bytes += count as u64;
        Event::Trash(data)
    }

    fn parse_one(&mut self) -> Option<Event> {
        let pos = self.find_magic();
        match pos {
            None => {
                // Keep the last byte: it may be a split 0xA5.
                if self.buf.len() > 1 {
                    return Some(self.emit_trash(self.buf.len() - 1));
                }
                None
            }
            Some(p) if p > 0 => Some(self.emit_trash(p)),
            Some(_) => {
                // Magic at position 0.
                if self.buf.len() < HEADER_SIZE {
                    return None;
                }
                let data_size = u16::from_le_bytes([self.buf[2], self.buf[3]]);
                let payload_size = (data_size as usize) & MAX_PAYLOAD;
                let crc_type = CrcType::from_bits(data_size >> 14);
                if crc8_maxim(&self.buf[2..6]) != self.buf[6] {
                    // Header corrupt: advance one byte past the magic start and
                    // let the magic hunt resync (firmware: rb_skip(off + 1)).
                    self.header_errors += 1;
                    return Some(self.emit_trash(1));
                }
                // ERRATA E6: empty payload never carries CRC bytes.
                let crc_size = if payload_size > 0 { crc_type.size() } else { 0 };
                let total = HEADER_SIZE + payload_size + crc_size;
                if self.buf.len() < total {
                    return None;
                }
                let cmd = self.buf[4];
                let seq = self.buf[5];
                let payload: Vec<u8> =
                    self.buf[HEADER_SIZE..HEADER_SIZE + payload_size].to_vec();
                let trailer: Vec<u8> =
                    self.buf[HEADER_SIZE + payload_size..total].to_vec();
                self.buf.drain(..total);
                if crc_size > 0 && payload_crc_bytes(crc_type, &payload) != trailer {
                    self.crc_errors += 1;
                    return Some(Event::CrcError { cmd, seq });
                }
                self.packets += 1;
                Some(Event::Packet(Packet { cmd, seq, payload }))
            }
        }
    }
}
