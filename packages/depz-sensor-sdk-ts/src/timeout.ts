/**
 * The timeouts a caller passes in milliseconds (`request`, `next`, the BNO086
 * waits, the backend transport's open ceiling): checked before anything is
 * written (contract 07 §3), then armed as a timer the platform can run.
 *
 * `Infinity` is a legal "no timeout" — the wait ends only with its item, the
 * close or the loss, as `Duration::MAX` in Rust and `Timeout.InfiniteTimeSpan`
 * in C#. A JS timer takes at most 2^31 − 1 ms (24.8 days); past that
 * `setTimeout` fires after 1 ms, so a longer finite delay is capped there.
 * A negative or NaN value is a RangeError (`invalid_argument`).
 */

/** Longest delay a JS timer takes; a longer one is capped here. */
export const MAX_TIMER_MS = 2 ** 31 - 1;

/** `value` as a timeout: a number ≥ 0 (`Infinity` = no timeout), else RangeError. */
export function checkTimeoutMs(name: string, value: number): number {
  if (typeof value !== "number" || Number.isNaN(value) || value < 0) {
    throw new RangeError(`${name}: ${String(value)} is not a timeout (ms >= 0, or Infinity for none)`);
  }
  return value;
}

/**
 * Arm `fn` after `timeoutMs` (checked as `checkTimeoutMs`): null for
 * `Infinity` — no timer at all — else the timer, its delay capped at
 * `MAX_TIMER_MS`. Clear it with `clearTimer`.
 */
export function startTimeout(
  name: string,
  fn: () => void,
  timeoutMs: number,
): ReturnType<typeof setTimeout> | null {
  const ms = checkTimeoutMs(name, timeoutMs);
  if (ms === Infinity) return null;
  return setTimeout(fn, Math.min(ms, MAX_TIMER_MS));
}

/** `clearTimeout` for what `startTimeout` returned (a no-op for null). */
export function clearTimer(timer: ReturnType<typeof setTimeout> | null): void {
  if (timer !== null) clearTimeout(timer);
}
