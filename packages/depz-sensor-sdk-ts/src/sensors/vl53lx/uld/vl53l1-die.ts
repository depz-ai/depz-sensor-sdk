/**
 * uld/vl53l1-die — the VL53L1 die: register map, result block, and the
 * VL53L4CD ULD body. Async port of the Python
 * `depz_sensor_sdk.vl53lx.uld.vl53l1_die`.
 *
 * The 17-byte result block at 0x0089, the range-status table, the
 * register-address width, the interrupt-release write and the bus ceiling are
 * what every part built on this die has in common. `VL53L1Die` is the body of
 * the VL53L4CD ULD, shared by `l4.ts` (VL53L4CD/CX) and `l3.ts` (VL53L3CX
 * ULP); `l1.ts` (VL53L1X ULD) overrides the ranging configuration.
 *
 * Register sequences and integer widths are the C driver's — do not
 * "simplify" them.
 */

import { I2C_KHZ_BOOT, ProtocolError, Vl53Error } from "./link.js";
import { measurement, SensorDriver, type Measurement } from "./base.js";
import { beUint, floorDiv, pyRound, u32 } from "./arith.js";

export const SOFT_RESET = 0x0000;
export const I2C_SLAVE__DEVICE_ADDRESS = 0x0001;
export const OSC_FREQUENCY = 0x0006;
export const VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND = 0x0008;
export const XTALK_PLANE_OFFSET_KCPS = 0x0016;
export const XTALK_X_PLANE_GRADIENT_KCPS = 0x0018;
export const XTALK_Y_PLANE_GRADIENT_KCPS = 0x001a;
export const RANGE_OFFSET_MM = 0x001e;
export const INNER_OFFSET_MM = 0x0020;
export const OUTER_OFFSET_MM = 0x0022;
export const GPIO_HV_MUX__CTRL = 0x0030;
export const GPIO__TIO_HV_STATUS = 0x0031;
export const SYSTEM__INTERRUPT_CONFIG_GPIO = 0x0046;
export const PHASECAL_CONFIG__TIMEOUT_MACROP = 0x004b;
export const RANGE_CONFIG__TIMEOUT_MACROP_A_HI = 0x005e;
export const RANGE_CONFIG__VCSEL_PERIOD_A = 0x0060;
export const RANGE_CONFIG__TIMEOUT_MACROP_B_HI = 0x0061;
export const RANGE_CONFIG__VCSEL_PERIOD_B = 0x0063;
export const RANGE_CONFIG__SIGMA_THRESH = 0x0064;
export const MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066;
export const RANGE_CONFIG__VALID_PHASE_HIGH = 0x0069;
export const INTERMEASUREMENT_MS = 0x006c;
export const THRESH_HIGH = 0x0072;
export const THRESH_LOW = 0x0074;
export const SD_CONFIG__WOI_SD0 = 0x0078;
export const SD_CONFIG__INITIAL_PHASE_SD0 = 0x007a;
export const ROI_CONFIG__USER_ROI_CENTRE_SPAD = 0x007f;
export const ROI_CONFIG__USER_ROI_XY_SIZE = 0x0080;
export const SYSTEM__INTERRUPT_CLEAR = 0x0086;
export const SYSTEM__MODE_START = 0x0087;
export const RESULT__RANGE_STATUS = 0x0089;
export const RESULT__SPAD_NB = 0x008c;
export const RESULT__SIGNAL_RATE = 0x008e;
export const RESULT__AMBIENT_RATE = 0x0090;
export const RESULT__SIGMA = 0x0092;
export const RESULT__DISTANCE = 0x0096;
export const RESULT__OSC_CALIBRATE_VAL = 0x00de;
export const FIRMWARE__SYSTEM_STATUS = 0x00e5;
export const IDENTIFICATION__MODEL_ID = 0x010f;
export const ROI_CONFIG__MODE_ROI_CENTRE_SPAD = 0x013e;

/** Detection-threshold window modes (SYSTEM__INTERRUPT_CONFIG_GPIO). */
export const WINDOW_BELOW = 0;
export const WINDOW_ABOVE = 1;
export const WINDOW_OUT = 2;
export const WINDOW_IN = 3;

/** The configuration blob spans 0x2D..0x87 on every part of the die. */
export const CONFIG_ADDR = 0x002d;
export const CONFIG_END = 0x0087;
/** Byte 0 of the blob: Fast Mode Plus on the sensor pad, set unconditionally. */
export const CONFIG_FMP_BYTE = 0x12;

/** The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099. */
export const RESULT_BLOCK_ADDR = RESULT__RANGE_STATUS;
export const RESULT_BLOCK_LEN = 17;

