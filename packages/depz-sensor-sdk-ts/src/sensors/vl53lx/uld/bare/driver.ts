/**
 * uld/bare/driver — the SensorDriver the device class talks to, on top of
 * BareDriver. Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.driver`.
 *
 * `core.BareDriver` configures the die and hands back a 24-bin histogram
 * frame; `hist.ts` turns that frame into targets. This is the adapter between
 * them and the family contract, so ranging, streaming and every UI work on
 * the histogram products without a special case.
 *
 * What this driver gives that the light ones do not: several targets per
 * frame. The first frame of a stream reports status 6 (no wrap-around check
 * done): the checks that would upgrade it need the frame before.
 *
 * `binData(raw)` and `toMeasurement(bins)` are synchronous so the device class
 * can decode streamed frames inside its report handler; both are stateful
 * (A/B frame alternation, frame history) — decode every frame exactly once.
 */

import { SensorDriver, type BridgePlatform, type Measurement } from "../base.js";
import { I2C_KHZ_BOOT, Vl53Error } from "../link.js";
import { phaseWindowMm } from "../vl53l1-die.js";
import {
  BareDriver,
  CLEAR_RANGE_INT,
  DISTANCE_MODES,
  HISTOGRAM_BIN_DATA_I2C_INDEX,
  HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES,
  TIMING_DIVISOR,
  TIMING_GUARD_US,
  type HistogramBinData,
} from "./core.js";
import { FrameHistory, processData, toMeasurement } from "./hist.js";
import { floorDiv } from "./imath.js";

export { HistogramBinData } from "./core.js";

// The VL53L1 die's own registers, the few this adapter needs by name.
export const GPIO_HV_MUX__CTRL = 0x0030;
export const GPIO__TIO_HV_STATUS = 0x0031;
export const SYSTEM__INTERRUPT_CLEAR = 0x0086;
export const FIRMWARE__SYSTEM_STATUS = 0x00e5;
export const PAD_I2C_HV__CONFIG = 0x002d;

/**
 * Bits 2 and 5, the byte `VL53L4CD_I2C_FAST_MODE_PLUS` writes to this
 * register in the C ULD. ST's histogram preset leaves the pad at 0x00; the pad
 * is a property of the die, so it is set here and never cleared.
 */
export const FMP_PAD_CONFIG = 0x12;

export const DEFAULT_MODE = "medium";

/** VL53L1CX / L1CB / L3CX / L4CX in histogram mode: multi-target ranging. */
export class VL53LX extends SensorDriver {
  static override readonly ADDR_WIDTH = 2;
  // One write clears the interrupt and lets the next frame run (measured
  // over 40 frames on both dies: no misses, stream count unbroken).
  static override readonly CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = [
    [SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT],
  ];
  static override readonly MAX_KHZ = 1000; // with Fast Mode Plus on the sensor pad
  static override readonly SUPPORTS: ReadonlySet<string> = new Set(["mode", "timing"]);
  static override readonly MODES: readonly string[] = DISTANCE_MODES;
  // TIMING_GUARD_US comes off the budget before it is divided, and the range
  // timeout is capped at FDA_MAX_TIMING_BUDGET_US: 2..551 ms, rounded inwards.
  // Both MODES and BUDGET_MS are narrowed per die in sensorInit().
  static override readonly BUDGET_MS: readonly [number, number] = [2, 550];
  static override readonly HISTOGRAM = true;

  readonly bare: BareDriver;
  private mode: string = DEFAULT_MODE;
  // The die's own MODES / BUDGET_MS once NVM is read (null = the class's).
  private dieModes: readonly string[] | null = null;
  private dieBudgetMs: readonly [number, number] | null = null;
  private budgetMs = 33;
  private interMs = 0;
  // The A/B alternation rides on the ll-driver state; with the clear done by
  // the bridge, the state is advanced when a frame arrives. Which frame that
  // is comes from the device's own `result__stream_count`, not a call count.
  private lastStreamCount: number | null = null;
  // The phase and event consistency checks read the frame before this one.
  private readonly history = new FrameHistory();

  constructor(platform: BridgePlatform, product: string) {
    super(platform, product);
    this.bare = new BareDriver(platform);
  }

  override get MODES(): readonly string[] {
    return this.dieModes ?? VL53LX.MODES;
  }
  override get BUDGET_MS(): readonly [number, number] {
    return this.dieBudgetMs ?? VL53LX.BUDGET_MS;
  }

  // ── identity ──
  modelId(): Promise<number> {
    return this.p.rdWord(0x010f);
  }

