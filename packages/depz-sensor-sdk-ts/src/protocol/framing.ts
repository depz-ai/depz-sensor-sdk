/**
 * Packet framing and incremental parser (contracts/01_TRANSPORT_FRAMING.md).
 * Mirrors the Python reference (`depz_sensor_sdk.transport.framing`) and the
 * firmware `common/transport/transport.c`. Empty payloads never carry CRC
 * bytes even when crc_type bits are set (contracts/ERRATA.md E6).
 */

import { crc8Maxim, crc16Modbus, crc32IsoHdlc } from "./crc.js";

export const MAGIC0 = 0xa5;
export const MAGIC1 = 0xc3;
export const HEADER_SIZE = 7;
export const MAX_PAYLOAD = 0x3fff;

export enum CrcType {
  None = 0,
  Crc8 = 1,
  Crc16 = 2,
  Crc32 = 3,
}

const CRC_SIZES: Record<CrcType, number> = {
  [CrcType.None]: 0,
  [CrcType.Crc8]: 1,
  [CrcType.Crc16]: 2,
  [CrcType.Crc32]: 4,
};

/** CRC trailer for a payload; empty payloads never carry CRC bytes. */
export function payloadCrcBytes(crcType: CrcType, payload: Uint8Array): Uint8Array {
  if (crcType === CrcType.None || payload.length === 0) return new Uint8Array(0);
  const out = new Uint8Array(CRC_SIZES[crcType]);
  const dv = new DataView(out.buffer);
  if (crcType === CrcType.Crc8) dv.setUint8(0, crc8Maxim(payload));
  else if (crcType === CrcType.Crc16) dv.setUint16(0, crc16Modbus(payload), true);
  else dv.setUint32(0, crc32IsoHdlc(payload), true);
  return out;
}

/**
 * Frame one packet. `crcType` bits are set in the header even for an empty
 * payload (matching device TX), but CRC bytes are only appended for
 * non-empty payloads.
 */
export function buildPacket(
  cmd: number,
  payload: Uint8Array = new Uint8Array(0),
  seq = 0,
  crcType: CrcType = CrcType.None,
): Uint8Array {
  if (payload.length > MAX_PAYLOAD) {
    throw new RangeError(`payload too long: ${payload.length} > ${MAX_PAYLOAD}`);
  }
  const trailer = payloadCrcBytes(crcType, payload);
  const out = new Uint8Array(HEADER_SIZE + payload.length + trailer.length);
  const dataSize = payload.length | (crcType << 14);
  out[0] = MAGIC0;
  out[1] = MAGIC1;
  out[2] = dataSize & 0xff;
  out[3] = dataSize >>> 8;
  out[4] = cmd & 0xff;
  out[5] = seq & 0xff;
  out[6] = crc8Maxim(out.subarray(2, 6));
  out.set(payload, HEADER_SIZE);
  out.set(trailer, HEADER_SIZE + payload.length);
  return out;
}

export interface PacketEvent {
  type: "packet";
  cmd: number;
  seq: number;
  payload: Uint8Array;
}

/**
 * Bytes discarded while hunting for a valid frame. Boundaries between
 * consecutive trash events depend on read chunking; only the concatenated
 * byte stream is deterministic.
 */
export interface TrashEvent {
  type: "trash";
  data: Uint8Array;
}

/** A frame with a valid header whose payload CRC failed; dropped. */
export interface CrcErrorEvent {
  type: "crcError";
  cmd: number;
  seq: number;
}

export type ParserEvent = PacketEvent | TrashEvent | CrcErrorEvent;

/**
 * Incremental frame parser. Feed arbitrary byte chunks; get events.
 * Event order is invariant to chunking (contract 01 §5) except trash event
 * boundaries — concatenate trash data when comparing streams.
 */
export class PacketParser {
  private buf = new Uint8Array(0);
  packets = 0;
  crcErrors = 0;
  headerErrors = 0;
  trashBytes = 0;

  feed(data: Uint8Array): ParserEvent[] {
    if (data.length > 0) {
      const next = new Uint8Array(this.buf.length + data.length);
      next.set(this.buf, 0);
      next.set(data, this.buf.length);
      this.buf = next;
    }
    const out: ParserEvent[] = [];
    for (;;) {
      const ev = this.parseOne();
      if (ev === null) break;
      out.push(ev);
    }
    return out;
  }

  /** Unconsumed bytes currently buffered (diagnostics/tests). */
  get residue(): Uint8Array {
    return this.buf;
  }

  private findMagic(from = 0): number {
    const b = this.buf;
    for (let i = from; i + 1 < b.length; i++) {
      if (b[i] === MAGIC0 && b[i + 1] === MAGIC1) return i;
    }
    return -1;
  }

  private emitTrash(count: number): TrashEvent {
    const data = this.buf.slice(0, count);
    this.buf = this.buf.slice(count);
    this.trashBytes += count;
    return { type: "trash", data };
  }

  private parseOne(): ParserEvent | null {
    const buf = this.buf;
    const pos = this.findMagic();
    if (pos === -1) {
      // Keep the last byte: it may be a split 0xA5.
      if (buf.length > 1) return this.emitTrash(buf.length - 1);
      return null;
    }
    if (pos > 0) return this.emitTrash(pos);
    if (buf.length < HEADER_SIZE) return null;
    const dataSize = buf[2]! | (buf[3]! << 8);
    const payloadSize = dataSize & MAX_PAYLOAD;
    const crcType = ((dataSize >>> 14) & 0x03) as CrcType;
    if (crc8Maxim(buf.subarray(2, 6)) !== buf[6]) {
      // Header corrupt: advance one byte past the magic start and let the
      // magic hunt resync (firmware: rb_skip(off + 1)).
      this.headerErrors += 1;
      return this.emitTrash(1);
    }
    // ERRATA E6: empty payload never carries CRC bytes.
    const crcSize = payloadSize > 0 ? CRC_SIZES[crcType] : 0;
    const total = HEADER_SIZE + payloadSize + crcSize;
    if (buf.length < total) return null;
    const cmd = buf[4]!;
    const seq = buf[5]!;
    const payload = buf.slice(HEADER_SIZE, HEADER_SIZE + payloadSize);
    const trailer = buf.subarray(HEADER_SIZE + payloadSize, total);
    this.buf = this.buf.slice(total);
    if (crcSize > 0) {
      const expected = payloadCrcBytes(crcType, payload);
      for (let i = 0; i < crcSize; i++) {
        if (expected[i] !== trailer[i]) {
          this.crcErrors += 1;
          return { type: "crcError", cmd, seq };
        }
      }
    }
    this.packets += 1;
    return { type: "packet", cmd, seq, payload };
  }
}
