/**
 * Tmf8829MeasurementBuilder on malformed streams (contract 14 §3 "Frames per
 * measurement"): parse failures, unknown frame kinds, frame-status problems,
 * the half-frame pairing rules, reset(), and the contract 09 record value.
 * Frames are the real captures of contracts/vectors/tmf8829.json, some edited.
 */

import { describe, expect, it } from "vitest";

import { Tmf8829MeasurementBuilder, tmf8829RecordValue, type Tmf8829Measurement } from "../src/index.js";
import { fromHex, loadVectors } from "./golden/vectors.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;
const V: Any = loadVectors("tmf8829.json");
const RAW = new Map<string, Uint8Array>(V.frames.map((c: Any) => [c.name, fromHex(c.raw)]));
const MEAS = new Map<string, Any>(V.measurements.map((m: Any) => [m.name, m]));
/** [raw, timestamp_us, edge_us] of a measurement vector's frame. */
const F = (meas: string, i: number): [Uint8Array, number, number] => {
  const f = MEAS.get(meas).frames[i];
  return [RAW.get(f.raw_case)!.slice(), f.timestamp_us, f.edge_us];
};
const add = (b: Tmf8829MeasurementBuilder, f: [Uint8Array, number, number]): Tmf8829Measurement | null =>
  b.add(f[0], f[1], f[2]);

describe("Tmf8829MeasurementBuilder", () => {
  it("counts a frame too short to parse and reports it with the next measurement", () => {
    const b = new Tmf8829MeasurementBuilder();
    expect(b.add(new Uint8Array(20), 1000)).toBeNull();
    expect(b.parseErrors).toBe(1);
    const m = add(b, F("8x8", 0))!;
    expect(m.problems).toEqual(["frame of 20 B is shorter than header + footer"]);
    expect(add(b, F("8x8", 1))!.problems).toEqual([]); // problems are per measurement
  });

  it("refuses an unknown frame kind", () => {
    const b = new Tmf8829MeasurementBuilder();
    const [raw, ts] = F("8x8", 0);
    raw[5] = 0x40;
    expect(b.add(raw, ts)).toBeNull();
    expect(b.parseErrors).toBe(1);
    expect(add(b, F("8x8", 1))!.problems).toEqual(["frame id 0x40: unknown kind"]);
  });

  it("keeps a measurement whose footer is bad but lists the problems", () => {
    const b = new Tmf8829MeasurementBuilder();
    const [raw, ts, edge] = F("8x8", 0);
    const foot = raw.length - 12;
    raw[foot + 8] = 0xc8; // status: not valid, aborted, HV_CP_OVERLOAD
    raw[foot + 10] = 0; // EOF 0xE000
    raw[0] = 0x30; // tx underrun + rx overrun
    const m = b.add(raw, ts, edge)!;
    expect(m).not.toBeNull();
    expect(m.problems).toEqual([
      "frame id=0x10 fN=1 225 B: EOF 0xE000, status 0xC8 not valid, aborted, HV_CP_OVERLOAD, txfifo_underrun, rxfifo_overrun",
    ]);
    expect(b.parseErrors).toBe(0);
  });

  it("drops a results frame whose data size does not match its layout", () => {
    const b = new Tmf8829MeasurementBuilder();
    const [raw, ts, edge] = F("8x8", 0);
    raw[6] = 0x02; // layout claims 2 peaks
    expect(b.add(raw, ts, edge)).toBeNull();
    expect(b.parseErrors).toBe(1);
    const m = add(b, F("8x8", 1))!;
    expect(m.problems).toEqual(["fN=1: result data 192 B, expected 64 px × 6 B (layout 0x2)"]);
  });

  it("a results frame that fails to decode also drops a pending first half", () => {
    const b = new Tmf8829MeasurementBuilder();
    expect(add(b, F("32x32", 0))).toBeNull(); // first half pending
    const [bad, ts, edge] = F("32x32", 1);
    bad[6] = 0x42; // second half claiming 2 peaks: size mismatch
    expect(b.add(bad, ts, edge)).toBeNull();
    expect(b.parseErrors).toBe(1);
    // The first half went with it: the real second half is now an orphan.
    expect(add(b, F("32x32", 1))).toBeNull();
    expect(add(b, F("32x32", 0))).toBeNull();
    const m = add(b, F("32x32", 1))!;
    expect(m.rows).toBe(32);
    expect(m.problems).toEqual([
      "fN=2: result data 1536 B, expected 512 px × 6 B (layout 0x42)",
      "second half fN=2 without a first one - dropped",
    ]);
  });

  it("drops a second half without a first, together with its ref-SPAD frame", () => {
    const b = new Tmf8829MeasurementBuilder();
    expect(add(b, F("32x32_ref", 2))).toBeNull(); // ref frame of the second half
    expect(add(b, F("32x32_ref", 3))).toBeNull(); // second half, no first: dropped with its ref
    for (let i = 0; i < 3; i++) expect(add(b, F("32x32_ref", i))).toBeNull();
    const m = add(b, F("32x32_ref", 3))!;
    // only this pair's two ref frames, not the one dropped with the orphan half
    expect(m.refFrames.length).toBe(2);
    expect(m.problems).toEqual(["second half fN=2 without a first one - dropped"]);
  });

  it("keeps the newer of two first halves in a row", () => {
    const b = new Tmf8829MeasurementBuilder();
    const [first, ts0] = F("32x32", 0);
    expect(b.add(first, ts0 - 60000)).toBeNull();
    expect(add(b, F("32x32", 0))).toBeNull();
    const m = add(b, F("32x32", 1))!;
    expect(m.problems).toEqual(["two first halves in a row (fN 1, 1)"]);
    expect(m.frames.map((f) => f.fNumber)).toEqual([1, 2]);
    expect(m.times[0]!.timestampUs).toBe(MEAS.get("32x32").expect[0].frame_times_us[0]);
  });

  it("drops a pair whose halves differ in format", () => {
    const b = new Tmf8829MeasurementBuilder();
    expect(add(b, F("32x32", 0))).toBeNull();
    const [second48] = F("48x32_2p_sig", 1);
    const [, ts1, e1] = F("32x32", 1);
    expect(b.add(second48, ts1, e1)).toBeNull();
    expect(add(b, F("32x32", 0))).toBeNull();
    const m = add(b, F("32x32", 1))!;
    expect(m.problems).toEqual(["half-frames of different formats (fN 1, 2) - dropped"]);
    expect(m.rows).toBe(32);
  });

  it("reset() forgets the pending half, frames, problems and counters", () => {
    const b = new Tmf8829MeasurementBuilder();
    b.add(new Uint8Array(3), 1);
    add(b, F("8x8_ref", 0)); // pending ref frame
    add(b, F("32x32", 0)); // pending first half
    b.reset();
    expect(b.parseErrors).toBe(0);
    expect(add(b, F("32x32", 1))).toBeNull(); // the first half is gone
    const m = add(b, F("8x8", 0))!;
    expect(m.refFrames).toEqual([]);
    expect(m.problems).toEqual(["second half fN=2 without a first one - dropped"]);
  });

  it("refuses times that are not u64 before parsing", () => {
    const b = new Tmf8829MeasurementBuilder();
    expect(() => b.add(RAW.get("8x8#0")!, -1)).toThrow(RangeError);
    expect(() => b.add(RAW.get("8x8#0")!, 1, 0.5)).toThrow(RangeError);
    expect(b.parseErrors).toBe(0);
  });
});

