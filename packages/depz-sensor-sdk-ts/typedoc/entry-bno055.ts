/**
 * TypeDoc entry point for the per-sensor `docs/bno055/api.md`.
 *
 * The whole BNO055 surface `src/index.ts` exposes: the `Bno055` device and
 * its sample type, the register map and codecs (`sensors/bno055/regs`), and
 * the register-bridge wire codecs (`protocol/bno055`).
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export * from "../src/sensors/bno055/bno055.js";
export * from "../src/sensors/bno055/regs.js";
export * from "../src/protocol/bno055.js";
