/**
 * VL53L8CX init over the committed real-hardware capture (no device): the
 * firmware-download progress callback. The session itself — every request
 * byte-identical, every frame decoded, the init transcripts — is asserted by
 * contract 15 scenarios (test/scenarios.test.ts); this keeps what a scenario
 * cannot express.
 *
 * `sleepImpl` is instant: the ULD boot/poll sleeps total ~15 s in real time.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";
import { ReplayTransport } from "../src/index.js";
import { Vl53l8 } from "../src/sensors/vl53l8/vl53l8.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDINGS = path.resolve(here, "../../../contracts/vectors/recordings");
const FIXTURE = path.join(RECORDINGS, "vl53l8_8x8_15hz_3s.depzrec");

describe("vl53l8 full-stack replay", () => {
  it("reports firmware download as one forward-only progress ramp", async () => {
    // The ~84 KB firmware is written in three banks; each used to report its
    // own 0→100, so the bar jumped backwards twice. Progress must now be
    // cumulative over the whole download: one constant total, done only ever
    // increasing, ending exactly at total.
    const replay = new ReplayTransport(readFileSync(FIXTURE, "utf8"), { strictTx: true });
    const dev = new Vl53l8(replay, { timeoutMs: 2000, sleepImpl: () => Promise.resolve() });
    await dev.open();
    const calls: Array<[number, number]> = [];
    try {
      await dev.identify();
      await dev.getSoftwareName();
      await dev.init("cx", { writeProgress: (done, total) => calls.push([done, total]) });
    } finally {
      await dev.close();
    }
    expect(calls.length).toBeGreaterThan(3); // multi-chunk, multi-bank
    const total = calls[0]![1];
    expect(total).toBe(0x15000); // full firmware size, not a per-bank size
    expect(calls.every(([, t]) => t === total)).toBe(true); // one constant total
    for (let i = 1; i < calls.length; i++) {
      expect(calls[i]![0]).toBeGreaterThanOrEqual(calls[i - 1]![0]); // forward-only
    }
    expect(calls.some(([d]) => d > 0x8000)).toBe(true); // spans past bank 1 → cumulative
    expect(calls[calls.length - 1]![0]).toBe(total); // ends at 100 %
  });
});
