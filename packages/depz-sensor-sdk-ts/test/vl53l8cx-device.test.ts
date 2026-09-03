/**
 * VL53L8CX base-ToF device facade: pure helpers, config guards, CX class
 * wiring, and the advanced feature methods delegating to the ULD driver
 * (exercised via the fake DCI platform). CH-only behaviour (configureCnh, the
 * CH↔CX distinction) lives in `vl53l8ch-device.test.ts`. Complements the
 * full-stack replay test (init + streaming) and the ULD-level advanced tests.
 */

import { describe, expect, it } from "vitest";
import {
  DepzError,
  LoopbackTransport,
  MIN_RANGING_FREQUENCY_HZ,
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  Vl53l8,
  Vl53l8Cx,
  zoneGrid,
} from "../src/index.js";
import { makeDriver } from "./fake-vl53l8.js";

function cx(): Vl53l8Cx {
  return new Vl53l8Cx(new LoopbackTransport());
}

// ── pure helpers / constants ──────────────────────────────────────────────────

describe("vl53l8cx helpers", () => {
  it("MIN_RANGING_FREQUENCY_HZ is 2", () => {
    expect(MIN_RANGING_FREQUENCY_HZ).toBe(2);
  });

  it("zoneGrid reshapes 4x4 and 8x8 row-major, filling missing with 0", () => {
    const arr16 = Array.from({ length: 16 }, (_, i) => i);
    const g4 = zoneGrid(arr16, RESOLUTION_4X4);
    expect(g4).toHaveLength(4);
    expect(g4[0]).toEqual([0, 1, 2, 3]);
    expect(g4[3]![3]).toBe(15);

    const arr64 = Array.from({ length: 64 }, (_, i) => i);
    const g8 = zoneGrid(arr64, RESOLUTION_8X8);
    expect(g8).toHaveLength(8);
    expect(g8[7]![7]).toBe(63);

    // short array → out-of-range zones read as 0
    expect(zoneGrid([1, 2], RESOLUTION_4X4)[0]).toEqual([1, 2, 0, 0]);
  });
});

// ── config guards (no init / uld needed) ──────────────────────────────────────

describe("vl53l8cx config guards", () => {
  it("setResolution rejects non-4x4/8x8 zone counts", async () => {
    await expect(cx().setResolution(32)).rejects.toThrow(DepzError);
  });

  it("setRangingFrequencyHz enforces the >= 2 Hz floor", async () => {
    await expect(cx().setRangingFrequencyHz(1)).rejects.toThrow(/2 Hz/);
  });

  it("uld getter throws before init()", () => {
    expect(() => cx().uld).toThrow(/init/);
  });

  it("variant getter throws before init()", () => {
    expect(() => cx().variant).toThrow(/init/);
  });

  it("ranging is false and stopRanging is a no-op before ranging starts", async () => {
    const dev = cx();
    expect(dev.ranging).toBe(false);
    await dev.stopRanging(); // must resolve without touching the transport
  });

  it("requireNotRanging blocks reconfiguration while the stream owns the bank", async () => {
    const dev = cx();
    (dev as unknown as { rangingFlag: boolean }).rangingFlag = true;
    await expect(dev.setResolution(RESOLUTION_8X8)).rejects.toThrow(/stopRanging/);
  });
});

// ── class / variant wiring (CX side) ──────────────────────────────────────────

describe("vl53l8cx class wiring", () => {
  it("Vl53l8 aliases the CX base class", () => {
    expect(Vl53l8).toBe(Vl53l8Cx);
    expect(cx()).toBeInstanceOf(Vl53l8);
  });

  it("init() rejects the CH blob variant on a CX class", async () => {
    await expect(cx().init("ch")).rejects.toThrow(DepzError);
  });
});

// ── advanced facade delegation (through the fake ULD platform) ─────────────────

describe("vl53l8cx facade → ULD delegation", () => {
  function withFakeUld(dev: Vl53l8Cx): ReturnType<typeof makeDriver>["p"] {
    const { drv, p } = makeDriver();
    (dev as unknown as { uldDriver: unknown }).uldDriver = drv;
    return p;
  }

  it("setResolution / getResolution round-trip through the driver", async () => {
    const dev = cx();
    withFakeUld(dev);
    await dev.setResolution(RESOLUTION_8X8);
    expect(await dev.getResolution()).toBe(RESOLUTION_8X8);
  });

  it("xtalk margin set/get delegates to the ULD codec", async () => {
    const dev = cx();
    withFakeUld(dev);
    await dev.setXtalkMargin(50);
    expect(await dev.getXtalkMargin()).toBeCloseTo(50, 3);
  });

  it("detection thresholds enable set/get delegates", async () => {
    const dev = cx();
    withFakeUld(dev);
    await dev.setDetectionThresholdsEnable(true);
    expect(await dev.getDetectionThresholdsEnable()).toBe(0x01);
  });

  it("configureMotionIndicator arms the motion block", async () => {
    const dev = cx();
    const p = withFakeUld(dev);
    // getResolution() inside configureMotionIndicator needs a valid zone config.
    p.dci.set(0x5450 /* DCI_ZONE_CONFIG */, Uint8Array.of(8, 8, 0, 0, 4, 4, 0, 0));
    const cfg = await dev.configureMotionIndicator(400, 1500);
    expect(cfg.pack().length).toBe(156);
    expect((dev as unknown as { uld: { motionPresent: boolean } }).uld.motionPresent).toBe(true);
  });

  it("advanced setters honor requireNotRanging", async () => {
    const dev = cx();
    withFakeUld(dev);
    (dev as unknown as { rangingFlag: boolean }).rangingFlag = true;
    await expect(dev.setXtalkMargin(10)).rejects.toThrow(/stopRanging/);
  });
});
