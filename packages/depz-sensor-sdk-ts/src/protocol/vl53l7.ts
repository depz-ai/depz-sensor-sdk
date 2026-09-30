/**
 * VL53L5CX/L7CX/L7CH I2C register-bridge wire codecs
 * (contracts/11_SENSOR_VL53L7.md). Mirrors the Python reference
 * `depz_sensor_sdk.protocol.vl53l7`.
 *
 * Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are bit-for-bit the
 * VL53L8 bridge (contract 04) and are reused from `./vl53l8.js`; this module
 * holds only what the I2C board adds, plus its tighter transfer limits.
 */

export enum Vl53l7Cmd {
  PinCtrl = 0x34,
  GetInfo = 0x37,
  SetI2cSpeed = 0x38,
}

export enum Vl53l7Rpt {
  Vl53Info = 0x92,
}

/**
 * VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
 * GPIO): after LpnOff or SoftCycle the host must re-run init().
 */
export enum PinAction {
  /** Stop streaming, drive LPn low: sensor I2C interface off. */
  LpnOff = 0,
  /** Drive LPn high: interface on (power-up default). */
  LpnOn = 1,
  /** Pulse I2C_RST. */
  I2cRst = 2,
  /** Stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears I2C counters. */
  SoftCycle = 3,
}

/** RPT_VL53_INFO.lastI2cError. */
export enum Vl53l7I2cError {
  Ok = 0,
  Nack = 1,
  Timeout = 2,
  BusError = 3,
}

/** VL53LMZ_READ_MAX: READ_REG len 1..1536. */
export const VL53L7_READ_MAX_LEN = 1536;
/** VL53LMZ_XFER_MAX: WRITE_REG N 1..2048. */
export const VL53L7_WRITE_MAX_LEN = 2048;
/** Bytes of frame data per RPT_VL53_FRAME chunk. */
export const VL53L7_STREAM_CHUNK_MAX = 1536;
export const VL53L7_INFO_SIZE = 20;
/** Nominal SCL steps the firmware carries a timing for; others snap to nearest. */
export const VL53L7_I2C_SPEED_STEPS_KHZ: readonly number[] = [
  100, 200, 400, 500, 600, 700, 800, 900, 1000,
];

export function packPinCtrl(action: number): Uint8Array {
  return Uint8Array.of(action & 0xff);
}

export function packVl53l7SetI2cSpeed(khz: number): Uint8Array {
  const out = new Uint8Array(2);
  new DataView(out.buffer).setUint16(0, khz, true);
  return out;
}

/**
 * RPT_VL53_INFO: bridge state only (the sensor is never probed). Counters run
 * from power-up / DEVICE_RESET; SoftCycle clears the I2C ones. The report
 * carries no echoed command byte.
 */
export interface Vl53l7Info {
  intEdges: number;
  framesDropped: number;
  i2cErrors: number;
  lastI2cError: number;
  lpnLevel: number;
  intLevel: number;
  i2cKhz: number;
  frameSize: number;
  streaming: boolean;
}

export function unpackVl53l7Info(payload: Uint8Array): Vl53l7Info {
  if (payload.length < VL53L7_INFO_SIZE) {
    throw new Error(
      `RPT_VL53_INFO: expected ${VL53L7_INFO_SIZE} bytes, got ${payload.length}`,
    );
  }
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    intEdges: dv.getUint32(0, true),
    framesDropped: dv.getUint32(4, true),
    i2cErrors: dv.getUint32(8, true),
    lastI2cError: dv.getUint8(12),
    lpnLevel: dv.getUint8(13),
    intLevel: dv.getUint8(14),
    i2cKhz: dv.getUint16(15, true),
    frameSize: dv.getUint16(17, true),
    streaming: dv.getUint8(19) !== 0,
  };
}

/** A sensor class an APP_VL53L7 board opens as. */
export type Vl53l7Model = "vl53l5cx" | "vl53l7cx" | "vl53l7ch";

const PART_RE = /VL53L([57])(CX|CH)/;

/**
 * Pick the L5/L7 class (contract 11 §1). All three boards run APP_VL53L7 and
 * the silicon only tells L5 from L7 after init() (module_type), never CX from
 * CH — so the production USB PID model decides, then the device name the
 * bootloader was stamped with (`… VL53L7CH USB v2.1 …`), then the CX base
 * (safe: its blob runs on every L5/L7 part).
 */
export function resolveVl53l7Model(
  usbModel: string | null | undefined,
  deviceName: string | null | undefined,
): Vl53l7Model {
  if (usbModel === "vl53l5cx" || usbModel === "vl53l7cx" || usbModel === "vl53l7ch") {
    return usbModel;
  }
  const m = PART_RE.exec(deviceName ?? "");
  if (m) {
    const part = `vl53l${m[1]}${m[2]!.toLowerCase()}`;
    if (part === "vl53l5cx" || part === "vl53l7cx" || part === "vl53l7ch") return part;
  }
  return "vl53l7cx";
}
