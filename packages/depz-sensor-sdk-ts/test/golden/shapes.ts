/**
 * SDK results -> golden-vector shapes, shared by the vector tests and the
 * differential-testing adapter (test/difftest-adapter.ts, contracts/16_DIFFTEST.md).
 *
 * Each helper turns what the public API returns into exactly the JSON shape
 * the matching vector group writes under `expect`. Wide integers stay
 * `bigint` (exact); tests compare through `plain()`, which turns them into
 * numbers like the vector JSON holds.
 */

import { createHash } from "node:crypto";
import { PacketParser, type ParserEvent, type Report, type ShtpCargo } from "../../src/index.js";
import { unpackBno055Info, unpackBno055RegData, unpackBno055Stream } from "../../src/protocol/bno055.js";
import {
  Tmf8829FrameReassembler,
  Tmf8829Rpt,
  unpackTmf8829Event,
  unpackTmf8829FrameChunk,
  unpackTmf8829Info,
  unpackTmf8829RegData,
} from "../../src/protocol/tmf8829.js";
import { unpackInfo, unpackRegData, unpackStream } from "../../src/protocol/vl53l4.js";
import { unpackVl53l7Info } from "../../src/protocol/vl53l7.js";
import { FrameReassembler, Vl53l8Rpt, unpackFrameChunk } from "../../src/protocol/vl53l8.js";
import { unpackVl53lxInfo } from "../../src/protocol/vl53lx.js";
import { decodeBno055Block } from "../../src/sensors/bno055/regs.js";
import { Tmf8829MeasurementBuilder } from "../../src/sensors/tmf8829/builder.js";
import {
  TMF8829_FID_RESULTS,
  parseTmf8829Frame,
  parseTmf8829Results,
  tmf8829FrameProblems,
  type Tmf8829PixelResults,
} from "../../src/sensors/tmf8829/frames.js";
import { parseResultBlock } from "../../src/sensors/vl53l4/uld.js";
import { decodeL0xRaw } from "../../src/sensors/vl53lx/decode.js";
import { toHex } from "./vectors.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;

/** bigint -> number, recursively (what the vector JSON holds). */
export function plain(v: unknown): Any {
  if (typeof v === "bigint") return Number(v);
  if (Array.isArray(v)) return v.map(plain);
  if (v !== null && typeof v === "object" && !(v instanceof Uint8Array)) {
    return Object.fromEntries(Object.entries(v).map(([k, x]) => [k, plain(x)]));
  }
  return v;
}

/** camelCase -> snake_case, digits as their own word (signalRateMcps1616 -> signal_rate_mcps_1616). */
export function snake(o: Record<string, unknown>): Record<string, unknown> {
  const out: Record<string, unknown> = {};
  for (const [k, val] of Object.entries(o)) {
    out[k.replace(/[A-Z]/g, (c) => "_" + c.toLowerCase()).replace(/(\d+)/g, "_$1")] = val;
  }
  return out;
}

// ── framing (contract 01 §5) ─────────────────────────────────────────────────

export interface Collected {
  events: Array<Record<string, unknown>>;
  trash: string;
  residue: string;
  header_errors: number;
}

/** Feed `chunks` in order; the framing_decode.json `expect` shape. */
export function framingCollect(parser: PacketParser, chunks: Uint8Array[]): Collected {
  const events: Array<Record<string, unknown>> = [];
  const trash: number[] = [];
  for (const chunk of chunks) {
    for (const ev of parser.feed(chunk) as ParserEvent[]) {
      if (ev.type === "packet") {
        events.push({ type: "packet", cmd: ev.cmd, seq: ev.seq, payload: toHex(ev.payload) });
      } else if (ev.type === "crcError") {
        events.push({ type: "crc_error", cmd: ev.cmd, seq: ev.seq });
      } else {
        trash.push(...ev.data);
      }
    }
  }
  return {
    events,
    trash: toHex(Uint8Array.from(trash)),
    residue: toHex(parser.residue),
    header_errors: parser.headerErrors,
  };
}

