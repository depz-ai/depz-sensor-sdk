// XTALK_PLANE_OFFSET_KCPS -> kcps on the VL53L1 die rounds like ST's C round():
// calibration stores kcps x512 with a fraction, so the register can hold an
// exact .5 kcps, and ST's VL53L4CD_GetXtalk sends it away from zero.
import { describe, expect, it } from "vitest";

import { roundHalfAway } from "../src/sensors/vl53lx/uld/arith.js";
import { VL53L4 } from "../src/sensors/vl53lx/uld/l4.js";
import type { BridgePlatform } from "../src/sensors/vl53lx/uld/base.js";

describe("vl53lx xtalk rounding", () => {
  it.each([
    [0.5, 1],
    [1.5, 2],
    [2.5, 3],
    [2.4999, 2],
    [-0.5, -1],
    [-2.5, -3],
    [-2.4, -2],
    [0.49999999999999994, 0],
    [0, 0],
  ])("roundHalfAway(%d) = %d", (x, want) => {
    expect(roundHalfAway(x)).toBe(want);
  });

  it.each([
    [256, 1],
    [767, 1],
    [768, 2],
    [1280, 3],
    [65535, 128],
  ])("getXtalk raw %d -> %d kcps", async (raw, kcps) => {
    const p = { rdWord: async () => raw } as unknown as BridgePlatform;
    expect(await new VL53L4(p, "VL53L4CD").getXtalk()).toBe(kcps);
  });
});
