/**
 * TypeDoc entry point for the per-sensor `docs/vl53l4cx/api.md`.
 *
 * The `Vl53l4cx` class (the family class `Vl53lx` with the product fixed to
 * VL53L4CX), followed by the 1D-family surface shared by every product page
 * (`vl53lx-family.ts`).
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Vl53l4cx } from "../src/sensors/vl53lx/vl53lx.js";
export * from "./vl53lx-family.js";
