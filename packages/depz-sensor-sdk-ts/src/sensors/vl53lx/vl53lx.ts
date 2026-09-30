/**
 * VL53L 1D ToF family — VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD,
 * VL53L4CX — over the `APP_VL53L0_4` register bridge (contracts/12).
 *
 * The MCU is a thin I2C bridge that knows no sensor; every ULD runs here on
 * the host (`uld/`). Two axes pick how a board is driven (`uld/registry.ts`):
 * the **product** (whose parameter set to load — normally the part the
 * board's device name carries; naming a neighbour borrows its driver) and the
 * **driver kind** (`uld`, `ulp` or `histogram`). Every product answers with
 * the same `Vl53lxMeasurement`; what differs is `supports(group)`.
 *
 * Mirrors the Python reference `depz_sensor_sdk.vl53lx`. TS-only difference:
 * the product-level description is `identifyProduct()`, because
 * `DepzDevice.identify()` already classifies the firmware.
 */

import { DepzDevice, StreamQueue, type DeviceOptions } from "../../device/device.js";
import { DepzError, DepzTimeoutError, StatusError } from "../../errors.js";
import { Status } from "../../protocol/common.js";
import type { PacketEvent } from "../../protocol/framing.js";
import {
  XFER_MAX,
  XSHUT_OFF,
  XSHUT_ON,
  XSHUT_RESET,
  packReadReg,
  packSetI2cSpeed,
  packWriteReg,
  packXshut,
  unpackRegData,
  unpackStream,
  type Vl53l4RegData,
} from "../../protocol/vl53l4.js";
import {
  Vl53lxCmd,
  Vl53lxRpt,
  packVl53lxSetAddrWidth,
  packVl53lxStartStream,
  unpackVl53lxInfo,
  type Vl53lxInfo,
} from "../../protocol/vl53lx.js";
import type { SerialTransport } from "../../transport/types.js";
import { chunkedRead, chunkedWrite } from "../reg-bridge.js";
import { BridgePlatform, SensorDriver, type Measurement, type Target } from "./uld/base.js";
import { ProtocolError, Vl53Error, type BridgeDevice } from "./uld/link.js";
import * as registry from "./uld/registry.js";

export { XSHUT_OFF, XSHUT_ON, XSHUT_RESET };

/**
 * Statuses that mean "this distance is real". A histogram product reports 6
 * on the first frame of a stream and 11 on a merged pulse: use this, not
 * `status === 0`, as the validity test on histogram products.
 */
export const PLOTTABLE_STATUSES: readonly number[] = [0, 6, 11];

/**
 * One ranging result, the same shape for every product of the family. On
 * the histogram driver `targets` holds every return strongest-first and the
 * top-level fields repeat `targets[0]`; `bins` carries the raw histogram
 * (the driver's bin object). On the light drivers `targets` is empty and
 * `bins` is null. `extra` is driver-specific — show it, don't branch on it.
 */
export interface Vl53lxMeasurement extends Measurement {
  /** MCU uptime at the INT edge (stream) / read (poll). */
  timestampUs: bigint;
  bins: unknown;
  /** `status === 0`. On histogram products prefer `plottable`. */
  valid: boolean;
  /** The frame has a usable range (PLOTTABLE_STATUSES). */
  plottable: boolean;
}

function toVl53lxMeasurement(timestampUs: bigint, m: Measurement, bins: unknown = null): Vl53lxMeasurement {
  return {
    timestampUs,
    distanceMm: m.distanceMm,
    status: m.status,
    statusText: m.statusText,
    signalKcps: m.signalKcps,
    ambientKcps: m.ambientKcps,
    sigmaMm: m.sigmaMm,
    spads: m.spads,
    targets: [...m.targets],
    extra: { ...m.extra },
    bins,
    valid: m.status === 0,
    plottable: PLOTTABLE_STATUSES.includes(m.status),
  };
}

/**
 * The distances a chart should draw for one measurement: the plottable
 * targets in driver order, or the single distance on a light driver.
 */
