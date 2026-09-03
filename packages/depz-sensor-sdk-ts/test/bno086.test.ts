/**
 * Bno086 device behavior against the in-process fake bridge + sensor hub.
 * Port of the Python `tests/test_bno086.py`.
 */

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  Bno086,
  BusyError,
  DepzError,
  FrsRecordId,
  OscillatorType,
  SensorId,
  Sh2Error,
  TareAxis,
  TareBasis,
  buildSetFeature,
  type Acceleration,
  type GyroIntegratedRV,
  type Magnetometer,
  type Report,
  type RotationVector,
  type StepCounter,
} from "../src/index.js";
import { toHex } from "./golden/vectors.js";
import { FakeBno086 } from "./fake-bno086.js";

let fake: FakeBno086;
let dev: Bno086;

beforeEach(async () => {
  fake = new FakeBno086();
  dev = new Bno086(fake.transport, { timeoutMs: 1000 });
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fake.close();
  vi.restoreAllMocks();
});

/** SH-2 input report: 4-byte header + body. */
function report(
  rid: number,
  body: Uint8Array,
  opts: { seq?: number; accuracy?: number; delay?: number } = {},
): Uint8Array {
  const { seq = 0, accuracy = 0, delay = 0 } = opts;
  const status = (accuracy & 0x03) | ((delay >> 8) << 2);
  const out = new Uint8Array(4 + body.length);
  out[0] = rid;
  out[1] = seq;
  out[2] = status;
  out[3] = delay & 0xff;
  out.set(body, 4);
  return out;
}

function timebase(delta100us: number): Uint8Array {
  const out = new Uint8Array(5);
  const v = new DataView(out.buffer);
  v.setUint8(0, 0xfb);
  v.setInt32(1, delta100us, true);
  return out;
}

/** Pack int16 LE values. */
function i16s(...vals: number[]): Uint8Array {
  const out = new Uint8Array(vals.length * 2);
  const v = new DataView(out.buffer);
  vals.forEach((x, i) => v.setInt16(i * 2, x, true));
  return out;
}

function concat(...parts: Uint8Array[]): Uint8Array {
  const out = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
  let off = 0;
  for (const p of parts) {
    out.set(p, off);
    off += p.length;
  }
  return out;
}

async function waitFor(cond: () => boolean, timeoutMs = 1000): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  while (!cond()) {
    if (Date.now() > deadline) throw new Error("waitFor timed out");
    await new Promise((resolve) => setTimeout(resolve, 2));
  }
}

async function take(it: AsyncIterableIterator<Report>): Promise<Report> {
  const { value, done } = await it.next();
  expect(done).toBe(false);
  return value as Report;
}

// ── identity / lifecycle ──────────────────────────────────────────────────────

describe("identity and lifecycle", () => {
  it("identity and product id", async () => {
    expect(await dev.getSoftwareName()).toBe(FakeBno086.SOFTWARE_NAME);
    const pid = await dev.productId();
    expect([pid.swVersionMajor, pid.swVersionMinor, pid.swVersionPatch]).toEqual([3, 8, 4]);
    expect(pid.version).toBe("3.8.4");
  });

  it("hardware reset waits for reset-complete", async () => {
    await dev.hardwareReset(1000);
    expect(toHex(dev.advertisement)).toBe(toHex(FakeBno086.ADVERTISEMENT));
    // SHTP TX seq restarted from 0 after reset
    expect((dev as any).shtp.txSeq(2)).toBe(0);
  });

  it("wake does not raise", async () => {
    await dev.wake(); // RPT_STATUS OK
  });
});

// ── enable / feature flow ─────────────────────────────────────────────────────

