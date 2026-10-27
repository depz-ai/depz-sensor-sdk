/**
 * `FrameReassembler` (protocol/vl53l8.ts): RPT_VL53_FRAME chunks into whole
 * frames — the happy paths the replay tests take, and the discards they
 * never hit: a restart mid-frame, a gap, a size change, an overrun.
 */

import { describe, expect, it } from "vitest";
import { FrameReassembler, type FrameChunk } from "../src/protocol/vl53l8.js";

function chunk(offset: number, data: number[], fullSize: number, ts = 1n): FrameChunk {
  return { timestampUs: ts, fullSize, offset, data: Uint8Array.from(data) };
}

describe("FrameReassembler", () => {
  it("completes a one-chunk frame and a multi-chunk frame, keeping the first timestamp", () => {
    const r = new FrameReassembler();
    expect(r.feed(chunk(0, [1, 2, 3], 3, 7n))).toEqual({ timestampUs: 7n, frame: Uint8Array.from([1, 2, 3]) });
    expect(r.feed(chunk(0, [1, 2], 4, 10n))).toBeNull();
    expect(r.feed(chunk(2, [3, 4], 4, 11n))).toEqual({ timestampUs: 10n, frame: Uint8Array.from([1, 2, 3, 4]) });
    expect([r.completed, r.discarded]).toEqual([2, 0]);
  });

  it("a new first chunk while a frame is in progress discards the partial one", () => {
    const r = new FrameReassembler();
    expect(r.feed(chunk(0, [1, 2], 4))).toBeNull();
    expect(r.feed(chunk(0, [9, 9, 9, 9], 4))).toEqual({ timestampUs: 1n, frame: Uint8Array.from([9, 9, 9, 9]) });
    expect([r.completed, r.discarded]).toEqual([1, 1]);
  });

  it("a gap, a size change or a continuation with nothing pending drops the frame", () => {
    const r = new FrameReassembler();
    expect(r.feed(chunk(0, [1, 2], 4))).toBeNull();
    expect(r.feed(chunk(3, [4], 4))).toBeNull(); // offset 3 ≠ 2 bytes held
    expect(r.discarded).toBe(1);
    expect(r.feed(chunk(0, [1, 2], 4))).toBeNull();
    expect(r.feed(chunk(2, [3, 4], 6))).toBeNull(); // fullSize changed mid-frame
    expect(r.discarded).toBe(2);
    expect(r.feed(chunk(2, [3, 4], 4))).toBeNull(); // nothing pending: not counted
    expect(r.discarded).toBe(2);
    // the next frame is unaffected
    expect(r.feed(chunk(0, [5, 6, 7, 8], 4))).not.toBeNull();
    expect(r.completed).toBe(1);
  });

  it("more bytes than the frame announced is an overrun, discarded", () => {
    const r = new FrameReassembler();
    expect(r.feed(chunk(0, [1, 2, 3, 4, 5], 4))).toBeNull();
    expect([r.completed, r.discarded]).toEqual([0, 1]);
    expect(r.feed(chunk(0, [1, 2], 3))).toBeNull();
    expect(r.feed(chunk(2, [3, 4], 3))).toBeNull(); // 4 bytes held for a 3-byte frame
    expect(r.discarded).toBe(2);
  });

  it("a zero-size frame never completes", () => {
    const r = new FrameReassembler();
    expect(r.feed(chunk(0, [], 0))).toBeNull();
    expect([r.completed, r.discarded]).toEqual([0, 0]);
  });
});
