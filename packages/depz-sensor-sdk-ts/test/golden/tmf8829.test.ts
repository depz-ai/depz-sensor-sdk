import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import {
  Tmf8829Cmd,
  packTmf8829ReadReg,
  packTmf8829StartStream,
  packTmf8829WriteReg,
} from "../../src/protocol/tmf8829.js";
import { Tmf8829CaptureTimer, Tmf8829SensorClock } from "../../src/sensors/tmf8829/clock.js";
import {
  TMF8829_CFG_FIELDS,
  tmf8829CfgGet,
  tmf8829CfgSet,
  type Tmf8829CfgField,
} from "../../src/sensors/tmf8829/config.js";
import { TMF8829_FIRMWARE_LOAD_ADDR, tmf8829FirmwareImage } from "../../src/sensors/tmf8829/tmf8829.js";
import { plain, tmf8829Decode, tmf8829Frame, tmf8829Measurements, tmf8829Reassembly } from "./shapes.js";
import { fromHex, loadVectors, toHex, seen } from "./vectors.js";

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type Any = any;

const V: Any = loadVectors("tmf8829.json");
const FT: Any = loadVectors("tmf8829_frame_time.json");

const sha256 = (b: Uint8Array): string => createHash("sha256").update(b).digest("hex");

describe("tmf8829.json", () => {
  it("encodes commands", () => {
    for (const c of V.encode) {
      let p: Uint8Array;
      if (c.cmd === Tmf8829Cmd.ReadReg) p = packTmf8829ReadReg(c.addr, c.len);
      else if (c.cmd === Tmf8829Cmd.WriteReg) p = packTmf8829WriteReg(c.addr, fromHex(c.data));
      else if (c.cmd === Tmf8829Cmd.StartStream) p = packTmf8829StartStream(c.int_mask, c.flags);
      else p = new Uint8Array(0);
      expect(toHex(p), c.name).toBe(c.payload);
      seen(c);
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
      seen(c);
    }
  });

  it("decodes reports", () => {
    for (const c of V.decode) {
      expect(plain(tmf8829Decode(c.report, fromHex(c.payload))), c.name).toEqual(c.expect);
      seen(c);
    }
  });

  it("reassembles chunks", () => {
    for (const c of V.reassembly) {
      expect(plain(tmf8829Reassembly(c.chunks.map(fromHex))), c.name).toEqual(c.expect);
      seen(c);
    }
  });

  it("parses frames and pixels", () => {
    for (const c of V.frames) {
      if ("error" in c) {
        // a results frame whose pixel decode must fail
        expect(() => tmf8829Frame(fromHex(c.raw)), c.name).toThrow();
        seen(c);
        continue;
      }
      const want: Any = { header: c.header, problems: c.problems };
      if ("results" in c) want.results = c.results;
      else want.data_sha256 = c.data_sha256;
      expect(tmf8829Frame(fromHex(c.raw)), c.name).toEqual(want);
      seen(c);
    }
  });

  it("assembles measurements", () => {
    const rawByCase = new Map<string, Uint8Array>(V.frames.map((c: Any) => [c.name, fromHex(c.raw)]));
    for (const s of V.measurements) {
      const frames = s.frames.map((f: Any) => [rawByCase.get(f.raw_case)!, f.timestamp_us, f.edge_us]);
      expect(tmf8829Measurements(frames), s.name).toEqual(s.expect);
      seen(s);
    }
  });

  it("gets and sets config fields", () => {
    expect(TMF8829_CFG_FIELDS).toEqual(V.config.fields);
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
    seen("tmf8829/config");
  });

  it("embeds the sensor firmware image", async () => {
    const img = await tmf8829FirmwareImage();
    expect(TMF8829_FIRMWARE_LOAD_ADDR).toBe(V.firmware.load_addr);
    expect([img.length, img.length / 4, sha256(img)]).toEqual([V.firmware.size, V.firmware.words, V.firmware.sha256]);
    seen("tmf8829/firmware");
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
      seen(s);
    }
    expect(checked).toBe(970);
  });
});

describe("recordings/tmf8829_frame_time.jsonl", () => {
  it("replays the board's own capture raw: each `scenario` line starts a stream", () => {
    const text = readFileSync(
      new URL("../../../../contracts/vectors/recordings/tmf8829_frame_time.jsonl", import.meta.url),
      "utf8",
    );
    const clock = new Tmf8829SensorClock();
    const timer = new Tmf8829CaptureTimer(clock);
    let checked = 0;
    for (const line of text.split("\n")) {
      if (line.trim() === "") continue;
      const v = JSON.parse(line);
      if ("scenario" in v) {
        clock.reset();
        timer.reset();
        continue;
      }
      const kind = v.id & 0xf0;
      if (kind === 0x30) continue;
      const flags = clock.add(v.ts, v.systick);
      if (kind === 0x20) {
        timer.histograms();
        continue;
      }
      const t = timer.results(v.id & 0x0f, v.systick, v.t0, v.t1, v.ts, v.edge_us, flags);
      expect(
        { timestamp_us: t.timestampUs, t1_len_us: t.t1LenUs, flags: t.flags, ratio_q16: clock.ratioQ16 },
        line,
      ).toEqual(v.exp);
      checked += 1;
    }
    expect(checked).toBe(970);
    seen("recordings/tmf8829_frame_time.jsonl");
  });
});