describe("enable / feature flow", () => {
  it("enable sends set-feature bytes and verifies", async () => {
    const resp = await dev.enable(SensorId.RotationVector, 100);
    expect(fake.lastSetFeature).not.toBeNull();
    expect(toHex(fake.lastSetFeature!)).toBe(toHex(buildSetFeature(SensorId.RotationVector, 10_000)));
    expect(resp).not.toBeNull();
    expect(resp!.intervalUs).toBe(10_000);
    expect(fake.features.get(SensorId.RotationVector)).toBe(10_000);
  });

  it("enable warns when the granted rate is out of band", async () => {
    const warn = vi.spyOn(console, "warn").mockImplementation(() => undefined);
    fake.intervalFactor = 4.0; // granted rate = requested/4 < 0.9x
    await dev.enable(SensorId.Accelerometer, 100);
    expect(warn).toHaveBeenCalledTimes(1);
    expect(String(warn.mock.calls[0]![0])).toContain("granted");
  });

  it("enable grid rounding within band emits no warning", async () => {
    const warn = vi.spyOn(console, "warn").mockImplementation(() => undefined);
    fake.intervalFactor = 0.8; // granted 125 Hz for a 100 Hz ask — in band
    await dev.enable(SensorId.Gyroscope, 100);
    expect(warn).not.toHaveBeenCalled();
  });

  it("disable clears the feature", async () => {
    await dev.enable(SensorId.Magnetometer, 50);
    await dev.disable(SensorId.Magnetometer);
    await waitFor(() => !fake.features.has(SensorId.Magnetometer));
  });

  it("enable argument validation", async () => {
    await expect(dev.enable(SensorId.Accelerometer)).rejects.toThrow(DepzError);
    await expect(
      dev.enable(SensorId.Accelerometer, 100, { intervalUs: 10_000 }),
    ).rejects.toThrow(DepzError);
  });
});

// ── busy / backoff ────────────────────────────────────────────────────────────

describe("busy / backoff", () => {
  it("busy then retry succeeds after backing off", async () => {
    fake.busyRemaining = 1;
    dev.busyBackoffMs = 40; // keep the test fast; timing still observable
    const t0 = performance.now();
    await dev.enable(SensorId.Accelerometer, 100, { verify: false });
    expect(performance.now() - t0).toBeGreaterThanOrEqual(dev.busyBackoffMs - 5);
    expect(fake.sendShtpAttempts).toBe(2);
    expect(fake.features.get(SensorId.Accelerometer)).toBe(10_000);
  });

  it("busy exhausts the bounded retries", async () => {
    fake.busyRemaining = 99;
    dev.busyRetries = 3;
    dev.busyBackoffMs = 5; // keep the test fast
    await expect(dev.enable(SensorId.Accelerometer, 100, { verify: false })).rejects.toThrow(
      BusyError,
    );
    expect(fake.sendShtpAttempts).toBe(3);
  });
});

// ── report streaming through the full stack ───────────────────────────────────

