/**
 * In-process fake APP_BNO055 register bridge + a register-level model of the
 * BNO055 behind it, over a LoopbackTransport pair (contract 13).
 *
 * The bridge side answers READ_REG (RPT_BNO_REG_DATA or RPT_STATUS error),
 * WRITE_REG / RESET / START_STREAM / STOP_STREAM (RPT_STATUS) and GET_INFO
 * (RPT_BNO_INFO). The sensor side models what the host logic depends on:
 * two register pages selected by PAGE_ID, "only CONFIG ↔ mode transitions
 * exist" (§4.1), suspend ignoring OPR_MODE writes (§4.6), the zero-quaternion
 * gap after entering fusion (§5), the self-test leftover SYS_STATUS 4 (§5),
 * SYS_TRIGGER CLK_SEL and INT_STA clear-on-read.
 */

import { CrcType, LoopbackTransport, PacketParser, buildPacket, type PacketEvent } from "../src/index.js";

export interface RegOp {
  op: "r" | "w";
  page: number;
  addr: number;
  /** Bytes written, or the length read (as one byte array of that length). */
  data: Uint8Array;
}

export interface SentCmd {
  cmd: number;
  payload: Uint8Array;
}

const OPR_MODE = 0x3d;
const PWR_MODE = 0x3e;
const SYS_TRIGGER = 0x3f;
const SYS_STATUS = 0x39;
const PAGE_ID = 0x07;
const QUA_DATA = 0x20;
const INT_STA = 0x37;
const ST_RESULT = 0x36;

/** RPT_BNO_INFO of the live board (vectors/bno055.json info_live_board). */
export const LIVE_INFO_HEX =
  "28a0fb320f1103150100000000006f012b09d105000000000000000000000000000000007d00";

function hexBytes(hex: string): Uint8Array {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  return out;
}

export class FakeBno055 {
  readonly transport: LoopbackTransport;
  readonly side: LoopbackTransport;

  /** page0 / page1 register files. */
  readonly regs: [Uint8Array, Uint8Array] = [new Uint8Array(256), new Uint8Array(256)];
  /** Every command the host sent, in order. */
  readonly sent: SentCmd[] = [];
  /** Every register access, with the page in force. */
  readonly ops: RegOp[] = [];

  mcuTimeUs = 5_000_000n;
  info: Uint8Array = hexBytes(LIVE_INFO_HEX);
  /** Answer the next command(s) with this opcode by RPT_STATUS(status). */
  readonly failNext = new Map<number, number[]>();
  /** Answer WRITE_REG to these page-0 registers by RPT_STATUS(status), once each. */
  readonly failWriteReg = new Map<number, number>();
  /** Never answer commands with these opcodes (host times out). */
  readonly silent = new Set<number>();
  /** Answer the next READ_REG with this many bytes fewer than asked. */
  shortReadBy = 0;
  /** Scripted SYS_STATUS values, consumed one per read before the model takes over. */
  sysStatusScript: number[] = [];
  /** QUA_DATA reads that still return zero after a CONFIG → fusion switch. */
  fusionZeroReadsAfterSwitch = 2;
  /** Fusion never starts (quaternion stays zero). */
  fusionStuck = false;
  /** Quaternion a running fusion outputs. */
  fusionQuat = hexBytes("bd2963f9efcf0000");
  /** ST_RESULT after a self-test. */
  selfTestResult = 0x0f;
  /** SYS_ERR after a self-test. */
  selfTestError = 0;
  /** START_STREAM payloads accepted. */
  readonly streams: Uint8Array[] = [];
  streaming = false;
  resetCount = 0;

