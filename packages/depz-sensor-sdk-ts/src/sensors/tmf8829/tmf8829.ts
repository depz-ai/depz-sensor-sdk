/**
 * TMF8829 multizone dToF sensor over the SPI register bridge
 * (contracts/14_SENSOR_TMF8829.md).
 *
 * The board is a thin bridge: this class runs the sensor — soft reset,
 * sensor firmware download through the sensor bootloader, presets and the
 * config page, start/stop — and turns the bridge's chunked frame stream into
 * measurements (half-frame merge, histograms, ref-SPAD frames, clock model,
 * capture time). Mirrors the Python reference `depz_sensor_sdk.tmf8829.Tmf8829`.
 */

import { DepzDevice, StreamQueue, type DeviceOptions } from "../../device/device.js";
import { DepzError, DepzTimeoutError, InvalidStateError, LinkClosedError } from "../../errors.js";
import type { PacketEvent } from "../../protocol/framing.js";
import { checkU64 } from "../../protocol/wire.js";
import {
  TMF8829_INT_HISTOGRAMS,
  TMF8829_INT_RESULTS,
  TMF8829_READ_MAX,
  TMF8829_SF_REF_FRAME,
  TMF8829_WRITE_MAX,
  Tmf8829Cmd,
  Tmf8829EventCode,
  Tmf8829FrameReassembler,
  Tmf8829Rpt,
  packTmf8829ReadReg,
  packTmf8829StartStream,
  packTmf8829WriteReg,
  unpackTmf8829Event,
  unpackTmf8829FrameChunk,
  unpackTmf8829Info,
  unpackTmf8829RegData,
  type Tmf8829Event,
  type Tmf8829Info,
  type Tmf8829RegData,
} from "../../protocol/tmf8829.js";
import type { SerialTransport } from "../../transport/types.js";
import { decodeBase64 } from "../vl53l8/assets/decode.js";
import { Tmf8829MeasurementBuilder, type Tmf8829Measurement } from "./builder.js";
import {
  TMF8829_CFG_SIZE,
  TMF8829_DEFAULT_PERIOD_MS,
  TMF8829_PRESETS,
  TMF8829_PRESET_KILO_ITERATIONS,
  TMF8829_REG_CFG,
  tmf8829CfgGet,
  tmf8829CfgSet,
  tmf8829ConfigFields,
  type Tmf8829CfgField,
  type Tmf8829Preset,
} from "./config.js";
import { TMF8829_FP_GRID } from "./frames.js";

/** Largest MCU µs time a measurement's `number` fields carry exactly. */
const MAX_SAFE_US = BigInt(Number.MAX_SAFE_INTEGER);

const REG_APP_ID = 0x00;
const REG_CMD_STAT = 0x08;
const REG_SERIAL = 0x1c;
const REG_CID_RID = 0x20;
const REG_INT_STATUS = 0xe1;
const REG_INT_ENAB = 0xe2;
const REG_ID = 0xe3;
const REG_REVID = 0xe4;
const REG_RESET = 0xf7;
const REG_ENABLE = 0xf8;
const REG_FIFOSTATUS = 0xfa;
const REG_FIFO = 0xff;

const DEVICE_ID = 0x9e;
const APP_ID_APP = 0x01;
const APP_ID_BL = 0x80;
const ENABLE_CPU_READY = 0x80;
const ENABLE_PUSEL_MASK = 0x30;
const ENABLE_PUSEL_BOOTMON = 0x10;
const ENABLE_PUSEL_RAM = 0x20;
const ENABLE_PON = 0x04;
const RESET_SOFT = 0x40;
const FIFO_ERROR_FLAGS = 0x30;

const BL_START_RAM_APP = 0x16;
const BL_I2C_OFF = 0x22;
const BL_W_FIFO_BOTH = 0x45;
const BL_STATUS_NAMES: Record<number, string> = { 0: "READY", 1: "ERR_PARAM", 2: "ERR_ADDR", 4: "ERR_FIFO" };

