/**
 * VL53L8CH-only device facade: the CH↔CX class relationship and the CH-only
 * `configureCnh()` surface. The base ToF facade (config guards, advanced ULD
 * delegation, helpers) is shared with CX and covered in
 * `vl53l8cx-device.test.ts`; the CNH pure-math parity lives in
 * `vl53l8ch-cnh.test.ts`. Exercised via the fake DCI platform (no hardware).
 */

import { describe, expect, it } from "vitest";
import {
  CnhConfig,
  DepzError,
  LoopbackTransport,
  MI_CFG_DEV_IDX,
  Vl53l8Ch,
  Vl53l8Cx,
} from "../src/index.js";
import { makeDriver } from "./fake-vl53l8.js";

function ch(): Vl53l8Ch {
  return new Vl53l8Ch(new LoopbackTransport());
}

function cx(): Vl53l8Cx {
  return new Vl53l8Cx(new LoopbackTransport());
}

// ── CH ↔ CX class relationship ────────────────────────────────────────────────

describe("vl53l8ch class wiring", () => {
  it("Vl53l8Ch is a Vl53l8Cx superset (inherits CX) with configureCnh", () => {
    const dev = ch();
    expect(dev).toBeInstanceOf(Vl53l8Cx);
    expect(typeof (dev as unknown as { configureCnh: unknown }).configureCnh).toBe("function");
  });

  it("configureCnh is CH-only — absent on the CX base", () => {
    expect((cx() as unknown as { configureCnh?: unknown }).configureCnh).toBeUndefined();
  });

  it("init() rejects the CX blob variant on a CH class", async () => {
    await expect(ch().init("cx")).rejects.toThrow(DepzError);
  });
});

// ── CNH arming (through the fake ULD platform) ─────────────────────────────────

describe("vl53l8ch configureCnh", () => {
  it("writes the CNH config block to the driver and remembers it", async () => {
    const dev = ch();
    const { drv, p } = makeDriver();
    (dev as unknown as { uldDriver: unknown }).uldDriver = drv;
    const config = new CnhConfig();
    config.initConfig(0, 24, 4);
    config.createAggMap(64, 0, 0, 2, 2, 4, 4);
    // sanity: the packed block is the fixed 156-byte MI config
    expect(config.pack().length).toBe(156);
    await dev.configureCnh(config);
    expect(p.dci.has(MI_CFG_DEV_IDX)).toBe(true);
    expect((dev as unknown as { cnhConfig: CnhConfig }).cnhConfig).toBe(config);
  });

  it("configureCnh honors requireNotRanging", async () => {
    const dev = ch();
    const { drv } = makeDriver();
    (dev as unknown as { uldDriver: unknown }).uldDriver = drv;
    (dev as unknown as { rangingFlag: boolean }).rangingFlag = true;
    const config = new CnhConfig();
    config.initConfig(0, 24, 4);
    config.createAggMap(64, 0, 0, 2, 2, 4, 4);
    await expect(dev.configureCnh(config)).rejects.toThrow(/stopRanging/);
  });
});
