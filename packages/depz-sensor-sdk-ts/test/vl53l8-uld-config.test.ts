/**
 * VL53L8 host ULD: the standard configuration surface (contract 04 §5 —
 * ranging frequency / mode, integration time, target order), poll-mode reads
 * (§6 — checkDataReady + getRangingData, incl. the motion-indicator block and
 * the header/footer integrity check), the xtalk calibration run and caldata
 * read-back (§7), and the Vl53l8cx facade's delegation of all of the above.
 *
 * Runs against the fake register/DCI platform (test/fake-vl53l8.ts), so the
 * real DCI byte sequences execute end to end — no hardware.
 */

import { describe, expect, it } from "vitest";
import { LoopbackTransport, Vl53l7cx, Vl53l8cx } from "../src/index.js";
import {
  DCI_OUTPUT_LIST,
  DCI_SINGLE_RANGE,
  DCI_ZONE_CONFIG,
  AMBIENT_RATE_IDX,
  DISTANCE_IDX,
  METADATA_IDX,
  MOTION_DETEC_IDX,
  NB_TARGET_DETECTED_IDX,
  REFLECTANCE_EST_PC_IDX,
  SIGNAL_RATE_IDX,
  POWER_MODE_SLEEP,
  RANGING_MODE_AUTONOMOUS,
  RANGING_MODE_CONTINUOUS,
  RESOLUTION_4X4,
  RESOLUTION_8X8,
  STATUS_CORRUPTED_FRAME,
  STATUS_ERROR,
  STATUS_INVALID_PARAM,
  TARGET_ORDER_CLOSEST,
  TARGET_ORDER_STRONGEST,
  TARGET_STATUS_IDX,
  VL53L8CX,
  Vl53l8cxError,
  XTALK_BUFFER_SIZE,
  XTALK_CAL_OUTPUT,
  swapBuffer,
} from "../src/sensors/vl53l8/uld.js";
import { FakeUldPlatform, makeDriver } from "./fake-vl53l8.js";

const UI_CMD_START = 0x2c04;
const XTALK_FOOTER = [0x00, 0x00, 0x00, 0x0f, 0x00, 0x01, 0x03, 0x04];

/** Driver with a sane 4x4 zone config already in the DCI store. */
async function configured(): Promise<{ drv: VL53L8CX; p: FakeUldPlatform }> {
  const { drv, p } = makeDriver();
  p.dci.set(DCI_ZONE_CONFIG, new Uint8Array(8));
  await drv.setResolution(RESOLUTION_4X4);
  return { drv, p };
}

async function expectInvalid(pr: Promise<unknown>): Promise<void> {
  const err = await pr.then(
    () => null,
    (e: unknown) => e,
  );
  expect(err).toBeInstanceOf(Vl53l8cxError);
  expect((err as Vl53l8cxError).code).toBe(STATUS_INVALID_PARAM);
}

// ── standard configuration surface (§5) ───────────────────────────────────────

