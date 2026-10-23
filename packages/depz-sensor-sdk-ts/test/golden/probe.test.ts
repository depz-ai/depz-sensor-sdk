/**
 * Golden consumer for `probe.json`: class resolution, contract 02 §4.4. TS is
 * the reference of `resolveSensorClass`; the vectors are generated through the
 * Python port and must agree with it case for case.
 */

import { describe, expect, it } from "vitest";
import { SENSOR_CLASS_NAMES, resolveSensorClass, sensorClass } from "../../src/index.js";
import { loadVectors, seen } from "./vectors.js";

interface ResolveCase {
  id: string;
  name: string;
  software_name: string;
  device_name: string;
  usb_vid: number | null;
  usb_pid: number | null;
  class: string;
}

const v = loadVectors<{ classes: string[]; resolve: ResolveCase[] }>("probe.json");

describe("probe.json", () => {
  it("class names", () => {
    expect([...SENSOR_CLASS_NAMES]).toEqual(v.classes);
    for (const name of SENSOR_CLASS_NAMES) {
      if (name !== "bootloader") expect(typeof sensorClass(name)).toBe("function");
    }
    seen("probe/classes");
  });

  for (const c of v.resolve) {
    it(`resolve ${c.name}`, () => {
      expect(resolveSensorClass(c.software_name, c.device_name, c.usb_vid, c.usb_pid)).toBe(c.class);
      seen(c);
    });
  }
});
