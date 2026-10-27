/**
 * BNO055 register-bridge wire codecs (contracts/13_SENSOR_BNO055.md).
 * Mirrors the Python reference `depz_sensor_sdk.protocol.bno055`.
 */

import { checkUint, needBytes } from "./wire.js";

export enum Bno055Cmd {
  ReadReg = 0x32,
  WriteReg = 0x33,
  Reset = 0x34,
  StartStream = 0x35,
  StopStream = 0x36,
  GetInfo = 0x37,
}

export enum Bno055Rpt {
  RegData = 0x91,
  Info = 0x92,
  Stream = 0x93,
}

/** Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100. */
export const BNO055_XFER_MAX = 128;

/** BNO_START_STREAM trigger: read every period_ms (the only data trigger on SW 03.11). */
export const BNO055_TRIGGER_TIMER = 0;
/** BNO_START_STREAM trigger: read on the INT edge; period_ms is a missed-edge watchdog. */
export const BNO055_TRIGGER_INT = 1;

/** BNO_RESET answers after the sensor's ~0.5 s boot handshake. */
export const BNO055_RESET_TIMEOUT_MS = 3000;

const REG_SPACE = 0x100; // 8-bit register addresses: a transfer never runs past 0xFF

/** BNO_START_STREAM TIMER period limits, ms (contract 13 §2). */
export const BNO055_TIMER_PERIOD_MIN_MS = 1;
export const BNO055_TIMER_PERIOD_MAX_MS = 60000;

/**
 * BNO_READ_REG payload `addr u8, len u8` with the transfer limit of
 * contract 13 §1: `len` 1..128 and `addr + len <= 0x100`, else RangeError.
 */
export function packBno055ReadReg(addr: number, length: number): Uint8Array {
  const out = Uint8Array.of(checkUint("addr", addr, 8), checkUint("length", length, 8));
  if (length < 1 || length > BNO055_XFER_MAX || addr + length > REG_SPACE) {
    throw new RangeError(`READ_REG length must be 1..${BNO055_XFER_MAX} and addr+length <= 0x100`);
  }
  return out;
}

/** BNO_WRITE_REG data length limit (contract 13 §2: 1..128 bytes). */
export const BNO055_WRITE_MAX = BNO055_XFER_MAX;

/** BNO_WRITE_REG payload `addr u8, data[N]`: N 1..128 and `addr + N <= 0x100`, else RangeError. */
export function packBno055WriteReg(addr: number, data: Uint8Array): Uint8Array {
  checkUint("addr", addr, 8);
  if (data.length < 1 || data.length > BNO055_WRITE_MAX || addr + data.length > REG_SPACE) {
    throw new RangeError(`WRITE_REG data must be 1..${BNO055_WRITE_MAX} bytes and addr+N <= 0x100`);
  }
  const out = new Uint8Array(1 + data.length);
  out[0] = addr;
  out.set(data, 1);
  return out;
}

/**
 * BNO_START_STREAM payload `trigger u8, addr u8, len u8, period_ms u16`. With
 * the TIMER trigger the period is 1..60000 ms (contract 13 §2), else
 * RangeError; with INT it is the missed-edge watchdog and 0 disables it.
 */
export function packBno055StartStream(
  trigger: number,
  addr: number,
  length: number,
  periodMs: number,
): Uint8Array {
  const out = new Uint8Array(5);
  const dv = new DataView(out.buffer);
  dv.setUint8(0, checkUint("trigger", trigger, 8));
  dv.setUint8(1, checkUint("addr", addr, 8));
  dv.setUint8(2, checkUint("length", length, 8));
  dv.setUint16(3, checkUint("periodMs", periodMs, 16), true);
  if (
    trigger === BNO055_TRIGGER_TIMER &&
    (periodMs < BNO055_TIMER_PERIOD_MIN_MS || periodMs > BNO055_TIMER_PERIOD_MAX_MS)
  ) {
    throw new RangeError(
      `periodMs: ${periodMs} outside ${BNO055_TIMER_PERIOD_MIN_MS}..${BNO055_TIMER_PERIOD_MAX_MS} for the TIMER trigger`,
    );
  }
  return out;
}

/** RPT_BNO_REG_DATA payload. */
export interface Bno055RegData {
  /** Echoed READ_REG opcode. */
  cmd: number;
  /** MCU uptime at I2C-read completion. */
  timestampUs: bigint;
  data: Uint8Array;
}

export function unpackBno055RegData(payload: Uint8Array): Bno055RegData {
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return { cmd: dv.getUint8(0), timestampUs: dv.getBigUint64(1, true), data: payload.slice(9) };
}

