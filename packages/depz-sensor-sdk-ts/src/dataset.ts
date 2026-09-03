/**
 * `.depzdata` datasets: decoded, multi-device, time-synced recording &
 * playback (contracts/09_DATASET_FORMAT.md). Mirrors the Python
 * `depz_sensor_sdk.dataset` module; browser-safe (no Node APIs).
 */

import type { DepzDevice } from "./device/device.js";
import type { Sr04, Sr04Measurement } from "./sensors/sr04.js";
import type { Vl53l8, Vl53l8Frame } from "./sensors/vl53l8/vl53l8.js";

export const DATASET_SCHEMA = "depz.dataset/1";

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
    this.lines.push(JSON.stringify({ d: deviceId, t: tHostUs, k: kind, v: value }));
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
    const dev = entry.device as DepzDevice & Partial<Sr04> & Partial<Vl53l8>;
    const toHost = (tsUs: bigint) => Number(tsUs - entry.offsetUs);

    if (typeof dev.onMeasurement === "function") {
      return dev.onMeasurement!((m: Sr04Measurement) => {
        writer.write(entry.id, toHost(m.timestampUs), "sr04", {
          echo_us: m.echoTimeUs,
          source: m.source,
        });
      });
    }
    if (typeof dev.onFrame === "function") {
      const layers = this.opts.vl53l8Layers ?? false;
      return dev.onFrame!((f: Vl53l8Frame) => {
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
    // Per-device order is monotonic; a stable sort merges exactly.
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

/**
 * Timeline player: paced delivery with play/pause/seek/speed — the engine
 * behind the viewer's playback mode.
 */
export class DatasetPlayer {
  private idx = 0;
  private timer: ReturnType<typeof setTimeout> | null = null;
  private stateCbs: Array<(s: PlayerState) => void> = [];
  private recordCbs: Array<(r: DatasetRecord) => void> = [];
  private _state: PlayerState = "idle";
  speed = 1.0;

  constructor(readonly reader: DatasetReader) {}

  get state(): PlayerState {
    return this._state;
  }

  /** Current position in µs from the first record. */
  get positionUs(): number {
    const recs = this.reader.records;
    if (recs.length === 0 || this.idx === 0) return 0;
    return recs[Math.min(this.idx, recs.length) - 1]!.tHostUs - recs[0]!.tHostUs;
  }

  onRecord(cb: (r: DatasetRecord) => void): () => void {
    this.recordCbs.push(cb);
    return () => {
      this.recordCbs = this.recordCbs.filter((c) => c !== cb);
    };
  }

  onState(cb: (s: PlayerState) => void): () => void {
    this.stateCbs.push(cb);
    return () => {
      this.stateCbs = this.stateCbs.filter((c) => c !== cb);
    };
  }

  private setState(s: PlayerState): void {
    this._state = s;
    this.stateCbs.forEach((cb) => cb(s));
  }

  play(): void {
    if (this._state === "playing") return;
    if (this.idx >= this.reader.records.length) this.idx = 0;
    this.setState("playing");
    this.scheduleNext();
  }

  pause(): void {
    if (this.timer) clearTimeout(this.timer);
    this.timer = null;
    if (this._state === "playing") this.setState("paused");
  }

  /** Seek to µs offset from the start; delivery resumes from there. */
  seekUs(offsetUs: number): void {
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
    if (this._state === "playing") {
      this.pause();
      this.play();
    }
  }

  private scheduleNext(): void {
    const recs = this.reader.records;
    if (this.idx >= recs.length) {
      this.setState("done");
      return;
    }
    const rec = recs[this.idx]!;
    const emit = () => {
      this.idx += 1;
      this.recordCbs.forEach((cb) => cb(rec));
      if (this._state === "playing") this.scheduleNext();
    };
    if (this.speed <= 0 || this.idx === 0) {
      emit();
      return;
    }
    const prev = recs[this.idx - 1]!;
    const gapMs = (rec.tHostUs - prev.tHostUs) / 1000 / this.speed;
    if (gapMs <= 1) {
      emit();
    } else {
      this.timer = setTimeout(emit, gapMs);
    }
  }
}
