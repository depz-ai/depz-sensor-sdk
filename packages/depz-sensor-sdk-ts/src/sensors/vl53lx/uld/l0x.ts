/**
 * uld/l0x — VL53L0X API 1.0.4: the ranging path. Async port of the Python
 * `depz_sensor_sdk.vl53lx.uld.l0x` (vl53l0x_api.c / vl53l0x_api_core.c /
 * vl53l0x_api_calibration.c): DataInit, StaticInit (reference SPADs from NVM
 * + tuning settings), the timing-budget arithmetic, Start/StopMeasurement,
 * GetRangingMeasurementData with its PAL range status (sigma estimate
 * included), PerformRefCalibration, PerformRefSpadManagement, the offset and
 * crosstalk calibrations and the ranging profiles of the four ST examples.
 *
 * Register sequences, integer widths and the deliberate overflows are kept as
 * in C — do not "simplify" them. Arithmetic that can leave int32 goes through
 * `arith.ts`; the sigma estimate (intermediates past 2^53) runs on BigInt.
 *
 * There is no ROI on this die.
 */

import { I2C_KHZ_BOOT, ProtocolError, Vl53Error } from "./link.js";
import { measurement, SensorDriver, type BridgePlatform, type Measurement } from "./base.js";
import { floorDiv, pyRound, shl, shr, u32 } from "./arith.js";

export const L0X_API_VERSION = [1, 0, 4] as const;

// ── registers (vl53l0x_device.h) ─────────────────────────────────────────────
export const SYSRANGE_START = 0x00;
export const SYSRANGE_MODE_SINGLESHOT = 0x00;
export const SYSRANGE_MODE_START_STOP = 0x01;
export const SYSRANGE_MODE_BACKTOBACK = 0x02;
export const SYSRANGE_MODE_TIMED = 0x04;

export const SYSTEM_SEQUENCE_CONFIG = 0x01;
export const SYSTEM_INTERMEASUREMENT_PERIOD = 0x04;
export const SYSTEM_RANGE_CONFIG = 0x09;
export const SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0a;
export const SYSTEM_INTERRUPT_CLEAR = 0x0b;
export const SYSTEM_THRESH_HIGH = 0x0c;
export const SYSTEM_THRESH_LOW = 0x0e;
export const RESULT_INTERRUPT_STATUS = 0x13;
export const RESULT_RANGE_STATUS = 0x14;
export const CROSSTALK_COMPENSATION_PEAK_RATE_MCPS = 0x20;
export const ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x28;
export const ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30;
export const GLOBAL_CONFIG_VCSEL_WIDTH = 0x32;
export const FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44;
export const MSRC_CONFIG_TIMEOUT_MACROP = 0x46;
export const FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47;
export const FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48;
export const DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4e;
export const DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4f;
export const PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50;
export const PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51;
export const PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56;
export const PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57;
export const MSRC_CONFIG_CONTROL = 0x60;
export const PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64;
export const FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70;
export const FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71;
export const POWER_MANAGEMENT_GO1_POWER_FORCE = 0x80;
export const GPIO_HV_MUX_ACTIVE_HIGH = 0x84;
export const SOFT_RESET_GO2_SOFT_RESET_N = 0xbf;
export const GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xb0;
export const GLOBAL_CONFIG_REF_EN_START_SELECT = 0xb6;
export const IDENTIFICATION_MODEL_ID = 0xc0;
export const OSC_CALIBRATE_VAL = 0xf8;

// Page 1 (0xFF ← 0x01 first).
export const ALGO_PHASECAL_LIM = 0x30;
export const RESULT_PEAK_SIGNAL_RATE_REF = 0xb6;

/** One byte at 0xC0, unlike the 16-bit family. */
export const MODEL_ID_VL53L0X = 0xee;

// Device modes (VL53L0X_DeviceModes).
export const DEVICEMODE_SINGLE_RANGING = 0;
export const DEVICEMODE_CONTINUOUS_RANGING = 1;
export const DEVICEMODE_CONTINUOUS_TIMED = 3;

export const GPIOFUNCTIONALITY_NEW_MEASURE_READY = 4;

// Sequence steps (VL53L0X_SequenceStepId).
export const SEQUENCESTEP_TCC = 0;
export const SEQUENCESTEP_DSS = 1;
export const SEQUENCESTEP_MSRC = 2;
export const SEQUENCESTEP_PRE_RANGE = 3;
export const SEQUENCESTEP_FINAL_RANGE = 4;
const SEQ_SET = [0x10, 0x28, 0x04, 0x40, 0x80];
const SEQ_CLEAR = [0xef, 0xd7, 0xfb, 0xbf, 0x7f];
const SEQ_TEST = [0x10, 0x08, 0x04, 0x40, 0x80];

export const VCSEL_PERIOD_PRE_RANGE = 0;
export const VCSEL_PERIOD_FINAL_RANGE = 1;

// Limit checks (VL53L0X_CHECKENABLE_*).
export const CHECK_SIGMA_FINAL_RANGE = 0;
export const CHECK_SIGNAL_RATE_FINAL_RANGE = 1;
export const CHECK_SIGNAL_REF_CLIP = 2;
export const CHECK_RANGE_IGNORE_THRESHOLD = 3;
export const CHECK_SIGNAL_RATE_MSRC = 4;
export const CHECK_SIGNAL_RATE_PRE_RANGE = 5;
export const CHECK_NUMBER_OF_CHECKS = 6;

export const DEFAULT_MAX_LOOP = 2000;
/** 20 Mcps in 9.7 format, the ref-SPAD target. */
export const TARGET_REF_RATE = 0x0a00;
export const SPEED_OF_LIGHT_IN_AIR = 2997;
export const REF_SPAD_BUFFER_SIZE = 6;

/**
 * The ranging modes of the four ST examples: (signal Mcps, sigma mm, budget
 * us, pre-range PCLK, final PCLK). 'default' is what StaticInit leaves.
 */
export const L0X_MODE_DEFAULT = "default";
export const L0X_MODE_SETTINGS: ReadonlyMap<
  string,
  readonly [number, number, number, number, number]
> = new Map([
  ["default", [0.25, 18, 33000, 14, 10]],
  ["long-range", [0.1, 60, 33000, 18, 14]],
  ["high-speed", [0.25, 32, 30000, 14, 10]],
  ["high-accuracy", [0.25, 18, 200000, 14, 10]],
]);

/**
 * DefaultTuningSettings[] (vl53l0x_tuning.h, "update 02/11/2015_v36"): a run
 * of {count, address, count bytes...} records ended by a zero count.
 */
export const DEFAULT_TUNING_SETTINGS = Uint8Array.from([
  0x01, 0xff, 0x01, 0x01, 0x00, 0x00,

  0x01, 0xff, 0x00, 0x01, 0x09, 0x00, 0x01, 0x10, 0x00, 0x01, 0x11, 0x00,

  0x01, 0x24, 0x01, 0x01, 0x25, 0xff, 0x01, 0x75, 0x00,

  0x01, 0xff, 0x01, 0x01, 0x4e, 0x2c, 0x01, 0x48, 0x00, 0x01, 0x30, 0x20,

  0x01, 0xff, 0x00, 0x01, 0x30, 0x09, 0x01, 0x54, 0x00, 0x01, 0x31, 0x04, 0x01, 0x32, 0x03,
  0x01, 0x40, 0x83, 0x01, 0x46, 0x25, 0x01, 0x60, 0x00, 0x01, 0x27, 0x00, 0x01, 0x50, 0x06,
  0x01, 0x51, 0x00, 0x01, 0x52, 0x96, 0x01, 0x56, 0x08, 0x01, 0x57, 0x30, 0x01, 0x61, 0x00,
  0x01, 0x62, 0x00, 0x01, 0x64, 0x00, 0x01, 0x65, 0x00, 0x01, 0x66, 0xa0,

  0x01, 0xff, 0x01, 0x01, 0x22, 0x32, 0x01, 0x47, 0x14, 0x01, 0x49, 0xff, 0x01, 0x4a, 0x00,

  0x01, 0xff, 0x00, 0x01, 0x7a, 0x0a, 0x01, 0x7b, 0x00, 0x01, 0x78, 0x21,

  0x01, 0xff, 0x01, 0x01, 0x23, 0x34, 0x01, 0x42, 0x00, 0x01, 0x44, 0xff, 0x01, 0x45, 0x26,
  0x01, 0x46, 0x05, 0x01, 0x40, 0x40, 0x01, 0x0e, 0x06, 0x01, 0x20, 0x1a, 0x01, 0x43, 0x40,

  0x01, 0xff, 0x00, 0x01, 0x34, 0x03, 0x01, 0x35, 0x44,

  0x01, 0xff, 0x01, 0x01, 0x31, 0x04, 0x01, 0x4b, 0x09, 0x01, 0x4c, 0x05, 0x01, 0x4d, 0x04,

  0x01, 0xff, 0x00, 0x01, 0x44, 0x00, 0x01, 0x45, 0x20, 0x01, 0x47, 0x08, 0x01, 0x48, 0x28,
  0x01, 0x67, 0x00, 0x01, 0x70, 0x04, 0x01, 0x71, 0x01, 0x01, 0x72, 0xfe, 0x01, 0x76, 0x00,
  0x01, 0x77, 0x00,

  0x01, 0xff, 0x01, 0x01, 0x0d, 0x01,

  0x01, 0xff, 0x00, 0x01, 0x80, 0x01, 0x01, 0x01, 0xf8,

  0x01, 0xff, 0x01, 0x01, 0x8e, 0x01, 0x01, 0x00, 0x01, 0x01, 0xff, 0x00, 0x01, 0x80, 0x00,

  0x00, 0x00, 0x00,
]);

/** Dmax lookup table set up by DataInit, FixPoint16.16. */
export const DMAX_LUT_AMB_RATE_MCPS: readonly number[] = [
  0x00000000, 0x0000b333, 0x00020000, 0x0003cccc, 0x00074ccc, 0x000a0000, 0x000f0000,
];
export const DMAX_LUT_DMAX_MM: readonly number[] = [
  0x04b00000, 0x044c0000, 0x03840000, 0x02ee0000, 0x02260000, 0x01f40000, 0x01900000,
];

