/**
 * VL53L4CD ULD driver (ST STSW-IMG026 2.2.3) over the register bridge.
 *
 * Port of `VL53L4CD_api.c` + `VL53L4CD_calibration.c` — a 1:1 async mirror of
 * the Python reference `depz_sensor_sdk.vl53l4.uld` (itself absorbed from the
 * firmware repo's hardware-proven port). The MCU owns nothing but the I2C
 * bus, XSHUT, INT and one streaming FSM — every register sequence below goes
 * over VL53_READ_REG / VL53_WRITE_REG (contracts/10_SENSOR_VL53L4.md §1).
 *
 * Register sequences are a faithful port of the C code, integer widths and
 * 32-bit truncations included; do not "simplify" them. Pure codec/math pieces
 * (`parseResultBlock`, `rangeTimingRegisters`, `decodeRangeTiming`,
 * `configBlock`, the tuning codecs) are module functions so the golden
 * vectors can hold them to byte-exact parity with the other SDKs.
 */

export const ULD_VERSION: readonly [number, number, number, number] = [2, 2, 3, 0];

// ── Registers (VL53L4CD_api.h) ───────────────────────────────────────────────
export const SOFT_RESET = 0x0000;
export const I2C_SLAVE__DEVICE_ADDRESS = 0x0001;
/** Unnamed in the C driver. */
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
export const SYSTEM__INTERRUPT = 0x0046;
export const RANGE_CONFIG_A = 0x005e;
export const RANGE_CONFIG_B = 0x0061;
export const RANGE_CONFIG__SIGMA_THRESH = 0x0064;
export const MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066;
export const INTERMEASUREMENT_MS = 0x006c;
export const THRESH_HIGH = 0x0072;
export const THRESH_LOW = 0x0074;
export const SYSTEM__INTERRUPT_CLEAR = 0x0086;
export const SYSTEM_START = 0x0087;
export const RESULT__RANGE_STATUS = 0x0089;
export const RESULT__SPAD_NB = 0x008c;
export const RESULT__SIGNAL_RATE = 0x008e;
export const RESULT__AMBIENT_RATE = 0x0090;
export const RESULT__SIGMA = 0x0092;
export const RESULT__DISTANCE = 0x0096;
export const RESULT__OSC_CALIBRATE_VAL = 0x00de;
export const FIRMWARE__SYSTEM_STATUS = 0x00e5;
export const IDENTIFICATION__MODEL_ID = 0x010f;

export const MODEL_ID_VL53L4CD = 0xebaa;

// Detection-threshold window modes (SYSTEM__INTERRUPT).
export const WINDOW_BELOW = 0;
export const WINDOW_ABOVE = 1;
export const WINDOW_OUT = 2;
export const WINDOW_IN = 3;

export const CONFIG_ADDR = 0x002d;
export const CONFIG_END = 0x0087;

/**
 * VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
 * `configBlock()` always overrides byte 0 (register 0x2D) with
 * CONFIG_FMP_BYTE (0x12) to put the sensor's I2C pad in Fast Mode Plus —
 * exactly what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD. FM+ pads work
 * at every bus step down to 100 kHz, so it is set unconditionally and never
 * cleared (clearing it mid-block NACKs and truncates the write).
 */
// prettier-ignore
export const DEFAULT_CONFIGURATION: Uint8Array = Uint8Array.from([
  0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   // 0x2D..0x34
  0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   // 0x35..0x3C
  0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,   // 0x3D..0x44
  0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   // 0x45..0x4C
  0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   // 0x4D..0x54
  0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   // 0x55..0x5C
  0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,   // 0x5D..0x64
  0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,   // 0x65..0x6C
  0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   // 0x6D..0x74
  0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   // 0x75..0x7C
  0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00,   // 0x7D..0x84
  0x01, 0x00, 0x00,                                 // 0x85..0x87
]);

export const CONFIG_FMP_BYTE = 0x12;

// The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099 — every field of
// VL53L4CD_ResultsData_t in one read.
export const RESULT_BLOCK_ADDR = RESULT__RANGE_STATUS;
export const RESULT_BLOCK_LEN = 17;

// The bridge boots at 400 kHz and an unconfigured sensor is only specified
// for that speed, so init always runs its configuration block there. The bus
// is left at I2C_KHZ_DEFAULT afterwards (the block read is ~4x faster at 1 MHz).
export const I2C_KHZ_BOOT = 400;
export const I2C_KHZ_DEFAULT = 1000;

/** GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c). */
// prettier-ignore
export const STATUS_RTN: readonly number[] = [
  255, 255, 255, 5, 2, 4, 1, 7, 3,
  0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6,
  255, 255, 11, 12,
];

/** UM2931, "Range status description". */
export const RANGE_STATUS_NAMES: Readonly<Record<number, string>> = {
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

export class Vl53l4cdError extends Error {
  constructor(message?: string) {
    super(message);
    this.name = "Vl53l4cdError";
  }
}

/** What the ULD needs from the register bridge. */
export interface Vl53l4Platform {
  rdMulti(addr: number, size: number): Promise<Uint8Array>;
  wrMulti(addr: number, data: Uint8Array): Promise<void>;
  setI2cSpeed(khz: number): Promise<void>;
  sleepMs(ms: number): Promise<void>;
}

/** VL53L4CD_ResultsData_t plus the sensor's own frame counter. */
export interface Vl53l4Results {
  /** 0 = valid (RANGE_STATUS_NAMES). */
  rangeStatus: number;
  distanceMm: number;
  ambientRateKcps: number;
  ambientPerSpadKcps: number;
  signalRateKcps: number;
  signalPerSpadKcps: number;
  numberOfSpad: number;
  sigmaMm: number;
  /**
   * 0x008B RESULT__STREAM_COUNT: wraps at 255. The C ULD ignores it; it is
   * what tells a frame the host never received from one the sensor never
   * produced.
   */
  streamCount: number;
}

/** Human-readable text for a decoded range status. */
export function rangeStatusText(rangeStatus: number): string {
  return RANGE_STATUS_NAMES[rangeStatus] ?? `unknown (${rangeStatus})`;
}

/** Exact 32-bit truncation (`& 0xFFFFFFFF` in the Python reference). */
function u32(x: number): number {
  return x >>> 0;
}

function beWord(raw: Uint8Array, off: number): number {
  return ((raw[off]! << 8) | raw[off + 1]!) & 0xffff;
}

/**
 * Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
 * decodes the same registers read one by one. Register contents are
 * big-endian words (the bridge passes them through untouched).
 */
export function parseResultBlock(raw: Uint8Array): Vl53l4Results {
  if (raw.length < 15) {
    throw new Vl53l4cdError(`result block too short: ${raw.length} bytes`);
  }

  let status = raw[0]! & 0x1f;
  if (status < STATUS_RTN.length) {
    status = STATUS_RTN[status]!;
  }

  const rawSpads = beWord(raw, 3); // 0x008C
  const signalKcps = beWord(raw, 5) * 8; // 0x008E
  const ambientKcps = beWord(raw, 7) * 8; // 0x0090

  return {
    rangeStatus: status,
    streamCount: raw[2]!,
    numberOfSpad: Math.floor(rawSpads / 256),
    signalRateKcps: signalKcps,
    ambientRateKcps: ambientKcps,
    sigmaMm: Math.floor(beWord(raw, 9) / 4), // 0x0092
    distanceMm: beWord(raw, 13), // 0x0096
    signalPerSpadKcps: rawSpads !== 0 ? Math.floor((signalKcps * 256) / rawSpads) : 0,
    ambientPerSpadKcps: rawSpads !== 0 ? Math.floor((ambientKcps * 256) / rawSpads) : 0,
  };
}

/**
 * The 91-byte block sensorInit() writes at CONFIG_ADDR: the ST default
 * configuration with byte 0 forced to CONFIG_FMP_BYTE (Fast Mode Plus).
 */
export function configBlock(): Uint8Array {
  const out = DEFAULT_CONFIGURATION.slice();
  out[0] = CONFIG_FMP_BYTE;
  return out;
}

/** SetRangeTiming register words. */
export interface RangeTimingRegisters {
  rangeConfigA: number;
  rangeConfigB: number;
  /** INTERMEASUREMENT_MS raw dword. */
  intermeasurementRaw: number;
}

/**
 * SetRangeTiming register math → RANGE_CONFIG_A, RANGE_CONFIG_B and the
 * INTERMEASUREMENT_MS raw dword.
 *
 * `oscFrequency` is the word read from 0x0006; `clockPll` is the word read
 * from RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
 * `interMeasurementMs > 0`).
 */
export function rangeTimingRegisters(
  timingBudgetMs: number,
  interMeasurementMs: number,
  oscFrequency: number,
  clockPll = 0,
): RangeTimingRegisters {
  if (oscFrequency === 0) throw new Vl53l4cdError("oscFrequency reads 0");
  if (!(timingBudgetMs >= 10 && timingBudgetMs <= 200)) {
    throw new Vl53l4cdError("timingBudgetMs must be 10..200");
  }

  let timingBudgetUs = timingBudgetMs * 1000;
  const macroPeriodUs = u32(2304 * Math.floor(0x40000000 / oscFrequency)) >>> 6;

  let intermeasurementRaw: number;
  if (interMeasurementMs === 0) {
    // continuous
    intermeasurementRaw = 0;
    timingBudgetUs -= 2500;
  } else if (interMeasurementMs > timingBudgetMs) {
    // autonomous low power (1.055 is a frozen double-precision PLL factor)
    intermeasurementRaw = Math.floor(1.055 * interMeasurementMs * (clockPll & 0x3ff));
    timingBudgetUs = Math.floor((timingBudgetUs - 4300) / 2);
  } else {
    throw new Vl53l4cdError("interMeasurementMs must be 0 or > timingBudgetMs");
  }

  timingBudgetUs = u32(timingBudgetUs * 4096); // (x << 12) & 0xFFFFFFFF
  const words: number[] = [];
  for (const mult of [16, 12]) {
    // RANGE_CONFIG_A, RANGE_CONFIG_B
    const tmp = u32(macroPeriodUs * mult) >>> 6;
    let lsByte = Math.floor((timingBudgetUs + (tmp >>> 1)) / tmp) - 1;
    let msByte = 0;
    while (lsByte > 0xff) {
      // ls_byte & 0xFFFFFF00 in the reference (lsByte is non-negative here)
      lsByte = Math.floor(lsByte / 2);
      msByte += 1;
    }
    words.push(((msByte << 8) + (lsByte & 0xff)) & 0xffff);
  }
  return { rangeConfigA: words[0]!, rangeConfigB: words[1]!, intermeasurementRaw };
}

/** GetRangeTiming decode result. */
export interface RangeTiming {
  timingBudgetMs: number;
  interMeasurementMs: number;
}

/**
 * GetRangeTiming register math → timing budget and inter-measurement period.
 *
 * Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
 * RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
 * word.
 */
export function decodeRangeTiming(
  intermeasurementRaw: number,
  clockPll: number,
  oscFrequency: number,
  rangeConfigA: number,
): RangeTiming {
  if (oscFrequency === 0) throw new Vl53l4cdError("oscFrequency reads 0");

  // 1.065 is a frozen double-precision PLL factor.
  const pll = Math.floor(1.065 * (clockPll & 0x3ff)) & 0xffff;
  const interMeasurementMs = pll !== 0 ? Math.floor(intermeasurementRaw / pll) & 0xffff : 0;

  let macroPeriodUs = u32(2304 * Math.floor(0x40000000 / oscFrequency)) >>> 6;
  const lsByte = (rangeConfigA & 0x00ff) << 4;
  let msByte = (rangeConfigA & 0xff00) >> 8;
  msByte = u32(0x04 - (msByte - 1) - 1);
  macroPeriodUs = u32(macroPeriodUs * 16);

  const sub = macroPeriodUs >>> 6;
  let budget = u32((lsByte + 1) * sub - (sub >>> 1)) >>> 12;
  if (msByte < 12) budget >>= msByte;
  budget = intermeasurementRaw === 0 ? budget + 2500 : budget * 2 + 4300;
  return { timingBudgetMs: Math.floor(budget / 1000), interMeasurementMs };
}

// ── Threshold / offset / xtalk raw codecs (register word ↔ user units) ───────

/** RANGE_OFFSET_MM word for setOffset (INNER/OUTER are zeroed alongside). */
export function offsetRaw(offsetMm: number): number {
  return (offsetMm * 4) & 0xffff;
}

/** getOffset: RANGE_OFFSET_MM word → signed millimetres. */
export function decodeOffset(rawWord: number): number {
  const temp = ((rawWord << 3) & 0xffff) >> 5;
  return temp > 1024 ? temp - 2048 : temp;
}

/** XTALK_PLANE_OFFSET_KCPS word for setXtalk. */
export function xtalkRaw(xtalkKcps: number): number {
  return (xtalkKcps << 9) & 0xffff;
}

/** getXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps. */
export function decodeXtalk(rawWord: number): number {
  return roundHalfEven(rawWord / 512.0);
}

export function signalThresholdRaw(signalKcps: number): number {
  return signalKcps >> 3;
}

export function decodeSignalThreshold(rawWord: number): number {
  return (rawWord << 3) & 0xffff;
}

export function sigmaThresholdRaw(sigmaMm: number): number {
  if (sigmaMm > 0xffff >> 2) {
    throw new Vl53l4cdError("sigmaMm must be <= 16383");
  }
  return sigmaMm << 2;
}

export function decodeSigmaThreshold(rawWord: number): number {
  return rawWord >> 2;
}

/**
 * Python's banker's rounding (`round()`): exact .5 goes to the nearest even
 * integer. Keeps the codecs bit-identical to the reference.
 */
function roundHalfEven(x: number): number {
  const r = Math.round(x);
  return Math.abs(x % 1) === 0.5 && r % 2 !== 0 ? r - 1 : r;
}

/** Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3). */
export class VL53L4CD {
  constructor(readonly p: Vl53l4Platform) {}

  // ── register access helpers (16-bit addr, big-endian contents) ─────────────

  async rdByte(addr: number): Promise<number> {
    return (await this.p.rdMulti(addr, 1))[0]!;
  }

  async rdWord(addr: number): Promise<number> {
    const b = await this.p.rdMulti(addr, 2);
    return ((b[0]! << 8) | b[1]!) & 0xffff;
  }

  async rdDword(addr: number): Promise<number> {
    const b = await this.p.rdMulti(addr, 4);
    return ((b[0]! << 24) | (b[1]! << 16) | (b[2]! << 8) | b[3]!) >>> 0;
  }

  async wrByte(addr: number, value: number): Promise<void> {
    await this.p.wrMulti(addr, Uint8Array.of(value & 0xff));
  }

  async wrWord(addr: number, value: number): Promise<void> {
    await this.p.wrMulti(addr, Uint8Array.of((value >> 8) & 0xff, value & 0xff));
  }

  async wrDword(addr: number, value: number): Promise<void> {
    await this.p.wrMulti(
      addr,
      Uint8Array.of((value >>> 24) & 0xff, (value >>> 16) & 0xff, (value >>> 8) & 0xff, value & 0xff),
    );
  }

  // ── identity ───────────────────────────────────────────────────────────────

  getSensorId(): Promise<number> {
    return this.rdWord(IDENTIFICATION__MODEL_ID);
  }

  async isAlive(): Promise<boolean> {
    return (await this.getSensorId()) === MODEL_ID_VL53L4CD;
  }

  // ── init ───────────────────────────────────────────────────────────────────

  async waitBoot(timeoutMs = 1000): Promise<void> {
    for (let i = 0; i < Math.max(1, timeoutMs); i++) {
      if ((await this.rdByte(FIRMWARE__SYSTEM_STATUS)) === 0x03) return;
      await this.p.sleepMs(1);
    }
    throw new Vl53l4cdError("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
  }

  /**
   * Initialise the sensor and leave the bus at `busKhz`.
   *
   * The configuration block is written at I2C_KHZ_BOOT (400 kHz) because that
   * is the only speed an unconfigured sensor is specified for; the bridge is
   * re-timed to `busKhz` after it. A sensor reset just re-runs this whole
   * sequence.
   */
  async sensorInit(busKhz: number = I2C_KHZ_DEFAULT): Promise<void> {
    await this.p.setI2cSpeed(I2C_KHZ_BOOT);
    await this.waitBoot();

    // The C driver writes the 91 bytes one register at a time; the sensor
    // auto-increments, so one transaction does the same job.
    await this.p.wrMulti(CONFIG_ADDR, configBlock());
    if (busKhz !== I2C_KHZ_BOOT) {
      await this.p.setI2cSpeed(busKhz);
    }

    await this.wrByte(SYSTEM_START, 0x40); // start VHV
    await this.waitDataReady();
    await this.clearInterrupt();
    await this.stopRanging();
    await this.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09);
    await this.wrByte(0x000b, 0x00);
    await this.wrWord(0x0024, 0x0500);

    await this.setRangeTiming(50, 0);
  }

  // ── ranging ────────────────────────────────────────────────────────────────

  async clearInterrupt(): Promise<void> {
    await this.wrByte(SYSTEM__INTERRUPT_CLEAR, 0x01);
  }

  async startRanging(): Promise<void> {
    // 0 = continuous, anything else = autonomous low power.
    const mode = (await this.rdDword(INTERMEASUREMENT_MS)) === 0 ? 0x21 : 0x40;
    await this.wrByte(SYSTEM_START, mode);
  }

  async stopRanging(): Promise<void> {
    await this.wrByte(SYSTEM_START, 0x80);
  }

  async checkForDataReady(): Promise<boolean> {
    const intPol = (((await this.rdByte(GPIO_HV_MUX__CTRL)) & 0x10) >> 4) === 1 ? 0 : 1;
    return ((await this.rdByte(GPIO__TIO_HV_STATUS)) & 1) === intPol;
  }

  async waitDataReady(timeoutMs = 1000): Promise<void> {
    for (let i = 0; i < Math.max(1, timeoutMs); i++) {
      if (await this.checkForDataReady()) return;
      await this.p.sleepMs(1);
    }
    throw new Vl53l4cdError("timeout waiting for data ready");
  }

  /**
   * One block read instead of the C driver's six register reads — the sensor
   * auto-increments and the decoding is identical.
   */
  async getResult(): Promise<Vl53l4Results> {
    return parseResultBlock(await this.p.rdMulti(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN));
  }

  // ── timing ─────────────────────────────────────────────────────────────────

  async setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void> {
    const clockPll = interMeasurementMs > 0 ? await this.rdWord(RESULT__OSC_CALIBRATE_VAL) : 0;
    const { rangeConfigA, rangeConfigB, intermeasurementRaw } = rangeTimingRegisters(
      timingBudgetMs,
      interMeasurementMs,
      await this.rdWord(OSC_FREQUENCY),
      clockPll,
    );
    await this.wrDword(INTERMEASUREMENT_MS, intermeasurementRaw);
    await this.wrWord(RANGE_CONFIG_A, rangeConfigA);
    await this.wrWord(RANGE_CONFIG_B, rangeConfigB);
  }

  async getRangeTiming(): Promise<RangeTiming> {
    return decodeRangeTiming(
      await this.rdDword(INTERMEASUREMENT_MS),
      await this.rdWord(RESULT__OSC_CALIBRATE_VAL),
      await this.rdWord(OSC_FREQUENCY),
      await this.rdWord(RANGE_CONFIG_A),
    );
  }

  // ── offset ─────────────────────────────────────────────────────────────────

  async setOffset(offsetMm: number): Promise<void> {
    await this.wrWord(RANGE_OFFSET_MM, offsetRaw(offsetMm));
    await this.wrWord(INNER_OFFSET_MM, 0);
    await this.wrWord(OUTER_OFFSET_MM, 0);
  }

  async getOffset(): Promise<number> {
    return decodeOffset(await this.rdWord(RANGE_OFFSET_MM));
  }

  // ── crosstalk ──────────────────────────────────────────────────────────────

  async setXtalk(xtalkKcps: number): Promise<void> {
    await this.wrWord(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000);
    await this.wrWord(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000);
    await this.wrWord(XTALK_PLANE_OFFSET_KCPS, xtalkRaw(xtalkKcps));
  }

  async getXtalk(): Promise<number> {
    return decodeXtalk(await this.rdWord(XTALK_PLANE_OFFSET_KCPS));
  }

  // ── thresholds ─────────────────────────────────────────────────────────────

  async setDetectionThresholds(
    distanceLowMm: number,
    distanceHighMm: number,
    window: number,
  ): Promise<void> {
    await this.wrByte(SYSTEM__INTERRUPT, window);
    await this.wrWord(THRESH_HIGH, distanceHighMm);
    await this.wrWord(THRESH_LOW, distanceLowMm);
  }

  async getDetectionThresholds(): Promise<{
    distanceLowMm: number;
    distanceHighMm: number;
    window: number;
  }> {
    const distanceHighMm = await this.rdWord(THRESH_HIGH);
    const distanceLowMm = await this.rdWord(THRESH_LOW);
    const window = (await this.rdByte(SYSTEM__INTERRUPT)) & 0x07;
    return { distanceLowMm, distanceHighMm, window };
  }

  async setSignalThreshold(signalKcps: number): Promise<void> {
    await this.wrWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS, signalThresholdRaw(signalKcps));
  }

  async getSignalThreshold(): Promise<number> {
    return decodeSignalThreshold(await this.rdWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS));
  }

  async setSigmaThreshold(sigmaMm: number): Promise<void> {
    await this.wrWord(RANGE_CONFIG__SIGMA_THRESH, sigmaThresholdRaw(sigmaMm));
  }

  async getSigmaThreshold(): Promise<number> {
    return decodeSigmaThreshold(await this.rdWord(RANGE_CONFIG__SIGMA_THRESH));
  }

  // ── temperature ────────────────────────────────────────────────────────────

  /** Recommended after a >8 °C ambient change (ST Example_3). */
  async startTemperatureUpdate(): Promise<void> {
    await this.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81);
    await this.wrByte(0x000b, 0x92);
    await this.startRanging();
    await this.waitDataReady();
    await this.clearInterrupt();
    await this.stopRanging();
    await this.wrByte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09);
    await this.wrByte(0x000b, 0x00);
  }

  // ── calibration (VL53L4CD_calibration.c) ───────────────────────────────────

  /**
   * The ranging loop both calibrations share: data-ready, getResult,
   * clearInterrupt, `nbSamples` times.
   */
  private async collect(
    nbSamples: number,
    onSample: (i: number, r: Vl53l4Results) => void,
    timeoutMs = 5000,
  ): Promise<void> {
    await this.startRanging();
    for (let i = 0; i < nbSamples; i++) {
      await this.waitDataReady(timeoutMs);
      const result = await this.getResult();
      await this.clearInterrupt();
      onSample(i, result);
    }
    await this.stopRanging();
  }

  async calibrateOffset(targetDistMm: number, nbSamples = 20): Promise<number> {
    if (!(nbSamples >= 5 && nbSamples <= 255) || !(targetDistMm >= 10 && targetDistMm <= 1000)) {
      throw new Vl53l4cdError("nbSamples must be 5..255, target 10..1000 mm");
    }

    await this.wrWord(RANGE_OFFSET_MM, 0);
    await this.wrWord(INNER_OFFSET_MM, 0);
    await this.wrWord(OUTER_OFFSET_MM, 0);

    await this.collect(10, () => undefined); // device heat loop

    const distances: number[] = [];
    await this.collect(nbSamples, (_i, r) => distances.push(r.distanceMm));

    const sum = distances.reduce((a, b) => a + b, 0);
    const offsetMm = targetDistMm - Math.floor(sum / nbSamples);
    await this.wrWord(RANGE_OFFSET_MM, offsetRaw(offsetMm));
    return offsetMm;
  }

  async calibrateXtalk(targetDistMm: number, nbSamples = 20): Promise<number> {
    if (!(nbSamples >= 5 && nbSamples <= 255) || !(targetDistMm >= 10 && targetDistMm <= 5000)) {
      throw new Vl53l4cdError("nbSamples must be 5..255, target 10..5000 mm");
    }

    await this.wrWord(XTALK_PLANE_OFFSET_KCPS, 0); // disable compensation

    await this.collect(10, () => undefined); // device heat loop

    const samples: Vl53l4Results[] = [];
    await this.collect(nbSamples, (i, r) => {
      // Discard invalid measurements and the first frame.
      if (r.rangeStatus === 0 && i > 0) samples.push(r);
    });

    if (samples.length === 0) {
      throw new Vl53l4cdError("xtalk calibration failed: no valid samples");
    }

    const n = samples.length;
    const avgDistance = samples.reduce((a, s) => a + s.distanceMm, 0) / n;
    const avgSpadNb = samples.reduce((a, s) => a + s.numberOfSpad, 0) / n;
    const avgSignal = samples.reduce((a, s) => a + s.signalRateKcps, 0) / n;

    const tmpXtalk = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
    if (tmpXtalk > 127) {
      // 127 kcps is the max xtalk value (65536/512)
      throw new Vl53l4cdError(`xtalk calibration failed: ${tmpXtalk.toFixed(1)} kcps > 127`);
    }

    await this.wrWord(XTALK_PLANE_OFFSET_KCPS, Math.trunc(tmpXtalk * 512.0) & 0xffff);
    return roundHalfEven(tmpXtalk);
  }
}
