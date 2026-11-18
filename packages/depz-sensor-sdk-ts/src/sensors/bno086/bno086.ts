/**
 * BNO086 9-axis IMU: SHTP + SH-2 host stack over the firmware pass-through
 * bridge (contracts/05_SENSOR_BNO086.md).
 *
 * The MCU only shuttles raw SHTP frames (SEND_SHTP_PACKET / RPT_DATA); the
 * whole sensor-hub protocol runs here. Per ERRATA E2 the bridge ACKs every
 * send with RPT_STATUS immediately and *all* inbound SHTP arrives as
 * RPT_DATA(cmd=0x00) — correlation happens at the SH-2 layer only.
 *
 * Mirrors the Python reference `depz_sensor_sdk.bno086.Bno086`.
 */

import { DepzDevice, type DeviceOptions } from "../../device/device.js";
import { BusyError, DepzError, DepzTimeoutError, InvalidReplyError } from "../../errors.js";
import type { PacketEvent } from "../../protocol/framing.js";
import {
  BUSY_BACKOFF_MS,
  Bno086Cmd,
  Bno086Rpt,
  unpackBno086Data,
} from "../../protocol/bno086.js";
import { clearTimer, startTimeout } from "../../timeout.js";
import type { SerialTransport } from "../../transport/types.js";
import { FLUSH_COMPLETED, SensorId, parseGyroRvCargo, parseInputCargo, type Report } from "./reports.js";
import {
  ControlReport,
  ErrorSource,
  FrsReadSession,
  FrsWriteSession,
  ME_CAL_GET,
  METADATA_RECORDS,
  OscillatorType,
  Sh2Command,
  Sh2Error,
  TareAxis,
  TareBasis,
  buildCommandRequest,
  buildForceFlush,
  buildGetFeatureRequest,
  buildProductIdRequest,
  buildSetFeature,
  splitControlCargo,
  countsClearParams,
  countsGetParams,
  errorRecordFromResponse,
  errorsParams,
  interactiveCalibrationParams,
  meCalibrationParams,
  periodicDcdParams,
  persistTareParams,
  sensorMetadataFromWords,
  setReorientationParams,
  tareNowParams,
  unpackCommandResponse,
  unpackFeatureResponse,
  unpackFrsReadResponse,
  unpackFrsWriteResponse,
  unpackProductId,
  type CommandResponse,
  type Counts,
  type ErrorRecord,
  type FeatureResponse,
  type ProductId,
  type SensorMetadata,
} from "./sh2.js";
import { ExecutableCommand, ShtpChannel, ShtpLayer, type ShtpCargo } from "./shtp.js";

// Rate verification bounds (contract 05 §7): the hub grants a grid rate; a
// result outside [0.9, 2.1]× the requested rate is worth a warning, never an
// error.
export const RATE_LOW_FACTOR = 0.9;
export const RATE_HIGH_FACTOR = 2.1;

/**
 * Re-reads allowed while `enable()`'s verify reads a rate outside the band
 * (contract 05 §7): a "disabled" answer stale from a preceding `disable()`, or
 * the hub answering before it runs the new rate — interval 0 for one or two
 * read-backs on the lab BNO085, and for up to 95 ms (0, or a slower rate) on a
 * hub just reset (bench 2026-11-12). A pause before each: about 0.4 s in all,
 * four times the longest seen. Mirrors the Python SDK.
 */
const VERIFY_REREADS = 15;
const VERIFY_PAUSE_MS = 20;

/** Whether a granted interval is the rate asked for, within the band of contract 05 §7 (0 = disabled: never). */
function rateInBand(requestedUs: number, grantedUs: number): boolean {
  const requestedRate = 1_000_000 / requestedUs;
  const actualRate = grantedUs !== 0 ? 1_000_000 / grantedUs : 0;
  return RATE_LOW_FACTOR * requestedRate <= actualRate && actualRate <= RATE_HIGH_FACTOR * requestedRate;
}

const EXECUTABLE_RESET_COMPLETE = 0x01;

const CONTROL_TIMEOUT_MS = 1000;
const FRS_TIMEOUT_MS = 2000;

/** ME calibration enables as reported by the sensor. */
export interface CalibrationConfig {
  accel: boolean;
  gyro: boolean;
  mag: boolean;
  planar: boolean;
}

export interface Bno086Options extends DeviceOptions {
  /** SEND_SHTP_PACKET attempts before giving up (default 5). */
  busyRetries?: number;
  /** ERR_BUSY backoff; >= 200 ms per the bridge spec (tests inject less). */
  busyBackoffMs?: number;
}