describe("vl53l8 uld: standard configuration", () => {
  it("ranging frequency round-trips through DCI_FREQ_HZ byte 1", async () => {
    const { drv } = await configured();
    await drv.setRangingFrequencyHz(15);
    expect(await drv.getRangingFrequencyHz()).toBe(15);
  });

  it("ranging mode: continuous / autonomous round-trip and set the single-range flag", async () => {
    const { drv, p } = await configured();
    await drv.setRangingMode(RANGING_MODE_CONTINUOUS);
    expect(await drv.getRangingMode()).toBe(RANGING_MODE_CONTINUOUS);
    expect(Array.from(p.dci.get(DCI_SINGLE_RANGE)!)).toEqual([0, 0, 0, 0]);

    await drv.setRangingMode(RANGING_MODE_AUTONOMOUS);
    expect(await drv.getRangingMode()).toBe(RANGING_MODE_AUTONOMOUS);
    expect(Array.from(p.dci.get(DCI_SINGLE_RANGE)!)).toEqual([1, 0, 0, 0]);
  });

  it("ranging mode rejects anything but continuous / autonomous", async () => {
    const { drv } = await configured();
    await expectInvalid(drv.setRangingMode(2));
  });

  it("integration time 2..1000 ms round-trips (stored in µs)", async () => {
    const { drv } = await configured();
    for (const ms of [2, 20, 1000]) {
      await drv.setIntegrationTimeMs(ms);
      expect(await drv.getIntegrationTimeMs()).toBe(ms);
    }
  });

  it("integration time outside 2..1000 (or NaN) is refused, never clamped", async () => {
    const { drv } = await configured();
    await drv.setIntegrationTimeMs(50);
    for (const bad of [1, 1001, Number.NaN]) await expectInvalid(drv.setIntegrationTimeMs(bad));
    expect(await drv.getIntegrationTimeMs()).toBe(50);
  });

  it("target order closest / strongest round-trips; other values are refused", async () => {
    const { drv } = await configured();
    await drv.setTargetOrder(TARGET_ORDER_STRONGEST);
    expect(await drv.getTargetOrder()).toBe(TARGET_ORDER_STRONGEST);
    await drv.setTargetOrder(TARGET_ORDER_CLOSEST);
    expect(await drv.getTargetOrder()).toBe(TARGET_ORDER_CLOSEST);
    await expectInvalid(drv.setTargetOrder(3));
  });

  it("setResolution rejects a zone count other than 16 / 64", async () => {
    const { drv } = await configured();
    await expectInvalid(drv.setResolution(32));
  });

  it("motionIndicatorSetResolution re-maps the aggregates and rewrites the config", async () => {
    const { drv } = await configured();
    const cfg = await drv.motionIndicatorInit(RESOLUTION_8X8);
    await drv.motionIndicatorSetResolution(cfg, RESOLUTION_4X4);
    expect(cfg.mapId.slice(0, 16)).toEqual(Array.from({ length: 16 }, (_, i) => i));
    expect(cfg.mapId.slice(16).every((v) => v === -1)).toBe(true);
  });
});

// ── poll mode (§6) ────────────────────────────────────────────────────────────

describe("vl53l8 uld: checkDataReady", () => {
  function setHeader(p: FakeUldPlatform, b: number[]): void {
    b.forEach((v, i) => p.reg.set(i, v));
  }

  it("a new stream count with the ready bits set is a fresh frame, once", async () => {
    const { drv, p } = makeDriver();
    setHeader(p, [5, 0x05, 0x05, 0x10]);
    expect(await drv.checkDataReady()).toBe(true);
    // same stream count again → not a new frame
    expect(await drv.checkDataReady()).toBe(false);
    setHeader(p, [6, 0x05, 0x05, 0x10]);
    expect(await drv.checkDataReady()).toBe(true);
  });

  it("stream count 255 or missing ready bits is not ready", async () => {
    const { drv, p } = makeDriver();
    setHeader(p, [255, 0x05, 0x05, 0x10]);
    expect(await drv.checkDataReady()).toBe(false);
    setHeader(p, [7, 0x05, 0x04, 0x10]); // byte 2 lacks bit 0
    expect(await drv.checkDataReady()).toBe(false);
    setHeader(p, [7, 0x05, 0x05, 0x00]); // byte 3 lacks 0x10
    expect(await drv.checkDataReady()).toBe(false);
  });

  it("GO2 error bit (byte 3 bit 7) raises with the status in byte 2", async () => {
    const { drv, p } = makeDriver();
    setHeader(p, [7, 0x00, 0x42, 0x80]);
    const err = await drv.checkDataReady().catch((e: unknown) => e);
    expect(err).toBeInstanceOf(Vl53l8cxError);
    expect((err as Vl53l8cxError).code).toBe(0x42);
  });
});

