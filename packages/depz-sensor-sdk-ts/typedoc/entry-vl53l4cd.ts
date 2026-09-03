/**
 * TypeDoc entry point for the per-sensor `docs/vl53l4cd/api.md`.
 *
 * The VL53L4CD single-zone ToF surface: the `Vl53l4Cd` device class, the
 * host-side ULD driver (`VL53L4CD` + codecs from `sensors/vl53l4/uld`), and
 * the register-bridge wire codecs (`protocol/vl53l4`, re-exported under the
 * same `Vl53l4*`-prefixed names `src/index.ts` uses) — so TypeDoc documents
 * that sensor in isolation.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export {
  I2C_ERROR_NAMES as VL53L4_I2C_ERROR_NAMES,
  I2C_KHZ_STEPS as VL53L4_I2C_KHZ_STEPS,
  SF_INT_ACT_HIGH as VL53L4_SF_INT_ACT_HIGH,
  Vl53l4Cmd,
  Vl53l4Rpt,
  XFER_MAX as VL53L4_XFER_MAX,
  XSHUT_OFF as VL53L4_XSHUT_OFF,
  XSHUT_ON as VL53L4_XSHUT_ON,
  XSHUT_RESET as VL53L4_XSHUT_RESET,
  packReadReg as packVl53l4ReadReg,
  packSetI2cSpeed as packVl53l4SetI2cSpeed,
  packStartStream as packVl53l4StartStream,
  packWriteReg as packVl53l4WriteReg,
  packXshut as packVl53l4Xshut,
  unpackInfo as unpackVl53l4Info,
  unpackRegData as unpackVl53l4RegData,
  unpackStream as unpackVl53l4Stream,
} from "../src/protocol/vl53l4.js";
export type { Vl53l4Info, Vl53l4RegData, Vl53l4StreamData } from "../src/protocol/vl53l4.js";
export * from "../src/sensors/vl53l4/uld.js";
export * from "../src/sensors/vl53l4/vl53l4.js";