export function plotDistances(m: Measurement): number[] {
  if (m.targets.length > 0) {
    return m.targets.filter((t) => PLOTTABLE_STATUSES.includes(t.status)).map((t) => t.distanceMm);
  }
  return PLOTTABLE_STATUSES.includes(m.status) ? [m.distanceMm] : [];
}

/** The first plottable target, or null when the frame produced nothing usable. */
export function primaryTarget(m: Measurement): Target | null {
  for (const t of m.targets) if (PLOTTABLE_STATUSES.includes(t.status)) return t;
  return null;
}

/**
 * What the device class needs from the histogram (Bare) driver beyond
 * `SensorDriver`: `binData(raw)` steps the frame-pair state and returns the
 * bin object, `toMeasurement(bins)` finds the targets. Both run exactly once
 * per frame.
 */
export interface HistogramDriverApi {
  binData(raw: Uint8Array): unknown;
  toMeasurement(bins: unknown): Measurement;
}

/** The optional capability groups' methods (see `supports()`). */
interface DriverExtras {
  getOffset(): Promise<number>;
  setOffset(offsetMm: number): Promise<void>;
  getXtalk(): Promise<number>;
  setXtalk(xtalkKcps: number): Promise<void>;
  calibrateOffset(targetDistMm: number, nbSamples?: number): Promise<number>;
  calibrateXtalk(targetDistMm: number, nbSamples?: number): Promise<number>;
  getDetectionThresholds(): Promise<[number, number, number]>;
  setDetectionThresholds(low: number, high: number, window: number): Promise<void>;
  getSignalThreshold(): Promise<number>;
  setSignalThreshold(signalKcps: number): Promise<void>;
  getSigmaThreshold(): Promise<number>;
  setSigmaThreshold(sigmaMm: number): Promise<void>;
  getRoi(): Promise<[number, number]>;
  setRoi(x: number, y: number): Promise<void>;
  getRoiCenter(): Promise<number>;
  setRoiCenter(centerSpad: number): Promise<void>;
  startTemperatureUpdate(): Promise<void>;
  performRefSpadManagement(): Promise<[number, number]>;
}

/** Everything about what is connected (`identifyProduct()`). */
export interface Vl53lxProductInfo {
  board: string;
  detected: string | null;
  product: string | null;
  driver: string | null;
  driverClass: string;
  kinds: string[];
  modelId: number;
  /** Cross-check only: L1CX/L1CB and L4CD/L4CX share their ids. */
  modelIdOk: boolean;
  supports: ReadonlySet<string>;
  modes: readonly string[];
  reachMm: number | null;
  driverReachMm: number | null;
  budgetMs: readonly [number, number];
  budgetChoices: number[];
  histogram: boolean;
  maxKhz: number;
  caveat: string | null;
}

export interface Vl53lxOptions extends DeviceOptions {
  /** ULD sleep implementation (tests inject an instant one). */
  sleepImpl?: (ms: number) => Promise<void>;
}

export interface Vl53lxConfigureOptions {
  /** Timing budget, ms (see budgetChoices()). Default 50. */
  budgetMs?: number;
  /** 0 = continuous; otherwise the period between measurements (> budget). */
  interMs?: number;
  /** One of `modes`, applied before the budget. */
  mode?: string | null;
  /** Re-apply a stored offset calibration. */
  offsetMm?: number | null;
  /** Re-apply a stored crosstalk calibration. */
  xtalkKcps?: number | null;
}

function isThenable(x: unknown): x is PromiseLike<unknown> {
  return typeof x === "object" && x !== null && typeof (x as { then?: unknown }).then === "function";
}

/**
 * A board of the VL53L 1D family (firmware `APP_VL53L0_4`).
 *
 * `init()` binds the (product, driver kind) pair and runs the ULD's
 * sensorInit; `configure()` re-initialises and applies the ranging
 * configuration — call it before every run. Then `startRanging()` arms the MCU
 * stream and measurements arrive via `onMeasurement` / `measurements()` /
 * `getMeasurement()`. Configuration must not change while ranging.
 *
 * This generic class serves any product (read from the board's device name,
 * or `product` at init); the per-product subclasses (`Vl53l0x`...) fix it.
 */
