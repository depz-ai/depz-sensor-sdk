/**
 * TypeDoc entry point for the per-sensor `docs/bno086/api.md`.
 *
 * Re-exports the whole BNO086 subtree (SHTP + SH-2 + reports + the `Bno086`
 * device) — the same surface `src/index.ts` exposes from `sensors/bno086` — so
 * TypeDoc documents that sensor in isolation.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export * from "../src/sensors/bno086/shtp.js";
export * from "../src/sensors/bno086/reports.js";
export * from "../src/sensors/bno086/sh2.js";
export * from "../src/sensors/bno086/bno086.js";
