/**
 * Full 1D-family stack over committed real-hardware captures (no device).
 * Mirror of the Python `tests/test_vl53lx_replay.py`.
 *
 * Recorded 2026-09-28 from live boards on APP_VL53L0_4_v0.24 by the Python
 * SDK: identify, device name (picks the product class), software name,
 * init(driver), configure(budget, mode), stream N frames. strictTx makes the
 * whole ULD — boot polls, NVM reads, the histogram preset — re-issue
 * byte-identical requests; every decoded frame (distance, status, targets,
 * histogram bins) must match the sidecar. The histogram driver is stateful
 * (A/B frame pairs). A sidecar with `refused` is a capture of a configuration
 * the driver no longer accepts (the short preset on an L4 die): init must
 * still replay, and configure must refuse the mode.
 */

import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { DepzError, ReplayTransport } from "../src/index.js";
import type { HistogramBinData } from "../src/sensors/vl53lx/uld/bare/core.js";
import {
  VL53LX_CLASS_BY_PRODUCT,
  Vl53lx,
  resolveVl53lxClass,
  type Vl53lxMeasurement,
} from "../src/sensors/vl53lx/vl53lx.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");
const FAMILY = ["vl53l0x_", "vl53l1cx_", "vl53l1cb_", "vl53l3cx_", "vl53l4cx_", "vl53lx_l4cd_"];

const CLASSES: Record<string, typeof Vl53lx> = { Vl53lx };
for (const C of Object.values(VL53LX_CLASS_BY_PRODUCT)) CLASSES[C.name] = C;

const CAPTURES = readdirSync(RECORDINGS)
  .filter((f) => f.endsWith(".expected.json") && FAMILY.some((p) => f.startsWith(p)))
  .map((f) => f.slice(0, -".expected.json".length))
  .sort();

interface ExpectedFrame {
  timestamp_us: number;
  raw: string;
  distance_mm: number;
  status: number;
  targets: [number, number][];
  bins?: { bin_data: number[]; vcsel_period: number; stream_count: number };
}

interface Expected {
  software_name: string;
  class: string;
  product: string;
  product_arg?: string | null;
  driver: string;
  budget_ms: number;
  mode: string | null;
  timing: [number, number];
  frames: ExpectedFrame[];
  refused?: Refused;
}

interface Refused {
  note: string;
  after_init: { modes: string[]; budget_ms: [number, number]; driver_reach_mm: number };
}

