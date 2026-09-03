/**
 * Full VL53L8 stack over the committed real-hardware capture (no device).
 * Mirror of the Python `tests/test_vl53l8_replay.py`.
 *
 * The fixture was recorded from a live VL53L8CX (SN000005): init (fw
 * download), 8x8 @15 Hz, 45 frames. Replay is causal (rx gated on tx
 * prefix), so the whole ULD init sequence — polls included — must re-issue
 * byte-identical requests. This is simultaneously a protocol regression test
 * (strictTx: every tx byte compared against the python-recorded stream —
 * cross-language byte parity of the entire stack) and a parser E2E: every
 * decoded frame must match the recorded sidecar exactly.
 *
 * `sleepImpl` is instant: the ULD boot/poll sleeps total ~15 s in real time
 * (the Python test lives with that; vitest should not).
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { RESOLUTION_8X8 } from "../src/sensors/vl53l8/uld.js";
import { Vl53l8, type Vl53l8Frame } from "../src/sensors/vl53l8/vl53l8.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");
const FIXTURE = path.join(RECORDINGS, "vl53l8_8x8_15hz_3s.depzrec");
const EXPECTED = path.join(RECORDINGS, "vl53l8_8x8_15hz_3s.expected.json");

interface ExpectedFrame {
  timestamp_us: number;
  resolution: number;
  silicon_temp_degc: number;
  distance_mm: number[];
  target_status: number[];
  nb_target_detected: number[];
}

interface Expected {
  software_name: string;
  frames: ExpectedFrame[];
}

describe("vl53l8 full-stack replay", () => {
  it("re-issues a byte-identical session and decodes every frame", async () => {
    const expected = JSON.parse(readFileSync(EXPECTED, "utf8")) as Expected;
    const replay = new ReplayTransport(readFileSync(FIXTURE, "utf8"), { strictTx: true });

    const dev = new Vl53l8(replay, {
      timeoutMs: 2000,
      sleepImpl: () => Promise.resolve(), // replay answers polls instantly
    });
    await dev.open();
    let frames: Vl53l8Frame[];
    try {
      // Same call sequence as the python test: _identify() + get_software_name().
      const ident = await dev.identify();
      expect(ident.sensorType).toBe("vl53l8");
      expect(await dev.getSoftwareName()).toBe(expected.software_name);
      await dev.init("cx");
      expect(dev.variant).toBe("cx");
      await dev.setResolution(RESOLUTION_8X8);
      await dev.setRangingFrequencyHz(15);
      // Subscribe BEFORE starting: replay serves the whole session
      // instantly, and the queue must be sized for all frames (drop-oldest
      // would otherwise eat some and the iteration would wait forever).
      const stream = dev.frames(expected.frames.length + 8);
      await dev.startRanging();
      frames = [];
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
      expect(got.siliconTempDegc).toBe(want.silicon_temp_degc);
      expect(Array.from(got.distanceMm)).toEqual(want.distance_mm);
      expect(Array.from(got.targetStatus)).toEqual(want.target_status);
      expect(Array.from(got.nbTargetDetected)).toEqual(want.nb_target_detected);
    }
  }, 30_000);
});
