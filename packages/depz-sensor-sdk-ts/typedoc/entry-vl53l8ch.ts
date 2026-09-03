/**
 * TypeDoc entry point for the per-sensor `docs/vl53l8ch/api.md`.
 *
 * CH-**only** surface: the `Vl53l8Ch` class (the CX superset that adds
 * `configureCnh()`) plus the Compact-Network-Histogram (CNH) symbols. Every
 * base ToF method CH inherits is documented in `docs/vl53l8cx/api.md` (via
 * `entry-vl53l8cx.ts`); this entry point deliberately omits the CX base so the
 * CH reference carries only what CH adds.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Vl53l8Ch } from "../src/sensors/vl53l8/vl53l8.js";
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
