/**
 * Common commands of contract 02 §2 not covered by the SR04 fake: payload CRC
 * mode get/set (0x08/0x09 → RPT_PAYLOAD_CRC_TYPE 0x87), DEVICE_RESET (0x02),
 * BOOTLOADER (0x01), and how the base device routes reports nobody claims
 * (unsolicited status, device-side sequence error, unknown report).
 *
 * The device is a tiny scripted firmware over a LoopbackTransport pair; after
 * SET_PAYLOAD_CRC_TYPE it frames its replies with that CRC, as real firmware
 * does, so the host parser's CRC path is exercised too.
 */

import { afterEach, beforeEach, describe, expect, it } from "vitest";
import {
  CrcType,
  DepzDevice,
  InvalidReplyError,
  LoopbackTransport,
  PacketParser,
  buildPacket,
  type DeviceEvent,
} from "../src/index.js";

class ScriptedFirmware {
  readonly transport: LoopbackTransport;
  readonly side: LoopbackTransport;
  readonly received: number[] = [];
  crc = CrcType.None;
  /** Corrupt the CRC trailer of the next reply (link CRC error). */
  corruptNext = false;
  /** Answer GET_MCU_TEMPERATURE with 3 of its 10 bytes (a short reply). */
  shortTemperature = false;
  /** Answer GET_PAYLOAD_CRC_TYPE with an empty payload (the ERRATA A4 example). */
  emptyCrcType = false;
  private seq = 0;
  private readonly parser = new PacketParser();

  constructor() {
    [this.transport, this.side] = LoopbackTransport.pair();
    void this.run();
  }

  async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    const pkt = buildPacket(cmd, payload, this.seq, this.crc);
    if (this.corruptNext) {
      pkt[pkt.length - 1] = pkt[pkt.length - 1]! ^ 0xff;
      this.corruptNext = false;
    }
    this.seq = (this.seq + 1) & 0xff;
    await this.side.write(pkt);
  }

  /** Skip `n` seq values: the next packet arrives after a gap. */
  skip(n: number): void {
    this.seq = (this.seq + n) & 0xff;
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type !== "packet") continue;
        this.received.push(ev.cmd);
        const { cmd, payload } = ev;
        if (cmd === 0x01 || cmd === 0x02) {
          await this.status(cmd, 0x00);
        } else if (cmd === 0x08) {
          await this.send(0x87, this.emptyCrcType ? new Uint8Array(0) : Uint8Array.of(this.crc));
        } else if (cmd === 0x09) {
          const t = payload[0]!;
          if (t > 3) {
            await this.status(cmd, 0x04); // ERR_INVALID_PARAM
          } else {
            await this.status(cmd, 0x00); // acked in the old mode
            this.crc = t as CrcType;
          }
        } else if (cmd === 0x07 && this.shortTemperature) {
          await this.send(0x83, new Uint8Array(3));
        } else if (cmd === 0x07) {
          const out = new Uint8Array(10);
          const dv = new DataView(out.buffer);
          dv.setBigUint64(0, 123n, true);
          dv.setInt16(8, 215, true);
          await this.send(0x83, out);
        } else {
          await this.status(cmd, 0x02);
        }
      }
    }
  }
}

let fw: ScriptedFirmware;
let dev: DepzDevice;
let events: DeviceEvent[];

beforeEach(async () => {
  fw = new ScriptedFirmware();
  dev = new DepzDevice(fw.transport, { timeoutMs: 500 });
  events = [];
  dev.onEvent((e) => events.push(e));
  await dev.open();
});

afterEach(async () => {
  await dev.close();
  await fw.side.close();
});

async function until(cond: () => boolean): Promise<void> {
  for (let i = 0; i < 200 && !cond(); i++) await new Promise((r) => setTimeout(r, 5));
  expect(cond()).toBe(true);
}

