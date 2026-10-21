/**
 * TMF8829 frame layout and pixel decoding (contracts/14_SENSOR_TMF8829.md §3).
 * Mirrors the Python reference `depz_sensor_sdk.tmf8829.frames`.
 */

import { DepzError } from "../../errors.js";

export const TMF8829_PRE_HEADER_SIZE = 5;
export const TMF8829_HEADER_SIZE = 16;
export const TMF8829_FOOTER_SIZE = 12;
/** The 21 bytes the bridge reads at 0xFA. */
export const TMF8829_FRAME_HEAD = TMF8829_PRE_HEADER_SIZE + TMF8829_HEADER_SIZE;
export const TMF8829_FRAME_EOF = 0xe0f7;

export const TMF8829_FID_MASK = 0xf0;
export const TMF8829_FID_RESULTS = 0x10;
export const TMF8829_FID_HISTOGRAMS = 0x20;
export const TMF8829_FID_REF_SPAD = 0x30;

export const TMF8829_LAYOUT_PEAKS = 0x07;
export const TMF8829_LAYOUT_SIGNAL = 0x08;
export const TMF8829_LAYOUT_NOISE = 0x10;
export const TMF8829_LAYOUT_XTALK = 0x20;
/** 32x32/48x32: set on the second (odd-row) half-frame. */
export const TMF8829_LAYOUT_SUBFRAME = 0x40;

const FRAME_VALID = 0x01;
const FRAME_ABORTED = 0xc0;
const FRAME_WARNINGS: Array<[number, string]> = [
  [0x08, "HV_CP_OVERLOAD"],
  [0x10, "VCDRV_OVERLOAD"],
  [0x20, "VCDRV_BURST_EXCEEDED"],
];
const FOOTER_DIST_IN_MM = 0x01;
const FIFO_TX_UNDERRUN = 0x10;
const FIFO_RX_OVERRUN = 0x20;

/** fp_mode → [rows, cols] of the full focal plane. */
export const TMF8829_FP_GRID: Readonly<Record<number, readonly [number, number]>> = {
  0: [8, 8],
  1: [8, 8],
  2: [16, 16],
  3: [32, 32],
  4: [32, 32],
  5: [32, 48],
};
export const TMF8829_FP_NAMES: Readonly<Record<number, string>> = {
  0: "8x8A",
  1: "8x8B",
  2: "16x16",
  3: "32x32",
  4: "32x32s",
  5: "48x32",
};

/** Pre-header, header and footer of one frame (any kind). */
export interface Tmf8829FrameInfo {
  fifoStatus: number;
  systick: number;
  id: number;
  /** TMF8829_FID_RESULTS / _HISTOGRAMS / _REF_SPAD */
  kind: number;
  fpMode: number;
  layout: number;
  payload: number;
  fNumber: number;
  temperature: [number, number, number];
  bdv: number;
  refPos: [number, number];
  t0Integration: number;
  t1Integration: number;
  frameStatus: number;
  reserved: number;
  eof: number;
  size: number;
  isSecondHalf: boolean;
  distanceInMm: boolean;
}

/** Full frame size (pre-header included) from its first 21 bytes. */
export function tmf8829FrameSize(head: Uint8Array): number {
  return TMF8829_PRE_HEADER_SIZE + 4 + (head[7]! | (head[8]! << 8));
}

export function parseTmf8829Frame(raw: Uint8Array): Tmf8829FrameInfo {
  if (raw.length < TMF8829_FRAME_HEAD + TMF8829_FOOTER_SIZE) {
    throw new DepzError(`frame of ${raw.length} B is shorter than header + footer`);
  }
  const v = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  const h = TMF8829_PRE_HEADER_SIZE;
  const f = raw.length - TMF8829_FOOTER_SIZE;
  const id = raw[h]!;
  const layout = raw[h + 1]!;
  const reserved = raw[f + 9]!;
  return {
    fifoStatus: raw[0]!,
    systick: v.getUint32(1, true),
    id,
    kind: id & TMF8829_FID_MASK,
    fpMode: id & 0x0f,
    layout,
    payload: v.getUint16(h + 2, true),
    fNumber: v.getUint32(h + 4, true),
    temperature: [v.getInt8(h + 8), v.getInt8(h + 9), v.getInt8(h + 10)],
    bdv: raw[h + 11]!,
    refPos: [v.getUint16(h + 12, true), v.getUint16(h + 14, true)],
    t0Integration: v.getUint32(f, true),
    t1Integration: v.getUint32(f + 4, true),
    frameStatus: raw[f + 8]!,
    reserved,
    eof: v.getUint16(f + 10, true),
    size: raw.length,
    isSecondHalf: (layout & TMF8829_LAYOUT_SUBFRAME) !== 0,
    distanceInMm: (reserved & FOOTER_DIST_IN_MM) !== 0,
  };
}

/** What is wrong with a frame, human-readable; empty when fine. */
export function tmf8829FrameProblems(f: Tmf8829FrameInfo): string[] {
  const out: string[] = [];
  if (f.eof !== TMF8829_FRAME_EOF) out.push(`EOF 0x${f.eof.toString(16).toUpperCase().padStart(4, "0")}`);
  if (!(f.frameStatus & FRAME_VALID)) {
    out.push(`status 0x${f.frameStatus.toString(16).toUpperCase().padStart(2, "0")} not valid`);
  }
  if (f.frameStatus & FRAME_ABORTED) out.push("aborted");
  for (const [bit, name] of FRAME_WARNINGS) if (f.frameStatus & bit) out.push(name);
  if (f.fifoStatus & FIFO_TX_UNDERRUN) out.push("txfifo_underrun");
  if (f.fifoStatus & FIFO_RX_OVERRUN) out.push("rxfifo_overrun");
  return out;
}

