/**
 * TypeDoc entry point for the per-sensor `docs/tmf8829/api.md`.
 *
 * The whole TMF8829 surface `src/index.ts` exposes: the `Tmf8829` device and
 * its measurement / config types, the frame parser and half-frame merge
 * (`sensors/tmf8829/frames`), the clock model and capture time
 * (`sensors/tmf8829/clock`), presets and config-page fields
 * (`sensors/tmf8829/config`), the measurement builder, the embedded sensor
 * firmware's license, and the register-bridge wire codecs (`protocol/tmf8829`).
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export * from "../src/sensors/tmf8829/tmf8829.js";
export * from "../src/sensors/tmf8829/builder.js";
export * from "../src/sensors/tmf8829/frames.js";
export * from "../src/sensors/tmf8829/clock.js";
export * from "../src/sensors/tmf8829/config.js";
export * from "../src/sensors/tmf8829/firmware-license.js";
export * from "../src/protocol/tmf8829.js";
