/**
 * Minimal base64 decoder for the generated VL53L8 blob assets. Pure JS —
 * identical behavior in browser and Node, no `atob`/`Buffer` dependency.
 */

const ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const LUT: Int16Array = (() => {
  const t = new Int16Array(128).fill(-1);
  for (let i = 0; i < ALPHABET.length; i++) t[ALPHABET.charCodeAt(i)] = i;
  return t;
})();

/** Decode standard (padded) base64 into bytes. */
export function decodeBase64(b64: string): Uint8Array {
  let end = b64.length;
  while (end > 0 && b64.charCodeAt(end - 1) === 0x3d /* '=' */) end -= 1;
  const outLen = Math.floor((end * 3) / 4);
  const out = new Uint8Array(outLen);
  let acc = 0;
  let bits = 0;
  let o = 0;
  for (let i = 0; i < end; i++) {
    const v = LUT[b64.charCodeAt(i) & 0x7f] ?? -1;
    if (v < 0) throw new Error(`invalid base64 character at index ${i}`);
    acc = (acc << 6) | v;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out[o++] = (acc >>> bits) & 0xff;
    }
  }
  return out;
}
