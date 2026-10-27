/**
 * Bno055 device behaviour against an in-process fake register bridge +
 * register-level sensor model (test/fake-bno055.ts), checked against
 * contracts/13_SENSOR_BNO055.md and vectors/bno055.json.
 */

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  BNO055_TRIGGER_INT,
  Bno055,
  Bno055OprMode,
  Bno055PwrMode,
  Bno055TempSource,
  DepzError,
  DepzTimeoutError,
  InvalidStateError,
  StatusError,
  type Bno055Sample,
  type Bno055Units,
} from "../src/index.js";
import { fromHex, loadVectors, toHex } from "./golden/vectors.js";
import { FakeBno055 } from "./fake-bno055.js";

const V = loadVectors<any>("bno055.json");
const vec = (section: string, name: string) => V[section].find((c: any) => c.name === name);
const FULL_BLOCK_HEX: string = vec("blocks", "full_block_live").data;

const SI: Bno055Units = { accelMg: false, gyroRps: false, eulerRad: false, tempF: false, android: false };
const ALL_UNITS: Bno055Units = { accelMg: true, gyroRps: true, eulerRad: true, tempF: true, android: true };

let fake: FakeBno055;
let dev: Bno055;
let sleeps: number[];
let clock: number;

beforeEach(async () => {
  fake = new FakeBno055();
  sleeps = [];
  clock = 0;
  dev = new Bno055(fake.transport, {
    timeoutMs: 300,
    sleepImpl: async (ms) => {
      sleeps.push(ms);
      clock += ms;
    },
  });
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fake.close();
  vi.restoreAllMocks();
});

/** Make Date.now follow the injected sleeps (deadline loops without real waiting). */
function virtualClock(): void {
  vi.spyOn(Date, "now").mockImplementation(() => clock);
}

const W = (page: number, addr: number, hex: string) =>
  `${page}:${addr.toString(16).padStart(2, "0")}=${hex}`;

async function flush(): Promise<void> {
  for (let i = 0; i < 5; i++) await new Promise((r) => setTimeout(r, 1));
}

// ── identity ─────────────────────────────────────────────────────────────────

describe("identity & bridge info", () => {
  it("bridgeInfo decodes RPT_BNO_INFO and sends GET_INFO with no payload", async () => {
    const info = await dev.bridgeInfo();
    const exp = vec("decode", "info_live_board").expect;
    expect(info.chipId).toBe(exp.chip_id);
    expect(info.swRev).toBe(exp.sw_rev);
    expect(info.readAvgUs).toBe(exp.read_avg_us);
    expect(info.loopMaxUs).toBe(exp.loop_max_us);
    expect(fake.sent.at(-1)).toEqual({ cmd: 0x37, payload: new Uint8Array(0) });
  });

  it("isAlive: true on a healthy board, false when the handshake never passed", async () => {
    expect(await dev.isAlive()).toBe(true);
    fake.info = fromHex(vec("decode", "info_recovering").payload); // initialized = 0
    expect(await dev.isAlive()).toBe(false);
  });

  it("isAlive: false on wrong chip ids", async () => {
    fake.info[1] = 0xa1; // chip id
    expect(await dev.isAlive()).toBe(false);
  });

  it("isAlive: false (not a throw) when GET_INFO fails or times out", async () => {
    fake.failNext.set(0x37, [0x09]);
    expect(await dev.isAlive()).toBe(false);
    fake.silent.add(0x37);
    expect(await dev.isAlive()).toBe(false);
  });
});

// ── raw register access ──────────────────────────────────────────────────────

