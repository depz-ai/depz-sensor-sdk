/**
 * In-process fake TMF8829 SPI register bridge + sensor over a LoopbackTransport
 * pair (contracts/14_SENSOR_TMF8829.md §2, §4).
 *
 * The bridge part answers TMF_READ_REG / TMF_WRITE_REG / TMF_START_STREAM /
 * TMF_STOP_STREAM / TMF_GET_INFO. The sensor part is a 256-byte register file
 * with the side effects the host sequence relies on: soft reset → bootloader,
 * bootloader commands at CMD_STAT (I2C_OFF, W_FIFO_BOTH, START_RAM_APP), the
 * firmware download through FIFO 0xFF, application commands (presets,
 * LOAD_CONFIG_PAGE, WRITE_PAGE, MEASURE, STOP) and the config page 0x22..0xDF
 * with the "preset does not reset everything" behaviour. Every command the host
 * sends is logged (`log`) so tests can assert the wire bytes.
 *
 * Frames are pushed by the test (`emitFrame`, `emitEvent`), chunked exactly as
 * the bridge does (≤1024 data bytes per RPT_TMF_FRAME).
 */

import { CrcType, LoopbackTransport, PacketParser, buildPacket, type PacketEvent } from "../src/index.js";

const REG_APP_ID = 0x00;
const REG_CMD_STAT = 0x08;
const REG_SERIAL = 0x1c;
const REG_CID_RID = 0x20;
const REG_CFG = 0x22;
const CFG_SIZE = 190;
const REG_INT_STATUS = 0xe1;
const REG_ID = 0xe3;
const REG_RESET = 0xf7;
const REG_ENABLE = 0xf8;
const REG_FIFOSTATUS = 0xfa;
const REG_FIFO = 0xff;

export interface LoggedCmd {
  cmd: number;
  payload: Uint8Array;
}

/** fp_mode each preset command loads. */
const PRESET_FP_MODE: Record<number, number> = {
  0x40: 0, 0x41: 0, 0x42: 0, 0x43: 2, 0x44: 2, 0x45: 3, 0x46: 3, 0x47: 5, 0x48: 5, 0x49: 0,
};

export class FakeTmf8829 {
  readonly transport: LoopbackTransport;
  readonly side: LoopbackTransport;

  /** Everything the host sent, in order. */
  readonly log: LoggedCmd[] = [];
  /** Sensor register file. */
  readonly regs = new Uint8Array(256);
  /** The sensor's stored config page (what LOAD_CONFIG_PAGE copies to 0x22..). */
  readonly cfg = new Uint8Array(CFG_SIZE);
  /** Bytes written to the FIFO port during the firmware download. */
  fifo: number[] = [];
  /** W_FIFO_BOTH params as received. */
  wFifoParams: Uint8Array | null = null;
  mode: "app-old" | "bootloader" | "app" = "app-old";
  i2cOff = false;
  appVersion: [number, number, number] = [2, 200, 0];
  serial = 0x12345678;

  // ── fault injection ──
  /** Value ID (0xE3) reads. */
  deviceId = 0x9e;
  /** Bootloader command → status it answers (default 0 READY). */
  blStatus = new Map<number, number>();
  /** Application command → status it answers (default OK; MEASURE → ACCEPTED). */
  appStatus = new Map<number, number>();
  /** CMD_STAT reads the command byte this many more times before the status (busy). */
  busyPolls = 0;
  /** Never leave the bootloader after a soft reset (APP_ID stays 0). */
  stuckBeforeBootloader = false;
  /** CID_RID value after LOAD_CONFIG_PAGE (null = the real 0x16). */
  cidOverride: number | null = null;
  /** Config fields WRITE_PAGE refuses to keep: offset in page → value it reads instead. */
  stuckBytes = new Map<number, number>();
  /** Bridge status for a bridge command id (instead of OK / the reply). */
  bridgeStatus = new Map<number, number>();
  /** Bridge commands left unanswered (host times out). */
  silent = new Set<number>();
  /** READ_REG answers this many bytes fewer. */
  shortRead = 0;
  /** Called after a CMD_STAT write is applied, before its WRITE_REG reply (e.g. raise an event). */
  onCommand: ((cmd: number) => Promise<void>) | null = null;
  streaming = false;
  intMask = 0;

