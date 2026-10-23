import { describe, expect, it } from "vitest";
import {
  packReadReg, packSetI2cSpeed, packStartStream, packWriteReg, packXshut,
} from "../../src/protocol/vl53l4.js";
import {
  DEFAULT_CONFIGURATION, configBlock, decodeOffset, decodeRangeTiming,
  decodeSigmaThreshold, decodeSignalThreshold, decodeXtalk, offsetRaw,
  rangeTimingRegisters, sigmaThresholdRaw,
  signalThresholdRaw, xtalkRaw,
} from "../../src/sensors/vl53l4/uld.js";
import { plain, vl53l4Decode, vl53l4Result } from "./shapes.js";
import { fromHex, loadVectors, toHex, seen, seenAfter } from "./vectors.js";

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
    seen(c);
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    expect(plain(vl53l4Decode(c.report, fromHex(c.payload)))).toEqual(c.expect);
    seen(c);
  });

  for (const c of v.result_block) it(`result ${c.name}`, () => {
    expect(vl53l4Result(fromHex(c.raw))).toEqual(c.expect);
    seen(c);
  });

  const timingDone = seenAfter("vl53l4/timing", v.timing.encode.length + v.timing.decode.length);
  for (const c of v.timing.encode) it(`timing encode ${c.name}`, () => {
    const got = rangeTimingRegisters(c.timing_budget_ms, c.inter_measurement_ms, c.osc_frequency, c.clock_pll);
    expect(got).toEqual({ rangeConfigA: c.range_config_a, rangeConfigB: c.range_config_b, intermeasurementRaw: c.intermeasurement_raw });
    timingDone();
  });

  for (const c of v.timing.decode) it(`timing decode ${c.name}`, () => {
    const got = decodeRangeTiming(c.intermeasurement_raw, c.clock_pll, c.osc_frequency, c.range_config_a);
    expect(got).toEqual({ timingBudgetMs: c.timing_budget_ms, interMeasurementMs: c.inter_measurement_ms });
    timingDone();
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
    seen(c);
  });

  // C round(): an exact .5 goes away from zero (256 -> 1), as ST's GetXtalk
  for (const c of v.xtalk_decode) it(`xtalk decode ${c.raw}`, () => {
    expect(decodeXtalk(c.raw)).toBe(c.kcps);
    seen(c);
  });

  it("config block", () => {
    expect(DEFAULT_CONFIGURATION.length).toBe(fromHex(v.config_block.data).length);
    expect(toHex(configBlock())).toBe(v.config_block.data);
    seen("vl53l4/config_block");
  });
});
