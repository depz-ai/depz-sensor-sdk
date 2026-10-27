/**
 * In-process fake of the VL53L register-bridge firmware over a
 * LoopbackTransport pair: `APP_VL53L0_4` (contract 12, protocol v2.01) or the
 * older `APP_VL53L4` (contract 10, v1.00). The MCU side knows no sensor, so
 * neither does this fake: behind the bridge sits a 64 KiB register file
 * pre-loaded with what a booted VL53L1-family die answers (boot status 3,
 * oscillator, model id, PLL), enough for the host ULDs (L4CD ULD, L1X ULD) to
 * run their init, timing, offset, xtalk, threshold, calibration and ranging
 * sequences against it.
 *
 * Every packet the host sends is logged (`rx`), so tests assert the exact wire
 * bytes. `failNext(cmd, status)` answers the next `cmd` with an error status
 * (or `"silent"`: no reply), `stream()` pushes an RPT_VL53_STREAM.
 *
 * The result block (0x0089, 17 B) and the data-ready bit (0x0031 bit 0) are
 * plain registers; the INT polarity bit of 0x0030 is set by the blob.
 *
 * A `die` model (fake-vl53l0x-die.ts) can take over the register file: the
 * bridge hands it every READ_REG / WRITE_REG, and a `DieNack` it throws goes
 * back as ERR_HARDWARE_FAULT, the status a NACKing sensor produces.
 */

import {
  CrcType,
  LoopbackTransport,
  PacketParser,
  buildPacket,
  type PacketEvent,
} from "../src/index.js";

export interface RxPacket {
  cmd: number;
  payload: Uint8Array;
}

export interface FakeVl53lxBridgeOptions {
  /** "vlx" = APP_VL53L0_4 v2.01 (default), "l4" = APP_VL53L4 v1.00. */
  firmware?: "vlx" | "l4";
  /** Bootloader device name (product read from it). */
  deviceName?: string;
  /** VL53_CLEAR_I2C_ERRORS unknown (a v2.00 bridge, firmware ≤ v0.23). */
  noClearI2cErrors?: boolean;
}

/** Thrown by a die model: the sensor did not ACK the register access. */
export class DieNack extends Error {}

/** Register behaviour behind the bridge, in place of the flat register file. */
export interface DieModel {
  read(addr: number, n: number): Uint8Array;
  write(addr: number, data: Uint8Array): void;
}

const ERR_HARDWARE_FAULT = 0x09;

export const DIE_MODEL_ID = 0xebaa;
export const DIE_OSC = 0xbccc;
export const DIE_PLL = 0x0123;

export class FakeVl53lxBridge {
  readonly transport: LoopbackTransport;
  readonly side: LoopbackTransport;
  readonly regs = new Uint8Array(0x10000);
  readonly rx: RxPacket[] = [];
  /** Bytes to drop from the next READ_REG answer (a short reply). */
  truncateNextRead = 0;
  readonly firmware: "vlx" | "l4";
  deviceName: string;
  mcuTimeUs = 5_000_000n;
  /** RPT_VL53_INFO payload served on GET_INFO. */
  info: Uint8Array;
  /** Called on every register write, after it lands. */
  onWrite: ((addr: number, data: Uint8Array) => void) | null = null;
  /** Optional register-behaviour model (read / write) replacing `regs`. */
  die: DieModel | null = null;