export class Vl53lx extends DepzDevice {
  /** The product this class drives; null = read it from the device name. */
  static readonly PRODUCT: string | null = null;

  protected driverInst: SensorDriver | null = null;
  protected productName: string | null = null;
  protected driverKindName: string | null = null;
  protected caveat: string | null = null;
  private boardNameCache: string | null = null;
  private measureCbs: Array<(m: Vl53lxMeasurement) => void> = [];
  private measureQueues: StreamQueue<Vl53lxMeasurement>[] = [];
  protected rangingFlag = false;
  private streamParseErrorCount = 0;
  /** Keeps asynchronous histogram decodes in frame order. */
  private decodeChain: Promise<void> = Promise.resolve();
  /** MCU timestamp of the latest register read (poll-mode measurements). */
  private lastRegTimestampUs = 0n;
  private readonly sleepImpl: (ms: number) => Promise<void>;
  private readonly platform: BridgePlatform;

  /**
   * The `BridgeDevice` the ULD ports talk to: register transfers split at the
   * bridge's 253-byte limit; a non-OK status or a missing answer surfaces as
   * `ProtocolError`, exactly what the ports catch.
   */
  readonly bridge: BridgeDevice = {
    readReg: async (addr: number, length: number): Promise<Uint8Array> => {
      try {
        return await chunkedRead(addr, length, {
          chunkSize: XFER_MAX,
          readChunk: async (a, n) => {
            const rep = await this.request<Vl53l4RegData>(Vl53lxCmd.ReadReg, packReadReg(a, n), {
              matcher: DepzDevice.expectReport(Vl53lxRpt.RegData, unpackRegData),
              timeoutMs: 2000,
            });
            return { data: rep.data, timestampUs: rep.timestampUs };
          },
          onTimestamp: (ts) => {
            this.lastRegTimestampUs = ts;
          },
        });
      } catch (err) {
        throw Vl53lx.asProtocolError(err, true);
      }
    },
    writeReg: async (addr: number, data: Uint8Array): Promise<void> => {
      try {
        await chunkedWrite(addr, data, {
          chunkSize: XFER_MAX,
          writeChunk: (a, chunk) =>
            this.request(Vl53lxCmd.WriteReg, packWriteReg(a, chunk), {
              okCompletes: true,
              timeoutMs: 2000,
            }),
        });
      } catch (err) {
        throw Vl53lx.asProtocolError(err);
      }
    },
    setI2cSpeed: async (khz: number): Promise<void> => {
      try {
        await this.request(Vl53lxCmd.SetI2cSpeed, packSetI2cSpeed(khz), { okCompletes: true });
      } catch (err) {
        throw Vl53lx.asProtocolError(err);
      }
    },
    setAddrWidth: async (width: number): Promise<void> => {
      try {
        await this.request(Vl53lxCmd.SetAddrWidth, packVl53lxSetAddrWidth(width), {
          okCompletes: true,
        });
      } catch (err) {
        throw Vl53lx.asProtocolError(err);
      }
    },
    clearI2cErrors: async (): Promise<void> => {
      // A resetting die NACKs its own address for a moment; only the driver
      // knows those NACKs were expected, so the host zeroes the counter once
      // its init is through. A v2.00 bridge does not know the command.
      try {
        await this.request(Vl53lxCmd.ClearI2cErrors, new Uint8Array(0), { okCompletes: true });
      } catch (err) {
        if (err instanceof StatusError && err.status === Status.ErrInvalidCmd) {
          throw new DepzError(
            "the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) " +
              "and cannot clear its I2C error counter — reflash it",
          );
        }
        throw Vl53lx.asProtocolError(err);
      }
    },
    xshut: async (action: number): Promise<void> => {
      try {
        await this.request(Vl53lxCmd.Xshut, packXshut(action), {
          okCompletes: true,
          timeoutMs: 1000,
        });
      } catch (err) {
        throw Vl53lx.asProtocolError(err);
      }
    },
  };

  /**
   * A reset makes the die NACK for a moment (the VL53L0X soft reset does it);
   * the driver waits those out, and they are no bus fault.
   */
  private async sensorInit(drv: SensorDriver): Promise<void> {
    await drv.sensorInit();
    await this.bridge.clearI2cErrors();
  }

