import { describe, expect, it } from "vitest";
import {
  packReadReg, packSetI2cSpeed, packStartStream, packWriteReg, packXshut,
  unpackInfo, unpackRegData, unpackStream,
} from "../../src/protocol/vl53l4.js";
import {
  DEFAULT_CONFIGURATION, configBlock, decodeOffset, decodeRangeTiming,
  decodeSigmaThreshold, decodeSignalThreshold, decodeXtalk, offsetRaw,
  parseResultBlock, rangeTimingRegisters, sigmaThresholdRaw,
  signalThresholdRaw, xtalkRaw,
} from "../../src/sensors/vl53l4/uld.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const v = loadVectors<any>("vl53l4.json");

describe("vl53l4.json", () => {
  for (const c of v.encode) it(`encode ${c.name}`, () => {
    let got: Uint8Array;
    if (c.kind === "read_reg") got = packReadReg(c.addr, c.len);
    else if (c.kind === "write_reg") got = packWriteReg(c.addr, fromHex(c.data));
    else if (c.kind === "xshut") got = packXshut(c.action);
    else if (c.kind === "start_stream") got = packStartStream(c.addr, c.len, c.flags);
    else if (c.kind === "set_i2c_speed") got = packSetI2cSpeed(c.khz);
    else throw new Error(`unknown encode kind ${c.kind}`);
    expect(toHex(got)).toBe(c.payload);
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    const raw = fromHex(c.payload);
    if (c.report === 0x91) {
      const got = unpackRegData(raw);
      expect({ cmd: got.cmd, timestamp_us: Number(got.timestampUs), data: toHex(got.data) }).toEqual(c.expect);
    } else if (c.report === 0x92) {
      const got = unpackInfo(raw);
      expect({
        int_edges: got.intEdges, slots_skipped: got.slotsSkipped,
        i2c_errors: got.i2cErrors, last_i2c_error: got.lastI2cError,
        model_id: got.modelId, fw_status: got.fwStatus, initialized: got.initialized,
        xshut_level: got.xshutLevel, int_level: got.intLevel, i2c_khz: got.i2cKhz,
      }).toEqual(c.expect);
    } else {
      const got = unpackStream(raw);
      expect({ timestamp_us: Number(got.timestampUs), addr: got.addr, len: got.length, data: toHex(got.data) }).toEqual(c.expect);
    }
  });

  for (const c of v.result_block) it(`result ${c.name}`, () => {
    const got = parseResultBlock(fromHex(c.raw));
    expect({
      range_status: got.rangeStatus, distance_mm: got.distanceMm,
      ambient_rate_kcps: got.ambientRateKcps, ambient_per_spad_kcps: got.ambientPerSpadKcps,
      signal_rate_kcps: got.signalRateKcps, signal_per_spad_kcps: got.signalPerSpadKcps,
      number_of_spad: got.numberOfSpad, sigma_mm: got.sigmaMm, stream_count: got.streamCount,
    }).toEqual(c.expect);
  });

  for (const c of v.timing.encode) it(`timing encode ${c.name}`, () => {
    const got = rangeTimingRegisters(c.timing_budget_ms, c.inter_measurement_ms, c.osc_frequency, c.clock_pll);
    expect(got).toEqual({ rangeConfigA: c.range_config_a, rangeConfigB: c.range_config_b, intermeasurementRaw: c.intermeasurement_raw });
  });

  for (const c of v.timing.decode) it(`timing decode ${c.name}`, () => {
    const got = decodeRangeTiming(c.intermeasurement_raw, c.clock_pll, c.osc_frequency, c.range_config_a);
    expect(got).toEqual({ timingBudgetMs: c.timing_budget_ms, interMeasurementMs: c.inter_measurement_ms });
  });

  for (const c of v.tuning) it(`tuning ${c.name}`, () => {
    const pairs: Record<string, [(n: number) => number, (n: number) => number]> = {
      offset: [offsetRaw, decodeOffset], xtalk: [xtalkRaw, decodeXtalk],
      signal_threshold: [signalThresholdRaw, decodeSignalThreshold],
      sigma_threshold: [sigmaThresholdRaw, decodeSigmaThreshold],
    };
    const [encode, decode] = pairs[c.kind]!;
    expect(encode(c.value)).toBe(c.raw);
    expect(decode(c.raw)).toBe(c.value);
  });

  it("config block", () => {
    expect(DEFAULT_CONFIGURATION.length).toBe(fromHex(v.config_block.data).length);
    expect(toHex(configBlock())).toBe(v.config_block.data);
  });
});
