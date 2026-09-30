import { describe, expect, it } from "vitest";
import {
  VL53L7_INFO_SIZE,
  packPinCtrl,
  packVl53l7SetI2cSpeed,
  resolveVl53l7Model,
  unpackVl53l7Info,
} from "../../src/protocol/vl53l7.js";
import { packReadReg, packWriteReg } from "../../src/protocol/vl53l8.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const v = loadVectors<any>("vl53l7.json");

describe("vl53l7.json", () => {
  for (const c of v.encode) it(`encode ${c.name}`, () => {
    let got: Uint8Array;
    if (c.kind === "read_reg") got = packReadReg(c.addr, c.len);
    else if (c.kind === "write_reg") got = packWriteReg(c.addr, fromHex(c.data));
    else if (c.kind === "pin_ctrl") got = packPinCtrl(c.action);
    else if (c.kind === "set_i2c_speed") got = packVl53l7SetI2cSpeed(c.khz);
    else throw new Error(`unknown encode kind ${c.kind}`);
    expect(toHex(got)).toBe(c.payload);
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    expect(c.report).toBe(0x92);
    const got = unpackVl53l7Info(fromHex(c.payload));
    expect({
      int_edges: got.intEdges, frames_dropped: got.framesDropped,
      i2c_errors: got.i2cErrors, last_i2c_error: got.lastI2cError,
      lpn_level: got.lpnLevel, int_level: got.intLevel, i2c_khz: got.i2cKhz,
      frame_size: got.frameSize, streaming: got.streaming,
    }).toEqual(c.expect);
  });

  it("RPT_VL53_INFO rejects a short payload", () => {
    expect(() => unpackVl53l7Info(new Uint8Array(VL53L7_INFO_SIZE - 1))).toThrow();
  });

  for (const c of v.model) it(`model ${c.name}`, () => {
    expect(resolveVl53l7Model(c.usb_model, c.device_name)).toBe(c.expect);
  });
});
