/**
 * DepzDevice: request correlation, events, common commands
 * (contracts/02_COMMON_COMMANDS.md, 07_SDK_FACADE.md).
 *
 * Behavioral rules per contract 07 §3: correlation by echoed request opcode
 * (never seq), one in-flight request per opcode, 200 ms default timeout,
 * one `Stream` per kind of item (`events` here, the sensor classes'
 * measurements / frames / samples / reports), callbacks run on the read-pump
 * context.
 *
 * Mirrors the Python reference `depz_sensor_sdk.device.DepzDevice`. The read
 * pump only starts in `open()`, after the (sub)class constructor finished —
 * every stream exists before the first dispatch.
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
import type { Tmf8829Event } from "../protocol/tmf8829.js";
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
import { Stream } from "../stream.js";
import { checkTimeoutMs, clearTimer, startTimeout } from "../timeout.js";
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
  | { type: "disconnected"; reason: string }
  | {
      /** `Tmf8829` only: an RPT_TMF_EVENT that is not an expected stop
       *  artefact — a MOTION / PROXIMITY interrupt (`event.code` 0) or a
       *  refused frame header (code 1: the sensor is lost, run `init()`
       *  again). The Python reference's `Tmf8829InterruptEvent`. */
      type: "tmf8829Interrupt";
      event: Tmf8829Event;
    };

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

/**
 * Claims a packet for an in-flight request: return the parsed result, or
 * `DepzDevice.NO_MATCH` to let other consumers see the packet.
 */
export type Matcher<T> = (pkt: PacketEvent) => T | typeof DepzDevice.NO_MATCH;

export interface RequestOptions<T> {
  /** First shot at every non-status packet while the request is pending. */
  matcher?: Matcher<T>;
  /** Resolve on RPT_STATUS(cmd, OK) — for commands whose success reply is the ack. */
  okCompletes?: boolean;
  /**
   * Reply timeout in ms (default the device's `timeoutMs`): a number ≥ 0,
   * `Infinity` for none — the request then ends only with its reply, the
   * close or the loss; a negative or NaN one is a RangeError.
   */
  timeoutMs?: number;
}

interface PendingRequest {
  cmd: number;
  matcher: Matcher<unknown> | null;
  okCompletes: boolean;
  resolve: (value: unknown) => void;
  reject: (error: Error) => void;
  /** null: no timeout (`timeoutMs: Infinity`). */
  timer: ReturnType<typeof setTimeout> | null;
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

/** @internal What a device hands over: its started link and the transport under it. */
export interface HandedOverConnection {
  link: DepzLink;
  transport: SerialTransport;
}

/**
 * Connection to one DEPZ device in application mode. Opened by the discovery
 * functions (`openDeviceFrom`, `openDeviceByDeviceSerial`, `openOnTransport`,
 * the `/node` and `/web` `openDevice`); `close()` when done.
 */
export class DepzDevice {
  /**
   * What a `Matcher` returns to decline a packet (the Python reference's
   * `DepzDevice.NO_MATCH`).
   */
  static readonly NO_MATCH: unique symbol = Symbol("depz.NO_MATCH");

  timeoutMs: number;

  /**
   * The unsolicited / diagnostic events of the device (contract 07 §2):
   * `events.on(cb)` subscribes (read-pump context; do not block),
   * `events.subscribe()` iterates. There is no `events.next()` — no single
   * report to wait for (§3).
   */
  readonly events: Stream<DeviceEvent>;

  private readonly link: DepzLink;
  private readonly transport: SerialTransport;
  private readonly pending = new Map<number, PendingRequest>();
  private streams: Stream<unknown>[] = [];
  private timeSyncState: TimeSync | null = null;
  private closing = false;
  private torndown = false;
  private handedOver = false;
  private lostError: DeviceLostError | undefined;
  private readonly unsubLink: Array<() => void>;