describe("vl53lx full-stack replay", () => {
  it("finds the ten captures", () => {
    expect(CAPTURES.length).toBe(10);
  });

  for (const stem of CAPTURES) {
    it(`${stem}: byte-identical session, every frame decoded`, async () => {
      const expected = JSON.parse(
        readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"),
      ) as Expected;
      const replay = new ReplayTransport(
        readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"),
        { strictTx: true },
      );
      // The class under test is the one the capture resolved; the request
      // sequence mirrors the Python capture (_identify reads the software
      // name, _promote reads the device name to pick the class, then the test
      // reads the software name again).
      const Cls = CLASSES[expected.class];
      expect(Cls).toBeDefined();
      const dev: Vl53lx = new Cls!(replay, { timeoutMs: 2000, sleepImpl: () => Promise.resolve() });
      await dev.open();
      const frames: Vl53lxMeasurement[] = [];
      try {
        expect((await dev.identify()).sensorType).toBe("vl53lx");
        // The firmware name cannot tell the six products apart — the name does.
        expect(resolveVl53lxClass(null, await dev.getDeviceName()).name).toBe(expected.class);
        expect(await dev.getSoftwareName()).toBe(expected.software_name);
        await dev.init(expected.driver, { product: expected.product_arg ?? undefined });
        expect(dev.product).toBe(expected.product);
        const refused = expected.refused;
        if (refused) {
          const after = refused.after_init;
          expect([...dev.modes]).toEqual(after.modes);
          expect([...dev.driver.BUDGET_MS]).toEqual(after.budget_ms);
          expect(await dev.driver.reachMm()).toBe(after.driver_reach_mm);
          await expect(
            dev.configure({ budgetMs: expected.budget_ms, mode: expected.mode }),
          ).rejects.toThrow(/no such mode/);
          return;
        }
        await dev.configure({ budgetMs: expected.budget_ms, mode: expected.mode });
        expect(await dev.getRangeTiming()).toEqual(expected.timing);
        const stream = dev.measurements(expected.frames.length + 8);
        await dev.startRanging();
        for await (const m of stream) {
          frames.push(m);
          if (frames.length === expected.frames.length) break;
        }
        await dev.stopRanging();
      } finally {
        await dev.close();
      }

      expect(dev.streamParseErrors).toBe(0);
      expect(frames.length).toBe(expected.frames.length);
      for (let i = 0; i < frames.length; i++) {
        const got = frames[i]!;
        const want = expected.frames[i]!;
        expect(got.timestampUs).toBe(BigInt(want.timestamp_us));
        expect(got.distanceMm).toBe(want.distance_mm);
        expect(got.status).toBe(want.status);
        expect(got.targets.map((t) => [t.distanceMm, t.status])).toEqual(want.targets);
        if (want.bins !== undefined) {
          const b = got.bins as HistogramBinData;
          expect(b.bin_data.slice(0, b.number_of_bins)).toEqual(want.bins.bin_data);
          expect(b.vcsel_period).toBe(want.bins.vcsel_period);
          expect(b.result__stream_count).toBe(want.bins.stream_count);
        } else {
          expect(got.bins).toBeNull();
        }
      }
    }, 30_000);
  }
});

async function replayToInit(stem: string): Promise<[Vl53lx, Expected]> {
  const expected = JSON.parse(
    readFileSync(path.join(RECORDINGS, `${stem}.expected.json`), "utf8"),
  ) as Expected;
  const replay = new ReplayTransport(readFileSync(path.join(RECORDINGS, `${stem}.depzrec`), "utf8"), {
    strictTx: true,
  });
  const dev: Vl53lx = new CLASSES[expected.class]!(replay, {
    timeoutMs: 2000,
    sleepImpl: () => Promise.resolve(),
  });
  await dev.open();
  await dev.identify();
  await dev.getDeviceName();
  await dev.getSoftwareName();
  await dev.init(expected.driver, { product: expected.product_arg ?? undefined });
  return [dev, expected];
}

describe("vl53lx configure({ signalKcps })", () => {
  it("goes on after the re-init (which puts the blob's threshold back)", async () => {
    // The capture has no such write: the driver call is recorded instead,
    // after the re-init and timing replayed strictly.
    const [dev, expected] = await replayToInit("vl53l1cx_uld_long_33ms");
    const calls: number[] = [];
    (dev.driver as unknown as { setSignalThreshold: (k: number) => Promise<void> }).setSignalThreshold =
      async (k) => {
        calls.push(k);
      };
    try {
      await dev.configure({ budgetMs: expected.budget_ms, mode: expected.mode, signalKcps: 512 });
      expect(calls).toEqual([512]);
      expect(await dev.getRangeTiming()).toEqual(expected.timing);
    } finally {
      await dev.close();
    }
  }, 30_000);

  it("is refused before the re-init on a driver without it (L0X)", async () => {
    // No traffic on the refusal, so the capture's own configure still replays.
    const [dev, expected] = await replayToInit("vl53l0x_uld_long-range_33ms");
    try {
      await expect(
        dev.configure({ budgetMs: expected.budget_ms, mode: expected.mode, signalKcps: 512 }),
      ).rejects.toThrow(DepzError);
      await dev.configure({ budgetMs: expected.budget_ms, mode: expected.mode });
      expect(await dev.getRangeTiming()).toEqual(expected.timing);
    } finally {
      await dev.close();
    }
  }, 30_000);
});
