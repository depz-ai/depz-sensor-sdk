/**
 * Stream<T>: one typed output of a device (contract 07 §3) — `DepzDevice` has
 * one for its events, every sensor class one for its measurements / frames /
 * samples / reports. `StreamQueue` is its pull side, behind the
 * `StreamIterator` the consumer sees.
 *
 * Mirrors the Python reference `depz_sensor_sdk.stream`.
 */

import { DepzError, DepzTimeoutError, DeviceLostError, InvalidStateError, LinkClosedError } from "./errors.js";
import { checkTimeoutMs, clearTimer, startTimeout } from "./timeout.js";

/**
 * A device stream as the pull consumer sees it — what `Sr04.measurements.subscribe()`,
 * `Vl53l8cx.frames.subscribe()`, `Bno086.reports.subscribe()` … return: a
 * bounded, drop-oldest async iterator (contract 07 §3). `for await` it, or
 * call `next()`; leaving the loop (`break`, `return()`) unsubscribes from the
 * device.
 */
export interface StreamIterator<T> extends AsyncIterableIterator<T> {
  /** Monotonic count of items evicted while the consumer lagged. */
  readonly droppedCount: number;
  /**
   * End the stream and unsubscribe from the device: the queue leaves
   * `droppedCounts`, nothing is queued any more; items already buffered are
   * still drained by the consumer. Calling it twice is a no-op.
   */
  close(): void;
  /** `close()`, as leaving a `for await` does (`break`, `return`). */
  return(): Promise<IteratorResult<T>>;
}

/**
 * @internal The SDK's stream plumbing behind `StreamIterator`: the producer
 * side (`push`) the stream feeds from the read pump.
 */
export class StreamQueue<T> implements StreamIterator<T> {
  droppedCount = 0;

  private readonly maxsize: number;
  private readonly onDone: (() => void) | undefined;
  private buf: T[] = [];
  private waiters: Array<(r: IteratorResult<T>) => void> = [];
  private closed = false;

  constructor(maxsize: number, onDone?: () => void) {
    this.maxsize = Math.max(1, maxsize);
    this.onDone = onDone;
  }

  /** Producer side: enqueue, evicting the oldest item when full. */
  push(item: T): void {
    if (this.closed) return;
    const waiter = this.waiters.shift();
    if (waiter) {
      waiter({ value: item, done: false });
      return;
    }
    if (this.buf.length >= this.maxsize) {
      this.buf.shift();
      this.droppedCount += 1;
    }
    this.buf.push(item);
  }

  /**
   * End the stream; buffered items are still drained by the consumer. Also
   * leaves the stream (`onDone`): a queue the consumer closed must not stay
   * subscribed — it would keep its place in `droppedCounts` and keep the
   * device from a hand-over for good (the Python iterator's `close()`
   * deregisters too).
   */
  close(): void {
    if (this.closed) return;
    this.closed = true;
    for (const waiter of this.waiters.splice(0)) {
      waiter({ value: undefined, done: true });
    }
    this.onDone?.();
  }

  async next(): Promise<IteratorResult<T>> {
    const item = this.buf.shift();
    if (item !== undefined) return Promise.resolve({ value: item, done: false });
    if (this.closed) return Promise.resolve({ value: undefined, done: true });
    return new Promise((resolve) => this.waiters.push(resolve));
  }

  async return(): Promise<IteratorResult<T>> {
    this.close();
    return Promise.resolve({ value: undefined, done: true });
  }

  [Symbol.asyncIterator](): AsyncIterableIterator<T> {
    return this;
  }
}

/** Options of `Stream.on` and `Stream.subscribe`. */
export interface StreamOptions<T> {
  /** Keep only the items the predicate accepts (a BNO086 SensorId filter, …). */
  where?: (item: T) => boolean;
}

/** The `where` of one subscription; undefined = every item. */
type Where<T> = ((item: T) => boolean) | undefined;

/**
 * The subscribers, the pull queues and the counters of one kind of item
 * (contract 07 §3): `dev.measurements`, `dev.frames`, `dev.samples`,
 * `dev.reports`, `dev.events`.
 *
 * Consumer side: `on(cb)` subscribes a callback (read-pump context — do not
 * block), `subscribe(maxsize)` makes an eager bounded drop-oldest iterator,
 * `next(timeoutMs)` waits for one item and is released by the device's
 * `close()`, `droppedCounts` and `parseErrors` are its counters. Owner side
 * (the sensor class, not public): the read pump emits one item per decoded
 * report and bumps `parseErrors` for a report it could not decode.
 * Subscribers are held as copy-on-write arrays: the pump iterates the array
 * it started with while callers replace the field — nothing is mutated under
 * the pump.
 */
export class Stream<T> {
  /**
   * Reports of this stream dropped because they did not decode (contract 07
   * §2) — a report shorter than its header, or a value the decoder refuses.
   * Monotonic over the device's life, except where a sensor contract resets
   * it (TMF8829 `start()`).
   */
  parseErrors = 0;

  private readonly stats: { callbackErrors: number };
  private readonly maxsize: number;
  // The report `next()` names in its DepzTimeoutError; null = no blocking
  // wait is offered (the events stream).
  private readonly what: number | null;
  private cbs: ReadonlyArray<{ cb: (item: T) => void; where: Where<T> }> = [];
  private queues: ReadonlyArray<{ q: StreamQueue<T>; where: Where<T> }> = [];
  private closed = false;
  // Why the stream ended (LinkClosedError / DeviceLostError); what next() rejects with.
  private end: DepzError | undefined;

