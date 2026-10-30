/**
 * DepzDevice: request correlation, events, common commands
 * (contracts/02_COMMON_COMMANDS.md, 07_SDK_FACADE.md).
 *
 * Behavioral rules per contract 07 §3: correlation by echoed request opcode
 * (never seq), one in-flight request per opcode, 200 ms default timeout,
 * bounded drop-oldest stream queues, callbacks run on the read-pump context.
 *
 * Mirrors the Python reference `depz_sensor_sdk.device.DeviceBase`. The read
 * pump only starts in `open()`, after the (sub)class constructor finished —
 * subclass stream state always exists before the first dispatch.
 */

import {
  BusyError,
  DepzError,
  DepzTimeoutError,
  DeviceLostError,
  InvalidReplyError,
  InvalidStateError,
  LinkClosedError,
  StatusError,
} from "../errors.js";
import {
  Cmd,
  Rpt,
  Status,
  UNSOLICITED,
  checkSyncPin,
  packSyncPinConfig,
  packSetPayloadCrcType,
  packSyncTime,
  syncTimeOffsetRtt,
  unpackPayloadCrcType,
  unpackSequenceError,
  unpackStatus,
  unpackSyncPinConfig,
  unpackSyncTime,
  unpackTemperature,
  unpackText,
  type SyncPinConfig,
} from "../protocol/common.js";
import { parseSoftwareName, type Identity } from "../protocol/identity.js";
import { CrcType, type PacketEvent, type ParserEvent } from "../protocol/framing.js";
import type { SerialTransport } from "../transport/types.js";
import { DepzLink, type LinkStats } from "./link.js";

export const DEFAULT_TIMEOUT_MS = 200;

// ── events (contract 07 §2) ──────────────────────────────────────────────────

/** Unsolicited/diagnostic events, mirrored from the Python event classes. */
export type DeviceEvent =
  | {
      type: "sequenceError";
      expectedSeq: number;
      receivedSeq: number;
      /** true: device saw a gap in host TX (RPT 0x84). */
      reportedByDevice: boolean;
    }
  | {
      /** A gap the host saw in the device→host seq (contract 01 §5 step 5);
       *  counted in `stats.seqGaps`. Not the device's RPT 0x84. */
      type: "seqGap";
      expectedSeq: number;
      receivedSeq: number;
      /** (received − expected) mod 256 packets that never arrived. */
      missed: number;
    }
  | { type: "linkCrcError"; cmd: number; seq: number }
  | { type: "trash"; data: Uint8Array }
  | { type: "unsolicitedStatus"; status: number }
  | { type: "text"; cmd: number; text: string }
  | { type: "temperature"; timestampUs: bigint; celsius: number }
  | { type: "disconnected"; reason: string };

/** Result of `syncTime()` (contract 02 §5). offset = device − host clock. */
export interface TimeSync {
  offsetUs: bigint;
  rttUs: bigint;
  syncedAtHostUs: bigint;
}

/** Monotonic host clock in µs — the host side of all time-sync math. */
export function hostNowUs(): bigint {
  return BigInt(Math.round(performance.now() * 1000));
}

// ── request correlation ──────────────────────────────────────────────────────

/** Returned by a matcher to decline a packet. */
export const NO_MATCH: unique symbol = Symbol("depz.NO_MATCH");

/**
 * Claims a packet for an in-flight request: return the parsed result, or
 * `NO_MATCH` to let other consumers see the packet.
 */
export type Matcher<T> = (pkt: PacketEvent) => T | typeof NO_MATCH;

export interface RequestOptions<T> {
  /** First shot at every non-status packet while the request is pending. */
  matcher?: Matcher<T>;
  /** Resolve on RPT_STATUS(cmd, OK) — for commands whose success reply is the ack. */
  okCompletes?: boolean;
  timeoutMs?: number;
}

interface PendingRequest {
  cmd: number;
  matcher: Matcher<unknown> | null;
  okCompletes: boolean;
  resolve: (value: unknown) => void;
  reject: (error: Error) => void;
  timer: ReturnType<typeof setTimeout>;
}

// ── stream queues ────────────────────────────────────────────────────────────