/** The block the MCU streams: RESULT_RANGE_STATUS .. 0x1F. */
export const L0X_RESULT_BLOCK_ADDR = RESULT_RANGE_STATUS;
export const L0X_RESULT_BLOCK_LEN = 12;

/** Two writes release this sensor's interrupt. */
export const L0X_CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = [
  [SYSTEM_INTERRUPT_CLEAR, 0x01],
  [SYSTEM_INTERRUPT_CLEAR, 0x00],
];

/** The only part of the family with 8-bit register addresses. */
export const L0X_ADDR_WIDTH = 1;

/** VL53L0X_GetRangeStatusString(). */
export const L0X_RANGE_STATUS_NAMES: Readonly<Record<number, string>> = {
  0: "Range Valid",
  1: "Sigma Fail",
  2: "Signal Fail",
  3: "Min Range Fail",
  4: "Phase Fail",
  5: "Hardware Fail",
  255: "No Update",
};

// Reference SPAD array geometry.
const REF_ARRAY_SPAD_0 = 0;
const REF_ARRAY_SPAD_5 = 5;
const REF_ARRAY_SPAD_10 = 10;
const REF_ARRAY_QUADRANTS = [REF_ARRAY_SPAD_10, REF_ARRAY_SPAD_5, REF_ARRAY_SPAD_0, REF_ARRAY_SPAD_5];

/** VL53L0X_isqrt() on BigInt — the bit-by-bit integer square root. */
export function isqrt(numIn: bigint): bigint {
  let num = numIn;
  let res = 0n;
  let bit = 1n << 30n;
  while (bit > num) bit >>= 2n;
  while (bit !== 0n) {
    if (num >= res + bit) {
      num -= res + bit;
      res = (res >> 1n) + bit;
    } else {
      res >>= 1n;
    }
    bit >>= 2n;
  }
  return res;
}

export function decodeVcselPeriod(reg: number): number {
  return (reg + 1) << 1;
}

export function encodeVcselPeriod(pclks: number): number {
  return (pclks >> 1) - 1;
}

/** (LSByte * 2^MSByte) + 1 format. */
export function encodeTimeout(timeoutMacroClks: number): number {
  if (timeoutMacroClks <= 0) return 0;
  let lsByte = timeoutMacroClks - 1;
  let msByte = 0;
  while (lsByte >= 0x100) {
    lsByte = floorDiv(lsByte, 2);
    msByte += 1;
  }
  return (msByte * 256 + (lsByte & 0xff)) & 0xffff;
}

export function decodeTimeout(encoded: number): number {
  return shl(encoded & 0x00ff, (encoded & 0xff00) >> 8) + 1;
}

/** 2304 vclks per macro period, PLL period fixed at 1655 ps. */
export function calcMacroPeriodPs(vcselPeriodPclks: number): number {
  return 2304 * vcselPeriodPclks * 1655;
}

export function calcTimeoutMclks(timeoutPeriodUs: number, vcselPeriodPclks: number): number {
  const macroPeriodNs = floorDiv(calcMacroPeriodPs(vcselPeriodPclks) + 500, 1000);
  return floorDiv(timeoutPeriodUs * 1000 + floorDiv(macroPeriodNs, 2), macroPeriodNs);
}

export function calcTimeoutUs(timeoutPeriodMclks: number, vcselPeriodPclks: number): number {
  const macroPeriodNs = floorDiv(calcMacroPeriodPs(vcselPeriodPclks) + 500, 1000);
  return floorDiv(timeoutPeriodMclks * macroPeriodNs + 500, 1000);
}

/** Which quadrant a SPAD index falls in decides whether it is an aperture SPAD. */
export function isAperture(spadIndex: number): boolean {
  return REF_ARRAY_QUADRANTS[spadIndex >> 6] !== REF_ARRAY_SPAD_0;
}

/** → index of the next enabled bit at or after `curr`, or -1. */
export function getNextGoodSpad(goodSpadArray: ArrayLike<number>, size: number, curr: number): number {
  const startIndex = floorDiv(curr, 8);
  const fineOffset = curr % 8;
  for (let coarse = startIndex; coarse < size; coarse++) {
    let dataByte = goodSpadArray[coarse]!;
    let fine = 0;
    if (coarse === startIndex) {
      dataByte >>= fineOffset;
      fine = fineOffset;
    }
    while (fine < 8) {
      if (dataByte & 1) return coarse * 8 + fine;
      dataByte >>= 1;
      fine += 1;
    }
  }
  return -1;
}

/** VL53L0X_RangingMeasurementData_t, the fields this port fills. */
export interface RangingMeasurementData {
  rangeStatus: number;
  distanceMm: number;
  rangeFractionalPart: number;
  /** FixPoint16.16 Mcps. */
  signalRateMcps: number;
  ambientRateMcps: number;
  /** 8.8 format. */
  effectiveSpadRtnCount: number;
  dmaxMm: number;
  sigmaMm: number;
  deviceRangeStatus: number;
}

export function l0xStatusText(status: number): string {
  return L0X_RANGE_STATUS_NAMES[status] ?? `unknown (${status})`;
}

/** PALDevData / DeviceSpecificParameters — the state the C driver keeps. */
export interface L0xDeviceData {
  ReadDataFromDeviceDone: number;
  LinearityCorrectiveGain: number;
  OscFrequencyMHz: number;
  XTalkCompensationRateMegaCps: number;
  XTalkCompensationEnable: number;
  StopVariable: number;
  SequenceConfig: number;
  RangeFractionalEnable: number;
  DeviceMode: number;
  MeasurementTimingBudgetMicroSeconds: number;
  Pin0GpioFunctionality: number;
  ReferenceSpadCount: number;
  ReferenceSpadType: number;
  RefGoodSpadMap: number[];
  RefSpadEnables: number[];
  PreRangeVcselPulsePeriod: number;
  FinalRangeVcselPulsePeriod: number;
  PreRangeTimeoutMicroSecs: number;
  FinalRangeTimeoutMicroSecs: number;
  TargetRefRate: number;
  LimitChecksEnable: number[];
  LimitChecksValue: number[];
  Part2PartOffsetAdjustmentNVMMicroMeter: number;
  SignalRateMeasFixed400mm: number;
  ModuleId: number;
  Revision: number;
  ProductId: string;
  PartUIDUpper: number;
  PartUIDLower: number;
}

// Fixed scheduler overheads, us (set_measurement_timing_budget_micro_seconds()).
const START_OVERHEAD_US = 1910;
const END_OVERHEAD_US = 960;
const MSRC_OVERHEAD_US = 660;
const TCC_OVERHEAD_US = 590;
const DSS_OVERHEAD_US = 690;
const PRE_RANGE_OVERHEAD_US = 660;
const FINAL_RANGE_OVERHEAD_US = 550;

function arraysEqual(a: ArrayLike<number>, b: ArrayLike<number>): boolean {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
  return true;
}

/** Port of the VL53L0X API 1.0.4 ranging path. */
export class VL53L0X extends SensorDriver {
  static readonly ADDR_WIDTH: number = L0X_ADDR_WIDTH;
  static readonly CLEAR_STEPS: ReadonlyArray<readonly [number, number]> = L0X_CLEAR_STEPS;
  /** No Fast Mode Plus on this die. */
  static readonly MAX_KHZ: number = 400;
  static readonly SUPPORTS: ReadonlySet<string> = new Set([
    "mode",
    "timing",
    "offset",
    "xtalk",
    "calib_offset",
    "calib_xtalk",
    "refspad",
  ]);
  static readonly MODES: readonly string[] = [...L0X_MODE_SETTINGS.keys()];
  /** Below ~20 ms the enabled sequence steps no longer fit the budget. */
  static readonly BUDGET_MS: readonly [number, number] = [20, 200];

  /** PALDevData — every field is written before it is read. */
  readonly d: L0xDeviceData = {
    ReadDataFromDeviceDone: 0,
    LinearityCorrectiveGain: 1000,
    OscFrequencyMHz: 618660,
    XTalkCompensationRateMegaCps: 0,
    XTalkCompensationEnable: 0,
    StopVariable: 0,
    SequenceConfig: 0,
    RangeFractionalEnable: 0,
    DeviceMode: DEVICEMODE_SINGLE_RANGING,
    MeasurementTimingBudgetMicroSeconds: 0,
    Pin0GpioFunctionality: GPIOFUNCTIONALITY_NEW_MEASURE_READY,
    ReferenceSpadCount: 0,
    ReferenceSpadType: 0,
    RefGoodSpadMap: new Array<number>(REF_SPAD_BUFFER_SIZE).fill(0),
    RefSpadEnables: new Array<number>(REF_SPAD_BUFFER_SIZE).fill(0),
    PreRangeVcselPulsePeriod: 0,
    FinalRangeVcselPulsePeriod: 0,
    PreRangeTimeoutMicroSecs: 0,
    FinalRangeTimeoutMicroSecs: 0,
    TargetRefRate: TARGET_REF_RATE,
    LimitChecksEnable: new Array<number>(CHECK_NUMBER_OF_CHECKS).fill(0),
    LimitChecksValue: new Array<number>(CHECK_NUMBER_OF_CHECKS).fill(0),
    Part2PartOffsetAdjustmentNVMMicroMeter: 0,
    SignalRateMeasFixed400mm: 0,
    ModuleId: 0,
    Revision: 0,
    ProductId: "",
    PartUIDUpper: 0,
    PartUIDLower: 0,
  };
  /** False when the NVM ref-SPAD record is out of range (static_init). */
  refSpadsFromNvm = true;
  /** Which of MODES is applied (the budget reads back rounded). */
  private mode: string = L0X_MODE_DEFAULT;

  constructor(platform: BridgePlatform, part: string) {
    super(platform, part);
  }

  // ── SensorDriver contract ──
  streamBlock(): [number, number] {
    return [L0X_RESULT_BLOCK_ADDR, L0X_RESULT_BLOCK_LEN];
  }

  decode(raw: Uint8Array): Measurement {
    return l0xAsMeasurement(this.parseResultBlock(raw));
  }

