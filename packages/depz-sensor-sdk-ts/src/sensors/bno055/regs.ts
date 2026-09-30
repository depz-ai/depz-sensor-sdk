/**
 * BNO055 register map and the pure codecs every SDK shares
 * (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8).
 * Mirrors the Python reference `depz_sensor_sdk.bno055.regs`.
 *
 * Nothing here touches the wire: these functions turn register bytes into
 * values and back, so they are what `vectors/bno055.json` pins.
 */

// ── page 0 ────────────────────────────────────────────────────────────────────
export const BNO055_REG_CHIP_ID = 0x00;
export const BNO055_REG_PAGE_ID = 0x07;
export const BNO055_REG_ACC_DATA = 0x08;
export const BNO055_REG_MAG_DATA = 0x0e;
export const BNO055_REG_GYR_DATA = 0x14;
/** heading, roll, pitch */
export const BNO055_REG_EUL_DATA = 0x1a;
/** w, x, y, z */
export const BNO055_REG_QUA_DATA = 0x20;
export const BNO055_REG_LIA_DATA = 0x28;
export const BNO055_REG_GRV_DATA = 0x2e;
export const BNO055_REG_TEMP = 0x34;
export const BNO055_REG_CALIB_STAT = 0x35;
export const BNO055_REG_ST_RESULT = 0x36;
/** Clear-on-read — never part of a routine block read. */
export const BNO055_REG_INT_STA = 0x37;
export const BNO055_REG_SYS_CLK_STATUS = 0x38;
export const BNO055_REG_SYS_STATUS = 0x39;
export const BNO055_REG_SYS_ERR = 0x3a;
export const BNO055_REG_UNIT_SEL = 0x3b;
export const BNO055_REG_OPR_MODE = 0x3d;
export const BNO055_REG_PWR_MODE = 0x3e;
export const BNO055_REG_SYS_TRIGGER = 0x3f;
export const BNO055_REG_TEMP_SOURCE = 0x40;
export const BNO055_REG_AXIS_MAP_CONFIG = 0x41;
export const BNO055_REG_AXIS_MAP_SIGN = 0x42;
/** 9 × i16, row-major. */
export const BNO055_REG_SIC_MATRIX = 0x43;
/** acc/mag/gyr offsets + acc/mag radius. */
export const BNO055_REG_CALIB_PROFILE = 0x55;
export const BNO055_CALIB_PROFILE_LEN = 22;

// ── page 1 ────────────────────────────────────────────────────────────────────
export const BNO055_REG1_ACC_CONFIG = 0x08;
export const BNO055_REG1_MAG_CONFIG = 0x09;
export const BNO055_REG1_GYR_CONFIG_0 = 0x0a;
export const BNO055_REG1_GYR_CONFIG_1 = 0x0b;
export const BNO055_REG1_INT_MSK = 0x0f;
export const BNO055_REG1_INT_EN = 0x10;
/** First / last page-1 motion-interrupt setting register (written raw). */
export const BNO055_REG1_INT_SETTINGS_FIRST = 0x11;
export const BNO055_REG1_INT_SETTINGS_LAST = 0x1f;
export const BNO055_REG1_UNIQUE_ID = 0x50;
export const BNO055_UNIQUE_ID_LEN = 16;

/** [addr, len] of the block carrying every output channel (0x08..0x35). */
export const BNO055_FULL_BLOCK: readonly [number, number] = [0x08, 46];
/** [addr, len] of the quaternion alone — the cheapest orientation read. */
export const BNO055_QUAT_BLOCK: readonly [number, number] = [0x20, 8];

// SYS_TRIGGER bits.
export const BNO055_SYS_TRIGGER_SELF_TEST = 0x01;
export const BNO055_SYS_TRIGGER_RST_SYS = 0x20;
export const BNO055_SYS_TRIGGER_RST_INT = 0x40;
export const BNO055_SYS_TRIGGER_CLK_SEL = 0x80;

/**
 * INT_EN / INT_MSK / INT_STA bits. The DRDY bits exist only on sensor
 * firmware 03.14+; the boards in this line carry 03.11.
 */
export const BNO055_INT = {
  accBsxDrdy: 0x01,
  magDrdy: 0x02,
  gyrAm: 0x04,
  gyrHighRate: 0x08,
  gyrDrdy: 0x10,
  accHighG: 0x20,
  accAm: 0x40,
  accNm: 0x80,
} as const;