// ── BNO055 (contract 13) ─────────────────────────────────────────────────────

export function bno055Decode(report: number, raw: Uint8Array): Any {
  if (report === 0x92) {
    const i = unpackBno055Info(raw);
    return {
      i2c_addr: i.i2cAddr, chip_id: i.chipId, acc_id: i.accId, mag_id: i.magId, gyr_id: i.gyrId,
      sw_rev: i.swRev, bl_rev: i.blRev, initialized: i.initialized, int_level: i.intLevel,
      int_edges: i.intEdges, read_min_us: i.readMinUs, read_max_us: i.readMaxUs,
      read_avg_us: i.readAvgUs, tx_dropped: i.txDropped, i2c_errors: i.i2cErrors,
      slots_skipped: i.slotsSkipped, bus_recoveries: i.busRecoveries,
      last_i2c_error: i.lastI2cError, sensor_resets: i.sensorResets, loop_max_us: i.loopMaxUs,
    };
  }
  if (report === 0x91) {
    const r = unpackBno055RegData(raw);
    return { cmd: r.cmd, timestamp_us: r.timestampUs, data: toHex(r.data) };
  }
  if (report === 0x93) {
    const s = unpackBno055Stream(raw);
    return { timestamp_us: s.timestampUs, addr: s.addr, len: s.length, data: toHex(s.data) };
  }
  throw new Error(`unknown report ${report}`);
}

export function bno055Block(addr: number, data: Uint8Array): Any {
  const b = decodeBno055Block(addr, data);
  return {
    accel: b.accel, mag: b.mag, gyro: b.gyro, euler: b.euler, quaternion: b.quaternion,
    linear_accel: b.linearAccel, gravity: b.gravity, temperature: b.temperature, calib_stat: b.calibStat,
  };
}

// ── BNO086 (contract 05) ─────────────────────────────────────────────────────

export function bno086Cargo(c: ShtpCargo): Any {
  return { channel: c.channel, seq: c.seq, payload: toHex(c.payload), capture_us: c.captureUs };
}

/** Scaled / derived getters next to the raw ints: not part of the vector `fields`. */
const BNO086_DERIVED = new Set([
  "type", "x", "y", "z", "bias", "i", "j", "k", "real", "accuracyRad", "value", "doubleTap", "name",
  "mostLikelyName", "angularVelocity", "intentName", "requestName", "wantsStillness",
]);

/** The raw `fields` of a decoded SH-2 report (bytes hex, bools as 0/1, snake_case keys). */
export function bno086Fields(rep: Report): Record<string, unknown> {
  const out: Record<string, unknown> = {};
  for (const [k, v] of Object.entries(rep)) {
    if (BNO086_DERIVED.has(k)) continue;
    const key = k.replace(/[A-Z]/g, (c) => "_" + c.toLowerCase());
    out[key] = v instanceof Uint8Array ? toHex(v) : typeof v === "boolean" ? Number(v) : v;
  }
  return out;
}

// ── VL53L4CD / L7 / LX info + result blocks ──────────────────────────────────

export function vl53l4Decode(report: number, raw: Uint8Array): Any {
  if (report === 0x91) {
    const got = unpackRegData(raw);
    return { cmd: got.cmd, timestamp_us: got.timestampUs, data: toHex(got.data) };
  }
  if (report === 0x92) {
    const got = unpackInfo(raw);
    return {
      int_edges: got.intEdges, slots_skipped: got.slotsSkipped,
      i2c_errors: got.i2cErrors, last_i2c_error: got.lastI2cError,
      model_id: got.modelId, fw_status: got.fwStatus, initialized: got.initialized,
      xshut_level: got.xshutLevel, int_level: got.intLevel, i2c_khz: got.i2cKhz,
    };
  }
  if (report === 0x93) {
    const got = unpackStream(raw);
    return { timestamp_us: got.timestampUs, addr: got.addr, len: got.length, data: toHex(got.data) };
  }
  throw new Error(`unknown report ${report}`);
}