/** How this die releases its interrupt: a single write. */
export const DIE_CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = [
  [SYSTEM__INTERRUPT_CLEAR, 0x01],
];

/** GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c). */
export const STATUS_RTN: readonly number[] = [
  255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11,
  12,
];

/** UM2931, "Range status description". */
export const DIE_RANGE_STATUS_NAMES: Readonly<Record<number, string>> = {
  0: "valid",
  1: "sigma above threshold",
  2: "signal below threshold",
  3: "distance below detection threshold",
  4: "phase out of valid limit",
  5: "hardware fail",
  6: "no wrap-around check done",
  7: "wrapped target, phase mismatch",
  8: "processing fail",
  9: "crosstalk signal fail",
  10: "interrupt error",
  11: "merged target",
  12: "signal too low",
  255: "other error",
};

export function dieStatusText(status: number): string {
  return DIE_RANGE_STATUS_NAMES[status] ?? `unknown (${status})`;
}

/** VL53L4CD_ResultsData_t (plus the sensor's own frame counter). */
export interface DieResultsData {
  rangeStatus: number;
  distanceMm: number;
  ambientRateKcps: number;
  ambientPerSpadKcps: number;
  signalRateKcps: number;
  signalPerSpadKcps: number;
  numberOfSpad: number;
  sigmaMm: number;
  streamCount: number;
}

/**
 * Decode the 17-byte block as a ULD of this die reads it. `signalAt` is the
 * offset of the signal rate (5 = 0x008E for the VL53L4CD ULD, 15 = 0x0098 for
 * the VL53L1X ULD) and `perSpadK` the per-SPAD scale (256 / 25).
 */
export function parseDieBlock(raw: Uint8Array, signalAt: number, perSpadK: number): DieResultsData {
  if (raw.length < RESULT_BLOCK_LEN) {
    throw new Vl53Error(`result block too short: ${raw.length} bytes`);
  }
  let status = raw[0]! & 0x1f;
  if (status < STATUS_RTN.length) status = STATUS_RTN[status]!;
  const rawSpads = beUint(raw, 3, 5); // 0x008C, 8.8
  const signalKcps = beUint(raw, signalAt, signalAt + 2) * 8;
  const ambientKcps = beUint(raw, 7, 9) * 8; // 0x0090
  return {
    rangeStatus: status,
    streamCount: raw[2]!,
    numberOfSpad: floorDiv(rawSpads, 256),
    signalRateKcps: signalKcps,
    ambientRateKcps: ambientKcps,
    sigmaMm: floorDiv(beUint(raw, 9, 11), 4),
    distanceMm: beUint(raw, 13, 15), // 0x0096
    signalPerSpadKcps: rawSpads ? floorDiv(signalKcps * perSpadK, rawSpads) : 0,
    ambientPerSpadKcps: rawSpads ? floorDiv(ambientKcps * perSpadK, rawSpads) : 0,
  };
}

/** ResultsData → the family-wide Measurement every caller speaks. */
export function dieAsMeasurement(r: DieResultsData): Measurement {
  return measurement({
    distanceMm: r.distanceMm,
    status: r.rangeStatus,
    statusText: dieStatusText(r.rangeStatus),
    signalKcps: r.signalRateKcps,
    ambientKcps: r.ambientRateKcps,
    sigmaMm: r.sigmaMm,
    spads: r.numberOfSpad,
    extra: {
      signal_per_spad_kcps: r.signalPerSpadKcps,
      ambient_per_spad_kcps: r.ambientPerSpadKcps,
      stream_count: r.streamCount,
    },
  });
}

/** 2304 × (2^30 / osc) wrapped to u32, then >> 6 (both ULDs' macro period). */
function macroPeriodUs(oscFrequency: number): number {
  return floorDiv(u32(2304 * floorDiv(0x40000000, oscFrequency)), 64);
}

/**
 * The body every driver of this die shares: identity, boot, the ranging
 * loop, the interrupt, the threshold, offset, crosstalk and ROI registers.
 * A concrete product supplies `CONFIGURATION`, `SUPPORTS` and its own hooks.
 */
export abstract class VL53L1Die extends SensorDriver {
  static readonly ADDR_WIDTH: number = 2;
  static readonly CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = DIE_CLEAR_STEPS;
  /** With Fast Mode Plus on the sensor pad. */
  static readonly MAX_KHZ: number = 1000;