describe("vl53l8 uld: getRangingData (poll-mode frame)", () => {
  // Frame (after swap): 16-byte header, DISTANCE (i16×16), NB_TARGET (u8×16),
  // TARGET_STATUS (u8×16), SIGNAL (u32×16), AMBIENT (u32×16), REFLECTANCE
  // (u8×16), MOTION (140 B), 32-byte tail; the footer id sits at size-12 (cx).
  const SIZE =
    16 + (4 + 32) + (4 + 16) + (4 + 16) + (4 + 64) + (4 + 64) + (4 + 16) + (4 + 140) + 32;
  const ID = [0x00, 0x07];

  function frame(opts: { motion?: boolean; footerId?: number[] } = {}): Uint8Array {
    const buf = new Uint8Array(SIZE);
    const dv = new DataView(buf.buffer);
    buf[8] = ID[0]!;
    buf[9] = ID[1]!;
    let i = 16;
    const block = (idx: number, type: number, size: number): number => {
      dv.setUint32(i, ((idx << 16) | (size << 4) | type) >>> 0, true);
      const data = i + 4;
      i = data + (type > 1 && type < 0xd ? type * size : size);
      return data;
    };
    let d = block(DISTANCE_IDX, 2, 16);
    for (let z = 0; z < 16; z++) dv.setInt16(d + 2 * z, (100 + z) * 4 + 3, true);
    dv.setInt16(d + 2 * 1, -9, true); // a negative distance: ST clamps it to 0
    d = block(NB_TARGET_DETECTED_IDX, 1, 16);
    for (let z = 0; z < 16; z++) buf[d + z] = z === 3 ? 0 : 1;
    d = block(TARGET_STATUS_IDX, 1, 16);
    for (let z = 0; z < 16; z++) buf[d + z] = 5;
    d = block(SIGNAL_RATE_IDX, 4, 16);
    for (let z = 0; z < 16; z++) dv.setUint32(d + 4 * z, z * 1000 + 1, true);
    d = block(AMBIENT_RATE_IDX, 4, 16);
    for (let z = 0; z < 16; z++) dv.setUint32(d + 4 * z, z * 3, true);
    d = block(REFLECTANCE_EST_PC_IDX, 1, 16);
    for (let z = 0; z < 16; z++) buf[d + z] = 200 + z;
    // motion block (type 0, 140 bytes) — or a same-size filler block
    d = block(opts.motion === false ? 0x1234 : MOTION_DETEC_IDX, 0, 140);
    dv.setUint32(d, 1234, true);
    dv.setUint32(d + 4, 5678, true);
    buf[d + 8] = 1;
    buf[d + 9] = 2;
    buf[d + 10] = 16;
    for (let k = 0; k < 32; k++) dv.setUint32(d + 12 + 4 * k, k * 10, true);
    const fid = opts.footerId ?? ID;
    buf[SIZE - 12] = fid[0]!;
    buf[SIZE - 11] = fid[1]!;
    return swapBuffer(buf); // what the sensor holds at register 0x00
  }

  function load(p: FakeUldPlatform, raw: Uint8Array): void {
    raw.forEach((v, i) => p.reg.set(i, v));
  }

  function driver(): { drv: VL53L8CX; p: FakeUldPlatform } {
    const { drv, p } = makeDriver();
    const a = drv as unknown as Record<string, unknown>;
    a.dataReadSize = SIZE;
    a.motionPresent = true;
    return { drv, p };
  }

  it("scales to ST's units, marks no-target zones 255 and surfaces motion", async () => {
    const { drv, p } = driver();
    load(p, frame());
    const r = await drv.getRangingData();
    // distance raw/4 floored, a negative one clamped to 0 (contract 04 §5)
    expect(r.distanceMm).toEqual(Array.from({ length: 16 }, (_, z) => (z === 1 ? 0 : 100 + z)));
    // signal / ambient raw/2048 (kcps/SPAD), reflectance raw/2 (%), fraction kept
    expect(r.signalPerSpad).toEqual(Array.from({ length: 16 }, (_, z) => (z * 1000 + 1) / 2048));
    expect(r.ambientPerSpad).toEqual(Array.from({ length: 16 }, (_, z) => (z * 3) / 2048));
    expect(r.reflectance).toEqual(Array.from({ length: 16 }, (_, z) => (200 + z) / 2));
    expect(r.reflectance[1]).toBe(100.5);
    expect(r.nbTargetDetected[3]).toBe(0);
    expect(r.targetStatus[3]).toBe(255);
    expect(r.targetStatus[2]).toBe(5);
    expect(r.motion).not.toBeNull();
    expect(r.motion!.globalIndicator1).toBe(1234);
    expect(r.motion!.globalIndicator2).toBe(5678);
    expect(r.motion!.status).toBe(1);
    expect(r.motion!.nbOfDetectedAggregates).toBe(2);
    expect(r.motion!.nbOfAggregates).toBe(16);
    expect(r.motion!.motion).toEqual(Array.from({ length: 32 }, (_, k) => k * 10));
  });

  it("motion is null when the motion detector is armed but the block is absent", async () => {
    const { drv, p } = driver();
    load(p, frame({ motion: false }));
    expect((await drv.getRangingData()).motion).toBeNull();
  });

  it("motion is not parsed at all when the indicator was never configured", async () => {
    const { drv, p } = driver();
    (drv as unknown as Record<string, unknown>).motionPresent = false;
    load(p, frame());
    expect((await drv.getRangingData()).motion).toBeNull();
  });

  it("a header/footer id mismatch is a corrupted frame", async () => {
    const { drv, p } = driver();
    load(p, frame({ footerId: [0x00, 0x08] }));
    const err = await drv.getRangingData().catch((e: unknown) => e);
    expect(err).toBeInstanceOf(Vl53l8cxError);
    expect((err as Vl53l8cxError).code).toBe(STATUS_CORRUPTED_FRAME);
  });

  // An all-zero frame (zero headers walk 4 bytes at a time) holding one
  // METADATA block too short to carry the temperature at body offset 8. Mid
  // frame, the byte where it would be is 0xF6 (-10); at the end, the read
  // would run past the frame.
  it.each([
    [4, false],
    [8, false],
    [0, true],
  ])("a %i-byte METADATA block (at end: %s) leaves the temperature at 0", (size, atEnd) => {
    const { drv } = makeDriver();
    const a = drv as unknown as Record<string, unknown>;
    a.dataReadSize = 540;
    const buf = new Uint8Array(540);
    const at = atEnd ? 540 - 4 : 16;
    new DataView(buf.buffer).setUint32(at, ((METADATA_IDX << 16) | (size << 4)) >>> 0, true);
    if (!atEnd) buf[16 + 12] = 0xf6;
    const r = drv.parseFrame(swapBuffer(buf));
    expect(drv.lastBlocks.map((b) => b[0])).toContain(METADATA_IDX);
    expect(r.siliconTempDegc).toBe(0);
  });
});

