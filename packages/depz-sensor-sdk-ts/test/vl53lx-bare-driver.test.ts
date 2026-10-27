/**
 * The histogram driver (uld/bare/driver.ts), its register image
 * (uld/bare/image.ts, uld/bare/regs.ts), the package barrel (uld/bare/index.ts)
 * and the driver contract (uld/base.ts) over the fake bridge, no hardware. The
 * scenario captures cover init, mode, timing and the streamed frames; this
 * covers the paths they never take — the boot timeout, polled reads, the
 * contract methods the session does not call, multi-block transfers and the
 * read-only refusals. Mirror of the Python `tests/test_vl53lx_bare_driver.py`.
 */

import { afterEach, describe, expect, it } from "vitest";
import * as bare from "../src/sensors/vl53lx/uld/bare/index.js";
import { BLOCK_NAMES, BLOCKS, RegBlock, type Block, type BlockName } from "../src/sensors/vl53lx/uld/bare/regs.js";
import { VL53LX } from "../src/sensors/vl53lx/uld/bare/driver.js";
import { DeviceImage } from "../src/sensors/vl53lx/uld/bare/image.js";
import { InvalidStateError } from "../src/errors.js";
import { BridgePlatform, SensorDriver, measurement, type Measurement } from "../src/sensors/vl53lx/uld/base.js";
import type { BridgeDevice } from "../src/sensors/vl53lx/uld/link.js";
import { Vl53lx } from "../src/sensors/vl53lx/vl53lx.js";
import { DIE_MODEL_ID, DIE_OSC, FakeVl53lxBridge } from "./fake-vl53lx-bridge.js";
import { fromHex, loadVectors } from "./golden/vectors.js";

const L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523";
const READ_REG = 0x32;
const FIRMWARE__SYSTEM_STATUS = 0x00e5;
const SYSTEM__INTERRUPT_CLEAR = 0x0086;
const HISTOGRAM_BIN_DATA_I2C_INDEX = 0x0088; // the 83-byte frame starts one before the result block

const toHex = (b: Uint8Array) => Buffer.from(b).toString("hex");

interface HistVector {
  name: string;
  raw: string;
  expect: { stream_count: number; dss_actual_effective_spads: number };
}

function histogramVector(name: string): [Uint8Array, HistVector["expect"]] {
  const v = loadVectors<{ histogram_raw: HistVector[] }>("vl53lx.json");
  const c = v.histogram_raw.find((x) => x.name === name);
  if (c === undefined) throw new Error(name);
  return [fromHex(c.raw), c.expect];
}