  /** The 91-byte blob written at CONFIG_ADDR, product-specific. */
  protected abstract readonly CONFIGURATION: Uint8Array;
  /** What StopRanging writes: the L4CD ULD 0x80, the L3CX ULP / L1X ULD 0x00. */
  protected readonly STOP_MODE: number = 0x80;
  /** Signal-rate offset in the block: 5 (0x008E, L4CD ULD) or 15 (0x0098, L1X ULD). */
  protected readonly SIGNAL_AT: number = 5;
  /** Per-SPAD scale: 256 (L4CD ULD) or 25 (L1X ULD, 200.0/8). */
  protected readonly PER_SPAD_K: number = 256;

  // ── result block ──
  streamBlock(): [number, number] {
    return [RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN];
  }

  parseResultBlock(raw: Uint8Array): DieResultsData {
    return parseDieBlock(raw, this.SIGNAL_AT, this.PER_SPAD_K);
  }

  decode(raw: Uint8Array): Measurement {
    return dieAsMeasurement(this.parseResultBlock(raw));
  }

  /** One block read instead of the C driver's six register reads. */
  async getResult(): Promise<DieResultsData> {
    return this.parseResultBlock(await this.p.rdMulti(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN));
  }

  override async readMeasurement(): Promise<Measurement> {
    return dieAsMeasurement(await this.getResult());
  }

  // ── identity ──
  modelId(): Promise<number> {
    return this.p.rdWord(IDENTIFICATION__MODEL_ID);
  }

  /** The bridge always addresses 0x29, so this makes the sensor unreachable. */
  setI2cAddress(newAddress: number): Promise<void> {
    return this.p.wrByte(I2C_SLAVE__DEVICE_ADDRESS, newAddress >> 1);
  }

  // ── init ──
  bootState(): Promise<number> {
    return this.p.rdByte(FIRMWARE__SYSTEM_STATUS);
  }

  async waitBoot(timeoutS = 1.0): Promise<void> {
    const deadline = Date.now() + timeoutS * 1000;
    for (;;) {
      if ((await this.bootState()) === 0x03) return;
      if (Date.now() > deadline) {
        throw new Vl53Error("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
      }
      await this.p.sleepMs(1);
    }
  }

  /**
   * Pull the die through its soft reset (VL53L1_software_reset()); a part that
   * stopped ACKing is reset through XSHUT instead.
   */
  async resetDevice(timeoutS = 1.0): Promise<void> {
    try {
      await this.p.wrByte(SOFT_RESET, 0x00);
      await this.p.sleepMs(1);
      await this.p.wrByte(SOFT_RESET, 0x01);
      await this.waitBoot(timeoutS);
    } catch (err) {
      if (!(err instanceof Vl53Error || err instanceof ProtocolError)) throw err;
      await this.p.xshutReset();
      await this.waitBoot(timeoutS);
    }
  }

  /** Initialise the sensor and leave the bus at this die's ceiling. */
  async sensorInit(): Promise<void> {
    await this.p.setAddrWidth(this.ADDR_WIDTH);
    await this.p.setI2cSpeed(I2C_KHZ_BOOT);
    await this.initBoot();

    const config = new Uint8Array(this.CONFIGURATION);
    config[0] = CONFIG_FMP_BYTE;
    await this.p.wrMulti(CONFIG_ADDR, config);
    if (this.MAX_KHZ !== I2C_KHZ_BOOT) await this.p.setI2cSpeed(this.MAX_KHZ);
    await this.initAfterConfig();

    await this.p.wrByte(SYSTEM__MODE_START, 0x40); // start VHV
    await this.waitDataReady();
    await this.clearInterrupt();
    await this.stopRanging();
    await this.p.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09);
    await this.p.wrByte(0x000b, 0x00);
    await this.initExtra();

    await this.setRangeTiming(50, 0);
  }

  /** How a part gets to a booted die (the VL53L1X ULD resets first). */
  protected initBoot(): Promise<void> {
    return this.waitBoot();
  }

  /** Writes right after the blob, before VHV (only the L1X ULD has one). */
  protected async initAfterConfig(): Promise<void> {}

  /** Writes at the end of init, before the timing is set. */
  protected async initExtra(): Promise<void> {}

  // ── ranging ──
  clearInterrupt(): Promise<void> {
    return this.p.wrByte(SYSTEM__INTERRUPT_CLEAR, 0x01);
  }

  async startRanging(): Promise<void> {
    // 0 = continuous, anything else = autonomous low power.
    const mode = (await this.p.rdDword(INTERMEASUREMENT_MS)) === 0 ? 0x21 : 0x40;
    await this.p.wrByte(SYSTEM__MODE_START, mode);
  }

  stopRanging(): Promise<void> {
    return this.p.wrByte(SYSTEM__MODE_START, this.STOP_MODE);
  }