const CMD_MEASURE = 0x10;
const CMD_WRITE_PAGE = 0x15;
const CMD_LOAD_CONFIG_PAGE = 0x16;
const CMD_STOP = 0xff;
const STAT_OK = 0x00;
const STAT_ACCEPTED = 0x01;
const STAT_NAMES: Record<number, string> = {
  0x00: "OK",
  0x01: "ACCEPTED",
  0x02: "ERR_CONFIG",
  0x03: "ERR_APPLICATION",
  0x04: "ERR_CONFIG_RESULT_SIZE",
  0x05: "ERR_CONFIG_VCSEL",
  0x06: "ERR_WAKEUP_TIMED",
  0x07: "ERR_RESET_UNEXPECTED",
  0x08: "ERR_UNKNOWN_CMD",
  0x09: "ERR_UNKNOWN_CID",
  0x0a: "ERR_STOP_0",
  0x0b: "ERR_STOP_1",
  0x0c: "ERR_STOP_2",
  0x0d: "ERR_STOP_3",
  0x0e: "ERR_OSC_TUNE",
};

/** ams tmf8829_application.hex load address (application 2.200.0). */
export const TMF8829_FIRMWARE_LOAD_ADDR = 0x00010000;
/** READ/WRITE_REG may wait behind a 25 KB histogram frame. */
const REGISTER_TIMEOUT_MS = 1000;

/** The sensor firmware image `init()` downloads (14 444 B, MIT). Lazily decoded. */
export async function tmf8829FirmwareImage(): Promise<Uint8Array> {
  const mod = await import("./firmware-asset.js");
  return decodeBase64(mod.tmf8829FirmwareB64);
}

const hex2 = (v: number): string => `0x${v.toString(16).toUpperCase().padStart(2, "0")}`;

/** The sensor refused or did not answer (bootloader / application status). */
export class Tmf8829Error extends DepzError {}

export interface Tmf8829ConfigureOptions {
  periodMs?: number;
  /** Defaults to the preset's own value. */
  kiloIterations?: number;
  nrPeaks?: number;
  signalStrength?: boolean;
  noiseStrength?: boolean;
  xtalk?: boolean;
  histograms?: boolean;
  refFrame?: boolean;
  /** Any other config-page field by name. */
  fields?: Partial<Record<Tmf8829CfgField, number>>;
}

/** The config written by `configure()`, as read back from the sensor. */
export interface Tmf8829Config {
  preset: Tmf8829Preset;
  /** The 190-byte page 0x22..0xDF. */
  page: Uint8Array;
  histograms: boolean;
  refFrame: boolean;
  fields: Record<Tmf8829CfgField, number>;
  /** [rows, cols] of a measurement. */
  grid: readonly [number, number];
}

export interface Tmf8829Options extends DeviceOptions {
  /** Injectable sleep (tests). */
  sleepImpl?: (ms: number) => Promise<void>;
}

/**
 * TMF8829 ToF device (8×8 … 48×32 zones, up to 4 peaks per zone, histograms).
 * `init()` downloads the 14.4 KB sensor firmware (~0.15 s), `configure(preset,
 * …)` writes the config page, `start()` streams measurements to
 * `onMeasurement` / `measurements()`, `stop()` ends it. The sensor's EN pin is
 * wired high: there is no sleep, recovery from any state is `init()`.
 */
export class Tmf8829 extends DepzDevice {
  appVersion: string | null = null;
  sensorSerial: number | null = null;
  private readonly sleepImpl: (ms: number) => Promise<void>;
  private chain: Promise<unknown> = Promise.resolve();
  private readonly reassembler = new Tmf8829FrameReassembler();
  private readonly builder = new Tmf8829MeasurementBuilder();
  private measCbs: Array<(m: Tmf8829Measurement) => void> = [];
  private measQueues: StreamQueue<Tmf8829Measurement>[] = [];
  private interruptCbs: Array<(e: Tmf8829Event) => void> = [];
  private streamingFlag = false;
  private stopping = false;
  private cfg: Tmf8829Config | null = null;

  constructor(transport: SerialTransport, opts?: Tmf8829Options) {
    super(transport, opts);
    this.sleepImpl = opts?.sleepImpl ?? (async (ms) => new Promise((r) => setTimeout(r, ms)));
  }

  // ── register access ────────────────────────────────────────────────────────

  /**
   * Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the
   * FIFO port 0xFF stay there (the sensor does not auto-increment it).
   */
  async readRegisters(addr: number, length: number): Promise<Uint8Array> {
    return this.locked(async () => this.rd(addr, length));
  }

  /** Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF). */
  async writeRegisters(addr: number, data: Uint8Array): Promise<void> {
    return this.locked(async () => this.wr(addr, data));
  }

