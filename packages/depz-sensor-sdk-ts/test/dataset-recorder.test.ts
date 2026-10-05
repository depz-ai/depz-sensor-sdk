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
  Vl53l8ch,
  Vl53l8cx,
  type Vl53l8Frame,
} from "../src/index.js";
import { FakeBno086 } from "./fake-bno086.js";
import { FakeSr04 } from "./fake-sr04.js";
import { FakeTof } from "./fake-tof.js";

/** Push a synthetic frame straight into the device's onFrame subscribers —
 * exactly what the read pump's handleReport does, without the ULD/wire. */
function emitFrame(dev: Vl53l8cx, timestampUs: bigint, resolution = 16): void {
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
    const cx = new Vl53l8cx(fCx.transport, { timeoutMs: 1000 });
    const ch = new Vl53l8ch(fCh.transport, { timeoutMs: 1000 });
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
    // the BNO086 has no sensor enabled here, so it contributes metadata only.
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
    const cxA = new Vl53l8cx(fCxA.transport, { timeoutMs: 1000 });
    const cxB = new Vl53l8cx(fCxB.transport, { timeoutMs: 1000 });
    const ch = new Vl53l8ch(fCh.transport, { timeoutMs: 1000 });
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

describe("DatasetRecorder — contract 09 kinds by device class", () => {
  it("writes vl53l4 / vl53lx / bno055 records, never sr04 for a ToF", async () => {
    const { Bno055, Vl53l1cx, Vl53l4cd, decodeBno055Sample, BNO055_FULL_BLOCK, BNO055_DEFAULT_UNITS } =
      await import("../src/index.js");
    const fakes = [new FakeTof(), new FakeTof(), new FakeTof()];
    const l4 = new Vl53l4cd(fakes[0]!.transport, { timeoutMs: 1000 });
    const lx = new Vl53l1cx(fakes[1]!.transport, { timeoutMs: 1000 });
    const imu = new Bno055(fakes[2]!.transport, { timeoutMs: 1000 });
    for (const d of [l4, lx, imu]) await d.open();

    const rec = new DatasetRecorder();
    for (const d of [l4, lx, imu]) await rec.add(d);
    rec.start();
    type Cbs<T> = { measureCbs?: Array<(m: T) => void>; sampleCbs?: Array<(s: T) => void> };
    for (const cb of (l4 as unknown as Cbs<unknown>).measureCbs!)
      cb({ timestampUs: 2_000_000n, rangeStatus: 0, distanceMm: 505, sigmaMm: 2, signalRateKcps: 900,
           ambientRateKcps: 3, signalPerSpadKcps: 0, ambientPerSpadKcps: 0, numberOfSpad: 12,
           streamCount: 7, valid: true, statusText: "Range valid" });
    for (const cb of (lx as unknown as Cbs<unknown>).measureCbs!)
      cb({ timestampUs: 2_100_000n, distanceMm: 612, status: 0, statusText: "Range valid", signalKcps: 812.5,
           ambientKcps: 3.1, sigmaMm: 4.2, spads: 12.5, extra: {}, bins: null, valid: true, plottable: true,
           targets: [{ distanceMm: 612, status: 0, statusText: "", signalKcps: 812.5, ambientKcps: 3.1,
                       sigmaMm: 4.2, minRangeMm: 600, maxRangeMm: 625 }] });
    const block = Uint8Array.from(Buffer.from(
      "c5ffb7ff9a032f00e2ffadfefeffffff0000cd046a0578e6b52963f9efcf00000000fbff0600b1ff0cfeb6fe1b0133", "hex"));
    const s = decodeBno055Sample(3_000_000n, BNO055_FULL_BLOCK[0], block, { ...BNO055_DEFAULT_UNITS, eulerRad: true });
    for (const cb of (imu as unknown as Cbs<unknown>).sampleCbs!) cb(s);
    rec.stop();
    for (const d of [l4, lx, imu]) await d.close();

    const kinds = Object.fromEntries(new DatasetReader(rec.dump()).records.map((r) => [r.kind, r.value]));
    expect(Object.keys(kinds).sort()).toEqual(["bno055", "vl53l4", "vl53lx"]);
    expect(kinds.vl53l4).toMatchObject({ range_status: 0, distance_mm: 505, stream_count: 7 });
    expect(kinds.vl53lx).toMatchObject({ status: 0, distance_mm: 612,
      targets: [{ distance_mm: 612, status: 0, signal_kcps: 812.5 }] });
    const b = kinds.bno055 as Record<string, unknown>;
    expect(b.unit_sel).toBe(0x04);
    expect(b.quaternion).toEqual(s.quaternion);
    expect(b.calib).toEqual([s.calibration!.system, s.calibration!.gyro, s.calibration!.accel, s.calibration!.mag]);
  });

  it("writes bno086 reports: snake_case, full components, host-time t, device timestamp_us", async () => {
    const fImu = new FakeBno086();
    const imu = new Bno086(fImu.transport, { timeoutMs: 1000 });
    await imu.open();
    const rec = new DatasetRecorder();
    await rec.add(imu);
    rec.start();
    // One cargo, device times out of order: the records stay monotonic in t.
    const rv = { type: "RotationVector", sensorId: 0x05, timestampUs: 5_000_300n, seq: 7, accuracy: 3,
                 delayUs: 0, iRaw: 8192, jRaw: 0, kRaw: 0, realRaw: 14189, i: 0.5, j: 0, k: 0,
                 real: 0.866, accuracyRaw: 820, accuracyRad: 0.2 };
    const acc = { type: "Acceleration", sensorId: 0x01, timestampUs: 5_000_100n, seq: 8, accuracy: 2,
                  delayUs: 0, xRaw: 0, yRaw: 0, zRaw: 2511, x: 0, y: 0, z: 9.81 };
    type Cbs = { reportCbs: Array<{ cb: (r: unknown) => void }> };
    for (const r of [rv, acc]) for (const { cb } of (imu as unknown as Cbs).reportCbs) cb(r);
    rec.stop();
    await imu.close();
    await fImu.close();

    const recs = new DatasetReader(rec.dump()).records.filter((r) => r.kind === "bno086");
    expect(recs.map((r) => r.value.type)).toEqual(["RotationVector", "Acceleration"]);
    expect(recs[0]!.tHostUs).toBeLessThanOrEqual(recs[1]!.tHostUs);
    expect(recs[0]!.value).toEqual({ type: "RotationVector", sensor_id: 0x05, i: 0.5, j: 0, k: 0, real: 0.866,
                                     accuracy_rad: 0.2, timestamp_us: 5_000_300 });
    expect(recs[1]!.value).toEqual({ type: "Acceleration", sensor_id: 0x01, x: 0, y: 0, z: 9.81,
                                     timestamp_us: 5_000_100 });
  });
});
