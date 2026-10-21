import { createHash } from "node:crypto";
import { describe, expect, it } from "vitest";
import {
  Tmf8829Cmd,
  Tmf8829FrameReassembler,
  Tmf8829Rpt,
  packTmf8829ReadReg,
  packTmf8829StartStream,
  packTmf8829WriteReg,
  unpackTmf8829Event,
  unpackTmf8829FrameChunk,
  unpackTmf8829Info,
  unpackTmf8829RegData,
} from "../../src/protocol/tmf8829.js";
import { Tmf8829MeasurementBuilder } from "../../src/sensors/tmf8829/builder.js";
import { Tmf8829CaptureTimer, Tmf8829SensorClock } from "../../src/sensors/tmf8829/clock.js";
import { tmf8829CfgGet, tmf8829CfgSet, type Tmf8829CfgField } from "../../src/sensors/tmf8829/config.js";
import {
  TMF8829_FID_RESULTS,
  parseTmf8829Frame,
  parseTmf8829Results,
  tmf8829FrameProblems,
  type Tmf8829PixelResults,
} from "../../src/sensors/tmf8829/frames.js";
import { TMF8829_FIRMWARE_LOAD_ADDR, tmf8829FirmwareImage } from "../../src/sensors/tmf8829/tmf8829.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;

const V: Any = loadVectors("tmf8829.json");
const FT: Any = loadVectors("tmf8829_frame_time.json");

const sha256 = (b: Uint8Array): string => createHash("sha256").update(b).digest("hex");
const list = (a: ArrayLike<number> | null): number[] | null => (a === null ? null : Array.from(a));

