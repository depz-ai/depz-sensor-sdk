/**
 * The generated base64 asset modules must decode byte-identical to the
 * Python package's .bin blobs (single source of truth). Regenerate with
 * `bun scripts/gen-vl53l8-assets.mjs` when the blobs change.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { loadAssets, type Vl53l8Assets } from "../src/sensors/vl53l8/assets/index.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const DATA_ROOT = path.resolve(
  here,
  "../../depz-sensor-sdk-python/depz_sensor_sdk/vl53l8/data",
);

const EXPECTED_SIZES: Record<keyof Vl53l8Assets, [file: string, size: number]> = {
  firmware: ["firmware.bin", 86016],
  defaultCfg: ["default_configuration.bin", 972],
  defaultXtalk: ["default_xtalk.bin", 776],
  getNvmCmd: ["get_nvm_cmd.bin", 40],
};

describe("vl53l8 blob assets", () => {
  for (const variant of ["cx", "ch"] as const) {
    it(`'${variant}' assets match the .bin sources byte-for-byte`, async () => {
      const assets = await loadAssets(variant);
      for (const key of Object.keys(EXPECTED_SIZES) as Array<keyof Vl53l8Assets>) {
        const [file, size] = EXPECTED_SIZES[key];
        const want = new Uint8Array(readFileSync(path.join(DATA_ROOT, variant, file)));
        expect(want.length, `${variant}/${file} source size`).toBe(size);
        const got = assets[key];
        expect(got.length, `${variant}/${file} decoded size`).toBe(size);
        expect(Buffer.from(got).equals(Buffer.from(want)), `${variant}/${file} bytes`).toBe(true);
      }
    });
  }
});
