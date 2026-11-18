/**
 * VL53L4CD ToF sensor: host-side ULD over the firmware register bridge.
 *
 * The MCU is a thin I2C bridge (contracts/10): the full ST ULD 2.2.3 driver
 * runs here on the host (`uld.ts`). Result streaming is INT-driven push from
 * the device — one 17-byte result block per RPT_VL53_STREAM, no reassembly
 * needed.
 *
 * Mirrors the Python reference `depz_sensor_sdk.vl53l4.Vl53l4cd`.
 */

import { DepzDevice, type DeviceOptions } from "../../device/device.js";
import { DepzError, InvalidStateError } from "../../errors.js";
import { chunkedRead, chunkedWrite } from "../reg-bridge.js";
import type { PacketEvent } from "../../protocol/framing.js";
import { checkUint } from "../../protocol/wire.js";
import {
  Vl53l4Cmd,
  Vl53l4Rpt,
  XFER_MAX,
  XSHUT_RESET,
  packReadReg,
  packSetI2cSpeed,
  packStartStream,
  packWriteReg,
  packXshut,
  unpackInfo,
  unpackRegData,
  unpackStream,
  type Vl53l4Info,
  type Vl53l4RegData,
} from "../../protocol/vl53l4.js";
import type { SerialTransport } from "../../transport/types.js";
import {
  I2C_KHZ_DEFAULT,
  RESULT_BLOCK_ADDR,
  RESULT_BLOCK_LEN,
  VL53L4CD,
  Vl53l4cdError,
  parseResultBlock,
  rangeStatusText,
  type RangeTiming,
  type Vl53l4Platform,
  type Vl53l4Results,
} from "./uld.js";

/** One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp). */
export interface Vl53l4Measurement {
  /** MCU uptime at the INT edge (stream) / read (poll). */
  timestampUs: bigint;
  /** 0 = valid (RANGE_STATUS_NAMES). */
  rangeStatus: number;
  distanceMm: number;
  sigmaMm: number;
  signalRateKcps: number;
  ambientRateKcps: number;
  signalPerSpadKcps: number;
  ambientPerSpadKcps: number;
  numberOfSpad: number;
  /** Sensor frame counter, wraps at 255. */
  streamCount: number;
  /** rangeStatus === 0. */
  valid: boolean;
  /** Human-readable range status. */
  statusText: string;
}

function measurementFromResults(timestampUs: bigint, r: Vl53l4Results): Vl53l4Measurement {
  return {
    timestampUs,
    rangeStatus: r.rangeStatus,
    distanceMm: r.distanceMm,
    sigmaMm: r.sigmaMm,
    signalRateKcps: r.signalRateKcps,
    ambientRateKcps: r.ambientRateKcps,
    signalPerSpadKcps: r.signalPerSpadKcps,
    ambientPerSpadKcps: r.ambientPerSpadKcps,
    numberOfSpad: r.numberOfSpad,
    streamCount: r.streamCount,
    valid: r.rangeStatus === 0,
    statusText: rangeStatusText(r.rangeStatus),
  };
}

export interface Vl53l4Options extends DeviceOptions {
  /** ULD sleep implementation (tests inject an instant one). */
  sleepImpl?: (ms: number) => Promise<void>;
}

/**
 * VL53L4CD single-zone ToF device.
 *
 * `init()` runs the ULD boot sequence (no firmware blob — the sensor carries
 * its own), then configure and `startRanging()`. Configuration methods must
 * not be called while ranging: the INT-driven stream owns the register bank
 * (contract 10). `measurements` is the stream (contract 07 §3).
 */
export class Vl53l4cd extends DepzDevice {
  /**
   * The streamed measurements (contract 07 §3): `on(cb)` subscribes,
   * `subscribe(maxsize)` iterates, `next(timeoutMs)` waits for one;
   * `droppedCounts` and `parseErrors` (stream reports whose result block did
   * not decode — a short block from a reconfigured stream, a corrupt read)
   * are its counters.
   */
  readonly measurements = this.stream<Vl53l4Measurement>(64, Vl53l4Rpt.Stream);