  override async readMeasurement(): Promise<Measurement> {
    return l0xAsMeasurement(await this.getResult());
  }

  // ── identity ──
  modelId(): Promise<number> {
    return this.p.rdByte(IDENTIFICATION_MODEL_ID);
  }

  /** No firmware-status register: the model id answering says the die is up. */
  async waitBoot(timeoutS = 1.0): Promise<void> {
    const deadline = Date.now() + timeoutS * 1000;
    for (;;) {
      try {
        if ((await this.p.rdByte(IDENTIFICATION_MODEL_ID)) === MODEL_ID_VL53L0X) return;
      } catch (err) {
        // NACK while the die is still booting.
        if (!(err instanceof ProtocolError)) throw err;
      }
      if (Date.now() > deadline) throw new Vl53Error("timeout waiting for MODEL_ID 0xEE at 0xC0");
      await this.p.sleepMs(1);
    }
  }

  // ── init ──
  /** DataInit + StaticInit, and leave the sensor in continuous mode. */
  async sensorInit(): Promise<void> {
    await this.p.setAddrWidth(L0X_ADDR_WIDTH);
    await this.p.setI2cSpeed(I2C_KHZ_BOOT);
    await this.waitBoot();

    await this.resetDevice();
    await this.dataInit();
    await this.staticInit();
    if (!this.refSpadsFromNvm) await this.performRefSpadManagement();
    await this.performRefCalibration();
    this.setDeviceMode(DEVICEMODE_CONTINUOUS_RANGING);
    this.mode = L0X_MODE_DEFAULT;

    if (this.MAX_KHZ !== I2C_KHZ_BOOT) await this.p.setI2cSpeed(this.MAX_KHZ);
  }

  /** VL53L0X_ResetDevice(): pull the die through its soft reset. */
  async resetDevice(timeoutS = 1.0): Promise<void> {
    const deadline = Date.now() + timeoutS * 1000;

    const pollModelId = async (wantZero: boolean): Promise<void> => {
      // A rebooting die NACKs its own address for a moment (ERR_HARDWARE_FAULT
      // from the bridge) — "not booted yet", not a fault.
      for (;;) {
        let byte: number | null;
        try {
          byte = await this.p.rdByte(IDENTIFICATION_MODEL_ID);
        } catch (err) {
          if (!(err instanceof ProtocolError)) throw err;
          byte = null;
        }
        if (byte !== null && (byte === 0x00) === wantZero) return;
        if (Date.now() > deadline) throw new Vl53Error("timeout in the soft reset of the sensor");
      }
    };

    await this.p.wrByte(SOFT_RESET_GO2_SOFT_RESET_N, 0x00);
    await pollModelId(true);
    await this.p.sleepMs(1);
    await this.p.wrByte(SOFT_RESET_GO2_SOFT_RESET_N, 0x01);
    await pollModelId(false);
    await this.p.sleepMs(1);
  }

  /** VL53L0X_DataInit(). */
  async dataInit(): Promise<void> {
    const p = this.p;
    const d = this.d;
    await p.wrByte(0x88, 0x00); // I2C standard mode
    d.ReadDataFromDeviceDone = 0;
    d.LinearityCorrectiveGain = 1000;
    d.OscFrequencyMHz = 618660;
    d.XTalkCompensationRateMegaCps = 0;
    d.XTalkCompensationEnable = 0;
    d.DeviceMode = DEVICEMODE_SINGLE_RANGING;

    await p.wrByte(0x80, 0x01);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x00);
    d.StopVariable = await p.rdByte(0x91);
    await p.wrByte(0x00, 0x01);
    await p.wrByte(0xff, 0x00);
    await p.wrByte(0x80, 0x00);