// ── xtalk calibration + caldata read-back (§7) ───────────────────────────────

describe("vl53l8 uld: xtalk calibration", () => {
  /** FW xtalk buffer as read back from UI_CMD_START (XTALK_BUFFER_SIZE + 4). */
  const fwXtalk = Uint8Array.from({ length: XTALK_BUFFER_SIZE + 4 }, (_, i) => (i * 7) & 0xff);

  async function calDriver(): Promise<{ drv: VL53L8CX; p: FakeUldPlatform }> {
    const { drv, p } = await configured();
    (drv as unknown as Record<string, unknown>).defaultCfg = Uint8Array.of(0xc0, 0xfe);
    const rd = p.rdMulti.bind(p);
    p.rdMulti = (addr: number, size: number): Promise<Uint8Array> =>
      addr === UI_CMD_START && size === XTALK_BUFFER_SIZE + 4
        ? Promise.resolve(fwXtalk.slice())
        : rd(addr, size);
    await drv.setRangingFrequencyHz(10);
    await drv.setIntegrationTimeMs(20);
    await drv.setSharpenerPercent(14);
    await drv.setTargetOrder(TARGET_ORDER_STRONGEST);
    await drv.setXtalkMargin(50);
    await drv.setRangingMode(RANGING_MODE_CONTINUOUS);
    return { drv, p };
  }

  function expectedBlob(): number[] {
    const out = new Uint8Array(XTALK_BUFFER_SIZE);
    out.set(fwXtalk.subarray(8, XTALK_BUFFER_SIZE), 0);
    out.set(XTALK_FOOTER, XTALK_BUFFER_SIZE - 8);
    return Array.from(out);
  }

  it("validates reflectance 1..99, distance 600..3000 mm and samples 1..16", async () => {
    const { drv } = await calDriver();
    await expectInvalid(drv.calibrateXtalk(0, 4, 600));
    await expectInvalid(drv.calibrateXtalk(100, 4, 600));
    await expectInvalid(drv.calibrateXtalk(3, 4, 599));
    await expectInvalid(drv.calibrateXtalk(3, 4, 3001));
    await expectInvalid(drv.calibrateXtalk(3, 0, 600));
    await expectInvalid(drv.calibrateXtalk(3, 17, 600));
  });

  it("runs, captures the 776-byte buffer and restores every saved setting", async () => {
    const { drv, p } = await calDriver();
    await drv.calibrateXtalk(3, 4, 600);

    expect(drv.xtalkCalibrationFailed).toBe(false);
    const xtalk = (drv as unknown as { xtalkData: Uint8Array }).xtalkData;
    expect(Array.from(xtalk)).toEqual(expectedBlob());

    // the firmware's calibration table went to 0x2C28, and the dedicated
    // 17-block calibration output list (not the ranging list) was programmed
    expect(p.writes.some(([addr]) => addr === 0x2c28)).toBe(true);
    expect(p.dci.get(DCI_OUTPUT_LIST)!.length).toBe(XTALK_CAL_OUTPUT.length * 4);
    // default config restored at 0x2C34
    expect(p.writes.some(([addr, b]) => addr === 0x2c34 && b[0] === 0xc0)).toBe(true);

    expect(await drv.getResolution()).toBe(RESOLUTION_4X4);
    expect(await drv.getRangingFrequencyHz()).toBe(10);
    expect(await drv.getIntegrationTimeMs()).toBe(20);
    expect(await drv.getSharpenerPercent()).toBe(14);
    expect(await drv.getTargetOrder()).toBe(TARGET_ORDER_STRONGEST);
    expect(await drv.getXtalkMargin()).toBe(50);
    expect(await drv.getRangingMode()).toBe(RANGING_MODE_CONTINUOUS);
  });

  it("flags XTALK_FAILED ('coverglass too good') yet still reads the FW buffer back", async () => {
    const { drv, p } = await calDriver();
    [0x10, 0x00, 0x7f, 0x80].forEach((v, i) => p.reg.set(i, v));
    await drv.calibrateXtalk(3, 4, 600);
    expect(drv.xtalkCalibrationFailed).toBe(true);
    expect(Array.from((drv as unknown as { xtalkData: Uint8Array }).xtalkData)).toEqual(
      expectedBlob(),
    );
  });

  it("gives up with STATUS_ERROR when the run never leaves the error state", async () => {
    const { drv, p } = await calDriver();
    p.reg.set(0, STATUS_ERROR);
    const err = await drv.calibrateXtalk(3, 4, 600).catch((e: unknown) => e);
    expect(err).toBeInstanceOf(Vl53l8cxError);
    expect((err as Vl53l8cxError).code).toBe(STATUS_ERROR);
  });

  it("getCaldataXtalk returns the FW buffer + footer and keeps the resolution", async () => {
    const { drv } = await calDriver();
    const blob = await drv.getCaldataXtalk();
    expect(blob.length).toBe(XTALK_BUFFER_SIZE);
    expect(Array.from(blob)).toEqual(expectedBlob());
    expect(await drv.getResolution()).toBe(RESOLUTION_4X4);
  });
});

