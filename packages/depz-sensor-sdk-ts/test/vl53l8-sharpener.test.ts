/**
 * VL53L8 sharpener scaling round-trip.
 *
 * Parity twin of the Python `tests/test_vl53l8cx_config.py`
 * `test_sharpener_roundtrip_is_exact_for_every_legal_value` — Python and TS are
 * the only two SDKs that carry the host ULD, so this scaling must stay
 * identical in both or a VL53L8 configured from the viewer would disagree with
 * one configured from a Python script.
 *
 * The register holds the percentage scaled to 0..255. Truncating on the way
 * back (as ST's C ULD does) loses a count for 95 of the 100 legal values, so
 * set(25) read back as 24 — and calibrateXtalk's save/restore then decayed the
 * setting by 1% on every run (25 -> 24 -> 23 -> ...).
 */

import { describe, expect, it } from "vitest";
import { makeDriver } from "./fake-vl53l8.js";

describe("sharpener percent", () => {
  it("round-trips exactly for every legal value 0..99", async () => {
    const { drv } = makeDriver();
    const broken: number[] = [];
    for (let pct = 0; pct < 100; pct++) {
      await drv.setSharpenerPercent(pct);
      const got = await drv.getSharpenerPercent();
      if (got !== pct) broken.push(pct);
    }
    expect(broken, `values that failed to round-trip: ${JSON.stringify(broken)}`).toEqual([]);
  });

  it("does not drift across repeated save/restore (the calibrateXtalk pattern)", async () => {
    const { drv } = makeDriver();
    await drv.setSharpenerPercent(25);
    for (let i = 0; i < 6; i++) {
      const saved = await drv.getSharpenerPercent();
      await drv.setSharpenerPercent(saved);
    }
    expect(await drv.getSharpenerPercent()).toBe(25);
  });

  it("rejects 100 and above", async () => {
    const { drv } = makeDriver();
    await expect(drv.setSharpenerPercent(100)).rejects.toThrow();
  });

  it("matches the python SDK's scaling for the documented sample points", async () => {
    // Cross-language parity: these are the exact pairs asserted on real
    // silicon in tests/hardware/test_hw_vl53l8.py::test_sharpener_round_trips.
    const { drv } = makeDriver();
    for (const pct of [0, 25, 50, 99]) {
      await drv.setSharpenerPercent(pct);
      expect(await drv.getSharpenerPercent()).toBe(pct);
    }
  });
});
