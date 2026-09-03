/**
 * `.depzrec` capture and replay transports (contracts/08_RECORDING_FORMAT.md).
 * Mirrors the Python reference `depz_sensor_sdk.transport.record_replay`.
 */

import { LinkClosedError } from "../errors.js";
import type { SerialTransport, SerialTransportInfo } from "./types.js";

export const RECORDING_SCHEMA = "depz.rec/1";

function bytesToHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
}

function hexToBytes(hex: string): Uint8Array {
  const out = new Uint8Array(hex.length >>> 1);
  for (let i = 0; i < out.length; i++) {
    out[i] = parseInt(hex.substring(i * 2, i * 2 + 2), 16);
  }
  return out;
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

  open(opts?: { baudRate?: number }): Promise<void> {
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

  close(): Promise<void> {
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
    const lines = content.split("\n").filter((l) => l.trim().length > 0);
    this.header =
      lines.length > 0 ? (JSON.parse(lines[0]!) as Record<string, unknown>) : {};
    const txChunks: Uint8Array[] = [];
    let txLen = 0;
    for (const line of lines.slice(1)) {
      const ev = JSON.parse(line) as { t: number; dir: string; data: string };
      const data = hexToBytes(ev.data);
      if (ev.dir === "rx") {
        this.rx.push({ tUs: ev.t, data, txPrefix: txLen });
      } else {
        txChunks.push(data);
        txLen += data.length;
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

  async open(): Promise<void> {}

  private notifyAll(): void {
    const waiters = this.waiters;
    this.waiters = [];
    for (const w of waiters) w();
  }

  private waitChange(): Promise<void> {
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
        throw new Error(
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