describe("raw register access", () => {
  it("READ_REG wire bytes and data", async () => {
    const ids = await dev.readRegisters(0x00, 7);
    expect(toHex(ids)).toBe(vec("decode", "reg_data_chip_ids").expect.data);
    expect(toHex(fake.sent.at(-1)!.payload)).toBe(vec("encode", "read_reg_chip_ids").payload);
    expect(await dev.readRegister(0x00)).toBe(0xa0);
  });

  it("splits reads and writes over 128 bytes into ≤128-byte transfers", async () => {
    fake.regs[0].fill(0x5a, 0x80, 0x100);
    const data = await dev.readRegisters(0x40, 0xc0);
    expect(data.length).toBe(0xc0);
    const reads = fake.sent.filter((c) => c.cmd === 0x32).map((c) => toHex(c.payload));
    expect(reads).toEqual(["4080", "c040"]);
    expect(data[0x40]).toBe(0x5a);

    const blob = Uint8Array.from({ length: 130 }, (_, i) => i);
    await dev.writeRegisters(0x70, blob);
    const ws = fake.sent.filter((c) => c.cmd === 0x33);
    expect(ws.map((c) => [c.payload[0], c.payload.length - 1])).toEqual([
      [0x70, 128],
      [0xf0, 2],
    ]);
  });

  it("page 1 access is wrapped in PAGE_ID 1 … 0", async () => {
    const id = await dev.readRegisters(0x50, 16, 1);
    expect(toHex(id)).toBe("c0c1c2c3c4c5c6c7c8c9cacbcccdcecf");
    expect(fake.writes()).toEqual([W(0, 0x07, "01"), W(1, 0x07, "00")]);
    expect(fake.ops.find((o) => o.op === "r")).toMatchObject({ page: 1, addr: 0x50 });
    expect(fake.page).toBe(0);
  });

  it("page 1 goes back to page 0 even when the access fails", async () => {
    fake.failNext.set(0x32, [0x09]);
    await expect(dev.readRegisters(0x50, 1, 1)).rejects.toBeInstanceOf(StatusError);
    expect(fake.page).toBe(0);
    expect(fake.writes().at(-1)).toBe(W(1, 0x07, "00"));
  });

  it("refuses pages other than 0 and 1", async () => {
    await expect(dev.readRegisters(0x00, 1, 2)).rejects.toThrow(RangeError);
    expect(fake.sent.length).toBe(0);
  });

  it("a device error status rejects with StatusError and does not wedge the lock", async () => {
    fake.failNext.set(0x32, [0x04]);
    const err = await dev.readRegisters(0x00, 1).catch((e: unknown) => e);
    expect(err).toBeInstanceOf(StatusError);
    expect((err as StatusError).status).toBe(0x04);
    expect(await dev.readRegister(0x00)).toBe(0xa0);
  });

  it("a short RPT_BNO_REG_DATA is an error, not silently padded", async () => {
    fake.shortReadBy = 1;
    await expect(dev.readRegisters(0x00, 7)).rejects.toThrow(/expected 7, got 6/);
  });

  it("a silent bridge times out", async () => {
    fake.silent.add(0x32);
    await expect(dev.readRegister(0x00)).rejects.toBeInstanceOf(DepzTimeoutError);
  });

  it("serialises concurrent multi-step sequences", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await Promise.all([dev.setUnits(ALL_UNITS), dev.readRegister(0x00)]);
    const kinds = fake.ops.map((o) =>
      o.op === "w" ? `w${o.addr.toString(16)}=${toHex(o.data)}` : `r${o.addr.toString(16)}`,
    );
    // the id read comes after the CONFIG round trip is complete, not inside it
    expect(kinds.indexOf("r0")).toBeGreaterThan(kinds.lastIndexOf("w3d=0c"));
  });
});

// ── modes ────────────────────────────────────────────────────────────────────

describe("operation mode", () => {
  it("masks OPR_MODE bits 7:4", async () => {
    fake.regs[0][0x3d] = 0x10;
    expect(await dev.getOperationMode()).toBe(Bno055OprMode.Config);
  });

  it("CONFIG → NDOF: one write, 7 ms+ wait, then waits for a non-zero quaternion", async () => {
    fake.fusionZeroReadsAfterSwitch = 3;
    await dev.setOperationMode(Bno055OprMode.Ndof);
    expect(fake.writes()).toEqual([W(0, 0x3d, "0c")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
    const quaReads = fake.ops.filter((o) => o.op === "r" && o.addr === 0x20 && o.data.length === 8);
    expect(quaReads.length).toBe(4); // 3 zero reads + the live one
    expect(sleeps).toEqual([10, 10, 10, 10]);
  });

  it("NDOF → AMG goes through CONFIG (a direct switch is ignored by the sensor)", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setOperationMode(Bno055OprMode.Amg);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3d, "07")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Amg);
    expect(sleeps).toEqual([25, 10]);
  });

  it("same mode: nothing written", async () => {
    fake.setMode(Bno055OprMode.Imu);
    await dev.setOperationMode(Bno055OprMode.Imu);
    expect(fake.writes()).toEqual([]);
  });

  it("to CONFIG: one write, 19 ms+ wait, no mode read", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setOperationMode(Bno055OprMode.Config);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00")]);
    expect(fake.ops.filter((o) => o.op === "r")).toEqual([]);
    expect(sleeps).toEqual([25]);
  });

  it("a plain mode switch gives up waiting for fusion after the deadline instead of failing", async () => {
    virtualClock();
    fake.fusionStuck = true;
    await dev.setOperationMode(Bno055OprMode.Ndof);
    expect(clock).toBeGreaterThan(1000);
  });
});

