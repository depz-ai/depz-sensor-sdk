/**
 * VL53L8 advanced ULD features (power modes, xtalk margin, detection
 * thresholds, motion indicator, caldata guards) against a fake register/DCI
 * platform. Port of the Python `tests/test_vl53l8_advanced.py`.
 *
 * The fake emulates the DCI read/write transport (swapBuffer + header/footer
 * framing), so the real byte sequences in uld.ts run end to end — no hardware.
 */

import { describe, expect, it } from "vitest";
import {
  DCI_DET_THRESH_START,
  DCI_MOTION_DETECTOR_CFG,
  DCI_PIPE_CONTROL,
  DCI_ZONE_CONFIG,
  DIST_MM,
  LAST_THRESHOLD,
  NB_THRESHOLDS,
  POWER_MODE_DEEP_SLEEP,
  POWER_MODE_SLEEP,
  POWER_MODE_WAKEUP,
  RESOLUTION_8X8,
  THRESH_OP_NONE,
  THRESH_OUT_OF_WINDOW,
  Vl53l8cxError,
  XTALK_BUFFER_SIZE,
  type DetectionThreshold,
} from "../src/sensors/vl53l8/uld.js";
import { makeDriver } from "./fake-vl53l8.js";

// ── power modes ───────────────────────────────────────────────────────────────

describe("vl53l8 uld: power modes", () => {
  it("get power mode: wakeup", async () => {
    const { drv, p } = makeDriver();
    p.reg.set(0x09, 0x04);
    expect(await drv.getPowerMode()).toBe(POWER_MODE_WAKEUP);
  });

  it("get power mode: sleep vs deep sleep by reg 0x0F", async () => {
    const { drv, p } = makeDriver();
    p.reg.set(0x09, 0x02);
    p.reg.set(0x000f, 0x00);
    expect(await drv.getPowerMode()).toBe(POWER_MODE_SLEEP);
    p.reg.set(0x000f, 0x43);
    expect(await drv.getPowerMode()).toBe(POWER_MODE_DEEP_SLEEP);
  });

  it("set power mode: sleep writes 0x02 to reg 0x09", async () => {
    const { drv, p } = makeDriver();
    p.reg.set(0x09, 0x04); // currently awake
    p.reg.set(0x06, 0x00); // poll target: (buf[0] & 0x01) == 0
    await drv.setPowerMode(POWER_MODE_SLEEP);
    expect(p.writes.some(([addr, d]) => addr === 0x09 && d.length === 1 && d[0] === 0x02)).toBe(true);
  });
});

// ── xtalk margin ──────────────────────────────────────────────────────────────

describe("vl53l8 uld: xtalk margin", () => {
  it("roundtrips through the DCI codec", async () => {
    const { drv } = makeDriver();
    await drv.setXtalkMargin(50.0);
    expect(await drv.getXtalkMargin()).toBeCloseTo(50.0, 3);
  });

  it("rejects an out-of-range margin", async () => {
    const { drv } = makeDriver();
    await expect(drv.setXtalkMargin(20000)).rejects.toThrow(Vl53l8cxError);
  });
});

// ── detection thresholds ──────────────────────────────────────────────────────

describe("vl53l8 uld: detection thresholds", () => {
  it("enable roundtrip", async () => {
    const { drv } = makeDriver();
    await drv.setDetectionThresholdsEnable(true);
    expect(await drv.getDetectionThresholdsEnable()).toBe(0x01);
    await drv.setDetectionThresholdsEnable(false);
    expect(await drv.getDetectionThresholdsEnable()).toBe(0x00);
  });

  it("scaling roundtrip (distance ×4) and full 64-entry read back", async () => {
    const { drv, p } = makeDriver();
    const thr: Partial<DetectionThreshold>[] = [
      {
        lowThresh: 200,
        highThresh: 600,
        measurement: DIST_MM, // scale 4
        type: THRESH_OUT_OF_WINDOW,
        zoneNum: LAST_THRESHOLD,
        operation: THRESH_OP_NONE,
      },
    ];
    await drv.setDetectionThresholds(thr);
    const got = await drv.getDetectionThresholds();
    expect(got).toHaveLength(NB_THRESHOLDS);
    expect(got[0]!.lowThresh).toBe(200);
    expect(got[0]!.highThresh).toBe(600);
    expect(got[0]!.measurement).toBe(DIST_MM);
    expect(got[0]!.zoneNum).toBe(LAST_THRESHOLD);
    // raw storage scaled ×4 (distance)
    const raw = p.dci.get(DCI_DET_THRESH_START)!;
    expect(new DataView(raw.buffer, raw.byteOffset).getInt32(0, true)).toBe(200 * 4);
  });

  it("auto-stop sets pipe-control byte[3]", async () => {
    const { drv, p } = makeDriver();
    p.dci.set(DCI_PIPE_CONTROL, Uint8Array.of(1, 0, 1, 0));
    await drv.setDetectionThresholdsAutoStop(true);
    expect(p.dci.get(DCI_PIPE_CONTROL)![3]).toBe(1);
  });
});

// ── motion indicator ──────────────────────────────────────────────────────────

describe("vl53l8 uld: motion indicator", () => {
  it("init writes a 156-byte config and enables motion output", async () => {
    const { drv, p } = makeDriver();
    const cfg = await drv.motionIndicatorInit(RESOLUTION_8X8);
    expect((drv as unknown as { motionPresent: boolean }).motionPresent).toBe(true);
    expect(p.dci.get(DCI_MOTION_DETECTOR_CFG)!.length).toBe(156);
    // 8x8 map_id follows ((i%8)>>1) + 4*(i//16)
    expect(cfg.mapId[0]).toBe(0);
    expect(cfg.mapId[63]).toBe(((7 % 8) >> 1) + 4 * Math.floor(63 / 16));
  });

  it("set-distance validates the window", async () => {
    const { drv } = makeDriver();
    const cfg = await drv.motionIndicatorInit(RESOLUTION_8X8);
    await expect(drv.motionIndicatorSetDistanceMotion(cfg, 100, 200)).rejects.toThrow(Vl53l8cxError);
    await drv.motionIndicatorSetDistanceMotion(cfg, 400, 1500); // valid
  });
});

// ── caldata xtalk ─────────────────────────────────────────────────────────────

describe("vl53l8 uld: caldata xtalk", () => {
  it("rejects a wrong-length blob", async () => {
    const { drv } = makeDriver();
    await expect(drv.setCaldataXtalk(new Uint8Array(10))).rejects.toThrow(Vl53l8cxError);
  });

  it("accepts and stores a 776-byte blob (save/restore path)", async () => {
    const { drv, p } = makeDriver();
    // getResolution() must return a valid resolution for the restore re-upload;
    // seed the zone-config DCI so 8x8 (8×8) is reported.
    p.dci.set(DCI_ZONE_CONFIG, Uint8Array.of(8, 8, 0, 0, 4, 4, 0, 0));
    const blob = new Uint8Array(XTALK_BUFFER_SIZE).fill(0xab);
    await drv.setCaldataXtalk(blob);
    expect((drv as unknown as { xtalkData: Uint8Array }).xtalkData).toEqual(blob);
  });
});
