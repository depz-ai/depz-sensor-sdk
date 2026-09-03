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

import { DepzDevice, StreamQueue, type DeviceOptions } from "../../device/device.js";
import { BusyError, DepzError, DepzTimeoutError } from "../../errors.js";
import type { PacketEvent } from "../../protocol/framing.js";
import {
  BUSY_BACKOFF_MS,
  Bno086Cmd,
  Bno086Rpt,
  unpackBno086Data,
} from "../../protocol/bno086.js";
import type { SerialTransport } from "../../transport/types.js";
import { SensorId, parseGyroRvCargo, parseInputCargo, type Report } from "./reports.js";
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
  buildGetFeatureRequest,
  buildProductIdRequest,
  buildSetFeature,
  countsClearParams,
  countsGetParams,
  errorRecordFromResponse,
  errorsParams,
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
import { ShtpChannel, ShtpLayer, type ShtpCargo } from "./shtp.js";

// Rate verification bounds (contract 05 §7): the hub grants a grid rate; a
// result outside [0.9, 2.1]× the requested rate is worth a warning, never an
// error.
export const RATE_LOW_FACTOR = 0.9;
export const RATE_HIGH_FACTOR = 2.1;

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

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function normalizeFilter(sensors: Iterable<number> | number | null | undefined): Set<number> | null {
  if (sensors === null || sensors === undefined) return null;
  if (typeof sensors === "number") return new Set([sensors]);
  return new Set(sensors);
}

type ControlWaiter = (payload: Uint8Array) => void;

/**
 * BNO086 device: enable SH-2 sensors, stream typed reports.
 *
 * Typical use:
 *
 *     const imu = new Bno086(transport);
 *     await imu.open();
 *     await imu.enable(SensorId.RotationVector, 100);
 *     for await (const r of imu.reports()) { ... }
 *
 * Callbacks run on the read-pump context — never await blocking device
 * methods (enable/tare/...) from inside one.
 */
export class Bno086 extends DepzDevice {
  /** SEND_SHTP_PACKET attempts before giving up. */
  busyRetries = 5;
  /** >= 200 ms per the bridge spec (contract 05 §2). */
  busyBackoffMs: number = BUSY_BACKOFF_MS;

  private shtp = new ShtpLayer();
  private txChain: Promise<void> = Promise.resolve();
  private reportCbs: Array<{ cb: (r: Report) => void; filter: Set<number> | null }> = [];
  private reportQueues: Array<{ queue: StreamQueue<Report>; filter: Set<number> | null }> = [];
  private controlWaiters = new Map<number, ControlWaiter[]>();
  private cmdSeq = 0;
  private resetResolvers: Array<() => void> = [];
  private advertisementChunks: Uint8Array[] = [];
  private features = new Map<number, FeatureResponse>();

  constructor(transport: SerialTransport, opts?: Bno086Options) {
    super(transport, opts);
    if (opts?.busyRetries !== undefined) this.busyRetries = opts.busyRetries;
    if (opts?.busyBackoffMs !== undefined) this.busyBackoffMs = opts.busyBackoffMs;
  }

  // ── RX path ────────────────────────────────────────────────────────────────

  protected override handleReport(pkt: PacketEvent): boolean {
    if (pkt.cmd !== Bno086Rpt.Data || pkt.payload.length < 9) return false;
    const data = unpackBno086Data(pkt.payload);
    const cargo = this.shtp.feed(data.shtp);
    if (cargo !== null) this.dispatchCargo(cargo, data.timestampUs);
    return true;
  }

