/**
 * In-process fake BNO086 bridge + sensor hub over a LoopbackTransport pair.
 * Port of the Python `tests/fake_bno086.py`.
 *
 * Mirrors the observable behavior of APP_BNO086 (per ERRATA E2):
 * SEND_SHTP_PACKET is ACKed with RPT_STATUS immediately (OK, or ERR_BUSY
 * when told to), and every sensor-originated SHTP frame — solicited or not —
 * arrives as RPT_DATA(cmd=0). The SH-2 side answers product id, feature
 * get/set, commands and FRS with canned-but-consistent responses.
 */

import {
  CrcType,
  LoopbackTransport,
  NUM_CHANNELS,
  PacketParser,
  SHTP_HEADER_SIZE,
  ShtpChannel,
  buildPacket,
  fragmentCargo,
  unpackShtpHeader,
  type PacketEvent,
} from "../src/index.js";

const CMD_SEND_SHTP = 0x34;

function hexBytes(hex: string): Uint8Array {
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
  return out;
}

export class FakeBno086 {
  static readonly SOFTWARE_NAME = "APP_BNO086_v0.95";
  static readonly DEVICE_NAME = "DEPZ BNO086";
  static readonly SERIAL = "SN0086";
  static readonly ADVERTISEMENT = hexBytes("000101020a"); // opaque channel-0 blob
  static readonly FRS_RECORDS: Record<number, number[]> = {
    0x2d3e: [0x10000000, 0, 0, 0x40000000], // system orientation (Q30)
    0xe302: [
      0x00040404,
      0x00500000,
      0x4000,
      (0x0800 << 16) | 4,
      2500,
      0x01190032,
      0x00001000,
      (0x0011 << 16) | 8,
      0,
      100000,
    ], // accelerometer metadata rev 4
  };

  /** Host-side transport: hand this to Bno086. */
  readonly transport: LoopbackTransport;
  /** Device-side transport (exposed for fault-injection tests). */
  readonly side: LoopbackTransport;

  mcuTimeUs = 1_000_000n;
  /** ERR_BUSY the next N SEND_SHTP_PACKETs. */
  busyRemaining = 0;
  /** granted = requested * factor. */
  intervalFactor = 1.0;
  /** Every SEND_SHTP_PACKET incl. busy ones. */
  sendShtpAttempts = 0;
  /** sensor -> granted intervalUs. */
  features = new Map<number, number>();
  lastSetFeature: Uint8Array | null = null;
  tareRequests: Uint8Array[] = [];
  frsWrites = new Map<number, number[]>();

