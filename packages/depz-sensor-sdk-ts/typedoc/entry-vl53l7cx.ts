/**
 * TypeDoc entry point for the per-sensor `docs/vl53l7cx/api.md`.
 *
 * What the I2C L5/L7 board adds over the VL53L8: the `Vl53l7cx` class (its
 * inherited `Vl53l8cx` surface is documented in `docs/vl53l8cx/api.md`), the
 * board's wire codecs (`protocol/vl53l7`) and the module-type constants that
 * tell L5 from L7 after `init()`. The subclasses `Vl53l5cx` and `Vl53l7ch`
 * have their own entry points.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { VL53L7_MIN_RANGING_FREQUENCY_HZ, Vl53l7cx } from "../src/sensors/vl53l7/vl53l7.js";
export type { Vl53l7Frame } from "../src/sensors/vl53l7/vl53l7.js";
export * from "../src/protocol/vl53l7.js";
export {
  MODULE_TYPE_MZ,
  MODULE_TYPE_MZEVO,
  MODULE_TYPE_MZPLUS,
  MODULE_TYPE_NAMES,
} from "../src/sensors/vl53l8/uld.js";