/**
 * Bounded drop-oldest async stream (contract 07 §3). Doubles as the async
 * iterator returned by e.g. `Sr04.measurements()`; `droppedCount` is the
 * monotonic count of items evicted while the consumer lagged.
 */
export class StreamQueue<T> implements AsyncIterableIterator<T> {
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

  /** End the stream; buffered items are still drained by the consumer. */
  close(): void {
    if (this.closed) return;
    this.closed = true;
    for (const waiter of this.waiters.splice(0)) {
      waiter({ value: undefined, done: true });
    }
  }

  async next(): Promise<IteratorResult<T>> {
    const item = this.buf.shift();
    if (item !== undefined) return Promise.resolve({ value: item, done: false });
    if (this.closed) return Promise.resolve({ value: undefined, done: true });
    return new Promise((resolve) => this.waiters.push(resolve));
  }

  async return(): Promise<IteratorResult<T>> {
    this.close();
    this.onDone?.();
    return Promise.resolve({ value: undefined, done: true });
  }

  [Symbol.asyncIterator](): AsyncIterableIterator<T> {
    return this;
  }
}

// ── device ───────────────────────────────────────────────────────────────────

export interface DeviceOptions {
  timeoutMs?: number;
  /**
   * CRC type of the frames the device sends (default `CrcType.None`); on an
   * adopted connection (`attachSensorClass`, `openOnTransport`) it replaces
   * the link's, left as it was when not given.
   */
  txCrcType?: CrcType;
  /**
   * @internal Continue on this already-started link instead of creating one
   * (the hand-over of `attachSensorClass`, contract 02 §4.4). Not for apps.
   */
  [ADOPT_LINK]?: DepzLink;
}

/**
 * @internal Key of the hand-over option: a constructor given
 * `{ [ADOPT_LINK]: link }` continues on that (started) link — same transport,
 * same read pump, host sequence numbers running on — and its `open()` is a
 * no-op. Produced only by `DepzDevice[HAND_OVER]()`.
 */
export const ADOPT_LINK: unique symbol = Symbol("depz.adoptLink");

/** @internal Key of the method that detaches a device's link for hand-over. */
export const HAND_OVER: unique symbol = Symbol("depz.handOver");

/** What a device hands over: its started link and the transport under it. */
export interface HandedOverConnection {
  link: DepzLink;
  transport: SerialTransport;
}

/**
 * Connection to one DEPZ device in application mode. Construct over any
 * `SerialTransport`, then `await open()` before use.
 */
export class DepzDevice {
  timeoutMs: number;

  protected readonly link: DepzLink;
  private readonly transport: SerialTransport;
  private readonly pending = new Map<number, PendingRequest>();
  private eventCbs: Array<(ev: DeviceEvent) => void> = [];
  private streams: Array<{ close(): void }> = [];
  private timeSyncState: TimeSync | null = null;
  private closing = false;
  private torndown = false;
  private handedOver = false;
  private readonly unsubLink: Array<() => void>;

  constructor(transport: SerialTransport, opts?: DeviceOptions) {
    this.transport = transport;
    this.timeoutMs = opts?.timeoutMs ?? DEFAULT_TIMEOUT_MS;
    const adopted = opts?.[ADOPT_LINK];
    this.link = adopted ?? new DepzLink(transport, { txCrcType: opts?.txCrcType ?? CrcType.None });
    if (adopted !== undefined && opts?.txCrcType !== undefined) adopted.txCrcType = opts.txCrcType;
    this.unsubLink = [
      this.link.onParserEvent((ev) => this.handleParserEvent(ev)),
      this.link.onSeqGap((gap) => this.emitEvent({ type: "seqGap", ...gap })),
      this.link.onClose((cause) => this.handleLinkClosed(cause)),
    ];
  }

  // ── lifecycle ──────────────────────────────────────────────────────────────

  /**
   * Open the transport and start the read pump. A no-op on a device that
   * continues an adopted connection (`attachSensorClass`) — already open.
   */
  async open(): Promise<void> {
    if (this.link.isStarted) return;
    await this.transport.open();
    this.link.start();
  }

