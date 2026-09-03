import { describe, expect, it } from "vitest";
import { crc8Maxim, crc16Modbus, crc32IsoHdlc, crc16CcittFalse } from "../../src/index.js";
import { fromHex, loadVectors } from "./vectors.js";

const data = loadVectors("crc.json");

describe("crc vectors", () => {
  for (const c of data.cases) {
    it(c.name, () => {
      const raw = fromHex(c.input);
      expect(crc8Maxim(raw)).toBe(c.crc8_maxim);
      expect(crc16Modbus(raw)).toBe(c.crc16_modbus);
      expect(crc32IsoHdlc(raw)).toBe(c.crc32_iso_hdlc);
      expect(crc16CcittFalse(raw)).toBe(c.crc16_ccitt_false);
    });
  }
});
