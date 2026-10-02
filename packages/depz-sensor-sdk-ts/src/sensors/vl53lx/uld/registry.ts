/**
 * uld/registry — the product table: one row per part number, holding what is
 * true of the part itself and which driver serves which way of driving it.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.registry`.
 *
 * Two independent axes:
 *  - **product** — whose parameter set to load (normally what the board's
 *    device name carries; naming a neighbour borrows its driver);
 *  - **driver kind** — one of `DRIVER_KINDS`.
 *
 * `driverFor(product, kind)` resolves the pair; a pair that does not exist is
 * a refusal, never a fallback. The model id is a cross-check (`modelIdOk`),
 * never a selector: L1CX/L1CB and L4CD/L4CX share theirs.
 */

import type { BridgePlatform, SensorDriver } from "./base.js";
import { VL53L0X } from "./l0x.js";
import { VL53L1 } from "./l1.js";
import { VL53L3 } from "./l3.js";
import { VL53L4 } from "./l4.js";
import { VL53LX } from "./bare/driver.js";

/** A concrete driver class: constructible, with the static facts of SensorDriver. */
export interface DriverClass {
  new (platform: BridgePlatform, product: string): SensorDriver;
  readonly name: string;
  readonly ADDR_WIDTH: number;
  readonly CLEAR_STEPS: ReadonlyArray<readonly [number, number]>;
  readonly MAX_KHZ: number;
  readonly SUPPORTS: ReadonlySet<string>;
  readonly MODES: readonly string[];
  readonly BUDGET_MS: readonly [number, number];
  readonly HISTOGRAM: boolean;
}

/**
 * The three kinds of driver ST ships for this family, in UI order:
 * `uld` (Ultra Lite — the die computes the distance), `ulp` (Ultra Low Power,
 * VL53L3CX only), `histogram` (the Bare Driver — 24 raw bins, the host finds
 * up to four targets).
 */
export const DRIVER_KINDS = ["uld", "ulp", "histogram"] as const;
export type DriverKind = (typeof DRIVER_KINDS)[number];

/** Every product of the 1D family, in UI order. */
export const PRODUCTS = [
  "VL53L0X",
  "VL53L1CX",
  "VL53L1CB",
  "VL53L3CX",
  "VL53L4CD",
  "VL53L4CX",
] as const;

/** One row of the table: the facts that belong to the part number. */
export interface Product {
  modelId: number;
  /** Datasheet rating of the module, mm. */
  reachMm: number;
  drivers: Partial<Record<string, DriverClass>>;
  caveats: Partial<Record<string, string>>;
}

const HIST_CAVEAT =
  "the histogram driver has no calibrations and no detection thresholds - " +
  "the light drivers are the ones with those";

/** Which pairs exist — every one measured on hardware. */
export const TABLE: Readonly<Record<string, Product>> = {
  VL53L0X: {
    modelId: 0x00ee,
    reachMm: 2000,
    drivers: { uld: VL53L0X },
    caveats: { uld: "no detection thresholds; there is no ROI on this die at all" },
  },
  VL53L1CX: {
    modelId: 0xeacc,
    reachMm: 4000,
    drivers: { uld: VL53L1, histogram: VL53LX },
    caveats: { histogram: HIST_CAVEAT },
  },
  // Same die and ULD as the CX; the CB is the cover-glass module.
  VL53L1CB: {
    modelId: 0xeacc,
    reachMm: 8000,
    drivers: { uld: VL53L1, histogram: VL53LX },
    caveats: { histogram: HIST_CAVEAT },
  },
  VL53L3CX: {
    modelId: 0xeaaa,
    reachMm: 3000,
    drivers: { ulp: VL53L3, histogram: VL53LX },
    caveats: {
      ulp: "single-target ranging only - the histogram driver gives several targets instead",
      histogram: HIST_CAVEAT,
    },
  },
  VL53L4CD: {
    modelId: 0xebaa,
    reachMm: 1300,
    drivers: { uld: VL53L4, histogram: VL53LX },
    caveats: { histogram: HIST_CAVEAT },
  },
  // The Bare Driver is what ST ships for this part; to run it light, name a
  // sibling: product 'VL53L4CD' (1.2 m, calibrations) or 'VL53L1CX'.
  VL53L4CX: {
    modelId: 0xebaa,
    reachMm: 6000,
    drivers: { histogram: VL53LX },
    caveats: { histogram: HIST_CAVEAT },
  },
};

const NAME_RE = /VL53L(\d[A-Z0-9]*)/;

/**
 * `ToF Sensor VL53L4CD USB v2.1` → 'VL53L4CD'. null if the name carries no
 * product number the table knows.
 */
export function productFromBoardName(name: string | null | undefined): string | null {
  if (!name) return null;
  const m = NAME_RE.exec(name.toUpperCase());
  if (!m) return null;
  const product = "VL53L" + m[1]!;
  return product in TABLE ? product : null;
}

/** An error the Python port raises as NotImplementedError. */
export class NotImplementedError extends Error {
  constructor(message?: string) {
    super(message);
    this.name = "NotImplementedError";
  }
}

/** → the table row. Throws for a product nobody serves. */
export function product(name: string): Product {
  const row = TABLE[name];
  if (row === undefined) {
    throw new NotImplementedError(
      `no such product: ${name} - served: ${supportedProducts().join(", ")}`,
    );
  }
  return row;
}

/** → the driver kinds this product has, in DRIVER_KINDS order. */
export function driverKinds(name: string): string[] {
  const have = product(name).drivers;
  return DRIVER_KINDS.filter((k) => have[k] !== undefined);
}

/** → [driver class, caveat or null] for one product/driver pair. */
export function driverFor(name: string, kind: string): [DriverClass, string | null] {
  const row = product(name);
  const cls = row.drivers[kind];
  if (cls === undefined) {
    throw new NotImplementedError(
      `${name} has no '${kind}' driver - it has ${driverKinds(name).join(", ")}`,
    );
  }
  return [cls, row.caveats[kind] ?? null];
}

/** The product's rated maximum ranging distance, mm (datasheet), or null. */
export function reachMm(name: string | null | undefined): number | null {
  const row = name ? TABLE[name] : undefined;
  return row ? row.reachMm : null;
}

/** Cross-check the id the sensor answered: true only says "not something else". */
export function modelIdOk(name: string | null | undefined, value: number): boolean {
  const row = name ? TABLE[name] : undefined;
  return row !== undefined && row.modelId === value;
}

export function supportedProducts(): string[] {
  return PRODUCTS.filter((p) => p in TABLE);
}