describe("report streaming", () => {
  it("input reports parse through the stack", async () => {
    const it_ = dev.reports(undefined, 16);
    const cargo = concat(
      timebase(120),
      report(0x01, i16s(256, -512, 2521), { seq: 7, accuracy: 2 }),
      report(0x05, i16s(100, -200, 300, 16000, 50), { seq: 8, accuracy: 3, delay: 17 }),
    );
    const capture = fake.pushInputCargo(cargo);
    const acc = (await take(it_)) as Acceleration;
    const rv = (await take(it_)) as RotationVector;
    expect(acc.type).toBe("Acceleration");
    expect(acc.sensorId).toBe(SensorId.Accelerometer);
    expect([acc.xRaw, acc.yRaw, acc.zRaw]).toEqual([256, -512, 2521]);
    expect(acc.x).toBeCloseTo(1.0); // 256 / 2^8 m/s²
    expect(acc.z).toBeCloseTo(2521 / 256);
    expect(acc.accuracy).toBe(2);
    expect(acc.seq).toBe(7);
    expect(acc.timestampUs).toBe(capture - 120n * 100n); // timebase applied
    expect(rv.type).toBe("RotationVector");
    expect(rv.real).toBeCloseTo(16000 / 16384);
    expect(rv.accuracyRad).toBeCloseTo(50 / 4096);
    expect(rv.timestampUs).toBe(capture - 120n * 100n + 17n * 100n); // + report delay
  });

  it("multi-frame cargo reassembly through the stack", async () => {
    const it_ = dev.reports(undefined, 16);
    const parts = [timebase(0)];
    for (let i = 0; i < 8; i++) {
      parts.push(report(0x03, i16s(160 + i, -160, 42), { seq: i }));
    }
    fake.pushInputCargo(concat(...parts), 32); // forces 3+ SHTP fragments
    const got: Magnetometer[] = [];
    for (let i = 0; i < 8; i++) got.push((await take(it_)) as Magnetometer);
    expect(got.every((m) => m.type === "Magnetometer")).toBe(true);
    expect(got.map((m) => m.xRaw)).toEqual([160, 161, 162, 163, 164, 165, 166, 167]);
    expect(got[0]!.x).toBeCloseTo(10.0); // 160 / 2^4 µT
  });

  it("report filtering and callbacks", async () => {
    const onlySteps = dev.reports(SensorId.StepCounter, 8);
    const seen: StepCounter[] = [];
    const unsub = dev.onReport((r) => seen.push(r as StepCounter), [SensorId.StepCounter]);
    const stepBody = new Uint8Array(8);
    const sv = new DataView(stepBody.buffer);
    sv.setUint32(0, 5000, true);
    sv.setUint16(4, 1234, true);
    const cargo = concat(timebase(0), report(0x01, i16s(1, 2, 3)), report(0x11, stepBody));
    fake.pushInputCargo(cargo);
    const step = (await take(onlySteps)) as StepCounter;
    expect(step.type).toBe("StepCounter");
    expect(step.steps).toBe(1234);
    expect(step.latencyUs).toBe(5000);
    expect(seen.length).toBe(1);
    expect(seen[0]!.steps).toBe(1234);
    unsub();
  });

  it("gyro-integrated RV on channel 5", async () => {
    const it_ = dev.reports(SensorId.GyroIntegratedRv, 8);
    const capture = fake.pushGyroRvCargo(i16s(1, 2, 3, 16384, 512, -512, 1024));
    const r = (await take(it_)) as GyroIntegratedRV;
    expect(r.type).toBe("GyroIntegratedRV");
    expect(r.real).toBeCloseTo(1.0);
    expect(r.angularVelocity[0]).toBeCloseTo(0.5); // Q10 rad/s
    expect(r.angularVelocity[1]).toBeCloseTo(-0.5);
    expect(r.angularVelocity[2]).toBeCloseTo(1.0);
    expect(r.timestampUs).toBe(capture);
  });
});

// ── tare / calibration ────────────────────────────────────────────────────────

describe("tare / calibration facade", () => {
  it("tare and calibration flow", async () => {
    await dev.tareNow(TareAxis.All, TareBasis.GameRotationVector);
    await dev.saveDcd(); // waits for command response, status 0
    await waitFor(() => fake.tareRequests.length === 1);
    const tare = fake.tareRequests[0]!;
    expect(tare[2]).toBe(0x03); // TARE command
    expect(Array.from(tare.subarray(3, 6))).toEqual([0x00, TareAxis.All, TareBasis.GameRotationVector]);
    await dev.setCalibration({ accel: true, gyro: true, mag: false });
    const cal = await dev.getCalibration();
    expect(cal).toEqual({ accel: true, gyro: true, mag: false, planar: false });
  });
});

// ── FRS ───────────────────────────────────────────────────────────────────────

describe("FRS", () => {
  it("multi-packet FRS read", async () => {
    const words = await dev.frsRead(FrsRecordId.SystemOrientation);
    expect(words).toEqual(FakeBno086.FRS_RECORDS[0x2d3e]);
  });

  it("unknown FRS record rejects", async () => {
    await expect(dev.frsRead(0xbeef)).rejects.toThrow(Sh2Error);
    await expect(dev.frsRead(0xbeef)).rejects.toThrow(/Unrecognized/i);
  });

  it("FRS write roundtrip", async () => {
    const payload = [0x11111111, 0x22222222, 0x33333333];
    await dev.frsWrite(FrsRecordId.SystemOrientation, payload);
    expect(fake.frsWrites.get(FrsRecordId.SystemOrientation)).toEqual(payload);
  });

  it("getMetadata parses the accelerometer record", async () => {
    const md = await dev.getMetadata(SensorId.Accelerometer);
    expect(md.revision).toBe(4);
    expect(md.minPeriodUs).toBe(2500);
    expect(md.maxPeriodUs).toBe(100000);
    expect(md.qPoint1).toBe(8);
    expect(md.rawWords).toEqual(FakeBno086.FRS_RECORDS[0xe302]);
  });
});

