import { describe, expect, it } from "vitest";
import { buildPacket, PacketParser, type ParserEvent } from "../../src/index.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const encodeVectors = loadVectors("framing_encode.json");
const decodeVectors = loadVectors("framing_decode.json");

describe("framing encode vectors", () => {
  for (const c of encodeVectors.cases) {
    it(c.name, () => {
      const frame = buildPacket(c.cmd, fromHex(c.payload), c.seq, c.crc_type);
      expect(toHex(frame)).toBe(c.frame);
    });
  }
});

interface Collected {
  events: Array<Record<string, unknown>>;
  trash: string;
  residue: string;
  header_errors: number;
}

function collect(parser: PacketParser, chunks: Uint8Array[]): Collected {
  const events: Array<Record<string, unknown>> = [];
  const trash: number[] = [];
  for (const chunk of chunks) {
    for (const ev of parser.feed(chunk) as ParserEvent[]) {
      if (ev.type === "packet") {
        events.push({ type: "packet", cmd: ev.cmd, seq: ev.seq, payload: toHex(ev.payload) });
      } else if (ev.type === "crcError") {
        events.push({ type: "crc_error", cmd: ev.cmd, seq: ev.seq });
      } else {
        trash.push(...ev.data);
      }
    }
  }
  return {
    events,
    trash: toHex(Uint8Array.from(trash)),
    residue: toHex(parser.residue),
    header_errors: parser.headerErrors,
  };
}

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
    });
  }
});
