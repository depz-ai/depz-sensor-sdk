/**
 * `.depzdata` datasets: decoded, multi-device, time-synced recording &
 * playback (contracts/09_DATASET_FORMAT.md). Mirrors the Python
 * `depz_sensor_sdk.dataset` module; browser-safe (no Node APIs).
 */

import { type DepzDevice } from "./device/device.js";
import { Bno055 } from "./sensors/bno055/bno055.js";
import { tmf8829RecordValue } from "./sensors/tmf8829/builder.js";
import { Tmf8829 } from "./sensors/tmf8829/tmf8829.js";
import { Bno086 } from "./sensors/bno086/bno086.js";
import type { Report as Bno086Report } from "./sensors/bno086/reports.js";
import { packBno055Units } from "./sensors/bno055/regs.js";
import { Sr04 } from "./sensors/sr04.js";
import { Vl53l4cd } from "./sensors/vl53l4/vl53l4.js";
import { Vl53l8cx, type Vl53l8Frame } from "./sensors/vl53l8/vl53l8.js";
import { Vl53lx } from "./sensors/vl53lx/vl53lx.js";

export const DATASET_SCHEMA = "depz.dataset/2";

/** Report field → record key, for the scalar fields contract 09 `bno086` keeps. */
const BNO086_RECORD_KEYS: [string, string][] = [
  ["i", "i"],
  ["j", "j"],
  ["k", "k"],
  ["real", "real"],
  ["x", "x"],
  ["y", "y"],
  ["z", "z"],
  ["accuracyRad", "accuracy_rad"],
  ["accuracy", "accuracy"],
  ["steps", "steps"],
  ["latencyUs", "latency_us"],
  ["flags", "flags"],
  ["doubleTap", "double_tap"],
  ["motion", "motion"],
  ["classification", "classification"],
  ["mostLikelyState", "most_likely_state"],
  ["value", "value"],
  ["intent", "intent"],
  ["request", "request"],
  ["flushedSensor", "flushed_sensor"],
];

/** The raw ADC streams (0x14..0x16): only these keep their integer counts. */
const BNO086_RAW_IDS = new Set([0x14, 0x15, 0x16]);

/**
 * One BNO086 report as its contract 09 `bno086` record value: snake_case, only
 * the fields the report carries — quaternion / vector components, accuracy,
 * uncalibrated bias, gyro-integrated angular velocity, detector and classifier
 * outputs, and the integer counts of the raw ADC streams. Shared by the SDK
 * recorder and the viewer's, so both write the same record.
 */
/** JSON.stringify that prints a bigint as its exact integer digits. */
function jsonExactBigint(obj: unknown): string {
  const text = JSON.stringify(obj, (_k, x: unknown) => (typeof x === "bigint" ? `@@bigint:${x}@@` : x));
  return text.replace(/"@@bigint:(-?\d+)@@"/g, "$1");
}

export function bno086RecordValue(r: Bno086Report): Record<string, unknown> {
  const f = r as unknown as Record<string, unknown>;
  const v: Record<string, unknown> = { type: r.type, sensor_id: r.sensorId };
  for (const [key, out] of BNO086_RECORD_KEYS) {
    const val = f[key];
    if (typeof val === "number" || typeof val === "boolean") v[out] = val;
  }
  for (const [key, out] of [
    ["bias", "bias"],
    ["angularVelocity", "angular_velocity"],
    ["confidences", "confidences"],
  ] as const) {
    const val = f[key];
    if (Array.isArray(val)) v[out] = [...(val as number[])];
  }
  if (r.type === "GenericEvent") v.value_raw = r.valueRaw;
  if (BNO086_RAW_IDS.has(r.sensorId)) {
    for (const [key, out] of [
      ["xRaw", "x_raw"],
      ["yRaw", "y_raw"],
      ["zRaw", "z_raw"],
      ["temperatureRaw", "temperature_raw"],
      ["sensorTimestampUs", "sensor_timestamp_us"],
    ] as const) {
      if (typeof f[key] === "number") v[out] = f[key];
    }
  }
  // u64 device µs passed through untouched (contract 07): a number while
  // exact, the bigint beyond 2^53 (the writer prints it as an exact integer)
  if (typeof r.timestampUs === "bigint") {
    const n = Number(r.timestampUs);
    v.timestamp_us = Number.isSafeInteger(n) ? n : r.timestampUs;
  }
  return v;
}

export interface DatasetDeviceMeta {
  serial?: string;
  sensor_type?: string;
  software_name?: string;
  time_sync: { offset_us: number; rtt_us: number };
  [k: string]: unknown;
}