  /** StatusError / DepzTimeoutError (and a short register read) → ProtocolError. */
  private static asProtocolError(err: unknown, shortReadToo = false): unknown {
    if (err instanceof StatusError || err instanceof DepzTimeoutError) {
      return new ProtocolError(err.message);
    }
    // chunkedRead reports a short reply as a plain DepzError.
    if (shortReadToo && err instanceof DepzError && err.constructor === DepzError) {
      return new ProtocolError(err.message);
    }
    return err;
  }

  constructor(transport: SerialTransport, opts?: Vl53lxOptions) {
    super(transport, opts);
    this.sleepImpl = opts?.sleepImpl ?? ((ms) => new Promise((r) => setTimeout(r, ms)));
    this.platform = new BridgePlatform(this.bridge, (ms) => this.sleepImpl(ms));
  }

  private get classProduct(): string | null {
    return (this.constructor as typeof Vl53lx).PRODUCT;
  }

  // ── identity ─────────────────────────────────────────────────────────────

  /**
   * The board's device name (bootloader metablock), e.g.
   * `DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523`. Read once, then cached.
   */
  async boardName(): Promise<string> {
    if (this.boardNameCache === null) this.boardNameCache = await this.getDeviceName();
    return this.boardNameCache;
  }

  /** The product the device name carries, or null on an unstamped board. */
  async detected(): Promise<string | null> {
    return registry.productFromBoardName(await this.boardName());
  }

  /** The product init() bound (null before init). */
  get product(): string | null {
    return this.productName;
  }

  get driverKind(): string | null {
    return this.driverKindName;
  }

  /** The ULD port in use — escape hatch for product-specific calls. */
  get driver(): SensorDriver {
    if (this.driverInst === null) throw new DepzError("call init() first");
    return this.driverInst;
  }

  get initialized(): boolean {
    return this.driverInst !== null;
  }

  /** The driver kinds a product has (default: this board's). */
  async driverKinds(product?: string): Promise<string[]> {
    const name = product || this.classProduct || (await this.detected());
    return name ? registry.driverKinds(name) : [];
  }

  // ── lifecycle ────────────────────────────────────────────────────────────

  /**
   * Bind the (product, driver kind) pair and initialise the sensor.
   *
   * `product` defaults to this class's product, else the board's device name;
   * `driver` defaults to the product's first kind (`uld`, else `ulp`, else
   * `histogram`). A pair the table has no row for throws
   * `NotImplementedError` naming what the product has.
   */
  async init(driver?: string | null, opts: { product?: string | null } = {}): Promise<void> {
    this.requireNotRanging();
    const name = opts.product || this.classProduct || (await this.detected());
    if (!name) {
      throw new DepzError(
        `board '${await this.boardName()}' carries no product number — pass product explicitly`,
      );
    }
    const kind = driver || registry.driverKinds(name)[0];
    if (kind === undefined) {
      throw new registry.NotImplementedError(`${name} has no driver in this build`);
    }
    const [DriverCls, caveat] = registry.driverFor(name, kind);
    // Sticky on the bridge and 2 after a reset: set before the first register
    // access, model id included.
    await this.bridge.setAddrWidth(DriverCls.ADDR_WIDTH);
    const drv = new DriverCls(this.platform, name);
    await this.sensorInit(drv);
    this.driverInst = drv;
    this.productName = name;
    this.driverKindName = kind;
    this.caveat = caveat;
  }

  /** Everything about what is connected (needs init()). */
  async identifyProduct(): Promise<Vl53lxProductInfo> {
    const drv = this.driver;
    const modelId = await drv.modelId();
    const detected = await this.detected();
    return {
      board: await this.boardName(),
      detected,
      product: this.productName,
      driver: this.driverKindName,
      driverClass: drv.constructor.name,
      kinds: registry.driverKinds(this.productName!),
      modelId,
      modelIdOk: registry.modelIdOk(this.productName, modelId),
      supports: new Set(drv.SUPPORTS),
      modes: [...drv.MODES],
      reachMm: registry.reachMm(detected || this.productName),
      driverReachMm: drv.reachMm(),
      budgetMs: drv.BUDGET_MS,
      budgetChoices: this.budgetChoices(),
      histogram: drv.HISTOGRAM,
      maxKhz: drv.MAX_KHZ,
      caveat: this.caveat,
    };
  }

