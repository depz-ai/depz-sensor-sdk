/**
 * Fake VL53L8 register/DCI platform for host-side ULD tests (no hardware).
 * Port of the Python `tests/fake_vl53l8.py`.
 *
 * Emulates the register bridge + DCI transport (swapBuffer + header/footer
 * framing) so the real byte sequences in uld.ts are exercised end to end.
 */

import {
  FW_CHECKSUM,
  OFFSET_BUFFER_SIZE,
  VL53L8CX,
  XTALK_BUFFER_SIZE,
  swapBuffer,
  type Vl53l8Platform,
} from "../src/sensors/vl53l8/uld.js";

const UI_CMD_STATUS = 0x2c00;
const UI_CMD_START = 0x2c04;
const UI_CMD_END = 0x2fff;

export class FakeUldPlatform implements Vl53l8Platform {
  readonly reg = new Map<number, number>();
  readonly dci = new Map<number, Uint8Array>();
  /** Every wrMulti (addr, bytes) — for transcript assertions. */
  readonly writes: Array<[number, Uint8Array]> = [];
  private pendingRead: [number, number] | null = null;

  sleepMs(): Promise<void> {
    return Promise.resolve();
  }

  wrMulti(addr: number, data: Uint8Array): Promise<void> {
    const buf = data.slice();
    this.writes.push([addr, buf]);
    // DCI read command: 12 bytes written to UI_CMD_END-11 (0x2FF4).
    if (addr === UI_CMD_END - 11 && buf.length === 12) {
      const index = (buf[0]! << 8) | buf[1]!;
      const size = ((buf[2]! & 0xff) << 4) | ((buf[3]! & 0xff) >> 4);
      this.pendingRead = [index, size];
      return Promise.resolve();
    }
    // DCI write: header(4) + swap(payload) + footer(8); footer sig 0F 05 01.
    const n = buf.length;
    if (n >= 12 && buf[n - 5] === 0x0f && buf[n - 4] === 0x05 && buf[n - 3] === 0x01) {
      const index = (buf[0]! << 8) | buf[1]!;
      const dataSize = ((buf[2]! & 0xff) << 4) | ((buf[3]! & 0xff) >> 4);
      this.dci.set(index, swapBuffer(buf.subarray(4, 4 + dataSize)));
      return Promise.resolve();
    }
    // plain byte writes → registers
    for (let i = 0; i < n; i++) this.reg.set(addr + i, buf[i]!);
    return Promise.resolve();
  }

  rdMulti(addr: number, size: number): Promise<Uint8Array> {
    if (addr === UI_CMD_STATUS) {
      const out = new Uint8Array(size);
      const base = [0x00, 0x03, 0x00, 0x00];
      for (let i = 0; i < Math.min(size, 4); i++) out[i] = base[i]!;
      return Promise.resolve(out);
    }
    if (addr === UI_CMD_START && this.pendingRead !== null) {
      const [index, dsize] = this.pendingRead;
      const stored = this.dci.get(index) ?? new Uint8Array(dsize);
      const s = new Uint8Array(dsize);
      s.set(stored.subarray(0, dsize));
      const raw = new Uint8Array(dsize + 12);
      raw.set(s, 4);
      return Promise.resolve(swapBuffer(raw));
    }
    const out = new Uint8Array(size);
    for (let i = 0; i < size; i++) out[i] = this.reg.get(addr + i) ?? 0;
    return Promise.resolve(out);
  }
}

/** Construct a VL53L8CX bypassing blob-file init (mirrors Python make_driver). */
export function makeDriver(): { drv: VL53L8CX; p: FakeUldPlatform } {
  const p = new FakeUldPlatform();
  const drv = Object.create(VL53L8CX.prototype) as VL53L8CX;
  const a = drv as unknown as Record<string, unknown>;
  a.p = p;
  a.variant = "cx";
  a.fwChecksum = FW_CHECKSUM.cx;
  a.outputEnableW3 = 0xc0000000;
  a.frameTail = 32;
  a.footerIdOff = 12;
  a.motionPresent = false;
  a.defaultXtalk = new Uint8Array(XTALK_BUFFER_SIZE);
  a.offsetData = new Uint8Array(OFFSET_BUFFER_SIZE);
  a.xtalkData = new Uint8Array(XTALK_BUFFER_SIZE);
  a.streamcount = 255;
  a.dataReadSize = 0;
  a.lastBlocks = [];
  return { drv, p };
}