export interface DatasetHeader {
  schema: string;
  created_utc: string;
  devices: Record<string, DatasetDeviceMeta>;
  note?: string;
  [k: string]: unknown;
}

export interface DatasetRecord {
  deviceId: string;
  tHostUs: number;
  kind: string;
  value: Record<string, unknown>;
}

/** Accumulates a dataset in memory; `dump()` gives the JSONL file content. */
export class DatasetWriter {
  private readonly header: DatasetHeader;
  private lines: string[];
  recordsWritten = 0;

  constructor(devices: Record<string, DatasetDeviceMeta>, note?: string) {
    this.header = {
      schema: DATASET_SCHEMA,
      created_utc: new Date().toISOString().replace(/\.\d+Z$/, "Z"),
      devices: { ...devices },
    };
    if (note) this.header.note = note;
    this.lines = [];
  }

  /**
   * Register (or replace) a device's metadata after construction. Lets a
   * recorder that adds devices mid-capture keep the header's `devices` map
   * complete — the header is (re)serialized lazily in `dump()`.
   */
  setDeviceMeta(deviceId: string, meta: DatasetDeviceMeta): void {
    this.header.devices[deviceId] = meta;
  }

  write(deviceId: string, tHostUs: number, kind: string, value: Record<string, unknown>): void {
    this.lines.push(jsonExactBigint({ d: deviceId, t: tHostUs, k: kind, v: value }));
    this.recordsWritten += 1;
  }

  dump(): string {
    return [JSON.stringify(this.header), ...this.lines].join("\n") + "\n";
  }
}

/**
 * Hooks live devices into a DatasetWriter on one shared host timeline.
 * Call `await recorder.add(device)` for each device (runs syncTime), then
 * `start()`; `stop()` unhooks; `dump()` returns the file content.
 */
export class DatasetRecorder {
  private entries: Array<{
    id: string;
    device: DepzDevice;
    offsetUs: bigint;
    meta: DatasetDeviceMeta;
    unsub?: () => void;
  }> = [];
  private writer: DatasetWriter | null = null;

  constructor(private opts: { note?: string; vl53l8Layers?: boolean } = {}) {}

  async add(device: DepzDevice, deviceId?: string, syncSamples = 5): Promise<string> {
    if (this.writer) throw new Error("add() all devices before start()");
    const id = deviceId ?? `d${this.entries.length}`;
    const sync = await device.syncTime(syncSamples);
    const meta: DatasetDeviceMeta = {
      time_sync: { offset_us: Number(sync.offsetUs), rtt_us: Number(sync.rttUs) },
    };
    try {
      meta.serial = await device.getSerialNumber();
      const ident = await device.identify();
      meta.software_name = ident.softwareName;
      meta.sensor_type = ident.sensorType ?? "unknown";
    } catch {
      /* identity is best-effort */
    }
    this.entries.push({ id, device, offsetUs: sync.offsetUs, meta });
    return id;
  }

  start(): void {
    if (this.writer) return;
    this.writer = new DatasetWriter(
      Object.fromEntries(this.entries.map((e) => [e.id, e.meta])),
      this.opts.note,
    );
    for (const entry of this.entries) {
      entry.unsub = this.hook(entry);
    }
  }