/** last_i2c_error values in RPT_BNO_INFO. */
export const BNO055_I2C_ERROR_NAMES: Readonly<Record<number, string>> = {
  0: "none",
  1: "NACK",
  2: "TIMEOUT",
  3: "BUS_ERROR",
};

/** Identity registers 0x00..0x03 of a healthy BNO055. */
export const BNO055_EXPECTED_IDS = { chipId: 0xa0, accId: 0xfb, magId: 0x32, gyrId: 0x0f } as const;

/**
 * RPT_BNO_INFO — sensor identity (registers 0x00..0x06) plus bridge
 * diagnostics. Counters wrap silently; `i2cErrors` is cleared by every
 * successful reset handshake, the read timings and `slotsSkipped` at
 * START_STREAM. A rising
 * `sensorResets` means the bridge pulsed nRESET to recover the bus: the
 * sensor is back in CONFIG mode and the host must restore its configuration.
 */
export interface Bno055Info {
  /** 7-bit sensor address in use (0x28). */
  i2cAddr: number;
  chipId: number;
  accId: number;
  magId: number;
  gyrId: number;
  /** Sensor firmware, BCD: 0x0311 = 03.11. */
  swRev: number;
  blRev: number;
  /** 1 = chip-ID handshake passed. */
  initialized: number;
  intLevel: number;
  /** EXTI rising edges (counted only while an INT stream is armed). */
  intEdges: number;
  /** Streamed block read timing since the last START_STREAM. */
  readMinUs: number;
  readMaxUs: number;
  readAvgUs: number;
  /** Packets refused by a full USB TX ring. */
  txDropped: number;
  /** Failed transfers since the last successful reset handshake. */
  i2cErrors: number;
  /** Stream slots dropped: bus still busy. */
  slotsSkipped: number;
  busRecoveries: number;
  lastI2cError: number;
  /** Rung-3 nRESET recoveries (sensor back in CONFIG). */
  sensorResets: number;
  loopMaxUs: number;
}

export const BNO055_INFO_SIZE = 38;

export function unpackBno055Info(payload: Uint8Array): Bno055Info {
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    i2cAddr: dv.getUint8(0),
    chipId: dv.getUint8(1),
    accId: dv.getUint8(2),
    magId: dv.getUint8(3),
    gyrId: dv.getUint8(4),
    swRev: dv.getUint16(5, true),
    blRev: dv.getUint8(7),
    initialized: dv.getUint8(8),
    intLevel: dv.getUint8(9),
    intEdges: dv.getUint32(10, true),
    readMinUs: dv.getUint16(14, true),
    readMaxUs: dv.getUint16(16, true),
    readAvgUs: dv.getUint16(18, true),
    txDropped: dv.getUint32(20, true),
    i2cErrors: dv.getUint32(24, true),
    slotsSkipped: dv.getUint32(28, true),
    busRecoveries: dv.getUint16(32, true),
    lastI2cError: dv.getUint8(34),
    sensorResets: dv.getUint8(35),
    loopMaxUs: dv.getUint16(36, true),
  };
}

/** True when the four identity registers carry the BNO055 values. */
export function bno055IdsOk(info: Bno055Info): boolean {
  const e = BNO055_EXPECTED_IDS;
  return (
    info.chipId === e.chipId && info.accId === e.accId && info.magId === e.magId && info.gyrId === e.gyrId
  );
}

/** Sensor firmware revision as Bosch writes it: 0x0311 → "03.11". */
export function bno055SwRevText(swRev: number): string {
  const hex = (b: number) => b.toString(16).toUpperCase().padStart(2, "0");
  return `${hex(swRev >> 8)}.${hex(swRev & 0xff)}`;
}

/**
 * RPT_BNO_REG_STREAM — one streamed register block. `addr`/`length` echo the
 * stream configuration so each report is self-describing.
 */
export interface Bno055StreamData {
  /** MCU uptime at the trigger (timer expiry or INT edge). */
  timestampUs: bigint;
  addr: number;
  length: number;
  data: Uint8Array;
}

export function unpackBno055Stream(payload: Uint8Array): Bno055StreamData {
  needBytes("RPT_BNO_REG_STREAM", payload, 10);
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const length = dv.getUint8(9);
  // contract 07 §3: checked against its own length field; trailing bytes ignored
  needBytes("RPT_BNO_REG_STREAM data", payload, 10 + length);
  return {
    timestampUs: dv.getBigUint64(0, true),
    addr: dv.getUint8(8),
    length,
    data: payload.slice(10, 10 + length),
  };
}
