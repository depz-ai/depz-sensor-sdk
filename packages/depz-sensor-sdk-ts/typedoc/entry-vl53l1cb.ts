/**
 * TypeDoc entry point for the per-sensor `docs/vl53l1cb/api.md`.
 *
 * The `Vl53l1cb` class (the family class `Vl53lx` with the product fixed to
 * VL53L1CB), followed by the 1D-family surface shared by every product page
 * (`vl53lx-family.ts`).
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Vl53l1cb } from "../src/sensors/vl53lx/vl53lx.js";
export * from "./vl53lx-family.js";
