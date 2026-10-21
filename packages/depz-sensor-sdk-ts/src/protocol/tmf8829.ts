/**
 * TMF8829 SPI register-bridge wire codecs (contracts/14_SENSOR_TMF8829.md §2).
 * Mirrors the Python reference `depz_sensor_sdk.protocol.tmf8829`.
 *
 * Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
 * TMF8829 payloads: an 8-bit register address and self-sized frames.
 */

export enum Tmf8829Cmd {
  ReadReg = 0x32,
  WriteReg = 0x33,
  StartStream = 0x35,
  StopStream = 0x36,
  GetInfo = 0x37,
}

export enum Tmf8829Rpt {
  RegData = 0x91,
  Info = 0x92,
  Frame = 0x93,
  Event = 0x94,
}

/** RPT_TMF_EVENT.code */
export enum Tmf8829EventCode {
  /** MOTION/PROXIMITY interrupt, no frame; its bits already cleared. */
  Int = 0,
  /** Frame header refused, frame not read (expected once on STOP mid histogram dump). */
  BadHeader = 1,
}

export const TMF8829_READ_MAX = 1024;
export const TMF8829_WRITE_MAX = 1023;
export const TMF8829_CHUNK_MAX = 1024;

export const TMF8829_INT_RESULTS = 0x01;
export const TMF8829_INT_MOTION = 0x02;
export const TMF8829_INT_PROXIMITY = 0x04;
export const TMF8829_INT_HISTOGRAMS = 0x08;
export const TMF8829_INT_MASK_ALL = 0x0f;
/** TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame. */
export const TMF8829_SF_REF_FRAME = 0x01;

export function packTmf8829ReadReg(addr: number, length: number): Uint8Array {
  if (!Number.isInteger(addr) || addr < 0 || addr > 0xff) throw new RangeError("addr must be 0..255");
  if (!Number.isInteger(length) || length < 1 || length > TMF8829_READ_MAX) {
    throw new RangeError(`length must be 1..${TMF8829_READ_MAX}`);
  }
  return Uint8Array.of(addr, length & 0xff, length >> 8);
}

export function packTmf8829WriteReg(addr: number, data: Uint8Array): Uint8Array {
  if (!Number.isInteger(addr) || addr < 0 || addr > 0xff) throw new RangeError("addr must be 0..255");
  if (data.length < 1 || data.length > TMF8829_WRITE_MAX) {
    throw new RangeError(`data must be 1..${TMF8829_WRITE_MAX} bytes`);
  }
  const out = new Uint8Array(1 + data.length);
  out[0] = addr;
  out.set(data, 1);
  return out;
}

export function packTmf8829StartStream(intMask: number, flags = 0): Uint8Array {
  if (!Number.isInteger(intMask) || intMask < 1 || intMask > TMF8829_INT_MASK_ALL) {
    throw new RangeError("intMask must be a non-zero subset of 0x0F");
  }
  if (flags & ~TMF8829_SF_REF_FRAME) throw new RangeError("flags: only SF_REF_FRAME (0x01) is defined");
  return Uint8Array.of(intMask, flags);
}

function view(b: Uint8Array): DataView {
  return new DataView(b.buffer, b.byteOffset, b.byteLength);
}

/** RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed. */
export interface Tmf8829RegData {
  timestampUs: bigint;
  data: Uint8Array;
}

export function unpackTmf8829RegData(p: Uint8Array): Tmf8829RegData {
  if (p.length < 9) throw new RangeError(`RPT_TMF_REG_DATA: need ≥9 bytes, got ${p.length}`);
  if (p[0] !== Tmf8829Cmd.ReadReg) {
    throw new RangeError(`RPT_TMF_REG_DATA: echoed cmd 0x${p[0]!.toString(16)}, expected 0x32`);
  }
  return { timestampUs: view(p).getBigUint64(1, true), data: p.slice(9) };
}

/** RPT_TMF_INFO: bridge state only (no echoed command byte). */
export interface Tmf8829Info {
  intEdges: number;
  frames: number;
  framesAborted: number;
  frameErrors: number;
  spiTimeouts: number;
  /** PA4 level now, 0 = asserted. */
  intLevel: number;
  streaming: boolean;
  intMask: number;
  lastIntStatus: number;
  lastFifoStatus: number;
}

export function unpackTmf8829Info(p: Uint8Array): Tmf8829Info {
  if (p.length < 25) throw new RangeError(`RPT_TMF_INFO: expected 25 bytes, got ${p.length}`);
  const v = view(p);
  return {
    intEdges: v.getUint32(0, true),
    frames: v.getUint32(4, true),
    framesAborted: v.getUint32(8, true),
    frameErrors: v.getUint32(12, true),
    spiTimeouts: v.getUint32(16, true),
    intLevel: p[20]!,
    streaming: p[21] !== 0,
    intMask: p[22]!,
    lastIntStatus: p[23]!,
    lastFifoStatus: p[24]!,
  };
}

