/**
 * BNO085 full stack over a committed real-hardware capture (no device).
 * Mirror of the Python `tests/test_bno086_replay.py`.
 *
 * Recorded 2026-09-29 from the lab BNO085 (I5MFL1ONMCD, APP_BNO086_v0.99,
 * SH-2 3.2.13) by the Python SDK: identify, device name, hardware reset,
 * product id, three enables with their read-backs, 150 mixed reports, the
 * disables, ME calibration, oscillator type, the rotation vector's metadata
 * record, counts, errors. strictTx makes every SH-2 request byte-identical —
 * SHTP and command sequence numbers included.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { Bno086 } from "../src/sensors/bno086/bno086.js";
import { SensorId, type Report } from "../src/sensors/bno086/reports.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

const camel = (k: string) => k.replace(/_([a-z])/g, (_, c: string) => c.toUpperCase());

describe("bno086 full-stack replay", () => {
  it("bno086_session: byte-identical session, every report decoded", async () => {
    const exp = JSON.parse(readFileSync(path.join(RECORDINGS, "bno086_session.expected.json"), "utf8"));
    const replay = new ReplayTransport(readFileSync(path.join(RECORDINGS, "bno086_session.depzrec"), "utf8"), {
      strictTx: true,
    });
    const dev = new Bno086(replay, { timeoutMs: 2000 });
    await dev.open();
    const reports: Report[] = [];
    try {
      expect((await dev.identify()).sensorType).toBe("bno086");
      expect(await dev.getSoftwareName()).toBe(exp.software_name);
      expect(await dev.getDeviceName()).toBe(exp.device_name);
      await dev.hardwareReset();
      const pid = await dev.productId();
      expect(pid.swPartNumber).toBe(exp.product_id.sw_part_number);
      expect(pid.swBuildNumber).toBe(exp.product_id.sw_build_number);
      const stream = dev.reports(null, exp.reports.length + 512);
      for (let i = 0; i < exp.enable.length; i++) {
        const [sid, hz] = exp.enable[i] as [number, number];
        const f = await dev.enable(sid, hz);
        expect(f?.intervalUs).toBe(exp.features[i].interval_us);
        expect(f?.sensorId).toBe(exp.features[i].sensor_id);
      }
      for await (const r of stream) {
        reports.push(r);
        if (reports.length === exp.reports.length) break;
      }
      for (const [sid] of exp.enable as [number, number][]) await dev.disable(sid);
      expect(await dev.getCalibration()).toEqual(exp.calibration);
      expect(Number(await dev.getOscillatorType())).toBe(exp.oscillator);
      const md = await dev.getMetadata(SensorId.RotationVector);
      expect(md.rawWords).toEqual(exp.metadata_rv_words);
      expect(md.revision).toBe(4); // word 3: revision high, supply current low
      expect(md.powerMaQ10 / 1024).toBeGreaterThan(5);
      const c = await dev.getCounts(SensorId.RotationVector);
      expect([c.offered, c.accepted, c.on, c.attempted]).toEqual([
        exp.counts_rv.offered, exp.counts_rv.accepted, exp.counts_rv.on, exp.counts_rv.attempted,
      ]);
      expect((await dev.getErrors()).length).toBe(exp.errors.length);
    } finally {
      await dev.close();
    }
    reports.forEach((r, i) => {
      const w = exp.reports[i] as Record<string, unknown>;
      const got = r as unknown as Record<string, unknown>;
      expect(r.type).toBe(w.type);
      for (const [k, v] of Object.entries(w)) {
        if (k === "type") continue;
        const g = got[camel(k)];
        expect(typeof g === "bigint" ? Number(g) : g, `report ${i} ${k}`).toEqual(v);
      }
    });
  });
});
