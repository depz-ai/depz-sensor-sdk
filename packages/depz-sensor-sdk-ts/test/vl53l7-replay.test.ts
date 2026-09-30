/**
 * Full VL53L5CX / VL53L7CH stack over committed real-hardware captures (no
 * device). Mirror of the Python `tests/test_vl53l7_replay.py`.
 *
 * Recorded 2026-09-24 from live boards on APP_VL53L7_v0.53 (L5CX TS5J50RYCG,
 * L7CH TXK5KAX6X4) by the Python SDK: identify, device name (it picks the
 * L5/L7 class), init (L5/L7 boot branch + fw download over the I2C bridge),
 * 8x8 (one 4x4) @15 Hz. The CNH capture's 3156 B frames arrive chunked.
 * strictTx makes every request byte-identical to the Python session.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { resolveVl53l7Model } from "../src/protocol/vl53l7.js";
import { Vl53l5cx, Vl53l7ch, Vl53l7cx } from "../src/sensors/vl53l7/vl53l7.js";
import { CnhConfig } from "../src/sensors/vl53l8/cnh.js";
import { RESOLUTION_4X4, RESOLUTION_8X8 } from "../src/sensors/vl53l8/uld.js";
import type { Vl53l8Frame } from "../src/sensors/vl53l8/vl53l8.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

interface ExpectedFrame {
  timestamp_us: number;
  resolution: number;
  silicon_temp_degc: number;
  distance_mm: number[];
  target_status: number[];
  nb_target_detected: number[];
  cnh_raw?: string;
}

interface Expected {
  software_name: string;
  class: string;
  module_type: number;
  frames: ExpectedFrame[];
}

const CASES: Array<[string, string, number, boolean]> = [
  ["vl53l5cx_8x8_15hz_3s", "Vl53l5cx", RESOLUTION_8X8, false],
  // 4x4 pins the L5/L7 output-list rule: per-target blocks stay 64 entries on
  // the wire and the parser trims them to the 16 real zones.
  ["vl53l5cx_4x4_15hz", "Vl53l5cx", RESOLUTION_4X4, false],
  ["vl53l7ch_8x8_15hz_3s", "Vl53l7ch", RESOLUTION_8X8, false],
  ["vl53l7ch_cnh_8x8_15hz", "Vl53l7ch", RESOLUTION_8X8, true],
];

const CLASSES = { vl53l5cx: Vl53l5cx, vl53l7cx: Vl53l7cx, vl53l7ch: Vl53l7ch };

function toHex(b: Uint8Array): string {
  return Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");
}

describe("vl53l7 full-stack replay", () => {
  for (const [stem, className, resolution, withCnh] of CASES) {
    it(`${stem}: byte-identical session, every frame decoded`, async () => {
      const expected = JSON.parse(
        readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"),
      ) as Expected;
      const replay = new ReplayTransport(
        readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"),
        { strictTx: true },
      );
      // The class under test is the one the capture resolved; the request
      // sequence mirrors the python capture (_identify, then _promote reads
      // the device name to pick the L5/L7 class, then get_software_name).
      const Cls = CLASSES[className.toLowerCase() as keyof typeof CLASSES];
      const dev: Vl53l7cx = new Cls(replay, { timeoutMs: 2000, sleepImpl: () => Promise.resolve() });
      await dev.open();
      const frames: Vl53l8Frame[] = [];
      try {
        expect((await dev.identify()).sensorType).toBe("vl53l7");
        // The firmware name alone cannot tell L5/L7/CH apart — the name does.
        expect(resolveVl53l7Model(null, await dev.getDeviceName())).toBe(className.toLowerCase());
        expect(className).toBe(expected.class);
        expect(await dev.getSoftwareName()).toBe(expected.software_name);
        await dev.init();
        expect(dev.moduleType).toBe(expected.module_type);
        await dev.setResolution(resolution);
        await dev.setRangingFrequencyHz(15);
        if (withCnh) {
          const cfg = new CnhConfig();
          cfg.initConfig(10, 20, 2);
          cfg.createAggMap(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
          await (dev as Vl53l7ch).configureCnh(cfg);
        }
        const stream = dev.frames(expected.frames.length + 8);
        await dev.startRanging();
        for await (const frame of stream) {
          frames.push(frame);
          if (frames.length === expected.frames.length) break;
        }
        await dev.stopRanging();
      } finally {
        await dev.close();
      }

      expect(frames.length).toBe(expected.frames.length);
      for (let i = 0; i < frames.length; i++) {
        const got = frames[i]!;
        const want = expected.frames[i]!;
        expect(got.timestampUs).toBe(BigInt(want.timestamp_us));
        expect(got.resolution).toBe(want.resolution);
        expect(got.resolution).toBe(resolution);
        expect(got.siliconTempDegc).toBe(want.silicon_temp_degc);
        expect(Array.from(got.distanceMm)).toEqual(want.distance_mm);
        expect(Array.from(got.targetStatus)).toEqual(want.target_status);
        expect(Array.from(got.nbTargetDetected)).toEqual(want.nb_target_detected);
        if (withCnh) {
          expect(got.cnhRaw).not.toBeNull();
          expect(toHex(got.cnhRaw!)).toBe(want.cnh_raw);
        } else {
          expect(got.cnhRaw).toBeNull();
        }
      }
    }, 30_000);
  }
});