  /**
   * The lines a UI should show about the chosen pair: product named by hand,
   * a borrowed driver, a pair that reaches less than the board is rated for,
   * the driver's caveat.
   */
  async notes(): Promise<string[]> {
    const drv = this.driver;
    const detected = await this.detected();
    const out: string[] = [];
    if (detected === null) {
      out.push(`${this.productName}: board name carries no product number, named by hand`);
    } else if (detected !== this.productName) {
      out.push(`${this.productName}: borrowing this driver - the board says it is a ${detected}`);
    }
    const rated = registry.reachMm(detected || this.productName);
    const reach = drv.reachMm();
    if (rated && reach && reach < rated) {
      out.push(
        `${this.productName}/${this.driverKindName}: this pair reaches ${reach} mm, the board ` +
          `is rated ${rated} mm - past ${reach} mm the phase wraps and frames come back ` +
          "with status 4",
      );
    }
    if (this.caveat) out.push(`${this.productName}/${this.driverKindName}: ${this.caveat}`);
    return out;
  }

  /**
   * Whether this product/driver serves an optional capability group: mode,
   * timing, offset, calib_offset, xtalk, calib_xtalk, thresholds,
   * signal_thresh, sigma_thresh, roi, temp_update, refspad.
   */
  supports(group: string): boolean {
    return this.driver.SUPPORTS.has(group);
  }

  /** Named ranging modes, first = what init leaves; [] if none. */
  get modes(): readonly string[] {
    return [...this.driver.MODES];
  }

  // ── sensor power (XSHUT) and bridge diagnostics ─────────────────────────

  /**
   * Drive XSHUT: XSHUT_OFF / XSHUT_ON / XSHUT_RESET (1 ms pulse + 5 ms wait;
   * the host confirms the boot). OFF and RESET stop the stream; the sensor
   * then holds none of the configuration — init() again.
   */
  async xshut(action: number): Promise<void> {
    await this.request(Vl53lxCmd.Xshut, packXshut(action), { okCompletes: true, timeoutMs: 1000 });
    this.rangingFlag = false;
    this.driverInst = null;
  }

  /** RPT_VL53_INFO — the bridge's own counters; safe while streaming. */
  bridgeInfo(): Promise<Vl53lxInfo> {
    return this.request(Vl53lxCmd.GetInfo, undefined, {
      matcher: DepzDevice.expectReport(Vl53lxRpt.Info, unpackVl53lxInfo),
    });
  }

  // ── configuration (init() first; not while ranging) ─────────────────────

  /**
   * Re-initialise the sensor and apply a ranging configuration. The re-init
   * is deliberate (the only way to know what the configuration registers
   * hold). `mode` goes on before the budget; `offsetMm` / `xtalkKcps`
   * re-apply a stored calibration.
   */
  async configure(opts: Vl53lxConfigureOptions = {}): Promise<void> {
    this.requireNotRanging();
    const drv = this.driver;
    await this.sensorInit(drv);
    if (opts.mode != null) await drv.setMode(opts.mode);
    await drv.setRangeTiming(opts.budgetMs ?? 50, opts.interMs ?? 0);
    if (opts.offsetMm != null) await this.need("offset").setOffset(opts.offsetMm);
    if (opts.xtalkKcps != null) await this.need("xtalk").setXtalk(opts.xtalkKcps);
  }

  /** → [timingBudgetMs, interMeasurementMs]; 0 for the period = continuous. */
  getRangeTiming(): Promise<[number, number]> {
    return this.driver.getRangeTiming();
  }

  /** The ranging mode in use, or null on a product without modes. */
  async getMode(): Promise<string | null> {
    return this.supports("mode") ? this.driver.getMode() : null;
  }

