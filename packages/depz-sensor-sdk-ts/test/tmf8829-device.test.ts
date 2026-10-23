/**
 * Tmf8829 device class against an in-process fake bridge + sensor
 * (test/fake-tmf8829.ts): the host sensor sequence of contract 14 §4 on the
 * wire (init / configure / start / stop), the refusals and sensor error
 * statuses, and the streamed frames → measurements path checked against the
 * golden measurement vectors (contracts/vectors/tmf8829.json).
 */

import { createHash } from "node:crypto";
import { afterEach, beforeEach, describe, expect, it } from "vitest";

import {
  DepzError,
  DepzTimeoutError,
  StatusError,
  Tmf8829,
  Tmf8829Error,
  Tmf8829EventCode,
  tmf8829FirmwareImage,
  type Tmf8829Event,
  type Tmf8829Measurement,
} from "../src/index.js";
import { FakeTmf8829 } from "./fake-tmf8829.js";
import { fromHex, loadVectors } from "./golden/vectors.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;
const V: Any = loadVectors("tmf8829.json");
const RAW = new Map<string, Uint8Array>(V.frames.map((c: Any) => [c.name, fromHex(c.raw)]));
const raw = (name: string): Uint8Array => RAW.get(name)!;
const MEAS = (name: string): Any => V.measurements.find((m: Any) => m.name === name);

const W = 0x33; // TMF_WRITE_REG
const R = 0x32; // TMF_READ_REG

let fake: FakeTmf8829;
let dev: Tmf8829;

beforeEach(async () => {
  fake = new FakeTmf8829();
  dev = new Tmf8829(fake.transport, { timeoutMs: 500, sleepImpl: async () => {} });
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fake.close();
});

const tick = (ms = 20): Promise<void> => new Promise((r) => setTimeout(r, ms));

/** Wait until `n` measurements arrived on `out` (or fail after 2 s). */
async function waitFor<T>(out: T[], n: number): Promise<void> {
  for (let i = 0; i < 200 && out.length < n; i++) await tick(10);
  expect(out.length).toBe(n);
}

/** Stream a measurement vector's frames through the fake. */
async function playVector(name: string): Promise<void> {
  for (const f of MEAS(name).frames) {
    await fake.emitFrame(raw(f.raw_case), BigInt(f.timestamp_us), BigInt(f.edge_us));
  }
}

function checkAgainstVector(m: Tmf8829Measurement, e: Any): void {
  expect(m.timestampUs).toBe(e.timestamp_us);
  expect(m.flags).toBe(e.flags);
  expect(m.fpMode).toBe(e.fp_mode);
  expect(m.fNumber).toBe(e.f_number);
  expect(m.times.map((t) => t.timestampUs)).toEqual(e.frame_times_us);
  expect(m.ratioQ16).toBe(e.ratio_q16);
  expect(m.histograms.length).toBe(e.n_histograms);
  expect(m.refFrames.length).toBe(e.n_ref_frames);
  expect(m.problems.length).toBe(e.n_problems);
  expect(m.frames.every((f) => f.distanceInMm === e.distance_in_mm)).toBe(true);
  expect([m.results.peaks, m.rows, m.cols]).toEqual([e.peaks, e.rows, e.cols]);
  expect(Array.from(m.results.distance!)).toEqual(e.distance);
  expect(Array.from(m.results.snr!)).toEqual(e.snr);
  for (const k of ["signal", "noise", "xtalk"] as const) {
    if (e[k] === null) expect(m.results[k]).toBeNull();
    else expect(Array.from(m.results[k]!)).toEqual(e[k]);
  }
  // distance_mm = raw × ratio (/4 in 0.25 mm units), ratio = q16 / (8 << 16)
  const ratio = e.ratio_q16 / (8 << 16);
  const d0 = e.distance[0] * ratio;
  expect(m.distanceMm![0]).toBeCloseTo(e.distance_in_mm ? d0 : d0 / 4, 9);
}

// ── init ─────────────────────────────────────────────────────────────────────

