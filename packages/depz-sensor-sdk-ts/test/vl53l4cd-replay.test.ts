/**
 * VL53L4CD full stack over a committed real-hardware capture (no device).
 * Mirror of the Python `tests/test_vl53l4cd_replay.py`.
 *
 * Recorded 2026-09-28 from the lab VL53L4CD (TL7TKSLW8Z, APP_VL53L4_v0.83) by
 * the Python SDK: identify, device name, bridge info, init, range timing 33 ms,
 * read-backs, two single shots, 20 streamed frames, stop. strictTx makes every
 * ULD poll loop re-issue byte-identical requests.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { Vl53l4cd, type Vl53l4Measurement } from "../src/sensors/vl53l4/vl53l4.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

const asJson = (m: Vl53l4Measurement) => ({
  timestamp_us: Number(m.timestampUs),
  range_status: m.rangeStatus,
  distance_mm: m.distanceMm,
  sigma_mm: m.sigmaMm,
  signal_rate_kcps: m.signalRateKcps,
  ambient_rate_kcps: m.ambientRateKcps,
  signal_per_spad_kcps: m.signalPerSpadKcps,
  ambient_per_spad_kcps: m.ambientPerSpadKcps,
  number_of_spad: m.numberOfSpad,
  stream_count: m.streamCount,
});

describe("vl53l4cd full-stack replay", () => {
  it("vl53l4cd_session: byte-identical session, every measurement decoded", async () => {
    const exp = JSON.parse(readFileSync(path.join(RECORDINGS, "vl53l4cd_session.expected.json"), "utf8"));
    const replay = new ReplayTransport(
      readFileSync(path.join(RECORDINGS, "vl53l4cd_session.depzrec"), "utf8"),
      { strictTx: true },
    );
    const dev = new Vl53l4cd(replay, { timeoutMs: 2000, sleepImpl: () => Promise.resolve() });
    await dev.open();
    const once: Vl53l4Measurement[] = [];
    const frames: Vl53l4Measurement[] = [];
    try {
      expect((await dev.identify()).sensorType).toBe("vl53l4");
      expect(await dev.getSoftwareName()).toBe(exp.software_name);
      expect(await dev.getDeviceName()).toBe(exp.device_name);
      const info = await dev.bridgeInfo();
      expect([info.modelId, info.fwStatus, info.i2cKhz]).toEqual([
        exp.info.model_id, exp.info.fw_status, exp.info.i2c_khz,
      ]);
      await dev.init();
      await dev.setRangeTiming(33, 0);
      const t = await dev.getRangeTiming();
      expect([t.timingBudgetMs, t.interMeasurementMs]).toEqual(exp.timing);
      expect(await dev.getOffsetMm()).toBe(exp.offset_mm);
      expect(await dev.getXtalkKcps()).toBe(exp.xtalk_kcps);
      for (let i = 0; i < exp.once.length; i++) once.push(await dev.measureOnce());
      const stream = dev.measurements(exp.frames.length + 64);
      await dev.startRanging();
      for await (const m of stream) {
        frames.push(m);
        if (frames.length === exp.frames.length) break;
      }
      await dev.stopRanging();
      expect(dev.streamParseErrors).toBe(0);
    } finally {
      await dev.close();
    }
    expect(once.map(asJson)).toEqual(exp.once);
    expect(frames.map(asJson)).toEqual(exp.frames);
  });
});