describe("tmf8829RecordValue (contract 09)", () => {
  it("rounds distances to 0.1 mm and omits absent optional channels", () => {
    const b = new Tmf8829MeasurementBuilder();
    const m = add(b, F("8x8", 0))!;
    const v = tmf8829RecordValue(m);
    expect(Object.keys(v).sort()).toEqual(
      ["cols", "distance_mm", "f_number", "flags", "fp_mode", "peaks", "rows", "snr", "temperature_c"].sort(),
    );
    const e = MEAS.get("8x8").expect[0];
    expect(v).toMatchObject({ fp_mode: 0, rows: 8, cols: 8, peaks: 1, f_number: 1, flags: e.flags, temperature_c: 20 });
    const dist = v.distance_mm as number[];
    expect(dist.length).toBe(64);
    expect(dist[0]).toBe(Math.round(((e.distance[0] * e.ratio_q16) / (8 << 16) / 4) * 10) / 10);
    expect(dist.every((x) => Math.abs(Math.round(x * 10) - x * 10) < 1e-6)).toBe(true);
    expect(v.snr).toEqual(e.snr);
  });

  it("includes signal, noise and xtalk when the layout has them", () => {
    const b = new Tmf8829MeasurementBuilder();
    const m = add(b, F("8x8_full", 0))!;
    const v = tmf8829RecordValue(m);
    const e = MEAS.get("8x8_full").expect[0];
    expect(v.peaks).toBe(4);
    expect(v.signal).toEqual(e.signal);
    expect(v.noise).toEqual(e.noise);
    expect(v.xtalk).toEqual(e.xtalk);
    expect((v.distance_mm as number[]).length).toBe(4 * 64);
  });
});