export function vl53l4Result(raw: Uint8Array): Any {
  const got = parseResultBlock(raw);
  return {
    range_status: got.rangeStatus, distance_mm: got.distanceMm,
    ambient_rate_kcps: got.ambientRateKcps, ambient_per_spad_kcps: got.ambientPerSpadKcps,
    signal_rate_kcps: got.signalRateKcps, signal_per_spad_kcps: got.signalPerSpadKcps,
    number_of_spad: got.numberOfSpad, sigma_mm: got.sigmaMm, stream_count: got.streamCount,
  };
}

export function vl53l7Info(raw: Uint8Array): Any {
  const got = unpackVl53l7Info(raw);
  return {
    int_edges: got.intEdges, frames_dropped: got.framesDropped,
    i2c_errors: got.i2cErrors, last_i2c_error: got.lastI2cError,
    lpn_level: got.lpnLevel, int_level: got.intLevel, i2c_khz: got.i2cKhz,
    frame_size: got.frameSize, streaming: got.streaming,
  };
}

export function vl53lxInfo(raw: Uint8Array): Any {
  const got = unpackVl53lxInfo(raw);
  return {
    int_edges: got.intEdges, slots_skipped: got.slotsSkipped,
    i2c_errors: got.i2cErrors, last_i2c_error: got.lastI2cError,
    xshut_level: got.xshutLevel, int_level: got.intLevel, i2c_khz: got.i2cKhz,
    addr_width: got.addrWidth, n_clear: got.nClear, frames_dropped: got.framesDropped,
  };
}

export function vl53lxL0xRaw(raw: Uint8Array): Any {
  const got = decodeL0xRaw(raw);
  return {
    distance_raw: got.distanceRaw,
    device_range_status: got.deviceRangeStatus,
    signal_rate_mcps_1616: got.signalRateMcps1616,
    ambient_rate_mcps_1616: got.ambientRateMcps1616,
    effective_spad_count_88: got.effectiveSpadCount88,
  };
}

// ── TMF8829 (contract 14) ────────────────────────────────────────────────────

const sha256 = (b: Uint8Array): string => createHash("sha256").update(b).digest("hex");
const list = (a: ArrayLike<number> | null): number[] | null => (a === null ? null : Array.from(a));

export function tmf8829Arrays(r: Tmf8829PixelResults): Any {
  return {
    peaks: r.peaks,
    rows: r.rows,
    cols: r.cols,
    distance: list(r.distance),
    snr: list(r.snr),
    signal: list(r.signal),
    noise: list(r.noise),
    xtalk: list(r.xtalk),
  };
}

export function tmf8829Decode(report: number, p: Uint8Array): Any {
  if (report === Tmf8829Rpt.RegData) {
    const r = unpackTmf8829RegData(p);
    return { timestamp_us: r.timestampUs, data: toHex(r.data) };
  }
  if (report === Tmf8829Rpt.Info) {
    const r = unpackTmf8829Info(p);
    return {
      int_edges: r.intEdges,
      frames: r.frames,
      frames_aborted: r.framesAborted,
      frame_errors: r.frameErrors,
      spi_timeouts: r.spiTimeouts,
      int_level: r.intLevel,
      streaming: r.streaming,
      int_mask: r.intMask,
      last_int_status: r.lastIntStatus,
      last_fifo_status: r.lastFifoStatus,
    };
  }
  if (report === Tmf8829Rpt.Frame) {
    const r = unpackTmf8829FrameChunk(p);
    return {
      timestamp_us: r.timestampUs,
      full_size: r.fullSize,
      offset: r.offset,
      frame_seq: r.frameSeq,
      edge_us: r.edgeUs,
      data: toHex(r.data),
    };
  }
  if (report === Tmf8829Rpt.Event) {
    const r = unpackTmf8829Event(p);
    return {
      timestamp_us: r.timestampUs,
      int_status: r.intStatus,
      code: r.code,
      frame_id: r.frameId,
      fifo_status: r.fifoStatus,
    };
  }
  throw new Error(`unknown TMF8829 report ${report}`);
}

