/**
 * Wire-field checks shared by the codecs (contract 07 §3).
 *
 * Encoding: every argument is checked against its wire width before a byte
 * is written — never truncated, wrapped or masked (a `RangeError`, which the
 * SDK reports as `invalid_argument`). Decoding: a payload shorter than the
 * fields the contract defines is rejected (`RangeError`); trailing bytes are
 * ignored by the callers.
 */

/** `value` as an unsigned integer that fits `bits` (≤ 53) bits, else RangeError. */
export function checkUint(name: string, value: number, bits: number): number {
  if (!Number.isInteger(value) || value < 0 || value > 2 ** bits - 1) {
    throw new RangeError(`${name}: ${value} out of range for u${bits}`);
  }
  return value;
}

/** `value` as a signed integer that fits `bits` (≤ 32) bits, else RangeError. */
export function checkInt(name: string, value: number, bits: number): number {
  const lim = 2 ** (bits - 1);
  if (!Number.isInteger(value) || value < -lim || value > lim - 1) {
    throw new RangeError(`${name}: ${value} out of range for i${bits}`);
  }
  return value;
}

/** `value` as an unsigned 64-bit integer, else RangeError. */
export function checkU64(name: string, value: bigint): bigint {
  if (value < 0n || value > 0xffff_ffff_ffff_ffffn) {
    throw new RangeError(`${name}: ${value} out of range for u64`);
  }
  return value;
}

/**
 * A u64 carried in a JS `number` (a µs timestamp): a non-negative integer
 * below 2^64 (exact only up to 2^53, which real timestamps never reach), else
 * RangeError.
 */
export function checkU64Number(name: string, value: number): number {
  if (!Number.isInteger(value) || value < 0 || value >= 2 ** 64) {
    throw new RangeError(`${name}: ${value} out of range for u64`);
  }
  return value;
}

/** Reject a device payload shorter than the `n` bytes its fields need. */
export function needBytes(what: string, payload: Uint8Array, n: number): void {
  if (payload.length < n) {
    throw new RangeError(`${what}: need ${n} bytes, got ${payload.length}`);
  }
}