  private hook(entry: { id: string; device: DepzDevice; offsetUs: bigint }): () => void {
    const writer = this.writer!;
    const dev = entry.device;
    const toHost = (tsUs: bigint) => Number(tsUs - entry.offsetUs);

    // Dispatch on the class, not on method names: every ranging device has an
    // onMeasurement(), and a duck-typed check wrote VL53L4CD / VL53L0X…L4CX
    // measurements as "sr04" records.
    if (dev instanceof Sr04) {
      return dev.onMeasurement((m) => {
        writer.write(entry.id, toHost(m.timestampUs), "sr04", {
          echo_us: m.echoTimeUs,
          source: m.source,
        });
      });
    }
    if (dev instanceof Vl53l4cd) {
      return dev.onMeasurement((m) => {
        writer.write(entry.id, toHost(m.timestampUs), "vl53l4", {
          range_status: m.rangeStatus,
          distance_mm: m.distanceMm,
          sigma_mm: m.sigmaMm,
          signal_rate_kcps: m.signalRateKcps,
          ambient_rate_kcps: m.ambientRateKcps,
          number_of_spad: m.numberOfSpad,
          stream_count: m.streamCount,
        });
      });
    }
    if (dev instanceof Vl53lx) {
      return dev.onMeasurement((m) => {
        const v: Record<string, unknown> = {
          product: dev.product,
          driver: dev.driverKind,
          status: m.status,
          distance_mm: m.distanceMm,
          sigma_mm: m.sigmaMm,
          signal_kcps: m.signalKcps,
          ambient_kcps: m.ambientKcps,
          spads: m.spads,
        };
        if (m.targets.length > 0) {
          v.targets = m.targets.map((t) => ({
            distance_mm: t.distanceMm,
            status: t.status,
            signal_kcps: t.signalKcps,
          }));
        }
        writer.write(entry.id, toHost(m.timestampUs), "vl53lx", v);
      });
    }
    if (dev instanceof Bno055) {
      return dev.onSample((s) => {
        const v: Record<string, unknown> = { unit_sel: packBno055Units(s.units) };
        const channels = {
          quaternion: s.quaternion, euler: s.euler, accel: s.accel, gyro: s.gyro,
          mag: s.mag, linear_accel: s.linearAccel, gravity: s.gravity,
        };
        for (const [key, val] of Object.entries(channels)) if (val !== null) v[key] = [...val];
        if (s.temperature !== null) v.temperature = s.temperature;
        if (s.calibration !== null) {
          const c = s.calibration;
          v.calib = [c.system, c.gyro, c.accel, c.mag];
        }
        writer.write(entry.id, toHost(s.timestampUs), "bno055", v);
      });
    }
    if (dev instanceof Tmf8829) {
      // Contract 09 `tmf8829`: one record per measurement at its capture time.
      return dev.onMeasurement((m) => {
        writer.write(entry.id, toHost(BigInt(m.timestampUs)), "tmf8829", tmf8829RecordValue(m));
      });
    }
    if (dev instanceof Bno086) {
      // Contract 09 `bno086`: every enabled sensor's report, snake_case, full
      // quaternion / vector components, at its measurement time (0xFB/0xFA +
      // delay) like every other sensor. Reports of different sensors in one
      // cargo carry out-of-order device times, so `t` may step back within a
      // device; readers sort.
      return dev.onReport((r) => {
        writer.write(entry.id, toHost(r.timestampUs), "bno086", bno086RecordValue(r));
      });
    }
    // VL53L8CX/CH and the VL53L5CX/L7CX/L7CH subclasses.
    if (dev instanceof Vl53l8cx) {
      const layers = this.opts.vl53l8Layers ?? false;
      return dev.onFrame((f: Vl53l8Frame) => {
        const n = f.resolution;
        const v: Record<string, unknown> = {
          resolution: n,
          silicon_temp_degc: f.siliconTempDegc,
          distance_mm: Array.from(f.distanceMm.subarray(0, n)),
          target_status: Array.from(f.targetStatus.subarray(0, n)),
          nb_target_detected: Array.from(f.nbTargetDetected.subarray(0, n)),
        };
        if (layers) {
          v.signal_per_spad = Array.from(f.signalPerSpad.subarray(0, n));
          v.ambient_per_spad = Array.from(f.ambientPerSpad.subarray(0, n));
          v.range_sigma_mm = Array.from(f.rangeSigmaMm.subarray(0, n));
          v.reflectance = Array.from(f.reflectance.subarray(0, n));
        }
        writer.write(entry.id, toHost(f.timestampUs), "vl53l8", v);
      });
    }
    return () => {};
  }

  stop(): void {
    for (const e of this.entries) {
      e.unsub?.();
      e.unsub = undefined;
    }
  }

  get recordsWritten(): number {
    return this.writer?.recordsWritten ?? 0;
  }

  dump(): string {
    if (!this.writer) throw new Error("recorder never started");
    return this.writer.dump();
  }
}

/** Parse a `.depzdata` file; records come back merged by host time. */
export class DatasetReader {
  readonly header: DatasetHeader;
  readonly records: DatasetRecord[];

  constructor(content: string) {
    const lines = content.split("\n").filter((l) => l.trim().length > 0);
    if (lines.length === 0) throw new Error("empty dataset");
    this.header = JSON.parse(lines[0]!) as DatasetHeader;
    if (!String(this.header.schema ?? "").startsWith("depz.dataset/")) {
      throw new Error("not a depz.dataset file");
    }
    const raw: DatasetRecord[] = lines.slice(1).map((l) => {
      const ev = JSON.parse(l) as { d: string; t: number; k: string; v: Record<string, unknown> };
      return { deviceId: ev.d, tHostUs: ev.t, kind: ev.k, value: ev.v };
    });
    // Per-device order is only roughly monotonic (contract 09); a stable sort
    // gives the one timeline.
    this.records = raw
      .map((r, i) => [r, i] as const)
      .sort((a, b) => a[0].tHostUs - b[0].tHostUs || a[1] - b[1])
      .map(([r]) => r);
  }

