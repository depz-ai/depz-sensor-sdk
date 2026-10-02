/**
 * VL53LX Bare Driver (histogram) over the committed real-hardware captures,
 * with no device class: a minimal local BridgeDevice on `DepzDevice.request`
 * reproduces the Python capture's request sequence (tests/test_vl53lx_replay.py
 * + depz_sensor_sdk/vl53lx/__init__.py) byte for byte (strictTx), and every
 * streamed frame is decoded exactly once through `binData(data)` →
 * `toMeasurement(bins)` and compared to the `.expected.json` sidecar.
 *
 * The histogram driver is stateful (A/B frame pairs, phase-consistency
 * history). A sidecar with `refused` (the short preset on an L4 die, which
 * the driver no longer accepts): init must still replay, and the mode must be
 * refused after the configure re-init.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { DepzDevice } from "../src/device/device.js";
import { DepzError } from "../src/errors.js";
import type { PacketEvent } from "../src/protocol/framing.js";
import {
  XFER_MAX,
  packReadReg,
  packSetI2cSpeed,
  packWriteReg,
  packXshut,
  unpackRegData,
  unpackStream,
} from "../src/protocol/vl53l4.js";
import { ReplayTransport } from "../src/transport/replay.js";
import { BridgePlatform, type Measurement } from "../src/sensors/vl53lx/uld/base.js";
import { ProtocolError, Vl53Error, type BridgeDevice } from "../src/sensors/vl53lx/uld/link.js";
import { VL53LX } from "../src/sensors/vl53lx/uld/bare/driver.js";
import type { HistogramBinData } from "../src/sensors/vl53lx/uld/bare/core.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

// contracts/12 — APP_VL53L0_4 bridge v2.00.
const READ_REG = 0x32;
const WRITE_REG = 0x33;
const XSHUT = 0x34;
const START_STREAM = 0x35;
const STOP_STREAM = 0x36;
const SET_I2C_SPEED = 0x38;
const SET_ADDR_WIDTH = 0x39;
const CLEAR_I2C_ERRORS = 0x3a;
const RPT_REG_DATA = 0x91;
const RPT_STREAM = 0x93;

function packStartStream(
  addr: number,
  length: number,
  clear: ReadonlyArray<readonly [number, number]>,
  flags = 0,
): Uint8Array {
  const out = new Uint8Array(6 + 3 * clear.length);
  const dv = new DataView(out.buffer);
  dv.setUint16(0, addr, true);
  dv.setUint16(2, length, true);
  dv.setUint8(4, flags);
  dv.setUint8(5, clear.length);
  clear.forEach(([a, v], i) => {
    dv.setUint16(6 + 3 * i, a, true);
    dv.setUint8(8 + 3 * i, v);
  });
  return out;
}

interface Sample {
  timestampUs: bigint;
  m: Measurement;
  bins: HistogramBinData;
}

/** The Python `BridgeLink` + the stream half of `Vl53lx`, nothing else. */
class MiniBridge extends DepzDevice implements BridgeDevice {
  driver: VL53LX | null = null;
  readonly samples: Sample[] = [];
  parseErrors = 0;
  private waiter: (() => void) | null = null;

  private async call<T>(fn: () => Promise<T>): Promise<T> {
    try {
      return await fn();
    } catch (err) {
      if (err instanceof DepzError) throw new ProtocolError(String(err));
      throw err;
    }
  }

  async readReg(addr: number, length: number): Promise<Uint8Array> {
    const out = new Uint8Array(length);
    let off = 0;
    while (length > 0) {
      const n = Math.min(length, XFER_MAX);
      const rep = await this.call(() =>
        this.request(READ_REG, packReadReg(addr, n), {
          matcher: DepzDevice.expectReport(RPT_REG_DATA, unpackRegData),
          timeoutMs: 2000,
        }),
      );
      if (rep.data.length !== n) throw new ProtocolError(`READ_REG: expected ${n}, got ${rep.data.length}`);
      out.set(rep.data, off);
      off += n;
      addr += n;
      length -= n;
    }
    return out;
  }

  async writeReg(addr: number, data: Uint8Array): Promise<void> {
    let done = 0;
    while (done < data.length) {
      const chunk = data.subarray(done, done + XFER_MAX);
      await this.call(() =>
        this.request(WRITE_REG, packWriteReg(addr, chunk), { okCompletes: true, timeoutMs: 2000 }),
      );
      addr += chunk.length;
      done += chunk.length;
    }
  }

  setI2cSpeed(khz: number): Promise<void> {
    return this.call(() => this.request(SET_I2C_SPEED, packSetI2cSpeed(khz), { okCompletes: true }));
  }

  setAddrWidth(width: number): Promise<void> {
    return this.call(() =>
      this.request(SET_ADDR_WIDTH, Uint8Array.of(width), { okCompletes: true }),
    );
  }

  clearI2cErrors(): Promise<void> {
    return this.call(() =>
      this.request(CLEAR_I2C_ERRORS, new Uint8Array(0), { okCompletes: true }),
    );
  }

  xshut(action: number): Promise<void> {
    return this.call(() =>
      this.request(XSHUT, packXshut(action), { okCompletes: true, timeoutMs: 1000 }),
    );
  }