/**
 * One RPT_TMF_FRAME chunk. `timestampUs` + the frame's pre-header SYSTICK are
 * a clock pair (not the capture time); `edgeUs` is the INT edge credited to
 * the frame, 0 = none.
 */
export interface Tmf8829FrameChunk {
  timestampUs: bigint;
  fullSize: number;
  offset: number;
  frameSeq: number;
  edgeUs: bigint;
  data: Uint8Array;
}

export function unpackTmf8829FrameChunk(p: Uint8Array): Tmf8829FrameChunk {
  if (p.length < 21) throw new RangeError(`RPT_TMF_FRAME: need ≥21 bytes, got ${p.length}`);
  const v = view(p);
  return {
    timestampUs: v.getBigUint64(0, true),
    fullSize: v.getUint16(8, true),
    offset: v.getUint16(10, true),
    frameSeq: p[12]!,
    edgeUs: v.getBigUint64(13, true),
    data: p.slice(21),
  };
}

/** RPT_TMF_EVENT. `intStatus` is INT_STATUS as read (unmasked). */
export interface Tmf8829Event {
  timestampUs: bigint;
  intStatus: number;
  code: number;
  /** Header id byte (BAD_HEADER), else 0. */
  frameId: number;
  /** Pre-header byte (BAD_HEADER), else 0. */
  fifoStatus: number;
}

export function unpackTmf8829Event(p: Uint8Array): Tmf8829Event {
  if (p.length < 12) throw new RangeError(`RPT_TMF_EVENT: expected 12 bytes, got ${p.length}`);
  return {
    timestampUs: view(p).getBigUint64(0, true),
    intStatus: p[8]!,
    code: p[9]!,
    frameId: p[10]!,
    fifoStatus: p[11]!,
  };
}

/** A reassembled frame: the sensor bytes from register 0xFA on. */
export interface Tmf8829RawFrame {
  timestampUs: bigint;
  edgeUs: bigint;
  frameSeq: number;
  data: Uint8Array;
}

/**
 * Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on offset
 * 0; append only a chunk of the same frame_seq at the expected offset;
 * complete at full_size. Never stitches chunks of different frame_seq.
 * `seqGaps` counts frames the bridge dropped (frame_seq jumps).
 */
export class Tmf8829FrameReassembler {
  completed = 0;
  discarded = 0;
  seqGaps = 0;
  private buf: Uint8Array | null = null;
  private filled = 0;
  private head: Tmf8829FrameChunk | null = null;
  private nextSeq: number | null = null;

  /** Forget everything — call at every stream start (frame_seq restarts at 0). */
  reset(): void {
    this.completed = 0;
    this.discarded = 0;
    this.seqGaps = 0;
    this.buf = null;
    this.filled = 0;
    this.head = null;
    this.nextSeq = null;
  }

  private drop(): void {
    this.head = null;
    this.buf = null;
    this.filled = 0;
  }

  feed(chunk: Tmf8829FrameChunk): Tmf8829RawFrame | null {
    if (chunk.offset === 0) {
      if (this.head !== null) this.discarded += 1; // the previous frame never completed
      if (this.nextSeq !== null && chunk.frameSeq !== this.nextSeq) {
        this.seqGaps += (chunk.frameSeq - this.nextSeq) & 0xff;
      }
      this.nextSeq = (chunk.frameSeq + 1) & 0xff;
      this.head = chunk;
      this.buf = new Uint8Array(Math.max(chunk.fullSize, chunk.data.length));
      this.buf.set(chunk.data, 0);
      this.filled = chunk.data.length;
    } else if (
      this.head !== null &&
      chunk.frameSeq === this.head.frameSeq &&
      chunk.fullSize === this.head.fullSize &&
      chunk.offset === this.filled &&
      this.filled + chunk.data.length <= this.buf!.length
    ) {
      this.buf!.set(chunk.data, this.filled);
      this.filled += chunk.data.length;
    } else {
      if (this.head !== null) this.discarded += 1;
      this.drop();
      return null;
    }
    const head = this.head!;
    if (this.filled === head.fullSize) {
      const frame: Tmf8829RawFrame = {
        timestampUs: head.timestampUs,
        edgeUs: head.edgeUs,
        frameSeq: head.frameSeq,
        data: this.buf!.subarray(0, this.filled),
      };
      this.drop();
      this.completed += 1;
      return frame;
    }
    if (this.filled > head.fullSize) {
      this.discarded += 1;
      this.drop();
    }
    return null;
  }
}