  private frsWrType: number | null = null;
  private frsWrLen = 0;
  private frsWrWords = new Map<number, number>();
  private txSeq = 0;
  private shtpTxSeq: number[] = new Array<number>(NUM_CHANNELS).fill(0); // device->host
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
    // Claim the seq synchronously so back-to-back un-awaited sends stay ordered.
    const seq = this.txSeq;
    this.txSeq = (seq + 1) & 0xff;
    try {
      await this.side.write(buildPacket(cmd, payload, seq, CrcType.None));
    } catch {
      // link closed
    }
  }

  private status(cmd: number, status: number): Promise<void> {
    return this.send(0x80, Uint8Array.of(cmd, status));
  }

  private async text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    await this.send(0x81, payload);
  }

  /**
   * Emit an SHTP cargo as RPT_DATA(cmd=0) frames; `maxFrame` < cargo size
   * exercises multi-frame reassembly on the host.
   */
  pushShtpCargo(channel: number, payload: Uint8Array, maxFrame = 4096): void {
    const seq = this.shtpTxSeq[channel]!;
    const frames = fragmentCargo(channel, payload, seq, maxFrame);
    this.shtpTxSeq[channel] = (seq + frames.length) & 0xff;
    for (const frame of frames) {
      this.mcuTimeUs += 100n;
      const out = new Uint8Array(9 + frame.length);
      const v = new DataView(out.buffer);
      v.setUint8(0, 0x00);
      v.setBigUint64(1, this.mcuTimeUs, true);
      out.set(frame, 9);
      void this.send(0x91, out);
    }
  }

  /**
   * Push a channel-3 cargo; returns the capture timestamp of the LAST frame
   * (the one that completes the cargo).
   */
  pushInputCargo(payload: Uint8Array, maxFrame = 4096): bigint {
    this.pushShtpCargo(ShtpChannel.InputNormal, payload, maxFrame);
    return this.mcuTimeUs;
  }

  pushGyroRvCargo(payload: Uint8Array): bigint {
    this.pushShtpCargo(ShtpChannel.GyroRv, payload);
    return this.mcuTimeUs;
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
    if (cmd === 0x03) {
      await this.text(cmd, FakeBno086.DEVICE_NAME);
    } else if (cmd === 0x04) {
      await this.text(cmd, FakeBno086.SOFTWARE_NAME);
    } else if (cmd === 0x05) {
      await this.text(cmd, FakeBno086.SERIAL);
    } else if (cmd === 0x06) {
      // SYNC_TIME: echo t1, stamp t2/t3 from the device clock (lets the IMU
      // join syncTimeAll / a DatasetRecorder shared timeline).
      const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
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
      odv.setInt16(8, 250, true); // 25.0 °C
      await this.send(0x83, out);
    } else if (cmd === 0x32) {
      // SENSOR_RESET: ack, then reset advertisement flow
      await this.status(cmd, 0x00);
      this.shtpTxSeq = new Array<number>(NUM_CHANNELS).fill(0);
      this.pushShtpCargo(ShtpChannel.Command, FakeBno086.ADVERTISEMENT);
      this.pushShtpCargo(ShtpChannel.Executable, Uint8Array.of(0x01)); // reset done
    } else if (cmd === 0x33) {
      // SENSOR_WAKE_UP
      await this.status(cmd, 0x00);
    } else if (cmd === CMD_SEND_SHTP) {
      this.sendShtpAttempts += 1;
      if (this.busyRemaining > 0) {
        this.busyRemaining -= 1;
        await this.status(cmd, 0x06); // ERR_BUSY, frame dropped
        return;
      }
      await this.status(cmd, 0x00);
      this.onShtp(payload);
    } else {
      await this.status(cmd, 0x02); // ERR_INVALID_CMD
    }
  }

  // ── SH-2 side ──────────────────────────────────────────────────────────────

  private onShtp(frame: Uint8Array): void {
    if (frame.length < SHTP_HEADER_SIZE) return;
    const hdr = unpackShtpHeader(frame);
    const payload = frame.subarray(SHTP_HEADER_SIZE);
    if (hdr.channel !== (ShtpChannel.Control as number) || payload.length === 0) return;
    const v = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
    const rid = payload[0]!;
    if (rid === 0xf9) {
      // product id request
      const resp = new Uint8Array(16);
      const rv = new DataView(resp.buffer);
      rv.setUint8(0, 0xf8);
      rv.setUint8(1, 0x01);
      rv.setUint8(2, 3);
      rv.setUint8(3, 8);
      rv.setUint32(4, 10003608, true);
      rv.setUint32(8, 475, true);
      rv.setUint16(12, 4, true);
      this.pushShtpCargo(ShtpChannel.Control, resp);
    } else if (rid === 0xfd) {
      // set feature
      this.lastSetFeature = payload.slice();
      const sensor = payload[1]!;
      const interval = v.getUint32(5, true);
      const granted = Math.trunc(interval * this.intervalFactor);
      if (granted === 0) this.features.delete(sensor);
      else this.features.set(sensor, granted);
      this.sendFeatureResponse(sensor);
    } else if (rid === 0xfe) {
      // get feature request
      this.sendFeatureResponse(payload[1]!);
    } else if (rid === 0xf2) {
      // command request
      this.onCommand(payload);
    } else if (rid === 0xf4) {
      // FRS read request
      this.frsRead(v.getUint16(4, true));
    } else if (rid === 0xf7) {
      // FRS write request
      this.frsWrType = v.getUint16(4, true);
      this.frsWrLen = v.getUint16(2, true);
      this.frsWrWords = new Map();
      this.frsWriteResponse(4, 0); // write mode ready
    } else if (rid === 0xf6) {
      // FRS write data
      const offset = v.getUint16(2, true);
      const d0 = v.getUint32(4, true);
      const d1 = v.getUint32(8, true);
      if (this.frsWrType === null) {
        this.frsWriteResponse(6, offset); // not in write mode
        return;
      }
      this.frsWrWords.set(offset, d0);
      if (offset + 1 < this.frsWrLen) this.frsWrWords.set(offset + 1, d1);
      if (this.frsWrWords.size >= this.frsWrLen) {
        const words: number[] = [];
        for (let i = 0; i < this.frsWrLen; i++) words.push(this.frsWrWords.get(i)!);
        this.frsWrites.set(this.frsWrType, words);
        this.frsWriteResponse(3, offset); // write completed
        this.frsWrType = null;
      } else {
        this.frsWriteResponse(0, offset); // words received
      }
    }
  }

  private sendFeatureResponse(sensor: number): void {
    const interval = this.features.get(sensor) ?? 0;
    const resp = new Uint8Array(17);
    const v = new DataView(resp.buffer);
    v.setUint8(0, 0xfc);
    v.setUint8(1, sensor);
    v.setUint32(5, interval, true);
    this.pushShtpCargo(ShtpChannel.Control, resp);
  }

  private onCommand(payload: Uint8Array): void {
    const seq = payload[1]!;
    const command = payload[2]!;
    const params = payload.subarray(3, 12);
    if (command === 0x03) {
      // tare — no response
      this.tareRequests.push(payload.slice());
      return;
    }
    if (command === 0x09) return; // periodic DCD config — no response
    if (command === 0x06) {
      // DCD save
      this.commandResponse(seq, command, [0]);
      return;
    }
    if (command === 0x07) {
      // ME calibration
      if (params[3] === 0x01) this.commandResponse(seq, command, [0, 1, 1, 0, 0]); // get
      else this.commandResponse(seq, command, [0]);
      return;
    }
    if (command === 0x0a) {
      // get oscillator type — r[0] IS the type (EXT_CRYSTAL)
      this.commandResponse(seq, command, [1]);
      return;
    }
    if (command === 0x0b) {
      // clear DCD and reset — no command response; the device resets
      this.shtpTxSeq = new Array<number>(NUM_CHANNELS).fill(0);
      this.pushShtpCargo(ShtpChannel.Command, FakeBno086.ADVERTISEMENT);
      this.pushShtpCargo(ShtpChannel.Executable, Uint8Array.of(0x01)); // reset done
      return;
    }
    if (command === 0x01) {
      // errors — stream records, then a source==255 terminator
      const recs = [
        [1, 0, 3, 0x10, 2, 5],
        [2, 1, 4, 0x20, 3, 6],
      ];
      recs.forEach((r, i) => this.commandResponse(seq, command, r, i));
      this.commandResponse(seq, command, [0, 0, 255], recs.length);
      return;
    }
    if (command === 0x02) {
      // event counts
      if (params[0] === 0) {
        // get: two responses (responseSeq 0 then 1)
        const u32 = (a: number, b: number): number[] => {
          const out = new Uint8Array(8);
          const v = new DataView(out.buffer);
          v.setUint32(0, a, true);
          v.setUint32(4, b, true);
          return Array.from(out);
        };
        this.commandResponse(seq, command, [0, 0, 0, ...u32(100, 90)], 0);
        this.commandResponse(seq, command, [0, 0, 0, ...u32(80, 70)], 1);
      } else {
        this.commandResponse(seq, command, [0]); // clear
      }
      return;
    }
    this.commandResponse(seq, command, [0xff]); // unknown -> error status
  }

  private commandResponse(cmdSeq: number, command: number, r: number[], respSeq = 0): void {
    const resp = new Uint8Array(16);
    resp[0] = 0xf1;
    resp[1] = 0;
    resp[2] = command;
    resp[3] = cmdSeq;
    resp[4] = respSeq;
    resp.set(r, 5);
    this.pushShtpCargo(ShtpChannel.Control, resp);
  }

  private frsWriteResponse(status: number, offset: number): void {
    const resp = new Uint8Array(4);
    const v = new DataView(resp.buffer);
    v.setUint8(0, 0xf5);
    v.setUint8(1, status);
    v.setUint16(2, offset, true);
    this.pushShtpCargo(ShtpChannel.Control, resp);
  }

  private frsRead(ftype: number): void {
    const words = FakeBno086.FRS_RECORDS[ftype];
    if (words === undefined) {
      this.frsReadResponse(1, 0, 0, [0, 0], ftype); // unrecognized
      return;
    }
    for (let off = 0; off < words.length; off += 2) {
      const chunk = words.slice(off, off + 2);
      const last = off + 2 >= words.length;
      this.frsReadResponse(last ? 3 : 0, chunk.length, off, chunk, ftype);
    }
  }

  private frsReadResponse(
    status: number,
    datalen: number,
    offset: number,
    words: number[],
    ftype: number,
  ): void {
    const resp = new Uint8Array(16);
    const v = new DataView(resp.buffer);
    v.setUint8(0, 0xf3);
    v.setUint8(1, (datalen << 4) | status);
    v.setUint16(2, offset, true);
    v.setUint32(4, (words[0] ?? 0) >>> 0, true);
    v.setUint32(8, (words[1] ?? 0) >>> 0, true);
    v.setUint16(12, ftype, true);
    this.pushShtpCargo(ShtpChannel.Control, resp);
  }
}
