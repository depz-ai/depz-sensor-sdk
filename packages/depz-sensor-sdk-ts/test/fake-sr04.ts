/**
 * In-process fake SR04 firmware speaking the wire protocol over a
 * LoopbackTransport pair. Port of the Python `tests/fake_device.py`.
 *
 * Mirrors the observable behavior of `usonic_sr04` cmd_handler.c closely
 * enough for DepzDevice/Sr04 unit tests: same reply kinds, same busy
 * semantics, same RPT_DATA layouts.
 */

import {
  CrcType,
  LoopbackTransport,
  PacketParser,
  buildPacket,
  type PacketEvent,
} from "../src/index.js";

export class FakeSr04 {
  static readonly SOFTWARE_NAME = "APP_usonic_SR04_v0.95";
  static readonly DEVICE_NAME = "DEPZ Usonic SR04";
  static readonly SERIAL = "SN0042";

  /** Host-side transport: hand this to DepzDevice/Sr04. */
  readonly transport: LoopbackTransport;
  /** Device-side transport (exposed for fault-injection tests). */
  readonly side: LoopbackTransport;

  samplePeriodUs = 50_000;
  echoDecayUs = 5_000;
  loopRunning = false;
  echoTimeUs = 5831; // ~1 m
  mcuTimeUs = 1_000_000n;
  temperatureRaw = 273; // 27.3 °C

  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor() {
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  // ── device-side TX ─────────────────────────────────────────────────────────

  private async send(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return; // link closed
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  private text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    payload[payload.length - 1] = 0x00;
    return this.send(0x81, payload);
  }

  /** Emit an RPT_DATA as the ISR callback would. */
  async sendMeasurement(sourceCmd: number, echo?: number): Promise<void> {
    this.mcuTimeUs += BigInt(this.samplePeriodUs);
    const payload = new Uint8Array(11);
    const dv = new DataView(payload.buffer);
    dv.setUint8(0, sourceCmd);
    dv.setBigUint64(1, this.mcuTimeUs, true);
    dv.setUint16(9, echo ?? this.echoTimeUs, true);
    await this.send(0x91, payload);
  }

  // ── device-side RX loop ────────────────────────────────────────────────────

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
      await this.text(cmd, FakeSr04.DEVICE_NAME);
    } else if (cmd === 0x04) {
      await this.text(cmd, FakeSr04.SOFTWARE_NAME);
    } else if (cmd === 0x05) {
      await this.text(cmd, FakeSr04.SERIAL);
    } else if (cmd === 0x06) {
      // SYNC_TIME
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
    } else if (cmd === 0x31) {
      // SET_SYNC_PIN_CONFIG
      const [pin, mode, pol] = [payload[0]!, payload[1]!, payload[2]!];
      if (pin < 1 || pin > 5 || mode > 4 || pol > 1) await this.status(cmd, 0x04);
      else await this.status(cmd, 0x00);
    } else if (cmd === 0x30) {
      // GET_SYNC_PIN_CONFIG
      const pin = payload[0]!;
      if (pin < 1 || pin > 5) await this.status(cmd, 0x04);
      else await this.send(0x90, Uint8Array.of(pin, 0, 0));
    } else if (cmd === 0x32) {
      const out = new Uint8Array(4);
      new DataView(out.buffer).setUint32(0, this.samplePeriodUs, true);
      await this.send(0x92, out);
    } else if (cmd === 0x33) {
      this.samplePeriodUs = dv.getUint32(0, true);
      await this.status(cmd, 0x00);
    } else if (cmd === 0x34) {
      const out = new Uint8Array(2);
      new DataView(out.buffer).setUint16(0, this.echoDecayUs, true);
      await this.send(0x93, out);
    } else if (cmd === 0x35) {
      const req = dv.getUint16(0, true);
      this.echoDecayUs = Math.min(Math.max(req, 4000), 65000);
      await this.status(cmd, 0x00);
    } else if (cmd === 0x36) {
      // MEASURE_ONCE
      if (this.loopRunning) await this.status(cmd, 0x06); // ERR_BUSY
      else await this.sendMeasurement(0x36); // no OK ack — data IS the reply
    } else if (cmd === 0x37) {
      this.loopRunning = true;
      await this.status(cmd, 0x00);
    } else if (cmd === 0x38) {
      this.loopRunning = false;
      await this.status(cmd, 0x00);
    } else {
      await this.status(cmd, 0x02); // ERR_INVALID_CMD
    }
  }
}