// ── diagnostics / housekeeping ────────────────────────────────────────────────

describe("diagnostics", () => {
  it("persistTare and setReorientation issue tare subcommands", async () => {
    await dev.persistTare(); // subcommand 1, no response
    await dev.setReorientation(0, 0, 0, 1); // subcommand 2, no response
    await waitFor(() => fake.tareRequests.length === 2);
    const [persist, reorient] = fake.tareRequests;
    expect(persist![2]).toBe(0x03); // TARE command
    expect(persist![3]).toBe(0x01); // persist subcommand
    expect(reorient![2]).toBe(0x03);
    expect(reorient![3]).toBe(0x02); // set-reorientation subcommand
    // command request: [0xF2, seq, 0x03, subcmd, x(i16), y, z, w] — w = 1.0 → Q14.
    const w = new DataView(reorient!.buffer, reorient!.byteOffset).getInt16(10, true);
    expect(w).toBe(1 << 14);
  });

  it("configurePeriodicDcd sends without expecting a response", async () => {
    await dev.configurePeriodicDcd(true); // command 0x09, no response — must not hang
  });

  it("getOscillatorType reads r[0]", async () => {
    expect(await dev.getOscillatorType()).toBe(OscillatorType.ExtCrystal);
  });

  it("getErrors streams records until the 255 terminator", async () => {
    const errors = await dev.getErrors();
    expect(errors).toHaveLength(2); // terminator dropped
    expect(errors[0]).toEqual({ severity: 1, seq: 0, source: 3, error: 0x10, module: 2, code: 5 });
    expect(errors[1]!.source).toBe(4); // Chip
    expect(errors[1]!.error).toBe(0x20);
  });

  it("getCounts merges the two-message response", async () => {
    const counts = await dev.getCounts(SensorId.RotationVector);
    expect(counts).toEqual({
      sensorId: SensorId.RotationVector,
      offered: 100,
      accepted: 90,
      on: 80,
      attempted: 70,
    });
  });

  it("clearCounts succeeds on status 0", async () => {
    await dev.clearCounts(SensorId.RotationVector); // status 0 → resolves
  });

  it("clearDcdAndReset re-runs advertisement and restarts SHTP seq", async () => {
    await dev.clearDcdAndReset(1000);
    expect(toHex(dev.advertisement)).toBe(toHex(FakeBno086.ADVERTISEMENT));
    expect((dev as unknown as { shtp: { txSeq(c: number): number } }).shtp.txSeq(2)).toBe(0);
  });
});

// ── two of a kind (same sensor type, two links) ───────────────────────────────

describe("two BNO086 devices at once", () => {
  it("keeps each device's report stream isolated", async () => {
    const fakeB = new FakeBno086();
    const devB = new Bno086(fakeB.transport, { timeoutMs: 1000 });
    await devB.open();
    try {
      const itA = dev.reports(undefined, 16);
      const itB = devB.reports(undefined, 16);
      // Distinct magnetometer x on each link.
      fake.pushInputCargo(concat(timebase(0), report(0x03, i16s(160, 0, 0))));
      fakeB.pushInputCargo(concat(timebase(0), report(0x03, i16s(320, 0, 0))));
      const a = (await take(itA)) as Magnetometer;
      const b = (await take(itB)) as Magnetometer;
      expect(a.xRaw).toBe(160);
      expect(b.xRaw).toBe(320);
    } finally {
      await devB.close();
      await fakeB.close();
    }
  });
});