  private dispatchCargo(cargo: ShtpCargo, captureUs: bigint): void {
    switch (cargo.channel) {
      case ShtpChannel.Command:
        this.advertisementChunks.push(cargo.payload);
        return;
      case ShtpChannel.Executable:
        if (cargo.payload[0] === EXECUTABLE_RESET_COMPLETE) {
          for (const resolve of this.resetResolvers.splice(0)) resolve();
        }
        return;
      case ShtpChannel.Control: {
        if (cargo.payload.length === 0) return;
        const rid = cargo.payload[0]!;
        if (rid === ControlReport.GetFeatureResponse && cargo.payload.length >= 17) {
          const resp = unpackFeatureResponse(cargo.payload);
          this.features.set(resp.sensorId, resp);
        }
        for (const waiter of [...(this.controlWaiters.get(rid) ?? [])]) {
          waiter(cargo.payload.slice());
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
    for (const { cb, filter } of [...this.reportCbs]) {
      if (filter === null || filter.has(rep.sensorId)) cb(rep);
    }
    for (const { queue, filter } of [...this.reportQueues]) {
      if (filter === null || filter.has(rep.sensorId)) queue.push(rep);
    }
  }

  // ── TX path ────────────────────────────────────────────────────────────────

  /**
   * Frame `payload` and push it through SEND_SHTP_PACKET.
   *
   * ERR_BUSY (both MCU TX slots full) backs off `busyBackoffMs` and
   * retransmits, up to `busyRetries` attempts (contract 05 §2). Sends are
   * serialized (single opcode, single wire).
   */
  private sendShtp(channel: number, payload: Uint8Array): Promise<void> {
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
  private controlExchange<T>(
    rid: number,
    timeoutMs: number,
    handler: (raw: Uint8Array, finish: (value: T) => void, fail: (err: Error) => void) => void,
    send: () => Promise<void>,
  ): Promise<T> {
    return new Promise<T>((resolve, reject) => {
      let settled = false;
      const cleanup = (): void => {
        clearTimeout(timer);
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
      const timer = setTimeout(() => fail(new DepzTimeoutError(rid, timeoutMs)), timeoutMs);
      const waiter: ControlWaiter = (raw) => handler(raw, finish, fail);
      const list = this.controlWaiters.get(rid) ?? [];
      list.push(waiter);
      this.controlWaiters.set(rid, list);
      send().catch((err: unknown) => fail(err instanceof Error ? err : new DepzError(String(err))));
    });
  }

  /** Send a control-channel payload and wait for one matching response. */
  private controlRequest<T>(
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
          obj = parse(raw);
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
          return;
        }
        if (pred === undefined || pred(obj)) finish(obj);
      },
      () => this.sendShtp(ShtpChannel.Control, payload),
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
  private commandCollect(
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
          resp = unpackCommandResponse(raw);
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
          return;
        }
        if (resp.command !== command || resp.commandSeq !== seq) return;
        if (collect(resp, results)) finish(results);
      },
      () => this.sendShtp(ShtpChannel.Control, payload),
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
   * (≤ v0.95) did **not** emit it (ERRATA E9 in contracts/ERRATA.md, now fixed
   * in newer firmware); the best-effort wait handles both, so its absence is
   * *not* an error — the sensor is fully usable without it.
   */
  async hardwareReset(timeoutMs = 2000): Promise<void> {
    this.shtp.reset();
    this.advertisementChunks = [];
    this.features.clear();
    const done = new Promise<void>((resolve) => this.resetResolvers.push(resolve));
    // RPT_STATUS OK from the hub confirms the reset command was accepted.
    await this.request(Bno086Cmd.SensorReset, undefined, { okCompletes: true, timeoutMs });
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
    }
  }

  /** Pulse WAKE (PS0): wakes the sensor from sleep, no state loss. */
  async wake(): Promise<void> {
    await this.request(Bno086Cmd.SensorWakeUp, undefined, { okCompletes: true });
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

  /** Product ID Request/Response round trip (first responding subsystem). */
  productId(timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<ProductId> {
    return this.controlRequest(
      buildProductIdRequest(),
      ControlReport.ProductIdResponse,
      unpackProductId,
      undefined,
      timeoutMs,
    );
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
    const resp = await this.getFeature(sensor, opts.timeoutMs ?? CONTROL_TIMEOUT_MS);
    const requestedRate = 1_000_000 / intervalUs;
    const actualRate = resp.intervalUs !== 0 ? 1_000_000 / resp.intervalUs : 0;
    if (
      !(RATE_LOW_FACTOR * requestedRate <= actualRate && actualRate <= RATE_HIGH_FACTOR * requestedRate)
    ) {
      console.warn(
        `BNO086 sensor 0x${sensor.toString(16).toUpperCase().padStart(2, "0")}: requested ` +
          `${requestedRate.toFixed(1)} Hz, granted ${actualRate.toFixed(1)} Hz (outside ` +
          `${RATE_LOW_FACTOR}–${RATE_HIGH_FACTOR}× band)`,
      );
    }
    return resp;
  }

  /** Disable `sensor` (Set Feature with interval 0). */
  async disable(sensor: number): Promise<void> {
    await this.sendShtp(ShtpChannel.Control, buildSetFeature(sensor, 0));
    this.features.delete(sensor);
  }

  /** Get Feature Request/Response round trip for `sensor`. */
  getFeature(sensor: number, timeoutMs: number = CONTROL_TIMEOUT_MS): Promise<FeatureResponse> {
    return this.controlRequest(
      buildGetFeatureRequest(sensor),
      ControlReport.GetFeatureResponse,
      unpackFeatureResponse,
      (r) => r.sensorId === sensor,
      timeoutMs,
    );
  }

  // sugar for the everyday sensors
  enableRotationVector(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.RotationVector, hz, opts);
  }

  enableGameRotationVector(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.GameRotationVector, hz, opts);
  }

  enableAccelerometer(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.Accelerometer, hz, opts);
  }

  enableGyroscope(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.Gyroscope, hz, opts);
  }