/** ST_RESULT bits (1 = passed). */
export const BNO055_ST = { acc: 0x01, mag: 0x02, gyr: 0x04, mcu: 0x08 } as const;
export const BNO055_EXPECTED_SELF_TEST = 0x0f;

/** OPR_MODE (0x3D) bits 3:0. */
export enum Bno055OprMode {
  Config = 0x00,
  AccOnly = 0x01,
  MagOnly = 0x02,
  GyroOnly = 0x03,
  AccMag = 0x04,
  AccGyro = 0x05,
  MagGyro = 0x06,
  Amg = 0x07,
  Imu = 0x08,
  Compass = 0x09,
  M4g = 0x0a,
  NdofFmcOff = 0x0b,
  Ndof = 0x0c,
}

export function bno055IsFusion(mode: Bno055OprMode): boolean {
  return mode >= Bno055OprMode.Imu;
}

/** PWR_MODE (0x3E) bits 1:0. */
export enum Bno055PwrMode {
  Normal = 0x00,
  LowPower = 0x01,
  Suspend = 0x02,
}

/** TEMP_SOURCE (0x40) bits 1:0. */
export enum Bno055TempSource {
  Accel = 0x00,
  Gyro = 0x01,
}

/** Datasheet Table 3-6 plus margin: CONFIG → any 7 ms, any → CONFIG 19 ms. */
export const BNO055_MODE_SWITCH_FROM_CONFIG_MS = 10;
export const BNO055_MODE_SWITCH_TO_CONFIG_MS = 25;
/** BIST runs ~400 ms (datasheet §3.9.2). */
export const BNO055_SELF_TEST_MS = 450;
/**
 * SYS_STATUS values while the sensor is still booting after BNO_RESET —
 * the bridge answers at the chip-ID handshake, ~15 ms before the sensor's
 * boot ends and it reverts OPR_MODE (contract 13 §5, ERRATA E13).
 */
export const BNO055_SYS_STATUS_BOOTING: readonly number[] = [2, 3, 4];
export const BNO055_BOOT_SETTLE_TIMEOUT_MS = 1000;
/**
 * SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in
 * CONFIG mode, not POST: POST shows 4 for ~35 ms (a handful of polls), the
 * leftover stays until the mode leaves CONFIG (measured on SW 03.11). Counted,
 * not timed, so a replay makes the same decision.
 */
export const BNO055_STUCK_SELF_TEST_POLLS = 20;
/** Fusion outputs read zero for ~70 ms after CONFIG → a fusion mode. */
export const BNO055_FUSION_START_TIMEOUT_MS = 1000;

export const BNO055_SYS_STATUS_NAMES: Readonly<Record<number, string>> = {
  0: "idle",
  1: "system error",
  2: "initializing peripherals",
  3: "system initialization",
  4: "executing self-test",
  5: "fusion algorithm running",
  6: "running without fusion",
};

export const BNO055_SYS_ERR_NAMES: Readonly<Record<number, string>> = {
  0: "no error",
  1: "peripheral initialization error",
  2: "system initialization error",
  3: "self-test failed",
  4: "register map value out of range",
  5: "register map address out of range",
  6: "register map write error",
  7: "low power mode not available for this operation mode",
  8: "accelerometer power mode not available",
  9: "fusion algorithm configuration error",
  10: "sensor configuration error",
};

// ── units (UNIT_SEL 0x3B) ─────────────────────────────────────────────────────

// UNIT_SEL bits as the silicon implements them (Table 3-11 / Bosch's driver;
// the datasheet's §4.3.60 table is off by one — measured on SW 03.11).
const UNIT_ACC_MG = 0x01;
const UNIT_GYR_RPS = 0x02;
const UNIT_EUL_RAD = 0x04;
const UNIT_TEMP_F = 0x10;
const UNIT_ORI_ANDROID = 0x80;

/**
 * Output units. Default (all false) = m/s², dps, degrees, °C, Windows
 * orientation (UNIT_SEL 0x00). The sensor's power-on value is 0x80.
 */
export interface Bno055Units {
  /** ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²). */
  accelMg: boolean;
  gyroRps: boolean;
  eulerRad: boolean;
  tempF: boolean;
  android: boolean;
}

