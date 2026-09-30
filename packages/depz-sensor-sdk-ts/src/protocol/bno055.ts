/**
 * BNO055 register-bridge wire codecs (contracts/13_SENSOR_BNO055.md).
 * Mirrors the Python reference `depz_sensor_sdk.protocol.bno055`.
 */

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

export function packBno055ReadReg(addr: number, length: number): Uint8Array {
  return Uint8Array.of(addr & 0xff, length & 0xff);
}

export function packBno055WriteReg(addr: number, data: Uint8Array): Uint8Array {
  const out = new Uint8Array(1 + data.length);
  out[0] = addr & 0xff;
  out.set(data, 1);
  return out;
}

export function packBno055StartStream(
  trigger: number,
  addr: number,
  length: number,
  periodMs: number,
): Uint8Array {
  const out = new Uint8Array(5);
  const dv = new DataView(out.buffer);
  dv.setUint8(0, trigger);
  dv.setUint8(1, addr);
  dv.setUint8(2, length);
  dv.setUint16(3, periodMs, true);
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
 * diagnostics. Counters are free-running and wrap silently. A rising
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
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const length = dv.getUint8(9);
  return {
    timestampUs: dv.getBigUint64(0, true),
    addr: dv.getUint8(8),
    length,
    data: payload.slice(10, 10 + length),
  };
}