export interface EnableOptions {
  /** Alternative to `hz`: exact report interval in µs. */
  intervalUs?: number;
  batchUs?: number;
  sensitivity?: number;
  flags?: number;
  cfgWord?: number;
  /** Read back the granted rate via Get Feature (default true). */
  verify?: boolean;
  timeoutMs?: number;
}

async function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

type ControlWaiter = (payload: Uint8Array) => void;

/**
 * A control response the hub answered a request with, decoded; one shorter
 * than its fields (or with the wrong report id) fails that request with
 * InvalidReplyError (contract 07 §3), never a bare RangeError.
 */
function decodeReply<T>(rid: number, raw: Uint8Array, unpack: (raw: Uint8Array) => T): T {
  try {
    return unpack(raw);
  } catch (err) {
    throw new InvalidReplyError(rid, err);
  }
}

/**
 * BNO086 device: enable SH-2 sensors, stream typed reports.
 *
 * Typical use:
 *
 *     const imu = await openOnTransport(transport);   // or openDeviceFrom(...)
 *     if (!(imu instanceof Bno086)) throw new Error("not a BNO086");
 *     await imu.enable(SensorId.RotationVector, 100);
 *     for await (const r of imu.reports.subscribe(64)) { ... }
 *
 * Callbacks run on the read-pump context — never await blocking device
 * methods (enable/tare/...) from inside one.
 */
export class Bno086 extends DepzDevice {
  /** SEND_SHTP_PACKET attempts before giving up. */
  busyRetries = 5;
  /** >= 200 ms per the bridge spec (contract 05 §2). */
  busyBackoffMs: number = BUSY_BACKOFF_MS;

  /**
   * The typed sensor reports (contract 07 §3): `on(cb, { where })` subscribes
   * (`where` keeps the reports it accepts — a SensorId filter),
   * `subscribe(maxsize, { where })` iterates, `next(timeoutMs)` waits for one;
   * `droppedCounts` and `parseErrors` (RPT_DATA reports shorter than their
   * header, contract 07 §2) are its counters.
   */
  readonly reports = this.stream<Report>(1024, Bno086Rpt.Data);

  private shtp = new ShtpLayer();
  private txChain: Promise<void> = Promise.resolve();
  private readonly flushWaiters = new Map<number, Set<() => void>>();
  // Rejecters of the flush() calls in flight, failed at teardown.
  private flushFails = new Set<(err: Error) => void>();
  private controlWaiters = new Map<number, ControlWaiter[]>();
  // Parallel registry of the `fail` handle for each in-flight control exchange,
  // so a link close can reject them all promptly (they are otherwise only
  // rejected by their own 1-2 s timeout — see onTeardown).
  private controlFails = new Set<(err: Error) => void>();
  private cmdSeq = 0;
  // The reset-complete waits in flight (`hardwareReset`, `clearDcdAndReset`):
  // resolved by the executable channel's 0x01, rejected at teardown.
  private resetWaiters: Array<{ resolve: () => void; reject: (err: Error) => void }> = [];
  private advertisementChunks: Uint8Array[] = [];
  private features = new Map<number, FeatureResponse>();

  constructor(transport: SerialTransport, opts?: Bno086Options) {
    super(transport, opts);
    if (opts?.busyRetries !== undefined) this.busyRetries = opts.busyRetries;
    if (opts?.busyBackoffMs !== undefined) this.busyBackoffMs = opts.busyBackoffMs;
  }

  protected override onTeardown(error: Error): void {
    // Reject every in-flight SH-2 control exchange now (each `fail` clears its
    // own timer + registry entry via cleanup) instead of leaving it to hang the
    // full control/FRS timeout; the same for the flushes and the reset-complete
    // waits (a lost device says so at once, not after the reset timeout —
    // the Python reference's `_waiters.failure`).
    for (const fail of [...this.controlFails]) fail(error);
    this.controlFails.clear();
    for (const fail of [...this.flushFails]) fail(error);
    this.flushFails.clear();
    this.controlWaiters.clear();
    for (const w of this.resetWaiters.splice(0)) w.reject(error);
  }

  /**
   * A promise for the next executable reset-complete (0x01). Rejected at
   * teardown with the device's error; marked handled, so one left behind by
   * a request that failed before the wait never surfaces as unhandled.
   */
  private resetComplete(): { done: Promise<void>; forget: () => void } {
    let entry!: { resolve: () => void; reject: (err: Error) => void };
    const done = new Promise<void>((resolve, reject) => {
      entry = { resolve, reject };
    });
    void done.catch(() => undefined);
    this.resetWaiters.push(entry);
    return {
      done,
      forget: () => {
        this.resetWaiters = this.resetWaiters.filter((w) => w !== entry);
      },
    };
  }

