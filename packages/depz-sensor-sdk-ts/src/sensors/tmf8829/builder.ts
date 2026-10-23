/**
 * Frames in stream order → measurements (contracts/14_SENSOR_TMF8829.md §6).
 * Mirrors the Python reference `depz_sensor_sdk.tmf8829.builder`.
 */

import { checkU64Number } from "../../protocol/wire.js";
import { Tmf8829CaptureTimer, Tmf8829SensorClock, type Tmf8829FrameTime } from "./clock.js";
import {
  TMF8829_FID_HISTOGRAMS,
  TMF8829_FID_REF_SPAD,
  TMF8829_FID_RESULTS,
  TMF8829_FP_GRID,
  TMF8829_LAYOUT_SUBFRAME,
  mergeTmf8829Halves,
  parseTmf8829Frame,
  parseTmf8829Results,
  tmf8829FrameProblems,
  type Tmf8829FrameInfo,
  type Tmf8829HistogramFrame,
  type Tmf8829PixelResults,
} from "./frames.js";

/**
 * One measurement: its results frame (8x8, 16x16) or both half-frames (32x32,
 * 48x32) merged, with the histogram / ref-SPAD frames before it.
 */
export interface Tmf8829Measurement {
  /** Capture time, MCU µs (floor mean of the half-frame captures). */
  timestampUs: number;
  /** TMF8829_FT_* of all its results frames. */
  flags: number;
  fpMode: number;
  fNumber: number;
  rows: number;
  cols: number;
  /** Raw sensor integers. */
  results: Tmf8829PixelResults;
  /** Clock-corrected mm, flat [peak][row][col] (null without peaks). */
  distanceMm: Float64Array | null;
  /** Clock ratio the distances were corrected with (Q16). */
  ratioQ16: number;
  frames: Tmf8829FrameInfo[];
  times: Tmf8829FrameTime[];
  histograms: Tmf8829HistogramFrame[];
  /** 65 B each, zero pre-header. */
  refFrames: Uint8Array[];
  /** What went wrong since the previous measurement, human-readable. */
  problems: string[];
  /** Sensor die temperature (header byte 0 of the last frame), °C. */
  temperatureC: number;
}

/** Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units). */
export function tmf8829DistanceMm(
  results: Tmf8829PixelResults,
  ratio: number,
  inMm: boolean,
): Float64Array | null {
  if (results.distance === null) return null;
  const out = new Float64Array(results.distance.length);
  for (let i = 0; i < out.length; i++) {
    const d = results.distance[i]! * ratio;
    out[i] = inMm ? d : d / 4;
  }
  return out;
}

/**
 * Histogram frames between two results frames belong to the second; a
 * ref-SPAD frame comes right before its results frame. Call `reset()` at
 * every stream start: the clock model and T1 are per stream.
 */
export class Tmf8829MeasurementBuilder {
  readonly clock = new Tmf8829SensorClock();
  readonly timer = new Tmf8829CaptureTimer(this.clock);
  parseErrors = 0;
  private pending: Array<[Tmf8829FrameInfo, Uint8Array]> = [];
  private hist: Tmf8829HistogramFrame[] = [];
  private refs: Uint8Array[] = [];
  private half: [Tmf8829FrameInfo, Tmf8829PixelResults, Tmf8829FrameTime] | null = null;
  private problems: string[] = [];

  reset(): void {
    this.clock.reset();
    this.timer.reset();
    this.pending = [];
    this.hist = [];
    this.refs = [];
    this.half = null;
    this.problems = [];
    this.parseErrors = 0;
  }