  /** The only budgets accepted right now (ascending), or [] for any in range. */
  budgetChoices(): number[] {
    return [...this.driver.budgetChoices()];
  }

  /** The nearest budget this product will actually accept. */
  snapBudget(budgetMs: number): number {
    const choices = this.budgetChoices();
    if (choices.length > 0) {
      let best = choices[0]!;
      for (const c of choices) if (Math.abs(c - budgetMs) < Math.abs(best - budgetMs)) best = c;
      return best;
    }
    const [low, high] = this.driver.BUDGET_MS;
    return Math.max(low, Math.min(high, budgetMs));
  }

  // Capability-gated passthroughs. Each throws DepzError on a product/driver
  // that does not serve the group (see supports()).

  getOffsetMm(): Promise<number> {
    return this.need("offset").getOffset();
  }

  async setOffsetMm(offsetMm: number): Promise<void> {
    this.requireNotRanging();
    await this.need("offset").setOffset(offsetMm);
  }

  getXtalkKcps(): Promise<number> {
    return this.need("xtalk").getXtalk();
  }

  async setXtalkKcps(xtalkKcps: number): Promise<void> {
    this.requireNotRanging();
    await this.need("xtalk").setXtalk(xtalkKcps);
  }

  /**
   * Offset calibration against a flat target at `targetDistMm`; returns the
   * offset now programmed. Store it on the host (sensor RAM, lost on reset).
   */
  async calibrateOffset(targetDistMm: number, nbSamples?: number): Promise<number> {
    this.requireNotRanging();
    return this.need("calib_offset").calibrateOffset(targetDistMm, nbSamples);
  }

  /** Crosstalk calibration; returns the xtalk now programmed (kcps). */
  async calibrateXtalk(targetDistMm: number, nbSamples?: number): Promise<number> {
    this.requireNotRanging();
    return this.need("calib_xtalk").calibrateXtalk(targetDistMm, nbSamples);
  }

  /** → [distanceLowMm, distanceHighMm, window]. */
  getDetectionThresholds(): Promise<[number, number, number]> {
    return this.need("thresholds").getDetectionThresholds();
  }

  /** Arm the distance-window interrupt; stays armed until the next init/configure. */
  async setDetectionThresholds(
    distanceLowMm: number,
    distanceHighMm: number,
    window: number,
  ): Promise<void> {
    this.requireNotRanging();
    await this.need("thresholds").setDetectionThresholds(distanceLowMm, distanceHighMm, window);
  }

  getSignalThresholdKcps(): Promise<number> {
    return this.need("signal_thresh").getSignalThreshold();
  }

  async setSignalThresholdKcps(signalKcps: number): Promise<void> {
    this.requireNotRanging();
    await this.need("signal_thresh").setSignalThreshold(signalKcps);
  }

  getSigmaThresholdMm(): Promise<number> {
    return this.need("sigma_thresh").getSigmaThreshold();
  }

  async setSigmaThresholdMm(sigmaMm: number): Promise<void> {
    this.requireNotRanging();
    await this.need("sigma_thresh").setSigmaThreshold(sigmaMm);
  }

  /** → [x, y] SPAD window size. */
  getRoi(): Promise<[number, number]> {
    return this.need("roi").getRoi();
  }

  async setRoi(x: number, y: number): Promise<void> {
    this.requireNotRanging();
    await this.need("roi").setRoi(x, y);
  }

  getRoiCenter(): Promise<number> {
    return this.need("roi").getRoiCenter();
  }

  async setRoiCenter(centerSpad: number): Promise<void> {
    this.requireNotRanging();
    await this.need("roi").setRoiCenter(centerSpad);
  }

  /** Re-run VHV after an ambient change over 8 °C. */
  async startTemperatureUpdate(): Promise<void> {
    this.requireNotRanging();
    await this.need("temp_update").startTemperatureUpdate();
  }

  /** VL53L0X: re-measure the reference SPADs → [count, isAperture]. */
  async performRefSpadManagement(): Promise<[number, number]> {
    this.requireNotRanging();
    return this.need("refspad").performRefSpadManagement();
  }