export const BNO055_DEFAULT_UNITS: Readonly<Bno055Units> = Object.freeze({
  accelMg: false,
  gyroRps: false,
  eulerRad: false,
  tempF: false,
  android: false,
});

export function packBno055Units(u: Bno055Units): number {
  return (
    (u.accelMg ? UNIT_ACC_MG : 0) |
    (u.gyroRps ? UNIT_GYR_RPS : 0) |
    (u.eulerRad ? UNIT_EUL_RAD : 0) |
    (u.tempF ? UNIT_TEMP_F : 0) |
    (u.android ? UNIT_ORI_ANDROID : 0)
  );
}

export function unpackBno055Units(value: number): Bno055Units {
  return {
    accelMg: (value & UNIT_ACC_MG) !== 0,
    gyroRps: (value & UNIT_GYR_RPS) !== 0,
    eulerRad: (value & UNIT_EUL_RAD) !== 0,
    tempF: (value & UNIT_TEMP_F) !== 0,
    android: (value & UNIT_ORI_ANDROID) !== 0,
  };
}

/** LSB per unit (contract 13 §4.2). */
export function bno055Lsb(u: Bno055Units): { accel: number; gyro: number; euler: number; temp: number } {
  return {
    accel: u.accelMg ? 1 : 100,
    gyro: u.gyroRps ? 900 : 16,
    euler: u.eulerRad ? 900 : 16,
    temp: u.tempF ? 0.5 : 1,
  };
}

export const BNO055_MAG_LSB = 16;
export const BNO055_QUAT_LSB = 16384;
/** LIA / GRV ignore the ACC_Unit bit: always m/s² at 100 LSB (measured). */
export const BNO055_FUSION_ACCEL_LSB = 100;

// ── calibration ──────────────────────────────────────────────────────────────

/** CALIB_STAT (0x35): 0 = not calibrated … 3 = fully calibrated. */
export interface Bno055CalibStatus {
  system: number;
  gyro: number;
  accel: number;
  mag: number;
}

export function unpackBno055CalibStatus(v: number): Bno055CalibStatus {
  return { system: (v >> 6) & 3, gyro: (v >> 4) & 3, accel: (v >> 2) & 3, mag: v & 3 };
}

export function packBno055CalibStatus(s: Bno055CalibStatus): number {
  return ((s.system & 3) << 6) | ((s.gyro & 3) << 4) | ((s.accel & 3) << 2) | (s.mag & 3);
}

export function bno055FullyCalibrated(s: Bno055CalibStatus): boolean {
  return s.system === 3 && s.gyro === 3 && s.accel === 3 && s.mag === 3;
}

type Vec3i = [number, number, number];

/**
 * Sensor offsets and radii, registers 0x55..0x6A (22 bytes, all i16 LE).
 * Read after a full calibration (CONFIG mode), store, write back after
 * every power-on reset. A written profile is a starting point: fusion
 * refines it as soon as it resumes.
 */
export interface Bno055CalibrationProfile {
  accelOffset: Vec3i;
  magOffset: Vec3i;
  gyroOffset: Vec3i;
  accelRadius: number;
  magRadius: number;
}

export function packBno055CalibrationProfile(p: Bno055CalibrationProfile): Uint8Array {
  const out = new Uint8Array(BNO055_CALIB_PROFILE_LEN);
  const dv = new DataView(out.buffer);
  [...p.accelOffset, ...p.magOffset, ...p.gyroOffset, p.accelRadius, p.magRadius].forEach((v, i) =>
    dv.setInt16(2 * i, v, true),
  );
  return out;
}

export function unpackBno055CalibrationProfile(data: Uint8Array): Bno055CalibrationProfile {
  if (data.length !== BNO055_CALIB_PROFILE_LEN) {
    throw new RangeError(`calibration profile is 22 bytes, got ${data.length}`);
  }
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const v = Array.from({ length: 11 }, (_, i) => dv.getInt16(2 * i, true));
  return {
    accelOffset: [v[0]!, v[1]!, v[2]!],
    magOffset: [v[3]!, v[4]!, v[5]!],
    gyroOffset: [v[6]!, v[7]!, v[8]!],
    accelRadius: v[9]!,
    magRadius: v[10]!,
  };
}

/** Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384. */
export const BNO055_SIC_IDENTITY: readonly number[] = [16384, 0, 0, 0, 16384, 0, 0, 0, 16384];

