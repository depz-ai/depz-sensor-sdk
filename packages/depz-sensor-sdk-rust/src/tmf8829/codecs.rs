//! TMF8829 SPI register-bridge wire codecs, protocol v1.00
//! (contracts/14_SENSOR_TMF8829.md §2).
//!
//! Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
//! TMF8829 payloads: an 8-bit register address and self-sized frames, so the
//! host never tells the bridge a frame size. Unlike the BNO055 encoders these
//! check their ranges (like the reference SDK): the bridge drops an oversized
//! write without a reply, so a bad payload is refused before it is sent.

use crate::protocol::common::CodecError;

/// TMF8829 host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Tmf8829Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}

/// TMF8829 device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Tmf8829Rpt {
    RegData = 0x91,
    /// No echoed cmd byte: a host matching replies by `payload[0] == cmd`
    /// must exempt it.
    Info = 0x92,
    Frame = 0x93,
    Event = 0x94,
}

/// `RPT_TMF_EVENT.code`: a MOTION/PROXIMITY interrupt, no frame; its bits
/// already cleared.
pub const EV_INT: u8 = 0;
/// `RPT_TMF_EVENT.code`: a frame header refused, the frame not read. Expected
/// exactly once on STOP mid histogram dump (SDKs swallow it); otherwise the
/// sensor is lost — re-run init.
pub const EV_BAD_HEADER: u8 = 1;

/// `TMF_READ_REG` `len` 1..=1024.
pub const READ_MAX: usize = 1024;
/// `TMF_WRITE_REG` N 1..=1023 (1024 B command buffer minus the address).
pub const WRITE_MAX: usize = 1023;
/// Frame bytes per `RPT_TMF_FRAME` chunk.
pub const CHUNK_MAX: usize = 1024;

/// `INT_STATUS` bits the bridge serves (`TMF_START_STREAM` `int_mask`).
pub const INT_RESULTS: u8 = 0x01;
pub const INT_MOTION: u8 = 0x02;
pub const INT_PROXIMITY: u8 = 0x04;
pub const INT_HISTOGRAMS: u8 = 0x08;
pub const INT_MASK_ALL: u8 = 0x0F;

/// `TMF_START_STREAM` flags: emit the 60-byte ref-SPAD frame (behind a zero
/// 5-byte pre-header) before each results frame.
pub const SF_REF_FRAME: u8 = 0x01;

pub const REG_DATA_HEADER_SIZE: usize = 9;
pub const FRAME_CHUNK_HEADER_SIZE: usize = 21;
pub const EVENT_SIZE: usize = 12;
pub const INFO_SIZE: usize = 25;

/// `TMF_READ_REG` payload: `addr u8, len u16 LE`. A read reaching `0xFF`
/// (the FIFO port) keeps streaming from the FIFO — there is no
/// `addr + len ≤ 0x100` check.
pub fn pack_read_reg(addr: u8, len: u16) -> Result<[u8; 3], CodecError> {
    if len == 0 || len as usize > READ_MAX {
        return Err(CodecError("TMF_READ_REG: len must be 1..1024"));
    }
    let l = len.to_le_bytes();
    Ok([addr, l[0], l[1]])
}

/// `TMF_WRITE_REG` payload: `addr u8, data[1..=1023]`.
pub fn pack_write_reg(addr: u8, data: &[u8]) -> Result<Vec<u8>, CodecError> {
    if data.is_empty() || data.len() > WRITE_MAX {
        return Err(CodecError("TMF_WRITE_REG: data must be 1..1023 bytes"));
    }
    let mut out = Vec::with_capacity(1 + data.len());
    out.push(addr);
    out.extend_from_slice(data);
    Ok(out)
}

/// `TMF_START_STREAM` payload: `int_mask u8, flags u8`. `int_mask` a
/// non-zero subset of [`INT_MASK_ALL`], `flags` only [`SF_REF_FRAME`].
pub fn pack_start_stream(int_mask: u8, flags: u8) -> Result<[u8; 2], CodecError> {
    if int_mask == 0 || int_mask > INT_MASK_ALL {
        return Err(CodecError(
            "TMF_START_STREAM: int_mask must be a non-zero subset of 0x0F",
        ));
    }
    if flags & !SF_REF_FRAME != 0 {
        return Err(CodecError(
            "TMF_START_STREAM: flags: only SF_REF_FRAME (0x01) is defined",
        ));
    }
    Ok([int_mask, flags])
}