describe("init()", () => {
  it("runs the contract 14 §4 sequence and downloads the firmware image", async () => {
    const progress: Array<[number, number]> = [];
    const [ver, serial] = await dev.init((d, t) => progress.push([d, t]));
    expect(ver).toBe("2.200.0");
    expect(serial).toBe(0x12345678);
    expect(dev.appVersion).toBe("2.200.0");
    expect(dev.sensorSerial).toBe(0x12345678);
    expect(dev.config).toBeNull();

    const w = fake.writes();
    expect(w[0]).toEqual([0xf8, 0x14]); // PON | bootmonitor
    expect(w[1]).toEqual([0xf7, 0x40]); // soft reset
    expect([...fake.sent(R)[0]!]).toEqual([0xe3, 1, 0]); // ID read first after the reset
    expect(w[2]).toEqual([0x08, 0x22]); // BL I2C_OFF
    // W_FIFO_BOTH: addr 0x00010000, words 3611 (contract 14 §4)
    expect(w[3]).toEqual([0x08, 0x45, 6, 0x00, 0x00, 0x01, 0x00, 3611 & 0xff, 3611 >> 8]);
    expect(fake.wFifoParams && [...fake.wFifoParams]).toEqual([0, 0, 1, 0, 0x1b, 0x0e]);
    const fifo = w.filter((x) => x[0] === 0xff);
    expect(fifo.length).toBe(Math.ceil(14444 / 1023));
    expect(fifo.every((x) => x.length - 1 <= 1023)).toBe(true);
    expect(createHash("sha256").update(Uint8Array.from(fake.fifo)).digest("hex")).toBe(V.firmware.sha256);
    const image = await tmf8829FirmwareImage();
    expect(fake.fifo.length).toBe(image.length);
    const afterFifo = w.slice(w.indexOf(fifo[fifo.length - 1]!) + 1);
    expect(afterFifo[0]).toEqual([0x08, 0x16]); // START_RAM_APP
    // ENABLE read back 0x94 (cpu_ready | PON | bootmon) → powerup_select = RAM
    expect(afterFifo[1]).toEqual([0xf8, 0xa4]);
    expect(progress.length).toBe(fifo.length);
    expect(progress[progress.length - 1]).toEqual([14444, 14444]);
    for (let i = 1; i < progress.length; i++) expect(progress[i]![0]).toBeGreaterThan(progress[i - 1]![0]);
    expect(fake.mode).toBe("app");
  });

  it("survives a CMD_STAT that reads busy for a while", async () => {
    fake.busyPolls = 3;
    await expect(dev.init()).resolves.toEqual(["2.200.0", 0x12345678]);
  });

  it("refuses a wrong sensor ID before downloading anything", async () => {
    fake.deviceId = 0x00; // silent SPI
    await expect(dev.init()).rejects.toThrow(/sensor ID 0x00, expected 0x9E/);
    expect(fake.fifo.length).toBe(0);
    expect(fake.writes().length).toBe(2);
  });

  it("times out when the bootloader never shows", async () => {
    fake.stuckBeforeBootloader = true;
    const err = await dev.init().catch((e) => e);
    expect(err).toBeInstanceOf(Tmf8829Error);
    expect(err.message).toMatch(/sensor bootloader: timeout, reg 0x00 = 0x00/);
  });

  it("reports a bootloader status by name (ERR_ADDR) or hex", async () => {
    fake.blStatus.set(0x45, 2);
    await expect(dev.init()).rejects.toThrow("bootloader cmd 0x45 -> ERR_ADDR");
    expect(fake.fifo.length).toBe(0);
    fake.blStatus.clear();
    fake.blStatus.set(0x16, 7);
    await expect(dev.init()).rejects.toThrow("bootloader cmd 0x16 -> 0x07");
  });

  it("stops a running stream first", async () => {
    await dev.configure();
    await dev.start();
    expect(dev.streaming).toBe(true);
    await dev.init();
    expect(dev.streaming).toBe(false);
    expect(fake.sent(0x36).length).toBe(1); // TMF_STOP_STREAM
    expect(fake.writes().findIndex((x) => x[0] === 0x08 && x[1] === 0xff)).toBeLessThan(
      fake.writes().findIndex((x) => x[0] === 0xf7),
    );
  });
});