  /** Close the connection. A device that handed its connection over owns none. */
  async close(): Promise<void> {
    if (this.handedOver) return;
    this.closing = true;
    await this.link.close();
  }

  /**
   * @internal Detach this device from its open connection so another device
   * object can continue on it (`attachSensorClass`, contract 02 §4.4). Must
   * be called with nothing in flight and no stream open; afterwards this
   * object sends nothing and its `close()` is a no-op. The caller constructs
   * the successor with `{ [ADOPT_LINK]: link }` in the same synchronous turn,
   * so no received byte goes to nobody.
   */
  [HAND_OVER](): HandedOverConnection {
    if (this.handedOver) throw new InvalidStateError("connection already handed over");
    if (!this.link.isStarted || this.torndown || this.closing) {
      throw new InvalidStateError("hand-over needs an open connection");
    }
    if (this.pending.size > 0 || this.streams.length > 0) {
      throw new InvalidStateError("hand-over with a request in flight or a stream open");
    }
    for (const unsub of this.unsubLink) unsub();
    this.handedOver = true;
    return { link: this.link, transport: this.transport };
  }

  get stats(): LinkStats {
    return this.link.stats;
  }

  private handleLinkClosed(cause?: unknown): void {
    if (this.torndown) return;
    this.torndown = true;
    let error: DepzError;
    if (this.closing) {
      error = new LinkClosedError("device closed");
    } else if (cause === undefined) {
      error = new DeviceLostError("transport closed");
    } else {
      // The read pump failed: the device is gone as surely as on an unplug.
      console.error("depz-sensor-sdk: the read pump ended", cause);
      error = new DeviceLostError(`read pump ended: ${cause instanceof Error ? cause.message : String(cause)}`);
      error.cause = cause;
    }
    this.failAllPending(error);
    if (!this.closing) this.emitEvent({ type: "disconnected", reason: error.message });
    // Let subclasses reject their own in-flight waiters (e.g. BNO086's SH-2
    // control layer) so a disconnect doesn't leave them hanging until their own
    // timeout, and clear any per-connection resolver state.
    this.onTeardown(error);
    for (const s of [...this.streams]) s.close();
  }

  /**
   * Hook for sensor subclasses: reject any subclass-managed in-flight requests
   * and clear per-connection state when the link closes. Runs once, after the
   * base `pending` map is failed. Default: no-op.
   */
  protected onTeardown(error: Error): void {
    void error;
  }

  private failAllPending(error: Error): void {
    const all = [...this.pending.values()];
    this.pending.clear();
    for (const p of all) {
      clearTimeout(p.timer);
      p.reject(error);
    }
  }

  // ── dispatch (read-pump context) ───────────────────────────────────────────

  private handleParserEvent(ev: ParserEvent): void {
    if (ev.type === "packet") {
      this.dispatch(ev);
    } else if (ev.type === "crcError") {
      this.emitEvent({ type: "linkCrcError", cmd: ev.cmd, seq: ev.seq });
    } else {
      this.emitEvent({ type: "trash", data: ev.data });
    }
  }