export function packBno055SicMatrix(m: readonly number[]): Uint8Array {
  if (m.length !== 9) throw new RangeError("SIC matrix has 9 elements");
  const out = new Uint8Array(18);
  const dv = new DataView(out.buffer);
  m.forEach((v, i) => dv.setInt16(2 * i, v, true));
  return out;
}

export function unpackBno055SicMatrix(data: Uint8Array): number[] {
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  return Array.from({ length: 9 }, (_, i) => dv.getInt16(2 * i, true));
}

// ── axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42) ──────────────────

export const BNO055_AXIS_X = 0;
export const BNO055_AXIS_Y = 1;
export const BNO055_AXIS_Z = 2;

/**
 * Which physical axis feeds each output axis, and its sign: `x = AXIS_Y`
 * means "output X is the chip's Y axis".
 */
export interface Bno055AxisRemap {
  x: number;
  y: number;
  z: number;
  xNegative: boolean;
  yNegative: boolean;
  zNegative: boolean;
}

export const BNO055_DEFAULT_AXIS_REMAP: Readonly<Bno055AxisRemap> = Object.freeze({
  x: 0,
  y: 1,
  z: 2,
  xNegative: false,
  yNegative: false,
  zNegative: false,
});

/**
 * → [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]. The sensor keeps the old mapping when
 * one axis is used twice, so this refuses a non-permutation up front.
 */
export function packBno055AxisRemap(a: Bno055AxisRemap): [number, number] {
  const sorted = [a.x, a.y, a.z].sort();
  if (sorted[0] !== 0 || sorted[1] !== 1 || sorted[2] !== 2) {
    throw new RangeError(`axis remap must be a permutation of X/Y/Z, got ${a.x}/${a.y}/${a.z}`);
  }
  const config = (a.z << 4) | (a.y << 2) | a.x;
  const sign = (a.xNegative ? 4 : 0) | (a.yNegative ? 2 : 0) | (a.zNegative ? 1 : 0);
  return [config, sign];
}

export function unpackBno055AxisRemap(config: number, sign: number): Bno055AxisRemap {
  return {
    x: config & 3,
    y: (config >> 2) & 3,
    z: (config >> 4) & 3,
    xNegative: (sign & 4) !== 0,
    yNegative: (sign & 2) !== 0,
    zNegative: (sign & 1) !== 0,
  };
}

/** Datasheet §3.4: placement → [AXIS_MAP_CONFIG, AXIS_MAP_SIGN]; P1 is the default. */
export const BNO055_PLACEMENTS: Readonly<Record<string, readonly [number, number]>> = {
  P0: [0x21, 0x04],
  P1: [0x24, 0x00],
  P2: [0x24, 0x06],
  P3: [0x21, 0x02],
  P4: [0x24, 0x03],
  P5: [0x21, 0x01],
  P6: [0x21, 0x07],
  P7: [0x24, 0x05],
};

export function bno055Placement(name: string): Bno055AxisRemap {
  const p = BNO055_PLACEMENTS[name.toUpperCase()];
  if (p === undefined) throw new RangeError(`unknown placement ${name}; expected P0..P7`);
  return unpackBno055AxisRemap(p[0], p[1]);
}

// ── page-1 sensor configuration (non-fusion modes only) ─────────────────────

export const BNO055_ACC_RANGE_G: readonly number[] = [2, 4, 8, 16];
export const BNO055_ACC_BANDWIDTH_HZ: readonly number[] = [7.81, 15.63, 31.25, 62.5, 125, 250, 500, 1000];
export const BNO055_GYR_RANGE_DPS: readonly number[] = [2000, 1000, 500, 250, 125];
export const BNO055_GYR_BANDWIDTH_HZ: readonly number[] = [523, 230, 116, 47, 23, 12, 64, 32];
export const BNO055_MAG_RATE_HZ: readonly number[] = [2, 6, 8, 10, 15, 20, 25, 30];

/** Register codes: `range`/`bandwidth` index the tables above, `power` the datasheet mode list. */
export interface Bno055AccelConfig {
  range: number;
  bandwidth: number;
  power: number;
}

export function packBno055AccelConfig(c: Bno055AccelConfig): number {
  return ((c.power & 7) << 5) | ((c.bandwidth & 7) << 2) | (c.range & 3);
}

