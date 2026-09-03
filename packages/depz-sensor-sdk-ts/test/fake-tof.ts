/**
 * Minimal in-process fake VL53L8 firmware over a LoopbackTransport pair —
 * enough to let a real `Vl53l8Cx` / `Vl53l8Ch` `open()`, `syncTime()`,
 * `identify()` and `getSerialNumber()` in multi-device tests (recorder /
 * syncTimeAll). It answers only the shared common commands (identity, sync,
 * temperature); it does NOT emulate the ULD register bridge or frame streaming
 * — tests that need frames inject synthetic `Vl53l8Frame`s directly.
 *
 * `variant` decides the reported software name so `identify()` classifies the
 * device as `vl53l8` (both CX and CH firmware report the "VL53L8" family; the
 * silicon variant is a USB model hint, not part of the software name).
 */

import {
  CrcType,
  LoopbackTransport,
  PacketParser,
  buildPacket,
  type PacketEvent,
} from "../src/index.js";

export class FakeTof {
  /** Host-side transport: hand this to Vl53l8Cx / Vl53l8Ch. */
  readonly transport: LoopbackTransport;
  /** Device-side transport. */
  readonly side: LoopbackTransport;

  readonly softwareName: string;
  readonly deviceName = "DEPZ VL53L8";
  readonly serial: string;
  mcuTimeUs = 1_000_000n;
  temperatureRaw = 251; // 25.1 °C

  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor(opts: { variant?: "cx" | "ch"; serial?: string } = {}) {
    const variant = opts.variant ?? "cx";
    this.softwareName = `APP_VL53L8${variant.toUpperCase()}_v0.95`;
    this.serial = opts.serial ?? (variant === "ch" ? "SN53C8" : "SN53CX");
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return; // link closed
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    payload[payload.length - 1] = 0x00;
    return this.send(0x81, payload);
  }

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type === "packet") await this.handle(ev);
      }
    }
  }

  private async handle(pkt: PacketEvent): Promise<void> {
    const { cmd, payload } = pkt;
    const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
    if (cmd === 0x03) {
      await this.text(cmd, this.deviceName);
    } else if (cmd === 0x04) {
      await this.text(cmd, this.softwareName);
    } else if (cmd === 0x05) {
      await this.text(cmd, this.serial);
    } else if (cmd === 0x06) {
      // SYNC_TIME: echo t1, stamp t2/t3 from the device clock
      const t1 = dv.getBigUint64(0, true);
      const t2 = this.mcuTimeUs + 500n;
      // Model a zero device-processing gap (t3 === t2) so the host
      // round-trip always dominates: rtt = (t4-t1) - (t3-t2) = t4-t1 >= 0
      // on any host, including a ~0us in-process loopback. A fabricated
      // 20us gap exceeded the real loopback round-trip often enough to
      // make syncTimeAll's `rttUs >= 0` assertion flake ~25% of runs.
      // Matches the python reference fake (tests/fake_device.py).
      const t3 = t2;
      const out = new Uint8Array(24);
      const odv = new DataView(out.buffer);
      odv.setBigUint64(0, t1, true);
      odv.setBigUint64(8, t2, true);
      odv.setBigUint64(16, t3, true);
      await this.send(0x82, out);
    } else if (cmd === 0x07) {
      const out = new Uint8Array(10);
      const odv = new DataView(out.buffer);
      odv.setBigUint64(0, this.mcuTimeUs, true);
      odv.setInt16(8, this.temperatureRaw, true);
      await this.send(0x83, out);
    } else {
      await this.send(0x80, Uint8Array.of(cmd, 0x02)); // ERR_INVALID_CMD
    }
  }
}