// ── Vl53l8cx facade delegation ────────────────────────────────────────────────

describe("vl53l8cx facade: configuration delegation", () => {
  async function facade(): Promise<{ dev: Vl53l8cx; drv: VL53L8CX; p: FakeUldPlatform }> {
    const dev = new Vl53l8cx(new LoopbackTransport());
    const { drv, p } = await configured();
    (dev as unknown as { uldDriver: unknown }).uldDriver = drv;
    return { dev, drv, p };
  }

  it("the frequency ceiling follows the resolution: 60 Hz at 4x4, 15 Hz at 8x8", async () => {
    const { dev, p } = await facade();
    await dev.setRangingFrequencyHz(60);
    expect(await dev.getRangingFrequencyHz()).toBe(60);
    const writes = p.writes.length;
    await expect(dev.setRangingFrequencyHz(61)).rejects.toThrow(/<= 60 Hz/);
    expect(p.writes.length).toBe(writes); // refused host-side, nothing on the bus
    await dev.setResolution(RESOLUTION_8X8);
    await dev.setRangingFrequencyHz(15);
    await expect(dev.setRangingFrequencyHz(16)).rejects.toThrow(/<= 15 Hz/);
    expect(await dev.getRangingFrequencyHz()).toBe(15);
    await dev.setResolution(RESOLUTION_4X4);
    await dev.setRangingFrequencyHz(16); // the ceiling moved back with the resolution
  });

  it("standard settings round-trip through the facade", async () => {
    const { dev } = await facade();
    await dev.setRangingFrequencyHz(12);
    expect(await dev.getRangingFrequencyHz()).toBe(12);
    await dev.setRangingMode(RANGING_MODE_AUTONOMOUS);
    expect(await dev.getRangingMode()).toBe(RANGING_MODE_AUTONOMOUS);
    await dev.setIntegrationTimeMs(33);
    expect(await dev.getIntegrationTimeMs()).toBe(33);
    await dev.setSharpenerPercent(25);
    expect(await dev.getSharpenerPercent()).toBe(25);
    await dev.setTargetOrder(TARGET_ORDER_STRONGEST);
    expect(await dev.getTargetOrder()).toBe(TARGET_ORDER_STRONGEST);
  });

  it("power mode, thresholds and caldata go through the facade", async () => {
    const { dev, p } = await facade();
    p.reg.set(0x09, 0x04); // awake
    p.reg.set(0x06, 0x00);
    await dev.setPowerMode(POWER_MODE_SLEEP);
    expect(p.reg.get(0x09)).toBe(0x02);
    p.reg.set(0x0f, 0x00);
    expect(await dev.getPowerMode()).toBe(POWER_MODE_SLEEP);

    await dev.setDetectionThresholds([{ lowThresh: 100, highThresh: 900, measurement: 1 }]);
    const th = await dev.getDetectionThresholds();
    expect(th).toHaveLength(64);
    expect(th[0]!.lowThresh).toBe(100);
    expect(th[0]!.highThresh).toBe(900);
    await dev.setDetectionThresholdsAutoStop(true);

    const blob = new Uint8Array(XTALK_BUFFER_SIZE).fill(0x5a);
    await dev.setCaldataXtalk(blob);
    expect((dev.uld as unknown as { xtalkData: Uint8Array }).xtalkData[0]).toBe(0x5a);
  });

  it("calibrateXtalk / getCaldataXtalk are refused while ranging", async () => {
    const { dev } = await facade();
    (dev as unknown as { rangingFlag: boolean }).rangingFlag = true;
    await expect(dev.calibrateXtalk(3, 4, 600)).rejects.toThrow(/stopRanging/);
    await expect(dev.getCaldataXtalk()).rejects.toThrow(/stopRanging/);
    await expect(dev.setPowerMode(POWER_MODE_SLEEP)).rejects.toThrow(/stopRanging/);
    await expect(dev.setRangingMode(RANGING_MODE_CONTINUOUS)).rejects.toThrow(/stopRanging/);
    await expect(dev.setIntegrationTimeMs(20)).rejects.toThrow(/stopRanging/);
  });

  it("calibrateXtalk validates its arguments before touching the sensor", async () => {
    const { dev, p } = await facade();
    const before = p.writes.length;
    await expectInvalid(dev.calibrateXtalk(3, 4, 100));
    expect(p.writes.length).toBe(before);
  });

  it("activeCnhConfig is null until configureCnh()", () => {
    expect(new Vl53l8cx(new LoopbackTransport()).activeCnhConfig).toBeNull();
  });

  it("isAlive on L5/L7 (I2C): revision 0x02, or 0x01 with device id 0xF0", async () => {
    const dev = new Vl53l7cx(new LoopbackTransport());
    const p = new FakeUldPlatform();
    (dev as unknown as { platform: unknown }).platform = p;
    for (const [id, rev, alive] of [
      [0x00, 0x02, true],
      [0xf0, 0x01, true],
      [0x00, 0x01, false],
      [0xf0, 0x0c, false],
    ] as const) {
      p.reg.set(0x00, id);
      p.reg.set(0x01, rev);
      expect(await dev.isAlive()).toBe(alive);
    }
  });

  it("isAlive: true on the L8 silicon ids, false on a foreign id or a bridge error", async () => {
    const dev = new Vl53l8cx(new LoopbackTransport());
    const p = new FakeUldPlatform();
    p.reg.set(0x00, 0xf0);
    p.reg.set(0x01, 0x0c);
    (dev as unknown as { platform: unknown }).platform = p;
    expect(await dev.isAlive()).toBe(true);

    p.reg.set(0x01, 0x02); // L5/L7 revision: answers, but not an L8
    expect(await dev.isAlive()).toBe(false);
    p.reg.set(0x00, 0x00);
    p.reg.set(0x01, 0x0c);
    expect(await dev.isAlive()).toBe(false);

    p.rdMulti = (): Promise<Uint8Array> =>
      Promise.reject(new Vl53l8cxError(STATUS_ERROR, "bridge"));
    expect(await dev.isAlive()).toBe(false);

    p.rdMulti = (): Promise<Uint8Array> => Promise.reject(new TypeError("bug"));
    await expect(dev.isAlive()).rejects.toThrow(TypeError);
  });
});
