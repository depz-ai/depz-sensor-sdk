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
  LoopbackTransport,
  PacketParser,
  StatusError,
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
          await this.send(0x87, Uint8Array.of(this.crc));
        } else if (cmd === 0x09) {
          const t = payload[0]!;
          if (t > 3) {
            await this.status(cmd, 0x04); // ERR_INVALID_PARAM
          } else {
            await this.status(cmd, 0x00); // acked in the old mode
            this.crc = t as CrcType;
          }
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

  it("an out-of-range mode is refused by the device (StatusError)", async () => {
    await expect(dev.setReportPayloadCrc(7 as CrcType)).rejects.toThrow(StatusError);
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
});
