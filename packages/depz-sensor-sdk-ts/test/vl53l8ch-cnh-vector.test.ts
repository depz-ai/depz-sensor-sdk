// CNH decode parity against the golden vector captured from a live VL53L8CH.
// Same vector the Python/C/C++/Java/Rust/C# SDKs consume — keeps all seven
// bindings byte-exact on the Compact-Network-Histogram decode path.
import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { CnhConfig, decode } from "../src/sensors/vl53l8/cnh.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const VECTOR = path.resolve(here, "../../../contracts/vectors/vl53l8_cnh.json");

function fromHex(h: string): Uint8Array {
  const out = new Uint8Array(h.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(h.slice(i * 2, i * 2 + 2), 16);
  return out;
}

describe("vl53l8ch CNH decode (golden vector)", () => {
  it("matches the live-captured golden vector byte-exact", () => {
    const v = JSON.parse(readFileSync(VECTOR, "utf8"));
    const c = v.config;
    const cfg = new CnhConfig();
    cfg.initConfig(c.start_bin, c.num_bins, c.sub_sample);
    cfg.createAggMap(c.resolution, ...(c.agg_map as [number, number, number, number, number, number]));

    const raw = fromHex(v.cnh_raw);
    const decoded = decode(cfg, raw);
    const exp = v.expected;

    const refWord = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(8, true);
    expect(refWord).toBe(exp.ref_residual_word);
    expect(decoded.aggregates.length).toBe(exp.nb_aggregates);
    decoded.aggregates.forEach((got, i) => {
      expect(got.histRaw).toEqual(exp.aggregates[i].hist_raw);
      expect(got.histScaler).toEqual(exp.aggregates[i].hist_scaler);
    });
  });
});