  /** Raw host-interface registers, for diagnostics. */
  async sensorState(): Promise<Record<string, number>> {
    return this.locked(async () => {
      const regs: Record<string, number> = {
        ID: REG_ID,
        REVID: REG_REVID,
        ENABLE: REG_ENABLE,
        RESET: REG_RESET,
        APP_ID: REG_APP_ID,
        CMD_STAT: REG_CMD_STAT,
        INT_STATUS: REG_INT_STATUS,
        INT_ENAB: REG_INT_ENAB,
        FIFOSTATUS: REG_FIFOSTATUS,
      };
      const out: Record<string, number> = {};
      for (const [k, a] of Object.entries(regs)) out[k] = await this.rd8(a);
      return out;
    });
  }

  /** Bridge counters (never touches the sensor, safe mid-stream). */
  async getBridgeInfo(): Promise<Tmf8829Info> {
    return this.request(Tmf8829Cmd.GetInfo, undefined, {
      matcher: DepzDevice.expectReport(Tmf8829Rpt.Info, unpackTmf8829Info),
    });
  }

  // ── init ───────────────────────────────────────────────────────────────────

  /**
   * Bring the sensor from any state to its running RAM application: soft
   * reset, bootloader, firmware download, start. Resolves [application
   * version, sensor serial]. Stops a running stream first. `progress(done,
   * total)` tracks the image download.
   */
  async init(progress?: (done: number, total: number) => void): Promise<[string, number]> {
    if (this.streamingFlag) await this.stop();
    const image = await tmf8829FirmwareImage();
    return this.locked(async () => {
      await this.wr8(REG_ENABLE, ENABLE_PON | ENABLE_PUSEL_BOOTMON);
      await this.sleepImpl(3);
      await this.wr8(REG_RESET, RESET_SOFT);
      await this.sleepImpl(3);
      const dev = await this.rd8(REG_ID);
      if (dev !== DEVICE_ID) {
        throw new Tmf8829Error(`sensor ID ${hex2(dev)}, expected ${hex2(DEVICE_ID)} (no TMF8829 on the bus?)`);
      }
      await this.poll(REG_APP_ID, (v) => v === APP_ID_BL, 100, "sensor bootloader");
      await this.blCmd(BL_I2C_OFF);
      // ENABLE reads 0 over SPI until the sensor's I2C is off.
      await this.poll(REG_ENABLE, (v) => (v & ENABLE_CPU_READY) !== 0, 100, "cpu_ready");
      const padded = new Uint8Array(image.length + ((4 - (image.length % 4)) % 4)).fill(0xff);
      padded.set(image);
      const params = new Uint8Array(6);
      const dv = new DataView(params.buffer);
      dv.setUint32(0, TMF8829_FIRMWARE_LOAD_ADDR, true);
      dv.setUint16(4, padded.length / 4, true);
      await this.blCmd(BL_W_FIFO_BOTH, params);
      await this.wr(REG_FIFO, padded, progress);
      await this.blCmd(BL_START_RAM_APP);
      await this.poll(REG_APP_ID, (v) => v === APP_ID_APP, 100, "START_RAM_APP");
      const en = await this.rd8(REG_ENABLE); // wake-ups must boot the RAM application
      await this.wr8(REG_ENABLE, (en & ~ENABLE_PUSEL_MASK) | ENABLE_PUSEL_RAM);
      const ver = await this.rd(REG_APP_ID, 4);
      this.appVersion = `${ver[1]}.${ver[2]}.${ver[3]}`;
      const ser = await this.rd(REG_SERIAL, 4);
      this.sensorSerial = new DataView(ser.buffer, ser.byteOffset, 4).getUint32(0, true);
      this.cfg = null;
      return [this.appVersion, this.sensorSerial];
    });
  }

  // ── config page ────────────────────────────────────────────────────────────

  /** LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor. */
  async configPage(): Promise<Uint8Array> {
    return this.locked(async () => this.loadConfig());
  }

