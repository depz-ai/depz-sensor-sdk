/**
 * VL53Lx `configure({ signalKcps })` over committed real-hardware captures
 * (APP_VL53L0_4_v0.24, no device). The captured sessions themselves — every
 * request byte-identical, every frame decoded, the refused mode — are asserted
 * by contract 15 scenarios (test/scenarios.test.ts); this keeps what a
 * scenario cannot express: a call the capture does not contain, after the
 * captured init replayed strictly.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { DepzError, ReplayTransport } from "../src/index.js";
import { VL53LX_CLASS_BY_PRODUCT, Vl53lx } from "../src/sensors/vl53lx/vl53lx.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");

const CLASSES: Record<string, typeof Vl53lx> = { Vl53lx };
for (const C of Object.values(VL53LX_CLASS_BY_PRODUCT)) CLASSES[C.name] = C;

interface Expected {
  class: string;
  product_arg?: string | null;
  driver: string;
  budget_ms: number;
  mode: string | null;
  timing: [number, number];
}

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
