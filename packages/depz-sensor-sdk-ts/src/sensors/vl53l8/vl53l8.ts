/**
 * VL53L8CX/CH ToF sensor: host-side ULD over the firmware register bridge.
 *
 * The MCU is a thin SPI bridge (contracts/04): the full ST ULD driver runs
 * here on the host (`uld.ts`). Frame streaming is INT-driven push from the
 * device, reassembled from ≤1528-byte chunks.
 *
 * Mirrors the Python reference `depz_sensor_sdk.vl53l8.Vl53l8`.
 */

import { DepzDevice, StreamQueue, type DeviceOptions } from "../../device/device.js";
import { DepzError, DepzTimeoutError } from "../../errors.js";
import type { PacketEvent } from "../../protocol/framing.js";
import {
  CHUNK_SIZE,
  FrameReassembler,
  Vl53l8Cmd,
  Vl53l8Rpt,
  packReadReg,
  packStartStream,
  packWriteReg,
  unpackFrameChunk,
  unpackRegData,
  type RegData,
} from "../../protocol/vl53l8.js";
import type { SerialTransport } from "../../transport/types.js";
import { loadAssets, type Vl53l8Variant } from "./assets/index.js";
import { MI_CFG_DEV_IDX, type CnhConfig } from "./cnh.js";
import {
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  VL53L8CX,
  Vl53l8cxError,
  readDeviceRevisionId,
  type DetectionThreshold,
  type MotionConfig,
  type MotionResult,
  type Vl53l8Platform,
  type Vl53l8Results,
} from "./uld.js";

/** Below this the sensor never streams (contract 04). */
export const MIN_RANGING_FREQUENCY_HZ = 2;

/**
 * One parsed ranging frame. Arrays are sized to the active resolution
 * (16 or 64 zones); zone index runs row-major (see datasheet zone maps).
 */
export interface Vl53l8Frame {
  timestampUs: bigint;
  /** 16 | 64 */
  resolution: number;
  distanceMm: Int32Array;
  /** 5/9 = valid, 255 = no target. */
  targetStatus: Uint8Array;
  nbTargetDetected: Uint8Array;
  /** kcps/SPAD. */
  signalPerSpad: Float64Array;
  /** kcps/SPAD. */
  ambientPerSpad: Float64Array;
  nbSpadsEnabled: Int32Array;
  rangeSigmaMm: Float64Array;
  /** %. */
  reflectance: Uint8Array;
  siliconTempDegc: number;
  /** CH variant: raw CNH block (decode via cnh). */
  cnhRaw: Uint8Array | null;
  /** Motion-indicator output when `configureMotionIndicator()` is active. */
  motion: MotionResult | null;
}

/** Zone array reshaped to (4,4) or (8,8) — mirror of Vl53l8Frame.grid(). */
export function zoneGrid(arr: ArrayLike<number>, resolution: number): number[][] {
  const side = resolution === RESOLUTION_4X4 ? 4 : 8;
  const rows: number[][] = [];
  for (let r = 0; r < side; r++) {
    const row: number[] = [];
    for (let c = 0; c < side; c++) row.push(arr[r * side + c] ?? 0);
    rows.push(row);
  }
  return rows;
}

export interface Vl53l8Options extends DeviceOptions {
  /** ULD sleep implementation (tests inject an instant one). */
  sleepImpl?: (ms: number) => Promise<void>;
}

export interface Vl53l8InitOptions {
  /** Receives phase strings. */
  progress?: (text: string) => void;
  /** Tracks the big blob writes: (done, total) bytes. */
  writeProgress?: (done: number, total: number) => void;
}

/**
 * VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware (takes a
 * few seconds over CDC), then configure and `startRanging()`.
 *
 * This is the base class for both silicon variants. The VL53L8CH superset
 * (compact-network-histogram output) lives in `Vl53l8Ch`, which inherits every
 * method here. All configuration methods require `init()` first and must not
 * be called while ranging (the ULD talks to the current register bank; the
 * stream owns it — contract 04).
 */
export class Vl53l8Cx extends DepzDevice {
  /** Sensor-firmware blob variant this class loads. */
  protected readonly variantId: Vl53l8Variant = "cx";

  private uldDriver: VL53L8CX | null = null;
  private reassembler = new FrameReassembler();
  private frameCbs: Array<(frame: Vl53l8Frame) => void> = [];
  private frameQueues: StreamQueue<Vl53l8Frame>[] = [];
  private rangingFlag = false;
  protected cnhConfig: CnhConfig | null = null;
  private writeProgressCb: ((done: number, total: number) => void) | null = null;
  private resolution = RESOLUTION_4X4; // ULD default after init
  private readonly sleepImpl: (ms: number) => Promise<void>;

