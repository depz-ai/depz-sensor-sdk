/**
 * Integer helpers for the ULD ports: Python-int semantics on JS numbers.
 *
 * The Python ports compute with unbounded ints and wrap on purpose where the
 * C driver does (`& 0xFFFFFFFF`). JS bitwise operators work on 32-bit signed
 * values, so any shift or mask that may see a value outside int32 goes
 * through these helpers instead. Every value they take stays below 2^53
 * (exact in a double); the sigma estimate of the VL53L0X, which can exceed
 * that, runs on BigInt in `l0x.ts`.
 */

const TWO32 = 4294967296;

/** C uint32 wrap-around (`value & 0xFFFFFFFF` in Python, negatives included). */
export function u32(value: number): number {
  const r = value % TWO32;
  return r < 0 ? r + TWO32 : r;
}

/** C int16 truncation (an assignment to an int16_t). */
export function i16(value: number): number {
  const r = u32(value) & 0xffff;
  return r & 0x8000 ? r - 0x10000 : r;
}

/** Python `a // b` (floor division). */
export function floorDiv(a: number, b: number): number {
  return Math.floor(a / b);
}

/** Python `a >> n` for any safe integer (floor semantics). */
export function shr(a: number, n: number): number {
  return Math.floor(a / 2 ** n);
}

/** Python `a << n` for any safe integer. */
export function shl(a: number, n: number): number {
  return a * 2 ** n;
}

/** Python `round(x)`: half to even. */
export function pyRound(x: number): number {
  if (Math.abs(x % 1) === 0.5) return 2 * Math.round(x / 2);
  return Math.round(x);
}

/**
 * C's `round()`: an exact .5 goes away from zero. ST's ULD rounds xtalk this
 * way; Python's `round()` and JS's `Math.round()` don't (0.5 → 0, -2.5 → -2).
 */
export function roundHalfAway(x: number): number {
  const t = Math.trunc(x);
  if (x - t >= 0.5) return t + 1;
  if (x - t <= -0.5) return t - 1;
  return t;
}

/** Big-endian unsigned integer of `b[start:end]`. */
export function beUint(b: Uint8Array, start: number, end: number): number {
  let v = 0;
  for (let i = start; i < end; i++) v = v * 256 + b[i]!;
  return v;
}
