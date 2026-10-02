/**
 * VL53L0X on a live session (both found on TB9BGETA6M, 30.09.2026):
 * - a single shot (the calibrations run on one) leaves the device mode on
 *   single ranging; startRanging() must resume the continuous mode chosen
 *   before it instead of refusing;
 * - stopRanging() zeroes register 0x91; the public reference-SPAD re-measure
 *   must put the stop variable back before its VHV/phase single shots, or it
 *   picks the aperture SPADs and every frame after is Signal Fail.
 * Mirror of the Python `tests/test_vl53l0x_device_mode.py`.
 */

import { describe, expect, it } from "vitest";
import type { BridgePlatform } from "../src/sensors/vl53lx/uld/base.js";
import {
  DEVICEMODE_CONTINUOUS_RANGING,
  DEVICEMODE_CONTINUOUS_TIMED,
  DEVICEMODE_SINGLE_RANGING,
  SYSRANGE_MODE_BACKTOBACK,
  SYSRANGE_MODE_TIMED,
  SYSRANGE_START,
  VL53L0X,
} from "../src/sensors/vl53lx/uld/l0x.js";

/** Just enough platform for startRanging(): it only writes. */
function writeLog() {
  const writes: Array<[number, number]> = [];
  const p = {
    wrByte: async (addr: number, value: number) => {
      writes.push([addr, value]);
    },
  } as unknown as BridgePlatform;
  return { p, writes };
}

async function start(modeBeforeSingle: number | null) {
  const { p, writes } = writeLog();
  const drv = new VL53L0X(p, "VL53L0X");
  if (modeBeforeSingle !== null) drv.setDeviceMode(modeBeforeSingle);
  drv.setDeviceMode(DEVICEMODE_SINGLE_RANGING); // what a calibration leaves
  await drv.startRanging();
  return { drv, last: writes[writes.length - 1] };
}

describe("VL53L0X startRanging after a single shot", () => {
  it.each([
    [DEVICEMODE_CONTINUOUS_RANGING, SYSRANGE_MODE_BACKTOBACK],
    [DEVICEMODE_CONTINUOUS_TIMED, SYSRANGE_MODE_TIMED],
  ])("resumes continuous mode %i", async (mode, startValue) => {
    const { drv, last } = await start(mode);
    expect(last).toEqual([SYSRANGE_START, startValue]);
    expect(drv.d.DeviceMode).toBe(mode);
  });

  it("still refuses when no continuous mode was ever chosen", async () => {
    await expect(start(null)).rejects.toThrow(/continuous device mode/);
  });
});

describe("VL53L0X performRefSpadManagement on a live session", () => {
  it("re-arms the stop variable before anything else", async () => {
    const writes: Array<[number, number]> = [];
    const abort = async () => {
      throw new Error("abort at first read");
    };
    const p = {
      wrByte: async (addr: number, value: number) => {
        writes.push([addr, value]);
      },
      rdByte: abort,
      rdWord: abort,
      rdDword: abort,
      rdMulti: abort,
    } as unknown as BridgePlatform;
    const drv = new VL53L0X(p, "VL53L0X");
    drv.d.StopVariable = 0x3c;
    await expect(drv.performRefSpadManagement()).rejects.toThrow(/abort/);
    expect(writes.slice(0, 7)).toEqual([
      [0x80, 0x01],
      [0xff, 0x01],
      [0x00, 0x00],
      [0x91, 0x3c],
      [0x00, 0x01],
      [0xff, 0x00],
      [0x80, 0x00],
    ]);
  });
});