  /** ULD `platform` object mapped onto the firmware register bridge. */
  private readonly platform: Vl53l8Platform = {
    rdMulti: async (addr: number, size: number): Promise<Uint8Array> => {
      const out = new Uint8Array(size);
      let off = 0;
      while (size > 0) {
        const n = Math.min(size, CHUNK_SIZE);
        const rep = await this.request<RegData>(Vl53l8Cmd.ReadReg, packReadReg(addr, n), {
          matcher: DepzDevice.expectReport(Vl53l8Rpt.RegData, unpackRegData),
          timeoutMs: 2000,
        });
        if (rep.data.length !== n) {
          throw new DepzError(
            `READ_REG 0x${addr.toString(16).toUpperCase().padStart(4, "0")}: ` +
              `expected ${n}, got ${rep.data.length}`,
          );
        }
        out.set(rep.data, off);
        off += n;
        addr += n;
        size -= n;
      }
      return out;
    },
    wrMulti: async (addr: number, data: Uint8Array): Promise<void> => {
      let done = 0;
      while (done < data.length) {
        const chunk = data.subarray(done, done + CHUNK_SIZE);
        await this.request(Vl53l8Cmd.WriteReg, packWriteReg(addr, chunk), {
          okCompletes: true,
          timeoutMs: 2000,
        });
        addr += chunk.length;
        done += chunk.length;
        if (this.writeProgressCb !== null && data.length > CHUNK_SIZE) {
          this.writeProgressCb(done, data.length);
        }
      }
    },
    sleepMs: (ms: number): Promise<void> => this.sleepImpl(ms),
  };

  constructor(transport: SerialTransport, opts?: Vl53l8Options) {
    super(transport, opts);
    this.sleepImpl = opts?.sleepImpl ?? ((ms) => new Promise((r) => setTimeout(r, ms)));
  }

  // ── lifecycle ──────────────────────────────────────────────────────────────

  /** The underlying ULD driver (escape hatch for advanced DCI access). */
  get uld(): VL53L8CX {
    if (this.uldDriver === null) throw new DepzError("call init() first");
    return this.uldDriver;
  }

  /** 'cx' | 'ch' (valid after init()). */
  get variant(): Vl53l8Variant {
    return this.uld.variant;
  }

  async isAlive(): Promise<boolean> {
    try {
      await readDeviceRevisionId(this.platform);
      return true;
    } catch (err) {
      if (err instanceof Vl53l8cxError || err instanceof DepzError) return false;
      throw err;
    }
  }

  /**
   * Initialize the sensor: firmware blob download + default config.
   *
   * The blob variant is fixed by the class (`Vl53l8Cx` → 'cx', `Vl53l8Ch` →
   * 'ch'); `variant` is accepted only for backward compatibility and must
   * match the class variant when given. Assets are loaded lazily (dynamic
   * import) so the blobs stay out of bundles that never init the ToF.
   */
  async init(variant?: Vl53l8Variant, opts?: Vl53l8InitOptions): Promise<void> {
    if (variant !== undefined && variant !== this.variantId) {
      throw new DepzError(
        `${this.constructor.name} loads the '${this.variantId}' firmware blob; ` +
          "use Vl53l8Ch for 'ch'",
      );
    }
    const assets = await loadAssets(this.variantId);
    this.writeProgressCb = opts?.writeProgress ?? null;
    try {
      const driver = new VL53L8CX(this.platform, assets, this.variantId);
      await driver.init(opts?.progress);
      this.uldDriver = driver;
    } finally {
      this.writeProgressCb = null;
    }
  }

  // ── configuration (init() first; not while ranging) ─────────────────────────

  async getResolution(): Promise<number> {
    this.resolution = await this.uld.getResolution();
    return this.resolution;
  }

  async setResolution(zones: number): Promise<void> {
    if (zones !== RESOLUTION_4X4 && zones !== RESOLUTION_8X8) {
      throw new DepzError("resolution is 16 (4x4) or 64 (8x8) zones");
    }
    this.requireNotRanging();
    await this.uld.setResolution(zones);
    this.resolution = zones;
  }

  getRangingFrequencyHz(): Promise<number> {
    return this.uld.getRangingFrequencyHz();
  }

  async setRangingFrequencyHz(hz: number): Promise<void> {
    if (hz < MIN_RANGING_FREQUENCY_HZ) {
      throw new DepzError(
        `ranging frequency must be >= ${MIN_RANGING_FREQUENCY_HZ} Hz: below that ` +
          "the sensor never enters its ranging loop and streams nothing (contract 04)",
      );
    }
    this.requireNotRanging();
    await this.uld.setRangingFrequencyHz(hz);
  }

  getRangingMode(): Promise<number> {
    return this.uld.getRangingMode();
  }

