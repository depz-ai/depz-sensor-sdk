import { appendFileSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const VECTORS_DIR = join(
  dirname(fileURLToPath(import.meta.url)),
  "../../../../contracts/vectors",
);

export function loadVectors<T = any>(name: string): T {
  return JSON.parse(readFileSync(join(VECTORS_DIR, name), "utf8")) as T;
}

export function fromHex(hex: string): Uint8Array {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) {
    out[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  }
  return out;
}

export function toHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
}

/**
 * Report a golden-vector case as asserted, for scripts/check_vector_coverage.py.
 * Call it after the case's last expect(), so a failing case is never reported.
 * No-op unless $DEPZ_VECTOR_SEEN names the file to append to; each ID is one
 * O_APPEND write, so parallel vitest workers never interleave partial lines.
 */
export function seen(c: string | { id: string }): void {
  const file = process.env.DEPZ_VECTOR_SEEN;
  if (!file) return;
  appendFileSync(file, `${typeof c === "string" ? c : c.id}\n`);
}

/**
 * For a group asserted across several `it`s (one vector ID, many sub-cases):
 * returns a callback to invoke after each sub-case passes; the ID is reported
 * once all `count` have.
 */
export function seenAfter(id: string, count: number): () => void {
  let left = count;
  return () => {
    if (--left === 0) seen(id);
  };
}