  /**
   * @internal Made by the device (`DepzDevice.stream()`), which closes it at
   * teardown; `stats.callbackErrors` counts the callbacks that threw.
   */
  constructor(stats: { callbackErrors: number }, maxsize: number, what: number | null) {
    this.stats = stats;
    this.maxsize = maxsize;
    this.what = what;
  }

  // ── consumer side ──────────────────────────────────────────────────────────

  /**
   * Subscribe `cb` to every item (`where` keeps only the items it accepts).
   * Runs on the read-pump context — do not block. Returns the unsubscribe
   * function; calling it twice is a no-op.
   */
  on(cb: (item: T) => void, opts: StreamOptions<T> = {}): () => void {
    const entry = { cb, where: opts.where };
    this.cbs = [...this.cbs, entry];
    return () => {
      this.cbs = this.cbs.filter((e) => e !== entry);
    };
  }

  /**
   * Async iterator over the items — bounded, drop-oldest (`droppedCount` on
   * the iterator); `maxsize` defaults to the stream's. Subscribes at the
   * call — items emitted after it are never missed — and ends when the
   * device closes or the consumer breaks out of iteration. The end is quiet
   * (`done: true`) after the program's own `close()` and after a loss alike;
   * the device's `lost` tells them apart (contract 07 §3).
   */
  subscribe(maxsize: number = this.maxsize, opts: StreamOptions<T> = {}): StreamIterator<T> {
    const q: StreamQueue<T> = new StreamQueue(maxsize, () => this.detach(q));
    if (this.closed) {
      q.close();
      return q;
    }
    this.attach(q, opts.where);
    return q;
  }

  /**
   * Wait for the next item on a one-slot queue of its own, subscribed for
   * this call only.
   *
   * Rejects with `DepzTimeoutError(what, timeoutMs)` when nothing arrives in
   * time. As soon as the device ends — before the call, or while waiting (an
   * item already queued still wins) — rejects with `LinkClosedError` after the
   * program's own `close()` and `DeviceLostError` (carrying the cause) after a
   * loss (contract 07 §3).
   * `timeoutMs` is a number ≥ 0, `Infinity` for no timeout (the wait ends
   * with the item or the close); a negative or NaN one is a RangeError. The
   * `events` stream offers no `next` (contract 07 §3: no single report to
   * name): there the call is refused with `InvalidStateError`.
   */
  async next(timeoutMs = 2000): Promise<T> {
    if (this.what === null) {
      throw new InvalidStateError(
        "the events stream offers no next(): no single report to wait for " +
          "(contract 07 §3) — subscribe with events.on(cb) or iterate events.subscribe()",
      );
    }
    if (this.closed) throw this.endError();
    checkTimeoutMs("timeoutMs", timeoutMs); // refused before anything is subscribed
    const q = new StreamQueue<T>(1);
    let timer: ReturnType<typeof setTimeout> | null = null;
    const expired = new Promise<never>((_, reject) => {
      timer = startTimeout("timeoutMs", () => reject(new DepzTimeoutError(this.what!, timeoutMs)), timeoutMs);
    });
    this.attach(q, undefined);
    try {
      const result = await Promise.race([q.next(), expired]);
      if (result.done === true) throw this.endError();
      return result.value;
    } finally {
      clearTimer(timer);
      this.detach(q);
    }
  }

  /** Drop counters of all live pull queues (diagnostics). */
  get droppedCounts(): number[] {
    return this.queues.map((e) => e.q.droppedCount);
  }

  // ── owner side (read-pump context) ─────────────────────────────────────────

  /**
   * @internal Hand `item` to every callback, then to every queue (contract 07
   * §3). A callback that throws is counted in `stats.callbackErrors`, logged
   * and dropped — it never stops the pump, the other callbacks or the queues.
   */
  emit(item: T): void {
    for (const { cb, where } of this.cbs) {
      if (where === undefined || where(item)) {
        if (!callGuarded(cb, item)) this.stats.callbackErrors += 1;
      }
    }
    for (const { q, where } of this.queues) {
      if (where === undefined || where(item)) q.push(item);
    }
  }

  /**
   * @internal The device ended, for the reason `end`: every pull queue ends
   * (a buffered item is still drained), every pending `next()` fails with
   * that reason, and a wait started later ends at once.
   */
  close(end?: DepzError): void {
    this.closed = true;
    this.end = end;
    const queues = this.queues;
    this.queues = [];
    for (const { q } of queues) q.close();
  }

  /** @internal True while a pull queue is live (hand-over refuses then). */
  get hasQueues(): boolean {
    return this.queues.length > 0;
  }

  /** A fresh error of the kind the stream ended with (the stack belongs to the caller's wait). */
  private endError(): DepzError {
    if (this.end instanceof DeviceLostError) {
      const lost = new DeviceLostError(this.end.message);
      lost.cause = this.end.cause;
      return lost;
    }
    return new LinkClosedError(this.end?.message ?? "device closed");
  }

  private attach(q: StreamQueue<T>, where: ((item: T) => boolean) | undefined): void {
    this.queues = [...this.queues, { q, where }];
  }

  private detach(q: StreamQueue<T>): void {
    this.queues = this.queues.filter((e) => e.q !== q);
  }
}

/**
 * Run a user callback where its error must not stop the caller (a device's
 * read pump, a dataset player — contract 07 §3, 09 §Player): one that throws
 * is logged with `console.error` and dropped. False if it threw, so the
 * caller counts it.
 * @internal
 */
export function callGuarded<T>(cb: (item: T) => void, item: T): boolean {
  try {
    cb(item);
    return true;
  } catch (err) {
    console.error("depz-sensor-sdk: a callback threw; dropped", err);
    return false;
  }
}