/// `TMF_STOP_STREAM` payload (empty).
pub fn pack_stop_stream() -> [u8; 0] {
    []
}

/// `TMF_GET_INFO` payload (empty).
pub fn pack_get_info() -> [u8; 0] {
    []
}

fn u32_at(b: &[u8], i: usize) -> u32 {
    u32::from_le_bytes(b[i..i + 4].try_into().unwrap())
}

fn u64_at(b: &[u8], i: usize) -> u64 {
    u64::from_le_bytes(b[i..i + 8].try_into().unwrap())
}

/// A decoded `RPT_TMF_REG_DATA`: register bytes and the MCU time the SPI
/// read completed. The echoed cmd byte is checked (must be 0x32).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RegData {
    pub timestamp_us: u64,
    pub data: Vec<u8>,
}

impl RegData {
    pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError> {
        if payload.len() < REG_DATA_HEADER_SIZE {
            return Err(CodecError("RPT_TMF_REG_DATA must be at least 9 bytes"));
        }
        if payload[0] != Tmf8829Cmd::ReadReg as u8 {
            return Err(CodecError("RPT_TMF_REG_DATA: echoed cmd is not 0x32"));
        }
        Ok(RegData {
            timestamp_us: u64_at(payload, 1),
            data: payload[REG_DATA_HEADER_SIZE..].to_vec(),
        })
    }
}

/// A decoded `RPT_TMF_INFO` (25 B, no echoed cmd byte): bridge state only,
/// the sensor is never touched. `int_edges` runs from MCU power-up; the frame
/// counters restart at `TMF_START_STREAM`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Tmf8829Info {
    pub int_edges: u32,
    pub frames: u32,
    pub frames_aborted: u32,
    pub frame_errors: u32,
    pub spi_timeouts: u32,
    /// INT pin level now, 0 = asserted.
    pub int_level: u8,
    pub streaming: bool,
    pub int_mask: u8,
    pub last_int_status: u8,
    pub last_fifo_status: u8,
}

impl Tmf8829Info {
    pub fn unpack(payload: &[u8]) -> Result<Tmf8829Info, CodecError> {
        if payload.len() < INFO_SIZE {
            return Err(CodecError("RPT_TMF_INFO must be 25 bytes"));
        }
        Ok(Tmf8829Info {
            int_edges: u32_at(payload, 0),
            frames: u32_at(payload, 4),
            frames_aborted: u32_at(payload, 8),
            frame_errors: u32_at(payload, 12),
            spi_timeouts: u32_at(payload, 16),
            int_level: payload[20],
            streaming: payload[21] != 0,
            int_mask: payload[22],
            last_int_status: payload[23],
            last_fifo_status: payload[24],
        })
    }
}

/// One `RPT_TMF_FRAME` chunk. `timestamp_us` + the frame's pre-header
/// SYSTICK are a clock pair (not the capture time); `edge_us` is the INT
/// falling edge credited to the frame, 0 = none. `timestamp_us`,
/// `full_size`, `frame_seq` and `edge_us` are the same in every chunk of a
/// frame.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FrameChunk {
    pub timestamp_us: u64,
    pub full_size: u16,
    pub offset: u16,
    pub frame_seq: u8,
    pub edge_us: u64,
    pub data: Vec<u8>,
}

impl FrameChunk {
    pub fn unpack(payload: &[u8]) -> Result<FrameChunk, CodecError> {
        if payload.len() < FRAME_CHUNK_HEADER_SIZE {
            return Err(CodecError("RPT_TMF_FRAME must be at least 21 bytes"));
        }
        Ok(FrameChunk {
            timestamp_us: u64_at(payload, 0),
            full_size: u16::from_le_bytes([payload[8], payload[9]]),
            offset: u16::from_le_bytes([payload[10], payload[11]]),
            frame_seq: payload[12],
            edge_us: u64_at(payload, 13),
            data: payload[FRAME_CHUNK_HEADER_SIZE..].to_vec(),
        })
    }
}