describe("power mode (§4.6)", () => {
  it("suspend from NDOF, then wake back into NDOF", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setPowerMode(Bno055PwrMode.Suspend);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3e, "02")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Config);
    fake.ops.length = 0;
    await dev.setPowerMode(Bno055PwrMode.Normal);
    expect(fake.writes()).toEqual([W(0, 0x3e, "00"), W(0, 0x3d, "0c")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
    expect(await dev.getPowerMode()).toBe(Bno055PwrMode.Normal);
  });

  it("suspend from AMG wakes back into AMG", async () => {
    fake.setMode(Bno055OprMode.Amg);
    await dev.setPowerMode(Bno055PwrMode.Suspend);
    fake.ops.length = 0;
    await dev.setPowerMode(Bno055PwrMode.LowPower);
    expect(fake.writes()).toEqual([W(0, 0x3e, "01"), W(0, 0x3d, "07")]);
  });

  it("a chip found asleep in CONFIG wakes into NDOF", async () => {
    fake.regs[0][0x3e] = Bno055PwrMode.Suspend;
    await dev.setPowerMode(Bno055PwrMode.Normal);
    expect(fake.writes()).toEqual([W(0, 0x3e, "00"), W(0, 0x3d, "0c")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
  });

  it("CONFIG and awake stays in CONFIG", async () => {
    await dev.setPowerMode(Bno055PwrMode.LowPower);
    expect(fake.writes()).toEqual([W(0, 0x3e, "01")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Config);
  });
});

describe("units, axes, temperature source", () => {
  it("power-on UNIT_SEL 0x80 reads as Android orientation", async () => {
    expect(await dev.getUnits()).toEqual({ ...SI, android: true });
  });

  it("setUnits writes UNIT_SEL inside a CONFIG round trip", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setUnits(ALL_UNITS);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3b, "97"), W(0, 0x3d, "0c")]);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
  });

  it("setUnits in CONFIG does not leave CONFIG", async () => {
    await dev.setUnits(SI);
    expect(fake.writes()).toEqual([W(0, 0x3b, "00")]);
  });

  it("the previous mode is restored even if the CONFIG-side write fails", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    fake.failWriteReg.set(0x3b, 0x09);
    await expect(dev.setUnits(SI)).rejects.toBeInstanceOf(StatusError);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
  });

  it("axis remap by placement name (vector encode write_reg_axis_map_p0)", async () => {
    await dev.setAxisRemap("p0");
    const w = fake.sent.filter((c) => c.cmd === 0x33).at(-1)!;
    expect(toHex(w.payload)).toBe(vec("encode", "write_reg_axis_map_p0").payload);
    const r = await dev.getAxisRemap();
    const exp = vec("axis_remap", "P0").expect;
    expect(r).toEqual({
      x: exp.x,
      y: exp.y,
      z: exp.z,
      xNegative: exp.x_negative,
      yNegative: exp.y_negative,
      zNegative: exp.z_negative,
    });
  });

  it("refuses a non-permutation remap and unknown placements before any traffic", async () => {
    for (const c of V.axis_remap_invalid) {
      await expect(
        dev.setAxisRemap({ x: c.x, y: c.y, z: c.z, xNegative: false, yNegative: false, zNegative: false }),
      ).rejects.toThrow(RangeError);
    }
    await expect(dev.setAxisRemap("P8")).rejects.toThrow(RangeError);
    expect(fake.sent.length).toBe(0);
  });

  it("temperature source", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setTemperatureSource(Bno055TempSource.Gyro);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x40, "01"), W(0, 0x3d, "0c")]);
    expect(await dev.getTemperatureSource()).toBe(Bno055TempSource.Gyro);
  });
});