  get devices(): Record<string, DatasetDeviceMeta> {
    return this.header.devices ?? {};
  }

  get durationUs(): number {
    if (this.records.length === 0) return 0;
    return this.records[this.records.length - 1]!.tHostUs - this.records[0]!.tHostUs;
  }
}

export type PlayerState = "idle" | "playing" | "paused" | "done";

/** Options of a {@link DatasetPlayer}. */
export interface DatasetPlayerOptions {
  /** Playback speed, finite and ≥ 0; 0 = as fast as possible. Default 1. */
  speed?: number;
  /** Monotonic clock in ms (default `performance.now`); injectable for tests. */
  clock?: () => number;
}

/** Timers resolve to ~1 ms: a record due within this is delivered now. */
const DUE_SLACK_MS = 1;
/** At speed 0 the player yields to the event loop after this much work. */
const BURST_MS = 10;

/**
 * Timeline player: paced delivery with play / pause / seek / speed — the
 * engine behind the viewer's playback mode (contract 09 §Player; reference:
 * Python `DatasetPlayer`).
 *
 * Timing: a media clock anchored to the host clock. Record i is due at
 * `anchorWall + (t_i − anchorT) / speed`; the anchor is (now, t of the last
 * delivered record — of the first record when none was) at play / resume /
 * seek, so the first record goes out at once and every later one after its
 * gap to the previous one. Pacing is against the clock, not cumulative: a
 * slow consumer gets overdue records back to back and playback does not
 * drift.
 *
 * Records and the `done` state are delivered from timers, never inside
 * `play` / `seekUs` / a speed change; state changes made by those calls are
 * reported before they return. An error thrown by an `onRecord` callback
 * pauses the player (the record counts as delivered) and is rethrown from the
 * timer, i.e. it reaches the global uncaught-error handler.
 */
export class DatasetPlayer {
  private idx = 0; // next record to deliver
  private timer: ReturnType<typeof setTimeout> | null = null;
  private stateCbs: Array<(s: PlayerState) => void> = [];
  private recordCbs: Array<(r: DatasetRecord) => void> = [];
  private _state: PlayerState = "idle";
  private _speed: number;
  private anchorWall = 0;
  private anchorT = 0;
  private readonly clock: () => number;

  constructor(
    readonly reader: DatasetReader,
    opts: DatasetPlayerOptions = {},
  ) {
    this._speed = checkSpeed(opts.speed ?? 1);
    this.clock = opts.clock ?? (() => performance.now());
  }

  get state(): PlayerState {
    return this._state;
  }

  /** Span of the timeline: last record's `tHostUs` minus the first's. */
  get durationUs(): number {
    return this.reader.durationUs;
  }

  /**
   * Current position, µs from the first record: the `t` of the last
   * delivered record (after a seek: of the record just before the target).
   * 0 before anything was delivered.
   */
  get positionUs(): number {
    const recs = this.reader.records;
    if (recs.length === 0 || this.idx === 0) return 0;
    return recs[Math.min(this.idx, recs.length) - 1]!.tHostUs - recs[0]!.tHostUs;
  }

  get speed(): number {
    return this._speed;
  }

  /**
   * Change the speed (finite, ≥ 0; 0 = as fast as possible; else RangeError).
   * While playing it applies at once: the rest of the current wait is
   * rescaled, records already delivered stay delivered.
   */
  set speed(value: number) {
    const v = checkSpeed(value);
    if (this._state === "playing") {
      const now = this.clock();
      this.anchorT = this.mediaNow(now);
      this.anchorWall = now;
    }
    this._speed = v;
    if (this._state === "playing") this.kick();
  }

  /** Subscribe to delivered records; returns an idempotent unsubscribe. */
  onRecord(cb: (r: DatasetRecord) => void): () => void {
    this.recordCbs.push(cb);
    return () => {
      const i = this.recordCbs.indexOf(cb);
      if (i >= 0) this.recordCbs.splice(i, 1);
    };
  }

