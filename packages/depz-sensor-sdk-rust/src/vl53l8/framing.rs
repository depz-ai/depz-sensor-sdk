//! VL53L8 streamed-frame chunk codec + reassembler (contract 04).
//!
//! The MCU pushes each sensor frame as one or more `RPT_VL53_FRAME` chunks
//! (payload ≤ 1528 data bytes). Each chunk carries the capture timestamp, the
//! total frame size, and this chunk's byte offset. The reassembler rebuilds
//! full frames: it resets on `offset == 0`, requires contiguous offsets, and
//! completes a frame when the accumulated bytes reach `full_size`.
//!
//! Mirrors the TS/Python `FrameReassembler`.

/// Bytes of frame data carried per `RPT_VL53_FRAME` chunk.
pub const STREAM_CHUNK_MAX: usize = 1528;
/// Largest `frame_size` accepted by `START_STREAM`.
pub const STREAM_TOTAL_MAX: usize = 8192;

/// One `RPT_VL53_FRAME` chunk (header + a slice of the frame).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FrameChunk {
    pub timestamp_us: u64,
    pub full_size: u16,
    pub offset: u16,
    pub data: Vec<u8>,
}

/// Decode an `RPT_VL53_FRAME` payload: `timestamp_us` u64 LE, `full_size` u16
/// LE, `offset` u16 LE, then the chunk data. Returns `None` if the payload is
/// shorter than the 12-byte header.
pub fn unpack_frame_chunk(payload: &[u8]) -> Option<FrameChunk> {
    if payload.len() < 12 {
        return None;
    }
    let timestamp_us = u64::from_le_bytes(payload[0..8].try_into().unwrap());
    let full_size = u16::from_le_bytes([payload[8], payload[9]]);
    let offset = u16::from_le_bytes([payload[10], payload[11]]);
    Some(FrameChunk {
        timestamp_us,
        full_size,
        offset,
        data: payload[12..].to_vec(),
    })
}

/// A completed sensor frame with its capture timestamp.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CompletedFrame {
    pub timestamp_us: u64,
    pub frame: Vec<u8>,
}

/// Rebuilds full sensor frames from chunked `RPT_VL53_FRAME` reports.
#[derive(Debug, Default)]
pub struct FrameReassembler {
    pub completed: u64,
    pub discarded: u64,
    buf: Vec<u8>,
    full_size: usize,
    timestamp_us: u64,
}

impl FrameReassembler {
    pub fn new() -> Self {
        FrameReassembler::default()
    }

    /// Feed one chunk; returns a `CompletedFrame` when a frame finishes.
    pub fn feed(&mut self, chunk: FrameChunk) -> Option<CompletedFrame> {
        let full_size = chunk.full_size as usize;
        if chunk.offset == 0 {
            // A new frame starts. Any partial in progress is abandoned.
            if !self.buf.is_empty() && self.buf.len() != self.full_size {
                self.discarded += 1;
            }
            self.buf = chunk.data;
            self.full_size = full_size;
            self.timestamp_us = chunk.timestamp_us;
        } else if chunk.offset as usize == self.buf.len()
            && self.full_size == full_size
            && !self.buf.is_empty()
        {
            self.buf.extend_from_slice(&chunk.data);
        } else {
            // Non-contiguous / mismatched continuation: drop the frame.
            if !self.buf.is_empty() {
                self.discarded += 1;
            }
            self.buf.clear();
            self.full_size = 0;
            return None;
        }

        if self.buf.len() == self.full_size && self.full_size > 0 {
            let frame = std::mem::take(&mut self.buf);
            self.full_size = 0;
            self.completed += 1;
            return Some(CompletedFrame {
                timestamp_us: self.timestamp_us,
                frame,
            });
        }
        if self.buf.len() > self.full_size {
            self.discarded += 1;
            self.buf.clear();
            self.full_size = 0;
        }
        None
    }
}
