/**
 * TypeDoc entry point for the per-sensor `docs/sr04/api.md`.
 *
 * Re-exports exactly the SR04 public surface (the same symbols `src/index.ts`
 * exposes from `sensors/sr04`) so TypeDoc documents that sensor in isolation.
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Sr04 } from "../src/sensors/sr04.js";
export type { Sr04Measurement } from "../src/sensors/sr04.js";
