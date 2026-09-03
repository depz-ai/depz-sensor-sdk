import { readFileSync } from "node:fs";
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