  async setRangingMode(mode: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setRangingMode(mode);
  }

  getIntegrationTimeMs(): Promise<number> {
    return this.uld.getIntegrationTimeMs();
  }

  async setIntegrationTimeMs(ms: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setIntegrationTimeMs(ms);
  }

  getSharpenerPercent(): Promise<number> {
    return this.uld.getSharpenerPercent();
  }

  async setSharpenerPercent(pct: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setSharpenerPercent(pct);
  }

  getTargetOrder(): Promise<number> {
    return this.uld.getTargetOrder();
  }

  async setTargetOrder(order: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setTargetOrder(order);
  }

  // ── advanced features (UM3109; init() first, not while ranging) ─────────────

  /** POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants). */
  getPowerMode(): Promise<number> {
    return this.uld.getPowerMode();
  }

  /**
   * Enter sleep / wake / deep-sleep. Not while ranging. Waking from
   * DEEP_SLEEP re-downloads the firmware blob (init()).
   */
  async setPowerMode(mode: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setPowerMode(mode);
  }

  getXtalkMargin(): Promise<number> {
    return this.uld.getXtalkMargin();
  }

  async setXtalkMargin(marginKcps: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.setXtalkMargin(marginKcps);
  }

  /**
   * Run on-device crosstalk calibration against a flat target at
   * `distanceMm` with the given `reflectancePercent` (1..99) averaging
   * `nbSamples` (1..16). The result is captured into the xtalk buffer; read
   * it back with getCaldataXtalk(). Blocks several seconds.
   */
  async calibrateXtalk(reflectancePercent: number, nbSamples: number, distanceMm: number): Promise<void> {
    this.requireNotRanging();
    await this.uld.calibrateXtalk(reflectancePercent, nbSamples, distanceMm);
  }

  /** Read back the 776-byte xtalk calibration blob (save/restore). */
  async getCaldataXtalk(): Promise<Uint8Array> {
    this.requireNotRanging();
    return this.uld.getCaldataXtalk();
  }

  /** Restore a previously saved 776-byte xtalk calibration blob. */
  async setCaldataXtalk(blob: Uint8Array): Promise<void> {
    this.requireNotRanging();
    await this.uld.setCaldataXtalk(blob);
  }

  getDetectionThresholdsEnable(): Promise<number> {
    return this.uld.getDetectionThresholdsEnable();
  }

  async setDetectionThresholdsEnable(enabled: boolean): Promise<void> {
    this.requireNotRanging();
    await this.uld.setDetectionThresholdsEnable(enabled);
  }

  getDetectionThresholds(): Promise<DetectionThreshold[]> {
    return this.uld.getDetectionThresholds();
  }

  /**
   * Program the 64 detection thresholds (interrupt-on-threshold). Each entry
   * carries lowThresh, highThresh, measurement, type, zoneNum, operation (see
   * uld THRESH_* constants).
   */
  async setDetectionThresholds(thresholds: Partial<DetectionThreshold>[]): Promise<void> {
    this.requireNotRanging();
    await this.uld.setDetectionThresholds(thresholds);
  }

  async setDetectionThresholdsAutoStop(autoStop: boolean): Promise<void> {
    this.requireNotRanging();
    await this.uld.setDetectionThresholdsAutoStop(autoStop);
  }

  /**
   * Enable the motion indicator over [distanceMinMm, distanceMaxMm] and
   * surface motion output in each frame's `.motion`. Returns the underlying
   * uld MotionConfig for advanced tuning.
   */
  async configureMotionIndicator(distanceMinMm = 400, distanceMaxMm = 1500): Promise<MotionConfig> {
    this.requireNotRanging();
    const resolution = await this.getResolution();
    const cfg = await this.uld.motionIndicatorInit(resolution);
    await this.uld.motionIndicatorSetDistanceMotion(cfg, distanceMinMm, distanceMaxMm);
    return cfg;
  }

  // ── ranging ──────────────────────────────────────────────────────────────────

  /** Configure the output list, start the sensor and the MCU stream. */
  async startRanging(): Promise<void> {
    this.requireNotRanging();
    const cnhSize = this.cnhConfig !== null ? this.cnhConfig.requiredMemory() : null;
    await this.getResolution(); // refresh the cache used for frame shaping
    await this.uld.startRanging(cnhSize);
    const frameSize = this.uld.dataReadSize;
    this.reassembler = new FrameReassembler();
    await this.request(Vl53l8Cmd.StartStream, packStartStream(frameSize), {
      okCompletes: true,
    });
    this.rangingFlag = true;
  }

  async stopRanging(): Promise<void> {
    if (!this.rangingFlag) return;
    await this.request(Vl53l8Cmd.StopStream, undefined, { okCompletes: true });
    this.rangingFlag = false;
    await this.uld.stopRanging();
  }

