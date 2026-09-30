/**
 * SR04 full stack over a committed real-hardware capture (no device).
 * Mirror of the Python `tests/test_sr04_replay.py`.
 *
 * Recorded 2026-09-28 from the lab SR04 (UMPVOB2461, APP_usonic_SR04_v0.97) by
 * the Python SDK: identify, device name, set/get sample period (20 ms), echo
 * decay, three single shots, the loop for 20 samples, stop. strictTx makes
 * every request re-issue byte for byte; every decoded measurement must match
 * the sidecar. The C and C++ SDKs replay the same file.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { Sr04, type Sr04Measurement } from "../src/sensors/sr04.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

const asJson = (m: Sr04Measurement) => ({
  timestamp_us: Number(m.timestampUs),
  echo_time_us: m.echoTimeUs,
  source: m.source,
});

describe("sr04 full-stack replay", () => {
  it("sr04_session: byte-identical session, every measurement decoded", async () => {
    const exp = JSON.parse(readFileSync(path.join(RECORDINGS, "sr04_session.expected.json"), "utf8"));
    const replay = new ReplayTransport(readFileSync(path.join(RECORDINGS, "sr04_session.depzrec"), "utf8"), {
      strictTx: true,
    });
    const dev = new Sr04(replay, { timeoutMs: 2000 });
    await dev.open();
    const once: Sr04Measurement[] = [];
    const loop: Sr04Measurement[] = [];
    try {
      expect((await dev.identify()).sensorType).toBe("sr04");
      expect(await dev.getSoftwareName()).toBe(exp.software_name);
      expect(await dev.getDeviceName()).toBe(exp.device_name);
      await dev.setSamplePeriodUs(20_000);
      expect(await dev.getSamplePeriodUs()).toBe(exp.sample_period_us);
      expect(await dev.getEchoDecayUs()).toBe(exp.echo_decay_us);
      for (let i = 0; i < exp.once.length; i++) once.push(await dev.measureOnce(2000));
      const stream = dev.measurements(exp.loop.length + 64);
      await dev.start();
      for await (const m of stream) {
        loop.push(m);
        if (loop.length === exp.loop.length) break;
      }
      await dev.stop();
    } finally {
      await dev.close();
    }
    expect(once.map(asJson)).toEqual(exp.once);
    expect(loop.map(asJson)).toEqual(exp.loop);
  });
});