  private dispatch(pkt: PacketEvent): void {
    if (pkt.cmd === Rpt.Status && pkt.payload.length >= 2) {
      const rep = unpackStatus(pkt.payload);
      if (rep.cmd !== UNSOLICITED) {
        const pending = this.pending.get(rep.cmd);
        if (pending !== undefined) {
          if (rep.status === Status.ErrBusy) {
            this.settle(pending, { error: new BusyError(rep.cmd) });
          } else if (rep.status !== Status.Ok) {
            this.settle(pending, { error: new StatusError(rep.cmd, rep.status) });
          } else if (pending.okCompletes) {
            this.settle(pending, { value: undefined });
          }
          // else: OK ack while waiting for the data report — leave the
          // pending request in flight untouched.
          return;
        }
      }
      this.emitEvent({ type: "unsolicitedStatus", status: rep.status });
      return;
    }

    // Give pending matchers first shot at any non-status packet.
    //
    // Correlation is by opcode/report-id only: the device echoes the request
    // opcode in its reply but NOT the host's TX sequence number (see link.ts
    // txSeq — nothing carries it back), so a sequence-based matcher is
    // impossible without firmware support. One consequence: a reply that arrives
    // *after* its request timed out is indistinguishable from the genuine reply
    // to a later same-opcode request and could settle it with stale data. The
    // only retry-in-place path (the viewer's whole-init retry) mitigates this by
    // pausing before re-issuing — during that quiet gap there is no pending
    // entry, so a late reply falls through to handleReport/`text` below and is
    // discarded harmlessly. A timeout should therefore be treated as "the link
    // may be desynced"; prefer reopening over transparent same-opcode retry.
    //
    // A matcher that throws has claimed the packet as its reply and failed to
    // decode it (contract 07 §3: a reply shorter than its fields is rejected):
    // that one request fails with InvalidReplyError and the pump goes on — an
    // exception escaping here would end the read pump and fail every later
    // request on this device.
    for (const p of [...this.pending.values()]) {
      if (p.matcher === null) continue;
      let result: unknown;
      try {
        result = p.matcher(pkt);
      } catch (err) {
        this.settle(p, { error: new InvalidReplyError(p.cmd, err) });
        return;
      }
      if (result !== NO_MATCH) {
        this.settle(p, { value: result });
        return;
      }
    }

    // Contract 07 §3: a report longer than its fields is decoded and the tail
    // ignored (a newer firmware may append fields); a shorter one is not a
    // report of that kind and stays visible as hex text below.
    if (pkt.cmd === Rpt.SequenceError && pkt.payload.length >= 2) {
      const rep = unpackSequenceError(pkt.payload);
      this.emitEvent({
        type: "sequenceError",
        expectedSeq: rep.expectedSeq,
        receivedSeq: rep.receivedSeq,
        reportedByDevice: true,
      });
      return;
    }
    if (pkt.cmd === Rpt.Text && pkt.payload.length > 0) {
      const rep = unpackText(pkt.payload);
      this.emitEvent({ type: "text", cmd: rep.cmd, text: rep.text });
      return;
    }
    if (pkt.cmd === Rpt.Temperature && pkt.payload.length >= 10) {
      const rep = unpackTemperature(pkt.payload);
      this.emitEvent({
        type: "temperature",
        timestampUs: rep.timestampUs,
        celsius: rep.rawDecidegrees / 10,
      });
      return;
    }

    // The report path runs guarded like the matchers above: a stream handler
    // that throws drops this one packet, counted and logged — it never ends
    // the read pump (contract 07 §3).
    let consumed: boolean;
    try {
      consumed = this.handleReport(pkt);
    } catch (err) {
      this.link.stats.reportErrors += 1;
      const id = pkt.cmd.toString(16).padStart(2, "0");
      console.error(`depz-sensor-sdk: the handler of report 0x${id} threw; packet dropped`, err);
      return;
    }
    if (consumed) return;
    // Unknown/unrouted report: keep it visible so new firmware is debuggable.
    this.emitEvent({ type: "text", cmd: pkt.cmd, text: toHex(pkt.payload) });
  }

  private settle(
    p: PendingRequest,
    outcome: { value: unknown; error?: undefined } | { error: Error },
  ): void {
    clearTimeout(p.timer);
    this.pending.delete(p.cmd);
    if (outcome.error !== undefined) p.reject(outcome.error);
    else p.resolve(outcome.value);
  }

  /**
   * Hook for sensor subclasses: route streaming reports here. Return true
   * when the packet was consumed. Runs after status/matcher/common-report
   * routing, on the read-pump context.
   */
  protected handleReport(pkt: PacketEvent): boolean {
    void pkt;
    return false;
  }

  /** Register a stream to be closed on device teardown. */
  protected registerStream(stream: { close(): void }): () => void {
    this.streams.push(stream);
    return () => {
      this.streams = this.streams.filter((s) => s !== stream);
    };
  }

  // ── events ─────────────────────────────────────────────────────────────────

  /**
   * Subscribe to unsolicited/diagnostic events (read-pump context; do not
   * block). Returns an unsubscribe function.
   */
  onEvent(cb: (ev: DeviceEvent) => void): () => void {
    this.eventCbs.push(cb);
    return () => {
      this.eventCbs = this.eventCbs.filter((c) => c !== cb);
    };
  }