export function unpackBno055AccelConfig(v: number): Bno055AccelConfig {
  return { range: v & 3, bandwidth: (v >> 2) & 7, power: (v >> 5) & 7 };
}

export interface Bno055GyroConfig {
  range: number;
  bandwidth: number;
  power: number;
}

export function packBno055GyroConfig(c: Bno055GyroConfig): Uint8Array {
  return Uint8Array.of(((c.bandwidth & 7) << 3) | (c.range & 7), c.power & 7);
}

export function unpackBno055GyroConfig(b: Uint8Array): Bno055GyroConfig {
  return { range: b[0]! & 7, bandwidth: (b[0]! >> 3) & 7, power: b[1]! & 7 };
}

export interface Bno055MagConfig {
  rate: number;
  mode: number;
  power: number;
}

export function packBno055MagConfig(c: Bno055MagConfig): number {
  return ((c.power & 3) << 5) | ((c.mode & 3) << 3) | (c.rate & 7);
}

export function unpackBno055MagConfig(v: number): Bno055MagConfig {
  return { rate: v & 7, mode: (v >> 3) & 3, power: (v >> 5) & 3 };
}

// ── system status ────────────────────────────────────────────────────────────

/** ST_RESULT (0x36) + SYS_CLK_STATUS/SYS_STATUS/SYS_ERR (0x38..0x3A). */
export interface Bno055SystemStatus {
  selfTest: number;
  clkStatus: number;
  status: number;
  error: number;
  statusText: string;
  errorText: string;
  selfTestPassed: boolean;
}

export function makeBno055SystemStatus(
  selfTest: number,
  clkStatus: number,
  status: number,
  error: number,
): Bno055SystemStatus {
  return {
    selfTest,
    clkStatus,
    status,
    error,
    statusText: BNO055_SYS_STATUS_NAMES[status] ?? `unknown (${status})`,
    errorText: BNO055_SYS_ERR_NAMES[error] ?? `unknown (${error})`,
    selfTestPassed: (selfTest & BNO055_EXPECTED_SELF_TEST) === BNO055_EXPECTED_SELF_TEST,
  };
}

// ── output block decode ──────────────────────────────────────────────────────

/**
 * Raw register values found in one block read. A channel is null when the
 * window `addr..addr+len` does not cover it completely.
 */
export interface Bno055RawBlock {
  accel: Vec3i | null;
  mag: Vec3i | null;
  gyro: Vec3i | null;
  /** heading, roll, pitch */
  euler: Vec3i | null;
  /** w, x, y, z */
  quaternion: [number, number, number, number] | null;
  linearAccel: Vec3i | null;
  gravity: Vec3i | null;
  temperature: number | null;
  calibStat: number | null;
}

const CHANNELS: ReadonlyArray<[keyof Bno055RawBlock, number, number]> = [
  ["accel", BNO055_REG_ACC_DATA, 3],
  ["mag", BNO055_REG_MAG_DATA, 3],
  ["gyro", BNO055_REG_GYR_DATA, 3],
  ["euler", BNO055_REG_EUL_DATA, 3],
  ["quaternion", BNO055_REG_QUA_DATA, 4],
  ["linearAccel", BNO055_REG_LIA_DATA, 3],
  ["gravity", BNO055_REG_GRV_DATA, 3],
];

/** Unpack whatever channels the register window starting at `addr` holds. */
export function decodeBno055Block(addr: number, data: Uint8Array): Bno055RawBlock {
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const end = addr + data.length;
  const out: Bno055RawBlock = {
    accel: null,
    mag: null,
    gyro: null,
    euler: null,
    quaternion: null,
    linearAccel: null,
    gravity: null,
    temperature: null,
    calibStat: null,
  };
  for (const [name, reg, n] of CHANNELS) {
    if (addr <= reg && reg + 2 * n <= end) {
      const v = Array.from({ length: n }, (_, i) => dv.getInt16(reg - addr + 2 * i, true));
      (out as unknown as Record<string, number[]>)[name] = v;
    }
  }
  if (addr <= BNO055_REG_TEMP && BNO055_REG_TEMP < end) out.temperature = dv.getInt8(BNO055_REG_TEMP - addr);
  if (addr <= BNO055_REG_CALIB_STAT && BNO055_REG_CALIB_STAT < end) {
    out.calibStat = dv.getUint8(BNO055_REG_CALIB_STAT - addr);
  }
  return out;
}