// ── configure ────────────────────────────────────────────────────────────────

describe("configure()", () => {
  it("loads the preset, writes every non-reset field and reads the page back", async () => {
    // Stale fields a preset does not reset (contract 14 §4 table).
    fake.cfg[0x2b - 0x22] = 1; // dump_histograms
    fake.cfg[0x2c - 0x22] = 1; // ref_spad_frame
    fake.cfg[0x2a - 0x22] = 0x3c; // 4 peaks + all strengths
    fake.cfg[0] = 100; // period
    const cfg = await dev.configure();

    const w = fake.writes();
    expect(w[0]).toEqual([0x08, 0x40]); // preset 8x8
    expect(w[1]).toEqual([0x08, 0x16]); // LOAD_CONFIG_PAGE
    const page = w[2]!;
    expect(page[0]).toBe(0x22);
    expect(page.length).toBe(1 + 190);
    expect(w[3]).toEqual([0x08, 0x15]); // WRITE_PAGE
    expect(w[4]).toEqual([0x08, 0x16]); // read back
    const reads = fake.sent(R).map((p) => [...p]);
    expect(reads).toContainEqual([0x20, 1, 0]); // CID_RID
    expect(reads).toContainEqual([0x22, 190, 0]); // the page

    expect(cfg.preset).toBe("8x8");
    expect(cfg.grid).toEqual([8, 8]);
    expect(cfg.histograms).toBe(false);
    expect(cfg.refFrame).toBe(false);
    expect(cfg.page.length).toBe(190);
    expect(cfg.fields).toMatchObject({
      period_ms: 33,
      kilo_iterations: 586,
      fp_mode: 0,
      nr_peaks: 1,
      signal_strength: 0,
      noise_strength: 0,
      xtalk: 0,
      dump_histograms: 0,
      ref_spad_frame: 0,
    });
    expect(dev.config).toBe(cfg);
  });

  it("writes caller options into the page (32x32, histograms, ref frame)", async () => {
    const cfg = await dev.configure("32x32", {
      periodMs: 300,
      nrPeaks: 2,
      signalStrength: true,
      noiseStrength: true,
      xtalk: true,
      histograms: true,
      refFrame: true,
      fields: { power_modes: 3 },
    });
    expect(fake.writes()[0]).toEqual([0x08, 0x45]);
    const page = fake.writes()[2]!.slice(1);
    expect(page[0]! | (page[1]! << 8)).toBe(300);
    expect(page[2]! | (page[3]! << 8)).toBe(2344); // preset kilo_iterations
    expect(page[0x2a - 0x22]).toBe(2 | 0x08 | 0x10 | 0x20);
    expect(page[0x2b - 0x22]).toBe(1);
    expect(page[0x2c - 0x22]).toBe(1);
    expect(page[0x2e - 0x22]).toBe(3);
    expect(cfg.grid).toEqual([32, 32]);
    expect(cfg.histograms).toBe(true);
    expect(cfg.refFrame).toBe(true);
    expect(cfg.fields.fp_mode).toBe(3);
  });

  it("refuses bad arguments before sending anything", async () => {
    await expect(dev.configure("9x9" as never)).rejects.toBeInstanceOf(RangeError);
    for (const nrPeaks of [0, 5, 1.5]) {
      await expect(dev.configure("8x8", { nrPeaks })).rejects.toThrow(/nrPeaks .* must be 1\.\.4/);
    }
    expect(fake.log.length).toBe(0);
  });

  it("refuses while streaming", async () => {
    await dev.start();
    await expect(dev.configure()).rejects.toThrow(/while streaming/);
  });

  it("names the sensor's refusal of the page", async () => {
    fake.appStatus.set(0x15, 0x04);
    const err = await dev.configure("48x32", { nrPeaks: 4 }).catch((e) => e);
    expect(err).toBeInstanceOf(Tmf8829Error);
    expect(err.message).toBe("sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE");
    fake.appStatus.set(0x15, 0x0f);
    await expect(dev.configure()).rejects.toThrow("sensor cmd 0x15 -> 0x0F");
  });

  it("rejects a field that does not read back", async () => {
    fake.stuckBytes.set(0x78 - 0x22, 5); // fov_correction: 3 written, 5 read (measured)
    await expect(dev.configure("8x8", { fields: { fov_correction: 3 } })).rejects.toThrow(
      "config fields did not stick: fov_correction wrote 3, reads 5",
    );
    expect(dev.config).toBeNull();
  });

  it("checks CID_RID after LOAD_CONFIG_PAGE", async () => {
    fake.cidOverride = 0x40;
    await expect(dev.configPage()).rejects.toThrow("CID_RID 0x40 after LOAD_CONFIG_PAGE");
  });

  it("configPage() returns the sensor's 190-byte page", async () => {
    fake.cfg[5] = 0xab;
    const page = await dev.configPage();
    expect(page.length).toBe(190);
    expect(page[5]).toBe(0xab);
  });
});