  private readonly failures = new Map<number, Array<number | "silent">>();
  private readonly noClear: boolean;
  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor(opts: FakeVl53lxBridgeOptions = {}) {
    this.firmware = opts.firmware ?? "vlx";
    this.deviceName = opts.deviceName ?? "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL";
    this.noClear = opts.noClearI2cErrors ?? false;
    this.info = new Uint8Array(this.firmware === "vlx" ? 23 : 21);
    // A booted die.
    this.regs[0x00e5] = 0x03;
    this.setWord(0x0006, DIE_OSC);
    this.setWord(0x010f, DIE_MODEL_ID);
    this.setWord(0x00de, DIE_PLL);
    this.regs[0x013e] = 0x91; // ROI_CONFIG__MODE_ROI_CENTRE_SPAD
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  // ── register file ──
  setWord(addr: number, v: number): void {
    this.regs[addr] = (v >> 8) & 0xff;
    this.regs[addr + 1] = v & 0xff;
  }

  word(addr: number): number {
    return (this.regs[addr]! << 8) | this.regs[addr + 1]!;
  }

  dword(addr: number): number {
    return (
      ((this.regs[addr]! << 24) | (this.regs[addr + 1]! << 16) | (this.regs[addr + 2]! << 8) | this.regs[addr + 3]!) >>>
      0
    );
  }

  /** Put a 17-byte result block at 0x0089 (what the next poll reads). */
  setResultBlock(raw: Uint8Array): void {
    this.regs.set(raw, 0x0089);
  }

  // ── traffic ──
  failNext(cmd: number, status: number | "silent"): void {
    const q = this.failures.get(cmd) ?? [];
    q.push(status);
    this.failures.set(cmd, q);
  }

  /** Packets of one opcode, in order. */
  sent(cmd: number): Uint8Array[] {
    return this.rx.filter((p) => p.cmd === cmd).map((p) => p.payload);
  }

  /** Every WRITE_REG as [addr, data]. */
  writes(): Array<[number, number[]]> {
    return this.sent(0x33).map((p) => [p[0]! | (p[1]! << 8), [...p.slice(2)]]);
  }

  /** RPT_VL53_STREAM: ts u64, addr u16, len u16, data. */
  async stream(data: Uint8Array, tsUs: bigint, addr = 0x0089, length = data.length): Promise<void> {
    const out = new Uint8Array(12 + data.length);
    const dv = new DataView(out.buffer);
    dv.setBigUint64(0, tsUs, true);
    dv.setUint16(8, addr, true);
    dv.setUint16(10, length, true);
    out.set(data, 12);
    await this.send(0x93, out);
  }

  /** A raw packet from the device (for malformed-report tests). */
  sendRaw(cmd: number, payload: Uint8Array): Promise<void> {
    return this.send(cmd, payload);
  }

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return; // link closed
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  private text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    return this.send(0x81, payload);
  }

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type === "packet") await this.handle(ev);
      }
    }
  }

  private async handle(pkt: PacketEvent): Promise<void> {
    const { cmd, payload } = pkt;
    this.rx.push({ cmd, payload: payload.slice() });
    const fail = this.failures.get(cmd)?.shift();
    if (fail === "silent") return;
    if (fail !== undefined) return this.status(cmd, fail);
    const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
    switch (cmd) {
      case 0x03:
        return this.text(cmd, this.deviceName);
      case 0x04:
        return this.text(cmd, this.firmware === "vlx" ? "APP_VL53L0_4_v0.25" : "APP_VL53L4_v1.0");
      case 0x32: {
        // READ_REG → RPT_VL53_REG_DATA: echoed opcode, ts u64, data
        const addr = dv.getUint16(0, true);
        const len = dv.getUint16(2, true);
        let data: Uint8Array;
        try {
          data = this.die !== null ? this.die.read(addr, len) : this.regs.slice(addr, addr + len);
        } catch (err) {
          if (err instanceof DieNack) return this.status(cmd, ERR_HARDWARE_FAULT);
          throw err;
        }
        if (this.truncateNextRead) {
          data = data.slice(0, data.length - this.truncateNextRead);
          this.truncateNextRead = 0;
        }
        const out = new Uint8Array(9 + data.length);
        out[0] = cmd;
        new DataView(out.buffer).setBigUint64(1, this.mcuTimeUs, true);
        out.set(data, 9);
        this.mcuTimeUs += 100n;
        return this.send(0x91, out);
      }
      case 0x33: {
        const addr = dv.getUint16(0, true);
        const data = payload.slice(2);
        if (this.die !== null) {
          try {
            this.die.write(addr, data);
          } catch (err) {
            if (err instanceof DieNack) return this.status(cmd, ERR_HARDWARE_FAULT);
            throw err;
          }
        } else {
          this.regs.set(data, addr);
        }
        this.onWrite?.(addr, data);
        return this.status(cmd, 0);
      }
      case 0x37:
        return this.send(0x92, this.info);
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x38:
        return this.status(cmd, 0);
      case 0x39:
        return this.status(cmd, this.firmware === "vlx" ? 0 : 0x02);
      case 0x3a:
        return this.status(cmd, this.firmware === "vlx" && !this.noClear ? 0 : 0x02);
      default:
        return this.status(cmd, 0x02); // ERR_INVALID_CMD
    }
  }
}

/** "0a0b" → bytes. */
export function hex(s: string): Uint8Array {
  return Uint8Array.from(s.match(/../g)!.map((b) => parseInt(b, 16)));
}