  /**
   * Load `preset` and write the config page. Every field a preset does not
   * reset is written (contract 14 §4), so the result never depends on an
   * earlier config. Rejects with `Tmf8829Error` when the sensor refuses the
   * page (e.g. ERR_CONFIG_RESULT_SIZE) or a written field does not read back.
   */
  async configure(preset: Tmf8829Preset = "8x8", opts: Tmf8829ConfigureOptions = {}): Promise<Tmf8829Config> {
    if (!(preset in TMF8829_PRESETS)) {
      return Promise.reject(new RangeError(`unknown preset ${preset}; one of ${Object.keys(TMF8829_PRESETS).join(", ")}`));
    }
    if (this.streamingFlag) return Promise.reject(new InvalidStateError("configure() while streaming: stop() first"));
    const peaks = opts.nrPeaks ?? 1;
    // The 3-bit field takes 0..7, the sensor reports at most 4 peaks.
    if (!Number.isInteger(peaks) || peaks < 1 || peaks > 4) {
      return Promise.reject(new RangeError(`nrPeaks ${peaks}, must be 1..4`));
    }
    const want: Partial<Record<Tmf8829CfgField, number>> = {
      period_ms: opts.periodMs ?? TMF8829_DEFAULT_PERIOD_MS,
      kilo_iterations: opts.kiloIterations ?? TMF8829_PRESET_KILO_ITERATIONS[preset],
      nr_peaks: opts.nrPeaks ?? 1,
      signal_strength: opts.signalStrength ? 1 : 0,
      noise_strength: opts.noiseStrength ? 1 : 0,
      xtalk: opts.xtalk ? 1 : 0,
      dump_histograms: opts.histograms ? 1 : 0,
      ref_spad_frame: opts.refFrame ? 1 : 0,
      ...opts.fields,
    };
    return this.locked(async () => {
      await this.appCmd(TMF8829_PRESETS[preset]);
      const page = await this.loadConfig();
      for (const [k, v] of Object.entries(want)) tmf8829CfgSet(page, k as Tmf8829CfgField, v!);
      await this.wr(TMF8829_REG_CFG, page);
      await this.appCmd(CMD_WRITE_PAGE);
      const back = await this.loadConfig();
      const stuck = Object.entries(want)
        .filter(([k, v]) => tmf8829CfgGet(back, k as Tmf8829CfgField) !== v)
        .map(([k, v]) => `${k} wrote ${v}, reads ${tmf8829CfgGet(back, k as Tmf8829CfgField)}`);
      if (stuck.length) throw new Tmf8829Error(`config fields did not stick: ${stuck.join(", ")}`);
      const fields = tmf8829ConfigFields(back);
      this.cfg = {
        preset,
        page: back,
        histograms: !!opts.histograms,
        refFrame: !!opts.refFrame,
        fields,
        grid: TMF8829_FP_GRID[fields.fp_mode] ?? [0, 0],
      };
      return this.cfg;
    });
  }

  /** The last `configure()` result (null after `init()`). */
  get config(): Tmf8829Config | null {
    return this.cfg;
  }

  // ── streaming ──────────────────────────────────────────────────────────────

  get streaming(): boolean {
    return this.streamingFlag;
  }

  /** MEASURE with the bridge stream armed; configures the 8x8 preset first if needed. */
  async start(): Promise<void> {
    if (this.streamingFlag) return;
    const cfg = this.cfg ?? (await this.configure());
    const intMask = TMF8829_INT_RESULTS | (cfg.histograms ? TMF8829_INT_HISTOGRAMS : 0);
    await this.locked(async () => {
      await this.wr8(REG_FIFOSTATUS, FIFO_ERROR_FLAGS); // sticky under/overrun
      await this.wr8(REG_INT_STATUS, 0xff);
      await this.wr8(REG_INT_ENAB, intMask);
      this.reassembler.reset();
      this.builder.reset();
      await this.request(
        Tmf8829Cmd.StartStream,
        packTmf8829StartStream(intMask, cfg.refFrame ? TMF8829_SF_REF_FRAME : 0),
        { okCompletes: true },
      );
      this.streamingFlag = true;
      try {
        await this.appCmd(CMD_MEASURE, [STAT_OK, STAT_ACCEPTED]);
      } catch (e) {
        this.streamingFlag = false;
        await this.request(Tmf8829Cmd.StopStream, undefined, { okCompletes: true }).catch(() => {});
        throw e;
      }
    });
  }

