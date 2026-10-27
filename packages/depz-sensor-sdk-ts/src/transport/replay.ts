/**
 * `.depzrec` capture and replay transports (contracts/08_RECORDING_FORMAT.md).
 * Mirrors the Python reference `depz_sensor_sdk.transport.record_replay`.
 */

import { LinkClosedError, ReplayMismatchError } from "../errors.js";
import type { SerialTransport, SerialTransportInfo } from "./types.js";

export const RECORDING_SCHEMA = "depz.rec/1";

function bytesToHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
}

const HEX_BYTES = /^(?:[0-9a-fA-F]{2})*$/;

/** `hex` as bytes; refused unless whole bytes of hex digits (even length). */
function hexToBytes(hex: string): Uint8Array | null {
  if (!HEX_BYTES.test(hex)) return null;
  const out = new Uint8Array(hex.length >>> 1);
  for (let i = 0; i < out.length; i++) {
    out[i] = parseInt(hex.substring(i * 2, i * 2 + 2), 16);
  }
  return out;
}

/** Contract 08 «Line rules»: lines end at LF only; a blank line is JSON whitespace only (not NBSP, NUL …). */
const BLANK_LINE = /^[ \t\r]*$/;

/**
 * `line` as the JSON object a capture line must be, else a TypeError naming
 * the line (a damaged capture is the caller's bad argument, contract 07 §3 —
 * never a crash in the reader).
 */
function jsonObjectLine(lineno: number, line: string): Record<string, unknown> {
  let obj: unknown;
  try {
    obj = JSON.parse(line);
  } catch (bad) {
    throw new TypeError(`.depzrec line ${lineno}: not JSON: ${(bad as Error).message}`);
  }
  if (typeof obj !== "object" || obj === null || Array.isArray(obj)) {
    throw new TypeError(`.depzrec line ${lineno}: not a JSON object`);
  }
  return obj as Record<string, unknown>;
}

interface RecordedEvent {
  tUs: number;
  dir: string;
  data: Uint8Array;
}

/**
 * One capture event line (contract 08): `t` a non-negative integer (µs),
 * `dir` a string, `data` a hex string of whole bytes — anything else is
 * refused with the line number, never coerced.
 */
function eventLine(lineno: number, line: string): RecordedEvent {
  const ev = jsonObjectLine(lineno, line);
  const { t, dir, data } = ev;
  if (typeof t !== "number" || !Number.isInteger(t) || t < 0) {
    throw new RangeError(`.depzrec line ${lineno}: 't' must be a non-negative integer, got ${JSON.stringify(t)}`);
  }
  if (typeof dir !== "string") {
    throw new TypeError(`.depzrec line ${lineno}: 'dir' must be a string, got ${JSON.stringify(dir)}`);
  }
  if (typeof data !== "string") {
    throw new TypeError(`.depzrec line ${lineno}: 'data' must be a hex string, got ${JSON.stringify(data)}`);
  }
  const bytes = hexToBytes(data);
  if (bytes === null) {
    throw new RangeError(`.depzrec line ${lineno}: 'data' is not hex: ${JSON.stringify(data)}`);
  }
  return { tUs: t, dir, data: bytes };
}

/**
 * Wraps any transport and tees both directions into `.depzrec` JSONL text.
 * Get the capture with `dump()` (line 1 is the header; browser-friendly —
 * no filesystem involved).
 */
export class RecordingTransport implements SerialTransport {
  private readonly inner: SerialTransport;
  private readonly lines: string[] = [];
  private readonly t0 = performance.now();

  constructor(inner: SerialTransport, opts?: { headerExtra?: Record<string, unknown> }) {
    this.inner = inner;
    const header: Record<string, unknown> = {
      schema: RECORDING_SCHEMA,
      created_utc: new Date().toISOString().replace(/\.\d{3}Z$/, "Z"),
      ...opts?.headerExtra,
    };
    this.lines.push(JSON.stringify(header));
  }

  get info(): SerialTransportInfo {
    return this.inner.info;
  }

  /** The capture so far as `.depzrec` file content. */
  dump(): string {
    return this.lines.join("\n") + "\n";
  }

  private emit(dir: "rx" | "tx", data: Uint8Array): void {
    if (data.length === 0) return;
    const t = Math.max(0, Math.round((performance.now() - this.t0) * 1000));
    this.lines.push(JSON.stringify({ t, dir, data: bytesToHex(data) }));
  }

  async open(opts?: { baudRate?: number }): Promise<void> {
    return this.inner.open(opts);
  }

  async write(data: Uint8Array): Promise<void> {
    // Record before the (async) write completes so an rx caused by this tx
    // can never be captured ahead of it — replay causality depends on it.
    this.emit("tx", data);
    await this.inner.write(data);
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    for await (const chunk of this.inner.readable()) {
      this.emit("rx", chunk);
      yield chunk;
    }
  }

  async close(): Promise<void> {
    return this.inner.close();
  }