  /** Subscribe to state changes; returns an idempotent unsubscribe. */
  onState(cb: (s: PlayerState) => void): () => void {
    this.stateCbs.push(cb);
    return () => {
      const i = this.stateCbs.indexOf(cb);
      if (i >= 0) this.stateCbs.splice(i, 1);
    };
  }

  /**
   * Start, or resume after `pause`. No-op while playing. From `done` (or
   * after a seek past the end) playback restarts at the first record. An
   * empty dataset goes straight to `done`.
   */
  play(): void {
    if (this._state === "playing") return;
    const recs = this.reader.records;
    if (this.idx >= recs.length) this.idx = 0;
    this.anchor(this.clock());
    this.setState("playing");
    if (recs.length === 0) {
      this.setState("done");
      return;
    }
    this.kick();
  }

  /**
   * Stop delivering; `play` resumes from the next record, which then waits
   * its full gap again. No-op unless playing.
   */
  pause(): void {
    if (this._state !== "playing") return;
    this.clearTimer();
    this.setState("paused");
  }

  /**
   * Move to `offsetUs` µs from the first record: the next record delivered
   * is the first with `t >= t_first + offsetUs` (negative = the start);
   * records in between are skipped. The state does not change, except that
   * a seek past the end while playing ends playback (`done`). A non-number
   * offset is a RangeError; on an empty dataset this is a no-op.
   */
  seekUs(offsetUs: number): void {
    if (typeof offsetUs !== "number" || Number.isNaN(offsetUs)) {
      throw new RangeError(`seekUs: offset must be a number, got ${String(offsetUs)}`);
    }
    const recs = this.reader.records;
    if (recs.length === 0) return;
    const target = recs[0]!.tHostUs + offsetUs;
    let lo = 0;
    let hi = recs.length;
    while (lo < hi) {
      const mid = (lo + hi) >> 1;
      if (recs[mid]!.tHostUs < target) lo = mid + 1;
      else hi = mid;
    }
    this.idx = lo;
    if (this._state !== "playing") return;
    if (this.idx >= recs.length) {
      this.clearTimer();
      this.setState("done");
    } else {
      this.anchor(this.clock());
      this.kick();
    }
  }

  private setState(s: PlayerState): void {
    this._state = s;
    for (const cb of [...this.stateCbs]) cb(s);
  }

  private clearTimer(): void {
    if (this.timer !== null) clearTimeout(this.timer);
    this.timer = null;
  }

  /** (Re)start delivery from a fresh timer turn. */
  private kick(): void {
    this.clearTimer();
    this.timer = setTimeout(() => this.pump(), 0);
  }

  /** The media clock restarts at `now` from the last delivered record's t. */
  private anchor(now: number): void {
    const recs = this.reader.records;
    this.anchorWall = now;
    this.anchorT = recs.length ? recs[Math.max(this.idx - 1, 0)]!.tHostUs : 0;
  }

  /** Media time (µs, host timeline) at `now` while playing. */
  private mediaNow(now: number): number {
    if (this._speed <= 0) {
      const recs = this.reader.records;
      return recs.length ? recs[Math.max(this.idx - 1, 0)]!.tHostUs : 0;
    }
    return this.anchorT + (now - this.anchorWall) * 1000 * this._speed;
  }

  /** Deliver every record that is due, then arm a timer for the next one. */
  private pump(): void {
    this.timer = null;
    const recs = this.reader.records;
    const started = this.clock();
    while (this._state === "playing") {
      if (this.idx >= recs.length) {
        this.setState("done");
        return;
      }
      const rec = recs[this.idx]!;
      const now = this.clock();
      if (this._speed > 0) {
        const due = this.anchorWall + (rec.tHostUs - this.anchorT) / 1000 / this._speed;
        if (due - now > DUE_SLACK_MS) {
          this.timer = setTimeout(() => this.pump(), due - now);
          return;
        }
      }
      if (now - started > BURST_MS) {
        // let the page (and pause / seek) in between records
        this.timer = setTimeout(() => this.pump(), 0);
        return;
      }
      this.idx += 1;
      try {
        for (const cb of [...this.recordCbs]) cb(rec);
      } catch (e) {
        if (this._state === "playing") {
          this.clearTimer();
          this.setState("paused");
        }
        throw e;
      }
      if (this._state === "playing" && this.idx >= recs.length) {
        this.setState("done");
        return;
      }
    }
  }
}

function checkSpeed(value: number): number {
  if (typeof value !== "number" || !Number.isFinite(value) || value < 0) {
    throw new RangeError(`speed must be a finite number >= 0, got ${String(value)}`);
  }
  return value;
}