  private readonly uldDriver: VL53L4CD;
  private rangingFlag = false;
  private initializedFlag = false;
  /** MCU timestamp of the latest register read (poll-mode measurements). */
  private lastRegTimestampUs = 0n;
  private readonly sleepImpl: (ms: number) => Promise<void>;

  /** ULD `platform` object mapped onto the firmware register bridge. */
  private readonly platform: Vl53l4Platform = {
    rdMulti: async (addr: number, size: number): Promise<Uint8Array> =>
      chunkedRead(addr, size, {
        chunkSize: XFER_MAX,
        readChunk: async (a, n) => {
          const rep = await this.request<Vl53l4RegData>(Vl53l4Cmd.ReadReg, packReadReg(a, n), {
            matcher: DepzDevice.expectReport(Vl53l4Rpt.RegData, unpackRegData),
            timeoutMs: 2000,
          });
          return { data: rep.data, timestampUs: rep.timestampUs };
        },
        onTimestamp: (ts) => {
          this.lastRegTimestampUs = ts;
        },
      }),
    wrMulti: async (addr: number, data: Uint8Array): Promise<void> =>
      chunkedWrite(addr, data, {
        chunkSize: XFER_MAX,
        writeChunk: async (a, chunk) =>
          this.request(Vl53l4Cmd.WriteReg, packWriteReg(a, chunk), {
            okCompletes: true,
            timeoutMs: 2000,
          }),
      }),
    setI2cSpeed: async (khz: number): Promise<void> => {
      await this.request(Vl53l4Cmd.SetI2cSpeed, packSetI2cSpeed(khz), { okCompletes: true });
    },
    sleepMs: async (ms: number): Promise<void> => this.sleepImpl(ms),
  };

  constructor(transport: SerialTransport, opts?: Vl53l4Options) {
    super(transport, opts);
    this.sleepImpl = opts?.sleepImpl ?? (async (ms) => new Promise((r) => setTimeout(r, ms)));
    this.uldDriver = new VL53L4CD(this.platform);
  }

  // ── lifecycle ──────────────────────────────────────────────────────────────

  /**
   * The underlying ULD driver (escape hatch for raw register access).
   * Unstable — not covered by the 1.0 freeze: the driver class and its
   * methods follow the ST ULD release the SDK ports, and the class is not
   * exported from the package root.
   */
  get uld(): VL53L4CD {
    return this.uldDriver;
  }

  /**
   * True after a successful init(). Cleared by resetSensor() and xshut() — a
   * power-cycled sensor holds none of the ULD configuration.
   */
  get initialized(): boolean {
    return this.initializedFlag;
  }

  /** True when the sensor answers with the VL53L4CD model id (0xEBAA). */
  async isAlive(): Promise<boolean> {
    try {
      return await this.uldDriver.isAlive();
    } catch (err) {
      if (err instanceof Vl53l4cdError || err instanceof DepzError) return false;
      throw err;
    }
  }

