/**
 * VL53L4 register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md).
 * Mirrors the Python reference `depz_sensor_sdk.protocol.vl53l4`.
 */

import { checkUint, needBytes } from "./wire.js";

export enum Vl53l4Cmd {
  ReadReg = 0x32,
  WriteReg = 0x33,
  Xshut = 0x34,
  StartStream = 0x35,
  StopStream = 0x36,
  GetInfo = 0x37,
  SetI2cSpeed = 0x38,
}

export enum Vl53l4Rpt {
  RegData = 0x91,
  Info = 0x92,
  Stream = 0x93,
}

// STM32 I2C NBYTES is 8 bit and a write spends two of them on the register
// address; the firmware applies the same 253 to both directions.
export const XFER_MAX = 253;

// VL53_XSHUT actions.
export const XSHUT_OFF = 0;
export const XSHUT_ON = 1;
/** Blocking on the MCU (~3 ms); answered after the boot handshake. */
export const XSHUT_RESET = 2;

/**
 * VL53_START_STREAM flags: interrupt polarity, mirroring bit 4 of
 * GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.
 */
export const SF_INT_ACT_HIGH = 0x02;

/**
 * Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
 * clamps to the nearest one).
 */
export const I2C_KHZ_STEPS: readonly number[] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];

export function packReadReg(addr: number, length: number): Uint8Array {
  const out = new Uint8Array(4);
  const dv = new DataView(out.buffer);
  dv.setUint16(0, checkUint("addr", addr, 16), true);
  dv.setUint16(2, checkUint("length", length, 16), true);
  return out;
}

/** VL53_WRITE_REG data length limit (contracts 10/12 §2: 1..253 bytes). */
export const WRITE_MAX_LEN = 253;

export function packWriteReg(addr: number, data: Uint8Array): Uint8Array {
  checkUint("addr", addr, 16);
  if (data.length < 1 || data.length > WRITE_MAX_LEN) {
    throw new RangeError(`WRITE_REG data must be 1..${WRITE_MAX_LEN} bytes`);
  }
  const out = new Uint8Array(2 + data.length);
  new DataView(out.buffer).setUint16(0, addr, true);
  out.set(data, 2);
  return out;
}

export function packXshut(action: number): Uint8Array {
  return Uint8Array.of(checkUint("action", action, 8));
}

export function packStartStream(addr: number, length: number, flags = 0): Uint8Array {
  const out = new Uint8Array(5);
  const dv = new DataView(out.buffer);
  dv.setUint16(0, checkUint("addr", addr, 16), true);
  dv.setUint16(2, checkUint("length", length, 16), true);
  dv.setUint8(4, checkUint("flags", flags, 8));
  return out;
}

export function packSetI2cSpeed(khz: number): Uint8Array {
  const out = new Uint8Array(2);
  new DataView(out.buffer).setUint16(0, checkUint("khz", khz, 16), true);
  return out;
}

/** RPT_VL53_REG_DATA payload. */
export interface Vl53l4RegData {
  /** Echoed READ_REG opcode. */
  cmd: number;
  /** MCU uptime at I2C-read completion. */
  timestampUs: bigint;
  data: Uint8Array;
}

export function unpackRegData(payload: Uint8Array): Vl53l4RegData {
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    cmd: dv.getUint8(0),
    timestampUs: dv.getBigUint64(1, true),
    data: payload.slice(9),
  };
}

/** last_i2c_error values in RPT_VL53_INFO. */
export const I2C_ERROR_NAMES: Readonly<Record<number, string>> = {
  0: "none",
  1: "NACK",
  2: "TIMEOUT",
  3: "BUS_ERROR",
};

/**
 * RPT_VL53_INFO — bridge diagnostics. Counters are free-running and wrap
 * silently; watch increments, not absolute values.
 */
export interface Vl53l4Info {
  intEdges: number;
  slotsSkipped: number;
  i2cErrors: number;
  lastI2cError: number;
  /** 0x010F..0x0110 — expected 0xEBAA. */
  modelId: number;
  /** 0x00E5 — expected 0x03 (booted). */
  fwStatus: number;
  /** 1 = MODEL_ID matched on this read. */
  initialized: number;
  xshutLevel: number;
  intLevel: number;
  i2cKhz: number;
}

export function unpackInfo(payload: Uint8Array): Vl53l4Info {
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    intEdges: dv.getUint32(0, true),
    slotsSkipped: dv.getUint32(4, true),
    i2cErrors: dv.getUint32(8, true),
    lastI2cError: dv.getUint8(12),
    modelId: dv.getUint16(13, true),
    fwStatus: dv.getUint8(15),
    initialized: dv.getUint8(16),
    xshutLevel: dv.getUint8(17),
    intLevel: dv.getUint8(18),
    i2cKhz: dv.getUint16(19, true),
  };
}

/**
 * RPT_VL53_STREAM — one streamed register block. `addr`/`length` echo the
 * stream configuration so each report is self-describing.
 */
export interface Vl53l4StreamData {
  /** MCU uptime at the INT edge (the sensor event). */
  timestampUs: bigint;
  addr: number;
  length: number;
  data: Uint8Array;
}

export function unpackStream(payload: Uint8Array): Vl53l4StreamData {
  needBytes("RPT_VL53_STREAM", payload, 12);
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const length = dv.getUint16(10, true);
  // contract 07 §3: checked against its own length field; trailing bytes ignored
  needBytes("RPT_VL53_STREAM data", payload, 12 + length);
  return {
    timestampUs: dv.getBigUint64(0, true),
    addr: dv.getUint16(8, true),
    length,
    data: payload.slice(12, 12 + length),
  };
}