  // ── RX path ────────────────────────────────────────────────────────────────

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Bno086Rpt.Data) return false;
    let data: ReturnType<typeof unpackBno086Data>;
    try {
      // short header: refused (RangeError) and counted — contract 07 §2
      data = unpackBno086Data(pkt.payload);
    } catch {
      this.reports.parseErrors += 1;
      return true;
    }
    const cargo = this.shtp.feed(data.shtp, data.timestampUs);
    // The first fragment's capture: 0xFB base deltas refer to it.
    if (cargo !== null) this.dispatchCargo(cargo, cargo.captureUs);
    return true;
  }

  private dispatchCargo(cargo: ShtpCargo, captureUs: bigint): void {
    switch (cargo.channel) {
      case ShtpChannel.Command:
        this.advertisementChunks.push(cargo.payload);
        return;
      case ShtpChannel.Executable:
        if (cargo.payload[0] === EXECUTABLE_RESET_COMPLETE) {
          for (const w of this.resetWaiters.splice(0)) w.resolve();
        }
        return;
      case ShtpChannel.Control: {
        // One cargo may hold several responses back to back.
        for (const report of splitControlCargo(cargo.payload)) {
          const rid = report[0]!;
          if (rid === ControlReport.GetFeatureResponse && report.length >= 17) {
            const resp = unpackFeatureResponse(report);
            this.features.set(resp.sensorId, resp);
          }
          for (const waiter of [...(this.controlWaiters.get(rid) ?? [])]) {
            waiter(report.slice());
          }
        }
        return;
      }
      case ShtpChannel.InputNormal:
      case ShtpChannel.InputWake:
        for (const rep of parseInputCargo(cargo.payload, captureUs)) this.emitReport(rep);
        return;
      case ShtpChannel.GyroRv: {
        const rep = parseGyroRvCargo(cargo.payload, captureUs);
        if (rep !== null) this.emitReport(rep);
        return;
      }
    }
  }

  private emitReport(rep: Report): void {
    if (rep.type === "FlushCompleted") {
      for (const done of [...(this.flushWaiters.get(rep.flushedSensor) ?? [])]) done();
    }
    this.reports.emit(rep);
  }

  // ── TX path ────────────────────────────────────────────────────────────────

  /**
   * Frame `payload` and push it through SEND_SHTP_PACKET.
   *
   * ERR_BUSY (both MCU TX slots full) backs off `busyBackoffMs` and
   * retransmits, up to `busyRetries` attempts (contract 05 §2). Sends are
   * serialized (single opcode, single wire).
   */
  private async sendShtp(channel: number, payload: Uint8Array): Promise<void> {
    const frame = this.shtp.nextFrame(channel, payload);
    const run = async (): Promise<void> => {
      for (let attempt = 0; ; attempt++) {
        try {
          await this.request(Bno086Cmd.SendShtpPacket, frame, { okCompletes: true });
          return;
        } catch (err) {
          if (!(err instanceof BusyError) || attempt >= this.busyRetries - 1) throw err;
          await sleep(this.busyBackoffMs);
        }
      }
    };
    const result = this.txChain.then(run);
    this.txChain = result.catch(() => undefined);
    return result;
  }

  /**
   * Register a control-channel waiter for `rid`, run `send`, and let
   * `handler` decide when the exchange finishes. Correlation is purely
   * SH-2-level (report ID + handler predicate).
   */
  private async controlExchange<T>(
    rid: number,
    timeoutMs: number,
    handler: (raw: Uint8Array, finish: (value: T) => void, fail: (err: Error) => void) => void,
    send: () => Promise<void>,
  ): Promise<T> {
    return new Promise<T>((resolve, reject) => {
      let settled = false;
      const cleanup = (): void => {
        clearTimer(timer);
        this.controlFails.delete(fail);
        const list = this.controlWaiters.get(rid);
        if (list !== undefined) {
          this.controlWaiters.set(
            rid,
            list.filter((w) => w !== waiter),
          );
        }
      };
      const finish = (value: T): void => {
        if (settled) return;
        settled = true;
        cleanup();
        resolve(value);
      };
      const fail = (err: Error): void => {
        if (settled) return;
        settled = true;
        cleanup();
        reject(err);
      };
      // A bad timeout (negative, NaN) throws here, before the waiter exists:
      // nothing is registered, nothing sent (contract 07 §3); Infinity arms
      // no timer — the exchange then ends with its response or the device.
      const timer = startTimeout("timeoutMs", () => fail(new DepzTimeoutError(rid, timeoutMs)), timeoutMs);
      const waiter: ControlWaiter = (raw) => handler(raw, finish, fail);
      const list = this.controlWaiters.get(rid) ?? [];
      list.push(waiter);
      this.controlWaiters.set(rid, list);
      this.controlFails.add(fail);
      send().catch((err: unknown) => fail(err instanceof Error ? err : new DepzError(String(err))));
    });
  }

  /** Send a control-channel payload and wait for one matching response. */
  private async controlRequest<T>(
    payload: Uint8Array,
    expectRid: number,
    parse: (raw: Uint8Array) => T,
    pred?: (obj: T) => boolean,
    timeoutMs: number = CONTROL_TIMEOUT_MS,
  ): Promise<T> {
    return this.controlExchange<T>(
      expectRid,
      timeoutMs,
      (raw, finish, fail) => {
        let obj: T;
        try {
          obj = decodeReply(expectRid, raw, parse);
        } catch (err) {
          fail(err as Error);
          return;
        }
        if (pred === undefined || pred(obj)) finish(obj);
      },
      async () => this.sendShtp(ShtpChannel.Control, payload),
    );
  }

  private nextCmdSeq(): number {
    const seq = this.cmdSeq;
    this.cmdSeq = (seq + 1) & 0xff;
    return seq;
  }

  /**
   * SH-2 Command Request; correlates the response on (command, commandSeq)
   * when the command produces one.
   */
  private async command(
    command: number,
    params: Uint8Array,
    waitResponse: boolean,
    timeoutMs: number = CONTROL_TIMEOUT_MS,
  ): Promise<CommandResponse | null> {
    const seq = this.nextCmdSeq();
    const payload = buildCommandRequest(seq, command, params);
    if (!waitResponse) {
      await this.sendShtp(ShtpChannel.Control, payload);
      return null;
    }
    return this.controlRequest(
      payload,
      ControlReport.CommandResponse,
      unpackCommandResponse,
      (r) => r.command === command && r.commandSeq === seq,
      timeoutMs,
    );
  }

  /**
   * Send a Command Request and gather every Command Response correlated on
   * (command, commandSeq) until `collect(resp, acc)` returns true. Used by
   * multi-message commands (Errors, Counts) where the hub streams several
   * 0xF1 responses with an incrementing responseSeq.
   */
  private async commandCollect(
    command: number,
    params: Uint8Array,
    collect: (resp: CommandResponse, acc: CommandResponse[]) => boolean,
    timeoutMs: number = CONTROL_TIMEOUT_MS,
  ): Promise<CommandResponse[]> {
    const seq = this.nextCmdSeq();
    const payload = buildCommandRequest(seq, command, params);
    const results: CommandResponse[] = [];
    return this.controlExchange<CommandResponse[]>(
      ControlReport.CommandResponse,
      timeoutMs,
      (raw, finish, fail) => {
        let resp: CommandResponse;
        try {
          resp = decodeReply(ControlReport.CommandResponse, raw, unpackCommandResponse);
        } catch (err) {
          fail(err as Error);
          return;
        }
        if (resp.command !== command || resp.commandSeq !== seq) return;
        if (collect(resp, results)) finish(results);
      },
      async () => this.sendShtp(ShtpChannel.Control, payload),
    );
  }

  // ── lifecycle ──────────────────────────────────────────────────────────────

  /**
   * Hard-reset the sensor via nRST (0x32). All SHTP state (seq counters,
   * partial cargos) and cached features restart from zero.
   *
   * The hub's RPT_STATUS OK ack for the 0x32 command is treated as the reset
   * confirmation. The SH-2 executable-channel reset-complete (which the BNO08X
   * SH-2 spec would emit) is only waited for best-effort: older firmware
   * (≤ v0.95) did **not** emit it (ERRATA E9, now fixed
   * in newer firmware); the best-effort wait handles both, so its absence is
   * *not* an error — the sensor is fully usable without it.
   */
  async hardwareReset(timeoutMs = 2000): Promise<void> {
    this.shtp.reset();
    this.advertisementChunks = [];
    this.features.clear();
    const { done, forget } = this.resetComplete();
    // RPT_STATUS OK from the hub confirms the reset command was accepted.
    try {
      await this.request(Bno086Cmd.SensorReset, undefined, { okCompletes: true, timeoutMs });
    } catch (err) {
      forget(); // never sent: no reset-complete to wait for
      throw err;
    }
    // Best-effort wait for the SH-2 executable reset-complete; older firmware
    // (≤ v0.95) legitimately never sent it (ERRATA E9, now fixed in newer
    // firmware) — the best-effort wait handles both, so resolve either way.
    let timer: ReturnType<typeof setTimeout> | undefined;
    const bestEffort = new Promise<void>((resolve) => {
      timer = setTimeout(resolve, Math.min(timeoutMs, 500));
    });
    try {
      await Promise.race([done, bestEffort]);
    } finally {
      clearTimeout(timer);
      forget();
    }
  }

  /**
   * Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.
   *
   * This is the board's hardware pin pulse (board command 0x33), not an SH-2
   * command; the SH-2 "On" command that ends {@link sleep} is {@link powerOn}.
   */
  async wake(): Promise<void> {
    await this.request(Bno086Cmd.SensorWakeUp, undefined, { okCompletes: true });
  }

  /**
   * Put the hub to sleep: SH-2 "Sleep" (0x03 on SHTP channel 1).
   *
   * Every report stops, except those enabled with
   * `flags: FeatureFlag.Wakeup | FeatureFlag.AlwaysOn`, which keep their rate;
   * the configuration (enabled sensors, rates) is kept. The hub sends no
   * response. {@link powerOn} ends it.
   *
   * On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16): all reports stopped within one
   * sample; a Wakeup|AlwaysOn report kept streaming at full rate on channel 4.
   */
  async sleep(): Promise<void> {
    await this.sendShtp(ShtpChannel.Executable, Uint8Array.of(ExecutableCommand.Sleep));
  }

  /**
   * End {@link sleep}: SH-2 "On" (0x02 on SHTP channel 1).
   *
   * Every report that was enabled comes back at its rate — no need to
   * re-enable. The hub sends no response. Not to be confused with
   * {@link wake}, which pulses the WAKE pin (board command 0x33) instead of
   * sending an SH-2 command.
   */
  async powerOn(): Promise<void> {
    await this.sendShtp(ShtpChannel.Executable, Uint8Array.of(ExecutableCommand.On));
  }

  /** Raw SHTP channel-0 advertisement bytes seen since open/reset. */
  get advertisement(): Uint8Array {
    const total = this.advertisementChunks.reduce((n, c) => n + c.length, 0);
    const out = new Uint8Array(total);
    let off = 0;
    for (const c of this.advertisementChunks) {
      out.set(c, off);
      off += c.length;
    }
    return out;
  }

  // ── SH-2: identification & features ──────────────────────────────────────

  /** The SH-2 application's Product ID — the first of `productIds()`. */
  async productId(timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<ProductId> {
    return (await this.productIds(timeoutMs))[0]!;
  }

  /**
   * Product ID Request → every component's response, in the sensor's order
   * (the SH-2 application first). Collects until no response came for
   * `settleMs`, so the late ones can't answer the next request (they arrive in
   * a second cargo; taking only the first once returned the bootloader's
   * version on a back-to-back call).
   */
  async productIds(timeoutMs: number = CONTROL_TIMEOUT_MS, settleMs = 150): Promise<ProductId[]> {
    const out: ProductId[] = [];
    let settle: ReturnType<typeof setTimeout> | undefined;
    return this.controlExchange<ProductId[]>(
      ControlReport.ProductIdResponse,
      timeoutMs + settleMs,
      (raw, finish, fail) => {
        try {
          out.push(decodeReply(ControlReport.ProductIdResponse, raw, unpackProductId));
        } catch (err) {
          fail(err as Error);
          return;
        }
        clearTimeout(settle);
        settle = setTimeout(() => finish(out), settleMs);
      },
      async () => this.sendShtp(ShtpChannel.Control, buildProductIdRequest()),
    ).finally(() => clearTimeout(settle));
  }

  /**
   * Enable `sensor` at the requested rate via Set Feature (0xFD).
   *
   * Give either `hz` or `opts.intervalUs`. The hub rounds to its 1 kHz/2^n
   * grid; with `verify` (default) the granted rate is read back via Get
   * Feature and a result outside 0.9–2.1× the request emits a console
   * warning (contract 05 §7 — warn, never throw). Resolves with the
   * FeatureResponse (null when `verify: false`).
   */
  async enable(sensor: number, hz?: number, opts: EnableOptions = {}): Promise<FeatureResponse | null> {
    if ((hz === undefined) === (opts.intervalUs === undefined)) {
      throw new DepzError("give exactly one of hz / intervalUs");
    }
    let intervalUs: number;
    if (hz !== undefined) {
      if (hz <= 0) throw new DepzError("hz must be positive; use disable()");
      intervalUs = Math.max(1, Math.round(1_000_000 / hz));
    } else {
      intervalUs = opts.intervalUs!;
    }
    await this.sendShtp(
      ShtpChannel.Control,
      buildSetFeature(
        sensor,
        intervalUs,
        opts.batchUs ?? 0,
        opts.sensitivity ?? 0,
        opts.flags ?? 0,
        opts.cfgWord ?? 0,
      ),
    );
    if (opts.verify === false) return null;
    const resp = await this.verifyFeature(sensor, intervalUs, opts.timeoutMs ?? CONTROL_TIMEOUT_MS);
    if (!rateInBand(intervalUs, resp.intervalUs)) {
      const requestedRate = 1_000_000 / intervalUs;
      const actualRate = resp.intervalUs !== 0 ? 1_000_000 / resp.intervalUs : 0;
      console.warn(
        `BNO086 sensor 0x${sensor.toString(16).toUpperCase().padStart(2, "0")}: requested ` +
          `${requestedRate.toFixed(1)} Hz, granted ${actualRate.toFixed(1)} Hz (outside ` +
          `${RATE_LOW_FACTOR}–${RATE_HIGH_FACTOR}× band)`,
      );
    }
    return resp;
  }

  /**
   * Read back a feature we just *enabled*, until the hub runs it.
   *
   * The hub emits an unsolicited Get Feature Response for every
   * state-changing Set Feature — including the `disable()` that typically
   * precedes a re-enable — and Get Feature Response carries no correlation
   * token (contract 05 §6: only `sensorId` identifies it). It also answers
   * before it runs the rate just set: interval 0 for one or two read-backs on
   * a hub that has been up, and for up to 95 ms — interval 0, or a slower
   * rate — on one just reset. So an answer outside the band of the rate asked
   * for is stale, early or a genuine refusal: re-read a bounded number of
   * times, a pause before each — a refusal survives the re-reads and reaches
   * the warning path.
   */
  private async verifyFeature(sensor: number, intervalUs: number, timeoutMs: number): Promise<FeatureResponse> {
    let resp = await this.getFeature(sensor, timeoutMs);
    for (let i = 0; i < VERIFY_REREADS && !rateInBand(intervalUs, resp.intervalUs); i++) {
      await sleep(VERIFY_PAUSE_MS);
      resp = await this.getFeature(sensor, timeoutMs);
    }
    return resp;
  }

  /** Disable `sensor` (Set Feature with interval 0). */
  async disable(sensor: number): Promise<void> {
    await this.sendShtp(ShtpChannel.Control, buildSetFeature(sensor, 0));
    this.features.delete(sensor);
  }

  /** Get Feature Request/Response round trip for `sensor`. */
  async getFeature(sensor: number, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<FeatureResponse> {
    return this.controlRequest(
      buildGetFeatureRequest(sensor),
      ControlReport.GetFeatureResponse,
      unpackFeatureResponse,
      (r) => r.sensorId === sensor,
      timeoutMs,
    );
  }

  // ── tare / calibration facade ──────────────────────────────────────────────

  /** Tare the selected axes against `basis` (no response per SH-2). */
  async tareNow(axes: number = TareAxis.All, basis: number = TareBasis.RotationVector): Promise<void> {
    await this.command(Sh2Command.Tare, tareNowParams(axes, basis), false);
  }

  /** Persist the current tare into FRS (no response per SH-2). */
  async persistTare(): Promise<void> {
    await this.command(Sh2Command.Tare, persistTareParams(), false);
  }

  /**
   * Set the runtime reorientation quaternion (Q14 on the wire; all zeros
   * clears). No response per SH-2.
   *
   * The rotation is applied on the board's side (new = report * r) and is
   * given in the hub's own axes: x and y swapped and z negated relative to
   * the axes of the reports — a rotation about x here turns the reports
   * about y (bench, 2026-10-22, SH-2 3.12.6). The System Orientation record
   * (FRS 0x2D3E) uses the same axes and is read only when the hub starts.
   */
  async setReorientation(x: number, y: number, z: number, w: number): Promise<void> {
    await this.command(Sh2Command.Tare, setReorientationParams(x, y, z, w), false);
  }

  /** Configure ME calibration; rejects with Sh2Error on non-zero status. */
  async setCalibration(
    config: { accel?: boolean; gyro?: boolean; mag?: boolean; planar?: boolean } = {},
    timeoutMs: number = CONTROL_TIMEOUT_MS,
  ): Promise<void> {
    const resp = await this.command(
      Sh2Command.MeCalibrate,
      meCalibrationParams(config.accel ?? true, config.gyro ?? true, config.mag ?? true, config.planar ?? false),
      true,
      timeoutMs,
    );
    if (resp!.status !== 0) {
      throw new Sh2Error(`ME calibration configure failed: status ${resp!.status}`);
    }
  }

  /** Read back which ME calibrations are running. */
  async getCalibration(timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<CalibrationConfig> {
    const resp = await this.command(
      Sh2Command.MeCalibrate,
      meCalibrationParams(false, false, false, false, ME_CAL_GET),
      true,
      timeoutMs,
    );
    if (resp!.status !== 0) {
      throw new Sh2Error(`ME calibration get failed: status ${resp!.status}`);
    }
    const r = resp!.r;
    return { accel: Boolean(r[1]), gyro: Boolean(r[2]), mag: Boolean(r[3]), planar: Boolean(r[4]) };
  }

  /** Save the dynamic calibration data to flash (DCD Save Now). */
  async saveDcd(timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<void> {
    const resp = await this.command(Sh2Command.SaveDcd, new Uint8Array(0), true, timeoutMs);
    if (resp!.status !== 0) {
      throw new Sh2Error(`DCD save failed: status ${resp!.status}`);
    }
  }

  /** Enable/disable the hub's periodic DCD autosave (no response). */
  async configurePeriodicDcd(enable: boolean): Promise<void> {
    await this.command(Sh2Command.PeriodicDcdConfig, periodicDcdParams(enable), false);
  }

  // ── interactive calibration (BNO086 only) ──────────────────────────────────

  /**
   * Tell the hub how the device is about to move (Interactive Calibration,
   * command 0x0E, see MotionIntent), so it can refine the gyro zero-rate
   * offset while the device is still. No response per SH-2; a BNO085 ignores
   * it. The hub's answer, if it wants the device to stop, arrives as a
   * MotionRequest report (enable SensorId.MotionRequest).
   */
  async setMotionIntent(intent: number): Promise<void> {
    await this.command(Sh2Command.InteractiveCalibration, interactiveCalibrationParams(intent), false);
  }

  /**
   * Enable MotionRequest (0x2B) reports. The hub sends one when its request
   * changes, not at `hz`; it reads the rate back late, so the granted rate is
   * not checked.
   */
  async enableMotionRequest(hz = 1, opts: EnableOptions = {}): Promise<FeatureResponse | null> {
    return this.enable(SensorId.MotionRequest, hz, { verify: false, ...opts });
  }

  // ── batching ───────────────────────────────────────────────────────────────

  /**
   * Force Sensor Flush (0xF0): have the hub send `sensor`'s batched reports
   * now instead of when its batch interval runs out. Resolves once the hub's
   * Flush Completed (0xEF) for `sensor` arrives — every report batched before
   * the call has been emitted by then.
   */
  async flush(sensor: number, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<void> {
    const payload = buildForceFlush(sensor);
    let waiters = this.flushWaiters.get(sensor);
    if (waiters === undefined) {
      waiters = new Set();
      this.flushWaiters.set(sensor, waiters);
    }
    let done!: () => void;
    let fail!: (err: Error) => void;
    let timer: ReturnType<typeof setTimeout> | null = null;
    const completed = new Promise<void>((resolve, reject) => {
      done = resolve;
      fail = reject;
      timer = startTimeout("timeoutMs", () => reject(new DepzTimeoutError(FLUSH_COMPLETED, timeoutMs)), timeoutMs);
    });
    // A teardown during the send rejects `completed` before it is awaited.
    void completed.catch(() => undefined);
    waiters.add(done);
    this.flushFails.add(fail);
    try {
      await this.sendShtp(ShtpChannel.Control, payload);
      await completed;
    } finally {
      clearTimer(timer);
      waiters.delete(done);
      this.flushFails.delete(fail);
    }
  }

  // ── FRS ────────────────────────────────────────────────────────────────────

  /** Read a whole FRS record; resolves with its 32-bit words. */
  async frsRead(recordId: number, timeoutMs: number = FRS_TIMEOUT_MS): Promise<number[]> {
    const session = new FrsReadSession(recordId);
    return this.controlExchange<number[]>(
      ControlReport.FrsReadResponse,
      timeoutMs,
      (raw, finish, fail) => {
        try {
          const resp = decodeReply(ControlReport.FrsReadResponse, raw, unpackFrsReadResponse);
          if (session.feed(resp)) finish(session.words);
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
        }
      },
      async () => this.sendShtp(ShtpChannel.Control, session.request()),
    );
  }

  /** Write a whole FRS record (word list); rejects with Sh2Error on failure. */
  async frsWrite(recordId: number, words: Iterable<number>, timeoutMs: number = FRS_TIMEOUT_MS): Promise<void> {
    const session = new FrsWriteSession(recordId, [...words]);
    return this.controlExchange<void>(
      ControlReport.FrsWriteResponse,
      timeoutMs,
      (raw, finish, fail) => {
        try {
          const resp = decodeReply(ControlReport.FrsWriteResponse, raw, unpackFrsWriteResponse);
          const next = session.feed(resp);
          if (session.done) {
            finish(undefined);
          } else if (next !== null) {
            this.sendShtp(ShtpChannel.Control, next).catch(fail);
          }
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
        }
      },
      async () => this.sendShtp(ShtpChannel.Control, session.request()),
    );
  }

  /** Read + parse the sensor's FRS metadata record. */
  async getMetadata(sensor: number, timeoutMs: number = FRS_TIMEOUT_MS): Promise<SensorMetadata> {
    const record = METADATA_RECORDS[sensor];
    if (record === undefined) {
      throw new Sh2Error(
        `no metadata FRS record known for sensor 0x${sensor.toString(16).toUpperCase().padStart(2, "0")}`,
      );
    }
    return sensorMetadataFromWords(await this.frsRead(record, timeoutMs));
  }

  // ── diagnostics / housekeeping commands ──────────────────────────────────────

  /** Get Oscillator Type (command 0x0A). r[0] is the type directly. */
  async getOscillatorType(timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<OscillatorType> {
    const resp = await this.command(Sh2Command.GetOscillatorType, new Uint8Array(0), true, timeoutMs);
    return resp!.r[0]! as OscillatorType;
  }

  /**
   * Clear the in-RAM dynamic calibration and reset the sensor (command 0x0B).
   * There is no command response — the hub resets, so this waits for the
   * executable reset-complete like hardwareReset(): `DepzTimeoutError` when
   * it does not come in `timeoutMs` (`Infinity` for no limit), the device's
   * own error as soon as the device is closed or lost while waiting.
   */
  async clearDcdAndReset(timeoutMs = 2000): Promise<void> {
    this.shtp.reset();
    this.advertisementChunks = [];
    this.features.clear();
    const { done, forget } = this.resetComplete();
    let timer: ReturnType<typeof setTimeout> | null = null;
    try {
      await this.command(Sh2Command.ClearDcdAndReset, new Uint8Array(0), false);
      const timedOut = new Promise<never>((_, reject) => {
        timer = startTimeout(
          "timeoutMs",
          () => reject(new DepzTimeoutError(Sh2Command.ClearDcdAndReset, timeoutMs)),
          timeoutMs,
        );
      });
      await Promise.race([done, timedOut]);
    } finally {
      clearTimer(timer);
      forget();
    }
    // The device restarted its counters on reset; realign the SHTP layer's TX
    // side: what the hub sent since the reset is in the reassembly (bug hunt
    // 2026-11-03 JAVA-12), and the captured advertisement survives.
    this.shtp.resetTx();
  }

  /**
   * Read the error queue (command 0x01), filtered to `severity` or greater.
   * Records stream until one with source == 255 (no more).
   */
  async getErrors(severity = 0, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<ErrorRecord[]> {
    const responses = await this.commandCollect(
      Sh2Command.Errors,
      errorsParams(severity),
      (resp, acc) => {
        if (resp.r[2] === ErrorSource.NoMoreErrors) return true; // terminator
        acc.push(resp);
        return false;
      },
      timeoutMs,
    );
    return responses.map(errorRecordFromResponse);
  }

  /**
   * Read a sensor's event counts (command 0x02). The hub answers with two
   * responses (responseSeq 0 then 1).
   */
  async getCounts(sensor: number, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<Counts> {
    const responses = await this.commandCollect(
      Sh2Command.Counter,
      countsGetParams(sensor),
      (resp, acc) => {
        acc.push(resp);
        // both halves, whatever their order: without one the exchange is not
        // complete (a timeout, contract 07 §3 — bug hunt 2026-11-03 PY-6)
        return acc.some((r) => r.responseSeq === 0) && acc.some((r) => r.responseSeq === 1);
      },
      timeoutMs,
    );
    const bySeq = new Map(responses.map((r) => [r.responseSeq, r] as const));
    const r0 = bySeq.get(0)!.r;
    const r1 = bySeq.get(1)!.r;
    const u32 = (r: number[], off: number): number =>
      (r[off]! | (r[off + 1]! << 8) | (r[off + 2]! << 16) | (r[off + 3]! << 24)) >>> 0;
    return {
      sensorId: sensor,
      offered: u32(r0, 3),
      accepted: u32(r0, 7),
      on: u32(r1, 3),
      attempted: u32(r1, 7),
    };
  }

  /** Clear a sensor's event counts (command 0x02, subcommand 1). */
  async clearCounts(sensor: number, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<void> {
    const resp = await this.command(Sh2Command.Counter, countsClearParams(sensor), true, timeoutMs);
    if (resp!.status !== 0) {
      throw new Sh2Error(`clear counts failed: status ${resp!.status}`);
    }
  }
}