  async getInterruptPolarity(): Promise<number> {
    return (await this.p.rdByte(GPIO_HV_MUX__CTRL)) & 0x10 ? 0 : 1;
  }

  async setInterruptPolarity(polarity: number): Promise<void> {
    const temp = (await this.p.rdByte(GPIO_HV_MUX__CTRL)) & 0xef;
    await this.p.wrByte(GPIO_HV_MUX__CTRL, temp | ((polarity & 1 ? 0 : 1) << 4));
  }

  async checkForDataReady(): Promise<boolean> {
    // Polarity first, as both C drivers read it.
    const intPol = await this.getInterruptPolarity();
    return ((await this.p.rdByte(GPIO__TIO_HV_STATUS)) & 1) === intPol;
  }

  // ── timing ──
  async setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void> {
    const oscFrequency = await this.p.rdWord(OSC_FREQUENCY);
    if (oscFrequency === 0) throw new Vl53Error("osc_frequency reads 0");
    if (!(timingBudgetMs >= 10 && timingBudgetMs <= 200)) {
      throw new Vl53Error("timing_budget_ms must be 10..200");
    }

    let timingBudgetUs = timingBudgetMs * 1000;
    const macroPeriod = macroPeriodUs(oscFrequency);

    if (interMeasurementMs === 0) {
      // continuous
      await this.p.wrDword(INTERMEASUREMENT_MS, 0);
      timingBudgetUs -= 2500;
    } else if (interMeasurementMs > timingBudgetMs) {
      // autonomous low power
      const clockPll = (await this.p.rdWord(RESULT__OSC_CALIBRATE_VAL)) & 0x3ff;
      const factor = 1.055 * interMeasurementMs * clockPll;
      await this.p.wrDword(INTERMEASUREMENT_MS, Math.trunc(factor));
      timingBudgetUs = floorDiv(timingBudgetUs - 4300, 2);
    } else {
      throw new Vl53Error("inter_measurement_ms must be 0 or > timing_budget_ms");
    }

    timingBudgetUs = u32(timingBudgetUs * 4096);
    for (const [reg, mult] of [
      [RANGE_CONFIG__TIMEOUT_MACROP_A_HI, 16],
      [RANGE_CONFIG__TIMEOUT_MACROP_B_HI, 12],
    ] as const) {
      const tmp = floorDiv(u32(macroPeriod * mult), 64);
      let lsByte = floorDiv(timingBudgetUs + floorDiv(tmp, 2), tmp) - 1;
      let msByte = 0;
      while (lsByte >= 0x100) {
        lsByte = floorDiv(lsByte, 2);
        msByte += 1;
      }
      await this.p.wrWord(reg, (msByte * 256 + (lsByte & 0xff)) & 0xffff);
    }
  }

  /** → [timingBudgetMs, interMeasurementMs]. */
  async getRangeTiming(): Promise<[number, number]> {
    const tmp = await this.p.rdDword(INTERMEASUREMENT_MS);
    let clockPll = (await this.p.rdWord(RESULT__OSC_CALIBRATE_VAL)) & 0x3ff;
    clockPll = Math.trunc(1.065 * clockPll) & 0xffff;
    const interMeasurementMs = clockPll ? floorDiv(tmp, clockPll) & 0xffff : 0;

    const oscFrequency = await this.p.rdWord(OSC_FREQUENCY);
    if (oscFrequency === 0) throw new Vl53Error("osc_frequency reads 0");
    const macropHigh = await this.p.rdWord(RANGE_CONFIG__TIMEOUT_MACROP_A_HI);

    let macroPeriod = macroPeriodUs(oscFrequency);
    const lsByte = (macropHigh & 0x00ff) << 4;
    let msByte = (macropHigh & 0xff00) >> 8;
    msByte = u32(0x04 - (msByte - 1) - 1);
    macroPeriod = u32(macroPeriod * 16);

    const mp6 = floorDiv(macroPeriod, 64);
    let budget = floorDiv(u32((lsByte + 1) * mp6 - floorDiv(mp6, 2)), 4096);
    if (msByte < 12) budget = floorDiv(budget, 2 ** msByte);
    budget = tmp === 0 ? budget + 2500 : budget * 2 + 4300;
    return [floorDiv(budget, 1000), interMeasurementMs];
  }

  // ── thresholds ──
  async setDetectionThresholds(
    distanceLowMm: number,
    distanceHighMm: number,
    window: number,
  ): Promise<void> {
    await this.p.wrByte(SYSTEM__INTERRUPT_CONFIG_GPIO, window);
    await this.p.wrWord(THRESH_HIGH, distanceHighMm);
    await this.p.wrWord(THRESH_LOW, distanceLowMm);
  }