  /**
   * Initialise the sensor: default configuration block + VHV calibration
   * (ULD sensorInit). Takes well under a second; the bus is left at `busKhz`
   * (one of I2C_KHZ_STEPS).
   */
  async init(busKhz: number = I2C_KHZ_DEFAULT): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.sensorInit(busKhz);
    this.initializedFlag = true;
  }

  // ── sensor power (XSHUT pin) ───────────────────────────────────────────────

  /**
   * Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and RESET
   * stop any active stream on the bridge; a power-cycled sensor needs init()
   * again.
   */
  async xshut(action: number): Promise<void> {
    const timeoutMs = action === XSHUT_RESET ? 2000 : undefined;
    await this.request(Vl53l4Cmd.Xshut, packXshut(action), { okCompletes: true, timeoutMs });
    this.rangingFlag = false;
    this.initializedFlag = false;
  }

  /**
   * Hardware sensor reset via XSHUT (blocks ≤ 20 ms on the MCU, typ. ~2 ms;
   * the reply is awaited up to 2 s). The ULD
   * configuration is wiped — call init() again.
   */
  async resetSensor(): Promise<void> {
    await this.xshut(XSHUT_RESET);
  }

  // ── bridge diagnostics ─────────────────────────────────────────────────────

  /**
   * RPT_VL53_INFO: sensor identity, pin levels and bridge counters. Counters
   * wrap silently — watch increments; `i2cErrors` is cleared by a successful
   * XSHUT reset. Safe to call while streaming.
   */
  async bridgeInfo(): Promise<Vl53l4Info> {
    return this.request(Vl53l4Cmd.GetInfo, undefined, {
      matcher: DepzDevice.expectReport(Vl53l4Rpt.Info, unpackInfo),
    });
  }

  /**
   * Re-time the bridge's I2C bus to the nominal step nearest `khz`
   * (I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
   * flight (ERR_BUSY). Read back the programmed step via bridgeInfo().
   */
  async setI2cSpeedKhz(khz: number): Promise<void> {
    this.requireNotRanging();
    await this.platform.setI2cSpeed(khz);
  }

  // ── configuration (init() first; not while ranging) ────────────────────────

  /**
   * → { timingBudgetMs, interMeasurementMs }. interMeasurement 0 means
   * continuous mode.
   */
  async getRangeTiming(): Promise<RangeTiming> {
    return this.uldDriver.getRangeTiming();
  }

  /**
   * Set the timing budget (10–200 ms) and inter-measurement period.
   * `interMeasurementMs = 0` selects continuous ranging; a value larger than
   * the budget selects autonomous low-power mode. Not while ranging.
   */
  async setRangeTiming(timingBudgetMs: number, interMeasurementMs = 0): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.setRangeTiming(timingBudgetMs, interMeasurementMs);
  }

  /** Configured ranging offset in mm (signed). */
  async getOffsetMm(): Promise<number> {
    return this.uldDriver.getOffset();
  }

  /** Set the ranging offset correction in mm. Not while ranging. */
  async setOffsetMm(offsetMm: number): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.setOffset(offsetMm);
  }

  /** Configured crosstalk compensation in kcps (0 = disabled). */
  async getXtalkKcps(): Promise<number> {
    return this.uldDriver.getXtalk();
  }

  /** Set the crosstalk compensation in kcps. Not while ranging. */
  async setXtalkKcps(xtalkKcps: number): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.setXtalk(xtalkKcps);
  }

  /**
   * → { distanceLowMm, distanceHighMm, window }. Window is one of
   * WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN.
   */
  async getDetectionThresholds(): Promise<{
    distanceLowMm: number;
    distanceHighMm: number;
    window: number;
  }> {
    return this.uldDriver.getDetectionThresholds();
  }

  /**
   * Program the distance-window interrupt (INT only fires when the window
   * condition holds). Not while ranging. The distances are u16 registers, the
   * window a u8 one: a value outside is refused before anything is written,
   * never masked (contract 07 §3).
   */
  async setDetectionThresholds(
    distanceLowMm: number,
    distanceHighMm: number,
    window: number,
  ): Promise<void> {
    checkUint("distanceLowMm", distanceLowMm, 16);
    checkUint("distanceHighMm", distanceHighMm, 16);
    checkUint("window", window, 8);
    this.requireNotRanging();
    await this.uldDriver.setDetectionThresholds(distanceLowMm, distanceHighMm, window);
  }

  async getSignalThresholdKcps(): Promise<number> {
    return this.uldDriver.getSignalThreshold();
  }

  /**
   * Discard measurements whose return signal is below `signalKcps`. Not
   * while ranging.
   */
  async setSignalThresholdKcps(signalKcps: number): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.setSignalThreshold(signalKcps);
  }

  async getSigmaThresholdMm(): Promise<number> {
    return this.uldDriver.getSigmaThreshold();
  }

  /**
   * Discard measurements whose sigma exceeds `sigmaMm` (≤ 16383). Not while
   * ranging.
   */
  async setSigmaThresholdMm(sigmaMm: number): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.setSigmaThreshold(sigmaMm);
  }

  /**
   * Re-run VHV calibration; recommended after a >8 °C ambient change. Not
   * while ranging (runs a short ranging burst internally).
   */
  async startTemperatureUpdate(): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.startTemperatureUpdate();
  }

  /**
   * Offset calibration against a target at `targetDistMm` (10–1000). Blocks
   * for the sample burst; returns the offset now programmed.
   */
  async calibrateOffset(targetDistMm: number, nbSamples = 20): Promise<number> {
    this.requireNotRanging();
    return this.uldDriver.calibrateOffset(targetDistMm, nbSamples);
  }

  /**
   * Crosstalk calibration against a target at `targetDistMm` (10–5000).
   * Blocks for the sample burst; returns the xtalk now programmed (kcps).
   */
  async calibrateXtalk(targetDistMm: number, nbSamples = 20): Promise<number> {
    this.requireNotRanging();
    return this.uldDriver.calibrateXtalk(targetDistMm, nbSamples);
  }

  // ── ranging ────────────────────────────────────────────────────────────────

  /**
   * Start the sensor's ranging loop and arm the MCU stream: one
   * RPT_VL53_STREAM per INT edge carrying the 17-byte result block.
   */
  async startRanging(): Promise<void> {
    this.requireNotRanging();
    await this.uldDriver.startRanging();
    try {
      await this.request(
        Vl53l4Cmd.StartStream,
        packStartStream(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN),
        { okCompletes: true },
      );
    } catch (e) {
      // The sensor ranges but the host will say it does not: stop it again,
      // or stopRanging() could never reach it. The START_STREAM error is the
      // one the caller gets, even if this stop fails too.
      await this.uldDriver.stopRanging().catch(() => undefined);
      throw e;
    }
    this.rangingFlag = true;
  }

  async stopRanging(): Promise<void> {
    if (!this.rangingFlag) return;
    // Clear host state and stop the sensor even if the MCU STOP_STREAM ack
    // fails (link hiccup): otherwise the device is wedged "ranging" and no
    // reconfiguration is possible.
    try {
      await this.request(Vl53l4Cmd.StopStream, undefined, { okCompletes: true });
    } finally {
      this.rangingFlag = false;
      await this.uldDriver.stopRanging();
    }
  }

  get ranging(): boolean {
    return this.rangingFlag;
  }

  /**
   * Single poll-mode measurement: start ranging, wait for data-ready, read
   * the result block, stop. Rejects while the stream is running.
   */
  async measureOnce(timeoutMs = 1000): Promise<Vl53l4Measurement> {
    this.requireNotRanging();
    await this.uldDriver.startRanging();
    let result: Vl53l4Results;
    try {
      await this.uldDriver.waitDataReady(timeoutMs);
      result = await this.uldDriver.getResult();
      await this.uldDriver.clearInterrupt();
    } finally {
      await this.uldDriver.stopRanging();
    }
    return measurementFromResults(this.lastRegTimestampUs, result);
  }

  // ── internal ───────────────────────────────────────────────────────────────

  protected requireNotRanging(): void {
    this.requireOpen();
    if (this.rangingFlag) {
      throw new InvalidStateError("stopRanging() first — the stream owns the register bank");
    }
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Vl53l4Rpt.Stream) return false;
    let sample: ReturnType<typeof unpackStream>;
    let result: Vl53l4Results;
    try {
      // short header or body: refused (RangeError) and counted — contract 07 §2
      sample = unpackStream(pkt.payload);
      result = parseResultBlock(sample.data);
    } catch {
      // Somebody re-armed the stream on a different block — count, don't
      // crash the read pump.
      this.measurements.parseErrors += 1;
      return true;
    }
    this.measurements.emit(measurementFromResults(sample.timestampUs, result));
    return true;
  }
}
