/**
 * DepzDevice/Sr04 behavior against the in-process fake firmware.
 * Port of the Python `tests/test_device_core.py`.
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import {
  Bno086,
  BusyError,
  DepzTimeoutError,
  LoopbackTransport,
  Sr04,
  StatusError,
  SyncPinMode,
  SyncPinPolarity,
  Vl53l8Ch,
  Vl53l8Cx,
  syncTimeAll,
  type DeviceEvent,
  type Sr04Measurement,
} from "../src/index.js";
import { FakeBno086 } from "./fake-bno086.js";
import { FakeSr04 } from "./fake-sr04.js";
import { FakeTof } from "./fake-tof.js";

let fake: FakeSr04;
let dev: Sr04;

beforeEach(async () => {
  fake = new FakeSr04();
  dev = new Sr04(fake.transport, { timeoutMs: 1000 });
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fake.close();
});

describe("DepzDevice core", () => {
  it("identity roundtrip", async () => {
    expect(await dev.getSoftwareName()).toBe(FakeSr04.SOFTWARE_NAME);
    expect(await dev.getDeviceName()).toBe(FakeSr04.DEVICE_NAME);
    expect(await dev.getSerialNumber()).toBe(FakeSr04.SERIAL);
  });

  it("identify() parses the software name", async () => {
    const ident = await dev.identify();
    expect(ident.mode).toBe("app");
    expect(ident.sensorType).toBe("sr04");
    expect(ident.version).toBe("0.95");
  });

  it("temperature", async () => {
    expect(await dev.readMcuTemperature()).toBeCloseTo(27.3);
  });

  it("syncTime produces an offset", async () => {
    const ts = await dev.syncTime(3);
    expect(ts.rttUs >= 0n).toBe(true);
    expect(dev.timeSync).toBe(ts);
    // toHostTimeUs inverts the offset
    expect(dev.toHostTimeUs(1000n + ts.offsetUs)).toBe(1000n);
  });

  it("syncTimeAll syncs every device and maps device → TimeSync", async () => {
    const fake2 = new FakeSr04();
    const dev2 = new Sr04(fake2.transport, { timeoutMs: 1000 });
    await dev2.open();
    try {
      const result = await syncTimeAll([dev, dev2], 2);
      expect(result.size).toBe(2);
      expect(result.get(dev)).toBe(dev.timeSync);
      expect(result.get(dev2)).toBe(dev2.timeSync);
      expect(result.get(dev)!.rttUs >= 0n).toBe(true);
    } finally {
      await dev2.close();
      await fake2.close();
    }
  });

  it("syncTimeAll spans all four sensor types (SR04, CX, CH, BNO086)", async () => {
    const fCx = new FakeTof({ variant: "cx" });
    const fCh = new FakeTof({ variant: "ch" });
    const fImu = new FakeBno086();
    const cx = new Vl53l8Cx(fCx.transport, { timeoutMs: 1000 });
    const ch = new Vl53l8Ch(fCh.transport, { timeoutMs: 1000 });
    const imu = new Bno086(fImu.transport, { timeoutMs: 1000 });
    await cx.open();
    await ch.open();
    await imu.open();
    try {
      // `dev` is the SR04 from beforeEach — one of every family, CX and CH both.
      const all = [dev, cx, ch, imu];
      const result = await syncTimeAll(all, 2);
      expect(result.size).toBe(4);
      for (const d of all) {
        expect(result.get(d)).toBe(d.timeSync);
        expect(result.get(d)!.rttUs >= 0n).toBe(true);
      }
      // The two ToF classes are distinct instances on the shared clock.
      expect(cx).toBeInstanceOf(Vl53l8Cx);
      expect(ch).toBeInstanceOf(Vl53l8Ch);
      expect(result.get(cx)).not.toBe(result.get(ch));
    } finally {
      await cx.close();
      await ch.close();
      await imu.close();
      await fCx.close();
      await fCh.close();
      await fImu.close();
    }
  });

  it("unknown cmd rejects with StatusError", async () => {
    const err: unknown = await dev
      .request(0x2a, undefined, { okCompletes: true })
      .catch((e: unknown) => e);
    expect(err).toBeInstanceOf(StatusError);
    expect((err as StatusError).statusName).toBe("ERR_INVALID_CMD");
  });

  it("sync pin validation", async () => {
    await dev.setSyncPin({ pin: 1, mode: SyncPinMode.OutBoth, polarity: SyncPinPolarity.IdleLow });
    await expect(
      dev.setSyncPin({ pin: 9, mode: SyncPinMode.In, polarity: SyncPinPolarity.IdleLow }),
    ).rejects.toThrow(StatusError);
    const cfg = await dev.getSyncPin(2);
    expect(cfg.pin).toBe(2);
    expect(cfg.mode).toBe(SyncPinMode.Disable);
  });

  it("one in-flight request per opcode", async () => {
    const first = dev.getDeviceName();
    await expect(dev.getDeviceName()).rejects.toThrow(BusyError);
    expect(await first).toBe(FakeSr04.DEVICE_NAME);
  });

  it("times out when the device is silent", async () => {
    const [host] = LoopbackTransport.pair(); // nobody answers on the far side
    const silent = new Sr04(host, { timeoutMs: 50 });
    await silent.open();
    try {
      await expect(silent.getSoftwareName()).rejects.toThrow(DepzTimeoutError);
    } finally {
      await silent.close();
    }
  });

  it("emits disconnected and updates stats on link loss", async () => {
    const events: DeviceEvent[] = [];
    dev.onEvent((ev) => events.push(ev));
    await dev.getSoftwareName();
    expect(dev.stats.rxPackets).toBeGreaterThan(0);
    expect(dev.stats.txPackets).toBeGreaterThan(0);
    await fake.close(); // loopback close tears down both sides
    await new Promise((r) => setTimeout(r, 10));
    expect(events.some((ev) => ev.type === "disconnected")).toBe(true);
  });
});

describe("Sr04", () => {
  it("config roundtrip", async () => {
    expect(await dev.getSamplePeriodUs()).toBe(50_000);
    await dev.setSamplePeriodUs(10_000);
    expect(fake.samplePeriodUs).toBe(10_000);
    expect(await dev.getSamplePeriodUs()).toBe(10_000);
  });

  it("echo decay clamp re-read", async () => {
    expect(await dev.setEchoDecayUs(1000)).toBe(4000); // device clamps to min
    expect(await dev.setEchoDecayUs(7000)).toBe(7000);
  });

  it("measureOnce", async () => {
    const m = await dev.measureOnce();
    expect(m.source).toBe("once");
    expect(m.echoTimeUs).toBe(fake.echoTimeUs);
    expect(m.valid).toBe(true);
    expect(m.distanceMm).toBeCloseTo((5831 * 343) / 2000);
  });

  it("measureOnce is busy during the loop", async () => {
    await dev.start();
    await expect(dev.measureOnce()).rejects.toThrow(BusyError);
    await dev.stop();
  });

  it("timeout sentinel", async () => {
    fake.echoTimeUs = 0xffff;
    const m = await dev.measureOnce();
    expect(m.valid).toBe(false);
    expect(m.distanceMm).toBeNull();
  });

  it("stream and callbacks", async () => {
    const seen: Sr04Measurement[] = [];
    const unsub = dev.onMeasurement((m) => seen.push(m));
    await dev.start();
    const it = dev.measurements(16);
    for (let i = 0; i < 3; i++) await fake.sendMeasurement(0x37);
    const got: Sr04Measurement[] = [];
    for (let i = 0; i < 3; i++) got.push((await it.next()).value as Sr04Measurement);
    expect(got.every((m) => m.source === "loop")).toBe(true);
    expect(seen).toHaveLength(3);
    unsub();
    await fake.sendMeasurement(0x37);
    expect(((await it.next()).value as Sr04Measurement).source).toBe("loop");
    expect(seen).toHaveLength(3); // callback unsubscribed
    await dev.stop();
  });

  it("SYNC_IN single shot goes to the stream", async () => {
    const it = dev.measurements(4);
    await fake.sendMeasurement(0x36); // unsolicited single shot (SYNC_IN edge)
    expect(((await it.next()).value as Sr04Measurement).source).toBe("once");
  });

  it("stream is bounded, drop-oldest, with a visible droppedCount", async () => {
    const it = dev.measurements(2);
    for (let i = 0; i < 4; i++) await fake.sendMeasurement(0x36);
    // A request round-trip guarantees all four reports were dispatched.
    await dev.getSamplePeriodUs();
    expect(it.droppedCount).toBe(2);
    expect(dev.streamDroppedCounts).toEqual([2]);
    const a = (await it.next()).value as Sr04Measurement;
    const b = (await it.next()).value as Sr04Measurement;
    expect(b.timestampUs - a.timestampUs).toBe(BigInt(fake.samplePeriodUs));
  });

  it("stream ends when the device closes", async () => {
    const it = dev.measurements(4);
    await dev.close();
    expect((await it.next()).done).toBe(true);
  });

  it("two SR04 over two loopbacks stream independently", async () => {
    const fake2 = new FakeSr04();
    const dev2 = new Sr04(fake2.transport, { timeoutMs: 1000 });
    await dev2.open();
    fake.echoTimeUs = 1000;
    fake2.echoTimeUs = 8000;
    try {
      await dev.start();
      await dev2.start();
      const itA = dev.measurements(2); // small → drop-oldest observable
      const itB = dev2.measurements(16);
      for (let i = 0; i < 4; i++) await fake.sendMeasurement(0x37);
      for (let i = 0; i < 3; i++) await fake2.sendMeasurement(0x37);
      // Round-trip both to guarantee all reports were dispatched.
      await dev.getSamplePeriodUs();
      await dev2.getSamplePeriodUs();
      // Each device only saw its own echoes; drop counters are per device.
      expect(itA.droppedCount).toBe(2); // 4 pushed into a size-2 queue
      expect(itB.droppedCount).toBe(0);
      expect(((await itA.next()).value as Sr04Measurement).echoTimeUs).toBe(1000);
      expect(((await itB.next()).value as Sr04Measurement).echoTimeUs).toBe(8000);
      await dev.stop();
      await dev2.stop();
    } finally {
      await dev2.close();
      await fake2.close();
    }
  });
});