// ── configure / reset ────────────────────────────────────────────────────────

const PROFILE_HEX: string = vec("calibration_profile", "extremes").bytes;
const profileOf = (name: string) => {
  const e = vec("calibration_profile", name).expect;
  return {
    accelOffset: e.accel_offset,
    magOffset: e.mag_offset,
    gyroOffset: e.gyro_offset,
    accelRadius: e.accel_radius,
    magRadius: e.mag_radius,
  };
};

describe("configure / restoreConfiguration / reset", () => {
  it("configure: boot wait → CONFIG → units → remap → profile (one 22-byte write) → NDOF", async () => {
    fake.sysStatusScript = [3, 4, 0]; // init, POST, idle
    await dev.configure({ units: ALL_UNITS, axisRemap: "P2", calibration: profileOf("extremes") });
    expect(fake.writes()).toEqual([
      W(0, 0x3d, "00"),
      W(0, 0x3b, "97"),
      W(0, 0x41, "2406"),
      W(0, 0x55, PROFILE_HEX),
      W(0, 0x3d, "0c"),
    ]);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
    // the units are cached: polling needs no UNIT_SEL read
    fake.ops.length = 0;
    const s = await dev.readSample([0x34, 1]);
    expect(s.units).toEqual(ALL_UNITS);
    expect(fake.ops.filter((o) => o.op === "r").map((o) => o.addr)).toEqual([0x34]);
  });

  it("configure with defaults writes UNIT_SEL 0 and touches neither remap nor profile", async () => {
    await dev.configure();
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3b, "00"), W(0, 0x3d, "0c")]);
  });

  it("configure into a non-fusion mode does not wait for the quaternion", async () => {
    await dev.configure({ mode: Bno055OprMode.Amg });
    expect(fake.ops.filter((o) => o.op === "r" && o.addr === 0x20)).toEqual([]);
    expect(fake.oprMode).toBe(Bno055OprMode.Amg);
  });

  it("configure fails when fusion never starts (quaternion stays zero)", async () => {
    virtualClock();
    fake.fusionStuck = true;
    await expect(dev.configure()).rejects.toThrow(/fusion did not start/);
  });

  it("configure fails when the sensor never leaves its boot states", async () => {
    virtualClock();
    fake.sysStatusScript = new Array(1000).fill(3);
    await expect(dev.configure()).rejects.toThrow(/did not finish booting/);
    expect(fake.writes()).toEqual([]);
  });

  it("a SYS_STATUS 4 lasting 20 polls is a self-test leftover: cleared via ACCONLY → CONFIG", async () => {
    fake.sysStatusScript = new Array(20).fill(4);
    await dev.configure({ mode: Bno055OprMode.Amg });
    expect(fake.writes().slice(0, 2)).toEqual([W(0, 0x3d, "01"), W(0, 0x3d, "00")]);
    const statusReads = fake.ops.filter((o) => o.op === "r" && o.addr === 0x39).length;
    expect(statusReads).toBe(21);
  });

  it("restoreConfiguration re-applies the last configure()", async () => {
    await expect(dev.restoreConfiguration()).rejects.toThrow(InvalidStateError);
    await dev.configure({ units: ALL_UNITS, axisRemap: "P2", calibration: profileOf("extremes") });
    const first = fake.writes();
    fake.ops.length = 0;
    await dev.resetSensor(); // back to power-on registers
    expect(fake.regs[0][0x3b]).toBe(0x80);
    fake.ops.length = 0;
    await dev.restoreConfiguration();
    expect(fake.writes()).toEqual(first);
    expect(fake.regs[0][0x3b]).toBe(0x97);
  });

  it("resetSensor sends BNO_RESET, waits out the boot tail, stops the stream and drops the units cache", async () => {
    await dev.setUnits(ALL_UNITS);
    await dev.startStream();
    fake.sysStatusScript = [2, 3, 0];
    await dev.resetSensor();
    expect(fake.sent.some((c) => c.cmd === 0x34 && c.payload.length === 0)).toBe(true);
    expect(dev.streaming).toBe(false);
    expect(fake.sysStatusScript).toEqual([]);
    // cache dropped: power-on 0x80 is read back, not the pre-reset units
    const s = await dev.readSample([0x34, 1]);
    expect(s.units).toEqual({ ...SI, android: true });
  });

  it("resetSensor surfaces a reset failure", async () => {
    fake.failNext.set(0x34, [0x09]);
    await expect(dev.resetSensor()).rejects.toBeInstanceOf(StatusError);
  });
});

