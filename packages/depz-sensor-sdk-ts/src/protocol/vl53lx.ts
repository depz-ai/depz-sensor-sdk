/**
 * VL53L 1D-family register-bridge wire codecs, protocol v2.01
 * (contracts/12_SENSOR_VL53LX.md). Mirrors the Python reference
 * `depz_sensor_sdk.protocol.vl53lx`.
 *
 * One firmware (`APP_VL53L0_4`) serves VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX,
 * VL53L4CD and VL53L4CX. It is the VL53L4CD bridge of contract 10 with the
 * three sensor-specific facts moved to the host: the register-address width
 * (VL53_SET_ADDR_WIDTH, new), the interrupt-release writes (carried by
 * VL53_START_STREAM) and the boot handshake (no longer inside VL53_XSHUT).
 * READ_REG, WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA /
 * STREAM reports are the contract-10 codecs of `protocol/vl53l4.ts`, identical
 * on the wire — import them from there. v2.01 (firmware v0.24) adds
 * VL53_CLEAR_I2C_ERRORS; a v2.00 bridge refuses it with ERR_INVALID_CMD.
 *
 * Every export here carries a `Vl53lx`/`VL53LX_` name so the root index can
 * re-export this module with `export *` next to the other VL53 protocols.
 */

export enum Vl53lxCmd {
  ReadReg = 0x32,
  WriteReg = 0x33,
  Xshut = 0x34,
  StartStream = 0x35,
  StopStream = 0x36,
  GetInfo = 0x37,
  SetI2cSpeed = 0x38,
  SetAddrWidth = 0x39,
  ClearI2cErrors = 0x3a, // v2.01
}

export enum Vl53lxRpt {
  RegData = 0x91,
  Info = 0x92,
  Stream = 0x93,
}

/** Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX). */
export const VL53LX_CLEAR_STEPS_MAX = 4;
/** RPT_VL53_INFO payload size (v2.00). */
export const VL53LX_INFO_SIZE = 23;

/** One interrupt-release step: write `value` to register `addr`. */
export type Vl53lxClearStep = readonly [addr: number, value: number];

/**
 * vl53_start_stream_t: block, flags, then `clear` — the (addr, value) writes
 * the bridge plays after every block read (0..4 steps). 6+3n bytes.
 */
export function packVl53lxStartStream(
  addr: number,
  length: number,
  clear: ReadonlyArray<Vl53lxClearStep> = [],
  flags = 0,
): Uint8Array {
  if (clear.length > VL53LX_CLEAR_STEPS_MAX) {
    throw new RangeError(`at most ${VL53LX_CLEAR_STEPS_MAX} interrupt-release steps`);
  }
  const out = new Uint8Array(6 + 3 * clear.length);
  const dv = new DataView(out.buffer);
  dv.setUint16(0, addr, true);
  dv.setUint16(2, length, true);
  dv.setUint8(4, flags);
  dv.setUint8(5, clear.length);
  clear.forEach(([stepAddr, value], i) => {
    dv.setUint16(6 + 3 * i, stepAddr, true);
    dv.setUint8(8 + 3 * i, value);
  });
  return out;
}

/** VL53_SET_ADDR_WIDTH payload: register-address width, 1 or 2 bytes. */
export function packVl53lxSetAddrWidth(width: number): Uint8Array {
  if (width !== 1 && width !== 2) {
    throw new RangeError("register address width is 1 or 2 bytes");
  }
  return Uint8Array.of(width);
}

/**
 * RPT_VL53_INFO (v2.00, 23 bytes) — bridge state only; the bridge reads no
 * sensor register. Counters are free-running (wrap silently): watch
 * increments. `slotsSkipped` = a slot that never got the bus, `i2cErrors` =
 * a bus that answered badly (since power-up, the last XSHUT reset or
 * VL53_CLEAR_I2C_ERRORS — the SDK sends that at the end of every sensor init,
 * so the NACKs of a resetting die are not counted), `framesDropped` = a good
 * sample the USB TX ring had no room for (since the stream was armed).
 */
export interface Vl53lxInfo {
  intEdges: number;
  slotsSkipped: number;
  i2cErrors: number;
  /** 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR. */
  lastI2cError: number;
  xshutLevel: number;
  intLevel: number;
  i2cKhz: number;
  addrWidth: number;
  nClear: number;
  framesDropped: number;
}

/** `<IIIBBBHBBI`; rejects a payload shorter than 23 bytes. */
export function unpackVl53lxInfo(payload: Uint8Array): Vl53lxInfo {
  if (payload.length < VL53LX_INFO_SIZE) {
    throw new RangeError(
      `RPT_VL53_INFO: expected ${VL53LX_INFO_SIZE} bytes, got ${payload.length}`,
    );
  }
  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    intEdges: dv.getUint32(0, true),
    slotsSkipped: dv.getUint32(4, true),
    i2cErrors: dv.getUint32(8, true),
    lastI2cError: dv.getUint8(12),
    xshutLevel: dv.getUint8(13),
    intLevel: dv.getUint8(14),
    i2cKhz: dv.getUint16(15, true),
    addrWidth: dv.getUint8(17),
    nClear: dv.getUint8(18),
    framesDropped: dv.getUint32(19, true),
  };
}
