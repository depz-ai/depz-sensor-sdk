/**
 * Full VL53L8CH stack over committed real-hardware captures (no device).
 * Mirror of the Python `tests/test_vl53l8ch_replay.py`.
 *
 * Recorded 2026-09-25 from the lab board TMNQ8E3PRR on APP_VL53L8_v0.92 by the
 * Python SDK: software name, init (CH / VL53LMZ blob over the SPI bridge),
 * 8x8 @ 15 Hz, then the same with a 16-aggregate × 20-bin CNH block that rides
 * inside every streamed frame. strictTx makes every request byte-identical.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { CnhConfig } from "../src/sensors/vl53l8/cnh.js";
import { RESOLUTION_8X8 } from "../src/sensors/vl53l8/uld.js";
import { Vl53l8ch, type Vl53l8Frame } from "../src/sensors/vl53l8/vl53l8.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

const toHex = (b: Uint8Array) => Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");

describe("vl53l8ch full-stack replay", () => {
  for (const [stem, withCnh] of [
    ["vl53l8ch_8x8_15hz_3s", false],
    ["vl53l8ch_cnh_8x8_15hz", true],
  ] as const) {
    it(`${stem}: byte-identical session, every frame decoded`, async () => {
      const expected = JSON.parse(readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"));
      const replay = new ReplayTransport(readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"), {
        strictTx: true,
      });
      // Over a bare transport there is no USB PID — the class is fixed by hand.
      const dev = new Vl53l8ch(replay, { timeoutMs: 2000 });
      await dev.open();
      const frames: Vl53l8Frame[] = [];
      try {
        expect(await dev.getSoftwareName()).toBe(expected.software_name);
        await dev.init();
        await dev.setResolution(RESOLUTION_8X8);
        await dev.setRangingFrequencyHz(15);
        if (withCnh) {
          const cfg = new CnhConfig();
          cfg.initConfig(10, 20, 2);
          cfg.createAggMap(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4);
          await dev.configureCnh(cfg);
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
        const want = expected.frames[i];
        expect(got.timestampUs).toBe(BigInt(want.timestamp_us));
        expect(got.resolution).toBe(64);
        expect(got.siliconTempDegc).toBe(want.silicon_temp_degc);
        expect(Array.from(got.distanceMm.subarray(0, 64))).toEqual(want.distance_mm);
        expect(Array.from(got.targetStatus.subarray(0, 64))).toEqual(want.target_status);
        expect(Array.from(got.nbTargetDetected.subarray(0, 64))).toEqual(want.nb_target_detected);
        if (withCnh) expect(toHex(got.cnhRaw!)).toBe(want.cnh_raw);
        else expect(got.cnhRaw).toBeNull();
      }
    }, 60_000);
  }
});
