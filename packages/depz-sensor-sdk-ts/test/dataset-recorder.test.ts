/**
 * Software cross-sensor synchronization: DatasetRecorder hooks several live
 * devices onto one shared host timeline via each device's syncTime offset.
 * (Hardware/AUX frame-sync is out of scope; this is the software path.)
 *
 * Scenarios span all FOUR device types — SR04, VL53L8CX, VL53L8CH, BNO086 —
 * including the two ToF classes (CX + CH) on one timeline and same-type
 * repeats (two CX). VL53L8 frames are injected synthetically (the ULD stack is
 * covered elsewhere); the point here is the recorder's multi-device merge.
 */

import { describe, expect, it } from "vitest";
import {
  Bno086,
  DatasetReader,
  DatasetRecorder,
  Sr04,
  Vl53l8Ch,
  Vl53l8Cx,
  type Vl53l8Frame,
} from "../src/index.js";
import { FakeBno086 } from "./fake-bno086.js";
import { FakeSr04 } from "./fake-sr04.js";
import { FakeTof } from "./fake-tof.js";

/** Push a synthetic frame straight into the device's onFrame subscribers —
 * exactly what the read pump's handleReport does, without the ULD/wire. */
function emitFrame(dev: Vl53l8Cx, timestampUs: bigint, resolution = 16): void {
  const n = resolution;
  const frame: Vl53l8Frame = {
    timestampUs,
    resolution,
    distanceMm: Int32Array.from({ length: n }, (_, i) => 100 + i),
    targetStatus: Uint8Array.from({ length: n }, () => 5),
    nbTargetDetected: Uint8Array.from({ length: n }, () => 1),
    signalPerSpad: new Float64Array(n),
    ambientPerSpad: new Float64Array(n),
    nbSpadsEnabled: new Int32Array(n),
    rangeSigmaMm: new Float64Array(n),
    reflectance: new Uint8Array(n),
    siliconTempDegc: 25,
    cnhRaw: null,
    motion: null,
  };
  const d = dev as unknown as {
    frameCbs: Array<(f: Vl53l8Frame) => void>;
    frameQueues: Array<{ push: (f: Vl53l8Frame) => void }>;
  };
  for (const cb of [...d.frameCbs]) cb(frame);
  for (const q of [...d.frameQueues]) q.push(frame);
}