// ── start / stop ─────────────────────────────────────────────────────────────

describe("start() / stop()", () => {
  it("configures 8x8 when needed and arms the stream (results only)", async () => {
    await dev.start();
    expect(dev.streaming).toBe(true);
    expect(dev.config?.preset).toBe("8x8");
    const w = fake.writes();
    const i = w.findIndex((x) => x[0] === 0xfa);
    expect(w.slice(i, i + 3)).toEqual([
      [0xfa, 0x30],
      [0xe1, 0xff],
      [0xe2, 0x01],
    ]);
    expect(fake.sent(0x35).map((p) => [...p])).toEqual([[0x01, 0x00]]);
    expect(w[w.length - 1]).toEqual([0x08, 0x10]); // MEASURE after the stream is armed
    const startIdx = fake.log.findIndex((c) => c.cmd === 0x35);
    const measIdx = fake.log.findIndex((c) => c.cmd === W && c.payload[0] === 0x08 && c.payload[1] === 0x10);
    expect(startIdx).toBeLessThan(measIdx);

    await dev.start(); // no-op while streaming
    expect(fake.sent(0x35).length).toBe(1);
  });

  it("arms histograms and the ref-SPAD frame from the config", async () => {
    await dev.configure("8x8", { histograms: true, refFrame: true });
    await dev.start();
    expect(fake.writes()).toContainEqual([0xe2, 0x09]);
    expect(fake.sent(0x35).map((p) => [...p])).toEqual([[0x09, 0x01]]);
  });

  it("disarms the bridge when MEASURE is refused", async () => {
    await dev.configure();
    fake.appStatus.set(0x10, 0x03);
    await expect(dev.start()).rejects.toThrow("sensor cmd 0x10 -> ERR_APPLICATION");
    expect(dev.streaming).toBe(false);
    expect(fake.sent(0x36).length).toBe(1);
  });

  it("stop() sends STOP, clears interrupts, then disarms the bridge", async () => {
    await dev.start();
    const n = fake.log.length;
    await dev.stop();
    const tail = fake.log.slice(n).filter((c) => c.cmd !== R);
    expect(tail.map((c) => [c.cmd, ...c.payload])).toEqual([
      [W, 0x08, 0xff],
      [W, 0xe2, 0x00],
      [W, 0xe1, 0xff],
      [0x36],
    ]);
    expect(dev.streaming).toBe(false);
    // Not streaming: the sensor is still stopped but the bridge is left alone.
    await dev.stop();
    expect(fake.sent(0x36).length).toBe(1);
  });

  it("stop() swallows the one BAD_HEADER a STOP raises, other events pass", async () => {
    const events: Tmf8829Event[] = [];
    const unsub = dev.onInterrupt((e) => events.push(e));
    await dev.start();
    fake.onCommand = async (cmd) => {
      if (cmd === 0xff) await fake.emitEvent(Tmf8829EventCode.BadHeader, 0x08, 0xff, 0x04);
    };
    await dev.stop();
    expect(events).toEqual([]);

    await fake.emitEvent(Tmf8829EventCode.Int, 0x02);
    await fake.emitEvent(Tmf8829EventCode.BadHeader, 0x01, 0x55, 0x00); // sensor lost
    await waitFor(events, 2);
    expect(events[0]).toMatchObject({ code: 0, intStatus: 0x02, frameId: 0, fifoStatus: 0, timestampUs: 987654321n });
    expect(events[1]).toMatchObject({ code: 1, frameId: 0x55 });
    unsub();
    await fake.emitEvent(Tmf8829EventCode.Int, 0x04);
    await fake.emitRaw(0x94, new Uint8Array(5)); // truncated event: ignored
    await tick();
    expect(events.length).toBe(2);
  });

  it("stop() rejects with the sensor's STOP status and leaves streaming off", async () => {
    await dev.start();
    fake.appStatus.set(0xff, 0x0a);
    await expect(dev.stop()).rejects.toThrow("sensor cmd 0xFF -> ERR_STOP_0");
    expect(dev.streaming).toBe(false);
  });

  it("close() stops a running stream", async () => {
    await dev.start();
    await dev.close();
    expect(fake.writes()).toContainEqual([0x08, 0xff]);
    expect(fake.sent(0x36).length).toBe(1);
  });
});

