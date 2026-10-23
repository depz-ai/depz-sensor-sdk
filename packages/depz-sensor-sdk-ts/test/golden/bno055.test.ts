import { describe, expect, it } from "vitest";
import { packBno055ReadReg, packBno055StartStream, packBno055WriteReg } from "../../src/protocol/bno055.js";
import {
  bno055FullyCalibrated,
  bno055Placement,
  packBno055AccelConfig,
  packBno055AxisRemap,
  packBno055CalibStatus,
  packBno055CalibrationProfile,
  packBno055GyroConfig,
  packBno055MagConfig,
  packBno055Units,
  unpackBno055AccelConfig,
  unpackBno055AxisRemap,
  unpackBno055CalibStatus,
  unpackBno055CalibrationProfile,
  unpackBno055GyroConfig,
  unpackBno055MagConfig,
  unpackBno055Units,
} from "../../src/sensors/bno055/regs.js";
import { bno055Block, bno055Decode, plain } from "./shapes.js";
import { fromHex, loadVectors, toHex, seen } from "./vectors.js";

const v = loadVectors<any>("bno055.json");

describe("bno055.json", () => {
  for (const c of v.encode) it(`encode ${c.name}`, () => {
    let got: Uint8Array;
    if (c.kind === "read_reg") got = packBno055ReadReg(c.addr, c.len);
    else if (c.kind === "write_reg") got = packBno055WriteReg(c.addr, fromHex(c.data));
    else if (c.kind === "start_stream") got = packBno055StartStream(c.trigger, c.addr, c.len, c.period_ms);
    else if (["reset", "stop_stream", "get_info"].includes(c.kind)) got = new Uint8Array(0);
    else throw new Error(`unknown encode kind ${c.kind}`);
    expect(toHex(got)).toBe(c.payload);
    seen(c);
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    expect(plain(bno055Decode(c.report, fromHex(c.payload)))).toEqual(c.expect);
    seen(c);
  });

  for (const c of v.units) it(`units 0x${c.unit_sel.toString(16)}`, () => {
    const u = unpackBno055Units(c.unit_sel);
    expect({
      accel_mg: u.accelMg, gyro_rps: u.gyroRps, euler_rad: u.eulerRad, temp_f: u.tempF, android: u.android,
    }).toEqual(c.expect);
    expect(packBno055Units(u)).toBe(c.repack);
    seen(c);
  });

  for (const c of v.calib_stat) it(`calib_stat 0x${c.value.toString(16)}`, () => {
    const s = unpackBno055CalibStatus(c.value);
    expect(s).toEqual(c.expect);
    expect(bno055FullyCalibrated(s)).toBe(c.fully_calibrated);
    expect(packBno055CalibStatus(s)).toBe(c.value);
    seen(c);
  });

  for (const c of v.calibration_profile) it(`calibration_profile ${c.name}`, () => {
    const p = unpackBno055CalibrationProfile(fromHex(c.bytes));
    expect({
      accel_offset: p.accelOffset, mag_offset: p.magOffset, gyro_offset: p.gyroOffset,
      accel_radius: p.accelRadius, mag_radius: p.magRadius,
    }).toEqual(c.expect);
    expect(toHex(packBno055CalibrationProfile(p))).toBe(c.bytes);
    seen(c);
  });

  for (const c of v.axis_remap) it(`axis_remap ${c.name}`, () => {
    const a = unpackBno055AxisRemap(c.config, c.sign);
    expect({
      x: a.x, y: a.y, z: a.z, x_negative: a.xNegative, y_negative: a.yNegative, z_negative: a.zNegative,
    }).toEqual(c.expect);
    expect(packBno055AxisRemap(a)).toEqual(c.repack);
    expect(bno055Placement(c.name)).toEqual(a);
    seen(c);
  });

  for (const c of v.axis_remap_invalid) it(`axis_remap_invalid ${c.name}`, () => {
    expect(() =>
      packBno055AxisRemap({ x: c.x, y: c.y, z: c.z, xNegative: false, yNegative: false, zNegative: false }),
    ).toThrow(RangeError);
    seen(c);
  });

  it("sensor_config", () => {
    for (const c of v.sensor_config.accel) {
      expect(unpackBno055AccelConfig(c.value)).toEqual(c.expect);
      expect(packBno055AccelConfig(c.expect)).toBe(c.value);
    }
    for (const c of v.sensor_config.gyro) {
      expect(unpackBno055GyroConfig(fromHex(c.bytes))).toEqual(c.expect);
      expect(toHex(packBno055GyroConfig(c.expect))).toBe(c.bytes);
    }
    for (const c of v.sensor_config.mag) {
      expect(unpackBno055MagConfig(c.value)).toEqual(c.expect);
      expect(packBno055MagConfig(c.expect)).toBe(c.value & 0x7f);
    }
    seen("bno055/sensor_config");
  });

  for (const c of v.blocks) it(`block ${c.name}`, () => {
    expect(bno055Block(c.addr, fromHex(c.data))).toEqual(c.expect);
    seen(c);
  });
});