const open: Array<{ dev: Vl53lx; fake: FakeVl53lxBridge }> = [];
afterEach(async () => {
  for (const { dev, fake } of open.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function l4cx(): Promise<{ dev: Vl53lx; fake: FakeVl53lxBridge; drv: VL53LX }> {
  const fake = new FakeVl53lxBridge({ deviceName: L4CX_BOARD });
  const dev = new Vl53lx(fake.transport, { sleepImpl: () => Promise.resolve(), timeoutMs: 500 });
  await dev.open();
  open.push({ dev, fake });
  await dev.init();
  fake.rx.length = 0;
  return { dev, fake, drv: dev.driver as VL53LX };
}

// ── uld/bare/driver.ts ───────────────────────────────────────────────────────

describe("VL53LX histogram driver", () => {
  it("identity, mode and driver info come off the image", async () => {
    const { dev, fake, drv } = await l4cx();
    expect(await drv.modelId()).toBe(DIE_MODEL_ID);
    expect(toHex(fake.sent(READ_REG).slice(-1)[0]!)).toBe("0f010200");
    expect(await dev.getMode()).toBe("medium");
    // a 33 ms budget: (33000 − 1700) / 6 = 5216 us of range timeout, and the
    // MM1/MM2 offset the fake's unprogrammed NVM gives
    expect(await drv.driverInfo()).toEqual({ "range timeout us": 5216, "MM1/MM2 offset mm": 0 });
    expect(await dev.getRangeTiming()).toEqual([32, 0]); // 5216 × 6 + 1700 = 32996 us, floored
  });

  it("before sensorInit the class's modes and budget range apply", () => {
    const drv = new VL53LX(new BridgePlatform({} as BridgeDevice), "VL53L4CX");
    expect(drv.MODES).toEqual(["short", "medium", "long"]);
    expect(drv.BUDGET_MS).toEqual([2, 550]);
    expect(drv.HISTOGRAM).toBe(true);
    expect(drv.streamBlock()).toEqual([HISTOGRAM_BIN_DATA_I2C_INDEX, 83]);
  });

  it("a die that never boots times out", async () => {
    const { dev, fake, drv } = await l4cx();
    fake.regs[FIRMWARE__SYSTEM_STATUS] = 0x00;
    await expect(drv.waitBoot(0.02)).rejects.toThrow(/FIRMWARE__SYSTEM_STATUS == 0x03/);
    await expect(dev.configure({ budgetMs: 50 })).rejects.toThrow(/FIRMWARE__SYSTEM_STATUS == 0x03/);
  });

  it("a budget outside the die's range is refused before any traffic", async () => {
    const { dev, fake, drv } = await l4cx();
    expect(drv.BUDGET_MS).toEqual([2, 200]); // an L4 die: FDA_MAX 200 ms
    expect(drv.MODES).toEqual(["medium", "long"]); // ... and no short mode
    await expect(drv.setRangeTiming(201, 0)).rejects.toThrow(/timing_budget_ms must be 2..200/);
    await expect(drv.setRangeTiming(1, 0)).rejects.toThrow(/timing_budget_ms must be 2..200/);
    await expect(drv.setMode("short")).rejects.toThrow(/no such mode: short \(have medium, long\)/);
    expect(fake.rx).toEqual([]);
    expect(await dev.getRangeTiming()).toEqual([32, 0]); // unchanged
  });

  it("decode is binData then toMeasurement on the handed-in frame", async () => {
    const { fake, drv } = await l4cx();
    const [raw, expected] = histogramVector("vl53l4cx_histogram_medium_50ms_f2");
    await drv.startRanging();
    fake.rx.length = 0;
    const m: Measurement = drv.decode(raw);
    expect(fake.sent(READ_REG)).toEqual([]); // nothing read: the frame was handed in
    expect(m.extra.stream_count).toBe(expected.stream_count);
    expect(expected.stream_count).toBe(1);
    expect(m.targets.length).toBeGreaterThan(0);
    expect(m.distanceMm).toBe(m.targets[0]!.distanceMm);
    expect(m.status).toBe(m.targets[0]!.status);
    for (const t of m.targets) expect(t.minRangeMm).toBeLessThanOrEqual(t.maxRangeMm);
  });

  it("readMeasurement reads the 83-byte block itself", async () => {
    const { fake, drv } = await l4cx();
    const [raw, expected] = histogramVector("vl53l4cx_histogram_medium_50ms_f2");
    fake.regs.set(raw, HISTOGRAM_BIN_DATA_I2C_INDEX);
    await drv.startRanging();
    fake.rx.length = 0;
    const m = await drv.readMeasurement();
    expect(fake.sent(READ_REG).map(toHex)).toEqual(["88005300"]);
    expect(m.extra.stream_count).toBe(expected.stream_count);
    expect(m.spads).toBe(expected.dss_actual_effective_spads >> 8);
  });

  it("measureOnce polls data-ready and clears the interrupt with one byte", async () => {
    const { dev, fake } = await l4cx();
    const [raw] = histogramVector("vl53l4cx_histogram_medium_50ms_f2");
    fake.regs.set(raw, HISTOGRAM_BIN_DATA_I2C_INDEX);
    const m = await dev.measureOnce(500);
    expect(m.bins).not.toBeNull();
    expect((m.bins as { result__stream_count: number }).result__stream_count).toBe(1);
    // data-ready: polarity byte, then the GPIO status, both reads
    const reads = fake.sent(READ_REG).map(toHex);
    expect(reads).toContain("30000100");
    expect(reads).toContain("31000100");
    // the one-byte clear, then the stop: system_control pushed as a block
    // (abort in system__mode_start), then the patch unloaded with the
    // firmware stopped and restarted
    const writes = fake.writes();
    const i = writes.findIndex(([a, d]) => a === SYSTEM__INTERRUPT_CLEAR && d.length === 1 && d[0] === 0x01);
    expect(i).toBeGreaterThan(-1);
    expect(writes[i + 1]).toEqual([0x0083, [0x00, 0x00, 0x01, 0x01, 0x86]]); // mode_start 0x86 = abort
    expect(writes.slice(i + 2)).toEqual([
      [0x0085, [0x00]],
      [0x0083, [0x00]],
      [0x0470, [0x00]],
      [0x0085, [0x01]],
    ]); // unloadPatch
  });

  it("data ready follows the polarity bit", async () => {
    const { fake, drv } = await l4cx();
    fake.regs[0x0030] = 0x11; // active low: ready when GPIO reads 0
    fake.regs[0x0031] = 0x02;
    expect(await drv.checkForDataReady()).toBe(true);
    fake.regs[0x0031] = 0x03;
    expect(await drv.checkForDataReady()).toBe(false);
    fake.regs[0x0030] = 0x01; // active high: the other way round
    expect(await drv.checkForDataReady()).toBe(true);
    fake.regs[0x0031] = 0x02;
    await expect(drv.waitDataReady(0.02)).rejects.toThrow(/timeout waiting for data ready/);
    fake.rx.length = 0;
    await drv.clearInterrupt();
    expect(fake.writes()).toEqual([[SYSTEM__INTERRUPT_CLEAR, [0x01]]]);
  });
});

// ── uld/bare/image.ts ────────────────────────────────────────────────────────

describe("DeviceImage", () => {
  it("pullRange is one read spanning the blocks", async () => {
    const { fake, drv } = await l4cx();
    const img = drv.bare.img;
    fake.setWord(0x0018, 0xfffe); // algo__crosstalk_compensation_x_plane_gradient_kcps
    fake.regs[0x001e] = 0xe0; // algo__part_to_part_range_offset_mm, high byte
    fake.regs[0x001f] = 0x04; // → 0xE004 & 0x1FFF = 0x0004
    fake.rx.length = 0;
    await img.pullRange(["static_nvm_managed", "customer_nvm_managed"]);
    // 0x0001..0x0023: static_nvm (11) + the reserved 0x000C + customer_nvm (23)
    expect(fake.sent(READ_REG).map(toHex)).toEqual(["01002300"]);
    expect(img.static_nvm_managed.v.osc_measured__fast_osc__frequency).toBe(DIE_OSC);
    expect(img.customer_nvm_managed.v.algo__crosstalk_compensation_x_plane_gradient_kcps).toBe(-2);
    expect(img.customer_nvm_managed.v.algo__part_to_part_range_offset_mm).toBe(4);
  });

  it("pushRange goes out as one write with the reserved byte zeroed", async () => {
    const { fake, drv } = await l4cx();
    const img = drv.bare.img;
    img.static_nvm_managed.v.vhv_config__init = 0x5a;
    img.customer_nvm_managed.v.mm_config__outer_offset_mm = -1;
    fake.rx.length = 0;
    await img.pushRange(["static_nvm_managed", "customer_nvm_managed"]);
    const writes = fake.writes();
    expect(writes).toHaveLength(1);
    const [addr, data] = writes[0]!;
    expect([addr, data.length]).toEqual([0x0001, 35]);
    expect(data[10]).toBe(0x5a); // last byte of static_nvm
    expect(data[11]).toBe(0x00); // the hole at 0x000C
    expect(data.slice(33, 35)).toEqual([0xff, 0xff]);
    expect(fake.word(0x0022)).toBe(0xffff);
  });

  it("pull and push move one block", async () => {
    const { fake, drv } = await l4cx();
    const img = drv.bare.img;
    fake.regs[0x0087] = 0x42; // system__mode_start, the last byte of system_control
    fake.rx.length = 0;
    const blk = await img.pull("system_control");
    expect(blk).toBe(img.system_control);
    expect(blk.v.system__mode_start).toBe(0x42);
    expect(fake.sent(READ_REG).map(toHex)).toEqual(["83000500"]);
    img.system_control.v.system__mode_start = 0x40;
    await img.push("system_control");
    // the block as pulled, with the one edited field
    expect(fake.writes()).toEqual([[0x0083, [...fake.regs.slice(0x0083, 0x0087), 0x40]]]);
  });

  it("every block is reachable under its C name", () => {
    const img = new DeviceImage({} as BridgePlatform);
    const named = BLOCK_NAMES.slice(0, BLOCK_NAMES.indexOf("nvm_copy_data") + 1);
    expect(named).toHaveLength(11);
    for (const name of named) {
      expect((img as unknown as Record<string, unknown>)[name]).toBe(img.blocks[name]);
    }
    expect(DeviceImage.span(["system_control", "system_results"])).toEqual([
      BLOCKS.system_control.base,
      BLOCKS.system_results.base + BLOCKS.system_results.size - BLOCKS.system_control.base,
    ]);
  });

  it("dump lists every field of the named blocks", async () => {
    const { drv } = await l4cx();
    const img = drv.bare.img;
    const lines = img.dump(["system_control"]).split("\n");
    expect(lines[0]).toBe("--- system_control @ 0x0083 (5 B)");
    expect(lines).toHaveLength(1 + BLOCKS.system_control.fields.length);
    expect(lines[lines.length - 1]!.split(/\s+/).filter(Boolean)).toEqual([
      "system__mode_start",
      String(img.system_control.v.system__mode_start),
    ]);
    // default: all blocks, in BLOCKS order
    const headers = img.dump().split("\n").filter((l) => l.startsWith("--- "));
    expect(headers.map((h) => h.split(" ")[1])).toEqual([...BLOCK_NAMES]);
  });

  it("a read-only block is refused before the wire", async () => {
    const noWire = {
      wrMulti: async () => {
        throw new Error("must not write");
      },
    } as unknown as BridgePlatform;
    // Every block of ST's table is writable; a synthetic read-only one
    // exercises the guard (the Python test monkeypatches the same).
    const table = BLOCKS as unknown as Record<string, Block>;
    table.ro_test = { base: 0x0f50, size: 1, writable: false, fields: [{ name: "x", off: 0, width: 1, signed: false, mask: null }] };
    try {
      const img = new DeviceImage(noWire);
      (img.blocks as Record<string, RegBlock>).ro_test = new RegBlock("ro_test" as BlockName);
      await expect(img.push("ro_test" as BlockName)).rejects.toThrow(InvalidStateError);
      await expect(img.pushRange(["system_control", "ro_test" as BlockName])).rejects.toThrow(
        /ro_test is read-only/,
      );
    } finally {
      delete table.ro_test;
    }
  });
});

// ── uld/bare/index.ts ────────────────────────────────────────────────────────

describe("uld/bare barrel", () => {
  it("re-exports the package surface", () => {
    expect(bare.VL53LX).toBe(VL53LX);
    expect(bare.DeviceImage).toBe(DeviceImage);
    expect(bare.BLOCKS).toBe(BLOCKS);
    for (const name of ["NvmReader", "RegBlock", "TUNING", "DEFAULTS", "BareDriver", "HistogramBinData", "processData", "toMeasurement", "FrameHistory"] as const) {
      expect(bare[name]).toBeDefined();
    }
  });
});

// ── uld/base.ts ──────────────────────────────────────────────────────────────

describe("SensorDriver contract", () => {
  class Blocky extends SensorDriver {
    async modelId() {
      return 0;
    }
    async sensorInit() {}
    async startRanging() {}
    async stopRanging() {}
    ready = false;
    async checkForDataReady() {
      return this.ready;
    }
    async clearInterrupt() {}
    streamBlock(): [number, number] {
      return [0x0089, 2];
    }
    decode(raw: Uint8Array): Measurement {
      return measurement({ distanceMm: (raw[0]! << 8) | raw[1]! });
    }
    async setRangeTiming() {}
    async getRangeTiming(): Promise<[number, number]> {
      return [50, 0];
    }
  }

  it("the default readMeasurement reads the stream block and decodes it", async () => {
    const dev = {
      readReg: async (addr: number, size: number) => {
        expect([addr, size]).toEqual([0x0089, 2]);
        return Uint8Array.of(0x02, 0x64);
      },
    } as unknown as BridgeDevice;
    const drv = new Blocky(new BridgePlatform(dev), "X");
    expect((await drv.readMeasurement()).distanceMm).toBe(612);
    expect(drv.product).toBe("X");
    expect(drv.budgetChoices()).toEqual([]);
    expect(await drv.reachMm()).toBeNull();
    expect(await drv.driverInfo()).toEqual({});
    expect(drv.SUPPORTS.size).toBe(0);
    expect(drv.MODES).toEqual([]);
    expect(drv.HISTOGRAM).toBe(false);
    await expect(drv.setMode("long")).rejects.toThrow(/X: no ranging modes/);
    await expect(drv.getMode()).rejects.toThrow(/X: no ranging modes/);
  });

  it("waitDataReady times out through the platform sleep", async () => {
    const slept: number[] = [];
    const p = new BridgePlatform({} as BridgeDevice, async (ms) => {
      slept.push(ms);
    });
    const drv = new Blocky(p, "X");
    await expect(drv.waitDataReady(0.01)).rejects.toThrow(/timeout waiting for data ready/);
    expect(slept.length).toBeGreaterThan(0);
    expect(new Set(slept)).toEqual(new Set([1]));
    drv.ready = true;
    await drv.waitDataReady(0.01);
  });
});