// ── streaming → measurements ─────────────────────────────────────────────────

describe("measurements from the stream", () => {
  it.each<string>(V.measurements.map((m: Any) => m.name))("%s matches the golden vector", async (name) => {
    const got: Tmf8829Measurement[] = [];
    dev.onMeasurement((m) => got.push(m));
    await dev.start();
    await playVector(name);
    const want = MEAS(name).expect;
    await waitFor(got, want.length);
    for (let i = 0; i < want.length; i++) checkAgainstVector(got[i]!, want[i]);
    expect(dev.framesDropped).toBe(0);
    expect(dev.framesDiscarded).toBe(0);
    expect(dev.frameParseErrors).toBe(0);
    expect(dev.clockRatioQ16).toBe(want[want.length - 1].ratio_q16);
  });

  it("histogram frames are reassembled from 1024-byte chunks", async () => {
    const got: Tmf8829Measurement[] = [];
    dev.onMeasurement((m) => got.push(m));
    await dev.start();
    await playVector("8x8_hist");
    await waitFor(got, 1);
    expect(got[0]!.histograms.map((h) => h.raw.length)).toEqual([25377, 25377]);
    expect(got[0]!.histograms[0]!.raw).toEqual(raw("8x8_hist#0"));
  });

  it("delivers to measurements(), getMeasurement() and every subscriber", async () => {
    const a: number[] = [];
    const b: number[] = [];
    dev.onMeasurement((m) => a.push(m.fNumber));
    const unsubB = dev.onMeasurement((m) => b.push(m.fNumber));
    const q = dev.measurements(4);
    await dev.start();
    const next = dev.getMeasurement(1000);
    await fake.emitFrame(raw("8x8#0"), 2125575099n, 2125575071n);
    expect((await next).fNumber).toBe(1);
    unsubB();
    await fake.emitFrame(raw("8x8#1"), 2125672180n, 2125672161n);
    await waitFor(a, 2);
    expect(b).toEqual([1]);
    const it1 = await q.next();
    const it2 = await q.next();
    expect([it1.value?.fNumber, it2.value?.fNumber]).toEqual([1, 2]);
    q.close();
  });

  it("getMeasurement() times out, and ends when the device closes", async () => {
    await expect(dev.getMeasurement(50)).rejects.toBeInstanceOf(DepzTimeoutError);
    const pending = dev.getMeasurement(2000);
    await tick();
    await dev.close();
    await expect(pending).rejects.toThrow("device closed");
  });

  it("counts dropped, discarded and unparseable frames without failing", async () => {
    const got: Tmf8829Measurement[] = [];
    dev.onMeasurement((m) => got.push(m));
    await dev.start();
    await fake.emitFrame(raw("8x8#0"), 2125575099n, 2125575071n, { seq: 0 });
    // frame_seq jumps 1 → 3: two frames dropped by the bridge
    await fake.emitFrame(raw("8x8#1"), 2125672180n, 2125672161n, { seq: 3 });
    // a frame cut short (STOP mid frame): only its first chunk arrives
    await fake.emitFrame(raw("8x8_full#0"), 2125937131n, 0n, { seq: 4, chunks: [0] });
    await fake.emitFrame(raw("8x8#1").subarray(0, 40), 1n, 0n, { seq: 5 }); // truncated frame: parse error
    await fake.emitRaw(0x93, new Uint8Array(10)); // truncated chunk header: ignored
    await waitFor(got, 2);
    await tick();
    expect(dev.framesDropped).toBe(2);
    expect(dev.framesDiscarded).toBe(1);
    expect(dev.frameParseErrors).toBe(1);

    // start() resets the counters (frame_seq restarts at 0)
    await dev.stop();
    await dev.start();
    expect([dev.framesDropped, dev.framesDiscarded, dev.frameParseErrors]).toEqual([0, 0, 0]);
  });
});

