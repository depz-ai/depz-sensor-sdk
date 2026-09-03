/**
 * TypeDoc entry point for the per-sensor `docs/vl53l8cx/api.md`.
 *
 * The base VL53L8**CX** time-of-flight surface: the `Vl53l8Cx` class (and its
 * `Vl53l8` alias), the frame type, and the ST ULD constants/helpers that drive
 * it. The CH superset (`Vl53l8Ch`) and the Compact-Network-Histogram symbols
 * are documented separately in `docs/vl53l8ch/api.md` (via `entry-vl53l8ch.ts`)
 * — keep them out of here so the CX reference has no CH bleed.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export {
  MIN_RANGING_FREQUENCY_HZ,
  Vl53l8,
  Vl53l8Cx,
  zoneGrid,
} from "../src/sensors/vl53l8/vl53l8.js";
export type {
  Vl53l8Frame,
  Vl53l8InitOptions,
  Vl53l8Options,
} from "../src/sensors/vl53l8/vl53l8.js";
export {
  CALIBRATE_XTALK,
  FW_CHECKSUM,
  GET_XTALK_CMD,
  MotionConfig,
  NB_THRESHOLDS,
  POWER_MODE_DEEP_SLEEP,
  POWER_MODE_SLEEP,
  POWER_MODE_WAKEUP,
  RANGING_MODE_AUTONOMOUS,
  RANGING_MODE_CONTINUOUS,
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  TARGET_ORDER_CLOSEST,
  TARGET_ORDER_STRONGEST,
  THRESH_IN_WINDOW,
  THRESH_OP_AND,
  THRESH_OP_NONE,
  THRESH_OP_OR,
  THRESH_OUT_OF_WINDOW,
  VL53L8CX,
  Vl53l8cxError,
  defaultMotionConfig,
  motionConfigSetResolution,
  packDetectionThresholds,
  swapBuffer,
  xtalkMarginToRaw,
} from "../src/sensors/vl53l8/uld.js";
export type {
  DetectionThreshold,
  MotionResult,
  Vl53l8Platform,
  Vl53l8Results,
} from "../src/sensors/vl53l8/uld.js";
export { loadAssets } from "../src/sensors/vl53l8/assets/index.js";
export type { Vl53l8Assets, Vl53l8Variant } from "../src/sensors/vl53l8/assets/index.js";