describe("DatasetRecorder cross-sensor sync", () => {
  it("records two devices on a shared host timeline", async () => {
    const fakeA = new FakeSr04();
    const fakeB = new FakeSr04();
    // Wildly divergent device clocks (~1 s vs ~2.5 h uptime): the merge must
    // key on the shared HOST timeline (via each device's syncTime offset), not
    // raw device timestamps.
    fakeB.mcuTimeUs = 9_000_000_000n;
    const a = new Sr04(fakeA.transport, { timeoutMs: 1000 });
    const b = new Sr04(fakeB.transport, { timeoutMs: 1000 });
    await a.open();
    await b.open();

    const rec = new DatasetRecorder({ note: "sync-test" });
    const idA = await rec.add(a, "imu-a");
    const idB = await rec.add(b, "imu-b");
    expect(idA).toBe("imu-a");
    expect(idB).toBe("imu-b");

    rec.start();
    for (let i = 0; i < 3; i++) {
      await fakeA.sendMeasurement(0x37);
      await fakeB.sendMeasurement(0x37);
    }
    // Round-trip each device to guarantee every RPT_DATA was dispatched.
    await a.getSamplePeriodUs();
    await b.getSamplePeriodUs();
    rec.stop();

    expect(rec.recordsWritten).toBe(6);
    const reader = new DatasetReader(rec.dump());

    // Both devices' metadata carries a time_sync offset (the sync anchor).
    expect(reader.devices["imu-a"]!.time_sync).toBeDefined();
    expect(reader.devices["imu-b"]!.time_sync).toBeDefined();
    expect(reader.devices["imu-a"]!.sensor_type).toBe("sr04");

    // Records from both devices, merged in non-decreasing host-time order,
    // despite the ~2.5 h device-clock gap between them.
    const ids = new Set(reader.records.map((r) => r.deviceId));
    expect(ids).toEqual(new Set(["imu-a", "imu-b"]));
    expect(reader.records.every((r) => r.kind === "sr04")).toBe(true);
    const ts = reader.records.map((r) => r.tHostUs);
    expect([...ts].sort((x, y) => x - y)).toEqual(ts);

    await a.close();
    await b.close();
    await fakeA.close();
    await fakeB.close();
  });

  it("spans all four sensor types (SR04 + CX + CH + BNO086) on one timeline", async () => {
    const fSr04 = new FakeSr04();
    const fCx = new FakeTof({ variant: "cx", serial: "SN53CX1" });
    const fCh = new FakeTof({ variant: "ch", serial: "SN53CH1" });
    const fImu = new FakeBno086();

    const sr04 = new Sr04(fSr04.transport, { timeoutMs: 1000 });
    const cx = new Vl53l8Cx(fCx.transport, { timeoutMs: 1000 });
    const ch = new Vl53l8Ch(fCh.transport, { timeoutMs: 1000 });
    const imu = new Bno086(fImu.transport, { timeoutMs: 1000 });
    for (const d of [sr04, cx, ch, imu]) await d.open();

    const rec = new DatasetRecorder({ note: "four-types" });
    await rec.add(sr04, "range");
    await rec.add(cx, "tof-cx");
    await rec.add(ch, "tof-ch");
    await rec.add(imu, "imu");

    rec.start();
    // SR04 measurements + synthetic CX and CH frames, interleaved.
    for (let i = 0; i < 2; i++) {
      await fSr04.sendMeasurement(0x37);
      emitFrame(cx, 2_000_000n + BigInt(i) * 1000n);
      emitFrame(ch, 2_000_000n + BigInt(i) * 1000n, 64);
    }
    await sr04.getSamplePeriodUs(); // flush the SR04 RPT_DATA dispatch
    rec.stop();

    const reader = new DatasetReader(rec.dump());

    // All four device families appear in the metadata (both ToF as vl53l8).
    expect(reader.devices["range"]!.sensor_type).toBe("sr04");
    expect(reader.devices["tof-cx"]!.sensor_type).toBe("vl53l8");
    expect(reader.devices["tof-ch"]!.sensor_type).toBe("vl53l8");
    expect(reader.devices["imu"]!.sensor_type).toBe("bno086");
    for (const id of ["range", "tof-cx", "tof-ch", "imu"]) {
      expect(reader.devices[id]!.time_sync).toBeDefined();
    }

    // SR04 (2) + CX (2) + CH (2) all land on one merged, host-sorted timeline;
    // BNO086 contributes metadata only (no onFrame/onMeasurement hook).
    expect(reader.records.length).toBe(6);
    const byId = new Map<string, number>();
    for (const r of reader.records) byId.set(r.deviceId, (byId.get(r.deviceId) ?? 0) + 1);
    expect(byId.get("range")).toBe(2);
    expect(byId.get("tof-cx")).toBe(2); // CX and CH recorded simultaneously
    expect(byId.get("tof-ch")).toBe(2);
    expect(byId.has("imu")).toBe(false);
    expect(reader.records.filter((r) => r.kind === "vl53l8").length).toBe(4);
    const ts = reader.records.map((r) => r.tHostUs);
    expect([...ts].sort((x, y) => x - y)).toEqual(ts);

    for (const d of [sr04, cx, ch, imu]) await d.close();
    await fSr04.close();
    await fCx.close();
    await fCh.close();
    await fImu.close();
  });

  it("records same-type repeats (two CX) alongside a CH on one timeline", async () => {
    const fCxA = new FakeTof({ variant: "cx", serial: "SN53CXA" });
    const fCxB = new FakeTof({ variant: "cx", serial: "SN53CXB" });
    const fCh = new FakeTof({ variant: "ch", serial: "SN53CHX" });
    const cxA = new Vl53l8Cx(fCxA.transport, { timeoutMs: 1000 });
    const cxB = new Vl53l8Cx(fCxB.transport, { timeoutMs: 1000 });
    const ch = new Vl53l8Ch(fCh.transport, { timeoutMs: 1000 });
    for (const d of [cxA, cxB, ch]) await d.open();

    const rec = new DatasetRecorder({ note: "two-cx-plus-ch" });
    await rec.add(cxA, "cx-a");
    await rec.add(cxB, "cx-b");
    await rec.add(ch, "ch");

    rec.start();
    for (let i = 0; i < 2; i++) {
      const t = 3_000_000n + BigInt(i) * 1000n;
      emitFrame(cxA, t);
      emitFrame(cxB, t);
      emitFrame(ch, t, 64);
    }
    rec.stop();

    const reader = new DatasetReader(rec.dump());
    expect(reader.records.length).toBe(6);
    const ids = new Set(reader.records.map((r) => r.deviceId));
    expect(ids).toEqual(new Set(["cx-a", "cx-b", "ch"]));
    // Two same-type CX devices are told apart only by their dataset id/serial.
    expect(reader.devices["cx-a"]!.serial).toBe("SN53CXA");
    expect(reader.devices["cx-b"]!.serial).toBe("SN53CXB");
    expect(reader.records.every((r) => r.kind === "vl53l8")).toBe(true);

    for (const d of [cxA, cxB, ch]) await d.close();
    await fCxA.close();
    await fCxB.close();
    await fCh.close();
  });
});