  // ── lifecycle ──
  async waitBoot(timeoutS = 1.0): Promise<void> {
    const deadline = Date.now() + timeoutS * 1000;
    for (;;) {
      if ((await this.p.rdByte(FIRMWARE__SYSTEM_STATUS)) === 0x03) return;
      if (Date.now() > deadline) {
        throw new Vl53Error("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
      }
      await this.p.sleepMs(1);
    }
  }

  /**
   * Reset the part, configure it for histogram ranging and leave the bus at
   * this die's ceiling. The bus is raised only after the FM+ pad byte has
   * reached the device.
   */
  async sensorInit(): Promise<void> {
    await this.p.setAddrWidth(this.ADDR_WIDTH);
    await this.p.setI2cSpeed(I2C_KHZ_BOOT);
    await this.p.xshutReset();
    await this.waitBoot();

    await this.bare.dataInit();
    // The die is known once NVM is read: an L4 has no short mode and a
    // 200 ms budget ceiling, as in ST's L4CX BareDriver.
    this.dieModes = this.bare.distanceModes();
    this.dieBudgetMs = [2, floorDiv(this.bare.fdaMaxTimingBudgetUs(), 1000)];
    await this.setMode(this.mode);
    await this.p.wrByte(PAD_I2C_HV__CONFIG, FMP_PAD_CONFIG);
    if (this.MAX_KHZ !== I2C_KHZ_BOOT) await this.p.setI2cSpeed(this.MAX_KHZ);
  }

  /**
   * short / medium / long — the device preset mode: the VCSEL period, which
   * sets where the phase wraps (1.6 / 2.4 / 4.0 m), and the valid phase
   * window inside it (`reachMm()`). Host-side only.
   */
  override async setMode(name: string): Promise<void> {
    if (!this.MODES.includes(name)) {
      throw new Vl53Error(`no such mode: ${name} (have ${this.MODES.join(", ")})`);
    }
    this.mode = name;
    this.bare.setDistanceMode(name);
    // The preset rewrites the whole static config, pad byte included; put FM+
    // back or the first range-start write would switch the pad off at 1 MHz.
    this.bare.img.static_config.v.pad_i2c_hv__config = FMP_PAD_CONFIG;
    await this.setRangeTiming(this.budgetMs, this.interMs);
  }

  override async getMode(): Promise<string> {
    return this.mode;
  }

  /**
   * The valid phase window of the preset mode, which the post-processing
   * applies: 992 / 1785 / 3373 mm for short / medium / long at the usual
   * oscillator. Past it the target is status 4, although the bins only wrap
   * further out. Host-side: no sensor traffic.
   */
  override async reachMm(): Promise<number | null> {
    return phaseWindowMm(
      this.bare.img.static_nvm_managed.v.osc_measured__fast_osc__frequency!,
      this.bare.hpp.valid_phase_high,
    );
  }

  // ── driver-specific readout ──
  override async driverInfo(): Promise<Record<string, unknown>> {
    return {
      "range timeout us": this.bare.rangeConfigTimeoutUs,
      "MM1/MM2 offset mm": this.bare.hpp.range_offset_mm / 4,
    };
  }

  async startRanging(): Promise<void> {
    this.lastStreamCount = null;
    this.history.reset();
    await this.bare.loadPatch();
    await this.bare.initAndStartRange();
    // The interrupt line comes out of reset already asserted: one clear
    // deasserts it (ST's ClearInterruptAndStartMeasurement).
    await this.bare.clearInterruptAndStartNextRange();
  }

  async stopRanging(): Promise<void> {
    await this.bare.stopRange();
    await this.bare.unloadPatch();
  }

  // ── data ──
  streamBlock(): [number, number] {
    return [HISTOGRAM_BIN_DATA_I2C_INDEX, HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES];
  }

  decode(raw: Uint8Array): Measurement {
    return this.toMeasurement(this.binData(raw));
  }

  override async readMeasurement(): Promise<Measurement> {
    return this.toMeasurement(await this.binData());
  }

  /**
   * The raw frame. `raw` is the streamed block when the bridge read it
   * (synchronous); without it the block is read here (async).
   */
  binData(raw: Uint8Array): HistogramBinData;
  binData(): Promise<HistogramBinData>;
  binData(raw?: Uint8Array): HistogramBinData | Promise<HistogramBinData> {
    if (raw === undefined) {
      const [addr, length] = this.streamBlock();
      return this.p.rdMulti(addr, length).then((r) => this.binDataFrom(r));
    }
    return this.binDataFrom(raw);
  }

  private binDataFrom(raw: Uint8Array): HistogramBinData {
    const streamCount = raw[3]!; // result__stream_count
    if (streamCount !== this.lastStreamCount) {
      if (this.lastStreamCount !== null) {
        // What clear_interrupt_and_start_next_range() does to the state
        // machine, without the register writes: one frame further on.
        this.bare.state.updateRd(
          this.bare.img.system_control.v.system__mode_start!,
          this.bare.img.dynamic_config.v.system__grouped_parameter_hold!,
        );
      }
      this.lastStreamCount = streamCount;
    }
    return this.bare.getHistogramBinData(raw);
  }

  toMeasurement(bins: HistogramBinData): Measurement {
    const results = processData(bins, this.bare.hpp);
    // Once per frame, and only here: the history is a state machine.
    this.history.apply(results, bins, this.bare.state.rd_device_state, this.bare.hpp);
    return toMeasurement(results, bins);
  }

  // ── interrupt ──
  async checkForDataReady(): Promise<boolean> {
    const intPol = ((await this.p.rdByte(GPIO_HV_MUX__CTRL)) & 0x10) >> 4 === 1 ? 0 : 1;
    return ((await this.p.rdByte(GPIO__TIO_HV_STATUS)) & 1) === intPol;
  }

  async clearInterrupt(): Promise<void> {
    await this.p.wrByte(SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT);
  }

  // ── timing ──
  /**
   * The budget covers the whole measurement, of which the range timeout is
   * one sixth after a fixed guard comes off. 0 = continuous. Host-side only:
   * the image reaches the sensor at startRanging().
   */
  async setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void> {
    const [low, high] = this.BUDGET_MS;
    if (!(timingBudgetMs >= low && timingBudgetMs <= high)) {
      throw new Vl53Error(`timing_budget_ms must be ${low}..${high}`);
    }
    this.budgetMs = timingBudgetMs;
    this.interMs = interMeasurementMs;
    this.bare.setMeasurementTimingBudgetUs(timingBudgetMs * 1000);
    this.bare.setInterMeasurementPeriodMs(interMeasurementMs);
  }

  /** → [timing budget ms, inter-measurement ms]. */
  async getRangeTiming(): Promise<[number, number]> {
    const budgetUs = this.bare.rangeConfigTimeoutUs * TIMING_DIVISOR + TIMING_GUARD_US;
    return [floorDiv(budgetUs, 1000), this.bare.interMeasurementPeriodMs];
  }
}