describe("payload CRC mode (0x08 / 0x09)", () => {
  it("reads the current mode", async () => {
    expect(await dev.getReportPayloadCrc()).toBe(CrcType.None);
  });

  it("each mode set is reported back, and CRC-framed replies still parse", async () => {
    for (const mode of [CrcType.Crc8, CrcType.Crc16, CrcType.Crc32, CrcType.None]) {
      await dev.setReportPayloadCrc(mode);
      expect(await dev.getReportPayloadCrc()).toBe(mode);
      expect(await dev.readMcuTemperature()).toBeCloseTo(21.5, 5);
    }
    expect(events.some((e) => e.type === "linkCrcError")).toBe(false);
  });

  it("an out-of-range mode is refused on the host (RangeError), nothing reaches the wire", async () => {
    // contract 02 §2 + 07 §3 (ERRATA A2): 4..255 are not CrcTypes, 300 does not fit the u8 field
    const sets = fw.received.filter((c) => c === 0x09).length;
    for (const bad of [4, 7, 255, 300, -1]) {
      await expect(dev.setReportPayloadCrc(bad as CrcType)).rejects.toThrow(RangeError);
    }
    expect(fw.received.filter((c) => c === 0x09).length).toBe(sets);
    expect(await dev.getReportPayloadCrc()).toBe(CrcType.None);
  });

  it("a reply with a bad CRC is a linkCrcError event, not a parsed report", async () => {
    await dev.setReportPayloadCrc(CrcType.Crc16);
    fw.corruptNext = true;
    await fw.send(0x83, new Uint8Array(10));
    await until(() => events.some((e) => e.type === "linkCrcError"));
    const ev = events.find((e) => e.type === "linkCrcError")!;
    expect(ev).toMatchObject({ type: "linkCrcError", cmd: 0x83 });
    expect(events.some((e) => e.type === "temperature")).toBe(false);
  });
});

describe("a reply shorter than its fields (contract 07 §3, ERRATA A4)", () => {
  it("fails that one request with InvalidReplyError; the read pump goes on and the next request is answered", async () => {
    fw.shortTemperature = true;
    const err = await dev.readMcuTemperature().then(
      () => null,
      (e: unknown) => e,
    );
    expect(err).toBeInstanceOf(InvalidReplyError);
    expect((err as InvalidReplyError).cmd).toBe(0x07);
    expect((err as InvalidReplyError).message).toContain("RPT_TEMPERATURE");
    expect((err as InvalidReplyError).cause).toBeInstanceOf(RangeError);
    // The ERRATA example: an empty RPT_PAYLOAD_CRC_TYPE.
    fw.emptyCrcType = true;
    await expect(dev.getReportPayloadCrc()).rejects.toThrow(InvalidReplyError);
    // The pump is alive: well-formed replies complete the next requests.
    fw.shortTemperature = false;
    fw.emptyCrcType = false;
    expect(await dev.readMcuTemperature()).toBeCloseTo(21.5, 5);
    expect(await dev.getReportPayloadCrc()).toBe(CrcType.None);
    expect(dev.stats.rxPackets).toBe(4);
    expect(events.some((e) => e.type === "disconnected")).toBe(false);
  });
});

describe("reset / bootloader (0x02 / 0x01)", () => {
  it("reset() completes on the device's OK status", async () => {
    await dev.reset();
    expect(fw.received).toContain(0x02);
  });

  it("enterBootloaderMode() sends BOOTLOADER and closes the connection", async () => {
    await dev.enterBootloaderMode();
    expect(fw.received).toContain(0x01);
    await expect(dev.getReportPayloadCrc()).rejects.toThrow();
  });
});

describe("unclaimed reports on the base device", () => {
  it("unsolicited status, device sequence error, temperature and unknown reports surface as events", async () => {
    await fw.send(0x80, Uint8Array.of(0xff, 0x01)); // unsolicited status
    await fw.send(0x84, Uint8Array.of(5, 9)); // device saw a host TX gap
    await fw.send(0x83, new Uint8Array(10)); // unsolicited temperature
    await fw.send(0xa5, Uint8Array.of(0xde, 0xad)); // unknown report id
    await until(() => events.length >= 4);

    expect(events).toContainEqual({ type: "unsolicitedStatus", status: 0x01 });
    expect(events).toContainEqual({
      type: "sequenceError",
      expectedSeq: 5,
      receivedSeq: 9,
      reportedByDevice: true,
    });
    expect(events.some((e) => e.type === "temperature")).toBe(true);
    const unknown = events.find((e) => e.type === "text" && e.cmd === 0xa5);
    expect(unknown).toBeDefined();
    expect((unknown as { text: string }).text.toLowerCase()).toContain("dead");
  });

  it("a gap the host sees in the device seq is a seqGap event ahead of its packet (ERRATA A1)", async () => {
    await fw.send(0xa5, Uint8Array.of(0x01)); // the first packet after open: never a gap
    fw.skip(3);
    await fw.send(0xa5, Uint8Array.of(0x02)); // seq 1, 2, 3 never sent
    await until(() => events.filter((e) => e.type === "text").length >= 2);

    const gaps = events.filter((e) => e.type === "seqGap");
    expect(gaps).toEqual([{ type: "seqGap", expectedSeq: 1, receivedSeq: 4, missed: 3 }]);
    expect(events.indexOf(gaps[0]!)).toBe(events.findIndex((e) => e.type === "text" && e.text === "02") - 1);
    expect(events.some((e) => e.type === "sequenceError")).toBe(false);
    expect(dev.stats.seqGaps).toBe(1);
  });
});