// ── status, self-test, calibration ───────────────────────────────────────────

describe("status & self-test", () => {
  it("systemStatus reads ST_RESULT and SYS_CLK_STATUS..SYS_ERR, never INT_STA", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    fake.regs[0][0x3a] = 9;
    const st = await dev.systemStatus();
    expect(st).toMatchObject({ selfTest: 0x0f, status: 5, error: 9, selfTestPassed: true });
    expect(st.errorText).toMatch(/fusion algorithm configuration/);
    const reads = fake.ops.filter((o) => o.op === "r").map((o) => [o.addr, o.data.length]);
    expect(reads).toEqual([
      [0x36, 1],
      [0x38, 3],
    ]);
  });

  it("selfTest from NDOF: CONFIG, SELF_TEST with CLK_SEL kept, 400 ms+, verdict, NDOF back", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    fake.regs[0][0x3f] = 0x80; // external clock selected
    fake.selfTestResult = 0x0d; // gyro failed
    fake.selfTestError = 3;
    const st = await dev.selfTest();
    expect(st.selfTestPassed).toBe(false);
    expect(st.selfTest).toBe(0x0d);
    expect(st.error).toBe(3);
    expect(st.status).toBe(4);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3f, "81"), W(0, 0x3d, "0c")]);
    expect(sleeps).toContain(450);
    expect(fake.oprMode).toBe(Bno055OprMode.Ndof);
  });

  it("selfTest in CONFIG clears the SYS_STATUS 4 leftover via ACCONLY", async () => {
    const st = await dev.selfTest();
    expect(st.selfTestPassed).toBe(true);
    expect(fake.writes()).toEqual([W(0, 0x3f, "01"), W(0, 0x3d, "01"), W(0, 0x3d, "00")]);
    expect((await dev.systemStatus()).status).toBe(0);
  });

  it("selfTest restores the mode when the trigger write fails", async () => {
    fake.setMode(Bno055OprMode.Imu);
    fake.failWriteReg.set(0x3f, 0x09);
    await expect(dev.selfTest()).rejects.toBeInstanceOf(StatusError);
    expect(fake.oprMode).toBe(Bno055OprMode.Imu);
  });

  it("selfTest is refused while streaming", async () => {
    await dev.startStream();
    await expect(dev.selfTest()).rejects.toThrow(InvalidStateError);
  });

  it("calibration status decodes CALIB_STAT", async () => {
    const c = V.calib_stat.find((x: any) => x.value === 228);
    fake.regs[0][0x35] = c.value;
    expect(await dev.calibrationStatus()).toEqual(c.expect);
  });

  it("readCalibrationProfile reads 22 bytes at 0x55 in CONFIG and returns to NDOF", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    fake.regs[0].set(fromHex(PROFILE_HEX), 0x55);
    expect(await dev.readCalibrationProfile()).toEqual(profileOf("extremes"));
    const r = fake.ops.find((o) => o.op === "r" && o.addr === 0x55)!;
    expect(r.data.length).toBe(22);
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x3d, "0c")]);
  });

  it("writeCalibrationProfile writes all 22 bytes in one transfer, in CONFIG", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    const live = vec("calibration_profile", "live_uncalibrated");
    await dev.writeCalibrationProfile(profileOf("live_uncalibrated"));
    expect(fake.writes()).toEqual([W(0, 0x3d, "00"), W(0, 0x55, live.bytes), W(0, 0x3d, "0c")]);
  });

  it("SIC matrix: identity by default, read back", async () => {
    await dev.setSicMatrix();
    expect(fake.writes()).toEqual([W(0, 0x43, "004000000000000000400000000000000040")]);
    expect(await dev.getSicMatrix()).toEqual([16384, 0, 0, 0, 16384, 0, 0, 0, 16384]);
  });
});

// ── page 1 ───────────────────────────────────────────────────────────────────