// ── register access and diagnostics ──────────────────────────────────────────

describe("register access", () => {
  it("reads in ≤1024-byte requests that stop advancing at the FIFO port", async () => {
    const out = await dev.readRegisters(0xf0, 2000);
    expect(out.length).toBe(2000);
    expect(fake.sent(R).map((p) => [...p])).toEqual([
      [0xf0, 0x00, 0x04],
      [0xff, 976 & 0xff, 976 >> 8],
    ]);
  });

  it("writes the FIFO port in ≤1023-byte chunks, all at 0xFF", async () => {
    await dev.writeRegisters(0xff, new Uint8Array(2100).fill(7));
    expect(fake.sent(W).map((p) => [p[0], p.length - 1])).toEqual([
      [0xff, 1023],
      [0xff, 1023],
      [0xff, 54],
    ]);
    expect(fake.fifo.length).toBe(2100);
  });

  it("rejects a short READ_REG reply", async () => {
    fake.shortRead = 1;
    await expect(dev.readRegisters(0x00, 4)).rejects.toThrow("READ_REG 0x00: expected 4, got 3");
  });

  it("surfaces a bridge error status and keeps serving later calls", async () => {
    fake.bridgeStatus.set(W, 0x03);
    await expect(dev.writeRegisters(0xe2, Uint8Array.of(1))).rejects.toBeInstanceOf(StatusError);
    fake.bridgeStatus.clear();
    await expect(dev.writeRegisters(0xe2, Uint8Array.of(1))).resolves.toBeUndefined();
    expect(fake.regs[0xe2]).toBe(1);
  });

  it("times out a READ_REG the bridge never answers (1 s register timeout)", async () => {
    fake.silent.add(R);
    const t0 = Date.now();
    const err = await dev.readRegisters(0x00, 1).catch((e) => e);
    expect(err).toBeInstanceOf(DepzTimeoutError);
    expect(Date.now() - t0).toBeGreaterThanOrEqual(900); // not the 500 ms device default
  });

  it("sensorState() reads the host-interface registers", async () => {
    await dev.init();
    const st = await dev.sensorState();
    expect(Object.keys(st)).toEqual([
      "ID", "REVID", "ENABLE", "RESET", "APP_ID", "CMD_STAT", "INT_STATUS", "INT_ENAB", "FIFOSTATUS",
    ]);
    expect(st.ID).toBe(0x9e);
    expect(st.APP_ID).toBe(0x01);
    expect(st.ENABLE! & 0x80).toBe(0x80);
  });

  it("getBridgeInfo() decodes RPT_TMF_INFO", async () => {
    await dev.start();
    const info = await dev.getBridgeInfo();
    expect(info).toMatchObject({ intEdges: 7, frames: 6, framesAborted: 1, streaming: true, intMask: 0x01 });
  });

  it("a failed sequence does not wedge the next one", async () => {
    fake.deviceId = 0x12;
    await expect(dev.init()).rejects.toBeInstanceOf(DepzError);
    fake.deviceId = 0x9e;
    await expect(dev.init()).resolves.toEqual(["2.200.0", 0x12345678]);
  });
});