  get ranging(): boolean {
    return this.rangingFlag;
  }

  /** Subscribe to parsed frames (read-pump context; don't block). */
  onFrame(cb: (frame: Vl53l8Frame) => void): () => void {
    this.frameCbs.push(cb);
    return () => {
      this.frameCbs = this.frameCbs.filter((c) => c !== cb);
    };
  }

  /**
   * Async iterator over parsed frames — bounded, drop-oldest (contract 07
   * §3). Subscribes eagerly at call time — call before or after
   * startRanging(); it ends when the device closes or the consumer breaks
   * out of iteration.
   */
  frames(maxsize = 8): StreamQueue<Vl53l8Frame> {
    const queue: StreamQueue<Vl53l8Frame> = new StreamQueue(maxsize, () => {
      this.frameQueues = this.frameQueues.filter((q) => q !== queue);
      deregister();
    });
    this.frameQueues.push(queue);
    const deregister = this.registerStream(queue);
    return queue;
  }

  /** Convenience: wait for the next frame. */
  async getFrame(timeoutMs = 2000): Promise<Vl53l8Frame> {
    const queue = new StreamQueue<Vl53l8Frame>(1);
    this.frameQueues.push(queue);
    // Register for teardown so a disconnect closes the queue promptly (the
    // wait resolves `done` at once) instead of stalling until the timeout.
    const deregister = this.registerStream(queue);
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
      const result = await Promise.race([
        queue.next(),
        new Promise<never>((_, reject) => {
          timer = setTimeout(
            () => reject(new DepzTimeoutError(Vl53l8Rpt.Vl53Frame, timeoutMs)),
            timeoutMs,
          );
        }),
      ]);
      if (result.done === true) throw new DepzError("device closed");
      return result.value;
    } finally {
      if (timer !== undefined) clearTimeout(timer);
      this.frameQueues = this.frameQueues.filter((q) => q !== queue);
      deregister();
    }
  }

  // ── internal ─────────────────────────────────────────────────────────────────

  protected requireNotRanging(): void {
    if (this.rangingFlag) {
      throw new DepzError("stopRanging() first — the stream owns the register bank");
    }
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Vl53l8Rpt.Vl53Frame || pkt.payload.length < 12) {
      return false;
    }
    const done = this.reassembler.feed(unpackFrameChunk(pkt.payload));
    if (done === null) return true;
    if (this.uldDriver === null) return true;
    let parsed: Vl53l8Results;
    try {
      parsed = this.uldDriver.parseFrame(done.frame);
    } catch {
      this.reassembler.discarded += 1;
      return true;
    }
    const frame = this.toFrame(done.timestampUs, parsed);
    for (const cb of [...this.frameCbs]) cb(frame);
    for (const q of [...this.frameQueues]) q.push(frame);
    return true;
  }

  private toFrame(timestampUs: bigint, parsed: Vl53l8Results): Vl53l8Frame {
    return {
      timestampUs,
      resolution: this.resolution,
      distanceMm: Int32Array.from(parsed.distanceMm),
      targetStatus: Uint8Array.from(parsed.targetStatus),
      nbTargetDetected: Uint8Array.from(parsed.nbTargetDetected),
      signalPerSpad: Float64Array.from(parsed.signalPerSpad),
      ambientPerSpad: Float64Array.from(parsed.ambientPerSpad),
      nbSpadsEnabled: Int32Array.from(parsed.nbSpadsEnabled),
      rangeSigmaMm: Float64Array.from(parsed.rangeSigmaMm),
      reflectance: Uint8Array.from(parsed.reflectance),
      siliconTempDegc: parsed.siliconTempDegc,
      cnhRaw: parsed.cnhRaw,
      motion: parsed.motion,
    };
  }
}

/**
 * VL53L8CH device: the VL53L8CX superset. Inherits every CX method and adds
 * Compact-Network-Histogram (CNH) output. `init()` downloads the CH firmware
 * blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware: each frame
 * can additionally carry a per-aggregate distance histogram.
 */
export class Vl53l8Ch extends Vl53l8Cx {
  protected override readonly variantId: Vl53l8Variant = "ch";

  /**
   * Arm the CNH histogram block for the next startRanging(). CH only — this
   * method does not exist on Vl53l8Cx.
   */
  async configureCnh(config: CnhConfig): Promise<void> {
    this.requireNotRanging();
    await this.uld.dciWriteData(MI_CFG_DEV_IDX, config.pack());
    this.cnhConfig = config;
  }
}

/**
 * Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
 * class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.
 */
export const Vl53l8 = Vl53l8Cx;
export type Vl53l8 = Vl53l8Cx;