  private txSeq = 0;
  private frameSeq = 0;
  private readonly parser = new PacketParser();

  constructor() {
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    // Power-on: an application left running by an earlier session.
    this.regs[REG_APP_ID] = 0x01;
    this.regs[REG_ID] = this.deviceId;
    // A non-default config page so tests can tell what configure() wrote.
    this.cfg[0] = 33; // period_ms
    this.cfg[2] = 0x4a; // kilo_iterations 586 = 0x024A
    this.cfg[3] = 0x02;
    this.cfg[0x2a - REG_CFG] = 1; // nr_peaks 1
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  /** Commands of one id, in order. */
  sent(cmd: number): Uint8Array[] {
    return this.log.filter((c) => c.cmd === cmd).map((c) => c.payload);
  }

  /** WRITE_REG payloads as [addr, ...data] arrays. */
  writes(): number[][] {
    return this.sent(0x33).map((p) => [...p]);
  }

  // ── unsolicited reports ──

  /** One frame as RPT_TMF_FRAME chunks (≤1024 B each). Returns the frame_seq used. */
  async emitFrame(
    raw: Uint8Array,
    timestampUs: bigint,
    edgeUs = 0n,
    opts: { seq?: number; chunks?: number[]; fullSize?: number } = {},
  ): Promise<number> {
    const seq = opts.seq ?? this.frameSeq;
    this.frameSeq = (seq + 1) & 0xff;
    const offsets = opts.chunks ?? [];
    if (!opts.chunks) for (let o = 0; o < raw.length; o += 1024) offsets.push(o);
    for (let i = 0; i < offsets.length; i++) {
      const off = offsets[i]!;
      const end = i + 1 < offsets.length ? offsets[i + 1]! : raw.length;
      const data = raw.subarray(off, Math.min(end, off + 1024));
      const p = new Uint8Array(21 + data.length);
      const v = new DataView(p.buffer);
      v.setBigUint64(0, timestampUs, true);
      v.setUint16(8, opts.fullSize ?? raw.length, true);
      v.setUint16(10, off, true);
      p[12] = seq;
      v.setBigUint64(13, edgeUs, true);
      p.set(data, 21);
      await this.send(0x93, p);
    }
    return seq;
  }

  async emitEvent(code: number, intStatus: number, frameId = 0, fifoStatus = 0, ts = 987654321n): Promise<void> {
    const p = new Uint8Array(12);
    new DataView(p.buffer).setBigUint64(0, ts, true);
    p[8] = intStatus;
    p[9] = code;
    p[10] = frameId;
    p[11] = fifoStatus;
    await this.send(0x94, p);
  }

  /** Any raw report (malformed-input tests). */
  async emitRaw(cmd: number, payload: Uint8Array): Promise<void> {
    await this.send(cmd, payload);
  }

  // ── transport ──

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return;
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private status(cmd: number, st: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, st));
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
    this.log.push({ cmd, payload: payload.slice() });
    if (this.silent.has(cmd)) return;
    const forced = this.bridgeStatus.get(cmd);
    if (forced !== undefined) {
      await this.status(cmd, forced);
      return;
    }
    if (cmd === 0x32) {
      const addr = payload[0]!;
      const len = payload[1]! | (payload[2]! << 8);
      const data = this.readRegs(addr, Math.max(0, len - this.shortRead));
      const out = new Uint8Array(9 + data.length);
      out[0] = 0x32;
      new DataView(out.buffer).setBigUint64(1, 5_000_000n, true);
      out.set(data, 9);
      await this.send(0x91, out);
    } else if (cmd === 0x33) {
      this.writeRegs(payload[0]!, payload.subarray(1));
      if (payload[0] === REG_CMD_STAT && this.onCommand) await this.onCommand(payload[1]!);
      await this.status(cmd, 0x00);
    } else if (cmd === 0x35) {
      this.streaming = true;
      this.intMask = payload[0]!;
      this.frameSeq = 0;
      await this.status(cmd, 0x00);
    } else if (cmd === 0x36) {
      this.streaming = false;
      await this.status(cmd, 0x00);
    } else if (cmd === 0x37) {
      const p = new Uint8Array(25);
      const v = new DataView(p.buffer);
      v.setUint32(0, 7, true); // int_edges
      v.setUint32(4, 6, true); // frames
      v.setUint32(8, 1, true); // frames_aborted
      v.setUint32(12, 0, true);
      v.setUint32(16, 0, true);
      p[20] = 1;
      p[21] = this.streaming ? 1 : 0;
      p[22] = this.intMask;
      p[23] = 0x01;
      p[24] = 0x00;
      await this.send(0x92, p);
    } else {
      await this.status(cmd, 0x02); // ERR_INVALID_CMD
    }
  }

  // ── sensor model ──

  private readRegs(addr: number, len: number): Uint8Array {
    const out = new Uint8Array(len);
    for (let i = 0; i < len; i++) out[i] = this.read8(Math.min(addr + i, REG_FIFO));
    return out;
  }

  private read8(a: number): number {
    if (a === REG_ID) return this.deviceId;
    if (a === REG_ENABLE) {
      // Reads 0 over SPI until the sensor's I2C is off.
      if (this.mode === "bootloader" && !this.i2cOff) return 0;
      return this.regs[a]! | 0x80;
    }
    if (a === REG_CMD_STAT && this.busyPolls > 0) {
      this.busyPolls -= 1;
      return this.pendingCmd;
    }
    return this.regs[a]!;
  }

  private pendingCmd = 0;

  private writeRegs(addr: number, data: Uint8Array): void {
    if (addr === REG_FIFO) {
      for (const b of data) this.fifo.push(b);
      return;
    }
    if (addr === REG_CMD_STAT) {
      this.command(data);
      return;
    }
    for (let i = 0; i < data.length; i++) {
      const a = addr + i;
      const b = data[i]!;
      if (a === REG_RESET && b & 0x40) this.softReset();
      else if (a === REG_INT_STATUS) this.regs[a] = this.regs[a]! & ~b; // write-1-to-clear
      else if (a === REG_FIFOSTATUS) this.regs[a] = this.regs[a]! & ~(b & 0x30);
      else this.regs[a] = b;
    }
  }

  private softReset(): void {
    this.mode = "bootloader";
    this.i2cOff = false;
    this.regs[REG_APP_ID] = this.stuckBeforeBootloader ? 0x00 : 0x80;
    this.regs[REG_CMD_STAT] = 0;
    this.fifo = [];
    // init() defaults (contract 14 §4 table): dump_histograms / ref_spad_frame kept.
    this.cfg[0] = 33;
    this.cfg[1] = 0;
    this.cfg[0x2a - REG_CFG] = 1;
  }

  private command(data: Uint8Array): void {
    const cmd = data[0]!;
    this.pendingCmd = cmd;
    let st: number;
    if (this.mode === "bootloader") {
      st = this.blStatus.get(cmd) ?? 0;
      if (st === 0) {
        if (cmd === 0x22) this.i2cOff = true;
        else if (cmd === 0x45) this.wFifoParams = data.slice(2, 2 + data[1]!);
        else if (cmd === 0x16) {
          this.mode = "app";
          this.regs[REG_APP_ID] = 0x01;
          this.regs.set(this.appVersion, 1);
          new DataView(this.regs.buffer).setUint32(REG_SERIAL, this.serial, true);
        }
      }
    } else {
      st = this.appStatus.get(cmd) ?? (cmd === 0x10 ? 0x01 : 0x00);
      if (st <= 0x01) this.appCommand(cmd);
    }
    this.regs[REG_CMD_STAT] = st;
  }

  private appCommand(cmd: number): void {
    const fp = PRESET_FP_MODE[cmd];
    if (fp !== undefined) {
      this.cfg[0x26 - REG_CFG] = fp;
      this.regs[REG_CID_RID] = cmd;
    } else if (cmd === 0x16) {
      this.regs.set(this.cfg, REG_CFG);
      this.regs[REG_CID_RID] = this.cidOverride ?? 0x16;
    } else if (cmd === 0x15) {
      this.cfg.set(this.regs.subarray(REG_CFG, REG_CFG + CFG_SIZE));
      for (const [off, v] of this.stuckBytes) this.cfg[off] = v;
    }
  }
}