  /** Resolves once `n` samples have been decoded. */
  waitSamples(n: number): Promise<void> {
    if (this.samples.length >= n) return Promise.resolve();
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error(`only ${this.samples.length}/${n} frames`)), 10_000);
      this.waiter = () => {
        if (this.samples.length >= n) {
          clearTimeout(timer);
          this.waiter = null;
          resolve();
        }
      };
    });
  }

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== RPT_STREAM || pkt.payload.length < 12) return false;
    const drv = this.driver;
    if (drv === null) return true;
    const sample = unpackStream(pkt.payload);
    try {
      // Decoded exactly once per sample, synchronously in the handler.
      const bins = drv.binData(sample.data);
      const m = drv.toMeasurement(bins);
      this.samples.push({ timestampUs: sample.timestampUs, m, bins });
    } catch {
      this.parseErrors += 1;
    }
    this.waiter?.();
    return true;
  }
}

interface ExpectedFrame {
  timestamp_us: number;
  distance_mm: number;
  status: number;
  targets: Array<[number, number]>;
  bins: { bin_data: number[]; number_of_bins: number; vcsel_period: number; stream_count: number };
}

interface Expected {
  software_name: string;
  class: string;
  product: string;
  driver: string;
  budget_ms: number;
  mode: string;
  timing: [number, number];
  frames: ExpectedFrame[];
  refused?: Refused;
}

interface Refused {
  note: string;
  after_init: { modes: string[]; budget_ms: [number, number]; driver_reach_mm: number };
}

const CAPTURES = [
  "vl53l1cx_histogram_long_33ms",
  "vl53l1cb_histogram_medium_33ms",
  "vl53l3cx_histogram_medium_33ms",
  "vl53l4cx_histogram_medium_50ms",
  "vl53l4cx_histogram_short_33ms",
];

describe("vl53lx bare driver replay", () => {
  for (const stem of CAPTURES) {
    it(`${stem}: byte-identical session, every frame decoded`, async () => {
      const expected = JSON.parse(
        readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"),
      ) as Expected;
      expect(expected.driver).toBe("histogram");
      const replay = new ReplayTransport(
        readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"),
        { strictTx: true },
      );
      const dev = new MiniBridge(replay, { timeoutMs: 2000 });
      await dev.open();
      let timing: [number, number] | null = null;
      try {
        // discovery._identify (GET_NAME_ACTIVE_SOFTWARE), then _promote reads
        // the device name. The sensor type itself is the device class's
        // business; the request is what the byte-identical session needs.
        await dev.identify();
        const name = await dev.getDeviceName();
        expect(name).toContain(expected.product);
        expect(await dev.getSoftwareName()).toBe(expected.software_name);

        // Vl53lx.init(): the address width before anything else, then the ULD.
        const platform = new BridgePlatform(dev, () => Promise.resolve());
        const drv = new VL53LX(platform, expected.product);
        await dev.setAddrWidth(VL53LX.ADDR_WIDTH);
        await drv.sensorInit();
        await dev.clearI2cErrors();
        dev.driver = drv;

        // Vl53lx.configure(budget, mode).
        await drv.sensorInit();
        await dev.clearI2cErrors();
        if (expected.refused) {
          const after = expected.refused.after_init;
          expect([...drv.MODES]).toEqual(after.modes);
          expect([...drv.BUDGET_MS]).toEqual(after.budget_ms);
          expect(await drv.reachMm()).toBe(after.driver_reach_mm);
          const err = await drv.setMode(expected.mode).catch((e: unknown) => e);
          expect(err).toBeInstanceOf(Vl53Error);
          expect(String(err)).toMatch(/no such mode/);
          return;
        }
        await drv.setMode(expected.mode);
        await drv.setRangeTiming(expected.budget_ms, 0);
        timing = await drv.getRangeTiming();

        // Vl53lx.start_ranging().
        await drv.startRanging();
        const [addr, length] = drv.streamBlock();
        await dev.request(START_STREAM, packStartStream(addr, length, drv.CLEAR_STEPS), {
          okCompletes: true,
        });
        await dev.waitSamples(expected.frames.length);
        dev.driver = null; // anything past the expected count is not decoded

        // Vl53lx.stop_ranging().
        await dev.request(STOP_STREAM, undefined, { okCompletes: true });
        await drv.stopRanging();
      } finally {
        await dev.close();
      }

      expect(timing).toEqual(expected.timing);
      expect(dev.parseErrors).toBe(0);
      const frames = dev.samples.slice(0, expected.frames.length);
      expect(frames.length).toBe(expected.frames.length);
      for (let i = 0; i < frames.length; i++) {
        const got = frames[i]!;
        const want = expected.frames[i]!;
        const where = `${stem} frame ${i}`;
        expect(got.timestampUs, where).toBe(BigInt(want.timestamp_us));
        expect(got.m.distanceMm, where).toBe(want.distance_mm);
        expect(got.m.status, where).toBe(want.status);
        expect(got.m.targets.map((t) => [t.distanceMm, t.status]), where).toEqual(want.targets);
        const b = got.bins;
        expect(b.bin_data.slice(0, b.number_of_bins), where).toEqual(want.bins.bin_data);
        expect(b.vcsel_period, where).toBe(want.bins.vcsel_period);
        expect(b.result__stream_count, where).toBe(want.bins.stream_count);
      }
    }, 30_000);
  }
});
