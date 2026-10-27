/**
 * VL53L8 register-bridge wire codecs (contracts/04_SENSOR_VL53L8.md).
 * Mirrors the Python reference `depz_sensor_sdk.protocol.vl53l8`.
 */

import { checkUint, needBytes } from "./wire.js";

export enum Vl53l8Cmd {
  ReadReg = 0x32,
  WriteReg = 0x33,
  // 0x34 intentionally unused (gap in the firmware's ID sequence)
  StartStream = 0x35,
  StopStream = 0x36,
}

export enum Vl53l8Rpt {
  RegData = 0x91,
  Vl53Frame = 0x93,
}

// MCU TRANSPORT_PAYLOAD_BUF_SIZE = 2304:
//   WRITE_REG payload = addr(2) + N  → N ≤ 2302
//   RPT_REG_DATA      = hdr(9) + N   → N ≤ 2295
// The reference tool uses a round 2048 for both directions.
export const READ_MAX_LEN = 2295;
export const CHUNK_SIZE = 2048;
/** Bytes of frame data per RPT_VL53_FRAME chunk. */
export const STREAM_CHUNK_MAX = 1528;
/** Max frame_size accepted by START_STREAM. */
export const STREAM_TOTAL_MAX = 8192;

export function packReadReg(addr: number, length: number): Uint8Array {
  const out = new Uint8Array(4);
  const dv = new DataView(out.buffer);
  dv.setUint16(0, checkUint("addr", addr, 16), true);
  dv.setUint16(2, checkUint("length", length, 16), true);
  return out;
}

export function packWriteReg(addr: number, data: Uint8Array): Uint8Array {
  checkUint("addr", addr, 16);
  // contract 04 §2: the payload is addr + at least one data byte
  if (data.length < 1) throw new RangeError("WRITE_REG data must be at least 1 byte");
  const out = new Uint8Array(2 + data.length);
  new DataView(out.buffer).setUint16(0, addr, true);
  out.set(data, 2);
  return out;
}

export function packStartStream(frameSize: number): Uint8Array {
  const out = new Uint8Array(2);
  new DataView(out.buffer).setUint16(0, checkUint("frameSize", frameSize, 16), true);
  // contract 04 §4 rule 4: the firmware refuses 0 and > 8192; so does the host
  if (frameSize < 1 || frameSize > STREAM_TOTAL_MAX) {
    throw new RangeError(`frameSize: ${frameSize} outside 1..${STREAM_TOTAL_MAX}`);
  }
  return out;
}

/** RPT_REG_DATA payload. */
export interface RegData {
  /** Echoed READ_REG opcode. */
  cmd: number;
  timestampUs: bigint;
  data: Uint8Array;
}

export function unpackRegData(payload: Uint8Array): RegData {
  needBytes("RPT_REG_DATA", payload, 9);
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    cmd: dv.getUint8(0),
    timestampUs: dv.getBigUint64(1, true),
    data: payload.slice(9),
  };
}

/** One RPT_VL53_FRAME chunk of a (possibly multi-chunk) sensor frame. */
export interface FrameChunk {
  timestampUs: bigint;
  fullSize: number;
  offset: number;
  data: Uint8Array;
}

export function unpackFrameChunk(payload: Uint8Array): FrameChunk {
  needBytes("RPT_VL53_FRAME", payload, 12);
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    timestampUs: dv.getBigUint64(0, true),
    fullSize: dv.getUint16(8, true),
    offset: dv.getUint16(10, true),
    data: payload.slice(12),
  };
}

/**
 * Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.
 *
 * Rules (contract 04 §4): reset on offset==0; chunks must be contiguous —
 * a gap discards the frame in progress; a frame completes when the
 * accumulated bytes equal `fullSize`. A chunk out of the rule-4 bounds
 * (more than `chunkMax` bytes, `fullSize` over STREAM_TOTAL_MAX, or running
 * past `fullSize`) is damaged: dropped and counted as a lost frame like the
 * other damaged input, never thrown. `chunkMax` is the device's
 * VL53_STREAM_FRAME_MAX (contract 11 §3: 1536 on the I2C board).
 */
export class FrameReassembler {
  completed = 0;
  discarded = 0;

  private buf = new Uint8Array(0);
  private fullSize = 0;
  private timestampUs = 0n;

  constructor(private readonly chunkMax: number = STREAM_CHUNK_MAX) {}

  /** Returns `{ timestampUs, frame }` when a frame completes. */
  feed(chunk: FrameChunk): { timestampUs: bigint; frame: Uint8Array } | null {
    if (
      chunk.data.length > this.chunkMax ||
      chunk.fullSize > STREAM_TOTAL_MAX ||
      chunk.offset + chunk.data.length > chunk.fullSize
    ) {
      // rule 4: damaged — the frame in progress is lost (counted, rules 1–2)
      // and so is the frame an offset-0 chunk starts (counted); a damaged
      // orphan with nothing in progress is dropped like any orphan
      if (this.buf.length > 0) this.discarded += 1;
      if (chunk.offset === 0) this.discarded += 1;
      this.buf = new Uint8Array(0);
      this.fullSize = 0;
      return null;
    }
    if (chunk.offset === 0) {
      if (this.buf.length > 0 && this.buf.length !== this.fullSize) {
        this.discarded += 1;
      }
      this.buf = chunk.data.slice();
      this.fullSize = chunk.fullSize;
      this.timestampUs = chunk.timestampUs;
    } else if (
      chunk.offset === this.buf.length &&
      this.fullSize === chunk.fullSize &&
      this.buf.length > 0
    ) {
      const next = new Uint8Array(this.buf.length + chunk.data.length);
      next.set(this.buf, 0);
      next.set(chunk.data, this.buf.length);
      this.buf = next;
    } else {
      if (this.buf.length > 0) this.discarded += 1;
      this.buf = new Uint8Array(0);
      this.fullSize = 0;
      return null;
    }
    if (this.buf.length === this.fullSize && this.fullSize > 0) {
      const frame = this.buf;
      this.buf = new Uint8Array(0);
      this.fullSize = 0;
      this.completed += 1;
      return { timestampUs: this.timestampUs, frame };
    }
    return null;
  }
}