/**
 * Decoded results of one frame or of a merged measurement. Per-peak arrays
 * are flat `[peak][row][col]` (row-major); noise / xtalk `[row][col]`. Raw
 * sensor integers: distance in 0.25 mm (or mm with the footer flag), not yet
 * clock-corrected. Absent fields are null.
 */
export interface Tmf8829PixelResults {
  peaks: number;
  rows: number;
  cols: number;
  distance: Uint16Array | null;
  snr: Uint8Array | null;
  signal: Uint16Array | null;
  noise: Uint16Array | null;
  xtalk: Uint16Array | null;
}

/** [rows, cols] one results frame carries: half the rows for 32x32/48x32. */
export function tmf8829FrameRows(fpMode: number): [number, number] {
  const g = TMF8829_FP_GRID[fpMode];
  if (!g) throw new DepzError(`unknown fp_mode ${fpMode}`);
  return g[0] === 32 ? [g[0] / 2, g[1]] : [g[0], g[1]];
}

/** Pixel results of one results frame. */
export function parseTmf8829Results(raw: Uint8Array, info?: Tmf8829FrameInfo): Tmf8829PixelResults {
  const f = info ?? parseTmf8829Frame(raw);
  if (f.kind !== TMF8829_FID_RESULTS) {
    throw new DepzError(`frame id 0x${f.id.toString(16)} is not a results frame`);
  }
  const [rows, cols] = tmf8829FrameRows(f.fpMode);
  const peaks = f.layout & TMF8829_LAYOUT_PEAKS;
  const noise = (f.layout & TMF8829_LAYOUT_NOISE) !== 0;
  const xtalk = (f.layout & TMF8829_LAYOUT_XTALK) !== 0;
  const signal = (f.layout & TMF8829_LAYOUT_SIGNAL) !== 0;
  const peakSize = 3 + (signal ? 2 : 0);
  const pxSize = (noise ? 2 : 0) + (xtalk ? 2 : 0) + peaks * peakSize;
  const n = rows * cols;
  const dataSize = f.size - TMF8829_FRAME_HEAD - TMF8829_FOOTER_SIZE;
  if (pxSize * n !== dataSize) {
    throw new DepzError(
      `result data ${dataSize} B, expected ${n} px × ${pxSize} B (layout 0x${f.layout.toString(16)})`,
    );
  }
  const out: Tmf8829PixelResults = {
    peaks,
    rows,
    cols,
    distance: peaks ? new Uint16Array(peaks * n) : null,
    snr: peaks ? new Uint8Array(peaks * n) : null,
    signal: peaks && signal ? new Uint16Array(peaks * n) : null,
    noise: noise ? new Uint16Array(n) : null,
    xtalk: xtalk ? new Uint16Array(n) : null,
  };
  let o = TMF8829_FRAME_HEAD;
  const u16 = (at: number): number => raw[at]! | (raw[at + 1]! << 8);
  for (let i = 0; i < n; i++) {
    if (out.noise) {
      out.noise[i] = u16(o);
      o += 2;
    }
    if (out.xtalk) {
      out.xtalk[i] = u16(o);
      o += 2;
    }
    for (let p = 0; p < peaks; p++) {
      out.distance![p * n + i] = u16(o);
      out.snr![p * n + i] = raw[o + 2]!;
      o += 3;
      if (out.signal) {
        out.signal[p * n + i] = u16(o);
        o += 2;
      }
    }
  }
  return out;
}

/** 32x32/48x32: the first half-frame holds the even rows, the second the odd ones. */
export function mergeTmf8829Halves(
  first: Tmf8829PixelResults,
  second: Tmf8829PixelResults,
): Tmf8829PixelResults {
  const rows = first.rows * 2;
  const cols = first.cols;
  function merge<T extends Uint8Array | Uint16Array>(a: T | null, b: T | null, layers: number): T | null {
    if (a === null || b === null) return null;
    const m = new (a.constructor as new (n: number) => T)(layers * rows * cols);
    for (let l = 0; l < layers; l++) {
      for (let r = 0; r < first.rows; r++) {
        const src = (l * first.rows + r) * cols;
        m.set(a.subarray(src, src + cols), (l * rows + 2 * r) * cols);
        m.set(b.subarray(src, src + cols), (l * rows + 2 * r + 1) * cols);
      }
    }
    return m;
  }
  return {
    peaks: first.peaks,
    rows,
    cols,
    distance: merge(first.distance, second.distance, first.peaks),
    snr: merge(first.snr, second.snr, first.peaks),
    signal: merge(first.signal, second.signal, first.peaks),
    noise: merge(first.noise, second.noise, 1),
    xtalk: merge(first.xtalk, second.xtalk, 1),
  };
}

/** A raw histogram frame (25 377 B); decoding the bins is left to the caller. */
export interface Tmf8829HistogramFrame {
  info: Tmf8829FrameInfo;
  raw: Uint8Array;
}