function arrays(r: Tmf8829PixelResults): Any {
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

describe("tmf8829.json", () => {
  it("encodes commands", () => {
    for (const c of V.encode) {
      let p: Uint8Array;
      if (c.cmd === Tmf8829Cmd.ReadReg) p = packTmf8829ReadReg(c.addr, c.len);
      else if (c.cmd === Tmf8829Cmd.WriteReg) p = packTmf8829WriteReg(c.addr, fromHex(c.data));
      else if (c.cmd === Tmf8829Cmd.StartStream) p = packTmf8829StartStream(c.int_mask, c.flags);
      else p = new Uint8Array(0);
      expect(toHex(p), c.name).toBe(c.payload);
    }
  });

  it("refuses out-of-range commands", () => {
    for (const c of V.encode_errors) {
      const f =
        c.kind === "read_reg"
          ? () => packTmf8829ReadReg(c.addr, c.len)
          : c.kind === "write_reg"
            ? () => packTmf8829WriteReg(c.addr, fromHex(c.data))
            : () => packTmf8829StartStream(c.int_mask, c.flags);
      expect(f, c.name).toThrow(RangeError);
    }
  });

  it("decodes reports", () => {
    for (const c of V.decode) {
      const p = fromHex(c.payload);
      let got: Any;
      if (c.report === Tmf8829Rpt.RegData) {
        const r = unpackTmf8829RegData(p);
        got = { timestamp_us: Number(r.timestampUs), data: toHex(r.data) };
      } else if (c.report === Tmf8829Rpt.Info) {
        const r = unpackTmf8829Info(p);
        got = {
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
      } else if (c.report === Tmf8829Rpt.Frame) {
        const r = unpackTmf8829FrameChunk(p);
        got = {
          timestamp_us: Number(r.timestampUs),
          full_size: r.fullSize,
          offset: r.offset,
          frame_seq: r.frameSeq,
          edge_us: Number(r.edgeUs),
          data: toHex(r.data),
        };
      } else {
        const r = unpackTmf8829Event(p);
        got = {
          timestamp_us: Number(r.timestampUs),
          int_status: r.intStatus,
          code: r.code,
          frame_id: r.frameId,
          fifo_status: r.fifoStatus,
        };
      }
      expect(got, c.name).toEqual(c.expect);
    }
  });

  it("reassembles chunks", () => {
    for (const c of V.reassembly) {
      const r = new Tmf8829FrameReassembler();
      const frames: Any[] = [];
      for (const ch of c.chunks) {
        const f = r.feed(unpackTmf8829FrameChunk(fromHex(ch)));
        if (f !== null) {
          frames.push({
            frame_seq: f.frameSeq,
            timestamp_us: Number(f.timestampUs),
            edge_us: Number(f.edgeUs),
            size: f.data.length,
            sha256: sha256(f.data),
          });
        }
      }
      expect({ frames, completed: r.completed, discarded: r.discarded, seq_gaps: r.seqGaps }, c.name).toEqual(
        c.expect,
      );
    }
  });

  it("parses frames and pixels", () => {
    for (const c of V.frames) {
      const raw = fromHex(c.raw);
      const f = parseTmf8829Frame(raw);
      const h = c.header;
      expect(
        {
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
        c.name,
      ).toEqual(h);
      expect(tmf8829FrameProblems(f), c.name).toEqual(c.problems);
      if (f.kind === TMF8829_FID_RESULTS) expect(arrays(parseTmf8829Results(raw, f)), c.name).toEqual(c.results);
      else expect(sha256(raw.subarray(21, raw.length - 12)), c.name).toBe(c.data_sha256);
    }
  });

  it("assembles measurements", () => {
    const rawByCase = new Map<string, Uint8Array>(V.frames.map((c: Any) => [c.name, fromHex(c.raw)]));
    for (const s of V.measurements) {
      const b = new Tmf8829MeasurementBuilder();
      const got: Any[] = [];
      for (const f of s.frames) {
        const m = b.add(rawByCase.get(f.raw_case)!, f.timestamp_us, f.edge_us);
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
          ...arrays(m.results),
        });
      }
      expect(got, s.name).toEqual(s.expect);
    }
  });

  it("gets and sets config fields", () => {
    const page = fromHex(V.config.page);
    for (const [name, value] of Object.entries(V.config.get)) {
      expect(tmf8829CfgGet(page, name as Tmf8829CfgField), name).toBe(value);
    }
    for (const c of V.config.set) {
      const pg = page.slice();
      tmf8829CfgSet(pg, c.field, c.value);
      expect(toHex(pg), c.field).toBe(c.page);
    }
    for (const c of V.config.set_errors) {
      expect(() => tmf8829CfgSet(page.slice(), c.field, c.value), c.field).toThrow(RangeError);
    }
  });

  it("embeds the sensor firmware image", async () => {
    const img = await tmf8829FirmwareImage();
    expect(TMF8829_FIRMWARE_LOAD_ADDR).toBe(V.firmware.load_addr);
    expect([img.length, img.length / 4, sha256(img)]).toEqual([V.firmware.size, V.firmware.words, V.firmware.sha256]);
  });
});

describe("tmf8829_frame_time.json", () => {
  it("reproduces every capture time bit for bit", () => {
    const col: Record<string, number> = Object.fromEntries(FT.columns.map((n: string, i: number) => [n, i]));
    let checked = 0;
    for (const s of FT.streams) {
      const clock = new Tmf8829SensorClock();
      const timer = new Tmf8829CaptureTimer(clock);
      for (const row of s.frames) {
        const id = row[col.id!];
        const ts = row[col.ts!];
        const tick = row[col.systick!];
        const kind = id & 0xf0;
        if (kind === 0x30) continue;
        const flags = clock.add(ts, tick);
        if (kind === 0x20) {
          timer.histograms();
          continue;
        }
        const t = timer.results(id & 0x0f, tick, row[col.t0!], row[col.t1!], ts, row[col.edge_us!], flags);
        expect([t.timestampUs, t.t1LenUs, t.flags, clock.ratioQ16], `${s.scenario} fN=${row[col.f_number!]}`).toEqual([
          row[col.exp_timestamp_us!],
          row[col.exp_t1_len_us!],
          row[col.exp_flags!],
          row[col.exp_ratio_q16!],
        ]);
        checked += 1;
      }
    }
    expect(checked).toBe(970);
  });
});
