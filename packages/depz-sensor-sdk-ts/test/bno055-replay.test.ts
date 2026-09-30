/**
 * BNO055 full stack over committed real-hardware captures (no device).
 * Mirror of the Python `tests/test_bno055_replay.py`.
 *
 * Recorded 2026-09-25 from a live board on APP_BNO055_v0.12 (sensor SW 03.11)
 * by the Python SDK: identify, device name, bridge info, reset (+ boot-settle
 * poll), configure (+ fusion-start poll), the calibration profile read (a
 * CONFIG round trip), system status, then a timer stream. strictTx makes every
 * poll loop re-issue byte-identical requests; every streamed block must decode
 * to the sidecar's raw values and be scaled by the stream's units.
 */

import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { Bno055, type Bno055Sample } from "../src/sensors/bno055/bno055.js";
import {
  BNO055_FUSION_ACCEL_LSB,
  BNO055_MAG_LSB,
  BNO055_QUAT_LSB,
  bno055Lsb,
  decodeBno055Block,
  unpackBno055Units,
  type Bno055OprMode,
} from "../src/sensors/bno055/regs.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");
const CAPTURES = readdirSync(RECORDINGS)
  .filter((f) => f.startsWith("bno055_") && f.endsWith(".expected.json"))
  .map((f) => f.slice(0, -".expected.json".length))
  .sort();

const toHex = (b: Uint8Array) => Array.from(b, (x) => x.toString(16).padStart(2, "0")).join("");

describe("bno055 full-stack replay", () => {
  it("finds the captures", () => {
    expect(CAPTURES.length).toBeGreaterThanOrEqual(2);
  });

  for (const stem of CAPTURES) {
    it(`${stem}: byte-identical session, every block decoded`, async () => {
      const exp = JSON.parse(readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"));
      const units = unpackBno055Units(exp.unit_sel);
      const block: [number, number] = [exp.block[0], exp.block[1]];
      const replay = new ReplayTransport(readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"), {
        strictTx: true,
      });
      const dev = new Bno055(replay, { timeoutMs: 2000, sleepImpl: () => Promise.resolve() });
      await dev.open();
      const frames: Bno055Sample[] = [];
      try {
        expect((await dev.identify()).sensorType).toBe("bno055");
        expect(await dev.getSoftwareName()).toBe(exp.software_name);
        expect(await dev.getDeviceName()).toBe(exp.device_name);
        const info = await dev.bridgeInfo();
        expect(info.chipId).toBe(exp.info.chip_id);
        expect(info.swRev).toBe(exp.info.sw_rev);
        await dev.resetSensor();
        await dev.configure({ mode: exp.mode as Bno055OprMode, units });
        const p = await dev.readCalibrationProfile();
        expect({
          accel_offset: p.accelOffset, mag_offset: p.magOffset, gyro_offset: p.gyroOffset,
          accel_radius: p.accelRadius, mag_radius: p.magRadius,
        }).toEqual(exp.calibration_profile);
        const st = await dev.systemStatus();
        expect({ self_test: st.selfTest, clk_status: st.clkStatus, status: st.status, error: st.error })
          .toEqual(exp.system_status);
        const stream = dev.samples(exp.frames.length + 8);
        await dev.startStream(exp.period_ms, block);
        for await (const s of stream) {
          frames.push(s);
          if (frames.length === exp.frames.length) break;
        }
        await dev.stopStream();
      } finally {
        await dev.close();
      }

      expect(dev.streamParseErrors).toBe(0);
      expect(frames.length).toBe(exp.frames.length);
      const lsb = bno055Lsb(units);
      for (let i = 0; i < frames.length; i++) {
        const got = frames[i]!;
        const want = exp.frames[i];
        expect(got.timestampUs).toBe(BigInt(want.timestamp_us));
        expect(got.addr).toBe(block[0]);
        expect(toHex(got.raw)).toBe(want.raw);
        expect(got.units).toEqual(units);
        const r = decodeBno055Block(got.addr, got.raw);
        expect({
          accel: r.accel, mag: r.mag, gyro: r.gyro, euler: r.euler, quaternion: r.quaternion,
          linear_accel: r.linearAccel, gravity: r.gravity, temperature: r.temperature, calib_stat: r.calibStat,
        }).toEqual(want.decoded);
        expect(r.quaternion!.some((x) => x !== 0)).toBe(true); // fusion live from frame 0
        expect(got.quaternion).toEqual(r.quaternion!.map((x) => x / BNO055_QUAT_LSB));
        if (r.accel !== null) {
          expect(got.accel).toEqual(r.accel.map((x) => x / lsb.accel));
          expect(got.mag).toEqual(r.mag!.map((x) => x / BNO055_MAG_LSB));
          expect(got.gyro).toEqual(r.gyro!.map((x) => x / lsb.gyro));
          expect(got.euler).toEqual(r.euler!.map((x) => x / lsb.euler));
          expect(got.gravity).toEqual(r.gravity!.map((x) => x / BNO055_FUSION_ACCEL_LSB));
          expect(got.temperature).toBe(r.temperature! / lsb.temp);
        } else {
          expect(got.accel).toBeNull();
          expect(got.calibration).toBeNull();
        }
      }
    }, 30_000);
  }
});
