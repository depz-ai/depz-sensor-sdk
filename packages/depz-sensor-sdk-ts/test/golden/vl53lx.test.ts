/**
 * Golden-vector consumer: vl53lx.json (contract 12). Mirror of the Python
 * `tests/test_vectors_vl53lx.py`.
 */

import { describe, expect, it } from "vitest";
import {
  VL53LX_INFO_SIZE,
  packVl53lxSetAddrWidth,
  packVl53lxStartStream,
  unpackVl53lxInfo,
} from "../../src/protocol/vl53lx.js";
import {
  packReadReg,
  packSetI2cSpeed,
  packWriteReg,
  packXshut,
} from "../../src/protocol/vl53l4.js";
import { decodeDieBlock, decodeHistogramRaw, decodeL0xRaw } from "../../src/sensors/vl53lx/decode.js";
import * as registry from "../../src/sensors/vl53lx/uld/registry.js";
import { resolveVl53lxClass } from "../../src/sensors/vl53lx/vl53lx.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const v = loadVectors<any>("vl53lx.json");

/** The Bare Driver (uld/bare/driver.ts) fills the histogram rows once it lands. */
const HISTOGRAM_READY = registry.TABLE.VL53L4CX!.drivers.histogram !== undefined;

function snake(o: Record<string, unknown>): Record<string, unknown> {
  const out: Record<string, unknown> = {};
  for (const [k, val] of Object.entries(o)) {
    out[k.replace(/[A-Z]/g, (c) => "_" + c.toLowerCase()).replace(/(\d+)/g, "_$1")] = val;
  }
  return out;
}

describe("vl53lx.json", () => {
  for (const c of v.encode) {
    it(`encode ${c.name}`, () => {
      let got: Uint8Array;
      if (c.kind === "set_addr_width") got = packVl53lxSetAddrWidth(c.width);
      else if (c.kind === "read_reg") got = packReadReg(c.addr, c.len);
      else if (c.kind === "write_reg") got = packWriteReg(c.addr, fromHex(c.data));
      else if (c.kind === "xshut") got = packXshut(c.action);
      else if (c.kind === "set_i2c_speed") got = packSetI2cSpeed(c.khz);
      else if (c.kind === "start_stream") {
        got = packVl53lxStartStream(
          c.addr,
          c.len,
          (c.clear as [number, number][]).map(([a, val]) => [a, val] as const),
          c.flags,
        );
      } else throw new Error(`unknown encode kind ${c.kind}`);
      expect(toHex(got)).toBe(c.payload);
    });
  }

  it("START_STREAM refuses five clear steps", () => {
    expect(() => packVl53lxStartStream(0x0089, 17, Array(5).fill([0x86, 1] as const))).toThrow();
  });

  it("SET_ADDR_WIDTH refuses 3", () => {
    expect(() => packVl53lxSetAddrWidth(3)).toThrow();
  });

  for (const c of v.decode) {
    it(`decode ${c.name}`, () => {
      expect(c.report).toBe(0x92);
      const got = unpackVl53lxInfo(fromHex(c.payload));
      expect({
        int_edges: got.intEdges,
        slots_skipped: got.slotsSkipped,
        i2c_errors: got.i2cErrors,
        last_i2c_error: got.lastI2cError,
        xshut_level: got.xshutLevel,
        int_level: got.intLevel,
        i2c_khz: got.i2cKhz,
        addr_width: got.addrWidth,
        n_clear: got.nClear,
        frames_dropped: got.framesDropped,
      }).toEqual(c.expect);
    });
  }

  it("RPT_VL53_INFO rejects a short payload", () => {
    expect(() => unpackVl53lxInfo(new Uint8Array(VL53LX_INFO_SIZE - 1))).toThrow();
  });

  it("product order", () => {
    expect(v.products.map((r: any) => r.product)).toEqual([...registry.PRODUCTS]);
  });

  for (const r of v.products) {
    const needsHist = (r.driver_kinds as string[]).includes("histogram");
    it(`product ${r.product}: model id, reach, light driver`, () => {
      const row = registry.TABLE[r.product]!;
      expect(row.modelId).toBe(r.model_id);
      expect(row.reachMm).toBe(r.reach_mm);
      const light = (r.driver_kinds as string[]).filter((k) => k !== "histogram");
      expect(registry.driverKinds(r.product).filter((k) => k !== "histogram")).toEqual(light);
      if (r.default_driver !== "histogram") {
        const drv = row.drivers[r.default_driver]!;
        expect([drv.ADDR_WIDTH, drv.MAX_KHZ]).toEqual([r.addr_width, r.max_khz]);
        expect(drv.CLEAR_STEPS.map((s) => [...s])).toEqual(r.clear_steps);
      }
    });
    it.skipIf(needsHist && !HISTOGRAM_READY)(`product ${r.product}: full row`, () => {
      const row = registry.TABLE[r.product]!;
      expect(registry.driverKinds(r.product)).toEqual(r.driver_kinds);
      const drv = row.drivers[r.default_driver]!;
      expect([drv.ADDR_WIDTH, drv.MAX_KHZ]).toEqual([r.addr_width, r.max_khz]);
      expect(drv.CLEAR_STEPS.map((s) => [...s])).toEqual(r.clear_steps);
    });
  }

  for (const c of v.model) {
    it(`model ${c.name}`, () => {
      expect(resolveVl53lxClass(c.usb_model, c.device_name).name).toBe(c.expect_class);
      expect(registry.productFromBoardName(c.device_name)).toBe(c.expect_product);
    });
  }

  for (const c of v.die_block) {
    it(`die_block ${c.name}`, () => {
      expect(snake({ ...decodeDieBlock(fromHex(c.raw), c.variant) })).toEqual(c.expect);
    });
  }

  for (const c of v.l0x_raw) {
    it(`l0x_raw ${c.name}`, () => {
      const got = decodeL0xRaw(fromHex(c.raw));
      expect({
        distance_raw: got.distanceRaw,
        device_range_status: got.deviceRangeStatus,
        signal_rate_mcps_1616: got.signalRateMcps1616,
        ambient_rate_mcps_1616: got.ambientRateMcps1616,
        effective_spad_count_88: got.effectiveSpadCount88,
      }).toEqual(c.expect);
    });
  }

  for (const c of v.histogram_raw) {
    it(`histogram_raw ${c.name}`, () => {
      expect(snake({ ...decodeHistogramRaw(fromHex(c.raw)) })).toEqual(c.expect);
    });
  }
});