describe("page-1 configuration and interrupts", () => {
  it("accel config: CONFIG, page 1, write, page 0, mode back", async () => {
    fake.setMode(Bno055OprMode.Amg);
    const c = V.sensor_config.accel.find((x: any) => x.value === 191);
    await dev.setAccelConfig(c.expect);
    expect(fake.writes()).toEqual([
      W(0, 0x3d, "00"),
      W(0, 0x07, "01"),
      W(1, 0x08, "bf"),
      W(1, 0x07, "00"),
      W(0, 0x3d, "07"),
    ]);
    expect(await dev.getAccelConfig()).toEqual(c.expect);
  });

  it("gyro (2 bytes) and mag configs round-trip through page 1", async () => {
    const g = V.sensor_config.gyro.find((x: any) => x.bytes === "2c02");
    await dev.setGyroConfig(g.expect);
    expect(toHex(fake.regs[1].subarray(0x0a, 0x0c))).toBe("2c02");
    expect(await dev.getGyroConfig()).toEqual(g.expect);
    const m = V.sensor_config.mag.find((x: any) => x.value === 109);
    await dev.setMagConfig(m.expect);
    expect(fake.regs[1][0x09]).toBe(109);
    expect(await dev.getMagConfig()).toEqual(m.expect);
    expect(fake.page).toBe(0);
  });

  it("uniqueId is 16 bytes at page-1 0x50", async () => {
    expect(toHex(await dev.uniqueId())).toBe("c0c1c2c3c4c5c6c7c8c9cacbcccdcecf");
  });

  it("page-1 access is refused while a stream runs, with nothing sent", async () => {
    await dev.startStream();
    const before = fake.sent.length;
    await expect(dev.setAccelConfig({ range: 0, bandwidth: 0, power: 0 })).rejects.toThrow(/stopStream/);
    await expect(dev.uniqueId()).rejects.toThrow(/stopStream/);
    await expect(dev.getGyroConfig()).rejects.toThrow(/stopStream/);
    expect(fake.sent.length).toBe(before);
    expect(fake.page).toBe(0);
  });

  it("interrupt enable / mask are written on page 1 without a CONFIG round trip", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    await dev.setInterruptEnable(0x1c0);
    await dev.setInterruptMask(0x40);
    expect(fake.writes()).toEqual([
      W(0, 0x07, "01"),
      W(1, 0x10, "c0"),
      W(1, 0x07, "00"),
      W(0, 0x07, "01"),
      W(1, 0x0f, "40"),
      W(1, 0x07, "00"),
    ]);
    expect(await dev.getInterruptEnable()).toBe(0xc0);
    expect(await dev.getInterruptMask()).toBe(0x40);
  });

  it("setInterruptSetting accepts only page-1 0x11..0x1F", async () => {
    await expect(dev.setInterruptSetting(0x10, 1)).rejects.toThrow(RangeError);
    await expect(dev.setInterruptSetting(0x20, 1)).rejects.toThrow(RangeError);
    expect(fake.sent.length).toBe(0);
    await dev.setInterruptSetting(0x1f, 0x1ff);
    expect(fake.regs[1][0x1f]).toBe(0xff);
    await dev.setInterruptSetting(0x11, 7);
    expect(fake.regs[1][0x11]).toBe(7);
  });

  it("INT_STA clears on read; clearInterrupt sets RST_INT and keeps CLK_SEL", async () => {
    fake.regs[0][0x37] = 0x40;
    expect(await dev.readInterruptStatus()).toBe(0x40);
    expect(await dev.readInterruptStatus()).toBe(0);
    fake.regs[0][0x3f] = 0x80;
    await dev.clearInterrupt();
    expect(fake.writes()).toEqual([W(0, 0x3f, "c0")]);
    fake.ops.length = 0;
    fake.regs[0][0x3f] = 0;
    await dev.clearInterrupt();
    expect(fake.writes()).toEqual([W(0, 0x3f, "40")]);
  });
});

// ── data ─────────────────────────────────────────────────────────────────────

