import { describe, expect, it } from "vitest";
import {
  packSyncTime,
  packSyncPinConfig,
  syncTimeOffsetRtt,
  unpackSequenceError,
  unpackStatus,
  unpackTemperature,
  unpackText,
} from "../../src/index.js";
import { fromHex, loadVectors, toHex } from "./vectors.js";

const data = loadVectors("common_commands.json");

describe("common command encode vectors", () => {
  for (const c of data.encode) {
    it(c.name, () => {
      if (c.kind === "sync_time_request") {
        expect(toHex(packSyncTime(BigInt(c.pc_timestamp_us)))).toBe(c.payload);
      } else if (c.kind === "set_payload_crc_type") {
        expect(toHex(Uint8Array.of(c.crc_type))).toBe(c.payload);
      } else if (c.kind === "sync_pin_config") {
        expect(
          toHex(packSyncPinConfig({ pin: c.pin, mode: c.mode, polarity: c.polarity })),
        ).toBe(c.payload);
      } else {
        throw new Error(`unknown encode kind ${c.kind}`);
      }
    });
  }
});

describe("common report decode vectors", () => {
  for (const c of data.decode) {
    it(c.name, () => {
      const payload = fromHex(c.payload);
      if (c.report === 0x80) {
        expect(unpackStatus(payload)).toEqual({ cmd: c.expect.cmd, status: c.expect.status });
      } else if (c.report === 0x81) {
        expect(unpackText(payload)).toEqual({ cmd: c.expect.cmd, text: c.expect.text });
      } else if (c.report === 0x83) {
        const rep = unpackTemperature(payload);
        expect(rep.timestampUs).toBe(BigInt(c.expect.timestamp_us));
        expect(rep.rawDecidegrees).toBe(c.expect.raw_decidegrees);
      } else if (c.report === 0x84) {
        expect(unpackSequenceError(payload)).toEqual({
          expectedSeq: c.expect.expected_seq,
          receivedSeq: c.expect.received_seq,
        });
      } else {
        throw new Error(`unknown report ${c.report}`);
      }
    });
  }
});

describe("sync time math vectors", () => {
  for (const c of data.sync_time_math) {
    it(c.name, () => {
      const { offsetUs, rttUs } = syncTimeOffsetRtt(
        BigInt(c.t1),
        BigInt(c.t2),
        BigInt(c.t3),
        BigInt(c.t4),
      );
      expect(offsetUs).toBe(BigInt(c.offset_us));
      expect(rttUs).toBe(BigInt(c.rtt_us));
    });
  }
});
