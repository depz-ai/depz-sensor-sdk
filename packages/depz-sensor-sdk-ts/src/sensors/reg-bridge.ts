/**
 * Chunked register-bridge transfers, shared by every VL53-family driver.
 *
 * The ST ULD platform reads/writes registers over the MCU's register bridge in
 * bounded chunks (2048 B for VL53L8's fast firmware upload, 253 B for VL53L4CD).
 * Every driver in the family needs the identical chunk loop; keeping one copy
 * here means a fix (e.g. a timeout tweak) lands for all of them at once instead
 * of drifting per driver. The per-driver bits — which opcode/report to use, the
 * chunk size, an optional device-timestamp sink, write-progress reporting — are
 * passed in as callbacks.
 */

import { DepzError } from "../errors.js";

export interface ChunkedReadOptions {
  /** Max bytes per register read. */
  chunkSize: number;
  /** Read exactly `n` bytes at `addr` over the bridge (one request). */
  readChunk: (addr: number, n: number) => Promise<{ data: Uint8Array; timestampUs?: bigint }>;
  /** Sink for each chunk's device timestamp (VL53L4CD poll-mode measurements). */
  onTimestamp?: (timestampUs: bigint) => void;
}

/** Read `size` bytes from `addr`, chunked; throws `DepzError` on a short reply. */
export async function chunkedRead(
  addr: number,
  size: number,
  opts: ChunkedReadOptions,
): Promise<Uint8Array> {
  const out = new Uint8Array(size);
  let off = 0;
  while (size > 0) {
    const n = Math.min(size, opts.chunkSize);
    const rep = await opts.readChunk(addr, n);
    if (rep.data.length !== n) {
      throw new DepzError(
        `READ_REG 0x${addr.toString(16).toUpperCase().padStart(4, "0")}: ` +
          `expected ${n}, got ${rep.data.length}`,
      );
    }
    if (rep.timestampUs !== undefined) opts.onTimestamp?.(rep.timestampUs);
    out.set(rep.data, off);
    off += n;
    addr += n;
    size -= n;
  }
  return out;
}

export interface ChunkedWriteOptions {
  /** Max bytes per register write. */
  chunkSize: number;
  /** Write one chunk at `addr` over the bridge (one request). */
  writeChunk: (addr: number, chunk: Uint8Array) => Promise<void>;
  /** Called after each chunk with cumulative bytes written / total for this call. */
  onProgress?: (done: number, total: number) => void;
}

/** Write `data` to `addr`, chunked. */
export async function chunkedWrite(
  addr: number,
  data: Uint8Array,
  opts: ChunkedWriteOptions,
): Promise<void> {
  let done = 0;
  while (done < data.length) {
    const chunk = data.subarray(done, done + opts.chunkSize);
    await opts.writeChunk(addr, chunk);
    addr += chunk.length;
    done += chunk.length;
    opts.onProgress?.(done, data.length);
  }
}