  // ── ranging ──────────────────────────────────────────────────────────────

  /**
   * Start the sensor's ranging loop and arm the MCU stream: one
   * RPT_VL53_STREAM per INT edge, followed on the MCU by the driver's
   * interrupt-release writes.
   */
  async startRanging(): Promise<void> {
    this.requireNotRanging();
    const drv = this.driver;
    await drv.startRanging();
    const [addr, length] = drv.streamBlock();
    await this.request(
      Vl53lxCmd.StartStream,
      packVl53lxStartStream(addr, length, drv.CLEAR_STEPS),
      { okCompletes: true },
    );
    this.rangingFlag = true;
  }

  async stopRanging(): Promise<void> {
    if (!this.rangingFlag) return;
    // Clear host state and stop the sensor even if the STOP_STREAM ack fails.
    try {
      await this.request(Vl53lxCmd.StopStream, undefined, { okCompletes: true });
    } finally {
      this.rangingFlag = false;
      await this.driver.stopRanging();
    }
  }

  get ranging(): boolean {
    return this.rangingFlag;
  }

  /**
   * Single poll-mode measurement: start ranging, wait for data-ready, read,
   * release the interrupt, stop. Rejects while the stream runs.
   */
  async measureOnce(timeoutMs = 1000): Promise<Vl53lxMeasurement> {
    this.requireNotRanging();
    const drv = this.driver;
    await drv.startRanging();
    let m: Measurement;
    let bins: unknown = null;
    try {
      await drv.waitDataReady(timeoutMs / 1000);
      if (drv.HISTOGRAM) {
        const [addr, length] = drv.streamBlock();
        const h = drv as unknown as HistogramDriverApi;
        bins = await h.binData(await this.platform.rdMulti(addr, length));
        m = h.toMeasurement(bins);
      } else {
        m = await drv.readMeasurement();
      }
      await drv.clearInterrupt();
    } finally {
      await drv.stopRanging();
    }
    return toVl53lxMeasurement(this.lastRegTimestampUs, m, bins);
  }

  /** Subscribe to streamed measurements. Returns an unsubscribe function. */
  onMeasurement(cb: (m: Vl53lxMeasurement) => void): () => void {
    this.measureCbs.push(cb);
    return () => {
      this.measureCbs = this.measureCbs.filter((c) => c !== cb);
    };
  }

  /** Async iterator over measurements — bounded, drop-oldest (contract 07 §3). */
  measurements(maxsize = 64): StreamQueue<Vl53lxMeasurement> {
    const queue: StreamQueue<Vl53lxMeasurement> = new StreamQueue(maxsize, () => {
      this.measureQueues = this.measureQueues.filter((q) => q !== queue);
      deregister();
    });
    this.measureQueues.push(queue);
    const deregister = this.registerStream(queue);
    return queue;
  }

