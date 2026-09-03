/**
 * Golden consumer for `vl53l8_advanced.json`: the VL53L8CX advanced-feature
 * DCI codecs (ST ULD port). Asserts byte parity on the motion-indicator
 * config pack, the 64-entry detection-threshold DCI payload + valid-status
 * block, and the xtalk-margin raw scaling.
 */

import { describe, expect, it } from "vitest";
import {
  type DetectionThreshold,
  defaultMotionConfig,
  packDetectionThresholds,
  xtalkMarginToRaw,
} from "../../src/index.js";
import { loadVectors, toHex } from "./vectors.js";

interface MotionCase {
  name: string;
  resolution: number;
  pack: string;
}
interface ThreshEntry {
  low_thresh: number;
  high_thresh: number;
  measurement: number;
  type: number;
  zone_num: number;
  operation: number;
}
interface ThreshCase {
  name: string;
  thresholds: ThreshEntry[];
  start_block: string;
  valid_status: string;
}
interface XtalkMarginCase {
  name: string;
  kcps: number;
  raw: number;
}
interface AdvancedVectors {
  motion: MotionCase[];
  thresholds: ThreshCase[];
  xtalk_margin: XtalkMarginCase[];
}

const data = loadVectors<AdvancedVectors>("vl53l8_advanced.json");

describe("vl53l8 advanced: motion-indicator config pack", () => {
  for (const c of data.motion) {
    it(c.name, () => {
      const cfg = defaultMotionConfig(c.resolution);
      expect(toHex(cfg.pack())).toBe(c.pack);
    });
  }
});

describe("vl53l8 advanced: detection thresholds", () => {
  for (const c of data.thresholds) {
    it(c.name, () => {
      const thresholds: Partial<DetectionThreshold>[] = c.thresholds.map((t) => ({
        lowThresh: t.low_thresh,
        highThresh: t.high_thresh,
        measurement: t.measurement,
        type: t.type,
        zoneNum: t.zone_num,
        operation: t.operation,
      }));
      const { start, valid } = packDetectionThresholds(thresholds);
      expect(toHex(start)).toBe(c.start_block);
      expect(toHex(valid)).toBe(c.valid_status);
    });
  }
});

describe("vl53l8 advanced: xtalk margin raw scaling", () => {
  for (const c of data.xtalk_margin) {
    it(c.name, () => {
      expect(xtalkMarginToRaw(c.kcps)).toBe(c.raw);
    });
  }
});