  /**
   * One frame with the MCU time of its header read and its INT edge (0 =
   * none) → the measurement it completes, or null. `raw` is copied.
   */
  add(rawIn: Uint8Array, timestampUs: number, edgeUs = 0): Tmf8829Measurement | null {
    // contract 07 §3: MCU µs times are u64 — checked before the frame is parsed
    checkU64Number("timestampUs", timestampUs);
    checkU64Number("edgeUs", edgeUs);
    const raw = rawIn.slice();
    let f: Tmf8829FrameInfo;
    try {
      f = parseTmf8829Frame(raw);
    } catch (e) {
      this.parseErrors += 1;
      this.problems.push(e instanceof Error ? e.message : String(e));
      return null;
    }
    if (f.kind === TMF8829_FID_REF_SPAD) {
      this.pending.push([f, raw]); // zero pre-header: no clock pair
      return null;
    }
    if (f.kind !== TMF8829_FID_RESULTS && f.kind !== TMF8829_FID_HISTOGRAMS) {
      this.parseErrors += 1;
      this.problems.push(`frame id 0x${f.id.toString(16).toUpperCase().padStart(2, "0")}: unknown kind`);
      return null;
    }
    const flags = this.clock.add(timestampUs, f.systick);
    const bad = tmf8829FrameProblems(f);
    if (bad.length) {
      this.problems.push(
        `frame id=0x${f.id.toString(16).toUpperCase().padStart(2, "0")} fN=${f.fNumber} ${f.size} B: ${bad.join(", ")}`,
      );
    }
    if (f.kind === TMF8829_FID_HISTOGRAMS) {
      this.timer.histograms();
      this.pending.push([f, raw]);
      return null;
    }

    const t = this.timer.results(f.fpMode, f.systick, f.t0Integration, f.t1Integration, timestampUs, edgeUs, flags);
    for (const [pf, praw] of this.pending) {
      if (pf.kind === TMF8829_FID_HISTOGRAMS) this.hist.push({ info: pf, raw: praw });
      else this.refs.push(praw);
    }
    this.pending = [];
    let res: Tmf8829PixelResults;
    try {
      res = parseTmf8829Results(raw, f);
    } catch (e) {
      this.parseErrors += 1;
      this.problems.push(`fN=${f.fNumber}: ${e instanceof Error ? e.message : String(e)}`);
      this.half = null;
      return null;
    }

    let frames: Tmf8829FrameInfo[];
    let times: Tmf8829FrameTime[];
    if ((TMF8829_FP_GRID[f.fpMode]?.[0] ?? 0) === 32) {
      if (this.half === null) {
        if (f.isSecondHalf) {
          this.problems.push(`second half fN=${f.fNumber} without a first one - dropped`);
          this.hist = [];
          this.refs = [];
          return null;
        }
        this.half = [f, res, t];
        return null;
      }
      if (!f.isSecondHalf) {
        this.problems.push(`two first halves in a row (fN ${this.half[0].fNumber}, ${f.fNumber})`);
        this.half = [f, res, t];
        return null;
      }
      const [f0, res0, t0] = this.half;
      this.half = null;
      const fmt = (x: Tmf8829FrameInfo): number => (x.fpMode << 8) | (x.layout & ~TMF8829_LAYOUT_SUBFRAME);
      if (fmt(f0) !== fmt(f)) {
        // Impossible within one stream (configure() needs a stop); never crash the read pump.
        this.problems.push(`half-frames of different formats (fN ${f0.fNumber}, ${f.fNumber}) - dropped`);
        this.hist = [];
        this.refs = [];
        return null;
      }
      frames = [f0, f];
      times = [t0, t];
      res = mergeTmf8829Halves(res0, res);
    } else {
      frames = [f];
      times = [t];
    }

    const m: Tmf8829Measurement = {
      timestampUs: Math.floor(times.reduce((s, x) => s + x.timestampUs, 0) / times.length),
      flags: times.reduce((s, x) => s | x.flags, 0),
      fpMode: f.fpMode,
      fNumber: f.fNumber,
      rows: res.rows,
      cols: res.cols,
      results: res,
      distanceMm: tmf8829DistanceMm(res, this.clock.ratio, f.distanceInMm),
      ratioQ16: this.clock.ratioQ16,
      frames,
      times,
      histograms: this.hist,
      refFrames: this.refs,
      problems: this.problems,
      temperatureC: f.temperature[0],
    };
    this.hist = [];
    this.refs = [];
    this.problems = [];
    return m;
  }
}

/** Contract 09 `tmf8829` record value of a measurement (distances rounded to 0.1 mm). */
export function tmf8829RecordValue(m: Tmf8829Measurement): Record<string, unknown> {
  const r = m.results;
  const v: Record<string, unknown> = {
    fp_mode: m.fpMode,
    rows: m.rows,
    cols: m.cols,
    peaks: r.peaks,
    f_number: m.fNumber,
    flags: m.flags,
    temperature_c: m.temperatureC,
    distance_mm: m.distanceMm ? Array.from(m.distanceMm, (x) => Math.round(x * 10) / 10) : [],
    snr: r.snr ? Array.from(r.snr) : [],
  };
  if (r.signal) v.signal = Array.from(r.signal);
  if (r.noise) v.noise = Array.from(r.noise);
  if (r.xtalk) v.xtalk = Array.from(r.xtalk);
  return v;
}
