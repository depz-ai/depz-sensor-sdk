/**
 * TypeDoc entry point for the per-sensor `docs/vl53l5cx/api.md`.
 *
 * Only the `Vl53l5cx` class: it is `Vl53l7cx` with a different expected
 * module type, so the board commands and codecs stay in
 * `docs/vl53l7cx/api.md` and the shared ToF surface in `docs/vl53l8cx/api.md`.
 *
 * Not part of the shipped package — a doc-generation entry point only.
 */

export { Vl53l5cx } from "../src/sensors/vl53l7/vl53l7.js";