function expectScaledFull(s: Bno055Sample, units: Bno055Units): void {
  const e = vec("blocks", "full_block_live").expect;
  const accLsb = units.accelMg ? 1 : 100;
  const gyrLsb = units.gyroRps ? 900 : 16;
  const eulLsb = units.eulerRad ? 900 : 16;
  const tempLsb = units.tempF ? 0.5 : 1;
  const near = (got: number[] | null, raw: number[], k: number) =>
    expect(got).toEqual(raw.map((x) => x / k));
  near(s.accel, e.accel, accLsb);
  near(s.mag, e.mag, 16);
  near(s.gyro, e.gyro, gyrLsb);
  near(s.euler, e.euler, eulLsb);
  near(s.quaternion, e.quaternion, 16384);
  near(s.linearAccel, e.linear_accel, 100); // LIA/GRV ignore the mg bit
  near(s.gravity, e.gravity, 100);
  expect(s.temperature).toBe(e.temperature / tempLsb);
  expect(s.calibration).toEqual({ system: 0, gyro: 0, accel: 0, mag: 0 });
}

describe("polling", () => {
  it("readSample: full block, units read from UNIT_SEL once and cached", async () => {
    fake.regs[0][0x3b] = 0x00;
    fake.setMode(Bno055OprMode.Ndof);
    fake.regs[0].set(fromHex(FULL_BLOCK_HEX), 0x08);
    const s = await dev.readSample();
    expect(toHex(fake.sent.at(-1)!.payload)).toBe(vec("encode", "read_reg_full_block").payload);
    expect(s.addr).toBe(0x08);
    expect(toHex(s.raw)).toBe(FULL_BLOCK_HEX);
    expect(s.timestampUs).toBe(fake.mcuTimeUs);
    expectScaledFull(s, SI);
    await dev.readSample();
    expect(fake.ops.filter((o) => o.addr === 0x3b).length).toBe(1);
  });

  it("readSample scales by every unit bit", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    fake.regs[0].set(fromHex(FULL_BLOCK_HEX), 0x08);
    fake.regs[0][0x3b] = 0x97;
    expectScaledFull(await dev.readSample(), ALL_UNITS);
  });

  it("readQuaternion reads only 0x20+8", async () => {
    fake.setMode(Bno055OprMode.Ndof);
    const q = await dev.readQuaternion();
    expect(q).toEqual(vec("blocks", "quaternion_live").expect.quaternion.map((x: number) => x / 16384));
    expect(fake.ops.at(-1)).toMatchObject({ op: "r", addr: 0x20 });
    expect(fake.ops.at(-1)!.data.length).toBe(8);
  });
});