  constructor(transport: SerialTransport, opts?: DeviceOptions) {
    this.transport = transport;
    this.timeoutMs = opts?.timeoutMs ?? DEFAULT_TIMEOUT_MS;
    const adopted = opts?.[ADOPT_LINK];
    this.link = adopted ?? new DepzLink(transport, { txCrcType: opts?.txCrcType ?? CrcType.None });
    if (adopted !== undefined && opts?.txCrcType !== undefined) adopted.txCrcType = opts.txCrcType;
    this.events = this.stream(256, null);
    this.unsubLink = [
      this.link.onParserEvent((ev) => this.handleParserEvent(ev)),
      this.link.onSeqGap((gap) => this.events.emit({ type: "seqGap", ...gap })),
      this.link.onClose((cause) => this.handleLinkClosed(cause)),
    ];
  }

  // ── lifecycle ──────────────────────────────────────────────────────────────

  /**
   * @internal Open the transport and start the read pump — the discovery
   * functions' step after construction. A no-op on a device that continues
   * an adopted connection (`attachSensorClass`) — already open.
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
    if (this.pending.size > 0 || this.streams.some((s) => s.hasQueues)) {
      throw new InvalidStateError("hand-over with a request in flight or a stream open");
    }
    for (const unsub of this.unsubLink) unsub();
    this.handedOver = true;
    return { link: this.link, transport: this.transport };
  }

  get stats(): LinkStats {
    return this.link.stats;
  }

  /**
   * The loss that ended this device (its `cause` is what the transport
   * raised), or undefined: still open, or ended by the program's own
   * `close()`. A pull iterator ends quietly either way and this tells which
   * (contract 07 §3).
   */
  get lost(): DeviceLostError | undefined {
    return this.lostError;
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
    if (error instanceof DeviceLostError) this.lostError = error;
    this.failAllPending(error);
    if (!this.closing) this.events.emit({ type: "disconnected", reason: error.message });
    // Let subclasses reject their own in-flight waiters (e.g. BNO086's SH-2
    // control layer) so a disconnect doesn't leave them hanging until their own
    // timeout, and clear any per-connection resolver state.
    this.onTeardown(error);
    for (const s of this.streams) s.close(error);
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
      clearTimer(p.timer);
      p.reject(error);
    }
  }

  // ── dispatch (read-pump context) ───────────────────────────────────────────