  /**
   * Sensor STOP, then the bridge stream (frames already queued drain). The
   * bad-header event a STOP raises mid histogram dump is expected and
   * swallowed.
   */
  async stop(): Promise<void> {
    return this.locked(async () => {
      this.stopping = true;
      try {
        await this.appCmd(CMD_STOP);
        await this.wr8(REG_INT_ENAB, 0);
        await this.wr8(REG_INT_STATUS, 0xff);
        if (this.streamingFlag) await this.request(Tmf8829Cmd.StopStream, undefined, { okCompletes: true });
      } finally {
        this.streamingFlag = false;
        this.stopping = false;
      }
    });
  }

  override async close(): Promise<void> {
    if (this.streamingFlag) await this.stop().catch(() => {});
    await super.close();
  }

  /** Subscribe to measurements (read-pump context; don't block). */
  onMeasurement(cb: (m: Tmf8829Measurement) => void): () => void {
    this.measCbs.push(cb);
    return () => {
      this.measCbs = this.measCbs.filter((c) => c !== cb);
    };
  }

  /** Async iterator over measurements — bounded, drop-oldest. */
  measurements(maxsize = 16): StreamQueue<Tmf8829Measurement> {
    const queue: StreamQueue<Tmf8829Measurement> = new StreamQueue(maxsize, () => {
      this.measQueues = this.measQueues.filter((q) => q !== queue);
      deregister();
    });
    this.measQueues.push(queue);
    const deregister = this.registerStream(queue);
    return queue;
  }

