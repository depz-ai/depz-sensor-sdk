/**
 * uld/base — what every sensor driver of the 1D family shares. Async mirror of
 * the Python `depz_sensor_sdk.vl53lx.uld.base` (itself the firmware repo's
 * `tof_vl53l0_4/tools/uld/base.py`).
 *
 * `BridgePlatform` is ST's platform layer (RdByte/WrWord/WaitMs...) over the
 * bridge; `SensorDriver` is the contract the device class talks to;
 * `Measurement` is the one result shape every caller understands. Every
 * register access is async, so every driver method that touches the sensor
 * returns a Promise; pure decoding (`decode`, `toMeasurement`) stays sync.
 */

import { Vl53Error, VL53_XSHUT_RESET, type BridgeDevice } from "./link.js";

/** VL53L4CD_RdByte/.../WaitMs over the bridge. Register values are big-endian. */
export class BridgePlatform {
  constructor(
    readonly dev: BridgeDevice,
    /** Sleep implementation (tests inject an instant one). */
    readonly sleepImpl: (ms: number) => Promise<void> = (ms) =>
      new Promise((r) => setTimeout(r, ms)),
  ) {}

  rdMulti(addr: number, size: number): Promise<Uint8Array> {
    return this.dev.readReg(addr, size);
  }

  wrMulti(addr: number, data: Uint8Array): Promise<void> {
    return this.dev.writeReg(addr, data);
  }

  async rdByte(addr: number): Promise<number> {
    return (await this.dev.readReg(addr, 1))[0]!;
  }

  async rdWord(addr: number): Promise<number> {
    const b = await this.dev.readReg(addr, 2);
    return (b[0]! << 8) | b[1]!;
  }

  async rdDword(addr: number): Promise<number> {
    const b = await this.dev.readReg(addr, 4);
    return ((b[0]! << 24) | (b[1]! << 16) | (b[2]! << 8) | b[3]!) >>> 0;
  }

  wrByte(addr: number, value: number): Promise<void> {
    return this.dev.writeReg(addr, Uint8Array.of(value & 0xff));
  }

  wrWord(addr: number, value: number): Promise<void> {
    const v = value & 0xffff;
    return this.dev.writeReg(addr, Uint8Array.of(v >> 8, v & 0xff));
  }

  wrDword(addr: number, value: number): Promise<void> {
    const v = value >>> 0;
    return this.dev.writeReg(
      addr,
      Uint8Array.of((v >>> 24) & 0xff, (v >>> 16) & 0xff, (v >>> 8) & 0xff, v & 0xff),
    );
  }

  /** Outside the C platform layer: only a driver re-times the bus. */
  setI2cSpeed(khz: number): Promise<void> {
    return this.dev.setI2cSpeed(khz);
  }

  /** Outside the C platform layer: register-address width, sticky. */
  setAddrWidth(width: number): Promise<void> {
    return this.dev.setAddrWidth(width);
  }

  /** Outside the C platform layer: the bridge owns XSHUT. */
  xshutReset(): Promise<void> {
    return this.dev.xshut(VL53_XSHUT_RESET);
  }

  sleepMs(ms: number): Promise<void> {
    return this.sleepImpl(ms);
  }
}

/**
 * One return of a multi-target frame. `minRangeMm`/`maxRangeMm` are the edges
 * of the target's own pulse (histogram parts); the others repeat `distanceMm`.
 */
export interface Target {
  distanceMm: number;
  status: number;
  statusText: string;
  signalKcps: number;
  ambientKcps: number;
  sigmaMm: number;
  minRangeMm: number;
  maxRangeMm: number;
}

/**
 * One range result, the same shape for every sensor of the family. On the
 * histogram driver `targets` holds every return strongest-first and the
 * top-level fields repeat `targets[0]`; on the light drivers it stays empty.
 */
export interface Measurement {
  distanceMm: number;
  status: number;
  statusText: string;
  signalKcps: number;
  ambientKcps: number;
  sigmaMm: number;
  spads: number;
  targets: Target[];
  extra: Record<string, unknown>;
}

/** Build a Measurement with the Python defaults for omitted fields. */
export function measurement(m: Partial<Measurement> = {}): Measurement {
  return {
    distanceMm: m.distanceMm ?? 0,
    status: m.status ?? 0,
    statusText: m.statusText ?? "",
    signalKcps: m.signalKcps ?? 0,
    ambientKcps: m.ambientKcps ?? 0,
    sigmaMm: m.sigmaMm ?? 0,
    spads: m.spads ?? 0,
    targets: m.targets ?? [],
    extra: m.extra ?? {},
  };
}