/// A decoded `RPT_TMF_EVENT` (12 B). `int_status` is INT_STATUS as read
/// (unmasked); `code` is [`EV_INT`] or [`EV_BAD_HEADER`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Tmf8829Event {
    pub timestamp_us: u64,
    pub int_status: u8,
    pub code: u8,
    /// Header id byte (BAD_HEADER), else 0.
    pub frame_id: u8,
    /// Pre-header byte (BAD_HEADER), else 0.
    pub fifo_status: u8,
}

impl Tmf8829Event {
    pub fn unpack(payload: &[u8]) -> Result<Tmf8829Event, CodecError> {
        if payload.len() < EVENT_SIZE {
            return Err(CodecError("RPT_TMF_EVENT must be 12 bytes"));
        }
        Ok(Tmf8829Event {
            timestamp_us: u64_at(payload, 0),
            int_status: payload[8],
            code: payload[9],
            frame_id: payload[10],
            fifo_status: payload[11],
        })
    }
}

/// A reassembled frame: the sensor bytes from register 0xFA on.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RawFrame {
    pub timestamp_us: u64,
    pub edge_us: u64,
    pub frame_seq: u8,
    pub data: Vec<u8>,
}

/// Header fields of the frame being rebuilt (from its offset-0 chunk).
#[derive(Debug, Clone, Copy)]
struct Head {
    timestamp_us: u64,
    edge_us: u64,
    full_size: u16,
    frame_seq: u8,
}

/// Rebuilds frames from `RPT_TMF_FRAME` chunks (contract 14 §2): reset on
/// offset 0; append only a chunk of the same `frame_seq` (and `full_size`)
/// at the expected offset; complete at `full_size`. Never stitches chunks of
/// different `frame_seq`. `seq_gaps` counts frames the bridge dropped
/// (`frame_seq` jumps, u8 wrap). Mirrors the Python `FrameReassembler`.
#[derive(Debug, Default)]
pub struct FrameReassembler {
    pub completed: u64,
    pub discarded: u64,
    pub seq_gaps: u64,
    buf: Vec<u8>,
    head: Option<Head>,
    next_seq: Option<u8>,
}

impl FrameReassembler {
    pub fn new() -> Self {
        FrameReassembler::default()
    }

    /// Forget everything — call at every stream start (`frame_seq` restarts at 0).
    pub fn reset(&mut self) {
        *self = FrameReassembler::default();
    }

    /// Feed one chunk; returns the frame it completes.
    pub fn feed(&mut self, chunk: FrameChunk) -> Option<RawFrame> {
        if chunk.offset == 0 {
            if self.head.is_some() {
                self.discarded += 1; // the previous frame never completed
            }
            if let Some(next) = self.next_seq {
                if chunk.frame_seq != next {
                    self.seq_gaps += chunk.frame_seq.wrapping_sub(next) as u64;
                }
            }
            self.next_seq = Some(chunk.frame_seq.wrapping_add(1));
            self.head = Some(Head {
                timestamp_us: chunk.timestamp_us,
                edge_us: chunk.edge_us,
                full_size: chunk.full_size,
                frame_seq: chunk.frame_seq,
            });
            self.buf = chunk.data;
        } else if matches!(self.head, Some(h) if h.frame_seq == chunk.frame_seq
            && h.full_size == chunk.full_size
            && chunk.offset as usize == self.buf.len())
        {
            self.buf.extend_from_slice(&chunk.data);
        } else {
            if self.head.is_some() {
                self.discarded += 1;
            }
            self.head = None;
            self.buf = Vec::new();
            return None;
        }
        let head = self.head.unwrap();
        let full = head.full_size as usize;
        if self.buf.len() == full {
            self.head = None;
            self.completed += 1;
            return Some(RawFrame {
                timestamp_us: head.timestamp_us,
                edge_us: head.edge_us,
                frame_seq: head.frame_seq,
                data: std::mem::take(&mut self.buf),
            });
        }
        if self.buf.len() > full {
            self.discarded += 1;
            self.head = None;
            self.buf = Vec::new();
        }
        None
    }
}