  /** → [distanceLowMm, distanceHighMm, window]. */
  async getDetectionThresholds(): Promise<[number, number, number]> {
    const high = await this.p.rdWord(THRESH_HIGH);
    const low = await this.p.rdWord(THRESH_LOW);
    return [low, high, (await this.p.rdByte(SYSTEM__INTERRUPT_CONFIG_GPIO)) & 0x07];
  }

  setSignalThreshold(signalKcps: number): Promise<void> {
    return this.p.wrWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS, signalKcps >> 3);
  }

  async getSignalThreshold(): Promise<number> {
    return ((await this.p.rdWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS)) << 3) & 0xffff;
  }

  async setSigmaThreshold(sigmaMm: number): Promise<void> {
    if (sigmaMm > 0xffff >> 2) throw new Vl53Error("sigma_mm must be <= 16383");
    await this.p.wrWord(RANGE_CONFIG__SIGMA_THRESH, sigmaMm << 2);
  }

  async getSigmaThreshold(): Promise<number> {
    return (await this.p.rdWord(RANGE_CONFIG__SIGMA_THRESH)) >> 2;
  }

  // ── offset ──
  async setOffset(offsetMm: number): Promise<void> {
    await this.p.wrWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xffff);
    await this.p.wrWord(INNER_OFFSET_MM, 0);
    await this.p.wrWord(OUTER_OFFSET_MM, 0);
  }

  async getOffset(): Promise<number> {
    const temp = (((await this.p.rdWord(RANGE_OFFSET_MM)) << 3) & 0xffff) >> 5;
    return temp > 1024 ? temp - 2048 : temp;
  }

  // ── crosstalk (kcps on both ULDs) ──
  async setXtalk(xtalkKcps: number): Promise<void> {
    await this.p.wrWord(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000);
    await this.p.wrWord(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000);
    await this.p.wrWord(XTALK_PLANE_OFFSET_KCPS, (xtalkKcps << 9) & 0xffff);
  }

  async getXtalk(): Promise<number> {
    return pyRound((await this.p.rdWord(XTALK_PLANE_OFFSET_KCPS)) / 512.0);
  }

  // ── ROI (VL53L1X_SetROI / VL53L3CX_ULP_SetROI, the same registers) ──
  /** An X by Y window of SPADs, 4..16 each way. */
  async setRoi(x: number, y: number): Promise<void> {
    let opticalCenter = await this.p.rdByte(ROI_CONFIG__MODE_ROI_CENTRE_SPAD);
    x = Math.min(x, 16);
    y = Math.min(y, 16);
    if (x > 10 || y > 10) opticalCenter = 199;
    await this.p.wrByte(ROI_CONFIG__USER_ROI_CENTRE_SPAD, opticalCenter);
    await this.p.wrByte(ROI_CONFIG__USER_ROI_XY_SIZE, ((y - 1) << 4) | (x - 1));
  }

  /** → [x, y]. */
  async getRoi(): Promise<[number, number]> {
    const temp = await this.p.rdByte(ROI_CONFIG__USER_ROI_XY_SIZE);
    return [(temp & 0x0f) + 1, ((temp & 0xf0) >> 4) + 1];
  }

  setRoiCenter(centerSpad: number): Promise<void> {
    return this.p.wrByte(ROI_CONFIG__USER_ROI_CENTRE_SPAD, centerSpad);
  }

  getRoiCenter(): Promise<number> {
    return this.p.rdByte(ROI_CONFIG__USER_ROI_CENTRE_SPAD);
  }

  // ── temperature ──
  /** Recommended after a >8 °C ambient change. */
  async startTemperatureUpdate(): Promise<void> {
    await this.p.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81); // full VHV
    await this.p.wrByte(0x000b, 0x92);
    await this.startRanging();
    await this.waitDataReady();
    await this.clearInterrupt();
    await this.stopRanging();
    await this.p.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09);
    await this.p.wrByte(0x000b, 0x00);
  }

  // ── calibration ──
  /**
   * The ranging loop every calibration shares: data-ready, GetResult,
   * ClearInterrupt, `nbSamples` times.
   */
  protected async collect(
    nbSamples: number,
    onSample: (i: number, r: DieResultsData) => void,
    timeoutS = 5.0,
  ): Promise<void> {
    await this.startRanging();
    for (let i = 0; i < nbSamples; i++) {
      await this.waitDataReady(timeoutS);
      const result = await this.getResult();
      await this.clearInterrupt();
      onSample(i, result);
    }
    await this.stopRanging();
  }
}