/**
 * Everything a caller may use on a sensor driver (mirror of the Python
 * `SensorDriver`). Anything else a driver defines belongs to that product and
 * is reached off the concrete class. `SUPPORTS` says which optional groups
 * the product actually has: mode, timing, offset, xtalk, calib_offset,
 * calib_xtalk, thresholds, roi, signal_thresh, sigma_thresh, temp_update,
 * refspad.
 *
 * Static facts (`ADDR_WIDTH`, `CLEAR_STEPS`, `MAX_KHZ`, `SUPPORTS`, `MODES`,
 * `BUDGET_MS`, `HISTOGRAM`) are exposed both as static members of the driver
 * class (the registry reads them before construction) and as instance getters.
 */
export abstract class SensorDriver {
  static readonly ADDR_WIDTH: number = 2;
  static readonly CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = [];
  static readonly MAX_KHZ: number = 400;
  static readonly SUPPORTS: ReadonlySet<string> = new Set();
  static readonly MODES: readonly string[] = [];
  static readonly BUDGET_MS: readonly [number, number] = [10, 200];
  static readonly HISTOGRAM: boolean = false;

  constructor(
    readonly p: BridgePlatform,
    /** The product the board reports, e.g. 'VL53L4CX' (one driver serves several). */
    readonly product: string,
  ) {}

  private get cls(): typeof SensorDriver {
    return this.constructor as typeof SensorDriver;
  }
  get ADDR_WIDTH(): number {
    return this.cls.ADDR_WIDTH;
  }
  get CLEAR_STEPS(): ReadonlyArray<readonly [number, number]> {
    return this.cls.CLEAR_STEPS;
  }
  get MAX_KHZ(): number {
    return this.cls.MAX_KHZ;
  }
  get SUPPORTS(): ReadonlySet<string> {
    return this.cls.SUPPORTS;
  }
  get MODES(): readonly string[] {
    return this.cls.MODES;
  }
  get BUDGET_MS(): readonly [number, number] {
    return this.cls.BUDGET_MS;
  }
  get HISTOGRAM(): boolean {
    return this.cls.HISTOGRAM;
  }

  // ── identity / lifecycle ──
  abstract modelId(): Promise<number>;
  /** Reset and configure; leaves the bus at MAX_KHZ. Safe to re-run. */
  abstract sensorInit(): Promise<void>;
  abstract startRanging(): Promise<void>;
  abstract stopRanging(): Promise<void>;

  // ── interrupt ──
  abstract checkForDataReady(): Promise<boolean>;

  async waitDataReady(timeoutS = 1.0): Promise<void> {
    const deadline = Date.now() + timeoutS * 1000;
    while (!(await this.checkForDataReady())) {
      if (Date.now() > deadline) throw new Vl53Error("timeout waiting for data ready");
      await this.p.sleepMs(1);
    }
  }

  abstract clearInterrupt(): Promise<void>;

  // ── data ──
  /** [addr, len] — the register block the bridge streams on each INT. */
  abstract streamBlock(): [number, number];
  /** Streamed block bytes → Measurement (pure; stateful on histogram parts). */
  abstract decode(raw: Uint8Array): Measurement;

  /** One polled result. Default: read the stream block and decode it. */
  async readMeasurement(): Promise<Measurement> {
    const [addr, length] = this.streamBlock();
    return this.decode(await this.p.rdMulti(addr, length));
  }

  // ── timing ──
  abstract setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void>;
  /** → [timingBudgetMs, interMeasurementMs]. */
  abstract getRangeTiming(): Promise<[number, number]>;

  /** The only budgets accepted right now (ascending), or [] for any in BUDGET_MS. */
  budgetChoices(): number[] {
    return [];
  }

  // ── ranging mode (only when 'mode' in SUPPORTS) ──
  async setMode(_name: string): Promise<void> {
    throw new Vl53Error(`${this.product}: no ranging modes`);
  }
  async getMode(): Promise<string> {
    throw new Vl53Error(`${this.product}: no ranging modes`);
  }

  /** How far this driver's configuration can measure (mm), or null. */
  reachMm(): number | null {
    return null;
  }

  /** Numbers beyond the contract, for display only. */
  async driverInfo(): Promise<Record<string, unknown>> {
    return {};
  }
}
