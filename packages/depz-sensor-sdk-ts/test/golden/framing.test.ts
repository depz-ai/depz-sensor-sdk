import { describe, expect, it } from "vitest";
import { buildPacket, PacketParser } from "../../src/index.js";
import { framingCollect } from "./shapes.js";
import { fromHex, loadVectors, toHex, seen } from "./vectors.js";

const encodeVectors = loadVectors("framing_encode.json");
const decodeVectors = loadVectors("framing_decode.json");

describe("framing encode vectors", () => {
  for (const c of encodeVectors.cases) {
    it(c.name, () => {
      const frame = buildPacket(c.cmd, fromHex(c.payload), c.seq, c.crc_type);
      expect(toHex(frame)).toBe(c.frame);
      seen(c);
    });
  }
});

const collect = framingCollect;

function randomChunks(stream: Uint8Array, seedState: { s: number }): Uint8Array[] {
  // Tiny LCG so the test is deterministic without extra deps.
  const next = () => {
    seedState.s = (seedState.s * 1664525 + 1013904223) >>> 0;
    return seedState.s;
  };
  const out: Uint8Array[] = [];
  let i = 0;
  while (i < stream.length) {
    const n = (next() % 37) + 1;
    out.push(stream.subarray(i, i + n));
    i += n;
  }
  return out;
}

describe("framing decode vectors — chunking invariance", () => {
  for (const c of decodeVectors.cases) {
    it(c.name, () => {
      const stream = fromHex(c.stream);
      const expected = c.expect;

      const whole = collect(new PacketParser(), [stream]);
      expect(whole).toEqual(expected);

      const byteChunks: Uint8Array[] = [];
      for (let i = 0; i < stream.length; i++) byteChunks.push(stream.subarray(i, i + 1));
      const bytewise = collect(new PacketParser(), byteChunks);
      expect(bytewise).toEqual(expected);

      const seed = { s: 0xde92 };
      for (let round = 0; round < 3; round++) {
        const split = collect(new PacketParser(), randomChunks(stream, seed));
        expect(split).toEqual(expected);
      }
      seen(c);
    });
  }
});
