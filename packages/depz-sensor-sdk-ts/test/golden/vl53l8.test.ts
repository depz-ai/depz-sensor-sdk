/**
 * vl53l8.json (contract 04 §2–§4, the streaming side of the SPI register
 * bridge): START_STREAM encode, RPT_VL53_FRAME chunk decode and the
 * FrameReassembler rules over chunks a live board emitted.
 */
import { describe, expect, it } from "vitest";
import { Vl53l8Rpt, packStartStream, unpackFrameChunk } from "../../src/protocol/vl53l8.js";
import { plain, vl53l8Decode, vl53l8Reassembly } from "./shapes.js";
import { fromHex, loadVectors, seen, toHex } from "./vectors.js";

const v = loadVectors<any>("vl53l8.json");

describe("vl53l8.json", () => {
  for (const c of v.encode) it(`encode ${c.name}`, () => {
    expect(c.kind).toBe("start_stream");
    expect(toHex(packStartStream(c.frame_size))).toBe(c.payload);
    seen(c);
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    expect(c.report).toBe(Vl53l8Rpt.Vl53Frame);
    expect(plain(vl53l8Decode(c.report, fromHex(c.payload)))).toEqual(c.expect);
    seen(c);
  });

  it("RPT_VL53_FRAME rejects a short payload", () => {
    expect(() => unpackFrameChunk(new Uint8Array(11))).toThrow();
    expect(unpackFrameChunk(new Uint8Array(12)).data).toHaveLength(0);
  });

  for (const c of v.reassembly) it(`reassembly ${c.name}`, () => {
    expect(plain(vl53l8Reassembly(c.chunks.map(fromHex)))).toEqual(c.expect);
    seen(c);
  });
});