describe("streaming", () => {
  it("START_STREAM wire bytes (vectors) and stream state", async () => {
    expect(dev.streaming).toBe(false);
    await dev.startStream(10);
    expect(toHex(fake.streams.at(-1)!)).toBe(vec("encode", "start_stream_full_block_100hz").payload);
    expect(dev.streaming).toBe(true);
    await dev.startStream(20, [0x20, 8]);
    expect(toHex(fake.streams.at(-1)!)).toBe(vec("encode", "start_stream_quaternion_50hz").payload);
    await dev.startStream(1000, [0x35, 1], BNO055_TRIGGER_INT);
    expect(toHex(fake.streams.at(-1)!)).toBe(vec("encode", "start_stream_int_status_watchdog_1s").payload);
  });

  it("refuses blocks outside 1..128 or past 0xFF before any traffic", async () => {
    await expect(dev.startStream(10, [0x08, 0])).rejects.toThrow(RangeError);
    await expect(dev.startStream(10, [0x00, 129])).rejects.toThrow(RangeError);
    await expect(dev.startStream(10, [0xf0, 0x11])).rejects.toThrow(RangeError);
    expect(fake.sent.length).toBe(0);
  });

  it("refuses a TIMER period outside 1..60000 ms before any traffic (contract 13 §2, ERRATA A13)", async () => {
    for (const periodMs of [0, 60001, 65536]) {
      await expect(dev.startStream(periodMs)).rejects.toThrow(/periodMs/);
    }
    expect(fake.sent.length).toBe(0);
    expect(dev.streaming).toBe(false);
  });

  it("latches units before START_STREAM and scales samples by them, not later units", async () => {
    fake.regs[0][0x3b] = 0x97;
    await dev.startStream();
    const order = fake.sent.map((c) => (c.cmd === 0x32 ? `r${c.payload[0]!.toString(16)}` : c.cmd));
    expect(order).toEqual(["r3b", 0x35]);
    await dev.setUnits(SI); // allowed while streaming, but does not rescale this stream
    const got: Bno055Sample[] = [];
    dev.onSample((s) => got.push(s));
    fake.pushStream(0x08, fromHex(FULL_BLOCK_HEX), 123n);
    await flush();
    expect(got.length).toBe(1);
    expect(got[0]!.timestampUs).toBe(123n);
    expectScaledFull(got[0]!, ALL_UNITS);
  });

  it("a failed START_STREAM leaves the stream off", async () => {
    fake.failNext.set(0x35, [0x08]); // ERR_NOT_INITIALIZED
    await expect(dev.startStream()).rejects.toBeInstanceOf(StatusError);
    expect(dev.streaming).toBe(false);
  });

  it("stopStream: no-op when idle, sends STOP_STREAM, clears the flag even on failure", async () => {
    await dev.stopStream();
    expect(fake.sent.length).toBe(0);
    await dev.startStream();
    await dev.stopStream();
    expect(fake.sent.at(-1)).toEqual({ cmd: 0x36, payload: new Uint8Array(0) });
    expect(dev.streaming).toBe(false);
    await dev.startStream();
    fake.failNext.set(0x36, [0x09]);
    await expect(dev.stopStream()).rejects.toBeInstanceOf(StatusError);
    expect(dev.streaming).toBe(false);
  });

  it("callbacks, unsubscribe, async iterator and drop-oldest", async () => {
    await dev.startStream(20, [0x20, 8]);
    const cb: Bno055Sample[] = [];
    const off = dev.onSample((s) => cb.push(s));
    const q = dev.samples(2);
    for (let i = 0; i < 5; i++) fake.pushStream(0x20, fromHex("bd2963f9efcf0000"), BigInt(i));
    await flush();
    expect(cb.length).toBe(5);
    expect(cb[0]!.quaternion).not.toBeNull();
    expect(cb[0]!.accel).toBeNull();
    expect(dev.streamDroppedCounts).toEqual([3]);
    const a = await q.next();
    expect(a.done).toBe(false);
    expect((a.value as Bno055Sample).timestampUs).toBe(3n);
    off();
    await q.return();
    expect(dev.streamDroppedCounts).toEqual([]);
    fake.pushStream(0x20, fromHex("bd2963f9efcf0000"));
    await flush();
    expect(cb.length).toBe(5);
  });

  it("getSample: next sample, timeout, and device close", async () => {
    await dev.startStream();
    const p = dev.getSample(1000);
    fake.pushStream(0x34, fromHex("1a00"), 77n);
    const s = await p;
    expect(s.timestampUs).toBe(77n);
    expect(s.temperature).toBe(26);
    expect(dev.streamDroppedCounts).toEqual([]); // its one-shot queue is gone

    await expect(dev.getSample(20)).rejects.toBeInstanceOf(DepzTimeoutError);

    const pending = dev.getSample(1000);
    await dev.close();
    await expect(pending).rejects.toThrow(/device closed/);
  });

  it("malformed stream reports are counted and dropped; the link keeps working", async () => {
    await dev.startStream();
    const got: Bno055Sample[] = [];
    dev.onSample((s) => got.push(s));
    await fake.pushRaw(0x93, new Uint8Array(9)); // shorter than the 10-byte header
    // header says 46 bytes, only 6 arrived (a truncated block)
    fake.pushStream(0x08, fromHex("a2037fff73ff"), 1n, 46);
    await flush();
    expect(dev.streamParseErrors).toBe(2);
    expect(got.length).toBe(0);
    fake.pushStream(0x08, fromHex("a2037fff73ff"), 2n);
    await flush();
    expect(got.length).toBe(1);
    expect(got[0]!.accel).toEqual([9.3, -1.29, -1.41]);
    expect(await dev.readRegister(0x00)).toBe(0xa0);
  });

  it("other reports are not swallowed as stream data", async () => {
    const texts: string[] = [];
    dev.onEvent((e) => {
      if (e.type === "text") texts.push(e.text);
    });
    await fake.pushRaw(0x9e, Uint8Array.of(1, 2));
    await flush();
    expect(texts).toEqual(["0102"]);
    expect(dev.streamParseErrors).toBe(0);
  });
});
