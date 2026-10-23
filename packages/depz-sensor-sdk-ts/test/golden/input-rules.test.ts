/**
 * `input_rules` groups (contract 07 §3): which inputs the SDK rejects and that
 * trailing payload bytes are ignored. Each case is `{op, in, expect}` in the
 * differential-test format (contract 16); it runs through the same SDK calls
 * as test/difftest-adapter.ts.
 */
import { describe, expect, it } from "vitest";
import { canon, OPS } from "../difftest-adapter.js";
import { loadVectors, seen } from "./vectors.js";

const FILES = [
  "framing_encode.json",
  "usb_ids.json",
  "common_commands.json",
  "sr04.json",
  "bno055.json",
  "bno086_shtp.json",
  "tmf8829.json",
  "vl53l4.json",
  "vl53l7.json",
  "vl53lx.json",
  "vl53l8_advanced.json",
];

describe("input_rules (contract 07 §3)", () => {
  for (const file of FILES) {
    it(file, async () => {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const cases = (loadVectors(file) as any).input_rules as any[];
      expect(cases.length, file).toBeGreaterThan(0);
      for (const c of cases) {
        const op = OPS[c.op]!;
        if ("error" in c.expect) {
          await expect(Promise.resolve().then(() => op(c.in)), c.id).rejects.toThrow();
        } else {
          expect(JSON.parse(canon(await op(c.in))), c.id).toEqual(c.expect);
        }
        seen(c);
      }
    });
  }
});
