/**
 * What the ULD ports expect from the firmware repo's `vl53_link` — mirror of
 * the Python `depz_sensor_sdk.vl53lx._link` (contract 12).
 *
 * The ports import the two error types, the boot bus speed and the XSHUT
 * action codes; the device surface itself is `BridgeDevice`, which the
 * `Vl53lx` device class implements over the register bridge.
 */

import { DepzError } from "../../../errors.js";

/** The only bus speed an unconfigured sensor is specified for. */
export const I2C_KHZ_BOOT = 400;

export const VL53_XSHUT_OFF = 0;
export const VL53_XSHUT_ON = 1;
export const VL53_XSHUT_RESET = 2;

/**
 * A ULD-level failure: the sensor did not do what the driver needed (timeout
 * waiting for data-ready or boot, a calibration that failed...).
 */
export class Vl53Error extends DepzError {}

/**
 * The bridge refused a register command or did not answer it — the firmware
 * repo's single error for both. The ports catch it where the sensor
 * legitimately NACKs for a while (e.g. right after a soft reset).
 */
export class ProtocolError extends DepzError {}

/**
 * The `Vl53Device` surface the ports use. Register values are passed through
 * untouched; the implementation splits transfers at the bridge's 253-byte
 * limit and turns a non-OK status or a missing answer into `ProtocolError`.
 */
export interface BridgeDevice {
  readReg(addr: number, length: number): Promise<Uint8Array>;
  writeReg(addr: number, data: Uint8Array): Promise<void>;
  setI2cSpeed(khz: number): Promise<void>;
  setAddrWidth(width: number): Promise<void>;
  /** Zero the bridge's I2C error counter (protocol v2.01, firmware v0.24+). */
  clearI2cErrors(): Promise<void>;
  xshut(action: number): Promise<void>;
}