  private handleParserEvent(ev: ParserEvent): void {
    if (ev.type === "packet") {
      this.dispatch(ev);
    } else if (ev.type === "crcError") {
      this.events.emit({ type: "linkCrcError", cmd: ev.cmd, seq: ev.seq });
    } else {
      this.events.emit({ type: "trash", data: ev.data });
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
      this.events.emit({ type: "unsolicitedStatus", status: rep.status });
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
      if (result !== DepzDevice.NO_MATCH) {
        this.settle(p, { value: result });
        return;
      }
    }

    // Contract 07 §3: a report longer than its fields is decoded and the tail
    // ignored (a newer firmware may append fields); a shorter one is not a
    // report of that kind and stays visible as hex text below.
    if (pkt.cmd === Rpt.SequenceError && pkt.payload.length >= 2) {
      const rep = unpackSequenceError(pkt.payload);
      this.events.emit({
        type: "sequenceError",
        expectedSeq: rep.expectedSeq,
        receivedSeq: rep.receivedSeq,
        reportedByDevice: true,
      });
      return;
    }
    if (pkt.cmd === Rpt.Text && pkt.payload.length > 0) {
      const rep = unpackText(pkt.payload);
      this.events.emit({ type: "text", cmd: rep.cmd, text: rep.text });
      return;
    }
    if (pkt.cmd === Rpt.Temperature && pkt.payload.length >= 10) {
      const rep = unpackTemperature(pkt.payload);
      this.events.emit({
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
    this.events.emit({ type: "text", cmd: pkt.cmd, text: toHex(pkt.payload) });
  }

  private settle(
    p: PendingRequest,
    outcome: { value: unknown; error?: undefined } | { error: Error },
  ): void {
    clearTimer(p.timer);
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

  /**
   * @internal The stream of one kind of item this device emits, closed with
   * the device (contract 07 §3). A sensor class makes one per field
   * (`readonly measurements = this.stream<Sr04Measurement>(256, Sr04Rpt.Data)`);
   * `what` is the report `next()` names in its timeout error, null for a
   * stream that offers no `next()` (`events`).
   */
  protected stream<T>(maxsize: number, what: number | null): Stream<T> {
    const s = new Stream<T>(this.link.stats, maxsize, what);
    this.streams.push(s as Stream<unknown>);
    return s;
  }

  // ── TX / requests ──────────────────────────────────────────────────────────

  /**
   * Send `cmd` and wait for its correlated completion (contract 02 §1).
   *
   * Exactly one of the completion paths must be configured:
   * - `okCompletes` — RPT_STATUS(cmd, OK) resolves with undefined;
   * - `matcher` — first packet for which the matcher returns non-`DepzDevice.NO_MATCH`
   *   resolves with the matcher's return value.
   * Non-OK RPT_STATUS echoing `cmd` always rejects (BusyError for ERR_BUSY,
   * StatusError otherwise). One in-flight request per opcode. An argument
   * the framing refuses (a payload over 16383 bytes, a cmd that is no u8, a
   * negative timeout) is a RangeError, nothing written (contract 07 §3).
   */
  /**
   * Throws what every request of a handed-over, lost or closed device throws (contract 07 §3): lost
   * (whatever came after it), or closed by this side. A sensor class calls it before a guard of its
   * own state — "stop the stream first" — so that a board unplugged while it streamed says it is
   * lost, not that it still streams.
   */
  protected requireOpen(): void {
    if (this.handedOver) throw new LinkClosedError("connection handed over to another device object");
    if (this.lostError !== undefined) {
      const lost = new DeviceLostError(this.lostError.message);
      lost.cause = this.lostError.cause;
      throw lost;
    }
    if (this.closing) throw new LinkClosedError("device closed");
  }

  async request<T = void>(
    cmd: number,
    payload: Uint8Array = new Uint8Array(0),
    opts: RequestOptions<T> = {},
  ): Promise<T> {
    const { matcher, okCompletes = false } = opts;
    if (matcher === undefined && !okCompletes) {
      throw new DepzError("configure matcher or okCompletes");
    }
    this.requireOpen();
    if (this.pending.has(cmd)) throw new BusyError(cmd);
    const timeoutMs = checkTimeoutMs("timeoutMs", opts.timeoutMs ?? this.timeoutMs);
    // Framed before the request exists: an argument the framing refuses is
    // the caller's RangeError as it is (contract 07 §3) — nothing written, no
    // slot taken, not a "write failed" on a device that is fine.
    const frame = this.link.frame(cmd, payload);
    const result = new Promise<T>((resolve, reject) => {
      const timer = startTimeout(
        "timeoutMs",
        () => {
          this.pending.delete(cmd);
          reject(new DepzTimeoutError(cmd, timeoutMs));
        },
        timeoutMs,
      );
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
      await this.link.sendFrame(frame);
    } catch (err) {
      this.forgetPending(cmd);
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

  /** Free the slot of a request whose write failed (its timer with it). */
  private forgetPending(cmd: number): void {
    const p = this.pending.get(cmd);
    if (p !== undefined) {
      clearTimer(p.timer);
      this.pending.delete(cmd);
    }
  }

  /** Matcher for a typed report identified by its report ID alone. */
  static expectReport<T>(reportId: number, unpack: (payload: Uint8Array) => T): Matcher<T> {
    return (pkt) => (pkt.cmd === reportId ? unpack(pkt.payload) : DepzDevice.NO_MATCH);
  }

  /** Matcher for RPT_TEXT echoing `requestCmd`. */
  static expectText(requestCmd: number): Matcher<string> {
    return (pkt) => {
      if (pkt.cmd !== Rpt.Text || pkt.payload.length === 0 || pkt.payload[0] !== requestCmd) {
        return DepzDevice.NO_MATCH;
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
      matcher: DepzDevice.expectReport(Rpt.PayloadCrcType, (p) => {
        const mode = unpackPayloadCrcType(p);
        // a value no CrcType names is an invalid reply (contract 07 §3)
        if (mode > CrcType.Crc32) throw new RangeError(`RPT_PAYLOAD_CRC_TYPE: ${mode} is not a CrcType`);
        return mode as CrcType;
      }),
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

function toHex(data: Uint8Array): string {
  let s = "";
  for (const b of data) s += b.toString(16).padStart(2, "0");
  return s;
}
