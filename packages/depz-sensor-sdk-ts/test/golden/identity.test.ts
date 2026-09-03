import { describe, expect, it } from "vitest";
import { parseSoftwareName, stripDeviceString } from "../../src/index.js";
import { fromHex, loadVectors } from "./vectors.js";

const data = loadVectors("identity.json");

describe("identity vectors", () => {
  for (const c of data.cases) {
    it(c.name, () => {
      const ident = parseSoftwareName(stripDeviceString(fromHex(c.raw)));
      expect(ident.mode).toBe(c.expect.mode);
      expect(ident.sensorType).toBe(c.expect.sensor_type);
      expect(ident.softwareName).toBe(c.expect.software_name);
      expect(ident.version).toBe(c.expect.version);
    });
  }
});
