/**
 * TypeDoc entry point for the per-sensor `docs/vl53l7ch/api.md`.
 *
 * The `Vl53l7ch` class (the `Vl53l7cx` superset that adds `configureCnh()`)
 * plus the Compact-Network-Histogram symbols it is configured and decoded
 * with — the same CNH surface as `docs/vl53l8ch/api.md`.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Vl53l7ch } from "../src/sensors/vl53l7/vl53l7.js";
export {
  CNH_BIN_WIDTH_MM,
  CNH_MAX_DATA_BYTES,
  CnhConfig,
  CnhConfigError,
  MI_CFG_DEV_IDX,
  decode as decodeCnh,
  maxBins as cnhMaxBins,
} from "../src/sensors/vl53l8/cnh.js";
export type { CnhAggregate, CnhDecoded } from "../src/sensors/vl53l8/cnh.js";