  /** Wait for the next streamed measurement. */
  async getMeasurement(timeoutMs = 2000): Promise<Vl53lxMeasurement> {
    const queue = new StreamQueue<Vl53lxMeasurement>(1);
    this.measureQueues.push(queue);
    const deregister = this.registerStream(queue);
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
      const result = await Promise.race([
        queue.next(),
        new Promise<never>((_, reject) => {
          timer = setTimeout(
            () => reject(new DepzTimeoutError(Vl53lxRpt.Stream, timeoutMs)),
            timeoutMs,
          );
        }),
      ]);
      if (result.done === true) throw new DepzError("device closed");
      return result.value;
    } finally {
      if (timer !== undefined) clearTimeout(timer);
      this.measureQueues = this.measureQueues.filter((q) => q !== queue);
      deregister();
    }
  }

  /** Stream reports dropped because the driver failed to decode them. */
  get streamParseErrors(): number {
    return this.streamParseErrorCount;
  }

  /** Drop counters of all live measurement queues (diagnostics). */
  get streamDroppedCounts(): number[] {
    return this.measureQueues.map((q) => q.droppedCount);
  }

  // ── internal ─────────────────────────────────────────────────────────────

  protected need(group: string): SensorDriver & DriverExtras {
    const drv = this.driver;
    if (!drv.SUPPORTS.has(group)) {
      throw new DepzError(
        `${this.productName}/${this.driverKindName} does not support '${group}'` +
          (this.caveat ? ` (${this.caveat})` : ""),
      );
    }
    return drv as SensorDriver & DriverExtras;
  }

  protected requireNotRanging(): void {
    if (this.rangingFlag) throw new DepzError("stopRanging() first — the stream owns the bus");
  }

  private emit(out: Vl53lxMeasurement): void {
    for (const cb of [...this.measureCbs]) cb(out);
    for (const q of [...this.measureQueues]) q.push(out);
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Vl53lxRpt.Stream || pkt.payload.length < 12) return false;
    const drv = this.driverInst;
    if (drv === null) return true;
    const sample = unpackStream(pkt.payload);
    // Decoded exactly once per sample: the histogram driver steps its own
    // frame-pair state on every frame it sees.
    try {
      if (!drv.HISTOGRAM) {
        this.emit(toVl53lxMeasurement(sample.timestampUs, drv.decode(sample.data)));
        return true;
      }
      const h = drv as unknown as HistogramDriverApi;
      const bins = h.binData(sample.data);
      if (!isThenable(bins)) {
        this.emit(toVl53lxMeasurement(sample.timestampUs, h.toMeasurement(bins), bins));
        return true;
      }
      // An async binData: keep frames in order through one chain.
      this.decodeChain = this.decodeChain.then(async () => {
        try {
          const b = await bins;
          this.emit(toVl53lxMeasurement(sample.timestampUs, h.toMeasurement(b), b));
        } catch {
          this.streamParseErrorCount += 1;
        }
      });
    } catch {
      // A bad sample must not kill the read pump.
      this.streamParseErrorCount += 1;
    }
    return true;
  }
}

/**
 * VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with
 * ranging profiles, offset and crosstalk calibration, ref-SPAD management.
 */
export class Vl53l0x extends Vl53lx {
  static override readonly PRODUCT: string | null = "VL53L0X";
}

/** VL53L1CX (4 m): VL53L1X ULD (short / long) or the histogram driver. */
export class Vl53l1cx extends Vl53lx {
  static override readonly PRODUCT: string | null = "VL53L1CX";
}

/** VL53L1CB (8 m, cover-glass module): same die and drivers as VL53L1CX. */
export class Vl53l1cb extends Vl53lx {
  static override readonly PRODUCT: string | null = "VL53L1CB";
}

/** VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver. */
export class Vl53l3cx extends Vl53lx {
  static override readonly PRODUCT: string | null = "VL53L3CX";
}

/**
 * VL53L4CX (6 m): the histogram driver only — to run it light, borrow a
 * sibling's with `init(undefined, { product: "VL53L4CD" })`.
 */
export class Vl53l4cx extends Vl53lx {
  static override readonly PRODUCT: string | null = "VL53L4CX";
}

/** Class per product (VL53L4CD on this firmware uses the generic class). */
export const VL53LX_CLASS_BY_PRODUCT: Readonly<Record<string, typeof Vl53lx>> = {
  VL53L0X: Vl53l0x,
  VL53L1CX: Vl53l1cx,
  VL53L1CB: Vl53l1cb,
  VL53L3CX: Vl53l3cx,
  VL53L4CX: Vl53l4cx,
};

/**
 * Pick the 1D-family class (contract 12 §1): the production PID model, then
 * the product the device name carries, then the generic class.
 */
export function resolveVl53lxClass(
  usbModel: string | null | undefined,
  deviceName: string | null | undefined,
): typeof Vl53lx {
  let product: string | null = (usbModel ?? "").toUpperCase() || null;
  if (product === null || !(registry.PRODUCTS as readonly string[]).includes(product)) {
    product = registry.productFromBoardName(deviceName ?? "");
  }
  return VL53LX_CLASS_BY_PRODUCT[product ?? ""] ?? Vl53lx;
}

export const VL53LX_PRODUCTS = registry.PRODUCTS;
export const VL53LX_DRIVER_KINDS = registry.DRIVER_KINDS;
export { ProtocolError, Vl53Error };