  private fusionZeroLeft = 0;
  private selfTestLeftover = false;
  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor() {
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    this.powerOn();
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  /** Power-on register values (contract 13 §2/§4.2). */
  powerOn(): void {
    this.regs[0].fill(0);
    this.regs[1].fill(0);
    this.regs[0].set([0xa0, 0xfb, 0x32, 0x0f, 0x11, 0x15, 0x03], 0); // as read on the live board (vectors reg_data_chip_ids)
    this.regs[0][0x3b] = 0x80; // UNIT_SEL: Android orientation
    this.regs[0][0x41] = 0x24; // AXIS_MAP_CONFIG P1
    this.regs[0][ST_RESULT] = 0x0f;
    this.regs[0][0x3a] = 0; // SYS_ERR
    for (let i = 0; i < 16; i++) this.regs[1][0x50 + i] = 0xc0 + i; // UNIQUE_ID
    this.regs[1][0x08] = 0x0d; // ACC_CONFIG reset value
    this.regs[1][0x09] = 0x6d;
    this.regs[1][0x0a] = 0x38;
    this.selfTestLeftover = false;
    this.fusionZeroLeft = 0;
  }

  get page(): number {
    return this.regs[0][PAGE_ID]! & 1;
  }

  get oprMode(): number {
    return this.regs[0][OPR_MODE]! & 0x0f;
  }

  /** Put the model into an operating mode directly (as if a previous session left it there). */
  setMode(mode: number): void {
    this.regs[0][OPR_MODE] = mode;
    if (mode >= 8) this.regs[0].set(this.fusionQuat, QUA_DATA);
  }

  /** Writes only (page, addr, hex) — the shape most assertions want. */
  writes(): string[] {
    return this.ops
      .filter((o) => o.op === "w")
      .map((o) => `${o.page}:${o.addr.toString(16).padStart(2, "0")}=${toHex(o.data)}`);
  }

  /** Emit RPT_BNO_REG_STREAM. */
  pushStream(addr: number, data: Uint8Array, timestampUs = this.mcuTimeUs, lenField = data.length): void {
    const out = new Uint8Array(10 + data.length);
    const dv = new DataView(out.buffer);
    dv.setBigUint64(0, timestampUs, true);
    out[8] = addr;
    out[9] = lenField;
    out.set(data, 10);
    void this.send(0x93, out);
  }

  /** Emit an arbitrary report. */
  pushRaw(cmd: number, payload: Uint8Array): Promise<void> {
    return this.send(cmd, payload);
  }

  // ── device-side TX ─────────────────────────────────────────────────────────

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    const seq = this.txSeq;
    this.txSeq = (seq + 1) & 0xff;
    try {
      await this.side.write(buildPacket(cmd, payload, seq, CrcType.None));
    } catch {
      // link closed
    }
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  // ── device-side RX loop ────────────────────────────────────────────────────

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type === "packet") await this.handle(ev);
      }
    }
  }

  private async handle(pkt: PacketEvent): Promise<void> {
    const { cmd, payload } = pkt;
    this.sent.push({ cmd, payload: payload.slice() });
    if (this.silent.has(cmd)) return;
    const fails = this.failNext.get(cmd);
    if (fails !== undefined && fails.length > 0) {
      await this.status(cmd, fails.shift()!);
      return;
    }
    this.mcuTimeUs += 1000n;
    switch (cmd) {
      case 0x32: {
        if (payload.length !== 2) return this.status(cmd, 0x03);
        const [addr, len] = [payload[0]!, payload[1]!];
        if (len < 1 || len > 128 || addr + len > 0x100) return this.status(cmd, 0x04);
        this.ops.push({ op: "r", page: this.page, addr, data: new Uint8Array(len) });
        let data = this.read(addr, len);
        if (this.shortReadBy > 0) {
          data = data.subarray(0, Math.max(0, data.length - this.shortReadBy));
          this.shortReadBy = 0;
        }
        const out = new Uint8Array(9 + data.length);
        out[0] = cmd;
        new DataView(out.buffer).setBigUint64(1, this.mcuTimeUs, true);
        out.set(data, 9);
        return this.send(0x91, out);
      }
      case 0x33: {
        if (payload.length < 2 || payload.length > 129) return this.status(cmd, 0x03);
        const addr = payload[0]!;
        const data = payload.subarray(1);
        if (addr + data.length > 0x100) return this.status(cmd, 0x04);
        const injected = this.page === 0 ? this.failWriteReg.get(addr) : undefined;
        if (injected !== undefined) {
          this.failWriteReg.delete(addr);
          return this.status(cmd, injected);
        }
        this.ops.push({ op: "w", page: this.page, addr, data: data.slice() });
        this.write(addr, data);
        return this.status(cmd, 0x00);
      }
      case 0x34:
        this.resetCount += 1;
        this.streaming = false;
        this.powerOn();
        return this.status(cmd, 0x00);
      case 0x35:
        if (payload.length !== 5) return this.status(cmd, 0x03);
        this.streams.push(payload.slice());
        this.streaming = true;
        return this.status(cmd, 0x00);
      case 0x36:
        this.streaming = false;
        return this.status(cmd, 0x00);
      case 0x37:
        return this.send(0x92, this.info);
      default:
        return this.status(cmd, 0x02);
    }
  }

  // ── sensor model ───────────────────────────────────────────────────────────

  private read(addr: number, len: number): Uint8Array {
    const page = this.page as 0 | 1;
    const out = this.regs[page].slice(addr, addr + len);
    if (page === 0) {
      const covers = (reg: number) => reg >= addr && reg < addr + len;
      if (covers(SYS_STATUS)) out[SYS_STATUS - addr] = this.sysStatus();
      if (covers(QUA_DATA) && this.fusionZeroLeft > 0) {
        this.fusionZeroLeft -= 1;
        out.fill(0, QUA_DATA - addr, Math.min(len, QUA_DATA - addr + 8));
        if (this.fusionZeroLeft === 0 && !this.fusionStuck) this.regs[0].set(this.fusionQuat, QUA_DATA);
      }
      if (covers(INT_STA)) this.regs[0][INT_STA] = 0; // clears on read
    }
    return out;
  }

  private sysStatus(): number {
    if (this.sysStatusScript.length > 0) return this.sysStatusScript.shift()!;
    if (this.selfTestLeftover) return 4;
    const mode = this.oprMode;
    return mode === 0 ? 0 : mode >= 8 ? 5 : 6;
  }

  private write(addr: number, data: Uint8Array): void {
    const page = this.page as 0 | 1;
    for (let i = 0; i < data.length; i++) {
      const reg = addr + i;
      const v = data[i]!;
      if (page === 0 && reg === OPR_MODE) this.writeOprMode(v & 0x0f);
      else if (page === 0 && reg === SYS_TRIGGER) this.writeSysTrigger(v);
      else if (page === 0 && reg === PAGE_ID) this.regs[0][PAGE_ID] = v & 1;
      else this.regs[page][reg] = v;
      if (page === 1 && reg === PAGE_ID) this.regs[0][PAGE_ID] = v & 1; // PAGE_ID is on both pages
    }
  }

  private writeOprMode(mode: number): void {
    const current = this.oprMode;
    const suspended = (this.regs[0][PWR_MODE]! & 3) === 2;
    if (mode !== 0 && (current !== 0 || suspended)) return; // silently ignored (§4.1, §4.6)
    this.regs[0][OPR_MODE] = mode;
    if (mode === 0) {
      this.regs[0].fill(0, 0x08, 0x34); // CONFIG reads zero everywhere
    } else {
      this.selfTestLeftover = false;
      if (mode >= 8) {
        this.fusionZeroLeft = this.fusionStuck ? Number.MAX_SAFE_INTEGER : this.fusionZeroReadsAfterSwitch;
        if (this.fusionZeroLeft === 0) this.regs[0].set(this.fusionQuat, QUA_DATA);
      }
    }
  }

  private writeSysTrigger(v: number): void {
    this.regs[0][SYS_TRIGGER] = v & 0x80; // CLK_SEL sticks, action bits self-clear
    if ((v & 0x01) !== 0 && this.oprMode === 0) {
      this.regs[0][ST_RESULT] = this.selfTestResult;
      this.regs[0][0x3a] = this.selfTestError;
      this.selfTestLeftover = true;
    }
    if ((v & 0x40) !== 0) this.regs[0][INT_STA] = 0;
  }
}

export function toHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
}
