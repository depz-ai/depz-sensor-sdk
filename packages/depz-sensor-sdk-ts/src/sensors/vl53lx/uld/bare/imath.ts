/**
 * uld/bare/imath — Python integer semantics on JS numbers, for the BareDriver
 * port. The Python reference (itself a port of ST's C) relies on arbitrary
 * precision ints, `//` flooring, `%` taking the sign of the divisor, and `<<`
 * / `>>` that never wrap. JS `<<`/`>>` are 32-bit and `/` is float, so every
 * such expression in core.ts / hist.ts goes through these helpers.
 *
 * Exactness: for integers |a| < 2^53 and b != 0, `Math.floor(a / b)` and
 * `Math.trunc(a / b)` are exact (the float quotient cannot round across an
 * integer unless |a| exceeds 2^53). Expressions that can leave that range —
 * the sigma estimate, the event-consistency scaler, the per-SPAD rate — are
 * done in BigInt where they are used.
 *
 * Division or modulo by zero throws, as Python raises ZeroDivisionError.
 */

export class ZeroDivisionError extends Error {}

function checkDivisor(b: number): void {
  if (b === 0) throw new ZeroDivisionError("integer division or modulo by zero");
}

/** Python `a // b` (floor). */
export function floorDiv(a: number, b: number): number {
  checkDivisor(b);
  return Math.floor(a / b);
}

/** Python `a % b` (sign of the divisor). */
export function pymod(a: number, b: number): number {
  checkDivisor(b);
  const r = a % b;
  return r !== 0 && r < 0 !== b < 0 ? r + b : r;
}

/** C `/` on integers: truncates towards zero (hist.py `cdiv`). */
export function cdiv(a: number, b: number): number {
  checkDivisor(b);
  return Math.trunc(a / b);
}

/** Python `a << n` without the 32-bit wrap. */
export function shl(a: number, n: number): number {
  return a * 2 ** n;
}

/** Python `a >> n` (floors, also for negatives) without the 32-bit wrap. */
export function shr(a: number, n: number): number {
  return Math.floor(a / 2 ** n);
}

/** `math.isqrt` on a non-negative safe integer. */
export function isqrtNum(n: number): number {
  if (n < 0) throw new RangeError("isqrt() argument must be nonnegative");
  let r = Math.floor(Math.sqrt(n));
  while (r * r > n) r--;
  while ((r + 1) * (r + 1) <= n) r++;
  return r;
}

/** `math.isqrt` on a non-negative BigInt. */
export function isqrtBig(n: bigint): bigint {
  if (n < 0n) throw new RangeError("isqrt() argument must be nonnegative");
  if (n < 2n) return n;
  // Newton from a float seed, then settle.
  let x = BigInt(Math.floor(Math.sqrt(Number(n))));
  if (x === 0n) x = 1n;
  for (;;) {
    const y = (x + n / x) >> 1n;
    if (y >= x) break;
    x = y;
  }
  while (x * x > n) x--;
  while ((x + 1n) * (x + 1n) <= n) x++;
  return x;
}

/** Python `abs` / `min` / `max` on BigInt. */
export function babs(a: bigint): bigint {
  return a < 0n ? -a : a;
}
export function bmin(a: bigint, b: bigint): bigint {
  return a < b ? a : b;
}

/** C-style truncating division on BigInt — BigInt `/` already truncates. */
export function bcdiv(a: bigint, b: bigint): bigint {
  if (b === 0n) throw new ZeroDivisionError("integer division or modulo by zero");
  return a / b;
}

/** Python `//` on BigInt (floor). */
export function bfloorDiv(a: bigint, b: bigint): bigint {
  if (b === 0n) throw new ZeroDivisionError("integer division or modulo by zero");
  const q = a / b;
  return (a % b !== 0n) && ((a < 0n) !== (b < 0n)) ? q - 1n : q;
}
