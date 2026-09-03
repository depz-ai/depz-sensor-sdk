import { describe, expect, it } from "vitest";
import {
  packEchoDecay,
  packSamplePeriod,
  unpackEchoDecay,
  unpackSamplePeriod,
  unpackSr04Data,
} from "../../src/index.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const data = loadVectors("sr04.json");

describe("sr04 encode vectors", () => {
  for (const c of data.encode) {
    it(c.name, () => {
      if (c.kind === "set_sample_period") {
        expect(toHex(packSamplePeriod(c.period_us))).toBe(c.payload);
      } else if (c.kind === "set_echo_decay") {
        expect(toHex(packEchoDecay(c.decay_us))).toBe(c.payload);
      } else {
        throw new Error(c.kind);
      }
    });
  }
});

describe("sr04 decode vectors", () => {
  for (const c of data.decode) {
    it(c.name, () => {
      const payload = fromHex(c.payload);
      if (c.report === 0x91) {
        const d = unpackSr04Data(payload);
        expect(d.sourceCmd).toBe(c.expect.source_cmd);
        expect(d.timestampUs).toBe(BigInt(c.expect.timestamp_us));
        expect(d.echoTimeUs).toBe(c.expect.echo_time_us);
      } else if (c.report === 0x92) {
        expect(unpackSamplePeriod(payload)).toBe(c.expect.period_us);
      } else if (c.report === 0x93) {
        expect(unpackEchoDecay(payload)).toBe(c.expect.decay_us);
      } else {
        throw new Error(String(c.report));
      }
    });
  }
});
