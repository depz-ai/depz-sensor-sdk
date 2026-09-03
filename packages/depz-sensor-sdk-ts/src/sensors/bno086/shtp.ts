/**
 * SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).
 *
 * Pure codec — no I/O. A frame is a 4-byte header plus a cargo fragment:
 *
 *     length u16 LE  — bits 14:0 cargo length *including* the 4-byte header;
 *                      bit 15 set marks a continuation fragment
 *     channel u8     — see ShtpChannel
 *     seq u8         — per-channel, per-direction free-running counter
 *
 * For a cargo that spans several bridge frames, the first fragment's length
 * field carries the TOTAL cargo length (header included) even though the
 * frame itself holds fewer bytes; each continuation fragment carries the
 * remaining length (its own header included) with bit 15 set. The receiver
 * trusts the first fragment's total and the actual frame sizes; continuation
 * length fields are informative only.
 *
 * Mirrors the Python reference `depz_sensor_sdk.bno086.shtp`.
 */

export enum ShtpChannel {
  /** SHTP command channel (advertisements). */
  Command = 0,
  /** Device executable: reset/on/sleep; RX 0x01 = reset done. */
  Executable = 1,
  /** SH-2 control: feature/FRS/command reports. */
  Control = 2,
  /** Non-wake input reports (0xFB timebase + sensors). */
  InputNormal = 3,
  /** Wake input reports (same cargo format as channel 3). */
  InputWake = 4,
  /** Gyro-integrated rotation vector, dense format. */
  GyroRv = 5,
}

export const SHTP_HEADER_SIZE = 4;
export const LENGTH_MASK = 0x7fff;
export const CONTINUATION_BIT = 0x8000;
export const NUM_CHANNELS = 6;

/** Host->sensor frames must fit one MCU transmit slot (ERRATA E2: 2 x 64 B). */
export const MAX_TX_FRAME = 64;

export interface ShtpHeader {
  /** Bits 14:0 — cargo length incl. this 4-byte header. */
  length: number;
  channel: number;
  seq: number;
  continuation: boolean;
}

export function packShtpHeader(hdr: ShtpHeader): Uint8Array {
  const word = (hdr.length & LENGTH_MASK) | (hdr.continuation ? CONTINUATION_BIT : 0);
  const out = new Uint8Array(SHTP_HEADER_SIZE);
  const v = new DataView(out.buffer);
  v.setUint16(0, word, true);
  v.setUint8(2, hdr.channel);
  v.setUint8(3, hdr.seq);
  return out;
}

export function unpackShtpHeader(data: Uint8Array): ShtpHeader {
  const v = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const word = v.getUint16(0, true);
  return {
    length: word & LENGTH_MASK,
    channel: v.getUint8(2),
    seq: v.getUint8(3),
    continuation: (word & CONTINUATION_BIT) !== 0,
  };
}

/** One reassembled cargo: `payload` excludes all SHTP headers. */
export interface ShtpCargo {
  channel: number;
  /** seq of the first fragment. */
  seq: number;
  payload: Uint8Array;
}

/** Single-fragment frame: length = header + payload. */
export function buildFrame(channel: number, payload: Uint8Array, seq: number): Uint8Array {
  const hdr = packShtpHeader({
    length: SHTP_HEADER_SIZE + payload.length,
    channel,
    seq: seq & 0xff,
    continuation: false,
  });
  const out = new Uint8Array(hdr.length + payload.length);
  out.set(hdr, 0);
  out.set(payload, hdr.length);
  return out;
}

/**
 * Split a cargo into wire frames of at most `maxFrame` bytes.
 *
 * First fragment advertises the TOTAL cargo length; continuations carry the
 * remaining length with the continuation bit set. seq increments per frame.
 */
export function fragmentCargo(
  channel: number,
  payload: Uint8Array,
  seqStart: number,
  maxFrame: number = MAX_TX_FRAME,
): Uint8Array[] {
  if (maxFrame <= SHTP_HEADER_SIZE) {
    throw new Error("maxFrame must exceed the 4-byte SHTP header");
  }
  const room = maxFrame - SHTP_HEADER_SIZE;
  const frames: Uint8Array[] = [];
  let off = 0;
  let seq = seqStart & 0xff;
  const total = SHTP_HEADER_SIZE + payload.length;
  for (;;) {
    const chunk = payload.subarray(off, off + room);
    const remaining = total - off; // includes one header
    const hdr = packShtpHeader({ length: remaining, channel, seq, continuation: off > 0 });
    const frame = new Uint8Array(hdr.length + chunk.length);
    frame.set(hdr, 0);
    frame.set(chunk, hdr.length);
    frames.push(frame);
    off += chunk.length;
    seq = (seq + 1) & 0xff;
    if (off >= payload.length) return frames;
  }
}