  /** Wait for the next measurement. */
  async getMeasurement(timeoutMs = 2000): Promise<Tmf8829Measurement> {
    const queue = new StreamQueue<Tmf8829Measurement>(1);
    this.measQueues.push(queue);
    const deregister = this.registerStream(queue);
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
      const result = await Promise.race([
        queue.next(),
        new Promise<never>((_, reject) => {
          timer = setTimeout(() => reject(new DepzTimeoutError(Tmf8829Rpt.Frame, timeoutMs)), timeoutMs);
        }),
      ]);
      if (result.done === true) throw new LinkClosedError("device closed");
      return result.value;
    } finally {
      if (timer !== undefined) clearTimeout(timer);
      this.measQueues = this.measQueues.filter((q) => q !== queue);
      deregister();
    }
  }

  /**
   * RPT_TMF_EVENT that is not an expected stop artefact: a MOTION/PROXIMITY
   * interrupt (code 0) or a refused frame header (code 1 — the sensor is lost,
   * run `init()` again).
   */
  onInterrupt(cb: (e: Tmf8829Event) => void): () => void {
    this.interruptCbs.push(cb);
    return () => {
      this.interruptCbs = this.interruptCbs.filter((c) => c !== cb);
    };
  }

  /** Frames the bridge dropped (frame_seq gaps) in this stream. */
  get framesDropped(): number {
    return this.reassembler.seqGaps;
  }

  /** Frames lost to reassembly (incomplete, out-of-order chunks) in this stream. */
  get framesDiscarded(): number {
    return this.reassembler.discarded;
  }

  get frameParseErrors(): number {
    return this.builder.parseErrors;
  }

  /** Current clock ratio (Q16), for diagnostics. */
  get clockRatioQ16(): number {
    return this.builder.clock.ratioQ16;
  }

  // ── internal ───────────────────────────────────────────────────────────────

  /** Run `fn` after every previously queued sequence (non-reentrant). */
  private async locked<T>(fn: () => Promise<T>): Promise<T> {
    const run = this.chain.then(fn, fn);
    this.chain = run.catch(() => undefined);
    return run;
  }

  private async rd(addrIn: number, length: number): Promise<Uint8Array> {
    const out = new Uint8Array(length);
    let addr = addrIn;
    let off = 0;
    while (off < length) {
      const n = Math.min(length - off, TMF8829_READ_MAX);
      const rep = await this.request<Tmf8829RegData>(Tmf8829Cmd.ReadReg, packTmf8829ReadReg(addr, n), {
        matcher: DepzDevice.expectReport(Tmf8829Rpt.RegData, unpackTmf8829RegData),
        timeoutMs: REGISTER_TIMEOUT_MS,
      });
      if (rep.data.length !== n) throw new DepzError(`READ_REG ${hex2(addr)}: expected ${n}, got ${rep.data.length}`);
      out.set(rep.data, off);
      off += n;
      addr = Math.min(addr + n, REG_FIFO);
    }
    return out;
  }

  private async rd8(addr: number): Promise<number> {
    return (await this.rd(addr, 1))[0]!;
  }

  private async wr(addrIn: number, data: Uint8Array, progress?: (done: number, total: number) => void): Promise<void> {
    let addr = addrIn;
    for (let off = 0; off < data.length; off += TMF8829_WRITE_MAX) {
      const chunk = data.subarray(off, off + TMF8829_WRITE_MAX);
      await this.request(Tmf8829Cmd.WriteReg, packTmf8829WriteReg(addr, chunk), {
        okCompletes: true,
        timeoutMs: REGISTER_TIMEOUT_MS,
      });
      if (addr !== REG_FIFO) addr += chunk.length;
      progress?.(Math.min(off + chunk.length, data.length), data.length);
    }
  }

  private async wr8(addr: number, value: number): Promise<void> {
    return this.wr(addr, Uint8Array.of(value & 0xff));
  }

  private async poll(addr: number, done: (v: number) => boolean, timeoutMs: number, what: string): Promise<number> {
    const deadline = Date.now() + timeoutMs;
    for (;;) {
      const v = await this.rd8(addr);
      if (done(v)) return v;
      if (Date.now() > deadline) throw new Tmf8829Error(`${what}: timeout, reg ${hex2(addr)} = ${hex2(v)}`);
    }
  }

  private async blCmd(cmd: number, params?: Uint8Array): Promise<void> {
    const payload = params?.length ? Uint8Array.of(cmd, params.length, ...params) : Uint8Array.of(cmd);
    await this.wr(REG_CMD_STAT, payload);
    const st = await this.poll(REG_CMD_STAT, (v) => v !== cmd, 100, `bootloader cmd ${hex2(cmd)}`);
    if (st !== 0) throw new Tmf8829Error(`bootloader cmd ${hex2(cmd)} -> ${BL_STATUS_NAMES[st] ?? hex2(st)}`);
  }

  private async appCmd(cmd: number, accept: number[] = [STAT_OK], timeoutMs = 1000): Promise<number> {
    await this.wr8(REG_CMD_STAT, cmd);
    const st = await this.poll(REG_CMD_STAT, (v) => v < 0x10, timeoutMs, `sensor cmd ${hex2(cmd)}`);
    if (!accept.includes(st)) throw new Tmf8829Error(`sensor cmd ${hex2(cmd)} -> ${STAT_NAMES[st] ?? hex2(st)}`);
    return st;
  }

  private async loadConfig(): Promise<Uint8Array> {
    await this.appCmd(CMD_LOAD_CONFIG_PAGE);
    const cid = await this.rd8(REG_CID_RID);
    if (cid !== CMD_LOAD_CONFIG_PAGE) throw new Tmf8829Error(`CID_RID ${hex2(cid)} after LOAD_CONFIG_PAGE`);
    return this.rd(TMF8829_REG_CFG, TMF8829_CFG_SIZE);
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd === Tmf8829Rpt.Frame) {
      if (pkt.payload.length < 21) return true;
      const raw = this.reassembler.feed(unpackTmf8829FrameChunk(pkt.payload));
      if (raw === null) return true;
      // The wire times are u64; the measurement carries them as numbers. One
      // a number cannot hold exactly (≥ 2^53, e.g. 0xFF×8 from a glitched
      // board) drops the frame as unparseable — never a RangeError on the pump.
      const ts = checkU64("timestampUs", raw.timestampUs);
      const edge = checkU64("edgeUs", raw.edgeUs);
      if (ts > MAX_SAFE_US || edge > MAX_SAFE_US) {
        this.builder.parseErrors += 1;
        return true;
      }
      const m = this.builder.add(raw.data, Number(ts), Number(edge));
      if (m !== null) {
        for (const cb of [...this.measCbs]) this.callGuarded(cb, m);
        for (const q of [...this.measQueues]) q.push(m);
      }
      return true;
    }
    if (pkt.cmd === Tmf8829Rpt.Event) {
      if (pkt.payload.length < 12) return true;
      const ev = unpackTmf8829Event(pkt.payload);
      if (ev.code === Tmf8829EventCode.BadHeader && this.stopping) return true; // expected on STOP (contract 14 §2)
      for (const cb of [...this.interruptCbs]) this.callGuarded(cb, ev);
      return true;
    }
    return false;
  }
}