  enableMagnetometer(hz = 50, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.Magnetometer, hz, opts);
  }

  enableLinearAcceleration(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.LinearAcceleration, hz, opts);
  }

  enableGravity(hz = 100, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.Gravity, hz, opts);
  }

  enableGyroIntegratedRv(hz = 400, opts?: EnableOptions): Promise<FeatureResponse | null> {
    return this.enable(SensorId.GyroIntegratedRv, hz, opts);
  }

  // ── report streaming ───────────────────────────────────────────────────────

  /**
   * Subscribe to typed sensor reports (read-pump context; do not block).
   * `sensors` filters by SensorId. Returns an unsubscribe fn.
   */
  onReport(cb: (r: Report) => void, sensors?: Iterable<number> | number | null): () => void {
    const entry = { cb, filter: normalizeFilter(sensors) };
    this.reportCbs.push(entry);
    return () => {
      this.reportCbs = this.reportCbs.filter((e) => e !== entry);
    };
  }

  /**
   * Async iterator over typed reports — bounded, drop-oldest (contract 07
   * §3). Subscribes eagerly — reports emitted after this call are never
   * missed.
   */
  reports(sensors?: Iterable<number> | number | null, maxsize = 1024): StreamQueue<Report> {
    const queue: StreamQueue<Report> = new StreamQueue(maxsize, () => {
      this.reportQueues = this.reportQueues.filter((e) => e.queue !== queue);
      deregister();
    });
    this.reportQueues.push({ queue, filter: normalizeFilter(sensors) });
    const deregister = this.registerStream(queue);
    return queue;
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

  // ── FRS ────────────────────────────────────────────────────────────────────

  /** Read a whole FRS record; resolves with its 32-bit words. */
  frsRead(recordId: number, timeoutMs: number = FRS_TIMEOUT_MS): Promise<number[]> {
    const session = new FrsReadSession(recordId);
    return this.controlExchange<number[]>(
      ControlReport.FrsReadResponse,
      timeoutMs,
      (raw, finish, fail) => {
        try {
          if (session.feed(unpackFrsReadResponse(raw))) finish(session.words);
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
        }
      },
      () => this.sendShtp(ShtpChannel.Control, session.request()),
    );
  }

  /** Write a whole FRS record (word list); rejects with Sh2Error on failure. */
  frsWrite(recordId: number, words: Iterable<number>, timeoutMs: number = FRS_TIMEOUT_MS): Promise<void> {
    const session = new FrsWriteSession(recordId, [...words]);
    return this.controlExchange<void>(
      ControlReport.FrsWriteResponse,
      timeoutMs,
      (raw, finish, fail) => {
        try {
          const next = session.feed(unpackFrsWriteResponse(raw));
          if (session.done) {
            finish(undefined);
          } else if (next !== null) {
            this.sendShtp(ShtpChannel.Control, next).catch(fail);
          }
        } catch (err) {
          fail(err instanceof Error ? err : new DepzError(String(err)));
        }
      },
      () => this.sendShtp(ShtpChannel.Control, session.request()),
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
   * executable reset-complete like hardwareReset().
   */
  async clearDcdAndReset(timeoutMs = 2000): Promise<void> {
    this.shtp.reset();
    this.advertisementChunks = [];
    this.features.clear();
    const done = new Promise<void>((resolve) => this.resetResolvers.push(resolve));
    await this.command(Sh2Command.ClearDcdAndReset, new Uint8Array(0), false);
    let timer: ReturnType<typeof setTimeout> | undefined;
    const timedOut = new Promise<never>((_, reject) => {
      timer = setTimeout(
        () => reject(new DepzTimeoutError(Sh2Command.ClearDcdAndReset, timeoutMs)),
        timeoutMs,
      );
    });
    try {
      await Promise.race([done, timedOut]);
    } finally {
      clearTimeout(timer);
    }
    // The device restarted its counters on reset; realign the SHTP layer (the
    // captured advertisement survives — it lives outside the SHTP layer).
    this.shtp.reset();
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
        return acc.some((r) => r.responseSeq === 1);
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