/** vl53l8.json decode shape: one RPT_VL53_FRAME chunk (contract 04 §3). */
export function vl53l8Decode(report: number, payload: Uint8Array): Any {
  if (report === Vl53l8Rpt.Vl53Frame) {
    const c = unpackFrameChunk(payload);
    return { timestamp_us: c.timestampUs, full_size: c.fullSize, offset: c.offset, data: toHex(c.data) };
  }
  throw new Error(`unknown VL53L8 report ${report}`);
}

/** RPT_VL53_FRAME payloads through one reassembler; vl53l8.json reassembly shape (contract 04 §4). */
export function vl53l8Reassembly(chunks: Uint8Array[]): Any {
  const r = new FrameReassembler();
  const frames: Any[] = [];
  for (const ch of chunks) {
    const f = r.feed(unpackFrameChunk(ch));
    if (f !== null) frames.push({ timestamp_us: f.timestampUs, size: f.frame.length, sha256: sha256(f.frame) });
  }
  return { frames, completed: r.completed, discarded: r.discarded };
}

/** Feed FRAME-chunk payloads to one reassembler; the reassembly shape. */
export function tmf8829Reassembly(chunks: Uint8Array[]): Any {
  const r = new Tmf8829FrameReassembler();
  const frames: Any[] = [];
  for (const ch of chunks) {
    const f = r.feed(unpackTmf8829FrameChunk(ch));
    if (f !== null) {
      frames.push({
        frame_seq: f.frameSeq,
        timestamp_us: f.timestampUs,
        edge_us: f.edgeUs,
        size: f.data.length,
        sha256: sha256(f.data),
      });
    }
  }
  return { frames, completed: r.completed, discarded: r.discarded, seq_gaps: r.seqGaps };
}

/** tmf8829.json frames shape: header, problems, then results or data_sha256. */
export function tmf8829Frame(raw: Uint8Array): Any {
  const f = parseTmf8829Frame(raw);
  const out: Any = {
    header: {
      fifo_status: f.fifoStatus,
      systick: f.systick,
      id: f.id,
      kind: f.kind,
      fp_mode: f.fpMode,
      layout: f.layout,
      payload: f.payload,
      f_number: f.fNumber,
      temperature: f.temperature,
      bdv: f.bdv,
      ref_pos: f.refPos,
      t0_integration: f.t0Integration,
      t1_integration: f.t1Integration,
      frame_status: f.frameStatus,
      reserved: f.reserved,
      eof: f.eof,
      size: f.size,
    },
    problems: tmf8829FrameProblems(f),
  };
  if (f.kind === TMF8829_FID_RESULTS) out.results = tmf8829Arrays(parseTmf8829Results(raw, f));
  else out.data_sha256 = sha256(raw.subarray(21, raw.length - 12));
  return out;
}

/** `frames` = [raw, timestamp_us, edge_us] in order; the measurements expect list. */
export function tmf8829Measurements(frames: Array<[Uint8Array, number, number]>): Any[] {
  const b = new Tmf8829MeasurementBuilder();
  const got: Any[] = [];
  for (const [raw, ts, edge] of frames) {
    const m = b.add(raw, ts, edge);
    if (m === null) continue;
    got.push({
      timestamp_us: m.timestampUs,
      flags: m.flags,
      fp_mode: m.fpMode,
      f_number: m.fNumber,
      frame_times_us: m.times.map((t) => t.timestampUs),
      t1_len_us: m.times.map((t) => t.t1LenUs),
      ratio_q16: b.clock.ratioQ16,
      n_histograms: m.histograms.length,
      n_ref_frames: m.refFrames.length,
      n_problems: m.problems.length,
      distance_in_mm: m.frames[m.frames.length - 1]!.distanceInMm,
      ...tmf8829Arrays(m.results),
    });
  }
  return got;
}