  onDisconnect(cb: () => void): () => void {
    return this.inner.onDisconnect(cb);
  }
}

interface RecordedRx {
  tUs: number;
  data: Uint8Array;
  /** Total tx bytes recorded before this event (causal gate). */
  txPrefix: number;
}

/**
 * Replays the `rx` side of a `.depzrec` capture **causally**: an rx event
 * is released only after the host has written at least as many tx bytes as
 * preceded that event in the recording. Without this gating a replay would
 * deliver responses before the SDK even sends the requests.
 *
 * `strictTx` additionally asserts the written bytes match the recorded tx
 * stream byte-for-byte (protocol regression mode). `realtime` paces rx
 * events by their recorded timestamps.
 */
export class ReplayTransport implements SerialTransport {
  readonly info: SerialTransportInfo = { path: "replay" };
  /** Parsed header line (unknown fields preserved, contract 08). */
  readonly header: Record<string, unknown>;

  private readonly rx: RecordedRx[] = [];
  private readonly txStream: Uint8Array;
  private readonly realtime: boolean;
  private readonly strictTx: boolean;
  private txWritten = 0;
  private rxIdx = 0;
  private closedFlag = false;
  private waiters: Array<() => void> = [];
  private disconnectCbs: Array<() => void> = [];

  constructor(content: string, opts?: { realtime?: boolean; strictTx?: boolean }) {
    this.realtime = opts?.realtime ?? false;
    this.strictTx = opts?.strictTx ?? false;
    // Line 1 is the header (a JSON object; unknown fields are kept), every
    // later non-blank line an event; a damaged line is refused by number
    // (contract 08) rather than skipped or coerced.
    if (content.length === 0) {
      throw new TypeError(".depzrec: empty, not a capture (no header line)");
    }
    const lines = content.split("\n");
    this.header = jsonObjectLine(1, lines[0]!);
    const txChunks: Uint8Array[] = [];
    let txLen = 0;
    for (let i = 1; i < lines.length; i++) {
      const line = lines[i]!;
      if (BLANK_LINE.test(line)) continue;
      const ev = eventLine(i + 1, line);
      if (ev.dir === "rx") {
        this.rx.push({ tUs: ev.tUs, data: ev.data, txPrefix: txLen });
      } else {
        txChunks.push(ev.data);
        txLen += ev.data.length;
      }
    }
    this.txStream = new Uint8Array(txLen);
    let off = 0;
    for (const chunk of txChunks) {
      this.txStream.set(chunk, off);
      off += chunk.length;
    }
  }

  /** True once every recorded rx event has been delivered. */
  get exhausted(): boolean {
    return this.rxIdx >= this.rx.length;
  }

  /** Recorded tx bytes the host has not written yet (0 = whole tx stream re-issued). */
  get txRemaining(): number {
    return this.txStream.length - this.txWritten;
  }

  async open(): Promise<void> {}

  private notifyAll(): void {
    const waiters = this.waiters;
    this.waiters = [];
    for (const w of waiters) w();
  }

  private async waitChange(): Promise<void> {
    return new Promise((resolve) => this.waiters.push(resolve));
  }

  async write(data: Uint8Array): Promise<void> {
    if (this.closedFlag) throw new LinkClosedError("replay transport closed");
    if (this.strictTx) {
      const expected = this.txStream.subarray(this.txWritten, this.txWritten + data.length);
      let mismatch = expected.length !== data.length;
      for (let i = 0; !mismatch && i < data.length; i++) {
        if (expected[i] !== data[i]) mismatch = true;
      }
      if (mismatch) {
        throw new ReplayMismatchError(
          `replay strictTx mismatch at offset ${this.txWritten}: ` +
            `expected ${bytesToHex(expected)}, got ${bytesToHex(data)}`,
        );
      }
    }
    this.txWritten += data.length;
    this.notifyAll();
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    const t0 = performance.now();
    while (!this.closedFlag) {
      if (this.exhausted) {
        // Recording drained: behave like a silent port until close().
        await this.waitChange();
        continue;
      }
      const ev = this.rx[this.rxIdx]!;
      if (this.txWritten < ev.txPrefix) {
        await this.waitChange();
        continue;
      }
      if (this.realtime) {
        const dueMs = t0 + ev.tUs / 1000;
        const waitMs = dueMs - performance.now();
        if (waitMs > 0) await new Promise((r) => setTimeout(r, waitMs));
        if (this.closedFlag) return;
      }
      this.rxIdx += 1;
      yield ev.data;
    }
  }

  async close(): Promise<void> {
    if (this.closedFlag) return;
    this.closedFlag = true;
    this.notifyAll();
    const cbs = this.disconnectCbs;
    this.disconnectCbs = [];
    for (const cb of cbs) cb();
  }

  onDisconnect(cb: () => void): () => void {
    this.disconnectCbs.push(cb);
    return () => {
      this.disconnectCbs = this.disconnectCbs.filter((c) => c !== cb);
    };
  }
}