    for (let check = 0; check < CHECK_NUMBER_OF_CHECKS; check++) {
      await this.setLimitCheckEnable(check, 1);
    }
    await this.setLimitCheckEnable(CHECK_SIGNAL_REF_CLIP, 0);
    await this.setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);
    await this.setLimitCheckEnable(CHECK_SIGNAL_RATE_MSRC, 0);
    await this.setLimitCheckEnable(CHECK_SIGNAL_RATE_PRE_RANGE, 0);

    await this.setLimitCheckValue(CHECK_SIGMA_FINAL_RANGE, 18 * 65536);
    await this.setLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE, floorDiv(25 * 65536, 100));
    await this.setLimitCheckValue(CHECK_SIGNAL_REF_CLIP, 35 * 65536);
    await this.setLimitCheckValue(CHECK_RANGE_IGNORE_THRESHOLD, 0);

    d.SequenceConfig = 0xff;
    await p.wrByte(SYSTEM_SEQUENCE_CONFIG, 0xff);
  }

  /** VL53L0X_StaticInit(): reference SPADs, tuning settings, GPIO. */
  async staticInit(): Promise<void> {
    const p = this.p;
    const d = this.d;
    await this.getInfoFromDevice(1);

    const count = d.ReferenceSpadCount;
    const apertureSpads = d.ReferenceSpadType;
    this.refSpadsFromNvm = !(
      apertureSpads > 1 ||
      (apertureSpads === 1 && count > 32) ||
      (apertureSpads === 0 && count > 12)
    );
    if (this.refSpadsFromNvm) await this.setReferenceSpads(count, apertureSpads);

    await this.loadTuningSettings(DEFAULT_TUNING_SETTINGS);

    await this.setGpioConfig(GPIOFUNCTIONALITY_NEW_MEASURE_READY, true);

    await p.wrByte(0xff, 0x01);
    const tempword = await p.rdWord(0x84);
    await p.wrByte(0xff, 0x00);
    d.OscFrequencyMHz = shl(tempword, 4); // FixPoint4.12 → 16.16

    d.MeasurementTimingBudgetMicroSeconds = await this.getMeasurementTimingBudget();

    d.RangeFractionalEnable = (await p.rdByte(SYSTEM_RANGE_CONFIG)) & 1;
    d.SequenceConfig = await p.rdByte(SYSTEM_SEQUENCE_CONFIG);

    // MSRC and TCC off by default, as in the C driver.
    await this.setSequenceStepEnable(SEQUENCESTEP_TCC, 0);
    await this.setSequenceStepEnable(SEQUENCESTEP_MSRC, 0);

    d.PreRangeVcselPulsePeriod = await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE);
    d.FinalRangeVcselPulsePeriod = await this.getVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE);
    d.PreRangeTimeoutMicroSecs = await this.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE);
    d.FinalRangeTimeoutMicroSecs = await this.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE);
  }

  // ── reference calibration (VHV / phase) ──
  async performSingleRefCalibration(vhvInitByte: number): Promise<void> {
    await this.p.wrByte(SYSRANGE_START, SYSRANGE_MODE_START_STOP | vhvInitByte);
    await this.waitDataReady(); // measurement_poll_for_completion()
    await this.clearInterrupt();
    await this.p.wrByte(SYSRANGE_START, 0x00);
  }

  /** VL53L0X_ref_calibration_io() in its read direction → [vhv, phase]. */
  async refCalibrationIoRead(vhvEnable: boolean, phaseEnable: boolean): Promise<[number, number]> {
    const p = this.p;
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x00);
    await p.wrByte(0xff, 0x00);
    const vhv = vhvEnable ? await p.rdByte(0xcb) : 0;
    const phase = phaseEnable ? await p.rdByte(0xee) : 0;
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x01);
    await p.wrByte(0xff, 0x00);
    return [vhv, phase & 0xef];
  }

  /** VL53L0X_PerformRefCalibration(): VHV, then phase → [vhv, phase]. */
  async performRefCalibration(): Promise<[number, number]> {
    const sequenceConfig = this.d.SequenceConfig;

    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, 0x01); // VHV only
    await this.performSingleRefCalibration(0x40);
    const [vhv] = await this.refCalibrationIoRead(true, false);

    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, 0x02); // phase only
    await this.performSingleRefCalibration(0x00);
    const [, phase] = await this.refCalibrationIoRead(false, true);

    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, sequenceConfig);
    this.d.SequenceConfig = sequenceConfig;
    return [vhv, phase];
  }

  // ── NVM ──
  async deviceReadStrobe(): Promise<void> {
    await this.p.wrByte(0x83, 0x00);
    let ok = false;
    for (let i = 0; i < DEFAULT_MAX_LOOP; i++) {
      if ((await this.p.rdByte(0x83)) !== 0x00) {
        ok = true;
        break;
      }
    }
    if (!ok) throw new Vl53Error("timeout waiting for the NVM read strobe");
    await this.p.wrByte(0x83, 0x01);
  }

  /**
   * VL53L0X_get_info_from_device(): the fuse copy of the reference SPAD
   * record (option 1), the product identification (option 2) and the part
   * UID plus the factory offset (option 4).
   */
  async getInfoFromDevice(option: number): Promise<void> {
    const d = this.d;
    const done = d.ReadDataFromDeviceDone;
    if (done === 7) return;
    const p = this.p;

    await p.wrByte(0x80, 0x01);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x00);

    await p.wrByte(0xff, 0x06);
    await p.wrByte(0x83, (await p.rdByte(0x83)) | 4);
    await p.wrByte(0xff, 0x07);
    await p.wrByte(0x81, 0x01);
    await p.sleepMs(1); // VL53L0X_PollingDelay()
    await p.wrByte(0x80, 0x01);

    const nvmDword = async (index: number): Promise<number> => {
      await p.wrByte(0x94, index);
      await this.deviceReadStrobe();
      return p.rdDword(0x90);
    };

    const goodSpadMap = [...d.RefGoodSpadMap];
    let refSpadCount = 0;
    let refSpadType = 0;
    let productId = "";
    let moduleId = 0;
    let revision = 0;
    let partUidUpper = 0;
    let partUidLower = 0;
    let signalRateMeas1104400mm = 0;
    let distMeas1104400mm = 0;

    if ((option & 1) === 1 && (done & 1) === 0) {
      let tmp = await nvmDword(0x6b);
      refSpadCount = (tmp >>> 8) & 0x7f;
      refSpadType = (tmp >>> 15) & 0x01;

      tmp = await nvmDword(0x24);
      goodSpadMap[0] = (tmp >>> 24) & 0xff;
      goodSpadMap[1] = (tmp >>> 16) & 0xff;
      goodSpadMap[2] = (tmp >>> 8) & 0xff;
      goodSpadMap[3] = tmp & 0xff;

      tmp = await nvmDword(0x25);
      goodSpadMap[4] = (tmp >>> 24) & 0xff;
      goodSpadMap[5] = (tmp >>> 16) & 0xff;
    }

    if ((option & 2) === 2 && (done & 2) === 0) {
      await p.wrByte(0x94, 0x02);
      await this.deviceReadStrobe();
      moduleId = await p.rdByte(0x90);

      await p.wrByte(0x94, 0x7b);
      await this.deviceReadStrobe();
      revision = await p.rdByte(0x90);

      const chars: number[] = [];
      let tmp = await nvmDword(0x77);
      chars.push((tmp >>> 25) & 0x7f, (tmp >>> 18) & 0x7f, (tmp >>> 11) & 0x7f, (tmp >>> 4) & 0x7f);
      let byte = (tmp & 0x00f) << 3;

      tmp = await nvmDword(0x78);
      chars.push(
        byte + ((tmp >>> 29) & 0x7f),
        (tmp >>> 22) & 0x7f,
        (tmp >>> 15) & 0x7f,
        (tmp >>> 8) & 0x7f,
        (tmp >>> 1) & 0x7f,
      );
      byte = (tmp & 0x001) << 6;

      tmp = await nvmDword(0x79);
      chars.push(
        byte + ((tmp >>> 26) & 0x7f),
        (tmp >>> 19) & 0x7f,
        (tmp >>> 12) & 0x7f,
        (tmp >>> 5) & 0x7f,
      );
      byte = (tmp & 0x01f) << 2;

      tmp = await nvmDword(0x7a);
      chars.push(
        byte + ((tmp >>> 30) & 0x7f),
        (tmp >>> 23) & 0x7f,
        (tmp >>> 16) & 0x7f,
        (tmp >>> 9) & 0x7f,
        (tmp >>> 2) & 0x7f,
      );
      productId = chars.map((c) => String.fromCharCode(c & 0x7f)).join("");
    }

    if ((option & 4) === 4 && (done & 4) === 0) {
      partUidUpper = await nvmDword(0x7b);
      partUidLower = await nvmDword(0x7c);

      signalRateMeas1104400mm = ((await nvmDword(0x73)) & 0xff) << 8;
      signalRateMeas1104400mm |= ((await nvmDword(0x74)) & 0xff000000) >>> 24;
      distMeas1104400mm = ((await nvmDword(0x75)) & 0xff) << 8;
      distMeas1104400mm |= ((await nvmDword(0x76)) & 0xff000000) >>> 24;
    }

    await p.wrByte(0x81, 0x00);
    await p.wrByte(0xff, 0x06);
    await p.wrByte(0x83, (await p.rdByte(0x83)) & 0xfb);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x01);
    await p.wrByte(0xff, 0x00);
    await p.wrByte(0x80, 0x00);

    if ((option & 1) === 1 && (done & 1) === 0) {
      d.ReferenceSpadCount = refSpadCount;
      d.ReferenceSpadType = refSpadType;
      d.RefGoodSpadMap = goodSpadMap;
    }
    if ((option & 2) === 2 && (done & 2) === 0) {
      d.ModuleId = moduleId;
      d.Revision = revision;
      d.ProductId = productId;
    }
    if ((option & 4) === 4 && (done & 4) === 0) {
      d.PartUIDUpper = partUidUpper;
      d.PartUIDLower = partUidLower;
      d.SignalRateMeasFixed400mm = shl(signalRateMeas1104400mm, 9);
      let offsetUm = 0;
      if (distMeas1104400mm !== 0) {
        const offset1104Mm = u32(distMeas1104400mm - (400 << 4));
        offsetUm = -shr(offset1104Mm * 1000, 4);
      }
      d.Part2PartOffsetAdjustmentNVMMicroMeter = offsetUm;
    }
    d.ReadDataFromDeviceDone = done | option;
  }

  // ── reference SPADs ──
  /** VL53L0X_set_reference_spads(): apply the NVM record and verify it. */
  async setReferenceSpads(count: number, isApertureSpads: number): Promise<void> {
    const startSelect = 0xb4;
    const spadArraySize = REF_SPAD_BUFFER_SIZE;
    const maxSpadCount = 44;
    const p = this.p;

    await p.wrByte(0xff, 0x01);
    await p.wrByte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    await p.wrByte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2c);
    await p.wrByte(0xff, 0x00);
    await p.wrByte(GLOBAL_CONFIG_REF_EN_START_SELECT, startSelect);

    const spadArray = new Array<number>(spadArraySize).fill(0);
    let currentSpadIndex = 0;
    if (isApertureSpads) {
      while (!isAperture(startSelect + currentSpadIndex) && currentSpadIndex < maxSpadCount) {
        currentSpadIndex += 1;
      }
    }

    const good = this.d.RefGoodSpadMap;
    for (let i = 0; i < count; i++) {
      const nextGood = getNextGoodSpad(good, spadArraySize, currentSpadIndex);
      if (nextGood === -1) throw new Vl53Error("ran out of good reference SPADs");
      if (isAperture(startSelect + nextGood) !== Boolean(isApertureSpads)) {
        throw new Vl53Error("the good SPAD map leaves the requested quadrant");
      }
      currentSpadIndex = nextGood;
      spadArray[floorDiv(currentSpadIndex, 8)]! |= 1 << currentSpadIndex % 8;
      currentSpadIndex += 1;
    }

    await p.wrMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, Uint8Array.from(spadArray));
    const check = await p.rdMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spadArraySize);
    if (!arraysEqual(spadArray, check)) throw new Vl53Error("reference SPAD map did not read back");

    this.d.RefSpadEnables = spadArray;
    this.d.ReferenceSpadCount = count;
    this.d.ReferenceSpadType = isApertureSpads;
  }

  // ── tuning settings ──
  /** VL53L0X_load_tuning_settings(): {count, address, bytes...} records. */
  async loadTuningSettings(buffer: Uint8Array): Promise<void> {
    let index = 0;
    while (buffer[index] !== 0) {
      const numberOfWrites = buffer[index]!;
      index += 1;
      if (numberOfWrites === 0xff) {
        // Host-side sigma parameters this port does not use.
        index += 3;
      } else if (numberOfWrites <= 4) {
        const address = buffer[index]!;
        index += 1;
        await this.p.wrMulti(address, buffer.slice(index, index + numberOfWrites));
        index += numberOfWrites;
      } else {
        throw new Vl53Error(`bad tuning record at offset ${index}`);
      }
    }
  }

  // ── GPIO / interrupt ──
  /** VL53L0X_SetGpioConfig() for pin 0 in its ranging-interrupt role. */
  async setGpioConfig(functionality: number, polarityLow = true): Promise<void> {
    await this.p.wrByte(SYSTEM_INTERRUPT_CONFIG_GPIO, functionality);
    const data = polarityLow ? 0x00 : 0x10;
    const current = await this.p.rdByte(GPIO_HV_MUX_ACTIVE_HIGH);
    await this.p.wrByte(GPIO_HV_MUX_ACTIVE_HIGH, (current & 0xef) | data);
    this.d.Pin0GpioFunctionality = functionality;
    await this.clearInterrupt();
  }

  /** VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three rounds. */
  async clearInterrupt(): Promise<void> {
    for (let i = 0; i < 3; i++) {
      await this.p.wrByte(SYSTEM_INTERRUPT_CLEAR, 0x01);
      await this.p.wrByte(SYSTEM_INTERRUPT_CLEAR, 0x00);
      if (((await this.p.rdByte(RESULT_INTERRUPT_STATUS)) & 0x07) === 0x00) return;
    }
    throw new Vl53Error("interrupt not cleared after three rounds");
  }

  async getInterruptMaskStatus(): Promise<number> {
    const byte = await this.p.rdByte(RESULT_INTERRUPT_STATUS);
    if (byte & 0x18) {
      throw new Vl53Error(
        `range error reported in RESULT_INTERRUPT_STATUS 0x${byte.toString(16).toUpperCase().padStart(2, "0")}`,
      );
    }
    return byte & 0x07;
  }

  async checkForDataReady(): Promise<boolean> {
    if (this.d.Pin0GpioFunctionality === GPIOFUNCTIONALITY_NEW_MEASURE_READY) {
      return (await this.getInterruptMaskStatus()) === GPIOFUNCTIONALITY_NEW_MEASURE_READY;
    }
    return Boolean((await this.p.rdByte(RESULT_RANGE_STATUS)) & 0x01);
  }

  // ── sequence steps ──
  /** → enabled flag per sequence step (indexed by SEQUENCESTEP_*). */
  async getSequenceStepEnables(): Promise<boolean[]> {
    const config = await this.p.rdByte(SYSTEM_SEQUENCE_CONFIG);
    return SEQ_TEST.map((mask) => Boolean(config & mask));
  }

  async setSequenceStepEnable(step: number, enabled: number): Promise<void> {
    const config = await this.p.rdByte(SYSTEM_SEQUENCE_CONFIG);
    const next = enabled ? config | SEQ_SET[step]! : config & SEQ_CLEAR[step]!;
    if (next === config) return;
    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, next);
    this.d.SequenceConfig = next;
    // The budget is spread over the enabled steps, so it has to be re-applied.
    await this.setMeasurementTimingBudget(this.d.MeasurementTimingBudgetMicroSeconds);
  }

  async getVcselPulsePeriod(periodType: number): Promise<number> {
    const reg =
      periodType === VCSEL_PERIOD_PRE_RANGE
        ? PRE_RANGE_CONFIG_VCSEL_PERIOD
        : FINAL_RANGE_CONFIG_VCSEL_PERIOD;
    return decodeVcselPeriod(await this.p.rdByte(reg));
  }

  async getSequenceStepTimeout(step: number): Promise<number> {
    if (step === SEQUENCESTEP_TCC || step === SEQUENCESTEP_DSS || step === SEQUENCESTEP_MSRC) {
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE);
      const mclks = decodeTimeout(await this.p.rdByte(MSRC_CONFIG_TIMEOUT_MACROP));
      return calcTimeoutUs(mclks, vcsel);
    }
    if (step === SEQUENCESTEP_PRE_RANGE) {
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE);
      const mclks = decodeTimeout(await this.p.rdWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
      return calcTimeoutUs(mclks, vcsel);
    }
    if (step === SEQUENCESTEP_FINAL_RANGE) {
      const steps = await this.getSequenceStepEnables();
      let preMclks = 0;
      if (steps[SEQUENCESTEP_PRE_RANGE]) {
        preMclks = decodeTimeout(await this.p.rdWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
      }
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE);
      const finalMclks = decodeTimeout(await this.p.rdWord(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));
      return calcTimeoutUs((finalMclks - preMclks) & 0xffff, vcsel);
    }
    throw new Vl53Error(`no timeout for sequence step ${step}`);
  }

  async setSequenceStepTimeout(step: number, timeoutUs: number): Promise<void> {
    if (step === SEQUENCESTEP_TCC || step === SEQUENCESTEP_DSS || step === SEQUENCESTEP_MSRC) {
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE);
      const mclks = calcTimeoutMclks(timeoutUs, vcsel);
      const encoded = mclks > 256 ? 255 : (mclks - 1) & 0xff;
      await this.p.wrByte(MSRC_CONFIG_TIMEOUT_MACROP, encoded);
    } else if (step === SEQUENCESTEP_PRE_RANGE) {
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE);
      const mclks = calcTimeoutMclks(timeoutUs, vcsel);
      await this.p.wrWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout(mclks));
      this.d.PreRangeTimeoutMicroSecs = timeoutUs;
    } else if (step === SEQUENCESTEP_FINAL_RANGE) {
      // The final-range register carries pre-range + final range, summed in
      // macro periods (the two steps run at different VCSEL periods).
      const steps = await this.getSequenceStepEnables();
      let preMclks = 0;
      if (steps[SEQUENCESTEP_PRE_RANGE]) {
        preMclks = decodeTimeout(await this.p.rdWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
      }
      const vcsel = await this.getVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE);
      const mclks = calcTimeoutMclks(timeoutUs, vcsel) + preMclks;
      await this.p.wrWord(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout(mclks));
      this.d.FinalRangeTimeoutMicroSecs = timeoutUs;
    } else {
      throw new Vl53Error(`no timeout for sequence step ${step}`);
    }
  }

  // ── timing budget ──
  async setMeasurementTimingBudget(budgetUs: number): Promise<void> {
    const steps = await this.getSequenceStepEnables();
    let finalBudgetUs = budgetUs - (START_OVERHEAD_US + END_OVERHEAD_US);

    const take = (subTimeout: number): void => {
      if (subTimeout >= finalBudgetUs) {
        throw new Vl53Error(
          `timing budget ${budgetUs} us is too small for the enabled sequence steps`,
        );
      }
      finalBudgetUs -= subTimeout;
    };

    if (steps[SEQUENCESTEP_TCC] || steps[SEQUENCESTEP_MSRC] || steps[SEQUENCESTEP_DSS]) {
      // TCC, MSRC and DSS share one timeout.
      const msrcUs = await this.getSequenceStepTimeout(SEQUENCESTEP_MSRC);
      if (steps[SEQUENCESTEP_TCC]) take(msrcUs + TCC_OVERHEAD_US);
      if (steps[SEQUENCESTEP_DSS]) take(2 * (msrcUs + DSS_OVERHEAD_US));
      else if (steps[SEQUENCESTEP_MSRC]) take(msrcUs + MSRC_OVERHEAD_US);
    }

    if (steps[SEQUENCESTEP_PRE_RANGE]) {
      const preUs = await this.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE);
      take(preUs + PRE_RANGE_OVERHEAD_US);
    }

    if (steps[SEQUENCESTEP_FINAL_RANGE]) {
      // Whatever is left goes to the final range.
      finalBudgetUs -= FINAL_RANGE_OVERHEAD_US;
      await this.setSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE, finalBudgetUs);
      this.d.MeasurementTimingBudgetMicroSeconds = budgetUs;
    }
  }

  async getMeasurementTimingBudget(): Promise<number> {
    const steps = await this.getSequenceStepEnables();
    let budgetUs = START_OVERHEAD_US + END_OVERHEAD_US;

    if (steps[SEQUENCESTEP_TCC] || steps[SEQUENCESTEP_MSRC] || steps[SEQUENCESTEP_DSS]) {
      const msrcUs = await this.getSequenceStepTimeout(SEQUENCESTEP_MSRC);
      if (steps[SEQUENCESTEP_TCC]) budgetUs += msrcUs + TCC_OVERHEAD_US;
      if (steps[SEQUENCESTEP_DSS]) budgetUs += 2 * (msrcUs + DSS_OVERHEAD_US);
      else if (steps[SEQUENCESTEP_MSRC]) budgetUs += msrcUs + MSRC_OVERHEAD_US;
    }
    if (steps[SEQUENCESTEP_PRE_RANGE]) {
      budgetUs += (await this.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE)) + PRE_RANGE_OVERHEAD_US;
    }
    if (steps[SEQUENCESTEP_FINAL_RANGE]) {
      budgetUs +=
        (await this.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)) + FINAL_RANGE_OVERHEAD_US;
    }
    this.d.MeasurementTimingBudgetMicroSeconds = budgetUs;
    return budgetUs;
  }

  async setInterMeasurementPeriod(periodMs: number): Promise<void> {
    const oscCalibrateVal = await this.p.rdWord(OSC_CALIBRATE_VAL);
    const value = oscCalibrateVal ? periodMs * oscCalibrateVal : periodMs;
    await this.p.wrDword(SYSTEM_INTERMEASUREMENT_PERIOD, value);
  }

  async getInterMeasurementPeriod(): Promise<number> {
    const oscCalibrateVal = await this.p.rdWord(OSC_CALIBRATE_VAL);
    const value = await this.p.rdDword(SYSTEM_INTERMEASUREMENT_PERIOD);
    return oscCalibrateVal ? floorDiv(value, oscCalibrateVal) : value;
  }

  // ── the family-wide timing entry point ──
  /** Inter-measurement 0 = back-to-back continuous, else timed continuous. */
  async setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void> {
    await this.setMeasurementTimingBudget(timingBudgetMs * 1000);
    await this.setInterMeasurementPeriod(interMeasurementMs);
    this.setDeviceMode(
      interMeasurementMs === 0 ? DEVICEMODE_CONTINUOUS_RANGING : DEVICEMODE_CONTINUOUS_TIMED,
    );
  }

  async getRangeTiming(): Promise<[number, number]> {
    const budget = floorDiv(await this.getMeasurementTimingBudget(), 1000);
    return [budget, await this.getInterMeasurementPeriod()];
  }

  // ── ranging ──
  setDeviceMode(mode: number): void {
    if (
      mode !== DEVICEMODE_SINGLE_RANGING &&
      mode !== DEVICEMODE_CONTINUOUS_RANGING &&
      mode !== DEVICEMODE_CONTINUOUS_TIMED
    ) {
      throw new Vl53Error(`device mode ${mode} is not supported`);
    }
    this.d.DeviceMode = mode;
  }

  /** The undocumented prologue of every VL53L0X_StartMeasurement(). */
  async armStopVariable(): Promise<void> {
    const p = this.p;
    await p.wrByte(0x80, 0x01);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x00);
    await p.wrByte(0x91, this.d.StopVariable);
    await p.wrByte(0x00, 0x01);
    await p.wrByte(0xff, 0x00);
    await p.wrByte(0x80, 0x00);
  }

  /** VL53L0X_StartMeasurement() for the two continuous modes. */
  async startRanging(): Promise<void> {
    await this.armStopVariable();
    const mode = this.d.DeviceMode;
    if (mode === DEVICEMODE_CONTINUOUS_RANGING) {
      await this.p.wrByte(SYSRANGE_START, SYSRANGE_MODE_BACKTOBACK);
    } else if (mode === DEVICEMODE_CONTINUOUS_TIMED) {
      await this.p.wrByte(SYSRANGE_START, SYSRANGE_MODE_TIMED);
    } else {
      throw new Vl53Error("start_ranging() needs a continuous device mode");
    }
  }

  /** VL53L0X_StopMeasurement(). */
  async stopRanging(): Promise<void> {
    const p = this.p;
    await p.wrByte(SYSRANGE_START, SYSRANGE_MODE_SINGLESHOT);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(0x00, 0x00);
    await p.wrByte(0x91, 0x00);
    await p.wrByte(0x00, 0x01);
    await p.wrByte(0xff, 0x00);
  }

  /** VL53L0X_PerformSingleRangingMeasurement(): one shot, start to finish. */
  async performSingleRangingMeasurement(): Promise<RangingMeasurementData> {
    this.setDeviceMode(DEVICEMODE_SINGLE_RANGING);
    await this.armStopVariable();
    await this.p.wrByte(SYSRANGE_START, 0x01);
    let cleared = false;
    for (let i = 0; i < DEFAULT_MAX_LOOP; i++) {
      if (!((await this.p.rdByte(SYSRANGE_START)) & SYSRANGE_MODE_START_STOP)) {
        cleared = true;
        break;
      }
    }
    if (!cleared) throw new Vl53Error("the single-shot start bit never cleared");
    await this.waitDataReady();
    const data = await this.getResult();
    await this.clearInterrupt();
    return data;
  }

  async getResult(): Promise<RangingMeasurementData> {
    return this.parseResultBlock(await this.p.rdMulti(L0X_RESULT_BLOCK_ADDR, L0X_RESULT_BLOCK_LEN));
  }

  // ── result decoding ──
  /**
   * VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14. Pure
   * arithmetic over the block and the cached device data.
   */
  parseResultBlock(raw: Uint8Array): RangingMeasurementData {
    if (raw.length < L0X_RESULT_BLOCK_LEN) {
      throw new Vl53Error(`result block too short: ${raw.length} bytes`);
    }
    const distance = (raw[10]! << 8) + raw[11]!;
    const signalRateMcps = ((raw[6]! << 8) + raw[7]!) * 512; // 9.7 → 16.16
    const ambientRateMcps = ((raw[8]! << 8) + raw[9]!) * 512;
    const effectiveSpads = (raw[2]! << 8) + raw[3]!; // 8.8
    const deviceRangeStatus = raw[0]!;

    let rangeMm: number;
    let fractional: number;
    if (this.d.RangeFractionalEnable) {
      rangeMm = distance >> 2;
      fractional = (distance & 0x03) << 6;
    } else {
      rangeMm = distance;
      fractional = 0;
    }
    const data: RangingMeasurementData = {
      rangeStatus: 0,
      distanceMm: rangeMm,
      rangeFractionalPart: fractional,
      signalRateMcps,
      ambientRateMcps,
      effectiveSpadRtnCount: effectiveSpads,
      dmaxMm: 0,
      sigmaMm: 0,
      deviceRangeStatus,
    };
    this.getPalRangeStatus(data);
    return data;
  }

  getTotalXtalkRate(data: RangingMeasurementData): bigint {
    if (!this.d.XTalkCompensationEnable) return 0n;
    const total =
      BigInt(data.effectiveSpadRtnCount) * BigInt(this.d.XTalkCompensationRateMegaCps);
    return (total + 0x80n) >> 8n;
  }

  /** VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out (BigInt). */
  calcSigmaEstimate(data: RangingMeasurementData): number {
    const cPulseEffectiveWidthCentiNs = 800n;
    const cAmbientEffectiveWidthCentiNs = 600n;
    const cDfltFinalRangeIntegrationTimeMs = 0x00190000n; // 25 ms
    const cVcselPulseWidthPs = 4700n;
    const cSigmaEstMax = 0x028f87aen;
    const cSigmaEstRtnMax = 0xf000n;
    const cAmbToSignalRatioMax = 0xf0000000n / cAmbientEffectiveWidthCentiNs;
    const cTofPerMmPs = 0x0006999an;
    const c16bitRoundingParam = 0x00008000n;
    const cMaxXtalkKcps = 0x00320000n;
    const cPllPeriodPs = 1655n;
    const M32 = 0xffffffffn;
    const min = (a: bigint, b: bigint): bigint => (a < b ? a : b);

    const ambientRateKcps = (BigInt(data.ambientRateMcps) * 1000n) >> 16n;
    const xtalkCompRateMcps = this.getTotalXtalkRate(data);
    const totalSignalRateMcps = BigInt(data.signalRateMcps) + xtalkCompRateMcps;

    const peakSignalRateKcps = (totalSignalRateMcps * 1000n + 0x8000n) >> 16n;
    const xtalkCompRateKcps = min(xtalkCompRateMcps * 1000n, cMaxXtalkKcps);

    const finalRangeTimeoutUs = this.d.FinalRangeTimeoutMicroSecs;
    const finalRangeVcselPclks = this.d.FinalRangeVcselPulsePeriod;
    const finalRangeMacroPclks = BigInt(calcTimeoutMclks(finalRangeTimeoutUs, finalRangeVcselPclks));
    const preRangeTimeoutUs = this.d.PreRangeTimeoutMicroSecs;
    const preRangeVcselPclks = this.d.PreRangeVcselPulsePeriod;
    const preRangeMacroPclks = BigInt(calcTimeoutMclks(preRangeTimeoutUs, preRangeVcselPclks));

    const vcselWidth = finalRangeVcselPclks === 8 ? 2n : 3n;
    let peakVcselDurationUs = vcselWidth * 2048n * (preRangeMacroPclks + finalRangeMacroPclks);
    peakVcselDurationUs = pyFloorDiv(peakVcselDurationUs + 500n, 1000n);
    peakVcselDurationUs *= cPllPeriodPs;
    peakVcselDurationUs = pyFloorDiv(peakVcselDurationUs + 500n, 1000n);

    const totalSignalRate2408 = (totalSignalRateMcps + 0x80n) >> 8n;
    let vcselTotalEventsRtn = (totalSignalRate2408 * peakVcselDurationUs + 0x80n) >> 8n;

    if (peakSignalRateKcps === 0n) return Number(cSigmaEstMax);

    if (vcselTotalEventsRtn < 1n) vcselTotalEventsRtn = 1n;

    const sigmaEstimateP1 = cPulseEffectiveWidthCentiNs;
    let sigmaEstimateP2 = pyFloorDiv(ambientRateKcps << 16n, peakSignalRateKcps);
    sigmaEstimateP2 = min(sigmaEstimateP2, cAmbToSignalRatioMax);
    sigmaEstimateP2 *= cAmbientEffectiveWidthCentiNs;
    const sigmaEstimateP3 = 2n * isqrt(vcselTotalEventsRtn * 12n);

    const deltaTPs = BigInt(data.distanceMm) * cTofPerMmPs;

    let diff1Mcps = pyFloorDiv(
      ((((peakSignalRateKcps << 16n) - 2n * xtalkCompRateKcps) & M32) + 500n) & M32,
      1000n,
    );
    const diff2Mcps = pyFloorDiv((peakSignalRateKcps << 16n) + 500n, 1000n);
    diff1Mcps <<= 8n;
    let q = pyFloorDiv(diff1Mcps, diff2Mcps);
    if (q < 0n) q = -q;
    const xtalkCorrection = q << 8n;

    let pwMult = pyFloorDiv(deltaTPs, cVcselPulseWidthPs);
    pwMult = (pwMult * (((1n << 16n) - xtalkCorrection) & M32)) & M32;
    pwMult = (pwMult + c16bitRoundingParam) >> 16n;
    pwMult += 1n << 16n;
    // Squaring 1.xx would leave 32 bits, so the C driver halves it first.
    pwMult >>= 1n;
    pwMult = (pwMult * pwMult) >> 14n;

    let sqr1 = (pwMult * sigmaEstimateP1 + 0x8000n) >> 16n;
    sqr1 *= sqr1;
    let sqr2 = (sigmaEstimateP2 + 0x8000n) >> 16n;
    sqr2 *= sqr2;

    const sqrtResultCentiNs = isqrt(sqr1 + sqr2) << 16n;
    let sigmaEstRtn = pyFloorDiv(pyFloorDiv(sqrtResultCentiNs + 50n, 100n), sigmaEstimateP3);
    sigmaEstRtn *= BigInt(SPEED_OF_LIGHT_IN_AIR);
    sigmaEstRtn = pyFloorDiv(sigmaEstRtn + 5000n, 10000n);
    sigmaEstRtn = min(sigmaEstRtn, cSigmaEstRtnMax);

    const finalRangeIntegrationTimeMs = BigInt(
      floorDiv(finalRangeTimeoutUs + preRangeTimeoutUs + 500, 1000),
    );
    // 1 mm * 25 ms / the actual integration time.
    let sigmaEstRef =
      isqrt(
        pyFloorDiv(
          cDfltFinalRangeIntegrationTimeMs + pyFloorDiv(finalRangeIntegrationTimeMs, 2n),
          finalRangeIntegrationTimeMs,
        ),
      ) << 8n;
    sigmaEstRef = pyFloorDiv(sigmaEstRef + 500n, 1000n);

    let sigmaEstimate = 1000n * isqrt(sigmaEstRtn * sigmaEstRtn + sigmaEstRef * sigmaEstRef);
    if (peakSignalRateKcps < 1n || vcselTotalEventsRtn < 1n || sigmaEstimate > cSigmaEstMax) {
      sigmaEstimate = cSigmaEstMax;
    }
    return Number(sigmaEstimate);
  }

  /** VL53L0X_calc_dmax(): ambient-rate → max-range lookup, interpolated. */
  calcDmax(ambRateMeas: number): number {
    const amb = DMAX_LUT_AMB_RATE_MCPS;
    const dmax = DMAX_LUT_DMAX_MM;
    const last = amb.length - 1;
    if (ambRateMeas <= amb[0]!) return shr(dmax[0]!, 16);
    if (ambRateMeas >= amb[last]!) return shr(dmax[last]!, 16);

    const index1 = amb.findIndex((a) => ambRateMeas <= a);
    const index0 = index1 ? index1 - 1 : 0;
    if (index0 === index1) return shr(dmax[index0]!, 16);

    const amb0 = amb[index0]!;
    const amb1 = amb[index1]!;
    const dmax0 = dmax[index0]!;
    const dmax1 = dmax[index1]!;
    if (amb1 === amb0) return shr(dmax0, 16);
    const linearSlope = floorDiv(u32(dmax0 - dmax1), shr(amb1 - amb0, 8));
    return shr(shr(amb1 - ambRateMeas, 8) * linearSlope + dmax1, 16);
  }

  /** VL53L0X_get_pal_range_status(): device status plus the enabled limit checks. */
  getPalRangeStatus(data: RangingMeasurementData): void {
    const d = this.d;
    const internal = (data.deviceRangeStatus & 0x78) >> 3;
    const noneFlag = [0, 5, 7, 12, 13, 14, 15].includes(internal);

    let sigmaLimitFlag = false;
    if (d.LimitChecksEnable[CHECK_SIGMA_FINAL_RANGE]) {
      const sigmaEstimate = this.calcSigmaEstimate(data);
      data.sigmaMm = shr(sigmaEstimate, 16);
      data.dmaxMm = this.calcDmax(data.ambientRateMcps);
      const sigmaLimitValue = d.LimitChecksValue[CHECK_SIGMA_FINAL_RANGE]!;
      sigmaLimitFlag = sigmaLimitValue > 0 && sigmaEstimate > sigmaLimitValue;
    }
    const signalRefClipFlag = false; // check disabled by DataInit
    const rangeIgnoreFlag = false; // check disabled by DataInit

    let status: number;
    if (noneFlag) status = 255;
    else if (internal === 1 || internal === 2 || internal === 3) status = 5; // hardware fail
    else if (internal === 6 || internal === 9) status = 4; // phase fail
    else if (internal === 8 || internal === 10 || signalRefClipFlag) status = 3; // min range
    else if (internal === 4 || rangeIgnoreFlag) status = 2; // signal fail
    else if (sigmaLimitFlag) status = 1; // sigma fail
    else status = 0;
    data.rangeStatus = status;
  }

  // ── VCSEL pulse period and the ranging profiles ──
  /** VL53L0X_set_vcsel_pulse_period(). */
  async setVcselPulsePeriod(periodType: number, pclks: number): Promise<void> {
    const limits: Record<number, Record<number, number>> = {
      [VCSEL_PERIOD_PRE_RANGE]: { 12: 0x18, 14: 0x30, 16: 0x40, 18: 0x50 },
      [VCSEL_PERIOD_FINAL_RANGE]: { 8: 0x10, 10: 0x28, 12: 0x38, 14: 0x48 },
    };
    const table = limits[periodType] ?? {};
    if (pclks % 2 || !(pclks in table)) {
      const kind = periodType === VCSEL_PERIOD_PRE_RANGE ? "pre-range" : "final range";
      throw new Vl53Error(`VCSEL period ${pclks} PCLK is out of range for the ${kind}`);
    }
    const p = this.p;

    if (periodType === VCSEL_PERIOD_PRE_RANGE) {
      await p.wrByte(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, table[pclks]!);
      await p.wrByte(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
    } else {
      const finals: Record<number, [number, number, number]> = {
        8: [0x02, 0x0c, 0x30],
        10: [0x03, 0x09, 0x20],
        12: [0x03, 0x08, 0x20],
        14: [0x03, 0x07, 0x20],
      };
      const [width, timeout, phasecalLim] = finals[pclks]!;
      await p.wrByte(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, table[pclks]!);
      await p.wrByte(FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
      await p.wrByte(GLOBAL_CONFIG_VCSEL_WIDTH, width);
      await p.wrByte(ALGO_PHASECAL_CONFIG_TIMEOUT, timeout);
      await p.wrByte(0xff, 0x01);
      await p.wrByte(ALGO_PHASECAL_LIM, phasecalLim);
      await p.wrByte(0xff, 0x00);
    }

    const vcselPeriodReg = encodeVcselPeriod(pclks);
    if (periodType === VCSEL_PERIOD_PRE_RANGE) {
      const preTimeout = await this.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE);
      const msrcTimeout = await this.getSequenceStepTimeout(SEQUENCESTEP_MSRC);
      await p.wrByte(PRE_RANGE_CONFIG_VCSEL_PERIOD, vcselPeriodReg);
      await this.setSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE, preTimeout);
      await this.setSequenceStepTimeout(SEQUENCESTEP_MSRC, msrcTimeout);
      this.d.PreRangeVcselPulsePeriod = pclks;
    } else {
      const finalTimeout = await this.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE);
      await p.wrByte(FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcselPeriodReg);
      await this.setSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE, finalTimeout);
      this.d.FinalRangeVcselPulsePeriod = pclks;
    }

    await this.setMeasurementTimingBudget(this.d.MeasurementTimingBudgetMicroSeconds);
    await this.performPhaseCalibration();
  }

  /** VL53L0X_perform_phase_calibration() with restore_config = 1. */
  async performPhaseCalibration(): Promise<void> {
    const sequenceConfig = this.d.SequenceConfig;
    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, 0x02);
    await this.performSingleRefCalibration(0x00);
    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, sequenceConfig);
    this.d.SequenceConfig = sequenceConfig;
  }

  /** Apply one of MODES: limits, budget and both VCSEL periods. */
  override async setMode(name: string): Promise<void> {
    const settings = L0X_MODE_SETTINGS.get(name);
    if (settings === undefined) {
      throw new Vl53Error(`no such mode: ${name} (have ${this.MODES.join(", ")})`);
    }
    const [signalMcps, sigmaMm, budgetUs, prePclks, finalPclks] = settings;
    await this.setLimitCheckEnable(CHECK_SIGMA_FINAL_RANGE, 1);
    await this.setLimitCheckEnable(CHECK_SIGNAL_RATE_FINAL_RANGE, 1);
    await this.setLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE, Math.trunc(signalMcps * 65536));
    await this.setLimitCheckValue(CHECK_SIGMA_FINAL_RANGE, sigmaMm * 65536);
    await this.setMeasurementTimingBudget(budgetUs);
    await this.setVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE, prePclks);
    await this.setVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE, finalPclks);
    this.mode = name;
  }

  override async getMode(): Promise<string> {
    return this.mode;
  }

  /** What the five registers a mode writes currently read back as. */
  override async driverInfo(): Promise<Record<string, unknown>> {
    return {
      signal_mcps: (await this.getLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE)) / 65536.0,
      sigma_mm: (await this.getLimitCheckValue(CHECK_SIGMA_FINAL_RANGE)) / 65536.0,
      budget_us: await this.getMeasurementTimingBudget(),
      pre_pclks: await this.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE),
      final_pclks: await this.getVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE),
    };
  }

  // ── offset ──
  /** 10.2-format register: stored in steps of 250 um. */
  async setOffsetUm(offsetUm: number): Promise<void> {
    offsetUm = Math.max(-512000, Math.min(511000, offsetUm));
    const steps = floorDiv(Math.abs(offsetUm), 250); // C truncates toward zero
    const encoded = offsetUm >= 0 ? steps : 4096 - steps;
    await this.p.wrWord(ALGO_PART_TO_PART_RANGE_OFFSET_MM, encoded & 0xffff);
  }

  async getOffsetUm(): Promise<number> {
    const register = (await this.p.rdWord(ALGO_PART_TO_PART_RANGE_OFFSET_MM)) & 0x0fff;
    return register > 2047 ? (register - 4096) * 250 : register * 250;
  }

  setOffset(offsetMm: number): Promise<void> {
    return this.setOffsetUm(offsetMm * 1000);
  }

  async getOffset(): Promise<number> {
    return pyRound((await this.getOffsetUm()) / 1000.0);
  }

  /** VL53L0X_perform_offset_calibration() → the offset in micrometres. */
  async performOffsetCalibration(calDistanceMm: number, nbSamples = 50): Promise<number> {
    if (calDistanceMm <= 0) throw new Vl53Error("the calibration distance must be positive");

    await this.setOffsetUm(0);
    const tccWasOn = (await this.getSequenceStepEnables())[SEQUENCESTEP_TCC];
    await this.setSequenceStepEnable(SEQUENCESTEP_TCC, 0);
    await this.setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);

    let sumRanging = 0;
    let count = 0;
    for (let i = 0; i < nbSamples; i++) {
      const data = await this.performSingleRangingMeasurement();
      if (data.rangeStatus === 0) {
        sumRanging = (sumRanging + data.distanceMm) & 0xffff;
        count += 1;
      }
    }
    if (count === 0) {
      throw new Vl53Error(
        "offset calibration got no valid measurement - check that a target is in front of the sensor",
      );
    }
    const meanMm = floorDiv(2 * sumRanging + count, 2 * count); // round half up
    if (meanMm === 0) {
      throw new Vl53Error(
        "the mean range came out 0 mm - the target is closer than the part-to-part offset; " +
          "move it out to 100..400 mm and calibrate again",
      );
    }
    const offsetUm = (calDistanceMm - meanMm) * 1000;
    await this.setOffsetUm(offsetUm);

    if (tccWasOn) await this.setSequenceStepEnable(SEQUENCESTEP_TCC, 1);
    return offsetUm;
  }

  /** The family-wide contract: an offset in whole millimetres. */
  async calibrateOffset(targetDistMm: number, nbSamples = 50): Promise<number> {
    return pyRound((await this.performOffsetCalibration(targetDistMm, nbSamples)) / 1000.0);
  }

  // ── crosstalk ──
  /** VL53L0X_SetXTalkCompensationEnable(). */
  async setXtalkEnable(enable: number): Promise<void> {
    const rate = enable ? this.d.XTalkCompensationRateMegaCps : 0;
    await this.p.wrWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, shr(rate, 3) & 0xffff); // 16.16 → 3.13
    this.d.XTalkCompensationEnable = enable ? 1 : 0;
  }

  /** VL53L0X_SetXTalkCompensationRateMegaCps(), FixPoint16.16 in. */
  async setXtalkRateMcps(rateMcps: number): Promise<void> {
    if (this.d.XTalkCompensationEnable) {
      await this.p.wrWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, shr(rateMcps, 3) & 0xffff);
    }
    this.d.XTalkCompensationRateMegaCps = rateMcps;
  }

  async getXtalkRateMcps(): Promise<number> {
    return shl(await this.p.rdWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS), 3);
  }

  /** The family-wide contract speaks kcps; the ULD speaks Mcps 16.16. */
  async setXtalk(xtalkKcps: number): Promise<void> {
    await this.setXtalkEnable(1);
    await this.setXtalkRateMcps(floorDiv(shl(xtalkKcps, 16), 1000));
  }

  async getXtalk(): Promise<number> {
    return shr((await this.getXtalkRateMcps()) * 1000, 16);
  }

  /** VL53L0X_perform_xtalk_calibration() → the rate in Mcps 16.16. */
  async performXtalkCalibration(calDistanceMm: number, nbSamples = 50): Promise<number> {
    if (calDistanceMm <= 0) throw new Vl53Error("the calibration distance must be positive");

    await this.setXtalkEnable(0);
    await this.setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);

    let sumRanging = 0;
    let sumSignal = 0;
    let sumSpads = 0;
    let count = 0;
    for (let i = 0; i < nbSamples; i++) {
      const data = await this.performSingleRangingMeasurement();
      if (data.rangeStatus === 0) {
        // The C sums are uint16 / uint32; kept that way on purpose.
        sumRanging = (sumRanging + data.distanceMm) & 0xffff;
        sumSignal = u32(sumSignal + data.signalRateMcps);
        sumSpads = (sumSpads + floorDiv(data.effectiveSpadRtnCount, 256)) & 0xffff;
        count += 1;
      }
    }
    if (count === 0) {
      throw new Vl53Error(
        "crosstalk calibration got no valid measurement - check that a target is in front of the sensor",
      );
    }

    const meanSignal = floorDiv(sumSignal, count);
    const meanRange = floorDiv(u32(shl(sumRanging, 16)), count);
    const meanSpads = floorDiv(u32(shl(sumSpads, 16)), count);
    const meanSpadsInt = shr(meanSpads + 0x8000, 16);
    const calDistance = shl(calDistanceMm, 16);

    let rateMcps: number;
    if (meanSpadsInt === 0 || meanRange >= calDistance) {
      rateMcps = 0;
    } else {
      let perSpad = floorDiv(meanSignal, meanSpadsInt);
      perSpad = u32(perSpad * (65536 - floorDiv(meanRange, calDistanceMm)));
      rateMcps = shr(perSpad + 0x8000, 16);
    }

    await this.setXtalkEnable(1);
    await this.setXtalkRateMcps(rateMcps);
    return rateMcps;
  }

  /** The family-wide contract: a crosstalk rate in kcps. */
  async calibrateXtalk(targetDistMm: number, nbSamples = 50): Promise<number> {
    return shr((await this.performXtalkCalibration(targetDistMm, nbSamples)) * 1000, 16);
  }

  // ── reference SPAD management ──
  getReferenceSpads(): [number, number] {
    return [this.d.ReferenceSpadCount, this.d.ReferenceSpadType];
  }

  private setRefSpadMap(spadArray: number[]): Promise<void> {
    return this.p.wrMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, Uint8Array.from(spadArray));
  }

  private async getRefSpadMap(): Promise<number[]> {
    return Array.from(await this.p.rdMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, REF_SPAD_BUFFER_SIZE));
  }

  /** enable_ref_spads(): append good SPADs, apply and read back → next index. */
  private async enableRefSpads(
    apertureSpads: number,
    spadArray: number[],
    start: number,
    offset: number,
    spadCount: number,
  ): Promise<number> {
    const good = this.d.RefGoodSpadMap;
    let currentSpad = offset;
    for (let i = 0; i < spadCount; i++) {
      const nextGood = getNextGoodSpad(good, REF_SPAD_BUFFER_SIZE, currentSpad);
      if (nextGood === -1) throw new Vl53Error("ran out of good reference SPADs");
      if (isAperture(start + nextGood) !== Boolean(apertureSpads)) {
        throw new Vl53Error("the good SPAD map leaves the requested quadrant");
      }
      currentSpad = nextGood;
      spadArray[floorDiv(currentSpad, 8)]! |= 1 << currentSpad % 8;
      currentSpad += 1;
    }
    await this.setRefSpadMap(spadArray);
    if (!arraysEqual(await this.getRefSpadMap(), spadArray)) {
      throw new Vl53Error("reference SPAD map did not read back");
    }
    return currentSpad;
  }

  /** perform_ref_signal_measurement() → peak reference signal rate, 9.7. */
  private async performRefSignalMeasurement(): Promise<number> {
    const sequenceConfig = this.d.SequenceConfig;
    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, 0xc0);
    await this.performSingleRangingMeasurement();
    await this.p.wrByte(0xff, 0x01);
    const peak = await this.p.rdWord(RESULT_PEAK_SIGNAL_RATE_REF);
    await this.p.wrByte(0xff, 0x00);
    await this.p.wrByte(SYSTEM_SEQUENCE_CONFIG, sequenceConfig);
    this.d.SequenceConfig = sequenceConfig;
    return peak;
  }

  /** VL53L0X_perform_ref_spad_management() → [count, isAperture]. */
  async performRefSpadManagement(): Promise<[number, number]> {
    const startSelect = 0xb4;
    const minimumSpadCount = 3;
    const maxSpadCount = 44;
    const targetRefRate = this.d.TargetRefRate;
    const p = this.p;

    let spadArray = new Array<number>(REF_SPAD_BUFFER_SIZE).fill(0);
    await p.wrByte(0xff, 0x01);
    await p.wrByte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    await p.wrByte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2c);
    await p.wrByte(0xff, 0x00);
    await p.wrByte(GLOBAL_CONFIG_REF_EN_START_SELECT, startSelect);
    await p.wrByte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
    await this.performRefCalibration();

    let needAptSpads = 0;
    let currentSpadIndex = await this.enableRefSpads(
      needAptSpads,
      spadArray,
      startSelect,
      0,
      minimumSpadCount,
    );
    let peak = await this.performRefSignalMeasurement();

    if (peak > targetRefRate) {
      // Too bright even at the minimum: start over on aperture SPADs.
      spadArray = new Array<number>(REF_SPAD_BUFFER_SIZE).fill(0);
      while (!isAperture(startSelect + currentSpadIndex) && currentSpadIndex < maxSpadCount) {
        currentSpadIndex += 1;
      }
      needAptSpads = 1;
      currentSpadIndex = await this.enableRefSpads(
        needAptSpads,
        spadArray,
        startSelect,
        currentSpadIndex,
        minimumSpadCount,
      );
      peak = await this.performRefSignalMeasurement();
      if (peak > targetRefRate) {
        // Nothing more to give: the minimum aperture set is the answer.
        this.storeRefSpads(minimumSpadCount, 1, spadArray);
        return [minimumSpadCount, 1];
      }
    }

    let refSpadCount = minimumSpadCount;
    let lastSpadArray = [...spadArray];
    let lastDiff = Math.abs(peak - targetRefRate);

    while (peak < targetRefRate) {
      const nextGood = getNextGoodSpad(
        this.d.RefGoodSpadMap,
        REF_SPAD_BUFFER_SIZE,
        currentSpadIndex,
      );
      if (nextGood === -1) throw new Vl53Error("ran out of good reference SPADs");
      if (isAperture(startSelect + nextGood) !== Boolean(needAptSpads)) break; // quadrant exhausted

      refSpadCount += 1;
      currentSpadIndex = nextGood;
      if (floorDiv(currentSpadIndex, 8) >= REF_SPAD_BUFFER_SIZE) {
        throw new Vl53Error("reference SPAD index out of range");
      }
      spadArray[floorDiv(currentSpadIndex, 8)]! |= 1 << currentSpadIndex % 8;
      currentSpadIndex += 1;
      await this.setRefSpadMap(spadArray);

      peak = await this.performRefSignalMeasurement();
      const diff = Math.abs(peak - targetRefRate);
      if (peak > targetRefRate) {
        if (diff > lastDiff) {
          // The previous map came closer to the target; go back to it.
          await this.setRefSpadMap(lastSpadArray);
          spadArray = [...lastSpadArray];
          refSpadCount -= 1;
        }
        break;
      }
      lastDiff = diff;
      lastSpadArray = [...spadArray];
    }

    this.storeRefSpads(refSpadCount, needAptSpads, spadArray);
    return [refSpadCount, needAptSpads];
  }

  private storeRefSpads(count: number, isApertureSpads: number, spadArray: number[]): void {
    this.d.RefSpadEnables = [...spadArray];
    this.d.ReferenceSpadCount = count;
    this.d.ReferenceSpadType = isApertureSpads;
  }

  // ── limit checks ──
  async setLimitCheckEnable(check: number, enable: number): Promise<void> {
    const d = this.d;
    if (check >= CHECK_NUMBER_OF_CHECKS) throw new Vl53Error(`no limit check ${check}`);

    let value: number;
    let disable: number;
    if (enable === 0) {
      value = 0;
      disable = 1;
    } else {
      value = d.LimitChecksValue[check]!;
      disable = 0;
    }

    if (check === CHECK_SIGNAL_RATE_FINAL_RANGE) {
      await this.p.wrWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, shr(value, 9) & 0xffff);
    } else if (check === CHECK_SIGNAL_RATE_MSRC) {
      const current = await this.p.rdByte(MSRC_CONFIG_CONTROL);
      await this.p.wrByte(MSRC_CONFIG_CONTROL, (current & 0xfe) | (disable << 1));
    } else if (check === CHECK_SIGNAL_RATE_PRE_RANGE) {
      const current = await this.p.rdByte(MSRC_CONFIG_CONTROL);
      await this.p.wrByte(MSRC_CONFIG_CONTROL, (current & 0xef) | (disable << 4));
    }
    // The other three checks are host-side arithmetic only.
    d.LimitChecksEnable[check] = enable === 0 ? 0 : 1;
  }

  async setLimitCheckValue(check: number, value: number): Promise<void> {
    const d = this.d;
    if (!d.LimitChecksEnable[check]) {
      d.LimitChecksValue[check] = value; // disabled: keep it here
      return;
    }
    if (check === CHECK_SIGNAL_RATE_FINAL_RANGE) {
      await this.p.wrWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, shr(value, 9) & 0xffff);
    } else if (check === CHECK_SIGNAL_RATE_MSRC || check === CHECK_SIGNAL_RATE_PRE_RANGE) {
      await this.p.wrWord(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT, shr(value, 9) & 0xffff);
    }
    d.LimitChecksValue[check] = value;
  }

  async getLimitCheckValue(check: number): Promise<number> {
    if (check === CHECK_SIGNAL_RATE_FINAL_RANGE) {
      return shl(await this.p.rdWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT), 9);
    }
    if (check === CHECK_SIGNAL_RATE_MSRC || check === CHECK_SIGNAL_RATE_PRE_RANGE) {
      return shl(await this.p.rdWord(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT), 9);
    }
    return this.d.LimitChecksValue[check]!;
  }
}

/** Python `//` on BigInt (floor, not truncation). */
function pyFloorDiv(a: bigint, b: bigint): bigint {
  const q = a / b;
  return (a % b !== 0n) && ((a < 0n) !== (b < 0n)) ? q - 1n : q;
}

/**
 * RangingMeasurementData → the family-wide Measurement. The rates are
 * FixPoint16.16 Mcps in the ULD and kcps everywhere else (× 1000/65536).
 */
export function l0xAsMeasurement(r: RangingMeasurementData): Measurement {
  return measurement({
    distanceMm: r.distanceMm,
    status: r.rangeStatus,
    statusText: l0xStatusText(r.rangeStatus),
    signalKcps: shr(r.signalRateMcps * 1000, 16),
    ambientKcps: shr(r.ambientRateMcps * 1000, 16),
    sigmaMm: r.sigmaMm,
    spads: r.effectiveSpadRtnCount >> 8, // 8.8
    extra: { dmax_mm: r.dmaxMm, device_range_status: r.deviceRangeStatus },
  });
}
