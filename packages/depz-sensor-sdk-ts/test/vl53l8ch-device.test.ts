/**
 * VL53L8CH-only device facade: the CH↔CX class relationship and the CH-only
 * `configureCnh()` surface. The base ToF facade (config guards, advanced ULD
 * delegation, helpers) is shared with CX and covered in
 * `vl53l8cx-device.test.ts`; the CNH pure-math parity lives in
 * `vl53l8ch-cnh.test.ts`. Exercised via the fake DCI platform (no hardware).
 */

import { afterEach, describe, expect, it, vi } from "vitest";
import {
  CnhConfig,
  DepzError,
  LoopbackTransport,
  MI_CFG_DEV_IDX,
  RESOLUTION_8X8,
  VL53L8CX,
  Vl53l8ch,
  Vl53l8cx,
} from "../src/index.js";
import { makeDriver } from "./fake-vl53l8.js";

function ch(): Vl53l8ch {
  return new Vl53l8ch(new LoopbackTransport());
}

function cx(): Vl53l8cx {
  return new Vl53l8cx(new LoopbackTransport());
}

// ── CH ↔ CX class relationship ────────────────────────────────────────────────

describe("vl53l8ch class wiring", () => {
  it("Vl53l8ch is a Vl53l8cx superset (inherits CX) with configureCnh", () => {
    const dev = ch();
    expect(dev).toBeInstanceOf(Vl53l8cx);
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

// ── CNH ↔ motion indicator: one DCI block, mutually exclusive (ERRATA B4) ─────

describe("vl53l8ch CNH and the motion indicator share DCI 0xBFAC", () => {
  afterEach(() => {
    vi.restoreAllMocks();
  });

  function armed(): { dev: Vl53l8ch; drv: VL53L8CX; p: ReturnType<typeof makeDriver>["p"] } {
    const dev = ch();
    const { drv, p } = makeDriver();
    (dev as unknown as { uldDriver: unknown }).uldDriver = drv;
    return { dev, drv, p };
  }

  function cnh16x20(): CnhConfig {
    const config = new CnhConfig();
    config.initConfig(10, 20, 2);
    config.createAggMap(64, 0, 0, 2, 2, 4, 4);
    return config;
  }

  it("configureCnh disarms the motion indicator", async () => {
    const { dev, drv, p } = armed();
    await drv.setResolution(RESOLUTION_8X8);
    await dev.configureMotionIndicator(400, 1500);
    expect(drv.motionPresent).toBe(true);
    const config = cnh16x20();
    await dev.configureCnh(config);
    expect(drv.motionPresent).toBe(false); // frames parse with motion null
    expect(dev.activeCnhConfig).toBe(config);
    expect(p.dci.get(MI_CFG_DEV_IDX)).toEqual(config.pack());
  });

  it("configureMotionIndicator disarms CNH", async () => {
    const { dev, drv, p } = armed();
    await drv.setResolution(RESOLUTION_8X8);
    await dev.configureCnh(cnh16x20());
    const cfg = await dev.configureMotionIndicator(400, 1500);
    expect(dev.activeCnhConfig).toBeNull(); // startRanging adds no CNH block
    expect(drv.motionPresent).toBe(true);
    expect(p.dci.get(MI_CFG_DEV_IDX)).toEqual(cfg.pack());
  });

  it("init() disarms both", async () => {
    vi.spyOn(VL53L8CX.prototype, "init").mockResolvedValue(undefined);
    const { dev, drv } = armed();
    await drv.setResolution(RESOLUTION_8X8);
    await dev.configureMotionIndicator(400, 1500);
    await dev.configureCnh(cnh16x20());
    await dev.init();
    expect(dev.activeCnhConfig).toBeNull();
    const fresh = (dev as unknown as { uldDriver: VL53L8CX }).uldDriver;
    expect(fresh).not.toBe(drv);
    expect(fresh.motionPresent).toBe(false);
  });
});
