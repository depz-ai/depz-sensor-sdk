import { describe, expect, it } from "vitest";
import { FwDepzError, fwDepzPayloadCrcOk, parseFwDepz } from "../../src/index.js";
import { fromHex, loadVectors } from "./vectors.js";

const data = loadVectors("fwdepz.json");

describe("fwdepz vectors", () => {
  for (const c of data.cases) {
    it(c.name, () => {
      const blob = fromHex(c.file);
      if (c.error) {
        expect(() => parseFwDepz(blob)).toThrow(FwDepzError);
        return;
      }
      const img = parseFwDepz(blob);
      expect(img.loadAddr).toBe(c.expect.load_addr);
      expect(img.fwSize).toBe(c.expect.fw_size);
      expect(img.fwCrc32).toBe(c.expect.fw_crc32);
      expect(img.curSec).toBe(c.expect.cur_sec);
      expect(img.totSec).toBe(c.expect.tot_sec);
      expect(fwDepzPayloadCrcOk(img)).toBe(c.expect.payload_crc_ok);
    });
  }
});