interface ChannelRx {
  chunks: Uint8Array[];
  received: number;
  /** Total cargo payload bytes (headers excluded). */
  expected: number;
  seq: number;
}

function emptyRx(): ChannelRx {
  return { chunks: [], received: 0, expected: 0, seq: 0 };
}

function concatChunks(chunks: Uint8Array[], total: number): Uint8Array {
  const out = new Uint8Array(total);
  let off = 0;
  for (const c of chunks) {
    out.set(c, off);
    off += c.length;
  }
  return out;
}

/**
 * Per-channel TX sequence counters + RX cargo reassembly.
 *
 * Feed every inbound frame (the RPT_DATA payload after cmd/timestamp) to
 * `feed()`; it returns a completed ShtpCargo or null. Build outbound frames
 * with `nextFrame()` which consumes the channel's TX seq. The device layer
 * serializes access.
 */
export class ShtpLayer {
  /** Incomplete cargos thrown away. */
  discarded = 0;

  private txSeqs: number[] = new Array<number>(NUM_CHANNELS).fill(0);
  private rx: ChannelRx[] = Array.from({ length: NUM_CHANNELS }, emptyRx);

  // ── TX ─────────────────────────────────────────────────────────────────────

  /**
   * Build a single-fragment frame, consuming the channel's TX seq.
   *
   * Host-side cargos always fit one MCU slot (control payloads are <= 21
   * bytes); larger payloads are a caller bug.
   */
  nextFrame(channel: number, payload: Uint8Array): Uint8Array {
    if (SHTP_HEADER_SIZE + payload.length > MAX_TX_FRAME) {
      throw new Error(`TX cargo ${payload.length}B exceeds the ${MAX_TX_FRAME}B MCU slot`);
    }
    const seq = this.txSeqs[channel]!;
    this.txSeqs[channel] = (seq + 1) & 0xff;
    return buildFrame(channel, payload, seq);
  }

  txSeq(channel: number): number {
    return this.txSeqs[channel]!;
  }

  // ── RX ─────────────────────────────────────────────────────────────────────

  /**
   * Consume one inbound frame; return the cargo when complete.
   *
   * Rules (contract 05 §3): a non-continuation fragment starts a new cargo
   * (discarding any partial one on that channel); a continuation without a
   * cargo in progress is dropped; the cargo completes when the accumulated
   * bytes reach the first fragment's advertised total.
   */
  feed(frame: Uint8Array): ShtpCargo | null {
    if (frame.length < SHTP_HEADER_SIZE) return null;
    const hdr = unpackShtpHeader(frame);
    if (hdr.channel >= NUM_CHANNELS || hdr.length < SHTP_HEADER_SIZE) {
      return null; // empty/padding header ("no data" read) or junk
    }
    const chunk = frame.subarray(SHTP_HEADER_SIZE);
    const rx = this.rx[hdr.channel]!;
    if (!hdr.continuation) {
      if (rx.expected !== 0 && rx.received !== 0) this.discarded += 1;
      rx.chunks = [chunk.slice()];
      rx.received = chunk.length;
      rx.expected = hdr.length - SHTP_HEADER_SIZE;
      rx.seq = hdr.seq;
    } else {
      if (rx.expected === 0) {
        this.discarded += 1;
        return null;
      }
      rx.chunks.push(chunk.slice());
      rx.received += chunk.length;
    }
    if (rx.received < rx.expected) return null;
    if (rx.received > rx.expected) {
      // Overrun — junk framing.
      this.discarded += 1;
      rx.chunks = [];
      rx.received = 0;
      rx.expected = 0;
      return null;
    }
    const cargo: ShtpCargo = {
      channel: hdr.channel,
      seq: rx.seq,
      payload: concatChunks(rx.chunks, rx.received),
    };
    rx.chunks = [];
    rx.received = 0;
    rx.expected = 0;
    return cargo;
  }

  /** Forget all TX seq counters and partial cargos (sensor reset). */
  reset(): void {
    this.txSeqs = new Array<number>(NUM_CHANNELS).fill(0);
    this.rx = Array.from({ length: NUM_CHANNELS }, emptyRx);
  }
}
