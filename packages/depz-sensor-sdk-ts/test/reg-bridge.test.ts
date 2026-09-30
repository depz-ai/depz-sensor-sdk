/**
 * chunkedRead / chunkedWrite — the shared register-bridge chunk loops used by
 * every VL53-family driver. Locks the chunking, assembly, short-reply guard,
 * timestamp sink and progress reporting so a future driver can rely on them.
 */

import { describe, expect, it } from "vitest";

import { DepzError } from "../src/index.js";
import { chunkedRead, chunkedWrite } from "../src/sensors/reg-bridge.js";

describe("chunkedRead", () => {
  it("splits into chunkSize reads, assembles in address order", async () => {
    const calls: Array<{ addr: number; n: number }> = [];
    const out = await chunkedRead(0x1000, 5, {
      chunkSize: 2,
      readChunk: async (addr, n) => {
        calls.push({ addr, n });
        // Return bytes that encode their absolute offset so order is verifiable.
        return { data: Uint8Array.from({ length: n }, (_, i) => (addr - 0x1000 + i) & 0xff) };
      },
    });
    expect(calls).toEqual([
      { addr: 0x1000, n: 2 },
      { addr: 0x1002, n: 2 },
      { addr: 0x1004, n: 1 },
    ]);
    expect([...out]).toEqual([0, 1, 2, 3, 4]);
  });

  it("throws DepzError when a reply is short", async () => {
    await expect(
      chunkedRead(0x2ffc, 4, {
        chunkSize: 4,
        readChunk: async () => ({ data: new Uint8Array(2) }),
      }),
    ).rejects.toBeInstanceOf(DepzError);
  });

  it("forwards each chunk's device timestamp to onTimestamp", async () => {
    const seen: bigint[] = [];
    await chunkedRead(0, 4, {
      chunkSize: 2,
      readChunk: async (addr) => ({ data: new Uint8Array(2), timestampUs: BigInt(addr + 7) }),
      onTimestamp: (ts) => seen.push(ts),
    });
    expect(seen).toEqual([7n, 9n]);
  });
});

describe("chunkedWrite", () => {
  it("splits into chunkSize writes at advancing addresses", async () => {
    const calls: Array<{ addr: number; bytes: number[] }> = [];
    await chunkedWrite(0x40, Uint8Array.from([1, 2, 3, 4, 5]), {
      chunkSize: 2,
      writeChunk: async (addr, chunk) => {
        calls.push({ addr, bytes: [...chunk] });
      },
    });
    expect(calls).toEqual([
      { addr: 0x40, bytes: [1, 2] },
      { addr: 0x42, bytes: [3, 4] },
      { addr: 0x44, bytes: [5] },
    ]);
  });

  it("reports cumulative progress with the full length as total", async () => {
    const progress: Array<[number, number]> = [];
    await chunkedWrite(0, new Uint8Array(5), {
      chunkSize: 2,
      writeChunk: async () => {},
      onProgress: (done, total) => progress.push([done, total]),
    });
    expect(progress).toEqual([
      [2, 5],
      [4, 5],
      [5, 5],
    ]);
  });
});
