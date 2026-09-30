import { describe, expect, it } from "vitest";
import {
  packBno055ReadReg,
  packBno055StartStream,
  packBno055WriteReg,
  unpackBno055Info,
  unpackBno055RegData,
  unpackBno055Stream,
} from "../../src/protocol/bno055.js";
import {
  bno055FullyCalibrated,
  bno055Placement,
  decodeBno055Block,
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
import { fromHex, loadVectors, toHex } from "./vectors.js";

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
  });

  for (const c of v.decode) it(`decode ${c.name}`, () => {
    const raw = fromHex(c.payload);
    if (c.report === 0x92) {
      const i = unpackBno055Info(raw);
      expect({
        i2c_addr: i.i2cAddr, chip_id: i.chipId, acc_id: i.accId, mag_id: i.magId, gyr_id: i.gyrId,
        sw_rev: i.swRev, bl_rev: i.blRev, initialized: i.initialized, int_level: i.intLevel,
        int_edges: i.intEdges, read_min_us: i.readMinUs, read_max_us: i.readMaxUs,
        read_avg_us: i.readAvgUs, tx_dropped: i.txDropped, i2c_errors: i.i2cErrors,
        slots_skipped: i.slotsSkipped, bus_recoveries: i.busRecoveries,
        last_i2c_error: i.lastI2cError, sensor_resets: i.sensorResets, loop_max_us: i.loopMaxUs,
      }).toEqual(c.expect);
    } else if (c.report === 0x91) {
      const r = unpackBno055RegData(raw);
      expect({ cmd: r.cmd, timestamp_us: Number(r.timestampUs), data: toHex(r.data) }).toEqual(c.expect);
    } else if (c.report === 0x93) {
      const s = unpackBno055Stream(raw);
      expect({ timestamp_us: Number(s.timestampUs), addr: s.addr, len: s.length, data: toHex(s.data) })
        .toEqual(c.expect);
    } else throw new Error(`unknown report ${c.report}`);
  });

  for (const c of v.units) it(`units 0x${c.unit_sel.toString(16)}`, () => {
    const u = unpackBno055Units(c.unit_sel);
    expect({
      accel_mg: u.accelMg, gyro_rps: u.gyroRps, euler_rad: u.eulerRad, temp_f: u.tempF, android: u.android,
    }).toEqual(c.expect);
    expect(packBno055Units(u)).toBe(c.repack);
  });

  for (const c of v.calib_stat) it(`calib_stat 0x${c.value.toString(16)}`, () => {
    const s = unpackBno055CalibStatus(c.value);
    expect(s).toEqual(c.expect);
    expect(bno055FullyCalibrated(s)).toBe(c.fully_calibrated);
    expect(packBno055CalibStatus(s)).toBe(c.value);
  });

  for (const c of v.calibration_profile) it(`calibration_profile ${c.name}`, () => {
    const p = unpackBno055CalibrationProfile(fromHex(c.bytes));
    expect({
      accel_offset: p.accelOffset, mag_offset: p.magOffset, gyro_offset: p.gyroOffset,
      accel_radius: p.accelRadius, mag_radius: p.magRadius,
    }).toEqual(c.expect);
    expect(toHex(packBno055CalibrationProfile(p))).toBe(c.bytes);
  });

  for (const c of v.axis_remap) it(`axis_remap ${c.name}`, () => {
    const a = unpackBno055AxisRemap(c.config, c.sign);
    expect({
      x: a.x, y: a.y, z: a.z, x_negative: a.xNegative, y_negative: a.yNegative, z_negative: a.zNegative,
    }).toEqual(c.expect);
    expect(packBno055AxisRemap(a)).toEqual(c.repack);
    expect(bno055Placement(c.name)).toEqual(a);
  });

  for (const c of v.axis_remap_invalid) it(`axis_remap_invalid ${c.name}`, () => {
    expect(() =>
      packBno055AxisRemap({ x: c.x, y: c.y, z: c.z, xNegative: false, yNegative: false, zNegative: false }),
    ).toThrow(RangeError);
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
  });

  for (const c of v.blocks) it(`block ${c.name}`, () => {
    const b = decodeBno055Block(c.addr, fromHex(c.data));
    expect({
      accel: b.accel, mag: b.mag, gyro: b.gyro, euler: b.euler, quaternion: b.quaternion,
      linear_accel: b.linearAccel, gravity: b.gravity, temperature: b.temperature, calib_stat: b.calibStat,
    }).toEqual(c.expect);
  });
});