  protected emitEvent(event: DeviceEvent): void {
    for (const cb of [...this.eventCbs]) this.callGuarded(cb, event);
  }

  /**
   * Run a user callback on the read pump: one that throws is counted in
   * `stats.callbackErrors`, logged and dropped — it never stops the pump, the
   * other callbacks or the queues (contract 07 §3).
   */
  protected callGuarded<T>(cb: (item: T) => void, item: T): void {
    if (!callGuarded(cb, item)) this.link.stats.callbackErrors += 1;
  }

  // ── TX / requests ──────────────────────────────────────────────────────────

  /**
   * Send `cmd` and wait for its correlated completion (contract 02 §1).
   *
   * Exactly one of the completion paths must be configured:
   * - `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
   * - `matcher` — first packet for which the matcher returns non-`NO_MATCH`
   *   resolves with the matcher's return value.
   * Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
   * StatusError otherwise). One in-flight request per opcode.
   */
  async request<T = void>(
    cmd: number,
    payload: Uint8Array = new Uint8Array(0),
    opts: RequestOptions<T> = {},
  ): Promise<T> {
    const { matcher, okCompletes = false } = opts;
    if (matcher === undefined && !okCompletes) {
      throw new DepzError("configure matcher or okCompletes");
    }
    if (this.handedOver) throw new LinkClosedError("connection handed over to another device object");
    // Contract 07 §3: closed by this side, or lost — said before sending.
    if (this.closing) throw new LinkClosedError("device closed");
    if (this.torndown) throw new DeviceLostError("transport closed");
    if (this.pending.has(cmd)) throw new BusyError(cmd);
    const timeoutMs = opts.timeoutMs ?? this.timeoutMs;
    const result = new Promise<T>((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(cmd);
        reject(new DepzTimeoutError(cmd, timeoutMs));
      }, timeoutMs);
      this.pending.set(cmd, {
        cmd,
        matcher: (matcher as Matcher<unknown> | undefined) ?? null,
        okCompletes,
        resolve: resolve as (value: unknown) => void,
        reject,
        timer,
      });
    });
    // Mark handled so an early rejection (timeout/disconnect racing the
    // write) never surfaces as an unhandled rejection; the caller still
    // receives it through the returned promise.
    void result.catch(() => undefined);
    try {
      await this.link.send(cmd, payload);
    } catch (err) {
      const p = this.pending.get(cmd);
      if (p !== undefined) {
        clearTimeout(p.timer);
        this.pending.delete(cmd);
      }
      if (err instanceof DepzError) throw err;
      // A transport's own Error (port gone, write failed) is the SDK's lost
      // error, or closed when this side is closing; the cause stays attached.
      const mapped = this.closing
        ? new LinkClosedError("device closed")
        : new DeviceLostError(`write failed: ${err instanceof Error ? err.message : String(err)}`);
      mapped.cause = err;
      throw mapped;
    }
    return result;
  }

  /** Matcher for a typed report identified by its report ID alone. */
  static expectReport<T>(reportId: number, unpack: (payload: Uint8Array) => T): Matcher<T> {
    return (pkt) => (pkt.cmd === reportId ? unpack(pkt.payload) : NO_MATCH);
  }

  /** Matcher for RPT_TEXT echoing `requestCmd`. */
  static expectText(requestCmd: number): Matcher<string> {
    return (pkt) => {
      if (pkt.cmd !== Rpt.Text || pkt.payload.length === 0 || pkt.payload[0] !== requestCmd) {
        return NO_MATCH;
      }
      return unpackText(pkt.payload).text;
    };
  }

  // ── common commands (contract 02) ──────────────────────────────────────────

  async getDeviceName(): Promise<string> {
    return this.request(Cmd.GetDeviceName, undefined, {
      matcher: DepzDevice.expectText(Cmd.GetDeviceName),
    });
  }

  async getSoftwareName(): Promise<string> {
    return this.request(Cmd.GetNameActiveSoftware, undefined, {
      matcher: DepzDevice.expectText(Cmd.GetNameActiveSoftware),
    });
  }

  async getSerialNumber(): Promise<string> {
    return this.request(Cmd.GetSerial, undefined, {
      matcher: DepzDevice.expectText(Cmd.GetSerial),
    });
  }

  /** Classify the running firmware (contract 02 §4). */
  async identify(): Promise<Identity> {
    return parseSoftwareName(await this.getSoftwareName());
  }

  /** Last cached MCU temperature in °C (device refreshes ~2 Hz). */
  async readMcuTemperature(): Promise<number> {
    const rep = await this.request(Cmd.GetMcuTemperature, undefined, {
      matcher: DepzDevice.expectReport(Rpt.Temperature, unpackTemperature),
    });
    return rep.rawDecidegrees / 10;
  }

  /** NTP-style sync; keeps the lowest-RTT sample (contract 02 §5). */
  async syncTime(samples = 5): Promise<TimeSync> {
    let best: TimeSync | null = null;
    for (let i = 0; i < Math.max(1, samples); i++) {
      const t1 = hostNowUs();
      const rep = await this.request(Cmd.SyncTime, packSyncTime(t1), {
        matcher: DepzDevice.expectReport(Rpt.SyncTime, unpackSyncTime),
      });
      const t4 = hostNowUs();
      const { offsetUs, rttUs } = syncTimeOffsetRtt(t1, rep.mcuRxUs, rep.mcuTxUs, t4);
      if (best === null || rttUs < best.rttUs) {
        best = { offsetUs, rttUs, syncedAtHostUs: t4 };
      }
    }
    this.timeSyncState = best;
    return best!;
  }

  get timeSync(): TimeSync | null {
    return this.timeSyncState;
  }

  /** Device µs → host monotonic µs (requires a prior `syncTime`). */
  toHostTimeUs(deviceTsUs: bigint): bigint {
    if (this.timeSyncState === null) throw new InvalidStateError("call syncTime() first");
    return deviceTsUs - this.timeSyncState.offsetUs;
  }

  async getReportPayloadCrc(): Promise<CrcType> {
    return this.request(Cmd.GetPayloadCrcType, undefined, {
      matcher: DepzDevice.expectReport(Rpt.PayloadCrcType, (p) => unpackPayloadCrcType(p) as CrcType),
    });
  }

  /** Set the device→host payload CRC mode (host→device is per-packet). */
  async setReportPayloadCrc(crcType: CrcType): Promise<void> {
    await this.request(Cmd.SetPayloadCrcType, packSetPayloadCrcType(crcType), { okCompletes: true });
  }

  async getSyncPin(pin: number): Promise<SyncPinConfig> {
    return this.request(Cmd.GetSyncPinConfig, Uint8Array.of(checkSyncPin(pin)), {
      matcher: DepzDevice.expectReport(Rpt.SyncPinConfig, unpackSyncPinConfig),
    });
  }

  async setSyncPin(config: SyncPinConfig): Promise<void> {
    await this.request(Cmd.SetSyncPinConfig, packSyncPinConfig(config), { okCompletes: true });
  }

  /** DEVICE_RESET: device ACKs then reboots; the link will drop. */
  async reset(): Promise<void> {
    await this.request(Cmd.DeviceReset, undefined, { okCompletes: true });
  }

  /**
   * Ask the device to reboot into the resident bootloader and close this
   * connection. Re-discovery/flash flow lives in the bootloader module
   * (contract 06).
   */
  async enterBootloaderMode(): Promise<void> {
    await this.request(Cmd.Bootloader, undefined, { okCompletes: true });
    await this.close();
  }
}

/**
 * Software-sync several devices to the common host monotonic clock.
 *
 * Runs `syncTime()` on each device so that every device's `toHostTimeUs()`
 * maps its own timestamps onto ONE shared host timeline — the basis for
 * correlating reports from multiple sensors live. Returns a Map
 * {device → TimeSync}. Mirrors the Python reference `sync_time_all`.
 */
export async function syncTimeAll(
  devices: Iterable<DepzDevice>,
  samples = 5,
): Promise<Map<DepzDevice, TimeSync>> {
  const out = new Map<DepzDevice, TimeSync>();
  for (const dev of devices) {
    out.set(dev, await dev.syncTime(samples));
  }
  return out;
}

function toHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
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
